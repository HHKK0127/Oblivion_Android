# V67 / V74 Supermemory API 挙動調査結果（2026-10-24）

対象プラグイン: `opencode-supermemory` v2.0.17
実装本体: `C:\Users\hiroki.kogarumai\.config\opencode\node_modules\opencode-supermemory\dist\index.js`（bundled、以下略称 `index.js`）
実行回数: `list(scope=user)` ×1、`search(scope=project)` ×6（計7回、リトライなし）

---

## §1 V67: userスコープの `list` がなぜ8件しか返さないのか

### 1.1 コード根拠（呼び出しチェーン）

| 段階 | 場所 | 内容 |
|---|---|---|
| ツール層 | `index.js:18721–18726` | `case "list"`: `limit = args.limit || 20`（18723）、`scope==="user"` → `internalScope="personal"`（18724）、`readTags = tags.personalReads`（18725）、`listMemoriesScoped(tags.canonical, readTags, "personal", limit)` 呼出（18726）。返却マッピングは `content: memory.summary`（18740）＝summary が null なら content=null。 |
| スコープ付与 | `index.js:18644–18650` | add 時のみ `sm_scope: internalScope` をメタデータに埋め込む（user→`personal`）。 |
| タグ集合 | `index.js:19061–19070`, `18988–18990` | `personalReads` = [canonical(repo_…__hex16), 生成タグ, claude/codex/opencode/cursor のレガシー user タグ群]。 |
| スコープ絞り込み | `index.js:17923–17932` | `listMemoriesScoped`: canonical タグへのクエリにのみ scope 適用（`supportsScopedCanonicalTag`= `repo_…__hex16` のとき、正規表現は 17646–17648）。各レガシータグへのクエリは **scope フィルタなし**（17929）。 |
| フィルタ定義 | `index.js:17641–17645` | `getScopeFilters(scope)` = `{AND:[{key:"sm_scope", value:"personal", filterType:"metadata"}]}` — スコープは **メタデータ属性フィルタ**。 |
| API 呼出 | `index.js:17889–17899` | `memories.list({containerTags, filters, limit, order:"desc", sort:"createdAt", includeContent:true})` — **page/offset パラメータは存在しない**（単発の直近N件クエリ）。 |
| V5 クライアント | `index.js:17117–17178` | `sdk.list(namespace,"documents",{limit, sort, order, filter})`（17123–17128、単一呼び出し・ページ送り無し）。各文書は `documents.get` で個別ハイドレーション（17129–17150）だが **予算 `listBudgetMs`（約29.75秒）内のみ**（17122/17135）。さらにクライアント側で `document.metadata?.[key] === value` を全フィルタclauseに再検証（17151）＝ `sm_scope` が無い文書はここで弾かれる。 |
| マージ | `index.js:17613–17633` | `mergeListResponses`: 結果を `listKey`（**本文優先**の重複キー、17603–17608）で dedupe → 日付降順ソート → **`.slice(0, limit)`**（17623）。pagination は常に1ページに偽装（17627–17631）。 |

### 1.2 実測（mode=list, scope=user — 1回のみ）

- 返却: `count=8`、全件 nanoid系ID、`createdAt` 降順、範囲 2026-10-21T09:13 〜 2026-10-24T05:43。
- ID一覧: `FevskxChACYbfiLavmuHwu`(10-24), `njE8APWxFj1nA4879vr2yp`(10-24), `1poBPTPJ3c9Sm1RGDnpChz`(10-22), `mb6pYL1pHLZdV2zKDwcUjz`, `uxPRfaVWziNwvi4rgm8AbC`, `Tqc5YoWLQk1DzznuNcdeY2`, `3Z8t3GkqWrt5wZ8HoPFgWf`, `Mj6FSoS3H7TJY9AymCvjwq`（以上10-21）。
- 既知のuser系ID（`cdc31ca…`, `5c40d2c…`, `iqGEEr…`, `6dfb32…`, `wUgLBdZ…`, `H1vgV2eaK1oD6zKA9q6wMG`, `kkUaS1R4NTY8fQjm9u6o7e`, `2S7bCsP2H1Pa1wRpVX8E9o`, `bKxLNnLuszBqQwGeGxBvxC`, `qyZjibCsda…`, `dee1f346…`, `33b810a7…`, `30e449151…` 等）は **1件も出現せず**。ID体系も別（UUID系 vs nanoid系）。

### 1.3 仮説の検証結果

