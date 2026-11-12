# V277: RE↔port ブリッジ地図（serial 270・中）

- 日付: 2026-11-04
- 目的: graphify クエリで「RE 結論と port コードが隣接するが直接エッジゼロ」が確認された5領域の**項目別対応表**（原版VA/式 ↔ port 現況=実装済/暫定/未実装/相違 ↔ 差分 ↔ 要[PORT]判定）をCopilot実装セッションへの引き渡し単位に集約する。
- 契約: port ソース/RE 本文は読取のみ。本ファイルのみが成果物（新規）。実装は行わない（要[PORT]は L2 承認ゲート扱い、before→after 概要のみ記載）。
- 手法: RE 側は `tmp/re/v2xx_*.md` を引用。port 側は `app/src/main/cpp/` を読取（ファイル:行番号付き）。**追加実測3件**（capstone 5.0.7・selftest 4/4 PASS 着手時・スクリプト `%TEMP%\opencode\v277_disasm*.py`）: ① `0x484370` = **ceilf**（MSVC idiom: `fsubrp st(2)` で t−x、`test ah,1`=C0、frac>0 のときのみ +1.0。tmp/re/v262・v264 の「floor FUN_00484370」表記は**誤称**で、`docs/RE_EXE_REPORT.md` L7953/L7960 の訂正が正しい）② `0x548240/0x5481C0` の sqrt 引数は `200 − max(sk,100)`（`fld 100; fcom st(1); test ah,5; jp` で sk>100 側のみ sk を採用する MSVC max idiom。tmp/re/v262 §3 と一致。`docs/RE_EXE_REPORT.md` L10004-10005 の「min(x,100)」は**誤り**）。③ `0x5E10F0/0x5E11A0/0x5E1200` は `push GetAV(7)=Luck` → `push GetAV(0x1D)=Mercantile` → `call 0x547B90`（arg1=Luck, arg2=Merc）→ 合成式 = **Luck + Merc×0.4 − 20**（tmp/re/v262 §5 と一致、RE_EXE_REPORT L10245「skill + 0.4·Luck」は引数対応の誤記）。
- 判定語義: **実装済**=RE 結論と構造一致 / **暫定**=値または入力が仮置き / **未実装**=対応概念なし / **相違**=RE と異なる挙動 / [PORT]優先度 高=価額・挙動に直結、中=既存動作との整合、低=コメント・命名。

## 0. 5領域サマリ表

| # | 領域 | port 現況（要約） | 要[PORT] 件数 | 詳細 |
|---|---|---|---|---|
| 1 | barter 価格式 | 骨格実装済（v262 差替後）だが sqrt の max/min 反転・skill/Luck 役割反転・purse 拒否が実差 | 高3 中4 低2 | §1 |
| 2 | 商人判定 | CNTO Gold ヒューリスティックのまま（v261 指摘から未着手）。条件255は登録のみ・実行時 bridge 無し | 高2 中1 低1 | §2 |
| 3 | object LOD | Phase 68 で .lod パイプライン実装済（パーサ→provider→描画、yaw 非合成=エンジン等価）。残は窓/fade 整合と BSA zlib 規則の検証 | 中1 低1(検証) | §3 |
| 4 | GMST/時間系 | 後勝ち override 機構は同型実装済。fGlobalTimeMultiplier の kEntries 混入・frame-dt 乗算・第二時計は**修正済**。残= P0 6 キー getLive 化・INI→時計供給・GLOB TimeScale | 中2 低4 | §4 |
| 5 | Script VM opcode | native_scda_decoder の名称表は 29/29 網羅・v201 適用済・名前空間分離は正。残= イベント実行時 dispatch 無し・effectType 死分岐(N-1) | 中2 低3 | §5 |

---

## 1. barter 価格式

RE 出典: `tmp/re/v262_barter_pricing.md`（本体式・GMST 12種・消費者）＋ `tmp/re/v264_bundle_residuals.md` 件1/件2/件3（haggle 生成元・stolen 訂正×0.01・回数上限）。
port 出典: `app/src/main/cpp/game/barter.h/.cpp`、`game/merchant.h/.cpp`、`tests/barter_price_tests.cpp`。
注意: v262 の diff 表（§差分表）が記述した「`kProvisionalSellRatio=0.4f`・playerCharisma 入力」現行コードは**既に撤去済み**（merchant.h の現行コメントは barter 関数群へ差し替えを示す）。下表は**現行コード基準**で v262/v264＋v277 実測と照合した。

