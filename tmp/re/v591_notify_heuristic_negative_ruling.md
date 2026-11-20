# v591 — V-591-1 の negative: `0x5821F0` 内 `0x5827D0` も notify ディスパッチではなく（vtable[3] 値の受け渡し）・`0xFA8` 読取は layout 用途（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v591_1**・1本 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v590_id98_loop_survey_ruling.md`（V-591-1 の起票元）
判定: `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| `0x5821F0` 内 `0x5827D0` は notify ディスパッチか | **[確定(命令)（negative）]** | **否**。実体は **`mov eax,[ecx+0xc]; push eax; lea ecx,[esp+0x78]; call 0x441920`**（`ecx=[0xB333A0]`）＝**vtable[3] の値を取り出して別関数へ渡す**処理で、**id を push して `call reg2` する notify 形式ではない**（v590 の判定ヒューリスティックが再び false positive）。 |
| `0x5821F0` の `0xFA8` 読取（11箇所）の用途 | **[確定(命令)]** | すべて **`add edi, <X>; call 0x588BD0(0xFA8)`** の形（`X` = 0x14/0x1c/0x18/0xc/0x10/8/0x20/0x24/0x28）＝**タイル配列を走査して各タイルの id trait を読む layout/レイアウト処理**（`edi` はタイルポインタのオフセット加算）。**notify 送信ではない**。 |
| id98 の発生源 | **[未確定]（打切り判断）** | **静的コード上「id98 を RaceSexMenu へ送る」サイトは存在しない**（v589 で確定）＋ 本裁定で「`0x5821F0` 経由」仮説も否定 → **id98 は完全に実行時依存**（UI 要素のクリック→親メニュー vtable[3] への id push が、静的に特定できない汎用ループ/コールバック経由）。**静的 RE では確定不能** → **残点 V-592-1 として「動的解析（デバッガ/トレース）または `0x579970` メニューマネージャ tick の完全 decode」へ引き継ぎ**（優先度は下げる）。 |
| 本レーンで確定した副次事実 | **[確定(命令)]** | `0x5821F0`（frame 更新）は**タイル配列の id trait（0xFA8）を読んで layout を更新する関数**で、notify 送信元ではない（`0x5827D0` の vtable[3] 読取は別用途）。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
(a) 0x5827D0（"notify" と誤判定された箇所）:
      5827C2  mov ecx, [0xB333A0]
      5827D0  mov eax, [ecx + 0xc]        ; vtable[3] の値
      5827D3  push eax
      5827D4  lea ecx, [esp + 0x78]
      5827E0  call 0x441920               ; ← notify ではなく値の受け渡し
    → 真の notify 形式（push <id>; mov ecx,obj; call reg2）ではない。

(b) 0x5821F0 の 0xFA8 読取（11箇所）:
      58245A  push 0xFA8 ; add edi,0x14 ; call 0x588BD0
      5824B5  push 0xFA8 ; add edi,0x1c ; call 0x588BD0
      5824FA  push 0xFA8 ; add edi,0x18 ; call 0x588BD0
      58254D  push 0xFA8 ; call 0x588B50
      582613  push 0xFA8 ; add edi,0xc  ; call 0x588BD0
      582696  push 0xFA8 ; add edi,0x14 ; call 0x588BD0
      582718  push 0xFA8 ; add edi,0x10 ; call 0x588BD0
      58277E  push 0xFA8 ; add edi,8   ; call 0x588BD0
      583415  push 0xFA8 ; add edi,0x20 ; call 0x588BD0
      58344B  push 0xFA8 ; add edi,0x24 ; call 0x588BD0
      583489  push 0xFA8 ; add edi,0x28 ; call 0x588BD0
    → タイル配列（edi を +X で走査）の id trait 読取＝layout 処理。
```

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-592-1** | **id98 の発生源＝動的解析領域**: 静的コードに送信サイトが存在しない（v589/v590/v591 で3仮説否定）。確定には**実行時トレース（`0x5CC200` の `[ebp+8]` に 0x62 が入る瞬間のバックトレース）**または `0x579970`（メニューマネージャ tick）の完全 decode が必要。**静的 RE の限界** | 低（実装は v570 で act 確定済みのため、発生源特定は参考情報） |
| V-592-2 | `0x441920`（`0x5827D0` の callee）の identity＝vtable[3] 値を受け取る関数 | 低 |

---

## 4. 手法メモ（再発型・重要）

**notify ディスパッチの静的判定は、`mov reg2,[reg+0xc]` だけでは不十分**（v590/v591 で false positive 2件）。正しい判定条件は:
1. `mov reg2, [reg + 0xc]`（vtable[3] のロード）
2. 直後に **`push <arg2>` と `push <id>` が連続**（2つの push）
3. `mov ecx, <obj>`（this セット）
4. `call reg2`（**レジスタ経由**。`call <direct>` は除外）

→ **「2連 push + mov ecx + call reg」の4条件を必須**にしないと、`vtable[3]` の一般読取（値を push して別関数へ渡す・引数として使う）を notify と誤認する。以後の probe はこの4条件で判定する。

---

## 5. probe 一覧

`v591_1`（`0x5827D0` 文脈＝`mov eax,[ecx+0xc]; push eax; call 0x441920` で notify 否定／`0x5821F0` の `0xFA8` 読取11箇所＝`add edi,X; call 0x588BD0` の layout 処理／notify 対象 vtable 読取）。1本 EXIT=0。
