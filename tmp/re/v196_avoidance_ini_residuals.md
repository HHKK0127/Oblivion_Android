# v196_avoidance_ini_residuals.md — 回避/INI消費束（status 1/2 書手・速度最終 store・bDisableAvoidance 読手）

- 作成日: 2026-10-29 / 担当: TODO serial 183 (V183)
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / 既存 tmp/re / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は一切変更していない（読取のみ）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000, 7,898,624 B)
- 出典（読取専用）: `tmp/re/v192_havok_magic_residuals.md` §2-R5/R7/R10・§8-2 / `tmp/re/v186_havok_ray_pathing.md` §2.4 / `GT_CHEATSHEET.md` / `cache/*`
- 中間スクリプト: `%TEMP%\opencode/v196_*.py`
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造は判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

---

## 1. 手法 / 入力

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- 解析: capstone 5 detail 付き Cs によるオペランド disp 走査（レジスタ源 `mov [reg+disp], reg` 型・即値 `C6/C7` 型の全区分）＋ `%TEMP%\opencode/v192b_*.py` の再利用。
- 参照数は `tmp/re/cache/call_targets.json`、ラベル境界は `cache/named_funcs.txt` を引く（再走査しない）。

（以降、判明するごとに追記）

---

## 2. タスク1: 回避 FSM status 1/2/6 の書手 — [確定(不在)]

### 2.1 ctor 引数経由説の否定（v192 訂正）

- **FUN_00680DC0 の `[eax+0xC]=cl` は引数ではない**（v192 R5「初期値は引数」は**誤り**）:
  0x680DE1 `mov eax,ecx` → 0x680DE3 **`xor ecx,ecx`** → 0x680DE5..0x680E0A で全フィールドをゼロ初期化する standard ctor。0x680DEC `mov byte [eax+0xC],cl` の cl は直前の `xor ecx,ecx` 由来＝**status 初期値は常に 0（NO AVOIDANCE）**。引数渡しもなし（0x6847E5-FC: `mov ecx,eax; call 0x680DC0`、push なし）[確定]。

### 2.2 FSM ドライバ経路の確定

- getter 0x680CB0 の callers = **11 件**（cache 一致）: 0x408859（プリンタ）/ FUN_00683B90 / FUN_00683DB0 / FUN_00684780（active 述語）/ FUN_00685880 / **FUN_00688120 ×5**。
- **遷移ドライバ**: FUN_00688120（PathingSystem メソッド）内 0x6888D9 `call 0x681DF0` → FUN_00681DF0 内 0x68213D `call 0x681050`（`mov ecx,ebp; push edi`）。つまり **Update(0x688120) → 0x681DF0（リセット/失敗系: status→0/7）→ 0x681050（Angling/Waiting/FAILED: status→3/4/5/7）** の2段呼び出し [確定]。

### 2.3 status 1/2/6 書手 = バイナリ全体に不在（網羅走査）

| 走査 | 範囲 | 結果 |
|---|---|---|
| C6 バイト即値 [reg+0xC]=imm∈{1,2,6} | .text 全域（esp 除く） | **22 件すべて回避帯外**（formType 0x13 ctor 0x429D41 等。回避オブジェクト無関係） |
| C7 ドワード/66C7 ワード即値 [reg+0xC]=imm∈{1,2,6} | .text 全域 | 帯内 **0 件** |
| 88 レジスタ源 byte ストア [reg+0xC],reg | .text 全域（esp 除く） | 帯内（0x67C000..0x6A2000）は **ctor 0x680DEC の 1 件のみ** |
| 別エンコード（80 グループ演算 / FE inc-dec / 8F pop / F7 not-neg） | 0x660000..0x6B0000 | 帯内の [reg+0xC] 命令は **FUN_00680340/00680500/00680570/006806B0 のみ＝別オブジェクト**（handle 索引 → 配列 0xB3BF00 + idx*0x10 の +0xC フラグ byte、bit0/1/2 set/clear。回避オブジェクトと無関係） |
| FUN_00681050/FUN_00681DF0 全内部の disp 0x0C 触碰 | 両関数 | 書き込みは **{7,5,5,3,4,3}（0x681050）と {0,0,7,0,0,7}（0x681DF0）の C6 即値のみ** |
| +0x0D 経由の間接書手 | .text 全域 byte ストア | 帯内は ctor ゼロ＋FUN_00681DF0 内 6 件（status の一時コピー: 0x682077/0x6820D5/0x68214C/0x682401 は [ebp+0xC]→[ebp+0xD] 転写、0x682097/0x6820F1 は imm 0）。**[reg+0xD] の読み（source オペランド）は 0x67C000..0x6A2000 で 0 件** ＝ write-only [確定] |

