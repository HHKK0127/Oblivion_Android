# v338+ — #93 / T / U / S 残件統合裁定

- 作成: 2026-11-09（L1・静的解析のみ）
- 対象: `v323_v319_residuals_ruling.md` §5、`v334_ess_next_bundle_T_ruling.md` §6、`v336_next_bundle_U_ruling.md`、`v337_v326_next_bundle_S_ruling.md` の未解決点
- 対象 EXE: Oblivion Steam v1.2.0416、ImageBase `0x400000`
- 制約: オリジナル EXE/ESM/.ess/解析原本/移植側ソースは読み取りのみ。実 `.ess` の追加取得、実機、#82 人間ゲートは実施していない。
- 既存成果物は重複再調査せず、残件に直接関係する既存 probe 出力と今回の新規読み取り専用 probe `%TEMP%\opencode\v338_probe.py/.out` のみを使用した。
- 判定は `[確定]` / `[部分確定]` / `[未確定]` / `[確定負]` を分離する。正式な C++ シンボル名が得られない場合、機能記述を正式名として扱わない。

## 1. 判定集計

### 1.1 主判定

| lane | [確定] | [部分確定] | [未確定] | [確定負] |
|---|---:|---:|---:|---:|
| #93 / v323 §5 | 0 | 4 | 5 | 0 |
| T / ESS | 2 | 3 | 4 | 0 |
| U / v336 | 1 | 2 | 0 | 0 |
| S / v337 | 1 | 4 | 0 | 0 |
| 合計 | **4** | **13** | **9** | **0** |

### 1.2 主判定とは別の負証拠

- bit7 の `test/cmp/bt` 型リーダは、既存 v323 の検索範囲内で `[確定負]`（SIB形、10命令超、レジスタ離脱型は対象外）。
- dry bit9 の既知 SaveGame 即値 RMW setter は、既存 v326 census の範囲で `[確定負]`。#82 の実行時 setter/復元判定は未実施。
- `0x452DF0` について「tail==0 と pass2 が別 callee/別 force」という説は `[確定負]`。

## 2. 最優先 #93 — v323 §5 残件

### 2.1 FUN_00463700 正式名 — **[未確定]**

- `0x463700` は v323 の既存観測どおり、`0x464440` 内から2回呼ばれるグローバルクリア連鎖である。
- 文字列参照は0件。末尾 `jmp 0x4F9DD0` の実体は v319 許可帯外であり、今回も深追いしていない。
- よって機能上の暫定記述（world/reset 系 helper）は維持するが、`FUN_00463700` の正式名は得られない。

### 2.2 `0x44F058` 上位 — **[未確定]**

- 既存浅層では、`0x44F058` は `0x44EE00` を呼び、`[0xB33AC4]` と `0x4D6760` の判定後に結果を受け取る。
- `0x44EE00..0x44F058` の深い包含 head/クラス名は #89 専有帯であるため照会しない。
- `find_head` の `0x44EDE0` は CC ラン不足型の誤答候補で、正式な包含 head として採用しない。

### 2.3 `0x5BA629` 上位 — **[未確定]**

- `find_head=0x5BA4D0` と callers の存在は既存成果物で確認済みだが、帯外のため本体・上位文脈は照会しない。

### 2.4 `0x659BE0` 本体 — **[未確定]**

- `callers=0x5E7D0B, 0x60831A, 0x69F745` は確認済み。
- 本体は v323 の許可帯外であり、`Actor +0x4C` の基礎値を正式に命名する証拠はない。

### 2.5 bit7 SIB と v316 530件対応 — **[部分確定]**（限定負は [確定負]）

- v323 の全 `.text` pattern census では、通常の `[reg+0x18]` に対する bit7 `test/cmp/bt` リーダは0件。既知の書込/復元は `0x4639A9` と `0x4CDA79` 等。
- `0x4CDA74: or [eax+0x18],edi` / `0x4CDA79: and [eax+0x18],0xffffff7f` は、v316 530件カタログの「bit7を含み得る computed-mask RMW」として対応するが、`edi` の値と各 bit0/6/9/12/18 の意味までは閉じない。
- SIB encoded operand、読み込みと判定が10命令を超えるもの、別レジスタへ離脱するものは今回の負証拠の対象外。
- 結論: 「通常形の bit7 reader 不在」は `[確定負]`、530件全体との意味対応は `[部分確定]`。

