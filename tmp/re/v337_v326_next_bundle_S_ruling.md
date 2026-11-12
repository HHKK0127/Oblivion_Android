# v337 — v326由来 次束S（6残件）静的裁定

## 1. 範囲・制約

- 参照元: `tmp/re/v326_v322_residuals.md`、`docs/RE_EXE_REPORT.md`、既存 `tmp/re` 成果物、v326 中間出力（`%TEMP%\opencode\v326_*`）。
- 対象 EXE: Oblivion Steam v1.2.0416 相当。EXE は既存 probe の読み取り結果のみを再利用し、原本を再保存・改変していない。
- オリジナル EXE/ESM/.ess 検体/解析原本/移植側ソースは変更していない。実 `.ess` の動的突合は実施していない。
- `0x4C0000-0x4E0000` は #106 専有のため、新規の深追いを行わず、v326 既存の浅層照会だけを採用した。
- 判定語は `[確定]` / `[部分確定]` / `[未確定]` / `[確定負]` を分離する。命令から直接読める契約と、C++正式名・実行時値の推論を混同しない。

## 2. 結論一覧

| # | 対象 | 本束判定 | 裁定 |
|---:|---|---|---|
| 1 | `0x5439CE/0x5439E7` bit10対、`fn 0x5437C0`、`caller 0x4633AE`、`[SaveGame+0x7C] < 0x69` | **[部分確定]** | bit10 (`0x400`) の Set/Clear 対と唯一 caller・ゲート条件は確定。`+0x7C` の正式フィールド意味、対が表すゲーム状態は未確定。 |
| 2 | dry bit9 の静的範囲 | **[部分確定]**（副次 **[確定負]**） | SaveGame の `[obj+0x18]` に対する bit9 即値 RMW は確認できず、SEH/x87 sentinel 等の誤候補は棄却。実行時 setter/ロード復元の発火は #82 委任で未決着。 |
| 3 | `[0xB13974]/[0xB1397A]` メニュー名表の構築経路 | **[部分確定]** | BSS のポインタ配列＋件数、`0x589B70` の取得、`0x65C620` linked-list追加 helper、登録窓までは接続。全59名の最終登録順・件数書手・実行時文字列供給元は未閉合。 |
| 4 | `0x6FF9C0/0x6FFAC0` 4文字名→TD辞書 | **[部分確定]**（TD同定は **[確定]**） | `BSX`→`BSXFlags`、`REF`→`TESObjectREFR` の TD/vtable 接続は確定。ロック下の辞書ノード構造・ハッシュ/チェーン・登録書手は未確定。 |
| 5 | `0x5D8FD0` の `+0x24`、`[0xB37FE0]`、`[0xB3B734]` | **[部分確定]** | `+0x1C` 文字列または `+0x24` vtable[0]値を比較し、設定方向とスケールを適用する3-way comparatorであることは確定級。正式サブ型・設定名・スケーラの意味は未確定。 |
| 6 | x87除算例外ハンドラ群の一般棄却リスト | **[部分確定]**（bit9誤候補の棄却は **[確定負]**） | `0x9847D6`/`0x9997F0` 等は SaveGame flag setter ではなく、SEH/除算例外復帰骨格の sentinel 書込。今後は handler/SEH 文脈・`[esi+0x0C]`補正・例外後処理を満たす候補を一般棄却する。ただし全 x87 handler の網羅リストではない。 |

集計（主判定）: `[確定]` 0 / `[部分確定]` 6 / `[未確定]` 0 / `[確定負]` 0。
副次判定として、#1 bit10対の命令契約、#2 bit9直接setter不在、#4 TD、#6 sentinel棄却はそれぞれ該当範囲で確定または確定負。

## 3. 項目1 — bit10 Set/Clear 対と `SaveGame+0x7C`

### 3.1 観測事実

v326 の全 `.text` census と個別窓で、次の対を再確認した。

```text
0x5439CE: or  dword ptr [ecx+0x18], 0x400
0x5439E7: and dword ptr [eax+0x18], 0xfffffbff
```

- `0x400` は dword の bit10。従って命令レベルでは `SetBit(10)` / `ClearBit(10)` の対である。
- 対を含む関数 head は `0x5437C0`。既存の境界検証でこの head と caller 関係を採用している。
- caller は `0x4633AE` の1件。対の前段に `0x54398E` の `cmp byte ptr [ecx+0x7C], 0x69` と `jb` があり、静的条件は `[SaveGame+0x7C] < 0x69`。
- `ecx/eax` は既存 v326 の文脈で `[0xB33B00]`（SaveGame）に接続される。ただし、関数途中のレジスタ再束縛を越えて `+0x18` の正式構造体型を命名する根拠はない。

