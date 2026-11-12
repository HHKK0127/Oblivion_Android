# V38: `0x413A90` 分岐先完全解読 — 効果名表示メソッド `MagicEffectItem::GetName`

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam)
作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
解析日: 2026-10-24 / セルフテスト 4/4 PASS 後に分析開始

## 1. セルフテスト4件の実測値

| # | 検査 | 期待 | 実測 | 結果 |
|---|------|------|------|------|
| 1 | `bytes@0x504050` | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| 2 | `bytes@0x4FB580` | `578b7908` | `578b7908` | PASS |
| 3 | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| 4 | オペコード表 opid `0x0015` impl | `0x504560` | `0x504560` (エントリ@`0xB0B290`) | PASS |

PE セクション（自前 struct 解析）: ImageBase=0x400000 / .text VA=0x401000 RawPtr=0x400 RawSize=0x626E00 / .rdata VA=0xA28000 RawPtr=0x627200 / .data VA=0xB02000 RawPtr=0x700C00。

## 2. 関数 `0x413A90` の完全逆アセンブル

シグネチャ（実測）: `void __thiscall FUN_00413a90(int *this, char *outBuffer)`
- `this` = ecx。`*this` (offset 0) = MGEF fourcc (u32)。`this[5]` (offset 0x14) = actorValue (u32)。
- `outBuffer` = スタック第1引数。`ret 4` で stdcall 呼び出し元戻し。
- 終端: `0x413BA1 ret 4` の次が `0xCC`。関数サイズ = **0x114 (276 bytes)**、命令数 = **86**。

