# v586 — Lane C 統合＋矛盾の解消: `0x587550` は多分岐 switch（Lane C の negative を [確定負] に訂正）・`0x416`=QuickKeysMenu・`0x578D70`/`0x579C10` の契約（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（Lane C probe **v601_1〜v601_10**＋本レーン検証 **v586_1〜v586_3**、全 EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v584_chargen_gate_ruling.md`（V-584-2/3/4 の起票元）／`tmp/re/v574_hud_open_path_ruling.md`（本裁定で訂正される記述を含む）
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 矛盾の解消（最重要）: `0x587550` は多分岐 switch である [確定(命令)]

Lane C は「`0x587550` は 35 命令で `ret` し switch ではない（`0x3FF`/`0x3FE` の 2 if のみ・`0x416` case なし）」と報告したが、**これは線形 decode を「最初の `ret 4` で打ち切った」デコードアーティファクト**である。本レーンで size 付き逐語で再検証した結果:

```
0x587550: push esi; push edi; mov edi,[esp+0xc]      ; edi = 窓 id（第1引数）
          0x589B70(edi) → vtable[0](1)               ; 既存インスタンスがあれば破棄
          0x58756F: cmp edi, 0x3FF → jne 0x5875B3     ; case 0x3FF
                     → 0x579F80 … ret 4              ; ★case ごとに個別 ret 4
          0x5875B3: cmp edi, 0x3FE → … ret 4         ; case 0x3FE
          0x5875F7: cmp edi, 0x3EA → … ret 4         ; case 0x3EA
          0x58763B: cmp edi, 0x3EB → … ret 4         ; case 0x3EB
          0x58767F: cmp edi, 0x3EC → call 0x5A6B00 → ret 4   ; case 0x3EC（HUDMainMenu）
          0x587693: cmp edi, 0x3ED → call 0x5A4840 → ret 4   ; case 0x3ED（HUDInfoMenu）
          0x5876A7: cmp edi, 0x3F2 → call 0x5A8E30 → ret 4   ; case 0x3F2（HUDSubtitleMenu）
          0x5876BB: cmp edi, 0x3EF → push 0; call 0x5ADCF0 → ret 4  ; case 0x3EF（LoadingMenu）
          0x5876D4: cmp edi, 0x414 → call 0x5B6040 → ret 4   ; case 0x414（MainMenu）
          0x5876E8: cmp edi, 0x3F0 → 0x589B70(0x3F0)+cast+0x599EE0  ; case 0x3F0
          … 以降 0x402 等も継続
```

* **`find_head(0x5875B3)=find_head(0x5876E8)=0x587550`**（＝全 case が同一関数 head 0x587550 に属する）を実測で確認。
* したがって **v574/v581 の「`0x587550`＝窓 id → 生成ヘルパの switch」は正しい**。Lane C の negative は **[確定負] に訂正**（`0x416` case が無いのは事実だが、それは「switch ではない」ではなく「0x416 は別経路」を意味するだけ）。
* **新教訓（ハーネスの罠・再発型）**: **switch 関数が case ごとに `ret` を持つ（early-return 型）場合、線形 decode を「最初の ret」で止めると関数を誤って短く見積もる** → **関数境界は `find_head`（named_funcs の bisect）で判定し、線形 decode の ret 打ち切りは使わない**。

---

## 2. Lane C の positive 成果（採用）[確定(命令)]

| 問い | 判定 | 結果 |
|---|---|---|
| 窓 id `0x416`(1046) のクラス | **[確定(命令)]** | **`QuickKeysMenu`**。`0x587D20` switch の `0x416` case → `0x5886F7: push 0x50; call 0x401F00（new）; call 0x5C0B50`（ctor が vtable `0xA6D1F4` 設定）。近傍文字列 `0xA699C4 'quickkeys_menu.xml'`／`0xA6A681 'QuickKeysMenu;'`／`0xA6D250 '.\Interface\Menus\QuickKeysMenu.cpp'`。 |
| `0x578D70` の戻り値契約 | **[確定(命令)]** | `singleton=[0xB3A6E0]` を `0x582160(1,0)` で取得 → **`bl=[eax+0xB0]`（旧値）→ `[eax+0xB0]=0xFF`（クリア）→ `al=bl`（旧値返却）**。`[+0xB0]` は ctor `0x5802B0` の `0x58042B: mov byte [esi+0xB0],0xFF` で初期化＝**`0xFF`＝未応答**。 |
| `0x579C10` の引数契約 | **[確定(命令)]（v570 サイトで実測）** | **6 引数**（v570 の Random サイト `0x5CC949`-`0x5CC966`: `push 0; push edx=[0xB38D00]=sNo; push eax=[0xB38CF8]=sYes; push 1; push 0x5C9FC0(callback); push ecx=[0xB39450]=sRandomizeFace; call 0x579C10; add esp,0x18`）→ **`0x579C10(text, callback, flag, yes, no, ?)`**。内部は `FUN_005BC8B0` を **5 引数**で call（`add esp,0x14`）。 |
| `FUN_005ABBC0` の通知 id 分岐 | **[確定(命令)（negative）]** | **5 分岐ではない**。`push 0x416; 0x589B70`（QuickKeysMenu 取得）→ `edi=[esp+0x24]`（通知 id）→ **`lea eax,[edi-1]; cmp eax,4; ja 0x5ABC02`**（＝`id∈[1,5]` の**単一条件ブロック**）→ `0x5AACF0(id)`（QuickKeys のホットキー番号割当）。`id∈{7,8}` は別ブロック `0x5AC30D`、他は `0x5ABCD3` 以降。**v583 の「通知 id 1..5 の5分岐」記述を訂正**。 |
| `[0xB3B3D9]` フラグ | **[確定(命令)]** | `.text` 参照5件（うち `0x5AB785` は命令境界でない false positive）。実体4件: **WRITE 0**＝`0x5A9B24`（ctor `0x5A9B00`）/`0x5ABBE2`（`FUN_005ABBC0`）／**WRITE 1**＝`0x5A9C26`（`0x5A9BE0`）／**READ**＝`0x5AC511`（`FUN_005ABBC0`・`cmp byte [0xB3B3D9],0`）。 |

