# V183: 束C レンダラ/ストリーミング残件束 — O1 六項目・O2 残3 の裁定 — Oblivion.exe v1.2.0416 Steam

- 作成日: 2026-10-29 / 担当: レンダラ/ストリーミング分析サブエージェント（TODO serial 171 / V171・高【移植直結】）
- 編集範囲: **本新規 1 ファイルのみ**（docs/TODO/session-memory/plan/cache/既存 tmp/re は読取のみ）。git 不使用。ESM 再スキャンなし。
- 入力（読取専用）: `tmp/re/TODO_re49.md` L99-L100（O1/O2 行）、`v180_letter_rows_audit.md` §2 O 節、`s61_render.md`、`s61_stream.md`、`s81_stream_audio_crt_io.md` Task A、`v92_shader_renderer_bundle.md`、`v129_shader_slot_residuals.md`、`v159_renderer_residuals.md`、`v173_small_residuals2.md`、`cache/{named_funcs,vtable_index,call_targets}`、`docs/Oblivion_decompiled.c`（部分検索）。
- 手法: capstone 5.0.7（skipdata）+ バイトパターン/imm32 参照スキャン。中間スクリプトは `%TEMP%\opencode\v183_*.py`。
- 表記: [確定] / [確定(観測)] / [推定] / [未解明]。重複排除: v92/v129/v159/v173 で解決済みの項目は当該節で明示して分析しない。

---

## §0 対象と重複排除ポリシー

### 0-1. 本束の対象（v180 §2 O 節判定に準拠）

| 項 | 対象 | 土台 | v180 判定 |
|---|---|---|---|
| O1-1 | SetLight 代替機構 | s61_render.md §3 | (c) 未着手 |
| O1-2 | DrawIndexedPrimitive 経路 | s61_render.md §1.2 | (c) 未着手 |
| O1-3 | BSShaderPPLighting 完全関数マップ | s61_render.md 残件3 | (c) 未着手 |
| O1-4 | シェーダハンドルテーブル | s61_render.md 残件4 | **v129 で解決済み → 本束では排除** |
| O1-5 | 天候ロード | s61_render.md §4 | (c) 未着手 |
| O1-6 | Portal/DMap カリング | s61_render.md §6 | (c) 未着手 |
| O2-a | NiNode ツリー NIF 組立経路 | s61_stream.md §2.3/§7-3 | s81 未消化 → 残3 |
| O2-b | LOD テクスチャブレンド経路詳細 | s61_stream + s81 A4 [PARTIAL] | s81 残点 |
| O2-c | メモリプール拡張 | s61_stream.md §4 | s81 未消化 → 残3 |

### 0-2. 重複排除（既存成果物で解決済みのため本束の分析対象外）

| 既存解決項目 | 出典 |
|---|---|
| シェーダハンドルテーブル（FUN_007B4290 factory → slot=b42ec0+i / arg0=i の 28 クラス対応表） | v129 §2 [確定] |
| NiDX9Renderer vtable = 0xA88EA4（98 スロット）/ [0xB42754] 同定 | v92 §b [確定] |
| NiDX9RenderState vtable = 0xA8A9F4（67 スロット）/ [state+0x8AC] | v92 §b-4・付録A |
| renderer[+0x280] = IDirect3DDevice9* | v159 付録A [確定] |
| renderer vtable[+0x54] getter 呼出し元 10 site / 残余 6 site | v159 付録B + v173 §2-1 |
| [state+0xFF8] = DeviceWrapper（ポインタ同一性証明） | v129 §4 |
| b42ec0..b42f2c 28 スロット実行時再初期化経路 / FUN_007B8530 役割分担 | v92 §c / v129 §7 |
| RefreshShaders（opid 0x133）→ FUN_007B81A0 シェーダ再生成 | v92 §a |
| [0xB43110] = NiSourceTexture | v173 §2-5 [確定(RTTI)] |
| renderer gap50 / state gap54 tiny getter ラベル化 | v129 §8 |
| NiD3DShaderInterface/NiShader/SkinShader 型連鎖 | v159 付録E/F + v173 §2-2 |

---

## §1 O1-1 SetLight 代替機構 — **[確定（機構）]** D3D9 ライト API 完全不使用・RenderState キャッシュ層経由の転送構造を解明

### 1-1. 再走査による s61 前提の訂正

- s61_render.md §1.2 の「SetLight (vtable+0xE4)」は **vtable offset の誤認**。IDirect3DDevice9 の実 vtable では **SetLight = index 51 = +0xCC**、**+0xE4 = SetRenderState (index 57)**。
- 本束の全形式再走査（`v183_o1_1b_probe.py` / `v183_o1_1f_devcreate.py`）:
  - `call [reg+0xE4]`（disp8/disp32・eax/ecx/edx/ebx/ebp/esi/edi 全レジスタ）= **0 件** → s61 の結論をバイトレベルで再確認
  - `call [reg+0xCC]`（= SetLight）/ `call [reg+0xD4]`（= LightEnable）= **0 件**（新規）
  - `call [reg+0x148]`（DrawPrimitive）/ `call [reg+0x14C]`（DrawIndexedPrimitive）も **0 件**（→ §2）

### 1-2. 代替機構の実体 — NiDX9RenderState キャッシュ層 + inner 転送 [確定]

**構造（2 段階）**:

1. **キャッシュ層 = NiDX9RenderState（vtable 0xA8A9F4）の slot +0x64 = FUN_0077B000**
   ```
   FUN_0077B000(this=state, idx, val, flag):          ; ret 0xC
     cmp [this + idx*8 + 0x120], val → 変化時のみ:
     inner := [this+0xFF8]      ; = NiDX9Renderer[+0x280]（COM AddRef/Release 付き）
     call [inner_vt + 0xE4](inner, idx, val)           ; = SetRenderState(idx, val)
     if flag: [this+idx*8+0x124] := old;  [this+idx*8+0x120] := val
   ```
   - キャッシュ配列 `this+0x120+idx*8`（dword val + dword prev）。idx は D3DRENDERSTATETYPE 番号そのもの。
