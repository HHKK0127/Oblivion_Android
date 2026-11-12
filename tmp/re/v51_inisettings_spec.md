# V51 — INISettings 設定抽象 移植仕様書

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, Steam)
- 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- マッピング (ground truth): ImageBase=0x400000 / .text RVA=0x1000 Raw=0x400 → `VA = 0x400C00 + file_off` / .rdata VA=0xA28000 Raw=0x627200 (VA = file_off + 0x400E00) / .data VA=0xB02000 Raw=0x700C00 (VA = file_off + 0x401400, SizeOfRawData=0x30C00 → 読取可能域 VA 0xB02000..0xB32C00)
- 解析方法: PE は `struct` 自前解析、逆アセンブルは capstone 5.0.7、decompiled.c (`docs\Oblivion_decompiled.c`, 1,066,496行) は lowercased ラベル照合 + `enumerate(f,1)` 1パス。全ランは read-only の `python -c` 単行（既存ドキュメント無編集、本ファイル新規のみ）。
- 前提: `tmp\re\v13_saveinifiles_basetype.md` (V13) は信頼可。本仕様は V13 を再検証しつつ **1件訂正**（§5 SaveAll の呼出先）+ **2インスタンス追加**（§3）を行う。

---

## 1. セルフテスト 4 件（全 PASS・実測値）

| # | 検査 | file_off | 期待値 | 実測値 | 結果 |
|---|------|----------|--------|--------|------|
| ① | `bytes@0x504050` (8B) | 0x103450 | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| ② | `bytes@0x4FB580` (4B) | 0xFA980 | `578b7908` | `578b7908` | PASS |
| ③ | `string@0xA7035C` | 0x66F55C | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| ④ | オペコードテーブル idx=21 entry `0xB0B290` +0x18 impl | 0x709E90 | `0x504560` | `0x504560` | PASS |

- ④ entry 全体（`<10I`）: `[0xa51778, 0xa2f7ec, 0x15, 0xa2f7ec, 0x1, 0x0, 0x504560, 0x4fde30, 0x0, 0x0]` → entry レイアウト確定: **+0x00 name / +0x04 shortname / +0x08 opid / +0x0C help / +0x10 flags / +0x14 / +0x18 impl / +0x1C compiler helper**（stride 0x28、基底 0xB0AF48、532件）。
- opid 0x0165 entry `0xB0C3E8` 同様に実測: `[0xa502a0, 0xa50298, 0x165, 0xa5027c, 0, 0, 0x513b60, 0x4fde30, 0, 0]` → impl `0x513B60` 再確認。
- 関連 opid 実測: 0x011C `0x514100`(SetGameSetting) / 0x011D `0x5143C0`(SetINISetting) / 0x011E `0x514230`(GetINISetting) / 0x013E `0x513BB0`(RefreshINI) / 0x0165 `0x513B60`(SaveIniFiles)。

---

## 2. vtable 全スロット表（共有/差分明示）

### 2.1 vtable `0xA404F8` — INI バックエンド（実体 `0xB07BF0`、.rdata 実読 9 ダワ + 隣接文字列）

| スロット | 実装 VA | シグネチャ | 役割 |
|---|---|---|---|
| +0x00 | `0x4040E0` | `void AddSetting(entry* /*stack4*/)` thiscall ret4 | +0x10C リストへ登録。同名(stricmp)重複なら警告 `0x4A7A60`(fmt 0xA2FAB0) して挿入せず、なければ `0x446CB0` で先頭 prepend |
| +0x04 | `0x403C20` | thunk (`add ecx,0x10C; jmp 0x65C620`, 6B) | 設定エントリリスト破棄（dtor 系） |
| +0x08 | `0x4A8B10` | `int Dump(printCtx*)` thiscall ret4 | コンソール一覧: ヘッダ `'INISettings'`(0xA40580) を `0x70F910` 出力 → リスト走査し各 entry を `0x4A7B10` 表示 → `push count; push 'Total INISettings'`(0xA4056C) を `0x70FB40` 出力、count を戻す |
| +0x0C | `0x4A85F0` | `void WriteOne(entry*)` thiscall (decompile L134459) | **永続化1件**: 型別 sprintf → `WritePrivateProfileStringA(section, key, value, file=this+4)`。非セッション時は自前 Begin(1)/End で挟む |
| +0x10 | `0x4A8800` | `void ReadOne(entry*)` thiscall (decompile L134525) | **読込1件**: 型別に `GetPrivateProfileIntA`/`GetPrivateProfileStringA`（file=this+4、既定値は entry 現在値 or 既定値オブジェクト）→ entry へ書戻し |
| +0x14 | `0x4A83B0` | `bool Begin(u32 flag)` thiscall ret4 (decompile L134339) | `mov [ecx+0x108], ecx; mov al,1; ret 4` — **flag は無視**、セッション開始 = [+0x108] に this を格納 |
| +0x18 | `0x4A7CB0` | `bool End()` thiscall ret0 (decompile L133888) | `mov [ecx+0x108], 0; mov al,1; ret` — セッション終了 |
| +0x1C | `0x4A8560` | `bool SaveAll()` thiscall ret0 | [+0x108]!=0 のとき +0x10C リストを走査し **backend 自身の vt+0x0C (WriteOne) を thiscall**、結果を AND |
| +0x20 | `0x4A85A0` | `bool LoadAll()` thiscall ret0 | 同リスト走査し **backend 自身の vt+0x10 (ReadOne) を thiscall**、結果を AND |
| +0x24 以降 | — | 非スロット | `.rdata` 文字列: 0xA4051C `"%u"` / 0xA40520 `"%u,%u,%u,%u"` / 0xA4052C `"%u,%u,%u"` / 0xA40538 `"%.4f"` / 0xA40540 `"%f"` |

### 2.2 vtable `0xA40590` — レジストリ バックエンド（実体 `0xB07D28`、.rdata 実読 9 ダワ + 隣接文字列）

