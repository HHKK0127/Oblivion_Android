# v251_process_havok_residuals11.md — process/Havok 残点第11束（V-249-11）

- 対象: `Oblivion.exe`（Steam v1.2.0416）／出典: `tmp/re/v249_process_havok_residuals10.md` §6-2 の新規残点 6 件
- 分析担当サブエージェント（TODO serial 244 / V244・低）。docs/GT/TODO/ソース無編集・訂正はドラフトのみ
- 環境: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、VA=off+0x400C00/.text、0x400E00/.rdata、0x401400/.data、bss=None）、Havok 帯 `0x84xxxx–0xA2xxxx`
- selftest: 着手時 `probe_harness.py selftest` = **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl → ALL PASS）
- 方針: 各件を「解明 / 部分解明 / 未解明」で裁定。**「確認できず（探索したが見つからない／取得失敗）」と「不存在証明（全数走査で 0 件＝構造上あり得ない）」を区別**。一次資料は Havok 5.1.0r1 公開ソース（`sigmaco/havok-v5.1.0r1-b2007.09.19`）。v249 既定の確定事項（map owner=遅延シングルトン・`0x34`/FLT_MAX/π は非 slot・'snap'=m_userTag・node2=hkWindowsSystemClock・node3=hkProcessFactory）は再検証せず前提として引用する。
- 中断対策: **1項目終えるたびに即追記**（§1→§6 の順）

## 対象 6 件（v249 §6-2）

1. B の3セクション名が Havok 4.0 正式名かの最終確認（低）
2. descriptor `+8` フラグの意味（低）
3. 静的登録ノード群（`0xA15980` 等）の CRT 初期化順（低）
4. `0x34`/FLT_MAX/π が属するクラス（静的メンバ）の同定（低）
5. node4/node5 のクラス名（hkArray<{handler,tag}> 保持クラス）（低）
6. hkProcess vtable のスロット順（低）

---

## §1 item1（V-249-11-1）B の3セクション名が Havok 4.0 正式名かの最終確認 — **部分解明（維持＋強化）: 4.0 期の公開資料・実 .hkx・コードリークいずれからも `__classindex__`/`__dataindex__` の一致例は 0 件。『4.0 正式名』は確認できず（≠不存在証明）。`__classnames__` は 4.5〜2010 期資料でも正式名として連続確認**

### 証拠A: Web 完全一致（本バッチ 5 クエリ、v249 の 5 クエリに追加）

| # | クエリ | 手段 | 結果 |
|---|---|---|---|
| 1 | `"__dataindex__" havok` | DuckDuckGo（webfetch） | **0 件** |
| 2 | `"__classindex__"`（havok 語なし・全 Web） | DuckDuckGo | **0 件**（v249 の `+havok` 版 0 件より厳格） |
| 3 | `"__dataindex__"`（全 Web） | Bing | **0 件**（v249 と再現一致） |
| 4 | `"dataindex" "classindex" havok hkx`（underline 除去系） | DuckDuckGo | **0 件**（Havok 無関係の echarts ヒットのみ） |
| 5 | `Oblivion "hkx" havok 4.0 format sections` 系 | DuckDuckGo + 一次ページ精読 | 4.0 期固有資料は**該当なし**。ヒットした Bethesda Modding Library（baddogskyrim）は **hk_2010.2.0-r1（Skyrim 期）**の wire format 節で、セクションは **`__classnames__`/`__types__`/`__data__` の 3 個**（ページ表にサイズ・用途まで明記）。`__classindex__`/`__dataindex__` は登場しない |

- `grep.app`（コードリーク含む全文検索）は本バッチでも **429＝取得失敗**（不存在証明ではなく未確認）。
- → 4.0 期の一次資料（SDK 文書・ヘッダ）は公開されておらず、**流通する hkx 解説は 4.5/2010 期（標準3名）に閉じる**。2 名を支持する材料は 0、同時に 2 名を「4.0 で正式だった」と示す材料も 0。

### 証拠B: 実 .hkx（Havok 4.0.3 書出しのゲーム配信物）による検証を試みたが遂げられず（本環境の事実）

- ローカルに Steam 版 Oblivion 実インストール（`C:\Program Files (x86)\Steam\steamapps\common\Oblivion`）が存在。loose `.hkx` は**ゼロ**（全ディレクトリ再帰走査）。
- `.hkx` は `Oblivion - Meshes.bsa`（**BSA v103**、ver=103・dirs=36・files=1927）側に格納される構成だが、v103 は**ファイル名ハッシュ索引**（ディレクトリ名のみ平文）で、平文域に `.hkx` 拡張子は 1 件も出現せず（`.nif` は 8,033 件＝NIF 名平文域と推定）。**ファイルデータ部のブロックオフセット表を本バッチのタイムボックス内で解読できず、展開検証は未実施**（候補 stride/order 8 組み合わせ×先頭 60 エントリの zlib 解凍成功率が全組合せ 0/60＝レイアウト前提が不正）。
- 平文文字列域への `__classnames__`/`__data__`/`__classindex__` 等の直接検索も 0 件（ファイル内容が zlib 圧縮のため**この検索は否定的根拠にならない**——単に平文で存在しない）。
- → 「実 .hkx のセクションテーブルに 2 名が現れるか」は **確認できず**（ローカル検証不可として明記）。

### 証拠C: 版間系統の再確認（v249 §3 の継続・再掲）

- 5.1 公開 SDK の SectionTag は `__data__`/`__types__`（hkPackfileWriter.h）＋ `__classnames__`（hkBinaryPackfileWriter.h）の 3 個で閉じる（**公開 API 定義の構造的負証拠**）。
- 本 EXE の writer B は version=2（5.1 は 4）の旧世代で、3 名 `__dataindex__`(0xA9C960)/`__classindex__`(0xA9C970)/`__classnames__`(0xA9C980) を書く。**標準 2 名（types/data）が B で 2 名（dataindex/classindex）に置換され、`__classnames__` だけ同名残存**という差分構造は不変。
- Bethesda Modding Library（証拠A-5）が示す通り、**4.5〜2010 期の実ファイルも標準 3 名**。つまり「2 名が正式だった時期がある」とすると 4.0〜4.5 の間で名称が完全回帰したことになり、不自然（旁証・確度: 中）。

### 裁定

> - **`__classnames__` = 4.0 期も 2010 期も通じた正式名**（5.1 ソース一次資料＋ Skyrim 期実ファイル記述の両立で確度: 高）。
> - **`__classindex__` / `__dataindex__` = 4.0 期の正式名かどうかは「確認できず」**（確度: 低のまま）。理由: 4.0 SDK/文書は未公開、Web 完全一致 0 件（4 クエリ）、grep.app 取得失敗（429）、ローカルの実 .hkx（BSA 内）は索引構造の解読が今回未了で検証不能。**「4.0 で正式だった証明」も「正式でなかった証明」も成立せず**。
> - 前回 v249 の「4.0 固有断定はしない」を維持。追加された知見は (i) underline 除去系のクエリでも 0 件、(ii) 2010 期実ファイルでも標準 3 名＝**2 名の支持材料は版を跨いで皆無**、(iii) ローカル実測手段（BSA）の限界が明確になったこと。
> - 将来の決着手段（残点化）: ①BSA v103 ファイル表オフセットの解読（pyffi 等既知ライブラリで可能な領域）、②4.0 期ツール（HavokMax 4.0.3）の出力サンプル入手、③Havok 4.0 SDK の非公開ソース発見。

