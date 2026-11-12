# v181_esm_integration.md — 束A: ESM 統合ドキュメント化（Q1–Q6 統合 + Q7/Q8 仕様）

- **serial**: TODO 169 / V169・中（`v180_letter_rows_audit.md` 束A の実行）
- **作成日**: 2026-10-29 / 担当: 成果物統合サブエージェント（分析専任）
- **編集範囲**: **本新規 1 ファイルのみ**。docs/（RE_ESM_REPORT.md 等は未開設のまま）・TODO・GT・台帳・plan・session-memory・既存 tmp/re 成果物は一切変更していない（読取のみ）。git 不使用。**ESM/ESP/DLC 本体へのアクセス 0 回（再スキャン禁止遵守）**。Ghidra 呼出なし。
- **入力（読取専用）**: `tmp/re/copilot_archive/` の content_inventory 群・wrld_analysis.txt・esm_*_probe.py・scda_opcode_table_out.txt・mgef_probe*.py・dump_watr*.py・esp_inventory.txt・esm_inventory4.txt / `tmp/re/` の v24・v32・v35・v164・v176・v179・s69_wrld・s72_navm・s73_land_refr_comp・s73_watr_dlc・s50_gmst 群 / `tmp/re/TODO_re49.md`（L115–L122・L582 serial 169 行）
- **方法**: 既存成果物の要約・数値突合・参照先明記による統合。Q1–Q6 は**統合文書化**（新規走査なし）、Q7/Q8 は**解禁待ちの調査仕様書**として v164/v176 の解禁仕様に接続。
- **転記ポリシー**: `docs/RE_ESM_REPORT.md` への転記は要承認のため行わない。本ファイルは**ドラフト**であり、承認後に親セッションが docs へ転記する。

---

## §0 全体サマリ

| 節 | 対象 Q | 統合した元ファイル数 | 主要数値（実測） | 状態 |
|---|---|---:|---|---|
| §1 | Q1 ESM 全記録棚卸し | 3（+V系再走査 2） | 1,167,017 rec / 64 種 / 0 error / 85,079 GRUP | 統合済 |
| §2 | Q2 WRLD 解析 | 2（+s系 18） | 84 worldspaces / 14,770 cells / NAM0-NAM9-OFST-WNAM | 統合済 |
| §3 | Q3 LAND/REFR/CELL/圧縮/NAVM | 5 プローブ + 出力 1 | LAND 31,823 / REFR 1,025,617 / zlib 41,789 | 統合済 |
| §4 | Q4 SCDA opcode 意味付け | 1 表 + プローブ 4 | 141 opcodes / 2,393 scripts / 29 opid 台帳未登録 | 統合済（追記含む） |
| §5 | Q5 MGEF 全レコード | 12 プローブ（+V系 3） | 145 rec / 登録 23 組 / 未登録 122 disposition | 統合済 |
| §6 | Q6 WATR/天候/GMST | 4ダンパ（+s系 5） | WATR 23 / WTHR 37 / GMST 2,304（EXE） | 統合済 |
| §7 | Q7 圧縮+STAT size=0 | 仕様のみ | 41,789 圧縮 rec / size=0 @7,984,486 | **解禁待ち・仕様書** |
| §8 | Q8 DLC SI parse 失敗 | 仕様のみ | 85 B スタブ / DLC 9 件 GT | **解禁待ち・仕様書** |

---

## §1 Q1: ESM 全記録棚卸しの main レポート向け統合

### 1.1 統合した元ファイル

| ファイル | サイズ | 役割 |
|---|---:|---|
| `tmp/re/copilot_archive/content_inventory.txt` | 2,067 B | 初期走査（BSA 17 本の棚卸し + ESM 走査の第 1 試行。TES4 ヘッダ後 [stop] implausible size で早期終了＝未完成） |
| `tmp/re/copilot_archive/content_inventory_final.txt` | 4,722 B | **GT（Ground Truth）**。100% カバレッジ・0 error。GT として採用（v164 §0.2「errs=0 の final 版を GT とする」と一致） |
| `tmp/re/copilot_archive/esm_inventory4.txt` | 2,396 B | 旧 walker v4 の出力（records=1,161,072 / errors=1 / distinct 63）。STAT size=0 エラーの証跡 |
| （V系再走査）`tmp/re/v32_esm_efid_report.md` | 16,141 B | ESM EFID 150 種の V 系再走査（1,167,017 件 / 0 error で GT と一致） |
| （V系再走査）`tmp/re/v35_mgef_code_report.md` §2 | — | ESM 走査統計（1,167,017 / 85,079 GRUP / 圧縮 41,789 / XXXX 例外 1 / エラー 0） |

### 1.2 主要数値（GT = content_inventory_final.txt）