### 2.6 `FUN_00462080` mode — **[部分確定]**

- `0x462080` は `Save Game Header`、`TES Class`、`Process Lists Class`、`Spectator Events`、`Sky/Weather`、`Quick Keys`、save buffer エラーを扱うセクション/バッファ処理関数である。
- caller は起動 `0x40F425`、world 初期化 `0x4644AA`、LoadGame `0x465B12` の3系統。
- `0x4627E1` で `FUN_0045DE30`（created base objects の find-or-add）を呼び、その直後に `shr ecx,9; test cl,1` があるため、保存系 dry bit9 の消費文脈は確定級。
- ただし LoadGame caller で同じ関数の write/save 分岐が実際に走るか、`[edi+8]` 間接 call の read/write mode は未閉合。mode の正式値は未確定。

### 2.7 `0x465C94` arg / reader — **[部分確定]**

- `esi=[ebp+0x40]`、`[esi+4]` が12B reader の間接 entry、`0x465C94` の call は size `0xC`。
- `L+0x1C` は事前に1を書き、`L+0x20` 側の出力を `edi` として formID 解決へ渡す。解決後、frame の先頭へ form pointer を上書きする。
- 未確定なのは `[ebp+0x40]` の正式クラス、reader の正式名、5 push と `add esp,0x18` の1 slot差、`L+0x1C/L+0x20` の正式フィールド名。

### 2.8 `0x464910` 正式名/上位 — **[未確定]**

- `0x464910..0x464AAA` は `0x464903` の CC pad 後に始まる別関数で、`0x46494A` の LoadForm 呼出を含む。
- caller は `0x4CDA22` ただ1件。#91 専有帯のため上位の深解析はしない。
- 「bit18 gate 付き変更適用系」は機能上の暫定分類に留め、正式名には昇格させない。

### 2.9 正式名群

| 対象 | 判定 | 静的に閉じた意味 | 正式名 |
|---|---|---|---|
| `0x45BB30` | [部分確定] | `TESObjectREFR` の配置/3D更新列から呼ばれる座標・状態更新 helper | 未取得。文字列/RTTI/slot根拠なし |
| `0x4DD4B0` | [部分確定] | reference の親/3D/Havok・cell/worldspace 接続と状態更新を行う本体 | 未取得。窓文字列だけでは正式名にならない |
| `0x4D8A10` | [部分確定] | floatを `this+0x28` に保存し、vtable `+0x40` を force=4 で dispatch | setter/dispatch 機能は確定、クラス/メソッド名は未取得 |
| `0x4D8A30` | [部分確定] | 3つの float を `this+0x2C/+0x30/+0x34` に保存し、同じ vtable `+0x40` を dispatch | vector/coordinate setter相当だが正式名は未取得 |
| `0x5F0310` | [部分確定] | `+0x3C` の3D所持を前提に、bit6、ActiveEffect/ReanimateEffect、script vfunc `+0x2E4` の状態を判定 | `IsRuntimeChangeTarget` は非正典の暫定呼称。正式名未確定 |

`0x4D8A10/0x4D8A30` は `0x45BB30`/`0x4DD4B0` の caller 列で同一の reference/位置更新系に現れるため機能分類は強まったが、これを Bethesda の公式メソッド名へ翻訳する根拠はない。

## 3. T — ESS 動的・帯外残件

### 3.1 trailer 先頭6B — **[確定]**（v334 item1 を更新）

既存検体 `@822,976` の `02 00 00 00 00 00` について、今回 `0x679480/0x679630` の関数本体を再確認した。

1. `0x45FB50` は `0x679480` を呼ぶ。
2. `0x679480` は `mov [esp+0x18],2` で開始し、`+0x40` と `+0x48` の二つの linked list の各要素について文字列長+1を加える**バイトサイズ**を返す。したがって空リスト時の返り値は2。
3. `0x45FB7B` はその値を callback に size=4 で渡す。検体の先頭4B `02 00 00 00` はこの size 値に一致する。
4. `0x45FBE6` は `0x679630` を呼ぶ。`0x679630` は内部件数を0で初期化し、該当要素を処理するごとに1加算し、最後に `mov word [out],dx` で**u16件数**を書き出す。空リスト時は0。
5. よって続く2B `00 00` は `45FB50` 側の `679630` が出力する u16件数であり、境界パディング/別 writer の先頭ではない。

