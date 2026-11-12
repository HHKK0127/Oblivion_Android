# v276 — UI/INI・opcode 細部束（6件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 269（V269・低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル）**
- 出典残点: `tmp/re/v275_ui_residuals2.md` X-1..X-3、`tmp/re/v274_opcode_gt_names17.md` §3 内残点＋serial267 完了記録の引継①②、`tmp/re/v272_opcode_gt_names16.md` §2（0x52734B/TESNPC）
- 確定済み足場（本束で再検証しない）: [0xB06C74]=bFull Screen:Display storage・FUN_004A8800=汎用INIローダ（slot+0x10・書込 `mov byte [esi],al`@0x4A88DF／`mov dword [esi],eax`@0x4A8AB3）・int→float=read側 fild@0x58D336/fstp@0x589FDF・template999=Instantiate FUN_00585410（E8呼主68点）再生成型・extra 0x23=**ExtraFollower**（v272 §2 で推定 ExtraPackages は訂正済・生成唯一窓 FUN_00424C50）・0xB0613C=NiTPointerMap<uint,TESForm*>（+8 buckets=[0xB06144]・+0xC 入数=[0xB06148] v272 §5）・TESSaveLoadGame=0xB33B00（member map v274 §3）
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00）、`tmp/re/cache/{named_funcs.txt,vtable_index.tsv,call_targets.json}`、`docs/Oblivion_decompiled.c`、中間物 `%TEMP%\opencode\v275_*`/`v274_*` 再利用。Ghidra headless は本環境未稼働（list_instances 0）→ capstone 直行。
- selftest: 着手時 **ALL PASS (4/4)**（`probe_harness.py selftest` → bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）
- 中間物: `%TEMP%\opencode\v276_a.py/_out`（マネージャ vtbl・+0x10c/+0x108 参照・slot帯 dump）、`v276_a2.py/_out`（**.text 全域再開型走査**：[reg+0x10c] 226 点・virtual +0x10 呼出 477 点・+0x20 呼出 195 点の関数別全量）、`v276_b.py/_out`（件2）、`v276_c.py/_out`（件3）、`v276_d.py/_out`（件4）、`v276_e.py/_out`（件5）、`v276_f.py/_out`（件6 初走・loose feed 誤検出→g 系で置換）、`v276_g1..g11.py/_out`（件6 確定版: ctor 全文字/0xA39044=NiTLargeArray vtbl RTTI 照合/メソッド不動点 150・work 571/派生 base・scaled index・volatile kill/脱出路負確認）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、supermemory Daily Triage、リポジトリ AGENTS.md の loop 節）には本契約により**従わない**。

---

## 件1 X-1: FUN_004A8800（slot+0x10）を全エントリへ回す起動時ドライバの仮想呼出元個別 VA — 【確定：ドライバ＝**FUN_004A85A0**（0x4A85A0..0x4A85E3・マネージャ vtbl **0xA404F8 の slot+0x20**＝0xA40518 の値・Ghidra 台帳上は FUN_004a84e0 の +0xC0 内区間）。`[this+0x108]` ゲート下、`lea esi,[this+0x10c]` の登録リスト（head 0xB07BFC・node{+0 entry ptr,+4 next}）を walk し、各 entry へ `mov edx,[edi]`→`mov eax,[edx+0x10]`@0x4A85C9→**`call eax` @0x4A85CE**＝エントリ単位の FUN_004A8800 仮想呼出 site。ドライバ自体の仮想呼出元は全画像で **2 点のみ**: ①**0x40E747**（FUN_0040D4D0 内の起動系路：0xB3F280 書式パス→mgr+4 書込→slot+0x14@0x40E733 で [mgr+0x108]=this ゲート ON→**slot+0x20 call@0x40E747**→slot+0x18@0x40E757 OFF）②**0x513C52**（FUN_00513BB0・0xB02C84 別パスで同型 3 連、本関数自身 E8 呼主 0＝さらに仮想）】

### 1-1. ドライバ関数の命令レベル確定（`v276_a2` W/V 走査＋全文字）[確定]
- FUN_004A85A0（thiscall ecx=manager 0xB07BF0・ret 0）:
  ```
  0x4A85A4: cmp dword [edi+0x108],0 / setne bl      ; ゲート（[mgr+0x108]≠0 の時だけ周回）
  0x4A85B3: lea esi,[edi+0x10c]                     ; 登録リスト head（=0xB07BFC）
  0x4A85C0: mov eax,[esi]                           ; node.data = entry ptr（{value,name*} エントリ）
  0x4A85C6: mov edx,[edi] / 0x4A85C9: mov eax,[edx+0x10]   ; this vtbl slot+0x10 = 0x4A8800
  0x4A85CC: mov ecx,edi / 0x4A85CE: call eax        ; ★ エントリ別 INI→storage ローダ呼出
  0x4A85D0: and bl,al / 0x4A85D2: mov esi,[esi+4]   ; 成否 AND 蓄積・next へ
  ```
- 対になる兄弟ドライバ（同一 pattern の他 slot 版）も全量取得：0x4A8560（slot+0x1C）= 同 walk で **slot+0x0C**（FUN_004A85F0・readback）を呼ぶ（call eax@0x4A858A）／FUN_004A84E0=破棄（+0x110 チェーン free＋[+0x10c]=0＋vtbl を初期 0xA403C8 へ戻す）。＝「登録リスト周回機構」は 2 slot（+0xC readback・+0x10 load）で対称、命令レベル確定。
- マネージャ vtbl 0xA404F8 全集（窓 dump）: +0x00=0x4040E0(upsert)／+0x04=0x403C20(remove wrapper `add ecx,0x10c;jmp 0x65C620`)／+0x08=0x4A8B10／+0x0C=0x4A85F0／+0x10=0x4A8800／+0x14=0x4A83B0(`mov [ecx+0x108],ecx`)／+0x18=0x4A7CB0(`mov [ecx+0x108],0`)／+0x1C=0x4A8560／**+0x20=0x4A85A0（load-all ドライバ）**。+0x24 以降は書式文字列 ('%u,%u…' 0x252C7525 群) で table 終端。
- E8 callers(0x4A85A0)=**0**（実測）＝v275 の教訓どおり間接経路のみ。

