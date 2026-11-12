# v327: v324 クラッシュ解析の新規残件 7 件の裁定（ai-todo #91 / 次束N）

- 作成: 2026-11-07 / L1 report-only・クリーンルーム / 成果物は本ファイルのみ新規（probe は `%TEMP%\opencode\v327_*` prefix）
- 出典: `tmp/re/v324_crash_rootcause.md` §6（および §1-§5 の 4 ダンプ解析結論を前提知識として継承）
- 判定5段階: [確定]/[部分確定]/[推定]/[未確定]/[確定負]
- **L2 禁止事項遵守**: 項目7 はパッチ**設計の提案書**まで。バイナリへの適用・実装は一切しない（**L2 承認待ち**）

---

## §0 手法 / 入力

- 入力 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / base=0x400000 / SizeOfImage=0x805000 / sections: .text RVA0x1000 raw0x626E00, .data RVA0x702000 VirtualSize0xA9C1C raw0x30C00、読み取りのみ）
- 入力ダンプ: `%LOCALAPPDATA%\CrashDumps\Oblivion.exe.{28964,9936,39788,32488}.dmp`（4本、v324 §7 と同パス）
- 入力 docs: `docs/Oblivion_decompiled.c`（Ghidra デコンパイル・参照のみ）
- 手段: pefile 2024.8.26 + capstone 5.0.7（線形 disasm・rule v3 fn-end・E8/disp32 機械 census）＋ MDMP 自前パース（v322 と同一方式）＋ **numpy 2.4.6 ベクトル化**（disp32 xref / 分岐ターゲット走査）
- **Numba**: 着手時に `import numba` を1回確認 → `ModuleNotFoundError`（未インストール）→ 規定どおり **numpy ベクトル化にフォールバック**（v327_4 の disp32 xref、v327_7/7b/7c の分岐ターゲット走査）。残りは素の Python / bytes.find / capstone（短命 probe で JIT 初回秒数を回収不可のため不採用）
- probe: `%TEMP%\opencode\v327_*.py` → `v327_*.out`（**18 本**: 1, 1b, 2, 2b, 2c, 3, 3b, 3c, 3d, 4, 4b, 5, 5b, 5c, 6, 7, 7b, 7c）。既存 `v322_*` は参照・流用のみ（改変なし）
- **帯遵守**: 深解析 = `0x49A000-0x49B000`（FUN_0049a200 全体・パッド）/ `0x40D000-0x445000`（FrameUpdate 系: FUN_004441A0/00444340/00444FB0/00444C70/00444EC0/004430F0/PerFrameUpdate/WinMain）/ `0x4C0000-0x4E0000`（センサス域の深帯28サイト＋callee FUN_004c9a80/004c9aa0）。v318 自帯は一覧・VA のみ、帯外は census（地址のみ）・caller 隣接の浅層 window（±0x40B）・浅層 xref に限定。**実プロセス操作なし・バイナリ書き換えなし**。例外宣言: pid39788 突合のため Windows Application イベントログを**読み取り専用**で1回照会（ログは帯外だが変更なし・プロセス操作なし）。

---

## §1 項目1: `FUN_0049a200` の詳細（二重ループの中身・docs シグネチャ）

### 1-1. docs シグネチャ `void(void)` は this(ECX) 引数欠落 — **[確定]（v324 §6-6② の再確認・訂正対象）**

- 実コード: **`0x49A22D: mov dword ptr [esp+0x18], ecx`**（プロローグ直後・v327_1）で ECX を受領し、**`0x49A324: mov ebx,[esp+0x18]; add ebx,0x48; cmp [ebx],esi`**（＝`*(this+0x48)` 検査、docs L123277-123278 に対応）で**実際に使用** [確定]。
- docs 側: `docs/Oblivion_decompiled.c` **L123202 `void FUN_0049a200(void)`**（this 欠落）。
- 補足: `[esp+0x18]` は **`0x49A5C6: mov [esp+0x18],edi`**（ループB内側イテレータ保存）で**再利用**される → docs が this 引数を lost した一因と推定される（stack slot の再利用）。
- **5 呼出しすべてが this=[World+0x54] を渡す**: `0x40F25B: mov ecx,[eax+0x54]`（WinMain・in-band disasm）/ `0x441E4A: mov ecx,[ebx+0x54]` / `0x444F90: mov ecx,[esi+0x54]`（in-band）/ `0x445515: mov ecx,[esi+0x54]`（帯外・浅層 bytes `8b4e54 e8…`）/ `0x482374` 付近 `mov ecx,[eax+0x54]`（帯外・浅層 bytes `a1a033b300 8b4854 e8…`）→ **this = `[World+0x54]` オブジェクト [確定]（4 disasm + 1 浅層 bytes）**。
- docs 呼出し行は 4 件（L51019, L53227, L53460, L104415）vs EXE 5 site → **v324 §6-6①（WinMain 内 0x40F25B 未収録）を再確認 [確定]**。

### 1-2. 二重ループの中身（線形 disasm 501 命令 / span 0x49A200-0x49A8B0・v327_1/1b）