2. **クリア系 = FUN_007716E0**（arg0≠0 かつ renderer flags(+0x5E0) bit8=0/bit4=1 時）: state slot +0xD0 を 2 回 + **slot +0x64 をループ 8 回（idx = 0x80..0x87、val=0）**で一括クリア。
3. **inner の正体 = NiDX9Renderer[+0x280] = IDirect3DDevice9 互換 COM オブジェクト [確定(観測)]**
   - 生成初期化: 0x7641A0 `mov [esi+0x280], 0` + `[esi+0x284]` へ `memset(0x130=304B)` = **D3DCAPS9 構造体**（v129 §4-2 の memcpy(+0x1018, &renderer[+0x284], 0x130) と完全整合）
   - 解放: 0x76BA47 `call [inner_vt+8](inner)` = **COM Release**、`[ebx+0x280] := 0`
   - リンク: FUN_0077F840（state setter）`[state+0xFFC] := renderer`、`[state+0xFF8] := [renderer+0x280]`（old Release / new AddRef）
   - vtable スロットの D3D9 index 突合: **+0xE4 = SetRenderState(57) / +0x104 = SetTexture(65) / +0x1AC = SetPixelShaderConstantF(111)** が全て実運用と一致 → inner は **raw IDirect3DDevice9 相当**（※ v129 §4-2「raw ではない」は「D3D9 vtable 86 スロット」の誤認による。IDirect3DDevice9 は 119 スロットで +0x1AC=111 番目は範囲内 → **v129 の当該一節は本束で訂正**）

### 1-3. 「代替機構」の意味論 [確定]

- D3D9 固定機能ライト（SetLight / LightEnable）は **完全に未使用**。ライティングはシェーダベース（BSShaderPPLighting 系、§3）で行い、固定機能のライト経路はオフのまま。
- s61_render §3.1「[推定]: 固定関数パイプライン (D3DRS_LIGHTING) の ON/O/OFF のみ」は**方向として正しいが実態はさらに一歩進む**: ライト設定はデバイスへ一切渡らず、**RenderState キャッシュ（D3DRS リマップ表 0xB427E0/0xB427B0 併用、v92 §b-4）のみが SetRenderState として inner へ転送**する。
- 転送レイテンシ最適化: 変化時のみ転送（cmp → je skip）＝**ステートキャッシュラッパ設計**。これは Android 移植時に「SetRenderState 互換層」を再実装する根拠になる。

### 1-4. 残点（§1 系）

- inner の**生成箇所（CreateDevice 本体）**は未特定: 0xB28E00 グローバル（vtable 付きオブジェクト、0x77BE86 が `call [vt+4]`）がデバイス制御系 [推定]。0x775DF0/0x775CC0 はアダプタ列挙（IDirect3D9::GetAdapterCount vtable+0x10 + D3DADAPTER_IDENTIFIER9 0x468B ×N 構築）であり s61_render §1.1「0x763DE0 = CreateDevice ラッパー / [0xB42160] = IDirect3DDevice9*」は**誤記**（正: [0xB42160] = NiTArray<NiDX9AdapterDesc*> アダプタ配列オブジェクト、vtable 0xA8A470）。→ 新規残点 V-171a
- ライト値の供給元（NiLight 系 → シェーダ定数の転記）は O1-3 と接続して追跡。


## §2 O1-2 DrawIndexedPrimitive 経路 — **[確定（反証）]** EXE は DIP/DP を一切直接呼出しない。s61 の vtable オフセット表はメソッド名対応自体が誤認

### 2-1. 全形式再走査の結果

`v183_o1_2_dip.py` / `v183_o1_2e_dip32.py` で IDirect3DDevice9 の実 vtable オフセットを全形式走査:

| スロット | D3D9 index | `call [reg+imm]` disp8/disp16/disp32 全レジスタ | `mov r32,[reg+imm]`（fnptr ロード） |
|---|---:|---|---|
| +0x148 DrawPrimitive | 82 | **0 件** | 多数（→ 2-3 の通りほぼ全て別クラスのメンバ +0x148 で誤検出） |
| +0x14C DrawIndexedPrimitive | 83 | **0 件** | 21 件（→ 2-3 で全て非該当と判定） |

### 2-2. s61_render.md §1.2 の vtable 表の系統的誤認（新規訂正）

s61 の「Clear +0x24 / BeginScene +0x28 / EndScene +0x2C / Present +0x30 / DrawPrimitive +0x74 / SetRenderState +0x58 / SetTexture +0x64 / SetLight +0xE4」は **IDirect3DDevice9 の実 vtable index と不一致**:

| s61 記載 | s61 記載 offset | IDirect3DDevice9 の実 index/offset | 実際に s61 offset に該当する D3D9 メソッド |
|---|---|---|---|
| Clear | +0x24 | index 43 / +0xAC | index 9 = GetCreationParameters |
| BeginScene | +0x28 | index 41 / +0xA4 | index 10 = SetCursorProperties |
| EndScene | +0x2C | index 42 / +0xA8 | index 11 = SetCursorPosition |
| Present | +0x30 | index 17 / +0x44 | index 12 = ShowCursor |
| DrawPrimitive | +0x74 | index 82 / +0x148 | index 29 = CreateDepthStencilSurface |
| SetRenderState | +0x58 | index 57 / +0xE4 | index 22 = SetGammaRamp |

- s61 §1.2 の「16 種 vtable 呼出件数マッピング」は**件数は実在するがメソッド同定が全項目ズレている**（表記の offset が 4 桁hex だが index を offset と取り違えた形跡）。
- 一方、**inner（renderer[+0x280]）のフォワード実測**（§1-2）+0xE4=SetRenderState(57)/+0x104=SetTexture(65)/+0x1AC=SetPixelShaderConstantF(111) は D3D9 index と完全一致 → inner 側の vtable レイアウトは D3D9 互換であることが独立に確認できる。

### 2-3. `mov r,[reg+0x14C]` 21 site の判定

- renderer 帯（0x76-0x78xxxx）: **0 件**（0x764905/0x764940 等は +0x148 = renderer 自身のバッファ管理メンバ）
- 残りは 0x44-0x4A 帯（TES 高レベルオブジェクト: g_thePlayer 系 0x4421AD = v173 §4-3 で PlayerCharacter と確定済み）・0x7E-0x7F 帯（NiTriShape 系ジオメトリの +0x14C メンバ）・0x8DC8xx（バッファ配列走査）→ **全て「クラスメンバの +0x14C オフセット」であって D3D9 vtable 呼出しではない** [確定(観測)]

### 2-4. 結論 [確定（反証）]

- **Oblivion.exe v1.2.0416 は IDirect3DDevice9::DrawIndexedPrimitive / DrawPrimitive を一切直接呼出しない**（disp8/16/32・全レジスタ・call/fnptr-load 全形式で 0 件、inner フォワードにも不含）。
- primitive 発行は NiDX9Renderer ライブラリ帯（0x760000-0x783000）の内部が inner の vtable を**実行時にロードして間接呼出し**する経由のみと推定される（`mov edx,[inner]; call [edx+N]` の 2 段構成のためバイトパターン走査では検出不能）。
- **inner の実 vtable は実行時グローバル**（renderer[+0x280]、初期化は 0x7641A0 で 0 化のみ）で、CreateDevice 本体（IDirect3D9 vtable +0x40 = index 16 の呼出し）も 0xB42154 参照 21 site 全走査で非検出（0xB42154 の vtable 呼出しは全て CheckDeviceFormat +0x28 / GetAdapterModeCount +0x18 / EnumAdapterModes +0x1C）→ **静的に DIP スロットの同定は不能**。
- → **新規残点 V-171b（B13 実機 .data ダンプ依存）**: inner の実 vtable と CreateDevice 呼出し箇所の同定。