### 1-2. 起動時呼出元 ①：FUN_0040D4D0 の 3 連スラート（`v276_a2` S/boot）[確定]
- FUN_0040D4D0（0x40D4D0..0x40F640・巨大初期化関数）内 0x40E710..0x40E757:
  1. 0x40E710-22: `[0xB3F280]` の NUL 終端文字列（ini パス様式）→ 0x1B0 バッファ経由で **[0xB07BF4]（mgr+4=パス dst）** へコピー
  2. 0x40E724-33: `mov edx,[0xB07BF0]; mov eax,[edx+0x14]; push ebp; mov ecx,0xB07BF0; call eax` ＝ slot+0x14 で **[mgr+0x108]=mgr ゲート ON**
  3. 0x40E739-47: 同形 `mov eax,[edx+0x20]; call eax`@**0x40E747** ＝ **load-all ドライバ（FUN_004A85A0）→ 内部 0x4A85CE で全 entry の slot+0x10＝FUN_004A8800 実行**
  4. 0x40E749-57: 同形 slot+0x18 → ゲート OFF。
  5. 直後 0x40E759: `call 0x53AC60` / 0x40E75E: `call 0x42F610`（表示初期系 [推定]）。
- FUN_0040D4D0 の E8 callers = {0x40DF54（同一函数内＝内部 2 回目の_reload 経路）, 0x4456B3, 0x44EB24, 0x50D567, 0x5BDD53}（`v276_a2`/単発照会）＝起動＋動画/device 再構成時 re-load の複数入口。
- v275 の「FUN_00404400/0x404540 帯がドライバ候補」は**訂正**：この 2 関数は slot+0x14→(0x0C/0x10)→0x18 の 3 連を**単一エントリ**（0xB02C4C・0xB02BB8 配列 0x1D 個は別経路の文字列書込ループ）へ当てる専用ペア（0x404486/0x4044A5/0x404518/0x4045DF は [eax+0x14]/[eax+0xc]/[eax+0x18]/[eax+0x10]）。全エントリ周回は行わない。

### 1-3. 起動時呼出元 ②：FUN_00513BB0（ランタイム re-load）[確定 位置／用途 推定]
- 0x513BB0..0x513C86: `[0xB02C84]` 文字列→バッファ→**[0xB07BF4]** 書込（0x513C23）→ slot+0x14（push 0）@0x513C3F → **slot+0x20 call @0x513C52** → slot+0x18 @0x513C61 → `push 0xA4D690; call 0x579B60`（0x579B60 帯＝UI リフレッシュ [推定]・0xA4D690=文字列/クラス名要約未追跡）。
- E8 callers(0x513BB0)=0＝**この関数自身も仮想呼出**（次の間接層＝本束範囲外・同型教訓枠）。姉妹 FUN_00513B60（E8 不在）は slot+0x14→**slot+0x1C**（readback-all）→0x18 の 3 連＝「値を書き戻す」側対（0x513B8A）。
- 付随確定: [mgr+0x108] ゲートは「INI 読取中」の再入フラグ（FUN_004A85F0 の readback 側も同じゲート for 自己設定@slot+0x14／解除@slot+0x18 を命令レベルで共用）。[mgr+4]（=0xB07BF4）は loader FUN_004A8800 に渡される ini パスバッファ（GetPrivateProfile* の file 引数側 [推定]）。

### 1-4. 裁定
> X-1 残点解消。**起動時ドライバ＝vtable slot+0x20 の FUN_004A85A0（0xA40518）**、エントリ別仮想呼出 site＝**0x4A85CE**（`mov eax,[edx+0x10]`@0x4A85C9 直後）、ドライバ起動 site＝**0x40E747**（FUN_0040D4D0・起動/re-load 両経路）と **0x513C52**（FUN_00513BB0・純仮想）。周回対象は upsert（FUN_004040E0@0x4040EE の `lea edi,[ecx+0x10c]`）が積む単方向リスト head **0xB07BFC**、ノード走査 `mov esi,[esi+4]`@0x4A85D2。移植側は「エントリ辞書＋起動時に 1 回だけ全件へ INI get（read 側 b/i/f/s 型分岐は FUN_004A8800 の jmp table 0x4A8AE4 で既知）」で等価。

**判定: 解明 [確定]**（証拠: `v276_a_out.txt` A/B/C/D・`v276_a2_out.txt` W/V/S。機械語アンカー: `8db70c010000`@0x4A85B3（lea esi,[edi+0x10c]）・`8b4210`@0x4A85C9＋`ffd0`@0x4A85CE（★slot+0x10 仮想呼出）・`8b4220 / ffd0`@0x40E73F/0x40E747（★slot+0x20 起動呼出）・`b9f07bb000`@0x40E742・`8b4218`@0x40E74F（ゲート解除）・`8b5020/ffd2`@0x513C4A/0x513C52・.rdata `0xA40518→0x4A85A0`（vtbl+0x20）・FUN_004A85A0 の E8 callers=0・[0xB07BF4] 書込 `8888f47bb000`@0x40E717/0x513C23）

（件1 了）

---

## 件2 X-2: ノード型番号体系の全域意味（0x28/0x2D 実測済・0x14/0x19 生成者詳細） — 【確定：型書込は専ら **FUN_0058D2F0 末尾分類器 0x58D87F..0x58D90E**（単一関数・全 .text 全量走査で dword 型書込 4 点＋0xBBA 分岐内昇格 1 点のみ）。判定変数は [node+0x10]=int code（=node+4 float の int 化）と [node+0]=raw モード(0xA/0xF)。**0x14 = code∈[2001..2027]∧raw 0xA**（`mov [ebp],0x14`@**0x58D8CC**）、**0x19 = 同コード域∧raw 0xF**（`mov [ebp],0x19`@**0x58D8DA**）＝**action キーワード語彙（copy/add/sub/mult..ref の 27 語・登録バンド 0x58A1CA.. 実抽出）のノード化**。消費側は FUN_0058CF40 の `cmp eax,[esi]` 分岐群（0x14→push 0xA／0x19→push 0xF→FUN_0058CEF0 に親 tile+op int+左辺値）＋警告 4 文 'Action …outside of any trait.'。v275 §3-4 の「999→0x19 付与」は**誤帰属と確定**（999∉[2001..2027]＝template ノードは raw 0xF のままか定義経路で木から除去。機構結論は無影響）】

