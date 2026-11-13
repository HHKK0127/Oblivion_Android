# V357: #93 帯内正式名称5件の再裁定 — #128 束③（FUN_00463700 / 0x44F058 / 0x5BA629 / 0x659BE0 / bit7 SIB）

- 日付: 2026-11-13 / タスク: ai-todo **#128 束③**（#93 帯内正式名称5件＝v338 §2.1-2.5・帯外解析制限つきの浅層留保分を帯外解禁の本レーンで再挑戦）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `tmp/re/v338_plus_residuals_ruling.md` §2（2.1-2.5）＋ `tmp/re/v323_v319_residuals_ruling.md` §5 項1/7/8 ＋ `v316_v313_residuals.md` L116（530件カタログ）
- 手法: capstone 5.0.7（v307_common.py）／head 到達性デコード・**外部転移全走査**（境界証明）／E8 rel32 逆算／.text 全文2.258M命令リニア走査2種（detail 付き operand/幅/SIB 解析）
- 中間証跡: `%TEMP%\opencode\v357_1..v357_8.py/.out`（**最終全 EXIT=0**。v357_2..5 は初回に CsInsn タプル参照・capstone import・`op.mem.disp` の API 誤りで失敗→修正再実行）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1。**#128 により帯外解析が解禁**（#89/#91/#106 専有帯の浅層留保を解除して再挑戦）

---

## §0 判定サマリ

| # | 設問（v338 §2） | 判定 |
|---:|---|---|
| A(2.1) | FUN_00463700 正式名 | **[未確定] 維持**（名称証拠＝文字列0・u32参照0 [確定負]）。ただし**機能は [確定] に強化**: this 付きグローバル解放チェーン全列挙＋**tail `jmp 0x4F9DD0` の正体解明**（帯外ブロッカー解消）＋**this 同一性の特定** |
| B(2.2) | 0x44F058 帯の正式境界・名称 | **境界＝[確定] `0x44EE00..0x44F0B5`**（外部転移0件で単一関数を証明・`call 0x44EE00`@0x44F058＝自関数再入）。find_head の `0x44EDE0` は v323 §4-9「CC ラン不足型」誤答と確定。**名称＝[未確定]**（文字列0 [確定負]） |
| C(2.3) | 0x5BA629 の上位文脈 | **[部分確定]** 上位＝**FUN_0x5BA4D0**（帯外で本体到達・全文脈列挙）。文字列0 [確定負] → 正式名 [未確定] |
| D(2.4) | 0x659BE0 本体の意味 | **[確定(命令)] セーブ時バイトサイズ基礎値 getter**（u16 = `1 + 4E2BF0(arg) + [this+0x58] の vfunc+0x3F0`）。v323「Actor 基礎値 [推定]」を意味レベルで確定昇格。クラス名は RTTI なしで [確定負] |
| E(2.5) | bit7 SIB と v316 530件対応 | **[確定負] を SIB 形へ拡張**: 直接即値型 bit7 リーダ（test/cmp imm=0x80 系・bt 7）は **.text 全体で0件**（SIB 含む）。**[部分確定] 530対応**: 非スタック ALU=545（差15は境界差で未特定）・**SIB 185件は全スタック**・bit7 即値書手**新規ペア検出 `0x4639A0/0x4639A9`** |

---

## §1 A: FUN_00463700（v338 2.1）

### 1.1 命令所見 [確定(命令)]

- **callers = 2件**（E8 逆算全走査）: `0x464499` / `0x4646D9` — いずれも頭 **`0x464440`**（v323 項1: world 再初期化＋LOD land 初期化・'Initializing LOD land…'）。**u32 参照（テキスト/データ）= 0件** → vtable/関数ポインタ登録なし [確定]。
- **本体 0x463700-0x4637BB**（`pop esi; jmp 0x4F9DD0` で終端）: this(esi=ecx) 付きの**グローバル解放/クリア連鎖**を全走査:

```
0xB3BDB0→0x67CF00 / [0xB3BF80]?→0x683500 / [0xB35B90]?→0x4BE420 /
[0xB35B8C]?→0x4BD8C0 / [0xB333C4]→0x65E800 /
[0xB33A98]+0x74 を this=esi に 0x45C470 / this→0x461580 /
[0xB333A0]→0x443300 / 0xB3BD00→0x677280・0x67AE90 /
[0xB33398]→[+0x24]obj→0x6AB160・0x6AB420 /
0x5C16D0・0x5A8BA0・0x57C0A0・0x4F9FD0 → tail jmp 0x4F9DD0
```