### 2-5. 0xBA7D98 の実体訂正（s61 §1.3 関連）

- s61「デバイスラッパーグローバル 0xBA7D98」の実体は **リソースプール管理オブジェクト** [確定(観測)]: 割当 `call [vt+0x10](0x100/0xf0, 0x2b)`（0x8A9660/0x8A968E）→ 返却ポインタへ `[eax+4] := size` → 構築 0x8EAD40（vtable 0xA9AF38）/ 解放 `call [vt+0x14](ptr, 0x2a/0x2b)`（0x8A9910/0x8A9940）。0x2a/0x2b = 種別コード。
- [0xB42154] = IDirect3D9*（実測で vtable 呼出と整合）だが [0xB42160] = **NiTArray<NiDX9AdapterDesc*>**（vtable 0xA8A470、0x775DF0 構築）で s61「IDirect3DDevice9* ptr」は**誤記**（→ §1-4）。


## §3 O1-3 BSShaderPPLighting 完全関数マップ — **[確定（クラス同定・型連鎖・vtable）/ 関数機能は主要 slot まで]** クラス名は「BSShaderPPLighting**Property**」

### 3-1. s61 残件3 の前提訂正: 実在クラス名

- .rdata に RTTI 名 ".?AVBSShaderPPLighting@@" は**不在**。プレーン文字列は **0xA9163C = "BSShaderPPLightingProperty"**（"BSShaderPPLighting" はそのプレフィックス）。
- → s61_render.md 残件3 の「BSShaderPPLighting クラス」は **BSShaderPPLightingProperty**（シェーダライティングプロパティ）を指す [確定]。派生に **SpeedTreeShaderPPLightingProperty**（0xA92CC0）あり。

### 3-2. 型レコード連鎖（.CRT 登録 stub 全 398 件抽出）— **[確定]**

手法: v159 付録E と同一パターン `push parent; push "name"; mov ecx, node; call 0x70E220; ret` を全セクション走査（`v183_o1_3c_typechain.py`、398 件検出）:

```
BSShaderPPLightingProperty   node 0xB45DA8  stub@0xA11550
 └ BSShaderLightingProperty  node 0xB4618C  stub@0xA11930
   └ BSShaderProperty        node 0xB46000  stub@0xA115F0
     └ NiShadeProperty       node 0xB401B0  stub@0xA0A650
       └ NiProperty          node 0xB3F68C  stub@0xA09990
         └ NiObjectNET       node 0xB3F584  stub@0xA09920
           └ (NiObject)      node 0xB3F684（未登録 = ルート直下）
派生: SpeedTreeShaderPPLightingProperty node 0xB468DC → BSShaderPPLightingProperty
```

- GetType スタブ: `mov eax,node; ret` = **0x7D7710（PPLighting）/ 0x7EE4E0（Lighting）/ 0x7E26F0（BSShaderProperty）/ 0x73DA90（NiShadeProperty）** [確定]
- ※ 本連鎖は v159 付録E の NiD3DShaderInterface 連鎖（ルート NiShader）とは**別系統**（NiProperty 側）。BS 系プロパティは RTTI 無効だが本システムで階層を保持。

### 3-3. vtable 同定 — **BSShaderPPLightingProperty vtable = 0xA91384（42 スロット）** [確定]

GetType(0x7D7710) が vtable slot1 に配置（v159 付録E の配置規則と一致）→ vtable[-1] = COL **0xAD0144**。

| slot | +off | target | 台帳 | 機能（実測） |
|---:|---|---|---|---|
| 0 | +0x00 | 0x7D97D0 | FUN_007d97d0 | dtor |
| 1 | +0x04 | 0x7D7710 | gap | **GetType**（`mov eax,0xB45DA8; ret`） |
| 6 | +0x18 | 0x7D9810 | FUN_007d9810 | 継承層メソッド |
| 12 | +0x30 | 0x7D9890 | FUN_007d9890 | 継承層メソッド |
| 22 | +0x58 | **0x7DA220** | FUN_007da220 | **シェーダフラグ/パス名パーサ [確定(観測)]**: `_stricmp/_strnicmp/_strtok/_sprintf` 使用、文字列 **"dynalpha" "RefractF" "Refract" "refract" "envmap2" "left eye" "right eye"** + エラー "Invalid tangent space data in NULL/%s : %s" |
| 23 | +0x5C | 0x7D85D0 | FUN_007d85d0 | ロード系（0x405760 文字列/ID 取得 → 0x7E2430/0x7ED1A0/0x85AAC0/0x85ABD0 経由） |
| 24 | +0x60 | 0x7D8B40 | gap | **能力/有効判定**: `[0xB43074]==0 && [0xB43108]!=0 && ([0xB42F40]&0x20) && [0xB42F48]>=2 → true` |
| 25 | +0x64 | 0x7B22F0 | gap | `lea eax,[ecx+0x58]; ret` = **+0x58 メンバ getter** |
| 26 | +0x68 | 0x7D7F80 | FUN_007d7f80 | **Stream ロード (SEH)**: 文字列 "tex %"、0x7B4160/0x7B8200 併用 |
| 27 | +0x6C | 0x7D8160 | FUN_007d8160 | **Stream 読出/読込 (SEH)**: 0x7B4160/0x7B8200 |
| 28 | +0x70 | 0x55CEF0 | gap | `mov eax,[ecx+0xDC]; ret` = **+0xDC getter** |
| 29 | +0x74 | 0x7D7720 | gap | `[ecx+0xDC] := arg1` = **+0xDC setter** |
| 30 | +0x78 | 0x7D73B0 | gap | **+0xDC モード判定**（1→?、3→0、2→? の三分岐） |
| 31 | +0x7C | 0x7D73D0 | gap | **+0xDC 範囲検証 setter**（arg1 ∈ 1..3 → =1、他は不正） |
| 32 | +0x80 | 0x7D7730 | gap | **配列 [+0xBC][idx] 差替え**（Release 付き） |
| 33 | +0x84 | 0x7D7790 | gap | **配列 [+0xC0][idx] 差替え** |
| 34 | +0x88 | 0x7D77F0 | gap | `[+0xBC][idx]` getter |
| 35 | +0x8C | 0x7D7800 | gap | `[+0xC0][idx]` getter |
| 36 | +0x90 | 0x7D7810 | gap | `[+0xC4][idx]` getter |
| 37 | +0x94 | 0x7D7820 | gap | **配列 [+0xC4][idx] 差替え** |
| 38 | +0x98 | 0x855240 | FUN_00855240 | **描画パス設定**: `[esi+0x1C]` の bit0/bit4 判定 → 0x852150/0x853580/0x854CD0/0x854E90 帯へ分岐 |
| 39 | +0x9C | 0x85AE20 | FUN_0085ae20 | **描画パス実行系 (SEH)**: 0x85AAC0/0x85ABD0/0x7B7650/0x8AA390 経由 |
| 41 | +0xA4 | 0x7D97F0 | FUN_007d97f0 | scalar deleting dtor（call 0x7D79C0 + free） |
| 42+ | +0xA8 | — | — | vtable 終了。直後は SpeedTree パラメータ名テーブル（"Edge Falloff" "Edge Color A/G/R" "Fill Color ..."）＝**別テーブルであり vtable ではない** |

