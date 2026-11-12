# v285_v72_b333c4_residuals.md — v60v76-bundle 依頼3 [P3]: v72 グローバル 0xB333C4 マップ残件5点（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼3 [優先度 P3]（出典 `tmp/re/v72_global_b333c4_map.md` §7 残点番号 #2〜#6）
- 対象: Oblivion.exe v1.2.0416 (Steam) / `DAT_00b333c4` = bss@0xB333C4 の int\* ポインタ1本・実体ヒープ 0x804 B
- ハーネス: `probe_harness.py selftest` → **4/4 PASS**（本セッション実行）
- **判定: 5点中 3点 [確定] / 2点 [部分確定]**

---

## #2（v72 §7-2）: decompiled 欠落195 関数の理由分類 → **[確定] 195/195 が「this 引数（DAT ポインタ）のエリジョン」**

### 手法と結果

`v72_bin_refs.txt`（binary 参照 791 関数）− `v72_dec_refs.txt`（decompiled 参照 596 関数）= **binary-only 195 関数**。各関数の decompiled 本文（`fun_src()`、**サイズ min 119 B / 中央 1,261 B / max 19,310 B・ゼロ本文 0 件**）を判定:

| 分類 | 件数 |
|---|---|
| A: decompiled 定義あり ＋ 本文に `DAT_00b333c4` 参照も有（部分的エリジョン） | **0** |
| B: **decompiled 定義あり ＋ 本文に `DAT_00b333c4` 一切なし** | **195（100%）** |
| C: decompiled 定義なし（未出力/未解析） | **0** |

- **サンプル実証（v72 の例）**: `FUN_0040C830` — binary は `0040C92B mov ecx,[0xb333c4] … call 0x660110`、decompiled L67 は `iVar13 = FUN_00660110(1);`（**DAT 引数が出力に残らない**）。
- → **「最適化」「データ誤分割」「未解析」のいずれでもない**。Ghidra デコンパイラが thiscall の this（DAT ポインタ）を省略した結果であり、参照数はバイナリ側（791）が正。

---

## #3（v72 §7-3）: +0x200〜+0x56C 帯の中身 → **[部分確定] float 配列3本（各72要素）＋特殊 float 3本＋getter/setter。配列ごとの意味名は [推定]**

### 3-1. 構造（ctor `FUN_0066A740` の初期化で完全確定）

ctor の dword ゼロ埋めは **3 ループ＋3 個別ストア**で帯を全覆蓋:

| 領域 | 初期化 | 要素 |
|---|---|---|
| **+0x204..+0x323** | ループ1（idx 0x81–0xC8） | **float 72 本**（配列A） |
| **+0x324..+0x443** | ループ2（idx 0xC9–0x110） | **float 72 本**（配列B） |
| **+0x444 / +0x448 / +0x44C** | 個別 `param_1[0x111/0x112/0x113] = 0`（L75–78） | **特殊 float 3 本** |
| **+0x450..+0x56F** | ループ3（idx 0x114–0x15B） | **float 72 本**（配列C） |
| +0x200 | `param_1[0x80] = 0`（v72 既知の named 1件） | dword 1 本 |

→ **72 × 3 ＋ 3 = 219 float（876 B）**。帯は「未同定のゴミ領域」ではなく**3×72 float の配列群**。

### 3-2. アクセス経路（getter / setter を特定）

```c
// getter FUN_0065d270(this, group, idx)  -> float
group==0: *(float*)(this + 0x204 + idx*4)          // 配列A
group==1: *(float*)(this + 0x324 + idx*4)          // 配列B
group==2: idx==8 -> +0x444 / idx==9 -> +0x448 / idx==10 -> +0x44C
          else  -> *(float*)(this + 0x450 + idx*4) // 配列C（idx8/9/10 のみ特殊位置へリマップ）

// setter FUN_0065d310(this, group, idx, a, b) -> 同位置へ FUN_0065bc70(old, a, b) の結果を書戻し
```