| スロット | 実装 VA | シグネチャ | 役割 |
|---|---|---|---|
| +0x00 | `0x4040E0` | **INI と同一** | AddSetting（backend 非依存） |
| +0x04 | `0x403C20` | **INI と同一** | リスト破棄 thunk |
| +0x08 | `0x4A8E50` | `int Dump(printCtx*)` thiscall ret4 | `'RegSettings'`(0xA405C8, push @0x4A8E55) / `'Total Reg Settings'`(0xA405B4, push @0x4A8EB4) を 0x70F910/0x70FB40 出力（以下 INI Dump と同型） |
| +0x0C | `0x4A8BC0` | `void WriteOne(entry*)` thiscall (decompile L134626) | `RegSetValueExA(hkey=[this+0x108], valuename, 0, dwType=型id, data=entry(先頭4B値), size=0x4A7A80(entry))`。型id==1('c') のとき名前を複写して先頭文字を `'s'` に変更（REG_SZ 相当に格納）。自動 Begin(1)/End |
| +0x10 | `0x4A8D20` | `void ReadOne(entry*)` thiscall | 自動 Begin(0)（無 hkey 時）→ `RegQueryValueExA`（文字列型は名前先頭 's' 化 + 0x104B バッファ、他は entry 直接 + size=4）@0x4A8DF4 |
| +0x14 | `0x4A8C90` | `bool Begin(u32 flag)` thiscall ret4 | **flag!=0**: `RegCreateKeyExA(HKCU=0x80000002, subkey=this+4, 0,0,0, 0xF003F, 0, &this+0x108, &disp)` / **flag==0**: `RegOpenKeyExA(HKCU, this+4, 0, 0xF003F, &this+0x108)`。`sete al` で成功返却 |
| +0x18 | `0x4A8CF0` | `bool End()` thiscall (decompile L134677) | hkey!=0 なら `RegCloseKey` → [+0x108]=0、`LVar1==0` を返す（hkey 未使用時は true） |
| +0x1C | `0x4A8560` | **INI と同一** | SaveAll |
| +0x20 | `0x4A85A0` | **INI と同一** | LoadAll |
| +0x24 以降 | — | 非スロット | 文字列 0xA405B4 `"Total Reg Settings"` / 0xA405C8 `"RegSettings"` |

### 2.3 突合サマリ（vtable 9 スロット）

- **共有4スロット（同一実装＝共通処理）**: +0x00 AddSetting / +0x04 list-dtor / +0x1C SaveAll / +0x20 LoadAll — 保存・読込ループ自体はバックエンド非依存で、差分は Read/Write/Begin/End/Dump に集約される。
- **差分5スロット（バックエンド差分）**: +0x08 Dump / +0x0C WriteOne / +0x10 ReadOne / +0x14 Begin / +0x18 End。
- オブジェクトレイアウト（全インスタンス共通）: `+0x00` vptr / `+0x04` ターゲット文字列バッファ（INI=ファイルパス、Registry=レジストリサブキー。既知書込元は全て this+4 へ NUL 終端コピー） / `+0x108` セッション状態（INI=**this 自身 or 0**、Registry=**HKEY or 0**） / `+0x10C` 先頭 entry ポインタ / `+0x110` ノードチェーン先頭。サイズ ≥ 0x114。
- `.data` 初期値実読: `[0xB07BF0]=0xA404F8`、`[0xB07D28]=0xA40590`（いずれも静的初期化のみ、実行時書き込みゼロ）。

---

## 3. 第3 / 第4 インスタンス

### 3.1 `0xB11C40` — BlendSettings.ini バックエンド（第3、稼働）

- vptr 実読 = **`0xA566A4`**、スロットは **INI 0xA404F8 と9スロット完全一致**（0x4040E0 / 0x403C20 / 0x4A8B10 / 0x4A85F0 / 0x4A8800 / 0x4A83B0 / 0x4A7CB0 / 0x4A8560 / 0x4A85A0）。
- 登録エントリ **62件**（AddSetting 呼出の ecx センサスで確定）。サンプル実読: entry `0xB11A24` name@0xA566D8 = `"fGetUpTime:HIT"`（アニメーションブレンド設定、セクション "HIT" 等）。
- ファイルパス: ディレクトリ `[0xB3F280]` + `[0xB11A20]` → ptr `0xA56684` = `"BlendSettings.ini"`（this+4 へコピー @0x53ACF0-0x53ACFC）。
- 起動時読込: 呼出元は **`0x40E759 call 0x53AC60` の1本のみ**（INI バックエンド初期化の直後）。`0x53AC60` 内 = パス構築 → `Begin(0)` (0x53AD00-0x53AD10) → `LoadAll` (0x53AD16-0x53AD24) → `End` (0x53AD26-0x53AD34) → `0x53A1B0/0x53A460/0x53A720` 後処理 → ret @0x53AD59。
- 別関数 `0x53AD60`（呼出元は起動部 `0x40F5A3` の1本のみ）: `sprintf(buf, "%d"(0xA2F860), 0xE=14)` → `WritePrivateProfileStringA("DEFAULT"(0xA566C8), "VERSION"(0xA566D0), "14", "BlendSettings.ini")` → ディレクトリ+ファイル名をスタックへ構築（以降の線形走査は未解読 = §9）。
- 破棄: CRT dtor thunk `0xA1D241`。

### 3.2 `0xB07AD8` — 死蔵インスタンス（第4、未使用）

- vptr 実読 = **`0xA40548`**、スロットも INI と完全一致（9スロット同一）。+4 バッファは .data 上全ゼロ。
- `.text` 中の imm32 参照は **1箇所のみ**: `0xA1AA21 mov ecx, 0xb07ad8; jmp 0x4a84e0`（CRT 終了時 dtor thunk）。**Begin/LoadAll/登録（AddSetting）はゼロ**。
- 結論: リンク済みだが一度も使われないゴースト実体。移植時は削除してよい。

### 3.3 AddSetting 呼出センサス（全637件、E8→0x4040E0 全走査）

| 対象 (ecx) | 件数 | 内訳 |
|---|---|---|
| `0xB07BF0` (INI) | **574** | 静的初期化 572 + ランタイム 2（`0x404893`, `0x4440A3`=登録系関数 0x444080 内） |
| `0xB11C40` (Blend) | **62** | CRT 静的初期化 `0x9E44xx`-`0x9E5Dxx` |
| `0xB07D28` (Registry) | **1** | `0x9DAC71` push `0xB0472C`（entry name@0xA36448 = `"sBasePath"`） |

---

## 4. INI バックエンド実装深掘り（decompiled 引用つき）

### 4.1 WriteOne `0x4A85F0`（vt+0x0C、永続化の実体）

