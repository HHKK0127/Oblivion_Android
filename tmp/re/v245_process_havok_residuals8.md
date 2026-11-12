# v245_process_havok_residuals8.md — process/Havok 残点第8束（V-238-8）

- 日付: 2026-11-02 / 作成: opencode サブエージェント（TODO serial 238 / V238・低）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v243_process_havok_residuals7.md` — §8 新規残点6件＋訂正記録（v240 本体は無編集）
- 手法: capstone 5.0.7（`v172_common.py` / `v217_util.py` 再利用）＋ `tmp/re/cache/{named_funcs,vtable_index,call_targets}`（参照のみ）
- 中断対策: **前回実行分（`v245_1.py`〜`v245_5e.py` 19本と各 `_out.txt`）を全て読み取り、続きから検証**。本バッチは `%TEMP%\opencode\v245_6.py`〜`v245_8b.py`（6本・各 `_out.txt` 付き）を追加。
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v245_*` のみ。ESM 読取 0 回・ソース編集 0 回
- selftest: `probe_harness.py selftest` **4/4 PASS**（本セッション着手時）
- **VA 換算**: `imm32_refs3()`（セクション個別 raw）・`e8_callers()`（raw = VA-0x400C00）で v234/v240 罠注記遵守
- 外部参照（Web: 検索8回・有効ソース5件）: ①hkxcmd `Core/hkclsnam.cpp`（Havok SDK クラス名→ヘッダパス表）②libSkyrim `RTTI.h`（`hkBinaryPackfileWriter`/`hkPackfileWriter` 型名）③ZeldaMods(BotW) Havok 仕様（binary packfile セクション名）④reshax/lukascone（XML packfile = `hkpackfile`/`hksection`）⑤nitaigao engine-showcase `hkBinaryPackfileReader.h`（Havok 2009 BUILD#20090216 実ヘッダ）。`'snap'`/`__classindex__`/`__dataindex__` は**全 Web 検索で確認できず（負証拠）**。

---

## §1 item1（V-238-8-1）fn@`0x40DF80` の命名確定と sole caller `0x98780E` の境界 — **解明: fn@0x40DF80 = `WinMain`（起動オーケストレータ）、caller = CRT `WinMainCRTStartup` 本体 fn@0x98769C..0x98787B**

### 証拠A: fn@`0x40DF80` = WinMain（4引数 stdcall・CRT からの直接 call）（`v245_1_out.txt` / `v245_6_out.txt`）

1. **sole caller**: `e8(0x40DF80)` = **`0x98780E` の1件のみ**、imm32 参照 0（`v245_6_out.txt` (b)）。
2. **呼び出し形**: `push ecx; push eax; push 0; push 0x400000; call 0x40DF80`（`v245_1c_out.txt` / `v245_6_out.txt` (g)）。ecx = **`SW_SHOWDEFAULT`(10)** または `STARTUPINFO.wShowWindow`（`test [ebp-0x44],bl / movzx ecx,word [ebp-0x40] / push 0xa; pop ecx`）＝**nShowCmd**、eax = `call 0x997725` の戻り値（GetCommandLineA 経由の `lpCmdLine`、`[0xBABC04]` 保持）、0 = **hPrevInstance**、`0x400000` = **hInstance**。
3. **戻り cleaning**: fn@0x40DF80 の最終 ret は **`ret 0x10` @0x40F637**（`v245_6_out.txt` (a)）＝**stdcall 4引数（WINAPI WinMain 型）**。`xor eax,eax`（return 0 = EXIT_SUCCESS）付き。
4. **冒頭で引数1を読む**: `[esp+0x6B0]` = entry 直後 esp+4 = arg1（hInstance）を ebx へ（`v245_1_out.txt` (1b)）。
5. **ini 読取**: 冒頭 `sprintf('.\%s')`（ini パス構築）→ **`GetPrivateProfileIntA('General', 'bUseMyGamesDirectory', 1, path)`**（IAT `0xA280E4` = KERNEL32!GetPrivateProfileIntA、`v245_1e_out.txt`）。引数1（hInstance `0x400000`）は prologue `mov ebx,[esp+0x6B0]` で保持。
6. **関数内容**: boot 文字列群（`'Loading initial area...'`/`'Initializing Renderer...'`/Gamebryo メモリプール25種/`CopyProtectionStrings`/`'Could not find starting cell...'`）を参照、直接 call 197件、Havok 初期化チェーン先頭（`0x40EB2C: call FUN_00406360`）を内包（`v245_1e_out.txt` (c)(d)、v243 §6(c) 継続）。
7. **境界**: `ret 0x10 @0x40F637` → int3 pad 6B（0x40F63A..0x40F63F）→ **FUN_0040f640 @0x40F640**（e8 callers: `0x40FF0C`/`0x40FF29`＝FUN_0040fee0 経由、独立関数として機械確定）。

### 証拠B: sole caller `0x98780E` の境界 = CRT `WinMainCRTStartup` 本体 fn@`0x98769C..0x98787B`（`v245_1b/1c_out.txt` / `v245_6_out.txt` / `v245_6b_out.txt`）