- 機械語実証: `0065D2F8 fld dword [ecx + edx*4 + 0x204]` / `0065D2E3 fld dword [ecx + eax*4 + 0x324]` / `0065D457 fld dword [esi + edi*4 + 0x204]`・`0065D466 fstp [esi+edi*4+0x204]`。
- **`FUN_0065D270` の caller 20 件は全件 `ecx = [0xB333C4]`（グローバル渡し）**（`FUN_005A5B50`×6 / `FUN_005A6DE0`×6 / `FUN_005DA1A0`×3 / `FUN_005F2530`/`005F25F0`/`005F2720` 等）→ **この帯はグローバルのフィールドで確定**。
- v72 の「`param_1` 引数経由アクセス群（統計対象外）」仮説を**実証**（グローバルを this として受け渡す関数内で `[this+idx*4+disp]` が走るため、v72 の named 統計には現れない）。
- 補足（混同注意）: 帯の数値を含む `*obj + disp` 形（+0x208/+0x224/+0x284/+0x39C/+0x444…）の多くは **vtable スロット**であってフィールドではない（グローバルロード→`mov r2,[r]`→`[r2+disp]` の2段）。本判定は base が直接グローバルのものだけを数えた。

### 3-3. 意味づけ

- **[確定]**: 要素型 = float、要素数 = 72×3、`FUN_0065D310` setter が `FUN_0065BC70`（計算/クランプ）経由で書戻し、ctor が完全ゼロ初期化。
- **[推定]**: **72 =アクターバリュー（AV）の総数と一致**。傍証: group2 系の入口 `FUN_0065E030` は `FUN_005F1910(param_1)`（= GetBaseActorValue 系 `FUN_004F45D0` が呼ぶ同一ヘルパ）を呼び、`FUN_0065FCA0` 等は AV 索引で算術する。HUD 帯（0x5A5/0x5A6）・chargen 帯（0x5DA1）・magic 帯（0x5F2）が消費者。
- **未確定（残点）**: 配列A/B/C が「base / 現在値 / 補正」等のどれに当たるかの正式名、idx8/9/10 を特殊位置へリマップした理由。

---

## #4（v72 §7-4）: `FUN_006600D0` / `FUN_00660110` の this は常に DAT か → **[確定] 常に DAT（両関数とも 100%）**

E8 caller 全数を、call 直前の `mov ecx, …` で分岐追跡:

| 関数 | caller | `ecx = [0xB333C4]` | 例外 |
|---|---|---|---|
| `FUN_00660110` | **79** | **79（100%）** | 0 |
| `FUN_006600D0` | **38** | **38（100%）** | 0（下記2件を解消） |

**当初「other」に見えた2件の解消（分岐解析）**:

1. `0x6A0E0A`（`FUN_006A0D90`）: 線形走査では直前の `mov ecx,eax`（0x6A0DDF）が見えるが、これは `je 0x6A0DEB` の**非対称分岐**（該当パスは `jmp 0x6A0E0F` で call を素通過）。call に至るパスでは `0x6A0DD3 mov ecx,[0xb333c4]` が生きている → **DAT**。
2. `0x67052B`（`FUN_006701B0` 内 `mov ecx,esi`）: `FUN_006701B0` は先頭で `mov esi,ecx`（this）を受け、**その caller 2件（`0x50B690` in `FUN_0050B3E0` / `0x5822A2` in `FUN_005821F0`）はいずれも直前に `mov ecx,[0xb333c4]`** → this = DAT → **DAT**。

→ **v72 §7-4「全呼出し未確認」は解消。両関数の this は例外なくグローバル `0xB333C4`（＝シングルトン固定）**。

---

## #5（v72 §7-5）: 近傍 bss クラスタのラベル未同定分 → **[確定] 230 本中 177 本ラベルあり・53 本未同定（全数リスト）**

`v72_bin_refs.txt` の (b2) 近傍走査（`B333C4+offset`、正方向）から**正方向 230 アドレス**（`0xB333C8`..`0xB33B80`、R=3,848 / W=243 / IMM=283 / RW=17 サイト）を抽出し、`docs\Oblivion_decompiled.c` の `DAT_00b33xxx` ラベルと全数照合:

| 判定 | 件数 |
|---|---|
| **decompiled に `DAT_00b33xxx` ラベルあり（同定済）** | **177** |
| **ラベルなし（未同定）** | **53** |

