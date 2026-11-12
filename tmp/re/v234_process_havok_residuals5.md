# v234_process_havok_residuals5.md — process/Havok 残点第5束（V-228-1..3）

- 日付: 2026-11-01〜02 / 作成: opencode サブエージェント（TODO serial 227 / V227・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v228_process_havok_residuals4.md` §5 新規残点 V-228-1..3
- 手法: capstone 5.0.7（`v172_common.py` / `v217_util.py` 再利用）＋ `cache/{named_funcs,vtable_index,call_targets}`（参照のみ）＋ `docs/Oblivion_decompiled.c`（部分読取）
- 中間物: `%TEMP%\opencode\v234_*.py` / `v234_*_out.txt`
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v234_*` のみ。ESM 読取 0 回
- **VA 換算**: セクション個別変換の `imm32_refs3()` のみ使用（v228 §ヘッダ罠注記遵守）

---

## §1 item1（V-228-1）slot13 (setWorldAabb) の呼出し元 — **解明: 実呼出しは 1 件 `FUN_0088C600+0x93`（2命令 vcall 形・受信 = ahkWorld+0x64 の broadphase）／構築時は ctor がインライン**

### 証拠A: v228 の「vcall 0件」は `call [reg+0x34]`（FF /2）形のみの走査で、**2命令形を取りこぼしていた**（`v234_1b_out.txt` / `v234_1g_out.txt`）

- 汎用スキャン結果（本タスク・.text 全域）:
  - **E8 直接呼出し → 0 件**（`v234_1_out.txt`: `e8_callers(0x8E1160)` = 0）
  - **E9 jmp rel32 → 0 件**（`v234_1b_out.txt`）
  - **imm32 参照 of 0x8E1160 → 1 件のみ** = `.rdata 0xA9A744`（= vtable `0xA9A710` slot13 自身。関数ポインタテーブル・thunk 等への格納は**ゼロ**）
  - **FF /2 `call [reg+0x34]` → 2 件**（`v234_1_out.txt` 再確認: `0x8A7118` / `0x8AD0B4` = v228 検出済みの2件、いずれも別クラス＝誤検出で維持）。FF /4 `jmp [reg+0x34]` → 0 件
  - **2命令 vcall 形**（`mov r32,[obj]` → `mov r32,[vt+0x34]` → `call r32`）→ 152 件（どのクラスの slot13 でも成立し得る汎用形）。うち**受信オブジェクトが `[reg+0x64]` ロードに由来するもの → 1 件のみ**（`v234_1g_out.txt`: `load@0x88C67C → call@0x88C693`）。さらに `.text` 全域の `mov r32,[reg+0x64]` ロード全数に「64 バイタイ内に slot13 ディスパッチ」を要求する独立走査（`v234_1h_out.txt`）でも **1 件のみ（0x88C67C）**。

### 証拠B: その1件が本物の slot13 呼出しである（`v234_1d_out.txt` 全文 / `v234_1g_out.txt`）

`FUN_0088C600`（thiscall, ret 4 — 1 引数 = 入力ベクトル ptr）:

```
0x88C639  mov esi, ecx                  ; this
0x88C653  movaps xmm0, [eax]            ; 入力 vec
0x88C656  mov eax, [esi]                ; this->vtable
0x88C65C  mov edx, [eax+0x58]           ; slot22
0x88C65F  subps xmm0, [esi+0x50]        ; delta = 入力 − this+0x50
0x88C66A  movaps [esp+0x34], xmm0       ; delta 退避
0x88C66F  call edx                      ; world = this->slot22()
0x88C671  mov edi, eax
0x88C67C  mov ecx, [edi+0x64]           ; ← 受信 = world+0x64
0x88C67F  mov eax, [ecx]                ; vtable
0x88C681  mov eax, [eax+0x34]           ; slot13
0x88C684  lea edx,[esp+0x18]; push edx  ; arg3（hkArray ヘッダ {0,0,0x80000000}）
0x88C689  lea edx,[esp+0x28]; push edx  ; arg2（出力 vec バッファ）
0x88C68E  lea edx,[esp+0x3c]; push edx  ; arg1（= 先頭 push で最後に積む → delta = esp+0x34 と一致）
0x88C693  call ecx                      ; slot13(broadphase, delta*, out*, array*)
0x88C695  movaps xmm0, [esp+0x24]       ; 戻り後 esp 復元 = callee が 12B pop ⇒ ret 0xC と整合
0x88C69E  addps [esi+0x50],  xmm0       ; this+0x50    += 結果
0x88C6AC  addps [edi+0x280], xmm0       ; world+0x280  += 結果
0x88C6C5  addps [edi+0x290], xmm0       ; world+0x290  += 結果
```

