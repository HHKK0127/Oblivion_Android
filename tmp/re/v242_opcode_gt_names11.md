# v242_opcode_gt_names11.md — opcode/GT 殘点 第11束（V-231-1..8）

- 作成日: 2026-11-02 / 担当: TODO serial 235 (V235・中) spawning / 分析: サブエージェント（前回実行はサーバ障害で中断・v242 はゼロから再実行）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v231_opcode_gt_names10.md` §9 新規残点 **V-231-1..8**
- 契約: 分析専任（docs/GT/TODO・cache・ソース編集禁止・既存 tmp/re 上書き禁止）。成果物は本ファイルのみ（新規）
- 手法: capstone 5.0.7（v172_common.py 経由・セクション個別変換）＋ MSVC RTTI デコード（v227_lib.py）＋ `docs/Oblivion_decompiled.c` 部分検索（読取のみ）
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）
- 中間物: `%TEMP%\opencode\v242_*.py`（v235_*.py は参照のみ）

（項ごとに即書込みで進める。以下、§1..§8 が確定順に埋まる）

## 1. 【V-231-1】MGEF DATA の 7 formID フィールドの意味と解決パス — **[解明]**

### 1.1 解決パスの所在: `FUN_00415960` = EffectSetting vtable **+0x6C** の「ロード後 formID 解決（finalize）」

- **発端**: `FUN_0044F3D0`（DataHandler::LoadFiles 本体）は全ファイル読み込み後、DataHandler の全フォームリスト（+0x44/+0x4C/+0x5C/+0x7C/+0x84/+0x8C/+0x94/+0x9C/+0xA4/+0xAC/+0xB4 の各リンク＋+0xC4 配列＋+0xBC リスト＋世界系 `FUN_004B2460` 列挙）の**全フォームに対して仮想スロット `+0x6C`（vt[27]）を1件ずつ呼ぶ**（decompiled.c:60555-60680 実測）。runtime 登録 MGEF（map 0xB33508 系）には `FUN_00416900` が同 vt[+0x6C] を呼ぶ（decompiled.c:15085-15110・`mov eax,[edi]; mov eax,[eax+0x6c]; call eax`）。
- **EffectSetting の vt[+0x6C] = `FUN_00415960`**（vtable 0xA32B14+0x6C 実測＝RTTI `.?AVEffectSetting@@`）。中間物 `v242_e/f_out.txt`。
- 逐語フロー（`FUN_00415960`）:
  ```
  if (formFlags(+8) & 8) return                    ; bit3 = "解決済/スキップ"
  if ((mgefFlags & 0x70000) && !(mgefFlags & 0x180000)) ; bits16-18 立ち & bits19-20 無し
      [+0x60] ← formID のみリマップ（FUN_0046BB20(&+0x60, id)・ポインタ化しない）
  for f in [+0x70, +0x80, +0x84, +0x88, +0x8C, +0x78, +0x7C]:   ; 非0のときのみ
      id  = FUN_0046B680(this, -1)      ; フォームの保留 formID ノード取得（+0x10 帯）
      FUN_0046BB20(&local, id)          ; mod-index リマップ（[0xB33B00]+0x18 bit17 で経路分岐）
      p   = FUN_0046B250(local)         ; グローバル formID→TESForm* マップ 0xB0613C の lookup
      f   = _RTDynamicCast(p, 0, &TD_TESForm(0xB02F9C), &TD_<型>, 0)  ; 型付きキャスト
      [f] = 結果                       ; 型不一致は NULL へ
  ```
- ⇒ **ReadRecord（生読み）→ ロード後 finalize（`FUN_00415960`）で formID→TESForm\* に変換**、という2段階。v231 §4.6 の「解決パス未特定」はこれで解消。

### 1.2 各フィールドの意味（dynamic_cast 先 TD が決定打）

finalize が各フィールドに割り当てる **TypeDescriptor が各フィールドの期待型**（`v242_e/f_out.txt` で TD 名を実取得）:

| オフセット | DATA | TD（finalize の cast 先） | 語義 | 補助証拠 |
|---|---|---|---|---|
| **+0x70** | +0x18 | 0xB03518 = **`.?AVTESObjectLIGH@@`** | **Casting Light（詠唱/付与時ライトフォーム）** | `FUN_005EDCE0`/`FUN_00602A70`: `[+0x70]` 非0判定 → `[+0x70]+0xC`（formID）→ `FUN_0069E250(formID, magicNode)` で 0x1C バイトのライト追従オブジェクトをキャスト時に生成し `[caster+0x60]`/`[caster+0x18]` へ格納（decompiled.c:355446/368042） |
| **+0x78** | +0x20 | 0xB034E0 = **`.?AVTESEffectShader@@`** | **EffectShader フォーム①**（v221/v227「+0x78 = アクティブエフェクト instance+0x34 複写」は当該フォームポインタの複写として整合） | ActiveEffect+0x34 へ複写＝shader 適用先フィールド |
| **+0x7C** | +0x24 | 0xB034E0 = **`.?AVTESEffectShader@@`** | **EffectShader フォーム②** | `FUN_006575B0` @0x6576AA: `[effect+0x7C]` を `this->vt[0x420](form)` へ渡す（＝shader セッタ）／`FUN_0060C940` @0x60CBB4: `[effect+0x7C]` を `new(0x4C)`+`FUN_006A0980`（MagicShaderHitEffect 系 ctor）の引数へ |
| **+0x80** | +0x28 | 0xB03500 = **`.?AVTESSound@@`** | **サウンド①（キャスト/使用時サウンド）** | `FUN_005ED5A0`/`FUN_005F3E00`: 非0必須ゲート → `[+0x80]+0xC`（formID）→ `owner->vt[0x330](0, formID)`（サウンド再生系）と `FUN_00663520(formID)` |
| **+0x84** | +0x2C | 0xB03500 = TESSound\* | **サウンド②** | finalize で同型 cast |
| **+0x88** | +0x30 | 0xB03500 = TESSound\* | **サウンド③** | 同上 |
| **+0x8C** | +0x34 | 0xB03500 = TESSound\* | **サウンド④** | 同上 |

- **+0x60（ActorValue 欄）の二重性格を finalize が明示**: `(mgefFlags & 0x70000) && !(mgefFlags & 0x180000)` のとき **+0x60 は formID としてリマップのみ**（ポインタ化しない）。`FUN_0041CAC0`（bit17 群同士の比較）が `FUN_004471A0(+0x60)` で都度解決するのと整合（＝bits16-18=ASSOCIATED フォーム指定時のみ +0x60 がフォームID）。
- **＋0x74/+0x90/+0x94 は非フォーム**（finalize 対象外・float）。+0x90/+0x94 の既定値グローバル 0xB33704/0xB3370C の imm32 参照は ctor 0x416514/0x416522 のみ＋初期化帯 0x9DA747/0xA178F1（CRT 初期化バンド）＝起動時に設定される BSS float。

### 1.3 残点（本項目内）

- `FUN_0046BB20` の byte 級経路分岐（`[0xB33B00]+0x18` bit17＝セーブゲーム/ESM 空間の formID リマップ切替）の全容は未逐語（機構は確定・詳細は低）。
- +0x80..+0x8C の TESSound 4 フィールドの各々の UI 名（Casting/Enchant/Hit/Bolt サウンド等の対応順）は EXE 内文字列未取得（未解明・低）。

## 2. 【V-231-2】ENIT cast type の由来と [weap+0x90]==4 / [book+0x88]&1 — **[解明]**

### 2.1 cast type は ESM 由来（ENIT 生読み）／runtime 付与は {2,3} のみ — **確定**

- **ESM 側**: ENIT 16B は ReadRecord `FUN_0041B2D0` が tag 一致→memset→`file->Read` する**生読み**（v231 §5.2 継承）＝ **ENIT dword0(+0x34)=cast type は ESM フィールド**。ctor 既定: EnchantmentItem `FUN_00419570` = **2** / SpellItem = **0**（v231 §5.2）。
- **runtime 側（新規発見）**: 付与時の設定元は **`FUN_005D4EE0`**（E8 caller は `FUN_005D5470` の1件のみ）。逐語実測（`v242_n_out.txt`）:
  ```
  edi = FUN_00447DF0(formType)            ; 対象と同型の新規フォーム生成（型別工場）
  edi->vt[0xB4](元 form)                  ; フォーム内容複写
  FUN_0044D950([0xB33A98], edi)           ; DataHandler 登録
  isArmor = (byte[edi+4]==0x14 ARMO || ==0x16 CLOT) → [esp+0x17]
  TESFullName cast → 名称を [esi+0x74] から複写
  TESEnchantableForm cast（TD 0xB03718）→ 失敗時は 0x5D5110 へ
  new(0x44) + FUN_00419570 → ebx = 新 EnchantmentItem
  for node in [esi+0x28] リスト（先頭 [this+0x7c]）:
      eax = [node+0x10];  isArmor → eax==0 のみ採用 / 非 armor → eax∈{1,2} のみ採用
      採用時: new(0x24)+FUN_004149A0(node) を FUN_00414B90(ebx+0x24, …) で ENCH 効果リストへ追加
  ecx = isArmor ? 3 : 2        ; xor ecx,ecx / cmp [esp+0x17],cl / setne cl / add ecx,2
  [ebx+0x34] = ecx             ; ★ 0x5D509C = cast type の runtime 設定
  [TESEnchantableForm+8] = word [esi+0x7c]   ; ANAM（チャージ）を対象アイテムへ
  FUN_0044D950([0xB33A98], ebx)             ; ENCH を DataHandler へ登録
  ```
- ⇒ **runtime が作る cast type は {2, 3} のみ**（非 armor 武器/矢→2、ARMO/CLOT→3）。**0（書物/scroll 系）と 1（杖）は runtime で作られない＝ESM（CS 作成時）専用**。
- **効果ノード検査 `node+0x10` = `EffectItem::range`（0=Self / 1=Touch / 2=Aimed）**（v229 §2-2 の EffectItem 布局 `+0x10 range` と FUN_004149A0 が node から 0x24B の EffectItem を複写する実装が決定打）⇒ **armor は Self のみ採用／武器は Touch・Aimed のみ採用＝Oblivion の付与規則（防具=恒久・武器=接触/狙い）と完全一致**。
- 型複製を伴うもう1つの runtime ENCH creator `FUN_005A1390`（caller `FUN_00587D20`）は **+0x34 への型調整なし＝ctor 既定 2 維持**。
- **検証経路（ロード時）**: enchantable item の finalize（vt[0x6C]）= BOOK `FUN_004B53E0` / WEAP `FUN_004BB120` / その他 `FUN_004A9000`・`FUN_004B4C20`・`FUN_004B5C60` が **`FUN_0046A880` → `FUN_004190C0`（CanApplyTo）** を呼ぶ（エラー 0xA3B98B 経路）。＝**ESM の cast type はロード確定時に妥当性検証される**。

### 2.2 [weap+0x90]==4 = **Staff**（weapon type byte・ESM DATA 由来） — **確定**

- **ESM 由来を実証**: WEAP DATA 読出 = ReadRecord `FUN_004BB150` の 'DATA' 分岐 @0x4BB1D1: `push 0x10; lea eax,[esi+0x90]; call FUN_0046BDA0` ＝ **16B 生読みの byte0**。書出 = WriteRecord `FUN_004BB000`: `push 0x10; lea eax,[esi+0x90]; call FUN_0046BBB0`。
- **有効値検証** @0x4BB300（v204 §4 継承）: `cmp byte [esi+0x90], 6; jl ok` ／否则 `"Clearing invalid type on weapon '%s' (%08X)."` を出力して 0 クリア ⇒ 有効値 **0..5**。
- **enum（v204 §4 確定寄りを本タスクで再確認）**: `0=BladeOneHand / 1=BladeTwoHand / 2=BluntOneHand / 3=BluntTwoHand / 4=Staff / 5=Bow`。整合証拠: `FUN_004E1900` が type==5 のときアタッチコード 0xE＋`FUN_006FF420("Bow")`、否则 9（v204 証拠2）／`s58_craft.md`「`[weapon+0x90]==4`（杖）→ `sPoisonStaffsNotAllowedMessage`」／`s57_combat_full.md` `0x5477F0` の `weaponType==5`（弓）が Marksman 枝／`s64_ai_combat.md`「weaponType==5 の弓＝矢在庫チェック」。
- ⇒ **CanApplyTo の `[weap+0x90]==4` ＝「杖（Staff）」**。cast type 対応表（v231 (U) を確定に昇格）:

  | target | 条件 | cast type |
  |---|---|---|
  | WEAP | type==4（**Staff**） | **1** |
  | WEAP | type∈{0,1,2,3,5}（Blade/Blunt/**Bow**） | **2** |
  | AMMO | — | **2** |
  | ARMO/CLOT | — | **3** |
  | BOOK | `[+0x88]&1` | **0** |

- **+0x90 の書手一覧**（`v242_g/j_out.txt`）: ReadRecord 既定 0（0x4BB327）／`FUN_00473EB0`・`FUN_00476D10`（=0xFF）／`FUN_008CD4E0`（=0/1）。**runtime による type 再計算は存在せず ESM DATA byte0 が正** ＝ v231 §5.4「値4の書手未発見＝ファイル DATA 由来」を**確定**。

### 2.3 [book+0x88]&1 = **BOOK DATA flags bit0**（ESM 由来・「使用可能＝scroll 系」述語） — **確定（GECK 正式名は未照合）**

- **ESM 由来を実証**: BOOK DATA 読出 = ReadRecord `FUN_004B5440` の 'DATA' 分岐 @0x4B54C1: `push 2; lea eax,[esi+0x88]; call FUN_0046BDA0` ＝ **2B 生読み**（+0x88=flags lo / +0x89=flags hi）。書出 = WriteRecord `FUN_004B5260`: `push 2; lea eax,[esi+0x88]; call FUN_0046BBB0`。ctor `FUN_004B59F0`: `word [+0x88]=0`、`byte [+0x89]=0xFF`。
- **読手12件**（decompiled.c 実測・`v242_k`/本タスク grep）:
  - `FUN_004190C0`（CanApplyTo BOOK 分岐: bit0 立ちが **cast type 0 の必要条件**）
  - **`byte[+0x88]&1 && dword[+0x64]!=0` ペア述語11件**: `FUN_00595F30` / `FUN_00596020` / `FUN_005962C0` / `FUN_00597D00` / `FUN_005A9C40` / `FUN_005B2B70` / `FUN_005C1A70` / `FUN_005C1DD0` / `FUN_005C1F70` / `FUN_005E4440`（+0x2D0 アイテムアクション・v121 継承） / `FUN_005E96E0`
  - `+0x64` = **ENAM（付与フォーム）**（v177 追補＋本タスク 2.1 実測: ENAM u32→+0x64 / ANAM u16→+0x68）⇒ ペア述語 = **「付与持ち＋使用可能フラグ」＝ scroll としての使用可否**。
- ⇒ bit0 は **「（付与持ちの本を）使用可能＝scroll として機能する」フラグ**で **ESM 由来**（runtime 書手は finalize の +0x89=0xFF リセットのみ）。
- 付随: finalize `FUN_004B53E0` は `byte[+0x89]` を `FUN_00565BE0(2, v)` の返値で **0x0C..0x21（formType 範囲）** 検証し範囲外なら 0xFF へ戻す（+0x89 の語義は §2.4 残点）。

### 2.4 残点（本項目内）

- GECK の BOOK DATA bit0 の正式チェックボックス名（EXE 内文字列なし・未照合・低）。
- `[book+0x89]`（DATA byte1）の語義 — finalize の formType 範囲検証（0x0C..0x21）の対象。低。
- `FUN_005A1390`（2 つ目の runtime ENCH creator・caller `FUN_00587D20`＝メニュ系 object factory 帯）の機能名（型調整なし＝既定2）。低。

## 3. 【V-231-3】ENIT dword1/dword2/byte3・ALCH ENIT {u32,u8} — **[大幅進展（ENIT 3 フィールド名確定／ALCH は残点維持）]**

### 3.1 ENIT/SPIT 16B のフィールド名 — **確定（v216/v229 の SPIT 契約を ENCH へ適用）**

同一アクセスサ（ChunkType `FUN_00419730`='ENIT'/'SPIT'・Ptr `FUN_00419000`=`lea [ecx+0x1C]`・Size `FUN_008C4830`=0x10、v231 §5.1 継承）で読まれる **16B は {dword, dword, dword, byte}**。SPIT 側の布局は v216 で**実測＋仕様照合**済み（`v216` L114: ExtraDataPtr=sub+0x1C / コピーヘルパ `0x41D2D0` が sub+0x1C/+0x20/+0x24/+0x28 を複写 / UESP SPEL 仕様）、v229 §2-2 が関係図化:

| オフセット | フィールド名 | SPIT（SpellItem） | ENCH（EnchantmentItem） | ctor 既定 |
|---|---|---|---|---|
| **+0x34** | **type** | **MagicType 0..8**（0=Spell/1=Disease/2=Power/3=LesserPower?/4=Ability/5=Poison/6=Enchantment/7=Alchemy/8=Ingredient・v216/v198/v201 三重照合） | **cast type 0..3**（v231 (U) 確定: 0=書物/1=杖/2=武器・矢/3=防具） | SPIT=0 / ENCH=2 |
| **+0x38** | **SpellCost**（dword） | **マジカコスト** | 付与コスト（同構造） | **-1**（未設定） |
| **+0x3C** | **SpellLevel**（dword） | スペルレベル | 同構造 | SPIT=**0** / ENCH=**-1** |
| **+0x40** | **Flags**（byte／実体は dword） | フラグ | 同構造 | **0** |

- **注記**: 同一オフセットでも**クラス毎に解釈が異なる**（SPIT=MagicType／ENCH=cast type）。MagicType 6=Enchantment は `EnchantmentItem::GetMagicType` 系の**定数スタブ**（v198/v201）由来で、ENIT field の 2（cast type）とは別物（矛盾しない）。
- ESM 由来: 16B 生読み（v231 §5.2）＝**全フィールド ESM 由来**。runtime 書手は ctor/clone/reset のみ: `FUN_00419570`(ENCH ctor) / `FUN_00418E90`（**ENIT リセット関数**: +0x34=2 / +0x38・+0x3C へレジスタ値 / `mov byte [+0x40],0` @0x418EF9＝magic band 内の唯一の byte3 書手）/ `FUN_0041D420`・`FUN_0041D590`(SPIT 系: `+0x38=0` バリアント @0x41D590) ＋汎用 form-copy `FUN_00441970`/`FUN_00446D80`/`FUN_004492E0`/`FUN_0045B300`/`FUN_00466E80`（+0x38/+0x3C を同時に複写＝オーバーライド/セーブ複写系）。

### 3.2 compare/copy の登録形態 — **確定（vtable 登録・E8 なし）**

- `FUN_00419010`（ENCH 4 フィールド compare）→ **.rdata 0xA33C6C**、`FUN_00419060`（copy）→ **0xA33C70** ＝ **EnchantmentItem 副 vtable `0xA33C3C` の slot12/13**（imm32_refs3 実値 10697836/10697840。v221 の「Compare/Copy（s12/s13）」と完全一致）。
- `FUN_00412A00`（ALCH dword+byte compare）→ **0xA32164**、`FUN_00412A40`（copy）→ **0xA32168** ＝ **AlchemyItem 副 vtable `0xA32134` の slot12/13**（10690916/10690920）。
- **E8 呼び出し 0 件＝純粋 virtual dispatch**（オーバーライド/等価判定経路）。
- 訂正: 初出の手動 `.rdata` 走査はセクション変換 base を .text 用（0x400C00）で誤用し **-0x200 オフ**（0xA33A6C 等）だった → 本表は imm32_refs3 の正値に訂正（§10 訂正まとめ (4)）。

### 3.3 ALCH/INGR ENIT {u32=-1, u8=0} — **[残点維持（低）]**

- 帯内（0x412000..0x414800）の `mov r32,[r32+0x78]` は **compare/copy 本体のみ**（0x412A22/0x412A62）、`mov [r32+0x78],r32` 書手は 0 件、外部 consumer は特定できず。
- ⇒ u32(+0x78)/u8(+0x7C) の**意味は未解明のまま**。次アクション候補: (a) UESP `Mod_File_Format` の ALCH/INGR ENIT 仕様照合（v216 と同手法）、(b) `FUN_00412BE0`（ALCH ctor）以外の +0x78 全書手走査（disp32 形含む）。

### 3.4 残点（本項目内）

- ENCH 側の **SpellCost/SpellLevel/Flags の EXE 消費者**（付与コスト表示・自動計算経路）未特定 — magic band 書手は ctor/clone/copy のみで計算/表示経路が見当たらない（低）。
- SPIT Flags byte(+0x40) のビット定義（低）。
- ALCH/INGR {u32,u8} の意味（§3.3・低）。

## 4. 【V-231-4】FUN_004DE980 enum 0..11 と [base+0x64] 述語 — **[解明]**

### 4.1 同定: `TESObjectREFR` の「操作カテゴリ（activate category）」取得

- **this = 参照オブジェクト（TESObjectREFR）**。冒頭 `mov eax,[esi+8]; shr eax,0xD; test al,1; jne → return 0` ＝ **formFlags(+8) bit13 が立っていれば 0**（非表示/無効 ref）。
- 続いて `edx=[esi]; eax=[edx+0x170]; call eax` ＝ **`this->vt[0x170]()` = baseForm**（vtable byte offset なので slot 0x5C）。直後に `switch (byte[base+4])`＝**baseForm の formType** で分岐。
- **`[this+0x1C]` も同じ baseForm**（`param_1[7]` として全ケースで直接参照。かつ 0x4DEB8B の LIGH 経路で `[esi+0x1C]+0x7C` を読む）⇒ **TESObjectREFR::baseForm = +0x1C**（v229/v231 の既存認識と一致）。
- 分岐表は **2段**: `byte table @0x4DEBD0`（0x1A=26 エントリ＝formType 0x12..0x2B の索引）→ `dword table @0x4DEBA8`（10 ターゲット）。範囲外（formType <0x12 または >0x2B）は default=0。

### 4.2 formType → 戻り値の全対応（`v242x_s4b_out.txt` 実測・26/26）

formType のクラス名は **各 form の ctor 内 `mov byte [reg+4], imm` と同一関数内の主 vtable ストア → RTTI デコード**で確定（`v242x_s4e/s4j_out.txt`。0x1F のみ ctor の imm 直書きが拾えず**補間**）。

| formType | クラス（RTTI 実測） | ctor | 戻り値 | 条件 |
|---|---|---|---|---|
| **0x12** | `TESObjectACTI` | `FUN_004B40A0` | **4 / 0** | `FUN_004D9890(base)!=0`（=TESFullName 名の長さ ≠0）→ **4**、無名→0 |
| 0x13 | `TESObjectAPPA` | `FUN_004B4910` | 1 | — |
| 0x14 | `TESObjectARMO` | `FUN_004B5140` | 1 | — |
| **0x15** | `TESObjectBOOK` | `FUN_004B59F0` | **1 / 6** | **`[base+0x64]!=0` → 1、==0 → 6**（4.3） |
| 0x16 | `TESObjectCLOT` | `FUN_004B6150` | 1 | — |
| **0x17** | `TESObjectCONT` | `FUN_004B6BE0` | **2** | — |
| **0x18** | `TESObjectDOOR` | `FUN_004B8550` | **8** | — |
| 0x19 | `IngredientItem` | `FUN_004198F0` | 1 | — |
| **0x1A** | `TESObjectLIGH` | `FUN_004B1F90` | **1 / 0** | `([base+0x7C]>>1)&1` が立つ→1、落ちる→0 |
| 0x1B | `TESObjectMISC` | `FUN_004B9860` | 1 | — |
| 0x1C | `TESObjectSTAT` | `FUN_004B9AA0` | **0**（default） | — |
| 0x1D | `TESGrass` | `FUN_004AF040` | **0** | — |
| 0x1E | `TESObjectTREE` | `FUN_004B9DA0` | **0** | — |
| 0x1F | `TESFlora`（**補間**・後述） | 未特定 | 1 | — |
| **0x20** | `TESFurniture` | `FUN_004AE660` | **3 / 5 / 0** | `FUN_004AE590`=MNAM **bit30**→**3**／bit30 落ち & `FUN_004AE5A0`=**bit31**→**5**／両方落ち→0 |
| 0x21 | `TESObjectWEAP` | `FUN_004BB610` | 1 | — |
| 0x22 | `TESAmmo` | `FUN_004A9430` | 1 | — |
| **0x23** | `TESNPC` | `FUN_00527220` | 0/2/11/10/7/1 | 4.4 のアクターブロック |
| **0x24** | `TESCreature` | `FUN_0051EB80` | 0/2/7/9/1 | 4.4 のアクターブロック |
| 0x25 | `TESLevCreature` | `FUN_004AF620` | **0** | — |
| 0x26 | `TESSoulGem` | `FUN_004BBD10` | 1 | — |
| 0x27 | `TESKey` | `FUN_004AF4E0` | 1 | — |
| 0x28 | `AlchemyItem` | `FUN_00412BE0` | 1 | — |
| 0x29 | `TESSubSpace` | `FUN_004BCA00` | **0** | — |
| 0x2A | `TESSigilStone` | `FUN_004BB8A0` | **0** | — |
| 0x2B | `TESLevItem` | `FUN_004B00B0` | 1 | — |

- **0x1F の補間根拠**: `mov byte [reg+4], imm` 走査で 0x1F を拾うのは `FUN_0042A1A0` の 1 件のみだが、これは **`ExtraPackage` の extra type（`[eax+4]=0x1F`・vtable 0xA35914）** で formType ではない（同帯 0x429xxx-0x42Axxx は BSExtraData 系の extra type を同じ +4 に置くため、この走査は formType と extra type を混同しうる）。0x1F は **0x1E=`TESObjectTREE` と 0x20=`TESFurniture` の RTTI 実測値に挟まれた唯一の未使用値**で、既知の TES4 formType 表（FLOR）と整合。
- なお **16 個の formType が 1**（APPA/ARMO/CLOT/INGR/MISC/FLOR/WEAP/AMMO/SLGM/KEYM/ALCH/LVLI）＝「汎用アイテム」、**0 が 5 個**（STAT/GRAS/TREE/LVLC/SBSP/SGST＝静的・景観・レベル付き生成元・亜空間・星座石）。

### 4.3 `[base+0x64]` 述語 = **BOOK の ENAM（付与 enchantment）有無**

- 逐語（0x4DE9BA）: `eax=[base+0x64]; neg eax; sbb eax,eax; and eax,0xFFFFFFFB; add eax,6` ⇒ `[base+0x64]!=0 → 1` / `==0 → 6`（`-(x!=0) & ~4` = 0 or -5、+6 で 6 or 1）。
- **`[book+0x64]` = TESEnchantableForm の ENAM**（v231 §2.3: ENAM u32→+0x64 / ANAM u16→+0x68）。v231 §2.3 で列挙した 11 件のペア述語 `byte[book+0x88]&1 && dword[book+0x64]!=0` と**同一フィールド**。
- ⇒ **付与（ENAM）付きの本 = 1（汎用アイテム＝使用/取得）／付与なしの本 = 6（読む）**。cast type 判定（§2.1）で BOOK の cast type 0 が要求されるのは `[+0x88]&1` が立つ場合であり、**ENAM の有無は cast type ではなく本カテゴリ（6→1）を切り替える**、と整理できる（両者は独立フィールド）。
- 併走証拠: 0x1A LIGH の `[base+0x7C] bit1` も「持ち運び可」（LIGH DATA flags bit1 = Can be carried）と読める同型の「取得可能」述語。ただし LIGH の +0x7C は本タスクで書手未走査（残点）。

### 4.4 アクターブロック（formType 0x23 TESNPC / 0x24 TESCreature）

```
共通前段: edi = this->vt[0x190]() ? this : NULL          ; vt[0x190] = 「アクターか」
          if (edi == Player[0xB333C4]) return 0
          if (this->vt[0x198](0)) {                       ; vt[0x198](arg0)
              if (edi->[0xB0] != 6) return 2              ; FUN_005E0DC0 = [actor+0xB0]
          }                                               ; ⇒ 死体/戦闘不能 → 2（=開く/漁る）
