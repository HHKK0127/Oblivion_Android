# V264 — 依頼バンドル残点集約束（10件）（Oblivion.exe v1.2.0416 Steam）

- 出典: `tmp/re/v260_lod_matrix_consumer.md` §6（V-260-1..5）・`tmp/re/v262_barter_pricing.md` §未達（3件）・`tmp/re/v263_video_settings_labels.md` §6（R-263-1/2）
- TODO serial 260 / V260（Copilot 依頼バンドルの残点集約束・低）
- 手法: EXE 静的解析（capstone 5.0.7・selftest **4/4 PASS 着手時**）・既存中間物再利用（`%TEMP%\opencode\v260_*/v262_*`）・`cache/{named_funcs,vtable_index,call_targets}`・`docs/Oblivion_decompiled.c` 部分検索（突合のみ）
- クリーンルーム: 外部逆コンパイル由来コード未参照。Ghidra 台帳は既存確定项との突合にのみ使用
- 既知の確定事項（再検証禁止・足場）: 合成は `M=T(pos)·S(scale)` のみ・consumer `0x4BCEF0`→composer `0x4BAA80`・barter 式 `FUN_00548240/0x005481C0/0x00547B90`・`.data 0xB14800` テーブル
- 本束スクリプト: `%TEMP%\opencode\v264_*.py`（生 out 同所）
- 更新方式: **1件完了ごとに即追記**（中断対策）

## 判定サマリ（随時更新）

| # | 残点 | 判定 |
|---|---|---|
| 1 | [player+0x11C]（haggle%）生成元 | （追記予定） |
| 2 | FUN_00484850 stolen 係数の全量 | （追記予定） |
| 3 | NegotiateMenu 回数上限強制箇所 | （追記予定） |
| 4 | 0x812510 aux dword=grp+0x14 配列の内容 | （追記予定） |
| 5 | flush バッチ 36B/instance の内訳 | （追記予定） |
| 6 | rec 登録 0xB33A1C の二次消費者 | （追記予定） |
| 7 | ctx+0x1C/20 の保持クラス | （追記予定） |
| 8 | 0x4B9CB0/0x405070 の意味 | （追記予定） |
| 9 | §2 テーブル（.data 0xB14800）の消費コード | （追記予定） |
| 10 | 非英語ロケールラベル | （追記予定） |

---

## 件1. `[player+0x11C]`（haggle %調整値）の生成元 — **[確定]**（命令列）

### 1-1. 書込命令は .text 全域で 2 箇所のみ（player 宛）
- 関数単位アライン scan（`%TEMP%\opencode\v264_9.py`・selftest 4/4 同一環境・`cache/named_funcs.txt` 21,597 関数窓で `[reg+0x11C]` store 56 箇所を列挙。うち `this=[0xB333C4]`（player）宛は以下 2 件のみ。単一線形 disasm（v264_2）は中盤で脱同步するため不採用・関数窓方式が正）:
  1. **`0x5BD4BD`**（NegotiateMenu msg ハンドラ `0x5BD490` = vft `0xA6CF0C`+0x0C・v264_6 dump）— `mov ecx,[0xB333C4]; mov eax,[0xB3B410]; mov [ecx+0x11C],eax`。msg==6 分岐（`0x5BD494 cmp eax,6`）＝**メニュー確定イベントでグローバル鏡 [0xB3B410] を player へ複写**。同時に `push eax; call 0x597CA0`（0x5BD4BC-C3）。
  2. **`0x59A3BD`**（PersuasionMenu 側・v262 既出）— `0x59A3A0: cmp byte [edi+0x61],0 / jne 実行` のゲート下で `ecx=[edi+0x44]+0x44 → call 0x422DC0 → call 0x9828C0(ftol) → mov [ecx+0x11C],eax`。**0x422DC0 = 「subsystem record idx 0x5C の float+0xC を読み、record 不在時 0.0」（命令 `0x422DC3 push 0x5c; call 0x41E210; fld [eax+0xc]`）[確定]**。
- **add/sub 形式の `+0x11C` 書込は 0 件** — 加算は行われず、常に「出力元からの複写（SET）」2 系統のみ。

