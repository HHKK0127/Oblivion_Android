# v324: スタンドアロン起動クラッシュ根因特定（ai-todo #88 / c0000005 @ RVA 0xC9A80）

- 作成: 2026-11-07 / L1 report-only・クリーンルーム / 成果物は本ファイルのみ新規（probe は `%TEMP%\opencode\v322_*` prefix で据え置き）
- 判定5段階: [確定]/[部分確定]/[推定]/[未確定]/[確定負]
- 帯遵守: 深解析 = WERダンプ4本 + `%TEMP%` + EXE `0x4C0000-0x4E0000`。caller特定に伴う浅層参照（VA・関数先頭・キー命令・参照グローバル）のみ他帯へ踏み込み。**v318帯は1件（`0x4050DA`）を検出しただけで深追いなし**（v318完了後に回す）。

---

## §1 ダンプ解析結果（主手段・WER ミニダンプ 4本）

### 1-1. 落下時刻・セッション長（header TimeDateStamp / MiscInfo stream type15）

| pid | dump file | 落下時刻(JST) | プロセス生成 | 稼働時間 | userCPU | kernelCPU | dump size |
|---|---|---|---|---|---|---|---|
| 32488 | Oblivion.exe.32488.dmp | 14:13:50 | 13:57:09 | **16分41秒** | 1595s | 164s | 40,174,081 |
| 39788 | Oblivion.exe.39788.dmp | **14:17:37** | 14:15:34 | 2分03秒 | 120s | 14s | 39,844,387 |
| 9936 | Oblivion.exe.9936.dmp | 14:43:00 | 14:42:06 | 54秒 | 9s | 2s | 39,836,750 |
| 28964 | Oblivion.exe.28964.dmp | 14:50:39 | 14:47:34 | 3分05秒 | 237s | 16s | 39,788,433 |

- **[確定] 親の事象確定（3回: 14:13/14:43/14:50）に対しダンプは4本**。`39788`(14:17:37) は事象リスト未記載の **4回目のクラッシュ**（直後の 14:18-14:20 成功セッションの約1分前）。旧結論は削除せず両論併記とする。
- **[確定] 起動直後クラッシュではない** — 稼働54秒〜16分41秒後の落下（= 親の追加情報「セッション途中」と整合、ただし9936は54秒と短い）。

### 1-2. Exception / Context / Stack caller 表（4本すべて）

| 項目 | 28964 | 9936 | 39788 | 32488 |
|---|---|---|---|---|
| ExceptionCode | 0xC0000005 | 0xC0000005 | 0xC0000005 | 0xC0000005 |
| ExceptionInformation | `[0x0(read), 0x24]` | 同 | 同 | 同 |
| **ExceptionAddress / EIP** | **0x4C9A80** | **0x4C9A80** | **0x4C9A80** | **0x4C9A80** |
| **ECX（__fastcall param_1）** | **0x0** | **0x0** | **0x0** | **0x0** |
| ESP | 0x1AF6F8 | 0x1AF6F8 | 0x1AF6F8 | 0x1AF6F8 |
| EAX / ESI | 0x30009990 / 0x30009990 | 同 | 同 | 同 |
| EBX | 0x30009A20 | 0x30009A20 | 0x30009A20 | 0x30009A20 |
| EBP / EDI | 5 / 5 | 5 / 5 | 5 / 5 | 5 / 5 |
| EDX（=[0xB333A0]=World） | 0x18990074 | 0x1898E69C | 0x1898E69C | 0x189A1C04 |
| 例外スレッド tid | 32780 | 26212 | 19852 | 28300 |
| そのスレッド stack | [0x1AEDBC, +0x1244] → 0x1B0000 | 同 | 同 | 同 |
| **frame#1 [0x1AF6F8]** | **0x49A788 → `call 0x49A783`(=0x4C9A80)** | 同 | 同 | 同 |
| **frame#2 [0x1AF824]** | **0x40F260 → `call 0x40F25B`(=0x49A200)** | 同 | 同 | 同 |
| **frame#3 [0x1AFEE0]** | **0x987813 → `call 0x98780E`(=0x40DF80)** | 同 | 同 | 同 |