NPC(0x23):
          if (edi && FUN_005E0F30(edi) && FUN_005E04C0(Player)) return 11
              ; FUN_005E0F30(ref): [ref+0x58]->vt[0x36c]() == 9
              ; FUN_005E04C0(actor): actor->vt[0x284](0x45) > 0
          if (FUN_005E0550(Player)) return 1
              ; Player[+0x58]->vt[0x2C0]() の bit10(0x400) 立ち && bit11(0x800) 落ち
          if (edi && FUN_005E1D30(edi)) return 10
              ; FUN_005E1D30(ref): baseForm(+0x170) の [+0x28] bit1
          return 7
CREA(0x24):
          if (this->vt[0x198](0)) return 2
          if (!FUN_004D74D0(this)) goto L_LIGH7C        ; 0x7C bit1 ? 1 : 0
              ; FUN_004D74D0: [this+0x1C]!=0 && formType==0x24 && [base+0x104]==4
          edi = this->vt[0x190]() ? this : NULL
          r = edi->vt[0x388]()                           ; 騎手/乗用者
          if (!r) return 9
          if (r == Player) return 0
          if (r->vt[0x198](0)) return 0
          if (r->[0x58] == 0) return 0
          if (r->[0x58]->vt[0x36c]() == 4) return 7
          if (r->[0x58]->vt[0x36c]() != 0) goto L_LIGH7C
          return 9
