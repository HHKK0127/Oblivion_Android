# v189_esm_ledger_residuals.md — ESM/台帳/CSV 接続残点の束（分析専任・編集なし）

- **serial**: TODO 177 / V177・中（`v179_csv_crosscheck.md` R1–R5 ＋ `v181_esm_integration.md` §9.2 R-b/R-g ＋ `v176_esm_gate_spec2.md` §10 の分析可能分）
- **実施日**: 2026-10-29 / 担当: 台帳/ESM 接続担当サブエージェント
- **編集範囲**: **本新規 1 ファイル** ＋ 新規 `%TEMP%\opencode\v189_*.py` / `v189_opid29.csv` / `v189_*_out.txt` のみ。docs / TODO / session-memory / plan / GT / 台帳 / 既存 CSV / 既存 tmp/re 成果物は**一切編集していない**（読取のみ）。git 不使用。**ESM/ESP/DLC 本体へのアクセス 0 回（再スキャン禁止遵守）**。
- **入力（読取専用）**: `tmp/re/v179_csv_crosscheck.md` / `tmp/re/v181_esm_integration.md` / `tmp/re/v176_esm_gate_spec2.md` / `tmp/re/copilot_archive/scda_opcode_table_out.txt` / `docs/RE_EXE_REPORT.md` §67.6・§67.6.1・§67.7–§67.9（読取）/ `plan.md`（読取）/ `tmp/re/cache/opcode_table.json`（533 行）/ `tmp/re/annotate_db.json`（functions 368）/ `tmp/re/s60_io_ini_keys.txt` / `tmp/re/v130_ini_residuals.md` / `tmp/re/s69_esm.md` / `docs/SCRIPT_OPCODE_TABLE.md` / `%TEMP%\opencode\{v170_ledger_append_candidates.csv, v166_eh_entries.csv, v155_union.csv, v158_map.csv, v174_xcu_entries.csv, func_list_new.txt}` / EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・29 opid の impl 先頭バイト一覧のみ）
- **selftest**: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。完了時再実行は §9。
- **方式**: 全突合は `%TEMP%\opencode\v189_a_opid29.py` / `v189_b_ledger.py` / `v189_c_locations.py` / `v189_d_execheck.py`（新規・再実行可能）。生出力は `v189_a_out.txt` / `v189_b_out.txt` / `v189_c_out.txt` / `v189_d_out.txt`。

---

## §1 v179 R1: v170 dup 4 VA の台帳反映時 drop 裁定 → **L7_EH adopt 行を正とし backlog 行を drop（notes 併記）**

### 1.1 実測（v189_b_ledger.py）

4 VA × 2 層 = **8 行**を機械抽出（検算 8/8）:

| VA | adopt 行（L7_EH, status=adopt） | backlog 行（L6a_seedB, status=backlog_priority） |
|---|---|---|
| 009B055A | kind=`seh_thunk` / host=`009DCCA0:FUN_009dcca0` / evidence `fi=00adc86c;term=jmp->980EA4` / notes `host_via=push_site;dup_cat3_backlog` | kind=`region_start_seedB` / host 空 / notes `part_of_Cat3_backlog_10261;dup=L7_EH` |
| 009B319A | kind=`seh_thunk` / host=`009E23B0:FUN_009e23b0` / `fi=00adeef4;term=jmp->980EA4` / 同上 dup 申告 | 同上（backlog 指紋のみ） |
| 009B36FF | kind=`unwind` / host=`004B4C90:FUN_004b4c90` / `fi=00adf290;term=jmp->4703B0` | 同上 |
| 009B7DFF | kind=`unwind` / host=`00520880:FUN_00520880` / `fi=00ae2638;term=jmp->56A7A0` | 同上 |

- adopt 行の kind/host/scope は **`v166_eh_entries.csv` の当該 VA 行と完全一致**（seh_thunk×2 / unwind×2、host 同値）→ adopt 行は EH 走査の正本そのまま。
- 4 VA はいずれも **v155_union 収録済み（in_u155=True）**、func_list 非収録（= v155 側で採用済み・func_list には無い）。

### 1.2 裁定

> **台帳反映バッチ（relmap §4-#1）では L7_EH の adopt 行を 1 VA = 1 行の正とし、L6a_seedB の backlog 行は drop する。drop 時に backlog 行の `part_of_Cat3_backlog_10261` を adopt 行の notes へ併記して Cat3 出所 provenance を保全する。**

根拠:
1. **一意性不変条件**: v155_union は 31,905 行 / VA unique 31,905 / dup 0（v179 §3.1）。同 VA を 2 行で登録すると台帳の VA 一意性を破壊する（二重登録案は不可）。
2. **情報量**: backlog 行は host 空・kind 汎用（`region_start_seedB`）・evidence は `seedB(pad-independent)` のみ。adopt 行は EH 正本の kind/host/scope/evidence を保持。v179 §1.4 の実矛盾 4 件（kind 不一致・host 空欄）はすべて backlog 行側が原因。
3. **provenance 保持**: adopt 行には既に `dup_cat3_backlog`、backlog 行には `dup=L7_EH` と自己申告があり、drop 後も `part_of_Cat3_backlog_10261` の併記で Cat3 束（10,261）への所属を追跡できる。
4. v179 §1.3 で v170 の構成が「10,870 = 10,716(L7_EH) + 21(adopt) + 129(backlog) + 4(重複行)」であることを踏まえると、**重複 4 行 = backlog 側の除去**で v170 は unique VA 10,866 に正規化され、これは「v170 を VA unique で台帳反映する」場合の自然な整合条件とも一致する。

