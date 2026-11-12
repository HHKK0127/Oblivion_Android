# v216_opcode_gt_names7.md — opcode/GT 名前 第7束（V207）

- 作成日: 2026-11-01 / 担当: TODO serial 207 (V207・中)
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `tmp/re/v201_opcode_gt_residuals3.md` §5 残留不明点 1–4
  1. 型トークン {0x12,0x13,0x14,0x1F} のキーワード名（FUN_004FF710 変数宣言 case 4 種）
  2. base+0x58 bit7 / bit22 / bit0 の ESM・GECK record-flag 定数名（B13 不要の資料照合）
  3. slot[6] 値4 の型コード対応語と ESM ロード時書手（SpellItem +0x34）
  4. FUN_00419B30 の type==1 → "DISE" 分岐の意味論（0xB33508 レジストリ fourcc）
- 契約: 分析専任（docs/GT/TODO・cache・ソース編集禁止・既存 tmp/re 上書き禁止）。成果物は本ファイルのみ（新規）
- 手法: capstone 5.0.7（v172_common.py 経由）＋ `docs/Oblivion_decompiled.c` 部分検索 ＋ web 検索（GECK flags 仕様）
- selftest: 着手時 `probe_harness.py selftest` → **4/4 PASS**

## 1. 残点1: 型トークン {0x12,0x13,0x14,0x1F} のキーワード名 — **[確定]**

### 1.1 トークン名テーブルの発見（本タスクの核心証拠）

- **キーワード引き当て関数 = `FUN_004FCA70`**（0x4FCA70..0x4FCB81、98 命令を capstone 全文実測）。3 つのテーブルを `_strcmpi`（0x98262D）で線形走査し、名前が合致したエントリの **id フィールドを `record[+0x208]` へ、`record[+0x204]='X'(0x58)` を「キーワード一致」マーカとして設定**する:
  ```
  表A: 0xB0A2C8..0xB0A54C  16 エントリ（stride 0x28）＝文キーワード（token 0x10..0x1F）
  表C: 0xB0B420..0xB0C89C 131 エントリ＝コンソール系コマンド（token 0x100..）
  表B: 0xB0C8C0..0xB1026C 369 エントリ＝スクリプト関数（token 0x1000..）
  エントリ布局: +0=name1 ptr / +4=name2(別名) ptr / +8=u32 token id / +0xC..=0 塗り
  ```
- 表A の 16 件を **.data から実読み**（`%TEMP%\opencode\v216_9_out.txt`）:

  | token | name1 | name2(別名) | 備考 |
  |---|---|---|---|
  | 0x10 | **Begin** | — | case 0x10「Missing block type in 'begin' command.」と一致 |
  | 0x11 | **End** | — | case 0x11「Failed to store the 'begin' 'end' jump block.」と一致 |
  | **0x12** | **Short** | **int** | ★ 変数宣言 case（→ +0x10 flag=1） |
  | **0x13** | **Long** | — | ★ 変数宣言 case（→ +0x10 flag=1） |
  | **0x14** | **Float** | — | ★ 変数宣言 case（→ +0x10 flag=0） |
  | 0x15 | Set | — | case 0x15「Missing variable name in set command.」と一致 |
  | 0x16 | If | — | 式付き if（case 0x16/0x18 の式コンパイル経路） |
  | 0x17 | Else | — | 式なし（case 0x17 は FUN_004fd430 ブロック整形のみ） |
  | 0x18 | ElseIf | — | 式付き（case 0x16/0x18 と同経路） |
  | 0x19 | EndIf | — | case 0x19 = no-op break（ブックキープは FUN_004fd430 側） |
  | 0x1A | **While_DEPRECATED** | — | switch に case なし（非推奨・到達不能） |
  | 0x1B | **EndWhile_DEPRECATED** | — | 同上 |
  | 0x1C | **.**（ドット） | — | no-op break（参照構文の一部） |
  | 0x1D | **ScriptName** | **scn** | FUN_00500170 の `iVar4==0x1d`＝「scriptname 必須」判定と一致 |
  | 0x1E | **Return** | — | no-op break |
  | **0x1F** | **Reference** | **ref** | ★ 変数宣言 case（→ +0x10 flag=0、name 登録を追加実行） |

