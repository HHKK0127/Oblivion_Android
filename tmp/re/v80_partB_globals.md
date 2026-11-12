# V80: Part B 未解析高参照グローバル15件の深掘り（Oblivion.exe v1.2.0416 Steam）

- 作成日: 2026-10-25 / 手法: probe_harness.py（selftest **4/4 PASS**: bytes@0x504050=518b4c24188b5424 等、dec_index=20,465、rd() セクション対応済）+ decompiled.c 全文走査 + .text バイナリ imm32 スキャン + capstone 全関数リニア走査（store/read 分類）+ E8 呼出し辺スキャン
- 対象: `v76_data_relationship_map.md` Part B の 15 件（0xB43110/10C/108/104 = COM refcount 単例族、0xB02E24、0xB42CDD、0xB42754、0xB42E8C、0xB39B80、0xB430DC、0xB430F0、0xB3FD64、0xB42EB8、0xB35C24、0xB39E04）
- 中間成果物（すべて %TEMP%\opencode 配下・Git 管理外）: v80_partb_scan.py/_out.txt、v80_partb_ctx.py/_ctx.txt、v80_partb_callers.py/_callers.txt、v80_partb_sweep.py/_sweep.txt、v80_partb_final.py/_final.txt、v80_partb_diag.py/_diag.txt、v80_partb_diag2.py/_diag2.txt、v80_partb_diag3.txt、v80_partb_diag4.txt
- 本ノートは 1 ファイル新規作成のみ。既存ファイルの編集・コミット・メモリ追記はなし。

---

## §1 手法と refs の定義

refs（参照数）は 3 経路で独立に数えた。

1. **decompiled 参照**: `docs/Oblivion_decompiled.c`（約 107 万行）を走査し、`0xb43110`（bare hex）および `DAT_00b43110`（Ghidra のグローバル命名、`00`+6桁で word-boundary 一致）を含む行を計数。行→関数帰属は dec_index ヘッダ（`// FUN_xxx @ 0x`、20,465 件）の二分探索。**v76 表の refs 値 = この decompiled 出現回数**と一致（13/15 で行カウント一致、差異 4 件は同一行内複数出現: 0xB42754=155(出現)/138(行)、0xB39B80=92/87、0xB42EB8=79/72、0xB43104=164/163）。
2. **バイナリ imm32 スキャン**: .text（VA 0x401000、const 0x400C00）の raw バイト列にリトルエンディアン imm32 を全出現走査。各 site を capstone でカバー命令に attribution し、**オペランド先頭がメモリ且つ store 系命令（mov/add/sub/inc/dec/xor/and/or/adc/sbb/rol…）の場合のみ W（書込み）**、それ以外（mov 読み/cmp/test/fild/fld/push/lea）は R と分類。decompiled には現れない thiscall global-this 読手（`mov ecx,[0xb43110]` 等）を捉えるための経路。
3. **E8 呼出し辺**: .text 全位置の E8 rel32 を解釈し、dest = 関数先頭（dec_index ヘッダ集合）に一致するもののみ採用（誤検出排除）。setter/init 関数の呼び出し元＝上位システム特定に使用。ラベルなし関数（後述 0x7B8530 型）は dest 非一致になるため、ラベル不要モードで個別再走査した。

既知の帰属バイアス（報告に反映済み）: dec_index はヘッダ付き 20,465 関数のみを収録し、**FUN ラベルなしの実コード領域**（例: 0x7B8530–0x7B890A、0x50AA20、0x405150）は「直前のヘッダ関数」に誤帰属する。逆に decompiled に本文の無いヘッダ関数も存在（fun_00820910 は 64 site のバイナリ読みに対し decompiled 記述 0 件 = thiscall global-this の描画省略）。

---

## §2 15 件の判定表

refs = v76 値（decompiled 出現回数）/ imm32 = .text 直接出現数。判定 [確定] = バイナリ+decompiled+文字列/テーブルの三重証拠、[仮説] = 構造証拠十分でクラス名未同定、[不明] = 到達経路等が未特定。