**帰結**: status **1(Checking)/2(Avoiding)/6(Forcing New Path) を書くコードは本 EXE に存在しない**。到達可能な遷移は ctor(0) → FUN_00681DF0({0,7}) と FUN_00681050({3,4,5,7}) のみ。0xB15728 の 8 状態名のうち 3 状態は**プリンタ用の死に名前**（アクティブ述語 0x684780 は status ∉ {0,7} なので実運用では 3/4/5 のみが「アクティブ」）[確定]。

（後続: タスク2/3 は下に追記）

---

## 3. タスク2: 速度 XYZ 最終 store — [確定(訂正): 速度 store は存在しない]

### 3.1 FUN_00688120（0x688569）: レシーバ = PathingSystem [確定（再証）]・書くのはノード位置

- 0x688552 `call 0x68CA20`（bool ゲート）が真のとき 0x688562 `call 0x6899C0`（ノード位置 getter、戻り=pos ポインタ）→ 0x688567-75 `mov ecx,[eax]; [esi+0x3C]=ecx; mov edx,[eax+4]; [esi+0x40]=edx; …[esi+0x44]`＝**3 ワード位置コピー**（v192 §8-2 の実測を本タスクで再確認）。esi = PathingSystem（内部に `lea ebx,[esi+0x14]` ＋ `call 0x68C170/0x42B410` のアクセサ使用、v192 §8-2）。
- これで「PathingSystem[+0x3C..+0x44] = 速度」説（s72 M7）は**再度否定**された。速度ではなく**追従中ノードの位置 XYZ**。

### 3.2 FUN_00682EF0（0x682FE5）: レシーバは PathingSystem ではない [確定(偽候補)]

- 0x682FD7 `[esi+0x30]=0xA74B78`（**vtable 書込**）→ 0x682FDE `[esi+0x34]=0x25`（count）→ 0x682FE5 `[esi+0x3C]=edi` → malloc(0x25*4) → 0x682FFC `[esi+0x38]=eax`（バッファ）→ `call 0x981630`（memcpy: dest=buffer, src=edi, size=count*4）。
- = **埋め込み配列コンテナの ctor**（+0x30=vtable / +0x34=count / +0x38=buffer / +0x3C=src ptr）。PathingSystem の +0x30 は回避オブジェクト ptr なので矛盾＝**別クラス** [確定]。

### 3.3 FUN_0068CD90 / FUN_0068CEA0: 非 RTTI クラス vtable 0xA75044 の ctor 群 [確定(偽候補)]

- 0x68CDD3 / 0x68CECC で `[esi]=0xA75044`（vtable 書込）。0x68CDDD-E9 `[esi+0x3C..+0x48]=0` は**ポインタのゼロ初期化**。
- **[+0x3C] は参照カウント付き NiPointer**: 0x68D34D `mov edi,[esi+0x3C]` → 0x68D354 `lea eax,[edi+4]` → Interlocked 呼出し（addref）→ 失敗時 0x68D36C で差し替え。float ではない [確定]。
- vtable 0xA74B78 / 0xA75044 / 0xA747BC は `vtable_index.tsv`（RTTI 1,715 本）に**行なし**＝non-RTTI 系。

### 3.4 PathingSystem[+0x3C..+0x44] 書手の全数裁定

| site | 実体 |
|---|---|
| 0x684F1F-2D（FUN_00684EC0 Init） | ゼロ化 |
| 0x688569-75（FUN_00688120） | **ノード位置コピー**（3 ワード） |
| 0x682FE5 / 0x68CDDD / 0x68CE34 / 0x68CF75 / 0x68D36C | **別クラス**（コンテナ ctor / 0xA75044 ctor・dtor）＝PathingSystem ではない |

**＝「[+0x3C..+0x44] へ速度を書く最終 store は本 EXE に存在しない」**。消費側は 0x6849F0 が `&PathingSystem[+0x3C]` を 0x4F1A80（NiNode 位置照合）へ渡す位置利用のみ（v192 §8-2）。

### 3.5 v192 §8-2 の「move オブジェクト = 速度本体」仮説への反証 [確定(反証)]

vtable 0xA747BC オブジェクト（ctor 0x67D300 / 0x67D3A0）の中身と消費者を実測:

- **+0x3C = f32 タイマー**: FUN_0062A0E0@0x62A2BB `fld [ebp+0x3C]; fadd [0xB33E9C]; fstp [ebp+0x3C]`（全局値の加算）/ FUN_0062FBD0@0x62FFFF `call 0x67D330; fld [edi+0x3C]; fadd qword[0xA2FC68]; fstp [edi+0x3C]`。
- **+0x40 = 整数カウンタ**: 0x67D330 = `add dword [ecx+0x40], eax; ret 4`（0x62A2CB で +1 / 0x62FFFA で +0 の 2 caller のみ）。
- **+0x44/+0x48 = NiObject ptr**: dtor 0x67D340 が 0x46B250 で解放。+0x4C=-1 / +0x50=1。
- ctor caller: 0x67D300 = FUN_00463EC0（placement 構築）、0x67D3A0 = FUN_006411E0（Process 帯）。
- slot[15] = 0x68F970 = **`ret 4` 空スタブ**（81 caller の共有 nop）。

