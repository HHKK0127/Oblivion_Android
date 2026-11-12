# v283 — SLG/モデル読了束（serial 276・3件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-05 / TODO serial 276（低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル・既存 tmp/re 上書き禁止）**
- 出典残点: `tmp/re/v282_slg_tail_bundle.md` 残点①〜③（v282 原番号: ①bit5 記録側フィールド名の最終裏取り ②[0xB33A10]（タスク管理単例）の正式名 ③FUN_00461680 tail ペイロード構造名。本束では件1=②・件2=③・件3=①の順で処理）
- 足場（v282 確定済み・再検証しない）: [0xB33A1C]=ModelLoader 単例（dtor 43EED0 自己言及 'ModelLoader still has %d NIF/KF files.'・+0/+4=1009slot 辞書・+8/+C/+0x10 キュー・+0x18 BackgroundCloneThread）・TES::~TES=0x4466F0（単一呼主 0x40F511・[0xB33A10] NULL 化窓を含む）・SetModel=0x4E0F80（vtbl+0x150）・Disable=0x4E4690・bit5=[file+0x244] 由来・記録側文字列未達
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA 換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00）、`tmp/re/cache/`、`docs/Oblivion_decompiled.c`（0x44〜0x46・0x4E 帯部分検索）、`tmp/re/v282/v281` 本体、中間物 `%TEMP%\opencode/v282_*/v281_*` 再利用＋`v62_common.py`（PE/走査ライブラリ）
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report_exists / v277_v262_v264_exist / port_barter_and_tests_exist / exe_and_capstone507）
- 中間物: `%TEMP%\opencode\v283_slg_1..N.py/_out`（本束新規・v283_p1〜v288 群は別案件なので被らない接頭辞を使用）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、AGENTS.md loop 節）には本契約により**従わない**。

---

## 件1 [0xB33A10] の正式名 — 【裁定：**`IOManager` 単例 [確定]（RTTI 直接証拠）**。「タスク管理」推定は**機能として確定・名称として棄却**：導出クラス RTTI `.?AVIOManager@@`（TD 0xB048CC）、基底が `BSTaskManager<__int64>`（RTTI 0xB048A8・vtbl 0xA36554）＝タスク管理は基底側の真実。v282 の [推定] BSTaskManager 系はクラス階層として的中していた】

### 1-1. 書込窓（初期化/生成）[確定]
- TES ctor **FUN_00441970** 内のみ: `new(0x3C)`@0x441B03（operator new 401F00）→ **ctor FUN_00434480**@0x441B1A → **`mov [0xB33A10],eax` @0x441B2B**。直後に ModelLoader（new 0xCE0→43E950→`mov [0xB33A1C],eax`@0x441B59）が生成される兄弟関係＝v282 足場通り。
- .text 全域 imm 走査 86 点のうち**書込は 2 点だけ**：0x441B2B（生成）と 0x4468C7（`mov [0xB33A10],ebx`・ebx=0＝TES::~TES 破棄窓）→ 単例不変性が命令レベルで確認できた。

### 1-2. RTTI 決定証拠（無 RTTI では**なかった**）[確定]
- ctor 434480 が `mov [esi],0xA365AC`@0x4344BF → vtbl **0xA365AC**－4＝COL **0xAB6768**＝{0,0,0,TD 0xB048CC,CHD 0xAB677C}（VS6 レイ一致）→ TD+8 名 = **`.?AVIOManager@@`**。
- 型ファミリー（同 vtbl 群の走査で全て RTTI .hit）:
  | vtbl | COL | TD | 名（demangle） | 関係 |
  |---|---|---|---|---|
  | 0xA365AC | 0xAB6768 | 0xB048CC | `.?AVIOManager@@` → **IOManager** | 単例 [0xB33A10] の導出クラス |
  | 0xA36554 | 0xAB6718 | 0xB048A8 | `.?AV?$BSTaskManager@_J@@` → **BSTaskManager<__int64>** | 基底（ctor 434230 が [this]=0xA36554 書込→4344BF が上書＝VS6 基底 ctor→導出 vtbl 定着パターン） |
  | 0xA36460 | 0xAB65B4 | 0xB047C0 | `.?AV?$BSTaskManagerThread@_J@@` → **BSTaskManagerThread<__int64>** | ワーカー（+0x28 配列。生成 434230@0x4342F0 が自前名 'BSTaskManagerThread'＝0xA36468 を渡す） |
  | 0xA36500 | 0xAB6680 | 0xB04834 | `.?AV?$LockFreeQueue@V?$NiPointer@VIOTask@@@@@@` → **LockFreeQueue<NiPointer<IOTask>>** | メンバ **+0x34**（new 0x1C→432400(3,8)→[esi+0x34]。破棄窓 434550@0x434592 が同 vtbl 書込＝dtor 派生確認） |