```
- **`FUN_005E0550` の bit10/bit11** は Player プロセスの dword フラグ（`vt[0x2C0]()` 戻り値の word 部）。**bit10 が立ち bit11 が落ちている**とき＝特定のプレイヤ状態で NPC が **1**（汎用）へ落ちる、という抑制経路。
- **`FUN_004D74D0` の `[base+0x104]==4`** は TESCreature のクリーチャ種別バイト（==4）。これが真のときだけ「騎乗」判定（`vt[0x388]` = 騎手）へ進む。⇒ **0x24 CREA の 7/9 分岐は「馬（creatureType==4）＋騎手」専用**。

### 4.5 enum の実体 — **HUD tile 0xFB3 に float で渡される操作種別コード**

- **E8 呼び出し元は 2 件のみ**（再走査・確定）: `FUN_005A4980` @0x5A4ADB と `FUN_005A8C90` @0x5A8CAE。**v231 §2.1 の表は「FUN_004DE980 の callers」と「FUN_005A4980 の callers」を混同している**（`FUN_005A5540`=HUDInfoMenu vt[0x2C] / `FUN_00671620`=PlayerCharacter vt[0x228] / `FUN_005ADCF0`=loading_menu / `FUN_005806D0`・`FUN_00583E60` はいずれも **FUN_005A4980 の** 呼び出し元／その近傍であって、`FUN_004DE980` を直接呼ばない）。訂正候補（GT ドラフト §9 に記載）。
- **`FUN_005A8C90`（E8 caller 0 件＝死コード）が enum の唯一の数値消費者**:
  ```
  iVar2 = FUN_004DE980(ref)
  ... 7/10 と 1 の抑制判定の後 ...
  FUN_005A8BC0(iVar2)                       ; tile 更新
  ```
  `FUN_005A8BC0(cat)` は `FUN_0058CEB0(0xFB2,1.0)` / `(0xFB2,0xA379B4)` / **`FUN_0058CEB0(0xFB3,(float)cat)`** / `FUN_0058CED0(0xFAE,0)` / `FUN_0058CEB0(0xFAF,1.0)` を順に実行 ⇒ **enum は tile 0xFB3 に float で書かれる「照準対象の操作種別コード」**。tile 0xFB3 は HUD の操作アイコン index と解せる（文字列テーブルで enum→名前を引く箇所は EXE 内に無い）。
- `FUN_005A4980`（生きた消費者）は enum の **{3,5} だけ** を使う: `cat==5 || cat==3` → `FUN_004DBAE0(form, Player->vt[0x174](1,1,&a,&b), 1, 1)` で**家具マーカーを探索**し、失敗なら即 return（＝座る/眠る位置が無ければラベルを出さない）。v231 §2.2 の「category 3/5 で家具系」を**述語レベルで裏付け**。

### 4.6 enum 0..11 の裁定

| 値 | 産出元 | 裁定語義 | 確度 |
|---|---|---|---|
| **0** | default（STAT/GRAS/TREE/LVLC/SBSP/SGST）／無名 ACTI／LIGH 非可搬／FURN 両ビット落ち／Player・騎手=Player・死体でない等 | **対象外（プロンプト非表示）** | 高 |
| **1** | APPA/ARMO/CLOT/INGR/MISC/FLOR/WEAP/AMMO/SLGM/KEYM/ALCH/LVLI ＋ BOOK(ENAM有) ＋ LIGH(可搬) ＋ NPC の Player 状態抑制時 | **汎用アイテム（取得/使用）** | 高 |
| **2** | CONT ＋ 死亡した NPC/CREA（`vt[0x198](0)` かつ `[+0xB0]!=6`） | **開く（コンテナ・死体を漁る）** | 高 |
| **3** | FURN の MNAM **bit30** | **家具: 座る**（bit30 = sitting・v231 §1.2） | 高 |
| **4** | ACTI かつ名前長≠0 | **名前付きアクティベータ** | 高 |
| **5** | FURN の MNAM **bit31** | **家具: 眠る**（bit31 = sleeping・v231 §1.2） | 高 |
| **6** | BOOK かつ ENAM==0 | **読む（付与なしの本）** | 高 |
| **7** | NPC の既定／CREA(馬) で騎手プロセス `vt[0x36c]()==4` | **アクター（対話系の既定）** | 中 |
| **8** | DOOR | **ドア（開閉）** | 高 |
| **9** | CREA(馬) で騎手なし／騎手プロセス `vt[0x36c]()==0` | **乗用可能な馬（騎手なし）** | 中 |
| **10** | NPC かつ `FUN_005E1D30`（baseForm[+0x28] bit1） | **アクター（サブ状態A）** | 中 |
| **11** | NPC かつ `FUN_005E0F30`（プロセス `vt[0x36c]()==9`）かつ `FUN_005E04C0(Player)`（`vt[0x284](0x45)>0`） | **アクター（サブ状態B）** | 中 |
| — | 未確定 | 7/9/10/11 の**正式な操作名**（Talk/Pickpocket/Steal 等の割当）は enum→文字列の対応表が EXE に無く未確定 | 低 |

### 4.7 残点（本項目内）

- 7/9/10/11 の正式名称（`FUN_005E04C0` の `vt[0x284](0x45)`＝ActorValue index 0x45 の意味、`[actor+0xB0]`＝`FUN_005E0DC0` のフィールド名、`vt[0x198](bool)` の意味）。低。
- `vt[0x36c]`（プロセス側）の返値 0/4/9 の意味（§6 の `FUN_00654D00` type コードと同一体系の可能性）。→ §6 で再訪。
- `[base+0x7C]`（LIGH）bit1 の書手未走査。低。

## 5. 【V-231-5】FUN_00678750 の this クラスと slot 8/0x17C/0x380/0x384 — **[解明]**

### 5.1 `this` の実体 = **静的非多態シングルトン `0xB3BD00`**（クラス名なし・サブシステム同定は「家具/ベッド使用の進行役」）

- **全 4 callers が `mov ecx, 0xB3BD00` の即値渡し**（実測・`v242x_s6b_out.txt`）:

  | call site | caller | 直列 |
  |---|---|---|
  | 0x4458B9 | `FUN_004455E0` | `mov ecx,0xB3BD00; call FUN_00677360` → `mov ecx,esi; call FUN_00441610` → **`mov ecx,0xB3BD00; call FUN_00678750`** |
  | 0x445D2F | `FUN_00445A10` | 同型（`FUN_00677360` → `FUN_00441610` → **本関数**） |
  | 0x66FB43 | `FUN_0066F420` | `mov ecx,0xB3BD00; call FUN_00676A40` → `mov ecx,[0xB333A0]; call FUN_00441610` → **本関数** |
  | 0x444DFB | `FUN_00444C70` | 同帯（0x444DCB 付近にジャンプテーブルがあり線形逆アセンブル不能。imm32 参照 0x444DF7/0x444E01/0x444E0B で `0xB3BD00` 使用を確認） |

- **`C7 05 <0xB3BD00> imm32`（vtable ストア）は .text 全体で 0 件** ⇒ このオブジェクトは**多態でない**（vtable を持たない静的構造体）。`0xB3BD00` は .bss（raw 非バック）＝初期値はすべて 0。
- `this+0x68`（= **0xB3BD68**）は冒頭 `add ecx,0x68; call FUN_007616D0` で取得。**`FUN_007616D0` = `mov eax,ecx; ret`（恒等 getter）** ⇒ `this+0x68` は「サブ構造体の先頭アドレス」を返すだけ。
- 同帯の兄弟メソッドも同じ `this+0x68` を使う: `FUN_00677360`（`add ecx,0x68; call FUN_007616D0` → リスト走査）／`FUN_00676A40`（`call FUN_00673A50` → `call FUN_007616D0`）。
- **`this+0x68` の実体 = `{REFR* ref; node* next;} ` の単方向リスト**（`FUN_00677360`: `edi=[this+0x68]; loop { esi=[edi]; if (esi && esi->vt[0x190]() && !esi->vt[0x198](0)) FUN_005EB370(esi); edi=[edi+4]; }`）。
- **機能同定 = 「家具（sit/sleep）使用の実行」**: 本体が `FUN_004AE5A0`(FURN MNAM **bit31**=sleep) / `FUN_004AE590`(**bit30**=sit) で候補家具を判定 → `FUN_004D73F0`（marker index）→ `FUN_004DB9D0`（marker 受入）→ `FUN_004AEB40`/`FUN_004AEBE0`（FurnitureMarker の位置・角度）→ `FUN_00659B90`/`FUN_0065A2C0`/`FUN_0065AC20`（actor の配置）→ 最後に **`[process]->vt[0x370](actor, type, furnitureForm, marker)`** で型コードを確定（§6）。
- **末尾**: `mov ecx,[0xB333A0]; jmp FUN_004418A0` ＝ プレイヤ側マネージャ（`+0x80` リストヘッド／`+0x84` を `FUN_00401F20` で解放し `+0x80=0`）のクリアを**テールコール**。`FUN_004418A0` は `[esi+0x84]` のリンクリストを全解放して `[esi+0x80]=0` にするだけの関数。

### 5.2 slot 8 / 0x17C / 0x380 / 0x384 は **`this` ではなく `[ref+0x58]`（プロセス）の vtable スロット**

v231 §3 の `[x+0x58] = LowProcess*`（実行時は HighProcess が vtable を差し替え）を踏襲。**4 スロットすべて `[actor+0x58]` 上の仮想呼び出し**であることを逐語で確認（`[this+…]` ではない）。

| slot（vtable byte offset） | HighProcess(0xA71814) | LowProcess(0xA720A4) | 実装の実体（実測） | 本関数での使われ方 |
|---|---|---|---|---|
| **+8**（3番目） | `0x6F7070` = `xor eax,eax; ret`（**常に 0**） | `0x73FD50`（`FUN_0073FB80` 内・`0xB401C8`/`0x729D00`/`0x70F910` を触る実装） | High=空スタブ／Low=実装 | `if (process->vt[+8]() != 0) → この候補を飛ばして次のノードへ`（0x6787D7-0x6787E2）。**HighProcess では常に 0 なのでゲートは開いたまま** |
| **0x17C** | `0x64F760` | `0x60D020` = `mov [ecx+4],arg; ret 4` | **セッタ**: arg を `[proc+4]`（`[proc+0xC0]!=0` のときは `[proc+0xCC]`）へ格納 | `process->vt[0x17C](1)` を 2 箇所（0x67891C / 0x678C53）で呼ぶ＝**「家具使用中」フラグのセット** |
| **0x380** | `0x64B0A0` = `lea eax,[ecx+0x128]; ret` | `0x6F7070`（常に 0） | **`&process->+0x128` を返す getter**（High のみ） | `ebx = process->vt[0x380]()`（0x67884A）→ 以降 marker 座標計算の作業領域として使う |
| **0x384** | `0x64B0B0`（`arg1->vt[0x164]()` と `FUN_00521450(arg1, [this+0x120])` を呼ぶ） | `0x6FE080` = `xor al,al; ret 4`（**常に false**） | **述語**: 「家具使用を受理できるか」 | `if (process->vt[0x384](actor)) { ... type=9 or 4 ... } else { type=0x7F（失敗/解除） }`（0x678C98-0x678CDB） |
| （参考）+0x36C | `0x64B090` = `movzx eax, byte[ecx+0x11D]; ret` | `0x6F7070` | **`[proc+0x11D]` の getter**＝§6 の型コード読み出し | §4 の `[actor+0x58]->vt[0x36c]()`（0/4/9 比較）と同一 |
| （参考）+0x370 | `0x654D00`（§6） | `0x5E1850`（`ret 0x10` スタブ） | **型コードのセッタ**（`[proc+0x11D]=type`, `[proc+0x120]=data`, `[proc+0x124]=sub`） | 本関数の最終段で type ∈ {1, 6, 4, 9, 0x7F} を渡す |

- **重要**: v231 §3.3 の `[this+0x17C]`（`FUN_00654D00` 内・**データフィールド**）と、本節の `vt[0x17C]`（**vtable スロット**）は**別物**。同じ数値だが前者はプロセス構造体の +0x17C オフセット、後者は vtable の 0x17C 番地。以後「`[proc+0x17C]`（データ）」／「`vt[0x17C]`（仮想）」と書き分ける。

### 5.3 FUN_00678750 が渡す型コード（§6 への橋渡し）

```
最終段（0x678C34-0x678CE7）:
  bl = 候補が sleep(bit31) 由来か
  if (bl) process->vt[0x370](actor, 6, furnForm, marker)    ; 6 = 眠る
  else    process->vt[0x370](actor, 1, furnForm, marker)    ; 1 = 座る
  process->vt[0x17C](1)
  process->vt[0x80](actor, 0)                               ; 0x80 = 別スロット（解除系）
  r = process->vt[0x384](actor)
  if (r) { if (bl) { ...; process->vt[0x370](actor, 9, furnForm, marker) }   ; 9 = 睡眠成立
           else    { process->vt[0x370](actor, 4, furnForm, marker) } }     ; 4 = 着席成立
  else     { process->vt[0x370](actor, 0x7F, 0, 0) }                        ; 0x7F = 解除/失敗
