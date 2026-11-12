# v191 — engine/gameplay 残点束（v184 §5 の10件 + v185 §4 の10件）の解析
# =====================================================================================
# 対象: Oblivion.exe v1.2.0416 (Steam, ImageBase 0x400000)
# 日付: 2026-10-29 / TODO serial 179 (V179・中) / 解析専任（docs/TODO は未編集）
# 出典: v184_engine_small_residuals.md §5（新規残点10件）/ v185_gameplay_residuals56.md §4（R1..R10）
# 方法: capstone 5.0.7 skipdata 逆アセンブル + disp32/E8/E9 生バイト全走査 + .data 初期イメージ dump
#       + Ghidra FID 名台帳（cache/named_funcs.txt）+ v174_gap_residuals2.md §3（R-l）接続
# 凡例: [確定]=命令列/データ直接確認 / [確定-構造]=構造からの必然 / [推定]=状況証拠 / [未解]=残点
# 証跡: %TEMP%\opencode\v191_{util,b1,b1b,b2,b2b,b3a,b3b,b3c}.py + v191_*_out.txt（§4）

## 手法 / 入力

- **対象バイナリ**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / ImageBase 0x400000）。
- **読取のみの入力**: `tmp/re/v184_engine_small_residuals.md`（§5 残点10件 + 本文）、`tmp/re/v185_gameplay_residuals56.md`（§4 R1..R10 + F3-2/F4-4/F5-1..6 本文）、`tmp/re/v174_gap_residuals2.md` §3（R-l: .CRT$XCU）、`tmp/re/cache/{named_funcs.txt, call_targets.json}`（引用のみ）。
- **セクション**: .text VA 0x401000..0xA282E9 / .rdata 0xA37000..0xB0F3C2 / .data 0xB02000..（file-backed 0xB32F40、以降 virtual=.bss 相当）。
- **ハーネス**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時 2026-10-29）。中間物は `%TEMP%\opencode\v191_*` のみ。
- **順守**: docs/・既存 tmp/re・TODO・台帳・git・Supermemory 無編集、ESM 再スキャンなし、成果物は本 1 ファイルのみ。

## 0. サマリ

| 束 | 実施 | 解明 | 内訳 |
|---|---|---|---|
| v184 §5（エンジン側 E1..E10） | 10 | **10** | [確定] 9 + [確定-構造/推定-強] 1（E1 +0x1BD5 の値意味） |
| v185 §4（ゲームプレイ側 G1..G10 = R1..R10） | 10 | **10** | [確定] 8 + [確定-構造] 2（R1 死帯 [確定-強]、R4 [確定-構造]） |
| **合計** | **20** | **20 解明** | 新規残点 5 件（§3） |

主要な前提訂正: (a) **bAllowConsole の Setting は 0xB135C0（既定=1）で 0xB135C8 ではない**（後者は fMenuPlayerLightAmbientRed=0.25f）、(b) **0x497C10 は解像度適用本体ではなく 2 スロット setter**、(c) **0xA41D00 帯はファイル全体で参照ゼロの死 vtable 帯**（v185 の 0x8A5C5E「参照」は call バイト列との偶然一致）、(d) **atan2 には SSE 実装が存在しない**、(e) s60_crt「0x4842F0=truncf」→ **floorf**。

---

## 1. v184 §5（エンジン側 10 項目）

### E1-（v184 §5-1）. inputMgr +0x1BD5 書込値 `0x0B` の意味 — 解明 [確定-構造] / 意味 [推定-強]

- **write-only は確定**: disp32 全走査で +0x1BD5 参照は 0x403B02 の 1 箇所のみ（v184 O4-4 の再確認）。隣接 +0x1BD2/0x1BD6 参照は 0 件、+0x1BD4/+0x1BD7/+0x1BD8 は ResetBindings 内書込と別構造体（0x7CDAxx = スタック）。
- **書込み文脈の全読（0x403960 ResetBindings）**: gamepad 既定ブロックの書き込みは **keycode 連番**で並ぶ（v184_o4b 相当を完全再逆断）:
  0x1BC1=0x00, 0x1BC5=0x01, 0x1BC0=0x02, 0x1BBD=0x03, 0x1BC7=0x04, 0x1BC6=0x05, 0x1BBF=0x06, 0x1BD4=0x07, 0x1BBE=bl(0x08), 0x1BBC=al(0x09), 0x1BC8=**0x0A**, **0x1BD5=0x0B**, 0x1BCA..0x1BD1=0x20..0x27（POV 8 方向）。
- **構造的位置**: バインディング表は 3 デバイス×29 アクション（0x1B7E..0x1BD4）。0x1BD5 = 0x1B7E+3×0x1D = **第4 デバイス行 action0 相当の越境アドレス**（オブジェクト総サイズ 0x1BD8 の末尾 3 バイト内）。device 3 へのクリア（0x1B7E/0x1B9B/0x1BB8 の 3 行）は 0x1BD5 を含まない。
- **判定**: 値 `0x0B` は **HID ボタン 11（btn11）の keycode** で、gamepad 既定 12 ボタン割当（btn0..btn11）の第 12 番目が表外 1 バイトに書き込まれたもの [推定-強]。読み手が存在しない以上「バージョン/フラグ」説は命令列で否定できないが、連番 0x0A→0x0B→0x20 の並びが btn 連番であることは確定。

