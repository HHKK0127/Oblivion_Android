# V15 / V20 / V22 台帳監査 — Oblivion.exe v1.2.0416 (Steam版)

> 作成日: 2026-10-24
> 対象: `docs\RE_EXE_REPORT.md` §67（11,778行・CRLF・BOM なし）/ `tmp\re\svm_495_miss_addrs.md`（504行）
>       / `tmp\re\v5_ghidra_gaps_report.md`（599行）/ `tmp\re\v10_impl_decompile_join.csv`（528行=ヘッダ+527）
>       / `tmp\re\v4v9_unresolved_report.md`（184行）
> バイナリ: `C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Oblivion.exe`
>       （SHA-256 `a8f313845c1545e9a60e1e995961eef4c033115da9443f6d756341df3c2b7dc6`、
>       `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` と**完全一致**）
> マッピング: ImageBase=0x400000 / .text VA=0x1000 Raw=0x400 / .rdata VA=0x628000 Raw=0x627200 / .data VA=0x702000 Raw=0x700C00
> 本監査は**読み取り専用**。既存ファイルの編集は一切行っていない（編集案は §6 に別掲・未実施）。
> 使用スクリプト: `%TEMP%\opencode\v15_v20_v22_audit.py` / `v15_v20_v22_audit2.py` / `v15_why22.py` / `v15_probe15.py` / `v15_final_data.py` / `v20_ledger_v5.py`

---

## 1. セルフテスト（テーブル走査の妥当性）— 5/5 PASS

| # | 検証内容 | 期待値 | 実測値 | 結果 |
|---|---------|--------|--------|------|
| 1 | `bytes@0x504050` | `518b4c24188b5424` | 同左 | **PASS** |
| 2 | `bytes@0x4FB580` | `578b7908` | 同左 | **PASS** |
| 3 | `string@0xA7035C` | `.\AI\CombatController.cpp` | 同左 | **PASS** |
| 4 | テーブル532件走査 + opid `0x0015` の idx/impl | idx=21, impl=`0x00504560` | idx=21, impl=`0x00504560` | **PASS** |
| 5 | opid `0x0015` の nameptr 実読 | `OnLoad` | `OnLoad` | **PASS** |

- 走査方法: 基底 `0xB0AF48`、stride `0x28`、532エントリ。`+0x00`=nameptr(u32)、`+0x08`=opid(u16)、`+0x18`=impl(u32) を PE セクション変換経由で実読。nameptr は NUL 終端 ASCII として `.rdata` から実読。
- 最終エントリ: idx=531 VA=`0x00B10240` opid=`0x1170` impl=`0x00507620` name=`ClearOwnership`（§67.1 の「opcode 範囲 0x0000〜0x1170」と整合）。
- **テーブル内の opid 重複は 0 件**（532 エントリ = 532 ユニーク opid）。

---

## 2. V22: §67 台帳表の行数・重複検証

### 2.1 行数カウント（実測）

| 表 | 範囲（行番号） | データ行数 | 除外したヘッダ/区切り行 |
|---|---|---|---|
| §67.4 頻度上位50件 | L10858〜L10907 | **50** | L10856（ヘッダ）/ L10857（`|---|`） |
| §67.6 全480件 | L10920〜L11399 | **480** | L10918（ヘッダ）/ L10919（`|---|`） |
| 合計 | — | **530** | 4行 |

- 抽出規則: `| <rank数字> | 0x<hex3-4桁> |` で始まる行のみをデータ行とし、`| rank | OpID |...` ヘッダと `|---|` 区切りは除外。§67.1/§67.3 の小表（頻度帯等）は2列構造で opid 列を持たないため規則に合致せず自動除外。
- 原表 `tmp\re\svm_495_miss_addrs.md` は 480 データ行・480 ユニーク opid・**§67.6 と opid 集合が完全一致**。

### 2.2 重複の内訳

| 指標 | 実測値 |
|---|---|
| 総データ行 | 530 |
| ユニーク opid | **480** |
| 重複して出現する opid | **50**（全て §67.4 に出現し、§67.6 にも再掲） |
| §67.6 内部の重複 | **0** |
| 重複ペアの値差異（name/impl/size/freq/calls） | **0 件（50/50 が完全同一行）** |
| §67.4 にあって §67.6 に無い opid | **0** |

重複50件の一覧（§67.4 行 → §67.6 行、値は全て identical）:

| opid | 名称 | §67.4 | §67.6 | opid | 名称 | §67.4 | §67.6 |
|---|---|---|---|---|---|---|---|
| 0x0015 | OnLoad | L10858 | L10920 | 0x1056 | StartConversation | L10876 | L10938 |
| 0x0019 | OnStartCombat | L10859 | L10921 | 0x1123 | PlayMagicShaderVisuals | L10877 | L10939 |
| 0x001C | OnActorEquip | L10860 | L10922 | 0x1064 | PickIdle | L10878 | L10940 |
| 0x0010 | OnPackageDone | L10861 | L10923 | 0x112B | DisableLinkedPathPoints | L10879 | L10941 |
| 0x001D | OnActorUnequip | L10862 | L10924 | 0x1146 | Reset3DState | L10880 | L10942 |
| 0x0011 | ScriptEffectStart | L10863 | L10925 | 0x1053 | ModDisposition | L10881 | L10943 |
| 0x0018 | OnTrigger | L10864 | L10926 | 0x105A | SetAlert | L10882 | L10944 |
| 0x0017 | OnSell | L10865 | L10927 | 0x10EE | EquipItem | L10883 | L10945 |
| 0x101D | RemoveSpell | L10866 | L10928 | 0x1124 | PlayMagicEffectVisuals | L10884 | L10946 |
| 0x1000 | MessageBox | L10867 | L10929 | 0x1025 | PlaceAtMe | L10885 | L10947 |
| 0x109E | MoveToMarker | L10868 | L10930 | 0x112A | EnableLinkedPathPoints | L10886 | L10948 |
| 0x1052 | RemoveItem | L10869 | L10931 | 0x1117 | SetOwnership | L10887 | L10949 |
| 0x101C | AddSpell | L10870 | L10932 | 0x1017 | StopCombat | L10888 | L10950 |
| 0x1033 | Say | L10871 | L10933 | 0x1125 | StopMagicShaderVisuals | L10889 | L10951 |
| 0x1059 | Message | L10872 | L10934 | 0x105C | Look | L10890 | L10952 |
| 0x1097 | AddScriptPackage | L10873 | L10935 | 0x10EC | SetGhost | L10891 | L10953 |
| 0x1016 | StartCombat | L10874 | L10936 | 0x10CC | SetDestroyed | L10892 | L10954 |
| 0x1034 | SayTo | L10875 | L10937 | 0x108C | ResurrectActor | L10893 | L10955 |
| 0x1061 | DisablePlayerControls | L10894 | L10956 | 0x10A8 | SetFactionReaction | L10903 | L10965 |
| 0x10DD | SetOpenState | L10895 | L10957 | 0x1144 | TriggerHitShader | L10904 | L10966 |
| 0x1058 | AddTopic | L10896 | L10958 | 0x1129 | SetActorAlpha | L10905 | L10967 |
| 0x1060 | EnablePlayerControls | L10897 | L10959 | 0x105D | StopLook | L10906 | L10968 |
| 0x10B2 | PlaySound3D | L10898 | L10960 | 0x1104 | SetWeather | L10907 | L10969 |
| 0x1003 | SetEssential | L10899 | L10961 | 0x1101 | StopCombatAlarmOnActor | L10900 | L10962 |
| 0x112D | ForceWeather | L10901 | L10963 | 0x1142 | Dispel | L10902 | L10964 |

### 2.3 判定: 「530行 = 480 + 50」説は**正しい**

- §67 内のオペコード台帳表（§67.4 + §67.6）のデータ行は実測 **530 行**、ユニーク opid は **480**、重複行は **50**（§67.4 の全50行が §67.6 への再掲、値差異ゼロ）。
- すなわち「530 = 480 ユニーク + 50 重複」は**実数で確認**。重複の正体は「頻度上位50件の §67.4 への二重掲載」であり、opid の誤登録でも値の食い違いでもない。
- 行数カウントの定義: データ行のみを数え、ヘッダ行・`|---|` 区切り行（計4行）を除外した。これ以外に §67 内の opid 列を持つ表は無い。
- 付記: 台帳は impl が非ゼロの 480 opid（ユニーク impl は 474 — OnEquip/OnActorEquip 等6組の impl 共有がある）。§67.3 の内訳（block 29 / mid 124 / cmd 327 = 480）も実データと一致。

---

## 3. V15: 台帳非掲載 opid — 実数は **52 件**（「8件」説は誤り）

### 3.1 差集合の実数

