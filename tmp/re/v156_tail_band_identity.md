# V156: tail 帯域（0x980000+）Cat-3 の正体と Ghidra 欠落理由

- 作成: 2026-10-27 / タスク: V156（中）/ 出典: `tmp/re/v_data_relmap.md` F3 ＋ `tmp/re/v127_labelless_cat3.md` 残点 R5（V147）。
- 前提: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B）。セクション: .text VA 0x401000（rsz 6,450,688）/ .rdata 0xA28000（891,392 B）/ .data 0xB02000（raw 199,680 B + bss）/ .tls / .rsrc / .bind。VA=off+0x400C00（.text）/+0x400E00（.rdata）。
- 編集範囲: 既存ファイルは一切読取のみ。成果物は本新規 1 ファイル。新規スクリプトは `%TEMP%\opencode\v156_*.py`。`func_list_new.txt`（共有台帳）・docs・TODO・session-memory は無変更。git 不使用。ESM 再スキャンなし。
- **v127 R5（V147）は本タスクで解消**（帯域の正体同定 + Ghidra 欠落理由の判定）。R6（V148、I1 140 スタブ）も §1-5 で大幅進展。

---

## (0) 手法と口径

| 項目 | 内容 |
|---|---|
| エントリ集合 | `%TEMP%\opencode\v127_cat3_regions.json`（16,660 region）から `code=true ∧ cls ∈ {C:v86-overlap, D:imm32-only, A:extref}` = **10,476 件**（＝v127 公式 Cat-3 10,261 の近似。JSON は v127_cat3_scan.py 系、公式は v127_scan.py 系で **215 件の口径差**）。うち start ≥ 0x980000 = **tail 6,874 件**（v127 帯域表の公式 tail 6,875 と 1 件差）。v_data_relmap F3 記載の「63%（6,475）」は帯域表内訳（399+1,692+4,784=6,875/10,261=67.0%）とも不一致だが、**"tail に 6 割超が集中"** という定性主张は本実測でも維持（6,874/10,476=65.6%）。
| decode | capstone 5.0.7 で全 tail エントリを [start, min(region_end, start+600)) 線形デコード。パターン分類（callstore/pushcall/x87/getter/callret/jmp/other）+ call 先 + .rdata 文字列参照を抽出。
| 参照走査 | 全セクション aligned dword スキャンで tail エントリ値を持つポインタ位置を全列挙（unaligned は v127 JSON の imm32 列で補完）。あらかじめ E8/E9 raw バイト走査（.text 全域）。
| RTTI | vtable-4 = CompleteObjectLocator → pTypeDescriptor(+8) → name(+8) の MSVC x86 鎖でマッチ試行。
| 台帳/ヘッダ | `func_list_new.txt` 21,597 行（CSV: va,name）＋ decompiled.c ヘッダ 1 回走査（`^// (.+?) @ 0x(HEX)$`）21,232 件。Cat-3b = 台帳 − ヘッダ = 365。
| 突合集合 | v90 5 件 / v92 の 0x76・0x77xxxx 171 アドレス（doc 全文 regex）/ v137 の 2 件 / v133 本文なし 8 件。 |

---

## (1) tail 帯域の機能分類結果 — 帯域マップ

tail 6,874 エントリの **99.1%（6,812 件）を 4 系統 + CRT 残片に分類**できました（残 62 件は .text 末尾越境界等）。

### 1-1 帯域構成表

| VA 帯 | サイズ/エントリ | **正体** | 主証拠 |
|---|---|---|---|
| 0x980000–0x9A9D44 | 台帳 819（CRT 名前付き）+ Cat-3 残片 105 | **MSVC 静的 CRT 本体**（locale/ios/float: `__cftoe` `$I10_OUTPUT` `_Locimp` `_Ios_base_dtor` 等）+ 末尾に **import thunk 列**（`VerQueryValueA`/`D3DXSaveTextureToFileA`… 6B `jmp [IAT]`） | 台帳名前（FunctionID）+ thunk 6B 間隔。**0x9A9D44 `D3DXVec4Transform` = `jmp dword ptr [0xA283B8]` → IAT = d3dx9_27.dll** を特定（thunk であり実体は DLL。v127 の「D3DX 直後」は同定済みだったが本体識別はここまで不可） |
| **[0x9A9D50, 0x9D8020)** | 189,136 B / **665** | **MSVC C++ 例外処理 funclet 帯（catch / unwind クリーンアップ）** | 下記 1-2 |
| [0x9E0690, 0x9F1C00) ほか 0x9E/0x9F 帯の小領域群 | 47,344 B/832 + 小領域 819 | **GameSetting 初期化関数帯**（1 設定 = 1 エントリ） | 下記 1-3 |
| **[0xA00190, 0xA10E50)** | 68,592 B / **3,744** | **C++ 静的初期化スタブ帯（.CRT$XCU 系配列参照）** | 下記 1-4 |
| [0xA10EB0, 0xA12380) | 5,328 B / **140** | **.CRT$XCU 本体テーブル（0xA2EFF0, 267 エントリ）の前半 140 個** = NiRTTI/Havok クラス登録イニシャライザ（v127 I1 の 140 スタブ = これ） | 1-4 |
| [0xA12380, 0xA27C39) | 899 | XCU テーブル残り 127（うち 45 は台帳登録済・81 未登録）+ 登録済み実関数（`FUN_00a12a30` 型、SEH+cookie prologue）と交在 + 0xA16700/0xA18AE0/0xA1C550 の `call 0x65C620 + call free(0x401F20)` 系初期化 250 + 0xA232A0–0xA26730 の pushcall 初期化残余 225 | 同上 |
| （残 62） | — | 0xA27C10（.text 実効端末尾の越境 1 件、`pushcall`）等の小ロス | — |

