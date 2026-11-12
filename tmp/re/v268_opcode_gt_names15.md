# v268_opcode_gt_names15.md — opcode/GT 殘点 第15束（7件）

- 作成日: 2026-11-03 / 担当: サブエージェント（v265 §9 新規残点1..7 の逐次解消）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ（v265 §9 残点・すべて低）:
  1. `FUN_00425040` 2 窓の no-arg call 形と MagicItem slot+0xE8 `ret 4` 契約の不整合 — レシーバ実クラス（[esi+0xC] の型）特定と v257 裁定「ALCH 経路」の再検証
  2. process 族 slot58（actor->vt[0x154] が [process+0x15C] をキャッシュ）の語義と callers 32 窓（0x5BE-0x5C 帯）分類
  3. UIMenu message ルータ側 vt[0x04] id 定義表（HUDInfoMenu id 2/0xA の発火名目）
  4. `FUN_004D7780` 経路 [entry+0xC]（0x31/0x32 extra payload）の formID/ptr 別と `FUN_00429990` レシーバ form 級
  5. `[0xB33A98]`（TES*）・`[0xB33B00]` の生成書込点（CRT 初期化子以外）
  6. formType レジストリ TOFT 項 +8=0xA3BA94 異常値の意味
  7. GECK 側 bit24 の設定挙動（TESCS.exe「Use actor value」との連動）— web 検索可、失敗時「確認できず」明記
- 継承確定事項（v265）: MagicItem 族 slot+0xE8 の実消費者=FUN_00425040×2 のみ／ALCH/INGR 実装 `0x4129C0`（ENIT flags bit0 反転・ret 4）／基底 MagicItemObject+0xE8=0x982D85 purecall／HUDInfoMenu vtable+0x04=`FUN_005A4700`（id 1..0xB→+0x28..+0x50）／`FUN_004D7780` 末尾 tail jmp `0x429990`（extra 0x31→0x32 [entry+0xC] 経由）／mgr=`.?AVTES@@` this=[0xB33A98]・0xB33A98 への非ゼロ store は .text なし／formType 69 項（TOFT idx 0x44・+8=0xA3BA94 異常）／MGEF bit24 は UESP/GECK 公開表に定義なし・EXE @0x4131B7 `test edi,0x1180000`→bit24 単独検査
- 契約: 分析専任（docs/GT/TODO/plan/session-memory/cache・ソース編集禁止／既存 tmp/re 読取専用・上書き禁止／git 書出なし／ESM 再スキャンなし／loop 運用指示（STATE.md・L2・worktree・テスト）には従わない）
- 手法: capstone 5.0.7（v172_common.py 経由・VA→raw はセクション個別換算）＋ MSVC RTTI（v227_lib.py）＋ 既存 v265 中間物再利用（`%TEMP%\opencode\v265_*_out.txt`）
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid 0x0015→impl 0x504560）
- 中間物: `%TEMP%\opencode\v268_*.py`（v265/v257 系再利用優先）
- 注: 本ファイルは v265 §9 残点 1..7 の逐次解消記録（1 項目完了ごとに即追記）

## 1. 【低1】FUN_00425040 2 窓の no-arg call と MagicItem slot+0xE8 `ret 4` 契約不整合 — **[確定]**（レシーバ実クラス=**TESPackage 族**。v257「ALCH 経路」裁定・v265 §6「実消費者=本 2 窓のみ」とも**撤回**）

### 1.1 extra type 写像の確定（窓が触る entry の実体）

- extra entry レイアウト = `{+0 vtable, +4 type byte, +8 next, +0xC payload}`（`v268_1f_out.txt`: ExtraPoison ctor `0x42A700` は type 0x48 store・payload=[+0xC]、ExtraOwnership `0x429E00` 同形。検索関数 `FUN_0041E210` の直後ログ文字列 = **`'BaseExtraList::GetExtraData'`@0xA34E5C**＝list クラス名の実証）。
- 型値→クラス表（ctor の type byte store 全走査・`v268_1g_out.txt`）で本 2 窓の extra を特定:
  - **type 0x1F = `.?AVExtraPackage@@`**（vt 0xA35914・ctor `FUN_0042A1A0`@0x42A1BF store・RTTI 実デコード `v268_1j2_out.txt`）
  - **type 0x20 = `.?AVExtraTresPassPackage@@`**（vt 0xA35920・ctor `FUN_0042A1E0`@0x42A1E6 store）
- **payload=[entry+0xC]=package ptr 確定**:
  - ExtraPackage ctor（`v268_1i_out.txt` 逐語）: `[+0xC]=arg1・[+0x10]=arg2・[+0x14]=arg3・[+0x18]/[+0x19]=byte arg4/5・ret 0x14`。唯一の E8 callers `FUN_004268B0`@0x426959（直前 `push esi; push edx; push ecx; push edx; push ecx` の arg1=esi を+[0xC] へ）＝リファレンス側 package 付与経路。
  - ExtraTresPass ctor（`v268_1k_out.txt` 逐語）: `[+0xC]=arg1・ret 4`（1 引数=package ptr のみ）。

### 1.2 2 窓のディスパッチ先は TESPackage 族 slot58（+0xE8・**no-arg・純 ret**）

- 窓 1 @0x4251B3-0x4251BE: `ecx=[esi+0xC]; call [[ecx]+0xE8]`（push なし）。**TESPackage primary 0xA6743C+0xE8 = `FUN_005660F0`**（59 slot 中の slot58＝最終 slot。逐語: `mov esi,ecx; [esi+0x24] 非 NULL なら FUN_00569AB0; [esi+0x28] 非 NULL なら jmp FUN_0056A080; pop esi; ret`＝**引数なし・ret 0 形**・`v268_1h_out.txt`）⇒ no-arg call と完全整合。
- 窓 2 @0x42520D-0x42521A: `mov eax,[eax+0xC]; edx=[eax]; eax=[edx+0xE8]; ecx=eax; call eax`。0x20 ペイロードの候補型 = TrespassPackage（0xA747BC+0xE8=`FUN_0067D340`、頭は基底 `FUN_005660F0` 呼び+[this+0x44] 処理＝同 no-arg 形・`v268_1d_out.txt`）。
- TESPackage 族の +0xE8 実装一覧（`v268_1d_out.txt`・primary vtable +0xE8 固有実装）: Alarm=`0x5660F0`（基底共有）／Dialogue=`0x626600`／Flee=`0x628050`／Spectator=`0x67B5E0`（=`jmp 0x5660F0`）／Trespass=`0x67D340`／CombatController=`0x617E80`（基底呼び）— **全実装が no-arg 系**。契約違反の実装は一つもない。
- 直後呼び `FUN_005672A0`（this=[entry+0xC]）が **byte[pkg+0x20]（package type 0..0x20）jump table 0x56764C** で分岐し [pkg+0x18] を書換（`v268_1h_out.txt` 全 0x21 項）＝ディスパッチ対象が package 級オブジェクトである動作証拠（slot58 実装と同一の +0x24/+0x28 サブオブジェクト群を操作）。