### 1.2 tail `0x4F9DD0` の正体 [確定(命令)]（帯外ブロッカー解消）

```asm
4F9DD0: xor eax, eax
        mov [0xB361BC], eax        ; = 0
        mov [0xB09E20], -1         ; = 0xFFFFFFFF
        mov [0xB361C0], eax        ; = 0
        ret
```
- v338 2.1「尾 jmp 0x4F9DD0 は帯外で深追いせず」→ **3グローバルの定数クリア（＋-1 設定）と確定**。

### 1.3 this の同一性 [確定(命令)]

- 両呼び出し site とも直前に **`mov ecx, ebp`**、fn 0x464440 先頭 `464467: mov ebp, ecx` → **FUN_00463700 の this ＝ fn 0x464440 の this（同一オブジェクト）**。fn0x464440 は world 再初期化系（v323 項1 [確定(命令+文字列)]）→ FUN_0x463700 は**その world/マネージャクラスのメソッド**。
- 近接文脈: `0x4646CF: or dword [ebp+0x18], 0x40`（this の +0x18 bit6 セット）→ 0x4646D9 呼び出し。

### 1.4 判定

- **正式名 = [未確定]**（文字列0件・u32参照0件 [確定負]）。
- **機能 = [確定]（本裁定で拡張）**: 「world 再初期化の前後に呼ばれる、this 付きグローバル解放/クリア連鎖（約18系統＋尾の3グローバル定数クリア）」。

---

## §2 B: 0x44F058 の正式境界・名称（v338 2.2）

### 2.1 真の head = `0x44EE00` [確定(静的)]

- **構造**: `0x44EDFB: ret 4`（前関数終端）→ `int3 ×2`（**CC run=2**）→ `0x44EE00: push -1 / push 0x9ADD8E / fs:[0]…`＝**SEH prologue の実関数頭**。
- **find_head の誤答機構の確定**: `func_head` は CC run≥4 必須 → run=2 の `0x44EE00` を不受理 → さらに戻って run=4 の `0x44EDE0`（前関数の**内部**）を採用 = **v323 §4-9「CC ラン不足型」誤答の実例がこれで確定**（v338 2.2 の「0x44EDE0 は誤答候補」を裏付け）。

### 2.2 関数境界 `0x44EE00..0x44F0B5` [確定(静的)] — 二重証明

1. **内部構造**: 0x44EE00→0x44F058 間の終端は**前方 jmp のみ5件**（0x44EE62/0x44EF1E/0x44EF76/0x44EFA4(→0x44F079 へ前方)/0x44F016）で **ret 0件**、0x44F06A に**後方 jmp 0x44EFB0**（内部ループ）→ 直線的単一フロー。
2. **外部転移全走査（.text 全体・jmp/jcc/call の相対ターゲット）**: ターゲット ∈ `(0x44EE00, 0x44F0B5]` で**ソースが領域外の転移＝0件** → 外部から領域内部へ入る経路が存在しない＝**単一関数包含の証明**。
3. 終端は `0x44F0B5: ret 0xc` → int3×8 → `0x44F0C0` 新関数。

### 2.3 `call 0x44EE00`@0x44F058 = 自関数再入 [確定(命令)]

- `0x44F049..0x44F058`: `mov edi,[esp+0x40]; mov ecx,[esp+0x3c]; push edi; push ecx; mov ecx,[esp+0x24]; push esi; call 0x44EE00`。
- エントリの **once ガード**: `0x44EE27: mov [esp+0x1c],ecx`（this 保存）→ `0x44EE2B: test byte [0xB33AD4],1; jne 0x44EE64` → `or dword [0xB33AD4],1`（1回目のみ init ブロック実行、`push 0x25` 開始）。
- **E8 callers of 0x44EE00 = 3件**: `0x44F058`（自関数再入）/ `0x507896`（v323 項7: コンソール/デバッグ系ハンドラ・fn 尾に 'All map markers %s'）/ `0x5BA629`（項C）。外部2件はいずれも **ecx=[0xB33A98]**（グローバル・v338/v320 でも頻出の save 系供給オブジェクト）。
- 副次: `0x44F07E: sub dword [0xB33AC0],1; jne; mov ecx,0xB33AC4; call 0x7C2A40` ＝ **[0xB33AC0] リファレンスカウンタ減算＋最終時に [0xB33AC4] へリリース** [確定(命令)]。

