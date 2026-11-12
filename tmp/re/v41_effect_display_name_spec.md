# V41: MGEF 効果表示名生成の実データ仕様（移植向け）

- 日付: 2026-10-24 / 手法: capstone 5.0.7 + PE構造自前解析 + decompiled.c 突合
- 関連: `v38_413a90_branches.md`（表示器=関数本体）, `v47_string_type_spec.md`（AV名0..39の格納型）, `v35_mgef_code_table.csv`
- **新 ground truth**: `.data` の **SizeOfRawData = 0x30C00**（VirtualSize 0xA9C1C）→ **ファイルで読めるのは VA 0xB02000〜0xB32C00 のみ**、それ以降は bss（実行時ゼロ＋初期化コードで充填）。**0xB32C00 超の .data 値をファイルから読むのは誤り**（本調査で実測: 0xB334C0 / 0xB3A034 は bss）。

## 0. セルフテスト（4/4 PASS）

| 検査 | 実測 | 結果 |
|---|---|---|
| `bytes@0x504050`（8B） | `518b4c24188b5424` | PASS |
| `bytes@0x4FB580` | `578b7908` | PASS |
| `string@0xA7035C` | `.\AI\CombatController.cpp` | PASS |
| opid `0x0015` impl | `0x504560` | PASS |

## 1. 接頭辞オブジェクト（5個・bss・stride 8）

初期化スタブ `0x9D9EE0..0x9D9F7F`（CRT init 群）の実逆アセンブル（各 0x20 バイト、末尾で破棄登録）:

| スタブ | グローバル | +0（接頭辞） | +4（登録キー） | 破棄thunk |
|---|---|---|---|---|
| `0x9D9EE0` | `0xB334C0` | `0xA3276C` = **`Fortify`** | `0xA32754` = `sMagicEffectItemFortify` | `0xA17510` |
| `0x9D9F00` | `0xB334C8` | `0xA3278C` = **`Restore`** | `0xA32774` = `sMagicEffectItemRestore` | `0xA17520` |
| `0x9D9F20` | `0xB334D0` | `0xA327AC` = **`Drain`** | `0xA32794` = `sMagicEffectItemDrain` | `0xA17530` |
| `0x9D9F40` | `0xB334D8` | `0xA327CC` = **`Damage`** | `0xA327B4` = `sMagicEffectItemDamage` | `0xA17540` |
| `0x9D9F60` | `0xB334E0` | `0xA327EC` = **`Absorb`** | `0xA327D4` = `sMagicEffectItemAbsorb` | `0xA17550` |

各スタブの形: `push <接頭辞>; push <キー>; mov ecx,<グローバル>; call 0x419A70; push <thunk>; call 0x981FB4; pop ecx; ret`
（`0x981FB4` は破棄登録系。`0xB334C0..E0` は bss = 静的初期値は無し＝スタブ実行時のみ確定）

**セッタ `0x419A70`（thiscall, `ret 8`）の挙動**（先頭40命令を実デコード）:
1. `mov [esi+0], 2nd arg`（= 接頭辞文字列）, `mov [esi+4], 1st arg`（= `sMagicEffectItem*` キー）
2. キー非NULL なら `mov ecx, 0xB35574; call 0x55E000(key, &obj)` → **中央レジストリ（マップ型グローバル 0xB35574）へ登録**
3. 既存衝突時（`al!=0`）: `push key; push 0xA32490; call 0x4A7A60`（重複警告/ログ系）
4. 戻り値 = this（`mov eax, esi`）

→ 5接頭辞は**起動時に 0xB35574 のマップへ登録される**（ESM 側 `sMagicEffectItem*` ローカライズキーと対応）。

## 2. 表示器 `0x413A90` の分岐→接頭辞 対応（V38 実測の再掲）

| 分岐先 | fourcc 比較 | 使用グローバル | 接頭辞 |
|---|---|---|---|
| `0x413AFA` | REAT | `0xB334C8` | Restore |
| `0x413ABE` | FOSK, FOAT | `0xB334C0` | Fortify |
| `0x413AEF` | ABSK, ABAT | `0xB334E0` | Absorb |
| `0x413B72` | DRSK, DRAT | `0xB334D0` | Drain |
| `0x413B7A` | DGAT | `0xB334D8` | Damage |
| `0x413B39` | 上記以外137種（default） | （不使用） | **ESM base name のみ表示** |

書式: `0x413A90` 内 `call 0x98208B`(_sprintf) / 書式 `0xA32564` = **`%s %s`** / 2つ目の `%s` は **`FUN_00565CC0(actorValue)`** の返す属性・スキル名。
呼び出し元4件（全てUI）: `FUN_00413be0`(tooltip/name) / `FUN_0041cbb0`(magic menu ACTIVE EFFECTS) / `FUN_00593cd0`(alchemy) / `FUN_005b2b70`(enchanting)。

## 3. 属性・スキル名の取得 `FUN_00565CC0` @0x565CC0（2テーブル構成）

実逆アセンブル全文:
```
mov eax, [esp+4]              ; actorValue index
cmp eax, 0x27                 ; idx <= 39
ja  0x565cd7
  mov eax, [eax*4 + 0xB12758] ; 表A: ポインタ P（String オブジェクト）
  test eax,eax / je → 0
  mov eax, [eax]              ; String.buf を間接参照して返す
  ret
0x565cd7: cmp eax, 0x48       ; idx < 72
  jge → 0
  mov eax, [eax*4 + 0xB12868] ; 表B: .rdata の char* をそのまま返す
  ret
0x565ce4: xor eax,eax / ret   ; 範囲外・NULL → 0
```
別関数 `FUN_00565CF0` @0x565CF0（`idx <= 0x20`）: `mov eax,[eax*4+0xB127F8] → test → mov eax,[eax]` ＝**表C（bss String 間接）**。

