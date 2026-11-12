# V190: renderer/graphics/anim 残点束 — V-171a–f / V-173a–c / v187 N1–N4 / v183 §11-4 訂正ドラフト — Oblivion.exe v1.2.0416 Steam

- 作成日: 2026-10-29 / 担当: graphics/anim 残点分析サブエージェント（TODO serial 178 / V178・中）
- 編集範囲: **本新規 1 ファイルのみ**（docs/TODO/session-memory/plan/cache/既存 tmp/re は読取のみ）。git 不使用。ESM 再スキャンなし。Ghidra 呼出なし。
- 入力（読取専用）: `tmp/re/v183_renderer_stream_residuals.md`（§残点 V-171a–f / §11-4 訂正素材）、`v173_small_residuals2.md`（V-173a–c）、`v187_anim_residuals.md`（N1–N4）、`s61_render.md`、`s75_anim.md`、`s81_stream_audio_crt_io.md`（A4）、`v129_shader_slot_residuals.md`（§4-2）、`cache/{named_funcs,vtable_index,call_targets}`、`docs/Oblivion_decompiled.c`（部分検索）。
- 手法: capstone 5.0.7（skipdata）+ probe_harness `Ctx` + バイトパターン/imm32/E8 全数走査。中間スクリプトは `%TEMP%\opencode\v190_*.py`。
- 表記: [確定] / [確定(観測)] / [確定(機序)] / [推定] / [未解明]。
- 着手時ハーネス: `probe_harness.py selftest` → **4/4 PASS**。

---

## §0 対象項目と作業状態

| 項 | 対象 | 状態 |
|---|---|---|
| V-171a | inner（renderer[+0x280]）生成箇所の静的残差 | **解明 [確定(観測)]**（§1） |
| V-171b | inner 実行時 vtable の capstone 同定可能スロット | **解明 [確定(観測)]**（§2、B13 依存なし） |
| V-171c | BSShaderPPLightingProperty slot38/39（0x855240/0x85AE20）描画パス設定本体 | **大部分解明**（§3＋§3-4 追記: ライト供給元・定数転記層を特定。残りは V-190a を縮小） |
| V-171d | SkyShader vtable と天候遷移駆動関数 | **解明 [確定(観測)]**（§4） |
| V-171e | 通常時の NIF→NiNode ツリー組立エントリ | **大部解明**（§5、NiStream::Load 単体は V-190d） |
| V-171f | s61 §1.2 件数データの実メソッド再対応 | **解明 [確定(観測)]**（§6） |
| V-173a | FUN_004dc270 入力 context の実クラス名 | **解明 [確定(機序)]**（§7） |
| V-173b | site 0x7688D3 base instance 同一性 | **解明 [確定(機序)]**（§8 改訂: base=NiDX9Renderer 本体・型 0xB3FD54=NiTriBasedGeom を静的特定） |
| V-173c | FUN_0053A720 内部エントリ 0x53AD60 caller | **解明 [確定]**（§9） |
| N1 | s75_anim.md 訂正ドラフト化 | **ドラフト化済**（§10） |
| N2 | ブレンド最終数式本文精査（0x6CBDF0/0x6CD0F0/0x6Cbee0/0x6CCBC0） | **解明 [確定]**（§11） |
| N3 | sneak/swim dispatch（0x51AC70 呼出 29 箇所のシャント解析） | **大部解明**（§12、書手のみ残＝V-190c） |
| N4 | NiControllerManager slot+0x1C=0x6C4E50 残整理 | **解明 [確定(観測)]**（§13） |
| §11-4D | v183 §11-4 訂正素材の before→after ドラフト化 | **ドラフト化済**（§15、新規 3 件追加） |

---

## §1 V-171a: inner 生成箇所 — **[確定(観測)] CreateDevice 本体 = FUN_0076A1C0（NiDX9Renderer::Initialize）内の IDirect3D9::CreateDevice（2 段ロード）**

### 1-1. 生成チェーン全体（実測）

```
0x498519 ─ call FUN_0076BD40（レンダラファクトリ）
  FUN_0076BD40:
    call 0x761DF0                      ; Direct3DCreate9 ラッパ
      → [0xB42150]=HMODULE d3d9 / [0xB42158]=Direct3DCreate9 proc / [0xB42154]=IDirect3D9*
    malloc(0xB00) via 0x401F00
    call 0x769BE0(alloc)               ; NiDX9Renderer ctor（vtable 0xA88EA4 書込 @0x769BFF）
    call FUN_0076A1C0(renderer, 15引数) ; = NiDX9Renderer::Initialize
      0x76A50C: mov eax,[0xB42154]; mov ecx,[eax]
      0x76A513: lea esi,[ebp+0x280]    ; &renderer[+0x280] を ppDevice 引数として push
      0x76A542: mov eax,[ecx+0x40]     ; IDirect3D9 vtable+0x40 = index 16 = CreateDevice
      0x76A545: call eax               ; ★ inner 生成点
      （失敗時リトライ 0x76A5B2、文字列 0xA89248
        "NiDX9Renderer::Initialize> Specified number of backbuffers was invalid - trying fewer backbuffers"）
      0x76A632: cmp dword [ebp+0x280],0 ; 成功ゲート（inner==NULL なら fail 文字列 0xA891AC）
```

- **inner（renderer[+0x280]）= IDirect3D9::CreateDevice の出力**として Initialize 内 0x76A545 で直接生成 [確定(観測)]。v183 §2-4「CreateDevice 呼出し 0xB42154 参照 21 site 全走査で非検出」は `call [reg+imm]` 単発走査の盲点（本件は **`mov eax,[ecx+0x40]; call eax` の 2 段ロード形式**）であった。
- 関数名証拠: 0xA893D8 = "NiDX9Renderer::Initialize> Invalid device ID specified - FAILING"（0x76A5FE 失敗パス）、0xA8941C = "Creation failed: Invalid 3D device type"。
- FUN_0076A1C0 の callers = **1 件（0x76BDDA、= FUN_0076BD40 内）**。FUN_0076BD40 の callers = **1 件（0x498519）**。
- 0x769BE0 ctor: `[esi]=0xA88EA4`（NiDX9Renderer vtable、v92 既記と一致）+ `[esi+0x604]=0xA88A3C→0xA88E04` サブ vtable 切替、0x25(37)要素×4B 配列確保（v129 §2 の 28 スロットテーブル系のホスト配列）。

### 1-2. s61 §1.1 の誤記再確認（本束での追加実測）

- 0x763DE0 は `mov eax,[0xB42160]; test; … Direct3DCreate9(0x20) → malloc(0x14) → call 0x775DF0(this,esi,[0xB28E00]) → [0xB42160]:=eax`（r1 全文デコード）。**CreateDevice ではなく「[0xB42160] オブジェクト（v183 §2-5: NiTArray<NiDX9AdapterDesc\*>）を構築するラッパ」**。callers = 0x4980E0/0x4980FD/0x49866C（[0xB34fc4] キャッシュ付き）/0x76A294（Initialize 内、アダプタ列挙経路）。s61「0x763DE0 = CreateDevice wrapper / [0xB42160] = IDirect3DDevice9*」は**再確認の上、誤記と確定**。
- [0xB42154]=IDirect3D9* は s61 のまま正しい（0x761DF0 が格納、参照 21 site は全て GetAdapterModeCount(+0x18)/EnumAdapterModes(+0x1C)/CheckDeviceFormat(+0x28)/GetCreationParameters 等または CreateDevice 経路）。

