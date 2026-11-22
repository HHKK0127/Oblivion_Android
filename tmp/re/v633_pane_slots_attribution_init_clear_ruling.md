# v633 — pane slot 配列・item 帰属の確定（入れ子なし）・Init の `+0x94` クリア・名前→id 固定

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u99_1 / u100_1**（EXIT=0）。
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. pane slot 配列 = `menu+0x40 + i*4`（査読 §1-1）[確定(命令)]

各 pane 呼出の戻り（eax）の store 先:

| pane 呼出 | store 先 | pane 呼出 | store 先 |
|---|---|---|---|
| `0x5CA448` Main | `[esi+0x40]` | `0x5CB02D` Eyes | `[esi+0x68]` |
| `0x5CA5A4` Race | `[esi+0x44]` | `0x5CB0EA` Forehead | `[esi+0x6c]` |
| `0x5CA62D` Hair | `[esi+0x48]` | `0x5CB181` Jaw | `[esi+0x70]` |
| `0x5CA726` Face | `[esi+0x4c]` | `0x5CB23E` Mouth | `[esi+0x74]` |
| `0x5CA85B` Shape | `[esi+0x50]` | `0x5CB3DF` Nose | `[esi+0x78]` |
| `0x5CAAE4` Tone | `[esi+0x54]` | `0x5CB618` GeneralSkin | `[esi+0x7c]` |
| `0x5CACA1` GeneralFace | `[esi+0x58]` | `0x5CB6FB` Beard | `[esi+0x80]` |
| `0x5CAD84` Brow | `[esi+0x5c]` | `0x5CB819` EyesTex | `[esi+0x84]` |
| `0x5CAE1B` Cheeks | `[esi+0x60]` | `0x5CB937` Eyebrows | `[esi+0x88]` |
| `0x5CAEFE` Chin | `[esi+0x64]` | `0x5CBA7E` MouthLips | `[esi+0x8c]` |
| | | `0x5CBB21` NoseTex | `[esi+0x90]` |

- **21 pane は `menu+0x40..+0x90` の 21 スロットに 1 個ずつ**（v627 の pane 配列と一致）。
- `+0x40`/`+0x44`/`+0x90` は**同じスロット配列の別要素**であり、独立フィールドではない（査読 §1-4 の判定）。

## 2. item の arg1 = 「現在の pane スロット」（査読 §1）[確定(命令)]

- `[esi+0x40]` の**読みは Main region（`0x5CA484..0x5CA57C`）にのみ**、`[esi+0x44]` の読みは Race region にのみ、
  というように **各スロットは自分の region 内でのみ読まれる**。
- → **item の arg1 = `[esi+0x40+i*4]`（現在の pane）**。**v632 §2 の帰属表は正しい**。
- **「text は `+0x44`」は誤読**: 最初の text 呼出 `0x5CA60B` が **Race region** にあるため `[esi+0x44]`（=Race）を読んだだけ。
  text は 1 件で Race に帰属（表のまま）。
- → **帰属の作り直しは不要**（査読 §1 の懸念は解消）。

## 3. 入れ子は無い（査読 §2）[確定(命令)]

- **21 pane 呼出すべての arg1 = `[esi+0x28]`（window）**（`push edx; mov eax,[esi+0x28]; push eax`）。
- → **親は全 pane で window（深さ 0）**。**入れ子（深さ 1）は 0 件**。
- → **21 = 21 ページ**（`+0x44` が「別列」という説明も不要）。**pane 生成回数 = ページ数**。

## 4. `0x5CBB2F` / `0x5CBB47` / `0x5CBB70`（査読 §1-3）[確定(命令)]

```
0x5CBB21 call 0x5C4340                 ; NoseTex を生成
0x5CBB2F mov [esi+0x90], eax           ; ★ 最終 pane を +0x90 に格納
0x5CBB47 mov eax,[esi+0x90]; push eax; call 0x5C93F0   ; 末尾 slider 1
0x5CBB70 mov eax,[esi+0x90]; push eax; call 0x5C93F0   ; 末尾 slider 2
```
- **`+0x90` = 最後の pane スロット**。末尾 2 slider は NoseTex に帰属（v632 表の NoseTex=2 と一致）。

## 5. Init `0x5C4E40` の `+0x94` クリア（査読 §5）[確定(命令)]

```
0x5C4E99 mov [esi+0x28], ebx(0)        ; window = 0
0x5C4E9C mov [esi+0x2c], 0
0x5C4E9F mov [esi+0x30], 0
0x5C4EA2 mov [esi+0x3c], 0
0x5C4EA7 lea edi, [esi+0x94]
0x5C4EAD mov ecx, 0x1F4                ; ★ 500 dwords
0x5C4EB2 rep stosd [edi], eax          ; ★ +0x94..+0x864 を 0 クリア
0x5C4F3C mov [0xB3B5D4], 0xC8          ; counter -> 200
```
- **`+0x94` 領域 = 500 dwords（`0x94..0x864`）**。id 最大 429 → `+0x94+429*4 = 0x748` < `0x864` ✓。
- hair color int 配列 `0x8F0` との余裕 = `0x8F0 - 0x864 = 0x8C`（140 bytes）。
- **Init は counter を 200 に戻す** → **再 Init で id は 500 を超えない** ✓。
- Init は window(`+0x28`) 他も 0 クリア。`callers_of(0x5C4E40) = 1`（`0x58847A`）。
- → **V-632-1 の第一仮説は「Init が表を走査して破棄」で確定**（`rep stosd` によるクリア）。

## 6. 名前 → id（査読 §4）[確定(命令)]

- 21 pane 呼出の名前は**すべて `mov edx,[0xB38XXX]`（名前グローバル）**。**`lea` は無し**。
- → 名前→id は**グローバルで固定**（v625/v627 の表のとおり）。以降の表は **id 基準で一意**。

## 7. 残点（優先度改訂・査読 §3）

- **V-632-2【済】**: スロットと親子 → §1/§2/§3 で確定（入れ子なし）。
- **V-631-1【低】**: 実装単位列の充填（`race_sex_panel` 未作成）。
- **V-632-1【中】**: `+0x94` を読む 5 関数の役割（**Init の破棄が第一仮説で確定**。残りは `0x5C0440`/`0x5C3BE0`/`0x5C6390`/`0x5C6860`/`0x5C6AF0`）。
- **V-630-1【低】**: arg4 の値（帰属に影響しないため後回し）。
- **V-629-1【低】**: `player[0xE8]` の実行時裏取り。
- （継続）V-626-2 / V-626-3 / row604。

---

## 8. 検証

- probe **u99_1**（pane 直後の store 先／`+0x38..0x48`・`+0x90` の全参照／`0x5CBB20..0x5CBB80`／名前の出所）。
- probe **u100_1**（pane の arg1 源／`+0x94` レイアウト／`0x5C4E40` のクリアとリセット）。
