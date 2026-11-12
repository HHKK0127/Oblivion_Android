# v205_havok_ini_naming.md — V192: havok/INI 命名束（v195/v196 残留5課題の追跡）

対象タスク: ai-todo #5 / TODO_re49 V192
入典: `v195_pick_pathing_residuals.md`（§3.3 O / §3.4 W+0x78 / §4.2 0xB3BCF0・§5 残留 1/3/4）、`v196_avoidance_ini_residuals.md`（§6 残留1 = 0xB1581C）
probe: `v192_probeT.py` … `v192_probeY.py`（`%TEMP%\opencode`、selftest 4/4 PASS 維持）

## §1 手法 / 入力

- EXE(`Oblivion.exe`, 7,898,624B) を PE セクション表で off↔VA 正確換算し、imm32 全ファイル走査＋decompiled(.c 1,066,496行) の FUN ヘッダ bisect enclosing で参照先を同定。
- **VA 換算の罠を回避**: `IMAGE_BASE 0x400000 + RVA` を使用（`.text` raw=0x400 / RVA=0x1000 → `va = 0x401000 + (off-0x400)`）。probe V/W で1度ずれたため W で全走査を正しい VA 換算で再実行。
- INI 名は `{value(4B), name_ptr(4B)}` の8バイト連続テーブル（`.data`）＋`.rdata` の `Key:Section` 形式文字列（631件）として列挙し、name_ptr の参照を突合。

## §2 証拠・分析

### §2-1 課題1: O（vtable `0xA9A710`）の正式クラス名 — [確度高: **hkp3AxisSweep**]

- vtable 直前の文字列群: `'3AxisSweep'`(0xA9A700)、`'Lthk3AxisSweep'`(0xA9A6D4)、`'LtquerySingleAabb'`(0xA9A694)。
- **決定的証拠**: vtable のメソッド実体がこれらをコード内で参照する。
  - `FUN_008e2f20`(L867187) → `'3AxisSweep'`
  - `FUN_008e24a0`(vtable slot+0x38、castRay 相当) → `'Lthk3AxisSweep'` / `FUN_008e2f20`
  - `FUN_008e1e20`(slot+0x28) → `'LtquerySingleAabb'` / `FUN_008e5950`
  - プロファイル名文字列 `'querySingleAabb'`(0xA9A696)
- ctor `FUN_008E4730`(L868232): `[esi]=0xA9A710`、refcount=1、hkArray×4(+0x40/+0x4C/+0x58/+0x64)、23反復収束ループ後に
  - `param_1[0xc..0xf] = 65532/range`（`0x477ffc00`=65532）、`param_1[4..7] = -min`（xorps `0xA965C0` 符号反転）、`param_1[8..0xb] = -min + range*1.525972e-05`
  - = **世界 AABB → 16bit 量子化変換**（Havok hkp3AxisSweep の型付きブロードフェーズと一致）。
- → **O = hkp3AxisSweep（Havok broadphase）** [確度高]。v195 §3 の「hkpBroadPhase 系（M-BP-I 最有力）」を置換。b13 #29「不明」への回答。

### §2-2 課題2: `0xB3BCF0` の INI 名 — [結論反転: **INI 名は存在しない（登録自体が無い）**]

