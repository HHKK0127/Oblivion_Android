# V57 — 設定・キー・表示名 統合移植仕様書

- 作成日: 2026-10-24 / 作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
- 対象: `Oblivion.exe` (Steam v1.2.0416) の「設定・キー・表示名」系システム全体
- **出典4資料（本タスクの唯一の入力・全て実測確定済みとして信頼）**:
  1. `tmp\re\v51_inisettings_spec.md`（V51, 511行）— INISettings 抽象（vtable 9スロット・4インスタンス・574エントリ）
  2. `tmp\re\v55_gmst_override_path.md`（V55）— GMST 上書き/解決経路（`0xB35574` = `0xB35468+0x10C` 埋め込みマップ）
  3. `tmp\re\v49_registry_type.md`（V49, 261行）— 中央マップ `0xB35574` の型解明（分離連結ハッシュ・登録2,304件）
  4. `tmp\re\v41_effect_display_name_spec.md`（V41, §8=V48 訂正を含む）— MGEF 効果表示名生成（prefix5・表A/B/C/表Icon）
- **本仕様の方針**: 4資料の事実を転記統合し、**食い違う箇所は §3 で列挙して統合**する。推測で埋めない（不明は §7 へ）。
- タスク指示由来で4資料の本文に無い事項（終了時 SaveAll、String 12件 = V50 確定等）は、出典を明記して扱う。

---

## 1. 全体構造図

「永続化層(INI 574)」「登録簿(0xB35574 = 2,304)」「変数本体(GMST f/s/i)」「表示名系(prefix/AV/Desc/Icon)」「MGEF fourcc マップ(0xB33508)」の関係と、各層の読み書き主体。

```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃  静的初期化層（CRT .CRT$XCU / SEH+atexit(0x981FB4) 付き thunk 群）— 登録は全てここで完結（runtime insert 0件）  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
   │ setter 0x41BAE0 (float, 992件) / setter 0x419A70 (obj, 1,312件)          │ CRT 登録 thunk 群
   │ thunk 例: push value; push key; mov ecx,<BSS entry>; call 0x419A70        │ 0x9F9660+0x20i (表A 40)
   │                                                                          │ 0x9F9B60+0x20i (表C 33)
   ▼                                                                          │ 0x9D9EE0..0x9D9F6F (prefix5)
┌─────────────────────────────────────────────────┐                           │ 0x9D8040 等 (INI entry 572)
│  登録簿: SettingsMap @0xB35574                    │◀──────────────────────────┘
│  = GMST/GameSetting マネージャ 0xB35468 の       │
│    埋め込みフィールド this+0x10C（V55 確定）      │
│  ctor 0x4A7E60(401) / node{next@0,key@4,value@8} │
│  insert 0x412D30(先頭prepend) / lookup 0x55E000   │
│  remove 0x4524C0 / 重複警告 0x4A7A60(fmt 0xA32490)│
│  総登録 2,304 = f*978 + s*970 + i*227 (=2,175)   │
│      + Desc45 + AV名36 + sMagicEffectItem*17      │
│      + Icon17 + 未分類14                          │
└───────┬──────────────────────────┬────────────────┘
        │ lookup only（insert なし）│ key→value 参照（getter）
        ▼                          ▼
┌─────────────────────────┐  ┌──────────────────────────────────────────────────────┐
│ 変数本体層（GMST f/s/i）  │  │ 表示名系（同マップ登録・getter は idx→静的テーブル経由） │
│ 例 0xB37B88 (fDiff...   │  │  ・prefix5: 0xB334C0..E0 {prefix, sMagicEffectItem*key} │
│  =10.0f) {value@0,key@4}│  │  ・表A AV名  : 0x565CC0 (0xB12758×40 + 0xB12868×32=72)    │
│ ▲既定値: 静的 init       │  │    key: sAttributeName*/sDerivedAttributeName*/sSkillName*│
│   0x9ED780→0x41BAE0     │  │    /sTraitName*/sVirtueName*                             │
│ ▲ESM上書き: 経路         │  │  ・表C Desc : 0x565CF0 (0xB127F8×33, idx≤0x20)            │
│   0x44E326→0xB35468     │  │    key: sAttributeDesc*/sDerivedAttributeDesc*/sSkillDesc* │
│   slot9 0x4A8040        │  │  ・表Icon   : 0x565D10 (0xB12880×33, idx≤0x20)            │
│   →lookup 0x55E000      │  │    key: sHandToHandIcon / sAttributeIcon*                 │
│   →0x4A7D60→0x4510E0/   │  │                                                            │
│     0x450C20 (DATA 4B   │  │ 表示器 0x413A90: fourcc 分岐 → prefix 選択                 │
│     直接書込・未知キー無視│  │   REAT→Restore / FOSK,FOAT→Fortify / ABSK,ABAT→Absorb      │
│ ▲SetGS: 0x514100系      │  │   DRSK,DRAT→Drain / DGAT→Damage / default137種→prefixなし  │
│   (atof/atoi→変数直書込) │  │   sprintf("%s %s", prefix, 0x565CC0(av)) 書式 0xA32564     │
│ ▲読取: 変数直接参照       │  │   呼出4件: 0x413BE0/0x41CBB0/0x593CD0/0x5B2B70 (全てUI)   │
│   例 0x5E2560            │  └──────────────────────────────────────────────────────────┘
│   fmul dword[0xb37b88]  │
└─────────────────────────┘        ※MGEF fourcc マップ 0xB33508（同胞・整数キー、
                                      ctor 0x418DB0、vtable 0xA32C34、hash=key%37）
                                      ※fourcc 化の直接証拠は4資料内で未検証（§7-3）
                                      ※0xB33508↔0xB35574 相互参照も未検証（§7-2）

┌──────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 永続化層: g_INISettings @0xB07BF0（ISettingsBackend、vtable 0xA404F8 — 登録簿 0xB35574 とは**別オブジェクト**）│
│  エントリ 574件 "key:Section"（.data 静的 8B レコード、GMST 変数と同一 {value,key} レイアウト）              │
│  ▲登録: CRT 静的 572 + ランタイム 2 (0x404893 / 0x4440A3)                                                    │
│  ▲読:   GetINISetting 0x514230 ← FindSetting 0x50EC30（ecx=0xB07BF0 固定）                                  │
│  ▲書:   SetINISetting 0x5143C0 ← 同 FindSetting                                                             │
│  ▲SaveIniFiles (opid 0x0165) 0x513B60: Begin(1)→SaveAll→End（SaveAll=backend vt+0x0C WriteOne 全走査）      │
│  ▲RefreshINI (opid 0x013E) 0x513BB0: Begin(0)→LoadAll→End→コンソールメッセージ 0xA4D690                    │
│  ▲起動部 0x40DF80 内: Begin(0)→LoadAll(574)→End / 欠損時 CopyFileA("Oblivion_default.ini")                  │
│  ▲起動完了 0x40F58E: 0x40C180(path,1) = Open+SaveAll+End の自動全量書出（=V53 判断待ち）                     │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────┘

【両層の共通基盤（矛盾ではなく統合の要）】
  ・型判定 0x4A78D0 — INI 型表(b/c/f/i/r/a/s/u/h) と ESM GMST 型判別('f'→5/'i'→3/'s'→6) が**同一関数**
  ・String set 0x4A7980 — INI ReadOne type6 と ESM GMST string 書込が**同一関数**
  ・警告ロガー 0x4A7A60 — マップ警告(fmt 0xA32490 "in map") と INI リスト警告(fmt 0xA2FAB0 "in list") が**同一関数**
  ・+0x108 フィールド — INI backend=セッション(this or 0) / GMST manager=現在レコード ptr（同オフセット別意味）
```