- **Oblivion.esm = 277,504,985 bytes**（Steam GOTY、Shivering Isles **統合済み**）
- **records = 1,167,017 / GRUPs = 85,079 / max depth = 5 / record types = 64 / errors = 0**
- レコード形式の確定: Record header 20B（tag4+size4+flags4+formID4+ver2+unk2）/ GRUP header 20B / Subrecord 6B（tag4+size2）→ `app/src/main/cpp/assets/esm_reader.cpp` の実装は**既に正しい**（早期懸念「ESM reader may be broken」は RESOLVED）
- 上位レコード型: REFR 1,025,617 / CELL 35,494 / LAND 31,823 / INFO 19,278 / PGRD 8,228 / PACK 7,209 / STAT 6,014 / DIAL 3,817 / NPC_ 2,482 / SCPT 2,393 / ACHR 2,190 / LIGH 1,625 / ENCH 1,542 / ACRE 1,473 / LVLI 1,398 / WEAP 1,319 / ACTI 1,252 / SOUN 1,140 / SPEL 1,137
- SI 統合チェック: "SEDementia"=118 / "Bliss"=177 / "Crucible"=194 / "ShiveringIsles"=15 → **SI is inside the ESM**
- DLC ESP 9 件 = 23,835 recs（Knights 9,563 / DLCMehrunesRazor 5,503 / … / **DLCShiveringIsles.esp = 85 bytes スタブ**）
- 合計対応対象（ESM+DLC）: REFR 1,044,109 / CELL 35,787 / QUST 416 / SCPT 3,046 / NPC_ 2,664 / INFO 20,333 / CREA 1,001 / WRLD 103 / LAND 31,927 / STAT 6,080
- BSA 17 本 5.47 GB: nif 9,645 / dds 21,856 / kf 2,687 / mp3 50,943 / wav 2,093 / lip 50,916 / lod 10,810 / xml 759 / egm 155 / tex 38 / fnt 7 / scc 8
- 実装カバレッジ（当時計測）: decoded 45/64 型 / **MISSING 19 型**（ACHR ACRE PGRD GMST LTEX WATR IDLE LSCR SGST EFSH SLGM CSTY AMMO GLOB FURN 等）/ script functions 118/約1,200 / load order 未実装 / Tests 未配線

### 1.3 口径差の注記（esm_inventory4 vs GT）

- `esm_inventory4.txt` は **records=1,161,072 / errors=1 / distinct 63 / STAT=70**（不自然に小）。原因は walker が `STAT size 0 @7,984,486` エラーで再同期し、以降の STAT 約 5,945 件を脱落させたため（1,167,017 − 1,161,072 = 5,945）。**GT は final 版（0 error）**とし、inventory4 は STAT size=0 の証跡として §7 に使用する。
- V 系再走査（v32/v35）は GT と**完全一致**（1,167,017 / 85,079 / 0 error）。v35 は圧縮レコード 41,789 件と XXXX 例外 1 件（WRLD 内、`tmp/re/v35_mgef_code_report.md` §2）を追加計測。

### 1.4 参照先

- KPI「ESM レコードタイプ: 全 ~50 種」への接続: 本統合で 64 種の分布・45 型 decoded・19 型 missing を main レポート骨子として提供。
- 統合先 `docs/RE_ESM_REPORT.md`（**未開設**・転記は要承認バッチ）。
- 関連: `tmp/re/s69_esm.md`（EXE ディスパッチ slot 表）。

---

## §2 Q2: WRLD 解析の統合

### 2.1 統合した元ファイル

| ファイル | サイズ | 内容 |
|---|---:|---|
| `tmp/re/copilot_archive/wrld_analysis.txt` | 12,437 B | 全 84 WRLD レコードの formID / name / keys / NAM0 値 |
| `tmp/re/copilot_archive/probe_out.txt` | 33,985 B | worldspaces 84・cells with coords 14,770・原点近傍グリッドサンプル |
| （s 系土台）`tmp/re/s69_wrld.md` + `s69_wrld_a1..a16` / `s69_wrld_b1_hierarchy.txt` | 16+1 プローブ | EXE ローダ/ctor/vtable/teleport 全容 |
| （V 系補助）`tmp/re/v35_mgef_code_report.md` §2 | — | XXXX 例外 1 件（WRLD 内 @0x23880D9、XXXX size=4 real=70,200 → OFST @0x2388155） |

### 2.2 主要知見

- **84 worldspaces / 14,770 cells with coords**（probe_out.txt 実測）。
- キー集合: EDID / DATA / MNAM / NAM0 / NAM2 / NAM9 / OFST / WNAM / FULL / CNAM / SNAM / ICON（+ XXXX エラー回避サブレコード 1 件）。
- **階層（WNAM 親リンク）**:
  - **Tamriel (0x0000003C)** = Cyrodiil ルート（WNAM なし）。子: 都市 Bruma 0x1C318 / Bravil 0x1C319 / Anvil 0x1C31A / Leyawiin 0x1C31B / Cheydinhal 0x1C31C / Skingrad 0x1C31D / Kvatch（3 formID: 0x32AFE/0x5CF61/0x71F7C）/ Chorrol 0x3633E、IC 地区 0x2C107–0x2C10F + 0x4A250/0x4A251、EmptyWorld 0x568F、TestEndGame 0x38D48 等。
  - **Realm of Sheogorath (0x00009F18)** = SI ルート（WNAM なし）。子: The Fringe 0x11F7B / Bliss 0x11F7D / Crucible 0x11F7E / Palace Grounds 0x122E5 / Vitharn Bailey 0x14D44（全て WNAM=0x9F18）。
  - ルート worldspaces（WNAM なし）計 60（残りは Test 系等）。
