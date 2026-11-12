# v321 — 次束H（#83）静的解析報告

- 対象: Oblivion Steam 1.2.0416、ImageBase `0x400000`
- 出典: `%TEMP%\opencode\v318_01..06.py/.out`
- 本成果物は既存v318 probe出力の統合であり、EXE・`tmp/re/re_util.py`・ソースは変更しない。
- 判定: `[確定] / [部分確定] / [推定] / [未確定] / [確定負]`

## 1. 判定表

| 項目 | 判定 | 根拠 | 残る不確定点 |
|---|---|---|---|
| `0x502500` dispatch基址 | [確定] | `.data` table start `0xB0AF50`、stride `0x28`、row103 `0xB0BF68`、impl `0x502500`、name `LoadGame <filename>` | 0x4FDE30 helperの正式名 |
| `[0xB33E9C]=195.062652...` | **[確定]（→親訂正 2026-11-07）** | 用途=**タイマ 0xB33E90 の dt（単位=秒[推定]）**・書手 `FUN_0047D170` `fstp [ecx+0xC]`・実初期値 0.0（fldz・0x9DC2A0 初期化）・**ファイル値 195.06 は BSS 超域の .rsrc file bytes ゴミ（fileOff 0x732A9C）**・参照 164 件全数（fld74/fsub52/fadd21/fmul14/fdiv2/fstp1） | 旧「raw f32・単位不明」は v318 §5 で訂正 |
| `0x44DCF0` の0条件 | [確定] | `xor eax,eax` at `0x44E672` と共通ret `0x44E69B`、複数分岐から収束 | 各opcode/入力条件の完全な意味付け |
| 間接call 595件 | [部分確定] | v318 census、log-context 104 sitesを分類、代表的vtable slotを確認 | 595件全件の最終callee・深さ制限外の到達 |
| fn-end walker短CC境界 | **[確定]（失敗型再現＋rule v3）／一般適用=[部分確定]（→親訂正 2026-11-07）** | 失敗型 3/14 再現（`0x45CC60`/`0x571660`/`0x571820`）＋rule v3（ret 後の内部分岐ターゲット判別）で**ラベル 16/16 一致**・旧 walker 誤り 3 件（0x51B490 end/0x502500 size/0x450550 end）を訂正 | sweep 残 45 件の全数検証（v318 §6-3 へ） |

## 2. 根拠

### 2.1 `0x502500` dispatch

- テーブルは `0xB0AF50` から `0xB0C8A0`、162 rows、stride `0x28`。
- row103 (`0xB0BF68`) は:
  - id `0x148`
  - name pointer `0xA509E4` → `LoadGame <filename>`
  - implementation `0x502500`
  - helper slot `0x4FDE30`
- row-table consumer側では `0x4FF892` が table baseを参照する。

よって `0x502500` はopcode/command tableのLoadGame row implementationであることを[確定]とする。これはLoadGame本体の正式C++名を意味しない。

### 2.2 `0xB33E9C`

- `.rsrc` raw位置に対応する値は `0x4343100A` のf32解釈ではなく、probe出力上 `195.06265258789062`。【訂正→v318 §5: この値は BSS 超域の **.rsrc ファイルゴミ**であり f32 実値ではない（fileOff 0x732A9C）・実初期値=0.0・単位=秒[推定]・単位不明は解消】
- `0xB33E90` baseへの参照は9件、`+0x0C`相当値への候補参照は164件。
- `0x47D170` は `[ecx+0x14]`との差分を取り、`[ecx+4]`/`[ecx+8]`をf32計算に使い、グローバル時間値を更新する。

したがって timer/time-step 系の値である可能性は高いが、値の単位・用途は静的証拠だけでは確定しない。参照数には命令境界上の定数誤認候補が含まれ得るため、全164件を意味参照として扱わない。

### 2.3 `0x44DCF0` の0条件

本体範囲は `0x44DCF0..0x44E69E` の主関数として読め、共通のゼロ経路は:

```text
0x44E672: xor eax, eax
0x44E69B: ret 8
```

少なくとも以下の分岐が局所的に `eax=0` を生成し、共通経路へ収束する。

