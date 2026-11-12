# v271 — 雑多残点束（6件）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 264（V264・低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0）**
- 出典残点: `tmp/re/v269_lod_havok_bundle.md` §8-2（V-269-1..3）＋ `tmp/re/v270_menu_params_bundle.md` §5-3（Y-1..Y-3）
- 入力（読取のみ）: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・.text VA=0x401000/raw 0x400、.rdata VA=0xA28000/raw 0x627200、.data VA=0xB02000/raw 0x700C00）、`tmp/re/cache/`（named_funcs/call_targets/vtable_index）、`docs/Oblivion_decompiled.c` 部分検索。
- 前提の既知確定（本束で再確認済・出典 v269/v270）: packed 索引は出荷で到達不能（bit31 無視可）、hkProcess 4.0=6 slot、抽象表先頭 0xAA2B5C、depth→z 変換（+0x58=newDepth×−0.008d）、メニュー ID 1031/1033/1050 未実装。
- selftest: 着手時 **ALL PASS (4/4)**（`%TEMP%\opencode\probe_harness.py selftest` → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- 中間物: `%TEMP%\opencode\v271_1..11*.py`（各 `_out.txt` 付き。v271_6/7 は再開跑分・v271_8..11 は件4〜6 追い込み）。
- 会話に注入されたループ運用指示（AGENTS.md の STATE.md/L2/worktree、supermemory Daily Triage）には本契約により**従わない**。

---

## 件1 V-269-1: 文字列 'HavokData'@0xA3A847 の参照関数（imm 参照 0 の原因考察含む）— 【確定：0xA3A847/0xA3A0A4 は完全文字列の内部オフセット（末尾断片）。コードは文字列先頭のみ参照＝参照 0 の原因。参照先は TESSaveLoadGame セーブ/ロードキュー帯】

### 1-1. 断片性の発見（imm refs 0 の原因）[確定]

- `0xA3A847` は独立文字列ではなく、`'TESSaveLoadGame::SaveQueuedHavokData'`（**先頭 0xA3A82C**・len 36）の **+27 位置の末尾断片**。NUL 境界走査で確定（0xA3A82B=00、文字列終端 0xA3A84F、以降 00 padding）。
- 同様 `0xA3A0A4` は `'LoadHavokData() call did not properly empty buffer.'`（**先頭 0xA3A0A0**・len 51）の **+4 位置断片**（先頭 4 バイト 'Load' が Ghidra 文字列表で分離表示された痕跡）。
- 全セクション imm32 走査（`v271_1.py` S3/S5）: `refs(0xA3A847)=0`・`refs(0xA3A0A4)=0`・**ファイル全体（非イメージ領域含む）でも 0**＝どのコード/データも文字列内部地址を指さない → **v269 の参照 0 は「断片地址を searched していた」ことが原因**（負証拠は構造的不存在ではなく検索対象の取り違え）。
- 先頭地址への参照（`v271_1b.py`）:
  - `refs(0xA3A82C)` = **.text 1 件のみ** `0x45D30C: push 0xa3a82c`（owner **FUN_0045D220**+0xEC）
  - `refs(0xA3A0A0)` = **.text 2 件** `0x4593BC`（FUN_00459370+0x4C）・`0x45D12F`（FUN_0045D030+0xFF）
  - 同帯姉妹文字列 `'LoadHavokData() call finished, but still has elements in the map.'`@0xA3A7E8 → 1 件 `0x45D179`（FUN_0045D030+0x149）、`'TESSaveLoadGame::SaveQueuedAttachedAnimation'`@0xA3A854 → 1 件 `0x45D3F9`（FUN_0045D390+0x69）

### 1-2. 参照サイトの意味 [確定]

- **0x45D30C パターン＝ロックラベル句法（文字列は実引数にされない）**: 直後 `0x45D311 mov ecx, 0xb33b80 / 0x45D316 call 0x401020`。`0x401020` は `mov [esp+4],ecx; jmp [IAT 0xA2806C]` ＝ **EnterCriticalSection** の thiscall ラッパ（IAT 名は import ディレクトリ自力解決で確定・`v271_1f`：0xA2806C=EnterCriticalSection、整合検査 0xA280D0=GetTickCount=v270 既知と一致）。push した文字列はラッパの `mov [esp+4],ecx` で**上書きされる死スロット**（グローバル CRITICAL_SECTION `0xB33B80` を引数化するための 1 スロット確保兼ロック帯ラベル）。同形实例: `0x424FB0..BA`（push 0xa35174→mov ecx,0xb33b80→call 0x401020）。関数末尾 `0x45D372 mov ecx,0xb33b80; call 0x401040` = **LeaveCriticalSection** ラッパ（IAT 0xA28074）で対。
- **0x4593BC / 0x45D12F＝実エラーログ**（`vtable+0x18` 経由）: `mov ecx,[0xB34D90]; mov edx,[ecx]; mov eax,[edx+0x18]; push 0xa3a0a0; call eax` ＝ログシングルトン（グローバル [0xB34D90]・.text refs 50 件）の print 系 slot へメッセージ実引数として渡す。FUN_00459370 は直前 `call 0x4E31E0`（formID 解決系）の結果と `[esi+0x14]` を比較して不一致時のみ出力（ret 8）。

### 1-3. owner 関数と帰属 band [確定]