- 文字列は **`Begin`/`Short`/`Float` 等の先頭大文字**（`_strcmpi` で大文字小文字無視照合）。v216_1 で小文字 `"begin"` を走査して 0 件だったのはこのため（表は 0xA51B94..0xA51C1C 帯に実在）。
- 表C/B のサンプル実測: `[0] Show/TST=0x100`（別名 TST 等）、`[0] MessageBox=0x1000` — FUN_004FF710 default 分岐の `&PTR_DAT_00b0b420 + (tok-0x100)*10` / `&PTR_s_MessageBox_00b0c8c0 + (tok-0x1000)*10`（*10 dword = 0x28 stride）と完全一致。

### 1.2 トークン供給経路（v201 の残点1 完結）

- 行→トークンの供給チェーン（decompiled 実測）: `FUN_004FF4C0`（1 行コンパイル）→ `FUN_004FCF10`（行読取、0x200 バッファ+[+0x204]=len）→ `FUN_004FD7C0`（単語抽出・引用符/`.` 参照処理）→ 未解決語なら **`FUN_004FD5F0`（%変数リスト）→ `FUN_004FCA70`（本表 A/B/C）** の順で照合（L197074-197076「Unknown variable or function '%s'.」）。一致時 `record[+0x204]='X'`・`record[+0x208]=token` → `FUN_004FF4C0` が **`piVar6[0x104] = local_224`（= +0x410 = 現トークン type）** へ転記（L198088／asm 0x4FF66A）→ `FUN_004FF710` の switch が分岐。
- **トークン供給の全体像が閉じた**: キーワード名 → token id → switch case の対応は表A 1 本で説明でき、「DAT_00B0AF48 帯（block-type 表）」は **`begin` の後続ブロック名表**（"Quest"/"GameMode"/"Region" 等の block type）でありキーワード表とは別物（v201 の推測どおり）。

### 1.3 'f'/'s' 判定の再解釈（v193 の訂正候補）

- v201 §2.2 の判定値 **{0x14 Float, 0x1F Reference} → 0（'f'）/ {0x12 Short, 0x13 Long} → 1（'s'）** は、本表で「'f'=**float 系宣言**（Float/Reference）、's'=**整数系宣言**（Short/Long）」と**確定**。
- ⚠️ **v193 の「's' = string」は誤り**（オーバーライド候補）。ObScript に文字列変数は存在せず、's' は **Short（整数）** の頭文字と解するのが表A と整合する。'f'+0x0D='s' 式（'f'=0x66 + 0x0D = 0x73）は型 char のエンコードにすぎない。

## 2. 残点2: base+0x58 bit7 / bit22 / bit0 の GECK 定数名 — **[確定(資料照合)]**

### 2.1 [base+0x58] の実体 = EffectSetting（MGEF）の mgefFlags（ESM DATA flags）