### 2-1. 分類器（生成者）全域 — 命令レベル [確定]（`v276_b` A/全文字）
| 最終型 | 生成 site | ガード条件 | 意味（消費側から逆算） |
|---|---|---|---|
| **0x28 (40)** | `mov [ebx],0x28` @**0x58D5FD** | 現ノードが `name`(0xBBA) attr ∧ 親 raw==0xA ∧ 親 float∈[901.0@0xA6ACB8, 907.0@0xA6ACB0] | **名前付きタイル要素**（factory 対象・逆リンク [node+0x10]=tile） |
| **0x2D (45)** | `mov [ebp],0x2d` @0x58D909 | raw 0xF ∧ float∈[901,907]（name 昇格を受けなかったタイル tag トークン） | タイル tag 系の非昇格ノード＝歩行者は生成せず**コンテキストのみ戻す**（`mov ebp,[ebp+0x10]`@0x5903AA・applier は [0xB3B0A1]=0@0x58CF97） |
| **0x14 (20)** | `mov [ebp],0x14` @**0x58D8CC** | int code∈[**0x7D1, 0x7EB**]=[2001,2027] ∧ raw 0xA | **action 要素形**（`<copy/> <add/> …`） |
| **0x19 (25)** | `mov [ebp],0x19` @**0x58D8DA** | 同上 ∧ raw 0xF | **action 属性形**（同じ語彙の別構形） |
| 0x1E (30) | 0x58D89F | code∈[4001,4097]∪≥10000 ∧ raw 0xA | 数値即値属性（消費: ebx=int([esi+4])@0x58D0CF） |
| 0x23 (35) | 0x58D8AD | 同上 ∧ raw 0xF | ゼロ operand（消費: ebx=0@0x58D0E8） |
| 0x32 (50) | 0x58D637・0x58D6DA | name attr で親不成立／value(0xBB9) attr の祖先が条件不一致 | 文字列属性ノード（name/string(4062)/filename(4070)→BSString 経路 0x58D058/0x58D014） |
| 0x37 (55) | 0x58D702 | value attr ∧ 祖先型 0x1E/0x14 ∧ float∈[4001,4097] 域内 | action に束ねる数値引数（消費 0x58D0EF→0x58B220(code,val)） |
| 0x3C (60) | 0x58D7D2 | `trait`(0xBBC) attr ∧ 親 code==`src`(0xBBB) ∧ 曽祖型==**0x14** | action の trait 参照バインド（消費 0x58D126→0x58B800 解決＋0x58CF10） |
| -1 | 0x58D70F | value attr が上記以外 | エラー＋警告 'Bad trait/action type in XML'@0xA6AC6C |
| raw 0xA/0xF | （書込なし） | 分類器非該当 | 歩行・適用の両方で無視（=注釈/未知トークン） |
- 全域 dword 型書込走査（.text 全量・再開型）: 0x14/0x19/0x28/0x2D の direct store は上表 4＋0x58D5FD のみ。レジスタ経由 store も band 0x585000..0x592000 で 0 件（`v276_b` E＋単発照会）。**分類器は FUN_0058D2F0 に集約**（E8 呼主 7 点：0x58DDC3/DE1A/DE38/DFE6/E03A/E092/E157＝tokenizer の状態 1/2 で push 0xF/0xA）。
- raw モードの極性 [推定]: tokenizer 実測では 0xF=開始/自己閉じ tag・0xA=属性側トークンでの descriptor 構築（0x58DD9A..0x58DE38 の `cmp byte[esp+0x17],0` 分岐）。0x14/0x19 とも同じ action コード域から raw だけで分岐＝**要素形/属性形の二形式説**が消費側（0x58CEF0 の引数 push 0xA/0xF の対称）と整合。

### 2-2. action 語彙表（code→名・登録バンド 0x58A1CA..0x58A4E3 実抽出 203 ペアから） [確定]
- **[2001..2027]** = copy,add,sub,mult(2004=mul 別名),div,rand,user,gt,gte,eq,lte,lt,min,max,and,or,neq,mod,trunc(2019=floor 別名),abs,**onlyif(2021),onlyifnot(2022)**,ln,log,ceil,not,ref（27 キーワード・25 スロット＋別名 2）。
- [3001..3004]=value,name,src,trait ／ [4001..4097]=可視座標・user0..26・file*・xbutton* 属性名 97 ／ [901..907,999]=タイル tag ／ [1001..1051/101..109/6008]=entity 展開名 ／ [5001..5007]=parent,me,sibling,child,screen,strings（参照語・分類器非該当＝raw のまま）。
- 実行側 FUN_0058BEE0 の switch `lea eax,[ecx-0x7d2]; cmp eax,**0x19**; jmp [eax*4+0x58C998]` ＝ **26 case（add..ref）**。ここの 0x19 は型番号でなく case 上限（誤読注意・本束実測）。

### 2-3. 消費側一覧（型フィールド [node+0] の read 全量） [確定]
- 歩行/factory: FUN_00590330 `cmp eax,0x28`@**0x590358**（→factory 0x5902A0＋逆リンク 0x59039E）／`cmp eax,0x2d`@**0x5903A5**（→0x5903AA でコンテキスト戻しのみ）。
- 適用: FUN_0058CF40 `cmp eax,0x28`@0x58CF6E（tile=[esi+0x10]・[0xB3B0A1]=1）／0x2D@0x58CF92（[0xB3B0A1]=0）／0x32@0x58CFD3／0x1E@0x58D0CF／0x23@0x58D0E3／0x37@0x58D0EF／0x3C@0x58D126／**0x14@0x58D161・0x19@0x58D175**（→`push 0xa`/`push 0xf`; `fld [esi+4]→call 0x9828C0`; call **FUN_0058CEF0**(ecx=親 tile, op int, 左辺 ebx)＝0x58B220+0x58CB70 合成）。operand 欠落時警告: 0xA6ABD0 'Action begun outside of any trait.'／0xA6ABAC 'Action ended outside of any trait.'。
- パサ内祖先ゲート: `cmp [esi],0x1e/0x14`@0x58D68F/694、`cmp [edi],0x14`@**0x58D7B6**（0x14 の上書き経路が 0x3C 生成に必要）。

