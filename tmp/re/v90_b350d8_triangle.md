# V90: 0xB350D8 ↔ 0xB43104 ↔ 0xB42754 の三角関係復元（Oblivion.exe v1.2.0416 Steam）

- 作成日: 2026-10-25 / 手法: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050=518b4c24188b5424 / bytes@0x4FB580=578b7908 / string@0xA7035C=`.\AI\CombatController.cpp` / opid0x0015 impl=0x504560）。rd() はセクション対応（.text const=0x400C00 / .rdata const=0xA28000 / .data const=0xB02000、bss は None+警告）、dec_index=20,465、enclosing_fun() で VA→ラベル。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（pefile なし・struct 手動パース）、capstone 5.0.7（skipdata=True / md.detail=True）、decompiled は `docs/Oblivion_decompiled.c`（python enumerate 部分検索、ラベル小文字 hex）。
- 前提資料: `tmp/re/v80_partB_globals.md` の §6-1・§3（COM 単例族）・§4（B42754 証拠鎖）・§5（残不明点 1 = B350D8 writer）を全文通読。
- 中間成果物（すべて `%TEMP%\opencode` 配下・Git 管理外）: v90_scan.py / v90_writer.py / v90_callers.py / v90_cfg.py / v90_cfg2.py / v90_cfg3.py / v90_cfg4.py / v90_cfg5.py / v90_region.py / v90_order.py / v90_hdr.py / v90_hdr2.py / v90_boot.py / v90_dis1.py / v90_dis2.py / v90_mirror.py / v90_52f0.py / v90_pair.py / v90_final.py / v90_verify.py / v90_crt.py / v90_crt2.py / v90_crt3.py / v90_crt4.py / v90_str.py / v90_sum.py（+ *_out.txt）
- 本ノートは 1 ファイル新規作成のみ。docs/・既存 tmp/re・TODO_re49.md・session-memory.json・plan.md は無変更、git / memory / sync は未実施。

---

## §1 前提（v80 通読で把握した点と本タスクの手法前提）

### 1-1. v80 から引き継いだ確定事項
1. **COM 単例 4 本（0xB43104/108/10C/110）は独立 refcounted singleton**。v80 §3 で「1 オブジェクトのフィールド」説は（独立 setter 4 本・独立 release ヘルパ 4 本・refcount が各指し先・base+disp 相互参照 0 件・束で扱う読手のみ、の 5 根拠）で**棄却済み**。
2. **0xB43104 = レンダラ実体 ptr への mirror**。setter = FUN_00406950（値 = `DAT_00b350d8` のコピー + addref）、clear = ラベルなし関数 0x7B8530（shader 単例一括リセット）。
3. **0xB42754 = Gamebryo レンダラ実体 ptr [確定]**。唯一の setter = FUN_00772060（FUN_00778F60 ← FUN_0076A1C0 ← FUN_0076BD40 ← FUN_00498340 の連鎖）、clear = FUN_0076B5D0 → FUN_00779010 → FUN_00772060(0)。
4. **残不明点 1（v80 §5）= 0xB350D8 の writer 未同定**（imm32 33 site、0x4061C3/0x4104EA/0x41178D/0x4816E1 等、Ni システム初期化系に見える）→ 本タスクの対象。