- メンバレイアウト（slot 実測より）: **+0x58 データ先頭 / +0xBC・+0xC0・+0xC4 = 3 本のポインタ配列（各 idx アクセス、Release 付き差替え）/ +0xDC = モード (1..3) / +0x1C = 描画フラグビット**。
- 基底共有: slot2-5/16-17 = 0x6F7070（報告スタブ）、slot13 = 0x68F970（共有スタブ、v92 §b-2 と同一）、slot19 = 0x73DAA0（NiShadeProperty 層）、slot7-11 = NiProperty 層。

### 3-4. 実装結論（移植示唆を含む）

- ライティングは **プロパティがシェーダフラグ（dynalpha/Refract/envmap2 等）と 3 配列のライティングデータ**を持ち、0x85xxxx 帯のパス設定関数が SM3 シェーダ（"SM3Lighting.v.hlsl"/"SM3Lighting.p.hlsl" 0xA9324C/0xA9333C、"HLighting30Shader" 0xA93353）へ反映する構造 [推定（パス設定関数の深掘りは未）]。
- D3D 固定機能ライト不使用（§1-3）と整合する。

### 3-5. 残点

- 0x855240/0x85AE20（slot38/39）の**パス設定本体**（シェーダ定数へライトを渡す経路）は深掘り未了 → 残点 V-171c（O1-1 の「ライト値供給元」と接続）。
- slot22 のフラグ文字列群の完全リスト（0x400B 以降）は未走査。


## §4 O1-4 シェーダハンドルテーブル — **[重複排除]** v129 §2 で確定済み

- FUN_007B4290 factory の 28 種クラス対応表（slot = 0xB42EC0+i、arg0 = i）が本項の実体（v180 §2 O1 判定も「= シェーダハンドルテーブル相当」と明記）。本束では再分析せず排除。
- 補助: v92 §c（0xB42EC0..0xB42F2C 再初期化経路）+ v129 §5（slot +0x140/+0x144 行列更新）でテーブルのライフサイクルも確定済み。

## §5 O1-5 天候ロード — **[確定]** 天候ロード関数群を特定（TESWeather ctor / Sky 構築 / Atmosphere・Clouds ロード）+ SkyShader クラス系統を同定

### 5-1. s61_render.md §4 の前提訂正

- s61 §4.1「天候関連文字列: 未発見」は**文字列走査の網羅不足による誤判定**。実際は .rdata に天候関連文字列が多数存在（`v183_o1_5_weather_str.py`）:
  - アセットパス: **"Meshes\Sky\Atmosphere.nif"(0xA56B88) / "Meshes\Sky\Clouds.nif"(0xA56C64) / "Sky\Sun.dds"(0xA45E40) / "Sky\SunGlare.dds"(0xA45E2C) / "Sky\Stars.nif"(0xA45E1C) / "Sky\CloudsClear.dds"(0xA480C8) / "Sky\CloudsCloudy.dds"(0xA480DC) / "Sky\Snow.NIF"(0xA56DE0) / "Meshes\Sky\RainLight.NIF"(0xA86098) / "Meshes\Sky\RainHeavy.NIF"(0xA860B4) / "Textures\Sky\MoonShadow.dds"(0xA56CD8) / "\Textures\Sky\%s_%s.dds"(0xA56D58, sprintf用テンプレート) / "Textures\Sky\SunGlareNonHDR.dds"(0xA56EB8)**
  - GMST: fWeatherCloudSpeedMax / fWeatherTransMax / fWeatherTransMin / fWeatherTransAccel / fWeatherFlashDuration / fWeatherFlashDirectional / fWeatherFlashAmbient / fSkyBrightness / fMagicSunDamageMinWeather
  - スクリプト: ForceWeather / SetWeather / ReloadCurrentWeather / ReleaseWeatherOverride / GetCurrentWeatherPercent / GetIsCurrentWeather / SetSkyParam / ToggleSky
  - 型名: **TESWeather(0xA4812B) / TESWeatherList(0xA4815A) / DefaultWeather(0xA37CB0)**

### 5-2. ロード関数の特定 — **[確定(観測)]**

| 関数 | 証拠 | 機能 |
|---|---|---|
| **FUN_0053D260** | sprintf "\Textures\Sky\%s_%s.dds" を **7 回**（0x53D2D2/53D2EB/53D2FF/53D313/53D32A/53D33E/53D34F）、vtable 0xA56CAC 設定、eh_vector_constructor_iterator(+0x18, 8要素×8B)、[esi+0x58..0x68] float 7 語 + [esi+0x74]=0.0 + [esi+0x78]=FLT_MAX(0xA32048) | **TESWeather 構築子（天候テクスチャパス 7 種を +0x18..+0x50 へ sprintf）** |
| **FUN_004BEE70** | [esi+0x3C]:="Sky\Sun.dds" / [esi+0x48]:="Sky\SunGlare.dds"（NiString ctor 0x4028D0）/ [esi+0x18] vtable+0x18 経由で "Sky\Stars.nif" | **Sky 天体オブジェクト構築（Sun/Glare/Stars）** |
| **FUN_0053B3F0** | "Meshes\Sky\Atmosphere.nif" 0x53B5AF | **大気レイヤ NIF ロード** |
| **FUN_0053BDB0** | "Meshes\Sky\Clouds.nif" 0x53BE42 | **雲レイヤ NIF ロード** |
| **FUN_0044AC20** | "DefaultWeather" 0x44CC2D | **ESM デフォルト天候解決 [推定]** |

### 5-3. Sky シェーダクラス系統 — **[確定（型登録）]**