読み書き主体の要約:
- **静的 init**: 登録簿 2,304件、INI 574件（572）、prefix5、表A/B/C/Icon の全登録
- **ESM ロード**: 登録簿へ insert せず、命中した変数本体へ DATA 4B を直接書込（未知キー無視・map 不変）
- **コンソール**: SetGS 0x514100系 / GetGS 0x513FC0系（変数本体）、SetINISetting/GetINISetting/SaveIniFiles/RefreshINI（INI 層）
- **SaveIniFiles / 起動自動書出**: INI 層のみ（登録簿・変数本体には一切書き戻さない）

---

## 2. key の名前空間対応表

| 名前空間 | 例 | 登録者（誰が登録するか） | lookup / getter（誰が読むか） | 0xB35574 との関係 |
|---|---|---|---|---|
| **GMST editorID** (`f*`/`s*`/`i*`) | `fDifficultyDamageMultiplier` (0xA5B128) | 静的初期化 setter `0x41BAE0`(float) / `0x419A70`(obj)。合計 f*978+s*970+i*227 = **2,175** | lookup `0x55E000`（ESM slot9 `0x4A8040` / GetGS `0x513FC0` / SetGS `0x514100`）。runtime 読取は**変数直接**（例 `0x5E2560`） | **同一マップ・同一命名空間**（V55§6 (aa) 確定） |
| **INI "key:Section"** | `"bUse Joystick:Controls"` (0xA2FB18)、`"sBasePath"`（無コロン→"MAIN"） | CRT 静的 572 + ランタイム 2 → `AddSetting 0x4040E0` で g_INI `0xB07BF0` の +0x10C リストへ | `FindSetting 0x50EC30`（呼出元 = GetINISetting `0x5142B4` / SetINISetting `0x51444F` の2件のみ、ecx=0xB07BF0 固定） | **無関係な別オブジェクト**（登録簿に登録されない）。命名の重なり有無は**資料内で未検証**（§7-1） |
| **sAttributeName\* 系（表A AV表示名）** | `sAttributeNameStrength` ほか `sDerivedAttributeName*`/`sSkillName*`/`sTraitName*`/`sVirtueName*` | thunk `0x9F9660+0x20i`（40個、.rdata 0xA2AA78 CRT 初期化テーブル）→ `0x419A70` | 表A getter `0x565CC0`: idx0..39 = `0xB12758` → Setting*→value、idx40..71 = `0xB12868`（.rdata char\* 直返・登録なし） | 登録簿に登録（V49: AV名**36件**、 thunk は**40個** — §3-2 参照） |
| **\*Desc\*（表C 説明文）** | `sAttributeDescStrength` / `sDerivedAttributeDescHealth` / `sSkillDescArmorer` | thunk `0x9F9B60+0x20i` → `0x419A70` | 表C getter `0x565CF0`: `0xB127F8` 33エントリ idx≤0x20。**表Icon と対で** UI が取得（LevelUp `0x5ACE20` / `0x5D5B40` / `0x5DBBD0`×2） | 登録簿に登録（V49: *Desc* **45件**、テーブル**33** — §3-1 参照） |
| **sAttributeIcon\* 等（表Icon）** | `sHandToHandIcon` [0]、`sAttributeIcon*` [1-]（fallback = `Menus\Stats\stat_pop_icon_*.dds`） | 同登録系 thunk（V49§5.4 実読） | 表Icon getter `0x565D10`: `0xB12880` 33エントリ idx≤0x20 | 登録簿に登録（V49: Icon **17件**、テーブル**33** — §3-3 参照） |
| **sMagicEffectItem\*** | `sMagicEffectItemFortify` (0xA32754) ほか（\*For/On/In/Seconds(Singular\|Plural)/Percent/Points(Singular\|Plural) 等） | prefix5 はスタブ `0x9D9EE0..0x9D9F6F` → `0x419A70`（obj = `{prefix文字列, key}`）。残りは同系 thunk | 表示器 `0x413A90`（prefix 5オブジェクト `0xB334C0..E0` を fourcc 分岐で参照）+ UI ローカライズ | 登録簿に登録（V49: **17件**。うち5件が prefix） |
| **MGEF fourcc** (`FOSK`/`REAT`/...) | 4文字コード | （4資料内で未検証 — §7-3） | 表示器 `0x413A90` の比較分岐（0x413AFA/0x413ABE/0x413AEF/0x413B72/0x413B7A） | 同胞マップ `0xB33508`（整数キー）が該当と推定されるが**根拠は4資料内で未検証** |

