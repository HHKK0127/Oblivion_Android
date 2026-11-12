# v330 — #94 / v325 由来 次束Q（6残件）裁定

- 作成: 2026-11-09（L1・静的／文書裁定）
- 出典: `tmp/re/v325_v318_residuals.md`、`tmp/re/v319_v317_static_residuals.md`、`docs/RE_EXE_REPORT.md`、既存 `tmp/re` 成果物および `%TEMP%\opencode\v325_*.out`
- 対象: v325 §5 の6残件。オリジナル `Oblivion.exe`、ESM、`.ess`、解析原本、移植側ソースは変更していない。
- 制約: v318/#89/#91 専有帯は既存証拠の照合と浅層照会のみ。実 `.ess`、実行トレース、移植側変更は本束の対象外。
- 判定語: `[確定]`（命令・表・負証拠が閉合）／`[部分確定]`（静的骨格のみ閉合）／`[推定]`（機能候補）／`[未確定]`（追加証拠待ち）／`[確定負]`（対象仮説を支持する証拠なし）。

## 1. 判定一覧

| # | v325残件 | 本束判定 | 裁定 |
|---:|---|---|---|
| 1 | `0x447050` reader 内部失敗経路の xor 帰属（`al=0`残部） | **[部分確定]** | slot7 の 69 target は型ゲート46（期待型46/46、一致0）・no-op2・無ゲート21へ分類済み。直接ゼロ書の帰属は引数算出型と返値失敗型が混在し、21 reader 全数を閉じる証拠は未取得。 |
| 2 | IAT 以外の間接 call／動的 lane 残分 | **[部分確定]** | 595件のうち v325で45件をIAT、1件をmember、1件を既知impl列へ整理。残る非IAT静的未解決は `0x6B6D2D` と `0x4FBC4C` の2件。vtable 450非global起源とclass不明global 28 siteは実行時／追加静的証拠待ち。 |
| 3 | `FUN_005169A0` 正式名 | **[推定]** | 挙動は ScriptRunner の bytecode 1 entry 実行ディスパッチャと整合し、caller側の `ScriptRunner::Run` 文言も確定。ただし関数自身の正式シンボル、RTTI、関数ポインタ登録、名前文字列参照は0件。正式名の確定格上げは不可。 |
| 4 | 型スイッチ15 caseの docs 非交差型名 | **[確定]** | map 67型（type 1..0x43）を15 caseへ全数分類済み。`docs/RE_EXE_REPORT.md`等との直接交差は GMST/CELL/WRLD の3型。残る64型も命令上の型名として確定するが、本件docsに直接交差する正式名根拠はない。 |
| 5 | dt 単位の静的範囲（#82動的依存） | **[確定]／動的は[未確定]** | `GetTickCount`（ms）→差分clamp→`0.001`→倍率1.0→`dt(+0xC)`の静的代数は閉合し、単位は秒。実値観測（1/60〜0.2秒帯等）は#82／人間ゲートに残す。 |
| 6 | v325副次残件 | **[部分確定]** | E9/EB拡張則の正式採用、factory/ctor→MAIN-vtable疑義行、dt動的観測、非IAT動的lane、reader深層帰属を残す。既存の負証拠や確定部分を未確定へ逆戻ししない。 |

**主判定集計（6件）**: `[確定]` 1（項4）／`[部分確定]` 3（項1,2,6）／`[推定]` 1（項3）／`[未確定]` 0／`[確定負]` 0。項5は静的面を確定、動的面を別レーンで未確定とする。

## 2. 項1 — 0x447050 reader 内部失敗経路

### 確定している範囲

- `0x447050(source=edi,obj=esi)` は vtable slot7（`[vtable+0x1C]`）を呼び、その戻り `al` を呼び手へ返す骨格が確定。
- v325 census は distinct 69 targetを `type-check=46 / no-check=21 / no-op=2` に分解。型ゲート46件は `0x4511C0` の型 getterと期待型の比較が46/46一致、mismatch=0。
- no-op `0x6FDFF0` と `0x977C50` は恒真。無ゲート21件については「型不一致がal=0を作る」経路ではないことが確定。
- reader内部の未知tagは `0x44FEA0`／`0x4510B0` 経由で素通りし、loop終端は `mov al,1` の成功側となる既存窓がある。したがって「未知tagが一律失敗」という解釈は採用しない。

### xor帰属の裁定

直接ゼロ書分布は `{0:9,1:18,2:17,3:8,4:9,5:2,6:5,8:1}`（合計69）。しかし `xor eax,eax`／`xor al,al` の同一形式でも、少なくとも次の二種類が混在する。

1. **返値失敗型**: 型取得・検査・リソース取得等の失敗後に戻り値を0へしてreturn。
2. **引数算出型**: NULL名、未解決ポインタ、空の補助値を作るためにレジスタを0化し、後段は成功を返し得る型。

