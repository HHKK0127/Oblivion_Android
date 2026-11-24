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