### E2-（v184 §5-2）. [0xB399C4] / [0xB399C8] の読手 — 解明 [確定]

- **実体**: FUN_009f5940 は **31 個**（index 0x00..0x1E）の 8 バイト Setting オブジェクトを 0xB39950..0xB399C8 へ連続構築（各 `new(8) → 0x419A70(name, default)`）。該当 2 つ:
  - **[0xB399C4] = {name 'sXboxRThumbRight'(0xA621C8), default 'move the Right Stick right'(0xA621DC)}**（index 0x1D）
  - **[0xB399C8] = {name 'sXboxNone'(0xA621BC), default 'assign a button in the Controls menu'(0xA62194)}**（index 0x1E）
- **読手 0 件を構造で確定**: 消費者配列は **[esi*4+0xB399D0] の 29 要素（0..0x1C）**（0x40C652 / 0x575D53 / 0x575D73 / 0x59BC47）。0xB399C4/C8 はその**直前 2 スロット（index 29/30）で配列外**。dword 参照は FUN_009f5940 の書込のみ、周辺 0xB399B0..0xB399C0 の各参照も全て書込み。
- **用途**: string GMST エントリ（ESM ロード時に名前解決で value が差し替わる型。F3-1 の sSkillIncreased と同型）[確定-構造]。ControlsMenu の Xbox 用「未割当表示/右スティック項目」の GMST [推定-強]。コードからの直接読手は設計上存在しない。

### E3-（v184 §5-3）. bAllowConsole の実行時ゲート — 解明 [確定]（v184 の前提を訂正）

- **前提訂正（最重要）**: Setting オブジェクトは **{u32 value(+0), char* name(+4)} の 8 バイト**（getter thunk 0x403BC0 が [ecx+4] の name 先頭を検査する dtor 仕様と .data 初期イメージの両方で確定）。よって:
  - **bAllowConsole = オブジェクト 0xB135C0（value = 1 = int true、既定値 1 [確定]、name = 0xA694E8 "bAllowConsole:Interface"）**
  - **0xB135C8 は fMenuPlayerLightAmbientRed（value = 0.25f）**。v184 の「0xB135C8 の既定 0.25f が解釈不能」の原因はこれ（bAllowConsole ではなかった）。登録スタブ: 0x9FB540 = `push 0xB135C0`（bAllowConsole）/ 0x9FB5A0 = `push 0xB135C8`（fMenuPlayerLightAmbientRed）。getter thunk: **0x9BEEE1**（bAllowConsole）/ 0x9BEF10（fMenuPlayerLightAmbientRed）。
- **0x57E6AA 帯の正体**: `fld [0xB135E0/E8/F0]` と `fld [0xB135C8/D0/D8]` を [eax+0xEC..F4] / [eax+0xE0..E8] へ格納 = **MenuPlayerLight の diffuse RGB(0.8×3) / ambient RGB(0.25×3) 一括コピー**。bAllowConsole とは無関係 [確定]。
- **実行時読手は 0 件** [確定]: 0xB135C0 のコード参照は (a) getter thunk 0x9BEEE1（E8 呼出 0 件）、(b) 登録スタブ 0x9FB541、(c) .rdata の atexit 破棄 thunk 0xA246D0（`push 0xB135C0; mov ecx,0xB07CFC; call 0x65C620`）のみ。コンソール経路（O4-1: grave → 0x403520 → 0x58373A 帯 → 0x57D640）にも直接読取なし。0xAE8560 表は [0]=0x9BEF10（fMenuPlayerLightAmbientRed thunk）+ タイムスタンプ 0x19930522 の 1 エントリのみで、消費者も 0 件。
- **残る可能性**: INI 読込時のみ 0xB07BF0（Setting manager）経由で値が書き込まれ、実行時の利用は manager の間解決（名前/ポインタ再取得）経由 [未解・静的限界] → §3-(2)。

### E4-（v184 §5-4）. CRT 静的初期化表 0xA2A540.. の走査主体 — 解明 [確定]（v174 R-l と完全接続）

- **帯の同定**: 0xA2A530..0xA2A6BC+ は 0x9F2AE0..0x9F6A50 帯の init スタブへの連続ポインタ（92/92 エントリが XCU の値域 0x9D7E40..0xA163C0 内）。**0xA2A644 = FUN_009f5940**（E2 のアクション名テーブル初期化）もこの帯に登録されている。
- **走査主体**: v174 §3 の R-l 結論どおり **0x981C6B の _initterm ループ**（`mov esi,0xA283D0 / mov edi,0xA2F77C / mov eax,[esi] / test / call eax / add esi,4 / cmp jb`）[確定]。E8 caller は **0x9877E1 の 1 件のみ**（_DllMainCRTStartup 相当）— 本調査で再実証。
- 0xA2A540..0xA2A6AC 帯は XCU 配列 0xA283D4..0xA2F77C の**中間帯**であり、特別な第 2 走査者は存在しない。v184 §5-4 の「実行主体（start/end 界）の突合」はこれで完了。

