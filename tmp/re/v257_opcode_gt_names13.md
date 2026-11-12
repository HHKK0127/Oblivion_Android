# v257_opcode_gt_names13.md — opcode/GT 殘点 第13束（7件）

- 作成日: 2026-11-03 / 担当: TODO serial 249 (V249・低) spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v256_opcode_gt_names12.md` §7 末尾の新規残点7件（すべて低）:
  1. fresh 分岐 `FUN_00451210` の byte[form+4] 書込と formType バイトの両立
  2. CreateForm case 0x03（`DAT_00B35468->vt[0x24](file)`・0xB35468）の所属
  3. `vt[0x90](1)` の意味・`FUN_00447050` の form+8 bit0 ガード両側の消費者
  4. HUD ラベル末尾 DOOR/BoundObject/ArrowProjectile 追加列と `[0xB3B33C]`/`[0xB3B350]` の初期化・RTTI
  5. `vt[0x164]` 返り値 +0xC4/BSAnim 帯（`FUN_004711F0/4706E0/472EA0/477E50`）の RTTI・`[0xB3F9A8-B-F]` 3 float 名称
  6. ALCH flags bit0 set/clear スタブ（0x4129C1 帯）の vtable 帰属
  7. GECK/CEF 正式名照合（BOOK DATA bit0・MGEF bit24）
- 継承確定事項（v256）: MGEF マージ = `FUN_0044DCF0`→`FUN_004168A0`→map 0xB33508→`FUN_00447050`(vt[0x1C])／type 5=椅子系・0xA=ベッド系／`FUN_006575B0`=HighProcess vt[0x42C]／vt[0x420]=`FUN_006294B0` 純セッタ
- 契約: 分析専任（docs/GT/TODO/plan/session-memory/cache・ソース編集禁止・既存 tmp/re 上書き禁止・訂正はドラフトのみ）
- 手法: capstone 5.0.7（v172_common.py 経由・VA→raw はセクション個別換算＝imm32_refs3 準守）＋ MSVC RTTI（v227_lib.py）＋ `docs/Oblivion_decompiled.c` 部分検索（読取のみ）
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）
- 中間物: `%TEMP%\opencode\v257_*.py`（v256 系の再利用優先）
- 注: 本ファイルは v256 §7「新規残点（次束候補）」1..7 の逐次解消記録（1項目完了ごとに追記）

## 1. 【低1】fresh 分岐 `FUN_00451210` の byte[form+4] 書込と formType バイトの両立 — **[確定]**（v256 §1.4「mod index 格納説」は**棄却**）

### 1.1 実測の書込鎖（`v257_1_out.txt` 逐語）

- `FUN_00451210(file=ecx, form=arg1)`（fresh 分岐の formID 割当）:
  ```
  eax = [file+0x23C]（現レコードヘッダ4CC）→ push → call FUN_0046AF70
  0x45125A: mov byte[form+4], al            ; ★ 書込対象 = 返値の低バイト
  0x451251-73: [form+8] = [file+0x244]（file flag bit0xE 立then or 0x4000）
  0x451279-84: SetFormID(FUN_0046C300, [file+0x248], 1)
  0x45128C:   call FUN_0046B6C0(form, file) ; file リンク
  ```
- **`FUN_004511C0`（CreateForm の switch 引数生成）の末尾は 0x4511F4-0x451204 で上記と同一呼び**（`[file+0x23C]`→FUN_0046AF70→eax→ret）。つまり **switch(ebx) の ebx と byte[form+4] の al は同一関数の同一返値** — 両立どころか「同じ値の二重書込」。

### 1.2 `FUN_0046AF70` の正体 = **4CC→formType 変換（フォーム種別レジストリ走査）** — 確定

- 本文（`v257_1_out.txt`）: `edx==[0xB33C20]`（1エントリ cache）なら `[0xB33C1C]` を返す；そうでなければ **`ecx=0xB05E08` から stride 0xC で `0xB06144` まで線形走査し、`[entry+0]==edx` の最初の index を eax=返却**（最大 69 エントリ・不発時 0）。
- `edx = [file+0x23C]` は現レコードヘッダの**4CC dword**そのもの: `FUN_00450080` @0x45009D-0x4500AD が **20 バイト（0x14）ヘッダを `[file+0x23C]` に生読**み（fourcc/size/flags/formID/VCID・`FUN_00450F10` @0x450F22-0x450F2A で `[file+0x23C]` と `[0xB05E20]`（='GRUP' エントリの 4CC スロット・下記）比較＝GRUP 特判と整合）。

### 1.3 テーブル `0xB05E04` の実体（**formType レジストリ・69 件**）— 確定（`v257_1b/1c_out.txt`）

- 画像初期値（hexdump `0xB05E04..`）: エントリ = **stride 0xC × 69（0x45 件）**＝`{+0: name ptr（.rdata 4文字列）, +4: 4CC dword（画像では 0＝実行時合成）, +8: 序号（i+1 連番・静的）}`。
- 初期化書手 = **TESForm base ctor `FUN_0046C460`**（全 form ctor が呼ぶ共通基底）@0x46C49A-0x46C52D: one-shot（guard `[0xB33C10]`/`[0xB06B18]`）でループ変数 `ebp=0xB05E04`、name ptr 4 文字から **`[ebp+4]` に 4CC dword を合成書込**（`b3<<24|b2<<16|b1<<8|b0` ＝メモリ順序そのまま＝little-endian fourcc）。重複検出警告 0xA3BF38/0xA3BF78 付き。
- name ptr 列デコード（`v257_1c_out.txt`）: index 0='NONE', 1='TES4', 2='GRUP', 3='GMST', 4='GLOB', 5='CLAS', 6='FACT', 7='HAIR', 8='EYES', 9='RACE', 10='SOUN', 11='SKIL'…**正統な Oblivion formType enum と一致**（⇒ **0x0C=MGEF** の v256 確定と自己整合）。
- 逆引き使用実測: `FUN_0046B990`（WriteRecord ヘッダ生成）@0x46B9CF-0x46B9DD = `movzx ecx,byte[form+4] → [ecx*0xC+0xB05E08]（=entry+4 の 4CC）→ ヘッダ dword0 へ` ＝ **formType→4CC**（走査の逆写像が `entry+4 = 0xB05E08+12i` であることの直接証拠）。
- 警告窓 `0x44DDCE-0x44DDDC` の「Should be/Is %s」は `[idx*0xC+0xB05E04]`（=name ptr）読＝**v256 §1.1 の注記「mod 名は 0xB05E04[]」は誤り**（正: formType の 4CC 名文字列。mod 名引数は別出所）。

### 1.4 裁定（両立機構の完結）

- **byte[form+4] = formType**。書込の一生: ①base ctor `FUN_0046C460` @0x46C557 が **0（'NONE'）初期化 → ②type 固有名 ctor が自 formType をハードコード**（EffectSetting: `FUN_00416430` @0x416544 `mov byte[esi+4],0xC`／LIGH: 0x4B1F90 の 0x1A=v256 §5.6 既出）→ ③**fresh/読込時 `FUN_00451210` がレコード 4CC→registry index を再書込**（正常系では ②と同値の冪等上書き。CreateForm 入口の formType 不一致ガード @0x44DDC6-0x44DDCC はこの同一性を前提とした検査）。
- ⇒ **矛盾なし・両立確定**。v256 §1.4 の「返値＝プラグインテーブル 0xB05E08 線形 index＝mod index 格納説」は**棄却**（0xB05E08 はプラグイン表ではなく formType レジストリ）。
- **新規確定 VA/データ**: `FUN_0046AF70`＝**GetFormTypeByFourcc**（cache 変数 `[0xB33C1C]`=index/`[0xB33C20]`=4CC）・`FUN_004511C0`＝TESFile::GetCurrentRecordFormType・**formType レジストリ 0xB05E04（69 項・{name,4CC,seq}・base ctor 初期化）**・`FUN_0046C460`＝TESForm base ctor（+4=0/+8=8/+0xC=0 初期＋レジストリ one-shot 初期化）・`FUN_0046B990`＝form→ヘッダ 4CC 書出。

証跡: `v257_1_out.txt`（FUN_00451210/46AF70/4511C0/46C300/416430 窓）、`v257_1b_out.txt`（FUN_004511C0 全文・FUN_00450F10/450080・imm32 refs 0xB05E04/08/0x6144・hexdump・警告文字列 2 本・ctor byte[+4]=0xC @0x416544）、`v257_1c_out.txt`（name 列デコード NONE..SKIL・FUN_0046C460 全文・FUN_0046B7A0/B910/B990・警告窓・FUN_00450980）。

### 1.5 本項目内残点

- レジストリ 69 項の index 12..68 の 4CC 名（formType 正表 26 件超過分の全体リスト化）— 次項 2 で転用可・未網羅。低。
- `FUN_0046B910`（ret 0xC の述語・file==GRUP エントリ && flags==0 && fourcc==自 type の 3 条件）の呼び手意味。低。

## 2. 【低2→解消】CreateForm case 0x03（0xB35468 vt[0x24]）の所属 — **[確定]**（= GMST / `GameSettingCollection`）

### 2.1 dispatch の index 基準訂正（v256 §1.1 読解の補強・新規実測）

- CreateForm 実 dispatch 式（@0x44DE1B-0x44DE2E 逐語）: `lea eax,[ebx-1]; cmp eax,0x42; ja default(0x44E5A5); movzx ecx,byte[eax+0x44E6DC]; jmp [ecx*4+0x44E6A0]` ＝ **byte テーブルは formType−1 索引（67 項＝ft 0x01..0x43）**。これで case 値とターゲットの全対応が復元できる（v256_7 の 15 ターゲット実測と矛盾なし）。
- 全 67 項の対応を実測（`v257_2c_out.txt`）:
  | byte | target | formType（レジストリ 4CC 名で注釈） |
  |---|---|---|
  | 0 | 0x44DE35 | TES4 |
  | 1 | 0x44E326 | **GMST（本項目）** |
  | 2 | 0x44E578 | generic: GLOB..SOUN・SCPT..BSGN・WTHR/CLMT/REGN・QUST・PACK/CSTY/LSCR・ANIO/WATR/EFSH |
  | 3 | 0x44E512 | SKIL |
  | 4 | 0x44E367 | **MGEF（v256 §1.1 の経路・再確認）** |
  | 5 | 0x44DEF0 | CELL |
  | 6 | 0x44E001 | REFR/ACHR/ACRE |
  | 7 | 0x44E2A5 | PGRD |
  | 8 | 0x44E401 | WRLD |
  | 9 | 0x44E237 | LAND |
  | 10 | 0x44DE83 | ROAD |
  | 11 | 0x44E474 | DIAL |
  | 12 | 0x44E4C9 | INFO |
  | 13 | 0x44E556 | IDLE |
  | 14 | 0x44E5A5 | default: GRUP・ACTI..LVLC(0x12..0x2B)・TLOD・LVSP（別 loader 所管／未知 4CC は警告） |
- ⇒ 項目 1 のレジストリ index==formType==4CC の自己整合が全 case で成立（ft 0x0C='MGEF'・ft 0x03='GMST' を name ptr 列で直接確認・`v257_2_out.txt` 0x00..0x0F 列挙）。

### 2.2 case 0x03 窓 @0x44E326 の実体（逐語・`v257_2b_out.txt`）

```
if (FUN_004510B0(file) == 'EDID') {        ; 0x44494445
    eax = [file+0x254]（subrecord 長）→ call 0x983FA0（バッファ確保?）
    FUN_00450C20(file, buf)               ; EDID 名読出（MGEF case と同一 helper）
    edx = [0xB35468]; eax = [edx+0x24]    ; ★ static オブジェクト自身の vtable
    push buf; push file; ecx = 0xB35468; call eax
}
return 1                                   ; ★ form 生成なし（FUN_00447DF0/447050/44D950 を経由しない）
```

### 2.3 `0xB35468` の所属 — **`GameSettingCollection` static シングルトン**［確定］

- **.CRT 初期化子 @0x9E06B0**（`v257_2d_out.txt` 逐語）: `push 0x191; mov ecx,0xB35468; call FUN_004A8330`（ctor・obj size 0x190）→ **`mov dword[0xB35468], 0xA4049C`**（完成 vtable 昇格・ctor 内 @0x4A8376 の暫定 store 0xA40474 の上書き＝MSVC 静的オブジェクト定型）→ dtor thunk `0xA1AA00`（`mov ecx,0xB35468; jmp FUN_004A8180`）を `FUN_00981FB4`（atexit 登録）へ。
- **RTTI 実デコード**: COL `[0xA4049C-4]=0xAB9CCC` → **`.?AVGameSettingCollection@@`**（`v257_2e_out.txt`）。
- slot 実測: `+0x10=FUN_004A7D60 / +0x14=0x4A7CA0 / +0x18=0x4A7CA0 帯 / +0x1C=0x4A81F0 / +0x20=0x4A8290 / +0x24=FUN_004A8040`（+0x28 以降は 'MAIN' 文字列直後でテーブル終端）。

### 2.4 `vt[0x24] = FUN_004A8040` の意味（EDID キーの名前引きマージ・逐語）

- `[this+0x108] = file` 保存／`byte[this+4] = 0`（dirty クリア）／**自前マップ `[this+0x10C]` へ `FUN_0055E000(map, EDID 文字列)` で Find**（全局 formID マップ `0xB0613C` と同一の汎用二分探索・MGEF の `0xB33508` と同型設計）／ヒット時 `this->vt[0x10](setting*)`（＝個々の GameSetting への ReadRecord 委譲・返値を bl に保存）／終了時 `this->vt[0x18]()`（後処理・file=0 解除相当）／`ret 8`。
- ⇒ **裁定: case 0x03 の EDID 別経路は「GMST レコードを TESForm として生成せず、static `GameSettingCollection`（0xB35468・vtable 0xA4049C・RTTI 実デコード済み）の名前マップで既存設定にマージする経路」**。MGEF case の code キー再利用と镜像の関係（GMST は EDID 名前キー／MGEF は 4 文字 code キー）。v256 §1.1 の「本件と無関係・未追跡」を**解消**。

証跡: `v257_2_out.txt`（レジストリ 0..15 名・jump 表・B35468 参照 5 件・画像値 0）、`v257_2b_out.txt`（switch 計算窓・case 窓・FUN_009E0630・A1AA00）、`v257_2c_out.txt`（全 67 項対応表）、`v257_2d_out.txt`（0x9E06B0 ctor ストア窓・atexit 群）、`v257_2e_out.txt`（RTTI '.?AVGameSettingCollection@@'・slot 一覧・FUN_004A8040 全文・ctor 冒頭）。

### 2.5 本項目内残点

- `GameSettingCollection::vt[0x10]`（=FUN_004A7D60・setting への ReadRecord 委譲）の内部と GameSetting 個体レイアウト（name@+0x4 文字列説）。低。
- レジストリ index 0x44..0x68（44..104）の 4CC 名が formType enum 公式範囲（0x43 まで）外に存在する意味（SVR 系/未使用スロット）。低。

## 3. 【低3】`vt[0x90](1)` の意味・`FUN_00447050` の form+8 bit0 ガード両側の消費者 — **[確定]**（機構完解明）

### 3.1 FUN_00447050 後段の逐語（`v257_3_out.txt`）

```
esi=form／edi=file
bl = byte[form+8] & 1          ; ★ ReadRecord 前に bit0 を捕捉
form->vt[0x1C](file)           ; ReadRecord（返値 al をスタック退避）
arg = bl ? 1 : FUN_0044FAA0(file)         ; = [file+0x3DC] bit0
FUN_0046A9A0(form, arg)        ; [form+8] bit0 = arg（or 1 / and ~1 の純セッタ・ret 4）
if (FUN_0044FAF0(file))                   ; = ([file+0x3DC]>>3)&1
    form->vt[0x90](1)          ; ★ 本残点の呼び
