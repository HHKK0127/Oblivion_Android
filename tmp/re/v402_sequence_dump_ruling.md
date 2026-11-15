# v402 — fn `0x6CA440` の正式名（テキスト Save か Debug dump か）の裁定（#153 ⑥-8・V-387-1）

- 採番: **v402**（ai-todo #153 束の第13弾＝低10件の第8弾、probe prefix v402_1..v402_7）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v387 の残点 **V-387-1（fn `0x6CA440` の正式名 — テキスト Save か Debug dump か。FUN_ 名のみで正式名未特定）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内 RTTI/文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x6CA440` の所属** | **[確定(RTTI＋命令)]** | **`NiControllerSequence` の vtable slot12（+0x30）** — .rdata VA `0xA79A64`（extent19 slots・file 0x678C34..0x678C80）の **idx12** に格納、COL（`0xAC6170`）→ **TypeDescriptor `.?AVNiControllerSequence@@`**。E8=1（`0x49FF2A`＝override 内の super 呼び出し）＋packed dword＝vtable のみ |
| **正式名（Save vs Debug dump）** | **[確定(命令＋decompiled＋経路)]** | **Debug dump（テキスト表示用ダンプ）**。フィールド群を **`"name = value"` 形式のテキスト行**として**呼び出し元の文字列バッファ（NiStringPalette 系）へ出力**（13スカラ: `State`/`ArraySize`/`Weight`/`CycleType`/`Freq`/`Begin`/`End`/`Last`/`Offset`/`Start`/`End`/`WeightLast`/`LastScaled`）。**Save ではない [確定負]** — 根拠5点: ①浮動小数を **`%g`（`"%s = %g"`・6桁丸め＝ラウンドトリップ不能）** で出力（`FUN_0070fb90` L553209）②enum を**シンボル名**で出力（`FUN_006c6140`＝State の switch フォーマッタ L495833-495846／`FUN_00715910`＝CycleType）③**スカラ13フィールドのみ**（keys/controllers/配列なし＝再構成不能）④出力先＝**in-memory の文字列バッファ**（`[+8]/[+0xa]/[+0xe]` capacity チェック＋grow＝NiStringPalette 系）でファイルストリームではない ⑤**実使用経路が全て GUI インスペクタ**（下記） |
| **実使用経路** | **[確定(命令＋decompiled)]** | **Win32 ツリービュー系インスペクタ**: `FUN_00495af0`（L119846-119878）が **`SendMessageA(HWND([param_1+0xc]), 0x1100, 0, &item)`** でノードを張り、ラベル **`"Object Palette"`／`"Active Sequences"`／`"Inactive Sequences"`**（EXE 文字列 `0xA3DBE8`/`0xA3DBFC`）を切り替え、**`[param_3+0x40]` 配列（count `[+0x46]`＝NiControllerManager layout）を走査**して sequence を `FUN_00495120`/`FUN_00495270` で追加（state≠0＝Active／==0＝Inactive で振り分け）。**slot12 の hierarchy への呼び出しは `FUN_00495120`/`FUN_00495270`/`FUN_00495840`/`FUN_00495e30`/`FUN_004962a0`/`FUN_004964f0`（0x495120..0x4964f0）の inspector 家族のみ**（他の `+0x30` 呼び出しは別階層＝logger の "Report"/"Warning" 等）＝**Save 経路は存在しない [確定(否定側)]** |
| **`BSAnimGroupSequence` の override** | **[確定(RTTI＋命令)]** | **`0x49FF20`＝`.?AVBSAnimGroupSequence@@` の vtable idx12**（COL→RTTI・extent19 slots・file 0x63E9D4..0x63EA20）。中身＝**基底 `FUN_006ca440` を super 呼び出し（E8 `0x49FF2A`）→ pool `0xB35270` から追記 → `sprintf("AccumRoot = %s", [this+0x5c])`（`0xA3F858`）**。エラー文字列 `"Could not find AccumRoot in animation '%s'."`（`0xA3C7D0`・`FUN_00472030`）＝AnimGroup 読込系と接続 |
| （補足）vtable 体系 | **[確定(RTTI)]** | NiControllerSequence idx6=`0x6C87A0`（**Clone**＝v398 と整合）／idx11=`0x6C7BB0`（IsEqual 系）／**idx13=`0x68F970`＝no-op stub（`FUN_0068f970`＝`{ return; }`）**／idx15=`0x6CACE0`（v400 の 0xFF reset fn）／idx18=`0x68F970`。BSAnimGroupSequence は idx12 のみ override（idx6 は thunk `0x49F490`→`0x6C87A0`） |

**V-387-1 への回答**: `0x6CA440` の正式名＝**NiControllerSequence のテキスト表示用ダンプ（Debug dump・vtable slot12）[確定]**。**テキスト Save ではない [確定負]**（%g 丸め・enum シンボル名・スカラのみ・in-memory バッファ・使用経路が Win32 GUI インスペクタのみ）。

---

## §2 証拠

### §2.1 vtable 2本の RTTI（v402_4(a)/v402_6(b)）

| vtable | RTTI TypeDescriptor | slot12 |
|---|---|---|
| VA `0xA79A64` 周辺（file 0x678C34..0x678C80・19 slots） | **`.?AVNiControllerSequence@@`**（COL `0xAC6170`） | **idx12 = `0x6CA440`** |
| VA `0xA3F7D4` 周辺（file 0x63E9D4..0x63EA20・19 slots） | **`.?AVBSAnimGroupSequence@@`**（COL `0xAB8FE4`） | **idx12 = `0x49FF20`** |