**適用差分（F1/反映バッチ向け指示・本タスクでは未実行・編集禁止）**:
- drop 対象: `layer=L6a_seedB AND notes に 'dup=L7_EH' AND VA ∈ {009B055A, 009B319A, 009B36FF, 009B7DFF}` の 4 行。
- 更新対象: 残す adopt 4 行の notes 末尾へ `;part_of_Cat3_backlog_10261` を追記。
- 反映後: v170 由来行数 10,870 → 10,866（unique VA 数と一致）。

ステータス: **裁定済み・未適用**（優先: 中 — 反映バッチ時）。

---

## §2 v179 R2: v130 A-1 口径差 2 キーの記述反映要否 → **反映要（v130 のみ・F1 要承認バッチ）／v91 は反映不要**

### 2.1 実測（s60 実在の再確認）

`tmp/re/s60_io_ini_keys.txt`（読取）:

- セクション見出し実在: `# HAVOK   23`（L24） / `# Pathfinding   7`（L32）、ブロック `== [HAVOK] ==`（L464） / `== [Pathfinding] ==`（L629）。
- 対象 2 キーの行: `bUseBackgroundPathing` / `iHavokSkipFrameCountTEST` とも **0 件**（セクションは走査済み・キー行のみ欠落）。

→ v179 §2.2 の機械分類（A-1 → A-3 移動）を完全裏取り。**case-insensitive 正規化後の厳密値は A-1=19（bLightAttenuation 12 + MAIN 5 + OPENMP 2）/ A-3=8（v130 の 6 + 口径差 2）**。合計 31・差 20・検算式 `19+4+8−4−2−1−4 = 20` は不変（v179 §2.1 の検算と同値）。

現行 v130 の記述位置（読取確認）: `tmp/re/v130_ini_residuals.md` L348–L370（A-1=21 の内訳に `PathFinding` 1 / `Havok` 1 が含まれる）、L20・L469 は検算式。

### 2.2 裁定

> **反映要。ただし本タスクは既存 tmp/re 編集禁止のため、転記文面を本稿で確定し F1 要承認バッチに送る。反映先は `v130_ini_residuals.md` §6.2 のみ（v91 側は反映不要）。**

- **v130 に反映する理由**: A-1=21 は「s60 が走査しないセクション外」（L352 の見出し）と読めるが、実際は 2 キーについて**セクションは s60 に実在**（case 差のみ）で「キー行の純漏れ」。次回突合で同じ束ね差が再発するのを防ぐ。
- **v91 は反映不要**: `v91_ini_defaults.md`/`.csv` は VA 付きキー一覧であり A-1/A-2/A-3 分類を持たない（口径差の影響を受けるのは v130 の集計節のみ）。
- 両口径は誤りではなく**束ね定義の差**（case-sensitive vs case-insensitive）。合計 31・差 20 はどちらでも一致するため「訂正」ではなく「注記」として扱う。

**転記文面（F1 用・確定）** — `v130_ini_residuals.md` L352 の表行直後に 1 行追加:
> `※口径注記（v179/v189）: 本 A-1=21 は section 名 case-sensitive 束ね。case-insensitive 正規化後は A-1=19 / A-3=8。うち bUseBackgroundPathing:PathFinding(0x00B15800) / iHavokSkipFrameCountTEST:Havok(0x00B14E44) は s60 にセクション実在（Pathfinding 7 キー / HAVOK 23 キー、s60_io_ini_keys.txt L32/L24）でキー行のみ欠落 = A-3 純漏れ。合計 31・差 20・検算式は不変。`

ステータス: **裁定済み・未適用**（優先: 低）。

---

## §3 v179 R3: 台帳未登録 29 opid の一覧化・口径整理（+ v181 R-b / V176-f 接続）

> **分担境界の遵守**: 本節は「一覧と台帳側整理」まで。名称・役割・impl の**実体解析は V181 担当**（impl は本節で VA まで同定し、先頭バイト存在のみ検証）。

### 3.1 機械検証（v189_a_opid29.py）

| 検査 | 結果 | 判定 |
|---|---|---|
| scda 表 distinct / Σ(count) | 141 / 63,235（tail-excl） | ✅ v179 と一致 |
| §67.6 台帳パース行数 | **480**（数値 Freq 112 / `-` 368） | ✅ |
| scda ∩ 台帳 | **112**、数値 Freq mismatch **0**（112/112） | ✅ |
| `-` 行 ∩ scda | **0** | ✅ |
| **scda − 台帳 = 29** | **29/29**（検算 112 + 29 = 141） | ✅ |
| 29 opid の `opcode_table.json`（532+哨兵1 行）在否 | **29/29 在**（532 テーブル外 0 件） | ✅ |
| 29 opid の `docs/SCRIPT_OPCODE_TABLE.md` 掲載 | **29/29**（opid 文字列ヒット ≥1） | ✅ |
| 29 opid の annotate_db 未照合 | 0 件（20 件は impl VA が annotate_db functions キーと一致） | ✅ |
| impl 先頭バイト EXE 実測（capstone 5.0.7） | 実 impl 27/27 読出可 + 共有スタブ 2/2 が `mov al,1; ret` | ✅ |

