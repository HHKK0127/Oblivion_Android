# v326 — v322 §4 残件 6 件の裁定（次束M / ai-todo #90）

- 出典: `tmp/re/v322_v316_v320_next_bundle_I.md` §4（残件 6 項）＋ §9 親訂正（seek 項消滅・thumb 確定は前提）
- 静的系: Oblivion Steam 1.2.0416（`re_util.EXE`・読み取り専用）。v316/v320/v323/v325 の既知を継承
- 判定: `[確定] / [部分確定] / [推定] / [未確定] / [確定負]`
- クリーンルーム: OpenTES4 / xOBSE / COEF 未参照。docs 交差は本プロジェクト RE_EXE_REPORT.md の既存記載のみ

## 1. 判定表（6 件・5 段階）

| # | 項目 | v322 時点 | 本束判定 | 一行結論 |
|---|------|----------|---------|----------|
| 1 | dry bit9 の間接 setter と実行時発火条件 | [未確定]寄り | **[部分確定]** | **静的書手の全階層網羅 census を完了**: 定数マスク RMW（dword `0x200/~0x200`）=0 件を再確認 [確定負]、レジスタマスク RMW の obj サイト 116 件を全列挙し bit7/bit0/bit10/bit13-15 の対と特定（bit9=0x200 を証拠上立てるサイトは存在せず）。**新規発見: 隣接 bit10 の SetBit/ClearBit 対 0x5439CE/0x5439E7**（fn 0x5437C0・条件 `[SaveGame+0x7C]<0x69`・唯一 caller 0x4633AE）。dry の発火は実行時 this 引数 setter か一括書込（new/ロード経由）に帰着＝動的 lane 残り |
| 2 | `[0xB38720]` の初期化元 | [未確定] | **[部分確定]** | **静的初期化元を特定**: 0x9F10E0 の名前一対レコード `ecx=0xB38720 / push 0xA5E820('Level') / push 0xA5E808('sMenuDisplayLevelString') / call 0x419A70（登録 ctor: [esi]=name ptr, [esi+4]=value ptr ＋ グローバル連結リスト 0xB35574 への link）/ push 0xA21560 / call 0x981FB4`。近隣 0x9F10C0/0x9F1100/0x9F1120 が 'Autosave'/'Unknown Location'/'Day' の同型レコードで連なり、**FUN_459570 arg3 の sprintf `%s%s` 接頭辞＝ Game Setting「sMenuDisplayLevelString」系**。ただし実行時に .ini/設定系が上書きし得るため**実最終値**は [未確定] のまま（動的 lane） |
| 3 | FUN_004610F0 の受け先型と thumbnail writer 対応 | [部分確定] | **[部分確定]**（前進） | 呼び手が **2 件に確定**（E8 raw 再走査: 0x5AE34F・0x5D312F。v316「1 件」は 45D5F0 窓の数え上げで別母数）。両者 `mov ecx,[0xB33B00]`（ecx=SaveGame）・`push 0; push esi`（arg10=esi 非 NULL→ thumb tail を読む）。受け先=呼び手側ローカル（esp+0x3b4 / esp+0x158 等）への **dword 出力先**で、返り値 esi=eax は直後の `0xA6C6A4='%s\n%s\n%s\n%s\n%s'` フォーマット窓（メニュー一覧行の 5 行組立）へ消費＝**セーブ/ロード一覧行のサムネイル欄と対応**。正式型名は RTTI 非在で [部分確定] 維持 |
| 4 | 'BSX'/'REF' 対象の RTTI・vtable | [部分確定] | **[確定]** | **RTTI TypeDescriptor を実体特定**: `.?AVBSXFlags@@` @0xB253DC（TD 頭=**0xB253D4**・vftable ptr=**0xAA3E80**・spare=0）／`.?AVTESObjectREFR@@` @0xB0305C（TD 頭=**0xB03054**）。'BSX'→BSXFlags・'REF'→TESObjectREFR。v316 の `[+0xC]>>4&1` 検査＝BSXFlags のフラグ bit4 に整合し、'REF' 側の `[ebx+4]` InterlockedDecrement＝TESObjectREFR リファレンスカウントと整合（v316 §4-4 の 2 経路が正式クラス名に接続） |
| 5 | 0x5D8FD0 の比較対象フィールド正式意味 | [部分確定]（§9: 甘め） | **[部分確定]**（充实） | **関数全体を初読取**: head=0x5D8F00 の callee。0x5D8FD0 本体=①`[0xB3B734]` bit7→±1 の**ソート方向符号**②`(b734&0x7F)` で case 分岐: case0=`[+0x1C]` の**名文字列 strcmp**（NULL→空文字 0xA2F7EC）／case1・2=`[0xB333C4]` 単体経由で `[+0x24]` サブオブジェクトの `vtable[0](arg)` 数値比較（case1 のみ `fmul [0xB37FE0]` スケーラ）③3-way 返却×符号。契約は確定級、**正式フィールド名**（+0x1C の名前の種類・+0x24 のサブ型・0xB3B734 の設定名）は [未確定] 残。v322 §9「候補列挙のみ」→ 全体読取で [部分確定] が妥当水準へ（§4 C2） |
| 6 | menu ID {0, 3, 0x3F5, 0x40F} の表示名 | [部分確定] | **[部分確定]**（2/4 は確定） | **0x3F5 = PauseMenu [確定]**（RTTI `.?AVPauseMenu@@`/XML 名簿 docs L5655・名称 fn 呼び手 0x58A567）／**0x40F = SaveMenu [確定]**（docs L5678・0x58A688）。**名称解決 fn を新規発見**: 0x589B70(id)= `[[0xB13974]+(id-0x3E9)*4]`（範囲 check `[id-0x3E9]<0x33` かつ `<件数 [0xB1397A]`）＝**メニュー名テーブル**（BSS・実行時構築、ID 空間 **0x3E9..0x41B** と docs §3.1 の 59 メニュー表が完全一致）。**ID 0/3 は名テーブル空間の外**＝「スナップイン未設定/0」で表示名なし [推定]。getter 0x578FE0=スナップイン配列（`[[SaveGame+0x68]+0xDC..]` 10 slot）の最大 ID（<0x3E9→0）／0x579060=slot[0] を新規確定（§3-6） |