- **引数形が slot13（`ret 0xC` = 3 引数: 入力 vec*/出力 vec*/int）と完全一致**（3 push・call 後の `movaps [esp+0x24]` が callee-pop 整合を示す）。
- **受信の同定**: `edi` = `this->slot22()` の戻り値で、`edi+0x64` / `edi+0x280` / `edi+0x290` の3フィールドは **ahkWorld ctor `FUN_0089A230` が書くものと完全一致**（`v234_1e_out.txt`）:
  - `0x89A6B5: movaps [esi+0x280], [edi+0x30]`（cinfo の aabb min → world+0x280）
  - `0x89A6C2: movaps [esi+0x290], [edi+0x40]`（cinfo の aabb max → world+0x290）
  - `0x89A6DE: call [0xB2FC80]`（= **broadphase factory `FUN_008E5020`**、imm32 参照はこれ1件のみ）→ `0x89A6E4: mov [esi+0x64], eax`
  - ⇒ `edi` = ahkWorld、`[edi+0x64]` = **hkp3AxisSweep（vtable 0xA9A710）**。
- **factory 経路**: `FUN_008E5020`（.data `0xB2FC80` に格納、唯一の使用 = ahkWorld ctor `0x89A6DE`）→ allocator `0x80` 確保 → ctor `FUN_008E4730`（`0xA9A710` 書き込み = `0x8E4747`）→ world+0x64。deleting dtor `0x8E6460` は vtable slot0（imm32 参照 = `.rdata 0xA9A710`）。

### 証拠C: 構築時の「内部呼出」はインライン展開（call なし）（`v234_1_out.txt` / `v234_1h_out.txt`）

- imm32 参照 of 符号反転マスク `0xA965C0`（slot13 本文が使用）の一覧に **`0x8E4905 = FUN_008e4730+0x1D5`** が存在。
- ctor `FUN_008E4730` の該当部: `movaps xmm2,[0xa965c0]; movaps xmm0,[eax]; xorps xmm0,xmm2; movaps [esi+0x10],xmm0; ... addps → movaps [esi+0x20],xmm0` — **slot13 本文と同一の「符号反転 → offset(+0x10) / scale(+0x20) 更新」数式を直接計算**（入力は aabb min/max ポインタ＝ctor 引数）。つまり構築時は setWorldAabb を call せずインラインで等価処理を行う。
- slot12（updateEntity `0x8E3A90`）も E8 呼出し 0 件（`v234_1_out.txt`）＝ slot13/slot12 とも直接呼出しは存在せず、vcall とインラインのみ。

### 証拠D: 呼出し元の文脈 — 「位置更新時のワールド原点シフト」

- `FUN_0088C600` は delta = 入力位置 − `this+0x50` を broadphase に渡し、**戻り値を `this+0x50` / `world+0x280`(aabb min) / `world+0x290`(aabb max) の3箇所に加算**（平行移動）。
- 呼出し元: `FUN_0088D260`（生定数 double `0.1428767293691635` で入力をスケール → `FUN_0088C600`。呼び出し: `FUN_004D4A20+0x1F8` / `FUN_004D5000+0x2A5`）、`FUN_008A7D50`（`this+0x50` からの差分を `this+0x90..+0x98`（3軸しきい値）と比較し閾値内なら `FUN_0088C600`。呼び出し: `FUN_004CA2F0+0x3A`、同様に生定数でスケール）。
- ⇒ **プレイヤー等の位置更新が閾値を超えたら、Havok ワールドの AABB（world+0x280/0x290）と broadphase の offset/scale を平行移動して原点近くに保つ機構**。slot13 = setWorldAabb（world 構築後に runtime で世界 AABB を移動させる唯一の経路）としての挙動と整合。
- `this`（FUN_0088C600 の ecx）: `+0x50` = 位置 vec、`+0x90..+0x98` = 3軸しきい値、slot22(+0x58) → ahkWorld を返す。クラス名は未特定（呼び出し元 `FUN_004CA2F0` / `FUN_004D4A20` / `FUN_004D5000` はいずれも imm32 参照なしの静的ヘルパで、更の上流未走査）— **[残: this クラスの特定]** だが slot13 呼出しの解明には不要。