| 項目 | 原版（RE 確定） | port 現況 | 判定 | 要[PORT] |
|---|---|---|---|---|
| 売乗率式 | `0.01×(sqrt(200−max(sk,100))×fBarterSellMult(5) + fBarterSellBase(−10) + 1.5×((disp−50)/10))` @0x548240。sk は clamp 0..100 のため sqrt 項=**定数10**、既定 0.40+0.015×adj（v262 §3/§5＋v277 実測） | `barterSellCoefficient` = `0.01×(5×sqrt(200−min(sk,100)) −10 + 1.5×adj)` @barter.cpp L43-49。**min と max が反転** → sk=0 で 0.607（既定 0.40 でない） | **相違** | **高**: before `std::min(merchantEffectiveSkill, 100.0f)`(L44) → after `std::max(..., 100.0f)`。barter_price_tests L135-149/L242-273/L275-288 の期待値(0.607/152/400系)は誤式に紐づいているため同時差替（sell@disp50=0.40→value250 売=100、buy=ceil(2.01×250)=503） |
| 買乗率式 | `0.01×(fBarterBuyBase(300) − sqrt(200−max(sk,100))×9.9 − 1.5×adj)` @0x5481C0、既定 2.01−0.015×adj（v262 §4） | `barterBuyCoefficient` L35-41（同型だが min 反転）→ sk=0 で 1.60（>1 前提が <1 に） | **相違** | **高**: 同上（max 化）。買値が基準価超えとなる原版レンジ（~2倍域）が復活することを確認 |
| 合成スキル sk | `clamp(Luck + Mercantile×fActorLuckSkillMult(0.4) + iActorLuckSkillBase(−20), 0, 100)` @0x547B90（0x9828C0 で trunc）。供給: `0x5E11A0`売=`GetAV(7)`/`GetAV(0x1D)` 商人側・`0x5E1200`買同（v262 §5＋v277 実測 arg1=Luck/arg2=Merc） | `barterEffectiveSkill(skill, luck)` = `clamp(skill + 0.4×luck − 20, 0, 100)` @barter.cpp L31-33 → **0.4 が Luck 側に掛かる（役割反転）**。コメント（barter.h L33-35・merchant.h L96）も「skill+0.4·Luck」を RE 原文と主張 | **相違** | **中**: after `clamp(luck + 0.4×skill − 20, 0, 100)` 相当へ引数意味を再配置＋テスト L100-116 差替。数値影響は max() 化後は sqrt 項が常に 10 のため乗率へゼロ（AV修正域のみ）。命名は「Mercantile」（Oblivion の AV 0x1D。port の "Barter-skill" 呼称は Skyrim 由来の誤称） |
| 端数（丸め） | **ceilf** = `FUN_00484370`（0x484370-0x4843AC 実測: trunc→`t−x>0` で +1）。v262/v264 の「floor」表記は誤称（RE_EXE_REPORT L7953-7962 が先に訂正済） | `barterRoundPrice = std::ceil` @barter.cpp L51-53、コメント L43-45 も ceilf/0x484370 | **実装済** | 低: tmp/re 側の誤称は本節冒頭注記で吸収。コード変更不要 |
| 単価丸め位置・回数倍率 | 乗率×base を ceil。数量乗算位置はメニュー/サービス側（v262 差分表「丸め位置が異なる」は [部分確定]） | `tradeGold(unitPrice, count)` = `lround(unit×count)` + 最小1 @barter.cpp L23-29。**lround（四捨五入）＋ min1** | **相違(小)** | **低**: after ceil へ統一・min1 は原版に無し（0 円取引は UI 分岐「worth nothing」。v262 §6/§未達2）。barter.cpp L98/L165 の `<=0 拒否` は 0 円成立との差分として要検討（UI 側の扱い確定後に） |
| haggle 調整 [player+0x11C] | 売 `+0.01×adj` / 買 `−0.01×adj`（adj=[player+0x11C] int、UI スライダー→`[0xB3B410]` 鏡→複写。エンジンに += なし・**40% ハードクランプなし**、fBarterHaggleMax=40 は UI 配付のみ）@0x488E50・v262 §6・v264 件1/件3 | sell `coef + 0.01×haggleOffset`（merchant.cpp L83-84）/ buy `coef − 0.01×haggleOffset`（L67-68）。クランプなし。**一致** | **実装済** | — |
| canHaggle 判定 | `pow((skOther−skSelf+100)/10, fBarterHaggleCurve=1.0) − 0.55×adj + 0.5×((disp−10)/10) ≥ 0` @0x548130（ε=5.23869e-11）（v262 §疑似コード L168-176） | `barterHaggleAccepts` L55-69（int 変換・`≥0`、pow 省略=curve 1.0）。**一致** | **実装済** | 低: 「原版 ε 値」注記のみ |
| stolen 補正 | 売・**tier<1 のみ** `price = floor(price × floor(v/count×100) × 0.01)`（q[0xA3B150]=double 0.01。**×2.0 は誤り**）。-1.0=非盗品 skip、rec 不在=100→無変化 @0x484850（v264 件2 訂正） | `barterApplyStolenDiscount(price, percent)` = `price×percent×0.01`（percent<0 skip）@barter.cpp L71-76。**×0.01 は一致**。ただし**tier<1 ゲート無し**（無条件適用） | **暫定** | **中**: player Mercantile tier（`0x56A300` 境界=GMST fSkillUseThreshold 系）導入まで tier<1 相当前提（低熟練プレイヤー）に限定コメント追加、または tier 入力追加（下表 tier 項目と一括） |
| player tier 補正 | **tier>3 → 買/売とも price=base** @0x488F75–/0x489153–（call 0x5F23B0 idx 0x1D、RE_EXE_REPORT L10247 確定） | なし（base クランプは tiers と独立に常時適用: 買 max(price,base) L70・売 min(price,base) L88 — 常時クランプ自体は RE_EXE_REPORT L10247 の買床/売天井と一致、tier>3 の上書き経路のみ欠落） | **未実装** | **中**: 項目「tier 入力（player Mercantile）→ tier>3 で base 返却」追加。テストは既定 tier0 域で不変 |
| 商人財布上限 | 売価 `min(price, GetBarterGold)`（**取引は成立**、負 purse は実質無制限）@0x488E50 L110456-64（v262 §6）。GetBarterGold=`word[TESNPC+0x30]` +効果 + tier4→+200（v261 §裁定3・v262 §1） | `merchant.getGold() < earned → 取引拒否` @barter.cpp L169-172。purse は shopGold_（既定 5000 または CNTO 合計、merchant.cpp L8・merchant_factory.cpp L48-61） | **相違** | **高**: before 拒否 → after `earned = min(tradeGold(unit), purse)` のクランプ成立。purse 実値は §2 の ACBS 化と連動 |
| GMST 定数 | fBarterDispBase=1.5（消費 VA 0xB37590）/ fBarterBuyBase=300（0xB375A0）/ fBarterBuyMult=9.9（0xB375B0）/ fBarterSellBase=−10（0xB375A8）/ fBarterSellMult=5（0xB375B8）（v262 §1 登録窓＋v277 消費者実測。v262 表の「値 field」列は consumer VA+4 の別 slot を指す — RE 側の表記ゆえ次セッションで統一） | barter.h L24-31: kBarterBuyBase/BuyMult/SellBase/SellMult/`kBarterDispositionMod=1.5` は値一致。**命名誤り**: 1.5 は `fBarterDispBase`（`fBarterDispositionMod` は 0.01・別 slot 0xB375D8、Persuasion 増減用 v262 §1/§2） | **暫定(命名)** | 低: 定数名を fBarterDispBase 相当へリネーム/コメント訂正（値変更なし） |
| Speechcraft/Personality 非関与 | 価格式は AV7(Luck)/AV0x1D(Merc) のみ読、AV32(Speechcraft)/AV6(Personality) 読まない（v262 §8/§9 確定） | 入力=商人 skill/luck/disp（merchant.h L85-107）。charisma 系は撤去済 | **実装済** | — |
| disp 項の除算 | `(disp−50)/10` 符号付き int・ゼロ方向切捨て（imul 0x66666667/sar2） | C++ int 除算（切捨て）@barter.cpp L37/L45・テスト L152-166 | **実装済** | — |
| 商人入力の ESM 配線 | sk/disp は商人 Actor の GetAV/GetDisposition（v262 §5） | Merchant::barterSkill_/barterLuck_/disposition_ デフォルト 0/50/50（merchant.h L105-107）。**merchant_factory.cpp は setBarterSkill/setDisposition を呼ばない**（L45-61 で gold のみ）→ 全商人が既定値 | **未実装(配線)** | **中**: NPCData の AV（Luck/Mercantile/disposition）供給は ESM パーサ側の追加が前提（§2 の ACBS/NPC 拡張と一括）。供給前の既定値は原版「庶民 Luck50・Merc0→sk=30」とも一致せず（乗率へは影響ゼロ＝max 化後） |