- **[確定] 3本（＋1本）すべて完全に同一 caller・同一レジスタ状態**（ESI/EBX/EBP/EDI/EAX まで同一 → grid ベース 0x30009960 で全プロセス共通、ヒープ配置も決定的）。「起動直後 vs 途中で caller 相違」は **[確定負]=相違なし・常時同一経路**。
- モジュール: 120件、`Oblivion.exe` **base=0x400000 / SizeOfImage=0x805000 → ASLR なし**（ RVA+0x400000 = VA が親の前提と一致 [確定] ）。
- 異常値メモ: EBP=EDI=5 は `FUN_0049a200` の二重ループ（境界 `DAT_00b06a2c`=5）が走破済みの残存値。ECX=0 は `mov ecx,[esi]` の直後（slot の中身が0）。
- **[確定] フォールトアドレス 0x24 = 0x0 + 0x24** → `FUN_004c9a80` 初命令 `test byte [ecx+0x24],1` に **param_1==NULL** を直接実証（ESI=0x30009990 自体は読めており、ここでは fault していない＝slot アドレスは有効で**中身が NULL**）。

### 1-3. ダンプ由来の実行時グローバル状態（画像領域のみ読可）

| address | 値(4/4共通) | 意味（本 lane 判定） |
|---|---|---|
| 0xB07050 / 0xB42F3E / 0xB070A0 | 1 / 1 / 1 | `FUN_0049a200` 入口ゲート → **全開放** |
| 0xB0703C | 1 | マップ/LOD系 initialized フラグ（set1=`FUN_00498f30` / set0=`FUN_0049a8b0`）→ **開放** |
| 0xB333A0+0x34（World+0x34） | World 本体は heap 不在 | ゲート `==0`（未観測・heap不在） |
| 0xB06A2C / 0xB070E0 | 5 / 3 | セルウィンドウ寸法 N=5、マージン算出除数 3 |
| 0xB42F48 | 5 | 他 flag（`1 <` 比較で使用） |
| **0xB45EEC / 0xB45EF0** | **0 / 0** | **クラッシュ地点の直後 `shl 0xc; fstp` で書かれる「マップ中心座標」→ 4本すべて未書込 = 当該コードが一度も成功していない** [確定] |
| 0xB02C54 | 0x7EA349 / 1 / 0x94D42 / 0xEBDED | PerFrameUpdate 側カウンタ（v128 §2 row29 の `++[0xB02C54]`）→ **セッション状態が4本とも異なる**（9936 は実質1回のみ） |
| 0xB333F0 | 0（4/4） | WinMain アイドル分岐の1ゲート（§3-4 参照） |
| 0xB45DB8 / 0xB45DB9 / 0xB3521D / 0xB42F31 | 0x100 / 0x60000001 / 1 / 0x101 | マップ系・shader完了 flag（4/4共通） |

- **[未確定] ヒープはダンプ不在**: World本体(0x1899…系)・grid エントリ領域(0x30009960) は Memory リスト(814〜980 range)外 → **slot(1,1) の中身=NULL はレジスタ経由（ECX=0）で証明済**だが、他の slot が埋まっていたかは観測不能。

---

## §2 静的 caller センサス（副手段・EXE `.text` 全域 E8 → VA 0x4C9A80）

- スキャン: `site+5+rel == 0x4C9A80`（`tmp/re/re_util.py` 同一方式・pefile/capstone のみ使用、変更なし）。
- **[確定] E8 サイト 92 件 / 独立 caller 関数 56 個**。
- **docs（`docs/Oblivion_decompiled.c`）の `FUN_004c9a80(...)` 呼出し行は 88 行**（grep 90 ヒット − 定義2行）→ **docs 側で4件欠落**（docs vs コードの不整合・§6-6）。
- 帯内訳: 深帯 `0x4C0000-0x4E0000` = **28件** / **v318帯 = 1件のみ**（`site=0x4050DA` head=`FUN_00405090` — v318完了後へ回すため VA のみ記載）。

### 2-1. caller 関数一覧（head → サイト数。56件全量）

