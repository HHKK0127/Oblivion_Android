# V266 — オブジェクト LOD 残点第2束（2件）（Oblivion.exe v1.2.0416 Steam）

- 出典: `tmp/re/v258_object_lod_residuals.md` 新規残点 V-258-3a / V-258-5a（serial 254/V254 低）
- 手法: EXE 静的解析（capstone 5.0.7・pe_selftest T1..T4 **4/4 PASS 着手時**）＋ BSA 索引読取（読取のみ）
  ＋ 実素材 `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\distantlod\`
- クリーンルーム: 外部逆コンパイル由来コード未参照。結論は EXE 命令・実データからのみ導出。
- 生データ: `%TEMP%\opencode\v266_{1,1b,1c,1d,1e,1f,2a,2c,2e,3,4b,5,6,7,8}_out.txt`（スクリプト同所 `v266_*.py`）
- 既存資産再利用: `v255_15_out.txt`（FUN_00483D60 全長 0x560B ダンプ）・`v258_1_out.txt`（.cmp 41 key 実測）・
  `v258_22.py`（BSA 索引走査関数）・`docs/Oblivion_decompiled.c` §FUN_00483D60/§FUN_004837C0 と突合

## 件1 V-258-3a 要求ループの外周 band 幾何（25/5/15 降格条件の境界式） — **[確定]**（运行时分布は実データ proxy で代替）

### 1-1. ループの枠（実測・命令レベル）

FUN_00483D60（要求スキャン・`this`=distant グリッド mgr `[esp+0x20]`）:

- グリッド寸法: `FUN_004838F0@0x483945 lea edi,[edx+eax*2]` → **dim = uGridsToLoad + 2·uGridDistantCount
  = 5 + 50 = 55**、`[mgr+0xC]=55`・スロット 16B（`shl eax,4` @0x483EB3 実測）× 55²。
- 二重ループ: 外側 `edi`(i=row) / 内側 `[esp+0x14]`(j=col) とも `0..dim-1`（境界 `0x483E64..6E`・
  `0x4841C4 cmp ecx,[mgr+0xC]`・`0x4841E2 cmp edi,[mgr+0xC]`）。
- **Δ counters**: `[esp+0x1c]` = 行 Δ、`[esp+0x18]` = 列 Δ。初期値 `c = [B34418] + (D>>1) = G + h`
  （`0x483E4C..5B`: `mov eax,[B06A2C]; shr eax,1; add ecx(=[B34418]),eax`）、1 ステップごとに **デクリメント**
  （内側 `0x4841BF sub dword[esp+0x18],1`（列）/ 外側 `0x4841DA sub dword[esp+0x1c],1`（行）。ループ上限比較は
  `0x4841BC mov eax,[mgr+0xc]` → `0x4841C7 cmp ecx,eax` / `0x4841E2 cmp edi,eax`）
  → **Δ = c − index = プレイヤーセル起点の符号付きセル距離**。
  出発点のセル座標は `FUN_00483750` が slot+8/+0xC へ書込み（`[mgr+4]/[mgr+8] − half`・0x48378A..0x483796）。
- `[esp+0x24] = h = D>>1 = 2`（0x483E5B）。

### 1-2. 窓/帯の境界式（3 箇所の band テストは同一式・実測）

**W（中心窓）条件 = `G ≤ index < G+D`（row・col 両軸）⇔ `|Δx| ≤ 2 かつ |Δy| ≤ 2` ⇔ 5×5**。
同一命令列が 3 箇所（`0x483EB9..0xED0` / `0x484089..0x40A8` / `0x484105..0x4120`、imm 直読み `[B06A90]`・`[B06A2C]`）。

- **T box（tree-range 窓）**: `FUN_004837C0@0x4837C7: R = (D>>1) + [B3441C] = h + T = 2 + 15 = 17`、
  判定は `R ≥ |Δx|`（fcom `test ah,5 jp`）→ `R ≥ |Δy|`（fcompp `test ah,0x41 je`）。
  **両軸とも inclusive（|Δ| ≤ 17）＝上下非対称なし**（Ghidra decompile §FUN_004837c0 と一致確認）。
- .lod パーサ要求の出生条件はこの T box（**中心窓ではない** — v258 §1-2 の「中心窓内」表記は誤り、§1-3 参照）。

### 1-3. kind 出生・降格・ prio の完全分岐（各 VA 実測）

```
出生（rec=[slot+4] の 0xDC レコード、req=[slot+1]、pending=[slot+0]）:
  rec∧¬req∧T  -> kind1 @0x483F26..30（neg/sbb/and/add ecx,4 イディオム ecx≠0→1）+ req=1 @0x483F32
  ¬rec∧  T    -> kind4 @0x483FF0→0x483F26（ecx==0→4）+ req=1
  ¬rec∧ ¬T    -> kind2 @0x484019（req ありなら 0x484007 call 0x4EF1D0(pack key)+0x483FDF 0x7B3A40 で掃除後）
  rec∧req∧¬T  -> cancel @0x483FBF..0x483FEB -> drop
  rec∧¬req∧¬T -> drop @0x483FC7（0x4841B4）