- **EXE 側（s69_wrld.md 確定）**:
  - WRLD ハンドラ slot 8 = 0x44E401（alloc 0xE0=224 B / ctor 0x4F2A10 / vtable 0xA4829C / グローバルリスト [0xB33AA0]）。
  - サブレコード仮想関数 vtable[7] = 0x4F1E70。**NAM0 = min/max bounds（f32×4、圧縮時 extra）/ NAM9 = center/extra（f32×4）/ NAM2 = u32 / DATA = flags byte / WNAM = parent WRLD FormID / CNAM = climate FormID / SNAM = sound FormID / OFST = alloc+fill バッファ**。
- **座標**: cells は XCLC (gridX, gridY) を持つ外装セル 14,770 件。Tamriel のセルは pos/4096 = グリッド（esm_ref_probe 検証済、§3）。

### 2.3 参照先

- `tmp/re/s69_wrld.md`（EXE 全析）/ `s69_wrld_b1_hierarchy.txt`（完全階層）/ `tmp/re/s73_land_refr_comp.md`（LAND/REFR 連動）。
- 統合先: `docs/RE_ESM_REPORT.md`（未開設・要承認）。

---

## §3 Q3: LAND / REFR / CELL クラス / 圧縮 / NAVM — esm_*_probe 群の統合

### 3.1 統合した元ファイル

| ファイル | 種別 | 役割 |
|---|---|---|
| `tmp/re/copilot_archive/esm_cell_class_probe.py` | 4,095 B | CELL 分類（DATA bit0 / XCLC 在否 / GRUP type1 在否の 3 信号比較） |
| `tmp/re/copilot_archive/esm_decomp_probe.py` | 2,975 B | 圧縮レコードの zlib 検証（400 件テスト） |
| `tmp/re/copilot_archive/esm_land_probe.py` | 3,734 B | LAND の親帰属（GRUP type6/type9）+ プレイヤースタートセル |
| `tmp/re/copilot_archive/esm_ref_probe.py` | 6,681 B | ACHR/REFR/ACRE 所有セル・座標系（Y-up/Z-up）クロス検証 |
| `tmp/re/copilot_archive/esm_cell_probe.py` | 5,914 B | CELL 汎用プローブ（EDID/FULL/XCLC/DATA） |
| `tmp/re/copilot_archive/probe_out.txt` | 33,985 B | 上記群の出力（worldspaces 84 / cells 14,770 / グリッドサンプル） |
| （s 系土台）`tmp/re/s73_land_refr_comp.md` | 14,891 B | LAND/REFR/圧縮の EXE 側完全文書 |
| （s 系土台）`tmp/re/s72_navm.md` + `s72_navm_a1..a50` | 50 プローブ | NAVM/PathingSystem（NavMeshManager 0xB333A0 等） |

### 3.2 主要知見

**LAND（31,823 件・EXE slot 9 / ctor 0x4C4C00）**
- LAND ↔ CELL は 1:1（**外装セルのみ**）。紐付け 0x4CE3C0 → new(0x28B) → リンク 0x4BFDC0、CELL+0x40 に格納。
- サブレコード: DATA(4B フラグ: bit0=VNML+VHGT / bit1=VCLR / bit2=VTEX·BTXT·ATXT)、**VNML 17×17=289 頂点×3 signed B**（/127.0→float、3,468 B）、**VCLR 17×17×RGB B**（/255.0→float+alpha、4,624 B）、**VHGT 1,096 B = float offset + 1,089 signed B（33×33 フル解像度）**、VTEX/BTXT quadIndex+FormID、ATXT quadIndex+FormID+layer(≤7)、MPCD（モーフ）。
- 紐付けプローブ（esm_land_probe.py）: LAND は GRUP type6/type9 経由で親セルへ帰属。

**REFR（1,025,617 件 + ACHR 2,190 + ACRE 1,473・EXE slot 6 / ctor 0x4DB260）**
- DATA = 24 B（posX/Y/Z float + rotX/Y/Z float・radian、2 回目は無視）。
- 所有セル = 最寄り CellChildren GRUP（type6）のラベル。**外装約 95% / 内装約 5%**。
- 外装の DATA 位置は**絶対ワールド座標**で pos/4096 = XCLC グリッド一致（Y-up 認定）。内装はセルローカル。
- NAME = 配置ベース FormID、XSCL = scale float、XTEL/XOWN/XPRM 等は汎用ストア 0x4259E0 経由。

**CELL 分類（esm_cell_class_probe.py）**
- 3 信号（DATA bit0 = interior / XCLC 在否 / GRUP type1 在否）のクロスで C++ リーダの信頼できる判定子を選定するための計測。XCLC (0,0) の外装セル等のボーダーケースも計上。

**圧縮（esm_decomp_probe.py + s73 §3）**
- FLAG_COMPRESSED = 0x00040000（flags bit18）。ボディ先頭 u32 = decompressedSize、以降 zlib ストリーム（ヘッダ 0x789C、windowBits 15+32）。**400 件テストで解凍 100% 成功・declen 一致・失敗 0**。圧縮レコード総数 = **41,789 件**（v35 計測）。
- 圧縮対象: CELL/REFR/LAND/INFO 等の大規模型。詳細は §7（Q7 仕様）に接続。