### 裁定

> - **slot13 (setWorldAabb `0x8E1160`) の実呼出し元は `FUN_0088C600+0x93`（VA 0x88C693）の1件で確定**（E8=0 / E9=0 / imm32 格納=0 / FF/2=誤検出2件の内訳を含め、全呼出し形の網羅走査の結果）。
> - **v228 の「vcall 呼出し元0件」は走査形の取りこぼきだった**: 本サイトは `mov eax,[ecx]; mov eax,[eax+0x34]; call eax` の**2命令形**で、`call [reg+0x34]` 形スキャンでは検出できない。
> - 受信は `ahkWorld+0x64` の **hkp3AxisSweep**（factory `FUN_008E5020` が ahkWorld ctor で生成・格納）。引数形（3引数・ret 0xC）と戻り後のスタック整合が slot13 本文と一致。
> - **構築時の内部呼出経路 = ctor `FUN_008E4730` へのインライン展開**（同一マスク `0xA965C0`・同一の offset/scale 更新数式、call なし）。world 構築後の runtime 更新（位置シフト）時にのみ slot13 vcall が発生する。
> - 残: `FUN_0088C600` の `this` クラス特定（低優先・呼出しの解明に不要）。

**判定: 解明**（証拠 `v234_1_out.txt` / `v234_1b_out.txt` / `v234_1c_out.txt` / `v234_1d_out.txt` / `v234_1e_out.txt` / `v234_1f_out.txt` / `v234_1g_out.txt` / `v234_1h_out.txt`、v228 §1 継続）。

---

## §2 item2（V-228-2）`0x40F86F`（FUN_0040f760 内）slot83 vcall の受信クラス — **解明: 受信は IDirect3DDevice9（D3D9 デバイス）＝ Get3D census から除外すべき非 TESObjectREFR サイト**

### 証拠A: サイト本体 — FUN_0040f760 は固定機能クアッド描画の4 vcall（`v234_2o_out.txt` / `v234_2p_out.txt` / decompile L8734）

```
FUN_0040f760（cdecl・ret 0、引数6個=4f+2u、受信は呼び出し元が ESI に設定）:
0x40F7D3  mov edx,[esi]                       ; ESI = 受信オブジェクト
0x40F7DD/E7 push 0 ; push esi  → 0x40F81E mov eax,[edx+0x170] / 0x40F83C call eax
                                             ; slot92 = SetVertexShader(This, 0)
0x40F840  mov ecx,[esi] ; [ecx+0x1AC] / push 0; push esi ; 0x40F849 call edx
                                             ; slot107 = SetPixelShader(This, 0)
0x40F84B  mov eax,[esi] ; [eax+0x164] / push 0x104; push esi ; 0x40F859 call ecx
                                             ; slot89 = SetFVF(This, 0x104)
0x40F85B  mov edx,[esi] ; [edx+0x14C]
           push 0x18 ; lea eax,[esp+0x10] ; push eax ; push 2 ; push 5 ; push esi
0x40F86F  call ecx                            ; ← v228 の未確定サイト = slot83 4引数
0x40F871  pop edi / pop ebp / pop ebx / add esp,0x60 / ret
                                             ; call 直後に esp 補正なし ⇒ callee-pop（stdcall）
```

- **v228 の「this がスタック経由の特殊形、4引数」の正体**: COM stdcall では This を最後に push（`push esi`）するため、`mov ecx, …`（thiscall）を前提にしたスキャンでは受信が「スタック経由」に見える。4引数 = `(5, 2, &quad, 0x18)`。

### 証拠B: 13スロットが IDirect3DDevice9 の vtable 順序と完全一致（外部参照: wine-mirror `include/d3d9.h`、対応箇所 `v234_2b/2j/2o_out.txt`）