### 1-2. 手法上の前提・制約（今回の検証で確定したバイアス）
1. **dec_index は `// FUN_xxx @ 0x` ヘッダのみ（20,465 件）**。一方 `// 名前付き @ 0x` ヘッダ（`_qsort` / `__vsnprintf_helper` / `thunk_*` / `CreateObject` 等）が **684 件**別途存在（全ヘッダ 21,149）〔**v90 時点の計数。訂正併記（v155 裁定・V175 適用）: 実数は名前付き 767 件 / 全ヘッダ 21,232 件** = v90 の identifier-only 正規表現が識別子外文字を含む 83 件（RTTI `` `name' `` 50 + `FID_conflict:*` 20 + デコレータ 13）を落としたため〕。enclosing_fun() は名前付きヘッダを無視するため、その境界近傍で直前 FUN に誤帰属する。
2. **Ghidra がデコンパイルしていない領域**（本タスクで 4 か所を確定、§2-4）も直前 FUN ヘッダに誤帰属する。v80 §1 の「帰属バイアス」の実例。本ノートでは **VA 直近ヘッダ（名前含む）+ 関数先頭からの線形逆アス**で宿主を補正した。
3. **B350D8 / B43104 / B42754 はいずれも .data の BSS**（.data VA=0xB02000 vsz=0xA9C1C / rsz=0x30C00 → raw は 0xB32C00 まで）。よって **rd() は 3 件とも返せず（None + 警告）、ファイルイメージ上の初期値は 0**。→ タスク指示どおり **rd 値比較は判定に使えない**（§4 は setter 経路のみで判定）。
4. .text = VA 0x401000 / raw 0x400 / 6,450,233 B / chars 0x60000020。全セクション（.text/.rdata/.data/.tls/.rsrc/.bind）で imm32 を走査した。

---

## §2 writer 追跡結果

### 2-1. 0xB350D8 の参照一覧（全セクション走査）

| セクション | imm32 出現数 |
|---|---|
| .text | **33**（= v80 の "imm32 33 site" と一致） |
| .rdata / .data / .tls / .rsrc / .bind | 0 / 0 / 0 / 0 / 0 |
| decompiled 出現（`b350d8` 小文字一致） | **33 行** |

宿主（名前付きヘッダ込みで補正、distinct = **18 関数** / 33 site）:

| host(VA補正後) | n | R/W | site |
|---|---|---|---|
| **0x4052F0（ラベルなし）** ※dec_index 誤帰属先は FUN_00405090 | — | — | (0x4052F0 は参照を持たない。B350D8 参照は 0 件) |
| FUN_00405ce0 | 1 | R | 0x4061C3 `cmp dword ptr [0xb350d8],0` |
| FUN_00406950 | 1 | R | 0x406A1F `mov ecx,[0xb350d8]`（mirror 元値読み） |
| FUN_004104c0 / 004105f0 / 004106c0 / 00410810 | 4 | R | 0x4104EA / 0x410622 / 0x41073F / 0x4108A7（`*(B350D8+0x280)` 系） |
| FUN_00411750 | 1 | R | 0x41178D ※窓デコード時の境界ずれで W 誤検出 → 関数先頭線形デコードで **R に訂正**（decompiled L10120 も読手） |
| FUN_00411b70 | 1 | R | 0x411B9F（vtable+0x8c 呼び出し） |
| FUN_004816e0 | 1 | R | 0x4816E1（`FUN_0047f340(param_1,B350D8,0)`） |
| FUN_004826f0 | 4 | R | 0x482819 / 0x4830DB / 0x4830ED / 0x4831AA（vtable+0x60/+0x68） |
| FUN_00497b20（文字列コピー helper） | 0 | — | （参照なし） |
| **0x497B50（ラベルなし＝レンダラ解放）** ※dec_index 誤帰属先は FUN_00497b20 | 3 | R+W | 0x497B5B / 0x497BA6 読み、**0x497BD4 `mov dword ptr [0xb350d8],0`（命令 10B, imm32 site 0x497BD6）= clear** |
| FUN_00497c30 / 00497d50 / 00497e70 / 00498060 | 7 | R | 0x497CE4 / 0x497D52 / 0x497E9A・0x497F1A・0x497F55・0x498011 / 0x498071 |
| **FUN_00498340** | 8 | R+W | 読み 7 件 + **0x498550 `mov dword ptr [0xb350d8],esi`（命令 6B, imm32 site 0x498552）= 唯一の set** |
| FUN_004ce460 | 1 | R | 0x4CE484 |
| FUN_00a19ef0 | 1 | R | 0xA19EF3（release ヘルパ: `InterlockedDecrement(B350D8+1)`） |

- **書込みは 2 箇所のみ**（set=0x498550 / clear=0x497BD4）。間接書込（`mov reg, 0xb350d8` → `[reg]` ストア、`push &DAT` 形）は 33 site 全数目視分類で **0 件**。
- decompiled での代入行は **L122004 `DAT_00b350d8 = piVar6;`（FUN_00498340）1 件のみ**（clear 側 0x497B50 はラベルなし関数のため decompiled に本文なし = 33 行中 1 件だけが代入、という v80 とは異なる今回の観測。残り 32 行はすべて読手）。

### 2-2. 生成元 FUN と型（タスク (a) の回答）

```
set 経路（E8 辺のみ・全辺 single-caller）
0x40DF80 WinMain [ラベルなし]  (ret 0x10 = __stdcall 4 引数)
  └ 0x40E865: mov ecx,[0xB33398] ; call 0x4052F0        ← 唯一の呼出し
       └ 0x4052F0 [ラベルなし]  (cookie prologue, thiscall)
            mov eax,ecx ; mov ecx,[eax+0xc] ; mov edx,[eax+8] ; push ecx ; push edx
            └ 0x40530E: call FUN_00498340               ← FUN_00498340 の唯一の呼出し
                 └ 0x498519: call FUN_0076bd40
                      └ FUN_0076bd40 = NiDX9Renderer::Create
                           ├ FUN_00401f00(0xB00)  … malloc(2816B)
                           ├ FUN_00769be0()        … ctor (mov ecx,eax / call)
                           └ 0x76BDD8 mov ecx,esi ; call FUN_0076a1c0 = NiDX9Renderer::Initialize
                 0x49851E: mov esi,eax
                 0x498550: mov [0xb350d8],esi ; 0x49855C-0x498566: InterlockedIncrement(esi+4)