**NAVM（s72 土台・参照統合）**
- NavMeshManager 0xB333A0 / ローダ 0x5F90 系 / pathfinding dispatch 0x68C000・0x68C600 / hkp_path 0x6A8000 系。NAVM レコード自体の ESM 走査統合は Phase 3 継続（TODO M7 行）。

### 3.3 参照先

- `tmp/re/s73_land_refr_comp.md`（実装ガイド §4 あり）/ `tmp/re/s72_navm.md` / `tmp/re/s69_esm.md`（slot 表）。
- 統合先: `docs/RE_ESM_REPORT.md`（未開設・要承認）。

---

## §4 Q4: SCDA opcode 意味付け（141 opcodes）× script_disasm.cpp 連動の統合

### 4.1 統合した元ファイル

| ファイル | サイズ | 内容 |
|---|---:|---|
| `tmp/re/copilot_archive/scda_opcode_table_out.txt` | 7,546 B | **141 distinct opcode の頻度・argLength ヒストグラム表**（2,393/2,393 完走） |
| `tmp/re/copilot_archive/scda_opcode_table.py` | 2,724 B | 表生成スクリプト（framing 仕様の確定版） |
| `tmp/re/copilot_archive/scda_probe.py`〜`scda_probe4.py`（4 本）+ 出力 4 件 | — | SCDA ブロック構造の逐步確定（prologue/tail/0x001C 例外） |
| （V 系）`docs/SCRIPT_OPCODE_TABLE.md`（638 行）+ `RE_EXE_REPORT.md` §67.6（480 opid 台帳） | — | opid → 名称/handler の表側（V77–V84 で大半確定） |
| （裁定）`tmp/re/v176_esm_gate_spec2.md` §1（N1）・§9（N9） | — | Freq 慣例裁定・tail 不変条件 |
| （突合）`tmp/re/v179_csv_crosscheck.md` §4 | — | 台帳×scda 表の機械検証・29 opid 未登録の判明 |

### 4.2 主要知見（既存表の統合）

- **SCDA 形式（確定）**: `[u16 opcode][u16 argLength][argLength bytes]`（advance = 4 + argLength）、例外 **0x001C は固定 4 B**（argLength 無視）。prologue = `1d 00 00 00`（0x001D）、terminator = `11 00 00 00`（0x0011）。**2,393/2,393 スクリプト完走**。72 スクリプトは prologue のみ。
- **141 distinct / Σ(count) = 63,235（tail 除外慣例）**。バンド別: 0x00xx = 50,573 / 0x10xx = 11,413 / 0x11xx = 1,249 命令。
- 主要 opcode: 0x0015 (12,098) / 0x0016 (10,480) / 0x0019 (10,530) / 0x0011 (2,023) / 0x001D (2,321) / 0x001C (5,647) / 0x0010 (4,344) 等。
- **script_disasm.cpp 連動（表側）**: opid 名称・handler は `docs/SCRIPT_OPCODE_TABLE.md`（638 行・V77–V84）と §67.6 台帳（480 opid）で大半確定。スロット表 0xB0AF48 / stride 0x28 / 532 エントリ。

### 4.3 追記必須事項（v179 + v176 裁定の統合反映）

**(a) v179（2026-10-28）: scda 141 のうち 29 opid が §67.6 台帳未登録**
- §67.6 は EXE 側 FunctionID 台帳（480 opid）、scda 表は ESM SCPT 実測（141 opid）。
- 機械検証: §67.6 の数値 Freq 行 112 件はすべて tail-excl 値と **mismatch 0**。`-` 行 368 件で scda 表に在るもの 0。
- **112 + 29 = 141** → 「EXE 台帳に無い 29 opid が SCPT 実測に存在する」＝台帳未登録 opcode 群（v179 R3 残点）。v176 §7 の「SCPT 外 15 種（全体 156 − 141）」とは**逆方向の残差**。V176-f（§67.9 理由未特定 15 opid）との集合交差は台帳 join 副産物で確認対象。

**(b) v176（2026-10-28）N1 裁定: Freq = tail 含む（incl）GT・2 行更新**
- 台帳 Freq 列の慣例は **tail incl を GT に採用**。差分は 2 行のみ:
  - `0x0011`: 2,023（excl）→ **4,344**（incl = 2,023 + 2,321 tail）
  - `0x001D`: 2,321（excl）→ **2,393**（incl = 2,321 + 72 prologue-only）
- 残る 110 行は tail が 0x0011 固定のため両慣例で同一（更新不要）。Σ incl = 65,628（plan.md / RE_EXE_REPORT.md 記載値と一致）。
- **N9**: 「末尾 4 B = 0x0011」は **SCPT 専用の不変条件**。QUST/INFO ブロック（result script）には tail が存在しない（先頭命令も 0x001D ではない）。QUST/INFO の検証式は framing 完走 + 先頭/末尾ヒストグラム。
- N8 の 12 ファイル 16 箇所（incl 慣例への注記・2 行更新）は **F1 バッチ待ち**（本タスクでは未編集）。