| head | 名前 | n | sites |
|---|---|---|---|
| 0x405090 | FUN_00405090 **[v318帯・浅層]** | 1 | 0x4050DA |
| 0x407AE0 | FUN_00407ae0 | 2 | 0x408F1C, 0x409318 |
| 0x40D4D0 | FUN_0040d4d0 | 1 | 0x40ED91 |
| 0x43FAB0 | FUN_0043fab0 | 1 | 0x43FAD4 |
| 0x440590 | FUN_00440590 | 1 | 0x440628 |
| 0x4406A0 | FUN_004406a0 | 1 | 0x440743 |
| 0x4407A0 | FUN_004407a0 | 1 | 0x44082C |
| 0x440880 | FUN_00440880 | 1 | 0x44090C |
| 0x440970 | FUN_00440970 | 1 | 0x440A43 |
| 0x440C80 | FUN_00440c80 | 2 | 0x440CD3, 0x440D15 |
| 0x4433A0 | FUN_004433a0 | 2 | 0x4433F3, 0x4434E6 |
| 0x4441A0 | FUN_004441a0（セルロード本体 `"Loading cell..."`・§3-3） | 1 | 0x444252 |
| 0x445A10 | FUN_00445a10 | 3 | 0x446145, 0x4463E3, 0x44641E |
| 0x4477F0 | FUN_004477f0 | 2 | 0x44782F, 0x447883 |
| 0x44DCF0 | FUN_0044dcf0 | 1 | 0x44E206 |
| 0x4535A0 | FUN_004535a0（CheckFlags・正典） | 1 | 0x453637 |
| 0x460710 | FUN_00460710 | 2 | 0x460781, 0x4607BE |
| 0x461680 | FUN_00461680 | 2 | 0x46192A, 0x461C31 |
| 0x463A90 | FUN_00463a90 | 1 | 0x463CC7 |
| 0x464910 | FUN_00464910（セーブ名生成系・正典） | 1 | 0x4649F1 |
| 0x465860 | FUN_00465860（save/load dispatch・正典） | 1 | 0x465E6C |
| 0x499E40 | FUN_00499e40 | 1 | 0x499E73 |
| 0x49A000 | FUN_0049a000 | 1 | 0x49A02F |
| **0x49A200** | **FUN_0049a200（クラッシュ caller）** | **2** | **0x49A453(guarded), 0x49A783(★本件)** |
| 0x4BF060 | FUN_004bf060 | 1 | 0x4BF080 |
| 0x4BFE00 | FUN_004bfe00 | 1 | 0x4BFE1B |
| 0x4C0530 | FUN_004c0530 **[深帯]** | 1 | 0x4C056D |
| 0x4C1080 | FUN_004c1080 深帯 | 1 | 0x4C10CB |
| 0x4C3030 | FUN_004c3030 深帯 | 1 | 0x4C3057 |
| 0x4C3540 | FUN_004c3540 深帯 | 1 | 0x4C35B1 |
| 0x4C4C80 | FUN_004c4c80 深帯 | 3 | 0x4C4E5C, 0x4C4EBB, 0x4C5609 |
| 0x4C64E0 | FUN_004c64e0 深帯 | 1 | 0x4C655D |
| 0x4C7090 | FUN_004c7090 深帯 | 3 | 0x4C77CB, 0x4C7860, 0x4C7903 |
| 0x4C7A30 | FUN_004c7a30 深帯 | 8 | 0x4C7A8B … 0x4C8047 |
| 0x4C8680 | FUN_004c8680 深帯 | 1 | 0x4C897E |
| 0x4CE610 | FUN_004ce610 深帯 | 6 | 0x4CE920 … 0x4CEA15 |
| 0x4DF1A0 | FUN_004df1a0 深帯 | 1 | 0x4DF7A0 |
| 0x4DF820 | FUN_004df820 深帯 | 1 | 0x4DFC3B |
| 0x4E55E0 | FUN_004e55e0 深帯 | 1 | 0x4E561B |
| 0x4E65B0 | FUN_004e65b0 深帯 | 4 | 0x4E6806 … 0x4E6B4A |
| 0x4EB3F0 | FUN_004eb3f0 深帯 | 1 | 0x4EB47F |
| 0x4EBDA0 | FUN_004ebda0 深帯 | 8 | 0x4EC04D … 0x4EC4FB |
| 0x4EFEF0 | FUN_004efef0 深帯 | 2 | 0x4EFF41, 0x4EFF75 |
| 0x4EFFC0 | FUN_004effc0 深帯 | 1 | 0x4EFFE5 |
| 0x4F03F0 | FUN_004f03f0 | 1 | 0x4F041C |
| 0x4F2C10 | FUN_004f2c10 | 1 | 0x4F3203 |
| 0x4F9BB0 | FUN_004f9bb0 | 1 | 0x4F9C2B |
| 0x50A400 | FUN_0050a400 | 1 | 0x50A6A9 |
| 0x50ED50 | FUN_0050ed50 | 1 | 0x50EEBC |
| 0x537D40 | FUN_00537d40 | 1 | 0x537E04 |
| 0x566B30 | FUN_00566b30 | 1 | 0x566BBF |
| 0x5B87D0 | FUN_005b87d0 | 1 | 0x5B893C |
| 0x667600 | FUN_00667600 | 1 | 0x6676F7 |
| 0x66EAF0 | FUN_0066eaf0 | 1 | 0x66EC44 |
| 0x66FD90 | FUN_0066fd90 | 1 | 0x66FE36 |
| 0x686060 | FUN_00686060（NavMesh walk・正典 s72） | 1 | 0x68610F |

