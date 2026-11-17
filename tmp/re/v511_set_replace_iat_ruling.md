# v511 — `FUN_0046C300`＝set/replace fn・IAT＝KERNEL32（`GetLastError`/`TlsGetValue`）・globals＝`-1` の裁定（V-511-1..3 完遂・旧残点 V-406-3 継続第41弾）

- 採番: **v511**（旧残点群消化・probe prefix **v511_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-511-1..3（v510 残点）** — 1=map ctor 本文特定と IAT import 先・global identity／2=Player `+0x80` 中身と gated mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-511-1: `FUN_0046C300`（v510「map ctor 候補」）本文** | **[確定相当]（set/replace・ctor 訂正）** | docs head 実読 [確定(decompiled)]: **`void __thiscall FUN_0046c300(int param_1, int param_2, char param_3)`: `iVar1 = *(param_1 + 0xc)`（**current ptr＝`+0xC`**）→ `if (param_2 != iVar1)`（新旧比較）→ **`if ((*(param_1 + 8) >> 0xe & 1) == 0)`（**`+0x8` bit14 gate＝v492 gated gate と同一 [確定]**）→ `if (iVar1 != 0) FUN_004524c0(iVar1)`（**old を remove（v505 remove fn）**）→ `if ((param_3 != '\0') && (*(param_1+0xc) != 0)) FUN_00446b40(...)`（**char flag 時に old を別処理（free 候補 [推定]）**）→ `if (param_2 != 0) FUN_00452570(param_2, param_1)`（**new を insert（v494 insert fn）**）→ `*(param_1 + 0xc) = param_2`（**`+0xC` を new に書く）[確定(構造)]** → **`FUN_0046C300`＝「`param_1+0xC`（current ptr）を差し替える set/replace fn（old remove→new insert→`+0xC` 更新）[確定相当]」→ v510「map ctor 候補」を訂正（ctor でなく set/replace）[確定]**。**`+0x8` bit14 gate が v492 gated gate と同一 → `FUN_0046C300` は gated notify（`FUN_0045B670` 経由 [推定]）と接続 [確定相当]**。`param_3`（char）＝old を free するかの flag [推定相当] |
| **V-511-1: IAT import 先（`0xA281EC`/`0xA28150`）** | **[確定]（実 import 解決）** | **PE import descriptor 走査 [確定(機械)]: **`IAT 0xA28150 → KERNEL32.dll!TlsGetValue`**／**`IAT 0xA281EC → KERNEL32.dll!GetLastError`** [確定] → **`FUN_0098C072`（v510）＝`GetLastError`/`TlsGetValue`（KERNEL32）を使う dynamic lookup/init [確定相当]**（**TLS/last-error 系 Windows API 呼び出し**＝v493 `GetCurrentThreadId` に続く thread 系 API と整合） |
| **V-511-1: globals `0xB310AC`/`0xB310B0` identity** | **[推定相当]（`-1`＝INVALID 候補）** | `.data` 実読 [確定(機械)]: **`0xB310AC: 0xFFFFFFFF`（-1）／`0xB310B0: 0xFFFFFFFF`（-1）** → **`FUN_0098C072` が push する2つの global＝`-1` [確定] → KERNEL32 API の引数（`TlsGetValue(-1)`/`GetLastError` 系 [推定相当]）、`0xFFFFFFFF`＝`INVALID_HANDLE_VALUE` 候補 [推定]** |
| **V-511-2: Player `+0x80` 中身／gated mask 源** | **[未確定]（残点継続）** | 本 probe 未着手（budget は map ctor/IAT/global に集中） |
| **V-511-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v511_1）

### §2.1 `FUN_0046C300`（v511_1a）
```
L85869: void __thiscall FUN_0046c300(int param_1, int param_2, char param_3) {
L85874:   iVar1 = *(int *)(param_1 + 0xc);                  // current ptr
L85875:   if (param_2 != iVar1) {                            // new != current
L85876:     if ((*(uint *)(param_1 + 8) >> 0xe & 1) == 0) {  // bit14 gate (v492 same)
L85877:       if (iVar1 != 0) { FUN_004524c0(iVar1); }        // old remove (v505)
L85880:       if ((param_3 != 0) && (*(param_1 + 0xc) != 0)) { FUN_00446b40(...); }  // free? [推定]
L85883:       if (param_2 != 0) { FUN_00452570(param_2, param_1); }  // new insert (v494)
L85887:     }
L85887:     *(int *)(param_1 + 0xc) = param_2;               // current = new
L85889:   } return; }
=> set/replace fn (NOT ctor)  [確定相当]; v510 ctor candidate corrected  [確定]
```

