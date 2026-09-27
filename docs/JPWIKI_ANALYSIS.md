# JPWiki Data Analysis

Analysis of the Japanese localization data set located at `D:\Cargo\Other\Oblivion\JPwiki`.

## Overview

The directory contains the JPModWiki Japanese localization package for The Elder Scrolls IV: Oblivion,
distributed by `jpmod.oblivion.z49.org`. The package consists of three TES4 plugins (ESP files),
a game executable patch, and a localization helper tool.

| File | Size | Description |
|---|---:|---|
| `JPWikiMod_Vanilla.esp` | 6,395,869 bytes | Main localization (Vanilla, books excluded) |
| `JPWikiMod_Vanilla+SI.esp` | 8,512,407 bytes | Main localization (Vanilla + Shivering Isles) |
| `JPBooks_Merged[V+S+ML].esp` | 2,263,009 bytes | Book localization (Vanilla + SI + Morrowind merged) |
| `TES4_12416_JaPatch_015.EXE` | 28,940 bytes | Game executable Japanese patch (analyzed below) |
| `obja_config.exe` | 1,536 bytes | OBJA launcher stub (analyzed below) |
| `obja.dll` | 32,768 bytes | OBJA runtime module (analyzed below) |
| `Oblivion.exe.bak` / `Oblivion.OLD` | 7,556,096 bytes | Original executable backups |

## Plugin Headers

All three plugins share the same header metadata.

| Field | Value |
|---|---|
| Signature | `TES4` |
| Master | `Oblivion.esm` |
| Author | `jpmod.oblivion.z49.org` |
| Form Version | 0 |
| Compressed records | 0 |
| Deleted records | 0 |

Version strings embedded in the header description:

- `JPWikiMod_Vanilla.esp`: Ver. 20150222, Data: Vanilla (without Books)
- `JPWikiMod_Vanilla+SI.esp`: Ver. 20150222, Data: Vanilla + SI (without Books)
- `JPBooks_Merged[V+S+ML].esp`: Ver. 20140505, Data: Vanilla(494/505), SI(56/56), Morrowind(67+4+19)

## Record Statistics

| Record Type | Description | Vanilla | Vanilla+SI | JPBooks |
|---|---|---:|---:|---:|
| `INFO` | Dialogue Info | 14,332 | 19,179 | - |
| `DIAL` | Dialogue Topic | 2,585 | 3,474 | - |
| `GMST` | Game Setting | 723 | 723 | - |
| `SCPT` | Script | 273 | 390 | - |
| `QUST` | Quest | 217 | 251 | - |
| `LSCR` | Load Screen | 279 | 337 | - |
| `CLAS` | Class | 29 | 29 | - |
| `SKIL` | Skill | 21 | 21 | - |
| `BSGN` | Birthsign | 13 | 13 | - |
| `RACE` | Race | 10 | 10 | - |
| `MGEF` | Magic Effect | 1 | 1 | - |
| `BOOK` | Book | - | - | 640 |
| `LVLI` | Leveled Item | - | - | 9 |
| `CLOT` | Clothing | - | - | 3 |
| `CONT` | Container | - | - | 1 |
| `NPC_` | NPC | - | - | 1 |
| `CELL` | Cell | - | - | 1 |
| `REFR` | Object Reference | - | - | 1 |
| `ACHR` | Actor Reference | - | - | 1 |
| `PACK` | AI Package | - | - | 1 |
| **Total** | | **18,483** | **24,428** | **658** |

## GRUP Structure

| Group Type | Vanilla | Vanilla+SI | JPBooks |
|---|---:|---:|---:|
| Top | 10 | 10 | 7 |
| Topic Children | 2,144 | 2,960 | - |
| Interior Cell Block | - | - | 1 |
| Interior Cell Sub-Block | - | - | 1 |
| Cell Children | - | - | 1 |
| Cell Persistent Children | - | - | 1 |

Top-level groups present:

- `JPWikiMod_Vanilla.esp`: GMST, CLAS, RACE, SKIL, MGEF, SCPT, BSGN, DIAL, QUST, LSCR
- `JPWikiMod_Vanilla+SI.esp`: same as Vanilla
- `JPBooks_Merged[V+S+ML].esp`: BOOK, CLOT, CONT, NPC_, LVLI, CELL, PACK

## Localizable Text

| File | Subrecords | Text bytes |
|---|---:|---:|
| `JPWikiMod_Vanilla.esp` | 40,778 | 1,749,435 |
| `JPWikiMod_Vanilla+SI.esp` | 54,145 | 2,311,276 |
| `JPBooks_Merged[V+S+ML].esp` | 1,285 | 2,137,023 |
| **Total** | **96,208** | **6,197,734** |