### 4.4 残件と KPI 接続

- 残: (1) 141 opcode の意味付けの `docs/SCRIPT_OPCODE_TABLE.md`/main レポートへの統合、(2) 29 opid の台帳未登録分の確認（v179 R3 / V176-f と集合交差）→ KPI「Script VM ハンドラ 480/480」の回収対象。
- **ESM 再スキャン禁止遵守**: 本統合は既存キャッシュ `scda_opcode_table_out.txt` のみ使用（TODO L261 と同一方针）。

### 4.5 参照先

- `tmp/re/v176_esm_gate_spec2.md` §1/§9 / `tmp/re/v179_csv_crosscheck.md` §4 / `tmp/re/v164_esm_gate_spec.md` §3 / `docs/RE_EXE_REPORT.md` §67（読取のみ）。
- 統合先: `docs/SCRIPT_OPCODE_TABLE.md` への反映は要承認バッチ（本ドラフトでは記述のみ）。

---

## §5 Q5: MGEF（魔法効果）全レコード解析 — mgef_probe.py 12 本の統合

### 5.1 統合した元ファイル

| ファイル | 種別 | 役割 |
|---|---|---|
| `tmp/re/copilot_archive/mgef_probe.py` + `mgef_probe2.py`〜`mgef_probe12.py`（計 12 本） | 1,291–3,074 B | ESM MGEF 走査群（EDID/FULL/DESC/DATA 構造・EFID 参照） |
| （V 系正本）`tmp/re/v24_mgef_codes.md`（訂正付き） | 6,999 B | fourcc 名空間区分（訂正版: ESM EFID 150 種が正本） |
| （V 系正本）`tmp/re/v32_esm_efid_report.md` | 16,141 B | ESM EFID 150 種の実測・EFID 参照 8 件 |
| （V 系正本）`tmp/re/v35_mgef_code_report.md` + `v35_mgef_code_table.csv` | 16,966 B + CSV 145 行 | **MGEF 145 件 × 登録 ctor 23 組の対応表（KPI 更新 2026-10-23）** |
| （s 系土台）`tmp/re/s90_magic_m1_m2_pipeline.md` / `s91_magic_m3_mgef_dispatch.md` | — | MGEF dispatch・ActiveEffect パイプライン |
| （関連）`tmp/re/v39_exe_only_codes.md` | — | EXE 側のみ存在する 16 コードの正体確定 |

### 5.2 主要知見

- **ESM 側**: MGEF レコード **145 件 / EDID distinct 145**（4 文字・重複なし）/ FULL 145/145 / DESC は 145/145 に存在するが実文字列は 1 件のみ（ABAT、他 144 件はサイズ 1 の空文字）/ DATA = **64 B × 144 件 + 36 B × 1 件（DARK）**。
- DATA フィールド: off4 = baseCost float / off8 = actorValue u32 / off12 = school u32 / off16 = flags u32。flags 分布: 0xFFFFFFFF × 104 / 0x40 × 35 / その他 6 種各 1。
- **EFID distinct 150 種**（v32）＝効果コード空間の正本（v24 訂正第 1 項）。
- **EXE 登録 23 組**: `push <ctor>; push <fourcc>; call 0x68DA10`（ctor が fourcc の 5 B 前）。23/23 が s91 と完全一致（CALM→0x691CA0 / INVI→0x693FE0 / SUDG→0x6A6890 / LGHT・REAN・TELE は SEH 付き new-wrapper で本 ctor 別体 等）。
- **登録 23 ⊂ ESM 145 = 100%**（不一致 0）。
- **未登録 122 種の disposition**: cmp_branch 8（ABAT DGAT DRAT FOAT REAT ABSK DRSK FOSK、0x413A90 の分岐）+ default_path 114（reader テーブル F0–F4 登録あり・専用 ctor なし＝汎用 ActiveEffect 経由）+ undetermined 0。**全 145 code が EXE 内に参照を持つ**。
- **EXE 側のみ 16 種**（BA01–BA10 / BW09 / BW10 / DISE / DUMY / RSWD / Z020）: `FUN_00417420` の組み込み EffectSetting 登録（161 件）に所在（V39 確定）。フォールバック供給 8 / 純予備 8、DISE は実行時 lookup 専用（`FUN_00419b30`）。
- 既知の「FormID リンク 62 件」は実測 60 件（残り REAN/TURN は actorValue=0）。
- MGEF+0x68 = DATA ブロック+0x10（0x416100 MGEF_Reader）は **W3-4 で確定済み**（TODO G2 行 2026-10-21）。

### 5.3 参照先

- `tmp/re/v35_mgef_code_table.csv`（145 行対応表）/ `tmp/re/v32_esm_efid_report.md` / `tmp/re/s91_magic_m3_mgef_dispatch.md` / `tmp/re/v41_effect_display_name_spec.md`（表示名仕様）。
- 統合先: `docs/RE_ESM_REPORT.md`（未開設・要承認）。

---

## §6 Q6: WATR（水）/ 天候 / GMST 解析 — dump_watr*.py の統合