| VA | refs | imm32 | type仮説 | 主要 writer（書込み site） | reader 例 | 判定 |
|----|------|-------|----------|---------------------------|-----------|------|
| 0xB43110 | 224 | 262 | refcounted obj ptr（+0 vtable, +4 refcount） | FUN_007B8990 (0x7BA022, shader init 本体) / clear: FUN_007B7250 (0x7B74FF), FUNラベルなし 0x7B8530 (0x7B8744) | fun_00864a60 等 = `FUN_0076c910(DAT_00b43110)`（this+4 NiPointer へ addref 代入, 16/16/14/12 回）, fun_00820910 (64 loads, decompiled 不可視), fun_00873e50系26関数が 108/10C/110 三役比較 | 仮説(高) |
| 0xB4310C | 200 | 100 | refcounted obj ptr | setter FUN_00441850 (0x441884; InterlockedDecrement(ptr+1)→vtable[0](1) リリース→addref) / clear: fun_004425d0/00444340/00444c70/00482670/004826f0/0055f550 | fun_0083bbf0系（B43108 とペア比較）, fun_007e8e10 = `FUN_0076c910(DAT_00b4310c)` | 仮説 |
| 0xB43108 | 188 | 96 | refcounted obj ptr | setter FUN_0055E340 (0x55E374; 唯一 caller = FUN_0055FDF0、`*(B39E04+4)` を引数渡し) / clear: fun_0055f550, fun_00560110 | fun_0083bbf0系（B4310C とセット）, fun_007e8e10, 0x873e50系比較 | 仮説 |
| 0xB43104 | 164 | 159 | refcounted obj ptr（デバイス/レンダラ風） | FUN_00406950 (0x406A56; `B43104 = DAT_00B350D8` の値を addref 付きコピー) / clear: FUNラベルなし 0x7B8530 (0x7B88FB) | fun_007c2450 = `FUN_007c1b50(B43104, w,h, D3DFMT 0x24/0x26, usage 0x71)` ×6 = RT/テクスチャ生成デバイス, vtable+0x7C 問い合わせ ×3, Shader_CompileVertex/Pixel | 仮説(高) |
| 0xB02E24 | 157 | 231 | **int 値（INI 設定ノード value フィールド）** | ランタイム絶対 store は .text に **0 件**。初期値 0x0A（.data）、INI パーサがノードポインタ経由で書換。ノード登録: FUN_009D9500 `push 0xb02e24; mov ecx,0xb07bf0; call FUN_004040E0`（登録系 637 の 1 つ） | fun_00407ae0: `0x500 - DAT_00b02e24`（79 site）、`fild dword[0xb02e24]`→(float)（113 site） | **確定** |
| 0xB42CDD | 156 | 157 | byte フラグ | FUN_007ABE30 (=0, エフェクト系 ctor 内 L664423) / FUN_007ACF40 (=1, 適用ループ入口) / FUN_007AD270 (=bl, vtable 0xA8CCB4 経由メソッド) | fun_00848fa0 系 78 関数が `cmp byte ptr [0xb42cdd],0` ゲート（154 site） | 仮説(高) |
| 0xB42754 | 155 | 10 | **レンダラ実体 obj ptr** | FUN_00772060 (0x772066, 唯一の setter) / clear: FUN_00779010 → FUN_00772060(0) | fun_00771830: +0xA00..+0xA3C の 4x4 float 行列, fun_007716e0: vtable+0x54 ビット(&8,&4), fun_007721e0: +0x8B8 vtable 呼び出し | **確定(高)**（§4） |
| 0xB42E8C | 125 | 61 | code ptr（シェーダエラーコールバック） | 既定値 0x405150 を FUN_0040D4D0 が set (0x40E8C1) / 一時差替え: FUN_0050AA20 (RefreshShaders ハンドラ) が 0x50AA10 に差替→復元 (0x50AA27/0x50AA36) | fun_00852470: `(*DAT_00b42e8c)("SHADER ERROR : no shader to handle AMBDIFFTEX_...")` ×44, Shader_CompileVertex/Pixel ×6 | **確定** |
| 0xB39B80 | 92 | 97 | 大型 obj ptr（0xDBC バイト, 遅延生成） | FUN_00553550: `malloc(0xDBC)→DAT=FUN_00553140()` (0x55359E/0x5535B5), 失敗時 =0 (0x5535EB) | fun_005547f0: "FaceGenEyeLeft/EyeRight"、"FaceGenHair"、"Meshes\\%s"、"Textures\\Characters\\Eyes\\EyeDefault.dds"、+0xDAC フィールド | 仮説(高) |
| 0xB430DC | 84 | 79 | refcounted obj ptr | FUN_007B8990 (0x7B966E, =puVar6) / clear: FUN_007B7250, FUN_007B84E0 後半, 0x7B8530 領域 | fun_0084abc0 等: `puVar7=DAT_00b430f0; if(条件) puVar7=DAT_00b430dc`（フォールバック選択） | 仮説 |
| 0xB430F0 | 80 | 75 | refcounted obj ptr | FUN_007B8990 (0x7B9752, =puVar6 ※DC と同時 set) / clear: 同上 | fun_0084abc0 等（フォールバックの主選択肢） | 仮説 |
| 0xB3FD64 | 82 | 83 | LONG カウンタ | 直接 store **0 件**。InterlockedIncrement/Decrement のみ（push &DAT 形式 52+ site、83 関数が 1 回ずつ） | ctor/dtor ペア: FUN_004A24F0 (ctor, vtable 0xA3FD80 系, +1) / FUN_004A2970 (dtor, −1) 等 83 組 | 仮説(高) |
| 0xB42EB8 | 79 | 70 | obj ptr（エフェクト/パラメータブロック） | FUN_007A9820 (0x7A983B, =param_1; 同時に 0xB42E90=param_2) / FUN_007AAA30 ×2 / 0x7F6FC0..0x7F9410 系（テクニック選択）/ clear: 7250, 7B8530, 7BA2F0, 7D21F0 | fun_007e8e10: `*(B42EB8+0xc)`、`*(*(B42EB8+0xc)+0xf4)` 判定 | 仮説 |
| 0xB35C24 | 76 | 81 | refcounted obj ptr（現在設定チェーン） | FUN_004D5600 (0x4D5667; 三つ組 0xB3F9A8/AC/B0 を FUN_004D5600 内で FUN_004D5000 に渡し構築、同時に _DAT_00B35C00=0)。**FUN_004D5600 自体は E8/imm32/データ参照とも 0 件 = 到達経路不明** / clear: FUN_004CA390 (caller fun_00483460) | getter FUN_004440C0（`if(FUN_004C97F0()) return FUN_00424180(); return DAT`）、Load_ProcessChangeRecords(FUN_0045FDA0) ×6、fun_00444340/00444fb0/00445a10 | 仮説（writer 不明点付き） |
| 0xB39E04 | 72 | 70 | obj ptr（+0=フィールド, +4=インターフェス） | factory FUN_0055E850（caller: fun_0055fdf0×3, fun_0055f750, fun_0055fcb0）/ writer: fun_0055f750/fun_0055fcb0/fun_0055fdf0（0x55FFF0/0x560077/0x5600E7 等） | fun_0055e390（15 R）: `*(int*)(*B39E04+4)`、`*(int**)(*B39E04+8)`。+4 → FUN_0055E340 経由で B43108 へ供給。FUN_0055FDF0 内に "Data\\Textures\\Trees\\CanopyShadow.dds" | 仮説 |