> 補足: `cache/opcode_table.json` は **533 行 = 実エントリ 532（idx 0..531）+ 哨兵 1（idx 532 / opid 0x1171 / "ADD NEW FUNCTIONS BEFORE THIS ONE!!!" / impl 0）**。§67.9 の「テーブル 532 エントリ」と矛盾しない（哨兵を除いた実数）。

### 3.2 29 opid 一覧（= `%TEMP%\opencode\v189_opid29.csv` 全行）

| # | opid | 名称（532 表） | scda count(excl) | band | idx | impl | §67.9 区分 | V176-f |
|---:|---|---|---:|---|---:|---|---|:--:|
| 1 | 0x0016 | OnMagicEffectHit | 10,480 | 0x00xx | 22 | 0x005045E0 | UNDET(15) | ● |
| 2 | 0x001E | OnReset | 191 | 0x00xx | 30 | 0x005045A0 | UNDET(15) | ● |
| 3 | 0x1002 | AddItem | 385 | 0x10xx | 165 | 0x00507320 | annotate_db(25) | |
| 4 | 0x100D | Activate | 888 | 0x10xx | 176 | 0x00507650 | annotate_db(25) | |
| 5 | 0x100F | SetActorValue | 307 | 0x10xx | 178 | 0x005016D0 | annotate_db(25) | |
| 6 | 0x1010 | ModActorValue | 64 | 0x10xx | 179 | 0x005017E0 | annotate_db(25) | |
| 7 | 0x1013 | PlayGroup | 824 | 0x10xx | 182 | 0x005107A0 | annotate_db(25) | |
| 8 | 0x101E | Cast | 338 | 0x10xx | 193 | 0x00501F50 | annotate_db(25) | |
| 9 | 0x1021 | Enable | 538 | 0x10xx | 196 | 0x0050A150 | annotate_db(25) | |
| 10 | 0x1022 | Disable | 410 | 0x10xx | 197 | 0x0050A240 | annotate_db(25) | |
| 11 | 0x1026 | PlaySound | 111 | 0x10xx | 201 | 0x00509490 | annotate_db(25) | |
| 12 | 0x1036 | StartQuest | 34 | 0x10xx | 217 | 0x00502D30 | annotate_db(25) | |
| 13 | 0x1037 | StopQuest | 48 | 0x10xx | 218 | 0x00502D80 | annotate_db(25) | |
| 14 | 0x1039 | SetStage | 1,391 | 0x10xx | 220 | 0x00502CB0 | annotate_db(25) | |
| 15 | 0x105E | EvaluatePackage | 558 | 0x10xx | 257 | 0x0050AF90 | annotate_db(25) | |
| 16 | 0x1071 | CompleteQuest | 3 | 0x10xx | 276 | 0x00502DD0 | annotate_db(25) | |
| 17 | 0x1072 | Lock | 105 | 0x10xx | 277 | 0x00504A10 | annotate_db(25) | |
| 18 | 0x1073 | UnLock | 102 | 0x10xx | 278 | 0x00504B00 | annotate_db(25) | |
| 19 | 0x1075 | SetCrimeGold | 9 | 0x10xx | 280 | 0x0050B4A0 | annotate_db(25) | |
| 20 | 0x1076 | ModCrimeGold | 15 | 0x10xx | 281 | 0x00504D00 | annotate_db(25) | |
| 21 | 0x1089 | SetFactionRank | 179 | 0x10xx | 300 | 0x0050AB80 | annotate_db(25) | |
| 22 | 0x108B | KillActor | 121 | 0x10xx | 302 | 0x00501960 | annotate_db(25) | |
| 23 | 0x10D9 | ShowClassMenu | 1 | 0x10xx | 380 | 0x00505810 | UNDET(15) | ● |
| 24 | 0x10DA | ShowRaceMenu | 1 | 0x10xx | 381 | 0x00505850 | UNDET(15) | ● |
| 25 | 0x10DB | ShowBirthSignMenu | 2 | 0x10xx | 382 | 0x00505870 | UNDET(15) | ● |
| 26 | 0x110C | TrapUpdate | 22 | 0x11xx | 431 | 0x00977C50 | shared_stub(7) | |
| 27 | 0x114A | AddAchievement | 1 | 0x11xx | 493 | 0x00977C50 | shared_stub(7) | |
| 28 | 0x1154 | SetActorRefraction | 1 | 0x11xx | 503 | 0x0050E340 | UNDET(15) | ● |
| 29 | 0x115B | SetNoAvoidance | 2 | 0x11xx | 510 | 0x00501390 | UNDET(15) | ● |

（● = v181 R-b / V176-f 交差ヒット。詳細列: arglen min/max、dispatcher、SOT ヒット数は CSV 参照）

