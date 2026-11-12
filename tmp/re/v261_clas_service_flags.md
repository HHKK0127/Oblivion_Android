# V261: CLAS（TESClass）サービスフラグ表と原版商人判定経路（TODO 257・依頼3）

- 日付: 2026-11-03
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam）
- 依頼: 「商人判定の原版準拠」— ①CLAS のサービスフラグビットのオフセット/意味 ②原版の商人判定経路（ビット vs 文字列）③既存 RE レポート番号 ④ポート側 CNTO-Gold ヒューリスティックとの差分
- スクリプト: `%TEMP%\opencode\v261_*.py`（依頼文は tmp/re 希望だが、本セッション規約（スクリプトは TEMP 置き）を優先する。以下注記として本書に記載）
- 証跡: `tmp/re/v261_funclassext.txt`（TESClass 関数群 0x51BE70..0x51C680 の生 disasm）

## 結論サマリ（作業進行に応じて更新）

### 確定
1. **formType 5 = CLAS / TESClass**（既存: v257 §1 name ptr 列 index5='CLAS'；本書で vtable/ReadRecord まで実在確認）。
2. **TESClass クラス構造の確定**（新規）:
   - primary vtable = **0x00A52C6C**（55 slots、COL 0xABDAD4、TD '.?AVTESClass@@'）。出典行: `tmp/re/b07_vtable_index.md` L1493（列は `# |型名|マングル|vtable|slots|off|COL|TD`。行の 3 番目= vtable VA）。
   - components（多重継承の二次 vtable、b13 L438-440 と ReadRecord 実測で相互確認）: **TESFullName @form+0x18**、**TESDescription @form+0x24**、**TESTexture @form+0x2C**。
   - ctor = **FUN_0051C110**（@0x51C13E で `mov dword [esi], 0xa52c6c`、+0x18/+0x24/+0x2c に二次 vptr 0xA52C54/0xA52C3C/0xA52C20 書込）。
3. **ReadRecord = FUN_0051C210（TESClass vtable vt[0x1C]）**。@0x51C22D `call 0x4511C0; cmp eax,5` で formType==5 を確認後、サブレコード 4CC ループ（読取は `FUN_0051C210` 内 @0x51C243 `call 0x4510B0`）:
   - `'DATA'`(0x41544144) @0x51C289: `push 0x34; lea eax,[form+0x38]; call FUN_0046BDA0` → **DATA を form+0x38 へ min(サイズ,0x34) バイト生コピー**（FUN_0046BDA0 @0x46BDE4-0x46BDEF で subrecord 長 `[rec+0x254]` word と 0x34 の min を算出、0x9812C0=memcpy）。
   - `'DESC'` → `lea [form+0x24]` 経由 FUN_0046A390（TESDescription）。
   - `'EDID'` @0x51C299: 0x200 バイト読込 → `call [vfrm+0xD8]`（TESForm editor-ID 汎用）。
   - `'FULL'` → `[form+0x18]` FUN_0046C7A0、`'ICON'` → `[form+0x2C]` FUN_004700E0。
4. **WriteRecord = FUN_0051BE70（TESClass vtable vt[0x24]）**: FULL/DESC/ICON 書き出し後 `push 0x34; lea eax,[form+0x38]; call FUN_0046BBB0` → **DATA = 0x34 バイト固定**（= form+0x38..+0x6B が CLAS DATA の実体、13 dword）。
5. **ctor の既定値（FUN_0051C1B0、TESClass vtable vt[0x14]=「初期化/リセット」）**:
   - `memset(form+0x38, 0, 0x34)`（@0x51C1B6-0x51C1C3）
   - ループ i=0..6: `FUN_00565BE0(i, 2)` の skill id を **[form+0x44 + i*4]** へ（@0x51C1D0-0x51C1EE、范围校验 `[eax-0xC]<=0x14`）→ **クラススキル 7 件が form+0x44..+0x5F（DATA+0xC..+0x27）**
   - `mov dword [form+0x38], 0`（@0x51C1F2）＝**DATA+0x00 属性1 dword = 0**
   - `mov dword [form+0x3C], 1`（@0x51C1FA）＝**DATA+0x04 属性2 dword = 1（既定）**