---

## §2 V-171b: inner の実行時 vtable スロット同定 — **[確定(観測)] 2 段ロード全走査で D3D9 スロット表を静的に確定。B13 依存は「vtable ポインタ実体値」のみ**

### 2-1. 手法

`[reg+0x280]`（renderer 経由）load **90 site** と `[reg+0xFF8]`（state 経由）load **25 site** を capstone デコードで全数抽出し、各 load から `mov vt,[inner]; mov fn,[vt+disp]; call fn/call [vt+disp]` まで追跡（`%TEMP%\opencode\v190_r8.py` / r9）。

### 2-2. 同定できた inner vtable スロット表（disp → D3D9 index = off/4）

| disp | idx | 実測呼出し site（代表） | 引数形状/証拠 | D3D9 メソッド |
|---:|---:|---|---|---|
| +0x04 | 4 | 0x77AC23, 0x76E342 | 1 引数 | AddRef |
| +0x08 | 8 | 0x76BA53, 0x77F864, 0x9A8BEE 等 12 | 1 引数 | Release |
| +0x0C | 12 | 0x40F1A5, 0x76B3FB | 戻り HRESULT `cmp eax,0x88760869`（D3DERR_DEVICENOTRESET） | TestCooperativeLevel |
| +0x1C | 28 | 0x762123 | `lea edx,[esi+0x284]`（D3DCAPS9）出力 | GetDeviceCaps |
| +0xA4 | 41 | 0x76BE35 | HRESULT test/jge | BeginScene |
| +0xA8 | 42 | 0x762619 | HRESULT test/jge | EndScene |
| +0xBC | 47 | 0x764DF6 | `lea edx,[esi+0x6c0]`（NiMaterial 構造体） | SetMaterial |
| +0xC0 | 48 | 0x764DB4 | `lea edx,[esp+0x20]` out 引数 | GetMaterial |
| +0xE4 | 57 | 0x4A1724/0x4A175E/0x4A1998（renderer グローバル [0xB3F928] 経由）, 0x77FE84（state 経由） | `push 0xAE`（D3DRS 定数）×3 引数 | SetRenderState |
| +0x104 | 65 | 0x77B2E8（state） | 2 引数 | SetTexture |
| +0x12C | 75 | 0x4A1742, 0x4A19B0 | `lea edx,[edi+0xdc]` | GetRenderState 系（75） |
| **+0x144** | **81** | **0x767199, 0x767491** | **3 引数**（[edi+0x38]/[edi+0x34]/[edi+0x3C]） | **DrawPrimitive** |
| **+0x148** | **82** | **0x767153, 0x767731, 0x76960D, 0x76985A, 0x7698E6** | **6 引数**（vidx/prim 調整込み・push 6 個実測） | **DrawIndexedPrimitive** |
| +0x16C | 93 | 0x801B82 | 3 引数（定数データ） | SetVertexShaderConstantF 系 |
| +0x190 | 100 | 0x766734, 0x766DD8 | 4 引数（vb/offset/stride） | SetStreamSource |
| +0x1A0 | 104 | 0x766757, 0x766DF8 | 1 引数 | SetIndices |
| +0x1AC | 107 | 0x77B112（state、tail-jmp フォワード） | 2 引数 | SetPixelShaderConstantF 系（v183 既記を維持） |
| その他 | — | +0x34/+0x64/+0x88/+0x114/+0x14C/+0x150/+0x164/+0x170/+0x178/+0x180/+0x188/+0x1B4/+0x1BC/+0x1C4/+0x1D8 | state/renderer 帯のフォワード（r8 全列挙） | 未名（需要時同定可） |

- **v183 §2-4 の結論訂正（新）**: 「DIP/DP は実行時 vtable ロード経由 [推定]、静的同定不能」→ 正: **呼出しは `mov fn,[vt+disp32]; call fn` 2 段形式で、静的に全スロット同定可能だった**。DIP(+0x148) は描画ループ内で 6 site、DP(+0x144) は 2 site。
- **v183 §2-2 の index 表訂正（新）**: +0x148=DrawPrimitive(82)/+0x14C=DIP(83) は誤り。実測の引数形状では **+0x144=DrawPrimitive(81・3 引数)/+0x148=DrawIndexedPrimitive(82・6 引数)/+0x14C=DrawPrimitiveUP(83)**。
- **訂正注記（追記・m_14–m_17 の vtable 归属実測に基づく）**: (1) idx 列の先頭 4 行（+0x04/+0x08/+0x0C/+0x1C）は offset 十進表記の誤りで正 idx = **1/2/3/7**、+0x16C の正 idx = **91**。(2) ラベルは D3D9 実 vtable の index 順で読み替える必要がある（引数形状を正とする）: **+0x12C = SetScissorRect 相当（1 引数、real idx 75 → 実 vtable で idx73 相当）**（「GetRenderState 系」は誤り候補）、**+0x16C = CreateVertexShader 相当（2 引数 @0x801B82 実測）**、**+0x1AC = SetPixelShader 相当（1 引数: `push 0; push eax` @0x77B118 実測）**、**SetPixelShaderConstantF の実スロットは +0x1B4（NiD3DHLSLPixelShader slot13 = FUN_007835D0 経由、§3-4）／SetVertexShaderConstantF は +0x178（NiD3DHLSLVertexShader slot11 = FUN_007839C0 経由）**。v183 の「+0x1AC = index 111」は算術誤り（0x1AC/4 = 107）。(3) +0x0C の比較値 0x88760869 は D3DERR_DEVICELOST/DEVICENOTRESET 系（D3D9 定数隣接）。
- B13（実機 .data ダンプ）依存は「inner の vtable ポインタの実体アドレス（COM オブジェクト内）」のみ。**スロット使用の一覧は本束で全て静的に確定** → V-171b は [確定(観測)] でクローズ。

---

## §3 V-171c: BSShaderPPLightingProperty slot38/39 描画パス設定本体 — **[確定(骨格)] flag デコード → シェーダテーブル lookup → パスビルダ帯分岐**

### 3-1. 両関数の共通骨格（0x855240 本文 200 命令 / 0x85AE20 本文 200 命令 実測）