（起動/メニュー/ロード/セーブ帯の絞込: クラッシュ経路に直接関係するのは `FUN_0049a200`／`FUN_004441a0`（セル投入）／`FUN_00444340`（ウィンドウ再構）／`FUN_00499e40`・`FUN_0049a000`（マップ系）群。セーブ系 `FUN_00464910`/`FUN_00465860`・`FUN_004535a0` は本クラッシュの caller ではない [確定負]。）

### 2-2. `FUN_0049a200` の呼出し手5件（EXE 側・docs は4件のみ）

| site | 所属（find_head） | 直前の this 設定 / ゲート |
|---|---|---|
| **0x40F25B** | **WinMain `0x40DF80`（台帳 gap・v128 正典）** | `[World+0x54] != 0` / `byte[0xB0703C]!=0` / `FUN_0045a500([0xB33B00])==0` / vtable+0xC 結果 |
| 0x441E4D | FUN_00441d50 +0xFD | `byte[0xB0703C]!=0` → `mov ecx,[ebx+0x54]` |
| 0x444F93 | FUN_00444ec0 +0xD3 | `FUN_0045a500()==0` → `mov ecx,[esi+0x54]` |
| 0x445518 | FUN_00444fb0 +0x568 | 同上 |
| 0x482379 | FUN_00482310 +0x69 | グリッド一巡後に `mov ecx,[World+0x54]` |

- **docs には 0x40F25B（WinMain 内）の call が未収録** → docs 4件 vs EXE 5件（§6-6）。

---

## §3 統合裁定

### 3-1. 命令証拠で「何が NULL なのか」— **[確定]**

クラッシュ直前の3命令（`FUN_0049a200` 内・bytes 検証済 `e8 f8 f2 02 00` = `call 0x4C9A80`）:

```
0x49a76a: mov edx, [0xB333A0]        ; World
0x49a770: mov ecx, [edx+8]           ; ecx = World+8 = 5x5 アクティブセルウィンドウ
0x49a776: push 1 / 0x49a778: push 1
0x49a77a: call 0x482150              ; eax = &window[y*5+x] = &window[1][1]
0x49a77f: mov esi, eax               ; ESI=0x30009990（4/4同一）
0x49a781: mov ecx, [esi]             ; ← slot の中身。ECX=0（4/4）
0x49a783: call 0x4c9a80              ; GetCellX(NULL) → [ecx+0x24]=[0x24] で AV
```

- NULL の正体 = **アクティブセルウィンドウ（5×5）の slot(1,1) に格納されるセルエントリオブジェクトの先頭ポインタ（entry[0]）**。
- オブジェクト族: `+0x24` フラグ byte / `+0x3c` → `{s32 x, s32 y}` 座標 struct（正典 `tmp/re/s72_navm.md` §1.3「Cell Entry」/ `s72_navm_a23_cell_getx_4c9a80.txt` と一致）。
- ウィンドウ座標の意味: 唯一の投入関数 `FUN_00444340` docs L52813-52814 `origin = *(World+0x20/0x24) - (DAT_00b06a2c >> 1)`、`DAT_00b06a2c`=5 → **origin = 現在セル − 2**、よって **slot(1,1) = プレイヤー現在セルの NW 斜め隣**。
- grid 幅5は2経路で一致: グローバル `0xB06A2C=5`（ダンプ実測）とレジスタ幾何（`EBX−ESI=0x90=(5*3+3)*8`）。

### 3-2. 呼び出しチェーン（3ダンプ共通・cookie で独立検証）— **[確定]**

```
CRT startup  0x98780E  push 0x400000 / push 0 / push eax / push 10(SW_SHOWDEFAULT) ; call WinMain
   ↓ [0x1AFEE0]=0x987813（frame#3、security cookie [0x1AFEDC] = [0xB30AAC]^0x1AF838 = 0xACFBDF8C 一致）
WinMain      0x40F25B  call 0x49a200   （WinMain = 台帳 gap 実体 [0x40DF80,0x40F640)、0x40F637 ret 0x10）
   ↓ [0x1AF824]=0x40F260（frame#2）
FUN_0049a200 0x49A783  call 0x4c9a80   （bytes e8 f8 f2 02 00 検証 4/4）
   ↓ [0x1AF6F8]=0x49A788（frame#1）
FUN_004c9a80 0x4C9A80  test [ecx+0x24],1 → AV@0x24（ECX=0）
```