### 1-2. 値の実際の発生源 = NegotiateMenu スライダー（ハッグル成立→加算の経路の正体）
- `[0xB3B410]`（.text 出現 6 箇所・v264_4_out）:
  - 書込 ① **`0x5BD570`**（スライダハンドラ `0x5BD550`）: `ecx=[esi+0x30]; push 0xFB5; call 0x588BD0`(UI float get)`; call 0x9828C0`(ftol)`; 0x5BD565 cmp [0xB3B410],eax; je skip; mov [0xB3B410],eax; jmp 0x5BCF20`(表示更新)。→ **プレイヤーがスライダーで選んだ値そのもの**が鏡に入る。
  - 書込 ② **`0x5BD1DB`**（Update `0x5BD080`・arg1 経由）: `mov eax,[ebx]; mov [0xB3B410],eax`。ebx=arg1 は**調整値保持先へのポインタ**。実呼び出し 3 系の引数（v264b_2/3 実測）:
    - `0x5D98DE`（haggle 実行 dispatcher 0x5D9890 msg6）: `add ecx,0x11C; push ecx` → **&player+0x11C そのもの**
    - `0x59B532/0x59B552`（PersuasionMenu 0x59B44B/0x59B4E7 case 0x20/0x22）: `call 0x422DC0; call 0x9828C0; mov [esp+..],eax` した**スタック局所=&local(subsystem 0x5C ftol 値)**
    - 書込 ③ なし（0x5BCF35/0x5BCF80/0x5BD4B8 は読取のみ）
- **加算演算はエンジンに存在しない**（[確定・否定含み]）: subsystem 0x5C float+0xC への書込は `0x422D20` のみ（callers 2 = `0x427FCF`・`0x597CEE`）。0x422D20 末尾 `0x422DA3: fstp [eax+0xc]` ＝**SET であって ADD でない**（record 不在時 0x401F00(0x10) new→0x42AC00→0x41E0A0 insert）。
  - `0x597CEE` = 0x597CA0（msg6/slider 更新で [0xB3B410] を NPC（`[esi+0x44]` の 0x9832E6 解決 obj）+0x44 の subsystem 0x5C へ鏡写し。ゲート `cmp byte [esi+0x61],0`）
  - `0x427FCF` = **スクリプト系ディスパッチ窓内**（0x4534D0([0xB33B00],4,&buf) で float パラメータ取得→SET）— 対話/条件文から直接設定する経路 [確定・命令/意味は推定]
- 従って「ハッグル成否→加算」の実姿は: **成否判定 `0x5E10F0`（canHaggle・v262 式確定済み）が TRUE の間だけ UI がスライダー値を進める（進歩幅そのものは negotiate_menu.xml 側のスライダ仕様 [推定]）→ ハンドラ 0x5BD550 が [0xB3B410] へ書込 → msg6 で 0x5BD4BD が player+0x11C へ複写（＋0x597CA0 で NPC subsystem 0x5C float+0xC へ鏡）。FALSE 側（0x5D9796）は 0x52ED50(5,2) 効果音 + `-1.0([0xA30634])` フィードバック（0x57ACC0）のみで書込一切なし**。
- 余補充（v262「生成元」欄の精緻化）: Update 0x5BD080 は arg1 指す語を `[item+0x54]`（0x5BD1CD `mov ecx,[ebx]; mov [esi+0x54],ecx`）と `[item+0x50]=arg2(商人)`（0x5BD1FB）、byte `[item+0x5c]=arg4` にも複写 — haggle 実行系（0x5D9730/0x5D9890）が同じスロットレイアウトの UI item object を共有している（0x5D97F2 `mov eax,[esi+0x54]` の読み元 [推定]）。
- 参考（誤解訂正）: 成否直後の `0x5D9839-0x5D9870` は `[player+0x11C]/10` を算出し `player->vft+0x39C(0x1D, (float)(adjust/10), 0)` を呼ぶが、**vft+0x39C の実装は Actor(0xA6E074)/PlayerCharacter(0xA6FC9C) とも `0x890B90 = ret 0xC` の空stub**（v264b_4・RTTI: 0xA6E074-4 COL 0xAC308C→TD 0xB03070 `.?AVActor@@` 確定。0xA6FC9C は slot+0x224/+0x284/+0x2DC が 0xA6E074 と同一 impl 組で PlayerCharacter vft と整合）→「成否で player Mercantile 加算」は**このビルドでは無効**。

---

## 件3. NegotiateMenu 回数上限（tries）強制箇所 — **[確定]**（命令列）

