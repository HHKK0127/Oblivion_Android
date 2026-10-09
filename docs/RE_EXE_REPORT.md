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

**Toggles の CPU 側書き込み（追加解析・部分確定）**
- `FUN_0080d110` は Toggles の x / y / z / w を `piVar2[0x56]`〜`piVar2[0x59]`（描画パスオブジェクトの +0x158〜+0x164）からコピーする（`DAT_00b46688`〜`DAT_00b46694`、または別系統の `DAT_00b4616c`〜`DAT_00b46178`）
- `piVar2` は引数 `param_5 + 0x18` から得る。その `piVar2[7]` のビット 0x400 / 0x800 / 0x1000 は別途読まれるが、各ビットが何の機能かは未確認
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

### 13.2 未確認 / 次の候補

- lip の形式（口形キーフレーム）の解析と、Android 側のリップシンク実装の要否。
- DLC Shivering Isles - Voices.bsa（452 MiB）の索引件数確認。
- mp3 と INFO レコードの対応件数の全数照合（`verify_voice_naming.py` の拡張）。
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
- DLC の esp（ShiveringIsles 等）は未解析。
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
- EFSH DATA のフラグ・ブレンド値は未デコード。
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
- 未確認: 色のバイト順の実機での見た目確認、Blend 列挙値の描画上の意味（ゲーム内の見た目との突き合わせ）。
- 96 バイト 2 件は別レイアウトの可能性があり、今回は未分析。
## 16. ESM world icon extraction (ICON = Clutter\...)

- Source: ICON subrecords in Oblivion.esm, 366 unique `Clutter\` paths (of 1419 total ICON paths).
- Lookup: `textures\menus\icons\clutter\<name>` in base BSAs, falling back to `textures\menus80\icons\clutter\<name>` (used by DLCShiveringIsles - Textures.bsa).
- Result: 366 / 366 located. 306 from Oblivion - Textures - Compressed.bsa, 60 from DLCShiveringIsles - Textures.bsa. 0 decode failures.
- Output: `tmp\world_icons\` (366 PNG, RGBA, from DDS via dds_decode).
- Script: `tmp\extract_world_icons_v2.py`.
- Note: the 60 DLC-only icons are present only when the Shivering Isles DLC is installed.

> Addendum (2026-10-08): Headless Ghidra (project program Oblivion.exe) found zero direct references to FUN_0080d110. Its caller is likely an indirect call (vtable or function pointer), so the upstream writer of piVar2[0x56..0x59] is still unidentified.
