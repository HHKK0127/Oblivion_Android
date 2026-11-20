# v583 — V-583-1（第3経路の実行時証拠＝発火タイミング全系統の確定）: **発火源はすべて「ゲーム内イベント」で、chargen フローには存在しない**（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v583_1〜v583_6**・6本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v582_command_chain_ruling.md`（V-583-1 の起票＝4 callers の上流）
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| 4 callers の上流（発火元）は何か | **[確定(命令)]** | **8 系統すべてを同定**（§2 の表）。上流は **ローディング／Sleep-Wait／state3 進行／セーブ欠損ダイアログ／国境離脱ダイアログ／menu 0x416 操作／Fast Travel 確認** のすべてが**ゲーム内イベント**。 |
| `FUN_005B8DC0`（callers なしだった第4 caller）の正体 | **[確定(命令)]** | **Fast Travel 確認ダイアログの callback** — `FUN_005BB880`（**MapMenu の通知ハンドラ**・`cmp edi,21`/`cmp edi,43`）が `sprintf('%s %s?', sTravelQuestion, …)` → **`0x579C10(text, sYes, sNo, flag=1, callback=0x5B8DC0)`** へ `push 0x5b8dc0` で渡す。本体＝`0x578D70()` 応答==1 → **`0x589B70(0x3FF)`＝MapMenu 取得** → `0x5898F0` 根探索 → `[+0xc4]` → `vfunc+0x174` → … → **`FUN_005821F0`（command 系）@`0x5B8F91`**。→ **E8/E9 callers が0件だったのは vtable/登録経由だから**（`0x579C10` の引数＝callback 登録）。 |
| 「chargen 中に HUD が開かないこと」の命令的裏付け | **[確定(命令)（negative）]＋[部分確定]** | **第3経路（唯一実証された HUD 生成経路）の発火源8系統すべてが chargen フロー外**＝**chargen フローに発火源が0件 [確定(命令)（negative）]**。open 経路（`0x591C50`）は v578-v580 で「成功呼び出し不存在」を全8系統で確定済み → **HUD 生成は第3経路のみ実証** → **chargen 中に HUD は生成されない**という帰結が成立。ただし**「どのフラグ/条件で chargen 中にブロックするか」という positively なゲート条件そのものは未特定** → **[部分確定]**（残点 V-584-1）。 |
| `0x5791A0`/`0x5791E0`/`0x579220` の3薄ラッパの関係 | **[部分確定]** | 3本とも **`0x582160(1,0)` singleton の `[+0x1c]` ゲート**を通過して `mov ecx,eax` → **別先頭へ jmp**: `0x5791A0`→`0x583E60`／`0x5791E0`→**`FUN_005821F0`**／`0x579220`→`0x583F40`。**begin / execute / end 型の3点セット**と読める（§3）。呼び出し側は常に `0x5791A0 → 0x5791E0 → 0x579220` の順で3連呼。 |

---

## 2. 発火源の完全表（第3経路の実行時証拠）[確定(命令)]

第3経路 = `4 callers → FUN_005821F0 → FUN_00586000(コマンドパーサ) → &name; 解決 → FUN_00587550(窓 id switch) → 生成ヘルパ`

| # | 発火元（上流） | 文脈の決定打 | タイミング |
|---|---|---|---|
| 1 | `FUN_00440C40` ← `FUN_00444FB0`×3, `FUN_00671620` | **`0x57ACC0([0xB38BF8]='Loading area...', 0, 0, float)`** → `0x5791A0→0x5791E0→0x579220` | **ローディング／エリア移動** |
| 2 | `FUN_0040D4D0` | `0x578FE0()` と `cmp eax,0x3F4`（**SleepWaitMenu**）＋ `0x5D7090` チェック → `0x5791A0→0x5791E0` | **Sleep/Wait メニュー** |
| 3 | `FUN_00459A10` | `0x578FE0()==3` と `0x579060()==3` の2段チェック → `0x6B94E0`/`0x579770` → `0x5791A0→0x5791E0→0x579220` | **state3（ゲーム内進行）** |
| 4 | `FUN_0045C9C0` ← `FUN_00579CF0` | **`sSaveGameContentIsMissing`**（"This save relies on content that is no longer present."）＋ `sYes`/`sNo` を積んで `FUN_00579CF0` | **セーブ読込の欠損警告 Yes/No 応答** |
| 5 | `FUN_0045DBC0` ← `FUN_00579CF0` | 同型（`lea eax,[esp+0x13c]` buffer 引数） | 同上（別経路） |
| 6 | `FUN_0066FF10` ← `FUN_00579CF0` | **`sPlayerLeavingBorderRegion`**（"You cannot go that way."）＋ **`sOk`** ＋ `[esi+0x72c]!=0` ゲート | **国境離脱警告の OK 応答** |
| 7 | `FUN_005ABBC0` ×2 ← `FUN_005AB980` | **`0x589B70(0x416)`（menu 1046 取得）**＋ `lea eax,[edi-1]; cmp eax,4; ja`（通知 id 1..5）＋ `[0xB3B3D9]` フラグ ＋ `0x6DA150([eax+0x20],0xF)==2` / `fld1 fcomp [eax+0x3c]` ゲート | **menu 0x416(1046) 系の HUD 操作** |
| 8 | `FUN_005B8DC0` ← **`0x579C10` への callback 登録**（E8/E9 0件の正体） | `FUN_005BB880`（MapMenu notify・`cmp edi,21`/`cmp edi,43`）が **`sprintf('%s %s?', [0xB38C60]='Do you want to travel to ', …)`** → `0x579C10(text, sYes, sNo, 1, **0x5B8DC0**)`。本体＝`0x578D70()==1` → **`0x589B70(0x3FF)`＝MapMenu** → `[+0xc4]` → `vfunc+0x174` → `FUN_005821F0` @`0x5B8F91` | **Fast Travel 確認の Yes** |