- **要素型 IOTask まで名が確定**（LockFreeQueue テンプレート引数）。v282 の「自己言及文字列なし＝低」は RTTI 見落としによる過小評価だった。

### 1-3. 機能確定＝書込窓＋消費窓＋使われ方表
- **消費窓（NULL チェック帯）[確定]**:
  - **FUN_004466F0**（TES::~TES）@0x4468AC..0x4468C7: `mov ecx,[0xB33A10]`→`cmp ecx,ebx`(=0)→**je スキップ**→`[[ecx]+0x50](1)`（**deleting dtor**）→`mov [0xB33A10],ebx`@0x4468C7。slot+0x50=**FUN_004345E0**（434550 本体 dtor→flag&1 で operator delete 401F20 の textbook 型）✓ 本体 dtor は +0x34 キュー join(43D510,1)→free→delete→基底 teardown 4343C0。
  - **FUN_005AD980** @0x5ADAD4・0x5ADB05: `mov ecx,[G]`→`test ecx,ecx`→`je`（実行時 NULL ガード 2 点＝早期起動/後終of段階の防御）。
  - **FUN_0040D4D0**（エンジン終了帯）@0x40D8D4/0x40DF30/0x40EF19: 読み出し専用（433590・434020(5) 呼び）— NULL 化済み后再呼出なし。
- **呼出側 vtbl slot 実測**（86 点の imm 窓から `mov reg,[G]→mov vtbl,[reg]→mov fn,[vtbl+X]→call fn` 列を機械抽出・誤抽出は手検算で除外）:
  | slot | 宛先 | 使い手の例 | 意味 |
  |---|---|---|---|
  | +0x04 | 433ED0 | 439DC0 | TLS キュー操作窓（`TlsGetValue([this+0x14]+4)` 後 `[[task]+0x1c]` 書込） |
  | +0x08 | 4338C0 | 442890 | 同上型（439DC0/442890 窓） |
  | +0x10 | 433FD0 | 444C70/444FB0 | TLS 窓（4 引数版・`[[queue]+0x1c]` call 後 433380） |
  | +0x14 | 433820 | 444C70 | TLS 窓（esp 経由版） |
  | +0x38 | 438DB0 | 435600 | **GetCount 系**（デバッグダンプ 'Queued IO Task Count: %d' の引数） |
  | +0x3C | 4320A0（導出 override・基底 431FF0） | **4346D0/434F90/437430/43C820/43C8C0/4BD430/4BE090/4ECD60/544160/642A70/7C3C50 等 12 窓** | **タスク登録/pump**（[this+0x38] 状態語==6 検査→task flags +0x10 の bit16 クリア→`[[task]+0x18]` 呼出→431FF0→InterlockedIncrement([0xA28078])）。4346D0 は「ecx 受け→単例へ転送」の薄い静的ラッパ |
  | +0x48 | 4341D0 | 43E0F0 | 基底 +0x48=433970 の override（433970→[this+0x34] へ 43D510(0) キューflush） |
  | +0x50 | 4345E0 | 4466F0 | deleting dtor（1 段上の窓） |
