# V258 — オブジェクト LOD 残点束（5件）（Oblivion.exe v1.2.0416 Steam）

- 出典: `tmp/re/v255_object_lod_structure.md`（残点 V-255-1..5 に対応・serial 250/V250 中）
- 手法: EXE 静的解析（capstone 5.0.7・selftest 4/4 PASS 着手時・pe_selftest T1..T4）＋ 実素材
  `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\distantlod\`（.lod 921 / .cmp 1）＋ BSA 索引（読取のみ）
- クリーンルーム: 外部逆コンパイル由来コード未参照。結論は EXE 命令・実データからのみ導出。
- 生データ: `%TEMP%\opencode\v258_{1,2,3,4}_out.txt`（スクリプト同所 `v258_*.py`）
- 関数対応（v255 からの継続同一性）: パーサ `0x4D16D0`・ローダ `0x4F1990`・タスク ctor `0x4BD430`・
  要求ループ `0x483D60`・.cmp 存在クエリ `0x4F0DA0`・kind predicate `0x4EF2D0`・.cmp ローダ `0x4F28B0`・
  **新規同定**: ws-mgr（TESWorldSpace 系）ctor `0x4F2A10`・ws-mgr LOD 初期化 `0x4F2C10`・
  cell 座標 commit `0x444EC0`→`0x444C70`→`0x444340`・land-LOD 再構築 `0x4EB0E0`/`0x4EBAE0`・
  settings dirty 消費者 `0x5BDCD0`（dirty フラグ `0xB3B740` test&clear `0x5DDCD0`）・ws-mgr 取得アクセサ `0x43F270`（`[B333A0+0x74]`）

## 件1 【最重要】.cmp key 41件の実体意味と mode==2 の発火条件

### 1-1. key の実体意味 = 「ランドマーク（稀少 STAT）セルのホワイトリスト」。存在索引ではない — [確定]

命令+データ双方の証拠:

1. **pack 規約**: .cmp ローダ `0x4F2972..0x4F2984` は読 u32 を `X=hi16(movsx) / Y=lo16(movzx)` のまま無変換で
   `+0xC8` NiTMap に insert。読手 `0x4F0DA0`（`0x4F0DA9 movsx edx,[esp+4]; shl 16 / 0x4F0DB3 movzx,[esp+0xc]; or`）も
   **hi16=X(signed)・lo16=Y** で照合。→ v255_20 の「40/41・(6,11) 孤児」は **x/y スワップ誤り**。
   本セッションで正規約で再照合し **41/41 が既存 `seworld_<x>_<y>.lod` ファイルに一致**（v258_1 F）。
2. **内容相関**（v258_3 E・`v255_lod_records.csv` 全 3202 レコード）:
   - key 41 セルに **exclusive formId 26 件**（非 key セルに一切現れない: `0x0003FFE6`×31, `0x0003FFF7`×22,
     `0x0003FFEF`, `0x00042AFF/B12`, `0x000419xx`, `0x000606xx`, `0x00068Fxx`, `0x0006FE97`, `0x0007F4AA` 他）。
     非 key セル 879 件は共通 18 formId（`0x0001AFEE/0x0001B015/0x00019F2C` 等の汎用 prop）のみ。
   - **rot 非ゼロは key セルにのみ存在**（`rot0_z`: cmp min=-3.14/max=3.14・other 全ゼロ）→ 方位固定のランドマーク物体。
   - key セルの rec/cell 平均 5.0 vs 3.4（多め）。
3. **形状**: 2 ブロブ = `{X∈9..12, Y∈2..6}` 15 件 と `{X∈-14..-8, Y∈-6..0}` 26 件（SI seworld 内で対角付近の
   2 地域、v258_1 keys 列挙）。921 の「全域索引」なら 921 key が必要なところ 41 のみ＝索引説は成立しない。
4. **.lod 存在はファイル名そのもの**で表現（open 失敗 `0x4BD59B test eax,eax → 0x4BD679` 無音中止）。
   [確定] の裁定: **.cmp = 「遠方帯（後述 kind2）にストリームしてよい特別セル（ランドマーク）集合」**。

### 1-2. mode==2 の正体と発火条件 — [確定]

- `0x4BD430`（タスク ctor・ret 0x1c=7 引数）の `0x4BD4C7 cmp [esp+0x154],2` のスロットは
  呼び出し側 `0x483D60@0x484194..0x4841A8` の push 列 `{ws mgr, x, y, &wsName(0xB34424), pathObj, prio01, kind}` の
  **最後 push（最高アドレス）= kind ∈ {1,2,4}**。同一スロットは `0x4BD531→task+0x24` に保存され、
  実行時 `0x4F1990` ディスパッチの mode 引数に回る。近接スロットの独立検証:
  `[esp+0x148]`→タスク+0x1C に文字列コピー＝worldspace 名（0x483E12-42 で埋まる）/
  `[esp+0x150]`→`[edx*4+0xA45A58]` の値 2/3＝タスク prio（v258_1 E の表実値）— 引数順序と整合。
- kind の発火元（`0x483D60` 内・命令レベル、v255_15/v258 追試）:
  - kind1: セル記録 `[esi+4]`（ロード済みセル対応オブジェクト 0xDC）あり・中心窓内（`0x483F26..0x483F30`）
  - kind4: 記録なし・中心窓内（新規要求。`0x483FF0→0x483F26` で ecx==0 経路 = 4）
  - **kind2**: (a) 遠方帯エッジ降格 — 窓外周バンド（グリッド index が中心 `uGridDistantCount`(=25・`[B06A90]`)
    ±`uGridsToLoad`(=5・`[B06A2C]`) 窓）該当で `0x484122..0x484129 mov ebp,2`・obj flags+0x18 bit1 set
    (b) stale pending 掃除 — 窓外 pending を cancel（`0x483FD9 call 0x4EF1D0 → 0x4B3A40 解放経路`）後 `0x484019 mov ebp,2`
    (c) 設定変換 — kind4 で `B09AE8==0 → kind1`（`0x484074`）/ `B09AE8!=0 && B09AF0==0 → kind2`（`0x484084`）
- **kind2 のときのみ .cmp 照合**: `0x484023 cmp ebp,2 → 0x484036 call 0x4F0DA0`（不在 → skip）と
  ctor 内 `0x4BD4C7→0x4BD4D5 call 0x4F0DA0`（false → `0x4BD6B0` タスク生成中止）。
  kind4 は .cmp hit 時に prio(far)=0 へ強制的に寄せる（`0x48416C..0x484188`）。kind2 は常に far=0（`0x484167→0x484188`）。
- **発火タイミング（ゲーム状態）**: 0x483D60 の caller は 3 系のみ（E8 全走査）:
  1. `0x444340`（"Starting world %08X %s with bounds (%i,%i) to (%i,%i)" `0xA37600`・`0x44493D`）—
     caller `0x444C70`@0x444D3C/0x444E42・`0x444FB0`@0x4454A1。`0x444C70` は `0x444EC0`（自セル座標 commit:
     `0x444F35/444F4E sar 0xC`＝**4096 単位=セルサイズの位置→セル index**、caller `0x40EE66`＝位置確定系/ロード/
     テレポート、`0x4455BF`）から。**＝セーブ/ロード/新規ゲームを問わず「ワールド空間内でセル座標が動くたび」に発火**。
     室内（`[esi+0x34]!=0`・`0x444C84`）は即 return。
  2. `0x4EB0E0(bool)`@0x4EB15A — land-LOD 再構築経路（同関数に `Data\Meshes\Landscape\LOD\%i.%02i.%02i.%i.NIF`
     `0xA47AE8`）。caller: `0x405CE0`（startup・"Initializing Sky..."）、コンソールハンドラ `0x508240`
     （"LOD land is now being displayed./disabled." `0xA4C20C/F0`＝ToggleLODLand 系）、`0x4EA160`、`0x4EBAE0`。
  3. `0x4EBAE0`@0x4EBB1D — 同上（`0x4EBB00` で 0x4EB0E0(1) を呼んだ後 0x483D60）。caller `0x5BDCD0`（設定 change）。
  → 裁定: **mode==2 は「特定メニュー操作」ではなく、セル移動/ワールド切替/settings 適用の要求ループ内で
  「遠方帯セル・掃除後セル」に自動的に立てられる要求種別**。実データで kind2 通過 = .cmp の 41 セルのみ。
- ws-mgr（TESWorldSpace 系）は `[0xB333A0]+0x74`（アクセサ `0x43F270`）。NULL（室内）時 `0x483DEA` で全走査中止。

### 1-3. 0x4EF2D0 の kind 実値と +0xDC マスク意味（seworld=7） — [確定]

- `0x4EF2D0`: `return ([mgr+0xDC] & kind) != 0`（3 命令）。kind 実値は呼び出し側 push 定数:
  - `0x483D60` 内: **1**（×3: 0x483F3A/483F5C/483F8C）・**2**（×3: 0x483F49/483F74/483FA5）。
    bit0=kind1 窓内要求許可・bit1=kind2 遠方帯要求許可。**両 bit 0 だと全セル skip**（`0x483F36..52` の初段 and）。
  - `0x4EA160`（ws 切替/update・caller `0x4EA6E0`→`0x4EBAE0` 連動）: **4**（0x4EA1D4/4EA1FE）—
    `mask&4==0` → `0x4EB0E0(0)` で **land LOD 無効化 + 0x4BDD40/0x4BDCD0 でフラッシュ**、
    `mask&4!=0` → land LOD 有効系（`0x4EB0E0(1)` 条件分岐・`0xB3608F` 現状態と `0xB02D70` 照合）。
  - `0x5BDCD0`（settings dirty `0xB3B740` 検知・`0x5DDCD0` test&clear。dirty setter: `0x5DDE85`(0x5DDE20)/`0x5DF09C`(0x5DEB80)）:
    **4**（0x5BDCFE）→ land LOD 再評価。
  → **[確定] trailer u32 の意味**: bit0/bit1 = オブジェクト LOD の 2 要求級（窓内/遠方帯）有効、
  **bit2 = その worldspace の Landscape(land) LOD 有効**。**seworld=7 = 3 機能全有効**。
- **デフォルト（.cmp 不在時）**: ws-mgr ctor `FUN_004F2A10`（callers 0x44874A/0x44B5D4/0x44E422＝フォーム生成帯）が
  `0x4F2B64 mov ebx,7 → 0x4F2BDC mov [esi+0xDC],7` で **+0xDC=7 を既定として書く**。
  `.cmp` ローダ失敗経路は +0xDC に触れない（`0x4F2923..28` で early out・`0x4F29A8` の +0xD8=1 は成功時のみ）。
  → **.cmp 不在 = マスク 7（全級許可）＋ +0xD8==0 → 0x4F0DA0 全許可** ＝「存在しない .cmp は実質全許可」で
  v255 の裁定が ctor レベルで成立。[確定]（seworld.cmp の 7 は既定値と同値＝データ上の差ゼロ）。
- 移植指針: 供給実データ（Meshes.bsa 系 55 worldspace 全て .cmp 同梱）ではマスク=7 と同然的な挙動。
  **移植実装は .cmp を読まず「全セル許可」でエンジン等価**（ランドマーク限定帯は描画品質差のみ、機能欠落なし）。
  忠実実装するなら「kind2=遠方帯ホワイトリスト + trailer bit2=land 有効」のみ。[確定]

## 件2 iDistantLODGroupWidth（既定 8・`0xB2C35C`）の実際の消費者 — **消費側を特定。LOD パーサ系参照 0 は再確認**

- **再確認 [確定]**: `.lod/.cmp` パーサ系（`0x4BD430`/`0x483D60`/`0x4F1990`/`0x4D16D0`/`0x4F28B0`/`0x4F0DA0`/`0x4EF2D0`）に
  `0xB2C35C` 参照命令なし（imm32 全セクション走査＋命令境界整合分類・v258_6 §1）。
  文字列 `0xA8D308` の参照は INI レコード `{value@B2C35C=8, nameptr@B2C360}`（.data）のみ（v255 継続）。
- **実際の消費者 = `FUN_007B3BE0`（Landscape（land）DistantLOD の NIF グループローダ）**:
  ```
  0x7B3C05  mov edi,[0xB2C35C]            ; GroupWidth=8
  0x7B3C0B  cmp edi,1 / jg                ; ≤1 は除算迂回（退路）
  0x7B3C14  ebx = [esp+0x2c]              ; packed key (hi16,lo16)＝セル座標パック
  0x7B3C24  movsx eax,hi16 → 0x7B3C28 idiv edi   ; X ÷ GroupWidth
  0x7B3C32  movsx eax,lo16 → 0x7B3C36 idiv edi   ; Y ÷ GroupWidth
  0x7B3C45/48 商を (blockX:i16<<16|blockY:u16) に再パック → ebp
  0x7B3C57  call 0x7B2AA0                 ; DistantLOD NIF ロード（0xA8D274 エラー経路本体）
  0x7B3C87  ecx=0xB2C33C（グローバル NiTMap）→ call 0x55E000（insert/find）
  ```
  → **意味 [確定]: 「land LOD NIF の 1 グループ＝8×8 セル」へのセル→ブロック変換幅**。
  所有者マップ `0xB2C33C` の操作側 = `0x7B3A40`（v255 で「タスク解放」と誤読——実は land LOD マップ操作）/
  `0x7B3BE0`/`0x7B3DB0`/`0x7B4010`（caller `0x7B4049`=0x7B3BE0 の caller 側）/`0xA27100`。
  オブジェクト LOD（.lod/.cmp）とは無関係の **Landscape LOD 系統専用設定**。
- その他参照 = `FUN_00A10E50`@0xA10E70 `push 0xB2C35C; ecx=0xB07BF0; call 0x4040E0` ＝
  **INI 自動保存登録**パターン（gateA/B の `0x9E33C0`/`0x9E3420` と同一形。終了時に INI へ値を戻す登録のみで消費ではない）。
  `0x9CDA81`/`0xA27121` のヒットはデータblob/命令境界外のパターン一致＝誤検出（v258_6 §1 で非整合確認）。
- 移植影響: ActiveBlock（ランドスケープ LOD）実装側は「セル座標÷8→ブロックキー」の対応を持つが、
  **オブジェクト LOD（Phase 65 の .lod ローダ）には GroupWidth は不要**と裁定 [確定]。

## 件3 fFadeDistance / `_objectlod` スライダー値（B09AE8/AF0）の適用点 — 命令レベル

### 3-1. fFadeDistance（`0xB06AB0`=12288.0f・name `0xB06AB4`）の適用点 = **FUN_004838F0（distant グリッド mgr 再構築）** — [確定]
```
0x483934-45  edi = uGridsToLoad(5) + 2×uGridDistantCount(25) = 55   ; 55×55 セルグリッド寸法（[mgr+0xC]）
0x483948/4D  [B34418]=25 / [B3441C]=15                              ; 中心 index / tree range
0x483A09..A51  [B34414] = float(uGridsToLoad<<12 ⇒×0.00586?→[0xA2FAA0] 倍) + float(uGridDistantCount<<12)
0x483A5D  fld dword [0xB06AB0]        ; ★fFadeDistance（12288.0）
0x483A65  fsubp                       ; [B34410] = [B34414] − fFadeDistance
0x483A75/7B  fstp [0xB2C334] / fstp [0xB2C338]     ; = fade near/far グローバル
```
- 消費者（命令境界整合で確定）: `FUN_00483590`（B34410/14 を読む小型 getter）、
  `FUN_00483D60` 末尾 `0x484256/6F/8C/96`（毎スキャンで文字照合設定ノード（`0x560920`・キー文字列 `0xB43484`＝**実行時生成 BSS**・内容静的不可 [未確定]）の
  float `[node+0x2c]/[+0x30]` を B34410/14 でクランプ → **B2C334/338 を上書き**）、
  **`FUN_00810F90`@0x811100 `push 0xB2C334`**（描画側＝フェード near を引数に渡す唯一の外部消費者・文脈 [推定]）。
- 裁定: **fFadeDistance は「描画距離そのもの」でなく「distant 窓外周からのフェード帯幅」**。
  距離基準 = uGridDistantCount×4096 単位系（`[B34414]`）、フェード開始 = `[B34414]−fFadeDistance`。
  パーサ/ストリーミング（.lod 読込）には影響せず、**LOD 描画のフェード計算専用** [確定・命令]。

### 3-2. gateA/B = `0xB09AE8`/`0xB09AF0`（video settings ID 9/10 の裏バイト）— 適用点一覧 [確定]
- 全参照（命令境界整合分類・v258_6）:
  | VA | 関数 | 作用 |
  |---|---|---|
  | `0x483DD4/483DDD` | FUN_00483D60 | **発火ゲート**: 両ゼロ→要求ループ即 return（`0x483DE4 je 0x48429A`）＝どちらか ON でないと .lod 要求生成ゼロ |
  | `0x484043/484057/48406B/48407B` | 同 | kind2 は A必須、kind1 は B必須、kind4 は A/B で 1⇔2 に降格（件1 §1-2 (c)） |
  | `0x5DED1C/0x5DED32` | FUN_005DEB80 | video メニュー生成：値を UI コントロール（`0x59B640`・ID 0x3FA/1018 系統）へ供給（表示側） |
  | `0x5DFC55..64/0x5DFCA6..B5` | FUN_005DF160 | **video settings 適用 dispatch**：switch ID 9（→A）/ID 10（→B）で `sete` 反転書換＋`0x404E10`（通知）＋ `[0xB3B744] |= 2/4` ＋ `0x579C10`（再適用予約） |
  | `0x5DFF36/0x5DFF3D` | 同 | **リセット/既定経路**：A=1,B=1（他 `0xB07090`,`0xB430AD` と連動）— .data 初期値も 1/1（v258_5 §.data） |
  | `0x9E33C0/0x9E3420` | FUN_009E33A0/009E3400 | INI 自動保存登録（`ecx=0xB07BF0; call 0x4040E0`）＝終了時 INI 書戻し。消費ではない |
  | `0xA1BD61/0xA1BD91` | データblob | 誤検出（関数外アドレス列挙） |
- **switch テーブル**: `0x5E012C`（u32×20）+`0x5E017C`（byte×20）。case8→`0x5DFC55`(ID9=A)/case9→`0x5DFCA6`(ID10=B)。
  ID→ラベル文字列の静的解決は video settings マネージャ側（`0xB147FC..` は `{float値, char*名前}` ペア 6 組＝リセット経路の float 群。
  名前 ptr `0xA6DD94/0xA6DDC8/0xA6DDE0/...` 未追跡）→ **A/B の UI ラベル名（Object LOD?/Tree LOD?）は [未確定]**。
- 依頼文の「`0x4F0E70` の B09AE8/AF0」は**命令実在なし**（その VA は NiTArray dtor グルー。参照ゼロ）— 誤り補正 [確定]。
- 移植指針 [推定]: `_objectlod` スライダー→エンジン 2 バイト対応は「A=distant 帯（kind2/.cmp 側）ON/OFF、B=近窓（kind1）ON/OFF」。
  スライダー値→バイトの離散化（4 段階等）は video settings 側（ID 9/10 の min/max/step）にあり本解析範囲外 **[未確定]**。

## 件4 rot の Euler 順 — 消費側（スロット配置は [確定]、行列組立関数は [未確定]）

### 4-1. エントリスロット対称性 — [確定]（命令レベル）
- .lod パーサ `0x4D16D0`（v255 §1.1 継続）: ファイルの float3#1(12n)→`[entry+0x14]`、float3#2(12n)→`[entry+0x04]`、float#3(4n)→`[entry+0x24]`、n→`[entry+0x30]`。
  entry ctor `0x4CC7C0` が配列3本のヘッダ（+0x00/+0x10/+0x20、data ptr +0x04/+0x14/+0x24、cap word +0x0E/+0x1E/+0x2E）を zero-init するのを再確認（v258_8）。
- **ESM 側ビルダ `0x4D43F0`@`0x4D46F9..0x4D4755` が同一クラスへ対称書き込み**（v258_16）:
  `NiTArray::Add 0x4D31D0` を `ecx=esi+0x10` へ 3 回（ワールド座標 xyz）→ pos 配列、
  `ecx=esi`(+0x00) へ 3 回（レコード float3 = `[esp+0x70/74/78]`）→ **rot 配列（data=[entry+0x04]）**、
  `ecx=esi+0x20` へ 1 回（スカラー `[esp+0x6c]`）→ scale 配列、`add dword [esi+0x30],1`（n++）。
  → 「ファイル 2 本目の float3 が entry+0x04 配列に入る」ことは両経路の命令で一致。[確定]
- **v255 §1.4 の量子化（×0.5→±0.97→fistp）は pos の方**（`0x4D45F2..0x4D4642` の fistp 列は
  `{0x60/64/68}` ではなく `player±record offset` 計算の丸め＝セルグリッド 0.5 単位量子化。
  実データ pos0_x=-40052.50 等が全部 .0/.5 で終わる根拠と一致・v255_lod_records.csv）。rot/scale は無変換 float。[確定]

### 4-2. Euler 順序 — [推定]
- コーパス 3,202 レコード全量で **rot スロット [0]/[1]=常に 0、[2] のみ非ゼロ**（値域 0, ±π/4, ±π/2, ±3π/4, ±π の 45° 倍数系・v258_3 rot0_z 分位＋v255_24）。
- file→配列→(同一クラスを共有する)消費側まで **ファイル出現順 (slot0,slot1,slot2) が保存**されるため（4-1）、
  行列組立がどの Euler 順序（ZYX 等）を仮定していようと、**出荷データでは slot2=Z 軸回転（yaw）以外ゼロ**であり順序論争は実務上観測不能。
- 移植実装指針 [推定→推奨]: `yaw = rot[i][2]`（ラジアン、Z 軸回転）として行列を組立。slot0/1 は無視可（非ゼロは全 corpus に存在しない）。

### 4-3. 最終行列組立関数の特定 — [未確定]（新規残点へ）
- `0x4D6760`(map find) の caller 8 件は form/cell/land 系のみで .lod エントリの描画組立は含まれず（v258_14）。
- ctx オブジェクト（task+0x2C、NiNode 派生・vft `0xA7E38C`・RTTI 名は MSVC TD 形式で取得不可（TD=0xABC204 が名 ptr 領域外を指す——COL 経由の virtual RTTI 構成 [未確定]））の子配列 +0xB0/+0xB6 への具現化経路が特定できなかった。
  sweep はオフセット 0x14/0x24/0x30 が汎用すぎて識別子にならない（v258_18 数百件）。
- 次の手（新残点 V-258-4a）: entry の 3 配列を読む唯一の関数は NiNode::Update 経由の `0x70xxxx` クラスバンド（vft 0xA7E38C の [8]..[12]）に在る仮説 → そのメソッド群 5 本を個別命令追跡。

## 件5 BSA per-file 圧縮ビット（.lod/.cmp） — 実測で裁定 [確定]

- 対象: `DLCShiveringIsles - Meshes.bsa`（v103・索引 922 件 = .lod 921 + seworld.cmp）と
  `Oblivion - Meshes.bsa`（v103・索引 9,944 件 = .lod 9,889 + .cmp 55）。索引レイアウトは v255_26 と同一走査、
  file record 16B = `{u64 nameHash, u32 f1, u32 f2}`。
- **field 役割の実測確定**（v258_20/21）: `f2 = データオフセット（素 u32、フラグなし）`、
  `f1 = 保存サイズ（実測 25bit で足りる値域）＋上位ビット`。
  根拠: SI rec[761] `f1=0x4000002F / f2=0x02AD8FA4(44,928,932)`、rec[762] のオフセット = rec[761] のオフセット+f1下位 →
  オフセット鎖が厳密成立。BASE `tamriel.cmp f1=0x172(370)` → f2 位置に 370B の Blob。
- **ペイロード形状 = `u32 展開後サイズ + zlib(78 9C) ストリーム`（圧縮保存）**:
  - SI: 922/922 件が `f2` 位置の先頭 4B = 展開後サイズ（seworld_-10_-1.lod: 40）、次の 2 ]byte = `78 9c`、
    `zlib.decompress(b[4:])` 成功かつサイズ一致 **922/922・bad 0**（v258_22）。
  - BASE: 9,944/9,944 件同形（zlibOK 9944・bad 0）。`seworld.cmp` 展開結果 == loose 168B 一致、
    `seworld_-10_-1.lod` 展開結果 == loose 40B 一致（v258_22）。
- **per-file フラグビットと圧縮の相関なし [確定]**: f1>>25 のヒストグラムは
  SI distantlod 全件 `32`（0x40000000=bit30）/ BASE distantlod 全件 `0`。
  **両アーカイブ全件が zlib 保存なのに BASE のフラグはゼロ** → bit30(0x40000000) は圧縮マーカーではない
  （v255 §7 の「0x787=圧縮ビット・0x703=非圧縮」連想も**ファイル単位では無効**——アーカイブレベル
  directory flags と per-file 実保存は別物。SI 0x703 相当のアーカイブでも .lod/.cmp は zlib）。
- **移植実装の要否判定**:
  1. loose/展開済み供給（現 `bsa_Extraction\distantlod\`）: 生 u32 配列・無圧縮で読込可（v255 継続）。
  2. **BSA 直読みを実装する場合 [確定]: フラグを見ず、常に「先頭 u32=展開後サイズ + 78 9C からの zlib 展開」を期待**。
     判別式は magic 検査が安全（無圧縮 .lod の先頭 u32=レコード count とは値域が重なるため、
     `b[4:6]==78 9C` で判定 → 展開、そうでなければ無圧縮歩行と解釈）。
  3. per-file 圧縮ビットによるフィルタは**不要**（実データで意味を持たないことが 19,000+ 件で反証済み）。

## 新規残点（V258）

- V-258-4a [中]: .lod エントリ（0x34・vft 0xA45AF0）を実描画ノードへ具現化する最終消費者（行列組立）未特定。
  vft `0xA7E38C`（0xDC NiNode 派生 ctx）メソッド群 0x70ACF0/0x70AD70/0x70B530/0x70B6A0/0x70BAE0/0x70B980 の
  個別追跡が次の手（v258_11 まで実施・ルーチンは子配列 +0xB0/+0xB6 走査まで確認）。
- V-258-1a [低]: video settings ID 9/10 の UI ラベル文字列（Object LOD/Tree LOD 対応）未解決
  （`0xB147FC..` 系は `{float, char*}` ペアで名前 ptr 0xA6DD94.. を指す——追跡可能）。
- V-258-3a [低]: 0x483D60 の band 幾何（B06A90=25/B06A2C=5/B06A98=15 の窓・降格条件 0x484089..0x484129 の
  上下非対称性）は命令追跡済みだが、要求 kind 分布の运行时裏取り（PC 版ログ）は未実施。
- V-258-5a [低]: 0x40000000（SI f1 bit30）の実意味（パス格納形式/世代フラグ?）は未確定。移植では無視可。

## 判定サマリ（5件）

| 件 | 判定 | 核心 |
|---|---|---|
| 1 | **[確定]** | .cmp key 41 = ランドマークセルホワイトリスト（41/41 実在・exclusive formId 26・rot 非ゼロは key 内のみ・2 クラスタ）。mode==2=kind2 は遠方帯/掃除要求のみ、.cmp hit 必須。trailer 7=bit0 kind1/bit1 kind2/bit2 地形LOD。既定 +0xDC=7（ctor `0x4F2BDC`）→ **.cmp 不在=全許可** |
| 2 | **[確定]** | GroupWidth の実消費者は `0x7B3BE0`（land LOD NIF ÷8 グルーピング）。LOD パーサ系参照 0 再確認。移植の .lod 側に不要 |
| 3 | **[確定]**(適用点)/[未確定](UI ラベル) | fFadeDistance は `0x4838F0` で B34410/14→B2C334/338（フェード帯）算出・`0x810F90` が消費。gateA/B(`B09AE8/AF0`)=settings ID 9/10 の反転トグル（`0x5DF160`）、既定 1。`0x4F0E70` 前提は誤り（参照ゼロ） |
| 4 | **[確定]**(スロット)/[推定](Euler)/[未確定](最終組立) | rot float3 = entry+0x04（ファイル2本目）、ESM 側 `0x4D46F9` 対称書込。corpus 全量 slot[2] のみ非ゼロ（45°倍数）→ yaw=slot2 で実装可 |
| 5 | **[確定]** | .lod/.cmp は両 BSA で **100% `u32 サイズ + zlib` 保存**。per-file 圧縮ビットは圧縮と無相関（flag 不要、magic 判定） |
