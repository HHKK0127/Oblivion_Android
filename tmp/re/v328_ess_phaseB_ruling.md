# v328_ess_phaseB_ruling.md — v320 Phase B 12 件（P1-P12）EXE 静的裁定（L1・日本語報告）

- 作成: 2026-11-08（ai-todo #92 / 次束P・サブエージェント lane。親が統合）
- 出典: `tmp/re/v320_ess_dynamic_phaseA.md` **§7-2（Phase B 12 件）**。§1-§6 の Phase A 実測を前提とする。
- 判定: **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論は削除せず訂正節（§5）で併記。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来知識の持ち込みなし。使用テンプレートは本 repo の artifact（v283/v307/v313/v315/v316/v317/v319/v320/v323）のみ。
- **実 .ess の EXE 動的実行は行っていない**（指示どおり静的のみ）。動的実行が必要な残部は [未確定]（§6）。
- probe: `%TEMP%\opencode\v328_01..19.py/.out`（19 実 probe + 目録ヘルパ v328_idx・全本 EXIT=0）。検体 v320_sample.ess（MD5 FC32EFAEB90D26141D3DC4A325AC10E7・836,742B）と v320 probe 出力は**読み取りのみ**。
- 成果物: **本ファイル 1 点のみ新規**（tmp/re 既存無編集・STATE/LOOP/TODO/docs/GT/ソース/git 無変更）。

---

## 1. 判定表（12 件）

