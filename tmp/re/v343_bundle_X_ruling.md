# RE_ADJUDICATION — v343 次束X（#116）: v340 §4 残件6 ＋ v341 §6 待記2 の静的裁定
- 作成: 2026-11-09（BG 自走解析レーン・L1 レポートのみ・バックグラウンド静的解析）
- 正本: `C:\Users\hiroki.kogarumai\Oblivion_Android`（読み取り専用遵守: docs/Oblivion.exe 系・docs/*.md・ESM/.ess・既存 tmp/re/*.md すべて無編集。**本文件のみ新規作成**）
- 検体: `re_util.EXE` = `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（ImageBase 0x400000・読み取り専用）。親指示の `docs/Oblivion.exe` は現正本に**実在せず**（docs に .exe なし・grep/ls 確認）→ 既存全 probe と同一検体（re_util.EXE）で裁定。docs 不整合として記録。
- Ghidra MCP: `execute` 経由 `list_instances()` → **instances=[]（稼働実体なし）** → python（pefile 2024.8.26 + capstone 5.0.7）PE 直接解析にフォールバック（re_util/v340_lib 準拠・v343_lib 自作・全 read-only）。
- 判定語彙: [確定]/[部分確定]/[未確定]/[確定負]。ランタイムゲート事項は §10 #82 待記へ（確定試行なし）。
- 教訓継承: C-1（type37≠容量語）・C-2（GetTickCount 確定負・再審議なし）、C-3（reader=登録表と stream 別系統）、C-4（mode 引数は引数契約完結でのみ語れる）、find_pattern 先頭20B／ゼロ census は不在証拠にしない（窓宣言＋再帰り）、RTTI TD=.data 0xB02000+、v344 教訓（probe 窓 0x48／JPT stub=最初 call new／vptr 書 ecx-map 分離）を §6/§7 で適用。
- 帯: 0x4C0000-0x4E0000＝#106 閉済で自前可（境界宣言: 0x4D3xxx < 0x4D5F20・0x4D55E0=当該 vftable slot+0x10 まで窓厳守）、0x57xxxx＝#105 閉済、0x45/0x46/0x44 帯＝#103/#104 系自帯、679xxx/0x764xxx/B139xx/0x9Fxxxx 帯外照会＝⑦⑧の親許可手段（head 境界＋命令窓のみ）。v344（#117）所管 topics（0x51EAF0/0xA320A4/vtbl+0x1C tier/E9-EB tierB/0x54-vtbl+0x40）とは非交差。

## §0 総括表（①〜⑧ × 判定 × 証跡）

| # | 対象 | 判定 | 主要数字・証跡（probe） |
|---|---|---|---|
| ① | TESObjectCELL vftable `0xA469D4` slot+0x6C の実間接 call site 全列挙＋vtable store 帰属 | 直接形=`call [reg+0x6C]` 全 .text **0件**（窓宣言・対照 FF/2 disp8∈0x60-70=518件で形自体は現存）→ 二段形 `mov r,[b+0x6c]`→`call r` **68件**（641 mov 候補中・68/68=VTBL-DISPATCH）[確定]。cell 限定サブセットは [部分確定]。store 2件帰属 [確定] | vftable 境界复唱: [VT-4]=0xABCCBC→TD 0xB03038 `'.?AVTESObjectCELL@@'`・slot idx27=0x4CD960。store: 0x4D325E`mov[esi],VT`→head **0x4D3210**（is_head✓・callers=**7**）／0x4D32ED→head **0x4D32C0**（callers=**1**=0x4D55E3∈**0x4D55E0=vftable slot+0x10**）。CELLZONE 二段 site: 0x4CBFD1/0x4CD993/0x4D43CE＋LoadGame 内 0x4665F7（P1/P1b/P1c/P1d/P1e） |
| ② | FUN_00579CF0 callee 意味（al==2・モーダル種別）＋0x578FE0/0x579060 の '3' 状態機械 | 0x579CF0=**モーダル メッセージ確認ランナー**（0xB131FC 応答byte・応答域=ボタン index）[確定]／al==2 語義=3番目ボタン [部分確定]／モーダル種別=modal slot 表 7 スロット (0xB12DD0..0xB12E18・stride 0xC) 索引・ラベルは外部 XML [部分確定]。'3'=[ui+8] 状態byte: 書込元=**0x57D640**（0x57D699/0x57D6EB・全画像 byte 形 2件）／読取元=**0x578FA0**（0x578FD0 cmp）と **0x583E60**（0x583E74）／遷移 3→5 条件=[ui+9]==0（0x583E96）[確定] | callers(0x579CF0)=3（0x45CC29/0x45DDEA/0x66FF42）。callback 0x578DC0: `bl=[ui+0xB0]→[0xB131FC];[ui+0xB0]=0xFF`。PeekMessageA/Translate/Dispatch = IAT 0xA2826C/68/6C（pefile 解決）。0x578FE0→`jmp 0x57CF60`（stack top）・0x579060→`call 0x57CFA0(1)`（2番目スロット）（P2/P2b/P9b/P12） |
| ③ | [[0xB33398]+0x10]（main スレッドID）書込主源＋[0xB33398] クラス正式名 | 書込主源=**0x404A50 `mov[esi+0x10],eax`**（eax=GetCurrentThreadId・ ctor 0x404A00 内）[確定] — 全 .text `GST→mov[reg+0x10]`（≤4命令）**1 hit**。0x405xxx 群=該当 0 [確定負]。0x431D60=**ThreadSpecificInterfaceManager::AddInterface**（自名文字列 0xA36480）＝worker 用配列書込・main 主源でなく [確定負]。クラス正式名: オブジェクト 0x28B・ctor に vptr store **0件**・[0xB33398] 静的=0xFFFFFFFF → **RTTI 到達不能 [確定負]**、機能名=メインスレッド情報ブロック（hwnd/TID/ThreadHandle・'Main' 0xA2FC44）[部分確定] | 唯一 WRITE-ABS=**0x40E831** `mov [0xb33398],eax` head **0x40DF80**（callers=1=0x98780E∈CRT 起動窓）：new(0x28)@0x40E816→ctor 0x404A00@0x40E826→store。ctor: +8=arg1・+0xC=hWnd(CreateWindowExA [0xA28270])・+0x10=GST・+0x14=DuplicateHandle（P2/P2b/P2c/P8/P11） |
| ④ | LoadGame 0x465860 引数契約完結（arg1=esi mode1 無視の呼び手5サイト）＋0x45A190/0x45A4E0 busy 連鎖 bit11 | 引数契約 [確定]: LoadGame(this=SLG, **arg1=esi=死引数**, arg2=name, arg3=detail bool)。arg1 使用= `mov[esp+0xb0],esi`（spill・再読 0件）＋ 0x465923 `push esi`→0x45F2E0 arg1（**arg1 slot [esp+0x648] 読取 0件**＝mode1 のみ全 mode 無視）。呼び手5サイト esi 算出表（§4）。busy 連鎖 [確定]: bit11 set=**0x465AF3**（直後 0x45A190→0x45AC7 経由）・clear=**0x465B9E**（0x45A4E0=0x465BA8 の直前）・再 clear=**0x46694F** — 全 .text 0x800/0xfffff7ff@[reg+0x18] = **3件全て head 0x465860 内**。v324/v315 0x47BF74 系 bit11 との同一物仮説=**[確定負]**（0x47BC40 窓に mask op 0 hit）。bit18 thread-mirror との独立性=**[確定]**（bit11 はスレッド条件なし unconditional・bit18 or site は 0x45A566/0x45EE17/0x465AE9/0x4667FE の別 op） | callers(0x465860)=**5**（v312 census 复唱）: 0x466B31/0x466D5F/0x5025F7/0x5AE1E6/0x5B5DA8。0x45A190 callers=2（0x465814∈SaveGame/0x465AC7∈LoadGame）、0x45A4E0 callers=1（0x465BA8）（P3/P4/P5/P6/P9/P10/P11/P12） |
| ⑤ | fn 0x444340 正式名称・所属（grid 二重ループ＋0x447740/0x4441A0 系譜） | grid 二重ループ実在 [確定]（[0xB06A2C]=uGridsToLoad 静的5・窓内 ref 5 site・外/内 `cmp e?,eax; jae`）。系譜照合 [確定]: callers(0x4441A0)=**1**=**0x4443EC（0x444340 内・唯一）**、0x447740=同ループ内 @0x4443FF（callers=21）。正式名称=**[未確定]**（fn 窓に文字列 0・vftable 所属 0・RE_EXE_REPORT grep `444340/444740/4441A0` **0 hits**）→ 所属は機能在確 [部分確定]: TES4 cell grid 管理帯（v245 FUN_0043F560..FUN_00444840）＝セル grid 読込/更新ルーチン（0x4441A0='Loading cell…' 本体を呼ぶ・0x45A500×3 スレッドゲート・0x57B950 進捗） | callers(0x444340)=3（0x444D3C/0x444E42∈0x444C70・0x4454A1∈0x444FB0）（P3 §5/P4 §5b/5c/P10） |
| ⑥ | fn 0x45F7A0 identity | **TESSaveLoadGame::SaveGameData** [確定]（埋込エラー文字列 0xA3A4E0='Error: TESSaveLoadGame::SaveGameData() was called while loading.' 自名・v232 TU 文字列機構実在の先例）。仕組み: head=0x45F7A0（0x45F790 JT 終端直後・16 align・CC 窓なし）→ [[0xB33398]+0x10]（③の main TID）vs GetCurrentThreadId で **[this+0x18] bit0/bit18 ミラー選択**（0x45A500 と同基準）→ ビット立=ロード中に呼ばれた側として [[0xB34D90]] vtbl+0x18（log）へ self-name 文字列通知 → `[ebp+edi*4]` 配列を [esi+0x14] ストリームカーソラに count/4 dword 書（[esi+0x7D] 立時は 0x45E0D0 経由）。callers=95 | 0x45E0D0（head False・callers=9）=[0xB33A98] ハッシュ照合書込 helper。[[0xB34D90]] の実行時身元=#82 待記 §10（P2c/P7） |
| ⑦ | [v341待記] Temp Effects container（679xxx）正式型名 — 主手段(a) 0x764650/0x9FB020/0x9FC350 | **親指定3座標は本検体で不成立 [確定負]**（⑦-a 代替経路で解決）: 0x764650=非 head・in-fn（0x764630=**ThreadSpecificInterfaceManager 系** per-interface プール・`push 0x33` 窓内 0 site）／0x9FB020=非 fn head（head 0x9FAFE0・callers=0・0x764699 に call なし）／0x9FC350=**vtable でなくコード**（VT-4=CCCCCCCC・SEH fn 先頭・唯一 xref=0xA2AE08∈.rdata **ファンクションポインタ表** 0x9FC140../0x9FC350/0x9FC3B0… 約 stride 0x60）。**実 Load/ディスパッチ fn = 0x679850**（head✓・callers=1=0x45FD4B∈head 0x45FC60∈callers=1 **0x466904∈LoadGame**）: u16 count=`call 0x4534d0(2,&w)`（v339 item1 契約衔接）→ JPT 0x679A54=**5 kind** → new{**0x1C,0x54,0x20,0x38,0x4C**}＋ctor{**0x56BDE0,0x56CDE0,0x570700,0x69E350,0x6A0700**} → vptr{**0xA6822C,0xA6851C,0xA686CC,0xA761C4,0xA764B4**} = RTTI 直読 `'.?AVBSTempEffectDecal@@'`/`'…GeometryDecal@@'`/`'…Particle@@'`/`'.?AVMagicModelHitEffect@@'`/`'.?AVMagicShaderHitEffect@@'`・base ctor 0x56BC20→0xA681AC=**'.?AVBSTempEffect@@'**。**要素正式型名=BSTempEffect 系 [確定]**。書込み先=`add ecx,[ctx]+0x40/0x48; call 0x677CF0`（=0xB3BD00 qA/qB head 対・node new(8){payload,next}）→ v341 §2.2/§3.4 と衔接。container/ctx クラス名自体=[待記継続→#82]（手段(b) v341 のとおり静的リーチ外） |
| ⑧ | [v341待記] メニュー名表 0xA69xxx 登録元 | store site 全列挙 [確定]: .rdata 0xA69xxx への絶対書込=構造上 0（RO ページ・census 0 窓宣言）／`mov[reg],imm(0xA69xxx)`=**52件 全て vptr 書**（TileMenu/Menu/NiTArray/NiTMap ctor-dtor 群 0x581850..0x58FADA・静的書換 3 件=0x9FB9E2/0x9FBA42/**0xA24916**）／`push imm(0xA69xxx)`=**123件**（XML 路径供給・0x57E000..0x5DD500 帯）。**登録元=0x591C50＝TileMenu vftable 0xA6AF14 slot+0x14（仮想・callers=0）** [確定]（0x591D1B-24 `mov ecx,0xB13970; SetAt(-0x3E9, item=esi≠0)`）／除去=0x591B70 dtor 系（0x591BC2-C9 item=0）。**v341 §3.3 訂正: 0x591D24 は除去でなく登録**（v250 N-37『真の登録側=0x591C50』を slot+0x14 まで確定）。59名の**登録順・最終名列挙=実行時（メニュー生成順）＝静的リーチ外→ [確定負] 宣言で最終クローズ**（.data [0xB13970] 初期=vptr+count0 のみ） |

判定内訳: [確定] ①(census/帰属)・②(状態機械命令列)・③(主源/0x431D60 排除)・④(契約/bit11)・⑤(二重ループ/系譜)・⑥(自名)・⑦(要素型名)・⑧(store census/登録元) ／ [部分確定] ①cell サブセット・②al==2 ラベル/モーダル種別名・③クラス機能名・⑤所属・⑦u16=文字列長計の対応細部・⑧供給元マップ ／ [確定負] ①直接形・③0x405xxx/RTTI 名前・④0x47BF74 同一物・⑦親指定3座標・⑧登録順（最終クローズ） ／ [未確定] ⑤正式名称。#82 移し=§10 の4件。

---

## §1 ① TESObjectCELL vftable 0xA469D4 — slot+0x6C 実 call site 全列挙＋vtable store 帰属
（帯境界宣言: 0x4C0000-0x4E0000 は #106 閉済で自前可。本節の窓は命令境界严格・head からの整列リニア（C-5 遵守）。0x4D3xxx は #106 の 0x4D5F20 未満側。）

**A. vftable 本体（复唱＋新規境界検証）** — P1/A1: `u32(0xA469D4-4)=0xABCCBC`（COL sig=0・TD(off+12)=0xB03038=`'.?AVTESObjectCELL@@'`）、`[0xA469D4+0x6C]=0x4CD960`（idx27・v340 F 复唱）。逆走査: 0xA469D4 直前 dword=COL（コードポインタでない）→ vtable 先=0xA469D4 確定（entry 0 個＝先頭）。`find_dword(0x4CD960)` 全イメージ=**1件**（0xA46A40=.rdata slot 内のみ）→ slot27 は vtable 経由のみ到達（直接 E8 callers=0 复唱）。

**B. `call dword ptr [reg+0x6C]` 直接形 census** — P1/A3＋再帰り P1b: 全 .text（0x401000 〜 +0x626E00）生走査、ModRM FF /2 の mod=01 disp8=6C／mod=10 disp32=6C000000／SIB 形（rm=4 含む）を全列挙し capstone 復号で命令先頭一致を検証 → **0件**。対照統制（教訓『0件は不在証拠にしない』対応）: 同一走査で FF/2 mod01 disp8∈0x60..0x70=**518件**（+0x68=292・+0x14=281・+0x24=134）→ 直接 call 形自体は多用されており、+0x6C 限定で 0 = **この検体では二段形が主流**。よって二段形へ再帰り。

**C. 二段形 `mov r32,[base+0x6C]` → `call/jmp r32`** — P1b/P1c/P1d: `8B /r`（mod=01/10・SIB 含）で disp=0x6C の全 load site=**641**（capstone 検証一致 641）。load 直後≤5命令に同 dst レジスタの `call reg`/`jmp reg` を持つ site=**68**。breg 供給元分類（head 起点の実命令チェーン逆引き・v344 教訓 ecx-map 分離準拠）: **68/68=VTBL-DISPATCH**（直前に `mov breg,[X]` 純ポインタ fetch＝vtable 参照）。初版 census 2件系統の訂正を §自己確認に記録（61→68=lookahead 4→5・jmp 尾形 2 site 追加: 0x740826/0x74E9A6 tail-jmp 形）。
- head 分布: **0x44F3D0×16**（リスト walk ディスパッチ群 head）／0x45EC50×2／0x4F2C10×2／0x63BA30×2／単発 48 head（0x465860 LoadGame・0x461310・0x45F180・0x45C4F0・0x4471A0・0x4CBD30・0x4CD960・0x4D4310・0x700460・0x707610・0x76A1C0・0x89D650・0x9A2090 …）。全 site の head・callers・命令窓（mov 起点〜call）を表で付す（表1）。


**表1: ① 二段形 call/jmp site 全 68 件**（site / mov / call 位置 / 所属 head, callers / 命令窓（head 起点実チェーン末端6+call・*=call 位置）。CELLZONE=0x4C0000-0x4E0000）

| # | mov site | mov | call/jmp | head, callers | 命令窓（mov〜call） |
|--:|---|---|---|---|---|
| 1 | 0040686E | mov eax, [edx + 0x6c] | call@00406871 | 0x00406360,1 | 00406858:mov dword ptr [eax + 8], ecx ; 0040686E:mov eax, dword ptr [edx + 0x6c] ; 00406871:call eax* |
| 2 | 00416982 | mov eax, [edx + 0x6c] | call@00416987 | 0x004168A0,1 | 00416977:add eax, 1 ; 00416982:mov eax, dword ptr [edx + 0x6c] ; 00416987:call eax* |
| 3 | 004472CD | mov eax, [edx + 0x6c] | call@004472D2 | 0x004471A0,60 | 004472BB:mov byte ptr [esp + 0x34], al ; 004472CD:mov eax, dword ptr [edx + 0x6c] ; 004472D2:call eax* |
| 4 | 00447D98 | mov edx, [eax + 0x6c] | call@00447D9B | 0x00447D80,2 | 00447D8D:lea ecx, [ecx] ; 00447D98:mov edx, dword ptr [eax + 0x6c] ; 00447D9B:call edx* |
| 5 | 0044F798 | mov edx, [eax + 0x6c] | call@0044F79B | 0x0044F3D0,2 | 0044F78E:mov edi, edi ; 0044F798:mov edx, dword ptr [eax + 0x6c] ; 0044F79B:call edx* |
| 6 | 0044F7B8 | mov edx, [eax + 0x6c] | call@0044F7BB | 0x0044F3D0,2 | 0044F7AD:lea ecx, [ecx] ; 0044F7B8:mov edx, dword ptr [eax + 0x6c] ; 0044F7BB:call edx* |
| 7 | 0044F7D8 | mov edx, [eax + 0x6c] | call@0044F7DB | 0x0044F3D0,2 | 0044F7CE:mov edi, edi ; 0044F7D8:mov edx, dword ptr [eax + 0x6c] ; 0044F7DB:call edx* |
| 8 | 0044F7F8 | mov edx, [eax + 0x6c] | call@0044F7FB | 0x0044F3D0,2 | 0044F7EE:mov edi, edi ; 0044F7F8:mov edx, dword ptr [eax + 0x6c] ; 0044F7FB:call edx* |
| 9 | 0044F818 | mov edx, [eax + 0x6c] | call@0044F81B | 0x0044F3D0,2 | 0044F80E:mov edi, edi ; 0044F818:mov edx, dword ptr [eax + 0x6c] ; 0044F81B:call edx* |
| 10 | 0044F838 | mov edx, [eax + 0x6c] | call@0044F83B | 0x0044F3D0,2 | 0044F82E:mov edi, edi ; 0044F838:mov edx, dword ptr [eax + 0x6c] ; 0044F83B:call edx* |
| 11 | 0044F858 | mov edx, [eax + 0x6c] | call@0044F85B | 0x0044F3D0,2 | 0044F84E:mov edi, edi ; 0044F858:mov edx, dword ptr [eax + 0x6c] ; 0044F85B:call edx* |
| 12 | 0044F878 | mov edx, [eax + 0x6c] | call@0044F87B | 0x0044F3D0,2 | 0044F86E:mov edi, edi ; 0044F878:mov edx, dword ptr [eax + 0x6c] ; 0044F87B:call edx* |
| 13 | 0044F898 | mov edx, [eax + 0x6c] | call@0044F89B | 0x0044F3D0,2 | 0044F88E:mov edi, edi ; 0044F898:mov edx, dword ptr [eax + 0x6c] ; 0044F89B:call edx* |
| 14 | 0044F8B8 | mov edx, [eax + 0x6c] | call@0044F8BB | 0x0044F3D0,2 | 0044F8AD:lea ecx, [ecx] ; 0044F8B8:mov edx, dword ptr [eax + 0x6c] ; 0044F8BB:call edx* |
| 15 | 0044F918 | mov edx, [eax + 0x6c] | call@0044F91B | 0x0044F3D0,2 | 0044F90F:nop ; 0044F918:mov edx, dword ptr [eax + 0x6c] ; 0044F91B:call edx* |
| 16 | 0044F942 | mov edx, [eax + 0x6c] | call@0044F947 | 0x0044F3D0,2 | 0044F939:mov edi, eax ; 0044F942:mov edx, dword ptr [eax + 0x6c] ; 0044F947:call edx* |
| 17 | 0044F981 | mov eax, [edx + 0x6c] | call@0044F984 | 0x0044F3D0,2 | 0044F974:jne 0x44f97c ; 0044F981:mov eax, dword ptr [edx + 0x6c] ; 0044F984:call eax* |
| 18 | 0044F9CF | mov eax, [edx + 0x6c] | call@0044F9D2 | 0x0044F3D0,2 | 0044F9C0:mov ecx, dword ptr [esi + 0xc4] ; 0044F9CF:mov eax, dword ptr [edx + 0x6c] ; 0044F9D2:call eax* |
| 19 | 0044F9E9 | mov eax, [edx + 0x6c] | call@0044F9EC | 0x0044F3D0,2 | 0044F9DF:je 0x44f9f5 ; 0044F9E9:mov eax, dword ptr [edx + 0x6c] ; 0044F9EC:call eax* |
| 20 | 0044FA18 | mov eax, [edx + 0x6c] | call@0044FA1B | 0x0044F3D0,2 | 0044FA0C:lea esp, [esp] ; 0044FA18:mov eax, dword ptr [edx + 0x6c] ; 0044FA1B:call eax* |
| 21 | 0045C6E8 | mov eax, [edx + 0x6c] | call@0045C6ED | 0x0045C4F0,4 | 0045C6D4:push 0xa3a5b8 ; 0045C6E8:mov eax, dword ptr [edx + 0x6c] ; 0045C6ED:call eax* |
| 22 | 0045ED84 | mov edx, [eax + 0x6c] | call@0045ED87 | 0x0045EC50,2 | 0045ED77:mov edx, dword ptr [ecx + 8] ; 0045ED84:mov edx, dword ptr [eax + 0x6c] ; 0045ED87:call edx* |
| 23 | 0045EDDD | mov edx, [eax + 0x6c] | call@0045EDE0 | 0x0045EC50,2 | 0045EDD0:mov edx, dword ptr [ecx + 8] ; 0045EDDD:mov edx, dword ptr [eax + 0x6c] ; 0045EDE0:call edx* |
| 24 | 0045F294 | mov eax, [edx + 0x6c] | call@0045F299 | 0x0045F180,1 | 0045F283:jne 0x45f28b ; 0045F294:mov eax, dword ptr [edx + 0x6c] ; 0045F299:call eax* |
| 25 | 004614B4 | mov eax, [edx + 0x6c] | call@004614B9 | 0x00461310,1 | 004614A6:lea ecx, [esi + 0x10] ; 004614B4:mov eax, dword ptr [edx + 0x6c] ; 004614B9:call eax* |
| 26 | 004665F7 | mov eax, [edx + 0x6c] | call@004665FC | 0x00465860,5 | 004665E6:jne 0x4665ee ; 004665F7:mov eax, dword ptr [edx + 0x6c] ; 004665FC:call eax* |
| 27 | 004CBFD1 | mov edx, [eax + 0x6c] | call@004CBFD6 | 0x004CBD30,1 | 004CBFC5:push 0 ; 004CBFD1:mov edx, dword ptr [eax + 0x6c] ; 004CBFD6:call edx* |
| 28 | 004CD993 | mov eax, [edx + 0x6c] | call@004CD998 | 0x004CD960,0 | 004CD987:mov eax, dword ptr [ebx + 8] ; 004CD993:mov eax, dword ptr [edx + 0x6c] ; 004CD998:call eax* |
| 29 | 004D43CE | mov eax, [edx + 0x6c] | call@004D43D3 | 0x004D4310,3 | 004D43BF:mov byte ptr [esp + 0x10], al ; 004D43CE:mov eax, dword ptr [edx + 0x6c] ; 004D43D3:call eax* |
| 30 | 004F17CF | mov edx, [eax + 0x6c] | call@004F17D4 | 0x004F1630,19 | 004F17C0:sete dl ; 004F17CF:mov edx, dword ptr [eax + 0x6c] ; 004F17D4:call edx* |
| 31 | 004F2E55 | mov edx, [eax + 0x6c] | call@004F2E58 | 0x004F2C10,0 | 004F2E46:call 0x452600 ; 004F2E55:mov edx, dword ptr [eax + 0x6c] ; 004F2E58:call edx* |
| 32 | 004F2E69 | mov edx, [eax + 0x6c] | call@004F2E6C | 0x004F2C10,0 | 004F2E5E:jne 0x4f2e30 ; 004F2E69:mov edx, dword ptr [eax + 0x6c] ; 004F2E6C:call edx* |
| 33 | 004FC855 | mov edx, [eax + 0x6c] | call@004FC859 | 0x004FC850,1 | 004FC850:push esi ; 004FC855:mov edx, dword ptr [eax + 0x6c] ; 004FC859:call edx* |
| 34 | 00520EBB | mov edx, [eax + 0x6c] | call@00520EC2 | 0x00520E90,0 | 00520EB0:push edi ; 00520EBB:mov edx, dword ptr [eax + 0x6c] ; 00520EC2:call edx* |
| 35 | 00521049 | mov edx, [eax + 0x6c] | call@0052104E | 0x00520F70,0 | 0052103E:mov edi, eax ; 00521049:mov edx, dword ptr [eax + 0x6c] ; 0052104E:call edx* |
| 36 | 0052EF63 | mov eax, [edx + 0x6c] | call@0052EF68 | 0x0052EE30,0 | 0052EF57:mov eax, dword ptr [ebp + 4] ; 0052EF63:mov eax, dword ptr [edx + 0x6c] ; 0052EF68:call eax* |
| 37 | 005377C5 | mov eax, [eax + 0x6c] | call@005377CD | 0x005377B0,2 | 005377B3:call 0x8a6410 ; 005377C5:mov eax, dword ptr [eax + 0x6c] ; 005377CD:call eax* |
| 38 | 00538641 | mov eax, [eax + 0x6c] | call@00538650 | 0x00537F10,0 | 0053862C:movaps xmmword ptr [esp + 0x120], xmm1 ; 00538641:mov eax, dword ptr [eax + 0x6c] ; 00538650:call eax* |
| 39 | 0063BC7C | mov edx, [eax + 0x6c] | call@0063BC86 | 0x0063BA30,0 | 0063BC62:ja 0x63bf1f ; 0063BC7C:mov edx, dword ptr [eax + 0x6c] ; 0063BC86:call edx* |
| 40 | 0063C310 | mov edx, [eax + 0x6c] | call@0063C31A | 0x0063BA30,0 | 0063C305:push edi ; 0063C310:mov edx, dword ptr [eax + 0x6c] ; 0063C31A:call edx* |
| 41 | 0064422D | mov eax, [edx + 0x6c] | call@00644237 | 0x00644050,0 | 0064421B:call 0x659a00 ; 0064422D:mov eax, dword ptr [edx + 0x6c] ; 00644237:call eax* |
| 42 | 00678E47 | mov eax, [edx + 0x6c] | call@00678E4C | 0x00678D90,3 | 00678E3C:jne 0x678e30 ; 00678E47:mov eax, dword ptr [edx + 0x6c] ; 00678E4C:call eax* |
| 43 | 006EA226 | mov edx, [eax + 0x6c] | call@006EA229 | 0x006EA220,0 | 006EA220:mov ecx, dword ptr [esp + 4] ; 006EA226:mov edx, dword ptr [eax + 0x6c] ; 006EA229:call edx* |
| 44 | 00700514 | mov edx, [eax + 0x6c] | call@00700519 | 0x00700460,1 | 0070050A:pop ebx ; 00700514:mov edx, dword ptr [eax + 0x6c] ; 00700519:call edx* |
| 45 | 007076B6 | mov eax, [edx + 0x6c] | call@007076BC | 0x00707610,117 | 007076AA:push 1 ; 007076B6:mov eax, dword ptr [edx + 0x6c] ; 007076BC:call eax* |
| 46 | 0070A8A3 | mov eax, [eax + 0x6c] | call@0070A8A7 | 0x0070A840,0 | 0070A896:mov ecx, dword ptr [edx + edi*4] ; 0070A8A3:mov eax, dword ptr [eax + 0x6c] ; 0070A8A7:call eax* |
| 47 | 00716068 | mov edx, [eax + 0x6c] | call@0071606D | 0x00716050,8 | 0071605A:call 0x68f970 ; 00716068:mov edx, dword ptr [eax + 0x6c] ; 0071606D:call edx* |
| 48 | 00740823 | mov edx, [edx + 0x6c] | jmp@00740826 | 0x00740800,0 | 00740813:mov ecx, dword ptr [ecx + 0x5c] ; 00740823:mov edx, dword ptr [edx + 0x6c] ; 00740826:jmp edx* |
| 49 | 0074DAE7 | mov eax, [eax + 0x6c] | call@0074DAED | 0x0074D9E0,0 | 0074DAD6:call edx ; 0074DAE7:mov eax, dword ptr [eax + 0x6c] ; 0074DAED:call eax* |
| 50 | 0074E9A3 | mov edx, [edx + 0x6c] | jmp@0074E9A6 | 0x0074E980,0 | 0074E993:mov ecx, dword ptr [ecx + 0x68] ; 0074E9A3:mov edx, dword ptr [edx + 0x6c] ; 0074E9A6:jmp edx* |
| 51 | 0076A6F6 | mov eax, [eax + 0x6c] | call@0076A6FA | 0x0076A1C0,1 | 0076A6E6:mov ecx, eax ; 0076A6F6:mov eax, dword ptr [eax + 0x6c] ; 0076A6FA:call eax* |
| 52 | 007781AA | mov eax, [ecx + 0x6c] | call@007781AD | 0x00778180,1 | 0077819A:mov edx, dword ptr [esp + 0x1c] ; 007781AA:mov eax, dword ptr [ecx + 0x6c] ; 007781AD:call eax* |
| 53 | 00778292 | mov edx, [edx + 0x6c] | call@00778297 | 0x007781F0,4 | 00778284:push ecx ; 00778292:mov edx, dword ptr [edx + 0x6c] ; 00778297:call edx* |
| 54 | 0077841A | mov eax, [ecx + 0x6c] | call@0077841D | 0x00778350,1 | 0077840D:push edx ; 0077841A:mov eax, dword ptr [ecx + 0x6c] ; 0077841D:call eax* |
| 55 | 00778FAC | mov eax, [edx + 0x6c] | call@00778FB0 | 0x00778F40,0 | 00778F9F:call 0x77c0f0 ; 00778FAC:mov eax, dword ptr [edx + 0x6c] ; 00778FB0:call eax* |
| 56 | 00779023 | mov edx, [eax + 0x6c] | call@0077902A | 0x00779010,1 | 00779016:mov esi, eax ; 00779023:mov edx, dword ptr [eax + 0x6c] ; 0077902A:call edx* |
| 57 | 0077B7BF | mov edx, [eax + 0x6c] | call@0077B7C4 | 0x0077B7B0,0 | 0077B7B4:mov esi, ecx ; 0077B7BF:mov edx, dword ptr [eax + 0x6c] ; 0077B7C4:call edx* |
| 58 | 0078014C | mov edx, [eax + 0x6c] | call@00780151 | 0x00780140,0 | 00780141:mov esi, ecx ; 0078014C:mov edx, dword ptr [eax + 0x6c] ; 00780151:call edx* |
| 59 | 00781633 | mov edx, [eax + 0x6c] | call@00781639 | 0x007814C0,0 | 00781628:push ecx ; 00781633:mov edx, dword ptr [eax + 0x6c] ; 00781639:call edx* |
| 60 | 007817E0 | mov edx, [eax + 0x6c] | call@007817E6 | 0x00781670,0 | 007817D5:push ecx ; 007817E0:mov edx, dword ptr [eax + 0x6c] ; 007817E6:call edx* |
| 61 | 007AB108 | mov eax, [edx + 0x6c] | call@007AB10D | 0x007AA990,0 | 007AB0FB:je 0x7ab10f ; 007AB108:mov eax, dword ptr [edx + 0x6c] ; 007AB10D:call eax* |
| 62 | 007C2775 | mov eax, [edx + 0x6c] | call@007C277A | 0x007C2710,2 | 007C2760:mov ecx, dword ptr [esp + 0x2c] ; 007C2775:mov eax, dword ptr [edx + 0x6c] ; 007C277A:call eax* |
| 63 | 007DA9D5 | mov eax, [edx + 0x6c] | call@007DA9DD | 0x007DA220,1 | 007DA9CB:mov eax, dword ptr [edx + 0x38] ; 007DA9D5:mov eax, dword ptr [edx + 0x6c] ; 007DA9DD:call eax* |
| 64 | 007F1E24 | mov eax, [edx + 0x6c] | call@007F1E29 | 0x007F1D90,0 | 007F1E1A:mov edx, dword ptr [edi] ; 007F1E24:mov eax, dword ptr [edx + 0x6c] ; 007F1E29:call eax* |
| 65 | 0089752F | mov eax, [edx + 0x6c] | call@00897534 | 0x00897490,1 | 00897523:test cl, 1 ; 0089752F:mov eax, dword ptr [edx + 0x6c] ; 00897534:call eax* |
| 66 | 00898719 | mov edx, [edx + 0x6c] | call@00898725 | 0x008984E0,0 | 0089870A:mov ecx, esi ; 00898719:mov edx, dword ptr [edx + 0x6c] ; 00898725:call edx* |
| 67 | 0089D660 | mov edx, [eax + 0x6c] | call@0089D666 | 0x0089D650,7 | 0089D652:mov edi, dword ptr [esp + 0xc] ; 0089D660:mov edx, dword ptr [eax + 0x6c] ; 0089D666:call edx* |
| 68 | 009A2272 | mov eax, [edx + 0x6c] | call@009A2277 | 0x009A2090,0 | 009A2267:mov eax, dword ptr [esp + 0x24] ; 009A2272:mov eax, dword ptr [edx + 0x6c] ; 009A2277:call eax* |


- cell 限定候補（CELLZONE・窓の this 供給が cell/obj ポインタ）: **0x4CBD30**（site 0x4CBFD1・直前 `call 0x45BDE0(this=SLG,esi,-1,0)`＝TESObjectCELL ref 検索後の dispatch）、**0x4CD960 本体**（site 0x4CD993・slot27 自身が this+0x48 list の要素に +0x6C dispatch＝再帰的走査）、**0x4D4310**（site 0x4D43CE・0x45A500/0x45A530 busy ゲート窓＋`or byte[esi+0x24],0x10`）、**0x465860 LoadGame 内 site 0x4665F7**（bit11 clear 群直後・this=[ebx]）。 receivers の型静的推論の限界 → cell 実 dispatch 確定は [部分確定]（残件 §9-1）。
**D. vtable store 2 件の帰属** — P1/A2/A4・P1e: imm32=0xA469D4 を持つ命令 = 0x4D325E `mov[esi],0xa469d4`（head **0x4D3210**・is_head✓cc=14・SEH prologue・`call 0x46C460`(TESForm base ctor)+[esi+0x18] に 2nd vftable **0xA469C0** 書＝多重 inheritance・callers=**7**: 0x44721B/0x448782/0x44DF13/0x4BDF86/0x4EF236/0x4F1716/0x4F31D0 → head 0x4471A0/0x447DF0/0x44DCF0/0x4BDF20/0x4EF1F0/0x4F1630/0x4F2C10＝**cell 生成経路 7 点**）と、0x4D32ED `mov[esi],0xa469d4`（head **0x4D32C0**・GS cookie・callers=**1**=0x4D55E3、host head=**0x4D55E0=find_dword 単一ヒット→0xA469E4=vftable 0xA469D4 slot+0x10（idx4）自体**。E8 callers=0=仮想のみ）。0x4D32C0 も同 pair（[esi]=0xA469D4／[esi+0x18]=0xA469C0）を書き [esi+8] bit 検査後 `call 0x4CED70` 等 → **slot+0x10 仮想（Load 系）から呼ばれる vptr 再設定 ctor/reset**。帰属 [確定]。

## §2 ② FUN_00579CF0 と '3' 状態機械（帯: 0x57xxxx #105 閉済・自前可）
**0x579CF0**（is_head✓・callers=3=0x45CC29/0x45DDEA/0x66FF42・窓 0x579CF0..0x579FB0）: ①entry `mov ecx,[0xb3a6b0]; push 1; push 2; mov byte[0xb131fc],0xFF; call 0x572EC0` — 0x572EC0=**モーダル slot 管理**（arg2×0xC・表 0xB12DD0..0xB12E18＝**7 slot**・0x572DC0 が ctor で全 clear／slot[2] を使う経路が本 fn）、0xB131FC=応答 sentinel 0xFF 初期化。②UI ゲート連鎖 `0x582160(0,1)`（InterfaceManager・遅延 new(0x134)+ctor 0x5802B0・[0xB3A6E0]）の [eax+0x1C]/[eax+0x68] 非ゼロ＋Tile trait **0xFAE** vs f32[0xA379B4] → 不成立で return 0。③表示 `push 0x578DC0(callback); … call 0x5BC8B0`（b20_impl の message_menu.xml ラッパ・menu 0x3E9）または fallback `call 0x5BCC00`。④`test al,al; je return0` → 応答まで **PeekMessageA/TranslateMessageA/DispatchMessageA**（IAT 0xA2826C/68/64 pefile 解決）ポンプ＋每回転 `mov ecx,[[0xB33398]+0x20]; call 0x4046A0`（エンジン tick）→ `mov al,[0xb131fc]; cmp al,0xFF; je ループ`、≠0xFF で **al を返して終了**。callback **0x578DC0**（E8 callers=0=ファンクタ）: `bl=[[0x582160]+0xB0]; mov[0xb131fc],bl; mov byte[…+0xB0],0xFF`＝UI+0xB0（ボタン index）→ 応答 byte。**al==2 = 3 番目ボタン**（命令レベル確定）・ラベル語義（Yes/No/OK の何れか）= message_menu.xml 外部依存 → [部分確定]。モーダル種別の列挙可能性: 表 7 slot は静态確定、slot→モーダル名は XML/メニュー側 → 列挙不可宣言。
**'3' 状態機械**（状態=ui+8 byte・ui=0x582160(0,1)）: 書込元（全 .text `C6 4x 08 03` census）=**2 site・both in 0x57D640**: 0x57D699（arg==3 特殊路: [ecx+0xE0]=3 push 後 stack 長1の時）と 0x57D6EB（通常 push: 初 slot 占有時に [ecx+8]=3＋0x3F3/0x3E9 特別分岐）。いずれも `mov byte[0xb42d54],1` 随伴（モダル活性 byte・消費側 0x583F40）。dword 形 `mov[reg+8],3` 5 site（0x4ED539/0x69C066/0x8B5B81×2/0x94A302×2/0x94D970）は別オブジェクト帯＝統制混同禁止注記。読取元: `cmp byte[reg+8],3` census=**1 site 0x578FD0**（head 0x578FA0・callers=2∈0x664C40・'3' 述語 bool 返）＋ 0x583E60@0x583E74（callers=3: 0x579E8F＝**0x579CF0 ポンプ尾**／0x5ABB1A／0x5B8F7E）。遷移: **3→5 条件=[ui+8]==3 ∧ [ui+9]==0**（0x583E7C）→ 0x583E96 `[esi+8]=5`（0x403160=[0xB33398]+0x20 lock＋cursor tile 0xFA1/文字列 0xA69194・v231 と衔接）。`cmp byte[reg+8],5` census=0（窓宣言）。0x578FE0（callers=27）/0x579060（callers=8）= ゲート連鎖後の **menu stack アクセサ**（0x57CF60=top 探索 [ecx+0xE0..0x104・max 10]/0x57CFA0(idx)=[ecx+idx*4+0xE0]・0x579060 の実 arg=1）。「返す 3」= stack へ modal push 時に最上位値が 3 になる状態機の反映（0x57D640 push 経路）であり、両 fn は stack 値 read そのもの。

## §3 ③ [[0xB33398]+0x10] main スレッドID 書込主源＋[0xB33398] クラス名
xrefs(0xB33398) 全 .text 検証済み=**296**（READ=146・WRITE-ABS=**1**・OTHER=149＝命令途中復号の誤検形注記）。WRITE=**0x40E831** `mov [0xb33398],eax`（head 0x40DF80・is_head✓・callers=1=0x98780E：`push 0/push eax([0xB02C84])/… call 0x40df80` の CRT 起動窓＝WinMain 系）。窓: CreateWindowExA([0xA28270])→**new(0x28)** 0x40E816→ctor **0x404A00**@0x40E826（callers=1）→0x40E831 store。ctor 0x404A00（16 align・CC 窓なし・SEH `push -1; push 0x9a9ef6`）: [esi+8]=arg1、[esi+0xC]=arg2=hWnd、**0x404A47 `call [0xa2808c]`(GetCurrentThreadId) → 0x404A50 `mov[esi+0x10],eax`**、+0x14=DuplicateHandle(GetCurrentThread・0xA280C0)。全 .text GST→`mov[reg+0x10],eax`（≤4命令）census=**1 hit（同 site）**→ **主源確定** [確定]。
候補排除: ①0x405xxx 群 — 窓内 0xB33398/GST→[+0x10] hit **0**（[0xB33398] 参照は 0x40DF80 窓＝0x40E-0x40F のみ）→ 主源でない [確定負]（0件は窓宣言付き、Write-ABS 全域 1 件の肯定証拠で補強）。②**0x431D60**（is_head✓・callers=26）= InterlockedIncrement 予約→GST→**`mov [edx+esi*8],eax`**（＝per-thread 配列・this+8・stride 8）＋`call [0xa28148]`=TlsSetValue、エラー窓 `push 0xa36480` → 文字列 **'Could not add new interface for thread %08X in ThreadSpecificInterfaceManager::AddInterface. Max threads is:'** **自名確定** — 役割=ワーカスレッドのインターフェース登録で **main tid 主源ではない** [確定負]（C-3 混線回避の実例）。
クラス正式名: ctor 全体（0x404A00..0x404AF2 ret 8）に **vptr store 0 件**、静的 [0xB33398]=0xFFFFFFFF（runtime fill）→ COL/TD 経路なし＝RTTI 直接読取 **[確定負]**（帯外可の宣言付き・TD=.data 0xB02000+ 走査も vptr 不在で対象なし）。機能名=メインスレッド情報ブロック（tid@+0x10・hwnd@+0xC・thread handle@+0x14・'Main' 命名文字列 0xA2FC44）[部分確定]。なお 0x579CF0/0x583E60 が読む [obj+0x20]=UI/ロック系 sub-object（[[0xB33398]+0x20]→0x4046A0／0x403160）— 本 0x28B オブジェクトの 4 番目のフィールド群。

## §4 ④ LoadGame 0x465860 引数契約＋busy 連鎖 bit11
（帯: 0x45/0x46 ＝ #103/#104 閉済・自前可・head 0x465860 整列リニア窓）
**引数契約完結** [確定]: prologue `mov esi,[esp+0x334]`(arg1)/`mov edi,[esp+0x338]`(arg2=name)/`cmp byte[esp+0x33c],0`@0x465A44(arg3=detail)・`mov ebp,ecx`(this)。arg1(esi) の fn 内参照=**3 件のみ**（0x46589B 読／0x4658AB `mov[esp+0xb0],esi` spill／0x465920 `push esi`）。spill スロット [esp+0xb0] の再読込=**0**（0x465860..0x466B00 census・[esp+0x334] 直接再読 0）。0x465923: `push 1(arg3)/push edi(arg2)/push esi(arg1)/mov ecx,ebp; call 0x45f2e0` — 0x45F2E0 側 arg1 slot=[esp+0x648]: 全 fn（0x45F2E0..0x45F790）で arg1 slot 読取 **0 件**（read=0x45F31B [esp+0x64c]/0x45F35D [esp+0x650] のみ・path=local [esp+0x234] @0x45F725）→ **v340「arg1=esi が mode1 で無視」は実際=全 mode で無視の死引数**（C-4: 命令窓で完結）。
**呼び手5サイト esi 算出＝意味表**（P4 §4.1b・生 push 順）:
| site | host head（callers） | 実 push（arg1,arg2,arg3） | esi(arg1) 算出の意味 |
|---|---|---|---|
| 0x466B31 | 0x466B00（2: 0x672F56/0x673955） | 0 / **0xA3B144='quicksave'** / 0 | quicksave 名ロード・arg1=null 死引数 |
| 0x466D5F | 0x466BE0（2: 0x443543/0x446459） | 0 / [ecx+0x1C0] / 0 | **pending-name スロット**（=0x45A190 が clear する [SLG+0x1C0]）の遅延ロード |
| 0x5025F7 | 0x502500（0=コンソール table） | 0 / &local buf / [esp+0x10] | console `load <名> [int]`・arg3=第2 int 非ゼロ→1（v253 と一致） |
| 0x5AE1E6 | 0x5AE140（0=仮想 LoadMenu 0x40E 選択） | **eax=[esi+0x4c]（選択項目）** / 0 / 0 | 選択 ptr を死 arg1 に回付（name は SLG 内部解決） |
| 0x5B5DA8 | 0x5B5D60（0=仮想） | **eax=[[SLG+0x6c]]** / 0 / 0 | 現 save name ptr を死 arg1 に回付 |
→ arg1 は 5 サイト全てで無視（意味=履歴的 name 枠・死参数）。arg2=0 の 4/5 系で name をどこから解くか（[esp+0x340]@0x465B50・0x465A44 ゲート）は LoadGame 後半未読 → 残件 §9-4。
**busy 連鎖 bit11 区間境界** [確定]: 全 .text `or [reg+0x18],0x800`／`and [reg+0x18],0xfffff7ff` census=**3 site 全て head 0x465860**（set @**0x465AF3**・直後クリア @**0x465B9E**＝0x45A4E0@0x465BA8 の 1 命令前・終盤クリア @**0x46694F**）。0x45A190（head✓・callers=2=0x465814∈SaveGame 0x465130・0x465AC7∈LoadGame・`[edi+0x1c0]` free+パス分解 '\\' 0xA3207C・ret 4）＝連鎖前段の slot reset、0x45A4E0（head✓・callers=1=0x465BA8・空文字 0xA2F7EC＋[0xB38740]→0x579C10 message ラッパ・callers(0x579C10)=**90**）＝clear 直後の UI 通知。→ bit11 区間=[0x465AF3, 0x465B9E]＋[?, 0x46694F]（2 区間の set 側は imm 形 census 不在＝reg マスク形未走査・窓宣言→残件 §9-5）。
**照合**: 0x47BF74 系（v315/v324 の bit11 言及）= head 0x47BC40（TESUpdate 窓・v273）内 `call 0x480820`@0x47BF74 — **0x47BC40..0x47C400 に 0x800/0xfffff7ff/0x40000 の and/or op 0 hit**（窓宣言）→ **SLG+0x18 bit11 と 0x47BF74 系 bit11 は別物** [確定負]（v315 の 0x461CF4 群 bit11=**changeFlags** ネームスペースで busy flags と混同不可＝v311 S3 と整合）。**bit18 との独立性** [確定]: bit18 set=0x465AE9/0x4667FE（＋0x45A566 helper/0x45EE17）、bit0 set=0x465AE4/0x4667F8 — bit11 set@0x465AF3 は cmp 分岐無しの unconditional（v340 G3 の命令列复唱と同一）→ thread-mirror 独立。

## §5 ⑤ fn 0x444340 — 正式名称・所属・系譜（帯: 0x444xxx 自帯域・v325/#103 系）
head ✓（v340 E1 复唱）・callers=3（0x444D3C/0x444E42∈head 0x444C70・0x4454A1∈head 0x444FB0）。**grid 二重ループ実在 [確定]**: 0x444343 `mov eax,[0xb06a2c]`（uGridsToLoad・ini 既定 5=v91 表／静的 [0xB06A2C]=5 复唱 P8 §7）→ `shr eax,1`→ [ebp+0x20]/[ebp+0x24]（中心 i/j）減算、0x4443BD `cmp ecx,eax; jae 0x444534`（外側）・0x4443D2 `cmp ebx,eax; jae 0x44452C`（内側）→ 本体 0x4443EC **`call 0x4441a0`**（セル読込本体・v324 §3-3）＋0x4443FF **`call 0x447740`**（±0x7FFF ガード grid accessor・0x4EFE80 内部）。[0xB06A2C] ref=5（0x444343/3B6/499/4F8/51B）。
**系譜照合 [確定]**: callers(0x4441A0)=**1 件＝0x4443EC（本 fn 内）**（v324 の 0x444252 は 0x4441A0 窓内命令＝`call 0x4c9aa0/0x4c9a80` 直後・head(0x444252)=0x4441A0 复唱 — 形式注記として記録し v324 裁定は差替えない）。0x447740 callers=21（0x4441EE/0x4443FF/0x44461F…＝本 fn 群と grid 再描写帯 0x4C7AA4../0x5E8791/0x6700D0）。v325 記録 0x447740/0x4441A0 は v325 ファイル内 0 hit（grep）→ 実出典は v324/v315 側である旨衔接注記。
**正式名称**: find_dword(0x444340)=**0**（vftable 所属なし＝メンバでなく自由 fn/thiscall 無）・fn 窓（0x444340..0x4445C5）に文字列 0（v340 E3 复唱）・RE_EXE_REPORT grep `444340/444740/4441A0/45F7A0…` **0 hits**（P9/P11）→ 正式 C++ 名 **[未確定]**。所属 [部分確定]: v245 帯写し（FUN_0043F560..FUN_00444840=TES4 cell grid 管理）＋ callee 0x4441A0('Loading cell…' v324)＋0x483D60(DistantLOD v135)＋0x45A500/0x45A530×3＋0x4523A0/0x57B950 進捗切替（v340 E2 复唱）＋0x45FDA0 → **「ロード中のセル grid 二重走査ルーチン（TES4 cell grid 管理帯・busy/スレッドゲート随伴）」**。GridCellArray(RTTI 0xB06A54・v245 §2) との同一系譜は this+0x74/DistantLOD 窓から [推定]注記まで（確証手段なし）。

## §6 ⑥ fn 0x45F7A0 identity（自帯 0x45xxxx・#103 閉済）
is_head=False（CC 窓なし・直前=0x45F790 の JPT 4 entries `eff54500/fcf64500/56f74500/63f74500`＝v340 ①(c) の表終端と一致・16 align）→ head 0x45F7A0 確立。callers=**95**。先頭 8 命令窓（命令严格・v344 JPT 教訓適用）: `mov eax,[0xb33398]; mov edi,[eax+0x10]`＝**③ の main TID を消費**（C-3: reader 側は登録表/スレッド系で混線なし）、GST と cmp → 同一スレッドなら `[esi+0x18] bit0` / 非同一なら `bit18`（0x45A500 と同基準ミラー読取）→ ビット成立時 `mov ecx,[0xb34d90]; mov edx,[ecx]; call [edx+0x18]` ＋ **push 0xA3A4E0='Error: TESSaveLoadGame::SaveGameData() was called while loading.'** ＝ **関数自己名文字列**（v232 TU 文字列機構実在の先例）→ 本体＝`[esp+0x14]` 配列の `shr ebx,2`（count/4）dword を **[esi+0x14] ストリームカーソラ**へ直接 `mov[edx],eax; add[edx],4` 書、[esi+0x7D] 立時は **0x45E0D0**（[0xB33A98] ハッシュ照合→[this+0x74] stream 追記 helper・callers=9）経由、`ret 8`。判定: **fn 0x45F7A0 = TESSaveLoadGame::SaveGameData（セーブ配列ストリーム書込・誤スレッド検出 self-log）** [確定]（名はバイナリ自前文字列由来・意味窓=ロード中誤呼出の警告経路）。[[0xB34D90]] の runtime 身元=#82（§10）。

## §7 ⑦ Temp Effects container 正式型名（v341 待記・手段(a)=push 側深掘り）
**親指定(a)座標の検証（命令窓＋生 census・不成立なら無効化せず代替へ）**:
- 0x764650: is_head=False・直前命令群は 0x764630（SEH head・callers=0?→**host head=0x764630**）の fn 中部。0x764630=**6 スロット stack 配列 {1,0,2,4,3,5} 初期化＋ [esi+0x6F4]×4 テーブル walk＋ new(0x70)→0x70E560（vptr=**NiPixelData** 0xA7E624）＋ Interlocked 差し替え** — 0xA36480 自名（§3）と同一家族の **インターフェース・プール**であり temp effect ctor でない。窓内 `push 0x33(=51)` census=**0 site**（0x764000..0x765000）。
- 0x9FB020@0x764699: 0x764699 の命令は `jne 0x76469a`/周辺（call なし）。0x9FB020 自体は .text 中部（head 0x9FB040 前有 0x9FAFE0・**callers(0x9FB020)=0**・find_dword=0）＝new ラッパ座標として不成立。
- vtable 0x9FC350: RTTI 経路不可 — [0x9FC34C..4F]=CC padding、0x9FC350 にコード（`push -1; push 0x9c074a; mov eax,fs:[0]…` SEH prologue＋`push 0xB14158; mov ecx,0xB07BF0`）。唯一 xref=**0xA2AE08**∈.rdata 約 stride 0x60 の **関数ポインタ配列**（0x9FBE60/0x9FC140/0x9FC170/0x9FC1D0/0x9FC230/0x9FC290/0x9FC2F0/**0x9FC350**/0x9FC3B0/0x9FC410/0x9FC470/0x9FC4D0…＝0x9Fxxxx 群＝CRT init-atexit 形 table・v341 §3.3 の 0x9FBA90 登録窓と同族）→ **「vtable 0x9FC350」[確定負]**（窓宣言）。
**代替主手段（実 load/dispatch fn）で解決 [確定]**: **fn 0x679850**（is_head✓ SEH・callers=**1**=**0x45FD4B**、host=**0x45FC60**・callers=1=**0x466904∈LoadGame head 0x465860** — 窓 `mov ecx,0xb3bd00; call 0x679850`）。body: u16 count を `call 0x4534d0(2, &word)`（=v339 item1 I/O 契約衔接・[SLG+0x7C] 0x26/0x5E ゲート随伴）→ count 回ループ→ type byte 読み **JPT 0x679A54 = {0x6798E4, 0x67995D, 0x679980, 0x6799FE(unknown), 0x6799AA}**：
- 0x6798E4: `push 0x1c; call new` → ctor **0x56BDE0** → vptr 0xA6822C = **`'.?AVBSTempEffectDecal@@'`**（col 0xAC0ABC）
- 0x67995D: `push 0x54` → ctor **0x56CDE0** → vptr 0xA6851C = **`'.?AVBSTempEffectGeometryDecal@@'`**
- 0x679980: `push 0x20` → ctor **0x570700** → vptr 0xA686CC = **`'.?AVBSTempEffectParticle@@'`**
- 0x6799AA: `push 0x38` → ctor **0x69E350** → vptr 0xA761C4 = **`'.?AVMagicModelHitEffect@@'`**
- 0x6799D4: `push 0x4c` → ctor **0x6A0700** → vptr 0xA764B4 = **`'.?AVMagicShaderHitEffect@@'`**
- 0x6799FE: `push 0xA74660='Unknown temp effect type: %i'; call 0x4A7A60`
全 ctor が base **0x56BC20** を呼び vptr **0xA681AC='.?AVBSTempEffect@@'`** を経る（RTTI 直読・TD=.data 0xB02000+ 教訓遵守）。生成物 enqueue=`lea ecx,[ctx+0x40]/[+0x48]… call 0x677CF0`（0x677CF0=tail 追加・**node new(8){payload,next}**・0x679850 内 0x679953/0x679A1B）→ v341 §2.2 qA/qB +0x40/+0x48 と §3.4 node{+0,+4} に完全衔接。判定: **Temp Effects container（679xxx）要素正式型名 = `BSTempEffect*`（派生5種：Decal/GeometryDecal/Particle/MagicModelHitEffect/MagicShaderHitEffect・JPT5 kind）[確定]**。u16=文字列長計 side（v339 item1）は本 load 窓では count として消費（save 側対応の細部 [部分確定]）。container/ctx クラス名自体＝手段(b) 経路（v341 静的リーチ外宣告）維持 → **#82/B13 runtime ダンプ待記**（§10）。

## §8 ⑧ メニュー名表 0xA69xxx 登録元（v341 待記）
- **store site 全列挙** [確定]: (i) .rdata 0xA69xxx（0xA28000+0x41000.. の RO 領域）への絶対書込命令=**0**（ページ属性上成立せず・census 0 件は窓宣言付き、肯定側の代替 (ii)(iii) で補強）。(ii) `mov [reg+disp], imm32∈0xA69000..0xA69FFF` 全 .text=**52 件**（P4 §8.1 列挙）— 全て **vptr 書**（Tile/TileMenu/Menu/NiTArray/NiTMapBase 群 ctor/dtor: 0x584400/584413/584433/5844D7/584644/5855B0/587470/58FADA/590…＋静的書換系 3 件＝0x9FB9E2→[0xB13948]/0x9FBA42→[0xB1395C]/**0xA24916→[0xB13970]=0xA69680**（v341 §3.1 の DTOR thunk 复唱））。(iii) `push imm∈0xA69xxx`=**123 件**（XML 路径/書式文字列供給＝'Data\Menus\*.xml'・0x57EE7C../0x5864E3 cursor/strings.xml 等）→ **0xA69xxx=名'テーブル'実体でなく UI クラス群 vtable/文字列池**。名表本体は [0xB13970] `NiTArray<Tile*>`（v341 ②c [確定] の差替なし）。
- **登録元（唯一の item 非null insert）= 0x591C50** [確定]: find_dword(0x591C50)=**[0xA6AF28] = TileMenu vftable 0xA6AF14（v341 §3.2）slot+0x14**・E8 callers=**0**＝**仮想呼出専用**（vptr-store 書込 0x591B9A と整合）。body: arg1=0xFA2 特別路（`0x587B20→0x587D20→[esi+0x44]`）＋共通尾 0x591D13-D24: `lea eax,[esp+0x10]; push eax; add ebx,0xfffffc17(-0x3E9); push ebx; mov ecx,0xB13970; mov[esp+0x18],esi; call 0x42bb90` = SetAt(idx=menuID-0x3E9, item=&esi)＝**登録**（v250 N-37「真の登録側 FUN_0x591C50」再確認・slot+0x14 まで確定）。除去=**0x591B70** dtor 系（0x591BC2-C9 同型 call・item=0）。**訂正 C-⑧（v341 §3.3）**: 0x591D24 は「除去経路」でなく**登録**（item=esi≠0）。v341 の「真の登録経路は census 上に存在せず」は本 bundle で解消。
- **登録順序・59 名列挙**: .data [0xB13970] 初期は vptr+count0（v248 §N-33 复唱）→ 実体は**実行時 TileMenu 生成順**（OnCreate→slot+0x14 仮想）で充填＝静的に順序確定不能 → 親指示ルールどおり **「静的リーチ外」宣言で最終クローズ [確定負]**。名文字列供給元は (ii) push 123 件マップ（XML 路径 .rdata 静的）だが「59 名の最終順」は XML データ/起動順依存で EXE 静的圏外。

## §9 新規残件＋次束候補（v344 は #117 所管＝非干渉。v345+ 仮予約を提案）
1. [①] 68 二段 site の**受信者型確定**（CELLZONE 4 点: 0x4CBD30/0x4CD960/0x4D4310/0x465860@0x4665F7 の this walk-up と 0x44F3D0×16 の供給リスト特定）— 自帯窓で到達可。
2. [②] モーダル slot 表 0xB12DD0×7 の各 slot 所有者（`[slot]` 読取元列挙＝0x572EC0 以外の使用者）と al ボタン値↔XML ラベル対応（XML=外部・docs 圏外）。
3. [②] [0xB42D54]（活性 byte）の消費側 0x583F40 詳細と 0x57CFE0（menu stack 管理者 head）identity。
4. [④] LoadGame arg2=0 系（0x5AE1E6/0x5B5DA8）の**name 解決分岐**＝[esp+0x340] 0x465B50・0x465A44 ゲート以降の後半 fn マップ。
5. [④] bit11 第 2 区間（0x465B9E..0x46694F）の set 側＝reg マスク `or [ebp+0x18], reg` 形 census（窓あり再帰り）。
6. [⑦] [0xB3BD00] container/ctx クラス正式名＝**B13 runtime ダンプ**（#82 §10）。
7. [⑧] 0x42BB90 の this=0xB13970・item≠0 全 callers（0x591C50 以外の二次登録源有無の総当り）＋push 123 件の XML→メニュー名マップ完成。
8. [③付随] 0x40DF80（WinMain 系）＋0x747830（'Main' 書込=SetThreadName?）の身份確認。
→ これらは **v345+ 仮予約（#118 と整合）**へ。#117（v344）topics には非干渉。

## §10 #82 待記移し一覧（ランタイムゲート・確定試行なし）
| 項目 | 必要証拠 |
|---|---|
| Temp Effects container/ctx（[0xB3BD00]）クラス正式名・qA/qB runtime vptr | B13 実機 .data ダンプ（0xB3BD00/0xB3B980 先頭語→COL→TD）＝v341 §5 から継続。要素側=BSTempEffect 系は本束 [確定] 済 |
| [0xB33398] オブジェクト（0x28B）のクラス正式名 | 静的リーチ外（vptr 0 件）— runtime vptr dump があれば RTTI 可（機能名は §3 で確定） |
| [[0xB34D90]]（SaveGameData エラー log 先）の身元 | runtime .data dump（WRITE-ABS 0・静的 0x7400C87D=未初期化） |
| message_menu.xml ボタン配置→al 値（al==2 の言語ラベル） | 実機/データファイル（外部 XML・docs 圏外） |
| モーダル 7 slot の種別名 | 0xB12DD0 runtime dump＋XML |
| メニュー登録 59 名の実順序 | runtime（TileMenu 生成順trace・§8 宣言で最終クローズ済み・記録のみ） |

## §11 無変更宣言・使用ツール
- 無変更: docs/*（Oblivion.exe 系含む）・既存 tmp/re/*.md・.ess/ESM・移植ソース・STATE.md/TODO.md（台帳クローズは親）・git（branch switch/edit なし）。**新規作成は本 1 ファイルのみ**。
- probe/一時: `%TEMP%\opencode\oblivion_v343\`（v343_lib.py＋v343_P1..P12.py/.out・table68.md）。リポジトリ内増殖なし。
- ツール: python 3 + pefile 2024.8.26 + capstone 5.0.7（re_util/v340_lib 読み取り再利用）。Ghidra MCP は `execute→tools["ghidra-mcp"].list_instances()` で **instances=[]（接続実体なし）** を確認し python PE 直接解析に代替（試行 1 回・AGENTS TOOL 則遵守）。
- 教訓遵守: ①直接形 0 件→窓宣言＋二段 68 件の再帰り成功／②dword/byte census 統制併記／③0x431D60 自名で混線排除／④引数契約は push 窓＋slot 読取 census のみで言及／RTTI TD 直接アドレス指定（H1 方式）／帯境界＋命令窓を全裁定に明記。
- ⑦の親指示座標（0x764650/0x9FB020@0x764699/vtable 0x9FC350）は本検体で不成立（§7 に反証窓）。裁定は代替経路 0x679850 で確定。

## 附表（① ②の生 census 数値）
- ①: 直接 call [reg+6C] 0／mov r,[r+6C] 641／二段 call/jmp 68（VTBL-DISPATCH 68・FIELD 0）／find_dword(0x4CD960)=1／vtable store 命令 2（imm hit 2）。
- ②: cmp byte[r+8],3=1／mov byte[r+8],3=2／dword 形 5（統制）／callers: 0x579CF0=3・0x578FE0=27・0x579060=8・0x578FA0=2・0x583E60=3・0x572EC0=8・0x579C10=90。
- ③: xrefs(0xB33398)=296・WRITE=1・GST→[+0x10]=1・0x431D60 callers=26。
- ④: callers(0x465860)=5・bit11 op=3（imm 形）・bit18 op set/clear §4・0x45A190/0x45A4E0 callers=2/1。
- ⑤: [0xB06A2C] ref=5・callers 3/1/21。
- ⑦: JPT 5 arm・new size 5 種・RTTI 6 名（base 含む）。
- ⑧: vptr-store 52・push-imm 123・自写込(名表)=0。
