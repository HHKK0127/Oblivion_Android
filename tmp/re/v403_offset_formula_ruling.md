# v403 — `Offset`（`[seq+0x48]`）の数式上の役割の裁定（#153 ⑥-9・V-387-2）

- 採番: **v403**（ai-todo #153 束の第14弾＝低10件の第9弾、probe prefix v403_1）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v387 の残点 **V-387-2（Offset の数式上の役割 — [推定高] で時間オフセットとされたものの、式中の各项の確定）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。名前 "Offset" は v387/v402 で確定済み。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **数式上の役割（本質）** | **[確定(命令＋decompiled)]** | **`effective_time = time + Offset` に変換する加算補正** — pose クエリ **`FUN_006c6b30(seq, time + Offset)`** と **time remap `0x6C5FC0`（v390 の `Freq×(t−Last)+WeightLast` wrap）へ `time+Offset` を入力**（decompiled L499594/L499613/L499620・machine `0x6CAABC: fadd [esi+0x48] → call 0x6c6b30`／`0x6CABC9: fadd [eax+0x48] → call 0x6c6b30`／**`0x6CABED: fadd [esi+0x48] → call 0x6c5fc0`**）。**Offset を足すことで「シーケンスローカル時刻」に変換する** |
| **遅延初期化** | **[確定(命令＋decompiled)]** | **`if (Offset == −FLT_MAX) Offset = −time`**（`FUN_006ca950` L499541-542・machine `0x6CA961-0x6CA97D`）→ **effective = time − 激活時刻 = 経過時間**（未設定 −FLT_MAX を「初回 update で現在時刻の負値」に置換） |
| **再基準化（一回限りのスナップ）** | **[確定(命令＋decompiled)]** | **`if ([+0x54] ≠ −FLT_MAX) { Offset = [+0x54] − time; [+0x54] = −FLT_MAX; }`**（L499586-588・machine `0x6CAA4F` ブロック）→ **effective を保留時刻 `[+0x54]` にスナップして同フィールドを消費** |
| **終了時の累積（繰越）** | **[確定(命令＋decompiled)]** | **Deactivate（fade<=0）で `Offset += (Last ≠ −FLT_MAX ? WeightLast/Freq − Last : 0)`**（`FUN_006c9cb0` L498944-546・machine `0x6C9D0E: fadd [ecx+0x48]; fstp [ecx+0x48]`）＝**停止時に未消化の時間分を Offset へ加算**（v393 の即時停止パスと一致） |
| **リセット** | **[確定(命令＋decompiled)]** | **`Offset = −FLT_MAX`** — Activate（`FUN_006c9ba0` L498920・machine `0x6C9C99`）と blend wrapper（`FUN_006c9e00` L499049・machine `0x6C9ED6`）＋各 ctor（`0x6C65C0`/`0x6C805A` 帯で Start/End=−FLT_MAX と同時） |
| **blend 中の同期（パートナー）** | **[確定(命令＋decompiled)]** | state6 で **`partner[+0x48] = FUN_006c6b30(this, time + Offset) / partner.Freq − time`**（L499594-597・machine `0x6CAAF4`）＝**partner の effective time を「this の（remap 済み）時刻」に一致させる**。併せて **`partner[+0x48] + time ≠ partner[+0x34]（Last）` チェック**（L499616-617）で partner 更新要否を判定 |
| （参考）表示 | **[確定]** | v402 のテキストダンプに `"Offset"` 行（L499403・`0xA35FEC`）＝machine `0x6CA677` の read |

**V-387-2 への回答**: Offset は **`effective_time = time + Offset` を作る加算補正 [確定]**。ライフサイクル: **reset（−FLT_MAX）→ 初回 update で `−time`（＝経過時間化）→ 状態遷移で `[+0x54]` からスナップ再基準 → 停止時に `WeightLast/Freq − Last` を累積 → blend 中は partner の effective time 同期に使用**。

---

## §2 証拠

### §2.1 主計算（effective_time 生成）