| disp | idx | 方法（d3d9.h） | 実測引数 | サイト |
|---|---|---|---|---|
| 0x0C | 3 | TestCooperativeLevel | (This) | `0x4103F5`（`[0xB3F928+0x280]` 経由） |
| 0x44 | 17 | Present | (0,0,0,0) | `0x4109E9`（フレーム関数） |
| 0xA4 | 41 | BeginScene | (This) | `0x4108FE` |
| 0xA8 | 42 | EndScene | (This) | `0x4109D9` |
| 0xAC | 43 | Clear | (0,0,1,0xff000000,1.0f,0) | `0x4108F3` |
| 0xE4 | 57 | SetRenderState | (7,0)/(0xe,0)/(0xa8,0xf)/(0x1b,0)… | `FUN_0040f970` / `FUN_0040fa20` |
| 0x104 | 65 | SetTexture | (0, tex) | `0x40FAD0` |
| 0x10C | 67 | SetTextureStageState | (0,Type,Value) ×4 | `FUN_0040f970` |
| 0x114 | 69 | SetSamplerState | (0,Type,Value) ×5 | `FUN_0040f970` |
| 0x14C | 83 | **DrawPrimitiveUP** | (5, 2, buf, 0x18) | **`0x40F86F`** |
| 0x164 | 89 | SetFVF | (0x104) | `0x40F859` |
| 0x170 | 92 | SetVertexShader | (0) | `0x40F83C` |
| 0x1AC | 107 | SetPixelShader | (0) | `0x40F849` |

- 順序・引数形・stdcall（callee-pop）まで含めて13点が一致 — 偶然一致は不可能。**slot89/92/107 = SetFVF / SetVertexShader / SetPixelShader と同定完了**。v228 時点で残っていた「SetVertexShaderConstantF（4引数）相当」仮説は実測が1引数であり d3d9.h 順序とも一致しないので**棄却**。
- IDirect3D9 側も同様: `[ecx+0x28]` = slot10 CheckDeviceFormat（6引数、`0x498751` 等）/ `[ecx+0x40]` = slot16 CreateDevice（7引数、`0x76A545`）— Wine `d3d9.h` の IDirect3D9 順序（RegisterSoftwareDevice が idx3 に存在する点を含む）と一致（`v234_2j/2k_out.txt`）。

### 証拠C: FVF × stride の内部整合（閉路検証）

- SetFVF 引数 `0x104` = D3DFVF_XYZRHW(0x4) | D3DFVF_TEX1(0x100) → 頂点 = 16B(x,y,z,rhw) + 8B(u,v) = **24B = 0x18** = 直後の DrawPrimitiveUP の stride 引数。**前後する2つの独立な命令の引数が数値で完全一致**。
- シーケンス `SetVertexShader(NULL) → SetPixelShader(NULL) → SetFVF(0x104) → DrawPrimitiveUP(TRIANGLESTRIP, 2, …)` は固定機能パイプラインで動画クアッドを描く正準形。前段 `FUN_0040f970` の SetTextureStageState ×4（COLOROP=MODULATE / COLORARG1=TEXTURE / ALPHAOP / ALPHAARG1=TEXTURE 相当）＋ SetSamplerState ×5 ＋ SetTexture(0,tex)（`0x40FAD0`）と整合。

### 証拠D: 受信オブジェクトの出所 = NiDX9Renderer::m_device（`v234_2b/2d/2f/2j/2k/2o_out.txt`、decompile L8989/L9016/L9353）

