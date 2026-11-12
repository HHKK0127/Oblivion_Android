# v227_opcode_gt_names9.md — opcode/GT 殘点 第9束（V-221-1..8）

- 作成日: 2026-11-01 / 担当: TODO serial 220 (V220・中) spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v221_opcode_gt_names8.md` §7 新規残点 **V-221-1..8**
- 契約: 分析専任（docs/GT/TODO・cache・ソース編集禁止・既存 tmp/re 上書き禁止）。成果物は本ファイルのみ（新規）
- 手法: capstone 5.0.7（v172_common.py 経由・セクション個別変換）＋ MSVC RTTI デコード（v227_lib.py・COL→TypeDescriptor→name）
- selftest: 着手時・完成直前とも `probe_harness.py selftest` → **4/4 PASS**

## 1. 【V-221-1】FUN_004DE980 全体機能名と enum 0..11 — **[部分解明]**

### 1.1 本体の完全デコード（`v227_2/3_out.txt`）

`this`(esi) は TESForm 系（[+8]=formFlags、vt[0x170]=GetBaseForm）。**`[this+8]` bit13 立ち→即 0**。以降 **GetBaseForm()->formType byte** でスイッチ（v221_2 のジャンプテーブルを継承）。全ケースの実測値:

| formType | 帰値 | 条件（asm 逐語） |
|---|---|---|
| 0x12（TESForm 基底） | **4 / 0** | `FUN_004D9890(base)` 述語 |
| 0x13,0x14,0x16,0x19,0x1B,0x1F,0x21,0x22,0x26,0x27,0x28,0x2B | **1** | 既定（0x4DEB98） |
| 0x15 | **1 / 6** | `[base+0x64] != 0` |
| 0x17, 0x24 | **2** | 0x24 は `vt[0x198](0)` 真時に限る |
| 0x18 | **8** | 定数 |
| 0x1A | **1 / 0** | `[base+0x7C]` bit1 |
| 0x1C,0x1D,0x1E,0x25,0x29,0x2A | **0** | 定数（0x4DEBA0） |
| **0x20 = TESFurniture**（後述） | **3 / 5 / 0** | `[base+0x58]` bit30→3、bit31→5（述語 `FUN_004AE590/0x4AE5A0`） |
| 0x23（actor 系基底） | **0 / 2 / 0xB / 1 / 0xA / 7 / 9** | プレイヤ `[0xB333C4]` 同定→0。`vt[0x198](0)`+`0x5E0DC0==6`→戦闘ゲート: `0x5E0F30(x)&&[0xB333C4] 0x5E04C0`→**0xB**、`[0xB333C4] 0x5E0550`→**1**、`0x5E1D30(x)`→**0xA**。容器連鎖 `vt[0x388]`→`[結果+0x58]`→`vt[0x36C]`: ==4→**7**、==0→**9**、非0 で bit1 帯→1/0 |

### 1.2 機能名の判定

- 呼び出し元は 2 件（v221 継承）: (a) `FUN_005A4980`（アイテムラベル文字列組立機 — `%s %s`/`%s's %s`/`%s (%d)`/`%i`/`%ik`/`%iM` 実測、呼び出し元=`FUN_005806D0`/`FUN_00583E60`/`FUN_005A5540`/`FUN_005ADCF0`/`FUN_00671620`）、(b) `FUN_005A8C90`（制御 0xFAE/0xFAF へ float 設定）。
- **`FUN_005A8C90` は E8 呼び出し 0 件・imm32 参照 0 件（whole-file 実測）** → 直接到達コードとしては未参照（dead 候補、または計算ジャンプ経由）。
- → **正式機能名は未解明（残）**。暫称 **`GetFormDisplayCategory`（フォーム表示/処理カテゴリ判定 0..11）**。enum の語義は上表のとおり各 formType 帯に対応し、{3,5} は「基底が formType 0x20 で +0x58 の bit30/bit31 が立つ」種別コード（callers はこれを level/feet 許容フラグとして `FUN_004DBAE0` へ転用 — v221 §2 継承）。
- **重要訂正（v221 §1.3）**: formType 0x20 は **EffectSetting ではなく TESFurniture**（§4 参照）。述語 `FUN_004AE590`（`[ecx+0x58]` bit30）/`FUN_004AE5A0`（bit31）は **TESFurniture のメソッド帯（0x4AE530/0x4AE540 が vtable slot5/8）に位置する非仮想ヘルパ**。したがって本 enum の {3,5} は「FURN フラグ +0x58 bit30/31」であり、CEF kMgefFlag_MagnitudeIsLevel/Feet との同一性は **未裁定（V-227-1）**。
- 未解明（残）: 所属メニューの特定、formType 0x15 ケースの `[base+0x64]` 述語の実体、0x23 帯 vt[0x190]/[0x198]/[0x388]/[0x36C] の正式名。