**判定: 部分解明（確認できず＝4.0 正式名説は未証明のまま、負証拠のみ増加）**（証拠: 本 md §1 表、`v251_hkx_scan*.py` / `v251_bsa_*.py` 出力、v249 §3 / v247 §2）

---

## §2 item2（V-249-11-2）descriptor `+8` フラグの意味 — **部分解明（機械面は確定）: descriptor `{+0 'snap', +4 'Snap', +8 u8=1}` は `0x90D960` が組む Options 構造（0x4C B）の先頭ポインタ経由で渡り、`+8` の読手は EXE 全体で `0x90E88C: mov al,[edx+8]`（0x90E860 内）の 1 箇所だけ。test→je が「エクスポートセット（`[0xBA8788]` 照合）収集ブロック（0x90E897..0x90EB9x）」の ON/OFF。意味名の SDK 対応は 5.1 Options と 1:1 にならないため推定どまり**

### 証拠A: descriptor → Options(temp) → 唯一読手の鎖（`v251_r2_out.txt` / `v251_r2b_out.txt`）

```
0x8BC540（ゲーム側スナップショット入口）
  ├─ 0x8BC5F3: mov byte [esp+0x14], 1      ; desc+8 = 1（u8）
  ├─ desc = [esp+0xC..0x14] → push → call [edx+0x10]（B slot4 = 0x90EF80）
  ├─ 0x90EF80: esi=[esp+0x6C]=desc → push desc → 0x90D960（temp 0x4C B 構築）
  │     0x90D960: temp[0] = desc ptr
  │     0x90D9A3: al = [desc+5]（'Snap' の 2 文字目 'n'）
  │     0x90D9AC: cmp cl, al / setne dl     ; cl = [0xB2FDE5]（グローバル key の byte+1）
  │     0x90D9B2: temp[0x48] = dl           ; 「key が前回と違う」差分フラグ
  ├─ 0x90EF80 後続: 0x90D9C0（section テーブル書込）→ 0x90DA40（ヘッダ）→ 0x90DC60（0xFF padding）→
  └─ 0x90F02D: call 0x90E860   ← **temp を受ける唯一の e8 呼出し（e8 走査で 1 件确认）**
        0x90E872: al = [temp+0x48]（差分フラグ → 0x9183A0 の {vt 0xA9D1B8,+8,[+0xc]=flag} オブジェクトへ）
        0x90E88A: edx = [temp+0]  = descriptor ptr
        0x90E88C: mov al, [edx+8]        ← **descriptor+8 の全 EXE 唯一の読み出し**
        0x90E88F..: test al,al / je 0x90EBA1   ; 0 なら収集ブロック全体をスキップ
```

- `0x9183A0` は `hkRef<0xA9D1B8>` ラッパ（+0xc に差分フラグ byte、+8 に `[temp+0x1C]`、refcnt++）。→ **`temp+0x48` の「差分フラグ」と `descriptor+8` は別物**（前者は `desc[5]` 対 `[0xB2FDE5]` の比較値、後者はゲームが常に 1 を置く u8）。v249 §4 の「`+8`=1 はフラグ/版（確度低）」が**どちらのフラグか**を分離した。

### 証拠B: `+8` がゲートする内容（`v251_r2b_out.txt`(a) / `v251_r2c_out.txt`(d)）

- ON 分岐（`+8 != 0`＝本 EXE の実挙動）: `[ebp+8]` 配列（stride **0x18** のエントリ）を走査し、`[ebx+4]`（hkClass ptr）が **`0xBA8788`** と等しいものだけ、`temp[0x10]` の **stride 8 ペア配列** {ptr, size} を複製してローカル配列へ（`0x8A6E40`=reserve / `0x8B1550`=index 検索 push -1/-2）。OFF 分岐（0x90EBA1）は配列を素走査して別経路の索引を作る。
- `0xBA8788` は hkClass グローバル（`0x9461CE` が `push 0` + `push 0xBA8788` で `0x918460` に渡す＝型索引系。v247 §3 の contents 型 `hkRootLevelContainer`/`hkPhysicsData` 帯と同群）。
- → **`+8` の機能 = 「ルートコンテナ（0xBA8788 クラス）向けのエクスポートセット／ポインタサイズペア収集を有効化」**。値は常に 1 で、`0x90E860` の呼出し元は 1 箇所のみ（e8 全走査）＝**OFF 分岐は本 EXE の生成経路では到達しない死分岐**。

### 証拠C: グローバル `0xB2FDE4`（key 格納）の系統（`v251_r2b_out.txt`(d)(e)）

- `u32@0xB2FDE4 = 0x01000004`（初期 .data 値）、refs = dword 側 6 件 / byte+1 側 5 件。読手: `0x90BB90`（desc+4 の初期化）／`0x942D92-DC8`（`[0xB2FDE4]` の byte0..3 と候補 object の +0xc..0xf を逐 byte 比較）／`0x953251-64`（`0x90BBA0` で 2 回コピーして `0x953530` へ）／`0x9461DE..0x946205`（byte0..2 を順に `0x9181B0` へ push＝型索引書式）／`0x90EED9`／`0x95841C`（`[ecx+1]` 対 `[0xB2FDE5]` の setne ＝ temp+0x48 と同型パターン）。
- → 「前回 key との差分検査」が **2 箇所（`0x90D960` と `0x95841C`）に反復**しており、descriptor+5 比較が**キャッシュ無効化判定**である構造と整合。

### 裁定

> - **確定（高）**: descriptor+8 は意味を持たない定数ではなく、`0x90E88C` で読まれる**機能的 ON フラグ**（エクスポートセット収集ゲート）。v249 §6-2 が懸念した「temp+0x48 の差分フラグとの関係」は**別系統**と切り分け完了（temp+0x48 は比較結果の出力、descriptor+8 は入力側 u8）。
> - **SDK 対応（低）**: 5.1 `hkPackfileWriter::Options` は {sectionDataAlignment, fileDataAlignment, floatFormat, compressFormat, typePtrAdjustment, userTag, …} で、**「u8=1 単独」のフィールドは存在しない**。本 temp（0x4C B）も 5.1 Options とサイズ・並びが一致しない。→ 4.0 期の Options 構造（未公開）由来の独自フラグと見るのが整合的で、意味は「exportSets／ルート変種収集 有効化（機能名）」どまり。**正式フィールド名の同定は未解明（4.0 ソース不在＝確認できず）**。
> - 不存在証明（構造的 0 件）: descriptor+8 の読み出しは `0x90E88C` を含め**全 .text で他に 0 件**（0x90EF80 全 0x5D9 B・0x90D960・0x90D9C0・0x90DA40・0x942D70・0x953530 の全文字走査で [reg+8] 型 read のうち descriptor 由来は該当なし）。