- テーブル 532 エントリは全て**異なった opid**（テーブル内重複 0）。
- 台帳ユニーク opid = 480 → **テーブル − 台帳 = 52 opid**。
- 背景の「8件」は V4/V9 報告の**部分集合**（直接E8ゼロ20件のうち台帳非掲載だった8件: 0x0141/0x0173/0x101F/0x10DC/0x110C/0x114A/0x115F/0x1160）を全体の差集合と取り違えたもの。**訂正: 全体差集合は 52 件**。
- 逆方向（台帳にあってテーブルに無い opid）は **0 件**。

### 3.2 非掲載52件の理由内訳

| 理由 | 件数 | 根拠 |
|---|---|---|
| impl == 0（ハンドラ実体なし） | **5** | 0x0183 哨兵 `ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!` / 0x1019 UnusedFunction1 / 0x101A UnusedFunction2 / 0x1035 GetScriptVariable / 0x104F GetQuestVariable |
| annotate_db.json 登録済み（§67.0 の明記された除外規則） | **25** | impl VA が annotate_db functions キー（hex 小文字）と一致。台帳480件の impl と annotate_db の重複は **0件** = 除外規則は正しく機能 |
| 共有スタブ impl `0x977C50`（`mov al,1; ret` 3B、V9 K1） | **7** | 0x0141 TestCode / 0x0173 BeginTrace / 0x10DC ShowNameMenu / 0x110C TrapUpdate / 0x114A AddAchievement / 0x115F StartMasterFileSeekData / 0x1160 DumpMasterFileSeekData |
| **理由未特定（UNDET）** | **15** | 下表 |
| 合計 | **52** | |

### 3.3 理由未特定15件の一覧

| opid | idx | name | impl | V5判定 | V10判定 | 備考 |
|---|---|---|---|---|---|---|
| 0x0016 | 22 | OnMagicEffectHit | 0x005045E0 | 欠落(cc×8) | ghidra_missing | block-type。SCDA頻度10,480で**台帳の頻度順なら rank2** なのに不掲載 |
| 0x001E | 30 | OnReset | 0x005045A0 | 欠落(cc×8) | ghidra_missing | block-type |
| 0x0117 | 54 | PickRefByID | 0x00513E90 | present | present | decompiled に FUN_ 定義あり |
| 0x0158 | 119 | SetTargetRefraction | 0x00507E60 | present | present | 同上 |
| 0x101F | 194 | GetButtonPressed | 0x005072B0 | 欠落(cc×8) | ghidra_missing | V9 K4 で分析済み（104B ビットテスト） |
| 0x10C5 | 360 | GetPCFactionSteal | 0x00505540 | present | present | |
| 0x10C9 | 364 | GetPCFactionSubmitAuthority | 0x00505600 | present | present | |
| 0x10D9 | 380 | ShowClassMenu | 0x00505810 | 吸収((1)) | absorbed_suspect | V5/V10 一致 |
| 0x10DA | 381 | ShowRaceMenu | 0x00505850 | 欠落(cc×8) | ghidra_missing | |
| 0x10DB | 382 | ShowBirthSignMenu | 0x00505870 | 欠落(cc×8) | ghidra_missing | |
| 0x10F2 | 405 | GetUnconscious | 0x00505B50 | 欠落(cc×6) | ghidra_missing | |
| 0x111B | 446 | SetCellFullName | 0x00512AE0 | 欠落(cc×8) | ghidra_missing | |
| 0x113D | 480 | ModScale | 0x00501E40 | 欠落(cc×8) | ghidra_missing | |
| 0x1154 | 503 | SetActorRefraction | 0x0050E340 | present | present | |
| 0x115B | 510 | SetNoAvoidance | 0x00501390 | 吸収((1)) | absorbed_suspect | V5/V10 一致 |

- 検証済み非仮説: impl VA 範囲外でない（全て 0x501390〜0x513E90）。cc 前置の有無では分類不能（cc=1〜15 に分散、gate 0x4FAE80 呼出の有無でも分類不能 — gate あり9件/なし6件、且つ台帳側にも gate なしハンドラは12件存在）。Android 側 `script_functions.cpp` / `native_scda_bridge.cpp` / `script_opcodes.h` にもこれら15件の実装/マッピングは**無い**。
- 推定: 台帳生成は Copilot セッション内で行われ**生成スクリプトが残存しない**。annotate_db 除外（25件）＋ impl=0（5件）＋共有スタブ（7件）で 37件が説明でき、残り15件は生成時の過少選択（または当時の annotate_db に登録済みで後に削除された可能性）と見られるが**未確定**。V9 報告 §4 の未解決項3「台帳非掲載8 opid の位置付け」は本調査で 7件（スタブ）＋1件（0x101F、UNDET15に内包）に分解でき、**残る未解決は UNDET15**。

