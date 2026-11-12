# V353: RaceSexMenu の page（user0）対応表とレース別/性別スライダー集合の裁定（#124）

- 日付: 2026-11-12 / タスク: ai-todo **#124**（v351 §1.3 の残件・main 起票） / 作成: RE レーン（opencode 直接解析）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）＋ 原本 XML `menus\chargen\race_sex_menu.xml`（BSA 抽出・705 行）
- 手法: capstone 5.0.7（v307_common.py）／template 名→C++ 生成コードの逆引き／trait 登録バンドの連続解析／初期化 thunk 帯からの文字列復元
- 中間証跡: `%TEMP%\opencode\v353_1..v353_18.py/.out`（**全 EXIT=0**）
- 制約: git / docs / GT / 台帳・session-memory / 既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM 未アクセス、L1

---

## §0 判定サマリ（#124 依頼3点＝v351 §1.3）

| # | 依頼 | 判定 |
|---|---|---|
| 1 | `user0`（page）の値 → ページ構成の対応表 | **[確定(命令)]**: `user0 = プロパティ 0xFAE(4014)`。page＝**pane の element id**（`race_template_pane` 生成 fn `0x5C4340` が `fild(element id) → set 0xFAE`）。表示条件は XML の `visible: me().user0 == RaceSexMenu.user0`。**pane は fn `0x5CA010`（RaceSexMenu 初期化）内で21個生成**され、各 pane にスライダー/BUTTON が対応（§3 表）。page 5/6/7 は RGB 系（§4） |
| 2 | レース/性別ごとのスライダー集合の決定規則 | **[確定(命令)] 2段階**: ①**静的（生成時）**= fn 0x5CA010 が pane ごとに固定のスライダー index 群を構築（§3 表・`race_template_slider` 生成 fn `0x5C93F0`）②**動的（状態更新）**= fn **`0x5C34D0`** が `g_thePlayer → vfunc+0x170() → [+0x1C8] → 0x51FE90（`byte[+0x48]` の **bit3** 判定）`に応じて **user13(0xFBB, enabled)=1.0 か user0(page)=5/6/7 へ切替**（§4）。制御対象はグローバル `'sRed'/'sGreen'/'sBlue'`（0xB38FC0/FC8/FD0） |
| 3 | スライダーの INI 名 → FaceGen 適用の対応の一部 | **[確定(命令)]**: **スライダー定義テーブル = `(表示ラベル, editor id "sXxx", テーブルスロット n)` の3点セット**（初期化 thunk 帯 0x9F6B10..0x9F74F0 から全抽出）。**chargen 系 n=0..123**（§2）。editor id `sXxx` が FaceGen INI のモーフキー（v351 §1.2 の 0xA62xxx-0xA63xxx と完全一致） |

---

## §1 機構の全体像 [確定]

```
[XML] race_sex_menu.xml
  race_template_pane(id=100): visible = (me().user0 == RaceSexMenu.user0)   ← page 切替の正体
  race_template_slider(id=103): user0=現在値 / user1=開始値 / user2=ラベル /
                                user6=マーカID / user7=editor id /
                                user8=geometry/texture / user10=変更 /
                                user13=enabled
  「<!-- template items added here by code -->」= 子要素は C++ が生成（XML に実体無し）
        │
        ▼
[EXE] fn 0x5CA010 = RaceSexMenu 初期化（race_sex_menu.xml 読込・v351 §1.1 と一致）
  ├─ pane builder   0x5C4340 (cdecl ret 0x14) : template "race_template_pane"
  │     └─ fild(element id) → set 0xFAE(user0)   ← page番号の付与
  ├─ slider builder 0x5C93F0                    : template "race_template_slider"
  │     └─ set 0xFA8/0xFAA(user?) / 0xFB0(user2=ラベル) / 0xFB1(user3) /
  │        0xFB4(user6) / 0xFB5(user7=editor id) / 0xFB6(user8)
  │        → [this+idx*4+0x94] へ element 登録（2系統 index）
  └─ button builder 0x5C4480                    : template "race_template_button"
        │
        ▼
[プロパティ] trait 登録テーブル（0x58AC40 帯・`push id; push name; call 0x58A0A0`）
        │
        ▼
[状態更新] fn 0x5C34D0（callers 3件: 0x5C4BD2/0x5C5312/0x5C7049）
      predicate 0x51FE90 = (byte[[player+0x1C8]+0x48] >> 3) & 1
```

