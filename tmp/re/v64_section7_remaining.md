# V64 — V57§7 残4点の確定報告（Oblivion.exe v1.2.0416 Steam版）

作成: 2026-10-24 / 手法: プローブハーネス（%TEMP%\opencode\v64_p*.py, capstone 5.0.7 + struct自前PE + decompiled.c索引）
既存ドキュメントは編集していない（本ファイルのみ新規）。

## 0. セルフテスト4件（ground truth）

| # | 検証 | 実測 | 結果 |
|---|---|---|---|
| 1 | bytes@0x504050 | `518b4c24188b5424` (8B) | PASS |
| 2 | bytes@0x4FB580 | `578b7908` (4B) | PASS |
| 3 | string@0xA7035C | `.\AI\CombatController.cpp` | PASS |
| 4 | opid 0x0015 impl | `0x504560`（表 0xB0AF48+0x15*0x28+0x18） | PASS |

**4/4 PASS**

---

## 1. INI ランタイム登録2件のキー [確定]

### 1.1 手法

登録サイト 0x404893 / 0x4440A3 の直前48Bに push 文字列がない理由を特定するため、(a) サイトを含む関数全体を capstone で線形ディスアセンブル、(b) .text 全域を E8 rel32 で走査して**その関数自体の呼出し元**を列挙、(c) 呼出し元の直前40B から push した .rdata 文字列を解決、(d) decompiled.c のアドレス範囲で enclosing FUN を照合。

### 1.2 実測

- **0x404893 は独立関数 `0x404850`（float エントリ追加 thunk）の内部**。本体:
  `mov esi,ecx`（this=呼び出し元が用意する entry オブジェクト）→ `fld dword [esp+0x20] / fstp [esi]`（value=float を entry+0 へ）→ `mov eax,[esp+0x1c] / mov [esi+4],eax`（name を entry+4 へ）→ `push esi; mov ecx,0xB07BF0; call 0x4040E0`（INI AddSetting）。
  → **キーは thunk 内ではなく呼び出し元から引数で渡される**（レジスタ/スタック経路の所以）。
- thunk 0x404850 の呼出し元は **1件のみ: `0x49D546`**（enclosing `FUN_0049D2A0`）。
  `fld dword [0xA3D65C]; fstp [esp]; push 0xA3F43C; mov ecx,0xB3524C; call 0x404850`、直前に `test byte [0xB35260],2 / jne skip / or [0xB35260],2` の**一度きりガード**。
  - key = `0xA3F43C` = **`fAlpha:Water`**、default = `[0xA3D65C]` = **0.5f**、entry オブジェクト = bss `0xB3524C`、ガード = bss `0xB35260` bit2。
- **0x4440A3 は独立関数 `0x444060`（dword エントリ追加 thunk）の内部**。本体:
  `mov eax,[esp+0x1c] / mov [esi+4],eax`（name）→ `mov ecx,[esp+0x20] / mov [esi],ecx`（value=生 dword、fld なし）→ `push esi; mov ecx,0xB07BF0; call 0x4040E0`。
- thunk 0x444060 の呼出し元は **1件のみ: `0x49DD72`**（enclosing `FUN_0049DD00`）。
  `push 0x80; push 0xA3F4C4; mov ecx,0xB35264; call 0x444060`、ガード `test/or byte [0xB3526C],1`。
  - key = `0xA3F4C4` = **`uSurfaceTextureSize:Water`**、default = **0x80 (128)**、entry = bss `0xB35264`、ガード = bss `0xB3526C` bit1。

### 1.3 結論

| サイト | 実体 | キー | 型/既定値 | 登録タイミング |
|---|---|---|---|---|
| 0x404893 | thunk 0x404850 の AddSetting 呼出 | **`fAlpha:Water`** | float = 0.5 | Water 系 FUN_0049D2A0 初回のみ（遅延1回） |
| 0x4440A3 | thunk 0x444060 の AddSetting 呼出 | **`uSurfaceTextureSize:Water`** | uint = 128 | Water 系 FUN_0049DD00 初回のみ（遅延1回） |

両キーとも「Water」セクションのレンダリング用設定。V51 の「ランタイム2件」は thunk 呼出2本そのものであり、キー名は thunk 呼び出し元（Water システムの初回処理）が1回だけ提供する。