| VA | hex | 命令 |
|----|-----|------|
| `0x00413A90` | `8b01` | `mov eax, dword ptr [ecx]` |
| `0x00413A92` | `83ec08` | `sub esp, 8` |
| `0x00413A95` | `3d52454154` | `cmp eax, 0x54414552` |
| `0x00413A9A` | `56` | `push esi` |
| `0x00413A9B` | `0f8f83000000` | `jg 0x413b24` |
| `0x00413AA1` | `7457` | `je 0x413afa` |
| `0x00413AA3` | `3d4452534b` | `cmp eax, 0x4b535244` |
| `0x00413AA8` | `7f3e` | `jg 0x413ae8` |
| `0x00413AAA` | `0f84c2000000` | `je 0x413b72` |
| `0x00413AB0` | `3d4142534b` | `cmp eax, 0x4b534241` |
| `0x00413AB5` | `7438` | `je 0x413aef` |
| `0x00413AB7` | `3d464f534b` | `cmp eax, 0x4b534f46` |
| `0x00413ABC` | `757b` | `jne 0x413b39` |
| `0x00413ABE` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413AC1` | `8b35c034b300` | `mov esi, dword ptr [0xb334c0]` |
| `0x00413AC7` | `51` | `push ecx` |
| `0x00413AC8` | `e8f3211500` | `call 0x565cc0` |
| `0x00413ACD` | `8b542414` | `mov edx, dword ptr [esp + 0x14]` |
| `0x00413AD1` | `50` | `push eax` |
| `0x00413AD2` | `56` | `push esi` |
| `0x00413AD3` | `686425a300` | `push 0xa32564` |
| `0x00413AD8` | `52` | `push edx` |
| `0x00413AD9` | `e8ade55600` | `call 0x98208b` |
| `0x00413ADE` | `83c414` | `add esp, 0x14` |
| `0x00413AE1` | `5e` | `pop esi` |
| `0x00413AE2` | `83c408` | `add esp, 8` |
| `0x00413AE5` | `c20400` | `ret 4` |
| `0x00413AE8` | `3d41424154` | `cmp eax, 0x54414241` |
| `0x00413AED` | `754a` | `jne 0x413b39` |
| `0x00413AEF` | `8b35e034b300` | `mov esi, dword ptr [0xb334e0]` |
| `0x00413AF5` | `e986000000` | `jmp 0x413b80` |
| `0x00413AFA` | `8b35c834b300` | `mov esi, dword ptr [0xb334c8]` |
| `0x00413B00` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413B03` | `51` | `push ecx` |
| `0x00413B04` | `e8b7211500` | `call 0x565cc0` |
| `0x00413B09` | `8b4c2414` | `mov ecx, dword ptr [esp + 0x14]` |
| `0x00413B0D` | `50` | `push eax` |
| `0x00413B0E` | `56` | `push esi` |
| `0x00413B0F` | `686425a300` | `push 0xa32564` |
| `0x00413B14` | `51` | `push ecx` |
| `0x00413B15` | `e871e55600` | `call 0x98208b` |
| `0x00413B1A` | `83c414` | `add esp, 0x14` |
| `0x00413B1D` | `5e` | `pop esi` |
| `0x00413B1E` | `83c408` | `add esp, 8` |
| `0x00413B21` | `c20400` | `ret 4` |
| `0x00413B24` | `3d44474154` | `cmp eax, 0x54414744` |
| `0x00413B29` | `744f` | `je 0x413b7a` |
| `0x00413B2B` | `3d464f4154` | `cmp eax, 0x54414f46` |
| `0x00413B30` | `748c` | `je 0x413abe` |
| `0x00413B32` | `3d44524154` | `cmp eax, 0x54415244` |
| `0x00413B37` | `7439` | `je 0x413b72` |
| `0x00413B39` | `8d542404` | `lea edx, [esp + 4]` |
| `0x00413B3D` | `52` | `push edx` |
| `0x00413B3E` | `e8adfeffff` | `call 0x4139f0` |
| `0x00413B43` | `8b08` | `mov ecx, dword ptr [eax]` |
| `0x00413B45` | `8b542410` | `mov edx, dword ptr [esp + 0x10]` |
| `0x00413B49` | `8da42400000000` | `lea esp, [esp]` |
| `0x00413B50` | `8a01` | `mov al, byte ptr [ecx]` |
| `0x00413B52` | `8802` | `mov byte ptr [edx], al` |
| `0x00413B54` | `83c101` | `add ecx, 1` |
| `0x00413B57` | `83c201` | `add edx, 1` |
| `0x00413B5A` | `84c0` | `test al, al` |
| `0x00413B5C` | `75f2` | `jne 0x413b50` |
| `0x00413B5E` | `8b442404` | `mov eax, dword ptr [esp + 4]` |
| `0x00413B62` | `50` | `push eax` |
| `0x00413B63` | `e8b8e3feff` | `call 0x401f20` |
| `0x00413B68` | `83c404` | `add esp, 4` |
| `0x00413B6B` | `5e` | `pop esi` |
| `0x00413B6C` | `83c408` | `add esp, 8` |
| `0x00413B6F` | `c20400` | `ret 4` |
| `0x00413B72` | `8b35d034b300` | `mov esi, dword ptr [0xb334d0]` |
| `0x00413B78` | `eb86` | `jmp 0x413b00` |
| `0x00413B7A` | `8b35d834b300` | `mov esi, dword ptr [0xb334d8]` |
| `0x00413B80` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413B83` | `51` | `push ecx` |
| `0x00413B84` | `e837211500` | `call 0x565cc0` |
| `0x00413B89` | `50` | `push eax` |
| `0x00413B8A` | `8b442418` | `mov eax, dword ptr [esp + 0x18]` |
| `0x00413B8E` | `56` | `push esi` |
| `0x00413B8F` | `686425a300` | `push 0xa32564` |
| `0x00413B94` | `50` | `push eax` |
| `0x00413B95` | `e8f1e45600` | `call 0x98208b` |
| `0x00413B9A` | `83c414` | `add esp, 0x14` |
| `0x00413B9D` | `5e` | `pop esi` |
| `0x00413B9E` | `83c408` | `add esp, 8` |
| `0x00413BA1` | `c20400` | `ret 4` |

## 3. cmp 8件の分岐条件・ターゲットと二分探索木

fourcc は little-endian u32 で比較（例 REAT = `0x54414552` = "TAEK" LE → "REAT"）。

| # | VA | cmp オペランド | fourcc | 分岐 | ターゲット | 集約先ブロック |
|---|----|----|----|----|----|----|
| 1 | `0x413A95` | `0x54414552` | REAT | `jg` | `0x413B24` | (右木へ) |
| 1 | `0x413AA1` | (REAT) | REAT | `je` | `0x413AFA` | **REAT ブロック** |
| 2 | `0x413AA3` | `0x4B535244` | DRSK | `jg` | `0x413AE8` | (中木へ) |
| 2 | `0x413AAA` | (DRSK) | DRSK | `je` | `0x413B72` | **DRSK/DRAT ブロック** |
| 3 | `0x413AB0` | `0x4B534241` | ABSK | `je` | `0x413AEF` | **ABSK/ABAT ブロック** |
| 4 | `0x413AB7` | `0x4B534F46` | FOSK | `jne` | `0x413B39` | (default) / fall-through |
| 4 | `0x413ABC` | (FOSK) | FOSK | fall-thr | `0x413ABE` | **FOSK/FOAT ブロック** |
| 5 | `0x413AE8` | `0x54414241` | ABAT | `jne` | `0x413B39` | (default) / fall-through |
| 5 | `0x413AED` | (ABAT) | ABAT | fall-thr | `0x413AEF` | **ABSK/ABAT ブロック** |
| 6 | `0x413B24` | `0x54414744` | DGAT | `je` | `0x413B7A` | **DGAT ブロック** |
| 7 | `0x413B2B` | `0x54414F46` | FOAT | `je` | `0x413ABE` | **FOSK/FOAT ブロック** |
| 8 | `0x413B32` | `0x54415244` | DRAT | `je` | `0x413B72` | **DRSK/DRAT ブロック** |
| 8 | `0x413B37` | (DRAT) | DRAT | fall-thr | `0x413B39` | (default) |

### 二分探索木の構造図（テキスト）

```
                       [cmp REAT 0x54414552]  @0x413A95
                       /         |           \
                  jg /      je ==|            \ (eax < REAT)
                     /           |             \
        [0x413B24 右木]     **0x413AFA REAT**   [0x413AA3 中木]
        (eax > REAT)        → "Restore"         (eax < REAT)
              |                                      |
   [cmp DGAT 0x54414744] @0x413B24        [cmp DRSK 0x4B535244] @0x413AA3
        / je   |  \ (>)                       / je   |   \ (<)
       /       |   \                          /       |    \
 **0x413B7A**  |  [0x413B2B]            **0x413B72**  |  [0x413AB0]
 DGAT          |  (eax>DGAT)             DRSK/DRAT    |  (eax<DRSK)