| # | 項目（v320 §7-2 の正式文言に準拠） | 判定 | 一句話裁定 |
|---|---|---|---|
| **P1** | LoadGame 読取列（45BB00 callers）と実レイアウト対応 | **[確定(命令)]** | 申し送り「**4+6+2=12 の分割読取 [推定]**」は **[確定負]** — 12B レコードヘッダは dispatch の**単一 n=0xC 直接 read**（0x465C7A-0x465C94、read ファンクタ直呼・45BB00 経由ですらない）。45BB00 の 4/6/0xC/0x24/0x2C は **changeFlags ビット別のペイロード可変読取**（§2-1 ゲート表）: bit1∧bit2=CELL 参照系→ver/bit26/bit25 で 4（flag+2×s8）/6（2×s16+u32→459990=u16idx→[+0x78] 解決）/0xC（ver<0x5B）、bit1∧¬bit2→0x24=36B、**¬bit1∧bit31→0x2C=44B＝created 位置/余剰 11×u32 のファイル読**（0x466113→459950×2・P11 の Δ=0x2C と一致）、¬bit1∧¬bit31→**GetForm 失敗時のみ台帳 bit31 entry の tail+4 から 44B を rep movsd 再構成**（0x4662F0-0x466333 `mov ecx,0xb`・v328_18 新規＝created 引用解消機構、Δ=0x1C との直接対応はサイズ不一致で不成立→§6-10）。v320 申し送り「0x2C=44 と 6 の意味も要突合」→ 上記で解消 [確定] |
| **P2** | serializer 0x460710/0x45C4A0（tail==0 経路）出力と v283 frame の関係 | **[確定(命令)]** | serializer（460710）出力＝payload バッファで、先頭に frame は作らない（＝v320 §6-9「実測差」の機構的正体。実測 `u16@payload+0==tailSize-4` **0/5,527** v328_08）。**v283 frame `<u16 size><char><typeID>` は台帳 entry の in-memory tail（旧 tail／pass2 再発行）専用コンテナ**で、ファイル書出（経路 A・tail≠0）で**分解**される: 旧 tail 先頭 4B 読（0x465365 `mov edx,[ecx]`・0x465369 cursor+4）→ **byte2(char/型区分)→10B buf+4**（0x46536D/0x465375 `[esp+0x3c]`＝buf(E+0x38)+4）、**byte3(typeID)→buf+9**（0x465371/0x46537E `[esp+0x41]`）、**u16→tailSize 2B 書**（0x4653DF push 2・buf=[esp+0x6c]）→ payload＝tail[4:] を tailSize B 書（0x465405-465419）。すなわち frame の 3 フィールドは**ファイル上は +4/+9/+10 のヘッダスロットへ分散**し、payload 先頭に frame は構造上現れない [確定]。経路 B（tail==0）: +4=[form+4] formType（0x465489→[esp+0x44]）、+9=[SaveGame+0x7c]=0x7D（0x46534A-0x465351・全経路共通の初期値）、tailSize=mask(u16)+serializer 長（0x4654FF-0x46553C）→ 460710 → 45C4A0（`write(stream,[ebp+0x14] バッファ,len)`・ret 0xC・0x4655AE）＝ v317 N1 の追認 |
| **P3** | dir・trailer・index 3,437 の writer・reader 突合 | **[部分確定]** | **dir 8B = FUN_462280 冒頭 2×u32 書出**（呼出 0x4652C1）: dir[0]@147,753=822,982 = `[[0xB33A98]+0x8C0]`、dir[1]@147,757=5,527 = `[[0xB333A0]の43F270 getter([ecx+0x74])]+0xC`（命令確定・呼称 [推定]）。**trailer+index = FUN_45E220** の 2 配列書出 `[u32 cnt(+0x74)][×u32][u32 cnt(+0x78)][×u32]`（ログ 'Numeric ID Array(%i)'@0xA3A9CC / 'WorldSpace ID Array(%i)'@0xA3A9B4）で、**実バイト fit = cnt1@822,978=0・cnt2@822,982=3,437・entries 822,986..836,734=footer 完全一致** → index 表 = **[SaveGame+0x78]** [確定(命令+実測)]。reader 対 = 0x462B20（唯一 caller 0x465C4D・byte3 mod-index 再マップ→46B250 解決）。残: **先頭 u32 2（@822,976）＋u16 0 の 6B 帰属**（45FB50 'Temp Effects List' count 候補・位相要追跡）と dir[1] の二重解釈（count 5,527 か formID 0x1597 解決か）は **[未確定]** |
| **P4** | pass2 の bit3 削除/翻訳の発火ゲート | **[確定(命令)]** | **外周ゲート = pass2 歩行が bit27(0x8000000) 立 entry のみ処理**（`test [entry+0],0x8000000 / je 0x461a0d` @0x4617C8-0x4617D5）。内側: bit3 なし→skip（0x461CB3）/ **bit1\|bit2 同時→skip＝bit3 残存**（0x461CBC）/ **bit3 のみ→452DF0 削除**（0x461CC5→0x461CD7）/ 複合翻訳 bit3→bit11（0x461CF4-CF7）。→ **v320 §6-7 の実測反例が完全説明される**: flags==8 単独 187 件＝bit27 消で歩行対象外→無加工書出 [確定]、bit3∧(bit1\|2)=1,679 件＝0x461CBC スキップで生存 [確定]、bit11=0 件＝bit27∧bit3∧¬(bit1\|2) エントリ本サンプル不在 [確定負(本サンプル)]。**v317 N1「bit3 単独は非書出」の一般化は bit27 文脈限定と訂正**（§5-3） |
| **P5** | FUN_45DBC0 が size 返却 = u32@34 の reader 側確定 | **[確定(命令)]** | **v307 残点4「FUN_45DBC0 内で値は戻されない」は否定（§5-2）**: size 4B read@0x45DD83 の dst = **E0+0x14**（E0=esp@0x45DD69・push 5 個=0x14: flag/out(E0+0x10・lea 45DD6E)/n=4/dst(lea 45DD75)/stream。read 5 引数契約 `(stream,dst,n,out,flag)` は v313 fill 解析で確定済）。正常経路（0x45DD96/9E/A3 の 3 je すべて）が `0x45DD87 add esp,0x14（→E0）→ 0x45DD8C pop ebx（→E0+4）→ 0x45DE08 mov eax,[esp+0x10] = [E0+0x14] = **dst そのもの** → 0x45DE23 ret 8` で返却（esp 完全追跡 v328_19）。失敗路 `0x45DDFF cmp al,2 → 0x45DE03 or eax,-1`（-1・eax 直接設定）、magic 不一致のみ `0x45DC7C xor eax,eax`（=0・返却行迂回）。**caller 4 点すべてが戻り値を size として消費 [確定]**: 0x46599E `mov ebx,eax → test/cmp ebx,-1（0x4659A0/A8）→ 0x465A3B push ebx = 45D5F0 arg2`（v313 確定契約「arg2=size=容量・fill バイト数」・v320 §6-4 link 実測）/ 0x4640B9→0x464115（fun 0x464320 counter 再計算）/ 0x46439A→0x4643D6 / 0x46112C∈FUN_4610F0 ヘッダ読ラッパ。v307 の「[E+0x18]」は esp 基準段差の表記差（誤りの本体は「戻されない」指摘）。writer 0x464DEC・実測 u32@34=147,521（v320 §4-1）と合わせ reader 側確定 |
| **P6** | +0x94 GetSystemTime 更新ゲート（+0xA4==0） | **[部分確定]** | ゲート命令 **[確定(命令)]**: `0x464B7F cmp [esi+0xA4],0 / jne 0x464BA1 → call [0xA28108]（GetSystemTime, &[esi+0x94]）→ [esi+0xA4]=125`（v307 [推定] を昇格）。**GetSystemTime の .text 呼出点は 0x464B97 と 0x748B59 の 2 のみ**（IAT xref 全走査 v328_10）→ 保存窓では 464AB0 のここだけ。効果（2 回目保存で +0x94 frozen 化）は load 側 45DBC0 が +0xA4 にファイル値を書くため閉鎖示唆だが **save#2 実データ不在＝ [未確定]** |
| **P7** | F5 float の型・getter（0x464FCF 前後） | **[部分確定]** | 源 = ゲーム時計 [0xB332E0]: **`call 0x402BD0 → fdiv qword[0xA2F920]（=24.0）→ 0x402C00（int 化・負時 [0xA2FC78]=2^32 補正）→ 商 int×2^12?（fild）→ frac（fadd [esp+0x34]）→ fstp [esp+0x48]`**（0x464CA1-0x464CDA・命令列確定）。0x464FCF/0x464FE3 = 当該 4B（F5=「日数 float」）の無条件書出（dry=計上のみ）。実測 1.042361 = (25.0167h)/24 と整合（新ゲーム 25.0167 時間経過）・playtime 71.015s とは別物（v320 §7-1-6 の不対応は妥当 [確定]）。命名（経過ゲーム日数）[推定]、v307 残点8 の [esp+0x48] スロット再利用は GetTickCount 計上（0x464D9F-0x464DA5＝Player+0x710/714）が別スロットで整合 [部分確定] |
| **P8** | 中間セクション（148,555..148,666）writer 突合 | **[部分確定]** | 当該帯は **FUN_462280 セクション連鎖の途中**（呼出 0x4652C1・head `83 ec 28`・pre16 CC ラン 6＋`c2 04 00` ✓）。連鎖: `677010([0xB3BD00])→u16+buf(2B×n) / 542EA0→5406F0/540720→u16+buf（'Process Lists Class'/'Spectator Events'/'Sky/Weather'/'Quick Keys'/'HUD Reticle'/'Interface'/'Regions' ログ）/ 4A2FF0 …`。観測 u16=16 の 1 件（'TES Class'@0xA3AF5C 帯）は実測帯の u16 16 と整合 [推定-強い]。u32 8 / f32 2.0 / u16 1700 の変数 1:1 対応は帯外 getter（542EA0 等）の身元未追跡＝ **[未確定]** |
| **P9** | global 候補 2 表の正式名（GLOB/GMST 判定・writer 呼出列） | **[部分確定]** | **表①の呼出列確定**: 462280@0x462456 `mov ecx,[0xB333A0]; call 0x441000`（u16 count・head: `mov ecx,[0xb33b00]; call 0x45a170`＝version∈[0x1F,0x5A) ゲート内のみ）→ 0x46248B 2B 書 → 0x46250F `[0x4410D0]` buf 書（count 95×8B=760 → 終端 148,555 実測一致 ✓）。**表②**（count 212・終端 150,364=チェーン起点一致 ✓）は同連鎖の後段セクション（5A8250/57BE10 群候補）[推定]。ただし **[0xB333A0] シングルトンの正式身元が旧 artifact 間で不一致（s68=playerRef 説 / s65=+0x78 NavMesh 説 / 本件の count/セクション群）** → GLOB/GMST の別を含む正式名 **[未確定]**（§6-4）。ディレクトリの dir[1] 供給元と同一オブジェクト [+0x74] サブ表であることのみ [確定(命令)] |
| **P10** | 2 SYSTEMTIME 二重書きの意図 | **[確定(命令)]** | **別用途の 2 書出と確定**: ①+0x94=UTC・`GetSystemTime` @0x464B97・ゲート [+0xA4]==0 時のみ＝**初回のみ更新（作成日時・frozen 候補）** ②F7=local・`GetLocalTime` 計測 @0x464CDE→書出 0x465044（**ゲートなし＝毎セーブ更新（保存日時）**、dry bit9 は計上のみ）。ms 同値 318（v320 §6-14）は同一保存イベントで両方埋まる帰結で矛盾なし。v320 §7-1-10「意図 [推定]」→ 機能分離は [確定(命令)]、命名（作成 vs 保存）は [確定-強い]（UTC/local 実測 §6-14 併せ） |
| **P11** | Δ=0x1C 切り抜き writer（0x461D94 append 元）と実データ対応 | **[確定(命令)]** | **Δ の正体 = created レコード（tail フレーム char 0x31/0x32/0x33）ペイロード先頭の位置/余剰ブロック**。pass2 は 0x461B65 `push 0x1c`（**7×u32=28B**）/ bit31 立 0x461B30 `push 0x2c`（**11×u32=44B**）/ bit1 系 0x461AE1 `push 0x24`（36B）を cursor から読んだ後 **`sar 0xC`（>>4096=グリッド変換）→ 446B80（created 判定）→ [SaveGame+0x74]/[+0x78] 配列解決 → 55E000 map find → (x,y) 一致走査**（0x461B8C-0x461CAF）＝ セルの再接続解決に専用使用。**再発行 tail は旧 payload[Δ:] のみ**（0x461D94 `lea eax,[edi+esi+4]`＝old+4+Δ、src 実測）→ ファイルから Δ ブロック除去の機構確定（v317 §6-1/v319 B1 の「中身 [未確定]」解消）。bit23 キー再挿入・bit31 +0x10B は v317 数式追認。**実データ側**: 経路 B payload の `u16@payload+0 == tailSize-4` **0/5,527**（v328_08 numpy 全件走査）＝ fresh に frame/位置ブロックなしの追証。7×u32 の各要素（pos x/y/z + 余剰）の個別名は [未確定] |
| **P12** | thumb 書出（×3B コピー元=D3D サーフェス形式）→ RGB/BGR | **[部分確定]** | 書出元 = **ロック済みサーフェスの描画ポインタ列**: ebx=thumb オブジェクト、`ppBits = *[[ebx+0x5C]] + [ebx+0x50]`（0x4650C3-0x4650CC・pitch 加算）、size/w/h 4B×3（45BAB0 書出ラッパ @0x4650AF/0x4650BE/0x4650D4）→ pixels = ebx 内部長（×3B・v313 `imul/lea(,8,2)` と整合・実 pitch 768=256×3 ✓）→ release `call [ebx]`(vtable+0) @0x4650E0。**読み取り側** F9/F10 = `new(0x70)+ctor 0x70E560`＋×3B memcpy（v313 既確定踏襲）。**RGB/BGR は変換命令ゼロ＝passthrough のため静的判別不能 [未確定]**（v320 §7-1-8 維持。判定には動的/画素突合が必要） |

