# v578 — V-577-1 (c) の negative 完全化: 「open 成功呼び出し」が全7系統で不存在（矛盾の完全記録）（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v578_1〜v578_4**・4本 全 EXIT=0・冒頭 Test-Path を習慣化）。
先行: `tmp/re/v577_fa2_jmp_and_ff2_complete_ruling.md`（5命題）／`tmp/re/v576_entity_mechanism_and_open_contradiction_ruling.md`
判定: `[確定(否定側)]` / `[未確定]（矛盾残存・次の一手を特定）**

---

## 1. 結論

| 系統（open `0x591C50` への到達 / a1=0xFA2 の渡し） | 判定 | 結果 |
|---|---|---|
| ① E8 direct | **[確定]** | `0x591C76` の1件のみ（open 内部）。`0x587D20` も open 内1件のみ（v576/577 済）。 |
| ② E9 tail-jmp | **[確定]** | 0件。 |
| ③ `ff /2` 1段（全 modrm+SIB+disp） | **[確定]** | 282件・**UI=0件**・直前 push の **即値は 0x0C..0x32/0x104 系のみで 0xFA2 なし・`push [mem]` も0件**。 |
| ④ 2段形式 `mov r,[r+0x14]; call r` | **[確定]** | **408対・UI 多数（`0x53F4C0`/`0x5A3160`/`0x5BB880`/`0x58BEE0` 等）**だが、mov→call 間の push hist に **`0xFA2` 0件・`[mem]` push 0件**（immediates は 0x0C..0x31 系と `0x1109` のみ）。 |
| ⑤ `push 0xFA2`（byte 列） | **[確定]** | `.text` 出現3件＝**jmp オペランド／entity 登録／open 内 cmp**（v577 済）。呼び出し側の push なし。 |
| ⑥ `mov reg,0xFA2` | **[確定]** | 0件（v576 済）。 |
| ⑦ **`0x587D20` の data pointer（dword `20 7d 58 00`）** | **[確定]** | **ファイル全体で0件** → `ff 15` 経由・`push 0x587D20; ret`（push imm も同一 byte 列で0件）も**不存在**。 |
| **総合** | **[確定(否定側)]** | **「open に a1=0xFA2 を渡して到達し、`0x587D20` switch を起動する場所」は全7系統に存在しない**。 |

**残る矛盾（5命題・v577 のまま維持）**: ①open 成功＝`a1==0xFA2` 固定 ②`0xFA2` 即値は2件のみ ③`0x587D20` は open 内1件のみ（④⑦で死角なし） ④open への成功呼び出しが全系統で不存在 ⑤**それでもメニュー群は生成される**（EXE が動作する）。

---

## 2. 次の一手（v579 の本命）[特定済み]

**`0x587B20`（open 内 `0x591C6C: call 0x587B20` の callee）の中身を decode する**:
* open 内の並び: `f2i(a2) → push id; push 1; call 0x587B20 → mov ecx,eax → call 0x587D20`（v574）— **`0x587B20(1, id)` が「id スペースの取得 or 内部で type を解決する」可能性**、または **`0x587D20` への到達が実はここの直後以外にもある**（＝`0x587B20` が独自に生成 switch を回す）。
* あわせて: **v547 記載の `open(type:int, time:float, ?:int)` の引数契約を実測確定**（a1=type=0xFA2 は保証、**a2=float の実体**（id の float 表現か time か）を f2i の前後で確定 → **float id のビット定数（例 1004.0f 等）を `push` していないか byte 列走査**）。

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-579-1** | **矛盾解消の最終手**: `0x587B20` の decode（独自生成経路 or id 解決）＋ open の a2(float) 実体と float-id の byte 列走査 | **中**（V-577-1/V-576-1 を再定義して継承） |
| V-579-2 | 2段形式 408対のうち UI heads（`0x53F4C0`/`0x5A3160`/`0x59ED30` 等）がどの vtable slot を叩くのかの分類（slot5=open 以外） | 低 |

---

## 4. 教訓（再発型）

1. **indirect call は「1段（`ff /2`）」「2段（`mov r,[r+disp]; call r`）」の両形式を走査しないと UI の大半を取りこぼす**（1段 UI=0 件 vs 2段 UI 多数＝v577/v578 で確認）。
2. **push の種別（imm / reg / [mem]）を機械分類して histogram 化すると、即値不在の negative が一目で確定**（0FA2=0・[mem]=0 が1表で完結）。
3. **到達経路の系統数（E8/E9/1段/2段/push-imm/mov-imm/data-pointer/push-ret）を列挙してから「不存在」と断じる**（本裁定の7系統がそう）→ **negative も検証済み [確定(否定側)]** として裁定価値を持つ。
4. probe 冒頭 `assert Test-Path(v307_common)` を習慣化（2回目の消失対策・memory 教訓の運用化）。

---

## 5. probe 一覧

`v578_1`（282件の直前 push 即値 histogram＝0xFA2 0件・0x0C..0x32 系）／`v578_2`（push 種別分類＝**[mem] push 0件**）／**`v578_3`（2段形式408対の push histogram＝`0xFA2` 0件・UI 多数 head 一覧）**／`v578_4`（**dword `0x587D20` をファイル全体走査＝0件＝data-pointer/push-ret 経路不存在**）。全 4 本 EXIT=0（各 probe 冒頭に Test-Path assert）。
