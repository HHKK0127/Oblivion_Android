# v274 — opcode/GT 残点 第17束（6件・すべて低）

- 作成日: 2026-11-04 / 担当: サブエージェント（v272 §9 新規残点 1..6 の逐次解消）
- 対象: Oblivion.exe (v1.2.0416 Steam) 静的解析のみ
- 出典: `tmp/re/v272_opcode_gt_names16.md` §9 残点 1..6
- 確定済み足場（再検証しない）: 8 Menu id 表全回収／sender=GFx AS データ駆動（機構候補 `FUN_00947AC0`）／extra 0x23=ExtraFollower／lock data 0xC 終端／teleport data 0x1C／[0xB33B00]=TESSaveLoadGame／0xB0613C=NiTPointerMap<uint,TESForm*>（bucket 131,213・CRT `FUN_009DBF00`）／actor+0x3C 書込一点 `FUN_004D7D10`／head 従属 obj=BSFaceGenAnimationData
- 環境: capstone 5.0.7 / selftest 4/4 PASS（着手時）/ VA換算: .text 0x401000=file 0x400・.rdata 0xA28000=file 0x627200・.data 0xB02000=file 0x700C00
- 注: 本ファイルは v272 §9 残点 1..6 の逐次解消記録（1 項目完了ごとに即追記）

## 0. 項目一覧
1. GFx AS バインディング名表一点の特定＋menuTypeId 51 の名前化（`FUN_00947AC0` 帯の登録文字列全数回収）
2. extra 0x27（`FUN_0041E7B0`）＝キー連携の実経路候補の確定
3. TESSaveLoadGame 完全 member map＋`FUN_0045A500`/IAT[0xA2808C] の義
4. map entry alloc サイズ（`FUN_004F0F60`）＋dirty 配列（[0xB06144] 隣接）拡張窓
5. Set3D callers 11窓の個別帰属＋byte[+0x1DB/0x1D5/0x1DA]（BSFaceGenAnimationData）名目
6. bit24 代替通路: uesp 再開確認 or tes4Edit.pas（GitHub raw 等）

## 1. 【低1】GFx AS バインディング名表一点＋menuTypeId 51 の名前化 — **[確定]**（名表一点=**`FUN_0057DE50`**（enum 0..0x23→UI イベント名 36 本・jump table 0x57E0C0）。同時に **v272 の機構候補 `FUN_00947AC0` は誤帰属**と訂正＝同帯は Havok VDB コマンド dispatch。menuTypeId 51 名前化=47 クラス名＋4 スロット「Unknown menu class!」）

