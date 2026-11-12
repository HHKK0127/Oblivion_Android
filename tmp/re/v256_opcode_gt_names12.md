# v256_opcode_gt_names12.md — opcode/GT 殘点 第12束（V-242-1..11）

- 作成日: 2026-11-02 / 担当: TODO serial 245 (V245・中) spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v242_opcode_gt_names11.md` の新規残点（§10・各節末）＝ **中4件＋低7件**
  - 中: (1) ESM MGEF→登録オブジェクト マージのファクトリ経路実証＋§1.2 波及 (2) 各 UI caller の enum 値→ラベル書式対応 (3) FUN_00654D00 の type 5/0xA 細分類 (4) FUN_006575B0 の vtable slot 割当と vt[0x420] 実装本文
  - 低: ALCH/INGR ENIT {u32,u8}・BOOK DATA bit0 の GECK 正式名・[book+0x89] 語義・ENCH 側 SpellCost 消費者・bit24 の CEF 正式名と runtime 読手・LIGH +0x7C bit1 書手・formType 表（26件）と GT の行単位突合
- 契約: 分析専任（docs/GT/TODO/plan/session-memory/cache・ソース編集禁止・既存 tmp/re 上書き禁止・訂正はドラフトのみ）
- 手法: capstone 5.0.7（v172_common.py 経由・VA→raw はセクション個別換算＝imm32_refs3 準守）＋ MSVC RTTI（v227_lib.py）＋ `docs/Oblivion_decompiled.c` 部分検索（読取のみ）
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）
- 中間物: `%TEMP%\opencode\v256_*.py`（v242 系の再利用優先・出力は `v256_*_out.txt`）
- 注: TODO serial245 行は成果物名を `v252_…` としていたが、v252-v254 は他束で消費済みのため本タスク指示どおり **v256** に新規作成（GT 反映時に対応する訂正候補を §10 に明記）

## 1. 【中1】ESM の MGEF が登録オブジェクトへマージされるファクトリ経路の実証 — **[確定]** ＋ §1.2 波及検証 **[確定]**

### 1.1 ファクトリ経路の全鎖（実測）

**連鎖**: DataHandler::LoadFiles 本体 `FUN_0044F3D0` → E8→ `FUN_0044F0C0`（@0x44F0C0 内 1 件・レコードループ）→ E8→ **`FUN_0044DCF0` = CreateForm（record→form ディスパッチ・ecx=this=DataHandler、param=file）**。（`FUN_0045BDE0`/`FUN_00461310`/`FUN_004D1340` からも E8 呼び出し計4件）

`FUN_0044DCF0`（decompiled.c:59162〜）内部:
```
ebx = FUN_004511C0(file)                    ; レコードの formType（switch(ebx) の case 値=formType）
prelude @0x44DDA9: esi = FUN_0055E000(0xB0613C map, &formID)   ; 全体 formID→TESForm* マップの既存検索
                   formType 不一致時は警告 0xA38598/0xA385D8 + return（mod 名は 0xB05E04[]）
switch(ebx) — jump table: byte@0x44E6DC(67項)→dword@0x44E6A0(15ターゲット・v256_7 で全 formType→case 対応を実測)
  case 0x0C (MGEF) @0x44E367: ★★ 本残点の経路 ★★
      tag = FUN_004510B0(file)              ; [file+0x250]=現在サブレコード tag
      if (tag == 'EDID'(0x44494445)) {
          FUN_00450C20(file, …)             ; EDID データ読出
          p = FUN_004168A0(EDID 4文字列)    ; ★ code 変換 + マップ検索（下記）
          if (p == 0) {                     ; 未登録コード → fresh 生成
              new(0xA8) → FUN_00416430(ctor) ; flags(+0x58)=0 / AV(+0x60)=0（v242 §7.5 継承）
              FUN_00451210(form, file)       ; formID 割当（FUN_0046C300=SetFormID([file+0x248],1) 等）
              (this+0x8B8).push_front(form)  ; FUN_00446CB0＝単方向リスト前方挿入
          } else {                          ; 既存登録オブジェクト → ★マージ
              FUN_00447050(p, file)         ; ★★ 下記
          }
      }
      return 1
```

**`FUN_004168A0` = EffectSettingCollection::GetSetting（code キー検索）— 確定**:
- 引数=EDID の 4 文字文字列。`strlen>=4` CHECK @0x4168BB の後、4 バイトからキー整数を合成（`b3<<24|b2<<16|b1<<8|b0`＝文字列先頭が最下位。例 "WABR"(57 41 42 52) → **0x52424157** ＝登録側 `push 0x52424157` と**完全一致**・v256_10 実測）。
- @0x4168E8: `mov ecx,0xB33508; lea eax,[esp]; push eax; call FUN_0055E000` ＝ **マップ `0xB33508` の Find**（ FUN_0055E000 は全局 formID マップ `0xB0613C` でも使う汎用二分探索 Find・v250 §N-31 継承）。返り値=既存 `EffectSetting*` or 0。

**`FUN_00447050(form, file)` = 汎用 ReadRecord デリパッチ（マージ実行点）— 確定**（逐語 @0x447050）:
```
esi=arg1(form)／edi=arg2(file)
edx = [form->vtable + 0x1C]        ; ★ vt[+0x1C]
ecx = form; push file; call edx    ; → EffectSetting では vtable 0xA32B14+0x1C = 0xA32B30 実値 = FUN_00416100
                                    ;   ＝ MGEF ReadRecord（used マスク正規化本体・v242 §7.2）