1. **[this+0x1C] フラグワードの bool 展開**: bit0/1/2/3/4/5/8/0xA/0xE/0xF/0x10/0x11/0x12/0x14/0x18/0x20/0x40000 を seta で約 18 個のスタック bool（+0x12..+0x4C 範囲）へ展開。`[0xB42EAC]`（u16）と 3 の比較を併走。
2. **派生クラス判定**: vcall slot1（GetType）→ GetType 鎖を [type+4] で歩き **0xB468C8 / 0xB478B0 / 0xB468BC**（SpeedTree 系派生 type node、v183 §3-2 の SpeedTreeShaderPPLightingProperty 帯）と照合 → 3 bool。
3. **シェーダテーブル lookup**: `[this+0x1C]>>28`（bits28-31 = テーブル index）を引数に **FUN_007B4280 = `return [index*4 + 0xB42F54]`** → `[entry+0x118]` を読み出しデスクリプタへ格納。※ 0xB42F54 は v129 §2 の 0xB42EC0 系と**別の 16 エントリテーブル**（新事実）。
4. **ジオメトリ対応**: arg1 = geometry。`FUN_00707530(geometry, idx)` = **NiGeometry::GetTexture（idx≥10 なら 0、配列 [+0x9C]）**、`[arg1+0xB4]→[+0x24]` チェック。`FUN_00405760/0x405AD0` = **NiString getter（[this+0xAC]/[this+0x100]、InterlockedIncrement AddRef 付き）**、`FUN_008AA390` = **float ベクトル比較（fucompp ×N）**、`FUN_007016A0` = NiPointer Release ヘルパ。
5. **パスビルダ帯へ分岐**（call 全数センサス）: **0x854E90 / 0x854CD0 / 0x853580 / 0x852150 / 0x853720 / 0x853970 / 0x853DC0 / 0x853F80**（＋補助 0x7ED600/0x7ED2A0/0x7ED4B0 = シェーダ/テクスチャ解決系）。v183 §3-4 の「0x852150/0x853580/0x854CD0/0x854E90 帯へ分岐」を本束で call サイト実測により確定。

### 3-2. slot39（0x85AE20）固有

- SEH 関数。**capability ゲート**: `[0xB43074]==0 && [0xB43108]!=0 && ([0xB42F40]&0x20) && [0xB42F48]>=2` → 本判定式は **v183 §3-3 slot24 の能力判定と完全同一**（実行時 capability 反映）。`[0xB44F04]` float → fistp で整数化（カウント）、`[0xB42F3E]` byte / `[0xB42F48]` dword をデスクリプタへ複写。

### 3-3. 判定と残点

- slot38 = **描画パス設定（非 SEH・フラグ＋テーブル lookup＋分岐）**、slot39 = **同 SEH 版（capability ゲート＋テクスチャ名解決 0x405760 併用）** [確定(観測)]。v183 §3-4 の推定「パス設定関数が SM3 シェーダへ反映」は**構造（入口）を確定**。
### 3-4. 追記: ライト供給元と定数転記層（m_4–m_18 実測 — V-190a を大幅縮小）

**型ノード（CRT 登録スタブの静的特定、B13 不要）**: NiLight = **0xB3FD14**（スタブ 0xA09F1A、parent 0xB3FA88）/ NiDirectionalLight = **0xB3FCFC**（スタブ 0xA09EBA、parent=NiLight）/ BSShaderAccumulator = **0xB42CEC**（スタブ 0xA10D2A）。GetType stub: 0x71A490 / 0x719760 / 0x7ABE30。

**供給元（アタッチ/デタッチ）**:

| 要素 | 同定 | 証拠 |
|---|---|---|
| レンダラ側ハンドラ | **NiDX9Renderer vtable slot47 (+0xBC) = FUN_00767810**（IS-A NiLight 判定 → 本体へ） | rdata 格納先 0xA88F60（rdata 単一セクション走査） |
| デタッチ | **FUN_00776A30**: `[light+0x104]`=レコード、`[rec+0x6C]`=スロット index → bitmask `[mgr+0x40+(idx>>5)]` の bit クリア、`vcall [mgr+0x20]->vt+0xD4([rec+0x6C],0)` で解放通知、`[light+0x104]:=0`、free | decompiled L622818-622854 |
| アタッチ | **FUN_007C6AE0**: 既存探索 0x7C6230 → 無ければ **0x220B レコード（FUN_007D6060 ctor）**生成、`FUN_007C16B0` 登録、`[rec+0xF4]` 時に **`FUN_007C23C0([0xB43104] レンダラ mirror, 0x16)` + `vcall [FUN_007D6FE0()]+0x6C`** でミラー通知 | decompiled L683546-683610 |
| 重複排除 | FUN_007C7DC0（list [obj+0xE8] 走査 → 一致時 0x7C77C0） | 同 |
| シーン再帰列挙 | **FUN_007C7EF0**: 子配列 [obj+0xB0]（count +0xB6）走査、子 IS-A NiLight → mode0=0x7C6AE0 / mode1=0x7C7DC0、非該当なら vtable[+8] 有子を再帰（外部 caller = FUN_007C7F90） | decompiled L684514-684553 |
| クラス台帳 | `NiTPointerMap<NiLight*, LightEntry@NiDX9LightManager>`（tsv 行405）/ `NiTMapBase`（行242）→ **NiDX9LightManager / LightEntry 実在**（本体 vtable 行は台帳外） | tsv |

**定数転記の受け口（全 vtable 归属で確定 — rdata 単一セクション走査）**:

| 関数 | vtable 归属 | inner forward（disp → 実 D3D9 相当） |
|---|---|---|
| FUN_0077B000 | NiDX9RenderState **slot25 (+0x64)** / NiD3DRenderState slot25（二重） | +0xE4 SetRenderState（v183 のキャッシュ層本体） |
| **FUN_0077B140** | NiDX9RenderState **slot50 (+0xC8)** / NiD3D 双方 | **複合 flush**: +0x104(SetTexture) / +0x170 / +0x164(`[+0x1004]:=2`=SetFVF 相当) / +0x15C(`[+0x100C]:=0`=SetVertexDeclaration 相当) を 1 関数で順次適用 |
| FUN_0077B610 | NiDX9RenderState slot52 (+0xD0) | +0x114（3 引数、SetSamplerState 相当 [推定]） |
| FUN_0077F990 | NiDX9RenderState slot4 (+0x10) / NiD3D 双方 | +0xE4 SetRenderState（条件付き派） |
| FUN_00782E70 | **NiD3DPixelShader slot12 (+0x30)** | +0x1BC / +0x1C4 = SetPixelShaderConstantB 相当（3 引数） |
| FUN_007830C0 | **NiD3DVertexShader slot10 (+0x28)** | +0x188(SetStreamSource) / +0x180(VS 定数 B 相当) |
| **FUN_007835D0** | **NiD3DHLSLPixelShader slot13 (+0x34)** | **+0x1B4 = SetPixelShaderConstantF**（idx 109） |
| FUN_00783400 | NiD3DHLSLPixelShader slot12 (+0x30) | PS 定数系 |
| **FUN_007839C0** | **NiD3DHLSLVertexShader slot11 (+0x2C)** | **+0x178 = SetVertexShaderConstantF**（idx 94） |
| FUN_007837F0 | NiD3DHLSLVertexShader slot10 (+0x28) | VS 定数系 |

- **合成側エントリ**: **HDRShader（vtable 0xA8FFAC・47 スロット・COL 0xACF574・台帳行743）slot42 (+0xA8) = FUN_007BDFC0**: グローバル定数 `{1,1,0,0}@0xB2C774` 初期化 → renderer [0xB3F928] vcall slot31(+0x7C) と param3 樹 vcall +0x4C/+0x50 の**比率（ビューポート/RT サイズ）算出** → stack に `{0,1,1,1}` ベクトル群 → **FUN_007C23C0（7 値分解合成）×3 回**（0x7BE102/0x7BE214/0x7BE755、引数 [0xB43104]/[0xB42F50]/[0xB43075]）→ 各 getter vcall → **inner forward（0x7BE1F4/0x7BE322: +0x114）で適用**。callers=0（vtable 主導）。
- slot38/39 からの分岐先（0x852150/0x853580/0x854CD0/0x854E90 ほか 8 関数）は **FUN_007E2370 への technique ID 登録**（desc: u16 id = 0x10/0x18/0x33/0x160…、count×dword リスト複写、パスカウンタ +1）までを実施（§3-1 の詳細はこの通り）。

