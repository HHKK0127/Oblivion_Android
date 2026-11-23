# v643 — `[esp+0x1c]` の実体（fstp qword は存在しない）と id 98

- **日付**: 2026-11-23
- **帯域**: RaceSexMenu HandleClick `FUN_005cc200`
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **起点**: v642（row619）の V-641-1 / V-642-1。

## §1 — `[esp+0x1c]` の書込は fstp qword ではない `[確定(命令)]`

- HandleClick（`0x5CC200..0x5CE800`）で `[esp+0x1c]` を書く命令は **4 件のみ**、いずれも 4 バイト:
  - `0x5CCB0E mov dword [esp+0x1c], ebx`（**24 サイトより後**。直後に `mov word [esp+0x20],bx` / `mov word [esp+0x22],bx` が続く → **BSStringT temp {data@0x1c, len@0x20, cap@0x22}** の構築）
  - `0x5CDFEA fstp dword [esp+0x1c]` / `0x5CE098 mov dword [esp+0x1c], eax` / `0x5CE0A0 fistp dword [esp+0x1c]` / `0x5CE3FE fstp dword [esp+0x1c]`（いずれも `0x5CDFxx`/`0x5CE0xx` 帯 = Update 側の別経路）
- **`fstp qword ptr [esp+0x1c]` は存在しない**（全域掃引）。生バイトでも比較命令は `DC 5C 24 1C` = `fcomp qword [esp+0x1c]`。
- → 24 サイト手前で `[esp+0x1c]` を初期化する 8 バイト書込は**無い**。ユーザーの「x87 ストア取りこぼし」仮説は**否定**。
- → `[esp+0x1c]` の初期化機構は `[未確定]`（V-643-1）。`fstp qword [esp+0x20]`（直後の GetFloat 結果）とは 4 バイトしか重ならず、単純な別名でもない。

## §2 — ガードの形と `jnp` 先 `[確定(命令)]`

```
0x5CC2A8 call 0x5c3110            ; pane 名 → pane
0x5CC2B0 push 0xfae ; call 0x588bd0 ; GetFloat([esi+4], user0)
0x5CC2BC fstp qword [esp+0x20]     ; 保存
0x5CC2C2 call 0x588bd0             ; GetFloat(edi, ...)  ※edi = 0x5c3110 の戻り
0x5CC2C7 fcomp qword [esp+0x1c]
0x5CC2CD test ah,0x44 ; jnp 0x5cca77
```
- `jnp` 先 `0x5CCA77`: `GetString(race_name, 0xFDE=string)` を走査し **空白(0x20)チェック**（キャラ名の検証）。
- → 不一致（jnp）は pane 切替ではなく**名前検証**へ抜ける。

## §3 — id 98 `[部分確定]`

- 抽出 89 XML に `<id>98</id>` は **0 件**。
- メニュー帯 `0x5B0000..0x5D4000` に **`push 0x62`（id 98 の生成）は 0 件**。
- → id 98 は XML tile id でも生成 site でもない → **コマンド/入力コード**の可能性が高い（V-642-1 継続）。
- `cmp ebx,0x62`(98) 分岐は `0xB333C4 vtable[0x170]()` → `0x526F00`（リスト操作: `[esi+0x1fa]` count / `[esi+0x1f4]` array）→ `0x5C91E0`（`0xB333C4` = Player 系, `vtable[0x170]`）。

## §4 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-643-1 | `[esp+0x1c]` の初期化機構（8B 書込が無い） | `[未確定]` |
| V-642-1 | id 98 の実体（コマンド/入力コードか） | `[部分確定]` |
| V-641-1 | 24 サイト × クリック id の完全対応 | `[部分確定]` |