### 6.1 統合した元ファイル

| ファイル | 種別 | 役割 |
|---|---|---|
| `tmp/re/copilot_archive/dump_watr.py` / `dump_watr2.py` | 2,671 / 1,972 B | WATR レコードのダンプ（DATA 102 B = 25 floats 等） |
| `tmp/re/copilot_archive/enum_watr.py` / `dump_short_watr.py` | 1,861 / 2,008 B | 全 23 レコードの DATA サイズ / ANAM 列挙 |
| （s 系土台）`tmp/re/s73_watr_dlc.md` + `s73_watr_check_mnam.py` / `check_tnam.py` / `direct_scan.py` / `parse.py` / `exe_query1-4.py` | 13,082 B + 7 本 | WATR構造の EXE 側確定 + DLC SI 調査 + WTHR/CLMT 接続 |
| （GMST 正本）`tmp/re/s50_gmst.md` + `s50_gmst_full.txt`（2,304 件生テーブル） | — | GMST 全数解析（Copilot 系） |
| （GMST V 系）`tmp/re/v55_gmst_override_path.md` / `v91_ini_defaults.md` / `v130_ini_residuals.md` / `v160_ini_log_residuals.md` | — | GMST 側 V 系完了（登録 2,304・ESM insert 0 件） |

### 6.2 主要知見

**WATR（23 件・全非圧縮・EXE ローダ 0x4EDF20 / ctor 0x4EDE40・Form 0xAC=172 B）**
- サブレコード: EDID / DNAM / MODL / MODB / MODT / **NAM0 160 B** / FNAM 16 B / HNAM 56 B / SNAM 8 B（サウンド FormID 連結リスト）/ **GNAM 12 B = FormID×3 [self, night, underwater]** / **MNAM = サブタイプ（0x00 通常 / "lava" 溶岩）** / **ANAM = 不透明度 0–100** / **TNAM = 実テクスチャパス**（CNAM は ESM に出現せず、TNAM が実質 CNAM 代替）/ **DATA 102 B = 25 floats + 2 B pad**。
- DATA 25 floats: index0=方向、1=深度、5=うねり、9/10=深度オフセット/範囲、**14=太陽グレア/反射（~200000、血液は負）**、17/20/21=水色 RGB、22=水中色、23/24=波数/振幅（float16–24 は全レコードほぼ共通値）。
- FNAM: 0x02 通常水 / 0x01 溶岩・油 / 0x00 血液等。ANAM 実測: 100（標準）/ 95（CamoranLava, XPBlood 等）/ 87（SE 系）/ 85（DungeonWater, SewerWater, OblivionOil01）/ 65（CamoranLava02）。
- GNAM 実測: DefaultWater (0x18) → DefaultWaterNight (0x84D0) → DefaultUnderwater (0x84D1)。SE 系は 0 連鎖（独立）。
- コンソール（0xA4CF38–0xA4D008）: set water opacity/reflectivity amount/frequency/amplitude/direction。

**天候（WTHR/CLMT）**
- WTHR = **37 件**（EXE slot 2 / idx45、ctor 0x4EE7C0、Form 0x148=328 B）。CLMT = **19 件**（slot 2 / idx46、ctor 0x4BEC50、Form 0x58 B）。
- ルーティング: **CELL → WRLD → REGN → CLMT → WTHR**（気候→天候マッピング）。WTHR DATA から NiSky へ展開。

**GMST**
- **fWater\* プレフィックスの GMST は 0 件** → 水レンダリングパラメータは WATR DATA で管理。水関連 GMST: `fUnderwaterFullDepth`=1000.0 / `sSplashParticles`="Effects\waterSplash.NIF" / `sCanNotSummonInWater`。fWeather\* も 0 件。
- EXE 側 GMST 登録 = **2,304 件**（f 992 / s 1,085 / i 227、static 2,056 / dyn 248）。registrar: 0x41BAE0（float、992）+ 0x419A70（int/string、1,312）= 2,304 全数一致。slot 範囲 0x00B33460–0x00B3B7C8。
- ESM 側 GMST レコード = **382 件**（Q1 GT 表）。**ESM insert 0 件**（V 系完了: v55/v91/v130/v160/v173 — エンジンは起動時に 2,304 件を登録するだけ、override 経路は別途 v55/v93）。
- 移植示唆（s50_gmst.md）: GMST システム自体の再現は不要、`s50_gmst_full.txt` の既定値テーブルを移植側へ。

### 6.3 参照先

- `tmp/re/s73_watr_dlc.md`（§1 WATR 全容・§3 WTHR/CLMT）/ `tmp/re/s50_gmst_full.txt`（2,304 件）/ `tmp/re/v91_ini_defaults.md`。
- 統合先: `docs/RE_ESM_REPORT.md`（未開設・要承認）。

---

## §7 Q7 仕様書: 圧縮レコード（zlib）+ 異常 STAT size=0 — 解禁時の調査仕様

> **スコープ**: ESM 本体へのアクセスは本タスクでは**禁止**（relmap §5 G-D ゲート）。本節は (a) 既存成果物による現状整理、(b) 解禁時の調査仕様（v164 §9 / v176 に接続）のみ。