### 1-1. 5 段階集計（主判定 12 件）

| 判定 | 件数 | 項番 |
|---|---|---|
| **[確定(命令)]** | **6** | P1, P2, P4, P5, P10, P11 |
| **[部分確定]** | **6** | P3, P6, P7, P8, P9, P12 |
| [推定] | 0（主判定） | —（各項内従属: P3 呼称・P7 命名・P8/P9 セクション身元・P11 要素名・P1 0x4662F0 呼称） |
| [未確定] | 0（主判定） | —（従属残件 = §6 の 9 件） |
| [確定負] | 0（主判定） | —（従属: P1「4+6+2 分割読取」[確定負]・P4 bit11 翻訳の本サンプル発火 [確定負(本サンプル)]・P6 GetSystemTime 保存窓の追加呼出 [確定負]・P2 fresh payload の frame 出現 [確定負]） |

---

## 2. 主要命令列（証拠）

### 2-1. P1: dispatch の 12B 単一読取と flags ゲート連鎖（v328_10/15 整列）
```
465C73: push 0 / call 0x5ad980
465C7A: mov edx,[esi+4] / push 1 / lea eax,[esp+0x20] / push eax
465C88: push 0xc / lea ecx,[esp+0x2c] / push ecx / push esi   ; n=0xc・dst=&[esp+0x1c..0x27]
465C90: mov [esp+0x30],1 / call edx        ; ★(stream,dst,n=0xc)=12B 単一 read・read=[esi+4] ファンクタ直呼（45BB00 非経由）
465C96: mov edi,[esp+0x34]（=dst+0 formID）/ cmp edi,0xfeffffff → Player 特殊枠（1B+4B→[Player]+0x116/+0x700）
465D07: call 452180 検証 / 0 時 'Form with ID %08X…'(0xA3AFF8) → seek -tailSize（0x465D2F）
465D44: call 45a140 / 465D52 call 46b250（GetForm）→ ebx
465D59: mov eax,[esp+0x25] / 465D5D add esp,4
465D60: test al,2 je 0x4660ea / 465D68 test al,4 je 0x465eb1
           ; ★bit1/bit2 の test 語義＝pass2 0x461CBC `test bl,6`・v312 件4「12B+5=台帳 changeFlags」と同物 [確定]
           ; （本窓の esp スロット ↔ヘッダ buf+5 の枠番対応は push 漂移含みで枠番のみ [部分]。0x46681B 側の枠番（+5）は v312 で確定済）
465D7A: cmp byte[0xb33b00+0x7c],0x5b（ver）
465D88: test eax,0x4000000(bit26) → push 4 → 0x465D97 → u32+movsx byte×2（0x465D9C-0x465DA6）
465DB1: test eax,0x2000000(bit25) → push 6 → 0x465DC0 → movsx word×2+u32（0x465DC5-0x465DD3）→ 459990（u16 idx）
465DF4: (ver<0x5B) push 0xc → 0x465DFF → 459950（seek -0xc でロールバック 0x465E3A）
465EB1: (bit1∧¬bit2) push 0x24 → 0x465ED6（36B 読・dst=lea ecx,[esp+0x78]・459950×2 @0x465EE2/0x465EF2・seek 補正 0x465F00 系）
4660EA: (¬bit1) test eax,0x80000000(bit31) → 立: push 0x2c → 0x466113（44B 読・459950×2・seek -0x2c）＝ created 11×u32（P11 Δ=0x2C と一致）
4662F0: (¬bit1∧¬bit31) ★この分岐は GetForm 失敗（ebx==0）時のみ発火 — 452D10 map find（entry bit31 立確認 0x46630D）→ entry tail+4 から **11×u32(44B) rep movsd 再構成**（`mov ecx,0xb`@0x466327）→ 459950×2 → sar 0xC（グリッド変換）→ 4EFE80 cell 検索 → 43F8C0 述語 → 45C4F0/4D35D0（v328_18 新規・命令列 [確定(命令)]）。再構成 44B と save 側 Δ=0x1C（=28B）は**サイズ不一致＝同一ブロックの直接対応は不成立**（created 引用再構成の別機構 [推定]。Δ=0x1C のロード側直接対応は次束残件 §6-10）
466816: count=tailSize u16 → payload 連続読取（v317 N4 踏襲）
```
- bit マスク粒度教訓遵守: 0x8000000=**bit27** / 0x4000000=**bit26** / 0x2000000=**bit25**（0x200=bit9・0x400=bit10 との混同なし）。

