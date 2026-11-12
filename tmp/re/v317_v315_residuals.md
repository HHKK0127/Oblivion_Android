# v317_v315_residuals.md — 次束G: v315 §3 新規残点 N1-N9 の EXE 静的検証（L1・日本語報告）

- 作成: 2026-11-07（ai-todo #81 / 次束G）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000）
  - セクション: .text VA 0x401000 → file 0x400 (0x626E00) / .rdata VA 0xA28000 → file 0x627200 (0xD9A00) / .data VA 0xB02000 → file 0x700C00 (0x30C00)。`F(va) = foff + (va - sva)`（v307_common.py、`pe.get_offset_from_rva(VA-0x400000)` と一致）。
- 調査項目の出典: `tmp/re/v315_v312_residuals.md` **§3 新規残点 N1-N9**（正式な調査項目＝これに従う）
- 突合先（重複調査禁止・既確定踏襲）: `tmp/re/v312_phase2_gate_verification.md` §1/§2（R1-R9 の既確定と §7 裁定）/ `tmp/re/v313_save_ess_boundary_followup.md` **§10 追補（v316 統合後の最新状態）** / `tmp/re/v316_v313_residuals.md`（N3 の仲裁素材）/ `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md`（5 段階判定・.ess 不在前提・解析制約）
- 判定: 5 段階 **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論は削除せず、矛盾時は両論併記＋裁定理由を §5 に記録。
- **実 `.ess` は不在（v313 §1.1 の8系統で確定・本件では再検索せず踏襲）** → 動的照合は全項目未実施（§7）。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コードは参照・流用なし（自ラボ過去 RE 記録との突合のみ）。
- 成果物: **本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は無編集。ESM 未接触・git 未実行・`tmp/re/re_util.py` 変更なし・上書き削除なし。
- 中間物: `%TEMP%\opencode\v317_01..12.py/_out`（24 ファイル・本タスクのみ新規）。

---

## 1. 解析対象と対象 VA 範囲

### 1.1 深解析した自帯（許可帯すべて）

| 帯 | 本件で深解析した主要 VA |
|---|---|
| `0x452000-0x467920`（pass2 0x452000-0x454000 / formatter 0x453A90-0x458E50 / caller1 0x45A000-0x45B300 / save 0x459000-0x460000 / dispatch 0x460000-0x464000 / SaveGame 0x464000-0x467920 を含む全域） | **0x452600/0x452D10/0x452D30/0x452230**（台帳ヘルパ）/ **0x4535A0..0x45379E**（flags ポストプロセッサ＝CheckFlags）/ formatter 全域（頭 0x453A90 から線形デコード）/ **0x459950..0x4599A8** / **0x459AF0..0x459D10** / **0x459F30..0x459F6E** / **0x45AD00..0x45ADB2** / **0x45ADC0..0x45B2FD**（caller1）/ **0x45B9A0..0x45B9F4** / **0x45BB00..0x45BB27** / **0x45BB30..0x45BC6D** / **0x45CC60..0x45CD9F+**（caller2）/ **0x461680..0x461E90**（pass2 全域）/ **0x463700..0x4639D0** / **0x465130..0x465856**（FUN_00465130 全文）/ 0x465D73-0x466357・0x4667D0-0x466870（LoadGame 帯窓）/ 0x466AD0-0x466B00（quicksave 頭） |
| `0x430000-0x431000` | 帯走査のみ（bit3/bit7 スキャン対象） |
| `0x4D8000-0x4F2000` | 帯走査（bit3/bit7/8bit sign 全走査）＋ 0x4DC22C 文脈（5F0310 caller） |
| `0x5E1000-0x5E2800` | 帯走査のみ |
| .rdata/.data 文字列帯 | quiet/label/RTTI 文字列読取（0xA391F8 等・CHANGEFLAG/CHANGE_ 帯） |

### 1.2 帯外参照（N5 指定および最小限・浅層のみ）

- **0x5F0310..0x5F03FE（N5 項目指定による本体全文読取）** — v315 の帯注記「先頭のみ帯外読取・深部未走査」の残点そのものであるため、N5 の調査対象として本体を読んだ（他への波及なし）。
- 0x625D30（4535A0 の callee getter、3 行のみ）/ 0x40A9A5（caller2 の唯一呼び手、文脈 20 行未満）/ 0x4CBECC（459AF0 呼び手の手元、文脈のみ）/ 0x672F3C（quicksave 呼び手、ecx 設定確認のみ）/ 0x65B813（5F0310 caller、文脈のみ）。
- .text 全体 raw E8/0F8x バイトスキャン（F6: 0x463922 へのジャンプ xref のみ。VA+命令種のみ記録）。

### 1.3 帯外へ**一切**深解析しなかった範囲

v314 禁止帯（0x404000-0x406000 / 0x44F000-0x452000 / 0x51B000-0x51C000 / 0x570000-0x574000）は非侵入（該当なし＝触れていない）。

---

## 2. 使用 probe 一覧

実行形: `chcp 65001` → `python $env:TEMP\opencode\v317_<n>.py *> $env:TEMP\opencode\v317_<n>.out`。依存は `%TEMP%\opencode\v307_common.py`（読取専用・変更なし）。`tmp/re/re_util.py` は不使用。

| script | 対応項 | 内容 / 主結果 |
|---|---|---|
| v317_01 | N1/N7 | 許可4帯の bit3 ALU 全走査（命令境界: 帯頭から線形デコード＋サイズ付き）＋台帳 API callers（452C90=3/452CF0=1/452D10=1/452D30=3/452D60=3/452DF0=14/452230=3）＋ 452D10/452D30/452230 本体 |
| v317_02 | N1/N3 | **FUN_00465130 全文 dump（0x465130..0x465856・616 行）**＋主要 call 注釈＋45AD00/45B9A0/45BB00/45BB30/45ADC0/45CC60 の callers |
| v317_03 | N2 | callers(0x45ADC0)=2 / callers(0x45CC60)=1 の push 列、FUN_0045ADC0 全 dump、FUN_0045CC60 窓 0x45CC60-0x45CE20、head 検証 |
| v317_04 | N6 | 459950/459990 head・本体・callers（17/6 件）、459AF0 head、459AF0 callers、文字列 13 点、rdata/data の imm32 ポインタスロット=0 |
| v317_05 | N7 | bit7（test *,0x80 / or / and / btr/btc）許可4帯走査＝145 ヒット（大半は `and ecx,3` 等のノイズ）、保存側 jns 11 件の文脈 |
| v317_06 | N4/N8 | 45B9A0/45BB00/45BB30 本体＋heads＋push 列、pass2 再発行窓 0x461D30-0x461DF0、LoadGame tail 窓 0x4667D0-0x466830 |
| v317_07 | N5 | **0x5F0310 全文**、RTTI TD 0xB14914/0xB14930 名、callers(0x5F0310)=4、Owner getter 文脈 |
| v317_08 | N9 | formatter 0x456E80-0x4575C0 ゲート列、区画内 jcc xref 逆引き、quiet/label 文字列 |
| v317_09 | N2/N3/N5/N6/N7/N8/N9 | 4535A0 の 5F0310 呼出窓、5F0310 callers 3 点文脈、459AF0 呼び手 fn 0x459F30、459AF0 payload 窓、fn 0x463700 窓（or/and 0x80）、0x458771 follow-up、pass2 整列窓、quiet 文字列 7 点、**45AD00 本体**、45ADC0 prologue、quicksave 頭（ecx） |
| v317_10 | N8/N3 | raw bytes 0x461D30.. の単発検証＋頭 0x461680 からの整列デコード（`call 0x453500`@0x461D4D をバイト確定 `52 E8 AE 17 FF FF`） |
| v317_11 | N4/N5/N7/N9 | V1: formatter `jns` 全数＝**4 件**（頭 0x453A90 から線形・サイズ付き境界検証）/ V2: 4535A0 bit16 合成窓（頭 0x4535A0 起点）/ V3: fn 0x463700 整列窓＋callers 0x464499/0x4646D9 文脈 / V4: fn 0x459F30 全文＋0x4CBECC 文脈 / V5: quicksave caller＝`mov ecx,[0xb33b00]` / V6: 0x625D30 |
| v317_12 | N4/N7/N9/N2 | F1: 45BB30 尾部（bit5 ラッチ）/ F2: pass2 0x461DEC-0x461E90 全文 / F3: 保存側 8bit sign test＝**0 件** / F4: 型チェーン 0x456263-0x45627A（頭起点）/ F5: 459AF0 dispatch 整列 / F6: 0x463922 xref＝jcc@0x46388E のみ / F7: head 一覧（0x45CC60 off-by-pad 再確認）/ F8: LoadGame の 45AD00 呼出 |