- `.data` の `{value, name_ptr}` テーブル全走査で **`0xB3BCF0` を指す参照はゼロ**（`.text` のみ10件: fld 読手6件 `0x673B02-0x673B63`、save/load push 2件 `0x677071`/`0x677122`、`0x678541`/`0x6787EB`）。
- 対照群の実測: `'fMaxTime:HAVOK'`(va 0xA46B94) → `.data` 1件 `0xB097C4`、`'bDisableAvoidance:Pathfinding'`(0xA74B2C) → `0xB1574C`、`'fGlobalTimeMultiplier:General'`(0xA3D150) → `0xB06708` … いずれも name_ptr 参照がテーブルに1件存在する。**0xB3BCF0 のみこの形式が無い**。
- v195 §4.2 の「`push 4; push 0xB3BCF0; call 0x45b9a0` = 設定管理(0xB33B00)への登録」は**訂正**: `FUN_0045B9A0`(L72410) の実体は `_memcpy(*(void**)(param_1+0x14), param_2, param_3)` ＝ **save-stream 書き込み**（`TESSaveLoadGame::SaveGameData()` チェック付き）。つまり `0x677060/0x6770F0` 経由の push は「セーブデータへ float 4B を保存」であり INI 登録ではない。
- setter `FUN_00673B10`(L435536): `_DAT_00b3bcf0 = param_1; if (100000.0 < p) p=0.0;` + `__isnan`/`FUN_00983ffd`(Inf 系) で **NaN/Inf/巨大値ガード**。callers 5件: `FUN_0065f770`(fSecs 系計算後)/`FUN_0066f420`×2/`FUN_006770f0`(save clear: `push 0`)/`FUN_00678510`(加算)。
- → `[0xB3BCF0]` は **runtime 計算の時間系 float（fSecsPerFrame/fGlobalTime 系）+ セーブ対象**であり、INI キー名は持ちません [確定(構造)]。v195 §5 残留2「INI 正式名を取得」は**前提が誤り**だった。

### §2-3 課題3: ctor の「23エントリ境界テーブル」— [訂正: **23 = 収束反復回数**]

- `FUN_008E4730` ループ実体を decompiled L868311-868363 で全読。
  - `mov ecx,0x17` のループは `local_40/local_44`(10.0/11.0) の**二分法的浮動小数点収束計算**（`>>7 &0xffff|1 < 0xc` の bit-test 終了条件）で、収束値を `param_1[0x1f]=(local_44+local_40)*0.5 - 11.0` に格納する1回のみ。
  - 直後に世界 AABB（min/max ベクトル2本）から `param_1[4..0xb]`（-min / -min+range*scale）＋`param_1[0xc..0xf]`（65532/range）を生成。
- → **v195 §3.3「`mov ecx,0x17`（23 エントリ）ループで … AABB 境界テーブル生成」は誤り**。23 は収束計算の反復回数、テーブルは **4+4 float の min/max + 16bit 量子化係数**（固定表ではなく動的生成）。v195 §7「23 エントリの境界テーブルは固定表として移植」も**廃止**（AABB はワールド固有なので実行時に生成する）。

### §2-4 課題4: `W+0x78` collectionFilter 系 — [確度高: **Havok broadphase/collection filter 系**]

- writer `0x89A835` の enclosing = **`FUN_0089a230`**(L805929)（v195 §3.2 の W 構築本体と同一）。`*param_1 = &PTR_FUN_00a96d6c`（base-phase vtable）の後、`0x89A831` で `0xA96B78` をロードし `[param_1+0x78]` へ（refcount inc 付き）＝ **W+0x78 に vtable `0xA96B78` のサブオブジェクトを保持**。
- vtable `0xA96B78` の `.text` refs は4件のみ: `0x899356`(ctor `FUN_00899330`)/`0x89A831`/`0x8DA23C`/`0xA12BD3`。ctor `FUN_00899330` は `*param_1=&PTR_FUN_00a96b78` かつ `param_1[2..5]` にサブクラス vtable `0xA96B64/68/70` を書く 4本書き換え型（v195 §3.4 の記載と一致）。
- 近傍文字列: `'collectionFilter'`(0xA96B76)、`'StCallbacks'`(0xA96B80)、`'StBroadphase'`(0xA96B8C)、`'Broadphase'`(0xA96BBC)。
- slot 実測: `0x899380/90`→ctor、`0x8993A0/D0/E0`→`FUN_008993a0`(`FUN_008da1d0`+`DAT_00ba7d98` vt+0x14 = DTC/destructor 系)、`0x927A70`→`FUN_009279f0`、`0x68F970`→**`FUN_0068f970` = 空関数（`return;`、no-op slot）**。
- → **W+0x78 = hkpCollectionFilter 系フィルタ** [確度高]。v195 §3.4 の推定を維持しつつ、enclosing（`FUN_0089a230` 内）と vtable refs 4件・no-op slot を追加。

