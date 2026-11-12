# v304_ref_change_state_evidence.md — serial 286 前半（REF change state 証拠棚卸し・本体側）

- 作成日: 2026-11-06 / 担当: 本セッション inline（#65 の本体部分。サブエージェント側=CHANGE_*/TES4SAVEGAME 棚卸しは v305 に統合予定）
- 出典: tmp/re/v303_ref_change_state_assessment.md（判断=「サンプルなしでは完全境界は確定不能」）→ 本束は **EXE 静的証拠だけで確定できる範囲の最大化**
- 中間物: `%TEMP%\opencode\v304_i1..i5.py/_out`（5 probe・全 EXIT=0）
- 判定サマリ: **(1)=確定: 変更状態フォーマッタ FUN_00453A90 の全構造 / (2)=確定: 66 種の列挙=エンジンが認識する REF 変更の完全カタログ / (3)=確定（負）: 変更レコードに 4cc サブレコード名なし / (4)=確定: 圧縮=zlib 静的リンク / (5)=手順書: サンプル受領後の検証パイプライン**

## 件1 FUN_00453A90 = 変更状態ヒューマンドスキューティブ生成器 — 【確定】

- 範囲 0x453A90..0x457A8C（**16,384B・5,344 命令**、fstart=0x453A8C の CC 境界後 4B=プロローグ直後）。`ecx=[0xB33B00]`=TESSaveLoadGame 単例（**両 caller とも mov ecx,[SLG] を push 群の前に実行**＝0x45B029（ロード警告帯 fn 0x45AE65 系・1KB ローカルバッファ `[esp+0x37c]` 経由）／0x45CD9F（extra 記述帯・node+4 の型 ID byte を push）。
- 引数（caller 実測）: `[esp+0x164]=form(ptr) / [esp+0x168]=出カ1byteクリア対象 buf(edi、入口で mov [edi],0) / [esp+0x170]=追加文脈(0x4535A0=ChangeFlags 台帳 find 系へ渡す) / [esp+0x178]=verbose ゲード byte / caller2 経由=extra node / flags`
- 入口の型安全化: `RTDynamicCast(0x9832E6)×3`（TD スロット 0xB03054/0xB05538/0xB03070・v271/v275 の TESForm/TESObjectREFR/ExtraDataList 系）→ esi=REFR。`[esi+0x58]`=extra データリスト（v286 と同オフセット）。
- 出力: 各分岐が `sprintf(0x98208B)→'xxx(%i)' 形式を buf へ`（`%i`= formID 下位やインデックス）。**=Warnings.txt の "what changed" 行の生成器**（v301 で確定したログ_sink 0x4941A0/'Warnings.txt'@0xA3D9BC と同系統の文言帯）。
- ジャンプテーブル 0x458E38（5 case）: `[esi+0x58]` リスト count+1 を 0..4 クランプ→case0='None(%i)'..case4='Base(%i)'（0xA39F28/F08/EE0/EB0/E8C/... 6 優先度ラベル帯・verbose/quiet の 2 書式 0xA39F34/0xA39E98 組併用）。**プロセス優先度ラベルの分岐**（0x5E4080=process→byte[+0x20]→0xB12988 索引テーブル＝優先度解決ヘルパが本体から x2 呼出）。

## 件2 変更状態 66 種の完全カタログ — 【確定】（v303「未確定」の本体解決）

probe #5 が文字列→直前条件を全ライン対応（66/66 収集）。型 ID/フラグ条件でグループ化すると:

| 系統 | ラベル（VA@formatter内） | 判定条件（命令） |
|---|---|---|
| 優先度 | None/Low/Mid Low/Mid High/High/Base(%i) @453C3D..D37 | jumptable 0x458E38（件1） |
| form | Form Flags @453E6F | `[esp+0x178]==0` verbose ゲート＋リスト走査 |
| quest | Quest Flags / Quest Stages / Quest Script(%i) / Quest Script(No longer has script locals) / Topic Said Once / Never Run Flag / Waiting Flag @453FC7..4545E2 | 同上（TESTHDR/KNAM 帯相当は quests 管理リスト走査） |
| cell | Cell Flags / Cell Created / Seen Data / Detach Time / Cell Changed @4546FF..455324 | **`call 0x452250`（GetForm(FormID)）＋`call 0x9832E6`(TESForm cast)** が 'Cell Created'/'Seen Data'/'Detach Time'/'Created'/'Cell Changed'/'Moved' 帯に出現＝**この 6 種は formID→form 解決を伴う**（境界=キー+form ptr） |
| ownership/name | Ownership / Full Name / Created @454BBA..455208 | cast 併用 |
| pathgrid/faction/book | Pathgrid Modified / Faction Flags / Faction Reactions / Book Skill @454DE1..455101 | 純 payload（452250 無し） |
| move | Moved @455464（3 float 帯）・Havok Moved @45557F/58E/5CB（×3=cell/pos/rot の 3 部材）| eax/ebp ガード |
| havokflag | Had Havok Move Flag @4556C3 | **`cmp edx,0x800`＝changeFlags bit11**（v275 disable ミラー・v297 bit3→bit11 翻訳と直結） |
| oblivion | Oblivion Flag(0) / Oblivion Flag(%i) @45581D/852 | `cmp dl,0xE`（extra 型 ID 0x0E 相当の特別扱い分岐） |
| visual | Animation / Scale / All Extra / Disabled×2 / Enabled×2 / Disabled-Enabled @45592E..455D58 | Disabled/Enabled=`cmp al,1` 値分岐（v275 bit5 系対応） |
| extra | Leveled Creature / Inventory / Script / Movement Extra / Map Marker / Furniture Markers / Extra Magic / Lock / Owner / Global Owner / Rank Owner / Dropped Item Flag / Empty Flag / Teleport / Door State / Door Default State / Equipment / Game Modifiers / Script Modifiers / Magic Modifiers / Run Once %08X / Trespass %08X / '%s %08X' generic / Non-saved Package / Disp Modifiers @455E3E..4579C4 | 各々 GetExtra(type) 台帳検索（[SLG] map）→ 型 ID 分岐。**'Run Once %08X'/'Trespass %08X'/'%s %08X'=key+formID 2 部材、generic `%s` 書式=未知 extra のフォールバック** |

→ **移植への帰結**: エンジンが追跡する REF 変更状態の**完全な一覧＝この 66 分岐**。移植側 delta 台帳（未実装）がカバーすべき項目仕様が、サンプルなしで確定した。特に (a) formID 解決必須の 6 種、(b) bit11=Had Havok Move Flag の changeFlags 直対応、(c) Run Once/Trespass の formID キー付き extra、(d) 未知 extra フォールバック `'%s %08X(%i)'`。

## 件3 4cc サブレコード名スウィープ — 【確定（負）】

保存帯 0x450000..0x466000 の imm32 全走査で既知 save 関連 4cc 候補（CHRD/CHID/CHDR/CHDL/CHNG/INTC/HCHR/HCHD/TNAM/KNAM/SNAM/SSTA/SCCS/SCHD/SCES/SDEL/SMB*/SI*/ZNAM/GDEL/QSCR 等 50 超）を照合 → **ヒットは ESM 側既知の DATA/FLTV/NAM0-2/PGRD/SNAM/TNAM/ZNAM のみ（いずれも ESM/GECK パーサ帯 0x415xxx/0x4B0xxx/0x4EExxx/0x4F1xxx/0x51Dxxx）。保存帯 0x45xxxx-0x46xxxx での change-record 用 4cc タグ=ゼロ**。v283 の「tail レコードに _SIGNATURE 無し（4cc 即値 sweep 0 件）」と整合——**REF change state はタグレス二進形式**（type byte '0'..'3'＋flags＋payload、v283 §2 仕様）。移植パーサに 4cc 期待は不要と確定。

## 件4 圧縮の正体 — 【確定: zlib 静的リンク】