### 1-2 189KB 領域 = MSVC C++ EH funclet（665/665 決定的証拠）

1. **参照は 1 エントリ = 1 本の imm32、参照先はすべて .rdata の EH 構造体**。参照 site の周辺 dword に **`0x19930522`（MSVC TryBlockMap の magic）** が必ず隣接（例: 0x9A9D80 → ref 0xAD6E40: `... 00000001 FFFFFFFF 009A9D80 19930522 00000001 00AD6E3C ...` = CatchHandler{adjectives, catch(...)=FFFFFFFF, handlerAddress} 並び）。
2. **パターン内訳（665）**: catch/unwind funclet `mov ecx,[ebp-0x10]; add ecx,imm; jmp` **469**（うち **466 が共通先 0x980EA4**）/ メンバクリーンアップ（`mov ecx; add; jmp 0x7016A0`・`free(0x401F20)` 呼出等）98 / cookie 付き filter ハンドラ 29（`xor ecx,eax; call 0x9811E2=__security_check_cookie`）/ 配列破棄 push 系（`push dtor; push count; push size` = `__ehvec_dtor` 型クリーンアップ）31 / callstore 21（領域末尾 0x9D7F00–0x9D8020、下記 XCU 配列 0xA28424 由来 = 境界の混入）/ decode 不可 16。
3. **共通 tail-jmp 先 0x980EA4 の正体**: 名前付き台帳 `_UnwindNestedFrames` @0x980E52 — `_CallSETranslator` @0x980F0A の**中間（+0x52）= CRT EH ランタイム内部エントリ**。本体は `push ebp … cld … call 0x98BA0C` で、0x98BA0C は `mov edi,0xE06D7363`（CPP 例外コード）`mov ebx,0x19930522`（EH magic）を比較するディスペッチャ = **`__CxxFrameHandler` 系 CRT 実装（静的リンク）**。周辺には `_JumpToContinuation`/`_CallCatchBlock2`/`__ArrayUnwind`/`___BuildCatchObject`/`CatchIt` 等の CRT EH ランタイムが名前付きで台帳登録済み。
4. 前置き台帳関数は import thunk `D3DXVec4Transform`（6 B + CC pad）— **「D3DX 関数群」説は否定**。この帯域は EXE 全体の try/catch/finally パッド（`/EHsc` コンパイル時の funclet）がリンカ出力として集積したもの。

### 1-3 0x9E/0x9F 帯 = GameSetting 初期化関数

- 47KB 領域の x87 パターン 695/832。典型形: `fld [0xa2fe7c](float定数); push ecx; fstp [esp]; push "fSneakTargetAttackBonus"; mov ecx, <global 0xB3xxxx>; call 0x41BAE0/0x419A70（SEH prologue 付き Setting 構築関数）`。
- 抽出した文字列 = **Oblivion.ini/GMST 名**: `fSneakTargetAttackBonus` `fAIRangedWeaponMult` `fArrowGravityMin` `fBarterSellMult` `fBlinkDelayMin` `fPotionT1CalDurMult` `fAIDefaultDodgeForwardWhileAttackingMult` … ＋文字列設定 `sAttributeText`/`Attribute`/`sSkillText`/`Skill`（0x9F1D70 領域）。
- 参照元 = .rdata の**連続関数ポインタ配列**（0xA2920C, 0xA2A394, 0xA28CEC … = 初期化配列）。登録録済み隣接関数 `FUN_009e62b0` も**同じ配列**に並ぶ（＝配列内で登録/未登録が混在）。

### 1-4 0xA0–0xA2 帯 = C++ 静的初期化スタブ（.CRT$XCU 系）

- **callstore 3,166**: `call f; mov [global], eax; ret`（11 B + int3 pad で 16B 揃え）。書き込み先グローバル 3,193 個・範囲 0xB23D84–0xB42974・**隣接差は delta 4 中心（2,742/3,373）= 配列要素を順に初期化**。呼出し先は少数ヘルパ群: 0x6BB550/0x6BBAF0/0x6BC260…（各 ~163 回、中身 = フラグガード付きテーブルクリア系 `xor eax,eax; cmp byte [flag]; je; ret; mov [0xB3D1xx/0xB3D2xx/0xB3D6xx], eax…`）、0x70E220（`this[0]=a; this[4]=b; ret 8` の 2 フィールド構築、214 回）、0x90D190（3+ フィールド構築、`ConstraintBasisA` 等に 96 回）。
- **pushcall 1,498**: `push 文字列/ポインタ; mov ecx, global; call 構築ヘルパ; ret`。文字列 = **Gamebryo/Havok クラス名**: `NiSkinData` `NiFloatsExtraData` `SkyShader` `TallGrassShaderProperty` `SkyShaderProperty` `ShadowSceneNode` `BSFogProperty` `ShadowLightShader` `BSShaderPPLightingProperty` `bhkPointToPathConstraint` `bhkPoweredHingeConstraint` `hkLinkedCollidable` `hkBroadPhaseHandle` `hkCharacterProxyCinfo` … = **NiRTTI / Havok ファクトリの起動時登録初期化**。
- **参照元 = .rdata の連続ポインタ配列（マージ済 .CRT$XCU）**: 0xA2B17C **len=2,051** / 0xA2D1E0 **len=1,923** / 0xA2EFF0 **len=267**（v127 I1 の表）/ 0xA28424（189KB 末尾 callstore 21 個）/ 0xA2920C（settings）/ 0xA2D73C / 0xA2F420。**計 ~4,300 ポインタ = tail 初期化スタブの全数と対応**。配列の読み手 = CRT 初期化ルーチン（`__CRT_INIT`/`_initterm`、v126 の CRT entry 経路 0x98787C→0x98769C 域）。
- **XCU テーブル 0xA2EFF0（267 ターゲット）の台帳登録状況 = Cat-3 221 / 台帳 45 / その他 1**。下段 140 個（微型スタブ）は 0% 登録、上段 127 個（SEH+cookie 付き大 prologue）でも 45/127 (35%) のみ登録。

