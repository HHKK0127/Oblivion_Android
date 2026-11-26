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

- **正本**: `graphify-out/.graphify_labels.json`（1,811 件）＋
  `graphify-out/.graphify_labels.json.sig`（メンバーシップ署名）。
  フォールバック名は別ファイル `.graphify_labels.pending.json` に分流される（graphify 0.9.95 の仕様）。
- **`graphify update` / `cluster-only` は既存ラベルを尊重する**: 再クラスタで cid が
  入り替わっても、署名が一致する（または双方の過半数が一致する）コミュニティは名前を維持する
  （`label_still_applies`）。署名が合わないコミュニティだけが hub 名に差し替えられる。
- つまり**定期更新でラベルが消えることはない**。コードが増えてコミュニティ構成が変わった
  分だけ該当コミュニティが hub 名に落ち、それは stderr に
  `renamed N community(ies) by their hub` と出る。

## ラベリングの手順（LLM バックエンド不要）

`graphify label` は既定で LLM バックエンド（Gemini/Kimi/Claude/OpenAI/…）を要求するが、
この環境は **API キー無し・`claude` CLI 未ログイン・無料枠 429** のため CLI 経由の
`graphify label` は使えない。代わりに graphify skill の **Step 5（ホストエージェント自身が命名）**
を使う。API キーは 1 つも読まない。

1. 命名材料を作る（god node 優先で各コミュニティ最大 12 個の代表ノード名）:
   `graphify.llm._community_label_lines` を graph.json に対して呼び、100 行ずつ
   `chunk_NN.txt`（`cid: name1, name2, …`）に分割する。
2. 各チャンクをサブエージェントに渡し、**2〜5 語の英語名**を
   `labels_NN.json`（`{"cid": "Name"}`）で返させる。全件を 1 メッセージで並列 dispatch。
3. 19 チャンクを結合して検証（ASCII・2〜5 語・引用符なし・全 cid 網羅）し、
   `.graphify_labels.json` と `.sig`（`community_member_sigs` で計算）に書き込む。
4. `PYTHONHASHSEED=0 graphify cluster-only .` で GRAPH_REPORT.md / graph.json / graph.html を再生成。

**実績（2026-11-26）**: 1,811 コミュニティ全数に命名。
`labels=1811 / sig=1811 / gap=0 / placeholder=0 / 非ASCII=0 / 単一識別子名=0`、
語数分布 2語 135 / 3語 1481 / 4語 191 / 5語 4、異なり名 1742/1811。
更新前は hub 名（自動命名）が 1,139 件残っていた。

**弱い名前の扱い**: 120 件（入力時点の弱パターン 29 ＋重複 95、うち 4 件が両方）を
文脈を広げて再命名した。members が `FUN_xxxxxxxx` アドレスだけでテーマを支えられない
コミュニティは、推測でテーマ名を付けず **`Anonymous Address Region`（20 件）** とした。
これは意図的な正直名であり、weak 正規表現
（`unlabeled|unknown|unidentified|misc|…|cluster|group|region|area|functions|symbols`）には
`region` が含まれるため、再命名後も weak は **29 件**のまま推移する
（**未解決の曖昧さ 0 件ではなく、正直に未同定と名付けた 20 件**がその大半）。

**重複名の内訳（内容が同一の別コミュニティに由来。再命名後の実測）**:
重複を含む cid **107 件 / 異なり重複名 38**。
`anonymous address region` ×20 / `rectangle geometry` ×9（各ファイルに `Rect, h, w, x, y`）/
`font scope guard` ×8（各ファイルに `FontScope, .initialize(), …`）。誤りではない。

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