```

- **生成元 FUN = FUN_00498340**（Win32 ウィンドウ生成 + レンダラ生成。文字列: `"Unknown error creating the Gamebryo Renderer."` / `"GetAdapterDesc() failed."` / `"NiXAdapaterDesc::GetDeviceCaps() failed."`）。
- **実体の型 = Gamebryo `NiDX9Renderer`（0xB00 バイト）**。根拠: FUN_0076BD40 に `"NiDX9Renderer::Create> Invalid device ID specified - FAILING\n"` / `"Creation failed: Could not initialize DirectX9"`、FUN_0076A1C0（=NiDX9Renderer::Initialize）に `"NiDX9Renderer::Initialize> Attempting to create rendering device"` ほか 14 種のエラー文字列、malloc サイズ 0xB00 がフィールド最大（+0xA58 使用を確認）と整合。
- **B350D8 は「WinMain 側のスマートポインタ」**: 解放関数 0x497B50 のログ文字列が `"There is 1 object with a smart pointer to the renderer we are trying to release."`（0xA3DD78）/ `"There are %d objects with smart pointers to the renderer we are trying to release."`（0xA3DD20）で、参照カウンタ `[B350D8+4]` を見て `cmp dword ptr [eax+4],1` した上で release + 0 化する。

### 2-3. clear / 関連 clear の所在

| 対象 | clear 命令 | 所属 | 呼出し元 |
|---|---|---|---|
| 0xB350D8 | 0x497BD4 `mov [0xb350d8],0` | **0x497B50（ラベルなし・レンダラ解放）** | 0x40F5CA（WinMain 末尾、`push 1; call 0x497b50`） |
| 0xB43104 | 0x7B88FB `mov [0xb43104],ebx` | **0x7B8530（ラベルなし・shader リセット、v80 既知）** | 0x497B56（同じく 0x497B50 内、`push ebx; call 0x7b8530`） |
| 0xB42754 | 0x772066 `mov [0xb42754],eax`（eax=0 で clear 兼 set） | FUN_00772060 | 0x779035 ← FUN_00779010 ← FUN_0076B5D0（0x76B7B3） |

つまり **3 本とも「WinMain 末尾の 0x497B50（ラベルなし）が総括して落とす」**構造のうち、B43104 は 0x7B8530 経由、B350D8 は直書き、B42754 はレンダラ本体の destroy 経由で落ちる（3 経路は別モジュール）。

### 2-4. 本タスクで新たに検出した「Ghidra ラベルなし」関数（4 + 1 件）

| VA | サイズ(概) | 役割 | dec_index の誤帰属先 | 検出根拠 |
|---|---|---|---|---|
| **0x4052F0** | 0x4052F0-0x40536F（0x4052E0-0x4052EF が int3 境界） | renderer 呼出し wrapper（this=[0xB33398]、引数 hwnd/hinstance） | FUN_00405090 | cookie prologue `sub esp,0x204` + FUN_00498340 呼出し |
| **0x40D800** | 0x40D800-0x40DF73 | FUN_0040D4D0 と FUN_00497E70 を呼ぶ中間初期化 | FUN_0040d4d0 | WinMain 0x40F270 から 1 件呼出、末尾 0x40DF54 で FUN_0040D4D0 へ |
| **0x40DF80** | 0x40DF80-0x40F639（`ret 0x10`） | **WinMain** | FUN_0040d4d0 | CRT 側 0x98780E の `push 0x400000/push 0/push eax/push ecx; call` |
| **0x497B50** | 0x497B50-0x497BE0 | レンダラ解放（B350D8 clear + 0x7B8530 呼出し） | FUN_00497b20 | int3 境界 0x497B43-0x497B4F |
| 0x98769C-0x98787B（+入口スタブ 0x98787C） | 約 0x1E0 | CRT startup（→ WinMain 呼出し） | `_qsort`/`__vsnprintf_helper` の間 | 0x98787C `call ___security_init_cookie` → 0x987881 `jmp 0x98769c` |

（v80 既知の 0x7B8530 / 0x50AA20 / 0x405150 に加える 5 件。ラベルなし関数の全数列挙タスク V85-V89 へ接続可能。）

---

## §3 初期化順序グラフ（テキスト）

### 3-1. 起動〜RT 群生成までの全体像

```
[PE AddressOfEntryPoint = 0xBAF2ED (.bind / chars=0x60000040 exec)]
   0xBB1204 から 0xD9 dwords をスタックへ複写 → バイトコピー/XOR ループ（複号系ブートストラップ）
   └─ CRT startup への接続は E8/E9/imm32/TLS コールバック/ポインタテーブルのいずれでも 0 件 = 動的（§5-1）