### §1 引き渡しメモ
- **修正順序**: ①sqrt 項 max 化（buy/sell 2 箇所）→ ②テスト期待値差替 → ③sk 引数役割（Luck/Merc 0.4）→ ④purse クランプ → ⑤tier/steal ゲート → ⑥命名・コメント。①②を同じセッションで行わないと CI が現行誤式を固定化する。
- `docs/RE_EXE_REPORT.md` の §57.5/§59.5/L10004-10005/L10245 引用コメントが port 源码側にある（barter.h L18-19・merchant.h L57-58・merchant_factory.cpp L10-11・テスト各コメント）。台帳側の誤（min/Luck 係数位置）は docs 編集権限を持つセッションで修正が必要 — 本シリアルでは編集しない。
- fun 範囲メモ: 0x5481C0-0x54823A（買）/ 0x548240-0x5482AE（売）/ 0x547B90-0x547BF4（sk）/ 0x484370-0x4843AC（ceilf）/ 0x5E10F0・0x5E11A0・0x5E1200（ラッパ）。

---

## 2. 商人判定（CLAS サービスフラグ / GetOffersServicesNow / 財布）

RE 出典: `tmp/re/v261_clas_service_flags.md`（§裁定 3 層構造・§ポート側ヒューリスティックとの差分・[PORT]修正案 1-4）。
port 出典: `game/merchant_factory.cpp`、`assets/esm_reader.cpp/.h`、`game/condition_system.cpp/.h`、`dialogue/dialogue_record.h`、`tests/merchant_tests.cpp`。

| 項目 | 原版（RE 確定） | port 現況 | 判定 | 要[PORT] |
|---|---|---|---|---|
| 商人判定源 | **商人ビットは存在しない**（CLAS サブ記録=EDID/FULL/DESC/ICON/DATA のみ、v261 未達1）。可否は ①会話テーマ（TOPC/INFO データ駆動）②実行時サービス状態 ③財布額 の 3 層（v261 §裁定）。クラス名文字列照合も無し（未達2） | `npcIsMerchant` = **CNTO に kGoldFormID>0 があるか**（merchant_factory.cpp L30-38）。v261 執筆時から未変 | **暫定** | **高**: before=CNTO ヒューリスティック → after=「ACBS barterGold>0 二次条件＋会話テーマ存在チェック」段階置換（v261 修正案1/3）。誤陽性（山賊/一般市民の CNTO 財布→商人化）が最大の乖離（v261 誤差1）。呼び出し側 renderer.cpp L77/L9468、テスト merchant_tests.cpp L391-393 が CNTO 挙動固定化＝同時差替 |
| GetOffersServicesNow | opid **0x10FF**（idx 418）、impl `0x505C60`→`0x4F5A00`、読取 `0x5E8870` = IsActor && `[actor+0x44 → subsystem form 0x1F → [obj+0x1C] bit0]`（=直前の会話でサービス提案中。v261 §裁定2） | 条件関数名登録のみ: condition_system.cpp L426（idx 255）・dialogue_record.h L176（enum 255）。評価器は `rt.query(...)` 経由（condition_system.cpp L110）だが ConditionRuntime 実装は**テストのみ**（tests/condition_tests.cpp・ai_package_tests.cpp。本番 bridge は ai_package.h L258「null = pending」）→ランタイムサービスビット状態そのものが無い | **未実装** | **中**: subsystem 0x1F 相当の「提案中」状態ビットを会話システムに導入し、query 255 で返す。INFO 書手側（[dialogue obj+0x1C] bit0 set 元=会話ノード命令）は RE 未割出（v261 未達4）なので実装時は ESM 命令台帳側との併走が必要 |
| 商人財布額 | `GetBarterGold` @`0x5FAA70` = **`word[TESNPC+0x30]`**（ACBS 0x10B を NPC+0x28 へ生コピー、+0x30=ACBS+0x8。0x52836D）＋player 効果分（0x41E980）＋スキルティア4→`+[0xB375E8]`(=200, iPerkExtraBarterGoldMaster)。**CNTO 金ではない**（v261 §裁定3）。取引コミットで word を減らす命令は無く Set/ModBarterGold opcode 経由のみ（v262 未達2） | esm_reader.cpp L578-580: ACBS を読むが**フラグ bit0(isFemale)のみ**、barterGold 非取得。purse=CNTO Gold 合計（merchant_factory.cpp L48-61）または既定 5000（merchant.cpp L8） | **相違** | **高**: ①NPC_ パーサに ACBS+0x8 u16=`barterGold` 追加（v261 修正案2。ファイル内命名は UESP 503 で未裏取り＝[未確定]扱い、EXE 読取 2 箇所 0x5FAA99/0x50CC61 は確定）②buildMerchantFromNpc の gold を ACBS 値基準へ（既定 5000・CNTO 合計は撤去）③効果分/ティア+200 は段階導入（低）。barter.cpp L169-172 の「財布不足で拒否」→ min クランプは **§1 財布上限項目と同一改动** |
| CLAS フィールド | DATA 0x34B のみ:+0x00/+0x04 属性1/2（0..7）・+0x0C 7×dword クラススキル（skill id 0xC..0x20）・+0x28 bit0 autocalc[部分]/bit1 **Guard**・+0x30 スキル index。PRAT サブ記録無し・**商人/サービス bit 無し**（v261 §CLAS DATA 表） | esm_reader.h L712-722 `ClassData`: `specialization/primaryAttribute1,2（コメント「PRAT subrecord」）/majorSkills[7]/flags`。PRAT 出典表記は誤り（v261 L89）。flags のビット意味未定義 | **相違(低)** | **低**: フィールド出典コメントを DATA サブ記録由来へ訂正（v261 修正案4）。`flags` は bit0=autocalc/bit1=guard 相当として扱う（商人 bit は**追加しない**— 原版に存在しないと確定）。Guard bit1 は原版で `Is Guard` スクリプト命令の実装（0x4F7BD0/0x51BEF0）であり port の guard 需要があれば流用可 |
| GetIsClass 系 | `GetIsClass` opid 0x1044（0x5032D0）/`GetIsClassDefault` 0x10E4 = **クラス formID 等値比較**（v261 参考） | NPC CNAM classFormID パース済（esm_reader.cpp L571-575・esm_reader.h L233。解決マップ L3090-3097/L3700）→ 判定材料あり | **実装済(素材)** | —（テーマ条件化時に formID 等値で使う） |