---

## 2. BlendSettings 62エントリの全列挙 [確定]

### 2.1 手法

(a) .text 全域 E8→`0x4040E0`(AddSetting) 走査 + 直前の `mov ecx, 0xB11C40` マッチで 62 サイトを抽出 → 各サイトは `push <.data entry アドレス>` の1本のみ。
(b) entry アドレス列（0xB11A24 + n*8, n=0..61, stride 8）を実データ走査し `{u32 value, char* name}` を読み取り、name は .rdata で解決。
(c) 登録テーブル: 先頭 init `0x9E44B0` のポインタを含む .rdata 配列 = **`0xA29084`（62本連続、stride 4、値 = 0x9E44B0 + n*0x60, n=0..61）＝ CRT$XCU 初期化配列**。各 init は `push entry; mov ecx,0xB11C40; call 0x4040E0` の同一形。

### 2.2 登録構造（確定）

```
CRT$XCU[62]           @ 0xA29084  (fn ptrs: 0x9E44B0 + n*0x60)
  └─ 各 init fn       push &entry[n]; mov ecx, 0xB11C40; call 0x4040E0 (AddSetting)
entry[62]             @ 0xB11A24 .. 0xB11C13  ({u32 value_or_strptr; char* name}, stride 8)
BlendSettings object  @ 0xB11C40 (vptr 0xA566A4, 直後に配置)
```

### 2.3 全62件（キーと既定値）