その後: [form+8] bit0 ガード付き FUN_0046A9A0(form, loaded?) → file->FUN_0044FAF0() 真なら form->vt[0x90](1)
```
- ⇒ **この `vt[+0x1C]` 呼出しが「登録済み EffectSetting オブジェクトへの ESM 値マージ」の実行点**。`old=[+0x58]` は CRT 登録値そのもの（v242 §7.2 の「old が登録値になりうる」の機構的実証完了）。
- 対照: generic case（formType 0x04-0x0A/0x0D-0x11/0x2D-0x2F/0x3B/0x3D-0x3F → target 0x44E578、default 0x44E5A5 も同型）は `FUN_00447DF0(formType)`（型別工場）→ `FUN_00447050(form,file)` → `FUN_0044D950(this, form)`（DataHandler 登録）。**MGEF case だけ「type 別 new に加えて code キーの既存検索」を持つ** ＝ マージ設計の実体。

**登録側の挿入（マップ同一性）— 確定**: `FUN_00417220`（内蔵登録 helper・FUN_00417420 から 161 回 E8）末尾 @0x4173DC: `push form(esi); push code(edi); mov ecx,0xB33508; call FUN_00452570` ＝ **同一マップ 0xB33508 への code キー挿入**。GetSetting(Find) と Insert が同じ `0xB33508`/count `0xB3350C`/配列 `0xB33510`（走査形 FUN_004167E0/0x416900/0x416A80 が count+配列を併用）を参照することを確認。

**formType 0x0C = MGEF の裁定根拠**: case 0x0C の生成先は `new(0xA8)+FUN_00416430`（EffectSetting ctor・vtable 0xA32B14 store 実測は v242 §7.5）。**MGEF formType=0x0C は本タスク実測の新規確定**（v242 §4.2 の 0x12..0x2B 帯の外）。参考: case 0x03（同帯外の別 formType）も EDID を読み `DAT_00B35468->vt[0x24](file)` へ渡す別経路（本件と無関係・未追跡）。

### 1.2 §1.2 波及（bit16..20 used=0 → fresh は +0x60 remap 非到達）— **[確定]**

- 前提1（v242 §7.5 実測）: T1 bit16..20 は used=0 ＝ ReadRecord 正規化は**旧値保持**（ファイル値を書かない）。
- 前提2（v242 §7.5 実測）: fresh ctor `FUN_00416430` は +0x58/+0x60 を 0 初期化。
- 前提3（本節実測）: マージ経路が存在するのは **GetSetting ヒット時だけ**。ヒットしない code の fresh オブジェクトは bits16..20 が構造的に 0 のまま。
- 帰結: `FUN_00415960` @0x415975/0x41597C のゲート `(flags&0x70000) && !(flags&0x180000)` は **fresh な EffectSetting では常に false → +0x60 formID リマップ分岐は非到達**。v242 §7.5 の「条件付き命題」が実証された（命題の「登録個体でのみ到達しうる」側は下記 1.3）。
- 副次実証: 登録 helper `FUN_00417220` 自体が @0x4172EB-0x417327 で bits **16,17,18,19,20,24** を順に test し、いずれも立たない個体のみ別処理（名前文字列 0xA330D4 帯）に分岐＝**登録時に bits16-20 の意味（関連フォーム種別）が確定済み**であることの鏡像証拠。

### 1.3 ゲート通過する内蔵効果の実体（新規・161 件中 **73 件**）— 実測（`v256_10_out.txt`）

`flags&0x70000 ≠ 0 && flags&0x180000 == 0` を満たす内蔵登録 73 件の flags と分類:

| flags | bit | コード群 | 件数 | +0x60 の実体（name 実測） |
|---|---|---|---|---|
| **0x40112** | bit18 | ZGHO/ZLIC/ZSKE…Z001-Z020（Summon 系）・MYTH/MYHL | 41 | **召喚creature の formID**（name="Summon Skeleton Archer" 等） |
| **0x10112** | bit16 | BWAX/BWBO/BWDA/BWMA/BWSW・BW01-BW10 | 15 | **Bound weapon の formID**（"Bound Axe" 等） |
| **0x20112** | bit17 | BABO/BACU/BAGA/BAGR/BAHE/BASH・BA01-BA10 | 17 | **Bound armor の formID**（"Bound Boots" 等） |

- ⇒ **bit16=bound武器/bit17=bound防具/bit18=召喚creature の「関連フォーム種別」セレクタ**［確定級］。`+0x60`（ActorValue 欄）はこれらの個体でのみ **formID** として扱われ、remap→finalize の TD キャストは WEAP/ARMO/CREA 系解決に対応（v242 §1.2「bits16-18=ASSOCIATED フォーム指定」を細分）。
- bit24=161 件中 61（v242 §7.3 分布と一致・`v256_10_out.txt` に no-bit24 100 件のコード一覧）。
- **マージ設計の帰結（移植注記）**: Oblivion_Android 側で fresh EffectSetting をファイル読込のみで生成すると +0x60 formID リマップは永久に発火しない。召喚/Bound 効果の +0x60 を解決するには「内蔵 73  effects の flags 定数（0x10112/0x20112/0x40112）＋code キーの事前登録」を同等に持つ必要がある。

### 1.4 残点（本項目内）

- fresh 分岐 `FUN_00451210` の byte[form+4] 書込（`FUN_0046AF70` の返値＝プラグインテーブル 0xB05E08 線形 index）が formType バイトとどう両立するか（=mod index 格納説・低確度、追跡未了）。低。
- case 0x03 の `DAT_00B35468->vt[0x24](file)` EDID 別経路の所属クラス。低。
- `FUN_00416100` 呼び後の `vt[0x90](1)` の意味。低。

証跡: `v256_1..12_out.txt`（imm32 refs・E8 callers・CreateForm 本体・jump table formType→case 全対応・GetSetting/ReadRecord デリパッチ逐語・161 登録の code/flags 抽出・ゲート 73 件一覧）。

## 2. 【中2】各 UI caller（HUD/loading/Player）での enum 値→ラベル書式対応・tile 0xFB3 への float 交付の実体 — **[解明]**

### 2.1 結論先出し

- **HUD ラベルの書式選択は enum 値（0..11）では行われない**。`FUN_005A4980`（フォーム表示ラベル生成）内の enum 消費は **{3,5} の家具マーカー要求ゲートのみ**（v242 §4.5 継承）。書式（`'%s %s'`/`"%s's %s"`/`'%s (%d)'`）の分岐条件は **baseForm formType・ExtraCount・actor 所有**（2.3）。
- **tile 0xFB3 への float 交付の実体** = `FUN_005A8BC0` @0x5A8C30-0x5A8C43 の `fild`（int→float 変換）→ `FUN_0058CEB0(0xFB3, float(cat))`。**この経路は全体が dead**（唯一の呼出元 `FUN_005A8C90` の E8=0・imm32=0 を v256_21 で再確認。`FUN_005A8BC0` の E8=1 件も `FUN_005A8C90` 内のみ）。⇒ 出荷バイナリで **0xFB3 の float 版に_live な書手は存在しない**。
- loading_menu 側 `FUN_005ADCF0` @0x5ADE1A の `push 0xFB3` は **文字列** `' '`（0xA36430＝半角スペース1文字・実測）の書込＝**別メニューの同 ID タイル**（タイル ID はメニュー毎名前空間・0xFB3 push  sites 199 件の内訳は v256_21）。enum と無関係。

### 2.2 UI caller 引数表（FUN_005A4980 呼び出し窓・実測）

`FUN_005A4980(ref, param_2 bl, param_3)`・E8 sites 5 箇所（4 関数）。書式: push 右→左。

| call site | caller（所属） | 引数実体 | 用途 |
|---|---|---|---|
| 0x5A557D / 0x5A5590 | **`FUN_005A5540` = HUDInfoMenu vtable slot 0x2C**（v231 §2.1 継承） | `ref=[esi+0x54]`（保持照準 ref）／`bl = (tile 0xFB2 の float == [0xA379B4]=2.0f)`（`FUN_00588BD0(0xFB2)` 読み戻し・`fcomp` 実測）／`param_3=1` | **HUD 本体**。生成側が書いた 0xFB2 を**読み戻して bl を再注入**するフィードバックループ |
| 0x580B60 | `FUN_005806D0`（メニュー帯・直前 `[0xB3A6E4]=5`／`[ebx+0xC8]` 判定） | `ref=esi`／`bl = ([ebx+0xC8]==0)`（`sete cl` 実測）／+ | メニュー状態からの再生成 |
| 0x583EDD | `FUN_00583E60`（tile UI 状態機械 3→5・v231 §2.3 継承。直後に tile 0xFE6 に `'Menus\Misc\cursor.dds'`(0xA69194）書込） | **(0,0,0)** | **ラベル消去呼び**（ref=NULL → 既定ラベル `' '` 書込経路） |
| 0x5ADDFF | `FUN_005ADCF0`（**loading_menu**） | **(0,0,0)** → 直後 `FUN_0058CED0(0xFB3, ' ')`（文字列） | ロード画面での消去＋自メニュー 0xFB3 リセット |
| 0x673175 | `FUN_00671620`（**PlayerCharacter** vtable slot 0x228） | **(0,0,0)** → 直後 `FUN_004DD260`（use 判定）／`vt[0x380]` | プレイヤ入力状態変化時の消去 |

⇒ 「HUD=生成／loading・Player・(0,0,0)系=消去」が実体の全部。**どの caller も enum 値をラベル文字列に変換してはいない**（enum→文字列対応表の不在を v242 §4.6 に続き全 caller で再確認）。

### 2.3 FUN_005A4980 のラベル分岐実体（書式選択の実際の条件・逐語）

1. **ゲート列**: `[0xB3B33C]==0`→return／`param_1(ref)` が無ければ初期化系／**cat=FUN_004DE980(ref)** が 5（または ref ありで 3）なら `Player->vt[0x174](1,1,&a,&b)` → `FUN_004DBAE0(ref, …)` で**家具マーカー探索・失敗時ラベル消去**（@0x5A4AEE-0x5A4B37・v242 §4.5 継承）。
2. **騎馬の乗手書換** @0x5A4B43-0x5A4B91: `FUN_004D74D0(ref)`（馬: CREA & base+0x104==4）→ `ref->vt[0x388]()`（騎手）→ 騎手存活かつ **騎手 process `vt[0x36c]()==4`（着席フェーズ・§6 type 体系と同一）** のときのみ `ref := 騎手`（乗馬中は騎手のラベルを出す）。
3. **formType switch** @0x5A4BB6: 2段テーブル byte@0x5A5520→dword@0x5A5510・範囲 ft-0x12∈[0,0x12]（**0x12..0x24** のみ、外は無条件ビルド）:
   - **ft 0x1C STAT / 0x1E TREE → ラベル消去**（@0x5A4BCC）
   - **ft 0x12 ACTI / 0x17 CONT / 0x18 DOOR / 0x23 NPC / 0x24 CREA → bl!=0 なら消去**、bl==0 ならビルド（@0x5A4BFA）
   - **ft 0x1A LIGH → `base+0x7C` bit1 立ち時のみビルド**（@0x5A4C00・§4.2 の cat 1 条件と同一ビット）
   - 他（0x13,0x14,0x15,0x16,0x19,0x1B,0x1D,0x1F,0x20,0x21,0x22＋範囲外）→ 無条件ビルド
4. **ビルド部** @0x5A4C16（`esi=[0xB3B33C]` シングルトンの 2 タイルインターフェース `[esi+0x28]`/`[esi+0x48]` へ両書き）:
   - **tile 0xFB2 = bl ? 2.0 : 1.0**（@0x5A4C4F-0x5A4C69・`float[0xA379B4]=2.0f` 実測）＝2.2 の HUD 読み戻しの源。
   - `FUN_004D7630(ref)`（=TESEntry/所有者関連 getter・`add ecx,0x44` 経由）非0 のとき: **`'%s %s'`(0xA32564) = (表示名, [0xB38D18] の全局文字列)** → tile 0xFDE／tile 0xFA1=2.0（@0x5A4C81-0x5A4CC3）。
     - **[0xB38D18] の正体（新規）**: CRT 初期化 `FUN_009F1C00` 内 @0x9F2A40: `push 'to'(0xA60FA8); push 'sTo'(0xA60FA4); ecx=0xB38D18; call FUN_00419A70` ＝ **GECK 文字テーブルキー `'sTo'`（既定 "to"）の BSString**。隣接 @0x9F2A20 に `'sFor'`（既定 "for"→[0xB38D10]）も同型。**HUD ラベルの動詞文字列**（v256_24 実測）。
   - 所有者なし（`FUN_004D7630==0`）は @0x5A4D02: `ExtraCount`（`FUN_0041E210(ref+0x44, 0x2A)`・既定1）が **==1 → `"%s's %s"`(0xA6BE8C)**（actor なら `vt[0x190]`→`_RTDynamicCast(TESObjectREFR→Actor 0xB03070)`→`process->vt[0x3D0]()` の名前＋表示名。TD 実デコード: 0xB03054=`TESObjectREFR`/0xB03070=`Actor`）。**!=1 → `'%s (%d)'`(0xA6BE84) = (表示名, count)**（@0x5A4DD5）。
   - 表示名 NULL 経路は tile 0xFDE=`' '`(0xA36430)／0xFA1=1.0 の消去値（@0x5A4E45）。
5. 末尾 @0x5A4E91〜: `[esi+0x4C]` 非0 の追加ターゲット（`TESObjectDOOR` 0xB05480 / `TESBoundObject` 0xB03248 / `ArrowProjectile` 0xB05AD0 の TD キャスト窓・`FUN_004DE770(ref,Player,1)`／`FUN_004B72C0` ゲート）＝ドア/武器系の特記ラベル列（本件enumと無関係・細部は残点）。

### 2.4 enum 数値の唯一の消費者 `FUN_005A8C90`（dead）— 数値処理の詳細（新規実測・v256_20 L706-802）

```
if ([0xB13238]!=0) → FUN_005A8BC0(0); ref==0 → FUN_005A8BC0(0)
ebx = cat = FUN_004DE980(ref); ft = byte[base+4]
edi = (ft==0x23 NPC) ? ref : 0                       ; NPC 限定の下流判定
cat==7 または cat==0xA:
    Player->FUN_005E6B40() 真 → BC0(0)               ; プレイヤ状態 suppress
    edi && edi->vt[0x334](1) 真 → BC0(0)             ; NPC 側判定 suppress
    そうでなければ通過（cat==7 は 0x5A8CDB 共有経路）
