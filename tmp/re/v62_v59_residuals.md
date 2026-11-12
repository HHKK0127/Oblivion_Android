# V62 — V59 §7 未解決3点の解消（sHandToHandIcon / s\*Description 12件 / 表B 32件）

対象: `Oblivion.exe` (Steam v1.2.0416, MD5 cdd2f0c5eff198d4f26b7b5b54ce4930)
手法: PE 自前解析（struct）＋ capstone 5.0.7 ＋ `docs\Oblivion_decompiled.c`（1,066,496行、ラベル照合は lowercased）＋ `rg` 15.2.0
既存ドキュメントは無編集。本ファイルのみ新規。

---

## 1. セルフテスト 4件 PASS

`%TEMP%\opencode\v59_st.py` を再実行（同一マッピング・同一バイナリ）:

| # | 検査 | 期待 | 実測 | 結果 |
|---|---|---|---|---|
| ① | `bytes@0x504050` | `518b4c24188b5424` (8B) | `518b4c24188b5424` | PASS |
| ② | `bytes@0x4FB580` | `578b7908` | `578b7908` | PASS |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| ④ | テーブル(基底`0xB0AF48`, stride`0x28`) opid `0x0015` impl | `0x504560` | idx21 @`0xB0B290` name=`OnLoad` opid=0x15 impl=`0x504560` | PASS |

**SELFTEST 4/4 PASS**

---

## 2. 点1 — `sHandToHandIcon` 読取3箇所の UI 文脈解明

### 2.1 実体

| 項目 | VA | 備考 |
|---|---|---|
| key 文字列 `sHandToHandIcon` | `0xA3D064` (.rdata) | key VA の非テキスト領域ポインタ参照 = **0件** |
| value 文字列 `Icons\Weapons\HandToHand.dds` | `0xA3D074` (.rdata) | 同上 0件 |
| bss Setting オブジェクト | `0xB33D84` | .data 非掲載（bss）、非テキスト参照 0件 |

### 2.2 参照の全走査結果（binary）

- **key VA `0xA3D064` の .text 参照 = 1件のみ**: `0x9DC1C5` — 登録 thunk `FUN_009DC160` 内。
  ```
  0x9DC1C0: push 0xA3D074          ; value
  0x9DC1C5: push 0xA3D064          ; key
  0x9DC1CA: mov  ecx, 0xB33D84     ; bss Setting obj
  0x9DC1CF: call 0x419A70          ; FUN_00419A70 = 中央マップ 0xB35574 への登録
  ```
  → **key 経由の map lookup 読者は 0 件**。
- **obj VA `0xB33D84` の .text 参照 = 5件**:
  1. `0x9DC1CB`（登録 thunk 本体）
  2. `0xA18651`（dtor thunk `FUN_00A18650`: `mov ecx,0xB33D84; jmp 0x4055E0`）
  3. `0x478890` / `0x5A6A57` / `0x5A6D55` = **V59 が計測した「コード直接参照3箇所」の正体**（下表）

V59 §4.3 の記載「コード直接参照 3 箇所: 0x478890 / 0x5A6A57 / 0x5A6D55」は key 文字列への参照ではなく、**bss オブジェクト `0xB33D84`（value フィールド）への生参照**であった。

### 2.3 3箇所の各文脈（capstone + decompiled の両照合）

すべて `mov reg, dword ptr [0xB33D84]` = **Setting オブジェクトの value フィールド（char\* アイコンパス）を直接デリファレンス**する生参照。getter `FUN_00565D10`（表Icon）は一切経由しない。