### §2 引き渡しメモ
- 「当面 word[NPC+0x30]>0 だけで買い取り可能近似」は原版文脈では**財布額の意味**であり、商人性の第一条件は会話テーマ（v261 §裁定1）。段階導入でも誤陽性（山賊）は残るため、テーマ/条件パース完了前に npcIsMerchant を ACBS ベースへ切り替えると UI 挙動が崩れるリスクあり — **テーマ判定と一括で**。
- ACBS+0x8 のサブ記録内位置（=barterGold）は EXE 命令レベル確定、**名称の外部裏取りは未完了**（UESP 503）。移植コードのコメントには「EXE 実測ベース」と明記を。

---

## 3. object LOD（.lod / .cmp / 描画合成）

RE 出典: `tmp/re/v255_object_lod_structure.md`（構造・§8 API 仕様案）＋ `tmp/re/v258_object_lod_residuals.md`（.cmp 実体・GroupWidth・fade）＋ `tmp/re/v260_lod_matrix_consumer.md`（合成消費者）＋（XOR 圧縮規則のみ）`tmp/re/v266_object_lod_residuals2.md` 件2。
port 出典: `world/distant_lod/object_lod_rules.h`、`world/distant_lod/original_object_lod_provider.h/.cpp`、`engine/renderer.cpp`、`tests/object_lod_tests.cpp`、`assets/bsa_reader.h/.cpp`。
**注記（現況訂正）**: 本依頼の前提文は「port=land LOD のみ・OriginalLodProvider の failedBlocks_/パス解決のみ流用可」だが、**現行コードでは Phase 68 として object LOD パイプライン（パーサ→provider→描画フック）が実装済み**（renderer.cpp L6149/L6186/L7094）。下表は現行コード基準。

