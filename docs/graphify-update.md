# Graphify の定期更新（差分のみ）とラベリング

`scripts/graphify_update.ps1` が **`graphify update`（コードの差分再抽出のみ・LLM 不要）** を実行し、
グラフを最新のソースに追随させる。Windows タスクスケジューラに登録済み。

## 定期更新（コード差分のみ）

| 項目 | 値 |
|---|---|
| タスク名 | `Oblivion_Android Graphify Incremental Update` |
| スケジュール | **毎日 03:00**（PC が起動していなければ次回起動時に実行） |
| 実行ファイル | `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1` |
| ログ | `graphify-out/logs/graphify-update-YYYYMMDD-HHmmss.log`（最新 14 本を保持） |
| 対象 | **変更されたコードファイルのみ**（`manifest.json` との差分。AST 再抽出で LLM 不要） |
| 出力 | `graphify-out/graph.json` / `GRAPH_REPORT.md` / `graph.html` |

- 差分が無い場合も再クラスタリングとレポート再生成は実行される。
  **実測（2026-11-26, 57,109 nodes / 137,102 edges）**: `cluster-only` は **77 秒**（`--no-viz`）、
  **423 秒 / 491 秒**（HTML 再生成込み・2 回実行）。
- 同時実行は防止済み: `graphify-out/.scheduled_update.lock`（本スクリプト）と
  `graphify-out/.rebuild.lock`（post-commit hook）の PID を確認し、実行中なら **exit 3** でスキップする。
- `PYTHONHASHSEED=0` を固定し、Louvain のクラスタリングを再現可能にしている。
  **再現実測（2026-11-26）**: 同一グラフで `cluster-only` を 2 回実行して
  node→community 写像の sha256 が完全一致（`c66a3002ca1f6bc7…`）。

### 手動実行

```powershell
cd C:\Users\hiroki.kogarumai\Oblivion_Android
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1
# 前提確認だけ（実行しない）
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\graphify_update.ps1 -CheckOnly
```

終了コード: `0` 成功 / `1` 前提不足（graphify・graph.json 無し） / `2` 更新失敗 / `3` 別の更新が実行中。

### スケジュールの変更・削除

```powershell
# 毎日 03:00 → 6 時間ごと（例）
schtasks /Change /TN "Oblivion_Android Graphify Incremental Update" /SC HOURLY /MO 6

# 削除
schtasks /Delete /TN "Oblivion_Android Graphify Incremental Update" /F
```

## ラベル（コミュニティ名）の保存と劣化防止

- **正本**: `graphify-out/.graphify_labels.json`（curated＝手動/LLM で付けた名前）＋
  `graphify-out/.graphify_labels.json.sig`（メンバーシップ署名）。
  **hub フォールバックは別ファイル `.graphify_labels.pending.json` に分流される**
  （graphify 0.9.95 の `watch.py` #3334）。ただし **`cluster-only` は hub 名を
  tracked 側にも書き込む**（`cli.py` のラベル再利用分岐）ため、`pending` の方が
  「未命名」の正しい指標になる。
- **署名チェック**: 再クラスタで cid が入り替わっても、署名一致（または双方の過半数が
  一致）するコミュニティは名前を維持する（`label_still_applies`）。
  合わないものは hub 名に落ち、stderr に
  `renamed N community(ies) by their hub` と出る。

### 実測: ラベルはコード変更で減る（重要・2026-11-26）

**「定期更新でラベルが消えることはない」は誤りだった。訂正する。**

`graphify update .`（9 ファイル再抽出・+7 ノード）を 1 回走らせた結果:

| 指標 | 値 |
|---|---|
| ラベル（curated） | **1,811 → 1,425（−386, 21.3% が hub 名へ）** |
| コミュニティ数 | 1,811 → 1,828 |
| ノード | 57,109 → 57,137（**+7 のみ**） |
| 別名への静的書き換え | **0 件**（失った cid はすべて hub 名に落ちた） |

- **原因は署名チェックの誤判定ではない**: +7 ノードで Louvain の分割が揺れ、
  196 cid のメンバーシップが過半数一致を割った（`cid 10`: 150→60 ノード等）。
  これは graphify の意図した安全動作（古い名前を新しい構成に貼らない）。
- **再クラスタ自体は決定的**: 同一グラフで `cluster-only` を 2 回実行し
  node→community 写像の sha256 完全一致（`c66a3002ca1f6bc7…` → 後の入力変更後は
  `90e7abfb71831c09…` も 2 回一致）。**揺れは入力変更が原因**。
- **運用**: コード変更の後は hub 名に落ちた分だけ再命名する。
  `pending` の件数が「未命名数」。再命名は本文の手順 1〜4 を対象を
  `pending` に限定して回す（1 回あたり 403 件＝5 チャンク、約 10 分）。
  定期タスクは差分抽出のみで LLM を呼ばないため、**再命名は人間/エージェントの手で行う**。

