# v272_opcode_gt_names16.md — opcode/GT 殘点 第16束（7件）

- 作成日: 2026-11-04 / 担当: サブエージェント（v268 §9 新規残点 1..7 の逐次解消）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ（v268 §9 残点・すべて低）:
  1. UIMenu sender 一点特定（0x438-0x43B 帯 2 引数窓の UI 固有切り分け）＋ jump table 形 8 Menu の id 表展開
  2. extra type **0x23** の ctor store 別 encoding 検索とクラス名（推定 ExtraPackages）
  3. ExtraLock データ +0xC..（キー連携）と ExtraTeleport データ +4 以降レイアウト
  4. [0xB33B00] obj（0x1C4B）の RTTI/vtable（ctor に直接 store 形なし＝サブ ctor 経由）
  5. `NiTMapBase<TESForm*>`（this=0xB0613C）の +4=0x0002008D の意味・データ配列割当窓
  6. [actor+0x3C] の actor 生成時書込 1 点収束・head 従属 obj([node+0xDC]) のクラス名
  7. GECK bit24 の代替通路（UESP talk 履歴等・web 可。不通なら負証拠維持・「確認できず」明記）
- 継承確定事項（v268・再検証しない）: FUN_00425040=TESPackage 族窓／[process+0x15C]=BSFaceGenNiNodeSkinned 従属 obj キャッシュ／UIMenu id 表=Menu 級 48 クラス +0x04 case 表（HUDInfoMenu 1..0xB→+0x28..+0x50）／0x31/0x32 [entry+0xC]=ptr・生成窓 FUN_00441970（TES new 0xCE0→FUN_00446D80／0x1C4 new→FUN_0045B300・self-register @0x45B3C8）／TOFT+8=NiTMapBase@0xB0613C（vtable 0xA3BC94）境界共有・[0xB06140]=0x0002008D
- 契約: 分析専任（docs/GT/TODO/plan/session-memory/cache・ソース編集禁止／既存 tmp/re 読取専用・上書き禁止／git 書出なし／ESM 再スキャンなし／loop 運用指示（STATE.md・L2・worktree・テスト）には従わない／本ファイルのみ新規成果物）
- 手法: capstone 5.0.7（v172_common.py 経由・VA→raw はセクション個別換算: .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00）＋ MSVC RTTI（v227_lib.py・vtable-4=COL・COL={0,off,0,TD+0xC,CHD+0x10}）＋ v268/v265 中間物再利用
- selftest: 着手時 `probe_harness.py selftest` → **ALL PASS (4/4)**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid 0x0015→impl 0x504560）
- 中間物: `%TEMP%\opencode\v272_*.py`（v268_*/v265_* 再利用）
- 注: 本ファイルは v268 §9 残点 1..7 の逐次解消記録（1 項目完了ごとに即追記）

## 1. 【低1】UIMenu sender 一点特定＋jump table 形 8 Menu の id 表展開 — **[確定・表]**（8 Menu すべて id 表回収＝実際は jump table でなく if 連鎖/索引演算形。TileMenu は非 id ルータと判明）＋ sender **[確定・負結果]**（小文字窓形の決定的走査で C++ 側一点 send 窓は**存在せず**・発火通路=GFx/AS データ駆動と機構特定）

### 1.1 8 Menu の +0x04 id 表（`v272_3a_out.txt`・実体 VA=各 vtable+0x04）
- v268_3g の「jump table 形（case 抽出不能）」8 クラスは実際には**すべて jump table を持たない**（`83 E8 xx` sub/if 連鎖または `lea reg,[eax-K]`＋索引演算でコンパイル済み）。展開結果:
  - **BookMenu** 0xA6B334+04=`0x595C00`: `0xB→+0x28 / 0xC→+0x2C`（ret 8・[esp+8] store）
  - **ClassMenu** 0xA6B49C+04=`0x5966B0`: `1→+0x28 / 2→+0x2C / 3→+0x30`（sub 連鎖）
  - **ControlsMenu** 0xA69FF4+04=`0x5879A0`: `lea edx,[eax-1]; cmp edx,0xC; ja skip; mov [ecx+eax*4+0x24],edx`＝**id 1..0xC→+0x28..+0x54**（索引演算形）
  - **CreditsMenu** 0xA6B784+04=`0x59C930`: `1→+0x28` のみ（`cmp [esp+4],1`）
  - **LevelUpMenu** 0xA6C264+04=`0x5AC580`: `2→+0x28` のみ
  - **TextEditMenu** 0xA6A04C+04=`0x587AE0`: `1..3→+0x28..+0x30`（`[ecx+eax*4+0x24]`）
  - **VideoMenu** 0xA6A09C+04=`0x587C40`: `1..0x30→+0x28..+0xE4`（`cmp edx,0x2F`）＝大領域 48 スロット
  - **TileMenu** 0xA6AF14+04=`0x590110`: **id ルータではない**（[確定]）。逐語=`mov [ecx+0x44],0; mov [esp+0xC],0; jmp 0x58DA00`（ret 0xC・3 引数）。FUN_0058DA00 は SEH 付き再初期化（`push 0xA2F7EC` は**SEH スコープテーブル**・文字列でない—raw=ptr 列`0x981E65/0x982897/0x98D90E…`＋'Access violation - no RTTI data!'@0xA2F80C と実測。`[esi+0x2C]=2` 書込・FUN_004028D0 文字列代入・FUN_0058D1C0 list 挿入）。E8 callers=FUN_00590580/590D20/591D40/591DC0（TileMenu 系初期化群）＝TileMenu（vtable 7 slot・Menu 級 15 slot と別譜）の +0x04 は「init/re-init(this, type, name?, &out)」契約。**v268_3h の「Tile=jump table 形」は分類誤り・id 表自体が存在せず**と訂正。
- 補強（v268_3g の if 連鎖 40 クラスは再掲不要・変更不要）。

### 1.2 sender 一点特定（負結果の確定・新機構証拠 3 点）
- 決定的走査 3 系統（すべて .text 全域・`v272_3b_out.txt`/`v272_3n_out.txt`）:
  1. `call dword [reg+4]`（FF 5x 04）・`mov r,[r+4]→call r` 窓のうち「call 直前 2 命令が push・片方 imm 1..0x64」= **72 窓**。UI/生成帯（0x56-0x5F）は 0 窓（0x56CD8E は `push 1`＋**this 別クラス**）。残は CRT/SEH（0x88x-0x8Cx）・Ni/BSX（0x43x-0x4Fx）帯。
  2. SIB*4 動的スロット `call [base+idx*4(+disp)]` 全形 = **14 窓のみ**。うち UI 関連 0・唯一の `call [ecx+eax*4+4]`=@**0x947ADD**（FUN_00947AC0・`ecx=[obj+0xC]` テーブル idx*0xC+4 叩き・E8 callers=FUN_00918AA0 1 件・0x8EC-0x94C 帯=Scaleform GFx SDK 譜）＝**AS/ExternalInterface 系 dispatch 機構**。
  3. メニュー別生成函（'Data\Menus\*.xml' 参照函 50 本・`v272_3j_out.txt`＝hud_info_menu.xml→FUN_005A4840 等・各 XML パスと host 一対一）を全函走査 → **どの生成函にも +4 dispatch 窓なし**（HUDInfoMenu 生成函 FUN_005A4840 は述語 FUN_005A46B0+失敗ログ `'HUD-Info Menu Creation Failed... Are your menu and art resources'`@0xA6BE30・子 movie 生成 FUN_005898F0→FUN_009832E6→FUN_00584880 の connect 形のみ）。
