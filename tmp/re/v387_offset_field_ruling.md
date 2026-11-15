# v387 — `[seq+0x48]` のフィールド名と `0xA7DEB4` 定数の裁定（#152 ⑥・V-380-2・付随で V-382-1 解決）

- 採番: **v387**（ai-todo #152 束の第6弾・低優先帯、probe prefix v387_1..v387_5）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v380 の残点 **V-380-2（`[esi+0x48]` のフィールド名＋`0xA7DEB4` 定数値）** を消化。**副次的に v382 の残点 V-382-1（s3 の厳密命名）も同時に解決**
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0xA7DEB4` の定数値 | **[確定(実データ)]** | **u32 `0x7F7FFFFF` = FLT_MAX = 3.4028235e+38**。`fld [0xA7DEB4]; fchs` ＝ **−FLT_MAX ＝ 無効/未設定マーカー**（**v187 L131 の blend cache「−FLT_MAX=無効」と同一定数**）。sequence ctor `0x6C6550` は `+0x2C` を **+FLT_MAX**、`+0x30/34/38/3C/48/4C/50/54` を **−FLT_MAX** で初期化 |
| `[seq+0x48]` のフィールド名 | **[確定(命令＋文字列＋decompiled)]** | **`"Offset"`** — `FUN_006ca440`（= `0x6CA440`・decompiled L499341 `__thiscall FUN_006ca440(int this,int stream)`）のトレースが `FUN_0070fb90("Offset", *(this+0x48))` を出力（decompiled L499403・EXE 文字列 `0xA35FEC`='Offset'）。usage: ctor/Activate/state 遷移で **−FLT_MAX（未設定）**、fade 更新の初回で **−time**、重み計算で **`fadd [esi+0x48]`** により加算 → **時間オフセット（フェード/重み計算の加算補正）[推定高]** |
| （付随）**s3 の厳密命名（V-382-1 解決）** | **[確定(命令＋文字列＋decompiled)]** | **s3 → `[seq+0x1C]` ＝ `"Weight"`** — decompiled L499367 `FUN_0070fb90("Weight", *(this+0x1c))`・EXE 文字列 `0xA79DF4`='Weight'。**v379/v380 の「s3=duration」ラベルは誤り**（重み計算の `fmul [esi+0x1c]` ＝ fade 係数 × Weight と整合） |
| （付随）**NiControllerSequence の完全フィールドマップ** | **[確定(命令＋文字列)]** | `FUN_006ca440` のトレース label 全11件: **State(+0x44)／Weight(+0x1C)／Freq(+0x28)／Begin(+0x2C)／End(+0x30)／Last(+0x34)／WeightLast(+0x38)／LastScaled(+0x3C)／Offset(+0x48)／Start(+0x4C)／End(+0x50)**（+0x30 と +0x50 は exe が同じ 'End' label を再利用）。オフセット群は sequence の Activate/fade/Deactivate が書くフィールドと **5/5 一致**（+0x44 state・+0x1C Weight・+0x48 Offset・+0x4C Start・+0x50 End）＝**param_1=this＝NiControllerSequence [確定]** |

**V-380-2 への回答**: `[esi+0x48]`＝**"Offset"**、`0xA7DEB4`＝**FLT_MAX（−FLT_MAX が無効マーカー）**。**V-382-1 への回答**: s3＝**"Weight"**（duration ではない）。

---

## §2 証拠

### §2.1 `0xA7DEB4`（v387_1(a) 実読）

```
0xA7DEB4: u32=0x7F7FFFFF float=3.4028234663852886e+38   ← FLT_MAX
0xA7DEB8: 0.0 / 0xA7DEC0: -0.0（近傍は定数プール＋文字列混在）
```
- ctor `0x6C6550`（v387_1(b)）: `+0x2C = +FLT_MAX`（fchs なし）、`+0x30/+0x34/+0x38/+0x3C/+0x48/+0x4C/+0x50/+0x54 = −FLT_MAX`（`fld; fchs; fstp`）、`+0x40/+0x44/+0x58/+0x5C/+0x60/+0x64 = 0`（eax）。

### §2.2 `"Offset"` と s3=`"Weight"`（decompiled.c ＋ EXE 文字列）

decompiled.c（`FUN_006ca440`・def L499341）:
```
L499355: FUN_006c6140("State",  *(this+0x44))
L499367: FUN_0070fb90("Weight", *(this+0x1c))     ← s3 の正名
L499379: FUN_0070fb90("Freq",   *(this+0x28))     （0xA79DE0='Freq'）
L499385: FUN_0070fb90("Begin",  *(this+0x2c))
L499391: FUN_0070fb90("End",    *(this+0x30))     （0xA51C18='End'）
L499397: FUN_0070fb90("Last",   *(this+0x34))     （0xA79DD8='Last'）
L499403: FUN_0070fb90("Offset", *(this+0x48))     ← 本依頼の対象
L499409: FUN_0070fb90("Start",  *(this+0x4c))     （0xA527B0='Start'）
L499415: FUN_0070fb90("End",    *(this+0x50))     （0xA51C18='End'・label 再利用）
L499421: FUN_0070fb90("WeightLast", *(this+0x38))
L499427: FUN_0070fb90("LastScaled", *(this+0x3c))
```
- 命令側でも同一（v387_1(v382_3)）: `0x6CA677: fld [edi+0x48]; …; push 0xA35FEC('Offset'); call 0x70FB90`／`0x6CA500: fld [edi+0x1C]; …; push 0xA79DF4('Weight')`／`0x6CA6B6: fld [edi+0x4C]; push 0xA527B0('Start')`／`0x6CA6F5: fld [edi+0x50]; push 0xA51C18('End')`。
- **fn 特定**: decompiled L499341 `void __thiscall FUN_006ca440(int param_1,int param_2)` ＝ machine head **`0x6CA440`**（`mov edi,ecx; call 0x7009A0`…`push 0xA79E08('State'); call 0x6C6140`・stream 書出 `0x42BB90` 経路）と一致。

### §2.3 `[seq+0x48]`（Offset）の usage（v382/v387_1(d) 再整理）

| site | 操作 | 意味 |
|---|---|---|
| ctor `0x6C65C0` | `= −FLT_MAX` | 未設定 |
| Activate s2≠0 `0x6C9C99` / s2==0 `0x6C9C99` 系 | `= −FLT_MAX` | リセット |
| fade 更新 `0x6CA97D`（初回・`[+0x48]==−FLT_MAX` 判定後） | `= −time` | 現在時刻でオフセット初期化 |
| 重み計算 `0x6CABED` | `fadd [esi+0x48]` | **係数へ加算** |
| Deactivate `0x6C9D11` / state1 `0x6CAA4F` / state4 `0x6CAAF4` / state6 `0x6C9ED6` | 再設定 | 状態遷移ごとに更新 |

## §3 残点 **V-387-1..2**（＋V-382-1 解消）
1. **fn `0x6CA440` の正式名**（テキスト Save か Debug dump か。FUN_ 名のみで正式名未特定）【低】
2. **'Offset' の厳密な数式上の役割**（`fadd` の accumulator の出所＝経過時間換算 [推定高] だが、初回 `−time` との組み合わせの完全な導出）【低】
- **V-382-1 は本 v387 で解決**（s3="Weight"）。

## §4 検証・自己批評
- probe: **v387_1..v387_5 全 EXIT=0**。
- **決定打の作り**: (i) 定数実読（FLT_MAX）→ (ii) `[+0x48]` 読手 census で debug print site を検出 → (iii) **format 文字列を実読（'Offset'/'Weight'/'Start'/'End'）** → (iv) **decompiled.c の fn def（L499341=FUN_006ca440）と label 列挙で完全マップ確定**。
- 妥当性チェック: ①label offsets ⇔ Activate/fade/Deactivate の store offsets（5/5 一致）— ②'Freq'(+0x28) ⇔ Deactivate の `fdiv [ecx+0x28]`（頻度で割る）— ③−FLT_MAX ⇔ 全「未設定」判定（`fucompp`+`jnp`）— **三方向で矛盾なし**。
- docs との不整合: **v379/v380 の「s3=duration」を確定訂正（正＝Weight）**・v380 の `[esi+0x48]` は「ease out 時間 [推定]」と記載したが**正名＝Offset**（役割はオフセット加算で概ね整合・命名のみ訂正）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c は読み取りのみ）・rust/ 側ソース変更なし。