## 2. 【V-221-2】0xB3BA80 / 0xB3BA9C — **[解明]**

- **0xB3BA80 = モード byte**。**唯一の書き手は `FUN_006553E0`**（arg1 → `mov byte [0xB3BA80],al` @0x6553F2）。読み手は `FUN_006505D0` のゲートのみ（whole-file imm32 走査で計 3 参照＝書 1/読 2）。**初期値=0（BSS・raw 非裏付け）** ⇒ バニラ常時 0 → `FUN_006505D0` は bit31(0x80000000) 立ちを必須とする経路が実効。bit30 経路は mode≠0 のときのみ（`FUN_006553E0` の arg1 が非0の呼び出し元依存）。
- **0xB3BA9C = グローバル連結リスト・ヘルド**（+0=elem / +4=node 鎖、汎用ヘルパ `0x446C30`(contains)/`0x446CB0`(add・347 callers の engine 共通パターン）。**追加者=`FUN_006505D0`**（条件通過時のみ）、**消費者=`FUN_006553E0`**（`this+0xB0` へドレイン → `0x4DB6B0` 述語で重複排除ループ @0x65550A..）。
- **機能**: `FUN_006553E0(this, magicItem, mode)`＝「位置起点の空間走査（`0x566B30` へ **`FUN_006505D0` を関数ポインタ渡し**＝FUN_006505D0 に E8 呼び出しが無い理由）＋コールバックで mode に応じ bit31/bit30 必須の候補 ref を 0xB3BA9C に収集 → this+0xB0 へ統合・整列」。callers: `FUN_0062D750`/`FUN_0062DA10`/`FUN_006578B0`/`FUN_00657A80`（magic 適用帯 0x62D/0x657）。
- **回答**: 「実体クラス」は **属クラス無しのグローバル変数**。機能名（暫称）: `g_feetLevelCandidateRefs`（bit31=feet 型 / bit30=level 型の対象候補 ref 受け渡しリスト）＋ `g_effectTargetMode`（0=feet 必須 / 非0=level 必須）。

## 3. 【V-221-3】FUN_00678750 — **[部分解明]**

- 本体（0x678750..0x678D20 実測・`v227_5/14_out.txt`）: thiscall（this+0x68 を `0x7616D0` 経由）。引数=候補リスト（`[arg]/[arg+4]` 歩行）。候補ごとに: `vt[0x190]`（actor 述語）→`0x5E32D0`→`vt[0x18C]()==0`→`[x+0x58] vt[8]` で即採用経路、否则 `0x4D8AF0` 所有者一致・`0x4DB6B0`/`0x4DE770`・`GetBaseForm`+**`FUN_004AE5A0`（+0x58 bit31）→ `[esp+0x12]=1`**（bit30 版 `FUN_004AE590` @0x678A98 で `[esp+0x12]=0`）・`0x4D73F0`/`0x4DB9D0` ゲート。`[x+0x20]` byte==4/==3 帯で `0x566DC0`/`0x5E0F30`/`0x5E6FA0` ゲート（v221 継承）。
- **`[esp+0x12]` の消費者（新規実測）**: 尾部 0x678C37: `bl=[esp+0x12]` → `vt[0x370]` を **feet なら引数 6 / 否則 1**（`[x+0x58]` オブジェクトへ）、成功時さらに **9（feet）/4（否）**、失敗時 0x7F。⇒ **feet/level 双双対マーカは「適用ディスパッチの型引数切替」に消費**。`[esp+0x13]` 成功時 `[0xB33A10] 0x434020(0)`、末尾 `jmp 0x4418A0`（`[0xB333A0]+0x80` リストのクリーンアップ）。
- 機能名（暫称）: **`SelectEffectTargetAndDispatch`（候補 ref から適用対象を選択し feets/level 型マーカ付きで適用呼び出し）**。callers: `FUN_00444C70`/`FUN_004455E0`/`FUN_00445A10`/`FUN_0066F420`（所属未解明）。
- 未解明（残）: `[x+0x58]` オブジェクト（巨大 vtable: slot 8/0x80/0x17C/0x188/0x370/0x380/0x384）のクラス名、`vt[0x370]` 型引数 1/4/6/9/0x7F の語義、callers の所属システム。

## 4. 【V-221-4】formType 0x0C クラス = **EffectSetting（RTTI 確定）** — **[解明]**

- **ctor `FUN_00416430`（vtable 0xA32B14）の RTTI = `.?AVEffectSetting@@`（vtable-4 の COL→TypeDescriptor→name を実デコード・`v227_6/9_out.txt`）**。size **0xA8**、**`mov byte [esi+4],0x0C` @0x416544** ⇒ **EffectSetting の formType = 0x0C**。基底 ctor `FUN_0046C460` は TESForm（RTTI `.?AVTESForm@@`）。
- レコードロード経路: ディスパッチ帯の MGEF ケース（0x44E367..0x44E3F2）: EDID 読出→`FUN_004168A0`(EDID 解決)→無ければ `new(0xA8)+FUN_00416430`→`TESFile::ReadRecord 0x451210`→マスタリスト `[ebp-0x14]+0x8B8` へ `0x446CB0` 追加。
- 実行時生成経路: `FUN_00417220` も同一 `new(0xA8)+FUN_00416430` → `[esi+0x58]=arg6(flags)` → 末尾 `[0xB33508]`（NiTMap<code,EffectSetting*>）へ `0x452570` で登録（**ESM 読込済み MGEF を実行時定義で上書き/補完**）。
- **'ESCE' チャンク**: imm32 参照は `FUN_00415800`(0x415945) と `FUN_00416100`(0x416143) の 2 件＝**EffectSetting のファイル chunk ハンドラ帯**。関連効果コード配列（`[+0x9C]`・件数 word `[+0x6C]`、`FUN_00415D40` が格納）を保持。
- **重大訂正（v221 §1.3）**: **formType 0x20 = TESFurniture**（ctor `FUN_004AE660`・vtable 0xA41D1C・RTTI `.?AVTESFurniture@@`（COL 0xABA4C0/pTD 0xB08088 二重実測）、size 0x5C、type-form 工場 `FUN_00447DF0` の case が `push 0x5C; call 0x4AE660`）。v221 の「formType 0x20 = EffectSetting [確定]」は **誤り**（述語 0x4AE590/0x4AE5A0 の隣接からの推測）。mgefFlags は EffectSetting の +0x58（ctor 0 初期化）で正しいが、述語の対象クラスは TESFurniture 系。

## 5. 【V-221-5】ENIT フィールド意味論 — **[部分解明]**

### 5.1 ctor 既定値の全実測（`v227_15/17_out.txt`）

| クラス | フィールド（既定） | 証拠 |
|---|---|---|
| EnchantmentItem (0x0F) | **+0x34 u32 = 2 / +0x38 u32 = -1 / +0x3C u32 = -1 / +0x40 u8 = 0** | ctor `FUN_00419570`: `[esi+0x34]=2, [esi+0x3C]=-1, [esi+0x38]=-1, byte [esi+0x40]=0` |
| SpellItem (0x10) | **+0x34 = 0 / +0x38 = -1 / +0x3C = 0 / +0x40 = 0** | ctor `FUN_0041D420` 実測 |
| AlchemyItem (0x28) | **+0x78 u32 = -1 / +0x7C u8 = 0** | ctor `FUN_00412BE0`: `[esi+0x78]=0xFFFFFFFF, byte [esi+0x7C]=0` |

### 5.2 語義の裁定

- **+0x34 = SPIT/ENIT `type`**: SpellItem 既定 0（Spell）/ EnchantmentItem 既定 2（Enchantment）。**`FUN_004190C0`（同一 ExtraData 帯の型ディスパッチ）が `[this+0x34]` を 2/1/3/0 と比較**（0x419107/0x41913D/0x41915A/0x4191AA/0x4191C1、base-class 別に「type==2」「type==1&&[base+0x90]==4」「type==3」「type==0」を判定）＝SPIT/ENIT 共通 16B レイアウト `{type, dword, dword, byte}` の先頭が type であることを EXE 側で確定。
- **+0x38 = cost 系（既定 -1 = auto-calc センチネル候補）**、**+0x3C / +0x40 = 未解明**（compare `FUN_00419010` は 4 フィールド全比較、copy `FUN_00419060` は全複写。RTTI 型検査 `_RTDynamicCast`(0x9832E6) 付き＝同種 ExtraData 同士の等値判定）。
- **ALCH/INGR ENIT 8B @+0x78 = {u32=-1, u8=0}**: compare `FUN_00412A00`（RTTI 0xB0320C）は dword+byte のみ比較。dword の意味未解明（vanilla で常に -1/0 と整合）。
- ENIT/SPIT は同一 impl（ChunkType `0x419730`/Ptr `0x419000`/Size `0x8C4830`）＝v221 継承。埋め込み ExtraData 位置: ENCH +0x18（compare 呼び出し時の `esi` 偏移 +0x1C/+0x20/+0x24/+0x28 = obj+0x34.. と整合）。
- 未解明（残）: cost/flags の各 dword の正式名、ALCH dword の意味、+0x34 type 1/3 の正体。

## 6. 【V-221-6】EffectSetting 未命名フィールド — **[大幅進展]**

### 6.1 登録ヘルパ `FUN_00417220` によるフィールド割当（逐語実測）

`FUN_00417220(code, name, arg3, arg4f, arg5, flags, arg7, count, rel[])` → `EffectSetting`:

| オフセット | DATA 偏移 | 引数 | 語義（証拠） |
|---|---|---|---|
| +0x58 | +0x00 | flags | mgefFlags（bit23/0x70 レンジ検査・bit16-21 ASSOCIATED 検査） |
| +0x5C | +0x04 | float | cost 系 float（全登録 0.0・ctor 0.0） |
| **+0x60** | +0x08 | arg5 | **ActorValue**（例: WABR=55, SHLD=43, BRDN=11=Encumbrance, WKFI=61=FireRes, CALM/FRNZ=33=Aggression — AV 番号と効果の対応が全一致） |
| **+0x64** | +0x0C | arg3 | **school（kMagicSchool 0..5）** — 登録列で 0=Alteration(WABR 等)/1=Conjuration(Zxxx,BAxx)/2=Destruction(DGxx 等)/3=Illusion(CHRM 等)/4=Mysticism(DSPL,STRP,TELE,DTCT,SABS,RFLC,REDG)/5=Restoration(RExx,FOxx,ABxx,RSxx,CUxx) と完全一致。**ctor 既定 6 =「学校なし」**（SEFF 登録 arg3=6）。**resolver `FUN_0041B820`** は school 0..5 → グローバル 0xB33560..0xB33574（>5→NULL）。**v221「+0x64=formID 系・0x41B820=フォーム解決」は訂正** |
| **+0x68** | +0x10 | arg7 | **resistAV**（-1=none、FIDG→61=FireRes、SHDG→68=ShockRes、DGxx→64=MagicResist、PARA→66=ParalysisResist、POSN→67=PoisonResist、DISE→63=DiseaseResist） |
| +0x6C | — | count(word) | 関連効果件数（`FUN_00415D40` 格納） |
| **+0x74** | +0x1C | — | **float・ctor 既定 1.0**（`fld1→[esi+0x74]`）。倍率系（v221 の fallback 1.0 3 callers と整合） |
| +0x78 | +0x20 | — | dword 0（アクティブエフェクト instance+0x34 へ複写＝v221） |
| +0x7C | +0x24 | — | ptr 0（VFX/シェーダ解決先候補・new 0x4C/vt0x420 適用＝v221） |
| +0x80 | +0x28 | — | dword 0（非ゼロ必須ゲート 2 callers＝v221） |
| +0x90/+0x94 | — | — | float（グローバル 0xB33704/0xB3370C 既定） |
| +0x98 | — | code | mgefCode（既定 -1、登録時四cc） |
| +0x9C/+0x6C | — | rel[] | 関連効果コード配列（'ESCE' 由来） |

- 未解明（残）: +0x5C の正式名（cost?）、+0x78/+0x7C/+0x80 の ESM DATA 内語義、+0x94 の用途。

## 7. 【V-221-7】FUN_00417420 登録列の完全枚挙 — **[解明]**

- 本体 = 0x417420..**0x418DAA**（`ret` 実測）。`call 0x417220` **計 161 件**（95+66・`v227_12/16/17_out.txt`）。全件の code/name/school/AV/flags/resistAV/関連 fourcc を表形式で実取得（本ファイルの証跡 txt 参照）。
- **fourcc 解読（全判明）**: DSPL=Dispel / REAT/REHE/RESP/REFA=Restore Attribute/Health/Spell Points/Fatigue、FOxx=Fortify、ABxx=Absorb、DRxx=Drain、DGxx=Damage、WKxx=Weakness to xx、RSxx=Resist xx（RSFI=Resist Fire・RSFR/RSSH/RSMA/RSDI/RSPO/RSPA/RSNW/RSWD）、CUDI/CUPO/CUPA=**Cure Disease/Poison/Paralysis**、FIDG/FRDG/SHDG=Fire/Frost/Shock Damage、LGHT=Light、DARK=Darkness、SUDG=Sun Damage、STMA=Stunted Magicka、VAMP=Vampirism、DUMY=Mehrunes Dagon Custom Effect、COHU/COCR=Command Humanoid/Creature、TURN=Turn Undead、SEFF=Script Effect、REAN=Reanimate、ZGHO..ZXIV/Z001-Z020=Summon xx/Extra Summon、BWxx/BAxx=Bound Weapon/Armor（Extra BW01-10/BA01-08）、DIAR/DIWE=Disintegrate Armor/Weapons、STRP=Soul Trap、TELE=Telekinesis、DTCT=Detect Life、SABS=Spell Absorption、RFLC=Reflect、REDG=Reflect Damage、NEYE=Night-Eye、WABR/WAWA=Water Breathing/Walking、SHLD/FISH/LISH/FRSH=Shield 系、BRDN/FTHR=Burden/Feather、LOCK/OPEN、INVI/CHML/PARA/SLNC/CHRM/DEMO/RALY/FRNZ/CALM、FOMM=Fortify Magicka Multiplier。**'DSPL' 等の「関連 fourcc」は対抗効果コード**（DUMY→REHE/FOHE/ABHE/DSPL、WKFI→FISH/RSFI/DSPL 等）。
- **bit30=MagnitudeIsLevel の実証追加**: COHU/COCR/TURN flags に **0x40000000(bit30)**（Command/Turn = "up to level" 表記）＋ **LGHT/DARK flags 0x80000072/0x81000072 の bit31**（Light/Darkness = feet 半径）— v221 §1.2 の GMST 表記と矛盾なし。
- **v221 §6.2 訂正**: 「multiple ASSOCIATED flags」エラー（0xA330D4）到達経路は bit16-20 が全て不立のときのみで、さらに `(flags>>0x18)&1 > 1` 判定が **`and eax,1` により恒偽（jle 常時継続）** ⇒ **当該エラー経路は実質死コード**。bits16-20 は「1 つでも立つとエラー」ではなく「立つと検査スキップ（継続）」。
- 登録は map Add（0xB33508）で **ESM 版を実行時定義で上書き**する挙動。

## 8. 【V-221-8】適用帯 bit23 ホルダ — **[機構解明・クラス名は残]**

- **ホルダ = flags dword `[obj+0x1C]` を持つ実行時オブジェクト**。適用帯 8 件の共通形は 3 重参照（`v227_15_out.txt`）:
  - `FUN_0063BA30`: `eax=[esi+8]; ecx=[eax+0x1C]; shr ecx,0x17` → 立ちなら適用スキップ。
  - `FUN_005ED860`: `edx=[esi+0x58]; eax=[edx+8]; ecx=[eax+0x1C]` → bit23 立ちで `[esi+0x44]` 経由の `0x420840` パスへ分岐。
- **生成側の実証（新規）**: `FUN_0060F250` @0x60F9AA..0x60F9FA: 新規オブジェクト ebp に対し、** ebx(+0x1C) の bit23(0x800000)/bit20(0x100000)/bit21(0x200000) を or/and 対で複写 → `or [ebp+0x1C], 6`（bit1|bit2 既定立）**。⇒ ホルダは「meta/associated フラグ群（bit16-21・23）を継承する実行時エフェクト系オブジェクト」。
- クラス名は未取得（ctor/RTTI 未探索）＝**未解明（残・V-227-6）**。CEF 呼称との対応は ActiveEffect 系が候補。

## 9. GT ドラフト（反映は要承認）

- **(M)** GT「formType 欄」行: **EffectSetting = formType 0x0C（ctor `FUN_00416430`・size 0xA8・RTTI `.?AVEffectSetting@@`）**、**formType 0x20 = TESFurniture（ctor `FUN_004AE660`・size 0x5C・RTTI 確定）**。v221「0x20=EffectSetting」は**削除訂正**。MGEF レコードロード＝ディスパッチ 0x44E367 帯（EDID 解決→new 0xA8→`0x451210`）、実行時登録＝`FUN_00417220`（map 0xB33508 へ上書き登録）。
- **(N)** GT「`FUN_004AE590/0x4AE5A0`」行 訂正候補: **TESFurniture(+0x58=FURN DATA flags) bit30/bit31 述語**（callers 全て formType 0x20 ゲート系）。CEF kMgefFlag_MagnitudeIsLevel/Feet との同一性は未裁定（→V-227-1）。EffectSetting の bit30/31 読み取りは `FUN_00413BE0` インライン経路が正（v221 (G) の本体記述は維持）。
- **(O)** GT「EffectSetting フィールド」行 追加: **+0x60=ActorValue / +0x64=School(0..5・既定6=なし・resolver `0x41B820`→0xB33560..) / +0x68=resistAV(-1=none) / +0x74=float 既定1.0（倍率系） / +0x98=mgefCode / +0x9C=関連効果配列(+0x6C 件数・'ESCE' 由来)**。v221「+0x64=formID 系」は**訂正**。
- **(P)** GT「ENIT/SPIT」行: **16B {type(+0x34・Spell=0/Ench=2・`FUN_004190C0` 型分岐 0/1/2/3), dword(-1=cost 候補), dword, byte}**。ALCH/INGR 8B @+0x78={u32=-1, u8=0}（比較対象のみ・意味未解明）。
- **(Q)** GT「`FUN_00417420`」行: **組み込み EffectSetting 登録 161 件の完全列（0x417420..0x418DAA）**。引数契約: (code, name, school, costF, AV, flags, resistAV, 関連件数, 関連 code[])。POSN/DISE の flags=0x800000 のみ（bit23）は v221 継承。関連 fourcc=対抗効果コード（CUPA/CUPO/CUDI= Cure Paralysis/Poison/Disease 等）。「multiple ASSOCIATED flags」エラー経路は**死コード**（訂正）。
- **(R)** GT「`0xB3BA80/0xB3BA9C`」行: **`g_effectTargetMode`(byte: 0=bit31-feet 必須/非0=bit30-level 必須・唯一の書込=`FUN_006553E0` arg1) ＋ `g_feetLevelCandidateRefs`(連結リスト・追加=`FUN_006505D0`(関数ポインタ渡し)・消費=`FUN_006553E0`)**。
- **(S)** GT「bit23 伝播」行 補強: `FUN_0060F250` は **bit20/21/23 の3ビット群**を or/and 対で複写し `or [dst+0x1C],6`。適用帯のホルダは `[x+8]→[+0x1C]`（`FUN_005ED860` は `[x+0x58]→[+8]→[+0x1C]` の3重参照）。

## 10. 新規残点（次タスク候補）

| ID | 内容 | 優先 |
|---|---|---|
| V-227-1 | TESFurniture+0x58（FURN DATA flags）bit30/31 の意味と、callers（FUN_004DE980/DBAE0/6505D0/678750）の「level/feet」解釈との裁定（CEF kMgefFlag_* との関係） | 中 |
| V-227-2 | FUN_004DE980 の正式機能名・enum 0..11 の対応メニュー特定（FUN_005A4980 の callers 0x5806D0/0x583E60/0x5A5540/0x5ADCF0/0x671620 の所属）、FUN_005A8C90 の未参照理由 | 中 |
| V-227-3 | FUN_00678750 の `[x+0x58]` オブジェクトクラス（slot 8/0x17C/0x370/0x380/0x384）と vt[0x370] 型引数 1/4/6/9/0x7F の語義、callers 0x444C70/0x4455E0/0x445A10/0x66F420 の系統 | 中 |
| V-227-4 | EffectSetting +0x5C/+0x78/+0x7C/+0x80 の ESM DATA 対応語義（+0x74=float 既定1.0 の正体含む） | 中 |
| V-227-5 | ENIT dword1/dword2/byte3・ALCH ENIT dword の意味（`FUN_004190C0` type enum 1/3 の正体） | 低 |
| V-227-6 | bit23 ホルダクラスの RTTI/ctor 特定（`FUN_0060F250` が ebp に代入する新規オブジェクトの生成元 0x6068D0 経路） | 低 |
| V-227-7 | school→グローバル 0xB33560..0xB33574 の 6 オブジェクトの正体（spell? form?）と school 有効化コード | 低 |

## 11. 本タスクでの訂正まとめ

| 対象 | 訂正内容 |
|---|---|
| v221 §1.3「formType 0x20 = EffectSetting [確定]」 | **誤り** → formType 0x20 = **TESFurniture**（RTTI+type-form 工場で確定）。EffectSetting = formType 0x0C |
| v221 §5.2/§7「+0x64 = formID 系・0x41B820 = フォーム解決」 | **誤り/不正確** → +0x64 = **school**、0x41B820 = **school→グローバル Object 解決**（>5→NULL） |
| v221 §6.2「multiple ASSOCIATED flags エラー経路（bits16-20 で発火）」 | **誤り** → bits16-20 は「立つと検査スキップ」。エラー本体の bit24 判定は `and eax,1` で恒偽＝**到達不能（死コード）** |
| v221 (G) 述語行 | 訂正候補: 対象は TESFurniture 系（(N) 参照） |
| v221 §6.1「formType 0x0C クラス（名未同定）」 | **解消** → EffectSetting（RTTI 確定） |

## 12. 自己検証・証跡

- **selftest**: 着手時・完成直前とも `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- **内部整合（一致 10 項目）**:
  1. EffectSetting ctor 既定（+0x58=0/+0x64=6/+0x74=1.0/+0x98=-1/+0x6C word0/+0x9C 0）と `FUN_00417220` の上書き引数が全フィールドで矛盾なし。
  2. school 値 0..5 の効果分布（登録 161 件）がバニラの学校割当（Conjuration=Summon/Bound、Mysticism=Dispel/SoulTrap、Restoration=Restore/Fortify/Absorb/Resist/Cure）と完全一致。SEFF=6=学校なし・ctor 既定6 と整合。
  3. AV 値（+0x60）: Burden/Feather=11(Encumbrance)、WKFI/RSFI/FISH=61(FireRes)、WKPO/POSN=67(PoisonRes)、CALM/FRNZ=33(Aggression) 等が AV 番号体系と整合。
  4. resistAV(+0x68): DGxx→64(MagicResist)、PARA→66、POSN→67、DISE→63 がダメージ耐性系 AV と整合。
  5. bit30=MagnitudeIsLevel（COHU/COCR/TURN）/ bit31=MagnitudeIsFeet（LGHT/DARK）が v221 の GMST 表記（"up to level"/"ft"）と整合。
  6. formType 0x20=TESFurniture は ctor RTTI（0xA41D1C→COL 0xABA4C0→pTD 0xB08088）と type-form 工場 `FUN_00447DF0`（push 0x5C）の二経路で一致。
  7. ENCH(0x0F)/SpellItem(0x10) の ctor 既定差（+0x34=2 vs 0、+0x3C=-1 vs 0）と `FUN_004190C0` の type 分岐（2/1/3/0）が両立。
  8. 0xB3BA80/0xB3BA9C の whole-file imm32 走査（file_off 0x24FA7A/0x24FA90/0x2547F3 / 0x24FA16/0x24FAA3/0x2548B5/0x2548FB）が VA 変換（.text raw 0x400C00）と過不足なく一致（追加リファレンス無し）。
  9. FUN_006505D0 の E8 呼び出し 0 件と `FUN_006553E0` @0x655473 の `push 0x6505d0`（関数ポインタ渡し）が整合。
  10. 制約遵守: 成果物は本ファイルのみ（新規1件）。docs/GT/TODO/cache/ソース/既存 tmp/re 編集 0 件。ESM 本体未アクセス。web 検索なし。
- **証跡（すべて `%TEMP%\opencode/`・本タスク生成）**: `v227_lib.py`（RTTI/func ヘルパ）/ `v227_2.py`（DE980 全文+callers）/ `v227_3.py`（DE980 tail+メニュー文脈）/ `v227_4.py`（0xB3BA80/9C 参照+0x446C30/0x446CB0+FUN_006505D0 全文）/ `v227_5.py`（**FUN_00678750 全文+callers**）/ `v227_6.py`（**formType 0x0C ctor 走査+RTTI+dispatcher+ESCE/MGEF fourcc**）/ `v227_7.py`（FUN_00417220 全文+登録列 45 件窓）/ `v227_8.py`（FUN_006553E0 全文+callers+初値）/ `v227_9.py`（**TESFurniture ctor RTTI+EffectSetting ctor 全文+dispatcher 0x44E000 帯**）/ `v227_10.py`（名称文字列+登録列 tail+callers）/ `v227_11.py`（**述語 0x4AE590/0x4AE5A0 全文+0x41B820+FUN_004DBAE0**）/ `v227_12.py`（**登録列機械デコード 95 件**）/ `v227_13.py`（**RTTI 再確認+TESFurniture vtable**）/ `v227_14.py`（FUN_006553E0 消費ループ+whole-file 走査+678750 tail）/ `v227_15.py`（**ENCH/ALCH/SpellItem ctor+compare+bit23 伝播/適用帯文脈**）/ `v227_16/17.py`（登録列 tail 66 件+type ディスパッチ+FURN 工場）/ `v227_18.py`（FURN ローダ+FUN_004190C0）＋対応 `v227_*_out.txt`。