```c
// docs\Oblivion_decompiled.c L134459  FUN_004a85f0(int *param_1 /*this*/, float *param_2 /*entry*/)
if (param_2[1] == 0.0) goto LAB_004a87b8;          // entry->name == NULL → skip
local_195 = param_1[0x42] == 0;                     // param_1[0x42] = [this+0x108] セッション状態
if ((bool)local_195) { (**(code **)(*param_1 + 0x14))(); }   // 自動 Begin（非セッション時）
if (param_1[0x42] == 0) goto LAB_004a87a8;          // Begin 失敗なら書込スキップ
uVar1 = FUN_004a78d0();                             // 型 = GetType(entry->name)（先頭文字判定, §5.3）
FUN_004a8430(param_2, aCStack_184);                 // section 64B バッファ（':' 以降 or "MAIN"）
FUN_004a8480(param_2, aCStack_144);                 // key 64B バッファ（':' 以前 or 全体）
switch(uVar1) {
case 1: case 3:  _sprintf(acStack_104, "%d");  goto LAB_004a86a9;
default:          _sprintf(acStack_104, "%u");  break;            // 0/2/4/9
case 5:           _sprintf(acStack_104, "%.4f", (double)*param_2); break;
case 6:           lpString = (char *)*param_2; goto LAB_004a879d; // 文字列は生ポインタ直渡し
case 7:           FUN_004a83f0(*param_2, &uStack_194, &uStack_190, &uStack_18c);
                  _sprintf(acStack_104, "%u,%u,%u", ...); goto LAB_004a86a9;
case 8:           FUN_004a83f0(*param_2, &uStack_188, &uStack_18c, &uStack_190);
                  _sprintf(acStack_104, "%u,%u,%u,%u", ...);
}
LAB_004a879d:
WritePrivateProfileStringA(aCStack_184, aCStack_144, lpString, (LPCSTR)(param_1 + 1));  // file = this+4
LAB_004a87a8:
if (local_195 != '\0') { (**(code **)(*param_1 + 0x18))(); }      // 自動 Begin した場合のみ End
```

- **ファイル名取得**: `(LPCSTR)(param_1 + 1)` = `this+4` の生バッファ（int 配列なので +4 バイト）。
- **引数処理**: section/key は entry->name を `':'`(0x3A) で分割（4.4）。値は型別に書式化し 256B スタックバッファ `acStack_104` へ。
- **リトライ/長さ制限**: **リトライなし**（1回呼び出し、戻り値も無視）。制限はバッファ実長のみ（value 256B / section 64B / key 64B）。security cookie (0xB30AAC) 検査あり。
- プロローグ: `sub esp, 0x198`（スタック消費 0x198）。

### 4.2 ReadOne `0x4A8800`（vt+0x10、読込の実体）

```c
// L134525  FUN_004a8800(int *param_1 /*this*/, uint *param_2 /*entry*/)
if (param_2[1] == 0) goto LAB_004a8ac6;                       // entry->name NULL → skip
local_695 = param_1[0x42] == 0;
if ((bool)local_695) { (**(code **)(*param_1 + 0x14))(1); }    // 自動 Begin(1)
if (param_1[0x42] == 0) goto LAB_004a8ab6;
uVar1 = FUN_004a78d0();  FUN_004a8430(param_2, aCStack_644 /*section*/);
                         FUN_004a8480(param_2, aCStack_684 /*key*/);
switch(uVar1) {
case 0:  pcVar7 = (char *)FUN_00404df0();                     // bool 既定値オブジェクト
         UVar3 = GetPrivateProfileIntA(sec, key, (*pcVar7 != '\0'), (LPCSTR)(param_1 + 1));
         *(bool *)param_2 = UVar3 != 0; break;
case 1:  pIVar2 = (INT *)FUN_00403be0();                      // int 既定値
         UVar3 = GetPrivateProfileIntA(sec, key, *pIVar2, ...);
         *(char *)param_2 = (char)UVar3; break;
default: uVar8 = *param_2;                                    // 現在値を既定値に
case 3:  puVar4 = (uint *)FUN_00403be0(); uVar8 = *puVar4;
         uVar8 = GetPrivateProfileIntA(sec, key, uVar8, ...); *param_2 = uVar8; break;
case 5:  pfVar5 = (float *)FUN_00403c00();                    // float 既定値
         _sprintf(acStack_504, "%f", (double)*pfVar5);
         DVar6 = GetPrivateProfileStringA(sec, key, acStack_504, aCStack_604, 0x100, ...);
         if (DVar6 != 0) { sscanf(aCStack_604, "%f"); } break;
case 6:  GetPrivateProfileStringA(sec, key, (LPCSTR)*param_2, aCStack_404, 0x100, ...);
         FUN_004a7980(); break;                               // entry の文字列へ Set
case 7:  DVar6 = GetPrivateProfileStringA(sec, key, "", aCStack_604, 0x100, ...);
         if (DVar6 == 0) break;
         sscanf(aCStack_604, "%u,%u,%u", ...);
         uVar8 = ((uStack_694 & 0xff | iStack_690 << 8) << 8 | uStack_68c & 0xff) << 8 | 0xff; break;
case 8:  ... sscanf(aCStack_604, "%u,%u,%u,%u", ...); uVar8 = FUN_004a83c0(...); break;
}
LAB_004a8ab6: if (local_695 != '\0') { (**(code **)(*param_1 + 0x18))(); }   // 自動 End
```

- **ファイル名**: 同じく `(LPCSTR)(param_1 + 1)` = this+4。
- **長さ制限**: `nSize=0x100` (256B) が全 String 読込（スタックは `sub esp, 0x698`、value バッファ 256B / 文字列 1024B 領域だが実効 256B）。
- **キー無し時の挙動**: GetPrivateProfile* は既定値（第三引数）を返すため entry は不変（bool は `!=0` 正規化のみ）。
- **リトライなし**。Begin 失敗時は一切読まない。自動 Begin は flag=1（書込モード相当の Begin、INI では flag 意味なし）。

### 4.3 AddSetting `0x4040E0`（vt+0x00、登録）

```c
// L3249  void __thiscall FUN_004040e0(int param_1 /*this*/, int param_2 /*entry*/)
piVar1 = (int *)(param_1 + 0x10c);
if ((*(int *)(param_1 + 0x110) != 0) || (*piVar1 != 0)) {
  for (; piVar1 != (int *)0x0; piVar1 = (int *)piVar1[1]) {        // ウォーク: &this+0x10C → [+0x110] → node->next
    iVar2 = __stricmp(*(char **)(*piVar1 + 4), *(char **)(param_2 + 4));   // 同名判定 (0x98262D = _stricmp)
    if (iVar2 == 0) {
      FUN_004a7a60("Setting key '%s' already used in list.\nSetting keys must be unique.\n",
                   *(undefined4 *)(param_2 + 4));                 // fmt @0xA2FAB0
      return;                                                     // 挿入しない
    }
  }
}
FUN_00446cb0(param_2);                                            // 挿入（ecx = &this+0x10C で呼び出し）
```