### 1.1 GFx AS バインディング名表一点（`v274_1k/1l/1m/1n_out.txt`）
- **一点=`FUN_0057DE50`@0x57DE50**（cdecl・arg1=enum）。先頭: `mov eax,[0xB33398]; mov ecx,[eax+0x24]; test ecx; je skip` → `arg1-1` 範囲 0..0x23 → **jump table 0x57E0C0（36 エントリ）** → 各 case `push 1; push 0x121; push <name cstr>; jmp 0x57E09A` → 末尾 0x57E09A: `call FUN_006ADE50(name,0x121,1)`→obj→`vt? FUN_006B7190 / FUN_006B73E0`→`call 0x401F20`（free）。
- **イベント名 36 本全数**（cstr 0xA68EE0..0xA68F8C・enum 昇順）: 0x00 UIMenuOK／0x01 UIMenuCancel／0x02 UIMenuPrevNext／0x03 UIMenuFocus／0x04 UIMenuTabs／0x05 ITMBookPageTurn／0x06 UISpeechRollover／0x07 UISpeechRotate／0x08 UIQuestNew／0x09 UIQuestUpdate／0x0A UIMessage／0x0B MenuEnd／0x0C MenuStart／0x0D UIMenuBracket／0x0E UIMessageFade／0x0F UIInventoryOpen／0x10 UIInventoryClose／0x11 UIPotionCreate／0x12 DRSLocked／0x13 UIMessage（0x0A と同名＝2 enum 共有）／0x14 UIMenuCancel（0x01 と共有）／0x15 UIStatsSkillUp／0x16 SPLEquip／0x17 ITMWelkyndStoneUse／0x18 ITMScrollOpen／0x19 ITMScrollClose／0x1A ITMBookOpen／0x1B ITMBookClose／0x1C ITMTakeAll／0x1D ITMIngredientNothing／0x1E ITMIngredientDown／0x1F ITMSoulTrap／0x20 UIArmorWeaponRepairBreak／0x21 ITMBoundDisappear／0x22 ITMGoldUp／0x23 UIItemEnchant。**ユニーク名 34**（UIMessage・UIMenuCancel が各 2 enum）。
- **発火者=Menu 級コード**: callers **101 窓**（`v274_1m_out.txt` 全列挙・0x4AE-0x5DD 帯＝BookMenu/InventoryMenu/DialogMenu…各メニュー処理函。例 0x5BC450 帯 MessageMenu ctor 隣接・0x5AC5E0 LevelUp 帯）。
- **受け皿経路**（sink・`v274_1n_out.txt`）: FUN_006B73E0=ゲート `[0xB33A98] 非 NULL && byte[[0xB33A98]+0xCD4]==0` → `FUN_0057B050`（条件チェック）／`FUN_0057AF10`／`FUN_0057B0F0`。3 者とも共通形 `call FUN_00582160(1,0)`（**最上段 Menu インスタンス getter** [推定]）→ `[menu+0x1C]/[menu+0x68]` 検査 → `mov ecx,[menu+0x68]; push 0xFAE; call FUN_00588BD0`＝**movie オブジェクト（Menu+0x68）へのイベント送信**（FUN_00588BD0 は 0x587D20 ファクトリ同一帯）。→ C++→AS の発火は「名表一点 FUN_0057DE50 → 最上段 menu の movie[+0x68] 送付」で閉じた。**[確定]（表・窓）／sink 細部（FUN_00588BD0 の GFx 呼び名）は [推定]**。
- 使い分け: FUN_0057B0F0 は `id=0xFAE` 送信（menu+0x68 が無効なら何もしない）、FUN_0057AF10/0057B050 は同形の条件付き経路（`v274_1n` 逐語）。