### 1.3 ゲート・callers・裁定

- 窓 1 直前ゲート: `eax=[[esi+0xC]+0xC]` → `ecx=[0xB33A98]`（TES mgr）→ `FUN_00446B80(eax)` 逐語 = `cmp [esp+4],0xFF000000; sbb/add → (arg>=0xFF000000 ? 1 : 0); ret 4` ＝**package の [pkg+0xC]（formID 級 dword・名目の target form は [推定]）が一時/生成 form 域のときだけ slot58 を呼ぶ**。窓 2 に formID ゲートなし（NULL チェックのみ）。
- `FUN_00425040` は this=BaseExtraList 所有フォームの extra 掃除・状態更新函（arg=[esp+0x20] flags、bit 0x4000020/0x20/0x40000/0x80000/0x4000/0x200000 で型別処理を切替。E8 callers 2 件: `FUN_00485BC0`@0x485BEB（push 0x20・list ループ）／`FUN_004DA2D0`@0x4DA3CF（`[esi+0x44]` extra list 経由・`shr 0xB bit11`=「一時 form 由来」フラグ検査併設）＝ref 側クリーンアップ文脈［推定］）。
- **裁定**: 2 窓は **TESPackage 族 slot58（+0xE8）窓**であり、MagicItem 族 slot+0xE8（ALCH/INGR `0x4129C0`＝`cmp byte[esp+4],0` 使用・ret 4 setter）ではない。**「no-arg call vs ret 4」不整合は +0xE8 の位相一致（別クラス族の別仮想）による見かけ**で解消。v257 §6.2 の「ExtraData 0x1F/0x23 base form への呼び＝MagicItem 系」は誤読、v265 §6「実消費者=FUN_00425040×2 のみ」も成り立たない。
- 帰結: **MagicItem 族 slot+0xE8 は retail 実行画像内にディスパッチ窓 0 件**（v257 全列挙 8 窓＝本 2 窓→TESPackage 族、残 6 窓→v265 §6.2 で process/NiDX9/float 系に帰属済）。`0x4129C0` の呼び手は TESCS/GECK 側のみと見るのが整合（[推定]・retail dead virtual）。GT 反映は §8 ドラフト (E1)。（注: この撤回は v265 §6.2 の窓 FUN_005E12B0 の「push ecx=actor ptr・ret 4 契約整合」判定には影響しない—窓の実体は同一、MagicItem 系譜説のみ撤回）

### 1.4 本項目内残点

- `[pkg+0xC]` の正確な意味（package target formID 説・ESM ENAM 側との突合は ESM 再スキャン禁止のため未実施）。低。
- extra type **0x23** の ctor store 窓が型値走査（`C6 4? 04 imm`）で不検出（`v268_1g_out.txt` 検出値一覧に 35 なし）＝別 encoding 推定。名目 `.?AVExtraPackages@@` 級と**推定**されるが未確定（FUN_00425040@0x42521C-0x42528E のループは payload をノード先頭 `{+0 head?/prev, +4 next}` 連なりとして free（`FUN_00401F20`）するリスト掃除＝package リスト扱いと整合ではある）。低。
- `FUN_0042A1A0` arg2..arg5（[+0x10..+0x19]）の意味。低。

証跡: `v268_1_out.txt`（FUN_00425040 全文・FUN_0041E210 全文・0x4129C0 逐語・E8 callers・TESPackage 系窓の位置）、`v268_1d_out.txt`（primary +0xE8 全クラス一覧と ret 形）、`v268_1f_out.txt`（entry レイアウト・BaseExtraList 文字列）、`v268_1g_out.txt`（type→クラス全表）、`v268_1h_out.txt`（0x5660F0 全文・switch テーブル）、`v268_1i/1j2/1k_out.txt`（両 ctor 逐語・RTTI 5 本・callers 窓）。

## 2. 【低2】process 族 slot58（actor->vt[0x154]→[process+0x15C] キャッシュ）の語義・callers 32 窓分類 — **[確定]**（機構＝**head ノード従属obj の process レベルキャッシュ**／語義名目のみ [推定]）

### 2.1 actor->vt[0x154] の実体（4 クラス実測）

- slot 実値（`v268_2_out.txt`/`v268_2b_out.txt`）: Actor 0xA6E074+0x154=`FUN_00422DE0`（逐語 `mov eax,[ecx+0x3C]; ret`＝**[actor+0x3C] 無条件 getter**・no-arg 返り値 ptr）／Creature 0xA710F4 同一（基底そのもの）／Character 0xA6FC9C=`FUN_00612220`（**`this==[0xB333C4]`(PC) でなければ [actor+0x3C] の byte[+0x18] bit0 立時 0 返却**＝非 player で「対象無効」フラグ時に NULL 化）／PC 0xA73A0C=`FUN_006600F0`（`byte[PC+0x588]==0 かつ [PC+0x5D0]!=0 なら [PC+0x5D0] 優先、でなければ jmp 0x612220`＝**v265 §1.2 の +0x588 交換ガード同族の専用スロット優先形**）。
- [actor+0x3C] の級: 消費窓 FUN_004DDFD0（下記）が **文字列 `'BSFaceGenNiNodeSkinned'`@0xA46ED8（実測・クラス名でなく NIF 探索名）** による子ノード検索に渡す＝actor の **3D シーン根ノードハンドラ [推定]**（NiAVObject 級）。[PC+0x5D0] の .text store は disp32 形で **0 件**（唯一の +0x5D0 store@0x76A37E は NiDX9Renderer フィールドで別物）＝未設定時 PC 分岐は Character 版へ自然フォールバック（生成窓 [未確定・低]）。

### 2.2 [process+0x15C] キャッシュ機構（High/MiddleHighProcess +0xE8 = FUN_0064B350）