**判定: 部分解明（読手・ゲート先・分岐到達性=確定／SDK 上の正式フィールド名=未証明）**（証拠 `v251_r2_out.txt` / `v251_r2b_out.txt` / `v251_r2c_out.txt`、v249 §4 継続）

---

## §3 item3（V-249-11-3）静的登録ノード群（`0xA15980` 等）の CRT 初期化順 — **解明: 9 登録関数は .rdata の単一の `_initterm` 配列（`0xA283D0..0xA2F77C`、5,579 スロット）に昇順で並び、実登録順は「0xB2EB0C → 0xB2F1B4 → 0xB30064 → 0xB30118 → owner(0xB30598) → node2 → node3 → node4 → node5」。実行主体は VS6 CRT の `__DllMainCRTStartup` 相当（`0x981C6B..0x981CFC`、`0x981CB4` のインライン走査ループ）＝ WinMain 前。walker の呼出し元（`0x8BBB5F` の上位）は `0x8BBA80`（遅延 boot、guard=`[0xBA8040]`）→ 単一 caller `0x88B0F7`（in `FUN_0088B070`）→ `FUN_005350F0+0x39`**

### 証拠A: 登録関数 9 件の正確な入口とテーブルスロット（`v251_r3_out.txt` (a)(b) / `v251_r3b_out.txt` (a)(e)）

- 9 関数の実体はいずれも 15 B の同一定型（`mov eax,[0xBA8188] / mov [node+4],eax / mov [0xBA8188],node / ret`）で、入口は **0xA12340 / 0xA126E0 / 0xA13C60 / 0xA13D40 / 0xA15980 / 0xA159D0 / 0xA159F0 / 0xA15A10 / 0xA15A30**（先頭 4 つは v249 の表し書き 0xA12344 等ではなく命令先頭が 0xA12340 系＝imm 照合時に 4 B ずれていると参照を見逃す罠。本バッチで命令先頭を逆アセンブルで確定）。
- 各入口の imm32 参照は**それぞれ .rdata の 1 スロットのみ**（コードからの e8/imm32 なし＝関数ポインタ経由でしか呼ばれない CRT 初期化子の形）:

| 実行順(=スロット地址順) | スロット | 登録関数 | ノード | slotptr(生成先) |
|---|---|---|---|---|
| 1 | `0xA2F218` | `0xA12340` | `0xB2EB0C` | `0xBA7DA0` |
| 2 | `0xA2F27C` | `0xA126E0` | `0xB2F1B4` | `0xBA803C` |
| 3 | `0xA2F400` | `0xA13C60` | `0xB30064` | `0xBA8450` |
| 4 | `0xA2F414` | `0xA13D40` | `0xB30118` | `0xBA84F4` |
| 5 | **`0xA2F63C`** | **`0xA15980`** | **`0xB30598`**（owner） | `0xBA94BC` |
| 6 | **`0xA2F644`** | **`0xA159D0`** | **`0xB305A4`**（node2=hkWindowsSystemClock） | `0xBA94E8` |
| 7 | **`0xA2F648`** | **`0xA159F0`** | **`0xB305B8`**（node3=hkProcessFactory） | `0xBA950C` |
| 8 | **`0xA2F64C`** | **`0xA15A10`** | **`0xB305E8`**（node4） | `0xBA9510` |
| 9 | **`0xA2F650`** | **`0xA15A30`** | **`0xB305F4`**（node5） | `0xBA9514` |

- 5〜9 は**連続スロット**（0xA2F63C..0xA2F650）。隙間の 0xA2F640 は `0xA159A0`（この関数は `0x90D190` 系の別登録でノード登録ではない）。1〜4 と 5〜9 の間（0xA2F218→0xA2F63C で約 161 スロット）は同帯の他静的初期化子で、**同一リンカ領域（0xA12340〜0xA15A30 の関数アドレス順＝スロット順）**に並ぶ＝ Havok 帯の初期化子がリンク順どおり積まれた形。

### 証拠B: テーブルの全域と走査主体（`v251_r3_out.txt` (b)(c) / `v251_r3b_out.txt` (b)(c)）

- 走査主体は **`0x981CB4..0x981CD2`** のインラインループ:
  ```
  0x981CB4  mov esi, 0xA283D0      ; __xi_a（imm32 だが refs 検出は命令内即値のみ）
  0x981CBB  mov edi, 0xA2F77C      ; __xi_z（imm32 refs = 0x981CBC の 1 件）
  0x981CC5  mov eax,[esi] / test eax,eax / je skip / call eax / add esi,4 / jb loop
  ```
  → **NULL スロット（0）をスキップしつつ `[0xA283D0, 0xA2F77C)` を昇順に 1 周 = VS6 CRT `_initterm` のインライン化**。配列全長 22,316 B = **5,579 スロット**（0xA2D1DC と 0xA2F77C に NULL、両端も 0xA2F77C/0xA2F780 が NULL で閉じる＝`__xi_a..__xi_z` の単一セグメント）。
- その直前（`0x981C93..0x981C9D`）に第 2 配列 `push 0xA2F7A4 / push 0xA2F780 / call 0x981BD8`＝ **`_initterm_e`（9 スロット、0xA2F780..0xA2F7A0）**。中身は `0x981F49/0x98524C/0x98BAF6/0x98C763/0x98FC88/0x99CB37/0x980E08/0x99286C` の 8 個（+終端 NULL）＝CRT ランタイム自身（`_initterm_e` 形式の dynamic init）で、**Havok ノードは含まれない**。
- 外側の関数 `0x981C6B..0x981CFC` の構成（`call 0x98D805` → `_initterm_e` → `call 0x981FB4` → `_initterm` ループ → `[0xBABC18]`（DllMain ベクタ）呼出し）は **VS6 の `__DllMainCRTStartup`（attach パス）そのもの**。v247 §1 の起動鎖（`0xBB097D jmp eax` → `0x98787C` → `0x98769C WinMainCRTStartup`）との順序関係は **CRT attach（静的初期化＝本 9 件）→ WinMain → 本 walker**（後述）。
- 9 ノードの**登録順＝スロット順**で、ノードの `next`（+4）連結は先頭挿しなので**リスト最終形は逆順**（head=0xB305F4→0xB305E8→0xB305B8→0xB305A4→0xB30598→…→0xB2EB0C）。walker は list 順に walk して各 slotptr を 0→実体へ満たすため、**生成順（初回 walker 実行時）= node5 → node4 → node3 → node2 → owner → 0xB30118 → 0xB30064 → 0xB2F1B4 → 0xB2EB0C**。依存: 各 create() は hk アロケータ `[0xBA7D98]` に依存するが、node1（`0xBA7DA0`）がそのアロケータ/ハブ系で先（9 番目）に生成される— walker 実行前に boot 関数（証拠C）がアロケータを別経路で初期化済みのため整合（順序は機能上不変）。