| 区分 | 境界 | 内容（証拠） |
|---|---|---|
| エントリゲート | 0x49A233-0x49A266 | `[0xB07050]/[0xB42F3E]/[0xB070A0]!=0` かつ `[World+0x34]==0` を通過、否則 early-out 0x49A889（ダンプ 4/4 全開放と一致・v324 §1-3 維持） |
| マージン算出 | 0x49A26C-0x49A2B1 | `div [0xB070E0]`（5/3）→ FPU 変換 2 連呼（FUN_00985A00/FUN_009828C0）→ EBP/iStack マージン（docs L123253-63） |
| **ループA** | **0x49A2C0-0x49A322** | 5×5 スキャン（外側 EBX・内側 ESI、境界 `[0xB06A2C]`=5）。条件: edge(`idx<EBP or N-EBP<=idx`) / slot 空(`test edi,edi; je`) / `FUN_004CE3C0(entry)!=0` / `entry+0x24` bit1。**ガード付き**: `0x49A2FD mov ecx,[edi]; 0x49A2FF test ecx,ecx; 0x49A301 je 0x49A318`。**メモリ書込なしの純スキャン**（有効セル発見で外側へ）|
| 後続ブロック | 0x49A324-0x49A4B6 | `*(this+0x48)==0` → エラーログ(FUN_007C23C0/FUN_0075FA90)、obj 0x124 確保、**ガード付き現在セル位置ブロック**: `0x49A43B call 0x440560; test eax,eax; je` → GetCellX/GetCellY → obj+0x54/58/5C へ（docs L123308-123321）。**本ブロックの GetCellX site=0x49A453 は FUN_00440560()!=0 ガード付き [確定]** |
| **ループB** | **0x49A5B6-0x49A72D** | 5×5（外側 EBP・内側 EDI）。各スロットに `FUN_00707530(4)` でオブジェクト生成、`[obj+0x72]` 有効フラグ、内側 do-loop 4 回（`FUN_00441800` → vtable+0x54 戻り値 5..10 判定 → float → `[obj+0xA0]`）、インデックス `[obj+0x74/0x78]` 記入。**ガード 2 か所**: `0x49A622 mov ecx,[ebx]; 0x49A624 test; je 0x49A719` / `0x49A66C mov ecx,[eax]; 0x49A66E test; je 0x49A6B8`。終了時 **EBP=EDI=5（クラッシュ時の EBP/EDI=5 は本ループの走破残存値 [確定]）** |
| **tail（クラッシュブロック）** | 0x49A76A-0x49A7B7 | `mov edx,[World]; mov ecx,[edx+8]` → `push 1; push 1; call 0x482150` → `mov esi,eax; mov ecx,[esi]; call 0x4C9A80`（★0x49A783）→ `shl eax,0xc; mov [esp+0x1c],eax; fild; fstp [0xB45EEC]` → `mov ecx,[esi]; call 0x4C9AA0`（0x49Y 側・非ガード）→ `shl; cmp [0xB45DCC],0; fstp [0xB45EF0]` → **`0x49A7B8: je 0x49A84E`** |
| DCC ブロック | 0x49A7BE-0x49A84D | `mov eax,[esp+0x30]`（local、0x49A283/0x49A32D で書込み済）→ 参照差し替え `*(0xB45DCC+0x10C) = 新規参照`（refcount inc/dec 付き、`0x49A81C je 0x49A84E` で同一なら無変更）= docs L123425-123451 と一致 |
| エピローグ | 0x49A84E-0x49A8A2 | `lea ecx,[esp+0x8c]; call 0x70DFA0` → refcount 解放 → cookie 復元 → pop×4 → `ret`。**先頭で ECX/ESI を再定義（skip 継続の安全性に寄与・項目7）** |

- **fn-end（rule v3）**: ret at 0x49A8A2 → E=0x49A8A3、early-ret 対象なし、CC ラン 13B → 16 アライン非 CC = **0x49A8B0 = docs next head `FUN_0049a8b0` と一致 [確定]**（CC ラン不足型誤答の教訓に従い rule v3 で判定）。
- 兄弟ガード符号（v327_7）: loop-A `8b 0f 85 c9 74 15` / loop-B `8b 0b 85 c9 0f 84 ed 00 00 00`（near jz）/ inner `8b 08 85 c9 74 46` → **`mov ecx,[reg]; test ecx,ecx; je/jz` の同一パターン [確定]**。
- 帯内 CC ラン（≥8B, 0x49A000-0x49B000）: 0x49A146(10) / **0x49A1C2(14)** / 0x49A1D5(11) / **0x49A8A3(13)** / 0x49AC58(8)。0x49A1D0 は `E9→0x49A170` の実命令（パッド結合は不可）。

---

## §2 項目2: slot(1,1) entry[0] を読む他 caller 経路の完全性（92サイト/56関数・深帯28）

### 2-1. センサス再現と分類 — **[確定]**

- 再 census: **E8→0x4C9A80 = 92 サイト / docs-head 56 関数**（v327_2 再現、v322_2_census.txt 92 行と一致）。GetCellY(E8→0x4C9AA0) = **94 サイト / 56+2 関数**（v327_2b）。
- 92 サイト全分類（v327_2・14 命令 tail・深帯43サイト=0x4C帯28+0x40D帯12+0x49A帯3 は全文 tail 出力）:
  - **window インデクサ(FUN_00482150)経由 = 3 サイト**: `0x49A783`(args=1,1) / `0x50EEBC`(ebx,ecx 動的) / `0x537E04`(0,0)
  - ECX 直接 deref 系 = 35、レジスタ/其它 = 54、**test ecx+即時 jcc ガード = 15、call を飛び越える jcc = 0**