- 既知 v268 走査（E8 直接 0・FormA/B 757 窓・0x438-43B は (id,&out) getter 契約＝格納が**値**か**アドレス**かで別契約と再確認・`v272_3f` FUN_004383B0 逐語: `push &local; push id; call target->vt[4]`→**arg2 がアウトパラメータ**＝Menu の「arg2 を値で格納」と非互換）を合わせて、**retail .text に Menu+0x04(id,ptr) の静的 send 窓は存在しない**＝発火は GFx(Scaleform) 側 AS→ExternalInterface データ駆動通路（機構候補窓 FUN_00947AC0 群・0x94C 帯 SDK）に一本化される。**「一点特定」は機構レベルで達成**（C++ 関数一点ではない）。
- 付随新事実（[確定]）: メニュー**生成 id 系譜**=FUN_00587550（0x586000 の E8 内@0x5866DC）の `switch(menuTypeId 0x3E9+n)`（0x3EA/0x3EB/0x3EC=HUDMain?/0x3ED=HUDInfo→FUN_005A4840/0x3EF=FUN_005ADCF0/0x3F2=FUN_005A8E30…51 case・jump table=**FUN_00587D20 末尾 0x5887B8**（type→new+ctor 表・case 0x06=`new 0x5C→FUN_005A4660`=HUDInfo ctor ✓ v268 §3.2 と相互補強）。type→ctor 登録 registry=`call 0x42BB90` with ecx=**[0xB13970]**（FUN_00591B70@0x591BC9・NiTMap 型・子 movie の vt[0x34] 返り−0x3E9 で検索）＝**TileMenu::OnCreate の自動接続**（FUN_005900F0→FUN_00591B70）。

### 1.3 本項目内残点
- GFx SDK 帯（0x8EC-0x94C）→Menu オブジェクトまでの AS バインディング文字列名（RegisterFunction 名表）の一点回収（本束時間外・低）。
- menuTypeId 0x3E9.. の全 51 名前一覧化（jump table 0x5887B8 読取のみで可能・低）。

証跡: `v272_3a_out.txt`（8 Menu 展開逐語）、`v272_3b/3n/3o_out.txt`（決定的窓走査）、`v272_3c/3d/3e`（FUN_005A4840 生成函全文・述語 callers・'Data\Menus\Main\hud_info_menu.xml'）、`v272_3f`（0x438 帯反証・SEH テーブル 0xA2F7EC 実体）、`v272_3g/3h`（FUN_00587D20 jump table・FUN_00591B70 registry）、`v272_3j_out.txt`（xml 50 本→host 55 窓対応表）、`v272_3k/3l/3m`（thunk 群＝NiTList 追加系・xml parser 内 +4 窓 0）。

## 2. 【低2】extra type 0x23 の ctor 別 encoding とクラス名 — **[確定]**（クラス=**ExtraFollower**・推定 ExtraPackages は**誤り**。encoding=**ctor 内 direct imm store**（生成者 switch の push 形なし）。生成窓一点収束=`FUN_00424C50`）

### 2.1 store encoding の裁定（`v272_2_out.txt` 走査1/2・`v272_2l_out.txt` 実体）
- .text 全域で `[obj+4]=0x23` の書込は **2 窓のみ**:
  1. **@0x429B48** `mov byte ptr [esi+4], 0x23` in **`FUN_00429B20`**（extra 本体の ctor・**これが 0x23 の唯一の型書込**）。同 ctor @0x429B5D `mov [esi], 0xA357C4` ＝ vtable store 併設 → vtable-4 COL → RTTI 実デコード **`.?AVExtraFollower@@`**（`v272_2l_out.txt`）。
  2. @0x52734B `mov byte ptr [esi+4], 0x23` in `FUN_00527220` ＝ **TESNPC ctor**（[esi]=0xA53DD4 rtti=`.?AVTESNPC@@`）＝ TESForm+4 **formType=0x23(NPC)** の位相一致で **extra と無関係**（ExtraTeleport/TESSound 帯で既出の同型ノイズと同じ扱い）。
- 既知 switch 生成者形（`push imm; new; ctor` の型引数 push による creator）は 0x23 に**存在しない**: `C6 4x 04 23` direct・SIB・disp32・`mov reg,0x23→mov [reg+4],r8`(12B 窓)・`push 0x23; call GetExtra` 正規形（`v272_2h`=0 窓）すべて走査 → ctor 内埋め込みのみ。**＝型値はファイル側読取ではなく ctor 固定値**（v268 §1 の extra 型表 0x1F/0x20/0x22/0x31/0x32/0x39/0x3E/0x48 に 0x23 不在＝**ESM/SAVE ローダ生成対象外・ランタイム専用 extra** と整合）。

### 2.2 生成窓一点（`v272_2m_out.txt` 全文）
- `FUN_00429B20` の E8 callers = **`FUN_00424C50`@0x424C9A 1 件のみ**（`v272_2a/2m`）。`FUN_00424C50`（thiscall・ret 4・SEH）= **get-or-create**: `GetExtraData(this,0x23)`(FUN_0041E210) → 無ければ `new 0x10`→`FUN_00429B20`→`FUN_0041E0A0`(AddExtra) → 既存も新規も `[entry+0xC]` の**単方向リスト**（node: +0=key, +4=next・`cmp [eax],edx` 走査）を walk して末尾無ければ `push edx; call 0x446CB0`（key 登録窓・**[推定]** AddForm 系）→ `mov ecx,[0xB33B00]; call 0x45A500`（＝項目4 のシングルトン method）。arg(ret 4)=追加 key。
- callers（E8・`v272_2_out.txt` §4 併記）: `FUN_00426A90`@0x4275D9／`FUN_004282A0`@0x42860D／`FUN_00568BB0`×2（0x568D5F/0x568DFB）／`FUN_00643E30`@0x644026／`FUN_006748B0`@0x674A08 ＝ process/AI 帯（0x64x/0x67x）と TEPackage 隣接帯（0x426-0x428）＝**追従者(follower) key の AI 実行時登録**という語義と整合（**[推定]**）。
- 読手（`v272_2_out.txt` §4/`v272_2b`）: `FUN_00420EF0`（`push 0x23; call 0x41E210; ret`＝get only）／`FUN_00420F00`（get→あれば `push 0x23; call 0x41E160`＝**remove** の get-replace 形）／`FUN_00424D00`（get→`[[entry+0xC] …]` walk→`call 0x65C620`）／`FUN_00425040`@0x42521C・`FUN_00425650`@0x42586B/78・`FUN_004D06C0`@0x4D090A/19（消費側）。