### 1.1 trait → u16 プロパティ id 対応表 [確定]（v353_15）

| trait | id | trait | id |
|---|---|---|---|
| `string` | **0xFDE(4062)** | `user7` | **0xFB5(4021)** ← editor id |
| `target` | 0xFC9(4041) | `user8` | 0xFB6(4022) ← geometry/texture |
| `tile` | 0xFCF(4047) | `user9` | 0xFB7(4023) |
| **`user0`** | **0xFAE(4014)** ← **page** | `user10` | 0xFB8(4024) |
| `user1` | 0xFAF(4015) ← 開始値 | `user11` | 0xFB9(4025) |
| `user2` | 0xFB0(4016) ← ラベル | `user12` | 0xFBA(4026) |
| `user3` | 0xFB1(4017) | **`user13`** | **0xFBB(4027)** ← **enabled** |
| `user4` | 0xFB2(4018) | `user14`..`user20` | 0xFBC..0xFC2(4028..4034) |
| `user5` | 0xFB3(4019) ← Done/Back フラグ | `user6` | 0xFB4(4020) ← マーカ ID |

- **v352 の「0xFDE=表示文字列」「0xFAE float 比較」等はすべて本表で解読可能**（0xFAE=page・0xFB3=user5=Done/Back 等）。
- 登録 fn = `0x58A0A0`（XML パーサが trait 名を u16 id に化す。バンド 0x58AC40-）。

---

## §2 スライダー定義テーブル（n = 0..123）[確定(命令)]

初期化 thunk 帯 **0x9F6B10..0x9F74F0+**（各ブロック = `push 表示ラベル; push editor_id; mov ecx, 0xB39070+n*8`）。**0xB39070+n*8 = .bss の std::string グローバル配列**（8バイト間隔・thunk `mov ecx,slot; jmp 0x4055E0`＝dtor。直接読取不可＝書込は初期化 thunk のみ [確定負]）。

### 2.1 グループ見出し（slot n=-7..-1 ＝ 0xB39038..0xB39068）

`Nose` / `General(Skin)`(`sGeneralSkin`) / `sSkin` / `Beard` / `Eyebrows` / `Mouth(Lips)`(`sMouthLips`) / `Lips` / `Face brow/nose/chin ratio`(`sFacebrow`, n=0)

### 2.2 モーフスライダー n=0..84（抜粋・全量は v353_9/10）

| n | 表示ラベル | editor id | n | 表示ラベル | editor id |
|---:|---|---|---:|---|---|
| 0 | Face brow/nose/chin ratio | `sFacebrow` | 43..47 | Nose flat/pointed 他 | `sNoseflat` 等 |
| 1 | Face forehead/sellion/nose ratio | `sFaceforehead` | 48..53 | Nose sellion/short/tilt | `sNosesellion*` 等 |
| 2..4 | Face heavy/light・round/gaunt・thin/wide | `sFaceheavy` 等 | 54..56 | Skin flushed/shade/tint | `sSkinflushed` 等 |
| 5..7 | Brow Ridge high/low・Inner・Outer | `sBrowRidge*` | 57..58 | Skin tint orange/purple | `sSkintint*` |
| 8..12 | Cheekbones low/shallow/thin・Cheeks concave/round | `sCheekbones*` 等 | 59 | Cheek blush light/red | `sCheekblush` |
| 13..19 | Chin forward/pronounced/retracted/shallow/small/tall/wide | `sChin*` | 60..65 | Beard flushed/light/cheeks/circle/goatee/moustache | `sBeard*` |
| 20..23 | Eyes down/small/tilt/together | `sEyes*` | 66..67 | Eye sockets bruised/dark | `sEyesockets*` |
| 24..26 | Forehead small/tall/tilt | `sForehead*` | 68..71 | Eyelids light/pale・Eyeliner・Eyeshadow | `sEyelids*` 等 |
| 27..31 | Jaw retracted/wide・Jaw-Neck・Jawline・Mouth drawn | `sJaw*` 等 | 72..79 | Eyes dark/whites・Eyebrows dark/low/thick/very/lower/outer/upper | `sEyesdark` 等 |
| 32..42 | Mouth happy/high/lips*/protruding/tilt/underbite・Mouth-Chin・Nose bridge | `sMouth*` 等 | 80..84 | Lips flushed・Lipstick×2・Nasolabial・Nares | `sLipsflushed` 等 |