- named_funcs は `0x98780E` を `_qsort+0x47E` と誤括り。実際の分割:
  - `_qsort` = `0x987390..0x987636`（ret 群 0x9873D3/0x987410/0x987636、epilogue `add esp,0x100; ret`）
  - 補助1 = `0x987637..0x98765A`（`push 0xff; call 0x981B99(ExitProcess系)`＝`_NMSG_WRITE` 型、e8: 0x9876DC/0x987779/0x98778A）
  - 補助2 = `0x98765B..0x98769B`（`cmp word [0x400000],0x5A4D` MZ/PE/PE32/`NumberOfRvaAndSizes>0xE`/DLL フラグ検査＝CRT の PE 画像検証）
  - **本体 = `0x98769C..0x98787B`**: `push 0x60(frame); push 0xAFFAD8(scope table); call 0x98C5A0`＝**MSVC `__SEH_prolog` フレーム**（0x98C5A0 は `fs:[0]` チェーン構築＋`sub esp,eax`＋cookie、対の epilog `0x98C5E5` が `0x987876` から call され `ret @0x98787B`）。中身 = `GetStartupInfoA`→`HeapAlloc(0x94)`→`GetVersionExA`→`__setargv(0x9979F5/0x997AAE)`→`GetCommandLineA(0x9877A9)`→**WinMain 呼出し（0x98780E）**。
  - 末尾トランポリン = `0x98787C..0x987885`: `call 0x997BE3; jmp 0x98769C`。**0x997BE3 = `__security_init_cookie`**（`[0xB30AAC]` 既定値 `0xBB40E64E` 比較＋`QueryPerformanceCounter/GetCurrentProcessId/GetCurrentThreadId` 系 IAT を xor — `v245_6b_out.txt` (c)(e)）。
  - `e8(0x98769C)` = **NONE** ＝ OS/CRT 経由の到達（下記 OEP 参照）。
- **PE OEP = `0xBAF2ED`**（AddressOfEntryPoint RVA 0x7AF2ED、`v245_6_out.txt` (c)）。OEP 帯は `[ebp-0x58]` 等の間接 call を45件持つ大関数（Steam 保護/初期化スタブ型、`entry+0x0`）で、CRT startup への**直接 E8 は無い**（到達は0x98787C トランポリン経由の間接接続と推定）。

### 裁定

> - **fn@`0x40DF80` = `WinMain`（＝ini パス構築・コピープロテクション・メモリプール生成・初期エリア読込・Sky/Havok 初期化を内包する起動オーケストレータ）で確定**（確度: 高）。引数4・stdcall・nShowCmd・CRT 呼出位置の4点が収束。v243 の「boot sequence 本体（確度: 中〜高）」を名称付きで確定。
> - **sole caller `0x98780E` の境界 = CRT `WinMainCRTStartup` 本体 fn@`0x98769C..0x98787B`**（+補助2関数+トランポリン+`__security_init_cookie`）。named_funcs の `_qsort+0x47E` は境界未分割による誤括り。
> - OEP `0xBAF2ED` は保護/初期化スタブ帯（未解明: 0x98787C への間接接続の詳細）。

**判定: 解明（命名+境界）**（証拠 `v245_1_out.txt` / `v245_1b_out.txt` / `v245_1c_out.txt` / `v245_1e_out.txt` / `v245_6_out.txt` / `v245_6b_out.txt`）。

---

## §2 item2（V-238-8-2）表 `0xA3D79C..0xA3D7C0` の所有者 — **解明: これは登録表ではなく `GridCellArray` の vtable。所有者クラス＝`GridCellArray`（.?AVGridCellArray@@）**

### 証拠A: RTTI（COL）決定（`v245_2d_out.txt`）

- `vtable[-4]` = `0xA3D798` = **COL `0xAB8D0C`** → PTD `0xB06A54` → 型名 **`.?AVGridCellArray@@`**（vtable_index.tsv **行741**・`0x00A3D79C`・10 slots）。
- base vtable `0xA3D718` = **`GridArray`**（COL `0xAB8C6C`、行740）、弟 vtable `0xA3D7E0` = **`GridDistantArray`**（COL `0xAB8D58`、行742）。

### 証拠B: vtable 構成と ctor/dtor（`v245_2_out.txt` / `v245_2b_out.txt` / `v245_2c_out.txt`）

| slot | +off | GridCellArray 実体 | GridArray(base) |
|---|---|---|---|
| 0 | 0xA3D79C | 0x483570（scalar deleting dtor） | 0x482030 |
| 1 | +4 | 0x482080（vector dtor thunk） | 0x481E00 |
| 2 | +8 | 0x481E20 | 0x481E20 |
| 3 | +0xC | 0x4821E0 | 0x481E10 |
| **4** | **+0x10** | **0x482090（＝fn@0x482090、セル移動→原点同期）** | 0x482050 |
| 5..9 | +0x14.. | 0x481E60 / 0x482200 / 0x482240 / 0x482260 / 0x4822A0 | 0x481E60/… |