- **const (1,1) の window 読み = 全 .text 中 `0x49A77A` の1 サイトのみ [確定]**（v327_2c・40 indexer site 全数の args 正確抽出。post-context = `mov esi,eax; mov ecx,[esi]; call 0x4c9a80` で**非ガード**）。
- GetCellY 側も同様: **indexer 経由 = 3 サイト**（`0x49A79B`(1,1・同一崩壊ブロック) / `0x50EED2` / `0x537E0E`(0,0)）→ **slot(1,1) の X/Y 読みは FUN_0049a200 の tail ブロックの2 call のみ・双方非ガード [確定]**。
- 他の39 indexer caller: 引数はレジスタ対（esi,edi 等）・(0,0)・空。post-context の大半は `mov reg,[eax]; test/jz` 形の**ガード付き deref**（例 0x43F930/0x4403CF/0x442F23/0x537DF7 `mov esi,[eax]; test esi,esi; je`）。
- GetCellX 92 の大半は **cell 階層ナビゲーション**（`[reg+0x20]`/`[reg+0x24]` → `[+0x98]`、親子セル参照）で window スロット非経由 [確定・深帯 tail 証拠]。

### 2-2. `[reg+0x30]` ファミリ（0x440590/004406A0/004407A0/00440880/00440970）の切り分け — **[部分確定]**

- 各先頭で `mov esi/edi, ecx`（this=ECX）→ `mov ecx,[this+0x30]` は **オブジェクトフィールド this+0x30 の単重参照**（window[1][1] には `[World+8]`→`+0x30` の二重参照が必要で別アドレス）。FUN_00440590 の呼出し `0x444D6D` は `mov ecx,esi`（World）を渡す → [World+0x30] フィールド [部分確定]。
- 5 site 中 4 は `test ecx,ecx; je` ガード付き（0x440A43 は test と je の間に `mov edi,edx` が挿まるが flags 非改変で実質ガード）。

### 2-3. 残る不確実性 — **[未確定]**（§9 R1 へ）

- **インライン slot(1,1) 読み**（FUN_00482150 を介さない `base+0x30` 直接アドレスコード）の全数走査は本 lane 未実施 → 92+94 サイト＋40 indexer サイトの網羅では拾えない可能性。

---

## §3 項目3: FrameUpdate 系 `FUN_004441A0` の投入タイミング定量（危険帯フレーム数）

### 3-1. 投入チェーンの census（v327_3・全 E8）— **[確定]**

```
FUN_004441A0 (cell loader・唯一の FUN_00482170 呼出し)  : 1 site  = 0x4443EC (in FUN_00444340)
FUN_00444340 (window pass)                             : 3 sites = 0x444D3C, 0x444E42 (FUN_00444C70)
                                                         + 0x4454A1 (FUN_00444FB0)
FUN_00444FB0 (World 毎フレーム更新)                     : 6 sites = 0x40DD31, 0x40DD98 (PerFrameUpdate)
                                                         + 0x445641, 0x445BA2, 0x446238, 0x462F89
FUN_00444C70                                             : 1 site  = 0x444F70 (in FUN_00444EC0)
FUN_00482170 (slot 投入/クリア)                          : 1 site  = 0x444275 (in FUN_004441A0)
FUN_004821b0 (空検査)                                   : 2 sites = 0x443159 (FUN_004430F0), 0x444211 (FUN_004441A0)
FUN_00482200 (slot クリア2)                             : **0 sites**
FUN_00482390 (投入後 notify)                            : 2 sites = 0x441DF0, 0x444283
FUN_00482310 (one-shot refresh→FUN_0049a200)           : 3 sites = 0x40EE73, 0x445C61, 0x460185
PerFrameUpdate 0x40D800                                 : 1 site  = 0x40F270 (WinMain)
```

### 3-2. pass の同期性・全数性 — **[確定]**

- `FUN_00444340` は **1 回の呼び出しで N×N=25 スロット全てを処理する同期パス**: docs L52813-14（origin = `[World+0x20/0x24] − (N>>1)`）+ L52853-52960（二重ループ内で `FUN_004441A0(x,y,...)` を全セルに呼ぶ＋進捗 `local_14/(N*N)`）。disasm でも pass は `0x4454A1 call 0x444340` として**直前のゲート（[0xB35C24] は progress 呼出しのみ.skip、pass 自体は無条件）で到達 [確定]**。
- `FUN_004441A0` の投入条件（docs L52734-52745）: `[World+0x74]!=0` ＋（`FUN_004f1630` または `FUN_00447740` でセル発見）＋ `FUN_004821b0(x,y)==0`（slot 空、または entry 状態≠6）→ sprintf "Loading cell..." → `FUN_00482170(x,y,cell)` → notify。**同期・1 pass で全空スロット投入 [確定]**。
- **同一フレーム内の順序**: `FUN_00444FB0` は pass(0x4454A1) の**後**に `0x445518: call 0x49a200`（クラッシュ関数）を呼ぶ → **フレーム有効時は「先に充填、後から読む」で安全 [確定]**。
- pass の触发経路: ①毎フレーム `PerFrameUpdate(0x40F270)` → `FUN_00444FB0`（ただし caller `0x40DD1C: cmp [cell+0x26],3/6; je skip` と `[0xB33A98]!=0` ゲート付き）、②origin 遷移: `FUN_00444FB0` の sentinel 経路 `0x444FF2 je 0x44556B`（origin==0x7FFFFFFF）→ origin 再計算 → `0x4455BF call 0x444EC0` → `0x444F70 call 0x444C70`（**再入ロック [0xB33A69]**: entry で set1・exit 0x444EA9 で clear・再入時は 0x444EB0 へ即 ret = one-shot ではなく**再入防止 [確定]**、両 branch 共に pass 含有）、③`FUN_00444EC0` の他 caller 群。
- **アイドル経路**: WinMain `0x40F260 push 0x32; call Sleep; jmp 0x40F275` が **`0x40F270 call PerFrameUpdate` を飛ばす** → アイドル時は pass が一切走らない [確定]（v324 §3-4 と整合）。
- `FUN_004430F0`（全 pass 直前の `push 0/1; call`）は **クリアではなく事前カウンタ**: window 全走査で cell+0x48 リンク数を数え、>0 なら `FUN_00440af0(1,0,0)`（docs L51968-52035 + v327_3d disasm 一致）→ **slot クリア経路ではない [確定]**。