### 2.3 テクスチャ/色 n=85..119

| n | 内容 |
|---:|---|
| 85/86 | **Eye shades(`sEyesTex`) / Nose shades(`sNoseTex`)** |
| 87 | **Hair Color(`sColor`)** |
| 88..103 | **`sHairColor0`..`sHairColor15`**（髪色16） |
| 104..119 | **`iHairColor00`..`iHairColor15`**（髪色インデクス16） |

### 2.4 メッセージ/操作 n=120..123

`'You must first enter a valid name.'`(`sNoNameCharacter`) / `'Enter character name.'`(`sEnterName`) / `'Reset face?'`(`sResetFace`) / `'Randomize face?'`(`sRandomizeFace`)

- **n=120/121 は v352 §5 の name 検証バンド（0x5CC200）が参照するメッセージと同一** [確定]（両タスクの接続点）。
- n=124+ は Journal 系（`sJournalTitle*`）＝**この thunk 帯はゲーム全体の文字列登録テーブル**で、chargen 系は n=0..123。

---

## §3 page 対応表（pane × スライダー）[確定(命令)]

fn `0x5CA010` 内で **PANE 21個**が順次生成され、各直後にその page の SLIDER/BUTTON 群が構築される（v353_4/5 の call サイト機械抽出）。

| # | PANE call | 後続スライダー index（arg3） | 後続 BUTTON |
|---:|---|---|---|
| 1 | 0x5CA448 | 0, 0（2本） | 3 |
| 2 | 0x5CA5A4 | — | — |
| 3 | 0x5CA62D | 0×4 | — |
| 4 | 0x5CA726 | — | 4 |
| 5 | 0x5CA85B | — | 9 |
| 6 | 0x5CAAE4 | — | 6 |
| 7 | 0x5CACA1 | 0x13,0x14,0x15,0x16,0x17（19..23） | — |
| 8 | 0x5CAD84 | 1, 2 | — |
| 9 | 0x5CAE1B | 4,5,6,7 | — |
| 10 | 0x5CAEFE | 9,0xA..0xE | — |
| 11 | 0x5CB02D | 0x10,0x11,0x12 | — |
| 12 | 0x5CB0EA | 0x19,0x1A | — |
| 13 | 0x5CB181 | 0x1C,0x1D,0x1E | — |
| 14 | 0x5CB23E | 0x20..0x28（9本） | — |
| 15 | 0x5CB3DF | 0x2A..0x36（13本） | — |
| 16 | 0x5CB618 | **group=1**: 0x1D,0x1E,0x1F,6 | — |
| 17 | 0x5CB6FB | **group=1**: 0,1,2,3,4,5 | — |
| 18 | 0x5CB819 | **group=1**: 7,8,0x10..0x13 | — |
| 19 | 0x5CB937 | **group=1**: 9,0xA..0xF | — |
| 20 | 0x5CBA7E | **group=1**: 0x16,0x17,0x18 | — |
| 21 | 0x5CBB21 | **group=1**: 0x19.. | — |

- **page番号 = pane の element id**（`0x5C4340` 内: arg3 が -1 なら `[0xB3B5D4]`（自動採番カウンタ）から採番、`fild → set 0xFAE`）[確定]。**RaceSexMenu.user0 の初期値は XML `<user0> 0 </user0>`＝0**（初期表示＝page0）[確定]。
- **SLIDER 生成の引数形**（cdecl・`this=esi`）: `push 0(arg5) / push 0|1(arg4) / push idx(arg3) / string(arg2=ラベル経路) / push [this+0x58](arg1)`。**arg4=1 の系統（pane 16-21）は 0x5C93F0 内で `arg4!=-1` 時の追加処理 `call 0x5C6860`（g_thePlayer→vfunc+0x170 由来の初期値計算）が入る** [部分確定]（group=1 の具体的意味は残点 V-353-3）。
- **element 登録**: `[this + idx*4 + 0x94]`（2系統 index＝v352 §2 の SetElement と同じ配列）。