- **SkyShader (0xA8F8E8) / SkyShaderProperty (0xA90454)** の型登録 stub は .CRT 帯 0xA10E50 関数内（0xA110B6 / 0xA11286）に存在（O1-3 と同一の `push parent; push name; mov ecx,node; call 0x70E220` システム）。
- HLSL ファイル文字列: **"sky\p\sky.p.hlsl"(0xA8F81C) / "sky\v\sky_quad.v.hlsl"(0xA8F8A0) / "sky\v\sky.v.hlsl"(0xA8F8D0)** → 天空レンダリングも SM パイプライン（O1-3 の SM3Lighting と同一設計）[確定(観測)]。

### 5-4. 結論と残点

- **「天候データロード関数の特定」（s61 残件5）は本項で解明**: TESWeather ctor = 0x53D260、Sky 構築 = 0x4BEE70、レイヤ NIF = 0x53B3F0/0x53BDB0。
- 残点: SkyShader vtable 本体と天候遷移（fWeatherTransMin/Max/Accel 適用）の駆動関数は未深掘り → 残点 V-171d（小）。


## §6 O1-6 Portal/DMap カリング — **[確定（反証）]** EXE に Portal/DMap カリングは存在しない。Occlusion Query も D3D9 実装なし

### 6-1. 文字列走査結果（網羅）

`v183_o1_6_portal.py`: "NiPortalManager" / "NiPortal" / "NiRoom" / "DMap" / "Portal" / "portal" / "FogZone" の全 .rdata 検索で **該当文字列 0 件**（大文字小文字両対応）。

### 6-2. Occlusion 関連の実体

| 文字列 | VA | 参照 | 実体 |
|---|---|---|---|
| "ToggleOcclusion" | 0xA506CC | （スクリプトコマンド名） | コンソールコマンド |
| "Occlusion Query : %s" | 0xA4B7CC | **FUN_00500860** | **トグル表示のみ**: `[0xB42E97]` を sete トグル → 0xA32600/0xA4B7E4 のどちらかを 0x579B60（メッセージ出力）へ push して表示、`mov al,1; ret`。クエリ実行コードは含まない |
| "Occlusion Geom: %d , %d tri , %d wait lo..." | 0xA304DC | FUN_00407AE0 (0x40ACF2 sprintf) | 統計デバッグログ（tri 数/wait） |
| "bEnableBoundingVolumeOcclusion" | 0xA314B0 | **.text 参照 0 件**、.data 0xB02D3C に Setting デスクリプタ | Setting システム経由の設定のみ |

- **`call [reg+0x110]`（IDirect3DDevice9::CreateQuery, index 68）disp8/disp32 全レジスタ = 0 件** → **D3D9 Occlusion Query (IDirect3DQuery9) は実装なし** [確定(反証)]。s61_render §6.1 の「Occlusion query 非使用[推定]」は**正しい**（本束でバイトレベル裏付け）。

### 6-3. 結論

- **s61_render.md 残件6「ルーム/Portal/DMap カリングのコード特定」は実行不能タスク**（コードが存在しない）。TES4 Oblivion は Gamebryo の NiPortalManager 系を使わず、**セルベース + バウンディングボリューム（bEnableBoundingVolumeOcclusion 設定）** でカリングする構造 [確定（反証）/ 設定名から推定]。
- s61_render §6.1 の推定「外郭セルは PVS 的カリング / 室内: Portal/DMap 文字列と関数未発見 / Occlusion query 非使用」は**3 点とも正しい**。
- **移植示唆**: Android 移植で Portal/DMap 再実装は不要。カリングはセル/バウンディングボリュームベースで実装すれば原作挙動と同じ [確定]。


## §7 O2-a NiNode ツリー NIF 組立経路 — **[部分解明]** whole-file mold 経路を確定（唯一 caller 特定）+ BSA reader 15 callers を全分類。通常時の NIF 組立エントリは残点

### 7-1. 0x448E20（whole-file NIF mold）の経路 — **[確定(観測)]**

E8 callers 全走査（`v183_o2a_nif.py`）:

```
FUN_00445A10  (TestAllCells デバッグ/テスト系: 文字列 "TestAllCells.xls" 0x445E53、
               "**************** Base Object Models/Icons/Textures/Interiors" 群、
               "%s (%d,%d) - (%d,%d)" レポート書式)
  └ FUN_0044E720  (0x44EAA8 で call 0x448E20)
      - フォームタイプ 36 種テーブル [0xB081AC] / [0xB081D0]（36×4B 相隣接）を走査
      - [0xB05584]/[0xB055A4]/[0xB0558C] ガード、[edi+0xcd6] reentry フラグ
      - 子走査 0x4B2460 / 0x4B2360、型名取得 [vt+0xD4] → ログ 0x4A7A60
      └ FUN_00448E20  (NIF mold: 引数 flags (0x8000001, 3, 0, 1, 0x80000000))
```

- **0x448E20 の callers は 1 件のみ**（FUN_0044E720）→ whole-file mold は**テスト/アセット検証経路専用** [確定(観測)]。

### 7-2. 0x42C8E0（BSA reader）15 callers の全分類 — **[確定(観測)]**

| caller | 実体（実測文字列） |
|---|---|
| FUN_0042EE80（×4） | **BSA アーカイブロード**（"Loading archive %s"）← FUN_0042F4C0 |
| FUN_00450550 | **TESFile 圧縮レコード zlib 解凍**（"TESFile: ... ZLib inflate stream" ×3 文字列、"1.2.1"）※束D O6 の「zlib inflate VA」残点の実体候補 |
| FUN_00450C20 | **TESFile::GetChunkData**（"First/Second ReadFile() in GetChunkData failed"、callers **239 件** = ESM パーサ群 0x413130/0x416100(MGEF_Reader)/0x41B2D0/0x4259E0 等） |
| FUN_005744E0 | **フォントテクスチャロード**（"Data\Fonts" / "%s\%s.tex"） |
| FUN_006AA9C0 | **サウンド MMIO**（"Unhandled message %i in MMIOReadBSFileProc." / "data\sound\"） |
| **FUN_00760DA0** | **DDS テクスチャロード**（s61_stream §3.1 既知、0x760EDF 呼出し） |
| FUN_0078E310 | **SpeedTree ロード**（"CSpeedTreeRT::LoadTree - %s"、callers 1 = FUN_00560600） |
| FUN_007DACA0 | BSShaderPPLightingProperty 帯のリソース読込（§3 slot26/27 系） |
| FUN_008014E0（×2）/ FUN_00801C90（×2） | 0x80xxxx 帯リソース読込（Ni 系アセット） |

→ 0x42C8E0 は**汎用アセット/チャンク reader**であり「NiNode ツリー組立」専用ではない [確定]。

### 7-3. 結論と残点

