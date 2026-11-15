# v391 — fn `0x474530` の `byte [esp+0x38]`（blend/crossfade 選択 byte）の元の裁定（#153 ②・V-383-1）

- 採番: **v391**（ai-todo #153 束の第2弾、probe prefix v391_1..v391_3）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v383 の残点 **V-383-1（fn `0x474530` の `byte [esp+0x38]` の元＝blend/crossfade 選択の真の入力）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| fn `0x474530` の identity | **[確定(命令＋文字列)]** | **「Morph」検証＋ディスパッチ fn** — decompiled L92858 `// FUN_00474530 @ 0x474530`／L92862 `int __thiscall FUN_00474530(int param_1,int param_2,float param_3,int param_4)`。body が **2種の Morph エラー trace を出力**: L92971/L474812 `"Morph Error: Trying to morph from sequence to itself.\r\n'%s' on '%s'."`（`0xA3CA28`）／L92977/47493A `"Morph Error: Controller count not the same.\r\n'%s' has %d controllers and\r\n'%s' has %d on '%s'."`（`0xA3C9C8`） |
| `byte [esp+0x38]` の元 | **[確定(命令)]** | **「Morph 検証の合否」フラグ** — writers＝**3件のみ**: ①`0x4745BA: =0`（初期化・slot 取得失敗時はこのまま）②**`0x47480B: =1`（検証成功）** ③`0x47482A: =0`（自己 morph エラー時のリセット）。**成功条件（=1）**: (a) `0x51AAD0` getter の2回呼び出し結果が一致（`cmp bl,al`）(b) **`[edi+0xC] == [ebp+0xC]`（controller count 一致）** (c) `ebp ≠ edi`（非自己）。**失敗時は 0 のまま**: getter 初回 fail（`je 0x47482F`）／getter 結果差（`jne 0x47482F`）／**count 不一致（`jne 0x474926` → trace → `jmp 0x47482F`）**／self-morph（trace → `=0`） |
| 分岐の意味（v383 の記述の確定） | **[確定(命令)]** | **flag≠0（Morph 検証成功）→ `0x474953` → `0x6C4060`（blend wrapper → `0x6C9E00` → state4/state5 ブレンド対）**／**flag==0（失敗/対象外）→ `0x474977` → `0x6C4000`（crossfade wrapper → state3/state2・A active/B inactive ゲート付き）** → **「controller count が一致する同期 blend（Morph）／不一致なら plain crossfade へのフォールバック＋エラーログ」** |
| `[seq+0xC]` の意味 | **[確定(文字列)]** | **controller count** — count 不一致 trace の `%d` 引数が `[edi+0xC]`/`[ebp+0xC]`（`0x4747FD`/`0x474800` の cmp） |

**V-383-1 への回答**: `byte [esp+0x38]` は **Morph 検証の合否**（init 0 → 成功時1 → self エラーで0リセット）。v383 が「blend/crossfade 選択 byte」としたのは正しく、**その入力＝controller count 一致・getter 一致・非自己の3条件**と確定。

---

## §2 証拠

### §2.1 fn identity（v391_1(c)/v391_3(b)）

- decompiled: **L92858 `// FUN_00474530 @ 0x474530`**／L92862 シグネチャ。
- **2種の Morph trace（本 fn 内）**: L92971（self・`0xA3CA28`）／L92977（count 不一致・`0xA3C9C8`）。他の "Morph" ヒット（L249000〜L259168）は **FaceGen/HairMorph＝別系統**（無関係）。

### §2.2 flag writers 全3件（v391_1(b) census）

```
① 0x4745BA: mov byte [esp+0x38], 0        ; 初期化（edi==null でも0のまま）
② 0x47480B: mov byte [esp+0x38], 1        ; ★検証成功
③ 0x47482A: mov byte [esp+0x38], 0        ; self-morph trace の後のリセット
```

### §2.3 検証ブロック（v391_2(a) unfiltered 0x4747D9..0x47482F）

```
4747D9: mov byte [esp+0x28], 1（別 byte flag・残点）
4747DE: call 0x51AAD0; test al,al; je 0x47482F     ; check1 fail → 0 のまま
4747E7: mov ecx,ebx; call 0x51AAD0 → bl
4747EE: mov ecx,[esp+0x30]（param_2）; call 0x51AAD0 → al
4747F9: cmp bl,al; jne 0x47482F                     ; check2: getter 不一致 → 0
4747FD: mov ebx,[edi+0xC]
474800: cmp ebx,[ebp+0xC]; jne 0x474926             ; ★controller count 不一致
474809: cmp ebp,edi
47480B: mov byte [esp+0x38],1                       ; ★成功 → 1
474810: jne 0x47482F                                ; 非自己 → 1 のまま維持
474812..474827: push ...; push 0xA3CA28; call 0x4A7A60（self エラー trace）
47482A: mov byte [esp+0x38],0                       ; self → 0
474926..474947: push ...; push 0xA3C9C8; call 0x4A7A60; jmp 0x47482F（count 不一致 trace → 0 維持）
```

### §2.4 分岐の確定（v383_3(a) 再掲＋本裁定で意味づけ）

```
47494C: cmp byte [esp+0x38],0
474951: je 0x474977                ; flag==0 → crossfade 経路
474953..47496D: push ...; call 0x6C4060   ; flag≠0 → blend wrapper（→0x6C9E00→state4/5）
474977..474999: [edi+0x44]!=0 ゲート; call 0x6C4000  ; crossfade wrapper（→state3/2）
```

### §2.5 文字列実読（v391_2(b)/v391_3(a)）

- `0xA3CA28` = `"Morph Error: Trying to morph from sequence to itself.\r\n'%s' on '%s'."`
- `0xA3C9C8` = `"Morph Error: Controller count not the same.\r\n'%s' has %d controllers and\r\n'%s' has %d on '%s'."`

## §3 残点 **V-391-1..3**
1. **`0x51AAD0` getter の identity**（何を返す byte か・Morph 検証の check1/check2 の意味）【低】
2. **fn `0x474530` の正式名**（decompiled FUN_ 名のみ・"Morph" のどのメソッドか）＋`[seq+0xC]`=controller count は文字列根拠（exe label 不在）【低】
3. **`[esp+0x28]`（`0x4747D9`/`0x474833` で設定される別 byte flag）の意味**【低】

## §4 検証・自己批評
- probe: **v391_1..v391_3 全 EXIT=0**。
- **決定打の作り**: (i) prologue で引数スロット特定（sub esp/push 数から `[esp+0x34]`=param_3・`[esp+0x3C]`=param_4）→ (ii) **flag writers 全3件 census** → (iii) 検証ブロックの unfiltered（3条件を直接確認）→ (iv) **trace 文字列の実読で "Morph" と controller count を確定**。
- 妥当性チェック: ①count 不一致 trace ⇔ `cmp [edi+0xC],[ebp+0xC]` — ②self trace ⇔ `cmp ebp,edi` 後の =0 — ③成功=1⇔ `je/jne` 全 fail 経路が0維持 — ④flag=1⇔blend⇔state5/4（v383/v379）— **四方向で矛盾なし**。
- docs との不整合: **なし**（v383 の V-383-1 を解消。v383 の「`[esp+0x38]` が blend/crossfade を選択」は正・本 v391 でその入力（Morph 検証合否）を確定）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