### 2-2. P4/P11: pass2 歩行・ゲート・Δ（v328_05/12/13/16 整列・頭 0x461680）
```
461680..: SEH prologue → TLS+0x185 クリア → new(0x25×4) tmp alloc（'Save Game Header' 系ログ 0xA3906C/0xA3AAB8）
4617C8: test ecx,0x8000000（=value[0] bit27）/ je 0x461a0d  ; ★外周ゲート（452600 next-entry 歩行）
461A70: call 0x452600 → [esp+0x14]=entry・[esp+0x24]=flags → 461A7B eax=[entry+4]=old tail
461A96: tail==0 → je 0x461bcc（作成系探索: 46B250 GetForm→formType∈{31,32,33}→4D6670/6ECC80 parent/worldspace→4C97F0/4C9CF0/4C9A80/4C9AA0 グリッド）
461AA5: cursor=[SaveGame+0x14] 張替 → 4B 読→[esp+0x30]＝tail フレーム（u16 size@+0x30・char@+0x32・typeID@+0x33）
461AB9: cmp char 0x31/0x32/0x33（＝created 系のみ 461AD0 以降）
461AD4: test al,2 → 36B 読（push 0x24）
461B1B: test al,0xc → jns→push 0x1c（28B）/ js→bit31 除去＋push 0x2c（44B）
461B8C..: 位置 2 組 sar 0xC → 446B80 → [SaveGame+0x74] 超過→[+0x14]=0 → 55E000 map find → (x,y) マッチ走査 → 不一致→452DF0 系削除
461CB3: test bl,8 je 0x461dec / 461CBC test bl,6 jne 0x461dec / 461CC5 cmp ebx,8→452DF0 削除（0x461CD7）
461CF4: and ebx,~8 / or ebx,0x800 → Δ（0x461D05 jns→0x1c・0x461D11→0x2c）→ sub ax,di（0x461D22）→ alloc（0x461D4D）→ 3 append（0x461D61/1D7B/1D94）
```
- v317 §4-8 と全 VA 一致（追認）。実データ照合（v320 §4-3）: bit27=3,010 / bit3∧(bit1|2)=1,679（0x461CBC 生存経路）/ flags==8 187（bit27=0＝歩行外・無加工書出）/ bit11=0 — **全数整合**。