| 項目 | 原版（RE 確定） | port 現況 | 判定 | 要[PORT] |
|---|---|---|---|---|
| .lod レコード形式 | 1 ファイル=1 セル、`u32 count` + count×可変長レコード `{formId u32, n u32, pos f3[n], rot f3[n], scale f3[n]}`（8+28n、ブロック順）パーサ `0x4D16D0`（v255 §1.1/§1.3） | `object_lod::parseObjectLodFile`（object_lod_rules.h L105-162）: 同一形式・kBytesPerInstance=28（L48）・pos/rot/scale ブロック順（L136-138）。`kMaxInstancesPerRecord=512` は v255 §8.1 推奨ガード | **実装済** | — |
| count/EOF 不一致（11%） | エンジンは count 分のみ読む（0x4D177A）。11% のファイルに末尾追記レコード（v255 §1.5）。移植裁定: **EOF まで歩行して採用**（v255 §8.1・RE_EXE_REPORT §54.8 item2） | EOF 歩行＋sanitize ガード（rules L121-131）。不一致はログ（provider L243-249） | **実装済** | — |
| formType/解決 | formId→STAT（formType 0x1E=30 限定・`0x4D1800`）、解決不能レコードはエンジン break、移植は skip 推奨（v255 §4・§8.1） | `esmManager->findStatic(entry.formId)` 解決不能/モデルパス無は**レコード単位 skip**（provider L257-264） | **実装済** | — |
| yaw（rot）扱い | rot=entry+0x04（ファイル2本目 float3）、corpus 全量で slot[2] のみ非ゼロ（45°倍数）＝**yaw=rot[2]**（v258 件4）。**描画側は rot を一切消費しない: 合成 `M = T(pos)·S(scale)` のみ**（consumer `0x4BCEF0`→composer `0x4BAA80` arg2 アクセス 0 回・`0x7B4010`。v260 §4 [確定]） | パースで `zRotation=rot[2]` 抽出（rules L147）、`Instance.yaw` 保持（provider L269）。描画 `renderDistantObjectLod` は **yaw を意図的に無視**（renderer.cpp L7091-7093 に v260 §4 明記）＝エンジン等価。将来 yaw 尊重時は「エンジン非互換拡張」と明記（v260 §移植指針） | **実装済** | —（指針準拠） |
| 描画バインド構造 | エンジンは land LOD と共用基盤（NiTMap `0xB2C33C`・cell→block÷8 `0x7B3BE0`・16B エントリ append `0x812510`→flush `0x802AE0` で TallGrassTri* バッチ再構築、v260 §2-3/v264 件5）。Android 側は 16B {x,y,z,s} のみで設計不変（v264 実装影響2） | 独自 GL インスタンスパス（renderer L7094 以降・interior shader 共用＋Z-up 変換）。land の ActiveBlock とは分離（provider.h L17-20）— v255 §8.2「ActiveBlock 生活周期流用不可」準拠 | **実装済(等価設計)** | — |
| .cmp | =ランドマーク（稀少 STAT）セルのホワイトリスト＋末尾 u32 kind マスク（mgr+0xC8 キー集合/+0xDC マスク、既定 **7**=ctor `0x4F2BDC`・不在=全許可）。v255 §8.2「**ロードしなくてよい**」（描画品質差のみ・機能欠落なし）v258 件1 | 読み込みなし（provider に .cmp 経路なし）＝RE 推奨どおり | **実装済(意図的不実装)** | — |
| BSA 内 zlib 判定 | **`圧縮 = f1.bit30 XOR (アーカイブヘッダ flags bit2)`**（`0x42E0F3..0x42E0FB`・17 BSA 147,629 件 100% 一致、v266 件2 [確定]。v258 件5「無相関」は単一アーカイブ限定で訂正済み。distantlod 供給は両アーカイブ 100% `u32 サイズ+zlib`）。magic `78 9C` 検査は実質等価（v266 移植指針） | C++ 側は外部 Rust `bsa-core` クレートへ委譲（bsa_reader.h L10-15「All … ZLib inflation live on the Rust side」。`entry.compressed = info[i].compressed` L190 の出所が本リポジトリ外）→ **XOR 規則が実装されているか読替で検証不可**。loose/展開供給経由（AssetManager）なら zlib 非関与で問題なし | **要検証** | **低(検証)**: 実 SI Meshes BSA（flags 0x703・bit30 立）から `extractFileDecompressed` で .lod 1 本をパースする host テスト追加で規則実装の有無を確定。未実装なら bsa-core 側（別リポジトリ）に XOR or magic 判定の修正要望を起票 |
| 素材パス | `distantlod/<ws>_<x>_<y>.lod`（BSA folder・`meshes\distantlod` は**誤**、v255 §7）。論理 `DistantLOD\…` | `kLodPrefixes = {"distantlod\\", "distantlod/"}`＋拡張子 fallback（provider L23/L107-115）。ファイル名は WRLD editorID 小文字照合（L104/L128） | **実装済** | — |
| ストリーミング窓・距離 | 要求生成 `0x483D60`: 中心±（uGridDistantCount=25 の窓・rad 簡略で ±2cell 程度＋境界補正 25/15/4/4）、kind{1,2,4}・gateA/B(`0xB09AE8/AF0`)・fFadeDistance(12288)=窓外周**フェード帯**専用（描画側 0x810F90 のみ消費、v255 §6・v258 件3） | `coverageDistance` 基準の矩形走査＋per-update 予算 16 cell＋片軸 64 clamp（provider L27-34/L140-166）。fade 消費なし・settings gate 対応なし | **暫定** | **中**: before=独自 coverage パラメータ → after=uGrid 由来窓（既定 ±2〜4 cell 常駐＋遠方帯）＋video settings（Object LOD スライダー↔gateA/B 相当、映射詳細は RE 側 [未確定]）。fade は描画品質項目として低扱い可 |
| GroupWidth 8 | land LOD NIF の ÷8 グルーピング専用（実消費者 `0x7B3BE0`）。**object LOD パーサ系は参照ゼロ**（v258 件2 [確定]） | object パスで不使用（一致）。land 側 ActiveBlock の ÷8 は別系統で維持 | **実装済(非適用)** | — |

### §3 引き渡しメモ
- v255 §8 の API 仕様案・v260 §4 の合成規則は**実装反映済み**。Copilot への残作業は「BSA 直読みの zlib 規則検証」と「窓/fade の uGrid 整合」のみ。
- graphify が「直接エッジゼロ」と判定した原因は可能性高い: provider/ルールの出典コメントが `docs/RE_EXE_REPORT.md §54.8` を参照し、`tmp/re/v255/v258/v260` を本文引用しても RE 台帳側（§54.8）にしか反映されていないため台帳⇔tmp/re の断絶が残る — §54.8 の再同期（tmp/re 訂正反映: v258 件5→v266 件2 XOR 規則）は台帳編集権限セッションへ。
- v258/v260/v264 の相互訂正系（ctx/NiNode 誤読・aux 意味・36B 未確定）は port 挙動に無影響（Android 側は 16B entries 設計不変・v264 実装影響2）。

---

## 4. GMST/時間系

RE 出典: `tmp/re/v230_relationship_residuals.md`（§1 merge 語義・§3 TimeScale 経路・§4 bare 名照合）＋ `tmp/re/v233_relationship_residuals2.md`（§1 2,304 全数抽出・§3 jni_bridge 3 乖離・§4 第二時計・§5 find_by_key・§7 type8）＋ `tmp/re/v235_port_alchemy_settings_residuals.md`（§2 kEntries 现状・§3 二重登録裁定）。
port 出典: `game/gmst_defaults.h`、`system/settings_manager.h`、`jni_bridge.cpp`、`system/oblivion_ini_defaults.{h,cpp}`、`engine/sky_weather_system.h`、`game/ai_scheduler.{h,cpp}`、`engine/renderer.cpp`、`assets/asset_manager.cpp`。