### 3-3. 危険帯フレーム数の裁定 — **[部分確定]**（v324 §4-4 の補正・削除せず併記）

- **v324 §4-4「ロード直後・遷移直後の数フレームが危険帯 [推定]」を本 lane の構造証拠で補正**: 投入は 1 pass で全25スロット・同期・origin 遷移でも直後の `FUN_00444C70` で即時再構されるため、**フレーム処理が有効な間の NULL ウィンドウ存続は ≤1 フレーム**（=「数フレーム」ではなく同一フレーム内）[部分確定]。
- 一方、**クラッシュ実態のアイドル分岐（Sleep 経路）では pass が走らないため、NULL は無期限に存続する** → 危険帯はフレーム数ではなく「**初回の成功 pass まで（フレーム有効なら ≤1 フレーム、アイドルなら無期限）**」[部分確定]。
- pass が走っても投入されない持続条件: `[World+0x74]==0` または `FUN_00447740` のセル発見失敗（docs L52734/52855 ゲート）→ **投入なしでスロットが NULL のまま [部分確定]**。
- slot クリア経路: `FUN_00482170(obj=0)` → vtable+0x1C（E8 呼出しは `FUN_004441A0` のみで、同関数は `iVar2+0xc` を投入前に deref するため obj=0 は到達困難 [推定]）/ `FUN_00482200` は **E8 caller 0 件**（間接 call 未特定）→ **NULL スロットの主源はウィンドウ配列の（再）確保時の NULL 充填 [部分確定]**（v324 §3-5 維持）。

---

## §4 項目4: `0xB45EEC` / `0xB45EF0` ゲートの意味（4ダンプ全0の正体）

### 4-1. 対象の実体 — **[確定]**

- **`.data` BSS**: VA0xB45EEC/EF0 → セクション内 offset 0x43EEC/0x43EF0 ≥ SizeOfRawData 0x30C00 → **raw 非包含 = 初期値 0（ローダ zero-fill）** [確定]（v327_4 BSS 判定。v327_4 初版の「file bytes 30a2f70d 等」は raw 超過後の隣接バイナリ誤読であり**誤り → 取り下げ**）。
- **4-dword 配列 `{0xB45EE4, 0xB45EE8, 0xB45EEC, 0xB45EF0}`**（v327_4b）:
  - 書き手: `FUN_0049a200` tail（`fstp [EEC]`@0x49A793 / `fstp [EF0]`@0x49A7B2・値 = **GetCellX/Y(slot(1,1)) << 12 を float 化** = cell 座標の12bit fixed-point）、帯外 `FUN_007dc9a0`（`mov [EE4],edx/[EE8],eax/[EEC],ecx/[EF0],edx`@0x7DCB73-0x7DCB88・int ビット直書き、docs L699025-6 = `obj+0x74/0x78` を float 化）、`FUN_007dcbd0`（`fstp [EE4]/[EE8]`@0x7DD027/0x7DD030）。
  - 読み手: **.text 内に disp32 読取りは0 件**。`0x7DB80E: push 0xb45ee4`（FUN_007db4b0・**アドレスを引数渡し**）の1件＋docs **INI/設定テーブル行 L697811 `("DepthOffset",…,&DAT_00b45ee4,…)`**（4-dword が設定バッファに結合されている示唆）＋docs 初期化 L698789-698792（4 連 =0）。
- ダンプ実測（v327_4/v327_5b、**v322_9 と独立一致**）: **EEC=0, EF0=0（4/4）**、DCC=0x18900A80×3 + 0x1894FC4C（heap ptr・4本目のみ相違）。

### 4-2. 「4 ダンプ全0 ＝ 当該コード未成功」の正体裁定 — **[部分確定]**（v324 §3-5 [確定] に幅を記す・削除せず併記）

- **[確定] クラッシュ実行の書込は未実行**: crash は 0x49A783（fstp の24バイト前）で AV → 当該 run では EEC/EF0 へ到達しない。直前の `0x49A75D fldz → [0xB44EE4]=0`（v322_9 で 4/4 = 0 一致）は**実行済み**であり、crash 直前までコードが走ったことの独立証拠。
- **[部分確定] セッション全体で一度も成功していない**、には2つの残存代替説明が排除できない: ①**過去の成功書込が cell (0,0)**（0<<12=0 → float 0.0f = ビット0）だった場合、②**`FUN_007dc9a0` 等のゼロ初期化/別書手が0 を書いた場合**。4/4 全0＋BSS 初期0 は「未書込」と整合するが実行時観測なし。
- **v324 訂正（併記）**: v324 §1-3/§3-5 の「全0＝当該コードが一度も成功していない [確定]」は、**crash-run 未実行 [確定] に限定して維持**し、「セッション未成功」は [部分確定] に格下げする（証拠は本節 4-1/4-2）。
- 語義: 「マップ中心座標（cell 座標 ×0x1000 fixed-point float）」（v324 語義を維持）＋ **4-dword 設定バッファ「DepthOffset」結合 [部分確定]**（docs L697811 の表行書式の厳密な解釈は §9 R2 へ）。