**集計**: [確定]1（#4）／[部分確定]5（#1,2,3,5,6）／[推定]0（主判定）／[未確定]0（残部は各項目に内包）／[確定負]1（副次=#1 bit9 定数 RMW）

## 2. probe 一覧（%TEMP%\opencode\v326_*・全 EXIT=0）

| probe | 内容 | 主要出力 |
|-------|------|----------|
| v326_1_bit9.py/.out | .text 全 resync disasm（命令 2,257,850・gaps=137）で `[reg+0x18]` 全接触＋shr,9/test,0x200 | RMW reg=145 / RMW imm=569 / shr9=9 / test200=20 |
| v326_all.py/.out | 6 項横断（[0xB38720] imm・FUN_4610F0 callers・BSX/REF キー xref・0x5D8F00 域・0x3F5/0x40F occurrences・getter callers） | 各 §3 |
| v326_2.py/.out | getter 0x578FE0/0x579060 全文・文字列（0xA6C6A4/0xA7D0EC='BSX'/0xA3CEB0='REF'/0xA2F884/0xA3AEB8）・0x589B70 発見 | §3-4,6 |
| v326_3.py/.out | 名テーブル [0xB13974]=BSS 確認・RTTI 全文走査・0x5D8FD0 tail（0x5D9100-0x5D9240）・[0xB3B734] imm 4 件 | §3-2,4,5 |
| v326_4.py/.out | bit9 候補 6 サイトの文脈dump・[0xB33B00] 参照 census | §3-1 |
| v326_6.py/.out | 0x419A70 ctor・初期化系列 0x9F10C0-0x9F112F・[0xB33B00] 直接書込=0件・文字列 'Level'/'sMenuDisplayLevelString' 等 | §3-2 |
| v326_7.py/.out | **bit9 定数演算の粒度別全列挙**（gran=8/16/32 正確分離・368 サイト）＋reg 書込 4538 件を SAVEBAND/NEAR-B33B00 でタグ付け 234 件 | §3-1 |
| v326_8.py/.out | bit9 サイト精査（0x5439CE/0x5439E7 確定・0x9847D6/0x9997F0=x87 例外 sentinel 棄却）・名表登録窓（0x65C620=linkedList 追加） | §3-1,6 |
| v326_9（インライン） | 0x5437C0 head/callers・bit9 全再走査（`or,0x400`=0x5439CE のみ／`mov,0x400`=0x74887F 別帯／`and 0xfffffbff`=0x5439E7） | §3-1 |

