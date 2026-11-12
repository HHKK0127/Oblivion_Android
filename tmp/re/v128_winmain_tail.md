# V128: WinMain 後半 0x40EB00-0x40F3EA の初期化順序・早期 return 3 本・B43104 再 mirror 欠落の検証

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / SHA256 A8F31384…B7DC6）。
出典: `tmp/re/v90_b350d8_triangle.md` §5-4/§5-7/§5-8、`tmp/re/v93_gmst_gt_residuals.md` 残点3、`TODO_re49.md` serial 128（追記(5)含む）。

## §1 手法 / 入力

- ハーネス: `%TEMP%\opencode\probe_harness.py`（`selftest` 4/4 PASS を冒頭で確認）。UTF-8 は pwsh 先頭 `chcp 65001` + スクリプト先頭 `sys.stdout.reconfigure(encoding="utf-8")` の2段。
- 入力:
  - バイナリ（capstone 5.0.7 / skipdata / 手動セクションパース、.text const 0x400C00）
  - `docs/Oblivion_decompiled.c`（1,066,496 行、`python enumerate(f,1)` の部分検索のみ）
  - `tmp/re/cache/call_targets.json`（13,946 ターゲット / 137,421 呼出し）、`tmp/re/cache/named_funcs.txt`（21,597 行台帳）
- 手順:
  1. WinMain 0x40DF80..0x40F640 の線形デコード + CFG（分岐全列挙）で 0x40EB00-0x40F3EA の call 列を抽出し、各 callee を台帳で同定。
  2. 早期 return 3 本（0x40E2C6 / 0x40E37B / 0x40E3AC）の支配条件（前方の cmp/test とその直近定数）をデコードで特定。
  3. B43104 / フラグ 0xB42F31 の全参照を全セクション走査し、再 mirror 書込の有無と FUN_007BA2F0 経路の使用回数を判定。
  4. FUN_0076BED0 の参照形（E8 0 件の裏取り → 間接参照の所在）を全セクションで検出。
  5. FUN_00405CE0 → FUN_00405090 の 6 分岐定数（0xA2FE7C/0xA2FE78/0xA7DEB4）と内部 callee FUN_0043FD20/FUN_004D1A90/FUN_0043FC20 をデコード + 定数読出で解釈（v90 残5 の包含）。
- 中間スクリプト: `%TEMP%\opencode\v128_*.py` のみ（`v128_dump / v128_cfg / v128_iat / v128_b43104 / v128_sweep / v128_scan / v128_callers / v128_misc / v128_scope / v128_profile / v128_range / v128_rd / v128_final / v128_gt / v128_g / v128_verify`）。tmp/re への追加は本ノート 1 件のみ。

---

## §2 WinMain 後半 0x40EB00-0x40F3EA の call 列（RT 群生成以降の起動順序）

### 2-1. 領域の CFG 事実

- WinMain = **台帳 gap**（`0x40d4d0` → 次 `0x40f640` に開始点0件）、実体 **[0x40DF80, 0x40F640) = 5,816 B**、`0x40F637: ret 0x10` 終端。
- 線形デコード **1,536 命令 / 到達 1,445 / 到達不能 91**（境界外転移 0）。→ v90 の CFG 数値（1,536/1,445）と**完全一致**。
- 到達不能レンジ: `0x40E234..0x40E2CB`（151 B）、`0x40E2D8..0x40E393`（187 B）、整列パディング 7+7+3 B、末尾 int3 6 B（詳細は §3）。
- エピローグ `0x40F61D..0x40F637` への入辺は **4 本**（`0x40E2C6 jmp` / `0x40E37B je` / `0x40E3AC jmp` / `0x40F618 fall-through`）＝ v93 §b-3 の「外部分岐4本」と一致。

### 2-2. RT 群生成以降の到達可能呼び出し順序

`0x40EAAC call 0x406950`（mirror → shader 初期化 → RT 群生成）直後から、次の順で実行される（すべて到達可能経路、条件のないものは無条件）。