mask 前判定（0x483F36..0x483FB5・0x4EF2D0=[mgr_ws+0xDC]&kind）:
  bit0=0 ∧ kind1 -> drop ・ bit1=0 ∧ kind2 -> drop
  bit0=0 ∧ kind4 -> kind2 @0x483F9C ・ bit1=0 ∧ kind4 -> kind1 @0x483FB5
ゲート（0x484023..0x484084）:
  kind2: .cmp hit 必須 @0x484036 call 0x4F0DA0（false→drop）＋ gateA [B09AE8]!=0 @0x484043
  kind1: gateB [B09AF0]!=0 @0x484057
  kind4: ¬A→1 @0x484074 / A∧¬B→2 @0x484084（変換後は .cmp 未照合・ctor 0x4BD4C7 で照合）
降格/掃除:
  W ∧ kind1         -> drop @0x4840AA..AD（再読込要求は窓内では不要）
  W ∧ kind∈{2,4}    -> rec+0x18 |= 0x0001 @0x484124・kind=2 @0x484129   ← **0x484105..129 は「外周 band」でなく中心窓そのもの**（v258 §1-2(a) の誤り訂正）
  W ∧ pending(byte0) -> cancel 経路 @0x483ED2..0x483FEB（窓内に戻った保留セルの掃除。rec+0x18 |= 0x0001 @0x483EDE）
prio（0x48412E..0x484188）: d = max(|Δx|,|Δy|)（cdq/xor/sub 絶対値×2・cmp/jbe @0x484142）
  idx = 0 (d ≤ h+2=4) / 1 (h+2 < d ≤ h+4=6) / 2 (d > 6)   ; sbb/neg/add @0x484160..164
  kind2 -> idx=0 強制 @0x484167..16A→0x484188 ・ kind4 ∧ .cmp hit -> idx=0 強制 @0x48416C..17F
  タスク prio 実値 = [0xA45A58 + 4·idx] = {2,3,4}（実測ダンプ: `02 00 00 00 | 03 00 00 00 | 04 00 00 00`・ctor スロット [esp+0x150]→task+0x1C）