### 1-5 Cat-3 tail の参照経路総括（Ghidra が見るべき「入口」）

| 参照種別 | 件数 | Ghidra 可視性 |
|---|---:|---|
| .text 直接 E8/E9 | **17 エントリのみ**（tail 全体、うち 0x990D10 が 46 site・CRT 浮動小数点除算ヘルパ。4 大領域は **0 サイト**） | 可視（が実質皆無） |
| RTTI vtable slot（vtable-4=COL） | **0** | 可視だが該当なし |
| .rdata EH テーブル（magic 付き） | 189KB 帯 665 | **不可視**（Ghidra は funclet テーブルを解釈しない） |
| .rdata .CRT$XCU 配列 | ~4,300 | **不可視**（生の dword 配列） |
| 無参照 | CRT 残片の一部（imm32=0 かつ E8/E9=0: 105 中 32 件等） | 不可視 |

---

## (2) Ghidra が tail 帯域を関数化しない理由 — **判定: 「分析中断/未到達」ではなく「到達経路が存在しない」＝ 参照エッジ不在が主因、int3 pad が二次封鎖**

### 2-1 仮説検証結果（代表 10 領域の capstone 実測）

| # | 領域/点 | 実測 | 判定 |
|---|---|---|---|
| 1 | [0x9A9D50,0x9D8020) 先頭 0x9A9D80 | 前方 10×int3 pad → `mov ecx,[ebp-0x10]; add ecx,0x80; jmp 0x401010(=IAT KERNEL32!DeleteCriticalSection スタブ)`。imm32 参照 1（0xAD6E40、EH magic 隣接）。**外部 E8/E9 = 0** | CFG 入辺ゼロ + EH 表参照 |
| 2 | 同 0x9BF1E0（中央） | 同型 funclet ×2 連続、参照先 0xAE87F4 に magic 0x19930522 | 同 |
| 3 | 同 0x9D7F00（末尾） | `call 0x6BDB00; mov [0xB332A4],eax; ret` + 12×int3。参照 = .rdata 連続配列 0xA28424（XCU 系） | 初期化スタブ混入 |
| 4 | [0x9E6310,0x9F1C00) 先頭 | `fld [0xa2fe7c] … push "fSneakTargetAttackBonus" … call 0x41BAE0`、参照 = 連続配列 0xA2920C、**E8/E9 = 0** | 初期化配列のみ |
| 5 | 同 0x9F1D70 | `push "sAttributeText"/"Attribute"; mov ecx,0xB389E8; call 0x419A70` 同 | 同 |
| 6 | [0xA001F0,0xA0A290) 先頭 | `call 0x6BB550; mov [0xB3C26C],eax; ret` + pad、参照 0xA2B17C 配列、**E8/E9 = 0** | 同 |
| 7 | 同 0xA0A200 | `push 0xB3F684; push "NiSkinData"; mov ecx,0xB3FF2C; call 0x70E220; ret`、参照 0xA2D73C | NiRTTI 登録初期化 |
| 8 | [0xA0A300,0xA10E50) 先頭 | 同型 pushcall（`"NiFloatsExtraData"`）、参照 0xA2D750 | 同 |
| 9 | 0xA10D00（同末尾寄り） | `mov ecx,0xB42A10; call 0x793C60; push …; call 0x981FB4; pop; ret` | XCU 系 |
| 10 | 0x990D10（CRT 帯の隙間、唯一 E8×46 を受ける tail エントリ） | `fld xword [esp+0x10]; … fdivp; ret` = CRT 浮動小数点除算ヘルパ。**前台帳関数の線形デコードが pad を越えて領域に侵入（+31 B）**を実測 = 線形走査では境界越えするが Ghidra は flow 基盤で ret+int3 停止 | int3 pad が二次封鎖の実証 |

**追加証拠（登録済み隣接関数との自然実験）**:
- 台帳登録済みの `FUN_009d8020` / `FUN_009f1c00` / `FUN_00a0a290` / `FUN_00a10e50` / `FUN_00a12a30` / `FUN_00a00130` / `FUN_00a00190` は **すべて decompiled ヘッダあり**（＝Ghidra がデコンパイル時点で関数所有）。しかし **E8/E9 参照も 0 サイト**。かつ XCU テーブル 267 ターゲット中 登録は 45 のみ（登録群は大 prologue、未登録群は 11–15B スタブ）→ **同一参照構造でも登録が不均一**。自動解析の到達性だけでは説明できず、プロジェクト期のスクリプト/手動登録の差異が混入している可能性が高い（→ 残点 R1）。
- 「Ghidra の分析が崩壊・途中終了した」説は **却下**: tail 帯域には台帳 3,802 関数が実在し（0xA1 帯 1,188 / 0xA2 帯 1,148）、CRT 名前付き関数群も正常。欠落は「直接 call/jmp なし + RTTI vtable なし + 参照が生 .rdata 配列のみ + int3 pad 分断」の**構造的必然**。