| # | VA | 呼び出し/操作 | 証拠・意味 |
|---|---|---|---|
| 1 | 0x40EAAC | `call 0x406950` | E8=1（唯一の caller）。内部: mirror `0x406A56` → `FUN_007BA0F0(0x406A83)`（`0x7BA2D1: [0xB42F31]=1`）→ `FUN_007C2450(0x406B4E)` で RT 群生成 |
| 2 | 0x40EAB1-0x40EAC8 | `[app+0x20]` の NiPointer 解放 | `FUN_00403160`（InterlockedDecrement 相当）+ `FUN_004046A0`、`ecx=0xB33E90 → FUN_0047D0F0`、`[app+2]=1` |
| 3 | 0x40EADB / 0x40EAEA | `FUN_00410310`（E8=1）/ `FUN_00410E40(-1,0,[0xB03084])`（E8=1） | 起動時に各1回のみの初期化 |
| 4 | 0x40EAF4 / 0x40EB06 | `[0xB39B84]`,`[0xB39E00] ← [([0xB333CC])+0xDC]` | 転送先オブジェクトのキャッシュ |
| 5 | 0x40EB0C | `call 0x67FCF0` | E8=2（もう1件は `FUN_00464440` 内）。`[0xB3BE00]` を書く |
| 6 | 0x40EB11 | `call 0x683030` | E8=**1 = WinMain のこの1箇所のみ**。`[0xB3BF80]` を書く |
| 7 | 0x40EB18 | `push 1; call 0x578CC0` | `FUN_00578cc0 → FUN_00582160(1,1)` |
| 8 | 0x40EB2C | `call 0x406360(ecx=[app], arg=[0xB333CC])` | E8=1 |
| 9 | 0x40EB32 | `push 0; call 0x55F750` | E8=29（ログ/入力系ヘルパ） |
| 10 | 0x40EB62 | `call 0x4431F0(ecx=[0xB333A0], arg=[0xB33A98+0xC])` + float `0xA2FF44`=2048.0f, `0xA2FFE8`=128.0f | `FUN_004431f0(this,v)`: `if (v && this[+0x74]!=v) { FUN_00977c50(3); … }` → **現在空間/セル ptr を this+0x74 へ切替**。`[0xB33A30]` 也を書く |
| 11 | 0x40EB6D | `call 0x662EE0(ecx=[0xB333C4])` | E8=1。内部 `_strrchr/FUN_005215C0/FUN_00435830/FUN_0043BDA0` |
| 12 | 0x40EB72 | `call 0x42C970()` | E8=1、内部は `FUN_00401f20`(free) のみ＝作業バッファ後始末 |
| 13 | 0x40EB86 | ログ `"Initializing Actor Locations..."` | `FUN_00404EE0`（E8=29、コンソール出力） |
| 14 | 0x40EB9F | `call 0x675310(ecx=0xB3BD00)` | E8=3。内部 `FUN_007616D0/FUN_004F9EC0/FUN_005ED860/FUN_005E6680/FUN_0067B320` |
| 15 | 0x40EBAA | `call 0x447300(ecx=[0xB33A98])` | E8=5。`FUN_004b2460 → FUN_009832e6` のリスト走査 + `FUN_005222d0(0)` = **アクター配置の走査系** |
| 16 | 0x40EBAF | ログ `"Loading initial area..."` | |
| 17 | 0x40EBB9-0x40EE78 | 開始セル解決ブロック | INI 由来文字列 `[0xB02CE0]/[0xB02CF8]/[0xB02D00]/[0xB02CF0]` の長さ判定 → 既定では**すべて共有空文字 0xA2F7EC** のため、`Could not find starting cell …` 系エラーは**不発**、`0x40EE7A` へ直行。非空時のみ `FUN_004476B0/FUN_004478B0/FUN_00447740/FUN_004F1630` → 見つかったら `FUN_004455E0(0x40ED72)`（内部 `0x4456E5` で `FUN_007B84E0` 呼出） |
| 18 | 0x40EE7A | ログ `"Placing player..."` | |
| 19 | 0x40EE7F-0x40EF54 | `FUN_00482150`→`FUN_004D5D70(esi)` / vtable+0x1e8・+0x1cc / `FUN_004D35D0` / `FUN_00434020(5)` / `FUN_00660110(0)`→`FUN_00707610` / `FUN_00660110(0)`→`FUN_007B4280`→`FUN_007C6C30` / `FUN_004D70E0` | プレイヤー座標・カメラ初期化 |
| 20 | 0x40EF8E-0x40EFE9 | worldspace セルの +0x54..+0x5c へ座標書込、`FUN_004D7AF0` の 9 dword を `[cell+0x30]` へ memcpy、`FUN_00707370` | |
| 21 | 0x40EFFA | `push 0; call 0x5E02E0(ecx=[0xB333C4])` | `FUN_005e02e0(this,ch)`: `ch!=0` 時のみ vtable+0x128 を呼ぶ。戻り値 +0x58 のリストを `FUN_00415360`（型==0x19 判定）で走査 → 一致時 `FUN_00664850`/`FUN_00664700` |
| 22 | 0x40F050 | `call 0x6632A0(ecx=[0xB333C4])` | **台帳 gap**（`FUN_00663070` の範囲内・ret+int3 で分離、`// FUN_006632a0` ヘッダなし）。内部: `FUN_004A7A60/FUN_004533F0/FUN_00453500/FUN_0046AC70` |
| 23 | 0x40F055 | `[app+3] = 1` | 「初期化完了」フラグ |
| 24 | 0x40F05E | `call 0x42BA50` | E8=1 |
| 25 | 0x40F069 | `call 0x6ECC80(ecx=[0xB333C4])`（E8=715 = 全体最頻） | NULL（＝プレイヤー未取得）なら代替初期化13連呼 `FUN_00447DB0(-1)/FUN_00440AF0(0,0,0)/FUN_005B6040/FUN_0040FDD0/FUN_007D71C0(7)/FUN_007D7210/FUN_00977C50(0)/FUN_005B5AC0/FUN_00410BA0/FUN_005B5C90/[0xB33430]=0/FUN_00977C50(1)`、非 NULL なら `0x40F0DB`（`FUN_0040FDD0` のみ） |
| 26 | 0x40F0E0 | ログ `"Begin Idle loop..."` | |
| 27 | 0x40F0EA-0x40F12F | メッセージループ構築 | IAT=`PeekMessageA(0xA2826C)/TranslateMessage(0xA28268)/DispatchMessageA(0xA28264)`、`[esp+0x10] ← [app+8]`（ウィンドウハンドル保存） |
| 28 | 0x40F130..0x40F287 | アイドルループ本体 | `PeekMessageA(&msg,0,0,0,PM_REMOVE)` → `GetActiveWindow()==[app+8]` で描画可否分岐（不可なら `Sleep(50)` で 0x40F275 へ）→ 可なら slot3(vtable+0xC) の結果 `jge 0x40F26A` |
| 29 | 0x40F270 | `call 0x40D800(ecx=[app])` | **台帳 gap**（ret+int3 で `FUN_0040d4d0` と分離、台帳開始点なし）。内部 `FUN_00578F60()` の成否で **one-shot フラグ `[0xB3341C]`** を駆動し、初回のみブロック実行 → `call 0x498F00` / `++[0xB02C54]`。末尾 `0x40DF54 call FUN_0040D4D0`（E8=5）→ `0x40DF59 cmp [0xB34FA7],bl; je skip` → `0x40DF61 call FUN_00497E70`（**§4 のとおり不達**） |
| 30 | 0x40F27D / 0x40F287 | ループ退出判定 | `[app+0]!=0 → 0x40F488`（終了経路）／`[app+1]==0 → 0x40F130`（メッセージのみ）／`[app+1]!=0 → フレーム処理` |
| 31 | 0x40F28D-0x40F483 | **1フレーム処理**（[app+1]!=0 のとき） | `FUN_006AA170` → `FUN_006ECC80`→`FUN_004CECD0` → `FUN_00660110(1)`→vtable+8/vt+0x88 → `FUN_00660110(0)` → `FUN_00442630(ecx=[0xB333A0],0,0)` → `[+0x20]/[+0x24]=0x7FFFFFFF` → `FUN_004533F0([0xB33B00],[player],0)` → `FUN_0045A530(1)` → `FUN_00464440(0)` → vtable+0x150(0) / vtable+0x170 → **`0x40F3EA call 0x405CE0`** → `FUN_005E9690` → `[+0x58]` の vtable+0x3A8 → `FUN_00663340(esi)` → `FUN_00462080`/`FUN_00459870`/`FUN_0045A530(0)`/`FUN_0045C320` → `FUN_00675310(0xB3BD00)`/`FUN_00447300([0xB33A98])` → `FUN_0057CCC0(0)`/`FUN_005B5AC0`/`FUN_005B6040` → `[app+1]=0,[app+4]=0` → 戻る |
| 32 | 0x40F488-0x40F61D | 終了経路（メッセージループ退出時のみ） | `FUN_00701A80`（`[0xB3F940]!=0`）→ `FUN_0043E0F0`/`FUN_00410B80`/`FUN_0067F180`/`FUN_00682430`/`FUN_006844D0`/`FUN_00684710`/`FUN_00405B00`/`FUN_00405B40`/`FUN_0067CF00`/`FUN_00578EF0`/`FUN_005C0FC0`/`FUN_0053FB30`/`FUN_006AC330`/`FUN_006F96B0` → `FUN_0040C350`（`0x40F580`、E8=3）→ **`0x40F5C8 push 1; call 0x497B50`**（E8=1）→ `FUN_00747BE0`（E8=1）→ epilog |