### 2-4. 裁定
> X-2 残点解消。**型番号体系は「raw モード(0xA/0xF) × int code 域」の二因子で FUN_0058D2F0 末尾単一分類器が決定**：0x28=名前付きタイル要素（name attr 昇格限定）・0x2D=タイル tag 非昇格・**0x14/0x19=action（copy/add/…/ref・[2001..2027]）の要素形/属性形**・0x1E/0x23=即値・0x32/0x37/0x3C=文字列属性/数値引数/trait 参照・-1=エラー。消費は歩行 0x590330（28/2D）と適用 0x58CF40（全型）。v275 §3-4「999→0x19」を誤帰属として訂正（template は raw のままか定義経路で木から外れる＝W-3 結論には無影響）。移植側は「type=分類器写像（コード域判定 2 回＋float 範囲 1 回）＋適用側分岐表」で等価再現可。

**判定: 解明 [確定]**（型→実行意味の微部（onlyif/ref の評価意味論）は範囲外＝残点。証拠: `v276_b_out.txt` A/D/E＋単発照会出力（語彙 203 ペア・tokenizer 機構・consumer cmp 全量）。機械語アンカー: `C7 45 00 14 00 00 00`@0x58D8CC・`C7 45 00 19 00 00 00`@0x58D8DA・`C7 03 28 00 00 00`@0x58D5FD・`C7 45 00 2D 00 00 00`@0x58D909・ガード `3D D1 07 00 00`/`3D EB 07 00 00`@0x58D8B6/8BD・doubles 0xA6AC98=2001.0/0xA6AC90=2027.0/0xA6ACB8=901.0/0xA6ACB0=907.0・消費 `83 F8 14`@0x58D161/`83 F8 19`@0x58D175・switch `8D 81 2E F8 FF FF`+`83 F8 19`@0x58C411/417）

（件2 了）

---

## 件3 X-3: TileTemplateItem 関連 RTTI 文字列照合（v275 の全文字needle 0 ヒットの再確認） — 【確定：**0 ヒットは走査範囲の落ち度で、文字列は実在する**。EXE ファイル全域バイト走査で `TileTemplateItem` needle **5 ヒット＝3 本の TypeDescriptor 名文字列（すべて .data 0xB13706/0xB137D5/0xB13DB1 辺り＝TD+8 の名部）**。cache vtable_index.tsv 行 120/143/330 の**連鎖を命令外データで実検証**：TD 0xB13DA8 の name=`.?AV?$NiTList@PAVTileTemplateItem@Tile@@@@`（TD+8 実読）・COL 0xAC1CB0={0,0,0,TD=0xB13DA8,CHD=0xAC1CC4}（VS6 形）・vtable 0xA6A1DC の実 slot 3 本=[0x58A080,0x46C0D0,0x573D00]＋直後が隣接文字列データ（slot 4+ の読みは位相一致ノイズ）。一方 **クラス自体 `.?AVTileTemplateItem@@` は 0 ヒット [確定負]**（VS6 がインスタンス化テンプレートのみ TD 発生的なパターンと整合）。v273 脚注（RTTI 索引 120 番=NiTList<TileTemplateItem>）は**正しい**／v275 の「0 ヒット→cache 索引由来か要照合」は**走査範囲不足（.data 未踏）として訂正**。機構結論（W-3 の再生成型クローン）は無変更】

### 3-1. 実在 3 名と所在 [確定]（`v276_c` A/B）
| # | TD string VA | mangled 全文（TD+8 実読） | 対応 vtable / COL | 出所行 |
|---|---|---|---|---|
| 1 | 0xB13DB0 | `.?AV?$NiTList@PAVTileTemplateItem@Tile@@@@` | vt 0xA6A1DC（slot3=[0x58A080,0x46C0D0,0x573D00]）・COL 0xAC1CB0 | vtable_index.tsv 行120 = v273 脚注の索引 120 |
| 2 | 0xB136E8 | `.?AV?$NiTListBase@V?$DFALL@PAVTileTemplateItem@Tile@@@@PAVTileTemplateItem@Tile@@@@` | vt 0xA696C8・COL 0xAC14E4 | 同 行143 |
| 3 | 0xB137B0 | `.?AV?$NiTPointerListBase@V?$DFALL@PAVTileTemplateItem@Tile@@@@PAVTileTemplateItem@Tile@@@@` | vt 0xA697C4・COL 0xAC160C | 同 行330 |
- needle 生ヒット数（ファイル全域・重複サフィックス込み）: `TileTemplateItem` 5・`NiTList@PAVTileTemplateItem` 1・`.?AVTileTemplateItem`（クラス単体名）**0**。
- TD 先頭語=0xAA3E80（TypeDescriptor VFTable・全 TD 共通の CRT 値）／spare=0 — VS6 レイアウト適合 [確定]。

### 3-2. v275 との突合 [確定]
- v275 §まとめ X-3 の「全文字needle 0 ヒット」は、同文档 selftest 列の needle 走査が .rdata 文字列群（0xA28000..）中心で、**RTTI TD 名が住む .data（0xB02000..・raw バックあり）を跨いでいなかった**ためと説明できる（本束の全ファイル scan で 5 ヒットすべて file_off 0x0712306..0x07129C1＝VA 0xB137xx/0xB13Dxx＝.data）。再現時は `Oblivion.exe` 全域バイト検索を使うこと。
- 帰結：**「TileTemplateItem（RTTI 索引 120 が関連実体）」という v273 の記載は追認**（索引行は TD/COL/vtbl の実連鎖と一致）。TileTemplateItem クラス自体の TD 不在は [確定負]（名前解決は list 側 TD の内側トークン `PAVTileTemplateItem@Tile` のみ）。

