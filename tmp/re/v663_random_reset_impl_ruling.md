# v663 裁定書 — chargen Random/Reset 実装仕様・説明文の受け先（#175/#176/#177 の実装詰め）

- 対象: `Oblivion.exe` v1.2.0416（md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`）
- probe: `D:\re\v663_probe.py`（**EXIT=0**）／補助 `v663_1.py`〜`v663_9.py`, `v664_1.py`〜`v664_3.py`
- 依頼元: 作製レーン（port #96）。前段 `tmp/re/v660_random_reset_eyes_race_ruling.md` の残点 `V-660-A1/A2/C1` の決着。

---

## A1. Reset の既定値（`V-660-A1`）`[確定(命令)]`

- 本体 `0x5C9C70`: `FindMenu(0x40C)` → `0x5898F0` → `0x9832E6` → **`0x521E40(actor)`** → `0x5C91E0(base)` → `0x5C7BA0(base,0)`。
- `0x521E40`: `actor->vtable[0x128](0x45)` → `ptr = ([esi+0x168] or [esi+0x108])` →
  - `push A(actor ptr)`; `call 0x5538D0` → **`[0xb39b80]+8`（グローバル・モーフテンプレート）**; `push B(template)`; **`call 0x5528F0(A, B)`**;
  - 再度 `actor->vtable[0x128](0x45)` → `push C(actor ptr)`; **`call 0x552880(C)`**。
- `0x5528F0`（cdecl）: arg1=`[esp+0xc]`=**B=テンプレート（src）**、arg2=`[esp+0x10]`=**A=actor（dest）**。`mov [ebx],eax`（arg2[0]=arg1[0]）→ **テンプレート → actor のコピー**。
- `0x552880(ptr)`: `[ptr]=50,[ptr+4]=1` / `[ptr+0x18]=30,[ptr+0x1c]=1` / `[ptr+0x30]=50,[ptr+0x34]=1`、各 `0x527160(ptr+8/0x20/0x38, count, 0.0)` → **3 群 count=50/30/50（合計 130）の値配列を 0.0 に初期化**。
- **最終状態**: 呼び出し順が `0x5528F0`（テンプレートコピー）→ `0x552880`（0.0 初期化）なので、**actor のモーフ値は最終的に 0.0**（テンプレート値は残らない）。
- **対象範囲**: **actor のモーフ配列全体**（カレントページ非依存）。UI は後段 `0x5C7BA0` が再充填。
- **`-2.0`**: **Reset 経路には無い**（`[0x521E40,0x522300)` で `[0xa3f400]`/`[0xa53954]` 参照 0 件）。Random 専用。
- → **port の Reset = 全スライダー `value` を raw 0 相当（=0.0）へ**。

## A2. Random の乱数源と写像（`V-660-A2`）`[確定(命令)]`

- **乱数源 = CRT `rand()`（MSVC LCG）** `0x9859DD`: `imul ecx,ecx,0x343fd`（214013）/ `add ecx,0x269ec3`（2531011）/ `shr eax,0x10` / `and eax,0x7fff` → **[0,32767]**。`0x47DF80(arg)` は seed ラッパ（arg≠0→`_srand(arg)`、else `[0xb069c3]≠0`→`_srand(__time64())`、最後に `rand()` へ tail-jmp）。
- **raw 生成**（`0x5273A0`）: `v = rand()/32767.0*50.0 + 15.0` ∈ **[15, 65]**。
- **写像**: `[base+0x880] = (v - 15.0)/50.0` ∈ **[0, 1]**。raw は `0x5273A0` で **[15,65] にクランプ**（`fld [0xa468fc]=15.0` / `fld [0xa47800]=65.0` + `fcom`/`test ah,0x41`）。
- **`user3`(0xFB1)**: (Main, Age)/(Main, Complexion) の各 tile へ **3 回** `SetFloat(0xFB1, …)`: ①`[0xa6d2d8]`(=-2³¹ sentinel) ②算出値 ③`0.0`。**最終値は 0.0**（Random の「乱数シード」ではなく一時書込）。
- **定数**: `15.0`/`50.0`=正規化、`0.25`=第 2 ブロックのスケール、`-2.0`=±2.0（`0x522230` と `fsub [0xa3f400]`）。

## C1. Race 説明文の受け先（`V-660-C1`）`[確定(命令)]`

- `0x5C7BA0` の DESC ブロック: `push 0x43534544("DESC")` で取得 → `mov eax,[0xb38fb0]` → 共有サイト `0x5C8269`（`mov eax,[0xb38f78]`）→ `call 0x5C3440` → `SetString(0xFB4, desc)`。DESC 無しは `[0xa36430]=" "`。
- **受け先 = pane `[0xb38f78]`(="Race") × tile `[0xb38fb0]`(="Race Description")** → Race ページの text。builder サイト `0x5CA60B` が同 `[0xb38fb0]` で生成 → **id 212**。
- race 名は `0x5C3440` の別 tile（`{[0xb38f78],[0xb38f70]}`）へ `SetString(0xFB4)` → **DESC とは別 tile**。

## 実装詰め（port）

- **Face 4 ボタン** `[確定(命令)]`（builder `0x5CA726..0x5CA85B`、Face pane id 224）: **225 Shape`0x5CA76D` / 226 Tone`0x5CA7B1` / 227 Random`0x5CA7F5` / 228 Reset`0x5CA839`**。
- **Random/Reset family 行** `[確定(命令)]`: 一致時はダイアログブロック → `jmp 0x5CD9C5`(finalize)＝**遷移なし `target=-1`**。不一致は次ブロック（Random `jp 0x5CCBF4` / Reset `jp 0x5CCCD2`）。本体は cb（Yes）またはフラグ済み経路（`0x5CCBE5`→`0x5C9CD0` / `0x5CCCC8`→`0x5C9C70`）から呼ぶ。
- **port 方針**: `rand()` の LCG をそのまま移植し、`value = rand()/32767`（raw 経由の clamp は結果的に [0,1]）。`morph→slider` の対応は未確定のため **builder 順に 1:1** で引く。Reset は全スライダー 0.0。`user3` 3 連書きは (Main, Age)/(Main, Complexion) の 2 要素のみ。確認ダイアログは上記に従う。**いずれも `V-654-4`（morph 未適用）の範囲内**。
