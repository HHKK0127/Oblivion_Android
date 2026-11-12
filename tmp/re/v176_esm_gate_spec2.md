# V176: ESM 解禁時仕様の残点確定 N1–N9（v164 §7 の精緻化・分析専任）

作成: 2026-10-28（opencode / 分析専任サブエージェント、TODO serial 163 / V163・低）
出典: `tmp/re/v164_esm_gate_spec.md` §7 新規残点 N1..N9
対象成果物: 本ファイル（**新規・1 ファイル**）。docs / TODO / session-memory / plan 編集なし。**ESM/ESP 本体への読み取り走査は 0 回**（EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` の capstone 実測のみ実施）。

---

## §0 手法・入力・制約

| 項目 | 内容 |
|---|---|
| selftest | 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（完了時に再実行 = §11） |
| 中間スクリプト | `%TEMP%\opencode\v176_probe1..10.py`（新規。probe2/3/4 は VA/RVA 混同の失敗版。probe7 で VA=0x400000+RVA 補正） |
| 入力（読取のみ） | `v164_esm_gate_spec.md` / `v141_b19_quest_residuals.md` / `b21_speculative_backing.md` / `b20_impl_5071d0.md` / `copilot_archive/content_inventory_final.txt` / `scda_opcode_table_out.txt` / `s57_svm_8_opcode_table.txt` / `s58_qst_condtable.txt` / `s50_gmst_*`（該当なし） / `plan.md`（読取） / `docs/RE_EXE_REPORT.md` §18.2・§67.0–§67.9（読取） / `docs/Oblivion_decompiled.c`（部分検索: FUN_00450C20 / FUN_0056A7B0 / FUN_0056A970 / FUN_0056A850 / FUN_004FBF00 / FUN_00500280） / EXE（capstone 5.0.7 実測） |
| 中間キャッシュ | `%TEMP%\async_race\` は**空ディレクトリ**（opcode_freq_all.txt / corpus.pkl 消滅を再確認）。`scda_cache.pkl` 欠損（b21 §0.2）。`%TEMP%\scro_probe\` 不在 |

### 0.1 裁定サマリ（本タスクの結論先出し）

| # | 残点 | 裁定 |
|---|---|---|
| N1 | 台帳 Freq 列定義 | **tail 含む（incl）GT 採用・2 行のみ更新**（§1） |
| N2 | SCPT 外 15 種 | **既存キャッシュで 5 種を特定済み**（0x10BF/0x10DE/0x1127/0x1133/0x1114、いずれも QUST 側）。残 10 種は walk 時確定（§2） |
| N3 | CTDT 20B × 0x18 生読み | **超過読みなし・size 判定あり（EXE 実測で確定）**。20B は 20B まま読み、+0x14 は type フラグ、+0x15..17 は init 残骸（§3） |
| N4 | CTDA 保有レコード種 | **EXE 側で 4 種に限定（実測確定）**: IDLE(TESIdleForm, formType 0x3C) / QUST / INFO / PACK。既知 3 種 + **IDLE 追加**が推定上限（§4） |
| N5 | DLC ESP 全未走査 | **解禁時 P5 のまま（確認のみ）**（§5） |
| N6 | SCTX 差分判定式 | SCTX 本文キャッシュは既存 0 件 → **仕様化**（コメント/文字列除外トークン一致・δ<0 は即時停止）（§6） |
| N7 | scriptType 分布 | EXE は scriptType **verbatim 保存のみ・3 値分岐表は無い**（compiler コンテキスト表 0xB0A110 = 3 名称エントリ、quest gate は context==1 固定）。「0/1/256 以外」は記録値扱い（§7） |
| N8 | 慣例差の反映先 | **12 ファイル 16 箇所**を grep 特定（訂正必須 2 / 注記追加 12 / 台帳 2 行）（§8） |
| N9 | tail 4B 0x0011 の一般性 | **「tail 恒 0x0011」は SCPT 専用不変条件であることを算術証明**（全体表 0x0011=4,344 = SCPT tail-incl と一致 → QUST/INFO の 0x0011 は 0 件）。QUST/INFO の検証式は framing 完走 + 先頭/末尾ヒストグラムに置換（§9） |

---

## §1 N1: 台帳 Freq 列の定義裁定 — **tail 含む（incl）を GT に採用。差分は 0x0011 / 0x001D の 2 行のみ**

### 1.1 事実（既存キャッシュで確定）

1. 台帳 §67.6 の Freq 列の出典は **`tmp/re/copilot_archive/scda_opcode_table_out.txt`**（`RE_EXE_REPORT.md` §67.0 L10800 / §67.1 L10812 明記）。同ファイルの walk は `stop = n - 4` で **tail 4 B を数えない**（tail 除外慣例、Σ = 63,235）。
2. 台帳 112 正頻度 vs SCDA 表は **mismatch 0**（v164 §6-4 で実測）→ 既存 112 はすべて tail 除外値。
3. 両慣例で値が異なる台帳行は **2 行のみ**（v164 §3-2 の算術 3 系統一致）:
   - `RE_EXE_REPORT.md:10933`（rank6）`0x0011` Freq **2,023** → incl では **4,344**（+2,321 tail）
   - `RE_EXE_REPORT.md:10932`（rank5）`0x001D` Freq **2,321** → incl では **2,393**（+72 prologue-only）
   - 残る 110 行は tail が 0x0011 固定のため**両慣例で同一**（更新不要）。
4. 「incl = 正」を支持する正典側の記述は多数（`plan.md` L507 SCPT 65,628 / WS-B decoder census `decodeFail 0 / instr 65,628`（L569/570/1091/1122）/ `RE_EXE_REPORT.md` L10126・L12105 / `SCRIPT_OPCODE_TABLE.md` L33 / `s59_svm_r7_bytecode.txt` total_insns=65628）。excl は scda_opcode_table_out.txt / s57_svm_8 / plan.md L454・L884・L916 のみ。

### 1.2 裁定

> **台帳 Freq 列は tail を含む（incl）慣例を GT とする。** 既存 112 のうち **110 行は不変、2 行（0x0011 → 4,344 / 0x001D → 2,393）のみ更新**する。owner 別内訳（esm_scpt / esm_qust / esm_info / dlc_total）は v164 §3.4 の出力スキーマ側で保持するため、**台帳に 3 列化は不要**（最小変更。列名に慣例を明示する注記 1 行を F1 バッチで追加）。

### 1.3 検証式（P3 完了時に自動裁定）

| # | 検証 | 受け入れ値 |
|---|---|---|
| N1-a | `tail_excl` 列 vs 既存台帳 112 正頻度 | **mismatch 0**（walker 回帰、v164 §3.3-1 と同一） |
| N1-b | `incl` 列 vs 既存 112 | 差分は **ちょうど 2 行**（0x0011=4,344 / 0x001D=2,393）。3 行目が出たら walker 異常 |
| N1-c | incl の Σ | SCPT 65,628（plan.md L507 一致） |
| N1-d | F1 バッチへの送り | 更新 2 行 + 注記 1 行のみ（§8 の一覧と接続） |

---

## §2 N2: SCPT 外 15 種の実在確認 — **既存キャッシュで 5 種を特定。残 10 種は walk 時確定**

### 2.1 既存キャッシュで特定できた範囲 [確定]

`plan.md` L813–823（2,646 (SCDA,SCTX) コーパスのソース整合命名）から、**141 表（SCPT）に無いが ESM に実在する opcode** を抽出（本タスク `v176_probe1.py`）:

| opid | 名称（plan.md 出典） | 出現 | 帰属（コーパス記述から） |
|---|---|---|---|
| `0x10BF` | ModAmountSoldStolen | 10（TG10Boots = QUST ステージ script） | QUST |
| `0x10DE` | CloseOblivionGate | 3（MQ13Gate1/2/3 連続 3 本） | QUST |
| `0x1127` | ResetInterior | 8（record 2407、addTopic 9 本と setStage 4 本の間） | QUST |
| `0x1133` | SetLevel | 2（AudensAvidiusRef / BaurusRef 行） | QUST（推定、INFO 除外は未証） |
| `0x1114` | PlayBink | ≥1（`playbink "OblivionOutro.bik"`、unanimous vote） | non-SCPT（owner 未特定） |

→ **SCPT 外 15 種のうち 5 種を既存キャッシュで実在確認**。残 10 種（うち INFO exclusive 8 + QUST/INFO 双方 2 と推定）の identity は `%TEMP%\async_race\opcode_freq_all.txt`（**消滅確認済み**）のみに存在したため、**walk 時確定が必須**。

### 2.2 差 2（17 vs 15）の調和解釈 [維持・walk 時確定]

- plan.md L543: 「186 命令（QUST+INFO の 0.87%）が SCPT 外 opcode = **9 exclusive to QUST + 8 exclusive to INFO**」。
- distinct 算術: 156（全体）− 141（SCPT）= **15**。9 + 8 = 17 との差 2 は「**QUST と INFO の両方に現れる SCPT 外 opcode が 2 種**」で調和する（各 exclusive 集合の重複 |A∩B| = 2 → |A∪B| = 15）。v164 §3.3-5 の推定を維持。

### 2.3 walk 時の検証式

| # | 検証 | 受け入れ値 |
|---|---|---|
| N2-a | SCPT 外 distinct | **15**（= 156−141） |
| N2-b | owner 分解 | QUST exclusive **9** / INFO exclusive **8** / 双方 **2**（`v164_scda_owner_matrix.json` で分解） |
| N2-c | 既知 5 種の再確認 | 0x10BF / 0x10DE / 0x1127 / 0x1133 / 0x1114 が ESM の QUST/INFO 側に在ること（頻度は 10/3/8/2/≥1 で照合） |
| N2-d | 186 命令 | SCPT 外 opcode 上の QUST+INFO 命令数 = **186**（plan.md L543 一致） |
| N2-e | 残 353 件 | §67.6 の `-` 368 のうち 15 種を除く **353 種は ESM 全体 0 回確定**（v164 §3.3-5） |

---

## §3 N3: CTDT 20 B と b20 の 0x18 生読みの整合 — **超過読みなし・size 判定あり（EXE 実測で確定）**

### 3.1 EXE 実測（本タスク probe8/probe10 + decompiled）

読込ヘルパ `FUN_00450C20`（GetChunkData、`docs/Oblivion_decompiled.c:61751` + capstone `0x450C20`）の分岐を asm で確定:

```
cmp dword [esi+0x254], 0            ; payload size == 0 ?
je  return(al=1)                    ; dest 無変更で即 return
mov edi, [esp+0x30]                 ; edi = maxSize (param_3 = 0x18)
test edi, edi / je 0x450E0A         ; maxSize==0 → 全読み
cmp dword [esi+0x254], edi / jbe 0x450E0A   ; size <= maxSize → 全読み
; --- 超過パス（size > maxSize）のみ: ---
mov byte [edi+ebp-1], 0             ; dest[maxSize-1] = NUL
call FUN_0042C8E0(dest, maxSize-1)  ; maxSize-1 バイトだけ読む
call FUN_004a7a60("Chunk size %d too big ... data truncated ...")
; --- 全読みパス 0x450E0A: ---
mov ecx, [esi+0x254]                ; ReadFile/memcpy 長 = size のまま
```

→ **`size <= maxSize` なら size ちょうどを読む（超過読みゼロ）。超過時のみ maxSize-1 + NUL 切り詰め + ログ。**

ロードハンドラ `FUN_0056A7B0`（`decompiled:270214`）: tag 判定（"CTDA"=0x41445443 / "CTDT"=0x54445443）→ `malloc(0x18)`（`FUN_00401f00`）→ **`FUN_0056AE00` で 0x18 ゼロ + 既定条件（+8=0x4C=GetDisposition X）初期化** → `FUN_0056A970` → `FUN_00450C20(record, dest, 0x18)` → `*(byte*)(dest+0x14) = (tag=="CTDT")`（`decompiled:270350`、type フラグ上書きは +0x14 下位バイトのみ）→ リスト挿入。

### 3.2 裁定（20 B CTDT の実挙動）

| 項目 | 挙動 |
|---|---|
| 読み込み | 20 ≤ 0x18 → **全読みパス**: `dest[+0x00..+0x13]` = ファイル 20 B、**バッファ超過なし** |
| `dest[+0x14]` | type フラグ = **1（CTDT）**（ファイル由来でない） |
| `dest[+0x15..+0x17]` | **初期化残骸 = 0**（FUN_0056AE00 のゼロ埋め。ファイル由来でない） |
| 超過パス | vanilla では **発生しない**（CTDA 全件 24 B = maxSize ちょうど、CTDT 20 B < 24）。超過ログ "Chunk size %d too big" は ESM では 0 件のはず（walk の size ヒストグラムで間接確認） |
| 保存（round-trip） | `FUN_0056AD60` は **tag 固定 "CTDA" + 0x18 verbatim**（caller 1 件、probe9）。→ **20 B CTDT をロード→保存すると 24 B CTDA に変わる**非対称。vanilla は condition を再保存しないため ESM には現れないが、移植側は「CTDT は 20 B のまま保持し、保存時も CTDT+20 B を対称に書く」か「EXE 同様に潰す」かを明示すること |

### 3.3 v164 §1.2 C6 への回答

`payload[8:10]`（i = u16）は 20 B の中身（+8..+9）なので **CTDT でも安全に読める**（超過読みは発生しない）。CTDT の `i` は別集計せよ（v164 C6 の指示を維持）。**b20 §6-3 の「0x18 生読みとの整合は未確認」は本裁定で解消。**

---

## §4 N4: PACK/QUST/INFO 以外の CTDA 保有レコード種 — **EXE 実測で 4 種に限定（+IDLE）**

### 4.1 EXE 実測（本タスク probe7/probe9）

- `FUN_0056A7B0`（CTDA/CTDT 共通ローダ）の **.text 内 E8 呼出し = 4 件のみ**（raw rel32 走査。Ghidra decompiled の 4 箇所と一致 = 呼出し欠落なし）。
- 4 件はいずれも**クラス vtable slot7（delta 28）のサブレコード読込ハンドラ**本体。各クラスは RTTI（vtable[-1] = COL → TD 名）と ctor の formType 代入で特定:

| # | ハンドラ | RTTI（TD 名） | formType | レコード種 |
|---|---|---|---|---|
| 1 | `FUN_00520970` @0x520CAB 呼出 | `…dleForm@@` = **TESIdleForm** | ctor `mov byte [esi+4], 0x3C`（=60） | **IDLE** |
| 2 | `FUN_0052A570` @0x52A84F 呼出 | `…uest@@` = **Quest** | （QUST = formType 59、v141 §4.2 の表） | **QUST** |
| 3 | `FUN_00530FB0` @0x53115E 呼出 | `…opicInfo@@` = **DialogInfo** | （INFO = 58） | **INFO** |
| 4 | `FUN_00566530` @0x566767 呼出 | `…ackage@@` = **Pack** | （PACK） | **PACK** |

- 参考: FUN_0056A7A0 = `jmp FUN_0056A750`（cond リスト初期化系。4 クラス + Quest コンテナ等 7 呼出し）。

### 4.2 裁定

> **CTDA/CTDT をロードし得るレコード種は IDLE / QUST / INFO / PACK の 4 種に限定される（EXE 構造的に確定）。**
> 既知 3 種（PACK 6,172 + CTDT 18 / QUST 3,784 / INFO 48,531 + CTDT 72）に加え、**IDLE（ESM 650 件、content_inventory_final.txt:18）が唯一の未計測 owner 候補**。
> 解禁時 walk で `new_owner_tags` に**この 4 種以外**が出た場合は「EXE がロードしないデーブル内 CTDA」＝ データ異常または新規発見として最大限重要度で報告する（EXE は slot7 ハンドラ以外で CTDA を受け付けない）。

### 4.3 検証式

| # | 検証 | 受け入れ値 |
|---|---|---|
| N4-a | 既知 3 種の件数 | PACK 6,172（+CTDT 18）/ QUST 3,784 / INFO 48,531（+CTDT 72）で個別一致（v164 C1） |
| N4-b | IDLE の件数 | **新規計測**（既存キャッシュに IDLE CTDA 件数は 0 件。v164 §6-11「ESM 側 opid 分布を持つ成果物は 0 件」の状況維持）。上限予想なし・測定値として確定 |
| N4-c | new_owner_tags | 上記 4 種以外は **0 件**が EXE 構造からの予測。1 件でも出れば「ローダ非接続 CTDA」として報告 |
| N4-d | 件数合計 | ≥ 58,487 + IDLE 分（v164 C1 の合計式を更新） |

---

## §5 N5: DLC ESP 全未走査 — **解禁時 P5 のまま（確認のみ）**

- `content_inventory_final.txt`（errs=0）で件数のみ既知: DLC 9 ファイル SCPT 653 / INFO 1,055 / QUST 26（+ DLCShiveringIsles.esp = 85 B スタブ = 走査対象外、同ファイル L35）。
- SCDA / SCHR / CTDA / scriptType / TrapUpdate の DLC 値は**未知（0 件の知見なし）**。旧 `esp_inventory.txt` は errs>0 で GT 不可（v164 §0.2）。
- → **本タスクでの変更なし。v164 §5 P5（0.25 人日、content_inventory_final 件数一致 / errs 0 ゲート）を維持。** IDLE の DLC CTDA も P5 に含まれる（N4-b の DLC 列）。

---

## §6 N6: SCTX `TrapUpdate` キーワードと SCDA 22 の差分判定式 — **仕様化（既存 SCTX キャッシュ 0 件）**

### 6.1 既存キャッシュの状況 [確定]

- SCTX 本文を持つ既存成果物は **0 件**（`%TEMP%\async_race\` 空、corpus.pkl / scda_cache.pkl / opcode_freq_all.txt 消滅。tmp/re 内の TrapUpdate 言及はすべて EXE 側表・台帳。plan.md に `trapupdate` は名称表 1 行のみ（L665）でソース行引用なし）。
- → 検証は P2 実走査時。**判定式を本節で仕様化**（v164 §9.4 の差分報告方針を形式化）。

### 6.2 判定式（レコード単位）

```
n_scda = レコードの全 SCDA ブロック中 0x110C の出現数（block 単位 walk、v164 §2.2）
n_src  = 同一レコードの全 SCTX 連結テキストから次を除外した後の
         トークン境界一致（英数字/_ に囲まれない独立語、大文字小文字非区別）の出現数
         除外対象:
           (a) ';' 以降の行コメント（SCTX のコメント記号）
           (b) '"' で囲まれた文字列リテラル内部
           (c) EDID 名との接尾一致（"TrapUpdate" を含む識別子は一致としない）