- **公開 RE 仕様（JRoush's Common-Oblivion-Engine-Framework、`API/Magic/EffectSetting.h`）**: `MEMBER /*58/84*/ UInt32 mgefFlags;` — **ゲームビルドでのオフセットは +0x58**。EffectSetting は `TESForm(+0x18) + TESModel + TESDescription + TESFullName + TESIcon` の多重継承で、+0x58 = **MGEF レコード DATA サブレコードの Flags dword**。
- **EXE 側の整合（既存実測の再解釈）**: bit22 チェッカ群（`0x414EB0..0x415306`）の `[[elem+0x1C]+0x58]` で elem は **EffectItem**（CEF `API/Magic/EffectItem.h` ゲーム布局: `+0x00 mgefCode / +0x04 magnitude / +0x08 area / +0x0C duration / +0x10 range / +0x14 actorValue / +0x18 ScriptEffectInfo* / +0x1C EffectSetting* effect / +0x20 float cost`）→ **base=[elem+0x1C]=MGEF、[MGEF+0x58]=mgefFlags** と 1:1。
- ⚠️ **前タスクの命名訂正（重要）**: v177 §5/v198/v201 が `FUN_004133C0/0x4133E0` を「base bit7 ガード付き **formID** getter/setter」としたのは**誤り**。+0xC は EffectItem の **duration**（SInt32）であり、
  - `FUN_004133C0` = **`EffectItem::GetDuration()`**（CEF 実装コメント "always returns 0 if effect cannot have duration" と逐語一致: NoDuration(bit7) 立ち → 0）
  - `FUN_004133E0` = **`EffectItem::SetDuration()`**（NoDuration or arg<0 → 拒否、成功時 duration 設定＋`[+0x20](cost)=-1.0f` へキャッシュ無効化）
  - `FUN_00413470` = **`EffectItem::IsHostile()`**（CEF "IMPORT bool IsHostile()"。`[+0x18]`(scriptInfo) 有 → `[+0x14]`=ScriptEffectInfo.hostile byte を返し、無 → `[effect+0x58]` の **bit0=Hostile**）
  - `FUN_00413BE0` はエフェクト記述子名の組立（NoDuration で duration 部を省略）、`FUN_00413FC0`/`FUN_0068E670` も同ファミリ。→ **「base-disabled guard」仮説（v201 §3.1）は棄却**し、**NoDuration ゲート**に確定。

### 2.2 ビット定数対応表（資料照合の結論）

| bit | 値 | 正式定数名（CEF `EffectSettingFlags`） | 実測機能との突合 |
|---|---|---|---|
| **bit0** | 0x00000001 | **`kMgefFlag_Hostile`** | IsHostile() 本体。OBSE 公式コマンド_doc: "**Only hostile effects may be in a poison**" → 毒判定（`FUN_00414EB0` 系）の基準と**完全一致** [確定] |
| **bit7** | 0x00000080 | **`kMgefFlag_NoDuration`** | GetDuration→0 / SetDuration 拒否 / 表示の duration 省略（v201 の「評価が閉じる」は duration 操作閉鎖）[確定] |
| bit21 | 0x00200000 | **`kMgefFlag_PCHasEffect`** | CEF コメント「Whether or not the player 'knows' the effect... List of effects player knows is stored as mgefCodes in savegame」＝ v193b の known-MGEF マーカ（Player LoadGame set／`FUN_004167E0` clear）と**完全一致** [確定] |
| **bit22** | 0x00400000 | **`kMgefFlag_Disabled`**（CEF 別名 "**ignored**"：`EffectItemList::HasIgnoredEffect()` = "checks for an effect with the 'ignored' flag (1<<22)"） | CEF コメント: "**No default effects have this. many (all?) of the methods in EffectItemList that loop over the effect items ignore effects with this flag. Spells with an effect with this flag are apparently uncastable.**" → v201 実測（毒判定素通し／効果選択除外／casting 拒否）と**逐語的に一致**。 vanilla 161 MGEF に 0 件＝「No default effects have this」も一致 [確定] |
| bit23 | 0x00800000 | `kMgefFlag_UnknownO` | コメント: "**POSN,DISE - these effects have \*only\* this bit set**, perhaps a flag for 'meta' effects? exempts effect from valid range check... makes effect unavailable in spellmaking alter" → 項目4 の DISE/POSN メタエフェクトと直結 [確定(傍証)] |
| bit30 | 0x40000000 | `kMgefFlag_MagnitudeIsLevel`（"for display purposes only"） | `FUN_004AE590` 述語＋`FUN_004de980` の {0,3,5} 戻り＝表示単位 enum の一因と推定 [有力] |
| bit31 | 0x80000000 | `kMgefFlag_MagnitudeIsFeet`（"for display purposes only"） | `FUN_004AE5A0` 述語。v193b の bit31=「負センチネル」解釈は **MGEF flags では成立しない**（MagnitudeIsFeet が正解）[訂正] |