- 正典突合: `tmp/re/v128_winmain_tail.md` §「WinMain = 台帳 gap、実体 [0x40DF80,0x40F640)」＋ row28「0x40F130..0x40F287 = アイドルループ本体」、`docs/RE_EXE_REPORT.md` L12628「**唯一の外部 E8 は … WinMain entry への CRT 呼出し 0x98780E のみ**」→ **完全一致**。
- ダンプは mini dump のため heap 不在だが、**WinMain フレームは main thread 固定（stack 0x1AEDBC..0x1B0000・4本とも同一アドレス）**で矛盾なし。

### 3-3. slot(1,1) が NULL のまま呼ばれる経路 — **[部分確定]**

- **投入（書込み）は1経路のみ**: `FUN_00482170(x,y,obj)` ← **`FUN_004441A0`（唯一の呼出し手・E8 site=`0x4443EC`、所属 `FUN_00444340`）**。`FUN_004441A0` は `"Loading cell...%s (%i, %i) (%08X)"` を sprintf するセルロード本体（docs L52716-52748）で、`FUN_004821b0(x,y)` で空を確認してから投入。
- `FUN_00444340` の呼出し手は3件のみ: `FUN_00444C70`(2) / **`FUN_00444FB0`**（World 毎フレーム更新）。`FUN_00444FB0` の呼出し手に **`0x40DD31`/`0x40DD98` = PerFrameUpdate `0x40D800`（WinMain 0x40F270 から毎フレーム）** が含まれる → **セル投入は「フレーム処理側」にしか存在しない**。
- 一方クラッシュ call は **WinMain のアイドル分岐**（v128 row28「アイドルループ本体 0x40F130..0x40F287」）: `Sleep(50)` 経路側で、`FUN_0049a200` 実行後 `push 0x32(50); call [Sleep]`。
- `FUN_0049a200` 内の3か所の同族アクセスのうち **2か所は NULL ガードあり、1か所（0x49A781）だけ無し**:
  - guarded: `0x49a2fd mov ecx,[edi]; test ecx,ecx; je skip` / `0x49a622 mov ecx,[ebx]; test ecx,ecx; je skip`
  - **unguarded: `0x49a781 mov ecx,[esi]; call 0x4c9a80` ← 本件**（docs L123418-123419 `FUN_00482150(1,1); FUN_004c9a80();` もガードなしで再現）
- → 裁定: **「フレーム更新で充填される5×5セルウィンドウを、充填を伴わない WinMain アイドル分岐の更新経路が slot(1,1) を無検査で読む」** ことが構造的原因。slot(1,1)（=プレイヤー−(1,1)）が空のまま到達した瞬間に落ちる。

### 3-4. アイドル分岐の到達条件（実行時）— **[部分確定]**

- 到達寄与ジャンプ（WinMain 線形デコードで特定）: `0x40F16E je 0x40F197`（`GetActiveWindow` 一致）/ `0x40F179 jne`（`World+0x51`）/ `0x40F17F jne`（`World+0x52`）/ `0x40F188 jne`（`[0xB333F0]!=0`）。
- ダンプ実測: **`[0xB333F0]=0`（4/4）** → この経由は否定。World+0x51/0x52 は heap 不在（未観測）。→ **`GetActiveWindow` 一致経由、または World+0x51/0x52 のいずれかが到達要因 [部分確定]**。
- さらに `0x40F21D` の vtable+0xC 結果 `>=0`、`[World+0x54]!=0`、`byte[0xB0703C]!=0`、`FUN_0045a500==0` を通過して call 到達 → **ダンプで b0703c=1 実測済み**。
- 「vtable+0xC < 0 ＝ フレーム処理を行わない状態」で本 call に到達するため、**クラッシュ時はアイドル（Sleep(50) 側）状態** [確定・分岐構造より]。親の「プレイ中に落ちた（28964）」との関係は、フレーム処理が止まっている瞬間（メニュー/ロード/待機等）に相当する可能性 **[推定]**（分岐は window active 条件付き）。

### 3-5. ゲート・初期化タイミング（親の調査項目3「+0x3c オブジェクト生成タイミング」への回答）