---

## §4 動的制御（レース/性別のスライダー集合）[確定(命令)]

### 4.1 predicate `0x51FE90`

```asm
movzx eax, byte ptr [ecx + 0x48]
shr   eax, 3
and   al, 1
ret                       ; = (byte[obj+0x48] >> 3) & 1
```
- 入力 obj = **`g_thePlayer → vfunc+0x170() → [+0x1C8]`**（v352 §4.2 の [player+0x1C] とは別フィールドの子オブジェクト）[確定]。

### 4.2 制御ハンドラ `0x5C34D0`（callers 3件: 0x5C4BD2 / 0x5C5312 / 0x5C7049）

```
g_thePlayer → vfunc+0x170() → [+0x1C8] → call 0x51FE90 (bit3)
  ├─ true  → 'sRed'/'sGreen'/'sBlue' element（0x5C3440 経由）へ:
  │            set user13(0xFBB) = 1.0        ← enabled ON
  │            set user6(0xFB4) / 連番 1,2,3,4 / 0xB38F88 系文字列
  └─ false → 各 RGB element の user0(0xFAE) = 5, 6, 7 … ← page 切替
              （get float で現在値も取得）
```
- **制御対象グローバル = `[0xB38FC0]='sRed' / [0xB38FC8]='sGreen' / [0xB38FD0]='sBlue'`** [確定]（初期化 thunk `0x9F693A/95A/97A: mov ecx,<g>; push 0xA629F8/0xA62A04/0xA62A14; call 0x419A70`）。
- **element 取得 `0x5C3440`** = `(key文字列) → call 0x5C3110`（**key→members 対応表**・v352 §5 の表）→ element。
- → **「page 5/6/7 は RGB 系ページ、+0x48 bit3 で enabled/page の二経路を切替える」** [確定(命令)]。
- 0x5C34D0 以外の user13 set（20件中 RaceSexMenu 帯7件）はすべて本 fn 内。帯外（0x599xxx/0x5AAxxx 等）は他メニュー系（class/skills 等）[部分確定]。

### 4.3 page 切替の受け手

- RaceSexMenu 主ハンドラ **0x5CC200 内に user0(0xFAE) 関連 set/get が4件**（`0x5C3110` 経由・0x5CC857/0x5CC9AD/0x5CC9F2/0x5CCA42）＋ 初期化 fn 内0x5CBB96/0x5CBBAE [確定]。
- **RaceSexMenu.user0 と pane.user0 が一致した pane のみ表示**（XML L251-252）→ **ボタン/スライダー操作が user0 を書き換えると page が切り替わる** [確定(命令)]。

---

## §5 依頼3（INI 名 → FaceGen 適用）への接続 [部分確定]

- **editor id（`sXxx`）が FaceGen INI のモーフキー**（§2 の `sFacebrow`/`sNosesellion*` 等は v351 §1.2 の INI 名・0xA62xxx-0xA63xxx と完全一致）[確定]。
- **user7(0xFB5) = スライド対象の editor id**（XML L582 コメント原文）＝ **スライダー→INI キーの対応は user7 に格納される** [確定]。
- user8(0xFB6) = geometry/texture フラグ（XML L583）→ n=85..119 のテクスチャ/色系スライダーが使用 [確定(命令)・適用細部は残点]。

---

## §6 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v353_1..v353_18 **全 EXIT=0** |
| 2 | template 名→生成コード | `race_template_pane/slider/button` の raw xref が各1件（0x5C4371/0x5C9437/0x5C44B9）→ 3 builder を確定。XML の「added here by code」コメントと一致 |
| 3 | trait id 表の整合 | user0=0xFAE が pane builder の set・user7=0xFB5 が slider builder の set・user13=0xFBB が制御 fn の set と**3経路で一致** |
| 4 | スライダー n テーブルの整合 | n=0..123 のラベル/editor id が v351 §1.2（s77 §1.5 の INI 名）と完全一致／n=120/121 が v352 の name 検証メッセージと一致 |
| 5 | page 表の網羅性 | PANE call 21件を fn 0x5CA010 の E8 全走査で機械抽出（BUTTON37/SLIDER 多数と同一ループ） |
| 6 | predicate の実体 | 0x51FE90 全文（4命令）で bit3 判定を確定。0x5C34D0 内の唯一の外部判定 |
| 7 | RGB グローバルの初期化 | 0xB38FC0/FC8/FD0 の thunk（0x9F693A 等）と文字列リテラル（0xA629F8='sRed' 等）を双方向で照合 |
| 8 | 負の証拠 | 0xB39070+n*8 への書込は .text に存在せず（xref 3件=読/thunk/データプールのみ）＝**ラベルは初期化 thunk 経由のみ** [確定負] |