### 2-2 判定文（要約）

> tail 帯域の Cat-3 は、**リンカ出力の「ファンクション・パッド群」（EH funclet / 静的初期化スタブ / GameSetting 初期化）**であり、実行時は **.CRT$XCU 配列と MSVC EH テーブル（.rdata）から間接起動**される。Ghidra の自動解析は (a) 直接 E8/E9 到達（この帯域では 0）、(b) RTTI vtable（0 件）、(c) IAT thunk の 3 経路しか関数を作らないため、(d) int3 pad が前後の台帳関数からの線形侵入も塞いでいる以上、**この帯域は定義上到達不能**。したがって「Ghidra 欠落」はバグや分析失敗ではなく、**MSVC EH テーブル / CRT 初期化配列を解釈する解析器が（当時も現在も）Ghidra 標準に存在しないこと**の直接帰結である。修復にはテーブル走査型の登録スクリプト（残点 R2/R3）が唯一の経路。

---

## (3) 住所集合突合（v92 / v90 / v137 / v133）

| 集合 | 件数 | tail 帯域交差 | 突合結果 |
|---|---:|---:|---|
| **v92** renderer/state gap 系（doc 中 0x76/0x77xxxx 全アドレス） | 171 | **0** | 内訳: 台帳 45 / **Cat-3(mid) 71** / JSON F:noncode 17 / JSON 未収録 38。**renderer gap クラスタ 0x769F00–0x769F80 は JSON F:noncode（2 命令で data 判定）**、v127 公式 Cat-3 にも非採用。→ **F3 の核心「renderer gap × tail Cat-3」は住所集合的に交差なし = 帯域は別系統**と確定。Cat-3(mid) 71 件に state vtable ターゲット（0x77AFE0=+0x4C、0x77B280=+0xE0 等）が含まれ、**v92 の gap 関数群は Cat-3(mid) に大量収録されている**（＝F3 の突合自体は成立、ただ tail ではなかった） |
| **v90** ラベルなし 5 件 | 5 | 1 | 0x4052F0/0x40DF80/0x497B50 = Cat-3(mid) 収録。0x40D800 = JSON **E:orphan**（公式 v127_scan は Cat-3 採用＝口径差）。**0x98769C は JSON 未収録**（公式は採用。CRT entry 経路 0x98787C→0x98769C は tail 帯域だが本 JSON 口径では不可視＝残点 R5）。0x98787C も JSON 未収録（v127 R4 と同じ既知 limitation） |
| **v137** 欠落関数 FUN_00510150 / FUN_005101E0 | 2 | **0** | ともに **Cat-3b（台帳あり・decompiled ヘッダなし、mid-band 0x51xxxx）**。v127 R2（V144）の対象であり tail と無関係 |
| **v133** 本文なし 8 件（0x815DB0–0x884D50） | 8 | **0** | すべて Cat-3b（ledger(hdrless)）。tail 交差 0 |
| Cat-3b 全体（台帳 21,597 − ヘッダ 21,232） | 365 | **0** | **Cat-3b は tail 帯域に一件も存在しない**（tail の欠落はすべて「台帳にも無い」Cat-3a であることが確認された） |

**RTTI 名（vtable-4=COL）による帯域識別**: tail 6,874 エントリに対する COL→TypeDescriptor マッチは **0 件**。「.rdata の RTTI typeinfo を帯域参照でマッチする」手法は tail 帯域では**原理的に不成立**（非 RTTI 帯域）。帯域識別の正攻法は **参照テーブルの種別判定**（EH magic 0x1993052x 隣接 = funclet / .CRT$XCU 連続配列 = 初期化スタブ / 連続配列 + f*/s* 文字列 = GameSetting）。

---

## (4) 成果物

| 種別 | パス |
|---|---|
| **本成果物** | `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v156_tail_band_identity.md` |
| スクリプト（すべて新規・%TEMP%\opencode） | `v156_tail_scan.py`（全体像: decode/パターン/参照/RTTI/E8E9/突合/スポットチェック）/ `v156_probe2.py`（参照テーブル特定・callee 同定・v92 口径突合）/ `v156_probe3.py`（IAT デコード・189KB 形状細分）/ `v156_probe4.py`（XCU 267 分類・ヘッダ存在・0x980EA4 深掘り）/ `v156_summary.py`（帯域集計）/ `v156_names.py`（台帳名前分布） |
| 中間出力 | `%TEMP%\opencode\v156_tail_out.txt` / `v156_probe2_out.txt` / `v156_probe3_out.txt` / `v156_probe4_out.txt` / `v156_tail_entries.json`（6,874 エントリの分類表） |
| 読取のみの参照 | `tmp/re/v127_labelless_cat3.md`、`v_data_relmap.md`、`v92_shader_renderer_bundle.md`、`v90_b350d8_triangle.md`、`v137_v80_global_residuals.md`、`v133_full_dup_scan.md`、`v126_bind_crt_bootstrap.md`、`%TEMP%\opencode\v127_cat3_regions.json`、`v127_cat3_out.txt`、`v127_scan_out.txt`、`func_list_new.txt`、`probe_harness.py`、`docs/Oblivion_decompiled.c`（ヘッダ走査 1 回） |

再現: `python "%TEMP%\opencode\v156_tail_scan.py"` → `v156_probe2.py` → `v156_probe3.py` → `v156_probe4.py` → `v156_summary.py`（前提: `probe_harness.py selftest` 4/4 PASS）。

