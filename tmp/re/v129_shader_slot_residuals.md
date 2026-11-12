# V129: シェーダ/レンダラ束の未特定 slot 束（child 28クラス・+0x5E0 flags・[state+0xFF8]）

- 作成日: 2026-10-26 / 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam / 7,898,624 B / SHA256 `A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`)
- 出典: `tmp/re/v92_shader_renderer_bundle.md` §a-5 / §b-7 / §c-5、`tmp/re/TODO_re49.md` serial 129（スコープ8点、追記(8)含む）
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時実行）
- 手法: probe_harness `Ctx.rd()` / `Ctx.cstr()`（.text const=0x400C00 / .rdata=0x400E00 / .data=0x401400、bss は None+警告）+ capstone 5.0.7（skipdata/md.detail）+ E8/E9 rel32 全数走査 + raw imm32 全数走査 + Ghidra 台帳 `tmp/re/cache/named_funcs.txt` (21,597行) 突合 + `docs/Oblivion_decompiled.c`（1,066,496行、`enumerate(f,1)` 部分検索）+ 共有キャッシュ `tmp/re/cache/{call_targets,vtable_index,opcode_table}.*`
- 中間スクリプト（すべて `%TEMP%\opencode`、接頭辞 `v129_`）: 既存再利用 `v129_01_bodies / v129_02_factories / v129_03_factdis / v129_04_gates / v129_05_struct / v129_06_scan / v129_extract / v129_a_factories / v129_a2_factories / v129_a3_ctor / v129_a4_ctorclass / v129_b_scan / v129_c_dump / v129_d_probe / v129_e_probe / v129_f_caller / v129_g_ident`（先行セッション分）+ 本セッション `v129_g_verify / v129_h_probe / v129_i_probe / v129_j_probe / v129_k_probe`
- 成果物は **本ノート 1 ファイルのみ**。docs/・既存 tmp/re・TODO・台帳・git・Supermemory・sync は無変更

判定表記: **[確定]** = バイナリ実測で一意 / **[仮説]** = 構造証拠十分で名칭・意味論が推定 / **[未特定]** = 残点。

---

## §1 手法 / 入力

（後述の各節の計測は、すべて本ハーネス 4/4 PASS 下で実施。地址引用は §4 規約に従い probe で裏取り）

<!--APPEND-->

---

## §2 (1) FUN_007B4290 factory → 戻りオブジェクトクラス対応表（28 種）— **[確定]**

### 2-1. 実測手法と整合確認

- switch 本体 0x7B4290..0x7B44A8 を全文ディスセンブル（v129_n_probe N1a-c）。ジャンプテーブル 0x7B44A8（27 エントリ、index = arg0-1）を raw 展開（v129_o_probe O1）し、各ボディの先頭 `call factory` を逆引き。
- クラス名は 2 系の独立手法で同定し一致: ①factory→big_malloc→ctor→ctor 内 vtable 代入の追跡（v129_a3_ctor / v129_a4_ctorclass、RTTI COL 照合）、②factory 内 malloc シーケンス→ctor→vt-store（v129_03_factdis）。
- 補足: v129_a_factories の `[d0]` ヒット（例: case 2 で BlurShader_P20、case 12 で GeometryDecalShader）は「factory が**呼出す ctor 内**の vtable 書き手」を depth-0 に帰属させた誤検であり、①②の ctor 連鎖判定を正とする（case 1 の ShadowLightShader は同スクリプト自身の depth-1 ヒットでも一致）。

### 2-2. 対応表（slot = b42ec0+i、arg0 = i）

| arg0 | gate | factory | ctor | vtable(COL) | 戻りクラス [確定] |
|---|---|---|---|---|---|
| 0 | —（**factory なし**） | — | — | — | **常に NULL**（§6 参照） |
| 1 | 無条件 | 0x7CBDC0 | 0x7C88B0 | 0xA90634 | ShadowLightShader |
| 2 | 無条件 | 0x7E9840 | 0x7E8BB0 | 0xA9202C | TallGrassShader |
| 3 | 無条件 | 0x8122A0 | 0x811C20 | 0xA946B4 | DistantLODShader |
| 4 | 無条件（**arg0=1 を自再帰で先行確保** @0x7B42EA `push 1; call 0x7b4290`） | 0x810B90 | 0x810AD0 | 0xA9459C | SpeedTreeBranchShader |
| 5 | 無条件 | 0x80EE10 | 0x80E8C0 | 0xA9443C | SpeedTreeFrondShader |
| 6 | 無条件 | 0x7F1410 | 0x7F1200 | 0xA92844 | SpeedTreeLeafShader |
| 7 | `[0xB42F48] >= 2` ? | 0x7EAE80 (ctor 0x7EAB50, vt 0xA9224C BlurShader_P20) : 0x7B1070 (ctor 0x7B0BD0, vt 0xA8CE1C BlurShader) | | | BlurShader / BlurShader_P20（二系統・NULL 化なし） |
| 8 | `[0xB43070] != 0` ? | 0x7C0B80 : **clear path** | 0x7C0330 | 0xA8FFAC | HDRShader |
| 9 | 無条件 | 0x7FA910 | 0x7FA730 | 0xA92FCC | DebugShader |
| 10 | 無条件 | 0x7BD920 | 0x7BA440 | 0xA8F654 | SkyShader |
| 11 | `[0xB42F48] >= 2` ? | 0x7EC950 : **clear path** | 0x7EC7D0 | 0xA92394 | GethitShader |
| 12 | 無条件 | 0x804C50 | 0x804790 | 0xA93E14 | CopyShader |
| 13 | 無条件 | 0x80D940 | 0x80CF20 | 0xA942A4 | HairShader |
| 14 | 無条件 | 0x80AE90 | 0x80AD40 | 0xA941AC | SkinShader |
| 15 | 無条件 | 0x8091D0 | 0x809080 | 0xA9405C | ParallaxShader |
| 16 | 無条件 | 0x806210 | 0x805D90 | 0xA93F2C | GeometryDecalShader |
| 17 | 無条件 | 0x7DD1E0 | 0x7DC9A0 | 0xA9170C | WaterShader |
| 18 | 無条件 | 0x7F59C0 | 0x7F5780 | 0xA92EB4 | NighteyeShader |
| 19 | 無条件 | 0x7E1430 | 0x7E0ED0 | 0xA91B74 | WaterShaderHeightMap |
| 20 | 無条件 | 0x7DE710 | 0x7DE240 | 0xA9193C | WaterShaderDisplacement |
| 21 | `[0xB42F48] >= 2` ? | 0x800E20 : **clear path** | 0x800B70 | 0xA9336C | RefractionShader |
| 22 | 無条件 | 0x7E33A0 | 0x7E2D90 | 0xA91E4C | ParticleShader |
| 23 | 無条件 | 0x7F3E00 | 0x7F3890 | 0xA92D8C | BoltShader |
| 24 | `[0xB42F48] >= 2` ? | 0x7AFBB0 : **clear path** | 0x7AF820 | 0xA8CCDC | MapShader |
| 25 | 無条件 | 0x7B1F20 | 0x7B1C70 | 0xA8CFD4 | MenuBGShader |
| 26 | `[0xB42EA5] != 0` ? | 0x7FC7D0 : **clear path** | 0x7FAC40 | 0xA930C4 | Lighting30Shader |
| 27 | 無条件 | 0x7EEE40 | 0x7EE800 | 0xA925F4 | PrecipitationShader |

