# V149: renderer/シェーダ/WinMain 残点束の裁定（renderer+0x280・jcc 極性・レンダラ slot0 等）

- 作成: 2026-10-27 / 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / SHA256 `A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`）
- 出典（一次資料）: `tmp/re/v129_shader_slot_residuals.md` §11-1 残点（V143×4 + V144×4 + 小1 = 9 行）＋ `tmp/re/v128_winmain_tail.md` §8 残点 1–7（#7 = 0x982D71 同定） ＋ `tmp/re/TODO_re49.md` serial 149（成果物パス指定: 本ファイル名 `v159_renderer_residuals.md`。タスク ID とファイル名は非依存）
- 内数の注記: TODO 記載は「v129 残点 V143/V144(7) ＋ v128 残点 1–6(6) = 13 件」だが、v129 §11-1 の実リストは 9 行（V144×4 + 小1）、v128 §8 の実リストは **7 項目**（#7 = 0x982D71 同定。タスク残点10 は 0x404940 戻り値と 0x982D71 の 2 項目を括ぶ）。**差 3 行を含む計 16 件（v129 9 ＋ v128 7）をすべて裁定する**（差は TODO 記載の内数誤差とみなす）。
- ハーネス: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（作成期 着手指定時・完走時 ＋ 本継続 再開時・完走確認時の計 4 回、全回 PASS）
- 中間スクリプト: `%TEMP%\opencode\v149_01_winmain / v149_02_refs / v149_03_types / v149_04_scan / v149_05_scan2 / v149_06_dec / v149_07_ctx / v149_08_dec2 / v149_09_scan3 / v149_10_scan4 / v149_11_dec3 / v149_12_flags / v149_13_final / v149_14_ctx / v149_15_verify` ＋ 追補 `v149_10_final / v149_16_access / v149_17_d8c` ＋ 再構成 `v149_18_restructure / v149_19_diag`（計 20 本、出力は各実行ログ）
- 成果物は **本ノート 1 ファイルのみ**。docs/・既存 tmp/re・TODO・台帳・git・Supermemory・sync は編集禁止（読取のみ）

判定表記: **[確定]** = バイナリ実測で一意 / **[確定(機序)]** / **[確定(論証)]** = 実測 + 排除論証で一意 / **[観測]** = 実測事実 / **[仮説]** = 推定 / **[未特定]** = 裁定後も残る。

---

## §1 手法 / 入力