```
⇒ **型コード 1/6/4/9/0x7F の発生源はここ**（§6 の「全意味」の一次証拠）。v231 の「旧観測値 1/6/0x7F」は**中間状態**（座る要求=1／眠る要求=6／失敗=0x7F）で、**成立後は 4（座る）/9（眠る）に遷移する**。

### 5.4 残点（本項目内）

- `0xB3BD00` のクラス名（vtable 無し・RTTI 無し・静的 .bss のため**名前を EXE から取得できない**）。同定は「`+0x68` に actor リンクリストを持つ家具使用の進行役」まで。
- `vt[+8]`（vtable 3番目）の LowProcess 実装 `FUN_0073FB80` の意味（`0xB401C8`＝NiNode/シーン系グローバルを触る）。低。
- `vt[0x80]`（本関数が `(actor,0)` で呼ぶ）の実装は未走査。低。

## 6. 【V-231-6】FUN_00654D00 の type コード全意味・0x88CE30 の正体 — **[解明]**

### 6.1 シグネチャと副作用（`vt[0x370]` = HighProcess 実装）

```
FUN_00654D00(this = process, arg1 = 対象 actor, arg2 = type(int), arg3 = 家具フォーム, arg4 = marker index(byte))
  ebp = arg1（= [esp+0xc]）
  if (!arg1) goto tail
  edi = arg1->vt[0x154]() ; 非0必須（actor の 3D/基底データ）
  if (arg1 == Player[0xB333C4]) goto tail
  f1 = ([proc+0x11D] ∈ {9,4})                      ; 直前の type
  f2 = (type ∈ {9,4})                              ; bl
    ── 0x88CE30(obj, a, b, c) の発火（2 経路）──
    if (proc->vt[+8]() == 0 && !f1 && !f2)   → 0x88CE30(edi, 0, 1, 0)
    if (proc->vt[+8]() == 0 && f1 && f2 && arg1->vt[0x380]() == 0) → 0x88CE30(edi, 1, 1, 0)
  ── BackWeapon / Quiver の表示切替 ──
  node = [proc+0x17C]; if (!node) goto tail
  if (type ∈ [3,5])  {  for k in {[0xB06560]="BackWeapon", [0xB06568]="Quiver"}:
                          p = node->[+0x98]->[+0x7C]->vt[+0x4C](k)   ; 名前で NiAVObject を引く
                          if (p) word[p+0x18] |= 1                   ; 非表示
  } else             { 同上で word[p+0x18] &= 0xFFFE                  ; 表示
  }