- `0x44DEA6`
- `0x44DF1A`
- `0x44E2ED`
- `0x44E4EF`
- `0x44E672`

【訂正→v318 §5: 上記のうち `0x44DEA6`/`0x44DF1A`/`0x44E2ED`/`0x44E4EF` は **SEH/取得失敗時の内部変数(esi)用で ret に到達しない** — 実際の return-0 は `0x44E672/0x44E674` 経由の **8 経路 F1..F8**（Partial Form 検出失敗/型不一致/[0xB33A9C] NULL×2/ConstructObject 失敗×2/0x447050 al=0×2・ログ文言5種付き・v318 §3 項目3）】

関数はjump tableで多数のopcodeを処理するため、「0」は単一の条件ではなく、opcode別の未該当・失敗・無操作経路の合流値と判定する。各opcodeの意味まで一括で確定するのは避ける。

### 2.4 間接call census

v318では595件を機械的に収集し、log-contextに限定して104 sitesを追加確認した。代表例として、vtable slot `0x4/0x8/0xC/0x14/0x84`、`call eax/edx`、`this` と引数準備を確認できる。

ただし、全595件について:

- 実クラスをRTTIまで確定
- 間接先を一意に解決
- 深さ5留保分を全追跡

するところまでは完了していない。従って「595件を完全解決」とは記載しない。

### 2.5 fn-end walker

v3ルールは短いCC paddingで旧walkerが後続関数へ過走する例を改善した。

- `0x44F0C0`: old `0x44F3C1` → v3 `0x44F3A8`、jump-table境界と整合
- `0x44DCF0`: 共通retは `0x44E69B` だが、機械的v3結果は後続領域まで伸び、一般則として未解決
- `0x450550` / `0x4507D0`: 分割候補を検出するが、隣接関数・参照先確認が必要
- 全sweepでは旧結果とv3結果の差分が多数あり、単純な「CC run長」だけではsound/completeにならない

結論は、v3を「補助候補生成器」として扱い、関数ごとにret、jump-table、直接caller、外部参照を併用して確定することである。

## 3. 残件

1. `0x4FDE30` の正式役割とtable consumerの引数契約。
2. `0xB33E9C` の動的初期値・単位・timer用途。
3. `0x44DCF0` のopcode別0条件。
4. 間接call 595件の全件解決。
5. fn-end walkerの一般則化。v3を自動的な確定器へ昇格させない。

## 4. 再現性

既存v318 probeの保存出力（`v318_01.out`〜`v318_06.out`、派生probeを含む）を根拠とした。既存の各probeは保存時にEXIT=0で完了済み。今回の統合は読み取りのみで、解析対象・ソース・共有helperは変更していない。

## 親による訂正（2026-11-07・v318 統合時）

本報告は同一 #83 への別セッション出力。親 lane の正式成果物 **`v318_v314_residuals.md`（31,670B/205行・probe 17 本）との突合で 3 点の訂正**を本ファイルへ inline 適用した（v318 §5 の裁定: 本束 v318 を正とする・本ファイルは削除せず併記）:

1. **`[0xB33E9C]` の 195.06**: ファイルゴミ（.rsrc fileOff 0x732A9C）と断定確定・用途=タイマ dt・単位=秒[推定]・初期値 0.0 — 旧「raw f32・単位不明 [部分確定]」から [確定] へ（表行と §2.2 を修正）。
2. **`0x44DCF0` の return-0 経路**: 0 条件リストの `0x44DEA6/0x44DF1A/0x44E2ED/0x44E4EF` は SEH/内部変数用で ret 非到達 — 実際は `0x44E672/0x44E674` 経由の 8 経路 F1..F8（§2.3 に修正挿入）。
3. **fn-end walker**: 失敗型 3/14 再現＋rule v3 でラベル 16/16 一致により、失敗型再現面は [確定]（一般適用は[部分確定]へ分割・表行を修正）。

残る相違はなし。整合点（0x502500 表 0xB0AF50/stride 0x28・0x44DCF0 範囲・ret 0x44E69B・595 census の引用）は v318 と一致。