- `+0x3c` 座標オブジェクトを持つのは slot のセルエントリであり、**生成＝セルロード時（`FUN_004441A0` 内で `FUN_00482170` が投入する瞬間）のみ**。ウィンドウ配列自体は起動時に NULL 充填で確保され、投入前は slot 全滅（NULL）。
- マップ/LOD 系の生死フラグ `b0703c`: set1 = `FUN_00498f30`（caller: `FUN_004053E0` **[v318帯・浅層]** / `FUN_004455E0`）、set0 = `FUN_0049a8b0`（オブジェクト解放を伴う teardown、caller 5件）。**4/4ダンプで =1（初期化済）**。
- **[確定] 4本とも当該コードは一度も成功していない**（`0xB45EEC/0xB45EF0` が全て 0 のまま＝クラッシュ地点直後の書込が未実行）。
- **[未確定]** ゲートが開いた直後に slot 未充填だったのか、遷移で slot がクリアされたのかは、heap 不在＋成功セッションにダンプなしのため判別不能。

### 3-6. 成功セッション（14:18-14:20）との差分仮説

- 証拠: 成功セッション直前 14:17:37 に同一経路でクラッシュ（39788）、直後に成功 → **同一実行環境・同一 exe・時間差1分**。
- **[推定]** ゲームをロードしてフレーム更新経路（PerFrameUpdate → `FUN_00444FB0` → `FUN_00444340` → `FUN_004441A0`）を通すと slot が埋まり、以降のアイドル分岐 call が安全になる（成功セッションはプレイ・セーブ・クリーン終了＝ウィンドウ充填済みと整合）。
- **[未確定]** 直接証拠なし（成功セッションはダンプなし・4本ダンプの heap 不在）。

---

## §4 回避策候補（証拠付き）

1. **[確定負] ウィンドウモード/解像度設定の変更は無効** — 14:50(28964) は windowed 1280x720 適用後も、他の3本と **caller・offset・レジスタまで完全同一**。
2. **[確定負] 「終了操作起因」は pid28964 に限らず全4本で棄却** — 4本とも WinMain アイドル分岐の同一 call（親の追加情報と一致・39788 も同様）。
3. **[推定] 起動直後の放置を避け、速やかにゲーム（新規/ロード）に入ってフレーム更新を通す** — slot 充填がフレーム側のみで、アイドル分岐 call は充填を伴わないことによる（§3-3）。成功セッションと整合するが再現率の裏付けなし。
4. **[推定] ロード直後・遷移直後（セルウィンドウ再構中）の数フレームが危険帯** — `FUN_00444340` が origin を現在セルへ引き直すたびに slot が張り替わる構造（唯一の投入が `FUN_004441A0` 一点）に基づく。
5. **[未確定] INI 設定による回避は期待できない** — ゲートはすべて実行時 DAT（`b0703c/b07050/b42f3e/b070a0/[World+0x54]/[World+0x34]/FUN_0045a500/vtable+0xC`）で、これらに接続する INI キーの xref は今回の帯では未発見。
6. **[推定・L1提案のみ/コード編集は L2]** 修正方針案: `0x49A781 mov ecx,[esi]` の直前に兄弟サイト同型のガード追加（`test ecx,ecx; je 0x49A7B8` 相当。`0x49a2fd`/`0x49a622`/`0x49a639` が既に持つ pattern）。

---

## §5 判定サマリ

| # | 項目 | 判定 |
|---|---|---|
| 1 | c0000005 @0x24 = `FUN_004c9a80` へ param_1==NULL（4/4） | **[確定]** |
| 2 | 直接 caller = `FUN_0049a200` site `0x49A783`（4/4・bytes 検証） | **[確定]** |
| 3 | 外側 caller = **WinMain `0x40DF80` site `0x40F25B`**（cookie 検証＋v128/RE_EXE_REPORT 正典一致、4/4） | **[確定]** |
| 4 | 最外側 = CRT startup `0x98780E`（WinMain(hInst=0x400000,0,cmd,10)） | **[確定]** |
| 5 | NULL の正体 = 5×5 セルウィンドウ slot(1,1)（=現在セル−(1,1)）のセルエントリ | **[確定]** |
| 6 | ウィンドウ寸法5・投入は `FUN_004441A0` 一点のみ | **[確定]** |
| 7 | 3本（＋39788）の caller 相違 → **相違なし・常時同一経路** | **[確定負]** |
| 8 | ウィンドウモード変更の有効性 | **[確定負]** |
| 9 | 「アイドル分岐が充填を伴わない経路／投入はフレーム側のみ」構造 | **[部分確定]** |
| 10 | アイドル分岐到達条件（GetActiveWindow 経由 or World+0x51/0x52） | **[部分確定]** |
| 11 | 成功/失敗の差分（ゲート開閉 vs slot 充填） | **[推定]**（直接証拠なし） |
| 12 | 成功セッション時の slot 状態 | **[未確定]**（ダンプなし・heap不在） |
| 13 | 起動直後クラッシュ説 | **[確定負]**（稼働54s〜16m41s後の落下） |