tail:
  if (arg1) { f = FUN_004A9720(arg1); FUN_0065A650(arg1, f); }
  [proc+0x11D] = (byte)type          ; ★ 現在の操作種別
  [proc+0x120] = arg3                ; ★ 家具フォーム
  [proc+0x124] = (byte)arg4          ; ★ marker index（0x7F = 無効）
```

- **`[0xB06560] = "BackWeapon"` / `[0xB06568] = "Quiver"`**（.rdata 文字列 0xA3CC24 / 0xA3CC10 を実読）⇒ `node->[+0x98]->[+0x7C]` は **NiNode（3D モデル）**、`vt[+0x4C]` は**名前による子ノード検索（GetObjectByName 相当）**、`word[+0x18]` は **NiAVObject のフラグ**（bit0 = 非表示/APP_CULLED）。
- ⇒ **`type ∧ {3,4,5}` = 「背中の武器と矢筒を隠す」フェーズ**。それ以外は表示に戻す。
- **E8 呼び出し 0 件**（再確認）＝純粋な仮想ディスパッチ。`vt[0x370]` の**呼び出しサイトは .text 全体で 21 箇所**（`8B 8x 70 03 00 00` 走査・`v242x_s6c_out.txt`）。

### 6.2 type コードの全集合と発生源（21 サイトの引数実測）

**引数順の確定**: `push arg4 → arg3 → arg2 → arg1` の順に積まれ、callee は `arg2` を type として `[esp+0x18]` から読む。`arg3` → `[proc+0x120]`、`arg4`（byte） → `[proc+0x124]`。

| type | 発生サイト（代表） | 文脈（実測） | 裁定 |
|---|---|---|---|
| **0** | 0x678CD5 / 0x650187 / 0x6519B3 / 0x6552EA / 0x63AB81 | いずれも `(type=0, form=0, marker=0x7F)` の3点セット。0x6552EA は直前に `mov byte [ebp+0xc4],1` | **解除（家具使用の終了）** |
| **1** | 0x678C4B（`FUN_00678750`・sit 枝）／0x6501EF（`FUN_00650030`・`bl==0` 枝）／0x6501E7 | marker 有効・家具フォーム付き | **着席 要求**（dynamic idle の再生要求） |
| **2** | 0x650257（同・`bl==0` かつ `vt[0x384]` 失敗） | 直前に `"Missing furniture dynamic idle for marker %d in the editor's idle manager."`(0xA72C28) を出力 | **着席 失敗**（idle 未定義） |
| **3** | 0x650415（`FUN_00650030`・`bl==0`・位置確定後） | `actor->vt[0x1CC](&proc->+0x128)` の後 | **着席 完了（位置確定）** |
| **4** | 0x678C4B 系（`FUN_00678750`・`vt[0x384]` 成功・sit）／0x650592（`FUN_00650030`）／0x6533F9／0x655923（`FUN_00655590`） | — | **着席 体位確定**（`0x88CE30` 発火対象） |
| **5** | 0x655011（`FUN_00654F10`・`0xA` 枝の対偶） | 家具基底の marker type を `FUN_004AE5D0`(type<10) で分類した結果で分岐 | **睡眠系 体位確定**（`0xA` と対）※中確度 |
| **6** | 0x678C49（`FUN_00678750`・sleep 枝・bit31）／0x6501CF | marker 有効 | **睡眠 要求** |
| **7** | 0x650237（`bl!=0` かつ `vt[0x384]` 失敗） | 同上エラー文字列の後 | **睡眠 失敗** |
| **8** | 0x6503F1（`bl!=0`・位置確定後） | — | **睡眠 完了（位置確定）** |
| **9** | 0x678C4B 系（`vt[0x384]` 成功・sleep）／0x650574（`FUN_00650030`・`FUN_005E12B0(actor)->vt[0x9C](1,0)` の後） | — | **睡眠 体位確定**（`0x88CE30` 発火対象） |
| **0xA** | 0x654FF1（`FUN_00654F10`・`FUN_005E12B0(actor)->vt[0x9C](0,0)` の後） | — | **睡眠系 遷移**（`5` の前段）※中確度 |
| **0x7F** | 0x678CD5 / 0x65018D / 0x6519B9 / 0x654F40 / 0x63AB87 | **type ではなく marker index の sentinel**（`arg4` 側）。旧 v231「type 0x7F」は**誤読** | **marker 無効** |

- ⇒ **v231 §3.3 の「type 1/6/0x7F」は 3 つとも「要求コード(1/6) + marker sentinel(0x7F)」の混在**だった。実際の type 空間は **0..0xA の 11 値**。
- **1→4（着席）／6→9（睡眠）が「要求 → 体位確定」の対**、**2/7 が失敗**、**3/8 が位置確定**、**0 が解除**。
- `[proc+0x11D]` は `vt[0x36C]`（HighProcess `0x64B090` = `movzx eax,byte[ecx+0x11D]`）で読める ⇒ **§4 の `[actor+0x58]->vt[0x36c]()==4 / ==9 / ==0` は「そのアクターが今どの家具操作フェーズにいるか」の照会**だった（4=着席、9=睡眠、0=なし）。§4 の 7/9/10/11 の語義もこれで裏付けられる（馬の 7 = 騎手が着席フェーズ 4）。

### 6.3 `0x88CE30` の正体 = **3D モデル（NiAVObject ツリー）の再帰走査ディスパッチャ**

```
FUN_0088CE30(obj, A, B, C):                ; 4 引数（ret 無し・bool 返し）
  if (!obj) return false
  if (C == 0) {                             ; C = arg4
      x = FUN_006FA970(obj)                 ; "BSX"(0xA7D0EC) をキーに 0xB3F484 レジストリを引く
      if (!x) return false
      if ((x->[0xC] & 2) == 0) return false
  }
  params = { 0, B(byte), 0xD, (A != 0) }    ; 16B スタック構造体
  if ([0xB2E334] != 0) FUN_0088A7D0(obj, &params, [0xB2E334])
  return true