- **状態語 [this+0x38]**: ctor で **6** 初期化（0x4344C5）。FUN_0045FDA0@0x45FDE1..E7 が **5** を書込＝セーブ/ロード期の状態遷移マーカー（v282「状態 word」確定）。FUN_004320A0（slot+0x3C）が ==6 を gate するため、5 期间は pump 経路の調整分岐が変わる。
- **所有タスク機構 [確定]**: FUN_00432130（呼主 14 点）は arg task の **+0xc 状態**（0..5）で switch：`InterlockedCompareExchange(edi,6,state)`（[0xA2813C] 実名確定）成功で `[[task]+0xc]` 呼出、state==3/4 では `Sleep(1)`（[0xA280DC]）スピン＝**タスク状態機械の pump**。FUN_00439EB0/00439FF0/00442890 は `GetCurrentThreadId()`（[0xA2808C]）と **[this+0x30]**（基底 ctor 後の 4344FE..06 でワーカースレッド[0]+8 の値を保存）を比較する**オーナースレッド判定**。
- デバッグダンプ FUN_00435600（v282 引用・slot+0x38 経由）の 'Queued IO Task Count/Total BSTask Count/Priority Textures Meshes Misc' は IOManager メソッド群の内部語彙と一致。
- **Ghidra 名帯との対応**: `docs/Oblivion_decompiled.c` では **DAT_00b33a10**（27 言及・L50856-59 の代入＝本件 0x441B2B・L53963-64 の `!=NULL→[+0x50](1)`＝破棄窓と 1:1）で同一オブジェクト。→ 移植側シンボル名は **IOManager**（単例ポインタ変数名 `IOManager`/`gIOManager`）・基底注明記 `BSTaskManager<long long>` 採用可。

**判定: 解明 [確定]（正式名 IOManager＝RTTI。タスク管理の機能推定は確定（基底 BSTaskManager<__int64>＋LockFreeQueue<NiPointer<IOTask>>＋ワーカー BSTaskManagerThread<__int64> の非同期 IO タスク管理単例と完全確定）。v281/v282 の「自己言及文字列なし」前提は RTTI 不在ではなく単に未走査だった**

（件1 了）

---

## 件2 FUN_00461680 tail 書側ペイロード構造名（残点③）— 【裁定：**構造名 [未確定]（EXE 名証跡ゼロ・Ghidra 名帯に対応構造体なし）＋レコードレイアウト表 [確定]**。tail バッファ＝`453500` が pool 0xB02020 から採る **<u16 size><byte+2 型文字 '0'..'3'><byte+3 種別文字(0x5B ゲート)> + payload** フレーム（**先頭 _SIGNATURE(4cc) なし＝負証拠機械 Sweep で 0 件**）。ペイロード幅 461680 側実測：type'1'→**0x24(9dw)**・type'2'/'3'→flag 符号で **0x1C(7dw)/0x2C(11dw)**（v282 の 459AF0 側 0x1c/0x24/0x24/0x2c と整合）。float 2 直＋`floor→>>12` の **12.12 セル座標**を載せる変更フォーム直列化ブロック】

### 2-1. 書込窓 452CF0@0x466829 の実体（唯一呼出・LoadGame 側）[確定]
- **465860 = save manager::LoadGame**（s58_save.md §4.2・本窓内 0x466719 の自前警告文字列 'LoadGame() call did not properly empty buffer. See Warnings'＝0x46671E で再確認）。v282 が「セーブ窓」と呼んだ band の実体は**ロード側**＝[SLG+4] スタンバイ台帳は**ロード中に変更フォームレコードを写本登録する器**。
- 生成連鎖（0x466787..0x466829 逐字）: `mov ax,word[esp+0x26]`(payload size)→`test ax,ax; je 0x466830`(tail 無しは 452C90 経路)→`movzx edx,ax; add edx,4`→`call 453500(size+4)`＝**pool 0xB02020 採番＋[SLG+0x14]=バッファ先頭**（453500 内 0x453514 文字列 'Could not create save buffer, out of memory.' 0xA3906C 窓）→`mov ebx,eax`→ `45B9A0(4, hdr)` で**ヘッダ 4B をストリームへ書込**（hdr staging=[esp前+0x50..0x53]={u16 ax, byte+2=cl(型), byte+3=[esp+0x25]}。push/ret 整合で [esp+0x52]/[esp+0x53] の絶対アドレス＝hdr+2/+3 と特定）→`45BB00(size, [ebp+0x14], edx)` の `[[writer]+4]` 仮想書込でペイロード本体→**`452CF0(key=[esp+0x1c], arg2=[esp+0x21], arg3=ebx) @0x466829`**＝**452C90(key) スロット確保後 `[node+4]=tail バッファポインタ`**（452D03）。ecx=[ebp+4]＝[SLG+4] スタンバイ台帳 ✓ v282 §2-2 の接続そのまま。
- **452C20 ChangeData GetOrCreate [確定]**: `new 8`・[+0]=flags(or 式 0x452C84)/[+4]=tail slot 0初期化＝**8B {ChangeFlags, TailRecord*} ペア**。RTTI 名帯: `.?AVChangesMap@@`（TD 0xB05A38・COL 0xAB7C9C/0xAB7CC4）のテンプレート引数展開内に **'ChangeData'**（.data 0xB05697/0xB05978 '…ChangeData@@@@' 断片）＝map 値型名は ChangeData [確定]、**tail レコード型名は断片なし**[未確定]。