### 2.3 ExtraFollower（0x23）データレイアウト（`v272_2l_out.txt` ctor 逐語）
- `+0x00` vtable **0xA357C4**／`+0x04` byte type **0x23**／`+0x08` dword **0**（count/フラグ・**[推定]**）／`+0x0C` = `new 8`{+0=0,+4=0} ＝ **リスト哨兵/ヘッド**（node+0=key, node+4=next）。obj size **0x10**。
- 一覧形（+0=key,+4=next のNiTList singly）は FUN_00424C50 の走査逐語で **[確定]**。

### 2.4 判定
- クラス名 **ExtraFollower [確定]**（RTTI 実デコード@vtable 0xA357C4・COL@0xA357C0）。**推定「ExtraPackages」は誤り**（ExtraPackage=型 0x1F・ctor `FUN_0042A1A0` とは別物・v268 §9 本文の対応も訂正）。RTTI 名表に `.?AVExtraPackages@@` 相当は**存在せず**（`v272_2_out.txt` §3 の 'Pack' 系全出現＝ExtraRunOncePacks/ExtraPackageStartLocation/ExtraTresPassPackage/ExtraPackage/ExtraProcessMiddleLow 等・0x23 と紐づく名は ExtraFollower のみ）。
- 本項目内残点: ①`0x446CB0` の正確な意味（AddForm 系 [推定] の確認）②key node の value 側（+0 以外）有無＝8B node は key のみ [推定] ③SaveLoad 側で 0x23 が間接永続化される経路の有無（ローダ switch 不在＝負証拠維持・低）。

証跡: `v272_2_out.txt`（走査1-5: direct/SIB/push 窓/ExtraPackage ctor 併座@0x426959）、`v272_2a_out.txt`（7 ctor の callers 一覧）、`v272_2b_out.txt`（FUN_00420EF0/F00/424C50/424D00 逐語）、`v272_2l_out.txt`（**FUN_00429B20 全文+RTTI 決定**・TESNPC 窓全文）、`v272_2m_out.txt`（FUN_00424C50 全文・callers）、`v272_2f/2k`（EDID 窓・BaseExtraList/ExtraDataList 名前列@0xA34DF0-0xA34F5C＝0x41E210/0x41E0A0/0x41E160 の帰属裏取）。

## 3. 【低3】ExtraLock データ +0xC..（キー連携）と ExtraTeleport データ +4.. レイアウト — **[確定]**（lock data=**0xC で終端＝+0xC 以降なし＝キー連携は ExtraLock データ外の負証拠が閉じた**。teleport data=**0x1C**＝{+0 ptr, +4..+C float3, +0x10..+0x18 float3}）

### 3.1 ExtraLock（0x31・entry vtbl 0xA357B8 `.?AVExtraLock@@`）
- entry 0x10（ctor `FUN_00429A80`・`v268_4b`）: `+0` vtbl／`+4`=0x31／`+8`=0／`+0xC`=**data ptr（ヒープ所有）**。dtor `FUN_0042AD70`（@0x42AD73 `push [esi+0xC]; call free`→self `[esi],0xA357B8`→`[esi],0xA34D50`→delete）＝**+0xC が data の唯一所有者**と確定（`v272_4e_out.txt`）。
- **data レイアウト 0xC バイト**（生成2窓でサイズ確定）:
  - `+0` u8 基準ロックレベル（`FUN_00429990` movsx 読・script 書込 `mov byte[eax],cl`@0x504A96）
  - `+4` u32 **非読取フィールド**（読者は比較 `FUN_00429AA0`@0x429AE8 と 0x10B ロード書込 `mov [esi+4],edx`@0x425B39 のみ。ゲームプレイ消費窓 0＝**[確定]（意味はファイル側不透過・time/unk 系 [推定]）**）
  - `+8` u8 flags: **bit2 = レベルドロック**（`FUN_00429990` test 4→Security×fLeveledLockMult→クランプ 99・v268 既載）／**bit0 = 施解錠状態ビット**（下項）／bit1/3-7 消費窓なし
  - **+0xC 以降のフィールド不存在 [確定]**: XLOC loader（`FUN_004259E0`@0x425A9A `new 0xC`）／script get-or-create（`FUN_004DBDF0`@0x4DBE00 `new 0xC`{0,0,0}）／dtor free(0xC 相当)／比較{+0,+4,+8}の 3 点照合。**⇒ キー連携は ExtraLock データに無い（v268 §4.3 残点の「+0xC 以降」仮説は否定。retail のキー解決経路=別通路のまま残点）**
