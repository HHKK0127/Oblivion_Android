# v642 — HandleClick arg1 の意味・`[esp+0x1c]`・比較 trait の確定

- **日付**: 2026-11-23
- **帯域**: RaceSexMenu HandleClick `FUN_005cc200` / page selection
- **判定語彙**: `[確定(命令)]` / `[確定(データ)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。XML は `tools/BSAFileExtractor/extracted/menus/`。
- **起点**: v641（row618）の V-641-1 と、ユーザーの `[ebp+8]`=arg1 訂正。

## §1 — `[ebp+8]` = arg1（クリック要素の id）`[確定(命令)]`

- prologue 実バイト: `55 8B EC 83 E4 F8 6A FF 68 38 1B 9C 00 64 A1 00 …`（`push ebp; mov ebp,esp; and esp,-8; push -1; …`）。
- → **`0x5CC25A mov ebx,[ebp+8]` = 第1スタック引数（arg1）**。ユーザーの訂正どおり「command」ではなく**クリックされた要素の id**。
- 即値 `cmp ebx,imm` は 3 つのみ: `0x5CC277 cmp ebx,0x5a`(90) / `0x5CC8A3 cmp ebx,0xa`(10) / `0x5CCBF4 cmp ebx,0x62`(98)。

### 3 分岐の動作 `[確定(命令)]`
| id | site | 動作 |
|---|---|---|
| 90 | 0x5CC277→0x5CC2A8 | Main 名(`0xB38F70`)を解決 → `edi=Main pane` → `GetFloat([esi+4],0xFAE)` と `GetFloat(Main,0xFAE)` を `[esp+0x1c]` と比較 → `jnp 0x5cca77` |
| 10 | 0x5CC8A3→0x5CC8AC | `call 0x5c2730` → `call 0x5c30c0` → `mov eax,[0xB38FF0]`(idx23) を解決 |
| 98 | 0x5CCBF4→0x5CCBF9 | `mov ecx,[0xB333C4]; vtable[0x170]()` → `call 0x526f00` → `call 0x5c91e0` → `jmp 0x5cd9c5` |

- **id 98 は抽出 89 XML に存在しない**（`<id>98</id>` 0 件）→ 98 は XML `<id>` ではなく**コード由来の特殊値**（or 別メニュー）。`[部分確定]` → V-642-1。
- id 90 = `race_button_done`（race_sex L213）、id 10 = `race_name`（L49）で確定。

## §2 — `[esp+0x1c]` と比較 trait `[確定(命令)]`

- `[esp+0x1c]` の**唯一の mov 書込**は `0x5CCB0E mov dword ptr [esp+0x1c], ebx`（`ebx` = arg1 = クリック id）。他は `fld qword [esp+0x1c]`（読）。
- 24 サイトのガードは `GetFloat(<tile>, 0xFAE=user0)` → `fcomp qword [esp+0x1c]` → `test ah,0x44; jnp <共通 skip>`。
- **比較 trait = `0xFAE`(user0)**（`0x5CC358 push 0xfae` 等で実測）。
- `jnp` は「不一致（fcomp で C3/C2 が揃わない）で共通 skip へ飛ぶ」＝**一致時のみフォールスルーして pane 切替**。
- → **ページ選択モデル = 「`user0 == クリック id` の pane を選ぶ」**。`FUN_005c4340` が `SetFloat(pane, 0xFAE, id)` で pane の user0 = id にする（v635 §6）ことと整合。
- `[esp+0x1c]` の qword 読は、`user0`(float) との比較のための型（4B 書込＋上位 4B はゼロ/未定義）→ 実装は float 比較として扱える。

## §3 — v640 の回収分（再掲・既了）`[確定(データ)]`

- idx12=`Age`/sAge、idx13=`Complexion`/sComplexion（thunk `0x9F6710+i*0x20` の display/editorid 実読）。
- 全 118 item ラベル表 = `tmp/re/v640_item_labels.md`（Main 6 件含む）。
- item 名と pane 名の衝突: 重複 idx `8`(Race)・`11`(Eyes)。`0x5C3110` は pane 名グローバルのみ走査するため衝突の実害なし（V-640-1 として留意）。

## §4 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-642-1 | id 98 の実体（XML 不在。コード由来 or 別メニュー） | `[部分確定]` |
| V-641-1 | 24 サイト × クリック id の完全対応（`user0==id` モデルで大半は説明可能。id 値の列挙は未完） | `[部分確定]` |
| V-639-1 | `_text_` 選択規則（独立） | `[部分確定]` |
| V-641-2 | 0x5CC2FC / 0x5CC99A の pane 名（Shape 推定） | `[部分確定]` |