### 2-3. 補足事実 [確定]

- 全 factory が共通の **0x7AED00**（MapShader 前提の共有初期化ヘルパ、vt 0xA8CCDC を先頭に書く 8B オブジェクト）と **0x771090**（NiDX9ShaderDeclaration ファクトリ、引数 = `[0xB43104]` オブジェクト/宣言番号等）を経由する（v129_a_factories 全 case の d1 パス）。= 28 クラスすべてが BSShader 系（NiD3DDefaultShader 派生）で、宣言キャッシュ（0xB43104 管理）を共有。
- arg0=4 の自再帰は「ShadowLightShader（slot1）が SpeedTreeBranch の内部依存として先行確保される」依存関係 [確定]。
- **v92 §a-5 の child クラス未特定问题は本表で解消**。vtable[1]（+0x04）= `mov eax, <静的型レコード>; ret` 型の **GetType()**（例 0x76C790 → &0xB4257C = NiD3DDefaultShader レコード、0x76D010 → &0xB42884 = NiD3DShader レコード）、vtable[0x8C] = FUN_007B81A0 が呼ぶ **再ロード系フック**（§9-a）[確定(機序)]。

---

## §3 (2) NiDX9Renderer +0x5E0 flags の各ビット — **[確定(機序)] / 名称 [仮説]**

### 3-1. 書き手（起動時 1 回のみ）

**FUN_00762110**（唯一の実質的書き手、E8 呼出し = 1 件 0x76A66A = FUN_0076a1c0+0x4AA = レンダラ初期化）:

1. `[renderer+0x280]`（COM 系オブジェクト）の vtable[+0x1C] に `&renderer[+0x284]` を渡して呼出 → **IDirect3DDevice9::GetDeviceCaps 相当**（vtable index 7）。失敗時 `"Creation failed: Could not query device caps"` (0xA88893C) / `"NiDX9Renderer::Initialize> Could not query device caps - FAILING"` (0xA888F8) を出力して false [確定]。= **D3DCAPS9 m_kD3DCaps = renderer+0x284（0x130 = 304B = sizeof(D3DCAPS9)、FUN_007641a0 で memset 0x130 と整合）**。
2. `cmp [renderer+0x318], 2 / cmp [renderer+0x31c], 2` の sanity gate（caps+0x94 = MaxVertexShaderConst / caps+0x98 = MaxNpatchTessellationLevel の解釈 [仮説]、実質は caps 充填確認）。
3. ビット算出（実装メモ）:

```
eax = caps.FVFCaps          ; renderer+0x2C0 = caps+0x3C
+0x5E0 := 0
if (!(FVFCaps & 0x002))          +0x5E0 := 8            ; D3DFVFCAPS_PALETTETEXTURES 非対応
else if (FVFCaps & 0x100)        +0x5E0 := 4            ; D3DFVFCAPS_DONOTSTRIPELEMENTS あり
+0x6D8 := caps.DevCaps2          ; renderer+0x32C = caps+0xA8
if (byte renderer+0x5C8 && !byte renderer+0x5C9)  +0x6D8 := 4   ; 上書き経路
if (+0x6D8 >= 4)                 +0x5E0 |= 2            ; DevCaps2 >= 4 相当
; 併行して +0x6E8 := (FVFCaps>>16)&1、+0x6E9 := (caps.Caps2>>29)&1 = D3DCAPS2_DYNAMICTEXTURES
```

### 3-2. ビット名（判定）

| bit | 値 | 機械的意味 [確定] | 推定名称 [仮説] |
|---|---|---|---|
| 3 | 8 | FVFCaps に PALETTETEXTURES(0x2) なし | 「パレットテクスチャ非対応」フラグ |
| 2 | 4 | PALETTETEXTURES あり かつ DONOTSTRIPELEMENTS(0x100) あり | 「ストライプ非強制」フラグ |
| 1 | 2 | DevCaps2 >= 4（+0x5C8/5C9 で強制 4 にした場合も成立） | 「拡張 D3D9 caps 有効」フラグ |

- 名称の全体像は「テクスチャ/FVF 処理能力レベル」 [仮説]。getter = **renderer vtable slot#21（+0x54）= 0x769F20** `mov eax,[ecx+0x5E0]; ret` [確定]。
- **getter の静的呼び出し元は検出 0 件** [未特定]：`call dword ptr [reg+0x54]`（disp8 全 7 レジスタ / .text 全域）は 6 件すべて非 renderer 系（1 引数形でゲッタの 0 引数シグネチャと不整合）。disp32 形・load→call 形の全走査は未実施 → **V143+ 残点**。

---

## §4 (3) [state+0xFF8] inner オブジェクトの正体 — **[確定]（DeviceWrapper 同一説 = ポインタ同一性を証明）**

### 4-1. 代入元（writers 全数、disp32=0xFF8 実命令スキャン v129_n_probe N4）

| writer | 動き |
|---|---|
| FUN_007802D0+0xA9（state ctor） | `[state+0xFF8] := 0`（初期化） |
| **FUN_0077F840**（setter、ctor から 1 本のみ呼ばれる） | 引数 = **renderer**（P8 再検証: factory 0x77BB00 は `push 0x1148` がスタック上に残る間に `[esp+0x18]` = arg1 = renderer を読んでいる）。処理: `[state+0xFFC] := renderer`、旧 inner があれば `vtable[2]`（Release）呼出、`edi := [renderer+0x280]`、`[state+0xFF8] := edi`、新 inner に `vtable[1]`（AddRef）呼出 |
| FUN_00780780+0x53 / FUN_0077F840(null 経路) | Release + 0 化 |