```

### 1-4. band 幅の閉形式（裁定 **[確定]**）

世界既定 G=25・D=5・T=15・h=2 のとき、半径 d（Chebyshev・プレイヤーセル起点）で：

| 帯 | 半径区間 | 幅 | 内容 | kind | .cmp 必要 | prio |
|---|---|---|---|---|---|---|
| 中心窓 W | `0 ≤ d ≤ 2` | D (=5) | kind1 は drop・kind2/4 は **kind2 降格** | 2 | ○（loop+ctor） | 0 強制 |
| 近傍帯 | `3 ≤ d ≤ 17` | T (=15) | 通常要求（kind1/4・再読込/新規） | 1/4 | ×（hit 時 prio0 のみ） | idx: d≤4→0・5–6→1・7–17→2（タスク値 2/3/4） |
| **外周 band** | `18 ≤ d ≤ 27` | **G − T = 10** | 遠方降格帯（T box 外=kind2 出生） | 2 | **○** | 0 強制 |
| グリッド外 | `d > 27` | — | ループ対象外（dim=55 の境界） | — | — | — |

- **band 境界式: 下限 `(D>>1) + T + 1 = 18`（= h+T+1）・上限 `G + (D>>1) = 27`（= grid edge）**。
  面積 = 55² − 35² = 3025 − 1225 = **1800 セル**。kind1 の窓内 drop とあわせ「25/5/15 の非対称」は
  **中心窓（±2=5 由来）と T box（±17=15 由来）が別々の降格/出生地点として直列評価されること**で発生。
  片方の式ではない（外周 band 降格=出生 kind2@0x484019、中心窓降格=0x484129 の 2 経路）。
- 市街地モード（`[B34420]!=0`→`[B06AA0]=4`/`[B06AA8]=4`・`0x483D96..0xA7` で B34418/1C 書換）: dim=55 のまま、
  band/降格テスト 3 箇所は `[B06A90]` 直読み＝**index窓 [25..30) 固定**（0x483EAB/0x484089/0x484105 実測）で、
  出生起点 c = `[B34418]+h = 6`・T 半径 = `[B3441C]+h = 6`（`0x4837D3..D5`）のみ市街値に縮む。
  → T inside = index 0..12（|Δ|≤6）で **index窓 [25..30) と重ならず**、窓降格（0x484129）は T 外（既に kind2 出生）
  セルに発火する=設計上非整合。実務上は都市 worldspace への .lod 供給が存在しないため open 失敗で無音中止
  （`0x4BD59B`）→ 観測不能 [推定]。band テストに市街値を使う命令経路は存在しない（[確定]）。

### 1-5. 运行时裏取り（PC 版ログ非所有のため実データ proxy・[確定]相当）

- v255_lod_records.csv（seworld 921 .lod・3202 レコード）: cell 範囲 X −16..21・Y −19..22（38×42=1596 可能性に対し
  921 のみ実在＝**地域輪郭**）。外周 band（半径 18..27 の 55×55 角丸枠）は地域を跨ぐため、band 内の .lod 実在は
  プレイヤー位置依存。Chebyshev(|cell|) ヒストグラムは 0..22 が単峰（v266_1_out F）で band 幾何と矛盾なし。
- .cmp 41 key（v258_1 §F 実測・hi16=X signed/lo16=Y）: X∈[−14..12]・Y∈[−6..6] の 2 ブロブ。
  プレイヤーが seworld 入口付近（例 (0,0)）だと key の d=9..17 → **近傍帯**（kind4・.cmp 不要、hit 時は prio0 化のみ）。
  プレイヤーが遠い位置（例 (21,22)・key (9,3): Δx=−12, Δy=−19 → d=19）だと **外周 band** に落ちて .cmp ホワイトリストが
  発火 — 「kind2=.cmp hit 必須」設計（v258 件1）と band 式が运行时に整合して機能する構図 [確定]。
- 新規残点 V-258-3a の「上下非対称」は命令上存在せず（T box・W とも両軸 inclusive 対称。A3/A4 のジャンプ先
  分岐差 `jb 0x4840B9` vs `jb 0x4840B5` は同一条件の経路差で結果不変。Ghidra §FUN_00483d60 の C 式
  `(i<G || G+D<=i || j<G || G+D<=j || ...)` とも一致）。**非対称の正体 = 窓(D)と木窓(T)の 2 降格経路の重ね合わせ** [確定]。

### 1-6. v258 からの訂正点（本束の実測による）

1. **v258 §1-2(a)「遠方帯エッジ降格 — 窓外周バンド該当で 0x484122..0x484129 mov ebp,2」→ 誤**。
   0x484105..129 の条件は **中心窓内部**（G ≤ i,j < G+D ⇔ |Δ| ≤ 2）。外周 band の kind2 は 0x484019（T box 外出生）が本体。
2. **同「obj flags+0x18 bit1 set」→ 誤（bit0・値 0x0001）**。`or word ptr [eax+0x18],1`（0x483EDE/0x484124）、
   消側は `and ...,0xFFFE`（0x483EFC）。
3. **v258 §1-2 kind1/kind4 の「中心窓内」→ 正しくは T box 内（|Δ| ≤ 17）**。中心窓（|Δ| ≤ 2）は drop/降格専用領域。

### 1-7. band 全区画の kind/prio カウント（55×55=3025 スロット・静的状態=記録なし/未要求/cmp 不問）

`v266_1f.py` 実測（出生関数を 1-3 の命令列どおり再現・Ghidra §FUN_00483d60 の C 式と逐語突合済み）:

| 区画 | セル数 | 出生 kind | .cmp hit 時 | prio idx→タスク値 |
|---|---:|---|---|---|
| 中心窓 d≤2 | 25 | kind4→**W 降格 kind2** | kind2 通過 | 0 強制→2 |
| 近傍帯 d=3..4 | 56 | kind4 | prio0 化 | 0→2 |
| 近傍帯 d=5..6 | 88 | kind4 | prio0 化 | 1→3 |
| 近傍帯 d=7..17 | 1056 | kind4 | prio0 化 | 2→4 |
| **外周 band d=18..27** | **1800** | **kind2（0x484019）** | **通過（false→drop）** | 0 強制→2 |

- 降格・掃除地点（中心窓）と降格出生地点（外周 band）の **band 幅 = G − T = 25 − 15 = 10（両軸）**、
  近傍帯（kind1/4 領域）幅 = T = 15、中心窓幅 = D = 5。グリッド半辺 = G + (D>>1) = 27 = h(2) + T(15) + band(10)
  （中心窓半幅 h、近傍帯 15、外周 band 10 の連続区画）で整合確認。
- gateA/B・mask 変体（同一 55×55・.cmp 疑似 hit）: A0→外周 band 全 drop（kind2 消滅）、B0→kind4 が全て kind2 へ
  変換され .cmp 通過分のみ残る、mask=4（bit0/1 both 0）→ 要求ゼロ。既定動作（A1B1 mask7）と band 幾何の独立性を確認。
- 実行時分布の裏取り proxy（.cmp 41 key × プレイヤー位置・`v266_1f` §keys-vs-band）:
  | プレイヤーセル | 窓内 | 近傍帯 | 外周 band（.cmp 必須） | グリッド外 |
  |---|---:|---:|---:|---:|
  | (0,0) | 0 | 41 | 0 | 0 |
  | (10,4) | 15 | 0 | 26 | 0 |
  | (-10,-16) | 0 | 26 | 15 | 0 |
  | (0,20) | 0 | 13 | 28 | 0 |
  | (21,22) | 0 | 6 | 9 | 26 |
  → 「ランドマークがホワイトリスト経由で遠方ストリームされる」のはプレイヤーがブロブから 18..27 セル離れたときで、
  近距離では kind4 経路（prio0 化）に回る。**25/5/15 の非対称降格条件 = 2 直列窓（W と T box）の境界 18/27 で確定 [確定]**。
- 运行时 PC ログ裏取りは本環境（PC 版 Oblivion 非所有・実行禁止）のため代替実施。命令+静的データからの band 式確定は上記のとおり **[確定]**、
  ログによる kind 分布の実測確認のみ **[未確定]**（残点化なし — 式は命令から完全導出済み）。

## 件2 V-258-5a BSA f1 bit30（0x40000000）の実意味 — **[確定]**（= アーカイブ既定と XOR の per-file 圧縮反転ビット）

### 2-1. 全 17 BSA × 全ファイルの f1 bit30 拡張子・相関分布（集計実測 `v266_2c/2e`）

- 対象: `Data\` の全 17 .bsa（v103）合計 147,629 ファイルレコード `{u64 hash, u32 f1, u32 f2}`。
- **f1 上位ビットは bit30 のみ使用**（bit25..29・bit31 は 17 アーカイブ全件で Set 0・`v266_6` §1/§2）。
  f2 の bit29/30 Set は `Oblivion - Meshes.bsa`（14572 件・オフセット 0x2001_761C..）等の実在オフセット値であり
  フラグではない（bit31 は 0 個＝エンジンが「無効レコード」判定に使う bit31 と両立）。
- bit30 Set 総計 5,221 件の内訳: SI Meshes 3014 / SI Textures 1869 / Knights 217 / Battlehorn 60 / VileLair 18 /
  ThievesDen 13 / Orrery 12 / Frostcrag 10 / HorseArmor 8 / BASE Meshes **0**。
- **拡張子分布（SI Meshes・要求文の対象アーカイブ）**: bit30 Set = .nif 1438・.lod 921・.kf 614・.spt 26・.egm 13・.tri 1・**.cmp 1**（= 全拡張子 100% Set）、
  唯一の非 Set は **.obj 3 件**（sezealot の Wavefront `upperbody/hand/arms.obj`・先頭バイトが平文 `v 20 30 2e 30...`＝圧縮なし）。
  → 「SI 専用」は誤りで **.obj 非圧縮分を除く SI Meshes 全件**、さらに **Textures/他 DLC でも圧縮ファイルに立っている**。

### 2-2. 実意味 = 「アーカイブ既定圧縮フラグとの XOR による展開経路選択」 — 命令レベル [確定]

BSA ファイルオープン系 `FUN_0042E070`/`FUN_0042E1A0`（v103 record ペア読み・Ghidra §FUN_0042e070 と一致）:

```
0x42E0EA  mov  edx,[edi+0x160]   ; アーカイブヘッダ flags u32（実行時ロード値・= 0x703/0x787 等）
0x42E0F0  mov  eax,[esi+8]       ; f1（本ファイルレコード）
0x42E0F3  shr  edx, 2            ; bit2 = アーカイブ既定「圧縮」(0x4)
0x42E0F6  shr  eax, 0x1e         ; bit30 = per-file 反転
0x42E0F9  xor  dl, al            ; ★ XOR
0x42E0FB  test dl,1 → jne 0x42E138
          =0（同じ）→ FUN_0042D540（stream オブジェクト 0x15C・raw 読み）
          ≠0（違う）→ FUN_0042D6D0（stream オブジェクト 0x174・内部 call 0x742490/0x743970
                       = zlib inflate — .rdata エラー文字列 'incorrect header check'@0xA826B0/
                       'invalid distance too far back'@0xA82568 を [stream+0x18] に設定する命令が
                       0x74276C/0x7435F4/0x74542E・zlib 組込み確認）