**重なりの別**:
- 検証済: GMST editorID ↔ 登録簿 0xB35574（同一・V55§6）。表A/B/C/Icon/sMagicEffectItem\* ↔ 登録簿（V49/V41 で登録 thunk 実測）。
- **未検証**: INI 574 キーと GMST 2,175 キーの命名一致有無（§7-1）。`0xB33508` と `0xB35574` の相互参照有無（§7-2）。
- 補足: ESM GMST は EDID で lookup するため s\* 系キーも理論上命中し得るが、ESM の GMST editorID は f\*/s\*/i\* 接頭辞に限られ（V55§6）、AV 名/Desc/Icon は ESM 側に同名 GMST が無いため実質無衝突。

---

## 3. 資料間の矛盾・不整合と統合方針（6件）

| # | 矛盾内容 | 出典A | 出典B | 統合方針（本仕様の採用） |
|---|---|---|---|---|
| 1 | `*Desc*` キー **45件** vs 表C テーブル/ getter **33エントリ** | V49§3（登録件数表: Desc45） | V49§5.1・V41§3（表C = 0xB127F8 33件 idx 0..0x20、内訳 8+4+21=33） | 「45」はキー名パターンのセンサス、「33」は getter テーブル実測。**差分12件の所在は4資料内で未検証**（§7-4）として併記 |
| 2 | AV 表示名キー **36件** vs 表A 登録 thunk **40個**（+表B32 = 72 AV名） | V49§3（AV名36） | V41§8（表A thunk `0x9F9660+0x20i` 40個、imm32=0xB3A034+8i が 40/40 全件2ヒット） | 「36」は s\*Name\* パターンのセンサス、「40」は thunk 実測。**差分4件は未検証**（§7-4）として併記。getter 側は idx0..71=72 で確定 |
| 3 | Icon キー **17件** vs 表Icon テーブル **33エントリ** | V49§3（Icon17） | V49§5.4（表Icon = 0xB12880 33件: [0] sHandToHandIcon、[1-] sAttributeIcon\* 等） | 同上。**差分16件（別キー形式 or NULL スロット）は未検証**（§7-4）として併記 |
| 4 | `0xB35574` の位置づけ: 「独立した中央レジストリ・グローバル。個別 vtable 未特定」 | V49§2（bss・ctor 呼出なしと推定） | V55§1（**GMST マネージャ `0xB35468` の埋め込みフィールド this+0x10C**。ctor `0x4A8330` → `lea edi,[this+0x10c]; call 0x4A7E60(401)`。マップ vtable = 0xA403A8/0xA403F0） | **V55 の確定を採用**。V49 未解決点1（vtable 未特定）は本件で**解消済み**。ただし文字列 hash/compare 関数の具体 VA は未特定のまま（§7-5） |
| 5 | 起動初期化: 「`0x40DF80` 内 **4インスタンス**順次初期化」（タスク指示の記述） | タスク指示 | V51§7-2（実測: **3インスタンス** = ①Reg `0x40E6C0` → ②INI `0x40E710` → ③Blend `0x40E759 call 0x53AC60`。第4 `0xB07AD8` は Begin/LoadAll/登録ゼロの死蔵） | **V51 実測を採用**（3インスタンス初期化）。死蔵 `0xB07AD8` は「第4実体=未使用ゴースト」として §5・§6 に併記 |
| 6 | 終了時: 「終了（**SaveAll**/破棄、atexit 系…）」（タスク指示） vs V51 の終了系証跡は **dtor thunk のみで終了時 SaveAll の呼出なし** | タスク指示（V50 由来の記述） | V51§4.5・§3.1（INI/Reg/死蔵 = `0xA1AA20/21/30/35`→`0x4A84E0`（node 連鎖 free）、Blend = `0xA1D241`。SaveAll 呼出は SaveIniFiles/起動自動書出/SavePath のみ） | 「**終了時 SaveAll は4資料内に未検証**」（§7-8）として明記。破棄経路は V51 実測を採用 |

（軽微な表記ゆれ: SetGS は V55§3「`0x514100` 系」、V55§5/タスク指示「`0x5141D4`(atof)」、fstp 位置「`0x5141DE`」— 同一経路の粒度差として §4 に併記。矛盾としない。）

