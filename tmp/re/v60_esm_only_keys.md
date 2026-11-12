# V60: ESM 専用 GMST キー5件の登録/無視経路 確定報告

対象: `Oblivion.exe` v1.2.0416 (Steam版) / 実ESM `Oblivion.esm` (277,504,985 B)
作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android` / 実行日: 2026-10-24
マッピング: .text `VA = 0x400C00 + file_off`、.rdata `VA=0xA28000 / off=0x627200+(VA-0xA28000)`（読取可能 VA 0xA28000–0xB01A00）、.data `VA=0xB02000 / off=0x700C00+(VA-0xB02000)`（file 読取は VA 0xB32C00 まで、以降 BSS）

---

## 1. セルフテスト（4件 / 4件 PASS）

| # | 検査 | 結果 | 判定 |
|---|------|------|------|
| ① | `bytes@0x504050` | `518b4c24188b5424` (8B) | PASS |
| ② | `bytes@0x4FB580` | `578b7908` (4B) | PASS |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | PASS |
| ④ | テーブル 基底`0xB0AF48` / stride`0x28` / 532件 | entry **#21**: opid `0x0015`（u32 @+0x08）impl **`0x504560`**（u32 @+0x18） | PASS |

④補足: エントリは `{name_ptr@+0x00, default_ptr@+0x04, opid@+0x08(u32), +0x0C, flags@+0x10, +0x14, impl@+0x18, impl2@+0x1C, ...}` の 10-dword 構成。

---

## 2. fRegionGenTex* 4件 — EXE 内文字列所在走査（全ファイル ASCII + UTF-16LE）

走査対象は EXE ファイル全域（7,898,624 B、.text/.rdata/.data/.tls/.rsrc/.bind 全部）。

| 検索パターン (ASCII, case-sensitive) | ヒット数 |
|---|---|
| `RegionGenTex` | **0** |
| `fRegionGen` | **0** |
| `RegionGen` | **0** |
| `fRegion` | **0** |
| `PlacedMatch` / `PlacedNotMatch` / `GenNotMatch` / `GenMatch` | **各 0** |
| `GenTex` | 1 — `bFaceGenTexturing:General`（.rdata VA 0xA54DD1、**無関係**。bFaceGen セッティングの文字列で "GenTex" は "GenTexturing" の一部） |

| 検索 (UTF-16LE) | ヒット数 |
|---|---|
| `fRegionGenTexPlacedNotMatch` / `...PlacedMatch` / `...GenNotMatch` / `...GenMatch`（4キー完全一致） | **各 0** |
| `RegionGen` (UTF-16LE) | **0** |

decompiled (`docs\Oblivion_decompiled.c`) 検索（lowercased）: `fregiongentex` **0件** / `regiongen` **0件** / `gentex` **0件** / `Gentex`(case-sensitive) **0件**。

**結論 (a): 4キーの文字列は EXE に完全不在。** 部分一致（`RegionGen` 系・`*Match` 系）・UTF-16・連結形式いずれもなし。動的キー構築の材料（部分トークン）すら存在しない。

### 2b. 読む側（ESM ロード後に4値を消費するコード）の探索 — 両方向

- **バイナリ**: キー文字列不在のため、名前経由 lookup（V55 経路 `0x44E326`→`0xB35468` slot9 `0x4A8040` EDID lookup、コンソール `GetGS`、`SetGS 0x5141D4` 系を含む）のいずれも当該キーには到達不能。
- **decompiled**: `fRegionGenTex` 参照 0件。読取系 `0x5E2560` 経由の引数経路も上記のとおりキーが存在しないため構築不能。
- **機能面**: Region テクスチャ生成（RegionGenTexPlaced/Gen マッチ率）は Construction Set の landscape ツール機能であり、ランタイム `Oblivion.exe` には実装自体が存在しない（文字列不在と整合）。

**影響判定: 読む側が存在しない → 全体として無害。** ESM ロード時、map `0xB35574` にキーが無いため lookup 未命中 → 無視（map 不変、V55 どおり）。ESM 側の値（1.0 / 1.0 / 0.95 / 1.1、§5 実測）はどこからも参照されない。

---

## 3. sSpecialization — 文字列所在・全参照走査・capstone デコード

### 3a. 文字列アドレス（.rdata 実測）

| 文字列 | VA | file off |
|---|---|---|
| `sClassSpecialization`（完全、NUL終端） | `0xA5D558` | `0x65C758` |
| ↳ その中で部分文字列 `sSpecialization` が開始 | `0xA5D55D` | `0x65C75D` |
| **`sSpecialization`（単独の完全文字列）** | **`0xA5E194`** | **`0x65D394`** |
| 既定値文字列（thunk の第2引数）`You will gain +10 in all specialization skills. Those skills will also increase faster.` | `0xA5E1A8` | `0x65D3A8` |

ファイル全体にバイト列 `sSpecialization` は **2件のみ**（上記の部分文字列と単独文字列）。

### 3b. そのアドレスへの全参照走査

| 走査 | 結果 |
|---|---|
| `.text` 内 imm32 `94 e1 a5 00`（=0xA5E194） | **1件**: file `0x5F0155` = VA **`0x9F0D55`** |
| ファイル全体（.rdata/.data/.bind 含む）imm32 `94 e1 a5 00` | **1件のみ**（同じ .text）。テーブル/配列経由の間接参照（データ内ポインタ）は **存在しない** |
| imm32 `5d d5 a5 00`（=0xA5D55D、部分文字列アドレス） | **0件** |
| decompiled: `0xa5e194` / `A5E194` / `a5e194` / `sSpecialization` | **0件**（thunk 領域 0x9F0xxx は decompiled 未収録。登録本体 `FUN_00419a70` は decompiled L16470–16501 で確認済み） |

### 3c. capstone デコード（VA 0x9F0D50 = 登録 thunk）

```
0x9F0D50: push 0xA5E1A8      ; arg2 = 既定文字列 "You will gain +10 in all specialization skills..."
0x9F0D55: push 0xA5E194      ; arg1 = キー "sSpecialization"
0x9F0D5A: mov  ecx, 0xB38640 ; this = 当該 Setting の静的オブジェクト
0x9F0D5F: call 0x419A70      ; FUN_00419a70 = 登録本体
0x9F0D64: push 0xA213A0      ; getter スタブ (mov ecx,0xB38640 ; jmp 0x4055E0)
0x9F0D69: call 0x981FB4      ; getter 側処理（→ 0x981F78 経由）
0x9F0D6E: pop  ecx
0x9F0D6F: ret
```

`FUN_00419a70` 本体（decompiled L16470–16501 + バイナリ確認）:

```c
undefined4 * __thiscall FUN_00419a70(undefined4 *param_1, int param_2, undefined4 param_3) {
  param_1[1] = param_2;              // entry.name  = key
  *param_1     = param_3;            // entry.value = default
  if (param_2 != 0) {
    if (FUN_0055e000(param_2, &param_2) != 0) {   // map 重複検査 (this = 0xB35574)
      FUN_004a7a60("Setting key '%s' already used in map.\n...");
      return param_1;                               // 重複なら insert しない
    }
    FUN_00412d30(param_1[1], param_1);              // map 0xB35574 へ insert
  }
  return param_1;
}
```

= **key→entry を登録マップ `0xB35574` へ insert する registrar**（float 用は `0x41BAE0`、int/string 用が `0x419A70`。既存 V50/V55 確定事実と一致）。

### 3d. thunk の到達性（間接経路）

- thunk `0x9F0D50` への参照はファイル全体で **1件のみ**: .rdata 関数ポインタ配列の **slot `0xA2A1BC`（index 1623）**。
- その配列 = **`0xA28860`..`0xA2D184`（4,682 エントリ、両端の dword が 0 で区切り）**。全エントリが初期化 thunk 領域（0x9D0000–0xA0FFFF）を指し、既知の float 初期化 `0x9ED780`（fDifficultyDamageMultiplier 既定 10.0f 設定、index 1280）も **同一配列に格納** = マスター静的初期化関数ポインタ配列。
- 隣接 thunk 例（`0x9F0D30`）: key=`sFavoredAttributes` / default=`You will gain +10 to each of your favored attributes.` — 同一形状（レベルアップヘルプ文字列 s タイプ群）。

### 3e. ground truth「sSpecialization 直接 push 未検出」との矛盾 — 解消

- 事実: 単独文字列 `0xA5E194` への `push` は `0x9F0D55` に **実在する**（§3b）。
- 前回未検出の最も蓋然的な原因（推定を明記）: `data.find(b"sSpecialization")` の **first-match が部分文字列 `0xA5D55D`（`sClassSpecialization` の内部）を返す**ため、そのアドレスで push を探して 0 件になったと考えられる。補助的要因として当該 push の imm32 は VA `0x9F0D56`（mod 4 = 2、非アライン）で、アライン dword 走査でも検出不能。
- 整合する既存記述: `docs\RE_EXE_REPORT.md` L9636 は既に「GMST 382 レコード中 378 名前一致 … **EXE 未登録は fRegionGenTex\* 4 件のみ**」と記載（V60 実測と一致）。L9632 の全 2,304 件 registrar 解析（0x41BAE0:992 / 0x419A70:1312）にも sSpecialization は含まれる。

**結論: sSpecialization は EXE登録済み。ESM 専用キーという前回分類は誤り。**

---

## 4. 登録経路の全体像（本件5件に共通）

```
起動時: マスター初期化配列 0xA28860..0xA2D184 (4682 slots)
          └─> thunk (例 0x9F0D50: push default ; push key ; mov ecx,<obj> ; call 0x419A70)
                └─> FUN_00419a70: entry{value,name} 構築 + 重複検査(0x55E000) + map 0xB35574 へ insert (0x412D30)