- 全イメージ検索: `'zlib'/'ZLIB'` @0xA35F20・0xA36037・0xA38E59/E78/EB4・0xA3BEE0/F03（7 所）、`'inflate'` @0xA3603C・0xA38EB9・0xA63280・0xA82A59、`'deflate'` @0xA3BF13・0xA63262/277・0xA828F9。zlib エラー文字列群が .rdata に揃う＝**リテールは zlib を静的リンク**。`.ess` 圧縮ペイロードのデコーダは zlib stream（v303「圧縮位置未確定」は**アルゴリズム側は確定、オフセットのみサンプル待ち**に縮約）。
- 78 9C/01/DA マジックの .rdata 出現 13 件=静的データ（ストリーム実体は不在＝当然）。

## 件5 サンプル受領後の検証手順書（ヘッダ→境界順・v303 Decision の具体化） — 【手順確定】

1. ヘッダのみ: 先頭 12B=='TES4SAVEGAME\0'、version dword を読む（閾値 0x1c/0x2e/0x34/0x37/0x38/0x51/0x52 は**サブエージェント棚卸し v305 が site 表を確定**）。
2. セーブスロット→チャンク: v283 §2 のプール 0x453500 採番・スロット {+0 next,+4,+8,+C} レイアウト適用。
3. change-record 歩行: v283 §2-2 tail フレーム（`<u16 len><byte+2 '0'..'3'><byte+3 extra type ID>`、≥0x5B=12B/0x2C、bit23 で +4、古典=4/6B）＋v301 §件3 の再発行形式（bit3 tombstone→bit11 翻訳）。
4. デコード検証: 各レコードの type/flags を**本件 2 の 66 ラベル表と突合**（Warnings.txt を実機で生成し、本文 'xxx(%i)' が表の分岐と 1:1 で落ちれば境界正しい）。=**フォーマッタが oracle になる**——これがサンプルなしで得られた最大の帰結。
5. 圧縮部: zlib inflate（0xA632xx 帯 APIs）で伸長後に 3 へ。
6. 禁止: パーサ/移行コードの推測実装（v303 判断維持）。

## 統合所見（台帳・v275/v283/v286/v297/v301 との接続）

- `Had Havok Move Flag=bit11(0x800)` の文言対応で、v275「bit11‖bit5 同一視 Disable 機構」と v297「pass2 bit3→bit11 翻訳再発行」が **Warnings.txt 語彙まで含めて閉じた**。
- 'Created(%i)'/'Cell Created'/'Cell Changed' の 452250+cast パターンは v274「+4 formID→FUN_0046B250 解決」の対（GetForm=0x452250）。

## docs 反映候補（+3 点・#20/#47）

1. REF change state=タグレス二進（4cc ゼロ確定）＋type '0'..'3'/12B/0x2C/0x1C フレーム仕様への v303 引用補強
2. 変更状態 66 種のカタログ（移植 delta 台帳の受入基準表）
3. .ess 圧縮=zlib 静的リンク（デコーダ仕様の確定）

## 残点（v305 統合後に再評価・低）

1. 'None(%i)'..6 優先度のジャンプテーブル意味の最終化（count+1 vs 0x5E4080 優先度の関係）
2. フォーマッタ分岐と**保存側の逆方向（write）窓**の対応（記述=load/warning のみ確認、save 書側は v283 dispatch に包含か要突合）

## 追補（probe #6・残点1/2 を解決、件1 の [推定]1点を訂正）【確定】

**(A) 件1 訂正:ジャンプテーブル 6 分岐は「プロセス優先度ラベル」ではなく process level 名**（同一成果物内の先の記述は [推定] 誤り、命令実測で差替）:

- ケースターゲットの verbose 変種が `'Process Level: None\r\n'`@0xA39F34 / `'Process Level: Low'`@0xA39F10 / `'...iddle Low\r\n'`@0xA39EFC（'M' 位置ずれ断片）/ `'Middle High'`@0xA39EC0 / `'High'`@0xA39E98 — **None/Low/Middle Low/Middle High/High/Base(%i)** 6 分岐（probe #5 の最初ラベル群）は **TESObjectREFR vtbl+0x4C プロセスレベル getter**（0x453AF2 `call [vtbl+0x4C](0)→movzx ax` 保存先 [esp+0x14]）の値を報告する分岐と確定。verbose=見出し付き・quiet='X(%i)'（%i=レベル値）。
- 0x5E4080 は**優先度解決器ではなくパッケージ名解決器**: process→[+8]→byte[+0x20]→**0xB12988 索引テーブル**（12 名: 'Find','Follow','Escort','Eat','Sleep','Wander','Travel','Accompany','Use Item At','Ambush','Flee not Combat','Cast Magic'）。formatter 内呼出 2 点（0x45774F/0x45775A）は generic `'%s %08X(%i)'`（0x457799・未知 package 系の警告）へ。E8 callers 計5（0x40942A/409447・0x45774F/75A・0x4F4C39）。