**新残点 V-190a（縮小・中）**: NiLight/0x220B レコードの**フィールド（色/方向/減衰）を float 配列へ詰め HLSL PS slot13(+0x34)/VS slot11(+0x2C) を呼ぶ中間合成関数**の本体のみ未特定。次手: (1) HLSL slot13/11 の vcall サイト（2 段ロード `mov r,[shader]; mov r,[r+0x34/0x2C]`）の caller 走査、(2) FUN_007D6060（0x220B ctor）のフィールド書込読解。

- （旧）**新残点 V-190a（中）**: パスビルダ帯 8 関数（0x852150 等）の「シェーダ定数へライト転記」最終段の深掘り（O1-1 のライト値供給元の最後の未解明段）→ **上記追記により縮小**。

---

## §4 V-171d: SkyShader vtable と天候遷移駆動関数 — **[確定(観測)] vtable 両クラス確定＋駆動関数 = FUN_005422F0**

### 4-1. vtable（cache/vtable_index.tsv 行1227/1228 と binary 実読み一致）

- **SkyShader vtable = 0xA8F654（42 スロット、vtable-4 = COL 0xACF514 を実測確認）**。主 slot: 0=0x7BD3B0（dtor）/ 1=0x7BA950（GetType 相当）/ 6=0x7BDAC0 / 8=0x7790B0 / 10=0x8052A0 / 12-15,18-21,24,26-29=0x77Axxx 帯（シェーダ実装）/ 38,40=0x977C50（報告スタブ）/ 41=0x7AEF60（scalar deleting dtor）。
- **SkyShaderProperty vtable = 0xA903D4（26 スロット、COL 0xACFC3C）**。slot1=0x7C5260、slot13/18=0x68F970（共有スタブ）、slot19=0x73DAA0（NiShadeProperty 層）、slot24=0x7B22E0 / slot25=0x7B22F0（+0x58 系 getter、BSShaderPPLightingProperty slot25 と同型）。
- v183 §5-3 の型登録 stub（0xA110B6/0xA1286）から vtable への接続は台帳で完結。

### 4-2. 天候遷移駆動関数

- fWeatherTransMax/Min/Accel は .rdata 文字列（0xA56FD8/0xA56FEC/0xA57000）を **CRT 登録 stub（0x9E5E40/0x9E5E70/0x9E5EA0）** が `fld [default] → push name → thiscall 0x41BAE0` で登録。**登録先は固定 Setting グローバル 0xB36628 / 0xB36630 / 0xB36638**（default 定数: 0xA41304 / 0xA34BA0 / 0xA46B10）。fWeatherFlashDuration → 0xB36640。
- abs32 参照は登録 stub ＋ atexit ラップ（0xA1D3xx）＋ **0x5424ED / 0x5424F3 / 0x54252E（いずれも FUN_005422F0 内）** のみ。0xB36640 参照は 0x5431A4。
- → **FUN_005422F0 = 天候遷移駆動関数（fWeatherTransMax/Min/Accel を集計する唯一の実行時コード）** [確定(観測)]。0x5431A4 帯（FUN_00543180 候補）= フラッシュ演出系。
- **新残点 V-190b（小）**: FUN_005422F0 本文の状態機械（遷移速度積分・T1/T2 フェード構造）の深掘り。

---

## §5 V-171e: 通常時の NIF→NiNode ツリー組立エントリ — **[大部解明] NiNode ctor = 0x70B780（callers 118＝通常経路は分散エントリ群）**

### 5-1. NiNode ctor の特定

- NiNode vtable = **0xA7E38C（39 スロット、台帳行996）**。imm32 書手は **0x70B7C3（FUN_0070B780）/ 0x70B840（FUN_0070B810 = コピー系）** の 2 件のみ。
- **FUN_0070B780 = NiNode ctor** [確定]: SEH、base ctor 0x708450（NiObjectNET）、name 配列 [+0xAC] へ 0x4B2D30、`[+0xC0]=[+0xC4]=[+0xC8]=0`、`[+0xBC]=0xA7E378`（サブ vtable、二重 vtable パターン）。

### 5-2. callers = 118 件の性格づけ

- 既知経路（v183/v173）: 0x667BE0（PlayerCharacter 1/3 人称再構築）、0x53B3F0（Atmosphere.nif）、0x572850（FloatingText tag）、0x4BEE70 系 Sky 帯。
- **通常経路**: TES フォーム帯（0x40-0x59xxxx）に**約 70 サイトが分散**（FUN_00436880 / 0x441970 / 0x44A7D0 / 0x4826F0 / 0x4CA7A0 / 0x4D58B0 / 0x53C830 / 0x544EA0 / 0x5640E0 / 0x592390 等）。すなわち「単一エントリ」ではなく **各フォームの 3D 構築関数群が NiNode ctor を直接呼ぶ分散型** [確定(観測)]。s61 §7-3 の回答（0x448E20=テスト専用 / 0x42C8E0=汎用 reader）は維持。
- **NiStream**: vtable = **0xA7EA1C（21 スロット、台帳行1114）**。slot1-6/8/16 の direct caller は各 1 件（0x6F9980 / 0x700790 / 0x494F90 / 0x6D8B50 / 0x6F9E90 / 0x6F9D90 系）で、**Load 系スロットは virtual 主導**のため単体静的特定は不能。slot8(+0x20)=0x713F00 は pool-malloc（0x401F00）呼出し＝ブロック確保系。
- **新残点 V-190d（低）**: NiStream::Load スロットの実行時 vtable 同定（inner と同型の 2 段ロード走査で可能）＋ slot1-6 の各 1-caller 宿主の役割確定。

---

## §6 V-171f: s61 §1.2 件数データの実メソッド再対応 — **[確定(観測)] s61 の表は全項目 Havok/Ni エンジン帯の vtable dispatch 件数**

`call dword [reg+disp]`（disp8 mod=01 / disp32 mod=10、rm≠4/5）を .text 全域でセンサス（`v190_r20.py`）:

| disp | s61 記載メソッド/件数 | 実測件数 | サイト帯 | 実体 |
|---|---|---|---|---|
| +0x24 | Clear 121 | **121** | 100% 0x8B-0x8E 帯 | Havok/Ni エンジン vtable dispatch |
| +0x28 | BeginScene 115 | **113** | 100% 0x89-0x8A 帯 | 同上（差 2 はスキャン方式差） |
| +0x2C | EndScene 21 | **21** | 100% 0x89-0x8F 帯 | 同上 |
| +0x30 | Present 8 | **8** | 0x8A97D2(筆頭)含め 0x8A-0x93 帯 | 同上（s61 §1.3 の「Present サイト」も誤認） |
| +0x4C | SetTransform 2 | 2 | 0x4FC0D0 + 0x8A9E32 | TES 帯 1 + エンジン 1 |
| +0x58 | SetRenderState 7 | **6** | 100% 0x8A-0x91 帯 | エンジン |
| +0x60 | SetSamplerState 7 | 6 | 100% エンジン帯 | エンジン |
| +0x64 | SetTexture 5 | **5** | 100% 0x8D-0x92 帯 | エンジン |
| +0x74 | DrawPrimitive 6 | 7 | 100% 0x8A-0x8C 帯 | エンジン |
| +0x78/+0x90/+0x94/+0xA8/+0xBC/+0xC8/+0xD0/+0xD4/+0xE4 | DIP 0 / VS 1 / PS 1 / VSC 2 / PSC 2 / SS 1 / SI 1 / SV 2 / SetLight 0 | **全て 0** | — | `call [reg+imm]` 形式では不在（s61 の 1/1/2/2/1/1/2 件は別形式スキャン由来） |