### 3.3 口径整理（台帳 join 時の接続情報）

- **「台帳未登録」の正確な意味**: 29 opid は **§67.6（480 行）に行が無い**だけであり、**532 エントリのスロット表（opcode_table.json）には全件 idx/name/impl 付きで在籍**し、`SCRIPT_OPCODE_TABLE.md` にも全件記載されている。欠落しているのは「§67.6 の付録表」のみ = KPI「ハンドラ 480/480」側の集計母数の問題。
- **§67.9 による欠落理由の分解（52 非掲載の内訳と接続）**: 29 = **annotate_db 登録済み 20 + 理由未特定 UNDET 15 中 7 + 共有スタブ `0x977C50` 中 2**（impl==0 哨兵系 0）。残る 23（annotate_db 5 / UNDET 8 / stub 5 / impl==0 5）は ESM SCPT 実測に出現しない側。
- **帯・頻度**: 0x00xx 2 opids（10,671 命令 = block-type 系 0x0016/0x001E） / 0x10xx 23 opids（6,434） / 0x11xx 4 opids（26）。**Σ = 17,131 命令 = tail-excl 総数 63,235 の 27.09%**（incl 総数 65,628 換算では 26.10%）。
- **excl/incl 慣例の影響なし**: Freq 列の incl 差（v176 N1）が及ぶのは 0x0011 / 0x001D の 2 行のみで、**29 opid はどちらの慣例でも値が同一**（tail は 0x0011 固定で 29 に入らないため）。→ 台帳 join 時、29 opid の Freq は scda 表値をそのまま採用してよい。
- **役割の型分類（名称レベル・V181 実体調査の入力）**: block-type 2（If 系 0x0016 / Return 系 0x001E）、クエスト制御 4（0x1036/0x1037/0x1039/0x1071）、メニュー 3（0x10D9/0x10DA/0x10DB）、コア setter/getter 系 13、変化通知/状態系 5、共有スタブ 2（TrapUpdate / AddAchievement）。
- **V176-f（29 × §67.9 UNDET15）の交差 = 7 件**: `0x0016, 0x001E, 0x10D9, 0x10DA, 0x10DB, 0x1154, 0x115B`（→ §5 R-b に接続）。うち 0x0016（rank2 相当・10,480）と 0x001E は §67.9 備考も「block-type。SCDA頻度10,480で台帳なら rank2 なのに不掲載」と一致。
- **EXE 側一覧の検証方法**: 既存 `cache/opcode_table.json`（EXE スロット表の完全キャッシュ）で idx/name/impl を取得し、EXE 直読（capstone 5.0.7）で impl 先頭 32 B をスポット確認 — 27 件は有効な関数プロローグ（`push/sub/mov` 等）、0x110C/0x114A は `mov al,1; ret`（§67.9 スタブ `0x977C50` と一致）。**impl 実体の意味解析は V181 に委譲**（本タスクでは実行していない）。

ステータス: **一覧化・口径整理完了**（優先: 低 — V181 の実体調査待ち）。

---

## §4 v179 R4/R5: 台帳外 thunk 先 29 の個体特定 / xcu in_v170 1 件の個体特定

### 4.1 R4: v158_map target_va 54 unique のうち u155 非収録 29 → **「台帳欠損」ではなく 24 が vcall スロット表記 + 5 がハンドラ内部ストア地点**

機械検証（v189_b_ledger.py）: unique target 54 = u155 収録 25 + 非収録 **29**（v179 §3.2 と一致）。**非収録 29 はいずれも func_list / v170 / v166EH に 0 件**（在否 0/0/0）。

個体特定の結果、29 件は次の 2 種類に**完全分類**される:

**(a) vtable スロット表記の擬似 target（24 件）** — `target_va` 列が実 VA ではなく `vt+0xOFF` 形式の動的ディスパッチ表記:

| target 表記 | v158 行数 | writer_label（例） |
|---|---:|---|
| 0x000vt+40 | 34 | 変化通知 (flag/対象) |
| 0x000vt+90 | 10 | bit1/bit14 登録系 (FUN_0046c640) |
| 0x000vt+e8 | 2 | Process+8 → CC の vtable+0xE8 (0x617E80) |
| 0x000vt+ec | 20 | スケール/既定値 (process vcall+0xEC(1)) |
| 0x000vt+f8 | 7 | (process) |
| 0x00vt+110 | 2 | 一時 3D ノード生成 (0x4B3310) |
| 0x00vt+154 | 48 | (slot) |
| 0x00vt+164 | 19 | GetParentCell (0x4D8370) |
| 0x00vt+170 | 88 | GetBaseForm = emit key 生成 (0x4D9B40) |
| 0x00vt+174 | 47 | 位置取得等 |
| 0x00vt+184 | 6 | process getter (0x5E0380 系) |
| 0x00vt+188 | 9 | 脱離通知 |
| 0x00vt+190 | 137 | 脱離通知 |
| 0x00vt+198 | 20 | IsDead/死亡 slot（Actor 0x5E33B0 / REFR 0x4D7DD0） |
| 0x00vt+1f8 | 2 | state==1 判定 (0x5E33E0) |
| 0x00vt+250 | 10 | meter getter vcall |
| 0x00vt+254 | 5 | meter setter vcall |
| 0x00vt+284 | 5 | slot59 ゲート |
| 0x00vt+2a4 | 5 | HP/状態修正系 |
| 0x00vt+2c0 | 6 | 戦闘クラス u16 (0x5E5690 内) |
| 0x00vt+304 | 4 | (process) |
| 0x00vt+330 | 11 | Actor+0x58 → BaseProcess+0x08 = CombatController |
| 0x00vt+364 | 2 | 死亡時後処理 |
| 0x00vt+370 | 1 | 死亡時後処理 |