- 全文（`v268_2_out.txt`）: arg=actor（ret 4・契約整合）。①`eax=actor->vt[0x154]()`；②eax==0 → **`[process+0x15C]=0` クリア**；③非 0 かつ `[process+0x15C]==0` のときのみ **`FUN_004D66B0(actor, 0)`** で算出しキャッシュ；④返り値=キャッシュ。＝**負値なし・ゼロ＝未算出/無効の単一スロットキャッシュ**。
- `FUN_004D66B0`（`v268_2b_out.txt`）: `actor->vt[0x138](0)`→ 非 NULL なら **`result->vt[0x9C]()`** を返す 2 段連結。
  - vt[0x138]=`FUN_004D66A0`（`mov eax,[ecx]; jmp [eax+0x134]` の **alias thunk＝実体は vt[0x134]**・4 クラス同一）。
  - vt[0x134]=`FUN_004DDFD0`（`v268_2d_out.txt` 全文）: `vt[0x190]()`（=`FUN_00977C50` `mov al,1; ret`＝既定 true）かつ `vt[0x27C]()`（Actor 基底=0x982D85 purecall／PC=`0x8CF6B0`＝派生必須の実体有無ゲート）非 NULL なら **`FUN_006F94A0([actor+0x3C], 'BSFaceGenNiNodeSkinned')`** ＝根ノードから head スキンノードを検索（`FUN_006F94A0` は `FUN_00700010([0xB3CAC0],name)` ハッシュ→ノード->vt[0x4C]/vt[0x58] 名称検索経路・`v268_2h_out.txt`）。
  - head ノード（BSFaceGenNiNode 0xA64F5C・50 slot）の **vt[0x9C]=`FUN_0055CEF0`（`mov eax,[ecx+0xDC]; ret`）** ＝**head ノード従属オブジェクトハンドラ [+0xDC] の getter**（`v268_2i_out.txt`）。
- ⇒ **[process+0x15C] = 「actor 3D 根→head スキンノード→従属 obj([+0xDC])」の検索結果キャッシュ**［確定・機構］。用途名目: 32 窓の使用形（下記）＋消費側 FUN_0054B010（`byte[+0x1DA]/[+0x1D5]`・`float[+0x174]` 操作・ret 0xC）と `mov byte[eax+0x1DB]` 書込群から**表情/口・head 連動 subsystem obj [推定]**（クラス名・字段名目は EXE 内文字列証拠なし）。
- 低 process レベル無効: Base/Low/MiddleLowProcess +0xE8=`FUN_004943A0`（`xor eax,eax; ret 4`＝常時 0・`v268_1d/2_out`）＋ v265 確定ゲート `process->vt[0x08]()<=1`（High/MiddleHigh のみ）＝**ディテール段階連動キャッシュ**として自己整合。
- actor 側 thunk `FUN_005E12B0` 全文（`v268_2b_out.txt`）: `if [actor+0x58](process)==0 → 0；否则 push actor(process->vt[0xE8](actor))`＝ret 4 契約一致の唯一実窓（E8 callers 32・`call [reg+0x154]` 直窓は disp32 走査 0 件だが `mov edx,[eax+0x154]; call edx` 形は存在: 0x6A4381・0x6282E9 等）。

### 2.3 callers 32 窓の分類（窓直後命令の eax 使用形・`v268_2d_out.txt`/`v268_2g_out.txt`）

| 群 | 使用形 | 件数 | 窓 host（帯） | 判定 |
|---|---|---|---|---|
| A | `mov byte ptr [eax+0x1DB], 0/bl` | 6 | 0x5BE400・0x5BEC70×2・0x5BF470・0x5BFB90×2・0x5C01D0（0x5BE-0x5C0 挙動更新帯・直前に `FUN_006B7BA0/006B7C20` の list ノードAccessor と `[ctx+0xD8]` の actor 取得併用） | 表情/head フラグリセット書込［機構確定・名目推定］ |
| B | `test eax,eax` ゲート | 14 | 0x5BE270・0x5F7310・0x603500(1)・0x6282B0・0x650030・0x6507C0・0x652A20・0x6545E0×4・0x654F10・0x678750・0x6A3AC0 | キャッシュ生存検査（家具着席帯 0x650-0x654＝v265 §1 の着席系譜と同帯。0x654F10 は v257_9 の vt[0x164] ゲート窓と同居） |
| C | `mov reg,eax`→後続呼（`FUN_0054B010`・vt 経由・引数化） | 9 | 0x5F743A（`byte[[PC]+0x588]` ゲート併設＝着席/交換ガード連動）・0x6019CF・0x603500×5（0x54B010 表情更新・0x5E05F0 push 0x30 併発）・0x62D9D9・0x63CE80・0x6A4397（vt[0x154] 直叩き併設） | キャッシュ値の実消費（head obj 更新・IK/戦闘連鎖）［帯帰属は推定］ |
| D | その他（`cmp byte[eax+..]` 書込・破棄） | 3 | 0x603C47・（0x5BE343 は B に含） | 境界窓 |
- host 関数の帯別内訳: 0x5BE-0x5C0（8 窓）=挙動/表情更新／0x5F7・0x601・0x603（11 窓）=対話・interaction・head 更新／0x628・0x62D・0x63C（3 窓）=anim・weapon 連鎖／0x650・0x652・0x654（8 窓）=家具着席／0x678・0x6A3・0x6A4（3 窓）=その他 UI/戦闘帯［いずれも帯位置と併用命令からの [推定]、名目文字列は画像内なし］。

### 2.4 本項目内残点

- [actor+0x3C] の書込元（actor 生成時の 3D 根割当窓）は disp8 store 走査で 39 窓混雑、Actor 帯の割当 1 点に未収束。低。
- head 従属 obj（[node+0xDC]）の具体クラス名（RTTI 未到達・生成側 0x6BC600/0x6C0170 帯の `mov [eax-0x24]` 窓は行列演算併設で型確定至らず）。低。
- `byte[x+0x1DB]/0x1D5/0x1DA` 字段の意味名（口アニメ/表情有効フラグ候補）。低。

証跡: `v268_2_out.txt`（0x64B350 全文・slot 実値・[+0x15C] disp32 全 69 窓）、`v268_2b_out.txt`（0x612220/0x6600D0/0x4D66B0/0x5E12B0 逐語・callers 32 一覧）、`v268_2c/2d_out.txt`（PC slot 実測・alias thunk 0x4D66A0・FUN_004DDFD0 全文・32 窓使用形分類）、`v268_2e/2f_out.txt`（cast 名文字列・vt[0x190/0x27C] 実体・0x65BCA0 ゼロクリア形）、`v268_2g_out.txt`（32 窓文脈逐語）、`v268_2h/2i/2j_out.txt`（FUN_006F94A0/0x700010・NiNode slot+0x9C=0x55CEF0・[+0x5D0] store 全走査）。

## 3. 【低3】UIMenu message ルータ側 vt[0x04] の id 定義表（HUDInfoMenu id 2/0xA の発火名目） — **[確定]**（id 定義表=Menu 級 48 クラスの +0x04 仮想 case 表そのもの／発火名目の細部は [推定] 併記）

### 3.1 +0x04 仮想の性格確定（全 Menu 級 vtable 実測・`v268_3g/3h_out.txt`）