→"Damage"      |                            →"Drain"   |
               |                                      |
        [cmp FOAT 0x54414F46] @0x413B2B     [cmp ABSK 0x4B534241] @0x413AB0
          / je    |  \ (>)                     / je   |  \ (<)
         /        |   \                       /       |   \
  **0x413ABE**     | [0x413B32]       **0x413AEF**     | [0x413AB7]
  FOSK/FOAT        |  (eax>FOAT)      ABSK/ABAT       |  (eax<ABSK)
  →"Fortify"       |                  →"Absorb"       |
                   |                                 [cmp FOSK 0x4B534F46] @0x413AB7
          [cmp DRAT 0x54415244] @0x413B32               / jne  |  \ (==)
            / je    |  \ (>)                            /      |   \
           /        |   \                              /       |    \
    **0x413B72**     | 0x413B39                **0x413ABE**    |  --
    DRSK/DRAT        |  (default)               FOSK/FOAT     |
    →"Drain"         |                         →"Fortify"    |
                     |                                       |
          [cmp ABAT 0x54414241] @0x413AE8            (eax<ABSK は FOSK のみ)
            / jne  |  \ (==)                          → 他は default 0x413B39
           /       |   \
          /        |    \
   0x413B39     **0x413AEF**
   (default)     ABSK/ABAT
                 →"Absorb"

 8 fourcc の数値順: ABSK< FOSK< DRSK< ABAT< REAT< DGAT< FOAT< DRAT