## 3. 各項の根拠

### 3-1. 項目1: dry bit9 — 静的書手の網羅 census（打ち止め宣言）

bit9 = マスク `0x200`（読側: 0x464E63 `je` 窓・0x465159 `shr eax,9/test al,1`＝v316 既知）。v316 は「`[reg+0x18]` への or/and/xor/bts/add/sub で 0x200/0xFFFFFDFF を即値とする命令 0 件」まで。本束で候補階層を全部並べた:

1. **dword 定数 RMW**: 粒度を正しく分けた全走査（v326_7）で `or/and/xor [obj+0x18], imm(bit9 関与)` の dword サイト=**0 件**（imm 書込は全 368 件のうち frame slot `[esp+0x18]` と SE sentinel・他クラスのみ）→ v316 追認 **[確定負]**。
2. **dword 一括書込 `mov [obj+0x18], imm`**: bit9 を含む即値は `0x9847D6/0x9997F0 = mov [esi+0x18],0x200` の 2 件だが、文脈（0x9840E0 系・`mov dword ptr [eax],0x16`=SE 型 22 書込・0x98540B/0x98C5A0/0x98ED40/0x9932CD 群）が **_except_handler3/__CxxFrameHandler3 スケルトンの除算例外処理**であり、SaveGame と無関係と確定 [確定負]（新規残件 R6）。
3. **word 演算**: `or word [eax+0x18],0x200`（0x5533A0/0x55EB19）は `call [0xA2807C]/[0xA28078]`（ハンドル走査系・Win32 ハブ）＋`[ebp+0xdb0]`/`[esi+0x14]` の handle 格納窓＝**ファイル/プロセス handle の flags word**（SaveGame でない）[確定負]。`and word 0xfdff`（0x561269）も 0x719A20/0x561030 経由の handle 窓。
4. **レジスタマスク RMW（obj 116 件）**（v326_1 全列挙・v326_7 タグ）: SaveGame 単体窓で意味のある対は — bit7: `0x444588/0x4445B0`（esi=0x80）と `0x4CDA46/0x4CDA74`（edi・v323 第 2 サイト）、bit0: `0x465AE4`（edi=1）、bit13/15: `0x40D554/0x40D56B`（and 0xffffdfff/0xffff7fff）、**bit10: 0x5439CE/0x5439E7（新規・下記）**。うち**実行時に edi/esi==0x200 となり得る証拠はなし**（0x465AE4 は直前 `mov edi,1`、0x5439CE は即値 0x400）。
5. **[0xB33B00] への直接書込（dword imm/reg パターン c7 05 / 89 05）= 0 件**（v326_6・SaveGame は new 経由）。単体窓の `mov [reg+0x18], reg`（NEAR-B33B00 タグ 234 件のうち obj ベース）は load 直後の `shr,9` リーダ側（v316 0x465159 系と同型）で書込は別名（frame slot）。
6. **新規発見（bit10 対）**: `0x5439CE: or dword ptr [ecx+0x18], 0x400` / `0x5439E7: and dword ptr [eax+0x18], 0xfffffbff`、ecx/eax=`[0xB33B00]`。fn head=**0x5437C0**（CC 境界・head 検証済み）、**唯一 caller 0x4633AE**。発火ゲート=`0x54398E: cmp byte ptr [ecx+0x7c], 0x69; jb`（**[SaveGame+0x7C] < 0x69**）。dry(bit9) との混同禁止＝bit10 の pair（新規残件 R1）。
→ **結論**: bit9 を立てる**静的命令は .text に存在しない**（6 階層の網羅列挙・打ち止め宣言）。残る発火経路は「this 引数経由 setter の 8 命令超 RMW」の間接性、またはロード時の一括復元（`[+0x18]` を含むオブジェクト読込窓＝FUN_00465860 列）。動的観測（#82 経路）でのみ最終決着 [部分確定]。

### 3-2. 項目2: [0xB38720] の初期化元