判定は `[確定]`。ただしその後の `45E220` が始める u32 `3437` の WorldSpace ID Array とは別 writer である。

### 3.2 `dir[1]=5527` の count/formID — **[部分確定]**

- writer `0x4622D2` は `[0xB333A0]+0x74` 配列の `+0xC` を出力し、検体値5527は change record 数と一致する。
- reader `0x462B20` は同じ u32 を byte3再マップして GetForm に流す。
- 「countをformIDとして再利用する構造」か「別表の偶然の同値」かは、save#2/複数セル検体なしでは決められない。

### 3.3 save#2 `+0x94` — **[未確定]**

`0x464B7F` の `[this+0xA4]==0` 時だけ `GetSystemTime` から `+0x94` へ書くことは確定しているが、2回目保存で凍結されるかは save#2 が必要。

### 3.4 `B333A0` 全体身元 — **[部分確定]**

`0x45E220` の第2配列が `WorldSpace ID Array` と対応し、`[0xB333A0]` がその供給側に現れることは確定級。ただし `[0xB333A0]` の正式な全体型、GLOB/GMST表の正式命名は静的証拠だけでは未確定。

### 3.5 `dir[0]` 書込元 — **[未確定]**

`0x462290` が `[[0xB33A98]+0x8C0]` を dir[0] として書き、値822982が index 開始と一致することは確定。`+0x8C0` を生成する上流の因果は未追跡。

### 3.5a Δの7×u32要素名 / Player特殊枠 — **[部分確定]**

pass2 の created 系 28B/44B/36B 読取、`>>12` のグリッド変換、cell/array 解決、tail再発行時の除去は確定。7×u32 の先頭側に位置 `(x,y,z)` が含まれることも命令列と変換から支持される。一方、残り4要素の正式名と `0xFEFFFFFF` Player特殊枠の意味は未確定。

### 3.6 `tail==0` の帰結 — **[未確定]**

探索失敗時に `[ebx+0x14]=0` となること、flags=8単独が検体に187件残ることは確定。しかし entry が削除、無効化、再発行のどれになるかは該当動的サンプルが必要。

### 3.7 thumbnail RGB/BGR — **[未確定]**

`256×192×3` の24bpp相当サイズは確定。保存側の `×3B` コピーだけでは RGB/BGR は決まらず、実機色比較または D3D surface format の追加証拠が必要。

### 3.8 `Δ0x1C` load側対応 — **[未確定]**

writer側の28B created位置/余剰ブロックと再発行時除去は確定。load側36B読取への包含、created flags/type別の別経路は追加検体なしに決められない。

### 3.9 P5 size返却先 — **[確定]**

既存 v328_19 の独立 esp 追跡どおり、size read の destination と `0x45DE08` の返却参照は同一 slot（`E0+0x14`）。旧「sizeが返らない」記載は採用しない。

## 4. U — v336 残件

### 4.1 `0x4CBE50` 正式名/bit意味 — **[部分確定]**

`[edi+0x24]` bit0は purge 連鎖へのゲート、bit3は bit0成立時の継続ゲート。bit8は許可浅層範囲に証拠なし。`PurgeSavedDataForCell` は `0x459F30` 側の文字列であり、`0x4CBE50` の正式名として転記しない。

### 4.2 上位呼び手 — **[部分確定]**

7 call site は `0x4B86C0`（検査系）、`0x4F2270`（遅延初期化単体）、`0x515D20/0x515EF0`（SEH付き参照列挙系）へ束ねられる。#106専有帯のため正式クラス/メソッド名は未確定。

### 4.3 `452DF0` 呼出規約 — **[確定]**

