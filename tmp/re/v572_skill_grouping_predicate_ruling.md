# v572 — V-569-1 クローズ: Skills タブのグルーピング判定 `0x51C090` の実体（管輪スキル7個判定＋charactergen 既定クラス除外）（ai-todo #157 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v572_1〜v572_3**・3本 全 EXIT=0）。
先行: `tmp/re/v569_p3_p5_rows_ruling.md`（V-569-1 の起票元・§2.1 の P）
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| P = `0x51C090(0x5E3320([0xB333C4]), form)` は何か | **[確定(命令)]** | **「form（AV 索引 12..32）が this の 7 要素 dword 配列 `this+0x44` に含まれるか」の線形探索**＝**クラスの管輪スキル7個判定**。`this` は `0x5E3320(player)` の戻り（§3）。**charactergen 既定クラス（GMST `iClassCharactergenClass` = int `0x230E6`＝クラス formID）と `this+0xC` が一致するときは即 false**（ループ抜ける）。 |
| それが v569 の p3 グルーピングをどう説明するか | **[確定(命令)]** | **管輪スキル（P=真）が先頭グループ（listindex 0..C-1）**、**番号 C が空白**、**非管輪（P=偽）が C+1 から**。C＝**管輪スキル数**（通常 7 → 空白番号 7・計 22 スロット＝v569 §2.1 の「最大 21＝22 スロット」と整合）。**charactergen 既定クラス時（P 常に false）は C=0 → グルーピング無し（0..20 連番・空白なし）**。 |
| `builder+0x30.user3 = C` の意味 | **[確定(命令)]** | **管輪スキル数の書き込み**（v569 §2.1 前パスの `SetFloat([ebx+0x30], 0xFB1, C)`）。同じ P を使う `0x5DA1A0` の -1 経路（v569 §2.3）も同一カウント。 |
| 判定はどこで使われるか | **[確定(命令)]** | `0x51C090` の caller は **8 本**: p3 行ループ `0x5DC630`×2（前パス・行毎）／`0x5DA1A0`×2（AV フィラー）／`0x50D020`（`0x6648D0` 経由のリスト系）／`0x663C50`／`0x666BB0`／`0x668070`（0x66xxxx 系＝スキル関連システム3箇所）。**管輪スキル判定は EXE 共通の helper**。 |

---

## 2. `0x51C090` の完全実体 [確定(命令)]

```
0x51C090(this, arg):                      ; ret 4 = 1 stack arg
  edx = [this + 0xC]
  al  = 0
  if (edx == [0xB37D00]) return false     ; 0xB37D00 = GMST iClassCharactergenClass (int 0x230E6)
  esi = arg
  ecx = &this->member[+0x44]
  for (edx = 0; edx < 7; edx++, ecx += 4):
      if (al) break
      if ([ecx] == esi) al = 1            ; 7 個の dword に arg と一致があれば真
  return al
```

* **`[0xB37D00]` の同定 [確定(命令)]**: 静的登録ブロック `0x9EDEA0` の逐語 = `push 0x230e6; push 'iClassCharactergenClass'; mov ecx,0xB37D00; call 0x419A70` → **int GMST `iClassCharactergenClass` の値 `0x230E6`(143,078)＝charactergen 用クラス formID**。（隣接 `iClassNightblade`=value `0x23C08` は**別のグローバル `0xB37CF8`** — probe v572_2 の自動逆引きは隣接混入で誤っていた → **逐語ダンプで是正**した教訓。）
* **`arg`**: v569 で `0x565BE0(2, skillIdx)` が返す **AV 索引 12..32**（21 スキル連番）と整合。
* **`this`**: `0x5E3320([0xB333C4])`。`0x5E3320` のゲート `0x4D7F80` = `vfunc+0x190` が真 かつ `vfunc+0x170` 戻りの **`[form+4] == 0x23`**（formType バイト）→ `[vfunc+0x170(this) + 0x104]` を返す（型の厳密な特定は残点 V-572-3）。

---

## 3. p3 Skills グルーピングの完全説明（v569 との接続）[確定(命令)]

```
[前パス 0x5DC630] C = count of { skillAV in 管輪スキル7個 }   （charactergen クラス時 C=0）
                  SetFloat(builder+0x30, user3, C)
                  C>0 → 偽群開始番号 = C+1
[行ループ]        P=true  → listindex = counterA（0..）
                  P=false → listindex = counterB（C+1..）※C=0 の時は 0..
```

* **表示結果**: 管輪スキルが先頭に集まり、**間に1つ空白行**、続いて非管輪スキル（管輪7個なら listindex 0-6／空白7／8-21）。
* **charactergen 既定クラス（formID 0x230E6）が選択されている間はグルーピング無効**（P 常に false → C=0 → 0..20 の連番・空白なし）。＝**キャラ生成中はクラス未確定なので並び替えをしない**という挙動 [確定(命令)]。
* 移植示唆: **「クラス選択前＝素のスキル順／クラス確定後＝管輪7個が先頭」**が原版の実挙動（v569 §6 の「分類グルーピング」の正体）。管輪スキルは `TESClass+0x44` の7個（クラス定義）。

---

## 4. 他の caller（8 本）[確定(命令)＋用途は [部分確定]]

| caller | head | 直前文脈 | 用途 |
|---|---|---|---|
| `0x5DC6AA` | `0x5DC630`（前パス） | 0x5E3320 経由 | **C 計数**（§3） |
| `0x5DC7EC` | `0x5DC630`（行毎） | 同上 | **listindex 2 群振り分け** |
| `0x5DA1EC` | `0x5DA1A0`（-1 パス） | 同上 | AV フィラーの同カウント（v569 §2.3） |
| `0x5DA85C` | `0x5DA1A0`（0x5DA7B0 ループ） | 同上 | 行ループ内の判定 |
| `0x50D1AC` | `0x50D020` | `0x6648D0` → `0x5E3320` → 判定 → `je` | スキル一覧系 [部分確定] |
| `0x663CE1` | `0x663C50` | `0x5E3320` → `0x6ECC80` → 比較 → 判定 | スキル関連システム [部分確定] |
| `0x666C83` | `0x666BB0` | （同型） | 同上 [部分確定] |
| `0x6681DC` | `0x668070` | （同型） | 同上 [部分確定] |

---

## 5. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-572-1 | `0x663C50`/`0x666BB0`/`0x668070`（管輪スキル判定を使う 0x66xxxx 系3システム）の用途同定 | 低 |
| V-572-2 | `this+0x44` の 7 要素配列が `TESClass.majorSkills` のどの並び（AV 順/定義順）か、`this+0xC` が何のメンバか | 低 |
| V-572-3 | `0x5E3320`/`0x4D7F80` の厳密な型（`[form+4]==0x23` の formType 名、`[vfunc+0x170(this)+0x104]` の先） | 低 |

---

## 6. probe 一覧

`v572_1`（`0x51C090` 全 decode 9 命令・`0x5E3320` head・caller 8 本の E8 走査）／`v572_2`（`[0xB37D00]` 自動逆引き〔誤〕・`0x4D7F80` head・caller 4 箇所の文脈）／`v572_3`（**静的登録ブロックの逐語ダンプ＝`iClassCharactergenClass`=int `0x230E6` を確定**・`iClassNightblade`=`0x23C08` は別グローバル `0xB37CF8`）。全 3 本 EXIT=0。