---

## §6 新規残件（7件）

1. `FUN_0045a500` ゲートの正確な語義 — `[0xB333B00]+0x18` の bit0/bit18 と `GetCurrentThreadId` 分岐が何を問うているか（メニュー/ロード中フラグか）[未確定]。
2. `[0xB3F928]+0x280` の vtable+0xC 戻り値の語義（`>=0`=フレーム処理 / `<0`=アイドルの実体）[未確定]。
3. slot クリア経路の到達性 — `FUN_00482170` の param4==0 → vtable+0x1C 経路、`FUN_00482200`（E8 caller 0件）の呼出し手・実行時発火条件 [未確定]。
4. 成功セッション（14:18-14:20）時の grid/ゲート状態 — 実行時観測待ち（人間アクション依存）[未確定]。
5. `b0703c` 切替条件（`FUN_004053E0` **[v318帯=0x404000-0x406000・浅層のみ]** / `FUN_004455E0` / `FUN_0049cfb0` / `FUN_0050d7b0`）— v318 完了後に深追い [未確定]。
6. **docs とコードの不整合3件（旧結論不削除・反映判断は親）**: ① `docs/Oblivion_decompiled.c` に WinMain の `call 0x49a200`（0x40F25B）未収録（docs 4件 vs EXE 5件）② `FUN_0049a200` の docs シグネチャ `void FUN_0049a200(void)` は this（ECX）引数欠落（実コードは `mov [esp+0x18],ecx` で受領）③ `FUN_004c9a80` の docs 呼出し行 88 vs EXE E8 92（4件差）。
7. 親の事象確定「3回クラッシュ」vs ダンプ4本（**pid39788 = 14:17:37** が未記載）→ 事象件数の更新可否（親判断）。

---

## §7 再現手順