---

## (5) 新規残点（V143+ 起票粒度）

| # | 提案 V-id タイトル | 優先 | 出典と内容 |
|---|---|---|---|
| R1 | **V157: .CRT$XCU 267 ターゲットの登録不均一（台帳 45 / 未登録 221）の差分要因裁定** | 中 | 同一配列参照・同一 E8/E9 ゼロでも登録群は大 prologue 関数のみ。Ghidra 自動解析のどの機構（またはプロジェクト期スクリプト）が 45 件を作ったかの裁定。func_list 作成履歴の調査を含む。 |
| R2 | **V158: 189KB EH funclet 665 件の一括台帳化（TryBlockMap 走査スクリプト）** | 高 | .rdata の EH テーブル（magic 0x19930522 隣接の handlerAddress 列）を走査し funclet 先頭を機械登録。catch（this 調整→member クリーンアップ）/ unwind（0x980EA4=_UnwindNestedFrames 内部エントリ tail-jmp）の機械分類を併せて実施。Cat-3 tail 最大ブロックの解消。 |
| R3 | **V159: GameSetting 初期化帯 ~1,650 件の台帳化と f*/s* 名対応表生成** | 中 | 0x9E/0x9F 帯の全エントリから（初期化配列順 = テーブル index）で「Setting 名文字列 → 初期化関数 → グローバル」対応表を機械生成。`v91_ini_defaults.md` / `s51_inisettings_spec.md` への合流候補。 |
| R4 | **V160: callstore/pushcall 初期化スタブ 3,744+ の global→初期化内容マップ** | 中 | 呼出し先ヘルパ群（0x6BB550 系テーブルクリア、0x70E220/0x90D190 構築、0x65C620+free 系）の同定と、B23D84–B42974 のグローバル配列（B42EC0 スロット表等の既知グローバルと連続）の意味づけ。V148（I1 140 スタブ）の「テーブルの読み手」は CRT `_initterm` 経路（v126）で確定済 → 本残点は「何を初期化しているか」の確定。 |
| R5 | **V161: v127 両スクリプトの口径統一（scan vs cat3_scan）** | 低 | 0x98769C/0x98787C が cat3_scan JSON で未収録、0x40D800 は E:orphan、F:noncode 4,860 の扱い等の不一致。tail 件数 6,874 vs 6,875、Cat-3 全体 10,476 vs 10,261 の差分原因の特定と公式口径の確定。 |
| R6 | **V162: CRT EH ランタイム断片 105 件（0x98–0x9A 帯）と 0x980EA4 内部エントリの整理** | 低 | 0x980EA4（_UnwindNestedFrames+0x52）等の内部エントリを台帳反映するかの裁定。0x990D10（E8×46 の浮動小数点除算ヘルパ）等の実用関数のラベル化。 |
| R7 | **V163: .text 末尾越境界 0xA27C10 の扱い裁定** | 低 | .text 実効端（0xA27C39）手前の 1 エントリが領域 [0xA27C10,0xBAF000) として出現（.bind オーバーレイ接続部）。ゴミ/リンク残滓の判定。 |

> v127 残点との対応: **R5（V147、本タスク）= 解消**。R6（V148、I1 140 スタブ）= 「テーブル読み手」確定により残点を R4 に集約。R4（V146、pad=0 エントリ）は本タスクの分類に包含（CRT entry 経路）。R2（V144、Cat-3b 365 件）= **tail と無関係**であることを確定（mid-band 専用課題として再定義可）。

---
生成物: `tmp/re/v156_tail_band_identity.md`（新規 1 ファイル） / 生成スクリプト: `%TEMP%\opencode\v156_tail_scan.py`, `v156_probe2.py`, `v156_probe3.py`, `v156_probe4.py`, `v156_summary.py`, `v156_names.py` / 前提: `probe_harness.py selftest` 4/4 PASS

---

## (6) V146 追記: gap50/54 × v90 × v137 × RTTI 1,529 の独立住所集合突合（2026-10-27）

- 担当: V146（serial 146）。既存 §0〜§5 は読取のみ、本節を追記（上書き破棄なし）。
- 手法・入力: `v92_shader_renderer_bundle.md` の 98 スロット表（L130–233）・付録 A 67 スロット表（L354–428）から gap ターゲットを機械抽出 → `%TEMP%\opencode\v127_cat3_regions.json`（Cat-3 口径 10,476）・`tmp/re/cache/named_funcs.txt`（21,597）・`docs/Oblivion_decompiled.c` ヘッダ（21,232）・`tmp/re/cache/call_targets.json`（E8）+ capstone skipdata 全域走査（E9）・`tmp/re/cache/vtable_index.tsv`（1,715 vtable）・.rdata/.data aligned dword 全走査 で突合。ESM 再スキャンなし・docs/TODO/台帳/git/Supermemory 編集なし。
- 中間スクリプト（すべて新規・%TEMP%\opencode）: `v146_inspect.py` / `v146_inspect2.py` / `v146_gap_rtti_cross.py`（突合本体）/ `v146_probe2.py`（巨大帯 Debug・E8/E9・gap 再突合）/ `v146_probe3.py`（tail vtable ターゲット同定・帯域別登録率）/ `v146_probe4.py`（0x982D85 深掘り・参照センサス裁定）/ `v146_probe5.py`（RTTI 名清点・__purecall 確定・帯域別表）/ `v146_probe6.py`（tail 台帳/ヘッダ清点・R6025 文字列）。出力: `v146_out.txt`, `v146_probe2_out.txt`〜`v146_probe6_out.txt`。

