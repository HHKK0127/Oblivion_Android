# v341 — #114 次束W（v337 §7/v339 item1 限定残余2件＋付随）静的裁定

- 作成: 2026-11-09（L1・静的解析のみ・バックグラウンド）
- 出典: `TODO.md` #114、`tmp/re/v337_v326_next_bundle_S_ruling.md` 残点3、`tmp/re/v339_ess_next_bundle_V_ruling.md` item1 限定、`tmp/re/v338_plus_residuals_ruling.md` §3.1/§4.2、`tmp/re/s65_hc_context.md`、`tmp/re/s81_havok_context_pending.md`、`tmp/re/s69_hc92_disasm.txt`、`tmp/re/v335_v317_stage2_static.md`、`tmp/re/v336_next_bundle_U_ruling.md`、`tmp/re/v340_v329_next_bundle_P2_ruling.md`（照会のみ）
- 対象 EXE: Oblivion Steam v1.2.0416（読み取り専用・`D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`・ImageBase `0x400000`・7,898,624B）
- probe: `%TEMP%\opencode\v341_probe1.py/.out`（A:0xB3BD00画像値/B:参照census/C:RTTI全走査/D:キーワード/.E:679xxx head窓/F:caller census/G:0x45FB40窓/H:0x6747C0窓/I:vptr書込census+J:679480/679630 walk窓）、`v341_probe2.py/.out`（B13974/7A census、0x65C620 callers全数、0x65C620窓、0x589B70窓、0xB13968..84静的値）、`v341_probe3.py/.out`（IAT解決、B13970各site窓、thunkサンプル、0x9FBA90窓、0x68F970連鎖）、`v341_probe4.py/.out`（0x42BB90/0x4E4A10 head・callers数、0x981FB4、0xA24930群、vptr→TD直読）、`v341_probe5.py/.out`（0x585364生窓、登録site群head、Tile TD、0x677C70 slot走査）、`v341_probe6.py`（Tile COL/vtable、Menu/TileMenu vptr同定）。python+capstone 5.0.7+pefile、全read-only。
- 帯宣言: 679xxx/B139xx は浅層留保（head境界＋命令窓＋RTTI/vtable/文字列走査＋既存 s69/s81 系読取のみで深掘りせず）。`0x4C0000-0x4E0000` は #106 専有＝境界レベル記録のみ。v331/v326 既存裁定（0x589B70 getter・[0xB13974]/[0xB1397A]・ID空間 0x3E9..0x41B・登録窓 0x9FBA90/0xA24950 の記録自体）は再調査せず差替えない。v339 item1 の size/count I/O契約 [確定] は維持（本束は型名到達性の裁定のみ）。
- 判定語: `[確定]` / `[部分確定]` / `[未確定]` / `[確定負]`。機能記述を正式 C++ 名へ昇格しない。#82 人間/実機ゲートに踏み込む項目は待記のみ。

## 1. 判定一覧

| # | 対象 | 本束主判定 | 要旨 |
|---:|---|---|---|
| ①a | `[0xB3BD00]` の正式クラス名（シングルトン身元） | **[待記宣言クローズ]**（伴い [確定] 構造事実3・[確定負]2・[部分確定]1） | 画像 .data の 0xB3BD00 は全ゼロ・絶対参照324件すべて `mov ecx,imm`・全vtable走査で context メソッドを含むvtableは0枚 → 静的経路で正式名は到達不能。実行時 vptr（B13/#82 ゲート）待ち |
| ①b | 679xxx 内部 container 型とリスト要素の正式型名 | **[待記宣言クローズ]**（伴い [確定] 命令レベル構造事実） | 帯内 window で確認できるのは head対 {+0,+4}/node{vptr+0,atomic+4,vtbl[0](1)} の機械形状のみ。対応 inline head の独立 RTTI anchor なし（NiT/BSS 群はテンプレ引数に出現するが本 queue への紐付けなし）→ 静的到達不可 |
| ②a | メニュー名表 [0xB1397A] の唯一書手 | **[確定]**（命令レベル・正式関数名は[未確定]） | 絶対書込 0件（[確定負]）。+A フィールドを書く汎用メソッドは 0x4E4A10/0x42BB90 の2関数だけ。this=0xB13970 の全6site census により、この実体へ非ゼロ値を書く唯一の書手 = 初期化 1 回経路の **0x4E4A10 呼び出し 0x587B74/0x587B92（push 0x33）** |
| ②b | 0x65C620 の caller 全数 | **[確定]** | E8=854＋E9=11＋jcc=0 → 合計 **865**。うち #106 専有帯内 4件（0x4C9053/0x4CC711/0x4CECEF/0x4D15D8）は境界レベル記録のみ。約655件は 0xA1643A..0xA2712A の DTOR/cleanup thunk 群（stride 0x30） |
| ②c | 登録 node 構造 | **[部分確定]** | 名表本体の正式型 = `NiTArray<Tile*>` [確定]（RTTI直読）、要素 = `Tile*`（`.?AVTile@@` TD 0xB13240・vtable 0xA690E8）。書手/除去側関数の this の正体 = `Menu`/`TileMenu` dtor 族（vptr 0xA696DC/0xA6AF14 直読）。59名の登録順・文字列供給元は [未確定] のまま待記 |
| 付随 | v338 §4.2 の 0x4CBE50 上位 7 heads 正式クラス名 | **[待記維持]**（#106 協働依存・更新なし [確定]） | v335 §F1/v336 §5/v338 の既存記述（検査系/遅延初期化単体系/SEH付き参照列挙系の機能記述のみ）に更新なし。v340（#113・P2）に本対象記述 0 件を確認。深掘り禁止帯のため据え置き |

