# v302_array_index_domain.md — serial 285（配列 index 域と 0xB39000 所有名束・3件）

- 作成日: 2026-11-06 / 担当: 本セッション内実施
- 出典: tmp/re/v301_micro_residuals_bundle.md 残点1..2 ＋ Graphify doc 登録保守点（Review 判定・新ルール4）
- 中間物: `%TEMP%\opencode\v285_i1..i3.py/_out`（3 probe・全 EXIT=0）＋ graphify skill 参照（update.md / graphify update CLI 実行）
- 判定サマリ: **件1=部分確定（構造閉じる・runtime 不変条件は B13 ゲート待ち）/ 件2=確定（GMST node 帯と確定）/ 件3=確定（gitignore ブロック記録・code 更新のみ実行）**

## 件1 配列 index 供給域と slot0 センチネル — 【部分確定】

### 構造（probe #1 実測・読手 0x57C240 全文）

```
arg eax: cmp eax,0x1d; ja out                       ; 29 カテゴリ（0..28）
edx = [[0xB33398]+0x20]                              ; TES 状態ブロック経由の表 base（v281 確定の 0x28B 非 polymorphic ブロック）
table1 = byte[edx + eax + 0x1B9B]（29 バイト）
cmp cl,9; jae → table2                               ; cl<9 なら table1 値を index に
  → 9 スロット配列 [cl*4 + 0xB39554]（0..8）
table2 = byte[edx + eax + 0x1B7E]（29 バイト・table1 と差 0x1D＝隣接）
cmp cl,0xee; jae out                                 ; cl<238 なら table2 値
  → 238 スロット主配列 [cl*4 + 0xB39578]（0..237）
```

- **9 スロット配列は 0xB39554..0xB39574（0x24B）で完全隣接**＝主配列 0xB39578 と隣接（v301 の隣接推定を確認・2 配列は連続 .bss 帯 [0xB39554..0xB39930]）。
- **index0 は両ゲート（<9 / <0xEE）とも構造的に通過可能**。供給元は runtime テーブル（[0xB33398]+0x20 の指し先＋0x1B7E/0x1B9B）＝TES ctor が埋めるため**静的に内容確定不能**。

### slot0 の裁定（証拠）

| 配列 | slot0 書手 | null 経路の挙動 |
|---|---|---|
| 9-slot @0xB39554 | **あり**（0x9F552B `mov [0xb39554],eax`＝v284_i6 確認） | null → `xor ecx,ecx` → コピーループ `mov al,[ecx]` = **[0x0] 読出し** |
| 238-slot @0xB39578 | **なし**（imm/disp32/SIB/A3 三重否定＝v301 確認） | 同上（0x57C2B2→0x57C2B6） |

- **読手 null 経路は素の `strlen(NULL)` 形**: `je null → xor ecx,ecx → mov edx,edi → mov al,[ecx]` にガードなし（v285_i1 実測・9-slot/238-slot 双方同型）。空文字列は dest が事前クリア済（0x57C255 `mov byte[edi],0`）なので**ロジック上は空文字で正しいが、[0x0] 読出しが発生する**。
- **裁定**: 238-slot slot0 は（書手不在ゆえ）**index0 が table2 から供給されない＝データ不変条件**に依存 [部分確定]。コード側の null ガードは「終端 0 到達で空文字」のみで、**null node→[0] 読出しの挙動は実機挙動に依存**（Oblivion がページ 0 をマップ済みなら空文字で無害、未マップなら AV＝理論的 latent crash）。静的判定不能＝**.data/EM 実行確認（B13 型・ユーザー実行ゲート）待ち**。
- 運用含意（移植向け）: ポータブル移植でこのコピー形を再実装する際は、**node null 時に strlen 前の空文字短絡を明示的に持つべき**（[0x0] 読出しは移植で暗黙成立しない）。docs 反映候補 +1（下記）。

## 件2 0xB39000 の帰属 — 【確定】

### node 帯の実証（probe #2/#3 実測）