**注**: `FUN_005B8DC0` の head `0x5BB880` は `.rdata 0xA6CCA8` を vptr に持つ（v556 の「MapMenu vtable +0x0C = 0x5BB880」と整合）。

---

## 3. 3薄ラッパの構造 [部分確定]

```
0x5791A0: 0x582160(1,0) → [+0x1c]!=0 → mov ecx,eax → jmp 0x583E60   ; begin
0x5791E0: 0x582160(1,0) → [+0x1c]!=0 → mov ecx,eax → jmp FUN_005821F0 ; execute (第3経路本体)
0x579220: 0x582160(1,0) → [+0x1c]!=0 → mov ecx,eax → jmp 0x583F40    ; end
```

* 呼び出し側（#1/#2/#3）は**常に3連呼 `0x5791A0 → 0x5791E0 → 0x579220`**。
* `0x582160(1,0)` は v582 で確定した **`[0xB3A6E0]` の lazy singleton**（`malloc(0x134)`+`0x5802B0`）。`[+0x1c]` がその**初期化済みフラグ**（未初期化なら何もしない＝`ret`）。
* → **「singleton が初期化済みなら begin → 実行 → end」**という3段構造。`0x583E60`/`0x583F40` の identity は残点（V-584-2）。

---

## 4. V-562-3 への帰結 [部分確定]

1. **HUD 生成の実証経路は第3経路のみ**（open 経路 `0x591C50` は全8系統で成功呼び出し不存在＝v578/v580）。
2. 第3経ロード**の発火源8系統すべてが chargen フロー外**（§2）→ **chargen フローに発火源が0件 [確定(命令)（negative）]**。
3. → **「chargen 中に HUD(0x3EC) は生成されない」**という V-562-3 の要求に**命令的裏付けが成立**（negative 側）。
4. **ただし**: 「chargen 中にどの条件でブロックするか」の positively なゲート（例: chargen フラグの参照）は本裁定で特定していない → **[部分確定]**（V-584-1）。

---

## 5. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-584-1** | **chargen ゲートの positively な特定**: §4-4 の「どの条件でブロックするか」（`0x582160` singleton の `[+0x1c]` が chargen 中は0なのか、または発火元側の前置条件）。**V-562-3 の最終クローズ条件** | **中** |
| V-584-2 | `0x583E60`（begin）／`0x583F40`（end）の identity と `FUN_005821F0`（execute）との役割分担 | 低 |
| V-584-3 | `FUN_005ABBC0` の menu `0x416`(1046) の実名と通知 id 1..5 の意味、`[0xB3B3D9]` フラグ | 低 |
| V-584-4 | `0x578D70` の応答値契約の差（v570 の cb は `al==2`、本件 `FUN_005B8DC0` は `al==1`）＝`0x579C10` の flag 引数依存か | 低 |

---

## 6. probe 一覧

`v583_1`（4 callers の E8+E9 全列挙＝`FUN_005B8DC0` のみ NONE・発火文脈12件）／`v583_2`（`0x5791A0`/`0x578FE0` head・`FUN_00440C40` 全 decode＝`sLoadingArea`・`FUN_005B8DC0` の `.text` dword ref＝`0x5BB9B8`・`FUN_00440C40` callers・`0x5791A0` callers 5関数）／`v583_3`（`0x57ACC0` head＝**`[0xB38BF8]='Loading area...'`**・`[0xb386c0]`/`[0xb38bf0]`/`[0xb38cf0]` ラベル・`FUN_00444FB0`/`FUN_00671620` 文脈・7関数 head）／`v583_4`（**`FUN_00579CF0` head＝`0x572EC0([0xb3a6b0],1,2)`**・`0x5BB9B8` 周辺＝**`push 0x5b8dc0; call 0x579C10`**）／`v583_5`（**callback 登録文脈＝`'%s %s?'`+`sYes`/`sNo`**・`0x5BB880` head＋vptr ref・3薄ラッパの jmp 先・`FUN_00579CF0` 内の call 2件）／**`v583_6`（`FUN_005B8DC0` head＝`0x578D70()==1`→`0x589B70(0x3FF)` MapMenu・`[0xb38c60]=sTravelQuestion`・`FUN_005BB880` の `cmp edi,21`/`cmp edi,43`）**。全 6 本 EXIT=0（各 probe 冒頭 Test-Path assert）。