### §2.2 ダンプ本体（v402_1(b)/v402_2(b) decompiled L499341-499433）

```
FUN_006ca440(this, buf):
  FUN_007009a0(buf);  header = FUN_0070f910([0xb3cb24]) → append
  "State"      ← FUN_006c6140(enum switch)  [this+0x44]
  "ArraySize"  ← FUN_0070fb40(int)          [this+0xc]
  "Weight"     ← FUN_0070fb90("%s = %g")    [this+0x1c]
  "CycleType"  ← FUN_00715910(enum)         [this+0x24]
  "Freq"/"Begin"/"End"(0xA51C18)/"Last"(0xA79DD8) ← %g  [this+0x28/2c/30/34]
  "Offset"/"Start"/"End"(0xA51C18)          [this+0x48/4c/50]
  "WeightLast"/"LastScaled"                 [this+0x38/3c]
  各行: capacity チェック([buf+8]/[+0xa]/[+0xe]) → FUN_004e4a10(grow) → FUN_0042bb90(append)
  return;   （L499433）
```
- **`FUN_0070fb90`（L553196-553210）**: `FUN_006c5d40(buf, n, "%s = %g", name, (double)value)`＝**%g 丸め出力**。
- **`FUN_006c6140`（L495833-495846）**: `switch(param_2)`＝**State のシンボル名フォーマッタ**。

### §2.3 GUI インスペクタ経路（v402_7(b) decompiled L119846-119878）

```
FUN_00495af0:
  if ([param_3+0x7c]) { local_1c = "Object Palette"; SendMessageA(hwnd,0x1100,0,&item); FUN_00495120(item, [param_3+0x7c]); }
  local_1c = "Active Sequences";   SendMessageA(hwnd,0x1100,0,&item);
  for i < [param_3+0x46]: seq=[param_3+0x40][i]; if (seq && [seq+0x44]!=0) FUN_00495270(item, seq);
  local_1c = "Inactive Sequences"; SendMessageA(hwnd,0x1100,0,&item);
  for i < [param_3+0x46]: seq=[param_3+0x40][i]; if (seq && [seq+0x44]==0) FUN_00495270(item, seq);
```
- `param_3` ＝ **NiControllerManager**（`[+0x40]` 配列＋`[+0x46]` count ＝ v395 layout）。
- slot12 呼び出し（v402_6(b)）: `FUN_00495120` L119484／`FUN_00495270` L119597／`FUN_00495840` L119787／`FUN_00495e30` L120049（`local_98`＝スタックバッファ渡し）／`FUN_004962a0` L120280／`FUN_004964f0` L120465-120588 — **全て0x495120..0x4964f0 の1家族**。

### §2.4 override 本体（v402_2(a)/v402_3 machine）

```
49FF20: push ebx/esi; esi=[esp+0xc]（buf）; push edi/esi; ebx=ecx
49FF2A: call 0x6ca440          ; ★super（NiControllerSequence::dump）
49FF2F: pool=[0xb35270] → FUN_0070f910 → append
49FF68: new(0x20); sprintf(0x98208b, "AccumRoot = %s" (0xA3F858), [ebx+0x5c]) → append
49FFAF: ret 4
```

## §3 残点 **V-402-1..3**
1. **slot12 呼び出しの第2引数（`puVar3`/`local_98`）の正体とダンプ結果の表示先**（ツリー子ノードか detail ペインか・`SendMessageA` 0x1100 のメッセージ意味と ITEM 構造体）【中】
2. **インスペクタ窓自体の同定**（`[param_1+0xc]` HWND の所有者＝どのデバッグ/検証窓か・`FUN_00495af0` の呼び出し元系列）【中】
3. **NiControllerSequence vtable idx14=`0x6C7470`／idx15=`0x6CACE0` のスロット意味**（隣接 virtual の役割）【低】

## §4 検証・自己批評
- probe: **v402_1..v402_7 全 EXIT=0**。
- **決定打の作り**: (i) machine＋decompiled で出力内容を全文取得（13スカラ＋"%s = %g"）→ (ii) **packed dword census で vtable2本の RTTI を確定**（NiControllerSequence/BSAnimGroupSequence idx12）→ (iii) **'Active/Inactive Sequences' 文字列の参照元＝SendMessageA(0x1100) GUI インスペクタと特定** → (iv) **slot12 呼び出し家族の全数 census（1家族のみ＝Save 経路なし）**。
- 妥当性チェック: ①%g⇔ラウンドトリップ不能⇔Save 否定 — ②enum シンボル名⇔表示用 — ③スカラのみ⇔再構成不能 — ④idx6=0x6C87A0⇔v398 の Clone — ⑤`[param_3+0x40]/[+0x46]`⇔v395 manager layout — **五方向で矛盾なし**。
- docs との不整合: **なし**（v387 の V-387-1 を解消。正式名＝**テキスト表示用ダンプ [確定]**、Save は **[確定負]**）。
- **教訓**: **「Save vs dump」は ①形式（%g/enum 名＝表示専用）②データ完全性（スカラのみ＝再構成不能）③呼び出し家族（GUI インスペクタのみ＝保存経路なし）の3点で裁定**＋**vtable extent＋RTTI COL で slot の所属を一撃確定**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
