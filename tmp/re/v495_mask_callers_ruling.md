# v495 — mask 値 census・`FUN_0045A500` callers・return carrier の裁定（V-495-1..3 完遂・旧残点 V-406-3/V-406-4 継続第25弾）

- 採番: **v495**（旧残点群消化・probe prefix **v495_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-495-1..3（v494 残点）** — 1=map node の `*node |= mask` 最終用途（caller 側 mask 値）と `FUN_0045A500` callers／2=`[node+0x28]` float と form `+0x28` の同異／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-495-1: `FUN_00452C20` caller 側の mask 値** | **[確定相当]（bit24 経路）＋[部分確定]（gated 側は動的）** | **E8 callers 7件の push 直前値を census [確定(機械)]: `0x45CF88`（`FUN_0045CEE0`）pushes＝`['eax','0x1000000','edi']` ／`0x45D0DC`（`FUN_0045D030`）同形 → **cdecl 逆順読替で `mask = 0x1000000`（**bit24**）、`param_1=edi` [確定相当]** → **bit24 設定経路を特定 [確定相当]**（v489 bit5 以外の mask の実例）。**gated 側（`FUN_0045B670` 2 sites／`FUN_0045B700` 3 sites）の mask は register（`edx`）経由で動的 [部分確定]**（**`edx` の値源は caller 上流＝残点**）。他 site: `FUN_0045B6D5` pushes `['cs','edx','edi']`（`push cs` は int3 帯 window decode の疑い＝**boundary 再検証要の制約を記録**） |
| **V-495-1: `FUN_0045A500` callers（`param_1+0x18` 書手探索）** | **[確定]（8件列挙）** | **E8 callers 8件 [確定(機械)]: `FUN_0040D4D0`（L8522）／`FUN_0041FA40`（L22089）／`FUN_004246F0`（L26599）／`FUN_00424C50`（L26947）／`FUN_00424D00`（L26991）／`FUN_0042A200`（L30474）／`FUN_00434020`（L38856）／`FUN_00438060`（L42454・v480 既知 fn）** → **bit18/byte の書手はこの8 fn 経由（各 caller の `param_1+0x18` usage を次 probe で特定）＝経路を固定 [確定]** |
| **V-495-2: `FUN_004CBA80` の return carrier（node vs form）** | **[部分確定]** | docs body（L159742..）の return 3箇所: **`L159750: return 0`／`L159757: return 0`（`param_2==0` 分岐・alloc 失敗）／`L159765: return piVar2` [確定(decompiled)]** → **戻り＝`piVar2`（本体で何を指すかは body 中間が未表示）[部分確定]** → **`[ret+0x28]`（v494 `fld`）の载体（node/form）は `piVar2` の特定待ち＝残点**（`param_2==0` で null を返す挙動は v485 docs と一致 [確定]） |
| **V-495-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v495_1）

### §2.1 mask census（v495_1a）
```
E8 callers of FUN_00452C20: 7
  0x45B6D5 (FUN_0045b670) pushes ['cs','edx','edi']            (window suspect: push cs)
  0x45B6E7 (FUN_0045b670) pushes ['edx','edi','eax','edi']     (register mask, dynamic)
  0x45B745 (FUN_0045b700) pushes ['edx','edi']
  0x45B757 (FUN_0045b700) pushes ['edx','edi','eax','edi']
  0x45B769 (FUN_0045b700) pushes ['edx','edi','eax','edi','ecx','edi']
  0x45CF88 (FUN_0045cee0) pushes ['eax','0x1000000','edi']      -> mask = 0x1000000 (bit24)
  0x45D0DC (FUN_0045d030) pushes ['eax','0x1000000','edi']      -> mask = 0x1000000 (bit24)
```

### §2.2 `FUN_0045A500` callers（v495_1b）
```
8 sites: FUN_0040D4D0 / FUN_0041FA40 / FUN_004246F0 / FUN_00424C50 /
         FUN_00424D00 / FUN_0042A200 / FUN_00434020 / FUN_00438060 (v480 known)
```

### §2.3 `FUN_004CBA80` returns（v495_1c）
```
L159750: return (int *)0x0;
L159757: return (int *)0x0;
L159765: return piVar2;        (carrier = piVar2, mid-body unread -> residual)
```

## §3 残点 **V-496-1..3**
1. 【中】**`piVar2`（`FUN_004CBA80` 戻り carrier）の特定**（body 中間 L159758..L159765）→ `[ret+0x28]` の载体（node/form）確定＋ **`FUN_0045B670` gated 側の mask 値源（`edx` の設定元）** ＋ `FUN_0045A500` callers 8件の `param_1+0x18` 書手特定。
2. 【低】**`FUN_0045CEE0`/`FUN_0045D030`（mask `0x1000000` callers）の identity**（bit24 設定の意味）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v495_1 の1本 EXIT=0**（V-495-1 mask 2系統（bit24 固定・register 動的）と `FUN_0045A500` callers 8件を確定／V-495-2 return 3箇所を確定・carrier は残点／V-495-3 引継ぎ）。
- **決定打の作り**: (i) **caller の push 直前値 census で mask（`0x1000000`）を特定**（cdecl 逆順読替）→ bit 設定の実値を獲得 → (ii) **`FUN_0045A500` callers 8件**で bit 書手探索の対象を固定 → (iii) **return 文3箇所の grep**で carrier が `piVar2` と分かり、body 中間が次の手と明確化。
- 妥当性チェック: ①`0x1000000`＝bit24⇔mask として `|=`（v494 構造）— ②`push cs`（`0x45B6D5`）⇔int3 帯 window 疑い（v474/v489 同じ精度制約）— ③`param_2==0 → return 0`⇔v485 docs と一致 — ④`FUN_00438060` callers⇔v480 既知 fn（横断整合）— ⑤8 callers⇔8 sites＝数一致 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **mask 値は caller の push 直前から読む**（cdecl 逆順で「最後の push＝param_1」「その前＝param_2」）— **固定値（`0x1000000`）と register 経由（動的）を区別**して記録。
  2. **`push cs` は int3 帯 window decode の疑い**（boundary 再検証要の制約を明記＝v474/v489 と同じ自己点検）。
  3. **bit 書手探索は「判定 fn の callers 列挙」から**（8件）→ 次 probe で各 caller の `+0x18` usage を読む経路を固定。
  4. **return 3箇所 grep が carrier を即特定**（`piVar2`）→ body 中間が残点と方向が明確化。
  5. **cross-verified fn（`FUN_00438060`＝v480 既知）が callers に現れることで系統整合**を確認。