- **(a) 先頭ページのみ（limit N=8?）** — **却下**。`limit` 既定値は20（18723）で8<20＝キャップ非継動。そもそも server/API 側に page/offset が無く（17123–17128）「ページ」概念が存在しない。返却8件は「canonicalタグ×sm_scope=personal フィルタ＋レガシータグでヒットした直近N件」の実数。
- **(b) 属性フィルタ** — **成立（主因）**。canonical クエリは `sm_scope=personal` メタデータ必須（17641–17645）＋クライアント側再検証（17151）。`sm_scope` を持つのは本プラグイン v2系の `add` モード経由のみ（18650）。会話自動保存 `ingestConversation` はメタデータに type/conversationId 等のみで `sm_scope` を付けない（17956–17962 → addMemory の mergedMetadata は sm_source/sm_client/sm_plugin_version/sm_capture_mode を自動付与するが sm_scope は自動付与しない＝17823–17829）。旧バージョンや他クライアント（Copilot 等）が書いたエントリも `sm_scope` 欠落 → スコープ付きクエリでは **構造的に不可視**。
- **(c) 重複/削除の隠蔽** — **部分的**。dedupe キーが本文優先（`content:…`、17603–17608）なので同一 summary のエントリは1件に縮約される。ただし今回の8件は本文が全て異なり、主因ではない。
- **(d) スコープラベル不一致（保存先の違い）** — **成立（欠落の帰結）**。既知≥13件のuser系IDが1件も出ないこと＝これらは「canonicalタグに sm_scope=personal で在る」も「personalReads のレガシータグに在る」も満たさない。レポート上の「user/W3-line entries」という系譜分類と、API上の `sm_scope=personal` 指定は **同一ではない**。

### 1.4 判定（V67）

**8件は「ページング不足」ではなく、`sm_scope=personal` メタデータフィルタ（＋レガシータグ範囲）で選ばれた直近N件（上限20・非継動）の実ヒット数であり、既知≥15件のuser系エントリはスコープ属性/保存タグを欠くため構造的に不可視 — 仮説(b)属性フィルタが主因、(d)スコープラベル不一致がその帰結、(c)は副次的、(a)は却下。**

---

## §2 V74: ハブ名の出現件数（search 1回ずつ、計6回）

search の構造的前提（コード）: API limit = `CONFIG.maxMemories` 既定 **5**（`index.js:17333` → `17714` で伝播）、semantic threshold=0.55（17332）、ツール層の `formatSearchResults` は `slice(0, limit ?? 10)`（18784）。ツールの search には limit 引数が無く（18603–18606）**5件キャップはユーザー側で変更不可**。⇒ 返却は常に≤5件。

| # | ハブ文字列 | 自ツール件数 | search返却 | 返却ID | 返却本文に当該文字列を明示含有 | 判定 |
|---|---|---|---|---|---|---|
| 1 | `0xB0AF48` | 7 | 5件 | `quRSuQPE3rqLRyDdz65tFV`, `Xqd31cbchggfheNmWENNq9`, `f8a7637a97efa7353988d3`, `DUmPz7YPAeH5sYr8Ey3MQ7`, `jM3osaKUPZr7xS3NhcpE9g` | 4/5（`jM3os…` は 0x4FAE80 言及のみ） | **少ない（過小検出あり）**。5<7 かつ構造的上限5のため在庫7件は原理的に検索不能。 |
| 2 | `0x4FDE30` | 6 | 5件 | `Max4FmucWDUznnHfS33VQn`, `7fcA8qW4UTjEdB4Kkwxvs6`, `mLHwanLJ82SFJLauNXip2Q`, `e639a7b29bc5e1166ec07a`, `jM3osaKUPZr7xS3NhcpE9g` | 0/5（`0x004FDE30` 表記=同アドレスが2件、文字列完全一致は0） | **少ない（過小検出あり）**。5<6、同アドレス表記ゆれでも2<6。 |
| 3 | `DAT_00b42f48` | 4 | 5件 | `Xqd31cbchggfheNmWENNq9`, `vKTaGrthYkFRQYkR3Y92UW`, `vYEhexR6dzSNmUdMM9KtZy`, `LWg5zfKFt3R1D1vNrECM5X`, `5d4262c91e7707f5277579` | 2/5（他3件は `0x00b45xxx` = 別アドレスへのセマンティックノイズ） | **実質少ない（過小検出あり）**。明示2<4、返却総数はノイズ込みで4以上に見える。 |
| 4 | `0x4FAE80` | （自ツール数は本タスク未提示／旧searchベース=3） | 5件 | `C7UzKyi7tLiEmEkSPPWju4`, `qAdpKbs3hcuHpcfnz64Z5S`, `jM3osaKUPZr7xS3NhcpE9g`, `mLHwanLJ82SFJLauNXip2Q`, `a3d3f8739de4152b4e9b2a` | 5/5 | **このクエリでは過小なし**（5≥旧3）。ただしこれも上限5到達であり在庫>5なら構造的過小。実数判定は自ツール数不明のため保留。 |
| 5 | `0x413A90` | （自ツール数は本タスク未提示／旧searchベース=3） | 5件 | `K6RK4BwLWzed7oiusUNnRC`, `tSwK8rZG6a4qQau2zgYmkU`, `4rp9LnfGXa1AZZ3knCYLFS`, `gyVCCBw4yTTTiBi5DH5RM5`, `cf18898a39a4b5b781e0fe` | 5/5 | **このクエリでは過小なし**（5≥旧3、旧3は下限近似だったことが逆証される）。自ツール数不明のため実数判定保留。 |
| 6 | `0x504050` | （自ツール数は本タスク未提示／旧searchベース=3） | 5件 | `W3nw6zhipcUrRX41ihe7Gb`, `cFbowXLBUmgDf6KS2jhrLd`, `qbJ8DfpNFf9GDz8gLgV8mq`, `iBjYJQZRUqVra9mhHtmnYv`, `e639a7b29bc5e1166ec07a` | 1/5（`iBjY…` のみ。他は 0x50AF90/0x50A240/0x50A150 等の近接ノイズ） | **少ない（過小検出あり・深刻）**。明示1件は旧searchベース3すら下回り、返却5件の大半は無関係。 |