### 2-3. P3/P8/P9: 保存側の dir・セクション連鎖・trailer/index（v328_04/07/09/14/17）
```
465230: call 0x45c870  ; masters（[[0xB33A98]+0x1C] 名前 u8len+B・count 1B・'Plugin List' 報告）＝ v320 実測 ✓
4652C1: call 0x462280  ; dir＋セクション連鎖:
  462290: ecx=[[0xb33a98]+0x8c0] → 4B 書 ＝ dir[0]（822,982）
  4622D2: ecx=[0xb333a0]; call 0x43f270（=`mov eax,[ecx+0x74];ret`）→ mov eax,[eax+0xc] → 4B ＝ dir[1]（5,527）
  462310: [0xb333a0]+0x20/+0x24 各 4B（0xFF000800 / 60）
  46234F: 4B（0）→ 46237B: 4d6670/6ecc80（Player parent cell/worldspace）→ formID 4B（0x1FBB9）/ -1 系 slot
  4623CA: [Player]+0x2C/0x30/0x34 → lea ecx,[eax+edx+0x24]… 0xC 書（f32×3 位置 ✓）
  462456: [0xb333a0]→441000（u16 95）/ 4410D0 buf 760B（終端 148,555 ✓）→ 677010/542EA0/5406F0/540720 …（u16+buf 連鎖・'Quick Keys'/'HUD Reticle'/'Interface'/'Regions' ログ）→ 表②212（終端 150,364 ✓）
465717: dir2=[stream+0x30]（-1→[+0x148]）→ 46574B: call 0x45e220
45E220: [u32 [[esi+0x74]+0xc]][×u32][u32 [[esi+0x78]+0xc]][×u32]（+0x40!=0 時 'Numeric ID Array(%i)'/'WorldSpace ID Array(%i)' 報告）
実 fit: cnt1@822,978=0 ・ cnt2@822,982=3,437 ・ entries@822,986..836,734=footer 一致（v328_17・全 start 候補 822,974/976/978/980 のうち単一 fit）
46571A: call 0x45fb50（[0xb3bd00]→679480 count → u32 書＋buf）＝ 先頭 u32 2（@822,976）候補
466829 側 reader: 0x462b20（唯一 caller 0x465C4D・dir 読→0xff000800 クランプ→[[0xb33a98]+0x8c0] 張替・mod byte3 再マップ→46B250 解決）＋ 0x45e3d0（+0x74/+0x78 クリア）
```

### 2-4. P5/P6/P10: ヘッダ reader 返却・SYSTEMTIME・時刻（v328_01/03/06/10）
```
; ★P5 返却算術（E0 = 0x45DD69 時点の esp として完全追跡・v328_01）:
45DD69: eax=[esi+4] / push 1 / lea ecx,[esp+0x14]（→E0+0x10）/ push ecx / push 4
        / lea edx,[esp+0x20]（→E0+0x14=dst）/ push edx / push esi
45DD7B: mov [esp+0x24],1（=E0+0x10 の got 書数スロットを 1 で初期化）/ call eax  ; read(stream, dst=E0+0x14, n=4, &got=E0+0x10, 1)
; 独立 esp 再追跡（v328_19）: E0=X とすると dst=lea@45DD75→X+0x14・return 路 esp=X+4 の [X+4+0x10]=[X+0x14]＝dst と严格一致
45DD87: add esp,0x14（esp=E0 回帰）/ 45DD8C: pop ebx（esp=E0+4）
45DE08: mov eax,[esp+0x10]（=E0+0x14=★dst スロット=size）→ 45DE23: ret 8
45DDFF/45DE03: cmp al,2 / jne→ or eax,-1（失敗返却。0x45DC7C 経路も xor eax,eax/jmp 0x45DE0C で返却スロット同一）
→ v307「値は戻されない」否定: size 読の dst と return 路が同一スロット（E0+0x14）で一致
45DC1B: strncmp('CON '@0xA3A954) 一致 → seek SET 0xD000 → [edi+0x8c]=0xD000 → magic 再読（v307 機構追認）
464AB0: 464B7F cmp [esi+0xA4],0 / jne 464ba1 → 464B95 call [0xa28108]（GetSystemTime,&[esi+0x94]）→ [esi+0xA4]=125
464C93-464CCB: 402BD0→fdiv[0xA2F920]（24.0）→402C00→（負時 +2^32 fadd [0xA2FC78]）→ 464CD1 fadd frac → 464CDA fstp（F5）
464CDE: call [0xa28174]（GetLocalTime）→ F7 書出 0x465044（ゲート=dry bit9 計上のみ）
IAT xref 全走査: GetSystemTime=[0x464B97, 0x748B59] の 2・GetLocalTime=[0x464CE0, 0x50A4B5, 0x748B64]
```