### 3.4 台帳480件の内訳（検算）

- impl 非ゼロ 527 = 台帳480 + 非掲載47（impl=0 の5件は 527 に含まれない）。
- 非掲載47（impl≠0）の V10 判定内訳: present 24 / ghidra_missing 16 / absorbed_suspect 7。
- 台帳480の V10 判定内訳: present 141 / ghidra_missing 244 / absorbed_suspect 95。
- 合算検算: 24+16+7+141+244+95 = **527** ✓

---

## 4. V20: V5 / V10 / 台帳 / テーブルの定義統一

### 4.1 1枚の対応表

```
テーブル 532 エントリ（opid ユニーク 532）
├─ impl == 0 ──────────────── 5 件（哨兵/未使用/間接ディスパッチ。以降の母数から除外）
└─ impl ≠ 0 ───────────────── 527 件（ユニーク impl 515 — 7組の impl 共有）
    ├─ decompiled.c に FUN_ 定義あり（V10 present）────── 165 件
    └─ FUN_ 定義なし = GAP ─────────────────────── 362 件
         ├─ CC前置 ≥ 4 バイト ────────────── 260 件 ＝ V10「ghidra_missing」
         │    （V5 でも「欠落」）
         ├─ CC前置 2〜3 バイト ──────────────  59 件 ＝ V5「欠落」/ V10「absorbed_suspect」
         └─ CC前置 0〜1 バイト ──────────────  43 件 ＝ V5「吸収」/ V10「absorbed_suspect」
```

- **V5 の 362 と V10 の 362 は同一集合**（impl 単位で突合し完全一致、差集合両方向ゼロ）。V10 の `in_decompile=no` 362件 = V5 のギャップリスト362行 = 357 unique impl（5件は impl 共有）。
- 違うのは**小分類の閾値だけ**: V5 は「CC前置 ≥2 → 欠落 / <2 → 吸収」で 319/43、V10 は cc_prefix フラグ（実態は **CC前置 ≥4** で yes）で 260/102。クロス表実測:

| V5判定 \ V10判定 | ghidra_missing | absorbed_suspect | 合計 |
|---|---|---|---|
| 欠落 | 260（cc×4〜×8） | **59**（cc×2=43, cc×3=16） | 319 |
| 吸収 | 0 | 43（(0)=24, (1)=19） | 43 |
| 合計 | 260 | 102 | 362 |

- 台帳480軸との突合（V5判定）: 台帳480 = V5-present 141 + 欠落 301 + 吸収 38。非掲載47 = present 24 + 欠落 18 + 吸収 5。→ **「台帳（Android移植 MISS）」と「ギャップ（Ghidra 欠落）」は独立した2軸**であり、台帳480 ⊄ ギャップ362（台帳の141件は decompiled に定義あり）。

### 4.2 推奨する統一定義（1案）

> **ギャップ（Ghidra gap）:= オペコードテーブル由来の impl VA（+0x18、非ゼロ、527件）のうち、`docs\Oblivion_decompiled.c` に `FUN_{impl:08X}` のlowercaseラベル定義が存在しないもの。実数 362 件（357 unique impl）。**

対応表（既存数字の再解釈）:

| 既存の数字 | 出典 | 統一定義での読み方 |
|---|---|---|
| 362 | V5 §3 | **統一定義のギャップ総数**（これ自体は V5/V10 で一致・改訂不要） |
| 319 / 43 | V5 §6（missing/absorbed） | 統一**サブ分類の推奨値**。基準「直前の連続 int3 ≥2 → 独立関数だがラベル未生成 / <2 → 前関数に吸収」。int3 パディングは関数境界の実証であり V10 の ≥4 閾値より妥当 |
| 260 / 102 | V10 / §67.6.1 | 同一362集合の**閾値違いの別分類**（≥4）。59件（cc×2〜×3）を absorbed_suspect に過大に含む。**§67.6.1 からは置換を推奨** |
| 165 / 527 | V5 / V10 一致 | present 率 = decompiled 突合可能数。527−165=362 |
| MISS 480 | §67 | **別軸**（annotate_db 登録済みを除く Android 未移植ハンドラ）。ギャップ362とは交差するが包含関係にない（480 ∩ gap = 339、480 ∩ present = 141） |
| 530 = 480 + 50 | §67.4+§67.6 | 行数の二重掲載（V22）。opid 集合の議論では常に 480 を使うこと |

