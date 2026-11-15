# v373 — registry `0xB3CD7C` の正体と 48B stride オブジェクトの種別の裁定（#45 N5b ④の最終確定・V-372-1）

- 採番: **v373**（#45 N5b の残点 V-372-1 消化、probe prefix v373_1..v373_4）
- 対象: Oblivion.exe v1.2.0416 ＋ b07_vtable_index.md（RTTI 表）
- 依頼経緯: v372 の残点 **V-372-1（registry `0xB3CD7C` の正体と 48B stride オブジェクトの種別）** が「#45 N5b ④の最終確定に必要」として残存 → 本 v373 で消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または b07 RTTI 表のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| registry `0xB3CD7C` の正体 | **[確定(b07+命令)]** | **`NiMultiTargetTransformController` の静的シングルトン**。根拠: (i) `0x6C5F50: mov eax,0xB3CD7C; ret` が **vtable `0xA79984` の slot1**（b07 row994 = `NiMultiTargetTransformController`）＝このクラスの getter がシングルトンを返す (ii) ctor `0x6C5EE0` が `mov [esi],0xA79984`＋`[+0x3C/40/44]=0` 初期化 (iii) `0x700010(obj,key)`＝**リンクリスト lookup**（`[obj+0xC]` 先頭・各ノード `next=[+0x34]`・`virtual+4`(GetName) の **name ptr をポインタ比較** `cmp eax,edi`・一致ノードを返す／不一致 NULL・`ret 4`） |
| 48B(0x30) stride オブジェクトの種別 | **[確定(b07+命令)]** | **`NiBlendTransformInterpolator`（48B）**。根拠: (i) `0x6CBD02: push 0x30; call 0x401F00`(new) → `call 0x6CC4E0`(基底) → **`mov [esi],0xA79E54`**（vtable）(ii) dtor `0x6CBCB0` も同一 vtable `0xA79E54` → **b07 row870 = `NiBlendTransformInterpolator`** (iii) accessor `0x6CFC05`/`0x6CFDE5`＝`lea eax,[eax+eax*2]; shl eax,4; add eax,[ecx+0x3C]; ret`（**idx*48→ptr**） |
| base/ptr/count 3フィールドの帰属 | **[部分確定]** | `[+0x3C]`=48B base／`[+0x40]`=ptr 配列（4B 要素・`cmp [esi+ebp*4]`）／`[+0x44]`=**count（u16）**。ctor `0x6C5EE0`（NiMultiTargetTransformController）が3つとも0初期化し、グローバル `0xB3CBA4/A8/AC/B0`（base/ptr/count/extra）から various object へコピーされる初期化群14件（`0x6C374D/0x6C3835/0x6CE4E8/0x6CB66A...`）を実測。**「NiMultiTargetTransformController（または骨ノード系）が持つ interpolator 配列」** [部分確定] |

**#45 N5b ④の最終確定**: field8（v372 で確定した解決格納先）に格納されるのは **`NiBlendTransformInterpolator` のポインタ（`base + index*48`）**＝**controlled block → interpolator の対応付け**。port の `clip`（AnimationClip）はこの interpolator 群に対応する port 側再構成（v372 §2.4 の「1対1不在」を補強）。

---

## §2 証拠

### §2.1 `0xB3CD7C` = NiMultiTargetTransformController シングルトン [確定]

- **getter**（v373_1(a)）: `0x6C5F50: mov eax, 0xB3CD7C; ret` — v373_3(a) で **vtable `0xA79984` の slot1** と同定（slot0=`0x6C5F60`=dtor・slot1=`0x6C5F50`=getter・slot6=`0x6D02E0`/slot7=`0x6D0370`=Load系・slot8=`0x6CE1C0`/slot9=`0x6CFE60`=interpolator 操作系）。
- **b07 RTTI 表**: `| 994 | NiMultiTargetTransformController | .?AVNiMultiTargetTransformController@@ | 0x00A79984 |`（v373_3(d)）。
- **ctor `0x6C5EE0`**（v373_2(a) unfiltered）:
  ```
  6C5EE0: SEH prologue
  6C5F08: call 0x6D04E0           ; 基底 ctor
  6C5F0D: xor eax,eax
  6C5F13: [esi+0x3C] = 0          ; ★base = NULL
  6C5F16: [esi+0x40] = 0          ; ★ptr 配列 = NULL
  6C5F19: word [esi+0x44] = 0     ; ★count = 0（u16）
  6C5F24: [esi] = 0xA79984        ; vtable
  6C5F2A: call 0x6D0010           ; 引数（name）で初期化
  6C5F41: ret 4
  ```
- **lookup `0x700010`**（v373_1(b) unfiltered）:
  ```
  700010: esi = [ecx+0xC]          ; リンクリスト先頭（ecx = シングルトン）
  700019: edi = [esp+0xC]          ; arg1（比較キー＝オブジェクト ptr）
  700020: loop: eax=[esi]; edx=[eax+4]; call edx   ; virtual+4 = GetName 系
  700030: cmp eax, edi             ; ★name ptr をポインタ同士で比較（interning）
  700034: eax=[eax+4] / jne loop   ; 候補チェーン
  70003B: esi = [esi+0x34]         ; ★next ノード
  700042: return NULL / 700049: return esi
  ret 4
  ```