**(B) 残点2 解決:保存側の write 窓は formatter を呼ばない（診断専用と確定）**:

- caller2関数 = **0x45AE65**（strings 'Buffer'/'NOT LOADED'＝CheckFlags ワーニング帯・v273 系）と **0x45CC5F**（strings **'SAVEGAME INFO'**/'References changes were nullified by CheckFlags().'/'Current reference has no changes.'/LoadAnimations・LoadHavokData 空バッファ検証群＝v282/297 の SLG 検証帯）。両方とも **LoadGame 完了後レポート band（0x45A-45Cxxx）**で、保存書側（0x465860 dispatch・v283）からの呼出はゼロ（E8 全数=2 のみ）。→ 件5 手順書の「フォーマッタ=oracle」は**ロード側検証専用オラクル**と明確化（保存側は v283 仕様が正典のまま）。

**(C) Graphify 関係性確認（今ラウンド・query 実行）**: フォーマッタはグラフ上 `FUN_00453a90()`（docs/Oblivion_decompiled.c L64561・community FUN_006ecc80）として既存ノード化済、ChangeFlags 台帳 `FUN_004535a0()`（同 community L64298）と同コミュニティで隣接＝**v273/v283/v297 系と本 v304 がグラフでも一体**。tmp/re 新規 .md（v289-v304）は gitignore 域で未登録のまま（#64 人間決定待ち・v302 §件3 踏襲）。

## 残点（残りは v305 統合後に再評価）

1. ~~優先度ラベルの意味~~ → **解決**（本追補 A）
2. ~~保存側 write 窓の対応~~ → **解決**（本追補 B・診断専用）
3. （新）0xA39F28 'None(%i)' の Base との順序対応（process level enum 0..5 の値域は実機/GECK 側＝恒久保留可）

## 追補2（probe #7–#9・lane X＝ゲート体系の全数確定）【確定】

並行レーン X（本体 inline）として formatter 全域の **cmp/test 即値センサス**と**ラベル網羅性**を確定させた。probe `v304_i7/i7b/i7c/i8/i9/i9b.py`（全 EXIT=0）。

### (A) 範囲訂正: formatter は 0x453A90..0x458E50（0x53C0B）

- 従来の fend() CC 境界判定 **0x457A8C は誤り**（内部 CC ランの誤検出）。v305（サブエージェント）が独立に確定した単一包含関数 **FUN_00453A90（次関数先頭 0x458E50・ジャンプテーブル 0x458E38..0x458E48）** に追随して延長帯 0x457A8C..0x458E50 を再走査（probe #9/#9b、7,049 命令）。
- **注意（再発罠）**: ジャンプテーブル本体 0x458E38..0x458E4C はデータであり線形 disasm で `cmp al, 0x45` 等の偽命令を生む（probe #9b で明示除外）。従来の 66 分岐は main-range 行数に過ぎず、延長帯に 18 ラベル行が存在した。

### (B) ラベル行の全数: 87 行 / 一意表示ラベル 80 種 — 【確定】

- **main 69 行**（probe #7c・うち一意 65: Havok Moved ×3 / Disabled ×2 / Enabled ×2 の重複除く。3 行は `CHANGE_ACTOR_*_PACKAGE %08X` printf 形＝probe#5 のフィルタで漏れていた分）＋ **延長 18 行**（probe #9b・うち新規一意 15: 'Oblivion Entry'/'Investment Gold'/'Life State'/'Persuasion'/'Crime Gold'/'Base Health'/'Base Attributes'/'Base Data'/'Spell List'/'Factions'/'Base Modifiers'/'AI Data'/'Skills'/'Combat Style'/'Fame'。'Full Name' は main と重複）＝ **87 行・一意 80 種**。
- 従来記載の「66 種」は probe#5 の main-range 行数（CHANGE_* 3 行除外）であり、**本追補で 80 種へ拡張・訂正**。