| 関数 | 文字列 | 呼び主（e8 実測） | band |
|---|---|---|---|
| **FUN_0045D220**（ret 4・冒頭 thread-guard: `call [0xA2808C]`=GetCurrentThreadId vs `[0xB33398]+0x10`、save フラグ `[+0x18]>>0x12&1` gate） | 'TESSaveLoadGame::SaveQueuedHavokData'（0xA3A82C）ロックラベル | FUN_004E0F80 の **0x4E1188/0x4E1196**（`mov ecx,[0xB33B00]; push ebp; call 0x45D220`＝セーブ書込ループ内・同所 0x4E11A2 で姉妹 FUN_0045D390 を呼ぶ） | セーブ書込 0x4Exxxx 帯 |
| **FUN_0045D030**（キュー配列 [esi+0x60] の null 巡回→drain 確認） | 'LoadHavokData() call did not properly empty buffer.'（0xA3A0A0）＋ '…finished, but still has elements in the map.'（0xA3A7E8） | FUN_00461030+0xA2@**0x4610D2**（FUN_0045FDA0=セーブ読込と同一 band・v269 既知） | セーブ読込 0x46xxxx 帯 |
| **FUN_00459370**（ret 8・[esi+0x14] 位置退避/復元） | 0xA3A0A0 エラー | FUN_00424F90+0x4A@**0x424FDA** | レコード処理 0x42xxxx 帯 |
| **FUN_0045D390** | 'TESSaveLoadGame::SaveQueuedAttachedAnimation'（0xA3A854） | FUN_004E0F80@0x4E11A2 | セーブ書込帯 |

### 1-4. 裁定

> - **'HavokData' 語の所有者 = TESSaveLoadGame セーブ/ロードキュー系**（`TESSaveLoadGame::SaveQueuedHavokData` 完全名で確定。グローバル [0xB33B00]=キュー保持シングルトン、ecx 実引数で FUN_0045D220/0x45D390 を呼ぶ書込側と、読込側 FUN_0045D030 の drain チェック 2 メッセージ）。Havok visualize/VDB 系（件5 帯）との紐付けは v269 に続き**再度否定**（文字列実体は save subsystem のロックラベル/ログ）。
> - **imm 参照 0 の原因 [確定]**: v269 が検索した 0xA3A847/0xA3A0A4 は文字列**内部**オフセットで、コードは先頭アドレス（0xA3A82C/0xA3A0A0）のみを参照する。先頭への参照は上表のとおり計 4 .site。
> - 副産物: `0x401020/0x401040` = EnterCriticalSection/LeaveCriticalSection の thiscall ラッパ（引数スロット埋め替え型）、`0xB33B80` = 共通グローバル CRITICAL_SECTION、`[0xB34D90]` = ログシングルトン（refs 50）。

**判定: 解明 [確定]**（証拠: `v271_1_out.txt` S1/S2/S4/S5・`v271_1b_out.txt` S2〜S4・`v271_1c_out.txt`・`v271_1d_out.txt` S1〜S4・`v271_1f_out` import 整合。V-269-1 残点解消）

（件1 了）

---

## 件2 V-269-2: FUN_004D5600 の呼出登録経路 — 【確定：GridCellArray クラス vtable 0xA3D79C の slot1(+4) に立候補 thunk 0x482080（`call 0x481E00; jmp 0x4D5600`）経由で登録。E8/imm 0 の原因は tail-jmp＋仮想呼び出し】

### 2-1. 唯一の静的エントリ = tail-jmp（`v271_2.py` S2）[確定]

- E8 呼出 0・imm32 refs 0 を再確認（`v271_2.py` S1/S6：.text/.rdata/.data 全走査＋ファイル全体 needle 0）。
- **E9 走査で唯一ヒット**: `0x482085: jmp 0x4D5600`。ブロック `0x482080 = call 0x481E00 / jmp 0x4D5600`（int3 分離の 10 バイト thunk。0x481E00 = thiscall で `[ecx+4]=[ecx+8]=0xFFFFD8F0(-10000)` を書く only-callee、refs 1 点＝当 thunk のみ）。
- → v269 件3 と同一教訓（**E8 走査は tail-jmp を捕らえない**）で参照 0 を説明。

### 2-2. 登録先の実体 = `.?AVGridCellArray@@` の vtable（`v271_2c/2f.py`）[確定]

- thunk 0x482080 への imm32 refs は全イメージで **1 点のみ**: `.rdata@0xA3D7A0`。周辺 hexdump で 0xA3D79C〜0xA3D7C0 は連続コードポインタ列＝**10 slot の vtable**（vtable 密集帯 0xA3D300〜、前後は GridArray/GridDistantArray 等の姉妹 vtable）。
- **RTTI 解決（vtable-4=COL 規約・VS6 COL={0,off,0,TD+0xC,CHD+0x10}）**: `[0xA3D798]=0xAB8D0C` → COL+0xC=0xAB8D18→TD=**0xB06A54**、TD+8 名 = **`.?AVGridCellArray@@`**。姉妹: COL 0xAB8C6C→`.?AVGridArray@@`、COL 0xAB8D58→`.?AVGridDistantArray@@`（`v271_2f.py` S3 実測）。
- vtable 0xA3D79C の slot 一覧: [0]=0x483570 / **[1]=0x482080(thunk→FUN_004D5600)** / [2]=0x481E20 / [3]=0x4821E0 / [4]=0x482090 / [5]=0x481E60 / [6]=0x482200 / [7]=0x482240 / [8]=0x482260 / [9]=0x4822A0。

### 2-3. 書込元（登録サイト）と実 dispatch（`v271_2f.py` S2・`v271_2g.py`）[確定]

- `mov [obj], 0xA3D79C` は .text に 2 点のみ:
  - `0x483382`（**FUN_00483350** = GridCellArray ctor。`call 0x481DE0` 後 `[+0x24]=0`、`[0xB06A2C]`(uGridsToLoad) を 5 以上奇数へクランプし配列確保）
  - `0x48348D`（**FUN_00483460** = reinit。唯一の呼び主＝slot0 FUN_00483570 先頭の `call 0x483460`）