- imm 参照 5 件再現（0x4596CF / 0x4611FE / 0x464129 / 0x9F10EB / 0xA21561 — v316 §4-5 と一致）。
- **初期化レコード@0x9F10DE**（v326_6）: `push 0xA5E820 / push 0xA5E808 / mov ecx,0xb38720 / call 0x419A70 / push 0xA21560 / call 0x981FB4`。文字列: 0xA5E808=`'sMenuDisplayLevelString'`・0xA5E820=`'Level'`。
- **0x419A70 = 名前付き設定スロットの ctor**: `[esi]=argA / [esi+4]=argB` 書込＋`[esi+4]` を link 先として**グローバル連結リスト 0xB35574** へ登録（`mov ecx,0xb35574; call 0x4524C0`）＋`0x4055E0` 型の finalizer トリム。隣接レコード列: 0x9F10C0=`'Autosave'/sMenuDisplayAutosaveName`・0x9F1100=`'Unknown Location'`・0x9F1120=`'Day'` — すべて `sMenuDisplay*` 系。
- 使用側との整合: FUN_459570 arg3 の `sprintf('%s%s', [0xB38720], seg)`（v316 §4-5）は中央セグメント接頭辞＝**Level 表示**（v316 §4-2 FUN_004610F0@0x4611FC の `sprintf('%s %i',[0xB38720],word)` と同一スロット）。
- 0xA21560=`mov ecx,0xb38720; jmp 0x4055E0` の**破棄トランポリン**（静的初期化登録 0x981FB4 の解除窓）— v316 記載を再現。
- 残部: Game Setting が .ini/SEH 読込で実行時に value 書換（[esi] 差し替え）し得るため、**転送先の「現在の」文字列**は動的観測待ち。初期化元そのものは確定した＝[部分確定]。

### 3-3. 項目3: FUN_004610F0（呼び手 2 件確定）

- E8 raw: **callers = 0x5AE34F・0x5D312F**（v316 §4-2 の「帯外 FUN_004610F0 に thumb 読取 caller 候補 1 件」は 45D5F0 引数窓の数え上げ。4610F0 自体の caller は 2）。
- 0x5AE34F の fn（head=**0x5AE242**・CC 境界検証）: 冒頭 `mov eax,[0xb33b00]; test byte ptr [eax+0x1a],1` ＋ tile trait 群（`0xfa1 visible`/`0xfde`/`0xA2F7EC` 空文字）＋ `'BSX'` 窓を含む 0x6FA853/0x6FA8C5/0x6FA97B の近傍帯（v326_all xref）＝**Load 一覧系メニュー表示 fn**。0x5D312F の fn（head=**0x5D2F92**）: `[ebx+0x38]/[+0x48]` 経由の trait 0xFB5 比較＝0x5D8FD0 と同型ソート窓＝**Save 一覧系メニュー表示 fn**。
- 両者 `push 0; push esi` → arg10=`esi` 非 NULL → FUN_45D5F0 の**thumb tail 読取が有効**（v316 §4-2 の「arg10=thumb 読取フラグ」と接続）。
- 返り値: `mov esi,eax` 後、`lea` 群（0x158/0x3b4/0x4d8/0x604）と `0xA6C6A4='%s\n%s\n%s\n%s\n%s'`（v326_2）— 一覧 1 行の 5 欄組立。受け先型は**メニュー行レコードの dword フィールド**（正式名 RTTI 無し＝[部分確定] 維持）。
- writer 対応: v320 確定 thumb=raw 8+256×192×3・pixels@103 の**読取側**が初めて実 consumer（メニュー 2 種）に接続した。

### 3-4. 項目4: 'BSX'/'REF' の RTTI（[確定]）

- TD 走査: `.?AVBSXFlags@@` 名 @0xB253DC → **TD 頭 0xB253D4**（先頭 4 バイト=vftable ptr 0x00AA3E80・次 4=spare 0）。キー 'BSX'（0xA7D0EC）/`'REF'`（0xA3CEB0）は**4 文字名**。`.?AVTESObjectREFR@@` 名 @0xB0305C → TD 頭 0xB03054。
- 呼出様式（v316 §4-4）: `'BSX'→0x6FF9C0` の返り `[+0xC]>>4&1`＝BSXFlags の属性 bit4（vtable[+0xC] で取る設計と整合）；`'REF'→0x6FFAC0` の返り `[ebx+4]` InterlockedDecrement＝TESObjectREFR リファレンスカウント。
- 辞書（0x6FF9C0 内部のキー検索・0xA2806C 経由ロック）が「4 文字名→TD」をどう引くかの中间構造は未追跡（残件 R4）。対象同定という問には完全回答 → **[確定]**。