Breakdown by subrecord type:

| Subrecord | Meaning | Vanilla | Vanilla+SI | JPBooks |
|---|---|---:|---:|---:|
| `NAM1` | Dialogue response text | 17,861 | 23,806 | - |
| `NAM2` | Dialogue response audio/notes | 17,851 | 23,796 | - |
| `FULL` | Display name | 2,853 | 3,776 | 645 |
| `CNAM` | Topic / class name | 1,850 | 2,346 | - |
| `DESC` | Description / book body | 353 | 411 | 640 |
| `SNAM` | Sound / script name | 10 | 10 | - |

## Technical Findings

### GRUP size semantics

The `size` field of a `GRUP` record **includes its own 20-byte header**. An initial parser
assumption that the size excluded the header caused the walker to misalign at every group
boundary, undercounting records by a factor of roughly 17 (1,076 instead of 18,483 for the
Vanilla plugin). Correcting this produced zero parse errors across all three plugins.

### Record layout

```
Record: type(4) + size(uint32) + flags(uint32) + form_id(uint32) + vcs1(uint32) + version(uint16) + vcs2(uint16) + data
GRUP:   'GRUP'(4) + size(uint32) + label(4) + group_type(int32) + vcs1(uint32) + version(uint16) + vcs2(uint16) + children
```

For `GRUP`, `label` holds a record type when `group_type == 0` (Top), otherwise a FormID.

### Text encoding

All Japanese text is encoded as **cp932 (Shift_JIS)**. Decoding succeeds without fallback for
every localizable subrecord in the data set.

### Book markup

Book `DESC` subrecords contain HTML-like markup used by the original engine renderer:

```
<font face=5><br>\r\n<br>\r\n親愛なる特使へ\r\n<br>\r\n<br>\r\n...
<DIV align="center">蟲の同胞たちよ！<br>\r\n...
<FONT color="9E0707">\r\n<font face=5>\r\n...
```

Tags observed: `<font face=N>`, `<FONT color="RRGGBB">`, `<br>`, `<BR>`, `<DIV align="...">`.

## Relevance to Oblivion_Android

- Approximately **6.2 MB of Japanese text** is extractable from the three plugins.
- The subrecord format is a simple `type + size + data` sequence, straightforward to parse in C++.
- The plugins are differential overrides, so applying the text requires resolving FormIDs
  against `Oblivion.esm`.
- Dialogue (`DIAL` / `INFO`) dominates the volume, so conversation text is the primary
  localization target; books are a separate, self-contained corpus.
- The Windows-side runtime (`obja.dll`) is not portable; the Android port needs its own
  font and input pipeline. See the executable analysis section for details.

## Analysis Tool

The parser used for this analysis is a standalone Python script that walks the GRUP tree,
decodes cp932 text, and reports record and text statistics. It is a session artifact and is
not part of the repository.

## Localization Data Integration

The JPWiki text is redistributable, so the game setting strings are shipped with the app.

### Extracted data

`app/src/main/assets/localization/jpwiki_localization.tsv` is a UTF-8, tab-separated file
generated from the three plugins:

| kind | entries | key | description |
|------|---------|-----|-------------|
| `gmst` | 723 | editor ID (e.g. `sContinue`) | Game Setting UI strings |
| `book` | 640 | FormID | Book body text (`DESC`) |
| `info` | 19,179 | FormID | Dialogue responses (`NAM1`) |
| `dial` | 3,474 | FormID | Dialogue topics (`FULL`) |
| `qst` | 249 | FormID | Quest names (`FULL`) |
| `full` | 4,421 | `RECTYPE:FormID` | Object names (`FULL`) |

Line format: `kind \t key \t english \t japanese`. The `english` column is currently empty
for all rows; English text is resolved from `Oblivion.esm` at runtime.

FormID keys are normalized to lowercase 8-digit hex while loading so they match
`LocalizationManager::formKey()`. The source plugins store them uppercase, which previously
made every FormID lookup miss and silently fall back to English.

Not every record carries a translation. The JPWiki plugins leave some strings in English,
so the Japanese column is not always Japanese:

| kind | entries | with Japanese text |
|------|---------|--------------------|
| `gmst` | 723 | 723 |
| `book` | 640 | 340 |
| `info` | 19,179 | 19,167 |
| `dial` | 3,474 | 2,607 |
| `qst` | 249 | 0 |
| `full` | 4,421 | 2,682 |

Quest names (`qst`) are never translated by JPWiki, so `getQuestName()` returns the English
name from `Oblivion.esm` for every quest. This is expected, not a lookup failure.

### Runtime loading

`LocalizationManager::loadJpwikiData()` reads the asset through `AAssetManager` during
`initialize()` and populates one map per kind:

| Map | Getter | Consumer |
|-----|--------|----------|
| `gameSettings` | `getGameSetting(editorID, fallback)` | UI strings |
| `bookTexts` | `getBookText(formID, fallback)` | `BookReader::getBookDescription()` |
| `infoTexts` | `getInfoText(formID, fallback)` | `DialogueManager::loadDialoguesFromESM()` |
| `dialogTexts` | `getDialogText(formID, fallback)` | `DialogueManager::loadDialoguesFromESM()` |
| `questTexts` | `getQuestText(formID, fallback)` | `QuestFlowController::getQuestName()` |
| `fullTexts` | `getFullText(recordType, formID, fallback)` | Object names |

Every getter returns the fallback when the current language is not Japanese or the key is
absent, so English behaviour is unchanged.

### Consumer wiring

- `BookReader::setLocalizationManager()` supplies the source used by `getBookDescription()`.
- `DialogueManager::setLocalizationManager()` supplies the default source for
  `loadDialoguesFromESM()`; the explicit parameter still overrides it.
- `QuestFlowController::setLocalizationManager()` supplies the source used by
  `getQuestName()`, which `activateQuest()` uses when creating the quest in `QuestManager`.
- `Renderer::initGameSystems()` attaches the manager to `DialogueManager` at construction.
- `Renderer` owns a `UIDialogue` panel and exposes `loadDialoguesFromESM()`,
  `openDialogueWithNpc(formID)`, `openDialogueWithNearestNpc()`, `isDialogueOpen()` and
  `closeDialogue()`. `createTestScenario()` calls `loadDialoguesFromESM()` after actors are
  placed so faction memberships are available for topic filtering.
- `NpcManager::getNpcByFormID()` resolves a spawned actor from its ESM base record FormID;
  `NPC::formID` and `NPC::factionFormIDs` are populated by `createNPCFromESM()`.
- The JNI layer exposes `nativeStartDialogue()`, `nativeCloseDialogue()` and
  `nativeIsDialogueOpen()` on `OblivionEngine`, backed by the `Renderer` instance held in
  `jni_bridge.cpp` (`jni_bridge_get_renderer()`).
- `Renderer` owns a `BookReader` and a `QuestFlowController`, both attached to the same
  `LocalizationManager`. `createTestScenario()` calls `bookReader->initialize(&esmMgr)` and
  `loadQuestsFromESM()` after the ESM data is available.
- `Renderer` owns a `UIBookReader` panel and exposes `openBook(formID)`, `isBookOpen()` and
  `closeBook()`. The panel strips Oblivion's HTML-style markup (`<font>`, `<DIV>`, `<br>`)
  and word-wraps the body, splitting CJK runs by character.
- `loadQuestsFromESM()` converts the ESM `QuestData` records into `QuestRecord`s and
  registers them with `QuestFlowController`; `QuestFlowController::initialize()` is called
  right after the Imperial Weave is initialized.
- The JNI layer exposes `nativeOpenBook()`, `nativeCloseBook()` and `nativeIsBookOpen()`.
- The debug console provides `readbook <formID>`, `closebook` and `listbooks`.

### Language preference

`SettingsManager` owns the persisted preference (`LANGUAGE=` in the app settings file).
`Renderer::initGameSystems()` mirrors it into `LocalizationManager` once both exist, and the
language toggles in `SettingsUI` and `LauncherScreen` update both objects. This replaces the
earlier behaviour where `LocalizationManager` reset to English on every launch.

### Terminology

The original game uses `Magicka` and `Fatigue`, not `Mana` and `Stamina`. The JPWiki data
keeps the original terms, and the built-in translation table was corrected to match.

## Executable and DLL Analysis

### `TES4_12416_JaPatch_015.EXE`

A 32-bit x86 GUI executable (28,940 bytes) that is **not** a custom patcher. Its version
resource identifies it as a generic binary-diff applier:

| Version field | Value |
|---|---|
| FileDescription | `Update for Windows95` |
| InternalName | `Update32` |
| OriginalFilename | `Update.exe` |
| CompanyName | `AjariSoft` |
| LegalCopyright | `Copyright(C) 1996-98 T.Nakagawa` |
| FileVersion | `1, 0, 5, 0` |
| ProductName | `DWiff` |

Embedded markers `DIFFMAKE` and `UPDATEICON` confirm it is the **DWiff / Update32** diff
applier. The target and version are hard-coded as strings:

```
Oblivion.exe
The Elder Scrolls IV OBLIVION ver1.2.416
```