- ctor 呼び主 1 点: `0x441AAA`（FUN_00441970＝**メイン初期化帯**・v269 の enable 6 点と同族 band）→ `push 0x64; call 0x401F00`(malloc 100B) → ctor → `mov [esi+8],eax` 後、即座に **`0x441AB6..C2: mov edx,[eax]; mov eax,[edx+4]; call eax` ＝ slot1 仮想呼出**（＝FUN_004D5600 の実起動証拠。以降 0x14 malloc 続行＝グリッド生成列）。
- 関数本体意味（`v271_2.py` S3）: `[0xB3F9A8/AC/B0]`（現在セル位置 triple・refs 407/324/323 のホット global）をスタック struct に組んで FUN_004D5000（enable site5 0x4D52D0 の owner）を呼出→返り値と `[0xB35C24]` を InterlockedIncrement/Decrement で入替（AddRef/Release 形）、`[0xB35C14]` float を `[0xA2FF44]` と比較し最小更新。**GridCellArray::slot1 = セル位置変更時のグリッド再要求ハンドラ**と解す（v269「ロード要求同期」[推定] を支持、登録経路確定で解消）。

### 2-4. 裁定

> **FUN_004D5600 = GridCellArray（`.?AVGridCellArray@@`）vtable slot1 の tail-jmp 実体** [確定]。登録経路 = ctor FUN_00483350@0x483382 / reinit FUN_00483460@0x48348D が vtable 0xA3D79C を書込 → dispatch は `call [vtable+4]`（実証拠 0x441AC2、初期化帯 FUN_00441970）。enable site5（0x4D52D0）到達 = GridCellArray slot1 呼出時（セル位置 triple 変化帯）。V-269-2 残点解消。

**判定: 解明 [確定]**（証拠: `v271_2_out.txt` S1/S2/S3/S6、`v271_2c_out.txt` S1/S2/S4、`v271_2f_out.txt` S1/S2/S3、`v271_2g_out.txt` S1〜S4。V-269-2 残点解消）

（件2 了）

---

## 件3 V-269-3: タイプ別アーカイブ表 0xB338E8 の行2（slot 9..17 = 0xB3390C..0xB3392F）を書こうとする外部経路 — 【確定：非ゼロ書込（登録）経路は**不在**。行2 に触れる書込は「ゼロ化」3 種のみ（memset / close 条件クリア / shutdown 全消去）。読取側（fallback 検索・bit31 セレクタ）は実在 → 半配線構造】

### 3-1. 全面走査の手順と結果（`v271_3.py`・`v271_3b/3d`）

- (a) 18 slot 全地址＋base の imm32 refs を列挙（S3）→ ヒットサイトは FUN_0042BE10/BE70/DB10/EBC0/F4C0 の 5 関数に閉じる（他 band からの参照ゼロ）。
- (b) .text 全体の**絶対ストア命令スキャン**（C7 05・A3・89/88 mod=00 rm=101 の全 6 レジスタ形・C6/88＋imm8 形、`v271_3d_out.txt`）→ [0xB338E8,0xB33930) への直接書込は **FUN_0042BE70 の 18 条のみ**（下表 W3）。base+disp 形（`mov [eax+0xB338E8],edi`）は (a) の refs に包含され全点検済。
- (c) `rep stosd/movsd` を表参照 5 関数内で探索（S7）→ FUN_0042F4C0 の 2 点（0x42F856/8FF）は**スタック buffer への文字列コピー**（`lea edi,[esp+0x148]` 等）で表と無関係。
- (d) .data 初期値: 0xB338E8 は raw バック外＝**BSS（ゼロ初期化）**（S1）。

### 3-2. 行2 に到達し得る書込の全量（3 サイト、すべてゼロ）

| # | サイト | 命令列 | 条件 | 到達経路 |
|---|---|---|---|---|
| W1 | `0x42F624..2D`（FUN_0042F4C0 内） | `push 0x48 / push 0 / push 0xB338E8 / call 0x981630` ＝ **memset(base,0,72)**（0x981630 は CRT memset：整列 byte フィル＋`rep stosd`） | 無条件（open/初期化準備列） | FUN_0042F4C0（アーカイブ open・v269 済） |
| W2 | `0x42BE42..4A`（FUN_0042BE10・close） | `cmp [eax+0xB3390C],esi / jne / mov [eax+0xB3390C],edi(0)` を eax=0..0x20 ループ（行0/行2 対で周回） | **閉じるアーカイブとスロット値が一致した時だけ 0 クリア** | close 列（呼び主 0x42C943 e8 実測） |
| W3 | `0x42BF38..9E`（FUN_0042BE70・shutdown） | 18 slot 全地址へ `mov [slot],edi(0)` 直列（0xB338E8..0xB3392C・89 3D 形 18 条） | 無条件全消去（解放は [0xB33930]/[+0x34] リスト free と伴） | release 列（呼び主 0x40C377 e8 実測） |
| (対) | `0x42F983/8D`（open 書込ループ） | `cmp [ecx*4+0xB338E8],0 / jne / mov [ecx*4+0xB338E8],eax / inc ecx / cmp ecx,9 / jb` | **ecx≤8 = 行0 のみ**（v269 再確認） | — |

- **非ゼロ値を書く行2 経路（＝bit31=1 レコードの実アーカイブ登録）は (a)(b)(c)(d) の全走査で 0 件** [確定]。
- 読取側は存在: FUN_0042DB10@`0x42DB14/21`（行0 slot 空なら **行2 `[edx*4+0xB3390C]` へ fallback** 検索し 0x42D240 へ渡す）＋ FUN_0042EBC0（bit31→行2 セレクタ）。→ 行2 は「読取配線あり・非ゼロ書込配線なし」の半完成機能で確定（v269 の移植 bit31 無視可 [確定] を再支持）。