- s61_stream §7-3「NiNode tree build from NIF using 0x448E20/0x42C8E0 callers」への回答: **両者は組立経路の入口ではなく「ファイル I/O 層」**。whole-file mold(0x448E20) はテスト系(FUN_00445A10→0x44E720)のみ、BSA reader(0x42C8E0) は 9 サブシステム共用。
- **通常時の NIF→NiNode ツリー組立エントリ**（TESForm の 3D 表現生成、NiNode Pool 0xB02020 使用）は本束で特定未了 → **新規残点 V-171e**（候補: NiNode pool alloc 呼出し元の上流、SceneGraph attach 経路）。
- pool 側の実装は §9（O2-c）で確定（NiNode Pool = 0xF4B×0x400000、s61_stream §2.2 の記載と一致）。


## §8 O2-b LOD テクスチャブレンド経路詳細 — **[確定]** ブレンド実体は「DistantLOD HLSL シェーダ」+ 生成済み LOD テクスチャ。CPU 合成は不存在

### 8-1. s81 A4 [PARTIAL] の残点への回答

s81 A4 が残した「exact blend code paths need deeper tracing of LODQuad/LODManager functions」への回答: **ブレンドコードは EXE の関数ではなく HLSL シェーダアセット内**にある（EXE 側はロードとパス設定のみ）。全文字列参照走査（`v183_o2b_lod_str.py` / `v183_o2b2_distlod.py`）:

### 8-2. LOD パイプライン完全マップ — **[確定(観測)]**

| 関数 | 証拠（文字列/内部メッセージ） | 機能 |
|---|---|---|
| **FUN_008115C0** | "tallgrass\1x\v\DistantLOD.v.hlsl" + "tallgrass\1x\p\highDetail.p.hlsl"、プロファイル判定 "BILLBOARD"/"VS_2_0"/"PS_2_0"、**プリコンパイルキャッシュ 4 種: "DISTLOD1%03i.vso"(vs_1_1) / "DISTLOD1%03i.pso"(ps_1_3) / "DISTLOD2%03i.vso"(vs_2_0) / "DISTLOD2%03i.pso"(ps_2_0)**。callers: FUN_007F9410 / FUN_00811ED0 | **DistantLOD シェーダロード + .vso/.pso キャッシュ**（SM1.x=vs_1_1/ps_1_3 と SM2.0=vs_2_0/ps_2_0 の二系統）。**ブレンドは HLSL 内で実装** |
| **FUN_004ECD60** | "Textures\LandscapeLOD\Generated\%i.%02i.%02i.%i.dds" + "%i.%02i.%02i.%i_FN.dds"（_FN = flat normal）、"TESTerrainLODQuad::BuildMesh called on LOD chunk that isn't ..."。callers: FUN_004ED560 | **TESTerrainLODQuad 生成: 事前生成済み地形 LOD テクスチャ読込 + BuildMesh** |
| **FUN_004EB1A0** | ".\Data\Meshes\Landscape\LOD\%i.%02i.%02i.%i.NIF"、"Found terrain LOD file for %i, %i in worldspace '%s'"、"Unable to add LOD for LOD space (%i, %i) because LOD already..."。callers: FUN_004EBC00 | **地形 LOD NIF ロード** |
| **FUN_004F28B0** | "Data\DistantLOD\%s.cmp"、climate/water type 解決メッセージ群。callers: FUN_004F2C10 | **DistantLOD .cmp（結合ファイル）ロード + クライメート/水型解決** |
| **FUN_004BD430** | "DistantLOD\%s_%i_%i.lod"。callers: FUN_00483D60 | **.lod インデックスファイルロード** |

- クラス名実測: **TESTerrainLODManager**（"Shared\TESTerrainLODManager.cpp" 0xA47A8F = __FILE__）/ **TESTerrainLODQuad**（0xA47D7C/DB0/DF4）/ **DistantLODShaderProperty**（".\DistantLODShaderProperty.cpp" 0xA8D15C、"DistantLODShader" 0xA94838、"DistantLOD.v.hlsl" 0xA94814）。
- ※ s81 A4 は "LODManager at 0xA47AA0 (LODManager.cpp)" としたが、正名は **TESTerrainLODManager**（TES 前置き）[訂正]。fLODQuadMinLoadDistance(0xA47B58) 等の GMST 設定も同一帯に存在。

### 8-3. 結論

- **LOD テクスチャブレンド = GPU シェーダ（DistantLOD HLSL）+ 事前生成 DDS**。実行時 CPU 合成は行わない。LODManager/LODQuad はパス構築・メッシュ構築・シェーダ割当のみを担当。
- **移植示唆**: Android では DistantLOD HLSL をそのまま翻訳（vs_1_1/ps_1_3 と vs_2_0/ps_2_0 の両系統を保有、SIMPLE 互換として vs_1_1 系を優先）すれば原作挙動を再現できる。Textures\LandscapeLOD\Generated の事前生成 DDS はオーサリング済みアセットとして扱う [確定]。

## §9 O2-c メモリプール拡張 — **[確定]** ページキャッシュ型プール + 擬似拡張（新規確保→コピー→旧解放）を解明

### 9-1. プールシステム全体像 — **[確定(観測)]**

`v183_o2a5_pool.py` / `v183_o2c_poolgrow.py`:

| 関数 | 動作（実測） |
|---|---|
| **FUN_00401F40**(this=0xB02020, capacity, elem_size, name) | **プール登録**: this+0x48 領域末尾（[this+0x4C] 配列 / [this+0x4A] word フリーカウント）へ (size, capacity, name) エントリ追加。登録実行は初期化 FUN_0040D4D0 内 0x40E5B0-0x40E61B |
| **FUN_00401F00**(size) | **malloc ラッパ**: `[this=0xB02020 + 0x54]` のアロケータオブジェクトの **vtable[+0x18]** を呼ぶ。デフォルト 0x4021C0 = CRT malloc。プール登録済みサイズなら FUN_00402600（プールキャッシュから）へ分岐 |
| **FUN_00401F20**(ptr) | **free ラッパ**: 同 `[+0x54]` の **vtable[+0x1C]**（デフォルト 0x4021E0 = CRT free） |
| **FUN_00401E60**(this, old_ptr, old_size) | **拡張本体**: (1) call 0x401580(size) 実サイズ取得 → (2) size を this+4 アライメントへ切り上げ（最小 8B）→ (3) **`mov ecx,0xB02020; call 0x401AA0(1, aligned)` で新規ページ確保** → (4) call 0x9812C0 (memcpy) で旧データ移行 → (5) 旧 ptr があれば `call 0x401D40(old_size)` で**プールへ返却**。失敗時は call 0x981AAC + vtable[0]（例外）。**callers 12 件**: 0x46AE86/0x46AF1D/0x46BA2C/0x46BA9C/0x46BAEC（NiT 系配列拡張）、0x51AD52、0x575DFA/0x575E7C/0x575F1F/0x576050/0x576154/0x576253（メモリブロック拡張） |
| **FUN_00401AA0**(1, size) / **FUN_00401D40**(size) | プールからのページ確保 / 返却本体 |
| **FUN_00400200** | **FreeUnusedPages**（s61_stream §4.2 の 0x402647/0x402741 callers = FUN_0040264C 内で使用） |
| **FUN_0040264C** | **MemoryPool manager 0xB32B80 の操作**（`call 0x401020(0xB32B80)` → [esi+0x10C] カウント × [esi+0x108] ワード配列を走査） |