| # | entry | key | name VA | 既定値 |
|---|---|---|---|---|
| 0 | `0xB11A24` | `fGetUpTime:HIT` | `0xA566D8` | 1 (0x3F800000) |
| 1 | `0xB11A2C` | `fKnockDownTime:HIT` | `0xA566E8` | 0.25 (0x3E800000) |
| 2 | `0xB11A34` | `fMinVelGain:HIT` | `0xA566FC` | 0.95 (0x3F733333) |
| 3 | `0xB11A3C` | `fMinHeirGain:HIT` | `0xA5670C` | 0.3 (0x3E99999A) |
| 4 | `0xB11A44` | `fHFMin:HIT` | `0xA56720` | 0 (0x00000000) |
| 5 | `0xB11A4C` | `fHFMax:HIT` | `0xA5672C` | 0 (0x00000000) |
| 6 | `0xB11A54` | `fBFMin:HIT` | `0xA56738` | 0 (0x00000000) |
| 7 | `0xB11A5C` | `fBFMax:HIT` | `0xA56744` | 0 (0x00000000) |
| 8 | `0xB11A64` | `sHitSpot:HIT` | `0xA56750` | str@0xA36ECC = 'Bip01 Spine2' |
| 9 | `0xB11A6C` | `sHitStart:HIT` | `0xA56760` | str@0xA36EBC = 'Bip01 Spine1' |
| 10 | `0xB11A74` | `sRHand:HIT` | `0xA5677C` | str@0xA56770 = '1.0, 1.0' |
| 11 | `0xB11A7C` | `sRForeArm:HIT` | `0xA56788` | str@0xA56770 = '1.0, 1.0' |
| 12 | `0xB11A84` | `sRUpperArm:HIT` | `0xA567A4` | str@0xA56798 = '0.2, 0.5' |
| 13 | `0xB11A8C` | `sLHand:HIT` | `0xA567B4` | str@0xA56770 = '1.0, 1.0' |
| 14 | `0xB11A94` | `sLForeArm:HIT` | `0xA567C0` | str@0xA56770 = '1.0, 1.0' |
| 15 | `0xB11A9C` | `sLUpperArm:HIT` | `0xA567D0` | str@0xA56798 = '0.2, 0.5' |
| 16 | `0xB11AA4` | `sSpine2:HIT` | `0xA567EC` | str@0xA567E0 = '0.5, 0.7' |
| 17 | `0xB11AAC` | `sSpine1:HIT` | `0xA56804` | str@0xA567F8 = '0.6, 0.8' |
| 18 | `0xB11AB4` | `sBody:HIT` | `0xA56810` | str@0xA56770 = '1.0, 1.0' |
| 19 | `0xB11ABC` | `sHead:HIT` | `0xA56828` | str@0xA5681C = '0.4, 0.6' |
| 20 | `0xB11AC4` | `sHitSpot:QUADHIT` | `0xA56834` | str@0xA3CAAC = 'Bip01 Head' |
| 21 | `0xB11ACC` | `sHitStart:QUADHIT` | `0xA56848` | str@0xA3CAAC = 'Bip01 Head' |
| 22 | `0xB11AD4` | `sRCalf:QUADHIT` | `0xA5685C` | str@0xA56770 = '1.0, 1.0' |
| 23 | `0xB11ADC` | `sRThigh:QUADHIT` | `0xA5686C` | str@0xA56770 = '1.0, 1.0' |
| 24 | `0xB11AE4` | `sRForeArm:QUADHIT` | `0xA5687C` | str@0xA56770 = '1.0, 1.0' |
| 25 | `0xB11AEC` | `sRUpperArm:QUADHIT` | `0xA56890` | str@0xA56770 = '1.0, 1.0' |
| 26 | `0xB11AF4` | `sLCalf:QUADHIT` | `0xA568A4` | str@0xA56770 = '1.0, 1.0' |
| 27 | `0xB11AFC` | `sLThigh:QUADHIT` | `0xA568B4` | str@0xA56770 = '1.0, 1.0' |
| 28 | `0xB11B04` | `sLForeArm:QUADHIT` | `0xA568C4` | str@0xA56770 = '1.0, 1.0' |
| 29 | `0xB11B0C` | `sLUpperArm:QUADHIT` | `0xA568D8` | str@0xA56770 = '1.0, 1.0' |
| 30 | `0xB11B14` | `sSpine2:QUADHIT` | `0xA568EC` | str@0xA56770 = '1.0, 1.0' |
| 31 | `0xB11B1C` | `sSpine1:QUADHIT` | `0xA568FC` | str@0xA56770 = '1.0, 1.0' |
| 32 | `0xB11B24` | `sBody:QUADHIT` | `0xA5690C` | str@0xA56770 = '1.0, 1.0' |
| 33 | `0xB11B2C` | `sHead:QUADHIT` | `0xA56928` | str@0xA5691C = '0.3, 0.9' |
| 34 | `0xB11B34` | `sHead:DEFAULT` | `0xA56938` | str@0xA56770 = '1.0, 1.0' |
| 35 | `0xB11B3C` | `sBody:DEFAULT` | `0xA56948` | str@0xA56770 = '1.0, 1.0' |
| 36 | `0xB11B44` | `sSpine1:DEFAULT` | `0xA56958` | str@0xA56770 = '1.0, 1.0' |
| 37 | `0xB11B4C` | `sSpine2:DEFAULT` | `0xA56968` | str@0xA56770 = '1.0, 1.0' |
| 38 | `0xB11B54` | `sLUpperArm:DEFAULT` | `0xA56978` | str@0xA56770 = '1.0, 1.0' |
| 39 | `0xB11B5C` | `sLForeArm:DEFAULT` | `0xA5698C` | str@0xA56770 = '1.0, 1.0' |
| 40 | `0xB11B64` | `sLHand:DEFAULT` | `0xA569A0` | str@0xA56770 = '1.0, 1.0' |
| 41 | `0xB11B6C` | `sLThigh:DEFAULT` | `0xA569B0` | str@0xA56770 = '1.0, 1.0' |
| 42 | `0xB11B74` | `sLCalf:DEFAULT` | `0xA569C0` | str@0xA56770 = '1.0, 1.0' |
| 43 | `0xB11B7C` | `sLFoot:DEFAULT` | `0xA569D0` | str@0xA56770 = '1.0, 1.0' |
| 44 | `0xB11B84` | `sRUpperArm:DEFAULT` | `0xA569E0` | str@0xA56770 = '1.0, 1.0' |
| 45 | `0xB11B8C` | `sRForeArm:DEFAULT` | `0xA569F4` | str@0xA56770 = '1.0, 1.0' |
| 46 | `0xB11B94` | `sRHand:DEFAULT` | `0xA56A14` | str@0xA56A08 = '1.0f, 1.0' |
| 47 | `0xB11B9C` | `sRThigh:DEFAULT` | `0xA56A24` | str@0xA56770 = '1.0, 1.0' |
| 48 | `0xB11BA4` | `sRCalf:DEFAULT` | `0xA56A34` | str@0xA56770 = '1.0, 1.0' |
| 49 | `0xB11BAC` | `sRFoot:DEFAULT` | `0xA56A44` | str@0xA56770 = '1.0, 1.0' |
| 50 | `0xB11BB4` | `sTail:DEFAULT` | `0xA56A54` | str@0xA56770 = '1.0, 1.0' |
| 51 | `0xB11BBC` | `sSideWeapon:DEFAULT` | `0xA56A64` | str@0xA56770 = '1.0, 1.0' |
| 52 | `0xB11BC4` | `sShield:DEFAULT` | `0xA56A78` | str@0xA56770 = '1.0, 1.0' |
| 53 | `0xB11BCC` | `sQuiver:DEFAULT` | `0xA56A88` | str@0xA56770 = '1.0, 1.0' |
| 54 | `0xB11BD4` | `sBackWeapon:DEFAULT` | `0xA56A98` | str@0xA56770 = '1.0, 1.0' |
| 55 | `0xB11BDC` | `sPonyTail:DEFAULT` | `0xA56AAC` | str@0xA56770 = '1.0, 1.0' |
| 56 | `0xB11BE4` | `fHighTrans:DEFAULT` | `0xA56AC0` | 0.07 (0x3D8F5C29) |
| 57 | `0xB11BEC` | `fHighRot:DEFAULT` | `0xA56AD4` | 0.005 (0x3BA3D70A) |
| 58 | `0xB11BF4` | `fLowTrans:DEFAULT` | `0xA56AE8` | 0.7 (0x3F333333) |
| 59 | `0xB11BFC` | `fLowRot:DEFAULT` | `0xA56AFC` | 0.2 (0x3E4CCCCD) |
| 60 | `0xB11C04` | `fPassOutTime:DEFAULT` | `0xA56B0C` | 1.2 (0x3F99999A) |
| 61 | `0xB11C0C` | `fPassOutForce:DEFAULT` | `0xA56B24` | -10 (0xC1200000) |