---

## 4. 実装すべきモジュール分割案（EXE の構造に忠実）

### 4.0 モジュール関係（呼出順の全体像）

```
静的init: [Variable 登録]──▶ SettingsMap ◀──[表示名 prefix/Desc/Icon 登録]
                                  │ lookup
        ESM GMST ロード ──────────┤
        コンソール GetGS/SetGS ──┘
ゲーム処理: Variable 直接参照（マップ経由の読取不要）
UI:       表示名レンダラ ──getter(表A/B/C/Icon)──▶ SettingsMap 静的テーブル
永続化:   opcode(SaveIniFiles/RefreshINI/Get/SetINISetting) ──▶ ISettingsBackend(INI) ──▶ ファイル
```

### 4.1 モジュール (a) `SettingsMap` — 分離連結ハッシュ（key→variable\*）

| 項目 | EXE の事実（VA・値） | 移植入出力 |
|---|---|---|
| 実体 | `0xB35574`（= マネージャ `0xB35468+0x10C`、bss） | 静的オブジェクト1個 |
| レイアウト | +0x00 vtable / +0x04 bucket数 **401** / +0x08 bucket配列(`0xB3557C`) / +0x0C count / +0x10 重複許可フラグ | 同オフセット維持 |
| node | 12B malloc(0xC): `{next@0, key@4, value@8}` | 同 |
| ctor | `0x4A7E60(401)`（マネージャ ctor `0x4A8330` 内、静的 init `0x9E06B0`） | 起動時に明示構築 |
| lookup | `0x55E000(this, key, &out)` — hash→bucket→連結走査、命中で `*out=node[2]`、**書込なし** | `Variable* find(key)` |
| insert | `0x412D30(this, key, value)` — 先頭 prepend、`[this+0xC]++`。呼出元47件中 `0xB35574` 実体は setter 経路のみ（**runtime insert 0件**） | `insert(key, variable*)`（静的 init 専用） |
| remove | `0x4524C0(this, key)` — unlink → vtable[+0x10](value解放=no-op) → free node → count-- | 終了時 |
| 一意性警告 | setter 内で lookup 命中時: `push key; push 0xA32490; call 0x4A7A60`（severity 1）。書式: `Setting key '%s' already used in map.\nSetting keys must be unique.\n`。**既存を残し新規を挿入しない** | ログ出力＋挿入拒否 |
| 登録件数 | **2,304** = float setter `0x41BAE0` 992 + obj setter `0x419A70` 1,312（内訳は §1 図・§3 参照） | テーブル駆動で再現 |
| 同胞 | `0xB33508`（整数キー、ctor `0x418DB0`、vtable `0xA32C34`、hash=`key%37`@`0x6A9060`、compare=`==`@`0x763E80`、bucket 37） | 別インスタンス（用途は §7-3） |

### 4.2 モジュール (b) `Variable` — f/s/i 型別変数（既定値＋ESM 上書き＋SetGS）

| 項目 | EXE の事実 | 移植入出力・呼出順 |
|---|---|---|
| オブジェクト | 8B `{value@+0, key@+4}`。変数本体は bss（例 `0xB37B88` = fDifficultyDamageMultiplier、+0 に float が直接入る）。INI entry（.data 静的 8B）と**同一レイアウト** | `struct Variable { union {u32 u; float f; char* s;} value; const char* key; }` |
| float setter | `0x41BAE0(thiscall, ret8)`: `fstp [this]`(float値) → `[this+4]=key` → lookup 重複検査 → insert | 静的 init: `setDefault(key, 10.0f)` 例 = init `0x9ED780`(`fld [0xA31C80]`→setter) |
| obj setter | `0x419A70(thiscall, ret8)`: `[this]=value, [this+4]=key` → lookup 重複検査 → insert（prefix5・表A/C/Icon・s\* 系に使用） | 同上 |
| 型判別 | `0x4A78D0(key)`: 'f'→5(float) / 'i'→3(int) / 's'→6(string)（INI 型表と共用。INI 側の全表: b=0,c=1,f=5,h=2,i=3,r=7,u=4,a=8,s=6,その他=9） | 共通ユーティリティ |
| **ESM 上書き** | `0x44E326`(GMST ハンドラ、type table `0xB05E04` idx3 経由) → `0xB35468` slot9 `0x4A8040`: EDID を `0x55E000` で lookup（**insert なし・未知キー無視・map 不変**）→ 命中時 `0x4A7D60`: `0x44FEA0` で DATA へ前進 → 型別 `0x4510E0`(int/float 4B 直接書込) / `0x450C20`+`0x4A7980`(string set) → 末尾 `0x4A7CB0`(record ptr クリア) | `applyEsmGmst(edid, data)`: find → 命中なら value へ書き込み |
| コンソール SetGS | `0x514100` 系（float: `atof 0x985FB2`→`fstp [esi]`@`0x5141DE` / int: `atoi 0x983479`→`mov [esi],eax` / string: `0x4A7980`）— 同じく**変数本体へ直接書込・map 不変** | opcode 0x011C から接続 |
| 読取 | **変数直接参照**（例 `0x5E2560`: `fmul dword[0xb37B88]`）。ESM 優先分岐は存在しない | ゲーム処理は `variable.value` を直接読む |
| GetGS | `0x513FC0` 系: lookup + 読取のみ。書式 `"GameSetting %s >> %.2f"` (0xA4D740/0xA4D758/0xA4D770) | コンソール出力 |
| 終了時 | dtor thunk 例 `0xA1FE31`: `jmp 0x4055E0` → マップ `0x4524C0` から削除 + 文字列 free | 逆順破棄 |