---

## §5 項目5: `ESI=0x30009990` / `EBX=0x30009A20` のアドレス系解析

### 5-1. 幾何 — **[確定]**（v327_5・全て検算 True）

| 検算 | 結果 |
|---|---|
| ESI = EAX = &window[1][1]（0x49A77F `mov esi,eax`、`FUN_00482150(1,1)` 戻り値） | 0x30009990 |
| grid base = ESI − (5*1+1)*8 = ESI − 0x30 | **0x30009960** |
| base + (5*4+4)*8 = base + 0xC0 == EBX? | **True**（EBX = loop B 最終スロット &window[4][4]・0x49A5E0 の残存値） |
| slot(1,1) offset = (5*1+1)*8 = 0x30 == ESI−base | True |
| EBX−ESI = 0x90 = 18 スロット × stride8 | True（v324 の (5*3+3)*8 と一致） |
| ウィンドウ配列 span = 25×8 = 0xC8 | [0x30009960, 0x30009A28) |
| FUN_00482150 実式 `base = [this+0x10], stride=N=[this+0xc]`（docs L104335） | this = [World+8] ウィンドウオブジェクト |
| page | 0x30009000 + 0x960（4KiB） |
| 画像外（image [0x400000,0xC05000)） | True |

### 5-2. ダンプ不在 — **[確定]**（パーサ交差検証済み）

- **grid base / ESI / EBX とも4本ダンプのいずれの memory range にも不在 [確定]**、どのモジュール（120 件・base 0x400000-0x774A0000）にも非包含 [確定]。
- パーサ検証: v327_5c で MINIDUMP_MEMORY_DESCRIPTOR の `[start u64][DataSize u32][Rva u32]` 順を確定（初版は1つ目 u32 を Rva と誤読 → **修正済み**）→ v327_5b で image globals が **v322_9 と4/4 完全一致**（EEC/EF0=0、DCC 一致）→ 不在判定の信頼性を確保。
- 補足: 0x30000000-0x3FFFFFFF 開始の range は 123/80/124/284 件存在（heap 頁が部分的に採取されている）が **0x30009xxx 頁は未採取**。

### 5-3. アイデンティティ — **[未確定] / [推定]**

- 0x30009960 = Windows プライベート heap 頁（画像外・モジュール外）だが、**mini dump に heap 不在のためアロケータ/所有オブジェクトの同定は不能 [未確定]**（window オブジェクトのヘッダ寸法・確保関数は未特定）。
- **4/4 で ESI/EBX 完全同一 = 実行毎に同一確保順・同一配置の決定的レイアウト [推定]**（heap dump なし）。
- 軽微な不一致: v324 §1-3 注記「Memory リスト 814〜980 range」vs 本 lane 実測 type5 count = **792/814/813/952**（集計基準の差 [未確定]・material 判定に影響なし）。

---

## §6 項目6: pid39788（14:17:37 落下）の事象未記載との突合

### 6-1. ダンプ再検証 — **[確定]**（v327_6、v324 §1-1/1-2 と一致）

| pid | size | eip | ECX | ESP | tid | プロセス生成 | user+kernel | dump mtime |
|---|---|---|---|---|---|---|---|---|
| 28964 | 39,788,433 | 0x4C9A80 | 0 | 0x1AF6F8 | 32780 | 14:47:34 | 237s+16s | 14:50:39 |
| 9936 | 39,836,750 | 0x4C9A80 | 0 | 0x1AF6F8 | 26212 | 14:42:06 | 9s+2s | 14:43:00 |
| **39788** | 39,844,387 | 0x4C9A80 | 0 | 0x1AF6F8 | 19852 | 14:15:34 | 120s+14s | **14:17:37** |
| 32488 | 40,174,081 | 0x4C9A80 | 0 | 0x1AF6F8 | 28300 | 13:57:09 | 1595s+164s | 14:13:51 |

- CrashDumps ディレクトリは **Oblivion*.dmp ちょうど4本のみ** [確定]。

### 6-2. イベントログ独立照会（読み取り専用）— **[確定]・4件目を独立確認**

- Windows Application ログ 2026-11-07 14:00-15:30 で Oblivion 該当 **EventID 1000（アプリケーション障害）が4件**: **14:13:46 / 14:17:33 / 14:42:57 / 14:50:35**、各1001（障害バケット 414526327・種類1）付き。全件 `Oblivion.exe、バージョン: 1.2.0.416、タイムスタンプ: 0x462392c7` [確定]。
- **14:17:33 の障害イベント ↔ dump 39788（mtime 14:17:37）** が対応 → **親の「3 事象」記録は不完全で、実際は4件 [確定]**（14:13:50 / 14:43:00 / 14:50:39 の3件＋未記載の14:17）。
- **事象件数の台帳更新（3→4）は親判断**（本 lane は報告のみ・ai-todo/台帳操作なし）。

---

## §7 項目7: L2 修正案 `0x49A781` ガードの正式パッチ設計（**提案のみ・L2 承認待ち・未適用**）