### 3.2 裁定

- **[確定]**: `0x5439CE/0x5439E7` は bit10 (`0x400`) の Set/Clear 対。
- **[確定]**: caller とゲート条件は `0x4633AE`、`[SaveGame+0x7C] < 0x69`。
- **[未確定]**: `SaveGame+0x7C` が save version、format generation、flags byte、または別の状態値のどれか。閾値 `0x69` だけから version と断定してはならない。
- **[未確定]**: bit10 のゲーム上の意味。bit9 dry と隣接しているが、隣接性だけで dry/dirty、ロード可否、圧縮形式等の同一概念を導けない。

したがって項目全体は **[部分確定]**。`+0x7C` の実値を持つ save/load 実行時観測または、同じ byte の読み書き全 census が次の必要証拠となる。

### 3.3 既存成果物との照合

- `v326_v322_residuals.md` §3-1/§5 R1 の新規発見と一致する。
- v326 の bit9 census はこの対を bit9 setter と誤認せず、bit10 (`0x400`) として分離している。
- `docs/RE_EXE_REPORT.md` はメニューXMLパーサー中心で、SaveGame flag の正式名を記載していない。直接矛盾はない。

## 4. 項目2 — dry bit9 の静的範囲と #82 委任

### 4.1 静的に確定できる負証拠

v326 の粒度別全数走査（`v326_1_bit9`、`v326_07`、`v326_09`）は、`[obj+0x18]` に対する次の候補を分離している。

1. dword 定数 RMW (`or/and/xor [obj+0x18], imm`) で bit9 (`0x200`) を立てる SaveGame site は **0件**。
2. `0x9847D6` と `0x9997F0` の `mov [esi+0x18], 0x200` は、後述の x87/SEH 骨格に属し SaveGame flag 書込ではない。
3. word `or/and` の `0x200` 候補（例 `0x5533A0/0x55EB19`）は handle/Win32 窓であり、SaveGame object ではない。
4. レジスタマスク RMW 116 site のうち、SaveGame 窓で `edi/esi==0x200` を静的に示す site はない。`0x465AE4` は直前 `mov edi,1`、bit10 対は即値 `0x400` である。
5. `[0xB33B00]` への dword 即値直接書込 census は0件。new/load の間接経路を即値 setter と混同しない。

以上から、**[確定負]**「既知の直接即値 RMW site が dry bit9 を立てる」という説は棄却できる。

### 4.2 残る静的・動的範囲

- `[+0x18]` 読出側には `shr 9/test` があり、v316/v326 の `0x464E63`、`0x465159` などで dry bit9 が消費される。
- v326 の loader/read census には `0x465860` 列のロード復元候補が残るが、書込値を実行時の object/stream まで閉じていない。
- setter が this/引数経由で8命令を越えてマスクを形成する可能性、new/load の一括復元、実行時の object lifetime は本束の静的証拠だけでは決着しない。

従って項目全体は **[部分確定]**。**動的決着は #82 委任**とし、本束では bit9 を「未使用」とは記録しない。

## 5. 項目3 — メニュー名テーブルの実行時構築経路

### 5.1 構造と取得

- `[0xB13974]` は BSS 上のテーブル先頭ポインタ、`[0xB1397A]` は件数の実行時領域。ファイル内の固定文字列配列ではない。
- `0x589B70(id)` は範囲確認後、`[[0xB13974] + (id-0x3E9)*4]` を返す。ID空間は `0x3E9..0x41B`、上限ガードは `<0x33` と件数比較。
- `0x58A567` は `push 0x3F5; call 0x589B70`、`0x58A688` は `push 0x40F; call 0x589B70`。`0x3F5=PauseMenu`、`0x40F=SaveMenu` は docs §3.1 の59メニュー表と一致する。
- ID `0`/`3` はこの名前テーブル空間外であり、既定値またはスナップイン未設定を示す可能性はあるが、表示名なしの意味を確定しない。

### 5.2 構築経路の静的接続

v326 の `v326_08`/`v326_03` で、次を構築経路の入口候補として確認した。

- `0x65C620`: linked-list への追加 helper として使われる窓。
- `0x9FBA90`、`0xA24950`: 登録窓候補。名文字列 VA 群（`0xA69xxx`）と実行時登録の接続点として記録されている。
- `0xB13974`/`0xB1397A` 自体は BSS のため、静的初期値から59名の完成配列を読めない。

