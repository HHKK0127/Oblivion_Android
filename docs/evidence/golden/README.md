# docs/evidence/golden — ゴールデン（数値グリッド / ホスト smoke JSON）

`scripts/compare_golden.py` が比較する基準。**生の PNG は置かない**（Bethesda の
アセットが写るため再配布不可）。

このディレクトリには用途の異なる **2 つのゴールデン**がある。混同しないこと。

| ファイル | 何の基準か | 比較方法 | 使うジョブ | 状態 |
|---|---|---|---|---|
| `newgame-no-esm.json` | host smoke の**状態 JSON** | 完全一致 | `smoke-host`（**blocking**） | 採取済み |
| `newgame-no-esm-prefix.json` | エミュレータ画面の **32x18 輝度グリッド** | グリッド差 | `smoke`（emulator・warn-only） | 未検証のピン（V688a park） |

`compare_golden.py` の**既定**（`--golden` 省略時）は PNG 経路用の
`newgame-no-esm-prefix.json`。host smoke は `--golden docs/evidence/golden/newgame-no-esm.json`
を**明示**して使う（既定ではない）。

## 採取（人がレビューしてからコミットする）

```
# エミュレータ画面（PNG）→ newgame-no-esm-prefix.json（compare_golden.py の既定）
python scripts/compare_golden.py artifacts/smoke-newgame.png --update
```

## 現状（V688）

**エミュレータの PNG ゴールデン `newgame-no-esm-prefix.json` は現状のピンであり、正解基準
ではない**（`docs/evidence/README.md` 参照）。V688a park（hosted runner に hypervisor 無し）で
emulator が起動しないため、Android CI の `smoke` ジョブは `continue-on-error: true`（warn-only）
かつ `if: github.event_name != 'push'`（workflow_dispatch / pull_request のみ）で残す。
self-hosted runner（KVM/HVF あり）が用意できたら検証して blocking に戻す（`--require` は
**外さない**: 外すと golden を入れても落ちなくなる）。

**既定経路の blocking ゲートは host smoke の JSON ゴールデン `newgame-no-esm.json` が担う**
（次節。採取済み）。

## ホスト smoke の JSON ゴールデン（`newgame-no-esm.json`）

`tools/host_tests/newgame_smoke` が固定フィクスチャ ESM で chargen を再現し、状態を
JSON で出す（V688a の代替・A 案）。`scripts/compare_golden.py` は候補が `.json` のとき
**完全一致**で比較する（PNG は従来の 32x18 グリッド比較のまま）。

- **フィクスチャ**: `docs/evidence/fixtures/minimal.esm`（合成・Bethesda データ非含有。
  `tools/host_tests/make_fixture_esm.py` が決定論的に生成、sha256 は再実行で不変）。
- **決定性**: 固定 seed（`12345`）・時計なし・相対パス・固定キー順・`%.6f`。CI は同一
  ジョブで 2 回実行し `diff` で一致を確認してから比較する。
- **golden は CI（ubuntu / linux-gnu）産のみ**。手元 Windows の MSVC 産は混ぜない
  （triple 差で浮動小数がずれ得る）。
- **採取（CI linux-gnu 産）**: `bash tools/host_tests/run_smoke.sh > out.json` →
  `python scripts/compare_golden.py out.json --update --golden docs/evidence/golden/newgame-no-esm.json`。
- **採取済み**: 初回緑（run `45339668361` / commit `e245f9db`）の artifact
  `newgame-smoke-json` をそのまま固定した。**現状の挙動（既知の未実装・バグを含む）
  を固定するピンであり、正解基準ではない**。`smoke-host` ジョブは blocking
  （`continue-on-error` なし）。再採取は意図した変更のときだけ `--update` してレビューする。

### 更新手順（blocking 後）

`smoke-host` が赤くなったら、**まず想定内の変化か**を確認する。想定外ならコードを直す。
想定内（意図した変更）のときだけ golden を差し替える。

1. **意図した変更だと確認する**: 失敗ログの `compare_golden` 差分（`$.*` の一覧）を読み、
   どのフィールドがなぜ変わったかを説明できること。説明できない差分は想定外 → コードを直す。
2. **artifact を取得する**: その run の `newgame-smoke-json` を落とす。
   `gh run download <run-id> -n newgame-smoke-json -D /tmp/smoke`
3. **golden を差し替える**:
   `cp /tmp/smoke/out1.json docs/evidence/golden/newgame-no-esm.json`
   （または `python scripts/compare_golden.py /tmp/smoke/out1.json --update
   --golden docs/evidence/golden/newgame-no-esm.json`）。
4. **golden 更新だけのコミットに分ける**（コード変更と混ぜない）。メッセージに、どの
   変更でなぜ差分が出たかを書く。
5. **差分を目視**して「想定外の変化が無い」ことを確認する。チェック項目:
   - `sliders` が **90 個**のままか（数が変われば要素の追加/削除）。
   - `attributes`(8) / `skills`(21) / `spells` の変化が、その変更で説明できるか。
   - `pageCount`=21 / `itemCount`=118 が意図せず動いていないか。
   - 数値が `%.6f` 形式のままか（桁が増えていれば出力側の変更）。
