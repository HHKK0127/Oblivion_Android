# docs/evidence/golden — 画面ゴールデン（数値グリッド）

`scripts/compare_golden.py` が比較する基準。**生の PNG は置かない**（Bethesda の
アセットが写るため再配布不可）。基準は 32x18 の輝度グリッドを JSON で保存した
派生物（既定 `newgame-no-esm.json`）。

## 採取（人がレビューしてからコミットする）

```
python scripts/compare_golden.py artifacts/smoke-newgame.png --update
```

## 現状（V688）

`newgame-no-esm.json` は**未採取**。それまでは Android CI の `smoke` ジョブが
`continue-on-error: true`（warn-only）かつ `if: github.event_name != 'push'`
（workflow_dispatch / pull_request のみ）で走る（golden 不在だと
`compare_golden.py --require` が exit 1 になるため）。golden をコミットしたら
`android.yml` の `smoke` ジョブから `continue-on-error` と `if:` を外し、blocking に
戻す（`--require` は**外さない**: 外すと golden を入れても落ちなくなる）。

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