### 3-3. 裁定・訂正ドラフト（v269 向け・本体無編集）

> - V-269-3 判定 **[確定]**: 外部 writer の有無 = **ゼロ化 writer 3 サイトのみ**（W1 memset@0x42F624・W2 close 条件クリア@0x42BE4A・W3 shutdown 全消去@0x42BF3E..9E）。**非ゼロ登録経路は不在**（.text 全スキャン＋レパルスキャンで網羅証明）。
> - v269 件2「行2 の書込元は出荷コードに**存在せず**」→ 厳密には「**非ゼロの書込元は存在せず**（ゼロ化は close/shutdown/memset の 3 所、うち BE10 は v269 が『読/クリア専用』と記した箇所＝実際に 0 書込を含む）」に精緻化ドラフト。結論（到達不能・bit31 無視可）不変。

**判定: 解明 [確定]**（証拠: `v271_3_out.txt` S1/S3/S4/S5、`v271_3b_out.txt` S1〜S4、`v271_3d_out.txt` S2 18 点一覧、v269_2f/2g_out 再利用。V-269-3 残点解消）

（件3 了）

---

## 件4 Y-1: [w+0x24] への Tile3D 系割当サイト（xref 0＝間接経路）と生成元の同一性 — 【確定：[reg+0x24] への非ゼロ書込（Tile の 3D 先続スロット）は Tile 系 slot2（update）3 関数 4 サイト（0x592D55/0x59211E/0x5926F3/0x5927AD）。窓の先続 X = **BSScissorNode**（NiNode 派生・FUN_004A15F0・vtable 0xA3FB54）で Tile3D ではない。Tile3D クラス自体の生成元は factory 0x5902A0 case0x388 の E9 tail-jmp 1 点のみ＝「xref 0」の正体】

### 4-1. [reg+0x24] 書込サイトの全量（band 0x570000..0x5C0000 機械走査）[確定]

- 走査手順：(a) imm32 全イメージ走査で Tile3D/姉妹 vtable（0xA6AEEC/0xA6AEC4/0xA6AF14/0xA6AF3C）の立候補点列挙（`v271_4/4b/4e`）、(b) slot 割当表から slot2=update 関数を特定（`v271_10 U4`：**0x592A70∈0xA6AF44=TileWindow vt+8・0x591DC0∈0xA69140=TileRect(0xA69138)+8・0x592390∈0xA6AECC=TileText(0xA6AEC4)+8**）、(c) 各関数内の `mov [reg+0x24]` 実文脈確定（`v271_6 U4`、`v271_7 A5`、`v271_9 T2`、`v271_11 V4`）。
- 非ゼロの「3D 先続」を書き込むサイト（すべて**仮想 slot2 経由のみ・直接 E8 呼出 0 件**＝Y-1 の「間接/共有呼び出し」推定どおり。`v271_9 T9`: e8(0x592A70)=0）:

| サイト | 所有者（slot） | 割当前の生成 | 書込内容 |
|---|---|---|---|
| `0x592D55: mov [esi+0x24],eax` | **FUN_00592A70 = TileWindow::slot2** | `0x592CFC push 0xec / 0x592D32 call 0x401F00`(malloc 0xEC) → `0x592D4C call 0x4A15F0` | **BSScissorNode**（4-3）。直後 `0x592D5E mov eax,[edx+0x84]; call` = addChild(cast 子 edi,1) |
| `0x59211E: mov [esi+0x24],eax` | **FUN_00591DC0 = TileRect::slot2** | `0x5920C7 push 0xdc / 0x5920FD malloc` → `0x592115 call 0x70B780` | **NiNode 素**（0x70B780=vtable 0xA7E38C `.?AVNiNode@@` 立込：`v271_10 U2`）。直後 +0x84 addChild 同形（`v271_11 V4`） |
| `0x5926F3 / 0x5927AD: mov [esi+0x24],eax` | **FUN_00592390 = TileText::slot2** | `call 0x70B780`（NiNode ctor）経由（`v271_7 A5` 実測） | NiNode 系先続（同族 2 書込＝切替再生成） |

- 基底 ctor 群は [+0x24]=0 クリアのみ（0x58FDE6/0x58FFAD(Tile3D 自身)/0x5900AB/0x59016B/0x59025B — `v271_4b S2`/`v271_4e S3`/`v271_10 U6`）。Tile::Init（slot1 共有 FUN_0058DA00）も `0x58DA22 mov [esi+0x24],ebx(0)` で**自己ゼロ化**のみ（`v271_8 S1` — Y-1 冒頭仮説「0x58DA00 が窓への登録関数」は否定。引数 (ebp,0,0) を読まず自フィールド初期化＋0x4028D0 登録＋0x58D1C0 呼びのみ・ret 0xc）。

### 4-2. Tile3D クラスの生成元と「xref 0」の正体 [確定]