### 5.3 裁定と残点

- **[部分確定]**: 「固定 `.rdata` 名簿を直接参照」ではなく、実行時に登録されるポインタ配列＋件数である。
- **[部分確定]**: 登録処理の候補は `0x65C620` と `0x9FBA90/0xA24950` の系列まで狭まる。
- **[未確定]**: 59件それぞれの登録順、`[0xB1397A]` の唯一の書手、重複時の挿入/置換規則、文字列ポインタの最終所有者。

`docs/RE_EXE_REPORT.md` §3.1 の「59メニュー」「0x3E9〜0x41B」とは整合する。同docsは `FUN_0058a1c0` の固定トークン登録を説明するが、今回の `[0xB13974]` 動的名表とは別テーブルであり、矛盾ではない。

## 6. 項目4 — 4文字名から RTTI TypeDescriptor への解決

### 6.1 確定した型接続

既存 v326 のキー検索窓と RTTI 走査を組み合わせると次の対応になる。

| キー | TD文字列 | TD頭 | vtable |
|---|---|---:|---:|
| `BSX` | `.?AVBSXFlags@@` | `0xB253D4` | `0xAA3E80` |
| `REF` | `.?AVTESObjectREFR@@` | `0xB03054` | 既存 vtable/RTTI接続 |

- `BSX` 側の使用結果 `[+0xC] >> 4 & 1` は `BSXFlags` の bit4 読出しと整合する。
- `REF` 側の `[ebx+4]` `InterlockedDecrement` は `TESObjectREFR` の参照カウント操作と整合する。
- `0x6FF9C0`/`0x6FFAC0` の内部に vtableを即値 storeする形はなく、呼出先が生成した/解決した型を使用する構造である。

### 6.2 未確定の辞書内部

- 4文字キーの比較方法（固定4-byte比較、文字列比較、ハッシュ）が辞書内部でどこにあるか未閉合。
- `0xA2806C` 経由のロック/同期窓は確認されているが、バケット数、node stride、next pointer、登録 caller の全数は未取得。
- `BSX`/`REF` の2例から、全4文字キーが同一辞書形式・同一TD用途だと一般化してはならない。

よって型同定は **[確定]**、依頼された「辞書内部構造」は **[未確定]**。項目全体は **[部分確定]**。

## 7. 項目5 — `0x5D8FD0` 比較器と未命名フィールド

### 7.1 命令契約

`0x5D8FD0`（head `0x5D8F00` の callee、別入口 `0x5D91D0` からも参照）の命令列は次の契約を示す。

1. `[0xB3B734]` を読み、bit7から比較方向符号を作る。`b734 & 0x7F` が比較モード。
2. mode 0: 各対象の `[+0x1C]` を文字列として比較する。NULL は `0xA2F7EC` の空文字列へ置換。
3. mode 1/2: `[+0x24]` のサブオブジェクトから vtable[0] を呼び、`[0xB333C4]` 経由の単体/比較引数を渡す。戻り値を比較する。
4. mode 1側には `fmul dword ptr [0xB37FE0]` があり、スケール後に `0x9828C0` 等を通じて比較する。mode 2側は同じ比較系列でもスケーラ適用位置が異なる。
5. `cmp` から -1/0/+1 の3値を作り、方向符号を掛けて返す。

### 7.2 裁定

- **[確定]**: これは UI/一覧レコード等を並べ替える3-way comparatorで、単純な真偽 predicate ではない。
- **[部分確定]**: `+0x1C` は名前文字列欄、`+0x24` は数値比較用サブオブジェクト入口、`[0xB37FE0]` は mode 1 のスケーラ候補、`[0xB3B734]` は方向・モード設定である。
- **[未確定]**: 正式なレコード型名、`+0x24` のサブ型/virtual method名、`B37FE0` の単位と設定名、`B3B734` の設定キー名。

既存 v326 §3-5 の「契約は確定級、正式名は未確定」という裁定を維持する。`docs/RE_EXE_REPORT.md` のメニューXML属性リストにはこの Save/Load 一覧 comparator の正式型情報はなく、直接矛盾はない。

## 8. 項目6 — x87/除算例外ハンドラ群の一般棄却リスト

### 8.1 実例と棄却理由

bit9 census の誤候補として次を記録する。