ESM ロード時 (V55):
  GMST ハンドラ 0x44E326 → 0xB35468 slot9 = 0x4A8040 (EDID lookup のみ / insert なし)
    ├─ 命中: 0x4A7D60 が DATA を entry.value へ転送（0x4510E0/0x450C20）→ EXE 既定値を上書き
    └─ 未命中: 無視（map 不変）
```

---

## 5. ESM 側実測（GMST GRUP 解析）

- GRUP @ file `0x2FC`: sig `GRUP` / **size `0x6B43`（合計サイズ、ヘッダ20B含む）** / label `GMST` → 終端 `0x6E3F` で綺麗に一致、**382 レコード**（f243 / s108 / i31）。
- ヘッダ形式実測: **RECORD = 20B（`sig@+0, dataSize@+4, flags@+8, fid@+12, ts@+16, vci@+18`）/ GRUP = 20B（`sig@+0, size@+4, label@+8, type@+12`）/ サブ = 6B**。ground truth「レコード20B/GRUP20B/サブ6B」に整合（dataSize が +4 にある点が決定的。標準 TES4 文献の 24B ヘッダとは異なる）。

| キー | ESM レコード位置 | ESM DATA 値 |
|---|---|---|
| fRegionGenTexPlacedNotMatch | `0x6988` | `1.0` (raw `0000803f`) |
| fRegionGenTexPlacedMatch | `0x69C8` | `1.0` (raw `0000803f`) |
| fRegionGenTexGenNotMatch | `0x6A05` | `0.95` (raw `3333733f`) |
| fRegionGenTexGenMatch | `0x6A42` | `1.1` (raw `cdcc8c3f`) |
| sSpecialization | `0x18E6` | `"Choose your Specialization. Specialized skills get +10 and increase faster."` |

---

## 6. 5件の判定表

| キー | 判定 | 根拠（要約） |
|---|---|---|
| fRegionGenTexPlacedNotMatch | **(α) 登録されず無視・無害** | 文字列 EXE 全不在（ASCII 部分一致・UTF-16・連結とも 0件、decompiled 0件）→ 登録不能・読取不能。ESM 値 1.0 は不参照 |
| fRegionGenTexPlacedMatch | **(α) 登録されず無視・無害** | 同上（ESM 値 1.0。読む側不存在を両方向で確認） |
| fRegionGenTexGenNotMatch | **(α) 登録されず無視・無害** | 同上（ESM 値 0.95） |
| fRegionGenTexGenMatch | **(α) 登録されず無視・無害** | 同上（ESM 値 1.1。Region テクスチャ生成は CS 専用機能でランタイムに実装なし） |
| sSpecialization | **(β) 間接登録あり** | 単独文字列 `0xA5E194` に .text 唯一の `push`（0x9F0D55）→ 登録 thunk `0x9F0D50` → `FUN_00419a70` が map `0xB35574` へ insert。thunk は初期化配列 index 1623 で到達可能。ESM lookup は**ヒット**し ESM 値が既定値を上書き |

**影響結論**: 無害（α）は 4件、正常上書き動作（β）は 1件。(γ)/(δ) 該当なし。
**ground truth 修正**: 「ESM 専用5件」は **4件**（fRegionGenTex\* のみ）。sSpecialization は EXE 登録済み（前回 'push 未検出' は `sClassSpecialization` 内部分文字列 first-match による走査誤り）。既存 `RE_EXE_REPORT.md` L9636 の「EXE 未登録は fRegionGenTex\* 4件のみ」と一致。

---

## 7. 移植示唆

1. 移植側が ESM の GMST 382件を処理する際、**上書きしてよい・できるのは 378件**（既に EXE/移植側に登録済みのキー）。`sSpecialization` は ESM 値 `"Choose your Specialization..."` を既定値 `"You will gain +10..."` に上書きする対象として正しく扱うこと。
2. **fRegionGenTex\* 4件は「ESM ロード時にキー未登録→無視」を再現すればよい**（値の格納・参照実装は不要）。CS 専用キーでありランタイム機能が無いため、取りこぼしても挙動差分は発生しない。
3. EXE レジストリ件数の扱い（V59 のキー件数差分）では、sSpecialization を登録済みに数えること（旧「2,175件」の s 系カウントはこの1件を漏らしている可能性あり）。

---

## 8. 未解決点

- **初期化配列の走査コードを静的に特定できず**: `0xA28860`/`0xA2D184` ほか範囲内任意アドレスへの imm32 参照がファイル全体で0件（.text 内の範囲値ヒット7件はすべて call rel32 / sete / cmp disp32 の偶然一致、instruction境界で検証済み）。TLS コールバック配列は空（`cb[0]=0`）。PE エントリポイントは `.bind` 内 VA `0xBAF2ED` のデコードスタブ（`rep movsd` + XOR デコード）で保護/難読化入り口のため、CRT 起動本体（`_initterm` 相当）は展開後に走る可能性。**判定には影響しない**（既存 V系レポートで同一メカニズムを 2,304件全数で検証済み）。
- `0x981FB4`（各 thunk の第2呼び出し、`0x981F78`→`0x981E9C` 経由）の正確な役割（getter 登録 or 検証）は未確定。sSpecialization の登録判定には非必須。
- 前回「push 未検出」の原因は first-match バグが最も蓋然的だが、前回走査コードの実体は参照できないため**推定**（アライン制約の副次的寄与の可能性を含む）。