- Menu 系 primary vtable（0xA6B000-0xA6E000 帯・15 slot 形）の **+0x04 = 「(id, interface_ptr) を受けて self の +0x28.. フィールド群へ結線する attach ルータ」**。基底 `Menu` 0xA696DC+0x04=`FUN_0060CF60`（純 `ret 8` no-op＝未対応メニューは無視）、`GenericMenu`/`LoadingMenu` も同 no-op。
- 各実装頭は例外なく `mov eax,[esp+4]; cmp/sub imm; …` の id switch（head byte 列 `8B 44 24 04 83 F8 xx` 群・`v268_3g_out.txt`）。arg2=[esp+8]=interface ptr を case 先フィールドへ store（v265 §4.1 の HUDInfoMenu 全 case 表と同形）。
- **id 定義表（Menu 級 48 の id→結線先 field・`v268_3h_out.txt` より要点）**:
  - **HUDInfoMenu 0xA6BDF4**: 1→+0x28 / **2→+0x2C** / 3→+0x30 / 4→+0x34 / 5→+0x38 / 6→+0x3C / 7→+0x40 / 8→+0x44 / 9→+0x48 / **0xA→+0x4C** / 0xB→+0x50（＝v265 §4.1 と独立再現✓）
  - HUDMainMenu 0xA6BEE4: 1..8→+0x28..+0x48（順）/ 0xE→+0x44 / 0x10→+0x50 / 0x12→+0x54 / 0x14→+0x58 / **0x5B→+0x4C**
  - MessageMenu 0xA6CE5C: 1..0xD→+0x28..+0x58（0..0xC スライド写像）／AudioMenu 1..0xA,0xC→+0x28..+0x50／DialogMenu 1..0x10 の 15 項／PersuasionMenu 1..0x1C→+0x3C..+0xD4／StatsMenu **0xA,0x14,0x1E,0x1F,0x20,0x28,0x29,0x2A,0x32,0x33,0x34,0x3C,0x3D**→+0x28..+0x58（10 刻み id 列＝UIOblivion の kInterface_* スロット番号と整合する離散値）
  - 単一 id 系: AlchemyMenu 0x13→+0x28／EffectSettingMenu 0x23→+0x28／EnchantmentMenu 0x18→+0x3C／MapMenu 0x3E→+0x28／MagicPopupMenu 0x14→+0x28／QuickKeysMenu 0x9→+0x28／RepairMenu 0x10／RechargeMenu 0x5／SpellMakingMenu 0x13／SpellPurchaseMenu 0x7／TrainingMenu 0xB／SigilStoneMenu 0x18→+0x30／Inventory/Container/Skills=4→+0x28／Load/Save=8→+0x3C／Magic=3／HUDSubtitle 1,2／Breath 1,2／Quantity 1..7／Pause 3,4,5,7,8／SleepWait 3..0xB／OptionsMenu 2,5..9／Gameplay 9→+0x48／MainMenu 2..8→+0x28..+0x44／RaceSex 1,2,0xA,0x14,0x15,0x5A／VideoDisplay 1..8／LockPick 1..0x14（+0x178..+0x144 の大領域）／Class/Book/Controls/Credits/LevelUp/TextEdit/Tile/Video=jump table 形（case 抽出不能・+0x04 実体 VA 一覧済）。
  - ⇒ **id 名前空間はメニュー毎ローカル**（同一数値でも結線先別）＝「UIMenu message ルータ側 id 定義表」とは本表のこと。共通列挙（kMessage_*）は +0x04 の id 欄には存在しない（各 case が即 interface スロット index）。
- 送信側形態: E8 直接呼びは実装 0x5A4700 で **0 件**（純仮想ディスパッチのみ・`v268_3f_out.txt`）。2 引数+bool 返却の `push &data; push id; call [[obj]+4]` 形窓群が `FUN_004383B0/004384E0/00438540/00439DC0/00439EB0/00439FF0/0043B420/0043B5E0/0043B840`（0x438-0x43B 帯・`v268_3c_out.txt`）で、うち 0x43A100 窓は XML プロパティ処理（文字列 `'FlameNode'`@0xA370A8 と 9 文字比較・BSX 組込帯）＝**帯性格は Ni/BSX プロパティ配線系**と実測。**UIMenu 専用 broadcast 関数（メニュー配列を回して +0x04 を 2 引数で叩く単一ルータ）は .text の形走査（`call [reg+4]`＋push imm id・`v268_3k_out.txt`）で特定至らず** → 発火経路の一点特定は [未確定・低]（推定: UI スレッド側 NiInterfaceManager 接続 callback・UIOblivion の SetInterface(kInterface_*) 相当・[推定]）。

### 3.2 HUDInfoMenu id 2/0xA の発火名目（EXE 内読手からの帰結）

- **id=2 → [this+0x2C]**: 読手=HUD 末尾列 `0x5A4FFB`・`0x5A4F1D` 帯（v265 §4.1/§4.2 確定済み＝ロック難易度ラベル（タイル 0xFDE 系）描画経路）と `FUN_005A5540`（vtable+0x2C slot・tick `%0x3C`→タイル 0xFB2 読→`FUN_005A4980` 再描画）。⇒ 発火名目 = **ドア/ロック情報表示タイル用の interface 接続（必須接続の一部）**［確定・機構］（固有名 [推定]）。
- **id=0xA → [this+0x4C]**: 読手=`0x5A4FA8` の `ebp=[esi+0x4C]` ゲート（末尾列の有効判定）。⇒ 発火名目 = **HUD 情報列の有効化ゲートが読む interface**［確定・機構］。
- 接続完了判定: `FUN_005A46B0`（+0x28..+0x4C の 10 連 non-null・v265 §4.1）＝**id 1..0xA が全て接続されて初めて HUDInfoMenu が稼働**＝id 2/0xA は「ロード時に必ず発火される必須接続」枠。0xB（+0x50）は判定外（任意）。
- menu 生成窓（参考）: ファクトリ `FUN_00587D20` の menu type **6** 枠（@0x587EAC `new 0x5C`→ctor `0x5A4660`・`v268_3b` 併記）＝HUDInfoMenu は type 6 として単一生成・全局 [0xB3B33C]。

証跡: `v268_3_out.txt`（0xA6BDF4 RTTI/slot0..0x1C・call [reg+4] 生走査 196 件）、`v268_3b_out.txt`（[0xB3B33C] 参照 7 窓逐語）、`v268_3c_out.txt`（mov+call [vt+4] 757 窓文脈）、`v268_3e/3f_out.txt`（UI 帯 direct 形 0 件・disp32 形 0 件）、`v268_3g_out.txt`（Menu 級 +0x04 実装 VA 一覧・0x5A47B0/0x5A47E0 全文・E8 callers 0 件）、`v268_3h_out.txt`（48 クラス id→field 全表）、`v268_3i/3k_out.txt`（push 付き窓フィルタ・精密走査の到達範囲）。

