# V165 — read_ptr() セクション定数バグの波及全調査と v140 派生数値の全体再計算

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B）
- 作成: 2026-10-28 / opencode サブエクト（分析専任・docs/TODO/共有台帳/既存 tmp/re 成果物は無編集）
- 着手時 `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）
- 起票元: `tmp/re/v162_gap_residuals.md` 残点 **R-k（最優先）**
- 新規スクリプト（すべて `%TEMP%\opencode\`）: `v165_scan_bug.py` / `v165_cascade.py` / `v165_walkstates.py` / `v165_66.py`
- 新規出力: `v165_scan_out.txt` / `v165_cascade_out.txt` / `v165_cascade_buggy.txt` / `v165_cascade_fixed.txt` / `v165_walkstates_out.txt` / `v165_66_out.txt` / **`v165_recompute.csv`**

---

## 0. バグの本質（再確認・実測）

`read_ptr(va)` の `raw[va − ImageBase]` は、PE セクションの「VA − PointerToRawData」定数を無視した**ファイルオフセット誤換算**である。

| セクション | 正しい const（VA−file_off） | バグ読み取りのズレ |
|---|---|---|
| `.text` | 0x400C00 | +0xC00 |
| `.rdata` | 0x400E00 | **+0xE00**（IAT/ポインタ表が集中・影響大） |
| `.data` | 0x401400 | +0x1400 |

実測（本タスク再確認）: `call dword ptr [0xa28090]` @0x448E39 の値 = 正 **0x700A9A**（`pe.get_data` / const 系） vs 誤 **0x9E27F0**（`raw[va−img]`）。
`pe.get_data(va − ImageBase, 4)` は pefile がセクションテーブル経由で RVA→file off するため**正**（本タスクで実測確認）。

---

## 1. タスク1: バグ波及スクリプトの全数スキャン

### 1-1 機械走査（`v165_scan_bug.py`）

走査対象 2,162 ファイル（`%TEMP%\opencode\v*.py` 全件 + `tmp/re/*.py` + `tmp/re/cache/*.py`）。
パターン: `raw[va − (0x400000|img|ImageBase)]` / `raw[rva]`（同ファイルに `rva = va − img` あり）/ `unpack*(…, raw, va−…)` / `int.from_bytes(raw[…])` / reader 関数本体の分類（`pe.get_data` / section const / va2off / delegate の別）。

### 1-2 **BUG 認定（`raw[va − ImageBase]` を持つスクリプト = 6 本）**

| スクリプト | read_ptr 行 | 備考 |
|---|---|---|
| **v140b_verify.py** | L262–266 | 中断 run |
| **v140c_adjfilter.py** | L180–184 | 中断 run（`v140c_out.txt` の 78.13% / 224,232 はこのバグ系） |
| **v140d_sites.py** | L256–260 | 中断 run（call-origin buckets 147/53/22 の初出） |
| **v140e_cascade.py** | L143–147 | 中断 run（**doc 採用値 111,276/58,935 の実行元**） |
| **v140f2_cascade.py** | L121–125 | 独立再実装（v140e と同一実装＝doc §3-2 の出典） |
| **v152_p2_cascade.py** | L100–104 | v162「バグ込み再現」用（v162_p2fix が訂正版） |

### 1-3 **非影響と判定したスクリプト（主要）**

| 群 | 判定根拠 |
|---|---|
| **v140_step3 / step4 / step5 / v140f_final / v140f3_64** | `read_ptr` は `pe.get_data(va − img, 4)` = **正**（R-k の「step2–step6 / v140b–v140f 系すべて」記述は**過大**。step1/2/6 には read_ptr そのものが無い） |
| v121 / v127 / v128 / v129 / v130 / v131 / v132 / v132x / v137 / v139 / v141 / v142 / v150–v153 / v160 / v161 | section const（0x400C00/0x400E00/0x401400 の三分岐）、`va2off`、`pe.get_data`、probe_harness `ctx.rd` のいずれか（本タスクで body 一件ずつ確認） |
| **v133_***（full_dup_scan ほか） | `probe_harness.Ctx`（セクション定数対応）＋テキスト台帳入力 |
| **v155_build_union / v155_named83** | `CTX.va2off` / `CTX.rd`（正）＋ CSV/台帳テキスト |
| **v145_***（V145 統合系） | cross/verify/supp/v86_set はテキストのみ、probe 群は probe_harness |
| **v96_e8_candidates.py**（`tmp/re/`） | `.text` スライス + va0 基準（バイト列走査）= read_ptr 非依存 |
| **v140_step1 / step2 / step6 / f4 / f5 / f6** | read_ptr・`raw[va…]` いずれも無し（バイト/台帳系のみ） |

範囲外スクリプト（v13–v49, v59, v69, v72 等）にも `rva = va − img` は多数あるが、本タスクの指定範囲（v90–v142 + v140 系 + cache）では上記6本が全数。

### 1-4 `tmp/re/cache/` 成果物の抽査（作成元特定）

| 成果物 | 作成元 | 読み込み経路 | 裁定 |
|---|---|---|---|
| `call_targets.json` | `cache/build_cache.py` `build_call_targets()` | `.text` スライス（0x400–+0x626E00）を capstone 線形走査して **E8 rel32 のみ**抽出（ポインタ間接解決なし） | **非影響** |
| `opcode_table.json` | 同 `build_opcode_table()` | `probe_harness ctx.rd`（セクション定数） | **非影響** |
| `vtable_index.tsv` / `named_funcs.txt` | 同（テキスト抽出・台帳コピー） | バイト列 VA 換算なし | **非影響** |

---

## 2. タスク2: v140 派生数値の全体再計算（旧→新 差分表）

### 2-1 手法と identity 検証

3 本の再計算スクリプトで、**同一パイプララインを read_ptr のみ切替えて二重実行**した。

1. `v165_cascade.py` — `v140f2_cascade.py` を exec で無改変実行（変更点＝OUT 名と read_ptr 本体のみ）。データスライスは v140f2 同様 `Misc_VirtualSize`（読 ptr 効果だけを隔離）。
2. `v165_walkstates.py` — `v140f_final`/`v140f3_64` の walk・サイト状態・A/B/C を忠実再実装、両モード。
3. `v165_66.py` — §1/§2-1/§3-1 のバイト/台帳系（read_ptr を一切定義しないパス）。

**identity 検証（両方向）**:

- **buggy 再実行 = v140 公表値に全一致（PASS）** — P0–P5 全 cand/recall/未ラベル、GT 生存、origin mix、buckets、F2 拒否、P4 Cat-3、66-in-P0 = すべて一致 → **doc §3-2 の数値が read_ptr バグ実装由来であることを実証**。
- **fixed 再実行 = v140 公表の walk 系値に全一致（PASS）** — walk1 decoded 1,635,166 / 78.10% / walk2 1,522 / walk3 5,122 / 78.44% / サイト状態 17・169・0 と 47・139・0 / A41・B15・C8 / unreached 139・fwdok 139・push58・bwd0 = 15 項目全一致 → **§2-2/§2-3 の公表値は pe.get_data（正）系で計算されておりバグ非影響**。
- **バイト系再計算 = 全一致（PASS）** — §1/§2-1/§3-1 の全項目。

補助的クロスチェック: v162 の訂正版 `v162_p2fix_cascade.py`（.text スライス=SizeOfRawData）は P2 **111,305** / P3 **58,989** / mix **40,628+150** を既に出力済みで、本タスクの fixed 結果と**完全一致**（スライス長差で変わるのは P0 のみ 234,974↔234,975 = read_ptr 無関係）。

### 2-2 **変わった数値（旧→新）— 全 10 項目**

| # | 指標 | 旧（v140 公表） | 新（fixed 再計算） | Δ | 出典箇所 |
|---|---|---:|---:|---:|---|
| 1 | P2 候補数（F1 後） | 111,276 | **111,305** | +29 | §3-2 表 L253 |
| 2 | P2 未ラベル（偽陽性） | 93,065 | **93,094** | +29 | §3-2 表 L253 |
| 3 | P3 候補数（F1+F2・本命） | 58,935 | **58,989** | +54 | §3-2 表 L254 |
| 4 | P3 未ラベル（偽陽性） | 40,724 | **40,778** | +54 | §3-2 表 L254 / L260 / L264 / L297 |
| 5 | GT Cat-3 生存 @P3 | 10,812 | **10,828** | +16 | §3-2 表 L254 |
| 6 | P3 mix: unreached | 40,502 | **40,628** | +126 | §3-2 読み方 L264 |
| 7 | P3 mix: call-origin | 222 | **150** | −72 | §3-2 L264 / R3 L279 |
| 8 | call-origin bucket: cat3 entry | 53 | **0** | −53 | §3-2 L264 |
| 9 | call-origin bucket: unknown | 22 | **3** | −19 | §3-2 L264 / R3 L279 |
| 10 | F2 拒否 未ラベル数 | 52,341 | **52,316** | −25 | v140f2_out.txt（doc 未掲載・証跡のみ） |

付随する文字列値:

| 指標 | 旧 | 新 |
|---|---|---|
| 未知帯（unknown band） | 0x9E27B0–0x9E4200（22 件） | **0x700A9A–0x700F06（3 件・IAT スロット 0xA28090/0xA28168/0xA282B0）** |
| 抑制率 | 203,213 → 40,724（−80.0%） | 203,213 → 40,778（**−79.9%**） |
| call-origin bucket: known | 147（不変） | 147 |

（call-origin 222→150 / 未知 22→3 の内訳確定は **v162 §3-3 で既に訂正済み**。本タスクはそれを cascade 全体へ波及させた。）

### 2-3 **変わらない数値（不変＝全 80 行超）— 重要**

| 群 | 不変だった項目（旧=新、buggy 再実行でも同値） |
|---|---|
| Cascade P0 / P1 | P0 234,974・recall 21,034・未ラベル 213,940 / P1 156,933・18,211・138,722（P0/P1 は read_ptr を経由しない） |
| Cascade P4 / P5 | P4 12,789・12,500・289 / P5 12,243・12,008・235、P4 Cat-3 256、P4 ≥2sites 6,848 |
| GT 生存（判定根拠） | **66 件 61/61/61/61/61/15・214 件 全段 214・64 件 全段 64**（ground truth ゼロ損失は fixed でも維持） |
| GT Cat-3 その他 | @P0 14,867 / @P1 10,846 / @P2 10,845 / @P4 256 / @P5 230 |
| coverage | 公表 **78.10%**（walk1）・**78.44%**（walk3 後）＝ fixed と一致。バグ版は 78.13% / 78.47%（= v140c/v140b 出力値、**公表値ではない**） |
| サイト状態・A/B/C | walk1 start17/unreached169/inside0、walk3 start47/unreached139/inside0、**A41/B15/C8**（unreached139・fwdok139・push58・bwd0）。**buggy 再実行でも 0 サイトが状態遷移（flip 0、A セット対称差 0）** |
| §1 66 件系 | 確定60/保留6/除外0、GAP54/MID-INSN12、adjacent60/padCC5/pad001、宿主外51(=45+6)/宿主内15、named9/9、fldz帯14(D9EE12)、Cat-3 66/66（エントリ開始点40・領域内26 = E10/A9/C3/F2/D2）、66-in-P0 61・非生成 5 |
| §2-1 214/64 構造 | pass150/fail64（site-NG のみ64）、186 サイト全 blocked（on-boundary 0）、stop 理由 top12 完全一致、距離 min18/med1,397/max188,074 |
| §3-1 gap 構造 | 214: 132 gap / ≥2 33 / ≥3 11 / 最大22 / multi 計115、66: 46 gap / ≥2 14 / 最大5 |
| §1-6 Cat-3 突合 | 領域16,660、64/64、186/186 |

### 2-4 buggy↔fixed の構造差（walk 実測）

- seen 位置の差 **1,208**、cov バイト差 **3,560**、origin バイト差 **1,250**（buggy 側が余分に 318 命令を誤到達 decode）
- P2 対称差 **33 位置**、P3 対称差 **58 位置**（buggy のみ2件: 0x701882/0x7018B5、fixed のみ56件: 0x700E6E/0x700F06/0x9E2130…）
- 誤読の機序: `call [abs]` の先が 0x9E27xx 帯（CRT 未ラベル領域）へ誤解決 → そこが flow 到達扱いになり P2/P3 で過剰除去 + call-origin へ誤帰属。

---

## 3. 判定が変わった項目 / 変わらない項目

### 3-1 **判定が変わる（= 訂正が必要）**

1. **§3-3 の差分帰属**（v140 L268）: 「P2 のみ v140f 初版で F2 誤実装により 111,305/84,499」としていたが、**P2 は F2 より前の段階**であり差分の正体は read_ptr。正しい帰属 = **P2 差（111,276↔111,305）は read_ptr バグ、v140f 初版の誤りは F2 のみ（P3=84,499）、真値は P2=111,305 / P3=58,989**。
2. **R3 残点の対象定義**（v140 L279）: 「call-origin 222 うち未知 22 件（0x9E27xx 帯）を imm32 ポインタ表で特定するパイプライン」→ **未知 22 はバグ由来の誤帰属**（正体は `.CRT$XCU` 7,402 エントリ、v162 R-l で同定済み）。実対象は **call-origin 150 件・未知 3 件（0x700A9A 帯 = IAT、R-m）**。
3. **R-k 自身の範囲裁定**（v162 R-k）: 「step2–step6 / v140b–v140f 系すべてにバグ」「coverage 78.13%・P0–P3 全体が影響下」→ 実測では **バグは v140b/c/d/e/f2 の 5 本のみ**、coverage 公表値 78.10% は正、影響は **P2/P3・mix・bucket・F2拒否・Cat-3@P3 に限定**（P0/P1/P4/P5・GT・site 状態・A/B/C は不変）。

### 3-2 **判定が変わらない（値のみの更新 or 完全不変）**

- 「**P3（accept+F1+F2）が本命の抑制・ground truth ゼロ損失**」という結論: **不変**（fixed でも 61/214/64 全段維持・再現率 84.3% 不変）。値のみ 40,724→40,778。
- 「E8 証拠は hard filter にしない」「16B 揃えは hard filter 不採用」: **不変**（P4/P5 と GT 損失が完全に同値）。
- 「186 サイトはデータ由来 E8 が 0 件」「64 件はサイト検証の厳しさ起因 A41/B15/C8」: **不変**（バイト系 + fixed walk で全一致、buggy walk でも同値）。
- 「新規 66 件 = 確定60/保留6・multi-gap 構造」: **不変**。
- coverage: **変化なし**（78.10% が既に正）。

---

## 4. タスク3: v155 / v133 / v127 波及裁定

| 成果物 | 裁定 | 証拠（作成スクリプト読取） |
|---|---|---|
| **`v155_label_ledger_union.md`（統合表 + V145 追記）** | **影響なし・再計算不要** | `v155_build_union.py` は `probe_harness CTX.va2off/CTX.rd`（セクション定数対応）＋ Ghidra 台帳・v86 scan・decompiled ヘッダ等の**テキスト入力のみ**。統合表の層サイズ/pairwise/membership（§8-1 の 61/61 PASS）はテキスト集合演算。§9 の「v140 66 件突合 55/66・未収録11」は **66 件リスト（バイト系・不変）** を入力とし cascade を参照しない。v155 md 全文を `222\|0x9E27\|call-origin\|cascade\|111,2\|58,9\|coverage` で grep → **0 件** |
| **`v133_full_dup_scan.md`（dup scan）** | **影響なし・再計算不要** | `v133_full_dup_scan.py` は `probe_harness.Ctx`（セクション定数）で関数本文を取得し mnemonic/imm ベースの MinHash・TS 骨格比較（間接ポインタ解決なし）。md grep（call-origin/memabs/coverage 等）→ **該当数値 0 件**。R-f/R-g/R-h の残点は別原因 |
| **`v127_labelless_cat3.md`（Cat-3）** | **影響なし・再計算不要** | `v127_cat3_scan.py` は `va2off`（セクション定数）＋ファイルオフセット直走査の imm32 スキャン（`unpack_from(…, DATA, off)` = ファイルオフセットそのもの＝変換誤りの余地なし）。`v127_scan/reconcile/stats/gap_*` も同様（probe_harness 経由、`raw[va−img]` 形式ゼロ＝機械走査でも BUG 0 件）。16,660 / 11,463 / 10,261 / seed 33,098/17,921/38,375 は byte/ledger 系で再計算対象外 |
| （参考）V145 統合の残り | 影響なし | `v145_cross_v140/named_x/supp/v86_set/verify_union` はテキストのみ、`v145_probe/reject_probe` は probe_harness |

---

## 5. 訂正ドラフト（適用は親セッション。本タスクでは列挙のみ）

### 5-1 `tmp/re/v140_e8_66_finalization.md`（影響あり・9 箇所）

| 箇所 | 現状 | 訂正後 |
|---|---|---|
| §3-2 表 L253（P2 行） | 111,276 / 84.3% / 93,065 | **111,305 / 84.3% / 93,094** |
| §3-2 表 L254（P3 行） | **58,935** / 40,724 / 61-214-64 / 10,812 | **58,989 / 40,778 / 61-214-64 / 10,828** |
| §3-2 読み方 L260 | 203,213 → 40,724（−80.0%） | 203,213 → **40,778（−79.9%）** |
| §3-2 L264 | 40,724 件 / unreached 40,502 / call 222 / 147・53・22 / 未知22件 0x9E27B0–0x9E4200 | **40,778 / 40,628 / 150 / 147・0・3 / 未知3件 0x700A9A–0x700F06** |
| §3-3 L268（帰属） | 「P2 のみ v140f 初版で F2 誤実装により 111,305…」 | 「**P2 の差は read_ptr バグ由来（v140f_final の pe.get_data 版 111,305 が正）**。v140f 初版の誤りは F2 のみ（P3=84,499）。origin-F2 + read_ptr 修正の真値 = 111,305 / 58,989（v165 実測・v162 p2fix と一致）」 |
| §4 R3 L279 | call-origin 222 / 未知22（0x9E27xx 帯） | **call-origin 150 / 未知3（0x700A9A 帯・IAT）**。0x9E27xx は `.CRT$XCU` 登録配列（v162 R-l） |
| §5 総括 L297 | 偽陽性 203,213→40,724 | 偽陽性 203,213→**40,778** |
| §5 証跡表 L291 | v140f2_cascade 行に注記なし | 「**read_ptr バグあり・§3-2 の値はバグ由来。訂正は v165_cascade.py（fixed）**」を追記 |
| §3-2 表 P0/P1/P4/P5・GT 各行 | — | **変更不要**（v165 で不変を実測済み。追記するなら「read_ptr 非依存を v165 で確認」1 行） |

### 5-2 `tmp/re/v162_gap_residuals.md`（R-k 行の前提訂正・2 箇所）

- **R-k 本文**: 「step2–step6 / v140b–v140f 系すべての read_ptr に共通」「coverage 78.13%・P0–P3 全体が影響下」→ 訂正: **バグは v140b/c/d/e/f2 の 5 本のみ（step3/4/5・v140f_final・v140f3_64 は pe.get_data=正、step1/2/6 は read_ptr 無し）**。影響は **P2/P3・未ラベル・origin mix・bucket・F2拒否・Cat-3@P3** の 10 項目に限定、他は不変（v165 で実測）。78.13% は v140c（バグ版）出力値で**公表値 78.10% は正**。なお「memabs エッジ 6,210 件」は本タスク未計測（代用計測: seen 差 1,208 / P2±33 / P3±58）。
- **§6 整合節**: 「read_ptr 修正版 P2=111,305 / P3=58,989」は v165 独立再計算と**完全一致（追認）**。P0 の 234,974↔234,975（p2fix）は `.text` スライス長差（Misc_VirtualSize vs SizeOfRawData）由来で read_ptr 無関係である旨を1行追記推奨。

### 5-3 **訂正不要**（裁定済み）

`v155_label_ledger_union.md` / `v133_full_dup_scan.md` / `v127_labelless_cat3.md` / `tmp/re/cache/*` / `v96_e8_candidates.txt` / `func_list_new.txt`（テキスト・バイト系 or 正の読経路）。

### 5-4 バグ版スクリプト・出力の扱い（推奨）

- `v140b/c/d/e/f2_*.py` と `v140*_out.txt`（b/c/d/e/f2）は**過去証跡として非編集で保存**（禁止事項遵守）。参照は訂正版 `v162_p2fix_cascade.py` / `v165_cascade_*.txt` へ誘導する注記のみ。
- `v152_p2_cascade.py` もバグ版のまま（v162 時点で p2fix に差し替え済み・新規参照しない）。

---

## 6. 新規残点

| ID | 残点 | 優先度 | 内容 |
|---|---|---|---|
| Rs1 | v142_p5/p7/p8 `rd_u32` の2定数만（.data 未対応） | 低 | `RD_OFF+(va−RD_VA) if va>=RD_VA else TEXT_OFF+(va−TEXT_VA)` は 0xB02000+（.data）だと +0x600 誤読。**現行呼び出しは .data 範囲 0 件を実測**（機械チェック済）。防御的修正候補 |
| Rs2 | v128_*/v138_11 の `b[va−0x400C00]` 系 rdt | 低 | .text 定数直書き。引数は全件 .text のみを実測（.rdata/.data 引数 0 件）だが共通 util 化時は要統一 |
| Rs3 | `tmp/re/s65_hc_vtable.py:63` `raw[rva − s.VirtualAddress]` | 低 | `PointerToRawData` 次元の欠落（別種の誤換算）。v90 未満の単発デバッグ出力1行で数値成果物への波及なし。**v90 未満スクリプトの全数走査は本タスク範囲外**（未実施） |
| Rs4 | R-k「memabs エッジ6,210件」未検証 | 低 | 本タスクは影響を seen/P2/P3 の位置差で代用計測。総 memabs 解決数の計測は未実施 |
| Rs5 | v140d_out「66 not in R1+adj: 6」vs doc「非生成 5」 | 低 | v140d の P0 実装差（0x4672D0 の FF E0 経路の有無）。**doc の 5 が正**（v165 再計算で gen61/非生成5 を確認） |
| Rs6 | v140 系バグ版スクリプトの upstream 修正 | 低 | 修正するか「訂正版へ誘導する注記のみ」かは親セッション判断（本タスクは編集不可のため未実施） |