[CRT startup 領域（Ghidra ラベルなし）]
   0x98787C: call ___security_init_cookie (0x997BE3)
   0x987881: jmp  0x98769C                      ← この関数への入辺はこの E9 のみ
   0x98769C..0x98787B: GetStartupInfoA/GetCommandLineA/__getmainargs 系 CRT 初期化
   0x9877E1: call 0x981C6B（この領域からの唯一呼出し）
   0x98780E: call 0x40DF80                       ← WinMain(hInstance=0x400000, hPrev=0,
                                                   lpCmdLine=eax, nShowCmd=ecx(=10))

[0x40DF80 WinMain（ラベルなし）]   … CFG: 線形デコード 1,536 命令 / 到達 1,445 / 境界外転移 0
   │
   ├─① 0x40E865  mov ecx,[0xB33398] ; call 0x4052F0        ★ B350D8 生成
   │      └ 0x40530E call FUN_00498340 (hwnd=[this+8], hinst=[this+0xc])
   │            └ 0x498519 call FUN_0076bd40 = NiDX9Renderer::Create
   │                  ├ malloc(0xB00) → FUN_00769be0 (ctor)
   │                  ├ FUN_0076a1c0 = NiDX9Renderer::Initialize (this=esi)
   │                  │     └ 0x76A763 push ebp ; call FUN_00778f60
   │                  │           ├ FUN_0075fb30 / FUN_00772060(renderer)
   │                  │           │     └ 0x772066 mov [0xB42754],eax  ★ B42754 = renderer
   │                  │           │        （+0xB42750 ← *(renderer+0x280) addref,
   │                  │           │          +0xB42758 ← *(renderer+0x8AC)）
   │                  │           └ DAT_00b428a8 = shader program factory
   │                  └ return esi
   │            0x498550 mov [0xB350D8],esi + InterlockedIncrement(esi+4)  ★ B350D8 = renderer
   │
   ├─② 0x40EAAC  mov ecx,[0xB33398] ; call FUN_00406950     ★ mirror → RT 群
   │      ├ 0x406A1D mov ecx,[0xB350D8] ; mov eax,[0xB43104] ; cmp/je
   │      ├ 0x406A56 mov [0xB43104],edi  (=B350D8) ; 0x406A5E add edi,4 ; InterlockedIncrement  ★ mirror
   │      ├ 0x406A70-81 "data\shaders\" を 0xB42F68 へコピー
   │      ├ 0x406A83 call FUN_007ba0f0（shader 初期化）
   │      ├ 0x406A88-9A FUN_007b4270 ×2 / 0xB42EA7,0xB42E85 = 0 / 0xB42F40,0xB2C678 フラグ演算
   │      └ 0x406B22 mov edx,[0xB43104] ; push edx ; mov ecx,[0xB42F50] ; call FUN_007c2450
   │            └ ★ RT 群生成（FUN_007c1b50 を 1×1/1×1/1×1/4×4/16×16/64×64、D3DFMT 0x24/0x26、
   │               usage 0x71、さらに FUN_007c23c0 ×4、 FUN_007c1b50 を解像度依存で 2 本）
   │
   ├─③ 0x40F270  call 0x40D800（ラベルなし中間初期化）
   │      └ 0x40DF54 call FUN_0040D4D0 → 0x40D658 call FUN_0040C830（flag 0xB33395 = 1）
   │        0x40DF61 (flag 0xB34FA7) → call FUN_00497e70（B350D8 読手 / FUN_007ba2f0 = shader 再 init）
   │
   ├─④ 0x40F3EA  mov ecx,eax(vtable+0x170 of [0xB333C4]) ; call FUN_00405ce0
   │      └ `cmp byte [0xB33395],0 ; jne skip` でガード → ①で flag=1 済みのため再生成しない
   │        （FUN_00405ce0 → FUN_00405090 ×6 はウィンドウ寸法/アスペクト系で renderer 生成はしない）
   │
   └─⑤ 0x40F5C8  push 1 ; call 0x497B50（終了時: shader リセット + B350D8 release/clear）
         0x40F637  ret 0x10 … WinMain 末尾
