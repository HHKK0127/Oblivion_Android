# v390 — `0x6C5FC0`（time remap helper）の identity の裁定（#153 ①・V-382-2）

- 採番: **v390**（ai-todo #153 束の第1弾、probe prefix v390_1..v390_2）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v382 の残点 **V-382-2（`0x6C5FC0`（time remap helper）の正式 identity）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0x6C5FC0` の identity | **[確定(命令)]** | **sequence の time remap fn** — decompiled L495756 `void __thiscall FUN_006c5fc0(int param_1, float param_2, char param_3)`（**this / float time / char flag**・L495752 に `// FUN_006c5fc0 @ 0x6c5fc0` 注記）。**数式（フィールド名は v387 の exe label）**: **`t' = Freq[+0x28] × (time − Last[+0x34]) + WeightLast[+0x38]`**（`Last == −FLT_MAX` 未設定なら `t' = Freq × time`＋offset も 0）→ **flag `[+0x24]==0` のとき wrap**: `span = End[+0x30] − Begin[+0x2C]`、範囲外なら **CRT 数学 thunk `0x982BFA`（`mov edx,0xB30AF0; jmp 0x9908C0`・2 float 引数＝fmod 系 [推定高]）で wrap** → **`+ Begin`** → 最終 clamp を **[Begin, End]**（下回る→Begin／超える→End）／**flag ≠0 は wrap スキップ（clamp のみ）**。戻り値=float（`fld [esp+4]`） |
| this の型 | **[推定高]** | **NiControllerSequence** — 0x6C5FC0 が読む `+0x28/+0x2C/+0x30/+0x34/+0x38` が **v387 の exe label（Freq/Begin/End/Last/WeightLast）と5/5 一致**、game 側 wrapper `0x49F4A0` が使う `+0x44/+0x48` も **State/Offset と一致（計7/7）**。vtbl/ctor での直接照合は残点 |
| game 側 wrapper `0x49F4A0` | **[確定(命令)]** | `ret 4`（1引数 float time）: **`[this+0x44] − 1 ≤ 2`（＝state ∈ {1,2,3}）のとき `return 0x6C5FC0(this, Offset[+0x48] + time, 1)`、否则 `return 0.0`**（`fldz; ret 4`）。decompiled L126640 `FUN_006c5fc0(*(float*)(param_1+0x48) + param_2, 1)` と一致 |
| flag（param_3・char） | **[未確定]** | decompiled シグネチャに存在するが、decoded 範囲（0x6C5FC0..0x6C60D0）で `[esp+0x18]` 参照なし＝**使用箇所は未デコード尾部**＝残点 V-390-1 |

**V-382-2 への回答**: `0x6C5FC0` は **`Freq×(t−Last)+WeightLast` を `[Begin,End]` へ wrap/clamp する time remap [確定(命令)]**。テキストキー同期（flag=0・wrap あり）と重みパス（flag=1・clamp のみ）、game 側 state ゲート wrapper（`0x49F4A0`）から呼ばれる。

---

## §2 証拠

### §2.1 本体（v390_1/v390_2 unfiltered 0x6C5FC0..0x6C60D0）

```
6C5FC0: sub esp,0xC; push esi; mov esi,ecx
6C5FC6: [esp+8] = [+0x38]（WeightLast・offset 仮置き）
6C5FCD: fld [+0x34]（Last）
6C5FD0: fld −FLT_MAX; fucompp; jp → Last 設定済み判定
6C5FE3:（未設定）[esp+8] = 0; fld time          → 6C5FF4
6C5FED:（設定済み）fld time; fsub [+0x34]        → elapsed = time − Last
6C5FF4: cmp [+0x24],0; jne 0x6C60B7            ; ★flag
6C5FFC: [esp+8] = [+0x28](Freq) × elapsed + offset
6C6019:（flag==0）span = [+0x30](End) − [+0x2C](Begin)
6C6032..6C605B: 範囲外判定 → call 0x982BFA（CRT thunk・2 float）
6C6068: fadd [+0x2C]（+ Begin）
6C607B..6C60D0: 結果 vs Begin/End → 下回る→Begin / 超える→End（clamp）
6C60AA: fld [esp+4]（戻り値 float）
```
- **`0x982BFA`**（v390_2(b)）: `mov edx, 0xB30AF0; jmp 0x9908C0` ＝ **CRT import thunk**（2 float 引数・float 戻り＝fmod 系 [推定高]・正確な CRT 名は残点）。