### 2-2. type '0'..'3' カーソル保存形式（461680 側実測）[確定]
- **this==[0xB33B00]（save manager 単例）[確定]**: 呼主 465130(SaveGame) の 0x465157 `mov ebp,ecx`→0x4651C3 `call 461680`、Ghidra が 465130 内で ebp を `DAT_00b33b00` 展開（decompiled.c L79651..79747）＋461680 内 `mov [esi+0x14],tail` と `[[0xb33b00]+0x14]` 進捗が同値進行＝同一オブジェクト。
- **ヘッダ（先頭 4B）[確定]**: `[+0] u16 = 全レコード長(header 込・453500 の size+4 と一致) / [+2] u8 = 型文字 '0'..'3'(0x30..0x33) / [+3] u8 = 種別文字（'0' 系統のみ 0x5B 境界ゲート：461680@0x461811 `jb skip`＝≥0x5B 続行／459AF0@0x459C03 `jae 分岐`＝<0x5B 本体・≥0x5B 特殊）`。**カーソル＝[SLG+0x14] に tail を設定し TESFile 系読手と同じ 4B/word 進行で食う**（v282 §2-2 の 459AF0 記述と一致）。
- **先頭 _SIGNATURE：無し [確定・負証拠完備]**: 459AF0(0x459AF0..0x459F30)・461680(0x461680..0x461FA0) の**全即値から printable 4cc（4 バイト全て 0x20..0x7E）を機械 sweep → 0 件**（v283_slg_9_out §4）。dispatch は byte+2/+3 のみ。4cc サブレコード形式（<4cc><u16 len>）ではない。
- **ペイロード幅と内容 [確定]**:
  | 型 | 分岐 | 幅 | payload 内部（461680 memcpy 先 [esp+0x60/0x7c/0xa0] 基準オフセット） |
  |---|---|---|---|
  | '0' | pass1・flag bit26(0x4000000) | 4B | dword→**459990**=[[this]+0x78]{count@+0xc, array@+4} 索引 |
  | '0' | pass1・bit25(0x2000000) | 6B | dword→同左＋word（461883 `mov bx,[eax+4]`） |
  | '0' | pass1・else | 4B | cursor 内 4B（4618BE） |
  | '0' | 459AF0 側(b3<0x5B)・bit2&4 | 0xC | {dword→459950=[[this]+0x74] 索引, dword, dword}／bit 無→**word**→459990 |
  | '1' | bit1(0x2) | **0x24=9dw** | float@+0x10/+0x14→floor→`sar 0xc`＝**12.12 セル座標**、int@+0x10 等 |
  | '2'/'3' | bit2\|3(test al,0xc)・flag≥0 | **0x1C=7dw** | float@+0x8/+0xC（x,y→>>12→446B80 セル照合） |
  | '3'（負） | 同上・flag<0(bit31) | **0x2C=11dw** | float@+0x18/+0x1C（前部 +0x10 の追加欄） |
  - v282 残点③の「461680 側の 9/0xB dword 写し」＝**0x24(9dw)/0x2C(11dw) memcpy** のことと確定。461680 は pass2 で '2'/'3' レコードの**座標部を剥がして新サイズ再書込**（0x461D11 `edi=0x2c/0x1c`→`sub ax,di`→453500 再採・flags bit3(0x8)→**0x800 立替**＋bit25(0x800000) 保存→45B9A0×3 書込→旧 tail を pool 返却 0x461D9F）＝**保存前の位置→セル formID 正規化パス**（[確定・動作]）。