### 2.4 判定

- **境界 = [確定] `0x44EE00..0x44F0B5`**（find_head の 0x44EDE0 採用不可＝確定）。
- **名称 = [未確定]**（関数範囲内に有効文字列0件 [確定負]）。機能記述: 「once ガード([0xB33AD4]bit0)＋SEH、this 保存、[0xB33AC0/AC4] refcount 伴随、外部からは主に ecx=[0xB33A98] で呼ばれる解決系」。

---

## §3 C: 0x5BA629 の上位文脈（v338 2.3）

- **上位＝FUN_0x5BA4D0**（`find_head=0x5BA4D0` 確認、SEH prologue `push 0x9C0D08`、`0x5BA74C` まで整列デコード192 insn、callers **4件**=0x5BB1F9/0x5BB310/0x5BB60E/0x5BB6AD（頭 0x5BB1B0/0x5BB210/0x5BB350））。
- **`0x5BA629` の直接文脈** [確定(命令)]:

```
5BA611: push ebp / lea ecx,[esp+0x34] / push ecx /
        mov ecx,[0xB333C4] / call 0x6ECC80     ; parent/worldspace 解決
5BA622: mov ecx,[0xB33A98]                      ; ★ this = グローバル
5BA628: push eax
5BA629: call 0x44EE00                           ; → 項B の関数
5BA62E: mov esi,eax → (0x5BA63A 後) 0x4F0620 → [edi+0xC4] へ格納
```

- **FUN_0x5BA4D0 の全文脈** [確定(命令)]: `0x589B70(0x3FF)`（v338 S5.2 の ID 添字化 fn・0x3FF=1003 は 0x3E9..0x41B 範囲内）→ `0x5898F0/0x5893B0` → `[edi+0x24]∈{8,1}` ゲート → `0x5B8FC0` → `[0xB333C4]→0x6ECC80` → **`0x58CEB0(0xFA1, float)` ×4**（値: `[edi+0x58]`・`[0xA379B4]`・`[edi+0x60]`・`[edi+0x64]`×2、`0xFA1=4001`）→ `0x5B7550` → `[edi+0x68]` のリスト walk（各要素 vfunc call・`0x573880`）→ `0x4D6670` → **`0x44EE00` → `0x4F0620` → `[edi+0xC4]`** → `[edi+0xC4]` リスト走査（`0x4D7730`/`0x42B310`・`shr 0xB` bit11 ゲート・**vtable+0x174**・`0x6ECC80`・`0x4CCE20`・`0x4D2D00`・fild→`fmul`×2→`0x9828C0`・`0x42B340/0x42B370` へ (0,0,0,ebp,1,1) 連鎖）。
- **判定 = [部分確定]**（上位関数・呼び手4件・全文脈を確定。ただし**本体・caller 帯に有効文字列0件 [確定負] → 正式名 [未確定]**）。`0x58CEB0(0xFA1,f)×4` の callee 意味は [未確定]（残点 V-357-3）。

---

## §4 D: 0x659BE0 本体の意味（v338 2.4）

### 4.1 本体 [確定(命令)]

`0x659BE0` は**自体が関数頭**（16アライン・前 CC、`ret 4` 終端）:

```asm
659BE0: edi = arg1
        call 0x4E2BF0 (this, arg1) → ax        ; 子計算（u16）
        eax = ax + 1                            ; +1 ベース
        if ([esi+0x58] != 0):
            child = [esi+0x58]
            ax += [child.vtable + 0x3F0](child, this, arg1)   ; ★ vfunc+0x3F0
        return u16 ax
```

### 4.2 callers 3件の文脈が意味を確定 [確定(命令)]

| caller | 直後 |
|---|---|
| `0x5E7D0B` | `movzx esi,ax` → `[0xB33B00]` → **`call 0x45A170`**（v323 項4: 現在 version∈[0x1F,0x5A) 述語）→ `test al; je` |
| `0x60831A` | `movzx esi,ax` → **`call 0x45A170`** → `test al; je` → **`add esi, 6`** |
| `0x69F745` | **`add ax, 0xC`** → `cmp [0xB33B00]+0x7C, 0x48; jb; add eax,4; cmp 0x64…`（セーブバージョンゲート） |