### 4-2. 判定

- **[state+0xFF8] == NiDX9Renderer[+0x280]（同一オブジェクト）[確定]**。refcount 付き（vtable[1]=AddRef / vtable[2]=Release の COM 互換レイアウト）。
- DeviceWrapper 同一説について: 「レンダラが保持するデバイス系 wrapper メンバと同一」= **証明**（ポインタ同一性）。ただし raw IDirect3DDevice9 そのものではない: inner へのフォワード呼び出しは vtable **+0xE4（index 57、3 引数）/ +0x104（index 65、3 引数）/ +0x1AC（index 111、tail-jmp）** で、raw D3D9 デバイス vtable（86 スロット）の範囲外 [確定(観測)]。クラス名の厳密同定は **[未特定 → V143+ 残点]**（renderer[+0x280] の生成箇所 = CreateDevice 直後の格納点を未追跡）。
- 使用部の全数（state 側、disp32=0xFF8 読み 39 site）: FUN_0077B000（+0xE4 フォワード、`[state+0xFA0+idx*4]` と同期）、FUN_0077B140 系（+0x104 フォワード 約 10 site）、FUN_0077B0A5（+0x1AC tail-jmp フォワード）、0x77FBE6（`[state+0xFF8]` を `[state+0xF8]` サブオブジェクトに 0x780940 で登録）[確定]。
- state は自前の D3DCAPS9 コピーを `+0x1018` に保持（factory 0x77BB27 `memcpy(+0x1018, arg2=&renderer[+0x284], 0x130)`、state malloc 0x1148 = 0x1018+0x130 と整合）[確定]。

---

## §5 (4) slot +0x140 / +0x144 の呼び出し元と行列更新の触发条件 — **[確定(観測)]**

### 5-1. スロット実体（NiDX9Renderer vtable 0xA88EA4、RTTI ベース確認 v129_d_probe E）

| slot | target | 要点 |
|---|---|---|
| #80 +0x140 | **0x762640**（401 命令） | 引数の NiPoint3/NiRect 系 3+7 個を `[0xB3F92C/30/34]`（カメラ位置グローバル）と `renderer[+0x980..]` に転記するカメラ/行列セットアップ本体 |
| #81 +0x144 | **0x762BE0**（50 命令） | `[renderer+0x87C]`（state）の vtable[+0x4C]/[+0x50] を arg=0 で呼出して幅高さ等を取得 → float 化して `[renderer+0x980]` を再構築 → `[renderer+0x280]`（デバイス wrapper）に `push &matrix; push 2` で転記 = **ビューポート/射影行列の再構築** |

### 5-2. 呼出し元（vtload+call 形の全走査 v129_e_probe A2）

- **+0x140 = 4 site**:
  1. `0x76A923`（FUN_0076a1c0+0x70F、レンダラ初期化）— ゲートなし。デフォルト定数グローバル `0xB3F9A8 / 0xB258D0 / 0xB258DC / 0xB258E8` を渡す初期カメラ設定 [確定]
  2. `0x7D4E21` / `0x7D4EED`（FUN_007d46c0 の 2 パス）— 7 引数（スタック上の NiRect/NiMatrix4 バッファ群）
  3. `0x8133D4`（FUN_00813300+0xA8）
- **+0x144 = 13 site**: FUN_0049bef0 / FUN_004dd4b0 / FUN_004e0f80 / FUN_00766f30 / FUN_007672f0 / FUN_007af5b0 / FUN_007b0170 ×2 / FUN_007b18c0 / FUN_007bdfc0 ×3 / FUN_007de8a0 — 大半が 1 引数（行列/オブジェクト ptr）。

### 5-3. 触发条件（ゲートの定型）— **[確定(観測)]**

呼出し直前に必ず同一の 3 条件ゲートが入る:

```c
if ((renderer[0x200] == 1 || renderer[0x204] == 1) && renderer[0x20c] == 1)
    renderer->vtable[+0x140 or +0x144](...);   // 実行されない場合は即スキップ
```

- 書き手（v129_i_probe I4 全数）: `+0x20C` = 1: FUN_007d6a80（+0x200/+0x204 が 1 で +0x20C が 0 のときのみ=BeginFrame 系）/ 0: FUN_007d6b00（EndFrame 系、vtable[+0x14C] 前提）。`+0x200` = FUN_007d7210（vtable[+0x134] 探測成功時に 2）/ FUN_00713ABC（0x400）/ ctor 0x701700（0）/ FUN_007D72D0 で +0x204:=0。`+0x204` = FUN_004FCC40 / FUN_004FCF10。
- 意味論: **「フレーム中（+0x20C）かつ シャドウパス/特殊パス active（+0x200/+0x204）のときだけ、シャドウ用カメラ・ビューポートに切り替えて行列を再設定する」**。通常時は +0x140/+0x144 を呼ばず、renderer 初期化時の 1 回（デフォルト行列）以外はシャドウパス固有 [確定(観測)] / 具体的なパス名（ShadowScene系）[仮説]。

---

## §6 (5) b42ea5 gate と case 0 factory の意味論 — **[確定]**

### 6-1. case 0（param_1 = 0）

- ディスセンブル実測: `cmp esi,0x1b; ja 0x7b44a4` → スロット既存検査 → `lea eax,[esi-1]; cmp eax,0x1a; ja 0x7b449b`。**esi=0 はこの時点で eax=-1 > 0x1A なので jump table に入らず 0x7B449B（`return slot[0]`）へ**。slot[0] は本関数のどの case からも書かれず常に 0。
- 判定: **arg0=0 は「シェーダ無し」を意味する素の NULL 返却。factory は存在しない。slot[0] も一切更新されない** [確定]。arg0>0x1B も 0x7B44A4 で NULL [確定]。

### 6-2. b42ea5（byte グローバル、0xB42EA4 dword の中間バイト、imm32 全 10 site 分類 v129_o_probe O2）