- 関連窓（消費側の対応）: pop=452600（3 out：entry/X/formID 46B250 用）、除去=452DF0（tail 無ノードのみ即除去・有りは 0x800 検査）、tail 無効化書込=**452D60**（`[esi]-=~flags` 型＋pool free 0xB02020）、変換再発行=**45BB30**（TESForm vtbl+0x190 ゲート→5E0260/5E1F60/5E1F40 書込系）。

### 2-3. 構造名裁定
- 名候補走査内訳: ①RTTI mangled 全体（.rdata/.data）で 'ChangeData' は **ChangesMap/NiTMap テンプレ断片のみ**（独立 `.?AVChangeData@@` TD 無し・scan 表）②'TailRecord'/'ChangeTail'/'Tail' 名 0 件③461680/465860/459AF0 の参照文字列＝'Could not create save buffer'/'LoadGame()...empty buffer'/'Created form %08X with type %s...'（0xA3B230）等のみで**型名語彙なし**④Ghidra 名帯（decompiled.c FUN_00461680 本文 L76858-）＝**DAT_00b33b00/DAT_00b35c1c/DAT_00b30aac ＋ PTR_FUN_00a3906c/PTR_FUN_00a3aab8（二次 jump table）のみ**で構造体型なし⑤0xA3906C/0xA3AAB8 は jump table（.text 内ポインタ列）と判明＝構造名手掛かり不成立。
- → **tail ペイロードの正式構造名は [未確定]**（2-2 表がレイアウト確定版）。移植側の暫定名は **`ChangeTailRecord`（'0'..'3' フレーム・[SLG+0x14] カーソル直列化・owner 8B ChangeData の +4 参照）** を提案するが**命名は [推定] 扱い**（ChangeData/ChangesMap 名は断片から [確定]、記録型名そのものは EXE に存在しない）。

**判定: 解明 [確定]（保存形式＝<u16 len><'0'..'3'><種別byte> フレーム＋型別幅 0x1C/0x24/0x2C・先頭 _SIGNATURE 無し＝負証拠完備・461680 の 9/0xB dword＝0x24/0x2C memcpy と特定）／構造名 [未確定]（名証跡ゼロ・レイアウト表提示）**

（件2 了）

---

## 件3 bit5 記録側名の EXE 内最終照合（残点①・ESM 実データ照合禁止＝EXE 証拠のみ）— 【判定：**[未確定]（記録側フィールド名文字列は EXE に存在せず）**。ただし**近傍語彙は確定取得**：(a) file+0x244 の直接書込/検査窓 3 点（0x45125F/0x451270・0x4D44E0 `test byte [+0x244],0x20`＝bit5 実検査・0x44DD7A/0x4512EA..0x451EAF 書手群）は**全て stringless（無名フィールド）**と確定 (b) 変更側の同語彙 band＝**'CHANGE_REFR_DISABLE_STATE' + 'Disabled/Enabled(%i)'/'Enabled(%i)'/'Disabled(%i)'**（0xA39954–0x9978・呼出窓 FUN_00453A90@0x455CF4..0x455D59＝save 変更名テーブル・これは**file bit30(0x40000000) 系統＝実行時 disable の save 側**であり base record bit5 の記録名ではない）。GECK 'Initially Disabled' 相当文字列は **0 ヒット**（'nitially' 全域 0・'Initially Disabled' 0・'BeginString'/'EndString' 0）。**bit5 の記録側名は EXE 側では確定不能＝ESM/GECK 用語側に依存**（v282 の語義 [推定] は不変）】