- **GECK 公開チェックボックスとの関係**: UESP MGEF DATA フラグ表（Hostile/Recover/…/Use actor value/ projectile type/ No hit effect）は bit0/bit7 を含む vanilla 既用ビットを網羅するが、**bit21/22/23 は UESP に未記載**（CEF が CS ダイアログで checkbox が無い "hidden" ビットとして分類）。GECK UI からは **bit22 (Disabled) は設定不能＝vanilla では常に 0**、mod で hex 編集した場合のみ有効、という「ESM データ由来・実行時非 set」の v201 判定と整合。
- **TESForm 側の対比（混同防止）**: CEF `TESForm.h` の `formFlags` は **+0x8**（`kFormFlags_Deleted=0x20` 等・`kFormFlags__SavedInRecord=0x000A8EE0`）。UESP 一般 record flags（0x20 Deleted / 0x400 Quest item / 0x800 Initially disabled / 0x40000 Compressed / 0x80000 Can't wait…）は **+0x58 とは無関係**。v177 の「TESBoundObject flags 対応表」課題は**前提が誤り**（+0x58 は MGEF クラス固有のオフセット）[訂正]。
- **供給源の再定義**: bit22/21/23 は「ESM レコードヘッダ flags」ではなく「**MGEF DATA flags（ESM からロード）＋エンジンによる未使用 bit の流用**」。bit21 のみ runtime set/clear 双対あり（既知効果リスト）、bit22 は vanilla 0・実行時非 set、bit23 は POSN/DISE のみ set（ESM 由来）。

## 3. 残点3: slot[6] 値4 の型コード対応語と +0x34 書手 — **[確定]**

### 3.1 値4 = `kMagicType_Ability`（訂正: 「Vampirism 系」説は棄却）

- **公開 RE 仕様（CEF `API/Magic/Magic.h`・`Magic::MagicTypes`）**: `kMagicType_Spell=0 / Disease=1 / Power=2 / LesserPower=3 / **Ability=4** / Poison=5 / Enchantment=6 / AlchemyItem=7 / Ingredient=8`。「these correspond to (perhaps are precisely) the type codes of magic items」明記。
- **三重照合**: (a) 6/7/8 がエンジン実測の定数スタブ（Enchant=6/Alchemy=7/Ingredient=8、v198/v201）と一致、(b) UESP SPEL ページの SPIT Type 仕様「0=Spell / 1=Disease / 2=Power / 3=Lesser Power / **4=Ability** / 5=Poison」と一致、(c) 項目4 で type==1→DISE（Disease）を実測。
- → **slot[6] 値4 の厳密名 = Ability（常時発動能力）** [確定]。v201 §4.2 の「"VAMP" fourcc 照合＝Vampirism 系 disease 型 spell [有力]」は**訂正**: vampirism は **Ability**（型4）として実装されており、`FUN_005239c0` の型4→"VAMP" 照合（「対象が既に vampirism Ability を持つか」）は Ability 帰属と矛盾しない。

### 3.2 ESM ロード時書手 = **一括ブロック読出**（フィールド別 dword store は存在しない）