### 1.2 【訂正】`FUN_00947AC0` 帯 = GFx ではなく **Havok Visual Debugger（VDB）コマンド dispatch**
- 再逐語（`v274_1a_out.txt`）: `FUN_00947AC0`（thiscall・ret 8）= `call FUN_008A7720([this+0x18])`（hkMutex スコープロック）→ `call [[this+0xC] + idx*12 + 4]`（**12B エントリ {key?, fn, arg?} テーブル**・arg2=このidx）→ `call [IAT 0xA28074]`。唯一 caller `FUN_00918AA0`（.rdata 0xA9D228 の **vtbl スロット 57**＝hkvdbApp クラス帯・run 0xA9D144..0xA9D234）。索引検索 `FUN_009186D0`=配列 [mgr+0x38]/count[mgr+0x3C] を walk して `vt[+4]` の返り id 比較＝**hkVDB コマンド id ルックアップ**。
- **帯登録文字列全数**（`v274_1g_out.txt`・0x916000-0x920000 / 0x944000-0x948800 の cstr 参照）: ハンドラ名=`ObjectInspection`@0x94655B／`Statistics`@0x94763B／`DebugDisplay`@0x9478FB＋ビューア登録群 `TtrcConvexVert/TtrcConvTransl/CvxVerts/Verts/CvxTransform/Child/PC/World/WorldMemory/MousePicking/Islands/Tensor/ConvexRadius/ContactPoints/ForcedContactPoints (See Docs For Usage)/Constraint/Broadphase/SweptTransforms/Shapes/CentreOfMass/Phantoms/ShapeDisplayViewerOptions/Tthk*Viewer 10 本/Ttget*・Ttsend*・Ttdraw* コマンド名群＋assert/パス群 '...\visualdebugger\viewer\*.cpp'・'Unable to build display geometry from hkShape geometry data'・**'VDB: Found a command (%x) with no handler. Could corrupt the stream.'@0x948039**。＝すべて Havok SDK（hkp/hkVDB）語彙。**GFx/Scaleform/ActionScript/ExternalInterface/FSCommand/.swf 文字列は画像内に 0 本**（`v274_1h/1i_out.txt`＝.rdata 全域検索）。
- ⇒ v272 D7 の「機構候補 FUN_00947AC0」は**却下**（Havok VDB と判明）。v272 の「retail .text に C++ 側 send 窓なし＝AS データ駆動」結論自体は維持、ただし C++→AS 方向の一点は本件 1.1 の **FUN_0057DE50 名表**として特定済み。AS→C++ 方向は menu 級 +0x04 ルータ（v272 §1.1 の 48 表）が受信側＝**双方向の名表一点が揃った**。

### 1.3 menuTypeId 0x3E9..0x41B（51）名前化（`v274_1e_out2.txt`・jump table 0x5887B8 読取のみ）
- `FUN_00587D20` の `add eax,-0x3E9; cmp eax,0x32; jmp [0x5887B8+idx*4]` 51 slot。slot→(new サイズ, ctor, vtable, RTTI 名)（型=Console::UI メニュークラス・RTTI 実デコード）:
  - 0x3E9 MessageMenu 0x64／0x3EA InventoryMenu 0x58／0x3EB StatsMenu 0xB4／0x3EC HUDMainMenu 0x94／0x3ED HUDInfoMenu 0x5C／0x3EE HUDReticle 0x28／0x3EF LoadingMenu 0x74／0x3F0 ContainerMenu 0x68／0x3F1 DialogMenu 0x98／0x3F2 HUDSubtitleMenu 0x44／0x3F3 GenericMenu 0x38／0x3F4 SleepWaitMenu 0x50／0x3F5 PauseMenu 0x3C／0x3F6 LockPickMenu 0x180／0x3F7 OptionsMenu 0x40／0x3F8 QuantityMenu 0x58／0x3F9 AudioMenu 0x54／0x3FA VideoMenu 0x118／0x3FB VideoDisplayMenu 0x48／0x3FC GameplayMenu 0x4C／0x3FD ControlsMenu 0xE8／0x3FE MagicMenu 0x5C／0x3FF MapMenu 0x100／0x400 MagicPopupMenu 0x60／0x401 NegotiateMenu 0x68／0x402 BookMenu 0x3C／0x403 LevelUpMenu 0x34／0x404 TrainingMenu 0x64／**0x405=_factory 欠落_**／0x406 ClassMenu 0x8C／**0x407=欠落**／0x408 SkillsMenu 0x50／**0x409=欠落**／0x40A PersuasionMenu 0xFC／0x40B RepairMenu 0x78／0x40C RaceSexMenu 0x9B0／0x40D SpellPurchaseMenu 0x68／0x40E LoadgameMenu 0x68／0x40F SaveMenu 0x60／0x410 AlchemyMenu 0xC0／0x411 SpellMakingMenu 0x78／0x412 EnchantmentMenu 0xA0／0x413 EffectSettingMenu 0x9C／0x414 MainMenu 0x50／0x415 BreathMenu 0x34／0x416 QuickKeysMenu 0x50／0x417 CreditsMenu 0x5C／0x418 SigilStoneMenu 0x80／0x419 RechargeMenu 0x54／**0x41A=欠落**／0x41B TextEditMenu 0x5C。
- **欠落 4 スロット（0x405/0x407/0x409/0x41A）は全同一ブロック 0x588794**＝`push 'Unknown menu class!'(0xA6A0D8); call FUN_004A7A60（エラーログ）; return NULL` [確定]。retail にクラス実体なし＝後方互換/未実装 id。
- 相互補強: BookMenu vtbl 0xA6B334・ClassMenu 0xA6B49C・ControlsMenu 0xA69FF4・CreditsMenu 0xA6B784・LevelUpMenu 0xA6C264・TextEditMenu 0xA6A04C・VideoMenu 0xA6A09C は v272 §1.1 の id 表 vtable と**一致**（＋0x04 ルータ保有クラスがファクトリに存在）。TileMenu（0xA6AF14）は 51 表に不在＝TileMenu は非 factory（v272 の「TileMenu は id ルータでない」と整合）。HUDInfoMenu slot 4=0x3ED は v272 §1.2「0x3ED=HUDInfo」と一致。
- 付随確定: **XML/エンティティ名表**（UI parser 帯 @0x58A1C3-0x58A4E3・`v274_1k_out.txt`）: タグ `image/menu/nif/rect/template/text/window`、エンティティ `&AudioMenu; &BookMenu; …`（メニュー名 &entity; 展開表）＋ `<INCLUDE`・'&generic;'・'&false;' 等＝menu XML の DTD 処理側。AS 名表（1.1）とは**別物**（データ駆動側は xml/AS に住む＝retail exe に AS ソース名表は 1.1 の 36 本のみ）。

証跡: `v274_1a/1d/1f/1g/1h/1i/1k/1l/1m/1n_out.txt`・`v274_1e_out2.txt`（51 表）。

## 2. 【低2】extra 0x27（`FUN_0041E7B0`）＝キー連携の実経路候補 — **[確定]**（クラス=**ExtraOwnership**・レイアウト 0x10 の単一 owner ptr・**KeyForm 参照なし [確定負]**＝v272 D2 の「キー連携候補」は**却下**。0x27 は盗み/所有権トラック専用）

### 2.1 アクセサ三点（`v274_2a_out.txt` 逐語）
- Get `FUN_0041E7B0`（this=BaseExtraList・ret 0）: `push 0x27; call FUN_0041E210`（GetExtraData）→ あれば **`[entry+0xC]` を返す**（＝owner ptr そのもの。data blk 間接なし）。無ければ 0。
- Set `FUN_00423660`（thiscall ecx=list・arg=[esp+0x1C]=owner TESForm*・SEH）: ①既存 entry あり & arg!=0 → `mov [entry+0xC], arg` 直接書換 ②既存 & arg==0 → `FUN_00422E20(list, entry, 1)` で削除 ③非存在 & arg!=0 → **`new 0x10` → `ctor FUN_00429E00(arg)`** → `FUN_0041E0A0`（AddExtra）。
- Clear `FUN_0041F600`: `push 0x27; call FUN_0041E160`（RemoveExtra）。
- **ctor `FUN_00429E00`（ret 4・唯一 caller=Set 窓 0x4236F1）**: `[+4]=0x27`（型）／`[+8]=0`（未使用）／**`[+0]=vtable 0xA35818`**／`[+0xC]=arg`（owner ptr）。⇒ entry = **{+0 vtbl, +4 type=0x27, +8=0, +0xC owner TESForm*}（0x10B・余分ヒープなし）** [確定]。
- 型書込走査: `C6 4x 04 27` パターン 2 窓検出も**両者とも誤検出**と確認（@0x429A06 は GMST 比較連鎖 FUN_004299E0 の内部バイト・@0x4AF149 は線形.disasm 境界外のデータ）＝0x27 の型書込は ctor 埋込のみ（ExtraFollower と同 encoding 形）。

### 2.2 クラス名（RTTI 実デコード）
- vtable **0xA35818** → COL 0xAB50CC → TD+8 → **`.?AVExtraOwnership@@`** [確定]（`v274_2d_out.txt`）。画像内 KEY 系 RTTI（`.?AVTESKey@@`@0xB03DD4・`.?AVExtraQuickKey@@`@0xB03FD8）と**型名・住処ともに無関係**。

### 2.3 消費窓の帰属＝所有権（キー経路でないことの決定証拠）
- `FUN_004DB6B0`（**IsOwned**・全文 `v274_2e_out.txt`）: own owner → `vt[0x190]`（=「所持者本人か」述語 [推定]）→ 無ければ container（`FUN_0041E6B0`）→ container の **ExtraTeleport 接続先**（`FUN_0042B410`＝v272 §3 teleport accessor・**接続先マーカーの owner を見る**）→ それでも無ければ base formType 検査 `0x20/0x18/0x12`（=所有コンテナ系型 [推定]）→ 最終 `FUN_004CA970`。**TESKey/TESKey* との比較は一切なし**。
- `FUN_004DE770`（取得可否＝窃取判定）: `FUN_004DB6B0`×2＋`FUN_0x4DB830/0x4DB7D0`（価格/危険系 [推定]）。
- 書込側（`v274_2f_out.txt`）: **0x5FC2B0/0x5FBB10 帯＝アイテム drop/pick up の所有権移転**（`GetBaseForm(vt[0x170])` の結果=**actor/NPC form ptr** を Set・`thePlayer[0xB333C4]` 帰属窓あり・Clear 窓あり）。窃盗転送コア 0x493xxx／スタックマージ 0x484B70／0x48F7C0（player==player で Clear）＝犯罪システム既知帰属（b21/s50）と完全一致。
- 永続化: extra ローダ switch（FUN_004259E0 の既知 list・v268）に 0x27 不在＝**ランタイム専用**（ExtraFollower と同じ扱い・XOWN の保存は SaveLoad 側の changes map 経由 [推定]＝本件範囲外）。
- **裁定: extra 0x27=ExtraOwnership（単一 owner ptr・actor/コンテナ系 form 専用）・キー連携経路でない [確定負]**。v272 D2「別経路候補=0x27」は却下。キー連携の残候補: ExtraLock の外側＝**TESKey form 自体／door-lock の別 extra（0x28 帯）／ゲームルール側総当たり**（次束・新規残点）。

証跡: `v274_2a/2b/2c/2d/2e/2f_out.txt`。

## 3. 【低3】TESSaveLoadGame 完全 member map（0x1C4B・assert 5 本帯 method 窓7）＋`FUN_0045A500`/IAT[0xA2808C] の義 — **[確定]**（IAT[0xA2808C]=**KERNEL32.GetCurrentThreadId**・`FUN_0045A500`=**セーブスレッド同調述語**（一致時+[0x18] bit0／不一致時 bit18）。member map=ctor 書込全集＋method 窓アクセス集計で骨格確定）

### 3.1 IAT[0xA2808C] ＝ `GetCurrentThreadId()`（`v274_3a_out.txt`・pefile import 解決 [確定]）
- `FUN_0045A500` 全文: `edi=[[0xB33398]+0x10]`（=**save/load ワーカースレッド ID**・mgr=[0xB33398] [推定]）→ `call [0xA2808C]`→`cmp eax,edi`→ 一致=`[this+0x18] bit0`／不一致=`[this+0x18] shr 0x12`（bit18）返却。
- **義**: 「現在スレッドが save/load スレッドであり、かつロード進行中フラグ(bit0)が立っているか」の述語。bit18 はその逆状況（ゲームスレッド側）フラグ。assert 帯 method 窓7 は**すべて先頭にこの述語をインライン展開**（0x45B9A0/0x45F7A0/0x45D220/0x45D390 逐語で確認・`FUN_0045B9A0@0x45B9A0..0x45B9BE` 冒頭が FUN_0045A500 と同一列）。SaveGameData assert（'…was called while loading.'）=述語真のときの早期 return＋ログ。[確定]（機構）／関数名 'IsLoadingOnSaveThread' は [推定]。
- 生成窓補強: シングルトンは `FUN_00441970`@0x441B94 `new 0x1C4`→`FUN_0045B300`→`mov [0xB33B00], eax`（ctor 内 self-register@0x45B3C8 を同一値で再書込）。[確定]

### 3.2 member map（0x1C4B・`v274_3a/3b/3c/3e_out.txt`）
| off | 内容 | 根拠 |
|---|---|---|
| +0x00 | **ChangesMap\***（`.?AVChangesMap@@`・NiTMapBase<uint,ChangeData*>）| ctor@0x45B3FC←`FUN_0045A860` [確定] |
| +0x04 | **InteriorCellNewReferencesMap\***（0xA3A310）←`FUN_0045A940`@0x45B418 | [確定] |
| +0x08 | ctor ゼロ（+4 の対で予約/副カウンタ [推定]） | ctor@0x45B33B |
| +0x0C/+0x10 | **ExteriorCellNewReferencesMap\* ×2**（0xA3A330）←`FUN_0045AA80` | [確定] |
| +0x14 | 作業バッファ ptr（**method 窓全員が書込**: B9A0 2W/F7A0 7W/F820 5W/D220 2W/D390 2W/4637C0 3W/463A90 2W・docs 形 `*(undefined2**)`） | [確定 位置]／名目 [推定: formID リマップ作業] |
| +0x18 | **flags dword**: bit0=セーブスレッド側ロード中／bit18=ゲームスレッド側（3.1 述語） | [確定] |
| +0x1C | ゼロ→`FUN_004637C0` が 3 回書込 | [確定 位置]/用途 [推定] |
| +0x20 | ゼロ→`FUN_00463A90` が読取 | 同上 |
| +0x24/+0x28/+0x2C/+0x30/+0x34/+0x38/+0x3C/+0x40 | ctor ゼロ・`FUN_0045F7A0/0x45F820` が +0x40 読取（キュー先 head/cursor 群 [推定]） | [確定 位置] |
| **+0x44/+0x48** | **ctor 書込なし（ゼロ初期化もされない＝未初期化 8 バイト）** [確定負]（new 0x1C4 は非ゼロ化）。遅延書込または死領域 | |
| +0x4C/+0x50 | ゼロ・+0x50 は `FUN_004637C0` 2W | [確定 位置] |
| +0x54..+0x60 | **NumericIDBufferMap\* ×4**（0xA3A350）←`FUN_0045ABE0` | [確定] |
| +0x64/0x68/0x6C | ゼロ（method 窓に直接アクセスなし） | [確定 位置] |
| +0x70/0x71 (byte) | 0→**0x71=0x7D(125) 固定**（mod index 上限 [推定]・4637C0 読取・463A90 書込）／+0x70 は 463A90 書込 | [確定 値] |
| +0x74 | **NiTLargeArray\<UInt32\>\***（vtbl 0xA39044・cap=**100**・grow=[+0x14]=50・buf new cap*4）← ctor@0x45B587 | [確定] |
| +0x78 | **NiTLargeArray\<UInt32\>\***（cap=**5**・grow=1）← ctor@0x45B606 | [確定] |
| +0x7C (byte) | **0x7D** 書込・4637C0/463A90 書込窓（+0x71 の作業コピー [推定]） | |
| +0x7D (byte) | 1→`FUN_0045F7A0`(SaveQueuedHavokData) が書込（havok キュー有効状態 [推定]） | |
| +0x80..+0x90 | ゼロ群・+0x80(2W)/+0x84(2W)/+0x90(1W) に method 窓書込（セーブストリーム関連 ptr 群 [推定]） | |
| +0x94/+0x98/+0x9C/+0xA0 | 明示 NULL×4（xor eax 書込） | [確定 位置] |
| +0xA4・+0xA8..0xAB(byte)・+0xAC・**+0x1C0** | ctor ゼロ | [確定 位置] |
| +0xB0..+0x1BC | **ctor 書込なし**（上記表の範囲外は全て未初期化＝遅延/未使用 [確定負]） | |
- NiTLargeArray 内部形（ctor 逐語で実測 [確定]）: `{+0 vtbl 0xA39044, +4 data ptr(new cap*4), +8 capacity, +0xC=0, +0x10=0, +0x14 grow}`＝v272 の「cap 100/5・0x18B obj」を内部レイアウト込みで確定。
- **本項目内残点（引き継ぎ可・低）**: +0x44/0x48・+0xB0..0x1BC の遅延書込窓（0x45B-0x466 帯全域の走査は method 窓7＋A500 のみ実施。全域集計は次束で可）。

証跡: `v274_3a_out.txt`（ctor store 全集+FUN_0045A500+IAT 名+callers）、`v274_3b/3c_out.txt`（ctor 末尾逐語・+0x74/78 内部・0x7D 群・ret 0x45B668）、`v274_3e_out.txt`（method 窓アクセス集計）、v272 §4（assert 5 本・method 窓7・sub-ctor RTTI 14 件＝再利用）。

## 4. 【低4】map entry alloc サイズ（`FUN_004F0F60`）＋dirty 配列（[0xB06144] 隣接）拡張窓 — **[確定]**（entry=**0xC バイト**・グローバル 0xC プール（2048 ノード/chunk・crit 保護）。dirty 配列=**NiTLargeArray@0xB0614C**（cap 0x20・grow 0x20・拡張=`FUN_00452910(size+32)`））

### 4.1 entry alloc の実体（`v274_4a/4b/4c/4d_out.txt`）
- Set 窓 `FUN_00452570`@0x4525AF の `call [vtbl+0x14]`＝導出 vtbl 0xA3BDD8+0x14＝**`FUN_004F0F60`＝thunk（`add ecx,0xC; jmp FUN_0044D810`）** [確定]（ecx 側合わせは実質無意味・下記グローバルプール形）。
- `FUN_0044D810`（ret 0・プール Get）逐語: `push 0xB33F00; call [IAT 0xA2806C]`=**EnterCriticalSection** → `call [IAT 0xA2808C]`=**GetCurrentThreadId** を `[0xB33F78]` に記録＋`[0xB33F7C]` の再入カウンタ ++/-- → free-list `[0xB33EAC]` からノード pop（空なら `FUN_0044D770` でバルク生成）→ ノード 12B を `{+0,+4,+8}=0` クリアして返す。`FUN_0044D770`（リフィル）: **`[0xB0686C]=2048`(0x800・.data 静的値) × 0xC = 24,576B を `new`** して +0 に次の 12B ノードを連結。
- ⇒ **entry alloc サイズ = 0xC**＝{+0 next, +4 key, +8 value} と完全一致 [確定]。value は slot+0x10 noop（非所有・v272 §5.3 維持）。Set 窓の構築は `call [vtbl+0xC]`@0x4525C1(entry,key,…)。
- 帰結: NiTPointerMap のノードは**ゲーム専有の 0xC グローバルプール**（crit 段取り・スレッド記録付き）から供給。malloc 経路ではない [確定]。

### 4.2 dirty 配列（[0xB06144] 隣接＝0xB0614C）の完全形と拡張窓
- **.data 静的初期値**（`v274_4e_out.txt` 実測）: `0xB0614C={+0 vtbl 0xA37AAC(NiTLargeArray), +4 buf=[0xB06150]=0, +8 cap=0xB06154=0x20, +0xC size=0xB06158=0, +0x10 active=0xB0615C=0, +0x14 grow=0xB06160=0x20}`＋隣 `0xB06164=0xA3BCB4/0xFA1`（v272 の別 map＝本件外）。
- buf 割当のみ CRT: `FUN_009DBF50` = `new 0x20*4 → [0xB06150]`＋`atexit FUN_00A18500`（free 側・0xA18500 引用窓で確認）。
- **拡張窓（確定）**: `FUN_00449070`@0x4490A7 — size([+0xC])==cap([+8]) のとき `FUN_00452910(this, size + grow)`＝**SetCapacity(+32 ずつ)**。`FUN_00452910` 逐語: 新 cap で `new cap*4`→コピー→（縮小時は non-null を `[+0x10]--` しながら truncate）→`[+8]=newcap`。**496 件目以降で 32 刻み成長** [確定]。
- **書込側（dirty 登録）**: `FUN_0046C640(this=TESForm, arg=状態)` @0x46C640 全文 — 状態真: form `+[8] bit18/19` 系ゲート→配列 linearscan（`[0xB06158]` 個数・`[0xB06150]` buf）→ 不在なら `FUN_00449070(0xB0614C, &form)` 追加＋**form+0x8 |= 2**（dirty マーク）・既知の除籍側は `&= ~2`＋該当スロット 0 クリア＋`[0xB0615C]--`。
- 公開ラッパ 5 窓（callers・`v274_4f_out.txt`）: `FUN_004BF110`／`FUN_004CCBC0`（BaseExtraList+0x28 経由）／`FUN_004D6DB0`／`FUN_004E8BF0`／`FUN_005306D0` — いずれも form 側「SetChanged(bool)」形（bit18 先行検査付き）[確定 形/推定 名]。
- 読み手（保存側）: `FUN_004492E0`（0xB06150/0xB06158/0xB0615C 読＝**dirty 走査ループ**・v272 §5.2 の consumers と同一）＋ `FUN_0046B2C0`。
- 自己整合: `FUN_00452910` は件3 の TESSaveLoadGame ctor 内 NiTLargeArray(+0x74/+0x78) resize と同一ヘルパ＝NiTLargeArray 共通レイアウト {+4 buf,+8 cap,+0xC size,+0x10 active,+0x14 grow} を 2 箇所で相互確認 [確定]。

証跡: `v274_4a/4b/4c/4d/4e/4f_out.txt`・v272 §5（0xA3BDD8 vtbl・FUN_009DBF00 割当窓＝再利用）。
