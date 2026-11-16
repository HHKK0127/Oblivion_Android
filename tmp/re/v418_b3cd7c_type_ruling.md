# v418 — `0xB3CD7C` singleton の型（部分確定）と `FUN_0047C930`（NiAVObject bulk 通知）の同定（V-418-1／派生残点第10弾）

- 採番: **v418**（派生残点群・probe prefix **v418_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v417 の残点 **V-418-1【中】（`0xB3CD7C` singleton 型）** を追跡（高精度な [部分確定] まで到達）＋**副次大収穫 `FUN_0047C930` の identity 確定**。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_0070F910` の identity（`[0xB3CD7C]` 値の受け手）** | **[確定]** | **`"--- %s ---"` ヘッダ文字列生成 fn**: `strlen(param_1)` → malloc → **`FUN_006C5D40(buf, size, "--- %s ---", str)`** → 返却（L553023-553038）。**arg は `char*`** → **`0x6D0450` が `FUN_0070F910([0xB3CD7C])` に渡す値は `char*`＝文字列 [確定]**（＝v401 ruling の「dump ヘッダ文字列出力」と一致） |
| **`0xB3CD7C` の型（高精度）** | **[部分確定]（[推定高] の根拠強化）** | **(i) `[0xB3CD7C]`（値）＝`char*`**（FUN_0070F910 へ直接）[確定]／**(ii) map key は `&0xB3CD7C`（アドレスそのもの）**（`FUN_00700010(map, &DAT_00b3cd7c)`・pointer 等値比較）[確定]／(iii) usage＝`"--- %s ---"` 出力・float set 系・flag set（`word[+8] \|= 0x40`）・**`FUN_006FFE90`（v371 V-371-2 の keyframe 生成候補）への接続**（FUN_00473200 L91987-90）・`FUN_006C8220`（NiTransformController 登録確認）・StoreTargets epilogue → **「singleton 文字列（global `char*` 変数）のアドレスが NiFixedString 的 key として map lookup に使われる」[推定高]**（中身の文字列そのものは実行時＝.data 不定で未読） |
| **副次大収穫: `FUN_0047C930(this, param_2, param_3)` の identity** | **[確定(命令＋decompiled)]** | **NiAVObject 系の bulk 通知 fn**: `param_3 != 0` 時に **`[this+0x9C]` リスト**（`next=[0]`・`elem=[2]`・**`elem[3] != 0` gate**）の各 elem へ **`elem->slot20(param_2)`** ＋ 常時 **`[this+0xC]` map**（`node = node[0xd]` 走査）の全 node へ **`node->slot21(param_2)`**（L99379-99399）。→ **v405 で StoreTargets が見ていた「resolved node の `[+0x9C]` リスト」と「map」の2構造を保持するオブジェクト＝NiAVObject 系 [確定]** |
| **`0xB3CD7C` 使用の decompiled owner 群** | **[確定]** | `FUN_00473120`（等値ループ）／`FUN_00473200`（lookup→`FUN_004715C0(0)`→**`FUN_006FFE90`（keyframe 生成候補・v371）**）／`FUN_0047C990`/`FUN_0047CA30`/`FUN_0047CC80`（等値ループ×3＝インライン key 探索）／**`FUN_0047F750`（lookup→`FUN_0047C710`→`cmp [0xA7A160]`）**／`FUN_004964F0`（等値）／**`FUN_004E3490`（lookup→`word[+8] \|= 0x40` flag set）**／`FUN_006C6910`（StoreTargets 連携）／`FUN_006C8220`（NiTransformController 登録確認・NiPointer refcount） |

## §2 証拠（v418_1）

### §2.1 `FUN_0070F910`（L553023-553038）
```c
FUN_0070f910(char *s) { n = strlen(s); buf = malloc(n + (9 - (s+1)));
  return FUN_006C5D40(buf, n+8, "--- %s ---", s); }
```
- `0x6D0450` 内: `mov eax,[0xB3CD7C]; push eax; call 0x70F910`（v417_1）→ **`[0xB3CD7C]` の値が文字列 arg**。

### §2.2 `FUN_0047C930`（L99379-99399）
```c
void FUN_0047C930(this, p2, p3) {
  if (p3) {
    for (e = [this+0x9C]; e; e = *e) {          // list: next=[0], elem=[2]
      elem = e[2];
      if (elem && elem[3]) (**(*elem + 0x50))(p2);   // slot20 notify    }
  }
  for (n = [this+0xC]; n; n = n[0xd])                // map walk (v406 layout)
    (**(*n + 0x54))(p2);                             // slot21 notify}
```

### §2.3 owner 群（decompiled grep `b3cd7c`）
L91918(`FUN_00473120`)／L91987(`FUN_00473200`→`FUN_006FFE90`)／L99429/99461/99638（等値ループ×3）／L101786(`FUN_0047F750`)／L120536(`FUN_004964F0`)／L178884-86(**`FUN_004E3490` → `word[iVar6+8] |= 0x40`**)／L496379(`FUN_006C6910`)／L497751(`FUN_006C8220`)。

## §3 残点 **V-419-1..3**
1. **【中】`0xB3CD7C` の中身（実行時文字列）** — 実データ/初期化経路（起動時にこの global へ何の char* が入るか）と、`FUN_00700010` の key として「そのアドレス」を使う設計意図（NiFixedString static or 登録キー）の最終確定。
2. 【低】`0x6D0450`（未注記 fn・`[0xB3CD7C]` を dump する側）の identity。
3. 【低】V-418-2/V-416-2/V-415-2 引継ぎ: `L498845` slot23 と `manager[+0x30]` 正体。

## §4 検証・自己批評
- probe: **v418_1 全 EXIT=0**（1本で V-418-1 を [部分確定: 高精度] へ＋副次大収穫）。
- **決定打の作り**: (i) **decompiled grep（小文字込み）で owner 群を一括取得** → (ii) **出力ヘルパ `FUN_0070F910` の body 実読（`"--- %s ---"` ＋ strlen＋malloc）で arg 型（char*）を確定** → (iii) **`FUN_0047C930` の body 実読で `[+0x9C] list` と `[+0xC] map` の2構造 bulk 通知を即判定**（v405/v406 の構造知識を直結）。
- 妥当性チェック: ①`FUN_0070F910(char*)` ⇔ `0x6D0450` の `push [0xB3CD7C]` — ②`FUN_0047C930` の `[+0x9C]`/`[+0xC]` ⇔ v405 StoreTargets の resolved-node リスト ⇔ v406 FUN_00700010 の map layout — ③等値ループ×4⇔v417 のインライン分類 — **三方向矛盾なし**。
- docs との不整合: **なし**（v417 の V-418-1 を高精度化）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **型特定の終着点は「受け手 fn の arg 型」** — `[global]` の値の消費者（本件 `FUN_0070F910(char*)`）を body 実読すれば値型が決まる。
  2. **bulk 通知 fn は「リスト走査＋map 走査の2ループ＋各 virtual 呼び出し」の body で即判定** — これまで別々の残点（+0x9C リスト・map）が同一 fn にまとまっていた。
  3. **既知残点の「構造名」（+0x9C list／map layout）を手元に持つと、未知 fn の body を読んだ瞬間に identity が当たる**（v405/v406 の資産活用）。