- **書手本体 = `FUN_0041B2D0` = SpellItem::LoadForm**（SpellItem primary vtable `0xA34C3C` slot7(+0x1C)＝CEF の `virtual bool LoadForm(TESFile&)` 順）。asm 全文実測（`%TEMP%\opencode\v216_15_out.txt`）:
  ```
  0x41B2E8  push edi; call 0x451210            ; base TESForm::LoadForm
  loop: 0x41B310  call 0x4510B0(file) → eax = 次サブレコード fourcc
    'EDID'(0x44494445) / 'EFID'(0x44494645) → FUN_004154B0 で効果リスト([edi+0x24])
    'FULL'(0x4C4C5546) → TESFullName 読み（sub オブジェクト経由）
    その他（含 'SPIT'）→ 0x41B3C1 へ:
  0x41B3C1  mov ecx, esi(=edi+0x18 = 埋め込み MagicItem sub)
            sub->vtable[0x3C/4=15] (file, chunkType)   ; = LoadBases（game 版 no-op: 0x60CF60 ret 8）
  0x41B3D3  eax = sub->vtable[0x24/4=9]()               ; = ExtraDataChunkType() → 'SPIT'
  0x41B3D5  cmp [chunk], eax / jne 次へ
  0x41B3E1  eax = sub->vtable[0x2C/4=11]() → 0x10       ; = ExtraDataSize()
  0x41B3ED  eax = sub->vtable[0x28/4=10]() → sub+0x1C   ; = ExtraDataPtr()
  0x41B3F0  call 0x981630 (memset(sub+0x1C, 0, 0x10))
  0x41B40E  call 0x450C20(file, sub+0x1C, 0x10)         ; ★ SPIT 16B を sub+0x1C へ一括読出
  ```
- **`ExtraDataPtr()` = `0x419000: lea eax,[ecx+0x1C]; ret`**（sub+0x1C = **SpellItem+0x34**）・**`ExtraDataSize()=0x8C4830: mov eax,0x10; ret`**（SPIT は 16B）・**`ExtraDataChunkType()=0x41D250: mov eax,0x54495053('SPIT'); ret`**（実測）。→ **SPIT 先頭 dword（type）がストック read で sub+0x1C=SpellItem+0x34 に直接着地する**。これで v201 の「FUN_00447DF0 の dword-store 0 件」の理由が**解明**: 型コードの書手は `mov [reg+0x34],imm/reg` ではなく **`FUN_0041B40E` の 16 バイト・ブロック read** である。
- **SPIT 布局（実測＋仕様）**: `+0x1C type(MagicType dword) / +0x20 SpellCost dword / +0x24 SpellLevel dword / +0x28 Flags dword`（コピーヘルパ `0x41D2D0` が +0x1C/+0x20/+0x24/+0x28 を対応複写、比較ヘルパ `0x41D260` が同 4 フィールドを比較＝vtable s12/s13 = CompareExtraData/CopyExtraData）。ctor `FUN_0041D420` の `[esi+0x34]=0` と整合。
- **vtable 0xA34BF4（16-slot 埋め込み MagicItem sub）の実装突合**（CEF の仮想関数順と完全一致）: s6=0x4D9B40 `GetMagicType`→`[ecx+0x1C]` / s9=0x41D250 / s10=0x419000 / s11=0x8C4830 / s12=0x41D260 / s13=0x41D2D0 / s15=0x60CF60。

## 4. 残点4: FUN_00419B30 type==1 → "DISE" 分岐の意味論 — **[確定]**

### 4.1 全文分岐（asm＋decompiled 1:1 実測、0x419B30..0x419C46・ret 4）

```
type = this->vt[6]();                       ; GetMagicType()（MagicItem 系 16-slot）
if (type == 5 /*kMagicType_Poison*/)            → return FUN_00416870(0x4E534F50 'POSN');
if (type == 7 /*kMagicType_AlchemyItem*/ && FUN_00414EB0(this+0xC) != 0)
                                              → return FUN_00416870('POSN');   ; 全 effect Hostile = 毒ポーション
if (type == 1 /*kMagicType_Disease*/)           → return FUN_00416870(0x45534944 'DISE');  ★ 本分岐
else: this+0xC の EffectItemList を走査:
  - elem = [node+4]（`add edi,-4` 歩行）、mgef = elem->effect（[elem+0x1C]）
  - **mgefFlags bit22 (kMgefFlag_Disabled) 立ち → 素通し除外**
  - [elem+0x10](range) 最大を採用、同値は **rand()（0x9828C0）でランダム**
  - 選中 elem の mgefCode == 'SEFF'(0x46464553) → FUN_00412CB0（script の fxMgefCode）を FUN_00416870 で解決
  - 否则 → elem->effect（EffectSetting*）をそのまま返す
```