### E5-（v184 §5-5）. sin/cos「dispatch」値の不整合 — 解明 [確定]（親裁定材料）

**裁定材料（全て命令列で直接確認）**:

| VA | 実体 | SSE 先 | 呼出 | 判定 |
|---|---|---|---|---|
| **0x986300** | **`sin`（matherr 付き主入口）**: [0xbaabdc] チェック → x87 path 0x98633B（__fload_withFB 0x991B88）→ 共通 x87 本体 **0x986358（fsin + name 'sin'@0xB30D70 refs 0x986389/0x9863DE, edx=0x1E）** | **E9→0x996980**（wrapper）→ 本体 **0x99699E** | **69** | ゲーム常用 |
| **0x9862C0** | **`_sin`（matherr なし副入口）**: je→0x98634F（0x991B45 経由）で x87 本体 0x986358 へ合流 | E9→0x996998（本体 0x99699E へ直結スタブ） | **0** | 未使用 |
| **0x986000** | **`cos`（主入口）**: x87 本体 0x986058（fcos + name 'cos'@0xB30D50） | **E9→0x9955A0**（wrapper）→ 本体 **0x9955BE** | **98** | ゲーム常用 |
| **0x986680** | **`_cos`（副入口）**: je→0x9866F7、共通 x87 本体 0x986058 | E9→0x996BD8（別実装の cos SSE） | **0** | 未使用 |

- s81 C1/s60_crt の「sin/cos dispatch = 0x9862C0/0x986680」「SSE = 0x996998/0x996BD8」は **方向として正しい**（sin/cos の薄入口と SSE 本体）。ただしそれらは matherr なし `_sin`/`_cos` 副入口（呼出 0 件）とその SSE 先であり、**ゲームが呼ぶ主入口は 0x986300/0x986000**。
- v184 の「0x9862C0∈_sscanf / 0x986680∈_toupper（Ghidra FID）」は **FID 名の境界ずれによる誤帰属**（実体は薄い sin/cos エントリ、各 0x36〜0x46 bytes）。
- **docs 訂正案**: `sin = 入口 0x986300（呼出 69）/ SSE 0x996980→0x99699E / x87 0x986358`、`cos = 入口 0x986000（呼出 98）/ SSE 0x9955A0→0x9955BE / x87 0x986058`。0x9862C0/0x986680 は `_sin`/`_cos`（呼出 0）、0x996998/0x996BD8 はその SSE 先。

### E6-（v184 §5-6）. FUN_00985A00 / FUN_00987C00 の正体 — 解明 [確定]

- **FUN_00985A00 = `floor`（SSE 版、matherr id 0x3ED）** [確定]:
  - 指数抽出（psrlq 0x34）→ 正: exp∈[0x3FF,0x432] で psrlq/psllq（=trunc）をそのまま返す（floor(x)=trunc(x) for x≥0）/ exp<0x3FF で **fldz=0.0**。
  - 負: exp∈[0xBFF,0xC32] で `andpd [0xAA3F40]=1.0 → cmpltpd mask → subsd` = **trunc(x)−1**（floor(-2.5)=-3）/ |x|<1 で `cmpltpd [0xAA3F70]=-0.0 → orpd → andpd [0xAA3F60]=(-1.0, 2^52)` = **-1.0**（floor(-0.5)=-1、floor(-0.0)=-0.0）。
- **FUN_00987C00 = `ceil`（SSE 版、matherr id 0x3EC）** [確定]:
  - 負: trunc(x) をそのまま返す（ceil(x)=trunc(x) for x<0）/ |x|<1 で `fld qword [0xAA3FD0]` = **-0.0**（ceil(-0.5)=-0.0）。
  - 正: `cmpnlepd(x,trunc)` mask + `andpd [0xAA3F90]=1.0` + addsd = trunc+1（ceil(2.5)=3、ceil(2.0)=2）。
- **呼出**: 0x985A00 = 19 件（float ラッパ 0x4842F0 経由 7 + 直接 12）、0x987C00 = 3 件（0x7F2F10/0x7F3254/0x7F3AE3 = CRT 内部）。
- **0x4842F0 = `floorf`** [確定]: `fld dword → fstp qword → call 0x985A00 → fstp dword → fld 原値` の float↔double 変換ラッパ。**s60_crt「0x4842F0 = truncf」は誤り → floorf に訂正**（本調査の計上）。

### E7-（v184 §5-7）. FUN_00993570 の完全検証 — 解明 [確定]