- → **戻り値は「バイトサイズ」**であり、version 述語・`+6`/`+0xC`/`+4` の条件加算はすべて**セーブ時バイトサイズ累積**の文脈。
- **判定: [確定(命令)] FUN_00659BE0 ＝「this（＋child vfunc+0x3F0）のセーブ時バイトサイズ基礎値を返す u16 getter」**。v323 の「0x659be0（Actor 基礎値）[推定]」のうち「基礎値＝セーブサイズ」部分を**確定昇格**。「Actor」所属の裏付けは未取得（クラス同定＝残点）。

### 4.3 vtable/RTTI

- **u32 参照 = rdata `0xA73348` の1件** [確定]。周辺は連続コードポインタ run `0xA732FC..0xA734FC`（**65スロット**、`0x659BE0` = run 先頭から index19、直前2スロット = `0x659C90`/`0x659C30`＝近接の妹関数群）。
- **vtable[-1] = `0xAC3C7C` → pTypeDescriptor = 0 → COL/RTTI なし** → **クラス名取得は [確定負]**（この表の範囲）。
- **C++ 正式メソッド名 = [未確定]**（名称証拠なし）。

---

## §5 E: bit7 SIB と v316 530件カタログ対応（v338 2.5）

### 5.1 直接即値型 bit7 リーダ — SIB 形を含めて 0件 [確定負]（v323 の除外を解消）

- **.text 全文リニア走査（2,258,077 insn）**: `test/cmp/bt` のうちメモリ `disp==0x18` かつ**即値付き = 803件** → うち **bit7 相当（imm==0x80 系 / `bt …,7`）= 0件**。
- → v323 項8 の「[reg+0x18] bit7 リーダ 0件 **[確定負]（SIB形は対象外）**」の **SIB 除外部分を今回埋めて完全化**: **SIB 形も含め直接型は0件**。

### 5.2 ALU カタログと v316 530件の突合 [部分確定]

- `or/and/xor/add/sub/bts`（ops6・btc/btr は0件）でメモリ `disp==0x18`:
  - **非スタック（base≠esp/ebp）= 545件**（byte 11 / word 414 / dword 120、imm 389 / reg(computed) 156、bts 0）
  - **SIB 形 = 185件 → すべてスタック演算**（base=esp/ebp）＝ **フィールド [reg+0x18] の ALU は SIB を使わない** [確定(静的)]
  - 全体 = 765件
- **v316 の530件との差15**: 幅内訳では説明不能（全幅545）。走査起点・命令境界判定差の可能性が最も高いが**原因未特定** → 対応は **[部分確定]**（v316 の530=当初カタログ、本走査545=現行・SIB/スタック明示分離版）。

### 5.3 bit7 即値書手（immediate）18件 [確定(静的)]

```
既知（v323/本裁定で位置確認）:
  4445B5: and dword [eax+0x18],0xffffff7f      ← 0x444588: or [eax+0x18],esi（computed ペア）
  4639A9: and dword [esi+0x18],0xffffff7f
  4CDA79: and dword [eax+0x18],0xffffff7f      ← 0x4CDA74: or [eax+0x18],edi（既知）
★新規ペア:
  4639A0: or  dword [esi+0x18],0x80            ← 0x4639A9 と **set/clear ペア**（v323 は clear 側のみ認識）
★CRT 帯（word 幅 set/clear ペア）:
  722B15: or  word [esi+0x18],0x80  /  722B21: and word [esi+0x18],0xff7f
  7AD84C: or  word [esi+0x18],0x80
その他（bit7 を側背する広幅 mask / zero / decrement・帰属選別要）:
  53CF31(and word,0xfe1f) 703EDA/7044AF/704791/7049F4(and word,0xf00f)
  7AB491(and word,0xfc01) 5A7891(add byte,-1) 6D82D8(add word,-1)
  76006E(add dword,-1) 98E08F/99AFC1(and dword,0)
```

- **dword 幅の SaveGame+0x18 系 bit7 書手は 0x4445B5 / 0x4639A0+A9 / 0x4CDA79 の3系統**（＝v323 の既知3件＋新規ペア）に収束 [部分確定]。
- v316 の bit 対応表（bit0/6/9/12/18）整理: bit7 書手群は上記。bit9 は v316 [確定負] 維持。

---

## §6 既知の補強（再利用可能な事実）