統計: prefix `f*`=14（float）/ `s*`=48（文字列ペア）。セクション内訳: HIT=20 / QUADHIT=14 / DEFAULT=28。
- `s*` 値は .rdata 文字列（例 `"1.0, 1.0"`=ブレンド重み min,max、`sHitSpot:HIT`=`"Bip01 Spine2"`=ボーン名）。
- float 既定値: fGetUpTime=1, fKnockDownTime=0.25, fMinVelGain=0.95, fMinHeirGain=0.3, fHFMin/Max・fBFMin/Max=0, fHighTrans=0.07, fHighRot=0.005, fLowTrans=0.7, fLowRot=0.2, fPassOutTime=1.2, fPassOutForce=-10。
- 特記: `sRHand:DEFAULT` のみ既定値が `"1.0f, 1.0"`（他は `"1.0, 1.0"`）。上流の vanilla 瑕疵と思われるが原文如此。

---

## 3. slot0 `0x412E20` を含む vtable `0xA40474`/`0xA4049C` の object と runtime 呼出元 [確定]

### 3.1 手法

(a) .data/.rdata file-backed（0xA28000-0xB01A00, 0xB02000-0xB32C00）を vtable アドレス dword で全走査、(b) vtable の .text imm32 参照（ctor 候補）を capstone で文脈確認、(c) object アドレスの .text imm32 参照を全列挙して runtime 利用を特定、(d) slot+0 間接 dispatch（`call [reg]`）と直接 E8 を全走査。

### 3.2 実測

- file-backed な**静的 object は0件**（vptr 初期値を持つ dword なし）。object は **bss `0xB35468`** に CRT 初期化で生成される:
  ```
  ; 0x9E06B0 (CRT$XCU 内、ラベル無し init fn)
  push 0x191            ; 引数 401（容量/要素数）
  mov  ecx, 0xB35468
  call 0x4A8330         ; ctor: [this]=0xA40474, +0x108=0, +0x4=0, lea +0x10C (map)
  push 0xA1AA00
  mov  dword [0xB35468], 0xA4049C   ; vptr を最終形へ
  call atexit           ; 0x981FB4
  ; 0xA1AA00 (exit thunk): mov ecx,0xB35468; jmp 0x4A8180 (dtor)
  ```