### 7.1 現状整理（既存知見・読取のみ）

**圧縮レコード（確定済み）**
- FLAG_COMPRESSED = 0x00040000（flags bit18）。ボディ先頭 u32 = decompressedSize、以降 zlib ストリーム（標準ヘッダ 0x789C、windowBits 15+32）。
- `esm_decomp_probe.py` 400 件テスト: 解凍 100% 成功 / declen 一致 / 失敗 0（`s73_land_refr_comp.md` §3.3）。
- 圧縮レコード総数 = **41,789 件**（v35 計測・既知値と一致）。
- 対象型: CELL / REFR / LAND / INFO 等の大規模型（s73 §3.5）。

**STAT size=0（未解消）**
- **オフセット 7,984,486**（ファイル先頭から）で `bad rec b'STAT' size 0`（`esm_inventory4.txt` の唯一の error）。
- STAT は EXE Log+Skip slot 14（0x44E5A5）→ Form は生成されるが size=0 でサブレコード読み（0x447050）が進まず**無限ループ**の恐れ。圧縮の有無は無関係（size 判定が先）。
- 対策（s73 §3.4）: パーサー側で size=0 レコードをスキップするガード。
- 口径差: GT（final 版）は STAT 6,014 件 / 0 error。inventory4 は同 1 error で STAT=70 まで脱落（§1.3）。**GT は final 版**。

### 7.2 解禁時の調査仕様（v164/v176 に接続）

| # | 調査項目 | 仕様 | 接続先 |
|---|---|---|---|
| 7-a | STAT size=0 レコードの正体 | 該当オフセット 7,984,486 のレコードヘッダ（flags / formID / version）を読み、欠損レコードの型・所属 GRUP・前後レコードを特定。esm_inventory4 の walker が脱落させた 5,945 件との関係を確認 | 本節・s73 §3.4 |
| 7-b | 圧縮レコード 41,789 の owner 別内訳 | v164 §1.2 walker（P0–P5）で owner（esm_scpt / esm_qust / esm_info / dlc_total）別に圧縮件数を再計測。esm_decomp_probe の 400 件サンプルを全数へ拡張 | `v164_esm_gate_spec.md` §1–§3 |
| 7-c | CTDA Op（byte0）相当 | CTDA payload byte0 = flags(低 5) \| op<<5(高 3) の実測仕様を ESM 側で確認 | `v164` §9.1 |
| 7-d | zlib inflate VA | s81 Task D の残件「zlib inflate 呼出 VA / CRT ラッパ同定」を ESM ローダ経路で裏取り | `v180` 束D O6 残 |
| 7-e | WRLD 内 XXXX 例外の再確認 | WRLD@0x23880D9 の XXXX(size=4, real=70,200)→OFST が walker で再現するか | `v35` §2 |

**前提**: 本項の実行は relmap §5 **G-D**（serial 163 の仕様確定後、v164 walker 実行）の承認後。執行担当はサブエージェント。

---

## §8 Q8 仕様書: DLCShiveringIsles.esp parse 失敗 — 解禁時の調査仕様

> **スコープ**: DLC 本体へのアクセスは本タスクでは**禁止**（relmap §5 G-D）。本節は (a) 現状整理、(b) 解禁時の調査仕様のみ。

### 8.1 現状整理（既存知見・読取のみ）

- **原因は確定済み（s73_watr_dlc.md §2）**: `DLCShiveringIsles.esp` は **85 バイトのスタブファイル**（TES4 ヘッダのみ。CNAM="nmcdyer" / MAST="Oblivion.esm" / 実レコード 0 件）。Steam GOTY (v1.2.0416) では Shivering Isles が `Oblivion.esm` に統合済み（Q1 の SI チェック: SEDementia 118 / Bliss 177 / Crucible 194 / ShiveringIsles 15）。
- **失敗メカニズム**: パーサー（0x44DCF0）がスタブ判定なしで第 1 パス（0x450080: FormID 事前登録）/ 第 2 パスで 85 B 超えを読み込み、EOF で read()=0 → parse failed。
- **対策（s73 §2.3）**: `if file_size <= 85: skip stub`。
- **旧走査の位置づけ**: `esp_inventory.txt` は **errs>0 の旧走査**（DLCShiveringIsles parse failed / Knights 25 errs / Battlehorn 10 errs 等、9 ファイル計 57 errs）で **GT に使わない**（v164 §0.2）。**GT は `content_inventory_final.txt`**（errs=0、DLC 9 件 = 23,835 recs、DLCShiveringIsles.esp = 85 B スタブ明記）。
- DLC 9 件の GT 値: SCPT 653 / INFO 1,055 / QUST 26 / REFR 17,291 / CELL 293 / NPC_ 182 / CREA 87 / WRLD 19 / LAND 86 / SPEL 177 / DIAL 309。
- **DLC 側の SCDA/SCHR/CTDA/scriptType は一切未走査**（v164 N5: 未知値 0 件）。

### 8.2 解禁時の調査仕様（v164/v176 に接続）