| role | site | 内容 |
|---|---|---|
| **W（起動）** | FUN_007b7250+0x3F8 | `mov [0xB42EA5], 0`（CRT 初期化） |
| **W（初期化）** | FUN_007b45f0+0x8B | 第 5 引数 `> 0xFF`（255）のとき `=1`。同関数は先頭で `[0xB42F48](shader level) := 2`、同 `> 0x60`（96）で 2→6 に上方修正、引数文字列の `_strnicmp(...,"ati",3)` 分岐あり = **アダプタ能力/VRAM 量ベースの shader level 決定関数** [確定(機序)]。第 5 引数の正体は未特定だが `>255` が Lighting30 許可条件（= INI `bAllow30Shaders` 相当 [仮説(高)]） |
| R | FUN_007b4290+0x1E7 | case 26 の gate（§2 表） |
| R | FUN_00553bb0+0x2F2 | `b42ea5 ? push 0x1A(26) : push 0xE(14)` → FUN_007B8940 に渡す shader property 生成系（0x1A=Lighting30 と整合、0xE フォールバックの名称は [未特定]） |
| R | FUN_007d1920 ×3 / FUN_007dab80 ×2 | レンダリング・パス選択（FUN_007dab80 は `[0xB42F48]-1` switch で b42ea5/b43070/b43071 組合せにより戻り値 1..0x11 を返す quality/technique コード化関数）[確定(機序)] |
| R（未分類） | FUN_00498340+0x807 | skipdata 域で文脈判定不能 [残点小] |

判定: **b42ea5 = 「Lighting30（SM3.0 ライティング）シェーダを有効化する設定フラグ」。起動 0、レンダラ初期化時にアダプタ条件で 1。case 26 のみが消費し、0 のときは clear path（NULL 返却）** [確定(機序)]。

### 6-3. 他の gate グローバル（意味確定）

- `0xB42F48` = **shader level**（int。起動 0 → FUN_007b45f0 で 2、VRAM>96 で 6。case 7/11/21/24 の閾値 2、FUN_007dab80 でも switch キー）[確定]
- `0xB43070` = **HDR 有効フラグ**（byte、case 8 gate、FUN_007dab80 でも参照）[確定]
- `0xB43071` = HDR 系サブフラグ（FUN_007dab80 の setne で加算値に使用）[確定(観測)] / 意味 [仮説]

---

## §7 (6) FUN_007B8530 arg0=0 分岐（0x7B88d9 以降）— **[確定(機序)]**

FUN_007B8530（台帳なし・無名、BSShaderManager 破棄ルーチン）先頭:

```asm
0x7B853A: cmp byte ptr [esp+0xc], bl(=0)
0x7B853F: je  0x7b88d9            ; arg0==0 → 共通 tail へ直行
```

- **arg0 != 0（フル経路）**: `[0xB42EBC]:=0` → call 0x7C4D90 / 0x7B3E60 → **28 スロット loop**（refcount>1 なら `"Shader %i is leaking..."` 0xA8F508 を 0x4A7A60 経由で出力 → FUN_007b7170(slot) → free → 0 化）→ call 0x7E30F0 / 0x7F3BA0 → b430xx シングルトン群（0xB430EC/DC/F0/E0/100/E4/11C/120/F8/110/D4/114/F4/FC）を refcount 減算+delete で解放 → 0xB42D7C / 0xB42F50 を dtor+free → 0xB430FC 解放 → call 0x7AB1D0(0) → `[0xB42F31]:=0` → **共通 tail 0x7B88D9 へ fall-through**。
- **arg0 == 0（軽量経路）**: 上記をすべて飛ばし、**共通 tail 0x7B88D9 のみ実行**: 0xB43104 オブジェクトの refcount 減算 + delete + 0 化、`[0xB42EB8]:=0`、return。**シェーブテーブル 28 スロットと b430xx シングルトン群には一切触れない** [確定(機序)]。
- 呼び出し元: 直接 E8 は FUN_00497B50 の 1 件のみ（引数を素通り）。FUN_00497B50 は [0xB350D8] の refcount を確認し、解放対象レンダラにスマートポインタが残るとき `"There is 1 object with a smart pointer to the renderer we are trying to release."` (0xA3DD78) を 0x4A7A60 で出力 [確定]。唯一の到達可能呼び出し 0x40F5CA（FUN_0040d4d0+0x20FA = 終了処理）は **`push 1`（フル）**。よって **arg0=0 経路は静的コールグラフ上到達可能 caller が 0 件** [確定(静的)] / 実行時 caller [未特定 → 残点]。付随: FUN_00497B20 = 最終レンダエラー文字列を 0xB34FC8 にコピーする setter [確定(機序)]。

---

## §8 (7) renderer gap 50 / state gap 54 の tiny getter ラベル化 — **[確定]（分類は v129_e_probe (F) 全リスト）**

台帳未登録（gap）スロットのうち、tiny getter/常値 thunk をラベル化（判定: ボディ実測 [確定]、名称は提案ラベル）。

### 8-1. NiDX9Renderer（gap 50 スロット中）

| slot | target | ボディ | 提案ラベル |
|---|---|---|---|
| #1 +0x004 | 0x769F00 | `mov eax, 0xB42168; ret` | `GetMyTypeRecord`（0xB42168 = NiDX9Renderer 静的型レコード、G1a 登録 stub 確認）[確定] |
| #6 +0x018 | 0x700640 | `mov eax, ecx; ret 4` | `GetThisPointer` [確定(機構)] |
| #19 +0x04C / #64 +0x100 | 0x6FDFF0 | `mov al, 1; ret 4` | `IsSupported`（常真述語、共有ヘルパ）[確定(機構)] |
| #21 +0x054 | 0x769F20 | `mov eax,[ecx+0x5E0]; ret` | `GetTextureCapabilityFlags`（§3）[確定] |
| #22 +0x058 / #23 +0x05C | 0x769F70 / 0x769F80 | `[+0x5D8]:=f4` / `ret` に fld | `SetNearVisibility` / `GetNearVisibility` [仮説] |
| #27 +0x06C / #28 +0x070 | 0x76A170 / 0x76A180 | `[+0x5DC]:=d4` / `mov eax,[ecx+0x5DC]; ret` | `SetFarVisibility` / `GetFarVisibility` [仮説] |
| #31 +0x07C | 0x762590 | `mov eax,[ecx+0x878]; ret` | `GetRenderStateA` [確定(参照先は state 系)] |
| #32 +0x080 | 0x769F60 | `mov eax,[ecx+0x87C]; ret` | `GetRenderStateB` |
| #33 +0x084 | 0x769F10 | `mov al,[ecx+0xA90]; ret` | `GetByteFlagA90` |
| #39 +0x09C | 0x762570 | `mov eax,[ecx+0x894]; ret` | `GetU32_894` |
| #40 +0x0A0 | 0x762580 | `mov al,[ecx+0x898]; ret` | `GetByteFlag898` |
| #43 +0x0AC | 0x7625F0 | `mov eax,[0xB42050]; ret` | `GetGlobalShaderDeclHint`（setter は #42 0x7625E0）[確定(機構)] |
| #34/#35/#41/#46/#56/#57/#58/#59 | 0x7625A0/0x7625B0/0x7625C0/0x763010/0x769F30/0x769F40/0x76A190/0x76A1A0 | `[+0x880]`/`[+0x8B8]`/`[+0x8AC]` メンバへ vtable フォワード | `ForwardTo<member>@slot` [確定(機構)] |
| #86/#87 +0x158/+0x15C | 0x76B310 | `jmp 0x769960` | 尾ジャンプ thunk [確定(機構)] |