### 3-3. 裁定
> X-3 解消。RTTI 文字列は**実在**（3 TD・.data 住い・連鎖検証済）。v275 の 0 ヒット判定は走査範囲の欠陥として**訂正**。機構側の結論（v275 件3：Instantiate=FUN_00585410・再生成型クローン）は不変で、クラス名 TileTemplateItem はリテラル名として正当（`Tile::TileTemplateItem`＝コンパイル時に実在、TD はテンプレート側のみ生成）。

**判定: 解明 [確定]**（証拠: `v276_c_out.txt` A/B。アンカー: file_off 0x7129B6→VA 0xB13DB6 の `NiTList@PAVTileTemplateItem`・TD[0xB13DA8]={80 3E AA 00, 0, '.?AV?$NiTList@PAVTileTemplateItem@Tile@@@@'}・COL[0xAC1CB0]=`00000000 00000000 00000000 A83DB100 C41CAC00`・vtable[0xA6A1DC]=`80A05800 D0C04600 003D5700`＋隣接文字列）

（件3 了）

---

## 件4 V267 引継①: TESNPC switch の extra 0x1F/0x20 書込と extra 0x23（旧称 ExtraPackages→**ExtraFollower**・v272 確定名）書込経路の並列性 — 【確定：**型 byte 書込窓は 3 者完全独立（ctor 内埋込 1 点ずつ）で重なり無し、ただし添付（生成）経路は同一呼び主 2 関数の中で字面上並列実行される**。0x1F=ExtraPackage（ctor `mov [eax+4],0x1f`@**0x42A1BF**・FUN_0042A1A0・0x1C obj・呼主 1 点=FUN_004268B0@0x426959）／0x20=ExtraTresPassPackage（ctor `mov [eax+4],0x20`@**0x42A1E6**・FUN_0042A1E0・0x10 obj・呼主 1 点=FUN_0041FBC0@0x41FC3E）／0x23=ExtraFollower（ctor `mov [esi+4],0x23`@**0x429B48**・FUN_00429B20・SEH・呼主 1 点=FUN_00424C50@0x424C9A）。**TESNPC ctor 自体の +4 書込は formType=0x23 の 1 点（0x52734B）のみで 0x1F/0x20 の書込は不在**（関数全域 0x527220..0x5273E0 走査で実測）＝「TESNPC switch 0x1F/0x20」の実体は TESNPC ctor でなく**package タイプ switch を持つ添付関数 FUN_004268B0**（@0x426900 `cmp byte[esi+0x20],0x13/0x11 → je skip`）と FUN_0041FBC0。並列性の核心: **FUN_00426A90 は 0x1F 添付(0x427363)・0x20 添付(0x4273C3)・0x23 follower 追加(0x4275D9)を同一関数内で呼ぶ**／FUN_004282A0 も 0x1F(0x428708)/0x20(0x428718)/0x23(0x42860D) の 3 連。共有するのは配管（GetExtraData 0x41E210・malloc 0x401F00・AddExtra 0x41E0A0）のみで、書込命令・型値・gen-window は型別に素（す）で並列＝**混線なし [確定]**】

### 4-1. 型 byte 書込窓の全量（.text 全域 `mov byte [reg+4], imm` 走査・`v276_d` D）[確定]
| imm | site | 帰属 | 判定 |
|---|---|---|---|
| 0x1F | **0x42A1BF**（FUN_0042A1A0 内） | ExtraPackage ctor | 実体（v268 型表と一致） |
| 0x20 | **0x42A1E6**（FUN_0042A1E0 内） | ExtraTresPassPackage ctor | 実体 |
| 0x20 | 0x4AE6B0 / 0x6B380E | 別クラス ctor 群 | 位相一致ノイズ（extra でない） |
| 0x23 | **0x429B48**（FUN_00429B20 内） | ExtraFollower ctor（唯一） | 実体（v272 §2.1 と一致・再取得） |
| 0x23 | 0x52734B（FUN_00527220=TESNPC ctor） | formType=NPC の位相一致 | ノイズ（本束で TESNPC ctor 内 +4 書込=この 1 点のみと再実測） |
- ExtraPackage ctor は 0x1C サイズ（new 0x1C@0x426928）・+0xC..+0x14 に 3 引数（package form 系）、+0x18/+0x19 2 バイト flag。ExtraTresPass ctor は 0x10 で payload=[+0xC] 1 ptr。ExtraFollower ctor は 0x10＋payload new 8 ヘッド {0,0}（+0xC）＝既知 3 レイアウトの相互対比完了。

### 4-2. 添付経路の並列構造（`v276_d` B 追加照会）[確定]
- **FUN_004268B0**（ExtraPackage get-or-create・ret 0x14）: 0x426900 で `[esi+0x20]`（package 側のタイプ byte）を `cmp 0x13`/`cmp 0x11` の 2 分岐 → 該当则 skip（0x4269C6）、Otherwise GetExtraData(0x1F)→new 0x1C→ctor→AddExtra。**E8 callers 22 点**（0x427363/0x428708＝NPC package 帯、0x50AF4F、0x524D6A（TESNPC 帯 0x52xxxx）、0x5F5xxx/0x60Exxx（AI/process 帯 12 点）、0x644019/0x6461B9（=v272 §2.2 の 0x23 呼び主姉妹と同関数）、0x66CE64/0x6751B7）。
- **FUN_0041FBC0**（ExtraTresPassPackage get-or-create）: callers 3 点 = **0x4273C3 / 0x428718**（上記と同関数内＝**0x1F 添付の直後に直列併設**）＋ 0x641E6E。
- **FUN_00424C50**（ExtraFollower get-or-create・v272 §2.2）: callers 6 点に再確認（0x4275D9/0x42860D/0x568D5F/0x568DFB/0x644026/0x674A08）。**FUN_00426A90（=0x427363,0x4273C3,0x4275D9 三个调用）/ FUN_004282A0（同型 3 连）で 0x1F/0x20/0x23 が同一スコープ内で並列発火**。
- 直列/並列の語義：3 extra は**別 window・別 ctor・別ゲート**（0x23 側には 0x13/0x11 タイプ switch 相当が無く、キー登録形（node{key,next}走査→0x446CB0）で独自制御）。ファイル側ローダ switch（FUN_004259E0）に 0x23 ケース不在は v272 の負証拠を維持（本束は同関数呼び出し列に 0x23 push が現れないことを再確認）。