---

## 3. V-584-2（begin/end の identity）[確定(命令)]

| 関数 | 役割 | 触る singleton メンバ |
|---|---|---|
| **`0x583E60`（begin）** | `[esi+8]==3` なら **`[esi+8]=5`**（実行中）＋ `[esi+0x1c]`（メニューマネージャ）取得＋フラグ bit0 クリア＋ `0xFA1`/`0xFE6`(`cursor.dds`)/`0x58E870`/`0x57D940`/`0x5A4980`/`0x585C10`/`0x585820`/`0x5A6040(0,1)`/`0x589B70(0x3ED)` → `0x584740` へ tail-jump。`[+8]!=3` かつ `==5` なら `[+8]=2`。 | `+8`, `+9`, `+0x1c`, `+0x88` |
| **`0x583F40`（end）** | `[edi+9]=0` リセット → `[edi+0x68]` 配下走査 → 選択スロット `[+0xE0]`/`[+0xE4]` 評価（`0x3F3`/`0x3E9` 比較）→ `[+0x24]=4`（終了）→ `0x588BD0(0x1772)` → `[edi+8]=1`（idle 復帰）。 | `+8`, `+9`, `+0x68`, `+0xE0`, `+0xE4` |
| 3連呼構造 | **`0x5791A0→begin`／`0x5791E0→execute`／`0x579220→end`**（3薄ラッパは完全同一形） | — |

---

## 4. 残点（引き継ぎ）

| ID | 内容 | 重要度 |
|---|---|---|
| **V-587-1** | **`0x578D70` の `al==1`/`al==2` の肯定/否定対応**と `[+0xB0]` の書き手（`1`/`2` を書く命令）。Lane C は「singleton 経由で `[+0xB0]` を書く命令」を検出できず → **`[+0xB0]` の writer を `0x582160` 全 head で線形走査** | 中 |
| V-587-2 | `0x579C10` の `flag`（arg3）の意味（1/0 が何を選ぶか）。`FUN_005BC8B0` の `[+0x60]` 保存を読む箇所を走査 | 中 |
| V-587-3 | `0x587550` switch の全 case 列挙（`0x3F0` 以降の続き）と各生成ヘルパの identity（V-574-2 の続き） | 低 |
| V-587-4 | `0x5AD980`（end のみ直接 caller）の begin/execute 呼び出し箇所 | 低 |

---

## 5. probe 一覧

**Lane C**: `v601_1`〜`v601_10`（`0x583E60`/`0x583F40` の full decode＋3連呼実読／`0x416`=QuickKeysMenu（`0x587D20` case＋vtable `0xA6D1F4`）／`FUN_005ABBC0` id ブロック／`[0xB3B3D9]` writers／`0x578D70` full decode＋callers 30件の `al` 比較／`0x579C10` prologue＋callers の push 列／`FUN_005BC8B0` prologue）。全 EXIT=0。
**本レーン検証**: `v586_1`（`0x587550` 線形 decode＝35 命令で ret＝**アーティファクトの再現**）／**`v586_2`（size 付き逐語＝`0x5875B3`/`0x5875F7`/`0x58763B`/`0x58767F` の case 連鎖を確定＋`find_head` が全て 0x587550 を返すことを実測）**／`v586_3`（`0x587550` prologue・`0x579C10` prologue・`0x5BC8B0` prologue・**v570 Random サイトの 6 push 実測**）。全 EXIT=0。