### (C) ゲート体系の三分類（全域ヒストグラム・probe #7b/#7c） — 【確定】

formatter 内の cmp/test 即値は次の3種に完全分解できる（capstone オペランド API による全命令走査）:

1. **文字列長ガード `cmp eax, 0x19`（x108＋`sub eax,edx`=strlen 後）**: probe #8 文脈で **25 字への右パディング・ループ**と確定（`mov si,[0xA36430]`=word `0x0020`=' ' を末尾へ書く→長さ25未満なら `jb` で反復。`jae` で既に ≥25 ならスキップ）。0xA36430 の値は `20 00 00 00`=' '（直後 "File" 文字列）。目的=整列カラム出力と推定（[確定]=挙動、[推定]=目的）。
2. **extra 型 ID ゲート `cmp al/word, NN`（18 サイト）**: **`al` ← `mov al, byte ptr [esp+0x174]`**（probe #8(a) @0x453F2B 実測）＝caller2 が push する **extra ノードの型 ID byte**（v303 仕様 tail frame byte+3 の対応物）。閾値: `0x04`(None帯/JT bounds 混在) `0x06`(Faction Flags) `0x0E`(Oblivion Flag) `0x15`(Book Skill) `0x18`(Teleport・下記 D) `0x23`(Quest Flags/Skills) `0x24`(Quest Flags/Skills) `0x30`(Cell Flags) `0x31/0x32/0x33`(Created/Map Marker) `0x3A`(Topic Said Once) `0x3B`(Quest Flags) `0x3D`(Never Run Flag) `0x32` word 形(Crime Gold) `0x45`(Skills/Combat Style 帯)。
3. **changeFlags ビットゲート `test dl/dx/edx, NN`（52 サイト）**: **`dl/edx` ← `mov edx, dword ptr [esp+0x170]`**（probe #8(g) 全 site 実測）＝**changeFlags 値そのもの**（bit11=Had Havok Move Flag が v275/v297 と一致）。ビット 0–6（0x1/0x2/0x4/0x8/0x10/0x20/0x40）、8–30（0x100..0x40000000）を網羅。**bit7(0x80) は formatter 全域で未検出**（負証拠: 未使用か別経路）。複合マスク `test dl,0xE`（=bit1+2+3・Oblivion Flag）1 件。

### (D) 型 ID ゲートの代表的帰属（main/延長の主要行） — 【確定】（ゲート位置はウィンドウ法のためラベル直前帰属・±1 行の揺れあり→[部分]）

| ラベル | extra 型 ID | changeFlags bit |
|---|---|---|
| Form Flags | – | 0x1 |
| Quest Flags | 0x23/0x24/0x3B | 0x4 |
| Cell Flags | 0x30 | 0x8 |
| Cell Created / Created | – / 0x31,0x32,0x33 | 0x2+0x4 / 0x2 |
| Faction Flags / Reactions | 0x6 / – | 0x4 / 0x8 |
| Book Skill | 0x15 | 0x4 |
| Topic Said Once / Never Run | 0x3A / 0x3D | 0x10000000 |
| Had Havok Move Flag | – | **0x800（bit11）** |
| Oblivion Flag | 0xE(複合 bit1-3) | 0x800000+0x2000000 |
| Lock / Global Owner / Rank Owner | – | 0x40 / 0x100 / 0x200 |
| Map Marker Flags | 0x32,0x33 | 0x400 |
| Teleport | **0x18（下記 D')** | 0x100000 |
| Equipment / Base Modifiers | – | 0x20000000 / 0x10000000 |
| Oblivion Entry / Investment Gold / Persuasion | – | 0x4000 / 0x2000 / 0x1000 |
| Life State / Base Health / Base Attributes / Base Data / Spell List / Factions | – | 0x40 / 0x4 / 0x8 / 0x10 / 0x20 / 0x40 |
| Skills / Combat Style / Fame | 0x23,0x24 / – / – | 0x200 / 0x400 / 0x800 |
| Crime Gold | **word [esp+0x174]==0x32** | – |

