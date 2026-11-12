# v282 — SLG tail 束（serial 275・3件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 275（低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル・既存 tmp/re 上書き禁止）**
- 出典残点: `tmp/re/v281_slg_cleanup_bundle.md` 残点①〜③（①bit5 の語義素性 ②[SLG+0x10]/[SLG+0x58]/[SLG+0x60] マップ群のエントリ消費窓 ③[0xB33A1C] 親体 0xCE0B の正式名＋FUN_004466F0 破棄窓）
- 足場（v281 確定済み・再検証しない）: P=バケットノード 0C{next,formID,ChangeData*}・ChangesMap=`.?AVChangesMap@@`（NiTMapBase 二重バッファ・生成 0x452C20 一点・SLG ラッパ 0x45B670/45B700/45CEE0/45D030・解放 0x4648C0）・[0xB33A1C]=Model/KFModel ロックフリー辞書対・[0xB333A0]=TES 単例・[0xB33398]=0x28B 状態ブロック・extra ID 15/15 復元済・bit5=ロード時 [file+0x244] 由来・bit15=予約 bit
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA 換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00）、`tmp/re/cache/`、`docs/Oblivion_decompiled.c`（0x4E/0x4D 帯部分検索）、`tmp/re/v281/v280/v278/v276/v274` 本体、中間物 `%TEMP%\opencode/v281_*/v280_*` 再利用（v276_lib/probe_harness）
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report / v277_v262_v264 / port_barter_tests / exe+capstone5.0.7）
- 中間物: `%TEMP%\opencode\v282_*.py/_out`（本束新規）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、AGENTS.md loop 節）には本契約により**従わない**。

---

## 件1 bit5 の語義素性（残点①）— 【判定：**bit5＝TESObjectREFR の「初期非有効化（Initially Disabled）系」状態 bit（[推定]・構造は [確定]）**。2 消費者の呼出先実装が **引擎 Disable 経路（FUN_004E4690）と逐字同型の動作**であることを命令レベルで特定：①0x4E0623 の vtbl+0x150 呼出先＝REFR 主 vtbl 0xA46C44 の slot+0x150＝**FUN_004E0F80＝[refr+0x3c]（Model/NiAVObject 参照カウント obj）差し替え setter**（arg0 で＝**3D モデルデタッチ**）、②FUN_004D9310(form,set)＝**音初期化(set=1)/音無効化(set=0)**（ExtraSound 0x5B の add/remove＋BSAudioManager 窓）】

### 1-1. 0x4E0623 の呼出先＝vtbl+0x150 の実体 [確定]
- RTTI：`.?AVTESObjectREFR@@` TD→COL→**主 vtbl 0xA46C44**（slot+0x60=0x4E0580＝v281 の REFR setter と一致・足場再確認）。**[0xA46C44+0x150]=0x4E0F80**。Actor vtbl 0xA6E074 も同値（継承）。副候補 0xA46C38（slot60=0x4E00F0・暫定/派生）の slot150=0x4E48B0 は「Enable 時 model 再アタッチ」系（0x4E48B0：vtbl+0x154 の結果 88CD50 登録＋[esi+0x40] CELL 検査→4C97F0/424180 経路＝再アタッチ側・v282_2_out）。
- FUN_004E0F80(this=refr, obj*) 0x4E0F80..0x4E157F の実装意味（中間物 v282_1_out §B・逐字要点）:
  - 0x4E0FAC `cmp [ebp+0x3c], arg` 一致かつ arg==0 → 0x4E0FC3 `439DC0([0xB33A1C], refr)` ＝**Model 辞書から除去して終端**。
  - 0x4E1077 `call [0xA28078]`(InterlockedIncrement &old+4)/0x4E10D2・0x4E1439 Decrement＋0x4E10E4 `[[old]+0](1)`（deleting dtor）＝**obj+4 が参照カウントの NiAVObject 系**。transform 保存：old の +0x30..（9dword 行列）/+0x54（位置）/+0x60（scale=fabs(4D7260 距離)）を新 obj へ 0x4E140B..0x4E1426 で移し替え。
  - 0x4E1384 `mov [ebp+0x3c],0` → 0x4E13D9 `call 4D7D10(arg)` ＝**[+0x3c] 代入本体 setter**（0x4D7D56 `mov [edi+0x3c],ebx`＋InterlockedInc/Dec＋0x4D7D17 vtbl+0x144 gate・v282_2_out）。
  - **bit5 の直接消費者が関数内に存在**: 0x4E1163 `mov ecx,[ebp+8]; shr ecx,5; test; jne 0x4e11a7` → bit5 立時に 45D220/45D390（SLG 台帳登録＝件2 の [SLG+0x58]/[+0x60] 側）を**スキップ**。0x4E109C の bit14 検査（`shr edx,0xe`）も併用窓。
  - 0x4E1395 arg==0 経路の末 0x4E13CF `4D9310(refr,0)` ＝**音無効化**（下位 1-2）。
  - → vtbl+0x150(0) の意味＝**「model デタッチ＋SLG 登録抑制＋音剥離」＝ disable 同等の初期化取り消し**。