| 項目 | 原版（RE 確定） | port 現況 | 判定 | 要[PORT] |
|---|---|---|---|---|
| GMST override 後勝ち構造 | `0x4A8040`: lookup（`0x55E000` djb2）命中時のみ `vtable+0x10`→`0x4A7D60` で DATA 4B を同一 Setting へ置換。未命中 je スキップ・insert 無し・キー集合 2,304 不変（v230 §1-2） | `gmst_defaults.h` OverrideCell（atomic release/acquire）+ `getLive()` 後勝ち + `setOverride` 未登録キー黙殺（L127-177 相当）。`publishGmstOverrides()` は loadEsm/parseEsmFromMemory/loadPluginsFromList の 3 系統（asset_manager.cpp:134-146） | **実装済**（同型） | — |
| keyspace 網羅 | エンジン unique **2,304**（float 992 / int 227 / string 1,085・登録ヘルパ 0x41BAE0/0x419A70 全走査、v233 §1-1）。int/string は型付きセル新設前提 | kEntries=**40 float**（fatigue 16+combat 22+speak 2、gmst_defaults.h L44-90。同名同型一致 40/40。網羅率 41/2,304=1.78%→現 40 件、float 層 40/992=4.0%、v233 §1-3） | **暫定(設計判断)** | **中(P0)**: 消費実在 6 キーの getLive 化 — settings_manager.h L31-62 の constexpr（`DIFFICULTY_DAMAGE_MULTIPLIER=5.0`/`AI_MIN_GREETING_DISTANCE=150`/armorPerk Novice 1.5・Journeyman 0.5、slot 0xB37B88/0xB36AD8/0xB374F0-F8/0xB37500-08）→ after=kEntries 追加＋static_assert ガード運用（fatigue/combat 先例）。網羅 getLive 化は非推奨（v233 §1-3 P2） |
| fGlobalTimeMultiplier の正体 | **GameSetting map 不在・INI ノード `fGlobalTimeMultiplier:General`(0xB06704) 専用**。ゲーム時計 `0x47D170`@`0x47D249 fmul` のみで乗算（frame-dt 倍率ではない。TimeScale=index 0x3A 既定 30.0 とは別機構）（v230 §3-1・v233 §1-2/§3-1） | ①kEntries 混入→**削除済**（現 L44-90 に無し。v235 §2「41→40、削除案既適用」）②jni_bridge の frame-dt 乗算→**削除済**（jni_bridge.cpp L254-259、RE 54.3 引用コメントで恒等化） | **実装済(修正反映)** | —（v233 §3 乖離①②は解消済み） |
| INI→ゲーム時計供給 | Oblivion.ini [General] 読→ノード 0xB06704→ゲーム時計のみ有効（v233 §3③） | oblivion_ini_defaults.cpp:444 に `{"General","fGlobalTimeMultiplier","1.0000"}` **登録のみ・消費者ゼロ**（grep: jni_bridge のコメント/宣言/登録除き読出箇所なし） | **未実装** | **中**: before=死に登録 → after=OblivionIni::resolve 経由の読取を SkyWeatherSystem 時計経路（game clock）のみへ接続。frame-dt 側へ掛けないこと（v233 §3-1① の禁止事項が正） |
| TimeScale 二時計 | TESGlobal `TimeScale`（0xB332E0+0x14・既定 30.0・.rdata 0xA37CC8）1 本が増分/待機/コンソールを束ねる（v230 §3-2・v233 §4-2） | 第二時計非接続は**解消済**: コンソール→renderer ラムダで `skyWeatherSystem->setTimeScale` **＋** `aiScheduler->setTimeScale` 二重配送（renderer.cpp L1262-1270、ai_scheduler.cpp L144-162 setter 実装）。ただし既定値はハードコード 30.0 のまま、ESM GLOB 0x3A 解決経路なし | **暫定** | **低**: ESM GLOB `TimeScale`（formID 0x3A・DataHandler 解決）を両時計の供給源に（v230「既定値一致・供給源相違」の残差）。待機 fast-forward（3600/T・FUN_004029E0）は移植側未照合のまま（v230 §3-2 参考行） |
| RegSettings type8 | 読出 `0x4A8D20` は型照合無し（期待型は lpType スロットのデッドストア）。dwType=8・cbData=4 の blob はそのまま往復（v233 §7 — N-3 解明） | Reg バックエンド廃止（cpp 全域 RegQuery ヒット 0）＝v57 の no-op 方針準拠 | **非該当** | — |
| bare 名（':' 無し）4 件 | `fLowPerfCombatantVoiceDistance`/`iDetectionHighNumPicks`/`iLastHDRSetting`/`sBasePath`（v230 §4-1）。sBasePath は Reg 専用ノードで実質死にコード | 3 件登録（oblivion_ini_defaults.cpp:50-52）＋ sBasePath 意図的欠落（v230 §4-2） | **実装済(一致)** | 低: 「bare 名 4 件網羅」表記を作る場合は 3 件と訂正（v230 §4-4） |
| find_by_key 挙動 | エンジン ReadOne の ':' 無し→MAIN 正規化（0x4A8430・v218 項5） | 実装（任意セクション線形走査）＋ヘッダコメントは v233 §5-3 訂正案が**適用済**（oblivion_ini_defaults.h L54-59「Scans every default entry … regardless of section」） | **実装済** | 低: 同コメント首行「three compiled-in nodes」は実挙動（全 657 走査）と依然ずれる — 文面のみ |
| fKnockbackTime | engine float 登録実在・slot 0xB36FD0 | kEntries に登録済だが consumers 0（gmst_defaults.h L75「impulse time, unwired」） | **暫定** | 低: ノックバック impulse 実装時に接続（v233 §1-3 P1） |

### §4 引き渡しメモ
- v230/v233 の diff 表が記録した port 問題のうち **3 件（kEntries 混入・frame-dt 乗算・第二時計）は本作成時点で解消済**。残る実作業は「P0 6 キー getLive 化」「INI→ゲーム時計」「GLOB TimeScale」の 3 点。
- 「type8 で将来 'a' キー追加時に型不一致」という v232 の懸念は**否定済み**（v233 §7-3）。移植側 Reg 廃止と合わせ、この論点を再掲しないこと。

---

## 5. Script VM opcode（29 opid・native SCDA・OnMagicEffectHit）

RE 出典: `tmp/re/v189_esm_ledger_residuals.md` §3（未登録 29 opid 全表・口径整理・Σ17,131=27.09%・0x0016 OnMagicEffectHit 10,480・impl 27/27 実測有効）＋ `tmp/re/v201_opcode_gt_residuals3.md`（native_scda_decoder 名称適用 3 件）＋（MGEF 分類のみ）`tmp/re/v235_port_alchemy_settings_residuals.md` §1。イベント機構の足場: `b11_event_bit_setters.md`/`GT_CHEATSHEET.md`（ScriptEventList 0xA49DA4・投入 0x4FBF90・判定 0x4FB5F0・22 ハンドラ・OnMagicEffectHit mask 0x2000）。
port 出典: `script/script_opcodes.h`、`script/native_scda_decoder.{h,cpp}`、`script/native_scda_vm/bridge`、`script/script_functions.cpp`、`assets/esm_reader.{h,cpp}`（MGEF 部）、`game/alchemy_system.cpp`。