### 証拠C: walker 呼出し元と boot ガード（`v251_r3c_out.txt` (c) / `v251_r3e_out.txt`）

- `e8(0x8BB420) = {0x8BBB5F}` の 1 件（v249 と再現一致）。`0x8BBB5F` を含む関数は **`0x8BBA80`**:
  - 先頭で guard `[0xBA8040] != 0` なら即 exit（単発 boot）。引数 esi（NULL なら exit、返り値 1）。
  - `call 0x8BA9C0`（状態取得）→ `0x8A70F0(esi)` → 0x330 B オブジェクト生成（`0x401F00`＝CRT new）→ `0x8A7260`（ハブ登録）→ **`[0xBA7D9C]` に 0x330 B 系を格納**（NULL なら 0）→ **`[0xBA7D98]` の hk アロケータ経由 alloc(8, tag0x15)**→ vptr `0xA98240` を持つ 8 B オブジェクトを `0x534070` へ → alloc(0x28, tag0x15)→ **`0x8BB560` ctor（引数 2 個保存）** → `0x534020`（登録）→ **`0x8BBB5F: call 0x8BB420`（＝walker 初回実行＝9 生成）** → `[0xBA803C]`（=1 ばん目の別系ノード singleton）の `call [vt+8]`（slot2）→ **`[0xBA8040]=1`** を書き return 0。
  - `[0xBA8040]` の書き手は **`0x8BBB6F` と `0x8BBC55` の 2 件だけ**（C6 走査。=1 は本 boot の成功、`0x8BBC55` 側は uninit/リセット経路と同一関数帯）。
- `e8(0x8BBA80)` は **1 件（`0x88B0F7`、in `FUN_0088B070`＝ SEH プロローグ `push -1/push 0x9D62EB` のゲーム側関数。先頭で同じ `[0xBA8040]` を検査）**。その上流 `e8(0x88B070)` も **1 件（`0x535129` = `FUN_005350F0+0x39`）**。→ 起動鎖: **CRT attach（9 登録）→（ゲーム側）`FUN_005350F0` → `FUN_0088B070` → `0x8BBA80`（初回のみ walker 実行）**。

### 裁定

> - **CRT 初期化順 = 確定（高）**: 9 登録関数は VS6 `_initterm`（インラインループ `0x981CB4`）で `.rdata 0xA283D0..0xA2F77C` 昇順に実行。表内位置は上表（連続 5 スロット 0xA2F63C..0xA2F650 に owner→node2→node3→node4→node5、その前に別系 4 ノード）。
> - **呼出し元（上位鎖）= 確定（高）**: walker 起動は CRT Attach 時ではなく **ゲーム初期化の `0x8BBA80` 初回実行時**（guard `[0xBA8040]`）。`e8` 全走査で 1 件鎖（`0x535129→0x88B0F7→0x8BBB5F`）＝**到達経路は単一**。
> - **順序の機能解析（中）**: 先頭挿しのリスト連結のため**生成順は登録順の逆**（node5→…→0xB2EB0C）。create 間にデータ依存は無く（各自 alloc+ctor のみ）、順序の可観測な影響は登録順の証明と同じくらいの実務的重要性（＝順序自体に意味はない／機構の帰結として確定）。
> - 残（低）: 配列 0xA283D0..0xA2F77C の全 5,579 スロットのうち本帯に属さない多数はゲーム側 C++ 静的初期化子で、**Havok 関連スロットの全数列挙は未実施**（9 件は特定済み）。`0x8BBC55` の uninit 経路の呼び主（`0x8BB990` teardown の上位鎖）は本バッチでは未追跡（teardown walker の e8=1 件 `0x8BBB9D` まで確認）。

**判定: 解明（CRT 順・テーブル実体・起動鎖の 3 点確定）**（証拠 `v251_r3_out.txt` / `v251_r3b_out.txt` / `v251_r3c_out.txt` / `v251_r3d_out.txt` / `v251_r3e_out.txt` / `v251_r3f.py` 出力）

---

## §4 item4（V-249-11-4）`0x34`/FLT_MAX/π が属するクラス（静的メンバ）の同定 — **部分解明（大幅進展）: 3 語は整数ではなく FLOAT の三つ組 `{FLT_EPSILON, FLT_MAX, π}`（+二つ組版×4）で、`hkDisplayGeometry` 家系（Visualize/Shape）の TU 帯に静的コンスタントとして並置と確定系。名前一つ（どのクラスのどのメンバか）は全地址 imm32 refs 0 のため機械的同定不能＝未解明（確認できず）**

### 証拠A: 値の再分類と帯の全構造（`v251_r456_out.txt`(a)）

- `0x34000000` を float と読むと **1.19209e-07 = FLT_EPSILON**（v249 の「0x34=52」は整数読み。`0x7F7FFFFF`=FLT_MAX、`0x40490FDB`=π と並び「epsilon / 最大実数 / 円周率」の組）。
- 帯 0xAA2A40..0xAA2C40 を全 dword 分類すると、三つ組/二つ組の出現は次の 6 箇所（**すべて vtable 同士の隙間**＝v249「実 slot 数 2、定数は slot ではない」を再確認・拡張）:
  | 定数ブロック | 内容 | 直後 vtable | 直後 vtable を ctor で書く関数 |
  |---|---|---|---|
  | `0xAA2B94..B98` | {ε, π} | —（0x982D85 列の末尾） | hkProcessHandler 系 6 純粋仮想 vt（0xAA2B44 帯） |
  | `0xAA2BC4..BCC` | {ε, MAX, π} | `0xAA2BD0`（node4/5 クラス） | `0x94A60B`（遅延 ctor） |
  | `0xAA2BD8..BE0` | {ε, MAX, π} | `0xAA2BE4` | `0x94B85E`/`0x94A53E`（0x94B84x ctor、free tag **0x32**） |
  | `0xAA2BFC..C00` | {ε, π} | `0xAA2C04` | `0x94D2A7` |
  | `0xAA2C24..C28` | {ε, π} | `0xAA2C2C` | （0x94D752/0x94D7C9 が `0xAA2C14` を書く帯） |
  | `0xAA2C3C..` | {ε, π} | 次帯 | — |
- 隣接 vtable のスロット群は「D2 dtor（free tag 0x32/8）+ 0x68F970 + 0x94CD00/0x94D430/0x94D510/0x94D7F0/0x94DE80（hkArray を増やして `[obj+0x50]` のハンドラ配列へ push する幾何生成関数）」＝ **node4/node5 サーバ帯（0x94A630/0x94BE00 dispatcher の consumer 側）と同じ TU 群**。

### 証拠B: 名前テーブルが `hkDisplayGeometry` 家系を指す（一次資料 2 本）

