# v293 — V219 残点6件の後続ラウンド照合（クローズ可否判定）（2026-11-05）

- 対象: `tmp/re/v223_ini_docs_bundle4.md` §6 新規残点 6 件（台帳 `TODO_re49.md` row219 = V219、成果物予定 `v225_ini_docs_bundle5.md` は**未作成**）
- 照合先: 後続ラウンド `tmp/re/v265`〜`v288`（機構補強のため `v264_bundle_residuals.md`・`v275_ui_residuals2.md` も参照）
- 方法: 文字列・アドレス needle の grep 照合＋該当節読解（**バイナリ再測なし**＝照合レポート。証拠は各出典ファイルの行番号で示す）
- selftest: 着手時 **4/4 PASS**（`tmp/re/handover_20261025/scripts/probe_harness.py selftest` → `bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl@0x504560` ALL PASS、`probe_last_report.txt`）
- 制約遵守: L1 report-only。本ファイル新規 1 件のみ。GT/台帳/docs/plan/既存 tmp/re は無編集。

---

## 1. 判定表（結論）

| # | 残点（v223 §6） | 判定 | 待ち条件 / 出典 |
|---|---|---|---|
| 1 | GECK 本体入手時の [ShockBolt]/[Absorb]/Xenon 閾値消費検証 | **未解消（環境待ち）** | GECK.exe 不在のまま・該当 needle 0 |
| 2 | `copilot_ini_registries.txt`「skipped: unresolvable name 3」実体突合 | **未解消（ファイル不在）** | registries needle 0 件 |
| 3 | getter thunk `0xA1B480` 帯の二次登録経路 | **部分解消** | 機構=確定（v264/v275）／対象範囲=未調査 |
| 4 | [PORT] Controls 表記差の A/B 選択 | **未解消（承認ゲート）** | 製品判断待ち・v272 の ControlsMenu は別件 |
| 5 | v218 補遺2ドラフトの fAlpha 既定値訂正（0.0→0.5f） | **未解消（適用待ち）** | 適用先ファイル不在・needle 0 |
| 6 | v223 §2-1 wording 精緻化（「実行時生成」→「VA 固定・.bss 領域」） | **未解消（docs 反映指示待ち）** | 反映記録なし |

**内訳: 完全解消 0 / 部分解消 1 / 未解消 5。**

要点: 未解消 5 件のうち **再調査（RE 実施）を要するのは残点3 の「対象範囲」のみ**。他 4 件は ①外部ファイル/本体の入手待ち（#1/#2）、②[PORT] 承認待ち（#4）、③docs 適用待ち（#5/#6）＝いずれも**追加の静的解析不要**。

---

## 2. 各項目の詳細（証拠付き）

### 残点1 — GECK 消費検証 【未解消・環境待ち】

- needle `ShockBolt|Xenon|Absorb` を `tmp/re/v2[6-8]*.md` に投射 → **該当 0 件**。「閾値」のヒットは全て `v265_opcode_gt_names14.md` L218-221 の**ロック強度閾値 `iLockLevelMax*`（既定 7/20/40/80/99・`FUN_004299E0` 難易度段階化）系＝完全別件**。
- 後続の GECK 言及（v265/v268/v272/v282/v283）は MGEF bit24 設定挙動・MagicItem slot+0xE8 dead virtual・bit5 記録側名系で、いずれも v223 残点1 の「閾値を読む外部ツール」とは無関係。
- v223 §7 の「GECK.exe/CreationKit 系が本環境に不在」は後続でも変化なし。
- **判定: 未解消。GECK 到着時のみの追加検証（優先度 低）＝再調査不要。**

### 残点2 — registries.txt「skipped: unresolvable name 3」突合 【未解消・ファイル不在】

- needle `registries` を `v2[6-8]*.md` に投射 → **0 件**。
- v223 §7: `copilot_ini_registries.txt` はプロジェクト内 glob 0 件・Steam 配下・D:\Cargo 主要配下でも未発見（v218 まで遡っても同様）。後続 24 ラウンドで発見記録なし。
- **判定: 未解消。同ファイル入手待ち（優先度 低）＝再調査不要。**

### 残点3 — getter thunk `0xA1B480` 帯の二次登録経路 【部分解消】

- **v223 の原記載**（L185/L191/L203）: `.text` 内 imm32 参照 `0xB08158`/`0xB08160` 各 6 件。getter thunk `0xA1B480`/`0xA1B4B0` 帯は `push <node>; mov ecx,0xB07CFC; call 0x65C620` で、bLightAttenuation ノードが**容器 0xB07CFC にも二次登録される経路**。機能・対象範囲とも未調査。
- **後続による機構特定（解消部分）**:
  - `v264_bundle_residuals.md` L134-160（§2 項9）: 同型 `push &slot0xB14800(+8i); mov ecx,0xB07CFC; call 0x65C620` を追跡し、**`0x65C620` = 番兵付き単方向リスト（head {first,next} @0xB07CFC = manager+0x20c）のノード操作**、**`0xB07CFC` singleton = INISetting キー登録テーブル**（スロット 8B = {キー名文字列 ptr, 値}・データ駆動・imm32 直接参照 0 件）[確定・命令]。消費側は 12 thunk（`0xA24EA0..0xA250C0`・stride 0x30）。
  - `v275_ui_residuals2.md` L13/L53（W-1）: 姊妹 site `0xA18BD0`（atexit 側）`push 0xB06C74; mov ecx,0xB07CFC; call 0x65C620` = **終了時のノード除去＋free**（直後にキー文字列がヒープマジック `'S'`(0x53) の時のみ `0x401F20 free`）＝0x65C620 が登録/解除双方向のリスト操作である裏付け。