### 3.3 本項目内残点

- UIMenu 専用 broadcast（sender）関数の一点特定（0x438-0x43B 帯の [obj+4] 2 引数窓が BSX 側と混在、UI 固有ものの切り分け未完）。低。
- Book/Class/Controls/Credits/LevelUp/TextEdit/Tile/VideoMenu の jump table 形 case 展開（id 表欠落）。低。
- 発火名目の固有名（kInterface_* 相当）は EXE 内文字列証拠なし＝UIOblivion ソース側対応（本束スコープ外）。低。

## 4. 【低4】0x31/0x32 extra の [entry+0xC] formID/ptr 別・FUN_00429990 レシーバ form 級 — **[確定]**（両 extra とも **ptr**／レシーバ=**ExtraLock データ構造 0x10B**（TESForm でない））

### 4.1 経路逐語（FUN_004D7780・`v268_4_out.txt`）

- `FUN_004D7780`（this=TESObjectREFR・ecx→`esi=[ecx+0x44]`＝ref の extra list（+0x44 配置は TESObjectREFR ctor `FUN_004D9A70` の `lea ecx,[esi+0x44]; call FUN_00422EE0`（BaseExtraList ctor）で実証・`v268_4c_out.txt`）:
  1. `FUN_0041E690` = **extra 0x31（`.?AVExtraLock@@`・vtable 0xA357B8・RTTI 実デコード）** 検索→有れば **[entry+0xC] を素返却**（`mov eax,[eax+0xc]`・deref だけ＝**ptr 使用**）。
  2. 無ければ `FUN_0041E6B0` = **extra 0x32（`.?AVExtraTeleport@@`・vtable 0xA357DC・RTTI 実デコード）** 検索→`esi=[entry+0xC]`（teleport データ ptr）→ `FUN_0042B410`（逐語 `mov eax,[ecx]; ret`＝**データ[+0]=接続先参照 ptr**）→ 返却先 REF の `[eax+0x44]` extra list で **もう一度 0x31 を検索**。
  3. 最終的に ExtraLock データ P を ecx に **tail jmp `FUN_00429990`**（@0x4D77C1）。
- ⇒ 経路中に **formID 解決窓（FUN_0046B250 系）一切なし**、`[entry+0xC]` は即 deref。**裁定: 0x31/0x32 とも [entry+0xC] は runtime データへの ptr**（formID ではない）。生成窓 `FUN_0041EAF0`（set-lock・`v268_4d_out.txt`）が `new 0x10` のデータ obj を arg に `ExtraLock ctor(FUN_00429A80)`→`[+0xC]=arg` を確認。**書換経路は旧データ `FUN_00401F20`(free)→[entry+0xC]=arg 置換**＝ヒープ所有。
- ExtraTeleport ctor（`FUN_00429C10`）も同形（`[+0xC]=arg1・ret 4`）。type store 走査の 0x31 混入 @0x4D9AD4 は **TESObjectREFR formType byte[+4]=0x31** の位相一致で本件無関係（ctor 頭 0xA46C44 store 併設で確定）。

### 4.2 FUN_00429990 レシーバ級（ExtraLock データ・form 級でない）

- 全文（`v268_4_out.txt`）: `test byte[ecx+8],4`（**データ+8 flags bit2 = leveled（actor value 連動）**）／`movsx eax, byte[ecx]`（**データ+0 = 基準難易度 byte**）→ bit2 時 `[0xB333C4]`(PC) の `FUN_005E1FD0`（Security skill・word）× `fLeveledLockMult`([0xB33880]) + 基準 → `FUN_009828C0`（float→int）→ **`cmp eax,0x63` 超なら 99 にクランプ**。
- ⇒ **レシーバは TESForm/`TESLock` 級フォームではなく ExtraLock の 0x10B データ構造**（{+0 byte level, +8 dword flags}＋残り 0xC.. 未査読・[低]）。**訂正**: v265 §4.2 の「100 クランプ」は実測 **99（0x63）**（GT ドラフト E3）。
- 直接 callers（E8・`v268_4_out.txt`）: `FUN_004B66B0`@0x4B68D2／`FUN_004B7490`@0x4B7639／`FUN_004B8A40`×4（着席/開錠大函・v265 §1.3 と同一 host）／`FUN_004F45D0`@0x4F47CC ＝ 開錠・使用帯 7 窓＋tail 窓 0x4D77C1。`FUN_00429A30`（tier 0..4→閾値 [0xB338B8..D8]・default 0x64）の jump table 実測も併記済（`v268_4b_out.txt`）。
- 「key form 経由」（v265 §4.2 表現）の検証: 0x4D7780 経路にキー解決は存在せず、**0x32 分岐は「接続先 REF の ExtraLock を拾う」だけ**＝キー formID は無関係（GT ドラフト E3 で表現修正）。キー連携は ExtraLock データの未査読 0xC 以降または別経路（残点）。

証跡: `v268_4_out.txt`（0x4D7780/0x41E690/0x41E6B0/0x429990 全文・type store 窓・callers）、`v268_4b_out.txt`（両 ctor・FUN_0042B410/0x429A30 全文）、`v268_4c_out.txt`（RTTI 2 本・REFR ctor 確認）、`v268_4d_out.txt`（FUN_0041EAF0 set-lock 全文）。

### 4.3 本項目内残点

- ExtraLock データ +0xC..（キー連携・"unlocked" 状態ビット等）の全字段割当。低。
- ExtraTeleport データ（[[P]]=接続 REF）の +4 以降（位置 float3・rot?）レイアウト。低。

## 5. 【低5】[0xB33A98]（TES*）・[0xB33B00] の生成書込点（CRT 外） — **[確定]**（生成=エンジン初期化連鎖 `FUN_00441970`・CRT 初期化子関与なし）

### 5.1 走査結果（.text 全域・store/lea 形全集）