### 6-1 住所集合突合（厳密抽出版 — TODO 146 の指定4集合）

| 集合 | 抽出 | 件数 | tail(0x980000+)交差 | 突合結果（本実測） |
|---|---|---|---:|---|
| **v92 renderer gap 50** | 98 スロット表の gap 列 | 50 slot / **48 distinct**（0x76B310 が #86/#87 で重複）range 0x4348A0–0x76BE00 | **0** | 台帳 0 / ヘッダ 0 / **E8=0・E9=0（48/48）** / Cat-3 収録 45（非該当 3 = 0x6FDFF0・0x700640・0x76B310） |
| **v92 state gap 54** | 付録 A 67 スロット表 | **54 distinct** range 0x629290–0x780840 | **0** | 台帳 0 / ヘッダ 0 / E8=0・E9=0（54/54）/ Cat-3 53（非該当 1 = 0x77B7A0） |
| 合計 union | — | **102 distinct** | **0** | Cat-3 **98/102**（C:v86-overlap 90 + D:imm32-only 8）。非該当 4 件はいずれも**次台帳開始点の 16B 手前 = 前関数 pad 内**（prev = FUN_006fdf70 / FUN_00700610 / FUN_0076b160 / FUN_0077b750、v127 の x-fun-header 除外と整合） |
| **v90 ラベルなし 5 件** | — | 5 | **1** | 0x4052F0・0x40DF80・0x497B50・0x40D800 = mid Cat-3（cls C、E8 各 1）。**0x98769C = tail だが cat3_regions.json 未収録（contains=NONE）**、E9=1（site @0x987881 = CRT entry jmp）で公式 v127_scan は CAT3 採用 → **v156 §3/残点 R5 の口径差を維持・再確認**。0x40D800 は入口レコード E:orphan（v156 §3）と包含領域 C:[0x40D4D0,0x40F640) が併存 |
| **v137 欠落 2 件** | FUN_00510150 / FUN_005101E0 | 2 | **0** | 台帳あり・ヘッダなし = **Cat-3b**、E8=0・E9=0・mid 帯域 → v127 R2/V144 系で tail 無関係（v156 §3 と一致） |

→ **F3 の核心「renderer gap × tail Cat-3 は住所単位で未突合」は本抽出で解消: gap 全 102 件と tail の交差は 0。両者は別系統（gap=mid の data-only vtable 群 / tail=EH・初期化配列群）と確定。**

### 6-2 RTTI 名 1,529 件による帯域識別（TODO 146 の指定手法）

**RTTI 名の清点（全文 raw 走査）**: `.?AV` = **1,529** / `.?AU` = **18** / 計 **1,547**（= GT「TypeDescriptor 1,547 件」と完全一致）。全件 `.data`、VA 範囲 **0xB02008–0xB329F0**（先頭は file off 0x700C08 = .data raw+8）。
- **v137 §2-4/§5-2 の「.?AV は 0xB01A08〜」は .rdata const 誤適用**: 0x700C08 + 0x400E00（.rdata const）= 0xB01A08 を再現。**正 = 0xB02008（.data const 0x401400）**（→ §6-5 訂正 1）。

**vtable 全走査（1,715 本 / 29,063 スロット）**:

| 検査 | 結果 |
|---|---|
| COL 一致性（vtable−4 == col 列） | **1,715/1,715 不一致 0** |
| .text 内 distinct ターゲット | 7,542 |
| ターゲット帯域分布 | <0x600000: 8,287 slot / 0x60–0x80M: 17,215 / 0x80–0x98M: 2,833 / **≥0x980000（tail）: 728 slot** |
| **tail ターゲットの所在地** | **728 slot / 69 distinct / 243 vtable — 全て [0x980000, 0x9A9D44)。0x9A9D44 以降（189KB EH 帯・GameSetting 帯・初期化スタブ帯・XCU 帯）の vtable 参照は 0 件** |
| 69 distinct の内訳 | 台帳ラベル済 **33**（全件ヘッダあり）/ **Cat-3 エントリ先頭 28** / 領域内中間・孤 8 |

**帯域識別で決まったこと（t1 小帯域の正体細分）**:
1. **0x982D85 = CRT `__purecall` [確定]**。228 vtable / **625 slot** が参照（NiRenderer 単独で 66/95 スロット = 抽象基底の pure 槽、NiD3DRenderState・NiInterpController・NiShaderFactory・NiTListBase 系テンプレート等も同様）。本文: `push [0xBAA5EC]; call __decode_pointer; …; call eax; push 0x19; call __NMSG_WRITE; …; jmp _abort`。呼出し先 0x98BEEB=`__decode_pointer` / 0x98D5B8=`__NMSG_WRITE` / 0x98BC6B=`_abort`（台帳名照合）。**文字列 `R6025 - pure virtual function call` を off 0x6A3EF3 で発見（0x19=25=R6025）** = pure virtual 経路の完全一致。Ghidra 未ラベル（Cat-3 先頭・E8/E9=0・vtable 参照のみ）の代表例。
2. **0x983D61（6 slot: std::bad_alloc / bad_cast / bad_exception / __non_rtti_object 等の slot+0x04）** = 共有 `what()` 系実装 [推定・高]。**0x9809EE** = `std::locale::_Locimp` scalar deleting dtor（台帳名）、**0x981E65** = `type_info` deleting dtor、**0x982897** = `type_info` slot+4（いずれも tail・Cat-3 か台帳名付き）。
3. **0x9A1B90–0x9A9AD0 群 = Gamebryo レンダラ系の実装メソッド [確定・RTTI 一次証]**: `.?AVNiD3DShaderConstantMap@@`（vt 0xAB2A4C, col/td 一致）の slot+0x18〜+0x4C → 0x9A8660/0x9A8800/0x9A8890/0x9A8C40…、`.?AVNiRenderTargetGroup@@`（vt 0xAB286C）→ 0x9A2110/0x9A1F10/0x9A1F20…、NiD3DSCM_Pixel / NiD3DSCM_Vertex / NiRenderedCubeMap / NiSCMExtraData / NiTArray も同所を参照。未ラベル分（Cat-3 先頭 28 件）の本文は `mov eax,0xBAA8xx; ret`（GetType スタブ）と getter/setter 群。
   → **v156 §1-1 の「0x980000–0x9A9D44 = MSVC 静的 CRT 本体」は 0x9A1B00 以降が NiD3D 系実装で混在している（部分訂正）**。0x980000–0x9A1B00 側は std::locale/type_info/exception/__purecall クラスタで CRT 説が正しい。