- **結論**: s61 §1.2 の 16 種件数表は「D3D9 メソッド件数」ではなく **0x88-0x93 帯（Havok/Phantom・Ni エンジン）の vtable dispatch 件数** [確定(観測)]。s61 §2.4「BeginScene >> EndScene からマルチパス示唆」も根拠喪失。**実 D3D9 呼出はすべて inner 経由の 2 段ロード（§2-2 表）のみ**。

---

## §7 V-173a: FUN_004dc270 入力 context の実クラス名 — **[確定(機序)] 入力 = NiNode。処理 = TESObjectExtraData 探親解決**

decompiled 全文（L173338-）と v173 §4-2 のデコードを突合:

```
FUN_004dc270(x):
  player_node = FUN_00660110(1)          ; g_thePlayer の 1 人称ノード
  loop:
    if x == player_node: return g_thePlayer (DAT_00B333C4)
    count = u16 [x+0x14]                  ; m_children.u16 size
    for i = count-1 .. 0:
      elem = [ [x+0x10] + i*4 ]           ; m_children.data[i]（素のポインタ配列）
      if elem isin GetTypeChain == &DAT_00B35ACC (TESObjectExtraData):
        form = [elem+0xC]                 ; TESObjectExtraData payload = TESForm*
        base = vcall form+0x170 (GetBaseForm)
        if base.formType != 0x22('"'=AMMO): return elem
        else: return FUN_004dc270([x+0x1C])   ; 親ノードへ再帰
    x = [x+0x1C]                          ; m_parent（兄弟/親チェーンを昇る）
```

- **入力 class = NiNode**: +0x10/+0x14 は NiTArray（{data, u16 size}）レイアウトそのもの、+0x1C = m_parent、子供に TESObjectExtraData（NiExtraData 系、payload +0xC = TESForm*）が付く構造と完全整合 [確定(機序)]。
- **引数供給系の実体**（r14）: FUN_004806E0 / FUN_008AFCE0 = `{+0x10 rel32, +0x18 tag(1|2)}` ハンドルオブジェクト → GetType 鎖照合（&DAT_00BA7D2C）→ **FUN_00452A60 = `return [wrapper+8]`**。FUN_00446A10 = `[param+0x34]` ゲート → FUN_004D4E30（[0xB35C24] グローバル / vcall+0x88 / FUN_00889CD0 経由解決）。→ 「ハンドル解決 → オブジェクト（NiNode）→ FUN_004dc270 で extra data 探索」の全チェーンを確定。
- v173 §4-2 の「+0x10 = 素のポインタ配列 / +0x14 = u16 / +0x1C = next」は正しく、**クラス名 = NiNode** を補完 [訂正完了]。

---

## §8 V-173b: site 0x7688D3 base instance 同一性 — **[確定(機序)] base = NiDX9Renderer 本体インスタンス。型 0xB3FD54 = NiTriBasedGeom（静的特定・B13 不要）**

### 8-1. 決定証拠 A: FUN_00768890 の全 5 call site で `ecx`（this）= 宿主の this レジスタ

```
call 0x768890 (E8) 全 5 site（%TEMP%\opencode\v190m_3_out.txt）:
 0x768EDF (slot44 内):    push 0 ; push edi(geometry) ; mov ecx, ebp   ← ebp = slot44 の this
 0x76944D (slot88/89 内): push 0 ; push ebp(geom系)  ; mov ecx, ebx   ← ebx = 宿主の this
 0x769537 (slot90 内):    push ebx; push edi; push 0 ; push esi ; mov ecx, ebp
 0x76975B (slot91 内):    push 0 ; push esi(geom系)  ; mov ecx, ebp
 0x7699D2 (0x7699xx 内):  push edi ; push ecx        ; mov ecx, esi
```

- slot44 宿主 FUN_00768E90 の prologue `mov ebp, ecx`（= this = NiDX9Renderer）→ **ecx に渡るのは geometry ではなくレンダラ本体**。stack 第1引数（edi/esi）が geometry。
- → v173 §2-1(c)/旧 §8 の「宿主の第1引数オブジェクトを this として」は decompiled 表示の読み違い → **訂正**（0x769537 の `push ebx; push edi` は callee(ret 8) 外側の早期 push＝ホストフレーム残置 [観測・注記]。残 4 site は 2 stack 引数で ret 8 と整合）。

### 8-2. 決定証拠 B: FUN_00768890 本文との多重整合

decompiled L610213-610248（this=param_1 / arg1=param_2=geometry / arg2=param_3）:

1. `param_2 + 0xb4/0xb8/0xbc` 逆参照 = geometry メンバ（NiTriBasedGeom メッシュ系）→ arg1 = geometry ✓
2. `param_1[0x1b9]`（= **this+0x6E4**）/ `(char*)param_1+0x5c9` = **レンダラ ctor 0x7641A0 が書くメンバ**（`mov [esi+0x6e4],ebx` @0x76420D / `mov byte [esi+0x5c9],bl` @0x7641F5 実測）→ **this = NiDX9Renderer** ✓✓
3. `(**(*param_1 + 0x54))()` = **0x7688D3 の vcall = this（レンダラ）の slot21(+0x54) = 0x769F20**（v159 既知の flags getter）✓✓（NiTriBasedGeom vtable 0xA7F7FC の slot21 = 0x722C60 = 別物）
4. `IsOfType(&DAT_00b42858=NiD3DShaderInterface, param_2[0x2f]/*geometry+0xBC*/)` = arg1 側のシェーダ判定 ✓（v173 記載の "[arg1+0xBC]" は arg1=geometry として正しい）
5. 戻り値ビット `{2}`（v159 の +0x54 getter 実測）= renderer flags と一致 ✓

### 8-3. 決定証拠 C: 型ノード 0xB3FD54 = **NiTriBasedGeom**（静的特定・B13 不要）

- slot44 の arg1 は GetType 鎖を 0xB3FD54 と照合（`for (puVar6=GetType(); puVar6; puVar6=puVar6[1]) if (puVar6==&DAT_00b3fd54)`）。
- **型名は .bss の実体を読まずに特定可能**: 0xB3FD54 の CRT 登録スタブ = **0xA0A000**（`push 0xB3FD5C(parent); push 0xA7F898(name); mov ecx,0xB3FD54; call 0x70E220`）、**name@0xA7F898 = "NiTriBasedGeom"**、GetType stub = **0x7226B0**（`mov eax,0xB3FD54; ret`）。直前の ctor が vtable **0xA7F7FC**（台帳行1131 NiTriBasedGeom・slot1(+4)=0x7226B0 と一致）を書く。
- → **slot44 = 「arg1 が NiTriBasedGeom（派生含む）である場合にのみ実行されるジオメトリ提出/可視判定系レンダラ仮想関数**」（内部で 0x768890 = 描画可否判定 → FUN_00778DB0 / FUN_00768580 / FUN_00768470 へ）[確定(観測)]。