6. **フィールドアクセサ群（TESClass のもの、直接 call 式）**:
   - FUN_0051C0D0: `if (val<=7) [ecx+0x38]=val`（**属性1** dword セッタ、**値域 0..7 に clamp**）
   - FUN_0051C0E0: `if (val<=7) [ecx+0x3C]=val`（**属性2** dword セッタ、同 clamp）
   - FUN_0051C0F0: `idx=(arg-0xC); if (idx<=0x14) [ecx+idx*4+0x44]=arg`（スキル setter。[ecx+0x44] は form 内 index のはずが **arg が skill id 兼 index** の形。→ 要追試）
   - FUN_0051C090: `[ecx+0xC]`（formID）が global `[0xB37D00]` と一致なら false、否则 **[ecx+0x44..+0x5F] の 7 項目と arg を走査比較**（=「このスキルはこのクラスのスキルか」）
   - FUN_0051BFE0（vt[0xB4]）: マージ経路。`FUN_009832E6(…,'CLAS' 0xB0535C=TESClass name…)` 後 **0xD dword = 0x34 バイト** を src+0x38→dst+0x38 へ rep movsd（DATA 全コピー）
   - FUN_0051BF10（vt[0xB8]）: form+0x38 領域のバイト逐次比較（ソート/差分比较用）
   - FUN_0051C030: `edx=[ecx+0x38]; esi=[ecx+0x3C];` から **両者の 0..7 範囲と 7 スキルの重複無しさ**を調べる検証関数
7. **GMST/4CC の対応（足場、再実測なし）**: CLAS の 4CC='CLAS'=formType 5（v257）、CreateForm 汎用窓=0x44E578（byte[ft-1+0x44E6DC]=2→jmp 0x44E578: `FUN_00447DF0`→`FUN_00447050`→`FUN_0044D950`）※ v257 §(AG) と自己整合。

### 部分確定
- **byte[form+0x60]（DATA+0x28）bit0 = 「Autocalcs（自動計算系フラグ）」**と推定（bit1 との対比でゲート対象が成長/クラス生成系のみ、名前の確定なし）。根拠: `FUN_0051BEC0` 呼び 7 箇所すべてが (a) レコードロード窓 `FUN_0044CDF0`@0x44D15B-0x44D168（formType==5 or 0x35==WRLD でないと call、false→0x44D30F へ回して後処理スキップ）、(b) `FUN_00505810`@0x505824（`!bit0(現クラス)→ FUN_00662750()` の PC 元クラスへ差替）、(c) `FUN_00596BC0`@0x596BF6（クラス一覧 [0xB33A98]+0x54 を walk して bit0 持ちを count、GMST id 0xFAA/0xFAB/0xFCB 併用＝クラス選択 UI）、(d) クラス staging `FUN_00597340/73F0/7651`、(e) `FUN_005DBBD0`@0x5DC138。**取引系の消費者は 0 件**。
- `form+0x40`（DATA+0x08）・`form+0x64`（DATA+0x2C）: クラス関数群（0x51BE70..0x51C680）に**アクセサ無し**・GetClass 呼び出し元 38 箇所の直後に +0x40/+0x38/+0x3C 直接読みもゼロ（v261_25.py 実走、`v261_getclass_fieldscan.txt` 0 hits）→ **EXE の実行時コードから未読**（ファイル互換のための保留領域の可能性大）。[意味未確定]
- `FUN_0051C090`（isClassSkill）の消費者 8 箇所（0x50D1AC/0x5DA1EC/0x5DA85C/0x5DC6AA/0x5DC7EC/0x663CE1/0x666C83/0x6681DC）— スクリプト opcode・会話/成長 UI と推定、個別意味は未追試。
- `FUN_0051C030`＝TESClass::IsValid（読み込み時検証。失敗で 0x51C31C→エラー窓）。

### 未達（試した地址と否定根拠付き）
1. **CLAS 内の商人/サービス提供ビット（buy/sell/training/repair/spellPurchase）は存在しない** — 依頼 1 の想定自体が否定された:
   - CLAS のサブ記録は `EDID/FULL/DESC/ICON/DATA` の 5 種のみ（ReadRecord `FUN_0051C210` の 4CC 分岐 = 0x44494445/0x43534544/0x41544144/0x4C4C5546/0x4E4F4349、v261_4.py 実デコード。他に cmp 値なし）。
   - DATA 0x34 バイトの既知フィールドにサービス語は無し（上記表）。`byte[form+0x60]` の実測アクセスは bit0/bit1 のみ（アクセサ 0x51BEC0/0x51BEF0/0x51BED0 が capstone 全窓 0x51BE70-0x51C680 内で唯一の +0x60 参照、`v261_funclassext.txt`）。
   - barter/services 側の消費者（`FUN_005E6C60` wrapper・`FUN_004F7B90`）は class+0x60 ではなく**_guard_ を読む**（下記 §裁定）。