### 2-5. P2/P12: 経路 A フレーム分解・thumb（v328_06・10B buf=E+0x38=[esp+0x44]（0x4653B0 lea）基準で枠番再追跡）
```
465360: ecx=[SaveGame+0x14]（cursor） / edx=[ecx]（旧 tail 先頭 4B）→ E+0x6C・465369 [SaveGame+0x14]+=4
46536D: al=[E+0x6E]（=旧 tail byte2＝char/型区分）→ 465375 mov [esp+0x3c],al ＝ buf(E+0x38)+4 ★ファイル +4 formType 枠へ
46537E: cl=[E+0x6F]（=旧 tail byte3＝typeID）→ 46537E/465382 mov [esp+0x41],cl ＝ buf+9 ★ファイル +9 枠へ
→ push 0xa 10B 書出（0x4653AE）→ push 2 tailSize u16 2B（0x4653DF・値=[E+0x6C]=旧 tail u16）→ payload＝tail[4:]×tailSize（0x465405-465419）
（経路 B・tail==0: 465489 al=[form+4] formType→46537E 系と同じ +4 枠 / 46534A al=[ebp+0x7c]=0x7D→+9 枠（両経路の +9 共通初期値＝v317 観察11 の「tail==0 +9=0x7D」「tail≠0 +9=旧 typeID」を命令で確定））
→ v283 frame の 3 フィールドはファイル上 +4/+9/+10 へ分散、payload 先頭に frame は残らない（＝v320 §6-9 実測差の機構 [確定]）
4650AF/4650BE: 45BAB0（4B×2=thumbSize・w/h）→ 4650C3: eax=[[ebx+0x5c]] / ecx=[eax]+[ebx+0x50]（ppBits+pitch）→ 45BAB0 pixels → 4650E0: call [[ebx]]（release）
```

---

## 3. 帯遵守宣言

- 深解析: **v316 自帯のみ** — `0x459000-0x460000`（45B9A0 引用/45C4A0/45C870/45D5F0 窓/45DBC0/45E220/45E3D0/45F820・45FB50）・`0x464000-0x467920`（464AB0 窓/464320/465130 ループ前後/LoadGame 0x465860 系窓（0x465994-0x466830）/466AD0 系外）。
- `0x460000-0x464000` の pass2/serializer（0x461680-0x461E90・0x460710・0x460BC0 文脈・0x462280/0x462B20＝**「0x462080 系」許可窓**）: v320 §7-2 の P4/P11/P2/P8/P9 指定 VA そのもので、v317/v319/v323 が自帯として公開済みの窓に限定（0x4C0000-0x4E0000 深帯・v318 自帯は深解析 **0 件**）。446B80/4D6670/6ECC80/4C9xxx 帯（0x4C0000-0x4E0000）は既刊 artifact 引用または head 1-6 命令の**浅層照会のみ**。
- .data/.rdata（IAT・文字列・imm xref）走査は読取のみ。実 .ess は読み取り専用（原本無変更）。`tmp/re/re_util.py` 無変更。

## 4. 教訓適用記録

- off-by-pad 7 例準拠: 462280 head pre16=`e8abfcf9ff 83c404 8bc6 5ec20400`+CC×6 — `c2 04 00` 終端＋CC ラン 6 ✓、先頭 `83 ec 28` で新関数開始（find_head 誤答なし）。45C870/45FB50/45E220/45C4A0/45E3D0/462B20 も pre16 CC ラン十分＋E9/EB/jcc 終端確認（fn-end walker rule v3）。
- bit マスク粒度: 0x8000000=bit27・0x4000000=bit26・0x2000000=bit25・0x800=bit11・0x80000000=bit31・0x10000=bit16 を §2 全域で明示区別（v326 教訓＝0x200/0x400 混同なし）。
- CC ラン不足型誤答回避: pass2 分岐先 0x461A18/0x461BCC/0x461E8C は頭 0x461680 起点の整列到達でのみ使用（線形スナップショット誤読排除）。帯境界 +0x400 教訓は本 lane の窓選択に反映。

## 5. 訂正節（旧結論は不削除・両論併記）