**(b) ハンドラ内部の direct_store 地点（5 件）** — 実 VA だが**関数先頭ではなく同一ハンドラ内部の命令アドレス**（opid impl からのオフセット付き）:

| target VA | opid | 同一 opid impl | impl 内オフセット | writer_label |
|---|---|---|---|---|
| 0x005090E5 | 0x1007 (SetPos) | 0x00508FC0 | +0x125 | `mov dword ptr [esi+0x58], ebx` |
| 0x0050F860 | 0x14E | 0x0050EF90 | +0x8D0 | `mov dword ptr [esi+0x58], edx` |
| 0x0050FC4E | 0x14E | 0x0050EF90 | +0xCB6 | `mov dword ptr [ebx+0x58], edx` |
| 0x00511E18 | 0x15F | 0x00511C10 | +0x208 | `mov dword ptr [esi+0x58], ecx` |
| 0x00511F65 | 0x15F | 0x00511C10 | +0x355 | `mov dword ptr [esi+0x58], edx` |

（5 件とも field 列 = `+0x58 (direct store, class unverified)`、source impl 内オフセットが 0x125〜0xCB6 であることから「関数内ストア地点」で確定。u155/func_list が関数先頭台帳である以上、非収録は正常）

**裁定（R4）**: 29 件は**台帳未登録の関数先頭 thunk 先ではない**。(a) 24 件は VA 空間に存在しない slot 表記（台帳登録不能）、(b) 5 件はハンドラ内部アドレス（先頭台帳の対象外）。→ **relmap §3-③b（v158×GT 突合）では「台帳欠損 0 件・target_va 列に実 VA と slot 表記が混在」を確認結果として報告し、台帳追加対象は発生しない**。副次提案: v158_map 制作側で target_va を `target_kind = real|vslot` に分離するか正規化するか（→ 新規残点 N-2）。
ステータス: **個体特定完了・裁定済み**（優先: 低）。

### 4.2 R5: xcu_entries `in_v170=1` の唯一の個体 → **slot_va=00A2EFF0 / fn_va=00A10EB0（CRT init stub・L6b_I 採用）**

- 該当行は **1/7,402 件のみ**（v179 の「1 件」と一致）: `slot_va=00A2EFF0, fn_va=00A10EB0, in_func_list=0, in_union=1, in_v170=1`。
- v170 側の対応行（fn_va 一致は 1 行のみ・重複なし）:
  - layer=`L6b_I` / kind=`entry_gap` / status=`adopt_priority` / union_layer=`L3+L4+L6b:I1`
  - evidence `pad=12xCC; ret@00A10EA3` / notes `I_no=I1; call 0x7b7250@0xA10EB5; crt_init_stub; 139 residual stubs=V148`
- 解釈: `.CRT$XCU` 初期化配列の slot 00A2EFF0 が指す **CRT init stub（関数先頭 00A10EB0、ret 直前に 0x7B7250 呼び出し）** を L6b entry-gap I1 として v170 が採用した個体。`in_union=1`（v155 収録済み）であり x 系 CSV と v170 の整合は正常。

**裁定（R5）**: 個体特定済み・**追加作業なし**（x 系 CSV 上唯一の v170 交点は正常な採用済みエントリ）。ステータス: **完了**。

---

## §5 v181 R-b / R-g: 29×V176-f 交差と口径差注記先の一覧（編集なし）

### 5.1 R-b: 29 opid × §67.9 理由未特定 15 opid の集合交差 = **7 件**

- 交差（機械計算）: **`0x0016, 0x001E, 0x10D9, 0x10DA, 0x10DB, 0x1154, 0x115B`**（7 件。全件 scda 実測あり・§67.6 未掲載・UNDET15 内）→ 台帳 join 副産物で確認する際の**優先 7 件**はこれ（§3.2 表の ●）。
- **V176-f 本体（UNDET15 × 「SCPT 外 15 種」）への接続（追加整理）**:
  - UNDET15 のうち **scda 141 に在る 7 件 = 上記交差分**（= SCPT で使用実績あり = 「SCPT 外」ではない）。
  - 残り **8 件（`0x0117, 0x0158, 0x101F, 0x10C5, 0x10C9, 0x10F2, 0x111B, 0x113D`）が scda 141 に不在** → V176-f の交差母集団は**この 8 件に絞られる**（この 8 件すべてが ESM QUST/INFO に現れれば交差 = 8、現れないものは ESM 全体 0 回）。
  - 既知「SCPT 外 5 種」（0x10BF/0x10DE/0x1127/0x1133/0x1114）と UNDET15 の交差は **0 件**（排他を確認）。
  - → walk 時（G-D 解禁後）に owner matrix（v164_scda_owner_matrix.json）へ**この 8 件のみ**を照合すれば V176-f は確定可能。