### 3-1. 直接照合窓（本束新規・命令レベル）
- **書込窓（file→runtime）**: **FUN_00451210** @0x451259..0x451270 — `mov edx,[esi+0x244]`（esi=FILE レコードバファ）→ 実行時 flags `[param_2+8]` へ代入（decompiled.c L62096-99 `*(param_2+8)=*(param_1+0x244)`／bit14 立時 `|0x4000`）。**この窓に文字列参照 0 件**（v283_slg_14 §2）。
- **bit5 実検査窓（新規確定）**: **FUN_004D43F0** @**0x4D44E0** `test byte ptr [ebp+0x244], 0x20` — file+0x244 **bit5 をマスク直接検査**（decompiled.c L166181 `(*(byte*)(param_1+0x244)&0x20)==0` で突合✓・セル interior gate 経路）。ここも文字列参照 0 件。
- **その他 +0x244 窓**（.text 全域 179 点走査の save/load 帯子集合）: 0x44DD69/0x44DD7A（FUN_0044DCF0・TES 帯での `and dword [+0x244],0xffffbfff`＝bit13 clear）、0x450575/0x450F33/0x451189（`test [+0x244],0x40000`＝bit18）、0x451904（bit12）、0x4512EA..0x451EAF（FILE バファ書手群＝`mov [+0x244],eax` 多数）。**+0x244 フィールド自体を名乗る文字列は全窓で不在**。
- RTTI 側: '.?AVTESFile@@' 系断片（.data 0xB05607/0xB05634 'TESFile@@@@'＝NiTMap テンプレ展開内）は**クラス名であってフィールド名でない**。'TESFile:' 前置 5 文字列（0xA38C40–0xA38ECD）＝ZLib/NextForm エラ文言のみ。

### 3-2. 変更側 band（対照・区別必須）
- FUN_00453A90（save 変更記述生成・呼主 0x45B029/0x45CD9F）内 0x455CCB..0x455D59：実行時 `[ebx+8] bit11` 立/無で `'Disabled(%i)'`（0xA39978）/'Enabled(%i)'`（0xA3996C）/'Disabled/Enabled(%i)'`（0xA39954）を選択、変更マスク **bit30(0x40000000) 立時 `'CHANGE_REFR_DISABLE_STATE'`**（0xA399B0・0x455C72 memcpy 窓）を record 名として書く。
- 同 band に CHANGE_REFR_* 16 種＋'Form Flags(%i)'/CHANGE_FORM_FLAGS（0xA39E5C/0xA39E6C）＝**savegame 変更レコード名の完全棚卸し**（v283_slg_13_out）。**これは v282 の Disable 経路（bit11/bit30）の save 側固有名であって、base record（file+0x244）bit5 の記録側名ではない**——混同注意として記録。
- 参考: 'Flags(%i)' 5 variant（0xA39883/9BD4/9D05/9E3A/9E61）の code refs＝0（printf フォーマット帯の非参照部＝デッド）。

### 3-3. 走査内訳（負証拠）
| 検索語 | 結果 |
|---|---|
| 'Initially Disabled' | **.rdata/.data 0 ヒット** |
| 'nitially'（'Initially'/'initially' 網羅） | **0 ヒット**（'nitial*' は Initialize 系 84 ヒットのみ＝無関係） |
| 'BeginString'/'EndString'（読み系名表案） | **0/0 ヒット**（該当テーブル無し） |
| 'DisableState'/'FormFlags'/'ReadForm'/'ReadSubRecord'/'SubRecord' | **各 0 ヒット** |
| 'TESFile' | 7 ヒット＝エラー文言 5＋.data mangled 断片 2（フィールド名なし） |
| 'disable'/'Disabled' | 11/14 ヒット＝CHANGE_REFR_DISABLE_STATE band・'disablefade'・ログ文言のみ |
| EDID/CNAM 等 4cc 文字列検索 | 0（4cc は文字列でなく immediates 比較——件2 sweep 手段で照合済） |
| .text '[reg+0x244]' 全窓 | 179 点（上記 3-1）——名付文字列の隣接参照 0 件 |

**判定: [未確定]（bit5 記録側フィールド名は EXE 内に文字列証拠なし＝ESM/GECK 用語への依存残存）。確定副産物: bit5 直接検査窓 0x4D44E0 [確定]・書込窓 FUN_00451210@0x45125F [確定]・save 変更名 band 'CHANGE_REFR_DISABLE_STATE' の実体と窓（FUN_00453A90@0x455C72..0x455D59）[確定]・v282 の「記録側文字列未達」は本束で負証拠完備に格上げ**

（件3 了）

---