### 8-4. 判定

- **base（0x7688D3 の this）= NiDX9Renderer インスタンス本体**（slot44/88/89/90/91 宿主の this と同一。実体は [0xB3F928] 主レンダラ／[0xB43104] mirror のいずれか＝実行時にその vtable を叩いた側）。v173 の [仮説(高)]「レンダラ本体か派生か」→ **「レンダラ本体（派生オブジェクトではない）」に確定**。
- slot44/88-91 の E8 direct caller = 0 件（vtable 経由のみ）＝ v173 の観測を再確認。同一性は外部 caller 走査ではなく **call site の ecx 設定＋本文 members で確定**した。
- 証拠: `%TEMP%\opencode\v190m_1..m_3*.py/_out.txt`（type stub デコード・decompiled 全文・5 call site 全文脈）+ `v190_r11_out.txt`（0xB3FD54/0xB42858 全参照）。

---

## §9 V-173c: FUN_0053A720 内部エントリ 0x53AD60 の caller — **[確定] 0x40F5A3（1 件）**

- `callers(0x53AD60) = 0x40F5A3`（1 件）。`callers(0x53AC60) = 0x40E759`（v173 既記を再確認）。両エントリとも FUN_0040D4D0 初期化帯の後続で、0x53AC60（INI 読込ヘルパ）→ 0x53AD60（INI 書込ヘルパ）の対称構造を確定。v160/v173 の結論に影響なし。

---

## §10 N1: s75_anim.md 訂正ドラフト（反映は親セッション承認要 — s 成果物は無編集）

**ドラフト化**: s75_anim.md の以下を before→after で訂正（本文は v187 C1-C4 の確定内容に準拠）。

| # | before（s75 現行） | after（訂正ドラフト） |
|---|---|---|
| 1 | [B1] NiControllerManager vtable at 0xA79804 (**16 slots**) | **30 slots**（cache/vtable_index.tsv と一致） |
| 2 | [B1] slot+0x18=0x6C5CC0 **Update** | **Clone 系ファクトリ**（new 0x80 + ctor 0x6C5520 + 引き写し 0x6C5A10） |
| 3 | [B1] slot+0x28=0x6C4DA0 **AddSequence** | **SaveBinary**（0x716050 ベース + +0x6C bool + +0x46 カウント + +0x40 配列 SaveObject + +0x7C パレット） |
| 4 | [B1] slot+0x30=0x716140 **Activate** | **Save（テキスト）= NiTimeController::Save 継承** |
| 5 | （未記載）slot+0x54 = 0x6C4200 | **NiControllerManager::Update（実体）**（Active bit3 ゲート、+0x4C/+0x54 のシーケンス列を 0x6CA950 へ毎フレーム呼出） |
| 6 | [B2] NiControllerSequence vtable (**12 slots**) | **19 slots**（slot6=+0x18 Clone=0x6C87A0 / slot7=+0x1C LoadBinary=0x6C8E40 / slot9=+0x24 IsEqual=0x6C77A0） |
| 7 | [B2] slot+0x1C=0x6C8E40 **Activate** | **LoadBinary**（版数 0x0A010068 ゲート） |
| 8 | [B2] slot+0x18=0x6C87A0 **Update** | **Clone 系ファクトリ**（new 0x68 + ctor 0x6C6550 + 0x6C70A0） |
| 9 | [B2] slot+0x24=0x6C77A0 **GetDuration** | **IsEqual**（各子 vt+0x24 再帰照合） |
| 10 | [B7] NiBlendTransformInterpolator: **handler 0x6CBCE0** / NiBlendFloatInterpolator: **handler 0x6E9030** | ハンドラは **NIF クラス登録ファクトリ**（0x6E9030 = named_funcs 上 CreateObject）。重み算出は 0x6C4200→0x6CA950→0x6C6DC0→0x6CBDF0/0x6CD0F0/0x6Cbee0 系 |
| 11 | （§C 未消化） | [C1] 解明（0x716140=Save）/ [C2] 解明（0x6C87A0=Clone、実更新 0x6C4200→0x6CA950）/ [C3] 大部分解明→**本束 N2 で数式確定** / [C4] 大部分解明（dispatch 読手確定、書手は V-190c） |

- vtable_index.tsv が正（NiBlend 系 4 件台帳化済）— s75 側の slot 数・命名を全面追従させるドラフト [ドラフト化済・未反映]。

---

## §11 N2: ブレンド最終数式本文精査 — **[確定] m_fNormalizedWeight 計算式・priority 抑制条件を全文実測**

### 11-1. FUN_006CD0F0 = 正規化重み計算本体（decompiled 全文 L… cap 120 行）

```
if (!(flags(+0xC) & 4)) return;            ; 再計算要求ゲート
flags &= ~4;
if (m_ucInterpCount(+0xE) == 1):           ; 単一
    unit[m_ucSingleIdx(+0xF)].m_fNormalizedWeight(+8) = 1.0f; return;
if (m_ucInterpCount(+0xE) == 2): FUN_006CC900(); return;   ; 2 体高速経路
; ---- multi ----
if (cache(+0x24) == -FLT_MAX):             ; 再集計
    [+0x24]=0; [+0x28]=0; [+0x2C]=0;
    for i in 0..m_ucArraySize(+0xD):
        u = units[i]; if u.interp==0 continue;
        w = u.m_fWeight(+4) * u.m_fEaseSpinner(+0x10);
        if (u.m_cPriority(+0xC) == minPriority(+0x10)): [+0x24]+=w; track max(u.ease)→[+0x2C]; n++;
        elif (u.m_cPriority == maxPriority(+0x11)):     [+0x28]+=w;
    if (n>1 && [+0x24]!=0) [+0x28]=0;      ; ★ priority 抑制: 最低 priority 群が優勢なら最高 priority 群を潰す
denom = [+0x24]*[+0x2C] + (1-[+0x2C])*[+0x28];
recip = denom<=0 ? 0 : 1/denom;
for i in units:
    if u.m_cPriority == minPriority: u.norm(+8) = u.weight * [+0x2C] * u.ease * recip;
    elif u.m_cPriority == maxPriority && [+0x28]!=0: u.norm = u.weight * (1-[+0x2C]) * u.ease * recip;
    else: u.norm = 0;
; ---- m_fWeightThreshold(+0x1C) 閾値 ----
if (+0x1C > 0):
    sum = Σ (u.norm != 0); each u.norm < threshold → u.norm = 0;
    if sum != 1: sum<=0 → all zero : else scale all norms by 1/sum;   ; 再正規化
```

→ **m_fNormalizedWeight 計算式・m_bOnlyUseHighestWeight 相当の priority 抑制・weight threshold 閾値処理の全文を確定** [確定]。

### 11-2. 他 3 関数