全12本、初回・再実行とも **EXIT=0**（§9）。

---

## 3. N1-N9 各項の判定表

| # | 残点（v315 §3） | 判定 | 一句话裁定 |
|---|---|---|---|
| N1 | 保存ライタの bit3・CELL 入力経路＝ファイル内 bit3 の存否 | **[確定]** | 保存ライタに **bit3 の直接生成・除去は 0 件**。ファイル bit3 は2経路のみ: (A) tail≠0 は pass2 後の entry flags を直書き（pass2 が bit3\|bit1\|bit2 の複合例をスキップした場合のみ残存）、(B) tail==0 は 4535A0（CheckFlags）出力を書く＝CELL は入力パススルー／非 CELL は 5F0310 述語で bit3 を**再合成**。bit3 単独 entry は pass2 で 452DF0 削除＝非書出。実ファイル面は §7 |
| N2 | caller1/caller2 の 12B レコード出所（SLG+0x40 同否） | **[確定]** | caller1（0x45B029）＝**SLG([SaveGame])+0x40 の 12B レコード・リスト**（this=ecx で渡る・ノード内容=12B を命令実測）。caller2（0x45CD9F）＝**同リスト非依存**、SaveGame+0 マップ（0x55E000 find）の value[0]=flags を form 本体から直接取得 |
| N3 | FUN_00465130 の save/load 同定矛盾の仲裁 | **[確定]** | **保存オーケストレータ**（quicksave 等 6 caller・mode=0 恒・open(mode0)→ヘッダ書→レコード書出。this=[0xB33B00] を quicksave 呼び手で実証）。pass2 の唯一 caller=0x4651C3（本関数内）→ **pass2 は保存直前実行＝v289「LoadGame pass2」は呼称誤り**。v315 R1(f) の裁定保留を解消 |
| N4 | 45BB30（pass2 再発行後の (form,0)）と 45BB00 本体の同定 | **[部分確定]** | 45BB00=**ストリーム read ラッパ**（(stream,buf,count)→[stream+4] ファンクタ、ret 0xC・LoadGame 7 caller）[確定]。45BB30=**TLS+0x185 クリア＋vfunc/レベル/位置系取得・正規化の副作用型述語**（arg2 立ちで [obj+0x18] bit5 ラッチ下 0x4DD4B0 呼出）・**pass2 は戻り値を棄却＝副作用目的** [確定(命令)]、名称/厳密目的 [未確定] |
| N5 | 5F0310 述語の名前と vtable+0x4C getter の名前 | **[部分確定]** | 5F0310 本体全文 [確定]（(form,flags)→bool: [form+0x3c]≠0 ∧ arg&0x40 で ActiveEffect/ReanimateEffect([ae+0x3c]≥0x1E) ∧ [form+0x58] スクリプト vfunc+0x2E4∈{1..5}）。RTTI TD=ActiveEffect/ReanimateEffect [確定]。**名称 [未確定]**（文字列なし・機能は [推定]）。vtable+0x4C getter は mask 返 u16・差分表示 [確定]、**名称 [未確定]**（クラス依存 vtable） |
| N6 | 0x459950 / 0x459990 の適用対象帰属 | **[確定]** | **ペイロード内参照 ID の解決ヘルパ**（459950=u32 ID→446B80 ゲート→[SaveGame+0x74] 配列 / 459990=u16 idx→[SaveGame+0x78] 配列）。ラベル帰属ではなく**フィールド形態別の解決処理**（459AF0 の bit{2∧4}/0x4000000/0x2000000 分岐＝形態別）。R6'「一般整形分岐」裁定を追認・強化。テーブル境界の解釈は [推定] |
| N7 | 保存側 bit7(0x80) の意味（v315: 調査可能帯 0 件＝残帯走査） | **[確定]** | 残帯全走査で **保存側 changeFlags の bit7 分岐は 0 件**（imm `test *,0x80`・8bit sign test・or/and 0x80 すべて 0）。bit7 実在は (1) **formatter 第3ゲート `test dl,dl/jns`@0x458771-73＝CHANGE_ACTOR_BASE_FULLNAME（type 0x23/0x24）新発見**、(2) SaveGame+0x18 の bit7 一時ラッチ@0x463966-0x4639A9（fn 0x463700 内）。実 .ess 面は §7 |
| N8 | pass2 再発行レコードの完全バイトレイアウト | **[確定]** | 新 tail = `<u16 size' = 旧size−Δ（bit23 立で +4）><char type><typeID>` ＋【bit23 のみ】4B key(formID) ＋ 旧 tail payload 先頭 Δ バイトを除く残り（Δ = bit31 立ち 0x2C / 消し 0x1C）。alloc＝size'+4（453500）で3 append と**数式的に完全一致**。Δ 領域の中身は [未確定] |
| N9 | Trespass 以前の型ゲートチェーン（区画 {32,33} 入口） | **[確定]** | 入口は型 gate 2 源のみ（`cmp al,0x32`@0x45626A / `cmp al,0x33`@0x456272 → 0x456FF1）。入口からの flag 連鎖: bit29@0x456FF1→bit22@0x457103→bit21@0x45722B(Script Modifiers)→bit20@0x457341(Magic Modifiers)→bit19@0x457461(Run Once)→**bit18 Trespass@0x4575A3（流入は 0x45746B のみ）**。新知見: 0x31 区画の bit18@0x456EEB = CHANGE_DOOR_OPEN_DEFAULT_STATE＝bit18 は formType 二義 |

### 3.1 5 段階集計（9 項）

| 判定 | 件数 | 項番 |
|---|---|---|
| [確定] | **7** | N1, N2, N3, N6, N7, N8, N9 |
| [部分確定] | **2** | N4, N5 |
| [推定] | 0（主判定） | —（副次: N6 テーブル境界・N7 bit7 ラッチ意味・N1/N5 機能解釈は各項内 [推定]） |
| [未確定] | 0（主判定） | —（副次・残件は §6） |
| [確定負] | 0（主判定） | —（副次: 保存側 bit7 分岐=0 件、保存ライタ bit3 ALU=0 件、459950/459990 の rdata ポインタスロット=0） |

---

## 4. VA/命令/caller-callee 根拠

### 4-1. N1: 保存ライタの bit3・CELL 入力経路 — [確定]

**(a) 保存実行の呼び出し順序（FUN_00465130 全文実測・本件新規の完全版）**
```
465159: [ebp+0x18]>>9 bit9(dry) ‖ call 0x452330 ‖ name=="autosave" → 継続（v316 項目6 踏襲）
4651C3: call 0x461680   ; ★pass2（ledger 整形）が open より先
4651DE: call 0x45f2e0    ; open(mode=0 書新規)
465204: call 0x45f0f0 → mov [ebp+0x40],eax  ; レコード・リスト生成
465228: call 0x464ab0    ; ヘッダ書出し（counter/size 等・v313/v316 既確定）
465303..0x465711: ★レコード出力ループ（下記 (b)(c)）
4657E1: call 0x45adc0    ; 変更一覧 .txt レポート（N2）
```
- this = **ebp = ecx = [0xB33B00] SaveGame 単体**: quicksave 呼び手 `mov ecx,dword[0xb33b00] / call 0x466ad0`@0x672F36-0x672F3C → `mov esi,ecx`@0x466AE1 → `mov ecx,esi`@0x466AF4 → `call 0x465130`@0x466AF6 [確定]（帯外・最小文脈）。

**(b) ループの flags 出所と2経路（bit3 直接操作なし）**
- entry 取得: `mov ecx,[ebp]`（=SaveGame+0 マップ）→ **`call 0x452600`**@0x46531F（pass2 と同一 API）→ `mov edx,[eax+4]`（tail ptr）@0x46533C・`test / je 0x465472`@0x46533F-55。
- **flags 読み = `mov eax,dword[eax]`（value[0]）→ `mov [esp+0x3d],eax`**@0x465344-0x465346。pass2 は 0x4651C3 で先行実行し entry+0 を書き換える（0x461DAF `mov [eax],ecx`）→ **ループは pass2 後の flags を見る** [確定]。
- **経路 A（tail≠0・旧 tail あり）** — flags 無加工直書き:
  - 旧 tail ヘッダ 4B 読取（`mov edx,[ecx]`@0x465365・cursor+4@0x465369）→ char/typeID をローカルへ（0x465375/0x46537E）。
  - **10B 書出**: buf=E+0x38（`lea ecx,[esp+0x44]`@0x4653B0, push 後算出）＝`<formID @+0><type @+4><flags @+5><typeID @+9>`、size=10（`push 0xa`@0x4653AE）→ **2B 書出**＝tailSize u16（buf=[esp+0x6c]・`push 2`@0x4653DF）→ **tail payload 書出**（count=movzx u16@0x465405, buf=[ebp+0x14]@0x465419）。
  - **この経路に bit3 の test/or/and は一切なし**（全文 dump と帯走査 v317_01 の双方で 0 件）[確定]。