- **0x993570 = asin SSE wrapper**（push ebp/mov ebp/fstp qword/movq xmm0 → `call 0x99358E` → leave/ret @0x993587）。
- **asin SSE 本体 = 0x99358E..~0x993AF0**: 単一 double 入力（movlpd/movsd）、√(1−x²) チェック（0x2000 pinsrw = 1.0x 帯判定）、多項式評価、内部 ret 群 0x9936AD/0x9937D4/0x99384A/0x9939A7/0x993A61/0x993A96/0x993AEF。
- **0x993B00 帯の帰属を解消**: 0x993B02-0x993B95 は **x87 象限補助ルーチン群**（fabs/fld1/fpatan/fldpi/fchs = atan2/asin/acos 共通の quadrant 処理、E8 to 0x993B98 = matherr）、**0x993B98 = __libm_error_support 本体**（[0xbaa648] チェック + [ebp+0x14] のエラーコード switch、0x98BEEB 呼出）。Ghidra が同一関数にマージした別関数群 [確定-構造]。E6 の floor/ceil から call 0x993B98 されるのは NaN 経路のみ。
- → asin SSE 断定は維持。0x993B1B 帯は asin SSE に非ず（matherr/x87 補助）。

### E8-（v184 §5-8）. atan2 の SSE 実装位置 — 解明 [確定]（**存在しない**）

- 入口 0x985980（__ctrandisp2 版）/ 0x98598A（__cintrindisp2 版）は `mov edx, 0xB31C70(name-struct {'\x05atan2', 2, 0x10}) ; jmp dispatcher` のみ [確定]。
- **dispatcher は x87 専用**: __cintrindisp2(0x9908C0)/__ctrandisp2(0x99093B) 内の 0x990BA7/0x990B40 は fxam デコード → `jmp dword [ebx]`（name-struct +0x10 起点の **x87 実装ジャンプテーブル**）。SSE 判定（stmxcsr）はエントリ側（sin/cos/exp 等の prologue）で行われ、atan2 エントリには存在しない。
- **name-struct テーブル 0xB31C80 の全 8 エントリ = x87 補助**: {0x993B0C, 0x993B88, 0x990C7F, 0x993B65×3, 0x990C7F, 0x993B65} — いずれも E7 で同定した fpatan 象限補助/特殊値ハンドラ。
- → **atan2 は x87 fpatan ベースのみで SSE 実装は存在しない**（MSVC CRT の仕様どおり）[確定]。移植時は atan2 は math.h へ畳み込みでよい。

### E9-（v184 §5-9）. CRT 到達不能死コード群 — 解明 [確定]

- **0x985B30 = `_hypot` 薄入口（到達不能）** [確定]: 通常 prologue（[0xbaabdc]/stmxcsr チェック）→ SSE `E9→0x993F39`（hypot blob）、x87 path 0x985BC4（lea edx/call 0x991B45 → x87 本体 0x985BCD 帯）。**E8/E9 呼手は 0 件**（call_targets キャッシュも 0）。
- **hypot/ldexp/modf 本体**: FUN_00993f39 blob 内に matherr id として物理存在 — id **0x18(hypot)** @0x994920/0x99496A/0x9949BC、id **0x19(ldexp)** @0x994930/0x99497A/0x9949B2、id **0x1C(modf)** @0x9945EE/0x99473B（他に 0x1A/0x1B/0x1D）。 blob への外部到達は **E9 0x985B6A→0x993F39（=0x985B30 経由・呼出 0 件）と E9 0x985BA6→0x993F20（pow のみ）の 2 系統のみ**（E8 0x993F32 は blob 内部）。
- .text 内に "ldexp"/"modf"/"hypot"/"_hypot" の cstr は 0 件（name-struct も無し）。
- → **hypot / ldexp / modf は入口なしの死コード**（v184 の hypot 判定を ldexp/modf/0x985B30 へ拡張）[確定]。移植時は再実装不要。

### E10-（v184 §5-10）. Ghidra 関数境界マージ 2 件 — 解明 [確定]

- **(a) FUN_0042f4c0 = 2 関数のマージ** [確定]: 0x42F60F `ret`（SEH クリーンアップ尾）→ **0x42F610 `sub esp,0x448`（独立 prologue、0xB338E8 memset）**。0x42F4C0 先頭は int3 パディング後に `push -1 / push 0x9ABC2E`（SEH prologue）。E8: **0x42F4C0 ← 0x42F95F の 1 件**（0x42F610 内の個別 BSA ロード）、**0x42F610 ← 0x40E75E の 1 件**。台帳 FUN_0042f4c0 は 2 関数を跨ぐため「再帰」に見えるのは境界ずれ。
- **(b) CRT 数学帯の薄入口（未ラベル）**: `mov edx, <id/name-struct> ; jmp` パターンの薄エントリを 0x985800..0x987200 で走査し **6 件**を列挙 — 0x985980/0x98598A（atan2）、0x985C3B（edx=0x1D）、0x9866F7/0x986701（exp 系 name 0xB31D0A）、0x986C18（edx=0x1A）、0x986D68（edx=0x1B）。加えて sin/cos の副入口 0x9862C0/0x986680（E5）も未ラベル。名前台帳利用時は「薄入口は直前関数に吸収される」ことが常態である点を注記。