- v262 §未達-3 の「カウンタ格納先未追跡」を解決:
  1. **格納**: subsystem record idx **0x1A** の `[rec+0xC]` = **object ポインタ**（`0x41E6F0: push 0x1A; call 0x41E210; mov eax,[eax+0xc]`）。その object の**有効エントリ数が残り回数**（`0x5E4420(ecx=actor) = 0x41E6F0(actor+0x44)→ jmp 0x4875C0`、0x4875C0 は obj+0x0/+0x4 の list を walk して 0x469980 判定通過数を数える [確定・命令]）。
  2. **要求値の入力**: 交渉実行 UI（menu **0x40D**・negotiate_menu.xml・0xA69B10）の message 処理 `0x5D9946`（arg≥0x3E8 群）内で `[esi+0x58] = ftol(UI trait 0xFB7)`（`0x5D99D0-0x5D99E1`、ebp=UI element）。同時に `[esi+0x4c]=ftol(0xFAA)`（0x5D99F4-0x5D9A05）、`[esi+0x54]=item ptr`（0x5D9A08）。
  3. **強制（ガード）**: `0x5D99EA: call 0x5E4420 (ecx=[0xB333C4])` → `0x5D99EF: cmp eax,[esi+0x58] / jl 0x5D9A66`。**残り<要求なら**コールバック `0x5D9730`（Try-to-haggle 実行ハンドラ。`push 0x5d9730` @0x5D9A56 で登録）を登録せず、無効側 `0x5D9A66`（コールバック NULL 登録 + `0x5D9A82/85: [esi+0x54]=[esi+0x58]=0`）へ。→ **デクリメント命令は存在せず、判定＋object list 消費 API で強制**（scan v264b_3-D: 0x5D8E00-0x5D9AC0 の `[reg+0x54]/[reg+0x58]` store は 8 箇所＝上書き/ゼロ化のみ、`dec/sub 1` 0 件）。
  4. **消費**: ハッグル確定経路 `0x5D980C-0x5D981A`: `ecx=attempts obj（0x41E6F0 返り）; (player,[esi+0x58],0) を引数に call 0x491700` — 0x491700 は obj の list 走査（0x469980 判定）経由のエントリ操作 API [確定・命令/細部はエントリ追加側分岐 0x491780 先のため一部推定]。
  5. 実行ハンドラ `0x5D9730`（v264b_3-A から 0x5D9730 開始確認）のゲート列: `[esi+0x54]≠0`（0x5D9768）・`0x578D70()==1`（0x5D9771）・**`cmp [esi+0x58],1 / jl 0x5D97EA`**（0x5D977E-82 — 残り<1 では成否ロール 0x5E10F0 を飛ばして直接確定経路へ）→ 通常は 0x5D978D `canHaggle` → TRUE のみ 0x5D97EA 確定（`player->vft+0x2DC([esi+0x54])`=0x5E0990（= 商人 obj+0x1C の list へ insert 系・v264b_2-B）→ 0x491700 消費 → menu 15 閉じ（0x5D982E push 0xf））。FALSE は 0x5D9796 効果音/表示のみ。
- 参考: v262 §未達-3 の「fBarterHaggleMax=40 の強制箇所」— 40% 側は**回数と別物**。40 は Update 0x5BD1D3 で ftol→ローカル `[esp+0x18]` と UI フィールド（0xFB7 帯 setter 0x5BD1EC-0x5BD211・[esp+0x18] は 0x5BD303 `lea eax,[esp+0x18]` で表示整形引数）に配るのみで、**エンジン側ハードクランプは無し**（スライダー max は XML 側 [推定]）。

---

## 件2. `FUN_00484850` stolen 係数の全量（分岐全パターン・定数 VA 付き）— **[確定]**（命令列 0x484850-0x484967・v264b_6）

签名: `float __thiscall FUN_00484850(item /*ecx*/, int arg1 /*[esp+0x14]=モード*/)`。区域内の分岐は**全5出口**（追加の裏分岐なし・0x48496A 以降は別関数）。

