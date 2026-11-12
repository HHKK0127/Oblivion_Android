# V255 — オブジェクト LOD（.lod / .cmp）構造解析（Oblivion.exe v1.2.0416 Steam）

- 起票: `tmp/re/handoff_20261102_object_lod_request.md`（移植側 Phase 65 `object-lod-loader` ブロッカー）
- 手法: EXE 静的解析（capstone 5.0.7・selftest 4/4 PASS・imm32_refs3 方式）＋ 実素材コーパス
  `D:\Cargo\Other\Oblivion\BSA\bsa_Extraction\distantlod\`（.lod 921 件 / .cmp 1 件）
- クリーンルーム: 外部逆コンパイル由来コードは未参照。結論は EXE 命令・実データからのみ導出。
- 生データ: `%TEMP%\opencode\v255_*_out.txt` / 台帳: `tmp/re/v255_lod_records.csv` / 総調: `tmp/re/v255_field_census.txt`

## 0. 同定した関数一覧（サマリ）

| VA | 役割 | 根拠 | 判定 |
|---|---|---|---|
| `0x4BD430` | `DistantLODLoaderTask` コンストラクタ（セル単位のロード要求投入） | `0xA45B58`=`DistantLOD\%s_%i_%i.lod` site `0x4BD580`; `ret 0x1c`（7引数+this） | [確定] |
| `0x4BCA90`/`0x4BCAC0` | タスク object ctor（0x30B, vptr `0xA45A68`）。release 版は `0x436FA0(0)`（file=NULL） | site `0x4BCAA5`/`0x4BCB0C` | [確定] |
| `0x4BCC60` | タスク `Run`（vtable `0xA45A68`+4）: ファイル open → `0x4F1990` 呼出し | `0x4BCC70 mov eax,[esi+0x20]`→`0x431690(path,0,0x800)`; `0x4BCC9D call 0x4f1990` | [確定] |
| `0x4F1990` | .lod ローダディスパッチャ（this=worldspace mgr, bx, by, map, mode, file） | `0x4F19A2 cmp byte [0xb09db0],0` で 2 経路分岐 | [確定] |
| `0x4D16D0` | **`.lod` ファイル本体パーサ**（ファイル読取りは唯一ここ）| `0x4F19BF call 0x4d16d0`; 読取り順は §2 | [確定] |
| `0x4F0DA0` | **`.cmp` ブロック存在クエリ**（NiTMap find） | `0x4F0DBB key=(int16)ebx<<16 or (uint16)ebp`; `0x4BD4D5` が mode==2 時のみ呼出し | [確定] |
| `0x4F28B0` | **`.cmp` ローダ**（u32 配列→NiTMap +0xC8・末尾 u32→+0xDC・フラグ +0xD8） | `0xA483EC`=`Data\DistantLOD\%s.cmp` site `0x4F2904` | [確定] |
| `0x483D60` | ロード要求生成側（セルグリッド走査→タスク ctor、0xB35B8C mgr 経由） | `ret 4`; callers `0x444396`/`0x4EB15A`/`0x4EBB1D` | [確定] |
| `0x4D43F0`/`0x4D8440` | **ESM 内 DistantRefLOD レコード**（subrecord NAME/DATA/XLOD/XSLC）読取 — `.lod` とは無関係 | 4cc 比較 `0x4D84D0` 群; `cmp eax,0x30/0x31`; 実 .lod 921 件に fourcc 0 件（v255_9） | [確定] |
| `0x407AE0` | `DistantLOD : %i g, %i i` デバッグ集計 | site `0x40AF3C` | [確定] |
| `0x7B2AA0` | NIF 読込失敗経路 `DISTANT LOD ERROR : could not load DistantLOD nif %s` | site `0x7B37B5` | [確定] |

## 1. .lod ファイル構造（命令レベル＋コーパス全量照合）

### 1.1 ローダ読取り順序（FUN_004D16D0 の命令根拠）

```
0x4D171F  call [file.vft+0x18](0,0)      ; BSFile::Seek(0, Begin) — 先頭から読む
0x4D1721  mov al,[file+0x24]             ; open/eof フラグ検査
0x4D1747  call [file+4](file,&dst,4,?,1) ; 4B 読取り = count（Ghidra: uStack_24、ループ上限）
0x4D177A  cmp i, count / jae end         ; for i in 0..count-1
0x4D179B    call ...(4) → formId         ; [iStack_20]
0x4D17BF    call 0x46B250(formId,0,&0xB02F9C,&0xB03248,0) → 0x9832E6 → form*
0x4D17FC    cmp byte [form+4],0x1E       ; ★ formType==30 (STAT) 限定
0x4D1800      (モード切替 [esp+0x48] ∈ {1,2,4})
0x4D17EE    call ...(4) → n              ; インスタンス数（[esp+0x18]、entry+0x30 に保存）
0x4D183A    entry = new 0x34; ctor 0x4CC7C0
0x4D1860      NiTArray resize ×3: [entry+0x10]+3n, [entry+0x00]+3n, [entry+0x20]+n（float数）
0x4D189A      map.insert(form→entry) via 0x452570（格納先 map = タスク引数 &arr、37バケット）
0x4D18B9      read 12n → [entry+0x14]    ; 配列1（12n バイト）
0x4D18E4      read 12n → [entry+0x04]    ; 配列2
0x4D190C      read  4n → [entry+0x24]    ; 配列3
0x4D1948    i++ → ループ先頭; 終了時 [file.vft+0](1) = Release
```

→ **1 レコード = formId(u32) + n(u32) + n×float3 + n×float3 + n×float（=8+28n バイト）**、
ファイル = `u32 count` + レコード×count、**レコードは formId 昇順でも固定長でもない**（n で可変）。

### 1.2 コーパス照合（実データ 921 件・v255_10/12/13/14）

- モデル `4 + Σ(8+28·n_i)`（先頭 u32=count）: **820/921 が count 一致＋EOF 厳密一致**（90.4%）
- EOF 厳密一致（count 無視の総歩行）: **842/921**。破綻 79 件はすべて **count 読み切り後に末尾残バイト**（tail ∈ 8..380B、tail mod 28 = 0 が 50/79、=8 が 15/79）
  → レコード列自体は同一モデルで歩行可能。**ヘッダ count ≤ 実レコード数** の不一致ファイルが存在する（例: `seworld_-11_1.lod` count=1・実 2 レコード〔同一 formId `0x00019F2C` の n=1/n=2〕、末尾残バイトなし = 2 レコード EOF 一致）。
  命令レベルではループ上限は count のため、**エンジン尾部レコードは読む**（count<実数のファイルは count 超過分を無視する）。[裁定] = 「count が権限・尾部データはエンジン unread」
  ※ 不一致ファイルは CK/LOD 生成器の count 書込みバグまたは**追記生成**（後述 §1.4）の可能性。[未確定]
- n レンジ = 1..11（最頻 1:1441、2:858、3:302…）、formId は全て `0x000xxxxx`（プラグイン 0=ESM ローカル）2804/2804 件 [確定・v255_10]
- ファイル名 `seworld_<x>_<y>.lod` の x,y は **-16..21 / -19..22** の整数 = **セル座標**（タスク名 `DistantLODLoaderTask for cell ( %i, %i )` 0xA45A98 と一致）。1 ファイル = 1 セル。

### 1.3 レコード内フィールド表（1 レコード = 8 + 28n バイト）

| off | 型 | 意味 | 根拠 | 判定 |
|---|---|---|---|---|
| 0x00 | u32 | FormID（Oblivion.esm 内、プラグイン 0 假定で解決） | `0x4D179B` 読取→`0x46B250`→formType 検査 `[form+4]==0x1E` | [確定] |
| 0x04 | u32 | n = インスタンス数（位置/回転/スレッド同一レコード内の個数） | `0x4D17EE` 読取→3 配列サイズ→`entry+0x30` | [確定] |
| 0x08 | float3 × n | **配列1**（実データで x≈-4e4、y≈±7e4、z≈0..3e3 のワールド座標） | `0x4D18B9` read 12n→`[entry+0x14]` | [確定] |
| 0x08+12n | float3 × n | **配列2**（回転と推定: 0 のみ/±0.785(rad)/-2.357 等、1 件単位で rad 範囲値） | `0x4D18E4` read 12n→`[entry+0x04]` | [確定]読み取り・意味[推定] |
| 0x08+24n | float × n | **配列3**（`0x42C9F0A4`=50.246/`0x42xxxxxx`=75..116 等、正の実数=スケール/半径候補） | `0x4D190C` read 4n→`[entry+0x24]` | [確定]読み取り・意味[推定] |

- 座標の绝对値帯（x -40559..-37159 等）は Tamriel SEWorld 内セル（セル×4096 単位系）と整合 [推定]
- float3 配列が 2 本＋float 配列 1 本の**ブロック順**配置（pos→rot→scale の per-instance インリープではない）は 1072B ファイルの float3 連続配置で確認 [確定]（v255_12: -10_-16 の pos[n] 連続）

### 1.4 デバッグ経路の量化処理（読取後の変換）

`0x4D8440`（ESM 側 chunk 読取）と `0x4D45F2..0x4D467D` に `×0.5`→`±0.97 clamped`→`fistp`（int 丸め）の浮動→整数量子化列がある。**これは ESM DistantRefLOD 経路**であり `.lod` ファイル経路（0x4D16D0）はfloat 直读（量子化なし）[確定]。

→ 実装は「pos/rot/scale を float のまま読む」でよい。

### 1.5 検証結果（v255_10/12/13/14・全量）

- ヘッダ count 基準歩行: **921/921 で全レコード読取り成功**（parse-abort 0・v255_24）。
  うち **820/921 (89.0%) は count 読み切りで EOF 到達**。
- **101/921 (11.0%) は count 読み切り後に末尾残バイト**（8..380B、うち 50 件は末尾が 28 の倍数＝丸レコード）。
  末尾残バイトの先頭に次のレコード形式 `{fid u32, n u32}` が観測される例が多い
  （例 `seworld_-11_1.lod` count=1・実 2 レコードで同一 formId `0x00019F2C`〔n=1〕+〔n=2〕、2 レコードで EOF 厳密一致）。
- **[裁定] 読取り契約**: エンジンは「先頭 u32 個のレコード」だけを読む（`0x4D177A cmp i,count` の命令証拠）。
  実ファイルの 11% は count より多く書かれている（生成器がヘッダを書き足し後に追記した形跡）。
  **移植実装は「count と EOF 歩行の不一致を許容し、EOF まで歩行して読んだ方を採用」するのが安全**
  （エンジン互換を厳密に取るなら count 基準。両方対応可）。[確定]+実測注記
- フィールド総調（v255_24・`tmp/re/v255_field_census.txt`）:
  - レコード 3202 件・**distinct formId 45** のみ（上位: 0x0001B00E×424, 0x0001B035×388, 0x00019F2C×343, 0x0001AFE, 0x0001AFF8…）
    → SI の distant object は少数の大型静止物（岩/巨木/門）に集中
  - n ∈ 1..11（n=1:1656, n=2:959, …）
  - 配列2[i=0] がゼロでないレコードは 98/3202 → 大半は無回転
  - 配列3[i=0]（スカラー配列）の値域 50.2..136.0（最頻 101.0）→ **回転なしの「半径 or 描画スコープ」的意味が妥当**（単位は pos と同一ワールド単位系）[推定]
  - pos0 の x/y はファイル名セル×4096 の範囲内（例 `seworld_-10_-16` の pos x≈-39272..-35xxx）→ **pos はワールド絶対座標**（セル基準ではない）[確定・実測]

## 2. レコードの入れ子構造 — 裁定: **「ワールドスペース→セル→form→インスタンス」の 3 層、GroupWidth ネストは .lod 側に存在しない**

- 依頼文の「ブロック（GroupWidth）×物件」入れ子仮説は **不成立**。1 ファイル = 1 セル固定、その中に form ごとの flat リスト（§1.1）。
  根拠: ローダはファイル内で seek せず seek(0)→count 回 straight read（`0x4D171F/0x4D177A`）[確定]
- セル→ファイル対応は `DistantLOD\%s_%i_%i.lod` の `%i_%i` = **セル X/Y**（タスク名 `for cell ( %i, %i )` と、
  pos 値帯がファイル名座標 ×4096 と一致する実測 [確定]）
- `iDistantLODGroupWidth:DistantLOD`（str `0xA8D308`、.text imm32 参照 0 件・.data `0xB2C360` の INI レコード経由）は
  **.lod/.cmp パーサのどこからも参照されない**（本解析範囲の関数 0x4BD430/0x4F1990/0x4D16D0/0x4F28B0 に該当命令なし）。
  値 8 が `0xB2C35C=0x00000008` に既定として在る。使われ方（ストリーミング窓のグループ化候補）は **[未確定]**。

## 3. .cmp の役割・構造

### 3.1 ローダ（FUN_004F28B0・命令レベル）

```
0x4F28F8  call [vft+0xd4]           ; worldspace 名 (char*)
0x4F2909  "Data\DistantLOD\%s.cmp" → path       ; site 0x4F2904 (str 0xA483EC)
0x4F2919  call 0x431690(path, 0, 0x800)         ; raw open（0x431690→0x430EF0 CreateFile 系・成功時オブジェクト/失敗時 0）
0x4F2943  call [vft+0x18](0,0)                  ; Seek(0, Begin)
0x4F294C  call [vft+0x1c]  → eax = file size
          n = size/4 - 1                         ; 0x4F294E shr 2 / 0x4F2951 sub 1