- **bit0 機構（新事実 [確定]・名目 [推定「lock 状態/スクリプト設定」】**: loader 0xC レコード経路 `or byte[esi+8],1`@0x425B0D・script 窓（op impl `FUN_00504A10`@0x504A98）も常に `or byte[eax+8],1`／解除系 `FUN_00428E90`（`and byte[[entry+0xC]+8],0xFE`）の callers=**開錠帯 `FUN_004B66B0`×2/`FUN_004B8A40`×3/`FUN_004DBEA0`**／検査 `FUN_00428E70`（test bit0→al）callers=開錠/使用帯 10 窓（`v272_4d`）。
- tier 変換 `FUN_004299E0`（level→0..4・閾値 [0xB338B8..D8]＝`FUN_00429A30` の逆写像・`v272_4f` 逐語）。
- 読取系（lock data を返す getter `FUN_0041E690`＝GetExtra(0x31)→[entry+0xC]）callers=10 窓: `FUN_004D7740`（self→無ければ teleport 経由で接続先の lock を返す tail jmp 形）／`FUN_004D7780`（計算版・v268 既載）／`FUN_004D77D0`（再帰版 @0x4D7807 `jmp 0x4d77d0` 自己再帰）／`FUN_004D9070`（lock 有→self::vt[0x40](0x40)／teleport 接続先→接続先::vt[0x40]）／`FUN_004DBDF0`（生成）／`FUN_004DEBF0`／`FUN_004DF1A0`（接続 check: 両者の [[teleport]+0] 比較・`cmp edi,eax`@0x4DF38E）。

### 3.2 ExtraTeleport（0x32・entry vtbl 0xA357DC `.?AVExtraTeleport@@`）
- entry 0x10 は ExtraLock と同一形（ctor `FUN_00429C10`、+0xC=data ptr・dtor `FUN_0042ADD0` 経由 free）。
- **data 0x1C バイト**（XTEL loader `FUN_004259E0`@0x425E84 `new 0x1C`→ctor→`FUN_0042B670`(=XTEL 0x1C 直読・file→7dword 逐一写し)・サイズ関数 `FUN_0042B4F0`=`mov ax,0x1C`）:
  - `+0` = **接続先 TESObjectREFR ptr**（[確定]・v268 の FUN_0042B410 帰結と一致。保存時 formID 変換窓 `FUN_0042B500`@0x42B533: 0xC(pos)→0xC(rot)→4(formID via `FUN_0045BA00`) の書込順。解決 helpers: `FUN_0042B410`=get／`FUN_0042B420`=set／`FUN_0042B460`=`jmp FUN_006ECC80`（読込済み REF 解決）／`FUN_0042B470`=解決失敗時 `[ptr+0x18]` 埋込 obj の vt[0] フォールバック）
  - `+4..+0xC` = **float3（位置 [推定]）**／`+0x10..+0x18` = **float3（回転/アングル [推定]）**：ctor `FUN_0042B590` は両 triple を **0x7F7FFFCE（≈FLT_MAX・0xA35BE0 の float 定数）で初期化**＝未設定 sentinel。比較 `FUN_0042BA10` は [0]/+4 triple/+0x10 triple を 3 点比較（+4・+0x10 はそれぞれ `FUN_008AA390` の float3 比較）。副 accessors `FUN_0042B430`（=&+0x10）／`FUN_0042B440`（+0x10.. triple 書換）／`FUN_0042B4B0`（0x1C の 7dword 全コピー）。
- **+4 以降のレイアウト確定 [確定]**（float6 のうち位置/回転の名目のみ [推定]）。XTEL レコード=0x1C＝{u32 formID; float3; float3} と loader/save 両窓で対称確認。
- 消費帯: `FUN_004DB6B0`（接続先経由の追加比較・`FUN_0041E7B0`=**extra 0x27** getter を併用）／`FUN_004DEBF0`（接続先 formType check: `cmp cl,6`(ACTOR)/`cmp cl,0x23`(NPC→`lea ecx,[eax+0x24]; call FUN_00467560`)・**項目2 の formType=0x23=NPC と独立整合**）／`FUN_004DF1A0`（双方向接続 check＋`vt[0x44](0x100000)` フラグ書込窓・`FUN_0042B420(data,0)` による切断）／`FUN_0042B5F0`（接続先双方への状態伝搬 ret 8）。
- XLOC/XTEL 以外の loader case（`FUN_004259E0` switch 全体・4CC=`XCCM/XCHG/XSED/XGLB/XLOC/XMRC/XRGD/XMRK/XSLM?/XTEL/XESP/XOWN/XHRS/XSPN/XCLM/XMLS?/XNT?/XACT/XHLT/XTCM/XCNT/XWCS?/XSLM`）は本項目スコープ外。

### 3.3 本項目内残点
- キー item→施錠先の実連携経路（extra 0x27＝`FUN_0041E7B0` のクラス/レイアウト未査読。0x4DB6B0 帯が connection 先で 0x27 を二重取得する意味）。低。
- data+8 bit1/3-7・teleport data +0x18 の最終意味（float として保存されるが実行時 obj ptr 経路と両立する用途名の確定）。低。

証跡: `v272_4_out.txt`（FUN_004259E0 switch 全文・getters・FUN_00429990 callers 全窓）、`v272_4b_out.txt`（FUN_0042B590/B670 ctor/loader 逐語・getters callers 一覧・vtbl imm 参照 2+1 窓）、`v272_4c_out.txt`（0xA35BE0=0x7F7FFFCE 実測・FUN_004D7740/7D0/4D9070/4DBDF0 全文・消費窓）、`v272_4e_out.txt`（teleport accessor 群 0x42B410..0x42B550 生逐語・entry dtor/compare・script 窓 FUN_00504A10 全文）、`v272_4f.py` 実行結果（FUN_00428E90 callers=開錠帯・FUN_0042BA10/B2D0/B5F0/99E0 逐語・'Unrecognized format for lock data in file'@0xA351EC）。

## 4. 【低4】[0xB33B00] obj（0x1C4B）の RTTI/vtable — **[確定]**（クラス=**TESSaveLoadGame**・ただし **vtable/RTTI 自体が存在しない非ポリモーフィック cls**＝「RTTI 未回収」の答えは「回収対象なし」。名は assert 文字列群で確定）

### 4.1 非ポリモーフィック確定（`v272_5_out.txt`/`v272_5c_out.txt`）
- ctor `FUN_0045B300` 全走査（0x45B300..0x45B700 の E8 27 窓）: メイン obj `[esi]` への vtable imm store は**ゼロ**（@0x45B3FC `mov [esi],eax` は new ChangesMap ptr の格納）。v268 §5「ctor に直接 store 形なし」は**サブ ctor 経由ではなく「この obj に vtable が無い」**だった。
- 画像全体 scan: RTTI 名前文字列 `.?AV*SaveLoadGame@@` **0 件**（`.?AVSaveGameFile@@`@0xB058D4/`.?AVSaveMenu@@`@0xB1474C は別 cls）。＝ COL/vtable が無く RTTI が生成されない普通の thiscall クラス [確定]。

### 4.2 クラス名確定（assert 文字列→method 窓の対応・`v272_5c_out.txt`）
- .rdata 名実体（head VA 修正済）: `TESSaveLoadGame::SaveGameData() was called while loading.`@**0xA3A4E0**／`TESSaveLoadGame::SaveQueuedHavokData`@**0xA3A82C**／`TESSaveLoadGame::SaveQueuedAttachedAnimation`@**0xA3A854**／`TESSaveLoadGame::LoadForm`@**0xA3B06C**／`TESSaveLoadGame::UnloadForm`@**0xA3B0B8**（＋'Form %08X is unloading, but...'）。
- これら head の .text imm 参照窓 = **`FUN_0045B9A0`@0x45B9D3・`FUN_0045F7A0`@0x45F7D4・`FUN_0045F820`@0x45F8F1・`FUN_0045D220`@0x45D30D・`FUN_0045D390`@0x45D3F9・`FUN_004637C0`@0x46385F・`FUN_00463A90`@0x463B5D** ＝ 0xB33B00 シングルトンの method 帯（0x45B-0x463）。docs/Oblivion_decompiled.c も同窓を `"TESSaveLoadGame::LoadForm"` 付きで保持（L78315 等・read-only 照合のみで独立一致）。
- 補強: ctor が組むメンバー map の RTTI 群（`v272_5_out.txt` 実デコード）＝ TESSaveLoadGame 固有の指紋:
  - `FUN_0045A860` → **`.?AVChangesMap@@`**（vtable 0xA3A2EC、基 `NiTMapBase<…, ChangeData*>` 0xA38FBC）→ `[this+0]`
  - `FUN_0045A940` → **`.?AVInteriorCellNewReferencesMap@@`**（0xA3A310、基 map `NiTMapBase<…,BSSimpleList<uint>*>` 0xA38FDC）→ `[this+4/+8]`
  - `FUN_0045AA80` → **`.?AVExteriorCellNewReferencesMap@@`**（0xA3A330、基 map `NiTMapBase<…,BSSimpleList<ExteriorCellReferenceData*>*>` 0xA38FFC）×2 → `[this+0xC]/[+0x10]`
  - `FUN_0045ABE0` → **`.?AVNumericIDBufferMap@@`**（0xA3A350、基 `NiTMapBase<…,void*>` 0xA3901C）×4 → `[this+0x54..0x60]`
  - +`[this+0x74]/[+0x78]` = `NiTLargeArray<UInt32>` ×2（vtable 0xA39044 埋込形・初期 cap 100/5・0x18B obj）

### 4.3 メンバ/状態フィールド（ ctor ゼロ埋め列＋docs 消費窓からの部分-map・**[確定]（位置）/**[推定]（名目））
- +0x14: 数値バッファ ptr（docs `*(undefined2**)(DAT+0x14)` 形・TESSaveLoadGame 側の formID リマップ配列 [推定]）／**+0x18 dword flags**（`FUN_0045A500`＝`[0xB33398]+0x10`(ロード mod 数 [推定]) と IAT `[0xA2808C]` call を比較し一致時 bit0・不一致時 bit18(0x40000) を返す述語・`v272_5b` 逐語）／**+0x7C byte**（mod index 上限制限 0x23/0x3F と docs 比較窓）。
- method 帯 `FUN_004620CF/0x46210D/0x462148/0x4646AC/0x465B33`（Interior/Exterior/Changes map を再生成する reload 経路＝save/load リロード時のマップ再建 [推定]）。

### 4.4 本項目内残点
- 完全 member map（+0x1C..+0x50/+0x80..+0x1C0 の個別意味）と `FUN_0045A500` の正確な義名（'ModLoaded?' 系 [推定]）、IAT `[0xA2808C]` の import 名取得。低。

証跡: `v272_5_out.txt`（ctor 先頭 0x120 逐語・E8 27 窓・sub-ctor vtbl/RTTI 14 件実デコード）、`v272_5b_out.txt`（ctor 残り 0x300・FUN_0045A500 逐語・0xB33B00 imm 参照全集）、`v272_5c_out.txt`（assert 5 本の head VA と method 窓 7・sub-ctor callers・NiTLargeArray vtbl 0xA39044・ctor 末尾 0x220 逐語）。

## 5. 【低5】NiTMapBase<TESForm*>@0xB0613C の +4=0x0002008D の意味・データ配列割当窓 — **[確定]**（+4=**バケタブ数（hash 法）131,213**・割当窓=**CRT 静的初期化子 `FUN_009DBF00`**（@0x9DBF2C store・new 0x2008D×4=524,852B＋memset）・初期 vtable 0xA3BC94→実行時 **`NiTPointerMap<UInt32,TESForm*>`** 0xA3BDD8 へ差替）

### 5.1 +4=0x0002008D の裁定（`v272_6_out.txt`・`v272_6c/d`）
- vtbl slot1=`FUN_006A9060`（逐語: `mov eax,[esp+4]; xor edx,edx; div dword ptr [ecx+4]; mov eax,edx`）＝**hash(key)=key mod [map+4]** → +4 は除数＝**バケタブ数**。v268 の「2 word 0x8D/0x02（バケタブ数/生成 flag 候補）」仮説は**棄却**：dword 全体 **0x0002008D=131,213** を除数として使用（高 word 0x0002 は語義なし・上位ビット）。
- 消費列（すべて *4 索引）: Get=`FUN_0055E000`@0x55E011 `mov ecx,[esi+8]; mov edi,[ecx+eax*4]`（entry={+0 next, +4 key, +8 value}・keyeq=slot2 `FUN_00763E80`、value 返却 [edi+8]）／Insert=`FUN_00452570`（slot4 entry alloc→slot3 構築→バケット頭差込→**`add [esi+0xC],1`＝count=[map+0xC]=[0xB06148]**）／Remove=`FUN_004524C0`（`sub [esi+0xC],1`）／ID 割当衝突ループ=`FUN_00448D40`@0x448D8B-A0（`mov eax,[edx+4]` slot1 呼び出し→`[0xB06144]+eax*4` walk）。
- ⇒ map オブジェクトレイアウト確定 [確定]: `+0 vtbl / +4 buckets / +8 buckets 配列 ptr=[0xB06144] / +0xC 入数=[0xB06148]`。

### 5.2 割当窓 = CRT 静的初期化子（`v272_6e_out.txt`・**0x9DBF00 の imm 参照=.rdata 0xA28400（.CRT イニシャライザ表）**）
- `FUN_009DBF00` 逐語: `mov eax,0x2008D; mul 4`（=**524,852B**）→ `call 0x401F00`（operator new）→ **@0x9DBF2C `mov [0xB06144],eax`** → `mov ecx,[0xB06140]; lea edx,[ecx*4]; push edx; push 0; push eax; call 0x981630`（memset）→ **@0x9DBF3B `mov [0xB0613C],0xA3BDD8`**（vtable 差替）→ `push 0xA18520; call 0x981FB4`（atexit）。
- **vtable 0xA3BDD8 = RTTI 実デコード `.?AV?$NiTPointerMap@IPAVTESForm@@@@`（＝`NiTPointerMap<UInt32,TESForm*>`）**。画像上の 0xA3BC94（v268 で確定した NiTMapBase<…,TESForm*> の vtable）は**静的初期化前の基底状態の vtbl**（コンストラクタ連鎖の基底 store が .data に残り、CRT init で導出 vtbl に切替わる MSVC 標準形）。切替後 slot 群: +0=`FUN_0046C220`（導出 dtor）/+4=`FUN_006A9060`（hash 同一）/+8=`FUN_00763E80`/+0xC=`FUN_0067F130`/+0x10=`FUN_0068F970`/+0x14=`FUN_004F0F60`（entry alloc 実体化）。
- atexit クリーン `FUN_00A18520`=@0xA18520 `mov ecx,0xB0613C; jmp 0x46C0F0`（map dtor 窓）＝free([0xB06144]) 側。**v268 §5 の「CRT 初期化子からの書込なし」判定は [0xB33A98]/[0xB33B00] の話で本 map と無矛盾**（本 map は CRT init 由来）。
- 隣接静的（同 init 帯）: `FUN_009DBF50`: new 0x20×4 → **`[0xB06150]`**＝ NiTLargeArray@0xB0614C（changed/dirty form 配列・vtable 0xA37AAC・count=[0xB06158]/別カウンタ[0xB0615C]・consumers `FUN_0046B2C0`/`FUN_004492F9` 群）／`FUN_009DBF80`: 0xFA1×4 buckets の別 map=[0xB06164..0xB0616C]（vtable 0xA3BF1C・本項目外）。

### 5.3 本項目内残点
- 導出 vtbl の追加スロット（+0x14 `FUN_004F0F60` の entry new サイズ等）と dirty 配列 [0xB0614C] の拡張（32→）窓。低。
- NiTPointerMap の value dtor 有無（owned フラグ＝+4 高位語義の代替候補は上記で棄却済み・entry 破棄は slot+0x10/+0x18=`FUN_0068F970`(noop ret 4)＝**TESForm* は非所有 [確定]**）。

証跡: `v272_6_out.txt`（0xA3BC94 slots 12・画像値 0xB0613C..50・FUN_0046B030/46B250/6A9060/763E80/67F130/68F970/55E000 逐語）、`v272_6b_out.txt`（`mov ecx,0xB0613C` 全 71 窓・0x801B4 imm 0 件・[0xB06144] 参照 16 窓一覧）、`v272_6c_out.txt`（FUN_00448D40 全文＝ID 割当＋map 参照・FUN_0046B2C0/46C300=SetFormID 経路 Remove/Set・FUN_0046AF70 の registry 終端=0xB06144 再確認）、`v272_6d_out.txt`（Remove/Set の entry 形と count 加算）、`v272_6e_out.txt`（**FUN_009DBF00 割当窓逐語**・FUN_004471A0 callers）、`v272_6f.py` 実行結果（0xA3BDD8=NiTPointerMap RTTI・0x9DBF00→.rdata@0xA28400 参照・atexit fn 逐語）。

## 6. 【低6】[actor+0x3C] の actor 生成時書込 1 点収束・head 従属 obj([node+0xDC]) のクラス名 — **[確定]**（書込点一点=`FUN_004D7D10`@**0x4D7D56**（NiPointer 参照カウンタ形・REFR ctor はゼロのみ）／head 従属 obj=**BSFaceGenAnimationData**（vtable 0xA6424C・0x1E0B））

### 6.1 [actor/REFR+0x3C] 書込の一点収束（`v272_7c/7f_out.txt`）
- **field 帰属**: vt[0x154] の getter `FUN_00422DE0`（`mov eax,[ecx+0x3c]; ret`）は **TESObjectREFR vtable 0xA46C44 の +0x154 スロットそのもの**（vtable 走査で Actor 0xA6E074/ArrowProjectile/Creature/MobileObject/Magic*Projectile 群が同関数を継承＝[actor+0x3C] は REFR 級フィールド＝**3D ルート obj ptr**）。
- REFR ctor `FUN_004D9A70` は @0x4D9AB9 `mov [esi+0x3c],0` のみ（初期化）。**非ゼロ書込の唯一窓 = `FUN_004D7D10`@0x4D7D56** `mov [edi+0x3c],ebx`（旧値 `[+4]` を InterlockedDecrement `[0xA2807C]`→0 なら vt[0] delete・新値は InterlockedIncrement `[0xA28078]`＝NiPointer 代入の定形・冒頭に `this->vt[0x144]` ゲート＋`call 0x977C50(3/2)`）。＝「一点収束」**[確定]**。
- 生成経路（callers 11 窓・`v272_7f`）: **`FUN_004E0F80`@0x4E13DB／`FUN_004E4080`@0x4E4117**（REFR ロード/生成帯＝actor 生成時経路・**[推定]**（帯帰属））＋ form 側 0x437BB0/0x43AE10/0x43FBA0/0x441EF0＋projectile 帯 5 窓（0x696-0x69F）。他クラスの disp8 [+0x3C] 書込は多数存在するが本 REFR フィールドの書込窓は Set3D のみ（A2 走査＝capstone 形別全集で網羅）。
- 付随: PC  override `FUN_006600F0` は [PC+0x5D0]（別フィールド・first-person override [推定]）、Character override `FUN_00612220` は obj+0x18 bit0 でゼロ化（E5 の flag フィルタ＝**AnimationData(+0x18) の状態ビット**と判明＝6.2 と連結）。

### 6.2 head 従属 obj のクラス名（`v272_7d/7e/7g_out.txt`）
- BSFaceGenNiNode の C++ vtable=**0xA64F5C**（slot+0x9C=`FUN_0055CEF0`=[node+0xDC] getter・アンカー検索で確定）。RTTI=**`.?AVBSFaceGenNiNode@@`**。**v268 E5 の「BSFaceGenNiNodeSkinned クラス」は C++ クラス名ではなく Ni ノード名**と訂正：文字列 0xA46ED8='**BSFaceGenNiNodeSkinned**'／0xA46EC0='**BSFaceGenNiNodeBiped**'（両者同一 C++ cls BSFaceGenNiNode・0x118B・ctor `FUN_0055CD70`）。
- 生成窓 **`FUN_0052DED0`**（callers=`FUN_00523220`@0x5232AE／`FUN_00525A70`@0x525F40＝NPC/actor 生成帯 [推定]）逐語: `new 0x1E0→FUN_0054CAF0`（ AnimationData）→ `new 0x118→FUN_0055CD70`→`FUN_006FF420('BSFaceGenNiNodeBiped')`→`node->vt[0xA0](animData)`→`vt[0xB0](1)`/`vt[0xB8](1)` → 2 番目 `new 0x118→FUN_0055CD70`→`FUN_006FF420('BSFaceGenNiNodeSkinned')`→`vt[0xA0](animData)`→`vt[0xB0](0)`/`vt[0xB8](0)`。
- **vt[0xA0]=`FUN_0055CF00`** が [node+0xDC] の refcount 書込 setter（@0x55CF35 唯一の非ゼロ +0xDC store 経路・ctor の +0xDC store はゼロ 3 点のみ＝`v272_7e` 走査と一致）。
- ⇒ **[node+0xDC] の obj 型 = vtable 0xA6424C = RTTI `.?AVBSFaceGenAnimationData@@`**（0x1E0B・ctor `FUN_0054CAF0`：サブリスト群= `.?AV?$NiTPointerList@PAVBSFaceGenKeyframe@@@@` 0xA6423C を 9 組＋float/byte フィルダ群。+0x18 は状態 dword＝6.1 の Character ゲート bit0 と同一物）。**[確定]**
- E5 のキャッシュ連鎖は「**NiPointer<BSFaceGenAnimationData>** を head ノード（名前検索 BSFaceGenNiNodeSkinned）から取ってくる」構造として完結。

### 6.3 本項目内残点
- `FUN_004D7D10` の正確な公開名（Set3D/Change3D）と callers 各窓の actor/form 帰属の個別確定（生成時経路は [推定] 維持）。低。
- byte[actor+0x1DB/0x1D5/0x1DA] 名目（v268 §2.3 分類 A=+0x1DB 書込群6 は facegen 帯 0x5BE-0x5C0 の AnimationData 状態書込と整合するが個別未追跡）。低。
- `FUN_006FF420` のノード名設定 API 名（NiAVObject::SetName 候補）。低。

証跡: `v272_7_out.txt`（getter 3 実装全文・BSFaceGen* RTTI 名表 16 本・FUN_004DDFD0 全文＝+0x3C からの名検索確認）、`v272_7c_out.txt`（[reg+0x3C] mod=01 書込全集・0x55CEF0@+0x9C アンカーで vtbl 0xA64F5C 確定＋store-ref 2 窓）、`v272_7d_out.txt`（BSFaceGenNiNode ctor 2 窓全文・+0xDC ゼロ・refcount パターン・0x422DE0@+0x154 vtbl 一覧＝REFR 帰属確定）、`v272_7e_out.txt`（disp32[+0xDC] 書込全集・FUN_0052DED0 全文）、`v272_7f/7g`（vt[0xA0]=FUN_0055CF00 逐語・0xA6424C/.?AVBSFaceGenAnimationData@@・0xA46EC0='BSFaceGenNiNodeBiped'・Set3D callers 11 窓）。

## 7. 【低7】GECK bit24 の代替通路（uesp 503 続行なら負証拠維持・「確認できず」明記） — **[未確定]**（代替通路も不通・**確認できず**／EXE 側負証拠を新窓で補強）

### 7.1 今束の試行記録（2026-11-04 実施）
- `webfetch en.uesp.net/wiki/Oblivion_Mod:Mod_File_Format/MGEF` → **503**（v256/257/265/268 に続き不通継続＝代替通路1 失敗）。
- `webfetch web.archive.org/web/2024/…同 URL`（Wayback 経由＝UESP 履歴の代替通路）→ **503**（v268 §9「talk/履歴側」候補を実行して不通確認）。
- `websearch` 2 発（bit24 0x01000000 MGEF GECK / tes4edit xEdit flags 表）→ 決定的一次資料なし（ gameplay 相談ノイズのみ・傍証も v268 §7.2 の2件から増えず）。
- ローカル代替: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\` に **TESCS.exe 不在**（Oblivion.exe/OblivionLauncher.exe のみ）＝実解析経路も不成립。

### 7.2 追加静的負証拠（`v272_8.py` 実行）
- .text 全域で bit24=0x01000000 を **OR 書込**する即値窓は 4 件だけで、host は `FUN_007F27E0`（Havok 帯）/`FUN_008A83C0`/`FUN_00954860`（CRT 帯）/`__hw_cw`（MSVC フレーム）＝**magic/GECK 関連ゼロ**＝retail 実行時にも bit24 を立てるコードは存在しない（データ由来ビットであることの裏取）。
- 既知消費窓 @0x4131B7 の逐語を再取得（`v272_8` C 節）: `edi=[[MGEF]+0x58]`（＝**flags dword は MGEF form+0x58 [確定]**）→ `test edi,0x1180000` → 単独 bit24 検査（`shr 0x18`）→ 立てば `[MGEF+0x60]` を EffectItem+0x14 へ copy。bit20/19/18 のみ検査分岐は subsequent 比較なし（=bit24 成立時にのみ +0x60 copy が発火）。v265 §7.3 の機構記述と完全一致＋flags@+0x58 が新確定。

### 7.3 裁定
- 「GECK(TESCS.exe) が 'Use actor value' チェック時に bit20 と **bit24** を連動設定する」仮説: **確認できず**（一次資料 web 不通・talk/履歴/Wayback 代替も不通・TESCS.exe 不在・実機 UI 確認不可）。**負証拠維持で [未確定] 継続**（v268 §7.3 と同一裁定＋新静的負証拠）。移植注記は v268 E7 のまま（bit20 公式名＋EXE 要求 bit24 の二重条件を尊重・bit24=0x01000000 はファイルデータ側のみで立つ）。
- 次束候補: uesp 再開時に MGEF 表＋'Magic Effect Flags' GECK 節の再取得／TES4Edit の .pas 定義（`tes4Edit.pas` MGEF flag 列挙）の静的取得（GitHub raw 経由）— 時間外。

## 8. GT/plan ドラフト（反映は要承認・本体無編集）

- **(D1・項目2)**: extra 型表に **0x23 = ExtraFollower**（vtable 0xA357C4・RTTI 実デコード・生成唯一窓 `FUN_00424C50` get-or-create・ローダ switch 不在＝**ランタイム専用**）を登记。既存ドラフトの「0x23=ExtraPackages 推定」は**全滅取り消し**（`.?AVExtraPackages@@` は画像に存在せず）。
- **(D2・項目3)**: ExtraLock データ = **0xC バイト**{+0 u8 level／+4 u32 不透過（ゲームプレイ読手 0）／+8 u8 flags: bit0=施解錠状態（loader 0xC 経路 & script 窓が or 1・開錠帯が 0x428E90 で and ~1・検査 0x428E70）／bit2=leveled}。**キー連携は ExtraLock データ外 [確定負]**（+0xC 以降なし）。GT の E3「キーは別経路」を確定化・別経路候補=extra **0x27**（`FUN_0041E7B0`・0x4DB6B0 帯）を次束。
- **(D3・項目3)**: ExtraTeleport データ = **0x1C**{+0 接続先 TESObjectREFR*（保存 formID 変換窓 `FUN_0042B500`）／+4..+C float3 位置／+0x10..+0x18 float3 回転 [推定]・初期値 0x7F7FFFCE(≈FLT_MAX) sentinel@0xA35BE0}。XTEL レコード=0x1C（loader `FUN_0042B670` 直読）。移植 ESM リーダ側は formID→ptr 変換を 0xB0613C map 経由で実施済であるべき（v268 E4 追記と整合）。
- **(D4・項目4)**: **[0xB33B00] = TESSaveLoadGame**（非ポリモーフィック・RTTI 無し＝回収対象なし）。assert 名 5 本（`TESSaveLoadGame::SaveGameData/SaveQueuedHavokData/SaveQueuedAttachedAnimation/LoadForm/UnloadForm`）＋ member 指紋（ChangesMap/InteriorCellNewReferencesMap/ExteriorCellNewReferencesMap/NumericIDBufferMap×4/NiTLargeArray×2）。v265 §3.4「TESSaveLoadGame 相当候補」→ **確定**に更新。
- **(D5・項目5)**: 0xB0613C map の詳細解像: クラス=**`NiTPointerMap<UInt32, TESForm*>`**（CRT 静的初期化 `FUN_009DBF00` で vtable 0xA3BC94→**0xA3BDD8** に切替・画像値は基底状態）、**[map+4]=0x0002008D=131,213 はバケタブ数**（hash=`key mod [map+4]`・flag 類ではない）、**[map+8]=[0xB06144]=buckets 配列**（同 init で `new 0x2008D*4`＋memset・atexit で free）、**[map+0xC]=[0xB06148]=入数**、entry={+0 next,+4 key,+8 value}・value 非所有（slot+0x10 noop）。GT の「TOFT 境界共有」節は v268 のまま有効＋本解像を追記。
- **(D6・項目6)**: GT の 'BSFaceGenNiNodeSkinned' 記述系譜を**訂正**：それは**ノード名**（0xA46ED8）で C++ クラスは **BSFaceGenNiNode**（vtable 0xA64F5C・0x118B）。head 従属 obj（[node+0xDC]・vt[0x9C]）の型=**BSFaceGenAnimationData**（vtable 0xA6424C・0x1E0B・`NiTPointerList<BSFaceGenKeyframe*>`×9）。[actor/REFR+0x3C]=3D ルート ptr の**唯一書込窓=`FUN_004D7D10`@0x4D7D56**（NiPointer refcount 代入・REFR ctor はゼロ初期化のみ）。生成時到達経路=callers `FUN_004E0F80/004E4080` 帯 [推定]。
- **(D7・項目1)**: UIMenu「+0x04 id 表」は 48 Menu 級で全網羅（if 連鎖 40＋索引/算術演算形 8＝Book/Class/Controls/Credits/LevelUp/TextEdit/Video/Tile）。うち **TileMenu+0x04 は id ルータでない**（init/re-init 契約・jump table 形 분류は誤り）。**C++ 側の一点 sender send 窓は retail .text に存在せず**（決定的走査 3 系統で 0 窓）＝発火通路=GFx AS→ExternalInterface データ駆動（機構候補 `FUN_00947AC0`）。GT の「sender 一点特定」残点は**機構解として解消**扱いに更新可。

## 9. 自己検証・判定まとめ

- **判定**: 1 **[確定・表]**（8 Menu id 表＋sender 機構解・一点は負確定）／2 **[確定]**（ExtraFollower・ctor 埋込 encoding・推定 ExtraPackages 訂正）／3 **[確定]**（lock data 0xC 終端＝キー連携負確定・teleport data 0x1C float6・bit0 施解錠機構新事実）／4 **[確定]**（TESSaveLoadGame・RTTI 不在＝回収対象なし）／5 **[確定]**（+4=バケタブ数・割当窓=CRT init `FUN_009DBF00`・導出 vtbl 0xA3BDD8 判明）／6 **[確定]**（Set3D 一点収束・BSFaceGenAnimationData）／7 **[未確定]**（代替通路（uesp 直接/Wayback/search 2 発）不通継続・**確認できず**明記・EXE 側静的負証拠を追加）。
- **新規確定 VA/データ**: `FUN_00429B20`（ExtraFollower ctor・型書込@0x429B48・vtbl store@0x429B5D）／vtable 0xA357C4・0xA357DC（dtor 0x42AD70/0x42ADD0）／lock data 述語 `FUN_00428E70`・解除 `FUN_00428E90`・get-or-create `FUN_004DBDF0`／XLOC 窓 @0x425A89・XTEL 窓 @0x425E77（`FUN_004259E0` switch）／teleport accessor 群 `FUN_0042B410..0x42B4F0`・save 窓 `FUN_0042B500`／assert 文字列 head 0xA3A4E0/0xA3A82C/0xA3A854/0xA3B06C/0xA3B0B8・method 窓 7／`FUN_009DBF00`（buckets new@0x9DBF16・store@0x9DBF2C・memset@0x9DBF31・vtbl 差替@0x9DBF3B・atexit 0xA18520）／vtable 0xA3BDD8=NiTPointerMap RTTI／BSFaceGenNiNode vtbl 0xA64F5C・AnimationData vtbl 0xA6424C・生成窓 `FUN_0052DED0`・setter `FUN_0055CF00`@0x55CF35／Set3D `FUN_004D7D10`@0x4D7D56。**RTTI 新規デコード 12**（ExtraFollower/BSFaceGenNiNode/BSFaceGenAnimationData/NiTPointerMap/Changes 系 map 5/NiTLargeArray/NiTPointerList）。
- **自己整合検査**: 項目2 の `mov byte[obj+4],0x23` 2 窓（extra=ExtraFollower／form=TESNPC formType）が項目3 teleport 消費窓の `cmp cl,0x23`(NPC 接続先 check) と位相互補。項目3 の lock data 0xC 終端は項目4 の TESSaveLoadGame dirty 配列（0xB06150）と矛盾なく独立。項目5 の map entry{next,key,value}＋非所有（noop dtor）は項目4 の NumericIDBufferMap（同親方 NiTMapBase・value void*）と形一致。項目6 の vt[0xA0] setter が項目2 既知の refcount IAT窓（[0xA28078]/[0xA2807C]）同一＝NiPointer 機構の横断確認。
- **遵守**: git 書出なし / docs・GT・TODO・plan・session-memory・cache・ソース無編集（docs/Oblivion_decompiled.c は read-only grep のみ・項目4 裏取）/ 既存 tmp/re 無上書き（v272 のみ・作成済 §1 は非改変）/ ESM 再スキャンなし / loop 運用指示（STATE.md・L2・worktree・テスト）非追従（分析専任契約）/ VA→raw セクション個別換算順守 / ヒアドキュメント不使用・スクリプトファイル化（v272_4..8.py 新規14 本・v268_*/v265_* 中間物再利用）/ 中断対策（1 項目完了ごとに即追記）**6 回実施**（§2/§3/§4/§5/§6/§7-9）。
- **自己テスト（着手時）**: `python probe_harness.py selftest` → **ALL PASS (4/4)**（bytes@0x504050・bytes@0x4FB580・string@0xA7035C・opid 0x0015→impl 0x504560）。
- **新規残点（次束候補・すべて低）**:
  1. GFx SDK 帯→Menu オブジェクトの AS バインディング名表（RegisterFunction）一点回収（項目1 続き）。menuTypeId 0x3E9.. 51 名前一覧化（jump table 0x5887B8 読取のみ）。
  2. extra 0x27（`FUN_0041E7B0`）の RTTI/レイアウト＝キー連携の実経路候補（項目3 続き）。
  3. TESSaveLoadGame の完全 member map（+0x1C..+0x50/+0x80..）と `FUN_0045A500`/'IAT[0xA2808C]' の意味（項目4 続き）。
  4. map Set 窓 `FUN_00452570` の entry alloc サイズ（slot+0x14 `FUN_004F0F60`）＋dirty 配列拡張窓（項目5 続き）。
  5. Set3D 呼び手 11 窓の個別帰属（生成時経路 [推定] の確定）＋byte[+0x1DB/0x1D5/0x1DA] 名目（項目6 続き）。
  6. bit24: uesp 再開待ち or tes4Edit.pas の flag 列挙を別ドメイン（GitHub raw）から静的取得（項目7 続き・現状**確認できず**）。