- **経路 B（tail==0・新規）** — **4535A0 経由のみ**:
  - `push ecx(flags) / push edi / mov ecx,ebp / call 0x46b250`（formID→form）→ **`push ecx / push edi / mov ecx,ebp / call 0x4535a0`**@0x465485-0x465494 → 出力を `mov [esp+0x3d],eax`@0x465499 → 同一フレームから 10B+2B 書出（`push 0xa`@0x4654C3 / `push 2`@0x465526）。
  - payload は vtable+0x4C(form,flags)@0x4654E8 → 0x452250 → **`call 0x460710`（直列化）**@0x465573 → vtable+0x50 → **`call 0x45c4a0`（バッファ書出）**@0x4655AE。
- **4535A0 の性質（v315 R1(d) 踏襲＋本件補完）**: CELL 経路＝**入力パススルー**（bit3/bit31 保持・bit24/25/26/28 をライブ合成）→ **CELL の Cell Flags 表示（formatter `test dl,8`@0x45468B）はファイル/台帳の bit3 に依存 [確定]**。非 CELL 経路＝bit3 ← `call 0x5f0310` 真で `or ebx,8`@0x453778 / 偽で vtable+0x198 判定→`and ebx,0xfffffff7`@0x45378F。**本件新規**: 同窓で bit16 もライブ合成（下記 N5 (d)）。
- **pass2 側の bit3 生死（v315 R1(a) を整列デコードで再確認）**: `test bl,8 / je 0x461dec`@0x461CB3（bit3 なし＝通過）/ **`test bl,6 / jne 0x461dec`@0x461CBC（bit1|bit2 同時＝翻訳スキップ→bit3 残存）** / `cmp ebx,8`@0x461CC5 単独なら `call 0x452df0`（削除）@0x461CD7 / 複合なら `and ebx,0xfffffff7 / or ebx,0x800`@0x461CF4-CF7。

**(c) 結論（ファイル内 bit3 の存否・静的）**
1. **保存ライタが bit3 を新規セット/除去する命令は 0 件 [確定]**（v317_01: 許可4帯の bit3 ALU は pass2 0x461CB3/0x461CF4・4535A0 0x453778/0x45378F・formatter 3 ゲート・帯外系のみ。FUN_00465130 全文にも 0 件）。
2. **ファイルに bit3 が書かれるのは2条件のみ [確定]**: (A) tail≠0 かつ pass2 がスキップした例（bit3∧(bit1|bit2)）、(B) tail==0 かつ（CELL＝pass2 後 entry に bit3 残存 / 非 CELL＝5F0310 述語が真）。
3. **bit3 単独 entry は pass2 で 452DF0 削除 → ファイル非書出 [確定]**（=保存を1周すると bit3 単独レコードは消える）。
4. **ファイル実在面（実 .ess に bit3 があるか）は動的照合待ち [未確定]**（§7）。

### 4-2. N2: caller1/caller2 のレコード出所 — [確定]

**(a) caller1 = FUN_0045ADC0（head 0x45ADC0: pre8=`…c2 04 00`+CC×11・16 アライン・非 CC）**
- 呼び手2件（push 列実測）:
  - **保存**: 0x4657E1 — `mov ecx,[ebp+0x40]`（=SaveGame+0x40 リスト）/ `lea edx,[esi+0x3c]`（slot path）/ `push edx` / `call 0x45adc0`。
  - **ロード**: 0x4669AC — 同型（`mov ecx,[ebp+0x40]`@0x466995・`lea eax,[esi+0x3c]`@0x4669A8）。ただし `test / je` でリスト NULL ならスキップ。
- 本体: `sub esp,0x554`・arg1=[esp+0x558]（path）→ IAT 文字列結合（`push '.txt'(0xA3A4D4)`@0x45ADF3・[0xa28164]/[0xa28160]）→ **`<path>.txt` 生成**→ 表見出し（`'Changes'`0xA3A4CC・`'Flags'`0xA3A4C0・`'Form ID'`0xA3A4B8・書式 `'  %-8s %-5s %-8s %02s %-40s  %s\r\n\r\n'`0xA3A494）→ ノード列挙。
- **ノード＝12B レコード [確定]**: `mov eax,[esp+0x30]`（列挙基点ローカル）→ `mov ebx,[eax]`→ **`mov ecx,[ebx]`（+0 formID）→ `call 0x46b250`**@0x45AFE1-45AFE8 / **`movzx eax,word[ebx+0xa]`（+10 tailSize）**@0x45AFE8 / **`mov ecx,dword[ebx+5]`（+5 flags）**@0x45B003 / **`movzx eax,byte[ebx+4]`（+4 formType）**@0x45B00E。
- **formatter push 列（0x45B016-0x45B029＋pre16 raw `51 8b 0d 00 3b b3 00 55 8d 94 24 7c 03 00 00 52`）**: `push 0`（quiet=0）→ `push eax`（formType）→ `push ecx`（flags）→ `mov ecx,[0xb33b00]`（this=SLG）→ `push ebp`（form）→ `lea edx,[esp+0x37c] / push edx`（buf）→ `call 0x453a90` [確定]。
- レコードの生成元 = **45AD00**（本件全文取得）: `new(0xC)`@0x45AD04 → 12B memcpy@0x45AD0F-0x45AD20 → 0x4527a0（所在確認）→ 0x452710（挿入）→ **`push 0x452150 / push esi / call 0x416650`**@0x45AD68-0x45AD70（compare 0x452150 = `[x+0xa]` u16 3-way＝v316 compare カタログ #2-4 と同一・45AD00 内3箇所 0x45AD70/0x45AD91/0x45ADA9）。
- **callers(0x45AD00)=6**: 0x45E02A / 0x45E0A3 / 0x4614EE（pass2）/ **0x465461・0x46560A（保存ループ）** / **0x46685D（LoadGame・`mov ecx,[ebp+0x40]`@0x466851 → ファイル由来 12B を追加）** [確定]。
- → **caller1 の出所 = [SaveGame+0x40] リスト [確定]**（保存時はループの出力フレーム＝post-pass2 flags、ロード時はファイル由来レコード）。付記: 列挙基点 `mov eax,[esp+0x30]` のローカル伝搬（this→[esp+0x30]）は完全追跡せず [部分確定]（ノード内容自体は [確定]）。
- 帳票用途 [確定]: 'Changes.txt' への変更レコード表（quiet=0＝通常ラベル）。fmt は formatter 1 本のみ（callers(0x453A90)=2 と整合）。

**(b) caller2 = FUN_0045CC60 — SLG+0x40 非依存 [確定]**
- **head = 0x45CC60（off-by-pad 第7例）**: 直前 `ret 4`@0x45CC5C（3B）+ **CC 1B**@0x45CC5F → `func_head(0x45CC60)=False`（直前≥4B CC ラン不充足）、`find_head(0x45CC60)=0x45C9C0` は誤答。先頭 `sub esp,0x204`（非 CC）で新関数開始 [確定]（教訓5例 0x45B300・第6例 0x452330 に続く）。
- 出所: **SaveGame+0 マップ lookup** — `mov eax,[0xb33b00] / lea ecx,[esp+0x18] / push ecx / mov ecx,[eax]`（=map）`/ push edx`（=[edi+0xc]=formID）`/ call 0x55e000`@0x45CCF2-0x45CD15 → `mov eax,[eax]`＝**value[0]=changeFlags**@0x45CD22 → **`push eax / push edi / mov ecx,[0xb33b00] / call 0x4535a0`**@0x45CD2A-0x45CD2C（CheckFlags）→ `test eax,eax / jne 0x45cd8b`、ゼロなら **`'References changes were nullified by CheckFlags().'`@0xA3A70C**、未ヒットなら `'Current reference has no changes.'`@0xA3A6E8。
- formatter push 列（pre16 raw `6a 01 51 8b 0d 00 3b b3 00 50 57 8d 54 24 2c 52`）: `push 1`（quiet=1）→ `push ecx`（=[edi+4] formType@0x45CD8B）→ `mov ecx,[0xb33b00]` → `push eax`（flags）→ `push edi`（form）→ `lea edx,[esp+0x2c]/push`（buf）→ `call 0x453a90` [確定]。
- 呼び手 = **0x40A9A5 の1件のみ**（帯外・浅層: `mov edx,[edi+0x24]` 等・push 列は不完全記録）→ コンソール 'SAVEGAME INFO' 系の参照ダンプ [推定]。
- → **caller2 の出所は SLG+0x40 リストではなく SaveGame+0 マップ [確定]**（caller1 と異ルート）。