1. **`0x44EE00` の構造**: SEH・once ガード `[0xB33AD4]`bit0・this 保存・`[0xB33AC0/AC4]` refcount・外部2 caller は ecx=`[0xB33A98]`・自再入 caller 1件・**境界 0x44EE00..0x44F0B5**。
2. **CC ラン不足型誤答の2例目**: `find_head(0x44F058)=0x44EDE0`（前関数内部）＝v323 §4-9 判定の再現。**rule v3 の E9/EB 追加提案に加え「minrun=4 を満たさない真 head（run1-3）」型として分類可能**。
3. **`FUN_00659BE0` = セーブサイズ基礎値 getter**（vtable 0xA73348 index19・隣接 0x659C30/0x659C90・65スロット run・RTTI なし）。
4. **`[reg+0x18]` フィールド ALU は SIB 非使用**（SIB はスタックのみ）＝今後の命令走査で SIB 除外フィルタが正当化される。

---

## §7 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v357_1..v357_8 **最終全 EXIT=0**（v357_2..5 は初回 API 誤り→修正再実行・失敗分も記録） |
| 2 | 境界証明の厳密性 | (i) 内部 ret=0（前方 jmp のみ）(ii) **.text 全走査で外部転移=0件** の二重証明 |
| 3 | 0件系の網羅性 | bit7 即値リーダは readers with imm=803 を全走査、ALU/reader は同一全文走査の detail 解析（幅・base・SIB を operand 級で判定） |
| 4 | v316 突合 | 545 vs 530 の差を幅・SIB・スタック・ops の各軸で分解して原因候補を限定（境界差のみ残） |
| 5 | v338/v323 との整合 | 2.1-2.5 の全項目で旧判定を上書き・訂正する箇所を明記（2.2 境界確定、2.4 意味確定、2.5 SIB 除外解消）。旧成果物は無変更 |

---

## §8 新規残点

| ID | 内容 | 深刻度 |
|---|---|---|
| V-357-1 | [未確定] `0x44EE00` の正式名（once ガード対象・self-call の目的・名称証拠0） | 中 |
| V-357-2 | [未確定] `FUN_00463700`/`fn0x464440` 共通のクラス正式名（this の RTTI/文字列なし） | 低 |
| V-357-3 | [未確定] `FUN_0x5BA4D0` の正式名と `0x58CEB0(0xFA1,f)×4` の意味 | 低 |
| V-357-4 | [未確定] `FUN_00659BE0` の属クラス名（vtable[-1] に COL なし）と「Actor」所属の裏付け | 低 |
| V-357-5 | [部分確定] 非スタック ALU 545 vs v316 530 の差15原因（走査境界差） | 低 |
| V-357-6 | [未確定] bit7 側背 word mask 群（0xF00F 等）と zero/decrement 群の帰属（SaveGame+0x18 か否か） | 低 |

---

## §9 main への報告

1. **v338 §2.1-2.5 は本裁定で全面更新可**: 2.2 **境界 [確定]（0x44EE00..0x44F0B5・外部転移0件証明・find_head 誤答確定）**、2.4 **意味 [確定]（セーブサイズ基礎値 getter・v323 推定を昇格）**、2.5 **SIB 除外を解消（直接型 bit7 リーダ=0件 を完全化）**、2.1/2.3 は正式名 [未確定] 維持も機能・文脈を大幅拡張。
2. **v316 への突合**: 現行走査=非スタック545（SIB 185件は全スタック）・差15は境界差で [部分確定]。
3. **docs/GT 反映**: 「CC ラン不足型」の2例目（rule v3 の E9/EB 提案への補強）と `FUN_00659BE0` の意味確定は #47/#20 一括時に候補（L1 では docs 無編集）。

---

### 参照（本タスクの証跡）

- v357_1（FUN_00463700 callers/u32/本体/tail 0x4F9DD0 解明＝項A の核心）
- v357_2（0x44F058 head 候補走査・cc_run 分布＝項B の手掛かり。nearest-coverer 法は中間デコードを含み不正確と判明→v357_7/8 で厳密化）
- v357_3（FUN_0x5BA4D0 本文脈・callers 4件＝項C）
- v357_4（FUN_00659BE0 本体・vtable 参照・callers 文脈＝項D の核心）
- v357_5（.text 全文 ALU/reader カタログ＝項E の基礎データ）
- v357_6（bit7 即値リーダ0件・530突合の軸分解）
- v357_7（0x44EE00 構造・外部 caller・RTTI 試行・this 源）
- v357_8（幅内訳・**外部転移0件証明**・vtable run・fn0x464440 先頭・項C 文字列）