### 4.2 "DISE" と 0xB33508 レジストリの正体

- **`FUN_00416870` = MGEF レジストリ検索**: asm 実測 `mov ecx,0xB33508 / call 0x55E000`（NiTMap ベース Lookup、key=mgefCode）→ **0xB33508 = EffectSettingCollection（エンジン唯一の mgefCode キー MGEF マップ）** [確定]。CEF: "EffectSettings are not tracked through the DataHandler... they have their own map, indexed by 'magic effect code'"。LookupByCodeString は「先頭4文字を byte-reversed integer として解釈」＝ fourcc キー運用の公式仕様。
- **"DISE"/"POSN" = メタエフェクト**: CEF の `kMgefFlag_UnknownO` コメント「**POSN,DISE - these effects have \*only\* this bit set**, perhaps a flag for 'meta' effects」＋ OME の `EffectSetting::diseaseVFXEffect / poisonVFXEffect` キャッシュ実装。＝視覚（VFX）用の専用 MGEF（実プレイ効果なし）。
- **FUN_00419B30 の命名**: **「MagicItem の代表エフェクト設定（GetRepresentativeEffectSetting）選択器」** — 毒アイテム→POSN、疾病→DISE、通常→「Disabled でない最強（同強度は無作為）」effect。callers **28 件**（raw E8 実測: 0x419CF5/0x419D96/0x419E57/0x419F5E/0x41A64F/0x41ABCF＋magic/戦闘/適用帯 0x5ED675..0x6A085B）が返り値の `[+0x1C]`（TESFullName name）と `[+0x20]`（school word）を読み、名称表示・コスト検証・適用に使う（`FUN_00419CF0`/`0x419E50` 等の兄弟述語も同様）。
- **本タスクのスコープ（EXE 側分岐の解明）はこれで閉じた**。V209（rDebugTextColor）との主題重複は「fourcc/文字列の実測照合手法」のみで、内容は無関係。

## 5. GT ドラフト（反映は要承認・本タスクでは未編集）

### (A) GT「`FUN_004133C0/0x4133E0` = base bit7 ガード付き formID getter/setter」行

- before: `base フォーム flags bit7 ガード付き formID getter（base=[this+0x1C]、[base+0x58] bit7 立ちなら 0、否则 [this+0xC]=formID）`
- after: **`EffectItem::GetDuration()/SetDuration()`（V216）**: `this`=EffectItem（+0x0C=duration / +0x1C=EffectSetting* / +0x20=cost）、`[base+0x58]`=**MGEF mgefFlags**、bit7=**kMgefFlag_NoDuration** 立ちなら duration=0／setter 拒否＋cost=-1.0f 無効化。formID とは無関係（+0x18 の scriptInfo 分岐は IsHostile 系）

### (B) GT「`FUN_00413470` = 毒判定基準（base+0x58 bit0）」行

- after: **`EffectItem::IsHostile()`（V216）**: `[this+0x18]`(ScriptEffectInfo*) 有 → `[+0x14]` hostile byte、無 → `[effect+0x58] & kMgefFlag_Hostile(0x1)`。毒に**Hostile 効果のみ使用可**（OBSE doc と一致）

### (C) GT「`base+0x58` bit7/bit22/bit31 名称 [要深掘]」→ 確定行