**(c) R7 残件（caller 側 push 群）への答 [確定]**: 両 caller の 5 引数（buf/form/flags/formType/quiet）を push 列＋pre16 バイトで実測 → callee 5 引数マッピング（v315 R7）と完全一致。quiet は caller1=0 / caller2=1。

### 4-3. N3: FUN_00465130 save/load 同定矛盾の仲裁 — [確定]

**両論併記**:
- (a) v312 件6/件11・v313 項目4・v316: 「FUN_00465130 = save（SaveGame 実行）」。
- (b) v289: 「LoadGame pass2 @0x461680」（pass2 をロード側処理と呼称）。

**本件の新証拠**:
1. **保存命令列（全文実測）**: open(mode0)@0x4651DE → ヘッダ書 0x464AB0@0x465228 → **12B レコード直接書出**（10B+2B×2 経路・N1 (b)）→ tail payload 0x45C4A0@0x4655AE → counter/size 計上（dry=計上のみ・`add [ebp+0x90],…` が全書出に並走）→ スロット破棄@0x46582E-0x465834。ロード系関数（0x465860 = FUN_465860 ロード実行）を内部に持たない。
2. **mode=0 恒・caller 6 件（quicksave/autosave/test/console/通常/再保存）** = v313/v316 既確定を踏襲（再計測不要のため引用）。
3. **this 実証**: quicksave 呼び手の `mov ecx,[0xb33b00]`@0x672F36（§4-1 (a)）。本関数内の `mov al,[ebp+0x7c]`（version=0x7D@0x465225 設定）・`[ebp+0x88]` counter・`[ebp+0x90]` size・`[ebp+0x40]` list はすべて SaveGame フィールド（v313/v316 既確定）と一致。
4. **pass2 の唯一到達点**: `callers(0x461680)=1 = 0x4651C3`（v312 件11・global 走査）＝**pass2 は FUN_00465130 のみから呼ばれる**。ロード経路（FUN_465860 / dispatch 0x466000-0x466AC6）に pass2 呼出なし。
5. **pass2 の中身が保存時処理と整合**: 整列デコードで pass2 末尾 = entry 削除/オブジェクト後始末の清理部 — {32,33} created 系は 0x446B80（created 判定）＋3 述語（0x4db4a0 / vfunc+0x78 / 0x4d9040）全滅で `call 0x452df0`@0x461E42＋deleting dtor@0x461E54、0x31 区画は Dropped Item（`test ebx,0x20000`@0x461E67）で `call 0x452df0`@0x461E78＋dtor@0x461E8A → **保存前の台帳掃引** [確定]。
6. **45AD00 の「ロードリストと同じ」は傍証不成立**: 0x46685D（ロード）と 0x465461/0x46560A（保存）の**双方向使用**を実測 → 「45AD00 あり＝ロード側」の推論は成り立たない（v315 R1(f) の (b) 方向旁証を棄却）。

**裁定**: **(a) 採用 [確定] — FUN_00465130 = 保存オーケストレータ。pass2 = 保存直前の ledger 整形 pass（呼称は「SaveGame pass2」が正当）**。v289 の「LoadGame pass2」は**呼称誤り**（pass2 が処理する ledger flags の「起源＝ロード時挿入」(v315 R1(e)) と「実行タイミング＝保存直前」は別命題で矛盾しない）。**v315 R1(f) の裁定保留を解消**。

### 4-4. N4: FUN_0045BB00 / FUN_0045BB30 の同定 — [部分確定]

**(a) FUN_0045BB00（head 0x45BB00: pre8=`c2 0c 00`+CC×11・16 アライン・非 CC）[確定]**
```
45BB00: push ecx
45BB01: mov ecx,[esp+0x10] (arg3=count) / mov edx,[esp+0xc] (arg2=buf)
45BB09: push 1 / lea eax,[esp+4](&got) / push eax / mov eax,[esp+0x10] (arg1=stream)
45BB14: push ecx / push edx / push eax
45BB17: mov eax,[eax+4]         ; [stream+4] = read ファンクタ（ctor 登録 0x430050）
45BB1A: mov [esp+0x14],1
45BB22: call eax                ; read(stream, buf, count, &got, 1)
45BB24: add esp,0x18 / ret 0xc
```
- → **ストリーム read ラッパ（3 引数・cdecl 相当の内部 5 引数 read 呼び）** [確定]。callers=7 すべて LoadGame 系: 0x465D97（`push 4`）/ 0x465DC0（`push 6`）/ 0x465DFF（`push 0xc`・ver<0x5B）/ 0x465ED6 / 0x466113（`push 0x2c`）/ 0x46667E / 0x466816（**count=tailSize u16**@0x466805、buf=[ebp+0x14]）＝v315 の「payload 長さとして使用」と一致。

**(b) FUN_0045BB30（head 0x45BB30: pre8=`…ff d0 83 c4 18 c2 0c 00`+CC×10・16 アライン・非 CC）**
- **TLS 操作**: `mov eax,[0xba9de4] / fs:[ecx+eax*4] / mov dl,[eax+0x185]` 保存→`mov [eax+0x185],0`@0x45BB44-0x45BB57（終端で復元）[確定]。
- **本体（引数 (form=arg1, flag=arg2)・this=ecx）**: `vfunc+0x190(form)`@0x45BB64 → `call 0x5e0260`@0x45BB7E → `call 0x5e1f60 / 0x5e1f40`（戻り edi/ebp・**0x5E1Fxx 帯＝Level 系 getter の隣接**）→ `vfunc+0xF4(this,&pos)`@0x45BBB8 → `call 0x4d8a30` → `vfunc+0xF0`@0x45BBE5 → `fld [eax+8] / call 0x4d8a10`@0x45BBEB-0x45BBF0。
- **flag=arg2 の作用 [確定]**: `mov bl,[esp+0x60] / test bl,bl / je 0x45bc05` → **`or dword[eax+0x18],0x20`（bit5 セット）@0x45BC01 → `call 0x4dd4b0`（3 引数）@0x45BC08 → `and dword[eax+0x18],0xffffffdf`（bit5 復元）@0x45BC18** → `[esp+0x12]=1` → jmp 0x45bd04（成功時 al=1）。偽経路 0x45BC26 は `lea edi,[esi+0x44]` からの位置系後段（`push 0x14/call 0x41e210` 等）。
- **callers=3**: 0x45BE40（`push setne(byte[esp+0x24],0xC)`＝**flags bit2|bit3 検査**→`push ebx`（RTDynamicCast 結果=form）→ `call`、結果 `test al,al / je` 使用）、**0x461DE7（pass2・`push 0 / push eax / call`）— 直後 0x461DEC に `test al` なし＝戻り値棄却 [確定]**（F2 で 0x461DEC-0x461E90 全確認。al が次に使われるのは 0x461E0A で 0x446B80 の戻り）、0x4CFF74（帯外・VA のみ）。
- → **本体 [確定]**: 「TLS+0x185 一時クリア＋位置/レベル系の取得・正規化、arg2 立ち時は [obj+0x18] bit5 ラッチ下で 0x4DD4B0 呼出、成功で al=1」。**名称と厳密目的 [未確定]**（pass2 が副作用目的で呼んでいることは確定、副作用の正体は 0x4DD4B0/0x4D8A10/0x4D8A30 の同定待ち＝§6）。

### 4-5. N5: FUN_005F0310 述語と vtable+0x4C getter — [部分確定]

**(a) FUN_005F0310 = 0x5F0310..0x5F03FE（`ret 4`・1 引数・head 検証: pre8=`c2 04 00`+CC×11）[確定]（帯外・N5 指定）**
```
5F0311: edi = ecx = form
5F0313: cmp [edi+0x3c],0 / je → false             ; [form+0x3c]≠0 必須
5F031F: test byte[esp+8],0x40 / je 0x5f0395        ; arg1(flags) bit6 未立なら共通部へ
5F0327: eax=[edi+0xb0]; ==1/2/6 → true(0x5f03f3); ==4 のみ次段、他→共通部
5F034D: [edi+0x68] の vtable+8 getter → リスト走査
5F0360: [esi+4]==0 ∧ [esi]==0 → 共通部 / RTDynamicCast([esi],0, TD 0xB14914, TD 0xB14930, 0)
        → 成功 ∧ [eax+0x3c] ≥ 0x1E(30) → true、失敗なら [esi]=next ループ
5F0395(共通): ecx=[edi+0x58]（スクリプト）/ null → false
        vfunc+0x2E4() ∈ {1,2,3,4,5} → true、他 → false
5F03F3: al=1 / 5F03FA: al=0 / ret 4
```
- **RTTI TD**: `0xB14914 = '.?AVActiveEffect@@'` / `0xB14930 = '.?AVReanimateEffect@@'`（+0xB14950=`.?AVSummonCreatureEffect@@` は 0x5F07B9 で使用）[確定]。
- **callers=4**: 0x45376F（4535A0 非 CELL・**arg1 = ebx = 計算中 flags**）/ 0x45D296（D220・**arg1 = SaveGame+0 マップ value[0]**）/ 0x4DC22C・0x65B813（**arg1 = -1**＝全 bit 立て）[確定]。
- **名称 [未確定]**（参照文字列なし）。**機能解釈 [推定]**: 「スクリプト（type 1..5）または魔法効果（ActiveEffect/Reanimate、accum≥0x1E）を持つ＝実行時変更対象」述語。bit3 の表示/保存での役割（非 CELL の bit3 再合成元）とは命令で接続 [確定]。