→ これは**速度 XYZ ではなく移動経過記録（タイマー＋ステップカウント）**。s72 M7 由来の「速度本体」置き場所候補も閉じた。

---

## 4. タスク3: bDisableAvoidance（node 0xB15748）の値読手 — [確定(不在): retail 死に設定]

### 4.1 INI 設定システムの構造（本タスクで実測）

- **登録**: `0x4040E0` = this=レジストリ 0xB07BF0、arg=ノード ptr。内部で既存ノード名と `strcmp`（0x98262D）し重複なら警告（push 0xA2FAB0 → call 0x4A7A60）、なければリスト（[registry+0x10C]）へ `call 0x446CB0`（generic list PUSH）で追加。`ret 4`。
- **bDisableAvoidance 登録**: 0x9FE6F0（push 0xB15748 → 0x4040E0）→ `push 0xA25DD0; call 0x981FB4`（**atexit 登録**）。
- **クリーンアップ thunk 0xA25DD0**: `push 0xB15748; mov ecx,0xB07CFC; call 0x65C620`（**Remove**）→ 名前先頭 'S' チェック（0x9C4CB0 経路と同型）で name 文字列 free。
- **INI パーサ本体**: FUN_0040D4D0（0x40E710-: バッファ **0xB07BF4** へ逐次コピー → レジストリ vfunc [vtable+0x14/+0x18/+0x20] 呼出し）。
- **コンソール動的入口**: SetINISetting 0x5143C0 / **GetINISetting 0x514230**（名前指定の汎用 reader）/ RefreshINI 0x513BB0 / SaveIniFiles 0x513B60（opcode_table 実測）。

### 4.2 値読手の不存在判定（対称証拠つき）

1. **[0xB15748] abs32 refs = 3 件のみ**（0x9FE6F0 push / 0x9C4CB0 mov ecx / 0xA25DD0 push）＝直接 reader 0 件（v192 と再一致、v196_12 再測定）。
2. **name 文字列 0xA74B2C refs = 1 件**（ノード自身の name フィールド 0xB1574C）＝名前経由 lookup も不在。
3. **対称証拠（本タスクの新事実）**: 兄弟 **bDebugAvoidance:Pathfinding（node 0xB15750, name 0xA74B4C）は FUN_00681DF0 内で直接読まれる** — 0x681E63 `cmp byte [0xB15750],bl; je`（デバッグ出力ゲート、push 0xA74B20 → vfunc[+0x58]）と 0x682001 `cmp byte [0xB15750],0; je`（0x680F60 デバッグダンプ呼出し）。refs = 5（読手 2 + 登録 0x9FE730 + cleanup 0xA25E00 + thunk）。
   → **同一サブシステム・隣接ノードで「ノード直接読手」パターンが実在する**のに、bDisableAvoidance には一切ない。
4. **bHavokPick 兄弟コピーの対応物を 0x441A 帯で特定**: FUN_00441980 内 0x441A0F-0x441A83 が HAVOK セクション兄弟ノードを一括でグローバルへコピーする: 0xB0520C(iUpdateType)→0xBA7918 / 0xB05214(fMoveLimitMass)→0xB2E764 / 0xB0521C(bAddBiped)→0xBA7909 / 0xB05224→0xB2E2F4 / 0xB0522C→0xB2E2F8 / 0xB05234→0xB2E2F0。**bHavokPick(0xB05204) 自身も bDisableAvoidance もこの帯に登場しない**（v186 の「bHavokPick 死に設定」判定と整合）。

**帰結**: bDisableAvoidance は **INI ロードでは [0xB15748] に値が書き込まれるが、実行時の消費者が 1 つもない retail 死に設定**（bHavokPick と同類）。動的経路はコンソール GetINISetting のみ。回避 FSM はこの INI 値に依存せず稼働する。

### 4.3 付随発見: 回避デバッグ描画トグル [0xB3C08A]

- FUN_00685FC0（PathingSystem 構築: `[esi+0x30]=eax; call 0x684EC0(Init)`）が 0x686010-1D で [0xB1581C] 系設定から [0xB3C08A] を設定。
- FUN_00683BE0 = `cmp [0xB3C08A],0; sete al; mov [0xB3C08A],al`（トグル setter）。
- FUN_006862C0@0x6862E5 = Update 内チェック → 真なら `push 0; mov ecx,esi; call 0x685EA0`（デバッグ経路）。refs 計 9 件。

---