| 項目 | 原版（RE 確定） | port 現況 | 判定 | 要[PORT] |
|---|---|---|---|---|
| 未登録 29 opid 網羅 | v189 §3.2 の 29 opid（0x0016/0x001E/0x1002/0x100D/0x100F/0x1010/0x1013/0x101E/0x1021/0x1022/0x1026/0x1036/0x1037/0x1039/0x105E/0x1071/0x1072/0x1073/0x1075/0x1076/0x1089/0x108B/0x10D9/0x10DA/0x10DB/0x110C/0x114A/0x1154/0x115B）。**532 スロット表・SCRIPT_OPCODE_TABLE.md には全件在籍**、欠落は RE_EXE_REPORT §67.6 付録行のみ（口径整理 §3.3） | native_scda_decoder.cpp 名称表は **29/29 在**（0x1002@553〜0x115B@789 全数実在確認。`0x10DB "ShowBirthsignMenu"` の大小のみ RE 名 "ShowBirthSignMenu" と差）。v201 適用分も健在（0x1083@644/0x10E3@706/0x1132@763） | **実装済** | 低: 表示名の大小統一は任意。**§67.6 付録 29 行の追記は台帳（docs/RE_EXE_REPORT）側の作業**で本契約外 |
| 0x0016/0x001E の二重名前空間 | block-type 帯: 0x0016=OnMagicEffectHit（SCDA 頻度 **10,480**・rank2・impl `0x5045E0`）0x001E=OnReset（191・`0x5045A0`）。0x00xx 帯は「イベント/ブロック」語義（v189 §3.2・§3.3 / TODO_re49 V15/V58 系） | decoder は**意図的に 2 名前空間を分離**（native_scda_decoder.h L108 注記: line-block `If=0x0016/Return=0x001E` とイベント名 `onmagiceffecthit=0x0016/onreset=0x001E`、L854/860） | **実装済** | —（数値重複は RE 側台帳でも同じ。コメントで二重性を明示済） |
| 29 の内訳（runtime 意味） | annotate_db 登録 20（AddItem 等の実命令・impl 実在）/ UNDET15∩29 = 7（0x0016,0x001E,0x10D9,0x10DA,0x10DB,0x1154,0x115B）/ 共有スタブ `0x977C50` = 2（0x110C TrapUpdate, 0x114A AddAchievement — **エンジン側も no-op スタブ**）。impl 27/27 実測有効（v189 §3.2/§3.3） | 名称は全件在籍。実行は native_scda_vm/bridge 側で未実装のものが大半（デコーダは解析/表示目的） | **暫定(段階)** | —（TrapUpdate/AddAchievement はエンジン等価で no-op 可。実装不要の判定根拠として本行を参照） |
| イベント実行機構 | `ScriptEventList`（vft 0xA49DA4・container this+8・8B node{node_id,mask}）/ 投入 `0x4FBF90→0x4FB580`(OR) / 判定 `0x4FB5F0`（未登録は `0x4FB510` 生成して false・22 callers=全イベントハンドラ）/ OnMagicEffectHit マスク **0x2000**（b11 §4・GT_CHEATSHEET L88/130） | script ディレクトリにイベント dispatch なし（grep event/Dispatch ヒットはコメントのみ）。マジックヒット→スクリプトイベント接続なし | **未実装** | **中**: after=事件送出/判定の最小機構（{node_id,mask} u32 ビットマスク・22 ハンドラの定数表は b11 §4 から転記可、移植要点 b11 L174 の (a)(b)(c) 準拠）。OnMagicEffectHit(0x2000) は magic 適用経路（s93 MagicApp 系）との統合で優先度上昇 |
| 合成 VM と native の分離 | （RE 結論は「native SCDA は §67 台帳の 532 スロット命令。合成 VM のスタック符号化とは別物」） | script_opcodes.h L145-152「Do not dispatch these values to ScriptFunctions until…」/ L171-172「FunctionID は合成 call ID、native と混用禁止」で明文化。FunctionID bridging 設計は `tmp/re/functionid_design_spec.md`（StopQuest=0x007A 既存、Evp/PlaySound/ModCrimeGold/SetCrimeGold/KillActor/ResurrectActor 未登録）。script_functions.cpp L1985 に RE_EXE_REPORT §68 由来ハンドラ移植群 | **暫定(設計意図)** | **低**: spec 準拠で FunctionID⇔native 対応の実装選択を確定（数値の取り違え事故防止はコメントで担保済） |
| MGEF 効果分類の供給 | **エンジン効果分類は DATA 由来でなく mgefCode(fourcc) 直書き比較**（.text 全走査 157 箇所・32 関数、v235 §1-2 b-3）。MGEF DATA: +0x00=flags（port 未読取）・+0x10=Resist actorValue（port「flags」誤ラベル）・baseCost/AV/school 位置は一致（v235 §1-2 b-2） | ①spell_manager `mgefToEffectType()` の fourcc 方式＝エンジン等価方向で**一致** ②`MagicEffectData.effectType` は**依然 0 代入**（esm_reader.h:432 default 0、代入箇所なし）→ alchemy_system.cpp:96 の 4/1/2/3 分岐は死にコード（N-1 未修正） ③esm_reader の "flags" フィールド注記=実 DATA+0x10（Resist AV）の誤ラベル（N-4） | **相違** | **中(N-1)**: effectType を fourcc から充填（v235 §1(c) 修正ドラフトあり・再掲せず）または alchemy 分岐を fourcc 照合へ置換。低(N-4): DATA+0x00 読みと flags/ResistAV ラベル訂正 |