**(b) vtable+0x4C getter（Owner 例・formatter）[確定]（名称のみ [未確定]）**
- `mov eax,[ebp] / mov edx,[eax+0x4c] / push 0x80 / mov ecx,ebp / call edx`@0x45670F-0x45671C → **戻りは u16**（`movzx eax,ax`@0x456723）→ `movzx ecx,word[esp+0x14]` → **`sub eax,ecx`（差分）**@0x456726 → `push eax / push 0xa397bc / call 0x98208b`（sprintf）。push 0x80＝要求 bit（changeFlags mask）[推定]、**名称はクラス依存 vtable のため [未確定]**（v315 R6 の留保を維持）。

**(c) 本件新規: 4535A0 非 CELL 経路は bit16 もライブ合成 [確定]（頭 0x4535A0 起点の整列デコード）**
```
453738: push 0xb03e00 ('.?AVTESPackage@@') ... call 0x5e0380 / push eax / call 0x9832e6 (RTDynamicCast)
45374F: test eax,eax / je 0x45376c             ; TESPackage 非該当なら無変更
453753: mov ecx,eax / call 0x625d30            ; 0x625D30 = mov eax,[ecx+0x5c]; ret
45375A: cmp eax,edi / je 0x453766
45375E: or ebx,0x10000                          ; 不一致 → bit16 セット（Non-saved Package）
453766: and ebx,0xfffeffff                      ; 一致 → bit16 クリア
45376C: push ebx / mov ecx,edi / call 0x5f0310 ... (bit3 合成)
```
- → **formatter の bit16（Non-saved Package・R5）も表示時に 4535A0 で合成され得る**。R5 の「ゲートは bit17/bit16」は維持しつつ、**入力依存の断定は 4535A0 経由分を補足**（保存側 tail==0 経路でも同じ合成がファイル flags に入る）。
- 0x625D30 = getter 1 行（[pkg+0x5C]）[確定]・呼び手 4 件（0x453755 / 0x611745 / 0x62F501 / 0x64FF50）。**cmp の対象（pkg[+0x5C] vs form）の意味 [推定]**。

### 4-6. N6: 0x459950 / 0x459990 の適用対象帰属 — [確定]

**(a) FUN_00459950 = 0x459950..0x459988（`ret 4`・head: pre8=`…5f 5e 5d c3`+CC×11）**
```
459951: esi = arg1 (u32 ID)、edi = ecx = this
459958: mov ecx,[0xb33a98] / push esi / call 0x446b80   ; created 判定（v309 確定: ≥0xFF000000）
459964: true → eax=esi のまま return                     ; created ID はパススルー
45996F: eax=[edi+0x74]（テーブル）/ cmp esi,[eax+0xc] / 超過 → 0
45997E: eax=[eax+4][esi*4] → return                      ; 配列解決
```
**(b) FUN_00459990 = 0x459990..0x4599A8（`ret 4`・head: pre8=`…c2 04 00`+CC×11）**
```
459990: eax=[ecx+0x78]（テーブル）/ ecx=movzx u16 arg1 / cmp ecx,[eax+0xc] / jae → 0
4599A2: eax=[eax+4][ecx*4] → return
```
- this は呼び出し直前 `mov ecx,ebp`（ebp = SaveGame）→ **[SaveGame+0x74] / [SaveGame+0x78]** = v316 項目7 で初期化が確定した2本の配列テーブル（new(0x18)+vtable 0xA39044）[確定]。rdata/data の imm32 ポインタスロット = **0 件**（直接 call のみ）。

**(c) 適用場所（ペイロード形態別ループ = 459AF0・頭検証 OK: pre8=`…5e c3`+CC×8）**
- 459AF0(this=SaveGame, arg1=objID, arg2=1)。gate: `[SaveGame+0x18]>>12 bit12`@0x459B10 / map find@0x459B29 / **version `cmp [esp+0x1f],0x5b / jae`**@0x459C03（頭起点整列）。
- **分岐（教訓③の整列確認済み）**: `test al,2 / je 0x459c5d`@0x459C0A ∧ `test al,4 / je`@0x459C0E → **両立で 12B 読取（cursor+0xC）→ `call 0x459950`**@0x459C34 / 欠け → **2B(u16) 読取 → `call 0x459990`**@0x459C7B / `test 0x4000000` → **4B → 459990**@0x459C9D / `test 0x2000000` → **6B（u32+u16）→ 459990**@0x459CE6。各パスとも **`cmp eax,ebx / jne skip → call 0x452df0`（entry 削除）**@0x459C41/0x459CAA/0x459CF3 相当。
- 459AF0 呼び手: **fn 0x459F30（`mov ecx,[0xb33b00]` の this・arg=[obj+0xc]=formID・0x4C97F0 真で 459AF0 呼出）← 0x4CBECC（帯外・`test byte[edi+0x24],1` 守）1 件のみ** [確定]。
- その他の消費者: **459950 = 17 件**（0x459C34 のほか 0x460C9E/0x460CAE/0x460E04/0x460E10/0x460E47・0x463DC9/0x463DD9/0x463E5E/0x463E6E・0x465E25/0x465EE2/0x465EF2/0x466122/0x466138/0x46633F/0x466357 ＝全 LoadGame/dispatch 帯）/ **459990 = 6 件**（459AF0×3 のほか **pass2 0x46183C/0x461894**・LoadGame 0x465DDF）。
- → **帰属裁定 [確定]: 459950/459990 は「ラベル」ではなく、serialized ペイロード中の参照 ID を [SaveGame+0x74]/[+0x78] テーブルで解決するフィールド解決ヘルパ**。v315 R6' の「S8『0x2000000 適用』は一般整形分岐」裁定を**追認・強化**（bit 分岐はラベル判定ではなく形態別読取長分岐）。**テーブル境界（[t+0xc] 以下解決/超過 0 と 0x446B80 の組合せ）の解釈は [推定]**（§6-7）。

### 4-7. N7: 保存側 bit7(0x80) — [確定]

**(a) 保存側 changeFlags の bit7 分岐 = 0 件 [確定]（残帯全走査）**
- `test *,0x80`（F6/F7 /0 形）: 許可4帯すべて **0 件**（0x4D8000 帯の `test byte[esi],…` 1 件は 0x4EF723 帯の命令境界不正＝除外）。
- 8bit sign 形（`test r8,r8` → js/jns）: 保存側 6 帯合計 **0 件**（F3）。
- `or *,0x80` / `and *,~0x80`: **0x463966 / 0x4639A0 / 0x4639A9 の3件のみ**＝いずれも **[esi+0x18] オブジェクトフラグ**（下記 (c)・changeFlags ではない）。
- 保存側 jns 11 件はすべて 32bit test（`test eax,eax` 等＝bit31 系）＝bit7 と無関係。
- → **v315 R6 の「調査可能帯の保存側 test 0x80=0」を残帯まで拡張して維持**。レコード flags の bit7 は保存ライタで参照されない＝**保存側 bit7 分岐 [確定負]**。実 .ess 面は §7。

**(b) formatter 第3の bit7 ゲート新発見 [確定]（v315/v312 の「jns=3 件」は4件に訂正）**
- 頭 0x453A90 起点の線形デコード（サイズ付き境界検証）で **jns = 4 件**: 0x4552C5（bit31 Cell Changed）/ 0x4566D5（Owner）/ 0x457EF1（Crime Gold）/ **0x458773（新）**。
- 新ゲート: **`test dl,dl`@0x458771 → `jns 0x458881`**、quiet = **`'CHANGE_ACTOR_BASE_FULLNAME\r\n'`@0xA391F8**（29B コピー@0x458792-0x45879E）。所在区画 = **0x23/0x24（ActorBase 系・0x457FF1 起点区画の第2ゲート、第1ゲート bit2@0x457FF1 → 0x458103=bit3 Base Attributes）**。
- → **bit7 の多義は3つに拡張: {type 0x31 Owner / type 0x32 Crime Gold / type 0x23・0x24 Base FullName}**（いずれも formType 区画分離＝件3 の既知パターンと整合）。v311 §5-4 多義リスト・v315 R6 の追記/更新提案（§5-1）。