```
FUN_006ca950（update・state6 ブロック）:
  L499594: local_4 = time + Offset                    ★effective_time
  L499595: fVar2 = FUN_006c6b30(seq, local_4)         （pose/weight クエリ）
  L499596: local_4 = fVar2 / [partner+0x28]（Freq）
  L499597: [partner+0x48] = local_4 − time            ★partner Offset 同期
  L499613: time = time + Offset（partner なし経路）
  L499620: FUN_006c6b30(partner, time + partner.Offset)
machine:
  0x6CAABC: fadd [esi+0x48] → call 0x6c6b30
  0x6CABC9: fadd [eax+0x48] → call 0x6c6b30
  0x6CABED: fadd [esi+0x48] → push 1 → call 0x6c5fc0   ★v390 time remap へ
```

### §2.2 初期化・再基準・累積・リセット

| site | 式 | decompiled |
|---|---|---|
| `0x6CA961-97D` | `Offset == −FLT_MAX → Offset = −time` | L499541-542 |
| `0x6CAA4F` ブロック | `[+0x54]≠−FLT_MAX → Offset = [+0x54] − time; [+0x54] = −FLT_MAX` → state=1 | L499586-591 |
| `0x6C9D0E` | `fadd [ecx+0x48]; fstp [ecx+0x48]`（**Offset += WeightLast/Freq − Last**）→ state=0・partner=0・`[+0x54]=−FLT_MAX`・Unregister | `FUN_006c9cb0` L498944-551 |
| `0x6C9C99` / `0x6C9ED6` | `Offset = −FLT_MAX`（Activate / blend wrapper） | L498920 / L499049 |
| `0x6C65C0` / `0x6C805A` | ctor 系: Offset と同時に `Start/End = −FLT_MAX` | （帯内 machine） |

### §2.3 scope 注意

- 帯内でも**別構造の `+0x48`** が存在（`FUN_006bfcd0`/`FUN_006bfdb0`/`FUN_006bfe90`（L491231-1297・0x6BFCxx 帯）・`FUN_006c0170`（L491392-93）＝ interpolator 系の own float フィールド）。本裁定の **`Offset` は NiControllerSequence の `[+0x48]` のみ**に適用。
- `0x6D0305: fldcw [edx+0x48]` は **FPU control word**（float フィールドではない）＝対象外。

## §3 残点 **V-403-1..3**
1. **`[+0x54]` フィールドの名前と供給元**（Offset 再基準化の「保留時刻」を誰がいつ書き込むか・dump 対象外の内部時刻）【中】
2. **state2 ブロック出口で state=1 へ遷移する条件**（`0x6CAA4F` ブロックの入口分岐＝fade-in から即時系へ抜ける条件）【低】
3. **`FUN_006c6b30` の戻り値の厳切な意味**（pose/weight の何を返すか・`/Freq` 正規化の根拠）【低】

## §4 検証・自己批評
- probe: **v403_1 全 EXIT=0**（x87 disp=0x48 census 16件＋decompiled 帯 grep＋`FUN_006ca950` body 全 0x48 行）。
- **決定打の作り**: (i) **x87 census（fld/fadd/fstp disp=0x48）で帯内 read/write 全数を取得** → (ii) decompiled で各 site を式に翻訳 → (iii) `FUN_006ca950` body の0x48 行を初回から末尾まで通読して **effective_time 生成・スナップ・partner 同期**を確定 → (iv) Deactivate の `fadd`（v393）と接続。
- 妥当性チェック: ①`Offset = −time` ⇔ `effective = time + Offset = 経過時間` — ②`fadd [esi+0x48] → call 0x6c5fc0` ⇔ v390 time remap 入力 — ③`Offset += …` ⇔ v393 Deactivate パス — ④`partner.Offset = …/Freq − time` ⇔ `time + partner.Offset = …/Freq`（同期の自己整合）— ⑤dump の "Offset" ⇔ `0xA35FEC` — **五方向で矛盾なし**。
- docs との不整合: **なし**（v387 の V-387-2 を [推定高]→**[確定]** に昇格）。
- **教訓**: **「〜の役割」系は ①入力元 census（x87 disp 走走査）②出力先（どの call へ流れるか）③ライフサイクル（init/更新/累積/リセット）の4点で式を復元**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