- **要素ファクトリ FUN_005902A0**（cdecl・switch 0x385..0x38B・jmp table 0x590314 実回収：`v271_6 U4`）：case **0x388** = `0x5902D1 push 0x5c / 0x5902D3 call malloc / 0x5902DF mov ecx,eax / **0x5902E1 jmp 0x58FF50**`（tail-jmp。E8 不在＝v269 件3 と同一教訓。A1 走査：e8(0x58FF50)=[]・**e9=['0x5902e1'] のみ**・dword refs 0）。他 case：0x385→FUN_00590070(TileRect 0x44・vt 0xA69138)、0x386→FUN_00590000(malloc 0x4c)、0x387→FUN_0058FEC0(TileText 0x54・vt 0xA6AEC4)、0x389→FUN_00590130(TileMenu 0x48・vt 0xA6AF14)、0x38B→FUN_00590220(TileWindow 0x40・vt 0xA6AF3C)。typecode は float ノード値→int 変換で獲得：`0x59035F fld [edi+4]; call 0x9828C0; push eax`（件5 の変換器と同一）。
- 唯一の呼び主 = **FUN_00590330**（要素生成器・`0x590369 call 0x5902A0(typecode, ebp=this)`）→ 生成物を**要素ノードの [+0x10] スロット**へ：`0x59039E mov [edi+0x10],esi`、slot1 仮想起動後 `0x59038B lea ecx,[esi+8]; call 0x4028D0`（参照登録）（`v271_8 S3/S10` 全文字）。FUN_00590330 の呼出元 2 点：FUN_00585410@0x5854DC（パラメータ適用帯）と **FUN_00590420@0x59047B**（XML 要素生成 API・**e8 呼び主 49 点**＝GUI ローダ各所。`v271_8 S2` 一覧）。
- 削除 ctor ラッパ FUN_0058FF30（`call 0x58FE50`＝TileText base ctor＋flag&1 で 0x401F20 free）は **e8・imm・dword 全て参照 0**＝出荷で到達しないデッド包装（`v271_4_out S2`）。→ Y-1 の「0x58FE50 派生 ctor 経路と推定」は**訂正**：実体生成は上記 tail-jmp のみ、0x58FE50/0x58FF30 は未使用経路。

### 4-3. 同一性の最終化（X = BSScissorNode）と v270 向け訂正ドラフト [確定]

- FUN_004A15F0 全文字（`v271_10 U1`）：`push 0; call 0x70B780`（NiNode base ctor）→ **`0x4A15FA mov [esi],0xA3FB54`**（派生 vtable 差替）→ ret。**RTTI 解決（vtable-4=COL・VS6 COL={0,off,0,TD+0xC}・TD+8 名）**：COL 0xAB9214→TD 0xB07308→名 **`.?AVBSScissorNode@@`**（Beth Softworks の UI クリップ用 NiNode 派生・size 0xEC）。typecode slot（vt+0xC）= 0x6F7070 = `xor eax,eax; ret` ＝**0**（NiNode 素と同一：0x6F7070）＝Tile3D の 0x388 を返さない。
- 帰結（v270 訂正ドラフト・本体無編集）：v270 §2 の「X=[w+0x24] 先続は Tile3D 系（vtable 0xA6AEEC・初值 −1.0f@+0x58）［推定］」は**対象取り違え**。(1) 窓（TileWindow・[root+0x1c]）の先続は**BSScissorNode**、(2) +0x54/58/5c の float 三つ組と +0x18 u16 は **NiAVObject/NiNode 側レイアウト**（slot2 実測：`0x5920E0/EF/FA・0x592D15/24/2F` で [子+0x54/58/5c] へ float 書込＋depth×−0.008 中央配置は 0x584980 と同型）、(3) −1.0f@+0x58・u16 ペア +0x54/56 は **Tile3D パラメータ要素**（0x58FF50・vt 0xA6AEEC・typecode 0x388）の自フィールドで、FUN_0058FD00 の 0x388 ゲートを通過するのはこちら（要素チェーンの [+0x24] 先続）。**z 変換の実体裁定（depth×−0.008 を先続 3D 位置の中央 float へ）はクラス名訂正後もそのまま成立**。
- 恒久化：X 生成=窓 slot2 FUN_00592A70@0x592D55（BSScissorNode 0xEC）、矩形 slot2 FUN_00591DC0@0x59211E（NiNode 0xDC）— いずれも仮想経由のみ。移植側は「窓開き時に 3D アタッチノードを 1 本確保し Z を depth×−0.008 に置く」で等価（Tile3D クラス再現は不要）。

### 4-4. 裁定

> Y-1 残点解消。[w+0x24] の割当サイト=Tile 系 slot2 3 関数 4 サイト（うち窓は 0x592D55 の 1 点・生成元 FUN_004A15F0=**BSScissorNode**［確定・RTTI 実名］）。Tile3D（`.?AVTile3D@@`）の生成元は factory 0x5902A0 case0x388 の tail-jmp 1 点に確定し、xref 0 の原因は E9 tail-jmp＋削除ラッパ（0x58FF30）不在使用。v270 の「X=Tile3D 系」は「X=NiNode 系（窓=BSScissorNode）」へ訂正（実体結論不変）。

**判定: 解明 [確定]**（証拠: `v271_4/4b/4c/4d_out`（vtable/ctor 群）、`v271_6 U4`、`v271_7 A1/A2/A5/A7`、`v271_8 S1/S2/S3/S10`、`v271_9 T1/T2/T9`、`v271_10 U1/U4/U6/U7`、`v271_11 V1/V4`）

（件4 了）

---

## 件5 Y-2: FUN_0058B2F0 の id(0xFA8) 特殊分岐が GetLast→vtable+4(uVar) に渡す `FUN_009828C0(param_1)` の引数意味（this 漏り疑い） — 【確定：0x9828C0 は ST(0) の float→int 変換ヘルパ（実引数なし・返り eax）。Ghidra の `(param_1)` _linkは FPU スタック追跡の誤帰属で、this（param_1=元タイル edi）は `push edi` で slot1 の実引数に明示的に渡っている。漏り無し】

### 5-1. 分岐全文字（`v271_9 T1` 0x58B4E1..0x58B50F・0x58B2F0 末尾側）[確定]