**要点**: RT 群生成（0x406B4E）は上表 #1 の直中であり、以降の #2..#26 はすべて「プレイヤー/ワールド配置 → idle loop 突入」までの一方向初期化。`FUN_00405CE0`（#31）は**フレーム処理の中**で毎回呼ばれる候補で、起動直後の renderer 生成には関与しない（§6）。

### 2-3. スコープ11関数（+系）の解読結果

台帳の `named_funcs.txt` 境界と decompiled.c シグネチャ、call 目標の実測（`v128_profile.py`）による。

| 関数 | シグネチャ（dec） | 直接 call（固有順） | 判定 |
|---|---|---|---|
| `FUN_0067fcf0` | `void FUN_0067fcf0(void)`（SEH） | `FUN_00452A60, FUN_00446CB0(list PUSH), FUN_00452570, FUN_004526E0(unlink), FUN_0055E000(map), FUN_004D7630, FUN_0042B410, FUN_0067F5D0/67FB10/67FBC0/67FC10/67FC80/67FD70, FUN_00680A00, FUN_00780D10` | 書込先 **`[0xB3BE00]`**。セル/座標キーをリスト+マップに積む静的表構築 [推定] |
| `FUN_00683030` | `void FUN_00683030(void)`（SEH） | `FUN_004DB260/FUN_004DB4A0(FUN_004DB360系), FUN_0046B590/0046C300, FUN_004D7040/004D8A30/004D89A0, FUN_004B2460+FUN_009832E6(走査), FUN_006438F0/00658690/0064B400/00628EE0, FUN_005E02E0, FUN_006ECC80, FUN_00485E00/00484420/0048AE30` | 書込先 **`[0xB3BF80]`**。E8=1（WinMain のみ）。ワールド/セル列の初回走査 [推定] |
| `FUN_004431f0` | `__thiscall (this, int v)` | `FUN_00977C50, FUN_004425D0, FUN_0043FFF0, FUN_0043FE30, FUN_004C9300, FUN_0043FC20, FUN_004EF7E0, FUN_004EBC00, FUN_004EA570, FUN_0057A0D0, FUN_00440F20, FUN_004C97F0, FUN_004C9AA0/004C9A80` | `this+0x74` への「現在空間」切替。**`FUN_0043FC20` を内包**（§6-4） |
| `FUN_00447300` | `void FUN_00447300(void)` | `FUN_004B2460 → FUN_009832E6 → FUN_005222D0/FUN_004B2360/FUN_0055E000` | `[…+0x28]>>7&1` を見るワールド走査（ログ "Initializing Actor Locations..." 直後の実働） |
| `FUN_004d5d70` | `__thiscall (this, float*, undefined4*)` | `FUN_004D4310, FUN_004CEE90, FUN_00496EA0, FUN_005B1E20, FUN_00480520, FUN_008AA350, FUN_004D7630, FUN_0042B460/0042B470, FUN_004D7E30` | `this+0x24&1` と `this+0x3c` で分岐する位置解決。書込先 `0xB35E24..0xB35E48`（8変数まとまり） |
| `FUN_005e02e0` | `int __thiscall (int *this, char b)` | vtable+0x128（`b!=0` のみ）、`FUN_00468670/004686C0, FUN_00402BD0, FUN_0067DD70, FUN_004BEF40, FUN_00547B90` | 「種別ぶん取り」関数（WinMain は `b=0` で呼ぶ → vtable+0x128 は呼ばない） |
| `FUN_00663070` | `__thiscall (this, char)` | `FUN_00589B70(0x3ED), FUN_005898F0, FUN_009832E6, FUN_00584740, FUN_00578CF0, FUN_005732D0, FUN_0065A2C0, FUN_00585C10/00584390, FUN_00578F60, FUN_00572EC0, FUN_007D92C0, FUN_00612220` | **内部エントリを2本持つ gap 群**: `0x6632A0`（E8=1, WinMain 0x40F050）、`0x663340`（E8=2, WinMain 0x40F41A + `FUN_00500C90+0x6F`）。いずれも台帳開始点なし・dec ヘッダなし |
| `FUN_00442630` | `__thiscall (this, undefined4, char)` | `FUN_004425D0, FUN_007B4280, FUN_007C7E50, FUN_0043FFF0/FD40/FD70/FE30/FED0/004400A0, FUN_00447BA0, FUN_004C97F0/004C9CF0, FUN_00442770` | `this+0x7c` リスト走査。WinMain は `(0,0)` で呼ぶ（フレーム内） |
| `FUN_004533f0` | `undefined4 __thiscall (this, int, char)` | `FUN_0055E000, FUN_0045F2E0, FUN_0065C620(list Remove), FUN_memcpy, FUN_00404EC0, FUN_00446B80, FUN_009832E6, FUN_00453530, FUN_004CCED0, FUN_004AF170, FUN_004C97F0/004C9A80/004C9AA0, FUN_0041E6F0` | 引数 `this`=`[0xB33B00]`、第2引数=`[0xB333CC]`(=arg)。E8=14 |
| `FUN_0045a530` | `void __thiscall (this, char)` | `FUN_007C2A40` + 間接、E8=21 | **`[DAT_00b33398+0x10]`(所有スレッドID) と `GetCurrentThreadId()` を比較** → 異なれば第2引数の指示どおり処理（＝**メインスレッド・デッドロック回避ディスパッチ**）。WinMain は `push 1`（フレーム側）と `push 0`（後始末側）で2回呼ぶ |
| `FUN_00464440` | `void __thiscall (int *this, char)` | `FUN_004599B0/00463700/00462080/00459870, FUN_00582160, FUN_0057ECB0, FUN_0067F180, FUN_0067FCF0, FUN_0043F6E0/0043F560, FUN_004492E0, FUN_00418E50, FUN_005B7150/005A6A80/005AD750, FUN_0044A2B0, FUN_0066A740, FUN_0046C300, FUN_0044F3D0, FUN_0045A530, FUN_006ECC80, FUN_004D8AA0, FUN_00447D80, FUN_004431F0` | **`[0xB333C4]` を書く唯一の関数**（起動時のプレイヤー/オブジェクト生成系）。スレッドID比較も内包。E8=3 |

補助（スコープ外だが同一帯）: `FUN_00662ee0`(E8=1) = `_strrchr`+`FUN_005215C0/0052BDB0/00435830/0043BDA0/0043B420/0046D3F0`、`FUN_0042c970`(E8=1) = free のみ、`FUN_00675310`(E8=3) = `FUN_007616D0/FUN_004F9EC0(=Event起動)/FUN_005ED860/FUN_005E6680/FUN_0067B320/FUN_006ECC80/FUN_004D6670`。

---

## §3 早期 return 3 本の成立条件

### 3-1. 共通: すべて `0x40F61D`（epilog）へ、戻り値は必ず 0