δ      = n_src − n_scda
```

### 6.3 合格条件

| # | 条件 | 扱い |
|---|---|---|
| N6-a | ESM SCPT: Σ n_scda = **22**（T1 と同一） | walker 回帰 |
| N6-b | レコード単位 δ ≥ 0 | **許容**（コメント/文字列混入由来）。δ > 0 の全レコードを EDID 付きで報告 |
| N6-c | レコード単位 δ < 0 | **即時停止レベルの矛盾**（ソースに無い命令が SCDA に在る = walker 誤りか SCTX 分割読み取り漏れ。注意: 210 分割レコードは非 SCPT のみ = plan.md L813、**SCTX 連結は必須**） |
| N6-d | SCTX 無し SCDA 有レコード | n_src=0 として別集計（QUST 1,277 ブロック = header without body 系、v164 S3） |
| N6-e | QUST/INFO/DLC | 既知値なし → 新規計測値として確定（T5 と同格） |

---

## §7 N7: QUST/INFO/DLC の scriptType 分布測定仕様 — **EXE は verbatim 保存のみ。3 値マッピング表は EXE に存在しない**

### 7.1 EXE 実測（本タスク probe8/probe10 + v141 突合）

| 検査 | 結果 |
|---|---|
| scriptType の格納 | SCHR(+0x10) → `Script+0x28` へ **20 B 直書き verbatim**（v141 §4.2 の 3 経路。変換なし） |
| scriptType の読取り経路（分岐） | in-game compile 入口 `FUN_004FBF00` の呼出し = **2 件のみ**（0x586EAB / 0x586FB6、いずれも FUN_00586000 内）。両サイトとも param_3（compile context）= **`push 1` 固定** → `if (param_3 == 1) DAT_00b361ac = 1`（quest モードフラグ）。**context は scriptType から導出されず固定** |
| compiler コンテキスト表 | `PTR_s_DefaultCompiler @ 0xB0A110` = **3 名称エントリ**: `[0] "DefaultCompiler"` / `[1] "SysWindowCompileAndRun"` / `[2] "DialogueCompileAndRun"`（参照 1 箇所 = 0x50038F、`FUN_00500280` 内 `(&PTR_s_DefaultCompiler_00b0a110)[local_5c]`）。scriptType 256 をそのままインデックスすると表外 = **scriptType がコンテキストに転用されることはない** |
| 3 値 {0,1,256} の分岐表 | **EXE 内に存在しない**（唯一の関連分岐は context==1）。{0,1,256} は ESM 側 SCPT の実分布 GT（plan.md L921: {0:2,031, 1:265, 256:97}）であり、0=Object / 1=Quest / 256=Magic は **CS 側の生成規約** |

### 7.2 測定仕様（v164 §4.2 S6 を維持しつつ補正）

1. SCHR 全数 walk で owner 別 `scriptType` ヒストグラムを出力（SCPT GT {0:2031,1:265,256:97} との mismatch 0 = S8 ゲート）。
2. QUST の SCHR は quest スクリプト（QSDT 1:1、2,978 件）→ **想定は 1 のみだが値を予想で裁かない**。INFO SCHR（19,231）→ 測定値。DLC 全 owner → 測定値。
3. **`0/1/256` 以外の値が出た場合の裁定**: EXE は verbatim 保持し実行時に参照しないため**実害なし**。ただし (a) 保存側 slot9 も verbatim 20 B 書き出し（v141 §4.3）なので round-trip 安全、(b) quest ゲートは context 固定で scriptType 非依遷 — よって「新規値 = 記録値として確定」でよく、**EXE マッピング不足による移植ブロッカーではない**（v164 S6 の「マッピング不足」表現をここで緩和）。
4. 副次チェック（v141 残点(1)と接続）: scriptType の **byte2/3 は全件 0** であること（seed 上位 2 B が write-back で残る唯一の成分。plan.md L921 の GT では 0 が全件）。

---

## §8 N8: 63,235 ↔ 65,628 慣例差の反映先一覧（grep 特定・編集なし。F1 バッチ入力）

### 8.1 一覧（12 ファイル 16 箇所）

| # | ファイル:行 | 値/慣例 | 扱い（F1） |
|---|---|---|---|
| 1 | `docs/RE_EXE_REPORT.md:10126` | 65,628（incl、§59.3 R7） | 注記追加: 「tail 含む」 |
| 2 | `docs/RE_EXE_REPORT.md:12105` | 65,628（incl、国勢調査） | 注記追加 |
| 3 | `docs/RE_ANALYSIS_ROADMAP.md:266` | 65,628（SCRIPT_OPCODE_TABLE 注記節の記録） | 注記追加 |
| 4 | `docs/SCRIPT_OPCODE_TABLE.md:33` | 65,628（incl、正典表） | 注記追加（証跡 s59 = incl で正しい） |
| 5 | `plan.md:454` | **63,235**（excl） | 注記追加: 「tail 除外慣例。incl では 65,628」 |
| 6 | `plan.md:507` | 65,628（incl） | 注記追加（対: L454） |
| 7 | `plan.md:569/570/1091/1122` | 65,628（decoder census） | 注記不要（incl で一貫）・参照 |
| 8 | `plan.md:815/884/916` | 65,628 / 63,235 併存記述 | 注記追加（L815 は「65,628 …matching the native_scda_decoder」明記） |
| 9 | `tmp/re/b21_speculative_backing.md:24` | 「scda_opcode_table_out.txt = 65,628 命令」と**誤記**（実体 Σ=63,235） | **訂正必須**（名指しファイルと矛盾） |
| 10 | `tmp/re/TODO_re49.md:261` | b21 L24 の誤記を踏襲 | **訂正必須** |
| 11 | `tmp/re/s57_script_vm.md:273` | 65,628（incl、0x001C 固定 4B 完走） | 注記追加 |
| 12 | `tmp/re/s59_section.md:33` | 65,628（incl） | 注記追加 |
| 13 | `tmp/re/v75b_findings.md:16/23/62` | 65,628（incl。s59 total_insns=65628 引用） | 注記追加（値は incl で正しい） |
| 14 | `tmp/re/s59_svm_r7_bytecode.txt:2` | total_insns=65628（incl 証跡） | 訂正不要・慣例注記のみ |
| 15 | `tmp/re/copilot_archive/scda_opcode_table_out.txt` | Σ=63,235（excl。ヘッダに注記なし） | ヘッダ注記追加（F1 のみ） |
| 16 | `docs/RE_EXE_REPORT.md` §67.6 `:10932/:10933` | 0x001D=2,321 / 0x0011=2,023（excl） | **incl GT への更新 2 行**（N1 裁定） |

（`docs/SCRIPT_OPCODE_TABLE.md.pre83/.pre84` は grep 0 件 = 対象外。`b14_slot_method_map.md` にも 0 件。`v164_esm_gate_spec.md` §3-2 は解明記述そのもの = 参照のみ・編集禁止。）

### 8.2 F1 バッチの最小差分

1. **訂正**: #9 / #10（「65,628」とファイル名の組み合わせを「63,235（tail 除外）」へ。あるいは 65,628 のみに直し出典表記を s57_svm_8 に変更）。
2. **更新**: #16 の 2 行（0x0011 → 4,344 / 0x001D → 2,393、incl 採用時）。
3. **注記 1 行テンプレ**（#1–#8, #11–#15 共用）: `※この命令数は tail(STOP 4B) を含む（incl）慣例。tail 除外は Σ=63,235（scda_opcode_table_out.txt）。差 = ブロック数 2,393。` —— 反復注記は F1 で各所に 1 行ずつ。

---

## §9 N9: 「tail は必ず 4 B 0x0011」の QUST/INFO/DLC への検証式 — **SCPT 専用不変条件であることを算術証明。QUST/INFO は framing 完走で検証**

### 9.1 既存キャッシュによる証明 [確定]

- plan.md L512–523 の**全体頻度表（9,646 ブロック / 86,916 命令、SCPT は incl 慣例）**で `0x0011` = **4,344**。
- SCPT の incl 0x0011 = 表 2,023 + tail 2,321 = **4,344**（v164 §3-2）。
- → **QUST（10,326 命令）+ INFO（10,962 命令）の 0x0011 出現は 0 件**（差がゼロ）。
- さらに plan.md L541: QUST/INFO ブロックは先頭が 0x001D でない（QUST 先頭 = 0x001C 671 / 0x0015 365 / 0x1037 155 / 0x1039 130 / 0x1058 68…、INFO 先頭 = 0x0015 3,427 / 0x1039 1,255 / 0x001C 516 / 0x1053 129 / 0x0016 46…）。**result script は begin/end ラッパを持たない裸の命令列**であり、tail STOP も持たない。ブロック境界は **SCDA サブレコード長**そのもの（plan.md L503: 全 9,646 ブロック zero misalignment = Σ命令長が全ブロックでサブレコードサイズに一致済み）。

### 9.2 裁定

> **「末尾 4 B = 0x0011」は SCPT ブロック（+DLC SCPT は要検証）専用の不変条件。QUST/INFO ブロックには tail が存在しない。**
> v164 §2.2 の walk 規則「terminator: 末尾 4 B = 11 00 00 00」を **QUST/INFO/DLC ブロックに適用してはならない**（適用すると framing fail が 7,257 ブロック出る）。P0 walker は owner 分岐必須。

### 9.3 検証式（owner 分岐）

| owner | 検証 | 受け入れ値 |
|---|---|---|
| SCPT（ESM） | 先頭 4 B = `1d 00 00 00` | 2,393/2,393 |
| SCPT（ESM） | size==4 ブロック = prologue のみ | 72 件、全件 `1d 00 00 00` |
| SCPT（ESM） | size>4 ブロックの末尾 4 B | 2,321/2,321 が `11 00 00 00` |
| QUST/INFO | **tail 条件なし**。代替: (a) framing 完走 Σ(4+arglen) == subrecord size（0x001C のみ固定 4B 例外）、(b) **ブロック内 0x0011 出現 = 0**（§9.1 の算術を walk で再確認）、(c) 先頭命令ヒストグラム = plan.md L541 の上位5種と一致、(d) 末尾命令ヒストグラムは新規計測値（0x0011 混入 0 を確認） | 9,646/9,646 zero misalignment の再現 |
| DLC SCPT | ESM と同じ prologue/tail 構造を**仮説として検証**（653 ブロック） | 仮説破れは新規事実として記録（P5） |
| DLC QUST/INFO | QUST/INFO 同様（tail なし仮説、26+1,055 ブロック） | 同上 |

### 9.4 0x110C 頻度への影響（T6 の再確認）

tail 慣例の差（excl/incl）が及ぶのは **SCPT の tail（0x0011 固定 4 B）のみ**。0x110C は arglen 0 の 4 B 命令だが **tail は 0x0011 固定なので tail 位置に 0x110C は来得ない** → T6「0x110C は両慣例で同一値（=22）」は QUST/INFO でも vacuous に成立（QUST/INFO に tail 概念自体がないため）。

---

## §10 新規残点（本タスクで検出した未解決点）

| ID | 内容 | 優先度目安 |
|---|---|---|
| V176-a | **IDLE の CTDA 実数**（ESM 650 IDLE 中、何件が CTDA を持つか）— 既存キャッシュに 0 件、walk 時確定（N4-b）。DLC の IDLE も同様 | 低（walk 副産物） |
| V176-b | **CTDT round-trip 非対称**（20 B CTDT → 保存時 24 B CTDA に潰れる、N3.2）。vanilla ESM には現れないが移植仕様で明示が必要 | 低 |
| V176-c | **QUST/INFO ブロック末尾命令ヒストグラム**が未計測（plan.md は先頭のみ記載）。N9 検証式 (d) の GT は walk 時に新規確定 | 低 |
| V176-d | N2 の残 10 種（INFO exclusive 8 + 双方 2）の identity — walk 時確定（2.3 N2-c）。既存キャッシュでは不可 | 低 |
| V176-e | `0x1133 SetLevel` / `0x1114 PlayBink` の owner（QUST or INFO）が既存記述から一意に決まらない（2.1）— walk 時確定 | 低 |
| V176-f | 台帳 §67.9 の理由未特定 15 opid と「SCPT 外 15 種」の集合交差が未照合（別空間の 15 件だが、0x0016/0x001E 等が重なり得る）。P3 の台帳 join 副産物で確認可 | 低 |

---

## §11 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | `probe_harness.py selftest` | 着手時 **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| 2 | ESM/ESP 本体への読み取り | **0 回**（メタデータ参照も今回なし。EXE のみ読解） |
| 3 | N3 根拠 | capstone `0x450C20` size 分岐（`cmp [esi+0x254],edi / jbe 0x450E0A` + 超過パス `mov byte[edi+ebp-1],0`）+ decompiled L61788–61857 の **2 系統一致**。full-read パス 0x450E0A で読み長 = `[esi+0x254]` を実測 |
| 4 | N4 根拠 | E8 raw 走査で FUN_0056A7B0 = **4 calls**（0x520CAB/0x52A84F/0x53115E/0x566767）、vtable slot7（delta 28）4 件、RTTI 名 TESIdleForm/Quest/DialogInfo/Pack、formType 0x3C 実測 |
| 5 | N7 根拠 | `PTR_s_DefaultCompiler @ 0xB0A110` = 3 エントリ（"DefaultCompiler"/"SysWindowCompileAndRun"/"DialogueCompileAndRun"）、参照 1 箇所（0x50038F）、compile 入口 2 site とも `push 1` 固定を capstone で実測 |
| 6 | N9 根拠 | plan.md L521 全体表 0x0011=4,344 と SCPT incl 4,344 の一致（QUST/INFO=0 件の算術証明）+ L541 先頭命令分布 |
| 7 | N1/N8 根拠 | §67.0/§67.1 の Freq 出典明記（scda_opcode_table_out.txt）+ v164 §3-2/§6-4 の mismatch 0 + grep 12 ファイル 16 箇所 |
| 8 | N2 根拠 | v176_probe1.py: plan.md 内 141 表外 opcode 抽出（0x10BF/0x10DE/0x1127/0x1133/0x1114/0x10FA/0x111B 等）→ L813–823 の出現文脈と突合し 5 種に絞込み |
| 9 | 失敗記録 | v176_probe1.py 初版（括弧位置誤り）/ probe2–5（VA/RVA 混同: RTTI 探索不能）→ probe7 で VA=0x400000+RVA に補正、probe9 で E8 target を RVA 空間比較に補正。**誤りは補正済みで、本稿の数値はすべて補正版の実測** |

証跡: `%TEMP%\opencode\v176_probe1.py` / `v176_probe7.py` / `v176_probe8.py` / `v176_probe9.py` / `v176_probe10.py`（probe2–6 は過程用）。

---

## §12 1 行まとめ

> **N1 は「incl GT・2 行更新」、N3 は「超過読みなし（size 判定あり）」、N4 は「EXE 上は IDLE/QUST/INFO/PACK の 4 種限定」、N7 は「scriptType は verbatim 保存のみ・3 値分岐表なし」、N9 は「tail 0x0011 は SCPT 専用」と**既存キャッシュ + EXE 実測で**裁定し**、N2 は 15 種中 5 種を既存記述で特定、N6 は差分判定式を仕様化、N8 は 12 ファイル 16 箇所の反映先を F1 バッチ入力として列挙、N5 は P5 維持——v164 の P0–P5 walker は **owner 分岐（tail 規則・CTDA owner 4 種・scriptType 測定）を加味した 1 pass** で実行すればよい。