```

### 5〜6個の分岐先（すべて特定済み）

木は **8 コードを 5 種の演算接頭辞 + 1 種の default に集約**する。分岐先ブロックは実質 6 個:

| 分岐先 VA | 到達 fourcc | 接頭辞グローバル | 接頭辞文字列(実読) |
|-----------|------------|----------------|-------------------|
| `0x413AFA` | REAT | `DAT_00b334c8` | `Restore` |
| `0x413ABE` | FOSK, FOAT | `DAT_00b334c0` | `Fortify` |
| `0x413AEF` | ABSK, ABAT | `DAT_00b334e0` | `Absorb` |
| `0x413B72` | DRSK, DRAT | `DAT_00b334d0` | `Drain` |
| `0x413B7A` | DGAT | `DAT_00b334d8` | `Damage` |
| `0x413B39` | その他137種 (default) | — | (base name のみ) |

## 4. 各分岐先ブロックの逆アセンブルと書式呼び出し・文字列

### 4.0 グローバル文字列の実読（.data BSS → 初期化スタブ経由で確定）

`DAT_00b334c0..e0` は .data の BSS（raw に無し、実行時初期化）。初期化スタブ `0x9D9EE0..0x9D9F6F` が
`push <value_str>; push <name_key>; mov ecx, <global>; call 0x419A70` (TString コンストラクタ) で登録。

| グローバル VA | name key (実読) | value 文字列 (実読) |
|---------------|----------------|--------------------|
| `0x00B334C0` | `sMagicEffectItemFortify` | **`Fortify`** |
| `0x00B334C8` | `sMagicEffectItemRestore` | **`Restore`** |
| `0x00B334D0` | `sMagicEffectItemDrain` | **`Drain`** |
| `0x00B334D8` | `sMagicEffectItemDamage` | **`Damage`** |
| `0x00B334E0` | `sMagicEffectItemAbsorb` | **`Absorb`** |

周辺ファミリ（同じ `sMagicEffectItem*` 系）: `pt` / `secs` / `pts` / `ft` / `Lock` / `Create` / `up to level` / `Strike`。

書式文字列: `0xA32564` = **`%s %s`**（実読）。書式関数: `0x98208B` = `_sprintf` (cdecl, `add esp,0x14` で4引数クリーン)。

### 4.1. FOSK/FOAT ブロック（"Fortify"）

| VA | hex | 命令 |
|----|-----|------|
| `0x00413ABE` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413AC1` | `8b35c034b300` | `mov esi, dword ptr [0xb334c0]` |
| `0x00413AC7` | `51` | `push ecx` |
| `0x00413AC8` | `e8f3211500` | `call 0x565cc0` |
| `0x00413ACD` | `8b542414` | `mov edx, dword ptr [esp + 0x14]` |
| `0x00413AD1` | `50` | `push eax` |
| `0x00413AD2` | `56` | `push esi` |
| `0x00413AD3` | `686425a300` | `push 0xa32564` |
| `0x00413AD8` | `52` | `push edx` |
| `0x00413AD9` | `e8ade55600` | `call 0x98208b` |
| `0x00413ADE` | `83c414` | `add esp, 0x14` |
| `0x00413AE1` | `5e` | `pop esi` |
| `0x00413AE2` | `83c408` | `add esp, 8` |
| `0x00413AE5` | `c20400` | `ret 4` |

### 4.2. ABSK/ABAT ブロック（"Absorb"）

| VA | hex | 命令 |
|----|-----|------|
| `0x00413AEF` | `8b35e034b300` | `mov esi, dword ptr [0xb334e0]` |
| `0x00413AF5` | `e986000000` | `jmp 0x413b80` |

### 4.3. REAT ブロック（"Restore"）