**(c) SaveGame+0x18 の bit7 = 一時ラッチ [確定(命令)]（意味は [推定]）**
- 含有関数 = **FUN_00463700（head 0x463700 検証済・`ret 4`@0x46391F・後続ブロック 0x463922 は `jcc@0x46388E` の遷移先のみ＝raw xref 実証）**。this = ebp = SaveGame（呼び手 0x464499/0x4646D9 とも head **0x464440** 内・`mov ecx,ebp` で呼出、0x4646CF に `or dword[ebp+0x18],0x40` と同オブジェクト操作）。
```
46395C: mov eax,[esi+0x18] / mov ebx,eax
463966: or eax,0x80 → mov [esi+0x18],eax     ; ★bit7 セット
463976: or dword[edi+8],0x200000              ; form 側 bit21 は永続セット
46398B: call 0x460bc0 / vfunc+0x54(edi,ebp,0)  ; ラッチ期間中の処理
46399C: test bl,bl(旧 bit7) / je→and [esi+0x18],0xffffff7f / else→or 0x80  ; ★元値へ復元
```
- → **bit7 は「0x460BC0+vfunc+0x54 実行中」のラッチであり、保存処理の bit7 とは無関係 [確定]**。厳密な意味（再入ガード [推定]）と 0x460BC0 の同定は §6-5。

**(d) 意味の総合裁定**: 「保存側 bit7(0x80)」は**存在しない（changeFlags 空間）**。既知の bit7 は (1) formatter 表示3 義（formType 分離）、(2) SaveGame フラグの処理中ラッチ。v311 §5-4 多義リストへの追記は (1) のみ。

### 4-8. N8: pass2 再発行レコードの完全バイトレイアウト — [確定]

整列デコード（頭 0x461680 起点・raw bytes `52 e8 ae 17 ff ff` で 0x461D4D を確定）:
```
461CAF: ebx = [esp+0x24] = flags
461CB3: test bl,8 / je 0x461dec        ; bit3 なし: 翻訳スキップ
461CBC: test bl,6 / jne 0x461dec       ; bit1|2: スキップ（bit3 残存）
461CC5: cmp ebx,8 / jne 0x461ce1       ; 単独 bit3 → 452DF0 削除
461CE1: edx=[esp+0x14](entry) / eax=[edx+4](old tail) → [esp+0x4c]
461CEE: je 0x461dbc                    ; tail==0 は別経路（この再発行なし）
461CF4: and ebx,0xfffffff7 / or ebx,0x800 / mov [esp+0x24],esi ; bit3→bit11
461D05: jns → di=0x1c（bit31 無） / else → and esi,0x7fffffff（bit31 除去）/ di=0x2c
461D1D: ax = [esp+0x30]（旧 tail ヘッダの u16 size・0x461AAD 読取分）
461D22: sub ax,di                      ; Δ = 0x1C / 0x2C
461D25: and esi,0x800000               ; ZF = bit23（Oblivion Flag）状態
461D2E: [esp+0x30] = ax（新ヘッダ u16 size'）
461D33: [esp+0x58] = ecx = size'（append3 長さ）
461D37: je(bit23 無) → 0x461d42 / bit23 有 → ax+=4 → [esp+0x30]=ax（size'+4）
461D42: call 0x453500 (alloc = ax+4)    ; → ebx = 新 tail ptr（entry+4 へ）
461D61: call 0x45b9a0 ①4B  src=[esp+0x30]（新ヘッダ: u16 size' + 旧 char + 旧 typeID）
461D66: test esi,esi / je 0x461d80      ; esi = flags&0x800000 → ★bit23 無なら key 付加をスキップ
461D7B: call 0x45b9a0 ②4B  src=[esp+0x30]@push後=E+0x2C ← ebp（key = formID）@0x461D77
461D94: call 0x45b9a0 ③len=[esp+0x58](=size')  src=old tail+4+di（旧 payload の di オフセット以降）
461DAC: mov [eax+4],ebx  ; entry+4 = 新 tail
461DAF: mov [eax],ecx    ; entry+0 = 翻訳後 flags
461DB1: mov [edi+0x14],0 ; cursor リセット
```
- **数値整合（完全レイアウトの証明）**: appends 合計 = 4(ヘッダ) + 【bit23 有のみ 4(key)】+ size'(=旧size−Δ)。alloc = (size' ＋【bit23 有のみ 4】) + 4 → **bit23 有無の両ケースで一致** [確定]。45B9A0 本体（`memcpy([this+0x14] 頭へ len B → cursor+=len`・スレッド/flag ゲートで警告 0xA3A4E0・`ret 8`）@0x45B9A0-0x45B9F4 も取得。
- **新 tail = `<u16 size'><char type><typeID>` + [bit23 時のみ `formID u32`] + `旧 payload[Δ:]`** [確定]。**Δ(0x1C=28B / bit31 時 +0x10B) が何を除去するかは [未確定]**（§6-1）。char/typeID は旧ヘッダ（v315 R8 の 0x461AAD 読取）を踏襲 [確定]。
- LoadGame 側の対応（整合確認）: 4B ヘッダ append@0x4667E4 → tailSize 読取 45BB00@0x466816 → 452CF0@0x466829（v312 件4/件9 と矛盾なし）。

### 4-9. N9: Trespass 以前の型ゲートチェーン（区画 {32,33} 入口）— [確定]

**（a）区画入口（型ゲート）**: 頭起点整列デコード — `mov al,[esp+0x174]`@0x456263 → **`cmp al,0x32`@0x45626A → `je 0x456ff1`@0x45626C** → **`cmp al,0x33`@0x456272 → `je 0x456ff1`@0x456274** → 不一致は fall で 0x31 区画 first gate（`test edx,0x400`@0x45627A）。**0x456FF1 への jcc 流入はこの2件のみ**（formatter 全域 xref 逆引きで 0x45626C/0x456274 のみ）[確定]。

**（b）{32,33} 区画の flag ゲート連鎖（入口→Trespass、全区間実測）**:

| # | gate | VA | quiet（先頭） | skip 先 |
|---|---|---|---|---|
| 1 | bit29 `test edx,0x20000000` | 0x456FF1 | `'CHANGE_ACTOR_EQUIPMENT\r\n'`@0xA3963C | je 0x457103 |
| 2 | bit22 `test edx,0x400000` | 0x457103 | `'CHANGE_ACTOR_GAME_MODIFIERS\r\n'`@0xA3960C | je 0x457227/0x45722B |
| 3 | bit21 `test edx,0x200000` | 0x45722B | `'CHANGE_ACTOR_SCRIPT_MODIFIERS\r\n'`@0xA395D8（label `'Script Modifiers(%i)'`@0xA395C0） | je 0x457341 |
| 4 | bit20 `test edx,0x100000` | 0x457341 | `'CHANGE_ACTOR_MAGIC_MODIFIERS\r\n'`@0xA395A0（label `'Magic Modifiers(%i)'`@0xA3958C） | je 0x457461 |
| 5 | bit19 `test edx,0x80000` | 0x457461 | `'CHANGE_ACTOR_RUNONCE_PACKAGE %08X\r\n'`@0xA39568（label `'Run Once %08X(%i)'`@0xA39554） | je 0x4575A3 |
| 6 | **bit18 Trespass `test edx,0x40000`** | **0x4575A3** | （v315 既載） | je 0x4576d3 |

- **0x4575A3 への流入は 0x45746B の1源のみ** [確定]（xref 逆引き）。
- **本件新知見**: **0x31 区画の bit18 `test edx,0x40000`@0x456EEB**（quiet `'CHANGE_DOOR_OPEN_DEFAULT_STATE\r\n'`@0xA39670・skip je 0x458e1a＝終端）→ **bit18 は formType 二義（{31}=Door Open Default / {32,33}=Trespass）**。12/13 組で確認された formType 分離パターンの追加例 [確定]。
- Trespass 以前（本項の中心）は上記 2 型 gate ＋ 5 flag gate の連鎖で完結（前置きに未解明ゲートなし）[確定]。

---

## 5. v312/v313/v315 既知との整合・訂正（旧結論は不削除）

