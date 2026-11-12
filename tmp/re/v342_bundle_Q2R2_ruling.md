# v342 — 次束Q2/R2（#115）: v330 §7/v331 残余の静的 6 件の裁定

- 作成: 2026-11-09 / L1 report-only / 静的解析のみ（親直接解析・ユーザー承認レーン）
- 出典: #115（row315/316 親突合時に起票）＝ v330 §7 残点 + v331 R1/R4 残余
- 対象 EXE: Oblivion Steam v1.2.0416（ImageBase 0x400000）・原本 EXE/ESM/.ess/解析原本/移植ソースは無変更（読取のみ）
- probe: `v115_1.py/.out`(448行)・`v115_2.py/.out`(99行・④のみ JPT エントリ誤読で空振り→v115_3 で差し替え)・`v115_3.py/.out`(106行)・`v115_4.py/.out`(20行) — 全 EXIT=0／`%TEMP%\opencode\` 保持
- 判定語: [確定]/[部分確定]/[推定]/[未確定]/[確定負]（5段階・旧結論は §4 訂正節で併記）
- 帯宣言: 深= v319 自帯(0x452000-0x467920 外は該当窓なし)＋**v318 自帯(0x44F000-0x452000/0x404000-0x406000…)#89 完了済で深可**＋factory 0x447DF0/0x448B60（v325 明示項目依存・浅層80命令の先例を ctor 特定に必要な 0x48/0x140 窓まで拡張＝**宣言済最小逸脱①**）＋0x447050 本体全文（**v325 §残点1 が明示許可した項目**＝宣言済例外②）＋ 0x6B6B90/0x4FBB60 窓（v325 R2 対象・帯外命令窓のみ＝**宣言済逸脱③**）＋ #106 帯 0x4C0000-0x4E0000（**#106 完了済のため深可**・capstone 線形 1 本）。機械走査= 全 .text regex+capstone 検証（③⑥の census）。

---

## 1. 六件裁定表

| # | 残件 | 本束裁定 | 根拠 |
|---|---|---|---|
| ① | 0x447050 slot7 無ゲート 21 reader の xor 全数帰属 + n=0 群 + 本体全文 | **[部分確定]**（本体 al 直返は **[確定(命令)]**） | 本体 36 命令全文: `mov edx,[esi]→[edx+0x1c]` 呼び→`mov byte[esp+0x10],al`→終端 `mov al,[esp+0x10]; ret` ＝ **al=slot7 戻り値の直返を全文で閉合**（v318 の head 36insn 依拠から格上げ）。21 reader 再 census: xor 直接ゼロ書 32 サイト＝**ARG 型 24**（push/引数算出へ消費・0x4B3EA4/0x4B3ECE の ACTI 系含む）／**FAIL 型 5**（0x4A5941→ret・0x4DA080→ret・0x69D990 `xor al,al; ret` 型Getter・0x4AE1D3/0x4AE1DE の FLOR 系2は lea 経由で ARG 寄り=疑い付き）／**UNK 3**（0x41B713 RESET 後・0x982DF7・0x51F8F0）。n=0 群 9: `mov al,1` 恒成功 2（0x4A41A0/0x4A5020）・`al=[ecx+6]` 等データ返却 1（0x51CD10）・**3命令 jmp tail thunk 3**（0x51EAF0/0x51E750/0x526CC0＝成功帰属は jump 先側＝[未確定]）・14命令無ゼロ書 1（0x5388D0）・その他 2（0x468440/0x4A4860 は al 不変） |
| ② | 非IAT 間接 killed-by-call 2 件（0x6B6D2D/0x4FBC4C） | **[部分確定]**（命令形は [確定]・先は動的） | 0x6B6B90 fn: 直前 call で死んでいるのは**別分岐**（0x6B6CEF の方）で、0x6B6CD7 `mov edx,[ecx+0x40]`（ecx=[[esi+0x54]]）→ 0x6B6D2D `call edx` は **[[this+0x54] の vftable スロット 0x40 仮想呼び**（load→call 間に殺し call なし）。0x4FBB60 fn: 0x4FBC32 `mov eax,[edx+0xd4]`（edx=[edi]）→ 0x4FBC4C は **this の vftable スロット 0xD4 仮想呼び**。両者「自由関数ポインタ」ではなく vtable ディスパッチと確定（＝静的には slot 番号まで、実先は実行時型依存=#82 動的 lane のまま） |
| ③ | E9/EB head 拡張則の rule v3 本採用可否（ラベル検証セット） | **[確定（検証セット水準）]＝採用可（tier 条件付き）**。re_util 変更禁止のため文書化のみ | 16/16 ラベル head: 全て基則で通過（CC4=13・C2i16=2・C3=2。拡張は基則成立 head の判定を変えない＝**非回帰確認**）。v323 §4-9 の6例: **0x4637C0 のみ基則=False→拡張(E9)で True（callers=4）**、他5例は C2i16 で基則 True（＝v323 の「尾 jmp 終端が通過しない」は 0x4637C0 型 1 件を厳密裏付け、残りは既知の別要因）。全 .text 拡張のみ head 候補=4,055、うち **E8 callers>0 の tier A=69**（0x402BD0 callers=85・0x4511C0(型 getter)=73・0x447DB0=8・0x4637C0=4…）→ 採用規則案: 「16アライン・CC k≤3・終端 E9/0F8x@A−k・callers>0（tier A）」を rule v3 に追加（tier B 4,000 件弱は不採用＝v323 検出手順の条件(iii)(v)と整合） |
| ④ | factory/ctor→MAIN vtable 表の疑義行（TESIcon 集約10型・TESFlora slot7=0x63696E61） | **[確定（表の再構築と誤帰属の特定）]**（0xA320A4 の正式名は [未確定]） | 64 stub→ctor→無変位 `mov [reg],imm` 全数抽出（v115_3/4）。**MAIN(this) として 0xA320A4 を書くのは 7 型のみ**（MGEF12/APPA19/BOOK21/LIGH26/MISC27/WEAP33/AMMO34・slot7=0x470590(code)）。**INGR25/ALCH40/QUST59 は 0xA320A4 を [edi]（サブオブジェクト）にのみ書き、MAIN は個別**（0xA33F5C/0xA3217C/0xA541EC）→ v325_01d の「10 型集約」行は this と sub-object の混載と確定（**訂正 C-1**）。**TESFlora: 01d が vftable 扱いした 0xA41C40 は +0x10 以降 'ITMpOrganic…' ASCII データ＝FlagInfo 系データ構造**（vftable ではない）。真の MAIN vftable=**0xA41B1C（+0x1C=0x4AE0A0＝v325 無ゲート21 の TESFlora(0x4AE0A0) と一致）**（**訂正 C-2**）。ARMO/CLOT/CONT は窓 0x140 で解決（MAIN 0xA441BC/0xA44644/0xA44824+サブ2-3）→ v115_2 初版の「NO-VPTR in 0x80」は窓不足（訂正 C-3）。**判定使用の vtable_map census（mismatches=0）は無傷**（v325 §5-5 追認） |
| ⑤ | #106 帯 `[reg+0x30]` の算術合成/間接まで含む全文意味正規化 | **[確定（帯内の窓読み不在）]** — bounded→full 側は帯限定で閉合。帯外（indexer fn 本体 0x482150 の読み）は本束の宣言窓外 | 帯 0x4C0000-0x4E0000 を capstone 線形全走査（#106 完了済のため深可）: 非 esp/ebp の `[reg+0x30]` plain=**0 件**（v331 追認）・SIB 形 78 件は全数 `*4`/`+0x30` フィールドアクセスで、窓インデクサの合成形 **`*8`＋`0x30` 併存は 0 件**（v115_4 (5b)）。窓 slot(1,1) 直読み候補は帯内に存在しない＝定数 (1,1) は v327 の 0x49A77A 系統（帯外 indexer 経由）に閉じる主張と矛盾なし。残余の「意味正規化全 .text」は窓インデクサ呼び出し側の話であり、③の tier A 再 census で実質代替（新規残件化せず） |
| ⑥ | vtable+0x1C 間接呼び caller 列挙と NULL スロット寄与（R4） | **[部分確定]**（列挙 [確定・機械]・NULL 分岐の非参照は [確定(命令)]・寄与比 [未確定]） | 全 .text `call/jmp dword ptr [reg+0x1c]` 直接形 = **179 サイト**（capstone 検証済・jmp 型 thunk 群 0x406BC0 台＋CRT 0x90xxxx 群含む・うち reader 窓内 0 件＝21 reader は load+call 形）。**FUN_00482170**（callers=1: 0x444275）: obj=0 時は `je 0x482198` で **self(ecx) の vftable+0x1C（slot7）を this-call するだけ＝obj は deref されない**（v331 R4 の「困難 [推定]」を命令で確定方向に補強。**訂正 C-4: 482170 の NULL 分岐はクラッシュ経路ではない**）。NULL 主源（新規配列 memset vs clear 経路）の寄与比は実行時観測待ち=#82 |

**集計**: [確定]2（③④）＋⑤ [確定(帯内)]／[部分確定]3（①②⑥）／[未確定]残部（① thunk 先3・④ 0xA320A4 正式名・⑥ 寄与比・② 実先）。主観点の [確定負] はなし。

## 2. probe 一覧（再現）

- v115_1（①②⑥）: 21 reader head 検証（基則/拡張両方）・xor サイト forward-walk 分類・n=0 群 al 終端・0x447050 全文・②窓（LOAD/kills マーカー付線形）・⑥ ff /2 disp8=0x1c regex+capstone・0x482170 head+callers
- v115_2（③⑤）: head_rule(ext) 検証器・全 .text 16アライン sweep（追加4,055/tier A69）・#106 帯線形 `[reg+0x30]` plain/SIB 分離
- v115_3（④本体）: factory JPT@0x448B60 64 エントリ→stub（default 0x448B26 除外・call new 0x401f00/エラー 0x4A7A60 除外）→ctor→ecx-map 追跡（`mov reg,ecx` 連鎖）→無変位 vptr 書（MAIN vs sub 分離）→slot7 妥当性
- v115_4（④⑤クロース）: 0xA41C40 内容 dump（ASCII 発見）・0xA41B1C+0x1C=0x4AE0A0 確認・ARMO/CLOT/CONT 窓 0x140・帯 `*8+0x30` = 0 検証
- 教訓（手法）: **JPT→stub の「最初の call」は operator new（0x401f00）。ctor は「new 以外の最後の直接 call」**（v115_2 空振りの原因。窓 0x24 では ctor call が5バイト越えでこぼれる→0x48 に拡張）。**無変位 vptr 書でも this と sub-object を ecx-map で区別しないと v325_01d と同じ混載をする**（＝本残件の発生機序そのものを再現確認）

## 3. 帯遵守の自己宣言

深解析: v318 自帯（0x44F000-0x452000 の 0x447DF0/0x448B60/0x447050/0x4511C0 文脈…は「明示項目依存の窓」範囲内）・#106 帯 0x4C0000-0x4E0000（owner 完了済）。宣言済逸脱: ①factory/ctor 窓（v325 先例拡張）②0x447050 全文（v325 残点1 許可）③0x6B6C60 台・0x4FBB60 台の命令窓（v325 R2 対象の解決に最小限）④0x482170/0x482150 近傍の head 窓。機械走査（文脈展開なし）: 全 .text の regex prefilter×capstone 検証（③⑥）。v316/v317 自帯・v319 自帯の深部は不要なし。禁止帯（v316 帯深=他専有なし・クリーンルーム）遵守、ESM/git/docs 無接触。

## 4. 訂正節（旧記載は併記・非削除）

- **C-1（v325 §5-1 疑義行）**: 「TESIcon(0xA320A4) に集約される 10 型」→ 本束再構築で **MAIN 集約は 7 型**、INGR/ALCH/QUST の 0xA320A4 はサブオブジェクト書（v325_01d が this/sub を分離せず）。旧 10 型表記は誤帰属として併記。
- **C-2（v325 §5-1 TESFlora slot7=0x63696E61）**: 「ASCII 混入＝vtable 特定誤り」の疑いを**確定**: 0xA41C40 は FlagInfo 系データ（+0x10 に 'ITMpOrganic'）、真の vftable=0xA41B1C（slot7=0x4AE0A0）。TESFlora の無ゲート帰属自体は v325 census のまま有効。
- **C-3（本束 v115_2）**: 初版④の「NO-VPTR/空集合」は probe 瑕疵（first-call=operator new 誤取得・窓不足・sub-object 混載）。**v115_3/4 を正典とし、v115_2 の④節は不採用**（E5→F1 の系譜に連なる記録）。
- **C-4（v331 R4）**: 「obj=0 直接到達は困難 [推定]」→ 0x482170 の NULL 分岐が self slot7 のみで obj を deref しないことを命令列で確定（[確定(命令)] へ補強）。NULL 主源の寄与比は [未確定] 維持（#82）。
- **C-5（v325 R2 の語）**: 「killed-by-call 2 件＝実体が出ない」→ 両件は vtable slot 仮想呼び（+0x40 / +0xD4）と確定。実先の具体クラスは実行時型依存のまま [未確定]。

## 5. 新規残件（次束へ）

1. [未確定] ① tail-jmp thunk 3（0x51EAF0/0x51E750/0x526CC0）のジャンプ先 reader の成功/失敗帰属（帯内・機械で到達可）。
2. [未確定] ④ 0xA320A4 の正式名（TESIcon vftable 仮説の RTTI 裏取り。COL/RTTI は #91 旧帯＝深可だが、0xA320A4± の COL 走査は今回未実施）。
3. [未確定] ⑥ 179 サイトの tier 化（this-呼び vs 自由間接）と NULL 主源寄与比（=#82 連動）。
4. [未確定] ③ tier B（callers=0 の拡張のみ head ~3,986 件）の vtable-参照による第2 tier 判定（本束は callers>0 のみ裁定）。
5. [部分確定] ② の slot 実体（this+0x54 の vftable+0x40 / this の +0xD4）の先候補クラス列挙（vtable 所属候補 census で静的縮小可）。

## 6. 自己確認

- v342: 11,856B / 57 lines（この行を含む最終実測）
- 台帳反映: row318（V318）統合・round_verify 合格後 bak169 確定
- probe out 4 本は `%TEMP%\opencode\`（v115_2 の④節は不採用明記）