- 戻りは void、引数 entry はスタック（`ret 4`）。重複名は警告のみで登録拒否（破棄はしない）。

### 4.4 セクション/キー分割ヘルパ（WriteOne/ReadOne 共用）

- `0x4A8430(entry, outSec64)`: entry->name に `strchr(name, ':')`(0x982EC0)。**見つかったら ':' の後ろ = section**、無ければ既定 **`"MAIN"`(0xA404C4)**。out へ strcpy。
- `0x4A8480(entry, outKey64)`: 同様に ':' 探索。**見つかったら ':' の前まで = key**（`sub eax,esi` で長さ算出）、無ければ name 全体。
- つまりエントリ名は **`"key:Section"` 形式**（実測: `"bUse Joystick:Controls"`@0xA2FB18、`"bBackground Keyboard:Controls"`@0xA2FB30、`"sBasePath"`（無コロン → section="MAIN"）、`"fGetUpTime:HIT"`@0xA566D8）。

### 4.5 セッション制御と補助

```asm
; 0x4A83B0 Begin (vt+0x14)                      ; 0x4A7CB0 End (vt+0x18)
mov dword ptr [ecx + 0x108], ecx                mov dword ptr [ecx + 0x108], 0
mov al, 1                                       mov al, 1
ret 4                                           ret
```
- `0x4040A0 Open(this, path, flag)` ret8: path!=NULL なら this+4 へ NUL 終端コピー → `vt+0x14(flag)`。path==NULL なら this+4 を 0 化 → `vt+0x14(flag)`。
- `0x40C180 SavePath(this, path)` ret4: `Open(path, 1)` → 成功時 `vt+0x1C (SaveAll)` → `vt+0x18 (End)`、SaveAll 結果を返す。
- `0x4A84E0`（CRT dtor 用共通 dtor、thunk `0xA1AA20/21/30/35` から jmp）: vptr を 0xA404D0 → **[this+0x110] のノードチェーンを free(0x401F20) で解放**（node+4 を next として走査）→ [+0x10C]=0 → vptr を 0xA403C8。
- IAT 実解決（IMAGE_IMPORT_BY_NAME 照合）: `[0xA280B0]=WritePrivateProfileStringA` / `[0xA280B4]=GetPrivateProfileStringA` / `[0xA280E4]=GetPrivateProfileIntA` / `[0xA28000]=RegSetValueExA` / `[0xA28004]=RegOpenKeyExA` / `[0xA28008]=RegCreateKeyExA` / `[0xA2800C]=RegCloseKey` / `[0xA28010]=RegQueryValueExA` / `[0xA280E0]=CopyFileA` / `[0xA280AC]=DeleteFileA`。

---

## 5. 登録構造（+0x10C/+0x110、エントリ型、SaveAll 訂正）

### 5.1 リスト構造（V13 からの精緻化）

```
SettingsObject
  +0x10C : entry*   ← 先頭 entry を直接格納（インライン head）
  +0x110 : node*    ← heap ノード連鎖の先頭
node (8B, operator new 0x401F00 で確保)
  +0x00 : entry*
  +0x04 : next node*
```

挿入 `0x446CB0(&head, entry)` の decompiled 引用（L54233）:

```c
void __thiscall FUN_00446cb0(int *param_1 /*&obj+0x10C*/, int param_2 /*entry*/)
{
  if (param_2 != 0) {
    if (*param_1 != 0) {                       // 既に1件以上ある
      piVar1 = (int *)FUN_00401f00(8);         // new(8)
      if (piVar1 != (int *)0x0) {
        *piVar1 = *param_1;                    // node->entry = 現行 head entry
        piVar1[1] = param_1[1];                // node->next  = 現行 head.next
        *param_1 = param_2;                    // head entry  = 新 entry
        param_1[1] = (int)piVar1;              // head.next   = 新 node
        return;
      }
      _DAT_00000004 = param_1[1];              // OOM フォールバック（decompiler 成果物の可能性, §9）
      param_1[1] = 0;
    }
    *param_1 = param_2;                        // 空リスト: head entry に直接
  }
}
```

- **先頭 prepend** のため **走査順は登録の逆順**（新しいものが先頭）。Walk パターン（AddSetting / SaveAll / LoadAll / Dump 共通）: `esi = &obj+0x10C; loop { entry = [esi]; ...; esi = [esi+4]; }` → 1回目で head entry、2回目以降は node 連鎖。
- dtor は `[+0x110]` 連鎖のみ解放し `[+0x10C]` を 0 化（§4.5）。

### 5.2 エントリ型（設定記録）

- **8B 静的レコード `{ u32/float/char* value @+0x00 , char* name @+0x04 }`**（.data に静的確保。実測例: `0xB02C2C = {1, "bUse Joystick:Controls"}`、`0xB02C34 = {0, "bBackground Keyboard:Controls"}`、stride 8 で連続配置）。
- name = `"key:Section"`（§4.4）。value は型により union（bool/char/int/uint/float/pack4/文字列ポインタ）。
- 型判定 `0x4A78D0(name)` — `name[0] - 'S'(0x53)` をインデクスにバイト表 `0x4A7954` → ケース、ジャンプ表 `0x4A792C`（既定ケースは入口で `mov eax,9` 済みのため `ret` のみ）:

| 先頭文字 | case | type id | WriteOne 書式 | ReadOne |
|---|---|---|---|---|
| `s`/`S` | 0 | **6** string | 生ポインタ直渡し | GetProfileString(1024領域/256実効) → Set(0x4A7980) |
| `a` | 1 | **8** 4×uint | `"%u,%u,%u,%u"` | GetProfileString→sscanf→pack(0x4A83C0) |
| `b` | 2 | **0** bool | default `"%u"` | GetInt(既定=0x404DF0) → `!=0` |
| `c` | 3 | **1** char | `"%d"` | GetInt(既定=0x403BE0) → byte |
| `f` | 4 | **5** float | `"%.4f"` | sprintf"%f" 既定→GetString→sscanf"%f" |
| `h` | 5 | **2** | `"%u"` | GetInt(現在値既定) |
| `i` | 6 | **3** int | `"%d"` | GetInt(既定=0x403BE0) |
| `r` | 7 | **7** 3×uint | `"%u,%u,%u"` | sscanf"%u,%u,%u" → pack(下位バイト0xFF) |
| `u` | 8 | **4** | `"%u"` | GetInt(現在値既定) |
| その他 | 9 | **9** | `"%u"` | GetInt(現在値既定) |
- pack/ unpack: `0x4A83F0` = u32 → 4バイト（shr24 / shr16&FF / AH / AL）、`0x4A83C0` = 逆。