v325で `ACTI`／`KEYM` 系の一部は引数算出型として読めるが、他の20 reader相当（およびn=0群）の全数を同じ粒度で閉じるには、v319禁止／専有帯を越えない深読と追加の関数全文が必要である。よって、**「69件のxorは全てal=0の原因」も「全て返値失敗」も否定**し、項1は[部分確定]を維持する。

### 残る証拠ゲート

- `0x447050` 本体全文によるslot7直返の再確認は既存head依存から格上げしない。
- 21無ゲートreaderの各xorについて、def-useをreader単位で分離する必要がある。
- 実データでreaderの失敗入力を通す動的確認は、実行環境／人間ゲート依存であり本束では行わない。

## 3. 項2 — IAT以外の間接call動的lane

v325の再現値は `indirect=595`、初期分類 `slot=348 / slot0=179 / global=19 / no-load=49`。full-function callee-saved deep traceでは、no-load 49件が `global=45 / killed-by-call=3 / member=1` へ分解された。

### 解決済み／非対象

- **IAT 45件 [確定]**: `InterlockedDecrement/Increment`、`ReleaseSemaphore`、`TlsGetValue`、`lstrcatA`、`InterlockedCompareExchange`等。vtable dispatchではない。
- **member 1件 [確定]**: `0x5172C6 call ecx`、直前の `mov ecx,[eax+0x18]`。`FUN_005169A0`内のmember dispatch。
- **既知impl列1件 [確定]**: `0x516AF3 call ecx`、`0xB0AF60 + index*0x28` のimpl列。既存v13/v159と整合。

### 残分

- **静的未解決2件 [未確定]**: `0x6B6D2D call edx`（fn `0x6B6B90`）と `0x4FBC4C call eax`（fn `0x4FBB60`）。値が直前callで殺されるため、現行の浅層・callee-saved追跡から実体は出ない。
- **動的lane 450 site [未確定]**: stack／parameter／field起源の非global vtable call。実object class、vtable、slot実体は実行時 object観測または許可帯内の追加全数追跡が必要。
- **class不明global 28 site [未確定]**: `0xB34D90×7`、`0xB33A04×3`、`0xB33A1C`、`0xB3B364`、`0xB430xx×16`。`0xB333C4` PlayerCharacter 13 siteはmain vtable `0xA73A0C`で13/13解決済みであり、28 siteと混同しない。

したがって、IAT以外の**静的残りは2件＋動的lane 478 site相当**（class不明global 28＋非global450）であり、「49件すべてが動的vtable残分」という旧表現は採用しない。

## 4. 項3 — FUN_005169A0正式名

### 閉合した挙動

- `0x516A23 movsx ecx,word ptr [edi+eax]` によりbytecode wordからindexを取得。
- `cmp ecx,0x1e` を越えると `0x516ACF xor al,al` の失敗経路。
- `0xB0AF5C + index*0x28` からarg length、`0xB0AF60 + index*0x28` からimplを取り、`0x516AF3 call ecx`。
- callerは `0x51754D`／`0x5177C6` の2箇所で、PCをargLength分進める。`0x5173F8` の文字列は `Nested call to ScriptRunner::Run...` であり、caller側がScriptRunner::Runであることを裏付ける。
- `0x517300..0x51732F` はdata table、次の`0x517330`はSEH head。関数境界は `0x5169A0..0x5172FF` として既存証拠を維持。

### 名称裁定

機能候補は **「ScriptRunner::Run の bytecode command-entry executor / command dispatcher」**。ただし、`0x5169A0`へのimm32参照、vtable／関数ポインタ登録、関数自身の正式名文字列は0件。したがって `FUN_005169A0` を `ScriptRunner::Run`そのもの、または公式の別メソッド名として確定しない。`ScriptRunner::Run`は呼び手fnの名称としてのみ確定する。

## 5. 項4 — 15 case型名とdocs非交差分

0x44DCF0の `type-1 → map@0x44E6DC → jt@0x44E6A0` は type 1..0x43 の67型を15 caseへ分類する。型名表 `0xB05E00`（stride12）はt=0..68で`d0==t`が全一致。

### docsと直接交差する3型

`GMST`（case1、`0x44E326`）、`CELL`（case5、`0x44DEF0`）、`WRLD`（case8、`0x44E401`）。交差根拠は `docs/RE_EXE_REPORT.md`のGMST/CELL記述と既存s69のWRLD記述で、3/3一致。

### docs非交差の64型（型名自体は表により確定）

