// utf8-console.ts - force UTF-8 for every shell/bash tool call (opencode V2)
//
// Why: opencode spawns pwsh as `-NoLogo -NoProfile -NonInteractive -Command ...`,
// so the user's profile (which sets [Console]::OutputEncoding = UTF-8) is skipped.
// Console CP stays 932 (shift_jis) while the tool reader expects UTF-8 -> mojibake.
// The CP also resets on every tool call, so the fix must be applied per command.
//
// Measured fix (2026-10-25):
//   chcp 65001 > $null; [Console]::OutputEncoding=[System.Text.Encoding]::UTF8;
// makes both pwsh string output and native command output render correctly.
//
// V2 port (2026-10-28): V1 exported `default async function` returning a hooks
// map; V2 rejects that with `Plugin must export a default definition with an id
// and an effect or setup function`. V2 uses Plugin.define({ id, setup }) and
// registers hooks on ctx (same pattern as rtk.ts in ~/.config/opencode/plugins).
// The tool is now named "shell" (accept "bash" too for safety).
// A global copy lives at ~/.config/opencode/plugins/utf8-console.ts and loads
// in every project; this project copy is kept for .opencode/plugins auto-discovery.
//
// Load marker: appends one line to %TEMP%\opencode\plugin_utf8_loaded.txt so the
// parent session can verify that this plugin was actually loaded after a restart.

import { Plugin } from "@opencode/plugin"
import type { Context } from "@opencode/plugin"
import * as fs from "node:fs"
import * as os from "node:os"
import * as path from "node:path"

const PREFIX =
  "chcp 65001 > $null; [Console]::OutputEncoding=[System.Text.Encoding]::UTF8; "

function mark(msg: string) {
  try {
    const p = path.join(os.tmpdir(), "opencode", "plugin_utf8_loaded.txt")
    fs.mkdirSync(path.dirname(p), { recursive: true })
    fs.appendFileSync(p, new Date().toISOString() + " " + msg + "\n")
  } catch {
    // never break anything because of the marker
  }
}

mark("loaded")

export default Plugin.define({
  id: "utf8-console",
  async setup(_ctx: Context) {
    mark("hooks-registered")
    await _ctx.tool.hook("execute.before", (event) => {
      try {
        const tool = String(event.tool ?? "").toLowerCase()
        if (tool !== "bash" && tool !== "shell") return
        const args = event.input
        if (!args || typeof args !== "object") return
        const record = args as Record<string, unknown>
        const command = record.command
        if (typeof command !== "string" || !command) return
        if (command.startsWith("chcp 65001")) return
        record.command = PREFIX + command
        mark("prefixed tool=" + tool)
      } catch (e) {
        mark("error " + String(e))
      }
    })
  },
})