2. **原版に「クラス名文字列照合で商人判定」の経路なし** — .rdata/.data 全域の strings スキャンで 'merchant' 系は GMST 名（sMerchantNoGold/iMerchantRespawnDay1/2/sHolidayMerchantsFestival）とホリデー名のみ（v261_12.py・v261_strings.txt）。GMST 名の xref は `FUN_009E62B0`@0x9EE133/0x9EE153 の**名前列**のみで照合コードなし。商人系の論理名文字列比較は .text に見つからない。
3. **`word[TESNPC+0x30]`（barterGold）のファイル側サブ記録名前の最終確定** — ReadRecord の ACBS 分岐 @0x52836D-0x528385 が `push 0x10; lea edx,[esi+0x28]` で**ACBS を NPC+0x28..0x37 へ生コピー**するまで確認（= barterGold は ACBS+0x8 word と整合）。だが UESP NPC_ ページ（仕様裏取り元）が HTTP 503 で 3 回取得不可（uefi 誤 URL 403 も 1 回、記録）。EXE 側の読取 2 箇所（0x5FAA99/0x50CC61）は確定、**ファイル内 ACBS+0x8=gold の命名は [未裏取り]**。
4. INFO 側「OfferServices」相当の**書手**（[dialogue obj+0x1C] bit0 の set 元、= 会話ノード命令の実装）は未割出（命令テーブル探索未実施）。読取側 `FUN_005E8870` は確定済み。
5. GMST fMerchantRespawn/iMerchantRespawn の消費経路（レスポーン日数選択）は未追跡（本依頼のスコープ外）。

## 裁定: 原版「この NPC は商人か」判定経路（依頼 2）

**結論: ビットマスクではない。クラス名文字列照合でもない。原版に「商人判定ビット」は存在しない。** 取引可否は次の 3 層の実装で決まる（全層 EXE 実測）:
1. **会話テーマ（TOPC/INFO データ）駆動**: 「Let's trade」等のテーマ・結果コマンドがメニューを開く。エンジン側に class bit ゲートなし。
2. **実行時サービス状態**: `GetOffersServicesNow`（opcodetable idx 418/opid 0x10FF、impl `FUN_00505C60`→`FUN_004F5A00`）= `Actor::IsActor(vt[0x190]) && [actor+0x44 dialog 経由 form 0x1F subsystem の [obj+0xC]+0x1C bit0]`（`FUN_005E8870`@0x5E8870-0x5E8887: `add ecx,0x44; call 0x41FB20(push 0x1F→0x41E210); test byte[eax+0x1C],1`）。= 直前の会話でサービス提案フラグが立っているか。
3. **財布額**: `FUN_005FAA70`（GetBarterGold 実装本体、@0x4F5A80 経由 opid 0x10F8）: `word[base+0x30] + player 効果分(0x41E980@actor+0x44, 0x5F1910(0x1D)=fame/infamy 系) + (0x56A300()==4 → +[0xB375E8])`。**CNTO の Gold 合算ではない**。