- **ESI の設定箇所**: `FUN_0040fa20` は `mov esi,[edi]`（`0x40FAD9`、edi=grid ⇒ grid[0]）で ESI を受け渡す。フレーム関数 `FUN_00410840`（`0x4109C8` を含む無名関数）は `mov edx,[0xB350D8] ; mov esi,[edx+0x280]`（`0x4108C5`）で**直接 m_device を読む**。
- **grid[0] = m_device の生成経路**: Bink ctx open `FUN_004104C0` が `mov ecx,[0xB350D8] ; mov edx,[ecx+0x280] ; mov [esi+4],edx`（`0x4104E8-0x4104FC`）で ctx[1]=m_device を格納 → `push ecx(=ctx[1]) … call FUN_0040FEE0`（`0x4105AD`）→ grid alloc は `*_Dst = param_1`（L9016）で **grid[0] = m_device**。
- **m_device の唯一の実行時書手** = `FUN_0076A1C0`（NiDX9Renderer::Initialize、RTTI `.?AVNiDX9Renderer@@` of vtable 0xA88EA4）:
  `mov eax,[0xB42154] ; mov ecx,[eax] ; lea esi,[ebp+0x280] ; push esi(=ppDevice) … mov eax,[ecx+0x40] ; call eax`（`0x76A50C-0x76A545`）＝ **IDirect3D9::CreateDevice の out 引数で renderer+0x280 を直接書く**。他は ctor ゼロ化（`FUN_007641A0+0x18`）と teardown の Release→0（`FUN_0076B5D0+0x489`）のみ。`[reg+0x280]` の mov-store 候補7件（0xB350D8 参照関数に限定、`v234_2d/2f_out.txt`）は、renderer の上記2件（いずれもゼロ化）＋**別クラスの誤検出5件**（`0x450980`/`0x451da0`/`0x628ee0`/`0x6324f0`/`0x633790` — 0x27c/0x280 迻接フィールドをベクトル/初期化群として書く別クラスで、うち4件は 0x27c/0x280/0x284 の FLOAT 3連）＝ CreateDevice の lea 書手は mov-store 候補の対象外で、合計は整合。
- **IDirect3D9 自体**: `FUN_00761DF0` = `LoadLibraryA("D3D9.DLL")`（IAT 0xA28118）→ `GetProcAddress("Direct3DCreate9")`（0xA2811C）→ `Direct3DCreate9(0x20)` → `[0xB42154]`（`v234_2k_out.txt`）。[0xB42154] の書手は作成（`0x761E33`）と破棄時 null（`FUN_007645C0`、`v234_2n_out.txt`）のみ＝ラッパ経由の別デバイスは存在しない。
- 補助: `FF /2 call [reg+0x44]`（Present disp）形スキャンはフレーム関数を検出せず（`.text` 全域で1件 `0x8A9DED` = 別関数、`v234_2m_out.txt`）。フレーム関数の Present は2命令形（`mov ecx,[eax+0x44] ; call ecx`、`v234_2b_out.txt`）＝FF/2 形走査が vcall を取りこぼすことを再度実証（V-228-1 と同じ教訓）。

### 裁定

> - **`0x40F86F` の受信クラス = IDirect3DDevice9（Oblivion.exe 外部・d3d9.dll が提供する COM オブジェクト＝NiDX9Renderer::m_device）で確定**。v228 §3 の slot83（vt+0x14C）census「追加2件」のうち本件は **TESObjectREFR/Get3D 系と無関係な混入**であり、Get3D census からは**除外**すべき（Get3D 系 vcall は20件→**19件**に是正）。
> - **slot83「クラス多重オーバーロード」に第3の意味（IDirect3DDevice9::DrawPrimitiveUP）が加わる**が、受信側 vtable 形状（13スロットの D3D 固有群）で識別可能＝受信クラス照合の必須性が再確認された。
> - slot89/92/107 = SetFVF(0x104) / SetVertexShader(NULL) / SetPixelShader(NULL) と同定完了（当初の SetVertexShaderConstantF 説は棄却）。
> - 慣習: デバイス vcall は全て **stdcall（callee-pop）**、`FUN_0040f760` 自体は cdecl（ret 0・呼出し側 `add esp,0x18`）— `0x40F86F` 直後に `pop edi` があること（esp 補正なし）で確認。

**判定: 解明**（証拠 `v234_2_out.txt` / `v234_2b_out.txt` / `v234_2d_out.txt` / `v234_2e_out.txt` / `v234_2f_out.txt` / `v234_2j_out.txt` / `v234_2k_out.txt` / `v234_2l_out.txt` / `v234_2m_out.txt` / `v234_2n_out.txt` / `v234_2o_out.txt` / `v234_2p_out.txt`、v228 §3 継続、外部参照: wine-mirror `include/d3d9.h`）。