### §2.2 IAT resolution（v511_1b）
```
import dir RVA=0x700304 size=0x12C
  IAT 0xA28150 -> KERNEL32.dll!TlsGetValue
  IAT 0xA281EC -> KERNEL32.dll!GetLastError
=> FUN_0098C072 uses KERNEL32 TLS/last-error APIs  [確定相当]
```

### §2.3 globals（v511_1c）
```
0xB310AC: 0xFFFFFFFF (-1)
0xB310B0: 0xFFFFFFFF (-1)
=> the pair pushed by FUN_0098C072 = -1 values  [推定相当] (INVALID_HANDLE_VALUE candidate)
```

## §3 残点 **V-512-1..3**
1. 【中】**`FUN_00446B40`（old free 候補）identity** ＋ **`+0x8` bit14 gate の semantic**（gated notify 経路の完成）＋ **Player `+0x80`（ptr）の中身**。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ `FUN_00452C20`（lazy node）の `param_1+0xC` key と `FUN_0046C300` の `param_1+0xC`（current ptr）の同 offset異物の切り分け。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v511_1 の1本 EXIT=0**（V-511-1 `FUN_0046C300`＝set/replace fn を確定相当で v510 ctor 訂正・IAT＝KERNEL32 実 import を確定・globals＝`-1` を推定相当／V-511-2 は budget 外で残点／V-511-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_0046C300` docs head 34行を読む** → `old remove→new insert→`+0xC` 更新` の set/replace 構造と **`+0x8` bit14 gate（v492 と同一）**を獲得 → ctor 訂正＋gated 接続を確定 → (ii) **PE import descriptor 走査**で IAT slot を `KERNEL32!GetLastError`/`TlsGetValue` に解決 → (iii) **`0xB310AC/0xB310B0` の dword dump**で `-1/-1` を獲得。
- 妥当性チェック: ①`FUN_004524C0`（old remove）⇔v505 remove（`count--`）— ②`FUN_00452570`（new insert）⇔v494 insert（`count++`）— ③`*(param_1+0xc)=param_2`⇔`+0xC`＝current ptr（v506 node `+4`＝key とは**同 offset 異物**＝区別）— ④`GetLastError`/`TlsGetValue`⇔v493 `GetCurrentThreadId`（KERNEL32 thread 系と整合）— ⑤`0xFFFFFFFF`⇔`INVALID_HANDLE_VALUE`（`-1`）— **五方向矛盾なし**。
- docs との不整合: **①v510 ruling「`FUN_0046C300`＝map ctor 候補 [推定]」→ 本裁定で set/replace fn [確定相当] と訂正（ctor でない）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **PE import descriptor 走査（`DataDirectory[1]`→thunk walk）が IAT slot の DLL/関数名を確定させる**（**`0xA281xx`＝KERNEL32 thread/last-error 系と即決**＝v510 import 先の完全解決）。
  2. **「set fn の old remove／new insert／`+0xC` 更新」の3段が差し替え fn の定番**（**v505 remove⇔v494 insert⇔`+0xC` write の横断**）。
  3. **同 offset 異物（`+0xC`＝node key（v506）と current ptr（本裁定））は「どの object の `+0xC` か」を明記**（**node `+4`/`+8` と `param_1+0xC` の区別を ruling に残す**）。
  4. **`>> 0xe & 1`（bit14）gate が v492 gated と同一** → **`FUN_0046C300` が gated notify の受け手 [推定相当]**（**bit gate の横断一致が fn 接続を語る**）。
  5. **KERNEL32 API 名（`GetLastError`/`TlsGetValue`）が `FUN_0098C072`（v510 dynamic lookup）の意味を確定**（**IAT 解決が fn semantic を完成**）。