## 2. ① — 679xxx container 型名・リスト要素正式型名・[0xB3BD00] 正式身元

### 2.1 [0xB3BD00] シングルトン — 静的到達経路のクローズ（待記宣言）

新規 read-only probe 事実（`v341_probe1.py`）:

1. **[確定]** セクションマップ上 0xB3BD00 ∈ `.data`（VA 0xB02000..、Misc ≥ raw 0x30C00 の BSS 相当領域）。ファイル画像上の 0xB3BCF0..0xB3BD40 近傍は出力された非ゼロ dword ゼロ件（=画像上全ゼロ）。静的 vptr は画像に存在しない。
2. **[確定]** `.text` 内 `0xB3BD00` 絶対参照 = **324件**（s65 の 324 sites と完全一致を再確認）。形式別内訳は **324/324 が `mov ecx,0xB3BD00`（B9 形）**。`mov [abs],imm`（C7 05）/`mov [abs],reg`（89 0D）/push/lea/読取形 = **0件**。→ **[確定負]** 「絶対アドレス経由で [0xB3BD00]（＋0x00 の vptr）へ値を書くコードがある」説。
3. **[確定]** RTTI 全走査（`.text` 以外全セクション、`vt[-4]→COL(sig==0)→TD(.?AV)` 条件で **1700枚** の candidate vtable を列挙）: s65 §6/§3.3 の既知 context メソッド集合（0x673A50/0x674500/0x674550/0x6747C0/0x674A20/0x674E10/0x675310/0x676E40/0x677360/0x678D30/0x678E70/0x679340/0x679480/0x679630/0x679A70/0x67ACA0/0x67AE90/0x67A8C0/0x67B320/0x6765F0/0x676EE0/0x6773B0/0x673A90）をスロットに含む vtable は **0枚**。→ context の 88 関数は全て非仮想（s65 特記3「direct call vs vtable call」の再確認）。クラスが仮想関数集合を画像に持たない以上、COL→TD による正式名復元経路は存在しない。
4. **[確定]** probe1-I の「`mov ecx,CTX` 直後 callee が `[reg]=vtable imm` を書く」census（91 distinct callee・0x140B窓）の唯一ヒット（0x68F970）は**窓超過の偽陽性**であり、以下 5. で差し替える（自己訂正）。
5. **[確定]** 精査（probe3-c）: `0x68F970 = ret 4`（24B 空スタブ）、隣接 `0x68F980 = mov [ecx],0xA752F0; ret`、`0x68F990` は `test byte[esp+4],1 → [esi]=0xA752F0 → (delete時)free(0x401F20)` の deleting-dtor テンプレ。0xA752F0 の COL=0xAC47E8・TD=0xB15A70=`'.?AVhkEntityActivationListener@@'`。0x68F970 の E8 caller = **83件**（this 多様: 0x42FA31 ecx=[esi+0xC]、0x442281 ecx=vtbl結果、0x49857C ecx=[TES系+0xA94]、0x506967 ecx=0xB3BD00 …）→ 0x68F970 は多数の vtable で共有される汎用 no-op 実装。**hkEntityActivationListener の vtable（0xA752F0）スロット6/9/10/14-17 と同じ値だが、`NiTArray<Tile*>` vtable（0xA69680）slot[6]/[14] も 0x68F970**（probe6）。→ **[確定負]** 「0x506967 の `mov ecx,CTX; call 0x68F970` は CTX が hkEntityActivationListener 型である証拠」（共有スタブに帰属力なし。この Weak 支持は棄却）。
6. ラベル衝突の棚卸し（既存成果物の照会のみ）: b10/handover=`CombatEventHandler/ScriptEventDispatcher`、`re_crime`=犯罪記録、s50_slp_c5=UI/ゲームマネージャ、s65/s81=HavokContext、N2（`TODO_re49.md` 92行目）=「HavokContext 0xB3BD00 の実行時 vtable 確定 — B13 実機 .data ダンプ待ち」。**いずれも非正典の機能暫定名**であり、本束の新規静的証拠でどれかを正典昇格させる根拠はない。
7. 裁定: 正式クラス名の静的復元経路（画像 vptr／絶対書込／vtable 走査／共有 no-op）は全て閉じた。残る経路は実行時ダンプのみ = **#82/B13 人間・実機ゲート**。→ **[待記宣言クローズ]**。必要証拠: 起動後 `.data` の 0xB3BD00 先頭 dword（vptr）1 語 + その COL→TD。