- vtable 代入者: **ctor `FUN_00483350`**（`call 0x481de0`（base ctor＝`[ecx]=0xA3D718`）→ `[esi]=0xA3D79C`。**`[0xB06A2C]` を読み min5 でスナップ、N×N の8バイト配列（`[esi+0xC]=N²`, `[esi+0x10]=配列`, `[esi+0x14..0x1C]=(0,0,0)` float）を確保**）／**dtor `FUN_00483460`**（N×N 全要素の `[entry+4]` を `FUN_0049E500`＋`free`）／scalar dtor `FUN_00483570`（e8: `0x483460` の sole 呼出し元）。
- **生成・保持**: `e8(0x483350)` = **`0x441AAA`（FUN_00441970+0x13A）1件のみ**。FUN_00441970 は `mov [esi],0xa37470` = **`TES`（.?AVTES@@、vtable_index 行1252）** のメソッドで、生成結果を **`[esi+8]`（TES+8）** に保持（`v245_2c_out.txt` (c)(e) / `v245_2d_out.txt` (c)）。
- `[0xB06A28]`（グリッドアクティブ化フラグ）は `FUN_00441970` 帯ほか8箇所、`[0xB06A2C]`（辺長）は**100箇所超**のグリッド/セル管理関数群（FUN_0043F560..FUN_00444840 帯＝TES4 セルグリッド管理）から参照（`v245_2c_out.txt` (d)）。
- 隣接文字列 `0xA3D7C4` = **`'uGridsToLoad:General'`**（ini キー、.data `0xB06A30` 経由の参照は `FUN_00a186d0+0xE0` の1件のみ＝同関数が ini 読取に使うと推定）＝**vtable とは無関係の rdata 隣接データ**（v243 の推定材料だった「'uGridToLoad:General' 隣接」は実際は `uGridsToLoad` で、WinMain 側の ini 読取（'General'/'bUseMyGamesDirectory'）とは別物）。

### 裁定

> - **表 `0xA3D79C..0xA3D7C0` = `GridCellArray`（Gamebryo/TES セルグリッド管理クラス）の vtable**。所有者＝`GridCellArray` で確定（RTTI COL＋ctor/dtor＋生成元 TES の3方向）。
> - fn@`0x482090` = **GridCellArray の virtual slot4**（cell 座標差分検知→`(x<<12,y<<12,0)` 構築→slot5 `0x481E60` 通知→bhkWorldM 原点同期、v243 §6(a) と整合）。上流 observer クラス命名はこれで了。
> - v243 の「ini/グリッド設定通知系コールバック表」推定は**実質妥当**（GridCellArray＝`uGridsToLoad` サイズのグリッド管理）だが、表の本体は vtable（コールバック登録表ではない）。

**判定: 解明**（証拠 `v245_2_out.txt` / `v245_2b_out.txt` / `v245_2c_out.txt` / `v245_2d_out.txt`、cache `vtable_index.tsv` 行740-742/1252 参照のみ）。

---

## §3 item3（V-238-8-3）0x6C packfile 3クラスの SDK 正式名と `'snap'` タグの語義 — **判定: ほぼ解明（A=XML writer/B=binary writer 系と SDK 実在を Web 裏付け。ただし B の正式名と 'snap' の上流出所は未解明）**

### 証拠A: RTTI 不在の再確認（`v245_3_out.txt` (a)）

- `0xAA21EC-4` = 0、`0xA9C8B4-4` = `0x657061`（文字列残骸）、`0xA9C990-4` = `0x5F5F`（`'__'`）＝**COL ではない**。vtable_index.tsv 行なし（v243 §5 継続）。→ 機械的型名確定は不可。

### 証拠B: 上流クラス名の Web 裏付け（2009 版 SDK 実在）

- hkxcmd `Core/hkclsnam.cpp`（Havok SDK クラス→ヘッダパスの公式マップ）に **`hkPackfileWriter` = `Common/Serialize/Packfile/hkPackfileWriter.h`**、**`hkBinaryPackfileWriter` = `Common/Serialize/Packfile/Binary/hkBinaryPackfileWriter.h`**、**`hkXmlPackfileWriter` = `Common/Compat/Deprecated/Packfile/Xml/hkXmlPackfileWriter.h`**、`hkPackfileHeader`/`hkPackfileSectionHeader`（＝v240/v243 の descriptor 名）が実在。
- libSkyrim `RTTI.h` に `hkPackfileWriter`/`hkBinaryPackfileWriter`/`hkBinaryTagfileReader` の型名実在（他エンジンでも同名が RTTI に残る）。
- 2009 BUILD#20090216 の実ヘッダ（engine-showcase `hkBinaryPackfileReader.h`）: reader 側の API（`loadEntireFile`/`getContentsWithRegistry(expectedClassName, finish)` 等）— **`getContentsWithRegistry(const char* className, ...)` の2引数形は binary 版 `FUN_0090EF80`（ret 8）のシグネチャと整合**。
- セクション名の上流知識: binary packfile の標準セクション = **`__classnames__` / `__types__` / `__data__`**（ZeldaMods BotW 仕様・lukascone・reshax・allegro の4系統）。XML packfile = `<hkpackfile>`→`<hksection name="...">` 形式（reshax）＝**A が登録する `__types__` と XML 出力と整合**。

### 証拠C: クラス別挙動（`v243_5b_out.txt` / `v245_3_out.txt` / `v245_3b_out.txt` / `v245_8_out.txt`）

