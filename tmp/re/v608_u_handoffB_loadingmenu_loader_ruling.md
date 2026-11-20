# v608 — 依頼 B 裁定: LoadingMenu のロード対象リスト構築元（V-597-2）と時間単位（V-597-1）＝**ロード対象＝`[0xB33A98]+0x9C` のレコード連結リストから formType フィルタ＋乱択／時間単位は ms／user1 接頭辞は `Textures\Menus\Loading\`（v597 記述の [確定負] 訂正）**

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u22_1/u23_1/u23_2**・全 EXIT=0・冒頭 Test-Path）＋ `docs/Oblivion_decompiled.c`（`FUN_005ad440` @L…/`FUN_005ad780`/`FUN_005adcf0`/`FUN_0065c620`/`FUN_004f96f0`）。
先行: `tmp/re/v597_u11_loadingmenu_trait_suppliers_ruling.md`。
依頼書: `tmp/re/handoff_20261120_movie_fit_loading_slideshow_chargen_pages.md` 依頼 B（ai-todo #96）。
判定: `[確定(命令)]` / `[確定負]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| **B-1 / V-597-2** ロード対象リストの構築元 | **[確定(命令)]** | 構築は **`FUN_005AD440(this=LoadingMenu, targetRecord)`**（callers 3: `0x5AD734`／`0x5ADCF0`＝ローダ／`0x5ADEC0`＝セクション開始）。**グローバル `[0xB33A98]+0x9C` の linked list**（TESDataHandler のレコード集合）を走査し、**`targetRecord` の formType で分岐**（`*(byte*)(param_2+4)`: `0x30`→`FUN_004C9CF0`／`0x31`/`0x32`/`0x33`→`FUN_004D6670`／`0x35`→`param_2` 自身）して候補集合を作り、**フィルタ `FUN_004F9BB0`/`FUN_004F9A20` で3分類（local_28/local_24/local_2C）**、各分類を **乱数 `FUN_0047DF80`** で1件選び **`[LoadingMenu+0x4C]` の連結リストへ追加**（要素＝レコードポインタ、`FUN_00401F00(8)` でノード確保、`FUN_0065C620` で重複除去）。→ **ロード対象＝`[0xB33A98]+0x9C` のレコード**（TESForm）。 |
| **B-1 続き** user1/user2 の実体 | **[確定(命令)]** | 各要素について **名前＝レコードの `[+0x1C]`（ファイル名/パス、null なら空）**、`'Menus\Loading'` を `_strstr` で含まなければ **`FUN_004F96F0`＝`"Textures\Menus\Loading\"` を前置**（`'%s%s'`）。**user2＝レコードの vtable+0x10 に `'DESC'`(0x43534544) を渡して取得した説明文**（末尾改行 0x0A を最大3個除去）。 |
| **B-2 / V-597-1** 時間単位 | **[確定(命令)]** | **ms（ミリ秒）**。`FUN_005AD980` は `(now_ms − [this+0x68])`（`0x5AD9CD`）を **`fdiv [0xA2FC70]=1000.0`**（`0x5ADA1E`）で秒に換算してから `[+0x40]` の残量を減算。しきい値 **`[0xB14160]=2500`（ms）**／**`[0xB14168]=100`**／**`[0xB14170]=10`** も ms。→ `0xB14160`=2500 は **2500ms＝2.5 秒**（セクション滞留の上限）。 |
| **B-3** user0=0（タイトル前）で load_main が出ない | **[確定(命令)]** | `FUN_005AD440` 冒頭: **`[0xB33A98]==0` なら `SetFloat(0xFAE, 0)` して return**（＝ロード対象集合が無い → user0=0 → `load_title_page` のみ）。対象がある場合のみ `0x5AD780` が各要素で user0 を +1（`>0` → `load_main`）。→ **v597 §2.2 の 0/1 切替は実効**。 |
| **B-訂正（重要）** user1 の接頭辞 | **[確定負]（v597 訂正）** | v597 §2.3/§3 は user1 を **`'Menus\Loading'`＋名前**と記したが、実測では **`0x4F96F0` の戻り＝`"Textures\Menus\Loading\"`（`0xA498F8`）**であり、**user1＝`'Textures\Menus\Loading\'`＋ファイル名**。`'Menus\Loading'`（`0xA6C4B8`）は **`_strstr` の判定用リテラル**（既に接頭辞を含む名前を再前置しないためのチェック）であって接頭辞そのものではない。**移植は `Textures\Menus\Loading\` を使うこと**。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-597-2 ロード対象リスト構築（`FUN_005AD440`・decompiled）

```
FUN_005ad440(this, targetRecord):
  if ([0xB33A98]==0) { SetFloat(0xFAE, 0); return; }            ; ★ 対象なし→user0=0
  if ([this+0x50]==0 && [this+0x4C]==0) {
    formType = (targetRecord) ? *(byte*)(targetRecord+4) : 0
    switch (formType) { case 0x30: alt=FUN_004C9CF0()          ; 0x31/32/33: alt=FUN_004D6670() ; 0x35: alt=targetRecord }
    node = [0xB33A98+0x9C]                                      ; ★ レコード集合の linked list
    do {
      rec = *node
      if (rec != 0) {
        if (FUN_004F9BB0(targetRecord)==0) {                    ; 分類①: 未ロード？
          if (alt==0 || FUN_004F9BB0(alt)==0) goto next
          local_28++
        } else {
          if (FUN_004F9A20()==0) { if (targetRecord==0) goto next; local_24++ }
          else local_2C++
        }
      }
    next: node = node[1]
    } while (node != 0)
    ; 各分類から乱数 FUN_0047DF80 で1件選び [this+0x4C] へ連結追加
    if (*piVar1==0) *piVar1=rec ; else { n=FUN_00401F00(8); n[0]=rec; n[1]=0; piVar1[1]=n; }
    FUN_0065C620(*node)                                         ; 重複除去
  }