- `0x9847D6`: `mov dword ptr [esi+0x18], 0x200`。直前に `[esi+0x0C]` の補正、`test al,1`、`test al,8`、`test ax,0x400` があり、直後に `0x98ED40` へ引数を積む。SaveGame global `[0xB33B00]` に結びつかず、SEH/例外処理骨格の sentinel/状態初期化として棄却する。
- `0x9997F0` 系: `0x999594`、`0x988666` 等を呼び、`[esi+0x0C]` の bit補正と例外復帰・除算結果処理が連続する。`[esi+0x18]=0x200` は同様に一時/例外状態であり、dry setter として棄却する。
- 近傍の `0x9840E0`、`0x98540B`、`0x98C5A0`、`0x98ED40`、`0x9932CD` 系は `_except_handler3` / `__CxxFrameHandler3` のスケルトンまたは除算例外 sentinel 文脈として扱う。個別 site は handler entry・SEH chain・例外後処理の有無を確認するまで SaveGame候補に昇格させない。

### 8.2 一般棄却規則

今後の bit9 走査では、次を満たす site を SaveGame bit9 setter から棄却する。

1. `[obj+0x18]` の base が `[0xB33B00]` または SaveGame由来の引数/this と接続しない。
2. 同じ関数内に x87 (`fdiv/fidiv/fnstsw`)・除算後の `jns/test al`・`[obj+0x0C]` の符号/bit補正がある。
3. `__except_handler3` / `__CxxFrameHandler3` 相当の SEH prologue、例外 chain、handler tail が確認できる。
4. `0x200` が sentinel の直接初期化で、後段が例外復帰/CRT handler である。
5. `0xB33B00` direct write census、SaveGame caller接続、dry bit9 readerとのデータフローのいずれもない。

この規則により `0x9847D6/0x9997F0` の bit9候補は **[確定負]**。ただし、これを「全 x87 handlerの完全網羅」とは呼ばない。未走査の handler・別コンパイラ骨格・間接 setter は残る。

## 9. 既存成果物・docsとの照合

| 参照 | 照合結果 |
|---|---|
| `v326_v322_residuals.md` | R1〜R6の6残件をすべて取り上げた。bit10対、bit9静的負証拠、BSS名表、TD、比較器、SEH棄却の既知証拠を継承し、未確定部分を維持した。 |
| `v325_v318_residuals.md` | `+0x24`/設定値などは「正式名未確定」とする既存方針に一致。SaveGame/ChangeFlags と別構造体を混同しない。 |
| `v335_v317_stage2_static.md` | `0x4C0000-0x4E0000` 制約を維持。#106専有帯の深追いはしていない。 |
| `v336_next_bundle_U_ruling.md` | 過去の stack offset 訂正方針を尊重した。今回6件の直接証拠に影響する不整合はない。 |
| `docs/RE_EXE_REPORT.md` §3.1 | 59メニュー、ID `0x3E9..0x41B`、`PauseMenu=0x3F5`、`SaveMenu=0x40F` が一致。docsは動的 BSS名表の登録経路を記載していないが、直接矛盾ではない。 |

## 10. 残る不確定点と次の証拠

1. `SaveGame+0x7C` の同一 byte 全読み書き census、実 save version/format 値との照合。ただし実 `.ess` が必要な結論は動的 lane に留保する。
2. #82 の実行時 laneで、dry bit9 setterまたは load/new 一括復元が `[+0x18]` を変更する瞬間を観測する。
3. `0xB1397A` の書手全数と `0x65C620` の caller/登録 node 構造を、#106専有帯を越えない範囲で追跡する。
4. `0x6FF9C0/0x6FFAC0` のロック範囲内で4文字キー比較、bucket/chain、TD pointerの実際の格納位置を閉じる。
5. `0x5D8FD0` の対象 caller と vtable[0] 実装を対応付け、`B37FE0/B3B734` の setting name を文字列/INI登録表から探す。
6. x87 handler census を全 `.text` に広げる場合も、SaveGame data-flow と handler骨格の二重条件で候補を分類する。

## 11. 再現・証拠ファイル

- `%TEMP%\opencode\v326_1_bit9.py/.out`: bit9 全体 census。
- `%TEMP%\opencode\v326_03.py/.out`: RTTI、`0x5D8FD0` 周辺、bit定数、ロード読み手。
- `%TEMP%\opencode\v326_08.py/.out`: bit10対、RTTIキー窓、BSS名表、getter等。
- `%TEMP%\opencode\v326_09.py/.out`: bit9再走査、`0x5439CE/0x5439E7` の命令列。
- `%TEMP%\opencode\v326_all.py/.out`: `0x5D8FD0` 本体、caller、横断照合。

本ファイルのみ新規作成。オリジナル検体、解析原本、ESM/.ess、移植側ソースは無変更。