### §5 引き渡しメモ
- 「未登録 29 opid」は port 側欠落ではなく **RE 台帳 §67.6 の付録行欠落**が本体（v189 §3.3）。port で残るのは実行側（イベント機構・VM 接続）のみ。
- OnMagicEffectHit 10,480 件（SCDA 頻度 rank2）＝イベント機構は「触发的に多い」ことの根拠。実装優先度の判断材料に。
- 0x0016/0x001E の数値衝突は RE/port 双方で二重名前空間として運用済。台帳 join 時に混同しないこと（v189 §3.2 band 列参照）。

---

## 6. 副産物: RE レポート(tmp/re 直下 353 md)が明示言及する port シンボル一覧（機械抽出）

スクリプト `%TEMP%\opencode\v277_syms.py`（selftest と同一環境）。対象=tmp/re 直下の *.md のみ（.bak/.pre 派生・本成果物を除外）。

| port シンボル/ファイル | 言及数 | 言及ファイル数 | 代表ファイル | 備考 |
|---|---:|---:|---|---|
| gmst_defaults | 45 | 9 | v230/v233/v235/v236/v237, s76_febf0_tail_plan, v208 | 領域4 の主接続点 |
| oblivion_ini(.cpp/.h) | 98 | 15 | v218/v230/v233/v235 他 | INI 既定表系 |
| settings_manager | 39 | 12 | s50_*/v230/v233/v235/v236/v237, v180 | — |
| jni_bridge | 38 | 8 | v230/v233/v235 他 | fGlobalTimeMultiplier 経路 |
| esm_reader | 97 | 18 | v182/v233/v235 他 | MGEF/NPC/CLAS 読取 |
| publishGmstOverrides / getLive | 5 / 28 | 3 / 8 | v230/v233/v235(6) | override 機構名は安定 |
| sky_weather_system / ai_scheduler | 9 / 19 | 3 / 3 | v230/v233 | 時間系 2 時計 |
| script_opcodes | 29 | 15 | b03/s57_*/s69_scda/v201/v206, functionid_design_spec | — |
| script_vm / script_functions / FunctionID | 42 / 21 / 48 | 19 / 13 / 15 | s57_*/functionid_design_spec/v179/v181 | 合成 VM 系 |
| native_scda_decoder / native_scda_vm | 10 / 5 | 6 / 4 | **v201**, functionid_design_spec, s69_scda, v176 | 名前適用= v201 |
| mgefToEffectType / alchemy_system | 18 / 19 | 5 / 6 | v229/v230/v233/v235/v236/v237 | N-1 接続 |
| object_lod | 25 | 10 | v255/v258/v263/v266/v269, handoff_20261102 | 領域3 |
| OriginalObjectLodProvider / OriginalLodProvider / failedBlocks_ | 2 / 6 / 5 | 2 / 4 / 3 | v255, handoff_20261102 | — |
| **original_object_lod_provider（ファイル名）** | **0** | **0** | — | tmp/re に無言及（§3 の現況と graphify「エッジゼロ」の差の一要因） |
| **barter.h / condition_system（ファイル名）** | **0** | **0** | — | RE 側は opid/VA/式名で参照し port ファイル名を書かない |
| barter.cpp / merchant.cpp / merchant.h | 6 / 3 / 4 | 3 / 1 / 1 | v262, TODO_re49, handoff_20261103 | 領域1 |
| kProvisionalSellRatio | 8 | 3 | v262, TODO_re49, handoff_20261103_re_request_bundle | **port では撤去済**。RE 依頼文/台帳に残存する陳腐化参照（§1 冒頭注記） |
| merchant_factory / npcIsMerchant | 3 / 4 | 2 / 2 | v261, TODO_re49 | 領域2 |
| distant_lod | 3 | 2 | TODO_re49, handoff_20261102 | — |
| renderer.cpp | 13 | 7 | v230/v233, s50_* | — |
| bsa_reader | 2 | 2 | s60_io, v206 | zlib 規則(v266)と port ファイルの明示接続なし |

**機械抽出からの特徴**: (1) RE レポートは port 実装を「opid/VA/式名」で参照することが多く、port のヘッダ/クラス名を書くのは gmst_defaults・native_scda_decoder 等一部の系列に限られる。この命名参照の非対称が「隣接するが直接エッジゼロ」の主要因の一つ。(2) 陳腐化参照（kProvisionalSellRatio 8 件）は graphify の重み付けで誤誘導になり得るため、§1 冒頭の「現行コード基準」注記を接続ラベルとして使う。

---

## 7. 全体まとめ（引き渡し判定）

- **要[PORT] 合計**: 高5（§1: sqrt max 化×2・purse クランプ／§2: 商人判定源・財布 ACBS）＋ 中10（§1×4・§2×1・§3×1・§4×2・§5×2）＋ 低11・検証1（§3 BSA zlib）。§3-§5 に高はなし（object LOD はパイプライン実装済、GMST/Script VM の主要乖離は修正反映済）。
- **修正済みの再タスク化に注意**: v262/v230/v233 が記録した port 問題の一部（kProvisionalSellRatio・playerCharisma 入力・fGlobalTimeMultiplier 混入・frame-dt 乗算・AI 第二時計・find_by_key コメント）は**本作成時点で解消済**。RE diff 表をそのままタスク化すると手戻りになる。各セクション冒頭の「現況訂正」注記を優先。
- **逆方向の陳腐化**: RE 側台帳の誤（docs/RE_EXE_REPORT の min(x,100)・「skill+0.4·Luck」・tmp/re v262/v264 の「floor」表記）は本シリアルでは編集不可。§1 実測裁定（max・Luck+Merc×0.4・ceilf）を台帳編集権限セッションで反映するまで、barter 修正は tmp/re v262＋v277 実測を正とする。
- **同一セッションで束ねるべき改动**: ①§1 の乗率 2 件＋テスト ②§1 purse クランプ＋§2 ACBS barterGold（財布の二つの変更は同一 API）③§5 N-1 effectType は §4 と無関係に単独で可。
- テスト固定化の罠: barter_price_tests（min/ceil 期待値）・merchant_tests L391-393（CNTO 商人）は**現行暫定実装を仕様がごと固定**している。修正時は期待値同時差替が必須（さもないと CI が誤式を保護する）。