---

## 2. v185 §4（ゲームプレイ側 10 項目 = R1..R10）

### G1/R1. 0xA41D00 vtable 帯の基底クラス同定 — 解明 [確定-強]（**死 vtable 帯**）

- **帯**: .rdata 0xA41D00..0xA41E38 = **79 スロット**。0xA41E3C から "Functions that aren't in a loaded file..." 文字列（=帯の終端）。前値 0xA41CFC = 0x412AB0。
- **参照ゼロを実証**: **find_dword(0xA41D00) = ファイル全体 0 件**。v185 F3-2 の「表全体へのコード参照は 0x8A5C5E のみ」は **偽陽性**（0x8A5C5E = `E8 1D A4 00 00` = `call 0x8B0080` の rel32 バイト列と偶然一致）。
- **クラス像（slot 値から）**: slot0=0x46E080（dtor、E8 13 件）/ slot9-10=0x46B800/0x46B870（RTTI dyn-cast ヘルパ、G2 参照）/ slot58=0x4AE8C0（Activate 相当 — v185 の同定自体は正しい）/ 他 slot は 0x46xxxxx/0x4AExxxx/0x60xxxxx 帯の TESForm 系関数。**MSVC vtable レイアウト（RTTI dyn-cast が先頭帯）と整合する TESForm 派生クラスの vtable** [推定-強]。
- **判定**: ctor/参照が存在しないため **未使用の死データ帯**。基底クラスの正式同定は不能（持ち主クラス名は残留しない）[確定-構造]。

### G2/R7. 0x497C10（解像度変更コア）内部 — 解明 [確定]（**前提訂正**）

- **0x497C10 は 4 命令の setter のみ**: `mov eax,[esp+4] / mov ecx,[esp+8] / mov [0xB06C5C],eax / mov [0xB06C64],ecx / ret` = **requested width→0xB06C5C / height→0xB06C64 の保存**。D3D 適用は行わない [確定]。
- 0x5DDAA0（VideoDisplayMenu 解像度適用）は `call 0x497C10 → GetMenuByID(0x3FB) → trait 0x1772=2.0 → 0x584740 再表示 → msgbox [0xB38CE0] → 0x5DDCA0`（v185 F4-4 と一致）。
- **実適用 consumer の一例**: 0x497C30 = [0xB06C2C] を gamma 係数として 256 エントリの word テーブルを pow(0x985B70) 曲線で生成 → D3D デバイス（0xB350D8→+0x280）vtable+0x54 呼出（SetGammaRamp 相当）[確定-構造]。デバイス Reset 本体の consumer は §3-(1) へ。

### G3/R8. VideoDisplayMenu command7/8 の値 — 解明 [確定]

- jump table 0x5DDC44 = {0x5DDBDA, 0x5DDBEF, 0x5DDC04, 0x5DDC19, 0x5DDC2E}（command 4..8）。全 5 経路が `push h / push w ; call 0x5DDAA0`。
- **command7（0x5DDC19）= `push 0x25F / push 0x500` = 1280×607** [確定]
- **command8（0x5DDC2E）= `push 0x2D0 / push 0x500` = 1280×720** [確定]
- 整合性: command6 = 0x5DDC04 = 1024×768（push 0x300/0x400）。607 行は 1080i の NTSC 実効行数（TV モード相当）。

### G4/R6. runtime メッセージ form 群の表示本文（静的範囲） — 解明 [確定]（**B13 ダンプ不要で全件取得**）

すべての form は **string GMST エントリ**（登録スタブ 0x9F09xx..0x9F2Cxx 帯の `push default; push name; mov ecx, <bss>; call 0x419A70` から静的に特定）。既定文は EXE 内、ESM の同名 GMST で上書き可能 [確定]。

| グローバル | GMST 名 | 既定本文（EXE 内） |
|---|---|---|
| [0xB37300] | sActivateCreatureCalmed | "The creature is calmed and cannot respond." |
| [0xB38568] | sDropQuestItemWarning | "You cannot remove Quest Items from your Inventory." |
| [0xB38A40] | sCantUnequipGeneric | "You cannot unequip this item." |
| [0xB38AA0] | sNoSleepTrespass | "You cannot sleep while trespassing." |
| [0xB38AA8] | sNoSleepInOwnedBed | "You cannot interact with owned furniture." |
| [0xB38AB0] | sNoSitOnOwnedFurniture | "You cannot sit on owned furniture." |
| [0xB38AB8] | sNoSleepCombat | "You cannot sleep while in combat." |
| [0xB38AC0] | sNoSleepHostilActorsNear | "You cannot sleep when enemies are near by." |
| [0xB38B10] | sGetAllConfiscatedItems | "You get all the items that were confiscated by the Guards." |
| [0xB38B30] | sServeTimeQuestion | "Do you just want to serve your time?" |
| [0xB38B50] | sFastTravelHorseatGate | "Your horse is stabled outside the city gate." |
| [0xB38C00] | sLoadingLOD | "Loading distant LOD..." |
| [0xB38C40] | sInvalidPickpocket | "You cannot place items in a container while pickpocketing." |
| [0xB38C98] | sReturnToMainMenu | "Return to the main menu?" |
| [0xB38CF0] | sOk | "Ok" |
| [0xB38CF8] | sYes | "Yes" |
| [0xB38D00] | sNo | "No" |
| [0xB38CE0] | sVideoChange | "You must exit and restart Oblivion for the new resolution to take effect..." |
| [0xB38DA0] | sOnButtonText | "On" |
| [0xB38DA8] | sOffButtonText | "Off" |

