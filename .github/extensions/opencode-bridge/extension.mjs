// Extension: opencode-bridge
// Bidirectional bridge between GitHub Copilot and OpenCode CLI
//
// [Copilot -> OpenCode]
//   - opencode_run tool: send commands to OpenCode and get results
//   - opencode_serve tool: manage OpenCode ACP server
//
// [OpenCode -> Copilot]
//   - HTTP webhook listener receives data from OpenCode
//   - Received data is injected into the Copilot session via session.send()
//   - OpenCode command template "copilot" sends data to this bridge

import { joinSession } from "@github/copilot-sdk/extension";
import { execFile, spawn } from "node:child_process";
import { promisify } from "node:util";
import { createServer } from "node:http";

const execFileAsync = promisify(execFile);

// ── Configuration ────────────────────────────────────────────────────────────

const OPENCODE_EXE = process.platform === "win32"
    ? "C:\\Users\\hiroki.kogarumai\\AppData\\Roaming\\npm\\node_modules\\opencode-ai\\bin\\opencode.exe"
    : "opencode";

const PROJECT_DIR = process.cwd();
const ACP_PORT = 4096;
const WEBHOOK_PORT = 4197;

// ── State ────────────────────────────────────────────────────────────────────

let acpServerProcess = null;
let webhookServer = null;
let pendingMessages = [];

// ── OpenCode run ─────────────────────────────────────────────────────────────

async function runOpenCode(message, options = {}) {
    const args = ["run", message, "--dir", options.dir || PROJECT_DIR];
    if (options.format === "json") args.push("--format", "json");
    if (options.model) args.push("--model", options.model);

    const { stdout, stderr } = await execFileAsync(OPENCODE_EXE, args, {
        timeout: options.timeout || 180000,
        maxBuffer: 20 * 1024 * 1024,
        shell: process.platform === "win32",
    });

    return { stdout, stderr: stderr || "" };
}

// ── OpenCode ACP Server ──────────────────────────────────────────────────────

function startAcpServer(port = ACP_PORT) {
    if (acpServerProcess) return { status: "already_running", port };

    const args = ["acp", "--port", String(port)];
    acpServerProcess = spawn(OPENCODE_EXE, args, {
        cwd: PROJECT_DIR,
        stdio: ["ignore", "pipe", "pipe"],
        detached: false,
        shell: process.platform === "win32",
    });

    acpServerProcess.on("exit", (code) => { acpServerProcess = null; });
    return { status: "started", port, pid: acpServerProcess.pid };
}

function stopAcpServer() {
    if (!acpServerProcess) return { status: "not_running" };
    acpServerProcess.kill();
    acpServerProcess = null;
    return { status: "stopped" };
}

// ── Webhook Listener (OpenCode -> Copilot) ───────────────────────────────────

function startWebhook(port = WEBHOOK_PORT) {
    if (webhookServer) return { status: "already_running", port };

    return new Promise((resolve) => {
        webhookServer = createServer(async (req, res) => {
            // CORS headers
            res.setHeader("Access-Control-Allow-Origin", "*");
            res.setHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
            res.setHeader("Access-Control-Allow-Headers", "Content-Type");

            if (req.method === "OPTIONS") {
                res.writeHead(204);
                res.end();
                return;
            }

            if (req.method !== "POST") {
                res.writeHead(405);
                res.end("Method not allowed");
                return;
            }

            let body = "";
            for await (const chunk of req) body += chunk;

            try {
                const data = JSON.parse(body);
                const content = data.message || data.content || data.text || body;

                // Store for polling and also inject into session
                pendingMessages.push({
                    timestamp: new Date().toISOString(),
                    source: data.source || "opencode",
                    content,
                });

                // Forward to Copilot session
                try {
                    await session.send({
                        prompt: `[OpenCode → Copilot]\n${content}`,
                    });
                } catch (sendErr) {
                    // session.send may fail if agent is busy; message is queued
                }

                res.writeHead(200, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ status: "ok", queued: true }));
            } catch (err) {
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ status: "error", message: err.message }));
            }
        });

        webhookServer.listen(port, "127.0.0.1", () => {
            resolve({ status: "started", port });
        });
    });
}

function stopWebhook() {
    if (!webhookServer) return { status: "not_running" };
    webhookServer.close();
    webhookServer = null;
    return { status: "stopped" };
}

// ── Session ──────────────────────────────────────────────────────────────────