```
0x40F61D: mov  ecx, [esp+0x6b4]
0x40F624: pop  edi / pop esi / pop ebp / pop ebx
0x40F628: xor  ecx, esp
0x40F62A: xor  eax, eax          ← 戻り値 0 に強制
0x40F62C: call 0x9811e2 (__security_check_cookie)
0x40F631: add  esp, 0x6a8
0x40F637: ret  0x10
```
3 本とも `[0x40F488, 0x40F61D)` の終了処理ブロック（`FUN_0040C350`・`FUN_00497B50` 等）を**素通り**する。到達時点ではウィンドウもレンダラも未生成（A/B はウィンドウ作成 0x40E80C より前、#3 も 0x40E7B8 より前）なので素通りは安全。

### 3-2. 各本の成立条件

| # | 命令列 | 成立条件 | 到達性 |
|---|---|---|---|
| 1 | `0x40E2C0 call [0xA28258]=MessageBoxA` → `0x40E2C6 jmp 0x40F61D` | ブロック A（`CopyProtectionStrings` の Title/Message を読み `MessageBoxA(NULL, msg, title, 0x40010=MB_OK\|MB_ICONERROR\|MB_SETFOREGROUND)`）に入った後は**無条件**に return 0。A への入場条件は `0x40E227 call 0x404940` の戻り値 `al`（コピー保護チェック） | **到達不能**（§3-3） |
| 2 | `0x40E376 call edi=MessageBoxA` → `0x40E378 cmp eax,2` → `0x40E37B je 0x40F61D` | ブロック B（`sCopyProtectionTitle2/Message2` を読み `MessageBoxA(…, 0x40035=MB_RETRYCANCEL\|MB_ICONEXCLAMATION\|MB_SETFOREGROUND)`）で **戻り値 == 2 (IDCANCEL)**。非 IDCANCEL なら `0x40E381 call 0x404940` → `[0xB33394]==0` の間ループ | **到達不能**（§3-3） |
| 3 | `0x40E39B call [0xA2829C]=FindWindowA` → `cmp eax,ebp(0)` → `je 0x40E3B1`（見つからなければ続行）→ `0x40E3A6 call [0xA282A0]=SetForegroundWindow` → `0x40E3AC jmp 0x40F61D` | **`FindWindowA(lpClassName=[0xB02C80]="Oblivion", lpWindowName=NULL) != NULL`** ＝ 同クラスのウィンドウが既に存在（=2 重起動）。前面化して return 0 | **到達可能** |

`[0xB02C80] = 0xA2FBB4 "Oblivion"`（ウィンドウクラス名。`RegisterClassA` の `lpszClassName` と `CreateWindowExA` の両方で同一値）を 0x40E393 で読む。IAT 紐付けは PE import テーブル実測（230 エントリ）。

### 3-3. #1/#2 が到達不能である決定的証拠（byte パッチ）

**前提の byte 列（raw 読出し）**:
```
0x40E22C: 84 c0                 test al, al
0x40E22E: 90                    nop
0x40E22F: e9 97 00 00 00        jmp  0x40E2CB     ← ブロック A を丸ごとスキップ
0x40E2D2: 90                    nop
0x40E2D3: e9 bb 00 00 00        jmp  0x40E393     ← ブロック B を丸ごとスキップ
```
- rel32 は `0x97` / `0xBB` のまま。**6 バイト jcc（`0F 84`/`0F 85` + rel32）を `90 E9` + 同一 rel32 で置換した 2 バイトパッチ**と一致（置換前は `0x40E22E+6 → 0x40E2CB`、`0x40E2D2+6 → 0x40E393` で、置換後と同じ飛び先）。
- **統計**: `.text`（2,258,215 命令）で `84 c0 90 e9` = **1 件のみ**（`84 c0 0f 84` = 1,196、`84 c0 0f 85` = 827）。`90 e9` 全体は 31 件で、うち本領域の2件だけが `test`/`cmp mem,imm` の直後。
- **CFG**: `0x40E234` と `0x40E2D8` への入辺は **0 本**（DFS 到達 False）。到達不能 91 命令のうち実質はこの2ブロック（151 B + 187 B）。
- 極性（元が `jz` か `jnz` か）は置換済み byte からは**判別不能**。文脈（A=致命エラー、B=リトライ）から「検証失敗時に A」＝ `0F 85` が自然 [推定]。

### 3-4. ブロック A/B の正体 = コピー保護（CD 検査）

`FUN_00404940`（**台帳 gap**: `pre16 = c3 cc×15` → 直前が ret+int3、台帳開始点なし、dec ヘッダなし、E8=2 件 = 0x40E227[到達可] と 0x40E381[死コード内]）:

```
0x404960 call [0xA280BC]=GetLogicalDriveStringsA(buf,0x200)
0x40496E mov byte [0xB33394], 0           ; 旗リセット
  loop: 0x40498E call [0xA280B8]=GetDriveTypeA(drive)
        cmp eax,5 (DRIVE_CDROM) → jne 次
        0x4049A3 sprintf(dst, "%sOblivionLauncher.exe", drive)   ; fmt 0xA2FC2C
        0x4049B2 call 0x982D71 (_access(path,0))
        test eax,eax; jne 次
        0x4049C0 mov byte [0xB33394], [drive]  ; = ドライブ文字（非 0）
        bl = 1
```
- ブロック A/B が読む INI は `Oblivion.ini` の **セクション `CopyProtectionStrings`**、キー `sCopyProtectionTitle` / `sCopyProtectionMessage`（A）と `sCopyProtectionTitle2` / `sCopyProtectionMessage2`（B）。
- **`[0xB33394]` の全参照は 5 件だけ**（W2: 0x40496E/0x4049C0、R3: 0x404980/0x40E2CB/0x40E386）＝ **このコピー保護コード専用**。他のグローバルから参照されない（bss = 起動時 0）。
- したがって v1.2.0416 Steam では: **チェック自体は実行されるが（0x40E227 は到達可能）、結果は捨てられ、A/B のダイアログと return は作動しない**。
- 関連: 4 本目の「終了」は WinMain ではない。`FUN_004052F0`（gap, E8=1）内のレンダラ生成失敗時 `0x405316 test eax,eax; je 0x40531A` → `sprintf("Failed to initialize renderer.\n%s")` → `MessageBoxA(NULL,…,"Oblivion",0)` → **`0x405347 call [0xA280D4]=ExitProcess(0)`**。

---

## §4 B43104 の 0 化と再 mirror 経路の欠落検証

### 4-1. 全参照の分類（`.text` 線形デコード 2,258,215 命令、全セクション imm32 走査）