```
- `[0xB33A98]`＝TES データハンドラ系グローバル（v558/v572 の「TESRace リスト `[0xB33A98]+0x44`」と同一 global）。
- `[this+0x4C]`/`[this+0x50]`＝LoadingMenu のロード対象連結リスト head/tail。
- `FUN_0065C620`（`0x65C620`）＝**連結リストから指定要素を除去**するヘルパ（decompiled: `while (*piVar3 != param_2) piVar2=piVar3[1]...`）。

### 2.2 user1/user2（`FUN_005AD780`・decompiled）

```
iVar6 = *piVar4                                              ; 選択された record
puVar7 = *(byte**)(iVar6+0x1C) ; if (null) puVar7 = &DAT_00a2f7ec("")
FUN_004028d0(puVar7,0)
pcVar8 = _strstr(local_21c, "Menus\\Loading")                ; 既に接頭辞を含むか
if (pcVar8==0) { uVar9 = FUN_004F96F0(puVar7)                ; ★ "Textures\Menus\Loading\"
                 FUN_00402E30(&local_21c, "%s%s", uVar9, name) }   ; ★ 前置
FUN_0058CED0(0xFAF, pcVar13)                                 ; user1 = その文字列
pcVar10 = (**(code**)(*(int*)(iVar6+0x24)+0x10))(0, 0x43534544)  ; ★ record vtable+0x10 に 'DESC'
... 末尾 0x0A を最大3個除去 ...
FUN_0058CED0(0xFB0, acStack_210)                             ; user2 = DESC 文字列
fVar14 = FUN_00588BD0(0xFAE); SetFloat(0xFAE, fVar14 + 1.0)  ; ★ user0 += 1
```
- `FUN_004F96F0`（`0x4F96F0`）＝ **`mov eax, 0xA498F8; ret`** の定数 getter。`0xA498F8` 実測＝**`"Textures\Menus\Loading\"`**。
- callers of `0x4F96F0` ＝ **`0x5AD86E`（`FUN_005AD780` 内）の1件のみ**。

### 2.3 時間単位（`FUN_005AD980`・命令逐語）

```
5AD9CD mov eax, ebp ; sub eax, [esi+0x68]     ; 経過 ms
5AD9D7 cmp eax, [0xB14160](=2500) ; jle ...   ; 2500ms しきい値
5ADA09 test ecx,ecx ; 5ADA0E [esp+0x14]=ecx
5ADA12 fild [esp+0x14]                         ; (now − [esi+0x6C])
5ADA18 fadd [0xA2FC78]  (負値補正)
5ADA1E fdiv qword [0xA2FC70] (=1000.0)         ; ★ ms → 秒
5ADA24 fsubp st(1)                             ; [esi+0x40] -= 秒
```
- `0xA2FC70`（f64）＝**1000.0**（実測）。
- `[esi+0x68]`/`[esi+0x6C]`＝前回タイムスタンプ（ms）、`[esi+0x40]`＝現在セクションの残り秒。

---

## 3. 移植への指針（依頼 B）

1. **user1 の接頭辞は `Textures\Menus\Loading\`**（v597 の `Menus\Loading` は誤り）。`'Menus\Loading'` は `_strstr` 判定用。
2. 進捗は **ms 基準**（`0xB14160`=2500ms ほか）。`0x5AD980` は `(now−last)/1000` 秒で `[+0x40]` を減算。
3. ロード対象は原版の **`[0xB33A98]+0x9C` レコード集合**（TESForm）。**port に「ロード対象レコード」概念が無い**なら、**user0=0 の `load_title_page` のみ**を再現するのが正典範囲（user1/user2 のスライドショーはロード対象レコードが無いと成立しない）。スライドショー再現には port 側でレコード相当（ロードする CELL/WRLD 名＋DESC）を供給する必要がある。
4. user2（DESC）は **レコードの `DESC` サブレコード**。固定文字列ではない。

---

## 4. 残点

- **V-608-1【低】**: `FUN_004F9BB0`/`FUN_004F9A20` の3分類（local_28/24/2C）の意味（ロード済み／未ロード等の正確な述語）。
- **V-608-2【低】**: formType `0x30`/`0x31`/`0x32`/`0x33`/`0x35` のレコード種別名（EXE に formType 名テーブルがあれば確定可能）。
- **V-608-3【低】**: `FUN_004C9CF0`/`FUN_004D6670` の役割（formType 別の代替レコード取得）。
- **V-597-3【低】**（未消化のまま）: セクション順序の規則。

---

## 5. 検証

- probe: **u22_1／u23_1／u23_2（3本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x5AD440`（`[0xB33A98]+0x9C` 走査・formType switch）／`0x5AD780`（`0x5AD86E call 0x4F96F0`）／`0x4F96F0`（`mov eax,0xA498F8; ret`）／`0x5AD980`（`0x5ADA1E fdiv [0xA2FC70]`）／`0x65C620`（リスト除去）。
- 文字列実測: `0xA498F8`=`"Textures\Menus\Loading\"`／`0xA6C4B8`=`"Menus\Loading"`／`0xA2F884`=`"%s%s"`。
- 定数実測: `0xA2FC70`=1000.0(f64)／`0xB14160`=2500／`0xB14168`=100／`0xB14170`=10／`0xB14158`=15.0f。
- 反証（v597 訂正）: `0x4F96F0` の callers は `0x5AD86E` の1件のみ・戻りは `Textures\Menus\Loading\`。