### 5.3 ★ SaveAll 呼出先の訂正（V13 に対する）

V13 §6 は SaveAll を「各 entry へ vt+0x0C を呼び出し」と記載したが、逆アセンブル実測は異なる:

```asm
; 0x4A8560 bool SaveAll()  — thiscall、引数なし
0x4a8560: push ebx / push edi / mov edi, ecx
0x4a8564: cmp dword ptr [edi + 0x108], 0        ; セッション中か？
0x4a856b: setne bl / test bl, bl / je 0x4a859b  ; 非セッション → false 返却
0x4a8572: push esi / lea esi, [edi + 0x10c]     ; esi = &head entry フィールド
0x4a8580: mov eax, dword ptr [esi]              ;   eax = entry
          mov edx, dword ptr [edi]              ;   edx = this->vtable
          push eax                              ;   arg = entry（スタック引数1個）
          mov eax, dword ptr [edx + 0xc]        ;   eax = this->vt+0x0C = WriteOne
          mov ecx, edi                          ;   this
          call eax                              ;   ★ WriteOne(this, entry)
          mov esi, dword ptr [esi + 4]          ;   次へ（head.next → node->next）
          and bl, al                            ;   結果 AND
          test esi, esi / jne 0x4a8580
          pop esi / pop edi / mov al, bl / pop ebx / ret
```

- **訂正**: 呼ばれるのは **エントリの仮想関数ではなく、backend オブジェクト自身の vt+0x0C（= INI なら `0x4A85F0` WriteOne、Registry なら `0x4A8BC0`）**。entry はただのデータ引数。エントリ側に vtable は無い（8B レコード §5.2）。
- LoadAll `0x4A85A0` も同一構造で `vt+0x10`（ReadOne）を呼ぶ。
- セッション非活性時は SaveAll/LoadAll は **false を返して何もしない**（Begin は呼び出し側の責務）。

### 5.4 エントリ登録経路

- 静的初期化: MSVC の CRT 初期化関数群（`.CRT` 節、0x9D7C00-0xA1F000 帯）が1件ずつ `push <entry addr>; mov ecx, <backend>; call 0x4040E0` を実行（SEH フレーム + `_onexit(0x981FB4)` 付き）。例 `0x9D8040`: entry `0xB02C2C` → INI バックエンド。
- ランタイム登録: `0x444080`（ecx=0xB07BF0 で `0x4040E0` 呼出 @0x4440A3）と `0x404893` の2系統。
- 逆転查找（FindSetting）: `0x50EC30(this, name, out**)` = +0x10C リストを `_stricmp`(0x98262D) で走査。**呼出元は `0x5142B4`(GetINISetting) と `0x51444F`(SetINISetting) の2件のみ、両方 `mov ecx, 0xb07bf0` ハードコード**（E8 全走査で確定）。

---

## 6. SaveIniFiles / RefreshINI 本文と呼出順

### 6.1 SaveIniFiles — opid 0x0165 → entry `0xB0C3E8` +0x18 → impl `0x513B60`

呼び出し経路: オペコードテーブル経由のみ（impl `0x513B60` への直接 E8 = 0件、テーブル基底参照はスクリプトコンパイラ `0x4FF904` の1箇所 = V13 と再確認一致）。console 名 `saveini`(0xA50298) / help `"Writes all the .ini files."`(0xA5027C)。

```asm
0x00513B60: mov ecx, 0xb07bf4                ; this+4 パスバッファ先頭
0x00513B65: mov al, byte ptr [ecx]           ; NUL まで読戻し付き走査（strlen 相当の_touch）
0x00513B67: mov byte ptr [ecx], al
0x00513B69: add ecx, 1
0x00513B6C: test al, al
0x00513B6E: jne 0x513b65
0x00513B70: mov eax, dword ptr [0xb07bf0]    ; vptr
0x00513B75: mov edx, dword ptr [eax + 0x14]  ; vt+0x14 Begin
0x00513B78: push 1                           ; flag = 1
0x00513B7A: mov ecx, 0xb07bf0                ; this = g_INI
0x00513B7F: call edx                         ; *** Begin(1) ***
0x00513B81: test al, al
0x00513B83: je 0x513ba3                      ; 失敗 → SaveAll/End 省略
0x00513B85: mov eax, dword ptr [0xb07bf0]
0x00513B8A: mov edx, dword ptr [eax + 0x1c]  ; vt+0x1C SaveAll
0x00513B8D: mov ecx, 0xb07bf0
0x00513B92: call edx                         ; *** SaveAll ***（全設定を1つの ini へ）
0x00513B94: mov eax, dword ptr [0xb07bf0]
0x00513B99: mov edx, dword ptr [eax + 0x18]  ; vt+0x18 End
0x00513B9C: mov ecx, 0xb07bf0
0x00513BA1: call edx                         ; *** End ***
0x00513BA3: mov al, 1                        ; 常に true
0x00513BA5: ret
```

**呼出順（1行）**: `パス走査 → vt+0x14 Begin(1) → [成功時のみ] vt+0x1C SaveAll（backend WriteOne で全 entry 書出）→ vt+0x18 End → al=1 返却`。

### 6.2 RefreshINI — opid 0x013E → entry `0xB0BDD0` +0x18 → impl `0x513BB0`

```asm
0x513BB0: sub esp, 0x108 ... security cookie
0x513BC4: mov eax, dword ptr [0xb02c84]      ; "Oblivion.ini" (0xA2FBA4) ポインタ
0x513BC9: test eax, eax / je 0x513c63        ; NULL でもメッセージへ
0x513BD3: copy [0xb3f280] → stack            ; ディレクトリ文字列（bss、起動時に構築）
0x513C0A..: strcat [0xb02c84] を stack 末尾へ（rep movsd/movsb）
0x513C20: copy stack → 0xb07bf4              ; this+4 へフルパス書込み
0x513C30: mov eax,[0xb07bf0] / mov edx,[eax+0x14] / push 0 / mov ecx,0xb07bf0 / call edx
                                                ; *** Begin(0) ***
0x513C41: test al, al / je 0x513c63
0x513C45: ... vt+0x20 call                    ; *** LoadAll ***
0x513C54: ... vt+0x18 call                    ; *** End ***
0x513C63: push 0xa4d690 / call 0x579b60       ; *** メッセージ出力 ***
0x513C79: mov al, 1 / ... ret
```

