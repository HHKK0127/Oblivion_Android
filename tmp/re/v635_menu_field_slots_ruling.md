# v635 — RaceSexMenu `+0x28` フィールドスロットの正体（chargen pane の親）

- **日付**: 2026-11-22
- **帯域**: RaceSexMenu（menu id `0x40C`）/ XML interpreter / RTTI
- **判定語彙**: `[確定(命令)]` / `[確定(命令)(negative)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 本裁定の EXE 実読は capstone probe（`D:\re\v307_common.py`、`Oblivion.exe` Steam版）と `docs/Oblivion_decompiled.c` の逆コンパイルによる。XML は `D:\re\race_sex_menu.xml`（22156B, raw md5 `d367341334fa89101ba4fc077edb52fe`）。引用は tool が表示した文字列のみ。範囲は半開区間。
- **起点**: v634（row611）の続き。ユーザー指示の順序 **1 → 2 → 3 → 4 → 5** で `+0x28` の正体を確定する。

## §0 総括表

| # | 命題 | 判定 | 主要証跡 |
|---|---|---|---|
| 1 | builder は `+0x28` に window を格納しない（読むだけ） | `[確定(命令)]` | `FUN_005ca010` L333100 / `u133_1` 直 store 0 件 |
| 2 | guard は window 生成 skip でなく **build 全体の中止** | `[確定(命令)]` | `0x5CA133..0x5CA15D` / `jmp 0x5cc1db` |
| 3 | `RaceSexMenu::SetField` の idx→offset | `[確定(命令)]` | `0x5C26D0` / vtable slot1 `0xA6D390` |
| 4 | `SetField` の idx はタイルの XML `<id>` | `[部分確定]` | idx 集合 {1,2,10,20,21,90} = XML `<id>` 集合 |
| 5 | `FUN_005c0440` の `this` は **PersuasionMenu** | `[確定(データ)]` | RTTI locator `0xAC292C`→`.?AVPersuasionMenu@@` |
| 6 | trait 名表: `0xFA8=id` / `0xBBA=name` | `[確定(データ)]` | 文字列 `"id"`@`0xA6A49C` / `"name"`@`0xA6A5D0` |
| 7 | **`+0x28` は 0/1/2 状態機械ではなく `Tile*`（= race_window）** | `[部分確定]` | §1・§3・§4 の合成 |

---

## §1 — builder は `+0x28` を読むだけ（store しない）`[確定(命令)]`

`FUN_005ca010`（builder, head=`0x5CA010`, `docs/Oblivion_decompiled.c` L333028〜）:

```c
fVar2 = (float)FUN_00589b70();
if (fVar2 == 0.0) {
  ...
  puVar3 = (undefined4 *)FUN_005898f0();          // menu 実体
  ...
  if ((((puVar3[10] != 0) && (puVar3[0xb] != 0)) && (puVar3[0xc] != 0)) && (puVar3[0xf] != 0)) {
      ...
      uVar6 = FUN_005c4340();                      // arg1 = [esi+0x28]（=puVar3[10]）
      puVar3[0x10] = uVar6;                        // +0x40 = Main pane
      ...
  }
  FUN_004a7a60("... menu and art resources up to date?");
  return 0;
}
```

- `puVar3[10]` = `+0x28`、`[0xb]`=`+0x2c`、`[0xc]`=`+0x30`、`[0xf]`=`+0x3c`。builder はこの 4 つを**前提条件として読むだけ**。
- `u133_1` 実測: `0x5CA010..0x5CBC20` に `mov dword ptr [esi+0x28/0x2c/0x30], ...` は **0 件**。
- 最初の `FUN_005c4340` 呼び出し（`0x5CA448`）の arg1 = `[esi+0x28]`（v626/v633 の実測 `8B 46 28 50`）。
- → **builder は window を先に格納しない**。ユーザーの「builder が window を `+0x28` に入れる」仮説は**否定**。`+0x28` は builder より前に設定済みでなければならない。

## §2 — guard の skip 範囲 = build 全体の中止 `[確定(命令)]`

```
0x5CA133 cmp dword ptr [esi + 0x28], 0 ; je 0x5CA14B
0x5CA139 cmp dword ptr [esi + 0x2c], 0 ; je 0x5CA14B
0x5CA13F cmp dword ptr [esi + 0x30], 0 ; je 0x5CA14B
0x5CA145 cmp dword ptr [esi + 0x3c], 0 ; jne 0x5CA15D
0x5CA14B push 0xa6d3f8 ; call 0x4a7a60(警告) ; jmp 0x5cc1db   ← 関数末尾
0x5CA15D ... build 本体（全 21 pane + 90 slider + ...）
```

- 4 フィールド（`+0x28/+0x2c/+0x30/+0x3c`）のいずれかが 0 → 警告を出して `0x5CC1DB` へ飛ぶ = **build 中止**。
- 「既に window があるなら window 生成だけ skip」という構図では**ない**（skip 先は関数末尾）。

## §3 — `RaceSexMenu::SetField` の idx→offset `[確定(命令)]`

- `0x5C26D0` = **RaceSexMenu vtable slot 1**（vtable `0xA6D38C`、slot1 @ `0xA6D390`）。`callers_of(0x5C26D0) = 0`（直接 caller なし = 仮想呼び出しのみ）。

```
0x5C26D0 mov eax,[esp+4]            ; idx
0x5C26D4 cmp eax,1   ; jne ... ; mov [ecx+0x28],edx ; ret 8
0x5C26E3 cmp eax,2   ; jne ... ; mov [ecx+0x2c],edx ; ret 8
0x5C26F2 cmp eax,0x14; jne ... ; mov [ecx+0x34],edx ; ret 8
0x5C2701 cmp eax,0x15; jne ... ; mov [ecx+0x38],edx ; ret 8
0x5C2710 cmp eax,0xa ; jne ... ; mov [ecx+0x30],edx ; ret 8
0x5C271F cmp eax,0x5a; jne ... ; mov [ecx+0x3c],edx ; ret 8
```

| idx | offset | idx | offset |
|---|---|---|---|
| `1` | `+0x28` | `0x14`(20) | `+0x34` |
| `2` | `+0x2c` | `0x15`(21) | `+0x38` |
| `0xA`(10) | `+0x30` | `0x5A`(90) | `+0x3c` |

- オフセットは**即値**（`lea` 変位ではない）。上限チェック無し。`ret 8`（stdcall 2 引数）。
- 対照: `QuantityMenu::SetField`（`0x5C0530`, vtable `0xA6D15C` slot1）= idx {1,2,3,4,6,7} → {+0x28,+0x2c,+0x30,+0x34,+0x38,+0x3c}。**idx 集合はクラスごとに異なる**。
- 第2引数は `Tile*`（XOBSE `Menu::SetField(UInt32 idx, Tile* value)`。builder が `+0x28` を Tile として `GetFloat(+0xFD0=childcount)` に渡すことと整合）。
- `FUN_005c4e40`（RaceSexMenu ctor, `*param_1=&PTR_FUN_00a6d38c`）は `param_1[10]=0 / [0xb]=0 / [0xc]=0 / [0xf]=0` で **`+0x28/+0x2c/+0x30/+0x3c` のみ 0 クリア**（`+0x34/+0x38` はクリアしない）。→ guard の 4 フィールドと一致。

## §4 — `SetField` の idx はタイルの XML `<id>` `[部分確定]`

- `race_sex_menu.xml` の `<id>` 値（`u` 実測 grep, 全 14 箇所）: `&generic;`×4, `1, 2, 10, 20, 90, 100, 101, 102, 103, 104`。
- うち `SetField` が扱う idx と一致するもの:

| idx | offset | XML 要素（`<id>` 行） |
|---|---|---|
| 1 | +0x28 | `<rect name="race_window">`（L148-149, "PAGES LAYOUT" コンテナ） |
| 2 | +0x2c | `<rect name="race_player_grab">`（L36-37） |
| 10 | +0x30 | `<text name="race_name">`（L48-49） |
| 20 | +0x34 | `<image name="race_scroll_bar">`（L169-171） |
| 90 | +0x3c | `<image name="race_button_done">`（L211-213） |
| 21 | +0x38 | 本 XML には無し（`<include>` 経由の可能性・未確認） |

- 呼び出し規約は `menu->SetField(tile-><id>, tile)` と推定。**推測ではなく集合一致＋意味整合**による `[部分確定]`。
- **`[未確定]`**: `SetField`（vtable slot1）の呼び出しサイトそのもの。全 `.text` で `call dword ptr [reg+4]` は 0x890000〜0x950000 帯に 196 件のみで **menu 帯（0x580000〜0x5D0000）に 0 件**。仮想呼び出しは `mov edx,[this]; mov eax,[edx+4]; mov ecx,this; call eax` 形で存在するが、`SetField` を指すサイトは未特定。→ 残点 **V-635-1**。

## §5 — `FUN_005c0440` の `this` は PersuasionMenu `[確定(データ)]`

- `0x5C0440` を dword で含むのは `0xA6D0D8` の 1 箇所のみ。
- vtable 先頭 = `0xA6D0CC`、その `[-1]`（`0xA6D0C8`）= RTTI locator `0x00AC292C`。
- locator `0xAC292C` の `+0xC` = type descriptor `0xB13384`。その `+8` の文字列 = **`.?AVPersuasionMenu@@`**（`0xB1338C`）。
- → `FUN_005c0440` は **PersuasionMenu の virtual**（slot3）。したがって `[this+0x28]==2` は **PersuasionMenu の別フィールド**であり、chargen RaceSexMenu の `+0x28` とは**無関係**。
- **訂正**: v634 までの「`FUN_005c0440` は chargen のゲート」という帰属は誤り。`[this+0x28]==2` を chargen `+0x28` の状態機械仮説の根拠に使えない。
- 参考（RTTI で確定した vtable）: `RaceSexMenu`=`0xA6D38C` / `QuantityMenu`=`0xA6D15C` / `PersuasionMenu`=`0xA6D0CC` / `Menu`=`0xAC15xx`（locator `0xAC1538`）。

## §6 — trait 名表の追記 `[確定(データ)]`

`FUN_0058a0a0(name, id)` 登録（`docs/Oblivion_decompiled.c` L295420-295568）:

| trait id | 名前 | 文字列アドレス |
|---|---|---|
| `0xFA8` | `id` | `0xA6A49C` |
| `0xBBA` | `name` | `0xA6A5D0` |
| `0xFA7` | `alpha` | — |
| `0xFA5` | `stackingtype` | — |
| `0xFAA` | `listindex` | — |
| `0xFD0` | `childcount` / `child_count` | — |
| `0xFB5` | `user7` | — |

- menu id: `&RaceSexMenu;`=`0x40C`（L295436）/ `&PersuasionMenu;`=`0x40A` / `&QuantityMenu;`=`0x3F8`。
- `RaceSexMenu` のオブジェクトサイズ = **0x9B0**（`0x588457 push 0x9b0; call 0x401f00(malloc); mov ecx,eax; call 0x5c4e40`）。

## §7 — 判定: `+0x28` は `Tile*`（race_window）`[部分確定]`

- `+0x28` は `RaceSexMenu::SetField(idx=1)` が書く `Tile*`。`[確定(命令)]`
- idx=1 は XML `<id>1</id>` = `race_window`（"PAGES LAYOUT" コンテナ）。`[部分確定]`（§4）
- builder の全 21 pane の親 = `[menu+0x28]` = race_window。`[確定(命令)]`
- → **`+0x28` は 0/1/2 の状態機械ではない**。ユーザーの #8 判定分岐のうち「1 が出れば 0/1/2 状態機械」は成立せず、**「Init の 0 だけ」でもない**（XML 由来の Tile* が入る）。
- ポート: pane の親 = race_window（`<id>1</id>` の `<rect>`）として扱う。`+0x28/+0x2c/+0x30/+0x3c` はそれぞれ race_window / race_player_grab / race_name / race_button_done のタイル参照。

## §8 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-635-1 | `Menu::SetField`（vtable slot1）の呼び出しサイト同定。仮想呼び出し idiom は存在するが menu 帯に特定サイトなし | `[未確定]` |
| V-635-2 | idx=21 → `+0x38` の対象タイル（本 XML に `<id>21</id>` なし。`<include>` 先の可能性） | `[未確定]` |
| V-635-3 | `SetField` 第2引数が常に `Tile*` か（`+0x28` を int として読む経路が chargen に無いことは §1・§3 で確認済みだが、他経路の網羅は未完） | `[部分確定]` |