- **A `0xA9C8B4`**: ctor `FUN_0090D430` がセクション名 `__types__` 追加、slot4 = `FUN_0090D4D0` = **XML 出力本体**（`sprintf("<hksection name=\"%s\">\n")` → `</hksection>`→`</hkpackfile>` → 書出）⇒ **`hkXmlPackfileWriter` 系（確度: 中〜高）**。
- **B `0xA9C990`**: ctor `FUN_0090D7D0` が **`__classnames__`/`__classindex__`/`__dataindex__`** の3セクション名を追加、slot4 = `FUN_0090EF80`（`ret 8`、0=成功。内部で `0x90d960/0x90d9c0/0x90da40/0x90dc60/0x90e860` の直列化ヘルパ群＋`call [eax+0x1c]` の型情報 vcall）⇒ **binary packfile 系 writer（`hkBinaryPackfileWriter` 同族、確度: 中）**。
  - **ただし**: 標準 binary セクションは `__classnames__/__types__/__data__` で、**`__classindex__`/`__dataindex__` は公開仕様4系統のどれにも出現せず**（Web 負証拠）→ B の SDK 正式名は機械確定不可。
- **base `0xAA21EC`**: slot4 = `0x982D85`（CRT `_access` 相当＝ファイル存在/可否確認）、dtor `FUN_00940E30` がセクション配列＋`+0x2C/+0x38` map を解放 ⇒ **`hkPackfileWriter`（抽象基底、確度: 中〜高）**。
- wrapper `FUN_008BC540` の slot4 呼出し契約: `neg ebx; sbb bl,bl; inc bl` で **eax==0 → 成功(true)**（base slot4 = `_access` の 0=成功と同型）。

### 証拠D: `'snap'` タグ（`v245_3_out.txt` (c) / `v245_8_out.txt` / `v245_8b_out.txt`）

- 4バイトを**バイト逐次構築**（`mov [esp+0x38],0x73 / +0x39,0x6E / +0x3A,0x61 / +0x3B,0x70`）→ dword 化して構造体 **`{+0='snap', +4=ラベル文字列 ptr（0x90bba0 で構築）, +8=1}`** の先頭として **slot4 へ渡す**（`FUN_008BC540`）。arg1 = 型ディスクリプタ（`FUN_008BC660` が `0xBA8320='hkRootLevelContainer'` と `0xBA82D8='hkPhysicsData'` を供給）。
- contents 構築部（`FUN_008BC660` 帯 @`0x8BC881/0x8BC8AA/0x8BC8D3`）が文字列 **`'Shape'` / `'CollAgtPtr'` / `'Properties'`** を参照（`v245_8b_out.txt` (b)）＝**物理ワールドのオブジェクト（Shape・衝突エージェントポインタ・プロパティ）を NamedVariant 的に詰めるスナップショット contents**。
- 上流: `FUN_00889D60`（SEH 保護・`'Running Cell Test'` 帯）→ `FUN_008BC6C0` → `FUN_008BC660` → `FUN_008BC540`（v243 §5C 継続）。
- **全 Web 検索（8回）で `'snap'` を Havok SDK のタグ/オプションとして確認できず** → ゲーム/Bethesda 側で構築される4バイトタグ。

### 裁定

> - **A `0xA9C8B4` = `hkXmlPackfileWriter` 系（確度: 中〜高）／B `0xA9C990` = `hkBinaryPackfileWriter` 系（確度: 中）／base `0xAA21EC` = `hkPackfileWriter`（確度: 中〜高）** — クラス名は SDK 実在＋挙動一致で裏付け。RTTI 不在のため機械的確定は不可。
> - **`'snap'` = 物理ワールド内容（hkRootLevelContainer + hkPhysicsData、内訳 'Shape'/'CollAgtPtr'/'Properties'）を slot4 に渡す4バイト・スナップショット・タグ**（"snapshot" の略と推定、確度: 中）。上流出所は未解明（負証拠: Web 検索8回で SDK 内に同タグなし）。
> - 未解明要素: B の正確な SDK クラス名（`__classindex__`/`__dataindex__` の出所）、slot4 の内部が書込系か読込系かの最終判別（A は書込確定、B は直列化ヘルパ群のみ）。

**判定: 解明（機能+SDK 名推定）＋うち2点は未解明**（証拠 `v245_3_out.txt` / `v245_3b_out.txt` / `v245_3c_out.txt` / `v245_8_out.txt` / `v245_8b_out.txt`、v243 §5 継続、Web 検索8回・有効ソース5件）。

---

## §4 item4（V-238-8-4）descriptor `+0xC` の live 読手最終機械走査 — **解明: write-only を機械確定（live 読手 0 件）**

### 証拠A: D-set の完全抽出（`v245_7_out.txt` (0)）

- `e8(0x90D190)` = **150件**、各 call 直前の `B9 imm32`（`mov ecx, D`）が **150/150 一致** → **D-set = 150個の hkClass デスクリプタ実体アドレス**（`0xBA81D0`, `0xBA81F4`, `0xBA8218`, … `0xBA9118` 等）を全数確定。
- v245_4/4b の「(0) D-set size: 0」は抽出バグ（本バッチで修正）→ **同出力の A/B「hits: 0」は D-set 空だったため無意味**と判明。**本走査で全経路を実測し直した。**

