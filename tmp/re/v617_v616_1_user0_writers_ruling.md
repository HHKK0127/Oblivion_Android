# v617 — V-616-1 遂行（2-1〜2-3）: builder caller は `user0==2.0` 判定・**user0 の書き手は 116 箇所以上（「builder 4 のみ」を否定）**・**「153」撤回**・12 サイト表

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u59_1**（EXIT=0）。既存資産（`docs/Oblivion_decompiled.c`・`named_funcs.txt`・`v307_common.py`）を使用。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| # | 問い | 判定 | 答え |
|---|---|---|---|
| 2-1 | builder caller `0x57B7E0` は何をするか | **[確定(命令)]** | `GetFloat(menuRoot, 0xFAE) == 2.0` の**ゲート**。`!=2.0` なら `0x57B8DF` へスキップし、`==2.0` の経路で `0x57B8D9` から **builder `0x5CA010` を呼ぶ**。 |
| 2-2 | user0 の書き手は builder の 4 箇所のみか | **[確定(命令)（negative）] 否定** | `0xFAE` 即値は `.text` に **322 箇所**。直後命令が call のものは **setter(WRITE) 116 / getter(READ) 22 / その他 184**。→ **user0 の書き手は 116 箇所以上**。 |
| 2-2' | 「153 件の user0 set サイト」 | **[確定(命令)（negative）] 撤回** | 我々の台帳に**出所が無い**（`153` は全て row 番号か無関係な数）。**v615/v616 で私が辿らずに書いた数字**。実測値（322/116/22）で置換。 |
| 2-3 | 12 サイトの対応 | **[確定(命令)]** | 全て同一パターン: ページ名グローバル → `0x4028D0`(名前構築)×2 → `0x5C3440`(tile 取得) → `0x588BD0` GetFloat(0xFAE)。 |
| 追加 | user0 の型/意味 | **[部分確定] 更新** | `menuRoot.user0 == 2.0` がゲート。**2.0 は bool の true**（xOBSE 表: 1=false/2=true）→ **user0 は bool 的フラグ**の公算大（v610 の「ページ index」は要再確認）。 |

---

## 2. (2-1) builder caller `0x57B7E0` [確定(命令)]

```
0x57B824  call 0x582160            ; InterfaceManager_GetSingleton
0x57B829  mov eax,[eax+0x68]       ; menuRoot
0x57B82F  push 0xFAE               ; user0
0x57B834  mov ecx,eax
0x57B836  call 0x588BD0            ; GetFloat(menuRoot, 0xFAE)
0x57B83B  fcomp dword [0xA379B4]   ; == 2.0
0x57B846  jp 0x57B8DF              ; !=2.0 ならスキップ
   ...（PlayerCharacter 系 0x6A2190/0x6A2280/0x678D90 など）
0x57B8D9  call 0x5CA010            ; ★ builder 呼出（==2.0 の経路）
0x57B8DF  ...                      ; !=2.0 の合流先
```
- → **`menuRoot.user0 == 2.0` が chargen（RaceSexMenu 構築）のゲート**。
- `0xA379B4` = **2.0**（v611 でも実測）。xOBSE の bool 規約（**1=false, 2=true**）と一致 → **user0 は bool 的**。
- `0x5790E0`（v616）も同じ `GetFloat(menuRoot, 0xFAE)==2.0` を検査しており、**同一のゲート**。

---

## 3. (2-2) `.text` 全域の user0 即値掃引 [確定(命令)（negative）]

- **`0xFAE` 即値の総出現: 322 箇所**（`.text` 全域、`push imm32` / `mov reg,imm32` / `cmp` の各形）。
- うち **`push 0xFAE` の直後命令が call**: **setter(WRITE)=116 / getter(READ)=22 / その他=184**。
- → **user0(0xFAE) の書き手は 116 箇所以上**（他 184 は値設定を挟む形で未分類）。
- → **「書き手は builder `0x5CA010` の 4 箇所のみ」は [確定(命令)（negative）] 否定**（v616 §3.3 の推定を撤回）。
- **「153 件」の撤回**: 台帳 grep の結果、我々の裁定に「153 件の user0 set サイト」は**存在しない**（`153` は row153/`#153`/無関係な集計値のみ）。**v615/v616 で私が出所を辿らずに引用した数字**であり、**本実測（322/116/22）で置換**する。
  - ◪ 参考: 過去裁定には `843 caller` 分類などの集計はあるが、user0 の件数ではない。

---

## 4. (2-3) 12 サイトの対応表 [確定(命令)]

| site | ページ名グローバル（近傍） | 呼出列 |
|---|---|---|
| `0x5CDF29` | sRed(0xB38FC0)・sHair(0xB38F88)・sGreen(0xB38FC8) | 0x4028D0×2 → 0x5C3440 → 0x588BD0 |
| `0x5CDF92` | sHair・sBlue(0xB38FD0) | 同上 |
| `0x5CDFF3` | sHair | 同上 |
| `0x5CE0F5` | sHair・sColor(0xB39330) | 同上 |
| `0x5CE170` | sHair・sColor | 同上 |
| `0x5CE1E3` | sHair・sRace(0xB38F78) | 同上 |
| `0x5CE24B` | sRace・sGender(0xB38FA8) | 同上 |
| `0x5CE2C4` | sRace・sAge(0xB38F98) | 同上 |
| `0x5CE345` | sMain(0xB38F70)・sComplexion(0xB38FA0) | 同上 |
| `0x5CE3A6` | sMain | 同上 |
| `0x5CE45C` | sMain・sStyle(0xB38FB8) | 同上 |
| `0x5CE4D5` | sHair | 同上 |

- **全て同一パターン**: ページ名で tile を引き（`0x5C3440`）、その **user0 を GetFloat（読み）**。
- 名前は **sMain / sRace / sHair / sColor / sRed / sGreen / sBlue / sStyle / sGender / sAge / sComplexion**（＝ページ名グローバル）。
- → **`0x5CDEF0` = ページ要素を名前で引いて user0(bool) を読む per-page 更新**。

---

## 5. user0 の意味の更新 [部分確定]

- `menuRoot.user0 == 2.0` が chargen の**ゲート**（`0x57B7E0`・`0x5790E0`）。
- ページ tile の user0 も bool として読まれる（`0x5CDEF0`）。
- → **user0(0xFAE) は bool（1=false/2=true）的フラグ**の公算大。**v610 の「ページ index」読みは要再確認**（V-617-1）。

---

## 6. 残点

- **V-617-1【高】**: user0 の型/意味の最終確定（bool か index か）。116 書き手のうち代表の setter 引数（値側）を読む。
- **V-617-2【中】**: 116 書き手のうち **chargen に関わるもの**の特定（builder 以外）。
- **V-617-3【中】**: 12 サイトの **消費先**（GetFloat 結果の比較相手＝どの分岐）。
- **V-617-4【低】**: 未分類 184 件の内訳（値設定を挟む setter 形）。

---

## 7. 検証

- probe **u59_1**（2-1: `0x57B824..0x57B8DF`／2-2: `.text` 全域 322 件＋分類／2-3: 12 サイト表）・EXIT=0。
- 2-2 厳密集計（`push 0xFAE` 直後が call）: setter 116 / getter 22 / その他 184。
- 台帳 grep: 「153 件の user0 set サイト」の出所なし（撤回の根拠）。
- 既存資産のみ使用（新規ツールチェーンなし・`v307_common.py`＋capstone）。