- 両アドレスは **.bss**（va2off 未マッピング＝画像値 0・`v268_5b/5c_out.txt`）。runtime で書かれる。
- imm32 全出現を命令形態別分類（`v268_5d_out.txt`、89/C7/8D+modrm(disp32)・A1/A3・FF 35 形）: **[0xB33A98] への store は .text に 2 点のみ** = `mov [0xB33A98], eax` @**0x441B8A**（FUN_00441970・生成）と `mov [0xB33A98], ebx(=0)` @0x446923（FUN_004466F0・v265 既載の dtor ゼロ化）。**[0xB33B00] への store は 3 点** = `mov [0xB33B00], eax` @**0x441BB9**（FUN_00441970・生成）、`mov [0xB33B00], esi` @**0x45B3C8**（FUN_0045B300＝ctor 内の self-register）、`mov [0xB33B00], ebx(=0)` @0x44690B（FUN_004466F0・ゼロ化）。**lea 形 = 0 件**（v265 の「非ゼロ store 無し」は A3 形（`A3 98 33 B3 00`）不落による誤脱と訂正・store 全集を本走査で再取得）。
- 負側: FUN_00441970 冒頭 @0x44199E `mov [esi], 0xA37470` ＝ **TES vtable（v265 §3.3 COL=`.?AVTES@@`）書込**＝関数自身 this 側の再初期化（v265「dtor 暫定 vtable」と同 vtable・生成/破棄両側で同一書込形）。

### 5.2 生成窓逐語（`v268_5e_out.txt`）

- `FUN_00441970`（thiscall・SEH/GS あり）内:
  - `push 0xCE0 → new(0x401F00)` → `mov ecx,eax; call FUN_00446D80`（TES ctor）→ **@0x441B8A `mov [0xB33A98], eax`** → `push [esp+0x28]; call FUN_0044A2B0`（TES 後初期化・arg=呼び手コンテキスト）。
  - 直後 `push 0x1C4 → new` → `mov ecx,eax; call FUN_0045B300`（ctor）→ **@0x441BB9 `mov [0xB33B00], eax`**。
  - `FUN_0045B300`（0x1C4 obj ctor）は @0x45B3C8 で `mov [0xB33B00], esi` **自分自身も global へ self-register**（＝同一 obj の二重書込・生成1経路に整合。他から ctor が呼ばれた場合も global が追随する設計）。
  - 末尾側で [0xB06A2C]/[0xB051DC]/[0xB051D4] 系のメモリ高域計算（別件）。
- 連鎖（E8 実測）: `FUN_00441970` ← **@0x40662D in `FUN_00406360`** ← @0x40EB2C in **`FUN_0040D4D0`**（WinMain/ゲーム起動大函・[0xB33A98]/[0xB33B00] の A1 load@0x40D550/0x40D55C 併設と自己整合）。
- ⇒ **裁定**: **[0xB33A98]（TES シングルトン）と [0xB33B00]（0x1C4B engine サブシングルトン・TESSaveLoadGame 相当候補は v265 §3.4 のまま未検証）の生成書込点は `FUN_00441970` @0x441B8A/@0x441BB9（＋0xB33B00 は ctor 内 self-store @0x45B3C8）**。CRT 初期化子（0x9D9-0x9DC 帯・atexit 群）からの書込は**無し**＝起動シーケンス（0x40D4D0→0x406360→0x441970）で生成、shutdown/reload 側（FUN_004466F0）でゼロ化。v265 §3.3「推定: main 起動器」が **実窓として確定**。

### 5.3 本項目内残点

- [0xB33B00] obj（0x1C4B）の RTTI（ctor@0x45B300 に `mov [reg], vtbl` 形が見当たらず、サブ ctor 経由の vtable 書込と推定・クラス名未回収）。低。
- `FUN_0044A2B0` の TES 後初期化引数（[esp+0x28]）の実体。低。

証跡: `v268_5b/5c/5d_out.txt`（.bss 判定・imm32 全出現形態分類・store/lea 全集）、`v268_5e_out.txt`（FUN_00441970 生成窓逐語・FUN_0045B300 頭・呼び手連鎖）、`v268_5f_out.txt`（vtable store 不在確認・0x406360 callers）。

## 6. 【低6】formType レジストリ TOFT 項 +8=0xA3BA94 異常値の意味 — **[確定]**（異常値でなく **隣接グローバル `NiTMapBase<…, TESForm*>` の vtable スロットとの領界共有**・v265 の値記載も 1 桁誤り）

### 6.1 実測訂正

- 画像値実測（`v268_6_out.txt`・raw=VA-0x401400）: `[0xB06134]=0xA3BA68`（'TOFT' name ptr ✓）、`[0xB06138]=0`（4CC 画像 0 ✓）、**`[0xB0613C]=0x00A3BC94`**（v265 §2.1 の「0x00A3BA94」は**誤記**・C↔A 1 桁差。v265_2_out の dump 自体は idx 0..67 の seq=1..68 を正しく列挙）。隣接 `[0xB06140]=0x0002008D`・`[0xB06144]=0`・`[0xB06148]=0`。
- **0xA3BC94 は vtable**（`v268_6_out.txt`）: slot 群 = `0x46B030 / 0x6A9060 / 0x763E80 / 0x67F130 / 0x68F970 / 0x982D85(purecall) / 0x982D85 / …`、**RTTI 実デコード = `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@IPAVTESForm@@@@`**（＝`NiTMapBase<NiTPointerAllocator<unsigned>, unsigned, TESForm*>`）。

### 6.2 意味裁定

- レジストリ本体は **idx 0..0x43 の {+0 name ptr, +4 4CC, +8 seq(=i+1)} ×68 項**で +8 列は idx 0x43(EFSH)=68 で終了。**idx 0x44(TOFT) は {+0 name, +4 4CC} の 8 バイトだけ**で、+8 スロット（0xB0613C）は直後配置の静的グローバル＝**TESForm*  assoc map object の vtable ptr** が立つ（MSVC の .data 連続配置・stride 12 仮定の越境読み）。
- ⇒ **「seq=異常値」は異常ではない**（TOFT に序号欄は存在しない＝TOFT は form 化しない 4CC 枠という v265 §2.3 の実務裁定と整合）。4CC→formType 検索 `FUN_0046AF70` の走査窓 `[0xB05E08, 0xB06144)` は 4CC スロットのみを見るので**機能は完全に正常**（TOFT の formType=68 は検索 index から得られ、seq 欄に依存しない）。GT ドラフト E2 で「TOFT +8 異常 [未確定]」を**解消**（レジストリ 68×12B+TOFT 8B＋隣接 NiTMapBase）。
- 当該 map（this=0xB0613C）の用途窓（`v268_6_out.txt` imm32 全出現＋既知窓）: `FUN_0046B250`（formID→form 解決・**[entry+0xC] の formID を ptr にインプレース書換**する FUN_00425040@0x425127/0x4251EF/0x4252D4 群と同一＝読み込み時 formID→runtime ptr 変換の本体、項目4 と相互補強）、`FUN_0044AC20`（TESSaveLoadGame 系・plugin slot 4CC `push 0x36..0x3A/7..0x17` を `[[0xB0613C]+4]` に渡して返 index で `[0xB06144]+4*idx` を引く FILE 参照ループ）、`FUN_004471A0/00447350..00447440/00448D40/004491D0/004492E0` 帯（thiscall 多数・マップ保守）。map 値型 `TESForm*` の RTTI と一致し、**v265 §3 の変更リスト 0xB0614C（NiTLargeArray<TESForm*>）とは別物**（0xB0613C map=検索用 assoc／0xB0614C=dirty list・隣接地だが独立）。