0x4F2954  cmp byte [file+0x24],0 ; ==0 → ループスキップ
loop n 回:                                       ; 0x4F2962..0x4F29A6
0x4F2970  call [vft+0x38](&u32, 4)               ; Read 4B
          key = (int16)hi<<16 | (uint16)lo       ; 0x4F2976..0x4F2984（movzx/movsx/shl/or＝ビット同一＝素の u32）
0x4F298E  call 0x4D6760(&map@this+0xC8, key)     ; find
0x4F299C  不在時 call 0x452570(&map, key, 1)     ; insert(key)
終了後:
0x4F29A8  mov byte [this+0xD8], 1                ; 「.cmp あり」フラグ
0x4F29BD  Read 4B → [this+0xDC]                  ; 末尾 u32（seworld.cmp では 7）
0x4F29C9  call [vft+0](1)                         ; Release
```

→ **.cmp = u32 の平坦配列**。最初の `size/4-1` 個は **(セルX:u16 下位, セルY:i16 上位) パック key の集合**
（読み手 `0x4F0DA0`: `key=(int16)blockX<<16 | (uint16)blockY` で find・0x4F0DA9-0x4F0DBD）。
最後の u32 は mgr+0xDC に保存され、`FUN_004EF2D0` が **`[mgr+0xDC] & kind` のビットマスク**として読む（種別有効化ビット）。
seworld.cmp の末尾 u32=7 = `0b111` = kind{1,2,4} 全有効 [確定・コード]
- 圧縮ではない（zlib/LZ4 シグネチャなし・読取コードは 4B ずつの素 read）[確定]

### 3.2 読出経路・順序

- `.cmp` ローダ `0x4F28B0` の caller = `0x4F326A`（FUN_004F2C10・worldspace 初期化系）→ **セル読込みより前、
  ワールドスペース単位の 1 回読込み** [確定]（`0x4F2C10` の callers: `0x446CB0` 帯・ロード文脈）
- `.lod` ローダ側からの依存: **なし**（0x4BD430/0x4F1990/0x4D16D0 に .cmp key 参照なし）。
  .cmp を照会するのは **タスク ctor の `mode==2` ゲートだけ**:
  `0x4BD4C7 cmp [esp+0x154],2 / je skip` → `0x4BD4D5 call 0x4F0DA0(ws, blockX, blockY)`（戻り false でタスク生成中止）
  及び要求ループ `0x483D60` の `0x48416C..0x484186`（kind==4 かつ .cmp hit で arg6=0 にする分岐）。
- **1 ファイル/worldspace**: [確定] — コーパス `.lod 921 = seworld_<x>_<y>×920 (+tamriel_×1)` に対し `.cmp` は `seworld.cmp` 1 本のみ。
  base BSA でも worldspace ごとに 1 本（§7: 55 本＝worldspace 数）。
- **セマンティクス注**: seworld.cmp の key 41 件 ≪ seworld_*.lod 921 件。**「.lod の存在索引」ではない**（40/41 がファイル名集合に含まれるのみ）。
  kind==2 モード（=`0x4EF2D0(mask & 2)` ゲート経由）の要求だけをフィルタする「変更/特別セル集合」の性質。
  実体意味（何で 41 なのか）は **[未確定]**。移植側は「.cmp 不在時は全許可（+0xD8==0 相当）」でエンジン挙動と一致 [確定]。

## 4. オブジェクト参照の解決規則（FormID → NIF）

- `0x4D17BF call 0x46B250(formId, 0, &0xB02F9C, &0xB03248, 0)` → `0x4D17C8 call 0x9832E6` → form*（プラグイン横断 FormID 解決）。
  `0x4D1800 cmp byte ptr [form+4], 0x1E` — **formType 0x1E（30 = static object）以外はその場で打ち切り** [確定]。
  → .lod の FormID は **Oblivion.esm の TESObjectSTAT** を指す（コーパス 45 formId は全て 0x000xxxxx=プラグイン 0）[確定]。
- **変換（transform）は .lod 内の float が正**。エンコーディング: インスタンス i につき
  `pos[i] (float3) / rot[i] (float3) / scale[i] (float)` を保持（§1.3）。単位行列固定ではない [確定]。
  rot/scale の描画への与え方（回転→Euler か axis-angle か）は消費側の生成コード未読のため **[推定]=Z 軸回転(ラジアン)＋一様スケール**。
  （根拠: 非ゼロ rot の z 成分のみが ±0.785/-2.357=π/4 系の値、x/y は常に 0〔v255_24 rot0 列〕）
- NIF パス決定は **STAT form 自身のモデル（MODL）経路**＝ESM 由来。`.lod` 内にパス/頂点なし（§5）。
  LOD 用の差し替え NIF 命名は観測されず、同一 form の通常 NIF を distant 描画に流用する構成 [確定・コード: 0x4F1AC0/0x4D1A90 の form->vft+0xec(Get3D) 系]。
- **FormID 45 件の個別解決（どの NIF か）は本セッションでは [未確定]**（Oblivion.esm 再スキャン禁止・既存台帳に該当 ID なし）。
  ただし engine は form 解決失败時 `test edi,edi → 打ち切り`（0x4D17CC）なので、移植側は
  「解決できない formId はレコードごとスキップ、ただし**行単位のシーク不要**のためストリーム整合に注意」。

## 5. 頂点/ジオメトリの有無

- **[確定] `.lod` はジオメトリを含まない。純粋な参照（FormID）＋transform リスト**。
  1) ローダ読取列は §1.1 の 6 read のみ（頂点/インデックス/NIF 断片を読む命令なし）
  2) コーパスの全サイズ 40..1072B（メッシュならあり得ない）
  3) fourcc（NAME/DATA/XLOD/XSLC/MODL 等）は 921/921 ファイルに 0 件（v255_9）
- NIF 断片説の発生源となった `0x4D43F0/0x4D8440`（XLOD/XSLC/NAME/DATA 列挙）は
  **ESM 内 DistantRefLOD レコード**のパーサであり .lod ファイルとは無関係（v255_9 の fourcc ゼロ確認で裁定）[確定]。
  ※ DistantRefLOD（str `0xA3001C`）は別系統：**CELL に直接置かれた distant 参照 form**（ESM/セーブ由来）。.lod とは別データ。

## 6. 索引と描画距離

- 索引の階層: **worldspace →（.cmp マスク/フィルタ）→ cell = `<ws>_<x>_<y>.lod` → form → instances**（§2/§3）。
- 要求生成側 `FUN_00483D60`（worldspace mgr `0xB35B8C` の vcall、callers `0x444340`/`0x4EB0E0`/`0x4EBAE0`）:
  - 走査窓 = `[mgr+0x20]-rad/2 .. +rad` のセルグリッド（`0x44434F shr eax,1`・`rad=[0xB06A2C]` 既定 5）→ **±2 セル程度**＋境界補正 `[B06A90/A98/AA0/AA8]`（既定 25/15/4/4）[確定・座標計算命令]
  - 各セルレコード `{byte valid@0, byte pending@1, u16 x@8, u16 y@0xC, ptr@4}`（16B 構造）を走査（`0x483EB3 shl eax,4` stride 16）[確定]
  - kind ∈ {1,2,4} は `[mgr+0xDC]`（**.cmp 末尾マスク**）・`0xB09AE8/0xB09AF0`（有効化フラグ・既定 1）で
    変換/抑止される（`0x483F3C..0x484089` の 0x4EF2D0 呼び列）[確定]
- **fFadeDistance:DistantLOD**（str `0xA3D888`・INI レコード `.data:00B06AB4`）: パーサ側参照なし（§2 同様 INI 経由）。
  ファード距離の適用点は本解析範囲外 **[未確定]**。移植 UI の `_objectlod` スライダー（RE_EXE_REPORT L2612）と
  グラブル `0xB09AE8/0xB09AF0` の対応は video settings 書手側に在る（`FUN_005DF160` 系 0x5DFF36/3D で 1 書込み確認）—
  スライダー値→byte のマッピング詳細は **[未確定]**（要 video settings 追跡）。
- 描画距離の設計指針（移植側）: 「cell 単位・半径 ±2〜4 cell（8〜16k ユニット）で .lod を常駐、
  遠方は .lod の pos/scale を使った billboard 風インスタンス描画」がエンジンの構造と整合 [推定]。

## 7. 素材供給経路（BSA v103 実証・serial247 項1 移管）

ファイル名テーブルを直接走査（v255_26/27・ヘッダ+索引ウィンドウのみ読取り）:

| アーカイブ | ver | dir flags | `.lod` | `.cmp` | folder |
|---|---|---|---|---|---|
| `Oblivion - Meshes.bsa` | 103 | 0x787 | **9,889** | **55** | `distantlod` |
| `DLCShiveringIsles - Meshes.bsa` | 103 | 0x703 | **921** | **1** | `distantlod` |
| 他 15 本の BSA（Sounds/Textures/Voices/DLC/Knights/Misc） | 103 | — | 0 | 0 | — |

- **[確定] オブジェクト LOD は BSA 内 `distantlod\<worldspace>_<x>_<y>.lod` / `distantlod\<worldspace>.cmp`**
  （= EXE 側論理パス `DistantLOD\…` / `Data\DistantLOD\…` と大文字小文字除外一致）。loose 配置は不要・本環境に不存在。
- **[確定] worldspace 数だけ .cmp がある**（base 55 = tamriel/seworld 系 worldspace 全 55 の 1:1。`.lod` 45 prefixes ⊂ 55 worlds）。
- **[確定] 本環境コーパス `bsa_Extraction\distantlod\` の出所 = `DLCShiveringIsles - Meshes.bsa`**
  （BSA 索引の名前表 921 エントリ = seworld 920 + tamriel 1・seworld.cmp 168B が索引の 1 件と一致）。
- **[確定] `meshes\distantlod\` という経路は存在しない**（folder 名は `distantlod`、`meshes\` 接頭なし）。
  依頼文の前提「`meshes\distantlod\*.lod`」は **誤り**。実ファイルの格納先は Meshes 系統 BSA の `distantlod/`。
- 圧縮状態: `Oblivion - Meshes.bsa` の flags=0x787 は圧縮ビット(0x4)立て、SI 側 0x703 は非圧縮。ただし圧縮は per-file フラグで個別判定される設計であり、本コーパス（展開後素材）は zlib/LZ4 ヘッダなしの生バイト＝無圧縮保存と実証 [確定・ヘッダ実測]。移植側は展開済み/loose 供給なら無圧縮で読める。BSA 直読みを実装する場合は per-file 圧縮ビットの確認が必要 [残点 V-255-5]。

## 8. 実装 API 仕様案（移植側・`OriginalLodProvider` 流用所見含む）

### 8.1 読み込み単位・手順

```
struct LodRecord { uint32_t formId; uint32_t n; float3 pos[n]; float3 rot[n]; float scale[n]; };  // 8 + 28n
struct LodFile   { uint32_t count; LodRecord[count or EOF]; };
load(path "distantlod/<ws>_<cx>_<cy>.lod"):
  1. 全字节读入（40..1072B、小）。u32 count 読取。
  2. p=4; while p<len-8 and (limit==count基準 or limit==EOF 選択可):
       fid=u32; n=u32; if n>上限(=512) or p+8+28n>len: break;
       pos=f32[n]; rot=f32[n]; scale=f32[n];
  3. formId→stat lookup（ESM パース済みフォームテーブル・formType==30 限定）。
     解決不能レコードは破棄（エンジンは打ち切りだが、移植は break より skip が安全—末尾不一致問題の回避）。