| グローバル | 参照総数 | 書込 | 読み | 備考 |
|---|---|---|---|---|
| `0xB43104` | **159（.text のみ、他セクション0）** | **2** | 157 | set=`0x406A56`, clear=`0x7B88FB` |
| `0xB42F31` | 5 | 3 | 2 | §4-5 |
| `0xB350D8` | 33 | 2 | 31 | set=`0x498550`, clear=`0x497BD4` |
| `0xB42754` | 10 | 1 | 9 | `0x772066`（値=param の set/clear 兼用） |
| `0xB43110` | 262 | 3 | 259 | `0x7B74FF/0x7B8744/0x7BA022` |
| `0xB430DC` / `0xB430F0` | 79 / 75 | 3 / 3 | 76 / 72 | `0x7B73BF/0x7B8604/0x7B966E` 系 |
| `0xB34FA7` | 2（W1=`0x40DF66`(=0) / R1=`0x40DF59`） | 1 | 1 | bss、**非 0 化の書込が存在しない** |

**`0xB43104` の書込2件だけが正**（`0F 84 → 90 E9` 同様の非整列誤読を避けるため、線形デコードから operand 一致で抽出）:

```
0x406A56: mov dword ptr [0xB43104], edi   ; FUN_00406950+0x106 = mirror（唯一の caller = WinMain 0x40EAAC, E8=1）
0x7B88FB: mov dword ptr [0xB43104], ebx   ; FUN_007B8530+0x41B（ebx=0 → 0 化）
```

### 4-2. clear の所在は `FUN_007B8530`、その caller 鏖は WinMain 終了経路一本

**境界証明**（`pre16` = `… c5 5e c3 cc cc cc cc cc cc cc cc cc cc cc cc`）:
`0x7B8522 ret` → int3×12 → `0x7B8530 push ebx; push ebp`（prologue）。
→ **`0x7B8530` は台帳開始点なしの gap 関数**（台帳は `FUN_007b84e0 [0x7B84E0,0x7B8910)` と1つに束ねている）。`dec` にも `// FUN_007b8530` は無い。

caller 列挙（`.text` 全域の E8/E9）:

| 目標 | E8 件数 | caller |
|---|---|---|
| `0x7B8530` | **1** | `0x497B56`（`FUN_00497B50` 内） |
| `0x497B50` | **1** | `0x40F5CA`（WinMain 終了経路 `0x40F488..0x40F61D` 内） |
| `0x406950`（mirror 実行本体） | **1** | `0x40EAAC`（WinMain 起動部） |

→ **`0xB43104` の 0 化は「メッセージループが終了し、終了経路を走って初めて1回だけ」発生する。再 mirror は不要**（以降に RT 群を再生成するコードが無いことも合わせて整合）。

### 4-3. v90 訂正: `FUN_007B84E0` は B43104 を 0 化しない

v90 §3-2/残4 は「`FUN_007B84E0`（teardown）→ `FUN_007C1FF0` → `FUN_007C2450`」としていたが、実体は次（`0x7B84E0` から線形デコード、`0x7B8522 ret` で終端）:

```
0x7B84E0: push esi; mov esi, 0xB42EC0
0x7B84E6: mov eax,[esi]; test; je next
          … vtable+0x1c の結果が -1 なら ecx=0 …
0x7B850C: mov edx,[ecx]; mov eax,[edx+0x90]; call eax   ; 各スロットの解放仮想関数
0x7B8516: add esi,4 ; cmp esi,0xB42F30 ; jl loop
0x7B8522: ret
```
＝ **`[0xB42EC0, 0xB42F30)`（28 スロット）を走査して各オブジェクトの `vtable+0x90` を呼ぶだけ**で、`0xB43104` / `0xB42F31` / `0xB42F50` は一切触らない。
`FUN_007B84E0` の E8=4 件は `0x43FC6A`（`FUN_0043FC20+0x4A`、クリティカルセクション内）、`0x4456E5`（`FUN_004455E0+0x105`、開始セル解決時）、`0x4DB391`（`FUN_004DB360+0x31`、**第2引数==0 のときのみ**）、`0x7BA334`（`FUN_007BA2F0+0x44`）。
**→ 「teardown で B43104 が落ちる」の正体は `FUN_007B84E0` ではなく `FUN_007B8530`。**

### 4-4. `FUN_007BA2F0`（再初期化経路）はこのビルドで到達不能

call 鏖（E8 実測）:

```
WinMain 0x40DF61 (gap 関数 0x40D800 の末尾)
   │  0x40DF59: cmp byte [0xB34FA7], bl(=0)
   │  0x40DF5F: je  0x40DF6C          ← 旗==0 ならスキップ
   │  0x40DF61: call 0x497E70   (E8=1)
   │  0x40DF66: mov byte [0xB34FA7], 0
   ▼
FUN_00497E70 (E8=1) → 0x497FE9: call 0x7BA2F0  (E8=1)
   ▼
FUN_007BA2F0: cmp byte [0xB42F31],0 ; jne 再構築 / else call FUN_007BA0F0
```
- **`0xB34FA7` の書込は `0x40DF66`（=0 のみ）**。非 0 化する命令は `.text` に無く（`0xB34FA4/FA5/FA6/FA8…` への byte/dword 書込はあるが FA7 を跨ぐ物なし）。`rd(0xB34FA7)` = **bss（ファイル無バック = 起動時0）**。
- `bl=0` の担保: gap 関数 `0x40D800` 内 `0x40D81C: xor ebx,ebx` 以降、`0x40DF6F: pop ebx` まで ebx を再ロードしない（`push ebx`/`cmp`/`mov [..],ebx` のみ）。
- → **旗は常に 0 → `FUN_00497E70` は一度も呼ばれず → `FUN_007BA2F0`（E8=1）は不達 → 「B43104=0 のまま `FUN_007C2450` を呼ぶ」経路は存在しない**。
- 仮に到達した場合の整合: `FUN_007BA2F0` は `call 0x7B84E0`（4-3 の通り clear しない）を先に走らせるため、後続 `0x7BA3C7 mov ecx,[0xB43104] → 0x7BA3DD call FUN_007C2450` でも **B43104 は有効値のまま**。v90 の「NULL ガード無しで崩溃する」懸念は、teardown を `FUN_007B8530` と取り違えたことから生じた誤爆。

### 4-5. フラグ `0xB42F31` の遷移

| VA | 命令 | 所属 | 意味 |
|---|---|---|---|
| `0x7BA2D1` | `mov byte [0xB42F31], 1` | `FUN_007BA0F0+0x1E1` | **shader 初期化完了時に set**（WinMain 0x406A83 から1回） |
| `0x7B734A` | `mov byte [0xB42F31], bl(0)` | `FUN_007B7250+0xFA` | clear（E8=1、唯一 caller `FUN_00A10EB5`） |
| `0x7B88D3` | `mov byte [0xB42F31], bl(0)` | `FUN_007B8530+0x3F3` | clear（**WinMain 終了時のみ**、§4-2 と同一関数） |
| `0x7BA116` | `cmp` | `FUN_007BA0F0+0x26` | 初期化本体の自己参照 |
| `0x7BA316` | `cmp byte [0xB42F31],bl` | `FUN_007BA2F0+0x26` | **再初期化経路のガード（不達）** |

