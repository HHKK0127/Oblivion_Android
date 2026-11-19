# v574 — V-562-3 の [部分確定]: chargen 中に HUD(0x3EC) が開かないことの命令的裏付け①＝open 経路の構造と XML 非参照の確定（ai-todo #156 系フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v574_1〜v574_5**・5本 全 EXIT=0）。
先行: `tmp/re/v562_u6_u4_assets_topleft_ruling.md`（V-562-3 の起票元・XML レベルの非存在は v562 で [確定] 済）／`tmp/re/v550_safezone_tilemenu_ruling.md`（TileMenu vtable・close ゲート）／`tmp/re/v544_menu_classes_ruling.md`（窓 id↔クラス47件）
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| メニュー open の総入口は何か | **[確定(命令)]** | **`TileMenu::open 0x591C50`（vtable +0x14・E8 は 0 件＝vtable 経由のみ）→ `0x591C76: call 0x587D20`**。`0x587D20(menuId)` = **id 1001..1051 の switch**（`eax=id-0x3E9 ≤0x32` → `jmp [eax*4+0x5887b8]` → 各 ctor 群）。**`0x587D20` の呼び出しはこの1箇所のみ**（E8 精査）。HUDMainMenu ctor `0x5A6780` の唯一の呼び出しも switch の case（`0x587E95`）。 |
| HUD をコードから開く経路の構造 | **[確定(命令)]** | open(0x591C50) → switch(0x587D20) → case 1004 → `0x5A6780` ctor。**`push 0x3EC` は25件あるがすべて `0x589B70`（既存インスタンス取得）か `&HUDMainMenu;` の XML エンティティ登録（`0x58A489`）で、「open を直接呼ぶ」箇所は0件** → **HUD の open は `call [reg+0x14]`（vtable 経由・281件）で、id の直 push は無い**（=引数はインスタンス固有・open の呼び出し直前に menu id が出ない）。 |
| XML から HUD が開かないこと | **[確定]（v562 済）＋[確定(命令)]（本裁定）** | v562: title/chargen の XML に HUD 要素なし。本裁定: **`&HUDMainMenu;`＝窓 id `0x3EC` を割り当てる XML エンティティ**（`0xA6A8CC`・登録 `0x58A489: push 0x3ec; push '&HUDMainMenu;'; call 0x58A0A0`）であることを確定 → **XML レベルで `&HUDMainMenu;` を参照しなければ HUD は生成されない**（エンティティ機構の確定）。 |
| 「メニュースタック実測」として言えること | **[部分確定]** | ①**窓 id は `vtable[+0x34]` 実行時取得**（`0x591B70` 内 `mov edx,[ecx+0x34]; call edx` → `eax-0x3E9` → `0x42BB90([0xB13970], …)`＝**id→生成済み配列への登録**）＝**「どのメニューが生成済みか」は `[0xB13970]` 体系で管理**。②close ゲート（v542/v550: `591B40` が 0x3EC/0x3EF を特別扱い＝HUD/Loading は処理スキップ）と v544 の close ゲート1004/1007 と整合。③**ただし「chargen 中に HUD open が呼ばれない」ことを示す分岐そのものは、open 呼び出し元（281件の vtable+0x14 呼び出し）の絞り込みが未完** → 残点 V-574-1。 |

---

## 2. 確定した命令構造 [確定(命令)]

