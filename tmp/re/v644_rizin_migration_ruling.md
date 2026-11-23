# v644 — radare2(rizin) 移行と V-643-1 / id 98 の byte レベル再走

- **日付**: 2026-11-23
- **帯域**: 探索ツール移行 / HandleClick `FUN_005cc200`
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 探索 = rizin 0.9.1（静的ビルド、`D:\re\r2q.sh`）。デコード最終確認 = capstone（`v307_common.py`）。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。

## §0 ツール移行

- radare2 は sudo 不可（`SUDO_NEEDS_PW`）のため、**rizin 0.9.1 静的ビルド**を sudo 無しで導入:
  `rizin-v0.9.1-static-x86_64.tar.xz` → `~/.local/rizin-root/`（`bin/rizin`）。※`--strip-components=1` は `bin/rizin` と `share/rizin/` が衝突するので使わない。
- ラッパー `D:\re\r2q.sh`: `rizin -q -e scr.color=0 -e scr.html=false -e asm.bytes=true -e asm.lines=false -e asm.comments=false -e asm.flags=false -e bin.relocs.apply=false "$@"`。
- 作業コピー `/mnt/d/re/Oblivion.exe`（原本は読み取り専用、md5 一致）。
- 規則: mnemonic 依存検索の禁止（`/x` のバイト検索）、窓固定の禁止（`[start,end)` 明記）、r2 出力の引用時は `px` 生バイト併記＋capstone デコード一致確認。

## §1 — `[esp+0x1c]` の書込（reg 非依存）`[未確定]`（V-643-1）

- `5C` は modrm の reg=ebx 固定だったため、**SIB+disp の `24 1c` で再走**（範囲 `[0x5CC200, 0x5CDEF0)`）。
- rizin `/x 241c` = **48 件**。capstone で全件分類:

| 命令 | 件数 |
|---|---|
| `fcomp qword ptr [esp+0x1c]` | 19（サイト 0x5CC2C7…0x5CC807） |
| `fld qword ptr [esp+0x1c]` | 28（0x5CC92B…0x5CD986） |
| `mov dword ptr [esp+0x1c], ebx` | **1**（0x5CCB0E） |

- `2418` = 33 件、`2420` = 74 件、`2424` = 5 件（いずれも 8 バイト書込は `fstp qword [esp+0x20]` 系のみ）。
- → **`[esp+0x1c]` を書くのは `0x5CCB0E` の 1 件のみで、19 fcomp サイトより後**。reg 非依存でも 8 バイト書込は増えない。
- → 19 fcomp サイトの `[esp+0x1c]` は**先行書込が存在しない**。**V-643-1 は `[未確定]`**（mnemonic 非依存で確定。capstone/rizin 一致）。
- 参考: head `pD 0xa8 @ 0x5cc200` にも `[esp+0x1c]` 書込なし（`0x5cc23c mov byte [esp+0x1b],0` のみ）。`0x5cc22c mov esi,ecx` → `esi=this`、`0x5cc2ad mov ecx,[esi+4]` = `Menu::tile`（根 tile）。

## §2 — id 98 は定数でない `[部分確定]`（V-642-1）

- **`push 0x62`（id 98）は全 `.text` で 0 件** → 98 は**静的定数ではない**（実行時の要素 id か入力コード）。
- `call dword [reg+0xC]`（vtable slot3）の site は **0x89957A〜0x8Exxx 帯のみ**（メニュー帯 0x5Cxxxx に 0 件）→ slot3 はエンジンの汎用入力ディスパッチ経由。
- `/r 0x5cc200` = **0 hits**（直接 caller なし）＝ vtable `0xA6D398`（RaceSexMenu slot3）経由のみ。
- `jnp` 先 `0x5CCA77`（`pD 0x60`）= `GetString(race_name,0xFDE)` の空白(0x20)チェック = キャラ名検証。

## §3 — `push 0xfa8`（id trait）`[確定(命令)]`

- `push 0xfa8` は全 `.text` で **126 site**。うち RaceSexMenu の生成器（`FUN_005c4340`/`0x5c4480`/`0x5c4630`/`0x5c4800`）が `SetFloat(tile,0xFA8,id)` で pane/item id を書く。arg1（クリック id）を `GetFloat(0xFA8)` から積む経路は vtable slot3 呼び出し（メニュー帯外）にあり、本帯では同定できない。

## §4 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-643-1 | `[esp+0x1c]` の 8B 初期化が存在しない（19 fcomp が未初期化を読む） | `[未確定]` |
| V-642-1 | id 98（定数なし。実行時 id か入力コード） | `[部分確定]` |
| V-641-1 | 24 サイト × クリック id の完全対応 | `[部分確定]` |
