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

### 13.3 未確認 / 次の候補

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