return ReadRecord 返値
```

### 3.2 `vt[0x90] = FUN_0046C640` の実体 — **「変更フォームリスト登録（dirty マーキング）」**［確定］

- **TESForm 基底共通実装**（EffectSetting 0xA32B14+0x90 と TESObjectLIGH 0xA4319C+0x90 が同一 VA 0x46C640＝派生上書きなし・`v257_3_out.txt`）。
- arg!=0 側（`v257_3c_out.txt` 全文）: bit1 立旗 or **bit0xE 立旗（=ローカルフォーム）**なら非積込で `or [form+8],2` のみ；そうでなければ全局リスト **`0xB0614C`**（data `[0xB06150]`・size `[0xB06158]`・used `[0xB0615C]`・vtable 0xA37AAC を静的初期化 `FUN_00A18500`@0xA18506 で実測）へ線形去重後 `FUN_00449070`（push_back・**E8 呼び手は 0x46C68A のみ＝登録経路は vt[0x90] 一意**）で積込、bit1 立旗。
- arg==0 側: bit1 立旗時だけリスト走査除去（slot=0・used--・末尾なら size--）→ bit1 クリア。**完全な対称 remove 分岐**。
- 基底 ctor での初期状態実測: `FUN_0046C460` @0x46C55B `[form+8] = 8`（bit3 既定立）、@0x46C557 `byte[form+4]=0`（項目1 と対）。
- 直接 E8 呼び 5 件（vtable 迂回・基底実装直叩き）: 0x4BF110/0x4CCBC0/0x4D6DB0/0x4E8BF0/0x5306D0（`v257_3c_out.txt`）。
- リストの読手（消費者）実測 2 系統:
  - `FUN_004492E0`（DataHandler 系・E8 callers `FUN_004466F0`/`FUN_00464440`）: `[mgr+0xCD4]=1` → **リスト全 slot ゼロクリア + size/used=0** + loaded file リスト `[mgr+0x64]` 各 link の `vt[0x10](1)`（クローズ）＋ 0x15×0x60B 配列リセット。＝ reload/save 後の dirty フラッシュ。
  - `FUN_0046B2C0`（フォーム解除経路・bit0xE 非立時のみ）: formID を全局マップ `0xB0613C` から remove、**リストからも除去**、DataHandler `0xB33A98` 側 list から `FUN_00446B40` 除去。

### 3.3 form+8 bit0 の「両側消費者」裁定 — **[確定・消費者はガード自身のみ]**

- 書手全集: ①`FUN_0046A9A0`（E8 2 件＝`FUN_00447050` 後段と **CreateForm TES4 case @0x44DF52** の `vt[0x90](1)`＋`push 1→46A9A0`＝プラグインヘッダ form は bit0 強制立旗・`v257_3d_out.txt`）／②`FUN_00451210` の dword 一括書込（[form+8]=[file+0x244] ＝レコード FLGS 写し・項目1 実測）。
- 読手全集（3 パターン走査・すべて .text 全域）:
  - test/or/and `[base+8]×imm{1,0xFE}` 直接形: **有効ヒット 0**（`[esp+8]` 系はスタック引数検査で別物・`or byte[esi+8],1` @0x425B0D は MagicItem X* subrecord 側の ExtraData エントリで TESForm でないことを FUN_004259E0 冒頭（XCRM/XLOC 等 fourcc switch）で確認・`v257_3f_out.txt`）。
  - `mov reg,[base+8]`→`and/test reg,1` 相関形 + `bt [base+8],0`: **0 件**（`v257_3e_out.txt`）。
  - 唯一の読=FUN_00447050 冒頭 `bl=[form+8]&1`（=ガード自身）。
- ⇒ **bit0 の機能 = 「一度でも active file（file+0x3DC bit0 立）から読込まれた」sticky ビット**。ガード両側は「旧 bit0 立→1 固定（後続の非 active file でクリアさせない）／旧 0→当該 file の bit0 を写す」の自己保存設計のみ。下流効果は bit0 自体ではなく **bit1（リスト入り）と file bit3 の組合せ**で発火する（§3.2）。
- file+0x3DC ビットの意味側証: bit0 getter `FUN_0044FAA0` **E8 35 件**（DataHandler/Save/Load 機構全域の「この file は loaded」述語）、bit3 getter `FUN_0044FAF0` **E8 6 件**（FUN_00447050 後段・LoadFiles 本体 `FUN_0044F3D0`@0x44F59E・save 窓 0x4C7090/0x4E74E0/0x4E7690/0x4E7950）＝「読込中に dirty 収集を起動する file 側スイッチ」。

### 3.4 裁定まとめ（移植注記）

- MGEF マージ経路の終端動作が確定: 「登録済み EffectSetting へ ESM 値をマージ → file bit3 立なら **その効果をグローバル変更リスト 0xB0614C へ登録**（保存対象化）。内蔵フォーム（bit0xE 立＝local）はリスト積込なしで bit1 だけ立つ」。
- GT 反映ドラフト（§6 (B) 参照）。

証跡: `v257_3_out.txt`（447050/46A9A0/44FAF0 逐語・vt[0x90]=46C640 共通確認）、`v257_3b_out.txt`（list refs・bit0 直接走査）、`v257_3c_out.txt`（46C640 全文/4492E0/46B2C0・E8 callers）、`v257_3d_out.txt`（TES4 case 窓・getter callers・A18500）、`v257_3e/3f_out.txt`（相関 0・4259E0 別物確認）。

### 3.5 本項目内残点

- dirty リスト 0xB0614C を「保存時に読む」経路が E8/imm32 走査では見えない（0x4492E0 のゼロクリアしか読手なし）— 保存本体は変更フォームを別機構（file 側の editor list ?）で列挙している可能性。低。
- `FUN_004492E0` の mgr（esi+0xCD4）= DataHandler(0xB33A98) か別シングルトンかの RTTI 確定。低。

## 4. 【低4】HUD 末尾 DOOR/BoundObject 追加列と `[0xB3B33C]`/`[0xB3B350]` の初期化・RTTI — **[確定]**（v256 §2.3 記述に訂正1件）

### 4.1 `[0xB3B33C]` = **HUDInfoMenu シングルトン**［確定・RTTI 実デコード］

- 初期化書手: **`FUN_005A4660`**（thiscall ctor）@0x5A4660-0x5A46A0 逐語: base ctor `FUN_00584640` → **`mov dword[esi], 0xA6BDF4`（vtable store）→ `mov [0xB3B33C], esi`** → フィールド `+0x28..+0x58` を 0 クリア（v256 §2.3 の [esi+0x28]/[esi+0x48]/[esi+0x4C] はこの 0 クリア群に含まれる＝実行時submenu接続で埋まる）。
- 呼び手: `FUN_00587D20` @0x587ECC のみ（メニュー工場帯）。解除: **`FUN_005A4810`**（dtor・同 vtable 再 store→`[0xB3B33C]=0`→base dtor `FUN_00585330`→delete 条件分岐）。
- **RTTI: COL `[0xA6BDF0]=0xAC22A8` → `.?AVHUDInfoMenu@@`**（`v257_4c_out.txt`）。vtable slot +0x2C = **0x5A5540** ＝ v256 §2.2 の「HUDInfoMenu vtable slot 0x2C」帰属と自己整合✓。
- セッタ群実測（`v257_4d_out.txt`）: `FUN_005A47B0`（[obj+0x50] タイル read→FUN_00588CF0）/`FUN_005A47E0`（[obj+0x54]==arg でクリア）/`FUN_005A4840`（[obj+4] 経由 vt[0](1)＝HUD アクチベータ列）。⇒ **+0x28/+0x2C/+0x48/+0x4C/+0x50/+0x54 = タイルインターフェース ptr 群**（0xFDE/0xFA1/0xFAF 等への書込先）。

### 4.2 `[0xB3B350]`（+54/+58）= **hud_reticle インターフェース3連キャッシュ**［確定］

- 書手: **`FUN_005A8000`**（E8 callers **1 件＝`FUN_0057D940`@0x57D9F5**）@0x5A800C-0x5A803B 逐語: `eax=FUN_005894F0(arg,1)→[0xB3B350]`／`(arg,2)→[0xB3B354]`／`(arg,3)→[0xB3B358]`。
- `FUN_005894F0(x, i)` = **タイル 0xFA8 の float 値==i で子チェーン（`[x+0x34]` 巡回）を名前解決するインターフェース探索**（0x5894F0-0x58953B 冒頭実測・tile read `FUN_00588BD0(0xFA8)`）。
- 供給元 `FUN_0057D940` @0x57D9E4: **`push 0xA68D7C = 'Data\Menus\Main\hud_reticle.xml'`** → `FUN_00590420`（メニュー XML 解決）→ 結果を `[esi+0x80]` 保存 → `FUN_005A8000` 呼出（`v257_4d_out.txt`）。⇒ **v256 の「HUD クロスヘア側シングルトン・RTTI?」への回答:  dedicated クラスの静的シングルトンではなく、hud_reticle.xml インスタンスから解決したインターフェース triple のキャッシュ**（格納型はチェーン要素＝メニュー UI オブジェクトと同一。専用 RTTI 無し＝書込窓が单一の代入のみで確認）。
- 読手 band: `FUN_005A8710`（交差更新・[0xB3B350]&[0xB3B354] ゲート→0xFA1 両書込＝v256 §2.4 の「書手 band 0x5A8000-0x5A8710」の正体）/`FUN_005A8BC0/005A8C90`（dead 列）。

### 4.3 HUD 末尾 DOOR 列 @0x5A4E91 の全文（v256 §2.3-5 の細部解消）

- ゲート: `ebp=[esi+0x4C]`==0 → 末尾ブロックへジャンプ（0x5A4FA8）。
- DOOR 窓: **`ref->vt[0x170](0, TD→0xB05480, TD←0xB03248, 0)`** ＝ TD 対デコード実測 `0xB03248=.?AVTESBoundObject@@` → **`0xB05480=.?AVTESObjectDOOR@@`**（`v257_4_out.txt` 7 TD 全デコード済み）。
- 列（door!=0 時）: `FUN_00428E70(door)` 述語（鍵プロパティ相当）→ `FUN_004DE770(ref, Player(0xB333C4), 1)` 真→1.0 経路 → `FUN_004B72C0(ref, Player, 0, 1)` 真→1.0 経路。**通過（＝鍵必要ドア）のみ**: `FUN_004D7780(ref)`（ref+0x44 ExtraData 走査で**キー formID** 取得・`FUN_0041E690` 経由）→ `FUN_004299E0(id)`（閾値 0xB338B8/C0/C8… の id→テーブル index 変換）→ **[index*4 + 0xB03E1C] = グローバル文字列表（キー名）**→ `'%s'`(0xA2F864) 整形 → **[esi+0x4C] 経由タイル 0xFDE=キー名**／`fild` で**タイル 0xFAF=float(keyID)**（0x5A4F7A）／0xFA1=2.0(0xA379B4)、1.0 経路は 0xFA1=1.0。
- 共通終端 @0x5A4FA8〜（DOOR と無関係の final 列・新規確定）: `ref->vt[0x170](0)`（=表示名 getter 相当）→ `FUN_00484420`（文字列複製）→ 空なら 8B ノード確保（{0,0}）→ **`FUN_00446CB0` で `[ref+0x44]` リスト前方挿入**（MGEF fresh と同一 helper）→ `FUN_0048F450/FUN_004028D0` 整形 → **[esi+0x2C] 経由タイル 0xFDE 書込**。関数終端 0x5A5029 付近。
- **訂正（v256 §2.3 記述）**: 「末尾 DOOR/BoundObject/**ArrowProjectile** 追加列」中の ArrowProjectile TD（`0xB05AD0=.?AVArrowProjectile@@`）は **FUN_005A4980 の末尾には存在しない**（窓内 push 走査で 0xB05480/0xB03248 の 2 件のみ・`v257_4e/4f_out.txt`）。0xB05AD0 キャストは §2.4 の **`FUN_005A8C90` 内 @0x5A8D5D**（dead 列の BC0 後ロジック）に属する。v256 の「末尾＝ドア/武器系の特記ラベル列」の"武器系"側は FUN_005A8C90 の誤記（末尾列はドア専用）。

証跡: `v257_4_out.txt`（imm32 refs 両全局・画像値 0・末尾窓・TD 7 件デコード）、`v257_4b_out.txt`（参照指令方向・FUN_005A8000 全文・4660 ctor）、`v257_4c_out.txt`（RTTI '.?AVHUDInfoMenu@@'・callers・5894F0 冒頭・5A8710 窓・4810 dtor）、`v257_4d_out.txt`（0xFDE/0xFAF/0xFA1 末尾列・'hud_reticle.xml'・4D7780/4299E0・slot+0x2C=5A5540）、`v257_4e/4f_out.txt`（push 0xB05AD0=0x5A8D5D 1 件のみ確認）。

### 4.4 本項目内残点

- HUDInfoMenu の +0x2C/+0x4C（DOOR/終端用インターフェース）に何を接続する書手（メニュー XML 解決側の setter）特定。低。
- `FUN_004299E0` 閾値群（0xB338B8/C0/C8…）と `0xB03E1C` 文字列表の全長（キー名テーブルの実体一覧化）。低。

## 5. 【低5】`vt[0x164]` 返り値 +0xC4/BSAnim 帯 RTTI と `[0xB3F9A8-B-F]` 3 float の名称 — **[確定]**（v256 §3.5「寝姿勢カメラオフセット推定」は**訂正**）

### 5.1 `vt[0x164]` の実体（Actor vtable 0xA6E074・cache vtable_index で取得）

- 返り値の供給関数 = slot +0x164 = **`FUN_004D8370`**。Actor 主 vtable 0xA6E074 の隣接 slot 実測（`v257_9_out.txt`）: **+0x160 = `FUN_004E3490`（0xDC struct を new する Actor Init 仮想そのもの）**・+0x168=0x6F7070・+0x16C=`FUN_0068F970`（§6 の no-op スタブと同一関数）。PlayerCharacter 0xA73A0C 側は **+0x164=`FUN_0065D670`（byte 引数形・FurnitureManager 0xB3BD00 と [PC+0x608]/[+0x610] を触る別挙動＝Actor 側 getter と署名衝突。PC が家具経路でこの slot をどう運用するか未解決＝残点）**・+0x160/+0x168=0x65FDA0・**+0x14C=`FUN_00667BE0`（0xDC struct の PC 側生成窓・§5.2 の callers と直結）**。Actor/PC 主 vtable 239 slot 中 **98 slot 差**。逐語（`v257_5c_out.txt`）:
  ```
  if (byte[actor+4] − 0x32 > 1) return 0          ; formType ∈ {0x32 ACHR, 0x33 ACRE} のみ
  ecx = [actor+0x58]（process）; if (!ecx) return 0
  if (process->vt[0x08]() > 1) return 0
  return [process + 0x17C]                         ; ★ アニメ状態構造体 ptr
  ```
- `[process+0x17C]` の書込窓（disp32=0x17c のバイト直撃走査・線形同期ズレを回避・`v257_5s_out.txt`）: process 帯に**清零 4 件**（`FUN_0064B400`@0x64B51B／`FUN_0064FFF0`@0x650017・家具直前関数／`FUN_00651F70`@0x6520FB／swap `FUN_0064B650`@0x64B6B6-6BC）＋読手多数（0x62-0x65 帯）。割当側は Actor Init `FUN_004E3490`（後述）。
- ⇒ v256 §3.3 の `ebp = actorRef->vt[0x164]()` ＝ **HighProcess+0x17C のアニメ状態構造体**（ACHR/ACRE＝配置済み actor 限定）。

### 5.2 返り値クラスの RTTI 裁定 — **「vtable 無し＝非ポリモーフィック struct・RTTI 不能」［確定]**

- `new(0xDC)`（`push 0xDC; call FUN_00401F00` @0x4E3813-0x4E3820 in Actor Init `FUN_004E3490`・@0x667E30 in process 帯）→ ctor **`FUN_00473EB0`**（E8 callers 4: 0x4E3490×2・`FUN_00667BE0`・`FUN_00668D00`）:
  - 初期化（`v257_5o_out.txt` 逐語）: `dword[0]=0`（**vtable 格納なし**）、`[+4]/[+8]/[+98]/[+B4]/[+B8]/[+CC]/[+D0]/[+D4]/[+D8]=0`、**`[+0xC..0x14] と [+0x18..0x20] を [0xB3F9A8/AC/B0] 3 連写込（×2組）`**、`+0x38/+0x94=0.0f`、`byte[+0x90]=0xFF`、`byte[+0xC4]=0`。
- 4 ヘルパーは全部この struct の**非仮想メソッド**（.rdata store 0 件＝vtable 不在を imm32 走査で確認・`v257_5_out.txt`）:
  - `FUN_004711F0` = 「アクティブ sequence `[+0xCC]` が state**1**（実行中）か」；`FUN_00472EA0` = 「`[+0xD0]`(次) 無く、かつ `[+0xCC]` 無 or state**3**（完了）」；`FUN_004706E0(i)` = 「スロット配列 `[+0xA0+4i]`（特殊再索引 i==5→slot0・i==6→slot3・其它→slot i）」；`FUN_00477E50(x)` = 「`[+0xCC]+0x28=x` 後、`FUN_005EFFD0([+0xCC]+0x10, 0xB)` で sequence 名 id をキュー（停止要求相当）」。
  - **帯の系譜（RTTI 実デコード・`v257_5n_out.txt`）**: 0x470000-0x479000 帯の vtable 群 = `.?AVAnimSequenceBase@@`(0xA3C6E4) / `.?AVAnimSequenceSingle@@`(0xA3C72C) / `.?AVAnimSequenceMultiple@@`(0xA3C758) / `NiTListBase<PBVBSAnimGroupSequence>`(0xA3C6B4) / `NiTPointerListBase<...>`(0xA3C700) / `NiTList<PBVBSAnimGroupSequence>`(0xA3C748) / `NiTMapBase<...AnimSequenceBase>`(0xA3C6C4) / `NiTPointerMap<...>`(0xA3C8A0) ＝ **v256「BSAnim 推定帯」は正しく、正確には AnimSequenceBase/BSAnimGroupSequence 系**（sub-object `[+0xCC]/[+0xD0]` = BSAnimGroupSequence*（+0 state・+0x10 名 id・+0x28 arg））。
  - **`[obj+0xC4]` byte = 「dynamic idle 失敗／位置リセット要求」フラグ**（家具経路 `FUN_00650030`@0x65046A・`FUN_00654F10`@0x654FD4帯で 1 設定・ctor で 0 初期化）。
- ⇒ 裁定: **クラス名は EXE に存在しない（RTTI 不在）**。機能名として **「ActorAnimState struct (0xDCC=220B) — AnimSequenceBase/BSAnimGroupSequence 管理ブロック」**と記録。HighProcess.vt[0x17C]=0x64F760（同 struct 参照メソッド）も slot 値実測済み。

### 5.3 `[0xB3F9A8/AC/B0]` 3 float = **静的ゼロベクトル（NiPoint3::ZERO 相当）**［確定・v256 推定を訂正］

- **書込元 0 件**（literal store 走査 mov/fstp 形・`v257_5j_out.txt`。前回の書式バグは再走査で修正済み。`.rdata/.data` 経由 descriptor 参照も 0 件）。BSS 既定値のまま＝**3 本とも 0.0f**。
- 隣接ブロックが **.CRT スタブ群で角度定数**と判明（`v257_5g_out.txt` 逐語＋`v257_5k_out.txt` double 実測）:
  - `[0xB3F9A4] = atan系(1.0)×4.0 = π`（stb @0xA09B90・double[0xA3C800]=**4.0** 実測・FUN_009870A0=MSVC _CI 数学ラッパ）
  - `[0xB3F9A0] = π+π = 2π`（@0xA09BD0）／`[0xB3F99C] = π×0.5 = π/2`（@0xA09BB0・double[0xA2FAA0]=**0.5** 実測）
  - **直後 0xB3F9A8..0xB3F9B0 = 書込み無し＝ゼロ 3 連**＝エンジン全域の既定位置ベクトル（**読手 407/324/323 件**・`v257_5_out.txt`。例: 本 struct ctor×2組、ExtraOblivionEntry ctor `FUN_0042A510` の +0xC..0x14 既定値、`FUN_00471640`@0x4716B9 が `lea esi,[ebp+0x30]` へ `push 0xB3F9A8→FUN_00471390`（リセット））。
- 家具失敗経路の実際の「位置/角度再計算」は `FUN_004D0C20` 窓で実証: `fld [0xB3F9A4]=π; fchs; fmul 0.5 → −π/2 pitch`＋**位置はゼロ 3 連でリセット**（`v257_5g_out.txt` 0x4D0C6F-0x4D0CAA）。
- ⇒ **v256 §3.5 「寝姿勢時のカメラオフセットと推定」は棄却**（値は 0 で固定、オフセットではない）。名称は EXE 内に非存在（記号文字列なし）だが、機能名 **「静的原点ベクトル（zero position）」**［確定級］。

証跡: `v257_5_out.txt`（4関数の .rdata store 0＝非仮想・3 float 参照数）、`v257_5b/5c_out.txt`（4関数本文逐語・vt[0x164]=4D8370・callers 帯）、`v257_5d/5e/5j_out.txt`（HighProcess ctor・store 走査 0 件＝BSS ゼロ確定）、`v257_5f/5g_out.txt`（.CRT 角度定数 stb・π/2π/π/2 算式・4D0C20 −π/2 窓）、`v257_5h/5i/5k_out.txt`（double 4.0/0.5 実測・使用形態内訳・ExtraOblivionEntry 窓）、`v257_5m/5n_out.txt`（Anim 帯 ctor 群・vtable RTTI 13 本デコード）、`v257_5o/5p/5q_out.txt`（FUN_00473EB0=0xDC ctor 全文・callers 4・4D83B0 setter・swap/clear）、`v257_5r/5s_out.txt`（disp32=0x17c バイト直撃・ST 11 件一覧）。

### 5.4 本項目内残点

- Actor Init `FUN_004E3490` 内で new した 0xDC struct を `[process+0x17C]` へ_link_するExact 書込点（4D83B0 の後部窓が未展開・swap/clear しか実測せず）。低。
- `FUN_004706E0` の再索引（5→slot0・6→slot3）の意味（スロット意味表＝idle/locoment/... 対応）。低。
- AnimSequenceBase/BSAnimGroupSequence 帯の正式フィールド名表（本束は state/+0x10 名 id/+0x28 arg まで実証）。低。
- **PlayerCharacter オーバーライド衝突**: PC 0xA73A0C+0x164=`FUN_0065D670` は byte 引数・FurnitureManager 触りの別挙動で、 Actor 側 getter（no-arg 返値）と署名が合わない（家具 tick の PC 経路がどのオブジェクト/vtable を通るかの実測未了）。低。

## 6. 【低6】ALCH flags bit0 set/clear スタブ（0x4129C1 帯）の vtable 帰属 — **[確定]**（= AlchemyItem/IngredientItem 主 vtable **slot+0xE8**）

### 6.1 関数同定（`v257_6b_out.txt`・整列再復唱）

- 先頭は **0x4129C0**（v256 の「0x4129C1 帯」は 1 バイト誤差——capstone 整列復唱で確定）。本文 6 命令:
  ```
  0x4129C0: cmp byte ptr [esp+4], 0
  0x4129C5: jne 0x4129CE
  0x4129C7: or  byte ptr [ecx+0x7C], 1      ; arg==0 で bit0 SET
  0x4129CB: ret 4
  0x4129CE: and byte ptr [ecx+0x7C], 0xFE   ; arg!=0 で bit0 CLEAR
  0x4129D2: ret 4
  ```
- ⇒ **ENIT flags byte（+0x7C・v256 §5.1 の {cast type@+0x78, flags@+0x7C}）bit0 の boolean セッタ**（引数 真=クリア／偽=セット＝「disable 系」語義の反転引数）。

### 6.2 vtable 帰属（v256 の「参照未追跡」を解消）

- `imm32_refs3(0x4129C0)` ＝ **2 件とも .rdata**: **0xA32264 / 0xA34044**。
- cache vtable_index と RTTI 実デコードで帰属確定: **0xA3217C=`.?AVAlchemyItem@@`（72 slots）・0xA33F5C=`.?AVIngredientItem@@`（72 slots）** の各 **+0xE8**（0xA3217C+0xE8=0xA32264 ✓・0xA33F5C+0xE8=0xA34044 ✓）。
- E8 直接呼び **0 件**（純仮想ディスパッチ・v256 再確認）。

### 6.3 slot+0xE8 は TESForm 族の共通仮想スロット（実装は型別）

- 並列 slot 実測（`v257_6b/6d_out.txt`）: **BOOK/LIGH/MISC = 0x68F970（純 `ret 4` no-op スタブ）**、**WEAP = 0x46C2F0**、**ARMO = 0x69D990**、**STAT = 0x4A8F90**、**ALCH/INGR = 0x4129C0（本件）** ⇒ 「フォーム種別ごとの +0x7C 系 flag accessor」スロット（ALCH/INGR だけが ENIT flags bit0 に結び付く）。
- **.text 全域の vt[0xE8] ディスパッチ窓**（`mov r,[vtbl+0xE8]; call r` 全列挙・`v257_6d_out.txt` 8 件）:
  - `FUN_00425040`@0x4251B8/0x425214 ×2（**ExtraData 0x1F/0x23 の base form へ対呼び**・MagicItem 系・直前に `FUN_0041E210(list,0x1F/0x23)`＋DataHandler `FUN_00446B80` チェック）
  - `FUN_005E12B0`@0x5E12BE（§3.3 の actor 側 getter と同一関数）／`FUN_0060D780`@0x60D7DB／`FUN_0061CE40`@0x61D0EB／`FUN_006516D0`@0x651718（process 帯）／`FUN_00773920`@0x77393C／`FUN_00774370`@0x77438C（UI/メニュー後方帯）
- ⇒ **実呼び出し経路あり（ディスパッチ 8 窓）で確定**。v256 §5.1 の「直接参照 0 件（vtable ストア経由の可能性あり）」は「.rdata ストア 2 件＋仮想窓 8 件」に置換。

証跡: `v257_6_out.txt`（窓・imm32 2 件・cache vtable 一覧）、`v257_6b_out.txt`（0x4129C0 整列本文・slot+0xE8 並列・RTTI '.?AVAlchemyItem@@'/'.?AVIngredientItem@@'・E8 0 件）、`v257_6c/6d_out.txt`（disp32=0xE8 byte 直撃 172 件から call 窓 8 件抽出・FUN_00425040 窓・callers）。

### 6.4 本項目内残点

- 8 ディスパッチ窓それぞれの引数値（disable=真 でクリアの呼び側意味）と UI 側の対応（enchant/alchemy メニュー）。低。
- WEAP `0x46C2F0`/ARMO `0x69D990`/STAT `0x4A8F90` の +0xE8 実装内容（flag 対象オフセット確認）。低。

## 7. 【低7】GECK/CEF 正式名照合（BOOK DATA bit0・MGEF bit24）— **BOOK [一次資料照合済み] / MGEF [未確認（取得不能継続）]**

### 7.1 BOOK DATA bit0 = **"Scroll"** — **[確定・照合済]**（2026-11-03 websearch 経由で UESP 本文抜粋を直接取得）

- 出典1: **UESP「Oblivion Mod:Mod File Format/BOOK」**（検索結果に表本文を含む全文抜粋が返却・en.uesp.net 直接 fetch は transport error×2 のため search 経由）:
  - `DATA (10B) = Flags(byte)／Teaches(byte)／Value(word)／Weight(float)`、**「Book flags: 0x0001 = Scroll, 0x0002 = Can't be taken」**。
- 出典2: **xEdit changelog**（Nexus/xEdit releases）: 「[FO3/FNV] **Flag 0 in BOOK means "Scroll"**」（TES4 系列で同語義の公式解読）。
- 出典3: **GECK Wiki「Book Menu Restored Plugin」**: "select the checkbox for **"scroll"** if desired" ⇒ GECK チェックボックス表示名＝**"scroll"**。
- **EXE 側との突合**: v242 §2.3 の「bit0=ENAM 付与持ちの書物を scroll 使用可にする述語（読手12件）」と**完全一致**（一次資料で語義確認完了）。さらに v256 §5.3 の `[book+0x89]=Teaches byte` 裁定も **「Set to 0xFF if no skill is taught」で相互確認**（UESP の 0xFF sentinel 記載＝EXE の無効 sentinel と一致✓）。
- GT 反映ドラフト（§8 (G)）: v256 §6.2 (AI) の「未照合」注記は **BOOK 分のみ解消**へ更新可。

### 7.2 MGEF flags bit24 の CEF/GECK 正式名 — **[未確定（Web 取得不能継続）]**

- 試行記録（2026-11-03）: en.uesp.net /wiki/…MGEF **transport error×2**、pt.uesp.net ×1、pt.m.uesp.net ×1（全て失敗・前回セッションの 503×4 と継続傾向一致）、tes5edit fopdoc ×1 失敗、websearch は Skyrim 版 MGEF の断片（0x1=Hostile 先頭列）のみ返却で **Oblivion bit24 のラベルは含まれなかった**。
- ⇒ **確認できず**。**断定は保留**（記憶由来の候補名は記録しない——捏造禁止ルール）。EXE 側の機構裁定は v256 §5.5 のまま有効: **bit24 = 「+0x60 は ActorValue（formID ではなく）」の宣言ビット**（消費実体 `FUN_00413130`@0x4131B4・EffectItem+0x14 交付）。GT 反映時は (AI) の pending タグを **MGEF のみ維持**。
- 参考（**未確認情報**としてのみ）: xEdit/UESP の Oblivion MGEF 表には 32 個の Flags チェックボックスが定義されており、bit24 は AV 使用系ラベルと推測されるが、一次資料未取得のため名称確定は次束へ持ち越し（uesp 到達回復時: en.uesp.net/wiki/Oblivion_Mod:Mod_File_Format/MGEF の Flags 表）。

### 7.3 本項目内残点

- MGEF flags 全 32 ビットの GECK 表示名表（uesp 到達時に一括取得）。低→**要 Web 再開**。
- BOOK bit1「Can't be taken」の EXE 側消費者（[book+0x7C] bit1 読手の対応確認・v256 §5.2 の読手12件とのマージ）。低。

証跡: websearch 結果ログ（本会話内・UESP BOOK 表全文抜粋＋xEdit changelog＋GECK Wiki 文）、fetch 失敗 5 件（en×2/pt/m/fopdoc）。

## 8. GT/plan ドラフト（反映は要承認・本体無編集）

> v256 §6 の (AG)-(AL) に対する本束の増分。反映前に GT 本体（docs/RE_EXE_REPORT.md 等）と行単位突合のこと。

### 8.1 v256 ドラフト行への訂正・更新

- **(AG) 更新（項目1）**: 「fresh 分岐 `FUN_00451210` の byte[form+4] 書込＝mod index 格納説（低確度）」→ **棄却**して次で置換: **0xB05E04 は formType レジストリ（69 項×stride 0xC＝{name ptr（4CC 文字列）, 4CC dword（TESForm base ctor `FUN_0046C460` が one-shot 合成）, 序号}）**。`FUN_0046AF70`＝**4CC→formType 変換**（1 エントリ cache `[0xB33C1C]/[0xB33C20]`）。`FUN_004511C0`（switch 引数）と byte[form+4] 書込は同一返値＝**+4 は両経路で formType**（base ctor 0 初期→type ctor ハードコード→読込時 4CC 変換で冪等再書込）。CreateForm 警告窓の `%s` 名は `[idx*0xC+0xB05E04]`（＝v256「mod 名」注は formType の 4CC 名に訂正）。
- **(AG) 追記（項目2）**: dispatch 式 **`byte[formType−1+0x44E6DC]→dword[×4+0x44E6A0]`（67 項全対応実測）**。case 0x03=**GMST**（EDID→static `GameSettingCollection` 0xB35468・vtable 0xA4049C・RTTI '.?AVGameSettingCollection@@'・vt[0x24]=FUN_004A8040 が自マップ +0x10C を EDID 名前引き→vt[0x10] 委譲、form 生成なし）。特殊窓: TES4=0x44DE35・SKIL=0x44E512・MGEF=0x44E367・CELL=0x44DEF0・REFR/ACHR/ACRE=0x44E001・PGRD=0x44E2A5・WRLD=0x44E401・LAND=0x44E237・ROAD=0x44DE83・DIAL=0x44E474・INFO=0x44E4C9・IDLE=0x44E556・default(0x44E5A5)=GRUP/ACTI..LVLC/TLOD/LVSP。
- **(AG) 追記（項目3）**: `FUN_00447050` 後段＝**`vt[0x90]`=TESForm 基底 `FUN_0046C640`「変更フォームリスト 0xB0614C（vtable 0xA37AAC・[0xB06150] data/[0xB06158] size/[0xB0615C] used）への dirty 登録＋bit1」**。local form（bit0xE）は積込スキップ。読手=DataHandler 系 `FUN_004492E0`（全消し）／`FUN_0046B2C0`（formID と共に除去）。**form+8 bit0 は読手=ガード自身のみ（.text 全域 test/and/or/bt/相関 走査で追加消費者 0 件）＝file+0x3DC bit0 の sticky 化専用**。TES4 case は bit0 強制立＋`vt[0x90](1)` @0x44DF40-52。
- **(AJ) 更新（項目4）**: **`[0xB3B33C]`＝HUDInfoMenu シングルトン**（ctor `FUN_005A4660`@0x5A4670 store・vtable 0xA6BDF4・**RTTI '.?AVHUDInfoMenu@@'**・dtor 窓 `FUN_005A4810` クリア・生成元 `FUN_00587D20`@0x587ECC）。**`[0xB3B350]/54/58`＝シングルトンではなく `hud_reticle.xml` インスタンス（0xA68D7C）から `FUN_005894F0`（タイル 0xFA8==index で子チェーン解決）が cache するインターフェース triple**（書手 `FUN_005A8000`・唯一の caller `FUN_0057D940`@0x57D9F5）。**末尾 DOOR 列全文**: cast TD 対は `0xB03248='TESBoundObject'→0xB05480='TESObjectDOOR'`・鍵必要時 `FUN_004D7780`（ExtraDataKey）→`FUN_004299E0` 閾値変換→**[0xB03E1C] グローバルキー名表**→[esi+0x4C] 経由 0xFDE=キー名・0xFAF=float(keyID)・0xFA1=2.0／それ以外の 1.0 経路。**訂正**: ArrowProjectile TD（0xB05AD0）cast は末尾列ではなく `FUN_005A8C90`@0x5A8D5D（dead 列）。final 終端列（0x5A4FA8〜）＝`ref->vt[0x170](0)`→`FUN_00446CB0` で [ref+0x44] 前方挿入→**[esi+0x2C] 経由 0xFDE**。
- **(AK) 更新（項目5）**: `vt[0x164]`＝**`FUN_004D8370`**（ACHR/ACRE 限定→`[[actor+0x58]（process）+0x17C]`）。返り値＝**vtable 無し 0xDC バイト plain struct**（new `push 0xDC`・ctor `FUN_00473EB0`・RTTI 不能＝**「BSAnim 帯」の正体は AnimSequenceBase/Single/Multiple＋BSAnimGroupSequence 系**（vtable RTTI 13 本実デコード・0xA3C6E4/72C/758 等）。[obj+0xCC]=アクティブ BSAnimGroupSequence{+0 state・+0x10 名 id・+0x28 arg}・**[obj+0xC4]=dynamic idle 失敗フラグ**。4 ヘルパー（4711F0/4706E0/472EA0/477E50）は非仮想メソッド（.rdata store 0 件で確定）。**[0xB3F9A8/AC/B0] ＝静的ゼロベクトル（NiPoint3::ZERO 相当・書手 0 件）**（隣接 .CRT stb で `[0xB3F9A4]=π・[0xB3F9A0]=2π・[0xB3F99C]=π/2` 実測・double 定数 4.0/0.5 使用）。**v256「寝姿勢時カメラオフセット推定」は棄却**（失敗経路は位置=ゼロリセット＋pitch=−π/2 再計算＝`FUN_004D0C20` 窓）。
- **(AL) 更新（項目6）**: ALCH flags bit0 スタブの帰属確定 — **先頭 0x4129C0（v256 の 0x4129C1 は 1B 誤差）・`AlchemyItem` 0xA3217C+0xE8 と `IngredientItem` 0xA33F5C+0xE8 の双方物**（RTTI 実デコード）。slot+0xE8 は TESForm 族共通仮想（BOOK/LIGH/MISC=no-op `FUN_0068F970`・WEAP=0x46C2F0・ARMO=0x69D990・STAT=0x4A8F90）。**引数反転セマントラック（arg 偽→bit0 SET／真→CLEAR）**・.text 全域に vt[0xE8] ディスパッチ窓 **8 件**（FUN_00425040×2=[ExtraData 0x1F/0x23] base form 宛・process/UI 帯 6 件）。
- **(AI) 更新（項目7）**: **BOOK DATA bit0=「Scroll」一次資料照合済み**（UESP: Flags 0x0001=Scroll/0x0002=Can't be taken・xEdit changelog・GECK Wiki checkbox "scroll"）。`[book+0x89]`（Teaches byte）の **0xFF=無効 sentinel も UESP 記述と一致確認**。pending 解除は BOOK 分のみ；**MGEF bit24 名は fetch 不能継続（transport error×4）＝未確認**（機構裁定=AV 宣言ビットは v256 §5.5 のまま確定維持）。

### 8.2 plan（Oblivion_Android 移植）への影響候補

- **formType レジストリ（項目1）**: 4CC→formType 変換と base ctor の one-shot 初期化（name→fourcc 合成・重複検査 0xA3BF38）はリーダー側の共通基盤として移植価値（现行 ESM リーダの formType 判定と突合のこと）。
- **GMST（項目2）**: 「GMST は form でなく GameSettingCollection マップへ EDID マージ」＝移植側が GMST を form として登録しているなら設計差（ドラフトのみ・本体訂正は承認後）。
- **dirty リスト（項目3）**: ESM マージ後の `vt[0x90](1)` に相当する「変更通知」は保存機構の要。移植側でリスト相当が無ければ保存対象追跡を代替で持つこと。
- **HUD/交差（項目4）**: 0xB3B350 系は「XML インスタンスからタイル 0xFA8 キーで解決したキャッシュ」— hud_reticle.xml 依存の記述に更新。
- **家具/アニメ（項目5）**: 「Anim 管理 obj＝plain struct 0xDC・+0x17C 格納・+0xC4 失敗フラグ・位置リセット=ゼロベクトル＋−π/2」に更新（v236/v237/v239/v241 port 資料の該当記述差替候補）。
- **ALCH ENIT flags（項目6）: vt[0xE8] 反転ブール**。GECK「Scroll」側と違い ALCH bit0 の意味名は未照合（§7.2 と同一 pending）。

## 9. 自己検証・判定まとめ・残点（次束候補）

- **selftest**: 着手時 **ALL PASS (4/4)**（§0 記載）→ 完成直前にも再実行 → 結果を末尾記載。
- **判定まとめ**: 1 **[確定]**（mod index 説棄却・+4=formType 両立＝冪等書込）/ 2 **[確定]**（GMST/GameSettingCollection・0xB35468 RTTI 実デコード・67 dispatch 表）/ 3 **[確定]**（vt[0x90]=dirty リスト登録・bit0 消費者=ガード自身のみ）/ 4 **[確定]**（HUDInfoMenu RTTI・hud_reticle キャッシュ・DOOR 列全文・v256 に訂正 1〔ArrowProjectile cast は末尾でなく 0x5A8D5D〕）/ 5 **[確定]**（無 vtable 0xDC struct・帯=AnimSequenceBase/BSAnimGroupSequence 系 RTTI・3 float=静的ゼロ＋π系定数・v256 のカメラオフセット推定を棄却）/ 6 **[確定]**（ALCH/INGR vt[0xE8]・RTTI 2 件・dispatch 8 窓）/ 7 **BOOK=照合済み［確定］／MGEF bit24=未確認（取得不能）**。
- **本タスク新規確定 VA/データ**: `FUN_0046AF70`（4CC→formType）・formType レジストリ 0xB05E04（69 項）・`FUN_0046C460` base ctor（レジストリ one-shot 初期化＋byte[+4]=0/+8=8）・jump 表計算 `byte[ft−1+0x44E6DC]`・**0xB35468=`GameSettingCollection`**（vtable 0xA4049C・`FUN_004A8040`）・`FUN_0046A9A0`（bit0 純セッタ・E8 2 件）・**変更リスト 0xB0614C**（vtable 0xA37AAC・`FUN_00449070` push 単一書手）・`FUN_0044FAA0/FAF0`（file+0x3DC bit0/bit3）・**[0xB3B33C]=HUDInfoMenu**（0xA6BDF4 COL 0xAC22A8）・`FUN_005A4660/4810`（ctor/dtor）・`FUN_005A8000`＋`FUN_005894F0`（タイル 0xFA8 解決）・0xA68D7C='Data\Menus\Main\hud_reticle.xml'・キー名表 0xB03E1C＋`FUN_004299E0`・`FUN_004D8370`（vt0x164）／`FUN_00473EB0`（0xDC ctor）／`FUN_004711F0/4706E0/472EA0/477E50`（非仮想）／Anim 帯 RTTI 13 本／[0xB3F9A4]=π・[0xB3F9A0]=2π・[0xB3F99C]=π/2・**0x4129C0=ALCH/INGR vt[0xE8]**（0xA32264/0xA34044）／`FUN_0068F970` no-op 並列。**RTTI 新規デコード 7 件**（GameSettingCollection/HUDInfoMenu/AlchemyItem/IngredientItem/AnimSequenceBase/Single/Multiple＋BSAnimGroupSequence 系）／**TD デコード 7 件**（TESObjectDOOR/TESBoundObject/ArrowProjectile/TESObjectREFR/Actor/TESNPC/TESCreature）。
- **証跡（すべて `%TEMP%\opencode\`）**: `v257_1/1b/1c_out.txt`（項目1）、`v257_2/2b/2c/2d/2e_out.txt`（項目2）、`v257_3/3b/3c/3d/3e/3f_out.txt`（項目3）、`v257_4/4b/4c/4d/4e/4f_out.txt`（項目4）、`v257_5/5b..5s_out.txt`（項目5・5s は disp32 バイト直撃で線形同期ズレ問題を回避）、`v257_6/6b/6c/6d_out.txt`（項目6）、`v257_9_out.txt`（最終検証）。Web: websearch 4 種＋fetch 5 試行（全失败明细は §7.2）。
- **遵守事項**: git 書出なし / docs・GT・TODO・plan・session-memory・cache・ソース無編集（cache は読取のみ）/ 既存 tmp/re 無上書き（本ファイル v257 のみ新規）/ ESM 本体再スキャンなし / VA→raw はセクション個別換算（v172_common・imm32_refs3/va2off 準守）・capstone 線形走査の同期ズレ 2 件（0x5A4F90・0x425180 窓）を関数先頭/バイト直撃復唱で補正し本文に反映 / PowerShell ヒアドキュメント不使用・`python -c` は正規表現なしの軽量確認のみ / Web 失敗は「確認できず」と明記（§7.2）・推測名は未確認情報としてのみ記録 / 中断対策（1 項目完了ごとに即追記）全 7 回実施。
- **新規残点（次束候補）**:
  1. （低）PlayerCharacter の vt[0x164]=`FUN_0065D670`（byte 引数形）と Actor 側 getter の署名衝突の実運用解明（PC の家具経路がどの vtable/slot を通るか）。低。
  2. （低）レジストリ 69 項の index 12..68 4CC 名の全列挙（formType 正表 26 超分の完全版）。低。
  3. （低）`FUN_004492E0` の mgr（esi+0xCD4）RTTI・変更リスト 0xB0614C を保存時に読む経路（E8/imm32 に見えない_consummer）。低。
  4. （低）HUDInfoMenu +0x2C/+0x4C インターフェース接続書手（XML 側 setter）／`0xB03E1C` キー名表全長。低。
  5. （低）0xDC anim struct の `[process+0x17C]` 初期 link Exact 書込点（swap/clear 実測済み・割当窓未展開）・`FUN_004706E0` 再索引表の意味。低。
  6. （低）vt[0xE8] ディスパッチ残り 6 窓の引数実体（disable 意味）と WEAP/ARMO/STAT 実装内容。低。
  7. （要 Web 再開）**MGEF flags bit24（全 32 ビット表）の GECK 正式名**（uesp fetch 通路回復時に一括照合）。低→保留。
- **最終 selftest（§8/§9 追記後）**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。