- **v185 F5-1 の用途ラベル差分**（親 docs 訂正材料）: [0xB38AA0] は「戦闘中」ではなく **trespass**、[0xB38AB8] は「敵接近」ではなく **combat**、[0xB38AC0]（jail 分岐）は **HostilActorsNear**、[0xB38B50] は sleep/jail ではなく **fast travel horse**、[0xB38C40] は barter 死体警告ではなく **sInvalidPickpocket**（pickpocket 中の容器操作禁止 — F5-2 の 0x59AC5B 経路とは別文脈の可能性）、[0xB38C00] は PauseMenu 再表示ではなく **sLoadingLOD**。分岐コードの帰属は v185 のまま、GMST 名の対応を本表で置き換えること。
- [0xB38CF0]/[0xB38CF8]/[0xB38D00] = msgbox の **Ok/Yes/No ボタンラベル群**（0x579C10 の第1/第5/第6引数）[確定]。

### G5/R5. `user0` 2.0 の書込コード特定 — 解明 [確定-構造]

- **setter = 0x58CEB0**（`Tile::SetTraitFloat(trait_id, value)`: 0x58B20 でノード取得 → 0x58CA00 で値設定、ret 8）[確定]。関連: 0x58CED0 = SetTraitString/オブジェクト系。
- **push 0xFAE(=user0) は 322 箇所**、うち 2.0 定数（[0xA379B4]）を直近に含むのは **32 箇所**（一覧は v191_b3b_out.txt）。主な決定打:
  - **0x57D518**: メニュー起動/切替系（直前に 0xFA1 へ 2.0 → `fstp [esp]; push 0xFAE; call 0x58CEB0`、ecx=[esi+0x68]=root tile）。
  - **0x596067**: `test byte [ebp+0x88],1 / je → fld 2.0 : fld1` = **bit0 により user0 を 2.0(GameMode)/1.0(MenuMode) へ双方向設定する同期点**（最有力の画面モード状態管理）。
  - 0x59C79D（モード解除経路 0x57DE50 後に 2.0 復帰）、0x5A2836/0x5A2849（fldz→0.0 と 2.0 の交互、メニュー内 tile）等。
- → v185 R5「ゲーム開始/遷移系 [推定]」は「**メニュー起動/モード切替系（SetTraitFloat 呼出し 32 箇所、本体は 0x596067 帯の bit0 同期）**」に更新 [確定-構造]。

### G6/R4. 0x470620 の引数厳密化 — 解明 [確定-構造]

- **本体**: `type = [arg1+4]`（byte）→ `type-0x13 ≤ 0x17` で switch（バイト表 0x470694 + jump 表 0x470674）→ **arg1 + {0x70,0x78,0x68,0x64,0x54,0x60,0x6C} の 7 オフセットのいずれか +4 の float を fld で返す**。該当外/ null は定数 [0xA30634]（0.0f）を返す。
- **引数の正体**: **ExtraData ノード列の先頭（または ExtraDataList 本体）** — [+4]=type byte は 0x41E210（ExtraDataList::GetByType、F5-4）のノード構造と一致。「type 0x13..0x2A の float 値付き ExtraData 群から値スロット（+0x54..+0x78）を type 応じて選択して返す汎用 float getter」[確定-構造]。
- **呼手 8 件**: 0x4698E4 / 0x487EC8 / 0x487FCA / 0x4882FC / 0x594F52 / **0x59AC0F（F5-2 の barter 経路、arg=[esi+8]）** / 0x6101E5 / 0x671399。
- **0x59AC0F の値**: [esi+8] = メニュー行の対象 item entry で、barter 中に取得した float は「価格/交渉額系 ExtraData」[推定-強]（gold 専用 getter ではない — type switch が float ExtraData 群を横断する汎用機構のため）。

### G7/R10. 0x58C230-0x58C33E（&token 解決成功時の最終搬送） — 解明 [確定]