annotate_db（368 functions）との突合: 15 件の主要 writer/reader のうち名前付きは少数 — **fun_0045fda0=Load_ProcessChangeRecords**（B35C24 reader）、**fun_008014e0=Shader_CompileVertex / fun_00801c90=Shader_CompilePixel**（B42E8C/B42EB8/B43104 reader）、**fun_00462b20=Load_ReadGlobalData・fun_00445a10 経由**、fun_0050a8a0 は無名。constants 節は空（0 件）のためグローバル名の裏付けはなし。opcode テーブル経由の同定が有効だった: **0xB0BC30 = entry82 `RefreshShaders`/REFRESHSHADERS (opid 0x133) の +0x18 = FUN_0050AA20**、**0xB0FE70 = entry506 `ShowViewerStrings`/svs (opid 0x1157) の +0x18 = FUN_0050FF70**。

---

## §3 COM 単例族（0xB43104/108/10C/110）の構造推定

### 結論: 「1 オブジェクトのフィールド」仮説は**棄却**。4 つの独立 refcounted singleton ポインタが BSS に物理連続して並ぶだけ

```
0xB43104  dword  ← FUN_00406950 が set（値は 0xB350D8 のコピー+addref）   … シェーダ/デバイス系 mirror
0xB43108  dword  ← FUN_0055E340 が set（値 = *(0xB39E04+4), 樹木系）      … tree/interface 系
0xB4310C  dword  ← FUN_00441850 が set（ShowViewerStrings 等が経路）      … 設定/ビューア系
0xB43110  dword  ← FUN_007B8990 が set（shader init; clear は 7250/7B8530）… シェーダ主要 singleton
```