### 2.2 679xxx container 型・要素の正式型名

帯内命令窓＋保存済み s69/v339 頭部（`s69_hc92_disasm.txt` #79/#80）＋walk 窓（probe1-J）からの機械的事実:

- **[確定]** head 対形状: `0x679480` の `esi=this+0x40`、`[esi]`/`[esi+4]` の 2 ポインタで空判定（`cmp [esi+4],0` → `cmp [esi],0`）。`0x679630` は `edi=this` を退避後 `mov ecx,[0xB33B00]`（SaveGame）→ `0x45B9A0(2, out)` を経由し、`add edi,0x40` で同じ qA head を walk。`0x679340` は `esi=this+0x48`（qB）で同形。`0x674E10`（ctx+0x58 removeFromPending）は `add ecx,0x58; jmp 0x65C620`（probe3）。→ ctx+0x40/+0x44/+0x48/+0x4C/+0x58/+0x60 群は「{start,end} 対＋単方向ノード」の同族リスト。
- **[確定]** walk 窓 0x6794D9-0x67952C / 0x67969B-0x6796D6 の要素処理: `lea eax,[ebx+4]; push eax; call dword[0xA2807C]`（IAT 解決 = **KERNEL32!InterlockedDecrement**・0xA2807C→[this+4]）でゼロ到達時のみ `mov edx,[ebx]; mov eax,[edx]; push 1; mov ecx,ebx; call eax`（= 要素先頭 vptr の slot[0] を delete-flag=1 で呼ぶ = deleting-dtor 形）。要素は {vptr@+0, atomic counter@+4} の多形オブジェクト。イテレータは `0x677C70(ecx=head対)` 経由（0x677C70 をスロットに持つ vtable は全画像に 0 枚＝非仮想ヘルパ）。
- **[部分確定]** walk 窓に解放パターン列が観察されることは、v339/v338 の [確定] I/O 契約（size=文字列長合計・u16 count）の再裁定を意味しない（本体全文の再走査は 679xxx 浅層留保越え。窓の区分のみ記録し、v339 item1 [確定] をそのまま正典として維持）。
- 正式型名の裁定: 上記 {start,end}+node{vptr,atomic} のテンプレ/クラス名を画像から結びつける RTTI anchor は存在しない（qA/qB は 0xB3BD00 オブジェクトの inline フィールドで、それ自体に vptr が画像上なく（2.1-1.）、0xB3BD00 クラス名の復元が 2.1 で閉塞したため派生命名も不能）。`BSSimpleList`/`NiTListBase`/`NiTPointerList` 等は RTTI 上に存在するが SaveGame 変更マップ（0xA38Fxx/0xA3A2xx 群）や AStar（0xA748xx）等の **別インスタンスのテンプレ引数・vtable としてのみ**出現し、679xxx の queue head と接続する命令列証拠は帯内窓にない。
- 裁定: container 型と要素の正式型名 = **[待記宣言クローズ]**（指定浅層帯＋RTTI/vtable/文字列走査＋s69/s81 既存読取の範囲で静的到達不可）。解消に要する証拠: (a) 実行時 0xB3BD00 の vptr→COL→TD（2.1 と共通ゲート）、または (b) push 側（0x6765F0/0x678D30 等、679xxx 帯外・本束宣言範囲外）の new サイズ＋vptr 書込の帯外深層照会（別起票扱い）。