- `.rdata 0xAA2A60..0xAA2B0C` の 9 文字列は `HK_DISPLAY_PLANE/CONVEX/SEMICIRCLE/CONE/AABB/BOX/CAPSULE/SPHERE/RECTANGLE`（`v251_r456_out.txt`(a) で RECTANGLE を新規確認）、`.data 0xB305C4..0xB305E4` の 9 ポインタ配列は **RECTANGLE→SPHERE→CAPSULE→BOX→AABB→CONE→SEMICIRCLE→CONVEX→PLANE の逆順格納**（ポインタ列先頭 0xB305C4=0xAA2AF8=RECTANGLE）。
- 5.1 `Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h`（本バッチ取得）:
  ```cpp
  enum hkDisplayGeometryType {
      HK_DISPLAY_RECTANGLE = 0, HK_DISPLAY_SPHERE, HK_DISPLAY_BOX, HK_DISPLAY_AABB,
      HK_DISPLAY_CONE, HK_DISPLAY_SEMICIRCLE, HK_DISPLAY_CONVEX, HK_DISPLAY_PLANE,
      HK_DISPLAY_CAPSULE, HK_DISPLAY_CYLINDER };
  ```
  ＝ **EXE の 9 名は 5.1 enum の CYLINDER を除く 9 項と完全一致**（4.0 期 enum=9 項 → 5.1 で CYLINDER 追加の差分まで一致）。`Shape/hkDisplayGeometry.h`（同取得）は `class hkDisplayGeometry : public hkReferencedObject`、`HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_DISPLAY)`、純粋仮想 2（getWireframeGeometry/buildGeometry）＝ 帯内の隣接 vtable（{D2, 0x68F970, 実体, 実体…}）と同型。
- → 定数ブロックの置く帯＝**hkDisplayGeometry 派生（Sphere/Box/Cone/Capsule…）各 .cpp の .rdata 列**で、`{epsilon, HK_REAL_MAX, HK_PI}` は**各派生 builder の既定値/丸め定数**（メッシュ细分の epsilon・「無限大」FLT_MAX・半円/円錐の π）と機能一致（推定・確度: 中）。

### 証拠C: 同定不能の機械的理由（不存在証明との区別）

- ブロック全 10 語（`0xAA2B94/BC4/BC8/BCC/BD8/BDC/BE0/BFC/C00/C24/C28/C3C/C40`）の **imm32 参照は .text/.rdata/.data 全域で 0 件**（v249 §2 + 本バッチ r3(b)/r456 で再現）。x87/SSE の `fld/fmov [abs]` は imm32 を持つため、**参照が存在すれば imm32 走査で必ず捕捉される**——0 件＝コードからの绝对参照が本当に無い（**構造的負証拠＝存在しないことの証明ではなく「読手ゼロ」は確定**）。
- RTTI/COL 不在（vtable-4 が文字列残骸）で名前の機械取得は不可。5.1 リポジトリの `*.cpp` は 168 B のスタブ（実体なし）のため、`.cpp` 側の `static const` メンバ名の照合元が存在しない（**取得素材の欠如＝確認できず**）。

### 裁定

> - **確定（高）**: 三つ組は float `{FLT_EPSILON, FLT_MAX, HK_PI}`（整数 52 の解釈は誤り）。表示ジオメトリ・ビルダ家系（5.1 `Common/Visualize/Shape` の hkDisplayGeometry 派生群）の TU 帯に、vtable 間へ並置された**未参照静的コンスタント**。enum 名 9 語が `hkDisplayGeometryTypes.h` と CYLINDER 差分まで一致し、家系の同族性は強い旁証。
> - **未解明（名前一つ）**: どの派生クラスのどの静的メンバかの一点は、読手 0・RTTI 0・4.0 ソース不在のため**この EXE からは原理的に特定不能**（「確認できず」）。将来手段: 4.0 期 hkDisplaySphere/Cone 等ヘッダの実物入手、または他 4.0 系 EXE（FNV 等）の同帯定数と読手付き実装の照合。
> - v249 §2 の「意味推定（頂点上限 52・FLT_MAX・半回転 π）」は**整数読みを伴うため本バッチで置換**（epsilon 読みへ訂正。ドラフト §7-2 参照）。

**判定: 部分解明（float 三つ組化・家系 hkDisplayGeometry まで確定／具体的メンバ名は特定不能）**（証拠 `v251_r456_out.txt` / `v249_r3_out.txt`、SDK 5.1 `Shape/hkDisplayGeometryTypes.h` `Shape/hkDisplayGeometry.h`）

---

## §5 item5（V-249-11-5）node4/node5 のクラス名 — **部分解明: クラス形状・生成・使われ方は完全確定。新規の機械確定＝「両配列への書込み手が EXE 全体に 0 件＝登録表は常時空、走査は空ループ」。機能名は「表示コマンド動的最適登録（{handler, commandId}）の遅延シングルトン×2」で、SDK 正式名は 5.1 に候補型が存在せず未証明（確認できず）**

### 証拠A: singleton アクセス経路の全数（`v251_r456_out.txt`(e)）

- `refs(0xBA9510) = {0x94B71A, 0x94B75A（node4 dispatcher の読取 2）, .data@0xB305F0（ノード slotptr）}`、`refs(0xBA9514) = {0x94CB86, 0x94CBCA（node5 dispatcher）, .data@0xB305FC}`。
- → 実体（0x14 B クラス）へ到達可能なコードは **両 dispatcher の読取のみ**。walker（初回生成・§1/§3）以外に**生成経路なし**、かつ **hkArray（+8/+0xC）への書込みコードは 0 件**（appera 系 generic append は obj 指针を受け取れない＝到達経路が存在しない）。
- **したがって node4/node5 の配列は生涯空**（size=0 で初期化され増える経路が無い）。default 分岐の stride-8 走査は実行されても 0 反復＝**登録専用・未使用の機構**（本ビルドでは登録側が剥がされている。5.1 の `HK_VISUALIZE` コンパイルオプション系で登録 .cpp が除かれた形と整合・推定）。

### 証拠B: クラス形状（v249 §2 からの確定事項の再掲＋新規）

- `{vptr 0xAA2BD0（2 slot: D2 dtor 0x94BBC0 + 0x68F970）, u16 size=0x14, u16 refcount, hkArray<{fn,id}> +8..+0x10, flags 0x80000000}`、tag 0x15・遅延シングルトン（§1/§3）。
- エントリの使い: dispatcher default で `[esi]`（handler fn ptr）を 4 引数 push + `call`、`[esi+4]`（id）を比較。**id 側は §6 で「ストリーム obj slot2=command 取得」の返り値と一致照合**＝ {commandId → handler}。
- 5.1 公開ソース（Visualize ディレクトリ全 listing 取得済み・本バッチ）に **同型の「{関数,id} 静的 registry」クラスは存在しない**（Visualize/Process の 3 クラス＝hkDebugDisplayProcess/hkInspectProcess/hkStatisticsProcess はいずれも process サブクラスで registry でない）。最上位の型は §6 で触れる `getConsumableCommands/consumeCommand` コマンド機構（5.1 hkProcess.h）の **4.0 版レジストリ実装**と機能対応するが、正式名（`hk???Registry` 等）は**確認できず**（4.0 ソース不在＝カテゴリの未確認であり、5.1 に同名型の無いこと自体は構造的負証拠として記録）。