- **同定済177本の例**: `0xB333C8/CC/D0/D4/D8/DC…0xB335B8`（連続帯）、`0xB3368C..0xB3370C`、`0xB338B8..0xB33934`、`0xB33A00..0xB33B08` 等（完全リストは下記）。
- **未同定53本の内訳**（連続帯で特徴的）:
  - `0xB335BC`〜`0xB33684`（stride 8、**22 本**）
  - `0xB33880`〜`0xB338B0`（stride 8、**7 本**）
  - `0xB33A6C`〜`0xB33A8C`（**8 本**）
  - 散発: `0xB333F5` `0xB3341C` `0xB33427` `0xB33430` `0xB33438` `0xB33454` `0xB33800` `0xB33980` `0xB33A35` `0xB33AC4` `0xB33B80`（11 本）＋上記帯の隙間（5 本: `0xB335C4` 等は帯に含むため上に計上済、実散布は stride8 帯の端数 5 本 = `0xB333F5`/`0xB3341C`/`0xB33427`/`0xB33430`/`0xB33438`/`0xB33454`/`0xB33800`/`0xB33980`/`0xB33A35`/`0xB33AC4`/`0xB33B80`）
- **未同定分の参照元（owner）**: `FUN_00445A10`（38 サイト）、`FUN_009D9FA0`（34 サイト）で大半が集中 → **decompiled 側に当該参照が出力されていない（エリジョン/未出力）＝ラベルが付かない**。他は `FUN_0040FDD0`(6) `FUN_0040D4D0`(4) `FUN_0044EE00`(4) `FUN_00424E60`(4) 等。
- 判定: 未同定53本は**独立 bss グローバル（ブロックと無関係）だが、decompiled 未出力のためラベルが付いていない**群。移植時は binary 側のアドレス（上記リスト）を直接使う必要がある。

### 未同定53本（完全リスト）

```
0xB333F5 0xB3341C 0xB33427 0xB33430 0xB33438 0xB33454
0xB335BC 0xB335C4 0xB335CC 0xB335D4 0xB335DC 0xB335E4 0xB335EC 0xB335F4 0xB335FC
0xB33604 0xB3360C 0xB33614 0xB3361C 0xB33624 0xB3362C 0xB33634 0xB3363C 0xB33644
0xB3364C 0xB33654 0xB3365C 0xB33664 0xB3366C 0xB33674 0xB3367C 0xB33684
0xB33800
0xB33880 0xB33888 0xB33890 0xB33898 0xB338A0 0xB338A8 0xB338B0
0xB33980
0xB33A35 0xB33A6C 0xB33A70 0xB33A74 0xB33A78 0xB33A7C 0xB33A80 0xB33A84 0xB33A88 0xB33A8C
0xB33AC4
0xB33B80
```

---

## #6（v72 §7-6）: 終端 byte +0x7A4〜+0x803 → **[部分確定] float 配列21要素（型別係数表）＋ byte フラグ2本＋ float 2本**

### 6-1. ctor 初期化（機械語で全数確認）

| VA | 指令 | 範囲 |
|---|---|---|
| `0x66AE6E..0x66AEE7` | `mov dword [esi+0x7a4 .. +0x7f4], ebx(=0)` × **21** | float 21 本をゼロ |
| `0x66B041` | `mov byte [esi+0x7f8], bl` | byte フラグ |
| `0x66B04D` | `mov byte [esi+0x7f9], bl` | byte フラグ |
| `0x66B059` | `fst dword [esi+0x7fc]` | float（0.0f） |
| `0x66B061` | `fstp dword [esi+0x800]` | float（0.0f） |

→ **`0x7FA`/`0x7FB` の2バイトは ctor にストアが無い**（fresh heap `new(0x804)` のため未初期化の可能性）＝**残点**。

### 6-2. +0x7A4..+0x7F4 = 型別係数の float 配列 21 要素 [確定]

- **索引**: `FUN_00565C50(2, formType)`。適用範囲は `formType - 0x0C < 0x15` ＝ **formType 0x0C..0x20 の21種**（`0x20-0x0C+1 = 21` ＝要素数21 と完全一致）。
- **読者**（`fld [reg + idx*4 + 0x7a4]`）:
  - `FUN_0065FAD0`（this=GLOBAL あり）: `return float[this + 0x7A4 + FUN_00565C50(2, formType)*4]` — 型別の倍率取得
  - `FUN_0065FCA0`（this=GLOBAL **2/2**）: `float[this+0x130+i*4] / float[this+0x7A4+i*4]` — **＋0x130 の配列値をこの係数で割る正規化**
  - `FUN_00666BB0`（SUBDYN の1件を含む大関数）、`FUN_0066C200`（this=GLOBAL 2/2、`FUN_00565BE0(2,i)` ループで全要素を処理）