cat==1:
    edi && (edi->vt[0x334](1) || edi->process->vt[0xAC]()) 真 → BC0(0)
    通過時 BC0(1)
他値: BC0(cat)                                       ; ★ tile 0xFB3 = float(cat) の交付点（dead）
BC0 後: [0xB3B350] && cat∉{0,7} && FUN_004DEBF0(ref) && cast(ref→ArrowProjectile)==0
        → tile 0xFAF = 2.0                           ; 矢でなければ 0xFAF 再起動
```
- `FUN_005A8BC0(cat)` は 0xFB2=1.0→0xFB2=2.0→**0xFB3=float(cat)**→0xFAE=''→0xFAF=1.0 の 5 タイル書込＋`FUN_00401F20` 解放（v256_20 L804-859 逐語）。インターフェース保持は **[0xB3B350]**（HUD クロスヘア側シングルトン・書手 band 0x5A8000-0x5A8710、細部未追跡＝残点）。
- ⇒ **enum 値→「ラベル」対応は EXE に存在しない**（v242 §4.6 の低裁定を全 caller 走査で確定に格上げ）。対応が存在するとすれば**メニュー XML 側のタイル 0xFB3(float)→アイコン/文字列変換**（`hud_crosshair` 帯）であり、EXE 外素材。dead 化により実際には未使用。

### 2.5 残点（本項目内）

- `[0xB3B33C]`（ラベル生成シングルトン）の初期化書手・クラス RTTI。低。
- @0x5A4E91〜 の DOOR/BoundObject/ArrowProjectile 追加ラベル列の全文（`FUN_005A8710` band との対応）。低。
- `vt[0x334]`/`vt[0xAC]`/`FUN_005E6B40`/`FUN_004DEBF0` の語義（NPC 話しかけ可否・くすね可否系と推定）。低。

証跡: `v256_20_out.txt`（FUN_005A4980/005A8C90/005A8BC0 全文字列注記付逆アセンブル）・`v256_21_out.txt`（callers/tile push 数・float[0xA379B4]=2.0）・`v256_22_out.txt`（formType→case テーブル・caller 窓・シングルトン参照一覧）・`v256_23/24_out.txt`（文字列/TD/グローバル書手）。

## 3. 【中3】FUN_00654D00 の type 5/0xA の細分類 — **[解明]**

### 3.1 判定先出し

- **type 5 と 0xA は「.dynamic idle 適用済み・位置確定前」中間状態の双生児**で、両者の差は **家具 marker type の分類のみ**:
  - **0xA ＝ ベッド系マーカー（marker type 0..9・`FUN_004AE5D0(type)` 真）**
  - **5 ＝ 椅子・机系マーカー（marker type 10..19・`FUN_004AE5E0(type)` 真帯）**
- v242 §6.2 の「5=睡眠系 体位確定（0xA と対）／0xA=睡眠系 遷移（5 の前段）」は**訂正**（5 は睡眠系でなく椅子側・前段関係ではなく同段の区分）。stable 終端も同じ分類で **4＝椅子側・9＝ベッド側**（3.3）。

### 3.2 分類関数（確定・逐語）

- `FUN_004AE5D0(x)` = `xor al,al; cmp [esp+4],0xA; jae 0; mov al,1; ret` ＝ **x < 10**（1 文述語）。
- `FUN_004AE5E0(x)` = **10 ≤ x ≤ 19**（`add ecx,-0xA; cmp ecx,9; ja 0`）。
- v231 §1.3 の対応（bit30 sit↔type 10..18／bit31 sleep↔type 0..9）と噛み合い、**0..9=ベッド用マーカー、10..19=着席用マーカー**が marker type の実区分（CS の FurnitureMarker 種別＝v231 残点の「正式名」側は依然 EXE 文字列なし・低）。

### 3.3 発生点の全貌（`FUN_00654F10` = marker 再評価・idle 取得 tick・新規全文実測）

`FUN_00654F10(process=ecx, actorRef=[esp+0x18])`（E8 callers 3 箇所: `FUN_0063A020` @0x63AB49/0x63B294・`FUN_00657A80` @0x6581A2 — HighProcess update 帯）。

```
ebp = actorRef->vt[0x164]()          ; アニメ/3D 系オブジェクト（+0xC4 バイト＝後述フラグ）
if (!ebp) return                     ; proc+0x120==0 なら vt[0x370](ref, 0, 0, 0x7F)＋vt[0x188](1,ref)
furn = [proc+0x120]（vt[0x370] の arg3 保存値）
bl = FUN_004AE5D0([proc+0x136])      ; ★ [proc+0x136] = marker type byte（新規フィールド特定）
cur = [proc+0x11D]                   ; 現在の type（vt[0x36C] のデータ元）
整合ガード: cur==4 && bl → return ／ cur==9 && !bl → return   ; stable と marker 級の不一致時は何もしない
switch (cur-4) 0..6 (byte@0x6553D4→dword@0x6553C8 実測):
  cur ∈ {4, 9}  → @0x654FBB:
      if (bl) { actorRef->FUN_005E12B0()->vt[0x9C](0,0)          ; ベッド側は 9 成立時 (1,0) の対（v242 §6.2）
                vt[0x370](ref, 0xA, furn, [proc+0x124]) }        ; ★ 0xA = ベッド系中間
      else      vt[0x370](ref, 5,  furn, [proc+0x124])           ; ★ 5 = 椅子系中間
      rec = FUN_00521450(*[0xB362C0], ref, furn)                 ; editor's idle manager 検索
      成功 → FUN_00520200(rec)→FUN_00475300(ebp,…) で idle 適用・return 1
      失敗 → [ebp+0xC4]=1; vt[0x370](ref, 0, furn, 0x7F)         ; 「Missing furniture dynamic idle…」系
             → 位置/角度再計算（FUN_004ABE0＋0x659B90・fchs/π 反転 0xA3D5B8）＋
               proc+0x120=0・[0xB3F9A8/B/F] を NiPoint3 スロット[esi+0x128]へ・0x65AC20(ref,0)
             → bl==0 で終了；bl!=0 は vt[0x184]() の [node+0x1C] bit20/21 で 2 フラグ決定後、
               FUN_005E32D0(ref) で **TESNPC**（cast TD 0xB03248→**0xB02FB4=.?AVTESNPC@@**）なら
               FUN_005227A0(…,1,0)、**TESCreature**（0xB0375C=.?AVTESCreature@@）なら FUN_0051E240(…) を適用
  cur ∈ {5, 0xA} → @0x655207（idle 適用後の再評価）:
      FUN_004711F0(ebp) かつ FUN_004706E0(ebp,0)->[+0x44]==1 のとき（アニメ完了検知相当）:
          [ebp+0xC4]=1
          ebx = cur; vt[0x370](ref, 0, furn, [proc+0x124])       ; 一時的に 0 へ
          位置/角度再計算・配置（同上の列）
          vt[0x370](ref, **ebx**, furn, [proc+0x124])            ; ★ 同じ 5/0xA へ復元（位地だけ更新）
          FUN_00477E50(ebp); return 1
      さもなくば FUN_00472EA0(ebp) 真 → idle manager 解除列（FUN_004D7300・vt[0x370](0,0,0x7F)・
          アングル再計算・process->vt[0x18C] 経由で [actor+0x2C] を list 0x452A10 へ追加）→ return 1
  cur ∈ {6, 7, 8} → @0x6553AB（**データ直接書換**・vt[0x370] 経由なし）:
          `neg bl; sbb bl,bl; and bl,5; add bl,4` ⇒ **[proc+0x11D] = bl ? 9 : 4**
          ; 要求/失敗/位置確定系 (6/7/8) を marker 級で stable へ収束（ベッド→9・椅子→4）