ステータス: **台帳側交差一覧完了（母集団 8 件に短縮）**・残は walk 時（優先: 低）。

### 5.2 R-g(1): 65,628 vs 63,235 口径差の docs 注記先 — **v176 N8 の 12 ファイル 16 箇所を含む全 23 行の所在を機械確定（編集なし）**

v189_c_locations.py で行番号・値を実照合（**22/23 が当該行に値一致、1/23 は表記差のみ**）:

| # | ファイル:行 | 値 | 判定 | F1 扱い |
|---:|---|---|---|---|
| 1 | docs/RE_EXE_REPORT.md:10126 | 65,628 | OK | 注記追加 |
| 2 | docs/RE_EXE_REPORT.md:12105 | 65,628 | OK | 注記追加 |
| 3 | docs/RE_ANALYSIS_ROADMAP.md:266 | 65,628 | OK | 注記追加 |
| 4 | docs/SCRIPT_OPCODE_TABLE.md:33 | 65,628 | OK | 注記追加 |
| 5 | plan.md:454 | 63,235 | OK | 注記追加（excl） |
| 6 | plan.md:507 | 65,628 | OK | 注記追加 |
| 7–10 | plan.md:569 / 570 / 1091 / 1122 | 65,628 | OK×4 | 参照（incl 一貫・注記不要） |
| 11 | plan.md:815 | 65,628 | OK | 注記追加（併存） |
| 12 | plan.md:884 | 63,235 | OK | 注記追加（併存） |
| 13 | plan.md:916 | 63,235 | OK | 注記追加（併存） |
| 14 | tmp/re/b21_speculative_backing.md:24 | 65,628 | OK | **訂正必須**（名指しファイルと矛盾） |
| 15 | tmp/re/TODO_re49.md:261 | 65,628 | OK | **訂正必須**（踏襲） |
| 16 | tmp/re/s57_script_vm.md:273 | 65,628 | OK | 注記追加 |
| 17 | tmp/re/s59_section.md:33 | 65,628 | OK | 注記追加 |
| 18 | tmp/re/v75b_findings.md:16 | 65,628 | OK | 注記追加 |
| 19 | tmp/re/v75b_findings.md:23 | **`total_insns=65628`（カンマ無し表記）** | OK（表記差のみ） | 注記追加 |
| 20 | tmp/re/v75b_findings.md:62 | 65,628 | OK | 注記追加 |
| 21 | tmp/re/s59_svm_r7_bytecode.txt:2 | total_insns=65628 | OK | 訂正不要・注記のみ |
| 22 | docs/RE_EXE_REPORT.md:10932 | 0x001D Freq 2321（excl） | OK | **incl GT 更新 2 行（N1）** |
| 23 | docs/RE_EXE_REPORT.md:10933 | 0x0011 Freq 2023（excl） | OK | **incl GT 更新 2 行（N1）** |

- ファイル別出現数（参考）: plan.md 65,628×8 / 63,235×3、RE_EXE_REPORT.md 65,628×2、TODO_re49.md ×3 / ×2、他各 1。
- **scda_opcode_table_out.txt ヘッダ**: `scripts: 2393` / `complete: 2393/2393` のみで **incl 注記 0 件・'tail' 言及 0 件**（F1 のヘッダ注記追加対象であることを確認）。
- → v176 §8.1 の一覧は**現行版でも行番号ズレなしで有効**。F1 バッチ入力としてそのまま使用可（本タスクでは編集していない）。
ステータス: **所在確定一覧完了**（優先: 中 — F1 バッチ待ち）。

### 5.3 R-g(2): esm_inventory4 vs GT 口径差（STAT 5,945 件脱落）の注記先 — **統合先 `docs/RE_ESM_REPORT.md` は未開設を確認**

- `docs/RE_ESM_REPORT.md`: **不在（未開設）** → 主たる注記先は未確定であり、v181 R-a（docs 転記・要承認バッチ）に紐づく。
- 既存成果物中の記載位置（grep・編集なし）:
  - `tmp/re/v181_esm_integration.md:53`（§1.3 ドラフト、口径差は**記載済み** = R-g の前提どおり）。
  - **`tmp/re/s69_esm.md:6 / :252`** — 旧値 `1,161,072` を**口径注記なし**で記載（候補注記先。docs 転記時は GT=content_inventory_final 1,167,017 / inventory4 は STAT size=0 で 5,945 件脱落、と注記する）。
  - `tmp/re/copilot_archive/esm_inventory4.txt:4`（生出力・編集対象外）。
- **裁定**: 注記は (1) docs 転記時に `RE_ESM_REPORT.md` の inventory4 記述へ 1 行（GT との対比）、(2) `s69_esm.md` への追記は要承認のため F1 送り、の 2 箇所。本タスクでは**編集しない**。
ステータス: **注記先一覧完了・適用待ち**（優先: 低 — docs 転記時）。

---