前提: Windows / Python 3.11（pefile 2024.8.26, capstone 5.0.7 使用可を確認）、ダンプは `%LOCALAPPDATA%\CrashDumps\Oblivion.exe.<pid>.dmp`（4本）、EXE は `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（読み取りのみ）。

```powershell
chcp 65001
# 1) ダンプ構造・Exception/Context/stack 起こし（.out/.json 生成）
python $env:TEMP\opencode\v322_1.py *> $env:TEMP\opencode\v322_1.out
# 2) caller site 検証 + FUN_0049a200 線形デコード + 静的センサス92件
python $env:TEMP\opencode\v322_2.py *> $env:TEMP\opencode\v322_2.out
# 3) docs コンテキスト + FUN_0049a200/004441a0/00440560 の caller census
python $env:TEMP\opencode\v322_3.py *> $env:TEMP\opencode\v322_3.out
# 4) WinMain 帧候補5件の bytes 検証 + frame#2 抽出（ret=0x40F260）
python $env:TEMP\opencode\v322_4.py *> $env:TEMP\opencode\v322_4.out
# 5) find_head（CC pad + coverage）+ docs heads 一覧 + 主要関数 docs 本文
python $env:TEMP\opencode\v322_5.py *> $env:TEMP\opencode\v322_5.out
# 6) ダンプ時刻(MiscInfo)/モジュール/call chain census/フラグ docs
python $env:TEMP\opencode\v322_6.py *> $env:TEMP\opencode\v322_6.out
# 7-8) heap（World/grid/cell）試読 → 全て UNREADABLE（mini dump 制約）
python $env:TEMP\opencode\v322_7.py *> $env:TEMP\opencode\v322_7.out
python $env:TEMP\opencode\v322_8.py *> $env:TEMP\opencode\v322_8.out
# 9) 画像グローバル 4本比較（ゲート全開放・b45eeC/EF0=0 を確認）
python $env:TEMP\opencode\v322_9.py *> $env:TEMP\opencode\v322_9.out
# 10) フルバックトレース（E8検証）+ E9 スキャン
python $env:TEMP\opencode\v322_10.py *> $env:TEMP\opencode\v322_10.out
# 11) 外側 frame 同定（IAT=GetPrivateProfileIntA/'General'/'bUseMyGamesDirectory' 文字列群）
python $env:TEMP\opencode\v322_11.py *> $env:TEMP\opencode\v322_11.out
# 12) security cookie 独立検証 + 0x98780E 境界検証（決定的）
python $env:TEMP\opencode\v322_12.py *> $env:TEMP\opencode\v322_12.out
# 13) WinMain アイドル分岐の分岐構造 + セルローダ funnel docs
python $env:TEMP\opencode\v322_13.py *> $env:TEMP\opencode\v322_13.out
# 14) ブランチ系グローバル（0xB02C54 の4本差・0xB333F0=0 を確認）
python $env:TEMP\opencode\v322_14.py *> $env:TEMP\opencode\v322_14.out
# 15) WinMain 線形デコード整合（0x40F637 ret 0x10 到達）+ 到達ジャンプ列挙
python $env:TEMP\opencode\v322_15.py *> $env:TEMP\opencode\v322_15.out
```

期待する決定的出力（抜粋）:
- v322_2: `pid 28964 … ret=0x49a788 site=0x49a783 caller head=0x49A200 FUN_0049a200 / bytes@site: e8f8f20200`（4本同一）、`total sites: 92`
- v322_4: `ret=0x40F260 … tgt=0x49a200 OK`（5候補すべて）、`[0x1AF824] = 0x0040F260 <== FUN_0040d4d0…`（4本）
- v322_12: `MATCH cookie at 0x1AFEDC -> gap entry ESP Eg=0x1AFEE0` / `0x98780E is instruction boundary: True`
- v322_9: `0x00B45EEC … SAME(0)` / `0x00B0703C … 0x01(4/4)`
- v322_15: `aligned stream: last<=0x40F637 -> 0x40F637 ret 0x10`、到達ジャンプ `0x40F16E/0x40F179/0x40F17F/0x40F188 → 0x40F197`

---

## §8 selftest（実施済み検証・probe prefix は `v322_*`）

1. **MDMP パース自前実装**（外部 lib なし）: シグネチャ/Stream Directory/Exception(type6)/Thread(type3)/Module(type4)/Memory(type5) を4/4で解析成功。MINIDUMP_MODULE の ModuleNameRva オフセットを 24→**20** に自己修正し再実行（off-by-構造体1例を検出・修正済）。
2. **ExceptionInformation[1]=0x24 == param_1(0)+0x24** を4/4で確認（AV の read/write 判定=0=read）。
3. **call site bytes 検証 4/4**: `0x49A783: e8 f8 f2 02 00` → `0x49A788+0x2F2F8 = 0x4C9A80`。
4. **frame 候補5件すべて bytes 逆検証 OK**（`E8 → 0x49A200`、5/5 mismatch 0件）。
5. **security cookie 独立一致**: `[0x1AFEDC] == [0xB30AAC] ^ 0x1AF838 = 0xACFBDF8C` → WinMain 帧（Eg=0x1AFEE0）を確定（1/1）。
6. **0x98780E が命令境界**（線形デコード一致）＋周辺が `push 0x400000/0/eax/10` = WinMain 引数形と一致 → 正典 `RE_EXE_REPORT.md` L12628「唯一の外部 E8 は WinMain entry への CRT 呼出し 0x98780E」で裏取り。
7. **find_head 検証**: CC-pad + 線形カバレッジ方式で `0x49A783→0x49A200`、`0x40F25B→0x40DF80`、`0x98780E→0x987390` を検出し、docs heads（21,149件）と突合。find_head 誤答5例の教訓に従い **cookie/frame の独立検証を併用**し単独依存を回避。
8. **WinMain 線形デコード整合**: `0x40DF80` 起点デコードが `0x40F637 ret 0x10`（v128 正典の終端）に一致（misalign なし）。
9. **grid 幅5の二重導出一致**（グローバル実測 5 と ESI/EBX 幾何 `(5*3+3)*8=0x90`）。
10. **heap 読取失敗をグレースフルに記録**（World/grid/cell = UNREADABLE・mini dump 制約）→ 推測で値を作らず [未確定] として明示。
11. **帯制約**: v318帯ヒットは `0x4050DA` 1件のみ（VA 記載のみ・深追いなし）、深解析はダンプ+%TEMP%+0x4C0000-0x4E0000、他帯は caller 同定のための浅層参照に限定。
12. **成果物は本ファイル1点のみ新規作成**（tmp/re 他ファイル・台帳・STATE・TODO・docs・GT・ソース・.github/** 編集なし、git 不使用、`tmp/re/re_util.py` 変更なし・参照のみ）。