| # | 条件 | リターン | 命令 VA | 定数 VA |
|---|---|---|---|---|
| A | 盗品コンポーネント無し: `edi=0x9832E6([item+8],0, 0xB03248, 0xB05CF4, 0)==0` | **-1.0**（無効 sentinel） | 0x484873→0x48495C | [0xA30634]=-1.0 |
| B | 成立 & actor チェーン `[[item]+0]+0` が null & **arg1==0** | `(float)(count<0 ? count+2^32 : count)` = **絶対値モード**（count=`edi->vft+0x10()`） | 0x484932-0x484959 | [0xA2FC78]=4.294967296e9 |
| C | 成立 & actor null & **arg1!=0** | **100.0**（=100%） | 0x484937→0x484924 | [0xA2FE7C]=100.0 |
| D | 成立 & `v=FUN_0041E810(actor)` が **-1.0**（subsystem **0x2B** rec 不在）| arg1==0 → count の float 版（B 同型・0x4848A3 経由 0x484901-0x484921）/ arg1!=0 → **100.0**（0x4848FA→0x484924） | 0x484896 `fcomp -1.0` | 同上 |
| E | 成立 & v≠-1.0: **arg1==0** | **v 本体**（subsystem 0x2B `float[rec+0xC]` を直接） | 0x4848AF→0x484962 | — |
| F | 成立 & v≠-1.0: **arg1!=0** | **`floor_f32( v / count × 100.0 )`**（int パーセント）| 0x4848B5-0x4848EF | q[0xA309F0]=100.0(double)、0x4842F0=(0x985A00=double→double ラッパ、floor 系 [推定]) |
  - （D-F の `v` = `0x41E810(ecx=[[item]+0]+0)` = **subsystem record idx 0x2B の float+0xC、不在時 -1.0**（`push 0x2b; call 0x41E210`・v264b_7）。隣接 getter: 0x41E830=idx 0x2C byte bool、0x41E8A0=idx 0x2E float）

**価格式への組込み（FUN_00488E50 売・tier<1）— v262 訂正**:
- `0x4890F4: tier=FUN_005F23B0(0x1D)` → `0x4890F9: cmp eax,1; jge スキップ` → `0x4890FE: push 1; call FUN_00484850`（=F パスの int%）→ `0x489111-0x48911A: v<0 なら適用 skip` → **`0x48911C: fmul qword [0xA3B150]` は 0xA3B150 の qword 実体=double 0.01**（直下 4B を float 読んだ 2.0 は先頭 dword のダミダ。bytes `00000040 e17a843f`＝64bit 0.01）→ `price ×= int%(0..100)×0.01` → `0x489136: floor(FUN_00484370)`。
- **v262 §部分確定「stolen 補正 ×2.0([0xA3B150])」は誤り。正: `price = floor( price × floor(v/count×100) × 0.01 )`（v=subsystem 0x2B float, count=盗品 comp vft+0x10 個数相当）。rec 不在時は 100.0×0.01=1.0 で無変化、非盗品(-1.0)は補正自体 skip** [確定・命令]。
- 使用側 flag 一覧（直前バイト実測・v264b_8）: 0x485014(=FUN_00484F80) flag0 / 0x485316(=FUN_004852B0) flag0 / 0x488D35(=FUN_00488CB0) flag0 / **0x489102(=FUN_00488E50) flag1** / 0x4892C0・0x4893DD(=FUN_004891C0) / 0x5D0C38 flag1 / 0x5F3933 flag0（全 24 callers・v264b_6）。
- 残る [未確定]（低・移植不要）: subsystem 0x2B float+0xC の「誰の」値か（`[[item]+0]+0` の主体＝actor チェーンの正体）、コンポーネント 0xB05CF4 の型名（TD 名解決は .data ポインタ窓 0xB032xx の一次変換が同一値を返す読取系の問題で未到達。v260 の TD 手法は .rdata 基準で動作済）。v/count の意味は命令上「value 総量÷個数＝単価率%」と読むのが整合 [推定]。

---

## 件4. `0x812510` aux dword=grp+0x14 配列の内容 — **[確定]（定数性）＋[推定]（正体）**