## §6 v176 残り: V176-c（QUST/INFO 末尾ヒストグラム・仕様のみ）と V176-e（0x1133/0x1114 owner・既存キャッシュ調査）

### 6.1 V176-c: QUST/INFO ブロック末尾命令ヒストグラム — **GT 未計測のまま、測定仕様のみ確定**

- 現状: plan.md L541 は**先頭命令ヒストグラムのみ**（QUST: 0x001C 671 / 0x0015 365 / 0x1037 155 / 0x1039 130 / 0x1058 68…、INFO: 0x0015 3,427 / 0x1039 1,255 / 0x001C 516 / 0x1053 129 / 0x0016 46…）。末尾は未計測。
- 参考: **SCPT 側の末尾ヒストグラムは既存**（`scda_opcode_table_out.txt` L153–163「last instruction of each script」: 0x0019 1,606 / 0x0015 285 / 0x1000 57 / 0x100D 57 / 0x1039 50 / 0x0010 41 / 0x101D 25 / 0x112B 21 / 0x105E 18 / 0x1021 15 …。tail-excl 慣例のため「tail 直前の実命令」分布）。
- **測定仕様（walk 時・G-D 解禁後）**:
  1. 対象: QUST 1,701 ブロック + INFO 5,552 ブロック = **7,257 ブロック**（DLC の 26+1,055 は P5 で同式）。
  2. 各ブロックを framing 完走（Σ(4+arglen) == subrecord size、0x001C のみ固定 4B 例外）させ、**最終にデコードされた命令の opcode** のヒストグラムを owner 別に出力。
  3. 受け入れ値: (a) ブロック内 **0x0011 出現 = 0**（v176 §9.1 の算術証明の再確認）、(b) 先頭ヒストグラムが plan L541 上位 5 種と一致、(c) 9,646 ブロック zero misalignment の再現、(d) **末尾ヒストグラムは予想値を置かず新規 GT として記録**（v176 方針: 計測値を GT にする）。
  4. 既知の対照: 全体表（plan L512–536）では末尾相当の 0x0019=10,826 / 0x0015=17,529 等が先頭・本文混在の値であるため、末尾単独分布は本測定が初出になる。
- ステータス: **仕様のみ確定・GT 未計測**（優先: 低 — walk 副産物）。編集なし。

### 6.2 V176-e: `0x1133 SetLevel` / `0x1114 PlayBink` の owner 一意性 — **既存キャッシュ調査を完了し「一意決定不可」を確定**

調査方法（既存成果物のみ・ESM 再スキャンなし）: `tmp/re/` 全 md ＋ `plan.md` を対象に owner 決定に足る記述を grep（`AudensAvidius|BaurusRef|playbink|OblivionOutro|0x1133|0x1114` + owner 組合せ）。

| 証跡 | 内容 | owner 判定力 |
|---|---|---|
| plan.md:815 | `0x1133` はソース行 `AudensAvidiusRef.SetLevel 0` / `BaurusRef.setlevel 1, 1`（2 against 2）で命名 | **呼び出し参照名**であり**レコード名ではない** → owner 不明 |
| plan.md:821 | `0x1114` は unanimous vote `playbink "OblivionOutro.bik"`（`[1][string]`） | コーパス行のみ・レコード名なし → owner 不明 |
| `b09_opcode_categories.md:496/:527` | 両 opid の分類は「**推定**」（script★） | EXE 側分類 = owner 無関係 |
| `svm_495_miss_addrs.md:477` | 0x1133 = Op1133 / Freq `-`（台帳側） | Freq `-` = SCPT 外であることの傍証のみ |
| v176 §2.1 | 0x1133「QUST（推定、INFO 除外は未証）」/ 0x1114「non-SCPT（owner 未特定）」 | 現行裁定 |
| `%TEMP%\async_race\{opcode_freq_all.txt, corpus.pkl}` / `probe_align7.*`（0x1133 ソースダンプ） | **消滅確認済み**（v176 §0・本タスクでも再確認） | owner を含む一次出力は不在 |

**裁定**: **既存キャッシュのみでは一意決定不可 → V176-e は walk 時確定のまま解除しない。** 付随事実として以下を確定:

1. **内部張力の検出**: v176 **§0.1 N2 表**は「5 種を特定済み（0x10BF/0x10DE/0x1127/0x1133/0x1114、**いずれも QUST 側**）」と記すが、**§2.1 個別表**は 0x1133 を「QUST 推定・INFO 除外は未証」、0x1114 を「owner 未特定」とする。→ §0.1 の一括表現が個別表より強い（→ 新規残点 N-1、F1 注記提案）。
2. **方向性の確認**: 0x1133 / 0x1114 は scda 141 に**不在**（本タスクで機械再確認）かつ §67.6 では Freq `-` 行 → 「**台帳に在るが SCPT 実測に無い**」側の残差であり、§3 の 29 opid（**SCPT 実測に在るが台帳に無い**）とは**逆方向**（口径混同防止のため明記）。
3. **walk 時の判定式（仕様確定）**: `v164_scda_owner_matrix.json`（V176-f/N2-b と同一出力）で両 opid を **QUST exclusive / INFO exclusive / 双方**に分解。照合頻度: `0x1133` = **2**、`0x1114` = **≥1**（計測値として確定させる。頻度期待は v176 N2-c と同一）。
ステータス: **既存キャッシュ調査完了・walk 時確定を維持**（優先: 低）。

