# v380 — `0x6C9BA0`（ActivateSequence）の s1/s2 引数の意味の裁定（#150 残点・V-379-1）

- 採番: **v380**（#150 残点 V-379-1 消化、probe prefix v380_1）
- 対象: Oblivion.exe v1.2.0416
- 依頼経緯: v379 の残点 **V-379-1（s1/s2 の厳密な意味）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **s1（`[esp+0xC]`）の意味** | **[確定(命令)]＋[推定高]** | **interpolator 引数** — `0x6C9BE3: mov eax,[esp+0xC]; push eax; call 0x6C6A50`（**`0x6C6A50`＝field8 interpolator の `virtual+0x98` 呼び出し**・v372/v376 確定）。caller `0x47603A` の **s1=0x64(100) は weight（百分率）** [推定高]（`0x47484E` 帯で `fild; fdiv qword [0xA3AA50]`＝**255 で割る weight 正規化**を通過した値） |
| **s2（`[esp+0x10]`）の意味** | **[確定(命令)]** | **byte flag（0/1）** — `0x6C9C8A: cmp byte [esp+0x10],0; je 0x6C9C9C; fld [0xA7DEB4]; fchs; fstp [esi+0x48]`＝**非0のとき `[esi+0x48]`（ease out 時間 [推定]）に `-const` をセット**。s2=1 の caller `0x47491C`（fn `0x474530`・weight 正規化経路）／s2=0 の caller 多数 |
| `0x6C9BA0` の6引数の完全図 | **[確定(命令)]＋[部分確定]** | **s1＝interpolator 引数（weight [推定高]）／s2＝byte flag（ease out 設定用）／s3（float）＝duration（`[esi+0x1C]` へ）／s4（float）＝fade（`0 < s4` で state 2/5、`s4 ≤ 0` で state 1）／s5＝相手 sequence ptr（`[edi+0x58]` 記録）／s6＝flag byte（0→state2 / 1→state5・v379 確定）** |

**#45 N5b への回答（v379 残点1）**: s1 は **interpolator 引数（`0x6C6A50` へ）**、s2 は **byte flag（ease out 設定用）**。state 1/2/5 の分岐は **s4（fade duration）と s6（ブレンド flag）**が担う。

---

## §2 証拠

### §2.1 `0x6C9BA0` 全体 decode での s1/s2 使用（v380_1(a) unfiltered）

```
6C9BE3: mov eax, dword [esp+0xC]        ; ★s1
6C9BE7: push eax
6C9BE8: mov ecx, esi
6C9BEA: call 0x6C6A50                   ; ★field8 interpolator の virtual+0x98 呼び出し（s1 を引数に）
...
6C9C8A: cmp byte [esp+0x10], 0          ; ★s2
6C9C8F: je 0x6C9C9C
6C9C91: fld dword [0xA7DEB4]
6C9C97: fchs
6C9C99: fstp dword [esi+0x48]           ; ★s2≠0 → [esi+0x48] に -const（ease out [推定]）
6C9C9C: pop edi; mov al,1; pop esi; ret 0x18
```

### §2.2 s1=100（0x64）の caller（v380_1(b) unfiltered・fn `0x475D80`）

- `0x47601C: mov esi, eax`（ノード ptr・`virtual+0x10` 経由で取得）
- `0x476022..0x47603A`: `fldz; push 0; push 0; sub esp,8; fstp[esp+4]=0.0; fld1; fstp[esp]=1.0; push 1; push 0x64; call 0x6C9BA0` → **s1=100, s2=1, s3=1.0, s4=0.0, s5=0, s6=0**（s4=0 → **state 1**）
- **s1=100 の出所**: `0x475D80` fn 内の weight 正規化経路（`0x47484E` 帯と同型の `fild; fdiv [0xA3AA50]`）[推定高]

### §2.3 s2=1 の caller（v380_1(c) unfiltered・fn `0x474530`）

- `0x47484E: ebx=[esp+0x14]` → `[esp+0x34]` に weight 計算（`movzx eax,al; [esp+0x34]=eax; fild; fdiv qword [0xA3AA50]; fstp`＝**255 で割る正規化**）
- `0x4748E1: fld [ebp+0x2C]; fadd qword [0xA2FC68]; fstp [ebp+0x48]`（ease 系）
- `0x474902..0x47491C`: `push 0; fldz; push 0; sub esp,8; fstp[esp+4]=0.0; fld1; fstp[esp]=1.0; push 1; push 0; call` → **s1=0, s2=1, s3=1.0, s4=0.0, s5=0, s6=0**（s4=0 → **state 1**）

## §3 残点 **V-380-1..2**
1. **s1 の厳密な型**（weight（百分率）vs interpolator ID。`0x6C6A50` の中身（`virtual+0x98` の引数解釈）decode で確定可）【中】
2. **`[esi+0x48]` のフィールド名**（ease out 時間 [推定]・`0xA7DEB4` 定数値）【低】

## §4 検証・自己批評
- probe: **v380_1 全 EXIT=0**。
- **決定打の作り**: (i) `0x6C9BA0` 全体 unfiltered decode で s1/s2 の**使用箇所を直接特定**（`0x6C9BE3`/`0x6C9C8A`）→ (ii) s1=100/s2=1 の caller fn 全体を取得し **weight 正規化経路（255 割）と接続**。
- 妥当性チェック: ①s1→`0x6C6A50`（field8 interpolator）⇔ v372/v376 の field8 消費 — ②s2→`[esi+0x48]`⇔ v377 の Deactivate が `[ecx+0x4C]/[+0x50]` をセットする対 — **矛盾なし**。
- docs との不整合: **なし**（v379 の残点を解消）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