---

## 5. RE_EXE_REPORT へ入れるべき訂正/追記の具体案（**未実施**）

> ⚠️ 本節は提案のみ。実行時は規約に従うこと:
> - `docs\` 編集は python `io.open(..., "rb"/"wb")` バイトモードで行い、**CRLF（11,778個・末尾CRLFあり）/ BOM なし**構成と行数を維持する。
> - 編集前に必ず `RE_EXE_REPORT.md.bak13` を作成（既存は .bak〜.bak12 まで存在済み → **次の番号は .bak13**）。
> - Ghidra ラベル照合は必ず lowercased。
> - `docs\` への編集は本監査では行わず、ユーザー承認後に別途実施する。

| # | 位置（現行行番号） | 操作 | 文面案 |
|---|---|---|---|
| 1 | L10789（§67.0 末尾） | 追記 | 「MISS 480 = テーブル impl≠0 527 − annotate_db 登録 25 − 共有スタブ impl 0x977C50 7 − 理由未特定 15 − impl==0 は母数外 5。非掲載 opid 全体は 52 件（詳細は tmp\re\v15_v20_v22_ledger_audit.md §3）。」 |
| 2 | L10807（§67.1 表の「MISS 件数」行） | 注記追加 | 値 480 は維持。備考に「※テーブル非掲載 52 opid あり（impl=0: 5 / annotate_db: 25 / スタブ: 7 / 未特定: 15）」を追加 |
| 3 | L10854（§67.4 見出し直下） | 注記追加 | 「※本表の50行は §67.6 の同一 opid 行の再掲（値差異なし）。§67 内の台帳表は 530 行 = 480 ユニーク + 50 重複。」 |
| 4 | L11404〜L11410（§67.6.1 表） | **置換** | 260/102 を「V10 旧分類（CC前置≥4閾値）」と明記し、推奨サブ分類として 319/43（V5、CC前置≥2）を併記。「362 の集合自体は V5/V10 で一致（impl 単位突合済み）」を追記 |
| 5 | L11402（§67.6.1 見出し） | 並べ替え提案 | 「### §67.7 残課題」(L11401) の直後に「### §67.6.1」が入って階層が逆転している。§67.6.1 を §67.7 の**前**（L11400 直後）へ移動し、L11417-11419 の番号付き残課題リストを §67.7 配下に戻す |
| 6 | L11413 | 訂正 | 「欠落362件」→「ギャップ362件（統一定義: decompiled.c に FUN_ 定義なし）」に語統一 |
| 7 | §68.0（L11433） | 注記追記 | 「§67 の MISS 480 件のうち7件」の記述に対し: 実際には7件中6件（0x1026/0x1037/0x105E/0x1075/0x1076/0x108B）は annotate_db 登録済みのため**§67 台帳には非掲載**（0x108C のみ台帳掲載）。§68 はテーブル全体を母数としている旨を注記 |
| 8 | L11425 の後（§67.8 の前） | 新節挿入 | 「### §67.9 台帳非掲載 52 opid（V15 監査 2026-10-24）」— §3.2/3.3 の表を転記 |

CRLF/バックアップ規約: 上記の通り `.bak13` 作成・バイトモード・行数維持（追記箇所のみ行数増は許容するが、置換 (#4) は既存表の行構造を活かし増減を最小化すること）。

---

## 6. 未解決点

1. **UNDET15 の除外理由**（§3.3）— 台帳生成スクリプトが Copilot セッション由来で残存しておらず、生成時の選択規則を直接確認できない。特に 0x0016 OnMagicEffectHit（SCDA 頻度 10,480・台帳なら rank2）が不掲載なのは生成規則の盲点を示唆。次セッションで Copilot セッション DB からの生成スクリプト復元を試みるのが最短。
2. **annotate_db の時系列変化** — 生成時点（2026-10-22、記載では367 funcs）と現時点（368 funcs）で差分があり、UNDET15 の一部が当時登録済みで後に削除された可能性。git 未追跡のため履歴照合不可。
3. **cc_prefix 閾値の公式化** — V10 の cc_prefix フラグが実態として「≥4 バイト」と判明（cc×2/×3 は no）。V10 join CSV 生成時の閾値コードを確認できていない（`%TEMP%\opencode\v10_join.py` に閾値実装の可能性あり、未読）。
4. Ghidra 本体（analyzeHeadless）不在のため、ギャップ362件の再ラベル化検証は未実施のまま（従来 TODO V23 の続行）。