### 証拠B: 5系統の機械走査すべて 0（`v245_7_out.txt` / `v245_7b_out.txt`）

| 経路 | 内容 | 結果 |
|---|---|---|
| A | `mov r, D` → 40 insn 内 `[r+0xC]`（this直渡し） | **0件**（push-D 115件は B へ） |
| A2 | 絶対アドレス形 **`[D+0xC]`** を含む .text 指令 | **0件** |
| B | `push D` → 直近 call の callee 内 `[esp+0x8..0x18]` → `[reg+0xC]`（引数経由） | **0件** |
| C/A3 | **list2 配列 `0xA9AA48`（94件・唯一の D 連続配列）** → .text 参照 → 要素 `[reg+0xC]` | **0件**（list2 の .text 参照は `0x9430B2/0x9430B9/0x94314C`＝FUN_00943070 ctor のみ） |
| D | Havok 帯 getter `FUN_0088D370`（31 caller）/ `FUN_00943410`（12 caller）の ecx 起源が D か | **0件**（43 caller 全数） |

### 証拠C: 安全網（F）＝D 含有関数の `[reg+0xC]` 全数分類（`v245_7_out.txt` (F)）

- `.text` 内の D imm 出現 = **292件**、それを含む関数 = **22関数**（FUN_008BC370/660、FUN_008CABF0、FUN_0090DA40、FUN_0090E860、FUN_00940D90、FUN_00940F60、FUN_00941070、FUN_009411E0、FUN_00942170、FUN_00942CB0、FUN_009461B0、FUN_00946880/D90/FB0、FUN_00953E10、FUN_009554E0、FUN_00A131A0/240/B00/DC0（thunk 帯）、FUN_00A15870 等）。
- それらの `[reg+0xC]` 読みは**全て** `[esp+0xC]`（stack）または `esi/ebp/eax/edx` 等の**別構造体**（serializer・array・variant 系）で、**D 由来のレジスタには一切到達しない**（v243 §1 が既知消費17系で判別した「stack/別構造体の誤検出」を全数で再確認）。
- thunk 本体（`mov ecx,D; call 0x90D190`）の `ecx` 追跡は callee 側で `+0xC` アクセス 0（ctor は `mov [eax+0xC],edx`＝**writer**）。

### 裁定

> - **descriptor `+0xC`（`m_numImplementedInterfaces`）の live 読手は 0 件＝write-only で機械確定**（確度: 高）。走査は D-set 全150件 × 直渡し/引数/絶対アドレス/配列(list2)/getter 起源の5経路＋関数内共存の安全網292出現で完封。
> - 残る理論経路は **`FUN_009546D0`（vtable→descriptor マップ）の値経由**（動的・imm/配列走査で検出不能）のみ。ただし値分布 {0:145, 1:4, 4:1}（v243 §1C）＋フィールド名整合から「実運用未参照の残存計数」で裁定（確度: 高）。
> - 補足: v240 §4-1 の version/flag 仮説は v243 §1 で棄却済み（本件は「読手不在」の確定）。

**判定: 解明（write-only 機械確定）**（証拠 `v245_7_out.txt` / `v245_7b_out.txt`、v245_4/4b の D-set 抽出バグを明記・追補）。

---

## §5 item5（V-238-8-5）ノード2-5（`0xBA94E8`/`0xBA950C`/`0xBA9510`/`0xBA9514`）のレジストリ命名 — **解明（機能命名まで。RTTI 不在のため SDK 正式名は推定）**

（ノード構造 `{+0 factory, +4 next, +8 out}` と head `0xBA8188` push は v243 §4A で確定済み。以下は各 out 実体の中身。）

### (1) node2 `0xBA94E8`（vt `0xAA28A4`・size 8）— **Havok クロック/タイマー・シングルトン（rdtsc＋QPC 校正型）**

- ctor `FUN_00945BD0`: allocator(8, tag0x15) → `[eax]=0xAA28A4`（`v245_5_out.txt` (c)）。
- vtable: slot0 `0x8BB3F0`（dtor）、slot1 `0x68F970`（`ret 4`＝purecall 系）、**slot2 `0x945A30` = `rdtsc` を edx:eax で返す tick ゲッタ**、**slot3 `0x945A50` = クロック校正/経過時間算出**。
- slot3 本体（`v245_8_out.txt` (B)）: slot2(tick)と **IAT `QueryPerformanceCounter`/`QueryPerformanceFrequency`（0xA28158/0xA2815C）** を交互に取り、`x=x*x+1` を5000回回す忙ループで rdtsc↔QPC の比を測定 → **`[0xBA94F0]/[0xBA94F4]`（64bit キャッシュ）に格納**、double 定数 **`1000000.0`（0xA47CA8）** で µs 換算、`[0xBA94F8]`（FUN_00945EB0 が参照）に係数/状態。
- 読手: `FUN_00917FC0`/`FUN_00917FD0` = `mov ecx,[0xBA94E8]; mov eax,[ecx]; jmp [eax+8]`（**slot2 への tail-jump**）。呼び手 = `FUN_008C9F30`（4件）/`FUN_008CA940`（2件）/`FUN_00946D90`（1件）＝Havok シミュレーション帯のタイミング計測。
- **命名: 「rdtsc＋QPC 校正型クロック/タイマー・シングルトン」**。SDK 候補 `hkSystemClock`/`hkPerformanceMonitor`/`hkStopwatch`（RTTI 不在・vtable-4 = `0x7070` 文字列残骸で**正式名は不可、確度: 低**）。