### §2-5 課題5: `0xB1581C` の正式名 — [確定: **`bDebugSmoothing:Pathfinding` の value スロット**]

- dword/文字列ダンプ（0xB156F0-0xB15860）: `0xB15818`=`'bSnapToAngle:Pathfinding'` name、`0xB1581C`=0(値)、`0xB15820`=`'bDebugSmoothing:Pathfinding'` name、`0xB15834`=0/`0xB15838`=`'bDrawPathsDefault'`。
- name slot 参照は INI 走査 `FUN_00a25e30` のみ（`0xB15818`→`0xA25E80`、`0xB15820`→`0xA25EB0`）＝テーブル走査の比較キー。
- value 読手が **`bSnapToAngle`(0xB15814) と完全対称の5件**:
  - `0xB15814`: `FUN_00686150`/`FUN_00688120`/`FUN_009a9a00`/`FUN_009fe810`/`FUN_00a25e30`
  - `0xB1581C`: `FUN_00685fc0`/`FUN_00688dc0`/`FUN_009a9a00`/`FUN_009fe870`/`FUN_00a25e30`
- `FUN_00685fc0`(PathingSystem 構築) が `0x686012` で読み `[0xB3C08A]` へ反映（v196 §3 の実測）と整合。
- → **`0xB1581C` = `bDebugSmoothing:Pathfinding`** [確定]。v196 §6 残留1 を解消。`0xB15814` = `bSnapToAngle:Pathfinding` も同時確定。

## §3 結論（3行）

1. **O = hkp3AxisSweep**（vtable メソッドが `'3AxisSweep'`/`'Lthk3AxisSweep'`/`'LtquerySingleAabb'` を参照、ctor は世界 AABB の 16bit 量子化）。
2. **`0xB3BCF0` に INI 名は無い**（{value,name} テーブル未登録、`0x45B9A0` は save-stream memcpy で登録経路自体が存在しない）＝時間系 runtime float + セーブ対象。
3. **`0xB1581C` = `bDebugSmoothing:Pathfinding`**、ctor の「23エントリ」は**収束反復回数**（AABB 量子化テーブル生成）で v195 §3.3/§7 は訂正対象。

## §4 残留不明点

1. `0xB3BCF0` の**値の意味の正式名**（fSecsPerFrame / fGlobalTime / fAltDelta のどれか）は、INI 名が無いため INI からは辿れず [残]。setter callers の1つ `FUN_0065f770`（fSecs 系計算後）の被加数を読めば特定可能。
2. 課題1 のメソッド文字列 `'Lthk3AxisSweep'` の `Lthk` 接頭辞（プロファイラ/RTTI 断片？）の意味は [残]（クラス同定には影響なし）。
3. `0x8DA23C`/`0xA12BD3`（vtable `0xA96B78` の残り2参照）の処理内容は未読 [残]。

## §5 自己検証

- probe T/U/V/W/X/Y を `probe_harness.py` 経由で実行、selftest 4/4 PASS。
- VA 換算エラーを probe V で1回発見 → probe W で `IMAGE_BASE+RVA` に修正し全走査をやり直し（キー文字列参照が0件→テーブル1件に是正）。
- 新規ファイル: `tmp/re/v205_havok_ini_naming.md` のみ。台帳/レポート訂正の追記は**未実施（承認待ち）**。

## §6 Android 移植への示唆（実装しない）

- **broadphase(O) は hkp3AxisSweep 相当**として、量子化係数（65532/range、-min、scale）を**実行時に世界 AABB から生成**する。v195 §7 の「固定表として移植」は誤りなので**採用しない**。
- INI の HAVOK/Pathfinding キー群は `{value, name_ptr}` テーブル（`Key:Section` 形式）を走査する方式で移植可能だが、`0xB3BCF0`（時間系）は **INI キーを持たない runtime 値**なので、INI ではなくフレーム計算＋セーブデータとして扱う。
- `bDebugSmoothing` / `bSnapToAngle`（Pathfinding）は Android 側のデバッグ描画フラグとしてそのまま移植可能（両者とも既定 0）。