### 4.3 モジュール (c) `ISettingsBackend` — 9スロット（INI/Reg の2具合・Blend 追加可）

vtable（EXE: `0xA404F8`=INI / `0xA40590`=Reg / `0xA566A4`=Blend / `0xA40548`=死蔵）:

| スロット | 共通/差分 | INI (`0xB07BF0`) | Reg (`0xB07D28`) | 役割 |
|---|---|---|---|---|
| +0x00 AddSetting | **共有** | `0x4040E0` | `0x4040E0` | stricmp 重複検査（重複時 fmt 0xA2FAB0 `"...already used in list..."` 警告して挿入拒否）→ prepend `0x446CB0` |
| +0x04 DestroyEntries | **共有** | `0x403C20`(→`0x65C620`) | 同 | +0x110 ノードチェーン破棄 |
| +0x08 Dump | 差分 | `0x4A8B10` | `0x4A8E50` | コンソール一覧（'INISettings'/'RegSettings'） |
| +0x0C WriteOne | 差分 | `0x4A85F0` | `0x4A8BC0` | INI: 型別 sprintf → `WritePrivateProfileStringA(sec,key,val, file=this+4)`。非セッション時は自動 Begin/End |
| +0x10 ReadOne | 差分 | `0x4A8800` | `0x4A8D20` | INI: 型別 `GetPrivateProfileIntA/StringA`（nSize=0x100）、既定値オブジェクト `0x404DF0`/`0x403BE0`/`0x403C00` |
| +0x14 Begin | 差分 | `0x4A83B0`（**flag 無視**、`[+0x108]=this`） | `0x4A8C90`（flag=1→`RegCreateKeyExA` / flag=0→`RegOpenKeyExA`, HKCU, subkey=this+4） | セッション開始 |
| +0x18 End | 差分 | `0x4A7CB0`（`[+0x108]=0`） | `0x4A8CF0`（`RegCloseKey`） | セッション終了 |
| +0x1C SaveAll | **共有** | `0x4A8560` | `0x4A8560` | `[+0x108]!=0` のときリスト走査し **backend 自身の vt+0x0C** を thiscall、結果 AND |
| +0x20 LoadAll | **共有** | `0x4A85A0` | `0x4A85A0` | 同上（vt+0x10 ReadOne） |

- オブジェクトレイアウト（全インスタンス共通）: `+0x00` vptr / `+0x04` target[0x104]（INI=ファイルフルパス、Reg=サブキー）/ `+0x108` session（INI=this or 0、Reg=HKEY or 0）/ `+0x10C` headEntry / `+0x110` headNode。
- エントリ: 8B `{value@0, name@4}`、name = `"key:Section"`（分割ヘルパ: `0x4A8430`=section（無コロン時 "MAIN"@0xA404C4）/ `0x4A8480`=key）。
- 構成: **共通4スロット + 差分5スロット**。Android では差分5のみ差し替えれば登録テーブル574件・opcode 経路は無改造（V51§8.6-1）。
- **具合**:
  - INI（`0xB07BF0`, 574件）— 必須。自前 key=value パーサで `Write/GetPrivateProfile*A` 相当を実現（読込: キー無し→既定値で不変 / 書出: upsert かつ**既存の未知セクション/キーは保持**）。
  - Reg（`0xB07D28`, sBasePath 1件のみ）— **起動 Triplet 以降は死にコード**（imm32 参照8件 = 起動6+登録1+dtor thunk1、FindSetting の Reg 版なし）。Android では no-op スタブ可（§6 未決定項番号参照）。
  - Blend（`0xB11C40`, 62件, `"BlendSettings.ini"`）— INI vtable 完全同一。追加可能だが保留可（未読込なら内部既定値で動作）。
  - 死蔵（`0xB07AD8`）— 削除可。
- 共通ユーティリティ: `0x4A8430`/`0x4A8480`(section/key 分割)、`0x4A78D0`(型判定)、`0x4A83F0`/`0x4A83C0`(u32↔4B pack)、`0x4040A0`(Open)、`0x40C180`(SavePath = Open+SaveAll+End)、`0x50EC30`(FindSetting)。
- opcode 接続（テーブル基底 `0xB0AF48` stride 0x28 532件、+0x18 impl、間接呼出のみ — 無改造流用可）:

| opid | 名前 | impl | 呼出順 |
|---|---|---|---|
| 0x011C | SetGameSetting | `0x514100` | lookup→変数本体へ書込（§4.2） |
| 0x011D | SetINISetting | `0x5143C0` | FindSetting→entry へ書込 |
| 0x011E | GetINISetting | `0x514230` | FindSetting→型別表示 `"INISetting %s >> %i/%.2f/'%s'"` |
| 0x013E | RefreshINI | `0x513BB0` | パス構築(`[0xB3F280]`+`"Oblivion.ini"`)→Begin(0)→LoadAll→End→メッセージ 0xA4D690（無条件出力） |
| 0x0165 | SaveIniFiles | `0x513B60` | パス走査→**Begin(1)→SaveAll→End**→常時 true |

### 4.4 モジュール (d) 表示名レンダラ（prefix+AV名）