| # | 旧記載 | 出典 | 本件裁定 | 根拠 |
|---|---|---|---|---|
| 1 | 「LoadGame 12B = **4+6+2 の分割読取 [推定]**」 | v320 §7-2 P1 申し送り | **[確定負]（訂正）** | 12B は count=0xC 単一 read（0x465C88-0x465C94・§2-1）。4/6/0xC/0x24/0x2C は flags 別ペイロード読取で、ヘッダ 12B の分割とは無関係 |
| 2 | 「0x45DD83 の 4B は FUN_45DBC0 内で値は戻されない [部分]」 | v307 残点4 | **弱化（返却行の実在で旧記述は不成立。dst スロット同一性のみ残件）** | 終端 `0x45DD8C pop ebx → 0x45DE08 mov eax,[esp+0x10] → ret 8` が明示返却（§2-4）。4 caller の size 消費・-1 対。ただし [esp+0x10] が dst か got かは read ファンクタ（FUN_4302B0・v307 で内部未解析指定）の cleanup 規約に依存 → §6-9 残件。機能命題（返却値=u32@34）は consumer 鎖＋実測で成立 |
| 3 | 「bit3 単独 entry は pass2 で削除され非書出 [確定]」の一般化 | v317 §4-1 N1 | **適用範囲訂正（機構は維持）** | pass2 歩行ゲート＝bit27 立限定（0x4617C8）。bit3 単独∧bit27 立→削除（v317 の事実はこの文脈でのみ成立）／bit3 単独∧bit27 消→無加工書出。**v320 §6-7 の反例 187 件と完全解消** |
| 4 | 「v283 frame と payload 先頭の対応＝実測差（Phase B 待ち）」 | v320 §6-9 | **[確定(命令)] へ昇格** | frame＝旧 tail 専用・書出時に +4/+9/+10（formType/typeID/tailSize）へ分解（§2-5）。fresh payload に frame が現れないのは機構的帰結 |
| 5 | 「+0xA4==0 のときのみ GetSystemTime [推定]」 | v307 残点4・v313 | **[確定(命令)]** | 0x464B7F ゲート＋IAT 全走査で保存窓の呼出 1 点のみ（§2-4） |
| 6 | 「2 SYSTEMTIME の用途差 [推定]」 | v320 §3.1 項2 | **[確定(命令)]** | UTC=初回ゲート式／local=無条件毎書（P10） |
| 7 | 「Δ=0x1C/0x2C の中身 [未確定]」 | v317 §6-1・v319 B1・v320 §6-8 | **[確定(命令)]（要素名は [未確定]）** | pass2 が 28/44/36B を created 位置ブロックとして読→グリッド解決専用→再発行 tail から除去（§2-2）。v317 の数式レイアウトは追認 |
| 8 | 「index entries はチェーン外別 formID 群か別意味 [未確定]」 | v320 §4-2 | **[部分確定] へ更新** | = [SaveGame+0x78] 'WorldSpace ID Array' の要素（45E220 書・462B20 が mod byte3 再マップ→GetForm 解決で読）。要素の個別意味（どの WS/cell 参照）は残 [未確定] |
| 9 | 「中間 32B（147,761..147,793）[未確定]」 | v320 §4-2 | **[部分確定] へ更新** | 462280 冒頭の u32 群＋[Player]+0x2C.. f32×3 位置（0xC 書）と formID（parent cell/worldspace）[確定(命令)]（§2-3）。0xFF000800/60/0/-1 の変数意味は残 |
| 10 | 「pass2=保存直前（v317 N3）」「v316 項目/実測値（counter・size・F1..F8）」 | v317/v320 | **追認（変更なし・引用のみ）** | 本 lane は再測せず §2 の文脈整合でのみ使用 |

## 6. 新規残件（次束・追加サンプル向け）

1. **[未確定] trailer 先頭 6B（`02 00 00 00 00 00` @822,976）の帰属** — 45FB50 'Temp Effects List' count（u32）＋0 バイト buffer 候補だが、count=2 と buffer 空の契約（679480/679630・帯外浅層）と 45E220 fit（@822,978）の 2B 位相を突合して決着要。
2. **[未確定] dir[1]=5,527 の二重解釈** — writer 側は [[B333A0]+0x74]+0xC（count）確定、loader 462B20 は同 u32 を formID 0x1597 として mod 再マップ→GetForm→cast([0xB0301C])→4431F0 で解決。「count と formID が同値」の正体（save#2/複数セル検体で判別）。
3. **[未確定] P6 の実効果** — 2 回目保存（別セッション要）で +0x94 が frozen か（静的ゲートは確定済）。
4. **[未確定] P8/P9 のセクション↔値 1:1・[0xB333A0] の正式身元** — 441000/4410D0・542EA0/5406F0/540720・5A8250/57BE10・4A2FF0・677010 群の個体同定（帯外・浅層留保）と、s68（playerRef 説）/s65（NavMesh 説）/本件（count/セクション供給元）の不一致解消。GLOB/GMST 正式名付けもここで決着。
5. **[未確定] dir[0]（822,982）供給元** — [B33A98]+0x8C0 に chain end を入れる書込側（45F0F0 生成リストの計上か）。ロード側クランプ 0xFF000800 は実測済。
6. **[未確定] P11 の Δ 7×u32 要素名**（pos x/y/z＋4 余剰）と P1 の Player 特殊枠（formID 0xFEFFFFFF）の意味。
7. **[未確定] pass2 tail==0 分岐（0x461BCC）の帰結全量** — 探索失敗時 `[ebx+0x14]=0` 後 entry の成否（削除/無効化）とファイル帰結。
8. **[未確定] P12 チャンネル順（RGB/BGR）** — passthrough のため静的原理不可。CON 実機突合または 0x70E560 ctor 窓（D3D フォーマット）の深掘り（0x70E560 は帯外）。
9. **[未確定] P5 size 返却の dst スロット严格同一性** — 返却行 `0x45DE08 mov eax,[esp+0x10]` の実スロット（size read dst 疑惑 E0+0x14 と out/got スロット E0+0x10 のいずれか）は read ファンクタ（FUN_430050→**FUN_004302B0**・v307 で「内部経路未解析 [部分]」と明記済）の 5 引数 cleanup/esp 収支規約に依存。FUN_004302B0（帯内 0x430000 帯）の全文解析で決着（本 lane では返却行と caller 消費のみ確認）。機能命題（返却値=u32@34）は v313/v320 fill 実測鎖で間接成立済。
10. **[未確定] Δ=0x1C（28B）除去ブロックのロード側直接対応** — dispatch の created 系再構成は 44B（0x2C/rep movsd ecx=0xb）でサイズ不一致（§2-1）。28B 経路は 0x465EB1 の 0x24=36B 帯に包含される可能性（bit1 系）を含め、created 系 flags 組合せ（bit1=0∧bit31=0∧型 {31,32,33}）の実 entry が本検体に無いため動的/追加サンプル待ち。

