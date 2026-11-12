# v346 — 次束AA（#119）: v344 §6＋v345 §4 静的残件 5 件の裁定＋レビュー（自己 probe 欠陥 2 の顕在化）

- 作成: 2026-11-09 / L1 report-only / 静的のみ（ReverseEngineering レーン＝オリジナルデータ解析／main=ゲーム作製 の分担 2026-11-09 承認）
- 出典: #119（N1/N2/N3=v344 §6、N4/N5=v345 §4 加刷）
- 対象 EXE: Oblivion Steam v1.2.0416（ImageBase 0x400000・読取のみ）
- probe: `v119_1`（N1+N3a 初期版＝欠陥あり・後記 C-10b）・`v119_1b`（N3a 修正再測＋N1 文脈窓）・`v119_2`（N2/N4/N5）・`v119_3`（N5 IAT 名直読）— 全 EXIT=0・`%TEMP%\opencode\`
- 帯宣言: 深= 0x468A40/0x5239C0/0x4D7C50（#106 帯・v319 帯隣接は v325/v344 の census 対象再視測レベル）・0x4FBDB0/0xA49DA4・0x6AB570 台 heads 命令窓・0x446CB0/0x6BB260・機械= callers 境界検証 census（v117_9 器）＋zone dword マップ。#116（BG・active）追跡帯（0x57xxxx/0x586-0x5DD/0x676xxx）と非交差（N2b の 0x6AE720→0x66BFA0 は 0x66xxxx で交差なし確認済）。
- 判定語: 5 段階・旧結論は削除せず §5 訂正節で併記

## 1. 五件裁定表

| # | 残件 | 裁定 | 根拠 |
|---|---|---|---|
| **N1** | thunk 先 callee 3 の al 契約（0x468A40/0x5239C0/0x4D7C50） | **[確定（命令・ゼロ経路列挙）]**（残=call-kills 経路の al 意味→#82/深追い任意） | **fn 本体を最初の int3 で切った正準窓で**: **0x468A40**（本体 0x468A40..0x468FEE・rets 多数）の直接ゼロ返却は **1 経路のみ** = `xor al,al`@0x468C72→`pop ebx; ret 4`@0x468C75、条件=**8 エントリースキャン尽局**（直前 `add esi,1; cmp esi,8; jl` ループ・別経路は `mov al,1`@0x468C7A）。`0x5239C0` は本体（..0x523D5C）内 **直接ゼロ返却なし**（体 0x523BEF/C30/E41/4296 の xor eax,eax は全て call-kills 先行＝al は後続 callee/scratch 由来）。**0x4D7C50**（本体 ..0x4D7FBA）= **1 経路** = `xor al,al`@0x4D7FB7→ret@0x4D7FBA、条件= `call edx`（**vtable+0x170 仮想呼び**・`mov edx,[eax+0x170]`）の返 object `[[obj]+4] != 0x23` ＝ **「type 35 (NPC_) でない」で偽**（成功側 al=1@0x4D7FB3）→ 0x4D7C50 は** predicate ヘルパ（返却オブジェクトの record 型が NPC_ か）** と語義確定。0x51C710 の call edx 系 = `mov edx,[eax+0x50]`＋`push 0x10`（フラグ）仮想 slot+0x50 呼び（0x51C840/0x51C852 連番 2 呼出・[esi+0x24] 群 object 経由）。**v119_1 が列挙した 0x469007/0x46906D/0x4691B4/0x523D75/0x4D8472 は int3 越えの隣接リーフ関数のゼロ**（型テスト系 `cmp/jne/mov al,1/xor al,al` 形）＝本 callee の契約に不算（§5 C-10a） |
| **N2** | 0x6B6B90 owner＋0x4FBB60 の slot 実先候補 | **[部分確定]**（Script 経路は [確定]） | **a) Script 経路クローズ**: head 0x4FBDB0 は **vftable 0xA49DA4 の slot idx 27**（zone ref 0xA49E10＝base+0x6C・COL→TD '.?AVScript@@'）。`this=Script` のとき 0x4FBC4C の実先 = **[0xA49DA4+0xD4] = 0x4129A0** = `mov eax,0xa2f7ec; ret`（**定数グローバル 0xA2F7EC 返却 getter**）[確定（当該 this 経路）]。**b) 0x6B6B90 owner 未決**: callers heads 4 の内 0x6AB570/0x6AB720/0x6AED70 は **E8 callers=0 かつ zone refs=0**（0x6AB570=head `cmp byte[0xb16178],0` ゲート・0x6AED70=capstone デコード空=窓要修正）、0x6AE720 は callers=1（site 0x66BFA0・ecx=esi・**0x66xxxx 帯**）→ jmp/E9 混在かデータ経由取得かの上 layer 追跡が残件（→#120 N2'） |
| **N3** | 過去 census の境界遡及（GST 166・callers(0x45F2E0)=8） | **[確定（機械・遡及済）** | GST（`ff 15 8c 80 a2 00`）生 166 → **境界検証 166/166 生存**（dropped 0・no-head 0・sanity 0x465AD5=生存 ✓）＝v340 G3 の 166 は完全堅牢。callers(0x45F2E0) 生 8 → **8/8 生存**（0x453495/0x45F6A2/0x45F6B4（自 body 再帰 2）/0x46110A/0x46409E/0x46437F/0x4651DE/0x465923＝v340 mode 表 site と 1:1 一致）。→ v340/v342 の E8 系 census に FF6A1C 類の汚染なし（tier A と併せ census 系列の信頼性は確立）。なお v119_1 初測の「0/166」は**自 probe の site VA off-by-one 誤誤**（`i += 1` 後に `tva+i` を採用）＝v119_1b で修正・§5 C-10b |
| **N4** | 0x65C620 本体帯 217 のマネージャ型帰属 | **[部分確定（列挙）]**（命名残→#120） | raw 217 サイト → owner heads **160 distinct**（v119_2 列挙）。**全 head が zone imm32 ref=0**＝仮想メソッドでなく自由関数形（TD 直結不能）。高密度 owner: 0x449FC0（12 サイト・`lea ecx,[esi+0x14/0x1c/0x24/0x2c/0x8c/0x94/0x9c/0xa4…]`）・**0x449E20（10・+0x34..0x5c 8 刻み）**・0x449D40（1・+0x84）= 0x449xxx 台の**複数 head 集合**が同一オブジェクトの +0x4..0xA4 幅の組込み list head 群を順次除去（dtor/teardown 形）→ マネージャ object の特定（head の E9/jmp 遡上＋静的 this グローバル解決）が残件。#116⑧（59 名登録元）とは別系統（本項は**除去側**・共有なし） |
| **N5** | 0x6BB260 / 0x446CB0 identity | **[確定（identity）]** | **0x6BB260 = DirectInput8Create の IAT jmp スタブ**（`jmp dword[0xa2802c]`→hint/name 直読 'DirectInput8Create'・隣 `jmp dword[0xa28034]`=[0xA28034] RVA 0x8000000B=**ordinal #11 インポート**）。よって v345 の「0x4040E0 のノード割当候補 0x6BB260」は **v118_2 の窓が ret を越えて隣接 fn を含んだ bleed**（§5 C-10c）。**0x446CB0 = 汎用 8B ノード割当 wrapper**（`push 8; call operator_new(0x401f00)`・失敗時 `call 0x401f20`・E8 callers **347**）＝0x4040E0 リスト追加の node 生成 callee の本命（[部分確定]・呼び 347 の共有 wrapper）。0x98262D=CRT 隣接（低価値・未追及） |

**集計**: N1 [確定(命令)]／N3 [確定(機械)]／N5 [確定(identity)]／N2・N4 [部分確定]（残 2 は #120 起票）。主観点 [確定負] なし。

## 2. probe 一覧（再現）

v119_1（初期・C-10a/b 顕在化）→ v119_1b（**N3a 修正 166/166**＋ callee 窓 int3 切り＋ゼロ 6 サイト文脈）→ v119_2（N2a base 逆走査 idx 算出・N2b owners zone/E8/ecx・N4 owners 160 集計＋lea offset 抽出・N5 head/callees/strings）→ v119_3（IAT hint/name 直読 2 slot）。教訓の再適用: ①calle census の site VA は `tva+i`（find 返り値）で取る ②callee 本体窓は最初の int3 で終端（v318 head ルールと併用）③「窓 callee multiset」は ret 終端必須（v118_2 の 0x6BB260 bleed）。

## 3. 帯遵守の自己宣言

深解析窓: 0x468A40/0x5239C0/0x4D7C50（int3 終端）／0x4129A0・0x446CB0・0x6BB260（head+数十命令）／0x4FBDB0（既存窓の base 逆走査のみ・Script vftable データ読）／0x6AB570/0x6AB720/0x6AE720/0x6AED70 head 6 命令。機械走査: callers/zone/GST regex×境界検証・0x65C620 owners 160（命令窓は lea/push 4 命令のみ）。未踏宣言: 0x66xxxx 本体・0x449xxx teardown 群の内部意味・Script vftable の 27 以外 slot・#116⑦⑧ 帯・#82 動的（残件として §5 外＝#120/#82）。

## 4. 残件（→ #120 次束 BB・v347+ 起票）

1. N2': 0x6B6B90 owner（0x6AE720→call site 0x66BFA0 の一段遡上＋E8 callers=0 の 3 head の jmp/E9/データ経由取得元 census・0xB16178 ゲート意味）。
2. N4': 160 owner heads のマネージャ object 帰属（head の this=esi 起源＋静的グローバル anchor・0x449xxx 台優先）。
3. （低）0x98262D／0x52B0C0・0x53142C（this=0xB36530）の 0x4FBB60 残 2 経路。
4. （任意・文書）call-kills al 意味の詳細（実行時観測があれば #82 側でクローズ）。

## 5. 訂正節（旧記載併記・非削除）

- **C-10a（本束 v119_1 self）**: callee ゼロ経路の初測リスト（8+5+2 件）は int3 終端なしの窓溢れで**隣接リーフ fn のゼロを含んでいた**。正典は §1-N1（0x468A40=1・0x5239C0=0・0x4D7C50=1、条件付）。v344 §1-① の「callee 側にゼロ形あり（0x468A40 xor al,al@0x468C72 他3）」の「他3」は本訂正で**当該 callee の契約外**に更新（列挙自体は実在の隣接 fn・誤削除なし）。
- **C-10b（本束）**: v119_1 の N3a「0/166」は probe の site VA off-by-one 誤実装（偽陰性）。v119_1b が正（166/166 生存）。
- **C-10c（v345 随伴）**: 「0x4040E0 のノード割当候補 0x6BB260」は窓 bleed 帰属（実体=DirectInput8Create IAT スタブ）。ノード割当候補は **0x446CB0**（new(8)・callers 347）に差し替え（v345 §1 付随行・§4-2 は本節で上書き併記）。ハンドオフ索引 §3 も同時更新。
- 遡及不問: v345 §1-0x42BB90/0x4E4A10 の callers（992/1043）は境界検証併走の実測で汚染なし ✓（v345 の named-5 精緻化は row322 反映済）。

## 6. クロージング

ai-todo: #119 note→complete・**#120 起票（v347+）**。台帳 row323（V323）・bak173・round_verify。STATE (bb)。memory/supermemory（不通時は滞留）。ハンドオフ索引 main_handoff_analysis_index.md §3/§5 更新（0x4040E0=リスト追加・node 割当 0x446CB0・0x6BB260=DI thunk・[Script+0xD4]=0x4129A0・0x4D7C50=NPC_ 述語）。
