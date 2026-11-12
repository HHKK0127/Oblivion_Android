# バッチレビュー: 全28件並行解析の完了 Review（2026-10-27）

ルール（loop-constraints.md 2026-10-26 追記）に基づく Review。対象 = TODO_re49.md の V系列 28 件。

## 1. 完了サマリ（27/28 完了 + 1 保留）

| 区分 | 件数 | 備考 |
|---|---|---|
| 今ラウンド（opencode 並行サブエージェント） | 21 件 | Wave1–4（V116–V142、V31 除外） |
| Copilot 並行完了表記 | 6 件 | V116/V117/V118/V119/V128–V132 等の一部で双方完了を確認 |
| 保留 | 1 件 | **V31**（MEDT パッチ＝移植セッション適用待ち、分析自体は完了済み） |

成果物は **tmp/re/ に24本**（v_data_relmap.md 関係性マップ含む、計 ~840KB）。全件 probe_harness selftest 4/4 PASS、round_verify ALL OK、docs/・共有台帳・ESM 本体は無変更。

主な確定成果（抜粋）:
- V126: Steam DRM (.bind 複号→`jmp eax` tail-jump) と CRT startup 接続を**解明確定**
- V127: Cat-3（デコンパイル無領域）=10,261件を定義、gap 10/10 判定
- V130: INI type switch・動的ノード・キー差20を検算一致で確定
- V131/V132: opcode 実行時到達経路（band2/3）と +0x1C 例外4件を全解明
- V133: 全21,597関数の重複を25秒で全数照合（コピペ候補12組24件）
- V139: 0xB333A0=TES グローバル確定（NavMesh説訂正）、bit14=管理外オブジェクト
- V140: E8 新規66件=確定60/保留6、A節64件はサイト検証起因と切り分け

## 2. データ関係性マップ（v_data_relmap.md）のレビュー結果

- **未解消矛盾 18 件**（真に未解消4: 767vs684 口径 / 1,819vs1,818 / b26 slot0-1 / v81 定数、+ 誤り確定済だが編集承認待ち14）
- **データ断層 F1–F9** を特定。トップ: F1 反映断層（成果物→GT/docs/台帳、14件）、F7 ラベル台帳6層未統合、F3 renderer⇄Cat-3 tail、F4 CC/slot⇄opcode dispatcher、F2 ESM⇄EXE 接点

## 3. 新タスク候補（残点統合: 本ラウンド全レポートから約90項目を束化）

| # | タスク（束） | 優先 | 出典（残点数） |
|---|---|---|---|
| V143 | 【要承認】GT 反映バッチ: slot 5件+新規 A–F+§2 追加（GT_CHEATSHEET/RE_EXE_REPORT へ） | 高 | V121/V116/V120(計12) |
| V144 | 【要承認】docs 訂正束: §35.5 atexit・§70.15(6) 前提崩壊・§59.3・b04/b09/b10/b14/b15/b17 誤り確定14件の反映 + V136 (4c) 適用 | 高 | V124/V125/V117/V137/V138/V139/V136(計14) |
| V145 | ラベル/台帳 6層統合住所テーブル（func_list/dec_index/V86/Cat-3/Cat-3b/seed-B を1本化） | 高 | relmap F7 + V127/V133/V140 副産物 |
| V146 | tail 帯域（0x980000+ = Cat-3 の63%）の正体と Ghidra 欠落理由 | 中 | relmap F3 + V127(2) |
| V147 | opcode 実行時残点束: executor arg8/arg9・Begin ペイロード・%変数バイト列・+0x24 argflag 消費者・a2/a3 規則 | 中 | V131(9)/V132(5)/V134(2) |
| V148 | CC/slot ⇔ opcode 書手の双方向マップ（emit this クラス・犯罪メーター・extra 書手） | 中 | relmap F4 + V138(6)/V139(3) |
| V149 | renderer/シェーダ残点束: renderer+0x280 クラス名・WinMain jcc 極性・レンダラ slot0 到達経路 等 | 中 | V129(7)/V128(6) |
| V150 | INI/Setting/Log 残点束: h 語源・GameSetting ノード・MessageHandler ライタ・path 起動順 | 低 | V130(7)/V120(5)/V137(3) |
| V151 | quest/Script/extra 残点束: SysWindow 機能名・bit14 名称確定・装備ブロック手スロット 等 | 低 | V141(9)/V139(4)/V142(5) |
| V152 | E8/台帳ギャップ残点束: C群8件呼出し元・保留6件裁定・MinHash near-dup・本文なし8件 | 低 | V140(6)/V133(8) |
| V153 | GT slot 語義残り: +8 名称・0x2D0 Actor 版用途・0x36C 状態vs種別 等 | 低 | V121 B群(6) |
| V154 | ESM 接点の事前整備（解禁時タスクの仕様確定: CTDA 生バイト/SCDA 22件/SCHR 供給源） | 低 | relmap F2 |

## 4. おすすめ実行順（本レビューの結論）

1. **承認ゲート**: V143+V144（docs/GT 編集はユーザー承認後に親セッション実施 — 分析ゼロ・即着手可で全タスクの前提精度を決める）
2. **V145 統合住所テーブル**（スクリプト1本で F7+F3+口径767vs684 を同時に消す）
3. **V147 opcode 実行時残点** → **V148 CC/slot⇔opcode 双方向**（移植仕様に直結）
4. **V146 tail 帯域** → **V149 renderer 束**
5. **V150–V154**（低優先を隙間時間に）

## 5. 人間ゲート一覧（ルールの Human Gates 適用）

- V143/V144/V136 の docs・GT 編集
- 本番 Supermemory sync（ガード中・pilot 保留）
- B13 ダンプ（adb はユーザー起動のみ）