| 項目 | EXE の事実 | 移植入出力 |
|---|---|---|
| prefix 表 | 5オブジェクト `{prefix@+0, key@+4}` @`0xB334C0..E0`（bss、stride 8）: Fortify(0xA3276C/`sMagicEffectItemFortify`)/Restore(0xA3278C/...Restore)/Drain(0xA327AC/...Drain)/Damage(0xA327CC/...Damage)/Absorb(0xA327EC/...Absorb)。スタブ `0x9D9EE0..0x9D9F6F`、破棄 thunk `0xA17510..0xA17550` | 固定5行テーブルで移植 |
| fourcc 分岐 | `0x413A90`: `0x413AFA` REAT→Restore / `0x413ABE` FOSK,FOAT→Fortify / `0x413AEF` ABSK,ABAT→Absorb / `0x413B72` DRSK,DRAT→Drain / `0x413B7A` DGAT→Damage / `0x413B39` default **137種**→prefix なし（ESM base name のみ） | 分岐表 |
| 書式 | `sprintf(buf, "%s %s", prefix, avName)` 書式 `0xA32564`、avName = `FUN_00565CC0(actorValue)` | `renderEffectName(fourcc, av)` |
| AV 名取得 | 表A getter `0x565CC0`: idx≤0x27 → `0xB12758[idx]`→Setting\*→value（bss 填充済）/ idx<0x48 → `0xB12868[idx]`（.rdata char\* 直返、32件 idx40..71 = "Magicka Multiplier".."Resist Water Damage"）/ 範囲外・NULL → 0 | `avName(idx)` |
| Desc/Icon 取得 | 表C `0x565CF0`（`0xB127F8` 33件）/ 表Icon `0x565D10`（`0xB12880` 33件）。UI は **Desc+Icon を必ず対で取得**（トークン 0xfaf/0xfb0/0xfb1/0xfb2/0xfb3/0xfb4 系） | `avDesc(idx)` / `avIcon(idx)` |
| UI 呼出4件 | `0x413BE0`(tooltip/name) / `0x41CBB0`(magic menu ACTIVE EFFECTS) / `0x593CD0`(alchemy) / `0x5B2B70`(enchanting) | 接続先 |
| 移植メモ | AV 名・Desc・Icon は**静的 fallback 英名を実データとして移植可**（ローカライズ key 併記）。表A/B 合計72件は `tmp\re\v48_av_names_initializer.md` を参照（本4資料外） | 静的表 |

---

## 5. 起動/終了シーケンス統合表（時系列1本）

| # | 時刻 | 処理（実測 VA） | 対象層 |
|---|---|---|---|
| 1 | CRT 静的初期化 | GMST 変数登録: float setter `0x41BAE0` 992件 + obj setter `0x419A70` 1,312件 → 登録簿 `0xB35574` へ insert（重複時 `0xA32490` 警告）。内訳: f\*978/s\*970/i\*227 + Desc45 + AV名36 + sMagicEffectItem\*17 + Icon17 + 未分類14 | 登録簿/変数 |
| 2 | CRT 静的初期化 | 表示名系: prefix5（`0x9D9EE0..`）、表A thunk `0x9F9660+0x20i`(40)、表C thunk `0x9F9B60+0x20i`(33) + 表Icon thunk → 全て `0x419A70` で登録簿へ | 表示名 |
| 3 | CRT 静的初期化 | INI entry 登録: CRT 帯 `0x9D7C00-0xA1F000` から `AddSetting 0x4040E0`（静的572件）。Reg: `0x9DAC71` push `0xB0472C`("sBasePath") 1件。Blend: `0x9E44xx-0x9E5Dxx` から 62件 | 永続化 |
| 4 | CRT 静的初期化 | マップ/マネージャ構築: GMST マネージャ ctor `0x4A8330`（静的 init `0x9E06B0`, push 401）→ 埋め込みマップ `0x4A7E60(401)` = `0xB35574` 構築、vtable `0xA4049C` へ書換。既定値例: `0x9ED780` が `fld [0xA31C80]`=10.0f → setter で `0xB37B88` へ | 登録簿/変数 |
| 5 | 起動部 `0x40DF80` 内（前半） | INI パス決定: ディレクトリ → bss `[0xB3F280]`（失敗時 `".\\"`）→ フルパス = dir+`"Oblivion.ini"`(`[0xB02C84]`)。ファイル無し → **`CopyFileA("Oblivion_default.ini" → フルパス)`** @`0x40E1C3` | 永続化 |
| 6 | 起動部（`0x40E6C0`-`0x40E759`） | **3インスタンス順次初期化**（V51 実測、§3-5）: ①Reg: subkey `'Software\Bethesda Softworks\Oblivion'` → Begin(0)=`RegOpenKeyExA` → LoadAll(sBasePath 1件) → End（`0x40E6C0`）②INI: Begin(0) → **LoadAll(574件)** → End（`0x40E710`）③Blend: `call 0x53AC60` = パス構築→Begin(0)→LoadAll(62)→End（`0x40E759`） | 永続化 |
| 7 | 起動部（`0x40F5A3`） | `call 0x53AD60`: BlendSettings へ `"DEFAULT"/"VERSION"="14"` スタンプ（再現不要、§6） | 永続化(Blend) |
| 8 | ESM ロード | GMST レコード: type table `0xB05E04` idx3 → ハンドラ `0x44E326` → EDID 読取 → `0xB35468` slot9 `0x4A8040` → **lookup のみ（insert なし）** → 命中時 `0x4A7D60` → `0x44FEA0`(DATA 前進) → `0x4510E0`(int/float 4B) / `0x450C20`+`0x4A7980`(string) で**変数本体へ直接書込**（例 fDifficultyDamageMultiplier 10.0f→5.0f）→ `0x4A7CB0`(record ptr クリア)。未知キーは無視・map 不変 | 変数 |
| 9 | 起動完了 `0x40F58E` | `push 0xb07bf4; mov ecx,0xb07bf0; call 0x40C180` = **Open(path,1)→Begin→SaveAll→End で Oblivion.ini を全設定で上書き再生成**（既知既定値がファイルへ吐き出される）— **再現可否は V53 判断待ち**（§6） | 永続化 |
| 10 | runtime | コンソール: SetGS `0x514100`系 / GetGS `0x513FC0`系（変数本体）、SetINISetting/GetINISetting（INI entry）、SaveIniFiles `0x513B60`/RefreshINI `0x513BB0` | 変数/永続化 |
| 11 | 終了（CRT dtor / atexit） | INI/Reg/死蔵: thunk `0xA1AA20/21/30/35` → `0x4A84E0`（vptr 段階変更 → `[+0x110]` ノード連鎖 free → `[+0x10C]=0`）。Blend: thunk `0xA1D241`。GMST 変数: 例 `0xA1FE31` → `0x4055E0`（マップ `0x4524C0` 削除 + 文字列 free）。prefix5: `0xA17510-0xA17550`。マネージャ本体: `0xA1AA00` → `0x4A8180`。String 12件: `atexit(0xA24B60)`→`FUN_0098124F`（**V50 確定・タスク指示由来**） | 全層 |
| 12 | 終了 | **SaveAll の呼出は4資料内に検出なし**（終了時 SaveAll は未検証、§7-8。実 EXE の書出経路は SaveIniFiles / 起動自動書出 / SavePath のみ） | — |

