# v611 — OBNG（Naragorn/OBNG）クロスチェック裁定: 同一 EXE の姉妹 RE プロジェクトとの照合と LoadingMenu レーン（v608）の確定補強

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。**md5 `cdd2f0c5eff198d4f26b7b5b54ce4930` ＝ OBNG の対象バイナリと完全一致**（ImageBase 0x00400000）。
根拠: (a) OBNG リポジトリ clone（`%TEMP%\opencode\OBNG`・depth1・1124 files・11.6 MB・commit `b9526a6`）。(b) 我々の EXE 実読 probe **u43_1 / u44_1**（EXIT=0・冒頭 Test-Path・recovery `callers_of(0x4E4A10)=1043` / `find_head(0x42703E)=0x426A90`）。(c) `docs/Oblivion_decompiled.c`・実物 XML。
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[参照(OBNG)]`（OBNG 記述のみ・我々未読）/ `[部分確定]`
先行: `v608_u_handoffB_loadingmenu_loader_ruling.md`（依頼 B・ai-todo #96）。

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| OBNG の対象バイナリは我々と同じか | **[確定(命令)]** | **同一**。md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`（我々の手元 EXE と一致）。→ OBNG のアドレスは我々の probe でそのまま照合できる。 |
| OBNG のデータは取得できるか | **[確定(命令)]** | **可能**。public・GPL-3.0。`knowledge/`（約40本）・`re/symbols/`（`xobse.csv` 2004 件ほか）・`re/ledger.md`・`re/slices/`（27）・`re/functions/`（114）を clone で取得。**`re/export/`（rea-all 21,240 関数 decompile・205 MB）は Git 外**で取得不可。 |
| 我々の FUN_ に実名が引けるか | **[確定(命令)]** | 我々 21,597 アドレスのうち **union 416 件**に OBNG 側の名前/記述（symbols 353・ledger 18・functions 104）。とくに **UI/Tile/Interface 系の実名**（xOBSE 由来）が取れる（§3.2）。 |
| LoadingMenu の知見は一致するか | **[確定(命令)]** | **完全一致＋我々が上積み**。定数（`0xA2FC70`=1000.0・`0xB14158`=15.0・`0xB14160`=2500・`0xB14168`=100・`0xB14170`=10）・`0xB33A98`=g_dataHandler・時間単位 ms は三者一致。**OBNG が「名前未エクスポート」とした4設定の実名を我々は読めた**（§4.2）。 |
| v608 の記述に誤りはあるか | **[確定(命令)]** | **実質誤り無し**。`0x5AD440` の callers は関数単位で `0x5AD700`/`0x5ADCF0`/`0x5ADEC0`（v608 記載どおり）。call site の精密値は §4.5。 |

---

## 2. OBNG の位置づけとアクセス

- **リポジトリ**: `https://github.com/Naragorn/OBNG`（public・GPL-3.0・472 commits・author Naragorn）。
- **目的**: Oblivion.exe 1.2.0.416 のホットパスを Ghidra で読み、C++ で再構築し、xOBSE プラグイン（`OBNG.dll`）で in-game 差分検証（shadow mode）。OBVR の姉妹プロジェクト。
- **構成**: `knowledge/`（話題別の恒久知見）・`re/`（作業中: symbols / slices / functions / ledger）・`src/engine/`・`tests/`・`tools/`・`plugin/`。
- **Git 外**: `re/export/`（関数ごとの pseudo.c/asm.s/data.json）と `re/export/rea-all/`（21,240 関数・205 MB の全 decompile）。**GitHub からは落とせない**（共有ディスクのみ）。
- **証拠規則**（`AGENTS.md`）: 4区分 **read / measured / derived / unverified**・**1アドレスは独立2ソース一致で初めてヘッダ入り**・**Ghidra 疑似コードは証明にしない**（我々の「[確定(命令)]・原本実読優先」と同型）。
- **スライスは性能系に集中**（time-info / frame-clock / loading-menu / actor-* / draw-* / scene-pass / havok-*）。**UI メニュー・chargen は対象外**＝我々の v610（RaceSexMenu）と重ならない。

---

## 3. アドレス照合（我々 21,597 FUN_ × OBNG 2,255 アドレス）

### 3.1 件数 [確定(命令)]
| 源 | 一致数 |
|---|---|
| `re/symbols/*.csv`（xobse 2004・obvr 131・slice-* 計） | **353** |
| `re/ledger.md` | 18 |
| `re/functions/*.md` | 104 |
| **union** | **416** |