- **D' Teleport の特殊形**: `cmp byte ptr [eax+4], 0x18`（@0x456C97）。eax は **vtbl+0x170 getter の 2 重呼出結果**（probe #8(f): 1 回目 null チェック、2 回目値取得）。＝**スタック引数ではなくオブジェクトの +4 byte を extra 型 ID(0x18=24) と比較**する形。getter の戻り構造は [部分]。

### (E) 与件への帰結

- 移植側 delta 台帳の受入基準＝**87 ラベル行×（型 ID or changeFlags bit）のゲート表**が上記で完全化（v303「境界未確定」の本体側解決が main+延長全域に及ぶ）。
- `cmp eax,0x19` パディングと `0xA36430`=' ' の発見で、**Warnings.txt 出力の整列書式**（25 字カラム）が命令レベルで確定。
- v305（CHANGE_* 67 件・TES4SAVEGAME 経路）と本追補は**非重複**: v305=文字列/xref/ヘッダ、本追補=formatter 内ゲート即値。

## 検証（selftest）

`v304_i1..i9b.py` 再実行 EXIT=0。整合: formatter 範囲 0x453A90..0x458E50（v305 と一致・旧 0x457A8C は訂正）、ラベル 87 行（69+18）＝i7c rows 69＋i9b extension 18、ゲート=型 ID 18＋flag bit 52＋長さガード 108、al←[esp+0x174]・edx←[esp+0x170] 実測、0xA36430=' '、4cc 負=50+ 候補 sweep、zlib 文字列 7+4+4 所、caller 2 点の ecx=[SLG] 実測＋呼び主 band 文字列（'SAVEGAME INFO' 等）で診断専用確定。既存本体無編集・ESM/git 非接触。

## 追補3（v306 統合による caller/CheckFlags 訂正・2026-11-06 18:15）【確定】

並行レーンY の成果物 `v306_warnings_path_evidence.md`（23,951B/232行）受領に伴い、本 artifact の従来記述を以下で**訂正**する:

1. **caller1 の関数先頭 = 0x45ADC0**（範囲 0x45ADC0..0x45B2FF・1,344B・Graphify 既存ノード `FUN_0045adc0` と**一致**）。追補 probe#6 の「0x45AE65」は `push 0xA3A4CC` の**即時値 CC 偽陽性**であり誤り（what-changed ファイル内部出力関数）。
2. **caller2 の関数先頭 = 0x45CC60**（範囲 0x45CC60..0x45CDF1・402B・Graphify `FUN_0045cc60` と**一致**）。従来の「0x45CC5F」は前関数末尾の 1B CC padding。**v305 の帰属「0x45CD9F∈FUN_0045C9C0」も誤り**（0x45C9C0 側は `ret 4`@0x45CC5C で終端、0x45CD9F は FUN_0045CC60 内。v305 の 8B CC run 境界規則が 1B 境界を見逃した形）。
3. **CheckFlags() = FUN_004535A0**（0x4535A0..0x453906・838B）と特定 — **本 formatter 入口（0x453AD0）が呼ぶ「ChangeFlags 台帳 find 系」と同一関数**であり、件1 の構造説明と接続。戻り==0 で「References changes were nullified by CheckFlags().」BB に入る命令証拠あり。
4. **Warnings.txt 出力経路は負証拠**: 'Warnings.txt'@0xA3D9BC の .text xref は 1 site（0x4944BC→FUN_00494480 path getter・DeleteFile/ログ用途のみ）。ロガー FUN_004941A0 への chain は caller2→0x57B8E0→0x5723E0→0x4A7A60→FUN_004941A0 で到達するが **ch1→vtable[1]=no-op で断絶**、caller1 はロガー非接続で自前 CreateFileA/WriteFile 直書きのみ。→ 件5 手順書の「フォーマッタを oracle に Warnings.txt 突合」は、**その Warnings.txt 自体が caller1 の直書き出力（出力名=object+0x3C+".txt"・実行時一致は唯一の [未確定]）**である点を付記して運用する。

（v305 の caller 帰属 1 点は row287 に訂正記録済み。v305 本体は成果物として保持し無編集。）
