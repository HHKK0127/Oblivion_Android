# v660 裁定書 — chargen Random/Reset 本体・Main "Eyes" トグル・Race 説明文

- 対象: `Oblivion.exe` v1.2.0416（md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`）
- probe: `D:\re\v660_probe.py`（**EXIT=0**）／補助 `D:\re\v660_1.py`〜`v660_4.py`
- 依頼元: 作製レーン（port #96）。ai-todo **#175 / #176 / #177**
- 前提（v570 §4・再掲）: Random=`0x579C10(text=[0xB39450], yes=[0xB38CF8], no=[0xB38D00], flag=1, cb=0x5C9FC0)`＋flg `[0xB3B4C8]`／Reset=`0x579C10(text=[0xB39448], flag=1, cb=0x5C9F70)`＋flg `[0xB3B4C9]`

---

## A. Random / Reset 実行本体（#175）

### A.0 同定 `[確定(命令)]`

- Random の cb `0x5C9FC0`: `FindMenu(0x40C)` → base tile → `0x9832E6` → `call 0x578D70` → `cmp al,2` → **`jne` なら `[0xB3B4C8]=0` で ret、`==2` なら `0x5C9FFC: jmp 0x5C9CD0`**。
- Reset の cb `0x5C9F70`: 同型で **`0x5C9FAC: jmp 0x5C9C70`**、失敗時 `[0xB3B4C9]=0`。
- **実行本体**: Random = **`0x5C9CD0`**、Reset = **`0x5C9C70`** `[確定(命令)]`。
- どちらも `FUN_005CC200` からも直接呼ばれる（`0x5CCBE5`→`0x5C9CD0`、`0x5CCCC8`→`0x5C9C70`）。
- **訂正**: `docs/chargen_ui_spec.md` §10.2 が `0x5C9CD0` を「commit」と記述していたのは**誤り**。`0x5C9CD0` = **Random 本体**。commit（確定→適用）は `0x5C28F0`（confirmed 経路）。

### A.1 触る trait の列挙 `[確定(命令)]`

- **Random 本体（187 命令, `0x5C9CD0..0x5C9F54`）が push する trait 即値は `0xFB1`（user3）のみ**。`SetFloat(tile,0xFB1,…)` は 6 回＝ **2 つの tile へ各 3 回**（`0x5C9E2C/0x5C9E40/0x5C9E52` と `0x5C9F0B/0x5C9F1F/0x5C9F31`）。
- tile は `0x5C3440(pane名,tile名)` の**名前引き**（`0x5C9E0A`, `0x5C9EE9`）。pane 名 `[0xB38F70]`、tile 名 `[0xB38F98]`/`[0xB38FA0]`（実行時ポインタ）。
- **顔モーフ軸（`0xFB5`=user7 等）への直接書込は無い**。軸の値は actor 側 API（後述）が持つ。
- **Reset 本体（`0x5C9C70..0x5C9CCE`）は trait を書かない**: `0x521E40(player actor)` → `0x5C91E0(base)` → `0x5C7BA0(base, 0)`。

### A.2 Reset が戻す「既定値」の実体 `[部分確定]`

- Reset 本体に **0.5 固定値も軸別テーブルも無い**。`0x521E40` が actor の外見リセットを行う（内部で `0x5528F0` / `0x552880` / `0x5538D0` を呼ぶ）。
- その後 `0x5C7BA0(base, 0)` が toggles/lists を再充填する（race 一覧もここで再構築）。
- → **既定値の実体は actor の外見データ側**。軸別テーブルの有無は `0x5528F0`/`0x5538D0` の解析が必要 → **V-660-A1 [未確定]**。

### A.3 乱数源とスケール `[部分確定]`

- Random 本体は `0x553B30`（→ `0x6EDD30`）と `0x522230` から値を取る。
- `0x6EDD30`: `cmp ebp,5` / `cmp ebx,2` / `cmp esi,2` で範囲検査（違反で push 0xbb/0xbc/0xbd）、`cmp byte [edi],0` → `fldz`。＝**顔モーフ accessor**であって乱数生成器ではない。
- `0x522230`: `0x519D20(actor)` の結果で **`[0xA53954]`(-2.0f)** か **`[0xA379B4]`(2.0f)** を返す。
- **スケール（実測定数）**:
  - `[esi+0x880] = (v - 15.0) / 50.0`（`[0xA492F0]`=**15.0**, `[0xA3F3D0]`=**50.0**）
  - `[esi+0x884] = (v - (-2.0)) * 0.25 * [esp+0x2c] + 0.0`（`[0xA3F400]`=**-2.0**, `[0xA3C770]`=**0.25**, `[0xA2FC68]`=**0.0**）
- → **乱数そのものの発生源・分布（一様か軸別か）は未同定** → **V-660-A2 [未確定]**。

### A.4 終端の再確認ループとキャンセル不変 `[確定(命令)]`

- cb は `0x578D70()` の戻りが 2 以外なら **フラグ（`[0xB3B4C8]`/`[0xB3B4C9]`）を 0 にクリアして ret**し、**本体へ jmp しない**。
- → **キャンセル時は本体（trait 書込）に到達しない＝状態は不変** `[確定(命令)]`。

### A.5 ダイアログのボタン id と文字列 `[部分確定]`

- `FUN_00579C10` = `message_menu.xml` の汎用ダイアログ。ボタンは `button_1..button_10` = **id 4..13**（`button_1` id 4, `button_2` id 5, … , `button_10` id 13）。各ボタンの `<string>` = `parent().string`。
- コールバックの判定は **`0x578D70()==2` → Yes**（`cmp al,2`）。→ **「2 = Yes」はコールバック側の規約**であり、ボタン id そのものではない。
- text 文字列（`[0xB39450]`/`[0xB39448]`/`[0xB38CF8]`/`[0xB38D00]`）は **実行時ポインタ**でファイルからは読めない → **v570 §4 の記載を正典**とする `[部分確定]`。

---

## B. Main ページの Toggle "Eyes"（#176 / V-610-A1）`[確定(命令)(negative)]`

- `FUN_005CC200` が arg1(ebx) と比較する即値は **`0x5A`(90) / `0xA`(10) / `0x62`(98) の 3 つのみ**（probe で全走査、`0x5CC277`/`0x5CC8A3`/`0x5CCBF4`）。
- → **Main トグル（id 204）専用の分岐は存在しない**。クリックは id 分岐ではなく**汎用経路**（`0x5CC8BA`〜、`0x5CC8BA` で `[0xB38FF0]` の tile の `0xFA8`(id) を読む）で処理される。
- → 制御対象は id からは決まらない。**V-610-A1 は [未確定] のまま**（次段: 汎用経路 `0x5CC8BA`〜 の解析）→ **V-660-B1**。

---

## C. Race ページ説明文本体の供給元（#177）`[部分確定]`

- Race ページの Text（builder 生成サイト `0x5CA60B`）の string は **`[0xB38FB0]`**（実行時ポインタ）。
- `[0xB38FB0]` の参照元: `FUN_005ca010`（builder）, `FUN_005c7ba0`, `FUN_005cda30`。→ builder が初期 string を入れ、**`0x5C7BA0` が更新**する可能性が高い（`0x5C7BA0` は race 一覧の populate も担う）。
- `0xFB4`(user6) の実際の用途: `FUN_005C7BA0` が **race/項目名**を入れる（`0x5C8100 SetString(tile,0xFB4,名前)`）、builder が Main ボタン名を入れる（`0x5CBD21`）。
  → **`0xFB4` は「項目の表示名」であって説明文本体ではない**（v283 の「sRaceDescription→0xFB4」は見出し/項目名側の記録）。
- → 本文明の書手・trait は **V-660-C1 [未確定]**（次段: `0x5C7BA0` の `[0xB38FB0]` サイトと RACE DESC の供給元）。

---

## 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-660-A1 | Reset の既定値の実体（actor 外見データ／軸別テーブルの有無）＝`0x521E40`→`0x5528F0`/`0x5538D0` | `[未確定]` |
| V-660-A2 | Random の乱数源と分布（`0x5273A0`/`0x553B30`/`0x522230` のどれが発生源か） | `[未確定]` |
| V-660-B1 | Main "Eyes" トグルの制御対象（汎用経路 `0x5CC8BA`〜） | `[未確定]` |
| V-660-C1 | Race 説明文本体の書手と trait（`0x5C7BA0` の `[0xB38FB0]` サイト／RACE DESC 供給元） | `[未確定]` |

## spec 訂正

- `docs/chargen_ui_spec.md` §10.2 の「commit `0x5C9CD0`」を訂正: **`0x5C9CD0` = Random 本体**（commit は `0x5C28F0`）。