---

## 7. 最終報告サマリ

1. **バグ波及スクリプト**: v90–v142 窗口では **v140b_verify / v140c_adjfilter / v140d_sites / v140e_cascade / v140f2_cascade の5本**（＋範囲外 **v152_p2_cascade**）。v140_step3/4/5・v140f_final・v140f3_64 は `pe.get_data` で**正**（R-k のスコープ記述を訂正）。cache 全成果物は非影響。
2. **変わった数値（旧→新）**: P2 111,276→**111,305** / P3 58,935→**58,989** / P3未ラベル 40,724→**40,778** / P2未ラベル 93,065→**93,094** / Cat-3@P3 10,812→**10,828** / mix unreached 40,502→**40,628** / call 222→**150** / bucket cat3 53→**0**・unknown 22→**3** / F2拒否 52,341→**52,316**（計10項目＋帯域・抑制率の文字列2件）。
3. **判定が変わった項目**: ①§3-3 の「P2 差 = F2 誤実装」帰属 → **read_ptr バグ由来**に訂正、②R3「未知22件(0x9E27xx)特定パイプライン」→ 対象消滅（CRT 配列と同定済み）・実対象は未知3件(IAT)、③R-k のバグ範囲（5本/限定10項目）・coverage 78.10% は正。**変わらない**: P3 本命・GT ゼロ損失・A41/B15/C8・186 サイト・66件60/6・P0/P1/P4/P5・coverage（=3-2 全項目）。
4. **v155/v133/v127**: 3 成果物とも **非影響・再計算不要**（作成スクリプトの読経路を一件ずつ確認、機械走査 BUG 0 件、md グレップ該当 0 件）。
5. **成果物**: `tmp/re/v165_v140_recompute.md`（本ファイル・新規）＋ `%TEMP%\opencode\v165_recompute.csv`（データ 119 行・changed=YES 10 行）＋ スクリプト4本・出力6ファイル。
6. **新規残点**: Rs1–Rs6（上表・すべて低）。
