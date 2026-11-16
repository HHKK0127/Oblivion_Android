# v413 — `FUN_00519F50` dispatcher の確定と V-412-1/3 追跡（派生残点第5弾・中間裁定）

- 採番: **v413**（派生残点群・probe prefix **v413_1..v413_4 の4本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v412 の残点 **V-412-1（`FUN_004677F0` の "level" 語）【中→低】／V-412-3（`Player[+0x24]` RTTI）【低】** を追跡。**主成果は新確定 `FUN_00519F50`（actor value SET dispatcher）と SET 元4 fn の特定**、V-412-1/3 は**手がかりを強化して [未確定] 維持**。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_00519F50(param_1=index, param_2=value)` の identity** | **[確定(命令)]** | **actor value 一括 SET dispatcher（switch on index）**: `case 0..7 → FUN_00468AF0(id, value)`（attribute byte setter）／`case 8 → FUN_00519C50(value)`／**`case 9 → FUN_00467290(value) ＝ [Player[+0x24]+0x8] へ SET`**／**`case 10 → FUN_004672B0(value) ＝ [Player[+0x24]+0xA] へ SET`**／`case 0x0B..0x1A+ → 続行`（L210353-210405）。→ **`Player[+0x24]` は actor value index 9/10 の格納先** [確定] |
| **`Player[+0x24]` の SET 元（両 setter の E8 caller）** | **[確定]** | **4 fn のみ・両 setter（`FUN_00467290`/`FUN_004672B0`）の caller が完全一致**: `FUN_00511570`（decompiled 未注記）／**`FUN_00519F50`（dispatcher）**／**`FUN_005222D0`**（v411 確定）／`0x523F40`（未注記・machine `cmp [esi+0xc],7 → formType7 ゲート → push 0 → SET`（0x5240B2-0x5240CE）＝v411 の `bVar18` と同型 pattern） |
| **V-412-1（`FUN_004677F0` の "level" 語）** | **[未確定]（手がかり強化）** | ①**E8=15 件**（`FUN_005222D0`・`FUN_0066C200`・**TESClass 帯 0x51C7B0/0x51CA10/0x51CA50/0x51CB80/0x51CC00**・`FUN_00488420`×3・`0x50AAA0`・`FUN_005EA350`）②使用形＝**flag `[obj+4]>>7&1` → clamp `[+0x10]`/`[+0x12]` → `<1→1` → 比較（`sVar5 < sVar4`）と `(v-1)`**③**`[Player[+0x24]+0xE]` の書手は SET dispatcher に存在しない** → **書手候補＝`FUN_00467240` / `FUN_00467320`（0x4672xx setter 同族・machine `word [reg+0xE] store` を2本発見）＝V-413-1 へ** |
| **V-412-3（`Player[+0x24]` vptr/RTTI）** | **[未確定]（追跡継続）** | Player ctor `FUN_0066A740` の 0x66A740..0x66AD40 に **`[esi/edi+0x24]` vptr store なし** → 型別初期化 fn（別パス）での設定 → 残点 V-413-3 |
| **`FUN_0046B250(id)`** | **[確定]** | `FUN_0055E000`（NiTMap Lookup）で `found ? value : 0`（L84743-84752）＝**id→value lookup helper**（L109880 の `FUN_0046B250(7)` 相当） |

## §2 証拠（v413_1..v413_4）

### §2.1 dispatcher（v413_3(a) decompiled L210353-210405）
```c
void FUN_00519f50(index, value) {
  switch (index) {
    case 0..7: FUN_00468AF0(id, value); return;   // attribute byte SET    case 8:    FUN_00519C50(value); return;
    case 9:    FUN_00467290(value); return;        // [Player[+0x24]+8]
    case 10:   FUN_004672B0(value); return;        // [Player[+0x24]+0xA]
    case 0x0B..: ...（続行）
  }
}
```

### §2.2 SET 元 census（v413_2(a)）
- `FUN_00467290` E8=4：`0x51162C`(head 0x511570)／`0x51A047`(FUN_00519F50)／`0x5226AE`(FUN_005222D0)／`0x5240CE`(0x523F40)。
- `FUN_004672B0` E8=4：`0x511617`／`0x51A036`／`0x522701`／`0x5240BC` — **完全一致（同一4 fn がペア呼出し）**。

### §2.3 `[+0xE]` 書手 census（v413_4・word store disp8=0x0E）
- **`0x467240` 帯・`FUN_00467320` 帯が `word [reg+0xE]` 書手**（0x4672xx = `FUN_00467290/004672B0` の setter 同族）＝**`[Player[+0x24]+0xE]` 書手の最有力2本**。
- 他の hit は別クラスの ctor init（`[eax+0xE]=1/3/0x10` 群・vtable 0xA47590/0xA38FB4/0xA7DB50/0xA7E858/0xA8A468 等）と struct 初期化（`+0xE=7`）＝**Player[+0x24] 系ではない**。
- **境界ズレ decode 注意**: `mov word [reg+0xE],imm16` が dword 表示になる例あり（`0x467257`）＝ capstone 開始位置調整が必要（次 probe）。

### §2.4 その他の確定（本4 probe）
- `FUN_0046B250` ＝ map Lookup（L84743-52・`FUN_0055E000`＝v404/v405 認知の NiTMap Lookup）。
- `FUN_004677F0` の usage contexts（v413_1(c)）: `FUN_00488420` 帯の `sVar5 < sVar4` 比較・TESClass 帯（L212397-212530）の `flag→clamp<1→1`・`[param+0x105..0x108]` 連番フィールド群との併用。

## §3 残点 **V-413-1..3**
1. **【中】`FUN_00467240`/`FUN_00467320` の identity（＝`[+0xE]` 書手）→ そこから V-412-1 の "level" 語を確定** — machine を正オフセット（`66 C7 41 0E xx xx`）で decode し、decompiled 注記と setter ファミリ（+8/+0xA/+0xE の3 setter 構造）を接続。
2. 【低】未注記2 fn（`FUN_00511570`／`0x523F40`）の identity と `FUN_00519F50` の `case 0x0B+` 経路。
3. 【低】`Player[+0x24]` の vptr 設定元（型別初期化 fn）＝V-412-3 継続。

## §4 検証・自己批評
- probe: **v413_1..v413_4 の4本全 EXIT=0**（V-412-1/3 は確定に至らず＝中間裁定）。
- **決定打の作り**: (i) setter の **E8 caller 完全一致ペア**（4+4同）で SET 元を一撃特定 → (ii) dispatcher の decompiled switch 実読で index→setter 対応を確定 → (iii) `[+0xE]` 書手 census（kind 分け＋context decode）で setter 同族を隔離 → (iv) ctor の vptr store 否定で V-412-3 の残置確認。
- 妥当性チェック: ①dispatcher case9/10 ⇔ `FUN_00467290/004672B0` ⇔ SET 元4fn — ②`FUN_005222D0` の2 setter call ⇔ dispatcher — ③`[+0xE]` 書手が0x4672xx 同族 ⇔ setter ファミリ構造 — **三方向矛盾なし**。
- docs との不整合: **なし**（V-412-1/3 は [未確定] 維持で記録）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **dispatcher（switch による setter 群）を発見すると「SET 元 fn 群」が E8 caller の完全一致ペアで即座に構造化される** — pairs（4+4 が同一）は dispatcher の存在証明になる。
  2. **`[reg+disp8]` 書手 census は imm/reg を kind 分けし、毎回 context decode（capstone 開始位置調整）で境界ズレを棄却** — `mov word` が dword 表示になる例あり。
  3. **未注記 fn（Ghidra decompiled 未収録）は machine の cmp ゲート（`cmp [esi+0xc],7`）まで含めて既知 pattern（v411 bVar18）と突合**して役割を補完。