| 表 | 基底 | 有効 idx | 方式 | 静的取得 |
|---|---|---|---|---|
| A | `0xB12758`（.data・ファイル内 ✓） | 0..39 | ポインタ→**bss の String オブジェクト** `0xB3A034+`（stride 8） | ✗（buf は実行時充填） |
| B | `0xB12868`（.data・ファイル内 ✓） | 40..71 | **.rdata の char\* 直返** | ✓ **32件確定** |
| C | `0xB127F8` | 0..32 | ポインタ→bss String `0xB3A174+`（stride 8） | ✗ |

## 4. 表B の32件（実読値・idx 40..71）

```
40 Magicka Multiplier   41 Night Eye Bonus    42 Attack Bonus      43 Defend Bonus
44 Casting Penalty      45 Blindness          46 Chameleon         47 Invisibility
48 Paralysis            49 Silence            50 Confusion         51 Detect Item Range
52 Spell Absorb Chance  53 Spell Reflect Chance 54 Swim Speed Multiplier 55 Water Breathing
56 Water Walking        57 Stunted Magicka    58 Detect Life Range 59 Reflect Damage
60 Telekinesis          61 Resist Fire        62 Resist Frost      63 Resist Disease
64 Resist Magic         65 Resist Normal Weapons 66 Resist Paralysis 67 Resist Poison
68 Resist Shock         69 Vampirism          70 Darkness          71 Resist Water Damage
```
（外部知識の ActorValue 40..71 一覧と**一致** — 表B の実読が正）

## 5. 表A（idx 0..39 = 属性8＋スキル20＋能力値系）について

- 表A エントリ（`0xB12758+idx*4`, ファイル内=読み取り可）= `0xB3A034, 0xB3A03C, …`（**stride 8**）
- その先の String オブジェクトは **bss（0xB32C00 超）= 静的中身なし**（読取実測: ファイル上の該当領域は raw 範囲外の別セクションデータ）
- → **AV 0..39 の名前は実行時にセットされる String.buf 経由**（V47 の `String{buf@+0,len@+4,cap@+6}` と**同一レイアウト・同一型**）
- **未解決(1)**: 0..39 の String を塗る初期化元（decompiled.c に `b3a034` の直参照は0件 → 計算済みベースの可能性）。ESM/ローカライズ文字列由来と推定されるが**未証明**

## 6. 移植仕様（まとめ）

1. **接頭辞5個**を `Fortify/Restore/Drain/Damage/Absorb` の固定表で実装（`{prefix, localizeKey}` の5×8バイト、キーは `sMagicEffectItem*`）
2. **分岐表**（§2）で fourcc→接頭辞を選ぶ（default は接頭辞なし＝ESM base name 表示）
3. **AV 名**: idx 40..71 は §4 の32文字列を静的データで移植可。**idx 0..39 は実装側で属性・スキル名を返す必要あり**（Strength/Intelligence/…Blade…Speechcraft 等の73種内、当初40）— 現実装のバッファ寿命（V47 参照: Set("") で free）に注意
4. 表示: `sprintf("%s %s", prefix, avName)`、prefix 無し時は avName のみ
5. UI 呼出4箇所に接続（V38 §呼び出し元）

## 7. 未解決点

- AV 0..39 の初期化元（bss String 充填ルーチン） — 追跡候補
- `FUN_00565CF0`（表C, idx0..32）の用途別呼び出し元分類（L316714/L339551 等で使用、別 getter の可能性）
- 中央レジストリ `0xB35574` の型（マップ構造）と全登録数（5接頭辞以外の登録者がいるか）


## 8. 訂正と補完（V48, 2026-10-24）

**訂正（§3・§5 の「bss String オブジェクト」表記）**: 表A/表C の bss エントリは **V47 の `String{buf,len,cap}` ではなく、`{char* value@+0, char* key@+4}` のレジストリ登録オブジェクト**。`FUN_00419A70` はローカライズ解決器ではなく **key→entry の登録関数**（中央レジストリ `0xB35574` へ登録、衝突時 `0x4A7A60` 警告）で、getter は **value = fallback 英名**を返す。prefix 5オブジェクト（§1）も同型（+0=prefix, +4=`sMagicEffectItem*` key）。

**初期化元（§5 未解決1 を解消）**: `.text` 走査で `imm32=0xB3A034+8i` が **40/40 全件2ヒット**。初期化 thunk = **`0x9F9660+0x20i`（40個）**: `push fallback / push key / mov ecx,entry / call 0x419A70 / atexit dtor登録`。登録元は `.rdata 0xA2AA78` の CRT 初期化テーブル（111+エントリ）。

**73種の充足**: **72/73 確定**（idx 0..71 全件。根拠3系統: 表A thunk fallback / 表B .rdata char\* / script-VM 名テーブル `0xB0A1A8`）。**idx≥72 は EXE に定義なし**（getter 上限 `0x48`、範囲外は NULL 返却）。

**表A vs 表C の関係（§3 補足）**: **別用途**。表A = AV 表示名（key `sAttributeName*`/`sSkillName*`/`sTraitName*`/`sVirtueName*`）、**表C = AV 説明文**（`*Desc*` 33件, idx≤0x20）＋アイコンパス表（`sAttributeIcon*`、fallback は dds パス）。key の `Name`→`Desc` 置換で 33/33 対応。表C 初期化 thunk = `0x9F9B60+0x20i`。

**移植への影響**: AV 名・Desc・Icon は**静的 fallback 英名を実データとして移植可能**（ローカライズ key も併記）。表A/B 合計72件は `tmp/re/v48_av_names_initializer.md` の一覧を使用。
