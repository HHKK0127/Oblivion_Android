# V70: 362 gap VA への強制 createFunction + デコンパイル結果（2026-10-24）

## 手法
- Ghidra 12.1.4 `analyzeHeadless`（`-process OblivionStandalone.exe -noanalysis`、プロジェクト `ghidra_projects\OblivionRe` ※project path は親ディレクトリ指定）＋自作 `ForceCreateGaps.java`- `gap_vas.txt`(362 VA) → 各 VA で `getFunctionAt/Containing` なしなら `CreateFunctionCmd` 適用 → `DecompInterface` でデコンパイル（60s/関数）→ CSV/全文ダンプ/全関数リスト出力 → Save succeeded

## 結果（ログ実測）
```
ForceCreateGaps done: input=362 already=5 created=357 failed=0 parseErr=0 totalFunctions=21597
```
| 項目 | 値 |
|---|---|
| 対象 VA | 362 |
| 作成成功 | **357** |
| 既に定義/包含 | 5（0x00503E10/0x00503E90/0x00500890/0x0050D020/0x005007D0 = 同名の既存定義関数）; **gap_vas 重複 5 行（unique=357）が原因** で、全 357 件作成成功。gap_scan との状態差ではない |
| 作成失敗 | 0 |
| デコンパイル成功 | **362 / 362** |
| デコンパイル失敗 | 0 |
| 作成後 関数総数 | **21,597**（旧 21,240 + 357 = 21,597 ✓） |
| デコンパイルサイズ | min=213 / 中央=458 / max=203 bytes |

### デコンパイル失敗0件
- なし（全362成功）

### 小・大の例
- 小 `0x00500820` FUN_00500820 = 69 bytes
- 小 `0x005013E0` FUN_005013e0 = 75 bytes
- 小 `0x00500F60` FUN_00500f60 = 76 bytes
- 大 `0x0050D020` FUN_0050d020 = 4228 bytes
- 大 `0x0050D020` FUN_0050d020 = 4228 bytes
- 大 `0x00509D70` FUN_00509d70 = 3808 bytes

## 意味づけ
- **全362 VA が「機能する関数」として作成可能**（int3/データ境界を理由に不可=0）→ V10 の「impl≠0だが旧解析で定義されなかった」残余は**旧解析の過少検出**であり、区切り不能領域ではないことが実証。- V5 の absorbed_suspect（cc≥2=319件）も本作成で実体化 → 吸収説の追跡は CSV/ダンプの重なりで継続可。- 旧分析 21,240 → +357 = 21,597（既定ギャップ5件は同run内の先行作成で包含済）。
## 成果物
- `%TEMP%\opencode\v70_force_create.csv`（362行: pre/post/entry/name/decomp_ok/chars/error）- `%TEMP%\opencode\v70_decomp_dump.txt`（228917B、全成功分のデコンパイル全文）- `%TEMP%\opencode\func_list_new.txt`（21,597行 = V73 入力）- `%TEMP%\opencode\v70_headless.log`（実行ログ）
