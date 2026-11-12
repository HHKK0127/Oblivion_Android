# RE_EXE_REPORT.md 追記ドラフト2（V258 / オブジェクト LOD 残点束）
# 適用対象: docs/RE_EXE_REPORT.md の「54.8 DistantLOD」節への反映 + v255 ドラフトの訂正。
# 本ファイルはドラフトのみ。docs 本体・v255_docs_re_exe_report_draft.md は未編集（人間承認待ち）。
# 証拠本体: tmp/re/v258_object_lod_residuals.md（生データ %TEMP%\opencode\v258_{1..22}_out.txt）

## A. v255 ドラフトからの訂正（4 点）

1. **§.cmp フィールド表の packed key 方向**（v255 ドラフト 36 行目）:
   `key = (uint16 cellX | int16 cellY<<16)` は**上下逆**。読手 `0x4F0DA0` の命令は
   `movsx(word[esp+4])<<16 | movzx(word[esp+0xc])` で、第一引数=**cellX(上位 i16)**・第二引数=cellY(下位 u16)。
   正: `(int16 cellX << 16) | (uint16 cellY)`。訂正後、seworld.cmp 41 key は **.lod 実ファイル名集合と 41/41 一致**
   （v255 の 40/41・孤児 (6,11) はスワップ誤り。v258_1 F）。
2. **「実体意味 [未確定]」→ [確定] に昇格**（v255 ドラフト 42 行目）: key 集合は
   「ランドマークセルのホワイトリスト」（件1・下 C）。
3. **§供給「base flags 0x787 は圧縮ビット立て、SI 0x703 は非圧縮」の連想はファイル単位では誤り**（件5）:
   両アーカイブの .lod/.cmp **全 10,866 件が zlib 圧縮保存**（`u32 展開後サイズ + 78 9C ストリーム`）。
   per-file f1>>25 は SI 全件 32 / BASE 全件 0 だが圧縮実態と無相関 → **圧縮判定は magic 検査のみ**。
   （展開済み loose コーパスが無圧縮だった v255 実測は妥当——抽出器が解凍していた。）
4. **「iDistantLODGroupWidth/fFadeDistance は参照ゼロ（適用点別系統 [未確定]）」→ 適用点 [確定]**（件2・3）:
   GroupWidth の消費者 = **land LOD NIF グループローダ `0x7B3BE0`**（÷8 でセル→ブロック）、
   fFadeDistance の適用点 = **`0x4838F0`**（B34410/14 算出 → B2C334/338、消費 `0x810F90`/`0x483590`）。
   両方とも**オブジェクト LOD（.lod/.cmp）パーサ系は非参照**のままで再確認（v258_5/6/7）。

## B. V258 新規事実（54.8 への追記案）

### B.1 .cmp の実体 — kind2（遠方帯）要求のホワイトリスト [確定]
- kind（=タスク ctor arg7、`0x4BD430@[esp+0x154]`）は要求ループ `0x483D60` で生成:
  kind1=窓内・既存 obj あり（`0x483F26`）、kind4=窓内・新規（`0x483FF0→+4`）、
  kind2=**外周 band 降格**（`0x484105..0x484129 mov ebp,2`）＋**stale pending 掃除**（`0x483FF8..0x484019`）
  ＋設定降格（`0x484066..0x484084`: B09AE8/B09AF0 で 4→1/4→2）。
- **kind2 のときのみ** `.cmp` NiTMap を照会し、不在セルは要求中止
  （要求側 `0x484036`・ctor 側 `0x4BD4C7→0x4BD4D5 call 0x4F0DA0`）。
- seworld.cmp の 41 key = {x∈9..12,y∈2..6}∪{x∈-14..-8,y∈-6..0} の 2 クラスタで、
  (i) 全 key が既存 .lod セル、(ii) **exclusive formId 26 件**（0x0003FFE6 等・非 key セルに一切出現せず）、
  (iii) **rot 非ゼロは key セルにのみ存在**（x/y スロットは全 corpus ゼロ、z のみ ±45°倍数）。
  → センター窓外の外周帯では「ランドマーク系 STAT を持つセルだけ」 distant LOD ストリームを許可する集合。
