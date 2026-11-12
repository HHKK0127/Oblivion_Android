# V29: 台帳ハンドラから呼ばれる IAT エントリの解決（Oblivion.exe v1.2.0416 Steam版）

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` / 入力: `tmp\re\v18_indirect_calls.csv`（間接呼出 406 件）
成果物: `tmp\re\v29_iat_calls.csv`（26 行）

## 1. セルフテスト 4 件（解析前実施・全 PASS）

| # | テスト | 期待値 | 実測値 | 結果 |
|---|--------|--------|--------|------|
| 1 | `bytes@0x504050` | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| 2 | `bytes@0x4FB580` | `578b7908` | `578b7908` | PASS |
| 3 | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| 4 | `opid 0x0015 impl` | `0x504560` | `0x504560` | PASS |

## 2. PE インポートテーブルの構造

- 解析方法: `struct` による手動 PE パース（pefile 不使用）。
  e_lfanew@0x3C → COFF(+4) → nsec@coff+2 / optsize@coff+16 → OptionalHeader ImageBase@opt+28 →
  **DataDirectory[1]（Import）= opt+96+8 に RVA/Size** → セクション表 opt+optsize+40*i（+8=VirtualSize/+12=VA/+16=RawSize/+20=RawPtr）。
  `file_off = PointerToRawData + (RVA - VirtualAddress)`（.text では `VA = 0x400C00 + file_off` と一致）。
- Import Directory RVA=0x700304, Size=0x12C
- 各 `IMAGE_IMPORT_DESCRIPTOR`（20 bytes: OriginalFirstThunk/TimeDateStamp/ForwarderChain/Name RVA/FirstThunk）を全零終端まで列挙。
- DLL 名は Name RVA の ASCII 文字列。INT は OriginalFirstThunk（0 の場合は FirstThunk）、**PE32 のためエントリは 4 バイト**。
  bit31=1 はインポート ordinal（下位 16bit）、bit31=0 は Hint/Name Table への RVA → hint(u16) + ASCII 名。
- IAT スロット VA = ImageBase + FirstThunk + 4*i をキーに `DLL!関数名` へ写像。バイト順はリトルエンディアンで `struct.unpack('<I')`。

| 項目 | 値 |
|------|----|
| DLL 数 | 14 |
| IAT スロット総数 | 230 |
| ordinal インポート数 | 18 |
| 呼出された distinct IAT スロット | 8 |

インポート DLL 一覧:

| DLL | スロット数 | IAT 範囲 |
|-----|-----------|----------|
| COMCTL32.dll | 3 | 0x00A2801C..0x00A28024 |
| DINPUT8.dll | 1 | 0x00A2802C..0x00A2802C |
| WINMM.dll | 7 | 0x00A282C8..0x00A282E0 |
| DSOUND.dll | 1 | 0x00A28034..0x00A28034 |
| binkw32.dll | 9 | 0x00A28330..0x00A28350 |
| KERNEL32.dll | 128 | 0x00A28044..0x00A28240 |
| USER32.dll | 25 | 0x00A28250..0x00A282B0 |
| GDI32.dll | 1 | 0x00A2803C..0x00A2803C |
| ADVAPI32.dll | 6 | 0x00A28000..0x00A28014 |
| SHELL32.dll | 1 | 0x00A28248..0x00A28248 |
| ole32.dll | 3 | 0x00A283C0..0x00A283C8 |
| VERSION.dll | 3 | 0x00A282B8..0x00A282C0 |
| d3dx9_27.dll | 25 | 0x00A28358..0x00A283B8 |
| WSOCK32.dll | 17 | 0x00A282E8..0x00A28328 |

## 3. `call [abs32]` 26 件の解決結果

- `v18_indirect_calls.csv` の `insn_text` に `call dword ptr [0x` を含む行: **26 件**（`disp` 列とアドレス一致 26/26、すべて 0xA28000〜0xA30000 帯: 26/26）。
- 解決: **26 件 / 未解決: 0 件**

| # | opid | ハンドラ名 | impl | site_va | IAT スロット | DLL!関数 | hint |
|---|------|-----------|------|---------|--------------|----------|------|
| 1 | 0x0120 | Op0120 | 0x00510C70 | 0x00510D42 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 2 | 0x012D | Op012D | 0x0050A400 | 0x0050A494 | 0x00A28090 | KERNEL32.dll!CreateFileA | 83 |
| 3 | 0x012D | Op012D | 0x0050A400 | 0x0050A4A8 | 0x00A280A4 | KERNEL32.dll!SetFilePointer | 795 |
| 4 | 0x012D | Op012D | 0x0050A400 | 0x0050A4B3 | 0x00A28174 | KERNEL32.dll!GetLocalTime | 371 |
| 5 | 0x012D | Op012D | 0x0050A400 | 0x0050A870 | 0x00A28094 | KERNEL32.dll!CloseHandle | 52 |
| 6 | 0x013A | Op013A | 0x00515330 | 0x005155BA | 0x00A28078 | KERNEL32.dll!InterlockedIncrement | 556 |
| 7 | 0x013A | Op013A | 0x00515330 | 0x00515605 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 8 | 0x013A | Op013A | 0x00515330 | 0x00515627 | 0x00A28078 | KERNEL32.dll!InterlockedIncrement | 556 |
| 9 | 0x013A | Op013A | 0x00515330 | 0x005156DB | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 10 | 0x013A | Op013A | 0x00515330 | 0x005156FB | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 11 | 0x013A | Op013A | 0x00515330 | 0x0051574D | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 12 | 0x013A | Op013A | 0x00515330 | 0x00515773 | 0x00A28078 | KERNEL32.dll!InterlockedIncrement | 556 |
| 13 | 0x013A | Op013A | 0x00515330 | 0x00515898 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 14 | 0x013A | Op013A | 0x00515330 | 0x005158D4 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 15 | 0x0147 | Op0147 | 0x005023D0 | 0x00502457 | 0x00A28164 | KERNEL32.dll!lstrcpyA | 966 |
| 16 | 0x0147 | Op0147 | 0x005023D0 | 0x0050249E | 0x00A28160 | KERNEL32.dll!lstrcatA | 957 |
| 17 | 0x0148 | Op0148 | 0x00502500 | 0x00502587 | 0x00A28164 | KERNEL32.dll!lstrcpyA | 966 |
| 18 | 0x0148 | Op0148 | 0x00502500 | 0x005025CE | 0x00A28160 | KERNEL32.dll!lstrcatA | 957 |
| 19 | 0x014E | Op014E | 0x0050EF90 | 0x0050F25B | 0x00A28078 | KERNEL32.dll!InterlockedIncrement | 556 |
| 20 | 0x014E | Op014E | 0x0050EF90 | 0x0050F345 | 0x00A28078 | KERNEL32.dll!InterlockedIncrement | 556 |
| 21 | 0x014E | Op014E | 0x0050EF90 | 0x0050F362 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 22 | 0x0179 | Op0179 | 0x0050E6A0 | 0x0050E710 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 23 | 0x0179 | Op0179 | 0x0050E6A0 | 0x0050E7BB | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 24 | 0x0179 | Op0179 | 0x0050E6A0 | 0x0050E8DD | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 25 | 0x0179 | Op0179 | 0x0050E6A0 | 0x0050E912 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |
| 26 | 0x101D | RemoveSpell | 0x00510B90 | 0x00510D42 | 0x00A2807C | KERNEL32.dll!InterlockedDecrement | 552 |

### 未解決 IAT スロット

なし（26 件すべて IAT テーブル内で `DLL!関数名` に解決）。Forwarder 疑いスロットは検出されなかった。

## 4. ハンドラ別 API 一覧（上位 20 ハンドラ）

| # | opid | ハンドラ | impl | 呼出数 | 呼ぶ API |
|---|------|---------|------|--------|----------|
| 1 | 0x013A | Op013A | 0x00515330 | 9 | `KERNEL32.dll!InterlockedDecrement`, `KERNEL32.dll!InterlockedIncrement` |
| 2 | 0x012D | Op012D | 0x0050A400 | 4 | `KERNEL32.dll!CloseHandle`, `KERNEL32.dll!CreateFileA`, `KERNEL32.dll!GetLocalTime`, `KERNEL32.dll!SetFilePointer` |
| 3 | 0x0179 | Op0179 | 0x0050E6A0 | 4 | `KERNEL32.dll!InterlockedDecrement` |
| 4 | 0x014E | Op014E | 0x0050EF90 | 3 | `KERNEL32.dll!InterlockedDecrement`, `KERNEL32.dll!InterlockedIncrement` |
| 5 | 0x0147 | Op0147 | 0x005023D0 | 2 | `KERNEL32.dll!lstrcatA`, `KERNEL32.dll!lstrcpyA` |
| 6 | 0x0148 | Op0148 | 0x00502500 | 2 | `KERNEL32.dll!lstrcatA`, `KERNEL32.dll!lstrcpyA` |
| 7 | 0x0120 | Op0120 | 0x00510C70 | 1 | `KERNEL32.dll!InterlockedDecrement` |
| 8 | 0x101D | RemoveSpell | 0x00510B90 | 1 | `KERNEL32.dll!InterlockedDecrement` |

（全 8 ハンドラが IAT 経由呼出を持つ。単位は呼出回数、`x N` は同一 API の重複回数）

重複内訳:

- 0x013A Op013A (0x00515330): `KERNEL32.dll!InterlockedDecrement` x6, `KERNEL32.dll!InterlockedIncrement` x3
- 0x012D Op012D (0x0050A400): `KERNEL32.dll!CreateFileA`, `KERNEL32.dll!SetFilePointer`, `KERNEL32.dll!GetLocalTime`, `KERNEL32.dll!CloseHandle`
- 0x0179 Op0179 (0x0050E6A0): `KERNEL32.dll!InterlockedDecrement` x4
- 0x014E Op014E (0x0050EF90): `KERNEL32.dll!InterlockedIncrement` x2, `KERNEL32.dll!InterlockedDecrement`
- 0x0147 Op0147 (0x005023D0): `KERNEL32.dll!lstrcpyA`, `KERNEL32.dll!lstrcatA`
- 0x0148 Op0148 (0x00502500): `KERNEL32.dll!lstrcpyA`, `KERNEL32.dll!lstrcatA`
- 0x0120 Op0120 (0x00510C70): `KERNEL32.dll!InterlockedDecrement`
- 0x101D RemoveSpell (0x00510B90): `KERNEL32.dll!InterlockedDecrement`

## 5. API 頻度上位 20（全 26 呼出 / distinct 8 API）

| 順位 | DLL!関数 | 呼出数 |
|------|----------|--------|
| 1 | `KERNEL32.dll!InterlockedDecrement` | 13 |
| 2 | `KERNEL32.dll!InterlockedIncrement` | 5 |
| 3 | `KERNEL32.dll!lstrcpyA` | 2 |
| 4 | `KERNEL32.dll!lstrcatA` | 2 |
| 5 | `KERNEL32.dll!CreateFileA` | 1 |
| 6 | `KERNEL32.dll!SetFilePointer` | 1 |
| 7 | `KERNEL32.dll!GetLocalTime` | 1 |
| 8 | `KERNEL32.dll!CloseHandle` | 1 |

### カテゴリ別集計（呼出回数 / distinct API）

| カテゴリ | 呼出数 | distinct API |
|----------|--------|--------------|
| kernel32 | 26 | 8 |

user32 / ole32 / gdi32 / その他 は **0 件**（本 26 件の呼出は全て kernel32 に集中）。

## 6. 移植上の示唆（API 名に基づく事実の列挙・カテゴリ分類のみ）

カテゴリ件数（26 件中の呼出数 / distinct API）: **kernel32 = 26 / 8、user32 = 0 / 0、ole32 = 0 / 0、gdi32 = 0 / 0、其他 = 0 / 0**。

補足（インポートテーブル全体の事実）: 本 exe は USER32.dll を 25 スロット、ole32.dll を 3 スロット、GDI32.dll を 1 スロットインポートしているが、これらを経由する呼出は台帳 480 ハンドラの間接呼出 406 件（うち call [abs32] 26 件）には含まれない。他のコード経路（ハンドラ外または直接 E8 呼出）は本タスクの対象外。

### kernel32（26 呼出 / 8 API）

- `KERNEL32.dll!CloseHandle`
- `KERNEL32.dll!CreateFileA`
- `KERNEL32.dll!GetLocalTime`
- `KERNEL32.dll!InterlockedDecrement`
- `KERNEL32.dll!InterlockedIncrement`
- `KERNEL32.dll!SetFilePointer`
- `KERNEL32.dll!lstrcatA`
- `KERNEL32.dll!lstrcpyA`

分類はインポート先 DLL 名に基づく。API 名から読み取れる事実のみ記載し、実装方式の推測は行っていない。
本 26 件に現れた API は上記 kernel32 の 8 種のみで、未知・未確認（要調査）の API は 0 件。
user32 / ole32 / gdi32 / 其他（WSOCK32, DSOUND, WINMM, d3dx9_27, binkw32, DINPUT8, COMCTL32, ADVAPI32, SHELL32, VERSION）は本 26 件に出現せず（0 件）、これらを含む呼び出しの移植要否は本タスクの範囲外（要調査）。