### 8-2. NiDX9RenderState（gap 54 スロット中）

| slot | target | ボディ | 提案ラベル |
|---|---|---|---|
| #31 +0x07C | 0x77B0C0 | `mov eax,[ecx+0xFE8]; ret` | `GetSavedSlotValueA(0xFE8)` [確定] |
| #35 +0x08C | 0x77B390 | `mov eax,[ecx+0xFE0]; ret` | `GetSavedSlotValueB(0xFE0)` [確定] |
| #60 +0x0F0 | 0x77B7A0 | `mov al,[ecx+0x1014]; ret` | `GetByteFlag1014`（factory が 0 初期化 @0x77BB3A）[確定] |
| #62 +0x0F8 / #64 +0x100 | 0x77B320 / 0x77B340 | `mov al,[ecx+0xFF4]/[+0xFF5]; ret` | `GetByteFlagFF4/FF5`（setter は #61/#63）[確定] |
| #15 +0x03C | 0x629290 | `fld dword ptr [ecx+0x88]; ret` | `GetFloat88`（別領域の共有ヘルパを流用）[確定(機構)] |
| #11 +0x02C 等 | 0x77BAC0 ほか | `[ecx+0x74]` 等の内部オブジェクトへフォワード | `ForwardTo<member>` 群 [確定(機構)] |

残りの gap スロット（renderer / state 各数十）は通常メソッドであり、全数分類表は v129_e_probe.txt (F) に保存済み（本ノートには差分のみ記載）。

---

## §9 (8) v92 残4件の解消

### 9-a. センチネル 0xB4257C の所有者（18 site / 4 関数）— **[確定]**

**0xB4257C は「静的型レコード」オブジェクト = {name = 0xA89800 "NiD3DDefaultShader", parent = 0xB42884}**（FUN_0070E220 = 2 ワード格納レコード初期化ヘルパ `[this]=name, [this+4]=parent, ret 8`）。親レコード 0xB42884 の登録 stub 実測: `push 0xB42858; push 0xA8A9CC("NiD3DShader"); mov ecx,0xB42884; call 0x70E220`（0xA10BE0）→ **0xB42884 = NiD3DShader 型レコード** [確定]。親 0xB42858 の name は未取得 [残点小]。

4 関数の役割（v129_n_probe N6, v129_l_probe L2）:

| 関数 | site 数 | 役割 [確定] |
|---|---|---|
| FUN_0076C790（FUN_0076c760 内 tiny fn） | 1 | `mov eax, 0xB4257C; ret` = **NiD3DDefaultShader 系の GetType()**（13 シェーダ vtable の slot+0x04 に配置、v129_d_probe H） |
| FUN_007B8150 | 2 | `[arg+0xBC]` オブジェクトの型連鎖を **0xB42884（NiD3DShader）** まで歩き、`FUN_00560920(0xB4257C, obj)` で NiD3DDefaultShader に dynamic-cast するヘルパ。annotate 名 "NiControllerManager_FindSeq" は**誤り**（0x560920 は汎用 **IsOfType(type)-based cast** ヘルパ）[確定(機序)] |
| FUN_00A0A290 | 1 | CRT 静的初期化 stub: NiD3DDefaultShader レコード登録（0xA109F0） |
| FUN_00A10E50 | 14 | CRT 静的初期化 stub 群: **親 = 0xB4257C の派生シェーダ型レコード登録**（SkyShader@0xB431D8 / ShadowLightShader@0xB44F90 / WaterShader@0xB45F38 ほか name 文字列実測、O8） |

= 18 site は「レコード本体の初期化 stub + GetType getter + IsOfType 判定」の 3 種に完全収束。FUN_007B81A0 の `p==&0xB4257C` 検査は **「この shader は NiD3DDefaultShader 派生か」** を意味する [確定]。v92 の「低信頼 annotate」問題は解消。

### 9-b. FUN_004DC270 の lookup キー / FUN_004A7A60・0x4941A0 の出力先 — **[確定(機序)]**

- **FUN_004DC270**: 本文実測（N9a）。引数オブジェクトの内側構造（`[arg+0x10]` 配列 / `[arg+0x14]` 個数、ループは後端から走査、`[0xB333C4]` グローバルから 0x660110 経由でベース取得）を歩き、各要素の `vtable[1]`（GetType）連鎖を **型レコード 0xB35ACC** まで辿って一致した要素を返す。0xB35ACC の登録 stub 実測: `push 0xA43C18; mov ecx,0xB35ACC; call 0x70E220`（0x9E26C5）→ **name = "TESObjectExtraData"** [確定]。
  判定: **lookup キーはメッセージ文字列ではなくオブジェクト（TESObjectExtraData 型フィルタ付きグラフ探索）**。v92 §a-4 の「arg0 → FUN_004DC270(msg)」の解釈は訂正: arg0 は script/form 系オブジェクトで、NULL 返却 = 「対象コンテキストなし → 出力なし」[確定(反証)]。