→ 「初回以外に使われるか」= **使われない**（ガード自体が不達）。`FUN_007BA0F0` の E8=2 は `0x406A83`（起動）と `0x7BA31D`（不達側）。

### 4-6. 判定

**[確定] 「再 mirror コードの欠落」は矛盾ではない。** 再 mirror が要らない理由は、(a) `0xB43104` の clear は WinMain 終了経路 1 経路のみ、(b) 0 化後に RT 群を再生成する `FUN_007BA2F0` 経路は `0xB34FA7` ガードで恒久不達、(c) `FUN_007B84E0`（頻繁に呼ばれる）は B43104 を触らない、の3点。（v90 残4 は解消、ただし v90 の teardown 関数同定は `FUN_007B84E0` → **`FUN_007B8530`** へ訂正。）

---

## §5 `FUN_0076BED0`（E8=0 件）経由のレンダラ破棄タイミング

- **cache の E8=0 を全セクション imm32 走査で裏取り**: 0x76BED0 を指す4バイトは**画像全体で1件のみ = `.rdata 0xA88EA4`**。これは GT §4 の **`NiDX9Renderer` vtable 先頭（98 スロット、slot0）**。
- `FUN_0076BED0 [0x76BED0,0x76BEF0) = 0x20 B` は `0x76BED3: call 0x76B5D0` を含む薄い入口で、`FUN_0076B5D0` の E8=1（唯一 caller = `0x76BED3`）。
- → **レンダラ破棄は「`NiDX9Renderer` の vtable slot0 を間接呼出しする」経由でしか行われない**（`mov edx,[esi]; mov eax,[edx]; push 1; mov ecx,esi; call eax` の定型）。E8=0 は欠落ではなく vtable dispatch の証拠。
- **具体的な到達経路（実測2系統）**:
  1. `FUN_007B8530` 内 `0x7B88D9..0x7B88FB`: `[0xB43104]` の refcount を `InterlockedDecrement(esi+4)` で 0 に落としてから vtable slot0 を呼ぶ → **WinMain 終了経路（0x40F5CA → FUN_00497B50）時のみ**。
  2. `FUN_00497B50` 内 `0x497B5B..0x497BD4`: `[0xB350D8]` を同じ refcount + vtable slot0 パターンで解放し `mov [0xB350D8],0`。
- **タイミング判定**:
  - レンダラ**生成は1回のみ**: `WinMain 0x40E865 → 0x4052F0(gap,E8=1) → 0x40530E → FUN_00498340(E8=1) → 0x498519 → FUN_0076BD40(E8=1) → FUN_0076A1C0(E8=1) → FUN_00778F60(E8=1)`。各段 E8=1、起点も WinMain の1箇所 → **プロセス内で1回しか作られない**。
  - `0xB42754` の書込は `0x772066` の1箇所のみで、set（`eax=param_1`=renderer）と clear（`eax=0`）を兼ねる。clear 側は v90 §4 のとおり renderer 本体 destroy 鏈（`FUN_0076B5D0 → FUN_00779010 → FUN_00772060`）内。
  - **結論**: 生成が1回 = 破棄も最大1回。B43104/B350D8 の明示 release はいずれも WinMain 終了経路にしか無いため、**既定の破棄タイミングはプロセス終了時**。[確定] 設定変更時等に他所有者が refcount を落として先に slot0 が走る経路は未特定（§8 残点）。

---

## §6 `FUN_00405CE0` → `FUN_00405090` の意味論（追記(5) / v90 残5）

### 6-1. 前提の是正: 6 分岐は `FUN_00405CE0` ではなく gap 関数 `0x405D60`

**境界証明**（`pre16` = `… 50 ff 15 78 80 a2 00 5f 5e 5b c3 cc`）:
`0x405D5E ret` → int3 → `0x405D60 sub esp,0x78`（prologue）。台帳は `FUN_00405ce0 [0x405CE0, 0x406360)` の1本で **0x405D60 の開始点なし**、dec にも `// FUN_00405d60` は無い。

| 関数 | 範囲 | 実体 |
|---|---|---|
| `FUN_00405CE0`（台帳どおり） | `[0x405CE0, 0x405D5F)` | `this+0x1d4` と `this+0x1d8` の NiPointer を `InterlockedDecrement(ptr+4)` → 0 なら `vtable+0` で解放し、`this+0x1d4` を0化する**だけ**。float 定数も `0xB33395` ガードも**無い**。E8=6（WinMain `0x40F3EA` + 5 件） |
| **`0x405D60`（gap）** | `[0x405D60, 0x4060AB)` | **17 分岐ディスパッチ**。`0x405D63 cmp dword [esp+0x7c],0`（arg1≠0 なら即 return 0）、`[0xB06B18]` 再入ガード、`jmp dword [esi*4+0x4060AC]`（esi=arg3, 0..0x10）。**E8=0 件 = 関数ポインタ経由のみ** |

WinMain は `0x40E620: mov dword [0xB02184], 0x405D60` で登録し、読手は `FUN_004014A0+0x62/+0xBA`、`FUN_00402480+0x8C`、`FUN_00501040+0x68/+0x7F`（各 E8 は1/1/0 = 間接）。
**→ v90 残5 の「FUN_00405CE0 → FUN_00405090 ×6」は台帳マージ由来の誤帰属。正しくは `0x405D60(gap) → FUN_00405090 ×6`。WinMain `0x40F3EA` の呼び出し先（0x405CE0）は上表の小関数で、レンダラ生成はしない（v90 の結論「renderer 生成はしない」は結論として正しいが、根拠の関数同定が1つずれている）。**

### 6-2. 6 分岐の float 定数（すべて `[0xB33395]!=0` で skip）

| 分岐先 | arg1 | arg2（最初の float） | arg3（次の float） | 定数実体 |
|---|---|---|---|---|
| `0x405F0A` | 1 | `-FLT_MAX`（`fchs`） | `100.0f` | `0xA7DEB4`=0x7F7FFFFF、`0xA2FE7C`=0x42C80000 |
| `0x405F3C` | 1 | `100.0f` | `250.0f` | `0xA2FE7C`、`0xA2FE78`=0x437A0000 |
| `0x405F6E` | 1 | `250.0f` | `+FLT_MAX` | `0xA2FE78`、`0xA7DEB4` |
| `0x405FE9` | 0 | `-FLT_MAX` | `100.0f` | 同上 |
| `0x406017` | 0 | `100.0f` | `250.0f` | 同上 |
| `0x406042` | 0 | `250.0f` | `+FLT_MAX` | 同上 |