Imports are minimal and consistent with a file-diff tool: `CreateFileA`, `ReadFile`,
`WriteFile`, `MoveFileA`, `DeleteFileA`, `SetFileTime`, `FindFirstFileA`, `GlobalAlloc`,
plus `CreateDialogParamA` / `msctls_progress32` for the progress dialog. It carries no
Oblivion-specific logic; the actual byte-level diff is stored in the executable's data
section and is applied to `Oblivion.exe` version 1.2.416.

### `obja.dll`

A 32-bit x86 DLL (32,768 bytes), version `0.0.15.14`, described as `OBJA Module`. This is
the runtime component that renders Japanese text inside the game. It is injected into the
Oblivion process and hooks the text rendering path.

Imports reveal its purpose precisely:

| DLL | Notable imports | Purpose |
|---|---|---|
| `gdi32.dll` | `CreateFontA`, `EnumFontFamiliesExA`, `GetGlyphOutlineA`, `GetTextMetricsA`, `GetTextExtentPoint32A`, `CreateDIBSection`, `BitBlt` | Font creation, glyph metrics, and glyph rasterization |
| `imm32.dll` | `ImmGetContext`, `ImmGetOpenStatus`, `ImmSetOpenStatus`, `ImmReleaseContext` | IME (input method editor) integration for Japanese input |
| `user32.dll` | `SetWindowLongA`, `CallWindowProcA`, `DialogBoxParamA`, `DrawTextA`, `MoveWindow` | Window subclassing and dialog handling |
| `advapi32.dll` | `RegOpenKeyExA`, `RegQueryValueExA` | Reading font-link registry settings |
| `DINPUT8.dll` | `DirectInput8Create` | DirectInput hooking |
| `kernel32.dll` | `VirtualAlloc`, `VirtualProtect`, `VirtualQuery`, `GetPrivateProfileIntA`, `WritePrivateProfileStringA` | Code patching and INI configuration |

Configuration is read from and written to:

```
\My Games\Oblivion\obja.ini
```

INI sections and keys found in the binary:

```
[Config]
[Console]
[OBJA]
Language
```

Font parameters are stored as comma-separated GDI `LOGFONT`-style tuples, one pair per
font slot (normal and interlinear spacing):

```
FontParam1_1 = N,0,32,-2,26,34,400,0
FontParam1_2 = N,0,32,-3,26,34,400,0
FontParam2_1 = N,0,40,-4,28,68,400,0
FontParam2_2 = N,0,40,-4,28,68,400,0
FontParam3_1 = N,0,28,-1,18,34,400,0
FontParam3_2 = N,0,28,-2,18,34,400,0
FontParam5_1 = N,0,34,-2,26,34,400,0
FontParam5_2 = N,0,34,-2,26,34,400,0
FontParam1_InterLinear
FontParam2_InterLinear
FontParam3_InterLinear
FontParam5_InterLinear
```

The font slots `1`, `2`, `3`, `5` correspond to the Oblivion UI font indices used by the
`<font face=N>` markup found in book text. The DLL also references the Windows font-link
registry key:

```
SOFTWARE\Microsoft\Windows NT\CurrentVersion\FontLink\SystemLink
```

Two dialog resources are embedded: `DLG_CFG` (configuration) and `DLG_IME` (IME control).
The default font family string is `MS UI Gothic`.

### `obja_config.exe`

A 1,536-byte launcher stub. It imports only `LoadLibraryA`, `FreeLibrary`, and `ExitProcess`,
and its sole string is `obja.dll`. It exists purely to load `obja.dll` and invoke its
configuration dialog, so the DLL can be configured without launching the game.

### Summary of the localization mechanism

The package works in three layers:

1. **`TES4_12416_JaPatch_015.EXE`** applies a binary diff to `Oblivion.exe` (v1.2.416),
   modifying the executable so it can load the OBJA module and handle Japanese text.
2. **`obja.dll`** is injected into the patched process and replaces the font rendering path
   with GDI-based Japanese font handling, including IME support and configurable font
   metrics from `obja.ini`.
3. **The three ESP files** supply the actual Japanese strings, overriding the English text
   by FormID.

None of these components provides a language toggle. The game becomes Japanese-only once
the patch is applied; reverting requires restoring `Oblivion.exe` from `Oblivion.exe.bak`.

### Relevance to Oblivion_Android

The OBJA approach is Windows-specific (GDI, IME, DirectInput, registry font links) and is
not portable to Android. The Android port must instead:

- Parse the ESP files directly and resolve FormIDs against `Oblivion.esm`.
- Render Japanese text with its own font pipeline (the `<font face=N>` indices map to the
  four font slots configured in `obja.ini`).
- Handle text markup (`<font>`, `<DIV>`, `<br>`) in the renderer.
- Provide its own input method integration rather than relying on `imm32.dll`.

The `FontParam*` values are useful as a reference for the intended font sizes and line
spacing per UI font slot.