- **FUN_004A7A60**: varargs フォワーダ `FUN_004941A0(1, fmt, va_list)` [確定]。
- **FUN_004941A0**: `[0xB34D90] == 0` なら即 return（**ゲート**）→ `0x982820`（vsnprintf 系）でスタックバッファ整形 → CRLF 正規化ジャンプテーブル 0x494364（10 ケース、末尾に `\r\n` 0xA3D9B0 追記）→ **`mov ecx,[0xB34D90]; mov eax,[ecx]; mov eax,[eax]; call` = [0xB34D90] オブジェクトの vtable[0] 呼び出しで最終出力** [確定(機序)]。
  判定: **出力先は「[0xB34D90] が指すコールバック/ロガー オブジェクト」経由**。ログファイル/OSM/コンソールのいずれかは [仮説(ロガー系)] で、0xB43104 との関係（= v92 §c-2 の shader declaration 管理オブジェクトと別物）[未特定 → 残点]。直接的な OS ファイル API 呼び出しは本関数内にない [確定]。

### 9-c. opid 0x133 の実行時 dispatcher 走査 — **[確定(観測)] ／ trampoline→impl は [残点]**

- entry82 @0xB0BC18 の raw ダンプ（N7b）= v92 a-1 表と**バイト一致**（+0x18 = 0x50AA20 impl / +0x1C = 0x4FDE30 shared dispatcher）。
- 実行時経路（v129_e_probe K2 実測）: VM 取り出しループ（0x4FF904 `lea eax,[ebx + 0xB0AF48]` = テーブル base + idx*0x28）→ entry[+0x10] flag / [+0x12] argc チェック → **`mov ebx,[eax+0x1C]; call ebx` @0x4FF9B9** = entry[+0x1C] 共通 trampoline 呼び出し → FUN_004FDE30（本文 0x4FDE30..、バイトコード列 `[esi+0x40c]` から argc 個の引数を 0xB0A54C 記述子テーブル + 0x4FEEAC ジャンプテーブルでパース）。
  判定: **runtime は「entry → +0x1C trampoline」を必ず経由する**（impl への直接 E8 は存在しない）[確定]。ただし **trampoline 内部から entry[+0x18] impl（0x50AA20）への transfer 位置は未同定**（`call [reg+0x18]` パターン 9 種 + load→call 4 種を 0x4FDE30..0x4FED20 で全走査し 0 件）[未特定 → V143+ 残点]。

### 9-d. renderer vtable 内部セルフ呼出リストの再検証 — **[確定]（誤検を確定）**

98 スロット全走査（v129_i_probe I3 / v129_e_probe G）: 候補 24 件を 4 方式に再分類:

| 分類 | 件数 | 内容 |
|---|---|---|
| **MEMBER-FWD** | 13 | `[this+0x8AC]`（state）/ `[this+0x8B8]` メンバへフォワードする thunk（slot #1/21/31/33/34/35/36/39/40/41/46/56/57）= **セルフ呼出ではない誤検** |
| **SELF**（真のセルフ再帰） | **2** | slot#12 +0x030 = FUN_00701A30（base=this @0x701A9B）、slot#82 +0x148 = FUN_007647E0（base=this @0x7647E2） |
| OTHER | 7 | base が別オブジェクト（[edi]/[esi]/[ebx+0x8A0]/0xA28074 等）由来 |
| 未分類 | 2 | slot#37 (+0x094) / slot#49 (+0x0C4)、prov 未解決 |

判定: v92 の懸念「誤検含み」は**妥当**。真の内部セルフ呼出は **2 スロットのみ** [確定]。

### 9-e. FUN_007B4290 clear path の触发条件 — **[確定]**

全文ディスセンブル（N1a-c）で clear path = **共有 stub 0x7B4359 `mov [esi*4+0xB42EC0], ecx(=0); mov eax, ecx; ret`**（= スロットに NULL を書いて NULL を返す）に収束する到達元を全数特定:

| 到達元 | 条件 | 対象 arg0 |
|---|---|---|
| 0x7B4349 `je 0x7b4359` | `[0xB43070] == 0`（HDR 無効） | 8 |
| 0x7B4387 `jl 0x7b4359` | `[0xB42F48] < 2`（shader level 不足） | 11, 21, 24 |
| 0x7B447B `je 0x7b4359` | `[0xB42EA5] == 0`（Lighting30 無効） | 26 |

- 注: 関数は既存スロット検査（0x7B42A0）を先に通った場合のみ switch に入るため、clear path の store は「まだ NULL のスロットに NULL を再書く」冗長だが無害な操作 [確定]。
- case 7 は b42f48 で**二系統を選択するだけ**で NULL 化しない。case 4 は先行自再帰あり。無条件 case 1-6,9,10,12-20,22,23,25,27 は常に生成 [確定]。
- 「どのケースが実行時 gate 失敗するか」= **{8, 11, 21, 24, 26} の 5 ケースのみ**。ゲート値はいずれも設定/アダプタ由来の静的値（§6-2）なので、実行時には「起動後ほぼ決定的」に NULL が返るか返らないかが決まる（動的バイナリ実行ではなくコードパス証明による検証）[確定]。

---

## §10 結論 + Android 移植示唆

### 10-1. 8 項目判定まとめ

| # | 項目 | 判定 |
|---|---|---|
| (1) | factory→28 クラス対応表 | **[確定]**（§2、v92 §a-5 child 未特定問題を解消） |
| (2) | +0x5E0 ビット &8/&4 | **[確定(機序)]**（8=FVFCaps.PALETTETEXTURES なし / 4=+DONOTSTRIPELEMENTS / 2=DevCaps2>=4）、名称 [仮説]、getter 呼び出し元 [未特定] |
| (3) | [state+0xFF8] 正体 | **[確定]** = renderer[+0x280]（refcount 付き device 系 wrapper、DeviceWrapper 同一説をポインタ同一性で証明）、クラス名厳密同定 [未特定] |
| (4) | +0x140/+0x144 呼び出し元・触发 | **[確定(観測)]** = レンダラ初期化 1 回 + シャドウパス中（[+0x200/204]==1 && [+0x20c]==1）限定 |
| (5) | b42ea5 / case 0 | **[確定]** = Lighting30 許可フラグ（>255 条件で設定）/ case 0 は factory なしの NULL 返却 |
| (6) | 0x7B8530 arg0=0 | **[確定(機序)]** = 0xB43104+0xB42EB8 のみ解放する軽量経路、静的 caller 0 件 |
| (7) | renderer gap 50 / state gap 54 tiny getter | **[確定]**（§8 ラベル表、全分類は v129_e_probe (F)） |
| (8) | v92 残4件 | sentinel 所有者 **[確定]**（静的型レコード系）/ lookup キー **[確定]**（TESObjectExtraData グラフ探索）/ 出力先 **[確定(機序)]**（[0xB34D90] オブジェクト vtable[0]、その正体は [仮説]）/ opid 0x133 runtime **[確定(観測)]**（+0x1C trampoline 経由を実測、trampoline→impl は [残点]）/ self-call 再検証 **[確定]**（真 SELF=2 件）/ clear path **[確定]**（case {8,11,21,24,26}） |

