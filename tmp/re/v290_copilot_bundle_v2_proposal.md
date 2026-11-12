# RE 依頼バンドル v2（提案）— 会話テーマ→商人判定の具象表 / GetBarterGold 詳細 — 2026-11-05

- 起案: Opencode「ReverseEngineering」セッション（ai-todo #42）
- 宛先: Copilot session 9427ac74（Oblivion_Android 開発ループ）
- 前バンドル: `tmp/re/handoff_20261103_re_request_bundle.md`（依頼3 CLAS／依頼4 価格式＝**完走済**）・`handoff_20261105_v60v76_residuals_bundle.md`・`handoff_20261105_v93_residuals_bundle.md`
- 対象バイナリ: Oblivion.exe v1.2.0416 (Steam) / Ghidra 12.1.4（total_functions=21,240）
- **提案成果物番号: 依頼1 = `tmp/re/v294_*.md` / 依頼2 = `tmp/re/v295_*.md`**（v290〜v293 は本セッションで使用済みのため空ける）

---

## 本バンドルの性質

前バンドル（依頼3/4）で商人システムの「判定骨格」と「価格式」は確定した。
本 v2 はその**残った2領域** = ①会話データ（TOPC/INFO）から商人メニューへ至る**具象配線**、
②GetBarterGold に含まれる **fame/infamy と週日リセット** を対象とする。
いずれも port の `merchant_factory` / `barter` を原版準拠へ寄せるために**最後に不足している情報**。

### 参考: 既確定（再依頼しない）

| 項目 | 確定内容 | 出典 |
|---|---|---|
| CLAS サービス/商人ビット | **不存在で確定**（サブレコード EDID/FULL/DESC/ICON/DATA のみ） | `v261_clas_service_flags.md` |
| 商人判定の骨格 | ①会話テーマ(TOPC/INFO・データ駆動) ②`GetOffersServicesNow`（opid `0x10FF`=FUN_00505C60=[actor+0x44 subsystem obj+0x1C bit0]）③財布 `word[TESNPC+0x30]`（ACBS+0x8・NPC ReadRecord@0x52836D） | v261 §裁定 |
| 価格式 | sk=trunc(clamp(Luck+Mercantile×0.4−20))、売/買式、ceilf、±haggle、Disposition ±1.5%/10pt | `v262_barter_pricing.md`・`v279_barter_formula_ledger_corrections.md` |
| GetBarterGold の型 | opid **`0x1108`**（index 427・pCnt=0）、impl `0x505D10`、cond `0x4F5A80`、本体 `FUN_005FAA70`、負 purse ガード `[0xA2FC78]`、売価 `min(price, GetBarterGold)` @0x48916F | `b06_field20_24.md`・`s57_*`・v262/v277 |
| CLAS/GetIsClass/'merchant' 文字列 | GetIsClass は formID 等値比較のみ、'merchant' 系は GMST 名のみ（.text 全域否定） | v261 |

---

## 依頼 1 [優先度 P1] NEW: 会話テーマ→商人判定の具象表（TOPC/INFO 経路）

**現状（v261 でここまで確定）**: 「原版の商人判定はビットマスクでもクラス名照合でもなく、**会話テーマ(TOPC/INFO・データ駆動)＋3層ランタイム判定**」。
つまり port の `npcIsMerchant()`（CNTO Gold ヒューリスティック・`merchant_factory.cpp` L31-38）を置き換えるには、**どの会話テーマが barter サービスを供給し、それをどう検出してメニューを開くか**の配線図が必要だが、ここが未解析。

**未確定:**
1. **TOPC レコードの構造とサービス種別**: TOPC（topic）レコード内に「このテーマは取引/訓練/修理/術を供給する」区分がどこに持たれているか（サブレコード fourcc・flags・該当フィールドのオフセットと値域）。EXE 側の TOPC ローダ（ReadRecord 系）の分岐と併せて。
2. **テーマ→メニュー起動の対応表**: 会話 UI（DialogMenu／会話タブのアイコン）が選択されたテーマから、barter メニュー（`BarterMenu`・メニュー ID 体系 0x3E9..0x41B の該当 ID）を起動する分岐の具象表。**アドレス＋テーブル＋全ケース**（列挙形式で）。
3. **TOPC↔INFO の関係**: INFO（会話行）側から取引へ到達する経路の有無（result script 外の、会話ヘッダのサービス表示 → メニュー起動）。DIAL/INFO のどのフィールドがそれに使われるか。
4. **最小判別条件の合成**: 「この actor は barter を開けるか」を1つの述語にまとめるために必要な全条件の AND/OR 構造（`GetOffersServicesNow` bit0 ＋ テーマ有無 ＋ 財布 ＋ 近接距離・会話中フラグ等、UI 側ゲートを含む）。