### 裁定

> - **確定（高）**: node4/5 は「表示サーバ 2 系統（node4=FUN_0094A630／node5=FUN_0094BE00）用の **{コマンド ID→ハンドラ} 動的登録表**」の遅延シングルトンで、**本 EXE では登録者が無く常時空**（=構造的負証拠: dispatcher 以外に array 書き込み経路 0、単一の到達経路全数走査で確認）。
> - **クラス名（SDK 正式名）= 未解明（低のまま・確認できず）**: 5.1 に同型クラスなし・RTTI なし・4.0 ソース非公開。機能記述名「DisplayCommandRegistry（仮称）×2」を推奨表記とする。

**判定: 部分解明（形状・到達・常時空=確定／正式名は未証明）**（証拠 `v251_r456_out.txt` / `v249_r5/r11_out.txt` / SDK 5.1 Visualize+Process/Shape ディレクトリ listing）

---

## §6 item6（V-249-11-6）hkProcess vtable のスロット順 — **解明（本 EXE 実測で確定・v249 の仮同定は誤り）: hkProcess 家系は「dtor 1 slot（D2）+ 5.1 と同一宣言順」で getProcessTag=slot1(+4)。よって dispatcher の `call [eax+8]`（slot2）は getProcessTag ではなく、呼出し対象も process ではなく update 第 1 引数の入力ストリーム系オブジェクト（返り値 2..25 = コマンド ID）。5.1 一次資料で宣言順 `~dtor, getProcessTag, getConsumableCommands, consumeCommand, isOk, init, step` を確定**

### 証拠A: 5.1 hkProcess.h（一次資料・全文取得済み）

- `Source/Common/Visualize/hkProcess.h`: `class hkProcess`（**hkReferencedObject 継承ではない**）:
  ```cpp
  hkProcess(hkBool selectable);
  virtual ~hkProcess() {}
  virtual int getProcessTag() = 0;          // 純粋
  virtual void getConsumableCommands(hkUint8*& commands, int& numCommands) {...} // 空実装(2 out引)
  virtual void consumeCommand(hkUint8 command) { }        // 空・1 引数
  virtual bool isOk() { return m_outStream && m_outStream->isOk(); }
  virtual void init() { }                    // 空・0 引数
  virtual void step(hkReal timeElapsed) { }  // 空・1 引数
  ```
  メンバ列は `m_selectable, m_inStream, m_outStream, m_displayHandler, m_processHandler`（5×4 B ＋ vptr = 0x18）。

### 証拠B: 本 EXE の実測スロット位置（`v251_r6c_out.txt` / `v251_r6d_out.txt`）

- 17 本の `registerProcess`（`0x947C50`）呼出し直後すべて `mov [tag_世界], eax`（返り値=タグ保存。保存先 16 グローバル `0xBA8410..0xBA8460 / 0xBA9500..0xBA9508` を全数列挙）。
- 各タグの世界の**読手に単独関数 `mov eax,[tag]; ret`（getProcessTag 実体）が存在**し、その vtable セル位置:
  | クラス（登録名） | getter | vtable | **slot** |
  |---|---|---|---|
  | ObjectInspection（tag `0xBA9500`） | `0x946450` | `0xAA2938` | **slot1（+4）** |
  | Statistics（`0xBA9504`） | `0x946D70` | `0xAA2964` | **slot1** |
  | Phantoms（`0xBA8448`） | `0x91CDD0` | `0xA9D6F0`（ctor 格納 vt） | **slot1** |
  | ConvexRadius（`0xBA842C`） | `0x91B510` | sub-vt `0xA9D510`（ctor `[esi+8]`） | **slot1** |
  | WorldMemory 帯（`0xBA8410`） | `0x918CF0` | `0xA9D24C..0xA9D264`（run=7） | 中部（ゲーム側派生は仮想追加で下流） |
  | SweptTransforms（`0xBA8420`） | `0x91AA70` | `0xA9D3F8..` | 同上（ゲーム派生は slot 追加） |
  | Shapes（`0xBA8438`） | `0x91BE10` | `0xA9D5A0..` | 同上 |