0x58C231 からの完全追跡:
1. 0x58C231: `fldz → [ebx]`（acc を 0 で初期化）、0x58C239: `push -1; push edi; call 0x58B040`（名前解決）。
2. 0x58C247-0x58C265: レジストリ木を **id（[ノード+0x18]）で探索**（一致 je 0x58C2A8）。
3. 失敗時 0x58C267: 0x58B220 → 0x58C273 で `fldz → 0x58CA00`（ノードへ 0.0 をセット）。
4. **解決成功 0x58C2A8**: (a) 命中ノードを **LRU 移動**（[+0x10]/[+0x14] リンク修正 → 末端葉へ再挿入）、(b) **0x58C2FE: `fld [edx+4] → fstp [esi+8]` = 保持値（float）をノードの値スロットへ反映**、(c) 0x58C308: 同値を **[esp+0x1c] へローカル退避**。
5. 0x58C31D-0x58C33E: FPU を 2.0/0.0 で再構成し、後続（0x58C340 以降）で acc [ebx] への最終載せ替えへ。
- → 「解決成功時は LRU 更新 + 保持 float を [ノード+8] とローカル [esp+0x1C] へ複写し、acc へは直後の共通尾で反映」[確定]。

### G8/R2. 0x46B800 系 vtable スロットのクラス同定 — 解明 [確定]

- **0x46B800 = `dynamic_cast<TESForm*>(BaseFormComponent*)` 相当** [確定]: `push 0 / push 0xB02F9C / push 0xB031EC / push 0 / push arg / call 0x9832E6(__RTDynamicCast)` → 成功時 this[+8] と cast 先の flag bit1 を vt+0x90 経由で同期 + type byte / flags word をコピー（`ret 4`）。
- **RTTI 型名**: 0xB02F9C = **`.?AVTESForm@@`**、0xB031EC = **`.?AVBaseFormComponent@@`**（MSVC TypeDescriptor 直読み）。0x46B870 は同型の別経路（slot 10）。
- → 0xA41D00 帯の slot9/10 は **BaseFormComponent→TESForm の dyn-cast スロット**。クラス像は G1 と整合（BaseFormComponent ベース + TESForm 派生）。

### G9/R3. [ebx+0x63]（=a4=barter）設定側の意味論 — 解明 [確定]

- **設定箇所は 1 箇所のみ**: `0x59A238: mov byte [edi+0x63], dl`（dl = [esp+0x24] = 引数 a4）。直前の 0x59A235 で `mov [edi+0x61], bl`（bl=[esp+0x1c]=引数 a3）。0x59A23B で [0xB13E90]=1（選択確定フラグ）。
- 所属: ContainerMenu ハンドラ帯 0x59A1xx（0x59A1F5-0x59A226 の trait 設定連鎖の直後）。つまり **ContainerMenu オブジェクト +0x61=a3 / +0x63=a4 を open 引数から保存する唯一のセットアップ点** [確定]。
- **意味論**: v185 s56 §1-2 の呼出元 4 経路（末尾 `(…, 0, 0, 1)` = barter / `(…, 0, 0, 0)` = loot）に対応し、**a4=1 が barter モード**（F5-2 の 0x59ABEA `cmp [ebx+0x63],0` 分岐で「barter 中の死体/価値 0 取得警告」へ進む）。barter 以外の値（0=loot）[確定-強]。a3(+0x61) は第 3 引数（0 = 通常取得系）。

### G10/R9. [0xB38CE0] / [0xB38C40] の表示本文（静的範囲） — 解明 [確定]

- **[0xB38CE0] = sVideoChange = "You must exit and restart Oblivion for the new resolution to take effect..."**（登録スタブ 0x9F296B）。0x5DDAA0 の解像度変更後 msgbox 本文として完全整合 [確定]。
- **[0xB38C40] = sInvalidPickpocket = "You cannot place items in a container while pickpocketing."**（登録スタブ 0x9F26EB）[確定]。v185 F5-2 は 0x59AC5B で barter 死体警告に使われるとしているが、本文は pickpocket 文脈 — 0x59AC5B 帯の msgbox が本当に [0xB38C40] を第 1 引数としているかは v185 の逆断結果を再引用（本調査では登録文の特定まで）。
- 両者とも B13 ダンプは不要（string GMST の既定文として EXE 内に静的保持）。

---

## 3. 新規残点（本調査で残ったもの・親セッションで起票判断）

1. **解像度リクエスト [0xB06C5C]/[0xB06C64] の実適用 consumer**（D3D Reset/デバイス再作成本体）。0x497C30（gamma ramp 生成）は consumer の一つだが、デバイス リセット本体の関数が未特定。
2. **bAllowConsole (0xB135C0) の実行時利用経路** — 直接読手・getter thunk 呼手とも 0 件。0xB07BF0（Setting manager）経由の間解決、または unused。実機 INI bAllowConsole=0 の効果観測で裏取り可能。
3. **0xA41D00 死 vtable 帯（79 slots）の持ち主クラス** — find_dword 0 件でコード上未使用。slot58=0x4AE8C0(Activate)/slot9-10=dyn-cast から TESForm 派生と推定されるがクラス名は不能。ESM ロード済みオブジェクトの実体でしか同定不能。
4. **v185 F5-1 用途表と GMST 名の差分 6 件**（G4 の表）— 分岐コードの帰属（例: 0x4AEA84 の msgbox が本当に sNoSleepTrespass か）を親 docs で訂正要。
5. **0x470620 の type 対応表（type 0x13..0x2A の ExtraData 型名）** — 7 オフセット（+0x54..+0x78）と type の厳密対応（どの type がどの ExtraData クラスか）は外部資料（型名 RTTI 等）との照合が残る。