- メッセージ文字列実読: `"The in-game settings have been refreshed from the Oblivion.ini file."` @0xA4D690。
- 出力先: **`0x579B60` = 可変引数コンソールプリンタ**（decompiled で `FUN_00579b60("...")` 形式の呼び出しが100箇所以上 = ゲーム内コンソール/ログ出力。`0x70FB40`/`0x70F910` も同系統）。
- **メッセージは無条件に出力される**（Begin 失敗・ini ポインタ NULL でも 0x513C63 へ合流）。
- 呼出順: `パス構築(dir+"Oblivion.ini") → this+4 へ書込 → Begin(0) → [成功時] LoadAll → End → コンソールへ refreshed メッセージ`。
- 対応関係: SaveIniFiles = `Begin(1)→SaveAll→End`（書出）、RefreshINI = `Begin(0)→LoadAll→End`（再読）。

### 6.3 周辺の呼出元（本解析で確認）

- `0x40C180`（Open+SaveAll+End ラッパ）の既知呼出: **起動部 `0x40F58E push 0xb07bf4 / mov ecx,0xb07bf0 / call 0x40c180`** = 起動完了時に Oblivion.ini を全量再生成する自動書出し。
- GetINISetting `0x514230`: `0x5142A7 mov ecx,0xb07bf0; call 0x50ec30`（FindSetting）→ `0x4A78D0(entry+4)` 型判定 → 書式 `"INISetting %s >> %i/%.2f/'%s'"` (0xA4D7FC/0xA4D7E4/0xA4D7CC)。
- Controls 保存/読込系 `0x404470`/`0x4045B0` も this+4 へパス書込み後、同一 vtable 呼出（V13 由来、本解析の参照走査で整合）。

---

## 7. バックエンド切替条件（証拠つき判定）

**結論: 実行時のバックエンド切替は存在しない。** 選択は「どのオブジェクトに登録し、どのオブジェクトに対して操作をハードコードするか」で静的に確定している。

1. **静的4インスタンス**（§2, §3）: すべて `.data` に静的に vptr が初期化され、実行時の代入はゼロ（imm32 参照の全分類で書き込みなし）。
2. **起動時初期化は fn `0x40DF80`（Ghidra FUN 欠落）内の `0x40E6C0`-`0x40E759` で無条件順次実行**（分岐は各 Begin の成否のみ）:

```asm
; ① レジストリ バックエンド
0x40E6C0: copy 'Software\Bethesda Softworks\Oblivion' (0xA30FEC) → 0xB07D2C (this+4)
0x40E6D3: vt+0x14 Begin(push ebp /*=0*/)     ; RegOpenKeyExA(HKCU, ...) 開く
0x40E6E4: test al,al / je 0x40e708           ; Begin 失敗 → LoadAll/End 省略
0x40E6E8: vt+0x20 LoadAll                    ; sBasePath 1件を読む
0x40E6F8: vt+0x18 End                        ; RegCloseKey
; ② INI バックエンド
0x40E710: copy stack[esp+0x1b0] → 0xB07BF4   ; フルパス（dir + "Oblivion.ini"）
0x40E724: vt+0x14 Begin(push ebp /*=0*/)
0x40E735: test al,al / je 0x40e771
0x40E739: vt+0x20 LoadAll                    ; 574件を読む
0x40E749: vt+0x18 End
; ③ BlendSettings バックエンド
0x40E759: call 0x53ac60                      ; パス構築 → Begin(0) → LoadAll → End（§3.1）
```

3. **INI パス決定とファイル フォールバック**（同一 fn 内）:
   - ディレクトリ取得 → bss `[0xB3F280]`（失敗時は `".\\"` @0xA3126C を 0x40E116-0x40E134 で書込み）。
   - フルパス = `[0xB3F280]` + `[0xB02C84]`(=`"Oblivion.ini"`)（0x40E13A-0x40E18D）。
   - `fopen` 相当チェック (0x982D71) が -1（ファイル無し）→ **`CopyFileA("Oblivion_default.ini"(0xA31254) → フルパス, bFailIfExists=TRUE)`** @0x40E1C3-0x40E221（IAT 0xA280E0）。→ **ini 無し時は default ini をコピーしてから LoadAll**。これは「ファイル単位のフォールバック」でありバックエンド切替ではない。
4. **読み書き経路は全て INI バックエンド固定**: GetINISetting/SetINISetting の FindSetting 呼出（`0x5142B4`/`0x51444F` の2件のみ）は `mov ecx,0xb07bf0` ハードコード。SaveIniFiles/RefreshINI/Controls 系/起動自動書出しも全て `0xB07BF0`。
5. **レジストリ バックエンドは起動後ただの sBasePath 保持体**（以後は死にコード）:
   - 登録は sBasePath 1件のみ（§3.3）。entry `0xB0472C` の .text 参照は初期化3箇所のみ（`0x9ABF21` SEH データ / `0x9DAC71` 登録 push / `0xA17B71` atexit thunk）で **実行時読取ゼロ**。
   - `0xB07D28` の imm32 参照は8件のみ = 起動 Triplet 6件（0x40E6D5-0x40E702）+ 登録1件（0x9DAC76）+ dtor thunk（0xA1AA31）。**起動後の仮想呼出はゼロ**。
   - FindSetting のレジストリ版は存在しない（呼出元2件は INI 固定）。
   - よって「レジストリ vs INI の選択分岐」に相当するコードは EXE 内に存在しない。指令ライン/INI キーによる切替も該当 imm32/文字列なし。**判定: 切替不能ではなく「そもそも切替機構が無い」**。
6. レジストリ Begin の flag は切替ではなくモード: 起動は flag=0(RegOpenKeyExA)、書出時は flag=1(RegCreateKeyExA) だが、レジストリ側 SaveAll を呼ぶ箇所は存在しない（0xB07D28 参照ゼロの帰結）。

---

## 8. 移植仕様

### 8.1 インターフェース定義（EXE の9スロット vtable に1対1）