### 1-2. FUN_004D9310(form,set)＝音 init/disable 窓 [確定]（v282_1_out §C・v282_2_out・docs L170797 突合）
- set!=0（立上げ）: 0x4D9342 `bit11 立→全スキップ退出`。type==0x12(ACTR): extra 先頭 6B73A0 で音 active 判定→非 active 時 `41E160([refr+0x44],0x5B)`＝**RemoveExtra(ExtraSound)**＋base+0x54 キーで BSAudioManager(`[[0xB33398]+0x24]`)6AE4B0 検索→423B10 add。type 0x18(FLOR) は **bit13 立時にスキップ** gate で同型（base+0x60）、type 0x1a(WEAP) も同型。
- set==0（0x4D960A・無条件経路）: type==0x0a(GLOB)→6ACA40/6AB890/6AC9F0(formID) で**グローバル音除去**；type∈{0x12,0x18,0x1a}→6B7240/6B73C0 停止＋`41E160(0x5B)`＝**RemoveExtra(ExtraSound)**（docs 逐字 `FUN_0041e160(0x5b)` ✓・0x5B=ExtraSound は v281 件2 表）。
- 呼主引数（v282_3_out）: **0**=0x4B7F46/0x4B7F5F/0x4E0829/0x4E13CF/0x4E4899、**1**=0x4DC1F3/0x4DD985（0x4CB64A は push ebx＝可変）。