---

## §7 成果物

| パス | 内容 | 状態 |
|---|---|---|
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v189_esm_ledger_residuals.md` | 本報告（R1–R5 / R-b / R-g / V176-c/e の裁定） | 新規 |
| `%TEMP%\opencode\v189_opid29.csv` | 29 opid 一覧（12 列 × 29 行 + ヘッダ） | 新規 |
| `%TEMP%\opencode\v189_a_opid29.py` | R3/R-b 突合（scda × §67.6 × 532 表 × annotate_db） | 新規・再実行可 |
| `%TEMP%\opencode\v189_b_ledger.py` | R1/R4/R5 台帳 CSV 突合 | 新規・再実行可 |
| `%TEMP%\opencode\v189_c_locations.py` | R-g/N8 注記先の所在確定 | 新規・再実行可 |
| `%TEMP%\opencode\v189_d_execheck.py` | 29 opid impl 先頭バイト検証（capstone 5.0.7） | 新規・再実行可 |
| `%TEMP%\opencode\v189_a_out.txt` / `v189_b_out.txt` / `v189_c_out.txt` / `v189_d_out.txt` | 生出力証跡 | 新規 |

（各ファイルのバイトサイズは完了時計測で最終報告に記載）

---

## §8 新規残点

| # | 内容 | 優先 | 行先 |
|---|---|---|---|
| N-1 | v176 §0.1 N2 表の「5 種いずれも QUST 側」と §2.1 個別表（0x1133=推定・0x1114=未特定）に**内部張力**。§0.1 側を §2.1 基準へ寄せる注記が要る | 低 | F1（v176 改訂は要承認） |
| N-2 | `v158_map.csv` の `target_va` 列が実 VA と `vt+OFF` slot 表記の**混在**（24/29 が slot 表記）。列の正規化（`target_kind` 追加等）の要否 | 低 | relmap §3-③b |
| N-3 | V176-f の交差母集団を **UNDET15 のうち scda 非収録 8 件**（0x0117/0x0158/0x101F/0x10C5/0x10C9/0x10F2/0x111B/0x113D）に短縮した件を v176 仕様へ追記（本タスクは編集禁止） | 低 | F1 / walk 時 |
| N-4 | `tmp/re/s69_esm.md:6/:252` が旧値 `1,161,072` を**口径注記なし**で記載（GT 1,167,017 との対比注記が docs 転記時に必要） | 低 | docs 転記時（R-g(2)） |
| N-5 | 29 opid 中 7 件は「§67.9 理由未特定（UNDET）」と「SCPT 実測使用あり」の**併存**。V181 の impl 実体調査時の優先度材料（0x0016 は頻度 10,480 で最優先） | 低 | V181 |

（v179 R1–R5 / v181 R-b / R-g / V176-c / V176-e は本稿で裁定または一覧化済み。**未解消のまま残るのは walk 時確定系（V176-c/e の計測、N-1..N-5）のみ**）

---

## §9 自己検証・遵守事項

| # | 検査 | 結果 |
|---|---|---|
| 1 | `probe_harness.py selftest` | 着手時 **4/4 PASS** / 完了時再実行 **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| 2 | ESM / ESP / DLC 本体への読み取り | **0 回**（再スキャン禁止遵守。opcode 頻度は scda_opcode_table_out.txt のみ使用） |
| 3 | git | **不使用**（0 回） |
| 4 | docs / TODO / session-memory / plan / GT / 台帳 / 既存 CSV の編集 | **0 件**（すべて読取のみ。RE_EXE_REPORT.md は §67 節の読取のみ） |
| 5 | 既存 tmp/re 成果物の上書き | **0 件**（新規は本 v189_esm_ledger_residuals.md のみ） |
| 6 | Ghidra / codegraph 呼出 | 0 回 |
| 7 | EXE アクセス | `Oblivion.exe` を capstone 5.0.7 で **29 opid の impl 先頭バイト一覧確認のみ**（許可範囲。実体深部解析は V181 に委譲） |
| 8 | R1 検算 | dup 4 VA の v170 行 = 8（4VA×2層）✓ |
| 9 | R3 検算 | 112（台帳数値行）+ 29 = 141（scda distinct）✓ / Freq mismatch 0 ✓ / 29/29 が 532 表に在籍 ✓ |
| 10 | R4 検算 | unique target 54 = u155 内 25 + 外 29 ✓（外 29 = vslot 24 + ハンドラ内部 5） |
| 11 | R-g 検算 | 注記先 23 行中 22 行が当該行に値一致 + 1 行は `65628` 表記差（行ズレ 0）✓ |
| 12 | 失敗記録 | shell ツールが 2 回一時的に pwsh 起動不可（transient）→ 再試行で復旧。分析結果への影響 0 |

---
*v189（分析専任・L1 報告のみ）。裁定は本稿に記述し、F1 要承認バッチへの適用は親セッションの判断に委ねる。*