### 3-5. 項目5: 0x5D8FD0 本体（初読取）

- 呼び手側 0x5D8F00 が esp+0x14/0x18/0x1C/0x20 に**比較対象 2 組**を積んで 0x5D8FD0 を呼ぶ（v326_all §S5 の head dump）。0x5D91D7（OUT）も同じミドルエントリ。
- 本体（v326_3 §7）:
  - `[0xB3B734]` bit7 → edi ∈ {+1, -1}（**ソート方向**）; eax=(b734&0x7F) 比較で case 分岐。
  - case0（eax=0）: `[a+0x1C]` / `[b+0x1C]` 文字列 strcmp（NULL→**0xA2F7EC 空文字列**）。
  - case1: `vtable[0]([a+0x24])`（arg=`[0xB333C4]` 単体）の戻り ×`fmul [0xB37FE0]` vs case2: `[b+0x24]` 経由（スケーラなし）。
  - 結果 `cmp esi,eax` → -1/0/+1 に方向 edi を掛けて返却（**3-way compare**）。
- 0x416650 の比較関数群（v316 §4-6）の一種として契約確定。正式名（+0x1C=どの名前・+0x24=どのサブ・0xB3B734=どの設定・0xB37FE0=何スケーラ）は [未確定] 残 → **[部分確定]**。

### 3-6. 項目6: menu ID 表示名

- **名称 fn**: 0x589B70(id)＝範囲ガード（`[0xB1397A]` 件数・`id-0x3E9<0x33`）の上で `[[0xB13974]+(id-0x3E9)*4]`。テーブルは **.data BSS（ファイル 0＝実行時構築）**。呼び手様式（v326_3）: `cmp word ptr [0xb1397a], N` ガード＋`mov eax,[0xb13974]; mov ecx,[eax+off]`（**名テーブル＝ポインタ配列＋件数**）。
- **ID 空間**: 登録走査 fn 0x57CFE0 系が `ebp=0x3E9` 下限／`[[SaveGame+0x68]+0xE4..0x108]`（10 slot）走査＝**0x3E9..0x41B**。docs RE_EXE_REPORT §3.1/メニュー名簿（59 件）と範囲一致 — 0x3F5=PauseMenu・0x40F=SaveMenu（L5655/L5678 の XML・class・handler VA 名簿）。
- 名設定窓の呼び手が同一 fn 内に 0x58A567=`push 0x3F5;call 0x589B70`・0x58A688=`push 0x40F;call 0x589B70`（v326_3 S6 xref）→ **表示名の取得経路そのもの**を確定。
- getter: 0x578FE0→**0x57CF60**（スナップイン 10 slot 走査の最大 ID、`[+0x104]` 経由、<0x3E9→0）／0x579060→**0x57CFA0**（slot[idx]、idx=0）。FUN_00452330 の「両 getter ∈{0,3,0x3F5,0x40F}」＝**スナップイン未設定(0) か Pause/Save 画面中** の述語として意味が閉じる（v316 §4-6 の extension）。
- ID 0/3: 名テーブル空間の外→表示名なし [推定]（生成元は UI スナップイン側の既定値。残件 R3）。

## 4. 訂正節（旧結論は削除せず併記）

- **C1（v316 §4-1/§8-1 の範囲限定の明記・追認）**: v316「dry(bit9) を立てる命令は .text に存在しない」は即値マスク RMW のみ。本束は 6 階層（§3-1）に拡張して再走査し、**結論不変＝[確定負] に格上げ**（word 0x200・SE sentinel 0x200 の 2 誤候補を文脈で棄却）。同時に**隣接 bit10 の setter 対 0x5439CE/0x5439E7 を新規特定**（v316 未記載・残件 R1）。
- **C2（v322 §9 所見項目5 の方向訂正）**: v322 は「0x5D8FD0 の [部分確定] は甘い（候補列挙のみ）」としたが、**本束で関数全体を初読取**し契約（入出力・方向・3-way）が確定側に動いた。所見は旧記録として併記。
- **C3（v316 §4-2 の数え上げ補足）**: 「FUN_004610F0 に thumb 読取 caller 候補 1 件」は 45D5F0 引数窓基準。4610F0 自体の caller は 2 件（0x5AE34F/0x5D312F）で完全。旧記述は削除しない。
- v320/v323/v325 と矛盾なし。