```
- **`[0xB2E334] = 0x0088AB60`**（.data 実読）。`FUN_0088A7D0` は **`[node+0xB0]`（子配列）を `word[node+0xB6]`（子数）だけ再帰的に辿り、各ノードでコールバック `arg3(node, params)` を呼ぶ**（0x88A849 で自分自身を再帰呼び出し）＝**シーングラフ走査器**。
- `FUN_004A05E0(ref)` = `return [ref+0xA8]` ＝ **TESObjectREFR の 3D モデル（NiAVObject\*）**。`FUN_0088A7D0` は `word[node+0x18]` を `&0xFFEF | 6` に更新（bit4 クリア・bit1/2 セット）＝ **NiAVObject フラグ操作**。
- ⇒ **`0x88CE30` = 「参照の 3D モデルを走査してノード単位のフラグ/付属物を切り替える」処理**。`FUN_00654D00` からは **type ∈ {4,9}（着席/睡眠の体位確定）でのみ** 呼ばれ、引数は `(model, 0|1, 1, 0)`。**§6.1 の BackWeapon/Quiver 非表示と同じ「家具使用時の見た目切替」の一部**。
- **`params` の `0xD`** はこの走査器のモード値。`B` は `1` 固定（呼び出し側）、`A` は `f1 && f2`（前回も今回も 4/9）で 0/1。

### 6.4 残点（本項目内）

- `type 5 / 0xA` の細分類（`FUN_00654F10` 内の `FUN_004AE5D0` 分類＝marker type<10 の結果による分岐）の完全な対応表。中。
- `0x88AB60` が探す `0xBA7A20` の実体（RTTI 範囲外＝.data 上の vtable 値。型名未取得）。低。
- `[proc+0x17C]`（データ）の設定元と `[+0x98]`→`[+0x7C]` のオブジェクト階層名（NiNode 連鎖であることは確定）。低。

## 7. 【V-231-7】MGEF flags bit24/bit21 とテーブル 0xB03418/0xB034C4 — **[大幅進展（テーブル正体・目的・bit21 確定／bit24 は AV-mode として有力）]**

### 7.1 テーブルの正体 — **「登録値保持 × ファイル値上書き」のビット所有権マスク表**（実測確定）

`v242_5_out.txt` 実読（.data）:

- **table1 `0xB03418` = 21 エントリ × {u32 mask, u8 used}**（stride 8・count は `[0xB034C0]=21`）:
  - mask = bit **{0..13, 15..20, 27}**（**bit14 は表に不在**）
  - **used=1 は bit {10, 11, 12, 13, 27} の5件のみ**（他16件は used=0）
- **table2 `0xB034C4` = 6 × u32**（count は `[0xB034DC]=6`）: mask = bit **{21, 22, 23, 24, 25, 26}**（used フラグ無し＝全件が同扱い）
- **imm32 参照は `FUN_00416100`（ReadRecord）のみ**（refs 実測: 0x4161CD / 0x4161AE / 0x4161F3 / 0x4161E6 / 0x4161C3 の5点に限定）＝**唯一の消費者は MGEF DATA 読出時の正規化**。

### 7.2 アルゴリズム（v231 §4.3 の逐語を確定含めて再掲）と目的裁定

```
old = [+0x58]（読出前の flags・edi へ退避）／ oldAV = [+0x60] ／ oldBit24 = byte[old+3]&1
memset(+0x58, 0, 0x40); [+0x74]=1.0f;  file → [+0x58] へ 0x40B 生読出
for i in table1 (21件):
    if (used[i])  ⇒ ファイル値で set/clear（ファイル所有ビット）
    else          ⇒ old を維持（スキップ＝エンジン所有ビット）
for i in table2 (6件):  ⇒ 常にファイル値で set/clear（bit21..26 は全件ファイル所有）
if (oldBit24)   ⇒ flags |= 0x01000000 して [+0x60] ← oldAV を復元（旧 AV 保護）
flags &= 0xFFDFFFFF                     ⇒ bit21 は毎ロードで常時クリア
```

- ⇒ **正体＝「（CRT 初期化で先に登録された）エンジン定数ビットを ESM 読出で潰さないためのマージ・マスク表」**:
  - **used=0（旧値保持）= bit {0-9, 14, 15-20}** … セマンティクス系フラグ（Hostile(bit0)/NoDuration(bit7) 等の CEF ビット帯）を**エンジン（登録）値優先**で守る。
  - **used=1 + table2（ファイル優先）= bit {10-13, 21-27}** … ESM 側で意味を持つ/異版間で差異が出うるビットのみファイルから採用。
  - **bit21 は毎ロード破棄** → プレイヤ既知（PCHasEffect）はセーブ/ランタイム管理へ完全委譲（v235/v236 の port 判断と一貫）。
- **なぜ「old」が登録値になりうるか**: MGEF 161 件の内蔵登録 `FUN_00417220`（flags 実引数 e.g. `0x1000172`）は **`FUN_00417420` に161コール**され、その呼び出し元は **`FUN_00418DB0` ← `FUN_009D9FA0`（CRT initializer 帯）＝ESM 読込より前に実行**（第2経路 `FUN_00418E50` ← `FUN_00464440` も実測）。効果オブジェクトは読込前に flags 付きで存在し、ReadRecord がその**既存値（old）と ESM 値をマージする**構造と整合。

### 7.3 bit21 / bit24 の裁定

| bit | 裁定 | 根拠 |
|---|---|---|
| **bit21 (0x00200000)** | **`kMgefFlag_PCHasEffect`（player-known・CEF）**。読出後に**必ず `and 0xFFDFFFFF`** で捨て、既知管理は runtime（セーブ）側へ | v216 §2.2 CEF 確定＋ReadRecord 末尾逐語＋v235/v236 整合 |
| **bit24 (0x01000000)** | **「+0x60 を ActorValue として扱う AV-mode マーカ（旧値保護）」[有力]**。old bit24 立ち時のみ **oldAV を [+0x60] へ復元**（file 値で上書きさせない）＋bit24 も再立上 | ReadRecord 逐語（v231 §4.3）＋分布相関（`v242_6`: 161 件中 61 件=38% に set。**無し群 = LOCK/OPEN(0xE0/0xC0・AV=0) / DIAR/DIWE(0x75・AV=0) / SUDG / POSN・DISE(bit23 のみ)** と「AV を持たない効果」に一致） |

- ctor 既定: **`FUN_00416430` が `xor edi,edi`（0x416463）→ `mov [esi+0x58],edi` / `mov [esi+0x60],edi`（0x4164D9/0x4164DC）＝ mgefFlags・AV とも既定 0**（実測）。⇒ fresh オブジェクトへの読出では used=0 ビットは 0 のまま＝**「登録オブジェクトへマージされる」設計が前提**であることの裏返し。
- **built-in 登録 flags の実分布**（`v242_6` 161件）: bit4=88% / bit1=83% / bit8=54% / bit5=48% / bit6=46% / bit24=38% / bit18=25% / bit0=20% / bit2=19% / bit3=14% …。used=1 ビットは登録側でほぼ 0（bit10 のみ1件=FIDG/SHDG/FRDG 帯）＝**「ファイル由来を許すビット」が vanilla 登録で使われない**のはマージ設計と矛盾しない。

### 7.4 残点（本項目内）

- **ESM の MGEF レコードが登録オブジェクトへマージされるファクトリ経路の実証**（`MGEF` fourcc のディスパッチが decompiled.c grep 0件。map `0xB33508`（EffectSettingCollection・v229 §2-2）キー再利用が最有力）— 中。
- bit24 の **CEF/GECK 正式名**（v216 §2.2 の表に bit24 無し・未照合）。低。
- used=1 ビット {10,11,13,13,27} の正式名（表の used バイトが置かれた意図＝バージョン差異か OBME か）。低。
- bit14（table1 に不在＝無条件で old 保持）の意図。低。
- bit24 の runtime 読手（ReadRecord 外・`0x1000000` 一致行は decompiled grep で無関係1件のみ＝`shr`系読走査が残る）。低。

### 7.5 追補（`v242x_s7b`/`s7c` による追加実測 — §7.1-7.4 の裁定を補強）

- **T2 の完全形**: `0xB034C4` = `0x00200000 / 0x00400000 / 0x00800000 / 0x01000000 / 0x02000000 / 0x04000000` ＝ **bit21..bit26 の連続6ビット**（過不足なし）。T1 の `used=1` は bit{10,11,12,13,27}（§7.1 と一致）。
- **静的ブロックの整列**: `0xB03418 + 21*8 = 0xB034C0`（T1 count）→ `0xB034C4`（T2 先頭）→ `0xB034C4 + 6*4 = 0xB034DC`（T2 count）→ `0xB034E0` = **`TESEffectShader` の TD**（§1.2）。3 者が magic band に連続配置され、**T1/T2 の件数も .data の静的値**（`[0xB034C0]=0x15` / `[0xB034DC]=6`）。**件数・used バイトを書き換えるコードは .text に 0 件**＝テーブルは完全に不変。
- **bit24/bit21 を直接テストする命令は .text に 0 件**: 次の2走査で該当なし。
  - `(F7|81) /r [reg+0x58], imm32`（`test`/`or`/`and` の disp8=0x58 形）で imm ∈ {`0x01000000`, `0x00200000`}
  - `mov eax,[reg+0x58]` の直後 7 バイト以内の `test eax, imm32`（`A9`）で同 imm
  ⇒ **bit24 の読み手は正規化ループ自身のみ**、**bit21 は誰も読まない**（＝「常時クリア」は完全な捨てビット）。
- **bit16..20 の実効性に関する注意（§1.2 への波及）**: `FUN_00415960` @0x415975/0x41597C が `test eax,0x70000` / `test eax,0x180000`（`eax=[eset+0x58]`）を行うことは実測どおりだが、**T1 の bit16..20 は `used=0`** なので「**fresh な（=登録オブジェクトでない）EffectSetting** では +0x60 の formID リマップ経路に到達しない」。§7.2 の「登録オブジェクトへマージされる」前提が成り立つ個体でのみ到達しうる、という条件付き命題に読み替える必要がある（§9 (AB) に注記済みの設計前提と表裏）。
- **ctor 既定の再確認**: `FUN_00416430` @0x416463 `xor edi,edi` → @0x4164D9 `mov [esi+0x58],edi` / @0x4164DC `mov [esi+0x60],edi`（flags・AV とも 0）。もう一方の `FUN_00415F10`（vtable 0xA32B14 を書くが 0x415FDA で `ret` する短い関数）は **+0x58 を初期化しない**ため、既定 0 の根拠は `FUN_00416430` の1箇所。

## 8. 【V-231-8】「+0x7C = VFX/new 0x4C/vt0x420」記述の再裁定 — **[解明]**

### 8.1 対象記述（v221/v227）

- v221 L145 表: 「**VFX/関連オブジェクト**（`[ret+0x7C]` ptr → new 0x4C / vt0x420 適用）」（sites: 0x60CBAB(`FUN_0060C940`) / 0x6576A1(`FUN_006575B0`)）
- v227 L92 (W) 候補表: 「+0x7C | +0x24 | — | **ptr 0（VFX/シェーダ解決先候補・new 0x4C/vt0x420 適用＝v221）**」
- 矛盾点（V-231-8 の出発点）: (W) は +0x7C を **TESForm\*（ESM formID → finalize で解決）**と確定済み（本ファイル §1.2）＝「VFX instance」という読みと衝突。

### 8.2 本タスクの実測（2サイト＋RTTI）

**`FUN_006575B0` 窓（0x65769C-0x6576CD・`v242_s_out.txt`）**:
```
push 0; lea ecx,[edi+0x18]; call FUN_00419B30      ; MagicItem(+0x18=sub) の代表エフェクト選択（v216）
test eax,eax; je skip
mov eax,[eax+0x7c]                                  ; ★ 選択結果(EffectSetting*)+0x7C = TESEffectShader*
test eax,eax; je skip
mov edx,[esi]; push eax; mov eax,[edx+0x420]; mov ecx,esi; call eax
                                                    ; this->vt[0x420](shader) = シェーダ適用 setter