＝ **`(-∞,100)` / `(100,250)` / `(250,+∞)` の3距離帯 × flag 2 種**。`0xA7DEB4` は `.rdata` の `FF FF 7F 7F` = **FLT_MAX**（`fchs` で -FLT_MAX）。ウィンドウ寸法・アスペクト比の定数ではない（`0xA2FF44`=2048.0f / `0xA2FFE8`=128.0f は §2-2 #10 の `FUN_004431F0` 用で別物）。

### 6-3. `FUN_00405090` の中身と各 callee の役割

```
0x405090: push ecx
0x405091: mov ecx,[0xB333A0] ; call 0x43FD20        ← FUN_0043FD20(this) → 戻り値 eax = 対象数
          if ([this+0x34]!=0) count = 1             ← 単一対象モード
loop i<count:
          esi = ([this+0x34] ? : [this+0x3c][i])    ← 対象（セル）取得
          if (byte arg1 != 0):                       ← 範囲内フィルタ有効時
              edi = FUN_004C9A80(esi)   (E8=92)     ← grid X
              ebp = FUN_004C9AA0(esi)   (E8=94)     ← grid Y
              if (abs([this+0x20]-edi)<2 && abs([this+0x24]-ebp)<2) → skip
          FUN_004D1A90(esi, arg2, arg3)   (E8=1 = この呼び出しのみ)
0x405139: push 1 ; call 0x43FC20                    ← FUN_0043FC20(this,1)
```
- **`FUN_0043FD20(this)`（E8=3）**: 「適用対象セル数」を返すゲッタ（this=`[0xB333A0]`）。`[this+0x34]!=0` なら1に強制。
- **`FUN_004D1A90(cell, lo, hi)`（E8=1）**: 距離帯 `[lo,hi)` をセルに適用する唯一の実装。6 分岐はすべてこの呼び出しの引数違い。
- **`FUN_004C9A80/004C9AA0`**: グリッド座標ゲッタ（`.text` 全体 E8=92/94、WinMain 帯の 0x40ED91/0x40EDAD でも同じ物を呼ぶ）。
- **`FUN_0043FC20(this, flag)`（E8=23）**: 実測内容 = `cmp [0xB350D5],0` ゲート → `EnterCriticalSection(0xB35380)` → `GetCurrentThreadId` → カウンタ `0xB353F8/0xB353FC` → `FUN_0055E390(1)` → **`0x43FC6A call 0x7B84E0`（§4-3 のスロット解放、B43104 非破壊）** → `[0xB35300]` 判定で `FUN_004A25F0` → `FUN_00402740` → `FUN_0055E390(1)` → `LeaveCriticalSection`。＝ **クリティカルセクション配下の一括後始末（描画オブジェクト解放）** [証拠ベース]。
  → **`FUN_00405090` の6呼出しはいずれも最後にこれを必ず呼ぶ**（＝距離帯更新は「対象を更新して保留分を解放」のワンセット）。
- WinMain の `0x40F3EA` は `FUN_00405CE0`（6-1 の小関数）であり、この6分岐も `FUN_00405090` も経由しない。

### 6-4. ガード `0xB33395` の意味

- 書込は2件のみ: `0x40C868 mov [0xB33395],1`（`FUN_0040C830+0x38`）と `0x40D136 mov [0xB33395],0`（`FUN_0040C830+0x906`）＝ **同じ関数内で set→clear される再入/実行中フラグ**。読み8件はすべて `0x405D60` 内（`0x405E40/0x405EE3/0x405F17/0x405F49/0x405F7B/0x405FC2/0x405FF6/0x406021`）。
- `FUN_0040C830`（E8=3）は WinMain 内 `0x40D658`（`FUN_0040D4D0` 内）から呼ばれるため、**起動処理の実行中はこのディスパッチの該当分岐が全部 skip される**（v90 の「①で flag=1 済みのため再生成しない」は、関数帰属は違えど「起動中に距離帯更新が走らない」という意味では正しい）。

---

## §7 結論（3行）

1. **RT 群生成以降の起動順序**は「関連ポインタ解放 → `FUN_0067FCF0`/`FUN_00683030` → 現在空間切替 `FUN_004431F0` → `Initializing Actor Locations...` → `FUN_00675310`/`FUN_00447300` → `Loading initial area...`（INI 未設定ならエラー不発）→ `Placing player...` → プレイヤー座標確定 → gap `0x6632A0` → `[app+3]=1` → `Begin Idle loop...` → PeekMessage ループ → gap `0x40D800`（one-shot `[0xB3341C]`）→ フレーム処理内で `0x40F3EA call 0x405CE0` 」と確定。
2. **早期 return 3 本のうち #1(0x40E2C6)/#2(0x40E37B) は「コピー保護ダイアログ」ブロック内にあり、`0F 84/85 → 90 E9` の2バイトパッチ（`.text` で `84 c0 90 e9` = 1/2,258,215、入辺0本）により恒久不達**。#3(0x40E3AC) のみ `FindWindowA("Oblivion",NULL)!=NULL`（2重起動）で到達可能、いずれも epilog の `xor eax,eax` により **return 0**。
3. **B43104 の 0 化は `FUN_007B8530`（gap、唯一 caller `FUN_00497B50` ← WinMain `0x40F5CA`）の1経路のみ**、再 mirror を要しない。v90 の teardown 同定（`FUN_007B84E0`）は訂正、`FUN_007BA2F0` 再初期化経路は `0xB34FA7`（非0化書込なし・bss）ガードで不達。レンダラ破棄は `NiDX9Renderer` vtable slot0（`.rdata 0xA88EA4` → `FUN_0076BED0`）経由で、生成が E8=1 連鎖のため既定はプロセス終了時1回。

---

## §8 残留不明点

1. **元 jcc の極性**（§3-3）: `0x40E22E`/`0x40E2D2` のパッチ前が `jz` か `jnz` かは byte 列から判別不能（rel32 は保存されているため飛び先は復元可）。文脈から「検証失敗→ダイアログ」＝ `0F 85` が自然だが [推定]。
2. **`0x4060F5`（WndProc 疑い、`cmp esi,0x84`=WM_GETMINMAXINFO 系）** が独立関数かデータ直後の継続かは jump table 隣接のため未確定（`0x405D60` は ret+int3 で確定）。
3. **`FUN_007B7250`（`0xB42F31` のもう1つの clear、E8=1 → `FUN_00A10EB5`）** の到達時期（`FUN_00A10E50` の呼び出し元）は未追跡。到達すれば `0xB42F31` が0になるが、`FUN_007BA2F0` 不達のため実害の有無は未判定。
4. **レンダラ slot0 の追加発火経路**: refcount を落としうる他所有者（`0xB42EB8` の21書込など）経由でプロセス途中に slot0 が走る可能性は未調査（生成は1回と確定済み）。
5. **`FUN_0067FCF0` / `FUN_00683030` の名前付きカテゴリ**（アクター位置表かワールド初期走査か）は call 目標と書込グローバル（`0xB3BE00`/`0xB3BF80`）からの [推定] にとどまる。
6. gap 関数 `0x404940` の戻り値が「CDROM を1台見つけたか」か「ファイル検証に成功したか」は、`bl` の設定位置（CDROM 判定でのみ1）から前者が濃いが [推定]。
7. `0x404940` が呼ぶ `0x982D71` の同定（`_access` 推定）は、`0x40E1B6` での `-1` 比較のみが根拠。