### 3.2 我々のレーンで使える実名（xOBSE 由来・抜粋）[確定(命令)]
| アドレス | 名前 | 我々の用途 |
|---|---|---|
| `0x0058CEB0` | `Tile::UpdateFloat` | **v610 の `SetFloat(tile,trait,value)` の実体** |
| `0x0058CED0` | `Tile::UpdateString` | SetString 系 |
| `0x005898F0` | `Tile::GetContainingMenu` | **v608 Tick 内で使用** |
| `0x00582160` | `InterfaceManager_GetSingleton` | v608 Tick B15 |
| `0x00578F60` | `IsMenuMode` | メニュー判定 |
| `0x00579770` | `CloseAllMenus` | メニュー閉鎖 |
| `0x00584880` / `0x00585190` | `Menu::RegisterTile` / `Menu::EnableMenu` | メニュー登録 |
| `0x00588EF0` / `0x00589080` | `TileStrToStrID` / `TileStrIDToStr` | trait 名解決 |
| `0x0058BEE0` / `0x00590420` | `Tile::DoActionEnumeration` / `Tile::ReadXML` | 入力・XML 読込 |
| `0x005C2B36` / `0x005C2BF0` | `EventManager::kChargenPatchAddr` / `kRaceMenuCamera` | **chargen 系** |
| `0x0066A400` / `0x0066C580` | `PlayerCharacter::SetBirthSign` / `EventManager::kChargenCallAddr` | chargen 系 |
| `0x0044D950` / `0x0044DCF0` | `AddFormToDataHandler` / `DataHandler::ConstructObject` | DataHandler |
| `0x00B13348` | `RTTI_RaceSexMenu` | v610 の RTTI 確認 |

- **注意（ライセンス）**: 実名は **xOBSE（GPL）由来**。本裁定は「読んだ」事実の記録に留め、**リポジトリへ一括コピーしない**（取り込みは §6 V-611-1 の別途判断）。作業用の merged CSV は `%TEMP%\opencode\obng_merged_symbols.csv`（2255 行）に生成。

---

## 4. LoadingMenu レーン突き合わせ（v608 の確定補強）

### 4.1 三者一致した項目 [確定(命令)]
| 項目 | 値 | 我々 v608 | OBNG | 我々実読（u43_1） |
|---|---|---|---|---|
| `0xA2FC70` (f64) | 1000.0 | ○ | ○ | ○ |
| `0xB14158` (f32) | 15.0 | ○ | ○ | ○ |
| `0xB14160` (i32) | 2500 | ○ | ○ | ○ |
| `0xB14168` (i32) | 100 | ○ | ○ | ○ |
| `0xB14170` (i32) | 10 | ○ | ○ | ○ |
| `0xB33A98` | DataHandler* | ○ | ○（g_dataHandler） | —（我々は v608 で実読済） |
| 時間単位 | ms | ○ | ○ | ○ |

### 4.2 我々が新規確定した項目（OBNG は未確定としていた）[確定(命令)]
OBNG の `re/functions/0x005AD980.md` は 4 設定の name ポインタ（`0x00A6C610/30/54/7C`）を「not exported＝名前不明」としていたが、**我々の EXE では文字列が読める**:

| アドレス | 実名（GMST 風設定名） | 実値 | 意味 |
|---|---|---|---|
| `0xB14158` | **`fLoadingSlideDelay:LoadingBar`** | 15.0 (f32) | スライド滞留（秒）・バー再アーム値 |
| `0xB14160` | **`iMoveBarMaxMilliseconds:LoadingBar`** | 2500 | これ以上で無条件に1ステップ進む |
| `0xB14168` | **`iMoveBarChaseMilliseconds:LoadingBar`** | 100 | cap 未満でこの値超なら進む |
| `0xB14170` | **`iMoveBarWaitingMilliseconds:LoadingBar`** | 10 | fastStepEnabled 時のみ使う第三しきい値 |

さらに **バー比率配列 `0xB14120..0xB1412C`（4 ポインタ）** を実読:

| スロット | 設定名 | 実値 |
|---|---|---|
| 0 | `fPercentageOfBar0:LoadingBar` | **0.06** |
| 1 | `fPercentageOfBar1:LoadingBar` | **0.35** |
| 2 | `fPercentageOfBar2:LoadingBar` | **0.44** |
| 3 | `fPercentageOfBar3:LoadingBar` | **0.15** |
| — | 和 | **1.00**（ctor の合計検証 `Total value of [LoadingBar] percentages is not 1` の対象） |