各スロットが指す先オブジェクトは Gamebryo NiRefObject 形（**+0 = vtable、+4 = Interlocked refcount**）。setter は全スロット同一パターン:

```c
void setter(T *p) {                       // 例: FUN_00441850 / FUN_0055E340
  if (g_slot != p) {
    if (g_slot && InterlockedDecrement(g_slot+1)==0) (**(code**)*g_slot)(1);  // vtable[0] へ release(1)
    g_slot = p;  if (p) InterlockedIncrement(p+1);
  }
}
```

**構造体1つ説を棄却した根拠**:
1. **独立 setter 4 本**: FUN_00406950（初期化）/ FUN_0055E340（樹木系）/ FUN_00441850（設定系）/ FUN_007B8990（シェーダ init）— それぞれ別サブシステムに属し、同一関数が複数スロットを set する事例は存在しない（同時触りは clear 系のみ）。
2. **独立 release ヘルパ 4 本**: FUN_00A27160（104）/ FUN_00A27490（108）/ FUN_00A274C0（10C）/ FUN_00A27340（110）— 末尾 4 本は decompiled 上 `InterlockedDecrement(DAT+1)` のみの teardown 関数。構造体なら refcount は1つのはず。
3. **refcount が各スロットの指し先に存在**: `InterlockedDecrement(DAT_00b4310c + 1)` は変数自身+4 ではなく「変数の値（ポインタ）+4 バイト」= **指し先オブジェクトの refcount** を叩く。4 スロット分の独立 refcount 運用が成立。
4. **base+disp 相互参照が 0 件**: `[0xB43104+0x4]` 形式の命令は .text に存在しない（全 site が絶対アドレス単体）。
5. **2 つ以上を同居触る関数は「束で扱う読手」のみ**: (a) FUN_007E8E10 = 108→10C→110 の順に `FUN_0076c910`（NiPointer assign）/リリース、(b) **FUN_00873E50 系 26 関数**（0x873e50..0x881b80）が同一引数を 110/108/10C の3つと順に比較、(c) FUN_0055F550 が 108 と 10C を連続 release。— これは「論理的に束（シェーダ/エフェクトが参照する3点セット）」であって物理構造体ではない。

**付随発見（帯域全体像）**: 同一 BSS 帯 0xB430D4..0xB43110 にはさらに 0xB430DC / 0xB430F0 / 0xB430EC（v76: refcount リリース確認済）/ 0xB430D4（FUN_007B8990 L673653 が `0xB430D4 = DAT_00b43110` + addref = 110 のキャッシュ）が並び、**すべてシェーダ初期化 FUN_007B8990 / teardown FUN_007B7250 / リセット FUNラベルなし 0x7B8530 の3本の writer 族**で管理される。0xB43108/10C/110 を束で比較する 0x83xxxx〜0x88xxxx の大関数群は、この3スロットを per-object NiPointer に代入して回っている。