| # | 参照 | 関数 | 処理内容（証拠） |
|---|---|---|---|
| 1 | `0x478890` | `FUN_00478780`（アタッチスロット解放、slot clear系。`+0xcc/+0xdc/+0x10c/+0x11c/+0x12C` 等へクリアする小関数群から呼ばれる） | `[ebx+0x150] == DAT_00B333C4`（player ref）かつ `ebx == FUN_006600D0(player,0)`（＝`[player+0x104]`、player 自身のオブジェクト）のとき `FUN_0057B190([0xB33D84])` を呼ぶ。**プレイヤーの武器スロット解放（装備解除）時に HUD 武器アイコンを H2H へフォールバック**。証拠: 0x478875-0x47889A の aligned disasm |
| 2 | `0x5A6A57` | `FUN_005A6A40`（HUD アイコン更新ヘルパ、`FUN_0057B190` の唯一の末端） | `FUN_0058CED0(0xFE6, path)`（HUD メニュー変数 0xFE6 ← アイコンパス）→ `__mbscmp(path, [0xB33D84])` → 結果(==0 なら H2H)を `FUN_005A6220(menu, isH2H)` へ。`FUN_005A6220` は **H2H 以外のみ**チャージ/状態変数 0xFAE/0xFAF を更新（H2H は 0xFAE=0 で非表示）。**H2H 判定用の比較オペランド** |
| 3 | `0x5A6D55` | `FUN_005A6B00`（**HUD メニュー初期化**。`Data\Menus\Main\hud_main_menu.xml` 読込・"Hud Menu Creation Failed..." メッセージ保有） | 装備中武器のモデル名から `sprintf("%s\\%s","Icons",name)` を構築して `FUN_0057B190` に渡すが、武器無し（構築失敗, bVar1=false）のとき `FUN_0057B190([0xB33D84])` = **初期アイコンを H2H.dds に** |

補助経路の確定: `FUN_0057B190(icon_path)` = HUD 武器アイコン setter（メニュー0x3EC の存在＋変数0xFAEのセンチネル確認後 `FUN_005A6A40` 呼び出し）。setter の他の呼出元は装備時（`FUN_00479780`: `sprintf("%s\\%s","Icons",item->+0x4c)` → `FUN_0057B190`、`FUN_0047BC40`: slot3=hand の player 分岐で同様）であり、3箇所はすべて「H2H（素手）をアイコン候補として扱う」HUD 武器アイコン系に完全に閉じる。

### 2.4 判定

**登録は中央マップ（`FUN_00419A70`→`0xB35574`）へ行われるが、読者は map lookup でも icon getter でもなく bss オブジェクト `0xB33D84` の value フィールドを直接読む生参照。用途は HUD 武器アイコンの「初期化・装備解除フォールバック・H2H 判定」の3系統のみ。**

---

## 3. 点2 — `s<名>Description` 別キー族12件の lookup 全走査

### 3.1 全件の登録構造（binary 確定）

12件すべて同一パターン（属性8+クラス3 = `FUN_009E62B0` 内 `0x9F0A90`..`0x9F0C5F`、Race = `FUN_009F6040` 内 `0x9F68F0`）:

```
push <value str> ; push <key str> ; mov ecx, <obj> ; call 0x419A70   (登録)
dtor thunk: mov ecx, <obj> ; jmp 0x4055E0                            (FUN_00A21240.. 群)
```

（登録 thunk は Ghidra 上 `FUN_009E62B0`/`FUN_009F6040` が SEH 付き巨大 CRT 初期化のため**デコンパイル本文がスタブ化**。decompiled grep では属性8の obj VA 0件。**根拠は capstone binary scan** が正。）

### 3.2 参照全走査（key VA / obj VA、.text＋非テキスト）

| # | key | obj VA | key VA refs（.text） | obj VA refs（.text） | 読者 |
|---|---|---|---|---|---|
| 1–8 | `sStrengthDescription` .. `sLuckDescription`（属性8） | `0xB38590`..`0xB385C8` | **登録のみ**（各1件） | **登録 thunk + dtor thunk のみ**（各2件） | **0件** |
| 9 | `sCombatDescription` | `0xB385F0` | 登録のみ | 登録+dtor+**`0x5D5CBA`** | `FUN_005D5B40` |
| 10 | `sMagicDescription` | `0xB385F8` | 登録のみ | 登録+dtor+**`0x5D5C57`** | `FUN_005D5B40` |
| 11 | `sStealthDescription` | `0xB38600` | 登録のみ | 登録+dtor+**`0x5D5C8D`** | `FUN_005D5B40` |
| 12 | `sRaceDescription` | `0xB38FB0` | 登録のみ | 登録+dtor+**`0x5C8205`,`0x5C8236`,`0x5CA5F2`,`0x5CDB72`,`0x5CDD87`,`0x5CDE19`** | `FUN_005C7BA0` / `FUN_005CA010` / `FUN_005CDA30` |