### (2) node3 `0xBA950C`（vt `0xAA2A0C`・size 0x1C＋0x7D0 バッファ）— **Havok デバッグ表示/検査システムの「名称→ファクトリ」レジストリ**

- ctor `FUN_00947B00`: size 0x1C、`+8/+0xC/+0x10`＝hkArray（data/size/flags）、`+0x14=0x80000000`、**`+0x18` = `InitializeCriticalSectionAndSpinCount` で初期化した0x7D0(2000)バイトの排他保護バッファ**（dtor `0x947B60` が `DeleteCriticalSection`）。slot0 = dtor `0x947C20`、slot1 = `0x68F970`（`v245_5_out.txt` (c)(d)）。
- 登録メソッド **`FUN_00947C50`（e8 17件）**: 既存名と `strcmp`（`0x8B1770`）重複検査 → 無ければ **stride 0xC のエントリ配列**へ {名前, factory} を追加（`v245_5c_out.txt` (b) / `v245_8_out.txt` (D)）。
- **登録名16種（全数・ユニーク）**: `ObjectInspection`、`DebugDisplay`（複数系統から登録）、`Statistics`、`WorldMemory`、`MousePicking`、`Islands`、`Tensor`、`SweptTransforms`、`ConvexRadius`、`Shapes`、`CentreOfMass`、`Phantoms`、`ForcedContactPoints (See Docs For Usage)`、`ContactPoints`、`Constraint`、`Broadphase`（`v245_5c_out.txt` (a) ＝13件＋`v245_5b_out.txt` (c) / `v245_5e_out.txt` (b) の ObjectInspection/Statistics/DebugDisplay、登録 call `e8(0x947C50)` = 17件・ユニーク16名＝`v245_5d_out.txt` (d)）。
- factory 例: `FUN_00946520`（tag0x32/size0x30 → `0x9463E0`）＝ObjectInspection 生成、`FUN_00947860`（size0x24 → `0x9477C0`）＝DebugDisplay 生成。結果は `0xBA9500`/`0xBA9508` 等の関連グローバルにも保存。
- **命名: 「Havok デバッグ表示/検査システムの名称→ファクトリ・レジストリ（VDB `hkProcess`/`hkDebugDisplay` 系と同型）」**（SDK 候補 `hkProcessFactory`、確度: 中。`hkDebugDisplay`/`hkProcess`/`hkStatisticsProcess`/`hkObjectInspector` は hkxcmd マップで実在確認）。

### (3) node4 `0xBA9510` / node5 `0xBA9514`（vt `0xAA2BD0`・size 0x14）— **コマンド/型ID→ハンドラの登録配列シングルトン×2（ディスパッチャ消費）**

- ctor `FUN_0094A5F0`（tag0x15/size0x14）: `+0`=vt `0xAA2BD0`、`+8/+0xC` = 0、`+0x10=0x80000000` ＝ **`+8..+0x10` は hkArray**（data ptr / size / flags）。dtor `FUN_0094BBC0→0x94BBF0`。vt 帯の生ダンプは slot0=`0x94BBC0`(dtor)、slot1=`0x68F970`(purecall)、その直後に **`0x34`/`0x7F7FFFFF`(FLT_MAX)/`0x40490FDB`(π) の定数3語**、さらに `0x94B880`/`0x68F970`/`0x8F4050` の別列が隣接（`v245_5_out.txt` (b)(c)）＝**slot2..4 は vtable ではなく定数と推定**（レイアウトの完全解明は未了）。
- **読手＝2本のディスパッチャ**（`v245_5_out.txt` (d) / `v245_5b_out.txt` (e) / `v245_8_out.txt` (E)）:
  - `FUN_0094A630`（span 約0x94A630..0x94B7DF、`sub esp,0x304`）が `[0xBA9510]` 参照 @`0x94B71A/0x94B75A`。
  - `FUN_0094BE00`（span 約0x94BE00..0x94CCAF、`sub esp,0x364`）が `[0xBA9514]` 参照 @`0x94CB85..`。
  - 共通構造: **引数オブジェクトの vtable slot2 戻り値を `add eax,-2; cmp eax,0x17; ja` で 2..0x19(2..25) のIDとして jump table 分岐**（`jmp [eax*4+0x94B780]` / `movzx [eax+0x94CC38]; jmp [ecx*4+0x94CC08]`）→ singleton 配列を `stride 8 {+0=ハンドラ ptr, +4=ID}` で線形探索 → 一致時に `push 4引数; call [entry]`。