---

## 6. 移植チェックリスト（EXE の事実 / Android 実装での決定事項）

| # | EXE の事実（実測） | Android 実装での決定事項 | 状態 |
|---|---|---|---|
| 1 | Reg バックエンド（`0xB07D28`）は起動 Triplet の sBasePath 読出後 **死にコード**（起動後仮想呼出ゼロ、FindSetting の Reg 版なし） | **廃止可否**（no-op スタブ vs 完全削除）。V51 で死にコード判定済み・機能喪失なし | **未決定**（推奨: no-op スタブ） |
| 2 | 起動完了 `0x40F58E` が **Oblivion.ini を全量上書き再生成**（Open+SaveAll+End） | **再現可否 = V53 判断待ち**。再現時: 外部モッド互換↑、未知キーはマージ維持必須。非再現時: default.ini フォールバックも不要化し得る | **未決定（V53）** |
| 3 | ファイル欠損時 `CopyFileA("Oblivion_default.ini")` フォールバック（§5-5） | 起動自動書出（上記2）の決定に連動して採否判断 | **未決定（V53 連動）** |
| 4 | BlendSettings（`0xB11C40` 62件）は INI backend 再利用、VERSION=14 スタンプ(`0x53AD60`)あり | 62件の読込対応時期（保留可: 未読込なら内部既定値で動作）。VERSION=14 スタンプは **再現不要**（V51 推奨） | **未決定**（保留可）/ スタンプ=不要で確定 |
| 5 | 死蔵 `0xB07AD8`（Begin/登録ゼロ、参照は CRT dtor thunk 1件のみ） | 削除してよい | **削除で確定**（V51 判定） |
| 6 | ESM GMST 未知キーは lookup 失敗で**無視・map 不変** | 同一挙動を実装（原本互換） | **決定**（同一） |
| 7 | opcode 0x011C/0x011D/0x011E/0x013E/0x0165 + impl 関数 + vtable 呼出は無改造流用可（依存は FindSetting と g_INI のみ） | 無改造流用 | **決定**（流用） |
| 8 | INI WriteOne は upsert で**既存の未知セクション/キーを保持**（WritePrivateProfileStringA 挙動）、リトライなし・戻り無視 | 自前パーサで upsert + 未知キー保持を再現 | **決定**（保持） |
| 9 | 文字列バッファ上限 256B（ReadOne nSize=0x100）、section/key 64B | 同上限を維持 | **決定**（維持） |
| 10 | 型判定 `0x4A78D0`・String set `0x4A7980`・警告 `0x4A7A60` を INI/GMST/表示名が共用 | 共通ユーティリティとして単一実装 | **決定**（共通化） |
| 11 | AV 名/Desc/Icon は静的 fallback 英名（表A72件/表C33件/Icon33件、ローカライズ key 併記） | 静的表として移植 | **決定**（静的化） |
| 12 | prefix 5 + fourcc 分岐表 + `"%s %s"` 書式（UI 呼出4件） | 固定表で移植、UI4箇所へ接続 | **決定** |
| 13 | 起動時 INI LoadAll は Begin(0)→LoadAll→End、SaveIniFiles は Begin(1)→SaveAll→End（**opcode 側が Begin を先に呼ぶ契約**、SaveAll/LoadAll はセッション必須） | 契約をそのまま維持（再入 Begin の多重化に注意） | **決定**（契約維持） |
| 14 | 未分類14キー（float setter の push 抓取パターン外、f\*978+14=992） | key 一覧を取得後に表へ合流させるか個別実装 | **未決定（未検証: §7-6）** |
| 15 | 終了時 SaveAll の有無（§3-6） | 終了時書出の要否判断 | **未決定（未検証: §7-8）** |
| 16 | 登録簿の bucket 401 / node 12B / 先頭 prepend / 一意性警告文言（"in map"）と INI リスト警告（"in list"） | 構造・文言をそのまま再現（デバッグ互換） | **決定**（再現） |

