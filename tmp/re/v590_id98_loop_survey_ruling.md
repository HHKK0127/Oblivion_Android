# v590 — V-589-1 追加調査: `0xFA8`（id trait）読取×notify の4 head と `push 0x40C`×notify の false positive 確定（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v590_1〜v590_4**・4本 全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v589_laneB_notify_dispatch_ruling.md`（V-589-1 の起票元）
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| notify の id に `0xFA8`（tile id trait）を使う head はどれか | **[確定(命令)]（絞込み）** | `.text` の `push 0xFA8` は **126 箇所**（UI head に多数）。そのうち **同 head 内に notify ディスパッチ（2段形式）を持つのは4 head のみ**: **`0x579970`（メニューマネージャ tick）／`0x581390`／`0x5821F0`（frame 更新）／`0x5894F0`**。 |
| `0x579970` の notify は id98 の発生源か | **[確定(命令)（negative）]** | `0x579970` の notify（`0x579A3F`）の id は **`eax`（`0xFA8` の float→int 変換値ではない別の値）** で、**呼出先は `edx`（インスタンス固有）**。RaceSexMenu(0x40C) 固有の送信ではない。 |
| `push 0x40C`（RaceSexMenu menuid）＋notify を持つ head は id98 を送るか | **[確定(命令)（negative）]** | 該当 head は **`0x474070`／`0x474530` の2件**だが、両者とも **`TESAnimGroup` 系（アニメーションシーケンス名）** で、`push 0x40C` は **menuid ではなく別用途の定数**（`0x474326`）。**notify 判定（`mov r2,[r+0xc]; call r2`）は「id を push せず `call r2` するだけ」の vtable[3] 読取と衝突する false positive** → **真の menuid 通知ではない**。 |
| id98 の発生源 | **[未確定]（絞込み・継続）** | **静的コードに「id98 を RaceSexMenu へ送る」サイトは存在しない**（v589 で確定）→ **実行時に決まる id**。本裁定で「`0xFA8` 経由」と「`push 0x40C` 経由」の2仮説を**いずれも否定**（前者は送信先が RaceSex 固有でない・後者は false positive）→ **残る経路＝「要素クリック→親メニュー vtable[3]」の中央ループ本体**（`0x5821F0` 内の notify `0x5827D0` を起点に、id を計算する箇所の完全 decode）＝**V-591-1 として継承**。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

```
(a) push 0xFA8 (id trait) 総数 126（UI head に多数）。
    0xFA8 読取と notify ディスパッチを両方持つ head は4つ:
      0x579970 (0xFA8×5, notify@0x579A3F id=eax)
      0x581390 (0xFA8×2, notify@0x581777)
      0x5821F0 (0xFA8×11, notify@0x5827D0 id=eax)
      0x5894F0 (0xFA8×1, notify@0x589588)

(b) 0x579970 notify（0x579A3F）:
      ... push eax ; call edx        ; id=eax、呼出先=edx（インスタンス固有）
    → 0xFA8 値の直接送信ではなく、別の計算値。

(c) push 0x40C を持つ head のうち notify を含むのは 0x474070 / 0x474530 のみ。
    0x474070（head）は `"Animation sequence '%s' not found in TESAnimGroup::G"`（0xA3C970）
    を参照＝TESAnimGroup 系。`push 0x40C` @0x474326 は menuid ではない。
    notify 判定（`mov eax,[edx+0xc]; mov ecx,esi; call eax` @0x474180）は
    **id を push しない vtable[3] 読取**＝false positive。
```

* **教訓**: `mov r2,[r+0xc]; call r2` を「notify ディスパッチ」と判定するヒューリスティックは、**「直前に push <id> があるか」を必須条件にしないと vtable[3] の一般読取と衝突する**（本裁定で false positive 2件を検出）。

---

## 3. 残点（引き継ぎ）

| ID | 内容 | 重要度 |
|---|---|---|
| **V-591-1** | **`0x5821F0` 内 notify `0x5827D0` の id 計算の完全 decode**（`0xFA8`×11 と notify を併せ持つ唯一の「frame 更新」関数）＋「要素→親メニュー vtable[3]」の中央ループ本体。**id98 の実行時発生源の最終候補** | **中**（V-589-1 を再定義して継承） |
| V-591-2 | `0x581390`／`0x5894F0`（`0xFA8`＋notify）の identity | 低 |
| V-591-3 | notify 判定ヒューリスティックの厳密化（直前 push 必須）を probe 標準に追加 | 低 |

---

## 4. probe 一覧

`v590_1`（候補 head の notify 走査・UI 変数 id notify 2009 サイト）／`v590_2`（UI head 変数 id notify 2009 サイト・head 別集計・文脈6件）／**`v590_3`（`push 0xFA8` 126件の head 別集計・`0xFA8`＋notify の4 head 抽出・`push 0x40C`＋notify の2 head 抽出）**／`v590_4`（`0x474070`/`0x474530` の decode＝TESAnimGroup 系と false positive 確定・`0x579970` の notify）。4本 EXIT=0。