```
0x58B4E1 cmp esi, 0xfa8          ; id=4008（'id'。v264 §2-1 表 |4008|FA8|id|）
0x58B4E9 mov ecx, edi / call 0x5898f0   ; GetLast(最終子)（X-1 の最終子 getter と同一）
0x58B4F6 fld dword ptr [esp+0x14]       ; ST(0) ← 第3スタック実引数 = param_3 = setter 新値 float
0x58B4FA mov ebx, [esi]                 ; 最終子の vtable
0x58B4FC push edi                       ; ★ 元タイル(this=param_1) を引数として push
0x58B4FD call 0x9828c0                  ; float→int 変換（ST(0) 消費・返り eax）
0x58B502 push eax                       ; 変換後 int を第1引数
0x58B503 mov eax, [ebx+4]               ; 最終子 slot1
0x58B506 mov ecx, esi                   ; this=最終子
0x58B508 call eax                       ; 最終子->slot1((int)新値, 元タイルedi)
```

- 関数入口の引数配置（`v271_8 S4` 冒頭）：`mov esi,[esp+0xc]`（push ebx/esi 後）= entry+4 = **id**（C の param_2 が denormal float 列 5.6e-42 で比較されているのは id 整数の比特別＝表示上の変換、0xFAD=5.62341e-42・0xFAB=5.62061e-42…と一致）、entry+8 = **新値 float**（C の param_3）、ecx = this。0x58B4F6 の `[esp+0x14]` は push 3 個後に一致＝param_3 の新値。

### 5-2. FUN_009828C0 の実体 [確定]

- 本体（`v271_8 S5`）：`cmp dword [0xBAABE0],0 / je FPU フォールバック / push ebp; mov ebp,esp; sub esp,8; and esp,-8 / fstp qword [esp] / cvttsd2si eax,[esp] / leave / ret`。**入力=ST(0)（float）、出力=eax（切り捨て int）、スタック引数・ecx this なし**。[0xBAABE0] は SSE2 有無フラグ、フォールバック（0x9828DC）は `fnstcw` で制御語 round-toward-zero を組み替える x87 経路。MSVC CRT の float→int 変換ヘルパ（_CI2f 相当）。
- 既知の他の呼出形態との完全一致：X-1 の `0x58B623 fild [esi+0x18]; fcomp st(1); jp; 0x58B62F call 0x9828c0; 0x58B634 mov [esi+0x18],eax`（新値 float が ST(0) のまま変換）／件4 の `0x59035F fld [edi+4]; call; push eax`（typecode 変換）— 全 site で直前に `fld/fild`、call 命令長 5 バイト（E8 disp32）のみ＝**push 引数ゼロ**が共通。

### 5-3. 裁定

> 「this 漏り疑い」は**否定**（decompiler artifact）。Ghidra は FPU スタックの ST(0) を直近の既知値 param_1（this）に誤紐付けして `FUN_009828C0(param_1)` と印刷している。実際の 0xFA8 分岐意味：**「id(4008='id') の float 新値を int に切り捨て、最終子 Menu の slot1(value:int, sourceTile:this) へ通知する」**（X-1 の depth 通知ハンドラ FUN_0058B4B0 が同一変換器で float→int キャッシュ更新するのと対になる id 特殊中継）。this は 0x58B4FC `push edi` で欠落なく渡っている。

**判定: 解明 [確定]**（証拠: `v271_8 S4/S5`、`v271_9 T1`。docs/Oblivion_decompiled.c L295765〜（C の誤帰属実物）・L295842 `uVar5 = FUN_009828c0(param_1);` 該当行と突合済み）

（件5 了）

---

## 件6 Y-3: E 帯（0x57F6C0 所在関数＝FUN_0057F490）の esi クラス特定 — 【確定：esi = **InterfaceManager** の単一实例（グローバル [0xB3A6E0]・size 0x134・遅延生成 FUN_00582160）。ソース文字列 `.\Interface\InterfaceManager.cpp` と 4 本の `"InterfaceManager: … Root"` ログ文字列で実名確定（RTTI 無し＝非仮想クラス）。z 変換第二实例・+0xb9 ダート機構も確定】

### 6-1. クラス名の確定経路 [確定]

- 関数境界訂正：v270/本束の「E 帯 0x57F6C0 関数」の本体先頭は **FUN_0057F490**（0x57F6C0 は中盤。境界走査で int3 区切り 0x57F490 頭・唯一の E8 呼出元 `0x579362`＝FUN_00579320+0x42・`v271_4b S1`/`v271_7 B3` e8=1 点・imm refs 0＝仮想外/private ヘルパ）。
- ecx 実体（呼出側 `v271_7 B2`）：`0x579358 call 0x582160`（root getter・push 0,1）→ `mov ecx,eax; call 0x57F490` ＝ **esi = root getter の返り値 = グローバル [0xB3A6E0] の単一实例**。
- 単一实例の生成（`v271_7 B4/B5`）：FUN_00582160 = 遅延 getter（`cmp byte [esp+0x14],0 / mov eax,[0xb3a6e0] / test / 0x582190 push 0x134; call malloc; ecx=eax; 0x5821AF call 0x5802B0`(ctor) → `0x5821C7 mov [0xb3a6e0],eax` → `0x5821CC call 0x581CC0`(2 期 ctor)）。**サイズ 0x134**。getter の E8 呼出 = **763 サイト**（.text 全体・`v271_9 T7`/S9）＝GUI 最_hot_ アクセッサ。
- 実名証拠（`v271_10 U5` 文字列参照解決）：
  - `'InterfaceManager: Menus Root'`@0xA691F0 → ref **FUN_00581CC0+0x81@0x581D41**（2 期 ctor 内）
  - `'InterfaceManager: DebugText Root'`@0xA691B4 → ref **FUN_00581CC0+0x202@0x581EC2**
  - `'InterfaceManager: Main Root'`@0xA6902C → FUN_0057ED20+0x172@0x57EE92、`'Cursor Root'`@0xA68FFC → FUN_0057ED20+0x2E7@0x57F007（`push str / call 0x6FF420` ＝デバッグ出力・0x581CC0 と同形）
  - ソースパス `'.\Interface\InterfaceManager.cpp'`@0xA69054（同一 .rdata 密集帯）