## 5. 結論（3行）

1. **status 1(Checking)/2(Avoiding)/6(Forcing New Path) の書手は本 EXE に存在しない**（全エンコード種を網羅: C6/C7/66C7 即値・88 レジスタ源・80 グループ・FE/8F/F7 別形すべてで帯内 0 件、ctor は引数ではなく `xor ecx,ecx` ゼロ初期化＝v192「初期値は引数」を訂正）。到達遷移は ctor(0) → FUN_00681DF0({0,7}) → FUN_00681050({3,4,5,7}) のみで、8 状態名前表の 3 状態はプリンタ用の死に名前。
2. **「PathingSystem[+0x3C..+0x44] に速度を書く store は存在しない」**: FUN_00682EF0/FUN_0068CD90 の候補は埋め込み配列コンテナ ctor（vtable 0xA74B78）と非 RTTI クラス 0xA75044 の ctor 群（+0x3C=NiPointer）で偽候補と裁定、残る実 store は Init ゼロ化と FUN_00688120 の**ノード位置コピー**のみ。move オブジェクト 0xA747BC も速度ではなく f32 タイマー＋整数カウンタ＋NiPointer で、v192 §8-2 の速度本体仮説も反証された。
3. **bDisableAvoidance は retail 死に設定と確定**: 参照は登録/クリーンアップの 3 件のみ・名前経由も 0 件。兄弟 bDebugAvoidance（0xB15750）が FUN_00681DF0 内 0x681E63/0x682001 で直接読まれる対称証拠と、0x441A0F-4A83 の兄弟ノード→グローバル コピー帯（iUpdateType 等 6 ノード）に本設定が不在なことで、読手の不在が閉じた。

## 6. 残留不明点

1. 付随: 0xB1581C（回避デバッグ描画トグル 0xB3C08A の元になる設定候補）の正式名は未取得（[Pathfinding] セクション系と推定）。本タスクの3スコープ自体は全て裁定済み。

## 7. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回、同一結果: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- 内部整合（v196_12 再測定と既存記載の一致）:
  1. C6 バイト即値 {1,2,6} → [reg+0x0C] 全 .text = **22 件・帯内 0 件**（v196_1 と一致）。
  2. 帯内 byte レジスタ源ストア = ctor 0x680DEC ＋ [esp+0xC] ローカル 2 件（FUN_0068E210）＝非スタック 1 件（v196_1 の 1b 再解釈。**罠**: capstone x86 のレジスタ ID は esp≠4 のため `base==4` フィルタは BH に一致する。本件は全件をラベルで再分類して確定）。
  3. `call 0x680CB0`（getter）= **11 件**（cache と一致）。
  4. 0xB15748 refs = **3 件** / 0xB15750 refs = **5 件** / name 0xA74B2C refs = **1 件**（v192 §8-1 と一致・拡張）。
  5. bHavokPick node 0xB05204 refs = 3 件（登録/cleanup thunk 系）＝v186「死に設定」と完全一致。
  6. FSM 呼び出し構造: Update FUN_00688120 → 0x681DF0（1 caller）→ 0x681050（1 caller、0x68213D）が v192 の遷移表 {0,3,4,5,7} と矛盾なし。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**。既存 tmp/re 上書き 0 件。ESM 読取 0 回。中間物は `%TEMP%\opencode/v196_1..12_*` のみ。

## 8. Android 移植への示唆（実装しない）

- **回避 FSM の移植では 8 状態を実装しない**: 到達可能な status は {0,3,4,5,7}（＋ctor の 0 初期化）のみ。Checking(1)/Avoiding(2)/ForcingNewPath(6) は UI 文字列としてのみ存在した旧状態と見なし、状態機械に空分岐を書かないこと。遷移ロジックは FUN_00681DF0（0/7 系: リセット/失敗）と FUN_00681050（3/4/5/7 系: Angling/Waiting/FAILED）の2層構造をそのまま移植する。
- **PathingSystem[+0x3C..+0x44] は「追従ノード位置」フィールド**として実装する（速度出力ではない）。速度はコンシューマ側で位置差分から算出する設計（Actor slot[218] 0x5FAAE0 帯のクランプと接続）。
- **bDisableAvoidance / bDebugAvoidance の扱い**: 前者は移植不要の死に設定。後者（直接 reader あり）だけをデバッグ描画フラグとして再現すれば、retail 挙動と一致する。デバッグ描画トグルの実体は [0xB3C08A] ＋ FUN_006862C0 の分岐（0x685EA0 呼出し）。
- **INI システム移植**: ノード={value, name ptr} をレジストリ（重複名 strcmp 検査付きリスト）へ登録し、ローダがファイルから value を書く、という構造だけ再現すれば足りる。コンソール GetINISetting 相当があれば死に設定も「読める」互換性が取れる。