### 10-2. Android 移植示唆

1. **28 スロット遅延確保テーブル**: gate 失敗時は **NULL 返却**が実際に起き得る（case 8/11/21/24/26）。移植側は nullable 参照 + 呼び出し元 null ハンドルが必須。原典は slot に NULL を再書くだけで呼び出し元が毎回再評価する設計。
2. **3 ゲートグローバル**（b42f48 shader level / b43070 HDR / b42ea5 Lighting30）は起動時のアダプタ能力判定一発で決まる静的設定 → Android では capability チェック 1 回に集約可能（FUN_007b45f0 の `_strnicmp("ati")` 分岐は不要化）。
3. **[state+0xFF8] フォワーディング**（+0xE4/+0x104/+0x1AC の 3 メソッド群）はデバイス級 dispatch → Vulkan/GLES では device-level dispatch 層（VkDevice ラップ）に集約できる。AddRef/Release 付き wrapper 1 本の差し替えで済む構造。
4. **+0x140/+0x144 の 2 段ゲート**（フレーム中 && シャドウパス中）は「シャドウ用カメラ切替」ロジックとして移植価値が高い。フラグ 3 本（+0x200/+0x204/+0x20c）の書き手（BeginFrame/EndFrame 系）まで対応表を切れば移植単位が確定する。
5. **RefreshShaders 経路**（opid 0x133 → trampoline → impl 0x50AA20 → FUN_007B81A0）はホットリロード機構。Android では asset 再ロード + コールバック差替（0xB42E8C パターン、v92 §a-4）を対にすれば同等機能が得られる。
6. **軽量 teardown（arg0=0）**: 「テーブル温存の部分解放」パスは設計上再利用可能（GPU device lost 時の再作成で 28 シェーダを守りつつ device wrapper だけ張り替える等）。フル teardown の leak 警告 + assert 出力はログ降格。
7. **静的型レコード系（0xB4257C 等）**: Gamebryo の IsOfType 連鎖は、Android 版では C++ RTTI/dynamic_cast か独自 TypeId 1 本に置換可能（GetType() getter + 親リンクの 2 ワード構造は極小）。

---

## §11 残留不明点（V143+ 粒度）+ 自己検証

### 11-1. 新規残点

| 粒度 | 内容 |
|---|---|
| V143 | renderer[+0x280] のクラス名厳密同定（CreateDevice 直後の格納箇所を未追跡。+0x1AC=slot111 が raw D3D9 vtable 範囲外という矛盾の解消） |
| V143 | renderer vtable[+0x54]（+0x5E0 getter）の呼び出し元（disp8 `call [reg+0x54]` 6 件は非該当。load→call 形 / disp32 形の全走査が未実施） |
| V143 | FUN_004FDE30（opcode trampoline）内から entry[+0x18] impl への transfer 位置（13 パターン全走査 0 件、レジスタ保持形の可能性） |
| V143 | [0xB34D90] ロガー オブジェクトのクラス・格納元（FUN_004941A0 の最終 sink） |
| V144 | 型レコード連鎖の根: 0xB42858 の name 文字列（0xB42884=NiD3DShader の親） |
| V144 | FUN_00553bb0 の `b42ea5 ? 0x1A : 0xE` 選択で FUN_007B8940 に渡す 0xE の名称（本 switch の case14=SkinShader か否か） |
| V144 | FUN_00498340+0x807 の 0xB42EA5 参照の read/write 分類（skipdata 域） |
| V144 | FUN_007B8530 arg0=0 経路の実行時 caller（静的到達 0 件） |
| 小 | 0xB42EA4/0xB42EA6/0xB42EA7（b42ea5 隣接 config バイト列）の各役割 |

### 11-2. 自己検証

- **ハーネス**: `probe_harness.py selftest` → **4/4 PASS**（着手時 + 本セッション着手直後の 2 回実行、パス `tmp\re\handover_20261025\scripts\probe_harness.py`）。
- **(1) の表**: 独立 2 手法（ctor vtable 追跡 v129_a3/a4 vs malloc→ctor→vt 連鎖 v129_03_factdis）が 28/28 一致。v129_a_factories の depth-0 誤帰属は §2-1 の通り解消済み。O1 のジャンプテーブル 27 エントリ全文展開が a4 の case 番号と全一致（case 12/13 のボディ順序がメモリ順と異なるのはテーブル間接のためで矛盾なし）。
- **(3) の自己訂正**: 途中「state ctor 引数 = &renderer[+0x284]（caps）」という誤読を発見し、P8 で「factory は `push 0x1148` 残し中に `[esp+0x18]`=arg1=renderer を読む」ことを再測して訂正（誤読のままなら inner=[renderer+0x504] となり矛盾）。state サイズ 0x1148 = 0x1018（caps コピー位置）+ 0x130（sizeof D3DCAPS9）で整合。
- **entry82**: raw ダンプ 40B が v92 a-1 のフィールド表とバイト一致。
- **§9-a**: 0xB4257C/0xB42884 の両登録 stub を別関数（FUN_00A0A290）内で実測し、親子関係（NiD3DDefaultShader → NiD3DShader）を双方向から確認。
- 成果物は本ノート 1 ファイルのみ。docs/・TODO・台帳・git 変更なし。

### 11-3. 使用証拠一覧（中間出力）

既存再利用: v129_01_bodies / 02_factories / 03_factdis / 04_gates / 05_struct / 06_scan / extract / a_factories / a2_factories / a3_ctor / a4_ctorclass / b_scan / c_dump / d_probe / e_probe / f_caller / g_ident / g_verify / h_probe / i_probe / j_probe / k_probe（txt 同梱）。
本セッション新規: **v129_n_probe**（switch 全文・0xB42EA5/0xFF8 全走査・`call [reg+0x54]`・sentinel contexts・entry82・+0x140/144 本体・lookup/output fn 見出し）、**v129_o_probe**（ジャンプテーブル 27 逆引き・b42ea5 10 site 文脈・0x497B20/50・0x4941A0 sink・state ctor/setter・0xB35ACC 登録）、**v129_p_probe**（0x4FDE30 深部・型レコード文字列・FUN_007b45f0・FUN_007dab80・0x553bb0・factory 引数再確認）、**v129_q_probe**（0xB42884 登録 stub・+0x280 書き手走査・trampoline impl-call パターン 13 種・0x553bb0 拡張）。