今回全14 caller を確認した。tail==0 `0x466848` は `push key; push 1; call 0x452DF0`、pass2 `0x461CD7` も `push key; push 1; call 0x452DF0`。callee の `ecx` は対象 owner/this、stack 引数は key と force=1。物理 slot の caller 内オフセットは異なるが、論理呼出規約は同一である。

## 5. S — v337 残件

### 5.1 bit10 と `SaveGame+0x7C` — **[部分確定]**

- `0x5439CE: or [ecx+0x18],0x400` と `0x5439E7: and [eax+0x18],0xfffffbff` は bit10 set/clear 対である `[確定]`。
- 今回の byte census では `SaveGame+0x7C` に対し、`0x45FC6D`/`0x45FCA9` の `0x25/0x27`、`0x45FD1E`/`0x45FD56` の `0x5E`、`0x464AF2`/`0x4669A2` の `0x7D`、`0x54398E` の `0x69` など、単調な format-version gate と current value `0x7D` の読み書きが確認できる。
- よって `+0x7C` は機能上「save format version byte」とみなせるところまで `[部分確定]`。EXE内の公式フィールド名と、bit10が dirty/dry・load可否・圧縮形式のどれかは未確定。

### 5.2 B13974/B1397A 登録 — **[部分確定]**

`[0xB13974]` は動的ポインタ配列、`[0xB1397A]` は動的件数で、`0x589B70` が ID `0x3E9..0x41B` を添字化する。`0x65C620` と `0x9FBA90/0xA24950` 系が登録候補だが、唯一の件数書手、全59件の順序、実行時文字列供給元は未閉合。

### 5.3 6FF9辞書内部 — **[部分確定]**

`0x6FF9C0`/`0x6FFAC0` 本体は、ロック `0xA2806C`/unlock `0xA28074` の範囲で `[this+0x10]` の pointer array を二分探索し、`0x452A60` で得た文字列を byte単位で比較する。`+0x14` は要素数、比較は終端NULまで行い、一致時は配列要素を返す。したがって少なくとも検索側は「ロック付き・ソート済み pointer array＋二分探索・文字列比較」である。

登録側の node stride、bucket/chain の有無、4文字キー辞書と TD pointer の所有関係は未確定。`BSX`→`BSXFlags`、`REF`→`TESObjectREFR` の型対応自体は既存 v337 の `[確定]` を維持する。

### 5.4 `0x5D8FD0` 正式意味 — **[部分確定]**

`[0xB3B734]&0x7F` の mode 0/1/2を分岐し、`+0x1C` の文字列または `+0x24` の virtual getter 値を比較し、mode 1では `[0xB37FE0]` を乗じ、方向 bitで -1/0/+1 を反転する三値 comparator であることは確定級。正式な record class、vtable slot名、`B37FE0/B3B734` の設定キー名は未確定。

## 6. 追加証拠が必要な項目

### 6.1 追加検体/実機なしでは閉じないもの

- 実 `.ess` save#2: T item2（count/formIDの関係）、item3（`+0x94`凍結）、item5（dir[0]上流）、item7（tail==0最終帰結）、item10（Δ0x1C load対応）。
- 実機色比較または D3D surface format: T item8 RGB/BGR。
- #82 実行時 lane: S dry bit9 setter/一括復元の発火、bit10のゲーム状態。
- #89/#91/#106専有帯解除: `0x44F058`、`0x5BA629`、`0x464910`、`0x4CBE50`上位、および正式 C++名。

### 6.2 今回の静的証拠で追加不要になったもの

- T item1 trailer先頭6Bの2B帰属。
- U `452DF0` の論理引数契約。
- S 6FF9C0/6FFAC0 の検索アルゴリズム（ただし登録側辞書構築は残る）。

## 7. 成果物・変更範囲

- 成果物: `tmp/re/v338_plus_residuals_ruling.md`
- 中間 probe: `%TEMP%\opencode\v338_probe.py` / `v338_probe.out`（リポジトリ外）
- probe selftest: 2回実行、両方 exit=0、MD5 `E14E6D8D49BBF6C7DB4A2CC5667342F4` で一致。
- オリジナル EXE/ESM/.ess、解析原本、移植側ソース、既存成果物、docs、GT は無変更。
- `STATE.md` は親セッション由来の既存変更を保持し、本サブタスクでは追記していない。