- ハーネス: probe_harness `Ctx.rd/cstr/enclosing_fun`（.text const=0x400C00 / .rdata=0x400E00 / .data=0x401400、bss は None+警告）+ capstone 5.0.7（skipdata / md.detail、**線形デコード 2,258,215 命令**を operand 照合で全数走査）。
- 入力: バイナリ本体、`docs/Oblivion_decompiled.c`（部分検索）、共有キャッシュ `tmp/re/cache/{call_targets,vtable_index,opcode_table,named_funcs}.*`（再走査せず引用）、既存成果物 v128/v129（記載の引用・再測定）。
- 手法方針（残点別）:
  1. **renderer[+0x280]**: disp32 `0x280` 書式命令の全数走査 → 書き手は「レジスタ経由の明示ストア」ではなく **out-parameter** と判明 → FUN_0076a1c0 の CreateDevice 呼出しを全文デコードし引数並びを照合。
  2. **vtable[+0x54] getter caller**: `call [reg+0x54]`（disp8/disp32）・load→call 形（333 site 全数）を抽出し、**呼び出し直後のビットテスト値（2/4/8）とレンダラグローバル経由**で絞り込み。
  3. **0x4FDE30 内 impl transfer**: trampoline 範囲（0x4FDE30..0x4FF900）の `[reg+0x18]` / `jmp [reg]` / `call [reg]` 全走査（0 件）→ オペコード表の列アドレス（0xB0AF48/0xB0AF50/0xB0AF5C/**0xB0AF60=impl 列**）を全体走査して**実 consumer を特定**。
  4. **[0xB34D90] ロガー**: 絶対参照の全数走査（書/読）→ 書き手 ctor をデコード → vtable 0xA3DA08 を RTTI 索引 `cache/vtable_index.tsv` で照合 → 登録 stub と初期化配列を imm32 走査。
  5. **40E22E/40E2D2 の jcc 極性**: WinMain コピー保護帯を全文デコード、境界 7 バイト帯を raw 復元、`0x404940` の戻り値規約をボディ実測、4 極性組合せの意味論整合を排他的に検証。
  6. **レンダラ slot0 途中経路**: vtable slot0 呼出定型（`mov edx,[reg]; mov eax,[edx]; call eax`）をレンダラ保持グローバル（0xB3F928 / 0xB350D8 / 0xB43104 / 0xB43110 / 0xB42EB8）の絶対ロードから前方走査して**全数列挙**、各 site の登録経路（CRT 初期化配列・atexit・E8）を辿る。
  7. **0xB42F31 clear 時期**: FUN_007B7250 の唯一 caller → CRT 初期化 stub → 初期化配列（.rdata 0xA2EFF0 帯）を imm32 で特定。
  8. 残る v129 V144/小・v128 #2/#5/#6 は、imm32 全セクション走査 + 該当箇所デコード + RTTI 索引照合で裁定。

---

## §2 裁定表（16 件）

| # | 残点（出典） | 裁定 | 根拠（最短） |
|---|---|---|---|
| 1 | renderer[+0x280] クラス名厳密同定（v129 V143-a） | **IDirect3DDevice9\*（生 COM）[確定]** | FUN_0076a1c0 `push &renderer[+0x280]` → `IDirect3D9::CreateDevice`（vtable +0x40=idx16）@0x76A545 |
| 2 | vtable[+0x54] getter caller（v129 V143-b） | **10 site 特定 [確定(観測)]** | load→call 333 site 中、直後に +0x5E0 の実測値 {2,4,8} のテストが続く 9 site + shr 形 1 site |
| 3 | 0x4FDE30 内 impl transfer（v129 V143-c） | **前提訂正 + 正位置特定 [確定]** | 0x4FDE30 はコンパイラ dispatcher（+0x1C）。impl(+0x18) は **FUN_005169a0 @0x516A5D 読取 → 0x516AF3 `call ecx`** |
| 4 | [0xB34D90] ロガー クラス・格納元（v129 V143-d） | **MessageHandler（static 0xB33F80）[確定]** | vtable 0xA3DA08 = `.?AVMessageHandler@@`、ctor 0x494650 は CRT 初期化 stub 0x9DC330 から |
| 5 | 型レコード 0xB42858 の name（v129 V144-a） | **"NiD3DShaderInterface"（親=NiShader）[確定]** | 登録 stub 0xA10BA0 `push 0xB40124; push 0xA8A84C` |
| 6 | FUN_00553bb0 の 0xE の名称（v129 V144-b） | **SkinShader（case14）[確定]** | FUN_007B7FC0 が `call 0x7B4290(idx)`（同一 28 スロット表、E8=48）で index14 を直接渡す |
| 7 | FUN_00498340+0x807 の 0xB42EA5 分類（v129 V144-c） | **READ（cmp）[確定]** | 実住所 0x498B45 = `cmp byte [0xB42EA5],0`（skipdata 懸念は解消、+0x807 表記は +0x805） |
| 8 | FUN_007B8530 arg0=0 の実行時 caller（v129 V144-d） | **0 件・到達不能 [確定(反証)]** | imm32 0x7B8530 = 0（全セクション）、E8/E9 = 1（0x497B56、唯一到達は push 1） |
| 9 | 0xB42EA4/EA6/EA7 役割（v129 小） | **writer 実測で確定 [観測]** | EA4 は読手1・書手0（bss 恒0）、EA6 は CRT init=0 のみ、EA7 は CRT=1→mirror=0、mirror 後は恒0 |
| 10 | 40E22E/40E2D2 パッチ前 jcc 極性（v128 #1） | **両サイトとも `0F 85`（jnz）[確定(論証)]** | 4 組合せ中、意味論が整合するのは jnz/jnz のみ（付録A-J） |
| 11 | 0x4060F5 WndProc 疑い（v128 #2） | **独立関数は 0x4060F0 起点の WndProc [確定]** | WinMain 0x40E76E `mov [esp+0x6C],0x4060F0` → RegisterClassExA（0x40E7B8） |
| 12 | 0xB42F31 のもう 1 つの clear 到達時期（v128 #3） | **CRT 初期化（WinMain 前）[確定]** | FUN_007B7250 ← stub 0xA10EB0（.CRT 初期化配列 0xA2EFF0 登録）、E8=1 |
| 13 | レンダラ slot0 のプロセス途中経路（v128 #4） | **0 件・稼働中は refcount≥2 で発火不可 [確定(静的)]** | slot0 間接発火 site 全 7 件が CRT init / WinMain 終了 / atexit のみ（付録A-M） |
| 14 | FUN_0067FCF0 / FUN_00683030 の名前（v128 #5） | **TESForm マップ / PathBuilder [確定(RTTI)]** | ctor 0x67FB70→vtable 0xA748F4、ctor 0x682EF0→vtable 0xA74C0C（`PathBuilder`） |
| 15 | 0x404940 戻り値の意味（v128 #6） | **「CD-ROM ドライブ存在」フラグ [確定]** | `mov al,bl` @0x4049EF、bl=1 は DRIVE_CDROM 分岐 0x4049C5（_access 成敗に関わらず） |
| 16 | 0x982D71 の同定（v128 #7、残点10 の後半） | **`_access`（CRT `__access`）[確定]** | 台帳 `FID_conflict:__access` ＋ 本体 {0,-1} 正規化 ＋ E8=14 件中 12 件の `cmp eax,-1`（付録A-P） |

---

## §3 結論（3行）

1. **renderer[+0x280] は生の `IDirect3DDevice9*`（FUN_0076a1c0 の CreateDevice out-param、IDirect3D9 は [0xB42154]）で確定**し、v129 の「+0x1AC が raw vtable 範囲外」矛盾は IDirect3DDevice9 の実スロット数 112（idx7 GetDeviceCaps / idx57 SetRenderState / idx65 SetTexture / idx111 CreateQuery）の数え違いとして消解。renderer vtable[+0x54] getter の caller もビット {2,4,8} テストで **10 site 特定**（うち 2 site は 0xB3F928 / 0xB42754 グローバル直読）。
2. **0x4FDE30（+0x1C）はコンパイラ用 StandardCompile であり impl(+0x18) を触らず**、実行時 impl transfer は **FUN_005169a0 @0x516A5D 読取 → 0x516AF3 `call ecx`（8 引数）** に所在（列アドレス 0xB0AF60 は .text 全体でこの 1 site）。ロガー [0xB34D90] は CRT 初期化で生成される bss static **MessageHandler（0xB33F80、vtable 0xA3DA08）**。
3. **40E22E/40E2D2 のパッチ前 jcc は両サイトとも `0F 85`(jnz)**（al=CD 有無 / [0xB33394]=launcher 検出の規約実測 + 4 組合せ排除論証）、`0xB42F31` の第2 clear は **CRT 初期化（.CRT$XCU 0xA2EFF0 → stub 0xA10EB0）で WinMain 前に 1 回**、**レンダラ slot0 のプロセス途中経路は 0 件**（発火 site 全 7 件が CRT init / WinMain 終了 / atexit、稼働中の refcount ≥2 が保証）。

---

## §4 残留不明点

| 粒度 | 内容 |
|---|---|
| 小 | +0x54 getter caller 9 site のうち base が引数/メンバ経由の 7 site（0x760755 / 0x7613F6 / 0x761409 / 0x7688D3 / 0x72AA12 / 0x72AA20）はポインタトレースの最終追跡未実施（ビット値+帯域での同定。provenance は [仮説(高)]） |
| 小 | `0x7790A0`（NiD3DShaderInterface::GetType）の配置 vtable は 0xA8A7CC で RTTI 確定済みだが、`0x7385F0`（NiShader::GetType, `mov eax,0xB40124;ret`）の配置 vtable は未照合 |
| 小 | WndProc 0x4060F0 の終端境界（開始 0x4060F0 と次台帳関数 0x406360 の間の ret 位置）は未検証。jumptable 0x4062F8 / byte テーブル 0x40630C が内部にあり得る |
| 小 | 0xB42EB8 の正体は「シェーダオブジェクト保持グローバル」[観測]（書手 19 の値の厳密なクラスは未同定。レンダラでないことは slot0 0 件で確定） |
| 小 | `[0xB43110]` が指すオブジェクトの厳密なクラス（FUN_007BA0F0 の esi はレンダラかシェーダマネージャか）は未特定（slot0 論証には非必須: B350D8/B43104 のみで下限成立） |

---

## §5 自己検証

- **ハーネス**: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。作成期 着手指定時・完走時 ＋ 本継続 再開時・完走確認時の計 4 回、全回 PASS）。
- **内部整合検証**（`%TEMP%\opencode\v149_15_verify.py` = **42 項目 42/42 OK**）:
  1. `.text` 線形デコード **2,258,215 命令** = GT ✓
  2. byte パターン `84 c0 90 e9`=**1** / `84 c0 0f 84`=**1,196** / `84 c0 0f 85`=**827** = v128 ✓
  3. `call [reg+0x54]` 全数 = **6** = v129「6 件非該当」✓
  4. 絶対参照: **B42EA5=10**（v129 の 10 site と一致）/ B42EA0=1 / EA1=1 / EA2=1 / EA3=1 / **EA4=1** / **EA6=2** / **EA7=4** / **B43104=159**（v128 一致）/ **B350D8=33**（=2W+31R、v128 一致）/ **B42754=10**（v128 一致）/ **B34D90=50**（2W+48R）/ **B42EB8=70** / **B42F31=5**（v128 一致）/ **B3F928=268**（書 3/読 265）→ **14/14 一致**
  5. imm32: **0x7B8530=0** / **0x4060F0=1** / **0x50AA20=1**（entry82+0x18 のみ）/ **0x76BED0=1**（renderer vtable slot0）/ **0x769F20=1**（renderer vtable +0x54）/ impl 列 disp 0xB0AF60 = **1 site** → 6/6 ✓
  6. renderer vtable 0xA88EA4: [0]=0x76BED0 / [+0x54]=0x769F20 ✓、entry82(+0x18)=0x50AA20 / (+0x1C)=0x4FDE30 ✓（opcode_table とバイト一致）
  7. 型レコード文字列: cstr(0xA8A84C)="NiD3DShaderInterface" / cstr(0xA80DAC)="NiShader" / MessageHandler vtable[0]=0x494390 ✓（3/3）
  8. E8 件数（cache 照合）: **0x7B4290=48** / **0x769F20=0** / **0x4FDE30=0**（GT「dispatcher は間接経由のみ」と一致）/ **0x7B8530=1** / **0x406950=1** / **0x7B7250=1** / **0x50AA20=0** / **0x497B50=1** / **0x497E70=1** → 9/9 ✓
- 独立再測の一致: +0x54 load→call = 333 site 全数から flag-style 9 site を v149_12/v149_13 の 2 パス（EAX ファミリ考慮）で一致抽出、slot0 site = 7 件を v149_10 の前方走査と v149_06/07 の個別デコードで相互確認。
- **追補検証（本継続セッション）**: `v149_10_final`（`call [reg+0x54]` 全数 **6** 件を独立再確認、load→call 独立走査で renderer 帯該当 site と `0x76A833`/`0x7716F9`/`0x771710` 等が一致、E8→`0x403160`=**8**、0x7B7FC0 の引数位置）、`v149_16_access`（0x982D71: E8=**14** 件中 **12** 件が `cmp eax,-1`、台帳 `FID_conflict:__access`、内部 0x982CEA=`FID_conflict:__waccess_s`）、`v149_17_d8c`（0xB34D8C: imm32=**1** 件・`lea` 形 **0** 件・隣接ベース相対書手 **0** 件 = 付録A-D の裏付け）、`v149_15_verify` **再実行 42/42 OK**。
- 編集範囲: 成果物は本ノート 1 件のみ。中間スクリプトは `%TEMP%\opencode\v149_*.py`（20 本）。`docs/`・`TODO_re49.md`・台帳・git・Supermemory・sync は無変更。

## §6 Android 移植への示唆

レンダラのデバイス保持は「生成時に CreateDevice の out-param をそのまま入れる生 COM ポインタ一枚」なので、移植では `VkDevice`/`GLES context` を保持するフィールド 1 本（+0x280 相当）＋ caps 構造体（+0x284, 0x130 B）で代替でき、state 経由のデバイス dispatch（+0xE4=SetRenderState / +0x104=SetTexture / +0x1AC=CreateQuery）はデバイス抽象層 3 関数に集約できる。テクスチャ能力フラグ（+0x5E0、getter vtable+0x54）はビット {2,4,8} をテストする 10 個所から読まれるだけなので、移植ではビットマスク定数＋getter 1 本（virtual を剥がして直接参照可）で置換可能。スクリプト opcode の二層構造（+0x1C=コンパイラ dispatcher / +0x18=実行時 impl、実行時 transfer は FUN_005169a0 の一箇所）は移植時に「コンパイル時ハンドラ配列」と「実行時ハンドラ配列」を分離すれば同じ設計で再現でき、実行時 impl 呼出しは 1 site の差し替えで済む。コピー保護 jcc 2 本は移植対象外の死コード。renderer slot0（破棄）は CRT/atexit/終了経路のみに束ねられているので、Android では `onDestroy` でまとめて release する 1 経路に集約してよい（実行中の再生成・再破棄は原典に存在しない）。ログは bss static MessageHandler シングルトン 1 本への virtual 出力（vtable[0]）なので、Android では同一シグネチャのログバックエンド 1 本に差し替えれば 40 箇所の呼び出し元を無改造で維持できる（実装は行わない）。


---

## 付録A 個別裁定と証拠（A–P）

### A. (V143-a) renderer[+0x280] のクラス名 = **IDirect3DDevice9\*** [確定]

**書込み機構（明示ストアが存在しないことの全数証明）**: `.text` 線形デコード 2,258,215 命令の `[reg+0x280]` アクセス 176 site 全数のうち、レジスタ基底の書き手は FUN_007641a0+0x18（0 化）と FUN_0076b5d0+0x489（破棄時 0 化）のみ。実体の書込は **out-parameter 経由**。

**CreateDevice 呼出し（FUN_0076a1c0+0x363〜+0x385、全文デコード）**:

```asm
0x76A50C: mov  eax, dword ptr [0xB42154]   ; IDirect3D9* グローバル
0x76A511: mov  ecx, dword ptr [eax]        ; vtable
0x76A513: lea  esi, [ebp + 0x280]          ; &renderer[+0x280] = ppDevice (最終引数)
0x76A519: push esi
0x76A51A: lea  edx, [esp + 0x4C]           ; &D3DPRESENT_PARAMETERS (スタック)
0x76A51E: push edx
0x76A51F: push dword ptr [ebp + 0x5C4]     ; hFocusWindow
0x76A526: push dword ptr [ebp + 0x3B8]     ; BehaviorFlags
0x76A533: push dword ptr [ebp + 0x5C0]     ; DeviceType（0x76A4FB で 2 = D3DDEVTYPE_HAL）
0x76A53A: push dword ptr [ebp + 0x5BC]     ; Adapter
0x76A541: push eax                          ; IDirect3D9* (this)
0x76A542: mov  eax, dword ptr [ecx + 0x40] ; vtable[+0x40] = idx16 = IDirect3D9::CreateDevice
0x76A545: call eax
```

- 引数 7 個（this + 6）の並びが `CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pp, ppPresentationParameters)` と**完全一致**。`[0xB42154]` は同一関数内のアダプタ走査（`[ecx+0x14]` = idx5 = GetAdapterIdentifier、GUID `repe cmpsb` @0x76A4DB）でも **IDirect3D9\*** として整合 [確定]。
- 以降の使用（v129 §4-1 の引用を再確認）: vtable **+0x1C = idx7 GetDeviceCaps**（out = &renderer[+0x284] = D3DCAPS9）、**+0xE4 = idx57 SetRenderState（this+2 = 3 引数）**、**+0x104 = idx65 SetTexture（3 引数）**、**+0x1AC = idx111（tail-jmp、3 引数）**。IDirect3DDevice9 のスロット列は GetDeviceCaps=7 / SetRenderState=57 / SetTexture=65 / **CreateQuery=111（最終スロット）** と対応し、**総数 112（idx 0..111）**。
- → **v129 §4-2 の「raw D3D9 デバイス vtable（86 スロット）の範囲外」という矛盾は消解: 86 の数え方が誤りで、+0x1AC は最終スロット（CreateQuery）の範囲内** [確定(機序)]。
- refcount: state セッタ（FUN_0077F840）が vtable[1]=AddRef / vtable[2]=Release を呼ぶ = IUnknown 互換 ✓。**v129 の「DeviceWrapper」は内部呼出用の通称であり、厳密なクラス名は生の `IDirect3DDevice9`（d3d9.dll 実体、wrapper クラスは存在しない）**。v129 の「+0x1AC が 86 スロット外」という観測矛盾の原因はスロット数の数え違いに特定 [確定]。

### B. (V143-b) renderer vtable[+0x54] getter の呼び出し元 — **10 site 特定** [確定(観測)]

- `call dword [reg+0x54]` 形: **6 site**（0x8AA07B / 0x8B8C98 / 0x8CBB89 / 0x8D9216 / 0x91FAE9 / 0x91FB01 = すべて 0x8A〜0x91 帯の非 renderer 系。disp32 形は 0、v129 の「6 件非該当」と一致）。E8 → 0x769F20 = **0**（直接呼出し 0 件）。
- load→call 形（`mov R,[base+0x54]` → 8 命令以内 `call R`）= **333 site 全数**。うち**呼び出し直後に +0x5E0 の実測値集合 {0,2,4,8} と一致するビットテスト（test al,2/4/8）が続く site = 9**、shr 連動形 = 1:

| call site | 所有関数 | base 由来 | 直後のテスト |
|---|---|---|---|
| **0x722F98** | FUN_00722F00+0x95 | **`mov ecx,[0xB3F928]`（レンダラグローバル直読）** | `test al,2` |
| **0x7716F9 / 0x771710** | FUN_007716e0+0x16/+0x2B | **`mov ecx,[0xB42754]`（レンダラ保持グローバル直読）** | `test al,8` / `test al,4` |
| **0x76A833** | FUN_0076a1c0+0x66B（レンダラ Initialize 内） | `mov ecx,ebp` = this = レンダラ | `shr eax,1; and eax,0xFFFFFF01` → 0x778C80 へ |
| 0x760755 | FUN_00760700+0x52 | `[esi+8]`（メンバ） | `test al,8` |
| 0x7613F6 / 0x761409 | FUN_00761270+0x183/+0x196 | `[esp+0x2C]`（引数ローカル） | `test al,8` / `test al,4` |
| 0x7688D3 | FUN_00768890+0x3E | `ebx` = this（本関数は 0xB42858=NiD3DShaderInterface の IsOfType 判定も併持） | `test al,2` |
| 0x72AA12 / 0x72AA20 | FUN_0072a9b0+0x5D/+0x6B | `ebp`（引数） | `test al,8` / `test al,4` |

- 判定根拠: (a) ビット集合 {2,4,8} は v129 §3 が実測した +0x5E0 の設定値そのもの、(b) base が既知レンダラグローバル（0xB3F928 / 0xB42754）直読の 2 site、(c) 帯域 0x722F00–0x771710 はレンダラ/シェーダ実装帯。
- 除外: `test ah,1` ×2（0x54A0C6 / 0x55C871）は +0x5E0 に存在しない値（0x100）→ 別クラス [確定]。`shr eax,0x1c` 形 ×5（0x7C6143 / 0x7C9B37 / 0x7E6846 / 0x7EEDA5 / 0x81136A）は `cmp eax,-1` など flags 値集合と不整合 → 別クラスの +0x54 getter（0x7C9B37 は直後に `cmp eax,-1` があり renderer flags と矛盾）[確定]。
- **0xB42754 の帰属も本調査で裏付**: FUN_007716E0 が [0xB42754] の vtable[+0x54] 戻りを `test al,8/4` する = [0xB42754] もレンダラ本体 [確定(機序)]（v128 §4-1 の「値=param の set/clear 兼用」と整合）。
- 残留: base が引数/メンバ経由の 7 site は「レンダラ実体」をポインタトレースで最終追跡していない（ビット値 + 帯域での同定）→ 個別 provenance は [仮説(高)]、caller 集合自体は [確定(観測)]。

### C. (V143-c) 0x4FDE30 内 impl transfer — **前提訂正と正位置の特定** [確定]

- **0x4FDE30 側に transfer は存在しない** [確定(反証)]: 0x4FDE30..0x4FF900 の全命令を走査し、`[reg+0x18]` アクセス **10 site 全部が `[esp+0x18]`（スタックローカル）**、`jmp [reg]` 0 件、`call [reg]`（disp0）0 件。列アドレス直接参照も 0x4FDE30 域には 0 件。
- **+0x1C の正体の訂正**: 現行 VM 的に見えていた 0x4FF8A0〜0x4FFA1D は実際には **FUN_004FF710（コンパイラ側 begin ブロック処理、GT 記載と一致）**。`0x4FF904: lea eax,[ebx+0xB0AF48]` → `0x4FF963: mov ebx,[eax+0x1C]` → `0x4FF9B9: call ebx`、push 引数は（文字列 / argdesc / argn / …）= 0x4FDE30 = **ScriptCompiler::StandardCompile（コンパイラ用共有 dispatcher）**。v129 の「VM 取り出しループ」呼称は誤り（コンパイラループ）→ 訂正。
- **impl(+0x18) の実 consumer = FUN_005169a0（実行時 executor）**:

```asm
0x516A3E: lea eax,[ecx+ecx*4] ... (×2)          ; eax = idx*20
0x516A45: mov ecx,[eax+eax+0xB0AF5C]             ; entry[+0x14] = argdesc
0x516A4C: add eax,eax                            ; eax = idx*40 = idx*0x28
0x516A5D: mov ecx,[eax+0xB0AF60]                 ; ★ entry[+0x18] = impl
0x516A67: je  0x516A9F                           ; [esp+0x77c]==0 → 実行モード
   （非 0 経路: argdesc → 0x4FAE80 引数ゲート、impl は破棄）
0x516A9F: cmp byte [eax+0xB0AF58],0              ; entry[+0x10] flagA
0x516AA6: je  0x516AD6 / test ebx,ebx; jne ...
   （flagA!=0 かつ ebx==0 → 0x4A7A60 でエラー文字 0xA4DCC0 出力）
0x516AD6: test ecx,ecx / je 0x516B2A             ; impl==NULL → エラー 0xA4DC88
0x516AF3: call ecx                               ; ★★ impl 実行（8 引数、cdecl add esp,0x20）
```

- 全体整合: 列アドレス走査で **disp 0xB0AF60（impl 列）は本 1 site のみ**、disp 0xB0AF5C（argdesc 列）= 0x516A45、disp 0xB0AF50（opid 列）= 0x4FF88E、disp 0xB0AF48（entry 先頭）= 0x4FF904。imm32 0x50AA20 は entry82 の +0x18（0xB0BC30）**1 箇所のみ**・E8 → 0x50AA20 = 0 → **impl は必ず表経由の間接 transfer** [確定]。

### D. (V143-d) [0xB34D90] ロガー = **MessageHandler シングルトン** [確定]

- 絶対参照 50 site（書 2 / 読 48）。書き手 2 件:
  - `0x49468B: mov [0xB34D90], esi`（FUN_00494650、thiscall ctor/initializer。`mov [esi],0xA3DA08` で vtable 設定、`ret 4`、引数は byte mode。あわせて `[0xB27E60]:=0x6F7070`、`[0xB40608]:=0x60D0A0` を登録）
  - `0x49471B: mov [0xB34D90], 0`（FUN_004946B0、dtor。CreateFileA(0xA28090) → GetFileSize → ReadFile(0xA280A0) → サイズ `cmp edi,0x46` 検証 → 同一 instance のときのみ 0 化。E8=1 = 0x40C4FA（終了処理系））
- **クラス同定**: ctor が書く vtable **0xA3DA08** を `cache/vtable_index.tsv` で照合 → **`MessageHandler`（`.?AVMessageHandler@@`、11 スロット、COL 0xAB8DA4、TypeDescriptor 0xB06AF8）** [確定(RTTI)]。vtable[0] = 0x494390 = FUN_004941A0+0x1F0（= 本文書式化関数内に置かれた出力仮想関数）で、v129 §9-b の最終 sink と一致。
- **格納元**: CRT 静的初期化 stub `0x9DC330: push 1 / mov ecx,0xB33F80 / call 0x494650 / push 0xA186C0 / call 0x981FB4(atexit)`。当該 stub は**初期化関数ポインタ配列（.rdata 0xA28840〜0xA2885C 帯、0x9DCxxx 連番 + 0 セパレータ）に登録** [確定]。atexit 登録の dtor 0xA186C0 = `mov ecx,0xB33F80; jmp 0x4946B0` [確定]。
- → **[0xB34D90] はヒープではなく bss 上の static MessageHandler インスタンス 0xB33F80 を指す singleton**。40 超の `mov ecx,[0xB34D90]` 読手（FUN_00421000 / FUN_00458ED0 系 / FUN_004941A0 等）はすべてこの instance を this に呼ぶ [確定]。FUN_004941A0 のゲート `[0xB34D90]==0 → 即 return` は「初期化前/後始末後は出力なし」を意味する [確定(機序)]。0xB43104（別物）とは無関係 [確定]（v129 の未特定点を解消）。
- **隣接 [0xB34D8C] の裁定（b24 残点1 の解消） [確定(反証)]**: 全 3 セクションの imm32 走査で **1 件のみ（読手 0x494767 = 本ライタ内部）**、`lea reg,[imm]` 形 **0 件**、隣接ベース経由の相対書式も全走査（0xB34D88 ロード 18 件の前方 14 命令内に `[reg+4]` 書込 **0 件**、0xB34D90 ロード後の相対書込は `[reg+4]/[reg+8]/[reg+0xC]` のみ = 0xB34D94/D98/D9C 対応）→ **書き手 0 件 = bss 恒 0**。
- 文脈（0x494767〜）: `mov eax,[0xB34D8C]; test; je 0x4947BF; cmp [0xB34E80],0 → …; cmp byte [eax+8],0; lea ecx,[eax+8]; mov [0xB34E80],1` = **一度きりガード付きコンテキストポインタ**。読手 1 件・恒 0 のため当該ブロックは**実行時死コード** [確定(観測)]。
- → **b24 残点1 の「[0xB34D8C] コンテキストオブジェクト」は「未初期化のまま読まれるだけの未使用ポインタ」であり、メッセージ本文の出所ではない**。[0xB34D90]（MessageHandler インスタンスへのポイントラ）とは**隣接（-4）する独立グローバル**で、ロガー本体（ctor 0x494650 / CRT stub 0x9DC330）とは初期化経路・用途とも無関係 [確定(反証)]。

### E. (V144-a) 型レコード連鎖の根 — **0xB42858 = "NiD3DShaderInterface"、根 = "NiShader"** [確定]

- 登録 stub 実測（.CRT 初期化 stub 群 0xA10BA0）: `push 0xB40124; push 0xA8A84C; mov ecx,0xB42858; call 0x70E220` → `cstr(0xA8A84C) = "NiD3DShaderInterface"`、parent = **0xB40124**。
- 0xB40124 の登録 stub（0xA0A488）: `push 0; push 0xA80DAC; mov ecx,0xB40124; call 0x70E220` → `cstr(0xA80DAC) = "NiShader"`、parent = **0（ルート）** [確定]。
- 既知の親子（v129 §9-a）と接続: **NiD3DDefaultShader(0xB4257C) → NiD3DShader(0xB42884) → NiD3DShaderInterface(0xB42858) → NiShader(0xB40124) = ルート** [確定]。
- 付随: `0x7790A0: mov eax,0xB42858; ret` は **NiD3DShaderInterface の GetType()**（vtable 0xA8A7CC の slot+0x04 に配置、RTTI 索引照合 1/1 一致）。`0x7385F0: mov eax,0xB40124; ret` = NiShader 系 GetType（照合対象 vtable 未走査 → owner は [未特定・小]）。0xB40124 の他参照: 0x723124（IsOfType 判定）、0x7385F1、0xA0A488、0xA10BA1 の 4 件のみ。

### F. (V144-b) FUN_00553bb0 の 0xE の名称 — **SkinShader（case14）** [確定]

- 選択点実測: `0x553EA2 cmp [0xB42EA5],bl / push 1 / push 1 / je 0x553EB2 / push 0x1A / jmp 0x553EB4 / 0x553EB2: push 0xE / 0x553EB4: push ebp / call 0x7B8940`。= **b42ea5(Lighting30) != 0 → 0x1A、== 0 → 0xE** [確定]。
- index の行方: FUN_007B8940（this=ebp, idx, 1, 1）→ **FUN_007B7FC0（arg2=idx）** → `0x7B8060: cmp edi,0x1C / jle`（idx≤0x1C）→ `0x7B809D: push eax / call 0x7B4290` = **v129 §2 の同一 28 スロット遅延確保テーブル**（E8=48 を実測）→ arg0=14 は v129 §2 表で **SkinShader**（arg0=26 = Lighting30Shader）[確定]。
- 付随の index 処理: idx>0x1C は `[0xB42E8C]` のコールバック（0x405150、WinMain 0x40E8C1 で登録）経由のカスタム経路、idx==1 は FUN_007D1920 特例 [観測]。従って 0xE の名称は「本 switch の case14 = SkinShader」で v129 の推定どおり [確定]。

### G. (V144-c) FUN_00498340 の 0xB42EA5 参照 = **READ** [確定]

- 実測: `0x498B45: cmp byte ptr [0xb42ea5], 0`（所属 FUN_00498340+0x805）。線形デコードで正常復号され **skipdata 域ではない**。v129 の「+0x807」はアドレス表記のズレ（+0x805 が正）[確定]。
- 他の 0xB42EA5 参照（合計 10 site、v129 の 10 件と一致）: 書 2 = 0x7B467B（FUN_007b45f0+0x8B, =1）/ 0x7B7648（FUN_007b7250+0x3F8, =0, CRT 初期化）、読 8 = 0x498B45 / 0x553EA2 / 0x7B4475 / 0x7D1AC9 / 0x7D1BBD / 0x7D1BCD / 0x7DABB3 / 0x7DABF5 [確定]。

### H. (V144-d) FUN_007B8530 arg0=0 経路の実行時 caller — **0 件** [確定(反証)]

- 全 3 セクションの imm32 走査で **0x7B8530 の出現 0 件**（関数ポインタ登録・atexit・テーブル登録が皆無）。E8/E9 → 0x7B8530 = **1 件のみ（0x497B56、FUN_00497B50 内）**、その唯一の到達 caller は WinMain 終了経路 0x40F5CA（`push 1`、E8 → 0x497B50 = 1、0x497E70 = 1 の不達経路は0xB34FA7 ガードで恒久不達 = v128 §4-4 と一致）。
- → **arg0=0 は静的・実行時とも到達不能**（残るのは jmp reg 等の動的 transfer のみだが、登録データ 0 件でそれを支える構造なし）[確定(反証)]。

### I. (小) 0xB42EA4 / 0xB42EA6 / 0xB42EA7 の各役割 [観測]

絶対参照の全数（読みは decoded 線形走査、書きは oi=0 判定）:

| byte | 書き手 | 読手 | 裁定 |
|---|---|---|---|
| 0xB42EA4 | **0 件**（dword 上書きの側溝も 0x42EA1..0x42EA4 で書 0 件を確認） | 1（0x7C1C5A `cmp ...; jne 0x7C1C74`） | bss 恒 0。FUN_007C1B50 の gate 列末尾（==0 で 0x7C1C63 = ebx リマップ判定へ）[確定(観測)] |
| 0xB42EA6 | 1（0x7B75EF FUN_007b7250+0x39F `=0`、CRT 初期化） | 1（0x7DA459 `cmp ...; 0` FUN_007da220+0x239） | 恒 0 → reader 分岐は実行時常に taken [確定(観測)] |
| 0xB42EA7 | 2（0x7B7316 CRT 初期化 `=1` / **0x406ABD mirror 0x406950 内 `=0`**） | 2（0x7ED7E0 FUN_007ed6c0+0x120 / 0x7EDDE9 FUN_007edcd0+0x119、いずれも `je`） | **「mirror 前（未初期化）」フラグ**。CRT 初期化=1 → mirror=0 → 以後恒 0 なので reader 側ブロック（0xB40224 由来の float 設定）は実行時に死化 [確定(観測)] |

- gate 列全体像: FUN_007C1B50+0xDE〜+0x10A は `cmp [0xB42EA0..3],0 → je 0x7C1C63 / jmp 0x7C1C74`、`cmp [0xB42EA4],0 → jne 0x7C1C74`、合流点 `cmp ebx,0x24; jne 0x7C1C74; ebx:=0x71` = **定数 0x24→0x71 リマップ gate**（EA0..EA4 は恒 0 のため実行時は常に同じ経路）[確定(観測)]。
- 付随: 0xB42EA8 は float（`fstp/fld` 5 site: 0x498D84 / 0x50B18C / 0x50B367 / 0x540DCC / 0x540E6D）。EA5 は v129 §6-2 記載どおり（書 2 / 読 8）[一致]。

### J. (v128 #1) 40E22E / 40E2D2 のパッチ前 jcc 極性 — **両サイトとも `0F 85`（jnz）** [確定(論証)]

**byte 実測（raw）**:

```
0x40E22C: 84 c0           test al,al
0x40E22E: 90              nop            ← 旧 0F 8x の第1バイト
0x40E22F: e9 97 00 00 00  jmp 0x40E2CB   ← rel32 保存（0x40E234+0x97 = 0x40E2CB）
0x40E2CB: 80 3d 94 33 b3 00 00   cmp byte [0xB33394],0   （7 バイト境界復元）
0x40E2D2: 90              nop            ← 旧 0F 8x の第1バイト
0x40E2D3: e9 bb 00 00 00  jmp 0x40E393   ← rel32 保存（0x40E2D8+0xBB = 0x40E393）
```

`.text` パターン再測: `84 c0 90 e9`=1 / `84 c0 0f 84`=1,196 / `84 c0 0f 85`=827（v128 の値と一致）。極性は byte では復元不能（0F 84/85 ともに上書き済み）。

**決定根拠（`0x404940` の戻り値規約を実測した上での排除論証）**:

1. `0x404940` は `xor bl,bl`(0x404966) → ドライブ走査 → **`mov bl,1`(0x4049C5) は `GetDriveTypeA==DRIVE_CDROM` 分岐内で `_access` の成否に関わらず設定** → `0x4049EF: mov al,bl`。よって **al = 「CD-ROM ドライブの有無」**（[0xB33394] = ドライブ文字は `_access("%sOblivionLauncher.exe")` 成功時のみ）[確定]。
2. ブロック A（0x40E234..0x40E2C6）= INI `CopyProtectionStrings` の Title/Message を読んで `MessageBoxA(MB_OK|MB_ICONERROR)` → **無条件 return 0**（致命ダイアログ）。A が走るべき状況は「チェック失敗 = al==0」→ fall-through が al==0 → **jcc は al!=0 で飛ぶ = jnz**。
3. ブロック B（0x40E2D8..）= Title2/Message2 を読んで `MB_RETRYCANCEL`、`0x40E381 call 0x404940 → 0x40E386 cmp [0xB33394],0 → 0x40E38D je 0x40E2F0`（= launcher 未検出なら再表示ループ、検出で抜けて 0x40E393 続行、Cancel で return 0）。B が走るべき状況は「[0xB33394]==0」→ fall-through が ==0 → **jcc は !=0 で飛ぶ = jnz**。
4. **4 組合せの排他検証**: (jz,jz) = CD ありで致命エラー / CD なしでチェック完全無効、(jz,jnz) = CD ありで致命エラー、(jnz,jz) = 成功時に 1 回だけ余計なリトライが出る & launcher 欠落時は検査が素通り、(jnz,jnz) = **「CD なし → 致命 A」「CD あり・launcher 欠落 → リトライ B」「検出成功 → 続行」のみ整合**。
- → **両サイトとも `0F 85`(jnz) [確定(論証)]**。byte 復元そのものは不可能なため、残余リスクは「A/B の役割付与（v128 §3-4 の INI キー実測）が誤っている」場合のみ。rel32・CFG（入辺 0）は v128 測定を再確認。

### K. (v128 #2) 0x4060F5 疑い — **独立関数は 0x4060F0 起点の WndProc** [確定]

- ジャンプテーブル（`jmp [esi*4+0x4060AC]`、17 エントリ = 68 B）は 0x4060AC..0x4060EF。**次のバイト 0x4060F0 から新関数**: `53 / 8B 5C 24 10 / 55 / 8B 6C 24 0C / 56 / 8B 74 24 14 / 81 FE 84 00 00 00 / ...` = `push ebx; mov ebx,[esp+0x10]; push ebp; mov ebp,[esp+0xC]; push esi; mov esi,[esp+0x14]; cmp esi,0x84; ...`。esi = **メッセージ**（cmp 0x84 = WM_NCHITTEST）、`lea eax,[esi-2]; cmp eax,0x45; ja …; movzx eax,[eax+0x40630C]; jmp [eax*4+0x4062F8]` = メッセージ 2..0x47 の switch → **ウィンドウプロシージャ** [確定]。
- **登録証拠**: imm32 0x4060F0 = **1 件のみ 0x40E772**（WinMain 内 `mov dword [esp+0x6C],0x4060F0` = WNDCLASSEX.lpfnWndProc、直後に 0x40E7B8 `call RegisterClassExA` 相当）。E8/E9 → 0x4060F0 / 0x4060F5 = **0 件**（純 callback）[確定]。
- → v128 の「データ直後の継続か独立関数か未確定」は **独立関数（登録された callback）**で確定。開始境界 ret+int3 規則はデータ隣接のため適用不能だが、imm32 登録が同等以上の証拠。台帳は FUN_00405CE0 [0x405CE0,0x406360) に 0x405D60 と同様にマージしており、**0x4060F0 は台帳 gap 内の独立関数**（終端は次の台帳関数 0x406360 直前まで [仮説]）。内部で FUN_00403160 / FUN_004046A0 / 0x497BF0 / IAT デフォルトプロシージャ を呼ぶ（`cmp [0xB350D8],0` ゲード付き 0x497BF0 = 設定 2-dword 保存 setter、E8=1 = 本 WndProc のみ）[観測]。

### L. (v128 #3) 0xB42F31 のもう 1 つの clear — **CRT 初期化（WinMain 実行前）** [確定]

- 0xB42F31 の参照総数 5（書 3 / 読 2、v128 一致）。clear 2 本の所在:
  1. `0x7B88D3`（FUN_007B8530+0x3F3）= WinMain 終了経路（v128 測定どおり）。
  2. **`0x7B734A`（FUN_007B7250+0xFA）** = 本調査で到達時期を特定:
     - FUN_007B7250 の E8 = **1 件のみ 0xA10EB5**。
     - その本体 stub = **`0xA10EB0: mov ecx,0xB430D8 / call 0x7B7250 / push 0xA27150 / call 0x981FB4(atexit) / ret`**。
     - imm32 0xA10EB0 = **0xA2EFF0** = `.CRT$XCU` 初期化関数配列（V146 で 483 連続 .text ポインタ実測済みの帯、0xA2EFE0..0xA2F01C に 0xA10D90..0xA10F70 が連続）→ **CRT スタートアップ（WinMain より前）に 1 回実行** [確定]。
     - 同 stub の atexit dtor 0xA27150 = **`ret`（no-op）** [確定]。
- FUN_007B7250 は本体でも確認できる通り「シェーダグローバル初期化」（28 スロット表 0xB42EC0..0xB42F2C の 0 化、`[0xB42EA5]:=0`、`[0xB42EA6]:=0`、`[0xB42EA7]:=1`、`[0xB42F48]:=0` shader level、`[0xB42EB8]:=0`、`[0xB42F31]:=0`）。
- → **初回 set（0x7BA2D1、WinMain 0x406A83 → FUN_007BA0F0）より前の起動時リセットであり、実行中の値に影響しない**（= v128 残点の「実害の有無」= 無）[確定]。遷移は「CRT init: 0（+EA5:0/EA6:0/EA7:1/level:0）→ WinMain mirror: 1 → WinMain 終了: 0」の一本。

### M. (v128 #4) レンダラ slot0 のプロセス途中経路 — **0 件** [確定(静的)]

**slot0（= FUN_0076BED0、imm32 は renderer vtable 0xA88EA4 の 1 箇所のみ）間接発火 site 全数 = 7**（レンダラ保持グローバル 5 種の絶対ロード → `mov R2,[R] → mov R3,[R2] → call R3` 前方走査の全数 + 既知 site 突合）:

| # | site | 対象グローバル | 到達経路 | 時期 |
|---|---|---|---|---|
| 1 | 0x497B5B（FUN_00497B50） | [0xB350D8] | WinMain 0x40F5CA `push 1`（E8=1） | **WinMain 終了** |
| 2 | 0x7B74FD（FUN_007B7250+0x28D） | [0xB43110] | CRT 初期化配列 stub（E8=1）。実行時は値 0 で no-op | **CRT init（WinMain 前）** |
| 3 | 0x7B8742（FUN_007B8530） | [0xB43110] | FUN_00497B50 ← WinMain 終了 | **WinMain 終了** |
| 4 | 0x7B88F9（FUN_007B8530） | [0xB43104] | 同上 | **WinMain 終了** |
| 5 | 0xA19F15（FUN_00A19EF0） | [0xB350D8] | **atexit**（stub 0x9DF280 `push 0xA19EF0; call 0x981FB4`） | **プロセス退出** |
| 6 | 0xA27185（FUN_00A27160） | [0xB43104] | atexit（0xA10ED0 系 stub） | **プロセス退出** |
| 7 | 0xA27365（FUN_00A27340） | [0xB43110] | atexit | **プロセス退出** |

- **AddRef（保持者）**: 0x498560（[0xB350D8]、生成 FUN_00498340）/ 0x406A62（[0xB43104]、mirror FUN_00406950 が `[0xB43104]:=[0xB350D8]` して Increment）/ 0x7BA074（[0xB43110]、shader init FUN_007BA0F0）/ `[0xB3F928]` 経由 6 site（0x70BF72 / 0x70C043 / 0x7AAAA2 / 0x7C7D10 / 0x81372D / 0x813BFB = 各オブジェクトのメンバ保持生成）。
- **稼働中の下限**: [0xB350D8] と [0xB43104] の解放はそれぞれ WinMain 終了経路のみ（上表 1,3,4 と atexit 5,6。クリア書込も B350D8=2 / B43104=2 でいずれも終了系のみ = v128 §4-1 と一致）→ **メッセージループ稼働中は refcount ≥ 2 が保証され、slot0 に到達できない** [確定(静的)]。
- **v128 が懸念した「0xB42EB8 の 21 書込経路」は棄却** [確定(反証)]: 0xB42EB8 の絶対参照 70 site を全数走査した結果、**0xB42EB8 経由の vtable[0] ディスパッチは 0 件**。書手 19 site はすべてシェーダ帯（FUN_007A9820 / 0x7AAA30 / 0x7D2070 / 0x7F6FC0 / 0x7F7680 / 0x7F7EE0 / 0x7F86C0 / 0x7F8DB0 / 0x7F9410 + CRT/shutdown の 0 化）で、レンダラではなくシェーダオブジェクトの保持グローバル [確定(観測)]。
- 限定: 「レンダラのローカルコピーをメンバに持つ他オブジェクトが汎用ヘルパ経由で解放」する経路は機械的に全数列挙できないが、上記の refcount 下限論により slot0 には到達しない（1 リリースでは 0 にならない）。
- 付随で判明: **`0x403160` は InterlockedDecrement ではなく `if ([this+0x28]) vtable[+0x28] 呼出` のメンバDetach系、`0x4046A0` は `[this+0x18F0]` カウント配列のフラッシュ関数**であり、WinMain 0x40EAB1 / WndProc の `[app+0x20]` パターンは NiPointer 解放ではなく**メソッド呼出**（v128 §2-2 #2 の「NiPointer 解放」記述は訂正的に読むこと）。[app+0x20] は renderer 生成（0x4052F0）では書かれず、+0x18F0 レイアウトもレンダラ不整合 → **renderer ではない** [確定(機構)]。

### N. (v128 #5) FUN_0067FCF0 / FUN_00683030 の名前付きカテゴリ [確定(RTTI)]

- **FUN_0067FCF0**: `if ([0xB3BE00]==0) { malloc(0x10) → ctor 0x67FB70(this, 0xBF) → [0xB3BE00]:=eax }`。ctor が書く vtable **0xA748F4** = RTTI **`NiTMapBase<unsigned int, TESForm*, …>`**（mangled `.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@PAVTESForm@@…`、7 スロット）→ **「FormID(u32) → TESForm\* の NiTMap（バケット 0xBF=191）遅延シングルトン生成関数」** [確定]。読手（0xB3BE00 参照 22 site）: FUN_0067F180（破棄）、0x67F340、0x67F5D0、0x67F690、0x67F830、0x67FD70、0x680090、0x680110 = マップ操作ヘルパ群。
- **FUN_00683030**: `if ([0xB3BF80]==0) { malloc(0x48) → ctor 0x682EF0 → [0xB3BF80]:=eax }`。ctor の vtable **0xA74C0C** = RTTI **`PathBuilder`**（4 スロット）→ **「PathBuilder シングルトン生成関数」** [確定]。読手（0xB3BF80 参照 34 site）: FUN_00462B20 / 0x463700 / 0x465130 / 0x5F13D0 / 0x643740 / 0x644E50 / 0x645280 / 0x646490 / 0x649340 / 0x64CD60 / 0x653520 / 0x657A80 / 0x63A020 / 0x63BA30 / 0x407AE0 等 = パス探索（A*）系。
- → v128 §8-5 の推定「アクター位置表 / ワールド初期走査」は**両方とも誤り**（RTTI 名で反証）[確定(反証)]。書込先グローバル（0xB3BE00 / 0xB3BF80）からの推定は不十分だった例。

### O. (v128 #6) gap 0x404940 の戻り値 — **「CD-ROM ドライブ存在」フラグ** [確定]

- ボディ全文デコード（0x404940..0x4049FF）: `xor bl,bl`(0x404966) → `GetLogicalDriveStringsA` → ループ `GetDriveTypeA==5(DRIVE_CDROM)` のとき `sprintf("%sOblivionLauncher.exe")` → `call 0x982D71(_access(path,0))` → **`_access` 成敗に関わらず `mov bl,1`(0x4049C5)**（成功時のみ `[0xB33394]:=ドライブ文字`）→ 末尾 **`0x4049EF: mov al,bl`**。
- → al = **「CD-ROM ドライブを 1 台でも検出できたか」**（v128 #6 の推定「前者」を確定）。`[0xB33394]` は「CDROM 上で launcher を実際に検出できたか」の別フラグであり、**al と [0xB33394] は別条件**（付録A-J の極性論証の前提）。付随: 0x982D71 は引数 (path, 0)・戻り 0=成功型で `_access` 推定と整合 [観測]（v128 #7 の同定は次節 P で裁定）。

### P. (v128 #7) 0x982D71 の同定 — **`_access`（CRT __access）** [確定]

- **台帳**: `cache/named_funcs.txt` = `00982d71,FID_conflict:__access`（Ghidra FID の関数名一致）。内部呼び出し先 0x982CEA = `FID_conflict:__waccess_s`（`_access` の下層）。
- **本体（0x982D71..0x982D84）**: `push [esp+8] / push [esp+8] / call 0x982CEA / pop ecx / neg eax / pop ecx / sbb eax,eax / ret` — inner の 0/非0 を **0/-1 に正規化** = `_access` の戻り規約（0=成功、-1=失敗）[確定(機序)]。
- **呼出し側（E8 = 14 件全数）**: **12 件**が直後に `cmp eax,-1; je/jne`（0x40E1B6 / 0x42E414 / 0x430A30 / 0x431857 / 0x431911 / 0x6AB2FB / 0x6ACED9 / 0x6AD1AB / 0x6AD491 / 0x6AD79B / 0x6ADB0A / 0x6ADCEF）、残 2 件も 0/非0 判定（0x4049B2 = `test eax,eax`、0x4300D6）。引数形はすべて cdecl 2 引数 `(path, mode)`（0x4049B2 = `(path,0)`）[観測]。
- **v128 #7 の唯一の根拠だった WinMain `0x40E1B6` の -1 比較を実測**: `call 0x982D71 / add esp,8 / cmp eax,-1 / jne 0x40E227` = 成功（-1 でない）で続行、-1 なら 0x40E1C3 のエラー文字列読み出し（0xA31254/58/5C）へ [確定]。
- → **0x982D71 = `_access` [確定]**。タスク残点10（0x404940 戻り値 ＋ 0x982D71）は**両項目とも裁定済み**（0x404940 は節 O）。

---