未決定事項 合計: **6件**（#1, #2, #3, #4, #14, #15）。

---

## 7. 既知の未検証点（4資料に書かれていないこと・推測で埋めない）

1. **INI 574 キーと GMST 2,175 キーの命名一致（重なり）** — INI は "key:Section" 形式、GMST は editorID 形式で、FindSetting は INI 固定・lookup は登録簿固定のため経路上は交差しないが、**キー文字列そのものの重複有無は4資料内で未検証**。
2. **`0xB33508`（同胞・整数キー）と `0xB35574` の相互参照有無** — 両者は別マップとして記述されるのみで、相互 lookup/参照の有無は未検証。
3. **`0xB33508` が「MGEF fourcc マップ」であることの直接証拠** — V49 は「同胞インスタンス（整数キー、bucket 37、vtable 0xA32C34）」までしか実測していない。fourcc（u32 パック）を扱うという接続は4資料内で未検証。
4. **登録件数センサスとテーブル件数の差分** — `*Desc*` 45 vs 表C 33（差12）、AV名キー 36 vs thunk 40（差4）、Icon 17 vs 表Icon 33（差16）。差分キーの所在・またはカウント法の相違は未検証（§3-1/2/3）。
5. **中央マップの文字列 hash/比較関数の具体 VA** — V55 でマップ vtable = 0xA403A8/0xA403F0 まで判明したが、そのスロットに入る文字列 hash/compare の VA は未特定（同胞の整数版のみ 0x6A9060/0x763E80 で確定）。 → 確定 (2026-10-24, V63): vtable 帰属 = `0xA40454` 系（hash=`FUN_004A80A0`=djb2、compare=`0x584D10`=`__stricmp`）。
6. **未分類14キーの中身** — float setter 992件のうち push 抓取パターン外の14件。key 一覧は未取得（§6-14）。
7. **INI ランタイム登録2件（`0x404893` / `0x4440A3`=0x444080 内）の内容** — 静的572件と区別されているが、登録される entry の実体・キー名は未取得。 → 確定 (2026-10-24, V64): `fAlpha:Water` + `uSurfaceTextureSize:Water`。
8. **終了時の SaveAll 有無** — タスク指示は終了時に SaveAll に言及するが、V51 の終了系証跡は dtor thunk のみ（§3-6, §5-12）。
9. **BlendSettings 62エントリの全列挙** — サンプル `"fGetUpTime:HIT"`(entry `0xB11A24`) のみ実読（V51§9-6）。 → 確定 (2026-10-24, V64): entry range `0xB11A24..0xB11C13` 全62。
10. **ESM 未登録 GMST キー（セーブデータ経由等）を lookup 失敗で無視する場合のゲームプレイ影響** — 未評価（V55§8）。
11. **slot0 `0x412E20`（Add ラッパ）の vtable 経由 runtime 呼出元** — E8 直呼び0件。mod 追加 GMST を追加する経路は確認できず（実機検証が望ましい、V55§8）。 → 確定 (2026-10-24, V64): slot0 は定義済みで初回呼出しは `FUN_0044DCF0` 経由（ESM EDID 読込の slot+0x24 のみ）。
12. **Dump の PrintCtx 型・呼び出し元コマンド**（V51§9-2）、**`0x53AD60` 後半の制御フロー**（V51§9-1）、**opid→idx の解決手続き**（V51§9-7、V10/V13 台帳で対応済みとの記載だが本4資料では未検証）。 → 確定 (2026-10-24, V64): Dump = 正しい（呼出し元なし・到達不能コード）。
13. **表A part2（`0xB12868` idx40..71）以外の表B 起源の細部** — V49§7-2 は未確認だったが V41§3/§4 で「.rdata char\* 直返32件」と**解消済み**（本項は解消済みとして §4-4 に反映済み。矛盾ではなく情報更新）。
14. **タスク指示由来・4資料本文に無い事項** — 「atexit 系・String 12件の破棄は V50 確定」は本4資料の範囲では裏付け不可（V50 資料が別途必要）。§5-11 で出典を明記して転記した。

---

### 参照（本統合の入力）
- `tmp\re\v51_inisettings_spec.md`（全511行 実読）
- `tmp\re\v55_gmst_override_path.md`（全123行 実読）
- `tmp\re\v49_registry_type.md`（全261行 実読）
- `tmp\re\v41_effect_display_name_spec.md`（全124行 実読、§8=V48 訂正を含む）

*本ファイルは V57 の新規成果物。既存ドキュメントは無編集。*

## 7. 判断記録（2026-10-24 ユーザー判断）

- **V53 = 案 (a') 折衷を採用**: **初回のみ生成**（ini 不在時に `CopyFileA(Oblivion_default.ini)` 相当の既定ファイル生成を再現）＋ **手動保存（opcode `0x0165` SaveIniFiles、Begin(1)→SaveAll→End）は維持**、**起動毎の全量再書出 `0x40F58E` は省略**。読込 Begin(0)→LoadAll→End も従来どおり維持。
→ §6 の未決定事項「INI 自動書出の再現可否」「default.ini フォールバック」は本判断で**確定**（残り4項目: Reg 廃止可否 / Blend 62件 / 未分類14キー / 終了時 SaveAll は継続）。