const session = await joinSession({
    tools: [
        // ── Tool: opencode_run ──────────────────────────────────────────────
        {
            name: "opencode_run",
            description: "Send a command/message to OpenCode CLI and return the response. Use this for code analysis, generation, review, or any task OpenCode can handle.",
            parameters: {
                type: "object",
                properties: {
                    message: {
                        type: "string",
                        description: "The instruction to send to OpenCode",
                    },
                    dir: {
                        type: "string",
                        description: "Working directory (default: project root)",
                    },
                    format: {
                        type: "string",
                        enum: ["default", "json"],
                        description: "Output format: 'default' for text, 'json' for raw events",
                    },
                    model: {
                        type: "string",
                        description: "Model override (e.g. 'anthropic/claude-sonnet-5')",
                    },
                    timeout: {
                        type: "number",
                        description: "Timeout in ms (default: 180000)",
                    },
                },
                required: ["message"],
            },
            handler: async (args) => {
                await session.log(`OpenCode run: ${args.message.substring(0, 120)}...`);
                try {
                    const result = await runOpenCode(args.message, {
                        dir: args.dir,
                        format: args.format,
                        model: args.model,
                        timeout: args.timeout,
                    });
                    if (result.stderr) {
                        await session.log(`OpenCode stderr: ${result.stderr.substring(0, 300)}`, { level: "warning" });
                    }
                    return result.stdout || "(no output returned)";
                } catch (err) {
                    const msg = `OpenCode execution failed: ${err.message}`;
                    await session.log(msg, { level: "error" });
                    return msg;
                }
            },
        },

        // ── Tool: opencode_serve ────────────────────────────────────────────
        {
            name: "opencode_serve",
            description: "Manage the OpenCode ACP server. Start before connecting external tools to OpenCode.",
            parameters: {
                type: "object",
                properties: {
                    action: {
                        type: "string",
                        enum: ["start", "stop", "status"],
                        description: "start / stop / status",
                    },
                    port: {
                        type: "number",
                        description: "Port (default: 4096)",
                    },
                },
                required: ["action"],
            },
            handler: async (args) => {
                if (args.action === "start") {
                    const result = startAcpServer(args.port || ACP_PORT);
                    await session.log(`OpenCode ACP server: ${result.status} (port ${result.port})`);
                    return JSON.stringify(result);
                } else if (args.action === "stop") {
                    const result = stopAcpServer();
                    await session.log(`OpenCode ACP server: ${result.status}`);
                    return JSON.stringify(result);
                } else {
                    return JSON.stringify({
                        status: acpServerProcess ? "running" : "stopped",
                        port: ACP_PORT,
                    });
                }
            },
        },

        // ── Tool: opencode_webhook ──────────────────────────────────────────
        {
            name: "opencode_webhook",
            description: "Start/stop the webhook listener that receives data FROM OpenCode. When running, OpenCode can POST data to http://127.0.0.1:{port} and it will be injected into this Copilot session.",
            parameters: {
                type: "object",
                properties: {
                    action: {
                        type: "string",
                        enum: ["start", "stop", "status", "pending"],
                        description: "start: start listener, stop: stop listener, status: check state, pending: show queued messages",
                    },
                    port: {
                        type: "number",
                        description: "Port (default: 4197)",
                    },
                },
                required: ["action"],
            },
            handler: async (args) => {
                if (args.action === "start") {
                    const result = await startWebhook(args.port || WEBHOOK_PORT);
                    await session.log(`Webhook listener: ${result.status} on port ${result.port}`);
                    return JSON.stringify(result);
                } else if (args.action === "stop") {
                    const result = stopWebhook();
                    await session.log(`Webhook listener: ${result.status}`);
                    return JSON.stringify(result);
                } else if (args.action === "pending") {
                    const msgs = [...pendingMessages];
                    pendingMessages = [];
                    return JSON.stringify({ count: msgs.length, messages: msgs });
                } else {
                    return JSON.stringify({
                        status: webhookServer ? "running" : "stopped",
                        port: WEBHOOK_PORT,
                        pendingCount: pendingMessages.length,
                    });
                }
            },
        },
    ],

    hooks: {
        onUserPromptSubmitted: async (input) => {
            if (input.prompt.toLowerCase().includes("opencode")) {
                await session.log(
                    "OpenCode detected — tools available: opencode_run, opencode_serve, opencode_webhook",
                    { ephemeral: true }
                );
            }
        },
    },
});