> **明記: 以下は設計提案書であり、バイナリへの書き換え・適用は一切行っていない（L1 で禁止・L2 承認後のみ）。コード片は仕様としてのみ記載。**

### 7-1. 設計入力となる検証済み事実（v327_7/7b/7c）

1. パッチ窓の正確な bytes（0x49A76A-0x49A7B7 全命令同定）: `8b15a033b300 8b4a08 83c408 6a01 6a01 e8d179feff 8bf0 8b0e e8f8f20200 c1e00c 8944241c df2c24 d91dec5eb400 8b0e e800f30200 c1e00c 833dcc5db40000 8944241c df2c24 d91df05eb400 0f8490000000`。
2. **`0x49A788` の命令は `C1 E0 0C`（shl eax,0xc）ちょうど3バイト** [確定]。
3. **`[0x49A77F,0x49A78B)` への直接分岐/呼び出しターゲット = 0 件**（numpy 走査: rel8/E8/E9/0F8x）[確定]。
4. **cave 候補 [0x49A8A3,0x49A8B0) = CC×13、rule v3 で証明済み dead**（ret@0x49A8A2 → CC13B → next head 0x49A8B0）。**外部分岐0 件**（候補 `src=0x49A926→0x49A8A8` は **偽陽性**: 0x49A926 は `mov edi,[0xa2807c]`(0x49A924) の disp32 内 byte、線形デコードで非命令境界 [確定]）。
5. **skip 先 0x49A7B8 → `je 0x49A84E`（flags は0x49A7A3 `cmp [0xB45DCC],0` 由来）→ epilogue 0x49A84E**。epilogue は先頭で `lea ecx,[esp+0x8c]`（ECX 再定義）、`mov esi,[esp+0x28]`（ESI 再定義）、EBX/EDI/EBP は pop で復元 → **入力レジスタのどれも読まれない [確定]**。
6. **`[0xB45DCC]` は4ダンプ全て非0**（0x18900A80×3 / 0x1894FC4C）→ crash 状態では元コードなら DCC ブロックを**実行する**はずだった（crash が先に発生）。
7. 兄弟ガード: `mov ecx,[reg]; test ecx,ecx; je/jz`（§1-2 符号）。

### 7-2. Design A（推奨・最小12B+13B、caller 側）

**置換窓 [0x49A77F,0x49A78B) = 12B（元: mov esi,eax(2)+mov ecx,[esi](2)+call(5)+shl(3) と正確に同長・死バイト0）＋ cave [0x49A8A3,0x49A8B0)=13B**:

```
; --- in place (12 B) ---
0x49A77F: 96                 xchg esi,eax        ; ESI=&slot（元 mov esi,eax と同義・フラグ不変）
0x49A780: 8B 0E              mov ecx,[esi]
0x49A782: 85 C9              test ecx,ecx         ; ZF=1 (NULL) / 0 (非NULL)
0x49A784: 74 32              je  0x49A7B8         ; rel8=0x32 → 0x49A786+0x32=0x49A7B8
0x49A786: E9 18 01 00 00     jmp 0x49A8A3         ; rel32=0x118
; --- cave (13 B, CC pad) ---
0x49A8A3: E8 D8 F1 02 00     call 0x4C9A80        ; rel32=0x2F1D8
0x49A8A8: C1 E0 0C           shl eax,0xc          ; 元 0x49A788 を再配置
0x49A8AB: E9 DB FE FF FF     jmp 0x49A78B         ; rel32=-0x125 → 元の継続先へ
```

- **検証（v327_7 in-memory disasm・ファイル非書込）**: patch/cave の長さが窓/pad に**厳密一致（True/True）**、jmp/call 目的値3件すべて期待一致、capstone 線形デコードで `je 0x49a7b8 / jmp 0x49a8a3 / call 0x4c9a80 / shl / jmp 0x49a78b` を確認。（※probe 内の je 検算行は `patch[5]`=opcode を読む off-by-one で誤出力 `0x49A7FA` → **capstone デコード `je 0x49a7b8` が正**。検算式の誤りを明記する。）
- **意味**: 非NULL時は元の処理（GetCellX→X 書込→GetCellY→Y 書込→DCC 判定）を**完全再現**。NULL 時は `je 0x49A7B8` → `je 0x49A84E`（ZF=1 で taken）→ **DCC 参照差し替えブロックをスキップして epilogue へ**。
- **既知の意味差（正直に記載）**: NULL 時に DCC ブロック（`*(0xB45DCC+0x10C)` の参照同期）を1回スキップする。**元コードは crash するため「元の挙動」は存在しない**（差分は「crash vs 非 crash 続行」）が、「本来なら実行されていたはずの参照同期が行われない」状態は残る。影響評価は [推定]（当該ブロックは参照コピー1件・stale 参照の次回成功実行で自己修正の可能性）。`[0xB45DCC]!=0` は4/4実測のため、**crash 状態ではこの差分は到達可能 [確定]**。
- レジスタ: xchg はフラグ不変・ESI を &slot 化（0x49A799 の `mov ecx,[esi]` で再利用）・EAX は旧 ESI（dead、call 直後に上書き）→ 元の流れと一致 [確定]。

### 7-3. Design B（DCC ブロックも実行する完全版）— **帯内 pad 不足で本 lane では非採用 [確定]**