---

## §3 item3（V-228-3）`0xA96B78` = `hkNullCollisionFilter` の参照経路突合 — **解明: クラス名レジストリに「name ↔ factory ↔ vtable」の直接ペアを確認、確度 中〜高 → 高 に昇格（＋v228 の文字列アドレスを `0xA9A2BA` → `0xA9A2B8` へ訂正）**

### 証拠A: 文字列の実体訂正と出現状況（`v234_3a_out.txt`）

- **'hkNullCollisionFilter' の実アドレスは `.rdata 0xA9A2B8`**（全ファイル1出現のみ、10進 11,117,240）。**v228 表記の `0xA9A2BA` は2バイト誤り**で、そこへの参照が0件に見える直接原因だった（`imm32_refs3(0xA9A2BA)` = 0 は誤アドレスに対する検査だったことが再検証で判明）。
- RTTI 形式名 `'.?AVhkNullCollisionFilter'` は**不在**（= RTTI 不在を名字列レベルでも再確認）。
- 帯の残り: `'hkConstraintInstance'` = `0xA9A288` / `'hkPhysicsSystem'` = `0xA9A2A8`（ともに参照2件: .text + .data）。帯には solver ステップ名（`'StBackstep'` `0xA9A104` 等）と**小 vtable が同居**（後述証拠C）。

### 証拠B: 文字列への参照は2箇所のみ — その1つが「クラスレコード」（`v234_3a/3b/3c_out.txt`）

- `imm32_refs3(0xA9A2B8)` → **`.text 0xA14E66`（登録 thunk）＋ `.data 0xB2FC3C`（レコードの name フィールド）** の2件のみ。
- `.data 0xB2FC24..0xB2FC44` = **12バイト stride ×3 の {name_ptr, factory_ptr, vtable欄} レコード**:

```
0xB2FC24: 00A9A288 'hkConstraintInstance' | 008D9A90 | (runtime)00A9A274
0xB2FC30: 00A9A2A8 'hkPhysicsSystem'      | 008D9EA0 | (runtime)00A9A2A0
0xB2FC3C: 00A9A2B8 'hkNullCollisionFilter'| 008DA210 | (runtime)00A96B78  ←★ ペア成立
```

### 証拠C: 第3フィールド = vtable、動的初期化子が起動時に書き込む（`v234_3b/3c/3e/3f/3i_out.txt`）

- **`FUN_00A12BC0`**（このレコード専属の動的初期化子）: `mov eax,0xA96B78` → バイト分解 → `mov dword [0xB2FC44],edx`（`0xA12BFF`）＝ レコード+8 に **vtable 0xA96B78** を格納（ファイル上は0、実行時書込。定数のバイト分解ストアは通常出力でない形だが意図は断定しない）。
- 兄弟も同型: `FUN_00A12B20` → `[0xB2FC2C]=0xA9A274`、`FUN_00A12B70` → `[0xB2FC38]=0xA9A2A0`。
- `0xA9A274` / `0xA9A2A0` は `.rdata` の **RTTI 無し小 vtable**（`[0]` = 各クラス dtor エントリ `FUN_008d9930` / `FUN_008da1a0`、`[1]` = 共通 `FUN_0068f970`、vtable-4 に有効な RTTI col なし）で、**名前バイト列と数バイト間隔で同居** ⇒ 3レコード連続で「名前↔vtable ペア」パターンが反復。
- **実行確立**: これら3初期化子（＋thunk）は `.rdata` のコードポインタ列（`FUN_00A12BC0` = `0xA2F300`、hkNull thunk `0xA14E50` = `0xA2F57C`。列は `0xA283D0..0xA2F77C`（479+エントリ、終端0））に並び、**CRT 起動ループ `0x981CB4..0x981CD0`**（`mov esi,0xA283D0 ; mov edi,0xA2F77C ; … mov eax,[esi] ; test eax,eax ; je skip ; call eax ; add esi,4`、end 境界 `0xA2F77C` への参照 = `0x981CBB`）が**起動時に全て呼ぶ**（`_initterm` 相当。named_funcs 归属は `__get_winmajor+N` だが実体は初期化子実行ループ）。