- hkProcess **base/中間** vt の並べ替え（`0xAA2BA4`＝ConvexRadius 系 base ctor `0x9491F0` が `[obj+8]` に書く副 bases vt）: `[0]=thunk→D2(0x9492A0)`, **`[1]=0x982D85=_purecall`（純粋 getProcessTag）**, **`[2]=0x918CD0`＝`mov [a1],0; mov [a2],0; ret 8`＝5.1 `getConsumableCommands` 空実装と完全一致**, `[3]=0x9266A0`（ret 4 空＝consumeCommand）, `[4]=0x918000`（ret＝isOk 4.0 版 `{}` と推定）, `[5]=0x9266A0`（ret 4 空＝step） — **dtor 1 slot + 5.1 宣言順**で全 slot が説明つく。
- **`0x982D85 = _purecall 本体と確定**（`push [0xBAA5EC]; call 0x98BEEB; … push 0x19; call 0x98D5B8; _XMSG 形式; abort` の CRT purecall パターン。`v251_r6_out.txt`(c)）。hkProcessHandler の 6 純粋仮想（5.1 ヘッダで registerProcess〜deleteProcess の 6 本＋dtor、**取得済み**）に対し `0xAA2B5C..0xAA2B70` の**連続 6 スロット=0x982D85** が本数まで一致。
- v249/v247 の「`0x68F970` slot1 = purecall 枠」表記は誤りで、**`0x68F970`=`ret 4` の共有空仮想実体、`0x982D85`=purecall** の二種併存が正（v249 §6-5 ドラフト 2 を補完）。

### 証拠C: dispatcher `call [eax+8]` の対象（`v251_r6_out.txt`(a)(b)）

- node4 dispatcher `0x94A630` 冒頭: `mov ebx,[ebp+8]`（=**第 1 引数オブジェクト**）→ `call [ebx_vt+8]` → `add eax,-2; cmp eax,0x17; ja default`。**push 0 個の thiscall で eax に整数を戻す slot2**。引数 2 本取り出しの getConsumableCommands（ret 8）や 1 本取り出し系ではスタック不整合となり、**呼出された側は必ず「引数なし int getter」**。
- 呼び出し鎖: process の update（例 `0x91ADB0`）が第 1 引数 O（`[O+8]`=キー、`[O+0x14]` 等を持つ入力ストリーム系）をそのまま wrapper（`0x94B7E0/0x94CC50`）へ渡し、wrapper→dispatcher の arg1 = **O**。node5 も同型（`mov edi,[ebp+8]; call [edi_vt+8]`）。
- → dispatcher の +8 = **O の「現在コマンド ID 取得」スロット**（2..25＝表示コマンド ID 空間。§5 の {handler,id} 表の id と同一空間）。**process の getProcessTag（+4）は呼出経路が有るが、それはサーバ登録側**（`0x946D71` 帯のように登録関数が tag を読む／`call [eax+4]` 系 thunk 群）。

### 裁定

> - **hkProcess 家系のスロット順（本 EXE・確定高）**: `[0]`=dtor（**scalar deleting 1 slot**。VS6 で配列 new 無効構成ならこの形）、`[1]`=getProcessTag、`[2]`=getConsumableCommands、`[3]`=consumeCommand、`[4]`=isOk、`[5]`=step/init 系の空実装群。**5.1 宣言順と dtor 1 slot 前提で 1:1**。
> - **「1 slot 版間差」懸念は解消**: v249 が getProcessTag と同定した `call [eax+8]` は**別オブジェクト（入力ストリーム系）の slot2 コマンド ID getter** で、hkProcess の +4 と競合しない。誤りを §7 訂正ドラフト化。
> - 残（低）: `[4]/[5]` の isOk/init/step の向きの完全割付（4.0 版の宣言順微差、`0x918000`=裸 ret を isOk と読む仮定）と、ゲーム側派生（0x91xxxx 帯）が vtable へ追加する仮想群の内訳は未追跡。4.0 と 5.1 で `isOk` の有無・並びが同じかは 4.0 ソース不在のまま（確認できず）。

**判定: 解明（EXE 実測でスロット順確定＋v249 仮同定の誤りを特定）**（証拠 `v251_r6_out.txt` / `v251_r6b_out.txt` / `v251_r6c_out.txt` / `v251_r6d_out.txt`、SDK 5.1 `Visualize/hkProcess.h` `Visualize/hkProcessHandler.h`）

---

## §7 まとめ・新規残点・自己検証・訂正ドラフト

### 7-1. 本バッチ（§1〜§6）の判定一覧

| # | 残点（v249 §6-2） | 判定 | 確度 | 主証拠 |
|---|---|---|---|---|
| §1 | B の 3 セクション名の 4.0 正式性 | **部分解明**（Web 5 クエリ+grep.app 429+BSA 実測不能＝**確認できず**。classnames 連続正式／2 名の支持材料は版跨ぎ皆無） | 低（4.0 点は不変） | 本 md §1 表、`v251_hkx_scan*.py`/`v251_bsa_*.py` |
| §2 | descriptor `+8` フラグ | **部分解明**（読手 1 箇所 `0x90E88C`＝エクスポートセット収集ゲート・ON 固定で分岐到達性確定／正式フィールド名未証明） | 機構=高・名前=低 | `v251_r2/r2b/r2c` |
| §3 | 静的登録ノード群の CRT 順 | **解明**（`_initterm` 配列 `0xA283D0..0xA2F77C` 昇順＝9 ノードの順序・`0x981CB4` 実体・起動鎖 `0x535129→0x88B0F7→0x8BBA80→walker`） | 高 | `v251_r3..r3f` |
| §4 | 0x34/FLT_MAX/π のクラス | **部分解明**（float 三つ組 `{FLT_EPSILON,FLT_MAX,HK_PI}`×6 ブロックへ再分類、家系=hkDisplayGeometry/Shape（enum 9 名一致）。メンバ名は機械的に特定不能） | 家系=中・名前=不能 | `v251_r456`、SDK 5.1 Shape 2 本 |
| §5 | node4/5 のクラス名 | **部分解明**（**常時空の {commandId→handler} 登録表**と確定（書込み手 0 件）。正式名は 5.1 同型不在・4.0 不在で未証明） | 形状/空=高・名前=不能 | `v251_r456`、5.1 Visualize listing |
| §6 | hkProcess vtable スロット順 | **解明**（dtor 1 slot + 5.1 宣言順。getProcessTag=**slot1(+4)**（4 クラスで実測）。**`call [eax+8]` は getProcessTag ではなく入力を渡すストリーム系 obj の slot2 コマンド ID getter**＝v249 仮同定の誤りを訂正） | 高（EXE 内）/低（4.0 版間） | `v251_r6..r6d`、5.1 hkProcess/hkProcessHandler.h |

**本バッチの新規確定事項（要点）**

1. 9 ノード登録の CRT 鎖: `_initterm` インラインループ `0x981CB4`（`__DllMainCRTStartup` 相当 0x981C6B 内、WinMain 前）。スロット順 §3 表の通り、**node2→…→owner ではなく「owner, node2, node3, node4, node5 が連続 5 スロット、他 4 ノードが前席」**。生成（walker）は起動関数 `0x8BBA80` 内で一括・リスト逆順（§3）。
2. descriptor `{snap, Snap, 1}` の 3 語目は「エクスポートセット（hkClass `0xBA8788` 照合）収集」の ON フラグで、読み出しは全 EXE で 1 命令のみ。temp+0x48 の「key 差分フラグ」（`desc[5]` vs `[0xB2FDE5]`）とは別物（§2）。
3. `{ε, MAX, π}` は float。hkDisplayGeometry 家系の 9 名（+RECTANGLE を含む）が 5.1 enum と CYLINDER 差分まで一致（§4）。
4. node4/5 表は**生涯空**（到達可能コード=dispatcher の読取のみ・全数走査で登録者 0）。デッド機構（§5）。
5. `_purecall=0x982D85` 確定（連続 6 純粋仮想=hkProcessHandler の 6 本と一致）。`0x68F970` は共有の空仮想（`ret 4`）で purecall ではない二種併存（§6）。
6. getProcessTag の実体は `mov eax,[tag 世界]; ret` 型で、17 登録のタグ保存先 16 グローバル（`0xBA8410..0xBA8460/0xBA9500..0xBA9508`）を全数列挙。vt 上の位置は**+4**（§6）。
7. dispatcher の ID 空間 2..0x19 は**コマンド ID**（process タグではない）。ID 取得 = 引数ストリーム obj の slot2（§6）。

### 7-2. 新規残点（次バッチ候補）

1. **[残] BSA v103 ファイル表のオフセット解読 → 実 Oblivion .hkx のセクション名照合**（低）: §1 の唯一残課題。タイムボックス超過（stride/order 8 組合せ全滅）。pyffi 等既知実装の参照で解ける可能性大。
2. **[残] hkDisplay 派生クラス 6 の vt↔クラス名対応**（低）: `0xAA2BE4/0xAA2BEC/0xAA2C04/0xAA2C14/0xAA2C2C` の ctor（`0x94B84x/0x94CCE9/0x94D2A7/0x94D752/…`）が生成する 0x54/0x3C 級オブジェクトの型名（ε/π/FLT_MAX を持つ静的群の帰属先）。4.0 ヘッダ素材待ち。
3. **[残] vt 0xAA2BA4 の [4]/[5] は isOk/step のどちらか**（低）: 4.0 の宣言順微差（isOk 裸 ret と step ret4 の並び）未確定。
4. **[残] ゲーム側派生（0x91xxxx 帯）process vt の追加仮想内訳**（低）: ST/Shapes/ConvexRadius/Phantoms が base の後に追加するスロット群の意味。
5. **[残] teardown 鎖の上位**（低）: `0x8BB990`（逆順破棄 walker）呼出元 `0x8BBB9D` の属する関数とその起動条件（終了フック経路）。§3 で位置まで特定。
6. **[残] 起動関数 `0x8BBA80` と別系 4 ノード（`0xB2EB0C/0xB2F1B4/0xB30064/0xB30118`）のクラス**（低）: §3 で表出。`[0xBA803C]` の slot2 呼出・`0x534020/0x534070` レジストリ帯を含む hkSystem 系初期化の一覧化。

### 7-3. 自己検証

- **selftest**: 着手時 4/4 PASS（ヘッダ記載）。**完了時にも再実行 → 4/4 PASS**（下記実行記録）。
- **VA 換算**: 全て `vc.va2off()` / `imm32_refs3()`（セクション個別 raw）／`e8_callers`（.text raw 起点 0x400C00）。v245 §8 の raw/VA 混同罠は非踏襲。§3 で「命令先頭 vs int3 後方 1 バイト」の教訓 1 件（0xA12340 系 4 ノードは命令先頭で imm 照合が必要だった）を §3 証拠 A に明記。
- **数値の相互整合**: §6 の tag グローバル 16 個は §3 の node3（hkProcessFactory）16 登録名と 1:1。§5 の「空表」は §6 のコマンド ID 空間と独立に矛盾なし（default 走査 0 反復）。
- **負証拠の区分**:
  - **不存在証明（構造的 0 件）**: descriptor+8 読手 1 件以外 0／node4/5 書込み手 0（全セクション imm32+e8 走査）／9 ノードのコード参照 0（CRT テーブルのみ）／三つ組の imm32 参照 0。
  - **確認できず**: 4.0 期セクション名の正式性（Web 5 クエリ 0 件＋grep.app 429＋BSA 展開未了）／hkDisplay* メンバ名／node4/5・Options フィールドの正式名（5.1 同型不在＋4.0 非公開）。
- **外部資料**: SDK 5.1（`Visualize/hkProcess.h` `Visualize/hkProcessHandler.h` `Visualize/Shape/hkDisplayGeometry.h` `Shape/hkDisplayGeometryTypes.h`・Visualize/Process/Shape ディレクトリ listing）＋ Bethesda Modding Library（Skyrim 期 wire format・実ファイル 3 セクション）。websearch はクォータ到達（0 回使用）、全文字検索は webfetch(DuckDuckGo/Bing) で代替。
- **制約遵守**: git 書込み 0／docs・GT・TODO・session-memory・plan・cache・ソース編集 0／既存 tmp/re 上書き 0（新規は本 v251 のみ）／ESM 再スキャン 0。中間物 `%TEMP%\opencode\v251_*`（新規 10 本＋out 8 本）。
- **中断対策**: §1→§2→§3→§4→§5→§6→§7 の順に**各節完成の都度即追記**（§2 追記時の差し込み順序事故は同一セッション内で再構成し、§1 が前に来る正規順に修正済み。以降の追記は末尾锚点方式）。

### 7-4. 使用証拠一覧

- **本体**: `tmp/re/v251_process_havok_residuals11.md`（本ファイル・新規）
- **入力（読取のみ）**: `tmp/re/v249_process_havok_residuals10.md`（出典）/ `v247_process_havok_residuals9.md` §2〜§5 / `v245_process_havok_residuals8.md` §5 / `tmp/re/cache/{named_funcs,vtable_index,call_targets}` / 既存中間物 `%TEMP%\opencode\v249_*`
- **本バッチ出力**（`%TEMP%\opencode\`）: `v251_r2/r2b/r2c(_out.txt)`（item2）、`v251_r3..r3f(_out.txt)`（item3）、`v251_r456`・`v251_r4b`（item4/5 帯）、`v251_r6/r6b/r6c/r6d(_out.txt)`（item6）、`v251_hkx_scan(.py/_out)`・`v251_bsa_*`（item1 実測試行）、`v251_util.py`（v249_util 継承）
- **ローカル実データ（読取のみ）**: Steam Oblivion `Data\Oblivion - Meshes.bsa`（BSA v103: ver=103, dirs=36, files=1927）— 平文域に `.hkx` ゼロ・`.nif` 8,033・展開未遂
- **一次資料（外部・本バッチ取得）**: `sigmaco/havok-v5.1.0r1-b2007.09.19` — `Source/Common/Visualize/hkProcess.h`／`hkProcessHandler.h`／`Shape/hkDisplayGeometry.h`／`Shape/hkDisplayGeometryTypes.h`／Visualize+Process+Shape listing、Bethesda Modding Library（animations/`hk_2010.2.0-r1` wire format 節）
- **Web**: DuckDuckGo webfetch 4 クエリ（0 件×3＋1 ヒット精読）、Bing 1 クエリ（0 件）、grep.app API（429＝**取得失敗**）。

### 7-5. 既存記述の訂正ドラフト（本体は未編集・反映は承認待ち）

1. **v249 §5（node3=hkProcessFactory 裁定内の傍注）「dispatcher の `call [eax+8]` を getProcessTag と同定」**: **誤り**。`call [eax+8]` の対象は process ではなく **update 第 1 引数の入力ストリーム系オブジェクト**で、slot2 は **コマンド ID getter（2..25）**。getProcessTag は本 EXE で **slot1（+4）**（`mov eax,[tag]; ret`、4 クラス実測 §6）。→ 「dispatcher は process タグでなくコマンド ID で分岐」に読み替え。
2. **v249 §2 証拠 C/裁定「0x34=52（整数）を含む 3 語組・表示既定値（頂点上限 52…）と推定」**: 3 語は **float `{FLT_EPSILON, FLT_MAX, HK_PI}`**（0x34000000=1.19209e-7）。整数 52 の読みは本バッチで置換（§4）。家系 hkDisplayGeometry/Shape の旁証は增强（9 名 enum 一致）。
3. **v249 §6-5 ドラフト 2「`0x68F970` = purecall ではなく空スタブ」を補完**: purecall は **`0x982D85`**（_purecall 本体確定 §6 証拠B・hkProcessHandler の連続 6 純粋仮想 slot と一致）。`0x68F970`（ret 4 共有空仮想）との**二種併存**が正しい記述。
4. **v245 §5(3)「node4/node5 = ID→ハンドラ登録配列シングルトン×2（ディスパッチャ消費）」を補強**: **本 EXE では登録手段が一切無く配列は常時空**（書込み手 0 件・§5 証拠A）。消費側の存在する未使用機構。
5. **v249 §6-2 残点 3「9 ノードの CRT 初期化順未特定」**: 本 md §3 で解明済み（残点クローズ）。