4. **t2–t5（EH 帯・GameSetting 帯・初期化スタブ帯・XCU 帯）は RTTI vtable を一切持たない** → 「RTTI 名 1,529 件で帯域識別」の結論は **RTTI で識別できるのは t1 のみ。t2–t5 は参照テーブル種別（EH magic / .CRT$XCU / f* 配列）でしか識別できない**（v156 §3 の方向付けを裏付け・定量）。

### 6-3 tail 参照構造センサス（Cat-3 6,874 エントリ、aligned dword 全走査）

| 参照種別 | 件数 | 備考 |
|---|---:|---|
| .rdata/.data aligned dword 参照あり | **6,142 / 6,874（89.4%）** | 内訳 by band: t1 63 / t2 EH 636 / t3 1,321 / t4 初期化スタブ **3,744** / t5 378 |
| うち **vtable スロット経由** | **28 エントリ**（728 slot / 69 ターゲット） | vtable_index スロット位置と 28/28 完全一致 |
| うち配列型データ（XCU / EH / setting 配列） | 6,114 | data 参照の **99.5%** |
| 直接 E8/E9 コード参照 | **17 エントリ** | v156 §1-5 の「17 エントリ」と**完全一致**（内部整合） |
| 参照ゼロ | 715 | — |

XCU 配列 `0xA2EFF0` の連続 .text ポインタは **実測 483**（終端は 484 番目 = 0）。先頭 267 中の台帳登録は **45**（v156 §1-4 の 45 と一致）だが、v127 I1/v156 の「len=267」は前半区分にあたる（全体では 65/483 = 13.5% 台帳）→ 口径差として記録。

### 6-4 Ghidra 欠落理由の再判定（v156 §2 の補強と限定的拡張）

**帯域別・data-only vtable ターゲットの台帳登録率**（=「vtable 参照はあるのに直接コード辺が無い」ターゲットの Ghidra 到達率）:

| 帯域 | data-only ターゲット | 台帳登録 | 率 |
|---|---:|---:|---:|
| <0x600000 | 2,149 | 1,011 | 47.0% |
| 0x60–0x80M | 3,527 | 1,547 | 43.9% |
| 0x80–0x98M | 889 | 520 | 58.5% |
| **t1 0x980000–0x9A9D43** | **68** | **33** | **48.5%** |
| t2–t5 | **0（vtable 参照自体が無い）** | — | — |

- **tail 台帳 = 3,802（全件ヘッダあり）** = v156 §2-1 と一致。内訳 t1 818 / t2 1 / t3 645 / t4 2 / t5 2,336。
- **判定（v156 §2-2 を本実測で裏付け・一般化）**: 欠落の機械的理由は 3 層。(1) **直接到達エッジ不在**: tail Cat-3 の E8/E9 は 17/6,874。(2) **残る参照の 99.5%（6,114/6,142）が配列型データ**（.CRT$XCU / MSVC EH TryBlockMap / GameSetting 初期化配列）で、**「配列型データ参照 → 関数生成」をする解析器は Ghidra 標準に存在しない**（vtable 型ポインタ列に限れば既存帯域と同率 44–58% が登録済み＝t1 も同水準）。(3) **int3 pad が隣接台帳関数からの線形侵入を遮断**（v156 の 0x990D10 +31B 侵入例と整合）。
- **却下した仮説**: 「シグネチャ判定・リーフ判定・アライメント・命名規則のバグ」説は**主因ではない**。証拠 = gap50/54 の 102/102 が E8/E9=0 であり（= 同現象が mid 帯域でも発生）、かつ t1 の data-only 登録率（48.5%）が他帯域（43.9–58.5%）と同水準だから、**欠落は「帯域（アドレス位置）」ではなく「参照の形（直接辺 vs 配列データ）」に起因する**。
- ただし **44–58% 登録率を生んだ機構**（Ghidra の pointer/data-ref 既定動作かプロジェクト期スクリプトか）は未裁定 = v156 残点 R1 と同一問題。

### 6-5 本追記で確定した訂正・差し替え