- 二段vtable: `0xA40474`=base ctor 部分 / `0xA4049C`=最終（derived）。ヘッダ文字列 **`'GameSettings'`(0xA40440) / `'Total GameSettings'`(0xA4042C) を Dump 実装 0x4A7FB0 が出力** → この object は **GameSettings バックエンド**（INI系と同型: +0x108 セッション, +0x10C map, +0x110 count）。スロット: +0x00=`0x412E20`(Add) / +0x08=`0x4A7FB0`(Dump) / +0x10=`0x4A7D60`(ReadOne) / +0x18=`0x4A7CB0`(End) / +0x1C=`0x4A81F0`(全 entry WriteOne 系) / +0x20=`0x4A8290`(全 entry ReadOne 系) / +0x24=`0x4A8040`。
- `0xB35468` の .text 参照は5件のみ: CRT init 2件、atexit thunk 1件、そして **runtime 利用は唯一 `FUN_0044DCF0`（ESM レコード読込、ゲート `cmp eax,"EDID"`）の `0x44E34B-0x44E35B`**:
  ```
  mov edx, [0xB35468] ; vptr
  mov eax, [edx+0x24] ; = 0x4A8040
  push esi            ; editor-id 文字列
  push edi            ; ESM レコード
  mov ecx, 0xB35468
  call eax            ; slot+0x24
  ```
  slot+0x24 `0x4A8040` = (session=引数1を +0x108 へ, key=引数2 を map 検索 0x55E000 → 命中時 slot+0x10 ReadOne → slot+0x18 End)。
- **Add ラッパ `0x412E20`（slot +0x00）の呼出元: 直接 E8 = 0件、5つの settings object 全参照(289件)の前方 0x180B に `call [reg]`(slot+0) dispatch = 0件** → **本バイナリでは実行時未使用**。本体の仕事は "entry->name NULL チェック → map(0x55E000) へ追加、重複時 `0xA32490` = "Setting key '%s' already used in map.\nSetting keys must be unique.\n" を 0x4A7A60 で出力"。

### 3.3 結論

- object = **bss `0xB35468`（GameSettings バックエンド、唯一インスタンス、CRT init 0x9E06B0 で生成・atexit 破棄）**。
- runtime 呼出元 = **`FUN_0044DCF0` の ESM "EDID" 読込経由の slot+0x24=`0x4A8040` のみ**。
- `0x412E20`(Add) 自体は **どこからも呼ばれない**（vtable スロット保持のみ）。INI 側の同型 Add が起動時登録に使われるのと対照的で、GameSettings への entry 追加は slot+0x24 経由（EDID ルックアップ＋ReadOne）に集約される。

---

## 4. INI Dump `0x4A8B10` の呼び出し元 [確定: 呼出元なし]

### 4.1 手法

(a) E8 直接呼出し全走査、(b) decompiled.c 1.07M 行を `PTR_PTR_00b0xxxx + 8`（Ghidra がこのバイナリで vtable dispatch を描画する形式）で全走査、(c) .text の `call dword [reg+8]` 全318件と `mov reg,[reg+8]` 全4560件を前後窓で object vptr 照合、(d) object imm32 参照289件の前方 0x180B を機械走査、(e) script opcode 台帳（0xB0AF48, stride 0x28, 532件）の INI 系ハンドラを確認。

### 4.2 実測