| # | 調査項目 | 仕様 | 接続先 |
|---|---|---|---|
| 8-a | スタブ判定の実装 | `file_size <= 85` ガードを DLC walker に追加。DLCShiveringIsles.esp を skip して残 9 ファイルを走査 | `s73_watr_dlc.md` §2.3 |
| 8-b | 旧 errs>0 の内訳 | esp_inventory.txt の errs（Knights 25 / Battlehorn 10 / ThievesDen 8 等 計 57）が「未対応レコード型」か「データ構造差」かを新版 walker で再分類。GT（final 版）との件数差を明細化 | `v164` §0.2 / `esp_scan.py` ロジック流用 |
| 8-c | DLC SCPT 653 ブロックの構造仮説 | ESM と同じ prologue(0x001D)/tail(0x0011) 構造を持つという仮説を framing 完走で検証（v176 §9: DLC SCPT 行）。破れは新規事実として記録 | `v176` §9 |
| 8-d | DLC QUST/INFO 26+1,055 ブロック | tail なし仮説（QUST/INFO は result script）を同様に検証 | `v176` §9 |
| 8-e | DLC SCDA 頻度 368 の再計測 | v164 §3 の SCDA 頻度表に DLC 分を合算（pass1 の副産物として scda_cache 欠損も再 walk） | `v164` §9.3 |
| 8-f | TrapUpdate 22 件の所在 | 手順は v164 §2 で確定済み。DLC 分の該当有無を pass1 で確認 | `v164` §9.4 |

**前提**: 本項の実行は relmap §5 **G-D** の承認後。旧走査（esp_inventory.txt）は GT に使わず、GT は `content_inventory_final.txt` とする制約を維持。

---

## §9 統合サマリ・新規残点

### 9.1 統合サマリ

| 節 | 統合元ファイル数 | 主要成果 |
|---|---:|---|
| §1 Q1 | 3 + V系 2 | GT（1,167,017 / 64 種 / 0 error）+ 口径差（inventory4 の STAT size=0 証跡）の整理 |
| §2 Q2 | 2 + s系 18 | 84 worldspaces / 14,770 cells / WNAM 階層 / NAM0-NAM9-OFST の EXE 対応 |
| §3 Q3 | 5 プローブ + 出力 1 + s系 52 | LAND 31,823 / REFR 1,025,617 / CELL 分類 3 信号 / zlib 41,789 / NAVM 土台 |
| §4 Q4 | 1 表 + 4 プローブ + 裁定 2 | 141 opcodes / Σ63,235 / 29 opid 台帳未登録 / Freq incl GT 裁定の統合反映 |
| §5 Q5 | 12 プローブ + V系 4 | MGEF 145 / EFID 150 / 登録 23 組 100% / 未登録 122 disposition |
| §6 Q6 | 4 ダンパ + s系 12 | WATR 23 / DATA 25 floats / WTHR 37 / CLMT 19 / GMST 2,304（EXE） |

### 9.2 新規残点

| # | 内容 | 優先 | 行先 |
|---|---|---|---|
| R-a | `docs/RE_ESM_REPORT.md` / `docs/SCRIPT_OPCODE_TABLE.md` への転記（本ドラフト → docs） | 中 | 要承認バッチ（親セッション） |
| R-b | Q4 の 29 opid（§67.6 未登録）× V176-f（§67.9 理由未特定 15 opid）の集合交差 — 台帳 join 副産物で確認 | 低 | v179 R3 / v176 V176-f |
| R-c | v176 N8 の 12 ファイル 16 箇所（incl 慣例注記 + 0x0011/0x001D 2 行更新）は F1 バッチ待ち（本タスク未編集） | 中 | F1 バッチ |
| R-d | Q7 の STAT size=0 正体特定・圧縮 41,789 の owner 別内訳 — ESM 解禁（G-D）待ち | 中 | 解禁サブエージェント |
| R-e | Q8 の旧 errs 57 件の再分類・DLC SCDA/SCHR/CTDA 走査 — DLC 解禁（G-D）待ち | 中 | 解禁サブエージェント |
| R-f | NAVM レコードの ESM 走査統合（s72 土台はあるが ESM 側全文書化は Phase 3 継続） | 低 | Phase 3（束F と連携） |
| R-g | esm_inventory4 と GT の口径差（STAT 5,945 件脱落）は main レポートに注記が必要（本ドラフト §1.3 に記載済み） | 低 | docs 転記時 |

### 9.3 遵守事項（自己検証）

- git 不使用 / docs・TODO_re49・GT・session-memory・plan・台帳の**編集 0 件** / 既存 tmp/re 上書き 0 件（本ファイル v181 のみ新規）。
- **ESM・ESP・DLC 本体への読み取り 0 回**（再スキャン禁止遵守。入力は copilot_archive と tmp/re の既存成果物のみ）。
- Ghidra 呼出 0 回 / `python` スクリプト実行なし（既存出力の読取のみ）。
- selftest 4/4 は本タスクのスコープ外（Q7/Q8 解禁時の probe_harness 系で該当、v176 §検証表の probe_harness selftest 4/4 PASS を参照）。

（了 — v181_esm_integration.md / 束A: Q1–Q6 統合 + Q7/Q8 仕様。docs 転記は要承認のためドラフト留め）