- **到達**: `e8(0x94B7E0)` = `FUN_0091ADB0`/`FUN_0091C150`/`FUN_0091D010`（デバッグ表示システム群＝`SweptTransforms`/`Shapes`/`CentreOfMass` 帯）→ `call 0x94A630`（`FUN_0094B7E0+0x4D` @`0x94B82D`）。`e8(0x94CC50)` = `FUN_0091B680+0xBB`（`ConvexRadius` システム）→ `call 0x94BE00`。自己 span 内 self-call 各5件（`0x94ACE9..0x94ADCC` / `0x94BEB3..0x94C014`＝再入ディスパッチ）。**e9=0・外部 imm32 参照=0**（静的 vtable/関数ポインタ表には格納なし＝実行時/間接到達）。
- **命名: 「ID→ハンドラ登録配列シングルトン×2（node4=FUN_0094A630 用、node5=FUN_0094BE00 用）」**（デバッグ表示の描画/更新ディスパッチ、確度: 中）。

### 裁定

> - **node2 = rdtsc+QPC 校正クロック・シングルトン／node3 = デバッグ表示システム名称→ファクトリ・レジストリ（16名称）／node4・node5 = ID→ハンドラ登録配列シングルトン×2** — いずれも `'__data__'` 無関係（v243 §4B）を維持しつつ、読手・登録名・消費ディスパッチャまで特定。
> - RTTI（COL）不在・vtable_index 行なし → **SDK 正式名は推定**（候補: `hkSystemClock`/`hkStopwatch` 系、`hkProcessFactory` 系、ハンドラ表は特定クラス不明）。

**判定: 解明（機能命名まで）＋SDK 正式名は推定**（証拠 `v245_5_out.txt` / `v245_5b_out.txt` / `v245_5c_out.txt` / `v245_5d_out.txt` / `v245_5e_out.txt` / `v245_8_out.txt` / `v245_8b_out.txt`）。

---

## §6 item6（V-238-8-6）訂正記録: v240 §2A-2／§4-6 の2件の訂正ドラフト — **ドラフト作成（v240/v243 本体は無編集・反映は要承認）**

> **注記（記載不一致）**: タスク出典は「v243 §5 が検出した」とするが、v243 の実記載は **§5(c) = v240 §2A-3（`FUN_00942170+0x37C` → `FUN_00940D60`）の訂正**、**§6(c) = v240 §2A-2／§4-6（`FUN_0040D4D0` チェーン）の訂正**。本ドラフトは指定どおり **§2A-2／§4-6 の2件**（＝v243 §6(c) 検出分）とする。§2A-3 の訂正は v243 §5(c) に既記のため再掲しない。

### 訂正1: v240 §2A-2（`v240_process_havok_residuals6.md` L101-104）

- **Before（v240 L102-103 原文）**:
  > 到達チェーン（全て E8、末段だけ E9 tail-jump）:
  > **`FUN_0040D4D0`**（e8 5件: 0x4455E0/0x44E720/0x50D4C0/0x5BDCD0 等＝広範な初期化関数）
  > → `FUN_00406360`（0x40EB2C = FUN_0040D4D0+0x165C）
- **After（ドラフト）**:
  > 到達チェーン（全て E8、末段だけ E9 tail-jump）:
  > **fn@`0x40DF80`（`WinMain`＝起動オーケストレータ）** → `FUN_00406360`（**`0x40EB2C` は fn@0x40DF80 内の call**。`FUN_0040D4D0` は `0x40DF73`（ret 0x10 直前、pad `0x40DF74..0x40DF80`）で終端し、**`FUN_00406360` への経路は存在しない**）
  → `FUN_00441970` → …（以下 v240 と同じ）
- **根拠**: `FUN_0040D4D0` の E8/E9 全呼出しは5件（E8×4＋E9×1、`0x40EB2C` は含まない）＝v243 §6(b)。`0x40EB2C` の call 目的地生確認 `call 0x406360`（`v245_1_out.txt` (1f)）。fn@0x40DF80 の終端 pad `0x40DF74..0x40DF80`（len 0xC、`v245_1_out.txt` (1f)）。
- **関連**: 本訂正により v240 L118 裁定(a) の「Havok 初期化（`FUN_0040D4D0`→…）」の起点表記も fn@`0x40DF80` 起点（→`FUN_00406360`）へ読み替える。

### 訂正2: v240 §4-6（`v240_process_havok_residuals6.md` L204）

- **Before（v240 L204 原文）**:
  > 6. **[残] item1 周辺の上流文脈名**（低）: `FUN_004820CF`→`FUN_004CCCC0`（bhkWorldM シングルトン更新）と **`FUN_0040D4D0`（Havok 初期化の起点、5呼び出し）**の役割名が未命名（named_funcs は FUN_ のみ）。
- **After（ドラフト）**:
  > 6. ~~[残]~~ **[解] item1 周辺の上流文脈名**（v243 §6 + v245 §1/§5 で解明）: `FUN_004820CF` = **fn@`0x482090`（`GridCellArray` vtable slot4）の「セル移動→bhkWorldM 原点同期」実行部**。`FUN_0040D4D0` = **読込/状態遷移ステップのステートマシン・ポンプ（5呼出し: E8×4＋E9×1）**であり **Havok 初期化の起点ではない**。Havok 初期化到達チェーンの起点は **fn@`0x40DF80`（`WinMain`）**（→`FUN_00406360`→`FUN_00441970`→…）。
