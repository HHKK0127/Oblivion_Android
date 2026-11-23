# v641 — HandleClick `0x5C3110` 24 サイトの pane 対応と command dispatch 構造

- **日付**: 2026-11-22
- **帯域**: RaceSexMenu HandleClick `FUN_005cc200` / page switch
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。pane 名は name registry（`0xB38F38+i*8`）の idx→display（thunk 帯 `0x9F6710+i*0x20`）で解決。
- **起点**: v640 完了後の実装入力（24 command 対応）。

## §1 — `0x5C3110` 呼び出し 24 サイト → pane 名 `[確定(命令)]`

`FUN_005cc200` 内の `call 0x5c3110` は 24 サイト。各サイトは pane 名グローバルを渡す（`0x5C3110` は 21 pane 名を unrolled strcmp で解決）。

| site | global | idx | pane |
|---|---|---|---|
| 0x5CC2A8 | 0xB38F70 | 7 | Main |
| 0x5CC2FC | (0xB38FE0 直前) | 21 | Shape |
| 0x5CC350 | 0xB38FE8 | 22 | Tone |
| 0x5CC399 | 0xB39000 | 25 | General(Face) |
| 0x5CC3E3 | 0xB39008 | 26 | Brow |
| 0x5CC42C | 0xB39010 | 27 | Cheeks |
| 0x5CC476 | 0xB39018 | 28 | Chin |
| 0x5CC4BF | 0xB38F90 | 11 | Eyes |
| 0x5CC509 | 0xB39020 | 29 | Forehead |
| 0x5CC552 | 0xB39028 | 30 | Jaw |
| 0x5CC59C | 0xB39008 | 26 | Brow |
| 0x5CC5E5 | 0xB39030 | 31 | Mouth |
| 0x5CC62F | 0xB39038 | 32 | Nose |
| 0x5CC678 | 0xB39040 | 33 | General(Skin) |
| 0x5CC6C2 | 0xB39050 | 35 | Beard |
| 0x5CC70B | 0xB39320 | 125 | Eye shades |
| 0x5CC755 | 0xB39058 | 36 | Eyebrows |
| 0x5CC79E | 0xB39060 | 37 | Mouth(Lips) |
| 0x5CC7E8 | 0xB39328 | 126 | Nose shades |
| 0x5CC844 | 0xB38F70 | 7 | Main |
| 0x5CC87F | 0xB38F70 | 7 | Main |
| 0x5CC99A | (0xB38FE0 直前) | 21 | Shape |
| 0x5CC9DF | 0xB38FE0 | 21 | Shape |
| 0x5CCA2F | 0xB38F80 | 9 | Face |

- 19 名（Main×3, Shape×3, Brow×2, 他 16 名×1）→ v634 §16 の「24 サイト」と一致。
- うち 0x5CC2FC / 0x5CC99A は直前のグローバルが pane 名でないため、**近傍の `0xB38FE0`(Shape) を渡す**と推定（`[部分確定]`）。

## §2 — HandleClick の head と command 源 `[確定(命令)]`

```
0x5CC200 push ebp ; mov ebp,esp ; and esp,0xfffffff8 ; ... ; mov esi,ecx
0x5CC22E mov ecx,[esi+0x30]              ; race_name
0x5CC237 push 0xff0 ; call 0x58ceb0       ; SetFloat(race_name, xdefault, 1.0)
0x5CC24D mov ecx,[esi+0x8ec] ; call 0x57d2f0
0x5CC25A mov ebx,[ebp+8]                  ; ← param_2 = command
0x5CC277 cmp ebx, 0x5a                    ; command == 90
0x5CC27A jne 0x5cc8ba
0x5CC280 mov eax,[0xb38f70] (Main) ... 0x5CC2A8 call 0x5c3110
```
- **`ebx = [ebp+8]` = HandleClick の第2引数（command）**。
- 即値 `cmp ebx,imm` は **3 つのみ**: `0x5CC277 cmp ebx,0x5a`(90) / `0x5CC8A3 cmp ebx,0xa`(10) / `0x5CCBF4 cmp ebx,0x62`(98)。
- → **24 サイトの大半は `cmp ebx,imm` ではなく、`GetFloat(tile,trait)` と `[esp+0x1c]` の値比較（`fcomp` + `jnp`）で守られている**。ページ切替は「クリックされた要素の値」と「現在値」の比較で成立。
- → command 値 → site の完全対応（`[esp+0x1c]` の源を含む）は **V-641-1**。

## §3 — 現時点のポート指針

- ページ切替は **pane 名 → `0x5C3110`（21 pane 名 strcmp）→ pane slot** の経路（v640 §3）。ポートは pane 名（display）をキーに pane を引ける。
- 24 サイトの pane 対応表（§1）で、どのボタン/入力がどの pane へ切り替えるかを実装可能。
- command 値そのもの（`ebx`）は 90/10/98 などの特殊値のみ即値で判明。通常のタブ切替は値比較経由。

## §4 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-641-1 | `[esp+0x1c]` の源と 24 サイト × command の完全対応 | `[未確定]` |
| V-641-2 | 0x5CC2FC / 0x5CC99A が渡す pane 名（Shape 推定） | `[部分確定]` |
| V-639-1 | `_text_` 選択規則（継続） | `[部分確定]` |