- 必要: in-place に `mov/test/jnz-cave` 12B ＋ null 経路 `cmp [0xB45DCC],0(7)+jmp 0x49A7B8(5)` 12B＋dead1B、cave = call+shl+X書込序列+`jmp 0x49A799` = **27B**。
- 帯内 CC pad 最大は 14B（0x49A1C2）＋11B（0x49A1D5）＝合計25B＜27B、かつ両 pad の間 [0x49A1D0,0x49A1D5) は `E9→0x49A170` の**実命令**で結合不可 → **27B 連続/分割(hop ごと+5B)とも不足 [確定]**。帯外 pad 使用は帯規約違反のため提案対象外。

### 7-4. Design C（callee 側ガード・代替案）— **[推定]（前提検証を L2 前提条件として列挙）**

- `FUN_004c9a80`（GetCellX・深帯）は既に **NULL 系フォールバックパス `xor eax,eax; ret`（0x4C9A90）を内包**（bit1 セット or `[ecx+0x3C]==0` でreturn 0）。**最初の命令 `test [ecx+0x24],1` だけが ECX=NULL で AV**する。
- 案: 先頭に `jecxz`（E3 rel8）で既存 zero-return へ分岐。符号例（+2B・body 19→21B・pad 13→11B で吸収、全内部ターゲット再計算）:
  ```
  0x4C9A80: E3 10        jecxz 0x4C9A92
  0x4C9A82: F6 41 24 01  test byte [ecx+0x24],1
  0x4C9A86: 75 0A        jne  0x4C9A92
  0x4C9A88: 8B 41 3C     mov  eax,[ecx+0x3C]
  0x4C9A8B: 85 C0        test eax,eax
  0x4C9A8D: 74 03        je   0x4C9A92
  0x4C9A8F: 8B 00        mov  eax,[eax]
  0x4C9A91: C3           ret
  0x4C9A92: 33 C0        xor  eax,eax
  0x4C9A94: C3           ret
  ```
- **L2 前提検証（未実施）**: ①`[0x4C9A80,0x4C9A95)` への外部分岐ターゲット走査、②GetCellY 双子 `0x4C9AA0` への同梱、③92/94 caller が戻り0を安全に扱うかのレビュー。**92/94 caller 全体への影響拡大**があり Design A より blast radius 大 [推定]。
- 本項も**提案のみ・未適用（L2 承認待ち）**。

### 7-5. 採否推奨

- **Design A を第一候補**（窓/pad 厳密一致・検証済み・影響は crash 状態に限定）。Design C は callee統一性だが前提検証3件と caller レビューが別途必要。**いずれも L2 承認まで適用しない。**

---

## §8 判定サマリ

| # | 項目 | 主判定 | 内訳 |
|---|---|---|---|
| 1 | `FUN_0049a200` 詳細＋docs シグネチャ | **[確定]** | ECX this 受領・使用=確定 / docs void(void)=訂正対象=確定 / ループ構造=確定 |
| 2 | 他 caller 経路の完全性 | **[部分確定]** | const(1,1)=crash の2 call のみ=確定 / [this+0x30] 切り分け=部分確定 / インライン読=未確定(§9 R1) |
| 3 | 投入タイミング定量 | **[部分確定]** | チェーン census・同期1-pass・順序=確定 / 危険帯=「初回成功 pass まで（有効≤1F・アイドル無期限）」部分確定 / v324 §4-4 を補正併記 |
| 4 | `0xB45EEC/EF0` 意味 | **[部分確定]** | BSS 初期0・書手2系統・値=cell×0x1000 float=確定 / 全0=crash-run 未実行=確定・セッション未成功=部分確定（v324 §3-5 に幅を記す） |
| 5 | ESI/EBX アドレス系 | **[部分確定]** | 幾何・画像外・dump 不在=確定 / heap 同定=未確定 / 決定的配置=推定 |
| 6 | pid39788 突合 | **[確定]** | ダンプ再検証=確定 / **イベントログ4件目独立確認=確定** / 台帳更新=親判断 |
| 7 | 0x49A781 パッチ設計（提案） | **[確定]**（設計検証まで） | Design A bytes/cave 検証=確定・意味差=推定 / Design B=帯内不足で非採用=確定 / Design C=前提検証待ち=推定 / **適用は L2 承認待ち** |

- 5段階件数（主判定）: **[確定] 3（項目1,6,7）／[部分確定] 4（項目2,3,4,5）／[推定] 0／[未確定] 0／[確定負] 0**（副次判定に [推定] 6・[未確定] 4 を内包）。

### v324 への訂正・補強（削除せず併記）

- **C1（項目3）**: v324 §4-4「ロード直後・遷移直後（セルウィンドウ再構中）の数フレームが危険帯 [推定]」→ **投入は同期1-pass 全25スロット・遷移時は即時再構 → 危険帯は「初回成功 pass まで」（フレーム有効 ≤1 フレーム / アイドル無期限）に補正 [部分確定]**。
- **C2（項目4）**: v324 §1-3/§3-5「0xB45EEC/EF0 全0＝当該コードが一度も成功していない [確定]」→ **crash-run 未実行 [確定] を維持、「セッション未成功」は (0,0) セル/別書手の代替説明が残るため [部分確定] に格下げ**。

---

## §9 残留不明点（新規残件・次束以降へ）