証跡: `v268_6_out.txt`（画像値 11 連・imm32 0xB0613C 全出現・FUN_0055E000 頭・0x44C684 窓逐語）、本会話内 dump（0xA3BC94 vtable スロット 8 個・RTTI 文字列）。

### 6.3 本項目内残点

- map の +4（0xB06140=0x0002008D の 2 word 0x8D/0x02）の意味（バケット数/生成 flag 候補）。低。
- 0xB06144 データ配列の割当窓（ランタイム new）特定。低。

## 7. 【低7】GECK 側 bit24 設定挙動（TESCS.exe「Use actor value」連動） — **[未確定]**（一次資料取得できず・web 不通明記）

### 7.1 今束の試行記録（すべて外枠・2026-11-04 実施）

- `webfetch https://en.uesp.net/wiki/Oblivion_Mod:Mod_File_Format/MGEF` → **503**（v256/v257/v265 に続き直接通路不通継続）。
- `webfetch https://cs.uesp.net/wiki/Magic_Effects`（Construction Set wiki・チェックボックス説明候補）→ **503**。
- `webfetch tes5edit.github.io/fopdoc/FalloutNV/Records/MGEF.html` → **503**。
- `websearch` 4 発（bit24/0x01000000/Use actor value 連動/xEdit talk 系）→ 決定的一次資料なし。得られた傍証のみ下記。

### 7.2 傍証（snippet 段階・断定不可）

- UESP MGEF（2024-07-30 版スニペット再取得）: 「`0x00080000 = Use skill : 0x00100000 = Use actor value : 0x02000000 …`」＝**bit20=Use actor value 据え置き・同表のフラグ列は 0x00100000 の次が 0x02000000 で、0x01000000（bit24）行は不在**（v265 §7.2「bit24 未定義」の独立再現✓）。
- xEdit fopdoc（FalloutNV MGEF・検索スニペット表示のみ）: 「`Flag Values Value Meaning … 21 ?? 22 ?? 23 ?? 24 …`」＝後継エンジン系の公開表でも bit21-24 は無名（"??"）。Oblivion 固有の挙動証拠ではない。
- EXE 側既知事実は v265 §7.3 のまま（@0x4131B7 `test edi,0x1180000`→bit24 単独検査→+0x60 を EffectItem+0x14 copy）。bit24 マスク 0x01000000 の即値出現は画像全体 1037 件で、magic 帯外の意味窓切り分けは未了（`v268_7_out.txt`・低）。

### 7.3 裁定と残点

- 「GECK(TESCS.exe) が 'Use actor value' チェック時に bit20 と bit24 を連動設定する」仮説は**確認できず**（一次資料 web 取得失敗・ローカルに TESCS.exe の解析対象なし・実機 UI 確認不可）。**[未確定] 継続**（v265 §7.4 のまま。v256 の「bit24=AV 宣言ビット」機構事実自体は EXE 実測で有効）。
- 次アクション候補（本束スコープ外）: TESCS.exe（GOTY Construction Set）の同一文字列帯検索、或いは UESP ページの talk/履歴側で bit24 の言及追跡。

証跡: `v268_7_out.txt`（imm 0x01000000 出現数）、webfetch 503×3 記録・websearch snippet 2 件（§7.1/7.2 本文に転記）。

## 8. GT/plan ドラフト（反映は要承認・本体無編集・既存ドラフト行は本節のみ）

- **(E1 訂正・項目1)**: v257 §6.2 / v265 §6.2-§6「MagicItem 族 slot+0xE8 の実消費者=FUN_00425040×2」→ **撤回**。2 窓のレシーバ実クラス=**TESPackage 族**（type 0x1F=ExtraPackage/0x20=ExtraTresPassPackage の payload=package ptr・TESPackage 0xA6743C+0xE8=`FUN_005660F0` no-arg 純 ret・子類 override 群 no-arg）。no-arg/ret 4 不整合は +0xE8 位相一致の見かけ。**MagicItem 族 slot+0xE8（ALCH/INGR `0x4129C0`・ret 4）は retail 内ディスパッチ窓 0 件**＝起動画像上 dead virtual（呼び手は TESCS/GECK 側 [推定]）。v265 §6.2 表の FUN_005E12B0 窓（process 族 actor ptr push）判定は維持。
- **(E2 解消・項目6)**: formType レジストリ「TOFT 項 +8=0xA3BA94 異常値」→ **実測 0x00A3BC94 として訂正**・実体は**隣接静的グローバル `NiTMapBase<NiTPointerAllocator<uint>, uint, TESForm*>`（vtable 0xA3BC94・this=0xB0613C）の vtable スロット**（レジストリ +8 seq 列は idx 0..0x43=1..68 で終了、TOFT 項は {name,4CC} の 8B）。異常でなく境界共有。`FUN_0046B250`（formID→form 解決・extra payload インプレース置換窓 0x425114→0x425127 等）と同 map の使用者。
- **(E3 訂正・項目4)**: (AJ) 系譜の DOOR 鎖: 「`FUN_004D7780` の 0x31/0x32 [entry+0xC] formID/ptr 別未確定」→ **両 extra とも ptr**（ExtraLock=`.?AVExtraLock@@` vtable 0xA357B8・データ{+0 byte 難易度, +8 dword flags bit2=leveled}・生成 `FUN_0041EAF0` new 0x10／0x32=`.?AVExtraTeleport@@` 0xA357DC・[[payload]]=接続 REF）。`FUN_00429990` のレシーバ=**ExtraLock データ 0x10B**（TESForm 級でなく key form 経由でもなく、0x32 分岐は接続先 REF の ExtraLock 検索）。**クランプ値 99（0x63）に訂正**（v265 §4.2「100 クランプ」）。
- **(E4 追記・項目5)**: TES シングルトン生成窓確定: `FUN_00441970`@0x441B8A（new 0xCE0→ctor `FUN_00446D80`→`[0xB33A98]=eax`→`FUN_0044A2B0` 後初期化）／`[0xB33B00]`=同 @0x441BB9（new 0x1C4→ctor `FUN_0045B300`、ctor 内 self-register @0x45B3C8 併存）。連鎖=WinMain 帯 `FUN_0040D4D0`←`FUN_00406360`←@0x40662D＝**CRT 外・起動器生成**（v265 §3.3「推定: main 起動器」を確定化、§3.4 残点解消）。ゼロ化=`FUN_004466F0`@0x44690B/0x446923。**v265 §3.3「0xB33A98 への非ゼロ store は .text なし」は A3 形不落による誤りとして訂正**。
- **(E5 追記・項目2)**: process 族 slot58（actor->vt[0x154]→[process+0x15C]）機構確定: vt[0x154]=[actor+0x3C] getter（Actor `FUN_00422DE0`・Character 0x612220 非 player+flag フィルタ・PC 0x6600F0 [PC+0x5D0] 優先+ガード）／キャッシュ=`FUN_0064B350`（High/MiddleHigh のみ・Low 系 0x4943A0 常時 0）／算出=vt[0x138](alias→0x134)→`FUN_004DDFD0`（根ノードから名 **'BSFaceGenNiNodeSkinned'**@0xA46ED8 検索）→ 結果->vt[0x9C]（BSFaceGenNiNode `FUN_0055CEF0`=`[node+0xDC]` getter）＝**head ノード従属 obj ハンドラのキャッシュ**。32 callers は A=byte[+0x1DB] 書込群6（0x5BE-0x5C0 帯）/B=test ゲート14（家具着席帯含む）/C=チェーン消費9/その他3。
- **(E6 追記・項目3)**: UIMenu「router id 定義表」=Menu 級 48 クラス vtable+0x04 の case 表そのもの（`v268_3h_out.txt`・HUDInfoMenu 1..0xB→+0x28..+0x50 含む）。基底 `Menu`+0x04=0x60CF60 no-op ret 8。HUDInfoMenu id2(+0x2C)=ロック/ドア表示タイル群読手・id0xA(+0x4C)=末尾列ゲート（`FUN_005A46B0` 10 連接続完了述語＝**id 1..0xA は起動時必須接続**、0xB は述語外）。sender 一点特定は未了（低）。
- **(E7 維持・項目7)**: bit24=**「確認できず」継続**（webfetch 503×3・websearch snippet は「UESP 表 bit24 行不在」「xEdit NV でも bit21-24 無名」の傍証のみ）。移植注記: MGEF flags は bit20（公式名）と EXE 要求 bit24 の**二重条件**である事実（v265 §7.3）をそのまま採用。
- **plan（移植）影響候補**: extra payload=ptr 確定（項目4）により ESM リーダ側は load 時 formID→ptr 変換（0xB0613C map 相当）を実装済みであるべき／`FUN_00441970` 生成連鎖（項目5）は移植側シングルトン初期化順の参考／TESPackage slot58 と MagicItem slot+0xE8 の**位相衝突**（項目1）は GT の「+0xE8 系譜表」を vtable 族別に持つ必要があることの再確認。