`TES4`, `GRUP`, `GLOB`, `CLAS`, `FACT`, `HAIR`, `EYES`, `RACE`, `SOUN`, `SKIL`, `MGEF`, `SCPT`, `LTEX`, `ENCH`, `SPEL`, `BSGN`, `ACTI`, `APPA`, `ARMO`, `BOOK`, `CLOT`, `CONT`, `DOOR`, `INGR`, `LIGH`, `MISC`, `STAT`, `GRAS`, `TREE`, `FLOR`, `FURN`, `WEAP`, `AMMO`, `NPC_`, `CREA`, `LVLC`, `SLGM`, `KEYM`, `ALCH`, `SBSP`, `SGST`, `LVLI`, `SNDG`, `WTHR`, `CLMT`, `REGN`, `REFR`, `ACHR`, `ACRE`, `PGRD`, `LAND`, `TLOD`, `ROAD`, `DIAL`, `INFO`, `QUST`, `IDLE`, `PACK`, `CSTY`, `LSCR`, `LVSP`, `ANIO`, `WATR`, `EFSH`。

`TOFT`はtype表t=68の名前として存在するが、対象mapのtype1..0x43には入らない。また`GMST`、`CELL`、`WRLD`は上記の直接交差3型なので非交差リストから除外した。したがって上記は重複なしの**実投入・docs非交差64型**であり、表記揺れを正式C++クラス名へ推測変換しない。

## 6. 項5 — dt単位の静的範囲と#82依存

静的に確定する範囲は次の通り。

`IAT 0xA280D0 = kernel32!GetTickCount`（ms）→ `FUN_0047D170`でtick差分を取得→0〜166ms相当へclamp→`0xA30E40 = 0.001`を乗算→`0xB06704 = 1.0f`のゲーム速度倍率→timer `0xB33E90+0x0C`（`0xB33E9C`）へ格納→`0x4052A0` getterおよび`0x4FBE26 fsub [0xB33E9C]`等で消費。

従って **dtの静的単位=秒 [確定]**。`pause`時のゼロ化、1ms丸め、166ms上限も命令上確定する。一方、次は静的証明の外であり、#82／人間ゲートに残す。

- 実行時に`dt(+0xC)`へ入る実値の取得。
- 1/60〜0.2秒系に収まることの観測。
- 実frame、pause、負荷、速度倍率変更を含む動的分布。

「動的確認済み」とは記載しない。v325の静的代数閉合と動的未実施を分離して継承する。

## 7. 項6 — v325副次残件の整理

| 残件 | 現裁定 | 次に必要なもの |
|---|---|---|
| E9/EB head拡張則のrule v3正式採用 | [未確定] | 拡張則専用ラベル検証セット。v325の118候補／36差分は初回裁定であり、現行16/16ラベルの対象外。 |
| factory/ctor→MAIN-vtable疑義 | [未確定] | TESIconへ集約される10型とTESFloraの`0x63696E61`混入行を、ctorの実vptr書込位置と全数照合。v325の型census46/46は影響を受けない。 |
| 非IAT間接call | [部分確定] | `0x6B6B90`／`0x4FBB60`の実先、class不明global28、非global450。#91専有帯は解除後に再評価。 |
| 0x447050 xor帰属 | [部分確定] | 21 readerのdef-use全文、入力算出ゼロと返値失敗の分離。実データ失敗は動的ゲート。 |
| dt実行時観測 | [未確定] | #82実行laneと人間承認。静的秒判定は変更しない。 |
| FUN_005169A0正式名 | [推定] | EXE外シンボル、RTTI、デバッグ情報、または実行時シンボル。現証拠で`ScriptRunner::Run`へ改名しない。 |

## 8. 既存記載との訂正・不整合

1. v325の「主判定[確定]3／[部分確定]2／[推定]1」は、項3を正式名の[推定]として独立計上する集計。本文では機能骨格[確定]と正式名[推定]を分離し、集計の混同を避けた。
2. v335の`0x459AF7 mov ebx,[esp+0x98]`をarg2とする旧記載は、v336でarg1（form/object ID）へ訂正済み。本束はv336訂正を採用し、v325対象の根拠として再利用しない。
3. `docs/RE_EXE_REPORT.md`はメニューXML中心で、v325のSaveGame／reader／dt残件を網羅する文書ではない。したがって項4の「docs非交差」はコードとの矛盾ではなく、直接照合対象の範囲差である。

## 9. 帯・検体・変更の宣言

- オリジナルEXE/ESM/.ess/解析原本は読取のみ。移植側ソース、`docs/RE_EXE_REPORT.md`、既存 `tmp/re`成果物は変更していない。
- v318/#89/#91専有帯は、v325/v336の既存結果の照合および浅層結果の引用に限定した。新規の深層追跡は行っていない。
- 新規成果物は本ファイルのみ。動的lane、実`.ess`、#82の人間ゲート依存を確定扱いしない。