```
TileMenu::open 0x591C50（vtable[+0x14]・E8=0）
  └ 0x591C76: call 0x587D20(id)          ; 唯一の呼び出し
       0x587D20: eax = id - 0x3E9(1001); cmp eax,0x32; ja → 0x588794
                 jmp dword [eax*4 + 0x5887b8]      ; 1001..1051 の switch
                 case 1004 → 0x587E95 → HUDMainMenu ctor 0x5A6780（唯一 caller）
                 （冒頭: push 0x64; call 0x401F00＝malloc）

窓 id→生成ヘルパ の別 switch（head 0x587550 内・cmp 0x58767F）:
  0x3EC(HUDMainMenu)      → 0x5A6B00
  0x3ED(HUDInfoMenu)      → 0x5A4840
  0x3F2(HUDSubtitleMenu)  → 0x5A8E30
  0x3EF(LoadingMenu)      → 0x5ADCF0
  0x414(MainMenu)         → 0x5B6040
  （後続にも id 判定が続く列挙）

TileMenu の破棄系（教訓: find_head の int3 帯誤帰属を検証して確定）:
  0x5900F0 = push esi; mov esi,ecx; call 0x591B70; test [esp+8],1; je; free(esi); ret 4  ; dtor 全体
  0x591B70 = [esi]=vtable 0xA6AF14 → [esi+0x44] の menu の vtable[+0x34]（窓 id 取得）
             → 0x42BB90([0xB13970], id-0x3E9, &slot) → 0x584880(menu,0) → vtable[0](arg=1)
             → [esi+4]==0 なら 0x58DA70 → 0x5819A0(this)     ; vtable 恢復＋メンバ menu 除去
  E8 callers of 0x5900F0 = 0（vtable 経由のみ）
```

* **`push 0x3EC` 25件の内訳 [確定(命令)]**: 24件が `0x589B70`（id→オブジェクト取得＝HUD の既存インスタンス操作）、1件が XML エンティティ登録。**open 直呼び0件。**
* **`call [reg+0x14]` 281件は、直前40B に menu id（1001..1051）push / `0x3EC` / menu 系文字列を含むものが0件** [確定(命令)] → open の引数体系は id 直指定ではない。

---

## 3. 残点（引き継ぎ）

| ID | 内容 | 重要度 |
|---|---|---|
| **V-574-1** | **「chargen 中に HUD open が呼ばれない」の最終裏付け**: `call [reg+0x14]` 281件のうち HUD の表示制御を担う呼び出しの特定（`0xA6AF14`/`0xA6BEE4` vtable 系オブジェクトの経路）と、その呼び出し元の **chargen/前置メニュー状態ゲート**（メニュースタック＝`[0xB13970]` 体系か、chargen フラグ） | **中**（V-562-3 を再定義して継承） |
| V-574-2 | `0x587550` の id→生成ヘルパ switch の全列挙（0x3EC/0x3ED/0x3F2/0x3EF/0x414 以降の続き）と、これが `0x587D20` switch とどう役割分担するか | 低 |
| V-574-3 | `[0xB13970]`／`0x42BB90`（id→生成済み配列）の実体（＝真の「メニュースタック」データ構造） | 低 |

---

## 4. 教訓（ハーネスの罠・再発型）

1. **`find_head` は int3 帯（`cc` 連続）で前関数に誤帰属する**: `call 0x587D20` の hit `0x591C76` の head が「0x591B70」と出たが、`0x591B70` の ret（0x591C18）より後＝**別関数（open `0x591C50` の内部）**。→ **E8/call 系の hit は「ret 位置と hit 位置の整合」を size 付き逐語で必ず検証する**（本裁定では v574_4 の decode と v574_5 の精査で是正）。
2. **E8 走査は hit アドレスを必ず出す**（head だけ出すと誤帰属を見逃す）。
3. 「open を探す」は `push <id>; call open` では見つからない（**vtable 経由・id は直 push されない**）→ 引数体系を先に確定させてから走査を絞る。

---

## 5. probe 一覧

`v574_1`（HUDMainMenu ctor `0x5A6780` の E8 callers＝1本 `0x587E95`／`push 0x3EC` 25件の文脈／`cmp 0x3EC` 3件）／`v574_2`（`0x587550` の id switch 実読／`0x587D20` switch 冒頭／`0x58A1C0`＝XML エンティティ定義群 `&generic;` 等／`&HUDMainMenu;` 文字列同定）／`v574_3`（`0x587D20` の E8＝1本／`call [reg+0x14]` 281件に id push 0件）／`v574_4`（`0x591B70` 全 decode＝dtor 中間と確定／`0x5900F0` の call site）／`v574_5`（**E8 hit アドレス精査＝`call 0x587D20` は `0x591C76`＝open 内・`0x5900F0` 先頭30命令で dtor 確定**）。全 5 本 EXIT=0。