### 2.3 caller census（参考・[確定]）

E8 全走査（probe1-F 修正版、`.text` 先頭 VA=0x401000 基準）: `0x679480` callers=1（0x45FB5A・v339/v338 と一致確認）、`0x679630`=1（0x45FBE6）、`0x679340`=2（0x5EE5B6/0x6005AC）、`0x679A70`=2（0x5D69B2/0x66FD0F）、`0x679850`=1（0x45FD4B）。呼び側窓（probe1-G）で `0x45FB55: mov ecx,0xB3BD00 → 0x45FB5A: call 0x679480` を直接視測（=「SaveGame 側が 0xB3BD00 の qA を尺寸取得に使う」契約の命令窓確認。s65/s81 の ctx ラベルと v339 の I/O 契約は同一オブジェクトの別側面であり矛盾ない）。`0x45FB66` の `shr ecx,9; test cl,1`（=dry bit9 消費）も同窓で再確認（v337 §4.2 の語彙どおり消費側のみ。発火は #82 待記維持）。

## 3. ② — メニュー名表 [0xB1397A] 唯一書手・0x65C620 callers 全数・登録 node 構造

### 3.1 B13974/B1397A/B13970 参照 census（[確定]）

`.text` 絶対参照（probe2-A/B・probe3-a）:

| 対象 VA | 件数 | 内訳 |
|---|---:|---|
| `0xB1397A` | 5 | 読のみ: `cmp word[abs],6`@0x57F358、`cmp word[abs],ebx`@0x58369F/0x5836DE、getter `movzx ecx,word[abs]`@0x589B7F、`cmp word[abs],0x1A`@0x5ACB40。**書込形（`66 89 0D`/`66 C7 05`/`FF 0D`）0件** |
| `0xB13974` | 6 | 読のみ（`mov eax,[abs]` 系＋`push`）。書込 0件 |
| `0xB13970` | 6 | this 回付の `mov ecx,0xB13970` ×5（0x585369、0x587B6F、0x587B7F、0x591BC9、0x591D1B）＋ 0xA24916 の `mov dword[0xB13970],0xA69680`（DTOR thunk の vptr 復元・データ書込） |
| `0xB13978/7C/80/84/88` | — | 0xA24910/0x9FBA90 群からの参照（3.3 で処理）。getter 上限 `cmp ecx,0x32` は 0x589B7A で再確認（v331 裁定差替えなし） |

→ **[確定負]** 「[0xB1397A] を絶対アドレスで書く唯一の書手がある」。同フィールドは **this+0xA の u16** であり、書込はメソッド経由のみ。

### 3.2 名表本体の正式型 — `NiTArray<Tile*>`（[確定]・RTTI 直読）

- **[確定]** 画像静的値: `[0xB13970]=0xA69680`（vptr・DTOR 0xA24916 も同じ値を復元書込）。0xA69680-4 = COL 0xAC140C（sig==0）→ TD 0xB13620 = `'.?AV?$NiTArray@PAVTile@@@@'`。**メニュー名表本体の正式コンテナ型名 = `NiTArray<Tile*>`**（ID 空間 [0xB13974]=this+0x4 data、[0xB1397A]=this+0xA の u16 境界値、this+0x8/+0xC に u16 フィールド対、0xB1397C=0x00010000/0xB13980=0x23/0xB13984=0x32 静的値はオブジェクト後部フィールド）。
- **[確定]** 要素型 anchor: `.?AVTile@@` TD=0xB13240・COL=0xAC132C・vtable=0xA690E8。Tile TD への `.text` imm32 参照は **59件**（0x586C6A..0x5DD544 群＝メニュー XML/ウィジェット code 域の RTTI 使用）。
- **[部分確定]** 要素の実体は `Tile` 派生系。除去 site 所在関数がデストラクタ族である証拠: `Menu::~Menu` 窓（0x585330 head、0x58533D `mov [esi],0xA696DC`＝TD 直読 `'.?AVMenu@@'`）、`TileMenu` dtor 窓（0x591B70/0x591C50、0x591B9A `mov [esi],0xA6AF14`＝TD 直読 `'.?AVTileMenu@@'`）。

### 3.3 [0xB1397A] の唯一書手（[確定]・命令レベル）

this+0xA を書くコードは 2 関数に限定（probe4）:

- `0x42BB90`（汎用 insert/remove・引数 `(this, idx, ptr-to-item)`・E8 callers **992**・`ret 8`）: grow 分岐 0x42BB9C-9F `mov word[ecx+0xA], idx+1`、上書き分岐は +0xA 不変（+0xC を ±1）、null-item 分岐は +0xA 書かない。
- `0x4E4A10`（汎用 resize・引数 `(this, n)`・E8 callers **1043**）: 0x4E4A62 `mov word[esi+0xA], arg`。
- this=0xB13970 の全site（3.1 census、6 site）に限定すると:
  - **0x587B6F/0x587B7F: `push 0x33; mov ecx,0xB13970; call 0x4E4A10` ×2（同一 init 関数の分岐両側・`[0xB3A708]` の one-shot ガード下）→ [0xB1397A]=0x33(51) を書く唯一の書手**。 **[確定]**
  - 0x585369/0x585372（`Menu::~Menu` 族 head 0x585330 内・`sub ecx,0x3E9` 後 `call 0x42BB90`＝raw hex 整合で 0x42BB90 と復元）、0x591BCE/0x591D24（`TileMenu` dtor 族内）= 除去経路（grow 分岐は容量 51 確定後 idx≤0x32 で構造的に不発）。
  - 0xA24916 = DTOR thunk の vptr 復元書込（+0xA ではない）: `mov eax,[0xB13974]; push eax; mov [0xB13970],0xA69680; call 0x401F20`（0xA2490D-26）＝終了時（life-end）清掃。
- 0xB13970 への other-reference（0xA24930 群）: 兄弟グローバル配列 `0xB13948`(vptr 0xA69688=`NiTMapBase<NiTPointerAllocator<I>, const char*, XMLStorage*>`)・`0xB1395C`(0xA696A8) の DTOR jmp thunk — 名表とは別オブジェクト（混同禁止）。
- 登録窓の性質訂正（v337 §5.2 の「候補」に対する狭め。v331 の記録差替えはしない）: `0x9FBA90` 窓 = `push 0xB13984; mov ecx,0xB07BF0; call 0x4040E0`（this+0x10C の list 追加）＋ `push 0xA24950; call 0x981FB4`（=0x981F78 経由の **CRT `_onexit` 登録**）。`0xA24950` = `_onexit` キールールThunk: `0x65C620(0xB07CFC, 0xB13984)` ＋ `byte[0xB13988]==0x53` 条件 free。→ **[部分確定]** 0x9FBA90/0xA24950 は「59名の実行時登録窓」ではなく**終了時クリーンアップ登録窓**。真の登録経路（非null insert）は 3.1/3.3 の census 上に存在せず、文字列供給元・59名の最終順・slot 充填元は **[未確定→待記]**（B139xx 浅層留保＋#106 帯外 XML code 域に依存）。
- docs 不整合 1 件（記載はコードに追従）: `docs/RE_EXE_REPORT.md` §3.1 の「59メニュー」は、コード側の名表容量 `0x33=51`（ID 空間 0x3E9..0x41B＝51・getter guard 0x32）と数値が一致しない。v337 の「docs と一致」記述に対し、**件数 59 は docs 側の数**（ID 空間由来の 51 と別基準）として記録。

### 3.4 0x65C620 — callers 全数と node 構造

- **[確定]** 全 census（probe2-C、E8/E9/0F84-85 の 3 形＋`.text` 全域）: **E8=854 / E9=11（0x403C26,0x42AFEA,0x4416E6,0x453962,0x46FEF3,0x607113,0x6349C6,0x651EE6,0x663FB1,0x674853,0x674E2D）/ jcc=0 → 865 件**。内訳: `.text` 本帯（0x414BE2..0x6A25A5）≈210 件＋末尾 thunk 域 0xA1643A..0xA2712A ≈655 件（stride 0x30 の DTOR/atexit cleanup thunk 群）。#106 専有帯内 site は **4件のみ**（0x4C9053/0x4CC711/0x4CECEF/0x4D15D8）＝境界レベル記録のみ・帯内解析なし。
- **[確定]** 0x65C620 本体（0x65C620-0x65C68E・全文窓）の機械的意味: 単方向リスト `this={+0 firstノード,+4 last}` の**除去 by value**（先頭走査 `[eax]==arg` → 見つけたノードを link splice → `0x401F20`(free) で解放）。末尾一致/中間一致/未達 ret 4 の 3 分岐。**「linked-list 追加 helper」（v337 §5.2）の語は機構的に不正確** — s65 §1.2（0x674E10 removeFromPending の tail jmp）と整合する除去/解放 helper として訂正記録（v337 の裁定自体は「候補」記述であり置換は行わない。次期統合時の注記推奨）。
- **[部分確定]** node/要素の構造二系統: (a) 0x65C620 がさばく汎用ノード = `{+0 payload, +4 next}`（s65 §1.3 DispatchQueue `{tag,next,prev}` の単方向側面と整合）；(b) 679xxx queue 要素 = `{vptr@+0, atomic@+4, vtbl[0](1)=deleting-dtor}`（2.2）。名表の要素 = `Tile*`（3.2）。正式テンプレ名（BS/NiT シリーズのどれか）は 2.2 と同じ理由で **[未確定→待記]**。