## ラベリングの手順（LLM バックエンド不要）

`graphify label` は既定で LLM バックエンド（Gemini/Kimi/Claude/OpenAI/…）を要求するが、
この環境は **API キー無し・`claude` CLI 未ログイン・無料枠 429** のため CLI 経由の
`graphify label` は使えない。代わりに graphify skill の **Step 5（ホストエージェント自身が命名）**
を使う。API キーは 1 つも読まない。

1. 命名材料を作る（god node 優先で各コミュニティ最大 12 個の代表ノード名）:
   `graphify.llm._community_label_lines` を graph.json に対して呼び、100 行ずつ
   `chunk_NN.txt`（`cid: name1, name2, …`）に分割する。
   **再命名時は対象を `pending`（未命名）に限定する**（初回は全件＝19 チャンク、
   再命名は 403 件＝5 チャンク）。
2. 各チャンクをサブエージェントに渡し、**2〜5 語の英語名**を
   `labels_NN.json`（`{"cid": "Name"}`）で返させる。全件を 1 メッセージで並列 dispatch。
   **無料枠で落ちたチャンクはファイルが書かれない**ため、件数を数えて再 dispatch する。
3. チャンクを結合して検証（ASCII・2〜5 語・引用符なし・全 cid 網羅）し、
   `.graphify_labels.json` と `.sig`（`community_member_sigs` で計算）に書き込み、
   `.graphify_labels.pending.json` は削除する（全て命名済みのため）。
4. `PYTHONHASHSEED=0 graphify cluster-only .` で GRAPH_REPORT.md / graph.json / graph.html を再生成。

**実績（2026-11-26・最終状態）**: 1,828 コミュニティ全数に命名。
`labels=1828 / sig=1828 / pending=0 / missing=0 / placeholder=0 / 非ASCII=0 / 単一識別子名=0`、
語数分布 2語 124 / 3語 1518 / 4語 183 / 5語 3、異なり名 1757/1828（重複 41）。
初回命名時は 1,811 件（更新前は hub 名が 1,139 件残っていた）。
`GRAPH_REPORT.md` 585,103 bytes / `graph.html` 1,807,535 bytes（community 集約ビュー）。

**弱い名前の扱い**: 初回は 120 件（入力時点の弱パターン 29 ＋重複 95、うち 4 件が両方）を
文脈を広げて再命名した。members が `FUN_xxxxxxxx` アドレスだけでテーマを支えられない
コミュニティは、推測でテーマ名を付けず **`Anonymous Address Region`** を使った
（最終状態で **23 件**）。これは意図的な正直名であり、weak 正規表現
（`unlabeled|unknown|unidentified|misc|…|cluster|group|region|area|functions|symbols`）には
`region` が含まれるため、最終状態の weak は **37 件**に数えられる
（**未解決の曖昧さ 0 件ではなく、正直に未同定と名付けた 23 件**がその大半）。

**重複名の内訳（内容が同一の別コミュニティに由来・最終状態の実測）**:
重複を含む cid **112 件 / 異なり重複名 41**。
`anonymous address region` ×23 / `rectangle geometry` ×7 / `physics body management` ×4 /
`font scope guard` ×4。いずれも複数ファイルに同一のクラス/構造体が存在することが原因で、誤りではない。

### 罠

- **`graphify label`（CLI）は既存ラベルを無視して全件を hub 名で作り直す**
  （`force_relabel = cmd == "label"` → `cli.py` の再利用分岐を素通り）。`--missing-only` を
  付ければ既存名を保持し欠落分だけ命名する。手動で付けた名前を保ちたいときは
  **`cluster-only`** を使うこと（署名が合えば全件維持）。
- **`cluster-only --no-viz` は `graph.html` を削除する**（2026-11-26 実測）。
  HTML を残したいなら `--no-viz` を付けない。
- **無料枠の枯渇**: サブエージェントが `Free usage limit reached` で落ちることがある。
  落ちた分はファイルが書かれないので件数を数えて再 dispatch する（1 チャンクだけの
  プローブを先に流すと回復確認になる）。
- **139 ファイルが構文エラーで部分抽出、67 ファイルがシンボル 0**（tree-sitter の既知制限）。
  `Oblivion_decompiled.c` の後半欠落と同じ原因。
- **画像 1,667 件は deferred**: `app/src/main/assets/fonts/*.png` 等。vision バックエンド待ち。

## 対象外（手動・別経路）

- **docs / papers の semantic 再抽出**: `graphify update` はコード専用。
  セッション内で `/graphify --update`（サブエージェント）を使う。
- **画像**: vision バックエンド待ち（既知のバックログ）。

## 関連

- post-commit hook（`.git/hooks/post-commit`）も commit 後にコードのみ再ビルドする。
  定期タスクは「未コミットの編集」や「commit されない期間」を埋める補完。
- `graphify-out/` は `.gitignore` 済み（生成物はコミットしない）。