- **lookup（`0x55E000` 系・getter 経由・key push）で読む関数は 12件すべてで 0件**。key 文字列 VA は登録 thunk 以外どこからも参照されない。
- 動的キー構築（文字列結合）も不成立: EXE 全ファイルに `%sDescription` / `s%sDescription` / `Description%s` / `%sIcon` の書式片 **0件**。
- 非テキスト領域（.rdata/.data）に obj VA / key VA の追加ポインタ参照 **0件**（V59 §4.1 の「どのテーブルにも出現しない」を再確認）。

### 3.3 読者4件の文脈（decompiled）

| key群 | 読者関数 | 文脈 | 参照形式 |
|---|---|---|---|
| sCombat/sMagic/sStealthDescription | `FUN_005D5B40`（chargen **skills_menu** 系の説明更新。`FUN_005D6390`=`Data\Menus\CharGen\skills_menu.xml` 初期化・`FUN_005D5E50` から呼ばれる） | `iVar2 = [this+0x3c]`（カテゴリ）==2 のとき、`param_2` 0/1/2 で `FUN_0058CED0(0xFAF, [0xB385F0/0xB385F8/0xB38600])` — **デフォルト3クラス（Combat/Magic/Stealth）のクラス説明文をメニュー変数0xFAFへ**（アイコンは `[0xB38608/10/18]` も同様に直読み）。カテゴリ1（スキル）では表C getter `FUN_00565CF0`＋表Icon getter `FUN_00565D10` を使うのと対照的に、**クラス側は生 obj 直読み** | 生 obj value 参照 |
| sRaceDescription | `FUN_005CA010`（**RaceSexMenu 初期化**。`Data\Menus\CharGen\race_sex_menu.xml` 読込） / `FUN_005CDA30`（レース選択ハンドラ、TESRace リスト `DAT_00B33A98+0x44` を走査） / `FUN_005C7BA0`（目/体情報更新） | 選択レースが無い/ルートパスで `[0xB38FB0]` をメニュー変数 **0xFB4** の既定説明として設定（`FUN_004028D0` 文字列構築＋`FUN_0058CED0`）。レース-specific DESC は vtable `+0x10(0,"DESC")`（=0x43534544）で取得した値に置換される | 生 obj value 参照 |

### 3.4 判定

- **属性8件（`sStrengthDescription` 等）: (乙) 登録のみ・読取なし** — key 経由 lookup 者0、obj 直接参照も初期化/dtor のみ。placeholder 値 `Need a gamesetting description.`（`0xA5DE7C` 共有）が UI に現れる経路は EXE 内に存在しない。
- **クラス3件 + sRaceDescription: 実在の読者あり（生 obj value 参照）** — chargen のクラス選択（skills_menu, 変数0xFAF）と RaceSexMenu（変数0xFB4）で使用。ただし**第4系統（key lookup）ではなく bss オブジェクト直接参照**であり、V59 §7-2 の「中央マップ lookup 経由で読む」との**推定は棄却**。

---

## 4. 点3 — 表B 32件の完全列挙と13件の判定

getter `FUN_00565CC0` 実機（再確認）:

```
0x565CC0: mov eax,[esp+4]; cmp eax,0x27; ja 0x565cd7
0x565CC9: mov eax,[eax*4+0xB12758]; test; mov eax,[eax]; ret   ; 表A idx0..39 (Setting* deref)
0x565CD7: cmp eax,0x48; jge ret0
0x565CDC: mov eax,[eax*4+0xB12868]; ret                        ; 表B idx40..71 (char* 直返、deref 無し)
0x565CE4: xor eax,eax; ret
```