### 証拠D: 第2フィールド = 自クラスの vtable を書くインスタンス生成関数（`v234_3d/3k_out.txt`）

- **`0x8DA210`**（int3 パディング直後の独立エントリ。`fname()` の `FUN_008da1d0+0x40` はキャッシュ境界の帰属誤り。前関数 `FUN_008DA1D0` は `0x8DA200 ret` で終了）:

```
[eax+6]=1refcount / [eax+8]=0xA95C60 / [+0xC]=0xA95C3C / [+0x10]=0xA95C48 / [+0x14]=0xA95C68
[eax]=0xA96B78 / [+8]=0xA96B64 / [+0xC]=0xA96B70 / [+0x10]=0xA96B68 / [+0x14]=0xA96B64
```

  = **v228 証拠A の ctor `FUN_00899330` が書くレイアウトと逐語一致**（既定4抽象インターフェイス → concrete 5vptr へ上書き）。つまり factory フィールドは挙動面でも「0xA96B78 クラスのインスタンスを生成する」関数。
- 兄弟も同型（`v234_3k_out.txt`）: `0x8D9A90` = `[eax+6]=1 ; [eax]=0xA9A274`、`0x8D9EA0` = `[eax+6]=1 ; [eax]=0xA9A2A0` — **factory が書く vtable がレコード+8 の値と一致**するため、name↔factory↔vtable の3点対応は3件とも自己整合。

### 証拠E: 名前レジストリ登録 thunk（約150クラス）とインデックス（`v234_3c/3e/3f/3h/3j_out.txt`）

- **`FUN_00A14E50`**（初期化子テーブル `0xA2F57C` 所属、起動時実行）:
  `push 0×7 ; push 0x18 ; push 0xBA965C ; push 0xA9A2B8('hkNullCollisionFilter') ; mov ecx,0xBA9118 ; call FUN_0090D190 ; ret`。
- `FUN_0090D190` = `ret 0x28`（10引数・this=ecx・callee-pop）で **this+0..+0x20 の9フィールドレコード生成**の単純構築関数。E8 呼出しは **150件** ＝ 約150クラスの {name, 補助構造体ptr, スカラー…} デスクリプタが `.bss`（hkNull = `0xBA9118`）へ起動時に構築される。引数8は `.rdata` の補助構造体ポインタ（`0xAA1194` 等は非文字列・生ダンプ確認）、引数3 = `0x18`（hkNull）等のスカラー — **意味は[残]**。
- **インデックス**: `.rdata 0xA9A910..0xA9AA3C`（NULL終端・要素=レコード先頭ポインタ、hkNull は `0xA9A954 → 0xB2FC3C`）と `.rdata 0xA9AA48..`（NULL終端・要素=`.bss` デスクリプタ、hkNull は `0xA9AA8C → 0xBA9118`）の**2本のポインタリスト**。

### 証拠F: 消費系の状況 — 残点として記録（`v234_3f/3g/3h_out.txt`）

- リスト走査者 `FUN_00943070`（3コンテナ生成 → `FUN_009546D0` へ両リスト渡し。コンテナ vtable `0xAA2738/0xAA2748/0xAA2758` も RTTI 無し）は **E8・imm32 の全走査で到達経路0件**、`FUN_009546D0` の唯一の呼出し元も本関数自身 ⇒ **出荷バイナリでこの汎用走査系は未使用の可能性**。レコード +4/+8 の最終消費者は未特定 = **[残]**。
- 一方、起動時 live 経路（初期化子ループ → thunk / vtable 初期化子）は実行確立しており、名前↔vtable のペア成立そのものは消費系の成否に依存しない。

### 裁定