参考（クラス系判定が欲しい場合の原版実在手段）:
- `GetIsClass`(opid 0x1044、impl `FUN_005032D0`) / `GetIsClassDefault`(0x10E4、`FUN_00506D90` 窓の `FUN_004F87F0`）= **クラス formID 等値比較**（b22 §4.3 と同一機構、文字列照合ではない）。
- `word[NPC+0x30]>0` だけで「買い取り可能」の近似にはなるが、原版 UI のテーマ可否は会話データ側の条件（GetIsClass 等）で、エンジンにMerchant-bitはない。
- guard 判定だけクラスビット実在: `Actor::IsGuard` 風 wrapper `FUN_005E6C60`@0x5E6CB3 = `tail call 0x51BEF0(class)` = **byte[CLAS+0x60] bit1**（スクリプト `Is Guard >> %0.2f` 実装 `FUN_004F7B90`@0x4F7BAE-0x4F7BD9: base formType==0x23(NPC)→[base+0x104]→0x51BEF0）。wrapper 消費者は 0x4B72C0/0x4CABC0/0x524510×4/0x5FCAB0 等＝犯罪・_guard_ 系。

## CLAS DATA（form+0x38..0x6B、0x34 バイト）フィールド表（実装粒度）

| form VA | DATA off | 幅 | 内容 | アクセサ（確定 VA） | 根拠 |
|---|---|---|---|---|---|
| +0x38 | 0x00 | dword | 属性 1（0..7。8=Attr enum index） | getter 0x51C030 経由／setter `FUN_0051C0D0`（≤7 guard） | ctor[0x38]=0@0x51C1F2; IsValid@0x51C041 `cmp edx,7` |
| +0x3C | 0x04 | dword | 属性 2（0..7、属性 1 と非同一必須） | setter `FUN_0051C0E0`（≤7 guard） | ctor 既定 1@0x51C1FA; 検証 0x51C038 `cmp edx,esi`（同一で無効） |
| +0x40 | 0x08 | dword | [未確定] 実行時コードから未読 | なし | フィールド表スキャン 0 hits（v261_25） |
| +0x44 | 0x0C | 7×dword | クラススキル 7 個（skill id 0x0C..0x20） | getter `FUN_0051BF00`(idx) / setter `FUN_0051C0F0`（id-0xC≤0x14）／ membership `FUN_0051C090` | ctor ループ@0x51C1D0-0x51C1EE; 重複検証 0x51C052-0x51C086 |
| +0x60 | 0x28 | byte bit0 | Autocalcs 系フラグ（[部分確定]） | `FUN_0051BEC0` get / `FUN_0051BED0` set | 消費者 7 箇所すべて成長/ロード系（上表） |
| +0x60 | 0x28 | byte bit1 | **Guard クラスフラグ** | `FUN_0051BEF0` get（shift>>1&1） | IsGuard 実装 0x4F7BD0 / wrapper 0x5E6CB3（文字列 0xA49334 'Is Guard >> %0.2f'） |
| +0x64 | 0x2C | dword | [未確定] 実行時コードから未読 | なし | 同上 |
| +0x68 | 0x30 | byte | スキル index（+0x0C で skill id 化＝trainer/得意スキル候補） | `FUN_0051BEB0`（`movzx[ecx+0x68]; add eax,0xC`） | 唯一消費者 0x522726（FUN_005222D0＝スキル成長倍率計算、0x780F70/0x422DE0 と skill 照合） |
| +0x6C | 0x34 | (外) | 0x34 範囲外 | — | copy/write 上限 0x34（0x51C289 push 0x34・0x51BE95 push 0x34） |

- On-disk の CLAS DATA サブ記録長 = min(実長, 0x34) で読み込み（`FUN_0046BDA0`@0x46BDB3 `[rec+0x254]` word と 0x34 の min、0x46BDF8 memcpy 0x9812C0）。48/52 バイト両長が存在するとの仕様記述（UESP ページ要旨、検索スニペットのみ取得・本文 503 で未読）＝**EXE 上限 0x34=52 と自己整合**、48 の方は末尾 4 バイト（+0x68 のスキル index 系）欠落と推定 **[未検証]**。
- クラス ptr の取得経路: **`[TESNPC+0x104]`**（CNAM→form 参照、ReadRecord@0x5280B4-0x5280C5）／ランタイム共通 getter **`FUN_005E3320`**（`Actor::IsActor→vt[0x170]→[base+0x104]`、38 箇所 call）。
- NPC 側 barter 金: **`word[TESNPC+0x30]`**（`FUN_005FAA70`@0x5FAA99・Set/debug `FUN_0050CBC0`@0x50CC61）。ACBS サブ記録は `FUN_00527E40`@0x52836D で 0x10 バイトを NPC+0x28 へ生コピー（=+0x30 は ACBS+0x8 と位置整合）。

## ポート側ヒューリスティックとの差分（依頼 4）

実在箇所（読取のみで確認）: `app/src/main/cpp/game/merchant_factory.cpp`
- `npcIsMerchant()` @L31-38: `esm.findNPC(npcFormID)` の CNTO 走査で `itemFormID==kGoldFormID && count>0` → true。
- `buildMerchantFromNpc()` @L40-65: gold=CNTO Gold 合算、>0 なら setGold。
- （クラス側）`esm_reader.h` `ClassData` L712-722 に `specialization/primaryAttribute1,2/majorSkills[7]/flags` があるが、**EXE 実測の CLAS に PRAT サブ記録は存在せず**（EDID/FULL/DESC/ICON/DATA のみ）— 現ポートの CLAS フィールド名/出典は要検証。

原版との誤差クラス:
1. **誤陽性（最大の乖離）**: 強盗・山賊・マラカー・一般市民の財布 Gold（CNTO に 0x0000000F を持つ多数の NPC）→ 原版は会話テーマが無いので barter を提供しない。ポートは商人化してしまう。
2. **誤陰性**: `word[NPC+0x30]`(ACBS gold)>0 かつテーマ有りの NPC で CNTO に Gold が無い（買い取り用 purse はランタイムで加算されるため、authored purse と ACBS gold は別物）→原版は買い取り可、ポートは non-merchant。
3. **額面の誤り**: ポートの merchant gold=CNTO gold 合計。原版 GetBarterGold=`word[+0x30] + fame/infamy 系(0x5F1910@0x1D) + player 効果(0x41E980) + 曜日==4(+0xB375E8)`。** disposition/fame で動く原版値と別系統**。
4. 原版の「サービス提案中」判定（`GetOffersServicesNow`＝[actor+0x44 subsystem form の +0x1C bit0]）はランタイム状態。ポートの静的 CNTO 判定に対応概念なし（メニュー開閉のゲート差に出る）。

### [PORT] 修正案（承認制ドラフト・コード未編集）
1. `npcIsMerchant` を **`esm ACBS gold(word)>0` の二次条件＋会話テーマ存在チェック**へ置換候補: `ClassData.flags` の商人 bit 使用は不可（原版に bit が無いと確定済み。CLAS は属性/スキル/autocalc/guard のみ）。
2. NPC_ パースに **ACBS 0x10 バイト（NPC+0x28 写像、+0x8 word=barterGold 候補）** を追加（`NPCData` に `uint16_t barterGold`）。ACBS+0x8 の命名は UESP ページ本文で最終裏取りすること（本 RE で 503 により未読・[未確定]扱い）。
3. 会話テーマ（INFO/TOPC）がポート済みなら「Let's trade」系テーマの有無を merchant 性の第一条件に。未パースなら段階移行: 当面 `barterGold>0 && (className/EditorID に依存しない)` で近似し、誤陽性（強盗系）は「テーマ or faction 条件」で絞る設計が原版に近い。
4. `ClassData.majorSkills[7]` の型/出典を本書フィールド表（DATA+0x0C 7×dword、skill id 0xC..0x20）に整合させる（PRAT ではなく **DATA サブ記録**からの派生である旨のコメント更新が必要。コードは編集しない）。

## 既存 RE レポート番号（依頼 3）
- **v257**（formType 5=CLAS・CreateForm dispatch・GMST 基盤）— 本書の足場。
- **b07**（TESClass vtable 0xA52C6C・TD/COL）、**b13**（二次 vtable: TESFullName/TESDescription/TESTexture disp）、**b14**（TESForm→TESForm TESClass スロット対応）、**b22 §4.3**（GetIsClassDefault の 0x662750/FormType==5 照合）、**v236/v237**（ReadRecord 一般形・DATA 生コピー）、**v185/v189**（opcodetable）、**v259**（TD+8 inline 名）。
- 本タスク新規: **v261**（本書）。

## 証跡
- `tmp/re/v261_clas_service_flags.md`（本書）
- `tmp/re/v261_funclassext.txt`（TESClass 関数群 0x51BE70-0x51C680 disasm）
- `tmp/re/v261_4f5730.txt`（script 関数群窓）
- `tmp/re/v261_59a400.txt`（barter gate 候補窓）
- `tmp/re/v261_60f250.txt`（IsGuard/skill UI 窓）
- `tmp/re/v261_5222d0.txt`（スキル倍率計算窓）
- `tmp/re/v261_readsub.txt`（FUN_0046BDA0 生 reader）
- `tmp/re/v261_opc_offers.txt`（GetOffersServicesNow impl）
- `tmp/re/v261_getclass_fieldscan.txt`（GetClass 38 call sites の直後フィールドスキャン＝0 hits）
- `%TEMP%\opencode\v261_npc_read.txt`（TESNPC ReadRecord 全文）
- `%TEMP%\opencode\v261_strings.txt`（.rdata商人系 strings スキャン）
- スクリプト: `%TEMP%\opencode\v261_*.py`（依頼文は tmp/re 希望だったが本セッション規約（スクリプト=TEMP）を優先）
- 仕様参照の出典: UESP TES4 CLAS ページ検索スニペット（DATA=48/52 バイト）のみ取得。本文裏取りは HTTP 503 で**未完了**＝EXE 実測 0x34 上限のみが確定。TES4 NPC_ ACBS=0x10 は EXE 実測（0x528378 push 0x10）で確定、gold フィールド名のみ未裏取り。OpenTES4/xOBSE/COEF は参照せず。