### 4.3 LoadingMenu フィールド表（ctor `0x5AD0D0` 実読）[確定(命令)]
| Offset | 型 | 値（ctor 初期化） | 名前 | 根拠 |
|---|---|---|---|---|
| +0x00 | vtable | `0x00A6C40C` | — | `mov [esi],0xa6c40c` @0x5AD11F |
| +0x28 | i32 | -1 | — | @0x5AD134 |
| +0x3C | i32 | **-1** | `slowStepCap` | @0x5AD131 |
| +0x40 | f32 | **15.0**（`0xB14158`） | `screenTimeLeft`（秒） | @0x5AD338..33E |
| +0x4C | ptr | 0 | ロード対象 list head | @0x5AD125 |
| +0x50 | ptr | 0 | ロード対象 list tail | @0x5AD128 |
| +0x54 | TimeInfo* | `FormHeap_Allocate(0x18)`→`TimeInfo::Init` | `clock` | @0x5AD2E7/2E9・0x5AD30E・0x5AD31F |
| +0x60 | i32 | 0 | `shownStep` | @0x5AD137 |
| +0x64 | i32 | 0 | `targetStep` | @0x5AD13A |
| +0x68 | u32 | `clock->clockMs`（+0x10） | `stepChangedMs` | @0x5AD332 |
| +0x6C | u32 | `clock->clockMs` | `lastTickMs` | @0x5AD335 |
| +0x70 | u8 | 0 | `fastStepEnabled` | @0x5AD13D |
| +0x71 | u8 | 1 | — | @0x5AD141 |

- `TimeInfo::Init`=`0x47D150`（thiscall(tick)・ret 4）／`TimeInfo::Update`=`0x47D170`。**`0x18` バイトの TimeInfo を FormHeap 確保し、失敗時 `[+0x54]=0`**（`xor edi,edi` @0x5AD315）。
- ctor は末尾で `[+0x40]` を `0xB14158`=15.0 に設定（＝Tick の再アーム値と同一）。

### 4.4 Tick `0x5AD980` のバー進捗則 [確定(命令)]（OBNG 記述を我々実読で一致確認）
- `now = clock->clockMs`（+0x10）、`sinceStep = now − [+0x68]`、`sinceLast = now − [+0x6C]`（i32 減算 @0x5AD9CD/D4）。
- 進捗: `sinceStep > 0xB14160`(2500) → `targetStep++`（無条件）／そうでなければ `targetStep < slowStepCap` のとき `sinceStep > 0xB14168`(100) → `targetStep++`／さらに `fastStepEnabled`(+0x70)≠0 かつ `sinceStep > 0xB14170`(10) → `targetStep++`（@0x5AD9D7..0x5ADA06）。
- 残量: `screenTimeLeft(+0x40) −= sinceLast / 1000.0`（`fdiv [0xA2FC70]` @0x5ADA1E・負値は `+2^32`（`0xA2FC78`=4294967296.0f）補正 @0x5ADA18）。
- 反映: `targetStep≠shownStep` か満了なら `Tile::UpdateFloat(tileMenu, 0xFB1, (float)shownStep)`（trait **user3** @0x5ADA89..8E）。新 `shownStep < 99.9999`（`0xA6C4C8`）かつ refresh なら **`0x5AD780`（=RefreshScreen）を呼び** `screenTimeLeft = *(0xB14158)` で再アーム（@0x5ADAAE..BF）。
- 末尾: `lastTickMs = now`（@0x5ADB12）→ メインスレッドなら `0x60D0A0(g_osGlobals)` へ tail jump（`[0xB33398]+0x10`=mainThreadID 比較 @0x5ADB15/1B）。
- `0xB33A10`=g_ioManager 経由の `0x432860`/`0x432890`、`InterfaceManager` の `0x583F40`/`0x579260(0)`/`0x5792B0`（@0x5ADAC2..0x5ADB0D）。

### 4.5 callers の精密化 [確定(命令)]
| 対象 | call site → 関数ヘッド |
|---|---|
| `0x5AD440`（v608 のローダ） | `0x5AD734`→`0x5AD700`／`0x5ADE2C`→**`0x5ADCF0`**／`0x5AE03E`→**`0x5ADEC0`**（＝v608 の関数単位記載は正しい） |
| `0x5AD780`（RefreshScreen） | `0x5ADAAE`（Tick 内・B14）／`0x5ADE34`→`0x5ADCF0` |
| `0x5AD980`（Tick） | 15 箇所（`0x5ADBA4`→`0x5ADB40`／`0x5AE030`→`0x5ADEC0`／`0x578E02`/`0x57E197`…/`0x462DC4`/`0x6655EE` 等） |
| `0x5AD0D0`（ctor） | `0x587F03`→`0x587D20` |