- **残る不足（未解消部分）**: `0xA1B480`/`0xA1B4B0` **そのもの**の個別呼び出し元・対象範囲（bLightAttenuation 群以外を含む全数）は v265-v288 で直接調査されていない。needle `0xA1B480` のヒットは **v223 本体・台帳 row219（と .bak 群）のみ**で、後続レポートに追跡記録なし。
- **判定: 部分解消** — 機構（0xB07CFC=INI 登録/解除テーブル、0x65C620=リスト操作）は確定。対象範囲の全数調査は**再調査対象として残す（低・次束候補）**。

### 残点4 — [PORT] Controls 表記差の A/B 選択 【未解消・承認ゲート】

- 後続の `Controls` ヒットは全て UIMenu id ルータ系（`v264` L135 メニュー ID 1021=ControlsMenu／`v268` L100/116・`v272` L26 `ControlsMenu 0xA69FF4+04 = 0x5879A0` 索引演算形 id 1..0xC→+0x28..+0x54／`v274` L34 0x3FD ControlsMenu 0xE8）で、**v223 項3-4 の「表記差 A/B 選択」とは無関係の別話題**。
- v223 L129「[PORT] 側の A/B 選択のみ未決（項6 残点として記録）」の状態が維持。判断材料整備済みのまま、選択の実施記録なし。
- **判定: 未解消。製品判断は [PORT] 側＝承認ゲート性質（台帳どおり）→ #20（V168 台帳/GT 反映）/#46（[PORT] L2 承認）系ゲートに統合して管理する。再調査不要。**

### 残点5 — fAlpha 既定値訂正（0x00000000 → 0.5f / 0x3F000000）の適用 【未解消・適用待ち】

- needle `fAlpha` を `v2[6-8]*.md` に投射 → **0 件**。後続で訂正内容に言及したレポートなし。
- 訂正値は v218 補遺2ドラフト側に保持済み。適用先 `copilot_ini_registries.txt` は残点2 と同一で本環境不在。
- **判定: 未解消。適用待ち（再調査不要・残点2 と同時に処理）。**

### 残点6 — v218 §2-1 wording 精緻化 【未解消・docs 反映指示待ち】

- 「node は実行時生成（静的 node 無し）」→「node VA は固定・.bss 領域（file 非 backed）」への書き換え候補。後続レポートに反映記録なし（docs は L1 で編集禁止・指示待ちのドラフト扱いのまま）。
- **判定: 未解消。docs/GT 反映指示待ち（再調査不要・承認ゲートへ）。**

---

## 3. クローズ可否の結論

- **#44（V219 残点確認）はクローズ不可**: 6 件中 完全解消 0 件。
- ただし「未解消」の内訳は再調査要否で切り分けるべき:
  - **再調査を要する（実質 RE 残点）: 1 件** — 残点3 の対象範囲（`0xA1B480` 帯全数）
  - **待ち条件付きで再調査不要: 5 件** — GECK 入手（1）／registries.txt 入手（2）／[PORT] 承認（4）／docs 適用（5・6）
- 推奨アクション:
  1. 残点3 の対象範囲調査を**新束**（次 serial）として起票（優先度 低）
  2. 残点4・5・6 は **#20（V168 台帳/GT 反映第2）・#46（[PORT] L2 承認）へ統合**と明記
  3. 残点1・2 は「GECK / `copilot_ini_registries.txt` 到着時」の恒久待ちとして V219 行に注記
  4. 上記反映後に V219（row219）を**「調査系のみ残・残りはゲート/待ち」として段階クローズ**するか、完全クローズするかは人間判断

---

## 4. 検証情報

- selftest: **4/4 PASS**（`tmp/re/handover_20261025/scripts/probe_harness.py selftest`、`exit=0`、report → `tmp/re/handover_20261025/scripts/probe_last_report.txt`）
  - 注: 慣習の `%TEMP%\opencode\probe_harness.py` は**本体不在**（`%TEMP%\opencode` 655 ファイルに `probe_harness.py` なし・直近記録 `probe_last_report.txt` のみ残存）→ リポジトリ内 `handover_20261025/scripts/` の同一物で代替。この点はハーネス運用の注意事項として要記録。
- needle 一覧（すべて `tmp/re/v2[6-8]*.md`、v264/v275 は補助参照）: `0xA1B480|0xB07CFC` → 5 件（v264 L134/136/160・v275 L13/53）／`fAlpha` → 0／`registries` → 0／`ShockBolt|Xenon|Absorb|閾値` → 8 件（全件ロック強度系）／`Controls` → 7 件（全件 UIMenu id ルータ系）／`0xA1B480`（tmp/re 全体）→ v223 L185/191/203 と台帳 row219・.bak 群のみ
- 読取のみ（無編集）: `v223_ini_docs_bundle4.md` / `v264_bundle_residuals.md` / `v275_ui_residuals2.md` / `v265` `v268` `v272` `v274` `v282` `v283` / `TODO_re49.md`（row219 確認）
- ESM 本体・git・docs・GT・plan: **未接触**（禁止遵守）
- 採番: 本ファイルは v293（v290=Copilot bundle v2 起案・v291=データ関係性・v292=V220 残点確認と並行割当。v294 以降は本ラウンドでは不使用）