- **FUN_006CBDF0（single 経路）**: ManagerControlled(+0xC bit0) → time=+0x20。time==±FLT_MAX センチネル（_DAT_00A79F00）なら出力を -FLT_MAX×3（param_4[0]/[4]/[7]）に設定し 0。interp(m_pkSingle=+0x18) の **vcall +0x4C(time)** 成功時、キャッシュ済み Transform（param_4[3]/[4]）と FUN_00715340/FUN_00471430 で合成し 1、失敗時 0 [確定]。
- **FUN_006CBEE0（multi Transform 合成）**: per-unit time = ManagerControlled ? unit[5]（=m_fEaseSpinner）: param_2 → vcall interp+0x4C → スカラ部は u.weight(=piVar1[2]) で加重和（local_80/local_84 は失敗 unit の減算分母補正）、回転はクオタニア成分を FUN_0072F930(スケール) で加重和＋**内積が閾値 0xA2FAAA8 未満なら FUN_00714CC0 で半球反転**、scalar spin 積和 local_7c [確定(観測)]。
- **FUN_006CCBC0（ターゲット適用）**: FUN_00700750(target) でリセット → FUN_0055E000 lookup → per-unit vcall **+0x38**（GetTransform 合成）→ m_pkSingle(+0x18) フォールバック [確定(観測)]。
- 残: NiBlendAccumTransformInterpolator（0xA79584、40 スロット）固有の積算パスは本束で未個別追跡 → 残点（低、N2 付録）。

---

## §12 N3: sneak/swim dispatch（0x51AC70 呼出 29 箇所のシャント解析） — **[大部解明] 読手全分類・判定形を確定、書手のみ残**

### 12-1. 29 callers の全分類（enclosing 実測）

| 帯 | 関数 | 件数 | 性格 |
|---|---|---:|---|
| KF import/検証 | FUN_0043BEB0(2), 0x472030(2・既知検証), 0x4723A0(2), 0x472C40(1), 0x474070(3), 0x474E90(3), 0x476380(1), 0x476D10(2) | 16 | KF 名合成/インポート検証（v187 C4 既記） |
| **実行時 decision** | **FUN_005EC180(1), 0x5EFFD0(2), 0x5F43E0(1), 0x5FCAB0(7), 0x634D10(1), 0x63CFB0(1)** | 13 | Actor 状態分岐 |

### 12-2. レシーバ構造と判定形（実測）

- 全 decision サイト共通: **`ecx = [form+0x68]`（TESAnimGroup サブオブジェクト）→ call 0x51AC70（[subobj+8] packed byte 読出）**。
- 判定形: group∈{0x28..0x2A}（0x5EC1B2 `add eax,-0x28; cmp 2`）/ **group==0x1D(29)**（0x5F0025）/ group∈{0x14..0x27}（0x5F440E）/ **36B 属性テーブル 0xB102E0 lookup**（0x5FCC1A `lea eax,[eax+eax*8]; [eax*4+0xB102E8]`）/ **group==0x29(41)**（0x5FD7CE）。0x5F43E0/0x5FCAB0 系は vcall +0x2D4（Actor 系メソッド）経由でノードを取得してから group 読出。
- ついで同定: **0x51AC80 = ループタイプ 4-7 判定**（36B テーブル [entry+0xC] が 4/5/6/7 → 1）、**0x51ACC0 = ==5 判定**、**0x51ACF0 = サブオブジェクト +0x24/+0x28 配列 setter**（pool 0xB02020 経由）。

### 12-3. 残点

- **group id の書手（[form+0x68]+8 の書込み）は未特定**: 0x5E-0x60 帯の decompiled grep（`+ 0x70) =` / `+ 0x68) =`）0 件、0x51AC00 近傍に setter なし → KF import 帯（0x470xxx-0x477xxx）での計算時書込みの可能性が高いが未証明 → **新残点 V-190c（中）**。

---

## §13 N4: NiControllerManager slot+0x1C = 0x6C4E50 残整理 — **[確定(観測)] LoadBinary（SetTarget 候補は棄却）**