---

## §9 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl、本ノート作成前・作成後・完走確認時の計3回すべて PASS）。
- **完走時の独立再検証**（`v128_cfg.py` / `v128_b43104.py` 再実行 + 新規 `v128_verify.py` を `python` で実行、いずれも本ノートの数値と一致）:
  - WinMain 線形デコード 1,536 / 到達 1,445 / 到達不能 91、`0x40F61D` 入辺 4 本、到達不能レンジ `[0x40E234,0x40E2CB)` 151 B・`[0x40E2D8,0x40E393)` 187 B、`reach 0x40E2C6/0x40E37B=False`、`reach 0x40E3AC=True` ✅
  - `.text` 線形デコード **2,258,215 命令**（skipdata、.byte 含む） ✅
  - グローバル書込（線形デコード operand 一致）: `B43104`=2（`0x406A56`/`0x7B88FB`）、`B42F31`=3（`0x7B734A`/`0x7B88D3`/`0x7BA2D1`）、`B34FA7`=1（`0x40DF66` のみ）、`B33394`=2W/3R、`B33395`=2W/8R、`B350D8`=2W/31R、`B42754`=1W ✅
  - E8 件数: `0x406950=1 / 0x683030=1 / 0x67FCF0=2 / 0x76BED0=0 / 0x7B8530=1 / 0x497B50=1 / 0x497E70=1 / 0x7BA2F0=1 / 0x7BA0F0=2 / 0x7C2450=2 / 0x7B84E0=4 / 0x405CE0=6 / 0x405D60=0 / 0x404940=2 / 0x405090=6 / 0x43FC20=23 / 0x43FD20=3 / 0x4D1A90=1 / 0x6632A0=1 / 0x663340=2 / レンダラ生成連鎖 `0x4052F0/0x498340/0x76BD40/0x76A1C0/0x778F60` = すべて 1 ✅
  - byte パターン: `84 c0 90 e9`=**1**、`84 c0 0f 84`=1,196、`84 c0 0f 85`=827、`90 e9`=31 ✅
  - 全セクション imm32 `0x76BED0` = **1 件（.rdata 0xA88EA4）**、`0xA2FBB4`="Oblivion"、epilog `0x40F61D` の `xor eax,eax`(`33 c0`) 実在 ✅
  - §2-2 の WinMain call 列 19 箇所（`0x40E865/0x40EAAC/0x40EADB/0x40EAEA/0x40EB0C/0x40EB11/0x40EB2C/0x40EB62/0x40EB6D/0x40EB72/0x40EB9F/0x40EBAA/0x40EFFA/0x40F050/0x40F05E/0x40F069/0x40F270/0x40F3EA/0x40F5CA`）をデコードダンプと突合し **19/19 一致** ✅
- **内部整合（一致した件数）**:
  1. WinMain 線形デコード **1,536 命令** = v90 の 1,536 ✅
  2. WinMain CFG 到達 **1,445** = v90 の 1,445 ✅
  3. `0x40F61D` 入辺 **4 本** = v93 §b-3 の「外部分岐4本」 ✅
  4. `.text` 線形デコード **2,258,215 命令** = GT §4 ✅
  5. `B43104` 書込 **2 件**（set 0x406A56 / clear 0x7B88FB）= v90 の writer 2 件 ✅
  6. `call 0x406950` = **1** / `call 0x7C2450` = **2** = v90 の記載 ✅
  7. `call 0x76BED0` = **0**、全セクション imm32 = **1**（`.rdata 0xA88EA4`）= v90 残8 ✅
  8. `FUN_0076BD40/0076A1C0/00778F60/00769BE0` の E8 がすべて **1** = v90 の「逆経路の混入なし」✅
  9. GT スポット 10 件: `0x446CB0=347` / `0x4468B0=0` / `0x401F20=7573` / `0x4FB5F0=22` / `0x4FB510=1` / `0x4FDE30=0` / `0x6135F0=234` / `0x4FBF90=77` / `0x40DF80=1`（E8）/ `0x98780E=0` → **10/10 OK**
  10. IAT 実測 **230 エントリ**、`0xA280E4=GetPrivateProfileIntA` = v93 の同定と一致 ✅
  11. 台帳 gap 関数の ret+int3 境界証明 **8 件**（`0x404940`/`0x4052F0`/`0x405D60`/`0x40D800`/`0x40DF80`/`0x497B50`/`0x6632A0`/`0x663340`/`0x7B8530` = うち新規 **5 件**: `0x404940`, `0x405D60`, `0x6632A0`, `0x663340`, `0x7B8530`）で全件「直前 ret + int3 パディング + prologue」が成立 ✅
  12. グローバル参照の総和: `B43104 159 + B42F31 5 + B350D8 33 + B42754 10 + B34FA7 2 + B33394 5 + B33395 10 = 224` 件（すべて `.text`、他セクション0件）✅
- 編集範囲: 成果物は本ノート1件のみ。中間スクリプトは `%TEMP%\opencode\v128_*.py`。`docs/`・`TODO_re49.md`・台帳・git・Supermemory・sync は無変更。

## §10 Android 移植への示唆

起動シーケンスは「ウィンドウ → レンダラ（1回のみ）→ shader/RT（mirror は起動時に1回、解放は終了時のみ）→ ワールド/プレイヤー配置 → PeekMessage 帯のフレームループ」という単方向の一本道であり、移植では (a) レンダラと RT 群の寿命を**アプリ終了まで同一**にしてよい（実行中に RT 群を破棄・再生成する経路は本ビルドでは存在しない）、(b) `FindWindowA` による多重起動防止は Android では Activity の launchMode/singleTask 相当に置き換えれば同じ早期終了（return 0）の意味論が保てる、(c) コピー保護系 early return 2 本は**移植対象外の死コード**であり再現不要、(d) `FUN_0045A530`/`FUN_00464440` のスレッドID照合は GLSurfaceView の render thread 与えであるため「メインスレッドでのみ実行したい処理」のキューイングとして同じ設計で移植できる、(e) gap 関数（`0x405D60` の17分岐ディスパッチ、`0x40D800` の one-shot ガード）は名前付きラベルが無いため移植時は独自の関数境界（ret+int3 相当）で切り出す必要がある、という5点を設計入力にできる（実装は行わない）。