- **根拠**: v243 §6(b)(c)＋本 md §1（fn@0x40DF80 = WinMain 確定、sole caller CRT 境界確定）。

**判定: ドラフト2件作成（反映は承認待ち。v240/v243 本体は無編集）**。

---

## §7 自己検証（内部整合・制約遵守）

- **selftest**: 着手時に `probe_harness.py selftest` = **4/4 PASS**。
- **中断対策**: 前回19本（v245_1..5e）の全出力を読み取り、結果のうち「D-set 抽出失敗（size 0）→ A/B 結果が空振り」「fn@0x40DF80 終端判定が pad 依存で過大（0x40F75F まで）」の2箇所を検出し、本バッチで **v245_6/6b（境界再確定: ret 0x10 @0x40F637＋pad 6B→FUN_0040f640）**と **v245_7/7b（D-set 150/150 再抽出＋5経路再走査）**で是正。
- **§1**: WinMain 判定は引数順（hInstance/NULL/lpCmdLine/nShowCmd）・`ret 0x10`（stdcall 4引数）・CRT 呼出し位置・ini 読取の4点独立一致。caller 境界は SEH prolog/epilog ペア（0x98C5A0/0x98C5E5）、`__security_init_cookie`（0x997BE3 の既定 cookie `0xBB40E64E` 確認）、`GetCommandLineA`/`GetStartupInfoA` IAT 名（v245_1e の import 解析）で機械確定。
- **§2**: RTTI COL→PTD→型名の3段を生読（0xAB8D0C→0xB06A54→`.?AVGridCellArray@@`）し vtable_index 行741/740/742 と一致。ctor/dtor の e8 単一性（0x441AAA / 0x483573）と `[0xB06A2C]` 読み（min5・N² 確保）を確認。
- **§3**: Web（検索8回・有効ソース5件: hkxcmd / libSkyrim / BotW 仕様 / reshax・lukascone / 2009 実ヘッダ）でクラス名とセクション名を裏取り。`'snap'` と `__classindex__`/`__dataindex__` の確認失敗は**負証拠として明記**（断定回避）。
- **§4**: D-set は thunk E8150件と `B9 imm32` 150/150 で完全一致。経路 A/A2/B/C/A3/D/F の全数結果を表化。list2 は「D の連続配列候補」走査で 0xA9AA48（94件）のみ＝v240 の list2 と一致。
- **§5**: node2 の slot3 は IAT 名（QueryPerformanceCounter/Frequency）と double 定数 1000000.0 の生読で校正ループと確定。node3 の登録名16種は `call 0x947C50` 全17件（ユニーク16名）の再走査。node4/5 は読手 disasm（stride8・ID 2..0x19・jump table）と到達 e8（表示システム4件）で確定。
- **§6**: 訂正2件の Before は v240 の行番号付き原文引用（L102-103 / L204）、After は根拠付きドラフト。v240/v243 は**読取のみ・編集0回**。
- **外部参照**: Web（検索8回・有効ソース5件）のみ（全て上記）。他は EXE 内訳拠＋既存 cache/decompiled（参照のみ）。
- **制約遵守**: ESM 読取0回・ソース編集0回・git/docs/GT/TODO/session-memory/plan/cache 無編集・既存 `tmp/re/*.md` 無編集（新規は本1ファイル）・`%TEMP%\opencode/v245_*` のみ新規。selftest 4/4 PASS。新規中間物: `v245_6, 6b, 7, 7b, 8, 8b`（各 `_out.txt` 付き、既存19本と合わせ25本）。

## §8 新規残点

1. **[残] OEP `0xBAF2ED`（RVA 0x7AF2ED）→ CRT startup（`0x98787C`/`0x98769C`）の間接経路**（低）: OEP 帯は間接 call45件の保護/初期化スタブ型で、`WinMainCRTStartup` への接続は未追跡（`e8(0x98769C)`=NONE・`e8(0x98787C)`=NONE）。
2. **[残] packfile B クラス（`0xA9C990`）の SDK 正式名と `__classindex__`/`__dataindex__` の出所**（低）: 標準 binary セクション（`__classnames__/__types__/__data__`）と不一致。Web 4系統で該当なし＝負証拠のみ。
3. **[残] `'snap'` タグの上流出所**（低）: Havok SDK 内に同タグなし（Web 検索8回）。ゲーム側構築と推定（確度: 中）のまま。slot4 B（`FUN_0090EF80`）の書込/読込最終判別も未了。
4. **[残] dispatcher `FUN_0094A630`/`FUN_0094BE00` の静的到達経路**（低）: e8 は self-span/wrapper 経由のみ、e9・外部 imm32=0 → 実行時構築かデッドコードかの判定未了。node2/node3 の SDK 正式名（RTTI 不在）も推定止まり。
5. **[残] descriptor `+0xC` のマップ値経路**（低）: `FUN_009546D0`（vtable→descriptor マップ）の値を経由する動的読手のみ理論残（§4 補記。実運用未使用と裁定済み・確度: 高）。