```

### 3.4 型コード体系の更新（v242 §6.2 表への反映候補）

| type | 修正後裁定 | 根拠 |
|---|---|---|
| 4 | **椅子系 stable（着席成立）** | 整合ガード `cur==4 && bl(ベッド級) → return`・`bl==0→4` 直接書換 |
| 5 | **椅子系中間（dynamic idle 適用済み・位置確定前）** | @0x654FFA-0x655016（bl==0 枝） |
| 9 | **ベッド系 stable（睡眠成立）** | 対称ガード・`bl!=0→9` |
| 0xA | **ベッド系中間（idle 適用済み）**・適用時 `vt[0x9C](0,0)` クリア | @0x654FBF-0x654FF6 |
| 6/7/8 | 要求/失敗/位置確定系から **marker 級で 4/9 へ収束**される過渡値 | @0x6553AB（v242「7=睡眠失敗」は過渡用途の一つにすぎない） |
| 0 | 解除 | 全経路共通（v242 継承） |

- **新規確定フィールド**: `[proc+0x136]` = 使用中家具の **marker type byte**（`vt[0x370]` arg4=[proc+0x124] とは別・tick 内で `FUN_004AE5D0` に供給）。
- idle manager シングルトン **[0xB362C0]**（エラー文字列「Missing furniture dynamic idle for marker %d in the editor's idle manager.」(0xA72C28) と同一系・v242 §6.2 継承）。
- アクター種別分岐の TD 実デコード: NPC=`0xB02FB4`／Creature=`0xB0375C`（出所 `.?AVTESNPC@@`/`.?AVTESCreature@@`）。

### 3.5 残点（本項目内）

- `vt[0x164]` 返り値の +0xC4 byte と `FUN_004711F0/0x4706E0/0x472EA0/0x477E50` の正確なクラス（BSAnim と推定帯・未 RTTI）。低。
- `[0xB3F9A8/AC/B0]` 3 float（寝姿勢時のカメラオフセットと推定）の名称。低。
- `FUN_005227A0`/`FUN_0051E240`（NPC/CREA 別 idle 適用）の内部。低。

証跡: `v256_30_out.txt`（FUN_00654F10 全文・FUN_004AE5D0/E0 逐語・switch テーブル）、TD/E8/callers は v256_30 末尾走査（0.4KB スニペットは `v256_30b` 相当出力に含む）。

## 4. 【中4】FUN_006575B0 の vtable slot 割当と vt[0x420] 実装の本文 — **[確定]**

### 4.1 slot 割当（帰属 vtable 確定・v242 §8.5 残点の解消）

遡及走査（base-4 の COL→TypeDescriptor を全候補で検証・`v256_40_out.txt`）:

| 登録サイト | 帰属 vtable base | RTTI | **slot（vtable byte offset）** |
|---|---|---|---|
| 0xA71C40 | **0xA71814** | `.?AVHighProcess@@`（COL 0xAC39B4） | **+0x42C** |
| 0xA72AB0 | **0xA72684** | `.?AVMiddleHighProcess@@`（COL 0xAC3C54） | **+0x42C** |

- ⇒ **FUN_006575B0 = HighProcess/MiddleHighProcess 共通の vt[+0x42C] 実装**。E8 callers **0 件**（純仮想ディスパッチ・再確認）。LowProcess 側は未登録（0xA720A4 の同 offset は別実装帯）。
- v242 の「0xA72AB0 の帰属未確定」は **MiddleHighProcess vtable**（base 0xA72684）と確定。

### 4.2 vt[0x420] の正slotと実装本文（v242 §8.3 の別解釈を訂正）

- **v242 §8.3「HighProcess+0x420×4 = 0xA72894 → 0x629300」は誤り**（0xA72894 は MiddleHighProcess(0xA72684)+0x210＝slot 0x210 の別関数）。vtable byte offset 規約（§5.2/§6 と同一）で読むと:
  - **HighProcess [0xA71814+0x420] = `FUN_006294B0`**（MiddleHighProcess [0xA72684+0x420] も同一実装）。
  - 実装本文（**全 3 命令**・確定）:
    ```
    0x6294B0: mov eax, [esp+4]      ; arg = TESEffectShader*（または 0）
    0x6294B4: mov [ecx+0x150], eax  ; ★ process+0x150 = エフェクトシェーダ slot へ保存
    0x6294BA: ret 4
    ```
  - ⇒ **vt[0x420] は「シェーダ適用 setter」＝ `process+0x150` への格納のみ**（副作用なしの純セッタ）。LowProcess 側 slot0x420 = 0x68F970（ret 4 スタブ帯・v249 記述と整合）。
- **読み手も確定**: 隣接 **vt[0x41C] = `FUN_0042BB10`** = `mov eax,[ecx+0x150]; ret`（**+0x150 の getter**）。バンド内（0x620000-0x670000/0x5E1000-0x5E5000）に `[reg+0x150]` の直接メモリの読みは **0 件**＝読取は常に vt[0x41C] 経由（`v256_42_out.txt`）。
- スロット群（HighProcess base+0x418..0x430 実測）: `0x418=0x645280 / 0x41C=0x42BB10(getter) / 0x420=0x6294B0(setter) / 0x424=0x6294C0(byte[+0x160] getter) / 0x428=0x6294D0 / 0x42C=FUN_006575B0(適用者) / 0x430=0x6294E0` — **シェーダ accessers cluster + 適用メソッド**の並び。

### 4.3 FUN_006575B0（vt[0x42C]）本文の flow（窓実測・v242 §8.2 を含む全文脈）

thiscall(ecx=process=esi)、args: `ebp=[esp+0x2C]`（アクター REFR）、`[esp+0x30]/[esp+0x34]`（2 flags）、末尾追加 flag `[esp+0x38]`:

```
[process+0x160] = 0                                  ; pending クリア（vt[0x424] で読める byte）
if (flagA) { obj = FUN_00679120(*0xB3BD00, ebp); if (obj) obj->vt[0x68]() }   ; 家具進行役から検索・実行
if (!flagB) → 0x657850（別経路）
this->vt[0x420](0)                                   ; ★ シェーダ slot クリア（0x65760E）
proc = [ebp+0x58]; proc->vt[0x13C]() 真ならスキップ
edi = FUN_004695A0( proc->vt[0xEC](1)->+8 )          ; マジックリスト系からエフェクトオーナー解決
时刻比较: FUN_004849C0(proc…) → double → edi+0x24 オブジェクト vt[0](ebp) と fcomp（経過判定）
eax = FUN_00419B30(&[edi+0x18], 0)                   ; 代表エフェクト選択（MagicItem・v216）
shader = [eax+0x7C]                                  ; ★ EffectSetting+0x7C = TESEffectShader*（§1.2 確定済み）
if (shader) this->vt[0x420](shader)                  ; ★★ シェーダ適用 setter（+0x150 へ格納） 0x6576B1-0x6576BC
r = this->vt[0x41C]()                                ; +0x150 read-back
obj = FUN_00679240(*0xB3BD00, ebp, r)                ; 進行役注册
if (obj) obj->vt[0x68]() else {
    if (eax2 = this->vt[0x41C]()) {                  ; シェーダ保持時のみ
        hit = new(0x4C); FUN_006A0980(hit, ebp, shader=this->vt[0x41C]())
                                                    ; ★ MagicShaderHitEffect instance（v242 §8.2 の生成経路そのもの・
                                                    ;   0x6576FD push 0x4C / 0x657733 ctor）
        [hit+0x28]=1; flagC → [hit+0x38]=float[0xA2FE7C]
        ... hit->vt[] 適用 …
    }
}
```
- ⇒ v242 §8 裁定（「+0x7C=シェーダフォーム／new 0x4C=MagicShaderHitEffect／vt[0x420]=適用 setter」の3要素分離）を**本文レベルで完遂**。setter の実体は「process+0x150 へのポインタ格納」、生成経路（0x6576FD 帯）では **getter vt[0x41C] の返値を ctor 引数**として渡していることも実測。
- float 定数 2 個（0xA30634=ctor 直前 fld・0xA2FE7C=[hit+0x38]）の語義（持続/強度候補）は未裁定＝残点（低）。

### 4.4 残点（本項目内）

- `FUN_006294D0`（0x428）/`FUN_006294E0`（0x430）の本文（accessors cluster の残り 2 本）。低。
- `[edi+0x24]` オブジェクトの vt[0](ebp) 時刻比較の意味（cast 系？）。低。
- LowProcess 側の対応 slot（0x42C 相当）が空栈ブか。低（v249 のスタブ帯記述と整合的だが未実測）。

証跡: `v256_40_out.txt`（vtable 帰属遡及・RTTI・E8 callers 0・前半窓）・`v256_41_out.txt`（vt[0x420]=0x6294B0 3 命令・0x657622-0x6576DF 窓）・`v256_42_out.txt`（+0x150 直接読 0 件・スロット一覧・getter 本文）。

## 5. 低束（7件）— 各判定

### 5.1 【低1】ALCH/INGR ENIT {u32,u8} の意味 — **[解明]**

- **書式契約（v231 (V) 継承）**: ALCH/INGR の ENIT 8B は Ptr アクセサ `lea [ecx+0x54]`（MI 基準）/ Size `FUN_00419750`=8B ⇒ form 絶対で **+0x78(u32)/+0x7C(u8)**（ALCH ctor 0x412BE0 が `-1 / 0` を書くこと・v242 §3.3 継承。INGR ctor `FUN_004198F0` @0x419997 も `+0x78=-1` を実測＝同一布局）。
- **新規実測（compare/copy 本体の逐語・`v256_54/55`）**: `FUN_00412A00`（compare）と `FUN_00412A40`（copy）は `_RTDynamicCast(arg, MagicItem(0xB03280)→AlchemyItem(0xB0320C))` 成功時に限り
  - compare: `[other+0x78]==[this+0x54] && byte[other+0x7C]==byte[this+0x58]`
  - copy: `[this+0x54]=[other+0x78]; byte[this+0x58]=byte[other+0x7C]`
  を行う。**+0x54/+0x58 は (V) の ENIT Ptr/Size アクセサの座標そのもの**（this=MagicItem 基準）⇒ **+0x78=cast type（-1=既定/自動）、+0x7C=flags（ENIT 16B の ENCH 版 dword0/byte3 と同役割の 8B 版）**＝意味確定。
- consumers: 帯内数値消費者は compare/copy のみ（v242 §3.3 の追加分も再現）。おまけ実測: **flags bit0 の set/clear スタブ**（0x4129C1 帯: `cmp byte[esp+4],0 / or byte[ecx+0x7C],1 / and byte[ecx+0x7C],0xFE / ret 4`）が実在するが **imm32/E8 の直接参照 0 件**（vtable ストア経由の可能性あり・参照未追跡＝残点・低）。
- 次アクション候補 (a)(b)（v242 §3.3）は (a) の Web 裏取り不要で内部証拠のみ確定化、(b) は帯走査完了（読手=copy/compare のみ）。

### 5.2 【低2】BOOK DATA bit0 の GECK 正式名 — **[未確定（Web 取得失敗）]**

- EXE 側は v242 §2.3 で確定済み（bit0 =「ENAM(+0x64) 付与持ちの書物を scroll として使用可にする述語」読手12件）。
- GECK のチェックボックス正式名は UESP 照合を試みたが **503（4回試行）・検索プロバイダ QUOTA 超過で取得不能** ⇒ **未確認情報**。一般に知られる名称は "Is Scroll"/"Scroll" 系と**断定せずに**記録（本タスクの一次証拠なし）。GT 反映ドラフトでは「正式名未照合」注記を推奨（§6 (AI)）。

### 5.3 【低3】`[book+0x89]` の語義 — **[機構確定・GECK 名未照合]**

- **新規実測（`v256_51/55`）**: 読手/書手を全帯走査で網羅（計6サイト）:
  - getter **`FUN_004B52D0`** = `mov al, byte[ecx+0x89]; ret`（0xFF=無効 sentinel）。
  - **唯一の実運用読手 = `FUN_004B5640`（TESObjectBOOK::Use 相当・冒頭 `_RTDynamicCast(→TESObjectBOOK)`）** の 3 箇所（0x4B57FB/0x4B580B/0x4B5832）: `v=getter; if(v!=-1){ FUN_005F1910(Player, v)<100 のとき [Player+0x6A0]+=1 …; f = FUN_00565C50(2, v); form = FUN_00446AF0(*0xB33A98 DataHandler, f) }`。
  - `FUN_00565BE0/0x565C50`（同型 switch-case 0..6: +0/+8/**+0x0C**/+0x21/+0x25/+0x28/+0x2A）＝**紧凑值→formType の再写像表関数**。finalize（`FUN_004B53E0` 全文 0x4B53E0..0x4B5430 逐語）は `kind=2` 適用後 **0x0C..0x21（=formType MGEF 0x0C〜WEAP 0x21）範囲外なら 0xFF リセット**。
- ⇒ **[book+0x89] の語義 = 「(formType − 0x0C) の紧凑セレクタ（0xFF=無効）」**。BOOK::Use はこの値から **対応 formType のフォームを DataHandler から解決して効果を適用**（使用回数カウンタ [x+0x69C]/[Player+0x6A0] 増分も実測）。v242「+0x89 = DATA byte1・formType 範囲検証」を**消費側の意味まで確定**で更新。
- GECK 上の表示名（"Special Item Type" スダウン）は UESP 503 で未照合（§6 (AI) に回付）。範囲 0..0x15 が GECK の 0..4 選択肢より広い点は注記（エンジン側は formType 互換の広域値を受け付ける設計）。

### 5.4 【低4】ENCH 側 SpellCost(+0x38) 消費者 — **[ scan 網羅・消費実体なし]**

- magic/enchant 帯（0x410000-0x420000 / 0x5D4000-0x5D6000）+ メニュー帯（0x597000-0x5A0000）の `dword [reg+0x38]` 走査（`v256_50 (3)`）: **実データ読手 0 件**（ヒット 2 件は EffectSetting の別意味 +0x38 と `lea` 引数転送のみ）。
- v242 §3.1 の書手（ctor/clone/reset/汎用 form-copy 5 関数）と併せ、**付与コストの手動値 +0x38 は EXE 内では「保存・複製・比較」専用で、計算/表示の直接消費者を持たない**（付与メニューのコストは効果リスト側積算と整合）。裁定: **write/copy-only［確定（走査網羅範囲では）］**。UI 側が仮想アクセサ経由で読む可能性は残点（低）。

### 5.5 【低5】bit24 の CEF 正式名と runtime 読手 — **[読手新規確定・正式名 Web 未取得]**

- **runtime 読手 = 1 件を新規確定**（広域走査 `v256_53`: mov[+0x58]→test/shr bit24、magic/combat/enchant/cast 帯）:
  - `FUN_00413130` @0x4131B4-0x4131CC: `edi=[eset+0x58]; if (edi & 0x1180000) && ((edi>>0x18)&1) → [item+0x14] = [eset+0x60]` ＝ **EffectItem 構築時に bit24 立上の個体のみ +0x60 を AV 値として EffectItem+0x14 へ写す**。ReadRecord 正規化（oldAV 保護）と対で、**bit24 = 「+0x60 は ActorValue（formID ではない）」の宣言ビット**としての消費実体が確定（v242 §7.3「AV-mode マーカ［有力］」→ 消費証拠付きで強化）。
  - 同関数 @0x4131CF は `[eset+0x98]=='SEFF'` 特殊分岐（コード保存位置 +0x98 の読手として参考）。
  - 他候補ヒット（0x5DC47D）は `or edi,-1` の誤検出（imm=0xffffffff）と棄却。
- CEF/GECK 正式名: UESP 503・検索 QUOTA 超過で**未取得＝未確認情報**（v216 §2.2 表に bit24 無しは v242 継承）。ドラフトでは数値定義のみ反映を推奨。

### 5.6 【低6】LIGH `+0x7C` bit1 の書手 — **[確定（実質）]**

- LIGH ReadRecord = **`FUN_004B0D10`**（LIGH 主 vtable 0xA4319C slot+0x1C・RTTI `.?AVTESObjectLIGH@@` 実デコード・ctor 0x4B1F90 の `byte[+4]=0x1A` 再確認）: **'DATA' 分岐 @0x4B0DA7 = `push 0x18; lea eax,[edi+0x70]; call FUN_0046BDA0` ＝ 24B 生読み → +0x70..+0x87（+0x7C はファイル bytes 12..15）**。
- 書手全集（広帯 0x4B1000-0x4B4000 実測）: (1) ctor `xor eax,eax → mov [esi+0x7C],eax`（既定 0）、(2) **form コピー `FUN_004B0FC0` @0x4B1000-03**（`[esi+0x7C]=[edi+0x7C]`）、(3) 上記 ReadRecord 生読み、(4) WriteRecord `FUN_004B0F10`（+0x70 基準の対称書出）。**実行時（runtime）に bit1 を単独操作する書手は 0 件**（or/and 系を帯内走査で確認）。
- ⇒ **bit1 は純ファイル由来**（runtime 不変）。読手は v242 §4.2（0x4DEB8B）に加え **`FUN_004B1A50`（0x48=bit3,6/0x100=bit8/符号のテスト＝light 挙動述語）**・`FUN_004B1600`（bit テスト群）を新規記録。+0x7C が DATA 内のどの GECK フィールドか（flags2/delay）は UESP 未照合＝残注記。

### 5.7 【低7】formType 表（26件）と GT の行単位突合 — **[突合完了・矛盾 0・追加候補のみ]**

- **GT 本体（docs/RE_EXE_REPORT.md・docs/SCRIPT_OPCODE_TABLE.md）に 26 formType 表の正表は存在しない**（ctor VA 27 本・FUN_004DE980/0x4DBAE0 を grep し**全 0 ヒット**・`v256` メモ）。SCRIPT_OPCODE_TABLE の `0x12` 等は引数型番号別物で衝突対象外。
- 既存 GT の断片記述との行突合（該当 3 行）:
  | GT 行 | 記述 | v242 §4.2 26 表との判定 |
  |---|---|---|
  | RE_EXE_REPORT L10672 | `byte[formType+4] ∈ {0x14=ARMO, 0x16=CLOT}` | **一致**（0x14 TESObjectARMO / 0x16 TESObjectCLOT ✓） |
  | L9908 | formType 0x17→base+0x24 / 0x23・0x24→base+0x44 | **一致**（0x17 CONT・0x23 NPC・0x24 CREA ✓） |
  | L9773 | formType 0x17/0x23/0x24 で DRSBodyOpen | **一致**（コンテナ/NPC/CREA の死体_sound 系統と矛盾なし） |
- v231 ドラフト (T) の 5 行（0x14/0x15/0x16/0x21/0x22 の ctor+主 vtable）と v242 §4.2 も行単位一致（ctor 5/5 ✓・vtable 値は v242 が RTTI で再確認済みの同値）。
- ⇒ 反映候補は「**正表の新規追加（26 行）**＋既存断片行へのクロスリンク」のみ（訂正 0 件）。ドラフト §6 (AH)。

証跡: `v256_50..56_out.txt`（帯走査・窓・RTTI・GT grep は会話内ログに VA 列挙）。

## 6. GT/plan ドラフト（反映は要承認・本体無編集）

> v242 §9 の (AB)-(AF) を旧状態とする新旧判断＋本タスク（§1-§5）由来の新規候補。反映前に GT 本体（docs/RE_EXE_REPORT.md 等）と行単位で突合のこと。

### 6.1 既存ドラフト行の裁定

| ID | 旧記述（v242 §9.2） | 本タスク裁定 |
|---|---|---|
| **(AB)** MGEF flags 正規化テーブル | used マスク・bit21/bit24 裁定 | **維持＋確定昇格**: bit24 の消費実体 Found（§5.5 `FUN_00413130`）。「登録オブジェクトへマージされる」前提自体が §1 で実証され、設計記述は**確定**に置換 |
| **(AD)** EffectSetting finalize | +0x60 bits16-18 条件付きリマップ | **細分更新**: bit16/17/18 = bound武器/防具/召喚creature の**関連フォーム種別セレクタ**（§1.3 73 件の flags 実測 0x10112/0x20112/0x40112）。「fresh 個体ではリマップ非到達」を**確定**追記（§1.2） |
| **(AF)** MagicShaderHitEffect | new 0x4C/ctor/vt0x420 setter | **更新**: vt[0x420]=**`FUN_006294B0`（`[process+0x150]=arg` の純セッタ）**・getter=vt[0x41C] `FUN_0042BB10`・適用者=**vt[0x42C] `FUN_006575B0`**（HighProcess 0xA71814 と MiddleHighProcess 0xA72684 の両 vtable）。**v242 §8.3 の「+0x420*4=0xA72894→0x629300（FUN_00629260 帯）」は誤変換**（0xA72894 は MHP+0x210 の別 slot）＝§7 訂正候補 (3) |

### 6.2 新規 GT 行ドラフト（本タスク由来）

- **(AG)** GT「ESM MGEF マージ経路」行（新規・主成果）: **`FUN_0044DCF0` = record→form CreateForm**（E8 callers: `FUN_0044F0C0`/`FUN_0045BDE0`/`FUN_00461310`/`FUN_004D1340`、switch(formType) jump table byte@0x44E6DC/dword@0x44E6A0、**formType 0x0C=MGEF は本タスク実測の新規確定**）。MGEF case は 'EDID' で**code-keyed 再利用**（`FUN_004168A0`→マップ `0xB33508` Find・キー=4 文字 big-endian 合成・例 "WABR"→0x52424157＝登録側 push 実値と一致）→ ヒット時 **`FUN_00447050(form,file)` = `form->vt[0x1C]` ReadRecord**（EffectSetting では `FUN_00416100`＝used マスク正規化）で**登録オブジェクトへマージ**；miss 時 new(0xA8)+ctor+`FUN_00451210`(SetFormID `FUN_0046C300`)+`this+0x8B8` リスト (`FUN_00446CB0`)。内蔵登録側 `FUN_00417220` の末尾 `mov ecx,0xB33508; call 0x452570` が**同一マップへの insert**（キー=code）。generic case（`FUN_00447DF0`→`FUN_00447050`→`FUN_0044D950`）＋ formID キーの全体再利用（`0xB0613C` Find・@0x44DDA9、formType 不一致警告 0xA38598/0xA385D8）も同節に記載。
- **(AH)** GT「formType 正表（26 行）」行: 低7 突合結果——**GT 本体に正表なし（全 26 行が追加候補）**、既存断片 3 行（L10672/L9908/L9773）と**矛盾 0**、v231 (T) 5 行と v242 §4.2 一致。正表は v242 §4.2 を正本として差替（0x1F=TESFlora 補間注記 유지）。
- **(AI)** GT「未照合 Web 項目」注記行: **BOOK DATA bit0 正式名・[book+0x89] の GECK 表示名・bit24 の CEF 名**＝UESP 503/検索 QUOTA 超過で**未照合**（2026-11-02/03 試行記録）。反映時は「一次資料照合済み」を装わず pending タグ付きで。
- **(AJ)** GT「HUD ラベル」行: `FUN_005A4980` の分岐実体更新——**ラベル書式選択は enum 非依存**（`'%s %s'`=[0xB38D18]=**'sTo'（既定 "to"）CRT 初期化 @0x9F2A40/0x9F2A20 に 'sFor' あり**＋所有者／`"%s's %s"`＝count==1×Actor / `'%s (%d)'`＝count≠1・`FUN_0041E860` が ExtraData 型 **0x2A**（ExtraCount）既定 1）。**tile 0xFB2 = bl?2.0:1.0 のエコー**で HUDInfoMenu.vt[0x2C]（`FUN_005A5540`）が読み戻して param_2 化（feedback loop 新規）。loading（`FUN_005ADCF0`）/Player（`FUN_00671620`）/(0,0,0)=**消去専用呼び**。**tile 0xFB3 float=dead 帯専用**（`FUN_005A8C90` E8/imm32 0 件再確認＋`FUN_005A8BC0` の E8=0x5A8D36 のみ）＝enum→文字列対応は EXE に不在（確定）。騎馬→乗手ラベル書換（`vt[0x388]`+乗手 process `vt[0x36c]()==4`）も同節。
- **(AK)** GT「家具 type 体系」行（(X) 後継・更新）: **5=椅子系中間 / 0xA=ベッド系中間**（marker type byte **[proc+0x136]** を `FUN_004AE5D0`(x<10)/`FUN_004AE5E0`(10≤x≤19) で分類・両者は `FUN_00654F10` の switch cur∈{4,9} 分岐から排他発生）。stable 4=椅子/9=ベッド、**6/7/8→bl?9:4 の直接書換**（0x6553AB・vt 非経由）、idle 適用完了復元（cur∈{5,0xA}→位置再計算→同 type 再設定）、失敗時 `[ebp+0xC4]=1`+type 0。**editor's idle manager = [0xB362C0]**・NPC/Creature 別適用関数 `FUN_005227A0`/`FUN_0051E240`（TD `.?AVTESNPC@@`/`.?AVTESCreature@@` 実デコード）。v242 §6.2 の「5=睡眠系 体位確定／0xA=5 の前段」は**訂正**。
- **(AL)** GT「EffectShader プロパティ」行（新規・低1/4/5/6 統合）: ALCH/INGR ENIT 8B = **{cast type(int,-1 既定), flags(byte)}**（compare/copy `_RTDynamicCast(MagicItem→AlchemyItem)` 経由で確定・bit0 set/clear スタブ 0x4129C1 帯あり参照未確認）／**[book+0x89]=(formType−0x0C) selector**（0xFF=無効、読手=BOOK::Use `FUN_004B5640`×3＋finalize 範囲検査、`FUN_00565BE0/5C50` kind 表 +0/+8/+0xC/+0x21/+0x25/+0x28/+0x2A）／**bit24 消費**=`FUN_00413130@0x4131B4`（AV→EffectItem+0x14）／**LIGH +0x7C は ReadRecord `FUN_004B0D10` の DATA 24B 生読（+0x70 base）のみが供給**（runtime bit1 書手 0 件・copy `FUN_004B0FC0`/ctor 0/WriteRecord `FUN_004B0F10`）。

### 6.3 plan（Oblivion_Android 移植）への影響候補

- **MGEF マージ（(AG)）**: 移植側は「内蔵 flags 定数（0x?0112/0x0112 帯 73 件の AV-mode 区分）＋code キー事前登録」を持たない限り ESM マージ意味論が再現不能（§1.3）。
- **HUD**: enum→文字列表は不要（書式選択は formType+count+所有者）。ただし **0xFB2 エコー→HUDInfoMenu 引数**の輻輳と**騎馬→乗手書換**は移植仕様として必要（(AJ)）。
- **家具ステートマシン**: type 5/0xA（中間）→4/9（stable）の**マーカー級収束**（bl?9:4 直接書換）と idle 失敗（[ebp+0xC4]/type 0+marker 0x7F）まで含む完全遷移図は §3（(AK)）が一次資料（port 資料 v236/v237/v239/v241 へ差し替え反映候補）。
- **MagicShader**: `process+0x150` セッタ/ゲッタ分離（§4）は v242 §8 の 3 要素裁定と一致——移植側は「シェーダ ptr 保持＝単一 field +0x150」で足りる（instance 生成は呼び出し側）。

## 7. 自己検証・証跡・残点

- **selftest**: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C `.\AI\CombatController.cpp` / opid0x0015 impl=0x504560）。**完成直前にも再実行 → ALL PASS (4/4)**（§7 末尾記載）。
- **判定まとめ**: 中1 **[確定]** / 中2 **[解明]** / 中3 **[解明]**（v242 §6.2 5/0xA 行に訂正）/ 中4 **[確定]**（v242 §8.3 の代替解釈 1 件を誤りと訂正）/ 低1 **[解明]** / 低2 **[未確定＝Web]** / 低3 **[機構確定]** / 低4 **[走査網羅・消費なし確定]** / 低5 **[読手確定・名未照合]** / 低6 **[確定]** / 低7 **[突合完了・矛盾 0]**。
- **本タスク新規確定 VA 主要**: `FUN_0044DCF0`（CreateForm・formType switch 2 段表 0x44E6DC/0x44E6A0）・**formType 0x0C=MGEF**・`FUN_004168A0`（code 変換＋0xB33508 Find）・`FUN_00447050`（vt[0x1C] ReadRecord デリパッチ）・`FUN_00417220`@0x4173DC（insert 0x452570）・`FUN_0042BB10`（[+0x150] getter）/`FUN_006294B0`（setter）/MiddleHighProcess vtable **0xA72684**・`FUN_00413130`@0x4131B4（bit24 消費）・`FUN_004B0D10/0F10`（LIGH R/W Record・DATA 24B@+0x70）・`FUN_004B5640`（BOOK::Use）・`FUN_00565BE0/565C50`（kind 再写像表）・[0xB38D18]='sTo'/'sFor' CRT 初期化・`FUN_0041E860`（ExtraData 0x2A）。
- **証跡（すべて `%TEMP%\opencode\`）**: `v256_1..12_out.txt`（中1：imm32/E8/jump table/GetSetting・マージデリバッチ・161 登録ゲート 73 件リスト）、`v256_20..24_out.txt`（中2：FUN_005A4980/005A8C90/005A8BC0 全文字列注記・caller 窓・書式/ゲート・グローバル sTo・RTTI TD）、`v256_30_out.txt`（中3：FUN_00654F10 全文・分類関数・switch テーブル）、`v256_40..42_out.txt`（中4：vtable 帰属遡及・setter/getter・slot 一覧）、`v256_50..56_out.txt`（低束：帯走査・窓・RTTI・GT grep）。
- **遵守事項**: git 書出なし / docs・GT・TODO・plan・session-memory・cache・ソース無編集 / 既存 tmp/re 無上書き（本ファイル v256 のみ新規）/ ESM 本体再スキャンなし / VA→raw はセクション個別換算（v172_common `va2off`/`imm32_refs3`）・capstone 線形走査の int3 ゾーン誤復元 1 件（ALCH 0x4129C7 の or/and）を関数先頭再復唱で確認し§5.1 に正確な位置を記録 / PowerShell ヒアドキュメント不使用・`python -c` は正規表現なしの軽量確認のみ使用 / Web 取得失敗（UESP 503×4・検索 QUOTA 超過）は捏造せず未照合明示（§5.2/§5.5/§6.2 (AI)）。
- **最終 selftest（§6 追記後）**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。
- **新規残点（次束候補）**:
  1. （低）fresh 分岐 `FUN_00451210` の byte[form+4] 書込（mod index 格納説）と formType バイトの両立機構。
  2. （低）CreateForm case 0x03（`DAT_00B35468->vt[0x24]` の EDID 別経路）の所属 formType/クラス。
  3. （低）`vt[+0x1C]` 後段 `vt[0x90](1)` の意味・`FUN_00447050` の form+8 bit0 ガード両側の消費者。
  4. （低）HUD ラベル末尾 DOOR/BoundObject/ArrowProjectile 追加列（`FUN_005A8710` band）と `[0xB3B33C]`/`[0xB3B350]` シングルトンの初期化書手・RTTI。
  5. （低）type 体系関連: `vt[0x164]` 返り値 +0xC4/`FUN_004711F0/4706E0/472EA0/477E50`（BSAnim 推定帯）の RTTI・[0xB3F9A8/B/F] の 3 float 名称。
  6. （低）ALCH flags bit0 set/clear スタブ（0x4129C1 帯）の vtable 帰属と実呼び出し有無。
  7. （低→要 Web）BOOK bit0/byte1・bit24 の GECK/CEF 正式名（UESP 到達再開時に照合）。