> - **`0xA96B78` の正式名 `hkNullCollisionFilter`（Havok SDK 組込の全許可フィルタ）を確度「中〜高」→「高」に昇格**。根拠: (1) 名前↔vtable↔factory の3点が**同一レコード `0xB2FC3C` に直接対応付け**、(2) factory `0x8DA210` の書込内容が v228 証拠A の ctor と**逐語一致**（挙動的裏付け）、(3) 同型ペアが兄弟2件で反復、(4) 書き込み・登録が**起動時初期化ループ `0x981CCB call eax` で実行される**ことまで確立。
> - **RTTI 不在は hkNull 単独ではなく Havok 静的ライブラリ全体**（兄弟 vtable `0xA9A274`/`0xA9A2A0`・`hkp3AxisSweep` `0xA9A710`・コンテナ `0xAA27xx` も不在、形式名も不在）であり、RTTI ありは `0xA95Cxx` 抽象インターフェイスのみ — v228 の「ビルド設定混在」説を訂正なく強化。
> - **v228 の文字列表記を `0xA9A2BA` → `0xA9A2B8` に訂正**（本束の新規発見。0xA9A2BA 参照ゼロは誤アドレス検査だった）。
> - 残: (a) レコード/リストの最終消費者（`FUN_00943070` の到達経路・未使用の可能性）、(b) 登録 thunk 引数3/8/9 の意味、(c) `0x8DA210` 等の factory を呼ぶ側（レコード経由の読み手）の未特定。

**判定: 解明（正式名の確度 高 に昇格）**（証拠 `v234_3a_out.txt` / `v234_3b_out.txt` / `v234_3c_out.txt` / `v234_3d_out.txt` / `v234_3e_out.txt` / `v234_3f_out.txt` / `v234_3g_out.txt` / `v234_3h_out.txt` / `v234_3i_out.txt` / `v234_3j_out.txt` / `v234_3k_out.txt`、v228 §2 継続）。

---

## §4 自己検証（内部整合・制約遵守）

- **§2 のスロット算術**: 全13件で `disp/4 = index` を再計算（0x0C→3 / 0x44→17 / 0xA4→41 / 0xA8→42 / 0xAC→43 / 0xE4→57 / 0x104→65 / 0x10C→67 / 0x114→69 / 0x14C→83 / 0x164→89 / 0x170→92 / 0x1AC→107）。FVF 0x104 = XYZRHW|TEX1 → 16+8 = 24 = DrawPrimitiveUP stride `0x18` と前後一致。
- **§2 の ESI 経路は2系統とも同一指**: `FUN_004104C0`（ctx[1] 格納 `0x4104FC` → FEE0 arg0 → `*_Dst=param_1` L9016）とフレーム関数直読（`0x4108C5`）がいずれも `[0xB350D8+0x280]` = CreateDevice out（`0x76A513 lea esi,[ebp+0x280]`）に収束。mov-store 候補7件の内訳 = renderer ctor ゼロ化1（`FUN_007641A0`）+ teardown Release→0 1（`FUN_0076B5D0`）+ 別クラス誤検出5 で合計7（CreateDevice の lea 書手は mov-store 候補の対象外）＝ `v234_2f` の文脈 dump と一致。
- **§3 の数値**: `0xA9A2B8` = 11,117,240（10進→16進一致）; レコード stride 12（`0x30-0x24` = `0x3C-0x30` = 12）; 初期化子走査範囲 `0xA2F77C-0xA283D0` = 29,612 B = 7,403スロット（NULL エントリはスキップ、列末境界 `0xA2F77C` の値は0）; thunk 引数は `v234_3c` のスタック積み順と `FUN_0090D190` の読み順（[esp+4]→[this+0] …）が一致。
- **§1/§2/§3 とも「imm32 参照」は `imm32_refs3()` またはセクション個別 raw スキャン（`vaddr+オフセット` 変換）のみ**＝VA 換算罠を回避。E8 は `e8_callers` / `e8()`、FF/2 形は生バイト走査。
- **制約遵守**: ESM 読取0回・ESM 以外のソース編集0回・既存 `tmp/re/*.md` 無編集（新規は本1ファイルのみ）・git/docs/GT/TODO/session-memory/plan 無編集・`cache/` は参照のみ（`fname()`）。スクリプトは全て `%TEMP%\opencode\v234_*.py`（`python "<path>"` で実行、ヒアドック・`python -c` 正規表現なし）。新規中間物: `v234_2o/2p`・`v234_3a..3k`（各 `_out.txt` 付き）。
- **外部参照（明示）**: §2 の vtable 順序は wine-mirror `include/d3d9.h`（IDirect3DDevice9 / IDirect3D9）を参照。他はすべて EXE 内証拠。