### 1-3. bit5↔bit11 の対比と Disable/Enable 窓（語義素性の裁定根拠）[確定＋推定]
- **FUN_004E4690＝Disable 経路本体**: 頭 0x4E4693 が **[form+8] bit11 立なら即終端**（冪等）→ vtbl+0x90(1)→**vtbl+0x40(0x40000000)**（file 側 flags に bit30）→**46ABA0(form,1)**（bit11 set）→ [edi+0x58] obj 解放→**vtbl+0x150(form,0)**（0x4E47C1/0x4E486F＝model デタッチ）・**4D9310(form,0)**（0x4E4899＝音剥離）。E8 呼主 5 点＝0x4DDADB/0x4E4812(自分)/0x4FA593/**0x5F2447・0x6333FE**（0x63 帯＝スクリプト/コンソールコマンド実装帯＝ `Disable` コマンド側）。
- **FUN_004DD850＝Enable 側**: 0x4DD89D `46ABA0(form,0)`（bit11 clear）＋0x4DD985 `4D9310(form,1)`（音 init）。両者相互呼出（0x4DDADB/0x4E480B・子 ref 群 0x4203A0 ループの分岐が「cell 内→Enable/それ以外→Disable」）。
- 対比表（v281 §3-2 拡張）: **bit11=実行時 disable ミラー（helper 46ABA0・Disable/Enable が set/clear）**／**bit5=同 semantics の file 由来版**（専用 helper ゼロ・[file+0x244] ロード書込のみ・読手は 0x4E0580 2 点＋0x4E0F80 内部 1 点）。**0x4E05C7/0x4E0802 の条件式が `bit11 || bit5`（0x4E0802 は FLOR 限定 bit13 を加う）＝エンジンが 2 bit を同一視処理**——「実行時 Disable された ref」と同一視される file 由来 bit＝**Initially Disabled（記録側 'disabled at load' 性）**以外に整合する解釈がない。bit5 立時 `SetModel(0)+4D9310(0)`＝**モデル/音を作らない無効化系初期化**（＝ロード時の ref 非有効化）。
- 命名の限界: [file+0x244] の記録側フィールド名（REFR 記録 flags bit5＝GECK 'Initially Disabled' 該当 bit）は静的には文字列確定不可 → **語義素性は [推定]**（構造・消費者動作・bit11 同一視・REFR 限定・runtime setter ゼロの 5 証拠で方向は確定）。CELL/Actor setter に bit5 検査なし（v281 側証）＝**form 汎用でなく ref 専用 bit**。

**判定: 解明 [推定]（呼出先 2 点の実装意味を命令レベルで確定＝①model デタッチ setter(0xA46C44+0x150=4E0F80)・②音 init/停止(4D9310)・bit5 は bit11(実行時 disable)と同一視される file 由来の「初期非有効化」系 bit）**

（件1 了）

---

## 件2 ChangeData tail/E70・F10・FE0 オーナーマップ群のエントリ消費窓（残点②）— 【判定：**消費窓一表化＋前提訂正 [確定]**。(a) E70/F10/FE0 の実オーナーは **[SLG+8]/[SLG+0xC]/[SLG+0x10]**（残点②記載の +0x10/+0x58/+0x60 の内 2 つは別系統＝**ロード側 subrecord バッファキュー** [SLG+0x54..0x60] だった）。(b) **+8/+0xC は「セル purge 窓」（FUN_00459AF0＝PurgeSavedDataForCell 実体・FUN_00464910）で消費されるが、[SLG+0x10] は全域スキャンで読取 site ゼロ＝書込専用（write-only）と裁定**。(c) +0x58/+0x60 の消費窓は各々 FUN_0045CEE0/FUN_0045D030（自己アサート文字列 `LoadAttachedAnimations()/LoadHavokData()` で関数窓名確定・読取 site 一表下記）】

### 2-0. マップ種別と生成 [確定]（SLG ctor FUN_0045B300・v282_5_out §C 逐字）
| slot | ctor | ノード | 用途系 |
|---|---|---|---|
| [SLG+0]/[+4] | 45A860 | ChangesMap（v281 足場） | 変更要求台帳 double-buffer |
| [SLG+8] | 45A940（vtbl 0xA3A310・0x25 slot） | E70=452E70 が 8B ノード連鎖 | unload キュー（**interior 系**） |
| [SLG+0xC] | 45AA80（0xA3A330・0x25 slot） | F10=452F10 が 0xC{data,x>>12,z>>12}（位置の 12.12 固定小数） | unload キュー（**exterior 系**） |
| [SLG+0x10] | 45AA80（同型） | FE0=452FE0 が 0xC{d1,d2,d3}（角度 3dword・4C9A80/4C9AA0/4C9CF0 系 getter 値） | unload キュー第3種＝**消費者ゼロ** |
| [SLG+0x54]/[+0x58]/[+0x5C]/[+0x60] | 45ABE0（0xA3A350）×4 | ロード側 subrecord バッファ（word len +2 pool 0xB02020・セーブストリーム [SLG+0x14] カーソル写本） | ANAM/AVNO/ACHR/ACDS 系 post-load apply キュー |
| [SLG+0x20]/[+0x30]/[+0x64] | NiTList（446CB0 AddHead・8B node） | 削除/保留リスト | 件2 対象外（参考） |

### 2-1. 読取 site・用途の一表（本件の本体）
| owner | 生成側（書込） | 消費窓＝読取 site（VA 逐字） | 用途 |
|---|---|---|---|
| **[SLG+8]** | 452E70←463A90(UnloadForm)@0x463C23・465860@0x466061/0x466277（key=親 CELL formID＝[refr+0x1c cell]+0xc・val=アタッチアニメ data ptr） | **FUN_00459AF0**: Get 0x459E3B→走査 0x459E80(`cmp [node+0],formID`)→適用 65C620@0x459E52／Get 0x459EC7→空 block 時 Remove 4524C0@0x459EE1＋delete 401F20。**FUN_00464910**（4CD960 pipeline・interior 分岐）: Get 0x464966→walk 0x464981→再適用 463D70@0x46498A→Remove 0x4649A6→4526E0 連鎖 free＋delete | セル purge 時に該当セルの unload 待ちアニメデータを開放/再適用 |
| **[SLG+0xC]** | 452F10←463A90@0x463C6D・465860@0x46609D/0x4662C2（float→`(s)int >>12`×2 の位置 ent 0xC） | **FUN_00459AF0**: Get 0x459E65(flag==0 側)→walk 0x459E80→65C620／Get 0x459EF1→Remove 0x459F0B＋delete。**FUN_00464910**（exterior 分岐 0x4649C8）: 4C9CF0(cell)+0xc key→Get 0x4649D7→walk 0x464A10（entry+4/+8 と位置一致時）→463D70 再適用→65C620/67F100→Remove 0x464A8E | 同上（位置版）。459AF0 の flag 分岐＝**!=0→+8・==0→+0xC**（0x459E2A/0x459EB4 の 2 系統のみ＝+0x10 分岐なし [確定]） |
| **[SLG+0x10]** | 452FE0←463A90@0x463CD7（Actor cast 0xB03038・request bit2 時）・465860@0x465E93（角度 changed 時） | **無し**（全域 E8 window scan：callers(452600/4524C0/55E000/452D30/452CF0/452570/452C90/452C20/452D60/452DF0)×`[reg+0x10]` 検査＝ヒットは [0xB33398]+0x10（worker TID・別 namespace）と vtbl+0x10 のみ。464440/461680/444340 内 `[reg+0x10]` も NiNode/TES メンバと判別済）。解放は SLG dtor 帯（462080/462B20→45EC50 窓）の map ごと破棄のみ | **write-only 死枝**（Steam 版 v1.2.0416）。[確定・負証拠完備] |
| **[SLG+0x58]** | 459020（=TESObjectREFR Load 帯 FUN_004E00F0@0x4E0360 が呼ぶ subrecord handler：`[esi+0x58]` map・len word 読・pool copy・Set@0x45906D key=[rec+0xc] formID val=buf） | **FUN_0045CEE0**（呼主 1 点=461030@0x461064）: pre-scan 0x45CEE6・pop 452600@0x45CF35→formID 46B250→dynamic_cast REFR（0xB02F9C→0xB03054）→`[refr+0x3c]==0` 時 4210E0(&[refr+0x44],val) ＋ MarkChanged bit24（452C20・台帳は [SLG+4]→無ければ [SLG+0]・0x45CF79..88）／`!=0` 時 [SLG+0x14] カーソル 2byte 読→4E2F70＋不一致 assert 0xA39F84。Remove 4524C0@0x45CFF4・pool free 0x45CFEB・末尾 map 空検査 0x45D006..0x45D021（'LoadAttachedAnimations() call finished, but still has elements in the map.'） | **attached-animation(AVNO 系) post-load apply**：3D 未生成 ref へ extra 付＋bit24 再要求／生成済みはストリームから 2B 読んで 4E2F70 |
| **[SLG+0x60]** | 459310（同帯@0x4E0394 の handler `[esi+0x60]`） | **FUN_0045D030**（呼主 461030@0x4610D2）: pre-scan 0x45D036・pop 452600@0x45D085→4211E0(&[refr+0x44],val)@0x45D0C8＋bit24 マーク@0x45D0DC／生成済みは 4E31E0（0x45D113・カーソル 2B 読）＋assert 0xA3A0A0。Remove 0x45D150・末尾検査 0x45D162（'LoadHavokData()'）。**副窓**: FUN_0045D220＝**TESSaveLoadGame::SaveQueuedHavokData**（自前文字列 0xA3A82C・呼主=SetModel 4E0F80 のみ・ChangesMap の ChangeData 要求 bit3/5F0310 検査→pool 新設→4211E0 add＋SetFlags bit24）／FUN_0045D390 同型副窓（4E0F80@0x4E11A2・4E0970/4E0A40 窓） | **Havok(ACDS 系) apply**＋ロード完了後の再保存窓 |
| （兄弟）[SLG+0x54] | 458E50（handler・呼主 0x4707A7） | **FUN_0045CE00**（461030@0x46105D）: pop 0x45CE48/Remove 0x45CEAD/assert 0xA3A750 'LoadAnimations()...' | animation キュー apply |
| （兄弟）[SLG+0x5C] | 4590E0（呼主 0x63F3CC 帯） | **FUN_00459140**（461030@0x46106B）: pop 0x459195・[[form]+0x18c] Get3D→657340・assert 0xA3A064/0xA39FC8/警告 0xA3A010 'Mob %s %08X does not have a character controller to load.' | char controller apply |

### 2-2. ドライバ／窓の位置づけ [確定]
- **FUN_00461030**＝post-load apply ドライバ（6 段直列：45CE00→45CEE0→459140→[arg1 分岐 677EC0/4424D0(TES)/6ECC80→4C97F0→424180・[0xB35C24]88BC20]→45D030→45D190(=[SLG+0x64] リスト walk：REFR cast→[[x+0x5c]+0x28]+0x3c→707370 scale0・60BE90・node delete)）。ゲート=[0xB33398]+0x10 ワーカー TID 一致時 [SLG+0x18] bit0/bit18（45B700 と同型）。呼主 4 点: **444340**@0x4445A2（ロード完了帯・45FDA0 も 0x444595 で隣接）・**465860**@0x4668FC（セーブ窓末尾）・**4CD960**@0x4CDA66（**4 段パイプライン 4637C0→464910→45FDA0→461030**＝セル終了/変更適用の本体・TESObjectCELL 帯）・663070@0x663470。
- **FUN_004637C0**（SaveFormChanges 系・ChangeData Get@0x46383E＋452DF0 除去@0x4638E0＋CheckFlags 453530 併用）／**FUN_0045FDA0**（[SLG+0] の value block GetOrCreate@0x45FF26..72・or bit4・46ABA0(bit11) 0x46006C・モデル辞書 438060@0x460136・Load 完了系 444340/465860/4CD960 から呼出）。
- ChangeData tail(+4) 直接窓（残点②の「接続」の実体）: 書込=**452CF0**（0x452D03 `[eax+4]=arg3`・唯一の外側呼出点 **465860@0x466829**＝[SLG+4] スタンバイ台帳側）／読取=**459AF0 主ループ**（452600 pop 0x459BBA→0x459BC5 `[out+4]`＝tail を [SLG+0x14] カーソルに設定→type byte **'0'..'3'(0x30..0x33)**＋要求 bit{2,4,0x2000000,0x4000000} で 459950/459990 適用→452DF0 除去・カーソル復元）＋461580 Purge（ChangeData 空 block の tail free 0x461625）。

**判定: 解明 [確定]（+0x58/+0x60 消費窓の命令レベル特定＋+8/+0xC 消費窓（459AF0/464910）確定＋[SLG+0x10] write-only 裁定（負証拠完備）＋残点②の slot 前提の訂正）**

（件2 了）

---

## 件3 [0xB33A1C] 親体 0xCE0B の正式名＋FUN_004466F0 破棄窓（残点③）— 【裁定：**親体＝`ModelLoader` 単例 [推定]（文字列直接証拠あり・RTTI 不在の POD）**。破棄窓 FUN_004466F0 ＝ **`TES::~TES` 本体 [推定]**（呼出 1 点・グローバル teardown FUN_0040D4D0@0x40F511・先頭で自 vtbl 0xA37470 書込）で、ModelLoader は 0x446892..0x4468B4（43EED0 dtor→delete→slot NULL）→ 兄弟 [0xB33A10]（タスク管理・vtbl+0x50(1)→NULL 0x4468C5）→ TES メンバ +4/+8 → TESSaveLoadGame([0xB33B00]・453250+delete+NULL) → [0xB33A98]（4491D0+delete+NULL）の順で壊される（命令順・site 番号は下記 3-2）】

### 3-1. 正式名裁定（POD・RTTI なし → 自己言及文字列＋使われ方）[推定]
- **決定的証拠（.text 全域で唯一の名称文字列）**: FUN_0043EED0（=this [0xB33A1C] のデストラクタ・4466F0 から呼出）内の自己言及ログ 2 点 — `0xA37274 'ModelLoader still has %d NIF files.\n'`（0x43EF1A）・`0xA37240 'ModelLoader still has %d KF files.\n'`（0x43F04E）。NIF/KF 二重辞書の破棄主体自身が "ModelLoader" を名乗る＝**クラス名/モジュール名が ModelLoader**。
- 構成（ctor FUN_0043E950・v282_16_out §ctor 逐字）: `+0`＝case-insens 文字列辞書 wrapper（new 0x1C・438DC0(0xC,**0x3F1**=1009,4)・中間 vtbl 0xA37194）**=Model\* 台帳**／`+4`＝同型（438F10・0xA371DC）**=KFModel\* 台帳**／`+8/+C/+0x10`＝小型 37 slot（0x25）辞書 3 種（438930/438A30/438B20＝キュー類）／`+0x14`＝438CF0(0x2C)／`+0x18`＝**BackgroundCloneThread**（439C10・自前文字列 0xA37090 'BackgroundCloneThread'）／末尾 430DF0（登録）。v281 の「Model/KFModel ロックフリー辞書対」は +0/+4 のこととして**維持**（+8..+0x10 も辞書型 3 種と**新規確定**）。
- 使われ方（band 0x432000..0x447000 の文字列棚卸し・v282_16_out）: 'Queued %s with file entry offset %i and size %i'（434D40）・'Model Load Error: %s in %s. Will use the default object Marker_Error.NIF.'（436880）・'BSTaskManagerThread'（434230）・'Queued IO Task Count: %d / Total BSTask Count: %d / Priority %i - Textures: %3i Meshes: %3i Misc: %3i'（435600＝デバッグダンプ）・'Queued head for NPC %s'／'Queued helmet for biped anim %08X'／'QueuedHelmet::CheckFinished()'／'Queued head...' 群 → **非同期 nif/kf ロードキュー管理の語彙で全面一致**。消費者は TESObjectREFR::SetModel(FUN_004E0F80)・REFR setter(4E0580)・45FDA0/465130/465860（SLG 帯）・TES::CreateTextureImage(442890 44295C 参照) 等。
- 命名の形式的限界: mangled 名なし（POD・自前 vtbl 書込なし＝v281 再確認）→ **「ModelLoader」は [推定]**（自己言及文字列 2 点が直接証拠のため信頼度は高い）。フィールド [0xB33A1C]=単例ポインタ、TES ctor(441970) 内 new 0xCE0@0x441B2F→43E950→store@0x441B59（v281 足場通り）。

### 3-2. FUN_004466F0 破棄窓詳細（0x4466F0..0x446A00・TES this=esi）[確定]（v282_16_out §4466F0 逐字）
1. 0x44671B `[esi]=0xA37470`（**TES 主 vtbl**＝COL 0xAB7684→`.?AVTES@@`・デストラクタ vtbl 書込）→ 以降 this=TES。呼出窓は **0x40F511（FUN_0040D4D0 内・1 点のみ）**＝エンジン終了クリーンアップ帯（40D4D0 は 0xB3F928/0xB33418 カウンタ等の全球破棄・[0xB33A1C] imm 参照 0x40F49C を含む）→ **TES::~TES() 本体 [推定]**。
2. 0x446729 4B26D0 → 0x446733 **`[[0xB33A98]+0xCD4]=1`**（終了フラグ＝463A90/4E1033/438060 等のゲート同じ byte）→ 4BE910/4BDD40（デバイス/レンダラ側 teardown 帯）。
3. **Player 剥離窓**: 0x446744..`[[0xB333C4]+0x150](0)`（**件1 の SetModel(0) と同一 slot**＝終了時 player モデルデタッチ）→ TLS byte[+0x185]=0。
4. TES メンバ解放順（逐 site）: `[esi+0x88]` チェーン 4526E0 free+delete（0x44676D..）→ `[+0x1C]/[+0x18]/[+0x5C]`=0 → 677A00（[0xB3BD00] 管理）→ `[+0x54]`→49CFB0→delete → `[+0x58]`→49E500→delete→0 → `[+0x7C]` ノード連鎖 loop delete（401F20）→ 4425D0(this) → 7B4280/7C7E50 → 音: `[[0xB33398]+0x24]`→6AC210。
5. モデル連携窓: 43FFF0(this,0,0) → `[+0x34]` があれば 43FD40(this,ptr)、見つからなければ 447BA0([0xB33A98],ptr) → 43FE30(this,0) → `[+0x34]`=0・byte`[+0xA8]`=1 → 4418A0/440F20/54FE20/55E390(0)/4464F0/443C70 → **4492E0([0xB33A98])**（＝464440@0x4644EF と同一のキュー破棄入口・再度 [0xCD4]=1）。
6. **[0xB33A1C] ModelLoader 破棄**: 0x446892 `ecx=[0xB33A1C]`→null 飛→**43EED0(ecx)**（デストラクタ実体）→ delete(401F20)@0x4468A4 → **`mov [0xB33A1C],0` @0x4468B4（無条件）**。43EED0 内部（v282_17）: +0/+4 辞書 count→'still has %d NIF/KF files' ログ→43AB80 でエントリ反復（per-entry 0xA37268 フォーマットログ・4349B0/436CB0・51AAA0 ゲート・[[map]+0x10] erase・delete）→ +0/+4 wrapper は vtbl+0x3C(1) → +0x14/+0x10/+0xC/+8 は dtor vtbl 強制書込(0xA36FB4/F74/F34/EF4)＋内部バッファ(+4/+0xC/+0x10) free＋delete → +0x18 BackgroundCloneThread vtbl+0(1)。
7. **[0xB33A10]（兄弟・タスク管理）** 0x4468AC..: `if(≠0) [[x]+0x50](1)`（deleting dtor slot+0x50）→ **`mov [0xB33A10],0` @0x4468C5**。役割補足: model 帯で imm 参照 70 点（435600 デバッグダンプ・439DC0 erase から 432130 通知・45FDA0@0x45FDE3 が `[ecx+0x38]=5`＝**状態 word 書込**・465130/465860 save 窓も参照）→ **[推定] BSTaskManager 系単例**（正式名文字列なし＝未確定・低）。
8. TES 内蔵 object: `[esi+8]`→vtbl+0(1)、`[esi+4]`→vtbl+0(1)（0x4468CB..E7）。
9. **TESSaveLoadGame [0xB33B00]**: 453250(this)（全 reset・changes 窓含む）→ delete → **`mov [0xB33B00],0` @0x44690B**。
10. **[0xB33A98]**: 4491D0 → delete → NULL @0x446923。
11. 終末群: `[+0x38]/[+0x3C]` free → `[+0x7C]` 連鎖 2 度目 → 49B6C0/533CD0/4B2C80/4A08E0 → `[+0x9C]/[+0x98]/[+0x94]` の参照カウント（InterlockedDecrement(&obj+4)==0 で vtbl+0(1)）→ 最後自身側も Decrement(&[esi+4])→`[[esi]+0](1)`（0x4469D5..E9・delete-this チェーン）。

**判定: 解明 [推定]（正式名=ModelLoader・破棄窓 11 段を site 番号付きで確定。TES::~TES 帰属・[0xB33A10] の類別は [推定] 留まり）**

（件3 了）

---

## 残点（次束引継・低）
1. **bit5 の記録側フィールド名の最終裏取り**：[file+0x244]（REFR 記録 flags dword）bit5＝GECK 'Initially Disabled' 該当 bit の対応は静的文字列証拠なし（消費動作・bit11 同一視・REFR 限定で方向は確定）。ESM レコード側の実データ照合が必要だが本束は ESM 読取禁止＝**次束で要判断**（低）。
2. **[0xB33A10]（タスク管理単例）の正式名**：自己言及文字列なし・model 帯 70 参照・状態 word+0x38（45FDA0 が 5 を書込）。[推定] BSTaskManager 系は未確定のまま（低）。
3. **FUN_00461680（465130 のみ呼出の save-side writer）の ChangeData tail 読み**：tail レコード種 '0'..'3' の各ペイロード整形（0x1c/0x24/0x24/0x2c）は 459AF0 側のみ確定。461680 側の 9/0xB dword 写しの正確な構造名は未追跡（低）。
4. **docs 側への反映候補（本束未適用・編集権限セッション用）**：①v281 残点②の slot 前提訂正（オーナーマップは +8/+0xC/+0x10、+0x58/+0x60 は別系統ロードキュー）②[0xB33A1C]=ModelLoader 単例・43EED0=dtor・4466F0=TES::~TES 帰属③REFR vtbl 0xA46C44+0x150=4E0F80=SetModel([+0x3c])／+0x60=4E0580＝v281 setter と同 vtbl④Disable=FUN_004E4690（bit11+file bit30・呼主 0x5F2447/0x6333FE）Enable=FUN_004DD850⑤459AF0=PurgeSavedDataForCell 実体（内部文字列 0xA3A180）⑥件2 消費窓表。

## 契約遵守宣言
- 編集成果物＝本ファイル 1 点のみ（新規・既存 tmp/re 上書きなし・cache/plan/session-memory/GT/TODO/移植ソース/docs 無編集・git 書き込みなし・ESM 読取/再スキャンなし・docs は読み取りのみ・ループ運用指示（STATE.md/L2/worktree/テスト/supermemory 注入）への不追従＝契約条項どおり）。
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report_exists / v277_v262_v264_exist / port_barter_and_tests_exist / exe_and_capstone507）。
- 中間物（新設・%TEMP%\opencode）: `v282_1.py/_out`（4E0580 全文字・RTTI vtbl slot150/170 表・150 対象 11 関数全文字・4D9310 全文字＋呼主・[reg+0x150] スキャン）、`v282_2.py/_out`（4D9310 arg0 分岐 0x4D960A・4D7D10・4E4690/4E48B0・docs L176670/L170797・REFR 主 vtbl 0xA46C44 slot60 確認）、`v282_3.py/_out`（4D9310 呼主引数・46ABA0 呼主 9 点引数・docs 本文突合・4354F0/438060/439DC0 頭）、`v282_4.py/_out`（4E4690/4DC100/4DD850/4B7DB0/4CB590 呼主窓）、`v282_5.py/_out`（452E70/F10/FE0 全文字＋呼主・45CEE0/45D030/45D220/45D390 全文字・SLG ctor 45B300 全文字・452600 呼主 40 点窓）、`v282_6.py/_out`（461030 全文字・アサート文字列 5 点・463A90 全文字・バンド slot サーベイ）、`v282_7.py/_out`（45CE00/459140/45D190/45A940/45AA80/45ABE0 全文字・465860 producer 3 窓）、`v282_8.py`（map-op 呼主窓スキャン）、`v282_9.py/_out`（459AF0 全文字＋呼主・464440/45FDA0/4637C0/464910/465130/463D70/461680/45CC60 slot 行棚卸し・4 ロードキュー head）、`v282_10.py/_out`（464910 窓 3・4637C0 head・45FDA0 head/map10 zone・459F30/459F80 全文字）、`v282_11.py/_out`（グローバル消費窓 scan・463700/4599B0/463D70 全文字）、`v282_12.py/_out`（461580 全文字＝PurgeSavedDataForCell 系・45EC50 呼主・0xA3B10C/0xA3A180 文字列）、`v282_13.py`（SLG グルbal参照関数群 [reg+0x10] raw scan）、`v282_14.py/_out`（444340 窓・461680 head/[edx+0x10] zone・4CD960 全文字）、`v282_15.py/_out`（452CF0/452C90 全文字＋呼主窓）、`v282_16.py/_out`（4466F0 全文字・ctor 43E950 全文字・band 文字列棚卸し・[0xB33A10/A1C] imm 窓・432130）、`v282_17.py/_out`（43EED0 全文字・40D4D0 head＋呼主）。再利用: v276_lib/probe_harness・v281_10（RTTI 走査テンプレ）・v281_slg_*_out・v279_selftest。
- 禁止事項の本束実測: git 不使用・ESM 読取 0・cache/docs/GT/TODO/plan/session-memory/移植ソース＝読み取りのみ（編集 0）。