| VA | hex | 命令 |
|----|-----|------|
| `0x00413AFA` | `8b35c834b300` | `mov esi, dword ptr [0xb334c8]` |
| `0x00413B00` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413B03` | `51` | `push ecx` |
| `0x00413B04` | `e8b7211500` | `call 0x565cc0` |
| `0x00413B09` | `8b4c2414` | `mov ecx, dword ptr [esp + 0x14]` |
| `0x00413B0D` | `50` | `push eax` |
| `0x00413B0E` | `56` | `push esi` |
| `0x00413B0F` | `686425a300` | `push 0xa32564` |
| `0x00413B14` | `51` | `push ecx` |
| `0x00413B15` | `e871e55600` | `call 0x98208b` |
| `0x00413B1A` | `83c414` | `add esp, 0x14` |
| `0x00413B1D` | `5e` | `pop esi` |
| `0x00413B1E` | `83c408` | `add esp, 8` |
| `0x00413B21` | `c20400` | `ret 4` |

### 4.4. DRSK/DRAT ブロック（"Drain"）

| VA | hex | 命令 |
|----|-----|------|
| `0x00413B72` | `8b35d034b300` | `mov esi, dword ptr [0xb334d0]` |
| `0x00413B78` | `eb86` | `jmp 0x413b00` |

### 4.5. DGAT ブロック（"Damage"）— 0x413B80 共通テール

| VA | hex | 命令 |
|----|-----|------|
| `0x00413B7A` | `8b35d834b300` | `mov esi, dword ptr [0xb334d8]` |
| `0x00413B80` | `8b4914` | `mov ecx, dword ptr [ecx + 0x14]` |
| `0x00413B83` | `51` | `push ecx` |
| `0x00413B84` | `e837211500` | `call 0x565cc0` |
| `0x00413B89` | `50` | `push eax` |
| `0x00413B8A` | `8b442418` | `mov eax, dword ptr [esp + 0x18]` |
| `0x00413B8E` | `56` | `push esi` |
| `0x00413B8F` | `686425a300` | `push 0xa32564` |
| `0x00413B94` | `50` | `push eax` |
| `0x00413B95` | `e8f1e45600` | `call 0x98208b` |
| `0x00413B9A` | `83c414` | `add esp, 0x14` |
| `0x00413B9D` | `5e` | `pop esi` |
| `0x00413B9E` | `83c408` | `add esp, 8` |

### 4.6. default ブロック（137種 fallback）

| VA | hex | 命令 |
|----|-----|------|
| `0x00413B39` | `8d542404` | `lea edx, [esp + 4]` |
| `0x00413B3D` | `52` | `push edx` |
| `0x00413B3E` | `e8adfeffff` | `call 0x4139f0` |
| `0x00413B43` | `8b08` | `mov ecx, dword ptr [eax]` |
| `0x00413B45` | `8b542410` | `mov edx, dword ptr [esp + 0x10]` |
| `0x00413B49` | `8da42400000000` | `lea esp, [esp]` |
| `0x00413B50` | `8a01` | `mov al, byte ptr [ecx]` |
| `0x00413B52` | `8802` | `mov byte ptr [edx], al` |
| `0x00413B54` | `83c101` | `add ecx, 1` |
| `0x00413B57` | `83c201` | `add edx, 1` |
| `0x00413B5A` | `84c0` | `test al, al` |
| `0x00413B5C` | `75f2` | `jne 0x413b50` |
| `0x00413B5E` | `8b442404` | `mov eax, dword ptr [esp + 4]` |
| `0x00413B62` | `50` | `push eax` |
| `0x00413B63` | `e8b8e3feff` | `call 0x401f20` |
| `0x00413B68` | `83c404` | `add esp, 4` |
| `0x00413B6B` | `5e` | `pop esi` |
| `0x00413B6C` | `83c408` | `add esp, 8` |
| `0x00413B6F` | `c20400` | `ret 4` |

### 4.7 共通テールの詳細（書式呼び出しの呼び出し先と引数）

3 箇所の `_sprintf` 呼び出し（すべて `push 0xA32564` = `"%s %s"`）:

| sprintf 呼出 VA | 呼び出し先 | 扱うブロック | 第1引数(dst) | 第2引数(fmt) | 第3引数(%s #1) | 第4引数(%s #2) |
|----------------|-----------|-------------|-------------|-------------|---------------|---------------|
| `0x413AD9` | `0x98208B` (_sprintf) | FOSK/FOAT | `[esp+0x14]` = outBuffer | `0xA32564` `%s %s` | esi = `DAT_00b334c0` "Fortify" | eax = `FUN_00565cc0(this[5])` AV名 |
| `0x413B15` | `0x98208B` (_sprintf) | REAT/DRSK/DRAT | `[esp+0x14]` = outBuffer | `0xA32564` `%s %s` | esi = "Restore"/"Drain" | eax = AV名 |
| `0x413B95` | `0x98208B` (_sprintf) | ABSK/ABAT/DGAT | `[esp+0x18]` = outBuffer | `0xA32564` `%s %s` | esi = "Absorb"/"Damage" | eax = AV名 |

AV名の取得先 `FUN_00565cc0` (0x565CC0):

```c
// docs\Oblivion_decompiled.c L265534
undefined * FUN_00565cc0(uint param_1) {
  if (param_1 < 0x28) { if (PTR_DAT_00b12758[param_1] != 0) return *PTR_DAT_00b12758[param_1]; }
  else if ((int)param_1 < 0x48) { return PTR_DAT_00b12868[param_1]; }
  return 0;
}
```

`PTR_DAT_00b12758` は .rdata の actorValue→TString* テーブル（AV 0..0x27）。先頭 `AV[0]`→`0xB3A034`。
その BSS オブジェクトは `0x9F966A` で `push 0xA51EF4("Strength"); push 0xA65B78; mov ecx,0xB3A034; call 0x419A70` と初期化。
→ **`0x565CC0` = actorValue 索引から属性/スキルの英語名文字列を返す関数**（"Strength" "Intelligence" "Magicka" "Blade" 等）。

したがって表示される文字列は **`"<演算接頭辞> <属性/スキル名>"`** 例: "Restore Strength" / "Fortify Blade" / "Absorb Magicka" / "Damage Intelligence" / "Drain Speed"。

### 4.8 default ブロックの data source

`0x413B39`: `lea edx,[esp+4]; push edx; call 0x4139F0` → ESM ベースのエフェクト名（`MagicItem` の name 文字列）を取得。
`FUN_004139f0` (0x4139F0) は `this[6]`(offset 0x18) の名前キャッシュを参照し、未初期化なら `FUN_004134c0` で ESM から解決。
戻り値の char* を outBuffer へ strcpy 後、`FUN_00401f20` (free) で解放。
→ 8 特殊コード以外（SEFF / FIRE / FRZE / 素の属性名など 137 種）は base name のみを返す。

## 5. 用途判定

### 判定: **(A) 効果名の表示系**

根拠:
1. **書式呼び出し**: 3 箇所すべてが `_sprintf(outBuffer, "%s %s", 演算接頭辞, actorValue名)` — 属性/スキル名付きの効果表示名を構築。
2. **文字列内容**: 接頭辞 "Fortify"/"Restore"/"Drain"/"Damage"/"Absorb" はいずれも Oblivion の magic effect 表示名の演算部分。AV 名は "Strength" 等の属性/スキル名。
3. **呼び出し元**: 4 件すべてが UI 表示系（§6）— magic menu の ACTIVE EFFECTS リスト、alchemy 作成画面、enchanting 画面、effect tooltip。
4. **エラー処理ではない**: エラーメッセージは `0xA2F7EC`（空文字）や専用文字列を使うが、本関数はそれを一切参照しない。戻り値は void、出力は outBuffer への文字列。
5. **デコンパイル的一貫性**: `docs\Oblivion_decompiled.c` L12074-12133 の `FUN_00413a90` は `void __thiscall FUN_00413a90(int *param_1, char *param_2)` として、`_sprintf(param_2,"%s %s",uVar,uVar5)` のみを実行し default で base name をコピー。

**推定クラス**: `MagicEffectItem`（ESM の MGEF レコード + 個別 actorValue instance）。`GetName(char*)` 相当のメソッド。

## 6. 呼び出し元（E8 / imm32 参照）

### 直接 E8 呼び出し: **4 件**

| # | 呼出元 VA | 所属関数 (decompile) | コンテキスト |
|---|-----------|---------------------|-------------|
| 1 | `0x413CDE` | `FUN_00413be0` @ 0x413BE0 | effect tooltip / name ビルダー。LOCK fourcc (`0x4B434F4C`) 判定や flag `0x1180000` チェックを含む上位メソッド。`FUN_00413a90` を else 分岐で呼ぶ。 |
| 2 | `0x41CFA1` | `FUN_0041cbb0` @ 0x41CBB0 | magic menu の **ACTIVE EFFECTS** リスト表示。`"CURRENT EFFECTS:"` ヘッダ (decompile L18995) を出力後、各 effect の名前を `0x413A90` で取得。magnitude/duration と併記。 |
| 3 | `0x594BA4` | `FUN_00593cd0` @ 0x593CD0 | **alchemy potion 作成** メニュー。`FUN_00593b20` (mixing) の後、state byte `+0xA4` が 2/3 でなければ effect 名を `0x413A90` で生成し `FUN_0057ff20` へ渡す（UI テキスト設定）。 |
| 4 | `0x5B336A` | `FUN_005b2b70` @ 0x5B2B70 | **enchanting メニュー**。`"Icons"` パス生成 (`_sprintf("%s\\%s","Icons")`) と effect リスト処理の中で effect 名を生成。 |

### imm32 参照（関数ポインタ等）: **0 件**

`.text` 全体を `90 3a 41 00` (0x413A90 LE) バイト走査 — 直接 E8 4 件以外に imm32 参照なし。vtable/テーブル登録も無し（通常の thiscall メソッド、間接参照なし）。

**系統判定**: 4 呼び出し元すべてが UI 表示（magic menu / alchemy / enchanting / tooltip）。本関数は **MagicEffectItem の表示名生成メソッド**であり、`0x413BE0`（上位 tooltip ビルダー）のサブルーチン的位置。

## 7. MGEF コード突合（`tmp\re\v35_mgef_code_table.csv`）

V35 正本 145 件のうち、本関数が cmp する 8 件を照合。`cpp_branch_va` 列が V35 時点で本解析と一致。

| fourcc | full_name (V35) | school | ESM FormID | V35 cpp_branch_va | 本解析の分岐先 | 一致 |
|--------|-----------------|--------|-----------|-------------------|---------------|------|
| REAT | Restore Attribute | 5 | 0x00001889 | 0x413AFA | 0x413AFA | OK |
| DRSK | Drain Skill | 2 | 0x0000186E | 0x413B72 | 0x413B72 | OK |
| ABSK | Absorb Skill | 5 | 0x000008CD | 0x413AEF | 0x413AEF | OK |
| FOSK | Fortify Skill | 5 | 0x00001879 | 0x413ABE | 0x413ABE | OK |
| ABAT | Absorb Attribute | 5 | 0x000008C9 | 0x413AEF | 0x413AEF | OK |
| DGAT | Damage Attribute | 2 | 0x00001863 | 0x413B7A | 0x413B7A | OK |
| FOAT | Fortify Attribute | 5 | 0x00001875 | 0x413ABE | 0x413ABE | OK |
| DRAT | Drain Attribute | 2 | 0x0000186B | 0x413B72 | 0x413B72 | OK |

8/8 が V35 正本に存在し、disposition はすべて `cmp_branch`、ref_cmp_dispatch = `yes`。登録 23 組（ctor あり）には含まれない（unregistered 122 のうちの 8 件）。

## 8. 判定できなかった点 / 未検証

1. **`this` の実体クラス名**: RTTI 上の正式クラス名は未特定（呼び出し元 `0x41CFA1` は `*(iVar3+0xC)` を this として渡す = effect entry リストの要素）。`MagicEffectItem` は name key 文字列 `sMagicEffectItem*` からの推定。
2. **actorValue 0..0x27 の全名**: テーブル構造は確認済みだが、AV[0]="Strength" 以外の 40 種は未実読（BSS 初期化のため）。表示系であることの判定には不要。
3. **`0x413BE0` の完全な flag 判定ロジック**: 呼び出し元 #1 の上位メソッド全体（flag `0x1180000` / byte `+0x5B` bit0 の意味）は V38 スコープ外。
4. **日本語/ローカライズ**: 初期化スタブは英語リテラル。ローカライズ経路（`0x419A70` の第2引数 name key が localized string table を引くか）は未追跡。

## 9. 成果物

- 本ファイル: `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v38_413a90_branches.md`
- 参照: `tmp\re\v35_mgef_code_table.csv`, `tmp\re\v35_mgef_code_report.md`, `docs\Oblivion_decompiled.c`

---
生成: opencode V38 セッション（capstone 5.0.7 / 自前 PE struct 解析 / %TEMP%\opencode\ スクリプト）