1. **[未確定] R1: インライン slot(1,1) 読みの全数走査** — FUN_00482150 非経由の `base+0x30` 直接アドレスコード（92+94+40 site で拾えない残余）。
2. **[未確定] R2: map/DepthOffset 系の意味** — `FUN_007dc9a0` / `FUN_007dcbd0` / `FUN_007db4b0`（0x7D4xxx-0x7DCxxx・帯外）と docs INI 表行 L697811「DepthOffset」のバッファ結合（帯外のため浅層止まり）。
3. **[未確定] R3: 実行時遷移の観測** — `[0xB33A98]==0` 早期退出・`[World+0x74]` ゲート・cell 状態 {3,6} スキップの実際の発火序列（heap 依存・人間アクション待ち）。
4. **[未確定] R4: NULL スロットの直接経路** — 新規 World 生成（配列 NULL 充填）vs vtable+0x1C クリアの寄与比。`FUN_00482170(obj=0)` の E8 到達性は [推定]（困難）、`FUN_00482200` の間接 caller 未特定。
5. **[未確定] R5: docs vs EXE 不整合の反映** — ①WinMain call 未収録（4 vs 5）②`void FUN_0049a200(void)` の this 欠落 ③`FUN_004c9a80` docs 88 行 vs EXE 92 — 3件とも本 lane で再確認＝**訂正対象 [確定]**、docs 反映は親判断。
6. **[未確定] R6: Design C の L2 前提検証** — 0x4C9A80 窓への外部分岐走査・GetCellY 双子・caller の戻り0レビュー。
7. **[未確定] R7: v324 range 数「814〜980」vs 実測 792/814/813/952** の集計基準差。

---

## §10 自己検証

- **probe 一覧（18 本・すべて v327_ prefix・v322 は流用のみ）**: 1(FUN_0049a200 events/prologue/ECX/fn-end/CC) / 1b(ループ窓・[esp+0x18]・caller E8 修正・兄弟ガード) / 2(92サイト分類・indexer/Y census) / 2b(indexer args・[reg+0x30] origin・head 集計) / **2c(indexer args 正確化=const(1,1)全数・Y 側 via-idx)** / 3(投入チェーン census・FUN_00444FB0/00444C70) / 3b(pass 直前ゲート・origin 遷移 context) / 3c(je 0x44556B 同定・0xB33A69) / 3d(FUN_004430F0 全体・flag 解除) / 4(xref・BSS・dump 値) / 4b(4-dword 配列 xref・docs grep) / 5(幾何・range/module contain) / **5b(dump パーサ交差検証=v322_9 一致)** / 5c(デスクリプタ誤りの根因特定) / 6(ヘッダ再検証・イベントログ) / 7(パッチ窓・分岐走査・Design A 検算) / 7b(偽陽性解消・代替 pad・epilogue 安全性・第二書手) / 7c(callee body・Design C)。
- **内部整合**: ①E8 census 92=92（v327_2 再現 = v322_2_census 92 行）②docs head 56 関数集計=56 行 ③GetCellY 94・indexer 40・via-idx X=3/Y=3 ④dump globals が v322_9 と4/4一致（EEC/EF0=0、DCC 値一致）⑤幾何検算 base+0xC0==EBX True・(5*1+1)*8==0x30 True ⑥イベントログ4件 ↔ dump 4本の時刻対応 4/4 ⑦Design A の patch/cave 長=窓/pad 厳密一致 True・分岐目的3/3 一致。
- **検出し修正した自前の誤り（明記）**: (a) v327_1 の E8 検証を site 起点でなく ret 起点で読んで全件 False → v327_1b で site 起点に修正・5/5 True。(b) v327_2/2b の indexer args 抽出2種のバグ → v327_2c で修正（const(1,1)=1 件を確定）。(c) dump MemoryList の DataSize/Rva 順誤読 → v327_5c で特定・v327_4/5/5b 修正後に v322_9 と一致。(d) `MINIDUMP_MODULE.ModuleNameRva` を32→**20** に修正（v324 §8 既知の off-by-構造体型罠に再遭遇・自ら修正）。(e) BSS 非包含 VA の file 誤読 → BSS 判定に修正。(f) v327_7 の je 検算 off-by-one（opcode を rel8 と誤読）→ capstone デコードが正。(g) numpy2 の `int + np.int8` 昇格 OverflowError → struct.unpack に修正。(h) PowerShell 複数行 `python -c`（契約禁止）を1回使用 → 単一行/script 経由に是正。
- **帯遵守宣言**: 深解析は0x49A000-0x49B000 / 0x40D000-0x445000 / 0x4C0000-0x4E0000 のみ。帯外は census（地址）・caller 隣接浅層 window・浅層 xref・docs 参照に限定（0x7DCBxx 第二書手は ±0x30B の浅層 disasm）。**逸脱: なし**（イベントログ照会のみ帯外への読み取り専用照会として §0 に明記）。v318 自帯（0x405090/0x4050DA/0x4054AD）は一覧・VA・args のみで深追いなし。
- **禁止事項遵守**: ai-todo/TODO.md 操作なし、STATE/LOOP/docs/GT/ソース/git 編集なし、既存 `tmp/re/*.md` 編集なし、**バイナリ・ダンプ・実 .ess への書き込みなし**（全 probe 読み取りのみ・Design 検算はメモリ内 disasm）、成果物は本ファイル1点のみ新規。
- **Numba**: 不可（ModuleNotFoundError）→ numpy（xref・分岐走査）／素 Python（census・アドレス計算）にフォールバック。未使用理由: 短命 probe（0.1〜2 秒級）で JIT 初回数秒を回収不可、かつ capstone/bytes.find 主体の処理は JIT オーバーヘッドが上回る。