- Dump 実装を vtable +0x08 に持つ vtable は6本: `0xA404F8`(INI)/`0xA40548`(死蔵)/`0xA566A4`(Blend) → 0x4A8B10、`0xA40590`(Reg) → 0x4A8E50、`0xA40474`/`0xA4049C`(GameSettings) → 0x4A7CD0/0x4A7FB0。
- **E8 直接呼出し: 0x4A8B10=0件 / 0x4A8E50=0件 / 0x4A7FB0=0件（0x4A7CD0 の唯一の E8 は 0x4A7FF5 = 0x4A7FB0 の内部呼出）**。
- decompiled: `PTR_PTR_00b07bf0 + 8` 等の slot+8 dispatch は **0件**（+0x14/+0xC/+0x20 等の dispatch は存在する＝描画形式自体は正しい）。
- ASM 側: object 289 参照の前方 0x180B に slot+8 dispatch なし、`call [reg+8]` 318件に Dump 系 vptr object との近接一致0件。
- script opcode 台帳の INI 系: `RefreshINI`(opid 0x013E, impl 0x513BB0) / `SaveIniFiles`(0x0165, 0x513B60) / `SetINISetting`(0x011D, 0x5143C0) / `GetINISetting`(0x011E, 0x514230) — **いずれも Dump を呼ばない**（RefreshINI/SaveIniFiles は Begin/LoadAll/End 系 0x579B60/0x9811E2 経由）。

### 4.3 結論

**`0x4A8B10`(INI Dump) を実行時に呼ぶ経路は本 EXE に存在しない**（コンソールコマンド/メニュー/HUD のいずれにも該当なし）。Dump 系6本は vtable +0x08 に実装が存在するのみで、到達不能コード（開発時のデバッグ残骸）である。V51 §9-2 の「入力呼び出し元コマンドが未同定」は **「呼び出し元が存在しない」** に確定更新できる。移植時は Dump スロットは no-op 化してよい（出力先 0x70F910/0x70FB40 経由のログ出力自体は別機能）。

---

## 5. 未解決点・注意

1. **Ghidra ラベルギャップ**: decompiled.c には未ラベル領域が残る（例: CRT init 0x9E06B0, FUN_005A3810 周辺の一部）。本報告の ASM 走査はギャップ非依存で結論しているが、`call [reg+8]`/前方窓走査は「object ロードが 0x180B より遠い大関数」を原理的に取りこぼしうる。ただし decompiled 全体での slot+8 dispatch 0件と整合しており、呼出なし結論は robust。
2. **GameSettings の +0x24 呼出元は EDID 読込の1系統のみ**: ESM 内 GSTF/GSTS 相当レコードの処理と思われるが、slot+0x24 が返す成功/失敗の上流分岐（FUN_0044DCF0 の文脈全体）は未展開。必要なら次回のタスク。
3. **`sRHand:DEFAULT` の既定値 `"1.0f, 1.0"`** は他54件と異なる表記（vanilla 原文の typo 濃厚）。Blend 実装側のパーサが許容するかは blend 再現タスクで確認要。
4. thunk 0x404850/0x444060 は float/dword 専用の2種のみ確認。他型（string 等）の runtime 登録 thunk は INI 側に存在しない（string は静的初期化で登録済み、V51 の572件に包含）。
5. `0xB35468` の ctor 引数 `0x191`(401) の意味（容量 or フラグ）は未検証。map 実装 0x55E000/0x7C2A40 の深掘りに依存。

## 6. 証跡（%TEMP%\opencode\, 本会話の実行物）

- `v64_p1_ini2.py`（thunk ディスアセンブル） / `v64_p2_scan.py`（E8全走査・Blend init 62） / `v64_p3_detail.py`（entry 実読62・thunk 呼出元・vtable ctor） / `v64_p4_detail.py`（enclosing FUN・s* デコード・ctor コンテキスト） / `v64_p5_detail.py`（object 参照・Add 本体） / `v64_p6_detail.py`（0x44E34D 確認・call[reg+8] 318件） / `v64_p7_dump.py`（Dump vptr 分類） / `v64_p8_dump2.py`（object ポインタ走査） / `v64_p9_common.py`（+0x1C/+0x20 本体） / `v64_p10_strings.py`（文字列→参照） / `v64_p11_dump3.py`（vtable 全走査） / `v64_p12_cmds.py`（opcode 台帳） / `v64_p13_dec.py`・`v64_p14_final.py`（decompiled dispatch 形式） / `v64_p15_conclude.py`（RefreshINI 確認） / `v64_p16_final.py`（slot+0/直接E8/0xA1AA00） / `v64_p17_misc.py`（XCU 配列・統計）
- 中間データ: `v64_blend62.txt`, `v64_blend62_keys.txt`, `v64_commands.txt`