- **trailer u32 = マスク `mgr+0xDC` の意味**: bit0=kind1 級許可・bit1=kind2 級許可
  （両ゼロなら要求ループが全セルスキップ `0x483F36..52`）、**bit2=地形(land) LOD 機能有効**
  （消費者 `0x4EA160@0x4EA1D4/0x4EA1FE push 4` → 無効時 `0x4EB0E0(0)`+`0x4BDD40/0x4BDCD0` フラッシュ、
  settings 側 `0x5BDCD0@0x5BDCFE push 4`）。seworld=7 = 3 機能全有効。
- **既定値 [確定]: worldspace mgr ctor `0x4F2A10` が `+0xDC = 7` を書く**（`0x4F2B64 mov ebx,7→0x4F2BDC`）。
  `.cmp` open 失敗時は +0xDC を触らない（`0x4F2923 early out`）→ **.cmp 不在ワールド = マスク 7（全級許可）+
  `+0xD8==0`（`0x4F0DA0` 全許可）= 事実上「全セル kind2 許可」**。移植側が .cmp を読む必要なし（v255 §8.2 継続妥当）。

### B.2 mode==2 の発火タイミング [確定]
- `0x483D60` の caller は 3 系のみ（E8 全走査）:
  (a) **セル座標 commit**: `0x40EE66/0x4455BF→0x444EC0`（位置→セル index 換算 `sar 0xC`=4096）→ `0x444C70` → `0x444340`
  （"Starting world %08X %s with bounds…" `0xA37600`）→ `0x483D60(ws mgr)`。
  **セーブ/ロード/新規ゲーム/テレポートを問わず「ワールド空間内でセルが動くたび」に発火**。室内は即 return（`0x444C84`）。
  (b) **land LOD 再構築**: `0x4EB0E0(bool)`（Landscape LOD 管理者・`Data\Meshes\Landscape\LOD\…` `0xA47AE8`）
  @`0x4EB15A`、`0x4EBAE0` @`0x4EBB1D`。caller: `0x405CE0`（startup）、コンソール `0x508240`
  （"LOD land is now being displayed./disabled." `0xA4C20C/F0`）、`0x4EA160`（ws 切替）、`0x5BDCD0`（設定 change）。
  (c) **video settings 適用**: `0x5BDCD0`（dirty フラグ `0xB3B740` test&clear `0x5DDCD0`、setter `0x5DDE85/0x5DF09C`）。
- ws mgr 引数は `[0xB333A0]+0x74`（アクセサ `0x43F270`）。NULL=室内 → 要求生成なし（`0x483DEA`）。

### B.3 settings ID 9/10 ↔ gateA/B [確定（ラベルのみ未確定）]
- `FUN_005DF160`（video settings 適用 dispatch・switch ID 1..19、jump table `0x5E012C`/`0x5E017C`）:
  **ID9 → `0x5DFC55..64` が `0xB09AE8` を sete 反転**（+通知 `0x404E10` + `0xB3B744 |= 2` + `0x579C10`）、
  **ID10 → `0x5DFCA6..B5` が `0xB09AF0` 反転**（`0xB3B744 |= 4`）。リセット経路 `0x5DFF36/3D` は両方 1。
  メニュー表示側は `0x5DEB80@0x5DED1C/0x5DED32`（video_menu.xml）。
- 両バイトの**消費点は `0x483D60` のみ**（窓ゲート `0x483DD4/DDF`、kind 降格 `0x484043..82`）→
  「スライダー値→距離計算」ではなく**要求級の ON/OFF ベクタ**。fFadeDistance との連結は無い（独立）。
- 依頼文の `0x4F0E70` は B09AE8/AF0 を参照せず（NiTArray dtor グルー。誤り補正）。