**ラベルなし関数 0x7B8530 の発見（副作用）**: FUN_007B84E0 の ret (0x7B8522) の後、int3 パディング 13 バイトを挟んで **0x7B8530 に FUN ラベルなしの独立関数**（prologue `push ebx/ebp`、arg に bool、0xB42EBC=0 クリア、B430DC/B430F0/B43110/B43104/B42EB8 を順に release し 0 化、ret at 0x7B890A）が存在する。E8 呼出しは **FUN_00497B20 の 1 件のみ**。Ghidra/decompiled.c のいずれにも本文が無く（dec_index にも defs にも不在）、V79 で議論した「FUN ラベル未生成の実在関数」の実例。役割 = **シェーダ系シングルトン一括リセット**。

---

## §4 0xB42754 = レンダラ実体論（証拠鎖）

**結論: 0xB42754 は Gamebryo レンダラ実体オブジェクトへのポインタ。Direct3DCreate9/device 作成の直後、同一初期化チェーン内で set される。**

証拠チェーン（すべてバイナリ E8 辺 + decompiled 本文で確認）:

```
FUN_00498340  … "Unknown error creating the Gamebryo Renderer." / "GetAdapterDesc() failed." /
                 "NiXAdapaterDesc::GetDeviceCaps() failed." / LoadLibraryA("ATIMGPUD.dll"|"NVCPL.dll") /
                 "%sRendererInfo.txt" へ "Renderer Device Information:" 出力、DAT_00b43070=0/1（シェーダフラグ, v76 Y7 と同一関数）
   └─ call FUN_0076BD40 (0x498519)
         ├─ 0x76BD40 先頭: call FUN_00761DF0 …… DAT_00b42154 = Direct3DCreate9(0x20)（v76 Hub4）
         └─ 0x76BDDA: call FUN_0076A1C0（15 引数。ウィンドウ/バックバッファ寸法等を param_1[0x296..0x2a3] に格納）
                └─ 0x76A76A: call FUN_00778F60(renderer)
                       ├─ FUN_00772060(param_1) …… DAT_00b42754 = param_1  ← **SET**
                       ├─ DAT_00b42758 = *(renderer+0x8AC)（描画ステート群をキャッシュ）
                       └─ DAT_00b428A8 = shader program factory（"Failed to create shader program factory!"）
```

clear 経路: FUN_0076BD40 側ではなく **FUN_0076B5D0 → FUN_00779010 → FUN_00772060(0)**（レンダラ破棄/リセット時に 0 化、あわせて FUN_0075FB30(0), DAT_00B42834=0）。

**指し先オブジェクトの型（フィールド証拠）**:
- +0x000 = vtable。vtable+0x54 の呼び出しは能力ビット（&8 / &4 判定, FUN_007716E0）。
- +0x280 = refcounted 子オブジェクト → キャッシュ先 0xB42750（release vtable+8 / addref vtable+4 を明示）。
- +0x8AC = 描画ステート群オブジェクト → キャッシュ先 0xB42758（vtable スロット 0xC8/0xD0/0xE0/0x104 を RenderState/TextureStage 設定系として使用）。sibling 0xB42834 にも転記。
- +0x8B8 = 別オブジェクト ptr（vtable+8 メソッドが文字列/リソース解決, FUN_007721E0）。
- **+0xA00..+0xA3C = 4x4 float 行列**（行×3+平行移動、FUN_00771830 が param のベクトル群と乗算 = ビュー/プロジェクション変換）。

以上より「D3D9 device + ビュー行列 + 描画ステートを保持するレンダラ/カメラ複合体」であり、v76 の「renderer object?」仮説を **確定(高)** に格上げ。Direct3DCreate9 との前後関係は「同一 fun_0076bd40 内で create9 → attach/set」の順で明確。

---

## §5 残不明点