### 4.6 OBNG 記述のみ（我々未読）[参照(OBNG)]
- `0x5AD780` の役割名「**RefreshScreen(menu, tile)**」＝OBNG 命名。我々は v608 で同関数を **user0/user1/user2（スライドの名前・説明・有効フラグ）設定**として解析済み。両者は同関数の別側面で矛盾しない（[部分確定]）。
- `0x5AD980` の **x87 精度制御 PC=24 の丸め**（`FADD`/`FDIV`/`FSUBP` の3箇所）＝OBNG の in-game 実測（`_control87` 0x000A001F）。**我々の C++ port には不要**（移植先は IEEE 倍精度/単精度）。移植時に参照する価値は無い。
- `TimeInfo` のフィールド名（`clockMs` 等）は OBNG 命名。

---

## 5. 移植への指針（LoadingMenu）

1. **フィールド表 §4.3 をそのまま採用**（`+0x3C slowStepCap`・`+0x40 screenTimeLeft`・`+0x4C/0x50` リスト・`+0x54` TimeInfo・`+0x60 shownStep`・`+0x64 targetStep`・`+0x68 stepChangedMs`・`+0x6C lastTickMs`・`+0x70 fastStepEnabled`）。
2. **設定名 §4.2 をそのまま使う**（`fLoadingSlideDelay`/`iMoveBarMaxMilliseconds`/`iMoveBarChaseMilliseconds`/`iMoveBarWaitingMilliseconds`・`fPercentageOfBar0..3`＝0.06/0.35/0.44/0.15）。
3. **時間は ms**。バー trait は **user3（`0xFB1`）**。ctor の `screenTimeLeft` 初期値＝15.0 秒。
4. OBNG の `LoadingMenu::Tick`/`LoadingMenu::LoadingMenu` は **verified**（shadow 0 mismatch）だが、**我々の port は C++ で x87 再現不要**のため、その実装（`LoadingMenu_TickLogic.h` 等）は **参照のみ**（GPL・コピー禁止）。

---

## 6. 残点

- **V-611-1【中】**: OBNG `re/symbols/xobse.csv`（2004 件）の実名を我々の `named_funcs.txt` へ取り込むか。**xOBSE は GPL** のため、ライセンス判断のうえ別途決定（本裁定は事実記録のみ）。
- **V-611-2【低】**: `0x5AD780` の完全 decode（我々 v608 は user1/user2 側面のみ。OBNG の「RefreshScreen」名の裏取り）。
- **V-611-3【低】**: OBNG の verified 関数（`TimeInfo::Update 0x47D170`・`PlanHavokSteps 0x889810` 等）は我々の port 範囲外。参照価値のみ。

---

## 7. 検証

- clone: `git clone --depth 1 https://github.com/Naragorn/OBNG.git` → 1124 files / 11.6 MB / commit `b9526a6`。
- md5: 手元 EXE `cdd2f0c5eff198d4f26b7b5b54ce4930` ＝ OBNG README 記載値（**一致**）。
- probe **u43_1 / u44_1**（EXIT=0・冒頭 Test-Path・recovery 一致）。
- 定数実測: `0xA2FC70`=1000.0(f64)／`0xA2FC78`=4294967296.0(f32)／`0xA6C4C8`=99.9999008178711(f64)／`0xB14158`=15.0／`0xB14160`=2500／`0xB14168`=100／`0xB14170`=10／`fPercentageOfBar0..3`=0.06/0.35/0.44/0.15。
- 設定名実測: `0xA6C610`=`fLoadingSlideDelay:LoadingBar`／`0xA6C630`=`iMoveBarMaxMilliseconds:LoadingBar`／`0xA6C654`=`iMoveBarChaseMilliseconds:LoadingBar`／`0xA6C67C`=`iMoveBarWaitingMilliseconds:LoadingBar`／`0xB14138/40/48/50`=`fPercentageOfBar0..3:LoadingBar`。
- 主要生バイト: `0x5AD980..0x5ADB3E`（Tick・447B）／`0x5AD0D0..`（ctor・`push 0x18;call 0x401F00`→`TimeInfo::Init 0x47D150`→`[+0x54]`→`[+0x40]=15.0f`）。
- 照合成果物: `%TEMP%\opencode\obng_merged_symbols.csv`（2255 行）・`obng_crosscheck.out`（**リポジトリへは未コミット**）。