- **xrefs 13件**（v373_1(a)）: `push 0xB3CD7C; call 0x700010` 系＝`0x4732EB`（v371 の game 側 fn `0x473200`）／`0x47F762`（→`0x47C710`→`cmp ax,[0xA7A160]`）／`0x4E3EC6`（→`or word [eax+8],0x40`）／`0x6C693D`（**骨解決 0x6C6910**）／`0x6C8251`（StoreTargets 前提 fn `0x6C8220`）＋リンクリスト走査型 `cmp eax,0xB3CD7C; je`（`0x473180`/`0x4969E0`）。

### §2.2 48B オブジェクト = NiBlendTransformInterpolator [確定]

- **ctor（new 48B）**（v373_4(c)）: `0x6CBD02: push 0x30; call 0x401F00` → `mov ecx,esi; call 0x6CC4E0`（基底）→ **`0x6CBD23: mov dword [esi],0xA79E54`**（vtable 設定）。
- **dtor `0x6CBCB0`**（v373_4(a)）: `call 0x6CC4E0; mov [esi],0xA79E54; ret`。`0x6CBCA0` は `jmp 0x6CCF50`（ deleting dtor 経路）。`0x6CBC7C`（別 ctor 形）も同一 vtable。
- **b07 RTTI 表**: `| 870 | NiBlendTransformInterpolator | .?AVNiBlendTransformInterpolator@@ | 0x00A79E54 |`（v373_3(d)）。
- **accessor 2本**（v373_1(d)）: `0x6CFC05`/`0x6CFDE5`＝`lea eax,[eax+eax*2]; shl eax,4; add eax,[ecx+0x3C]; ret`（**idx*48 → ptr**）。
- **48B 格納 site census**（v373_1(d) 12件）: `0x6C6A34`（**骨解決 0x6C6910**）／`0x6C8502`（StoreTargets 前提 fn `0x6C8220` 内・`mov [ebx+8],eax; or byte [eax+0xC],1`）／`0x6CFC05`/`0x6CFC5F`/`0x6CFDE5`/`0x6CFFC6`/`0x6D01A6`/`0x6D0206`（`virtual+0x38` 呼び出し）— **field8=base+idx*48 格納が複数経路で共通**。

### §2.3 3フィールド（base/ptr/count）の初期化 [部分確定]

- **グローバル4個** `0xB3CBA4`(base)/`0xB3CBA8`(ptr)/`0xB3CBAC`(count)/`0xB3CBB0`(extra) から various object へコピーする初期化群 **14件**（v373_1(c)/v373_2(b)）: `0x6C374D`(`[esi+0x10..0x18]`)／`0x6C3793`(`[esi+0x30..0x38]`)／`0x6C3835`(`[esi+0x3C..0x48]`・vtable **0xA79584=`NiBlendAccumTransformInterpolator`** b07 row863)／`0x6CE4E8`／`0x6CB66A`／`0x6CB84B`／`0x6C3CCF`／`0x6CBD79`／`0x6CC02B`／`0x6CEBDE`／`0x470ADE`／`0x47166E`／`0x47C796`／`0x6CF6C5`。
- **count は u16**（ctor の `mov word [esi+0x44],ax` ＋ `0x6C6910` の `movzx edx,word [esi+0x44]` ＋ `0x6CFFD6: cmp di,[esi+0x44]`）[確定(命令)]。
- **`0xB3CBA4` の直接書手は census に不在**（読み取りコピーのみ）→ .bss 静的初期化か別 fn [未確定＝残点 V-373-2]。

### §2.4 StoreTargets 前提 fn `0x6C8220` の再解釈（v371 §2.2A の補強）

- `0x6C8220`（head=0x6C8220・E8 callers=`0x49F86C`/`0x6C95C1`＝StoreTargets）は v371 で「NiTransformController 登録確認」としたが、v373_2(b) で **`push 0xB3CA58; call 0x700010`（別 registry）→ `[edx+edi*4]` 照合 → `0x6C5F80`（ptr 配列 set/get）→ `push 'Bip01'; call 0x98262D`(strcmp) → `call 0x6FFE90` → `0x6C8502`（48B 格納＋`or [eax+0xC],1`）** までを実行する **前提チェック兼 interpolator 格納 fn** である [確定(命令)]。v371 の「登録確認」記述は本 v373 で**補強・拡張**される。

## §3 残点 **V-373-1..3**
1. **`0xB3CBA4`（base グローバル）の writer**（.bss 静的初期化か、未発見の init fn か。interpolator 群の実配列がどこに構築されるか）【中・#45 N5b の interpolator 配列構築元に必要】
2. **`NiBlendTransformInterpolator` の48B フィールド布局**（`[+0x0C]` byte flag（`or [eax+0xC],1`）等の意味）【中】
3. **`0xB3CA58` registry（0x6C8220 が呼ぶ）の正体**（0xB3CD7C と何が違うか）【低】

## §4 検証・自己批評
- probe: **v373_1..v373_4 全 EXIT=0**。
- **決定打の作り**: (i) `0xB3CD7C` getter の vtable slot1 同定 → (ii) b07 表の vtbl 行照合（`NiMultiTargetTransformController`）→ (iii) 48B new site の dtor vtable 追跡 → (iv) b07 行照合（`NiBlendTransformInterpolator`）。**推測ゼロの b07×命令の二重根拠**。
- 妥当性チェック: ①ctor の3フィールド0初期化⇔グローバル4個コピー初期化14件 ②count=u16 が ctor/0x6C6910/0x6CFFD6 の3所で一致 ③48B 格納 site（field8=base+idx*48）が骨解決・前提 fn・accessor の3系統で一致 — **三方向で矛盾なし**。
- docs との不整合: **なし**（本裁定は新規。v372 の V-372-1 を解消）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
