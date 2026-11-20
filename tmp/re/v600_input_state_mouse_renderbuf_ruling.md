# v600 — V-599-1/2/3 クローズ（低優先バンドル）: 入力状態セレクタの意味・マウスボタン名表・描画バッファ寸法の供給元（ai-todo #96/#164 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u13_1〜u13_7**・全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v599_u10_u11_u12_residuals_ruling.md`（V-599-1/2/3 の起票元）
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-599-1** `0x402FC0`（slot≥2）の状態ビット | **[確定(命令)]** | `0x402FC0(slot, keybyte, state)` は **`ecx = input + slot*0xA0`** を基準に、**`state`（0..3）で 4 分岐**する。`0x60`＝現在状態バイト／`0xB0`＝直前状態バイト（`0xA0` stride の「現在/直前」対）。**state の意味は `0x4032D0`（slot==0）と同一**: **0=現在押下（down, bit7）／1=押下エッジ（pressed）／2=離上エッジ（released）／3=変化（changed, 現在 XOR 直前の bit7）**。 |
| **V-599-2** マウスボタンの実体 | **[部分確定]** | マウスボタン名は `0x9F54D0` が `0x419A70` で登録する（格納先 `0xB39554..0xB39570`・9 個）: **`L-Button`(sMouseLeftButton)／`R-Button`(sMouseRightButton)／`Wheel`(sMouseMiddleButton)／`Button 3..8`(sMouseButton3..8)**。DirectInput コールバック **`0x403DD0`** がデバイス状態を **`[input+0x530 + devidx*0x244]`**（`0x91` dword＝`0x244` B）へコピーする。擬似アクション `0x1D`/`0x1E`/`0x1F` は入力オブジェクトの**マウスボタン状態バイト対**（`0x18f5`/`0x19f5`・`0x191d`/`0x1a1d`・`0x19ab`/`0x1aab`）を読む＝**3 つのマウスボタン枠**。左/右/中の厳密対応は**デバイス状態ブロック内の絶対オフセット**に依存し、静的には本裁定で**未ピン留め**（実測=動的領域）。→ **実装は「いずれかのマウスボタン＝スキップ」で等価**（v598 の「タップでスキップ」を裏付け）。 |
| **V-599-3** `0xB350CC`/`0xB350D0` の供給元 | **[確定(命令)]** | 両者は **`.data` の描画バッファ寸法（幅/高）** で、**`.text` 内の読み書きは `0x497E70` の 4 箇所のみ**（`0x497EA6`/`0x497EB2`/`0x497ECA`/`0x497ECF`/`0x497ED5`/`0x497EDB`/`0x497F47`/`0x497F4D`）。**直接ストア（`mov [g],reg`）は 0 件** → **`0x497E70`（現在の描画バッファから iSize W/H:Display を同期する関数）自身が設定元**。デバイス singleton `[0xB350D8]` は `0x497B20`（レンダラ破棄・警告付き）で `0` にクリアされる。 |

**移植への指針**: スキップ入力は「**マウスボタン（タップ）または Space**」で等価（V-599-2 の部分確定で十分）。`0x402FC0` の状態セレクタは移植の入力ラッパで「down/pressed/released/changed」の 4 状態を提供すればよい。

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-599-1 `0x402FC0`（状態セレクタ）

```
0x402FC0(slot, keybyte, state): ecx = input + slot*0xA0   ; lea eax,[eax+eax*4]; shl eax,5
  402FD3 cmp edx,3 ; ja → return 0
  402FD8 jmp [edx*4 + 0x403054]     ; state 0..3
  state 0 (0x402FDF): test byte [ecx + keybyte + 0x60], 0x80   ; 現在押下
  state 1 (0x402FF5): [0xb0] up & [0x60] down                  ; 押下エッジ
  state 2 (0x403015): [0x60] up & [0xb0] down                  ; 離上エッジ
  state 3 (0x403035): xor [0xb0],[0x60] ; bit7                 ; 変化
```
→ **`0x60`/`0xB0` は `0xA0` stride の「現在/直前」対**。`0x4032D0`（`0x18f4`/`0x19f4` 対）と同型。

### 2.2 V-599-2 マウスボタン名表（`0x9F54D0`）

```
0x9F5512 push 'L-Button' ; push 'sMouseLeftButton' ; call 0x419A70 → [0xB39554]
0x9F554D push 'R-Button' ; push 'sMouseRightButton' → [0xB39558]
0x9F5585 push 'Wheel'    ; push 'sMouseMiddleButton' → [0xB3955C]
0x9F55BD.. 'Button 3'..'Button 8' / sMouseButton3..8 → [0xB39560..0xB39570]
```
DirectInput 状態コピー:
```
0x403DD0 (DI callback, dwOfs 0x14/0x15): 
  403DF2 imul eax,[ebx+0x18f0],0x244 ; lea edi,[eax+ebx+0x530]
  403E0F rep movsd (0x91 dwords = 0x244 B)  ; ★ デバイス状態を [input+0x530+idx*0x244] へ
```
擬似アクション `0x1D`/`0x1E`/`0x1F`（`0x403520` tail）:
```
4035A8 (0x1D): test [edi+0x19f5],0x80 ; test [edi+0x18f5],0x80
403594 (0x1E): test [edi+0x1a1d],0x80 ; test [edi+0x191d],0x80
403580 (0x1F): test [edi+0x1aab],0x80 ; test [edi+0x19ab],0x80
```
→ 各 2 バイト対（現在/直前）。

### 2.3 V-599-3 描画バッファ寸法

```
0x497E70:
  497EC8 fild [0xb350d0]     ; 高
  497ECE mov eax,[0xb350cc]  ; 幅
  497EDF mov [0xb06c5c],eax  ; iSize W:Display = 幅
  497EE4 mov [0xb06c64],ecx  ; iSize H:Display = 高
  497EF0 fcomp [0xa31c70](=0.75) ; 497EFD [0xb350d7]=1 (widescreen)
0x497B20: 497BD4 mov [0xb350d8],0  ; デバイス singleton クリア
```
`0xB350CC`/`0xB350D0` は `.data`（VA `[0xB02000,0xBABC1C)`）に属し、`.text` の参照は `0x497E70` のみ。

---

## 3. 残点（更新後）

- **V-599-2【低】**（継続）: 擬似アクション `0x1D`/`0x1E`/`0x1F` とマウス左/右/中の厳密対応（デバイス状態ブロック内オフセット依存＝動的領域）。**移植は「いずれかのマウスボタン＝スキップ」で等価**のため影響なし。
- **V-598-4【低】**（継続）: ムービー実音量（動的確認・静的 RE 範囲外）。
- **V-600-1【低】**: `0x403DD0` の DI callback が扱う `dwOfs 0x14/0x15`（デバイス種別）の厳密名（キーボード/マウス/ジョイスティック）は未特定。

---

## 4. 検証

- probe: **u13_1〜u13_7（7本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x402FC0`（`0x60`/`0xB0` 対・jump table `0x403054`）／`0x403DD0`（DI callback `0x244` stride）／`0x9F54D0`（マウス名表 `0xB39554..0xB39570`）／`0x403520` tail（`0x18f5`/`0x19f5`/`0x191d`/`0x1a1d`/`0x19ab`/`0x1aab`）／`0x497E70`（`[0xB350CC]`/`[0xB350D0]` の唯一の参照）／`0x497B20`（`[0xB350D8]=0`）。
- セクション確認: `0xB350CC`/`0xB350D0`/`0xB350D8` は **`.data`**（`imagebase=0x400000`・`.data` VA `[0xB02000,0xBABC1C)`）。