**総括（V74）**: 「search件数は下限近似であり、ハブ在庫の過小検出あり」— 6ハブ中4ハブ（`0xB0AF48`, `0x4FDE30`, `DAT_00b42f48`, `0x504050`）で明確な過小検出を確認。残2ハブ（`0x4FAE80`, `0x413A90`）は当該クエリでは全5件が当該文字列を含有したが、これは**上限5到達**であり在庫実数が5以下の保証はない。加えてセマンティック検索は近接アドレス（`0x00b45xxx` が `0x00b42f48` に、`0x50Axxx` が `0x504050` に混入）を返すため、**search件数は「≤5の下限近似かつノイズ混入あり」**であり在庫カウント用途には不適。

---

## §3 残課題・完全列挙の方法

1. **userスコープの全件列挙**: `list` に offset/page が無いため（17123–17128）ページングでは網羅不可。代替:
   - ツールは `limit` 引数を受ける（18615）ので `supermemory mode=list scope=user limit=100` でキャップ引き上げは可能。ただし V5 の本文ハイドレーション予算（約29.75秒、17122）により大 limit 時は content が欠落するリスクあり。
   - `sm_scope` 欠落エントリ（旧バージョン/会話保存/他クライアント系＝`cdc31ca…` 系を含む既知≥13件）は scope 付きクエリでは原理的に不可視。特定には canonical タグ・レガシータグに対する **scope 無し**の直接 API クエリ（SDK 直叩き）が必要。既存ダンプ（前セッションの sm_dump.txt、76件）が最も確実な在庫台帳。
2. **search の完全列挙**: ツール search は limit 引数を持たず5件固定キャップ（18603–18606, 17333）。網羅が必要なら (a) SDK 直叩きで limit を引き上げる、(b) 表記ゆれクエリ（`0x4FDE30`/`0x004FDE30` 等）を追加する、(c) `searchMode`/threshold の調整、のいずれかが必要。同一ハブでも実行ごとに返却が変わり得る（本タスクでも旧「3件」が今回「5件」に変化）ため、件数は必ず実行条件とセットで記録すること。
3. **メタデータ完全性**: `addMemory` は `sm_source/sm_client/sm_plugin_version/sm_capture_mode` を自動付与するが `sm_scope` は自動付与しない（17823–17829）。今後 `ingestConversation` 経由で増えた記憶は user/project どちらのスコープ検索でも取りこぼす。プラグイン側修正（ingest 時の sm_scope 付与）を提案可能だが本調査では変更しない。
4. 本調査は一切の書き込み（add/forget/ファイル改編・git）を行っていない。作成物は本ファイルのみ。

---
（V67 list 実行: 1回 / V74 search 実行: 6回 / 合計7回、リトライなし）