### 4.1 32件完全表

| idx | slot VA | str VA | 文字列 | .text VA参照数 | 参照先 |
|---|---|---|---|---|---|
| 40 | 0xB12908 | 0xA65B64 | Magicka Multiplier | 0 | — |
| 41 | 0xB1290C | 0xA65B54 | Night Eye Bonus | 0 | — |
| 42 | 0xB12910 | 0xA65B44 | Attack Bonus | 0 | — |
| 43 | 0xB12914 | 0xA65B34 | Defend Bonus | 0 | — |
| 44 | 0xB12918 | 0xA65B24 | Casting Penalty | 0 | — |
| 45 | 0xB1291C | 0xA51D60 | Blindness | 0 | — |
| 46 | 0xB12920 | 0xA339B0 | Chameleon | 1 | FUN_00417420 @0x417B39 |
| 47 | 0xB12924 | 0xA339BC | Invisibility | 1 | FUN_00417420 @0x417B14 |
| 48 | 0xB12928 | 0xA51D54 | Paralysis | 0 | — |
| 49 | 0xB1292C | 0xA3399C | Silence | 1 | FUN_00417420 @0x417B92 |
| 50 | 0xB12930 | 0xA51D48 | Confusion | 0 | — |
| 51 | 0xB12934 | 0xA65B10 | Detect Item Range | 0 | — |
| 52 | 0xB12938 | 0xA65AFC | Spell Absorb Chance | 0 | — |
| 53 | 0xB1293C | 0xA65AE4 | Spell Reflect Chance | 0 | — |
| 54 | 0xB12940 | 0xA65ACC | Swim Speed Multiplier | 0 | — |
| 55 | 0xB12944 | 0xA33C14 | Water Breathing | 1 | FUN_00417420 @0x417434 |
| 56 | 0xB12948 | 0xA33C04 | Water Walking | 1 | FUN_00417420 @0x417459 |
| 57 | 0xB1294C | 0xA33A08 | Stunted Magicka | 1 | FUN_00417420 @0x417A62 |
| 58 | 0xB12950 | 0xA65AB8 | Detect Life Range | 0 | — |
| 59 | 0xB12954 | 0xA338F8 | Reflect Damage | 1 | FUN_00417420 @0x417DEF |
| 60 | 0xB12958 | 0xA33930 | Telekinesis | 1 | FUN_00417420 @0x417D5B |
| 61 | 0xB1295C | 0xA337EC | Resist Fire | 1 | FUN_00417420 @0x41807E |
| 62 | 0xB12960 | 0xA337DC | Resist Frost | 1 | FUN_00417420 @0x4180A3 |
| 63 | 0xB12964 | 0xA337AC | Resist Disease | 1 | FUN_00417420 @0x418112 |
| 64 | 0xB12968 | 0xA337BC | Resist Magic | 1 | FUN_00417420 @0x4180ED |
| 65 | 0xB1296C | 0xA33770 | Resist Normal Weapons | 1 | FUN_00417420 @0x418181 |
| 66 | 0xB12970 | 0xA33788 | Resist Paralysis | 1 | FUN_00417420 @0x41815C |
| 67 | 0xB12974 | 0xA3379C | Resist Poison | 1 | FUN_00417420 @0x418137 |
| 68 | 0xB12978 | 0xA337CC | Resist Shock | 1 | FUN_00417420 @0x4180C8 |
| 69 | 0xB1297C | 0xA33A24 | Vampirism | 1 | FUN_00417420 @0x417A1B |
| 70 | 0xB12980 | 0xA33950 | Darkness | 1 | FUN_00417420 @0x417CE7 |
| 71 | 0xB12984 | 0xA3375C | Resist Water Damage | 1 | FUN_00417420 @0x4181A6 |

（参照 19件 / 参照 0件 13件 — V59 の件数実測と一致）

### 4.2 参照19件の性質 — 表B 読取ではない

19件の参照は**すべて `FUN_00417420` の中**にある。文脈は一律:

```
push <fourcc>            ; 例: 0x4C4D4843 "CHML", 0x52424157 "WABR", 0x504D4156 "VAMP"
push <表Bと同じ文字列VA>  ; 例: push 0xA339B0 'Chameleon'
...
call 0x417220            ; EffectSetting 登録（アロケータ+fourcc+name+FUN_00452570 Form登録）
```

`FUN_00417220` は EffectSetting インスタンスを確保し fourcc(+0x98)・表示名(param_2→`FUN_00416580`)を格納して Form マップ登録する**MGEF 静的登録関数**。＝**MGEF 表示名リテラルと表B 文字列リテラルが同一アドレスを共有しているだけ**で、表B への読取ではない（`FUN_00417420` の呼出元は `FUN_00418DB0`/`FUN_00418E50`）。

### 4.3 参照0の13件 — 別形式参照の全走査

| 検査 | 結果 |
|---|---|
| ASCII 全ファイル走査（同一文字列の追加リテラル） | 12件は .rdata の主リテラル1個のみ。`Magicka Multiplier` のみ `.rdata 0xA3371C` に**別リテラル**が存在するが、当該リテラルも **.text 参照0**（MGEF 名クラスタ側の未使用リテラル。表B リテラル `0xA65B64` への参照とは別物） |
| UTF-16LE 走査（ロケール/リソース経由） | 全13件 **0件** |
| ポインタ走査（全セクション、string VA を指す4バイト値） | `Blindness`/`Paralysis`/`Confusion` のみ **AV コード名テーブル**（基底 `0xB0A1A8`、AV0 `Strength` .. AV71 `ResistWaterDamage`、`Begin`→"" 終端）にも同リテラルのポインタが存在。**ただしこのテーブルは別システム**: `0xB0A1A8` の .text 参照は `FUN_004F45D0`/`FUN_004F6060`（`cmp byte,[0xB361AC],0` でゲートされたデバッグログ出力、`mov ecx,[edi*4+0xB0A1A8]`＋書式 `%g %s ...`）と `FUN_004FDE30`（`edi` を0..0x48 で走査する全AVダンプ）の3件のみ。表B 表示（スペース区切り名）とは別テーブル・別用途（無区切りコード名） |
| 文字列結合/ランタイム生成 | 書式片0件（§3.2 と同一走査） |

### 4.4 「getter が唯一の表B読者」の証明

1. テーブル base `0xB12868` の .text 参照 = **`FUN_00565CC0` の `0x565CDF` の1件のみ**（テーブル走査・他getter含め全走査で他0件）。
2. 個別スロット `0xB12908`..`0xB12984` への直接参照 = **0件**。
3. 文字列 VA への直接参照は「MGEF 登録によるリテラル共有19件＋0件13件」であり、いずれも表B からの読み出しではない。
4. `FUN_00565CC0` の E8 呼出しは **34件**（`FUN_00413A90` MGEF表示名生成 `%s %s` sprintf、`FUN_00413BE0`、`FUN_00415390`/`FUN_004156B0`、`FUN_0041CBB0`、`FUN_0052EA90`、`FUN_005A0260`/`FUN_005A08E0`、`FUN_005ACB60`、`FUN_005D5610`、`FUN_005D6390` skillsメニュー、`FUN_005DBBD0`/`FUN_005DC630`/`FUN_005DD4B0`、`FUN_005F8890`(5回)、`FUN_005FEBF0`(3回, ダメージ系)、`FUN_00666BB0`、`FUN_006684B0`、`FUN_006A83D0` 等）。idx 40..71 を受ける唯一の経路が `0x565CDC` 枝であり、AV40+ の表示名（"Blindness" 等）が必要な MGEF 名生成・ツールチップ等がこの枝の消費者。

### 4.5 判定

**(丙) 純静的データで正常** — 表B スロットの唯一の reader は `FUN_00565CC0` の idx40..71 パス。参照0の13件は「getter 経由でのみ読む・静的 VA 参照なし」の正常な純静的データであり、未使用領域でも別形式参照でもない（`Blindness`/`Paralysis`/`Confusion` のみ他システム〔AVコード名テーブル〕とリテラルを共有）。