- 追記命令（v260 §2-5 復用・再実証なし）: `0x812541 mov esi,[esp+0xC]`（arg2）→ `0x81254D mov [[grp+0x14]+cnt*4],esi`。
- **ループ内で arg2 は変化しない**（v260_composer_chain_disasm の 0x7B4010 ループ 0x7B4074-0x7B40BE で E'+0x18 スロットへの書込は 0 件・ループ内で書かれるのは float4 locals と i/edi/ebp/esi のみ）。→ **aux[cnt] は同一バースト（同一 grp・同一 flush 区間）の全インスタンスで同一値**。per-instance データではない [確定・命令]。
- arg2 の値 = 0x7B4010 の arg6 = composer `0x4BAC30 push` の `[esp+0x18]` ローカル = **`0x4BAAC4 mov eax,[esp+0x250]` → `0x4BAAD4 mov [esp+0x18],eax` で退避された composer 入力スロット（v260 §1 の引き渡し列「n」）** → **aux dword = n（このバッチのインスタンス数）[推定]**（0x7B4010 内での bp デクリメント `add ebp,0xffff`@0x7B40AE と整合）。v260 §2-5「arg2=aux u32・0x7B4010 arg6 由来」の同一性確定版。
- 対照（第2 callers・草パス）: `0x812510` の callers は 2 件だけ（`0x7B40A6` .lod / `0x7C50FA` 草@0x7C4FC0 ループ、v264b_17）。草側は arg2 に出発点ローカル（毎イテレーション更新の ftol 結果窓 0x7C50A0-0x7C50DC）→ aux は「インスタンス補助 u32 タグ」枠で、パスにより意味が違う（草=per-blade 値、.lod=定数 n）[命令確定/意味 推定]。
- 消費側の追跡（移植不要・補強）: `0x802DB0`（flush 前リセット）が aux を全 zero クリア（`0x802DCE mov [esi+eax*4],0`+entries の [8]/[C] も 0・0x802DB0-0x802DE5）→ aux は flush までに消費される一時データ。**grp 自体が TallGrassTriStrips/TriShape ノード**（RTTI 確定は件5 参照）で、flush は SetBounds/SetInstanceCount 経由でジオメトリ再構築へ渡す。aux の頂点展開先は D3D 帯（v260 範囲外宣言）— 静的に読み切れず **[未確定・低]**。

---

## 件5. flush バッチ「0x170+36n」の内訳 — **v260 §2-6 の数式表記は誤り [確定・訂正]、36B 実体は [未確定]（D3D 帯）**

- **0x802AE0 全窓（0x802AE0-0x802DA5・v264b_10 完全 dump）に `imul 0x24`/0x36/0x170 定数・アロケーションは存在しない**（帯スキャン 0x812000-0x81C000 も 0 ヒット・v264b_19-b。0x81A050 は関数開始ではなく関数中部バイト列、E8 callers 0 件・v264b_12）。→ v260 §2-6/V-260-2 の「`0x81A050(0x170+36*cap)` でバッチ確保」は**台帳側取り違え**（160命令 dump 打ち切り窓の補完時の混入と推定）。
- flush の実動作 [確定・命令]: ① entries(stride 0x10) から AABB min/max（0x802B37-0x802BE2・半径定数 [0xA9372C]）② 中心/半径算出（`f*[0xA2FAA0=0.5]`・`call 0x982C30(sqrt)`・0x802C35-0x802CD6）③ grp+0 の RTTI チェーン walk で `0xB47878` を検索（0x802CE6-0x802CFE）、無ければ `0x560920(0xB4786C, grp->vft)`（0x802D56）で型判定 ④ マッチオブジェクトへ **vft+0x9C（4 float 構造 {ebx,ebp,[x],[y]} = bounds/radius 転写：impl `0x864460` は +0x20../+0xC4.. の 2 組 doublet へ書込）** と **vft+0xA0（word cnt=[grp+0xE] 転写：impl `0x8644A0` = `mov [ecx+0xC0],ax`＝'instance count' フィールド・名文字列 0xA95924 実測）** を呼ぶ ⑤ vft+0xA0 の派生メソッド（0x8644B0/0x8644F0/0x864630）が count>0 なら `0x719B60/0x719BA0/0x717690` → **TriShape::vft+0x164（=0x8825D0）/TriStrips 系（0x71A530）へ再構築委譲**。
- 型名の確定 [確定]: descriptor **0xB47878='TallGrassTriShape' / 0xB4786C='TallGrassTriStrips'**（登録 site `0xA11FCB/0xA11FAB`=`0x70E220(ecx=&desc, 0xA95A04/'TallGrassTriShape'..)`・名前文字列 0xA95A04/0xA95940 実読。vft 0xA9595C/0xA9587C の COL→TD→name も同一 [確定]）。→ **v258/v260 の「DistantLOD グループ」は TallGrassTri* ジオメトリノード自体**（構造訂正・land LOD 共用の 0xB2C33C マップ側は変更なし）。
- 「0x170」の実体 [確定]: `0x882A85 push 0x170; call 0x401F00` → ctor `0x8823F0`（vft **0xA95ABC='HairShaderProperty'**・COL/TD 実読）→ **0x170B は「+36n」付き確保ではなく別オブジェクト（シェーダプロパティ）の固定サイズ**。`+36n` に相当する stride-36 書込は 0x802AE0/0x812xxx/0x88xxxx の静的窓で観測できず（0x8825D0 窓スキャンに imul 0x24 なし）。**36B/instance=FVF 9float 対応は実測不能 → [未確定]**（頂点ビルドは 0x8825D0→0x882700 以降の D3D 後端、v260 の「範囲外」宣言どおり残す）。