| # | 既存記載 | 出典 | 本件の裁定 | 裁定理由（証拠） |
|---|---|---|---|---|
| 1 | formatter `jns` = **3 件**（0x4552C5/0x4566D5/0x457EF1） | v312 probe15 / v315 R6 | **訂正: 4 件**（**0x458773 追加**） | 頭 0x453A90 起点の線形デコードでサイズ付き検証（v317_11 V1） |
| 2 | bit7 多義 = {0x31 Owner / 0x32 Crime Gold} の2義 | v315 R6 / v311 §5-4 | **3義に拡張: + {0x23/0x24 Base FullName}** | quiet `'CHANGE_ACTOR_BASE_FULLNAME'`@0xA391F8・区画 0x457FF1 起点（v317_11 V1） |
| 3 | v289「LoadGame pass2 @0x461680」 | v289 / v315 R1(f) 両論 (b) | **呼称訂正: 保存直前の pass2（SaveGame pass2）** | callers(0x461680)=1=0x4651C3＋FUN_00465130 全文が保存命令列（§4-3）。R1(f) の裁定保留を解消 |
| 4 | v315 R1(f)「FUN_00465130 内の 45AD00 は (b) LoadGame 方向の旁証」 | v315 R1(f) | **旁証棄却** | 45AD00 callers 6 件中 3 件が保存/pass2、1 件がロード（0x46685D）＝双方向使用。リスト共有は同定の傍証にならない |
| 5 | v312 件5 の pass2 force-remove VA: `test@0x461E67 → call@0x461E3F-42 / dtor@0x461E4B-54` | v312 件5 | **VA 引用を訂正**（結論は維持） | 0x461E3F-42/0x461E4B-54 は **{32,33} ブロック**（0x461DFA 系）。0x31 ブロック（test@0x461E67 の下流）の実 VA = **push1@0x461E75 / call 0x452df0@0x461E78 / dtor@0x461E8A**（v317_12 F2）。Dropped Item force remove の結論自体は正しい |
| 6 | off-by-pad 教訓 6 例（0x466D4D/0x5AE18D/0x44F3C1/0x51B484/0x45B300/0x452330） | 教訓 / v313 / v316 | **第7例: 0x45CC60** | 直前 `ret 4`@0x45CC5C＋CC×1 → `func_head=False`・`find_head(0x45CC60)=0x45C9C0`（誤答）・先頭 `sub esp,0x204` |
| 7 | R7「caller 側 push 群は帯外で再測不可」 | v315 R7 | **[部分確定] → [確定] へ格上げ提案** | caller1/caller2 の push 列＋pre16 バイト実測（5 引数完全一致・quiet 0/1）＝帯開放により解消 |
| 8 | v315 R6'「459AF0 の 0x2000000 分岐＝一般整形分岐」 | v315 R6' | **維持＋強化** | 459950/459990 の本体同定で「分岐＝読取長（形態別）・解決ヘルパ」を確認。row30 裁定はそのまま |
| 9 | v315 R1(e)「ledger flags はファイル・レコード由来（挿入は LoadGame のみ）」 | v315 R1(e) | **整合（訂正なし）＋接続点を追加** | 452C90 callers=3 再確認。保存側の flags 出所は同一 ledger（452600）＝起源ロード・整形保存の2段 |
| 10 | R5「Non-saved(bit16) は入力 flags 依存」 | v315 R5 | **補足: 4535A0 非 CELL 経路で bit16 もライブ合成** | 0x453753-0x453766（TESPackage キャスト→0x625D30→set/clear）。ゲート帰属（bit16）は変更なし |
| 11 | v312 件4「12B +9 = tail frame typeID」 | v312 件4 | **観察追加（解釈は [未確定]）** | tail≠0 書出の +9 = 旧 typeID／**tail==0 書出の +9 = [ebp+0x7c] = 0x7D（version byte）**（0x46534A-0x465351・0x4654C3 経路で無改変）。両者の整合は §6-4 |
| 12 | v316 項目3「[0xB333C4]=PlayerCharacter」等 | v316 | **整合（引用のみ）** | FUN_00465130@0x465616 `cmp edi,[0xb333c4]`（Player 特有ブロック）を確認・矛盾なし |
| 13 | v313/v316 の FUN_00465130 mode=0・ガード・caller 6 件 | v313 項目4 / v316 項目6 | **整合（踏襲）** | 本件全文 dump で命令列一致（再調査せず引用） |

---

## 6. 新規残件（次束用）

1. **[未確定] N8 の Δ(0x1C/0x2C) 領域の中身** — pass2 再発行は旧 tail payload 先頭の 0x1C（bit31 立ち時は +0x10B）を無条件除去し、bit23 のときのみ 4B key を挿入する。この 28B/44B ブロックの意味（旧ヘッダ相当か、bit31 由来サブフィールドか）は上位構造の特定待ち。実 `.ess` 照合が最短。
2. **[未確定] FUN_0045BB30 の名称・厳密な目的** — TLS+0x185 クリア・vfunc+0x190/0x5E0260/0x5E1F40-F60（Level 系）・vfunc+0xF4/F0（位置）・arg2 時の [obj+0x18] bit5 ラッチ＋0x4DD4B0 呼出＋復元。pass2 は戻り値を捨てている＝副作用目的だが、副作用の正体は 0x4DD4B0 / 0x4D8A10 / 0x4D8A30（0x4D8000-0x4F2000 帯・次束で調査可）の同定待ち。
3. **[未確定] FUN_005F0310 と vtable+0x4C getter の正式名** — 構造は本件で全文 [確定]（N5 (a)(b)）。名称を裏付ける文字列/RTTI 名が存在しない。getter はクラス依存（Owner は type 0x31 相当）vtable の解決が必要。
4. **[未確定] 保存レコード +9 byte の意味差** — tail≠0＝旧 typeID / tail==0＝0x7D(version)。v312 件4 の「typeID」解釈と tail==0 の 0x7D がどう整合するか（0x7D が typeID として合法か）はロード側 0x46679C 以降の再追跡＋実データ待ち。
5. **[未確定] FUN_00463700（SaveGame メソッド）と 0x460BC0 の同定** — bit7 ラッチ・`or [edi+8],0x200000`・`call 0x45a140`・`mov [esi+0x71]→[esi+0x7c]` を含む。呼び手 head 0x464440（v316「Player 生成」窓）との関係（0x464499/0x4646D9 の2呼出）も未説明。
6. **[推定] SaveGame+0x18 bit7 の意味** — 0x460BC0/vfunc+0x54 実行中のみ立ち元値へ復元＝再入/処理中ラッチ。他 [SaveGame+0x18] bit の相互作用（v316 の530件カタログ）との対応は未整理。
7. **[推定/未確定] 459950 のテーブル境界解釈** — created(≥0xFF000000) はパススルー、非 created は [t+0xc] 以下のみ配列解決・超過は 0。通常 formID が0に落ちる点の実解釈（入力が formID か内部 index か）は 446B80 の真偽方向の再確認（v309 既確定の引用）と保存 ID 空間の整理待ち。
8. **[未確定] 459AF0 連鎖の発動文脈** — 呼び手は 0x4CBEC0 帯（帯外）の1系統のみ（`test byte[edi+0x24],1` 守）。どの保存/ロード工程から走る整合チェック（entry 削除）かは上位関数の同定待ち。
9. **[未確定] fn 0x459F30 の arg2（`push 1`）と 459AF0 の arg2 の契約** — 459AF0 本体で arg2 の読取を特定せず（bit12 gate・map find・payload 走査のみ）。
10. **[部分確定] caller1 の列挙基点** — 45ADC0 の `mov eax,[esp+0x30]`（ノード ptr）への this（=SaveGame+0x40）伝搬をローカル範囲で未完全追跡（ノード内容・呼び手 ecx は [確定]）。
11. **[未確定] 0x464440 の v316 窓（0x464440..0x46458A）と find_head の関係** — find_head(0x4646D9)=0x464440 は v316 記載の窓を跨ぐ（関数境界の再確認要・off-by-pad 候補の可能性）。

（新規残件 = **11 件**）

---

## 7. 実 `.ess` 不在の影響（動的未実施箇所の明記）

v313 §1.1 の8系統検索（ワークスペース全深さ / %TEMP% / ユーザープロファイル / D: depth8 / Documents\My Games / Steam 先 / BSA 展開先 / `rg -a TES4SAVEGAME`）で **実 `.ess` = 0 件 [確定]**（本件では再検索せず踏襲）。