### 4-3. 裁定
> 引継① 解消。**並列性=有（呼び主 FUN_00426A90/FUN_004282A0 レベルで 3 extra 添付が同一関数から順次呼ばれる）／書込窓の共有=無（型 byte は各自 ctor 埋込のみ、TESNPC ctor の 0x23 は formType ノイズ）**。「TESNPC switch」の正体は FUN_004268B0 の package タイプ分岐（0x13/0x11 スキップ）であり TESNPC ctor 自体は extra を一切書かない。移植側は「package 付与時に ExtraPackage(+0x1C)/ExtraTresPass(+0x10) を get-or-create、follower キーは ExtraFollower(+0x10) のランタイム辞書へ（ファイル経路なし）」を独立実装＝既存 GT（v272 §2）と矛盾なし。

**判定: 解明 [確定]**（証拠: `v276_d_out.txt` A/B/C/D＋単発照会（callers 一覧）。機械語アンカー: `C6 40 04 1F`@0x42A1BF・`C6 40 04 20`@0x42A1E6・`C6 46 04 23`@0x429B48・`8A 46 20 / 3C 13 / 3C 11`@0x426900-0D（switch）・`E8 42 38 00 00`@0x426959・`E8 9D A5 00 00`@0x41FC3E・`E8 F8 38 00 00`帯?（0x424C9A=call FUN_00429B20 は v272 記載）・TESNPC ctor 範囲 0x527220..0x5273E0 で +4 imm store 1 点＝`C6 46 04 23`@0x52734B）

（件4 了）

---

## 件5 V267 引継②: qword[0xB06148]=entry 数の意味（NiTMapBase カウンタ整合） — 【確定：**[0xB06148] は map=this 0xB0613C の +0xC dword ＝エントリ数（m_allocated 単一カウンタ）で、.text 全域に絶対アドレス参照 0 件**（needle 実走査・対照 [0xB06144]=16/[0xB0613C]=96 件で手法妥当性確認）。コードからの到達はベースレジスタ経由のみで、**書込=Insert `add dword [esi+0xc],1` @0x4525D1 と Remove の 2 経路 `add dword [esi+0xc],-1` @0x45250C/0x45255C の 3 点だけ**（±1 完全整合）。**読取窓 0 件**：Get（FUN_0055E000）もセーブ側全走査（FUN_004492E0・0x449A9A/449AB5 で [0xB06140]/[0xB06144] を直接使う）も [map+0xC] を参照せず＝カウンタは**更新専用台帳（ランタイム非消費）**。CRT 静的初期化 FUN_009DBE90（0x9DBF1D/2D/3D で +4/+8/+0 書込）は +0xC を触らない—.data 画像値 0 がそのまま初期値。一方 **「qword[0xB06148]」表記はデータダンプ由来の位相越境**：qword 読取 = 0x00A37AAC_00000000 ＝ count(0) ＋ 隣接静的オブジェクト **NiTLargeArray@0xB0614C の vtbl 語**（v274 §4.2 の dirty 配列そのもの）で、意味単位は dword 1 つ。x87/qword 幅命令の 0xB06144..0xB06150 直参照も 0（浮動/64bit store 窓なし）。**整合結論: NiTMapBase として +0xC 単一カウンタは Insert/Remove で閉じ、二重カウンタ（allocated/used）は本 retail 実装に存在しない [確定負]**】

### 5-1. 参照全量と幅 [確定]（`v276_e` A/B/C）
- imm32 needle 0xB06148（.text 全量・ファイルオフセット→VA 変換）: **0 サイト**。対照: 0xB0613C=96 サイト/19 関数、0xB06140=4/3、0xB06144=16/6（FUN_00448D40/004492E0/0044AC20/0044CDF0/0046AF70/009DBE90＝衝突 walk・セーブ走査・初期化）。
- .register-indirect 書込（map base レジスタが 0xB0613C と確定する窓のみ）:
  - `0x4525D1: 81 46 0C 01 00 00  add dword [esi+0xc],1`（FUN_00452570 Insert・slot+0x14 entry alloc→slot+0xC 構築→バケット頭差込の後）
  - `0x45250C / 0x45255C: 83 46 0C FF  add dword [esi+0xc],-1`（FUN_004524C0 Remove の 2 削除経路＝key 削除／リンク外し削除）
- 読取: FUN_00452570/0x4524C0 内の他命令・FUN_0055E000(Get)・FUN_00448D40（ID 割当衝突 walk＝[0xB06144]＋key mod [0xB06140]）に [map+0xC] read なし（0x4496D2/0x4496F0 群は別オブジェクト＝解放チェーン head ポインタ群で map でない）。

### 5-2. qword 表記の出自 [確定]
- .data 画像値（A セグ実測）: `[0xB0613C]=0xA3BC94(vtbl基底状態) / [0xB06140]=0x0002008D / [0xB06144]=0 / [0xB06148]=0 / [0xB0614C]=0xA37AAC…`。**qword[0xB06148]=0x00A37AAC00000000**＝count 下位語＋隣接 NiTLargeArray の語 — v268 §E2（TOFT 項の +8=map vtbl 共有）と同型の**境界共有アーチファクト**。Ghidra データツリーが 8 バイト単位で見た残像であり、意味的な qword エントリ数は存在しない。
- 64bit/浮動の絶対書込（movq/fst(p) で disp32∈0xB06140..0xB06154）も 0（imm needle 時点で 0xB06148 が 0 のため自動確認、rep stosd 経由は FUN_009DBF00/9DBE90 の書込列が +0/+4(+0x100)/+8 の 3 語のみで閉じる）。