## 4. 使用証跡

- **本セッション新規**: `%TEMP%\opencode\v191_util.py`（v185_util 派生: scan_disp32/scan_bytes/find_e8_to/find_e9_to/cc）、`v191_b1.py`（E1-E4）、`v191_b1b.py`（E2 文字列/E3 name hunt）、`v191_b2.py`（E5-E10 CRT math）、`v191_b2b.py`（floor/ceil 定数・atan2 dispatcher）、`v191_b3a.py`（G1/G2/G3/G6/G7）、`v191_b3b.py`（G1b/G4/G5/G9 + RTTI）、`v191_b3c.py`（登録スタブ厳密化/+0x63 setter/bAllowConsole getter/atan2 テーブル）、`v191_verify.py`（37 項目自己検証）。生出力 `v191_{b1,b1b,b2,b2b,b3a,b3b,b3c}_out.txt`。
- **既存資料（読取のみ）**: `tmp/re/v184_engine_small_residuals.md`、`v185_gameplay_residuals56.md`、`v174_gap_residuals2.md`（§3 R-l）、`cache/named_funcs.txt`（CRT 帯 FID 名 + vtable slot 関数名）、`cache/call_targets.json`（呼出数: sin 69 / cos 98 / 0x985A00=19 / 0x987C00=3 / 0x9862C0=0 / 0x986680=0 等）。
- **主な決定打**: bAllowConsole=0xB135C0（name 'bAllowConsole:Interface'@0xA694E8）/ fMenuPlayerLightAmbientRed=0xB135C8 / floor 定数 0xAA3F40=1.0・0xAA3F70=-0.0・0xAA3FD0=-0.0 / command7=0x5DDC19(1280×607)・command8=0x5DDC2E(1280×720) / RTTI '.?AVTESForm@@'@0xB02F9C・'.?AVBaseFormComponent@@'@0xB031EC / +0x63 setter 0x59A238 の 1 件 / atan2 テーブル 0xB31C80 全 x87 / find_dword(0xA41D00)=0 件 / _initterm 0x981C6B（esi=0xA283D0, edi=0xA2F77C, caller 0x9877E1）。

## 5. 自己検証

- **`python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl、2026-10-29 着手時）。
- **`python "%TEMP%\opencode\v191_verify.py"` → 37/37 PASS**（生バイト 15 件: 0x403B00 の 0x0B 書込・sin/cos/SSE jmp 4 経路・_initterm 定数・0x42F60F/610 境界・0x59A238・0x58C2FE・0x497C10 setter・command7/8 push・0x4842FA call 等 / 数値 8 件: [0xB135C0]=1・[0xB135C8]=0.25f・XCU ポインタ・floor/ceil 定数・slot 値 / 台帳照合 6 件: cc(sin=69, cos=98, _sin=0, _cos=0, 0x985A00=19, 0x987C00=3) / 参照走査 4 件: find_dword(0xA41D00)=0・_hypot 呼手 0・E8 各 1 件 / 文字列 4 件: RTTI/`.?AVTESForm@@`・sVideoChange・sInvalidPickpocket・bAllowConsole name）。初回 2 FAIL は検証スクリプト側の期待バイト長記述ミス（実バイナリと照合して修正、分析結論には影響なし）。
- 契約順守: 成果物は新規 1 ファイルのみ、docs/・既存 tmp/re・TODO・台帳・git・Supermemory 無編集、中間物は `%TEMP%\opencode\v191_*`、ESM 再スキャンなし。

## 6. Android 移植への示唆（要点のみ）

- CRT: floor/ceil は math.h へ畳み込み（0x985A00/0x987C00 は再実装不要）、sin/cos/exp は主入口（0x986300/0x986000/0x9866BC）経路を単一化、atan2/hypot/ldexp/modf は実装不要（atan2 は math.h、他 3 つは死コード）。
- UI: user0 は SetTraitFloat(0xFAE, 2.0/1.0) の bit0 同期（0x596067 帯）で画面モードを表現。runtime メッセージ form 群は GMST 経由（ESM で差し替え可能）— 移植側でも既定文テーブル + オーバーライドの構造を踏襲。
- 解像度変更は「setter（要求値保存）→ 事後処理（再表示 + 確認 msgbox）」の 2 段構え（デバイス適用は別段）。

（完 — 実施 20 / 解明 20。新規残点 5 件。TODO/docs 未編集）
