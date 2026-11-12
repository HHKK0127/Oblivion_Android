# v278 — SLG 残点束（2件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 271（低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル）**
- 出典残点: `tmp/re/v276_ui_ini_residuals.md` §6（TESSaveLoadGame 集計）末尾「残点」2 項＝①+0x48 書込経路（跨関数派生 base 未踏）②FUN_0045F180 の遅延確保定数 0x1FFFF000/0x60000000 の意味（サイズ上限説 [推定]）
- 確定済み足場（本束で再検証しない）: SLG=**vptr 非設置の非ポリモーフィック単例**（[0xB33B00]・0x1C4B）・メソッド集合=直接呼出不動点 **150**（work **571**＝針関数 445 ∪ methods）・**+0x44 = write-only 台帳**（書込 2 点 FUN_0045F180@0x45F1EA/0x45F2B6・定数 0x1FFFF000/0x60000000・読取 0 点）・**+0x48 = 読取 2 点**（`cmp al,[ecx+0x48]`@0x452195／`movzx esi,[ecx+0x48]`@0x4521E9・FormID 上位バイト境界）で**書込 0 点＝[未確定]**・+0xB0..0x1BC=完全死窓 [確定]
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA 換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00）、`tmp/re/cache/{named_funcs.txt,vtable_index.tsv,call_targets.json,named_funcs}`、`docs/Oblivion_decompiled.c`（部分検索）、中間物 `%TEMP%\opencode\v276_*`（g9 走査器・lib 再利用）
- selftest: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest` → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）
- 中間物: `%TEMP%\opencode\v278_1.py/_out`（件2: FUN_0045F180/FUN_004535A0 全文字＋callers 使用型・1b: 兄弟定数使用・1c: FUN_004E0580 全文字・1d: 定数 imm 全域分类＋[reg+0x44] store 走査・1e: 台帳帰属＋merge-block 判定＋this 同一性）、`v278_2.py/_out`（+0x48 エスケープ追跡・初版）、`v278_2b/_out`（全 site の ctx 視覚検証＋callee store 一覧）、`v278_2c/_out`（マージブロック台帳境界＋vtable ポインタ検索）、`v278_2d/_out`（0x4E0840/0x4E2D60 帰属・GetExtraData 配管確定）、`v278_dbg/dbg3/_out`（g9 の seed pop バグ再現＋マージブロック状態リーク再現）、`v278_3.py/_out`（境界リセット付き確定版 walk・初回）、`v278_4.py/_out`（89 形式レジスタ store＋台帳 C 行の関数帰属）、`v278_5.py/_out`（Ghidra 台帳 `+0x44/+0x48) =` の全 occurrences をアンカーで FUN 帰属）、`v278_6/7/_out`（生成窓 FUN_00441970 の esi 帰属）、`v278_8.py/_out`（自己多重書込型 cmp の取込漏し 3 site 特定）、`v278_9.py/_out`（**確定版最終集計** site-check を状態更新前へ移動）、`v278_10.py/_out`（[reg+0x48] byte store 形式 全域走査）、`v278_11.py/_out`（アンカーバイト列検証）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage、リポジトリ AGENTS.md の loop 節）には本契約により**従わない**。

---

## 件1 v276 §6 残点①: SLG +0x48 の書込経路（跨関数 &this+K 派生 base の追跡） — 【確定：跨関数経路も書込ゼロ。[確定負]に格上げ（v276 の「静的到達域外 [未確定]」は小残点として解消）。ただし本走査で **v276 g9 集計器に 3 つの取込漏しバグ**が発覚し、+0x44 側の足場数値を訂正（+0x44 は write-only ではなく **書込 6 点/3 関数・読取 17 点/12 関数**）】

### 1-0. g9（v276 最終走査器）のバグ再現と修正（v278_dbg / v278_dbg3 / v278_9）[確定]
- **バグa（seed pop）**: g9 の walk は `mov reg,[0xB33B00]` を seed した直後、同一命令を mov/lea 追跡部で `else: slg.pop(dst)` に落とし、**グローバル seed 追跡が全滅**（v278_dbg: FUN_00663070 の `mov edx,[0xb33b00]`→`mov [edx+0x44],0x1ffff000`@0x6633C3 が slg={} のまま＝site 0）。→ 修正: seed マッチ時は pop 節をスキップ。
- **バグb（マージブロック越境リーク）**: Ghidra 台帳は int3 で区切られた複数サブメソッドを 1 関数に潰す（例 0x6A7290..0x6A73AF）。g9/初版 v278_2 は `ret` で状態を消さず（v278_2 は call のみ kill）、前サブメソッドの `mov ecx,[0xb33b00]` が次サブの `mov esi,ecx` に漏れて **幻の R/W site** を生成（0x6A72ED・0x6A7337・0x6A7373・0x67D567/0x67D58B・0x4E2DE3 ほか）。→ 修正: `ret` 直後に int3 パディングが続いたら全状態リセット。
- **バグc（自己多重書込 read の取込漏し）**: `mov eax,[eax+0x44]` 形式は dst が base と同一のため、site チェック（更新後）で base が消えて読取点を見落とす（g9 と v278_3 まで同様）。0x425825・0x66CF93・0x6A2F90・0x4D26A3 ほか。→ 修正: **site チェックを状態更新前**に移動（v278_9＝確定版）。
- 確定版 walk 条件: methods=raw 88＋不動点 60＝**148**、work=**569**（g9 の 150/571 に対し、幻 method 化していた 0x4E0840/0x4E2D60 等ポリモーフィック受信者を正しく除外）。

### 1-1. 「跨関数で &this+K を渡すヘルパ」候補の全排除 [確定]（v278_2 E/E2 → v278_9 で正当ゼロ）
確定版 walk が拾った +0x44/+0x48 の lea 派生・push・ecx 引き渡し候補 5 件を個別帰属。すべて SLG でない:
- **0x6A7290 マージブロック**（0x6A72E0 `mov eax,[esi+0x48]`／0x6A7350 `mov [esi+0x48],edx`@0x6A7373・`add esi,0x44; push esi; call FUN_0045B9A0/0x4534D0`）: 0x6A72B0/0x6A72E0/0x6A7350 は **vtable スロット**（file 0x675EA8-EB0＝VA 0xA76CA8..0xA76CB0・COL 0xAC55B4）＝受信者はポリモーフィック→SLG（vptr 非設置）になり得ない [確定]。+0x40/+0x44/+0x48 はそのクラス自身のストリーム書直列化フィールド（buf=&this+0x40 を SLG のストリームヘルパ FUN_0045B9A0/0x45BA00/0x4534D0/0x45F7A0（`mov ecx,[0xb33b00]` で呼出＝SLG 側メソッド）へ引数渡し）——SLG+0x48 への書込ではない。
- **FUN_004E0840@0x4E0843・FUN_004E2D60@0x4E2DCA/0x4E2DF3**（`lea ecx,[esi+0x44]; call FUN_004211C0/0x41E6F0/0x421A00`）: FUN_004211C0＝`push 0x4B; call FUN_0041E210; mov eax,[eax+0x14]`、FUN_0041E6F0＝`push 0x1A; call 0x41E210; mov eax,[eax+0xc]` ＝ **GetExtraData(type) 配管**（v276 §6-4 の 0x4203A0/0x420340 と同型）→ &this+0x44 は extra コンテナ。受信者には `mov edx,[esi]; call [edx+0x190]` の vtbl 間接がある（0x4E2DE1/0x4E2E05）＝ポリモーフィック→SLG でない [確定]。e8 呼出側（0x45D3EB ほか）も this=保存中オブジェクト。
- **FUN_0067D3A0@0x67D567/0x67D58B**（`[esi+0x44]` を読んで `mov eax,[eax+0xc]` と追従／`[esi+0x48]` 読→ストリーム書込 FUN_0046AC70 系）＋ FUN_0067D5C0（`mov [esi+0x48],eax`@0x67D63E）: 受信者の this が +0x40..0x54 のポインタ群を持つ**別クラス**（v276 §6-4 の 0x4DBF90 型誤帰属と同pattern）。バグb 修復後は site 自体が消える（幻）。
- 確定版での正当 escape（push &SLG+K／ecx=&SLG+K 引き渡し）＝**0 件**（v278_9 §E/E2）。

### 1-2. 形式網羅の負証拠（走査内訳）[確定]
| 走査 | 対象形式 | 結果 |
|---|---|---|
| 修正 walk（v278_9・work 569） | 追跡 base の [reg+0x48] W/ADDR/escape | **0**（R のみ 2 点＝0x452195/0x4521E9） |
| 全域バイト走査（v278_5 §3） | `mov dword [reg+0x48],imm/reg`（C7/89・SIB・disp32 含む） | 実命令 19 点＝すべて非 SLG 関数（0x402100/0x4066C9/0x41A15D/0x41A862/0x43DAA3/0x43F231/0x443289/0x44330F/0x44C3AE/0x4670BE/0x47461D/0x474A55/0x476569/0x52A1C3（=[esp 誤検出]）/0x62665B/0x67D63E/0x68FC8C/0x441C6B（後述））。うち SLG ニードル関数内の 5 件は base が自 this（0x68FC80 に `mov [esi],0xA75344` の **vptr 設置**あり＝ポリ＝非 SLG） |
| 全域バイト走査（v278_10） | `mov/or/and/add byte [reg+0x48],imm8/reg8`（C6/88/80 /x） | 37 点＝**SLG 関数集合（ニードル 445∪methods 148）外のみ**（0x51FD90/0x591022/0x5917F1/0x62249C/0x6D7170/0x6F73F0/0x96DD1D ほか） |
| Ghidra 台帳オラクル（v278_5） | `DAT_00b33b00 + 0x44|0x48) =`（グローバル base 直接 store） | **0 件**（+0x44/+0x48 への DAT ベース書込は台帳全体に存在せず・読 16 行はすべて `==` 述語） |
| 生成窓（v278_6/7） | `mov [esi+0x48],0x7FFFFFFF`@**0x441C6B**（FUN_00441970 内・SLG new@0x441B94 と同一関数） | esi=`mov esi,ecx`@0x441996＝**FUN_00441970 自身の受信者**（SLG シングルトンはこのメソッド内で new＋[0xB33B00] 登録@0x441BB9 される別オブジェクト）→ 0x7FFFFFFF 群は +0x38/+0x3C に配列 ptr を持つ兄弟マネージャのフィールド初期化。**SLG+0x48 の生成時初期化ではない** [確定] |

### 1-3. 判定
> **+0x48 書込経路 = [確定負]**（跨関数 &this+K ヘルパ追跡・直接 [reg+0x48] store（dword/byte・SIB/disp32 含む）・グローバル base store・生成窓の全網羅で 0 点。脱出路は v276 §6-1 の負確認（FF35/B8/B9/68 系 needle 0・rep 系 1 点のみ非 SLG）と併せて闭合）。読取 2 点（0x452195/0x4521E9）のみ実在＝**未初期化読取欠陥候補 [推定] は維持**（v276 §6-3 の裁定どおり、移植側は +0x48 の初期値を明示定義すること——参考: 兄弟マネージャ側の同オフセット慣用値は 0x7FFFFFFF（@0x441C6B）だが SLG のものと同一性の証拠はない）。
> 同時に **v276 §6-2 の「+0x44 = write-only」を訂正**（読取 17 点/12 関数が実在——件2 参照。g9 のバグa/b/c に由来）。

**判定: 解明 [確定負＋足場2点の訂正]**（機械語アンカー: 生成窓 `C7 46 48 FF FF FF 7F`@0x441C6B〔非 SLG〕・`8B 74 24 08`(引数読)等の全 store 点は v278_5/10 一覧。vtable 証拠: file 0x675EA8..EB0 → VA 0xA76CA8〔スロット 0x6A72B0/0x6A72E0/0x6A7350〕。GetExtraData 配管: `68 4B FF FF FF`? 実バイト=`6A 4B`@0x4211C0・`6A 1A`@0x41E6F0＋`E8`→0x41E210）

（件1 了）

---

## 件2 v276 §6 残点②: FUN_0045F180 の定数 0x1FFFF000/0x60000000 の意味 — 【確定：**サイズ上限説は命令レベルで棄却**。両定数はファイル属性ビットフィールドの**グループマスク**（A=bit12..28(+bit7)／B=bit29..30）で、+0x44 は「直前に file オブジェクトへ適用した属性グループ」を記録する**台帳**（read/write 両用・0x7FFFF000=A|B は消費側のみ）】

### 2-1. FUN_0045F180 の命令レベル全図（0x45F180..0x45F2E0・ecx=SLG→`mov edi,ecx`@0x45F19F・arg1=[[esp+8]]=要求フラグ dword・esi=[esp+0x10]=ファイルオブジェクト）[確定]（`v278_1_out.txt` §A 全文字）
| VA | 命令 | 意味 |
|---|---|---|
| 0x45F19C/0x45F1A1 | `mov ebx,ebp / and ebx,0xfff` | 要求 dword の下位 12 ビット＝モード要求を抽出 |
| 0x45F1A7 | `call 0x9832E6(esi,…)` | 名前/パス問い合わせ（失敗時 0x45F1B3 `and ebx,0xfffff7ff`＝bit11 クリア） |
| 0x45F1C0-C7 | `mov ecx,[0xB33A98]; call 0x446B80(…,[esi+0xc])` | プラグインファイル述語（v276 §1-2 と同一配管）→真なら FUN_0045C7A0 経由で早期 return |
| 0x45F1E3 | `call FUN_004535A0(esi=file, ebp=flags)` | **eax=属性ビットフィールド**（2-2） |
| **0x45F1EA** | `mov [edi+0x44],0x1FFFF000` `c7474400f0ff1f` | **台帳へグループ A 記録** |
| 0x45F1F6-FE | `and eax,0x1FFFF080` `2580f0ff1f` / `push eax` / `mov edx,[[esi]+0x60]` / `call edx` | **A グループビットだけをファイル vtbl+0x60（属性セッタ）へ適用** |
| 0x45F200-02 | `test ebx,ebx; je 0x45F2B6` | ebx==0 → 直接 B 経路 |
| 0x45F25E-2B4 | FUN_0045A500／`[esi+4]=='0'`／`call [0xA2808C]`(=GetCurrentThreadId・v274 §3.1)／`[edi+0x18]` bit0/bit18 マスク／FUN_0045A530/45C020 | スレッド同調＋種別判定（v274/276 の +0x18 意味と整合） |
| **0x45F2B6** | `mov [edi+0x44],0x60000000` `c7474400000060` | **台帳へグループ B 記録**（条件付第2書込＝v276 の「遅延」の正体） |
| 0x45F2C4-CC | `mov eax,ebp / and eax,0x60000000` `2500000060` / `call [vtbl+0x60]` | **B グループビットを同セッタへ適用** |
| 0x45F2D0-D2 | `push ebp / push esi / call FUN_0045B7A0` | 適用後の登録（file,flags） |

- 対 **ファイル側**: `[[esi]+0x60]` の仮想セッタへ「マスクで絞った属性 dword」だけ渡す——定数が**サイズ**なら AND 引数になること自体が不可能（上限は cmp/ja/cmov で実装され、AND はフィールド選択）。

### 2-2. FUN_004535A0（0x4535A0..0x453903・ret 8）＝属性フラグ照会・修復関数 [確定]（`v278_1_out.txt` §B 全文字）
- 関数本体は test/and/or の**ビット演算のみ**（`test ebx,0x10000000; je…`→該当述語 false で `and ebx,0xefffffff`（bit28 クリア）ほか、bit 24/26/25/27/19/16/17/23/11/10/8/3・`or ebx,0x2000000/0x4000000/0x80000/0x800000/0x10000/8/0x4000000`、`and ebx,0x7fffffff`@0x453792、`or ebx,0x80000000`@0x4538D5（失敗センチネル）、`and ebx,0xff7fffff`@0x4538DE。返却 `mov eax,ebx`@0x4538FD）。
- 操作される bit 域 ⊂ **{bit0..11（要求域）} ∪ {bit12..28} ∪ {bit29..30} ∪ {bit31}** ＝ AND マスク 0x1FFFF080（bit7＋bit12..28）／0x60000000（bit29..30）の定義域と整合。
- 名称根拠: 呼び出し側 FUN_0045CD2C のエラー文 0xA3A70C `'References changes were nullified by CheckFlags().'` → 本関数系は **CheckFlags 系** [確定(名称文字列)・正式名は復元外]。

### 2-3. サイズ上限説の棄却（命令レベル根拠）[確定]
1. 定数の使われ方は全画像で **AND マスク 4 点**（0x45F1F6/0x45F2C4/0x4664EE/0x46672C・and ecx 0x6633CC）と **== 比較のみ**（1d §A: `cmp eax,0x1FFFF000` 型 17 点＋`cmp eax,0x7FFFF000` 17 点＋`cmp eax,0x60000000` 3 点）。`cmp reg,0x1FFFF000 / ja`(上限クランプ)形・加算減算形は**ゼロ**。
2. AND 引数 0x1FFFF080＝台帳値 0x1FFFF000＋bit7——数値としてではなく**ビット集合**としてしか意味が通らない（0x80 は FUN_004535A0 の `add esi,0x80; cmp esi,0xff; ja`（0x453645..51）系が選択する状態 bit）。
3. 第3値 0x7FFFF000 = 0x1FFFF000 | 0x60000000 の**ビット和**（0x1FFFF000+0x60000000 と同値だが、消費側の述語は `==A || ==A|B` / `==B || ==A|B` の排他的グループ判定＝2 ビット状態の enum として振る舞う＝サイズ域では成立しない）。
4. 台帳セッタ群（2-5 の 3 関数）は store 直後に必ず「同定数 AND→ファイル vtbl+0x60 呼出」を伴う——サイズ上限なら AND 結果をファイルへ渡す理由がない。
5. 0x1FFFF000 の 18 ヒット中、数値として読む余地のある場所（FUN_004E0580@0x4E073A ほか）はすべて**台帳値との == 一致判定**（v278_1c §全文字・2-4）。

### 2-4. 使われ方＝消費側 12 関数 17 site [確定]（v278_9 §C 最終集計・全 site は `mov reg,[0xB33B00]`→`mov eax,[reg+0x44]`→`cmp` 形）
- A 述語（`==0x1FFFF000 || ==0x7FFFF000`＝「A グループ適用済み」）14 site: FUN_00425650(0x425825)、FUN_004D2230(0x4D26A3)、FUN_004E0580(0x4E0737/0x4E0761)、FUN_005F85E0(0x5F85E5/0x5F8606/0x5F8743)、FUN_00608410(0x6088C3)、FUN_00612110(0x612123)、FUN_006324F0(0x632E7B)、FUN_006478E0(0x6478FA)、FUN_00651880(0x65195F)、FUN_0066CF80(0x66CF93)、FUN_006A2F80(0x6A2F90)。
- B 述語（`==0x60000000 || ==0x7FFFF000`）3 site: FUN_004E0580(0x4E07C2)、FUN_006324F0(0x633053)、FUN_0065A7F0(0x65A83D)。
- 分岐の中身（例）: FUN_00425650——台帳∈A系 のとき extra 0x1F/0x21/0x23/0x35/0x17/0x8 と NiTPointerMap 経由のレコードをシリアライズ（decompiled L27480-27505 逐語＝`FUN_0041E210(0x1F)` 群）。FUN_0066CF80——A系なら `fldz; fst dword [0xB3BB08]` 等状態リセット。→ **+0x44 は「open 時に適用済みの属性グループ」を後段の保存/負荷直列化が分岐参照する台帳** [確定]（bit 単位の和訳は範囲外＝残点）。
- 0x7FFFF000 の静的 producer は**不在**（imm 走査＋walk＋台帳オラクルで 0 点）＝消費側が受け付ける複合状態は現 build で到達不能 [確定負]（防御的 or 他ビルド由来 [推定]）。

### 2-5. 台帳 producer＝3 関数 6 書込点 [確定]（同一イディオム「store→AND→vtbl+0x60 セッタ呼出」）
1. **FUN_0045F180** 0x45F1EA(A) / 0x45F2B6(B) — v276 §6-2 [確定] に対し本束で**AND 併用と意味を確定**。
2. **FUN_00465860**（0x465860..・SEH・大域 open/書き込み経路）0x4664E0 `c7454400f0ff1f`(A)／0x466720 `c7454400000060`(B)＋0x4664EE `and eax,0x1FFFF080`／0x46672C `and edi,0x60000000`——base ebp=`mov ebp,ecx`@0x4658A9＝受信者。受信者=SLG の証拠: 5 callers 中 0x5025F7/0x5AE1E6 が `mov ecx,[0xB33B00]` 直接・0x466D5F（受信者 FUN_0046BE0=[ecx+0x1C0] アクセス＝SLG ライブ末尾）・0x466B31（FUN_0046B00∈methods）＋Ghidra 本体 L80358/80438 `*(param_1+0x44)=`。
3. **FUN_00663070** 0x6633C3 `c7424400f0ff1f`(A・edx=[[0xB33B00]]@0x6633B9)／0x663425 `c7414400000060`(B・ecx=[0xB33B00]@0x66341F)＋0x6633CC `and ecx,0x1FFFF000`／0x663433 `and eax,0x60000000`——同関数は `[eax+0x7d]`/`[eax+0x14]` も直接書込（v274 §3.2 の SLG メンバと一致）＝SLG this 確定。
- v276 の「producer=FUN_0045F180 のみ／write-only」は g9 バグ a に起因する**部分誤り**（訂正: producer 3 関数・write-only ではない）。

### 2-6. 判定
> 定数の意味 [確定]: **0x1FFFF000=ファイル属性 bit12..28（A グループ・AND 時 bit7 選択 bit 併用）／0x60000000=bit29..30（B グループ）**の属性グループマスク。FUN_0045F180（および姉妹 FUN_00465860/FUN_00663070）はファイル open 時に CheckFlags 系（FUN_004535A0）の返す属性 dword を A→B の順でファイル vtbl+0x60 セッタへ適用し、**適用したグループを TESSaveLoadGame+0x44 の台帳**へ記録する。12 関数の消費者が `==A||==A|B` / `==B||==A|B` で保存/負荷ペイロード（extra 群など）の分岐に使用する。**サイズ上限説は棄却**。移植側は「+0x44=u32 グループ台帳（値域 {0x1FFFF000, 0x60000000}・初期未定義）＋open パス 3 系統での属性 group 適用の記録＋消費者分岐」で等価。

**判定: 解明 [確定]**（アンカー: `c7474400f0ff1f`@0x45F1EA・`2580f0ff1f`@0x45F1F6・`c7474400000060`@0x45F2B6・`2500000060`@0x45F2C4・`c7454400f0ff1f`@0x4664E0・`c7424400f0ff1f`@0x6633C3・`81e100f0ff1f`@0x6633CC・`c7414400000060`@0x663425・`8b4044`@0x425825（自己読形式）・`3d00f0ff1f`@0x425828・`3d00f0ff7f`@0x42582F・`894648`@0x441C6B（=生成窓の `mov [esi+0x48],eax`・eax=0x7FFFFFFF@0x441C5A・非 SLG）・CheckFlags 文字列 0xA3A70C）

（件2 了）

---

## 残点（次束引継・低）
1. 属性 bit12..30 の個別意味（セッタ `vtbl+0x60` の実装先＝ファイルクラス特定とビット名）＝範囲外。
2. 0x7FFFF000 を受け付ける消費者述語の producer 不在（防御的/他ビルド）——現 build では到達不能と確認済みだが、実行時に他モジュール（プラグイン系 loader）が台帳を直接書かない保証は静的には取れない。
3. FUN_0045F180 の引数 arg1=[dword ptr] の供給元（open モード要求フラグの組み立て側）は未追跡。