- imm32 0x6C4E50 全画像参照 = **1 件 = 0xA79820 = NiControllerManager vtable 0xA79804 + 0x1C（slot7）** [確定]。
- 本文実測（60 命令）: arg1(esi) に対して 0x715F40 呼出し → `[arg1+0x21C]` の vtable+4 ゲート（5 引数、戻り bool）→ **`[this+0x6C] := setne** → 0x712AE0(arg1) → **`cmp [arg1+0xD8], 0x0A010068`（NiStream 版数ゲート）** ≥ 時のみ 0x712A90 で読み出し → **`[this+0x7C]`（オブジェクトパレット）を 0x75FA90 へ構築** → ret 4。
- → **0x6C4E50 = NiControllerManager::LoadBinary**（+0x6C bool / +0x7C パレット読込、NiObject 構造の slot7(+0x1C)=LoadBinary 慣例と一致）。v187 N4 の「SetTarget 候補」は**棄却** [確定(観測)]。
- NiControllerSequence vtable 0xA79A34 の 19 スロットを全文列挙し、v187 C1/C2 の訂正（slot6=Clone / slot7=LoadBinary / slot9=IsEqual）と**三重一致**を確認。「真の GetDuration」は 19 スロット内に独立エントリなし（slot12(+0x30)=0x6CA440 等が候補だが本束では未深掘り＝残点(低)）。

---

## §14 v187 N1–N4 判定総括

| 残点 | 本束判定 | 主証拠 |
|---|---|---|
| N1 | **ドラフト化済**（§10、11 項目） | v187 C1-C2 訂正 + vtable 実読み |
| N2 | **解明**（§11、正規化式・priority 抑制・閾値処理を全文確定） | decompiled 4 関数全文 |
| N3 | **大部解明**（読手 29 全分類・判定形確定、書手のみ V-190c） | callers+enclosing 実測、0x5FCC1A テーブル lookup |
| N4 | **解明**（0x6C4E50 = LoadBinary、SetTarget 棄却） | 本文 60 命令 + imm32 配置 1 件 |

---

## §15 v183 §11-4 訂正素材の before→after ドラフト（s/v 系成果物は無編集 — ドラフトのみ）

既存 9 件（v183 §11-4 既載）＋ **新規 3 件**。

1. **s61_render.md §1.2（全面訂正・強化）**: before「D3D vtable 呼出件数（16 種メソッド）」→ after「**表は全て 0x88-0x93 帯 Havok/Ni エンジンの vtable dispatch 件数**（本束 §6 再実測: +0x24=121/+0x28=113/+0x2C=21/+0x30=8、全サイトエンジン帯）。D3D9 メソッド件数ではない」。
2. **s61_render.md §1.1**: before「0x763DE0 = CreateDevice wrapper、[0xB42160] = IDirect3DDevice9*」→ after「0x763DE0 は [0xB42160]（NiTArray<NiDX9AdapterDesc\*>）構築ラッパ。**CreateDevice 本体 = FUN_0076A1C0 内 0x76A545（IDirect3D9 vtable+0x40 2 段ロード、ppDevice=&renderer[+0x280]）**、レンダラファクトリ = FUN_0076BD40、ctor = 0x769BE0」（v183 訂正の実行点確定版）。
3. **s61_render.md §1.3**: before「0x8A97D2 = Present」→ after「FUN_008A9630 内 +0x30 呼出はエンジン帯 vtable dispatch（Present ではない）」（§6）。
4. **s61_render.md §4.1**: before「天候関連文字列: 未発見」→ after「多数存在（v183 §5-1）＋ **天候遷移駆動 = FUN_005422F0**（fWeatherTrans* は固定 Setting 0xB36628/30/38）」。
5. **s61_render.md 残件3**: before「BSShaderPPLighting」→ after「BSShaderPPLightingProperty（slot38/39 = パス設定 0x855240/0x85AE20、シェーダテーブル = [idx*4+0xB42F54]、パスビルダ帯 8 関数を特定）」。
6. **s61_render.md 残件6**: Portal/DMap 不存在（v183 のまま維持）。
7. **v129 §4-2**: before「inner は raw IDirect3DDevice9 ではない」→ after「**raw IDirect3DDevice9 互換で確定**。さらに CreateDevice 出力＝inner（ppDevice 直渡し）を特定し、DIP=+0x148(6 引数)/DP=+0x144(3 引数) を実測で確定」。
8. **s81 A4**: before「LODManager at 0xA47AA0」→ after「正名 TESTerrainLODManager」（v183 のまま維持）。
9. **s81 Task D 補足**: FUN_00450550 = zlib 解凍（v183 のまま維持）。
10. **【新】v183 §2-2 D3D9 index 表**: before「+0x148=DrawPrimitive(82) / +0x14C=DIP(83)」→ after「**+0x144=DrawPrimitive(81・3 引数) / +0x148=DrawIndexedPrimitive(82・6 引数) / +0x14C=DrawPrimitiveUP(83)**（引数形状実測、本束 §2-2）」。
11. **【新】v183 §2-4**: before「DIP/DP 検出不能・実行時 vtable ロード経由 [推定]、B13 依存」→ after「**2 段ロード形式（mov fn,[vt+disp32]; call fn）を走査すれば静的同定可能**。DIP 6 site / DP 2 site を特定、B13 依存は vtable ポインタ実体値のみ」。
12. **【新】v173 §4-2 (V-173a)**: before「入力 context の実クラス名は未特定」→ after「**入力 = NiNode**（+0x10/+0x14 = m_children NiTArray、+0x1C = m_parent）。FUN_004dc270 = TESObjectExtraData 探親解決（AMMO ベースフォーム時は親へ再帰）」。

---

## §16 新規残点（親セッション起票用）

| ID | 内容 | 優先度 |
|---|---|---|
| **V-190a** | （縮小済）NiLight/0x220B レコードのフィールドを float 配列へ詰め HLSL PS slot13(+0x34)/VS slot11(+0x2C) を呼ぶ**中間合成関数**の本体のみ。次手: HLSL slot13/11 の vcall サイト caller 走査＋FUN_007D6060（0x220B ctor）読解。※供給元（NiDX9LightManager アタッチ/デタッチ）と受け口（vtable 归属）は §3-4 で確定済み | 中 |
| **V-190b** | FUN_005422F0（天候遷移駆動）本文の状態機械深掘り（遷移速度積分・T 構造） | 小 |
| **V-190c** | TESAnimGroup packed field（[form+0x68]+8）の書手特定（KF import 帯 0x470xxx-0x477xxx 候補） | 中 |
| **V-190d** | NiStream::Load スロットの実行時 vtable 同定（2 段ロード走査）＋ slot1-6 各 1-caller 宿主の役割 | 低 |
| （解消・§8 改訂） | ~~V-173b: type node 0xB3FD54 の型名 B13 依存~~ → **NiTriBasedGeom として静的特定済（CRT スタブ 0xA0A000・name@0xA7F898）＋ base=NiDX9Renderer 本体を確定** | 解消 |
| （継続） | N2 付録: NiBlendAccumTransformInterpolator（0xA79584）積算パス | 低 |
| （継続） | N4 付録: 真の GetDuration 候補（NiControllerSequence slot12 = 0x6CA440 等） | 低 |

---

## §17 自己検証・遵守事項

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時）。**完了時再実行 → 4/4 PASS**（2026-10-29 追記: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 既記成果物との一致検査: NiDX9Renderer vtable 0xA88EA4 ✓（r7 ctor 実測）/ state[+0xFF8]=inner フォワード（v129 §4 既記）一致 ✓ / BSShaderPPLightingProperty vtable 0xA91384（v183 既記）は本束で不使用 ✓ / vtable_index.tsv: NiNode 0xA7E38C・SkyShader 0xA8F654・SkyShaderProperty 0xA903D4・NiStream 0xA7EA1C・NiControllerSequence 0xA79A34 を binary 実読みで全て照合一致 ✓。
- 制約遵守: 新規 = 本ノート 1 ファイル + `%TEMP%\opencode\v190_r1..r22.py`・出力ログ のみ。**docs/TODO/session-memory/plan/cache/既存 tmp/re 編集 0 件**、git なし、**ESM 本体未アクセス**、Ghidra 呼出なし。
- 中間スクリプト一覧（すべて `%TEMP%\opencode`）: v190_r1（0x763DE0/0x761DF0 全文+writers）/ r2（0x7641B8 host/0x76A294 ctx）/ r3（0x7623D0 捜査）/ r4（decompiled +0x280 writers）/ r5（[0xB42154] 21 ref 全 ctx）/ r6（0x76A4E0 CreateDevice 実測）/ r7（0x76BD40 factory/0x769BE0 ctor/success path）/ r8（inner forward 90+25 load 全文脈）/ r9（+0x144/+0x148 push 数・V-173b/c callers）/ r10（0x855240/0x85AE20 本文）/ r11+11b（E8 census/type node/key func heads/weather stubs）/ r12（SkyShader/SkyShaderProperty vtable+GMST 文字列）/ r13（NiNode ctor 特定/callers 118）/ r14（V-173a resolvers/call sites）/ r15（weather Setting refs/NiNode ctor 布局/resolver headers）/ r16（N2 4 関数 decompiled）/ r17（N3 callers 29+hosts）/ r18（FUN_004dc270 全文/N4/weather refs）/ r19（N3 setter grep/N4 0x6C4E50/Sequence vtable）/ r20（V-171f disp census）/ r21（NiStream vtable/ctor callers）/ r22（NiStream slot 捜査）＋ **追記分 m_1–m_18**（m_1: 0xB3FD54 ref-1 byte 分類/0x40F5A3 host、m_2: 型ノード name/parent+slot44/0x768890 decompiled、m_3: call 0x768890 全 5 site ecx 設定+0x6C4E50 本文、m_4–m_6: slot38/39 call graph+分岐 callers、m_7–m_9: パス設定先 4 関数+NiLight/大域ノード+ライト比較 3 host、m_10–m_12: renderer slot47/FUN_00776A30/アタッチ 0x7C6AE0・0x7C5F60、m_13–m_16: HDRShader 0x7BDFC0 hosts+vtable slot 判定、m_17–m_18: rdata 単一セクション格納先再走査+HDRShader 台帳行）。

（了 — v190_graphics_residuals.md / 15 項目全書き切り・訂正ドラフト 12 件・新規残点 4 件（V-190a 縮小・V-173b 解消を追記マージ、selftest 着手時・完了時とも 4/4 PASS））
