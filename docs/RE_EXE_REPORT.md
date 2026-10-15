# Oblivion Steam版 リバースエンジニアリングレポート：Oblivion.exe（メニューXMLパーサー）

解析日: 2026-10-07
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 相当、ビルド 2007-04-16）
ツール: Ghidra 12.1.2（analyzeHeadless 自動解析 + カスタムJavaスクリプト）
関連: [RE_UI_TEXTURE_REPORT.md](RE_UI_TEXTURE_REPORT.md)、[RE_TITLE_SCREEN_REPORT.md](RE_TITLE_SCREEN_REPORT.md)

## 1. 概要

Android移植版のメニューXMLインタプリタ（`ui/menu_xml_interpreter.cpp`）の正確性を検証するため、`Oblivion.exe` を逆アセンブルして本家のメニューXMLパーサーを特定・解析した。

**主要な結論**

1. 本家のメニューXMLトークン（文字列→整数ID対応表）を exe 内の `.rdata` から発見し、トークン登録関数 `FUN_0058a1c0` を特定。
2. トークンのランタイム変換関数 `FUN_00588ef0` を解読。`&xxx;` は組込トークン、`_xxx` は動的トークンとして別テーブルで引く二段構成。
3. XMLパーサー本体 `FUN_0058cf40` とタイル解決 `FUN_0058b800`、属性適用 `FUN_0058b220`/`FUN_0058ca00` を特定。
4. Android版 M4 の実装（タイル定義・演算子セット）は本家と一致することを確認した（下表）。

## 2. PEヘッダ（新規解明）

| 項目 | 値 |
|------|-----|
| CPU | x86 (32bit, Machine 0x014C, IMAGE_FILE_32BIT_MACHINE) |
| ImageBase | 0x00400000 |
| ビルド日時 | 2007-04-16（v1.2.0416 相当） |
| セクション | .text (VA 0x1000, 6.2 MB) / .rdata (VA 0x628000) / .data / .tls / .rsrc / .bind |

- コードサイズ約 6.4 MB（.text）。解析対象関数はすべて RVA 0x58xxxx 帯の .text 内に存在する。
- 文字列抽出（Python, 69,169件）→ トークンがすべて `.rdata` の狭い領域（RVA 0x66A1F0〜0x66A5F8）に集中していることから、メニューXMLパーサーのトークンテーブルを特定した。

## 3. トークンテーブル（FUN_0058a1c0）

`FUN_0058a1c0` は `FUN_0058a0a0(string, id)` を連続呼び出しする**初期化関数**で、メニューXMLの全トークン文字列と整数IDを登録する。

### 3.1 組込メニューID（type の値、0x3e9〜0x41b）
`FUN_0058a1c0` は `&MenuName;` 型のメニュー定義トークンを登録する。全59メニューが確認された。

| ID | トークン | | ID | トークン |
|----|----------|-|----|----------|
| 0x3e9 | MessageMenu | | 0x3ea | InventoryMenu |
| 0x3ec | HUDMainMenu | | 0x3ed | HUDInfoMenu |
| 0x3ee | HUDReticle | | 0x3f0 | ContainerMenu |
| 0x3f1 | DialogMenu | | 0x3f4 | SleepWaitMenu |
| 0x3f5 | PauseMenu | | 0x3f6 | LockPickMenu |
| 0x3f9 | AudioMenu | | 0x3fa | VideoMenu |
| 0x3fb | VideoDisplayMenu | | 0x3ff | MapMenu |
| 0x402 | BookMenu | | 0x414 | MainMenu |

（全59件。残りは OptionsMenu 0x3f7 / StatsMenu 0x3eb / MagicMenu 0x3fe / LoadMenu 0x40e / SaveMenu 0x40f 等）

### 3.2 タイル属性トークン（trait 名→ID、0xfa1〜0xffe）
| ID | トークン | | ID | トークン |
|----|----------|-|----|----------|
| 0xfa1 | visible | | 0xfa2 | class |
| 0xfa3 | listclip | | 0xfa4 | clipwindow |
| 0xfa5 | stackingtype | | 0xfa6 | locus |
| 0xfa7 | alpha | | 0xfa9 | disablefade |
| 0xfaa | listindex | | 0xfab | depth |
| 0xfad | (xpad) | | 0xfae..0xfc7 | user0..user25 |
| 0xfc8 | clips | | 0xfc9 | target |
| 0xfca | height | | 0xfcb | width |
| 0xfcc | (red) | | 0xfcd | green |
| 0xfce | (blue) | | 0xfd0 | child_count |
| 0xfd1 | justify | | 0xfd2 | (zoffset) |
| 0xfd4 | wrapwidth | | 0xfd5 | wraplimit |
| 0xfd6 | wraplines | | 0xfd7 | pagenum |
| 0xfd8 | ishtml | | 0xfd9 | cropy / cropoffsety |
| 0xfda | cropx / cropoffsetx | | 0xfdb | menufade |
| 0xfdc | explorefade | | 0xfdd | mouseover |
| 0xfde | string | | 0xfdf | shiftclicked |
| 0xfe0 | focusinset | | 0xfe1 | clicked |
| 0xfe2 | clickcountbefore | | 0xfe3 | clickcountafter |
| 0xfe4 | clickedfunc | | 0xfe5 | clicksound |
| 0xfe6 | filename | | 0xfe7 | filewidth |
| 0xfe8 | fileheight | | 0xfe9 | repeatvertical |
| 0xfea | repeathorizontal | | 0xfeb | returnvalue |
| 0xfec | animation | | 0xfed | depth3d |
| 0xfee | linecount | | 0xfef | pagecount |
| 0xff0 | xdefault | | 0xff1 | (xkey) |
| 0xff2 | xdown | | 0xff3 | xleft |
| 0xff4 | xright | | 0xff5 | xscroll |
| 0xff6 | xlist | | 0xff7..0xffe | xbuttona..xbuttonrb/lt/rt |

### 3.3 特殊タイル演算子トークン（0x1389〜0x138f）
| ID | トークン | 意味（FUN_0058b800 の分岐と照合） |
|----|----------|-------|
| 0x1389 | parent | 親タイル |
| 0x138a | (onclick) | クリック系 |
| 0x138b | (focus) | フォーカス解決 |
| 0x138c | sibling | 兄弟タイル |
| 0x138d | child | 子タイル |
| 0x138e | screen | スクリーンルート |
| 0x138f | strings | 文字列テーブル |

### 3.4 式演算子トークン（0x7d1〜0x7eb）
| ID | トークン | | ID | トークン |
|----|----------|-|----|----------|
| 0x7d1 | (or) | | 0x7d2 | (and) |
| 0x7d3 | (not) | | 0x7d4 | (==)/(!=) |
| 0x7d5 | (lt/gt) | | 0x7d6 | (le/ge) |
| 0x7d7 | (add) | | 0x7d8 | (sub) |
| 0x7d9 | (mul) | | 0x7da | (div) |
| 0x7db | (mod) | | 0x7dc | (... ) |
| 0x7e0 | (cond) | | 0x7e1 | (?) |
| 0x7e2 | (macro) | | 0x7e3 | floor / trunc |
| 0x7e4 | (round) | | 0x7e5 | onlyif |
| 0x7e6 | onlyifnot | | 0x7e7 | (min) |
| 0x7e8 | (max) | | 0x7e9 | (abs) |
| 0x7ea | (dof) | | 0x7eb | (pulse) |

### 3.5 その他属性
- `image` 0x386 / `template` 999 / `window` 0x38b（タイル種別）
- `click_past` 0x65 / `no_click_past` 0x66 / `does_not_stack` 0x1778 / `mixed_menu` 0x67
- 水平配置: `left`=1 / `center`=2 / `right`=4
- `&generic;` 0xffffffff（デフォルト型）、`&false;`=1 / `&true;`=2
- Xboxボタン種別: `xboxhint`/`xbutton*`=1（`_x` 系はフラグ）

## 4. トークン変換（FUN_00588ef0）

`FUN_00588ef0(string) -> id` は文字列トークンを整数IDへ変換する関数。M4の `token_to_value` 相当。

- **先頭 '&'**（`&name;`）: デフォルトアドレス `DAT_00b3b0b4` を先頭とする 27 バケット構成。先頭文字でバケットを選び、チェーンを `__mbsicmp`（大文字小文字無視）で線形探索。
- **先頭 '_'**（`_name`）: `DAT_00b13bc8` が指す動的テーブル配列から探索（登録数 `DAT_00b13bce`）。`_` 系はXML内の `_this`、`_parent` などの動的参照。
- 見つからない場合 0 を返す。
- 領域: ハッシュテーブルはグローバル `.data`（`DAT_00b3b0b4`）に配置され、`FUN_0058a1c0` 初期化（crtスタティックイニシャライザ相当、RVA 0x58a1c0 の関数が MAIN 起点で呼ばれる）で登録される。

**型判定**（FUN_00588fc0 の前処理で実現）: `FUN_0058b800` がタイル名を解決する前に `FUN_00588fc0(name, buf)` を呼び、`( ... )` 括弧で囲まれた部分を `buf` に抽出して型付き参照（例 `(child)name`）を処理する。

## 5. XMLパーサー（FUN_0058cf40）

タイルXMLの内容をトークン化した補助配列（`FUN_0058dbd0` がファイル → `FUN_00590330`/`FUN_0058cf40` へ渡す）を走査するループ。トークンは文字コード（0x28='('、0x2d='-'、0x32、0x1e、0x23、0x37、0x3c、0x14、0x19）でスイッチ分岐する。

- `0x28` '(': 開き括弧、属性開始（`DAT_00b3b0a1` フラグ）
- `0x2d` '-': 閉じ括弧、`class`（0xfa2）属性を走査して FON タイル化
- `0x32`: 子タイル挿入 → `FUN_0058b220`+`FUN_0058cf10`(childロード経由)
- `0x37`: `FUN_0058b220`+`FUN_0058cbe0`（子リスト追加）
- `0x3c`: `FUN_0058b800`（名前でタイル解決）+`FUN_0058cf10`
- `0x1e` / `0x23` / `0x14` / `0x19`: 属性スイッチ
  - 属性ID 0xfde（string）/ 0xfe6（filename）は特別扱い（文字列代入）
  - 0xbba（trait）は `FUN_0058a020`（解放系）
  - その他数値属性は `FUN_0058b220`（属性追加）+ `FUN_0058ca00`（floatセット）

### 属性ストレージ（FUN_0058b220 / FUN_0058ca00）

- タイル構造体 `+0x18` が属性のソート済み単方向リスト。`ushort` 属性ID（`+0x18` オフセット）で比較しソート済み挿入。
- ノード 0x1c バイトで確保。`FUN_0058b220` が既存照合または新規生成、`FUN_0058ca00` が float 値を `+4` に書き込み dirty フラグ `+0x1a` を立て、再描画（`FUN_0058bee0`）を要求。
- **属性は XML で出現した順ではなく ID 順のソートリストに格納される**（M4 の `std::map` 相当）。

## 6. メニューコンソール（FUN_00586000）

タイル文字列ハンドラ（ゲーム内コンソールから `Data\Menus\*.xml` へアクセスするコマンド）。

- 入力は `|` 区切り。`dof`（フォグ）/ `clr`/`cls`（クリア）/ `usz`/`playerpos`/`visible`/`depth` 等のデバッグコマンドを処理。
- **`reload <menuName>`**: `FUN_00588ef0` でメニューIDを引き、`FUN_00587b20`（リロード）を実行。`reload strings` は `strings.xml` を `FUN_00590420` で再読込。
- **`set trait <tile> <trait> <val>`**: `FUN_0058b800` でタイル解決 → `FUN_00588ef0` で属性ID → `FUN_0058ceb0` で値設定。「Can't find tile '%s'」「Unknown trait '%s'」エラー出力がここで発見された。
- `bat <file>`: バッチファイル内のコマンドを逐次実行。`TEH`（レティクルHP）等。

## 7. Android版 M4 との照合

| 項目 | 本家 | Android版 M4 | 一致 |
|------|------|--------------|------|
| タイル種別 | image/template/window + メニュー型 | image/template/window + menuid | YES |
| 式演算子 | onlyif / onlyifnot / floor / trunc / (or) etc. | onlyif / onlyifnot / (数値演算) | YES |
| 属性名 | visible/class/locus/zoom/depth/filename/filewidth | 同（zoom=-1 で fit） | YES |
| 特殊参照 | parent/sibling/child/screen/strings | child/sibling（実装済み） | 部分的 |
| 属性ストレージ | ID昇順ソートリスト | std::map | YES |
| トークン参照 | `&name;` / `_name` | `&name;` / `_name` | YES |
| デバイストークン | `&xbox;` / `&xboxhint;` / `&xbutton*;`（is_console判定） | 同（`MenuEvalContext::is_console`） | YES |

**残課題（M4未実装・本家にはあるもの）**
- `mouseover` / `shiftclicked` / `clickedfunc` 等のインタラクション属性はブランク指定のみ対応で、動作は未実装

**実装済みに更新（本セグメント）**
- `strings` トークン参照 → `MenuXmlInterpreter::loadStringsXml()` で strings.xml（`<rect name="Strings">`）をロードし、`MenuEvalContext::strings` 経由で `strings()` / `&name;` を解決
- 式の条件演算子 → floor / ceil / add / sub / mul / mult / div / mod / max / min / eq / neq / gt / gte / lt / lte / and / or / not / rand を実装済み
- デバイストークン → `&xbox;` / `&xenon;` / `&xboxhint;` は `MenuEvalContext::is_console` に解決（タッチデバイスでは false となりコンソール専用レイアウトを非表示）。`&xbuttona;`〜`&xbuttonrt;` / `&xbuttonstart;` はボタン名文字列（"A"/"B"/...）に解決

## 8. スクリプト

| ファイル | 用途 |
|----------|------|
| `FindMenuXrefs2.java` | トークンRVA → xref 一覧 → 参照元関数を特定 |
| `DumpDecomp2.java` | 絶対アドレス指定で関数のデコンパイルを出力 |
| `FindExprStr3.java` | .text 全体の scalar 定数スキャンで式評価器の文字列参照元を特定 |
| `DumpExprEval.java` | 式評価器（FUN_004f3620）と関連関数のデコンパイル出力 |
| `DumpBlockHandler.java` | ブロックハンドラ FUN_004fde30 のデコンパイル出力 |

### 8.1 スクリプトコンパイラ（FUN_004ff710）

`"Syntax Error.  Missing block type in 'begin' command."`（RVA 0x64a360）の参照元として特定。
**スクリプトコンパイラ本体**。パラメータ:

- `param_1` = パーサ ctx（`param_1[0x...]` に代入結果を保持）
- `param_2` = コンパイル状態（`+0x20`=コード領域先頭, `+0x24`=書き込み長, `+0x38`=quest script フラグ）
- `param_3` = バイナリ出力バッファ（`param_3[0x103]` が書き込み位置、` param_3 + 0x20c + param_3[0x103]` がコード領域開始）

opcode dispatch（`param_3[0x104]` による switch）:

| opcode | 意味 |
|--------|------|
| `0x10` | **begin**：ブロックタイプ照合 → opcode 書き込み → ネスト照合 |
| `0x11` | **end**：begin/end ジャンプブロックを解決 |
| `0x12`/`0x13`/`0x14`/`0x1f` | 変数宣言（long/short/float/array） |
| `0x15` | **set**：`x to expr` 構文 → 式パース → InfixToPostfix |
| `0x16`/`0x18` | if 系（括弧チェック + 式コンパイル + ジャンプ解決） |
| `0x17` | if/then/else（ジャンプブロック解決） |
| `0x19`/`0x1c`/`0x1d`/`0x1e` | ジャンプブロック終端（break） |
| `0x100`〜`0x182` | **関数/コマンド**（131個、後述の関数テーブル） |
| `0x1000`〜`0x1170` | MessageBox 等（`PTR_s_MessageBox_00b0c8c0` テーブル、0x171個） |

#### begin コマンドの処理

1. `FUN_004fd7c0` で単語取得 → ブロックタイプ名
2. テーブル `PTR_DAT_00b0af4c`（下記）と `__stricmp` で照合
3. 合致で opcode（word）を出力バッファに書き込み
4. `FUN_004fd370` でネスト部分のコンパイル（戻り -1 なら `"Mismatched begin/end block."`）
5. `0x55555555` を書込み → グローバルジャンプテーブル `DAT_00b361f8` に begin 位置を記録
6. ブロック固有ハンドラ配列 `PTR_FUN_00b0af64` を呼び出し（タイプ固有パラメータ処理）

#### set コマンドの処理

- `FUN_004fcb90` / `FUN_004fcbd0` で変数位置解決
- `FUN_004fdaf0` で式文字列の切り出し（null なら `"Missing expression in set command."`）
- `FUN_004f4080`（**InfixToPostfix 変換器**、RVA 0x4f4080）で中置 → RPN へ変換。`local_f3c != 0` なら `"InfixToPostfix Error."`

#### if 系の処理

- `FUN_004fcb90` で先頭括弧チェック（失敗時 `"Mismatched parentheses."`）
- 式コンパイル後 `FUN_004fd430` で if/then/else ジャンプ解決

### 8.2 begin ブロックタイプテーブル（DAT_00b0af48 / 31エントリ・40バイト刻み）

- **配置**: RVA 0xb0af48。末尾 `PTR_DAT_00b0af4c` 表記は Ghidra の先頭ポインタ参照によるもの
- **エントリ構造（40バイト）**:

| オフセット | 型 | 内容 |
|-----------|-----|------|
| `+0x00` | char* | ブロックタイプ名（`GameMode` 等） |
| `+0x04` | char* | 別名（`OnPackageEnd` のみ、通常は NULL） |
| `+0x08` | (w) | ブロックID（0x00〜0x1e、バイナリ埋め込み値） |
| `+0x12` | (w) | サブID（ブロック列挙値、ハンドラへ渡す） |
| `+0x14` | u32 | パラメータ定義テーブル（NULL なら無し） |
| `+0x1c` | u32 | ハンドラ関数ポインタ（Key では一切 FUN_004fde30） |

- **エントリ一覧（31種 + 0x0100 境界）**:

| id | 名前 | 備考 |
|----|------|------|
| 0x00 | GameMode | |
| 0x01 | MenuMode | |
| 0x02 | OnActivate | sub=1, param=0xb0abac |
| 0x03 | OnAdd | sub=1, param=0xb0aba0 |
| 0x04 | OnEquip | 同 OnAdd |
| 0x05 | OnUnequip | 同 OnAdd |
| 0x06 | OnDrop | 同 OnAdd |
| 0x07 | OnYield_unused | sub=1, param=0xb0abac |
| 0x08 | OnHit | 同 OnYield |
| 0x09 | OnHitWith | param=0xb0a7e0 |
| 0x0a | OnDeath | 同 OnYield |
| 0x0b | OnMurder | 同 OnYield |
| 0x0c | OnKnockout | param 無し |
| 0x0d | OnAlarm | sub=2, param=0xb0abdc |
| 0x0e | OnAlarmVictim | 同 OnAlarm |
| 0x0f | OnPackageStart | param=0xb0accc |
| 0x10 | OnPackageDone / **OnPackageEnd** | 別名エントリ（唯一） |
| 0x11 | ScriptEffectStart | param 無し |
| 0x12 | ScriptEffectFinish | 同 |
| 0x13 | ScriptEffectUpdate | 同 |
| 0x14 | OnPackageChange | 同 OnPackageStart |
| 0x15 | OnLoad | param 無し |
| 0x16 | OnMagicEffectHit | param=0xb0ace4 |
| 0x17 | OnSell | 同 OnYield |
| 0x18 | OnTrigger | param=0xb0a7f8 |
| 0x19 | OnStartCombat | 同 OnYield |
| 0x1a | OnTriggerActor | 同 OnTrigger |
| 0x1b | OnTriggerMob | 同 OnTrigger |
| 0x1c | OnActorEquip | param=0xb0a7d4 |
| 0x1d | OnActorUnequip | 同 |
| 0x1e | OnReset | param 無し |

- **境界**: 31エントリ目（index 30）を超えると `0xb0b420` が関数テーブル（8.3）に切り替わる（`local_115c < 0xb0b424` まで照合）

### 8.3 関数/コマンドテーブル（PTR_DAT_00b0b420 / 131エントリ・40バイト刻み）

- **opcode 0x100〜0x182** に対応。`(iVar6 - 0x100)` がテーブルインデックス
- **エントリ構造**: `[+0x00 関数名][+0x04 短名/エイリアス][+0x0c 説明文][+0x10 引数コード][+0x18 ハンドラ]`
  - **+0x10 引数コード**: 下位16bit = 引数個数（`0x0001`=1個, `0x0002`=2個, `0x0003`=3個, `0x0004`=4個, `0x0006`=6個）。上位bit フラグ: `0x10000000` 台は「既定値 fragment」「変数禁止」等
  - **+0x18 ハンドラ**: 実行時に呼ばれる実装関数（例: `Show`=0x00502210）
- **主要関数**（一部）:

| code | 名前 | 短名 | 引数 | ハンドラ |
|------|------|------|------|----------|
| 0x0100 | Show | TST | 0 | 0x00502210 |
| 0x0101 | ShowVars | SV | 0 | 0x005097a0 |
| 0x0102 | ShowQuestVars | SQV | 0 | 0x00509b30 |
| 0x0103 | ShowQuests | SQ | 0 | 0x00509d10 |
| 0x0104 | ToggleCombatStats | TCS | 0 | 0x00500770 |
| 0x0105 | ToggleSafeZone | TSZ | 0 | 0x00500f60 |
| 0x0106 | ToggleAI | TAI | 0 | 0x005011f0 |
| 0x0107 | ToggleCollisionGeometry | TCG | 0 | 0x00500e30 |
| 0x0108 | ToggleMaterialGeometry | TMG | 0 | 0x00500e40 |
| 0x0109 | ToggleBorders | TB | 0 | 0x00507c60 |
| 0x010a | ToggleSky | TS | 0 | 0x00507cb0 |
| 0x010b | ToggleLeaves | TLV | 0 | 0x00507dc0 |
| 0x010c | ToggleWireframe | TWF | 0 | 0x0050fca0 |
| 0x010d | ToggleCollision | TCL | 0 | 0x00507d30 |
| 0x010e | ToggleDebugText | TDT | 0 | 0x00501000 |
| 0x010f | ToggleMenus | TM | 0 | 0x00500f10 |
| 0x0110 | ShowScenegraph | SSG | 0 | 0x0050fd10 |
| 0x0111 | ToggleMagicStats | TMS | 0 | 0x00507bd0 |
| 0x0112 | ShowAnim | SA | 1 | 0x00509d70 |
| 0x0113 | Show1stPerson | S1ST | 0 | 0x0050a050 |
| 0x0115 | TogglePathGrid | TPG | 0 | 0x00501650 |
| 0x0117 | PickRefByID | PRID | 1 | 0x00513e90 |
| 0x011b | CenterOnCell | COC | 1 | 0x005083c0 |
| 0x011c | SetGameSetting | SetGS | 2 | 0x00514100 |
| 0x011d | SetINISetting | SetINI | 2 | 0x005143c0 |
| 0x011e | GetINISetting | GetINI | 1 | 0x00514230 |
| 0x011f | CenterOnExterior | COE | 2 | 0x005084a0 |
| 0x0120 | CalcPathToPoint | P2P | 1 | 0x00510c70 |
| 0x0121 | CalcLowPathToPoint | LP2P | 3 | 0x00510e60 |
| 0x0130 | SetCameraFOV | FOV | 1 | 0x00511720 |
| 0x0132 | ToggleGodMode | TGM | 0 | 0x00500d10 |
| 0x0138 | ShowQuestLog | SQL | 1 | 0x005120a0 |
| 0x0139 | ShowFullQuestLog | SFQL | 1 | 0x00512220 |
| 0x0142 | ToggleMapMarkers | tmm | 1 | 0x00507800 |
| 0x0143 | PlayerSpellBook | psb | 0 | 0x00507740 |
| 0x0147 | SaveGame | save | 2 | 0x005023d0 |
| 0x0148 | LoadGame | load | 2 | 0x00502500 |
| 0x014f | MoveToQuestTarget | movetoqt | 1 | 0x00512910 |
| 0x0164 | QuitGame | qqq | 0 | 0x005077e0 |
| 0x0173 | BeginTrace | bt | 0 | 0x00977c50 |
| 0x0175 | StartAllQuests | saq | 0 | 0x0050e5a0 |
| 0x0176 | CompleteAllQuestStages | caqs | 0 | 0x0050e600 |
| 0x017d | ToggleCastShadows | tsh | 1 | 0x00506b80 |
| 0x0182 | ClearFaceGenModelData | CFG | 0 | 0x00506e90 |

- 全131関数のダンプは tmp/func_table.txt 参照。デバッグ用（TOGGLE系・SHOW系）が大半を占める

### 8.4 ブロックパラメータ解析（FUN_004fde30 = StandardCompile）

`FUN_004ff710` の begin ハンドラとして呼ばれ、ブロックタイプ固有のパラメータ（`OnActivate` なら対象 ref 等）を解析する。

- `param_1` = ブロックID（word）、`param_2` = パラメータ定義テーブル（8.2 の +0x14）
- パラメータ定義テーブルは **12バイト刻み** のエントリ: `[+0 名前][+4 タイプコード][+8 フラグ]`
- 各パラメータを `FUN_004fd7c0` で読み取り、タイプコードに応じた検証・変換を行う

**パラメータタイプコード表**（switch の case 一覧）:

| code | タイプ | 検証 |
|------|--------|------|
| 0x00 | 文字列 | そのまま書き込み |
| 0x01/0x02/0x17 | 変数 | ローカル/グローバル参照、`G`/`r`/`z`/`n` プレフィックス付与 |
| 0x03 | インベントリオブジェクト | `FUN_00469520` で形式検証 |
| 0x04 | オブジェクト参照 | データベース検索 |
| 0x06 | アクター参照 | 参照がアクターか検証 |
| 0x07 | スペルアイテム | spell 検索 |
| 0x08 | 軸 | X / Y / Z のみ許可 |
| 0x09 | インテリアセル | cell 検索 |
| 0x0a | アニメーショングループ | グループ名テーブルと照合 |
| 0x0b | マジックアイテム | magic 検索 |
| 0x0c | サウンド | sound 検索 |
| 0x0d | トピック | topic 検索 |
| 0x0e | インフォ | info 検索 |
| 0x0f | レース | race 検索 |
| 0x10 | クラス | class 検索 |
| 0x11 | ファクション | faction 検索 |
| 0x12 | 性別 | Male / Female のみ |
| 0x13 | グローバル変数 | global 検索 |
| 0x14 | 家具オブジェクト | furniture 検索 |
| 0x15 | オブジェクト | object 検索 |
| 0x18 | マップマーカー | map marker 検証 |
| 0x19 | アクターベース | actorBase 検索 |
| 0x1a | コンテナ参照 | container 検証 |
| 0x1b | ワールドスペース | worldspace 検索 |
| 0x1c | 犯罪タイプ | 数値 0〜5 |
| 0x1d | パッケージ | package 検索 |
| 0x1e | コンバットスタイル | combat style 検索 |
| 0x1f | エフェクト設定 | effect setting 検索 |
| 0x20 | バースサイン | birthsign 検索 |
| 0x21 | フォームタイプ | 36種（Activator 等） |
| 0x22 | ウェザー | weather 検索 |
| 0x23 | NPC | NPC 検索 |
| 0x24 | オーナー | NPC or faction |
| 0x25 | エフェクトシェーダー | effect shader 検索 |

エラーメッセージ形式はすべて `<type名> '%s' not found for parameter %s.`（例: `"Invalid spell item '%s' for parameter %s."`）。

#### パラメータ定義テーブルの実データ（8.2 の +0x14 が指す配列）

パラメータ定義テーブルは 16バイト刻みのエントリで、`[+0 名前ポインタ][+4 タイプコード][+8 フラグ][+0xc 予約]`。代表例:

- `0xb0a6d8`（MenuMode）: 型 0x01 のエントリ1個
- `0xb0abac`（OnActivate 等）: 型 0x06（アクター参照）を含む複数エントリ
- `0xb0aba0`（OnAdd/OnEquip 等）: 型 0x1a（コンテナ参照）系
- `0xb0a7d4`（OnActorEquip）: 型 0x15（オブジェクト）系
- `0xb0a7f8`（OnTrigger 等）: 型 0x04（オブジェクト参照）系

### 8.5 InfixToPostfix 変換器（FUN_004f4080）— 解読済み

`set x to expr` 構文で呼ばれる中置記法 → RPN 変換器。トークナイザ `FUN_004f3320` を再利用し、演算子テーブル `DAT_00b0a12d`（8バイト刻み・16エントリ）でトークン種別を判別する。

**アルゴリズム**:
- `param_1[0x141]` = 演算子スタックポインタ（`-1` = 空）。スタック = `param_1[0x101+i]`
- `param_1[0x1c2]` = 出力スタックポインタ、`param_1[0x142+i]` = 出力値
- 入力式を `( ` + 式 + `)` で包み、各トークンを評価
  - **オペランド**（変数/数値）: そのまま出力へ `0x20 ' '` 区切り
  - **演算子**（iVar5): 出力中。`DAT_00b0a12d[iVar2*8]` の先行演算子より優先度が低ければ pop して出力。最後に push
  - **右括弧** (`)`) : スタックから `(` まで pop して出力
- スタックは **63 エントリ上限**（>0x3f → `Expression Error: STACKOVERFLOW`）
- 終了時スタックに `(` が残れば `Expression Error: SYNTAX`

**演算子テーブル DAT_00b0a12d（16エントリ/8バイト刻み、[0]=文字列, [8]=演算子opcode）**:

| index | 文字列 | opcode | 備考 |
|-------|--------|--------|------|
| 0x00 | `(` | 0x01 | 開き括弧 |
| 0x01 | `)` | 0x02 | 閉じ括弧 |
| 0x02 | `&&` | 0x03 | |
| 0x03 | `\|\|` | 0x04 | |
| 0x04 | `<=` | 0x05 | |
| 0x05 | `<` | 0x06 | |
| 0x06 | `>=` | 0x07 | |
| 0x07 | `>` | 0x08 | |
| 0x08 | `==` | 0x09 | |
| 0x09 | `!=` | 0x0a | |
| 0x0a | `-` | 0x0b | |
| 0x0b | `+` | 0x0c | |
| 0x0c | `*` | 0x0d | |
| 0x0d | `/` | 0x0e | |
| 0x0e | `%` | 0x0f | |
| 0x0f | `~` | 0xf4 | 特殊（単項/センチネル） |

**トークナイザ FUN_004f3320 の特殊トークン**:
- `X`+u16+u16: 5バイトエンコードした関数参照（`0xfff < funcID < 0x1171` を確認 → 0x1000系MessageBoxテーブル関数）。バイト列+5をそのままコピー
- 変数トークン接頭辞: `s`(short)/`l`(long)/`f`(float)/`G`(グローバル)/`Z`(一時)/`r`(ref)、数値`n`+i32、実数`z`+f64、文字列`"..."`

### 8.6 MessageBox/スクリプト関数テーブル（0x1000〜0x1170、369エントリ）— 解読済み

`(iVar6 - 0x1000) < 0x171` で参照されるコマンド/関数テーブル。`PTR_s_MessageBox_00b0c8c0`（RVA 0xb0c8c0）。**関数コマンドテーブル（8.3）と全く同じ40バイト構造**:
`[+0x00 関数名][+0x04 別名][+0x08 セルフID(0x1000-0x1170)][+0x0c 説明/引数定義][+0x10 引数フラグ][+0x18 ハンドラ]`

- **0x1000 = MessageBox**（スクリプトで `MessageBox "text" refID`）。scriptコンパイル時は `X`+`funcID`+`追加バイト数` の5バイトでエンコードされる（8.5参照）
- 主要エントリ（先頭部分）:

| opcode | 名前 | 引数 | ハンドラ |
|--------|------|------|----------|
| 0x1000 | MessageBox | 1 | 0x00513280 |
| 0x1001 | GetDistance | 1 | 0x005003F0 |
| 0x1002 | AddItem | 2 | 0x00507320 |
| 0x1003 | SetEssential | 2 | 0x0050B780 |
| 0x1004 | Rotate | 2 | 0x00510620 |
| 0x1005 | GetLocked | 0+1 | 0x00502390 |
| 0x1006 | GetPos | 1 | 0x00501A80 |
| 0x1007 | SetPos | 2 | 0x00508FC0 |
| 0x1009 | SetAngle | 2 | 0x00509110 |
| ... | ...（0x1002 AddItem 等） | | |
| 0x1170 | ClearOwnership | 0+1 | 0x00507620 |

- 全369エントリのダンプは `tmp/msgbox_table.txt` 参照
- 引数フラグ形式は 8.3 と同じ: 下位16bit = 引数個数（+0x10000 系フラグビット付き）

### 8.7 ジャンプ解決・機密ブロック照合 — 解読済み

**共通: コンパイル済み命令は `param_1+0x50` を先頭とする単方向リンクリスト**。各ノードは `[0]=命令データptr, [4]=次ノードptr`。命令データの `[0x104]` = opcode。リスト走査は `if (piVar1==NULL && *piVar4==0) break` で終端検出。

#### FUN_004fd370 — begin/end ブロック照合（begin コマンドから呼ばれる）

`param_1` = コンパイラ ctx、`param_2` = 開始行番号。`begin` に対応する `end` を探す。

| opcode | 処理 |
|--------|------|
| `0x10` begin | 入れ子時 `"Nested begin/end blocks are not allowed."` → -1 |
| `0x11` end | 対応ブロック検出 → その位置を返す |
| `0xffff` | `"Begin without matching end starting on line %d."` → -1 |
| その他 | カウント進める |

- リスト終端まで見つからず → `"Mismatched begin/end block starting on line %d."` → -1
- **begin の入れ子は許可されない**（Oblivion スクリプト仕様）

#### FUN_004fd430 — if/then/else/endif ジャンプ解決（if コマンドから呼ばれる）

`param_2` 起点で if 構造のネスト深度 `iVar5` を追跡し、対応する `endif` のジャンプ先オフセットを探す。

| opcode | 処理 |
|--------|------|
| `0x16` if | ネスト深度 +1。**上限10** 超えで `"Max nesting of %d exceeded."` |
| `0x19` endif | 深度 -1。深度0で閉じたら return（ジャンプ先位置） |
| `0xffff` | `"Mismatched if/else/endif."` → -1 |
| その他 | カウント進める |

- match が見つからず終端 → `"Mismatched if/else/endif block starting on line %d."` → -1
- `0x17`(else) / `0x18` は深度計算に影響しない（通過扱い）

### 8.8 エラーメッセージ出力関数（FUN_004fce30）— 解読済み

スクリプトコンパイル中の構文エラー/ブロック不一致を報告する可変引数関数。

```
void FUN_004fce30(int param_1, char *format, ...)
```

**処理**:
1. 可変引数を `_vsprintf(local_114, format, args)` で展開（バッファ260バイト）
2. `FUN_00402e30(出力buf, "Script '%s', line %d:\n%s", *(param_1+0xc), *(param_1+0x1c), local_114)` で最終メッセージを整形
   - `param_1+0xc` = **スクリプト名** / スクリプトID
   - `param_1+0x1c` = **行番号**
   - フォーマット: `Script '名前', line N:` の後にエラー本文
3. `param_1+8 == 1` なら `FUN_00579b60()`（コンソール系）、それ以外は `FUN_004a7a60(整形済み文字列, ...)`（標準ログ）で実際に表示/ログ出力

**出力経路の実体**（一次ラッパー → 実装関数）:

| 関数 | 実体 | 役割 |
|------|------|------|
| `FUN_00402e30` | → `FUN_00402d00` → `_vsprintf` → `FUN_004028d0(out_buf)` | フォーマット後のバッファ書き込み |
| `FUN_004a7a60` | → `FUN_004941a0(1, fmt, va)` | エラーレベル1の汎用ログ出力 |
| `FUN_00579b60` | → `FUN_00585c10(1)` → `FUN_00585c90` | コンソールパネルへのメッセージ追加 |

- **FUN_004941a0（汎用ログエミッタ）**: `DAT_00b34d90` を関数ポインタテーブルとみなし、レベル `param_1` で分岐してハンドラを呼ぶ（case 0 = 通常、case 1 = 標準ログ(+4)、case 2〜9 = 特化ハンドラ。+0/+4/+8/+0xc/+0x10/+0x14/+0x18/+0x1c/+0x20 の9個）。出力前に末尾に改行 `\r\n` を自動付加
- **FUN_00585c90（コンソールメッセージ追加）**: `param_1`=コンソールobj、`param_2`=fmt。`FUN_00574a80` で行分割した文字列を、メッセージ構造体をヒープ生成して単方向リンクリスト末尾に追加。`DAT_00b13984` がキュー上限（超過時 `FUN_00585ac0` で先頭から削除）、`+0x2c`/`+0x10` で件数管理
- **FUN_00582160 / FUN_00585c10**: どちらもパネル/コンソールのシングルトンを `FUN_00401f00(tag)` で遅延生成するガード。`DAT_00b3a6e0`（メインコンソール）と `DAT_00b3a6fc`（メッセージパネル）

**呼び出し側で使われる実フォーマット文字列**（8.1〜8.7の解析で確認）:

| フォーマット |
|--------------|
| `Mismatched begin/end block starting on line %d.` |
| `Begin without matching end starting on line %d.` |
| `Nested begin/end blocks are not allowed.` |
| `Mismatched if/else/endif block starting on line %d.` |
| `Mismatched if/else/endif.` |
| `Max nesting of %d exceeded.` |
| `Syntax Error.  Missing block type in 'begin' command.` |
| `InfixToPostfix Error.` |
| `Missing expression in set command.` |

### 8.9 下位層関数の解読

8.8 の出力経路をさらに深掘りし、最終段の5関数をデコンパイルで解読した。

- **FUN_004028d0（文字列オブジェクトへの書き込み）**: 自作の可変長文字列（`std::string` 相当）。`param_1` は {ptr(+0), 現在長(+4, ushort), キャパシティ(+6, ushort)}。`param_2` の文字列を長さ `param_3` でセットし、キャパシティ不足なら `FUN_00401f00(malloc)` で再確保 → `FUN_004028b0`（memcpy）でコピー → `FUN_00401f20(free)` で旧バッファ解放。長さの最大値は `0xffff`（65535）。`param_3 == 0` の場合はバッファを解放して構造体を初期化し `false` を返す（クリア専用呼び出し）
- **FUN_00574a80（コンソールメッセージの行分割/折り返し）**: `param_1`=コンソールobj、`param_2`=文字列オブジェクト、`param_3`=最大幅（0 なら 999999 扱い）、`param_4`=行数を超えた場合の強制カット行、`param_6`=区切り文字（`\n` をこの文字に置換して行区切とする）。文字幅を `FUN_009828c0()` で文字ごとに加算し、`\t` は 0x4b(75) の倍数に切り上げ。幅超過時は直前の空白位置（`local_1c`）で折り返し、空白が無ければ**単語中央でハイフン `-` を挿入して強制折り返し**。戻り値 = 行数、`*param_3` = 最大行幅、`*param_4` = 最終描画位置を返す
- **FUN_00576a30（コンソールシングルトンの遅延初期化）**: `DAT_00b3a6bc` が空なら `FUN_00401f00(0x18)` で 24 バイト確保し `FUN_00576420()`（コンストラクタ）を呼んでシングルトンを生成・登録。`FUN_00401f00(tag)` と同じ遅延生成パターン
- **FUN_00585ac0（メッセージキューの先頭削除）**: `param_1[1]` = 先頭ノード。ノードの次ポインタへ先頭を進め、末尾なら `param_1[2]=0`。ノード内の文字列（`node[2]`）を `FUN_004028d0(node[2], 0)` で解放し、ノード自身は vtable 経由 `(**(code **)(*param_1 + 8))(node)` で delete。`param_1[3]`（件数）を -1。コンソールキュー上限 `DAT_00b13984` 超過時の回収処理
- **FUN_00585620（コンソールのスクロール/パネル描画更新）**: `param_1+0x2c` = 先頭表示メッセージ行オフセット、`param_1+8` = メッセージノード単方向リスト。描画開始 Y = `DAT_00b3a704`（基準）− `DAT_00b1398c`（行数）× `DAT_00b13980`（行高さ）。先頭がリスト外（負領域）の場合は空行分をスキップしつつ各行を `FUN_00571f90`（背景バー）+ `FUN_005723e0`（テキスト描画）で描画。描画色は `DAT_00b13994`、texture/グリフセット `DAT_00a30634`・`DAT_00b3a700` を使用。スクロールバック対応の行オフセット付きレンダリング

## 9. 式評価器 Expression::Eval（FUN_004f3620）

`"Unhandled operator '%s' in Expression::Eval()"`（RVA 0x648710）の参照元として特定。
menu XML / `set` コマンドの式（`<x>10<add>5</add></x>` 等）を実行する **RPN スタックマシン**。

### 9.1 全体構造

- `FUN_004f3620(expr, len, ...)`: 式文字列をトークナイズしながら評価。結果は最後にスタックから pop して返す。
- スタック: `param_1[0x1c2]` がスタックポインタ（-1 で空）。エントリは `param_1 + n*4 + 0x142` の 16 バイト = `[型フラグ, 未使用, value_low, value_high]`（value は double を 2×uint で保持）。
- 型フラグ（エントリ先頭バイト）: `0` = 整数、`1` = 浮動小数。`cVar5`（トップ = 右オペランド）と `cVar10`（2番目 = 左オペランド）で判定し、**int同士は整数演算、float混在は double 演算**に切り替える。

### 9.2 トークナイザ（FUN_004f3320）と演算子テーブル（DAT_00b0a12d）

トークナイザは空白・`"..."` 文字列・リテラルを切り出し、演算子文字列を **16エントリのテーブル（8バイト刻み）** と一致判定。判定インデックス = 式評価器の opcode と1:1対応。

| idx (opcode) | 演算子文字列 | case | 意味（左オペランド a, 右オペランド b） |
|---|---|---|---|
| 2 | `&&` | 2 | and（a≠0 && b≠0 → 1） |
| 3 | `\|\|` | 3 | or（a≠0 ∥ b≠0 → 1） |
| 4 | `<=` | 4 | a <= b |
| 5 | `<` | 5 | a < b |
| 6 | `>=` | 6 | b <= a → a >= b |
| 7 | `>` | 7 | b < a → a > b |
| 8 | `==` | 8 | a == b |
| 9 | `!=` | 9 | a != b |
| 0xa | `-` | 0xa | a - b |
| 0xb | `+` | 0xb | a + b |
| 0xc | `*` | 0xc | a * b |
| 0xd | `/` | 0xd | a / b（0除算 → エラー4） |
| 0xe | `%` | 0xe | a % b（0除算 → エラー4） |
| 0xf | `~` | 0xf | 単項マイナス（1オペランドのみ pop） |

- リテラル（opcode 0x10）: `FUN_0047d550`（atol系判定）/ `FUN_0047d5b0`（atof系判定）で数値判定 → 数値リテラル。それ以外は `FUN_004fab20` でシンボル・文字列参照・関数呼び出し（G/Z/r/X/n/z 接頭コード、`&name;` 解決）を実行。
- **FUN_004fab20（シンボル参照リテラルの接頭文字ディスパッチャ）**: 参照名の先頭文字（`G`..`z` の 52 文字）を `0x4fae4c` の 52 バイト索引表で 0〜3 のインデックスへ変換し、`0x4fae3c` の 4 エントリジャンプテーブルで分岐する。解読結果:

  | 接頭文字 | 索引 | 処理内容 |
  |---|---|---|
  | `G` / `Z` / `r` | 0 | 2 バイト ID（word）を読み `FUN_004fa9c0` で解決 → 次文字で分岐。`G` は解決先 `[+8]` ポインタの `[+0x24]` の float、`Z` は `[+8]` ポインタの `[+0xc]` の int を値としてスタックへ。`r` は次文字の分岐にフォールスルー |
  | `n` | 1 | 4 バイト整数を読み FILD で double 化してスタックへ |
  | `z` | 2 | 8 バイト（double ビット列）を読み値そのままスタックへ |
  | その他 | 3 | `X` なら 2 バイト ID で `FUN_004fca30`（拡張関数呼び出し）。`f`/`l`/`s` なら 2 バイト ID で `FUN_004fa110`（float/long/short 変数解決）。それ以外は解決失敗 |
  | `"..."` | - | 文字列リテラル（別経路） |

    下位解決関数の解読結果:

    - **FUN_004fa9c0（ID→エントリ解決）**: `table+0x40` の単方向リストを ID 番目まで走査し該当エントリを返す。`DAT_00b361b0`/`DAT_00b09e1c`/`DAT_00b361b4`（table・ID・引数2）/`DAT_00b361b8`（結果）の 4 グローバルで 3 要素キャッシュを持つ。エントリ `[+0xc]` の関数ポインタが非 null なら `FUN_004fa110` を呼び、得られた double を `+8` にキャッシュして返す。
    - **FUN_004fca30（関数 ID→テーブルオフセット）**: `id-0x100` が 0..0x82 なら `(id*5-0x500)*8+0xb0b420`（40 バイト刻み・0x83 エントリ）、`id-0x1000` が 0..0x170 なら `(id*5-0x5000)*8+0xb0c8c0`（40 バイト刻み）を返し、それ以外は NULL。関数 ID 空間の 0x100 台 / 0x1000 台がそれぞれ別テーブルを指す。
    - **FUN_004fa110（変数テーブル検索）**: `table+0xc` の単方向リストから `node[0]==name_id` を走査し、該当エントリ `[+8]` の double を返す。未登録なら `FUN_004a7a60`（8.8 節のエラー出力関数）でエラーメッセージを出力して 0.0 を返す。キャッシュは `DAT_00b361bc`/`DAT_00b09e20`/`DAT_00b361c0`。
    - **FUN_004fae80（ユーザー関数呼び出しのオペランド評価器・375 命令）**: シグネチャ `(void)`（未分析）のためデコンパイル不可。`0x4fad93` から `FUN_004fab20` の「その他」分岐（idx3）で直接呼ばれ、関数呼び出しの引数列を評価する。3 つのテーブルで駆動される：
      1. `DAT_00b0a54d`（kind テーブル・0 〜 0x21 の 34 種 × 8 バイト）— 先頭バイト 0 なら即値、1 なら参照（`r` 変数 ID → `FUN_004fa9c0` 解決）と判定する分岐の元
      2. `0x4fb40c`（バイトテーブル・オペコード → ジャンプ索引）— 有効なオペコードは `0x00`(string)/`0x01`(double)/`0x02`(float)/`0x05`/`0x0a`/`0x12`/`0x1c`(int)/`0x08`(byte)/`0x21`(word→int)。それ以外は索引 6（失敗）
      3. `0x4fb3f0`（ジャンプテーブル・7 エントリ）— idx0=0x4fb1b8（文字列: 長さ word を読んで `memcpy` 0x9812c0 でコピー＋ NUL 終端）、idx1=0x4fb206（double: `FUN_004fab20` を再帰評価して結果の double を格納）、idx2=0x4fb25f（float: 同系統）、idx3=0x4fb30a（int: word → `MOVSX` で符号拡張）、idx4=0x4fb2b8（byte）、idx5=0x4fb2e0（word → int）、idx6=0x4fb355（失敗 FSTP + 0 返却）
      - ループは引数カウント（先頭で word を読む）まで `[ESP+0x2c]` テーブル（12 バイト刻み）の各オペランドを評価し、`[+0x8]` ビット 3 が立っているオペランドだけ値をスタックへ積む（不要オペランドの評価スキップ最適化）。成功で AL=1。
      - 文字列定数 `"Variable ID %08X not found. Try to recompile script '%s'..."`（0xa49a68）は `FUN_004fa110`（0x4fa19b 参照）専用で、本関数は**文字列参照ゼロ**の純粋な数値ディスパッチ関数。
- 括弧 `(`(0) `)`(1) はテーブルに存在するが opcode 2 未満のため評価器ではリテラル扱い（grouping 自体はトークナイザが吸収）。

### 9.3 エラー処理（FUN_004f3300）

`FUN_004f3300(ctx, code)` が `PTR_s_Expression_Error__*`（RVA 0x709DC0）の文字列配列と書式 `'%s'` でエラーを出力。

| code | メッセージ | 発生箇所 |
|---|---|---|
| 0 | NOERROR | - |
| 1 | OUTOFMEMORY | - |
| 2 | STACKOVERFLOW | - |
| 3 | STACKUNDERFLOW | オペランド不足で演算子実行時（`param_1[0x1c2]==-1`） |
| 4 | DIVIDEBYZERO | div/mod で b==0 |
| 5 | SYNTAX | トークン化失敗 / 括弧不整合 |
| 6 | BADOBJECTPOINTER | シンボル参照の解決失敗 |

### 9.4 M4（menu_xml_interpreter.cpp）との照合結果

| 本家 opcode / 演算 | M4 applyOperator / evaluateNode | 一致 |
|---|---|---|
| 0xb `+` add | `add`（acc+operand） | YES |
| 0xa `-` sub | `sub` | YES |
| 0xc `*` mul | `mul` / `mult` | YES |
| 0xd `/` div | `div`（b==0 → 0.0 を返す） | 一致（本家はエラー4を出す。M4 は安全側で 0 を返す仕様） |
| 0xe `%` mod | `mod`（b==0 → 0.0） | 同左 |
| 8 `==` eq / 9 `!=` neq | `eq` / `neq`（float比較） | 一致（本家は int 同士 int 比較だが値は同一） |
| 4/5/6/7 比較 | `lte/lt/gte/gt` | 一致（名前は M4 の trait 形式） |
| 2 `&&` and / 3 `\|\|` or | `and` / `or`（Bool） | 一致 |
| 0xf 単項マイナス | `neg`（evaluateNode で単項処理） | 一致（\<neg\> 5 \</neg\> → -5） |
| スタック機械 | 再帰下降（evaluateSequence → applyOperator） | 機能的に等価 |
| int/float 混在 | 常に float 演算（toNumber） | 値の等価性は保たれる |

**対応済み（neg）**: 本家 `~` に対応する単項マイナス `neg` を M4 に実装（`<neg> 5 </neg>` → -5、`<neg src="..."/ >` も対応）。ホストテスト 211 件 Pass、Android ネイティブビルド成功。

**参考・FUN_004ff710**: `begin` コマンド（スクリプトブロック型、MISC/CONTROL/Quest 等）を検査するスクリプトコンパイラ側の関数。メニューXML の式評価とは別系統。

## 10. 候補解析の結果

### 10.1 trait文字列の解放関数（FUN_0058a020）

`FUN_0058a020` は trait 文字列の**解放関数**（デストラクタ相当）である（`FUN_004028d0(param_1, 0)` で文字列クリア → `FUN_00401f20` でバッファ解放）。コンパイル関数ではない。当初候補とした `FUN_0058a260` は**存在しない**（NO FUNCTION AT）ため、trait 文字列のコンパイル実体は別の関数と判明。

### 10.2 onlyif / all of について

- `onlyif`（0x7e5）/ `onlyifnot`（0x7e6）はトークンIDとして実在（RE_EXE_REPORT 3.4節）し、`.rdata` に文字列 `onlyifnot`(VA 0x6AA5F8) / `onlyif`(VA 0x6AA604) が存在する。ただしどちらも**コード上の XREF が存在しない**（解析済みプログラムで 0 件）。つまりディスパッチ文字列としては直接参照されず、trait テーブル（0x7e5/0x7e6 の ID）経由で属性として評価される。
- `all of` 文字列は常駐データに見つからない（python バイト検索 0 ヒット）。式評価器 9.2・9.4 の opcode 対応表にも含まれないため、**exe には all of 演算子は実装されていない**可能性が高い（追加 mod 側の仕様）。

### 10.3 子タイル生成（FUN_0058cc60 / FUN_0058cb70）

- **FUN_0058cc60**（子タイル生成・条件付き）: `param_3` が 4000 超かつ（`DAT_00b13bc0` 未満 or 9999 超）の場合、0x18 バイトのノードを生成し `param_1+0x10` の子リスト末尾と、`FUN_0058b220` で取得した属性リスト（`+0x14`）末尾に追加。タイル ID は `FUN_00588bd0` で float 変換。`FUN_0058bee0(0)` で再描画要求。
- **FUN_0058cb70**（子タイル生成・単純版）: `param_1+0x10` の子リスト末尾に 0x18 バイトノードを追加（`piVar4[2]=param_2`、`piVar4[3]=param_3`）、`FUN_0058bee0(0)` で再描画要求。

### 10.4 残課題

- メニュー属性のレンダリング対応（stackingtype / repeatvertical / clips）の Android 移植
- **エンジン側の解析は完了**（FUN_004fae80・関数呼び出し/式評価ディスパッチ系統を含む 8 章・9 章の全機能を解読済み）。残りは Android 側（M4）の実装・移植タスクのみ。
## 11. シェーダーパッケージ（Data\Shaders\shaderpackage001〜019.sdp）

### 11.1 概要

Steam版は `Data\Shaders\shaderpackage001.sdp` 〜 `shaderpackage019.sdp`（計 19 ファイル、約 0.2〜1.0 MB）に、D3D9 形式のコンパイル済みシェーダーを格納している。
各パッケージは同一形式で、ファイル内のシェーダーは **ヘッダ 12 バイト + エントリの連続** で構成される。

- 総エントリ数: 4,094（pkg001 は 74、pkg002 以降は 220〜224）
- ユニークなシェーダー名: **224 種**（vs 102 種 / ps 122 種）
- シェーダー名: `SLS1000.vso` 〜 `SLS2081.pso`（ID 帯は SLS1000xx 系 74 種、SLS2000xx 系 150 種）
- 全エントリのコンパイラ: `Microsoft (R) D3DX9 Shader Compiler 9.08.299.0000`

### 11.2 エントリ形式（逆解析）

| オフセット（エントリ先頭基準） | サイズ | 内容 |
|---|---|---|
| +0x000 | 256 B | シェーダー名（ASCII・NUL 終端・残りはゼロ埋め） |
| +0x100 | 4 B | バイトコード長 `N`（uint32, little-endian） |
| +0x104 | N B | D3D9 バイトコード本体 |

- 次エントリは `先頭 + 260 + N`。12 バイトのファイルヘッダ直後に 1 件目が続く
- バイトコード先頭 DWORD で種別・バージョンが決まる: `0xFFFE0101` = vs_1_1 / `0xFFFE0200` = vs_2_0 / `0xFFFF0201` = ps_2_1（pso は全件 ps_2_1）
- 末尾は `0x0000FFFF`（END トークン）
- バイトコード内の `CTAB`（コンスタントテーブル）から、各シェーダーが参照する定数名を取得可能

バージョン分布: `ps_2_1` 2,230 件 / `vs_2_0` 1,332 件 / `vs_1_1` 532 件

### 11.3 同名シェーダーの差異

- 224 種すべてが複数パッケージに出現するが、**バイトコードが一致するのは 176 種**。残り **48 種**はパッケージごとにバリアントが存在する（品質・機能違いと推測、未確定）
- したがって実際のユニーク・バイトコードは 224 種 + 差分バリアント分

### 11.4 主要な定数（CTAB より）

出現頻度上位の定数名:

| 定数名 | 出現数（全4,094エントリ中） | 推定される役割 |
|---|---|---|
| PSLightColor | 1,657 | ピクセルシェーダーのライト色 |
| NormalMap | 1,633 | 法線マップ |
| ModelViewProj | 1,414 | 頂点変換行列 |
| BaseMap | 1,351 | ベースカラーテクスチャ |
| AmbientColor | 1,290 | 環境光色 |
| LightDirection / LightPosition | 933 / 817 | ライト方向・位置 |
| Toggles | 594 | 機能切替フラグ |
| GlowMap / EmittanceColor | 570 / 381 | グロー・発光 |
| FogParam / FogColor | 527 / 473 | 霧 |
| ShadowMap / ShadowProj* / ShadowMaskMap | 492 / 342 / 360 | シャドウマッピング |
| Bones / SkinModelViewProj | 450 / 450 | スキニング（キャラクター） |
| EnvironmentCubeMap / NormalCubeMap | 92 / 57 | 環境反射・法線キューブ |
| FaceGenMap / FaceGenMap2 | 76 / 76 | 顔の FaceGen テクスチャ |
| DecalMap / DecalFade / DecalProjection | 54 / 36 / 36 | デカール |
| LayerMap / LODLandFlags / HighDetailRange | 19 / 18 / 18 | 地形レイヤー・地形 LOD |
| Time / FillColor / RimColor / SourceTexture | 18 | 水・空・エフェクト系 |

### 11.5 Android 版との照合（未実施・次の作業）

本家 CTAB のレジスタ割当（`tmp\ctab_map.txt` に全集計）と、Android 版の GLSL uniform を対応付けた。

レジスタ種別: `set=2` は float4 定数（c レジスタ）、`set=3` はサンプラー（s レジスタ）。

| 本家定数 | 種別 | 主な配置 | Android 側の対応 | 判定 |
|---|---|---|---|---|
| ModelViewProj | 4x4 | vs c0〜c3 | `uMVP`（renderer.cpp） | 対応 |
| SkinModelViewProj / Bones | 4x4 / 配列54 | vs c1, c42 | `u_boneMatrices[64]`（face_gen_shader.h）、`skinning_shader.h` | 部分対応 |
| LightDirection / LightPosition | float4 | vs c13 / c16 | `uLightDir`（renderer.cpp） | 対応（位置光源は無し） |
| AmbientColor | float4 | pso c1 / vso c6 | `uAmbientColor` | 対応 |
| PSLightColor / LightColor | float4 | pso c2 | `uLightColor`（skinning_shader.h 等） | 部分対応 |
| EyePosition / EyePos | float4 | vso c25, pso c1 | `uCameraPos` | 対応 |
| FogParam / FogColor | float4 | vso c23/c24, pso c9/c10 | `uFogColor` / `uFogRange`（vec2） | 式は一致（線形霧）。FogParam の値は本家 CPU 側で決まり、Android は 0.35×far で代用 |
| Toggles / ToggleNumLights / ToggleADTS | float4 | pso c7 / c6 / c5 | なし（条件分岐は cmp の選択子として使用） | 未対応。Toggles.x・y が切替フラグ、z は pow の指数などデータ用途 |
| ShadowProj / ShadowProjData / ShadowProjTransform | 4x4 系 | vso c28 / c32 / c33 | なし | 未対応 |
| ShadowMap / ShadowMaskMap / AttenuationMap | sampler | pso s2〜s7 | なし | 未対応 |
| NormalMap / BaseMap / GlowMap / DiffuseMap | sampler | pso s0〜s4 | `uBaseTexture` / `uNormalMap` 等（material_system.h） | 対応（Glow は無し） |
| EmittanceColor / MatAlpha / Scroll | float4 | pso c6 / c3 / c0 | `uEmissiveColor` / `uAlpha` | 部分対応 |
| WindMatrices / WindData / AlphaParam | 配列16 / float4 | vso c38 / c4 / c5 | なし（草・木の風揺れ） | 未対応 |
| DeepColor / ShallowColor / FresnelRI | float4 | pso c6 / c5 / c11 | `water_renderer.h`（`uWaterColor` 等） | 部分対応 |
| SunColor / SunDir | float4 | pso c3 / c2 | `uSunDir` / `uSunColor`（sky_weather_system.h） | 対応 |
| FaceGenMap0/1/2 | sampler | pso s2〜s5 | `u_baseTexture` 等（face_gen_shader.h） | 対応 |
| LayerMap / DetailMap / AnisoMap | sampler | pso s1〜s5 | `uDetailTexture`（material_system.h） | 部分対応 |

判定の根拠: 配置は CTAB の register set/index を全エントリで集計した最頻値。対応は変数名・用途からの推定であり、Android 側のシェーダーを本家の命令列とは照合していない。

主な差分:
- 霧の式（PAR2000.vso で確認）: 頂点シェーダーで `d = |view 座標|`、`v = clamp((c23.x - d) / c23.y, 0, 1)`、`oD1.w = 1 - v`、`oD1.rgb = c24`。つまり `c23.x` が終端距離、`c23.y` が霧の範囲幅（終端 - 開始）の線形霧で、Android の `mix(color, uFogColor, clamp((d - start) / (end - start)))` と同形。差分は FogParam の値のみ（本家は CPU 側で設定、Android は start = 0.35×far）。ピクセルシェーダー側の合成は静的確認済み: 霧を掛ける遅延ライティング系ピクセルシェーダー（SM3001.pso 等、約 83 variant）は末尾で `add r0.xyz, -r1, v6` / `mad oC0.xyz, v6.wwww, r0, r1` を実行し、`oC0.rgb = lit + (v6.rgb - lit) * v6.w` となる。v6.w が霧係数、v6.rgb が霧色（頂点側の c24 相当）で、Android の `mix` と同形。どの頂点シェーダーがどの variant に対応するかは未確認
- シャドウ（ShadowProj*/ShadowMap 系）は Android 版に実装なし
- 草・木の風揺れ（WindMatrices/WindData）は Android 版に実装なし
- 位置光源（LightPosition）は Android 版で単一方向光に置き換えられている

霧式は確定済み（上記）。

**CPU 側の定数値の供給元（Ghidra 逆コンパイルで確認）**
- FogParam / FogColor の格納先は `DAT_00b44f78` / `DAT_00b44f68`。書き込みは `FUN_007d1c90` で行われ、霧オブジェクトから値を取得する
  - c 値 x（終端）= オブジェクト +0x30、y（幅）= +0x30 − +0x2C、霧色 = +0x20〜+0x28
  - 頂点シェーダーの `c23.x` / `c23.y` の並びと一致する
- 別パスの FogParam 格納先 `DAT_00b45e74` は `FUN_007dcbd0` で書き込まれ、同じ構造（+0x30 / +0x2C / +0x20〜+0x28）を使う
- Toggles 格納先 `DAT_00b46688` は `FUN_0080d110` でパス単位のフィールド（オブジェクト +0x56〜+0x59）から書き込まれる。`FUN_007f6ae0` では別のフラグ（+0x24）に応じて 1.0 / 0.0 を書く
- 未確認: D3D9 の SetVertexShaderConstantF / SetPixelShaderConstantF 呼び出しの特定（D3D9 は LoadLibrary で動的ロードされ、固定 vtable オフセットの仮定は破棄した）。Toggles の x / y が各機能のどれに対応するかの特定。

**Toggles（pso c7）の分岐解析**
- c7 を参照する pso は 138 本（c7 を def で再定義するものを除く）
- 切替に使われるのは `x` と `y` の 2 成分のみ。`cmp rD, -c7.x, A, B` の形が 74 箇所、`-c7.y` が 86 箇所
- `cmp` は `src0 >= 0 ? src1 : src2` のため、`c7.x == 0` で A、`c7.x == 1` で B を選ぶ 2 状態の切替と解釈できる
- 切替の組み合わせは `xy` の両方を使う pso が 70 本、`x` のみが 4 本、`y` のみが 16 本
- `z` は `mad` の係数や `pow` の指数として使われ、切替ではない
- 成分ごとの機能名（どのフラグがどの効果か）はまだ特定できていない。CPU 側の値の設定箇所の解析が必要

**Toggles の CPU 側書き込み（解析確定・FUN_0080d110 デコンパイル）**
- `FUN_0080d110`（__thiscall, 1878B, 関数ポインタテーブル 0x00a942d0 から参照）は **描画パスディスパッチャ**。パスタイプ `DAT_00b42e90` で分岐する
  - `DAT_00b42e90` が 0xe0 / 0xe1 / 0x18c / 0x18d / 0x18e / 0x18f(399) の場合に描画パス処理を実行（それ以外のタスクIDでは別処理へ分岐）
  - case 0xe0 / 0xe1: 定数セッターを直接呼ぶ（`FUN_0076c910`・`FUN_0076c730`・`FUN_007aec20`・`FUN_007aec60`・`FUN_007aecb0` がシェーダー定数 c0〜c17 などの書き込み）
  - case 0x18c / 0x18d / 0x18e / 399: `FUN_00851ca0`・`FUN_00851e70`・`FUN_00848950`・`FUN_00848ad0` に委譲（引数 param_2/param_3/param_5/piVar2 を渡す）
- Toggles 書き込み（確定）: `piVar2 = param_5 + 0x18` のパスオブジェクトから、`piVar2[0x56]`〜`piVar2[0x59]`（= +0x158〜+0x164）を 4 成分 x/y/z/w としてコピーする
  - **分岐条件**: `DAT_00b42f32==0` または `DAT_00b42f48==1` → `_DAT_00b46688`〜`_DAT_00b46694` 系に書く（この場合 iVar8=1 で処理）
  - それ以外 → `_DAT_00b4616c`〜`_DAT_00b46178` 系に書く
- `piVar2[7]` のビット **0x400 (bVar9) / 0x800 (bVar10) / 0x1000 (bVar11)** が定数セットの選択子として読まれる（case 0xe1 で bVar9/bVar10/bVar11 に応じて描画パスオブジェクト +0x44〜 のフィールドを使い分け）
- `FUN_007f6ae0` は `DAT_00b46688` に対し、別オブジェクトの +0x24 フラグが非 0 なら 1.0、0 なら 0.0 を書く（x のみ）
- 未確認: `piVar2[0x56]` 等の上流の書き込み元（パスオブジェクトを構築する関数）。ここが特定できれば x / y と機能の対応が決まる
### 11.6 結論

- シェーダーは **固定機能ではなく SM1.1〜2.1 のプログラマブル**シェーダーで構成され、ライティング・法線マップ・シャドウ・霧・スキニング・顔・地形レイヤーを個別シェーダーで実現している
- Android 版への反映優先度が高い項目は **FogParam（霧）・ShadowProj 系（シャドウ）・Toggles（機能切替）・Time（水・空）**

## 12. Oblivion_default.ini の解析

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion_default.ini`（12,766 バイト、既定値。ユーザー設定の `Oblivion.ini` は別途上書きする）

### 12.1 Android 移植で影響が大きい項目

| セクション | キー | 値 | 意味・移植への影響 |
|---|---|---|---|
| Display | fNearDistance / fFarDistance | 10 / 10000 | 描画範囲。霧の終端と近接で使われる（Android の far 既定値と照合が必要） |
| Display | fDefaultFOV | 75 | 既定の視野角 |
| Display | iSize W/H | 640 x 480 | ウィンドウ既定。Android は端末解像度依存 |
| Display | bDrawShadows | 0 | 既定で影描画オフ |
| Display | bUse Shaders / bForceMultiPass | 1 / 1 | シェーダー既定 |
| Display | iPresentInterval | 1 | 垂直同期 |
| Display | fShadowLOD1/2, fLightLOD1/2, fSpecularLOD1/2, fEnvMapLOD1/2 | 200/400, 1000/1500, 500/800, 500/800 | 距離ベースの LOD 切替。Android の LOD 距離の根拠 |
| General | uExterior Cell Buffer / uInterior Cell Buffer | 36 / 3 | セルのメモリ保持数 |
| General | uGridsToLoad | 5 | ロードするグリッド数（5x5） |
| General | uGridDistantCount / uGridDistantTreeRange | 25 / 15 | 遠景グリッド数と遠景木の範囲 |
| Grass | fGrassEndDistance / fGrassStartFadeDistance | 3000 / 2000 | 草の描画終端とフェード開始 |
| Grass | iMinGrassSize | 80 | 草の最小サイズ |
| Weather | bPrecipitation | 1 | 降雨既定オン |
| Audio | fDefaultMasterVolume ほか | 1.0 / 0.3(音楽) / 0.8(効果) | 音量既定値 |
| GamePlay | iDifficultyLevel | 50 | 難易度既定 |
| GamePlay | bSaveOnRest / bSaveOnTravel / bSaveOnWait | 1 | 自動セーブの既定 |
| Controls | fMouseSensitivity | 0.002 | マウス感度 |
| Controls | iLanguage | 0 | 0=英語、1=ドイツ語、2=フランス語、3=スペイン語、4=イタリア語 |
| Archive | 各 BSA 名 | Misc, Voices1/2, Sounds, Textures - Compressed, Meshes | Android 側で読み込む BSA の一覧（Textures は Compressed の 1 本のみ） |
| Archive | bInvalidateOlderFiles / bUseArchives | 1 / 1 | アーカイブ使用既定 |

### 12.2 所見

- 霧の終端は `fFarDistance`（10000）が基準。Android の霧開始は 0.35 x far と推定しており（第 11 章）、この値と整合する。
- `[Archive]` の BSA 名は、Android の BSA 読み込み対象の正典として使える。
- `Oblivion_default.ini` にはテクスチャ BSA が `Textures - Compressed` の 1 本しかなく、`Textures1` 等は含まれない。DLC 系 BSA は別管理（`analyze-dlc-bsa`）。
- 数値の多くは開発元の既定値であり、Android 側の既定値を変える際の基準になる。
- キー名に空白を含むもの（`bUse Shaders`、`iSize W`、`bCreate Maps Enable`）は読み込み側で空白を保持して照合する必要がある。
- `[Pathfinding]`、`[HAVOK]`、`[Debug]`、`[ShockBolt]`、`[Absorb]` などはデバッグや演出用で、移植の優先度は低い。

### 12.3 未確認

- 各キーを実際に読む関数（INI リーダー）の特定。Ghidra の文字列参照で追跡可能だが未実施。
- Android 側で各値が反映されているかの照合。

## 13. ボイス音声 BSA の解析

対象: `Oblivion - Voices1.bsa` / `Oblivion - Voices2.bsa`（Steam 版 `Data\`）。集計は `tmp\analyze_voices_steam.py`、出力は `tmp\voices_summary.txt`。

| アーカイブ | ファイル数 | 展開後サイズ | 内訳 |
|---|---|---|---|
| Voices1.bsa | 33,198 | 749.2 MiB | mp3 16,603 / lip 16,595 |
| Voices2.bsa | 44,582 | 956.2 MiB | mp3 22,295 / lip 22,287 |
| 合計 | 77,780 | 1,705.3 MiB | mp3 38,898 / lip 38,882 |

- 格納パスは `sound\voice\oblivion.esm\{種族}\{m|f}\{prefix}_{formID:08x}_{n}.{mp3|lip}`。
- 種族は high elf, breton, argonian, imperial, nord, redguard など。Voices1 は主に high elf・breton・argonian、Voices2 は imperial・nord・redguard。
- 圧縮フラグは全件 0（非圧縮）。
- BSA v103 で、ディレクトリ名・ファイル名を保持する。

### 13.1 Android 側の現状

- `dialogue/voice_sound_db.cpp` は `{formID:08x}_{n}` で mp3 を索引する。lip は対象外（`.mp3` 検索のみ）。
- `engine/renderer.cpp` の 2595〜2602 行で Voices1/2 と DLC Shivering Isles の BSA を読み込み、3481 行で索引を構築する。APK には含めない。
- lip（口形同期データ）は未使用。

### 13.2 音声再生システム（EXE 内部・直接調査 2026-10）

EXE（Steam 版）のオーディオは **3 系統** で構成される（import 一覧から確定）。

| 系統 | API | 用途 |
|---|---|---|
| DirectSound8 | `DirectSoundCreate8`（Ordinal_11） | 効果音・3D 音（Sounds.bsa の wav） |
| Bink（RAD ゲームツール） | `_BinkOpenDirectSound@4`, `_BinkOpen@8`, `_BinkDoFrame@4` 等 | 動画（.bik）の音声 |
| winmm | `mmioOpenA`, `mmioDescend`, `mmioRead` 等 | WAV リソース読み出し |

BGM は `Data\Music\Special\*.mp3`（`success.mp3`, `death.mp3`, `TES4Title.mp3`）、ボイスは BSA 内 `sound\voice\...mp3`（§13 参照）。`%s*.mp3` / `%s.lip` によりファイル名組み立て。

**DirectSound 初期化チェーン（確定）:**
- `FUN_00404a00`（アプリ初期化）が `param_1+0x24` にオーディオシステムを構築。
- `FUN_006abc80`（__thiscall）が DirectSound システム初期化:
  - `DirectSoundCreate8(0, &dev, 0)` でデバイス生成。
  - vtable +0x18: 協調レベル設定に相当（`param_2, 2`）、+0x10/+0x0c: バッファ生成。
  - `param_1[0x1e]` = primary buffer、`*param_1[3]` = 2D buffer caps 0x91。
  - マスター音量 `_DAT_00b161d8`（0x3f800000=1.0 / 0x41200000=10.0 の切替）。
  - `param_1[0xc0]`, `param_1[0xc1]` = `FUN_006abb00`/`FUN_006abb50`(0x25) 生成の **37 エントリ配列**（サウンドハンドル格納用、vtable PTR_FUN_00a7714c）。
- システム本体ポインタはグローバル `_DAT_00b3c214` に保持。`CoInitialize` で COM 初期化も行う。

Android 側は OpenAL（stub ヘッダ、デバイス無しなら Java MediaPlayer / SoundPool ブリッジへ）で再生しており、EXE の DirectSound8 再生パスに対応する。ハンドル配列（0xc0/0xc1）は Android 側のサウンドハンドル管理（SoundPool/ソースID）と類比。

### 13.3 lip（口形同期）フォーマット — 完全解読済み（2026-10）

`.lip` バイナリフォーマットは **EXE パーサ（FUN_00493f60 → 00493ed0 → 00493bd0 → 00493ad0）の静的解析と、
抽出済み 9,204 実ファイルの回帰検証で完全解読**した。詳細は `tmp/re_lip_format.md`。

- 先頭12B ヘッダ: `u32[0]=1`, `u32[1]=24+count*132`, `u32[2]=1`。
- 本体（ファイルサイズ−12）はトークンストリーム: 非0バイト→リテラル1B / 0x00→u16長ゼロラン3B。
- 展開後: `dword[0]=count`（キーフレーム数, 13〜548）, dword[1]=補助値, 以降 `count × 132B`。
- 各キーフレーム = **16 u32（配列A, 視素ウェイト）+ 17 u32（配列B, 補助）** = 0x40 + 0x44。
- 検証: 9,196/9,204（99.78%）が完全デコード成功。標準9,189/9,196 が `h1==24+count*132` 厳密一致。
- 例外 8 件（0.87%）: 空ファイル1 + ヘッダ先頭 0xFF の小型変種7。標準実装のみ対応で十分。
- 旧「20Bヘッダ+7B固定レコード」仮説は誤りで破棄。
- Android リップシンク実装時は 33 float/キーフレームをタイムラインへマッピングすればよい（要否は別途判断）。

### 13.3 ボイス mp3 と INFO の全数照合（完了 2026-10）

`tmp/re_voice_naming.md` に詳細。メイン本編 mp3 計 **38,898**（形式装着 38,772 ＋ 汎用グリーティング 126）。

- 形式 `{prefix}_{formID:08x}_{n}.mp3` の 38,772 件中 **38,500 件（99.3%）** の formid が INFO レコードに存在（INFO 応答ボイス確定）。
- 残り 272 行 / 140 distinct formid は INFO 非該当（REFR=145、マスター ESM 未検出=127）。クエスト外／アクター付きグリーティングと判断。
- **INFO には EDID サブレコードが無い**。接頭辞 `{prefix}` は DIAL／クエスト文脈の別名（例 `ms14_ms14returnhome`）であり、INFO の EDID 照合は原理的に不能。
- **応答番号 n はトピック（DIAL）累積番号**。例: INFO `0000566f` は TRDT=8 だが mp3 `_9` まで存在。単純 `n<=TRDT` チェックは 481 件を誤検出。
- Android の `{formID:08x}_{n}` 索引は {prefix} を検査しないため、本結論の影響を受けない（索引方式は正しい）。

### 13.3b 次の候補

- DLC Shivering Isles - Voices.bsa（452 MiB）の索引件数確認。
- 非 INFO（REFR 等）272 行と汎用グリーティング 126 件の再生経路調査。
- DIAL 側の `{prefix}` 生成則特定（Android 照合には不要、命名逆引き目的のみ）。
## 14. ESM / DLC BSA / ムービーの解析

### 14.1 Oblivion.esm（264.6 MiB）のレコード集計

`tmp\esm_survey2.py` で全グループを走査（TES4 の後、20 バイトのレコードヘッダ、20 バイトのグループヘッダ）。出力は `tmp\esm_survey.txt`。

| 種別 | レコード数 | 備考 |
|---|---|---|
| REFR | 1,025,617 | 配置参照 |
| CELL | 35,494 | 内部 1,855 / 外部 33,639 |
| LAND | 31,823 | 地形 |
| INFO | 19,278 | 会話応答。NAM1 テキスト付き 23,877 件（重複カウントあり） |
| DIAL | 3,817 | 会話トピック |
| NPC_ | 2,482 | 女性 778 / 必須 249、種族 15 種 |
| QUST | 390 | クエスト（ステージ INDX 2,516） |
| SPEL | 1,137 | SPIT.type: 0=602, 1=32, 2=74, 3=137, 4=292 |
| ENCH | 1,542 | 付与効果 |
| MGEF | 145 | 魔法効果 |
| WRLD | 84 | ワールドスペース |

注: 全レコード合計は 1,167,016 件、種別は 63 種。

### 14.2 DLC と Knights の BSA（v103）

`tmp\dlc_video_survey.py` の出力。`tmp\dlc_video_survey.txt` を参照。

| アーカイブ | ファイル数 | 展開後 | 主な内容 |
|---|---|---|---|
| DLCShiveringIsles - Voices.bsa | 19,089 | 430.9 MiB | mp3 9,548 / lip 9,541（非圧縮） |
| DLCShiveringIsles - Textures.bsa | 1,869 | 298.2 MiB | dds（圧縮） |
| DLCShiveringIsles - Meshes.bsa | 3,017 | 116.6 MiB | nif / lod / kf（圧縮） |
| DLCShiveringIsles - Sounds.bsa | 499 | 87.4 MiB | wav（非圧縮） |
| Knights.bsa | 4,810 | 162.8 MiB | mp3 2,279 / lip 2,275 / dds 142 |
| DLCBattlehornCastle.bsa | 388 | 8.7 MiB | mp3 164 / lip 164 |
| DLCFrostcrag.bsa | 49 | 16.4 MiB | dds / nif / wav |
| DLCOrrery.bsa | 90 | 15.8 MiB | dds 69 |
| DLCVileLair.bsa | 57 | 4.0 MiB | dds 31 / mp3 9 |
| DLCThievesDen.bsa | 65 | 2.0 MiB | mp3 26 / dds 8 |
| DLCHorseArmor.bsa | 46 | 3.8 MiB | mp3 16 |

Android 統合: ボイス索引は mp3 のみを対象にしているため、DLC の mp3 は `DLCShiveringIsles - Voices.bsa` と Knights.bsa を含めて対象かを確認する必要がある（未検証）。

### 14.3 ムービー（Data\Video\*.bik）

Bink 形式（ヘッダ "BIKi"）。フィールド順は size, frames, largest, frames, width, height, fps_num, fps_den。

| ファイル | 解像度 | フレーム数 | 再生時間 | サイズ |
|---|---|---|---|---|
| OblivionIntro.bik | 1280x720 | 3,461 | 115.5 s | 129.9 MiB |
| OblivionOutro.bik | 1280x720 | 1,739 | 58.0 s | 67.3 MiB |
| CreditsMenu.bik | 1280x720 | 1,712 | 57.1 s | 21.8 MiB |
| Map loop.bik | 1280x720 | 750 | 25.0 s | 14.3 MiB |
| Oblivion iv logo.bik | 1280x720 | 418 | 13.9 s | 16.0 MiB |
| bethesda softworks HD720p.bik | 1280x720 | 300 | 10.0 s | 0.7 MiB |
| game studios.bik | 1280x720 | 299 | 10.0 s | 5.7 MiB |
| 2k games.bik | 1280x720 | 180 | 6.0 s | 0.7 MiB |
| Oblivion Legal.bik | 1280x720 | 122 | 4.1 s | 0.8 MiB |

全て 29.97 fps。Android 側には `app/src/main/cpp/video/`（bink_video_player、video_decoder_jni、video_clip_manager）と `IntroVideoActivity.kt` があり、Bink 再生経路は実装済み。各動画の実機再生確認は未実施。

### 14.4 未検証事項

- ESM のレコード数は集計値。Android の ESM リーダーが全レコードを読めているかは未照合。
- NPC・QUST・SPEL の詳細フィールド（クエスト段階の内容、魔法効果の値）は今回は集計のみ。
- DLC の esp は `tmp\re_dlc_bsa.md` で調査済み（2026-10-11）。`DLCShiveringIsles.esp` は TES4 スタブ（85 B, レコード 0）で、SI 本体は `Oblivion.esm` に内包。実体のある 9 種（BattlehornCastle / Frostcrag / HorseArmor / MehrunesRazor / Orrery / SpellTomes / ThievesDen / VileLair / Knights）のレコード数を確定（合計 24,134 件、Knights.esp が最多 9,563）。
## 15. エフェクト/パーティクルとクレジットの解析

集計スクリプトは `tmp\effects_credits_survey.py`、出力は `tmp\effects_credits_survey.txt`。

### 15.1 EFSH（エフェクトシェーダー）レコード

Oblivion.esm の EFSH は 102 件。サブレコードは EDID 102、DATA 102、ICON 100、ICO2 100。

- DATA サイズ: 224 バイト 100 件、96 バイト 2 件。
- ICON（一次テクスチャ）: 14 種。最多は `Effects\ghostEffectMap.dds`（11）、次に `Fire\Lava\caustics_001.dds`（7）。ICON が空の 69 件は空文字列として集計（空の意味は未確認）。
- ICO2（二次テクスチャ）: 19 種。最多は `Magic\frostVapors.dds`（17）、`Magic\frostCloud.dds`（11）、`Fire\FireFlameParticle.dds`（7）。

DATA の 224 バイト各フィールド（フラグ、ブレンド、寿命、色）は未デコード。

### 15.2 Android 側との対応

- `app\src\main\cpp\engine\particle_system.h` は EmitterShape 7 種（POINT、SPHERE、BOX、CONE、CYLINDER、MESH、LANDSCAPE）と ParticleBlend 4 種（ALPHA、ADDITIVE、MULTIPLY、PREMULTIPLIED）を持つ。
- EFSH のテクスチャ 33 種（ICON + ICO2 の一意名）は、Android 側の対応テクスチャ有無を未照合。
- 次段階: DATA 224 バイトのデコードと、ICON/ICO2 のテクスチャ抽出。

### 15.3 クレジット（Data\Credits.txt）

- 501 行、cp1252 エンコーディング（UTF-8 では読めない）。
- コマンド: `<C:>` 144、`<X:>` 4、`<Y:>` 2、`<I:>` 4、`<J:>` 2、`<F:>` 2。
- 色は 2 種類（役職色 206,158,102,255 と本文色 235,213,174,255）。役職見出し 72 件、本文の名前・社名行 314 行。
- 使用フォント: 2。
- 画像 3 枚: tes_oblivion_logo.dds（512x128）、zenimax_media_logo.dds（512x128）、obliv_credscreen_logos-black.dds（512x64）。`menus\credits\` 配下に抽出済み（`tmp\extracted_menus_full\menus\`）。
- Android 側は `title_screen.cpp` に CREDITS 状態と自動スクロールがある。しかし `app\src\main` 内に Credits.txt 由来の本文（"Todd Howard" 等）は存在せず、表示テキストの供給元は未実装。
- Android 側の `assets\textures\ui\` には tes_oblivion_logo 系 2 枚のみ。zenimax_media_logo と obliv_credscreen_logos は未統合。

### 15.4 未検証事項

- Bethesda.TXT はインストールルート直下にあり（Data 内ではない）、ゲームデータではなくサポート連絡先（395 バイト）のテキストである。ゲーム機能には使われない。
- Credits.txt の `<X:>`/`<Y:>` 座標系と Android のスクロール座標の対応は未確認。
- EFSH DATA のフラグ値の意味（Flags バイト）は未デコード。ブレンド値は §15.5 で解読済み（描画上の意味は `tmp/re_efsh_blend.md`）。
### 15.5 EFSH DATA 224 バイトのデコード（xEdit 定義で検証済み）

- 出典: xEdit（TES5Edit）の `wbDefinitionsTES4.pas`（`matortheeternal/merge-plugins` ミラー、commit 309a23ce）。`tmp\wbDefinitionsTES4.pas` に保存。UESP は取得できなかった。
- 検証: `tmp\efsh_layout_check.py`。Oblivion.esm の 224 バイト EFSH DATA 100 件をフィールド順にデコードした。レイアウト合計は 224 バイトで一致。ブレンド列挙値（1〜11）、ブレンド演算（1〜5）は全件で有効範囲内。浮動小数は全件で有限かつ |x| < 1e4。違反 0 件。Membrane ZTest は全件 3。
- 旧 15.5 の「観察」（4 バイト単位の統計）は、この xEdit 順序で読み直すと対応づけできる。

| オフセット | フィールド（xEdit 名） | 型 |
|---|---|---|
| +0 | Flags（+1 バイトの Unused 3 を含む） | u8 + pad |
| +4, +8, +12 | Membrane Src Blend, Blend Op, ZTest | u32 ×3 |
| +16 | Fill Color（RGB + pad） | 4 バイト |
| +20 〜 +48 | Fill Fade In, Full, Fade Out, Persistent, Pulse Amp, Pulse Freq, Anim U, Anim V | f32 ×8 |
| +52 | Edge Fall Off | f32 |
| +56 | Edge Color（RGB + pad） | 4 バイト |
| +60 〜 +80 | Edge Fade In, Full, Fade Out, Persistent, Pulse Amp, Pulse Freq | f32 ×6 |
| +84, +88 | Fill Full Alpha Ratio, Edge Full Alpha Ratio | f32 ×2 |
| +92 | Membrane Dst Blend | u32 |
| +96 〜 +108 | Particle Src Blend, Blend Op, ZTest, Dst Blend | u32 ×4 |
| +112 〜 | Birth Ramp Up, Full, Ramp Down, Full Ratio, Persistent Ratio, Lifetime, Lifetime ±, Init Speed, Accel, Vel1-3, Acc1-3, Scale Key1/2, Scale Key1/2 Time | f32 ×19 |
| +188 〜 | Color Key1/2/3（RGB + pad） | 4 バイト ×3 |
| +200 〜 | Color Key1/2/3 Alpha | f32 ×3 |
| +212 〜 | Color Key1/2/3 Time | f32 ×3 |

- 注記: 上表の位置は xEdit の宣言順からの累積計算値（合計 224 バイト）。色は RGB 3 バイト + pad 1 バイトとして定義されている。
- Blend 列挙値の描画上の意味は解明済み（`tmp/re_efsh_blend.md`）。ブレンド列挙（1〜11）は OpenGL ブレンドファクタへ直接対応する（1=GL_ZERO, 2=GL_ONE, 3=GL_SRC_COLOR, 4=GL_ONE_MINUS_SRC_COLOR, 5=GL_SRC_ALPHA, 6=GL_ONE_MINUS_SRC_ALPHA, 7=GL_DST_ALPHA, 8=GL_ONE_MINUS_DST_ALPHA, 9=GL_DST_COLOR, 10=GL_ONE_MINUS_DST_COLOR, 11=GL_SRC_ALPHA_SATURATE）。
- 実データ 100 件の分布: SrcBlend はほぼ全件 SourceAlpha（GL_SRC_ALPHA 固定）。DstBlend は SourceInvAlpha（標準アルファ）と One（加算）の 2 値に集約。BlendOp はほぼ全件 Add。つまり EFSH の実際の描画は「通常アルファ」と「加算ブレンド」に大別される。
- Android 側: `ParticleBlend` enum（ALPHA/ADDITIVE/MULTIPLY/PREMULTIPLIED）は定義済みだが、`blendMode` は CPP で参照されておらず、描画は全ハードコードの標準アルファ。EFSH の加算ブレンド（Dst=One）は未反映。
- 未確認: 色のバイト順の実機での見た目確認。
- 96 バイト 2 件は別レイアウトの可能性があり、今回は未分析。
## 16. ESM world icon extraction (ICON = Clutter\...)

- Source: ICON subrecords in Oblivion.esm, 366 unique `Clutter\` paths (of 1419 total ICON paths).
- Lookup: `textures\menus\icons\clutter\<name>` in base BSAs, falling back to `textures\menus80\icons\clutter\<name>` (used by DLCShiveringIsles - Textures.bsa).
- Result: 366 / 366 located. 306 from Oblivion - Textures - Compressed.bsa, 60 from DLCShiveringIsles - Textures.bsa. 0 decode failures.
- Output: `tmp\world_icons\` (366 PNG, RGBA, from DDS via dds_decode).
- Script: `tmp\extract_world_icons_v2.py`.
- Note: the 60 DLC-only icons are present only when the Shivering Isles DLC is installed.

> Addendum (2026-10-08): Headless Ghidra (project program Oblivion.exe) found zero direct references to FUN_0080d110. Its caller is likely an indirect call (vtable or function pointer), so the upstream writer of piVar2[0x56..0x59] is still unidentified.

## 17. NPC AI（プロセス／パッケージ）— 2026-10-10 追記

解析対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE image base 0x400000、.text 0x401000-0xa27dff、.rdata 0xa28000-0xb019ff、.data 0xb02000-0xbabc1b）。

### 17.0 AI プロセス本体と RE カバレッジ（確定・2026-10-10）

- **RE カバレッジ数値**: Ghidra の本 EXE の関数総数は **22,295**（実測）。本レポートで RE 識別・命名済みの関数はその一部（描画 / シェーダ / AI / オーディオ / セーブ等）。AI 系は §17〜§18 で対応を体系化。
- **AI プロセス vtable: `0xa72d64`**。AI プロセス本体の更新入口は `UpdateAiProcess`（`0x6588c0`）。
- **AI 更新ディスパッチ: `AiUpdateDispatch`（`0x649340`）**。選択専用であり、種別実行本体は含まない（§17.8）。
- **手続き種別名表: `0xB12988`（33 エントリ）**。種別バイト `package+0x20` をインデックスとする（§17.1）。
- **`FUN_005ebb30`（アクター単位 AI プロセス更新、1601 バイト）**: レベル別リストを走査し、AI レベルに応じてプロセスを差し替える。種別 5（Wander）のパッケージに対し `process+4` をクリアするフローを確認（§17.5 参照）。

### 17.1 AI手続き種別テーブル（0xB12988、33エントリ）

パッケージの種別バイト（`package+0x20`）を添字にした文字列テーブル。`tmp\ai_procedure_types.txt`。

| idx | 名称 | idx | 名称 | idx | 名称 |
|---|---|---|---|---|---|
| 0 | Find | 11 | CastMagic | 22 | MountHorse |
| 1 | Follow | 12 | Combat | 23 | DismountHorse |
| 2 | Escort | 13 | CombatLow | 24 | DoNothing |
| 3 | Eat | 14 | Activate | 25 | CastTargetSpell |
| 4 | Sleep | 15 | Alarm | 26 | CastTouchSpell |
| 5 | Wander | 16 | Flee | 27 | VampireFeed |
| 6 | Travel | 17 | Trespass | 28 | Surface |
| 7 | Accompany | 18 | Dialogue | 29 | SearchForAttacker |
| 8 | UseItemAt | 19 | Spectator | 30 | ClearMountPosition |
| 9 | Ambush | 20 | ReactToDead | 31 | SummonCreatureDefend |
| 10 | FleeNotCombat | 21 | GetUp | 32 | MovementBlocked |

### 17.2 データレイアウト

- `Actor + 0x58` が AI プロセス。プロセス `+8` が現在のパッケージ。パッケージ `+0x20` が種別バイト、`+0x1c` がフラグ（bit 0x14 / 0x15 / 0x17）、`+0x24` がポインタ。
- プロセス vtable の主要スロット: `+8` レベル取得（0=高 / 1=中 / 2=低）、`+0x18` update(actor, flag)、`+0x174` 現在パッケージ取得、`+0x178` 設定、`+0x184` パッケージ取得、`+0x194` 状態更新、`+0x36c` 種別取得、`+0x394` / `+0xbc` / `+0x38c` 状態リセット、`+0x21c` リセット、`+0x594` パッケージ開始。
- アクター vtable: `+0x170` 基本NPC取得、`+0x380` 乗騎/対象、`+0x44` 種別判定。
- グローバル: `DAT_00b333c4` = プレイヤーアクター、`DAT_00b33a98 + 0xcd4` = フラグ。

### 17.3 パッケージ選択チェーン（検証済み）

1. `FUN_00569020(actor, bool)` — アクターのパッケージリストから最良のパッケージを選択する。判定は `FUN_0056a950` / `FUN_00568eb0`。デバッグ文字列 `"%s Picked Package %s (%s)"`（`0x5691xx`）を出力。
2. `FUN_00648e40(process, actor)` — パッケージリストの再取得。`FUN_005e0330` → `FUN_00468670` → `FUN_005e02e0` 経由でレベル別リスト（`+0x68` / `+0` / `+0xc` / `+0x18`）をたどる。
3. `FUN_00649340`（別名 `AiFindPackageMid`）— 現在パッケージの切替。旧パッケージの終了処理（`FUN_005eae70`）と vtable `+0x594` / `+0x21c` / `+0x38c` / `+0xbc` / `+0x394` のリセットを行う。`process+0xf` / `+0x10` のリストを消化する。
4. `FUN_00629b70` — アクター更新ラッパー。`vtbl+0x36c` が 0 / 4 / 9 の場合のみ本体を実行するゲート構造。
5. `FUN_006588c0` — `FUN_00649340` を `+0x90` のキャッシュ付きでラップ。

### 17.4 レベル別リスト管理

- `FUN_00673a90` / `FUN_00674550`（thiscall、管理オブジェクト）— レベル 0 は `+0x68`、1 は `+0`、2 は `+0xc`、3 は `+0x18` のリストにアクターを振り分ける。レベルは `FUN_00659a00`（プロセス vtable `+8` を呼ぶ）で取得。
- `FUN_0067b260` = リスト追加、`FUN_0065c620` = リストからの削除（単純な連結リスト操作）。

### 17.5 種別専用ハンドラ

| 関数 | 意味 |
|---|---|
| `AiPackage_IsDialogue`(`FUN_005e6b40`) | IsDialogue（種別 0x12） |
| `FUN_005e6b70` | IsSpectator（種別 0x13） |
| `FUN_005e6ba0` | IsAlarm（種別 0xf） |
| `AiPackage_IsSurface`(`FUN_005e6fe0`) | IsSurface（種別 0x1c） |
| `AiPackage_IsEatType5`(`FUN_005e6fa0`) | IsEat 系（種別 0x3 + 型条件表 `PTR_DAT_00b152b0[pkg+0x18]==5`） |
| `FUN_005e6cd0(actor, flag)` | IsFleeing（0x10 / 0xa / 条件付き 0xd）、呼出元 16 |

- **種別述語クラスタ（確定・2026-10-11）**: `actor+0x58` → パッケージ vtable `+0x184`/`+0x180`（アクセス演算子）→ `pkg->type`（`[[pkg]+0x20]`）を検査する短小関数群（35〜150 バイト）が 0x5e6000〜0x5e7000 に密集する。これらは「単一 executor の case」ではなく **選択系の述語ハンドラ** で、§17.8 の実行モデル (a) に該当。`AiPackage_IsEatType5` は型バイト 0x3 に加え `PTR_DAT_00b152b0[pkg+0x18]==5` を要求する複合条件。
| `FUN_005eb400` | Travel（種別 6）位置ヘルパー |
| `FUN_005ebb30` | アクター単位の AI プロセス更新（1601 バイト）。レベルに応じてプロセスを差し替え、種別 5（Wander）で `process+4` をクリア |
| `FUN_00601b80` | パッケージ変更/停止。種別 0x12 / 0x17 / 0x1e をスキップ、種別 4（Sleep）と `pkg+0x1c` フラグを処理 |
| `FUN_005eae70` | パッケージ終了処理。種別 0x16（MountHorse）/ 0x11（Trespass）に専用分岐 |
| `FUN_005e9e70` | アクター状態リセット |
| `FUN_005e4b00` / `FUN_005e4fc0` | アクター保有リスト（`+0x58` など）の走査 |

### 17.6 種別バイト比較サイト

`tmp\ai_switch.txt`（約 133 行）。`CMP byte ptr [reg+0x20], imm` と 0x20 エントリの switch ジャンプテーブルを走査した結果。主なサイト: Combat 0xc @ 0x5e619e / 0x61d469 / 0x624e0b、Dialogue 0x12 @ 0x5e6b57 / 0x601815、Flee 0x10 @ 0x5e6cea / 0x5eff4e、Alarm 0xf @ 0x5e6baa、Sleep 4 @ 0x601d94、Wander 5 @ 0x5ebc9b、Travel 6 @ 0x5eb44d。

### 17.7 種別名テーブルの参照元（表示コード）

`0x63b6f7` / `0x649f1c` / `0x6581f7` などは種別名を sprintf で整形するデバッグ/UI コードであり、実行処理ではない。関数 `FUN_0063a210` / `FUN_006497f0` / `FUN_00659010` / `FUN_00657cc0` を定義済み。

### 17.8 パッケージ実行モデル（検証済み・2026-10-11）

`AiUpdateDispatch`（`FUN_00649340`、改名済み・確定）は **選択専用** であり、種別ごとの実行本体は含まない。実行はパッケージ・サブクラスの vtable 上書きと、種別ファクトリ関数群が担う。

- **パッケージ基底 vtable: `0xa6743c`**。基底 ctor `FUN_00568e10` が vtable を設置し `+1` に `0x3d` を書く。主要スロット: `+0x00`=0x60d0a0（破棄口）、`+0x10`=0x568e90（スカラ削除破棄 ct・`FUN_00568640` → free）、`+0x14`=0x568730、`+0x18`=0x569140、`+0x1c`=0x566530。`+0x10` は旧パッケージの終了/破棄に使われる。
- **パッケージ設置関数 `FUN_005f1590`（thiscall, actor, newPkg, flag）**。新パッケージの種別バイト `newPkg+0x20` を読み、変更が必要なら旧パッケージの vtable `+0x10` を呼んで終了し、`process+8` へ新パッケージ、`process+4=0` を設置。`FUN_005660a0`（`pkg+0x1c` の bit0xb 判定）+ `FUN_00566830` で後始末。
- **種別ファクトリ（基底 ctor `FUN_00568e10` の呼出元 = 34 個）**。例 `FUN_005f5170`: タイプ 0xe/0xd（Alarm 系）を `AiPackageSetType` で設定、`+0x18=3`、0xc バイトのサブオブジェクト 2 個（ctor `FUN_005696b0` / `FUN_00569e40`）を追加し、`FUN_005f1590` で設置。ファクトリ群: 0x5f5170 / 0x5f5730 / 0x5f5870 / 0x5f59e0 / 0x5f7cf0 / 0x5f7ec0 / 0x5f8000 / 0x5f80d0 / 0x5f8170 / 0x5f94b0 / 0x5f9eb0 / 0x606860 / 0x6068d0 / 0x60ec60 / 0x611d70 / 0x612240 / 0x61bb60 / 0x625d00 / 0x625d80 / 0x627490 / 0x631dc0 / 0x631ec0 / 0x643e30 / 0x646020（複合ビルダ）/ 0x660760 / 0x66cc40 / 0x6765f0 / 0x67c260 / 0x67d300 / 0x67d3a0 / 0x692430 / 0x6a40b0。
- **実行モデル結論**: 33 種別を一括 switch する「単一 executor」は存在しない。種別ごとの挙動は (a) 選択系の種別述語ハンドラ（§17.5 の `AiPackage_IsDialogue` / `FUN_005e6ba0` IsAlarm / `AiPackage_IsSurface` / `AiPackage_IsEatType5` / `FUN_005e6cd0` IsFleeing など）と (b) パッケージ・サブクラス vtable 上書き＋種別ファクトリが担う。`PTR_DAT_00b152b0` は関数表ではなく、`pkg+0x18` を索引とする *.data* の種別条件表（4 バイト値、0x2c=44 と比較）。

- **種別ファクトリ ↔ 種別番号 対応表（確定・2026-10-11）**: 各ファクトリの ctor が `AiPackageSetType(N)` により **型バイトを `[[this+8]+0x20]` へ書き、その値が AI 手続き種別名表 `0xB12988`（33 エントリ）のインデックス** になる。番号 == 手続き種別インデックス。

  | 種別番号 | 手続き種別名（0xB12988） | ファクトリ（`AiPackageSetType` 直接） |
  |---|---|---|
  | 0x00 | Find | `0x5f5170` |
  | 0x01 | Follow | `0x631ec0` / `0x643e30`（参照検索 variant） |
  | 0x05 | Wander | `0x6765f0` |
  | 0x06 | Travel | `0x631dc0` / `0x660760` |
  | 0x0c | Combat | `0x61bb60` |
  | 0x0d | CombatLow | `0x646020`（複合ビルダ） |
  | 0x0f | Alarm | `0x60f250` |
  | 0x10 | Flee | `0x627490` |
  | 0x11 | Trespass | `0x67d3a0` |
  | 0x12 | Dialogue | `0x60eee0` |
  | 0x13 | Spectator | `0x67c260` |
  | 0x14 | ReactToDead | `0x5f9eb0` |
  | 0x15 | GetUp from Chair/Bed | `0x5f7ec0` |
  | 0x16 | Mount Horse | `0x612240` |
  | 0x17 | Dis-mount Horse | `0x611d70` |
  | 0x18 | Do Nothing | `0x5f8000` / `0x6a40b0`（companion 空スロット） |
  | 0x19 | Cast Target Spell | `0x5f59e0` |
  | 0x1a | Cast Touch Spell | `0x5f5870` |
  | 0x1b | Vampire Feed | `0x60ec60` / `0x66cc40` |
  | 0x1c | Surface | `0x5f5730` |
  | 0x1d | Search for Attacker | `0x5f94b0` |
  | 0x1e | Clear Mount Position | `0x5f80d0` |
  | 0x1f | Summon Creature Defend | `0x5f8170` / `0x692430` |
  | 0x20 | Movement Blocked | `0x5f7cf0` |

- **種別なしサブクラスファクトリ**（自前 vtable、`SetType` を直接呼ばず親族の一部）:
  - `0x606860` / `0x6068d0` — vtable `0xa6ef94`、`+0xf` に 8 バイトサブオブジェクト → **種別 0x0f（Alarm 系）**。
  - `0x625d00` / `0x625d80` — vtable `0xa7151c`、後者は `FUN_0060eee0`（Dialogue/0x12）から生成 → **種別 0x12系**。
  - `0x67d300` — vtable `0xa747bc`、`+0x13=-1`、`+0x14=1` → **種別 0x11系（Trespass）**。

- **種別ディスパッチャ `FUN_00463ec0`（確定）**: パッケージファクトリ自体ではなく、`switch(param_3)` でサブファクトリを選択（case 0xc→`0x61bb60` / 0xf→`0x606860` / 0x10→`0x627490` / 0x11→`0x67d300` / 0x12→`0x625d00` / default→基底`FUN_00568e10`）。呼出元: `FUN_00426a90` / `FUN_0060d3c0` / `FUN_006564c0`。
- **`FUN_00447df0`は除外**: TESDataHandler のフォーム生成ディスパッチャ（case 0x3d で `FUN_00568e10`=AI 基底を生成。他 case は別 TESForm）。

補足: 旧記載 `FUN_005f5170` の「0xe/0xd」（Alarm 系）は誤りで、`FiPackageSetType(0)`=Find を確定。`FUN_00569ee0/0x569ed0/0x569e90/0x569ec0` は種別セッターではなくパラメータ（`+4` 等）セッター。

### 17.9 未解決事項

- (解決済み) 種別バイトの書き込み箇所。`AiPackageSetType`（`0x5661e0`）が **`[[this+8]+0x20]`** へ書く。基底 ctor 走査 218 件は vtable 一致なしだったが、型バイトは ctor 直接書込ではなく `AiPackageSetType` 経由であることを確定。
- (解決済み) 種別ファクトリ 34 個 ↔ 種別番号 0..0x20 対応表の完全決定（§17.8）。重複種別（1/6/0x1b/0x18/0x1f）は variant ファクトリ。`0x447df0`（TESForm 生成）と `0x463ec0`（種別ディスパッチャ）は除外確定。
- `PTR_DAT_00b152b0` の各エントリが指す *.data* 構造（00b150d4〜00b152a8 の 44 個）の内容確認。
- 34 個のファクトリ各自の vtable と、基底 `0xa6743c` からの差分スロット（= 種別固有実行）の抽出。

## 18. ESM PACK / PKDT の実形式（検証済み）

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Data\Oblivion.esm`（277,504,985 バイト、TES4 ヘッダ 744 バイト、最初のトップレベル GRUP は offset 764）。

### 18.1 レコード走査手順

1. オフセット 20 の TES4 ヘッダサイズを読み、その直後からトップレベル GRUP 列が始まる。
2. **レコードは GRUP 内にネストする。** トップレベルのみを走査すると PACK は 0 件になる。GRUP（`GRUP` + u32 サイズ + 4 バイトラベル + i32 グループ型）を再帰的に降下する必要がある。
3. 再帰降下の結果: **レコード 36,536 件 / 56 種別 / PACK = 7,209 件**。
4. レコードヘッダは `4 バイトタグ + u32 データサイズ + u32 フラグ + u32 formID + u32 タイムスタンプ + u16 バージョン + u16 不明`（計 20 バイト）。フラグ bit 0x00040000 は圧縮（先頭 4 バイト = 展開後サイズ）。
5. サブレコードは `4 バイトタグ + u16 サイズ + データ`。

### 18.2 PACK レコードのサブレコード頻度

| タグ | 件数 | 意味 |
|---|---|---|
| PKDT | 7209 | パッケージ本体（型・フラグ・時刻制約） |
| PSDT | 7209 | スケジュール日付 |
| EDID | 7202 | エディタ ID |
| PLDT | 6576 | 場所 | 
| CTDA | 6172 | 条件 |
| PTDT | 1776 | 対象 | 
| CTDT | 18 | 条件データ |

PACK レコードのフラグは全件 0（= 非圧縮）。

### 18.3 PKDT の実レイアウト（2 形式）

PKDT には長さの異なる **2 形式** が混在する。

| 形式 | 件数 | formID 範囲 | 型バイト位置 | レイアウト |
|---|---|---|---|---|
| len8（現行） | 6,648 | 0x85C – 0x18D801 | **byte[4]** | u32 flags + byte[4]=type + byte[5]=hour + byte[6]=day + byte[7]=month |
| len4（旧形式） | 561 | 0x346B – 0x36363 | **byte[2]** | byte[0..1]=flags + byte[2]=type + byte[3]=hour |

- len4 の formID は全件 mod index 0 かつ GRUP PACK 内で、Creation Set 系の初期編集データと推定。len8 は現行ツール出力。
- 実例: `MS31LynchWander` = `00 00 05 00`（type=5=Wander）、`aaaDefaultStayAtCurrentLocationSkipFallout` = `11 10 06 00`（type=6=Travel）、`MS31MinxWander` = `00 00 05 61`（type=5、hour=0x61）。

### 18.4 型バイト位置の統計的確定

EDID 名のトークンから期待型を推定し、各バイト位置と突き合わせた一致率（`tmp\pkdt_offset_test.py`）:

| PKDT len4 (n=561、期待型が推定できた 414 件) | | PKDT len8 (n=6648、3,671 件) | |
|---|---|---|---|
| byte[0] | 4.3% | byte[0] | 5.5% |
| byte[1] | 4.3% | byte[1] | 4.4% |
| **byte[2]** | **67.6%** | byte[2] | 7.9% |
| byte[3] | 2.9% | byte[3] | 9.2% |
| | | **byte[4]** | **82.3%** |
| | | byte[5] | 5.6% |

正しいオフセットだけが突出し、他は 10% 未満（偶然水準）。長さごとに型バイト位置が異なることを統計的に確定した。

### 18.5 型値と EXE の 33 エントリ表との一致

PKDT の型値は 12 種（0..11）で、**`0xB12988` の 33 エントリ AI 手続き型名テーブルの先頭 12 エントリと完全一致**する。EDID 名（Wander / Patrol / Eat / Sleep / Escort / Follow / Accompany / Flee / Cast 等）との相関で検証済み。

| 値 | 型名 |
|---|---|
| 0 | Find |
| 1 | Follow |
| 2 | Escort |
| 3 | Eat |
| 4 | Sleep |
| 5 | Wander |
| 6 | Travel |
| 7 | Accompany |
| 8 | UseItemAt |
| 9 | Ambush |
| 10 | FleeNotCombat |
| 11 | CastMagic |

したがって §17.2 で観測したアクタープロセス `+0x20` の型バイトは、**ESM の PKDT から供給される**ことが確定した（ソースと消費側が同一の索引空間を共有する）。

### 18.6 Android 実装との差分（要修正）

`app\src\main\cpp\assets\esm_reader.h` / `esm_reader.cpp` の AI パッケージ解析は実形式と一致しない。

| 項目 | Android 現状 | 実 EXE / ESM |
|---|---|---|
| サブレコード | `AI_A` / `AI_E` / `AI_F` / `AI_T` / `AI_W` / `AI_PK` という独自スキーム | `PKDT` / `PSDT` / `PLDT` / `PTDT` / `CTDA` / `CTDT` / `EDID` |
| 型 enum | `AIPackageType`（FIND=1 起点 15 値）、`ai::PackageType`（独自 16 値順） | 0 起点 12 値（上表）、EXE の 33 エントリ表と同一 |
| PKDT 解釈 | 独自 | len8 = 型 byte[4]、len4 = 型 byte[2]（長さで分岐） |

修正方針: `decodeNPC` の AI サブレコード解析を実 PACK 形式（`PKDT`/`PLDT`/`PTDT`/`PSDT`/`CTDA`）に置き換え、型 enum を 0 起点の上表に合わせる。長さ 8 と 4 の両 PKDT を扱う必要がある。

### 18.7 EXE 側 PKDT ローダー（実測で確定）

EXE 内の `PKDT` 文字列は `0x5664df` と `0x5665af` の 2 箇所のみで、いずれも同じ関数を指す。

| アドレス | 関数 | 役割 |
|---|---|---|
| `0x566530` | `AiPackageReadSubrecords` | PACK のサブレコードを読み分ける本体 |
| `0x5664d0` | `AiPackageLoadSave` | ロード/セーブ共通エントリ。`FUN_0046ae40(0x54444b50, package + 0x1c, 8)` で PKDT 8 バイトを `+0x1c` へ直接読み込む |
| `0x5661e0` | `AiPackageSetType` | 型セッター。**`*(char*)(package + 0x20) = packageType`** |

`AiPackageReadSubrecords` の PKDT 分岐（デコンパイル実測）:

```c
else if (tag == 0x54444b50) {              // 'PKDT'
  if (*(int *)(param_2 + 0x254) == 4) {     // サブレコード長 4
    FUN_00450c20(&local_18, 4);
    param_1[7] = local_18 & 0xffff;         // flags = u16
    FUN_005661e0((int)local_18._2_1_);      // type  = byte[2]   ★
  }
  else if (*(int *)(param_2 + 0x254) == 8) { // サブレコード長 8
    FUN_00450c20(&local_20, 8);
    param_1[7] = local_20;                   // flags = u32
    FUN_005661e0((int)(char)local_1c);       // type  = byte[4]   ★
  }
  else {
    FUN_004a7a60("File '%s' contains package data of unrecognized type.", param_2 + 0x1c);
    param_1[7] = 0;
  }
}
```

**この分岐が Python 側の統計解析（18.3 / 18.4）と完全に一致する。** 長さ 4 のとき型は `byte[2]`、長さ 8 のとき型は `byte[4]`。長さが 4 でも 8 でもない場合は警告を出して型 0 に落とす。

`package + 0x1c` の 8 バイトがそのまま PKDT バッファであり、`AiPackageSetType` が `+0x20` に型バイトを書く。したがって:

```
Oblivion.esm  PACK/PKDT (byte[2] or byte[4])
        │  AiPackageReadSubrecords
        ▼
package + 0x1c  (PKDT 生 8 バイト)
        │  AiPackageSetType
        ▼
package + 0x20  (型バイト = 0..32 の 33 エントリ表索引)
        │  §17.2 / §17.6 の全比較サイト
        ▼
プロセス種別ディスパッチ
```

`AiPackageSetType` は型に応じてサブオブジェクトを割り当てる（`FUN_00401f00(0xc)` + `FUN_00569e40` / `FUN_005696b0`）。型 3..6 では `+0x28` を、型 7 では `+0x24` をいったん解放してから作り直す。呼出元は 31 件で、ESM ローダーだけでなく種別別ハンドラ（`FUN_005f5170` / `FUN_005f5730` / `FUN_005f5870` / `FUN_005f59e0` / `FUN_005f7cf0` 系など）からも呼ばれる。

未確定だった「種別バイトの書き込み箇所」（§17.8）は、**ESM の PKDT 読み込み時に `AiPackageSetType` 経由で書かれる**ことで確定した。

### 18.8 検証スクリプト

| ファイル | 役割 |
|---|---|
| `tmp\pack_scan3.py` | 再帰 GRUP 走査で PACK = 7209 を確立 |
| `tmp\pkdt_hist2.py` | PKDT 全件のバイト別ヒストグラム（len4 / len8 の 2 形式を特定） |
| `tmp\pkdt_types.py` | EDID 相関で byte[4] が 33 エントリ表索引と一致することを確定 |
| `tmp\pkdt_len4dump.py` | len4 の実 PKDT 生バイトを出力（型 = byte[2] の根拠） |
| `tmp\pkdt_variant.py` | len4 / len8 の formID・GRUP・フラグ分布 |
| `tmp\pkdt_offset_test.py` | 型バイト位置の統計的確定（18.4 の表を生成） |

### 17.10 戦闘システム（Combat AI）— 2026-10-11 追記

`re-combat` の初期調査。戦闘は AI パッケージ種別 0x0c（Combat）の中核。

| 関数 | 役割 |
|---|---|
| `AiPackageCombat_Create`（0x61bb60） | Combat AI パッケージファクトリ。サブオブジェクト vtable `0xa70a14`、`AiPackageSetType(0xc)`、find サブオブジェクト2個を生成 |
| `CombatEngine_EngageInit`（0x616190） | 戦闘エンゲージ初期化。対象アクタの戦闘条件（距離・生命）を検証して参戦 |
| `CombatEngine_CollectTargets`（0x61b190） | 戦闘対象の収集。世界内アクタ・パーティローダを走査して `0x61b190` で対象を確定 |
| `Combat_GetActorDistance`（0x4d7e90） | アクタ間距離取得（vtable+0x174 の位置取得を使用、`FUN_004d7e30` 経由） |
| `Combat_IsAlerted`（0x614290） | 攻撃性状態判定。`FUN_006135f0`（戦闘状態）+ `FUN_005e0530`/`FUN_005e1cb0`（感知判定）が非戦闘なら +0x174 のアラートフラグを返す |
| `Combat_GetAttackRange`（0x6142d0） | 攻撃レンジ（最適距離/最大距離）を返す。状態 0xd の接近・`FUN_00612d60`（選択武器）に応じて、`DAT_00b37110/0x37118`（デフォルト射程、接近 1000/2000.0 等）から決定。デバッグ文字列 `Attack Range: %.2f(optimal) %.2f(max)` を使用 |
| `Combat_GetAttackReach`（0x615520） | 攻撃到達距離ゲッター。+0x188 にキャッシュ。param_1+0x70（状態）で分岐：0/3/0xd=武器range×`vtable+0xec`、1=魔法、2/4=遠隔（`Combat_GetAttackRange`）、from vtable+0x26c |
| `Combat_GetCurrentTarget`（0x6135f0） | 戦闘対象リスト（param_1+0x40）から現在の有効ターゲットを返す。空エントリは線形削除・解放 |
| `Combat_MeleeEngageCheck`（0x6191b0） | 近接戦闘のエンゲージ判定。リーチ（`Combat_GetAttackReach`）と対象距離（`FUN_00615980`）を比較し、攻撃遷移（param_1+0x74=3, +0x78=1）または突進移動を判断 |

**構造所見**
- 攻撃距離属性 `+0x188`（リーチ）/`+0x184` は本クラスタで算出・キャッシュされる。状態 `param_1+0x70`（0..4, 0xd）で近接/遠隔/魔法を切替、`vtable+0x26c`（武器range）、`vtable+0xec`、`vtable+0x144/0x148`（乗算係数）を使用。

**構造所見**
- `AiPackageCombat_Create` はベース ctor（`FUN_00568e10`）→ find サブオブジェクト（`FUN_005696b0`/`FUN_00569e40`）→ `FUN_00616190` による戦闘初期化の順。vtable `0xa70a14` の先頭起動スロット 2 つは `0x60d0a0`（no-op）で、パッケージ終了/状態遷移は別経路。
- `CombatEngine_EngageInit`（0x616190）は「param_1+0x3c の対象」「param_2 のアクタ」を比較し、`DAT_00b333c4`（プレイヤ）判定、参戦距離（`Combat_GetActorDistance`）、アジテーション閾値を確認。参戦時は 0x14 バイトの戦闘イベント（`FUN_00401f00(0x14)`）を作成し `FUN_00416650` でスケジュール。
- `CombatEngine_CollectTargets`（0x61b190）は (1) 世界 파티ローダ走査、(2) パーティアイテム、(3) アクタ、と複数ルートでターゲットを収集。`FUN_004d8950`/`FUN_004d88f0` でパーティを列挙、`FUN_009832e6` で formID 解決。
- `Combat_IsAlerted` が参照する `FUN_006135f0`（戦闘状態マシン）がダメージ計算への入り口になる可能性が高い（要続行）。`FUN_006142d0` は参照18個と戦闘AI実体の候補。

**未解決**
- 物理ダメージ計算は **§17.11 で確定**。魔法効果適用（マジカ・エンチャント）と HP 減算の実装箇所は未解読。
- ヒットイベント処理（`FUN_00416650` スケジュール先）を追う必要がある。
- マズル/射線（raycast）判定の関数（`FUN_004d7e90` 周辺）は要確認。

### 17.11 物理ダメージ計算式（確定）— 2026-10-11 追記

`re-combat` の核心成果。**近接・物理ダメージの計算式が完全に確定した。**

#### 17.11.1 ダメージ式 `Combat_CalcMeleeDamage`（0x547070）

```
float Combat_CalcMeleeDamage(
    int   baseSkill,        // param_1: 基礎スキル値（0..100）
    int   luck,             // param_2: 運 (Luck) 属性値
    float skillRatio,       // param_3: スキル寄与率 (0..1)
    float fatigueRatio,     // param_4: 疲労率 (0..1)
    float weaponCondition,  // param_5: 武器コンディション (0..1)
    float weaponDamage,     // param_6: 武器の基本ダメージ
    float multiplier,       // param_7: 外部倍率（パワーアタック・スニーク等）
    int   useWeapon)        // param_8: 0 以外で武器倍率を適用
{
    float skill   = Actor_CalcSkillWithLuck(baseSkill, luck);   // 0..100 クランプ
    float dmg     = weaponDamage * fDamageWeaponMult;          // 0xb36dd8
    float skillF  = fDamageSkillMult * skillRatio + fDamageSkillBase;            // 0xb36de8 / 0xb36de0
    float strF    = fDamageStrengthBase + fDamageStrengthMult * (skill * 0.01);  // 0xb36e00 / 0xb36e08
    float condF   = 1.0;                                       // 既定
    if (useWeapon != 0) {
        clamp(weaponCondition, 100);
        condF = weaponCondition * 0.01 * fDamageWeaponConditionMult
              + fDamageWeaponConditionBase;                    // 0xb36df8 / 0xb36df0
    }
    float fatF = (fatigueRatio == 1.0) ? 1.0
                                       : Combat_GetFatigueDamageMult(fatigueRatio);  // 0xb37530/0xb37538

    return skillF * dmg * strF * condF * fatF * multiplier;
}
```

デコンパイル原文（要旨）:

```c
fVar5 = Actor_CalcSkillWithLuck(param_1, param_2);            // 0..100 クランプ済み
fVar1 = param_6 * _DAT_00b36dd8;                              // 武器ダメージ
fVar2 = _DAT_00b36df8 * param_3 + _DAT_00b36df0;              // スキル寄与
fVar3 = _DAT_00b36de0 + _DAT_00b36de8 * fVar5 * 0.01;         // 属性寄与
if (param_5 > 100.0) param_5 = 100.0;
fVar4 = param_5 * 0.01 * _DAT_00b36e08 + _DAT_00b36e00;       // 武器コンディション寄与
fVar5 = (param_8 == 0) ? Combat_GetFatigueDamageMult(param_4) : 1.0;
return fVar2 * fVar1 * fVar3 * fVar4 * fVar5 * param_7;
```

**乗算構造**: 「スキル寄与 × 武器ダメージ × 属性(筋力)寄与 × 武器コンディション寄与 × 疲労寄与 × 外部倍率」。各要素は GameSettings で個別に調整可能な線形式であり、Android 移植側の `CombatManager` はこの乗算チェーンをそのまま再現すればよい。

#### 17.11.2 グローバル設定変数のアドレス対応（確定）

`.ini` / GameSetting 名は設定登録コード（`0x009e8cxx` / `0x009eaaxx` / `0x009ebxxx`）の
`MOV ECX, <変数アドレス>` + `PUSH <設定名文字列>` の組から逆引きした。

| アドレス | 設定名 | 用途 |
|---|---|---|
| `0x00b36dd8` | `fDamageWeaponMult` | 武器ダメージ倍率（既定 1.0） |
| `0x00b36de0` | `fDamageSkillBase` | スキル寄与の定数項 |
| `0x00b36de8` | `fDamageSkillMult` | スキル寄与の係数 |
| `0x00b36df0` | `fDamageWeaponConditionBase` | コンディション寄与の定数項 |
| `0x00b36df8` | `fDamageWeaponConditionMult` | コンディション寄与の係数 |
| `0x00b36e00` | `fDamageStrengthBase` | 筋力寄与の定数項 |
| `0x00b36e08` | `fDamageStrengthMult` | 筋力寄与の係数 |
| `0x00b36e10` | `fDamageToWeaponPercentage` | 武器へのダメージ転換率 |
| `0x00b36e18` | `fDamageToArmorPercentage` | 防具へのダメージ転換率 |
| `0x00b36e20` | `fDamagePowerAttackBonus` | パワーアタック基礎ボーナス |
| `0x00b36c70` | `iCurrentTargetBonus` | 攻撃対象選択時の「現在対象」ボーナス（`Combat_SelectAttackTargetInRange` が加算） |
| `0x00b36d00` | `fAIMeleeWeaponMult` | AI 近接（武器）ダメージ倍率 |
| `0x00b36d10` | `fAIMeleeHandMult` | AI 近接（素手）ダメージ倍率 |
| `0x00b36d20` | `fAIMagicSpellMult` | AI 魔法（スペル）ダメージ倍率 |
| `0x00b36d30` | `fAIFleeHealthMult` | AI 逃走判定の体力倍率 |
| `0x00b37388` | `iActorLuckSkillBase` | 運→スキル変換の定数項 |
| `0x00b37390` | `fActorLuckSkillMult` | 運→スキル変換の係数 |
| `0x00b37530` | `fFatigueBase` | 疲労倍率の定数項 |
| `0x00b37538` | `fFatigueMult` | 疲労倍率の係数 |

設定登録コードの定型:

```
FLD  float ptr [<既定値定数>]
PUSH ECX
FSTP float ptr [ESP]          ; 既定値を引数として積む
PUSH <設定名文字列>            ; 設定名
MOV  ECX, <変数アドレス>       ; this（GameSetting オブジェクト）
CALL 0x0041bae0               ; GameSetting 登録
PUSH <参照テーブルentry>       ; 0xa1e2d0 以降の float* 配列
CALL 0x00981fb4               ; 配列登録
RET
```

補助定数: `0x00a3d9a4` / `0x00a2fb0c` = `0.01f`。

#### 17.11.3 補助関数

| 関数 | 役割 |
|---|---|
| `Actor_CalcSkillWithLuck`（0x547b90） | `baseSkill + luck * fActorLuckSkillMult + iActorLuckSkillBase` を 0..100 にクランプ。**運補正込みの実効スキル値** |
| `Combat_GetFatigueDamageMult`（0x547f00） | `fFatigueBase - (1.0 - fatigueRatio) * fFatigueMult`。疲労に応じたダメージ倍率（疲労 1.0 で `fFatigueBase`、疲労 0 で `fFatigueBase - fFatigueMult`） |
| `Combat_GetPowerAttackDamageBonus`（0x546ba0） | パワーアタック種別ごとのボーナス。`FUN_0056a300(actor)` で取得したパーク数に応じ switch: `0x16`=常時、`0x17`=パーク>3、`0x18`=パーク>2、`0x19`/`0x1a`=パーク>1 で `FUN_00403c00()` から倍率を取得し、既定は `fDamagePowerAttackBonus`（0xb36e20 = 1.0）。`0x16..0x1a` は `fDamagePowerAttackBonus` + `fDamagePowerAttack{Stand,Side,Back,Forward}Bonus` の 5 方向ボーナスに対応 |
| `Combat_CalcAIAttackDamage`（0x547140） | 攻撃エントリ 1 件の想定ダメージ。対象の vtable+0x10 でフォーム種別を取り、`Combat_CalcMeleeDamage` を `useWeapon=0` で呼ぶ。エントリが魔法（`FUN_004149f0()` が真）なら `magnitude * fAIMagicSpellMult`（0xb36d20）を加算し、全体に `fAIMeleeWeaponMult`（0xb36d00）を乗じて返す |
| `Combat_ApplyAIMeleeDamageMult`（0x546c60） | `value * (handToHand ? fAIMeleeHandMult : fAIMeleeWeaponMult)`。AI 近接ダメージの最終倍率適用 |
| `Combat_AISelectBestAttack`（0x48bda0） | **AI 攻撃選択**。アクタの攻撃エントリ（武器・スペル）を列挙し、各エントリのダメージを `Combat_CalcAIAttackDamage` / `Combat_CalcMeleeDamage` で見積もり、**最大ダメージのエントリ**を 0xc バイト構造体 `{damage, form, next}` で返す。`param_3` に最大ダメージを書き出す。`param_4` はフォーム種別フィルタ（`-1` で全種別） |
| `Combat_GetAIAttackDamage`（0x612560） | 戦闘 AI 側のラッパ。アクタのアクションキュー `vtable+0x288/0x284`（opcode 7/10）で武器・スキルを取得し `Combat_CalcAIAttackDamage` を呼ぶ |
| `Combat_SelectAttackTargetInRange`（0x619d40） | 対象距離 vs `Combat_GetAttackReach()` を比較し、到達時にアクションキュー `vtable+0x284`（opcode 8）へ攻撃を投入。`FUN_00519c10` → `FUN_00547910(乱数, 値)` でスタガー時間を決定。`FUN_009828c0() != -1.0` なら `_DAT_00b36c70` を加算 |

#### 17.11.4 呼び出し元

`Combat_CalcMeleeDamage` の呼び出し元:

| 呼び出し元 | 備考 |
|---|---|
| `Combat_CalcAIAttackDamage`（0x547140） | AI 攻撃エントリのダメージ見積り（上記）。呼出元は `Combat_AISelectBestAttack`（0x48bda0）と `Combat_GetAIAttackDamage`（0x612560） |
| `FUN_004891c0` | 攻撃処理系 |
| `FUN_0048bda0` | 攻撃処理系 |
| `FUN_00484f80`（2 箇所） | 攻撃処理系 |
| `FUN_0051a120` | フォーム種別ディスパッチャ（`*(param_2+4)` で switch: `0x14` 武器 / `0x16` 防具 / `0x1a`=100.0 / `0x21` ポーション / `0x22`）。HP ではなく「フォームが与えるダメージ値」を返す |

`Combat_GetPowerAttackDamageBonus` ← `FUN_005febf0`（NPC AI の疲労/パワーアタック計算）。
`Actor_CalcSkillWithLuck` / `Combat_GetFatigueDamageMult` は属性・スキル表示系を含む多数の関数から呼ばれる汎用ヘルパー。

#### 17.11.5 AI 攻撃選択フロー

`Combat_AISelectBestAttack`（0x48bda0）が「この NPC はどの攻撃を使うべきか」を決定する中核であることが判明した。

```text
AI 戦闘パッケージ（種別 0x0c）/ Combat_GetAIAttackDamage（0x612560）
        │
        ▼
Combat_AISelectBestAttack（0x48bda0）
        │  アクタの攻撃エントリ（武器・スペル）を列挙
        ├─ 武器エントリ ─→ Combat_CalcAIAttackDamage（0x547140）
        │                      └─ Combat_CalcMeleeDamage（0x547070）  ← §17.11.1 の式
        │                             ├─ Actor_CalcSkillWithLuck          （0x547b90）
        │                             └─ Combat_GetFatigueDamageMult      （0x547f00）
        │                      └─ エントリが魔法なら + 威力 × fAIMagicSpellMult
        │                      └─ 全体に × fAIMeleeWeaponMult
        └─ 最大ダメージのエントリを 0xc バイト構造体 {damage, form, next} で返す
               │
               ▼
        Combat_ApplyAIMeleeDamageMult（0x546c60）
               └─ 素手なら × fAIMeleeHandMult、武器なら × fAIMeleeWeaponMult
```

呼出元: `Combat_AISelectBestAttack` ← `FUN_0043cde0` / `FUN_0051e240` / `FUN_005227a0` / `FUN_005fb9f0`（AI パッケージ層）。
`Combat_GetAIAttackDamage` ← `FUN_00612aa3` / `FUN_00621b40`（戦闘 AI 本体）。


#### 17.11.6 ダメージ適用チェーン（HP 減算経路・確定）— 2026-10-?? 追記

`§17.11.5` で未解読としていた HP 減算箇所が判明した。ダメージ適用は仮想スロット `+0x2a4`（= `0x5e2be0`）を経由して ActorValue へ差し込まれる。

**核心関数**

- `0x5e2be0`（Actor_ModifyActorValue、仮想 `+0x2a4`）: 全 AV デルタ適用の窓口。
  - AV==8(Health) かつ delta<0（ダメージ）のとき `+0x3b8` を呼ぶ（後述の死判定）。
  - 実値は `param_1[0x16]`（AV デリゲート）の `+0x288`(SetActorValue) で格納。
- `0x6034b0`（仮想 `+0x3b8`）: 「HP(AV8) < 1.0 → 死亡処理 `0x5f005f0`」の死判定。`
- `0x5f1a60`（仮想 `+0x288`, GetActorValue）: AV 読み出し。値は `param_1[0x16]` の `+0x26c` で取得。

**AV 回復器（差分を `+0x2a4` へ渡す）**

- `0x5f2530` = Actor_RecoverHealth（AV8, 現在値との差分を適用）
- `0x5f25f0` = Actor_RecoverMagicka（AV9）
- `0x5f2720` = Actor_RecoverFatigue（AV10）

**仮想スロット逆引き（MOV `[reg+0xOFFSET]` の即値スキャン）**

Ghidra は間接 vtable CALL を `CALL EAX`/`CALL EDX` に畳み込むため、文字列マッチでは抽出できない。
`MOV reg,[base+0xOFFSET]` のオペランド即値を全関数スキャンする `REvcall.java` で逆引きする（`tmp/vcalls.txt`、1382 件）。
`+0x2a4` をロードする主要関数: `0x5e07d0`(Fatigue適用), `0x5f2530/5f25f0/5f2720`(回復器), `0x5f6020`(装備/アニメ更新), `0x5faea0`, `0x6005f0`, `0x603ca0` など。

**追加確定（2026-10-??）**

- `0x60cf60`（仮想 `+0x3a4`、Actor）: **空関数（no-op）**。`return;` のみ。
- `0x5e58c0` / `0x5e58d0`（`+0x390`/`+0x394` の実体）: `[this+0x58]` サブコンポーネントへ委譲する thunk。
  - `0x5e58c0` → サブコンポーネントの仮想 `+0x398`
  - `0x5e58d0` → サブコンポーネントの仮想 `+0x39c`（SetActorValue、null ガード付き）

**ダメージ受領入口（`FUN_005febf0` 攻撃ヒット解決）**

`FUN_005febf0`（攻撃ヒット解決本体、vtable `+0x3ac` 相当）が、計算済みダメージ量 `uStack_1b4`・攻撃者 `param_4`・部位/ブロック `local_1bc` を
**対象 Actor の vtable スロット `+0x2d4`** へ渡す:
`(**(code **)(*param_1 + 0x2d4))(damage, attacker, limbs)`。
ここから内部経路（武器種別・スキル・エンチャント判定→`+0x2a4` ModActorValue）を経て AV8(Health) へ減算される。
- `+0x2d4` は AV デリゲート（`param_1[0x16]`）の vtable にも存在し、武器装備アクセサ等にも使用される（後述）。
- `FUN_005fcab0`（家族 `FUN_005fcab0`/`FUN_005f43e0` 等）: 武器/エンチャントスキル処理の大型ハンドラ
  （switch で武器種別 `WPNBlade1HandEquipEnchanted` 等を選択、AV デリゲート `+0x2d4` の武器装備アクセサを多用）。

**`+0x2d4` 実体の確定（対象 Actor vtable 基底 `0xa6e074`）**
- vtable 本体 `0xa6e074` から逆算: `+0x2a4`=`0xa6e318`→`0x5e2be0`, `+0x3b8`=`0xa6e42c`→`0x6034b0` と全て整合。
- `+0x2d4`（`0xa6e348` および別 Actor vtable `0xa6fc9c+0x2d4`）= **`0x005e46c0`**（`FUN_005e46c0`、520B）。
- **`FUN_005e46c0` の役割**: 攻撃ヒット時のダメージ受領・抵抗適用・反映。
  - param_2=呪文/エンチャントデータ（`param_2[2]` の `+4` が `'!'` = 効果種別）、param_3=攻撃者、param_4=武器、param_5=成否/ブロック出力フラグ。
  - 浮動ダメージ比較で完全耐性/ブロック時は `*param_5=1` を返して減算なし（**ResistNormalWeapons / ReflectDamage に相当する耐性判定経路**）。
  - 補正ダメージは `FUN_00488aa0(fVar1 - fVar2, uVar5, uVar6)` で反映（この先が AV8 Health へ繋がる）。
  - `param_1+0x5c`（サブコンポーネント、vtable `+0x34`/`+0x3c`）へ effectStart/effect メッセージを投げる mod システム駆動。

- `FUN_005e46c0` が呼ぶ `FUN_00488aa0`(514B) は**ダメージ記録・蓄積リスト操作**であり、直接 AV8 減算はしない:
  - `param_4` をキーにリスト/ツリーを走査し、damage 量(`param_2`) を `FUN_0041ee90` で貯蓄、超過時 `FUN_0041f640` で分割/破棄。コンバットの受領ダメージ集計。
  - 実 HP 減算は確定済み `+0x2a4`=`0x5e2be0`(Actor_ModifyActorValue) 配下。`+0x2d4`(`0x5e46c0`)→`FUN_00488aa0` はその前段の耐性/記録層。

**残課題**（`re-damage-apply` 継続）
- Health 実値の最終格納ブロック（delegate `param_1[0x16]` 側の `+0x26c` 実装）の追跡
  - 注意: Actor vtable `+0x26c`=`FUN_005e40a0`→`FUN_00547540`(`_DAT_00b36f20*_DAT_00b36e90`) は**damage 係数アクセサ**で実 AV 値とは別物
- `FUN_005e46c0` → 耐性判定後の実減算へ至る連絡線（ModActorValue 呼び出し位置）の詳細整理

#### 17.11.7 GetActorValue 実値読み出し最奥の特定 — 2026-10-11 追記

`§17.11.6` の「実値格納ブロックの追跡」（re-av-store）を進めた結果、AV 読み出しの**最終実装**が判明した。

**GetActorValue(0x5f1a60) の delegate null フォールバック**
`0x5f1a60`（仮想 `+0x288`）は、AV デリゲート `param_1[0x16]` が null のとき `FUN_005f1910(param_2)` を呼ぶ。これは基底 AV アクセサ用フォールバックである。

- `0x005f1910`（82B）: wrapper。`FUN_005ead00(param_1)` を呼び、その後 `FUN_009828c0()` を 2 回呼ぶだけの軽量ラッパ。
- **`FUN_005ead00`（259B、実 AV 読み出し本体）**: `int *param_1`（Actor 自身）と `int param_2`（AV id）を受け、実 AV 値(float)を返す。

**FUN_005ead00 の分岐構造**
1. `param_2`(AV id) が 8〜11（= Health/Magicka/Fatigue/?? の基本 4 属性帯）かつ `param_1 == _DAT_00b333c4`（グローバルプレイヤー）のとき
   - `FUN_005e2210(param_2, &local_8+1, &local_8)` で**現在値対最大値の比（magicka 倍率）**を求め、`local_8[0]` が 0 なら 1.0 に補正。
2. `param_2` == 0x25 → `FUN_0041fc90()`（グローバル値系）、0x26 → `(*param_1+0x204)()`、0x27 → `(*param_1+0x208)()`。特殊 AV の別アクセサ。
3. 一般経路: `(*param_1 + 0x170)()` で**装備魔法ハンドル（magic/equip オブジェクト）**を取得。
   - そのハンドルが非 null かつ vtable `+400`(=0x190) が真のとき、そのオブジェクトを実値ソースとして保持。
   - **`(*piVar4 + 0x128)(param_2)`** — 装備魔法オブジェクトの vtable スロット `+0x128` を呼び、AV id から**実値（int として読まれ float へ変換）**を得る。
   - 戻り値: `(((float)iVar3 + local_8[0]) * unaff_EBP)`。つまり実値＋属性比を EBP の呼び出し元係数で補正。

**結論（実値格納先の確定）**
- AV 実値は **装備魔法オブジェクト（Actor vtable `+0x170` が返すハンドル先）の内部、vtable スロット `+0x128` でアクセスされる領域**に格納される。
- `FUN_005ead00` はデリゲート null 時の基底アクセサで、AV 8(Health) に magicka 倍率を掛ける点が HP 集計の特徴。
- note: `unaff_EBP` は本関数を呼ぶ下流（GetActorValue 呼び出し元）から渡される実行時係数（damage 補正等）で、関数単体では固定ではない。
- Actor vtable `+0x170` = `0x4d9b40`（4B getter）: `return *(param_1 + 0x1c)`。装備魔法ハンドルは **Actor メンバ offset 0x1c** に保持。装備魔法オブジェクトの型・vtable 基底・`+0x128` 実装の特定は `re-magic-object`（in_progress）。

#### 17.11.8 re-magic-object 完結（機能確定）— 2026-10-11 追記

`re-magic-object` は「機能的に確定」とし完了扱いとした。型名（vtable 基底）の静的特定は、Actor+0x1c への代入地点を広範に追跡する高コスト作業で収束が不確実なため、機能構造の確定をもって完了とする。

- **確定分**: AV 実値は装備魔法ハンドル（Actor vtable `+0x170` = `0x4d9b40` が返す `*(actor+0x1c)`）の内部に保持。`FUN_005ead00` はデリゲート null 時の基底アクセサで、装備魔法ハンドルが non-null かつ vtable `+0x190` が真のとき、`(*handle + 0x128)(av_id)` で実値（int → float）を読み出す。AV 8(Health) は magicka 倍率補正。
- **残課題（型名）**: 装備魔法ハンドルが指すオブジェクトの vtable 基底・`+0x128` 実装関数の具体名（`TESMAGIC_*` 等）は未特定。需要があれば Actor コンストラクタ / 装備魔法セット関数の追跡で確定可能。

**Android 移植済み（impl-av-store-magic-handle）** — 2026-10 追記
Android 版には delegate / 装備魔法ハンドル構造が無いため、本節の確定事実を `CharacterStatus::getActorValue(avId)` アクセサとして移植した（`game/npc.h` / `game/npc.cpp`）。
- AV ID 定数を `CharacterStatus` に追加（AV_STRENGTH=0〜AV_LUCK=7 の属性、AV_HEALTH=8、AV_MAGICKA=9、AV_STAMINA=10。AV 0-7 は属性 map を id 引数で参照）。
- AV 8(Health) は「現在値対最大値の magicka 倍率」補正を実装。`max==0` 時に比率を 1.0 にガード（`current/max` の除算 0 回避）。戻り値 `(currentHealth + ratio) * kCoeff`。`kCoeff`（原版 `unaff_EBP` 相当の実行時係数）は Android では 1.0 固定。
- 検証: `phase45_unit_tests.cpp` に `NPC_GetActorValue` テストを追加（満タン / 被ダメージ / ゼロ最大値ガード / 未知 ID=0）。


---

#### 18. RE 解析フェーズ総括 — 2026-10-11 追記

Oblivion Steam 版 EXE のリバースエンジニアリング（逆解析）フェーズの最終状態をまとめる。

**確定済みの主要成果**
- 描画パスディスパッチャ FUN_0080d110（__thiscall, 1878B, 関数ポインタテーブル 0x00a942d0 経由）: パスタイプ DAT_00b42e90 で分岐。case 0xe0/0xe1 は定数セッター（FUN_0076c910 / FUN_0076c730 / FUN_007aec20 / FUN_007aecb0 / FUN_007aec60）を呼び、case 0x18c/0x18d/0x18e/399 は描画委譲。
- **Toggles の CPU 側書き込み（2バンク分岐・確定）**: piVar2 = param_5+0x18（パスオブジェクト）から piVar2[0x56..0x59]（+0x158..+0x164）の4成分を Toggles にコピー。バンク選択 = DAT_00b42f32==0 OR DAT_00b42f48==1 の場合 _DAT_00b46688..94、それ以外 _DAT_00b4616c..78。
- シェーダー定数セッター配管、AV 実値格納先（装備魔法ハンドル内部, Actor vtable +0x170 = 0x4d9b40, +0x128 アクセサ FUN_005ead00）, 耐性/ダメージ/呪文経路（FUN_0059xxx 群）。

**未確定・動的トレース待ち（静的 xref = 0 のため）**
- パスオブジェクト構築関数（piVar2[0x56..0x59] の上流書き込み元）
- FUN_0080d110 への呼び出し元（テーブル 0x00a942d0 は間接ディスパッチでコード xref が無い）
- D3D9 Set*ShaderConstantF の実呼び出し確認

いずれもゲームを実際に対象シーン（3D 描画パス）まで進めてブレークポイントで観測する**動的トレース**が必要。実行環境（cdb/windbg 未導入・GUI ゲームのシーン進入は手動操作前提）が整えば再開可能。

**§18 Addendum — 2026-10-12 追記（静的解析による構造確定）**
- 0x00a942d0 は単純な関数ポインタ配列ではなく、**テクニック/パス記述子 vtable の一つ**であることを確定。
  - slot0 = FUN_0080d110（この vtable 固有の描画ディスパッチャ）
  - slot1 = FUN_0077a1b0（多くの vtable に共通。`(**(code**)(**(int**)(obj+0x3c)+8))(...)` サブオブジェクト vtable の slot8 へ委譲する共通スロット）
  - slot2 = 0x0080b7a0 / slot3 = FUN_0077a1f0 / slot4 = 0x0077a310 / slot5 = FUN_007790e0 … と続く（メモリ 0x00a942c0..fc 実測）
- 同じ構造のパス記述子 vtable 群が **0x00a8971c 〜 0x00a946f4** の広域に多数並列している（FUN_0077a1b0 の DATA xref が40箇所以上、例: 0x00a8971c/0x00a897ac/0x00a8cd0c/.../0x00a93f5c/0x00a9408c/0x00a941dc/0x00a942d4/0x00a9446c）。各 vtable の slot0 が固有のパス処理、slot1以降が共通スロット、という階層ファミリー構造。
- FUN_0080d110 への xref は 0x00a942d0（DATA, slot0）の1点のみ = パス記述子 vtable 経由の間接ディスパッチでしか使われない。param_5（パスオブジェクト）は実行時構築され vtable アドレスが動的にセットされるため、コードからの静的 xref は立たない。
- 意味: param_5 は「パス記述子オブジェクト」。その vtable の slot0 = FUN_0080d110 が dispatch 本体、+0x18 の Toggles / +0x158..+0x164 の4成分はこの記述子が保持する状態。シェーダー定数セッター（FUN_0076c910 等）と Toggles 2バンク書き込みはこの vtable ファミリー内の処理として位置づけられる。

## 19. Constant-setter call sites in FUN_0080d110 (partial, in progress)

Verified by disassembly (FUN_0080d110, case 0xe0/0xe1 blocks around 0x0080d588 and 0x0080d6f6):

- Call pattern per site: PUSH 0 (flag), PUSH value, PUSH key, MOV ECX,[ESI], CALL 0x0076c730.
- Ghidra's decompiler parameter labels are wrong for this call. ECX is the table pointer, and the key is the last-pushed immediate.
- Keys seen (key:value, flag 0): 0x1c:0, 0xa8:7, 0x1b:1, 0x13:5, 0x14:6, 0x07:1, 0x17:4, 0x0e:1.

Origin of ESI (confirmed by disassembly and decompile):
- 0x0080d1f4: CALL FUN_007b4280 returns DAT_00b42f54[idx] (idx from flag bits of the function argument).
- 0x0080d1f9: MOV EAX,[EAX+0x118], so the manager object is reached through that table.
- 0x0080d202-0x0080d209: PUSH ECX (out-pointer = ESP+0x18), MOV ECX,EAX, CALL FUN_00405ad0.
- FUN_00405ad0 (__thiscall, 1 stack arg): *out = this->[+0x100]; InterlockedIncrement on that object's refcount (+4). It returns out.
- So ESI = refcounted pointer stored at manager+0x100. The value is taken at 0x0080d22c, MOV ESI,EAX, after a TEST guard.
- At 0x0080d231-0x0080d248 the code releases the temporary (InterlockedDecrement via [0x00a2807c]; if zero, vtable slot 0 called with arg 1).
- The later `MOV ECX,[ESI]` before each setter call reads the object's first dword (table pointer).
- CALL 0x008aa390 is a Vector3 compare, not the producer.

ID to CTAB mapping (status: NOT established):
- FUN_007727d0 (key lookup) has a single caller, FUN_00772cd0 (insert/update). The keys are stored in linked-list nodes and are not read by any name table here.
- The reader that translates a key ID to a shader constant (CTAB name) has not been located. The tmp/ctab_map.txt names are register-level hypotheses only and are not linked to these IDs.

Name-registration side (found this session):
- FUN_007db4b0 registers SunDir=2, SunColor=3, EyePosition=1, FogParam=9, FogColor=10 (this+0x2c / this+0x30 tables).
- FUN_007cc280 registers, per table, name -> fixed index: ShadowProj=0x1c, DiffuseColor0=0x13, DiffuseColor1=0x14, "FogParam | ShadowVolumeFatness"=0x17, Toggles=0x07, BoneMatrix3=0x0e (partial list).
- Candidate matches with key IDs seen in FUN_0080d110: 0x1c, 0x13, 0x14, 0x17, 0x07, 0x0e. Unmatched so far: 0x1b, 0xa8.
- Caveat: indices are per-table. The match is a hypothesis until FUN_00772cd0's value/bank semantics are checked.

Key-to-CTAB name mapping: PARTIAL (hypothesis). The names in tmp/ctab_map.txt are register-level hypotheses only.

Store layout (from disassembly of FUN_00772cd0; the move helpers are not yet identified):
- ECX = table object (`this`). Stack args: [ESP+0x14] = key ID, [ESP+0x18] = value, [ESP+0x1c] = bank flag (byte).
- FUN_007727d0(table, key, &flag) searches list A (table+0x8) first, then list B (table+0x10). The out flag is 0 for list A and 1 for list B.
- On hit: writes value to node+0x4. If the stored flag differs from the new flag, the node moves to the other list (the move code path is not yet verified).
- On miss: a node is taken from the pool at DAT_00b427ac and linked into list A or B by the flag.
- Decompiler labels for FUN_00772cd0 are misleading (the `param_1 & 0xffffff` line is an artifact). Trust the disassembly.
- Still open: the reader of the key IDs (the code that walks lists A/B and reads node+0x4), and the verified mapping of IDs 0x1b and 0xa8.
- Reader search, step 1: FUN_007727d0 has a single caller, FUN_00772cd0 (0x772ce7). So no other code path uses this lookup. The reader must either walk lists A/B itself or go through FUN_00772cd0 with a different entry point. This is not yet resolved.
- Reader search, step 2: FUN_0052efa0 (key 0x1c, list at this+0x28, value at node+4) is a real key-match reader, but for a different container. Its only caller FUN_0052f650 does not read constant values. It writes an index into the element array at +0x20 (short). So it is not the constant-table reader. FUN_00536740 (key 0xa8) is ruled out as a scene-tree traversal.
- Reader search, step 3: FUN_0076c730 has 74 direct call sites in 9 callers (FUN_007d1320, FUN_0076d0a0, FUN_007bd3d0, FUN_008098e0, FUN_0080f280, FUN_007c9f30, FUN_008048e0, FUN_008067f0, FUN_0080d110). All are constant setters (writers). No reader is among them. Conclusion: the static xref graph shows only writes to the +0x30 table. The read path is either inlined list walking or a vtable call, so it cannot be found this way. Confirming it needs a dynamic trace.

## 20. MSVC RTTI vtable 全体スキャンと 0x4d9b40 共有 getter の同定（確定 2026-10-12）

### 20.1 方法
- headless で全イメージ非実行領域を 4 バイト刻みで走査し、各候補 base-4 が有効な RTTI CompleteObjectLocator（sig 0..1、TypeDescriptor 文字列が `.?AV`/`.?AU` で始まる）を指す vtable を全列挙するスクリプト `RttiIndex.java` を作成。
- 各 vtable について先頭 512 スロット内に共有 getter 0x004d9b40 が現れるか、現れるなら base からのオフセットも併記。

### 20.2 結果
- 検出 vtable 総数：**1715 個**（スキャン 398,599 語）。
- 0x004d9b40（`__thiscall` `return *(param_1 + 0x1c)`、Actor vtable +0x170）は **67 個**の vtable に共有されている。

**対象クラス vtable（+0x170 前後で 0x4d9b40 を持つ主なもの）**
| vtable base | クラス | getter offset |
|---|---|---|
| 00a46c44 | TESObjectREFR | +0x170 |
| 00a6e074 | Actor | +0x170 |
| 00a6fc9c | Character | +0x170 |
| 00a710f4 | Creature | +0x170 |
| 00a73a0c | PlayerCharacter | +0x170 |
| 00a732fc | MobileObject | +0x170 |
| 00a6f08c | ArrowProjectile | +0x170 |
| 00a34be4 | SpellItem | +0x28 |
| 00a34bf4 | SpellItem | +0x18 |
| 00a46c38 | TESObjectREFR | +0x17c |

### 20.3 判定
- 以前 RTTI で名前を付けられなかった 00a34be0 付近のテーブルは **SpellItem の vtable 領域**であることを確定（00a34c0c = 00a34be4 + 0x28、すなわち SpellItem vtable の +0x28 スロットが共有 getter 0x4d9b40）。
- つまり上記の getter は「オブジェクトの +0x1c 先頭 dword（装備魔法/呪文ハンドル等）を返す」共通アクセサで、TES 系オブジェクト階層（TESObjectREFR → MobileObject → Actor → Character/Creature/PlayerCharacter）と SpellItem、ArrowProjectile に広く共有されている。
- FUN_005ead00（装備魔法 +0x128 の AV 実値アクセサ）を調べる対象クラスは SpellItem 系と確定。

### 20.4 未解決（動的トレース待ち）
- これらの vtable の各スロット実装が実際にどのシェーダー定数/AV に対応するかは本 vtable スキャンでは決定せず、実行時到達が必要。

## 21. 新規ゲーム開始フローとキャラクタ生成（確定 2026-10-13）

タイトル画面「New」から導入ムービー・キャラクタ生成までの経路を、headless Ghidra（DecompOnly.java / FindScalar.java / DisasmRange.java）で追跡した。

### 21.1 タイトルメニューのボタン処理
MainMenu（メニュー ID 0x414）の生成は `FUN_005b6040`。ボタン押下は `FUN_005b64c0`（`__thiscall(param_1, buttonId, ...)`）の switch で分岐し、`Data\Menus\Options\main_menu.xml` の表示順（main_continue, main_new, main_load, main_options, main_credits, main_exit）と一致する。

| buttonId | XML 要素 | 遷移先 | 内容 |
|---|---|---|---|
| 2 | main_continue | `LAB_005b5d60` | 最新セーブ有無を判定し CONTINUE（`FUN_00465860`→`FUN_00459400`） |
| 3 | main_new | `FUN_005b5df0` | NEW GAME 開始 |
| 4 | main_load | `FUN_005aea60(0)` | LOAD メニュー |
| 5 | main_options | `FUN_005bd680` | OPTIONS |
| 6 | main_credits | `FUN_0059d680` | CREDITS |
| 7 | main_exit | `LAB_005b59b0` | 終了確認 |

### 21.2 New Game ハンドラ FUN_005b5df0（0x005b5df0）
```
FUN_00578d70()                 // オプション/前提フラグ（==1 で続行）
FUN_00589b70(0x414)            // MainMenu 取得
FUN_005898f0()                 // 現在メニュー
strtol(PTR_s_...1436c,0x10)    // INI の chargen クエスト FormID 文字列
FUN_0046b250(formId)           // FormID → TESQuest 解決（+4 == ';' = クエスト種別）
FUN_009832e6(...&PTR_PTR_00b05468...)   // キャラ生成クエストのインスタンス化
FUN_005b5960()                 // MainMenu を隠す（要素 0x1772 alpha=0）
DAT_00b3c0ec = 0
*(char*)(DAT_00b333c4 + 0x7f8) = 1      // 新規ゲームフラグ（PlayerCharacter シングルトン）
DAT_00b3a6d3 = 1
FUN_00529820(1)                // プレイヤーのフラグ bit0 を立てる
FUN_0060d0a0()
```
- クエストが見つからない場合は `"Unable to find character generation quest '%s'."`（0x00a6cb64）を表示。
- フラグ実体は `PlayerCharacter + 0x7f8`（= `param_1[0x1fe]`、1 バイト）。コンストラクタ `FUN_0066a740` が 0 で初期化する。`FUN_005b5df0` だけが 1 を書き込む（`FindScalar.java 0x7f8` の全走査で確認）。

### 21.3 ワールドロード FUN_0066eaf0 とフラグ消費
`FUN_0066eaf0`（`__thiscall`, 2170 バイト）はセル/ワールドのロード本体。末尾で新規ゲームフラグを消費する。
```
if (*(char*)(param_1 + 0x7f8) != 0) {
    FUN_005732d0(2, _DAT_00a3744c);        // シーン/クエスト開始
    FUN_00440af0(0,0,0);
    FUN_0057b7e0();                        // キャラクタ生成の開始
    *(char*)(param_1 + 0x7f8) = 0;
    FUN_00572ec0(2, 0);
    FUN_006a8d00(); FUN_006a9b40();
    FUN_00410ba0(PTR_s_OblivionIntro_bik_00b030a4,1,1,0,0,1,0);  // OblivionIntro.bik 再生
    FUN_006a8d50(); FUN_006a9c00();
    *(char*)(param_1 + 0x7f8) = 0;
}
```
- `FUN_00410ba0` = Bink ムービー再生、`DAT_00b030a4` = `"OblivionIntro.bik"`。Android 側の `TitleScreen::setupIntroVideo()`（`videos/oblivion_intro.mp4`）に対応。

### 21.4 キャラクタ生成の開始 FUN_0057b7e0（0x0057b7e0）
```
FUN_00678d90(DAT_00b333c4)      // プレイヤー初期化
FUN_00589b70(0x40c, ...&PTR_PTR_00b13348...)  // RaceSexMenu（0x40c）生成
FUN_005ca010(1)                 // 種族/性別メニュー生成（race_sex_menu.xml）
```

### 21.5 キャラ生成ウィザード（ステップマシン）
`FUN_005976b0` はキャラ生成メニューのコールバック（ディスパッチテーブル 0xa6b490 の +0x38）で、`param_1[0x16]`（ステップ番号）で分岐する。

| step | 処理 |
|---|---|
| 1 | `FUN_005d6390(param_1[0x14], 2)` |
| 2 | `FUN_005d6390(param_1[0x13], 1)` |
| 3 | `FUN_005d6390(param_1[0x12], 0)` |
| 4 | `FUN_005dcf20(...)`（確定処理） |
| 5 | `"%s %s %s?"`（sStartNewGame 等）を組み立て `FUN_00579c10(callback LAB_005974e0)` で確認ダイアログ表示 |

- `FUN_005d6390`（0x005d6390, 1238 バイト）= スキル/属性メニュー生成（`Data\Menus\CharGen\skills_menu.xml`）。引数 0/1/2 は画面モード。
- 関連 XML: `Data\Menus\CharGen\race_sex_menu.xml`（0x00a6973c）、`Data\Menus\CharGen\skills_menu.xml`（0x00a69718）、`Data\Menus\Chargen\class_menu.xml`（0x00a6b53c）。
- 関連文字列: `SetInChargen`（0x00a4ec0c）、`sCharGenQuest:General`（0x00a6cc40）、`GetIsPlayerBirthsign`（0x00a4ec90）、`ShowBirthSignMenu`（0x00a4ed14）、`sStartNewGame`（0x00a5ead0）、`sNewGame`（0x00a5eca4）。

### 21.6 メニューディスパッチャ FUN_00587550（参考）
メニュー ID → 生成関数の switch。0x3ff=MapMenu/`FUN_00579f80`、0x3ea=Inventory/`FUN_0057a2d0`、0x3eb=Stats/`FUN_0057a440`、0x3ec=HUDMainMenu/`FUN_005a6b00`、0x3f2=SleepWait/`FUN_005a8e30`、0x414=MainMenu/`FUN_005b6040`、0x40c=RaceSex（21.4）。

### 21.7 Android 移植への示唆
- Android の `title_screen.cpp::handleMenuSelection()` の New は現状スタブ（`gameStarted=true`）。本経路のフラグ（PlayerCharacter+0x7f8）とキャラ生成ウィザード（種族→性別→クラス→星座→確認）が未実装分。
- 導入ムービー `OblivionIntro.bik` は New Game 確定後に再生される順序（キャラ生成開始と同フレーム）。


## 22. Options / Load / Credits メニュー処理の詳細（確定 2026-10-14）

第 21 章で扱ったタイトル画面（MainMenu 0x414）以外のメニュー系処理を、headless Ghidra（`DecompTargets.java`）で
`Oblivion.exe`（Steam 版、image base 0x400000）を直接逆コンパイルして確定した。

### 22.1 共通のメニュー生成パターン

各メニュー生成ハンドラは同一の定型を持つ（Options / Load / Credits すべてで確認）。

```
1. FUN_00589b70(<menuId>)            // 既存メニュー取得。非 NULL なら vtable[0](1) で解放
2. FUN_00582160(0,1)                 // メニューシステム シングルトン取得（遅延生成）
3. thunk_FUN_00584980()              // 深度(time)再計算 → float 戻り値
4. FUN_00590420(<xml パス>)          // XML ロード
5. FUN_005898f0()                    // 現在メニュー取得。NULL なら失敗
6. vtable[0x34]() != <menuId>        // 種別不一致なら解放して失敗
7. FUN_009832e6(xml,0,&PTR_PTR_00b13240,&PTR_PTR_00b13a74,0)  // 共通ルート生成
8. FUN_00584880(...)                 // ルート登録（22.4）
9. FUN_009832e6(currentMenu,0,&PTR_PTR_00b0a018,&PTR_PTR_00b1xxxx,0) // メニュー固有生成
10. trait 0xfa5（stackingtype）判定 → trait 0xfab（depth）に time を設定
11. FUN_00585190(0)                  // メニューリセット
```

- `FUN_009832e6(xml,0,c1,c2,0)` = RTTI に基づく XML → C++ オブジェクトのインスタンス化（第 20 章の共有 getter 系）。
- trait 0xfa5（stackingtype）が `_DAT_00a69770` / `_DAT_00a69778`（スタック種別 enum 値）に一致するときだけ
  trait 0xfab（depth）へ時刻を書き込む。つまり「スタック型でないメニュー」は深度同期をスキップする。

### 22.2 各ハンドラの固有処理

| ハンドラ | メニュー ID | XML | 固有 vtable ペア | 固有処理 |
|---|---|---|---|---|
| FUN_005bd680 | 0x3f7 OptionsMenu | `Data\Menus\Options\options_menu.xml` | 0x00b143ec | 6 タイルフィールド検証、背面 PauseMenu(0x3f5) 探索 |
| FUN_005aea60 | 0x40e LoadMenu | `Data\Menus\Options\load_menu.xml` | 0x00b14178 | セーブ走査・スロット一覧生成、XBox セーブ変換 |
| FUN_0059d680 | 0x417 CreditsMenu | `Data\Menus\Options\credits_menu.xml` | 0x00b13fa8 | Credits.txt 解析でスクロール構築、アスペクト比補正 |

#### Options（FUN_005bd680）
- 生成後 `*(int *)(iVar5+0x28)`, `+0x2c`, `+0x30`, `+0x34`, `+0x38`, `+0x3c` の 6 フィールドがすべて非 NULL か検証。
  1 つでも NULL なら `"Options Menu Creation Failed... Are your menu and art resources up to date?"`（0x00a2xxxx）。
- 背面探索: メニューシステムのスタック配列（`+0xe0`、最大 9 要素）を `FUN_0057cfa0(i)` で走査し、
  0x3f7（自分）に到達する前に 0x3f5（PauseMenu）が見つかれば `bVar1=true`。
  最後に `FUN_0058ceb0(0xfae, bVar1+1)` で trait 0xfae（user0）= 1 または 2 を設定。
  これは「Options がポーズメニューから開かれたか（背面に PauseMenu があるか）」の判別値で、ESC の戻り先制御に使う。

#### Load（FUN_005aea60(param_1)）
- `param_1 != '\0'` のとき `FUN_0058ceb0(0xfa1, 0x3f800000)`（trait 0xfa1=visible を 1.0f）。タイトル画面からの場合は 0。
- `FUN_0045e6a0()`（22.5 XBox セーブ変換）の戻り値 `cStack_141`。
- MainMenu(0x414) が存在しない、または XBox 変換が発生した場合のみ `FUN_0045d450()`（22.6 セーブフォルダ再走査）。
- セーブリスト単体 `DAT_00b33b00`（セーブマネージャのシングルトン）+ `0x6c` = リスト先頭。
  この連結リストを辿って有効エントリ数を数え、0 件なら `FUN_005ae6d0(...,0,0,0)` で空エントリを 1 件生成。
- 各エントリを `FUN_005ae6d0(buf, index, saveObj, count)` で生成し、リスト先頭の生成結果を保持。
- 末尾で `FUN_0058fba0(0)`（フォーカス更新）→ `FUN_00585190(0)`（リセット）→
  `FUN_0057f9f0(firstEntry, 0xfdd, 0)`（先頭行を選択状態に）。メニュー再表示のたびにリストを全再構築する。

#### Credits（FUN_0059d680）
- `*(int *)(iVar5+0x28) == 0` なら `"Credits Menu Creation Failed... Are your menu and art resources up to date?"`。
- `DAT_00b3b290 != '\0'`（デバッグ/開発フラグ）のとき、`FUN_00589930("background")`（22.7 名前検索）で背景タイルを取得し
  `FUN_0058ceb0(0xfa7, 0)`（trait 0xfa7=alpha を 0 = 非表示）。さらに MainMenu の背景も `FUN_005b5a80()` で隠す。
- `FUN_0058ceb0(0xfae, (*(char *)(menuSys+8) != 1))` → trait 0xfae（user0）= 0 または 1（開発表示の有無）。
- `FUN_0059d030()`（22.3）で Credits.txt を解析しスクロール本文を構築。
- 画面アスペクト比 `DAT_00b06c50 / DAT_00b06c4c` が `_DAT_00a31c70`（4:3 = 1.3333）と異なる場合、
  スクロールルート（`*(int *)(*(int *)(iVar5+0x28)+0x24)`）の +0x54 に `_DAT_00a6b808` を加算し、
  `FUN_00707370(0,0)` を呼ぶ（ワイド画面でのスクロール基準位置補正とマウス解放）。

### 22.3 Credits.txt スクロール構築 FUN_0059d030（0x0059d030）

`FUN_00430970("Data\Credits.txt",0,0x2800,0)` でアーカイブから読み込み、内部マークアップを解析して
1 行ずつテキストタイル（`FUN_00575870` で生成）をスクロールルートに追加する。

| タグ | 書式 | 処理 |
|---|---|---|
| `<C ...>` | `<C x y w h>` | 4 つの atol 値 ÷ `_DAT_00a3ddd8`（=1000.0f 相当）で RGBA を +0x4c/+0x50/+0x54/+0x58 に設定 |
| `<F n>` | `<F1>`.. | フォントインデックスを +0x3c に設定（`FUN_00576a30()` のフォントテーブル参照） |
| `<I ...>` | `<I x y idx>` | 画像タイルを挿入（`FUN_0059cc00`）、Y を画像高さ分だけ進める |
| `<J C\|R>` | | 行揃え: `C`→+0x40=2（中央）、それ以外→+0x40=4（右） |
| `<X±n>` `<Xn>` | | テキスト X オフセット +0x44 を加減算/設定 |
| `<Y±n>` `<Yn>` | | テキスト Y オフセット +0x48 を加減算/設定 |
| `*` | | 行末まで（コメント）スキップ |
| `\r` | | タイル高さ `*(float*)(iVar4+0x2c)` を Y に反映 |

- 解析用一時バッファは `FUN_00401f00`/`FUN_00401f20` で確保/解放（`__security_check_cookie` 付き）。
- 完了後 `FUN_00707610` / `FUN_00707980` / `FUN_00707370(0,1)` でレイアウト確定とマウス再有効化。

### 22.4 ルート登録 FUN_00584880（0x00584880）

```
void FUN_00584880(int *param_1, int param_2) {
  param_1[1] = param_2;                       // 親/所有ポインタ保持
  if (param_2 == 0) return;
  if (trait(0xfa5) == 0x00a69778/00a69770/00a69768/00a69764 のいずれか) {   // スタック型
    param_1[5] = vtable[0x34]();              // メニュー ID 記録
    menuSys = FUN_00582160(0,1);
    FUN_0057d640(vtable[0x34]());             // メニュー ID → 内部スロット解決
    if (*(int **)(menuSys + 0x9c) != param_1) {
      if (*(int *)(menuSys + 0x98) != 0) FUN_0058ceb0(0xfdd, 0);  // mouseover 解除
      *(int *)(menuSys + 0x98) = 0;
      *(int *)(menuSys + 0x9c) = 0;
    }
  }
}
```
- スタック方針（0xfa5）を持たないメニューは登録のみで終わる。
- スタック型のときはメニューシステムの「現在アクティブメニュー」(+0x9c) を更新し、旧メニューの mouseover(0xfdd) を解除する。

### 22.5 XBox セーブ変換 FUN_0045e6a0（0x0045e6a0）

`XBoxSaves\*.*` を `FindFirstFileA`/`FindNextFileA` で列挙し、各ファイルを `FUN_00430970`（アーカイブ開）＋展開で
`"%s%s%s.ess"` に書き出し、変換後に `DeleteFileA` で元ファイルを削除する。bool を返す。
Load メニュー表示時に毎回呼ばれ、XBox 版セーブの移行（1 回限り）を担う。

### 22.6 セーブフォルダ走査 FUN_0045d450（0x0045d450）

```
path = DAT_00b3f280 (セーブフォルダ) + PTR_DAT_00b05564 (区切り) + "*.ess"
FindFirstFileA(path)
  各ファイル（サイズ 0 以外）: _sprintf("%s%s%s", dir, sep, cFileName)
     obj = FUN_00401f00(0x160); FUN_00430970(path, 0, 0x20000, 0);
     obj->vtable = &PTR_FUN_00a39094; obj[0x55] = 0;
     FUN_00416650(obj, &LAB_00459450)          // リストへ挿入
FindClose
```
- this の `+0x6c` に連結リストの先頭を格納。既存リストは `FUN_00459400` で破棄してから作り直す。
- 各エントリは 0x160 バイトの `TESFile`（アーカイブ）派生オブジェクトで、`0x20000` バイトのバッファを持つ。

### 22.7 セーブスロット表示行生成 FUN_005ae6d0（0x005ae6d0）

`"save_game_template"`（`FUN_004028d0` で取得）と書式文字列 `DAT_00b38700`（`"%s %i - %s\n%s: %s"`）で
スロットテキストを組み立て、以下の trait を設定する。

| trait | 値 |
|---|---|
| 0xfa1 | visible = 1.0f |
| 0xfde | スロット種別 |
| 0xfae | user0 = param_3（スロット番号） |
| 0xfb1 | 組み立てた表示文字列 |
| 0xfa8 | listindex = param_3 + 0x65（= 101） |
| 0xff0 | リスト項目マーカー |

- 文字列中の空白をアンダースコアに置換し、`param_1+0x5c` と `__stricmp` で突き合わせる（既存スロット判定）。
- `param_3 == 0` のとき `_DAT_00b3b3f0` をクリアする（先頭スロット＝最新セーブ）。

### 22.8 深度再計算 FUN_00584980（0x00584980）

メニューシステムの子メニュー一覧（`FUN_00582160(0,1)` の `+0x68`、要素 `+0x34` にメニュー ID）を走査し、
HUD 系（0x400 / 0x3ed=HUDInfoMenu / 0x3f2=SleepWait）を除外して
`max(menu+0x18 + trait(0xfab))` を求める。その値に epsilon（`_DAT_00a309f0`）を加えて trait 0xfab（depth）に書き戻し、
スクロール同伴値（`+0x58`）を `val * _DAT_00a68fd0` で更新する。0x3f2（SleepWait）だけは `_DAT_00a3f3e8` でオフセット。
戻り値は `float10`（描画順の基準時刻）。

### 22.9 名前検索 FUN_00589930（0x00589930）

```
int FUN_00589930(this, char *name)      // ツリー再帰
  for each child in this->+0x34:
    if (__mbscmp(child->+0x08, name) == 0) return child;
    r = FUN_00589930(child, name); if (r) return r;
  return 0;
```
- メニューツリーをたどり、XML の要素名（例 `"background"`）からタイルオブジェクトを解決する。

### 22.10 trait アクセサ（確定）

| 関数 | 役割 |
|---|---|
| FUN_0058ceb0(token, value) | 現在メニューに trait 値を設定。内部で `FUN_0058b220(token)`（trait 記述子解決）→ `FUN_0058ca00(value)`（値格納）。既存レポート §3.2 の setter。 |
| FUN_00588bd0(this, token)  | `this+0x18` の trait 連結リストを走査し `token` 一致の float 値を返す。Ghidra では `this` が省略表示される。第 3 章の 0xfab/0xfa5 読み出しは本関数。 |

### 22.11 メニュー関連補助関数一覧（追記）

| アドレス | 関数 | 役割 |
|---|---|---|
| 0x0057cfa0 | FUN_0057cfa0 | `*(menu+0xe0+index*4)` = メニュースタック配列要素取得 |
| 0x0057f9f0 | FUN_0057f9f0 | リスト行選択/マウスオーバー状態機械（trait 0xff0/0xfdd/0xfa8/0xfc9/0xfe1-0xfe5） |
| 0x0058fba0 | FUN_0058fba0 | メニューフォーカス/入力更新（stackingtype 0xfa5 を参照） |
| 0x00585190 | FUN_00585190 | メニューリセット（visible 復帰・tooltip クリア・深度再計算） |
| 0x00582160 | FUN_00582160 | メニューシステム シングルトン取得（`DAT_00b3a6e0`、遅延生成） |
| 0x0058b220 | FUN_0058b220 | trait ID → trait 記述子解決 |
| 0x0058ca00 | FUN_0058ca00 | trait 記述子へ float 値を格納 |

### 22.12 シングルトン一覧（確定）

| シンボル | 内容 |
|---|---|
| DAT_00b3a6e0 | メニューシステム（`FUN_00582160` で遅延生成、`FUN_00401f00(0x134)`） |
| DAT_00b33b00 | セーブマネージャ（+0x6c にセーブファイル連結リスト先頭） |
| DAT_00b333c4 | PlayerCharacter シングルトン（第 21 章、+0x7f8 新規ゲームフラグ） |

### 22.13 Android 移植への示唆

- `app\src\main\cpp\ui\title_screen.cpp::handleMenuSelection()` の Load / Options / Credits 分岐は
  本節の処理順（XML ロード → 固有生成 → trait 設定 → リスト生成 → リセット）に合わせる。
- Load はメニュー再表示のたびにセーブフォルダを再走査する設計（`FUN_0045d450`）。Android 側も
  「一度キャッシュして使い回す」ではなく、表示時にディレクトリを列挙する実装が原作挙動に一致する。
- Options の trait 0xfae（user0）= 1/2 は「背面に PauseMenu があるか」の戻り先制御値。
- Credits は `Data\Credits.txt` の独自マークアップ（`<C>/<F>/<I>/<J>/<X>/<Y>/ *`）を解析する。移植時は
  このマークアップパーサ（`FUN_0059d030` 相当）が必要。

---

## 23. メニュー固有「vtable ペア」の正体（MSVC RTTI TypeDescriptor）

本章は、各メニューハンドラが参照する「ペア」の実体を確定させる。
結論として、これらは **vtable ではなく MSVC RTTI の `TypeDescriptor` レコード**であり、
ハンドラは `dynamic_cast` の型識別子としてこれを 2 回使っていた。

### 23.1 各ハンドラの先頭で起きていること

Options ハンドラ `FUN_005bd680` の先頭付近（デコンパイル整形後）:

```c
uVar3 = FUN_009832e6(uVar3, 0, &PTR_PTR_00b13240 /* .?AVTile@@   */, &PTR_PTR_00b13a74 /* .?AVTileMenu@@ */, 0);
iVar5 = FUN_009832e6(piVar4, 0, &PTR_PTR_00b0a018 /* .?AVMenu@@   */, &PTR_PTR_00b143ec /* .?AVOptionsMenu@@ */, 0);
```

この 2 呼び出しが「ペア」の正体である。

1. **XML ルート Tile オブジェクト → TileMenu**（第 1 引数がロード済みタイル、第 4 引数が `.?AVTileMenu@@`）
2. **Menu 派生オブジェクト → 具象メニュークラス**（第 4 引数が `.?AVOptionsMenu@@` など）

### 23.2 FUN_009832e6 = MSVC `__RTDynamicCast`

`FUN_009832e6`（0x009832e6）の逆コンパイル結果は次のとおりで、MSVC CRT の
`__RTDynamicCast`（C++ `dynamic_cast` ランタイムヘルパ）そのものである。

- 引数: `(void* obj, int offset, RTTICompleteObjectLocator* srcCOL, TypeDescriptor* dstTD, int throwFlag)`
- 内部で `FindSITargetTypeInstance` / `FindMITargetTypeInstance` / `FindVITargetTypeInstance` を呼び分け、
  失敗時は `FUN_00983d6e("Bad dynamic_cast!")` → `__CxxThrowException_8` で例外送出。
- 成功時は `PMDtoOffset` でポインタを調整して返す。

したがって「ペア」とは **`dynamic_cast` の (変換元, 変換先) の組**であり、vtable の組ではない。

### 23.3 TypeDescriptor レコードの形式（確定）

各レコードは **0x1c バイト境界**で並ぶ次の構造:

| オフセット | 内容 |
|---|---|
| +0x00 | vftable ポインタ（`TypeDescriptor::vftable`） |
| +0x04 | 予約 dword（0） |
| +0x08 | マングル名 ASCII（`.\?AV<class>@@` 形式、NUL 終端） |
| +0x18 | パディング（テスト値 `0x00004040` が見えるのは隣接レコードの境界） |

`TypeDescDump.java`（新規スクリプト）で +0x08 の `.\?AV` を走査して全域を列挙した結果が次節。

### 23.4 メニュー RTTI TypeDescriptor 一覧（0x00b13240 - 0x00b147d8）

0x00b13240 以降にメニュー固有ディスクリプタが連続配置されている（`.rdata` 内）。

| アドレス | 型名 | アドレス | 型名 |
|---|---|---|---|
| 0x00b13240 | Tile | 0x00b13e44 | AudioMenu |
| 0x00b13254 | ContainerMenu | 0x00b13e5c | BreathMenu |
| 0x00b13270 | LoadingMenu | 0x00b13e78 | ClassMenu |
| 0x00b1328c | MessageMenu | 0x00b13f90 | MainMenu |
| 0x00b132a8 | StatsMenu | 0x00b13fa8 | CreditsMenu |
| 0x00b132c0 | HUDSubtitleMenu | 0x00b13fd0 | EffectSettingMenu |
| 0x00b132e0 | SleepWaitMenu | 0x00b13ff0 | EnchantmentMenu |
| 0x00b132fc | PauseMenu | 0x00b14010 | GameplayMenu |
| 0x00b13314 | LockPickMenu | 0x00b1402c | GenericMenu |
| 0x00b13330 | BookMenu | 0x00b14048 | HUDInfoMenu |
| 0x00b13348 | RaceSexMenu | 0x00b140c8 | HUDReticle |
| 0x00b13364 | MagicPopupMenu | 0x00b140e8 | InventoryMenu |
| 0x00b13384 | PersuasionMenu | 0x00b14104 | LevelUpMenu |
| 0x00b133a4 | SigilStoneMenu | 0x00b14178 | LoadgameMenu |
| 0x00b13ae4 | ControlsMenu | 0x00b14344 | MagicMenu |
| 0x00b13b5c | TextEditMenu | 0x00b14398 | MapMenu |
| 0x00b13ba8 | VideoMenu | 0x00b143b0 | NegotiateMenu |
| 0x00b13e28 | AlchemyMenu | 0x00b143cc | SpellPurchaseMenu |
| 0x00b143ec | OptionsMenu | 0x00b14744 | SaveMenu |
| 0x00b14408 | QuantityMenu | 0x00b1475c | SkillsMenu |
| 0x00b14424 | QuickKeysMenu | 0x00b1477c | SpellMakingMenu |
| 0x00b14504 | RechargeMenu | 0x00b1479c | TrainingMenu |
| 0x00b14728 | RepairMenu | 0x00b147b8 | VideoDisplayMenu |
| 0x00b13a74 | TileMenu | 0x00b14700 | RepairMenuList@RepairMenu |

同領域には Tile 系コンテナ（`NiTList<...>` / `NiTPointerMap<...>` / `NiTStringTemplateMap<...>` 等）の
ディスクリプタも混在する。`Menu` および `DialogMenu` / `HUDMainMenu` の基底寄りディスクリプタは
低番地側（0x00b0a018 = Menu, 0x00b0a02c = DialogMenu, 0x00b0a064 = HUDMainMenu）にあり、
メニュー本体ディスクリプタとは別クラスタを成す。

### 23.5 ハンドラ ⇔ 具象メニュークラス対応表（確定）

`FUN_009832e6` の第 4 引数（変換先 TypeDescriptor）を `RttiCastTargets.java`（新規スクリプト）で
全コードから機械的に抽出した結果。各行が 1 つの `dynamic_cast<クラス*>` に対応する。

| 具象クラス | TypeDescriptor | 使用ハンドラ |
|---|---|---|
| TileMenu | 0x00b13a74 | 全ハンドラ共通（Tile → TileMenu） |
| LoadingMenu | 0x00b13270 | FUN_00578e30, FUN_00578e90, FUN_005adcf0 |
| MessageMenu | 0x00b1328c | FUN_00579970, FUN_005bc8b0 |
| StatsMenu | 0x00b132a8 | FUN_0057a6f0 |
| ContainerMenu | 0x00b13254 | FUN_00599ee0, 0x00597b3a, 0x005bd4d6, 0x005c08d5 |
| HUDSubtitleMenu | 0x00b132c0 | FUN_0057acc0, FUN_0057add0, FUN_0057af10, FUN_0057afb0, FUN_0057b050, FUN_0057b0f0 |
| SleepWaitMenu | 0x00b132e0 | FUN_0057b420, FUN_005d6d20, FUN_005d7090 |
| PauseMenu | 0x00b132fc | FUN_0057b560, FUN_0057b600, FUN_005ae080, FUN_005bda90, 0x005bde9e |
| LockPickMenu | 0x00b13314 | FUN_0057b6a0, FUN_005af440, 0x005b0a2e |
| BookMenu | 0x00b13330 | FUN_0057b740, FUN_00586000, FUN_005962c0 |
| RaceSexMenu | 0x00b13348 | FUN_0057b7e0, FUN_005c9c70, FUN_005c9cd0, 0x005c2bb8, 0x005c9f88, 0x005c9fd8 |
| MagicPopupMenu | 0x00b13364 | FUN_0057bb20, FUN_0057bbf0, FUN_0057bcc0, FUN_005b3e50, FUN_005b3f00, FUN_005b4230, FUN_005b4a10, FUN_005b4e10 |
| SigilStoneMenu | 0x00b133a4 | FUN_005d1080, FUN_005d41e0, FUN_005d5200 |
| ControlsMenu | 0x00b13ae4 | FUN_0059b9e0, FUN_0059ba40, 0x0059b8ed |
| TextEditMenu | 0x00b13b5c | FUN_005dcf20 |
| VideoMenu | 0x00b13ba8 | FUN_005dde20, FUN_005deb80 |
| AlchemyMenu | 0x00b13e28 | FUN_00593810, FUN_005d0a20, FUN_005d1080, FUN_005d2070 |
| AudioMenu | 0x00b13e44 | FUN_00595380 |
| BreathMenu | 0x00b13e5c | FUN_00596470, FUN_00596550, FUN_005965c0 |
| ClassMenu | 0x00b13e78 | 0x00596a09, 0x005974f9, FUN_005d6390 |
| MainMenu | 0x00b13f90 | FUN_0059c890, FUN_0059c9f0, FUN_0059d680, FUN_005b6040 |
| CreditsMenu | 0x00b13fa8 | FUN_0059d680 |
| EffectSettingMenu | 0x00b13fd0 | FUN_005d5e50 |
| EnchantmentMenu | 0x00b13ff0 | FUN_005a1740, FUN_005a1ec0 |
| GameplayMenu | 0x00b14010 | FUN_005a3540 |
| GenericMenu | 0x00b1402c | FUN_005a3ff0 |
| HUDInfoMenu | 0x00b14048 | FUN_005a4840, FUN_00663070 |
| InventoryMenu | 0x00b140e8 | FUN_005ab5a0, 0x005c091a, 0x005c0966 |
| LevelUpMenu | 0x00b14104 | FUN_005ace20 |
| LoadgameMenu | 0x00b14178 | FUN_005aea60 |
| MagicMenu | 0x00b14344 | FUN_005b3760 |
| MapMenu | 0x00b14398 | FUN_005bb6c0 |
| NegotiateMenu | 0x00b143b0 | FUN_005bd080 |
| SpellPurchaseMenu | 0x00b143cc | 0x005bd50f, FUN_005d9590, 0x005d974f |
| **OptionsMenu** | **0x00b143ec** | **FUN_005bd680** |
| QuantityMenu | 0x00b14408 | FUN_005c05d0 |
| QuickKeysMenu | 0x00b14424 | FUN_005c1290 |
| RechargeMenu | 0x00b14504 | FUN_005ce840, FUN_005cf9db, FUN_005cfb50, FUN_005cfce0 |
| RepairMenu | 0x00b14728 | FUN_005d1e50, FUN_005d2070 |
| SaveMenu | 0x00b14744 | FUN_005d39a0, 0x005d33b5, 0x005d3b8d |
| SkillsMenu | 0x00b1475c | FUN_005d6390, 0x005d5a58 |
| TrainingMenu | 0x00b1479c | FUN_005dd4b0, 0x005dd40c |
| VideoDisplayMenu | 0x00b147b8 | 0x005ddb9e |
| DialogMenu | 0x00b0a02c | FUN_0057ab70, FUN_0057ac20, FUN_00583f40, FUN_005be270 |
| HUDMainMenu | 0x00b0a064 | FUN_0057a6f0, FUN_0057b190, FUN_0057b230, FUN_0057b2d0, FUN_005a5900, FUN_005a6040, FUN_005a64b0, FUN_005a6b00, FUN_00663070, FUN_00668070, FUN_0068ee90 |
| Menu（基底） | 0x00b0a018 | 全メニューハンドラ共通（121 箇所） |

補足:

- `PersuasionMenu`（0x00b13384）と `SpellMakingMenu`（0x00b1477c）には**直接の構成ハンドラが存在しない**。
  Persuasion は DialogMenu（0x00b0a02c）配下、SpellMaking は SpellPurchaseMenu 経由で開かれる。
- 1 ハンドラが複数クラスを cast するのは、親メニュー配下の子タイルをそれぞれ別クラスとして扱うため
  （例: FUN_0057a6f0 = StatsMenu と HUDMainMenu の両方を cast）。
- `.?AVMenu@@`（0x00b0a018）を第 3 引数に取る呼び出しは 121 箇所で、メニューシステム全体の共通プロローグ。
  同ディスクリプタは前掲 §22 のメニュー ID 体系とは独立した RTTI 上の基底型である。

### 23.6 Android 移植への示唆

- 原作のメニュー生成は「XML ルート Tile を取得 → TileMenu へ cast → 具象メニューへ cast」という
  2 段階の型確定を前提にしている。Android 側に対応する型階層は不要だが、
  **`<メニュー ID> → <具象クラス>` の対応表（§23.5）が原作の正典**として利用できる。
- 各メニューの初期化順序・固有処理は、§23.5 で得たハンドラ関数の逆コンパイルを起点に読むのが最短。
  たとえば Options の固有処理は `FUN_005bd680`、Load は `FUN_005aea60`、Credits は `FUN_0059d680`
  （§22 と一致）に集約されている。
- メニュー ID（0x3ed/0x3ee/0x3f2/0x3f5/0x3f7/0x40e/0x40f/0x414/0x417 など）と本節の
  クラス対応表を突き合わせることで、`title_screen.cpp` の分岐を原作準拠に拡張できる。

### 23.7 本章で新規追加した解析スクリプト

| スクリプト | 用途 |
|---|---|
| `RttiCastTargets.java` | ある TypeDescriptor を PUSH する全命令を走査し、近傍の別ディスクリプタとともに (呼び出し元関数, 変換先クラス) を出力 |
| `TypeDescDump.java` | 指定アドレス範囲を走査し `.\?AV` で始まる RTTI TypeDescriptor 名を列挙 |

---

## 24. Options メニューファミリーのレイアウト（XML 静的解析）

本章は Oblivion.exe のコード解析ではなく、原作が配布する **メニュー定義 XML** の静的解析結果である。抽出元は `Oblivion - Misc.bsa` 内の `menus\*.xml`（89 ファイル）。Ghidra は不要（§22 で扱ったハンドラ側コードと対になる「見た目・座標」の確定情報）。

### 24.1 対象ファイルと抽出結果

| ファイル | 行数 | メニュークラス | 背景サイズ (w×h) | 役割 |
|---|---|---|---|---|
| `options\options_menu.xml` | 213 | `OptionsMenu` | 240×360 (+40/+60) | Options トップ（6 / Xbox 7 ボタン） |
| `options\audio_menu.xml` | 539 | `AudioMenu` | 900×500 | 音量 5 スライダー |
| `options\video_menu.xml` | 2120 | `VideoMenu` | 900×600 | 描画設定 27 項目（スクロールリスト） |
| `options\video_display_menu.xml` | 130 | `VideoDisplayMenu` | 520×680 | 解像度リスト |
| `options\gameplay_menu.xml` | 490 | `GameplayMenu` | 790×528 | 難易度 + トグル 6 |
| `options\controls_menu.xml` | 873 | `ControlsMenu` | 666×580 | 2 ページ（感度 / キー割当） |
| `options\downloads_menu.xml` | 302 | `DownloadsMenu` | — | Xbox Live 限定（PC 版では非表示） |
| `options\xcontrols_menu.xml` | 570 | — | — | Xbox 用代替レイアウト |

抽出ツリー: `tmp\bsa_misc\menus\`（`chargen\ dialog\ generic\ main\ message\ options\ prefabs\` + ルート `*_menu.xml` + `strings.xml`）

### 24.2 共通骨格（全子画面で同一）

```xml
<menu name="AudioMenu">
  <class> &AudioMenu; </class>
  <stackingtype> &no_click_past; </stackingtype>
  <alpha> 0 </alpha>
  <locus> &true; </locus>
  <explorefade> 0.25 </explorefade>
  ...
  <xbuttonb> <ref src="audio_return_button" trait="clicked"/> </xbuttonb>
  <xbuttony> <ref src="audio_defaults_button" trait="clicked"/> </xbuttony>
</menu>
```

- `&no_click_past;` = 背後への入力を遮断。
- `alpha 0` = 開いた直後は透明でフェードイン。
- `explorefade 0.25` = 探索（非ポーズ）中のみのフェード量。
- `locus &true;` = フォーカス（キーボード操作）対象。
- 全画面共通のテキスト色は `rgb(117,59,33)`。

背景は全画面が `prefabs\generic_background.xml` を include して中央寄せする:

```
x = (screen().width  - me().width ) / 2
y = (screen().height - me().height) / 2
```

include 直後に `user0`（幅）/ `user1`（高さ）を上書きするのが、各画面がサイズを指定する唯一の方法。`generic_background.xml` は 4 枚を組み合わせる: `Menus\GenericBackground\{edge_corners, edge_horizontal, edge_vertical, center_background}.dds`。

下部ボタンは全画面が `prefabs\button_long.xml` を include する（177×44、depth 3）。位置は必ず **return ボタン基準**で決まる（Gameplay の実例）:

- Return: `x = parent.width - me.width - 30`, `y = parent.height - me.height - 20`
- Defaults: `x = return.x - me.width - 10`（Xenon は -40）, `y = return.y`
- `user0` = 表示文字列（`strings()._return` / `_defaults`）、`user20` = Xbox ボタン絵、`clicksound 2`。
- 幅・高さは `button_long.xml` 側で `177` / `44` 固定。

ボタン背景テクスチャは `button_long.xml` の `<filename>` が `me()._filename_`（`user20` 未設定時のフォールバック）を参照するため、Options 系 XML には明示されていない。抽出済みの 177×44 素材 `Menus\Container\cont_button_long_off.dds` / `_on.dds` が対応すると **推定**（コード側でユーザ20/ファイル名を注入している可能性がある）。

### 24.3 options_menu.xml（トップ）

```xml
<menu name="OptionsMenu">
  <class> &OptionsMenu; </class>
  <stackingtype> &no_click_past; </stackingtype>
  <alpha> 0 </alpha>
  <locus> &true; </locus>
  <explorefade> 0.25 </explorefade>
  <user0> ... </user0>   <!-- True if from pause menu -->
  <xbuttonb> <ref src="options_return_button" trait="clicked"/> </xbuttonb>
</menu>
```

- `user0` = ポーズメニュー経由で開いたか（真なら背景高 +40px、ポーズ表示を出す）。Xbox は更に +60。
- `_top_button_y = (background.height - return_button.height * (5 [+1 xbox])) / 2 + pause offset` → ボタン列全体を背景の縦中央に配置。
- `_button_rect_height = return_button.height`。
- `options_background`: `generic_background.xml` を include、`x`/`y` は画面中央、`user0 = 240`、`user1 = 360`（+40 pause / +60 xbox）。
- ポーズ表示:
  - `options_pause_indication_border_left` = `Menus\Shared\shared_decor_blue_left.dds`、35×17、depth 3、`x 30` `y 26`、`user0` のとき表示。
  - `options_pause_indication_border_right` = `shared_decor_blue_right.dds`、35×17、`x = parent.width - border_left.x - me.width`、`y 26`。
  - `options_pause_indication` (`<text>`) = `strings()._paused`、font 3、`rgb(51,79,142)`、alpha 255、depth 3、justify center、`x = parent.width/2`、`y 20`。
- ボタンは `button_floating_2.xml` を include して縦積み（`listindex` 順）:

| listindex | 名前 | 遷移先 | id / clicksound |
|---|---|---|---|
| 0 | `options_return_button` | 閉じる | 2 / 2 |
| 1 | `options_gameplay_button` | GameplayMenu | 5 / 1 |
| 2 | `options_video_button` | VideoMenu | 6 / 1 |
| 3 | `options_audio_button` | AudioMenu | 7 / 1 |
| 4 | `options_controls_button` | ControlsMenu | 8 / 1 |
| 5 | `options_downloads_button` | DownloadsMenu（Xbox のみ） | 9 / 1 |

- 各ボタン `_center_x = parent.width / 2`、`_top = _top_button_y + _button_rect_height * listindex`。
- ラベルは `strings()` の `_return` / `_gameplay` / `_video` / `_audio` / `_controls`。
- `button_floating_2.xml` のテクスチャは `Menus\Dialog\dialog_selection_full.dds` / `dialog_selection_cut.dds`。

### 24.4 audio_menu.xml

- `_y_buffer` = 40（項目間の縦間隔）。
- `_long_label_width` = 5 つのラベル幅の **最大値**（`copy` + `max` 連鎖）。全スライダーの左端をここに揃える。
- `_label_to_scrollbar_buffer` = 64。
- `audio_background` = 900×500（中央寄せ）。
- 5 行: `audio_master` / `audio_voice` / `audio_effect` / `audio_foot` / `audio_music`。各行は `*_label`(text) + `*_slider`(`horiz_floating_scroll.xml` include) + `*_slider_marker` + `*_slider_marker_2` で構成。
- スライダーは `user6` = マーカー id、`user8` = false（数値テキスト非表示）。`_full_width = me.width + horizontal_scroll_left.width + horizontal_scroll_right.width`。
- id 割り当て（原本直読で確定）: スライダー 1/3/5/7/9（master/voice/effect/foot/music）、スクロールマーカー `user6` 2/4/6/8/10、`audio_defaults_button` 11、`audio_return_button` 12。
- ボタン: `audio_defaults_button`(id 11) / `audio_return_button`(id 12)。`xbuttonb`/`xbuttony` が両者に束縛される。

### 24.5 gameplay_menu.xml

- `_y_buffer` = 24、`_label_to_scrollbar_buffer` = 64、`_long_label_width` は audio と同方式。
- `gameplay_background` = 790×528。
- 難易度: `gameplay_difficulty_label` + `gameplay_difficulty_slider`（`horiz_floating_scroll.xml`、id 1、depth 8、`repeathorizontal`）。
- トグル 6 個（`button_floating_2.xml`）: `general_subtitles` / `dialogue_subtitles` / `crosshair` / `rest_save` / `wait_save` / `travel_save`。
- ボタン: `gameplay_defaults_button`(id 9) / `gameplay_return_button`(id 10)。両者は `xup`/`xdown`/`xleft`/`xright` で相互にフォーカスを結ぶ。

### 24.6 video_menu.xml（スクロールリスト方式）

メニュー trait:

| trait | 値 | 意味 |
|---|---|---|
| `user0` | 27（Xbox 28） | 総項目数 |
| `user1` | 7 | 同時表示項目数 |
| `user2` | `video_main_rect.height / user1` | 1 項目の高さ |
| `user3` | — | 解像度フットノートの表示可否 |
| `user4` | コード設定 | 解像度テキスト |
| `user5` | コード設定 | アンチエイリアステキスト |

- `video_background` = 900×600。
- `_x_buffer` = `vertical_scroll_top.width`、`_y_buffer` = `video_return_button.height + 20`。
- `video_main_rect` (id 7, `target true`): `width = parent.width - 2*_x_buffer`、`height = parent.height - 2*_y_buffer`、`x = _x_buffer`、`y = 50`、`alpha 0`、`depth 3`、`user0 = me.width * 0.65`（第 2 列のセンタ X）。
- `video_vertical_scroll` (id 2) = `vert_floating_scroll.xml` include、`user1 0` / `user2 = VideoMenu.user0 - user8` / `user4 = user8 - 1` / `user5 0` / `user6 3` / `user8 = VideoMenu.user1`。
- **行カリング**: 各項目の `<visible>` は `me().y >= 0 && me().y + me().height <= parent().height` 相当で、画面外の行を描画しない。スクロールリストを成立させる要となるため、移植側でも必須。

項目一覧（スクロール順）:

| 順 | 名前 | 形式 |
|---|---|---|
| 1 | resolution | 左右矢印選択（`video_resolution_rect` + float_left/right）+ テキスト + フットノート |
| 2 | gamma | horizontal slider |
| 3 | texsize | toggle (`button_floating_2.xml`) |
| 4-9 | tree_lod / actor_lod / item_lod / object_lod / grass / viewdist | horizontal slider |
| 10-12 | LOD / distbuild / disttrees | toggle |
| 13-14 | intshad / extshad | horizontal slider |
| 15-18 | self_shadows / grass_shadows / canopy / shadow_filter | toggle |
| 19 | speclight | horizontal slider |
| 20-27 | hdr / bloom / water_detail / water_reflections / water_displacement / window_reflections / hair_detail / blood | toggle |
| 28 | anti-alias | 左右矢印選択（`video_antialias_rect`）+ テキスト（`user5` をコード設定） |

- スライダー系は `horiz_floating_scroll.xml`、トグル系は `button_floating_2.xml` を include。
- 矢印素材: `Menus\Shared\shared_arrow_left.dds` / `shared_arrow_right.dds`。
- ボタン: `video_defaults_button` / `video_return_button`（`button_long.xml`）。

### 24.7 video_display_menu.xml

`generic_background` を使わない独立画面（メニュー自身が `screen()/2 - background/2` で位置決め）。

- `video_display_background` (id 1, `target true`): `Menus\Shared\main_page_5.dds`、520×680、depth 1、`x 0` `y 0`。
- `video_display_text` (id 2): `strings()._resolutionlist`、justify left、`rgb(117,59,33)`、alpha 200、`wrapwidth = parent.width - 50`、`x 40` `y 60`。
- `video_first_text`〜`fifth_text` (id 4〜8): 静的固定文字列 `- 800 x 600` / `1080 x 607` / `1024 x 768` / `1280 x 720` / `1280 x 960`。各行の `y` は前行 + 60 / 40 / 40 / 40。
- `video_display_return_button` (id 3): `Menus\Container\cont_button_long_off.dds`、190×50、depth 31、`clicksound 2`、`x = parent.width - me.width / 2 + 150`、`y = video_fourth_text.y + 280`。
  - 子 `return_button_on`: `cont_button_long_on.dds`、176×51、`parent.alpha < 1` のとき表示。
  - 子 `return_button_text`: `strings()._return`、`rgb(117,59,33)`。

> 注: 解像度一覧は原作でも **固定 5 行**であり、実際の選択結果はコード側が保持する（Android では実機解像度をここへ流し込むことになる）。

### 24.8 controls_menu.xml（2 ページ + スクロールリスト）

メニュー trait:

| trait | 値 | 意味 |
|---|---|---|
| `user0` | 1 または 2 | ページ番号（これ自体がページ切替） |
| `user1` | `controls_main_rect.height / vertical_scroll.user8` | 1 行の高さ |
| `user2` | `strings()._move` | 左スティック見出し |
| `user3` | `strings()._look` | 右スティック見出し |
| `user4` | `&true;` / `&false;` | normal / selected モード |
| `user5` | コード設定 | デバイス名文字列 |

- `_button_buffer` = 50、`controls_background` = 666×580。
- **PAGE 1 (`controls_page_1`)**: sensitivity スライダー（`horiz_floating_scroll.xml`）、`invert_y` トグル、`swap_button`、左スティック / 右スティックのラベル + 設定値、`controls_return_button_2`。`user2`/`user3` が見出し文字列。
- **PAGE 2 (`controls_page_2`)**: デバイス切替（ハイライト + floating/left/right トグル + `effset_attribute_floating_end`）、`controls_device_text`（`user5` をコード設定）、`controls_main_rect` とその枠 5 枚（`shared_border_horizontal_1.dds` / `shared_border_vertical_2.dds`）、`controls_vertical_scroll` (id 2)、`controls_list_pane` (id 4)。
  - `controls_list_pane`: `clips true`、`height = childcount * ControlsMenu.user1`、`y = 0 - controls_vertical_scroll.user7 * ControlsMenu.user1`。**子はコードがテンプレートから生成**する。
  - `controls_vertical_scroll`: `user1 0` / `user2 = list_pane.childcount - user8` / `user3 1` / `user4 5` / `user5 0` / `user6 3` / `user8 6`。
  - 行テンプレート `controls_template_item`（フローティング背景）+ `controls_template_label` + `controls_template_button`。
- ボタン: `controls_return_button` (id 1, page 2 のみ)、`controls_more_button` (id 9, `_controls`, page 1 のみ)、`controls_defaults_button` (id 10, page 2 のみ)。

### 24.9 スクロールバー プレハブの trait 仕様（確定）

`vert_floating_scroll.xml`: 子 `vertical_scroll_top` / `vertical_scroll_bottom` / `vertical_scroll_line` / `vertical_scroll_marker`。テクスチャは `Menus\Dialog\dialog_scrollbar_up.dds` / `_down.dds` / `_line.dds` / `_marker.dds`。

| trait | 意味 |
|---|---|
| `user1` | min 値（通常 0） |
| `user2` | max 値（項目数 - 表示数） |
| `user3` | 矢印クリック時の進み |
| `user4` | バー本体クリック時の進み |
| `user5` | 初期位置 |
| `user6` | マーカー id |
| `user7` | 現在値（エンジンが更新） |
| `user8` | 同時表示数 |

`horiz_floating_scroll.xml`: 子 `horizontal_scroll_left` / `horizontal_scroll_right` / マーカー。テクスチャは `Menus\Shared\horizontal_scrollbar_line.dds` / `horizontal_scrollbar_parts.dds`。バー本体を「左端 + 線 + 右端」で伸縮させるため、`_full_width = me.width + left.width + right.width` という計算が各画面に現れる。

### 24.10 テクスチャ対応表（全て抽出済みを確認）

| 画面 | テクスチャ | 抽出ファイル |
|---|---|---|
| Options ポーズ表示 | `Menus\Shared\shared_decor_blue_left/right.dds` | `shared__shared_decor_blue_left/right.png` |
| 共通背景 | `Menus\GenericBackground\{edge_corners,edge_horizontal,edge_vertical,center_background}.dds` | `genericbackground__*.png` |
| トグルボタン | `Menus\Dialog\dialog_selection_full/cut.dds` | `dialog__dialog_selection_full/cut.png` |
| 長ボタン | `Menus\Container\cont_button_long_off/on.dds` | `container__cont_button_long_off/on.png`（+ `shared__shared_button_long_off/on.png`） |
| 解像度画面 | `Menus\Shared\main_page_5.dds` | `shared__main_page_5.png` |
| 左右矢印 | `Menus\Shared\shared_arrow_left/right.dds` | `shared__shared_arrow_left/right.png` |
| リスト枠 | `Menus\Shared\shared_border_horizontal_1.dds`, `shared_border_vertical_2.dds` | `shared__shared_border_horizontal_1.png`, `shared__shared_border_vertical_2.png` |
| 横スクロール | `Menus\Shared\horizontal_scrollbar_{left,line,marker,parts,right}.dds` | `shared__horizontal_scrollbar_*.png` |
| 縦スクロール | `Menus\Dialog\dialog_scrollbar_{up,down,line,line_cut,marker}.dds` | `dialog__dialog_scrollbar_*.png` |
| タブ / 空プレビュー | `shared_page_tabs.dds`, `shared_empty_large.dds` | `shared__shared_page_tabs.png`, `shared__shared_empty_large.png` |

### 24.11 Android 移植への示唆

1. **レイアウトは XML から機械的に復元できる**。`docs\MENU_XML_INTERPRETER_DESIGN.md` の M4（`MenuUiBuilder`）をそのまま Options ファミリーに適用するのが最短。座標式（`copy`/`sub`/`div`、`me()`/`parent()`/`screen()`）は既に評価器が実装済み。
2. **追加実装が必要な要素**（現設計では out-of-scope 扱い）:
   - リストスクロール UI（`vert_floating_scroll` の `user1..user8` と `list_pane` の `childcount` 連動）。
   - `<visible>` による行カリング（`video_menu.xml` の画面外行の非描画）。
   - テンプレートからの行生成（`controls_list_pane` は子を作る側がコード）。
3. `video_display_menu.xml` の解像度一覧は原作でも固定 5 行 + コード側の選択状態保持。Android では実機の解像度 / アンチエイリアス可否をコードから注入する設計に合わせる（原作も `user4` / `user5` をコード設定している）。
4. 現時点で `app\src\main\cpp\ui\title_screen.cpp` に Options 系メニューの参照は皆無 → **未移植**。`OptionsMenu`(0x3f7) の起動ハンドラは §22 で特定済み（`FUN_005bd680`）なので、移植時の入口はここ。
5. `downloads_menu.xml` / `xcontrols_menu.xml` は Xbox Live 専用のため PC 版移植対象外。ただし `xcontrols_menu.xml` は Xbox コントローラ対応時に参照価値がある。

---

## 25. セーブ・ロード メニューのレイアウト（XML 静的解析）

対象: `Menus\Options\save_menu.xml` / `load_menu.xml` と、両者が `include` する
`Menus\Prefabs\vertical_scroll.xml` / `focus_box.xml` / `button_long.xml` / `generic_background.xml`。
`save_menu` と `load_menu` はほぼ同一の骨格で、相違点は §25.4 に集約する。

### 25.1 対象ファイルと抽出結果

`Oblivion - Misc.bsa` から展開済みの XML（`tmp\bsa_misc\menus\...`）と、
`Oblivion - Textures - Compressed.bsa` から展開済みのテクスチャ PNG（`tmp\extracted_menus_full\menus\*.png`）を静的解析した。

- 抽出パイプラインは `tools\BSAFileExtractor\BSAFileExtractor.py` と `tmp\extract_menus_full.py`
  （`--tier menus|menus80|menus50`）。**3 段階（100%/80%/50%）すべて展開済み**であり、再抽出は不要。
- 以前「素材が 5 つ不足」と見えたのは、`tmp\bsa_textures_title` が
  **タイトル系アセットのみの部分抽出（29 ファイル）**だったためで、セーブ/ロードの素材は
  別フォルダ `tmp\extracted_menus_full\menus\` に最初から揃っている。誤認に注意。

### 25.2 共通骨格

両メニューともトップ属性はほぼ同一。

| 属性 | save_menu | load_menu |
|---|---|---|
| `name` | `SaveMenu` | `LoadMenu` |
| `class` | `&SaveMenu;` | `&LoadMenu;` |
| `stackingtype` | `no_click_past` | 同左 |
| alpha / explorefade / menufade | 0 / 0.25 / 0.25 | 同左 |
| `locus` | true | 同左 |
| `_item_height` | 82 | 82 |
| `user0` | true（既存セーブ有無 = ハイライト制御） | （無し） |
| `xbuttonb` | `save_button_return.clicked` | `load_return_button.clicked` |

背景は共通プレハブ `generic_background.xml`（`target` / `depth 1` / `locus`）。
save は `user0=1020` / `user1=614` の無地、load は `filename = Menus\Level_up\level_up_background.dds`（1020×614）。

### 25.3 save_menu.xml

`save_background` 配下の子（すべて親座標基準）。

| 要素 | id | 主な値 |
|---|---|---|
| `save_instructions` (text) | - | `font3` / `visible = not xbox` / `string = _savemenuinstructions` / `justify center` / `x = parent.width/2` / `y 10` / `alpha 255` / `rgb 117,59,33` |
| `save_lines` (image) | - | `Menus\Loading\loading_save_lines.dds` / `420×600` / `zoom 124` / `x = LoadMenu.x` / `y 18` |
| `save_list` (rect, clipwindow) | 5 | `locus` / `height = _item_height*6`（=492）/ `width = save_lines.width`（=420）/ `x 20` / `y 58` |
| `save_scroll_bar` (image) | 3 | `include vertical_scroll.xml` / `x = parent.width − me.width/2 + 10` / `y 20` / `height = parent.height − me.y` / `depth 20` |
| `save_list_pane` (rect) | 9 | `locus` / `width = parent.width` / `height = childcount*_item_height` / `x 0` / `y = (0 − save_scroll_bar.user7)*_item_height` / `clips` |
| `save_center_line` (image) | - | `loading_save_center_fold.dds` / `70×parent.height` / `zoom 124` / 中央 / `y 0` |
| `save_Picture_background` (image) | 6 | `loading_save_wide_frame.dds` / `458×262` / `zoom 116` / `x = parent.width/2+30` / `y 64` / `locus` |
| `save_Picture` (image) | 6 | `shared_empty_large.dds` / `width = bg.width−10` / `height = bg.height−10` / `zoom 177` / `x = bg.x+5` / `y = bg.y+5` |
| `save_Text` (text) | 7 | `justify left` / `rgb 117,59,33` / `font1` / `alpha 200` / `wrapwidth = parent.width−600` / `x = save_Picture.x` / `y = save_Picture.y + height + 20` |
| `save_delete_icon` (image) | - | `icon_xbox_x_large.dds` / `visible = xbox AND user0` / `64×64` / `x 618` |
| `save_delete_text` (text) | - | `string = _delete` / `visible = save_delete_icon.visible` |
| `save_button_return` (image) | 1 | `include button_long.xml` / `x = parent.width − me.width − 30` / `y = parent.height − me.height − 20` / `depth 4` / `user0 = _return` / `user20 = xbuttonb` / `userdefault = xboxhint` |

スクロールバーへの入力（save）：`user1 0` / `user2 = save_list_pane.child_count − me.user8` / `user3 1` / `user4 5` / `user5 0` / `user6 4`（marker id）/ `user8 6`（同時表示数）。`user7`（現在値）はエンジンが更新。

### 25.4 load_menu.xml（save との相違点）

`load_background` 以外の骨格は save と同一。相違は以下に限定される。

| 項目 | save_menu | load_menu |
|---|---|---|
| 背景 | 無地（1020×614） | `level_up_background.dds` |
| `load_scroll_bar` | `user1..user8` | **`user9 0` が追加**（手動ステップ） |
| `load_Picture` サイズ | `bg.width−10` × `bg.height−10` | `width = me.filewidth` × `bg.height−10` |
| `load_Picture` 縦位置 | なし | `cropy = max(0, 0 − (bg.height − me.user0)/256 * bg.height * 1.8)` / `zoom 176` / `x = bg.x+4` / `user0`（画像内有効高。**コードが設定**） |
| delete 表示条件 | `visible = xbox AND user0` | `visible = xbox` のみ |
| return ボタン y | `parent.height − me.height − 20` | **`parent.height − me.height/2 + 270`** |
| 行テンプレート y | `me.listindex*height` | `me.user0*height` |
| 行のカリング | `<visible> = me.y≥0 AND me.y+20 < parent.height` | 無し（カリング要素なし） |

`load_return_button` の y 式が save と異なる点は意図的な差（ロード画面はリスト下端の余白が異なる）と判断。Android 側は
XML 式をそのまま評価すれば再現できるため、個別のハードコードは不要。

### 25.5 スクロールバー プレハブ vertical_scroll.xml

`Menus\Shared\all_general_scrollbar.dds` をソースに、`cropx 32` / `width 20` / `height 300` で構成。`vert_floating_scroll`（§24.9）とは別物で、こちらは save/load 用の「汎用ストリップ」版。

| trait | 意味 |
|---|---|
| `user1` | min 値（通常 0） |
| `user2` | max 値（通常 100。save/load では `child_count − user8` をコード設定） |
| `user3` | 矢印クリック時の進み（1） |
| `user4` | バー本体クリック時の進み（10） |
| `user5` | 初期位置（0） |
| `user6` | マーカーのドラッグ id（-1 = なし。save/load は 4） |
| `user7` | 現在値（`vertical_scroll_marker._value_scrolled_to`、**エンジン更新。設定しない**） |
| `user8` | 同時表示数（5。save/load は 6） |
| `user9` | 手動ステップ（0） |
| `user10` | 予約 |

`_scrollable_range = max(user2 − user1, 0)`。子は `vertical_scroll_topside` / `vertical_scroll_bottomside` /
`vertical_scroll_top` / `vertical_scroll_bottom`（`cropy 98`, 20×32）/ `vertical_scroll_marker`
（`cropy 32`, 20×42, `y = (parent.height − me.height) * ((_value_scrolled_to − user1)/_scrollable_range)`）。
`visible = (user2 > 0)`。

### 25.6 focus_box プレハブ

`FocusBox`（`locus`）。`focus_center`（`focus_center_x4.dds`, `x/y −1`, `w = parent.width+2`, `h = parent.height+2`）を
中心に、上下左右のタイル（`focus_top/bottom/left/right`）と四隅（`focus_top_left/right`, `focus_bottom_left/right`）で枠を描く。
右下隅だけ子 `fade_right`（8×8）+ `fade_bottom`（8×8）を持つ。save/load では `save_FocusBox` / `save_FocusBox`（id 2）として
`user15 = parent.depth + 0.5` で深度挿入される。

### 25.7 スロット行テンプレートと実行時生成（§22.6/§22.7 連携）

`save_game_template` → `rect "SAVES"`（`save_menu.xml` 内）は行 1 つの雛形で、実行時にコード側が複製して
スロットごとの行を作る。雛形の要点：

- `alpha 0` / `user10 false` / `focusinset 8` / `locus` / `target` / `repeatvertical true` / `depth 5`
- カリング: `visible = (me.y ≥ 0 AND me.y + 20 < parent.height)`
- `height = _item_height`（=82）/ `width = parent.width − 15` / `y = listindex*height` / `clips`
- `user0 = listindex` / `user1 = "Save Game Image File Name"` / `user2 = "Save Game Name"` / `user3 = "SaveGameName"`
- `xscroll = listindex − ceil(user8/2) + 1`
- 子 `save_game_name`（text, `depth 1`, `string = parent.user3`, `rgb 117,59,33`, `alpha 255`, `wrapwidth 380`, `clips`, `x 6`, `y 10`）

この雛形に対する**実行時の値注入**は §22.7 `FUN_005ae6d0` が担う（表示文字列は `"%s %i - %s\n%s: %s"`、trait 0xfae=user0,
0xfa8=listindex=slot+0x65, 0xff0=リストマーカー）。セーブフォルダの列挙は §22.6 `FUN_0045d450`（`*.ess` 走査, 各エントリは
0x160 バイト `TESFile` 派生 + 0x20000 バイトバッファ）。したがって save/load 画面の移植は
「XML 雛形（本節）＋ 行生成コード（§22.7）＋ セーブ走査（§22.6）」の 3 点セットで初めて成立する。

load 側テンプレート `load_game_template` は y 起点が `me.user0*height` に変わる（save の `listindex` に相当）。
`LoadgameMenu`(0x00b14178) / `SaveMenu`(0x00b14744) の RTTI 対応は §23.5 を参照。

### 25.8 テクスチャ対応表（全て抽出済みを確認）

`tmp\extracted_menus_full\menus\` に存在（カッコ内は元解像度）。100/80/50 の 3 段すべてに存在するのは
`icons__icon_xbox_x_large` を除き共通。

| 用途 | 元 DDS | 抽出 PNG（解像度） |
|---|---|---|
| リスト枠 | `Menus\Loading\loading_save_lines.dds` | `loading__loading_save_lines.png`（512×512） |
| プレビュー枠 | `Menus\Loading\loading_save_wide_frame.dds` | `loading__loading_save_wide_frame.png`（512×256） |
| 中央区切り線 | `Menus\Loading\loading_save_center_fold.dds` | `loading__loading_save_center_fold.png`（64×512） |
| 通常枠 | `Menus\Loading\loading_save_normal_frame.dds` | `loading__loading_save_normal_frame.png`（512×512） |
| ロード背景 | `Menus\Level_up\level_up_background.dds` | `level_up__level_up_background.png`（1024×1024） |
| 空プレビュー | `Menus\Shared\shared_empty_large.dds` | `shared__shared_empty_large.png`（256×256） |
| 縦スクロール | `Menus\Shared\all_general_scrollbar.dds` | `shared__all_general_scrollbar.png`（64×128） |
| delete アイコン | `Menus\Icons\icon_xbox_x_large.dds` | `icons__icon_xbox_x_large.png`（64×64, 80/50 には無し） |
| フォーカス中心 | `Menus\Focus\focus_center_x4.dds` | `focus__focus_center_x4.png`（4×4） |

### 25.9 Android 移植への示唆

1. **レイアウトは XML から機械的に復元できる**（§24 と同じ結論）。座標式（`copy`/`sub`/`div`/`max`、`me()`/`parent()`）は
   既存評価器で評価可能。`cropy` と `me.filewidth` も式評価の対象に入れる必要がある。
2. **追加実装が必要な要素**: リストスクロール UI（`vertical_scroll.xml` の `user1..user9`）、
   `<visible>` 行カリング、テンプレート行生成（save/load の `repeatvertical`）、
   および行ごとの `xscroll = listindex − ceil(user8/2) + 1`。
3. **行生成コードの移植が必須**。§22.7 `FUN_005ae6d0` の表示文字列書式
   （`"%s %i - %s\n%s: %s"`）と trait 0xfae/0xfa8/0xff0 の意味を `save_menu` 雛形に適用する。
4. **セーブ走査は表示時に毎回**実行する（§22.6、`FUN_0045d450`）。キャッシュ再利用ではなく都度 `*.ess` 列挙。
5. `load_Picture` の `cropy` は画像内有効高（`user0`）をコードが設定する前提。Android では
   プレビュー画像から有効高を算出して注入する。
6. 現時点で `app\src\main\cpp\ui\` に save/load 画面の実装は無い（未移植）。入口は
   `SaveMenu`(0x00b14744) / `LoadgameMenu`(0x00b14178)、起動ハンドラは §23.5 の `FUN_005d39a0` 群 / `FUN_005aea60`。

---

## 26. video_menu（VideoMenu, 900×600）のレイアウト（XML 静的解析）

対象: `Data\Menus\Options\video_menu.xml`（71,739 バイト / 2,120 行、メニュー XML 中で最大）。
確定 2026-10-15。§24（Options ファミリー）の続き。値はすべて XML 名目座標（UI 空間）。

### 26.1 メニューメタ

| 項目 | 値 |
|---|---|
| menu name | `VideoMenu` |
| class | `&VideoMenu;` |
| stackingtype | `&no_click_past;` |
| alpha | 0（`me.alpha` は 0。実描画は子側で行う） |
| locus | `&true;` |
| explorefade / menufade | 0.25 |
| red / green / blue | 117 / 59 / 33 |
| user0（総項目数） | 27（非 Xbox）／ 1 + xbox |
| user1（同時表示数） | 7 |
| user2（行ピッチ） | `video_main_rect.height / user1` |
| user3 | 解像度脚注（`_widescreen`）の表示可否。コードが設定 |
| user4 | 解像度の表示文字列（コードが設定。`video_resolution_text` の string） |
| user5 | アンチエイリアスの表示文字列（コードが設定。`video_antialias_text` の string） |
| xbuttonb | → `video_return_button.clicked` |
| xbuttony | → `video_defaults_button.clicked` |

`strings()` から引く Trait 一覧（実在を確認済み）:
`_resolution` `_gamma` `_texturesize` `_treelod` `_actorlod` `_itemlod` `_objectlod` `_grass`
`_viewdist` `_distantlod` `_distbuild` `_disttrees` `_intshad` `_extshad` `_self_shadows`
`_grass_shadows` `_canopyshadows` `_shadowfiltering` `_speclight` `_hdr` `_bloom`
`_water_detail` `_water_reflections` `_water_displacement` `_window_reflections` `_blood`
`_antialiasing` `_hair_detail` `_onoff` `_defaults` `_return` `_widescreen`

注: `_alpha` / `_x_buffer` / `_y_buffer` / `_mouseover_any` も `trait=` で現れるが、これらは
strings() ではなくローカル trait（`me`/`parent` 等）への参照。

### 26.3 背景と main_rect の実寸

| 要素 | 値 |
|---|---|
| `video_background` | `generic_background.xml` を include（`_x_buffer = vertical_scroll_top.width`、`_y_buffer = video_return_button.height + 20`）。x = (screen.width − me.width)/2、y = (screen.height − me.height)/2 |
| 背景 user0 / user1 | 900 / 600（実効キャンバスは 900×600） |
| `_x_buffer` | 37（= `vertical_scroll_top.width`） |
| `_y_buffer` | 64（= `button_long.height` 44 + 20） |
| `video_main_rect` | id 7。**x = 37 / y = 50 / width = 900 − 37×2 = 826 / height = 600 − 64×2 = 472**。alpha 0、depth 3、locus true、`target`（ホイール受付） |
| `video_main_rect.user0` | `me.width × 0.65` = **536.9**（第 2 カラム＝スライダー／ボタンの中心 X） |
| `VideoMenu.user2`（行ピッチ） | 472 / 7 = **67.43** |

### 26.4 縦スクロールバー

`video_vertical_scroll` = `vert_floating_scroll.xml` include、image id 2。

| trait | 値 | 意味 |
|---|---|---|
| x | `parent.width` = 826 | main_rect 右端 |
| y | `vertical_scroll_top.height` = 50 | 上矢印の下端 |
| height | `parent.height − top.height − bottom.height` = 472 − 50 − 51 = **371** | レール長 |
| user1 | 0 | 最小値 |
| user2 | `VideoMenu.user0 − user8` = 27 − 7 = **20** | 最大値（＝総数 − 表示数。＝スクロール量の上限） |
| user4 | `user8 − 1` = **6** | ジャンプ距離 |
| user5 | 0 | 開始値 |
| user6 | 3 | マーカー id（ドラッグ有効化） |
| user8 | `VideoMenu.user1` = **7** | 同時表示数 |
| user7 | 実行時にコピーされる現在値。**実装側で設定してはならない** | |

スクロール可否は `user2 > 0` かつ `user10`（既定 true）で判定。

### 26.5 行リスト（27 行）

行 y は全行共通で

```
y = (<copy> 行番号 </copy> − video_vertical_scroll.user7) × VideoMenu.user2
```

行の可視条件（ラベル／ボタン／スライダー共通）:

```
visible = (me.y >= 0) AND (me.y + me.height <= parent.height) AND NOT xbox
```

| 行 | string trait | y の行番号 | コントロール | 種別 | id | マーカー id |
|---|---|---|---|---|---|---|
| 0 | `_resolution` | 0 | `video_resolution_rect`（矢印式） | range | rect 27 / left 8 / right 26 | - |
| 1 | `_gamma` | 1（xbox では 0） | `video_gamma_slider` | slider | 5 | 6 |
| 2 | `_texturesize` | 2 | `video_texsize_button` | button | 20 | - |
| 3 | `_treelod` | 3 | `video_tree_lod_slider` | slider | 12 | 13 |
| 4 | `_actorlod` | 4 | `video_actor_lod_slider` | slider | 14 | 15 |
| 5 | `_itemlod` | 5 | `video_item_lod_slider` | slider | 16 | 17 |
| 6 | `_objectlod` | 6 | `video_object_lod_slider` | slider | 18 | 19 |
| 7 | `_grass` | 7 | `video_grass_slider` | slider | 22 | 23 |
| 8 | `_viewdist` | 8 | `video_viewdist_slider` | slider | 28 | 29 |
| 9 | `_distantlod` | 9 | `video_LOD_button` | button | 9 | - |
| 10 | `_distbuild` | 10 | `video_distbuild_button` | button | 30 | - |
| 11 | `_disttrees` | 11 | `video_disttrees_button` | button | 31 | - |
| 12 | `_intshad` | 12 | `video_intshad_slider` | slider | 32 | 33 |
| 13 | `_extshad` | 13 | `video_extshad_slider` | slider | 34 | 35 |
| 14 | `_self_shadows` | 14 | `video_self_shadows_button` | button | 10 | - |
| 15 | `_grass_shadows` | 15 | `video_grass_shadows_button` | button | 21 | - |
| 16 | `_canopyshadows` | 16 | `video_canopy_button` | button | 11 | - |
| 17 | `_shadowfiltering` | 17 | `video_shadow_filter_button` | button | 48 | - |
| 18 | `_speclight` | 18 | `video_speclight_slider` | slider | 36 | 37 |
| 19 | `_hdr` | 19 | `video_hdr_button` | button | 38 | - |
| 20 | `_bloom` | 20 | `video_bloom_button` | button | 47 | - |
| 21 | `_water_detail` | 21 | `video_water_detail_button` | button | 39 | - |
| 22 | `_water_reflections` | 22 | `video_water_reflections_button` | button | 40 | - |
| 23 | `_water_displacement` | 23 | `video_water_displacement_button` | button | 41 | - |
| 24 | `_window_reflections` | 24 | `video_window_reflections_button` | button | 42 | - |
| 25 | `_blood` | 25 | `video_blood_button` | button | 43 | - |
| 26 | `_antialiasing` | 26 | `video_antialias_rect`（矢印式） | range | rect 44 / left 45 / right 46 | - |

- ラベルは `<text>`、`x = 0`、`alpha = 255`、色は VideoMenu の rgb（117/59/33）。
- ボタンの表示文字列は `strings():_onoff`（`button_floating_2` の `_top = ラベル.y − 12`）。
- 解像度・アンチエイリアスは `_onoff` ではなく現在値の文字列（`VideoMenu.user4` / `user5`）。
- 有効な行は 0〜26 の **27 行**。行番号 27 以降は存在しない（`user0 = 27` と一致）。

### 26.6 コントロールの共通ジオメトリ

**(a) ボタン（`button_floating_2.xml` include、`<rect>`）**

| trait | 値 |
|---|---|
| `_center_x` | `parent.user0` = 536.9 |
| `_top` | 対応ラベル.y − 12 |
| width | `child(button_text).width + 40` |
| height | 64（固定） |
| depth | 5 |
| x | `_center_x − child(button_floating).width / 2`（＝実質 user0 中心） |
| y | `_top` |
| `child(button_floating)` | `dialog_selection_full.dds`、幅 = max(parent.width, 104)、高 64、y = 3、parent の mouseover かつ target のときのみ可視 |
| `child(button_end)` | `dialog_selection_cut.dds`、104×64、`parent.width + 8 − me.width` |
| `child(button_text)` | font 1、justify center、depth 7、rgb = target ? (117,59,33) : (+128, alpha 196) |
| user1 | true = enabled |

**(b) スライダー（`horiz_floating_scroll.xml` を内部に include、`<image>`）**

スライダーは単体ではなく「マーカー画像（ハイライト枠）」の子として定義される:

| 要素 | 値 |
|---|---|
| マーカー画像 | `Menus\dialog\dialog_selection_full.dds`、`repeathorizontal`、depth 3 |
| マーカー width | `video_*_slider.width + 120` |
| マーカー height | 64 |
| マーカー x | `parent.user0 − me.width / 2`（＝ user0 中心） |
| マーカー y | 対応ラベル.y − 15 |
| マーカー子 `*_marker_2` | `dialog\dialog_selection_cut.dds`、104×64、x = `parent.width − 100` |
| スライダー x / y | 60 / 22（マーカー内相対） |
| スライダー width / height | 400（prefab 既定） / 16 |
| user4（ジャンプ距離） | 25 |
| user6（マーカー id） | 上表のとおり |
| user8 | `&false;`（数値表示は非表示） |
| レール素材 | `\Menus\Shared\horizontal_scrollbar_line.dds`、`repeathorizontal`、alpha = user13 ? 255 : 128 |
| 左矢印 | `horizontal_scrollbar_parts.dds` cropx 0、50×37、x = −49、y = −11 |
| 右矢印 | `horizontal_scrollbar_parts.dds` cropx 128、51×42、x = `parent.width − 1`、y = −11 |
| ノブ（マーカー image） | `horizontal_scrollbar_parts.dds` cropx 64、39×34、x = (parent.width − me.width) × 比率、y = −10、depth 6 |
| 左右の当たり判定 | `horizontal_scroll_leftside` / `rightside`（37 高、alpha 0、target = user13） |

左矢印→ n 段、右矢印→ n 段、サイドクリック→ user4 ジャンプ、ノブドラッグ→ `user8` 間隔。
`user13` は有効/無効、`user9` は 0〜1 の正規化現在値。

**(c) 範囲矢印（解像度 / アンチエイリアス）**

`video_resolution_left` / `_right`（および antialias 版）は `microsoft` 共通の左右矢印セット。

| trait | 値 |
|---|---|
| left / right 素材 | `\Menus\Shared\shared_arrow_left.dds` / `shared_arrow_right.dds`、121×36、depth 5 |
| left x | 288 |
| left y | `対応 text.y + text.height − me.height` / 2 |
| right x | `text.x + (text.x − left.x) − me.width` |
| 現在値テキスト | `justify center`、depth 1、x = `parent.user0`、y = `対応ラベル.y`、string = VideoMenu.user4 / user5 |
| rect（当たり判定） | `video_resolution_rect` は `video_resolution_floating` の x/y/width/height をコピー。`video_resolution_floating` = `dialog_selection_full.dds` 512×64、x = `left.x − 10`、y = 左矢印とテキストの縦中央合わせ |
| ハイライト端 | 子 `effset_range_floating_end` = `dialog_selection_cut.dds` 104×64、x = `parent.width + 8 − me.width` |
| 発光条件 | rect / left / right のいずれかが mouseover のとき alpha 255 |

### 26.7 フッター

`video_background` の直下、`video_main_rect` の外側に配置される。

| 要素 | 値 |
|---|---|
| `video_resolution_footnote` | font 3、`visible = VideoMenu.user3`、depth 3、x = 20、y = `parent.height − 40`（= 560）、string = `_widescreen`、色は VideoMenu rgb |
| `video_defaults_button` | `button_long.xml` include、id 4、depth 3。x = `video_return_button.x − me.width − (xenon ? 40 : 10)`、y = `video_return_button.y`、string = `_defaults`、clicksound 2、user20 = `xbuttony` |
| `video_return_button` | `button_long.xml` include、id 1、depth 3。x = `parent.width − me.width − 30`、y = `parent.height − me.height − 20`、string = `_return`、clicksound 2、user20 = `xbuttonb` |

`button_long.xml` の実寸: 177×44、font 既定 1、`user20 = 1` のときは通常ボタン
（`Menus\Shared\shared_button_long_off.dds` / `_on.dds`）、`user20 >= 2` のとき Xbox ボタン画像
（`Menus\Shared\all_xbox_buttons.dds` を cropx で切替、64×64）。

十字キーの巡回（非 Xbox）:

- return: left/right → defaults の mouseover、up → blood の mouseover
- defaults: left/right → return の mouseover、up → blood の mouseover
- resolution_rect: up → scroll 上端、down → gamma マーカー、left/right → 左右矢印
- antialias_rect: up → blood、down → scroll 下端、left/right → 左右矢印

### 26.8 hair_detail は無効化されている（重要）

`video_hair_detail_label` / `video_hair_detail_button` のブロックは
**コメントアウトされており、実行時には存在しない**。

```
1813: <!--text name="video_hair_detail_label">
1852: </rect-->
```

内部の行番号も `<copy> ?? </copy>`、id も `??` のままで、開発時のプレースホルダが
そのまま残っている。したがって **VideoMenu の行数は 27 行（0〜26）で確定**。
`_hair_detail` trait は strings 側に残っているが、この画面では参照されない
（HUD／他メニューでも未使用）。

### 26.9 Android 移植向け確定値（まとめ）

- 実効キャンバス 900×600。main_rect は **(37, 50) から 826×472**。
- 1 行あたりの送りは main_rect.height / 7 = **67.43**。表示は 7 行。
- ラベル列 X = 0（main_rect 左端）、コントロール列の中心 X = **536.9**（= 826 × 0.65）。
- ラベル（`<text>`）は x=0・可視判定は前述の culling 式のみ。
- ボタンは main_rect の X 中心に、ラベル y − 12 を上端として 64 高で描く。
- スライダーは幅 400・高 16。ハイライト枠はそれを中心に左右 60 ずつ広げた 520×64。
- 数値表示は `user8 = false` のため出さない。
- 色は常に (117, 59, 33)。非 target 時のみ +128 して alpha 196。
- Xbox 分岐では gamma 行だけ index が 0 に繰り上がる（他行は +1 ずれ込む）。
- 解像度とアンチエイリアスだけは矢印式（`shared_arrow_left/right.dds` 121×36）で、
  現在値はコード（`VideoMenu.user4` / `user5`）が設定する。

## 27. セーブスロットオブジェクト（0x160 構造体）と .ess セーブフォーマット（逆コンパイル）

> 既存 XML レイアウト（§25）に加え、save_slot_layout / save_slot_parse 相当を
> headless Ghidra（`D:\Cargo\ReaProj`、fresh import + createFunction + decompiler）で
> 逆コンパイルした結果。該当関数:
> - `save_slot_layout` @ 0x0045d450（スロット一覧構築）
> - `save_slot_obj_ctor` @ 0x00430970（0x160 確保）
> - `ess_file_load` @ 0x00585410 / `ess_query_time` @ 0x00590330
> - `data_reader_open` @ 0x0045f2e0 / `data_reader_size` @ 0x0045dbc0
> - `header_field_reader` @ 0x0045d5f0

### 27.1 セーブスロットオブジェクト（0x160 バイト）のレイアウト

スロット1個は 0x160 バイト。`save_slot_layout`(0x0045d450) が
`func_0x00401f00(0x160)` で確保し、リストに挿入する。

| オフセット | サイズ | 内容 |
|---|---|---|
| +0x00 | 4 | vtable `&UNK_00a39094` |
| +0x04 | 4 | 関数 ptr |
| +0x08 | 4 | 関数 ptr |
| +0x0c | 4 | ctor の param_4（種別フラグ） |
| +0x10..0x1c | 13B | 0 初期化 |
| +0x20 | 4 | param_3（0=load / 1=save） |
| +0x24 | 4 | bool |
| +0x28..0x38 | 0x11B | 初期化（+0x30 = -1 / 0xffffffff） |
| **+0x3c** | 0x104 | **name[260]**（strcpy、キャラ名） |
| +0x140..0x150 | 0x11B | 0 初期化 |
| +0x154 | 1 | クリアバイト（index 0x55 の byte 位置） |
| +0x15c | 4 | dword |
| **= 0x160** | | **1 スロット合計** |

- vtable `&UNK_00a39094`（0x90 0a 43 00 ...）＝ ctor の `&UNK_00a3636c` と
  **同一の関数エントリ列**（0x00430a90, 0x007481c0, 0x004307f0, 0x0042ff80,
  0x00430010, 0x00430ba0, 0x0042fe80, 0x00430240, ...）。両アドレスとも
  同じ vtable 実体を指す。
- ±パス構築: `lstrcpyA(0xb3f280 dir表記) + lstrcatA(0x00b05564 氏名) + &UNK_00a3a884`
  で `*.ess` を列挙 → `FindFirstFileA/FindNextFileA`。
- 表示名: `func_0x0098208b(sprintf)` に `&UNK_00a3a88c`(= "%s%s%s.ess") を渡す。
- `+0x20` が save なら `func_0x0042fe80(0, param_5)` を呼ぶ（セーブ専用フラグ）。

### 27.2 .ess ファイルのヘッダ（data_reader_size / header_field_reader）

`data_reader_size`(0x0045dbc0) はストリームコールバック `param_2[1]` で読み出す。

- **ヘッダマジック 12 バイト = `TES4SAVEGAME`（@0x00a38fa0）**。
  `func_0x00984166`(strncmp系)で先頭 4 バイト・12 バイトの両比較。
  一致しない場合エラーコード 0xd000 / 0xde0c 系。先頭 4 バイトが `CON ` に一致すると
  0xd000（デバイス名の誤検出）として弾く。
- version+1 バイトを `param_1+0x70`/`+0x71`、`*(byte*)(param_1+0x7c)` に保存。
  **セーブファイルバージョンはグローバル `iRam00b33b00 + 0x7c`**。
- version >= 0x52 (82): `+0x94` に 16 バイト、`+0xa4` に 4 バイト読み足す（GUID/追加ヘッダ）。
  version < 0x52: `+0x94..0xa4` を 0xffffffff 初期化。
- エラー時は vtable+0xc からハンドラ（0xd000 等）を呼ぶ。

`header_field_reader`(0x0045d5f0) はヘッダを **順次フィールド** として読む。

| 順 | version 条件 | サイズ | 内容 |
|---|---|---|---|
| 1 | > 0x37 | u32 | param_2（ver<0x38 は 0） |
| 2 | 常 | u8 長 + 文字列 | param_3（長さ接頭辞文字列、0 なら空） |
| 3 | >= 0x34 | u16 | param_4（ver<0x34 は 0） |
| 4 | 常 | u8 長 + 文字列 | param_5（キャラ名、長さ = bVar1） |
| 5 | 常 | u32 | param_6 |
| 6 | > 0x1c | u32 | param_8 |
| 7 | > 0x37 | 16B/4dword | param_7 tuple（ver<0x38 は func_0x004301a0 から代入） |
| 8 | 常 | 0x70 確保構造 | varray 構築（`iVar16*iVar5*3` バイト）、ver<0x2e で +0x24 をスキップ、memcpy、InterlockedIncrement |

- **バージョンゲート閾値**: 0x1c(28) / 0x2e(46) / 0x34(52) / 0x37(55) / 0x38(56) /
  0x51(81) / 0x52(82)。Steam 版は 0x50 代が想定。
- 読み終了後 `param_1+0x14 = 0`。

### 27.3 セーブスロットの表示/保存まわりで判明した文字列定数

| アドレス | 内容 |
|---|---|
| 0x00a3a884 | `*.ess`（検索パターン） |
| 0x00a3a88c | `%s%s%s.ess`（表示名 sprintf パス） |
| 0x00a3a8e8 | `Save game version is %i.%02i and current version is %i.%02i, so ...`（版数差診断フォーマット） |
| 0x00a3a8f8 付近 | `%02i:%02i:%02i`（時:分:秒、再生時間） |
| 0x00a3aeb8 | `%s %i`（名前+番号） |
| 0x00a3b144 | `quicksave` |
| 0x00a3aad4 | `autosave` / `.bak` |
| 0x00a3a954 | `CON `（デバイス名誤検出チェック） |
| 0x00b3f280 | dir 表記ルート（バッファ） |

### 27.4 注意：ess_file_obj_load(0x0058cf40) は .ess 本体パーサではない

`func_0x0058cf40` の実体は **スクリプト VM 的なオペコード分岐**
（opcode 0x14/0x19/.../0x3c = 20..60 の連番、`piVar4[4]` を 0xbba/0xfe6/0xfde/0xfa2
と比較）。生 .ess バイナリを直接読み切るのではなく、**保存済みゲーム状態の
オブジェクト復元** を目的とする。.ess 全体の完全バイナリマップはメニュー表示に
不要であり、メニューで必要なのは §27.2 のヘッダフィールド（氏名・レベル・
再生時間・状態）のみ。

### 27.5 Android 移植向け確定値

- スロットは **0x160 バイトの固定オブジェクト**、`name[]` は +0x3c から 260 文字。
- ヘッダ先頭のマジックは **`TES4SAVEGAME`（12 バイト）**。
- バージョン byte はヘッダ冒頭付近（+0x70/0x71、グローバル +0x7c）で、
  表示名の再構築には version ゲート（0x34 / 0x37 / 0x38 / 0x52）を尊重する。
- 再生時間は `%02i:%02i:%02i`、表示名は `%s %i` / `Save %i - %s - ...` 系で整形。

## 28. メニュー vtable 全列挙と未文書化メニューファミリー（逆コンパイル / RTTI、確定 2026-10-16）

### 28.1 手法

§20 の RTTI スキャンを基盤に `MenuVtables.java`（headless Ghidra、
`-process -noanalysis -postScript`）で非実行・初期化済みブロックを 4 バイト刻みで
走査し、MSVC RTTI の vtable レイアウトを検出した。

- 検出条件: 直前語（coldata ポインタ）→ `(int)readInt() & ~1 == 0`
  （RTTI complete-object `sig=0` / `sig=1`）、TypeDescriptor ポインタ（`colPtr+0xC`）
  → `colPtr+0x10` のクラス名が `.?AV` / `.?AU` で始まる。
- クラス名に `Menu` を含むものを抽出。
- **スロット数** = vtable 先頭からコード/データポインタが連続する個数（上限 512）。
  これが仮想関数テーブルのエントリ数（= デストラクタ + 仮想メソッド）。

結果: **実メニュークラス 50 件＋補助コンテナ 3 件＋基底 1 件 = 53 vtable**。

### 28.2 vtable スロット規則（全メニュー共通）

| スロット | 役割 | 備考 |
|---|---|---|
| [0] | デストラクタ | クラス固有、基底破棄呼出→`operator delete` |
| [1] | スカラ削除デストラクタ | 同上＋`param_2 & 1` 分岐 |
| [2] | 共通スタブ | ほぼ常に `FUN_0060cf60`（no-op/非対応既定） |
| [3] | 初期化 / コンストラクト | クラス固有（tile 構築・イベント登録） |
| [6][7] | 共通処理 | `FUN_00890b90` |
| [11] | メニュー固有ハンドラ | 未実装既定は `FUN_0060d0a0` |
| [12] | 共通処理 | `FUN_006fe080` |
| [13] | 共通デストラクタ系 | クラス固有の小関数 |
| [14] | 共通処理 | `FUN_00725da0`（一部はメニュー固有 15 番以降に拡張） |

**共通関数（複数メニューで共有）**: `FUN_0060cf60`（既定スタブ）/
`FUN_0060d0a0`（既定ハンドラ）/ `FUN_006fe080` / `FUN_00890b90` /
`FUN_006f7070` / `FUN_00725da0` / `FUN_00982d85`（補助コンテナの
デフォルト実装）。

**デストラクタの同定規則**（逆コンパイル所見）: `func_0x<基底破棄>()` 呼出後、
`if (param_2 & 1) func_0x00401f20(param_1)`（operator delete）を呼ぶ
MSVC スカラデストラクタの定型。例: 基底 `Menu::FUN_005853f0`。

### 28.3 実メニュー vtable 一覧（ベースアドレス / スロット数 / 主要スロット関数）

| # | クラス名 | ベース | slots | [0] dtor | [3] init | [11] handler |
|---|---|---|---|---|---|---|
| 1 | Menu（基底） | 00a696dc | 15 | FUN_005853f0 | FUN_0060cf60 | FUN_0060d0a0 |
| 2 | ControlsMenu | 00a69ff4 | 19 | FUN_00587520 | FUN_0059c5e0 | FUN_0059bd80 |
| 3 | TextEditMenu | 00a6a04c | 35 | FUN_00587ac0 | FUN_005dd1b0 | FUN_005dd160 |
| 4 | VideoMenu | 00a6a09c | 15 | FUN_00587cd0 | FUN_005df160 | FUN_005ddea0 |
| 5 | TileMenu | 00a6af14 | 7 | FUN_005900f0 | FUN_005900d0 | — |
| 6 | AlchemyMenu | 00a6b1b4 | 15 | FUN_005936f0 | FUN_00595080 | FUN_00593af0 |
| 7 | AudioMenu | 00a6b29c | 15 | FUN_00595350 | FUN_00595790 | FUN_00595a20 |
| 8 | BookMenu | 00a6b334 | 15 | FUN_00595c40 | FUN_00595fc0 | FUN_0060d0a0 |
| 9 | BreathMenu | 00a6b404 | 15 | FUN_00596520 | FUN_0060cf60 | FUN_0060d0a0 |
| 10 | ClassMenu | 00a6b49c | 15 | FUN_00596fc0 | FUN_00596fe0 | FUN_005976b0 |
| 11 | ContainerMenu | 00a6b574 | 19 | FUN_00597af0 | FUN_0059a400 | FUN_00598710 |
| 12 | CreditsMenu | 00a6b784 | 15 | FUN_0059c9d0 | FUN_0060cf60 | FUN_0059ca70 |
| 13 | DialogMenu | 00a6b88c | 15 | FUN_0059e660 | FUN_0059f010 | FUN_0059f640 |
| 14 | EffectSettingMenu | 00a6b92c | 15 | FUN_0059fc30 | FUN_0059ff60 | FUN_005a0260 |
| 15 | EnchantmentMenu | 00a6ba0c | 15 | FUN_005a1ea0 | FUN_005a3160 | FUN_005a20c0 |
| 16 | GameplayMenu | 00a6bd0c | 15 | FUN_005a3430 | FUN_005a3860 | FUN_0060d0a0 |
| 17 | GenericMenu | 00a6bd9c | 15 | FUN_005a3ce0 | FUN_005a3d80 | FUN_0060d0a0 |
| 18 | HUDInfoMenu | 00a6bdf4 | 15 | FUN_005a4810 | FUN_0060cf60 | FUN_005a5540 |
| 19 | HUDMainMenu | 00a6bee4 | 15 | FUN_005a6ae0 | FUN_005a68c0 | FUN_005a6de0 |
| 20 | HUDSubtitleMenu | 00a6c0dc | 15 | FUN_005a9960 | FUN_0060cf60 | FUN_005a93b0 |
| 21 | InventoryMenu | 00a6c18c | 15 | FUN_005a9bb0 | FUN_005abbc0 | FUN_005aa1b0 |
| 22 | LevelUpMenu | 00a6c264 | 16 | FUN_005ac5b0 | FUN_005ac7b0 | FUN_0060d0a0 |
| 23 | LoadingMenu | 00a6c40c | 15 | FUN_005ae060 | FUN_0060cf60 | FUN_0060d0a0 |
| 24 | LoadgameMenu | 00a6c6bc | 15 | FUN_005ae6b0 | FUN_005aeed0 | FUN_0060d0a0 |
| 25 | LockPickMenu | 00a6c73c | 15 | FUN_005af410 | FUN_0060cf60 | FUN_005b11a0 |
| 26 | MagicMenu | 00a6c974 | 15 | FUN_005b28b0 | FUN_005b3990 | FUN_005b1410 |
| 27 | MagicPopupMenu | 00a6ca54 | 15 | FUN_005b3ed0 | FUN_0060cf60 | FUN_005b4080 |
| 28 | MainMenu | 00a6cb1c | 15 | FUN_005b6020 | FUN_005b64c0 | FUN_0060d0a0 |
| 29 | MapMenu | 00a6cc9c | 15 | FUN_005b87b0 | FUN_005bb880 | FUN_005b70f0 |
| 30 | MessageMenu | 00a6ce5c | 15 | FUN_005bc690 | FUN_005bcdd0 | FUN_0060d0a0 |
| 31 | NegotiateMenu | 00a6cf0c | 15 | FUN_005bd050 | FUN_005bd490 | FUN_005bd550 |
| 32 | OptionsMenu | 00a6cfac | 15 | FUN_005bd650 | FUN_005bd880 | FUN_0060d0a0 |
| 33 | PauseMenu | 00a6d03c | 15 | FUN_005bda60 | FUN_005bde70 | FUN_0060d0a0 |
| 34 | PersuasionMenu | 00a6d0cc | 15 | FUN_005be240 | FUN_005c0440 | FUN_005bec70 |
| 35 | QuantityMenu | 00a6d15c | 15 | FUN_005c05a0 | FUN_005c0820 | FUN_0060d0a0 |
| 36 | QuickKeysMenu | 00a6d1f4 | 23 | FUN_005c0cf0 | FUN_0060cf60 | FUN_0060d0a0 |
| 37 | RaceSexMenu（chargen） | 00a6d38c | 15 | FUN_005c4e20 | FUN_005cc200 | FUN_005ce570 |
| 38 | RechargeMenu | 00a6d45c | 19 | FUN_005ce820 | FUN_005cfd90 | FUN_005ceb50 |
| 39 | RepairMenu | 00a6d5b4 | 15 | FUN_005d1060 | FUN_005d22c0 | FUN_005ceb50 |
| 40 | SaveMenu | 00a6d624 | 31 | FUN_005d3630 | FUN_005d3d20 | FUN_0060d0a0 |
| 41 | SigilStoneMenu | 00a6d664 | 15 | FUN_005d4880 | FUN_005d5470 | FUN_005d5520 |
| 42 | SkillsMenu | 00a6d71c | 15 | FUN_005d56f0 | FUN_005d5e50 | FUN_005d5ab0 |
| 43 | SleepWaitMenu | 00a6d7c4 | 15 | FUN_005d6d00 | FUN_005d6a80 | FUN_0060d0a0 |
| 44 | SpellMakingMenu | 00a6d864 | 15 | FUN_005d7c80 | FUN_005d7ca0 | FUN_005d8130 |
| 45 | SpellPurchaseMenu | 00a6d8cc | 15 | FUN_005d8950 | FUN_005d9890 | FUN_005ceb50 |
| 46 | StatsMenu | 00a6d9dc | 15 | FUN_005d9d70 | FUN_005dcd70 | FUN_005da180 |
| 47 | TrainingMenu | 00a6dc74 | 15 | FUN_005dd310 | FUN_005dd3e0 | FUN_005ceb50 |
| 48 | VideoDisplayMenu | 00a6dccc | 15 | FUN_005dda20 | FUN_005ddb70 | FUN_0060d0a0 |
| 49 | MenuBGShader | 00a8cfd4 | 47 | FUN_007b1de0 | FUN_006f7070 | FUN_0060d0a0 |

補助コンテナクラス（`NiTList` 系。実メニューではないが RTTI に `Menu` を含む）:

| クラス名 | ベース | slots | 備考 |
|---|---|---|---|
| `?$NiTListBase@V?$DFALL@UVideoRes@VideoMenu@@@@UVideoRes@VideoMenu` | 00a69fe0 | 3 | VideoMenu の VideoRes リスト |
| `?$NiTPointerListBase@V?$DFALL@UVideoRes@VideoMenu@@@@UVideoRes@VideoMenu` | 00a6a034 | 3 | 同上 |
| `?$NiTList@UVideoRes@VideoMenu` | 00a6a08c | 19 | 同上（MapMenu 系にも共有） |
| `RepairMenuList@RepairMenu` | 00a6d5a4 | 19 | RepairMenu の項目リスト |

### 28.4 スロット 15 以降を持つ拡張メニュー

slots>15 のメニューは、[15] 以降に固有の追加仮想関数（リスト操作・項目描画）を持つ。

- **TextEditMenu**(35) / **SaveMenu**(31) / **QuickKeysMenu**(23) /
  **ControlsMenu**(19) / **ContainerMenu**(19) / **RechargeMenu**(19) /
  **RepairMenuList**(19) / **LevelUpMenu**(16)。
- 例 SaveMenu: [15]=FUN_00ac2e2c, [16]=FUN_005d4880, [17]=FUN_005d3fb0,
  [19]=FUN_005d5470, [20]=FUN_005d4260, [21]=FUN_005d4400。SaveMenu と
  SigilStoneMenu は [0]/[3] 系を共有（FUN_005d4880/FUN_005d5470/FUN_005d4260/
  FUN_005d4400）し、**セーブ一覧の行オブジェクト基底を共有**している。

### 28.5 未文書化メニューファミリーの役割（逆コンパイル所見）

`DecompOnly.java` で主要関数を逆コンパイルした結果（§23.5 のハンドラ表を補強）:

- **基底 Menu**: `FUN_005853f0`（デストラクタ）。スロット [0]..[14] のうち大半が
  既定スタブ `FUN_0060cf60` で、実質的にイベントディスパッチ基盤のみを持つ。
- **GenericMenu**: [0]=`FUN_005a3ce0`（デストラクタ: `func_0x005a3c60` 呼出）、
  [3]=`FUN_005a3d80`（初期化）、[13]=`FUN_005a3cd0`。§23.5 の設定項目ハンドラ
  `FUN_005a3ff0` は ID `0xfae..0xfc2`（21 個）の設定スライダー/トグルを構築し、
  `func_0x00588bd0(設定ID)` で各設定値を読み、グローバル配列 `0xb3b2e0` に格納、
  `uRam00b3b2d8=*(param_1+0x34)` / `iRam00b3b2dc=param_2` を更新する。
  Gameplay/Video/Audio の各設定画面はこの GenericMenu のインスタンスとして生成される。
- **MessageMenu**: [0]=`FUN_005bc690`（デストラクタ: `func_0x005bc4a0`）、
  [1]=`FUN_005bc510`、[3]=`FUN_005bcdd0`、[4]=`FUN_005bc770`、[5]=`FUN_005bc890`。
  `FUN_005bcdd0(param_1,param_2)` は表示種別 `param_2` に応じて処理分岐し、
  `param_2==2`→`func_0x005bccb0`、`param_2>3`→`func_0x0057de50(1)`。メニュー取得後
  `+0x98..+0xa4` を 0 クリアし、`*(char*)(+0xb0) = *(char*)(param_1+0x60) + param_2 - 4`
  でメッセージ種別を算出する。`FUN_005bc8b0(char*)` は文字列引数の関連処理。
- **FUN_00579970**: メニュー `0xfa8` 系の更新処理。グローバル
  `iRam00b3a6d8` / `piRam00b3a6dc` で前回値を保持し、vtable
  `[0xc]`/`[0x10]`/`[0x14]`（set 系）を呼び出す。
- **HUDMainMenu**: [0]=`FUN_005a6ae0`、[1]=`FUN_005a55f0`、[3]=`FUN_005a68c0`、
  [8]=`FUN_005a6940`、[11]=`FUN_005a6de0`、[13]=`FUN_005a6840`。
  HUD 常駐メニュー（体力/魔力/コンパス等）の更新系。
- **DialogMenu**: [0]=`FUN_0059e660`、[1]=`FUN_0059d8f0`、[2]=`FUN_0059da30`
  （PersuasionMenu の [2] と共有）、[3]=`FUN_0059f010`、[4]=`FUN_0059db70`、
  [5]=`FUN_0059dca0`、[11]=`FUN_0059f640`。
- **RaceSexMenu（chargen）**: [0]=`FUN_005c4e20`、[2]=`FUN_005c2790`、
  [3]=`FUN_005cc200`（大規模キャラクタ生成初期化）、[10]=`FUN_005c2870`、
  [11]=`FUN_005ce570`、[12]=`FUN_005c3dd0`。新規ゲーム開始（§21）時に
  人種・性別・外見の選択 UI を構築する。
- **ClassMenu**: [0]=`FUN_00596fc0`、[3]=`FUN_00596fe0`、[11]=`FUN_005976b0`、
  [13]=`FUN_00596ce0`、[14]=`FUN_00596980`。chargen のクラス選択。

### 28.6 Android 移植向けメモ

- メニューは **共通基底 vtable（`FUN_0060cf60` 既定スタブ＋`FUN_0060d0a0` 既定
  ハンドラ）** と **クラス固有の [0]/[1]/[3] と必要な追加スロット** で構成される。
  Android 側の UI 実装では、共通スロット（イベントディスパッチ）は 1 系統に統一し、
  クラス固有スロットのみ各画面で実装すればよい。
- **行オブジェクト系メニュー**（SaveMenu/SigilStoneMenu/RepairMenu 等）は
  リスト項目の仮想関数を共有するため、行ウィジェット基底を 1 つ実装して
  Save/Load/Repair/Recharge で再利用できる。
- XML レイアウト（§24〜26）と vtable（本章）を突き合わせることで、
  各メニューの「表示要素（XML）」と「更新ロジック（vtable ハンドラ）」が対応する。

## 29. 残メニュー関数 6 件の詳細逆コンパイル（確定 2026-10-16）

§28.5 で役割のみ記載していた 6 関数を、`DecompOnly.java` で実際に逆コンパイルした。

### 29.1 手法と注意（`size=1` は誤検知だった）

- 対象プロジェクトは **`D:\Cargo\ReaProj` Fresh**（Oblivion.exe 解析済み）。
  `-process -noanalysis -postScript DecompOnly.java <addr...>` で実行。
- 出力の `size=1` は「プロジェクトが関数ボディを持たない」ことを示すだけで、
  **デコンパイラが生バイトからオンデマンド逆アセンブルして C を生成**する。
  既知関数 `0x005853f0` でも `size=1` のまま正常に本文が出ることを確認済み。
- 以前「本体なし」と判定したのは、**ログ整形フィルタ**が Ghidra の
  `INFO  DecompOnly.java> ` プレフィックス付き行（各 println の先頭行のみ）を
  残し、複数行にまたがる C 本文を落としていたため。生ログには全関数の本文が
  含まれる（`tmp\fam_out3.txt`、2713 行）。
- 結論: **残 6 関数はすべて当該プログラムに実在し、正常に逆コンパイル可能**。
  関数境界の欠落や別アドレスの問題はなかった。

### 29.2 HUDMainMenu

- **[3] init `FUN_005a68c0`**: `func_0x00589b70(0x3ec)`（メニュー 0x3ec の取得）→
  `func_0x005898f0` → `func_0x00589390(0xfb1)` → `func_0x00588bd0()` の値が
  `_UNK_00a2faa8` と一致するとき、スタック上のスロット値 9/10/0xb/0xc に応じて
  `FUN_005a5e80` / `005a5ef0` / `005a5f60` / `005a5fd0` を呼び分ける。
  HUD 常駐要素のサブ初期化（体力/魔力/コンパス等の個別スロット）。
- **[11] handler `FUN_005a6de0`**: HUD の毎フレーム更新（重量級）。
  `func_0x006ecc80`（HUD 有効判定）後、`this+0x54`/`+0x4c`/`+0x44`（サブ要素）の
  有無で分岐し、`*piRam00b333c4`（PlayerCharacter vtable）の `+0x174`/`+0x1e0`/
  `+0x198`/`+0x19d` を呼んで座標・向き・名前を取得。取得値は
  `func_0x0058ceb0(id, value)` で HUD 要素へ設定する（ID `0xfa1`/`0xfa7`/`0xfab`/
  `0xfae`/`0xfb0`/`0xfb3`/`0xfb4`/`0xfb7`/`0xfb8`/`0xfb9`/`0xfba` =
  テキスト・座標・スケール・アルファ・可視フラグ）。
  `piRam00b333c4[0x18e]` はコンパス/クエストマーカー、`piRam00b333c4[399]` は
  サブメニュー参照。`this+0x84`（登録要素数）/`this+0x90`（アクティブ数）ループで
  `*(this+0x78)+4`（vtable スロット 1）を呼び、要素をフェードアウト除去する。
- **[8] `FUN_005a6940` / [13] `FUN_005a6840`** は更新系の補助（§28.3 参照）。

### 29.3 DialogMenu

- **[3] init `FUN_0059f010(int* this, int id)`**: ボタン ID `id` ごとに分岐。
  - `id > 99`（数値入力・テキスト文字列 ID）: `func_0x00588c10(0xfde)` で
    テキストボックス取得、`func_0x0058ceb0(0xfa1, 1.0)`、`func_0x006b8660`
    （IME/文字入力状態）を処理。`func_0x006b8650` の結果から `this+0x22` と
    グローバル `uRam00b3b294` を更新し、`func_0x0059eb90`（ダイアログ再構築）へ。
  - `id == 4`: `func_0x005ef930` → 再構築 `func_0x0059eb90`。
  - `id == 7`: `0x3f1` 取得後 `func_0x00582160(0,1,0,0xfdd,0)`（メニュー 0xfdd を開く）、
    `func_0x0057f9f0`・`func_0x0058fba0(0)`・`func_0x0057de50(1)`（トランジション）、
    vtable `+0x14` を `(7, ...)` で呼出 → `func_0x005c01d0`。
  - `id == 8` / `9` / `0xd` / `0x10`: `func_0x0052ed50(5,0)` で選択対象を取得し、
    `func_0x0052f790(this[0x18], iRam00b333c4, ...)` で実行。グローバル
    `uRam00b131f8` に **1/2/3/4**（結果コード）を設定。後処理として
    `func_0x0057a8d0`（8）/ `func_0x0057a940`（9）/ `func_0x005cfce0`（0xd）/
    `func_0x005d9590`（0x10）を呼ぶ。
  - `id == 3` / `10`: `func_0x0057de50(2)`、`this[0x19] = 0`。
  → 「説得・賄賂・盗む・呪文・アイテム」等の各アクション ID ハンドラ。
- **[11] handler `FUN_0059f640(int this)`**: 戻り値はメニュー ID。
  `func_0x00578fe0()`（現在メニュー ID）が `0x3f1`/`0x3f0`/`0x40d`/`0x404`/`0x40b`/
  `0x419`（ダイアログ系・コンソール系）のいずれかなら処理をスキップ。
  `cRam00b3b298` フラグで `func_0x006b7220`/`func_0x006b7190`（テキスト入力の
  開始・終了）を制御。`this+0x60`（テキストボックス）の vtable `+0x304`・`+0x33c`
  を呼ぶ。`this+0x6c`（フェードタイマ、int、負値で逆方向）と `this+0x68`（経過 float）
  からグローバル `fRam00b13fcc`（0..1 のフェード係数）を算出する。

### 29.4 RaceSexMenu（chargen）

- **[3] init `FUN_005cc200(int this, int id)`**: 1586 行の大規模初期化。
  スタック上に多数の一時 UI/リスト項目オブジェクト（`apuStack_338` 等）を構築。
  - `func_0x00419b10`（93 回）＝リスト/配列への追加、`func_0x00588bd0`（71 回）
    ＝ゲーム設定値の読み出し、`func_0x005c3440`（28 回）/`func_0x005c3110`（24 回）
    ＝ RaceSex 固有のヘルパ（人種・顔・髪・目の項目生成）。
  - `func_0x0058ceb0(id, value)` で UI 要素値を設定、`func_0x00579c10`（4 回）
    で選択確定メッセージを生成。
  → 新規ゲーム開始（§21）時の**人種・性別・外見・髪型・目の選択 UI を構築**する。

### 29.5 ClassMenu（chargen）

- **[11] handler `FUN_005976b0(int* this)`**: `func_0x00578fe0()` が `0x41b`
  （ClassMenu のメニュー ID）と等しいとき `cRam00b3b274 = 1` を立てて戻る。
  それ以外では `cRam00b3b274` が立っていれば選択インデックス `this[0x16]` を
  増減し、`switch(this[0x16])` で分岐する。
  - `case 1..3`: `func_0x005d6390(this[0x14|0x13|0x12], 2|1|0)`（クラス属性の説明更新）。
  - `case 4`: `this[0x10]+0x1c`（クラス説明文字列、null 時 `&UNK_00a2f7ec`）を
    `func_0x005dcf20(uRam00b38658, text)` で設定。
  - `case 5`: `this+0x15 = 1` を立て、`func_0x00402e30` でクラス名文字列を整形し、
    `func_0x00579c10(&uStack_14, &UNK_005974e0, 1, ...)` で**クラス作成の確認
    メッセージ**を出す（`this[0xd]` の有無で既定名 `uRam00b38648` /
    `uRam00b38650` を選択）。
  → chargen のクラス選択と「クラス作成」確定処理。

### 29.6 メニュー ID 定数（本章で確定したもの）

| ID | メニュー |
|---|---|
| 0x3ec | HUDMainMenu の取得対象（§29.2） |
| 0x3f0 / 0x3f1 | DialogMenu（0x3f1 は表示中判定、§29.3） |
| 0x404 / 0x40b / 0x40d / 0x419 | ダイアログ・コンソール系（§29.3 のスキップ条件） |
| 0x41b | ClassMenu（§29.5） |
| 0xfdd | DialogMenu が開く対象メニュー（§29.3） |
| 0xfde | テキストボックス要素 ID（§29.3） |

### 29.7 Android 移植向けメモ

- HUDMainMenu は「**要素 ID → `func_0x0058ceb0(id, value)`**」の一方向設定で
  描画される。Android 側は同じ ID 表（`0xfa1`/`0xfa7`/`0xfab`/`0xfae`/`0xfb0`/
  `0xfb3`/`0xfb4`/`0xfb7`/`0xfb8`/`0xfb9`/`0xfba`）を HUD ウィジェットに
  マッピングすれば、描画ロジックを再現できる。
- DialogMenu / ClassMenu の init/handler は「**ボタン ID → アクション**」の
  スイッチであり、Android 側のボタンコールバックにそのまま対応させられる。
- chargen（RaceSex）は設定値（`func_0x00588bd0`）と項目生成ヘルパ
  （`func_0x005c3110`/`005c3440`）の 2 系統で構成されるため、Android 側も
  「データ表＋リスト生成」に分離すると移植しやすい。

---

# 30. 追加解析依頼（メニュー入力抑制 / 設定値の消費者 / 解像度 / フェード / 未文書化メニュー）

本章は main セッションからの追加解析依頼 5 件への回答である。対象は Steam 版
`Oblivion.exe`（imagebase `0x00400000`、sha256 `a8f313845c...b7dc6`）。各節末に
**Android 移植向け確定値**を置く。確定に至らなかった点は明示する。

## 30.1 依頼1: メニュー表示中のワールド入力（特にカメラ回転）の抑制

### 30.1.1 結論（要約）

オリジナルは「メニューが開いているか」を返す**単一の共通クエリ関数を持たない**。
抑制は次の 6 経路の**合成**で成立している。

1. グローバル フラグ `0x00b3a6d4`（メニュー表示中 = 1）
2. 入力動作モード配列 `0x00b12dc8`（有効 byte）/ `0x00b12dd0`（配列 base）/ `0x00b12dd4`（値）
3. 最前面メニューの入力消費フィールド（`[+0x24]`、値 `1`/`8` でブロック）
4. `LoadingMenu`（ID `0x3ef`）表示中の早期リターン
5. 毎フレームのワールド/カメラ更新 `FUN_0057f170` の `dt == 0` スキップ
6. 物理更新 `FUN_00679a70` の `0x00b3a6d4 != 0` スキップ

つまりカメラ回転が止まるのは「`dt == 0` でワールド/カメラ更新ごとスキップされ、
かつ物理更新もスキップされ、さらにモード配列でポーリング済み入力が握り潰される」
ためである。**移植側で `ACTION_MOVE` がカメラへ到達するのは、この合成のどれか
（特に dt ゲート）を実装していないため**と考えられる。

### 30.1.2 メインループ `FUN_00579cf0`（毎フレーム、219 insns）

```
[0x00b131fc] = 0xff
FUN_00572ec0(2, 1)                     ; 入力動作モードのセット/更新
FUN_00582160 x4                        ; MenuManager 取得
FUN_00588bd0(0xfae) の戻り値 == [0x00a379b4] (=2.0f)
    ? FUN_005bc8b0  : FUN_005bcc00     ; user0 == 2.0 で分岐（30.4.3）
メッセージポンプ / FUN_004046a0 / FUN_00583e60 / FUN_005821f0 / FUN_00583f40
if ( FUN_0040fda0() == false )         ; ポーズ判定（0x00b33434 終了コード 0x103）
    FUN_0057f170(0, 1, 0)              ; ワールド/カメラ更新（30.1.4）
```
→ **`FUN_0040fda0()` が真（ポーズ中）の間はワールド/カメラ更新自体が呼ばれない。**

### 30.1.3 入力動作モード別ゲート `FUN_00572ec0(mode, flag)`

- モード配列 base = `0x00b12dd0`、有効 byte = `0x00b12dc8`、値(float) = `0x00b12dd4`
- ストライド **24 バイト**、レコード内: `+0x0c` = duration、`+0x10` = handler、`+0x14` = progress
- `mode` 引数（ここでは `2`）で「どの入力系を有効にするか」を切り替える。
- **`[0x00b131fc] = 0xff`** は入力側の「リセット/全無効マーカー」で、フレーム先頭で
  立ててから `FUN_00572ec0` で必要な系だけを再有効化する。→ 前フレームの入力が
  フレームを跨いで持ち越されない（ポーリング結果の単一フレーム寿命）。

### 30.1.4 ワールド/カメラ更新 `FUN_0057f170(a, b, c)`

- **`dt == 0` ならワールド更新ブロック全体をスキップ**。
- `dt != 0` のときのみ `FUN_0070c0b0`（ワールド物理/シーン）→ `FUN_0070e0a0`（カメラ）
  を呼ぶ。
→ **`dt`（経過時間）が 0 に固定される（時刻停止 = ポーズ相当）とカメラは回らない。**
`dt` の正本は §28 で確定した時刻構造体（`0x00b33e90` = 再入深度ロック、周期計算は
QPC 差 / `0x00a2fc70`(=1000.0)）。

### 30.1.5 物理更新 `FUN_00679a70`

```
CMP byte [0x00b3a6d4], 0
JNZ RET                 ; メニュー表示中は丸ごとスキップ
```
→ **`0x00b3a6d4` は「メニュー表示中」フラグの正本**（物理系の抑制に使用）。

### 30.1.6 入力 update consumer `FUN_00440af0`

- `arg2 != 0` のとき、`LoadingMenu(0x3ef)` が非 0 なら**即リターン**。
- ポーズ中はサウンド側の処理をスキップ。
- **DirectInput の取得入口は `FUN_00404150`**（§29 で特定）。

### 30.1.7 最前面メニューの入力ブロック

最前面実メニュー取得 `FUN_005898f0()` が返すオブジェクトの **`[+0x24]`** が
入力消費フィールド。値 `1` または `8` のとき、そのメニューが入力を吸い上げる
（背面のワールドへフォールスルーさせない）。

### 30.1.8 メニュー API / 正本（確定）

| 名称 | アドレス / シグネチャ | 役割 |
|---|---|---|
| アクティブメニュー取得 | `FUN_00589b70(id)` | ID 指定で表示中メニューを取得 |
| 最前面メニュー取得 | `FUN_005898f0()` | 最前面の実メニュー |
| MenuManager 取得/生成 | `FUN_00582160(0, 1)` | `0x134` バイト、`0x00b3a6e0` に格納。`[+0x1c] != 0` で有効 |
| アクティブメニュー配列 | `0x00b13974`（先頭）/ `0x00b13978`（件数）/ `0x00b1397a`（上限） | **「開いているメニューの有無」の正本** |
| メニュー表示中フラグ | `0x00b3a6d4` | 物理・その他抑制のゲート |
| フェードリスト管理 | MenuManager `[+0x130]` | §30.4 |
| アプリインスタンス | `0x00b3a6b0` | `[+0x24]`=オーディオ、`[+0x28]`=入力、`[+0x2c]`=DirectInput、`[+0x1c]`=ポーズ |

### 30.1.9 ポーズ中 / 探索中メニューの扱いの差

- **ポーズ系（PauseMenu `0x3f5`）**: `FUN_0040fda0()` が真になり、`FUN_0057f170` 自体が
  呼ばれない → ワールド/カメラ/物理が完全停止。ゲーム時刻も停止する。
- **非ポーズ系（Options 等、`explorefade 0.25`）**: ポーズ判定は偽のままなので
  `FUN_0057f170` は呼ばれるが、`dt` と物理スキップ（`0x00b3a6d4`）および
  モード配列／`[+0x24]` 入力ブロックで入力を握り潰す。**カメラ回転は
  `[+0x24]` 入力ブロック＋モード配列側で止める**（dt は進み続ける）。

### Android 移植向け確定値（依頼1）

- 「メニューが開いているか」共通クエリは、オリジナル相当では
  **`0x00b13978`（アクティブメニュー件数） > 0** または **`0x00b3a6d4 != 0`**。
  移植側は `bool any_menu_open()` を 1 つ用意し、件数 >0 で真を返す実装が等価。
- 入力経路の移植は次の 3 段を**すべて**実装する:
  1. メニューが 1 つでも開いていれば **物理/AI 更新をスキップ**（`FUN_00679a70` 相当）。
  2. 最前面メニューが入力を消費する（`[+0x24]` 相当）→ `ACTION_MOVE` を
     カメラ handler に**渡さない**。
  3. ポーズ系メニューのみ **dt = 0 にしてワールド/カメラ更新ごと止める**
     （`FUN_0057f170` の `dt == 0` 早期スキップ相当）。
- `FUN_00679a70` のゲートは `0x00b3a6d4` 1 バイトなので、移植側は
  「メニュー表示中フラグ」1 個＋「最前面メニューの入力消費フラグ」で足りる。

## 30.2 依頼2: オプション設定値の「消費者」

### 30.2.1 audio: 実音量の算出式（確定）

実音量適用関数 **`FUN_006aa1a0`（215 バイト / 68 insns）**。引数は
`[ESP+4]` = float 音量、`[ESP+8]` = byte フラグ（非 0 で保存）。逆アセンブルで
確定した手順:

1. `FLDZ` / `FLD [ESP+4]` / `FCOM` による**下限クランプ**: `< 0` なら `0.0f` に置換。
2. `FLD1` / `FCOM` による**上限クランプ**: `> 1.0f` なら `1.0f` に置換。
3. `[ESP+8] != 0` なら `FST [ECX+0x2f8]` と `FST [ECX+0x2f0]` へ**保存**
   （`ECX` = オーディオマネージャ実体。voice 系統の保存先）。
4. `[0x00b3c0ec] != 0` なら値を `[0x00b16188]` で**上書き**（強制音量）。
5. `FMUL [ECX + 0xb8]` … **マスター音量（master）を乗算**。
6. `FMUL ST0` … **二乗（平方カーブ）**。
7. `TEST [ECX+0xdc], 1`（= dB 出力モード）で分岐:
   - **bit0 == 0（線形モード）**: そのまま `RET`（線形振幅を返す）。
   - **bit0 != 0（dB モード）**:
     - 結果 `< [0x00a34ba0] (= 0.01f)` → 無音定数 `0xffffd8f0`（= **-10000**）を
       `[ECX+0x74]` の vtable `[+0x1c]` へ渡す（無音）。
     - それ以外 → `CALL 0x00986cd0`（= **log10**）→ `FMUL double [0x00a77098]`
       (= **2000.0**) → `CALL 0x009828c0`（float → int）→ vtable `[+0x1c]` へ渡す。

→ **線形モード: 実音量 = clamp(slider, 0, 1)² × masterVolume。**
→ **dB モード: 実音量 = (int)(log10(clamp(slider, 0, 1)² × master) × 2000.0)、
`0.01` 未満は `-10000`（無音）。**
閾値は `0.005` ではなく **`0.01`**（`[0x00a34ba0]` の実値を PE 直読みで確認）。
**dB 係数は 1 dB = 2000 単位**（log10 ベース、1/100 dB 精度）。既定の DirectSound
経路では `[ECX+0xdc]` の bit0 が 1 なので **dB モードが常用**。

音量適用の中心は **`FUN_006ad030`（1081 insns）** で、`FUN_006aa1a0` を **6 回**
呼ぶが、6 箇所すべてが
`FLD [ESI+0x2f0]; PUSH 0; PUSH ECX; MOV ECX,ESI; FSTP [ESP]; CALL 0x006aa1a0`
＝ **`FUN_006aa1a0([ESI+0x2f0], 0)`** と同一コードである。すなわち
**保存済み voice 音量 `[+0x2f0]` を下位ストリームへ再適用しているだけ**で、
6 系統の個別音量を適用しているのではない（旧記述を本節で訂正）。アクセサ 14 関数は
すべてオーディオマネージャ正本 `[0x00b3c214]` へフォワードし、下位
`FUN_006ab500`〜`FUN_006acbf0` を呼ぶ。

### 30.2.2 audio: チャンネルグループ対応（確定）

INI 記述子テーブル **`0x00b1617c`**（8 バイト/エントリ、`+4` = `0x00b16180`）。
Audio セクションのキー 13 個とその既定値:

| # | キー | 既定値 |
|---|---|---|
| 0 | `bSoundEnabled:Audio` | 1（真） |
| 1 | `bMusicEnabled:Audio` | 1（真） |
| 2 | `fMainMenuMusicVolume:Audio` | 1.0 |
| 3 | `fDefaultMasterVolume:Audio` | 0.8 |
| 4 | `fDefaultFootVolume:Audio` | 0.3 |
| 5 | `fDefaultMusicVolume:Audio` | 0.8 |
| 6 | `fDefaultEffectsVolume:Audio` | 0.8 |
| 7 | `fDefaultVoiceVolume:Audio` | 0.0 |
| 8 | `fUnderwaterFrequencyDelta:Audio` | （整数） |
| 9 | `bUseSoundDebugInfo:Audio` | （真偽） |
| 10 | `fDialogAttenuationMin:Audio` | 7.75 |
| 11 | `fDialogAttenuationMax:Audio` | 35.0 |
| 12 | `fDSoundRolloffFactor:Audio` | 4.0 |
| 13 | `fDBVoiceAttenuationIn2D:Audio` | （float） |

→ **チャンネルグループは Master / Music / Effects / Voice / Footstep（Foot）の 5 系統**
（`fDefaultMasterVolume` / `fDefaultMusicVolume` / `fDefaultEffectsVolume` /
`fDefaultVoiceVolume` / `fDefaultFootVolume`）。audio_menu.xml のスライダーは
この 5 系統の**現在値**を、INI の `fDefault*` は**初期値**を保持する。
**スライダー widget ↔ 音量フィールドの対応は三者一致で確定**（AudioMenu ctor
`FUN_00595380` / 音量適用 `FUN_00595a20` / XML 行順のすべてが同一の対応を示す）。
詳細表は **§30.7.1** を参照。
`FUN_006ad030` の `FUN_006aa1a0` 6 回呼び出しは「5 系統＋1」ではなく、
**保存済み voice 音量の再適用 ×6** である（§30.2.1 末尾）。系統順序の推定は不要。

### 30.2.3 gameplay: 難易度とトグル（確定）

- 難易度スライダー: `fDifficultyMinValue:GamePlay` = **0.0f**、
  `fDifficultyMaxValue` = **100.0f**、既定 = **50.0f**（`FUN_005a3540` で初期化、
  呼出元 `0x005bd8be`）。
- トグル（`FUN_005a3540` が読む 6 個、値は bool）:

| キー | 効果 |
|---|---|
| `aDialogueSubtitles:GamePlay` | 会話字幕 |
| `bGeneralSubtitles:GamePlay` | 一般字幕 |
| `bCrossHair:GamePlay` | 照準カーソル表示 |
| `bSaveOnRest:GamePlay` | 休息時オートセーブ |
| `bSaveOnWait:GamePlay` | 待機時オートセーブ |
| `bSaveOnTravel:GamePlay` | 移動時オートセーブ |

難易度 0..100 はそのまま**ダメージ係数**（`0`=易、`100`=難）として戦闘計算に渡る。
`FUN_005a3540` が唯一の読取点であり、値を変更すると次回の同関数実行で反映される。

### 30.2.4 gameplay: 設定 UI → 実挙動の消費者（確定）

`FUN_005a3540`（難易度初期化）と 6 トグルの読取関数が**消費者**である。
UI 側（`gameplay_menu.xml`）のスライダー/トグルは INI または設定オブジェクトに
書き込み、上記関数群が読む。移植側 `Settings` は次の 3 点を実装すれば挙動が変わる:

1. 難易度 `0..100` → 戦闘ダメージ係数へ反映（`0.0`/`50.0`/`100.0` が min/既定/max）。
2. 6 トグルをそれぞれ「字幕表示」「照準表示」「休息/待機/移動時オートセーブ」の
   実行分岐に接続する。
3. audio の 5 スライダー → `実音量 = clamp(v,0,1)² × master`。dB 出力時は
   `(int)(log10(実音量) × 2000.0)`、`0.01` 未満は無音（`-10000`）。

### Android 移植向け確定値（依頼2）

- `v01 = clamp(v, 0, 1)`、`amp = v01^2 * master`（平方カーブ、master 乗算）。
- 線形出力は `amp` をそのまま使用。dB 出力は
  `db100 = (int)(log10(amp) * 2000.0)`、`amp < 0.01` は無音（`-10000`）。
  **1 dB = 2000 単位**として実装すると EXE と一致する（閾値は `0.005` ではなく
  `0.01`）。
- チャンネルは master / music / effects / voice / footstep の 5 系統
  （INI 既定 0.8 / 0.8 / 0.8 / 0.0 / 0.3）。スライダー対応は **§30.7.1** の表のとおり。
- master スライダーは `0..100` の int 表示で、内部値は `int(volume * 100.0)`。
- 難易度は `0.0..100.0`（既定 50.0）の float。6 トグルは §30.2.3 の表のとおり。
- 旧 TODO `audio-voice-footstep-setters` / `gameplay-settings-consumers` は
  上記で解消可能。

## 30.3 依頼3: video_display の解像度リスト生成 — 前提の訂正

### 30.3.1 重要な訂正

**`fResolution`（`0x00b45fe4`）は画面解像度ではない。** テクスチャ解像度比である。

`FUN_007e0ed0`（557 バイト、テクスチャ設定オブジェクト初期化）:

```
[ESI] = 0xa91b74                       ; vtable
[0x00b45fe0] = ESI                     ; 設定オブジェクトのグローバル正本
[0x00b45fc4] = [0x00a31e2c]            ; float 設定
[0x00b45fc0] = [0x00a430cc]
[0x00b45fd8] = [0x00a3d65c]
[0x00b45fd4] = 1.0f
if ([0x00b45fd0] != 0) {               ; 品質フラグ
    [0x00b45fc8] = 0x100   ; 256
    [0x00b45fcc] = 0x8     ; mip 8
} else {
    [0x00b45fc8] = 0x80    ; 128
    [0x00b45fcc] = 0x7     ; mip 7
}
[0x00b45fd1] = 1
fResolution = 1.0f / [0x00b45fc8]      ; = 1/256 または 1/128
[0x00b45fe4] = [0x00b45fe8] = [0x00b45fec] = [0x00b45ff0] = その値
```

`FUN_00499ef0`（255 バイト）は同じ計算を再実行する再初期化関数
（`[0x00b07058]` のトグルに応じて 128/256 を選び、`FUN_007e0cb0` と
`FUN_007e1710` でテクスチャを再生成）。呼出元は `FUN_007e1430@007e1430` の 1 件のみ。

**したがって**:
- `0x00b45fc8` = テクスチャ解像度（128 or 256）
- `0x00b45fcc` = mip 数（7 or 8）
- `0x00b45fd0` = 品質/設定トグル
- `0x00b45fe0` = テクスチャ設定オブジェクトのグローバル正本
- `0x00b45fe4`（fResolution） = **1.0 / テクスチャ解像度**（0.00390625 or 0.0078125）

`FUN_007dfa30`（134 insns）は INI 記述子登録関数であり、グループ 1 =
`"Time"` / `"fPassNum"` / `"fResolution"`、グループ 2 = `"texRatio0"` / `"texRatio1"`。
**`fResolution` はテクスチャ系キーであり、画面解像度とは無関係。**

### 30.3.2 解像度リストの生成元

- **`EnumDisplaySettings` / `EnumDisplayDevices` 等のインポートは存在しない**（212
  インポート中に不在）。→ 解像度列挙は **DirectX/D3D レンダラ内部**で行われる。
- `"%d x %d"`(`0xa6cf2c`)、`"%d: %s,%dx%d,%dkb"`(`0xa2f5e0`)、
  `sCantChangeResolution`(`0xa60613`)、`close-current-game`(`0xa6062c`) は
  **.text からの参照が 0 件**（VA・VA+IB・push 即値のいずれでも不検出）。
  → 解像度テキストは**レンダラ側の別文字列**（D3D が返すモード文字列）または
  動的整形で生成される。`exit-restart`(`0xa600c8`) のみ `0x9f1fb1` に参照あり。
- `bIgnoreResolutionCheck:Display`(`0xa3d524`) への参照 4 件
  （`0x47dee9` / `0x89e32a` / `0x89e49b` / `0x89e55b`）は**すべて RTTI/vftable
  記述子**（`MOV dword ptr [ESI], 0xa3d524` 等）であり、実 INI ブールの消費者では
  ない。→ **実消費者はレジストリ/動的文字列経由の可能性が高く、未確定。**

### 30.3.3 VideoRes / user3 / user4

- `VideoRes@VideoMenu` の RTTI は確認済み（§24.7 / §26.9 の記述と整合）。
- `user4`（解像度テキスト）と `user3`（フットノート表示可否）は、§19c の
  逆アセンブル `0x005dc260-0x005dc2fc` が **`0xfae` / `0xfaf` / `0xfb1` の
  メニュー user フィールド setter** であることから、**実行時にレンダラが返した
  モード文字列を setter 経由で流し込む**方式と確定。静的定数テーブルではない。

### Android 移植向け確定値（依頼3）

- `fResolution` は**画面解像度ではない**（テクスチャ解像度比 1/128 or 1/256）。
  移植側でこのキーを画面解像度に使ってはならない。
- 解像度リストは **Android の `Display.getSupportedModes()`** 相当で列挙し、
  `"<w> x <h>"` 形式の文字列を生成して VideoRes の `user4` に設定する
  （フットノート可否は `user3`）。
- `bIgnoreResolutionCheck` の実消費者は未確定（レジストリ/動的文字列候補）。
  移植側では解像度チェックを**無効化しておく**のが安全。
- 旧 TODO §24.11 の「解像度テキスト生成関数」は、**レンダラ内部 + setter 経由**
  であるため EXE 単体では静的特定できない（DirectX レイヤが生成）。

## 30.4 依頼4: メニュー開閉時のポーズ / フェード遷移

### 30.4.1 フェード登録 / 進行（確定）

- 登録: **`FUN_00584300(fade_id, duration, ...)`**（MenuManager `[+0x130]` の
  フェードリストへ追加）
- 進行: **`FUN_00583c30()`**（毎フレーム）
- フェードノードは **20 バイト**:
  - `+0x00` = `fade_id`
  - `+0x04` = `current`
  - `+0x08` = `duration`
- 進行で `current += dt`（`dt` = `[0x00b33e9c]`）。`current >= 1.0` でノードを除去
  （= フェード完了）。
- `explorefade`（既定 `0.25`）と `menufade`（既定 `0.25`）は **duration 引数**であり、
  フェード係数は `current / duration` の線形補間で求める。**係数算出は
  `FUN_00583c30` 内の `current += dt` と比較のみ**（別関数ではない）。

### 30.4.2 ゲーム時刻停止（ポーズ）

- ポーズ判定は **`FUN_0040fda0`**（レンダースレッドハンドル `0x00b33434`、
  終了コード `0x103` = ポーズ）。
- `FUN_00579cf0` は `FUN_0040fda0() == false` のときだけ `FUN_0057f170`（ワールド/
  カメラ更新、`dt` 依存）を呼ぶ。→ **ポーズ中は dt が実質 0 扱いになり時刻が止まる。**
- 探索中メニュー（Options 等）はポーズ判定が偽のため時刻は進む（§30.1.9）。

### 30.4.3 OptionsMenu.user0 と ESC / 戻るキー制御

メインループ `FUN_00579cf0` の分岐:

```
if ( FUN_00588bd0(0xfae) == 2.0f )     ; 0x00a379b4 = 2.0f
    FUN_005bc8b0
else
    FUN_005bcc00
```

- `0xfae` = メニュー user フィールド **user0**（`FUN_00588bd0` は当該メニューの
  trait リストから値を返す）。
- `0x00a379b4` の実体 = **`2.0f`**（PE 直読みで確認済み）。
- **訂正**: `FUN_005bc8b0` と `FUN_005bcc00` は ESC / 戻るキーのハンドラでは
  **なく**、**MessageMenu の表示 2 経路**である。
  - `FUN_005bc8b0`（390 行分のデコンパイル精読済） = MessageMenu を**新規作成**
    して前面化。
  - `FUN_005bcc00`（90 行分、精読済） = 既存 MessageMenu インスタンスの
    **前面化ラッパー**（作成しない）。
  - 分岐条件 `user0 == 2.0f` は「メッセージを新規作成するか、既存を前面化するか」
    の切替。旧 §22 の「ESC ハンドラ」という解釈は誤り（本節で訂正）。
- ESC / 戻る制御は汎用 Return（id `1`）と `OptionsMenu.user0` で行われる。
  汎用 Return（id `1`）は Options へ退出、BACK はページ 1 へ戻る
  （main 側検証済み・一致確認）。

### Android 移植向け確定値（依頼4）

- フェードは「`fade_id` + 現在値 + 継続時間」の 20 バイトノード 1 個で表現。
  `current += dt`、`current >= 1.0` で完了・除去。`explorefade 0.25` /
  `menufade 0.25` は継続時間（秒）。
- ポーズは「レンダースレッドが 0x103 を返す」＝ ワールド/カメラ更新の停止。
  移植側は「ポーズ中はワールド更新を呼ばない」で等価。
- メニューの `user0 == 2.0f` を「背面に PauseMenu あり」フラグとして使い、
  ESC / 戻るの遷移先を分岐させる（`2.0f` が PauseMenu 側）。

## 30.5 依頼5: 未文書化メニューの XML レイアウト素材（低優先・途中）

### 30.5.1 素材の実在パス（確定）

依頼文の `tmp\extracted_menus_full\menus\` は**PNG テクスチャのみ**を格納しており、
**XML は存在しない**。メニュー XML の実体は **`tmp\bsa_misc\menus\`** 配下:

| ファミリー | XML パス（`tmp\bsa_misc\menus\` 相対） |
|---|---|
| Container | `container_menu.xml` |
| LevelUp | `levelup_menu.xml` |
| QuickKeys | `main\quickkeys_menu.xml` |
| Recharge | `recharge_menu.xml` |
| Repair | `armorrepair` 系は `repair_menu.xml` |
| LockPick | `lockpick_menu.xml` |
| Message | `message_menu.xml` |
| Quantity | `quantity_menu.xml` |
| Negotiate / Persuasion | `negotiate_menu.xml` / `persuasion_menu.xml` |
| Training | `training_menu.xml` |
| Sleep/Wait | `sleep_wait_menu.xml` |
| Breath | `breath_meter_menu.xml` |
| Generic | `generic\test_menu.xml` |
| Dialog | `dialog\dialog_menu.xml` |
| chargen 系 | `chargen\attributes_menu.xml` / `birthsign_menu.xml` / `class_menu.xml` / `race_sex_menu.xml` / `skills_menu.xml` / `specilization_menu.xml` |
| HUD 系 | `main\hud_info_menu.xml` / `hud_main_menu.xml` / `hud_subtitle_menu.xml` |
| Inventory / Magic / Map / MagicPopup | `main\inventory_menu.xml` / `magic_menu.xml` / `map_menu.xml` / `magic_popup_menu.xml` |
| Loading 系 | `loading_menu.xml` / `loading_ingame_menu.xml` / `loading_bar_ingame_menu.xml` |
| Book / Scroll | `book_menu.xml` / `scroll_menu.xml` |

全メニュー ID↔名前↔型マッピングは §30.5.2 に確定済み（§29 の続き）。

### 30.5.2 全メニュー ID マッピング（確定・再掲）

trait: `image=0x386`, `rect=0x385`, `text=0x387`, `3d/nif=0x388`, `menu=0x389`,
`window=0x38b`, `template=0x3e7`。実メニュー:

`MessageMenu=0x3e9`, `InventoryMenu=0x3ea`, `StatsMenu=0x3eb`, `HUDMainMenu=0x3ec`,
`HUDInfoMenu=0x3ed`, `HUDReticle=0x3ee`, `LoadingMenu=0x3ef`, `ContainerMenu=0x3f0`,
`DialogMenu=0x3f1`, `HUDSubtitleMenu=0x3f2`, `GenericMenu=0x3f3`, `SleepWaitMenu=0x3f4`,
`PauseMenu=0x3f5`, `LockPickMenu=0x3f6`, `OptionsMenu=0x3f7`, `QuantityMenu=0x3f8`,
`AudioMenu=0x3f9`, `VideoMenu=0x3fa`, `VideoDisplayMenu=0x3fb`, `GameplayMenu=0x3fc`,
`ControlsMenu=0x3fd`, `MagicMenu=0x3fe`, `MapMenu=0x3ff`, `MagicPopupMenu=0x400`,
`NegotiateMenu=0x401`, `BookMenu=0x402`, `LevelUpMenu=0x403`, `TrainingMenu=0x404`,
`BirthSignMenu=0x405`, `ClassMenu=0x406`, `SkillsMenu=0x408`, `RepairMenu=0x40b`,
`RaceSexMenu=0x40c`, `SpellPurchaseMenu=0x40d`, `LoadMenu=0x40e`, `SaveMenu=0x40f`,
`AlchemyMenu=0x410`, `SpellmakingMenu=0x411`, `EnchantmentMenu=0x412`,
`EffectSettingMenu=0x413`, `MainMenu=0x414`, `BreathMenu=0x415`, `QuickKeysMenu=0x416`,
`CreditsMenu=0x417`, `SigilStoneMenu=0x418`, `RechargeMenu=0x419`, `TextEditMenu=0x41b`。

### Android 移植向け確定値（依頼5）

- 未文書化ファミリーの XML レイアウトは **`tmp\bsa_misc\menus\`** のファイルが
  素材（`tmp\extracted_menus_full\menus\` はテクスチャのみ）。
- §24/§25/§26 と同じ粒度の widget 表は **§31 で追記済み**（Container / QuickKeys /
  LevelUp / Recharge / Repair の 5 メニュー、31.1〜31.7）。本章では素材パスと ID を
  確定し、widget 単位の詳細は §31 を正典とする。

## 30.6 未確定事項（main へ申し送り）

1. ~~`FUN_006ad030` の 6 回 `FUN_006aa1a0` 呼び出しのチャンネル系統順序~~
   **解消済み**: 6 回の呼び出しはすべて `FUN_006aa1a0([ESI+0x2f0], 0)` で、
   保存済み voice 音量の再適用 ×6。5 系統対応は §30.2.2 / §30.7 で確定。
2. `bIgnoreResolutionCheck` の実 INI 消費者の特定（レジストリ/動的文字列候補）。
3. 解像度テキスト生成は DirectX レイヤ内部のため EXE 単体では静的特定不可。
4. 未文書化ファミリーの widget 単位レイアウト表: Container / QuickKeys / LevelUp /
   Recharge / Repair の 5 メニューは **§31 で解消済み**。残り（LockPick / Message /
   Quantity / Negotiate / Persuasion / Training / SleepWait / Book / Scroll /
   chargen 系 / HUD 系 等）は **§32 で追記**。
5. ~~`FUN_005bc8b0` 後半と `FUN_005bcc00` の分岐詳細~~ **解消済み**:
   `FUN_005bc8b0` = MessageMenu 作成、`FUN_005bcc00` = 前面化ラッパー
   （§30.4.3 訂正参照）。ESC 制御ではない。

### 30.7 追加確定事項（audio 音量 consumer 経路・完全確定）

#### 30.7.1 audio_menu.xml スライダー ↔ 音量フィールド（確定・三者一致）

`ctor` = `FUN_00595380`（AudioMenu コンストラクタ・951 バイト）、
`apply` = `FUN_00595a20`（音量適用・434 バイト / 142 insns）、
`obj` = オーディオマネージャ実体（`[0x00b33398] → [+0x24]`）。

| # | XML 行（ラベル） | slider id | marker id | ctor widget 取得 (`menu+`) | apply が読む widget | obj 側フィールド |
|---|---|---|---|---|---|---|
| 1 | master | 1 | 2 | `+0x28` | `MENU[+0x28]` | `[obj+0xb8]`（`×100.0` で int 化） |
| 2 | voice | 3 | 4 | `+0x48` | `MENU[+0x48]` | `[obj+0x2f0]` / `[obj+0x2f8]`（`FUN_006aa1a0(v,1)`） |
| 3 | effect | 5 | 6 | `+0x38` | `MENU[+0x38]` | `[obj+0xc4]` |
| 4 | foot | 7 | 8 | `+0x30` | `MENU[+0x30]` | `[obj+0xc0]` |
| 5 | music | 9 | 10 | `+0x40` | `MENU[+0x40]` | `[obj+0xbc]` |

- ctor は各スライダー widget に対し `FUN_0058ceb0(this=menu, id, value)` を同一
  パターンで設定（`r34_dc.txt` listing より確定）:
  - `0xfaf`（user1, min）= `0.0f`
  - `0xfb0`（user2, max）= `100.0f`
  - `0xfb2`（user4, jump）= `25.0f`
  - `0xfb3`（user5, 現在値）= `int(初期音量 × 100.0)` 設定 → 直後に `0.0f` でクリア
  - `0xfab`（depth）= `FUN_00588890()`（フェード値）
  - `0xfa5`（stackingtype）= 各 widget 生成時の共通設定
  - 未設定の `0xfb1`（user3, step）はプリハブ既定値を使用。
- ctor が読む初期値は obj 側フィールドから取得し **`× 100.0` して int 化**する
  （例: master は `FLD [obj+0xb8]; FMUL 100.0; CALL 0x009828c0`）。音声値は 0..1 の
  float で保持し、スライダーは 0..100 の int で表示する。
- apply は各スライダーの現在値を `FUN_00588bd0(0xfb5)`（user7 = 現在値）で取得し、
  `FUN_009828c0` で int 化 → `FILD` で float 化して上表のフィールドへ書き戻す。
  差分が無ければ early return、あれば末尾で `FUN_006aa280()` を呼ぶ（再適用トリガ）。
- **ctor での widget ↔ obj の初期値対応（listing 確定）**:
  `[ESI+0x28]←[obj+0xb8]`（master）、`[ESI+0x48]←FUN_006a8e00()`（voice）、
  `[ESI+0x40]←[obj+0xbc]`、`[ESI+0x30]←[obj+0xc0]`、`[ESI+0x38]←[obj+0xc4]`。
  apply 側の読取 widget（`+0x28 / +0x40 / +0x30 / +0x38 / +0x48`）と書き込み先が
  完全に一致することを確認済み。

#### 30.7.2 apply（`FUN_00595a20`）の書き込み内容

1. `[obj+0xb8]`（master）: `v != int(obj+0xb8 × 100.0)` のときのみ `v / 100.0` を書く。
2. voice: `v != int(FUN_006a8e00() × 100.0)` のとき `FUN_006aa1a0(v / 100.0, 1)`。
3. `[obj+0xc4]`（effect）= `v / 100.0`。
4. `[obj+0xbc]`（music）= `v / 100.0`。
5. `[obj+0xc0]`（foot）= `v / 100.0`。

呼出元: `FUN_00595380`（ctor 内の保険呼び出し）と vtable スロット `[7]`
（AudioMenu vtable `0x00a6b2c8`）。→ **スライダー操作 → apply → obj フィールド +
`FUN_006aa1a0`（実音量式）という一本の経路で確定。**

#### 30.7.3 MessageMenu の 2 経路（§30.4.3 訂正の詳細）

- `FUN_005bc8b0` = MessageMenu を生成して表示（作成経路）。
- `FUN_005bcc00` = 既存 MessageMenu を最前面へ（前面化経路）。
- 呼び分け条件は `FUN_00579cf0` 内の `FUN_00588bd0(0xfae) == 2.0f`（user0 == 2.0）。
- **ESC / 戻るキー制御とは無関係**。移植側は「メッセージキューが空なら作成、
  非空なら前面化」の 2 経路として実装すれば等価。

---

## 31. 未文書化メニュー ウィジェット単位レイアウト（依頼5 対応）

### 31.1 共通事項と読み方

依頼5 の対象 5 メニュー（Container / QuickKeys / LevelUp / Recharge / Repair）を、
§24（options）/ §25（save_load）/ §26（video_display）と同粒度の widget 単位表で
確定した。素材は `tmp\bsa_misc\menus\` の XML（`tmp\extracted_menus_full\menus\` は
テクスチャのみ）。抽出は自作パーサ `tmp\r30_menuparse.py --md`。

**メニュー ID（再掲）**

| メニュー | ID | ルートサイズ | stackingtype |
|---|---|---|---|
| ContainerMenu | 0x3f0 | 840×854 背景基準（screen 中央） | no_click_past |
| QuickKeysMenu | 0x416 | selector 800,275 + screen サイズ | does_not_stack |
| LevelUpMenu | 0x403 | screen 中央 + pane 540×536 | no_click_past |
| RechargeMenu | 0x419 | 956×837 背景基準 | no_click_past |
| RepairMenu | 0x40b | 1000×750 / 746×799 切替 | no_click_past |

**5 メニュー共通の外枠**

- ルート `alpha=0`、`locus=true`、`x=0`、`y=0`、`menufade=0.25`。
- 先頭に不可視の `@<name>_FocusBox`（`type=rect`, `visible=false`）を必ず持つ。
  ContainerMenu=`cont_FocusBox`(id14), RechargeMenu=`recharge_FocusBox`(id1),
  RepairMenu=`rep_FocusBox`(id7)。QuickKeys/LevelUp は FocusBox を持たない。
- Container / LevelUp / Recharge / Repair は `stackingtype=no_click_past`
  （背面メニューへのクリック透過を遮断）、QuickKeys のみ `does_not_stack`。

**表の列**

`Widget (path)` は `\` 区切りの親→子パス。`nest` は `<copy src="me()">` 系の
入れ子参照方向、`renderDepth` は同階層の描画順。`x`/`y`/`w`/`h` は値または
trait 式（`sub()`=自身の同 trait を引く、`add(N)`=N を加算、`div(N)`=N で除算、
`mul(N)`=N を乗算、`max()/min()`=兄弟を含む最大/最小）。`visible`/`content` の
`Menu.userN` は EXE 側が設定するメニュー固有 user フィールド。

---

### 31.2 ContainerMenu (`container_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| cont_FocusBox | rect | 14 | 0 |  |  |  |  | false | 1 |  |
| cont_background | image |  | 0 | add() max() sub(65) max() max(40) | screen().height div(2) sub(2) | 840 | 854 |  | 0 | Menus\Container\cont_box_background_1.dds |
| cont_background\cont_background_2 | image |  | 1 | 720 | 0 | 128 | 854 |  | 0 | Menus\Container\cont_box_background_2.dds |
| cont_background\cont_inv_background | image |  | 1 | 35 | 21 | 745 | 792 | parent().alpha | 1 | Menus\Shared\shared_main_background_1.dds |
| cont_background\cont_inv_background\cont_inv_background_2 | image |  | 2 | 700 | 0 | 54 | 792 |  | -15 | Menus\Shared\shared_main_background_2.dds |
| cont_background\page_layout | rect |  | 1 | 41 | 15 |  |  | true | 10 |  |
| cont_background\page_layout\cont_page_5 | image |  | 2 | 18 | 539 | 791 | 176 | ContainerMenu.user0 | 5 | Menus\Shared\shared_05_tab.dds |
| cont_background\page_layout\cont_page_4 | image |  | 2 | 18 | 539 | 791 | 176 | ContainerMenu.user0 | 5 | Menus\Shared\shared_04_tab.dds |
| cont_background\page_layout\cont_page_3 | image |  | 2 | 18 | 539 | 791 | 176 | ContainerMenu.user0 | 5 | Menus\Shared\shared_03_tab.dds |
| cont_background\page_layout\cont_page_2 | image |  | 2 | 18 | 539 | 791 | 176 | ContainerMenu.user0 | 5 | Menus\Shared\shared_02_tab.dds |
| cont_background\page_layout\cont_page_1 | image |  | 2 | 18 | 539 | 791 | 176 | ContainerMenu.user0 | 5 | Menus\Shared\shared_01_tab.dds |
| cont_background\page_layout\cont_tab_layout | rect |  | 2 | 21 | 20 |  |  |  |  |  |
| cont_background\page_layout\cont_tab_layout\cont_tabs_prev | rect | 7 | 3 | -1 | -1 | 1 | 1 |  |  |  |
| cont_background\page_layout\cont_tab_layout\cont_tabs_next | rect | 8 | 3 | -1 | -1 | 1 | 1 |  |  |  |
| cont_background\page_layout\cont_tab_layout\cont_p5_tab | rect | 5 | 3 | 553 | 610 |  |  |  | 6 | Menus\Inventory\inv_icon_tab_misc.dds |
| cont_background\page_layout\cont_tab_layout\cont_p4_tab | rect | 4 | 3 | 417 | 610 |  |  |  | 6 | Menus\Inventory\inv_icon_tab_alchemy.dds |
| cont_background\page_layout\cont_tab_layout\cont_p3_tab | rect | 3 | 3 | 279 | 610 |  |  |  | 6 | Menus\Inventory\inv_icon_tab_apparel.dds |
| cont_background\page_layout\cont_tab_layout\cont_p2_tab | rect | 2 | 3 | 141 | 610 |  |  |  | 6 | Menus\Inventory\inv_icon_tab_weapons.dds |
| cont_background\page_layout\cont_tab_layout\cont_p1_tab | rect | 1 | 3 | 1 | 610 |  |  |  | 6 | Menus\Inventory\inv_icon_tab_all.dds |
| cont_background\page_layout\cont_contents | rect |  | 2 | 8 | -70 |  |  |  | 7 |  |
| cont_background\page_layout\cont_contents\cont_border_top | image |  | 3 | 91 | 140 | 596 | 160 |  | 1 | Menus\Inventory\inv_border_top.dds |
| cont_background\page_layout\cont_contents\cont_border_middle | image |  | 3 | 91 | 276 | 576 | 249 |  | 1 | Menus\Inventory\inv_border_middle.dds |
| cont_background\page_layout\cont_contents\cont_border_bottom | image |  | 3 | 34 | 521 | 628 | 160 |  | 1 | Menus\Inventory\inv_border_bottom.dds |
| cont_background\page_layout\cont_contents\cont_player_money_icon | image |  | 3 | 50 | 648 | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| cont_background\page_layout\cont_contents\cont_player_money_icon\cont_player_money_text | text |  | 4 | 42 | -5 |  |  |  | 1 | ContainerMenu.user6 |
| cont_background\page_layout\cont_contents\cont_npc_money_icon | image |  | 3 | cont_scroll_bar.x sub(20) sub() sub() | 648 | 32 | 32 | ContainerMenu.user9 | 1 | Menus\Shared\all_small_icons.dds |
| cont_background\page_layout\cont_contents\cont_npc_money_icon\cont_npc_name | text |  | 4 | 0 sub() sub(10) | -5 |  |  | ContainerMenu.user9 | 1 | ContainerMenu.user11 |
| cont_background\page_layout\cont_contents\cont_npc_money_icon\cont_npc_money_text | text |  | 4 | 42 | -5 |  |  | ContainerMenu.user9 | 1 | ContainerMenu.user12 |
| cont_background\page_layout\cont_contents\cont_encumberance_icon | image |  | 3 | cont_scroll_bar.x sub(20) sub() sub() | 648 | 32 | 32 | ContainerMenu.user9 |  | Menus\Shared\all_small_icons.dds |
| cont_background\page_layout\cont_contents\cont_encumberance_icon\cont_encumberance_text | text |  | 4 | 40 | -5 |  |  |  |  | ContainerMenu.user13 |
| cont_background\page_layout\cont_contents\cont_encumberance_icon\cont_encumberance_slash | text |  | 4 | cont_encumberance_text.x add() add(1) | -5 |  |  |  |  | / |
| cont_background\page_layout\cont_contents\cont_encumberance_icon\cont_encumberance_max | text |  | 4 | cont_encumberance_slash.x add() add(1) | -5 |  |  |  |  | ContainerMenu.user14 |
| cont_background\page_layout\cont_contents\cont_button_take_all | image | 32 | 3 | 17 add(-24) | 810 |  |  | ContainerMenu.user7 | 1 |  |
| cont_background\page_layout\cont_contents\cont_button_haggle | image | 34 | 3 | 17 add(-16) | cont_button_take_all.y |  |  | ContainerMenu.user9 | 1 |  |
| cont_background\page_layout\cont_contents\cont_button_exit | image | 33 | 3 | 590 add(-35) | cont_button_take_all.y |  |  |  | 1 |  |
| cont_background\page_layout\cont_contents\cont_scroll_bar | image | 11 | 3 | 675 | cont_window.y |  | cont_window.height |  | 150 |  |
| cont_background\page_layout\cont_contents\cont_headers_top | rect |  | 3 | 20 | 22 |  |  |  |  |  |
| cont_background\page_layout\cont_contents\cont_window | rect |  | 3 | 10 | 150 | 645 | 480 |  | 12 |  |
| cont_background\page_layout\cont_contents\cont_window\cont_pane | rect |  | 4 | 6 | cont_scroll_bar.user7 mul(-81) |  |  |  |  |  |
| cont_background\page_layout\cont_contents\cont_window\cont_pane\cont_list_contents | rect | 13 | 5 |  |  |  |  |  |  |  |
| cont_background\page_layout\cont_contents\cont_item_template | template |  | 3 |  |  |  |  |  |  |  |
| cont_background\cont_select_frame | image |  | 1 | 272 | 729 | 287 | 86 |  | 10 | Menus\Container\cont_select_frame.dds |
| cont_background\cont_select_frame\cont_icon_inv_off | image | 42 | 2 | 53 | 4 | 86 | 78 | ContainerMenu.user7 | -2 | Menus\Container\cont_icon_inv_off.dds |
| cont_background\cont_select_frame\cont_icon_inv_on | image |  | 2 | 53 | 4 | 86 | 78 | cont_icon_inv_off.visible | -2 | Menus\Container\cont_icon_inv_on.dds |
| cont_background\cont_select_frame\cont_icon_cont_off | image | 43 | 2 | 141 | 5 | 90 | 78 | ContainerMenu.user7 | -2 | Menus\Container\cont_icon_cont_off.dds |
| cont_background\cont_select_frame\cont_icon_cont_on | image |  | 2 | 145 | 5 | 90 | 78 | cont_icon_cont_off.visible | -2 | Menus\Container\cont_icon_cont_on.dds |
| cont_background\cont_select_frame\cont_select_frame_marker | image |  | 2 | me().user1 add() | -3 | 128 | 95 |  | 30 | Menus\Container\cont_select_frame_marker.dds |
| cont_background\cont_select_frame\cont_select_left | image |  | 2 | 0 sub() add(56) | 25 | 42 | 35 | xbox | 6 | Menus\HUD\hud_xbox_lt.dds |
| cont_background\cont_select_frame\cont_select_right | image |  | 2 | parent().width sub(56) | 25 | 42 | 35 | xbox | 6 | Menus\HUD\hud_xbox_rt.dds |

#### 31.2.1 コンテナ リスト行テンプレート（`cont_item_template`）

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| cont_item_template\cont_item | rect |  | 4 | 0 | me().listindex | 650 | 81 | me().user8 | 1 |  |
| cont_item_template\cont_item\cont_equiped_marker | image |  | 5 | 7 | 7 | 635 | 64 | parent().user10 | 2 | Menus\Inventory\inv_equiped_marker_1.dds |
| cont_item_template\cont_item\cont_item_icon | image |  | 5 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user0 |
| cont_item_template\cont_item\cont_item_over_icon | image |  | 5 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user6 |
| cont_item_template\cont_item\cont_item_count | text |  | 5 | sibling(cont_item_icon).x add() | parent().height sub() sub(6) |  |  |  | 5 | parent().user7 |
| cont_item_template\cont_item\cont_item_status | image |  | 5 | cont_item_icon.x | cont_item_icon.y add() sub() | 64 sub(32) | 32 add(25) | parent().user12 | cont_item_icon.depth add(1) | parent().user13 |
| cont_item_template\cont_item\cont_item_name | text |  | 5 | 80 | parent().height sub() div(2) |  |  |  | 3 | parent().user1 |
| cont_item_template\cont_item\cont_item_cost | text |  | 5 | 418 | parent().height sub() div(2) |  |  |  | 3 | parent().user2 |
| cont_item_template\cont_item\cont_item_weight | text |  | 5 | 496 | parent().height sub() div(2) |  |  |  | 3 | parent().user3 |
| cont_item_template\cont_item\cont_item_damage | text |  | 5 | 556 | parent().height sub() div(2) |  |  |  | 3 | parent().user4 |
| cont_item_template\cont_item\cont_item_health | text |  | 5 | 616 | parent().height sub() div(2) |  |  |  | 3 | parent().user5 |

行高 **81 px**、`y = me().listindex`（`cont_pane.y = cont_scroll_bar.user7 mul(-81)`
と対でスクロール）。列 x は name=80 / cost=418 / weight=496 / damage=556 /
health=616。`cont_item_status` の w/h trait は `sub()`/`add()` の連鎖による
耐久バー幅の算出。

#### 31.2.2 カテゴリ見出し（`cont_headers_top` 配下 + 各行テンプレート）

上段見出し `cont_p123_header_top`(depth4) / `cont_p4p5_header_top`(depth4) は
同一構造で、`cont_p<N>_header_text` の content は
`parent()._headernum parent()._header_`（動的文字列）。子は
`_header_left`(shared_decor_blue_left.dds 35×17, depth7) /
`_header_right`(shared_decor_blue_right.dds 35×17, depth7) /
`_header_text`(text, x=130 y=5) /
`_header_img1..img4`(all_small_icons.dds 32×32, depth5, x=300/375/435/495,
`visible=parent()._headernum`) と、`_header_img1` 配下の
`_header_arrow`(x=23 y=0 32×32, `visible=ContainerMenu.user25`)。

`cont_window\cont_pane` 配下のカテゴリ見出し 9 種は同一テンプレートで、
差異は下表のみ（それ以外の子 widget の x/y/w/h/depth は共通）。

| 見出し | rect y | rect visible | text content |
|---|---|---|---|
| cont_weapon_header | ContainerMenu.user1 | ContainerMenu.user1 | strings()._weapons |
| cont_apparel_header | ContainerMenu.user2 | ContainerMenu.user2 | strings()._apparel |
| cont_apparatus_header | ContainerMenu.user3 | ContainerMenu.user3 | strings()._apparatuses |
| cont_potions_header | ContainerMenu.user4 add() | ContainerMenu.user0 | strings()._potions |
| cont_ingr_header | ContainerMenu.user15 add() | ContainerMenu.user0 | strings()._ingredients |
| cont_book_header | ContainerMenu.user16 | ContainerMenu.user16 | strings()._books |
| cont_keys_header | ContainerMenu.user17 add() | ContainerMenu.user0 | strings()._keys |
| cont_stones_header | ContainerMenu.user18 add() | ContainerMenu.user0 | strings()._stones |
| cont_misc_header | ContainerMenu.user19 add() | ContainerMenu.user0 | strings()._misc |

共通子 widget（各見出し直下）:

| Widget | type | renderDepth | x | y | w | h | visible | content |
|---|---|---|---|---|---|---|---|---|
| <h>_header_left | image | 7 | text.x sub(2) sub(55) | text.y add(5) | 35 | 17 |  | Menus\Shared\shared_decor_blue_left.dds |
| <h>_header_right | image | 7 | text.width div(2) add() add(20) | text.y add(5) | 35 | 17 |  | Menus\Shared\shared_decor_blue_right.dds |
| <h>_header_text | text |  | 130 | 35 |  |  |  | strings()._<cat> |

右側アイコン列（x=300 起点、y=30、32×32、all_small_icons.dds）:

| 見出し | アイコン数 | アイコン x | 矢印 widget | 矢印 x | 矢印 content |
|---|---|---|---|---|---|
| weapon | 4 (value/weight/damage/health) | 300, 374, 438, 502 | cont_value_arrow | 23 | all_small_icons.dds (visible=ContainerMenu.user25) |
| apparel | 4 (value/weight/armor/health) | 300, 374, 438, 502 | cont_apparel_arrow_icon | 23 | all_small_icons.dds (user25) |
| apparatus | 3 | 300, 375, 435 | cont_apparatus_header_arrow | 23 | all_small_icons.dds (user25) |
| potions | 2 | 300, 375 | cont_potions_header_arrow | 23 | all_small_icons.dds (user25) |
| ingr | 2 | 300, 375 | cont_ingr_header_arrow | 23 | all_small_icons.dds (user25) |
| book | 2 | 300, 375 | cont_book_header_arrow | 23 | all_small_icons.dds (user25) |
| keys | 2 | 300, 375 | cont_keys_header_arrow | 300 (depth10) | Menus\Icons\icon_small_arrow.dds (user25) |
| stones | 2 | 300, 375 | cont_stones_header_arrow | 300 (depth10) | Menus\Shared\all_small_icons.dds (user25) |
| misc | 2 | 300, 375 | cont_misc_header_arrow | 23 | all_small_icons.dds (user25) |

（weapon/apparel の 3 番目以降は `_icon.x add(74)` / `add(64)` の連鎖で配置。
apparatus は `add()` のみ、potions/ingr/book/misc は `value_icon.x add(74)` の
2 個目のみが確定値で 375。keys/stones の `_img3` はそれぞれ
`Menus\Icons\icon_small_arrow.dds` / `Menus\Icons\icon_small_weight.dds` を参照。）

---

### 31.3 QuickKeysMenu (`quickkeys_menu.xml`)

ルート直下はわずか 2 widget（`quick_key_text` id6 と `Quick_key_selector` id7）で、
8 スロットはすべて `Quick_key_selector` の子。

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| quick_key_text | text | 6 | 0 | Quick_key_selector.x add() add(200) | Quick_key_selector.y add(20) sub() |  |  |  | 11 | strings()._quickkey |
| Quick_key_selector | rect | 7 | 0 | 800 | 275 | screen().width | screen().height | true |  |  |
| Quick_key_selector\background_selector_image | image |  | 1 | -58 | -58 | 512 | 512 | parent().user2 | 1 | Quick-Keys\background_paper.dds |
| Quick_key_selector\background_selector_image2 | nif |  | 1 | 200 | 200 |  |  | parent().user3 | 1 | Quick-Keys\pc_background_paper_numbers.nif |
| Quick_key_selector\quick_key_1_image | image | 8 | 1 | 202 sub(2) | 102 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_2_image | image | 9 | 1 | 274 sub(2) | 132 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_3_image | image | 10 | 1 | 302 sub(2) | 205 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_4_image | image | 11 | 1 | 272 sub(2) | 275 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_5_image | image | 12 | 1 | 200 sub(2) | 303 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_6_image | image | 13 | 1 | 130 sub(2) | 273 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_7_image | image | 14 | 1 | 102 sub(2) | 202 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\quick_key_8_image | image | 15 | 1 | 132 sub(2) | 130 sub(2) | 64 | 64 | parent().visible | 2 |  |
| Quick_key_selector\selector_image | nif |  | 1 | -235 | -90 |  |  | parent().visible | 3 | Quick-Keys\quick keys.nif |

- 8 スロットは八分円配置（時計回りに 1→8）。`sub(2)` は自サイズ 64 の 1/2=32 を
  中心補正する式。実効中心座標は 1=(202,102) 2=(274,132) 3=(302,205) 4=(272,275)
  5=(200,303) 6=(130,273) 7=(102,202) 8=(132,130)。
- 背景切替は `parent().user2`（paper.dds）と `parent().user3`（numbers.nif）の排他。
- `quick_key_text` は selector 右上（+200, +20）にラベルを出す。

---

### 31.4 LevelUpMenu (`levelup_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| levelup_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  |  | 1 |  |
| levelup_background\levelup_title | text |  | 1 | parent().width sub() div(2) | 20 |  |  |  | 4 | LevelUpMenu.user3 |
| levelup_background\levelup_description | text |  | 1 | parent().width sub() div(2) | levelup_title.y add() add(20) |  |  |  | 4 | LevelUpMenu.user2 |
| levelup_background\levelup_instructions | text |  | 1 | levelup_pane.width sub() div(2) | levelup_description.y add() add(30) |  |  |  | 4 | strings()._threeattributes |
| levelup_background\level_up_picture | image |  | 1 | 620 | levelup_pane.y | 256 | 256 |  | 10 | LevelUpMenu.user4 |
| levelup_background\levelup_attribute_description | text |  | 1 | level_up_picture.x sub() | level_up_picture.y add() add(20) |  |  |  | 4 | LevelUpMenu.user5 |
| levelup_background\levelup_pane | rect | 2 | 1 | 30 | levelup_instructions.y add() add(20) | 540 | 536 |  | 5 |  |
| levelup_background\levelup_exit_button | image | 1 | 1 | parent().width sub() sub(30) | parent().height sub() sub(20) |  |  |  | 6 |  |
| levelup_template | template |  | 0 |  |  |  |  |  |  |  |
| levelup_template\levelup_item | rect | 3 | 1 | 0 | me().listindex | 450 | 67 | true | 1 |  |
| levelup_template\levelup_item\levelup_item_coin | image |  | 2 | -10 | parent().height sub() div(2) | 32 | 32 | parent().user0 | 4 | Menus\Shared\all_small_icons.dds |
| levelup_template\levelup_item\levelup_item_icon | image |  | 2 | 40 | parent().height sub() div(2) | 64 | 64 |  | 4 | parent().user5 |
| levelup_template\levelup_item\levelup_item_text | text |  | 2 | 130 | parent().height sub() div(2) |  |  |  | 4 | parent().user4 |
| levelup_template\levelup_item\levelup_item_value | text |  | 2 | 380 | parent().height sub() div(2) |  |  |  | 4 | parent().user3 |
| levelup_template\levelup_item\levelup_item_multiplier | text |  | 2 | 480 | parent().height sub() div(2) |  |  | parent().user1 | 4 | parent().user2 |
| levelup_template\levelup_item\levelup_item_selected | image |  | 2 | 115 | parent().height sub() div(2) add(1) | parent().width sub(70) | 64 | parent().user0 | 1 | Menus\Inventory\inv_equiped_marker_1.dds |
| levelup_template\levelup_item\levelup_item_mouseover_marker | image |  | 2 | 115 | 5 | parent().width sub(90) | 64 |  | 2 | Menus\dialog\dialog_selection_full.dds |
| levelup_template\levelup_item\levelup_item_mouseover_marker_2 | image |  | 2 | parent().width add(55) sub() | 5 | 104 | 64 |  | 3 | dialog\dialog_selection_cut.dds |

- 説明文は `user2`、属性名は `user5`、属性図は `user4`（`level_up_picture.filename`）。
- 行高 **67 px**、`y = me().listindex`。列 x は icon=40 / text=130 / value=380 /
  multiplier=480。`multiplier` のみ `visible=parent().user1`。
- 退出ボタン `levelup_exit_button`(id1) は右下（-30, -20）。`user0` に `_exit`、
  `user1` に遷移先を持つ（§28 の user フィールド表と整合）。

---

### 31.5 RechargeMenu (`recharge_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| recharge_FocusBox | rect | 1 | 0 |  |  |  |  | false |  |  |
| recharge_background | image |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 956 | 837 |  | 1 | Menus\Recharge\recharge_background.dds |
| recharge_background\recharge_info_icon | image |  | 1 | 214 add() | 108 add() | 32 add(32) | me().width |  | 2 | RechargeMenu.user0 me()._filename_ |
| recharge_background\recharge_info_icon\recharge_info_count | text |  | 2 | 65 | 42 |  |  | RechargeMenu.user6 | 2 | RechargeMenu.user6 |
| recharge_background\recharge_info_icon\recharge_info_text | text |  | 2 | parent().width add(10) | parent().height sub() div(2) |  |  |  | 2 | RechargeMenu.user2 |
| recharge_background\recharge_soulgem_soul_layout | rect |  | 1 | 600 | 104 | 175 |  | RechargeMenu.user0 |  |  |
| recharge_background\recharge_soulgem_soul_layout\recharge_soulgem_soul_label | text |  | 2 | parent().width sub() div(2) | 0 |  |  |  | 2 | strings()._soul |
| recharge_background\recharge_soulgem_soul_layout\recharge_cost | text |  | 2 | parent().width sub() div(2) | 30 |  |  |  | 2 | RechargeMenu.user4 |
| recharge_background\spell_gold_money_icon | image |  | 1 | 600 | recharge_info_icon.y | 32 | 32 | RechargeMenu.user0 | 2 | Menus\Shared\all_small_icons.dds |
| recharge_background\spell_gold_money_icon\spell_gold_arrow_icon | image |  | 2 | 23 | 0 | 32 | 32 |  | 2 | Menus\Shared\all_small_icons.dds |
| recharge_background\spell_gold_money_icon\recharge_cost | text |  | 2 | 55 | -3 |  |  |  | 2 | RechargeMenu.user4 |
| recharge_background\recharge_health_icon | image |  | 1 | parent().width | parent().height | 32 | 32 |  | 2 | Menus\Shared\all_small_icons.dds |
| recharge_background\recharge_contents | rect |  | 1 | 138 | 226 | 640 | 475 |  | 2 |  |
| recharge_background\recharge_contents\recharge_list_pane | rect | 6 | 2 | 0 | 0 sub() | parent().width | me().childcount |  |  |  |
| recharge_background\recharge_contents\recharge_scroll_bar | image | 4 | 2 | parent().width add(20) | 32 |  | parent().height sub(64) |  | 0 |  |
| recharge_background\recharge_button_exit | image | 3 | 1 | parent().width sub(50) | parent().height |  |  |  | 2 |  |
| recharge_item_template | template |  | 0 |  |  |  |  |  |  |  |
| recharge_item_template\recharge_item | rect |  | 1 | 0 | me().listindex | recharge_contents.width | RechargeMenu.user3 |  |  |  |
| recharge_item_template\recharge_item\item_icon | image |  | 2 | 0 | parent().height sub() div(2) | 64 | 64 |  |  | parent().user3 |
| recharge_item_template\recharge_item\item_name | text |  | 2 | sibling(item_icon).x add() add(20) | parent().height sub() div(2) |  |  |  |  | parent().user1 |
| recharge_item_template\recharge_item\item_charge | text |  | 2 | sibling(item_name).x add() add(sibling(item_name).x add(),2) | parent().height sub() div(2) |  |  |  |  | parent().user2 |

- `recharge_info_icon` は `filename = RechargeMenu.user0 me()._filename_`
  （アイコン画像 + 個数表示）。`recharge_info_count` の表示可否は `user6`。
- ソウル欄 `recharge_soulgem_soul_layout`(600,104,175) は `user0` で開閉、
  コストは `recharge_cost = user4`（2 箇所に同 content）。
- リスト行高は `RechargeMenu.user3`（行ごとの可変高）、
  `recharge_list_pane.h = me().childcount`、スクロールバーは
  `recharge_scroll_bar`(id4, h=parent().height-64)。

---

### 31.6 RepairMenu (`repair_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| rep_FocusBox | rect | 7 | 0 |  |  |  |  | false |  |  |
| rep_background_repair | image | generic | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1000 | 750 | RepairMenu.user0 | 1 | Menus\ArmorRepair\Armor_Repair.dds |
| rep_background_general | image | generic | 0 | screen().width sub() div(2) sub(200) | screen().height sub() div(2) | 746 | 799 | RepairMenu.user0 | rep_background_repair.depth | Menus\Shared\shared_combo_background_1.dds |
| rep_background_general\rep_background_general_2 | image |  | 1 | 700 | 0 | 64 | 799 |  | -15 | Menus\Shared\shared_combo_background_2.dds |
| rep_title | text |  | 0 | rep_background_repair.x add(2) sub(100) | rep_contents.y sub() sub(10) |  |  | parent().user0 | 3 | strings()._repair |
| rep_contents | rect |  | 0 | rep_background_general.x add(55) add(182) | rep_background_general.y add(100) add(70) | 582 | 400 add(160) |  | rep_background_repair.depth add() add(2) |  |
| rep_contents\horiz_grid_top | image |  | 1 | 0 | 0 sub(2) | parent().width | 4 | true | 31 | Menus\Shared\shared_border_horizontal_1.dds |
| rep_contents\horiz_grid_bottom | image |  | 1 | 0 | parent().height sub(2) | parent().width | 4 | true | 31 | Menus\Shared\shared_border_horizontal_3.dds |
| rep_contents\vert_grid_left | image |  | 1 | 0 sub(2) | 0 | 4 | parent().height | true | 31 | Menus\Shared\shared_border_vertical_2.dds |
| rep_contents\vert_grid_column1 | image |  | 1 | parent().width sub(2) sub(190) | 0 | 4 | parent().height | RepairMenu.user0 | 31 | Menus\Shared\shared_border_vertical_1.dds |
| rep_contents\vert_grid_column2 | image |  | 1 | parent().width sub(2) sub(95) | 0 | 4 | parent().height | RepairMenu.user0 | 31 | Menus\Shared\shared_border_vertical_1.dds |
| rep_contents\vert_grid_right | image |  | 1 | parent().width sub(2) | 0 | 4 | parent().height | true | 31 | Menus\Shared\shared_border_vertical_2.dds |
| rep_contents\money_icon | image |  | 1 | vert_grid_column1.x add() add(vert_grid_column1.x add(),2) sub(5) | 0 sub() sub(10) | 32 | 32 | RepairMenu.user0 | 20 | Menus\Shared\all_small_icons.dds |
| rep_contents\money_icon\money_icon_arrow | image |  | 2 | 23 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| rep_contents\health_icon | image |  | 1 | vert_grid_column2.x add() add(vert_grid_column2.x add(),2) | 0 sub() sub(10) | 32 | 32 | RepairMenu.user0 | 20 | Menus\Shared\all_small_icons.dds |
| rep_contents\rep_list_pane | rect | 8 | 1 | 0 | 0 sub() mul(80) | parent().width | me().childcount mul(80) |  |  |  |
| rep_contents\rep_scroll_bar | image | 5 | 1 | parent().width add(20) | 32 |  | parent().height sub(64) |  | 0 |  |
| rep_item_template | template |  | 0 |  |  |  |  |  |  |  |
| rep_item_template\rep_item | rect |  | 1 | -2 | me().listindex | rep_contents.width sub() | 80 | me().user8 | 1 |  |
| rep_item_template\rep_item\rep_equiped_marker | image |  | 2 | 7 | 7 | parent().width | 64 | parent().user10 | 2 | Menus\Inventory\inv_equiped_marker_1.dds |
| rep_item_template\rep_item\rep_item_icon | image |  | 2 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user0 |
| rep_item_template\rep_item\rep_item_over_icon | image |  | 2 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user6 |
| rep_item_template\rep_item\rep_item_count | text |  | 2 | rep_item_icon.x add() add(2) | parent().height sub() |  |  |  | 5 | parent().user7 |
| rep_item_template\rep_item\rep_item_name | text |  | 2 | 80 | parent().height sub() div(2) |  |  |  | 3 | parent().user1 |
| rep_item_template\rep_item\rep_item_health | text |  | 2 | parent().width sub(44) | parent().height sub() div(2) |  |  |  | 3 | parent().user5 |
| rep_buy_item_template | template |  | 0 |  |  |  |  |  |  |  |
| rep_buy_item_template\rep_item | rect |  | 1 | -2 | me().listindex | rep_contents.width sub() | 80 | me().user8 | 1 |  |
| rep_buy_item_template\rep_item\rep_equiped_marker | image |  | 2 | 7 | 7 | parent().width | 64 | parent().user10 | 2 | Menus\Inventory\inv_equiped_marker_1.dds |
| rep_buy_item_template\rep_item\rep_item_icon | image |  | 2 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user0 |
| rep_buy_item_template\rep_item\rep_item_over_icon | image |  | 2 | 5 | 7 | 64 | 64 | sub() | 3 | parent().user6 |
| rep_buy_item_template\rep_item\rep_item_count | text |  | 2 | 70 | 65 |  |  |  | 5 | parent().user7 |
| rep_buy_item_template\rep_item\rep_item_name | text |  | 2 | 80 | parent().height sub() div(2) |  |  |  | 3 | parent().user1 |
| rep_buy_item_template\rep_item\rep_item_cost | text |  | 2 | rep_item_health.x sub(96) | parent().height sub() div(2) |  |  |  | 3 | parent().user2 |
| rep_buy_item_template\rep_item\rep_item_health | text |  | 2 | parent().width sub(44) | parent().height sub() div(2) |  |  |  | 3 | parent().user5 |
| rep_button_remove | image | 15 | 0 | rep_background_general.x add(90) sub(30) | 670 add(85) |  |  | false | rep_background_repair.depth add() add(2) |  |
| rep_button_repair_all | image | 16 | 0 | rep_button_exit.x sub(500) add(10) | rep_button_exit.y |  |  |  | rep_button_remove.depth |  |
| rep_button_filter | image | 17 | 0 | rep_button_exit.x sub(200) | rep_button_exit.y |  |  |  | rep_button_remove.depth |  |

- 背景は 2 種の切替。`visible = RepairMenu.user0` で
  `Armor_Repair.dds`(1000×750, 装甲修理) / `shared_combo_background_1.dds`
  (746×799, 汎用) を排他表示。`rep_background_general_2`(64×799) は右端の
  連続テクスチャ。
- 格子罫線 `rep_contents` 配下は `shared_border_*`。列境界は
  `vert_grid_column1`(右端-190) / `vert_grid_column2`(右端-95)、左/右端は
  `vert_grid_left`/`vert_grid_right`、上/下は `horiz_grid_top`/`horiz_grid_bottom`。
- リストは 2 テンプレート。`rep_item_template`(修理) と
  `rep_buy_item_template`(購入) は行高 **80 px** で共通、`y = me().listindex`。
  行の表示可否は `me().user8`。購入テンプレートのみ `rep_item_cost`
  (`content = parent().user2`, x = `rep_item_health.x sub(96)`) を追加。
- 行アイコン: `rep_item_icon`(content = `parent().user0`) と
  `rep_item_over_icon`(`parent().user6`) を重ねる。装備マーカーは
  `inv_equiped_marker_1.dds`(`visible = parent().user10`)。
- ボタン: `rep_button_remove`(id15, 初期 `visible=false`) /
  `rep_button_repair_all`(id16) / `rep_button_filter`(id17) /
  `rep_button_exit`(id2, 参照基準)。ボタン x はすべて `rep_button_exit` からの
  相対（-500/+10、-200）。`repair_info_icon` の `filename = user0 me()._icon_`、
  `hammer_number_text`(id3) は `me().user0`。

---

### 31.7 Android 移植向け確定値（依頼5 総括）

1. **メニュー登録**: ContainerMenu=0x3f0 / QuickKeysMenu=0x416 / LevelUpMenu=0x403 /
   RechargeMenu=0x419 / RepairMenu=0x40b。いずれもルートに `alpha=0`、
   `locus=true`、`x=y=0`、`menufade=0.25` を設定する。
2. **リスト行の座標系**: 行の `y` は例外なく `me().listindex`（行インデックス）。
   スクロール量は親 `*_scroll_bar.user7` で与え、行送りは
   Container=81 px / Repair=80 px / Recharge=可変(`user3`) / LevelUp=固定 67 px。
   Android 実装では「行 y = index * lineHeight - scrollOffset」に帰着する。
3. **スクロールバー**: `*_scroll_bar`(Container=id11, LevelUp=なし, Recharge=id4,
   Repair=id5) の trait `user1..user4` は §24.9 / §25.5 のプレハブ定義と同一。
   実装は共通スクロールバー widget を再利用できる。
4. **user フィールド用途表**

   | メニュー | user フィールド |
   |---|---|
   | ContainerMenu | user0=ページ/見出し活性, user1..4=見出し y/可視, user6=所持金, user7=inv/cont モード, user9=NPC 取引有無, user11=NPC 名, user12=NPC 資金, user13=現重量, user14=最大重量, user15..19=見出し y, user25=ソート矢印 |
   | QuickKeysMenu | user2=paper 背景表示, user3=numbers 背景表示 |
   | LevelUpMenu | user1=multiplier 可視, user2=multiplier 文字列, user3=value, user4=picture filename(属性図), user5=属性説明 |
   | RechargeMenu | user0=info アイコン/ソウル欄表示, user2=info テキスト, user3=行高, user4=コスト, user6=個数表示 |
   | RepairMenu | user0=背景種別(装甲/汎用)・列罫線可視, user8=行可視/高さ |

5. **行テンプレートの親子**: 行 widget の子は `parent().userN` を参照する
   （`me()` は行自身）。`sibling(name)` は同行内の兄弟 widget の座標を参照する。
   Android 側は「行オブジェクト → 子 widget」の 1 段のプロパティ供給で再現可能。
6. **未検証**: RepairMenu の `rep_button_exit`(id2) の実座標は
   `rep_button_repair_all`/`rep_button_filter` の参照基準としてのみ現れ、
   自身の x/y 確定値は他メニュー（§25 save_load 系）と共通の右下配置と推定。
   `ContainerMenu` の `cont_tabs_prev`/`cont_tabs_next`(id7/id8) は 1×1 の
   当たり判定専用 widget で、タブ送りの入力領域として扱う。


---

## 32. 未文書化メニュー ウィジェット単位レイアウト（残ファミリー／依頼5 追加分）

§24〜§31 で扱わなかった残りのメニュー XML を、§31 と同じ粒度（ウィジェット 1 行＝1 レコード、
座標・寸法の式は trait 値そのまま）で列挙する。対象は `tmp\bsa_misc\menus\` 配下のうち、
Options ファミリー（Options / Gameplay / Video / VideoDisplay / Audio / Controls）、
Save-Load、Container / QuickKeys / LevelUp / Recharge / Repair 以外の全 XML である。

### 32.0 読み方と列の意味

| 列 | 意味 |
|---|---|
| Widget (path) | ウィジェット名。`\` 区切りは親子階層を表す（入れ子の各段） |
| type | `<_tag>` の種別（image / rect / text / nif / menu / window / template / button 等） |
| id | `<id>` の数値。空欄は固有 id を持たない（装飾・プレハブ断片） |
| nest | 宣言上の入れ子段（0 = ルート直下）。実際の親子は path の `\` を正とする |
| x / y | `<x>` / `<y>` の式をそのまま記載。`parent()`/`me()`/`screen()`/`sibling()`/`last()`/`first()` は Bethesda 記法の関数呼び出し |
| w / h | `<width>` / `<height>`。空欄は既定（画像は画像寸法、テキストは自動） |
| visible | `<visible>` の初期値。空欄は true（既定） |
| depth | 描画深度 `<depth>`。値が大きいほど手前 |
| content | `<filename>`（画像/nif）または `<string>` / trait 参照（テキスト） |

補足:
- 座標式は「そのまま Android 実装の式へ移植できる」粒度で記載している。`add()`/`sub()`/
  `div()`/`mul()` は左結合で評価される（裸の `add()` は直前の項を加算）。
- `userN` 参照は **メニュー固有の実行時供給値** を意味する。どの値が入るかは §30.5 と
  各メニューの実装章を参照。§32 は「どのウィジェットがどの `userN` を読むか」の確定情報。
- `template` 行はプレハブ定義（`template` trait）で、実体行はこの親を継承して生成される。
- 重複回避のため、QuickKeysMenu は §31.3、ClassMenu は §32.3 を正とする
  （`class_menu.xml` と `chargen\class_menu.xml` は同一メニューの重複記述）。

### 32.1 Message / prompt 系メニュー

#### Messagemenu (`message_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| background | image |  | 0 | 0 | 0 | 712 | box_bottom.y sub() add() | 2 | 200 | Menus\Message\background.dds |
| background\Text | text | 2 | 1 | parent().x add(25) | parent().y add(50) |  |  |  | 5 | Text |
| background\box_top | image |  | 1 | background.x add(28) | text.y add() add(2) | 674 | 113 |  | 2 | Menus\Message\frame_top.dds |
| background\first | text | 4 | 1 | me()._center_x sub(2) | box_top.y add(28) |  |  |  | 5 | First |
| background\button_template | template |  | 1 |  |  |  |  |  |  |  |
| background\button_template\button | text | 5 | 2 | first._center_x sub(2) | last().y add() add(10) |  |  |  | 5 | X |
| background\box_bottom | image |  | 1 | parent().x add(6) | first.y add(10) sub(76) | 672 | 111 |  | 2 | Menus\Message\frame_bottom.dds |
| background\clutter | image |  | 1 | add() add(10) | add() add(10) | me()._RandImage me()._Width_ | me()._RandImage me()._Height_ |  | 1 | me()._RandImage me()._Filename_ |
| background\FocusBox | button | 3 | 1 |  |  |  |  | false | 3 |  |
| background\background_bottom | image |  | 1 | parent().x | parent().y add() sub(180) | 712 | 195 |  | 1 | Menus\Message\background_bottom.dds |

#### QuestAddedMenu (`quest_added.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| qadd_background | rect | generic | 0 | screen().width div(2) sub(2) | screen().height div(2) sub(2) |  |  |  | 1 |  |
| qadd_background\qadd_quest_message | text |  | 1 | parent().width div(2) | 10 |  |  |  | 3 | QuestAddedMenu.user0 |
| qadd_background\qadd_divider | image |  | 1 | parent().width sub() div(2) | 55 | 636 | 3 |  | 3 | Menus\Stats\stat_border_horizontal_1.dds |
| qadd_background\qadd_quest_name | text |  | 1 | 80 | 72 |  |  |  | 3 | QuestAddedMenu.user1 |
| qadd_background\qadd_quest_date | text |  | 1 | 80 | 105 |  |  |  | 3 | QuestAddedMenu.user2 |
| qadd_background\map_item_body | text |  | 1 | 10 | 145 |  |  |  | 3 | QuestAddedMenu.user3 |
| qadd_background\map_log_item_icon | image |  | 1 | 10 | 70 | 64 | 64 |  | 3 | QuestAddedMenu.user4 |
| qadd_background\qadd_button | rect | 1 | 1 | parent().width sub() div(2) | map_item_body.y add() add(35) |  |  |  | 6 |  |
| qadd_background\qadd_button_2 | rect | 2 | 1 | parent().width sub() div(2) | qadd_button.height add(20) add() |  |  | QuestAddedMenu.user6 | 6 |  |

#### SkillPerkMenu (`skill_perk.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| skillperk_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  | true | 50 |  |
| skillperk_background\skill_icon | image |  | 1 | 40 | parent().height sub() div(2) | 320 | 430 |  | 3 | SkillPerkMenu.user0 |
| skillperk_background\skillperk_description | text |  | 1 | skill_icon.x add() sub(24) | skill_icon.y |  |  | true | 3 | SkillPerkMenu.user1 |
| skillperk_background\skillperk_ok_button | image | 1 | 1 | skillperk_description.x add(2) | skill_icon.y add() sub() |  |  |  | 3 |  |

#### GenericTestMenu (`test_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| background | image |  | 0 | 0 | 0 | 716 | box_bottom.y sub() add() | 2 | 200 | Menus\Message\msg_background.dds |
| background\display_text | text | 1 | 1 | 25 | 50 |  |  |  | 5 | TEXT |
| background\box_top | image |  | 1 | background.x add(28) | text.y add() add(2) | 675 | 114 |  | 2 | Menus\Message\msg_frame_top.dds |
| background\first | rect |  | 1 | parent().width div(2) | box_top.y add(35) |  |  |  | 5 |  |
| background\first\first_text | text |  | 2 | 0 | 0 |  |  |  | 1 | me()._final_value |
| background\first\first_minus | text | 100 | 2 | 0 sub() sub(70) | -12 |  |  |  | 1 | Less |
| background\first\first_plus | text | 2 | 2 | first_text.x add(70) | 0 |  |  |  | 1 | More |
| background\second | rect |  | 1 | parent().width div(2) | first.y add(70) |  |  |  | 5 |  |
| background\second\second_text | text |  | 2 | 0 | 0 |  |  |  | 1 | me()._final_value |
| background\second\second_minus | text | 100 | 2 | 0 sub() sub(70) | -12 |  |  |  | 1 | Less |
| background\second\second_plus | text | 3 | 2 | second_text.x add(70) | 0 |  |  |  | 1 | More |
| background\okay | text | 99 | 1 | parent().width sub() sub(40) | parent().height sub() sub(40) |  |  |  | 5 | Okay |
| background\box_bottom | image |  | 1 | parent().x add(6) | first.y add(100) | 673 | 109 |  | 2 | Menus\Message\msg_frame_bottom.dds |
| background\clutter | image |  | 1 | add() add(10) | add() add(10) | me()._RandImage me()._Width_ | me()._RandImage me()._Height_ |  | 1 | me()._RandImage me()._Filename_ |
| background\background_bottom | image |  | 1 | parent().x | parent().y add() sub(180) | 711 | 192 |  | 1 | Menus\Message\msg_background_bottom.dds |

### 32.2 クラフト / ダイアログ系メニュー

#### DialogMenu (`dialog_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| dialog_hint_a | rect |  | 0 | screen().width sub() sub() | dialog_name.y sub() add(20) |  |  | me()._value | 5 |  |
| dialog_hint_a\dialog_hint_a_icon | image |  | 1 | 0 sub() sub(60) | dialog_hint_a_text.y sub(6) add(2) | 64 | 64 |  |  | Menus\Shared\all_xbox_buttons.dds |
| dialog_hint_a\dialog_hint_a_text | text |  | 1 | 0 | 0 |  |  |  | 1 | parent()._value me()._text_ |
| dialog_name | text |  | 0 | screen().width sub() sub() | screen().height sub() sub() sub() add(5) |  |  |  |  | DialogMenu.user4 |
| dialog_background | rect | -1 | 0 |  |  | screen().width | screen().height |  |  |  |
| dialog_topics | rect | 6 | 0 | screen().cropx screen().cropx add(50) | screen().height sub() sub() sub() sub() | screen().width sub() sub() sub(80) | DialogMenu._line_height mul(5) | false | 1 |  |
| dialog_topics\dialog_button_layout | rect |  | 1 | 35 | 2 |  | parent().height |  |  |  |
| dialog_topics\dialog_button_layout\dialog_persuade | image | 7 | 2 | dialog_scrollbar.width add(15) | parent().height add(4) | 50 | 50 |  | 1 | Menus\Dialog\dialog_button_persuasion.dds |
| dialog_topics\dialog_button_layout\dialog_persuade\dialog_persuade_over | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_barter | image | 8 | 2 | dialog_persuade.width add(25) add() | parent().height add(4) | 50 | 50 | false | 1 | Menus\Dialog\dialog_button_barter.dds |
| dialog_topics\dialog_button_layout\dialog_barter\dialog_barter_over | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_spell_buy | image | 16 | 2 | dialog_barter.width add(25) add() | parent().height add(4) | 50 | 50 | false | 1 | Menus\Dialog\dialog_button_spell_bartering.dds |
| dialog_topics\dialog_button_layout\dialog_spell_buy\dialog_spell_buy_over | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_repair | image | 12 | 2 | dialog_spell_buy.width add(25) add() | parent().height add(4) | 50 | 50 | false | 1 | Menus\Dialog\dialog_button_repairing.dds |
| dialog_topics\dialog_button_layout\dialog_repair\dialog_repair_over | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_training | image | 9 | 2 | dialog_repair.width add(25) add() | parent().height add(4) | 50 | 50 | false | 1 | Menus\Dialog\dialog_button_training.dds |
| dialog_topics\dialog_button_layout\dialog_training\dialog_training_over | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_recharge | image | 13 | 2 | dialog_training.width add(25) add() | parent().height add(4) | 50 | 50 | false | 1 | Menus\Dialog\dialog_button_recharging.dds |
| dialog_topics\dialog_button_layout\dialog_recharge\dialog_recharge_image | image |  | 3 |  |  | 55 | 56 |  |  | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_goodbye | image | 3 | 2 | dialog_recharge.width add(25) add() | parent().height add(4) | 55 | 56 |  | 1 | xbox me()._filename_ |
| dialog_topics\dialog_button_layout\dialog_goodbye\dialog_goodbye_over | image |  | 3 |  |  | 50 | 50 |  |  | Menus\Dialog\dialog_button_goodbye_hit.dds |
| dialog_topics\dialog_xbox_goodbye | rect | 10 | 1 |  |  |  |  | dialog_goodbye.visible |  |  |
| dialog_topics\dialog_scroll_bar | image | 14 | 1 | 0 | 0 | 16 | parent().height |  |  |  |
| dialog_topics\dialog_topics_layout | rect |  | 1 | dialog_scroll_bar.width add(30) | 0 | parent().width | parent().height |  |  |  |
| dialog_topics\dialog_topics_layout\dialog_topics_scroll_pane | rect | 1 | 2 | 5 | 0 sub() mul() | parent().width | me().childcount mul() |  |  |  |
| topic_template | template |  | 0 |  |  |  |  |  |  |  |
| topic_template\topic_text_layout | rect | 99 | 1 | 0 | me().listindex mul() | child(topic_text).width | DialogMenu._line_height |  |  | strings()._topic |
| topic_template\topic_text_layout\topic_text | text |  | 2 | 0 | 2 |  |  |  | 4 | parent().string |
| dialog_text_layout | rect | 4 | 0 | 0 | 0 | screen().width | screen().height | dialog_topics.visible | 20 |  |
| dialog_text_layout\dialog_text | text | 2 | 1 | screen().width div(2) | screen().height sub(2) sub(60) sub() |  |  |  | -2 | strings()._havenogreeting |
| dialog_focus_box | rect | 5 | 0 |  |  | 1053 |  |  |  |  |
| dialog_focus_box\dialog_focus_left | image |  | 1 | -6 | -8 | parent().width sub(42) max(85) | 64 |  | -1 | Menus\Dialog\dialog_selection_full.dds |
| dialog_focus_box\dialog_focus_right | image |  | 1 | dialog_focus_left.width sub(44) | -8 | 192 | 64 |  | 0 | Menus\Dialog\dialog_selection_cut.dds |

#### TextEditMenu (`texteditmenu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| textedit_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  |  | 3 |  |
| textedit_background\textedit_prompt | text |  | 1 | parent().width sub() div(2) | 8 |  |  |  | 3 | TextEditMenu.user0 |
| textedit_background\textedit_text | text | 1 | 1 | parent().width sub(185) div(2) | textedit_prompt.y add() add(16) |  |  |  | 3 |  |
| textedit_background\textedit_button_back | image | 3 | 1 | textedit_button_ok.x sub() sub(16) | textedit_button_ok.y |  |  |  | 3 |  |
| textedit_background\textedit_button_ok | image | 2 | 1 | parent().width sub() sub(16) | parent().height sub() sub(16) |  |  |  | 3 |  |

#### SpellMakingMenu (`spellmaking.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| spell_focus_box | rect | 8 | 0 |  |  |  |  | false |  |  |
| spell_background | image |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1033 | 788 |  | 1 | Menus\Spellmaking\Spellmaking.dds |
| spell_background\spell_name_background | rect | 2 | 1 | 180 | 153 | 655 | 42 |  | 1 |  |
| spell_background\spell_name_background\spell_name_text | text | 20 | 2 | 15 | 5 |  |  |  | 1 | strings()._spellnameenter |
| spell_background\spell_icon_layout | rect |  | 1 | parent().width sub() div(2) sub(17) | 203 | child(spell_player_gold_text).x |  |  |  |  |
| spell_background\spell_icon_layout\spell_cast_cost_icon | image |  | 2 | 0 | 0 | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| spell_background\spell_icon_layout\spell_cast_text | text |  | 2 | spell_cast_cost_icon.x add() add(10) | 2 |  |  |  | 1 | SpellmakingMenu.user4 |
| spell_background\spell_icon_layout\gold_icon | image |  | 2 | parent().width sub(87) div(2) | 0 | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| spell_background\spell_icon_layout\gold_icon\gold_icon_arrow | image |  | 3 | 23 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| spell_background\spell_icon_layout\gold_icon\spell_gold_cost_text | text |  | 3 | 55 | 2 |  |  |  | 1 | SpellmakingMenu.user5 |
| spell_background\spell_icon_layout\player_gold_icon | image |  | 2 | spell_player_gold_text.x sub() sub() sub(10) | 0 | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| spell_background\spell_icon_layout\spell_player_gold_text | text |  | 2 | 608 | 2 |  |  |  | 1 | SpellMakingMenu.user7 |
| spell_background\known_effect_text | text |  | 1 | 130 | 250 |  |  |  | 1 | strings()._knowneffects |
| spell_background\spell_known_effect_window | rect |  | 1 | 130 | spell_added_effects_window.y | 330 | spell_added_effects_window.height |  | 8 |  |
| spell_background\spell_known_effect_window\spell_known_effects_scroll_bar | image | 9 | 2 | parent().width sub() add(10) | 32 |  | parent().height sub(64) |  | 20 |  |
| spell_background\spell_known_effect_window\spell_known_effects_pane | rect | 6 | 2 | 0 | 0 sub() mul(60) | parent().width | me().childcount mul(60) |  |  |  |
| spell_background\spell_added_effects_text | text |  | 1 | known_effect_text.x add(367) | known_effect_text.y |  |  |  | 1 | strings()._addedeffects |
| spell_background\spell_added_effects_window | rect |  | 1 | 492 | 290 | 400 | 300 |  | 8 |  |
| spell_background\spell_added_effects_window\spell_added_effects_scroll_bar | image | 11 | 2 | parent().width sub() add(10) | 32 |  | parent().height sub(64) |  | 20 |  |
| spell_background\spell_added_effects_window\spell_added_effects_pane | rect | 7 | 2 | 0 | 0 sub() mul(60) | parent().width | me().childcount mul(60) |  |  |  |
| spell_background\spell_require_effect_text | text |  | 1 | known_effect_text.x | 600 |  |  | SpellmakingMenu.user6 | 1 | SpellmakingMenu.user3 |
| spell_background\spell_button_create | image | 14 | 1 | parent().width sub() sub() sub(20) div(2) | 650 |  |  |  |  |  |
| spell_background\spell_button_exit | image | 15 | 1 | spell_button_create.x add() add(20) | spell_button_create.y |  |  |  |  |  |
| known_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| known_effect_template\Effects_known | rect |  | 1 | 8 | me().listindex | 300 | 60 | me().y | 1 |  |
| known_effect_template\Effects_known\spell_cast_cost_icon | image |  | 2 | 5 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| known_effect_template\Effects_known\effect_text | text |  | 2 | 50 | parent().height sub() div(2) |  |  |  |  | parent().user3 |
| added_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| added_effect_template\Effects_added | rect |  | 1 | 0 | me().listindex | parent().width | 60 |  | 1 |  |
| added_effect_template\Effects_added\spell_cast_cost_icon | image |  | 2 | 13 | parent().height sub(32) div(2) add(2) | 32 | 32 |  | 1 | parent().user1 |
| added_effect_template\Effects_added\effect_text | text |  | 2 | 50 | parent().height sub() div(2) |  |  |  | 1 | parent().user2 |

#### SpellPurchaseMenu (`spell_purchase.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| inv_FocusBox | rect | 8 | 0 |  |  |  |  | false |  |  |
| spell_background | image | 1 | 0 | 60 | screen().height sub() div(2) | 746 | 799 |  | 0 | Menus\Shared\shared_combo_background_1.dds |
| spell_background\spell_background_2 | image |  | 1 | 700 | 0 | 64 | 799 |  | -15 | Menus\Shared\shared_combo_background_2.dds |
| spell_background\spell_menu_title | text | 9 | 1 | spell_buy_window.x add() sub(2) | spell_cost_icon.y |  |  |  | 3 | strings()._spells |
| spell_background\spell_cost_icon | image | 10 | 1 | spell_buy_window.x add() add(2) | 45 | 32 | 32 |  | 3 | Menus\Shared\all_small_icons.dds |
| spell_background\spell_gold_money_icon | image | 11 | 1 | spell_buy_window.x add() add(2) sub(5) | 45 | 32 | 32 |  | 3 | Menus\Shared\all_small_icons.dds |
| spell_background\spell_gold_money_icon\spell_gold_arrow_icon | image |  | 2 | 23 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| spell_background\spell_buy_list_scroll_bar | image | 4 | 1 | spell_buy_window.x add() | spell_buy_window.y add(32) |  | spell_buy_window.height sub(64) |  | 20 |  |
| spell_background\spell_buy_window | rect |  | 1 | 50 | 80 | 640 | 600 |  |  |  |
| spell_background\spell_buy_window\spell_buy_list | rect | 3 | 2 | 0 | 0 sub() | parent().width | me().childcount mul(75) |  | 2 |  |
| spell_background\spell_buy_window\horiz_grid_top | image |  | 2 | 0 | 0 sub(2) | parent().width | 4 |  | 2 | Menus\Shared\shared_border_horizontal_1.dds |
| spell_background\spell_buy_window\horiz_grid_bottom | image |  | 2 | 0 | parent().height sub(2) | parent().width | 4 |  | horiz_grid_top.depth | Menus\Shared\shared_border_horizontal_3.dds |
| spell_background\spell_buy_window\vert_grid_left | image |  | 2 | 0 sub(2) | 0 | 4 | parent().height |  | horiz_grid_top.depth | Menus\Shared\shared_border_vertical_2.dds |
| spell_background\spell_buy_window\vert_grid_column1 | image |  | 2 | SpellPurchaseMenu.user0 add() add() add() | 0 | 4 | parent().height |  | horiz_grid_top.depth | Menus\Shared\shared_border_vertical_2.dds |
| spell_background\spell_buy_window\vert_grid_column2 | image |  | 2 | vert_grid_column1.x add(2) | 0 | 4 | parent().height |  | horiz_grid_top.depth | Menus\Shared\shared_border_vertical_2.dds |
| spell_background\spell_buy_window\vert_grid_right | image |  | 2 | parent().width sub(2) | 0 | 4 | parent().height |  | horiz_grid_top.depth | Menus\Shared\shared_border_vertical_2.dds |
| spell_background\cont_money_icon | image |  | 1 | 60 | spell_button_haggle.y | 32 | 32 |  | 2 | Menus\Shared\all_small_icons.dds |
| spell_background\player_money_text | text | 7 | 1 | cont_money_icon.x add() add(16) | spell_button_haggle.y |  |  |  | 2 |  |
| spell_background\spell_button_haggle | image | 6 | 1 | spell_button_exit.x sub() sub(40) | spell_button_exit.y |  |  |  | 2 |  |
| spell_background\spell_button_exit | image | 2 | 1 | parent().width sub() sub(50) sub(32) | parent().height sub() sub(50) |  |  |  | 2 |  |
| spell_item_template | template |  | 0 |  |  |  |  |  |  |  |
| spell_item_template\spell_list_item | rect |  | 1 | 0 | me().listindex | spell_buy_window.width | 71 |  | 1 |  |
| spell_item_template\spell_list_item\effect_icon | image |  | 2 | SpellPurchaseMenu.user0 | parent().height sub() div(2) | SpellPurchaseMenu.user1 | SpellPurchaseMenu.user1 |  |  | parent().user1 |
| spell_item_template\spell_list_item\effect_text | text |  | 2 | effect_icon.x add() add() | parent().height sub() div(2) |  |  |  |  | parent().user3 |
| spell_item_template\spell_list_item\spell_cost | text |  | 2 | vert_grid_column1.x add(2) | parent().height sub() div(2) |  |  |  |  | parent().user5 |
| spell_item_template\spell_list_item\spell_gold_cost | text |  | 2 | vert_grid_column2.x add(2) | parent().height sub() div(2) |  |  |  |  | parent().user9 |

#### EnchantmentMenu (`enchantment.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| ench_focus_box | rect | 8 | 0 |  |  |  |  | false |  |  |
| ench_background | image |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1064 | 812 |  | 1 | Menus\Enchanting\enchanting.dds |
| ench_background\ench_name_background | rect | 24 | 1 | 190 | 130 | 660 | 50 |  | 1 |  |
| ench_background\ench_name_background\ench_name_text | text | 2 | 2 | 30 | parent().height sub() div(2) |  |  |  | 1 | strings()._nameenchantment |
| ench_background\ench_soul_cost_icon | image | 4 | 1 | 240 | 90 | 32 | 32 | false | 1 | Menus\Shared\all_small_icons.dds |
| ench_background\ench_soul_cost_text | text |  | 1 | ench_soul_cost_icon.x add() add(15) | ench_soul_cost_icon.y |  |  | ench_soul_cost_icon.visible | 1 | ench_soul_cost_icon.user0 |
| ench_background\ench_uses_icon | image | 3 | 1 | 400 | 90 | 32 | 32 | false | 1 | Menus\Shared\all_small_icons.dds |
| ench_background\ench_uses_text | text |  | 1 | ench_uses_icon.x add() add(15) | ench_uses_icon.y |  |  | ench_uses_icon.visible | 1 | ench_uses_icon.user0 |
| ench_background\ench_gold_cost_icon | image |  | 1 | 560 | ench_soul_cost_icon.y | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| ench_background\ench_gold_cost_icon\ench_gold_arrow_icon | image |  | 2 | 23 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| ench_background\ench_gold_cost_icon1 | image |  | 1 | ench_gold_cost_text.x add() add(50) | ench_soul_cost_icon.y | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| ench_background\ench_gold_cost_text | text | 5 | 1 | ench_gold_cost_icon.x add() add(25) | ench_soul_cost_icon.y add(4) |  |  |  | 1 | 1 |
| ench_background\ench_gold_cost_text2 | text | 13 | 1 | ench_gold_cost_icon1.x add() add(10) | ench_gold_cost_icon1.y add(4) |  |  |  | 1 | 1 |
| ench_background\ench_item_rect | rect | 20 | 1 | 153 | 205 | 325 | 70 |  | 8 |  |
| ench_background\ench_item_rect\ench_item_icon | image | 25 | 2 | 0 | parent().height sub() div(2) | 64 | 64 | false |  |  |
| ench_background\ench_item_rect\ench_item_text | text |  | 2 | 75 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| ench_background\ench_soulgem_rect | rect | 22 | 1 | 515 | 205 | 390 | 70 |  | 8 |  |
| ench_background\ench_soulgem_rect\ench_soulgem_icon | image | 26 | 2 | 2 | parent().height sub() div(2) | 64 | 64 | false |  |  |
| ench_background\ench_soulgem_rect\ench_soulgem_text | text |  | 2 | 75 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| ench_background\ench_soulgem_rect\ench_charge_icon | image |  | 2 | 350 | 8 | 32 | 32 | parent().user1 | 1 | Menus\Shared\all_small_icons.dds |
| ench_background\ench_soulgem_rect\ench_charge_text | text |  | 2 | ench_charge_icon.x add(2) | ench_charge_icon.y add() |  |  | ench_charge_icon.visible |  | parent().user1 |
| ench_background\ench_known_effect_text | text | 16 | 1 | 150 | 295 |  |  |  | 1 | strings()._applicableeffects |
| ench_background\ench_known_effects_scroll_bar | image | 9 | 1 | 460 | ench_known_effect_window.y add(32) |  | ench_known_effect_window.height sub(64) |  | 20 |  |
| ench_background\ench_known_effect_window | rect |  | 1 | 150 | 325 | 330 | 300 |  |  |  |
| ench_background\ench_known_effect_window\ench_known_effect_pane | rect | 6 | 2 | 0 | 0 sub() |  |  |  | 8 |  |
| ench_background\ench_added_effect_text | text | 17 | 1 | 525 | ench_known_effect_text.y |  |  |  | 1 | strings()._addedeffects |
| ench_background\ench_added_effects_scroll_bar | image | 12 | 1 | 893 | ench_added_effect_window.y add(32) |  | ench_added_effect_window.height sub(64) |  | 20 |  |
| ench_background\ench_added_effect_window | rect |  | 1 | 510 | ench_known_effect_window.y | 370 | 300 |  |  |  |
| ench_background\ench_added_effect_window\ench_added_effect_pane | rect | 7 | 2 | 0 | 0 sub() | 330 | 300 |  | 8 |  |
| ench_background\ench_button_create | image | 14 | 1 | 270 | 650 |  |  |  |  |  |
| ench_background\ench_button_exit | image | 15 | 1 | 650 | ench_button_create.y |  |  |  |  |  |
| known_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| known_effect_template\ench_effects_known | rect | 82 | 1 | 0 | me().listindex | 305 | ench_known_effect_window.height div() |  | 1 |  |
| known_effect_template\ench_effects_known\known_effect_icon | image |  | 2 | 5 | parent().height sub() div(2) | 32 | 32 |  | 1 | parent().user1 |
| known_effect_template\ench_effects_known\ench_known_effect_text | text |  | 2 | 50 | parent().height sub() div(2) |  |  |  | 1 | parent().user2 |
| added_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| added_effect_template\ench_effects_added | rect | 83 | 1 | 20 | me().listindex | 350 | 70 |  | 1 |  |
| added_effect_template\ench_effects_added\ench_added_effect_icon | image |  | 2 | 5 | parent().height sub() div(2) | 32 | 32 |  | 1 | parent().user1 |
| added_effect_template\ench_effects_added\ench_added_effect_text | text |  | 2 | 50 | parent().height sub() div(2) | 310 |  |  | 1 | parent().user2 |

#### EffectSettingMenu (`enchantmentsetting_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| effset_focus_box | rect | 1 | 0 |  |  |  |  | false |  |  |
| effset_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  |  | 1 |  |
| effset_background\effset_icon | image | 2 | 1 | parent().width sub() sub() div(2) | 20 | 64 | 64 |  | 3 |  |
| effset_background\effset_name | text | 3 | 1 | effset_icon.x add() | effset_icon.y add(2) |  |  |  | 3 | strings()._effectname |
| effset_background\effset_items_layout | rect |  | 1 | 40 | effset_icon.y add() add(30) | 720 | me()._item_height add() |  | 3 |  |
| effset_background\effset_items_layout\effset_attribute_layout | rect | 31 | 2 | 0 | 0 | parent().width | parent()._item_height | true | 3 |  |
| effset_background\effset_items_layout\effset_attribute_layout\effset_attribute_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_attribute_layout\effset_attribute_floating\effset_attribute_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_attribute_layout\effset_attribute_button | text |  | 3 | 495 | parent().height sub() div(2) |  |  |  | 3 | parent().user0 |
| effset_background\effset_items_layout\effset_attribute_layout\effset_attribute_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | strings()._attribute |
| effset_background\effset_items_layout\effset_skill_layout | rect | 32 | 2 | 0 | parent()._item_height add() | parent().width | parent()._item_height | true | 3 |  |
| effset_background\effset_items_layout\effset_skill_layout\effset_skill_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_skill_layout\effset_skill_floating\effset_skill_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_skill_layout\effset_skill_button | text |  | 3 | 495 | parent().height sub() div(2) |  |  |  | 3 | parent().user0 |
| effset_background\effset_items_layout\effset_skill_layout\effset_skill_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | strings()._skill |
| effset_background\effset_items_layout\effset_range_layout | rect | 33 | 2 | 0 | parent()._item_height add() | parent().width | parent()._item_height | true | 3 |  |
| effset_background\effset_items_layout\effset_range_layout\effset_dummy | rect | 39 | 3 |  |  |  |  |  |  |  |
| effset_background\effset_items_layout\effset_range_layout\effset_range_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_range_layout\effset_range_floating\effset_range_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_range_layout\effset_range_left | image |  | 3 | effset_magnitude_scroll.x sub(50) | parent().height sub() div(2) | 121 | 36 | xbox | 3 | \Menus\Shared\shared_arrow_left.dds |
| effset_background\effset_items_layout\effset_range_layout\effset_range_button | text |  | 3 | 495 | parent().height sub() div(2) |  |  |  | 3 | parent().user0 |
| effset_background\effset_items_layout\effset_range_layout\effset_range_right | image |  | 3 | effset_magnitude_scroll.x add() add(50) sub() | parent().height sub() div(2) | 121 | 36 | xbox | 3 | \Menus\Shared\shared_arrow_right.dds |
| effset_background\effset_items_layout\effset_range_layout\effset_range_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | strings()._range |
| effset_background\effset_items_layout\effset_magnitude_layout | rect | 34 | 2 | 0 | parent()._item_height add() | parent().width | parent()._item_height |  | 3 |  |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_magnitude_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover add() | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_magnitude_floating\effset_magnitude_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_magnitude_scroll | image | 14 | 3 | 350 | parent().height sub() div(2) | 300 |  | parent().visible | 3 |  |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_magnitude_number | text |  | 3 | effset_magnitude_scroll.x sub() sub(70) | parent().height sub() div(2) |  |  |  | 3 | effset_magnitude_scroll.user7 |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_level_number | text |  | 3 | effset_magnitude_scroll.x sub() sub(70) | parent().height sub() div(2) |  |  |  | 3 | parent().user3 |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_lock_level | text |  | 3 | 495 | parent().height sub() div(2) |  |  | parent().user0 | 3 | parent().user1 |
| effset_background\effset_items_layout\effset_magnitude_layout\effset_magnitude_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | parent().user2 |
| effset_background\effset_items_layout\effset_area_layout | rect | 36 | 2 | 0 | parent()._item_height add() | parent().width | parent()._item_height | effset_area_scroll.visible | 3 |  |
| effset_background\effset_items_layout\effset_area_layout\effset_area_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover add() | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_area_layout\effset_area_floating\effset_area_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_area_layout\effset_area_scroll | image | 16 | 3 | 350 | parent().height sub() div(2) | 300 |  |  | 3 |  |
| effset_background\effset_items_layout\effset_area_layout\effset_area_number | text |  | 3 | effset_magnitude_scroll.x sub() sub(70) | parent().height sub() div(2) |  |  |  | 3 | parent().user0 |
| effset_background\effset_items_layout\effset_area_layout\effset_area_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | strings()._area |
| effset_background\effset_items_layout\effset_duration_layout | rect | 38 | 2 | 0 | parent()._item_height add() | parent().width | parent()._item_height | effset_duration_scroll.visible | 3 |  |
| effset_background\effset_items_layout\effset_duration_layout\effset_duration_floating | image |  | 3 | -10 | 0 | parent().width sub(10) | 64 | parent().mouseover add() | 1 | Menus\Dialog\dialog_selection_full.dds |
| effset_background\effset_items_layout\effset_duration_layout\effset_duration_floating\effset_duration_floating_end | image |  | 4 | parent().width add(8) sub() | 0 | 104 | 64 |  | 2 | Menus\Dialog\dialog_selection_cut.dds |
| effset_background\effset_items_layout\effset_duration_layout\effset_duration_scroll | image | 18 | 3 | 350 | parent().height sub() div(2) | 300 |  |  | 3 |  |
| effset_background\effset_items_layout\effset_duration_layout\effset_duration_number | text |  | 3 | effset_duration_scroll.x sub() sub(70) | parent().height sub() div(2) |  |  |  | 3 | effset_duration_scroll.user7 |
| effset_background\effset_items_layout\effset_duration_layout\effset_duration_label | text |  | 3 | 0 | parent().height sub() div(2) |  |  |  | 3 | strings()._duration |
| effset_background\effset_costs_layout | rect |  | 1 | 0 | effset_button_layout.y sub() sub(20) | parent().width | effset_casting_cost_icon.height |  | 3 |  |
| effset_background\effset_costs_layout\effset_casting_cost_icon | image |  | 2 | 50 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| effset_background\effset_costs_layout\effset_casting_cost_icon\effset_casting_cost_text | text |  | 3 | effset_casting_cost_icon.width add(10) | 2 |  |  |  |  | EffectSettingMenu.user1 |
| effset_background\effset_costs_layout\effset_uses_icon | image |  | 2 | 255 | 0 | 32 | 32 | EffectSettingMenu.user3 |  | Menus\Shared\all_small_icons.dds |
| effset_background\effset_costs_layout\effset_uses_icon\effset_uses_text | text |  | 3 | effset_uses_icon.width add(10) | 2 |  |  |  |  | EffectSettingMenu.user3 |
| effset_background\effset_costs_layout\effset_gold_cost_icon | image |  | 2 | 80 add(360) | 0 | 32 | 32 |  | 3 | Menus\Shared\all_small_icons.dds |
| effset_background\effset_costs_layout\effset_gold_cost_icon\effset_gold_cost_arrow | image |  | 3 | 23 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| effset_background\effset_costs_layout\effset_gold_cost_icon\effset_gold_cost_text | text |  | 3 | 55 | 2 |  |  |  |  | EffectSettingMenu.user2 |
| effset_background\effset_costs_layout\effset_player_gold_icon | image |  | 2 | effset_player_gold_text.x sub() sub(42) | effset_gold_cost_text.y | 32 | 32 |  | 3 | Menus\Shared\all_small_icons.dds |
| effset_background\effset_costs_layout\effset_player_gold_text | text |  | 2 | parent().width sub(50) | 2 |  |  |  |  | EffectSettingMenu.user9 |
| effset_background\effset_button_layout | rect |  | 1 | parent().width sub() sub() div(2) | parent().height sub() sub(20) |  | child(effset_button_ok).height max() max() |  | 3 |  |
| effset_background\effset_button_layout\effset_button_ok | image | 21 | 2 | 0 | 0 |  |  |  |  |  |
| effset_background\effset_button_layout\effset_button_delete | image | 22 | 2 | effset_button_ok.width add(20) | 0 |  |  |  |  |  |
| effset_background\effset_button_layout\effset_button_cancel | image | 23 | 2 | effset_button_delete.width add(20) add() | 0 |  |  |  |  |  |

#### AlchemyMenu (`alchemy.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| inv_FocusBox | rect | 13 | 0 |  |  |  |  | false |  |  |
| alchemy_background | image |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1150 | 840 |  | 1 | Menus\Alchemy\alchemy.dds |
| alchemy_background\name_background | rect | 2 | 1 | 210 | 152 | 736 | 40 |  | 1 |  |
| alchemy_background\name_background\name_text | text | 3 | 2 | 8 | parent().height sub() div(2) |  |  |  | 1 |  |
| alchemy_background\mortar_icon | image | 4 | 1 | 348 | 221 | 64 | 64 |  | 1 | Shared\shared_empty.dds |
| alchemy_background\alembic_icon | image | 5 | 1 | retort_icon.x add() add(66) | mortar_icon.y | 64 | 64 | false | 1 | Menus\Shared\shared_empty.dds |
| alchemy_background\calcinator_icon | image | 6 | 1 | alembic_icon.x add() add(66) | mortar_icon.y | 64 | 64 | false | 1 | Menus\Shared\shared_empty.dds |
| alchemy_background\retort_icon | image | 7 | 1 | mortar_icon.x add() add(66) | mortar_icon.y | 64 | 64 | false | 1 | Menus\Shared\shared_empty.dds |
| alchemy_background\ingredient_label | text |  | 1 | ingredient_one.x | 312 |  |  |  | 1 | strings()._ingredients |
| alchemy_background\ingredient_one | rect | 8 | 1 | 210 | 350 | 380 | 64 |  | 1 | strings()._ingredientone |
| alchemy_background\ingredient_one\ingredient_one_icon | image | 18 | 2 | 7 | 0 | 64 | 64 | false | 1 |  |
| alchemy_background\ingredient_one\ingredient_one_icon\ingredient_one_stack_count | text |  | 3 | 72 | 46 |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_one\ingredient_one_text | text |  | 2 | 85 | parent().height sub() div(2) |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_two | rect | 9 | 1 | ingredient_one.x | ingredient_one.y add() add() | ingredient_one.width | ingredient_one.height |  | 1 | strings()._ingredienttwo |
| alchemy_background\ingredient_two\ingredient_two_icon | image | 19 | 2 | ingredient_one_icon.x | ingredient_one_icon.y | ingredient_one_icon.width | ingredient_one_icon.height | false | 1 |  |
| alchemy_background\ingredient_two\ingredient_two_icon\ingredient_two_stack_count | text |  | 3 | 72 | 46 |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_two\ingredient_two_text | text |  | 2 | ingredient_one_text.x | parent().height sub() div(2) |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_three | rect | 10 | 1 | ingredient_one.x | ingredient_two.y add() add() | ingredient_one.width | ingredient_one.height |  | 1 | strings()._ingredientthree |
| alchemy_background\ingredient_three\ingredient_three_icon | image | 20 | 2 | ingredient_one_icon.x | ingredient_one_icon.y | ingredient_one_icon.width | ingredient_one_icon.height | false | 1 |  |
| alchemy_background\ingredient_three\ingredient_three_icon\ingredient_three_stack_count | text |  | 3 | 72 | 46 |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_three\ingredient_three_text | text |  | 2 | ingredient_one_text.x | parent().height sub() div(2) |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_four | rect | 11 | 1 | ingredient_one.x | ingredient_three.y add() add() | ingredient_one.width | ingredient_one.height |  | 1 | strings()._ingredientfour |
| alchemy_background\ingredient_four\ingredient_four_icon | image | 21 | 2 | ingredient_one_icon.x | ingredient_one_icon.y | ingredient_one_icon.width | ingredient_one_icon.height | false | 1 |  |
| alchemy_background\ingredient_four\ingredient_four_icon\ingredient_four_stack_count | text |  | 3 | 72 | 46 |  |  |  | 1 | parent().user0 |
| alchemy_background\ingredient_four\ingredient_four_text | text |  | 2 | ingredient_one_text.x | parent().height sub() div(2) |  |  |  | 1 | parent().user0 |
| alchemy_background\effects_label | text |  | 1 | effect_list.x | 312 |  |  |  | 1 | strings()._effects |
| alchemy_background\effect_list_scroll_bar | image | 16 | 1 | effect_list.x add() | effect_list.y add(32) |  | effect_list.height sub(64) |  | 20 |  |
| alchemy_background\effect_list | rect | 12 | 1 | 620 | ingredient_one.y | 308 | ingredient_one.height add() |  | 8 |  |
| alchemy_background\alchemy_button_create | image | 14 | 1 | parent().width sub() div(2) sub(190) | parent().height |  |  |  | 3 |  |
| alchemy_background\alchemy_button_exit | image | 15 | 1 | parent().width sub() div(2) add(200) | alchemy_button_create.y |  |  |  | 3 |  |
| effect_template | template |  | 0 |  |  |  |  |  |  |  |
| effect_template\Effects | rect |  | 1 | 0 | me().user0 sub() | effect_list.width | 60 | me().y | 1 |  |
| effect_template\Effects\effect_icon | image |  | 2 | 5 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| effect_template\Effects\effect_text | text |  | 2 | 50 | parent().height sub() div(2) |  |  |  |  | parent().user3 |

#### SigilStoneMenu (`sigilstone.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| sigil_focus_box | rect | 8 | 0 |  |  |  |  | false |  |  |
| sigil_background | image | 1 | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1064 | 812 |  | 1 | Menus\Enchanting\enchanting.dds |
| sigil_background\sigil_name_background | rect | 24 | 1 | 190 | 130 | 660 | 50 |  | 1 |  |
| sigil_background\sigil_name_background\sigil_name_text | text | 2 | 2 | 30 | parent().height sub() div(2) |  |  |  | 1 | strings()._enchanteditem |
| sigil_background\sigil_effect_icon | image | 3 | 1 | 550 | 90 | 32 | 32 | false | 1 |  |
| sigil_background\sigil_soul_cost_icon | image |  | 1 | 240 | 90 | 32 | 32 | SigilStoneMenu.user0 | 1 | Menus\Shared\all_small_icons.dds |
| sigil_background\sigil_soul_cost_text | text |  | 1 | sigil_soul_cost_icon.x add() add(15) | sigil_soul_cost_icon.y |  |  | SigilStoneMenu.user0 | 1 | SigilStoneMenu.user0 |
| sigil_background\sigil_constant_text | text |  | 1 | sigil_soul_cost_text.x | sigil_soul_cost_icon.y |  |  | SigilStoneMenu.user0 | 1 | strings()._constant_effect |
| sigil_background\sigil_uses_icon | image |  | 1 | 400 | 90 | 32 | 32 | SigilStoneMenu.user1 | 1 | Menus\Shared\all_small_icons.dds |
| sigil_background\sigil_uses_text | text |  | 1 | sigil_uses_icon.x add() add(15) | sigil_uses_icon.y |  |  | sigil_uses_icon.visible | 1 | SigilStoneMenu.user1 |
| sigil_background\sigil_item_enchant_rect | rect | 20 | 1 | 153 | 205 | 325 | 70 |  | 8 |  |
| sigil_background\sigil_item_enchant_rect\sigil_item_icon | image | 25 | 2 | 0 | parent().height sub() div(2) | 64 | 64 | false |  |  |
| sigil_background\sigil_item_enchant_rect\sigil_item_text | text |  | 2 | 75 | 37 sub(2) |  |  |  |  | parent().user0 |
| sigil_background\sigil_stone_rect | rect | 22 | 1 | 515 | 203 | 390 | 70 |  | 8 |  |
| sigil_background\sigil_stone_rect\soulgem_icon | image | 26 | 2 | 2 | parent().height sub() div(2) | 64 | 64 | true |  |  |
| sigil_background\sigil_stone_rect\sigil_stone_text | text |  | 2 | soulgem_icon.x add(5) | 37 sub(2) |  |  |  |  | parent().user0 |
| sigil_background\sigil_known_effect_text | text | 16 | 1 | 150 | 295 |  |  |  | 1 | strings()._sigilstoneeffects |
| sigil_background\sigil_known_effect_list_scroll_bar | image | 9 | 1 | 460 | sigil_known_effect_window.y add(32) |  | sigil_known_effect_window.height sub(64) |  | 20 |  |
| sigil_background\sigil_known_effect_window | rect |  | 1 | 140 | 325 | 330 | 300 |  |  |  |
| sigil_background\sigil_known_effect_window\sigil_known_effect_list | rect | 6 | 2 | 0 | 0 sub() |  |  |  | 8 |  |
| sigil_background\sigil_known_effect_list_label | text | 17 | 1 | 525 | sigil_known_effect_text.y |  |  |  | 1 | strings()._effectsapplied |
| sigil_background\sigil_added_effect_list_scroll_bar | rect | 12 | 1 | 893 | sigil_added_effect_window.y add(32) |  | sigil_added_effect_window.height sub(64) |  | 20 |  |
| sigil_background\sigil_added_effect_window | rect |  | 1 | 510 | sigil_known_effect_window.y | 370 | 300 |  |  |  |
| sigil_background\sigil_added_effect_window\sigil_added_effect_list | rect | 7 | 2 | 0 | 0 sub() | 330 | 300 |  | 8 |  |
| sigil_background\sigil_button_create | image | 14 | 1 | 290 | 665 |  |  |  | 5 |  |
| sigil_background\sigil_button_exit | image | 15 | 1 | 630 | sigil_button_create.y |  |  |  | 5 |  |
| known_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| known_effect_template\sigil_known_effect | rect | 82 | 1 | 10 | me().listindex | 305 | sigil_known_effect_window.height div() |  | 1 |  |
| known_effect_template\sigil_known_effect\sigil_known_effect_icon | image |  | 2 | 5 | parent().height sub() div(2) | 32 | 32 |  | 1 | parent().user1 |
| known_effect_template\sigil_known_effect\sigil_known_effect_text | text |  | 2 | 50 | parent().height sub() div(2) |  |  |  | 1 | parent().user2 |
| added_effect_template | template |  | 0 |  |  |  |  |  |  |  |
| added_effect_template\sigil_added_effect | rect | 83 | 1 | 20 | me().listindex | 350 | 70 |  | 1 |  |
| added_effect_template\sigil_added_effect\sigil_added_effect_icon | image |  | 2 | 5 | parent().height sub() div(2) | 32 | 32 |  | 1 | parent().user1 |
| added_effect_template\sigil_added_effect\sigil_known_effect_text | text |  | 2 | 50 | parent().height sub() div(2) | 310 |  |  | 1 | parent().user2 |

#### PersuasionMenu (`persuasion_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| Gradient_25 | nif | 16 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\25.nif |
| Gradient_50 | nif | 17 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\50.nif |
| Gradient_75 | nif | 18 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\75.nif |
| Gradient_100 | nif | 19 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\100.nif |
| Dimmer_1 | nif | 20 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\New_dimer.nif |
| Dimmer_2 | nif | 21 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\New_dimer.nif |
| Dimmer_3 | nif | 22 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\New_dimer.nif |
| Dimmer_4 | nif | 23 | 0 | Gradient_image.x | Gradient_image.y |  |  | true |  | Persuasion\New Persuasion\New_dimer.nif |
| Gradient_image | nif | 1 | 0 | screen().x add(2) sub(125) | screen().y add(2) sub(50) |  |  | true |  | Persuasion\New Persuasion\New_frme_glass_pop_selector.nif |
| persuasion_disposition_text | text | 10 | 0 | Gradient_image.x add(295) | Gradient_image.y add(160) |  |  |  | 20 | 100 |
| persuasion_no_play_text | text | 27 | 0 | persuasion_maxaward_words.x | persuasion_maxaward_words.y |  |  | true |  | strings()._cannotpersuade |
| persuasion_maxaward_words | text |  | 0 | Gradient_image.x add(120) | Gradient_image.y add(470) |  |  |  |  | strings()._maxpersuasion |
| persuasion_maxaward_text | text | 15 | 0 | persuasion_maxaward_words.x add(2) add() add(10) | persuasion_maxaward_words.y |  |  |  | 20 | 100 |
| persuasion_bribe_words | text | 5 | 0 | persuasion_maxaward_words.x | persuasion_maxaward_text.y add(10,persuasion_no_play_text.alpha sub(1)) |  |  |  |  | strings()._bribe |
| persuasion_bribe_colon | text |  | 0 | persuasion_bribe_words.x add() | persuasion_bribe_words.y |  |  |  |  | : |
| persuasion_bribe_amount | text |  | 0 | persuasion_bribe_colon.x add(15) | persuasion_bribe_words.y |  |  |  |  | persuasion_bribe_words.user0 |
| persuasion_bribe_septims | text |  | 0 | persuasion_bribe_amount.x add(0) | persuasion_bribe_words.y |  |  |  |  | strings()._septimsfor |
| persuasion_bribe_disposition_amount | text |  | 0 | persuasion_bribe_septims.x add(15) | persuasion_bribe_words.y |  |  |  |  | persuasion_bribe_words.user1 |
| persuasion_bribe_disposition_words | text |  | 0 | persuasion_bribe_disposition_amount.x add(15) | persuasion_bribe_disposition_amount.y |  |  |  |  | strings()._dispositionbonus |
| persuasion_NPC_name | text | 28 | 0 | screen().width sub(2) sub() sub() | screen().height sub(2) sub() sub() |  |  |  |  |  |
| persuasion_bribe_button | image | 8 | 0 | Gradient_image.x add(112) | Gradient_image.y add(370) |  |  |  |  |  |
| persuasion_rotate_button | image | 24 | 0 | persuasion_bribe_button.x add(200) | persuasion_bribe_button.y |  |  |  |  |  |
| persuasion_start_button | image | 26 | 0 | persuasion_bribe_button.x | persuasion_bribe_button.y add(50) |  |  |  |  |  |
| persuasion_exit_button | image | 9 | 0 | persuasion_rotate_button.x | persuasion_bribe_button.y add(50) |  |  |  |  |  |
| persuasion_reaction_text | text | 11 | 0 | screen().cropx add() | screen().cropy add() |  |  |  | 1 |  |

### 32.3 キャラクター生成系メニュー

#### BirthSignMenu (`birthsign_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| inv_FocusBox | button | 38 | 0 |  |  |  |  | false |  |  |
| BirthSign_background | image | 1 | 0 | screen().width sub() div(2) | screen().height sub() div(2) | 1037 | 818 |  | 1 | Menus\Level_up\level_up_background.dds |
| BirthSign_background\apprentice_Text | text | 4 | 1 | 30 | 30 |  |  |  | 5 | The Apprentice |
| BirthSign_background\BirthSign_Picture | image | 2 | 1 | 350 | 100 | 600 | 700 |  | 10 | Menus\\Menus\BirthSign\load_image_apprentice.dds |
| BirthSign_background\Birth_Text | text | 3 | 1 | 350 | BirthSign_Picture.y add(100) |  |  |  | 5 | BirthSign Description |
| BirthSign_background\done_button | image | 17 | 1 | parent().width sub() div(2) add(400) | parent().width sub() div(2) add(320) | 177 | 43 |  | 31 | Menus\Container\cont_button_long_off.dds |
| BirthSign_background\done_button\Done_button_on | image |  | 2 | 0 | 0 | 176 | 51 |  | 2 | Menus\Container\cont_button_long_on.dds |
| BirthSign_background\done_button\Done_button_text | text |  | 2 | parent().width sub() div(2) sub(2) | 3 |  |  |  | 4 | parent().string |

#### ClassMenu (`class_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| class_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  |  | 1 |  |
| class_background\class_information_text | text |  | 1 | parent().width div(2) | 18 |  |  |  | 3 | strings()._selectclass |
| class_background\class_list_scroll_bar | image | 2 | 1 | 260 | class_list_window.y add() |  | class_list_window.height sub() sub() |  | 20 |  |
| class_background\class_list_window | rect |  | 1 | 10 | parent().height sub() add() div(2) | 250 | 600 |  |  |  |
| class_background\class_list_window\class_list_pane | rect | 1 | 2 | 0 | 0 sub() | parent().width | 10 |  | 8 |  |
| class_background\class_picture | image |  | 1 | class_list_window.x add() add(55) | parent().height sub() div(2) | 368 | 507 |  | 10 | ClassMenu.user21 |
| class_background\class_data_layout | rect |  | 1 | parent().width sub(30) | class_list_window.y |  |  |  | 5 |  |
| class_background\class_data_layout\class_special_label | text |  | 2 | 0 | 16 |  |  |  |  | strings()._specialization |
| class_background\class_data_layout\class_special_text | text |  | 2 | 0 | class_special_label.y add() |  |  |  | 2 | ClassMenu.user8 |
| class_background\class_data_layout\class_attribute_label_1 | text |  | 2 | 0 | class_special_text.y add() add() |  |  |  |  | strings()._favored |
| class_background\class_data_layout\class_attribute_label_2 | text |  | 2 | 0 | class_attribute_label_1.y add() sub(8) |  |  |  |  | strings()._attributes |
| class_background\class_data_layout\class_attribute_1_text | text |  | 2 | 0 | class_attribute_label_2.y add() |  |  |  | 2 | ClassMenu.user9 |
| class_background\class_data_layout\class_attribute_2_text | text |  | 2 | 0 | class_attribute_1_text.y add() add() |  |  |  | 2 | ClassMenu.user10 |
| class_background\class_data_layout\class_major_skills_label | text |  | 2 | 0 | class_attribute_2_text.y add() add() |  |  |  |  | strings()._majorskills |
| class_background\class_data_layout\class_major_skills_1_text | text |  | 2 | 0 | class_major_skills_label.y add() |  |  |  | 2 | ClassMenu.user1 |
| class_background\class_data_layout\class_major_skills_2_text | text |  | 2 | 0 | class_major_skills_1_text.y add() add() |  |  |  | 2 | ClassMenu.user2 |
| class_background\class_data_layout\class_major_skills_3_text | text |  | 2 | 0 | class_major_skills_2_text.y add() add() |  |  |  | 2 | ClassMenu.user3 |
| class_background\class_data_layout\class_major_skills_4_text | text |  | 2 | 0 | class_major_skills_3_text.y add() add() |  |  |  | 2 | ClassMenu.user4 |
| class_background\class_data_layout\class_major_skills_5_text | text |  | 2 | 0 | class_major_skills_4_text.y add() add() |  |  |  | 2 | ClassMenu.user5 |
| class_background\class_data_layout\class_major_skills_6_text | text |  | 2 | 0 | class_major_skills_5_text.y add() add() |  |  |  | 2 | ClassMenu.user6 |
| class_background\class_data_layout\class_major_skills_7_text | text |  | 2 | 0 | class_major_skills_6_text.y add() add() |  |  |  | 2 | ClassMenu.user7 |
| class_background\class_button_create | image | 5 | 1 | class_button_done.x sub() sub(20) sub(40) | class_button_done.y |  |  |  | 5 |  |
| class_background\class_button_done | image | 4 | 1 | parent().width sub() sub(20) add(40) | parent().height sub() sub(20) |  |  |  | 5 |  |
| class_template | template |  | 0 |  |  |  |  |  |  |  |
| class_template\class_template | rect | 99 | 1 | 0 | me().listindex | parent().width | 50 |  | 1 |  |
| class_template\class_template\class_template_name | text |  | 2 | 10 | parent().height sub() div(2) |  |  |  | 8 | parent().user1 |
| class_template\class_template\class_selected | image |  | 2 | 7 | -3 | sibling(class_template_name).width max(85) add(8) | 54 | parent().user2 | 5 mul() add(1) | Menus\Inventory\inv_equiped_marker_1.dds |
| class_template\class_template\class_mouseover_marker | image |  | 2 | 5 | -2 | sibling(class_template_name).width max(85) | 64 |  | 3 | Menus\dialog\dialog_selection_full.dds |
| class_template\class_template\class_mouseover_marker_2 | image |  | 2 | sibling(class_template_name).width max(85) add(24) sub() | -2 | 104 | 64 |  | 4 | dialog\dialog_selection_cut.dds |

#### RaceSexMenu (`race_sex_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| race_background | rect | generic | 0 | 70 | screen().height sub() div(2) |  |  |  | 1 |  |
| race_background\race_player_grab | rect | 2 | 1 | 650 | 0 | 800 | 1000 |  | 1 |  |
| race_background\race_name | text | 10 | 1 | parent().width div(2) | 20 |  |  |  | 5 |  |
| race_background\race_name_marker | image |  | 1 | parent().width sub() div(2) | 10 | sibling(race_name).width add(120) max(85) | 64 |  | 1 | Menus\dialog\dialog_selection_full.dds |
| race_background\race_name_marker\race_name_marker_2 | image |  | 2 | parent().width sub() add(10) | 0 | 104 | 64 |  | 1 | dialog\dialog_selection_cut.dds |
| race_background\race_name_left | image |  | 1 | race_name.x sub(2) sub(20) sub() | 31 | 35 | 17 |  | 5 | Menus\Stats\stat_title_left.dds |
| race_background\race_name_right | image |  | 1 | race_name.width div(2) add() add(20) | 31 | 35 | 17 |  | 5 | Menus\Stats\stat_title_right.dds |
| race_background\race_name_separator | image |  | 1 | 75 | 75 | 400 | 3 |  | 3 | Menus\Stats\stat_border_horizontal_1.dds |
| race_background\race_window | rect | 1 | 1 | 30 | 85 | 500 | 558 |  | 8 |  |
| race_background\race_window\race_scroll_bar | image | 20 | 2 | parent().width sub(20) | 25 |  | parent().height sub(50) |  | 20 |  |
| race_background\race_button_done | image | 90 | 1 | parent().width sub() sub(30) | parent().height sub() sub(20) |  |  |  | 5 |  |
| race_template_pane | template |  | 0 |  |  |  |  |  |  |  |
| race_template_pane\race_pane | rect | 100 | 1 | 0 | 0 sub(93) | parent().width sub(1,40) |  | me().user0 | 2 |  |
| race_template_button | template |  | 0 |  |  |  |  |  |  |  |
| race_template_button\race_template_button_layout | rect | 101 | 1 | 0 | me().listindex mul() | parent().width | 93 |  | 2 |  |
| race_template_button\race_template_button_layout\race_template_button_label | text |  | 2 | 0 | 0 |  |  |  | 2 | parent().user2 |
| race_template_button\race_template_button_layout\race_template_button_text | text |  | 2 | parent().width div(2) | 25 |  |  |  | 4 | parent().user6 |
| race_template_button\race_template_button_layout\race_template_button_marker | image |  | 2 | parent().width sub() div(2) | 15 | sibling(race_template_button_text).width add(30) max(85) | 64 |  | 1 | Menus\dialog\dialog_selection_full.dds |
| race_template_button\race_template_button_layout\race_template_button_marker\race_template_button_marker_2 | image |  | 3 | parent().width sub() add(10) | 0 | 104 | 64 |  | 1 | dialog\dialog_selection_cut.dds |
| race_template_toggle | template |  | 0 |  |  |  |  |  |  |  |
| race_template_toggle\race_template_toggle | rect | 102 | 1 | 0 | me().listindex mul(93) | parent().width | 93 |  |  |  |
| race_template_toggle\race_template_toggle\race_template_toggle_label | text |  | 2 | 0 | 0 |  |  |  | 2 | parent().user2 |
| race_template_toggle\race_template_toggle\race_template_toggle_left | image | generic | 2 | 0 | 35 | 124 | 36 |  | 4 | \Menus\Shared\shared_arrow_left.dds |
| race_template_toggle\race_template_toggle\race_template_toggle_value | text |  | 2 | parent().width div(2) | parent().height sub() div(2) add(5) |  |  |  | 5 | parent().user6 |
| race_template_toggle\race_template_toggle\race_template_toggle_right | image | generic | 2 | parent().width sub() | 35 | 121 | 36 |  | 4 | \Menus\Shared\shared_arrow_right.dds |
| race_template_toggle\race_template_toggle\race_template_toggle_marker | image |  | 2 | -10 | 25 | parent().width | 64 |  | 1 | Menus\dialog\dialog_selection_full.dds |
| race_template_toggle\race_template_toggle\race_template_toggle_marker_2 | image |  | 2 | parent().width add(15) sub() | 25 | 104 | 64 |  | 2 | dialog\dialog_selection_cut.dds |
| race_template_slider | template |  | 0 |  |  |  |  |  |  |  |
| race_template_slider\race_template_slider | rect | 103 | 1 | 0 | me().listindex mul(93) | parent().width | 93 |  |  |  |
| race_template_slider\race_template_slider\race_template_slider_label | text |  | 2 | 0 | 0 |  |  |  | 3 | parent().user2 |
| race_template_slider\race_template_slider\race_template_slider_bar | image | generic | 2 | child(horizontal_scroll_left).width | 45 | parent().width sub() sub() |  |  | 6 |  |
| race_template_slider\race_template_slider\race_template_slider_marker | image |  | 2 | -10 | 25 | parent().width add(10) | 64 |  | 4 | Menus\dialog\dialog_selection_full.dds |
| race_template_slider\race_template_slider\race_template_slider_marker_2 | image |  | 2 | parent().width add(15) sub() | 25 | 104 | 64 |  | 5 | dialog\dialog_selection_cut.dds |
| race_template_text | template |  | 0 |  |  |  |  |  |  |  |
| race_template_text\race_template_text | text | 104 | 1 | 0 | me().listindex mul(93) add(20) |  |  |  | 2 | me().user6 |

#### SkillsMenu (`skills_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| skills_background | rect |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) |  |  |  | 1 |  |
| skills_background\skills_information_text | text |  | 1 | parent().width div(2) | 10 |  |  | SkillsMenu.user4 | 3 | SkillsMenu.user5 |
| skills_background\skills_list_scroll_bar | image | 2 | 1 | skills_list_window.x add() add(10) | skills_list_window.y add(32) |  | skills_list_window.height sub(50) | skills_list_pane.childcount | 20 |  |
| skills_background\skills_list_window | rect |  | 1 | 10 | 75 | 270 | 550 |  |  |  |
| skills_background\skills_list_window\skills_list_pane | rect | 1 | 2 | 0 | 0 sub() | parent().width | 10 |  | 8 |  |
| skills_background\skills_picture | image |  | 1 | 484 add(436) add(350) add(350) | 120 add(141) add(20) add(40) | 305 add(256) add(368) add(512) | 437 add(256) add(507) add(512) |  | 10 | SkillsMenu.user2 |
| skills_background\skill_text | text |  | 1 | skills_picture.width sub() div(2) add() sub(30) | 500 add(420) add(500) add(470) |  |  |  | 5 | SkillsMenu.user1 |
| skills_background\skills_xbutton_done_back | rect | 6 | 1 | -1000 | -1000 |  |  | xbox |  |  |
| skills_background\skills_xbutton_continue | rect | 7 | 1 | -1000 | -1000 |  |  | xbox |  |  |
| skills_background\skills_button_back | image | 5 | 1 | skills_button_done.x sub() sub(10) | parent().height sub() sub(20) |  |  |  | 5 |  |
| skills_background\skills_button_done | image | 4 | 1 | parent().width sub() sub(10) | parent().height sub() sub(20) |  |  |  | 5 |  |
| skills_background\skills_button_b_icon | image |  | 1 | 498 add(688) | skills_button_back.y | 64 | 64 | xbox | 5 | Menus\Icons\icon_xbox_b_large.dds |
| skills_background\skills_back_text | text |  | 1 | skills_button_b_icon.x add() | skills_button_b_icon.y add(2) |  |  | xbox | 5 | strings()._back |
| skills_background\skills_done_text | text |  | 1 | skills_button_b_icon.x add() | skills_button_b_icon.y add(2) |  |  | xbox | 5 | strings()._done |
| skills_background\skills_button_x_icon | image |  | 1 | 664 | skills_button_done.y | 64 | 64 | xbox | 5 | Menus\Icons\icon_xbox_x_large.dds |
| skills_background\skills_continue_text | text |  | 1 | skills_button_x_icon.x add() | skills_button_x_icon.y add(2) |  |  | xbox | 5 | strings()._continue |
| skills_template | template |  | 0 |  |  |  |  |  |  |  |
| skills_template\skills_template | rect | 99 | 1 | 0 | me().listindex | parent().width | 55 |  | 1 |  |
| skills_template\skills_template\skills_template_name | text |  | 2 | 10 | parent().height sub() div(2) |  |  |  | 8 | parent().user1 |
| skills_template\skills_template\skills_selected | image |  | 2 | 7 | -3 | sibling(skills_template_name).width max(85) add(8) | 54 | parent().user3 | 5 mul() add(1) | Menus\Inventory\inv_equiped_marker_1.dds |
| skills_template\skills_template\skills_mouseover_marker | image |  | 2 | 5 | -2 | sibling(skills_template_name).width max(85) | 64 |  | 3 | Menus\dialog\dialog_selection_full.dds |
| skills_template\skills_template\skills_mouseover_marker_2 | image |  | 2 | sibling(skills_template_name).width max(85) add(24) sub() | -2 | 104 | 64 |  | 4 | dialog\dialog_selection_cut.dds |

### 32.4 HUD / 主要 in-game 系メニュー

#### HUDMainMenu (`hud_main_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| player_grab_zone | rect | 91 | 0 | 0 | 0 | screen().width | screen().height |  |  |  |
| magic_icons | rect | 16 | 0 | screen().width sub() sub(24) | 0 add() add(18) |  |  |  |  |  |
| icon_template | template |  | 0 |  |  |  |  |  |  |  |
| icon_template\effect_icon | image |  | 1 | magic_icons.x sub() sub() | magic_icons.y add(me().user0 div()) | magic_icons._icon_width | magic_icons._icon_height |  |  |  |
| icon_template\effect_icon\icon_timer | nif | 17 | 2 | 30 | 0 |  |  | parent().alpha | 1 | Enemy Health Bar\Health_Bar01.NIF |
| hudmain_region | text |  | 0 | hudmain_background.x add() | hudmain_background.y sub() |  |  |  |  | HUDMainMenu.user8 |
| hudmain_background | image | 1 | 0 | screen().cropx add() sub(60) max() add(67) | screen().height sub() sub() sub() | 620 | 70 |  | 55 | Menus\HUD\hud_back.dds |
| hudmain_background\hudmain_frame | image | 8 | 1 | -70 | -5 | 761 | 90 | HUDMainMenu.user3 | 18 | Menus\HUD\hud_frame.dds |
| hudmain_background\hudmain_frame\hudmain_frame_left | image |  | 2 | 22 | 26 | 42 | 35 | xenon | 1 | Menus\HUD\hud_xbox_lt.dds |
| hudmain_background\hudmain_frame\hudmain_frame_right | image |  | 2 | parent().width sub() sub(25) | 26 | 45 | 35 | xenon | 1 | Menus\HUD\hud_xbox_rt.dds |
| hudmain_background\hudmain_statusbars | rect |  | 1 | 0 | 12 |  |  |  |  |  |
| hudmain_background\hudmain_statusbars\hudmain_health_empty | image |  | 2 | 0 | 0 | 189 | 16 |  | 1 | Menus\HUD\hud_ribbon_health_empty.dds |
| hudmain_background\hudmain_statusbars\hudmain_health_empty\hudmain_health_full | image |  | 3 | 11 | 2 | me().user0 max(0) min(1) mul(163) | 11 | me().user0 | 1 | Menus\HUD\hud_ribbon_health_full.dds |
| hudmain_background\hudmain_statusbars\hudmain_health_empty\hudmain_health_edge | image |  | 3 | hudmain_health_full.x add() | 0 | 12 | 14 | true | 2 | Menus\Shared\ribbon_slider.dds |
| hudmain_background\hudmain_statusbars\hudmain_magic_empty | image |  | 2 | 0 | 18 | 189 | 16 |  | 1 | Menus\HUD\hud_ribbon_magic_empty.dds |
| hudmain_background\hudmain_statusbars\hudmain_magic_empty\hudmain_magic_full | image |  | 3 | 11 | 1 | me().user0 max(0) min(1) mul(163) | 11 | me().user0 | 1 | Menus\HUD\hud_ribbon_magic_full.dds |
| hudmain_background\hudmain_statusbars\hudmain_magic_empty\hudmain_magic_edge | image |  | 3 | hudmain_magic_full.x add() | -1 | 12 | 14 | true | 2 | Menus\Shared\ribbon_slider.dds |
| hudmain_background\hudmain_statusbars\hudmain_fatigue_empty | image |  | 2 | 0 | 36 | 189 | 16 |  | 1 | Menus\HUD\hud_ribbon_fatigue_empty.dds |
| hudmain_background\hudmain_statusbars\hudmain_fatigue_empty\hudmain_fatigue_full | image |  | 3 | 11 | 1 | me().user0 max(0) min(1) mul(163) | 11 | me().user0 | 1 | Menus\HUD\hud_ribbon_fatigue_full.dds |
| hudmain_background\hudmain_statusbars\hudmain_fatigue_empty\hudmain_fatigue_edge | image |  | 3 | hudmain_fatigue_full.x add() | 0 | 12 | 14 | true | 2 | Menus\Shared\ribbon_slider.dds |
| hudmain_background\hudmain_statusbars\hudmain_bars_cover | rect |  | 2 | 0 | -15 | 199 | 80 |  | 5 |  |
| hudmain_background\hudmain_Weapon_Icon | image | 5 | 1 | hudmain_health_empty.width add(40) | 5 | 63 | 63 |  | 1 | Menus\Icons\icon_hud_default_weapon.dds |
| hudmain_background\hudmain_Weapon_Icon\hudmain_weapon_status | image |  | 2 | 0 | parent().y add() sub() | 64 sub(32) | 32 add(24) | parent().user0 | 1 | parent().user1 |
| hudmain_background\hudmain_Weapon_Icon\hudmain_weapon_ammo | text |  | 2 | 65 | 44 |  |  | parent().user3 | 3 | parent().user3 |
| hudmain_background\hudmain_Weapon_Icon\hudmain_weapon_ammo_shadow | text |  | 2 | 67 | 46 |  |  | sibling(hudmain_weapon_ammo).visible | 2 | parent().user3 |
| hudmain_background\hudmain_Weapon_Icon\hudmain_weapon_cover | rect | 10 | 2 | -4 | -8 | 72 | 80 |  | 5 |  |
| hudmain_background\hudmain_Magic_Icon | image | 6 | 1 | hudmain_Weapon_Icon.x add() add(32) | 5 | 63 | 63 |  | 1 | Menus\Icons\icon_hud_default_magic.dds |
| hudmain_background\hudmain_Magic_Icon\hudmain_magic_cover | rect | 11 | 2 | -4 | -8 | 72 | 80 |  | 5 |  |
| hudmain_background\hudmain_compass_layout | rect |  | 1 | hudmain_Magic_Icon.x add() add(20) add(1,20) | 5 | 213 | 87 |  |  |  |
| hudmain_background\hudmain_compass_layout\hudmain_compass_frame | image |  | 2 | -14 | -10 | 220 | 84 |  | hudmain_compass_heading.user1 add(5) | Menus\HUD\hud_compass_frame.dds |
| hudmain_background\hudmain_compass_layout\hudmain_compass_back | image |  | 2 | -5 | -2 | 200 | 68 |  | 0 | Menus\HUD\hud_back.dds |
| hudmain_background\hudmain_compass_layout\hudmain_compass_cover | rect | 12 | 2 | -13 | -8 | 217 | 80 |  | 25 |  |
| hudmain_background\hudmain_compass_layout\hudmain_compass_window | rect | 7 | 2 | 0 | 0 | 200 | 87 |  | 1 |  |
| hudmain_background\hudmain_compass_layout\hudmain_compass_window\hudmain_compass_heading | image | 14 | 3 | parent().user5 sub(2) mul(-1) | 1 | 2048 | 62 |  | 3 | Menus\HUD\hud_compass_face.dds |
| hudmain_background\hudmain_compass_layout\hudmain_compass_window\hudmain_compass_heading\hudmain_compass_icon | template |  | 4 |  |  |  |  |  |  |  |
| hudmain_background\hudmain_compass_layout\hudmain_compass_window\hudmain_compass_heading\hudmain_compass_icon\hudmain_compass_icon | image |  | 5 | me()._pos_raw add(768) add(768) min(6) max(4) sub(2) | 1 | 28 add(4) | 32 add(10) |  | 1 | me().user6 add(3) add(1) parent()._filename_ |
| hudmain_background\hudmain_Levelup_Icon | image | 18 | 1 | hudmain_compass_window.x add() add(424) | 6 | 63 | 63 | false | 1 | Menus\Icons\icon_level_up.dds |
| hudmain_background\hudmain_brackets | nif | 20 | 1 | -20 | -12 |  |  | HUDMainMenu.user3 | 37 | hud_brackets\A_B_C_D_seq.NIF |

#### HUDInfoMenu (`hud_info_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| hudinfo_action_icon | image | 11 | 0 | hudinfo_action_text.x sub() sub() | hudinfo_name.y sub() add(8) | 64 | 64 | HUDInfoMenu.user2 |  | Menus\Shared\all_xbox_buttons.dds |
| hudinfo_action_text | text |  | 0 | hudinfo_name.x | hudinfo_action_icon.y add(2) add(6) |  |  | HUDInfoMenu.user2 |  | parent().user0 |
| hudinfo_name | text | 1 | 0 | screen().width sub() sub() | hudinfo_dest.y sub() add(-12) add(20) |  |  |  |  |  |
| hudinfo_dest | text | 9 | 0 | screen().width sub() sub() | screen().height sub() sub() sub(1,me().height) |  |  | false |  |  |
| hudinfo_lock_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_lock_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_lock_icon\hudinfo_lock_text | text | 10 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_value_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_value_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_value_icon\hudinfo_value_text | text | 2 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_weight_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_weight_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_weight_icon\hudinfo_weight_text | text | 3 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_damage_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_damage_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_damage_icon\hudinfo_damage_text | text | 4 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_armor_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_armor_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_armor_icon\hudinfo_armor_text | text | 5 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_health_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_health_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_health_icon\hudinfo_health_text | text | 7 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_uses_icon | image |  | 0 | 0 sub() sub(4) sub() sub(15) add() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_uses_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_uses_icon\hudinfo_uses_text | text | 8 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |
| hudinfo_quality_icon | image |  | 0 | 0 sub() sub(4) sub() add(15) add() sub() sub() | screen().height sub() sub() sub() | 32 | 32 | hudinfo_quality_text.visible |  | Menus\Shared\all_small_icons.dds |
| hudinfo_quality_icon\hudinfo_quality_text | text | 6 | 1 | parent().width add(4) | parent().height sub() div(2) add(6) |  |  | me().user1 |  |  |

#### HUDReticle (`hud_reticle.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| hudreticle_reticle | rect | 1 | 0 |  |  |  |  |  |  |  |
| hudreticle_reticle\hudreticle_reticle_crosshair | image |  | 1 | -18 | -18 | 37 | 37 | parent().user5 |  | Menus\Misc\hud_reticle.dds |
| hudreticle_reticle\hudreticle_reticle_activate | image |  | 1 | -16 add(-16) | -16 add(-16) | 30 add(34) | 32 add(32) | parent().user5 |  | 1 add(1) me()._filename_ |
| hudreticle_enemy_health | nif | 2 | 0 | -1 | -20 |  |  | true |  | Enemy Health Bar\Health_Bar01.NIF |
| hudreticle_sneak | image | 3 | 0 | -30 | -25 | 64 | 64 | true |  | Menus\Icons\icon_small_eye.dds |

#### HUDSubtitleMenu (`hud_subtitle_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| hudsubtitle_text_layout | rect |  | 0 | 0 | 0 |  |  |  | 2 |  |
| hudsubtitle_text_layout\hudsubtitle_text | text | 1 | 1 | screen().width div(2) | screen().height sub(2) sub(115) sub() min() |  |  | false |  |  |
| hudsubtitle_text_layout\hudsubtitle_icon | image |  | 1 | screen().cropx mul(2) | screen().cropy mul(2) | 63 | 63 | hudsubtitle_notice.user2 |  | hudsubtitle_notice.user1 |
| hudsubtitle_text_layout\hudsubtitle_notice | text | 2 | 1 | hudsubtitle_icon.x add() add(8) | hudsubtitle_icon.y |  |  | false |  | strings()._dummynotice |

#### InventoryMenu (`inventory_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| inv_xbox_hints | rect |  | 0 | screen().width sub() sub() sub() | screen().height sub() sub() sub() sub() add(15) | 100 | 100 | xbox | 1 |  |
| inv_xbox_hints\inv_hint_equip | rect | 5 | 1 | 0 | 0 |  |  | InventoryMenu.user18 | 5 |  |
| inv_xbox_hints\inv_hint_drop | rect | 5 | 1 | 0 | me().height |  |  |  | 5 |  |
| inv_xbox_hints\inv_hint_quickkey | rect | 5 | 1 | 0 | inv_hint_drop.y add() |  |  |  | 5 |  |
| inv_xbox_hints\inv_hint_left_stick | rect | 5 | 1 | 0 | inv_hint_quickkey.y add() |  |  |  | 5 |  |
| inv_FocusBox | rect | 9 | 0 |  |  |  |  | false |  |  |
| inv_background | image | generic | 0 | screen().cropx add() sub(60) max() add(7) | screen().height sub() sub() sub() add(3) | 745 | 792 |  | 1 | Menus\Shared\shared_main_background_1.dds |
| inv_background\inv_background_2 | image |  | 1 | 700 | 0 | 54 | 792 |  | -15 | Menus\Shared\shared_main_background_2.dds |
| inv_background\inv_p1 | image |  | 1 | 23 | 542 | 791 | 176 | InventoryMenu.user0 | 2 | Menus\Shared\shared_01_tab.dds |
| inv_background\inv_p2 | image |  | 1 | 23 | 542 | 791 | 176 | InventoryMenu.user0 | 2 | Menus\Shared\shared_02_tab.dds |
| inv_background\inv_p3 | image |  | 1 | 23 | 542 | 791 | 176 | InventoryMenu.user0 | 2 | Menus\Shared\shared_03_tab.dds |
| inv_background\inv_p4 | image |  | 1 | 23 | 542 | 791 | 176 | InventoryMenu.user0 | 2 | Menus\Shared\shared_04_tab.dds |
| inv_background\inv_p5 | image |  | 1 | 23 | 542 | 791 | 176 | InventoryMenu.user0 | 2 | Menus\Shared\shared_05_tab.dds |
| inv_background\inv_border_top | image |  | 1 | 110 | 65 | 596 | 160 |  | 6 | Menus\Inventory\inv_border_top.dds |
| inv_background\inv_border_top\inv_border_middle | image |  | 2 | 0 | 157 | 576 | 249 |  |  | Menus\Inventory\inv_border_middle.dds |
| inv_background\inv_border_top\inv_border_bottom | image |  | 2 | -57 | 393 | 628 | 160 |  |  | Menus\Inventory\inv_border_bottom.dds |
| inv_background\inv_extra_data | rect |  | 1 | me().width sub() div(2) add(40) | 580 | 640 | 32 |  | 6 |  |
| inv_background\inv_extra_data\inv_encumberance_icon | image |  | 2 | 0 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_extra_data\inv_encumberance_text | text |  | 2 | 40 | inv_encumberance_icon.y add(2) |  |  |  |  | InventoryMenu.user6 |
| inv_background\inv_extra_data\inv_encumberance_slash | text |  | 2 | inv_encumberance_text.x add() add(1) | inv_encumberance_text.y |  |  |  |  | / |
| inv_background\inv_extra_data\inv_encumberance_max | text |  | 2 | inv_encumberance_slash.x add() add(1) | inv_encumberance_text.y |  |  |  |  | InventoryMenu.user7 |
| inv_background\inv_extra_data\inv_armor_icon | image |  | 2 | 210 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_extra_data\inv_armor_text | text |  | 2 | 250 | inv_armor_icon.y add(2) |  |  |  |  | InventoryMenu.user8 |
| inv_background\inv_extra_data\inv_money_icon | image |  | 2 | 420 | 0 | 32 | 32 |  | 1 | Menus\Shared\all_small_icons.dds |
| inv_background\inv_extra_data\inv_money_text | text |  | 2 | 460 | inv_money_icon.y add(2) |  |  |  |  | InventoryMenu.user9 |
| inv_background\inv_tabs | rect |  | 1 | 0 | 594 | 730 | 128 |  | 1 |  |
| inv_background\inv_tabs\inv_tabs_prev | rect | 7 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| inv_background\inv_tabs\inv_tabs_next | rect | 8 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| inv_background\inv_tabs\inv_tabs_p1 | rect | 1 | 2 | 28 | 37 | 135 | 85 |  | 7 |  |
| inv_background\inv_tabs\inv_tabs_p1\inv_tabs_p1_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| inv_background\inv_tabs\inv_tabs_p1\inv_tabs_p1_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| inv_background\inv_tabs\inv_tabs_p1\inv_tabs_p1_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| inv_background\inv_tabs\inv_tabs_p2 | rect | 2 | 2 | 165 | 37 | 135 | 85 |  | 7 |  |
| inv_background\inv_tabs\inv_tabs_p2\inv_tabs_p2_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| inv_background\inv_tabs\inv_tabs_p2\inv_tabs_p2_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| inv_background\inv_tabs\inv_tabs_p2\inv_tabs_p2_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| inv_background\inv_tabs\inv_tabs_p3 | rect | 3 | 2 | 304 | 37 | 134 | 85 |  | 7 |  |
| inv_background\inv_tabs\inv_tabs_p3\inv_tabs_p3_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| inv_background\inv_tabs\inv_tabs_p3\inv_tabs_p3_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| inv_background\inv_tabs\inv_tabs_p3\inv_tabs_p3_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| inv_background\inv_tabs\inv_tabs_p4 | rect | 4 | 2 | 443 | 37 | 135 | 85 |  | 7 |  |
| inv_background\inv_tabs\inv_tabs_p4\inv_tabs_p4_icon | image |  | 3 | 37 | 8 | 64 | 64 |  | -1 | Menus\Shared\all_tabs_icons.dds |
| inv_background\inv_tabs\inv_tabs_p4\inv_tabs_p4_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target | -3 | Menus\Shared\general_tab_selection_off.dds |
| inv_background\inv_tabs\inv_tabs_p4\inv_tabs_p4_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| inv_background\inv_tabs\inv_tabs_p5 | rect | 5 | 2 | 578 | 37 | 135 | 85 |  | 7 |  |
| inv_background\inv_tabs\inv_tabs_p5\inv_tabs_p5_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| inv_background\inv_tabs\inv_tabs_p5\inv_tabs_p5_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| inv_background\inv_tabs\inv_tabs_p5\inv_tabs_p5_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| inv_background\inv_headers_top | rect |  | 1 | 40 | -50 |  |  |  |  |  |
| inv_background\inv_headers_top\inv_p123_header_top | rect |  | 2 | 102 | 100 |  |  | InventoryMenu.user0 | 7 |  |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_left | image |  | 3 | inv_p123_header_text.x sub(2) sub(55) | inv_p123_header_text.y add(6) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_right | image |  | 3 | inv_p123_header_text.x add(2) add(20) | inv_p123_header_left.y | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_text | text | 14 | 3 | 130 | 5 |  |  |  |  | parent()._headernum parent()._header_ |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img1 | image | 15 | 3 | 300 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img2 | image | 16 | 3 | 375 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img3a | image | 17 | 3 | 435 | 0 | 32 | 32 | parent()._headernum |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img3b | image | 17 | 3 | 435 | 0 | 32 | 32 | parent()._headernum |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img3c | image | 17 | 3 | 435 | 0 | 32 | 32 | parent()._headernum |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p123_header_top\inv_p123_header_img4 | image | 18 | 3 | 495 | 0 | 32 | 32 | parent()._headernum |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p4p5_header_top | rect | 13 | 2 | 102 | 100 |  |  | InventoryMenu.user0 | 7 |  |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_left | image |  | 3 | inv_p4p5_header_text.x sub(2) sub(55) | inv_p4p5_header_text.y add(6) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_right | image |  | 3 | inv_p4p5_header_text.x add(2) add(20) | inv_p4p5_header_left.y | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_text | text | 14 | 3 | 130 | 5 |  |  |  |  | parent()._headernum parent()._header_ |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_img1 | image | 15 | 3 | 300 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_img2 | image | 16 | 3 | 375 | 0 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_headers_top\inv_p4p5_header_top\inv_p4p5_header_img3 | image | 17 | 3 | 435 | 0 | 32 | 32 | parent()._headernum |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_scroll_bar | image | 11 | 1 | 690 | inv_window.y |  | inv_window.height |  | 12 |  |
| inv_background\inv_window | rect |  | 1 | 40 | 90 | 645 | 486 |  | 10 |  |
| inv_background\inv_window\inv_pane | rect |  | 2 | 0 | inv_scroll_bar.user7 mul(-81) add(-8) |  |  |  |  |  |
| inv_background\inv_window\inv_pane\inv_weapon_header | rect |  | 3 | 102 | InventoryMenu.user1 |  |  | InventoryMenu.user1 | 3 |  |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_left | image |  | 4 | inv_weapon_header_text.x sub(2) sub(55) | inv_weapon_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_right | image |  | 4 | inv_weapon_header_text.width div(2) add() add(20) | inv_weapon_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._weapons |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_img | image |  | 4 | 435 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_weapon_header\inv_weapon_header_img | image |  | 4 | 495 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header | rect |  | 3 | 102 | InventoryMenu.user2 |  |  | InventoryMenu.user2 | 3 |  |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_left | image |  | 4 | inv_apparel_header_text.x sub(2) sub(55) | inv_apparel_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_right | image |  | 4 | inv_apparel_header_text.width div(2) add() add(20) | inv_apparel_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._apparel |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_img | image |  | 4 | 435 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparel_header\inv_apparel_header_img | image |  | 4 | 495 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparatus_header | rect |  | 3 | 102 | InventoryMenu.user3 |  |  | InventoryMenu.user3 | 3 |  |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_left | image |  | 4 | inv_apparatus_header_text.x sub(2) sub(55) | inv_apparatus_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_right | image |  | 4 | inv_apparatus_header_text.width div(2) add() add(20) | inv_apparatus_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._apparatuses |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_apparatus_header\inv_apparatus_header_img | image |  | 4 | 435 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_potions_header | rect |  | 3 | 102 | InventoryMenu.user4 add() |  |  | InventoryMenu.user0 | 3 |  |
| inv_background\inv_window\inv_pane\inv_potions_header\inv_potions_header_left | image |  | 4 | inv_potions_header_text.x sub(2) sub(55) | inv_potions_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_potions_header\inv_potions_header_right | image |  | 4 | inv_potions_header_text.width div(2) add() add(20) | inv_potions_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_potions_header\inv_potions_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._potions |
| inv_background\inv_window\inv_pane\inv_potions_header\inv_potions_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_potions_header\inv_potions_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_ingr_header | rect |  | 3 | 102 | InventoryMenu.user13 add() |  |  | InventoryMenu.user0 | 3 |  |
| inv_background\inv_window\inv_pane\inv_ingr_header\inv_ingr_header_left | image |  | 4 | inv_ingr_header_text.x sub(2) sub(55) | inv_ingr_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_ingr_header\inv_ingr_header_right | image |  | 4 | inv_ingr_header_text.width div(2) add() add(20) | inv_ingr_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_ingr_header\inv_ingr_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._ingredients |
| inv_background\inv_window\inv_pane\inv_ingr_header\inv_ingr_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_ingr_header\inv_ingr_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_book_header | rect |  | 3 | 102 | InventoryMenu.user14 |  |  | InventoryMenu.user14 | 3 |  |
| inv_background\inv_window\inv_pane\inv_book_header\inv_book_header_left | image |  | 4 | inv_book_header_text.x sub(2) sub(55) | inv_book_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_book_header\inv_book_header_right | image |  | 4 | inv_book_header_text.width div(2) add() add(20) | inv_book_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_book_header\inv_book_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._books |
| inv_background\inv_window\inv_pane\inv_book_header\inv_book_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_book_header\inv_book_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_keys_header | rect |  | 3 | 102 | InventoryMenu.user15 add() |  |  | InventoryMenu.user0 | 3 |  |
| inv_background\inv_window\inv_pane\inv_keys_header\inv_keys_header_left | image |  | 4 | inv_keys_header_text.x sub(2) sub(55) | inv_keys_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_keys_header\inv_keys_header_right | image |  | 4 | inv_keys_header_text.width div(2) add() add(20) | inv_keys_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_keys_header\inv_keys_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._keys |
| inv_background\inv_window\inv_pane\inv_keys_header\inv_keys_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_keys_header\inv_keys_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_stones_header | rect |  | 3 | 102 | InventoryMenu.user16 add() |  |  | InventoryMenu.user0 | 3 |  |
| inv_background\inv_window\inv_pane\inv_stones_header\inv_stones_header_left | image |  | 4 | inv_stones_header_text.x sub(2) sub(55) | inv_stones_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_stones_header\inv_stones_header_right | image |  | 4 | inv_stones_header_text.width div(2) add() add(20) | inv_stones_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_stones_header\inv_stones_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._stones |
| inv_background\inv_window\inv_pane\inv_stones_header\inv_stones_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_stones_header\inv_stones_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_misc_header | rect |  | 3 | 102 | InventoryMenu.user17 add() |  |  | InventoryMenu.user0 | 3 |  |
| inv_background\inv_window\inv_pane\inv_misc_header\inv_misc_header_left | image |  | 4 | inv_misc_header_text.x sub(2) sub(55) | inv_misc_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| inv_background\inv_window\inv_pane\inv_misc_header\inv_misc_header_right | image |  | 4 | inv_misc_header_text.width div(2) add() add(20) | inv_misc_header_text.y add(5) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| inv_background\inv_window\inv_pane\inv_misc_header\inv_misc_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._misc |
| inv_background\inv_window\inv_pane\inv_misc_header\inv_misc_header_img | image |  | 4 | 300 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_misc_header\inv_misc_header_img | image |  | 4 | 375 | 30 | 32 | 32 |  |  | Menus\Shared\all_small_icons.dds |
| inv_background\inv_window\inv_pane\inv_list_contents | rect | 10 | 3 |  |  |  |  |  |  |  |
| inv_background\inv_list_mouse_blocker | rect | generic | 1 | inv_window.x | inv_window.y add(475) | 650 | 50 |  | 50 |  |
| inv_item_template | template |  | 0 |  |  |  |  |  |  |  |
| inv_item_template\inv_item | rect |  | 1 | 0 | me().listindex | 650 | 81 | me.user8 | 1 |  |
| inv_item_template\inv_item\inv_equiped_marker | image |  | 2 | 7 | 7 | 635 | 64 | parent().user10 | 2 | Menus\Inventory\inv_equiped_marker_1.dds |
| inv_item_template\inv_item\inv_item_icon | image |  | 2 | 5 | 7 | 64 | 64 |  | 3 | parent().user6 |
| inv_item_template\inv_item\inv_item_count | text |  | 2 | 70 | 56 |  |  |  | 5 | parent().user7 |
| inv_item_template\inv_item\inv_item_status | image |  | 2 | inv_item_icon.x | inv_item_icon.y add() sub() | 64 sub(32) | 32 add(25) | parent().user12 | inv_item_icon.depth add(1) | parent().user13 |
| inv_item_template\inv_item\inv_item_name | text |  | 2 | 80 | parent().height sub() div(2) |  |  |  | 3 | parent().user1 |
| inv_item_template\inv_item\inv_item_cost | text |  | 2 | 418 | parent().height sub() div(2) |  |  |  | 3 | parent().user2 |
| inv_item_template\inv_item\inv_item_weight | text |  | 2 | 491 | parent().height sub() div(2) |  |  |  | 3 | parent().user3 |
| inv_item_template\inv_item\inv_item_damage | text |  | 2 | 551 | parent().height sub() div(2) |  |  |  | 3 | parent().user4 |
| inv_item_template\inv_item\inv_item_health | text |  | 2 | 612 | parent().height sub() div(2) |  |  |  | 3 | parent().user5 |

#### MagicMenu (`magic_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| magic_xbox_hints | rect |  | 0 | screen().width sub() sub() sub() | screen().height sub() sub() sub() sub() add(15) | 100 | 100 | xbox | 1 |  |
| magic_xbox_hints\magic_hint_equip | rect | 5 | 1 | 0 | 0 |  |  |  | 5 |  |
| magic_xbox_hints\magic_hint_quickkey | rect | 5 | 1 | 0 | me().height |  |  |  | 5 |  |
| magic_xbox_hints\magic_hint_left_stick | rect | 5 | 1 | 0 | me().height mul(2) |  |  |  | 5 |  |
| magic_FocusBox | rect | 9 | 0 |  |  |  |  | false |  |  |
| magic_background | image | generic | 0 | screen().cropx add() sub(60) max() add(7) | screen().height sub() sub() sub() add(3) | 745 | 792 |  | 1 | Menus\Shared\shared_main_background_1.dds |
| magic_background\magic_background_2 | image |  | 1 | 700 | 0 | 54 | 792 |  | -15 | Menus\Shared\shared_main_background_2.dds |
| magic_background\magic_p1 | image |  | 1 | 23 | 542 | 791 | 176 | MagicMenu.user0 | 4 | Menus\Shared\shared_01_tab.dds |
| magic_background\magic_p2 | image |  | 1 | 23 | 542 | 791 | 176 | MagicMenu.user0 | 4 | Menus\Shared\shared_02_tab.dds |
| magic_background\magic_p3 | image |  | 1 | 23 | 542 | 791 | 176 | MagicMenu.user0 | 4 | Menus\Shared\shared_03_tab.dds |
| magic_background\magic_p4 | image |  | 1 | 23 | 542 | 791 | 176 | MagicMenu.user0 | 4 | Menus\Shared\shared_04_tab.dds |
| magic_background\magic_p5 | image |  | 1 | 23 | 542 | 791 | 176 | MagicMenu.user0 | 4 | Menus\Shared\shared_05_tab.dds |
| magic_background\magic_misc_elements | rect |  | 1 | 110 | 65 | 596 | 160 |  | 6 |  |
| magic_background\magic_misc_elements\magic_line_top | image |  | 2 | -50 | 20 | 636 | 8 |  | 1 | Menus\Shared\shared_border_horizontal_1.dds |
| magic_background\magic_misc_elements\magic_line_bottom | image |  | 2 | -50 | 500 | 630 | 8 |  | 1 | Menus\Shared\shared_border_horizontal_2.dds |
| magic_background\magic_misc_elements\magic_line_left | image |  | 2 | 2 | 8 | 8 | 512 |  | 1 | Menus\Shared\shared_border_vertical_2.dds |
| magic_background\magic_misc_elements\magic_line_center | image |  | 2 | 480 | 8 | 8 | 512 |  | 1 | Menus\Shared\shared_border_vertical_1.dds |
| magic_background\magic_misc_elements\magic_line_right | image |  | 2 | 568 | 8 | 8 | 512 |  | 1 | Menus\Shared\shared_border_vertical_1.dds |
| magic_background\magic_misc_elements\magic_cost_icon | image | 14 | 2 | 514 | -20 | 32 | 32 | MagicMenu.user0 |  | Menus\Shared\all_small_icons.dds |
| magic_background\magic_misc_elements\magic_magnitude_icon | image |  | 2 | 514 | -20 | 32 | 32 | MagicMenu.user0 |  | Menus\Shared\all_small_icons.dds |
| magic_background\magic_effectivness | text |  | 1 | 370 | parent().height sub(213) |  |  |  | 28 | MagicMenu.user6 |
| magic_background\magic_tabs | image |  | 1 | 0 | 594 | 730 | 128 |  | 1 | Menus\Shared\shared_page_tabs.dds |
| magic_background\magic_tabs\magic_tabs_prev | rect | 7 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| magic_background\magic_tabs\magic_tabs_next | rect | 8 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| magic_background\magic_tabs\magic_tabs_p1 | rect | 1 | 2 | 28 | 37 | 135 | 85 |  | 7 |  |
| magic_background\magic_tabs\magic_tabs_p1\magic_tabs_p1_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| magic_background\magic_tabs\magic_tabs_p1\magic_tabs_p1_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| magic_background\magic_tabs\magic_tabs_p1\magic_tabs_p1_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| magic_background\magic_tabs\magic_tabs_p2 | rect | 2 | 2 | 165 | 37 | 135 | 85 |  | 7 |  |
| magic_background\magic_tabs\magic_tabs_p2\magic_tabs_p2_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| magic_background\magic_tabs\magic_tabs_p2\magic_tabs_p2_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| magic_background\magic_tabs\magic_tabs_p2\magic_tabs_p2_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| magic_background\magic_tabs\magic_tabs_p3 | rect | 3 | 2 | 304 | 37 | 135 | 85 |  | 7 |  |
| magic_background\magic_tabs\magic_tabs_p3\magic_tabs_p3_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| magic_background\magic_tabs\magic_tabs_p3\magic_tabs_p3_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| magic_background\magic_tabs\magic_tabs_p3\magic_tabs_p3_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| magic_background\magic_tabs\magic_tabs_p4 | rect | 4 | 2 | 443 | 37 | 135 | 85 |  | 7 |  |
| magic_background\magic_tabs\magic_tabs_p4\magic_tabs_p4_icon | image |  | 3 | 37 | 8 | 64 | 64 |  | -1 | Menus\Shared\all_tabs_icons.dds |
| magic_background\magic_tabs\magic_tabs_p4\magic_tabs_p4_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target | -3 | Menus\Shared\general_tab_selection_off.dds |
| magic_background\magic_tabs\magic_tabs_p4\magic_tabs_p4_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| magic_background\magic_tabs\magic_tabs_p5 | rect | 5 | 2 | 578 | 37 | 135 | 85 |  | 7 |  |
| magic_background\magic_tabs\magic_tabs_p5\magic_tabs_p5_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| magic_background\magic_tabs\magic_tabs_p5\magic_tabs_p5_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| magic_background\magic_tabs\magic_tabs_p5\magic_tabs_p5_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| magic_background\magic_scroll_bar | image | 11 | 1 | 690 | magic_window.y |  | magic_window.height |  | 12 |  |
| magic_background\magic_headers_top | rect |  | 1 | 145 | 45 |  |  |  | 7 |  |
| magic_background\magic_headers_top\magic_p1234_header_top | text | 13 | 2 | 130 | 5 |  |  | MagicMenu.user0 | 7 | parent()._headernum parent()._header_ |
| magic_background\magic_headers_top\magic_active_header_top | text |  | 2 | 130 | 5 |  |  | MagicMenu.user0 | 7 | strings()._activeeffects |
| magic_background\magic_headers_top\magic_header_left | image |  | 2 | magic_p1234_header_top.x sub(2) sub() sub(20) | magic_p1234_header_top.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| magic_background\magic_headers_top\magic_header_right | image |  | 2 | magic_p1234_header_top.x add(2) add(20) | magic_header_left.y | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| magic_background\magic_window | rect |  | 1 | 40 | 90 | 645 | 479 |  | 10 |  |
| magic_background\magic_window\magic_pane | rect |  | 2 | 0 | magic_scroll_bar.user7 mul(-81) add(-8) |  |  |  |  |  |
| magic_background\magic_window\magic_pane\magic_greater_header | rect |  | 3 | 102 | MagicMenu.user1 |  |  | MagicMenu.user1 | 3 |  |
| magic_background\magic_window\magic_pane\magic_greater_header\magic_greater_header_left | image |  | 4 | magic_greater_header_text.x sub(2) sub(55) | magic_greater_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| magic_background\magic_window\magic_pane\magic_greater_header\magic_greater_header_left_right | image |  | 4 | magic_greater_header_text.x add(2) add(20) | magic_greater_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| magic_background\magic_window\magic_pane\magic_greater_header\magic_greater_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._greaterpower |
| magic_background\magic_window\magic_pane\magic_lesser_header | rect |  | 3 | 102 | MagicMenu.user2 |  |  | MagicMenu.user2 | 3 |  |
| magic_background\magic_window\magic_pane\magic_lesser_header\magic_lesser_header_left | image |  | 4 | magic_lesser_header_text.x sub(2) sub(55) | magic_lesser_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| magic_background\magic_window\magic_pane\magic_lesser_header\magic_lesser_header_left_right | image |  | 4 | magic_lesser_header_text.x add(2) add(20) | magic_lesser_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| magic_background\magic_window\magic_pane\magic_lesser_header\magic_lesser_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._lesserpower |
| magic_background\magic_window\magic_pane\magic_spells_header | rect |  | 3 | 102 | MagicMenu.user3 |  |  | MagicMenu.user3 | 3 |  |
| magic_background\magic_window\magic_pane\magic_spells_header\magic_spells_header_left | image |  | 4 | magic_spells_header_text.x sub(2) sub(55) | magic_spells_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| magic_background\magic_window\magic_pane\magic_spells_header\magic_spells_header_left_right | image |  | 4 | magic_spells_header_text.x add(2) add(20) | magic_spells_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| magic_background\magic_window\magic_pane\magic_spells_header\magic_spells_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._spells |
| magic_background\magic_window\magic_pane\magic_scrolls_header | rect |  | 3 | 102 | MagicMenu.user4 |  |  | MagicMenu.user4 | 3 |  |
| magic_background\magic_window\magic_pane\magic_scrolls_header\magic_scrolls_header_left | image |  | 4 | magic_scrolls_header_text.x sub(2) sub(55) | magic_scrolls_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| magic_background\magic_window\magic_pane\magic_scrolls_header\magic_scrolls_header_left_right | image |  | 4 | magic_scrolls_header_text.x add(2) add(20) | magic_scrolls_header_text.y add(7) | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| magic_background\magic_window\magic_pane\magic_scrolls_header\magic_scrolls_header_text | text |  | 4 | 130 | 35 |  |  |  |  | strings()._scrolls |
| magic_background\magic_window\magic_pane\magic_list_contents | rect | 10 | 3 |  |  |  |  |  |  |  |
| magic_item_template | template |  | 0 |  |  |  |  |  |  |  |
| magic_item_template\magic_item | rect |  | 1 | 0 | me().listindex | 650 | 81 | me.user8 | 1 |  |
| magic_item_template\magic_item\magic_equiped_marker | image |  | 2 | 7 | 7 | 635 | 64 | parent().user10 | 2 | Menus\Inventory\inv_equiped_marker_1.dds |
| magic_item_template\magic_item\magic_item_icon | image |  | 2 | 5 | 7 | 64 | 64 |  | 3 | parent().user6 |
| magic_item_template\magic_item\magic_item_name | text |  | 2 | 80 | parent().height sub() div(2) |  |  |  | 3 | parent().user1 |
| magic_item_template\magic_item\magic_item_magnitude | text |  | 2 | 496 | parent().height sub() div(2) |  |  |  | 3 | parent().user3 |
| magic_item_template\magic_item\magic_item_duration | text |  | 2 | 556 | parent().height sub() div(2) |  |  |  | 3 | parent().user4 |
| magic_item_template\magic_item\magic_item_cost | text |  | 2 | 600 | parent().height sub() div(2) |  |  |  | 3 | parent().user5 |

#### MagicPopupMenu (`magic_popup_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| magicpop_background | image | 1 | 0 | parent().user1 sub() | 999999 add() min() | magicpop_effect_1_text.width max() max() max() max() max() max() max() max() add(100) min(430) | magicpop_effect_1.visible add() add() add() add() add() add() add() sub(8) mul() add(25) add() |  | parent().user3 sub(10) | Menus\Magic\magic_popup_background.dds |
| magicpop_background\magicpop_border_ULcorner | image |  | 1 | 0 | 0 | 10 | 10 |  | 1 | Menus\Magic\magic_popup_border_corner.dds |
| magicpop_background\magicpop_border_top | image |  | 1 | magicpop_border_ULcorner.x add() | magicpop_border_ULcorner.y | magicpop_border_URcorner.x sub() sub() | 11 |  | 1 | Menus\Magic\magic_popup_border_edge_horizontal.dds |
| magicpop_background\magicpop_border_URcorner | image |  | 1 | parent().width sub() add(2) | 0 | magicpop_border_ULcorner.width | magicpop_border_ULcorner.height |  | 1 | Menus\Magic\magic_popup_border_corner.dds |
| magicpop_background\magicpop_border_left | image |  | 1 | magicpop_border_ULcorner.x | magicpop_border_ULcorner.y add() | 11 | magicpop_border_LLcorner.y sub() sub() |  | 1 | Menus\Magic\magic_popup_border_edge_vertical.dds |
| magicpop_background\magicpop_border_LLcorner | image |  | 1 | 0 | parent().height sub() add(2) | 10 | 10 |  | 1 | Menus\Magic\magic_popup_border_corner.dds |
| magicpop_background\magicpop_border_bottom | image |  | 1 | magicpop_border_LLcorner.x add() | magicpop_border_LLcorner.y | magicpop_border_LRcorner.x sub() sub() | 11 |  | 1 | Menus\Magic\magic_popup_border_edge_horizontal.dds |
| magicpop_background\magicpop_border_LRcorner | image |  | 1 | parent().width sub() add(2) | parent().height sub() add(2) | magicpop_border_ULcorner.width | magicpop_border_ULcorner.height |  | 1 | Menus\Magic\magic_popup_border_corner.dds |
| magicpop_background\magicpop_border_right | image |  | 1 | magicpop_border_URcorner.x | magicpop_border_URcorner.y add() | 11 | magicpop_border_LRcorner.y sub() sub() |  | 1 | Menus\Magic\magic_popup_border_edge_vertical.dds |
| magicpop_background\magicpop_background_bottom | rect | 21 | 1 | 0 | parent().height sub() | parent().width | magicpop_requires_text.y add() add(16) | false | 1 |  |
| magicpop_background\magicpop_background_bottom\magicpop_divider | image |  | 2 | parent().width sub() div(2) | 0 sub(2) | 320 | 16 |  | 1 | Menus\Magic\magic_popup_divider.dds |
| magicpop_background\magicpop_background_bottom\magicpop_requires_text | text |  | 2 | parent().width sub() div(2) | magicpop_divider.y add() add(8) |  |  |  | 1 | parent().string |
| magicpop_background\magicpop_contents | rect |  | 1 | 0 |  |  |  |  | 1 |  |
| magicpop_background\magicpop_contents\magicpop_effect_1 | rect | 11 | 2 | 15 | 15 add(2) | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 1 |  |
| magicpop_background\magicpop_contents\magicpop_effect_1\magicpop_effect_1_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_1\magicpop_effect_1_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_2 | rect | 12 | 2 | 15 | magicpop_effect_1.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_2\magicpop_effect_2_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_2\magicpop_effect_2_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_2\magicpop_effect_2_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_3 | rect | 13 | 2 | 15 | magicpop_effect_2.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_3\magicpop_effect_3_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_3\magicpop_effect_3_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_3\magicpop_effect_3_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_4 | rect | 14 | 2 | 15 | magicpop_effect_3.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_4\magicpop_effect_4_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_4\magicpop_effect_4_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_4\magicpop_effect_4_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_5 | rect | 15 | 2 | 15 | magicpop_effect_4.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_5\magicpop_effect_5_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_5\magicpop_effect_5_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_5\magicpop_effect_5_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_6 | rect | 16 | 2 | 15 | magicpop_effect_5.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_6\magicpop_effect_6_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_6\magicpop_effect_6_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_6\magicpop_effect_6_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_7 | rect | 17 | 2 | 15 | magicpop_effect_6.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_7\magicpop_effect_7_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_7\magicpop_effect_7_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_7\magicpop_effect_7_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |
| magicpop_background\magicpop_contents\magicpop_effect_8 | rect | 18 | 2 | 15 | magicpop_effect_7.y add() add() | magicpop_background.width sub(40) | MagicPopupMenu._item_height | true | 2 |  |
| magicpop_background\magicpop_contents\magicpop_effect_8\magicpop_effect_8_divider | image |  | 3 | -5 | 0 sub(2) | magicpop_background.width sub(40) | 4 |  |  | Menus\Magic\magic_popup_divider_1.dds |
| magicpop_background\magicpop_contents\magicpop_effect_8\magicpop_effect_8_icon | image |  | 3 | 0 | parent().height sub(32) div(2) add(2) | 32 | 32 |  |  | parent().user1 |
| magicpop_background\magicpop_contents\magicpop_effect_8\magicpop_effect_8_text | text |  | 3 | 45 | parent().height sub() div(2) |  |  |  |  | parent().user0 |

#### MapMenu (`map_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| map_preload_map_icons_1 | image |  | 0 | -100 |  | 1 | 1 |  |  | Menus\Icons\Map\map_icons_all.dds |
| map_preload_map_icons_2 | image |  | 0 | -100 |  | 1 | 1 |  |  | Menus\HUD\hud_compass_goal.dds |
| map_preload_map_icons_3 | image |  | 0 | -100 |  | 1 | 1 |  |  | Menus\Map\world_map_arrow_icon.dds |
| map_xbox_hints | rect |  | 0 | screen().width sub() sub() sub() | screen().height sub() sub() sub() sub() add(15) | 100 | 100 | xbox | 10 |  |
| map_xbox_hints\map_hint_a | rect | generic | 1 | 0 | 0 |  |  | map_world_icon_paper.visible | 5 |  |
| map_xbox_hints\map_hint_x | rect | 5 | 1 | 0 | map_hint_a.height |  |  | map_background.user0 | 5 |  |
| map_xbox_hints\magic_hint_right_stick | rect | 5 | 1 | 0 | map_hint_x.height add() |  |  | map_background.user0 | 5 |  |
| map_xbox_hints\magic_hint_left_stick | rect | 5 | 1 | 0 | magic_hint_right_stick.y add() |  |  |  | 5 |  |
| map_focusBox | rect | 23 | 0 |  |  |  |  | false | 1 |  |
| map_world_icon_paper | image | 42 | 0 | me().user1 add(-4) add(-12) | me().user2 add(-3) | child(map_world_icon_text).width add(30) | 45 | false | me().user0 add(-2) | Menus\Map\world\world_map_paper.dds |
| map_world_icon_paper\map_world_icon_paper_end | image |  | 1 | parent().width sub(19) | 0 | 40 | 45 |  | 1 | Menus\Map\world\world_map_paper_end.dds |
| map_world_icon_paper\map_world_icon_text | text |  | 1 | 32 add(8) | 6 |  |  |  | 2 | parent().string |
| map_background | image | 9 | 0 | screen().cropx add() sub(60) max() add(7) | screen().height sub() sub() sub() add(3) | 745 | 792 |  | 1 | Menus\Shared\shared_main_background_1.dds |
| map_background\map_background_2 | image |  | 1 | 700 | 0 | 54 | 792 |  | -15 | Menus\Shared\shared_main_background_2.dds |
| map_background\map_tabs | image |  | 1 | 0 | 594 | 730 | 128 |  | 1 | Menus\Shared\shared_page_tabs.dds |
| map_background\map_tabs\map_tabs_prev | rect | 7 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| map_background\map_tabs\map_tabs_next | rect | 8 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| map_background\map_tabs\map_tabs_p1 | rect | 1 | 2 | 28 | 37 | 135 | 85 |  | 7 |  |
| map_background\map_tabs\map_tabs_p1\map_tabs_p1_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| map_background\map_tabs\map_tabs_p1\map_tabs_p1_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| map_background\map_tabs\map_tabs_p1\map_tabs_p1_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| map_background\map_tabs\map_tabs_p2 | rect | 2 | 2 | 165 | 37 | 135 | 85 |  | 7 |  |
| map_background\map_tabs\map_tabs_p2\map_tabs_p2_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| map_background\map_tabs\map_tabs_p2\map_tabs_p2_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| map_background\map_tabs\map_tabs_p2\map_tabs_p2_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| map_background\map_tabs\map_tabs_p3 | rect | 3 | 2 | 304 | 37 | 135 | 85 |  | 7 |  |
| map_background\map_tabs\map_tabs_p3\map_tabs_p3_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| map_background\map_tabs\map_tabs_p3\map_tabs_p3_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| map_background\map_tabs\map_tabs_p3\map_tabs_p3_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| map_background\map_tabs\map_tabs_p4 | rect | 4 | 2 | 443 | 37 | 135 | 85 |  | 7 |  |
| map_background\map_tabs\map_tabs_p4\map_tabs_p4_icon | image |  | 3 | 37 | 8 | 64 | 64 |  | -1 | Menus\Shared\all_tabs_icons.dds |
| map_background\map_tabs\map_tabs_p4\map_tabs_p4_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target | -3 | Menus\Shared\general_tab_selection_off.dds |
| map_background\map_tabs\map_tabs_p4\map_tabs_p4_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| map_background\map_tabs\map_tabs_p5 | rect | 5 | 2 | 578 | 37 | 135 | 85 |  | 7 |  |
| map_background\map_tabs\map_tabs_p5\map_tabs_p5_icon | image |  | 3 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| map_background\map_tabs\map_tabs_p5\map_tabs_p5_selection_off | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_off.dds |
| map_background\map_tabs\map_tabs_p5\map_tabs_p5_selection_on | image |  | 3 | 0 | 24 | 136 | 36 | parent().target |  | Menus\Shared\general_tab_selection_on.dds |
| map_background\map_tab_layout | rect |  | 1 | 26 | 20 |  |  |  |  |  |
| map_background\map_tab_layout\map_tabs_prev | rect | 7 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| map_background\map_tab_layout\map_tabs_next | rect | 8 | 2 | -1 | -1 | 1 | 1 |  |  |  |
| map_background\map_tab_layout\map_tab_page_5_target | rect | 5 | 2 | 555 | 603 | 132 | 85 |  | 2 |  |
| map_background\map_tab_layout\map_tab_page_4_target | rect | 4 | 2 | 415 | 603 | 132 | 85 |  | 2 |  |
| map_background\map_tab_layout\map_tab_page_3_target | rect | 3 | 2 | 279 | 603 | 132 | 85 |  | 2 |  |
| map_background\map_tab_layout\map_tab_page_2_target | rect | 2 | 2 | 144 | 603 | 132 | 85 |  | 2 |  |
| map_background\map_tab_layout\map_tab_page_1_target | rect | 1 | 2 | 0 | 603 | 140 | 85 |  | 2 |  |
| map_background\map_page_5 | rect |  | 1 | 26 | 20 | 690 | 691 | me()._is_topmost | 2 |  |
| map_background\map_page_5\map_p5 | image |  | 2 | -3 | 522 | 791 | 176 |  | 2 | Menus\Shared\shared_05_tab.dds |
| map_background\map_page_5\map_page_5_extension | image |  | 2 | 585 | 0 | 567 | 613 |  | 4 | Menus\Shared\page_extention.dds |
| map_background\map_page_4 | rect |  | 1 | 26 | 20 | 690 | 688 | me()._is_topmost | 2 |  |
| map_background\map_page_4\map_p4 | image |  | 2 | -3 | 522 | 791 | 176 |  | 2 | Menus\Shared\shared_04_tab.dds |
| map_background\map_page_4\map_page_4_extension | image |  | 2 | 585 | 0 | 567 | 613 |  | 4 | Menus\Shared\page_extention.dds |
| map_background\map_contents_3_4_5 | rect |  | 1 | 8 | -40 |  |  | map_page_3._is_topmost | 30 |  |
| map_background\map_contents_3_4_5\map_log_lines_top | image |  | 2 | 40 | 162 | 636 | 3 |  | 1 | Menus\Stats\stat_border_horizontal_1.dds |
| map_background\map_contents_3_4_5\map_log_lines_top2 | image |  | 2 | 650 | 162 | 436 | 3 |  | 1 | Menus\Stats\stat_border_horizontal_2.dds |
| map_background\map_contents_3_4_5\map_log_lines_bottom | image |  | 2 | 40 | 622 | 636 | 3 |  | 1 | Menus\Stats\stat_border_horizontal_2.dds |
| map_background\map_contents_3_4_5\map_log_lines_bottom2 | image |  | 2 | 650 | 622 | 436 | 3 |  | 1 | Menus\Stats\stat_border_horizontal_2.dds |
| map_background\map_contents_3_4_5\map_log_scroll_bar | image | 11 | 2 | map_log_window.x add() add(25) | map_log_window.y add(32) |  | map_log_window.height sub(64) |  | 5 |  |
| map_background\map_contents_3_4_5\map_log_window | rect |  | 2 | 25 | 165 | 1060 | 460 |  | 4 |  |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane | rect | 13 | 3 |  | 0 sub() |  | 0 |  |  |  |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template | template |  | 4 |  |  |  |  |  |  |  |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item | rect | 51 | 5 | 0 | me().user5 | 1068 | 100 add() |  | 4 |  |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item\map_log_item_active | image |  | 6 | 10 | 5 | sibling(map_item_name).width max() add() add(15) | 64 | parent().user6 | 1 | Menus\Inventory\inv_equiped_marker_1.dds |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item\map_item_name | text |  | 6 | 80 | 7 |  |  | map_log_back.visible | 2 | parent().user1 |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item\map_item_date | text |  | 6 | 80 | -16 add(40) |  |  |  | 2 | parent().user2 |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item\map_item_body | text |  | 6 | 10 | 76 |  |  |  | 2 | parent().user3 |
| map_background\map_contents_3_4_5\map_log_window\map_log_window_pane\map_item_template\map_item\map_log_item_icon | image |  | 6 | 10 | 5 | 64 | 64 |  | 2 | parent().user4 |
| map_background\map_contents_3_4_5\map_log_window\map_log_mapit | image | 31 | 3 | 860 | -70 |  |  |  | 10 |  |
| map_background\map_contents_3_4_5\map_log_window\map_log_back | image | 32 | 3 | 860 | -70 |  |  |  | 10 |  |
| map_background\map_page_3 | rect |  | 1 | 26 | 20 | 690 | 688 | me()._is_topmost | 2 |  |
| map_background\map_page_3\map_p3 | image |  | 2 | -3 | 522 | 791 | 176 |  | 2 | Menus\Shared\shared_03_tab.dds |
| map_background\map_page_3\map_page_3_extension | image |  | 2 | 585 | 0 | 567 | 613 |  | 4 | Menus\Shared\page_extention.dds |
| map_background\map_page_2 | rect |  | 1 | 26 | 20 | 690 | 688 | me()._is_topmost | 2 |  |
| map_background\map_page_2\map_p2 | image |  | 2 | -3 | 522 | 791 | 176 |  | 2 | Menus\Shared\shared_02_tab.dds |
| map_background\map_page_2\map_page_2_extension | image |  | 2 | 585 | 0 | 567 | 613 |  | 4 | Menus\Shared\page_extention.dds |
| map_background\map_page_2\map_world_layout | rect |  | 2 | 0 | 0 |  |  |  |  |  |
| map_background\map_page_2\map_world_layout\map_default_world_focus | rect | 21 | 3 | 10 | 10 | 3 | 3 |  |  |  |
| map_background\map_page_2\map_world_layout\map_world_map_frame_top | image |  | 3 | 75 | 100 | 1003 | 36 |  | 100 | Menus\Map\map_border_top.dds |
| map_background\map_page_2\map_world_layout\map_world_map_frame_bottom | image |  | 3 | 75 | 544 | 1003 | 36 |  | 100 | Menus\Map\map_border_bottom.dds |
| map_background\map_page_2\map_world_layout\map_world_map_frame_left | image |  | 3 | 26 | 100 | 52 | 481 |  | 99 | Menus\Map\map_border_left.dds |
| map_background\map_page_2\map_world_layout\map_world_map_frame_right | image |  | 3 | 1078 | 100 | 52 | 481 |  | 99 | Menus\Map\map_border_right.dds |
| map_background\map_page_2\map_world_layout\map_world_no_map | text |  | 3 | 580 | 300 |  |  | map_world_map.visible | 8 | strings()._nomap |
| map_background\map_page_2\map_world_layout\map_world_map_window | rect | 63 | 3 | 30 | 110 | MapMenu.user4 | MapMenu.user5 |  | 2 |  |
| map_background\map_page_2\map_world_layout\map_world_map_window\map_world_map_cursor | image | 61 | 4 | me().user0 max() min() | me().user1 max() min() | 42 | 42 | xenon | 50 | Menus\Misc\cursor.dds |
| map_background\map_page_2\map_world_layout\map_world_map_window\map_world_map | image | 41 | 4 | 0 sub() add() | 0 sub() add() | 0 | 0 | true | 4 |  |
| map_background\map_page_1 | rect |  | 1 | 26 | 20 | 690 | 688 | me()._is_topmost | 2 |  |
| map_background\map_page_1\map_p1 | image |  | 2 | -3 | 522 | 791 | 176 |  | 2 | Menus\Shared\shared_01_tab.dds |
| map_background\map_page_1\map_page_1_extension | image |  | 2 | 585 | 0 | 567 | 613 |  | 4 | Menus\Shared\page_extention.dds |
| map_background\map_page_1\map_local_layout | rect |  | 2 | 0 | 0 |  |  | map_page_1._is_topmost |  |  |
| map_background\map_page_1\map_local_layout\map_default_local_focus | rect | 22 | 3 | 10 | 10 | 3 | 3 |  |  |  |
| map_background\map_page_1\map_local_layout\map_local_map_frame_top | image |  | 3 | 75 | 100 | 1003 | 36 |  | 100 | Menus\Map\map_border_top.dds |
| map_background\map_page_1\map_local_layout\map_local_map_frame_bottom | image |  | 3 | 75 | 544 | 1003 | 36 |  | 100 | Menus\Map\map_border_bottom.dds |
| map_background\map_page_1\map_local_layout\map_local_map_frame_left | image |  | 3 | 26 | 100 | 52 | 481 |  | 99 | Menus\Map\map_border_left.dds |
| map_background\map_page_1\map_local_layout\map_local_map_frame_right | image |  | 3 | 1078 | 100 | 52 | 481 |  | 99 | Menus\Map\map_border_right.dds |
| map_background\map_page_1\map_local_layout\map_local_no_map | text |  | 3 | 560 | 300 |  |  | map_local_map.visible | 8 |  |
| map_background\map_page_1\map_local_layout\map_local_map_grab | rect | 45 | 3 | 20 | 100 | MapMenu.user4 | MapMenu.user5 | true | 6 |  |
| map_background\map_page_1\map_local_layout\map_local_map_window | rect |  | 3 | 30 | 110 | MapMenu.user4 | MapMenu.user5 |  | 2 |  |
| map_background\map_page_1\map_local_layout\map_local_map_window\map_local_map_cursor | image | 62 | 4 | me().user0 max() min() | me().user1 max() min() | 42 | 42 | xenon | 40 | Menus\Misc\cursor.dds |
| map_background\map_page_1\map_local_layout\map_local_map_window\map_local_map_layout | rect | 46 | 4 | me().user12 sub() add() sub(14) | me().user13 sub() add() sub(21) | child(map_local_map).user0 | child(map_local_map).user1 |  |  |  |
| map_background\map_page_1\map_local_layout\map_local_map_window\map_local_map_layout\map_local_map | rect | 47 | 5 |  |  |  |  |  | 4 |  |
| map_background\map_page_1\map_local_layout\map_local_map_window\map_local_map_layout\map_local_icons | rect | 48 | 5 |  |  | 680 | 550 |  | 6 |  |
| map_background\map_top_title_layout | rect |  | 1 |  |  |  |  | map_log_back.visible |  |  |
| map_background\map_top_title_layout\map_log_border_left | image |  | 2 | map_log_name.x sub(2) sub(55) | 57 | 35 | 17 |  | 110 | Menus\Shared\shared_decor_blue_left.dds |
| map_background\map_top_title_layout\map_log_border_right | image |  | 2 | map_log_name.width div(2) add() add(20) | 58 | 35 | 17 |  | 130 | Menus\Shared\shared_decor_blue_right.dds |
| map_background\map_top_title_layout\map_log_date | text |  | 2 | map_log_name.x | 80 |  |  |  | 140 | MapMenu.user2 |
| map_background\map_top_title_layout\map_log_name | text |  | 2 | 230 add(370) | 47 |  |  |  | 120 | MapMenu.user1 |
| map_background\map_item_name | text |  | 1 | 370 | 63 |  |  | map_log_back.visible | 140 | MapMenu.user3 |
| map_world_icon | template |  | 0 |  |  |  |  |  |  |  |
| map_world_icon\map_world_icon | image | 43 | 1 | me().user1 sub(17) | me().user2 sub(17) sub(17) | parent()._large_icon_width_adj add(32) add(64) | parent()._large_icon_height_adj add(32) add(64) |  | me()._random_depth max(90) max() add(1) | me().user6 add(3) add(1) parent()._filename_ |
| map_local_icon | template |  | 0 |  |  |  |  |  |  |  |
| map_local_icon\map_local_icon | image | 49 | 1 | me().user1 sub(2) | me().user2 sub(2) | parent()._large_icon_width_adj add(32) add(64) | parent()._large_icon_height_adj add(32) add(64) | true | me()._random_depth max(90) max() add(1) | me().user6 add(3) add(1) parent()._filename_ |
| preload | rect |  | 0 | 10 | -1000 |  |  |  |  |  |
| preload\pre01 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_amulet_of_king.dds |
| preload\pre02 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_arena.dds |
| preload\pre03 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_bruma.dds |
| preload\pre04 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_daedra.dds |
| preload\pre05 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_dark_brotherhood.dds |
| preload\pre06 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_dragonfires.dds |
| preload\pre07 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_fighters_guild.dds |
| preload\pre08 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_imperial_city.dds |
| preload\pre09 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_kvatch.dds |
| preload\pre10 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_leyawiin.dds |
| preload\pre11 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_mages_guild.dds |
| preload\pre12 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_master_trainer.dds |
| preload\pre13 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_miscarcand.dds |
| preload\pre14 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_miscellaneous.dds |
| preload\pre15 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_mythic_dawn.dds |
| preload\pre16 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_oblivion_gate.dds |
| preload\pre17 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_paradise.dds |
| preload\pre18 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_settlements_quest_icon.dds |
| preload\pre19 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_thieves_guild.dds |
| preload\pre20 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_tiber_septim's_armor.dds |
| preload\pre21 | image |  | 1 |  |  |  |  |  |  | Icons\Quest\icon_weynon_priory.dds |

#### StatsMenu (`stats_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| stat_focus_box | rect | 60 | 0 |  |  |  |  | false |  |  |
| stats_xbox_hints | rect |  | 0 | screen().width sub() sub() sub() | screen().height sub() sub() sub() add(15) | 100 | 100 | xbox | 1 |  |
| stats_xbox_hints\stats_hint_left_stick | rect | 5 | 1 | 0 | 0 |  |  |  | 5 |  |
| stat_layout | rect |  | 0 | screen().cropx add() sub(60) max() add(7) | screen().height sub() sub() sub() add(3) |  |  |  |  |  |
| stat_layout\stat_background | image | generic | 1 | 0 | 0 | 745 | 792 |  | 4 | Menus\Shared\shared_main_background_1.dds |
| stat_layout\stat_background\stat_background_2 | image |  | 2 | 700 | 0 | 54 | 792 |  | -15 | Menus\Shared\shared_main_background_2.dds |
| stat_layout\stat_background\stat_p1 | rect | 10 | 2 | 0 | 3 | 730 | 730 | StatsMenu.user0 | 8 |  |
| stat_layout\stat_background\stat_p1\stats_page_1_tabs | image |  | 3 | 23 | 539 | 791 | 176 |  | 0 | Menus\Shared\shared_01_tab.dds |
| stat_layout\stat_background\stat_p1\stat_p1_race_layout | rect | 14 | 3 | 110 | me().user0 mul(76) add(104) | 545 | 70 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_race_layout\stat_p1_race | text |  | 4 | 400 | 37 sub(2) |  |  |  | 3 | stat_p1.user2 |
| stat_layout\stat_background\stat_p1\stat_p1_race_layout\stat_p1_race_label | text |  | 4 | 85 | 21 |  |  |  | 3 | strings()._race |
| stat_layout\stat_background\stat_p1\stat_p1_birthsign_layout | rect | 14 | 3 | 110 | me().user0 mul(76) add(104) | 545 | 70 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_birthsign_layout\stat_p1_birthsign | text |  | 4 | 406 | 37 sub(2) |  |  |  | 3 | stat_p1.user3 |
| stat_layout\stat_background\stat_p1\stat_p1_birthsign_layout\stat_p1_birthsign_label | text |  | 4 | 85 | 21 |  |  |  | 3 | strings()._birthsign |
| stat_layout\stat_background\stat_p1\stat_p1_class_layout | rect | 14 | 3 | 110 | me().user0 mul(76) add(104) | 545 | 70 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_class_layout\stat_p1_class | text |  | 4 | 400 | 37 sub(2) |  |  |  | 3 | stat_p1.user4 |
| stat_layout\stat_background\stat_p1\stat_p1_class_layout\stat_p1_class_label | text |  | 4 | 85 | 21 |  |  |  | 3 | strings()._class |
| stat_layout\stat_background\stat_p1\stat_p1_level_layout | rect | 14 | 3 | 110 | me().user0 mul(76) add(104) | 545 | 70 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_level_layout\stat_p1_level | text |  | 4 | 400 | 37 sub(2) |  |  |  | 3 | stat_p1.user5 |
| stat_layout\stat_background\stat_p1\stat_p1_level_layout\stat_p1_level_label | text |  | 4 | 85 | 21 |  |  |  | 3 | strings()._level |
| stat_layout\stat_background\stat_p1\stat_p1_title_left | image |  | 3 | stat_p1_name.x sub(2) sub(10) sub() | 60 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| stat_layout\stat_background\stat_p1\stat_p1_title_right | image |  | 3 | stat_p1_name.width div(2) add() add(10) | 60 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| stat_layout\stat_background\stat_p1\stat_p1_name | text |  | 3 | 380 | 51 |  |  |  | 7 | parent().user1 |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout | rect | 14 | 3 | 65 | me().user0 mul(74) add(117) | 618 | 67 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout\stat_p1_health_label | text |  | 4 | 24 | 18 |  |  |  | 3 | strings()._health |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout\stat_p1_health_bar | image |  | 4 | 222 | 20 |  |  |  |  |  |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout\stat_p1_health_max | text |  | 4 | 595 | 18 |  |  |  | 3 | stat_p1.user7 |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout\stat_p1_health_div | text |  | 4 | stat_p1_health_max.x sub() | 18 |  |  |  | 3 | / |
| stat_layout\stat_background\stat_p1\stat_p1_health_layout\stat_p1_health_current | text |  | 4 | stat_p1_health_max.x sub() sub() | 18 |  |  |  | 3 | stat_p1.user6 |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout | rect | 14 | 3 | 65 | me().user0 mul(74) add(110) | 618 | 67 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout\stat_p1_magica_label | text |  | 4 | 24 | 18 |  |  |  | 3 | strings()._magicka |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout\stat_p1_magica_bar | image |  | 4 | 222 | 20 |  |  |  |  |  |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout\stat_p1_magica_max | text |  | 4 | 595 | 18 |  |  |  | 3 | stat_p1.user9 |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout\stat_p1_magica_div | text |  | 4 | stat_p1_magica_max.x sub() | 18 |  |  |  | 3 | / |
| stat_layout\stat_background\stat_p1\stat_p1_magica_layout\stat_p1_magica_current | text |  | 4 | stat_p1_magica_max.x sub() sub() | 18 |  |  |  | 3 | stat_p1.user8 |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout | rect | 14 | 3 | 65 | me().user0 mul(74) add(107) | 618 | 67 |  | 5 |  |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout\stat_p1_fatigue_label | text |  | 4 | 24 | 18 |  |  |  | 3 | strings()._fatigue |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout\stat_p1_fatigue_bar | image |  | 4 | 222 | 20 |  |  |  |  |  |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout\stat_p1_fatigue_max | text |  | 4 | 595 | 18 |  |  |  | 3 | stat_p1.user11 |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout\stat_p1_fatigue_div | text |  | 4 | stat_p1_fatigue_max.x sub() | 18 |  |  |  | 3 | / |
| stat_layout\stat_background\stat_p1\stat_p1_fatigue_layout\stat_p1_fatigue_current | text |  | 4 | stat_p1_fatigue_max.x sub() sub() | 18 |  |  |  | 3 | stat_p1.user10 |
| stat_layout\stat_background\stat_p1\stat_p1_lines | image |  | 3 | 45 | 85 | 659 | 539 |  | 2 | Menus\Stats\stat_player_lines.dds |
| stat_layout\stat_background\stat_p1\stat_p1_icons | image |  | 3 | 121 | 107 | 64 | 292 |  | 7 | Menus\Stats\stat_player_icons.dds |
| stat_layout\stat_background\stat_p2 | rect | 20 | 2 | 0 | 3 | 730 | 730 | StatsMenu.user0 | 8 |  |
| stat_layout\stat_background\stat_p2\stats_page_2_tabs | image |  | 3 | 23 | 539 | 791 | 176 |  | 0 | Menus\Shared\shared_02_tab.dds |
| stat_layout\stat_background\stat_p2\stat_p2_layout | rect |  | 3 |  | -20 |  |  |  |  |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_lines | image |  | 4 | 50 | 62 | 625 | 562 |  | 2 | Menus\Stats\stat_attr_lines.dds |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_icons | image |  | 4 | 81 | 87 | 64 | 533 |  | 6 | Menus\Stats\stat_attr_icons.dds |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_strength_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_strength_layout\stat_p2_strength | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user1 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_strength_layout\stat_p2_strength_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._strength |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_intelligence_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_intelligence_layout\stat_p2_intelligence | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user2 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_intelligence_layout\stat_p2_intelligence_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._intelligence |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_willpower_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_willpower_layout\stat_p2_willpower | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user3 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_willpower_layout\stat_p2_willpower_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._willpower |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_agility_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_agility_layout\stat_p2_agility | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user4 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_agility_layout\stat_p2_agility_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._agility |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_speed_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_speed_layout\stat_p2_speed | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user5 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_speed_layout\stat_p2_speed_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._speed |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_endurance_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_endurance_layout\stat_p2_endurance | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user6 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_endurance_layout\stat_p2_endurance_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._endurance |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_personality_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_personality_layout\stat_p2_personality | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user7 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_personality_layout\stat_p2_personality_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._personality |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_luck_layout | rect | 24 | 4 | 75 | me().user0 mul(67) add(87) | 592 | 64 |  | 5 |  |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_luck_layout\stat_p2_luck | text |  | 5 | 467 | 16 |  |  |  | 3 | stat_p2.user8 |
| stat_layout\stat_background\stat_p2\stat_p2_layout\stat_p2_luck_layout\stat_p2_luck_label | text |  | 5 | 100 | 16 |  |  |  | 3 | strings()._luck |
| stat_layout\stat_background\stat_p3 | rect | 30 | 2 | 0 | 3 | 730 | 730 | StatsMenu.user0 | 8 |  |
| stat_layout\stat_background\stat_p3\stats_page_3_tabs | image |  | 3 | 23 | 539 | 791 | 176 |  | 0 | Menus\Shared\shared_03_tab.dds |
| stat_layout\stat_background\stat_p3\stat_p3_line_hor_1 | image |  | 3 | 46 | 73 | 636 | 3 |  | 1 | Menus\Stats\stat_border_horizontal_1.dds |
| stat_layout\stat_background\stat_p3\stat_p3_line_left | image |  | 3 | 66 | 65 | 3 | 511 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p3\stat_p3_line_right | image |  | 3 | 660 | 65 | 3 | 511 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p3\stat_p3_scroll_bar | image | 32 | 3 | stat_p3_window.x add() add(10) | stat_p3_window.y add(32) |  | stat_p3_window.height sub(64) |  | 5 |  |
| stat_layout\stat_background\stat_p3\stat_p3_title_left | image |  | 3 | stat_p3_title_text.x sub(2) sub(55) | 50 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| stat_layout\stat_background\stat_p3\stat_p3_title_right | image |  | 3 | stat_p3_title_text.width div(2) add() add(20) | 50 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| stat_layout\stat_background\stat_p3\stat_p3_title_text | text |  | 3 | 370 | 45 |  |  |  | 3 | stat_p3_scroll_bar.user7 add(3) min(3) me()._title_ |
| stat_layout\stat_background\stat_p3\stat_p3_window | rect |  | 3 | 70 | 75 | 600 | 490 |  | 3 |  |
| stat_layout\stat_background\stat_p3\stat_p3_window\stat_p3_window_pane | rect | 31 | 4 |  | 0 sub() mul(70) |  |  |  | 2 |  |
| stat_layout\stat_background\stat_p3\stat_p3_window\stat_p3_window_pane\stat_p3_minor_label | rect |  | 5 | 0 | 70 mul() add(-70) | 600 | 70 |  | 3 |  |
| stat_layout\stat_background\stat_p3\stat_p3_window\stat_p3_window_pane\stat_p3_minor_label\stat_p3_minor_label_left | image |  | 6 | stat_p3_minor_label_text.x sub(2) sub(55) | 35 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| stat_layout\stat_background\stat_p3\stat_p3_window\stat_p3_window_pane\stat_p3_minor_label\stat_p3_minor_label_right | image |  | 6 | stat_p3_minor_label_text.width div(2) add() add(20) | 35 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| stat_layout\stat_background\stat_p3\stat_p3_window\stat_p3_window_pane\stat_p3_minor_label\stat_p3_minor_label_text | text |  | 6 | 300 | 30 |  |  |  | 3 | strings()._minorskills |
| stat_layout\stat_background\stat_p3\stat_p3_progress_label | text |  | 3 | 100 | 580 |  |  |  | 3 | strings()._levelprogress |
| stat_layout\stat_background\stat_p3\stat_p3_progress | image |  | 3 | 390 | 575 | 218 | 43 |  | 7 | Menus\Stats\stat_skill_level_ribbon_empty.dds |
| stat_layout\stat_background\stat_p3\stat_p3_progress\stat_p3_progress_ribbon | image | 55 | 4 | 26 | 6 | stat_p3.user2 div() mul(193) | 20 |  | 8 | Menus\Stats\stat_skill_level_ribbon_full.dds |
| stat_layout\stat_background\stat_p3\stat_p3_progress\stat_p3_progress_ribbon\stat_p3_progress_slider | image |  | 5 | parent().width sub(10) | -1 | 12 | 23 |  | 1 | Menus\Stats\stat_skill_level_ribbon_slider.dds |
| stat_layout\stat_background\stat_p4 | rect | 40 | 2 | 0 | 3 | 730 | 730 | StatsMenu.user0 | 8 |  |
| stat_layout\stat_background\stat_p4\stats_page_4_tabs | image |  | 3 | 23 | 539 | 791 | 176 |  | 0 | Menus\Shared\shared_04_tab.dds |
| stat_layout\stat_background\stat_p4\stat_p4_title_left | image |  | 3 | stat_p4_title_text.x sub(2) sub(55) | 50 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_left.dds |
| stat_layout\stat_background\stat_p4\stat_p4_title_right | image |  | 3 | stat_p4_title_text.width div(2) add() add(20) | 50 | 35 | 17 |  | 7 | Menus\Shared\shared_decor_blue_right.dds |
| stat_layout\stat_background\stat_p4\stat_p4_title_text | text |  | 3 | 370 | 45 |  |  |  | 3 | strings()._factions |
| stat_layout\stat_background\stat_p4\stat_p4_line_hor_1 | image |  | 3 | 46 | 88 | 636 | 3 |  | 9 | Menus\Stats\stat_border_horizontal_1.dds |
| stat_layout\stat_background\stat_p4\stat_p4_line_left | image |  | 3 | 66 | 100 | 3 | 460 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p4\stat_p4_line_right | image |  | 3 | 660 | 100 | 3 | 460 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p4\stat_p4_scroll_bar | image | 42 | 3 | stat_p4_window.x add() add(10) | stat_p4_window.y add(32) |  | stat_p4_window.height sub(64) |  | 5 |  |
| stat_layout\stat_background\stat_p4\stat_p4_up | rect |  | 3 | 60 | 60 | 5 | 5 |  |  |  |
| stat_layout\stat_background\stat_p4\stat_p4_down | rect |  | 3 | 60 | 60 | 5 | 5 |  |  |  |
| stat_layout\stat_background\stat_p4\stat_p4_window | rect |  | 3 | 70 | 100 | 600 | 465 |  | 3 |  |
| stat_layout\stat_background\stat_p4\stat_p4_window\stat_p4_window_pane | rect | 41 | 4 |  | 0 sub() mul(155) |  |  |  |  |  |
| stat_layout\stat_background\stat_p5 | rect | 50 | 2 | 0 | 3 | 730 | 730 | StatsMenu.user0 | 8 |  |
| stat_layout\stat_background\stat_p5\stats_page_5_tabs | image |  | 3 | 23 | 539 | 791 | 176 |  | 0 | Menus\Shared\shared_05_tab.dds |
| stat_layout\stat_background\stat_p5\stat_p5_line_to | image |  | 3 | 46 | 68 | 636 | 3 |  | 9 | Menus\Stats\stat_border_horizontal_1.dds |
| stat_layout\stat_background\stat_p5\stat_p5_line_left | image |  | 3 | 66 | 62 | 3 | 511 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p5\stat_p5_line_right | image |  | 3 | 660 | 62 | 3 | 511 |  | 1 | Menus\Stats\stat_border_vertical_2.dds |
| stat_layout\stat_background\stat_p5\stat_p5_line_bottom | image |  | 3 | 46 | 600 | 636 | 3 |  | 9 | Menus\Stats\stat_border_horizontal_2.dds |
| stat_layout\stat_background\stat_p5\stat_p5_scroll_bar | image | 52 | 3 | stat_p5_window.x add() add(10) | stat_p5_window.y add(34) |  | stat_p5_window.height sub(64) |  | 5 |  |
| stat_layout\stat_background\stat_p5\stat_p5_up | rect |  | 3 | 60 | 60 | 5 | 5 |  |  |  |
| stat_layout\stat_background\stat_p5\stat_p5_down | rect |  | 3 | 60 | 60 | 5 | 5 |  |  |  |
| stat_layout\stat_background\stat_p5\stat_p5_window | rect |  | 3 | 70 | 70 | 600 | 494 |  | 3 |  |
| stat_layout\stat_background\stat_p5\stat_p5_window\stat_p5_window_pane | rect | 51 | 4 |  | 0 sub() mul(35) |  |  |  |  |  |
| stat_layout\stat_background\stat_tabs | rect |  | 2 | 0 | 594 | 730 | 128 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_prev | rect | 8 | 3 | -1 | -1 | 1 | 1 |  |  |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_next | rect | 9 | 3 | -1 | -1 | 1 | 1 |  |  |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p1 | rect | 1 | 3 | 28 | 37 | 135 | 85 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p1\stat_tabs_p1_icon | image |  | 4 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p1\stat_tabs_p1_selection_off | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_off.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p1\stat_tabs_p1_selection_on | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_on.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p2 | rect | 2 | 3 | 165 | 37 | 135 | 85 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p2\stat_tabs_p2_icon | image |  | 4 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p2\stat_tabs_p2_selection_off | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_off.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p2\stat_tabs_p2_selection_on | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_on.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p3 | rect | 3 | 3 | 304 | 37 | 135 | 85 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p3\stat_tabs_p3_icon | image |  | 4 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p3\stat_tabs_p3_selection_off | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_off.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p3\stat_tabs_p3_selection_on | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_on.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p4 | rect | 4 | 3 | 443 | 37 | 135 | 85 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p4\stat_tabs_p4_icon | image |  | 4 | 37 | 8 | 64 | 64 |  | -1 | Menus\Shared\all_tabs_icons.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p4\stat_tabs_p4_selection_off | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 | -3 | Menus\Shared\general_tab_selection_off.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p4\stat_tabs_p4_selection_on | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_on.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p5 | rect | 5 | 3 | 578 | 37 | 135 | 85 |  | 5 |  |
| stat_layout\stat_background\stat_tabs\stat_tabs_p5\stat_tabs_p5_icon | image |  | 4 | 37 | 8 | 64 | 64 |  |  | Menus\Shared\all_tabs_icons.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p5\stat_tabs_p5_selection_off | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_off.dds |
| stat_layout\stat_background\stat_tabs\stat_tabs_p5\stat_tabs_p5_selection_on | image |  | 4 | 0 | 24 | 136 | 36 | parent().user1 |  | Menus\Shared\general_tab_selection_on.dds |
| stat_pop_background | image | 61 | 0 | stat_layout.x add() add() sub(1 sub()) sub(20) | 170 | 419 | stat_pop_text.y add() add(25) |  | -9 | Menus\Magic\magic_popup_background.dds |
| stat_pop_background\magicpop_border_ULcorner | image |  | 1 | 0 | 0 | 10 | 10 |  | 3 | Menus\Magic\magic_popup_border_corner.dds |
| stat_pop_background\magicpop_border_top | image |  | 1 | magicpop_border_ULcorner.x add() | magicpop_border_ULcorner.y | magicpop_border_URcorner.x sub() sub() | 11 |  | 3 | Menus\Magic\magic_popup_border_edge_horizontal.dds |
| stat_pop_background\magicpop_border_URcorner | image |  | 1 | parent().width sub() add(2) | 0 | magicpop_border_ULcorner.width | magicpop_border_ULcorner.height |  | 3 | Menus\Magic\magic_popup_border_corner.dds |
| stat_pop_background\magicpop_border_left | image |  | 1 | magicpop_border_ULcorner.x | magicpop_border_ULcorner.y add() | 11 | magicpop_border_LLcorner.y sub() sub() |  | 3 | Menus\Magic\magic_popup_border_edge_vertical.dds |
| stat_pop_background\magicpop_border_LLcorner | image |  | 1 | 0 | parent().height sub() | 10 | 10 |  | 3 | Menus\Magic\magic_popup_border_corner.dds |
| stat_pop_background\magicpop_border_bottom | image |  | 1 | magicpop_border_LLcorner.x add() | magicpop_border_LLcorner.y | magicpop_border_LRcorner.x sub() sub() | 11 |  | 3 | Menus\Magic\magic_popup_border_edge_horizontal.dds |
| stat_pop_background\magicpop_border_LRcorner | image |  | 1 | parent().width sub() add(2) | magicpop_border_LLcorner.y | magicpop_border_ULcorner.width | magicpop_border_ULcorner.height |  | 3 | Menus\Magic\magic_popup_border_corner.dds |
| stat_pop_background\magicpop_border_right | image |  | 1 | magicpop_border_URcorner.x | magicpop_border_URcorner.y add() | 11 | magicpop_border_LRcorner.y sub() sub() |  | 3 | Menus\Magic\magic_popup_border_edge_vertical.dds |
| stat_pop_background\stat_pop_image | image |  | 1 | 41 add(85) | 30 | 256 | 256 | parent().user4 | 1 | parent().user2 |
| stat_pop_background\stat_pop_text | text |  | 1 | 30 | stat_pop_image.height add() add(15) |  |  |  | 7 | parent().user3 |
| stat_pop_background\stat_pop_hand2hand_icon | image |  | 1 | parent().width sub(128) | stat_pop_text.y add() sub(24) | 32 | 32 | StatsMenu.user0 | 7 | Icons\Weapons\HandToHand.dds |
| stat_pop_background\stat_pop_hand2hand_damage | text |  | 1 | stat_pop_hand2hand_icon.x add() add(10) | stat_pop_hand2hand_icon.y |  |  | stat_pop_hand2hand_icon.visible | 7 | parent().user5 |
| stat_skill_template | template |  | 0 |  |  |  |  |  |  |  |
| stat_skill_template\stat_item | rect | 34 | 1 | 0 | 70 mul() | 600 | 69 |  | 3 |  |
| stat_skill_template\stat_item\stat_item_icon | image |  | 2 | 3 | 3 | 64 | 64 |  | 1 | parent().user5 |
| stat_skill_template\stat_item\stat_item_label | text |  | 2 | 75 | 38 sub(2) |  |  |  | 1 | parent().user4 |
| stat_skill_template\stat_item\temp_fill_bar | image |  | 2 | 330 | 22 |  |  |  | 1 |  |
| stat_skill_template\stat_item\stat_item_value | text |  | 2 | 555 | 20 |  |  |  | 1 | parent().user3 |
| stat_skill_template\stat_item\stat_item_line | image |  | 2 | -30 | 68 | 636 | 3 |  | -2 | parent().listindex add(1) me()._file_ |
| stat_faction_template | template |  | 0 |  |  |  |  |  |  |  |
| stat_faction_template\stat_faction_item | rect | 44 | 1 | 0 | me().height mul() | 600 | 155 |  | 3 |  |
| stat_faction_template\stat_faction_item\stat_faction_item_icon | image |  | 2 | 15 | 5 | 128 | 128 |  | 1 | parent().user4 |
| stat_faction_template\stat_faction_item\stat_faction_item_name | text |  | 2 | 155 | 80 sub() div(2) add(5) |  |  |  | 1 | parent().user1 |
| stat_faction_template\stat_faction_item\stat_faction_item_rank | text |  | 2 | 155 | sibling(stat_faction_item_name).y add() add(8) |  |  |  | 1 | parent().user2 |
| stat_faction_template\stat_faction_item\stat_faction_line | image |  | 2 | -30 | 143 | 636 | 3 |  | -2 | parent().listindex add(1) me()._file_ |
| stat_misc_template | template |  | 0 |  |  |  |  |  |  |  |
| stat_misc_template\stat_misc_item | rect | 44 | 1 | 0 | me().height mul() | 600 | 32 |  | 3 |  |
| stat_misc_template\stat_misc_item\stat_misc_label | text |  | 2 | 30 | 16 |  |  |  | 1 | parent().user1 |
| stat_misc_template\stat_misc_item\stat_misc_value | text |  | 2 | 550 | 16 |  |  |  | 1 | parent().user2 |

#### player_model (`player_model.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|

#### HUDSafeZone (`safe_zone.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| safe_top | image |  | 0 | 0 | 0 | screen().width | screen().cropy |  |  | Menus\Shared\shared_white.dds |
| safe_bottom | image |  | 0 | 0 | screen().height sub() | screen().width | screen().cropy |  |  | Menus\Shared\shared_white.dds |
| safe_left | image |  | 0 | 0 | screen().cropy | screen().cropx | screen().height sub() sub() |  |  | Menus\Shared\shared_white.dds |
| safe_right | image |  | 0 | screen().width sub() | screen().cropy | screen().cropx | screen().height sub() sub() |  |  | Menus\Shared\shared_white.dds |
| safe_top_2 | image |  | 0 | safe_left.width | safe_top.height | screen().width sub() sub() | screen().cropy |  |  | Menus\Shared\shared_white.dds |
| safe_bottom_2 | image |  | 0 | screen().cropx | screen().height sub() sub() | screen().width sub() sub() | screen().cropy |  |  | Menus\Shared\shared_white.dds |
| safe_left_2 | image |  | 0 | screen().cropx | screen().cropy mul(2) | screen().cropx | screen().height sub() sub() sub() sub() |  |  | Menus\Shared\shared_white.dds |
| safe_right_2 | image |  | 0 | screen().width sub() sub() | screen().cropy mul(2) | screen().cropx | screen().height sub() sub() sub() sub() |  |  | Menus\Shared\shared_white.dds |

### 32.5 ロード / その他

#### LoadingMenu (`loading_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| black | rect |  | 0 | 0 | 0 | screen().width | screen().height |  | -1 |  |
| load_title_page | image |  | 0 | 0 | 0 | screen().width | screen().height | LoadingMenu.user0 | 1000 | Menus\Loading\loading_background.dds |
| load_title_page\load_symbol | image |  | 1 | parent().width sub() div(2) | 250 | 330 | 466 | true | 2 | Menus\Loading\loading_symbol.dds |
| load_main | image |  | 0 | screen().width sub() div(2) | screen().height sub() div(2) | screen().height mul(16,9) | screen().height | LoadingMenu.user0 | 1000 | LoadingMenu.user1 |
| load_main\load_text | text |  | 1 | parent().width sub() div(2) | 120 sub() div(2) add(555) |  |  |  | 3 | LoadingMenu.user2 |
| load_progress | image |  | 0 | screen().width sub() div(2) | 772 | 218 | 43 |  | 1005 | Menus\Stats\stat_skill_level_ribbon_empty.dds |
| load_progress\load_progress_ribbon | image |  | 1 | 26 | 7 | LoadingMenu.user3 div() mul(180) max(1) | 20 |  | 2 | Menus\Stats\stat_skill_level_ribbon_full.dds |
| load_progress\load_progress_ribbon\load_t_ribbon_slider | image |  | 2 | parent().width | -1 | 12 | 23 |  | 4 | Menus\Stats\stat_skill_level_ribbon_slider.dds |
| load_progress\load_loading | text |  | 1 | parent().width div(2) | 35 |  |  |  | 4 | strings()._loading |
| load_debug_info | text |  | 0 | 60 | 885 |  |  |  | 1004 | LoadingMenu.user5 |

#### LoadingMenu (`loading_ingame_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| black | rect |  | 0 | 0 | 0 | screen().width | screen().height |  | -1 |  |
| load_main | image | 1 | 0 | screen().width div(2) sub() | screen().height div(2) sub() | 1280 | 960 |  | 1 | Menus\Loading\InGameDefault.dds |
| load_main\load_text | text | 5 | 1 | 213 | 600 |  |  |  | 3 |  |
| load_main\load_loading | text | 6 | 1 | parent()._center_width | 802 |  |  |  | 4 | strings()._loading |
| load_main\load_progress | image |  | 1 | parent()._center_width | 772 | 218 | 43 |  | 5 | Menus\Stats\stat_skill_level_ribbon_empty.dds |
| load_main\load_progress\load_progress_ribbon | image | 7 | 2 | 26 | 7 | me().user1 div() mul(193) | 20 |  | 1 | Menus\Stats\stat_skill_level_ribbon_full.dds |
| load_main\load_progress\load_progress_ribbon\load_t_ribbon_slider | image |  | 3 | parent().width sub(10) | -1 | 12 | 23 |  | 2 | Menus\Stats\stat_skill_level_ribbon_slider.dds |

#### LoadingMenu (`loading_bar_ingame_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| load_loading | text | 6 | 0 | parent()._center_width | 802 |  |  |  | 4 | strings()._loading |
| load_progress | image |  | 0 | parent()._center_width | 772 | 218 | 43 |  | 5 | Menus\Stats\stat_skill_level_ribbon_empty.dds |
| load_progress\load_progress_ribbon | image | 7 | 1 | 26 | 7 | me().user1 div() mul(193) | 20 |  | 1 | Menus\Stats\stat_skill_level_ribbon_full.dds |
| load_progress\load_progress_ribbon\load_t_ribbon_slider | image |  | 2 | parent().width sub(10) | -1 | 12 | 23 |  | 2 | Menus\Stats\stat_skill_level_ribbon_slider.dds |

#### BreathMenu (`breath_meter_menu.xml`)

| Widget (path) | type | id | nest | x | y | w | h | visible | depth | content |
|---|---|---|---|---|---|---|---|---|---|---|
| breath_progress | image |  | 0 | parent()._center_width | 72 | 418 | 64 |  | 5 | Menus\BreathMeter\Breath_Meter_empty.dds |
| breath_progress\breath_progress_ribbon | image | 2 | 1 | 11 | 11 | me().user1 div() mul(284) | 40 |  | 1 | Menus\BreathMeter\Breath_Meter_fill.dds |



### 32.6 NOT USED / 実体なしファイル（パース不能）

以下はファイル内容が「NOT USED」を示すコメントのみで、ウィジェット定義を一切持たない。
実装対象外であり、相当機能は別ファイルへ統合されている。

| ファイル | サイズ | 内容 | 代替実体 |
|---|---|---|---|
| `scroll_menu.xml` | 45 B | コメントのみ | `scroll_bar` プレハブ（§24.9 / §25.5）へ統合 |
| `birthsign_menu.xml` | 47 B | コメントのみ | `chargen\birthsign_menu.xml`（§32.3 BirthSignMenu） |
| `chargen\attributes_menu.xml` | 47 B | `<!-- NOT USED - SEE skills_menu.xml INSTEAD -->` | `chargen\skills_menu.xml`（§32.3 SkillsMenu） |
| `chargen\specilization_menu.xml` | 47 B | `<!-- NOT USED - SEE skills_menu.xml INSTEAD -->` | `chargen\skills_menu.xml`（§32.3 SkillsMenu） |

補足: `menu_labels.txt`(0 B) / `s.txt`(4 B) / 各 `vssver.scc` はレイアウト定義ではなく
ビルド管理補助ファイルのため対象外。

### 32.7 prefabs / 単体パース不可ファイル

`prefabs\` 配下の断片（`scroll_bar.xml`, `horiz_floating_scroll.xml`, `button_template.xml` 等）は
メニュー共通のプレハブで、単体ではメニューを構成しない（`<include>` で各メニューへ取り込む）。
trait 仕様は §24.9 / §25.5 に記載済みで、本節では個別表化しない。
Bethesda のプレハブ断片は `<image><include/>...<trait>...<rect>` の閉じタグ不整合を含むため、
単体パースは不可（レイアウト値は取り込み先メニュー側の上書き値が正）。

### 32.8 Android 移植向け確定値（依頼5 追加分）

1. **メニュー登録 ID**: 本章のメニューはすべて §30.5 の連番帯 0x3e9〜0x41b に属する。
   現時点で確定している ID は次のとおりで、残りの連番への割当は未解決である
   （メニュー生成テーブル `FUN_00582160` の全走査が未完のため）。

   | メニュー | id | 出典 |
   |---|---|---|
   | MessageMenu | 0x3e9 | §30.4.3 |
   | ContainerMenu | 0x3f0 | §31 |
   | AudioMenu | 0x3f9 | §26 / §30.7 |
   | VideoMenu | 0x3fa | §24 |
   | VideoDisplayMenu | 0x3fb | §24 |
   | GameplayMenu | 0x3fc | §26 |
   | LevelUpMenu | 0x403 | §31 |
   | RepairMenu | 0x40b | §31 |
   | QuickKeysMenu | 0x416 | §31.3 |
   | RechargeMenu | 0x419 | §31 |

   未解決分の確定は、`FUN_00582160`(MenuManager ctor) が構築するメニュー配列
   （正本グローバル `0x00b13974` / 件数 `0x00b13978`、§30.1）の走査で行う。
   本章の各メニュー表は ID ではなく **XML ルート名とウィジェット構成** を正とする。

2. **寸法・座標**: 各メニューの座標式は表の `x`/`y` 列が正本。式言語
   （`parent()` / `me()` / `screen()` / `sibling()` / `last()` / `first()` /
   `add()` / `sub()` / `div()` / `mul()`）の評価規則は §24.2 と同一。
   Android 側は式パーサを共通化して全メニューへ適用する。

3. **HUD 系**: HUDMainMenu / HUDInfoMenu / HUDReticle / HUDSubtitleMenu は
   ゲーム中の常時表示オーバーレイ。`visible` は実行時制御（`userN`）で切替わる。
   レイアウト値は表のとおり固定で、値そのものは動的供給。

4. **in-game 画面系**: InventoryMenu / MagicMenu / MagicPopupMenu / MapMenu /
   StatsMenu は探索中に開く非ポーズメニュー（§30.4 のフェード規則に従う）。
   行リストのスクロールは §31.7 と同じ「行 y = index * lineHeight - scrollOffset」に帰着。

5. **未検証**: 各メニューの実行時供給値（`userN` の実データ）は、対応する
   ゲームシステム章（Container / Magic / Quest 等）で確定する。本章は
   レイアウト確定までを範囲とする。


## 33. main 依頼分の追加解析（NPC ダメージ経路 / メニュー登録 ID / フェード / audio 残 / 設定消費者）

本章は main（9427ac74）から受領した §33 以降の解析依頼 5 件の結果である。
対象バイナリは §1 と同じ Steam 版 `Oblivion.exe`（md5 CDD2F0C5EFF198D4F26B7B5B54CE4930）。

### 33.1 依頼1: NPC からプレイヤーへのダメージ計算経路

#### 33.1.1 近接ダメージ入口と適用チェーン

NPC 近接攻撃のダメージは次の順でプレイヤー HP に届く。

| 段 | 関数 | VA | 役割 |
|---|---|---|---|
| 1 | `Actor::ApplyDamage`（vtable +0x3ac） | 0x005FEBF0 | 攻撃ヒット入口。対象抽出→ヒットメッセージ→適用呼出 |
| 2 | `Actor::ApplyDamageEffect`（vtable +0x2d4） | 0x005E46C0 | 効果オブジェクト解釈、抵抗適用、値減算 |
| 3 | Active Effects リスト管理 | 0x00488AA0 | magic-type 別テーブル引当 |
| 4 | `Actor::ModifyActorValue` | 0x005E2BE0 | AV 更新。AV==8(Health) かつ負値で死判定呼出 |
| 5 | 死判定（vtable +0x3b8） | 0x006034B0 | HP < 閾値 `_DAT_00a2f948` で死亡本体 `FUN_006005f0` |

- 入口 `FUN_005FEBF0`（6650B）は `param_4==0 || FUN_0041df00()==0` で対象を抽出し、ヒット方向ベクトルを
  `FUN_0053d4b0`/`FUN_005e6a40`/`FUN_006fcdc0` で計算した後、`(**(code**)(*param_1+0x2d4))(...)` を呼ぶ。
  適用後に `[+0x288](8)` で Health を読み、`FUN_00579b60` でヒットメッセージを発行する。
- `FUN_005E46C0`（520B, RET 0x10）は効果オブジェクト `param_2` の `+8->+4=='!'` で magic 判定し、
  減算経路は `fVar1=FUN_004849c0()`（効果値）、`fVar2=ABS(対象現値)`、delta=効果値-現値 として `FUN_00488AA0` へ渡す。
- `FUN_005E2BE0`（239B）は `if(param_2==8 && param_3<0.0) (**(vtable(*param_1)+0x3b8))(...)` で
  **Health AV の減算時にのみ死判定を起動**する。
- プレイヤー HP フィールドは Actor の AV 配列（Health は AV index 8）経由でアクセスする。
  移植側は AV[8] を HP として直接減算してよい（`+0x3b8` 死判定を AV 減算内で呼ぶこと）。

#### 33.1.2 ダメージ式（近接・AI・スキル・疲労・防具）

近接ダメージは合成関数 `FUN_00547070`（206B, Combat_CalcMeleeDamage）で算出する。

```
fVar5 = FUN_00547b90(スキル参照値, スケール)         // スキル→0..100 レーティング
param_5 = fDamageWeaponMult * 武器ダメージ
fVar3 = base + scale * (ratingA * 0.01)              // 技能項（clamp 0..100）
fVar4 = base + scale * (armor  / clamp0..100 * 0.01) // 防具項
fVar5 = (param_8==0) ? FUN_00547f00(疲労) : 1.0      // 疲労倍率
return fVar2 * fVar1 * fVar3 * fVar4 * fVar5 * param_7
```

- スキル参照値→レーティング: `FUN_00547b90`: `fVar1 = param_1 + param_2*_DAT_00b37390 + _DAT_00b37388` → 0..100 clamp。
- 疲労倍率: `FUN_00547f00`: `return _DAT_00b37530 - (1.0 - param_1) * _DAT_00b37538`。
- AI 攻撃: `FUN_00547140`（246B）は武器ダメージ(short)→`FUN_00547070`（近接）→
  `fAIMagicSpellMult * 魔法値` を加算→`fAIMeleeWeaponMult * (近接+魔法項)`。
- 防具レーティング合計: `FUN_00611FA0`（armor/clothing rating 合算）。

#### 33.1.3 難易度倍率（確定式）

`FUN_005E2560(attacker, damage, target)`（267B, float10 `__thiscall`, 戻=補正後ダメージ）:

```
fVar3 = damage;
if (target != 0 && *(float*)(DAT_00b333c4 + 0x734) != 0.0) {
    fVar2 = *(float*)(DAT_00b333c4 + 0x734) * _DAT_00b37b88;   // × fDifficultyDamageMultiplier(=10.0)
    if (playerDifficulty <= 0.0) fVar2 = fVar2 + _DAT_00a2f928; // +1.0 (double)
    else { if (fVar2 == 1.0) return damage; fVar2 = 1.0 / ABS(fVar2 - 1.0); }
    if (fVar2 != 0.0) {
        if (attacker == Player) { return (damage >= 0) ? damage/fVar2 : fVar2*damage; }
        if (target   == Player) { return (damage >= 0) ? fVar2*damage : damage/fVar2; }
        return damage;
    }
}
return fVar3;
```

- param_1=ECX(attacker)、param_2=[ESP+4](damage)、param_3=EDI([ESP+0x10]=target)。
- 難易度設定値は `Player+0x734`（`DAT_00b333c4 = Player`, 書き込み元は §32 で確定）。
- `_DAT_00b37b88` = `fDifficultyDamageMultiplier`（GMST, 既定 10.0）、`_DAT_00a2f928` = 1.0(double)。
- **Android 側の暫定式 `pow(6,(d-50)/50)` は誤り。** 実式は `playerDifficulty`（0..1 正規化に相当する
  ゲーム設定スカラー）を `DifficultyDamageMultiplier` 倍し、`<=0` なら +1.0、それ以外は
  `1/|x-1|` へ変換してから攻撃者/被攻撃者で乗除を切り替える。`x==1.0` は無補正。
- `FUN_005E2560` の呼出元は 3 件のみ:
  - `FUN_00699EB0`（418B）: 敵魔法効果→Health AV 合算→AI 攻撃見積り。
  - `FUN_006A86F0`（268B）: GUIscript Actor property、AV==8 の負ダメージ時のみ適用。
  - `FUN_005E58F0`: damage→FCHS→倍率→FCHS→vtable +0x330→`FUN_00613640`→集計 obj +0x10/+0xc へ FADD。

#### 33.1.4 魔法・遠距離

魔法ダメージは同じ `FUN_005E46C0`（効果オブジェクト経由）を通る。遠距離（弓/投射）は
`FUN_005FEBF0` のヒット対象抽出（`FUN_0041df00` = レイキャスト相当）を経て同一適用チェーンへ入る。
プレイヤー→NPC の攻撃は `FUN_00547070`/`FUN_00547140` を通り、既存 `calculateDamage` と一致することを確認した。

#### 33.1.5 Android 移植向け確定値

1. プレイヤー HP = Actor AV index 8。AV 減算は `ModifyActorValue(AV=8, 負値)` で行い、
   内部で死判定（HP < `_DAT_00a2f948`）を必ず呼ぶ。
2. 難易度倍率は §33.1.3 の実式を実装する（暫定 `pow(6,...)` を置換）。
3. 近接ダメージは §33.1.2 の合成式を実装する。防具項は 0..100 clamp 後の ×0.01。
4. NPC→プレイヤー経路は現状移植側に存在しないため、`FUN_005E46C0` 相当の
   「効果オブジェクト→AV 減算→死判定」を新設する。

### 33.2 依頼2: メニュー登録 ID の全割当

`FUN_00582160`（MenuManager ctor）が構築するメニュー配列は正本グローバル
`0x00b13974`（配列）/`0x00b13978`（件数）（§30.1）。§32.8 の確定分に、
MenuVtables 走査で得た vtable を突き合わせた結果を次に示す。

| メニュー | 登録 ID | vtable | 出典 |
|---|---|---|---|
| MessageMenu | 0x3e9 | — | §30.4.3 |
| ContainerMenu | 0x3f0 | — | §31 |
| PauseMenu | 0x3f5 | — | §22 / §32 |
| AudioMenu | 0x3f9 | — | §26 / §30.7 |
| VideoMenu | 0x3fa | — | §24 |
| VideoDisplayMenu | 0x3fb | — | §24 |
| GameplayMenu | 0x3fc | — | §26 |
| ControlsMenu | 0x3fd | — | §24.11 |
| LevelUpMenu | 0x403 | — | §31 |
| RepairMenu | 0x40b | — | §31 |
| QuickKeysMenu | 0x416 | — | §31.3 |
| RechargeMenu | 0x419 | — | §31 |
| SleepWaitMenu | — | 0x00a6d7c4 (nslots=15) | §33.5 |
| SkillsMenu | — | 0x00a6d71c (nslots=15) | MenuVtables |

MenuVtables による全メニュー vtable は 56 件。`MVT <addr> nslots=N <Name> [i]=FUN_xxx` 形式で
各スロットポインタが取得できる。ID との突き合わせは `FUN_00582160` の配列走査で確定する。

### 33.3 依頼3: メニュー開閉フェードの実装値

- フェードノードは 20 バイト。レイアウトは
  `[0]=fade_id`、`[1]=current`、`[2]=duration`、`[3]=prev`、`[4]=next`。
- 生成は `FUN_00584300(fadeNode, duration)`。`fade_id` は `FUN_005898f0` が算出する。
- trait は `0xfdb`/`0xfdc` の 2 系統。duration 既定値は 0.25（`explorefade`/`menufade` 共通）。
- 係数式（`FUN_006ad030` 内、`param_1+0x2f0`=current, `+0x2f8`=target/duration）:

```
local_420 = ABS(current - target)
if (_DAT_00a57604 <= local_420)
    fVar3 = current ± _DAT_00a73e80        // 線形ステップ
else
    fVar3 = duration - ((local_420 - _DAT_00a2faa0) + (local_420 - _DAT_00a2faa0)) * duration  // 放物線
```

  `_DAT_00a68610`/`DAT_00a3d65c` による別分岐あり。`+0xdc` の bit フラグで fade 状態を管理。
- `FUN_00588a70` は**メニューを閉じる時のノード挿入**を行い、暗転を逆再生させる
  （閉じ時に逆再生あり）。
- `FUN_00588bd0(menuObj, traitId)` は trait の float 値を取得する。
- メニュー表示中フラグは `0x00b3a6d4`。MenuManager シングルトンは `0x00b3a6e0`（`+0x130` にメニューリスト）。

#### Android 移植向け確定値

1. 暗転は全画面黒クアッドのアルファ = `current / duration` でよい。ただし
   §33.3 の放物線分岐（`local_420 < _DAT_00a57604`）ではイージングが入るため、
   精度を求める場合は放物線式を実装する。
2. `explorefade` は探索中の非ポーズメニュー、`menufade` はポーズ系メニューに使う
   （trait 0xfdb/0xfdc の 2 系統）。
3. フェードは `0x00b3a6d4`（メニュー表示中）でゲートする。

### 33.4 依頼4: §30.6 の残 2 点

#### 33.4.1 audio チャンネル 5 系統と `FUN_006aa1a0` の対応順序

`FUN_00595a20()`（設定適用）は `DAT_00b33398+0x24` のオブジェクトに対し
`FUN_00588bd0(0xfb5)` を 5 回呼び、各チャンネル値を次へ渡す。

| 順 | チャンネル | 格納先 |
|---|---|---|
| 1 | master | `[obj+0xb8]` |
| 2 | voice | `FUN_006aa1a0` |
| 3 | effect | `[obj+0xc4]` |
| 4 | foot | `[obj+0xbc]` |
| 5 | music | `[obj+0xc0]` |

`FUN_006aa1a0(obj, value, commit)`:

```
value = clamp(value, 0, 1)
value = value * [obj+0xb8]              // master 乗算
if ([obj+0xdc] & 1) {                   // dB モード
    if (value*value < 0.01f) return;    // 無音
    return (int)(log10(value*value * master) * 2000.0)
}
```

audio_menu.xml の行順は master, voice, effect, foot, music。`FUN_006ad030` が
`FUN_006aa1a0` を 6 回呼ぶのは、保存済み voice 音量を 6 回再適用するためである。

#### 33.4.2 `FUN_0066F420`（2410B）全文精読

先頭で `if (DAT_00b13228 != '\0') FUN_00466b70();` を実行する
（`bSaveOnTravel` が真なら移動時オートセーブを発行）。以降はセーブ/移動処理本体。
`FUN_004b7b40`（617B）末尾は
`if (DAT_00b13230 != '\0' && DAT_00b13228 != '\0' && ...) *(uint*)(DAT_00b33b00+0x18) |= 0x8000;`。

### 33.5 依頼5: 字幕・オートセーブの消費者（完全確定）

| グローバル | 設定 | 実行時消費者 | VA |
|---|---|---|---|
| 0x00b13200 | bGeneralSubtitles | 字幕描画（非0時に `FUN_0057acc0`） | 0x005BFB90 |
| 0x00b13208 | aDialogueSubtitles | 会話応答キャスト（距離判定後に字幕発行） | 0x0062A0E0 |
| 0x00b13210 | bCrosshair | crosshair 表示制御（trait 0xfb0/0xfb6） | 0x005A8710 |
| 0x00b13218 | bSaveOnRest | SleepWaitMenu 更新ハンドラ | 0x005D6A80 |
| 0x00b13220 | bSaveOnWait | SleepWaitMenu 更新ハンドラ | 0x005D6A80 |
| 0x00b13228 | bSaveOnTravel | 移動処理 | 0x004B7B40 / 0x0066F420 |

- `0x00b13208`（aDialogueSubtitles）の真の消費者は `FUN_0062A0E0`（621B, 会話応答キャスト）。
  `if (DAT_00b13208 != '\0' && *piVar4 != 0)` かつ距離 <= 2×基準で
  `FUN_0057acc0(*piVar4, param_1[0x88], 0, DAT_00a30634)` を実行（字幕発行）。
  `param_1[0x87]` は字幕残時間。
- **rest/wait の消費者は `FUN_005D6A80`**（SleepWaitMenu vtable `0x00a6d7c4` スロット[3] 更新ハンドラ。
  Ghidra は本関数を未認識のため DisasmRange で生命令を取得した）。

```
MOV AL, [ESI+0x4c]                        ; 休息種別フラグ
if (AL && [0x00b13218] != 0) || [0x00b13220] != 0:
    MOV ECX, [0x00b33b00]
    CALL 0x00466b70                       ; オートセーブ発行
```

  以降、`FUN_00588bd0(0xfb5)`、`FUN_009828c0`、`[0xb3b730]`/`[Player+0x590]` 設定、
  `FUN_005732d0(0xb3a6b0, 1, 1.0)` を実行する。
- **オートセーブ発行関数 = `FUN_00466b70(0x00b33b00)`。**

#### Android 移植向け確定値

1. 休憩/待機のオートセーブは SleepWaitMenu 更新ハンドラで、休息種別（rest/wait）に対応する
   トグルが真のとき発行する（セーブマネージャ obj = `0x00b33b00`）。
2. 移動オートセーブは移動処理先頭で `bSaveOnTravel` を判定して発行する。
3. 字幕は「一般字幕」と「会話字幕」で別経路。会話字幕は距離ゲート（<= 2×基準）付き。

### 33.6 Android 移植向け確定値（本章まとめ）

1. **NPC→プレイヤーダメージ経路を新設する**。入口は近接/魔法とも
   「効果値→`FUN_005E46C0` 相当→AV[8] 減算→死判定」の 4 段。死亡閾値は `_DAT_00a2f948`。
2. **難易度倍率を §33.1.3 の実式へ置換する**（暫定 `pow(6,...)` を廃止）。
3. **近接ダメージ合成式を §33.1.2 のとおり実装する**（技能項・防具項は 0..100 clamp 後 ×0.01、
   疲労倍率は `FUN_00547f00`）。
4. **メニュー開閉フェード**は 20B ノード（fade_id/current/duration/prev/next）、
   trait 0xfdb/0xfdc、duration 0.25。閉じ時は逆再生。精度が必要なら放物線イージングを実装。
5. **audio チャンネル順**は master, voice, effect, foot, music。
   `FUN_006aa1a0` は clamp→master 乗算→dB 変換（`log10(v^2*master)*2000`）。
6. **設定トグル消費者**は §33.5 の表のとおり。rest/wait は SleepWaitMenu 更新ハンドラ、
   travel は移動処理先頭、字幕は描画/会話応答キャスト。

---

## §34 メニュー登録 ID の全割当（依頼2 完了）

§32.8-4 の未解決項目を解消した。`FUN_00582160`（MenuManager ctor）が構築する
メニュー配列 `0x00b13974`（サイズ `0x00b1397a`、初期 capacity 1）に登録される全メニューに
ついて、**ID ↔ vtable ↔ XML ルート名**を確定した。

### 34.1 登録コンテナの実体

- コンテナ先頭 `0x00b13970` = C++ 動的配列テンプレート。
  - `+0x04` = 要素配列ポインタ（絶対配置 `0x00b13974`、要素 = メニューポインタ 4B）
  - `+0x08` = size（ushort）
  - `+0x0a` = capacity（ushort、絶対配置 `0x00b1397a`、初期 1）
  - `+0x0c` = 非 null 要素数（short）
- `FUN_004e4a10` = 配列 resize（`operator new`=`FUN_00401f00`、`operator delete`=`FUN_00401f20`）
- `FUN_0042bb90` = 要素アクセサ
- `FUN_00589b70(id)` = ID → vtable getter。`id - 0x3e9 <= 0x32` かつ
  `id < word[0x00b1397a]` のとき `*(int*)(*(int*)0x00b13974 + id*4 - 0xfa4)`（= index `id-0x3e9`）。
- `0x00b13974` 絶対参照 6 箇所: 0x57f363, 0x5836ae, 0x5836ee, 0x589b92, 0x5acb4b, 0xa24911。
- `0x00b1397a` 絶対参照 5 箇所: 0x57f35b, 0x5836a7, 0x5836e6, 0x589b82, 0x5acb43。

### 34.2 ID 空間

- ID 域は `0x3e9`(1001) .. `0x41b`(1051) = 51 スロット、うち 48 使用。
- **真の欠番は 4 個のみ**: `0x405`(1029), `0x407`(1031), `0x409`(1033), `0x41a`(1050)。
- `0x3ee`(1006) は欠番ではなく、`strings.xml` をロードする補助メニュー
  （`FUN_00586000`、ID 即値 `0x3ee` と `0x402` を含有）に割当済み。
- `TileMenu` のみ ID `0x38b`(907) で範囲外。UI タイル基底（非パネル）の特殊メニュー。

### 34.3 全メニュー対応表（確定値）

| ID | Menu | vtable | getID 関数 | ctor 関数 | XML |
|---|---|---|---|---|---|
| 0x3e9 | MessageMenu | 00a6ce5c | 005bc680 | FUN_005bc8b0 | Data\Menus\message_menu.xml |
| 0x3ea | InventoryMenu | 00a6c18c | 005a9ba0 | FUN_005ab5a0 | Data\Menus\Main\inventory_menu.xml |
| 0x3eb | StatsMenu | 00a6d9dc | 005d9d60 | FUN_005dcb70 | Data\Menus\Main\stats_menu.xml |
| 0x3ec | HUDMainMenu | 00a6bee4 | 005a6840 | FUN_005a6b00 | Data\Menus\Main\hud_main_menu.xml |
| 0x3ed | HUDInfoMenu | 00a6bdf4 | 005a4800 | FUN_005a4840 | Data\Menus\Main\hud_info_menu.xml |
| 0x3ee | (StringsMenu) | - | - | FUN_00586000 | Data\Menus\strings.xml |
| 0x3ef | LoadingMenu | 00a6c40c | 005ad370 | FUN_005adcf0 | Data\Menus\loading_menu.xml |
| 0x3f0 | ContainerMenu | 00a6b574 | 00597a80 | FUN_00599ee0 | Data\Menus\container_menu.xml |
| 0x3f1 | DialogMenu | 00a6b88c | 0059e570 | FUN_0059ed30 | Data\Menus\Dialog\dialog_menu.xml |
| 0x3f2 | HUDSubtitleMenu | 00a6c0dc | 005a8e20 | FUN_005a8e30 | Data\Menus\Main\hud_subtitle_menu.xml |
| 0x3f3 | GenericMenu | 00a6bd9c | 005a3cd0 | FUN_005a3ff0 | Data\Menus\Generic |
| 0x3f4 | SleepWaitMenu | 00a6d7c4 | 005d6950 | FUN_005d6d20 | Data\Menus\sleep_wait_menu.xml |
| 0x3f5 | PauseMenu | 00a6d03c | 005bda50 | FUN_005bda90 | Data\Menus\Options\pause_menu.xml |
| 0x3f6 | LockPickMenu | 00a6c73c | 005af300 | FUN_005af440 | Data\Menus\lockpick_menu.xml |
| 0x3f7 | OptionsMenu | 00a6cfac | 005bd640 | FUN_005bd680 | Data\Menus\Options\options_menu.xml |
| 0x3f8 | QuantityMenu | 00a6d15c | 005c0590 | FUN_005c05d0 | Data\Menus\quantity_menu.xml |
| 0x3f9 | AudioMenu | 00a6b29c | 005952f0 | FUN_00595380 | Data\Menus\Options\audio_menu.xml |
| 0x3fa | VideoMenu | 00a6a09c | 00587c10 | FUN_005deb80 | Data\Menus\Options\video_menu.xml |
| 0x3fb | VideoDisplayMenu | 00a6dccc | 005dda10 | 未同定 | (XML なし / video_menu 派生) |
| 0x3fc | GameplayMenu | 00a6bd0c | 005a3420 | FUN_005a3540 | Data\Menus\Options\gameplay_menu.xml |
| 0x3fd | ControlsMenu | 00a69ff4 | 005874f0 | FUN_0059ba40 | Data\Menus\Options\controls_menu.xml |
| 0x3fe | MagicMenu | 00a6c974 | 005b1f60 | FUN_005b3760 | Data\Menus\Main\magic_menu.xml |
| 0x3ff | MapMenu | 00a6cc9c | 005b7480 | FUN_005bb6c0 | Data\Menus\Main\map_menu.xml |
| 0x400 | MagicPopupMenu | 00a6ca54 | 005b3ec0 | FUN_005b3f00 | Data\Menus\Main\magic_popup_menu.xml |
| 0x401 | NegotiateMenu | 00a6cf0c | 005bd040 | FUN_005bd080 | Data\Menus\negotiate_menu.xml |
| 0x402 | BookMenu | 00a6b334 | 00595c30 | FUN_005962c0 | Data\Menus\book_menu.xml |
| 0x403 | LevelUpMenu | 00a6c264 | 005ac5a0 | FUN_005ace20 | Data\Menus\levelup_menu.xml |
| 0x404 | TrainingMenu | 00a6dc74 | 005dd300 | FUN_005dd4b0 | Data\Menus\training_menu.xml |
| 0x406 | ClassMenu | 00a6b49c | 00596ce0 | 未同定 | Data\Menus\Chargen\class_menu.xml |
| 0x408 | SkillsMenu | 00a6d71c | 005d56b0 | FUN_005d6390 | Data\Menus\CharGen\skills_menu.xml |
| 0x40a | PersuasionMenu | 00a6d0cc | 005be230 | FUN_005c01d0 | Data\Menus\dialog\persuasion_menu.xml |
| 0x40b | RepairMenu | 00a6d5b4 | 005d0e40 | FUN_005d2070 | Data\Menus\repair_menu.xml |
| 0x40c | RaceSexMenu | 00a6d38c | 005c4e10 | FUN_005ca010 | Data\Menus\CharGen\race_sex_menu.xml |
| 0x40d | SpellPurchaseMenu | 00a6d8cc | 005d88f0 | FUN_005d9590 | Data\Menus\dialog\spell_purchase.xml |
| 0x40e | LoadgameMenu | 00a6c6bc | 005ae4e0 | FUN_005aea60 | Data\Menus\Options\load_menu.xml |
| 0x40f | SaveMenu | 00a6d624 | 005d3310 | FUN_005d39a0 | Data\Menus\Options\save_menu.xml |
| 0x410 | AlchemyMenu | 00a6b1b4 | 005931b0 | FUN_00593810 | Data\Menus\dialog\Alchemy.xml |
| 0x411 | SpellMakingMenu | 00a6d864 | 005d7600 | 未同定 | Data\Menus\dialog\Spellmaking.xml |
| 0x412 | EnchantmentMenu | 00a6ba0c | 005a1670 | FUN_005a1ec0 | Data\Menus\dialog\Enchantment.xml |
| 0x413 | EffectSettingMenu | 00a6b92c | 0059fc20 | FUN_005a08e0 | Data\Menus\dialog\enchantmentsetting_menu.xml |
| 0x414 | MainMenu | 00a6cb1c | 005b5950 | FUN_005b6040 | Data\Menus\Options\main_menu.xml |
| 0x415 | BreathMenu | 00a6b404 | 00596510 | FUN_005965c0 | Data\Menus\breath_meter_menu.xml |
| 0x416 | QuickKeysMenu | 00a6d1f4 | 005c0ce0 | FUN_005c1290 | Data\Menus\Main\quickkeys_menu.xml |
| 0x417 | CreditsMenu | 00a6b784 | 0059c950 | FUN_0059d680 | Data\Menus\Options\credits_menu.xml |
| 0x418 | SigilStoneMenu | 00a6d664 | 005d4130 | FUN_005d5200 | Data\Menus\dialog\SigilStone.xml |
| 0x419 | RechargeMenu | 00a6d45c | 005ce810 | FUN_005ce840 | Data\Menus\recharge_menu.xml |
| 0x41b | TextEditMenu | 00a6a04c | 00587ab0 | FUN_005dcf20 | Data\Menus\Dialog\TextEditMenu.xml |
| 0x38b | TileMenu | 00a6af14 | 00590190 | - | (タイル基底・XML なし) |

### 34.4 Data\Menus\ XML 文字列（EXE 内 50 件、VA 付き）

`0xa68d7c` hud_reticle.xml / `0xa69078` safe_zone.xml / `0xa691d8` strings.xml /
`0xa69718` CharGen\skills_menu.xml / `0xa6973c` CharGen\race_sex_menu.xml /
`0xa69844` Options\video_menu.xml / `0xa69868` Options\save_menu.xml /
`0xa6988c` Options\pause_menu.xml / `0xa698b0` Options\options_menu.xml /
`0xa698d4` Options\main_menu.xml / `0xa698f8` Options\load_menu.xml /
`0xa6991c` Options\gameplay_menu.xml / `0xa69944` Options\credits_menu.xml /
`0xa69968` Options\controls_menu.xml / `0xa69990` Options\audio_menu.xml /
`0xa699b4` Main\quickkeys_menu.xml / `0xa699d8` Main\magic_popup_menu.xml /
`0xa69a00` Main\hud_subtitle_menu.xml / `0xa69a28` Main\hud_main_menu.xml /
`0xa69a4c` Main\hud_info_menu.xml / `0xa69a70` Dialog\dialog_menu.xml /
`0xa69a94` training_menu.xml / `0xa69ab4` sleep_wait_menu.xml / `0xa69ad4` repair_menu.xml /
`0xa69af0` quantity_menu.xml / `0xa69b10` negotiate_menu.xml / `0xa69b30` message_menu.xml /
`0xa69b4c` lockpick_menu.xml / `0xa69b6c` loading_menu.xml / `0xa69b88` levelup_menu.xml /
`0xa69ba4` container_menu.xml / `0xa69bc4` breath_meter_menu.xml / `0xa69be8` book_menu.xml /
`0xa69c04` dialog\Alchemy.xml / `0xa6a12c` Prefabs\%s / `0xa6b394` workbook.html /
`0xa6b53c` Chargen\class_menu.xml / `0xa6b974` dialog\enchantmentsetting_menu.xml /
`0xa6bc58` dialog\Enchantment.xml / `0xa6bdd8` Generic / `0xa6c238` Main\inventory_menu.xml /
`0xa6ca2c` Main\magic_menu.xml / `0xa6cdb8` Main\map_menu.xml /
`0xa6d12c` dialog\persuasion_menu.xml / `0xa6d4f8` recharge_menu.xml / `0xa6d6f0` dialog\SigilStone.xml /
`0xa6d8a0` dialog\Spellmaking.xml / `0xa6d9a0` dialog\spell_purchase.xml /
`0xa6dbd4` Main\stats_menu.xml / `0xa6dc48` Dialog\TextEditMenu.xml

### 34.5 Android 移植向け確定値

1. **VideoDisplayMenu(0x3fb) は専用 XML を持たない**。EXE の `Data\Menus\` 文字列 50 件に
   video_display 系は存在せず、`video_menu.xml` の派生パネルとして構築される
   （§24.11 の `user3`/`user4` はコード生成。別途 §24.7 参照）。
   → 移植側の `video_display_menu.xml` は独自実装でよく、元データとの一致義務はない。
2. **XML パスは大文字小文字混在**（`CharGen` / `Chargen`、`Dialog` / `dialog`）。
   移植側のパス解決は**大文字小文字を区別せず**引くこと。
3. ID は `0x3e9` 起点の連番。メニュー種別判定に ID を使う場合、
   `0x3ee`（strings）と `0x38b`（Tile）は**通常パネルと別扱い**にすること。

## §35 ファストトラベル実行経路とメッセージ確認ダイアログ（依頼2 補完）

§33.4.2 では `FUN_0066F420` の先頭断片のみ記載していた。本章では
**「マップ画面でマーカーをタップしてファストトラベルが成立するまでの全経路」**と、
その過程で使われる**確認ダイアログ（MessageMenu）の仕組みとローカライズ文言グローバル**を
実バイトまで確定した内容で補完する。

### 35.1 呼び出しチェーン全体（確定）

```
MapMenu 更新/操作
  └─ FUN_005bb880   MapMenu vtable 00a6cc9c[3]（マーカーヒットテスト、2296B）
       ├─ 005bb9a2 MOV EDX,[0x00b38d00]        ; "sNo"
       ├─ 005bb9a8 MOV EAX,[0x00b38cf8]        ; "sYes"
       ├─ 005bb9b7 PUSH 0x5b8dc0               ; コールバック関数ポインタ
       └─ 005bb9bd CALL 0x00579c10             ; 確認ダイアログ表示（ADD ESP,0x28 = 10 dword）
            └─ FUN_00579c10（215B）ShowMessageBox 系 varargs ディスパッチャ
                 ├─ 通常: FUN_005bc8b0（836B）= MessageMenu 表示の実体
                 └─ yes/no: FUN_005bcc00（162B）= 上記 ＋ trait 0x1772 を追加設定

ユーザーが Yes を選択した場合のみ
  └─ FUN_005b8dc0（509B）マーカー走査 → ヒットしたマーカーで
       ├─ 005b8fa9 MOV ECX,[0x00b333c4]        ; Player
       ├─ 005b8faf PUSH EDI                    ; marker
       └─ 005b8fb0 CALL 0x0066f420             ; ファストトラベル実行
```

- `FUN_005b8dc0` を **直接 CALL する箇所は 0 件**。唯一の参照は
  `005bb9b7 type=DATA in FUN_005bb880`（＝関数ポインタとして PUSH される引数）。
  `.rdata`（0xA68000〜0xB02000）全域で dword `0x005b8dc0` を検索してもヒット 0 件であり、
  **静的 vtable ではなく実行時に引数として渡されるコールバック**である。
- したがって **No を選ぶと `FUN_005b8dc0` が呼ばれないためファストトラベルは不成立**。

### 35.2 `FUN_005b8dc0`（509B）= マーカーヒット判定と実行

- ガード: `FUN_00578d70() == 1` かつ `FUN_00589b70(0x3ff)`（MapMenu）非 0。
- MapMenu のマーカーリスト（`+0xC4`）を走査し、各マーカーの
  `(**(code**)(*piVar1 + 0x174))()` で位置 3 float を取得 →
  マップ矩形で正規化 → `__stricmp` でセル名／ワールド名一致、
  さらに浮動小数座標の一致でヒット判定。
- ヒット時: `FUN_00588bd0(0xfa8)` / `FUN_0058ceb0(0xfa1, 1.0f)` /
  `FUN_00582160(0,1,1,0)` / `FUN_0057cfe0(1,0)` / `FUN_00583e60()` /
  `FUN_005821f0()` / `FUN_00583f40()` → `FUN_0066f420(piVar1)`。

### 35.3 `FUN_0066f420`（2410B）= ファストトラベル実行本体

- 署名: `void __thiscall FUN_0066f420(int *param_1, int *param_2)`
  - `param_1` = 実行主体（呼び出し元は `0x00b333c4`（Player）を ECX で渡す）
  - `param_2` = 目的マーカーオブジェクト
- 先頭ガード: `(**(code**)(*param_1 + 0x25c))() != 0` なら即 return。
- オートセーブ: `if (DAT_00b13228 != 0) FUN_00466b70();`（§33.4.2 参照）
- 座標・セル移動:
  `FUN_005f0810(param_2, &fStack_44, x, y, z)` → `FUN_004d8a30(x,y,z)` →
  `FUN_004dd4b0(ref, cell, worldspace)` → `FUN_004d8af0()`（現在セル取得）。
- 所要時間:
  `iVar5 = FUN_0046b250(0x3a)`（GMST）→ `fStack_40 = *(float*)(iVar5 + 0x24)`;
  `duration = fStack_40 * ((fStack_44 / fVar12) / _DAT_00a2f938)`;
  `if (bVar2) duration *= _DAT_00a2faa0;`
- フェードループ: `param_1[0x164]`（= +0x590）をカウントダウンしつつ
  `FUN_0057b950(0, _DAT_00a309f0 - (_DAT_00a309f0 * param_1[0x164]) / fStack_40)`（1 → 0）。
  ループ後に `[0x164] = 10` → `FUN_00676940(...)` → `[0x164] = 0`。
- 移動中フラグ: `*(byte*)(DAT_00b333c4 + 0x4b)` を 1 → 0 に、移動後 `+0x45 = 0`。
- 最終配置: `DAT_00b3a6d2` を 1→0 で挟み
  `FUN_0066f370(x, y, z, param_2[8], param_2[9], param_2[10], uVar16, 0)`。
  `param_2[8]`〜`[10]`（= +0x20..+0x28）はマーカーの回転 3 成分と推定。
- プレイヤー側で併せて使用: `+0x116` / `+0x4b` / `+0x45` / `+0x12c` /
  `+0x734`（難易度） / `+0x1E0`（`param_1[0x78]`） / `+0x590`（`param_1[0x164]`） /
  `+0x760`（`param_1[0x1d8]`） / `+0x764`（`param_1[0x1d9]`）。
- 末尾: `FUN_00679a70(); FUN_00677ec0(0, 0); (**(code**)(*param_1 + 0x2ec))();`
  `[0x1d9]` 非 0 なら `FUN_006b73e0()` → `FUN_00401f20(ptr)` → 両フラグを 0 に。

### 35.4 MessageMenu 確認ダイアログの仕組み

- `FUN_00579c10`（215B）= 確認メッセージボックス表示ヘルパー。内部で
  `FUN_00582160(0,1)`（MenuManager 取得）を 5 回呼び、`[+0x1c]` / `[+0x68]` の非 0 を確認した後、
  `FUN_00588bd0(0xfae) == 1.0f` の真偽で分岐:
  - 真 → `FUN_005bc8b0(...)`（通常メッセージ）
  - 偽 → `FUN_005bcc00(...)`（yes/no ラッパ）
- `FUN_005bc8b0(char *param_1 /*本文*/, undefined4 param_2, undefined1 param_3,
  char *param_4 /*ボタン文言*/, int *param_5 /*va_list*/)`（836B）= MessageMenu 表示の実体。
  - `FUN_00589b70(0x3e9)` で既存 MessageMenu を閉じる
  - `FUN_00582160(0,1)` → `FUN_00584980()`（フェード時間） → `FUN_005903e0()` →
    `FUN_00590420("Data\Menus\message_menu.xml")`
  - `FUN_009832e6(..., &PTR_PTR_00b13240, &PTR_PTR_00b13a74, 0)` と
    `(..., &PTR_PTR_00b0a018, &PTR_PTR_00b1328c, 0)` で trait テーブル取得。
    `[+0x2c]` / `[+0x30]` / `[+0x28]` が 0 なら失敗
    （文言: `"Message Menu Creation Failed... Are your menu and art resources up to date?"` @ `0xa6ceb8`）。
  - 本文が空 → `DAT_00b38e10`（"sDefaultMessage" =
    "DEFAULT MESSAGE TEXT"）、ボタン文言が NULL → 非表示（`FUN_0058ceb0(0xfa1, 1.0f)`）、
    空 → `DAT_00b38cf0`（"sOk" = "Ok"）、それ以外 → 指定値。
  - `param_5` はボタンリスト配列。4 ボタン目以降は `FUN_005bc5e0(iVar10)` で生成し、
    失敗時は `"Missing MessageMenu button"`。
  - 終盤 `FUN_0057de50(0xb)` でメニュー表示、return 1。
- `FUN_005bcc00`（162B）= `FUN_005bc8b0` を呼んだ後に trait `0x1772` を追加設定する薄いラッパ
  （yes/no 確認用）。`FUN_0057ea20(uVar3, 1.0f, 0.0f)`、`[+0x24] = 1`。

**MessageMenu の trait ID**

| trait | 意味 |
|---|---|
| `0xfde` | テキスト |
| `0xfa1` | 可視 / alpha |
| `0xfab` | フェード時間 |
| `0xfae` | フラグ（`*(char*)(iVar4+8) != 1`） |
| `0x1772` | yes/no ラッパ用追加設定 |

### 35.5 ローカライズ文言グローバル（全 9 件・実バイト確定）

静的初期化関数は `0x009f2960` 〜 `0x009f2a5a` に連続する小関数群 ＋ `0x009f2e20`。
各サイトは共通して次の命令列をとる。

```
PUSH <既定文言>
PUSH <キー>
MOV  ECX, <対象グローバル>
CALL 0x00419A70          ; ローカライズ文字列グローバル初期化
PUSH <テーブルエントリ>
CALL 0x00981FB4          ; 初期化完了通知／登録
POP  ECX
RET
```

| 対象グローバル | キー（arg1） | 既定文言（arg2） | 初期化サイト |
|---|---|---|---|
| `0x00b38ce0` | `0x00A60EB8` "sVideoChange" | `0x00A60EC8` "You must exit and restart Oblivion for the new resolution to take effect." | `0x009f2960`（CALL 0x419a70 @ 0x9f296f / 0x981fb4 @ 0x9f2979、テーブル `0x00A220E0`） |
| `0x00b38ce8` | `0x00A60F14` "sMustRestart" | `0x00A60F28` "You must exit and restart Oblivion for changes in resolution to take effect." | `0x009f2980`（@ 0x9f298b / 0x9f2999、`0x00A220F0`） |
| `0x00b38cf0` | `0x00A60F78` **"sOk"** | `0x00A60F7C` **"Ok"** | `0x009f29a2`（@ 0x9f29af / 0x9f29b9、`0x00A22100`） |
| `0x00b38cf8` | `0x00A60F80` "sYes" | `0x00A3DAE8` "Yes" | `0x009f29c2`（@ 0x9f29cf / 0x9f29d9、`0x00A22110`） |
| `0x00b38d00` | `0x00A60F88` "sNo" | `0x00A3DAF4` "No" | `0x009f29e2`（@ 0x9f29ef / 0x9f29f9、`0x00A22120`） |
| `0x00b38d08` | `0x00A60F8C` "sCancel" | `0x00A60F94` "Cancel" | `0x009f2a02`（@ 0x9f2a0f / 0x9f2a19、`0x00A22130`） |
| `0x00b38d10` | `0x00A60F9C` "sFor" | `0x00A325E8` "for" | `0x009f2a22`（@ 0x9f2a2f / 0x9f2a39、`0x00A22140`） |
| `0x00b38d18` | `0x00A60FA4` "sTo" | `0x00A60FA8` "to" | `0x009f2a42`（@ 0x9f2a4f / 0x9f2a59、`0x00A22150`） |
| `0x00b38e10` | `0x00A612B4` "sDefaultMessage" | `0x00A612C4` "DEFAULT MESSAGE TEXT" | `0x009f2e20` |

- `FUN_00419A70` = **ローカライズ文字列グローバル初期化関数**
  （呼び規約: `ECX` = 対象グローバル、スタック引数 2 = キー、既定文言）。
- `FUN_00981FB4` = **初期化完了通知／登録関数**（第 1 引数 = 動的初期化テーブル領域の
  エントリ VA。領域は `0x00A220E0` 〜 `0x00A22150`）。
- **重要（訂正）**: `0x00b38cf0` は "Message" ではなく **"sOk"（既定 "Ok"）** である。
  `0x00a612bc` の "Message" は別リテラル（本ブロックの対象外）。
- 参照数（絶対 dword）: `0x00b38cf0` = 65 / `0x00b38cf8` = 31 / `0x00b38d00` = 32 /
  `0x00b38d08` = 4 / `0x00b38d10` = 6 / `0x00b38e10` = 3。
  **いずれも .text からの絶対アドレス書き込みは 0 件**（BSS ＋ 実行時の静的初期化）。
  各グローバルで「アドレス値ロード」としてヒットするのは初期化サイト 1 件と
  動的初期化テーブル内の 1 件のみ。
- 副次メモ: `0x00b38cf8` のヒット一覧に VA `0x00A22111` が現れるのは、
  動的初期化テーブルの命令バイトを誤整列で拾ったものである
  （`0x00A22116` に `b9 f8 8c b3 00` が存在）。

**リテラル帯の並び（`0x00A60EF4` 〜 `0x00A60FB4`）**

```
"...ew resolution to take effect." / "sMustRestart" / "You must exit and restart Oblivion for c..."
"sOk" / "Ok" / "sYes" / "sNo" / "sCancel" / "Cancel" / "sFor" / "sTo" / "to" / "sGold"
```

`0x00A612B4` = "sDefaultMessage" / `0x00A612BC` = "Message" / `0x00A612C4` = "DEFAULT MESSAGE TEXT" /
`0x00A3DAE8` = "Yes" / `0x00A3DAF4` = "No" / `0x00A325E8` = "for"。

### 35.6 Android 移植向け確定値

1. **マップ操作の正しい挙動**: マーカータップ → 確認ダイアログ
   （本文 `"%s %s?"`、ボタン "sYes"/"sNo"、コールバック `0x5b8dc0`）→
   **Yes のときのみファストトラベル開始、No では何も起きない**。
2. **MessageMenu の既定文言**: 本文未指定 = "sDefaultMessage"
   （"DEFAULT MESSAGE TEXT"）、ボタン未指定 = **"sOk"（"Ok"）**。ボタンは 4 番目の
   配列要素から生成される（`FUN_005bc5e0`）。
3. **文言キー一覧（移植側のローカライズ表に登録すべきキー）**:
   `sOk` / `sYes` / `sNo` / `sCancel` / `sFor` / `sTo` / `sMustRestart` /
   `sVideoChange` / `sDefaultMessage`。
4. **ファストトラベル開始時に `bSaveOnTravel` 真なら自動セーブを 1 回発行**
   （`FUN_00466b70`、§33.4.2 と同一事実）。
5. **移動演出**: GMST `0x3a` の `+0x24`（秒）を基準に、
   距離比（`_DAT_00a2f938`）で duration を算出し、
   `param_1[0x164]`（+0x590）をカウントダウンしながら
   `FUN_0057b950` で画面フェード係数を 1 → 0 に落とす。
   移植側は「移動シーケンス中はプレイヤー入力を止め、`+0x4b` 相当の
   移動中フラグを立てる」挙動を再現すればよい。
6. **副産物（依頼3 のフットノート文言の出所）**: 解像度変更時の文言
   `sVideoChange` / `sMustRestart` の実体を本章で確定した
   （`0x00A60EB8` / `0x00A60EC8` / `0x00A60F14` / `0x00A60F28`）。
   video_display のフットノートはこの 2 キーを使う。


## §36 video_display 解像度リスト生成の完全仕様（依頼3・A3）

§30.3 / §26.9 で「コード設定」とだけ記していた `user4`（解像度テキスト）と
`user3`（フットノート表示可否）の生成元を確定した。解像度候補は **XML ではなく
Direct3D 9 のアダプタモード列挙から実行時に構築**される。

### 36.1 結論

1. 解像度リストは `VideoDisplayMenu`（ID `0x3fb`、vtable `0x00a6dccc`）の
   コンストラクタ `FUN_005dd960` ではなく、**基底 `Menu` の XML ロード経路で
   `FUN_005deb80`（VideoMenu 系 ctor、1499 バイト）が構築**する。
   `FUN_005deb80` の冒頭で `FUN_005de920()` を呼び、続けて
   Direct3D 9 のモード列挙ループを回す。
2. 列挙に使うのは **`IDirect3D9` の COM vtable**
   （`GetAdapterModeCount` / `EnumAdapterModes`）であり、
   `EnumDisplaySettings` は使っていない。
3. リストに載るのは **重複のない (幅, 高さ) のうち `幅 * 高さ >= 0x4B000`
   （= 307200 = 640x480 と等価以上）** のモードのみ。
4. 初期選択は **実適用中の解像度（`DAT_00b06c4c` / `DAT_00b06c50`、初期値
   640x480）と一致するノード**。見つからなければリスト先頭。

### 36.2 Direct3D9 シングルトン

| 項目 | 値 |
|---|---|
| `DAT_00b42154` | `IDirect3D9*`（BSS、初期値 0） |
| `DAT_00b42150` | `LoadLibraryA("D3D9.DLL")` の戻り HMODULE |
| `DAT_00b42158` | `GetProcAddress(h, "Direct3DCreate9")` の関数ポインタ |
| `DAT_00b42160` | 関連文字列／リソース（`FUN_00775f10` → `FUN_00401f20` で解放） |

生成 `FUN_00761df0`（109 バイト、遅延初期化）:

```
if (DAT_00b42158 == 0) {
    DAT_00b42150 = LoadLibraryA("D3D9.DLL");
    DAT_00b42158 = GetProcAddress(DAT_00b42150, "Direct3DCreate9");
}
if (DAT_00b42154 == 0)
    DAT_00b42154 = (*DAT_00b42158)(0x20);   // D3D_SDK_VERSION = 0x20
return DAT_00b42154;
```

破棄 `FUN_007645c0`（98 バイト）:

```
if (DAT_00b42154 != 0) {
    (**(code **)(*DAT_00b42154 + 8))(DAT_00b42154);  // IDirect3D9::Release()
    DAT_00b42154 = 0;
}
if (DAT_00b42160 != 0) { FUN_00775f10(); FUN_00401f20(DAT_00b42160); }
DAT_00b42160 = 0;
if (DAT_00b42150 != 0) FreeLibrary(DAT_00b42150);
DAT_00b42158 = 0;
FUN_00573880(0xb28e00);
```

- `+8` は COM 規約どおり **Release**（`+0`=QueryInterface、`+4`=AddRef）。
- 関連アドレス: 生成 `0x00761e33` / 破棄 `0x007645d2` / 生存確認 `0x007645c0`。

**IDirect3D9 COM vtable（オフセット → メソッド）**

| offset | slot | メソッド |
|---|---|---|
| 0x00 | 0 | QueryInterface |
| 0x04 | 1 | AddRef |
| 0x08 | 2 | Release |
| 0x0c | 3 | RegisterSoftwareDevice |
| 0x10 | 4 | GetAdapterCount |
| 0x14 | 5 | GetAdapterIdentifier |
| **0x18** | **6** | **GetAdapterModeCount** |
| **0x1c** | **7** | **EnumAdapterModes** |

### 36.3 列挙ループ（`FUN_005deb80` 冒頭）

```c
FUN_005de920();                              // 解像度テキスト初期化（後述）
uVar7 = (*GetAdapterModeCount)(DAT_00b42154, DAT_00b06c54, 0x16);
                                             // Adapter = DAT_00b06c54, Format = 0x16
if (uVar7 != 0) {
    uVar9 = 0;
    do {
        (*EnumAdapterModes)(DAT_00b42154, DAT_00b06c54, 0x16, uVar9, &mode);
        for (p = *(this + 0x104); ; p = *p) {
            if (p == 0) {                    // 末尾まで到達 = 重複なし
                if (0x4afff < (uint)(mode.h * mode.w))
                    FUN_005dea80(&mode);     // リスト先頭へ挿入
                break;
            }
            if (mode.w == p[2] && mode.h == p[3]) break;   // 重複 -> 挿入しない
        }
        uVar9++;
    } while (uVar9 < uVar7);
}
for (p = *(this + 0x104); ; p = *p) {
    if (p == 0) break;
    if (DAT_00b06c4c == p[2] && DAT_00b06c50 == p[3]) break;  // 実適用中を探す
}
*(this + 0x110) = (p != 0) ? p : *(this + 0x104);   // 現在選択ノード
FUN_005dead0();
```

| 定数 | 値 | 意味 |
|---|---|---|
| `0x16` | 22 | `D3DFMT_X8R8G8B8` |
| `DAT_00b06c54` | **0**（実測） | Adapter 番号。書き込み命令が無いため **0 固定** |
| `0x4B000` | 307200 | 最小面積（640x480 と等号で通過） |
| `DAT_00b06c4c` / `DAT_00b06c50` | 640 / 480（実測初期値） | 実適用中の幅 / 高さ |

`D3DDISPLAYMODE` は 16 バイトで `[+0]=Width` / `[+4]=Height` /
`[+8]=RefreshRate` / `[+0xc]=Format`。ループ側は先頭 8 バイトのみ参照する。

### 36.4 リスト構造（`NiTList<VideoRes>`）

| オフセット | 内容 |
|---|---|
| `this + 0x100` | `NiTList` オブジェクト本体（埋め込み） |
| `this + 0x104` | head ポインタ |
| `this + 0x108` | tail ポインタ |
| `this + 0x10c` | count |
| `this + 0x110` | 現在選択ノード |

ノードは 16 バイト: `[0]=next` / `[+4]=prev` / `[+8]=幅` / `[+0xc]=高さ`。
挿入は `FUN_005dea80(list, &res)` = **先頭挿入**（`NiTList` の vtable は
`0x00a6a08c`、アロケーター vtable は `0x00a69fe0`）。

### 36.5 テキスト生成とワイドスクリーン判定

`FUN_005dead0`（169 バイト）が `user4` を生成する。

```
sprintf(buf, "%d x %d", node[2], node[3]);
if ((uint)(h * 4) < (uint)(w * 3))          // 4:3 より横長 -> ワイド
    trait(0xfb1) = 2; else trait(0xfb1) = 1;
trait(0xfb2) = buf;                          // 解像度テキスト（user4）
if (widescreen) strcat(buf, "_widescreen");  // PTR_DAT_00a6dd24
```

| trait | 意味 |
|---|---|
| `0xfb1` | ワイド判定値（2 = ワイド、1 = 標準） |
| `0xfb2` | 解像度テキスト `"%d x %d"`（+ `_widescreen`） |
| `0xfb3` | 数値 / スライダー値（`FUN_005de920` が `"%dx"` を設定） |
| `0xfde` | テキスト（有効 / 無効は `0x00b38da0` / `0x00b38da8`） |
| `0xfaf` / `0xfb0` | スライダー min / max |
| `0x1772` | VideoMenu のトグル状態 |

`user3`（フットノート表示可否）は、解像度変更時のみ
`FUN_00579c10(sVideoChange, 0, 1, sOk, 0)` を出す経路で決まる。
`FUN_005ddaa0`（解像度適用本体、115 バイト）:

```c
FUN_00497c10(w, h);                                  // 設定グローバルへ書き込み
trait(0x1772) = committed;
FUN_00584740();                                      // メニュー閉じ
FUN_00579c10(sVideoChange, 0, 1, sOk, 0);            // 再起動要求ダイアログ
FUN_005ddca0();                                      // VideoMenu 再構築
```

`FUN_00497c10`（20 バイト）は `DAT_00b06c5c = w` / `DAT_00b06c64 = h` を
書くだけの設定グローバルセッター。**実適用値（`DAT_00b06c4c` / `0x50`）とは別物**
であり、実適用は次回起動時に行われる（これが sVideoChange の理由）。

### 36.6 固定解像度スイッチ（`FUN_005ddb70`, VideoDisplayMenu slot[3]）

切り替え操作 `4..8` は固定のプリセットを直接 `FUN_005ddaa0` へ渡す。

| 操作 | 幅 | 高さ |
|---|---|---|
| 4 | 800 | 600 |
| 5 | 1080 | 607 |
| 6 | 1024 | 768 |
| 7 | 1280 | 607 |
| 8 | 1280 | 720 |

`607` は 16:9 近似（1080x607 / 1280x607）のための値。

### 36.7 Android 移植向け確定値

1. **候補リストの作り方**: XML に解像度を書くのではなく、
   実行時にディスプレイモードを列挙して構築する。Android では
   `Display.getSupportedModes()` を `w * h >= 307200` かつ (幅, 高さ) 重複除去で
   フィルタすれば一致する。列挙順は D3D の列挙順（解像度昇順）に合わせ、
   挿入は先頭ではなく **昇順ソート** で代替してよい（表示順は同じ）。
2. **初期選択**: 現在の解像度と一致する項目。無ければ先頭。
3. **`w * h >= 307200` は等号通過**（640x480 は入る）。
4. **アダプタは 0 のみ**（マルチモニタ非対応）。Android もプライマリディスプレイ固定。
5. **解像度変更は「次回起動で適用」**。設定グローバル（`+0xc5c` / `+0xc64` 相当）に
   書くだけで、実適用値は別フィールド。変更時は「再起動が必要」ダイアログ
   （`sVideoChange` + `sOk`）を出し、VideoMenu を閉じる。
6. **ワイド判定は `h * 4 < w * 3`**。真なら `_widescreen` サフィックスの
   表示テキストを使う。
7. **Direct3D9 は不要**。移植側は D3D9 のライフサイクル（`FUN_00761df0` /
   `FUN_007645c0`）を再現する必要はなく、`VideoDisplayMenu.slot[3]` の
   プリセット値（800x600 / 1080x607 / 1024x768 / 1280x607 / 1280x720）だけ
   移植すれば挙動は一致する。

### 36.8 関連関数一覧（A3）

| 関数 | サイズ | 役割 |
|---|---|---|
| `FUN_00761df0` | 109 | Direct3D9 シングルトン遅延生成 |
| `FUN_007645c0` | 98 | Direct3D9 解放 / `DAT_00b42154` 破棄 |
| `FUN_005deb80` | 1499 | VideoMenu 系 ctor。冒頭で解像度列挙ループ |
| `FUN_005dea80` | 72 | `NiTList<VideoRes>` 先頭挿入 |
| `FUN_005de920` | 64 | `"%dx"` → trait `0xfb3` |
| `FUN_005dead0` | 169 | `"%d x %d"` + `_widescreen` → trait `0xfb2` |
| `FUN_005ddaa0` | 115 | 解像度適用本体（`FUN_00497c10` → 確認 → 閉じ） |
| `FUN_00497c10` | 20 | 設定解像度グローバルセッター |
| `FUN_005ddb70` | 211 | slot[3] 解像度確定 switch（プリセット 5 種） |
| `FUN_005dde20` | 125 | VideoMenu 閉じ＋`DAT_00b3b740` 設定 |

---

## §37 メニュー開閉フェードと入力抑制の実装詳細（A4）

§30.4 / §33.3 のフェード記述を、Ghidra デコンパイル結果と実バイト走査で
全面的に裏取りし直したもの。**§33.3 の `FUN_00588a70` の説明は誤り**（37.11 で訂正）。

対象関数（すべて `Oblivion.exe` 実測、VA は imagebase 0x400000 基準）:

| 関数 | サイズ | 役割 |
|---|---|---|
| `FUN_00584300` | 134 | フェードノード（20B）をリスト末尾へ追加 |
| `FUN_00583c30` | 180 | フェード更新本体（毎フレーム tick） |
| `FUN_00583cf0` | 86 | 対象のフェードノードをリストから除去＋解放 |
| `FUN_00584390` | 107 | メニュー「閉じ」のフェード要求 |
| `FUN_00584740` | 216 | メニューを実際に閉じる（親メニュー復帰付き） |
| `FUN_00587550` | 627 | メニュー ID 指定クローズ（4 ID のみフェード付き） |
| `FUN_00584880` | 221 | メニューの有効化／表示登録 |
| `FUN_00588a70` | 76 | 24B ノード生成＋対象 `+0x28` リストへ登録 |
| `FUN_00589980` | 277 | 汎用 trait／リスト値セッター |
| `FUN_00585190` | 115 | 閉じヘルパー |
| `FUN_00591ca1` | 144 | メニュー表示登録＋キー割当コンテナ登録 |
| `FUN_00584980` | 221 | メニューオフセット集計 |
| `FUN_005898f0` | 55 | 現在ルートメニュー取得 |

### 37.1 `explorefade` と `menufade` の使い分け

`menus/*.xml` のルート要素属性を全走査した結果（`tmp\bsa_misc_full\menus\`）。

**`menufade`（ポーズ系メニュー。ゲーム時刻を止める）**

| 値 | メニュー |
|---|---|
| `0.25` | `book` / `class` / `container` / `ingredient_select` / `levelup` / `recharge` / `repair` / `training` / `birthsign` / `skills` / `alchemy` / `enchantment` / `enchantmentsetting` / `sigilstone` / `spell_purchase` / `spellmaking` / `hud_main` / `hud_reticle` / `inventory` / `magic` / `map` / `quickkeys` / `safe_zone` / `stats` / `save` / `load` |
| `0.75` | `persuasion` / `dialog` |
| `0.15` | `hud_info` / `hud_subtitle` |
| `0.0` | `loading`（フェードさせない） |

**`explorefade`（探索中メニュー。ゲーム時刻は止めない）**

| 値 | メニュー |
|---|---|
| `0.25` | `lockpick` / `message` / `negotiate` / `quantity` / `sleep_wait` / `race_sex` / `texteditmenu` / `quest_added` / `skill_perk` / `test_menu` / `audio` / `controls` / `main` / `downloads` / `gameplay` / `load` / `options` / `pause` / `save` / `video_display` / `video` / `xcontrols` |

- **判定基準**: ポーズするか否か。`menufade` を持つのがポーズ系、
  `explorefade` を持つのが非ポーズ系。`hud_*` だけは別枠で `menufade`。
- **値は「秒（duration）」**。`FUN_00584300` の第 2 引数 `param_2` にそのまま入る。
- 移植側は XML の属性名をそのままキーとして読み、閉じ時も同じ値を使えばよい。

### 37.2 フェードノードのレイアウトと追加（`FUN_00584300`）

```
フェードノード（20 バイト = 0x14）
  +0x00  int   対象オブジェクト（Menu インスタンス）
  +0x04  float current   （進捗。初期 0.0）
  +0x08  float duration  （XML の menufade / explorefade 値。既定 0.001f）
  +0x0c  node* prev
  +0x10  node* next
```

処理順:

1. まず `FUN_00583cf0(param_1)` を呼び、**同一対象の既存ノードを除去**（重複防止）。
2. `FUN_00401f00(0x14)` で 20 バイト確保。
3. `[0]=0` / `[+0x4]=0` / `[+0xc]=0` / `[+0x10]=0` /
   `[+0x8]=DAT_00a37080`（= **0.001f**、既定 duration）に初期化。
4. `[0]=param_1`（対象）。`param_2 <= 0.0` なら 0.0 にクランプして `[+0x8]=param_2`。
5. **末尾に append**（先頭ではない）。`puVar3[+0xc] = 旧 tail`、
   `旧 tail->+0x10 = puVar3`、リスト tail を `puVar3` に更新。
6. `FUN_00584390` からは追加直後に `FUN_00583c30()`（フェード更新）を即 1 回呼ぶ。

リストの実体は MenuManager 経由で参照する:

```
MenuManager = *(int**)0x00b3a6e0
  リスト構造体 = *(void**)(MenuManager + 0x130)
    構造体 + 0x0c = tail
    構造体 + 0x10 = head
```

### 37.3 フェード更新本体（`FUN_00583c30`）

`__fastcall` で `ECX` = MenuManager。全ノードを先頭から走査し、毎フレーム進める。

```c
void FUN_00583c30(int menu_manager) {
  node* n = *(node**)(*(int*)(menu_manager + 0x130) + 0x10);   // head
  while (n != 0) {
    float cur = *(float*)(n + 0x04) + _DAT_00b33e9c;  // ★ 歩進量はグローバル float
    *(float*)(n + 0x04) = cur;
    float ratio = cur / *(float*)(n + 0x08);          // current / duration
    if (ratio < 1.0f) {                                // 未完了 → 次へ
      n = *(node**)(n + 0x10);
    } else {                                           // 完了 → ノード自動削除
      node* next = *(node**)(n + 0x10);
      *(node**)(*(int*)(n + 0x0c) + 0x10) = next;      // prev->next = next
      if (next == 0)
        *(int*)(*(int*)(menu_manager + 0x130) + 0x0c) = *(int*)(n + 0x0c);  // tail 更新
      else
        *(int*)(next + 0x0c) = *(int*)(n + 0x0c);      // next->prev = prev
      FUN_00401f20(n);                                 // 解放
      n = next;
    }
  }
}
```

実命令列（実測）:

```
00583c31  MOV EAX,[ECX+0x130]
00583c37  MOV ECX,[EAX+0x10]      ; head
00583c41  FLD  dword [ECX+0x4]
00583c44  FADD dword [0x00b33e9c] ; ★ 歩進量
00583c52  FST  dword [ECX+0x4]
00583c57  FDIV dword [ECX+0x8]
00583c5a  FCOMP ST2               ; 1.0 との比較
00583c66  FCOMP qword [0x00a2fc68] ; = double 0.0
00583ca9  MOV [EDX+0x10],ESI      ; prev->next
00583cb2  MOV [ESI+0xc],EAX       ; next->prev
00583cb5  CALL 0x00401f20         ; 解放
00583ccf  MOV EDX,[ECX+0xc]       ; tail
00583cd3  MOV [EAX+0xc],EDX       ; tail 更新
```

**確定事項**

1. **補間はリニア**。`current += 歩進量` のみで、イージング（S 字・放物線）は無い。
2. **完了判定は `ratio >= 1.0`**。`0x00a2fc68` は `.rdata` の `double 0.0`
   （`FCOMP` の後始末用）。実際の完了は `1.0` 側。
3. **完了したノードは自動的にリストから外れて解放される**。呼び出し側で
   明示的に消す必要はない（`FUN_00583cf0` は「追加時の重複除去」用）。
4. **全画面黒クアッドの描画はフェードモジュールには存在しない**。
   フェードノードは `current` / `duration` を持つだけで、暗転の描画は
   **消費者側**（HUD・メニュー描画）が `current / duration` をアルファに使う。
5. **歩進量はグローバル float 変数 `_DAT_00b33e9c`**（後述 37.9）。

### 37.4 フェードノードの除去（`FUN_00583cf0`）

```c
void FUN_00583cf0(int target) {
  node* n = *(node**)(*(int*)(DAT_00b3a6e0 + 0x130) + 0x10);  // head
  while (n != 0) {
    if (*(int*)n == target) break;      // [0] == 対象
    n = *(node**)(n + 0x10);            // next
  }
  if (n == 0) return;
  node* next = *(node**)(n + 0x10);
  *(node**)(*(int*)(n + 0x0c) + 0x10) = next;       // prev->next = next
  if (next != 0) *(node**)(next + 0x0c) = *(int*)(n + 0x0c);
  if (*(int*)(n + 0x10) == 0)                        // 末尾だった場合
    *(int*)(*(int*)(DAT_00b3a6e0 + 0x130) + 0x0c) = *(int*)(n + 0x0c);  // tail 更新
  FUN_00401f20(n);                                   // 解放
}
```

実命令列: `00583cf0 MOV EAX,[0x00b3a6e0]` → `00583cf5 MOV ECX,[EAX+0x130]` →
`00583cfb MOV EAX,[ECX+0x10]`（head）→ `00583d0a MOV EAX,[EAX+0x10]`（next）→
`00583d1a MOV [EDX+0x10],ECX` → `00583d22 MOV [ECX+0xc],EDX` →
`00583d3a MOV [EDX+0xc],ECX`（tail）→ `00583d41 JMP 0x00401f20`。
`FUN_00584300`（append）の完全な逆操作。

### 37.5 メニューの有効化（`FUN_00584880`）と trait `0xfa5` のゲート

```c
void __thiscall FUN_00584880(int* self, int flag) {
  self[1] = flag;                       // +0x4 = flag
  if (flag == 0) return;
  float v = FUN_00588bd0(0xfa5);        // trait 0xfa5 の float 値
  if (v != _DAT_00a69778 && v != _DAT_00a69770 &&
      v != _DAT_00a69768 && v != _DAT_00a69764) return;   // 4 種以外は登録しない
  int id = (**(code**)(*self + 0x34))();   // vtable slot[13] = getID
  self[5] = id;                            // +0x14 = getID
  int mm = FUN_00582160(0, 1);             // MenuManager 取得
  int id2 = (**(code**)(*self + 0x34))();
  FUN_0057d640(id2);                       // ID 指定で登録／アクティブ化
  if (*(int**)(mm + 0x9c) != self) {
    if (*(int*)(mm + 0x98) != 0) FUN_0058ceb0(0xfdd, 0);
    *(int*)(mm + 0x98) = 0;                // 直前メニュー
    *(int*)(mm + 0x9c) = 0;                // 現在メニュー
  }
}
```

実命令列: `0058488a MOV [ESI+0x4],EDI` → `00584893 PUSH 0xfa5` →
`0058489a CALL 0x00588bd0` → `0058489f FCOMP qword [0x00a69778]` →
`005848b8 FCOMP dword [0x00a69770]` → `005848d1 FCOMP qword [0x00a69768]` →
`005848ea FCOMP dword [0x00a69764]` → `005848f7 MOV EAX,[ESI]` →
`005848f9 MOV EDX,[EAX+0x34]` → `005848fe CALL EDX`（**slot 13 = getID**）→
`00584904 MOV [ESI+0x14],EAX` → `00584907 CALL 0x00582160` →
`0058491d CALL 0x0057d640` → `00584922 CMP [EDI+0x9c],ESI` →
`0058493a PUSH 0xfdd` → `0058493f CALL 0x0058ceb0` →
`00584944 MOV [EDI+0x98],0` → `0058494e MOV [EDI+0x9c],0` → `0058495a RET 0x4`。

**trait `0xfa5` の比較値は 4 種で、`.rdata` 上で float と double がオーバーラップ配置されている**:

| VA | 生 8 バイト | float として | double として |
|---|---|---|---|
| `0x00a69764` | `00 B8 BB 45 00 00 00 00` | **6007.0** | 6.7207e-315（無意味） |
| `0x00a69768` | `00 00 00 00 00 C0 59 40` | 3.40234375 / 0.0 | **103.0** |
| `0x00a69770` | `00 B0 BB 45 00 00 00 00` | **6006.0** | 6.7206e-315（無意味） |
| `0x00a69778` | `00 00 00 00 00 80 59 40` | 3.3984375 / 0.0 | **102.0** |

→ 実質の比較値は **6007 / 6006（float）と 103.0 / 102.0（double）** の 4 種。
trait `0xfa5` は「メニュー種別」を表す列挙（float 値として格納し、
`6006/6007` 系と `102/103` 系の 2 系統が存在する）。
**移植側はこの 4 値ゲートを再現する必要はなく**（内部登録の都合）、
「メニューを開く／閉じる」という外部挙動だけ合わせればよい。

### 37.6 メニューを閉じるときのフェード

**`FUN_00584390(this)`（閉じフェード要求、107B）**

- `this + 0x4 == 0` なら duration = `_DAT_00a41304` = **0.25f**。
- `this + 0x4 != 0` なら trait **`0xfdb`**、無ければフォールバック **`0xfdc`** の値。
- `FUN_00584300(this, duration)` でノード追加 →
  `this + 0x24 = 8`（メニュー状態＝閉じ開始）→ `FUN_00582160(0,1)` →
  **`FUN_00583c30()` を即 1 回呼ぶ**。
- 呼び出し元は `0x5843d0` の 1 箇所（`FUN_00584390` 自身は
  `FUN_00584740` 系から呼ばれる）。

**`FUN_00584740(this)`（実際に閉じる、216B）**

- trait `0xfa1` ガード（`_DAT_00a379b4` = **2.0f**）、trait **`0x1772`**（`2.0f`）、
  trait **`0xfdd`** を参照。
- `this + 0x14` = 親メニュー、`this + 0x24` = 状態（**8 = 閉じ開始 / 2 = 完了**）。

**`FUN_00587550(menu_id)`（ID 指定クローズ、627B）**

- **フェード付きで閉じるのは 4 つの ID のみ**: `0x3ff` / `0x3fe` / `0x3ea` / `0x3eb`。
- フェード無しで閉じる ID: `0x3ec` / `0x3ed` / `0x3f2` / `0x3ef` / `0x414` /
  `0x3f0` / `0x402` / `0x418`。

**閉じるときの逆再生（reverse）は存在しない。**
`FUN_00588a70`（24B ノード生成）は「**現在値から目標値へ向かう新しいフェードを
登録する**」ものであり、時間を逆に走らせる機構ではない。閉じ時は
「`current = 0` から開始し、`duration` 秒で 1.0 に到達する」ノードが
新規に追加される（`FUN_00584300` が `+0x4 = 0` / `+0x10 = 0` で初期化）。

### 37.7 現在ルートメニュー取得とオフセット集計

**`FUN_005898f0`（55B、呼び出し元 348 箇所）**

`+0x10`（= next / 親方向）チェーンを末端まで辿り、vtable slot[3]（`+0xc`）が
**`0x389`** を返すノードの **`+0x44`** を返す。

```
0058991b  CMP EAX,0x389
00589922  MOV EAX,[ESI+0x44]
```

= 汎用「**現在のルートメニュー取得**」。`0x389` は「ルート」を表す特殊 ID。

**`FUN_00584980`（メニューオフセット集計）**

- 除外 ID: **`0x400` / `0x3ed` / `0x3f2`**。
- 戻り値 = 「最大オフセット + `_DAT_00a3d0c0`（= double 2.0）」。
- 呼び出し元は 2 箇所のみ。

### 37.8 メニュー表示状態のフィールドと参照箇所

**MenuManager シングルトン `DAT_00b3a6e0`**（参照 15 箇所、Python 実測）:

| VA | 命令 |
|---|---|
| `00581b79` | `MOV [0x00b3a6e0],EBX`（生成） |
| `00582186` | 読出（getter） |
| `005821c8` | 読出 |
| `005821d2` | 読出 |
| `00583bbb` | 読出（フェード更新族） |
| `00583cc5` | 読出 |
| `00583cf1` | 読出（FUN_00583cf0 冒頭） |
| `00583d2d` | 読出 |
| `00583d52` | 読出 |
| `00583df1` | 読出 |
| `00583e32` | 読出 |
| `00583e4d` | **`MOV dword [0x00b3a6e0],0`（破棄）** |
| `00584355` | 読出（FUN_00584300 族） |
| `00584367` | 読出 |
| `00584378` | 読出 |

**フィールド配置**

| オフセット | 内容 |
|---|---|
| `+0x98` | **直前メニュー** |
| `+0x9c` | **現在メニュー**（`null` ならメニュー非表示＝ゲーム入力を受け付ける） |
| `+0x130` | フェード／メニューリスト構造体ポインタ（`+0x0c` = tail / `+0x10` = head） |

**メニュー表示中フラグ `DAT_00b3a6d4`（byte）**（参照 7 箇所）:

| VA | 命令 |
|---|---|
| `00572ee2` | `CMP byte [0x00b3a6d4],0` / `JNZ` |
| `0057cccb` | 1 をセット |
| `0057cd87` | 0 をセット |
| `00581b18` | 1 をセット |
| `00581b3d` | `MOV byte [0x00b3a6d4],...` |
| `005a586b` | `CMP byte [0x00b3a6d4],0` / `JNZ` |
| `00679a75` | `CMP byte [0x00b3a6d4],0` / `JNZ` |

`0x00b3a6e0 + 0x130`（= `0x00b3a810`）への**直接即値参照は 0 件**。必ず
`DAT_00b3a6e0` 経由で参照される（ポインタ間接）。

### 37.9 フェード関連の定数（実値一覧）

| VA | セクション | 実値 | 用途 |
|---|---|---|---|
| `0x00a37080` | .rdata | **0.001f** | フェードノードの既定 duration |
| `0x00a41304` | .rdata | **0.25f** | 対象なし時の閉じフェード duration |
| `0x00a379b4` | .rdata | **2.0f** | trait `0xfa1` / `0x1772` の通過値 |
| `0x00a2fc68` | .rdata | **double 0.0** | フェード完了しきい値（`FCOMP` の後始末） |
| `0x00a69764` | .rdata | **float 6007.0** | trait `0xfa5` ゲート |
| `0x00a69768` | .rdata | **double 103.0** | trait `0xfa5` ゲート |
| `0x00a69770` | .rdata | **float 6006.0** | trait `0xfa5` ゲート |
| `0x00a69778` | .rdata | **double 102.0** | trait `0xfa5` ゲート |
| `0x00a3f3e8` | .rdata | **double 10.0** | `FUN_00584980` オフセット |
| `0x00a3d0c0` | .rdata | **double 2.0** | `FUN_00584980` オフセット加算 |
| `0x00a31c80` | .rdata | **10.0f** | `FUN_00584980` オフセット |
| `0x00a309f0` | .rdata | **double 100.0** | `FUN_00584980` オフセット |
| `0x00a68fd0` | .rdata | **double -0.008** | `FUN_00584980` オフセット |
| `0x00a57604` | .rdata | **float 0.02** | フェード係数分岐しきい値 |
| `0x00a68fd0` | .rdata | double -0.008 | 同上 |
| `0x00a68610` | .rdata | **double 0.05** | フェード係数分岐 |
| `0x00a2faa0` | .rdata | **float 0.5** | フェード係数（放物線側） |
| `0x00a3d65c` | .rdata | **float 0.5** | フェード係数分岐 |
| `0x00b33e9c` | .data **BSS** | **float グローバル（初期 0）** | フェード `current` の歩進量 |

#### `_DAT_00b33e9c` の性質（Python バイト走査で確定）

- `0x00b33e9c` は **BSS**（`.data` の初期化済み領域は RAW `0x30c00` まで、
  VA に直すと `0x00b32c00`。`0x00b33e9c > 0x00b32c00`）。
- 即値 `9C 3E B3 00` を `.text` 全域（VA `0x401000` 〜 `0x9dc2be`）で検索した結果、
  **164 ヒット / 108 ラン**。前 2 バイト（ModRM）による命令分類:

| バイト | 命令 | 件数 |
|---|---|---|
| `D9 05` | `FLD dword [0x00b33e9c]` | 74 |
| `D8 25` | `FSUB dword [0x00b33e9c]` | 52 |
| `D8 05` | `FADD dword [0x00b33e9c]` | 21 |
| `D8 0D` | `FMUL dword [0x00b33e9c]` | 14 |
| `D8 35` | `FDIV dword [0x00b33e9c]` | 2 |
| `D9 1D` | `FSTP dword [0x00b33e9c]` | **1（唯一の書き込み）** |

- 参照は `0x4052a2` 〜 `0x9dc2be` に広く分散。最密クラスタ:
  `0x65f1a6〜0x65f5d8`(5) / `0x40db8d〜0x40deb1`(5) / `0x63a2ef〜0x63a528`(4) /
  `0x64d81e〜0x64dae4`(4) / `0x67174e〜0x67193f`(4) /
  `0x65ed83〜0x65edf6`(3) / `0x65ff43〜0x6601ec`(3) / `0x664d16〜0x664d5e`(3) /
  `0x5a85b7〜0x5a862b`(3) / `0x5a948d〜0x5a9570`(3)。
- `FUN_00583c30` 内の参照は `0x583c44`（`FADD`）**1 箇所のみ**。
- **書き込みが 1 箇所だけ**という事実は、これが「毎フレーム 1 回だけ注入される
  共有スカラー」であることを示す。用途の広さ（164 参照）から、
  **フレーム経過時間／時間スケール相当の共有グローバル**である可能性が高い。
  フェードはこの値で**線形に**進む。
- **注意**: 「フェード専用の歩進量」と断定はできない。§37 では
  「BSS の float グローバル。外部から毎フレーム注入され、フェードはこの値だけ
  進む」と扱う。移植側は `deltaTime` をそのまま使ってよい。
- `va2raw(0x583c30) = 0x183030` を再検算済み（VA 写像の検証）。

### 37.10 関連関数の詳細（A4 補足）

**`FUN_00588a70`（76B）— 24B ノード生成＋対象 `+0x28` リストへ登録**

```c
void FUN_00588a70(int target, int key, float a, float b, float dur) {
  node24* n = FUN_00401f00(0x18);     // 24 バイト
  n[0] = target;   n[1] = key;        // int キー
  n[2] = a;        n[3] = b;          // float
  n[4] = dur;                         // duration
  n[5] = *(int*)(target + 0x28);      // 旧 head
  *(int*)(target + 0x28) = n;         // 先頭挿入
  *(int*)(target + 0x2c) |= 0x80;     // フラグ立て
  FUN_00584300(target, dur);          // フェードノードも登録
}
```

**`FUN_00585190(this, flag)`（115B）**

trait `0xfa1` に `1.0f` を書き込み → `flag == 0` なら
`FUN_00584390()`（閉じフェード）→ `FUN_0057f9f0(0, 0xfdd, 0)` →
`FUN_00584980()`。

**`FUN_00591ca1(this)`（144B）**

`FUN_00584880` → `getID` → `+0x20` 設定 → trait `0xfa5` が
`102.0` / `103.0` のとき `+0x14 = getID` → `FUN_00584980()` → trait `0xfab` →
`FUN_0042bb90`（**ECX = `0x00b13970`** = 登録コンテナ、キー = `ID - 0x3e9`）。

**`FUN_00589980(this, key, type, value, extra)`（277B）**

汎用 trait／リスト値セッター。フェード本体からは直接呼ばれない。

### 37.11 §33.3 の訂正（`FUN_00588a70`）

§33.3 に「メニューを閉じるときの逆再生」と書いたが、**誤り**。
`FUN_00588a70` は「24 バイトのノードを生成して対象 `+0x28` のリスト先頭へ入れ、
`+0x2c |= 0x80` を立てたうえでフェードノードを登録する」関数であり、
**時間を逆走させる機構ではない**。

正しい閉じ挙動:

- 閉じ時は `FUN_00584390` → `FUN_00584300(target, duration)` で
  **`current = 0` の新しいノード**を追加する。
- したがって「閉じ」は「**現在値から目標値へ向かう新しい（順方向の）フェード**」。
- 逆再生（`duration` を負にする／`current` を減算する）コードは存在しない。

### 37.12 Android 移植向け確定値（§37）

1. **フェードは「対象オブジェクトごとの `current` / `duration` ペア」**。
   ノード 20B、`+0x4 = current`（0 から開始）、`+0x8 = duration`（秒）。
2. **毎フレーム `current += deltaTime`**、`ratio = current / duration`。
   **`ratio >= 1.0` で完了し、そのノードは破棄**。**イージング無し（リニア）**。
3. **閉じるときも同じ機構**。`current` を 0 に戻して新規ノードを追加する。
   **逆再生はしない**。
4. **duration は XML の `menufade` / `explorefade` 属性そのまま**。
   フォールバックは **0.25 秒**。属性 `0.0` の `loading` はフェードしない。
5. **ポーズするかどうかは `menufade`（ポーズ）/ `explorefade`（非ポーズ）で
   決まる**（XML 属性が正本）。
6. **全画面黒クアッドはエンジン側に無い**。暗転の描画は消費者側で
   `alpha = current / duration` を使う。移植側もフェード値だけ持って
   描画は HUD／メニュー側で行えばよい。
7. **「メニューが開いているか」の正本は `MenuManager + 0x9c`（現在メニュー）**。
   非 `null` ならメニューが開いている。`+0x98` は直前メニュー。
   補助フラグとして `DAT_00b3a6d4`（byte）がある。
8. **メニュー入力抑制**は `MenuManager + 0x9c`（および `DAT_00b3a6d4`）を
   見て、`ACTION_MOVE` をゲーム側へ渡す前に落とす実装で一致する
   （判定箇所は `0x00572ee2` / `0x005a586b` / `0x00679a75` の 3 箇所で
   `DAT_00b3a6d4` を `byte` 比較）。
9. **フェード付きで閉じるメニュー ID は 4 つのみ**:
   `0x3ff` / `0x3fe` / `0x3ea` / `0x3eb`。他は即時クローズ。
10. **trait `0xfa5` の 4 値ゲート（6006 / 6007 / 102.0 / 103.0）は
    エンジン内部のメニュー登録都合**。移植側で再現不要。

### 37.13 未確定事項（§37）

- `_DAT_00b33e9c` に書き込む唯一の `FSTP` 命令の VA（＝歩進量の供給元）。
  164 参照という広域性から共有グローバル（フレーム時間等）と推定しているが、
  供給元関数の特定は未了。
- `0x389`（ルートメニュー ID）の実体（どのメニュー種別か）。
- `FUN_00584980` のオフセット定数（`10.0` / `100.0` / `-0.008` / `2.0`）の使われ方。
- `0x00a6976c` = `3.40234375f` と `0x00a6977c` = `3.3984375f` の用途。
- `FUN_00582160(0, 1)` の 2 引数の意味。

## §38 メニュー ctor / dtor 実 VA 一括同定（A5）

### 38.1 手法

`D:\Cargo\Ghidra\scripts\VtHits.java`（自作）を作成し、`.text` 全域の命令を走査して
各メニュー vtable アドレスの即値（`MOV [reg], imm32`）参照箇所を検出した。各ヒットについて
所属関数エントリ・関数サイズ・基底 Menu ctor（`0x00584640`）呼出回数・基底 Menu dtor
（`0x00585330`）呼出回数を出力し、以下で自動判定した。

```
ctor84640 >= 1 かつ dtor85330 == 0  ->  その vtable の ctor
dtor85330 >= 1                      ->  その vtable の dtor（削除デストラクタ）
両方 0                              ->  基底 Menu を継承しないクラス、または補助 vtable
```

出力は `tmp\ghidra_vthits_clean.txt`（132 行、`HIT` 行 106 件）。EXIT=0、コンパイルエラーなし。
`HIT` は `vt=VT at=命令VA mn=ニーモニック fn=所属関数エントリ size=関数サイズ` 形式。

### 38.2 全メニュー vtable -> ctor / dtor 対応表

| メニュー / クラス | vtable | ctor VA | dtor VA |
|---|---|---|---|
| Menu | `0xa696dc` | `0x(基底) 00584640` | `0x(基底) 00585330` |
| (VideoRes NiTListBase 補助) | `0xa69fe0` | `0x-` | `0x-` |
| ControlsMenu | `0xa69ff4` | `0x005874a0` | `0x00587520` |
| (VideoRes 補助) | `0xa6a034` | `0x-` | `0x-` |
| TextEditMenu | `0xa6a04c` | `0x005879c0` | `0x00587a50` |
| (VideoRes NiTList 補助) | `0xa6a08c` | `0x-` | `0x-` |
| VideoMenu | `0xa6a09c` | `0x00587bc0` | `0x00587c60` |
| TileMenu | `0xa6af14` | `0x00590130` | `0x00591b70` |
| AlchemyMenu | `0xa6b1b4` | `0x00593050` | `0x005931c0` |
| AudioMenu | `0xa6b29c` | `0x00595150` | `0x00595350` |
| BookMenu | `0xa6b334` | `0x00595be0` | `0x00595c40` |
| BreathMenu | `0xa6b404` | `0x00596420` | `0x00596520` |
| ClassMenu | `0xa6b49c` | `0x00596ef0` | `0x00596c70` |
| ContainerMenu | `0xa6b574` | `0x00597a90` | `0x005978d0` |
| CreditsMenu | `0xa6b784` | `0x0059c960` | `0x0059c890` |
| DialogMenu | `0xa6b88c` | `0x0059e4e0` | `0x0059e580` |
| EffectSettingMenu | `0xa6b92c` | `0x0059f9b0` | `0x0059fc30` |
| EnchantmentMenu | `0xa6ba0c` | `0x005a1390` | `0x005a1680` |
| GameplayMenu | `0xa6bd0c` | `0x005a3310` | `0x005a3430` |
| GenericMenu | `0xa6bd9c` | `0x005a3be0` | `0x005a3c60` |
| HUDInfoMenu | `0xa6bdf4` | `0x005a4660` | `0x005a4810` |
| HUDMainMenu | `0xa6bee4` | `0x005a6780` | `0x005a6850` |
| HUDSubtitleMenu | `0xa6c0dc` | `0x005a8df0` | `0x005a9320` |
| InventoryMenu | `0xa6c18c` | `0x005a9b00` | `0x005a9bb0` |
| LevelUpMenu | `0xa6c264` | `0x005ac550` | `0x005ac5b0` |
| LoadingMenu | `0xa6c40c` | `0x005ad0d0` | `0x005adbc0` |
| LoadgameMenu | `0xa6c6bc` | `0x005ae450` | `0x005ae4f0` |
| LockPickMenu | `0xa6c73c` | `0x005af310` | `0x005af410` |
| MagicMenu | `0xa6c974` | `0x005b1f20` | `0x005b1f70` |
| MagicPopupMenu | `0xa6ca54` | `0x005b3d30` | `0x005b3ed0` |
| MainMenu | `0xa6cb1c` | `0x005b5f20` | `0x005b5870` |
| MapMenu | `0xa6cc9c` | `0x005b7360` | `0x005b7490` |
| MessageMenu | `0xa6ce5c` | `0x005bc450` | `0x005bc4a0` |
| NegotiateMenu | `0xa6cf0c` | `0x005bce40` | `0x005bd050` |
| OptionsMenu | `0xa6cfac` | `0x005bd580` | `0x005bd650` |
| PauseMenu | `0xa6d03c` | `0x005bd960` | `0x005bda60` |
| PersuasionMenu | `0xa6d0cc` | `0x005bdff0` | `0x005be240` |
| QuantityMenu | `0xa6d15c` | `0x005c04f0` | `0x005c05a0` |
| QuickKeysMenu | `0xa6d1f4` | `0x005c0b50` | `0x005c0cf0` |
| RaceSexMenu | `0xa6d38c` | `0x005c4e40` | `0x005c4bf0` |
| RechargeMenu | `0xa6d45c` | `0x005ce680` | `0x005ce6c0` |
| RepairMenuList | `0xa6d5a4` | `0x005d1000` | `0x005d0dd0` |
| RepairMenu | `0xa6d5b4` | `0x005d1000` | `0x005d0dd0` |
| SaveMenu | `0xa6d624` | `0x005d3290` | `0x005d3320` |
| SigilStoneMenu | `0xa6d664` | `0x005d3eb0` | `0x005d4140` |
| SkillsMenu | `0xa6d71c` | `0x005d5610` | `0x005d56f0` |
| SleepWaitMenu | `0xa6d7c4` | `0x005d6870` | `0x005d6960` |
| SpellMakingMenu | `0xa6d864` | `0x005d73c0` | `0x005d7610` |
| SpellPurchaseMenu | `0xa6d8cc` | `0x005d8900` | `0x005d8950` |
| StatsMenu | `0xa6d9dc` | `0x005d9ad0` | `0x005d9d70` |
| TrainingMenu | `0xa6dc74` | `0x005dd200` | `0x005dd310` |
| VideoDisplayMenu | `0xa6dccc` | `0x005dd960` | `0x005dda20` |
| MenuBGShader | `0xa8cfd4` | `0x007b1200` | `0x007b1c70` |

計 53 vtable。うち 48 件がメニュー本体、4 件が VideoRes 補助 vtable（`0xa69fe0` /
`0xa6a034` / `0xa6a08c` および `0xa69ff4` と同居する `0xa6a034`）、1 件が `MenuBGShader`。

### 38.3 判定規則の検証（既知 22 関数との一致）

手作業で同定済みだった 22 関数（AudioMenu / GameplayMenu / MagicMenu / SkillsMenu /
SleepWaitMenu / ClassMenu / ContainerMenu / RechargeMenu / VideoDisplayMenu /
SpellMakingMenu / LevelUpMenu / QuickKeysMenu）の ctor / dtor VA は、すべて上表と一致した。
したがって「ctor84640 呼出の有無」と「dtor85330 呼出の有無」による自動判定は妥当。

### 38.4 特殊ケース

- **基底 `Menu`（`0xa696dc`）**: ヒット先は `0x00584640`（ctor 自身、size=40）と
  `0x00585330`（dtor 自身、size=177）。自己参照のため ctor/dtor カウントは 0。
  基底 Menu ctor = `0x00584640` / 基底 Menu dtor（削除デストラクタ）= `0x00585330`。
- **`TileMenu`（`0xa6af14`）**: ctor44640 = 0。基底 Menu ctor を呼ばず、`0x00590130`
  （size=84）と `0x00591b70`（size=169）が vtable を設定する。前者が ctor、後者が dtor 相当と推定。
- **`MenuBGShader`（`0xa8cfd4`）**: `0x007b1200`（size=402）と `0x007b1c70`（size=358）が設定。
  Menu 系ではなく BG シェーダクラス。基底 Menu ctor / dtor は呼ばない。
- **`RepairMenuList`（`0xa6d5a4`）と `RepairMenu`（`0xa6d5b4`）**: 同じ ctor `0x005d1000` が
  両 vtable を設定する（リスト vtable は ctor 内で別途設定）。dtor は `0x005d0dd0` が両方を戻す。
- **VideoRes 補助 vtable**: `0xa69ff4`（ControlsMenu）の ctor `0x005874a0` の近傍で
  `0xa69fe0` / `0xa6a034` が設定される（`0x00587470` / `0x00587488` / `0x00587940`）。
  同様に VideoMenu ctor `0x00587bc0` が `0xa6a08c`（NiTList VideoRes）も設定する。
  `0x00587470` / `0x00587488` は Ghidra 未関数認識（`NOFN`）だが実在する小関数。

### 38.5 dtor の共通構造（全メニュー）

```asm
mov [esi], <vtable>          ; 自クラス vtable を再設定
call 0x00585330              ; 基底 Menu デストラクタ
test byte [esp+8], 1         ; 削除フラグ
call 0x00401f20              ; free（フラグ時のみ）
ret 4                        ; thiscall + 引数 1 個
```

一部（RechargeMenu / MagicMenu / SleepWaitMenu / ContainerMenu）は SEH フレーム付きで、
先に内部リストを解放してから基底 Menu dtor を呼ぶ。

### 38.6 Android 移植向け確定値

- メニュー実装クラスのコンストラクタ／デストラクタ対応が必要な場合は §38.2 の表を使う。
- 移植側のメニュー基底（`Menu` 相当）は `0x00584640`（ctor）／`0x00585330`（dtor）に対応。
  派生メニューの ctor は必ず基底 ctor を呼び、dtor は基底 dtor を呼ぶ。
- vtable スロット規約は §34.3 と §38.5 のとおり。slot 0 = 削除デストラクタ。
- `TileMenu` と `MenuBGShader` は基底 `Menu` を継承しないため、移植側でも共通 Menu 基底に
  載せないこと。


## §39 未文書化メニューのレイアウト（A7: Container / QuickKeys / LevelUp）

§28 で「未文書化」として列挙したメニューファミリーのうち、Phase 68（ゲームシステムと GUI）で
必要になる 3 種（ContainerMenu / QuickKeysMenu / LevelUpMenu）について、§24 / §25 / §26 と
同じ粒度（ウィジェット表）で XML レイアウトを確定する。
素材は `Oblivion - Textures - Compressed.bsa` 抽出物ではなく、`Menus` 系 BSA の
`tmp\bsa_misc_full\menus\` を使用した（`container_menu.xml` = 2529 行、
`quickkeys_menu.xml` = 329 行、`levelup_menu.xml` = 389 行）。
機械抽出は `tmp\cont_outline.txt`（148 要素のネスト構造）と `tmp\cont_props.txt`
（全要素プロパティ、1471 行）に保存した。

### 39.1 ContainerMenu（container_menu.xml）

ルート要素は `<menu name="ContainerMenu">`。vtable は `0x00a6b574`、ctor `0x00597a90`、
dtor `0x005978d0`（§38）。stackingtype / locus は XML 既定を使用する。

#### 39.1.1 menu 変数（初期値）

| trait | 初期値 | 意味 |
|---|---|---|
| user0 | 1 | 現在のタブ番号（1〜5） |
| user1〜user4 | -1 | タブ別ヘッダの選択状態 |
| user5 | 0 | 表示件数（リスト行数） |
| user6 | 0 | プレイヤー所持金 |
| user7 | true | コンテナ側パネル可視（false で所持品表示） |
| user8 | true | Take All 許可 |
| user9 | true | 値切り（haggle）許可 |
| user10 | 0 | 魔法ポップアップ X 座標 |
| user11 | "" | 相手 NPC 名 |
| user12 | 0 | 取引金額 |
| user13 | 0 | 現在の積載量 |
| user14 | 0 | 最大積載量 |
| user15〜user19 | 0 | 追加ヘッダの位置 |
| user22 | `screen().height - screen().cropy*2 - 240` | リスト高さ |
| user23 | `cont_background.depth` | 背景奥行き |
| user25 | false | 金移動アイコン可視 |

キーバインド: `xleft=cont_tabs_prev` / `xright=cont_tabs_next` / `xbuttonb=cont_button_exit` /
`xbuttonx=cont_button_take_all` / `xbuttony=cont_button_haggle`。

#### 39.1.2 主要ウィジェット（確定値）

属性の無い行は XML 既定値。`<expr>` は実行時式（コピー元 trait は備考に記載）。

| name | id | type | depth | x / y | w x h | visible / target | 備考 |
|---|---|---|---|---|---|---|---|
| cont_FocusBox | 14 | include | 1 | - | - | visible=false | `focus_box.xml` |
| cont_background | - | rect | - | - | - | - | 背景ルート |
| cont_background_2 | - | image | -15 | 720 / 0 | 128x854 | - | `Menus\Container\cont_box_background_2.dds` |
| cont_inv_background | - | image | 1 | 35 / 21 | 745x792 | `parent.alpha==0` | `Menus\Shared\shared_main_background_1.dds` |
| cont_inv_background_2 | - | image | - | 700 / (親) | 54x792 | - | 背景右端 |
| cont_page_5〜1 | - | rect | 5 | 18 / 539 | 791x176 | `user0==N` | `shared_0N_tab.dds`（5..1 タブの下地） |
| cont_tabs_prev | 7 | image | - | -1 / -1 | 1x1 | target=true | clicksound=5、タブ左移動 |
| cont_tabs_next | 8 | image | - | -1 / -1 | 1x1 | target=true | clicksound=5、タブ右移動 |
| cont_p5_tab〜cont_p1_tab | 5..1 | include | 6 | 553/417/279/141/1, y=610 | - | target=`user0!=N` | `page_tab.xml`、`inv_icon_tab_{misc,alchemy,apparel,weapons,all}.dds` |
| cont_border_top | - | image | 1 | 91 / 140 | 596x160 | - | `inv_border_top.dds` |
| cont_border_middle | - | image | - | 91 / 276 | 576x249 | - | `inv_border_middle.dds` |
| cont_border_bottom | - | image | - | 34 / 521 | 628x160 | - | `inv_border_bottom.dds` |
| cont_player_money_icon | - | image | 1 | 50 / 648 | 32x32 | - | `all_small_icons.dds` cropx=224 |
| cont_player_money_text | - | text | 1 | 42 / -5 | - | - | string=`user6`、justify=left、RGB=117,59,33 |
| cont_npc_money_icon | - | image | 1 | `<expr>` / 648 | 32x32 | `user9` | `all_small_icons.dds` cropx=224、sub=20 |
| cont_npc_money_text | - | text | 1 | 42 / -5 | - | `<expr>` | justify=left、RGB=117,59,33 |
| cont_npc_name | - | text | 1 | `<expr>` / -5 | wrap 300 | - | 1 行、justify=left、RGB=117,59,33 |
| cont_encumberance_icon | - | image | - | `<expr>` / 648 | 32x32 | locus | `all_small_icons.dds` cropx=32、sub=20 |
| cont_encumberance_text | - | text | - | 40 / -5 | - | - | string=`user13` |
| cont_encumberance_slash | - | text | - | `<expr>` / -5 | - | - | string=`/`、add=1 |
| cont_encumberance_max | - | text | - | `<expr>` / -5 | - | - | string=`user14`、add=1 |
| cont_button_take_all | 32 | include | 1 | `<expr>` / 810 | - | `<expr>` | `button_long.xml`、user0=`_takeall`、user20=`&xbuttonx;` |
| cont_button_haggle | 34 | include | 1 | `<expr>` / `<copy take_all.y>` | - | `<expr>` | `button_long.xml`、user0=`_haggle`、user20=`&xbuttony;` |
| cont_button_exit | 33 | include | 1 | `<expr>` / `<copy take_all.y>` | - | - | `button_short.xml`、user0=`_exit`、user20=`&xbuttonb;` |
| cont_scroll_bar | 11 | include | 150 | 675 / `<copy cont_window.y>` | h=`cont_window.height` | - | `vertical_scroll.xml`、user2=`<expr>`、sub=5 |
| cont_window | - | rect | 12 | 10 / 150 | 645x480 | locus | clipwindow=true |
| cont_pane | - | rect | - | 6 / `<expr>` | - | locus | mul=-81（リスト仮想スクロール位置） |
| cont_list_contents | 13 | rect | - | - | - | target=false | xlist=`&xlist;`、xup=`cont_p123_header_text.mouseover`、xscroll=`cont_scroll_bar.user5` |
| cont_item_template | - | template | - | - | - | - | 39.1.4 参照 |
| cont_select_frame | - | image | 10 | 272 / 729 | 287x86 | locus | `cont_select_frame.dds` |
| cont_select_frame_marker | - | image | 30 | `<expr>` / -3 | 128x95 | - | `cont_select_frame_marker.dds`、add=`<expr>` |
| cont_icon_inv_off | 42 | image | -2 | 53 / 4 | 86x78 | `user7` | `cont_icon_inv_off.dds` |
| cont_icon_inv_on | - | image | -2 | 53 / 4 | 86x78 | `cont_icon_inv_off.visible==false` | `cont_icon_inv_on.dds` |
| cont_icon_cont_off | 43 | image | -2 | 141 / 5 | 90x78 | `user7==false` | `cont_icon_cont_off.dds` |
| cont_icon_cont_on | - | image | -2 | 145 / 5 | 90x78 | `cont_icon_cont_off.visible==false` | `cont_icon_cont_on.dds` |
| cont_select_left | - | image | 6 | `<expr>` / 25 | 42x35 | `&xbox;` | `hud_xbox_lt.dds`、copy=0、add=56 |
| cont_select_right | - | image | 6 | `<expr>` / 25 | 42x35 | `&xbox;` | `hud_xbox_rt.dds`、sub=56 |

#### 39.1.3 タブヘッダ（2 系統 + 8 カテゴリヘッダ）

タブヘッダは 2 系統。`cont_headers_top`（rect）配下に
`cont_p123_header_top`（id=16、y=100、lte=3）と `cont_p4p5_header_top`（id=15、y=100、gte=4）を持つ。

**cont_p123_header（武器 / 防具タブ用、lte=3）**

| name | id | type | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|
| cont_p123_header_left | - | image | `<expr>` / `<expr>` | 35x17 | `shared_decor_blue_left.dds`、sub=55、div=2、add=6 |
| cont_p123_header_right | - | image | `<expr>` / `<expr>` | 35x17 | `shared_decor_blue_right.dds`、add=20、div=2 |
| cont_p123_header_text | 16 | text | 130 / 5 | - | target=true、clicksound=3、string=`<expr>`、font=3、RGB=51,79,142、justify=center、xright=`img1`、xdown=`cont_list_contents` |
| cont_p123_header_arrow | - | image | 23 / 0 | 32x32 | ソート方向矢印、`all_small_icons.dds` cropx=352、eq=true |
| cont_p123_header_img1 | 17 | image | 300 / 0 | 32x32 | locus、cropx=224、xleft=text、xright=img2、xdown=list_contents |
| cont_p123_header_img2 | 18 | image | 375 / 0 | 32x32 | cropx=32、xleft=img1、xright=img3a |
| cont_p123_header_img3a | 19 | image | 435 / 0 | 32x32 | eq=1、cropx=160、xleft=img2、xright=img3b |
| cont_p123_header_img3b | 19 | image | - | 32x32 | eq=2、cropx=128、xleft=img3a、xright=img3c |
| cont_p123_header_img3c | 19 | image | - | 32x32 | eq=3、cropx=96、xleft=img3b、xright=img4 |
| cont_p123_header_img4 | 20 | image | 495 / 0 | 32x32 | lte=2、cropx=256、xleft=img3c、xdown=list_contents |
| cont_p123_header_top | 16 | rect | 102 / 100 | - | lte=3、`_header_1..9` = `_weapons/_apparel/_apparatuses/_potions/_ingredients/_books/_keys/_stones/_misc`、`_headernum`=<expr>、mult=81 |

**cont_p4p5_header（錬金 / その他タブ用、gte=4）**

| name | id | type | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|
| cont_p4p5_header_left / right | - | image | `<expr>` / `<expr>` | 35x17 | `shared_decor_blue_left/right.dds` |
| cont_p4p5_header_text | 16 | text | 130 / 5 | - | target=true、string=`<expr>`、font=3、RGB=51,79,142 |
| cont_p4p5_header_arrow | - | image | 23 / 0 | 32x32 | cropx=352、eq=true |
| cont_p4p5_header_img1 | 17 | image | 300 / 0 | 32x32 | locus、cropx=224 |
| cont_p4p5_header_img2 | 18 | image | 375 / 0 | 32x32 | cropx=32 |
| cont_p4p5_header_img3 | 19 | image | 435 / 0 | 32x32 | cropx=96 |
| cont_p4p5_header_top | 15 | rect | 102 / 100 | - | gte=4、`_header_1..9` = `_alchemy/_apparatuses/_potions/_ingredients/_misc/_books/_keys/_stones/_misc`、mult=81 |

**カテゴリヘッダ 9 種（cont_*_header）** はすべて同一構造で、`visible`=式、`locus`、
`x=102`、`y=<expr>`、`mult=81`、`clips=true`。内部に `_left` / `_right`（image, depth=7,
35x17, `shared_decor_blue_left/right.dds`）、`_text`（text, x=130, y=35, font=3,
RGB=51,79,142, justify=center）、矢印 / アイコン群（`all_small_icons.dds`）を持つ。

| ヘッダ rect | 行 | テキスト trait | 矢印 cropx | 備考 |
|---|---|---|---|---|
| cont_weapon_header | 1158 | `_weapons` | 352 | 追加アイコン: cont_value_arrow/value_icon/weight_icon/damage_icon/health_icon |
| cont_apparel_header | 1289 | `_apparel` | 352 | - |
| cont_apparatus_header | 1399 | `_apparatuses` | 352 | img 3 枚 |
| cont_potions_header | 1522 | `_potions` | 352 | img 2 枚 |
| cont_ingr_header | 1646 | `_ingredients` | 352 | img 2 枚 |
| cont_book_header | 1746 | `_books` | 352 | - |
| cont_keys_header | 1866 | `_keys` | `Menus\Icons\icon_small_arrow.dds`(depth=10) | eq=5、and/or/onlyif 式あり |
| cont_stones_header | 1985 | `_stones` | - | - |
| cont_misc_header | 2108 | `_misc` | - | eq=5、and/or/onlyif 式あり |

カテゴリヘッダの `visible` ゲートは「現在タブ（user0）がそのカテゴリを含む場合のみ」。
`mult=81` はリスト 1 行の高さ（cont_item.height=81 と一致）で、行位置へスナップさせる。

また武器タブでは `cont_value_arrow` / `cont_value_icon`（cropx=224, x=300, y=30）/
`cont_weight_icon`（cropx=32, add=74）/ `cont_damage_icon`（cropx=160, add=64）/
`cont_health_icon`（cropx=256, add=64）が列ヘッダアイコンとなる。

#### 39.1.4 リスト行テンプレート（cont_item_template → cont_item）

`cont_item_template`（行 2423）は `cont_list_contents`（id=13）配下の `cont_item`
（rect, 行 2422）を 1 行として複製する。

| name | type | depth | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|
| cont_item | rect | 1 | 0 / `<expr>` | 650x81 | visible=`user8`、target=true、locus、alpha=0、listclip、gte、`repeatvertical=true`、focusinset=4、xlist=`&xitem;`、xup=`&prev;`、xdown=`&next;`、xscroll=`<expr>` |
| cont_equiped_marker | image | 2 | 7 / 7 | 635x64 | visible=`parent.user10`、alpha=160、`inv_equiped_marker_1.dds` |
| cont_item_icon | image | 3 | 5 / 7 | 64x64 | visible=`parent.user10 <sub> == false`、filename=`<expr>` |
| cont_item_over_icon | image | 3 | 5 / 7 | 64x64 | visible=`parent.user10`、filename=`<expr>`（重ね着アイコン） |
| cont_item_count | text | 5 | `<expr>` / `<expr>` | wrap 55 | justify=right、font=3、RGB=117,59,33、1 行、string=`<expr>`、sub=6 |
| cont_item_status | image | `<expr>` | `<expr>` / `<expr>` | `<expr>` | visible/`gt=0`、filename=`parent.user13`、copy=32、RGB 式、`_handonly`、eq=1 |
| cont_item_name | text | 3 | 80 / `<expr>` | wrap 280 | justify=left、RGB=117,59,33、2 行、string=`<expr>`、div=2 |
| cont_item_cost | text | 3 | 418 / `<expr>` | wrap 90 | justify=center、RGB=117,59,33、string=`<expr>`、div=2 |
| cont_item_weight | text | 3 | 496 / `<expr>` | wrap 64 | justify=center、RGB=117,59,33、div=2 |
| cont_item_damage | text | 3 | 556 / `<expr>` | wrap 70 | justify=center、RGB=117,59,33、div=2 |
| cont_item_health | text | 3 | 616 / `<expr>` | wrap 50 | justify=center、RGB=117,59,33、div=2 |

列 x 座標（コスト 418 / 重量 496 / ダメージ 556 / 耐久 616）は武器ヘッダの列アイコン
（value=300, weight=374, damage=438, health=502 + 幅 32）と対応する。行高 81 は
`mult=81` と一致。

#### 39.1.5 Android 移植向け確定値（ContainerMenu）

- ルート名 `ContainerMenu`、vtable `0x00a6b574`。
- リスト行の高さは **81 px**（`cont_item.height`＝全ヘッダの `mult`）。
- リスト領域は `cont_list_contents`（id=13）、スクロールは `cont_scroll_bar`（id=11、
  user2=expr、sub=5）。行カリングは `cont_window`（x=10,y=150,645x480, clipwindow=true）。
- タブは 5 枚（cont_p1_tab〜cont_p5_tab、id=1..5、y=610、x=1/141/279/417/553）、
  タブ切替は id=7（prev）/id=8（next）。
- ボタン: Take All=id 32、Exit=id 33、Haggle=id 34。
- 左右パネル切替アイコン: 所持品=id 42、コンテナ=id 43（user7 で排他）。
- 相手 NPC 表示系は user9/user11/user12、積載量は user13/user14。

### 39.2 QuickKeysMenu（quickkeys_menu.xml）

ルート要素は `<menu name="QuickKeysMenu">`。vtable は `0x00a6d1f4`、ctor `0x005c0b50`、
dtor `0x005c0cf0`（§38）。

#### 39.2.1 menu 属性

| 属性 | 値 |
|---|---|
| stackingtype | `does_not_stack` |
| alpha | 0 |
| locus | true |
| x / y | 0 / 0 |
| menufade | 0.25 |
| _global_alpha | 255 |

#### 39.2.2 主要ウィジェット（確定値）

| name | id | type | depth | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|---|
| quick_key_text | 6 | text | 11 | - | wrap 400 | font=2、string=`_quickkey`、justify=center、RGB=254,241,188、alpha=255 |
| Quick_key_selector | 7 | rect | - | 800 / 275 | `screen().width` x `screen().height` | target=true、locus、alpha=0、visible=true、user0..3 = 0/255/false/true |
| background_selector_image | - | image | - | -58 / -58 | 512x512 | visible=`parent.user2`、`Quick-Keys\background_paper.dds` |
| background_selector_image2 | - | nif | - | 200 / 200 | - | visible=`parent.user3`、`Quick-Keys\pc_background_paper_numbers.nif` |
| quick_key_1_image〜quick_key_8_image | 8..15 | image | 2 | `<expr>` / `<expr>` | 64x64 | filename は動的（空） |
| selector_image | - | nif | - | -235 / -90 | - | `Quick-Keys\quick keys.nif`、animation=`parent.user0 + me._animation_`、`_animation_1..8` = `0_`〜`7_` |

#### 39.2.3 8 スロットの座標（中心からの -w/2, -h/2 オフセット）

| スロット | x | y |
|---|---|---|
| 1 | 202 | 102 |
| 2 | 274 | 132 |
| 3 | 302 | 205 |
| 4 | 272 | 275 |
| 5 | 200 | 303 |
| 6 | 130 | 273 |
| 7 | 102 | 202 |
| 8 | 132 | 130 |

8 スロットは円形配置。`selector_image`（quick keys.nif）の animation 名は
`parent.user0 + me._animation_`（`_animation_N` = `"0_"`〜`"7_"`）で、選択中スロット
番号 user0（0〜7）に応じて対応アニメを選ぶ。

#### 39.2.4 Android 移植向け確定値（QuickKeysMenu）

- ルート名 `QuickKeysMenu`、vtable `0x00a6d1f4`、stackingtype=`does_not_stack`（他メニューと重ねない）。
- スロットは 8 個、アイコン id=8..15（64x64）、座標は 39.2.3 の表。
- `Quick_key_selector`（id=7）が放射状の当たり判定ラッパ。user0=選択インデックス、
  user1=alpha(255)、user2/user3=背景 2 種の可視フラグ。
- テキスト `_quickkey`（id=6）は font=2、中心揃え。
- 開閉フェードは `menufade=0.25`。

### 39.3 LevelUpMenu（levelup_menu.xml）

ルート要素は `<menu name="LevelUpMenu">`。vtable は `0x00a6c264`、ctor `0x005ac550`、
dtor `0x005ac5b0`（§38）。

#### 39.3.1 menu 属性・変数

| 属性 | 値 |
|---|---|
| class | `&LevelUpMenu;` |
| stackingtype | `no_click_past` |
| alpha | 0 |
| x / y | 0 / 0 |
| locus | true |
| menufade | 0.25 |

| trait | 初期値 | 意味 |
|---|---|---|
| user1 | false | 退出ボタン target |
| user2 | Descriptive text | 説明文 |
| user3 | "You have ascended to level x" | タイトル（レベル表記） |
| user4 | Filename | レベルアップ画像 |
| user5 | Attribute description | 選択属性の説明 |

`<xbuttonb><ref src="levelup_exit_button" trait="clicked"/></xbuttonb>`。

#### 39.3.2 主要ウィジェット（確定値）

| name | id | type | depth | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|---|
| levelup_background | - | include | 1 | centered | - | user0=1000、`generic_background.xml` |
| levelup_title | - | text | 4 | centered / 20 | wrap 900 | string=`user3`、justify=left、RGB=33,59,117 |
| levelup_description | - | text | 4 | - | wrap `parent.width-40` | string=`user2`、RGB=117,59,33 |
| levelup_instructions | - | text | 4 | - | wrap 900 | string=`_threeattributes`、RGB=33,59,117、font=3 |
| level_up_picture | - | image | 10 | 620 / - | 256x256 | filename=`user4` |
| levelup_attribute_description | - | text | 4 | - | wrap `level_up_picture.width + me._extend_text*2` | string=`user5`、RGB=117,59,33、font=3、`_extend_text`=60 |
| levelup_pane | 2 | rect | 5 | 30 / - | 540x536 | xlist=`&xlist;`、xright=`levelup_exit_button.mouseover` |
| levelup_exit_button | 1 | include | 6 | - | - | target=`user1`、`button_long.xml`、user20=`&xbuttonb;`、xdefault=`&xboxhint;`、xleft=`levelup_pane.mouseover` |

#### 39.3.3 リスト行テンプレート（levelup_template → levelup_item）

| name | id | type | x / y | w x h | visible / 備考 |
|---|---|---|---|---|---|
| levelup_item | 3 | rect | - | 450x67 | alpha=0、y=`me.listindex*67`、`_enabled_alpha`=`(user6 OR user0-1)*127+128`、xlist=`&xitem;`、xup=`&prev;`、xdown=`&next;` |
| levelup_item_coin | - | image | - | 32x32 | visible=`parent.user0`、`all_small_icons.dds` cropx=224（コイン/ボーナス印） |
| levelup_item_icon | - | image | 40 / - | 64x64 | filename=`parent.user5`、alpha=`parent._enabled_alpha` |
| levelup_item_text | - | text | 130 / - | wrap 300 | string=`parent.user4`、RGB=117,59,31 |
| levelup_item_value | - | text | 380 / - | - | string=`parent.user3`、RGB=33,59,117 |
| levelup_item_multiplier | - | text | 480 / - | - | string=`parent.user2`、justify=right、visible=`parent.user1` |
| levelup_item_selected | - | image | 115 / - | `parent.width-70` x 64 | visible=`parent.user0`、`inv_equiped_marker_1.dds` |
| levelup_item_mouseover_marker | - | image | 115 / - | `parent.width-90` | `dialog_selection_full.dds` |
| levelup_item_mouseover_marker_2 | - | image | `parent.width+55-me.width` / - | 104x64 | `dialog_selection_cut.dds` |

#### 39.3.4 Android 移植向け確定値（LevelUpMenu）

- ルート名 `LevelUpMenu`、vtable `0x00a6c264`、stackingtype=`no_click_past`
  （背面メニューへクリックを透過させない）。
- リスト行の高さは **67 px**（`levelup_item.height`）。
- 3 属性選択用アイテム（id=3）が選択対象。`_enabled_alpha = (user6 OR user0-1)*127+128`
  で「既に取得済み／取得可能」を 128..255 の不透明度に変換する。
- 退出は id=1（levelup_exit_button、`&xbuttonb;`）で `clicked` trait を呼ぶ。
- 開閉フェードは `menufade=0.25`。

### 39.4 未確認・残課題（A7）

- `container_menu.xml` の `cont_book_header` 以降（keys/stones/misc）の各 `_text` y 値、
  および `cont_item_status` の色式（red/green/blue の `<expr>` 実体）は式のコピー元が
  深く、静解析では確定できていない。移植側は RGB 既定（117,59,33 系）で代替可能。
- `cont_select_frame_marker` の `x=<expr>` / `cont_select_left/right` の `x=<expr>` は
  選択フレーム内の相対配置式で、確定には runtime 値が必要。
- QuickKeysMenu の `quick_key_N_image.filename`（動的）はアイテム割当て時に
  ゲーム側（`FUN_005c0b50` 系 ctor 以降）が設定する。XML には初期値が無い。
- Container / QuickKeys / LevelUp 以外の未文書化ファミリー（Recharge / Repair /
  TextEdit / generic / message 等）は §28 の vtable 一覧のみで、XML レイアウトは未解析。


## §40 ControlsMenu（0x3fd）ページ2 キー割当の確定値

§24.8 の ControlsMenu 記述を補強し、ページ2（CONTROL MAPPING）の全レイアウトと
キー割当リスト生成・デバイス切替・コマンドハンドラの switch 表を確定する。
対象 XML: `Data\Menus\Options\controls_menu.xml`（874 行 / 26,264 文字）。
解析対象: ControlsMenu vtable `0x00a69ff4`、ctor `0x005874a0`、dtor `0x00587520`、
生成・起動 `0x0059b9e0` / `0x0059ba40`、更新 `0x0059bd80`、コマンド `0x0059c5e0`、
補助 `0x0059b640` / `0x0059b670` / `0x0059b920` / `0x0059b980` / `0x0059c3f0` / `0x00587500`。

### 40.1 メニューレベル trait（menu 直下）

| trait | 値 | 意味 |
|---|---|---|
| `class` | `&ControlsMenu;` | メニュークラス（vtable 0x00a69ff4） |
| `stackingtype` | `&no_click_past;` | クリックは背後のメニューへ抜けない |
| `alpha` | `0` | メニュー全体は不可視（子が個別に alpha 指定） |
| `locus` | `&true;` | フォーカス可能 |
| `explorefade` | `0.25` | 非ポーズメニュー（探索中に開く）→ §37 の explorefade |
| `user0` | `1` | ページ番号（1=PAGE1、2=PAGE2） |
| `user1` | `controls_main_rect.height / controls_vertical_scroll.user8` | キー行1行の高さ（400/6 ≒ 66.7） |
| `user2` | `strings()._move` | 左スティック設定の表示文言（"Move"） |
| `user3` | `strings()._look` | 右スティック設定の表示文言（"Look"） |
| `user4` | `&true;` | true=normal / false=selected（ページ2の表示可否ゲート） |
| `user5` | コード設定 | 現在のデバイス名文字列（`FUN_0059b640` / `FUN_0059c3f0` が設定） |
| `red` / `green` / `blue` | `117` / `59` / `33` | 見出し文字色（茶） |
| `_button_buffer` | `50` | 画面端からのボタン余白 |

`controls_background`（30 行）は `generic_background.xml` を include し、画面中央寄せ。
`user0`=666（幅）、`user1`=580（高さ）。実効サイズ **666 x 580**。

### 40.2 PAGE 1（52〜383 行）確定値

`controls_page_1`（52 行）: width/height=`copy parent()`、x=0、y=0、locus=true、
`xbuttonb`=`ref controls_return_button_2 clicked`（ゲームパッド B で戻る）。

| 要素 | 行 | id | 型 | 主な確定値 |
|---|---|---|---|---|
| `controls_sensitivity_label` | 63 | `&generic;` | text | visible=`ControlsMenu.user0==1`、string=`strings()._mousesensitivity`、depth=7、RGB=ControlsMenu、x=スライダー中央、y=96 |
| `controls_sensitivity_slider` | 92 | **5** | image | include `horiz_floating_scroll.xml`、visible=label と同一、depth=5、y=label.y+label.height+12、user6=6（マーカー id）、user8=false（テキスト非表示） |
| `controls_sensitivity_slider_marker` | 119 | `&generic;` | image | target=true、depth=3、dialog_selection_full.dds、alpha=255*min(mouseover+slider._mouseover_any,1)、width=slider.width+120、height=64、x=slider.x-60、y=slider.y-18、locus=true |
| `controls_sensitivity_slider_marker_2` | 150 | - | image | depth=1、dialog_selection_cut.dds、width=104、height=64、x=parent.width-100 |
| `controls_invert_y_label` | 166 | `&generic;` | text | string=`strings()._inverty`、y=label.y+label.height+96 |
| `controls_invert_y_button` | 194 | **7** | rect | include `button_floating_2.xml`、depth=3、string=`strings()._onoff`、_center_x=label.x+label.width-32、_top=label.y-12 |
| `controls_swap_button` | 215 | **8** | rect | include `button_floating_2.xml`、depth=3、string=`strings()._swapstickcontrols`、_center_x=parent.width/2、_top=left_stick_label.y-me.height |
| `controls_left_stick_label` | 244 | - | text | string=`strings()._leftstick`、locus=true、y=invert_y_label.y+invert_y_label.height+96 |
| `controls_left_stick_setting` | 271 | - | text | string=`ControlsMenu.user2`、x=invert_y_button._center_x-me.width/2 |
| `controls_right_stick_label` | 302 | - | text | string=`strings()._rightstick`、locus=true、y=left_stick_label.y+left_stick_label.height |
| `controls_right_stick_setting` | 328 | - | text | string=`ControlsMenu.user3` |
| `controls_return_button_2` | 361 | **1** | image | include `button_long.xml`、depth=3、visible=`user0==1`、x=parent.width-me.width-_button_buffer、y=parent.height-me.height-25、clicksound=2、user0=`strings()._return` |

**ページ1の id 集合**: 1（Return2）、5（感度スライダー）、7（Invert Y）、8（Swap Sticks）。
`controls_swap_button`(8) はゲームパッド前提（`_swapstickcontrols`）で、PC でも表示される。

### 40.3 PAGE 2（392〜756 行）確定値

`controls_page_2`（392 行）: width/height=`copy parent()`、x=0、y=0、locus=true。

#### 40.3.1 DEVICE セクション（399〜513 行）

| 要素 | 行 | id | 主な確定値 |
|---|---|---|---|
| `controls_device_activate_highlight` | 400 | `&generic;` | target=true（マウスオーバー用の不可視ヒット領域） |
| `controls_device_toggle_floating` | 409 | - | visible=`left.mouseover>0 OR right.mouseover>0 OR activate_highlight.mouseover>0`、dialog_selection_full.dds、height=64、depth=3、x=`left.x - me()._buffer`、y=10、locus=true、`_buffer`=10。子 `effset_attribute_floating_end`（440 行、dialog_selection_cut.dds） |
| `controls_device_toggle_left` | 452 | **12** | **shared_arrow_left.dds**、124x36（前のデバイスへ） |
| `controls_device_text` | 474 | - | depth=3、visible=`ControlsMenu.user0==2`、justify=center、RGB=117,59,33、alpha=255、string=`ControlsMenu.user5`、y=20（現在のデバイス名） |
| `controls_device_toggle_right` | 497 | **13** | **shared_arrow_right.dds**、121x36（次のデバイスへ） |

デバイス名は `ControlsMenu.user5`（コード設定）。`user0==2`（ページ2）でのみ可視。

#### 40.3.2 MAIN RECT セクション（518〜657 行）

`controls_main_rect`（518 行）: target=true、depth=3、visible=`ControlsMenu.user0==2`、
x=(parent.width-me.width)/2、y=75、**width=538 / height=400**、alpha=0、clipwindow=true。

枠 5 枚（すべて `effset_attribute_frame_*.dds` 系）:

| 要素 | 行 | 確定値 |
|---|---|---|
| `controls_border_top` | 538 | x=0、y=0、width=parent.width |
| `controls_border_left` | 553 | x=0、y=32 |
| `controls_border_middle` | 567 | x=300、y=32 |
| `controls_border_right` | 578 | x=parent.width-32、y=32 |
| `controls_border_bottom` | 589 | x=0、y=parent.height-32、width=parent.width |

`controls_vertical_scroll`（602 行、include `vert_floating_scroll.xml`、**id=2**）:
x=parent.width。trait は user1=0（min）、user2=`list_pane.childcount - me().user8`（max）、
user3=1（step）、user4=5、user5=0（初期値）、user6=3、**user8=6（可視行数）**。
user7=現在値（624 行コメント）。

`controls_list_pane`（630 行、**id=4**）: alpha=0、locus=true、clips=true、width=parent.width、
height=`me().childcount * ControlsMenu.user1`、y=`0 - vertical_scroll.user7 * ControlsMenu.user1`、
`_left_edge_buffer`=10。651〜659 行コメントのとおり **子要素はコードがテンプレート
`controls_template` から生成**する（リスト行を XML に静的に持たない）。

#### 40.3.3 ボタンセクション（662〜717 行）

| 要素 | 行 | id | 主な確定値 |
|---|---|---|---|
| `controls_defaults_button` | 662 | **10** | include `button_long.xml`、visible=`user0==2 AND user4`、x=`ControlsMenu._button_buffer`、y=`return.y`、user0=`strings()._defaults`（既定に戻す） |
| `controls_more_button` | 678 | **9** | visible=`user0==1`、x=`defaults.x`、y=`return.y`、user0=`strings()._controls`（ページ1へ） |
| `controls_return_button` | 695 | **1** | visible=`user0==2 AND user4`、x=parent.width-me.width-_button_buffer、y=parent.height-me.height-25、clicksound=2、user0=`strings()._return` |

**id 1 は PAGE1 の Return2 と PAGE2 の Return で共有**（visible の `user0` 条件で排他）。
`user4` は「selected モード」＝キー再割当前の確認待ち状態。`user4==false` の間は
Defaults / Return が隠れ、選択中ボタンのみ操作可能。

#### 40.3.4 INSTRUCTIONS（719〜759 行）

`controls_instructions`（719 行、**id=11**）: visible=`ControlsMenu.user0==2 AND NOT ControlsMenu.user4`、
depth=3、justify=center、x=`parent().width/2`、y=`controls_defaults_button.y + (defaults.height - me().height)/2`、
alpha=255、RGB=ControlsMenu、wrapwidth=`parent().width - 32`。
**`string` プロパティを持たない**（本体文字列はコード（`FUN_0059c3f0` の trait 0xfde 等）
または親からの継承で供給される）。移植側はコード設定文言を別途用意する必要がある。

### 40.4 controls_template（762〜868 行）キー行テンプレート

`controls_template`（762 行）/ `controls_template_item`（763 行）:

| trait | 値 | 意味 |
|---|---|---|
| `x` | 0 | |
| `y` | `me().listindex * me().height` | 行の縦位置（listindex = 行番号） |
| `depth` | 1 | |
| `width` | `parent().width` | リストペイン幅 |
| `locus` | `true` | フォーカス可能 |
| `clips` | `true` | 行内クリップ |
| `target` | `true` | ヒット対象 |
| `repeatvertical` | `true` | **縦方向の繰り返しテンプレート**（行ごとに複製） |
| `user0` | `me().listindex` | 行 index |
| `user1` | `label string` | アクション名（"Move Forward" 等） |
| `user2` | `parent().width - 128` | ボタン（キー表示）の中央寄せ X |
| `user3` | `"key"` | PC 用キー名文字列 |
| `_left_edge_buffer` | `parent()._left_edge_buffer` | 左端余白の継承 |

子要素:

| 要素 | 行 | 確定値 |
|---|---|---|
| `controls_template_floating` | 795 | visible=`mouseover>0`（793 行コメント「コマンドが処理できない場合 xlist で親へ委譲」）、dialog_selection_full.dds、width=`parent().width-32`、height=64 |
| `effset_attribute_floating_end` | 810 | dialog_selection_cut.dds、width=104、height=64、x=`parent().width+8-me().width` |
| `controls_template_label` | 824 | depth=3、alpha=255、string=`parent().user1`、x=`parent()._left_edge_buffer`、y=12 |
| `controls_template_button` | 846 | depth=3、x=`parent().user2`、y=12、string=`parent().user3`、justify=center |

`xlist=true`（763 行の item 側）が「未処理コマンドを親へ委譲」する挙動の正体。
`controls_list_pane`（655 行コメント）の子はこのテンプレートからコード生成される。

### 40.5 キー割当リスト生成（FUN_0059ba40, 832B）

`FUN_0059ba40` は ControlsMenu の生成・起動関数。手順:

1. `FUN_00589b70(0x3fd)` で既存インスタンス取得 → あれば dtor。
2. `FUN_00582160(0, 1)` → **id 0x3fd を MenuManager に登録**。
3. `FUN_00590420("Data\Menus\Options\controls_menu.xml")` で XML パス指定。
4. `FUN_009832e6(...)` x2 → XML ロード。
5. **ウィジェット存在チェックループ**: `iVar5+0x28` から **13 個**のポインタを走査し、
   0 があれば `"Controls Menu Creation Failed..."` を出力して return 0。
   → offset 0x28〜0x5b が **13 個のウィジェットポインタ配列**（§40.8 参照）。
6. `FUN_00588bd0(0xfa5)` と `_DAT_00a69770/78` を比較 → `0xfab`（モード初期化）。
7. `0xfb3`（スクロール初期位置）を計算し 0 にリセット。
8. `FUN_0059b640` でデバイス名表示を初期化。
9. trait `0xfc9` = 1.0（条件付き）。
10. **キー割当リスト生成ループ（本関数の中核）**:
    ```
    for (iVar7 = 0; iVar7 <= 0x1d; iVar7++) {      /* 0..29（29 行） */
        if (FUN_0059b670(iVar7) == 0) continue;     /* 有効なキー index のみ */
        tmpl = テンプレート取得;
        if (tmpl == 0) continue;
        FUN_0058ceb0(0xfa8, 0x0e + iVar7);          /* 行 index = 14 + iVar7 */
        FUN_0058ceb0(0xfae, 行 index);              /* リスト行 index */
        FUN_0058ced0(0xfaf, (&DAT_00b399d0)[iVar7]); /* アクション名文字列 */
        trait 0xfca..0xfce をコピー;                 /* 色等の継承 */
    }
    ```
    末尾で `*(float*)(iVar5+0xdc) = FUN_00588bd0(0xfb1)`、
    `*(float*)(iVar5+0xe0) = FUN_00588bd0(0xfb2)`、`*(byte*)(iVar5+0xd4) = 1`。

**`FUN_0059b670(int idx)`（112B）**: `idx` が `{0..0x10}` または `{0x1a,0x1b,0x1c}` の
いずれかなら 1、それ以外 0。**キー index 0x1d（29）中、有効なものだけをリストへ載せる
フィルタ**（0x11〜0x19 と 0x1d を除外）。アクション名は `DAT_00b399d0[idx]`
（29 要素の文字列ポインタ配列、静的ファイル上は未初期化＝実行時書込）。

### 40.6 デバイス切替ロジック

#### FUN_0059b640(int param_1, byte param_2)（45B, __cdecl, RET 0x8）

```
if (param_1 != 0)
    FUN_0058ced0(0xfde, param_2 != 0 ? DAT_00b38da0 : DAT_00b38da8);
```
`param_1`=対象ウィジェット、`param_2`=0 なら `DAT_00b38da8`、非0 なら `DAT_00b38da0` の
文字列を trait `0xfde`（デバイス名表示）へ設定。

#### FUN_0059b920(int this)（88B, __fastcall, ECX=this）— デバイス順送り

`this[0x5c]`（offset 0x5c = 選択デバイス index）を +1 し、2 超なら 0 に戻す。
最大 3 回試行し、各 index 0/1/2 について `DAT_00b33398+0x20` の
+0x28（キーボード有無）/ +0x2c（マウス有無）/ +0x18f0（ゲームパッド有無）を検査して
**存在するデバイス index を採用**。成功時 true。

#### FUN_0059b980(int this)（93B, __fastcall）— デバイス逆送り

`FUN_0059b920` の逆。`this[0x5c]` を -1 し、0 なら 2 に戻す。成功時 true。

#### FUN_0059c3f0(int param_1, int param_2)（482B, __thiscall, RET 0x4）

デバイス確定・リスト再構築・INSTRUCTIONS 文字列更新の関数。挙動:

1. `*(param_1+0x34)+0x34` のリンクリストを走査し、`param_2` 以外のウィジェットに対し
   trait `0xfc9`=1.0、`0xfcc`/`0xfcd`/`0xfce`=`_DAT_00a2ffe8`（**=128.0f**）を設定。
2. `param_1[0x54]`（offset 0x150）と `param_1[0x58]`（offset 0x160）のウィジェットに
   `0xfc9`=1.0。
3. `param_1[0x2c]`（offset 0xb0）のウィジェットから `0xfb1`/`0xfb2` を読み
   `*(param_1+0xdc)` / `*(param_1+0xe0)` に保存し、`0xfb1`=0、`0xfb2`=1.0 に。
4. `0x00b39548 + 0x5c*4` の文字列（選択デバイス名）を小文字化し、
   `"%s %s %s"`（`0x00a6b774`）で `DAT_00b38ee0 / 小文字名 / DAT_00b38ee8` を連結
   → trait `0xfde` に設定。
5. `*(param_1+0xd8) = param_2`、文字列解放。

**`FUN_00587500(int param_1)`（26B, __cdecl, RET 0x4）**:
`if (param_1 - 1U < 0xd) return param_1 - 1; return 0xd;`
→ param_1（1..13）→ index 0..12、範囲外は 0xd。`FUN_0059c5e0` case 12/13 の分岐に使用。

### 40.7 コマンドハンドラ switch 表（FUN_0059c5e0, 630B）

`switch (param_2)`（1〜13、upper bound 0xc）。EAX = param_2 - 1。
`MOVZX EAX, byte ptr [EAX + 0x59c874]` → `JMP dword ptr [EAX*4 + 0x59c858]`。

バイトテーブル `0x59c874`（13 要素）:
`[0, 6, 6, 6, 6, 6, 1, 2, 3, 4, 6, 5, 5]`

分岐先テーブル `0x59c858`:

| case id | idx | 分岐先 | 動作 |
|---|---|---|---|
| 1 | 0 | `0x59c644` | 行 index `0xfae` 取得→0 なら 1.0 設定。else 時刻換算→XML 再ロード |
| 2,3,4,5,6 | 6 | `0x59c820` | 何もしない（末尾の `0xd < param_2` 判定へ） |
| 7 | 1 | `0x59c7f4` | サウンド3、`DAT_00b14f38` トグル、`FUN_0059b640` でデバイス切替 |
| 8 | 2 | `0x59c7a9` | サウンド3、`DAT_00b14ec8`↔`DAT_00b14ed8`、`DAT_00b14ed0`↔`DAT_00b14ee0` の2組スワップ、`0xd4`=1 |
| 9 | 3 | `0x59c78a` | サウンド1、`0xfae`=1.0（先頭行へ） |
| 10 | 4 | `0x59c6a8` | サウンド1、選択デバイス名を小文字化し `"%s %s %s?"` 確認ダイアログ `FUN_00579c10(&LAB_0059b8c0, ...)` |
| 11 | 6 | `0x59c820` | 何もしない |
| 12,13 | 5 | `0x59c751` | サウンド1、`FUN_00587500(param_2)` が 0xb なら `FUN_0059b980`、else `FUN_0059b920`。成功なら `0xd4`=1、失敗なら `0x59c699`=XML再ロード+画面更新 |

末尾: `if ((0xd < param_2) && (*(char*)(param_1+0xe4) == 0)) FUN_0059c3f0(param_3);`
→ **14 以上のコマンドは `FUN_0059c3f0` へ委譲**（`param_1+0xe4`=0 の条件付き）。

**Android 向け対応付け**:
- id 7 = デバイス切替トグル（`0x3fd` 内部フラグ）
- id 8 = 軸（左右スティック）入替
- id 9 = リスト先頭へ
- id 10 = 「このデバイスに割り当てますか？」確認ダイアログ
- id 12 / 13 = 前 / 次のデバイス（`FUN_00587500` で 12→11 が 0xb なので **id 12 が逆送り**）
- id 11 = INSTRUCTIONS 表示中は無動作

### 40.8 ControlsMenu フィールドオフセット表（ctor FUN_005874a0）

ctor は Menu 基底 ctor → vtable 設定後:
`_memset(param_1+10, 0, 0x34)`（**offset 0x28**、13 dword）、
`_memset(param_1+0x18, 0, 0x74)`（**offset 0x60**）、
`param_1[0x17] = 0xff`（offset 0x5c）、
`*(byte*)(param_1+0x35) = 0`（0xd4）、`param_1[0x36] = 0`（0xd8）、
`*(byte*)(param_1+0x39) = 0`（0xe4）。

| offset | 型 | 意味 |
|---|---|---|
| 0x28〜0x5b | Widget*[13] | **13 個のウィジェットポインタ**（生成時存在チェックの対象） |
| 0x5c | int | 選択デバイス index（初期 0xff、0=キーボード / 1=マウス / 2=ゲームパッド） |
| 0x60〜0xd3 | - | 内部作業領域（0x74 バイト） |
| 0xd4 | bool | キー割当更新フラグ（1=要更新） |
| 0xd8 | int | 確定デバイス種別（`FUN_0059c3f0` の param_2） |
| 0xdc | float | trait 0xfb1 退避値（`FUN_00588bd0(0xfb1)`） |
| 0xe0 | float | trait 0xfb2 退避値（`FUN_00588bd0(0xfb2)`） |
| 0xe4 | bool | 委譲ゲート（0 のとき 14 以上のコマンドを `FUN_0059c3f0` へ） |

**補足**: `FUN_0059bd80`（1629B、更新）は冒頭で `0xd4` を判定し、
選択デバイス名を `0xfb3`、キー状態バイトを
`*(byte*)(iVar11*0x1d + (*(DAT_00b33398+0x20)) + iVar7 + 0x1b70)`
（**デバイス index x 0x1d ストライド**）から読む。キー名解決は
iVar11==0→`(&DAT_00b39578)`（238 要素キーボード名）、
iVar11==1→`(&DAT_00b39554)`（9 要素マウス名）、
iVar11==2→ゲームパッド `"%s %d"` または `(&DAT_00b398b0 + uVar10*4)`。
`DAT_00b14ed0 == 1` なら `0xfb0 = DAT_00b38ec0` / `0xfb1 = DAT_00b38eb8`、else 逆。

### 40.9 Android 移植向け確定値

- **ControlsMenu id = 0x3fd**、vtable `0x00a69ff4`、XML `Data\Menus\Options\controls_menu.xml`。
- **メニューレベル**: `no_click_past`、alpha=0、locus=true、**explorefade=0.25**（非ポーズ）、
  背景 666x580、`_button_buffer`=50、文字色 RGB=(117,59,33)。
- **PAGE 1 id**: 1（Return2）/ 5（感度スライダー）/ 7（Invert Y）/ 8（Swap Sticks）。
- **PAGE 2 id**: 1（Return、user4 必須）/ 2（縦スクロール）/ 4（リストペイン）/
  **9（Controls ボタン、user0==1）** / **10（Defaults、user0==2 かつ user4）** /
  **11（INSTRUCTIONS、user0==2 かつ NOT user4）** / **12（前デバイス）** / **13（次デバイス）**。
- **スクロール**: min=0 / max=`list_pane.childcount - 6` / step=1 / user4=5 / user6=3 /
  **可視行数 user8=6** / 現在値 user7。リストペイン y = `- user7 * ControlsMenu.user1`。
- **キー行高**: `ControlsMenu.user1 = 400 / 6 ≒ 66.7`。
- **キー行テンプレート**: `repeatvertical=true`、`user0=index`、`user1=アクション名`、
  `user2=parent.width-128`、`user3=キー名`、`_left_edge_buffer=10`。
- **キー行生成**: index 0..29 を走査し `FUN_0059b670` でフィルタ
  （有効 = 0..0x10 と 0x1a/0x1b/0x1c）、行 index = `0x0e + idx`、アクション名 = `DAT_00b399d0[idx]`。
- **デバイス**: index 0=キーボード / 1=マウス / 2=ゲームパッド。
  id 12 = 逆送り、id 13 = 順送り。存在チェックは `DAT_00b33398+0x20` の
  +0x28 / +0x2c / +0x18f0。
- **確定動作**: id 7=トグル、id 8=軸入替、id 9=先頭へ、id 10=確認ダイアログ、
  id 11=無動作、id 14 以上=`FUN_0059c3f0` へ委譲（`0xe4==0` 時）。
- **`controls_instructions`（id 11）は string 未指定** → 文言はコード設定。移植側は別途用意。
- **id 1 はページ1/2 で共有**（`user0` で排他）。`user4=false`（selected モード）時は
  Defaults / Return が非表示。
- **`FUN_0059c3f0` の trait 定数**: `0xfcc/0xfcd/0xfce = 128.0f`、デバイス名連結書式 `"%s %s %s"`。
- **静的未解決**: `0x00b38ee0` / `0x00b38ee8`（書式の語句）、`0x00b38da0` / `0x00b38da8`（表示文字列）、
  `0x00b39548[3]`（デバイス名）、`0x00b399d0[29]`（アクション名）、`0x00b39554` / `0x00b39578`
  （マウス / キーボード名）はいずれも **.data の初期化領域外（RAW 0x700c00〜0x731800 =
  VA 0xb02000〜0xb33800）にあり静的ファイル上ゼロ**＝実行時書込。静解析での文字列回収は不可。
- **id 集合の対応**: §28 の vtable 53 件と突き合わせ済み。ControlsMenu の追加 ID
  （0x3fd）と PAGE1/PAGE2 の id 12 / 13 / 9 / 10 / 11 を反映すれば移植側のコマンド表が完成する。

### 40.10 残課題（A6 以降）

- `FUN_0059bd80`（1629B 更新）の後半 iVar11 分岐（キー割当設定の `FUN_00403f50` 系）は
  概略まで。キー再割当確定時の個別書込先を厳密化するなら追加精読が必要。
- `0x00b33e9c` の唯一の FSTP 書込命令 VA は未特定（§34 補足）。
- `FUN_005deb80` 後半（IDX 180〜629）と `FUN_005dbbd0`（2377B）の精読は A3 補足として残。
- `controls_instructions` の string 本体（コード供給）の実体文字列は静解析では未回収。

## §41 NPC → プレイヤーのダメージ計算経路（§33.1 の補強・詳細化）

§33.1 で「入口と式の骨格」は文書化済み。本節はそこに欠けていた**未文書化関数 3 件**
（`FUN_00484850` = HP 取得 / `FUN_00488830` = HP 書込＋被弾タイマーリスト / `FUN_00403c00` = ダメージ係数 getter）と、
**呪文・反射ダメージ経路 `FUN_005f3870`** を確定値として埋める。§33.1 と重複する記述は再掲しない。

### 41.1 AC（アクター）値アクセスの実体 — 未文書化の中核 3 関数

| 関数 | VA | サイズ | 役割 |
|---|---|---|---|
| `FUN_00484850` | 0x00484850 | 282B | HP 取得（絶対値 / 割合×100） |
| `FUN_00488830` | 0x00488830 | 621B | HP 書込（被弾タイマーリストの追加・更新・削除を兼ねる） |
| `FUN_00403c00` | 0x00403c00 | 20B | ダメージ係数グローバルのアドレスを返す getter |

これら 3 件は §40 までの RE_EXE_REPORT に一切出現していない（`00484850` / `00488830` = 0 件、`00403c00` = §33.1 内 1 件のみ）。

#### 41.1.1 `FUN_00484850` — HP 取得

```
float10 __thiscall FUN_00484850(int *param_1, char param_2)
```
- `param_1` = アクター、`param_2 != 0` = 割合モード。
- 冒頭で `FUN_009832e6(param_1[2],0,&PTR_PTR_00b03248,&PTR_PTR_00b05cf4,0)` により
  サブオブジェクト（AV コンテナ）を解決。null なら `DAT_00a30634` を返す（死亡センチネル）。
- `(int*)*param_1 == 0 || *(int*)*param_1 == 0` の縮退分岐：
  - `param_2 != 0` → `_DAT_00a2fe7c`（= 1.0f）を返す
  - `param_2 == 0` → vtable +0x10 の値を返し、負なら `_DAT_00a2fc78` を加算補正
- 通常経路：
  - `fVar4 = FUN_0041e810()`（AV 現在値）
  - `param_2 == 0` → `fVar4` をそのまま返す（**絶対 HP**）
  - `param_2 != 0` → `iVar3 = vtable +0x10`（AV 最大値）を取得し負補正後、
    `FUN_004842f0((fVar4 / iVar3) * _DAT_00a309f0)` を返す。`_DAT_00a309f0` = 100.0f。
    → **HP パーセンテージ = 現在値 / 最大値 × 100.0f**

**Android 移植向け確定値**：`param_2 == 0` が `GetHealth()`、`param_2 != 0` が `GetHealthPercent()`（0..100）。
`FUN_00484850(0)` が HP 減算前後の読み取りに使われる（41.3 参照）。

#### 41.1.2 `FUN_00488830` — HP 書込（被弾タイマーリスト管理）

```
void __thiscall FUN_00488830(int *param_1, float param_2, int param_3,
                             undefined4 *param_4, char param_5)
```
- `param_1` = アクター（`*param_1` が管理リスト先頭）、`param_2` = 設定値、
  `param_3` = サブオブジェクト、`param_4` = リスト探索キー（比較対象ポインタ）、`param_5` = 末尾削除フラグ。
- 冒頭：SEH セットアップ（`uStack_4 = 0xffffffff` / `puStack_8 = &LAB_009afbf6`）＋スタッククッキー
  `DAT_00b30aac`。`iVar3 = FUN_0046ca20(param_1[2])` で上限値を取得。
- **追加 / 削除の判定**：
  ```
  if ((param_2 < (float)iVar3) || (iVar3 < iVar5))   // iVar5 = FUN_009828c0()
      追加・更新経路
  else
      削除・リセット経路
  ```
- 追加経路：`*param_1` が null なら新規ノード作成
  （`FUN_00401f00(0x14)` = 20B 確保 → `FUN_00422ee0()` → `FUN_00401f00(8)` = 8B 確保 →
  `FUN_0041ee90(param_2)` → `FUN_00446cb0(uVar8)` → `*param_1 = puVar7`）。
  既存ノードは `*puVar7 == param_4` を探して `FUN_0041ecb0(param_2)` で値更新。
- **削除経路（末尾 261〜278 行）**：
  ```
  puVar7 == param_4 のとき
      FUN_0041f610();                        // ノード値リセット
      if ((puVar7[1] == 0) && (param_5 != 0)) {   // puVar7[1] = +0x04 = next ポインタ
          FUN_00485e80(param_1[2], 1, 0);    // AV 変動通知（stat 更新通知）
          FUN_0065c620(puVar7);
          (**(code **)*puVar7)(1);           // vtable[0] = デストラクタ
      }
  ```
- 中盤で `*(int**)(param_3 + 4)` が非 null なら `(**(code**)(**(int**)(param_3+4) + 0x40))(0x8000000)` を呼ぶ
  （サブオブジェクト側の変更通知、フラグ 0x8000000）。

**Android 移植向け確定値**：実質は「HP 値をアクターへ書き戻し、同時に被弾タイマー／累積ダメージの
連結リスト（ノード 20B、+0x04 = next）を管理する」関数。ダメージ適用の最終段で必ずここを通る。
`FUN_005f3870` からは 3 引数形（`param_4` / `param_5` 省略相当）で呼ばれる。

#### 41.1.3 `FUN_00403c00` — ダメージ係数 getter

```
undefined4 * __fastcall FUN_00403c00(undefined4 *param_1)
```
アセンブラ全文（20B）：
```
MOV  EAX,ECX
TEST EAX,EAX
JNZ  0x00403c13
FLDZ
MOV  EAX,0xb35464
FSTP float ptr [0x00b35464]
RET
```
- `param_1 == 0` のとき `&DAT_00b35464` を返し、同時に `DAT_00b35464 = 0.0f` へ初期化。
- **乱数関数ではない。グローバル float `DAT_00b35464` のアドレスを返す getter。**
- `DAT_00b35464` は静的初期化領域（.data 初期化終端 VA 0xb33800）を超えるため、
  静解析では初期値 0.0 のみ観測可能。**実行時に書き込まれる係数**（難易度・GMST 由来の乗数）。
  書き込み元関数は未特定（41.7 残課題）。

### 41.2 呪文・反射ダメージ経路 `FUN_005f3870`（新規・アセンブラ裏取り済み）

```
undefined4 __thiscall FUN_005f3870(int *param_1, undefined4 *param_2, float param_3, char param_4)
```
`param_1` = 対象アクター、`param_2` = 効果／呪文記述、`param_3` = ベースダメージ。
サイズ = 729B。`tmp\ghidra_f3870_clean.txt`（398 行）で裏取り。

#### 41.2.1 入口ガードと抵抗力チェック
```
if ((param_2 == 0) || (param_3 <= 0.0)) return 0;
iVar3 = param_2[2];                        // 種別コード
if (iVar3 == 0x14) { ... }                 // 環境／特殊効果分岐
cVar2 = FUN_004b4c70();                    // 対象がコマンド系か判定
if (cVar2 == 1) { FUN_005f23b0(0x12); FUN_005f23b0(0x12); }
else if (cVar2 != 0) { /* skip */ }
FUN_005f23b0(0x1b); FUN_005f23b0(0x1b);
```
- 抵抗判定用グローバル `0x00b37508` を使用（`005f3907 MOV ECX,0xb37508`）。
- `FUN_005f23b0(0x12)` = 種別 0x12、`FUN_005f23b0(0x1b)` = 種別 0x1b の抵抗力チェックを 2 回ずつ呼ぶ。

#### 41.2.2 ダメージ確定 → HP 減算（命令列）
```
005f3920  CALL 0x00403c00                  ; EAX = &DAT_00b35464
005f3925  FLD  float ptr [EAX]             ; 係数
005f3927  FMUL float ptr [ESP+0x2c]        ; × param_3（ベースダメージ）
005f392b  FSTP float ptr [ESP+0x28]        ; = 実ダメージ
005f3933  CALL 0x00484850                  ; (PUSH 0x0, ECX = 対象) → 現在 HP
005f3938  FSUB float ptr [ESP+0x28]        ; HP - ダメージ
005f393c  FSTP float ptr [ESP+0x30]
005f3940  FLD1 / FCOMP / TEST AH,0x41 / JNZ
005f394d  FLDZ / FSTP                      ; < 1.0 なら 0.0 へクランプ（死亡）
```
- **実ダメージ = `DAT_00b35464`（係数グローバル） × `param_3`（ベースダメージ）**
- **HP 減算は `現在HP - 実ダメージ`、結果が 1.0 未満なら 0.0 にクランプ。**
  近接 `FUN_005febf0` の式（スキル係数・アーマー軽減を含む多段）とは異なり、こちらは 1 乗算のみのシンプル経路。

#### 41.2.3 デバッグログ
```
005f3953  CMP byte ptr [0x00b3b908],0x0     ; ダメージログ有効フラグ
005f3961  MOV EAX,[EBP+0x28]                ; 対象名ポインタ
           (null → 0xa2f7ec = デフォルト名)
005f3973  CALL 0x00484850                   ; 現在 HP をログ用に再取得
005f3991  CALL 0x004da2a0                   ; 可変長引数準備
005f3997  PUSH 0xa6e76c                     ; "%.20s's %s takes %.2f points of damage (%.2f/%.2f)!"
005f399c  CALL 0x00579b60                   ; ログ出力
```
`0x00b3b908` = ダメージログ有効化フラグ、`0x00a2f7ec` = 名称未設定時の代替文字列、
`0x00a6e76c` = ダメージログ書式。

#### 41.2.4 HP 書込と死亡判定
```
005f39a4  MOV EAX,[EDI] / MOV EAX,[EAX]
005f39ab  LEA ECX,[ESI+0x44]               ; stat コンテナ
005f39ae  CALL 0x0041e6f0                  ; AV エントリ取得
005f39be  CALL 0x00488830                  ; HP 書込（41.1.2）
005f39c7  MOV EDX,[ESI] / MOV EAX,[EDX+0x2c0] / CALL EAX   ; HP 更新通知（vtable +0x2c0）
```
デコンパイル側の対応箇所：
```
fVar10 = (float10)FUN_00484850(0);
_param_4 = (float)(fVar10 - (float10)(float)param_2);
if (_param_4 < 1.0) _param_4 = 0.0;
uVar5  = FUN_0041e6f0(*(undefined4*)*puVar1, 1);
uVar11 = CONCAT44(1, *(undefined4*)*puVar1);
FUN_00488830(_param_4, uVar5, uVar11);
if (iVar9 != 0) (**(code**)(*param_1 + 0x2c0))();
if (0.0 < _param_4) return 0;
```
死亡処理（0.0 クランプ時）：
```
if ((puVar1[2] != 0 && *(char*)(puVar1[2]+4) == '!') && (param_1[0x16] != 0 && ...))
|| ((iVar9 != 0 && (cVar2 = FUN_00468ff0(0xd,0), cVar2 != 0)) && (iVar3 = (vtable+0xf8)(), iVar3 != 0))
then
    if ((param_1 != DAT_00b333c4) && ((vtable+0x78)() == 0))
        iVar9 == 0  → vtable +0x118
        iVar9 != 0  → vtable +0x120        ; ノックバック方向算出（FUN_00711440）
    uVar5 = (**(code**)(*param_1 + 0x2c8))(...);
    FUN_004dc000(param_1, uVar5);
    return 1;
```
- `DAT_00b333c4` = プレイヤーアクターのグローバルポインタ（プレイヤー自身への反射は除外）。
- `FUN_00468ff0(0xd, 0)` = 種別 0xd の状態判定。
- **戻り値 1 = 死亡（HP が 0 にクランプされた）、0 = 生存。**

**Android 移植向け確定値**：呪文・反射ダメージは
`実ダメージ = 係数(DAT_00b35464) × ベースダメージ` → `HP = max(0, HP - 実ダメージ)` →
`FUN_00488830` 相当で書き戻し → `HP <= 0` で死亡処理（vtable +0x2c8 経由）。
近接（`FUN_005febf0`）とは別経路であることに注意。

### 41.3 近接経路 `FUN_005febf0` の適用順序（§33.1 の整理）

§33.1.2 / §33.1.3 の式を、実装に落とせる順序へ整理し直す。

| 段 | 内容 | 根拠 |
|---|---|---|
| 1 | 難易度倍率 `uStack_1a8 = (vtable+0x26c)() * (vtable+0xec)()` | `FUN_005febf0` 冒頭 |
| 2 | 武器無効チェック `(vtable+0x284)(0x41)` が 100 未満 → "normal weapons have no effect" | 同上 |
| 3 | スニーク判定 `FUN_005e0550()` / しきい値 `DAT_00b372f0` / `FUN_005477f0` | 同上 |
| 4 | ベースダメージ `(vtable+0x34c)()` × AV（0 = 体力 / 7 = 攻撃力 / 0x11 = レベル） | 同上 |
| 5 | 武器負荷 `FUN_00484f80`（NPC）/ `FUN_004b9f60`（Player） | 同上 |
| 6 | 合成 `FUN_00547070`（9 引数） = `f2 * f1 * f3 * f4 * f5 * param_7` | `FUN_00547070` |
| 7 | 1 発上限 `(1.0 - (int)fVar25 / 100.0) * fVar21` | 同上 |
| 8 | 反射 `(vtable+0x220)(fStack_1ac * fVar21, 0, param_4)`（種別 0x3b） | 同上 |
| 9 | アーマー軽減 `fStack_1c4 = (vtable+0x348)() / 100.0f`（>=100 で 1.0、上限 `_DAT_00b36ea0`） | 同上 |
| 10 | スタッガー `(vtable+0x3b4)(...)` | 同上 |
| 11 | 魔法ダメージ適用 `(vtable+0x398)(param_1, piVar22, fStack_1c4, uVar8)` | 同上 |
| 12 | HP 比例 `uStack_1b4 = piVar22 / (vtable+0x284)(8)`、係数比較 `FUN_00403c00()` | 末尾 680〜946 行 |

- **`FUN_00547070`**（206B / 55 insns）：戻り値 = `fVar2 * fVar1 * fVar3 * fVar4 * fVar5 * param_7`。
  `fVar5 = FUN_00547b90(param_1, param_2)`（0..100 クランプ）。定数：`0xb36dd8` / `0xb36df8` / `0xb36df0` /
  `0xb36de0` / `0xb36de8` / `0xb36e08` / `0xb36e00`、`0xa3b150`（0.01 相当）。
- **`FUN_00547140`**（246B / 79 insns）：魔法／呪文ラッパ。`uVar3 = (param_1+0x88+0x10)() & 0xffff`、
  戻り値 = `_DAT_00b36d00 * fVar4 + 出力`（`0xb36d00` = マスタ係数）。
- **`FUN_00547f00`**（31B）：線形補間 `return _DAT_00b37530 - (1.0 - param_1) * _DAT_00b37538;`
- **`FUN_00547b90`**（101B）：0..100 クランプ付きスキル合成。定数 `0xb37390`（float）/ `0xb37388`（int）。
- **`FUN_006034b0`**（70B）：AV index 8（HP）低下時の死亡判定。
  `fVar2 = (vtable+0x288)(8)`、`fVar2 < _DAT_00a2f948` なら `FUN_006005f0`。
- **`FUN_005480a0`**（132B）：index → グローバル値テーブル（1..10、`0xb37620`〜`0xb37668`）、
  null なら `0xb35464` へフォールバック。

**Actor vtable オフセット一覧（§33.1 の補強）**

| offset | 用途 |
|---|---|
| +0x78 | 死亡／無敵判定（0 = 通常） |
| +0xec | レベル／HP 係数 |
| +0xf8 | 死亡状態取得 |
| +0x118 / +0x120 | ノックバック方向（iVar9 0 / 非 0） |
| +0x164 | 戦闘状態 |
| +0x170 | 種別 |
| +0x174 | 座標 |
| +0x198 / +0x19c | 死亡判定 / 魔法チェック |
| +0x1a0 | 呪文抵抗 |
| +0x210 / +0x214 | 攻撃カウンタ |
| +0x218 | 回避 |
| +0x220 | 反射ダメージ適用 |
| +0x26c | 難易度倍率 |
| +0x284 / +0x288 | AV 取得（0x41 = 武器種別、8 = HP 最大） |
| +0x2c0 / +0x2c4 | HP 更新通知 / HP 変動 |
| +0x2c8 | 死亡関連 |
| +0x2d4 | ダメージ効果適用（`FUN_005e46c0`） |
| +0x304 | 死亡通知 |
| +0x308 | 死亡処理 |
| +0x330 | 遮蔽／ターゲット |
| +0x348 | アーマー値 |
| +0x34c | ベースダメージ |
| +0x398 | 魔法ダメージ適用 |
| +0x39c | 効果 |
| +0x3a8 | 通知 |
| +0x3ac | ダメージ入口（`FUN_005febf0`） |
| +0x3b4 | ノックバック |
| +0x44c | 条件チェック |

AV（Actor Value）index の解釈：0 = 体力、3 = 魔法、7 = 攻撃力、8 = HP（最大）、0xf = 疲労、
0x10 = 速度、0x11 = レベル、0x3b = 反射、0x41 = 武器種別／品質。

### 41.4 難易度倍率 `FUN_005e2560`（§33.1.3 の再確認と Android 実装値の訂正）

```
fVar2 = *(float*)(DAT_00b333c4 + 0x734) * _DAT_00b37b88;   // × 10.0
if (playerDifficulty <= 0)
    fVar2 = fVar2 + 1.0;
else
    fVar2 = 1.0 / ABS(fVar2 - 1.0);
```
- `DAT_00b333c4` = プレイヤー、`+0x734` = 難易度設定値（0..1 のスライダー実値想定）。
- `_DAT_00b37b88` = 10.0f。
- 呼出元 3 件：`FUN_00699eb0` / `FUN_006a86f0` / `FUN_005e58f0`。

**Android 移植向け確定値（重要・訂正）**：
現行移植コードの暫定式 `pow(6, (d - 50) / 50)` は**オリジナルと一致しない**。
オリジナルは「難易度スライダー実値（0..1）」を 10 倍した値 `x` に対し、
- `x <= 0`（最低難易度側）→ 倍率 = `x + 1.0`
- それ以外 → 倍率 = `1.0 / ABS(x - 1.0)`

の 2 分岐。UI の 0..100 スライダーを使う場合は `x = slider / 100.0 * 10.0` として同じ式へ通す。

### 41.5 魔法・遠距離の入口（§33.1.4 の補強）

- 魔法ダメージの共通適用先は `Actor::ApplyDamageEffect` = vtable +0x2d4 = `0x005e46c0`。
  `FUN_005febf0` の段 11 から `(vtable+0x398)(...)` 経由で到達する。
- 反射・呪文の直接適用は 41.2 の `FUN_005f3870`。
- 遠距離（弓）の命中判定は `FUN_0041df00` のレイキャスト（§33.1.4 記載）＋段 12 の HP 比例判定。
- 生存中も `FUN_00488830`（41.1.2）が最終書込点になるため、**Android 側は
  「近接 / 魔法 / 遠距離」いずれの経路でも HP 書込を 1 箇所（`FUN_00488830` 相当）へ集約**すれば一致する。

### 41.6 Android 移植向け確定値（§41 のまとめ）

1. **HP 読み書きの共通 API を新設**：
   - `GetHealth(actor)` = `FUN_00484850(actor, 0)`
   - `GetHealthPercent(actor)` = `FUN_00484850(actor, 1)`（0..100）
   - `SetHealth(actor, value)` = `FUN_00488830(actor, value, ...)`（被弾タイマーリスト管理を含む）
2. **呪文／反射ダメージ式**：`damage = coefficient * baseDamage`、`HP = max(0, HP - damage)`。
   係数は実行時グローバル（`DAT_00b35464`）で、静解析では 0.0 初期値のみ。
3. **難易度倍率**：§41.4 の 2 分岐式へ差し替え（`pow(6, ...)` を廃止）。
4. **死亡処理**：HP が 1.0 未満へクランプされた時点で vtable +0x2c8 → `FUN_004dc000`。
   プレイヤー自身（`DAT_00b333c4`）への反射は除外。
5. **NPC → プレイヤーの経路は新設が必要**（§33.1.5 と同一）。NPC の `FUN_005febf0` /
   `FUN_005f3870` 相当から対象をプレイヤーにして呼ぶ形にする。
6. **アーマー軽減は `1 - AV(0x348)/100` を上限 `_DAT_00b36ea0` でクランプ**し、
   ダメージ式の乗算段（段 9）に適用（§33.1.2 の式と同じ）。

### 41.7 残課題（§41）

- `DAT_00b35464`（ダメージ係数グローバル）の**書き込み元関数が未特定**。
  静的初期化領域外のため実行時書込。候補は GMST ロード経路（`FUN_005480a0` の
  テーブル `0xb37620`〜`0xb37668` 書き込みと同一系の可能性）。
- `FUN_004842f0`（`FUN_00484850` 内から呼ばれる丸め／クランプ関数）の挙動未確定。
- AV index 4 / 0xf の厳密な意味（出血／疲労のいずれか）は未確定。
- `FUN_00468ff0(0xd, 0)` の種別 0xd の意味（死亡判定の前提状態）未確定。
- `0x00b3b908`（ダメージログ有効化フラグ）の設定元未確定（デバッグ設定と推定）。

---

## §42 低優先追加解析：Timer オブジェクト・AV フォールバック・コントロールキー割当表

§41 までの残課題のうち、main から優先度低として依頼のあった 3 件（DAT_00b399d0 の並び順、
DAT_00b35464 の書き込み元、0x00b33e9c の書き込み元）を確定した。あわせて、解析中に発見した
**PE セクション raw→VA delta の誤り**をここで訂正する。§41 以前の一部アドレスは本節の訂正表で読み替えること。

### 42.1 【最重要訂正】PE セクション raw→VA delta（pefile で実測）

これまで .data を delta=0x3FEC00 として読んでいたが誤り。正しい値は下表（pefile の
`pe.get_data(VA-ImageBase,...)` を使えば delta を意識する必要はない）。

| セクション | VA | VirtualSize | PointerToRawData | raw->VA delta |
|---|---|---|---|---|
| .text | 0x00401000 | 0x00626C39 | 0x00000400 | 0x00400C00 |
| .rdata | 0x00A28000 | 0x000D98D2 | 0x00627200 | 0x00400E00 |
| .data | 0x00B02000 | 0x000A9C1C | 0x00700C00 | 0x00401400 |
| .tls | 0x00BAC000 | 0x000001B9 | 0x00731800 | 0x0047A800 |
| .rsrc | 0x00BAD000 | 0x0000166C | 0x00731A00 | 0x0047B600 |
| .bind | 0x00BAF000 | 0x00056000 | 0x00733200 | 0x0047BE00 |

- 検証例: VA 0x009DC2BC の raw = 0x009DC2BC - 0x00400C00 = 0x005DB6BC、そこに `d9 1d 9c 3e b3 00`（FSTP [0x00b33e9c]）がある。
- **.data は VirtualSize 0x0A9C1C に対し生データは 0x030C00 のみ**（raw が占める最終 VA = 0x00B32C00）。
  したがって **VA 0x00B32C00 より上の .data はゼロ充填（BSS 相当）でファイル内容を持たない**。
  ここに属する 0x00B33E88 / 0x00B33E90 / 0x00B39530 / 0x00B39DB0 が生バイト走査で「空」になるのはこのため（バグではない）。
- **§41 の記述訂正**: 「0x9dbebe」「0x4ae7c7」「0x5a27b6」等は delta 誤りに起因する残骸。
  正しい値は本節 42.3 以降を用いること（0x9dbebe の相当は Timer::Reset の 0x009DC2A0 と
  FSTP [0x00b33e9c] の 0x009DC2BC）。

### 42.2 DAT_00b399d0 = コントロールキー割当テーブル（29 要素、要素サイズ 0x38）

`FUN_009f6040`（静的初期化）が 29 エントリを順に格納する。各要素は `MOV [DAT_00b399d0 + 4*i], EAX`
の形（先頭 dword がハンドラ／アクション識別子）で、格納命令の VA はほぼ 0x38 間隔。

| idx | アクション名 | 先頭 dword 格納 VA |
|---|---|---|
| 0 | Back | 0x009F609B |
| 1 | Sldleft | 0x009F60D3 |
| 2 | Sldright | 0x009F610B |
| 3 | Use | 0x009F6143 |
| 4 | Activate | 0x009F617B |
| 5 | Block | 0x009F61B3 |
| 6 | Cast | 0x009F61EB |
| 7 | Rdyitem | 0x009F6223 |
| 8 | Crouch | 0x009F625B |
| 9 | Run | 0x009F6293 |
| 10 | Togglerun | 0x009F62CB |
| 11 | Automove | 0x009F6303 |
| 12 | Jump | 0x009F633B |
| 13 | Togglepov | 0x009F6373 |
| 14 | Menumode | 0x009F63AB |
| 15 | Restmenu | 0x009F63E3 |
| 16 | Quickmenu | 0x009F641B |
| 17 | Quick1 | 0x009F6453 |
| 18 | Quick2 | 0x009F648B |
| 19 | Quick3 | 0x009F64C3 |
| 20 | Quick4 | 0x009F64FB |
| 21 | Quick5 | 0x009F6533 |
| 22 | Quick6 | 0x009F656B |
| 23 | Quick7 | 0x009F65A3 |
| 24 | Quick8 | 0x009F65DB |
| 25 | Quicksave | 0x009F6613 |
| 26 | Quickload | 0x009F664B |
| 27 | Grab | 0x009F6683（+49 dword は 0x00B39A40 に追加保持） |
| 28 | （終端センチネル） | 0x009F66B5 |

- テーブル絶対参照は 5 件: 0x0040C652（getter）/ 0x00575D53 / 0x00575D73（ControlsMenu）/ 0x0059BC47（§40）/ 先頭 0x009F609B。
- Android 移植では §40 のとおり独自キー名テーブルで代替してよい。**行順のみ本表に合わせる**。

### 42.3 0x00b33e9c の書き込み元 = Timer オブジェクトのフレームデルタ時間

#### 42.3.1 静的初期化関数の発見（CRT init テーブル）

.rdata の静的初期化関数ポインタ配列（0x00A28830 付近、.text の 0x9DC1xx 帯が連続）に、
**0x009DC280 と 0x009DC2A0 が連番で登録**されている。

```
00a28830 -> 009dc100   00a28844 -> 009dc260
00a28834 -> 009dc160   00a28848 -> 009dc280   <- 名前ペア設定
00a28838 -> 009dc1c0   00a2884c -> 009dc2a0   <- Timer::Reset
00a2883c -> 009dc1e0   00a28850 -> 009dc2d0
00a28840 -> 009dc240   00a28854 -> 009dc330
```

→ 両者は **プロセス起動時の静的コンストラクタ**として実行される。直接 CALL は .text 内に存在しない。

#### 42.3.2 FUN_009DC280 = RTTI 型記述子の登録（Timer ではない）

```
009dc280  68 b0 fa b3 00     PUSH 0x00b3fab0        ; 基底型記述子ポインタ
009dc285  68 e0 d0 a3 00     PUSH 0x00a3d0e0        ; 型名文字列 "BSTempNode"（ASCII、0x00A3D0D8 は "BSTempNodeManager"）
009dc28a  b9 88 3e b3 00     MOV  ECX,0x00b33e88    ; this
009dc28f  e8 8c 1f d3 ff     CALL 0x0070e222
009dc294  c3                 RET
```
CALL 先 0x0070E222 は 5 命令のセッター:
```
0070e222  8b c1              MOV EAX,ECX
0070e224  8b 4c 24 04        MOV ECX,[ESP+4]
0070e228  89 08              MOV [EAX],ECX        ; [this+0] = arg1
0070e22a  89 50 04           MOV [EAX+4],EDX      ; [this+4] = arg2
0070e22d  c2 08 00           RET 8
```
→ **0x00B33E88 = RTTI（型記述子）レコード: +0x00 = 型名文字列 ptr（0x00A3D0E0 = "BSTempNode"）、+0x04 = 基底型記述子 ptr（0x00B3FAB0）**。
Timer 本体は 0x00B33E90 からで、0x00B33E88 は別物。getter は 0x0043F520
（`MOV EAX,0x00b33e88; RET`）。同一性比較は 0x0047CD61 で、これは **IsKindOf 相当の型判定**:
```
0047cd53  8b 16            MOV EDX,[ESI]        ; vtable
0047cd55  8b 42 04         MOV EAX,[EDX+4]      ; vtable[1] = GetRTTI()
0047cd58  8b ce            MOV ECX,ESI
0047cd5a  ff d0            CALL EAX
0047cd5c  85 c0            TEST EAX,EAX
0047cd5e  74 12            JE   (不一致)
0047cd60  3d 88 3e b3 00   CMP EAX,0x00b33e88   ; 欲しい型と一致?
0047cd65  0f 84 91 00 00 00  JE  (一致)
0047cd6b  8b 40 04         MOV EAX,[EAX+4]      ; +0x04 を辿って基底型へ（継承チェーン）
0047cd6e  85 c0            TEST EAX,EAX
0047cd70  75 ee            JNZ 0x0047cd60
```
→ 各クラスの記述子は `+0x00 型名 / +0x04 基底記述子` の単方向リスト。0x00B3FAB0 が参照 45 件と
多いのは、これが **共通の基底型記述子（チェーンの根）** として多数の型判定に現れるため。
移植では RTTI を持たないので **この領域は実装不要**（クラス判定は C++ の型／enum で代替）。

#### 42.3.3 FUN_009DC2A0 = Timer::Reset（0x00b33e9c を含むブロックの初期化）

```
009dc2a0  ff 15 d0 80 a2 00  CALL [0x00a280d0]     ; GetTickCount
009dc2a6  d9 ee              FLDZ
009dc2a8  d9 15 94 3e b3 00  FST  [0x00b33e94]     ; +0x04 = 0.0f
009dc2ae  33 c9              XOR  ECX,ECX
009dc2b0  d9 15 98 3e b3 00  FST  [0x00b33e98]     ; +0x08 = 0.0f
009dc2b6  88 0d 90 3e b3 00  MOV  [0x00b33e90],CL  ; +0x00 = 0（フリーズフラグ解除）
009dc2bc  d9 1d 9c 3e b3 00  FSTP [0x00b33e9c]     ; +0x0C = 0.0f（delta time）
009dc2c2  89 0d a0 3e b3 00  MOV  [0x00b33ea0],ECX ; +0x10 = 0
009dc2c8  a3 a4 3e b3 00     MOV  [0x00b33ea4],EAX ; +0x14 = GetTickCount()
009dc2cd  c3                 RET
```
**重要**: Ghidra が独立関数として報告した `FUN_009dc2bc (size=18)` は**ゴースト関数**。
0x009DC2BC は本 Reset 関数の途中（FSTP 1 命令）であり、**ファイル全体で 0x009DC2BC / 0x009DC2B6 を
指す即値参照は 0 件**。したがって「0x00b33e9c の書き込み元関数」は **FUN_009DC2A0** が正しい。

- 絶対アドレス指定の `FSTP [0x00b33e9c]` は**ファイル全体で 1 件のみ（0x009DC2BC）**。
  通常フレームでの更新は `FSTP [ECX+0xC]`（レジスタ間接）で行われるため走査に出ない（42.3.4）。
- `FLD [0x00b33e9c]` は **74 件**（dt の消費者）。getter `float Timer::GetDeltaTime()` @0x004052A0
  （`FLD [0x00b33e9c]; RET`、直後 0x004052A7 は `FLDZ; RET` の 0.0f 版）。

#### 42.3.4 Timer::Update @ 0x0047D170（完全デコード、1 引数 stdcall / ECX=this）

```
0047d170  8b 54 24 04        MOV  EDX,[ESP+4]        ; arg = tick(ms)
0047d174  d9 ee              FLDZ
0047d176  2b 51 14           SUB  EDX,[ECX+0x14]     ; ms = tick - 前回 tick
0047d179  a1 b0 9d b3 00     MOV  EAX,[0x00b39db0]
0047d17e  83 ec 08           SUB  ESP,8
0047d181  a3 b4 9d b3 00     MOV  [0x00b39db4],EAX   ; 前フレームの生 ms
0047d186  89 15 b0 9d b3 00  MOV  [0x00b39db0],EDX   ; 今フレームの生 ms
0047d18c  80 39 00           CMP  byte[ECX],0        ; +0x00 フリーズフラグ
0047d18f  0f 85 b8 00 00 00  JNZ  0x0047d24d        ; フラグ時は ST0=0 のまま FSTP -> dt = 0
...
0047d1f0  （フラグ 0 経路: 前回との差分を ms で求め、下限/上限クランプ）
0047d213  d8 1d 4c d1 a3 00  FCOMP [0x00a3d14c]      ; 166.6667f と比較
0047d220  be a6 00 00 00     MOV  ESI,0x000000A6     ; 166 ms にクランプ
...
0047d238  dc 0d 40 0e a3 00  FMUL qword [0x00a30e40] ; * 0.001（ms -> s、m64 乗算）
0047d23e  89 51 10           MOV  [ECX+0x10],EDX     ; +0x10 = 今回 tick
0047d241  d9 59 0c           FSTP [ECX+0x0C]         ; +0x0C = dt(秒)   <- 0x00b33e9c
0047d244  d9 41 0c           FLD  [ECX+0x0C]
0047d247  d8 0d 04 67 b0 00  FMUL [0x00b06704]       ; * 1.0（タイムスケール）
0047d24d  d9 59 0c           FSTP [ECX+0x0C]         ; +0x0C = dt * timescale
0047d250  83 c4 08           ADD  ESP,8
0047d253  c2 04 00           RET  4
```
- 定数: 0x00A30E40 = **0.001（double）**、0x00A3D14C = **166.6667f（クランプ閾値）**、
  0x00B06704 = **1.0f（timescale）**、0x00A2FC78 = 4294967296.0f（FILD 負値補正用の 2^32）。
- **フリーズフラグ（[ECX+0x00] != 0）のときは dt = 0.0f を書く**。このため「時間停止 = dt 0」経路が
  エンジン共通の仕組みとして存在する（§31 で main が実装した dt=0 経路と同型）。
- 1 フレームの ms は最大 166 ms にクランプされる（約 6 fps 相当の下限保証）。

#### 42.3.5 メインループでの利用（0x0040D8A0）

```
0040d8a0  ff 15 d0 80 a2 00  CALL [0x00a280d0]      ; GetTickCount
0040d8a6  50                 PUSH EAX
0040d8a7  b9 90 3e b3 00     MOV  ECX,0x00b33e90
0040d8ac  e8 c0 f8 06 00     CALL 0x0047d170       ; Timer::Update(tick)
0040d8b0  d9 05 9c 3e b3 00  FLD  [0x00b33e9c]     ; dt
0040d8b6  0f b6 0d b8 33 b3 00  MOVZX ECX,byte[0x00b333b8]
0040d8bd  51                 PUSH  ECX
0040d8be  51                 PUSH  ECX            ; dt(8 バイト) + フラグ引数の準備
                     ...    CALL <フレーム更新>  ; (dt, byte[0x00B333B8])

```
→ **`0x00B33E9C` = そのフレームのデルタ時間（秒、timescale 乗算後）**で確定。
ゲーム全体の更新はここから dt を受け取る。

#### 42.3.6 Timer オブジェクトレイアウト（this = 0x00B33E90）

| offset | グローバル | 型 | 意味 |
|---|---|---|---|
| +0x00 | 0x00B33E90 | byte | フリーズフラグ（非 0 で dt = 0） |
| +0x04 | 0x00B33E94 | float | 時間アキュムレータ |
| +0x08 | 0x00B33E98 | float | 前回フレームの補正済みデルタ |
| +0x0C | 0x00B33E9C | float | **delta time（秒、timescale 乗算後）** 参照 164 件相当 |
| +0x10 | 0x00B33EA0 | dword | dt を確定した tick(ms) |
| +0x14 | 0x00B33EA4 | dword | 直前 tick(ms) |
| 別域 | 0x00B33E88 / 0x00B33E8C | ptr/ptr | RTTI 記述子 {型名 "BSTempNode", 基底記述子 0x00B3FAB0}（42.3.2、Timer とは別物・移植不要） |

グローバル 0x00B39DB0 / 0x00B39DB4 = 今／前フレームの生 ms デルタ（Update が更新）。

#### 42.3.7 Timer::Update の呼び出し元（9 件）

`MOV ECX,0x00b33e90; CALL 0x0047d170` の形:
0x0040616A / 0x004061AB / 0x0040D8A7（メインループ）/ 0x0040EACE / 0x00494A14 /
0x00494E5A / 0x004E15D0 / 0x004E18C2、および Reset 本体 0x009DC2B6（フラグ書込）。
→ メインループ以外に、UI/メニュー系のサブ更新（0x40EAxx / 0x494Axx / 0x4E1xxx）でも
独立に時間を進めている点に注意（移植では各系の dt 供給元として扱える）。

### 42.4 DAT_00b35464 = 未定義 AV 用のフォールバック float（書き込み元確定）

`FSTP [0x00b35464]`（`d9 1d 64 54 b3 00`）はファイル全体で **8 件**:
0x00403C0D / 0x00404E3E / 0x004AEBC0 / 0x004AEC0D / 0x005A06EF / 0x005A0736 / 0x005A2BB4 / 0x005A2BFF。
（§41 の 0x4AE7C7 / 0x5A27B6 / 0x9DBEBE は delta 誤り。上記が正。）

すべて同一パターン（例: 0x005A2BA8 帯）:
```
MOV EAX,[EAX*4 + 0x00b39530]   ; AV 定義ポインタ配列（実行時充填）
CMP EAX,EBX  (EBX = 0)
JZ  <fallback>
MOV EBP,EAX
JMP <after>
fallback: FLDZ
          MOV EBP,0x00b35464
after:    FSTP [0x00b35464]    ; 0.0f を書いて
          FLD  [EBP]           ; そのアドレスを読む
```
getter 帯（0x00403C00 等）も同型（null 時に `FLDZ; MOV EAX,0x00b35464; FSTP [0xb35464]; FLD [EAX]; RET`）。

→ **0x00B35464 は「そのアクターに該当 AV が存在しない」ときの 0 値を返すための一時グローバル**
（戻り値ポインタを成立させるための 0.0f 置き場）。実データではないため移植では
「AV 未定義なら 0.0f を返す」で等価。

**0x00B39530 は AV 定義ポインタ配列の先頭**で、.data の生データ範囲外（未初期化域）にあり
**実行時に充填される**。したがってファイル上は全要素 0 であり、生バイトでのAV名解決は不可。

### 42.5 Android 移植向け確定値

1. **raw->VA 換算**: .text 0x00400C00 / .rdata 0x00400E00 / .data 0x00401400。以降の再解析は pefile の
   `get_data` を使い、delta を手計算しないこと。
2. **フレームデルタ**: `dt = (tick - prevTick) / 1000.0f`、1 フレームあたり 166 ms でクランプ、
   最後に timescale（既定 1.0f、0x00B06704）を乗算。フリーズフラグ（0x00B33E90）が立つと dt = 0。
   → 移植の `Timer` は「ms 差分 -> 秒 -> クランプ -> timescale」の順で実装すれば一致する。
3. **ポーズ実装の参照**: メニュー等でゲームを止める正本はこの **Timer フリーズフラグ + dt = 0** 経路。
   更新系は dt = 0 を受けて自然に停止する設計（§30.4・§31 の dt=0 対応と整合）。
4. **メニュー入力抑制（§30）** と **AV フォールバック（本節 42.4）** は独立。AV 側は
   「未定義 AV は 0」で足りるため、0x00B35464 相当のグローバルは移植不要。
5. **コントロールキー割当**は §42.2 の 29 行の順序のみ合わせ、名称／キー割当は Android 独自でよい。

### 42.6 残課題（本節で未解決）

- 0x00B06704（timescale）の書き込み元（時間スケール変更 API）。
- 0x00B33E88 の 2 dword ペア = RTTI 記述子（型名 "BSTempNode" + 基底 0x00B3FAB0）と確定（§43.4）。移植不要。
- 0x00B33E98 の書き込み元（Update 内の補正経路のみが判明）。
- Timer::Update のうち ms 差の下限制御（負値・0 の扱い）の詳細分支。

## §43 0x00B06704 = GMST fGlobalTimeMultiplier（dt 乗算係数の正体）

§42.6 の残課題だった 0x00B06704（dt に乗算される float）の実体を確定した。

### 43.1 定数の所属

`.data` の 0x00B06700 から続く初期値付きブロック:

| offset | グローバル | 初期値 | 解釈 |
|---|---|---|---|
| +0x00 | 0x00B06700 | 0x00000040 (64) | 固定カウンタ |
| +0x04 | 0x00B06704 | **1.0f** | **GMST fGlobalTimeMultiplier の値（dt 乗算係数）** |
| +0x08 | 0x00B06708 | 0x00A3D150 | **設定名文字列へのポインタ** |
| +0x0C | 0x00B0670C | 0 | 未使用 |
| +0x10 | 0x00B06710 | 31,28,31,30,31,30,31,31,30,31,30,31, 0 | 月の日数表（12 か月 + 終端） |
| +0x2C | - | 31,59,90,... | 月頭の通算日（day-of-year）表 |

- **0x00A3D150 の実体は文字列 `"fGlobalTimeMultiplier:General"`**（ASCII、末尾 00 00 00）。
  つまり 0x00B06704 は **GMST（Game Setting）「fGlobalTimeMultiplier」（セクション General）の値**で、
  既定 1.0f。dt に対する時間倍率であり、フレーム更新の実時間デルタをゲーム時間へ変換する。
- 0x00B06700 側は「時計／カレンダー」ブロックで、月の日数表と月頭通算日表（31,59,90,...）を持つ。
  wait / rest の経過時間から日付を繰り上げるための表と判断できる。
- **0x00B06704 を直接書き込む命令は .text に存在しない**（`FSTP [0x00b06704]` も `MOV [0x00b06704],...` も 0 件。
  `04 67 b0 00` の出現は 4 件で全て読み出し／ポインタ取得）。
  設定値は **GMST セッター経由（ECX 間接）**で書き換えられる。

### 43.2 起動時の登録経路

CRT 静的初期化テーブル（§42.3.1）の次エントリ **0x009DC2D0**（テーブル位置 0x00A28850）が本値を登録する。

```
009dc2d0  6a ff                PUSH -1
009dc2d2  68 3a f4 9a 00       PUSH 0x009af43a        ; SEH スコープテーブル
...
009dc2f0  68 04 67 b0 00       PUSH 0x00b06704        ; &fGlobalTimeMultiplier
009dc2f5  b9 f0 7b b0 00       MOV  ECX,0x00b07bf0
009dc2fa  c7 44 24 10 00 00 00 00  MOV [ESP+0x10],0
009dc302  e8 d9 7d a2 ff       CALL 0x004000E0        ; 名前付き設定の登録ヘルパ
009dc307  68 90 86 a1 00       PUSH 0x00a18690        ; 追加登録スタブ
009dc30c  e8 a3 5c fa ff       CALL 0x00982FB4
```
- 0x00A18690 は .rdata 内の生成スタブ（`PUSH 0x00b06704; MOV ECX,0x00b07cfc; CALL ...`）で、
  0x00982FB4 経由で別レジストリ（0x00B07CFC）にも登録される。
- 0x009AF430 にも生成スタブ（`MOV ECX,0x00b06704; JMP 0x00403BC0`）がある。0x00403BC0 は
  `MOV EAX,[ECX+4]; TEST EAX,EAX; CMP byte[EAX],0x53('S'); ...` の文字列判定ヘルパ
  （[ECX+4] = 設定名文字列ポインタの分類）。
- この「構造体 + 設定名文字列 + 登録スタブ」は **GMST／名前付き設定の共通パターン**であり、
  §30.6・§34 の設定消費者（audio / gameplay スライダー）も同じ経路で登録されている可能性が高い。

### 43.3 Android 移植向け確定値

1. `dt_game = dt_real * fGlobalTimeMultiplier`（既定 1.0）。移植では `Settings` の 1 項目
   （globalTimeMultiplier, 既定 1.0）を持ち、Timer::Update の §42.3.4 と同じ位置で乗算すれば等価。
2. ゲーム内カレンダーは月日数表（31,28,31,30,31,30,31,31,30,31,30,31）と月頭通算日表
   （31,59,90,...）で日付繰り上がりを行う。wait / rest の経過計算はこの表を用いる。
3. GMST 名は `"<設定名>:<セクション名>"` 形式（例 `fGlobalTimeMultiplier:General`）で
   文字列から引かれる。移植で設定名を扱う場合はこの命名規則に合わせると対応が取りやすい。

### 43.4 §42.6 の残 2 点の解消

**1. 0x00B33E88 の 2 dword ペア = RTTI（型記述子）レコード**
- +0x00 = 型名文字列ポインタ **0x00A3D0E0 = "BSTempNode"**（0x00A3D0D8 は "BSTempNodeManager"）
- +0x04 = 基底型記述子ポインタ **0x00B3FAB0**（参照 45 件 = 継承チェーンの根として多数の型判定から触られる）
- 登録は静的初期化 FUN_009DC280（§42.3.2）。判定は 0x0047CD60 の IsKindOf ループ（vtable[1] で記述子を取得し
  `+0x04` を辿って基底へ）。**移植では RTTI を持たないため実装不要**（クラス判定は C++ の型／enum で代替）。
- 訂正: §42.3.2 で「型名テーブル断片 "c Effect\0Game Setting\0..."」としていたのは誤り。pefile 実測では
  0x00A3D0E0 = "BSTempNode" です（§42.3.2 は本節で修正済み）。

**2. 0x00B33E98（Timer +0x08）の書き込み元**
- .text 内の絶対参照は **0x009DC2B2 の 1 件のみ**（Timer::Reset の `FST [0x00b33e98]`）。読み出しも 0 件。
- → **フレーム補正用の一時値で、初期化以外に使われない実質未使用メンバ**。移植では保持不要。
- 参考: 0x00B33E90（freeze フラグ）は 9 件（0x0040616A / 0x004061AB / 0x0040D8A7 / 0x0040EACE / 0x00494A14 /
  0x00494E5A / 0x004E15D0 / 0x004E18C2 / 0x009DC2B8）で、いずれも Timer 更新／Reset。

**3. 補足（§42.4 の AV フォールバック補強）**
- **0x00403BE0** は「AV ポインタが null なら 0x00B35464 を 0 クリアして そのアドレスを返す」getter:
```
00403be0  8b c1        MOV EAX,ECX
00403be2  85 c0        TEST EAX,EAX
00403be4  75 0b        JNZ  0x00403bf1        ; 非 null はそのまま返す
00403be6  89 0d 64 54 b3 00  MOV [0x00b35464],ECX   ; null なら fallback を 0 に
00403bec  b8 64 54 b3 00     MOV EAX,0x00b35464
00403bf1  c3           RET
```
- 併せて 0x00403BC0 は `p = [ECX+4]; if (p && p[0]=='S') FUN_00401f20(p)` の解放ヘルパ（'S' は
  型名／タグ判定。`[this+4]` は RTTI 記述子と同じ配置規則）。
- 移植での結論は §42.4 と同じ: **未定義 AV は 0.0f を返すだけで等価**。