```

### 3-2. 仮説「B350D8 生成 → B43104 へ mirror → FUN_007C2450 が RT 群生成」の検証結果

**判定: 成立 [確定]**（呼出しグラフ + CFG 支配関係で検証）。

検証に使った CFG（対象 = WinMain 0x40DF80..0x40F640、線形デコード + DFS、ジャンプ/フォールスルー先の境界欠落 **0 件**）:

| 命題 | 結果 |
|---|---|
| 0x40EAAC（mirror）へ到達可能か | True（entry→mirror のパス長 416） |
| 0x40F3EA（後段 create-guard）へ到達可能か | True（パス長 674、mirror は idx415） |
| **0x40E865（B350D8 生成呼出し）を避けながら mirror へ到達** | **False = 生成が mirror を支配** |
| **mirror を避けながら 0x40F3EA へ到達** | **False = mirror が後段 create-guard を支配** |
| 0x40E865 を避けながら 0x40F3EA へ到達 | False |

補助的な実行時制約（順序が逆なら破綻する箇所）:
- FUN_007C2450 は **B43104 の NULL ガードを持たない**（0x7C2594 `mov ecx,[0xb43104]; mov edx,[ecx]` = vtable 解参照が無条件、`DAT_00b43070/00b42f3e` のフラグは別物）。B43104 の writer は mirror(0x406A56) と clear(0x7B88FB) のみ → **初回 mirror 時に B350D8 ≠ 0 が必須**、これは「生成が mirror を支配」であることの実行時帰結と一致。
- FUN_00406950 内でも mirror(0x406A56) → FUN_007BA0F0(0x406A83) → FUN_007C2450(0x406B4E) の順で、RT 群生成呼び出しは無条件（Ghidra decompiled L5112-5150 とバイナリが一致）。

**v80 仮説からの訂正点（3 件）**:
1. **実行主体は WinMain（0x40DF80、Ghidra ラベルなし）**。mirror 呼出し 0x40EAAC は dec_index 上 FUN_0040d4d0 に見えるが、これは誤帰属（FUN_0040d4d0 の本体は 0x40D4D0-0x40D7FF で ret 終了、0x40EAA6 以降は別関数）。「初期化関数 = FUN_0040D4D0」という読みは訂正。
2. **初期化は明示 start 経路であって CRT 初期化テーブル由来ではない**。根拠: (a) 0x40DF80 / 0x40D800 / 0x4052F0 / 0x498340 / 0x406950 / 0x7C2450 / 0x7BA2F0 / 0x76BD40 / 0x772060 / 0x9870F8 の 10 VA を**全セクションで imm32 走査 → 0 件**（ポインタテーブル経由の登録が無い）、(b) 0xA2ED04 は `0xA10000, 0xA10020, 0xA10040…` と増加する .text ポインタ列（本チェーンは含まれない）、(c) TLS コールバック配列 0xA2F7AC の第 1 エントリが 0（コールバック無し）、(d) 唯一の経路は 0x98780E の直接 E8 で WinMain を呼ぶ CRT startup。
3. **FUN_007C2450 の呼出しは 2 箇所のみ**: FUN_00406950(0x406B4E) と FUN_007BA2F0(0x7BA3DD)。後者は `DAT_00b42f31` ガード付きの shader 再初期化経路で、FUN_007B84E0（teardown）→ FUN_007C1FF0（this=0xB42F50 を作り直し）→ FUN_007C2450 の順。初回経路は前者のみ。

---

## §4 B350D8 vs B42754 の同一性判定

### 判定: **[確定] 同一オブジェクト**（setter 経路で判定、rd 値比較は不使用）

B350D8 / B42754 はいずれも .data BSS（rd 不可・イメージ上 0）のため、タスク指示どおり **値比較ではなく「どちらの writer が同じ格納先・同じ構築関数を呼ぶか」**で判定した。

**set 経路の合流点（同一 call チェーン・同一レジスタ）**:

```
FUN_0076bd40 (NiDX9Renderer::Create)  … 呼出し元 = 0x498519 の 1 件のみ
   esi = FUN_00401f00(0xB00) で確保したメモリ → FUN_00769be0() (ctor, ecx=buf)
   ├ (経路 A: チェーン内部) 0x76BDD8: mov ecx,esi → FUN_0076a1c0 (NiDX9Renderer::Initialize)
   │      0x76A1EB: mov ebp,ecx  … ebp = this = esi であることを逆アスで確認
   │      0x76A763: push ebp → FUN_00778f60(renderer)        … FUN_00778f60 の唯一の呼出し
   │            0x778F73: call FUN_00772060(param_1=ebp)      … FUN_00772060 の set 経路
   │                  0x772066: mov [0xB42754],eax (eax=param_1=ebp)  ★ B42754 = esi
   └ (経路 B: チェーン帰直後) FUN_00498340:
          0x49851E: mov esi,eax (= FUN_0076bd40 の戻り値)
          0x498550: mov [0xB350D8],esi (+ InterlockedIncrement)      ★ B350D8 = esi
