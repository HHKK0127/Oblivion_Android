# v613 — v612 検収と訂正: NiTArray の count フィールド（firstFreeEntry vs numObjs）・vtable 妥当性検算・Tile 値レイアウトの writer 裏取り・名前の出所分類・probe 改訂

対象: `Oblivion.exe` v1.2.0416 (Steam, md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`・PE timestamp `0x462392c7`)。
根拠: 我々の実読 probe **u48_1 / u49_1 / u50_1**（EXIT=0）＋ **xOBSE `obse/obse/NiTypes.h` の宣言（一次資料）** ＋ OBVR `GameAddresses.h`。v612 の検収。
判定: `[確定(命令)]` / `[確定(外部+実読)]` / `[参照]`

---

## 1. 指摘への回答（結論）

| # | 指摘 | 判定 | 結論 |
|---|---|---|---|
| 1 | NiTArray の count offset が疑わしい | **[確定(命令)] 指摘は正しい** | `0xB1397A`=**firstFreeEntry**（+0x0A）／`0xB1397C`=**numObjs**（+0x0C）。**v612 の「count u16 @ 0xB1397A」は誤り**。ただしエンジン `0x589B70` の境界判定は firstFreeEntry を使う（実バイト確認）。 |
| 1' | m_pBase の二段読み | **[確定(命令)] 問題なし** | probe は `data = u32(0xB13974)`（＝`[0xB13974]` の値）で二段読み済み。 |
| 2 | Tile レイアウトは ReadXML で裏取りを | **[確定(命令)] 実施** | **`0x58B220 Tile::GetValue`／`0x58CA00 Value::SetNumber`（書く本人）**から値リスト `+0x18`／Value レイアウトを確定。parent `+0x10` は `0x5898F0`、child `+0x34` は `0x589930` で確認。ReadXML `0x590420` は補助。 |
| 2' | 子リストの parent 不一致は黙って落とすな | **[確定(命令)] 反映** | probe に「総ノード数／採用数」の記録と**差分検出で停止**（`--allow-mismatch` で継続）を実装。 |
| 3 | vtable 妥当性検算 | **[確定(命令)] 全通過** | `0xA6D38C` COL→`0xAC2AF4`→TypeDescriptor **`.?AVRaceSexMenu@@`**、15 slots すべて `0x5Cxxxx`（**slot[3]=`0x5CC200`**＝v610 の Done dispatch）。`0xA6C40C` COL→`0xAC2504`→**`.?AVLoadingMenu@@`**。 |
| 4 | RaceSexMenu 15 slots = Menu と同数 | **[確定(命令)] そのとおり** | オーバーライドが vtable に出ない＝**chargen の意味は vtable に無い**。UI は XML、C++ は user trait を埋めるだけ。本命は「user\* を書く箇所」。 |
| 5 | 名前の出所分類が未処理 | **[確定(命令)] 実施** | §6 の表。**`0x589B70` は自前命名**、`0x5C2B36` は **hook_site（関数ヘッドでない）**、`0x58CEB0` は関数ヘッド。 |
| 6 | V-612-2 の母集団差 | **[方法] 反映** | キーは名前・値一致を oracle にしない・不一致は `[部分確定]`・`--esm` 分離（§7）。 |
| 7 | 順序（V-612-1 優先） | **[決定]** | runtime 依存。代替＝`0x584880 Menu::RegisterTile` に `menuId==0x40C` フィルタのブレークポイント（§8）。 |

---

## 2. NiTArray の count フィールド [確定(命令)]

**一次資料（xOBSE `obse/obse/NiTypes.h`）**:
```cpp
template <typename T> struct NiTArray {
    void** _vtbl;          // 00
    T*     data;           // 04
    UInt16 capacity;       // 08 - init'd to size of preallocation
    UInt16 firstFreeEntry; // 0A - index of the first free entry in the block of free entries at the end
    UInt16 numObjs;        // 0C - init'd to 0
    UInt16 growSize;       // 0E - init'd to size of preallocation
};
```

**我々の実読**:
- `0x589B70`: raw `0FB70D 7A39B100` = **`movzx ecx, word ptr [0x00B1397A]`** → エンジンは **firstFreeEntry(+0x0A)** で境界判定（`cmp ecx, index; jbe -> return 0`）。
- `0x42BB90 AddAtIndex(index, item)`: `movzx edx, word [ecx+0xA]`（firstFreeEntry を読む）→ `index >= firstFreeEntry` なら `firstFreeEntry = index+1` に更新。**`add word [ecx+0xC], 1`（numObjs を +1）**。`mov ecx,[ecx+4]`（data）; `mov [ecx+index*4], item`。

**帰結**:
- **`0xB1397A` = firstFreeEntry**（＝エンジンが境界に使う、使用済み範囲の上端）。静的値 0（runtime で埋まる）。
- **`0xB1397C` = numObjs**（＝真の要素数）。
- **OBVR `kTileMenuArrayCount = 0x00B1397A` は誤称**（firstFreeEntry を "count" と命名）。ただし**エンジンと同じ境界**なので動作は一致。firstFreeEntry ≥ numObjs（要素削除で穴が空いても firstFreeEntry は戻らない）＝ firstFreeEntry 内でもスロットが null のことがある → **必ず null チェック**。
- v612 の「count u16 @ 0x00B1397A」を **[確定(命令)（negative）] 訂正**。

---

## 3. vtable 妥当性検算 [確定(命令)]

| vtable | [vtable-4] COL | TypeDescriptor | slots |
|---|---|---|---|
| `0xA6D38C`（RaceSexMenu） | `0x00AC2AF4`（sig=0 offset=0 cd=0） | **`.?AVRaceSexMenu@@`** | 15 slots すべて `0x5Cxxxx`。`[0]=0x5C4E20`（ctor）・**`[3]=0x5CC200`（v610 Done dispatch）**・`[10]=0x5C2790`・`[12]=0x5C3DD0`・`[13]=0x5C4BF0` |
| `0xA6C40C`（LoadingMenu） | `0x00AC2504` | **`.?AVLoadingMenu@@`** | `[0]=0x5AE060`・`[11]=0x60D0A0`（Tick の tail jump 先）・`[13]=0x5AD370`（ctor 圏） |

→ **RTTI 同定は完了**。`0xA6D38C` は別物ではない（COL・型名・スロット域が一致）。

---

## 4. Tile レイアウトの writer 裏取り [確定(命令)]

「書く本人」から確定（OBVR 定数と独立に一致）:

### 4.1 `0x58B220` = `Tile::GetValue(trait)`
```
ecx = [this+0x18]                    ; ★値リスト先頭ノード (Tile+0x18)
walk: v = [node+0x08]; if (v) { if (word[v+0x18]==trait) return v; if (>trait) break } node=[node+0x00]
not found: Value = alloc(0x1C); [Value+0x00] = this (owning tile); insert into list at [this+0x14]
```
→ **`Tile+0x14`=値リスト本体(NiTListBase)／`Tile+0x18`=先頭ノード**、**node `{+0x00 next, +0x08 data}`**、**Value `+0x00`=owning tile**、**`Value+0x18`=id(u16)**、Value サイズ **0x1C**。

### 4.2 `0x58CA00` = `Value::SetNumber(value)`
```
[esi+0x1A] = 1                        ; ★isNumber = 1
if (new != old) { if (word[esi+0x18] != 0xFDE) ...; FUN_004028d0(esi+8, ""); [esi+4] = new; ... }
```
→ **`Value+0x04`=number(f32)／`Value+0x08`=string(BSStringT)／`Value+0x18`=id(u16)／`Value+0x1A`=isNumber(u8)**。

### 4.3 `0x58CA50` = `Value::SetString`
- `[esi+0x1A] = 0`（isNumber=0）→ 再度 `+0x1A`=isNumber を確認。

### 4.4 親・子
- **`Tile+0x10`=parent**：`0x5898F0 GetContainingMenu` のループが `mov eax,[esi+0x10]` で親方向に歩む（u45_1 実読）。
- **`Tile+0x34`=子リスト先頭ノード**：`0x589930`（tile 名前検索）が `mov esi,[ecx+0x34]`（u46_1 実読）。
- **補助**：`Tile::ReadXML 0x590420` head は `0x58DBD0`(parse)・`0x590330`・`0x58CF40`（1 tile 分の読込）を呼ぶ。値/子の書込はこの先の helper 側（残点 V-613-4）。

### 4.5 子リスト差分検出（probe 反映）
走査時、各階層で「**リスト総ノード数**」と「**`parent==tile` で採用した数**」を記録し、差分が出たら **STOP**（`--allow-mismatch` で継続）。レイアウト誤りを黙って落とさない。

---

## 5. RaceSexMenu の vtable と chargen の意味 [確定(命令)]

`0xA6D38C` は `Menu`（`0xA696DC`）と同じ 15 slots。**chargen 固有のオーバーライドは vtable に出ない** → **chargen の意味は vtable に無い**。UI は XML（104 ファイル）が持ち、C++ は `user0..N` trait を埋めるだけ（compendium ノート・我々の v610 と一致）。→ **木を歩いた後の本命は「各 tile の `user*` 実値」と「それを書く箇所」**。vtable 比較でクラスを区別しようとすると空振り。

---

## 6. 名前の出所分類 [確定(命令)]

我々が引用した名前の **kind（head/site/data）× source** を明示（`classify_names.py`）。

| アドレス | 名前 | kind | source | 我々の扱い |
|---|---|---|---|---|
| `0x589B70` | （GetTileMenu） | — | **自前命名（OBNG に無し）** | **我々の提案名**。実読で確定した関数ヘッド |
| `0x58CEB0` | `kTileUpdateFloat` / `Tile::UpdateFloat::callAddr` | site(OBVR) / function(xobse) | OBVR / xOBSE | **実読＝関数ヘッド**（leaf・`ret 8`）。v611 の表記は妥当 |
| `0x5C2B36` | `EventManager::kChargenPatchAddr` | **hook_site** | xOBSE | **関数ヘッドではない**（フックする中間アドレス）。v611 の「実名」欄は site と明記すべき |
| `0x5898F0` | `Tile::GetContainingMenu` | function（**inferred**） | xOBSE | 関数ヘッド・推論名 |
| `0x582160` | `InterfaceManager_GetSingleton` | function | xOBSE | 関数ヘッド |
| `0x578F60` | `IsMenuMode` / `kIsMenuMode` | function(xobse) / site(OBVR) | xOBSE / OBVR | 関数ヘッド |
| `0xB13970` | `g_TileMenuArray` / `kTileMenuArray` | **global**(xobse) / site(OBVR) | xOBSE / OBVR | **データ**（配列オブジェクト本体） |
| `0xB13974` | `kTileMenuArrayData` | site | OBVR | データ（m_pBase フィールド） |
| `0xB1397A` | `kTileMenuArrayCount` | site | OBVR | データ・**誤称**（firstFreeEntry） |
| `0xB1397C` | （numObjs） | — | 未命名（xOBSE struct 定義） | **我々の追加**（v613） |
| `0xB13348` | `RTTI_RaceSexMenu` | rtti_type_descriptor | xOBSE | データ |
| `0x42BB90` | `AddAtIndex` | function（inferred） | xOBSE | 関数ヘッド（NiTArray::AddAtIndex） |
| `0x5AD980` / `0x5AD0D0` | `LoadingMenu::Tick` / `LoadingMenu::LoadingMenu` | function | OBNG slice CSV | 関数ヘッド |
| `0x5CA010`/`0x5C3440`/`0x5C4340`/`0x5AD440`/`0x5AD780`/`0x5CC200`/`0x5C9650`/`0x5C2710`/`0x5C4E20`/`0x60D0A0` | （各） | — | **自前命名／未命名** | 我々の実読由来。出所を都度明記 |

- **xOBSE 由来＝GPL**、**OBVR 由来＝GPL**（事実参照のみ）、**compendium＝MIT**（スクリプト使用可）。自前命名は我々の実読根拠を付す。

---

## 7. V-612-2（gmst 突合）の方法論 [方法]

- **母集団が違う**：gmap＝登録バンド `0x9F6000..0x9F8000` の 218 件、`gmst_defaults.py`＝静的初期化子 378/382 件。片方にしか無い名前は矛盾ではない。
- 突合キーは**名前**。アドレスは比較材料であって一致必須にしない。
- **値の一致を oracle にしない**（compendium gotcha 5：exe 既定値は master の placeholder を上書きするので大半不一致）。
- 不一致は negative ではなく **`[部分確定]`** に落とす。
- `--esm` を渡すと master 値が混ざる → **exe 既定値だけ見たいなら分けて出す**。

---

## 8. 順序（V-612-1 優先）[決定]

- **V-612-1（実機走査）は runtime の用意に完全依存**。用意できるなら V-612-2 より先。木が取れれば `user*` 実値が一気に埋まる。
- **代替（runtime が重い場合）**：`0x584880 Menu::RegisterTile` にブレークポイントを置き、**`menuId==0x40C` で絞って登録時の tile を記録**。メニューを開き切らなくても、新規ゲームで chargen が組まれる過程で発火。走査より軽く同じ情報が取れる。

---

## 9. 残点

- **V-613-1【中】**: probe `u47_1.py` の実機走査（`--pid`）。静的には配列が空。
- **V-613-2【中】**: `gmst_defaults.py` と gmap の突合（§7 の方法論で）。
- **V-613-3【中】**: `0x584880 Menu::RegisterTile` ブレークポイント方式（§8 の代替）。
- **V-613-4【低】**: `Tile::ReadXML 0x590420` の値/子リスト書込（`0x58CF40`/`0x590330` 系）の完全 decode。

---

## 10. 検証

- probe **u48_1 / u49_1 / u50_1**（EXIT=0・冒頭 Test-Path）。
- 一次資料: xOBSE `obse/obse/NiTypes.h`（NiTArray 宣言・`AddAtIndex` は `ThisStdCall(0x0042BB90,...)`）。
- 実バイト: `0x589B70`=`0FB70D 7A39B100`（`movzx ecx, word [0xB1397A]`）／`0x42BB90`（`[+0xA]` 更新・`[+0xC]` +1）。
- vtable: `0xA6D38C` COL `0xAC2AF4`→`.?AVRaceSexMenu@@`／`0xA6C40C` COL `0xAC2504`→`.?AVLoadingMenu@@`。
- Value: `0x58CA00`（`[+0x1A]=1`・`[+0x04]`・`[+0x18]==0xFDE`）／`0x58B220`（`[this+0x18]`・`alloc 0x1C`）。
- probe 改訂: `u47_1.py`（firstFree/numObjs 併記・二段読み・子リスト差分検出）。静的実行で `firstFree=0 numObjs=0`。
- 出所分類: `classify_names.py`（OBNG `re/symbols/*.csv` と突合）。