- after: **+0x58 = EffectSetting::mgefFlags（MGEF DATA Flags・ゲーム +0x58）**。bit0=**Hostile** / bit7=**NoDuration** / bit21=**PCHasEffect**（known-MGEF、runtime set/clear 双対）/ bit22=**Disabled**（CEF "ignored"。vanilla 未使用・EffectItemList ループが無視・casting 不可＝v201 実測と逐語一致）/ bit23=**UnknownO**（POSN/DISE メタエフェクトのみ）/ bit30=**MagnitudeIsLevel** / bit31=**MagnitudeIsFeet**。TESForm.formFlags(+0x8) とは別物（ Deleted=0x20 等は +0x8 側）。出典: CEF `API/Magic/EffectSetting.h`・`API/Magic/Magic.h`・`API/Magic/EffectItem.h`

### (D) GT「slot[6]=GetMagicItemType・値4=？」行

- after: **値4 = `kMagicType_Ability`**（CEF Magic::MagicTypes＝UESP SPIT 仕様と一致。1=Disease / 5=Poison / 6=Enchantment / 7=Alchemy / 8=Ingredient）。SpellItem+0x34 への ESM 書手 = **`SpellItem::LoadForm 0x41B2D0` の `0x41B40E call 0x450C20`（SPIT 16B 一括読出 → sub+0x1C=SpellItem+0x34）**、ExtraData 契約: ChunkType='SPIT'(0x41D250) / Ptr=sub+0x1C(0x419000) / Size=0x10(0x8C4830)

### (E) GT「FUN_00419B30」関連行

- after: **`FUN_00419B30` = MagicItem 代表エフェクト選択**: Poison(5) or 全 Hostile Alchemy(7) → **POSN**、**Disease(1) → DISE**、否则 Disabled(bit22) 除外の range 最大・同値 rand、SEFF は script fxMgefCode。`FUN_00416870` = **`[0xB33508]` EffectSettingCollection::LookupByCode(mgefCode)**

## 6. 新規残点

| ID | 内容 | 優先 |
|---|---|---|
| V-216-1 | `FUN_004AE590/0x4AE5A0`（bit30/31述語）の戻り値 {0,3,5} が表示単位 enum（MagnitudeIsLevel/Feet → "level"/"feet" 表記）である件の EXE 側確定（`FUN_004de980` 文脈の名称文字列照合） | 低 |
| V-216-2 | `FUN_00419B30` の arg（push 0 系 callers）の契約と「range 最大・同値 rand」選択の游戏上意味（代表 effect 決定がマジカコスト/名称/適用のどれに必須か）＝callers 28 件の分類 | 低 |
| V-216-3 | `0x450C20`（サブレコード一括 reader）の正式契約（size 引数・超過時挙動）。v177 の「4-byte formID reader」呼称は本タスクで**呼称訂正候補**（汎用ブロック reader） | 中 |
| V-216-4 | ENCH（ENCH fourcc の ExtraData='ENIT'?）/ALCH/INGR の ExtraDataChunkType fourcc・ExtraDataPtr オフセット・Size の実測（SpellItem と同構造か） | 中 |
| V-216-5 | `kMgefFlag_UnknownO`(bit23) の EXE 側 reader（メタエフェクト判定）の所在（POSN/DISE 専用経路が 0x419B30/0x417Axx 以外にあるか） | 低 |
| V-216-6 | v193b の「bit31=負センチネル」解釈は MGEF flags では不成立（MagnitudeIsFeet）。bit31 の別系統利用（`FUN_004AE5A0` の callers 5 件の実体）を再分類 | 中 |

## 7. 自己検証・証跡