### 5-3. 裁定
> 引継② 解消。**entry 数の意味＝「NiTPointerMap<uint,TESForm*> の保持数台帳（map+0xC dword）。Insert/Remove の ±1 のみで更新され、retail コードのどの検索・走査・保存経路からも読まれない」**。移植側は (a) 0xB06148 を dword 1 語として ±1 管理、(b) qword 解釈・隣接 0xB0614C（dirty NiTLargeArray）との混同を禁止、(c) 走査は [0xB06144] バケット配列＋entry{+0 next,+4 key,+8 value} で行う（count 非依存）＝既存 GT（v272 §5・v274 §4）と完全整合、新実体なし。

**判定: 解明 [確定]**（証拠: `v276_e_out.txt` A/B/C ＋ 単発照会（+0xC ops 一覧・0x4492E0 map 窓）。機械語アンカー: `81 46 0C 01 00 00`@0x4525D1・`83 46 0C FF`@0x45250C/0x45255C・`8B 3D 40 00 02 00`系 [0xB06140] read@0x449A9A/0x44CFA3/0x9DBF1D・.data `48 B0 06`=00 00 00 00 4C B0 06=AC 7A A3 00・qword 実読 0x00A37AAC00000000・絶対参照 0 件（needle 全域走査））

（件5 了）

---

## 件6 v274 §3 残点: TESSaveLoadGame +0x44/0x48・+0xB0..0x1BC 遅延書込窓の全域集計 — 【確定：**SLG は非ポリモーフィック単例（ctor が vptr を一切設置しない）ゆえメソッド集合＝`mov ecx,[0xB33B00]` 直接呼出チェーン**（tight 初期 88→不動点 **150**・needle 関数 445 と併せ work **571**）。集計結果＝**+0x44 は遅延書込 2 点実在（FUN_0045F180@0x45F1EA/0x45F2B6 の alloc 上限台帳・静的読取 0＝write-only）／+0x48 は読取 2 点実在（FormID 上位バイト境界アクセスサ 0x452180/0x4521D0）で書込 0 点＝書込側 [未確定]／+0xB0..0x1BC は 68/68 オフセット全零アクセス＝遅延でなく完全死窓 [確定]（ライブ末尾は窓外 +0x1C0 のみ W=2/R=1）**。なお `0xA39044` は ctor が `mov [edi],0xA39044`@0x45B560/0x45B5DF で埋め込む **+0x74/+0x78 NiTLargeArray<uint> メンバの vtbl**（RTTI TD 0xB057D0=`.?AV?$NiTLargeArray@I@@`・COL 0xAB7A34={0,0,0,TD}）であり、g1 の一時誤帰属（SLG 本体 vtbl 説）を棄却。v274 の「ctor 書込なし [確定負]」は全文字走査で再確認（ctor 書込集合に 0x44/0x48・0xB0..0x1BC なし・0xAC の次は 0x1C0）】

### 6-1. 方法論（波設計・`v276_g2..g9`）[確定]
- v276_f 初走の欠陥: ①v275 式 32B 窓＋`assume_ecx` 全域化で tile 帯 0x582xxx を誤招入（169 誤サイト）、②capped=True でメソッド集合未完、③call 境界で volatile レジスタ（eax/ecx/edx）が生きたまま伝播。
- 確定版走査器（g9・最終）: instruction 境界安全な `mov ecx, dword ptr [0xB33B00]`→直後の最初の `call rel32` をメソッド種（ecx 破壊前に限り push 等は無視）、**call/ret で volatile kill**、`lea dst,[slg+K]` の派生 base シフト追跡、`mov [esp+X],slgreg`→`mov reg,[esp+X]` の stash 復元、`[base+idx*scale+disp]` 全幅解析、disp∈{0x44,0x48}∪[0xB0,0x1BC] を eff 換算で集計。メソッド不動点 = 5 ラウンドで **150**（g4 無 kill 版 191/211 は上位包含＝棄却）、work **571**、最終サイト **4**（W2/R2）。
- 脱出路の負確認（全域 byte 走査）: `push dword ptr [0xB33B00]`(FF 35)=**0**、スロットアドレス即値（B8/B9/68 系）=**0**、自明 getter `mov eax,[g]; ret`(A1..C3)=**0**、work 内 rep 系=1 点のみ（0x609057・FUN_00608FC0 の **別クラス** tile オブジェクト間コピーで base が SLG でない）→ **memcpy bulk / 引数 escape の経路なし [確定]**。
- 生成窓: ctor FUN_0045B300（0x45B300..0x45B669・SEH・self-register `mov [0xb33b00],esi`@0x45B3C8・return this@0x45B653）は [esi] に vtbl を一切置かない＝**非ポリ単例**（g1b/g3 全文字）。ctor の窓内書込 = ゼロ（+0x40→+0x4C に跨ぎ、+0x44/0x48 スキップ；+0xAC の次は +0x1C0）[確定]。

### 6-2. +0x44（dword）— 書込側 [確定]／読取側 [確定ゼロ]
- 書込 2 点: `mov dword ptr [edi+0x44],0x1FFFF000`@**0x45F1EA**／`mov dword ptr [edi+0x44],0x60000000`@**0x45F2B6**（FUN_0045F180・0x45F180..0x45F2E0・edi=this@0x45F19F `mov edi,ecx`・引数 esi=TESFile 系（+4=='0'(0x30) 分岐@0x45F263・vtbl +0x60/+0x6C）。同関数は alloc ヘルパ **FUN_004535A0**（callers 5: 0x453AD0/0x45CD2C/0x45F1E3/0x463B72/0x465494）を呼び、返値へ `and eax,0x1FFFF080`@0x45F1F6／`and eax,0x60000000`@0x45F2C4 — 定数組から **+0x44＝割当上限/保護モード選択の台帳** [推定]。
- 帰属確証: 同関数が FUN_0045A500（v274 セーブスレッド同調述語）@0x45F25E と [edi+0x18] の bit0/bit18 マスク書込（0xfffffffe@0x45F285／0xfffbffff@0x45F28B）＝ v274 §3 の +0x18 意味と整合 → edi=SLG this 確定。
- 読取 0 点: work 571 全体に `[reg+0x44]` の read/ADDR なし＝**write-only 台帳**（遅延書込は実在、消費は小窗内ゼロ）。唯一の外部入口: E8 callers(FUN_0045F180)=1（0x464790・メソッド 0x464440 内 `mov ecx,ebp` this チェーン）[確定]。