```

**裏付け（4 点）**:
1. **逆経路の混入が無い**: FUN_0076BD40 = 呼出し 1 件 / FUN_00778F60 = 1 件 / FUN_00772060 = set(0x778F73) と clear(0x779035←FUN_00779010←FUN_0076B5D0) の 2 件のみ / FUN_00769BE0 = 1 件。→ B42754 に入り得る値はこのチェーンの esi 以外に存在しない。
2. **同一性チェック付き refcounted 代入**: B350D8 の set 前に `mov eax,[0xB350D8]; cmp eax,esi; je skip`（0x498520-0x49852A）＝ old==new なら何もしない。B42754 側も FUN_00772060 が `if (DAT_00b42754 != param_1)` 相当の流儀（旧値 release + キャッシュ更新）。
3. **両者の読手が同じフィールド +0x280 を解参照**: B350D8 = `**(int**)(DAT_00b350d8 + 0x280)`（L121561）/ `DAT_00b350d8[0xa0]`（L121963、ポインタ型のため 0xA0×4=0x280）; B42754 = `*(param_1 + 0x280)`（FUN_00772060 内、→ 0xB42750 へキャッシュ）。0xB00 サイズの同一インスタンスで矛盾しない。
4. **解放時に両方が同時に落ちない点も整合**: B350D8 は 0x497B50（WinMain 末尾 0x40F5C8 から）で release+0 化、B42754 はレンダラ本体 destroy（FUN_0076B5D0→FUN_00779010）で 0 化。モジュールは別だが対象は 1 体。

**三角の関係（確定した構造）**:

```
            NiDX9Renderer 実体（1 オブジェクト, malloc 0xB00, ctor FUN_00769BE0）
                       │
   ┌───────────────────┼─────────────────────┐
   │ ①オーナー所有       │ ②レンダラ内部キャッシュ   │ ③shader サブシステム所有
   │ 0xB350D8           │ 0xB42754              │ 0xB43104
   │ = WinMain 側 smart │ = B42750(+0x280 子)・ │ = B43104/108/10C/110
   │   pointer          │   B42758(+0x8AC) と   │   COM 単例族の 1 席
   │ set 0x498550       │   一緒に 0x772066      │ set 0x406A56 (mirror+addref)
   │ clear 0x497BD4     │ clear 0x772066(=0)    │ clear 0x7B88FB (0x7B8530)
   │ 読手 18 関数        │ 読手 4 関数            │ 読手 74 関数
   └───────────────────┴─────────────────────┘