---

## 件6. rec 登録 `0xB33A1C` の二次消費者 — **[確定]（登録一義性）/ [未確定]（走査列挙）**

- **登録 site は composer のみ**：`FUN_00438670` の直接呼び出し元は **0x4BABD3（0x4BAA80 内）の 1 件だけ**（callers_text・v264b_24-H）→ rec（0x1C・{form, aux×2, key 群, n, pos 12n, scale 4n}）を 0xB33A1C に積む経路は .lod composer 一義。**ESM ビルダ 0x4D43F0・.lod パーサ 0x4D16D0 群は 0xB33A1C を参照しない**（0x4Dxxxx の ref あり=0x4D387C/0x4D5D14/0x4D646C/0x4D6727 はいずれも別関数内の他用途）→ v260 §1 の「合流点はエントリ map 側（entry ctor callers 0x4D1852/0x4D454C）」がそのまま成立 [確定]。
- **0xB33A1C 自体は汎用マネージャ**：.text の参照 **250+ 件**（0x404C6E〜0x7C4A3C の広帯域・v264b_23-B）。0x438670 はこのマネージャへの汎用 add（キー&スロット渡し）。→ rec の「二次消費者」は個別列挙不可（コンテナ全体の汎用走査に埋もれる）— **[未確定・低]**（v260 V-260-3 の「一覧化」は実効性なしと判定し打ち切り）。
- 参考（LOD 帯の 0xB33A1C 読取）：0x4AFB2A/0x4AFC2F/0x4B1563/0x4B26EB/0x4B2863/0x4B31xx/0x4B3AC6/0x4B3B15/0x4BABC1（composer）— 0x4B1xxx-0x4B3xxx 帯は STAT/TESObject 系ファイルバンドで、rec 型特化の読取は確認されず（rec は `0x7C2882/0x7C3EB9/0x7C4A3C` のローダ帯で一括扱いの可能性 [推定]・未追試）。

---

## 件7. `ctx+0x1C/+0x20` の保持クラス — **[部分確定]（構造・setter・生成元）/ [未確定]（型名）**