---

## §7 新規残点（次束用）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-353-1 | [部分確定] predicate の `byte[obj+0x48]` **bit3 の意味**（性別/レース/編集ロックのどれか）。[player+0x1C8] のクラス同定が未了 | **中** |
| V-353-2 | [未確定] pane の **element id（=page番号）の実値**（[0xB3B5D4] 自動採番の初期値と PANE 21個の採番順・実行時依存）。XML 側に pane instance が無いため静的に確定不可 | 中 |
| V-353-3 | [部分確定] SLIDER arg4=1 系統（pane 16-21）の `0x5C6860`（g_thePlayer+0x170 由来の初期値計算）の役割と、group=0/1 が**レース系統か性別系統か**の切り分け | 中 |
| V-353-4 | [未確定] pane ごとのスライダー index → §2 テーブル n の**完全対応**（arg3 の idx と n のオフセット関係。idx は 0x13-0x36 と 0-0x1F の2系統で n への写像が未確定） | **中** |
| V-353-5 | [部分確定] 0x5C6EA0/0x5C4920/0x5C50A0（0x5C34D0 の callers3件）の呼び出し文脈（レース選択/性別切替/初期化のどれか） | 低 |
| V-353-6 | [未確定] 0xFA8/0xFAA/0xFA5 など **trait 表（user0-20/string/target/tile）に無い id の trait 名**（XML の他 trait: id/name/listindex 等との対応） | 低 |

---

## §8 main への報告（v351 §1.3 の更新材料）

1. **§1.3-1（page 対応表）**: `user0=0xFAE`・page＝pane の element id・**pane 21個が fn 0x5CA010 で生成**・page 切替は user0 書換（§3 表）。**v351 の「Race ページ/Face ページ群の内訳」は本表（PANE 21個×スライダー idx グループ）で置換可能**。
2. **§1.3-2（レース/性別スライダー集合）**: **2段階決定**（生成時の固定集合＋`0x5C34D0` の bit3 判定による user13/user0 更新）。RGB 系（sRed/sGreen/sBlue）が具体例 [確定]。
3. **§1.3-3（INI 対応）**: **スライダー定義テーブル n=0..123（ラベル＋editor id）を全抽出**（§2）。user7=editor id 格納 [確定]。**port の Appearance 実装（user0 対応表・スライダー集合）は §2/§3 のデータで照合可能**。
4. **#96/#101 への副次材料**: page 切替の正体（user0 書換）・enabled=user13・**name メッェージ n=120/121 が v352 と接続**。

---

### 参照（本タスクの証跡）

- `v353_1/2`（trait/template 文字列 xref・3 builder 本体・trait 登録バンド初出）
- `v353_3/4/5`（builder の callers 機械抽出・call 直前 push 列・page×slider 表の初版）
- `v353_6/7/8`（引数復元の失敗系・テーブル xref3件走査・.bss 判定と thunk/データプール）
- **`v353_9/10`（スライダー定義テーブル n=0..127 全抽出＝本裁定の核心データ）**
- `v353_11/12/13/14`（user0 set 153件・trait 表パーサ修正の経緯・RaceSexMenu 帯 set id 6種）
- **`v353_15`（trait id 表 user0..user20/string/target/tile の確定・user13=0xFBB）**
- **`v353_16/17`（制御 fn 0x5C34D0 全文・predicate 0x51FE90・0x5C3440・RGB グローバル xref）**
- **`v353_18`（'sRed'/'sGreen'/'sBlue' の確定＝最後の決定打）**