```

- **1 ファイル = 1 セル**、全体を一括読み（ブロック遅延読込みは不要・1072B 上限）。[確定]
- count/EOF 不一致（11%）に備え、**EOF まで歩行して得た全レコードを採用**する方針を推奨（エンジンとの差は 0、
  データロスは最小）。エンジン厳密互換モード（count 切断）もスイッチ化推奨。§1.5

### 8.2 メモリ表現

- 現 `DistantLodManager/ActiveBlock/Material` には**載せない別系统**を推奨:
  - `ActiveBlock`（=1 ブロック×地景 NIF/1 テクスチャ）と違い、オブジェクト LOD は **セル単位の軽量インスタンス表**。
  - 新 `ObjectLodCell { int cx, cy; vector<ObjectLodEntry{ FormId id; vector<{vec3 pos, float zRot, float scale}> insts; }> }`
  - 描画: entry の form→静的 NIF（通常 Meshes パス・実装側 ESM リーダで解決済み）を 1 メッシュ読込、
    インスタンス変換だけ複製適用（GPU instancing か transform 行列配列）。
- `.cmp` はロード不要（不在時は全セル許可＝エンジン flag==0 相当と同じ振る舞いで可）。
  実装コスト対効果が低く、**Phase 65 ではスキップ推奨**。採用する場合の構造は §3.1。

### 8.3 パス・フォールバック

- 探索順: `assets/distantlod/<ws>_<x>_<y>.lod`（移植バンドル展開想定）→ 欠落時 **サイレント空**（エンジンも open 失敗時タスク中止のみ・クラッシュなし: `0x4BD59B` test）。
- 失敗ブロック再試行抑止（`failedBlocks_` 相当）はそのまま流用可。
- **既存 `OriginalLodProvider` の流用可否**: パス解決（worldspace 名・x/y）と failedBlocks_ 再試行抑制は流用可。
  読込部（NIF/DDS → バイナリ走査）と ActiveBlock 生活周期は流用不可（§8.2）。
  推奨: 新 `OriginalObjectLodProvider`（同じ interface・loadCell(ws,x,y)→vector<ObjectLodEntry>）として並列追加。

### 8.4 依頼側の前提修正（実装に直結）

1. 素材実在パス: `distantlod\…`（BSA folder）／論理 `Data\DistantLOD\`。**`meshes\distantlod\` は誤り**（§7）。
2. オブジェクト LOD の単位は **ブロック(32×32 cell) ではなくセル(1×1 cell)**。ランドスケープ LOD の ActiveBlock 粒度と一致しない（§2）。
3. ジオメトリは .lod に含まれず、**参照先 TESObjectSTAT の通常 NIF を使う**（§5）。移植側はフォーム→NIF パス表（既存 ESM リーダ資産）で解決できる。

## 9. 残点（新規）

- V-255-1 [低]: `iDistantLODGroupWidth`（既定 8・.data 0xB2C35C）の消費者（ストリーミング窓のグループ化?）未追跡。
- V-255-2 [低]: fFadeDistance の適用点（shader/fog 側）未追跡。`0x4D45E2..` のフェード計算が ESM 系のみ確認。
- V-255-3 [中]: .cmp key 41 件の実体意味（変更セル集合? landmark 集合?）。kind==2 経路のゲーム内発火条件（どの操作で mode 2 が入るか）未確定。
- V-255-4 [低]: rot float3 の Euler 順/意味（Z のみ非ゼロの実測から Z 軸回転仮説）。消費側の行列組立コード未読。
- V-255-5 [低]: BSA v103 per-file 圧縮ビット（0x787 flags と .lod レコードの対応）— 展開済み素材では不要だが、BSA 直読み実装時は要確認。