---

## 5. 結論（3点）

| 点 | 結論（1行） |
|---|---|
| 1. `sHandToHandIcon` | **(登録済・生 obj 参照)** — key は登録 thunk のみ参照、3箇所は bss obj `0xB33D84` の value 直読みで、すべて HUD 武器アイコン系（初期化 `FUN_005A6B00`／装備解除フォールバック `FUN_00478780`／H2H判定 `FUN_005A6A40`）。getter/lookup 非経由。 |
| 2. `s*Description` 12件 | **8件は (乙) 登録のみ・読取なし**（属性8、lookup 呼出し元0・動的キー構築0）、**4件は実在の読者あり**（クラス3＝chargen スキル/クラスメニュー変数0xFAF、sRaceDescription＝RaceSexMenu 変数0xFB4）— ただし読者はいずれも bss obj の生 value 参照であり「第4系統 key lookup」は不存在（V59 の推定は棄却）。 |
| 3. 表B 32件 | **(丙) 純静的で正常** —32件完全列挙、base 参照= getter `0x565CDF` のみ・スロット直参照0、参照19件は MGEF 登録（`FUN_00417420`）によるリテラル共有、参照0の13件は別形式参照なし＝getter idx40..71 パスが唯一の表B 読者。 |

---

## 6. 未解決点

1. 属性8件 `s*Description` の placeholder が **ESM GMST 読込時（V55 の key→ptr 上書き機構）で実行時に差し替えられるか**は本タスク範囲外（EXE 内に読者はいないが、ESM 側の該当 GMST 有無は未検証）。差し替えられても読む EXE コードは存在しない。
2. `FUN_00478780` の `this`（ebx）の型名（HighProcess 系か TESObjectREFR 系か）は未特定。条件式 `[this+0x150]==player` / `this==[player+0x104]` と slot clear 族（`+0xcc/+0xdc/+0x10c/+0x11c/+0x12C`）である事実のみ確定。
3. AV コード名テーブル `0xB0A1A8` の reader 3件（`FUN_004F45D0`/`FUN_004F6060`/`FUN_004FDE30`）はデバッグ出力（`[0xB361AC]` ゲート＋`%g %s` 書式）と全AVダンプと判明したが、正式な呼び出し元（コンソール/ログのどの機能か）は未特定。
4. Ghidra デコンパイルは `FUN_009E62B0`/`FUN_009F6040`（SEH 付き巨大 CRT 初期化）の本文をスタブ化しており、登録箇所の照合は capstone binary 側で実施した（decompiled grep では検出不可）。

---

## 7. 使用スクリプト（すべて `%TEMP%\opencode\`）

| ファイル | 用途 |
|---|---|
| `v59_common.py` / `v59_st.py` | 既存の PE 共通部・セルフテスト（再実行、4/4 PASS） |
| `v62_common.py` | v59_common 拡張: decompiled 関数インデックス（`v62_decl_index.json` キャッシュ、20,465関数）・func_of/fun_src・scan_text_refs/scan_all_refs/find_str_va/disasm |
| `v62_p1.py` / `p1b` / `p1c` / `p1d` / `p1e` | 点1: key/obj VA 参照走査・3関数デコンパイル・`FUN_0057B190`/`FUN_005A6220`/`FUN_006600D0` 解析・aligned disasm |
| `v62_p2.py` / `p2b`..`p2h` | 点2: 12件 key/obj 参照全走査・分類・読者4関数デコンパイル・メニューXML特定・decompiled hex grep |
| `v62_p3.py` / `p3b`..`p3e` | 点3: 32件列挙・base/slot 参照走査・getter callers E8スキャン(34件)・13件のASCII/UTF16/ポインタ走査・AVコード名テーブル解析・`FUN_00417220` 確認 |
| `v62_final_sweep.py` | 動的キー書式片0件・key/value VA 非テキスト参照0件の最終確認 |

*本ファイルは V62 の新規成果物。既存ドキュメントは無編集。*