---

## §12 再検証エージェントによる独立確認と §5 の訂正（2026-10-27 追記）

> 本節は末尾への**追記のみ**（§1〜§11 は無改変）。§5-2 / §5-3 の記載は 12-1 の訂正値で読むこと。

### 12-1. 訂正 2 件

**C-1（§5-2）: `+0x144` 呼出し元は 13 site ではなく 33 site / 18 関数 [確定]**

- 根拠は本ノートが引用する `v129_e_probe.txt` (A2) 自身の末尾行 `=> 33 confirmed vtable-call site(s)`（+0x140 は `=> 4 confirmed` で §5-2 の「4 site」と一致）。
- §5-2 の 13 はアドレス昇順の先頭 13 件（0x49C267 … 0x7DEBDD）で打ち切り、FUN_007de8a0 を ×1 と数えた値。欠落 20 件 = FUN_007de8a0 残り 2 + FUN_007e17d0 ×7 + FUN_007e9db0 ×3 + FUN_007eb3d0 ×2 + FUN_007f5120 / FUN_007f9d10 / FUN_00800440 / FUN_00802890 各 1 + FUN_00803e40 ×2。
- 全 18 関数（R16b 実測）: FUN_0049bef0 / FUN_004dd4b0 / FUN_004e0f80 / FUN_00766f30 / FUN_007672f0 / FUN_007af5b0 / FUN_007b0170(×2) / FUN_007b18c0 / FUN_007bdfc0(×3) / FUN_007de8a0(×3) / FUN_007e17d0(×7) / FUN_007e9db0(×3) / FUN_007eb3d0(×2) / FUN_007f5120 / FUN_007f9d10 / FUN_00800440 / FUN_00802890 / FUN_00803e40(×2)。

**C-2（§5-3）: 「呼出し直前に必ず同一の 3 条件ゲート」は過大。37 site 中 5 例外 [確定]**

gate あり = **32 / 37**。例外 5 件（`v129_p_rverify2.py` R17a/b で A2 の全 37 site を 0x20C disp32 presence で byte 走査）:

| site | 所有関数 | 例外の内容 |
|---|---|---|
| 0x76A923 (+0x140) | FUN_0076a1c0 | gate なし（§5-2 の「ゲートなし」註記と一致 = レンダラ初期化） |
| 0x4DD829 (+0x144) | FUN_004dd4b0 | 代わりに `cmp byte ptr [esp+0x24],0 / je` のスタックフラグ gate |
| 0x4E1355 (+0x144) | FUN_004e0f80 | gate なし（前段 vtable 呼出と直列） |
| 0x7671A9 (+0x144) | FUN_00766f30 | gate なし。受信 base が `[esi+0x280]`（wrapper）で renderer 本体でもない |
| 0x7674A1 (+0x144) | FUN_007672f0 | 同上（`[edi+0x280]`） |

- gate を持つ 32 site の式は §5-3 の記載どおり `([+0x200]==1 || [+0x204]==1) && [+0x20c]==1` [確定]。比較値 1 を実測: FUN_007d46c0+0x718 `mov eax,1` → 直後 3 cmp（0x7D4DDD）、FUN_007af5b0+0x1C9 `mov edi,1` → `cmp [ecx+0x200],edi` / `cmp byte [ecx+0x20c],1`（0x7AF78E）、FUN_007e17d0+0x309 `mov eax,1`（0x7E1AE1）＝標本 3/3 一致。
- gate base の大半は `mov ecx,[0xB3F928]`（グローバル）。FUN_00701700 ctor が `[0xB3F928]:=this`（0x701767、v129_i_probe I5 と一致）を書き、かつ +0x200/+0x204/+0x20c を 0 初期化する（0x701748/0x70174E/0x70175A）。§5-3 の `renderer[...]` 表記は FUN_00701700 が NiDX9Renderer の base ctor である限り整合 [確定(機構)] / FUN_00701700 の型名は [仮説]。

### 12-2. 独立再検証（本エージェント）

- **ハーネス: `probe_harness.py selftest` → 4/4 PASS**（本セッション 2 回実行）。
- 検証スクリプト（`%TEMP%\opencode`）: `v129_p_rverify.py`（初回 22 項目中 NG12 = 実誤り 1〔R16b +0x144 カウント〕+ §5-3 過大を示唆 1〔R14〕+ 検査側バグ 10〔窓長・期待バイト・書式〕、rverify2 で全解消）/ `v129_p_rdump.py`（+0xFF8 setter 本文・gate 4 site 文脈の実測）/ `v129_p_sprobe.py`（vtload 生カウント・gate presence）/ **`v129_p_rverify2.py` = 27 項目 27/27 PASS**（出力 `v129_p_rverify2.txt`）。
- 27 項目（すべてバイナリ実測）: R1 ジャンプテーブル 27 body→factory 実在（case4 自再帰込み）/ R2 §2 の 28 クラス名 = RTTI 索引 28/28 一致 / R3 renderer vtable +0x04/+0x54/+0x140/+0x144 = 0x769F00/0x769F20/0x762640/0x762BE0 / R4 tiny getter 6 本のバイト列 / R5 引用文字列 8 / R6 型レコード登録 stub 3（0xB4257C・0xB42884・0xB35ACC）/ R7 0x7B8530 の arg0 gate / R8 0x4941A0 の [0xB34D90] gate / R9a-c clear path stub + gate case {8,11,21,24,26} + 非 gate 20 case が ret で離脱 / R10 entry82 40B byte 一致 / R11 opid 0x133 impl・dispatcher・name / R12 [state+0xFF8] setter 3 命令 / R13 0xB42EA5=10 site・0xB4257C=18 site/4 owner / R14 28-slot loop・shader level 2→6 / R15 gate==1 標本 3 / R16 site census 4・33 / R17 gate presence 3/4・29/33。
- **§2〜§4・§6〜§11 の記載は訂正なく再現**（訂正は上記 §5-2 / §5-3 の 2 件のみ）。
- 成果物は本ノート 1 ファイルのみ。docs/・TODO・台帳・git・Supermemory・sync 無変更。