| 項目 | 未実施の照合 | 現状の判定への影響 |
|---|---|---|
| N1 | 実保存ファイルの change レコード +5 に bit3 が現れるか（pass2 スキップ例・非 CELL 再合成例の実在） | 保存経路と条件は [確定]、**実在面は [未確定]** |
| N7 | 実保存ファイルの bit7 有無 | 保存側分岐なしは [確定]、**実在面は [未確定]** |
| N8 | 旧 tail payload 先頭 Δ(0x1C/0x2C) バイトの実データ照合 | レイアウト式は [確定]、**Δ の内容は [未確定]** |
| N1/N11 等 | +9 byte（typeID vs 0x7D）の実値照合 | **[未確定]** |
| N4 | 45BB30 副作用の動的観測（bit5 ラッチ/TLS 操作の実行時意味） | **[未確定]** |

推測を確定扱いしない方針を維持した（契約書 §解析制約）。本成果物は静的解析のみ。

---

## 8. 再現手順

1. 環境: Windows / python 3.11（capstone 5.0.7・pefile）/ EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`。
2. 依存ヘルパー（読取専用・変更禁止）: `%TEMP%\opencode\v307_common.py`（F / func_head / find_head / disasm / disasm_range / str_at / callers_of / resolve_imm32_refs）。`tmp/re/re_util.py` は不使用（変更なし）。
3. 実行（全12本）:
   ```powershell
   chcp 65001
   python $env:TEMP\opencode\v317_<n>.py *> $env:TEMP\opencode\v317_<n>.out
   ```
   `<n>` = 01..12（§2 の表）。
4. 解析上の要点（本件で実効した教訓）:
   - **境界検証は「関数頭（head 検証済）からの線形デコード＋サイズ表示」で行う**（v317_10/11/12）。任意アドレス起点の窓デコードは 1 箇所で食い違うため（0x461D4C-0x461D4E で実際に発生 → raw bytes `52 e8 ae 17 ff ff` と頭起点デコードで 0x461D4D `call 0x453500` に確定）。
   - raw E8 caller 判定 = `va+5+rel==target`（callers_of）、imm32 xref はセクション名で .rdata/.data のみ採用（v317_04）。
   - off-by-pad 検証: head ごとに pre16 バイトを併記（第7例 0x45CC60 を検出）。
   - formatter 走査上限は 0x458E38（ジャンプテーブル 0x458E38-0x458E4C はデータ＝教訓④）。
5. selftest（§9）:
   ```powershell
   $dir="$env:TEMP\opencode"; $before=@{}; Get-ChildItem "$dir\v317_*.out" | %{ $before[$_.Name]=(Get-FileHash $_.FullName -Algorithm MD5).Hash }
   1..12 | %{ $f="{0:d2}" -f $_; python "$dir\v317_$f.py" *> "$dir\v317_$f.out" }
   # 前後の MD5 比較 → 差分0 を確認（2 回連続）
   ```

---

## 9. selftest 結果

- **初回実行**: v317_01..v317_12 全12本 **EXIT=0**（分析用に各 .out を読み取り・本文の VA はすべてその出力から転記）。
- **初回 selftest**: 全12本を再実行 → EXIT=0・**MD5 差分 6 件**（v317_10 は `.out` が初回生成分のため実質 5 件: 05/07/09/11/12）。差分原因は特定できなかったが、内容は分析時読み取りと一致することを確認（例: v317_05 の `TOTAL: 145` が現行出力に存在・v317_09 は行数 553 で初回と一致）。
- **再実行（決定的検証）**: 全12本をもう一度再実行 → **全 EXIT=0・MD5 比較 = 差分0（hash-differences=0）**。直前の selftest 再実行との比較も **差分0** → **出力は2回連続で一致＝現行状態で決定的**と判定。
- Traceback / Error / Warning 出現 = 全 .out で **0 件**。
- 内容別 selftest ポイント:
  - v317_02: FUN_00465130 dump が 0x465856 `ret 0xc` で終端（v313 と一致）、call 注釈 7 本（452330/461680/45f2e0/464ab0/45ad00×2/4535a0）。
  - v317_03: callers(0x45ADC0)=2 / callers(0x45CC60)=1 / head 0x45ADC0=16 アライン・pre8 CC 確認。
  - v317_04: callers(0x459950)=17 / callers(0x459990)=6 / callers(0x459AF0)=2 / rdata・data の imm32 スロット=0。
  - v317_11: formatter jns=**4**（0x4552C5/0x4566D5/0x458773/0x457EF1 を含む全列を出力）、find_head(0x45CC60)=0x45C9C0（off-by-pad 再現）。
  - v317_12: F3 8bit sign test=**0**、F6 xrefs→0x463922=[('0x46388e','jcc')] の1件、F5 の整列（0x459C0A/0x459C0E）。

---

## 10. 遵守宣言

- EXE: read-only 解析のみ（disasm は自帯 0x430000-0x431000 / 0x452000-0x467920 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800＋§1.2 の帯外最小参照）。v314 禁止帯・他 lane 成果物への非侵入。ESM: 未接触。git: 未実行。
- 編集: **`tmp/re/v317_v315_residuals.md` のみ新規作成**。既存 tmp/re 成果物（v311/v312/v313/v315/v316 等）・台帳・STATE・TODO・docs・GT・ソース（app/**）・.github/** は読み取りのみ。訂正は本ファイル §5 で提示。
- 中間物: `%TEMP%\opencode\v317_*` のみ新規（.py 12＋.out 12＝24 ファイル）。`tmp/re/re_util.py` 変更なし。
- 判定整合: 9 項 = [確定] 7（N1/N2/N3/N6/N7/N8/N9）＋ [部分確定] 2（N4/N5）＋ [推定] 0 ＋ [未確定] 0（主判定）＋ [確定負] 0 ＝ 9（一致）。副次判定・新規残件 11 件・裁定更新 13 項。

## 追補（段階1＝v319 統合時・2026-11-07）

`v319_v317_static_residuals.md`（48,061B/378行・probe 13 本・2回連続 MD5 差分 0）により本ファイルの静的残件を処理し、以下を**訂正・拡張**する（旧記述は不削除・詳細は v319 §各項）:

1. **`FUN_00463700` は誤答を訂正 → `FUN_004637C0 = TESSaveLoadGame::LoadForm` [確定(文字列)]** — 本ファイルの「0x463700」表記は **find_head の CC ラン不足型誤答**（v319 が体系化した誤答 5 例の第 1 号）。bit7 ラッチの含有関数は 0x4637C0、ラッチ機構＝`0x460BC0`（payload 適用/スキップ両用ディスパッチャ [確定]）＋vfunc+0x54、元値復元＝**適用中ラッチ [推定]**、許可帯のリーダは 0 件 [確定負]。
2. **`0x459950/0x459990` の境界を確定**: `[t+0xc] = 要素数 count` [確定]・比較が **459950=`jbe` / 459990=`jb` で 1 件ずれ** [確定]・find-or-add は `FUN_0045DE30` 内 0x45E0EF-0x45E141（＝**v316 §6-9 の投入元を同時解消**）・created 判定は 446B80 の `id≥0xFF000000` [確定]。
3. **A5（+9 byte）: 「version byte」説を [推定（4 根拠）] で採用候補**（tail==0 で [SaveGame+0x7c]=0x7D 恒常入力／tail b3 が 0x5B version ゲート比較／0x7D=現在 version 文字列／[+0x71]→[+0x7c] 初期化）。本ファイルの typeID 説と**両論併記のうえ降格を提案**。
4. **45BB30 副作用 [確定]**: 親取得（5E1F60=CELL/5E1F40=WRLD getter）→ `0x4DD4B0 = 親セル決定・接続ルーチン`（位置>>12 の grid 検索 0x4EFE80＋0x4D35D0 接続）＋ `4D8A30/4D8A10 = 位置・float セット＋通知`＋位置転記＋TLS+0x185 復元。呼び手②＝**`FUN_0045BDE0 = ResetObject` [確定(文字列)]**、45BB30 自体の正式名は [推定]。
5. **vtable+0x4C getter の分類**: slot19(+0x4C)=TESForm 既定 0x46AC50 / REFR 0x4E2BF0 / CELL 0x4D1EB0 / Actor 0x5E7D00、`ExtraDataList::GetSaveSize` 文字列と定数合算から **「mask 指定のセーブ時バイトサイズ算出 GetSaveSize 系仮想」[推定]**（`Owner(%i)`＝getter(0x80)−基準 word・基準の出所は[未確定]）。`FUN_005F0310` 正式名は [未確定]（スロット 0・文字列 0）。
6. **ess 依存で解析せず**（#82 待ち）: E1=N8 Δ 中身 ／ E2=+9・tail byte3 実値 ／ E3=v317§7 の bit3/bit7 実在面 ／ E4=45BB30 副作用の動的観測。
7. 新規残件 10（v319 §6・**find_head 誤答 5 例の体系化**ほか）→ #87。docs/旧記載との不整合 5 件は v319 §10。