### 9-2. pool 初期化実測（FUN_0040D4D0 内 0x40E5B0-0x40E636）— **[確定]**

```
call 0x401F40(0xB02020, ?,  0xD8=216B, 0xA3104C)   ; name @0xA3104C
call 0x401F40(0xB02020, 0x400000=4MB, 0xF4=244B, "NiNode Pool"@0xA31040)
call 0x401F40(0xB02020, 0x800000=8MB, 0x100=256B, 0xA3102C)
call 0x401F40(0xB02020, 0x400000=4MB, 0x108=264B, 0xA3101C)
[0xB02184] := 0x405D60; call 0x747BA0; call 0x6F98F0
```
- s61_stream §2.2 の「0xF4×0x400000 / 0x100×0x800000 / 0x108×0x400000」記載と**完全一致**（+ 0xD8×? エントリを s61 は取りこぼし）。

### 9-3. 「メモリプール拡張」の答え

- 拡張方式 = **固定キャパシティのページキャッシュ（NiNode Pool 等）**であり、プール自体の growth はしない。プール外または拡張要求数組は **FUN_00401E60 が「新規確保 → memcpy → 旧ページ返却」で擬似拡張**する（呼び出し元 12 件すべてがこの形）。
- アロケータ差替え点は **[0xB02020+0x54]** の vtable オブジェクト 1 箇所（+0x18 = alloc / +0x1C = free）。**Android 移植ではこの 1 オブジェクトを差し替えれば全体が移植可能** [確定]。
- s61_stream §4.2「Background loading uses pool the same allocator」の [Est] は、pool 共用（0x401F00/1F20 を全システムが経由）の実測により**支持** [確定(観測)]。


## §10 新規残点（本束で起票）

| ID | 内容 | 根拠/次手 |
|---|---|---|
| **V-171a** | **inner（NiDX9Renderer[+0x280]）の生成箇所 = CreateDevice 本体**が未特定 | [0xB42154]（IDirect3D9*）参照 21 site 全走査で vtable+0x40（CreateDevice）呼出 0 件、0xB28E00 = NiTPointerList<unsigned> グローバル（vtable 0xA7C694）で s61「present_params」は誤記。0x775DF0/0x775CC0 はアダプタ列挙（D3DADAPTER_IDENTIFIER9 0x468B 構築）。**B13 実機 .data ダンプ**または renderer Initialize 系深掘りで突破 |
| **V-171b** | **inner の実行時 vtable**（DIP/DP スロット同定不能） | `call [reg+0x148/0x14C]` disp8/16/32 全形式 0 件、inner フォワード既知は +0xE4/+0x104/+0x1AC のみ。primitive 発行は実行時 vtable ロード経由 [推定] → B13 依存 |
| **V-171c** | **BSShaderPPLightingProperty slot38/39（0x855240/0x85AE20）の描画パス設定本体**（シェーダ定数へのライト転記経路） | §3-5。O1-1 の「ライト値供給元」と接続。0x85xxxx 帯（0x852150/0x853580/0x854CD0/0x854E90）深掘りで解明見込み |
| **V-171d** | **SkyShader vtable 本体と天候遷移駆動関数**（fWeatherTransMin/Max/Accel 適用） | §5-4。SkyShader/SkyShaderProperty 型登録 stub（0xA110B6/0xA11286）は同定済み、vtable 未展開 |
| **V-171e** | **通常時の NIF→NiNode ツリー組立エントリ**（TESForm の 3D 表現生成） | §7-3。0x448E20 はテスト系のみ、0x42C8E0 は汎用 reader。NiNode Pool alloc 呼出し元の上流追跡が次手 |
| **V-171f** | s61_render §1.2 の件数データ（BeginScene 115 件等）の**実メソッド再対応** | §2-2 で vtable offset 表の系統的誤認を確定。件数は実在するため、各 site の vtable 元（inner vs 描画オブジェクト 0xA97A20 系 vs 他）を再ラベリングする作業（小） |

## §11 自己検証

### 11-1. 対象項目の最終判定

| 項 | 判定 | 摘要 |
|---|---|---|
| O1-1 SetLight 代替機構 | **[確定（機構）]** | D3D ライト API 全不使用（+0xCC/+0xD4 呼出 0 件）+ RenderState キャッシュ層（FUN_0077B000）→ inner +0xE4(SetRenderState) 転送構造 |
| O1-2 DrawIndexedPrimitive 経路 | **[確定（反証）]** | DIP/DP 直接呼出 0 件（全形式）。s61 の vtable 表はメソッド対応自体が誤認（§2-2 訂正表） |
| O1-3 BSShaderPPLighting | **[確定（クラス同定・型連鎖・vtable 42 スロット）]** | 正名 BSShaderPPLightingProperty、vtable 0xA91384、型連鎖 7 段、主要 slot 機能マップ |
| O1-4 シェーダハンドルテーブル | **[重複排除]** | v129 §2 で確定済み（§4） |
| O1-5 天候ロード | **[確定]** | TESWeather ctor=0x53D260 / Sky 構築=0x4BEE70 / Atmosphere=0x53B3F0 / Clouds=0x53BDB0 |
| O1-6 Portal/DMap カリング | **[確定（反証）]** | EXE に存在しない。Occlusion Query も CreateQuery 0 件 |
| O2-a NiNode NIF 組立 | **[部分解明]** | mold(0x448E20)=テスト系のみ・BSA reader(0x42C8E0)=汎用と確定。通常時エントリは V-171e |
| O2-b LOD テクスチャブレンド | **[確定]** | DistantLOD HLSL（SM1.x/SM2.0 二系統 .vso/.pso キャッシュ）で GPU ブレンド、CPU 合成なし |
| O2-c メモリプール拡張 | **[確定]** | 固定キャパシティページキャッシュ + FUN_00401E60 擬似拡張、差替え点 [+0xB02020+0x54] |

### 11-2. 重複排除の遵守