| # | 対象 | 旧記載 | 新記載（本実測） |
|---|---|---|---|
| 1 | v137 §2-4 / §5-2 | `.?AV` 先頭 = 0xB01A08（.rdata 末尾） | **0xB02008（.data const 0x401400、.rdata const 誤適用を再現して証明）**。件数 1,529 は正しい |
| 2 | v156 §1-5「RTTI vtable slot = 0」 | tail エントリの vtable 参照は 0 | **Cat-3 先頭 28 エントリが vtable スロット参照（728 slot / 69 distinct / 243 vtable）**。0 は誤り |
| 3 | v156 §1-1 t1 ラベル | 0x980000–0x9A9D44 = MSVC CRT 本体 | 0x9A1B00 以降は **Gamebryo NiD3D/レンダラ系実装**（RTTI 一次証）で混在。0x980000–0x9A1B00 は CRT 説で正确 |
| 4 | v127 I1 / v156 §1-4 | .CRT$XCU 0xA2EFF0 = 267 エントリ | 連続 .text ポインタ実測 **483**（267 は前半区分。先頭 267 の台帳 45 は一致） |
| 5 | v_data_relmap F3 | gap × tail は「一度も突合されていない」 | gap 全 102 件を厳密抽出して**交差 0 を確定**（Cat-3 98/102、残 4 は前関数 pad 内） |

### 6-6 結論（3行）

1. tail 帯域の正体は v156 §2-2 の結論（EH funclet / CRT 静的初期化スタブ / GameSetting 初期化 = 間接テーブル起動物）を維持し、本追記は t1 小帯域を「std/CRT 异常・type_info・`__purecall` クラスタ + Gamebryo NiD3D 実装群」に細分、**t2–t5 が RTTI vtable を一切持たないこと**を 1,715 vtable 全走査で確定した。
2. Ghidra 欠落の機械的理由 = **①直接 E8/E9 17/6,874、②残り参照の 99.5% が配列型データでそれを関数化するアナライザが標準に不在、③int3 pad の線形遮断**。シグネチャ/リーフ/アライメント/命名規則説は帯域横断の登録率実測（44–58%）で**却下**。
3. gap50/54（mid 102 件）・v90 4 件・v137 2 件は tail と住所交差ゼロ（tail 交差は v90 の 0x98769C のみ・口径差 R5）、**F3 の「renderer gap × tail Cat-3」統合仮説は不成立**。

### 6-7 残留不明点

- XCU 483 のうち index 268–482 の所属区分（.CRT$XCU か連結する他サブセクションか）未特定 = v127 口径 267 との差分裁定。
- `__purecall` の handler 格納先 0xBAA5EC の正式名（_purecall_handler の符号化変数）は未照合（NMSG チェーン + R6025 でほぼ確定）。
- data-only ターゲットの 44–58% 登録機構（Ghidra 既定 vs プロジェクト期スクリプト）未裁定 = v156 R1。
- 0x40D800 の入口 E:orphan / 包含 C の口径差、gap 非該当 4 件の pad 内判定 = v127 x-fun-header 口径（R5 に集約）。

### 6-8 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- 内部整合（一致した項目のみ列挙、13 件）: ①ヘッダ 21,232 = v127/GT ②台帳 21,597 = GT ③Cat-3 JSON 10,476・tail 6,874 = v156 §0 ④巨大帯 json_starts vs 公式 entries: 832/832・2,294/2,294・1,450/1,450・140/140（664/668 は 4 件差のみ） ⑤capstone E8 137,421 sites = cache 137,421、`call 0x446CB0`=347・`0x4FB5F0`=22 = GT ⑥v92 表 renderer 98 行/gap 50・state 67 行/gap 54 = v92 記載と一致 ⑦vtable 1,715 / slots 29,063 = GT、COL 不一致 0 ⑧`.?AV` 1,529 + `.?AU` 18 = 1,547 = GT TypeDescriptor 1,547 ⑨tail E8/E9 = 17 = v156 §1-5 の 17 ⑩tail 台帳 3,802 = v156 §2-1 ⑪vtable-slot 参照 28 ⊂ data 参照（28/28 一致） ⑫XCU 先頭 267 の台帳 45 = v156 §1-4 ⑬data-only 帯域別合計 6,633 = 全体センサスの code-edge=0 6,633。
- 禁止事項の遵守: ESM 再スキャンなし、docs/TODO/台帳/git/Supermemory/sync 編集なし、既存 `tmp/re/*.md` は本節追記以外の読み取りのみ。

### 6-9 Android 移植への示唆

移植時は tail 帯域を「リンカ生成の間接起動物」としてコード化せず、**(a) .CRT$XCU 相当の静的初期化配列を起動時に順に呼ぶラムダ群**（NiRTTI/Havok 登録・GameSetting 設定・グローバル構築）、**(b) MSVC EH funclet は try/catch 言語構造へ直列化**、**(c) `__purecall` 相当は未実装仮想関数の到達時に abort** と再表現するのが素直である。gap50/54 と t1 の未ラベル 28 件は実体を持つ実関数（getter・GetType スタブ・共有 what）なので、vtable スロット表（vtable_index.tsv）を移植側のインターフェース定義の一次資料として使う価値が高い。RTTI 名簿（.data 0xB02008–0xB329F0、1,547 件）はクラス階層の検証に使えるが、v137 の 0xB01A08 は使わず 0xB02008 から数えること。

---
追記生成物: 本節（§6）/ 追記スクリプト: `%TEMP%\opencode\v146_inspect.py`, `v146_inspect2.py`, `v146_gap_rtti_cross.py`, `v146_probe2.py`〜`v146_probe6.py` / 中間出力: `v146_out.txt`, `v146_probe2_out.txt`〜`v146_probe6_out.txt` / 前提: `probe_harness.py selftest` 4/4 PASS（V146 実行時にも再確認）