### B.4 GroupWidth / fFadeDistance の適用点 [確定]
- GroupWidth(8): `0x7B3BE0@0x7B3C05` 読取 → packed key (i16 i16) を **X,Y 各自 ÷8**（idiv）してブロックキー再パック
  → NIF ロード `0x7B2AA0` → 管理者マップ `0xB2C33C`（操作メソッド `0x7B3A40/0x7B3BE0/0x7B3DB0/0x7B4010`）。
  = **ランドスケープ LOD の 8×8 セルグループ化**。オブジェクト LOD とは無関係（移植の .lod ローダは非依存で確定）。
- fFadeDistance(12288.0): `0x4838F0`（distant 55×55 グリッド再構築 `edi=uGridsToLoad+2×uGridDistantCount`）内の
  `0x483A5D fld [0xB06AB0]` → `B34410 = B34414 − fFadeDistance`、`0x483A75/7B` で `B2C334/B2C338` に保存。
  `0x483D60` 末尾（`0x48422E..94`）がスキャン毎にキー文字列 `0xB43484`（BSS 実行時生成・内容静的不可）の
  設定ノード float `[+0x2c]/[+0x30]` を B34410/14 でクランプして B2C334/338 再書込。
  外部消費は `0x810F90@0x811100 push 0xB2C334`（フェード帯の転記）と getter `0x483590`。
  → 意味: **遠方 LOD のフェード開始距離（=distant 境界−fFadeDistance）**。描画距離そのものは uGridDistantCount(25)窓。

### B.5 rot スロット & エントリ対称性 [確定] / 最終行列組立 [未確定]
- .lod の rot float3（ファイル 2 本目）→ `entry+0x04` 配列。ESM 側ビルダ `0x4D43F0@0x4D46F9..0x4D4755` が
  `NiTArray::Add(0x4D31D0)` で pos→`+0x10`配列 / rot→`+0x00`配列 / scale→`+0x20`配列 / `+0x30` n++ を対に行うことを命令で確認
  → .lod 経路と同一エントリ（ctor `0x4CC7C0`）。v255 §1.4 の量子化（fistp 列）は **pos 側**（0.5 セル格子）と確定。
- 実データは rot slot[0]/[1] が全 3,202 レコードでゼロ、slot[2] のみ 45°倍数 →
  **移植は `yaw=rot[i].z`（Z 軸回転・ラジアン）で出力同値**（Euler 順の最終消費者は V-258-4a として残点）。

### B.6 BSA 直読みの圧縮取り扱い [確定]
- file record 16B = `{u64 nameHash, u32 size(f1), u32 offset(f2)}`（オフセット鎖で役割確定）。
- .lod/.cmp の保存ペイロードは**常に** `u32 展開後サイズ + zlib(78 9C)`（SI 922/922・BASE 9,944/9,944、
  展開==loose をサンプル一致確認）。圧縮は**アーカイブ単位で全件**なので per-file フラグ参照は不要
  （flag bit0x40000000 は SI 専用で圧縮と無相関＝意味未確定、無視可）。
- 無圧縮 .lod の先頭 u32（count）と区別するため **b[4:6]==78 9C で判定**こと。

## C. 移植側（Phase 65）への最終指針差分
1. .cmp: **読まなくてよい**（既定マスク 7 + 不在時全許可と命令・既定値両面で確定）。読む場合の仕様は B.1。
2. .lod: EOF 歩行 + `yaw=slot2`（B.5）。count 切断モードはスイッチ推奨（v255 継続）。
3. BSA 直読み実装: u32+zlib を常に期待（B.6）。loose 供給なら無圧縮生読込。
4. video settings `_objectlod` 相当スライダーはエンジンでは 2 進ゲート（ID9/10）+fFadeDistance 独立の構成。
   段階スライダー実装は移植側の造作として可（エンジン意味 = ON/OFF×2 + フェード帯幅）。