1. **0xB350D8（B43104 の供給元）**: FUN_00406950 は `B43104 = DAT_00B350D8` を addref 付きで行うが、0xB350D8 自体の writer は本タスク範囲外（imm32 33 site、0x4061C3/0x4104EA/0x41178D/0x4816E1 等）。Ni システム初期化系に見えるが未同定。
2. **FUN_004D5600（B35C24 setter）の到達経路**: E8 呼出し 0 件、imm32 参照が .text/.rdata/.data 全セクションで 0 件（0x4D55D0 ret → int3 → 0x4D55E0 別関数 → int3 → 0x4D5600 prologue と独立開始）。デッドコードまたは未検出の間接経路。同様に FUN_0050FF70 は opcode entry506 (+0x18) 経由、FUN_005DF160 は .rdata 0xA6A0A8 の vtable スロット経由と判明したが、B35C24 の実行時構築タイミングは未特定。
3. **B4310C と ShowViewerStrings (opid 0x1157)**: FUN_0050FF70（svs ハンドラ）が FUN_00441850 経由で B4310C を set する意味論（ビューア文字列表示がなぜ設定系 singleton を差し替えるか）未解明。fun_004826f0 の 2 箇所 set/clear とも未精査。
4. **FUN ラベルなし関数の系統**: 0x7B8530（shader リセット）/0x50AA20（RefreshShaders 本体だが opcode 表由来で判明）/0x405150（既定シェーダエラー logger、260B スタックバッファで整形）の 3 件を検出。「ret→int3→次の prologue」パターンで全 .text を走査すれば同種がもっとあるはず（V79 の gap 分析と接続可能）。
5. **0xB39E04 の厳密クラス名**: CanopyShadow.dds から樹木系と推定したが、+0/+8 フィールド構造（`*obj`、`*(obj+4)`、`*(*obj+4)`、`*(*obj+8)`）の完全な解釈は未了。
6. **B430DC と B430F0 のペア意味論**: FUN_007B8990 が同一 puVar6 を両方に set する点、読手が F0→DC の順にフォールバックする点を確認済みだが、両者が同一値で alias するのか別値で入るのか（puVar6 の再代入追跡）は未確定。
7. **refs 定義の差**: v76 refs（出現回数）と行カウントの差 4 件は同一行内複数出現。混同しないこと。

---

## §6 未解決 → 次タスク案

1. **[高] 0xB350D8 と B43104/B42754 の三角関係**: 0xB350D8 の writer 追跡（0x4061C3 近傍）→「初期化順序: B350D8 生成 → B43104 へ mirror → FUN_007C2450 が RT 群生成」の復元。B42754（レンダラ）と B350D8 が同一オブジェクトかを rd 値比較ではなく setter 経路で判定。
2. **[高] FUN ラベルなし関数の全数列挙**: ret+int3 パディング境界検出スクリプトで「dec_index に無い独立関数」を全 .text から抽出（0x7B8530 型）。V79 の gap357 分類（class2/3）と突合し、opcode テーブル +0x18 の 5 ゼロエントリ等との関係を確認。
3. **[中] 0xB07BF0 INI 設定リストの全ノード走査**: FUN_004040E0 の this=0xB07BF0（head/tail at +0x10C/+0x110）を起点に、.data の {int value, char* "Name:Section"} ノード群を機械列挙し、Oblivion.ini の全既定定値テーブルを復元（Android 移植の INI 既定値テーブルになりうる）。B02E24 はこの中核例。
4. **[中] RefreshShaders (opid 0x133) → FUN_007B81A0 のシェーダ再生成経路**: 0xB42E8C コールバック差替えの実用価値（Android 移植時のエラー報告系）と、シェーダキャッシュ再構築の手順復元。
5. **[中] B42754 の vtable 全スロット同定**: +0x54/+0xC8/+0xD0/+0xE0/+0x104 等を既存 NiDX9Renderer スロット表と突合（RE_EXE_REPORT の Renderer 節と接続）。
6. **[低] B3FD64 を跨ぐ 83 クラスの列挙**: ctor/dtor ペア 83 組から対象クラス族（debug 用 instance counter か）を vtable 0xA3FD80 起点で同定。
7. **[低] B43110/B43108/B4310C の実体クラス**: FUN_0076C910 で代入される先（this+4 NiPointer）の所有クラス群（0x83xxxx-0x88xxxx）をたどり、singleton の正体（shader program? effect? tree?）を確定。

---
検証: `python probe_harness.py selftest` → 4/4 PASS（本ノート作成前後とも実行）。バイナリ・decompiled.c・既存 docs は無変更。作成ファイルは本ノート 1 件のみ。