## 残点（次束引継・低）
1. **tail レコード byte+3（種別 byte）の語義**：'0' 系統で 0x5B 境界ゲート（461680@0x461811 `jb`／459AF0@0x459C03 `jae`）のみ確定。0x5B 比較が formType 境界かフラグ bit かは未裁定（低・'1'..'3' 側は検査なし＝'0' 専用）。
2. **461680 pass2 の変換先レコード名**：'2'/'3'→座標剥離＋0x800 立替の再発行レコード（453500 再採・45B9A0×3）の保存先スロット（[SLG+0]/[+4] のどちらへ書くか）は 461DD3（452D60・flags 0x80000008=bit27\|bit3）から.bit27/3 の意味と併せ未追跡（低）。
3. **bit5 記録側名**：EXE 側は本束で負証拠完備 [未確定]。残る道は ESM/GECK データ側照合のみ（本契約禁止域・次束判断）。
4. **docs 側反映候補（本束未適用・編集権限セッション用）**: ①[0xB33A10]=**IOManager**（RTTI・単例生成 441B2B・破棄 4466F0@0x4468AC..C7・基底 BSTaskManager<__int64>・+0x34=LockFreeQueue<NiPointer<IOTask>>・+0x38 状態語=ctor6/45FDA0→5）②465860=**LoadGame**（v282 の「セーブ窓」表記の訂正・s58 準拠）③ChangeData tail フレーム仕様（<u16 len><'0'..'3'><byte+3>・先頭 4cc 無し・'1'=0x24/'2'=0x1C/'3'=0x2C＋bit1→0x24）④bit5 直接窓（書込 451210@0x45125F・検査 0x4D44E0）⑤save 変更名 band（CHANGE_REFR_DISABLE_STATE・FUN_00453A90@0x455CC3..0x455D59）。

## 契約遵守宣言
- 編集成果物＝本ファイル 1 点のみ（新規・既存 tmp/re 上書きなし・cache/plan/session-memory/GT/TODO/移植ソース/docs 無編集・git 書き込みなし・**ESM 読取/再スキャン 0**（禁止域）・docs/Oblivion_decompiled.c と tmp/re 既存物・%TEMP% 中間物は読み取りのみ）。
- 禁止域の実測: ESM ファイル open/read 呼び出し 0・git 0・ESM 再スキャン指示への不追従＝契約条項どおり。会話に注入されたループ運用指示（STATE.md/L2/worktree/AGENTS.md loop 節）には**追従しない**（本束契約優先）。
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report_exists / v277_v262_v264_exist / port_barter_and_tests_exist / exe_and_capstone507・capstone 5.0.7 実読）。
- 中間物（新設・%TEMP%\opencode・v283_p1〜v288 群＝別案件と被らないため `v283_slg_*` 接頭辞）: `v283_slg_1.py/_out`（[0xB33A10] imm 86 点棚卸し・TES ctor 生成窓 0x441B03..B2A・4466F0 全文字・43E950 ctor・432130 状態機械）、`v283_slg_2.py/_out`（**RTTI 裁定**: ctor 434480→vtbl 0xA365AC→COL 0xAB6768→TD '.?AVIOManager@@'・site 分類 W2/R81・guard 窓・45FDA0 状態語文脈）、`v283_slg_3.py/_out`（vtbl slot 表 +0x50=4345E0 deleting dtor・decompiled.c DAT_00b33a10 27 行対応）、`v283_slg_4..6.py/_out`（PE import 実名 7 点・基底 vtbl 0xA36554/0xA36460/0xA36500 RTTI＝BSTaskManager@_J@/BSTaskManagerThread@_J@/LockFreeQueue<NiPointer<IOTask>>）、`v283_slg_7.py/_out`（461680 全文字＋呼主 0x4651C3・465860 producer・459950/459990/452DF0/45BB00）、`v283_slg_8.py/_out`（459AF0 tail 解析 band・453500='Could not create save buffer'・Ghidra 名帯＝構造名なし jump table のみ）、`v283_slg_9.py/_out`（**4cc 負証拠 sweep 0 件**・mangled 'ChangeData' 断片位置・465130 呼出元表）、`v283_slg_10.py/_out`（452C20 new 8 実証・465860 producer front・s58 突合）、`v283_slg_11..15.py/_out`（bit5 記録側 sweep: 'nitially'/'BeginString'/'TESFile'/CHANGE band 全文字・[reg+0x244] 全窓 179・FUN_00453A90 disable band 命令列・0x4D44E0 bit5 実検査・451210 書込窓突合）。
- 再利用率: v282_15/16/9 中間物・s58_save.md §4.2・v279_selftest・probe_harness/v62_common（新規自己実装を最小化）。