### §2.2 callers（v390_1(b)/v390_2(c)）

| caller | 文脈 | flag |
|---|---|---|
| `0x6C6B4E`（`0x6C6B30` テキストキー同期内・v187 L140） | `fstp [esp+0x30]` → `[edi+0x20]` の reader 経由で使用 | **0**（decompiled L496524） |
| `0x6CAC00`（重みパス `0x6CA950` 内・v382） | `fstp [esp+8]` → arg2 として `0x6C6DC0` へ | **1**（decompiled L499627） |
| `0x49F4C0`（fn `0x49F4A0`） | 下記 | **1** |
| `0x49F6A5`（同帯 tiny fn） | `call; fstp st(0); …; ret 4`（結果は即 pop＝用途は後続 [部分確定]） | 1（同型 [推定]） |

**fn `0x49F4A0`（v390_2(c) unfiltered）**:
```
49F4A0: eax = [ecx+0x44]（State）
49F4A3: eax -= 1; cmp eax,2; ja → 0x49F4C8   ; state ∈ {1,2,3} のみ通過
49F4AB: fld [ecx+0x48]（Offset）
49F4AE: push 1（param_3）
49F4B0: fadd [esp+8]（+ time 引数）
49F4B4..49F4BD: (Offset+time) を param_2 スロットへ
49F4C0: call 0x6C5FC0
49F4C5: ret 4
49F4C8: fldz; ret 4                     ; 他 state → 0.0
```
- 付近: `0x49F490: jmp 0x6C87A0`（v187 C2 の factory への jump thunk・別物）。

### §2.3 this の型の根拠（v387 との突合）

| 0x6C5FC0/0x49F4A0 が読む offset | v387 の exe label | 一致 |
|---|---|---|
| +0x28（倍率） | Freq | ✓ |
| +0x2C（下限） | Begin | ✓ |
| +0x30（上限） | End | ✓ |
| +0x34（減算基準） | Last | ✓ |
| +0x38（加算項） | WeightLast | ✓ |
| +0x44（state ゲート） | State | ✓ |
| +0x48（time 加算） | Offset | ✓ |

→ **7/7 一致＝this = NiControllerSequence [推定高]**。

## §3 残点 **V-390-1..3**
1. **param_3（char flag）の使用箇所と意味**（fn の未デコード尾部 0x6C60D0 以降・`[esp+0x18]` 参照探索）【低】
2. **`0x49F4A0`/`0x49F6A5` の this の型の直接確定**（7/7 offsets 一致＝[推定高]・vtbl/ctor 照合）＋**`0x982BFA`（CRT thunk）の正確な関数名**（IAT/エクスポート照合）【低】
3. **flag `[this+0x24]` の名前**（wrap 有効/無効のどちら側か・exe label 不在の可能性）【低】

## §4 検証・自己批評
- probe: **v390_1・v390_2 全 EXIT=0**。
- **決定打の作り**: (i) 本体 unfiltered（freq/last/offset 数式＋wrap/clamp）→ (ii) **decompiled の fn def（L495752/56）と4 callers の flag 実値**→ (iii) **CRT thunk の実バイト確認**→ (iv) **v387 label との7/7 offset 突合**。
- 妥当性チェック: ①数式⇔ v387 ラベル（Freq が倍率・Begin/End が範囲）— ②`0x49F4A0` の `Offset+t`⇔ L126640 逐語一致— ③`−FLT_MAX` 未設定判定⇔ v387 の ctor 初期化 — **三方向で矛盾なし**。
- docs との不整合: **なし**（v382 の V-382-2 を解消）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
