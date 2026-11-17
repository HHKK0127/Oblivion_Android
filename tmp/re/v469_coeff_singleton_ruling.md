# v469 — 係数 `-1.0` 確定と `DAT_00B35EB8` singleton の裁定（V-469-1..3 完遂・旧残点 V-406-3 継続第6弾）

- 採番: **v469**（旧残点群消化・probe prefix **v469_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-469-1..3（v468 残点）** — 1=旧残点 V-406-3 継続（`DAT_00B35EB8` と `_DAT_00A3D360` の identity → id `0x4C` float の正式な語）／2=旧残点 V-406-4（`DATA+0x2C/+0x31`）と V-406-5（`PTR_DAT_00B10D90`）／3=V-468-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-469-1: `_DAT_00A3D360`（乗算係数）の identity** | **[確定]** | **.rdata `0xA3D360` の raw: `00 00 00 00 00 00 F0 BF` ＝ float64 `-1.0` [確定]** → **`FUN_00420C60((float)piVar8[10] * (float)_DAT_00a3d360)` ＝ `elem[10] × (-1.0)` ＝符号反転 [確定]** → **id `0x4C` float ＝ `-elem[10]`（負の値）[確定]** |
| **V-469-1: `DAT_00B35EB8`（`virtual+0x170()` の比較対象）の identity** | **[確定相当]** | **4-dword（16byte）構造体の先頭 `0xB35EB8/BC/C0/C4` [確定(機械)]**: `FUN_004492E0`（L56295）が `mov [0xB35EB8], ebp`＋`0xB35EBC/C0/C4` へ連続4 dword 書き込み／`FUN_0044AC20`（L57509・v466 flag writers caller）が同様に `ebx` で4 dword 書き込み／`FUN_0044AC20` 内 `0x44B999: mov [0xB35EB8], eax`→**`push 0x3C; call 0x401F00`（alloc 0x3C=60 bytes）**／`0x44B9E4: or eax,0xB35EB8`＋`mov edx,[ecx]; mov eax,[edx+0xD8]`（virtual `+0xD8` call）＋`push 0xA38154` [確定(機械)] → **`DAT_00B35EB8`＝alloc 0x3C で初期化される singleton object（4-dword header＋`+0xD8` virtual 使用）[確定相当]**、docs mentions 13件（L56800/57560/57914…）。**`virtual+0x170()` の結果と `DAT_00B35EB8` の比較＝singleton 等値判定 [推定高]**（何の singleton かは [未確定]＝残点） |
| **V-469-1: id `0x4C` float の正式な語** | **[部分確定]（式）＋[未確定]（semantic）** | **式は確定: `id_0x4C_float = -(list_elem[+0x1C? no: +40 float])`（`elem[10]`＝`piVar8[10]`＝byte +40）when `byte[class_form+0x24]&1` and `virtual[elem]+0x170() == DAT_00B35EB8` [確定(構造)]**。**elem の class（`virtual+0x170()` の receiver）と「何の負の値か」は [未確定]＝残点**（`param_1+0x48` list の insert 元が docs body に無く未特定） |
| **V-469-2: 旧 V-406-4/5** | **[未確定]（残点継続）** | 本 probe 未着手（budget は V-469-1 に集中） |
| **V-469-3: V-468-3 引継ぎ** | **[未確定]（残点継続）** | `FUN_005222D0` 一語名別法、predicate slot35 英名裏取り |

## §2 証拠（v469_1）

### §2.1 係数（v469_1a）
```
_DAT_00A3D360 (.rdata): hex 00 00 00 00 00 00 F0 BF
  float32 = 0.0 (first 4 bytes) / float64 = -1.0 (full 8 bytes)
  -> _DAT_00a3d360 is a double -1.0; (float) cast -> -1.0f
  FUN_00420c60((float)piVar8[10] * (float)-1.0) = -(elem[10])
```

### §2.2 `DAT_00B35EB8`（v469_1b）
```
refs: 0x449CC4 mov [0xb35eb8], ebp / 0xb35ebc / 0xb35ec0 / 0xb35ec4 (FUN_004492e0, L56295)
      0x44AC75 mov [0xb35eb8], ebx / 0xb35ebc / 0xb35ec0 / 0xb35ec4 (FUN_0044ac20, L57509)
      0x44B999 mov [0xb35eb8], eax ; push 0x3c / call 0x401f00 (alloc 0x3C)
      0x44B9E4 or eax, 0xb35eb8 / mov edx,[ecx] / mov eax,[edx+0xd8] / push 0xa38154
      0x44B9E9 mov edx, [ecx] / mov eax, [edx+0xd8]   (virtual +0xD8 call)
docs mentions: 13 (L56800, L57560, L57914, L57916, L57920, L57923, ...)
```

### §2.3 `FUN_004CD960` list（v469_1c）
```
docs body mentions of 0x48 / param_1[0x12]: (none in body slice)
  -> list insert source not visible in docs body = residual
```

## §3 残点 **V-470-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`DAT_00B35EB8` singleton の class（`virtual+0x170()` receiver＝`param_1+0x48` list elem の class）と「何の負の値か」の semantic**（id `0x4C` float の正式な語確定へ）。
2. 【低】**旧残点 V-406-4／V-406-5** — `DATA+0x2C/+0x31` 意味（UESP 差分の docs 内裏取り）／`PTR_DAT_00B10D90` 所有 fn と spec 名前テーブル書手。
3. 【低】**V-469-3 引継ぎ** — `FUN_005222D0` 一語名別法と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v469_1 の1本 EXIT=0**（V-469-1 は係数 `-1.0` 確定＋`DAT_00B35EB8` singleton 確定相当＋式確定／V-469-2/3 は未着手＝残点）。
- **決定打の作り**: (i) **float 定数は .rdata の raw を IEEE754 で読むだけ**（`-1.0` が即決）→ (ii) **`DAT_00B35EB8` は .text refs の「4-dword 連続書き込み」パターンと `push 0x3C; call 0x401F00`（alloc）で singleton と特定** → (iii) **`or eax,0xB35EB8`＋`mov eax,[edx+0xD8]`（virtual call）で使用形を確認** → (iv) list の insert 元が docs body に無いことは**否定側として記録**。
- 妥当性チェック: ①`-1.0`⇔`(float)` cast⇔`elem[10] × (-1)` — ②4-dword 連続書き（`0xB35EB8/BC/C0/C4`）⇔16byte header 構造 — ③`push 0x3C; call 0x401F00`⇔alloc 0x3C（v468 の `FUN_00401F00(0x10)` と同系）— ④`virtual+0xD8`⇔`+0xD8` slot 使用 — ⑤docs mentions 13件⇔複数 fn が使用 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **float 定数は `.rdata` raw を IEEE754（f32/f64 両方）で読む** — 8byte で `-1.0`（f64）と判別できた（**4byte だけ読むと 0.0 と誤認する**）。
  2. **singleton の特定は「4-dword 連続書き込み」＋「alloc サイズ付き `call 0x401F00`」の2証拠**（header 構造と確保サイズが揃う）。
  3. **`or eax, <addr>` は「アドレスをビット演算で混ぜる」mid-instruction 偽陽性の典型**（`0x44B9E4: or eax, 0xb35eb8` は `push 0xa38154` の一部と誤decode）— **byte-pattern ref は forward decode の整合で裏取る**（v446 の教訓再現）。
  4. **docs body に field の提及が無い＝insert 元が docs に無い** — 否定側として記録し残点の形を変える。
  5. **係数が `-1.0` なら「符号反転コピー」と読める** — float定数の値が式の意味を一気に決める。