## 9. 自己検証・判定まとめ

- **判定**: 1 **[確定]**（TESPackage 族・v257/265 裁定撤回）／2 **[確定・機構]**（語義名目 [推定]）／3 **[確定・表]**（sender 一点 [未確定]）／4 **[確定]**（ptr・ExtraLock データ級・クランプ 99 訂正）／5 **[確定]**（生成窓 `FUN_00441970`・A3 形訂正）／6 **[確定]**（隣接 NiTMapBase の vtable スロット・値 0xA3BC94 に訂正）／7 **[未確定]**（web 不通・「確認できず」明記）。
- **新規確定 VA/データ**: extra 型表 0x1F/0x20/0x22/0x31/0x32/0x39/0x3E/0x48（v268_1g）・`FUN_0042A1A0`（ExtraPackage ctor 5 引数 ret 0x14）・`FUN_0042A1E0`（ExtraTresPass ctor）・`FUN_00429A80/429C10`（ExtraLock/Teleport ctor）・`FUN_0041EAF0`（set-lock・free+置換窓）・`FUN_00429990`（逐語・クランプ 99）・`FUN_004D7780`（全文・0x32→接続 REF 追跡）・`FUN_005660F0/0067D340/0067B5E0`（TESPackage slot58 群）・`FUN_00422DE0/00612220/006600F0`（vt[0x154] 三実装）・`FUN_004D66B0/004D66A0/004DDFD0/006F94A0/00700010/0055CEF0`（head 検索連鎖）・`FUN_00441970`（シングルトン生成）・[0xB0613C] map（RTTI `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@IPAVTESForm@@@@`・vtable 0xA3BC94）。**RTTI 新規デコード 4**（ExtraPackage/ExtraTresPassPackage/ExtraLock/ExtraTeleport・NiTMapBase）。
- **自己整合検査**: 項目1 の TESPackage 帰属は項目2 の家具/会話帯（0x650-0x654・0x5BE-0x5C0）と同一アクタ subsystem に整合。項目6 の 0xB0613C map は項目4 の FUN_0046B250 解決と同一 this で相互補強。項目5 の [0xB33B00] self-register は二重 store（ctor 内/呼び手側）として矛盾なく整合。
- **遵守**: git 書出なし / docs・GT・TODO・plan・session-memory・cache・ソース無編集（cache 読取のみ）/ 既存 tmp/re 無上書き（本 v268 のみ新規・作成済ヘッダへの追記のみ）/ ESM 再スキャンなし / VA→raw セクション個別換算順守（.text -0x400C00・.rdata -0x400E00・.data -0x401400・.bss 判定）/ ヒアドキュメント不使用・スクリプトファイル化（v268_1j2/1k/2h-2j/4-7.py）/ 中断対策（1 項目完了ごとに即追記）**7 回実施**（うち 1 回は前回中断分の §1-§3 回収から）。
- **自己テスト（完成直前）**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。
- **新規残点（次束候補・すべて低）**:
  1. UIMenu sender 一点特定（0x438-0x43B 帯 2 引数窓の UI 固有切り分け）＋jump table 形 8 Menu の id 表展開（項目3 続き）。
  2. extra type **0x23** の ctor store 別 encoding 検索とクラス名（推定 ExtraPackages）（項目1 続き）。
  3. ExtraLock データ +0xC..（キー連携）と ExtraTeleport データ +4 以降レイアウト（項目4 続き）。
  4. [0xB33B00] obj（0x1C4B）の RTTI/vtable（ctor に直接 store 形なし＝サブ ctor 経由）（項目5 続き）。
  5. `NiTMapBase<TESForm*>` の [0xB06144] データ配列割当窓と +4 0x0002008D の意味（項目6 続き）。
  6. [actor+0x3C] の actor 生成時書込 1 点収束・head 従属 obj([node+0xDC]) のクラス・byte[+0x1DB/0x1D5/0x1DA] 名目（項目2 続き）。
  7. bit24: TESCS.exe 実解析か UESP talk 履歴での代替通路（項目7 続き・web 不通継続中）。