- **0xB39000 = GMST/GameSetting のスカラー node**（probe #2: static-init 帯 0x9F6A30 が `push 0xa62a84('General(Face)'); push 0xa62a74('sGeneralFace'); mov ecx,0xb39000; call 0x419a70` → 登録後 `push 0xa22720; call 0x981fb4`(atexit)。probe #3: key 文言は sReset/Reset・sKBEsc/Esc・sMouseLeftButton/L-Button・sGeneralFace/General(Face) = **GMST の s 型（string 型）レコードの実行時値とキー**）。
- **node 帯 = 706 本の atexit 破棄 thunk**（`mov ecx,imm; jmp 0x4055E0`＝v283 確定の破棄関数）を census（probe #3b）: thunk@0xA20720..0xA235F0・node=0xB38000..0xB39E50（stride 8）。
- **アドレスギャップはちょうど当方の 2 配列領域**: ギャップ①0xB39528→0xB39AB8（この間に 9-slot/238-slot 配列 [0xB39554..0xB39930] が収まる）・ギャップ②0xB39B40→0xB39E08。**＝node 帯と fast-index 配列が同一 .bss 帯に交互配置**という整合した地図が確定。
- **graphify 既存ノードとの接続**: `GMST/GameSettingシステム (registrar 0x41BAE0 float / 0x419A70 dword; map 0xB35574; ESM GMSTロードで実行時上書き)` — 0x419A70/0xB35574/0x4055E0 の三要素が既存ノードと完全一致＝**本件は既存ノードの実測拡張**（node 帯・登録 stub・atexit thunk・fast-index 配列の内訳を追加確定）。
- `0x419B10`（probe #2c・[0xB39000] 読出し先）= `{[esi]=0; [esi+4]=0; [esi+6]=0; call 0x4028d0(copy)}` の小 ctor 群（0x419A70 の兄弟 API・2 文字列 node 構築系）。

## 件3 Graphify doc 登録（/graphify --update 経路） — 【確定（ブロック記録・code のみ実行）】

- **実行**: graphify skill 読込 → update.md ランブック確認 → Gemini/Google キー**未設定**（sem 抽出=ホスト負荷）→ `detect_incremental` 実行 → **結論: 現構成では v289-v301 の登録は不可能**。
- **ブロック原因（命令・行証拠）**: `.gitignore:53 /tmp/`（commit 8e4c1b88 / 2026-09-27・現在 dirty なし）により **graphify は tmp/ を scan 対象外**（manifest 3,598 行に tmp/re 行=0 件）。
- **既存 RE ノードも除去済み**: 11-05 の CLI `graphify update .` 実行時に「pruned 1207 nodes from 80 newly-ignored files」＝**旧 1,190 tmp/re ノード（v258/v292/v283 等）が graph.json から実消失**（probe 確認: graph.json 内に当該文字列ゼロ）。
- **本日実行（code 分）**: `graphify update .` → `AST extraction: 2/2 uncached`（他 44 件は hash cache キャッシュ）→ **`No code-graph topology changes detected; outputs left untouched`**＝graph.json/HTML/report は改変なし。
- **保留（各理由）**: ①tmp/re 登録=**.gitignore 変更（`!/tmp/re/` 否定規則）が必要＝L1 で不可・人間決定** ②検出済み 1,667 images（vision 抽出＝Gemini キー or 現セッション非現実的）③`docs/RE_EXE_REPORT.md`（12,564 行・並行実装セッションが更新）＝インライン sem 抽出不能・次メンテナンスラウンド ④9 documents 中の meta 群（LOOP/STATE/TODO/.ai-todo×3/CMakeLists×2）＝小物だが①-③と一括で次ラウンドへ。
- 台帳様式に従い ticket の「失敗時は skill コマンド仕様を記録して保留」を履行: update.md の detect_incremental→AST→build_merge→Steps4-8 手順、code_only 分岐条件、ステップ9 manifest stamp 規則（未抽出 sem は unstamped 維持＝再試行される設計）を本件に記録。

## docs 反映候補（+3 点・#20/#47）

1. Settings コピー形の移植注意（node null 時の [0x0] 読出し＝移植で空文字短絡を明示）
2. GMST 実行時 node 帯（0xB38000..0xB39E50・706 本・stride 8・2 ギャップに fast-index 配列）— graphify 既存 GMST ノードへの実測追記
3. slot0/index0 のデータ不変条件（table2 の byte≠0 暗黙）— .data/EM 実行確認待ちの注記

## 残点（次候補・低／またはゲート）

1. table2 帯（[0xB33398]+0x20+0x1B7E・29 バイト）の実データ値域＝**B13 型実機ダンプゲート待ち**（残りは runtime 不変条件の確認のみ）
2. Graphify バックログ: tmp/re 登録（gitignore 人間決定）・1,667 images（キー設定）・RE_EXE_REPORT（大きさ）— **メンテナンス/人間決定**であり解析作業ではない

## 検証（selftest 相当）

`v285_i1..i3.py` 再実行 EXIT=0。整合: 9=0x24/4・238=0x3B8/4=0xEE・29=0x1D（表差）・706 thunk=probe#3c 数値、0x419A70/0xB35574/0x4055E0/0x981FB4 は v283/v291/v298 既確定と完全一致、0x419A70 の引数レイアウト（[esp+0x1c]=arg1/[esp+0x20]=arg2・SEH 6 push）は v283 §件1 の実測と一致。graphify 検証: manifest 3,598 行=tmp/re 0 行・/tmp/ blame=8e4c1b88・graph.json 旧ノード 0 件・CLI update 出力 3 行の実測保存（本ファイル §件3）。既存 tmp/re 成果物・docs・GT・ソース無編集、ESM/git 非接触（git は log/blame/status の読み取りのみ）。