（新規残件 = **10 件**）

## 7. probe 一覧

| script | 対応 P | 内容 | 主結果 |
|---|---|---|---|
| v328_01 | P5/P6 | 45DBC0 全文＋callers 4 文脈＋464320 全文 | 0x45DD8C/0x45DE08 返却確定・'CON ' 追認 |
| v328_02 | P1 | 45BB00 全 site 整列文脈（4/6/0xC/0x2C/0x24＋消費側） | §2-1 ゲート表 |
| v328_03 | P6/P7/P10 | 464AB0 SYSTEMTIME/F5/F7 窓・ctor 尾・[+0xA4] 書込点 .text 全走査（492 候補→整列精査） | ゲート・÷24・frozen 候補 |
| v328_04 | P3/P8 | 45C870/462280/45FB50/45E220 head 検証＋全文＋callers（各 1: 0x465230/0x4652C1/0x46571A/0x46574B） | dir/trailer/index writer 連鎖 |
| v328_05 | P4/P11 | pass2 0x461680-0x461CB0 整列全文 | 外周 bit27 ゲート・Δ 読窓 |
| v328_06 | P2/P7/P12/P1 | 45C4A0 全文＋経路 A 4B 読窓＋F5..F7＋thumb 書出窓＋dispatch 0x466090 | フレーム分解・ppBits・0x2C 入口 |
| v328_07 | P8/P9 | 462280 本体 0x462280-0x4627E0（dir 8B・cell/Player・セクション） | §2-3 |
| v328_08 | P2/P11 | 実 .ess フレーム検分（numpy ベクトル・読み取りのみ・walk 822,976 再確認） | `u16@p0==tailSize-4` **0/5,527** |
| v328_09 | P8/P9 | 462280 尾＋文字列＋被呼出 head 浅層 | 'TES Class'..'Interface' 順・441000/4410D0 |
| v328_10 | P1/P3/P10 | dispatch 0x465B60 整列（12B 単一読・Player 枠・seek 対）＋IAT xref 全走査 | 0xC 単一 read・GST×2/GLT×3 |
| v328_11 | P3 | ESS dir/trailer/index/footer 実バイト＋45E220 fit 走査 | fit @822,978（822,974/976/980 は不成立） |
| v328_12 | P4/P11 | pass2 0x4619C0-0x461CB3 詳細 | 28/44/36B 読・sar>>12・55E000 find |
| v328_13 | P4 | 0x461C60-0x461D61 精确（cmp ebx,8 / Δ 選択） | v317 追認＋整列再確認 |
| v328_14 | P3/P1/P5 | 462B20 全文（dir/index reader）＋45E3D0＋pre-loop 0x465A60（0x800/0x40000・462080） | mod 再マップ対称・462080 呼出位置 |
| v328_15 | P9/P1/P5 | セクション名文字列・0x465E66 整列（0x24）・0x4640EA 窓・462280 push 文字列列挙 | 'Numeric/WorldSpace ID Array'・消費側 |
| v328_16 | P4 | pass2 頭部 0x461680-0x461800＋452600 本体 | 歩行=452600・bit27 ゲート位置 |
| v328_17 | P3 | trailer/index 実バイト最終突合（start 822,974/976/978/980） | @822,978 単一 fit・e[0]=0/e[1]=0x1000CE7 再現 |
| v328_18 | P1/P11 | dispatch ¬bit1 後段 0x4662F0-0x466480 整列＋窓内 45BB00 site の push imm 全列挙 | **0x4662F0=44B rep movsd 再構成（ecx=0xb）**・push sizes={4,6,12,44,reg×3} |
| v328_19 | P5 | FUN_45DBC0 の prologue/size read/return の再 dump（esp 独立再追跡） | dst=E0+0x14=返却 [esp+0x10]@E0+4 と严格一致 |
| （v328_idx） | — | v328_03.out（5,865 行）目録ヘルパ | 節索引 |

- 実行形: `chcp 65001` → `python %TEMP%\opencode\v328_NN.py *> %TEMP%\opencode\v328_NN.out`（全本 EXIT=0）。
- **Numba 不使用**: 全 probe は capstone disasm / bytes.find / struct / 5,527 件 walk 主体で JIT 回収不能域（指示どおり）。重い機械走査は発生しなかったため出番なし。v328_08 の全件統計のみ **numpy ベクトル化**（numba 0.68.1 import 可は確認済・未使用）。

## 8. v320 §7-1（file 側残件 12）への影響

- 残件 1（index 意味）→ P3+§5-8 で [部分確定]／残件 2（trailer）→ §6-1／残件 3（dir）→ P3+§5-9 で [部分確定]／残件 4（中間）→ P8／残件 5（global 2 表）→ P9／残件 6（F5）→ P7／残件 7（+0x94 更新）→ P6（ゲート命令確定・実効果待ち）／残件 10（二重書き意図）→ P10 [確定(命令)]／残件 11（Δ 内容）→ P11 [確定(命令)]／残件 8（RGB/BGR）→ P12 で「静的原理不可」と裁定（動的専用へ明確化）。残件 9（name2 語彙）と footer 8B のみ file 側追加サンプル待ちのまま。

### 自己確認

v328: (最終行集計は報告文に実測転記)
