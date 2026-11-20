# v601 — V-600-1 クローズ: DirectInput デバイス種別（GUID と EnumDevices コールバックの devtype）の同定（ai-todo #96/#164 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u14_1〜u14_2**・全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v600_input_state_mouse_renderbuf_ruling.md`（V-600-1 の起票元）
判定: `[確定(命令)]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| **V-600-1** DI callback `0x403DD0` が扱うデバイス種別 | **[確定(命令)]** | **`0x403DD0` は `IDirectInput8::EnumDevices` のコールバック**で、`DIDEVICEINSTANCE.dwDevType`（オフセット `+0x24`）の下位バイトを **`0x14`／`0x15`** と比較する＝**`DI8DEVTYPE_JOYSTICK`(0x14)／`DI8DEVTYPE_GAMEPAD`(0x15)`＝ジョイスティック/ゲームパッド**。**キーボード/マウスはこの callback では扱わない**（`CreateDevice` で GUID を直接指定して生成）。 |
| デバイス GUID の同定 | **[確定(命令)]** | `0x404150`（入力デバイス初期化）が `IDirectInput8::CreateDevice` に渡す GUID: **`0xA78EA4`＝`{6F1D2B61-D5A0-11CF-BFC7-444553540000}`＝`GUID_SysKeyboard`**（キーボード）／**`0xA78EB4`＝`{6F1D2B60-D5A0-11CF-BFC7-444553540000}`＝`GUID_SysMouse`**（マウス）。`0xA78F54`＝`{BF798031-483A-4DA2-AA99-5D64ED369700}`＝`IID_IDirectInput8`（`DirectInput8Create` の引数）。 |
| 入力デバイス初期化の全体 | **[確定(命令)]** | `0x404150`: `DirectInput8Create(0x800, IID_IDirectInput8, &[esi+4], 0)` → `EnumDevices(0, 0x403DD0, esi, DIEDFL_ATTACHEDONLY)`（**ジョイスティック/ゲームパッド**）→ `CreateDevice(GUID_SysKeyboard, &[esi+0x28], 0)` → `CreateDevice(GUID_SysMouse, &[esi+0x2c], 0)`。 |

**移植への指針**: 原版のキーボード/マウスは **DirectInput のシステムデバイス（`GUID_SysKeyboard`/`GUID_SysMouse`）を直接生成**し、ゲームパッドは `EnumDevices` で列挙する。移植（Android のタッチ/ゲームパッド）では、マウス相当＝**タッチ**、キーボード相当＝**Space** を割り当てれば v598 のスキップ仕様と一致する。

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 入力デバイス初期化（`0x404150`）

```
404164 push 0xa78f54              ; IID_IDirectInput8
404169 push 0x800
40416E push eax ; 404177 call 0x6bb260(DirectInput8Create)
40418D mov edx,[ecx+0x10] ; 404193 push 0x403dd0 ; 40419A call edx   ; EnumDevices(cb=0x403DD0)
4041B5 push 0xa78ea4              ; GUID_SysKeyboard
4041BB mov eax,[ecx+0xc] ; 4041BE call eax                          ; CreateDevice(→[esi+0x28])
4042C7 push 0xa78eb4              ; GUID_SysMouse
4042CD mov eax,[ecx+0xc] ; 4042D0 call eax                          ; CreateDevice(→[esi+0x2c])
```
`IDirectInput8` vtable: `+0xc`＝`CreateDevice`／`+0x10`＝`EnumDevices`。

### 2.2 EnumDevices コールバック（`0x403DD0`）

```
403DD0 mov edx,[esp+4]        ; DIDEVICEINSTANCE*
403DD4 mov al,[edx+0x24]      ; ★ dwDevType の下位バイト
403DDA cmp al,0x14 ; 403DDC je 0x403DED   ; DI8DEVTYPE_JOYSTICK
403DDE cmp al,0x15 ; 403DE0 je 0x403DED   ; DI8DEVTYPE_GAMEPAD
403DE2 mov eax,1 ; ret 8                   ; それ以外は無視（DIENUM_CONTINUE）
403DED...  デバイス設定（SetDataFormat/SetCooperativeLevel 等）＋状態バッファ初期化
```
`DI8DEVTYPE_*`（`dinput.h`）: `MOUSE=0x12`／`KEYBOARD=0x13`／**`JOYSTICK=0x14`／`GAMEPAD=0x15`**。→ **callback はジョイスティック/ゲームパッドのみ**。

### 2.3 GUID 実測（`.rdata`）

```
0xA78F54 = 30 80 79 bf 3a 48 a2 4d aa 99 5d 64 ed 36 97 00  = IID_IDirectInput8  {BF798031-483A-4DA2-AA99-5D64ED369700}
0xA78EA4 = 61 2b 1d 6f a0 d5 cf 11 bf c7 44 45 53 54 00 00  = GUID_SysKeyboard   {6F1D2B61-D5A0-11CF-BFC7-444553540000}
0xA78EB4 = 60 2b 1d 6f a0 d5 cf 11 bf c7 44 45 53 54 00 00  = GUID_SysMouse      {6F1D2B60-D5A0-11CF-BFC7-444553540000}
```

---

## 3. 残点（更新後）

- **V-599-2【低】**（継続）: 擬似アクション `0x1D`/`0x1E`/`0x1F` とマウス左/右/中の厳密対応（デバイス状態ブロック内オフセット依存＝動的領域）。**移植は「いずれかのマウスボタン＝スキップ」で等価**のため影響なし。
- **V-598-4【低】**（継続）: ムービー実音量（動的確認・静的 RE 範囲外）。
- **V-601-1【低】**: キーボード/マウスの `SetDataFormat` データフォーマット配列（`0xA78E5C`/`0xA78A4C` の `DIDATAFORMAT`）の厳密なキー対応表は未列挙（`dwDataSize`＝`0x100`/`0x14`）。

---

## 4. 検証

- probe: **u14_1〜u14_2（2本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x404150`（`DirectInput8Create`／`EnumDevices(cb=0x403DD0)`／`CreateDevice(GUID_SysKeyboard)`／`CreateDevice(GUID_SysMouse)`）／`0x403DD0`（`[edx+0x24]`＝`dwDevType` を `0x14`/`0x15` 比較）／GUID 実測（`0xA78F54`/`0xA78EA4`/`0xA78EB4`）。
- 照合: `GUID_SysKeyboard`/`GUID_SysMouse`/`IID_IDirectInput8` の既知バイト列と一致。