## 5. 新規残件（次束用・#93/#106 との住み分けを明記）

1. **R1**: bit10 SetBit/ClearBit 対（0x5439CE/0x5439E7・fn 0x5437C0・caller 0x4633AE・条件 `[SaveGame+0x7C]<0x69`）の意味。0x7C バイトの同定（v318 帯外隣接に注意）。
2. **R2**: dry bit9 の動的決着（this 経由 setter／ロード一括復元窓 0x465860 列）＝**#82 動的経路の申し送り**。
3. **R3**: メニュー名テーブル [0xB13974] の実行時構築経路（0x65C620=linkedList 追加 helper・登録窓 0x9FBA90/0xA24950 確認済。名文字列 VA 群 0xA69xxx と件数 0xB1397A の書手）。
4. **R4**: 0x6FF9C0/0x6FFAC0 のキー辞書内部構造（4 文字名→TD の解决表）。TD は確定済。
5. **R5**: 0x5D8FD0 の +0x24 サブオブジェクト vtable[0]・[0xB37FE0]・[0xB3B734] 名（UI レコード型名の一般化）。
6. **R6**: x87 除算例外ハンドラ群（0x9840E0/0x999594 系・SE 型 22・sentinel [esi+0x18]=0x200）の一般棄却リスト化（今後の bit9 走査の教訓候補）。

## 6. 再現手順

```
python %TEMP%\opencode\v326_1_bit9.py   # 全 .text [reg+0x18] census（命令 2,257,850・~5 分）
python %TEMP%\opencode\v326_all.py      # 6 項横断 bundle（~5 分）
python %TEMP%\opencode\v326_2.py        # getter/strings/0x589B70 発見（速）
python %TEMP%\opencode\v326_3.py        # 名テーブル/RTTI TD/0x5D8FD0 tail/0x57CFxx
python %TEMP%\opencode\v326_4.py        # bit9 候補文脈（v326_1 サイト抽出から）
python %TEMP%\opencode\v326_6.py        # 初期化系列/粒度判定バグ修正版
python %TEMP%\opencode\v326_7.py        # bit9 定数演算 粒度別全列挙（368）・A2 タグ 234（~5 分）
python %TEMP%\opencode\v326_8.py        # 0x5439CE/E7・0x5AE242/0x5D2F92・0x65C620（~5 分）
# v326_9（§3-1 bit 再走査）は本文に手順コードを転記。
# 全出力 .out は同名。EXE 読取のみ・改変なし。
```

- **Numba 使用報告**: `numba 0.68.1 / numpy 2.4.6` インストール済み・`@njit` 動作確認済みだが、本束の負荷は capstone 逐次 disasm（C 拡張）主体で、教訓（JIT 回収不能・capstone/I/O 不向き）どおり **`@njit` は未使用**。軽量な数値走査（imm 検索）も `bytes.find` で十分。
- **帯遵守宣言**: 深解析=本 lane 自帯（v316 帯: 0x459000-0x460000 / 0x464000-0x467920 / 0x430000 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800）。それ以外（0x5439CE・0x5AE242/0x5D2F92・0x57CFxx/0x589B70・0x9F10xx/0xA215xx・0x6FFxxx/RTTI・0x5D8F00・0x9840E0・0x65C620）は**照会のみ**（命令窓・データ窓の読み取り、関数境界の深追跡なし）。**v318 自帯（0x44F000 ほか・#94 専有）と 0x4C0000-0x4E0000（#106 専有）は命令 dump せず浅層参照のみ。逸脱なし**。
- クリーンルーム: 外部仕様書未参照。menu 名簿交差は本プロジェクト docs/RE_EXE_REPORT.md（§3.1・L5655/L5678）のみ。