```
- **mirror = ①→③ の 1 回きりコピー**（FUN_00406950 内、0x406A56、InterlockedIncrement 付き）。B350D8 と B43104 を同時に参照する関数は **FUN_00406950 の 1 本のみ**（decompiled で 2 つのグローバルを含む行は L5115 `if (DAT_00b43104 != DAT_00b350d8)` の 1 行だけ）。
- ②は構築中に FUN_00772060 が B42750/B42758 と共に置く「描画ステート系キャッシュ」、③は 0x498550 の 120 行後（WinMain 内 0x40EAAC、+0x9E1 バイト）に作られる。

### (d) B43104 に mirror する意味論（なぜ 2 本必要か）

1. **所有権の分離**: ①は「WinMain が renderer を生成・解放する側の所有権」（clear 時に `smart pointer to the renderer we are trying to release` とログ）、③は「shader/RT サブシステムが保持する所有権」。③は addref 付き NiPointer で、①側が勝手に落とさない（実際に clear は 0x7B8530 = shader リセット側で行う）。
2. **読手集合が重ならない**: ①の読手はウィンドウ/レンダラ生命周期群（FUN_00405CE0/004104C0系/00411750/00411B70/004816E0/004826F0/00497C30系/004CE460/0x497B50/0x406950）、③の読手は shader/RT/effect 群（FUN_007C2450 13 site、FUN_007BA0F0/007BA2F0、FUN_007B8990、FUN_008014E0=Shader_CompileVertex、FUN_00801C90=Shader_CompilePixel、FUN_007BDFC0、FUN_007FC7D0、FUN_004D0190 等 74 関数）。**互いのモジュールを include せずに参照できるよう、鏡像を shader 初期化時に 1 回だけ作る**のが FUN_00406950 の設計。
3. **RT 群生成が ③ に依存**: FUN_007C2450 は B43104 しか読まない（this=0xB42F50 の中身も使わない）。③が無ければ D3DFMT 0x24/0x26・usage 0x71 のレンダーターゲット群（1×1×3 / 4×4 / 16×16 / 64×64 / 解像度依存 2 本）を作れない。
4. **寿命の独立**: renderer 再生成・shader リセット時に 0x7B8530 が ③（と B430DC/B430F0/B43110/B42EB8）だけ落とせる。①は WinMain 末尾の 0x497B50 で落ちる。3 本は「同じ指す先」だが「落ちる経路・時期」が別 → 1 本にまとめられない。

---

## §5 残点

1. **PE entry 0xBAF2ED と CRT startup の接続（未解析）**: entry（.bind / exec）は 0xBB1204 から 0xD9 dwords をスタックへ複写し、バイトコピー・XOR（鍵 0x72c99b67 起点）を行う複号系ブートストラップ。CRT startup（0x98787C/0x98769C）への参照は E8/E9（全セクション）・imm32・TLS コールバック（0xA2F7AC 先頭 0）・ポインタテーブルのいずれでも 0 件 → **動的接続**。複号ループの先を追う必要がある。
2. **CRT startup 関数の厳密な開始点と名称**: 0x98769C は prologue 無しで `[ebp-4]`（SEH scope）を書くのに、入辺は 0x987881 の E9 のみ。0x987699 に 3 バイトの `xor eax,eax; ret` が割り込み、`55 8B EC` は 0x98717A/0x987886 にしか無い。→ prologue 所在（別 chunk / 分割関数）が未確定。名称は CRT 文字列（`GetStartupInfoA`/`__getmainargs` 系の間接呼出し、直前の `call 0x981C6B`）から `WinMainCRTStartup`/`__tmainCRTStartup` と推定（[仮説]）。
3. **ラベルなし関数の増加**: 今回 4 + 1 件（0x4052F0 / 0x40D800 / 0x40DF80 / 0x497B50 / 0x98769C-0x98787B）を検出。V85-V89 の「ret+int3 パディング境界で dec_index 外の独立関数を全 .text 抽出」の走査に、**名前付きヘッダ 684 件（v90 時点の計数。v155 裁定の実数 = 767 件、V175 併記）を除いた「デコンパイル自体が無い領域」**という新カテゴリを追加する必要がある（現行の ret+int3 規則では 0x40DF80 は検出されるが 0x4052F0 は int3 が 16B あるため検出可能・0x98769C は prologue 無しで検出困難）。
4. **renderer 再生成時の B43104 再 mirror 経路**: FUN_00406950 の呼出し元は WinMain 0x40EAAC の 1 件のみ。一方 B43104 は 0x7B8530（FUN_00497B50 → WinMain 0x40F5C8、および FUN_00497E70/FUN_004980D0 経由の shader リセット）で 0 化されうる。**再 mirror するコードが見当たらず**（B350D8/B42754 は再生成で上書きされるが B43104 は無視される？）、`FUN_007BA2F0` 経路が本当に初回以外で使われるのか、フラグ `0xB42F31` の遷移と合わせて要検証。
5. **FUN_00405CE0 → FUN_00405090 の意味論**: 6 分岐が渡す float 定数（0xA2FE7C/0xA2FE78/0xA7DEB4 等）と内部の FUN_0043FD20 / FUN_004D1A90 / FUN_0043FC20(1) の役割（ウィンドウ寸法・アスペクト比の再適用？）未解釈。レンダラ生成はしない（0x498340 の呼出しは 0x40530E のみ）ことは確定。
6. **B350D8 読手の個別意味論**: FUN_00411750 / FUN_00410810 / FUN_004CE460 / FUN_00A19EF0（release ヘルパの到達経路: E8 0 件）の分類が未了。
7. **WinMain 後半（0x40EB00-0x40F3EA）の呼び出し内容**: FUN_0067FCF0 / FUN_00683030 / FUN_004431F0 / FUN_00447300 / FUN_004D5D70 / FUN_005E02E0 / FUN_00663070 / FUN_00442630 / FUN_004533F0 / FUN_0045A530 / FUN_00464440 系（レンダラ生成後〜RT 群生成後の後続初期化）は未解析。ここの解読で「RT 群生成以降の起動順序」が埋まる。
8. **FUN_0076BED0（→ FUN_0076B5D0 レンダラ破棄）は E8 呼出し 0 件** = vtable/間接経由と推定。B42754 clear の実行タイミング（プロセス終了のみか、設定変更時もか）未確定。

---

検証: `python "%TEMP%\opencode\probe_harness.py" selftest` → 4/4 PASS（本ノート作成前）。バイナリ・decompiled.c・既存 docs/tmp は無変更。作成ファイルは本ノート 1 件のみ（スクリプトは `%TEMP%\opencode\v90_*.py`）。