- RTTI 不在の説明：2 期 ctor FUN_00581CC0 の vtable 立込は子生成分のみ（0x581D30 `mov [eax],0xa69138`=TileRect・`v271_9 T6`）。1 期 ctor FUN_005802B0 は `[esi]=0/[esi+4]=0`（+0x00/+0x04 が**先頭/末尾ポインタ対のリストヘッド**で、vtable 欄ではない）。＝非仮想クラス → `.?AVInterfaceManager@@` 型記述子は存在しない（'.?AVInterfacedClass@@'@TD 0xB047F0 は別物＝Interface 登録基底で、COL 参照 0・本束範囲外）。**クラス名の格は「デバッグ文字列確定」＝確定扱い可**（出荷バイナリに唯一対応する実名帯）。

### 6-2. Y-3 のフィールド照合（+0x1c/+0x2c..34/+0xb9）[確定]

- **+0x1c** = 先頭窓ポインタ（`mov ecx,[esi+0x1c]; mov eax,[ecx+0x24]`＝件4 の窓→BSScissorNode 先続。0x58DA70（Tile::Init 末尾・共通 clear 関数）でも `call 0x582160` 後 `[eax+0x98]==ebp` クリア参照あり＝同一实例）。
- **+0x2c/0x30/0x34 平行三つ組**（z 変換第二实例）：`v271_8 S8`（FUN_0057F490 tail 0x57F700..780 実測）：
  `0x57F725..736 mov [0xb3a6e8]=edi/[0xb3a6ec]=eax/[0xb3a6f0]=ebx`（入力 x,y,z のグローバルミラー・ref は FUN_0057F490 のみ＝`v271_9 T8`）→ `0x57F736 mov ecx,[esi+0x1c]; push 0xfab; call 0x588bd0`（**窓 w の depth getter**）→ `0x57F743 fmul qword[0xA68FD0]`（**−0.008d・X-2 と同一の定数**）→ `0x57F74D mov [esi+0x2c],edi / 0x57F758 mov [esi+0x30],nv / 0x57F75C mov [esi+0x34],ebx / 0x57F75F mov byte[esi+0xb9],1`。**中央スロット +0x30 = depth×−0.008**（X-2 §2-2 の「中央スロット変換」の原型がこれ。第二实例どころか v270 が参照した 0x57F743 site そのものの全文字確定）。
- **+0xb9 ダート機構**（`v271_6 U6` パターン列 + `v271_8 S8` 文脈）：
  | サイト | 内容 | 意味 |
  |---|---|---|
  | 0x58043E（1 期 ctor） | `mov byte[esi+0xb9],1` | 生成時=ダート |
  | 0x57F75F（FUN_0057F490 末尾） | 1 セット（位置三つ組更新時） | 位置変更→要再適用 |
  | 0x57FDA9（FUN_0057FD60 末尾） | 0 クリア＋直前 `or word [X+0x18],1`（先続ノードの u16 ダート bit0）＋`setter([esi+0x1c], 0xfa1, float)` | 再適用後に親窓側へ伝播 |
  | 0x581012（FUN_00580BA0・cmp ebp,1 節） | 条件クリア | 別更新路 |
  | 0x582365/0x5823EB/0x582737/0x58274A（FUN_005821F0） | `cmp byte[esi+0xb9],0 / jne` 消費 | **メイン更新ループ**（InterfaceManager::Update 相当・0x5821F0 は [0xB3A6E0] 参照帯の最大消費者・`v271_7 B7`） |
- FUN_0057F490 本体の意味確定：引数 2 float（esp+4/esp+8）を解像度グローバル **fild [0xB06C4C]=640 / [0xB06C50]=480** と画面 const **1280.0@0xA688A8 / 960.0@0xA68D78** 等で正規化スケーリング（`v271_6` float/int 実測表）→ キャッシュ三つ組＋depth×−0.008 を窓先続ノード（[w+0x24]＋0x54/58/5c）へ反映し、`0x57F61F call 0x707370`（X-2 で確定済の 3D 反映関数）を呼ぶ。**HUD 系ではなく InterfaceManager（Menus Root）の「窓 3D 内容の位置設定」**。呼出元 FUN_00579320（2 float をそのまま透過）の呼び主は 0x405370・0x5AF440（`v271_5 S6`）。

### 6-3. 裁定

> Y-3 残点解消。E 帯関数 = **FUN_0057F490（先頭 0x57F490）**、esi = **InterfaceManager 単一实例**（[0xB3A6E0]・0x134B・ctor FUN_005802B0＋FUN_00581CC0・実名は 4 ログ文字列＋cpp パス文字列で確定・RTTI 無し）。+0x1c=先頭窓、+0x2c/30/34=位置三つ組（中央=窓 depth×−0.008＝z 変換の実装原型）、+0xb9=ダート（ctor/変更で 1→FUN_0057FD60/0x580BA0 で 0・FUN_005821F0 更新ループが消費、伝播先 u16 bit0@X+0x18 は FUN_0058FD00 の集約と同じ旗）。v270 の「HUD 系か」推定は **InterfaceManager（メニュー root）に訂正**。

**判定: 解明 [確定]**（証拠: `v271_4b S1`、`v271_5 S6`、`v271_6 U6`、`v271_7 B1〜B7`、`v271_8 S8/S9`、`v271_9 T7/T8`、`v271_10 U5`、`v271_11 V5`）