## 4. 付随 — 0x4CBE50 上位 7 heads 正式クラス名（#106 協働依存・照会のみ）

- v335（`v335_v317_stage2_static.md` §S1・証拠 v335_3.out §F1）: 7 call site の上位 4 heads = `0x4B86C0`（0x4D6670 経由 cell 検査系）/`0x4F2270`（[0xB360C0..C8] 遅延初期化単体系）/`0x515D20`/`0x515EF0`（SEH 枠の参照列挙系）＋ 0x4CC045=0x4CBE50 自己再入。各 head 24 命令＋文字列の浅層照会のみ。
- v336 §5: 正式クラス名・上位 caller 名は「帯域制約上未確定」と明記（#106 専有）。v338 §4.2: [部分確定] 維持。
- v340（#113・P2・稼働中成果物）: 本束対象（4CBE50/4B86C0/4F2270/515D20/515EF0/65C620/B1397x/B3BD00/679xxx）の記述 **0 件**（grep 確認）→ 既存記述の更新なし。
- 裁定: **[待記維持]**（正式クラス名は #106 協働・帯解除待ち。本束は照会のみで深掘りせず — 宣言遵守）。

## 5. 集計・待記宣言・次証拠

主判定: `[確定]` 3（②a 書手・②b callers・付随の「更新なし」確認）/ `[部分確定]` 2（②c node・0x9FBA90 性質）/ `[待記宣言クローズ]` 2（①a・①b）/ `[確定負]` 主従合算 3（絶対書込説・画像 vptr 復元説・no-op stub 帰属説）。

待記（#82 人間/実機ゲート・他専有帯依存）:

| 待項目 | 必要証拠 |
|---|---|
| [0xB3BD00] 正式クラス名 | 実行時 .data ダンプ（B13 系）— vptr 1 語→COL→TD |
| 679xxx container/要素正式型名 | 同上＋(または) push 側 0x6765F0/0x678D30 の帯外深層照会（別起票） |
| 59名登録順・文字列供給元・slot 充填元 | B139xx 留保解除＋XML code 域（0x586-0x5DD・Tile TD 参照 59 site 群）の追跡 |
| 0x4E4A10/0x42BB90/0x65C620 の正式関数名・0xB13970 インスタンス宣言元 | 帯外浅層＋RTTI テンプレ引数対応の別レーン裁定 |
| 0x4CBE50 上位 7 heads 正式クラス名 | #106 帯解除（協働依存） |

## 6. 成果物・変更範囲

- 成果物: `tmp/re/v341_v337_v339_next_bundle_W_ruling.md`（本ファイルのみ新規・上書きなし）。
- 中間 probe（リポジトリ外）: `%TEMP%\opencode\v341_probe1.py/.out(v341_probe1b/1c 含む)`、`v341_probe2.py/.out`、`v341_probe3.py/.out`、`v341_probe4.py/.out`、`v341_probe5.py/.out`、`v341_probe6.py`（実行出力は会話記録）。
- 自己訂正 2 件（本束内・透明化）: probe1-I の vptr 書込ヒットは窓超過偽陽性（2.1-4/5）。probe1-F 初出の caller 列挙は `.text` 先頭 VA 基準 off-by-0x1000 を修正して再実行（2.3 が正）。
- オリジナル EXE/.ess/解析原本/移植側ソース/docs/STATE.md/TODO.md/.github/memory/MCP 台帳は変更していない（読み取りのみ）。v339 item1 の [確定] 契約・v331/v326 の既存裁定は差替えていない（v337 の「追加 helper」語・「登録窓候補」は上記 3.3/3.4 のとおり訂正注記として扱う）。