- factory `0x4BD430` の呼び出し元は **0x4841A8 の 1 件だけ**（callers_text・v264b_24-J）。呼び出し側は **0x483E00 群 = scene mgr 帯の distant ブロック走査ループ**（`[0xB34418]+([0xB06A2C]>>1)·…` の **stride 0x10 ブロックテーブル走査**・`shl eax,4`@0x483EB3、[esi]/[esi+4]/[esi+0x18] 操作、v264b_26-b）。
- 引き渡し（命令確定・v264b_23-C + v264b_26-b）：`0x484194-0x4841A7` の push 列 = (ebp=mode, edi=エッジフラグ 0/1（0x484140-0x484164 で算出）、edx=[ebx]（ebx=**[0xB06A90] グローバル**の先頭語 @0x483EAB→0x48418A `mov edx,[ebx]`）、eax=**[0xB34424]（scene mgr singleton）**、ecx/y、edx/x、ローカル）→ **factory 側で ctx+0x20 ← arg5=[ebx]=[[0xB06A90]]、ctx+0x1C ← arg6**（`0x4BD504-0x4BD51F: push [esp+0x14]; lea ecx,[esi+0x20]; call 0x75FA90`、`0x4BD524-0x4BD52C: push [esp+0x18]; lea ecx,[esi+0x1C]; call 0x75FA90`）。ctx+8 = arg4 = [0xB34424] scene mgr 自身（ws mgr スロット確定・v260 §0-4 と一致）。
- **0x75FA90 = intrusive_ptr 追加セッタ**（addref/release 書換・v260 §0-4 既出）。同じ setter が `[0xB34424]` グローバル自体にも使われている（`0x483E42 mov ecx,0xB34424; call 0x75FA90`）→ ctx+0x1C/20 も同一 refcount ライフのオブジェクト。
- 使い方で分かる形状 [確定・命令]：consumer 末尾 `0x4BD00D` は **`[ctx+0x20]+4` を読み「型値>2」で分岐 → `mgr[0xB34424]->vft+0x84(1, obj)` 通知**；composer 0x4BAA80 は arg7/8 として 0x7B4010 へ再送。→ **obj+4 が u32 型タグのNi系リソース/要求オブジェクト**（[0xB06A90] グローバル配列の要素）[推定]。
- 型名の最終確定は [未確定]（obj は実行時グローバル生成物で vft 静的特定に追加追跡が必要・低優先として打ち切り）。v260 §0-4 の [部分確定] から前進：**生成 site・引数来源・refcount 機構まで確定**。

---

## 件8. `0x4B9CB0` / `0x405070` の意味 — **[確定]**

- **`0x4B9CB0(form)` = float フィルドゲート**（v264b_23-D1・thiscall ecx=form・ret bool(al)）:
  `0.0 と form+0x78` を比較 → 0 でない側は **form+0x78 vs 200.0(q[0xA37478])**、通れば `0.0 と form+0x7C`、0 でない側は **form+0x7C vs 350.0([0xA44F64])** — 帰結式：**`(form+0x78==0 || form+0x78<200.0) && (form+0x7C==0 || form+0x7C>350.0)` で真**（NaN/順序無視も真側）。意味：MODL/FNAM 系 float（+0x78/+0x7C）の異常値（0..200 のフェード値・350 超 sentinel）判定 = 「この STAT は distant LOD 対象として妥当か」[推定]。v260 §2-1「0x4B9CB0(form) bool ゲート」の解読完了。
- **`0x405070 = intrusive_ptr 代入（AddRef 書込セッタ）**（v264b_23-D2）: `mov eax,[esp+4]; mov ecx=this-slot; mov [slot],eax; if(eax) InterlockedIncrement(eax+4)（`call [0xA28078]`=CRT 原子的加算 import）; ret 4` — composer では `lea ecx,[esp+0x18]（ロード要求ハンドル slot）; push eax(要求 obj); call 0x405070`（0x4BAB64）→ 「ハンドル slot への addref 書込」確定。v260 §2-1 の「(0x405070/0xB35300 系統のロード要求ハンドル)」の 0x405070 側役割 = セッタ [確定]。

---

## 件9. §2 テーブル（.data 0xB14800 12 エントリ）の消費コード — **[確定]（消費機構）＋ [推定]（Actor Min/Max の解釈）**

- 消費コードを特定：**12 個のサフィックス thunk（0xA24EA0 先頭・stride 0x30・最終 0xA250C0=0xB14858 対応）** — 各 thunk：
  `push &slot0xB14800(+8i); mov ecx,0xB07CFC; call 0x65C620;` → `mov eax,[slot]; if (eax && *eax=='S'(0x53)) free(eax); ret`（v264b_25-(item9)/v264b_26-(c)）。
  `0x65C620` は **片方向リスト unlink（ノード比較→書継ぎ→free 0x401F20）**（v264b_26-a・命令列確定）。
- よって **0xB07CFC singleton（INISetting クラス群）の登録/解除機構側のキー登録テーブル**が正体：スロット 8B = **{キー名文字列 ptr, 値}**（.data ファイル画像の静的初期値＝v263 §2 の {nameptr, float} 12 対は**そのままキー名+既定値**）。.text imm32 直接参照 0 件（v263 §1）→ データ駆動（0x65C620/登録系が ptr 経由）と整合。**スライダー min/max バインドではなく INI（GMST 形式キー）機構** [確定・命令]。
- Actor `Min15>Max2`：12 行の「値」列は既定値像で **Min/Max はキー名の一部（clamp 対ではない）**。よって 15>2 の不整合はエンジンの制約違反にならない（読み手＝INI 系が名前を解釈しない）[確定・命令範囲]。ランタイム更新の有無（v263 §2 の「値スロットが実行時更新される可能性」）は**解除 site のみ確認・書込 site は特定せず**（低・残し）。

---

## 件10. 非英語ロケールラベル（R-263-2）— **[未確定]（確認まで・想定どおり）**

- 実環境の抽出物ツリーを走査：`D:\Cargo\Other\Oblivion\BSA\bsa_Extraction` 直下は `{distantlod, facegen, fonts, menus, meshes, sound}` のみ。**翻訳 TAF・Localization/Translated・非英語 strings.xml の類は再帰走査で 0 件**（ワイルドカード `*.taf` 0・パス名 localiz/translat/各国語名 ヒット 0・2026-11-03 実測）。
- v263 §0-3 の英語ラベル確定（menus\strings.xml trait 直読）に対して、非英語版は**この環境に一次資料が存在せず解析不能**。判定 **[未確定]**（本件は「抽出物なしなら [未確定] 確認のみ」の依頼どおり、ここで打ち切り）。

---

## 判定サマリ（最終）

| # | 残点 | 判定 |
|---|---|---|
| 1 | [player+0x11C]（haggle%）生成元 | **[確定]** 書込 2 箇所のみ（0x5BD4BD / 0x59A3BD）、加算は UI スライダー→[0xB3B410]→複写。エンジン内 += 無し |
| 2 | FUN_00484850 stolen 係数の全量 | **[確定]** 分岐 5 出口・flag0=絶対値/flag1=int%、**×2.0 誤り→×0.01（q[0xA3B150]）訂正** |
| 3 | NegotiateMenu 回数上限強制箇所 | **[確定]** subsystem 0x1A obj の count（0x5E4420→0x4875C0）と要求 0xFB7 の比較 0x5D99EF。dec 命令なし・0x491700 経由 |
| 4 | 0x812510 aux dword=grp+0x14 配列 | **[確定+推定]** per-instance ではない（グループ定数）=.lod 経路は n [推定]。消費先は D3D 帯 [未確定・低] |
| 5 | flush バッチ 36B/instance の内訳 | **[確定・訂正]** 0x802AE0 に確保/36B 書込なし。0x170=HairShaderProperty 系 obj 固定サイズ。36B 式自体は裏付不能 [未確定]（v260 §2-6 訂正） |
| 6 | rec 登録 0xB33A1C の二次消費者 | **[確定]** 登録は composer 一義（0x438670 callers=1）。0xB33A1C は汎用 mgr（250+ refs）で rec 特化消費者は列挙不能 [未確定・低] |
| 7 | ctx+0x1C/20 の保持クラス | **[部分確定]** factory 0x4BD430 callers=1（0x4841A8・scene mgr ブロック走査）で arg5/arg6 を 0x75FA90 addref 保持。+0x20=[0xB06A90] 由来の型タグ語 obj [推定]、型名 [未確定] |
| 8 | 0x4B9CB0/0x405070 の意味 | **[確定]** 0x4B9CB0=(+0x78∈{0,<200})&&( +0x7C∈{0,>350}) の LOD 妥当性ゲート、0x405070=intrusive_ptr 代入（InterlockedIncrement@0xA28078） |
| 9 | §2 テーブル（0xB14800）の消費コード | **[確定]** 12 thunk（0xA24EA0..0xA250C0・stride 0x30）→ 0xB07CFC INI 機構×0x65C620(unlink)。キー名+既定値テーブルと確定。**Actor Min15>Max2 は clamp 対でなく既定値像** |
| 10 | 非英語ロケールラベル | **[未確定]（確認）** 環境に翻訳 TAF/localized 抽出物 0 件 |

### 実装セッションへの影響がある発見（優先）
1. **v262 訂正（価格式）**: tier<1 の売価 stolen 補正は `price *= floor(stolen%)*0.01`（**×2.0 ではない**）。`[0xA3B150]` の qword 実体=double 0.01（bytes 00000040e17a843f）。drft の「2.0」系コメント/定数は要差替。
2. **v260 §2-6 訂正（LOD flush）**: 「0x81A050(0x170+36*cap) 確保」は誤り。バッチ確保は vft+0x9C(SetBounds 4float)/vft+0xA0(SetInstanceCount word) 経由の TallGrassTriShape/TriStrips（RTTI 名確定・0xB47878/0xB4786C）再構築へ委譲。**Android 側は 16B {x,y,z,s} entries だけ見ればよい**設計不変。
3. **v258/v260 構造補強**: DistantLOD「グループ」= TallGrassTri* ノード実体。aux[grp+0x14] は per-instance でなくグループ定数（移植では読まなくてよい）。
4. 売買調整 `[player+0x11C]` の+= は**エンジンに存在しない**（UI→グローバル鏡→複写）。haggle 成否は判定のみで増減なし。回数制限は subsystem 0x1A の count オブジェクト。
5. fBarterHaggleMax=40 は**ハード強制なし**（UI スライダー max への配付のみ）。cap40% 相当は NegotiateMenu の表示 clamp（buy%≥100/sell%≤100）とセットで UI 仕様。