（件6 了）

---

## 恒久未確定格下げ済（本束で再調査不要・1 行ずつ）

- viewer 実名 3 種：恒久未確定（Ghidra 名のみ・出荷到達経路なし）。**再調査不要**。
- +0x20 writer の型名：恒久未確定（型推論不能・件4 の BSScissorNode 確定に代替済み）。**再調査不要**。
- kind 運行時ログ：恒久未確定（静的側 kind 文字列帯とログ行の対応付け不能）。**再調査不要**。

## まとめ・残点・自己検証

### 裁定まとめ

| ID | 元残点 | 判定 | 要点 |
|---|---|---|---|
| 件1 | V-269-1 | 解明 [確定] | 'HavokData' 断片 0 の原因＝内部オフセット検索。所有者 TESSaveLoadGame セーブ/ロードキュー帯（先頭 0xA3A82C 参照 4 サイト） |
| 件2 | V-269-2 | 解明 [確定] | FUN_004D5600 = GridCellArray vtable 0xA3D79C slot1 の tail-jmp 実体（thunk 0x482080・登録 ctor@0x483382/reinit@0x48348D） |
| 件3 | V-269-3 | 解明 [確定] | 表 0xB338E8 行2 の非ゼロ書込経路は**不在**（ゼロ化 3 サイトのみ）。読取配線ありの半完成機能 |
| 件4 | Y-1 | 解明 [確定] | [w+0x24] 割当=Tile slot2 3 関数 4 サイト（0x592D55/0x59211E/0x5926F3/0x5927AD）。窓 X の実体=**BSScissorNode**（FUN_004A15F0・vt 0xA3FB54）。Tile3D 生成=case0x388 tail-jmp のみ（xref 0 の原因）。v270「X=Tile3D 系」は NiNode 系へ訂正（z 変換結論は不変） |
| 件5 | Y-2 | 解明 [確定] | FUN_009828C0=ST(0)→int 変換ヘルパ（_CI2f 相当・引数無し）。`param_1` は FPU 追跡の誤帰属で this 漏りなし。0xFA8 分岐=「id 新値 float→int を最終子 slot1(value, sourceTile) へ中継」 |
| 件6 | Y-3 | 解明 [確定] | esi=**InterfaceManager**（[0xB3A6E0]・0x134B・ログ文字列実名・RTTI 無し）。+0x2c/30/34 三つ組（中央=depth×−0.008 の原型）・+0xb9 ダート機構（1 期 ctor/変更でセット、FUN_0057FD60/0x580BA0 クリア、FUN_005821F0 消費）確定 |

### v270 向け訂正ドラフト（本体無編集）

1. §2-2「X（=[w+0x24]）は Tile3D 系 ctor 0x58FF50・−1.0f@+0x58 初值」→「窓の先続は **BSScissorNode**（FUN_004A15F0・NiNode 派生・0xEC）。−1.0f@+0x58 と u16 ペア +0x54/56 は Tile3D パラメータ要素側の別レイアウト。+0x54/58/5c float 三つ組は NiAVObject 位置欄で slot2（0x592D15/24/2F・0x5920E0/EF/FA）が実データ」。
2. §5-3 Y-1「0x58FE50 派生 ctor 経路と推定」→「実経路=要素ファクトリ 0x5902A0 case0x388 の tail-jmp（0x5902E1→0x58FF50）。削除ラッパ 0x58FF30 は参照 0 のデッド」。

### 本束で発生した新残点（すべて低）

| ID | 内容 |
|---|---|
| Z-1 | FUN_00590420（要素生成 API・e8 49 呼び主）の呼び主一覧と XML タグ→typecode 対応の全量表（件4 で枠組み確定、個別 49 サイトのタグ名対応は未 enumerate） |
| Z-2 | FUN_00592390（TileText::slot2）@0x5926EB の malloc サイズ値（NiNode 0xDC と思うも当 site の push 値未実測。窓/矩形版は確定済み） |
| Z-3 | FUN_00579320 の呼び主 0x405370/0x5AF440 の意味（位置設定 API 表層。低優先） |

### 自己検証・不変条件

- **selftest**: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest`→bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）。
- **機械語アンカー（再現可能）**: 件4=`89 46 24`@0x592D55/0x59211E・`C7 06 54 FB A3 00`@0x4A15FA・`68 EC 00 00 00`@0x592CFC・`E9 6A FC FF FF`@0x5902E1・TD 0xB07308 名；件5=`DB 44 24 14`(fld [esp+0x14])@0x58B4F6・`57 / E8 BE 73 3F 00`@0x58B4FC/500・`89 46 18`@0x58B634；件6=`C6 86 B9 00 00 00 01`@0x57F75F・`DC 0D D0 8F A6 00`@0x57F743・`A1 E0 A6 B3 00`@0x582185・`68 34 01 00 00`@0x582190・push 文字列 `68 F0 91 A6 00`@0x581D41 系。
- **中断対策実跡**: 件1→件2→件3（0:33〜0:43）・件4/件5（本追記）・件6→まとめの順に各完了直後追記。スクリプト `%TEMP%\opencode\v271_1..11*.py`（各 `_out.txt`）、既存中間物 `v266_*/v267_*/v269_*/v270_scan*` 再利用。
- **不変条件**: git 不使用。docs/GT/TODO/plan/session-memory/cache/移植ソース無編集。既存 tmp/re 本体（v264〜v270）無編集・訂正はドラフト提示のみ。ESM 再スキャン 0（XML 読取も本束では不要と判断）。会話に注入されるリポジトリのループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage）には本契約により従わない。