mov edx,[esi]; mov eax,[edx+0x41c]; ...             ; 続きに vt[0x41C]、ecx=0xB3BD00; call FUN_00679240
```

**`FUN_0060C940` 窓（0x60CBAB-0x60CC12）**:
```
lea ecx,[eax+0x18]; call FUN_00419B30               ; 同じく代表エフェクト選択
push 0x4C; call operator_new                        ; 0x4C バイト確保
push ecx; push edi; push esi; call FUN_006A0980     ; ctor（引数に選択結果のデータを渡す）
```
- **`FUN_006A0980` = ctor**: `call FUN_0069D8C0`（基底）→ **`mov [esi], 0xA764B4`** → RTTI 実デコード → **`.?AVMagicShaderHitEffect@@`**（＋`[esi+0x3C/0x40/0x48]=0`、`[esi+0x34]=ecx`）。

### 8.3 裁定表

| 要素 | v221/v227 の記述 | 本タスクの裁定 |
|---|---|---|
| **`[ret+0x7C]`** | 「VFX/シェーダ解決先候補 ptr」（field 未確定） | **`EffectSetting+0x7C` ＝ MGEF DATA+0x24 の TESEffectShader\* フォーム**（§1.2: finalize cast TD `0xB034E0=.?AVTESEffectShader@@` ＋ WriteRecord `FUN_00415800` の formID 変換7件に該当。ret＝`FUN_00419B30`=MagicItem 代表エフェクト選択（v216）の返す EffectSetting\*）→ **「フォームポインタ」であって VFX instance ではない** |
| **`new 0x4C`** | 「VFX/関連オブジェクト生成」（+0x7C の instance と読める書き方） | **`MagicShaderHitEffect` の実行時 instance**（vtable 0xA764B4・ctor `FUN_006A0980`・RTTI 確定）。+0x7C の**シェーダフォームを素材に生成される結果オブジェクト**であり「+0x7C 自身の格納先」ではない |
| **`vt[0x420]`** | 「適用」（未同定） | **シェーダ適用 setter 仮想**（shader ptr を1引数で受け取る）。実装候補 = HighProcess(0xA71814)+0x420\*4 = **0xA72894 → 0x629300（`FUN_00629260` 帯）**・`FUN_006575B0` 自身の .rdata 登録は **0xA71C40 / 0xA72AB0** の2箇所（HighProcess 帯／slot 割当は残点） |

### 8.4 結論

- **v221/v227 の「+0x7C = VFX」的表現は3要素の混同**: (1) **+0x7C = シェーダフォーム（TESEffectShader\*・ESM DATA+0x24）**、(2) **new 0x4C = そこから生成する MagicShaderHitEffect instance**、(3) **vt0x420 = それを適用する仮想 setter**。
- v231 (W) の「+0x7C は VFX/new 0x4C 訂正候補」は**確定に昇格**（v227 (W) 記述は本裁定で置換）。

### 8.5 残点（本項目内）

- `FUN_006575B0` の正確な vtable slot 割当（0xA71C40/0xA72AB0 の帰属 vtable 確定）と vt[0x420] 実装（0x629300）の本文逐語。低〜中。
- `MagicShaderHitEffect`（0x4C）のフィールド名（+0x34/+0x3C/+0x40/+0x48 初期化のみ実測）。低。
- `FUN_00419B30` の返値型（EffectSetting\* と断定する根拠は「+0x7C のフィールド同一性」＝§1 決定打。返却文の1点は未逐語）。低。

## 9. GT ドラフト（反映は要承認）

> GT 本体（docs/GT 系・編集禁止）とは**未突合**。以下は v231 §8 の (T)-(AA) を旧状態とした新旧判断＋本タスク（§1-§8）由来の新規候補。反映前に GT 本体と行単位で突合のこと。

### 9.1 既存 (T)-(AA) の裁定

| ID | 旧記述（v231 §8） | 裁定 |
|---|---|---|
| **(T)** formType 行（5件追加） | 0x14/0x15/0x16/0x21/0x22 | **拡張 → 26 formType 全対応表に置換**: 0x12 `TESObjectACTI`(`FUN_004B40A0`) から 0x2B `TESLevItem`(`FUN_004B00B0`) まで ctor 内 `mov byte[+4],imm` ＋主 vtable store → RTTI で実測（§4.2。0x1F=`TESFlora` のみ補間） |
| **(U)** `FUN_004190C0` 行 | cast type 要求値表 | **更新（確定に昇格）**: 条件の実値解釈 — **`[weap+0x90]==4` = Staff**（enum 0..5 = Blade1H/Blunt1H…/Staff/Bow・v204 整合・ESM DATA 16B byte0 生読み）、**`[book+0x88]&1` = BOOK DATA flags bit0**（2B 生読み）。**runtime 付与書手 `FUN_005D4EE0`@0x5D509C を追記**（ARMO/CLOT→3・他→2 の2値のみ／range フィルタ armor=Self・weapon=Touch/Aimed／ANAM チャージ転送／caller `FUN_005D5470`）。検証経路＝enchantable finalize 5件（`FUN_004B53E0`/`FUN_004BB120`/`FUN_004A9000`/`FUN_004B4C20`/`FUN_004B5C60`）→`FUN_0046A880` |
| **(V)** ENIT/SPIT 行 | アクセサ三点セット＋16B/8B 布局 | **更新: 4 フィールド名を確定** — **+0x34 type**（SPIT=MagicType 0..8 / ENCH=cast type 0..3・クラス毎解釈）/ **+0x38 SpellCost**（既定 -1）/ **+0x3C SpellLevel**（ENCH -1・SPIT 0）/ **+0x40 Flags**（byte・既定 0）（v216/v229 SPIT 契約を ENCH へ適用）。compare/copy = 副 vtable **slot12/13**（ENCH 0xA33C6C/0xA33C70・ALCH 0xA32164/0xA32168・E8 0件）。ENIT リセット関数 = `FUN_00418E90` |
| **(W)** EffectSetting フィールド行 | 7 formID ＋「+0x7C=VFX/new 0x4C 訂正候補」 | **確定に昇格＋§8 裁定を追記**: +0x7C = **TESEffectShader\***（フォーム）／**new 0x4C = `MagicShaderHitEffect` instance**（vtable 0xA764B4・ctor `FUN_006A0980`・RTTI 実デコード）／**vt[0x420] = シェーダ適用 setter**。「+0x7C=VFX instance」的読法は**訂正** |
| **(X)** `FUN_00678750` vt[0x370] 行 | Low/High vt0x370 差分＋type{3,4,5}/{4,9} | **更新/置換**: this = **静的非多態シングルトン `0xB3BD00`**（+0x68 = REFR リンクリスト・機能＝家具/ベッド使用の進行役・vtable ストア 0 件）。slot 8/0x17C/0x380/0x384 は **`[ref+0x58]` プロセスの仮想**（実装表 §5.2）。型コード = **着席/睡眠のステートマシン 0..0xA**（1/6=要求 → 4/9=成立、2/7=失敗、3/8=位置確定、0=解除、**0x7F は marker sentinel＝旧「type 0x7F」は誤読**）。`0x88CE30` = **BSX ゲート付きシーングラフ走査器**（`FUN_0088A7D0` 再帰 walker・callback `0x88AB60`・`[0xB2E334]`） |
| **(Y)** bit23 AlarmPackage 行 | — | **維持**（反証なし） |
| **(Z)** school グローバル行 | — | **維持**（反証なし） |
| **(AA)** `FUN_005A4980` 行 | ラベル生成＋callers 所属 | **更新**: `FUN_004DE980` の **E8 は2件のみ**（`FUN_005A4980`/`FUN_005A8C90`。v231 §2.1 の「callers 5件」表は **FUN_005A4980 の callers との混同＝訂正**）。enum の実体 = **HUD tile `0xFB3` へ float で渡される操作種別コード**（`FUN_005A8BC0` 経由・当該 caller は dead）。live で消費する値は **{3,5}=家具マーカー探索ゲート**（`FUN_004DBAE0`）のみ。本カテゴリ 6/1 = **ENAM 有無**（§4.3） |

### 9.2 新規 GT 行ドラフト（本タスク由来）

- **(AB)** GT「MGEF flags 正規化テーブル」行: **`0xB03418`（21×{u32 mask,u8 used}）/ `0xB034C4`（6 mask）= EffectSetting ReadRecord 専用のビット所有権マスク**（imm32 参照は `FUN_00416100` のみ）。used=0（bit0-9,14,15-20）＝**登録値保持**、used=1（bit10-13,27）＋ table2（bit21-26）＝**ESM 値採用**、**bit21 = `kMgefFlag_PCHasEffect` 常時クリア**、**bit24 = 旧 AV（+0x60）保護マーカ [有力]**。内蔵登録161件は **CRT 初期化 `FUN_009D9FA0`→`FUN_00418DB0`→`FUN_00417420`**（ESM 読込前）。ctor 既定 `+0x58=0/+0x60=0`（`xor edi,edi` 実測）。
- **(AC)** GT「runtime enchantment 付与」行: **`FUN_005D4EE0`**（対象型複製 `FUN_00447DF0`＋vt[0xB4] 複写 → `FUN_0044D950` 登録 → EffectItem 複写 `FUN_004149A0`→`FUN_00414B90`(+0x24) → **cast type = ARMO/CLOT→3・他→2** → ANAM チャージ転送）。
- **(AD)** GT「EffectSetting finalize」行: **vt[+0x6C] = `FUN_00415960` ＝ロード後の formID→TESForm\* 解決**（+0x60 は bits16-18 条件付きリマップのみ／7 フィールドを TD キャスト: `0xB03518` LIGH・`0xB034E0` EffectShader×2・`0xB03500` TESSound×4）＝ v231 §4.6 の解消。
- **(AE)** GT「BOOK/WEAP の Enchantable データ」行: **BOOK DATA = 2B 生読み @+0x88**（bit0 = 使用可能/scroll 述語・12 読手、+0x89 = byte1 は finalize が `FUN_00565BE0` で formType 範囲 0x0C..0x21 検証）、**ENAM(u32 formID)→+0x64 / ANAM(u16 charge)→+0x68**（TESEnchantableForm@+0x60・WriteRecord `FUN_0046A7B0`）、**WEAP DATA = 16B 生読み @+0x90**（byte0 = weapon type 0..5・検証 @0x4BB300 "Clearing invalid type on weapon"）。
- **(AF)** GT「MagicShaderHitEffect」行: **`new(0x4C)` / ctor `FUN_006A0980` / vtable `0xA764B4`（`.?AVMagicShaderHitEffect@@`）** — `FUN_00419B30`（代表エフェクト選択）の結果 +0x7C（TESEffectShader\*）を素材に生成・`vt[0x420]` で適用（`FUN_006575B0`/`FUN_0060C940` の2経路）。

## 10. 自己検証・証跡

- **selftest**: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes@0x504050 `518b4c24188b5424` / bytes@0x4FB580 `578b7908` / string@0xA7035C `.\AI\CombatController.cpp` / opid0x0015 impl=0x504560）。**完成直前（§9-§10 書込み前）に再実行 → ALL PASS (4/4)**。
- **8 件の判定**: V-231-1 **[解明]** / V-231-2 **[解明]** / V-231-3 **[大幅進展（ENIT 3 フィールド名確定・ALCH は残点）]** / V-231-4 **[解明]** / V-231-5 **[解明]** / V-231-6 **[解明]** / V-231-7 **[大幅進展（正体・目的確定）]** / V-231-8 **[解明]**。
- **証跡（すべて `%TEMP%\opencode/`・本タスク生成。再実行より出力読取を優先済み）**:
  - **§1（前回中断まで）**: `v242_1..22.py/_out.txt`（e/f=`FUN_00415960` finalize・b=ReadRecord 全文・d=EffectSetting vtable・11/12=TD・5/6=テーブルと161登録・13..18/21/22=BOOK/WEAP 読手と weapon 文字列）
  - **§2**: `v242_k`（WEAP/BOOK の ENAM/ANAM 分岐・WriteRecord tags・`FUN_004B5200`=ARMO 判明・TESEnchantableForm RTTI・callers）/ `v242_l`（ENCH creator 窓・`sEnchant` xrefs・mov[+0x34] disp8 スキャン）/ `v242_m`（disp8 書手全走査443+35件）/ `v242_n`（**`FUN_005D4EE0` 全文・0x5D509C type 設定**・callers・helpers）/ `v242_j`（TD 名・WEAP+0x90 読手・BOOK DATA reader）
  - **§3**: `v242_o`（`cmp [r32+disp],-1` 全走査＝0件と decompiled 帯抽出）/ `v242_p`（`FUN_005A1390` 全文＝setter thunk 判明・compare/copy callers=0・帯読手）/ `v242_q`（副 vtable slot 登録・+0x78/+0x38/+0x3C/+0x40 書手）
  - **§4**: `v242x_s4, s4b..s4l`（26 formType ctor RTTI・アクターブロック逐語・enum 消費/tile 0xFB3）/ `v242_19`（本文全）/ `v242_20`（ジャンプテーブル2種）/ `v242_r`（`FUN_005A4980` 消費窓＝{3,5} ゲート）
  - **§5**: `v242x_s5, s5b..s5e`（0xB3BD00 系統・slot 実装・兄弟メソッド）/ `v242_r`（decompiled slot 行・4 callers 文脈）/ `v242_u`（本体 slot 全 grep＝8/0x17C/0x380/**0x384**/0x370 実測）/ `v242_1`（High/LowProcess slot 実装表）
  - **§6**: `v242x_s6a..s6g`（vt0x370 呼出21サイト・type 引数実測・0x88CE30 周辺）/ `v242_r`（decompiled `+0x370)` 36行・HighProcess slot heads）/ `v242_s`（`FUN_0088CE30` C 本文＋callers 文脈）
  - **§7**: `v242_5`（テーブル実読）/ `v242_6`（161 登録 flags ビット分布）/ `v242_b`（ReadRecord 全文）/ `v242x_s7..s7f`（テーブル raw・2 ctor 比較・**imm32 0x01000000/0x00200000 全 .text 走査＝未フィルタ生リスト**（次アクション入力））
  - **§8**: `v242_s`（`FUN_006575B0`/`FUN_0060C940` 窓・.rdata 登録 2 箇所）/ `v242_t`（**`.?AVMagicShaderHitEffect@@` RTTI 判定**）/ `v242_v, w`（0xB3BD00 の vtable ストア探索＝`C7 05`/`89 05`/`mov reg,imm` 全 0 件を実証→非多態確定）
- **訂正まとめ（本タスクで確定した新旧・ドラフト反映時に GT へ波及）**:
  1. **v231 §2.1「FUN_004DE980 の E8 callers 5件」→ 実 E8 は 2 件のみ**（`FUN_005A4980`/`FUN_005A8C90`）。他4件は `FUN_005A4980` の callers＝表の混同（§4.5）。
  2. **v231 §3.3「type 0x7F」→ 誤読**（arg4 の marker sentinel）。type 空間は **0..0xA**（§6.2）。
  3. **v221 L145 / v227 L92「+0x7C = VFX/new 0x4C」→ +0x7C は TESEffectShader\* フォーム、new 0x4C は `MagicShaderHitEffect` instance**（§8・(W) を確定に）。
  4. **v231 §5.4「`[weap+0x90]==4` の書手未発見」→ ESM DATA byte0 生読みと確定**（§2.2）。**v231 §3.4「0x88CE30 正体未解明」→ BSX ゲート付きシーングラフ走査器**（§6.3）。
  5. **本タスク §3.2 初出の手動 .rdata 走査がセクション base 誤り（-0x200）** → imm32_refs3 正値（0xA33C6C 等）へ訂正。
  6. **中間物 `v242_15`「BOOK band +0x64 書手」→ `FUN_004B5200` は TESObjectARMO ctor 断片**（§4.3 付記）。
  7. v227 (P)「ENIT type enum 1/3 未解明」（v231 (U) で付与先カテゴリ化）→ 本タスクで **Staff(4)→1 ほか実値解釈と runtime 書手まで確定**（§2）。
- **完成後（§4-§10 追記後）の最終 selftest**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（同 4 件）。本ファイルは **§1-§10 全完結・「（作業中）」プレースホルダ 0 件**（最終 62KB 超）。
- **§7.5 追補の証跡**: `v242x_s7b`（T1 21件・T2 6件の raw 実読／正規化ループ 0x4161C0-0x416220 逐語）・`s7c`（`(F7|81) [reg+0x58],imm32` と `mov eax,[reg+0x58];test eax,imm32` の2走査＝bit24/bit21 とも **0 件**）・`s7d`/`s7e`（ctor `FUN_00416430` の +0x58/+0x60 既定 0 実測・`FUN_00415F10` は +0x58 非初期化）・`s7g`（`FUN_00415960` の `0x70000`/`0x180000` 実測）・`s4b..s4l`/`s5b..s5e`/`s6a..s6g`（§4-§6）。
- **遵守事項**: git 書出無し / docs・GT・TODO・session-memory・plan・cache・ソース無編集 / **v242 以外の既存 tmp/re 無上書き**（本ファイルへのみ追記）/ ESM 再スキャン無し / PowerShell ヒアドキュメント無使用（`python -c` は軽量確認のみ・うち正規表現使用1回＝s7b 生リストの帯フィルタ。失敗・歪曲なし）/ .rdata+.data 跨ぎはセクション個別変換（§3.2 の base 誤りを本タスク内で自検出・訂正）。