- **書手**: `FUN_00663C50`（`fstp [edi + eax*4 + 0x7a4]` @0x663D1D、caller 5 件）
- → **formType 0x0C..0x20（アイテム/オブジェクト型21種）ごとの float 係数表**。`+0x130` の配列との比算に使う。

### 6-3. +0x7F8 = byte フラグ（write-only）[確定・ただし decompiled 未出力]

- **唯一の実行時書手**: `005B5EE1 mov eax,[0xb333c4]` → `005B5EF1 mov byte [eax+0x7f8], 1`（eax は intervening で不変＝真のフィールド書込）。直前に `mov byte [0xb3a6d3],1`（別 bss）と同時。
- owner（台帳）= `FUN_005B5B70`（caller 1 件 = `FUN_005AEDF0` @0x5A5EF23… = `0x5AEF0` 帯）。**decompiled の `FUN_005b5b70` 本文には当該書込が現れない**（= #2 のエリジョン型の残例）。
- 読手は disp32 スキャンで検出0件 → **write-only フラグ**（静的 disp 限定。間接経路は未排除）。
- `+0x7F9` も ctor のみ・実行時書手検出0件。
- `+0x7FC` / `+0x800` は ctor が float(0.0f) で初期化 → **float フィールド2本**（用途未特定）。

### 6-4. 判定

**構造 [確定]: float 21要素（formType 0x0C..0x20 係数）＋ byte フラグ（+0x7F8/+0x7F9）＋ float 2本（+0x7FC/+0x800）。意味づけ [部分確定]: 係数表の用途は「+0x130 配列との正規化」まで。0x7FA/0x7FB 未初期化は残点。**

---

## 残点（本依頼範囲外・記録）

1. 配列A/B/C（72×3）の正式な役割名（base/current/modifier 等の確定）と idx8/9/10 リマップの理由。
2. `+0x7FC`/`+0x800` float の用途、`0x7FA`/`0x7FB` が未初期化かどこかで覆われるか。
3. `+0x7F8` フラグの読手（間接経路）と `FUN_005B5B70` が decompiled で欠落した理由の個別特定。
4. 未同定53本のうち `FUN_00445A10`/`FUN_009D9FA0` が触る帯（`0xB335BC`〜 stride8 等）の要素型・用途。

## 証跡（すべて `%TEMP%\opencode\`）

| スクリプト / 入力 | 内容 |
|---|---|
| `v285_p3.py` + `_out.txt` | #2 分類195件 / #4 ecx分類 / #3・#6 disp帯走査 / #5 初出（正規表現バグ検出） |
| `v285_p3b.py` + `_out.txt` | ctor 300–400行ダンプ / (b2) 正方向230本 / 帯ヒットの clobber 窓 / 0x6A0E0A 分岐窓 / FUN_005B5B70 窓 |
| `v285_p3c.py` + `_out.txt` | +0x7F8 owner / FUN_006701B0 の esi 出所 / ラベル存在確認 / lea-add 帯走査（0件）/ scaled-index 75件抽出 |
| `v285_p3d.py` + `_out.txt` | ラベル走査修正（177/53）/ scaled-index の base 出所 / FUN_006701B0 caller ecx（2/2 GLOBAL） |
| `v285_p3e.py` + `_out.txt` | SUBDYN 1件（`DAT_00b333c4[local_d8+0x62]` in `FUN_00666BB0`）/ 候補関数の caller / tight scan |
| `v285_p3f.py` + `_out.txt` | **getter/setter 本体（FUN_0065D270/D310）と帯アクセス disasm** |
| `v285_p3g.py` + `_out.txt` | ctor の 0x7A4..0x803 全ストア / .text disp32 188件（esp系除く実フィールド）/ FUN_005B5B70 caller |
| `v285_p3h.py` + `_out.txt` | 帯 users 5関数の caller ecx 分類＋decompiled 先頭 |
| `tmp\re\v72_global_b333c4_map.md` | 出典（読取のみ） |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: binary-only 195 = 791−596 ✓ / 分類合計195 ✓ / this 分類 79+38 ✓ / 正方向230本 = v72 の「230」一致 ✓ / float 72×3+3 = 帯 0x204..0x56F と一致 ✓ / formType 21種 = 21要素 一致 ✓。