### 6-3. +0x48（byte）— 読取側 [確定]／書込側 [未確定]
- 読取 2 点: `cmp al,byte ptr [ecx+0x48]`@**0x452195**（FUN_00452180・al=arg FormID>>24＝プラグイン index 境界判定＋[ecx+0x4C] テーブル走査）／`movzx esi,byte ptr [ecx+0x48]`@**0x4521E9**（FUN_004521D0・esi=count・edi=[ecx+0x4C] を 0..esi 線形スキャン）。[ecx+0x4C] は ctor 済ゼロ（0x45B365）で隣接 COL の TD **0xB057F0=`.?AV?$NiTMapBase@...BSSimpleList@PAULoadFormHeader@@@@@@`** と文脈一致 → **+0x48＝load-form（プラグイン）テーブル有効数/境界バイト [推定]**。
- 呼出チェーン [確定]: E8 callers(0x452180)={0x465D07（fun 0x465860・`mov ecx,ebp`）, 0x46BB38（fun 0x46BB20・`[ecx+0x18]` bit17 ゲート@0x46BB26＝v274 +0x18 意味と整合）}、callers(0x4521D0)={0x453A6B（fun 0x453A00・`lea eax,[eax+ebx+0x4c]`@0x453A57）。
- 書込 0 点: 不動点 work 全体＋派生 base＋stash でも `[reg+0x48]` の store ゼロ。ctor 初期化なしと合わせ **未初期化読取の欠陥候補**、または書込関数（&this+K の引数渡し跨関数派生 base＝本集計の外周）に残留 [未確定]。移植側は「+0x44/+0x48 を 8 バイトの遅延書込_pair としてゼロ初期化し、割当台帳(+0x44)とプラグイン境界バイト(+0x48)で独立管理」で安全に上位互換。

### 6-4. +0xB0..+0x1BC（272B・68 スロット）— [確定：完全死窓]
- work 571・eff 換算（base シフト/スケール索引/stash 含む）で W/R/ADDR **全 0**。ヒストグラム上、メソッド可达は +0xAC まで、次は窓外 +0x1C0（W=2 R=1・ctor ゼロ@0x45B3C0 のライブ末尾語）のみ。bulk copy 脱出路なし（6-1 負確認）。
- 裁定変換: v274 の「遅延/未使用 [確定負]（未踏）」→ 本束全域集計により **未使用（dead padding）として [確定]**。「遅延書込窓」の実体は +0x44 の 2 点のみだった。

### 6-5. 中間走査の誤検出（本束で棄却・記録）
- 0x4DBF90（初走 v276_f が `lea ecx,[ebp+0x44]`@0x4DBF95 を拾った関数）: e8 callers=2（自己 0x4DBFE7＋0x4DFB4B∈0x4DF820）で [0xB33B00] フィード無し＝**別クラス**（+0x44 に extra コンテナ埋込）の走査関数。0x4203A0/0x420340 は `push 0x40/0x3F; call 0x41E210`（GetExtraData 配管）、0x46ABA0 は `[ecx+8]|=0x800` ＋ vtbl+0x40 tail-jmp の flag setter — いずれも SLG と無関係 [確定]。
- g2 の 0x582xxx/0x580xxx 帯 169 サイト（assume-ecx 乱発）と g8 の `[eax+0x154]`×11・sh=100 の +0xC0×1（call 未 kill のレジスタ再利用）は g9 の volatile-kill 導入で全消滅 [確定棄却]。

### 6-6. 裁定
> v274 §3 残点（全域集計）解消。**+0x44/0x48 は「8 バイト未初期化ペア」ではなく、+0x44＝セーブ/ロード開通時に FUN_0045F180 が書く割当上限台帳（write-only・書込 2 点確定）、+0x48＝FormID 上位バイト境界の読取専用カウント（読 2 点確定・書込経路は静的到達域外 [未確定]）。+0xB0..0x1BC の 272 バイトは全到達コードで一度も触られず、遅延書込ではなく純粋な未使用領域**。SLG は非ポリ単例ゆえ移植側は「直接呼出のメソッド集合（150）・+0x44 write-only 台帳・+0x48 境界バイト（初期値要定義）・窓は予約ゼロ埋め（+0x1C0 からがライブ）」で等価。

**判定: 解明 [確定]＋書込側1点小残点（+0x48 writer=[未確定]）**（証拠: `v276_g3_out.txt` ctor 全文字・`v276_g9_out.txt` 最終集計（methods=150/work=571/sites=4）＋g1b/g5/g6/g8/g10/g11。機械語アンカー: `C7 47 44 00 F0 FF 1F`@0x45F1EA・`C7 47 44 00 00 00 60`@0x45F2B6・`3A 41 48`@0x452195・`0F B6 71 48`@0x4521E9・`8B F9`@0x45F19F（edi=this）・`89 35 00 3B B3 00`@0x45B3C8・`C7 07 44 90 A3 00`@0x45B560/0x45B5DF（=NiTLargeArray<uint> vtbl・TD 0xB057D0）・TD 0xB057F0（NiTMapBase<..,BSSimpleList<LoadFormHeader*>>）・負確認 needle 0 件群（FF35/B8/B9/68/A1..C3）。残点: +0x48 書込経路（跨関数派生 base 未踏）と FUN_0045F180 の alloc 定数意味（0x1FFFF000/0x60000000＝サイズ上限説 [推定]））

（件6 了）
