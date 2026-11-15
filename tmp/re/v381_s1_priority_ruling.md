# v381 — `0x6C9BA0`（ActivateSequence）の s1 の厳密な型の裁定（#150 残点・V-380-1）

- 採番: **v381**（#150 残点 V-380-1 消化、probe prefix v381_1）
- 対象: Oblivion.exe v1.2.0416 ＋先行裁定 v187_anim_residuals.md
- 依頼経緯: v380 の残点 **V-380-1（s1 の厳密な型: weight vs interpolator ID）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **s1（`[esp+0xC]`）の厳密な型** | **[確定(命令)]** | **priority（byte）** — `0x6C6A50` の `0x6C6A7D: movsx edx, byte [esp+0x18]`（**s1 を符号拡張**）→ `0x6C6A91: push edx` → **`0x6CC5C0`（RegisterInterpolator）の arg3（`[ebp+0xC]`＝m_cPriority）** へ格納。`[esi+0xD]`（block の byte flag）が **0xFF のときのみ s1 が使われる**（否则 unit 側の flag 優先 `movzx edx,dl`） |
| `0x47603A` の s1=0x64(100) | **[確定(命令)]** | **priority 100（最高優先）** — s1 が priority byte であるため |
| `0x6C6A50` の identity | **[確定(命令)]**（v380 から補強） | **各 block の field8 interpolator へ `RegisterInterpolator(block, interpolator, priority, 0.0, 1.0)` を呼ぶループ fn**（`ret 4`＝1引数＝s1） |
| `0x6CC5C0` の引数レイアウト | **[確定(命令)]**（v187 L132 補強） | **`ret 0x10`＝4スタック引数**: **arg1（`[esp+0x10]`）＝block ptr**（unit[0] へ格納）／**arg2（`[esp+0x18]`）＝float weight**（`[ebp+4]`＝m_fWeight）／**arg3（`[esp+0x1C]`）＝byte priority**（`[ebp+0xC]`＝m_cPriority）／**arg4（`[esp+0x20]`）＝float**（`[ebp+0x10]`＝m_fEaseSpinner） |

**#45 N5b への回答（v380 残点1）**: **s1 は priority（byte）**（weight でも interpolator ID でもない）。`0x6C9BA0` の6引数は **s1=priority／s2=ease out flag／s3=duration／s4=fade／s5=相手 sequence／s6=ブレンド flag**。

---

## §2 証拠

### §2.1 `0x6C6A50` 全体 decode（v381_1(a) unfiltered）

```
6C6A50: push ecx/ebp/edi; xor ebp,ebp; mov edi,ecx（this=owner）
6C6A57: cmp [edi+0xC],0（block 数）
6C6A62: esi = [edi+0x14]（配列A＝block 構造・stride16）
6C6A65: eax = [esi+ebp]（block ptr）
6C6A6E: ecx = [esi+8]（★field8＝解決済み interpolator）
6C6A75: dl = [esi+0xD]（block の byte flag）
6C6A78: cmp dl,0xFF → 0xFF なら [esp+0x18]（★s1）を符号拡張
6C6A84: 否则 movzx edx,dl（unit 側 flag 優先）
6C6A87: fld1; push 1.0; fldz; push 0.0; push edx（priority）
6C6A92: push ecx（field8 interpolator）
6C6A96: push eax（block ptr）
6C6A97: eax = [ebx+0x98]（field8 の vtable slot+0x98）
6C6A9D: call eax（★RegisterInterpolator(block, interp, priority, 0.0, 1.0)）
6C6A9F: [esi+0xC] = al（結果 byte）
6C6AA2: i++; ebp += 0x10（stride16）
6C6ABA: ret 4（1引数＝s1）
```

### §2.2 `virtual+0x98` = `0x6CC5C0`（v381_1(b)）

- **NiBlendTransformInterpolator vtbl `0xA79E54` の slot+0x98 = `0x6CC5C0`** — **v187 L132 の RegisterInterpolator と完全一致**。
- `0x6CC5C0` の first insns: `push ebx; push esi; mov esi,ecx; mov dl,[esi+0xD]`（m_ucInterpCount）→ v187 L132-134「空きユニット探索 → interp/priority/weight 格納 → +0x0E++ → priority 範囲更新」[確定]。

### §2.3 `0x6CC5C0`（RegisterInterpolator）の引数レイアウト（v381_1(c) unfiltered）

- `ret 0x10`＝**4スタック引数**:
  - **arg1（`[esp+0x10]`）＝block ptr** — `0x6CC60C: edx=[esp+0x10]` → `0x6CC63C: [ebp]=edx`（unit[0]）
  - **arg2（`[esp+0x18]`）＝float weight** — `0x6CC651: fld [esp+0x18]; fstp [ebp+4]`（m_fWeight）
  - **arg3（`[esp+0x1C]`）＝byte priority** — `0x6CC655: al=[esp+0x1C]; [ebp+0xC]=al`（m_cPriority）
  - **arg4（`[esp+0x20]`）＝float** — `0x6CC65F: fld [esp+0x20]; fstp [ebp+0x10]`（m_fEaseSpinner）
- 満杯時 `virtual+0xA4`（拡張）→ 失敗で `or al,0xFF; ret 0x10`（0xFF＝失敗）[確定]。

## §3 残点 **V-381-1..2**
1. **`[esi+0xD]`（block の byte flag）と s1 の優先関係の実運用**（0xFF のとき s1 が使われる条件・caller 側で s1 に何を渡すかの census）【低】
2. **`0x6CC5C0` の arg2/arg4（weight 0.0 / ease1.0）が Activate の s3/s4 とどう接続されるか**（`0x6C6A50` は常に 0.0/1.0 を渡すため、s3/s4 は別経路 [推定]）【中】

## §4 検証・自己批評
- probe: **v381_1 全 EXIT=0**。
- **決定打の作り**: (i) `0x6C6A50` 全体 unfiltered decode で **s1 が `movsx` されることを直接確認** → (ii) `virtual+0x98` = `0x6CC5C0` を vtable 実測で特定 → (iii) `0x6CC5C0` の引数レイアウトを unfiltered で確定。
- 妥当性チェック: ①s1→`movsx`→RegisterInterpolator arg3（priority byte）⇔ v187「m_cPriority」— ②`0x47603A` の s1=100⇔ priority 100 — ③`0x6C6A50` の stride16⇔ v372 の block 配列 — **三方向で矛盾なし**。
- docs との不整合: **なし**（v380 の残点を解消。v187 L132 の RegisterInterpolator 記述を本 v381 が接続）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