```cpp
// EXE: vtable 0xA404F8 / 0xA40590 / 0xA566A4 / 0xA40548 — 全9スロット, thiscall
struct ISettingsBackend {                       // 抽象（+0x00..+0x20）
  virtual void  AddSetting(SettingEntry* e);     // +0x00 = 0x4040E0 (共有実装)
  virtual void  DestroyEntries();               // +0x04 = 0x403C20 (共有実装)
  virtual int   Dump(PrintCtx* ctx);            // +0x08 = 0x4A8B10 / 0x4A8E50 (差分)
  virtual void  WriteOne(SettingEntry* e);      // +0x0C = 0x4A85F0 / 0x4A8BC0 (差分)
  virtual void  ReadOne (SettingEntry* e);      // +0x10 = 0x4A8800 / 0x4A8D20 (差分)
  virtual bool  Begin(uint32_t flag);           // +0x14 = 0x4A83B0 / 0x4A8C90 (差分)
  virtual bool  End();                          // +0x18 = 0x4A7CB0 / 0x4A8CF0 (差分)
  virtual bool  SaveAll();                      // +0x1C = 0x4A8560 (共有実装)
  virtual bool  LoadAll();                      // +0x20 = 0x4A85A0 (共有実装)
};

// 実体レイアウト（全インスタンス共通）
struct SettingsObject /*: ISettingsBackend*/ {
  /*+0x00*/ void*      vptr;
  /*+0x04*/ char       target[0x104];   // INI=ファイルフルパス / Reg=サブキー / Blend=iniパス
  /*+0x108*/ uintptr_t session;         // INI: this (開始時) or 0 / Reg: HKEY or 0
  /*+0x10C*/ SettingEntry* headEntry;   // 先頭 entry（インライン）
  /*+0x110*/ EntryNode*    headNode;    // 8B {entry*, next*} heap 連鎖
};

// エントリ（.data 静的 8B レコード、vtable 無し）
struct SettingEntry { union { uint32_t u; float f; char* s; } value; char* name; }; // name = "key:Section"
```

### 8.2 実装すべきメソッド一覧（VA・意味）

| VA | 意味 | 移植要否 |
|---|---|---|
| 0x4040E0 | AddSetting（stricmp 重複検査 → 0x446CB0 prepend） | 必須（共通） |
| 0x446CB0 | ノード prepend（new 8B / OOM 時フォールバック） | 必須（共通） |
| 0x403C20+0x65C620 | エントリリスト破棄 | 必須（共通） |
| 0x4A83B0 / 0x4A7CB0 | Begin(session=this) / End(session=0) | 必須（共通） |
| 0x4A8560 / 0x4A85A0 | SaveAll / LoadAll（セッション必須、結果 AND） | 必須（共通） |
| 0x4A85F0 | WriteOne INI（型別書式 → ファイルへ section/key/value） | INI backend |
| 0x4A8800 | ReadOne INI（型別 Get* → entry 書戻し、既定値3系統 0x404DF0/0x403BE0/0x403C00） | INI backend |
| 0x4A8430 / 0x4A8480 | section/key 分割（":" 区切り、既定 "MAIN"@0xA404C4） | 共通ユーティリティ |
| 0x4A78D0 | 先頭文字 → type 判定（表 §5.2、テーブル 0x4A792C/0x4A7954） | 共通ユーティリティ |
| 0x4A83F0 / 0x4A83C0 | u32 ↔ 4バイト pack/unpack（type 7/8 用） | 共通ユーティリティ |
| 0x4A8B10 / 0x4A8E50 | Dump（コンソール一覧） | 任意（デバッグ） |
| 0x4A8C90 / 0x4A8CF0 / 0x4A8BC0 / 0x4A8D20 | Registry Begin/End/Write/Read | **Android では不要（スタブ）** |
| 0x4040A0 / 0x40C180 | Open(path,flag) / SavePath(path)=Open+SaveAll+End | 必須（起動・Controls 用） |
| 0x513B60 | SaveIniFiles opcode impl（Begin(1)→SaveAll→End） | 必須 |
| 0x513BB0 | RefreshINI opcode impl（パス再構築→Begin(0)→LoadAll→End→ログ） | 必須 |
| 0x50EC30 | FindSetting(name) | 必須 |

### 8.3 2バックエンドの抽象化方針（EXE の事実に基づく）

- **共通部分（SaveAll/LoadAll/AddSetting/dtor/list/セッション）は基底で実装し、差分5スロットのみバックエンドでオーバーライド**する — これが EXE の vtable 配置そのままである（2.3）。
- **INI バックエンド**: Win32 `Write/GetPrivateProfileStringA/IntA` 直叩きの代わりに**自前 key=value パーサ**で代替。振る舞い互換の要点:
  - Read: キー/セクション無し → 既定値を返す（entry 不変）。INI ファイルは(section, key) → value のマップとして保持。
  - Write: (section, key, value) の **upsert**。既存ファイルの未知セクション/キーは **保持**すること（`WritePrivateProfileStringA` が既存ファイルの他エントリを残す挙動の再現。全文書き直しを行う場合は既存内容とマージ必須）。
  - 文字列バッファ上限 256B（読込 nSize=0x100）、section/key 64B は静的 name 依存のため実害なし。
  - リトライ・戻り値検査は元実装どおり不要（WriteOne は戻り無視、SaveAll は全 true のみ AND）。
- **Registry バックエンド**: Android では **no-op スタブ**（Begin/End = true、Read/Write は無視）。実 EXE でも起動後に呼出が無い（§7-5）ため機能喪失なし。
- **BlendSettings バックエンド**（0xB11C40, 62件）: INI バックエンドの再利用（同一 vtable）。アニメーションブレンド値を使うシステムが対応するまで保留可（未読込なら内部既定値で動作する設計）。

### 8.4 SaveIniFiles オペコードとの接続

- オペコードテーブル（基底 0xB0AF48、stride 0x28、532件）の entry `+0x18` が impl を保持し、**テーブル間接呼出のみ**（直接 E8 ゼロ）。この構造は移植側でもそのまま使える: opcode dispatcher → `SaveIniFiles()` → `g_INISettings.Begin(1)` → `SaveAll()`（+0x10C リスト全走査 → WriteOne）→ `End()`。
- 同系統 opcode 5本（0x011C SetGameSetting / 0x011D SetINISetting / 0x011E GetINISetting / 0x013E RefreshINI / 0x0165 SaveIniFiles）は **impl 関数も vtable 呼出も無改造で流用可能**（依存は FindSetting 0x50EC30 と g_INISettings の this のみ）。
- Get/SetINISetting の出力書式（0xA4D7FC 等）と FindSetting の名前比較（_stricmp、大文字小文字非区別）も保持すること。

### 8.5 起動時自動全量書出（0x40F58E）の再現可否判断