**知りたいこと**: 1〜4 を **確定/部分確定** で。2 と 4 は**表（アドレス→条件→帰結）形式**。port の `npcIsMerchant()` を差し替える実装材料として、原版と1対1で対応できる粒度で。

**エビデンス形式**: 各項目に (a) 関数アドレス・(b) 該当命令の逐語または decompile 引用・(c) 参照カウント。TOPC の ESM 実データ確認が要る場合は Copilot 側で `Oblivion3.esm` を読む（RE セッション側は ESM 禁止のため）。

---

## 依頼 2 [優先度 P2] NEW: GetBarterGold の fame/infamy と週日リセット詳細

**現状**: `FUN_005FAA70` の骨格式 = `word[base+0x30] + player 効果分(0x41E980@actor+0x44) + (0x56A300()==4 → +[0xB375E8])` まで確定（v261/v262/v277）。ただし**内訳の第2項（fame/infamy 系）と「週日リセット」の中身は未解析**。

**未確定:**
1. **fame/infamy の本体と読み出し**: `0x5F1910`（v261 は「引数 0x1D=fame/infamy 系」と記載）の**実引数と返り値の実体**。fame/infamy はどのオブジェクトのどのフィールドに保存されているか（TESNPC ACBS 内か、別構造か）。
   - **要注意（再検証事項）**: `0x1D` は v279/v280 で **Mercantile AV** として確定済み。v261 の「0x5F1910(0x1D)=fame/infamy 系」は**矛盾の可能性が高い**ので、0x5F1910 の引数の正体（AV index か別 enum か）と、GetBarterGold での第2項の合算式（符号・係数・クランプ）を命令レベルで確定させたい。
2. **関連 opcode 5件の実装**: `0x10F8 modpcfame` / `0x10F9 getpcfame` / `0x10FB getpcinfamy` / `0x10FC setpcfame` / `0x10FD setpcinfamy`（`plan.md` L664/L718-719 の opcode 一覧）の impl VA・本体関数・読み書き先フィールドの一覧。GetBarterGold と同じ storage を叩くのかの照合。
3. **週日リセット機構**: UESP 仕様上、犯罪系（infamy/crimeGold）はゲーム内週境界でリセットされる。EXE 側で**そのリセットを行う関数**（毎フレーム/毎時刻更新の比較窓・リセット対象変数・週の判定方法＝ゲームカレンダー or ティック比較）を特定。あわせて **`0x56A300()==4` の意味**（曜日判定か時刻系か）と `[0xB375E8]`（既定 200・`iPerkExtraBarterGoldMaster`）の帰属確定。
4. **GetBarterGold 全体の項別の合成順序**: 上記1〜3 を含めた項の列挙（項・VA・式・丸め/クランプ）を1本の式として最終確定。

**知りたいこと**: 1〜4 は **確定/部分確定** で。1 は「`0x5F1910` の引数の正体」を最優先（v261 と v279/v280 の矛盾解消）。3 は関数アドレスと更新頻度（何が呼んでいるかの callers）まで。

**エビデンス形式**: (a) decompile 逐語・(b) アドレスと参照数・(c) v261/v262/v277 の該当行との突合（一致/相違の明示）。

---

## 訂正候補（本バンドル起案時の照合で判明・反映判断は承認ゲート）

1. **v261 L59 の「GetBarterGold … @0x4F5A80 経由 opid 0x10F8」は誤記**: GetBarterGold の正 opid は **`0x1108`**（index 427）。`0x10F8` は `modpcfame`。`SCRIPT_OPCODE_TABLE` 側（0x1108）は正 → v261 本文に訂正注記が要る（既存本体は無編集）。
2. **v261 L59/L94 の「fame/infamy 系(0x5F1910@0x1D)」** → 依頼2-1 の再検証結果で確定後に注記（v279/v280 の 0x1D=Mercantile 確定と両立させる）。

## 成果物・検証

- 提案ファイル名: `tmp/re/v294_topc_merchant_wiring.md`（依頼1）/ `tmp/re/v295_getbartergold_fame_weekly.md`（依頼2）。番号が埋まっていれば v296 以降へ送ってよい。
- 検証: 着手時に `probe_harness.py selftest` **4/4 PASS**（`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl@0x504560`）。
  - 注: `%TEMP%\opencode\probe_harness.py` は現在本体不在。`tmp/re/handover_20261025/scripts/probe_harness.py` の同一物を使用すること（v292/v293 §4 参照）。
- 優先度: **依頼1 > 依頼2**（port の商人判定差し替えがブロッカーのため）。
- 制約: L1 report-only。docs/GT/plan の編集はしない（訂正反映は Opencode 側の承認ゲート #20 で管理）。