0x42E125/161  and esi, 0x3FFFFFFF ; 展開後サイズ = f1 下位 **30bit**（v258 の 25bit は値域の実測であってマスクではない）
0x42E0DE      and ebx, 0x7FFFFFFF ; f2 = データオフセット・bit31 Set は無効レコード扱い
```

- **判定規則: `圧縮 = f1.bit30 XOR (bsaヘッダflags & 0x4)`**（展開時は zlib ヘッダなし・`u32 展開後サイズ + deflate ストリーム`＝
  v258 件5 のペイロード形状と同一。78 9C マジックはヘッダ付き保存の実装上の副産物で、エンジン判定はマジック非参照）。
- **全件交差表（`v266_2e`・各ファイルのオフセット先 6B マジック vs bit30・全 17 アーカイブ 147,629 ファイル総当り）**:
  `予測圧縮 == 実 zlib` の一致率 **100.00%**。外れ 36 件はすべてマジック・ヒューリスティック側の人工物で、
  規則の反証は **ゼロ**（0 バイト保存 'tiny' 35 件＝先頭読取不能、`Oblivion - Sounds` の 1 件は非圧縮 WAV の
  `RIFF` チャンクサイズ `78 01 01 00` が `78 9C`族に誤マッチ・`v266_8` §outlier 実データ確認）。
  例: `Oblivion - Textures - Compressed.bsa`（hdr 0x707 bit2=1・bit30=0）→ XOR=1 → zlib ✓（18005/18005）。
  `Oblivion - Meshes.bsa`（0x787 bit2=1・bit30=0）→ zlib ✓ 20182/20182。**SI Meshes（0x703 bit2=0・bit30=3014）→ zlib ✓ 3014/3014、
  .obj 3 件のみ XOR=0 → raw ✓**。Voices/Sounds（0x703・bit30=0）→ raw ✓ 47076/47076。
  Knights の DDS/NIF 217 件（bit30=1・アーカイブ既定 0）→ zlib ✓、他 4593 件 raw ✓。
- v258 件5 の「per-file フラグと圧縮は**無相関**」は **単一アーカイブ内（既定フラグ固定域）の話**としてのみ成立。
  全アーカイブでは上記 XOR で完全相関 → **訂正 [確定]**。「SI f1 bit30 は圧縮マーカーではない」は
  「BASE 側の .lod も zlib なのに bit30=0」というアーカイブ既定の差を見落とした帰納で、ビット意味自体は圧縮系。
- v255 §7 の旧連想（0x787=圧縮アーカイブ）も**アーカイブレベルでは正しい**（bit2 がそれ）。per-file bit30 はその反転。
- 拡張子特異性なし（.lod/.nif/.kf/.dds/.wav/.mp3/.lip/.cmp 全て規則内）。パス格納形式/世代フラグ説は
  全件交差表で否定 [確定]。

### 2-3. 移植裁定 — 「無視可」の最終確認

- **loose/展開済み供給（`bsa_Extraction\distantlod\`・現行 Phase 65 入力）**: 完全に無視可 [確定]（ビットは索引メタのみ）。
- **BSA 直読みを実装する場合**: 判定式は **ヘッダ flags bit2 と f1 bit30 の XOR**（2-2）。
  v258 の「magic `78 9C` 判定で代替可」は distantlod 供給全件で動作するが（実保存が zlib ヘッダ付きのため）、
  Enginesemantics とは独立経路。どちらでもこの供給では同一結果 — **移植実装は bit30 を読まず
  「u32 サイズ + inflate or raw」を XOR 式（推奨・正確）または magic 検査（実質等価）で選べばよい** [確定]。
  無視した場合の破綻は .obj 3 件型（非圧縮保存）のみで distantlod/.lod/.cmp 供給に存在せず（実測 0 件）。
- 「相関のある他フラグビット」: **無し**（bit25..29/31 は全ファイル 0、f2 上位はオフセット値域）。XOR 相手が
  アーカイブヘッダの別格納（レコード内ビットではない）である点が v258 の見落とし [確定]。

## 判定サマリ（2件）

| 件 | 判定 | 核心 |
|---|---|---|
| 1 (V-258-3a) | **[確定]** | band 幾何 = 3 箇所の同一窓テスト（`G≤idx<G+D`⇔\|Δ\|≤2）+ T box（\|Δ\|≤h+T=17・両軸 inclusive 対称）。中心窓は kind1 drop/kind2・4 降格（0x4840AD/0x484129）、外周 band（18≤d≤27・**幅 G−T=10**・1800 セル）は kind2 出生（0x484019）で .cmp 必須。prio idx={0:d≤4,1:d≤6,2:d>6}→タスク値 {2,3,4}（表 0xA45A58 実測）。v258 §1-2 の降格場所/フラグ bit/「中心窓」表記 3 点訂正 |
| 2 (V-258-5a) | **[確定]** | f1 bit30 = per-file 圧縮反転ビット。**展開経路 = bit30 XOR (ヘッダ flags bit2)**（`0x42E0F3..0x42E0FB`・FUN_0042D540 raw / FUN_0042D6D0 inflate=0x742490/0x743970 zlib 組込み）。全 17 BSA 147,629 件で 100% 一致（v258 の「無相関」は単一アーカイブ限定・訂正）。サイズマスクは 0x3FFFFFFF（30bit）。移植: loose 供給では無視可、BSA 直読みでは XOR 式（magic 検査は実質等価） |

## 新規残点（V266）

- V-266-1a [低]: kind 分布の运行时ログ実測（PC 版実行環境が必要。静的導出は完了済みで移植実装上は不要）。
- V-266-2a [低]: f2 bit31（エンジン読取側で `and 0x7FFFFFFF`＋bit31 無効判定・`FUN_0042E070`@0x42E0CE..DE）を
  立てる packed-索引モード（`[obj+0x194]&8`・設定側 `FUN_0042EE80`@0x42EF6B`mov byte[esi+0x194],8`）は本束範囲外。
  出荷 17 BSA 全件で f2 bit31=0・0x194=8 経路は .bsa ヘッダ読みモードのみで distantlod 供給では発火せず
  （`v266_6` §1 bit31=0 個）→ 移植影響なし [推定]。