- **selftest**: 着手時・完成直前とも `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- **内部整合（一致 10 項目）**:
  1. キーワード表A 16 件の token id が FUN_004FF710 の switch case 群（0x10..0x1F、0x1A/0x1B 除外）と過不足なく一致。表C/B の id 列（0x100..、0x1000..）と default 分岐のテーブルベース（0xB0B420/0xB0C8C0）・stride 0x28 が一致。
  2. 'Short' の別名 'int'、'ScriptName'→'scn'、'Reference'→'ref' は .rdata 実読（0xA51B94..0xA51C1C 帯）。
  3. mgefFlags +0x58（CEF ゲーム布局）と EXE の `[elem+0x1C]→[+0x58]` デリファレンスが 1:1。`knownEffectSetting+0x58 |= 0x200000`（v193b L15470）= kMgefFlag_PCHasEffect と一致。
  4. kMgefFlag_Disabled の CEF コメント（list ループが無視・uncastable・vanilla 未使用）が v201 の 3 実測（毒素通し/選択除外/busy拒否）＋161 MGEF 0 件と逐語整合。
  5. Magic::MagicTypes の 6/7/8 が v198 の定数スタブ実測と一致 → 4=Ability の帰結が閉じる。UESP SPIT 仕様（0..5）とも一致。
  6. SPIT ブロック reader: ExtraDataChunkType='SPIT' / Ptr=sub+0x1C / Size=0x10 を vtable 実装から実読み。SpellItem+0x34=sub+0x1C の対応は v201 の ctor 実測（0x41D46D）と一致。
  7. FUN_00416870 が `mov ecx,0xB33508; call 0x55E000`（asm 実測）＝ v201_20 の「0xB33508=MGEF レジストリ」裁定と二重一致。0xB33508 refs 18 件（.text）。
  8. FUN_00419B30 の type 値 5/7/1 が Magic::MagicTypes（Poison/AlchemyItem/Disease）と一致。`FUN_00414EB0` 呼び出し（type7 時）は v201 の毒判定チェッカと同一。
  9. bit22 マスクの reader（shr 0x16）が FUN_00419B30 内にも存在（0x419BC7）→ Disabled 除外が本文ループと一致。
  10. 制約遵守: 成果物は本ファイルのみ（新規1件）。docs/・GT・TODO・session-memory・plan・cache・ソース・既存 tmp/re は**編集 0 件**。ESM 本体**未アクセス**。web 検索は GECK/UESP/公開 RE ヘッダ（CEF/OME）資料のみ。
- **証跡（すべて `%TEMP%\opencode/`・本タスク生成）**: `v216_1.py`（switch case 列挙＋小文字キーワード走査=0件の記録）/ `v216_2.py`（FUN_004FF710 decompiled 全文）/ `v216_3.py`（+0x410 書手スキャン）/ `v216_4.py`（トークナイザ帯）/ `v216_5.py`（FUN_004FCF10=行読取器と判明）/ `v216_6..8.py`（FUN_004FD7C0/0x4FD0A0/0x4FCA70）/ **`v216_9.py`（表A/B/C 実ダンプ＝項目1 の核心証拠）** / `v216_10.py`（DISE/SPIT/VAMP/POSN fourcc refs＋FUN_00419B30 全文）/ `v216_11.py`（FUN_00416870＋0xB33508 refs）/ `v216_13/14.py`（0x41D250 帯 raw＋整列 disasm）/ **`v216_15.py`（SpellItem vtable→LoadForm 0x41B2D0 全文）** / **`v216_16.py`（sub vtable 実装: 'SPIT'/sub+0x1C/0x10）** / `v216_17.py`（FUN_00419B30 callers 28 件）＋対応 `v216_*_out.txt`。
- **資料（web・読取のみ）**: UESP `Oblivion_Mod:Mod_File_Format`（record flags 表）/ `…/MGEF`（DATA flags）/ `…/SPEL`（SPIT Type 0..5）/ OBSE command doc（"Only hostile effects may be in a poison"）/ GitHub JRoush `Common-Oblivion-Engine-Framework`（TESForm.h / EffectSetting.h / EffectItem.h / MagicItem.h / **Magic.h＝MagicTypes**）/ JRoush `Oblivion-Magic-Extender`（Source/OBME/EffectSetting.h＝フラグシフト表・EffectItem.h）。