- v92（NiDX9Renderer vtable 0xA88EA4 / NiDX9RenderState 0xA8A9F4 / b42ec0 経路）、v129（28 クラス factory / +0x5E0 / [state+0xFF8] / gap getter）、v159（renderer+0x280 / +0x54 getter / 型連鎖根）、v173（+0x54 残余 / NiSourceTexture）で解決済みの項目は **§0-2 に明記し再分析していない**。本束の新規性: inner の D3D9 index 突合による「+0xE4=SetRenderState(+0x1AC=SetPixelShaderConstantF)」特定と v129 §4-2「raw ではない」判断の訂正（D3D9 は 119 スロットで +0x1AC は範囲内）。

### 11-3. 遵守事項

- git 不使用 / **docs・TODO_re49・session-memory・plan・台帳・cache 編集 0 件** / 既存 tmp/re 上書き 0 件（本ファイル v183 のみ新規）/ **ESM・ESP 本体読み取り 0 回**（ESM 再スキャン禁止遵守）/ Ghidra 呼出なし。
- 中間物は `%TEMP%\opencode\v183_*.py`（13 本）+ 出力 txt のみ。
- 本レポートは L1 report-only（分析専任）。TODO 行更新・ロードマップ KPI 更新は親セッションの判断事項。

### 11-4. 既存前提への訂正素材（親セッション承認要）

1. **s61_render.md §1.2**: vtable offset → メソッド名対応の全面誤認（§2-2 訂正表）。「DrawIndexedPrimitive 0件/SetLight 0件」の結論自体は正しいが理由が異なる（+0x78/+0xE4 は DrawIndexedPrimitive/SetLight ではない）。
2. **s61_render.md §1.1**: [0xB42160] は IDirect3DDevice9* ではなく **NiTArray<NiDX9AdapterDesc\*>**（vtable 0xA8A470）。0x775DF0 はアダプタ列挙オブジェクトの ctor。0xB28E00 は **NiTPointerList<unsigned int>** グローバルで present_params ではない。
3. **s61_render.md §1.3**: 0xBA7D98 は「デバイスラッパー」ではなく**リソースプール管理オブジェクト**（割当 +0x10 / 解放 +0x14、種別 0x2a/0x2b）。
4. **s61_render.md §4.1**: 「天候関連文字列: 未発見」→ 多数存在（§5-1）。
5. **s61_render.md 残件3**: 「BSShaderPPLighting」→ **BSShaderPPLightingProperty**（実在クラス名）。
6. **s61_render.md 残件6**: Portal/DMap は存在しない（再実装不要）。
7. **v129 §4-2**: 「inner は raw IDirect3DDevice9 ではない」→ D3D9 vtable index 突合（+0xE4/+0x104/+0x1AC）で **raw IDirect3DDevice9 互換**（119 スロット）。
8. **s81 A4**: "LODManager at 0xA47AA0" → 正名 **TESTerrainLODManager**（"Shared\TESTerrainLODManager.cpp"）。
9. **s81 Task D 補足（束D に有益）**: FUN_00450550 = TESFile 圧縮レコード zlib 解凍（"TESFile: Error initializing ZLib inflate stream" 等）= O6「zlib inflate 呼出 VA」残点の実体候補。

## §12 使用証拠一覧

### 12-1. 中間スクリプト（すべて `%TEMP%\opencode`・新規 13 本）

| スクリプト | 用途 |
|---|---|
| v183_util.py | PE セクション解析（.text=0x400C00/.rdata=0x400E00/.data=0x401400 差分を実測確認）+ capstone 5.0.7 skipdata + imm32/string/callers スキャン |
| v183_o1_1_setlight.py | FUN_0077B000 / FUN_007716E0 ディスアセンブル |
| v183_o1_1b_probe.py | [reg+0x280] 全書手 / call [reg+0xE4] 全形式 |
| v183_o1_1c_inner.py | vtable RTTI / 0x7641A0 / 0x76BA5B / FUN_0077F840 |
| v183_o1_1d_dev.py | renderer ctor (vtable 0xA88EA4 設定) / RTTI 名検索 |
| v183_o1_1e_cc0.py / v183_o1_1f_devcreate.py / v183_o1_2h_d3d9.py | 0x775DF0/0x775CC0 / 0xB28E00 / 0xB42154 全参照 |
| v183_o1_2_dip.py / v183_o1_2e_dip32.py | call [reg+0x148/0x14C] disp8/16/32 全形式・fnptr ロード全 site |
| v183_o1_2b..2f | 0xBA7D98 参照群 / 描画オブジェクト vtable 0xA97A20・0xA9AF38 / renderer slot29-30 |
| v183_o1_3_shaderlight.py / o1_3b..3g | BSShader* 文字列参照 / CRT 登録 stub 全 398 件抽出・型連鎖構築 / GetType→vtable / slot 機能・SEH 文字列 |
| v183_o1_5_weather_str.py / o1_5b / o1_5c | 天候・Sky 文字列全走査 / 参照関数 / TESWeather ctor・Sky 構築実測 |
| v183_o1_6_portal.py / o1_6b_occl.py | Portal/DMap/Occlusion 文字列走査 / CreateQuery 全形式・ToggleOcclusion 実体 |
| v183_o2a_*.py / o2b_*.py / o2c_poolgrow.py | 0x448E20/0x42C8E0 callers・分類 / LOD 文字列・DISTLOD シェーダ / pool 拡張実装 |

### 12-2. 主要実測値（抜粋）

- `call [reg+0xE4]/[+0xCC]/[+0xD4]/[+0x148]/[+0x14C]` 全レジスタ・disp8/16/32 = **各 0 件**
- FUN_0077B000: `mov eax,[esi+0xff8]; mov ecx,[eax]; mov edx,[ecx+0xe4]; push ebx; push edi; push eax; call edx`（キャッシュ一致時 skip）
- FUN_007716E0: state+0x64 ループ 8 回（esi 0x80..0x87、val=0）
- FUN_0077F840: `[state+0xFF8] := [renderer+0x280]`（old Release / new AddRef）
- 0x7641A0: `[esi+0x280]:=0` + memset(0x284, 0x130) = D3DCAPS9
- CRT 登録 stub 398 件、BSShaderPPLightingProperty chain 7 段（0xB45DA8→…→0xB3F684）
- BSShaderPPLightingProperty vtable = **0xA91384**（vtable[-1]=COL 0xAD0144、42 スロット）
- TESWeather ctor 0x53D260: sprintf "\Textures\Sky\%s_%s.dds" ×7
- pool 初期化: 0xF4×4MB / 0x100×8MB / 0x108×4MB / 0xD8×? （0x401F40 ×4）
- FUN_00401E60: align → 0x401AA0(1,size) → memcpy 0x9812C0 → 旧返却 0x401D40（callers 12）
- CreateQuery idx68 (+0x110) 全形式 = 0 件

（了 — v183_renderer_stream_residuals.md / 束C O1 六項目・O2 残3 の裁定・新規残点 6 件・既存訂正素材 9 件）
