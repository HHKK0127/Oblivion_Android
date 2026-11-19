# v575 — V-574-1 の絞込み②（[部分確定]）: **HUD/RaceSex の open slot＝共通 no-op** と `TileMenu::open` 実体の確定（v574 訂正を含む）（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v575_1〜v575_6**・6本 全 EXIT=0）。
先行: `tmp/re/v574_hud_open_path_ruling.md`（open 総入口の記述＝本裁定で訂正）／`v550`（TileMenu vtable）／`v544`（4002 メッセージタイプ）
判定: `[確定(命令)]` / `[確定負（v574 訂正）]` / `[部分確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| HUDMainMenu は `TileMenu::open 0x591C50` 経由で開くのか | **[確定負（v574 訂正）]** | **HUDMainMenu vtable `0xA6BEE4` の slot5(+0x14)＝`0x60CF60`＝`ret 8` の no-op stub!!**（TileMenu vtable `0xA6AF14` slot5＝`0x591C50` と**別**）。→ **HUDMainMenu は「0x591C50 型の class 生成 open」を持たない（オーバーライドで無効化）**。**RaceSexMenu vtable[4]/[5] も同じ no-op**（v573 の「汎用ディスパッチ候補」とは別物＝**空関数**）→ v574 §1「open 総入口＝TileMenu::open 経由」という記述は、**TileMenu vtable 系オブジェクトに限られる**と訂正。 |
| `TileMenu::open 0x591C50` の実体 | **[確定(命令)]** | `(a1 == 0xFA2(4002) でなければ即 0 返却)` → `f2i(a2: float)` → `0x587B20(1, id)` → **`0x587D20(ctx, id)`＝生成 switch** → `[this+0x44] = menu`、NULL なら `'ERROR: Cant find menu class!'` 出力。**`0xFA2` は XML エンティティ `&class;` の値**（`0x58A9A6: push 0xfa2; push 'class'; call 0x58A0A0`＝entity 定義群の一員）。 |
| open の実呼び出し元はどこか | **[部分確定]（絞込み）** | **`push 0xFA2`＝XML 登録1件のみ／`mov reg,0xFA2`＝0 件／`E8 0x591C50`＝0 件**（vtable 経由のみ）→ **`call [reg+0x14]`（短7形式）281件は head が全て 0x88-0x94 領域（UI 0x57-0x5C に0件）**、SIB 形式 `ff 14 xx` 19件は switch ジャンプ等で open なし。→ **呼び出し側は a1=0xFA2 を渡す形式が未特定**。ただし **open の入力が「XML エンティティ `&class;`(4002) メッセージ」であることが確定した** → **次の入口＝`0x58A0A0` 系 entity 実行（XML→メッセージ送信）**。 |
| HUD の生成経路（現時点の全体像） | **[確定(命令)]＋[部分確定]** | `push 0x3EC` 25件＝**取得のみ（open 直呼び0件）**（v574 済）。生成は **`0x587D20` の switch case 1004 → ctor `0x5A6780`（唯一 caller）**。その `0x587D20` を呼ぶのは **`0x591C50` のみ** → **HUD は「0x591C50 を持つ TileMenu レイヤー経由」または「`&class;`(4002) メッセージ経由」で生成される**。**chargen ゲートは open 呼び出し元側**（V-574-1 継続・入口を entity 実行系に特定）。 |

---

## 2. vtable 比較（本裁定の主証拠）[確定(命令)]

| slot | TileMenu `0xA6AF14` | HUDMainMenu `0xA6BEE4` | RaceSexMenu `0xA6D38C` |
|---|---|---|---|
| 0 (+0x00) | `0x5900F0`（dtor） | `0x5A6AE0` | `0x5C4E20` |
| 3 (+0x0C) | `0x5900D0` | `0x5A68C0` | **`0x5CC200`（notify）** |
| **5 (+0x14)** | **`0x591C50`（open＝生成）** | **`0x60CF60`（no-op `ret 8`）** | **`0x60CF60`（no-op）** |
| 6 (+0x18) | `0x591B40`（HUD/Loading ゲート） | `0x890B90` | `0x890B90` |

* `0x60CF60` の E8 callers は 6 関数（`0x4B2C00`/`0x53B0E0`/`0x53B730`/`0x53C100`/`0x544420`/`0x544EA0`）＝**no-op 呼び出しで何の役割もない**（別システムの void 呼び）。
* → **v573 の「[4]=[5]=[9]=0x60CF60 汎用ディスパッチ候補」は [確定負] 訂正＝空関数**（ただし v573 の残点 V-573-1（id98 送信側＝message 配線）は本件と別系統のため継続）。

---

## 3. 残点（引き継ぎ・再定義）

| ID | 内容 | 重要度 |
|---|---|---|
| **V-575-1** | **open 実呼び出し元の特定（入口＝entity 実行系）**: `0x58A0A0` 系 XML エンティティ（`&class;`＝4002）の**実行コード**＝「メッセージ4002 を送り `0x591C50`（or vtable slot）を叩く場所」。`push 0xfa2` が見つからない以上、a1 は**変数/配列経由**（entity 値テーブル `0x58A0A0` 登録先からの読み）と推定 → 登録済み entity 値の**読み手**を走査。 | **中**（V-574-1 を再定義して継承） |
| V-575-2 | HUDMainMenu/RaceSexMenu が slot5 を no-op 化した理由（＝「class 生成 open を使わないメニュー」の共通クラス設計）と、それでも両者が生成される実経路（`&class;` メッセージか、XML `<menu>` タグか） | 中 |
| V-575-3 | `0x587B20(1, id)` の役割（生成前処理） | 低 |

---

## 4. 教訓（再発型）

1. **vtable の slot 値が「共通 no-op」であることを読まないと、open 経路を誤同定する**（v574 の「open 総入口＝TileMenu::open 経由」は **HUDMainMenu/RaceSexMenu には該当しない**）。→ **vtable 比較（複数クラスの同一 slot を並べる）が最短の訂正手段**。
2. `push imm32` / `mov reg,imm32` / `E8` の3系統を走査して**全0に近い結果なら「引数は変数経由」**と断定できる（v573 の negative 手法を継続適用）。

---

## 5. probe 一覧

`v575_1`（`push 0x3EC` 25件の前方40命令＝open 呼び出し0件）／`v575_2`（`call [reg+0x14]` 281件の head 分布＝UI 0件・`TileMenu::open 0x591C50` 全 decode）／`v575_3`（`push 0xFA2` 全走査＝1件のみ＝XML entity 登録）／`v575_4`（`mov reg,0xFA2` 0件・`E8 0x591C50` 0件・entity 文字列＝`'class'`）／`v575_5`（**HUDMainMenu/TileMenu vtable 16 slot 比較**・`ff 15` 7035/`ff 14` 19件＝open なし）／`v575_6`（**`0x60CF60`＝`ret 8` no-op 確定**・E8 6関数）。全 6 本 EXIT=0。