- 実 EXE は起動完了時に `0x40C180(path,1)` = **Open+SaveAll+End で Oblivion.ini を全設定で上書き再生成する**（既知既定値がファイルへ吐き出される）。この挙動の再現要否は移植方針で判断:
  - **再現する場合**: 初回起動で完全な Oblivion.ini が生成され、外部ツール/モッドとの互換が高い（Bethesda の実機挙動に一致）。注意: 未知キーはマージ維持（§8.3）、書出失敗時はログのみ（元実装は戻り無視）。
  - **再現しない場合**: ファイルは実際の変更時のみ書き換わり、既定値は内部マップで保持。ファイル欠損時の `CopyFileA(Oblivion_default.ini)` 相当（§7-3）も不要化し得る。
  - 推奨: **再現する**（ゲーム設定を ini から読む外部要素への配慮）。ただし BlendSettings の VERSION=14 スタンプ（0x53AD60）は再現不要（自前 ini の世代管理は不要）。

### 8.6 移植時の注意

1. **Windows API 直叩き vs ポータブル化**: 差分5スロット（+0x08..+0x18）の内部だけを置き換えれば、共通4スロット・登登録テーブル574件・opcode 経路は無改造。Win32 依存は WriteOne/ReadOne/Begin(Reg)/Dump 出力先のみ。
2. **パス**: this+4 は NUL 終端バッファ（0x104B 想定）。実 EXE は `[0xB3F280](dir)+"Oblivion.ini"` を起動時と RefreshINI の両方で再構築する（ディレクトリは bss で実行時生成）。移植ではアプリの設定ディレクトリへ読み替え、RefreshINI のメッセージ文言（0xA4D690）はファイル名部分のみ実パスに置換可。
3. **リスト順**: prepend のため SaveAll の書出し順 = 登録逆順。自前パーサがセクション単位で全文を再構成する場合、順序差は仕様差として許容（INI は順序非依存）。
4. **Begin/End の対**: WriteOne/ReadOne は非セッション時に自動 Begin/End する（再入時に多重 Begin しないこと。INI Begin は flag 無視で必ず session=this なので二重 Begin は実質無害だが End でセッションが閉じられる点に注意 — 元実装は「自動 Begin した場合のみ End」で対応）。
5. **SaveAll/LoadAll はセッション必須**（[+0x108]==0 なら即 false）。opcode 側が Begin を先に呼ぶ契約を守る。
6. **security cookie / CRT 初期化順**: 静的初期化での AddSetting は CRT フェーズで走る（登録順の不定性あり）。移植では起動時の明示的テーブル登録へ置換可。

---

## 9. 未解決点

1. **`0x53AD60` 後半**: VERSION=14 スタンプとパス構築以降の制御フローが未解読（線形逆アセンブルが他関数領域へ踏み込む）。blend 設定の再読込/再書出の有無は未確定。呼出元は起動部 `0x40F5A3` の1本のみ。
2. **Dump の PrintCtx 型**: vt+0x08 の引数（+0x8/+0xA/+0xE の u16 フィールドを持つバッファ管理オブジェクト、grow=0x4E4A10 / append=0x42BB90 / entry 表示=0x4A7B10 / 出力=0x70F910, 0x70FB40）の型・呼び出し元コマンドが未同定。`0x4FAE80`（GetINISetting の引数解決, L194577）も未深掘。
3. **起動 fn の全域**: `0x40DF80` は実起点ではなく中盤（fld/fstp 既定値コピー帯）。関数開始アドレス、ディレクトリ取得 API（`0x40E0F3 call edi` の edi 供給元 = docs path 取得）は未特定。周辺に CopyProtection 文字列読込（`[0xB02DE8]` String + section "CopyProtectionStrings"@0xA31224）と単一インスタンス検査（FindWindowA/SetForegroundWindow@0x40E393）を確認済みだが詳細は範囲外。
4. **sBasePath「読まれない」の証拠限界**: imm32 全走査（直接参照ゼロ）+ FindSetting 呼出元2件 INI 固定に基づく。レジスタ経由・関数テーブル経由の間接参照は理論上残り得る（ただし 0xB07D28 ベースの仮想呼出は起動後ゼロで反証も強い）。
5. **`0x446CB0` の OOM フォールバック** `_DAT_00000004 = param_1[1]` は decompiler 成果物の可能性（実コードの nibble 判定未了）。OOM は事実上発生しないため移植影響ゼロ。
6. **BlendSettings 62エントリの全列挙**: サンプル `"fGetUpTime:HIT"`(entry 0xB11A24) のみ実読。残りの name 一覧は未取得。
7. **opid → idx 写像**: entry +0x08 に opid が入るにもかかわらず idx132 が opid 0x0165（idx21 が 0x0015）であり、テーブルは「実装済み opcode の詰め込みリスト」。opid→entry の解決手続き（線形探索 or 別表）は本タスク範囲外（V10/V13 台帳で対応済み）。

---

### 参照（本タスクの読み取り実績）

- セルフテスト4件 + オペコードテーブル opid 5件の entry 実読
- vtable 4本（0xA404F8 / 0xA40590 / 0xA40548 / 0xA566A4）全スロット実読
- 逆アセンブル: 0x4040E0, 0x4040A0, 0x40C180, 0x4A78D0, 0x4A83B0, 0x4A83F0, 0x4A8430, 0x4A8480, 0x4A7CB0, 0x4A84E0, 0x4A8560, 0x4A85A0, 0x4A8B10(+tail), 0x4A8C90, 0x4A8CF0, 0x4A8D20(+tail), 0x40E6A0-0x40E780, 0x40E0E0-0x40E280, 0x40F560-0x40F5C0, 0x513B60, 0x513BB0-0x513C80, 0x514230, 0x53AC60-0x53AD59, 0x53AD60-head, 0x9D8020, 0x9DAC50, 0xA1AA20
- decompiled.c 引用: L3249 (0x4040E0), L54233 (0x446CB0), L133888 (0x4A7CB0), L134339 (0x4A83B0), L134459 (0x4A85F0), L134525 (0x4A8800), L134626 (0x4A8BC0), L134677 (0x4A8CF0)
- 参照走査（imm32 / E8 全 .text）: 0xB07BF0 / 0xB07D28 / 0xB07AD8 / 0xB11C40 / 0xB0472C / 0xB02C2C / 0xB3F280 / 0xB02C84 / 0xA4D690 / 0xA40580 / 0xA4056C / 0xA405B4 / E8→0x4040E0(637) / E8→0x50EC30(2) / E8→0x53AC60(1) / E8→0x53AD60(1)
