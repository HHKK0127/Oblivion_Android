# V177: GT slot 語義残り（最終束）— V-163a..e ＋ v121 V147 残りの裁定

作成: 2026-10-28（opencode サブエージェント V177 / **分析専任**）
対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / SHA256 `A8F31384…C2B7DC6`）
手法: capstone 5.0.7（`skipdata=True`, `md.detail=True`）、ImageBase `0x400000`、VA = `0x400C00` + file_off（.text）/ `0x400E00`（.rdata）/ `0x401400`（.data）
入力（読取のみ）: `tmp/re/v163_gt_slot_semantics.md`（§7 残点 V-163a..e）、`v121_gt_corrections.md`（§5 V147＋§5.1 V153 追記）、`v171_gt_candidates.md`（V158/V171 確定分・整合確認）、`b27_slot_residuals.md`、`v158_slot_opcode_map.md`（extra 書手）、`GT_CHEATSHEET.md`（§4.1 現行）、`cache/vtable_index.tsv`、`docs/RE_EXE_REPORT.md`（§59.3 マスク表ほか読取のみ）
状態: **分析完了・編集なし**（`docs/`・`GT_CHEATSHEET.md`・TODO・session-memory・plan・cache・既存 `tmp/re/*.md` はバイト1つも変更していない。新規は本 1 ファイル＋`%TEMP%\opencode\v177_*.py/.txt` のみ）
着手時 selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**

> 本稿は「訂正前（現行 GT/ハンドオーバ記述）→ 訂正後（反映可能なドラフト）」を示す。GT 本体の編集は行わない。

---

## §0 判定サマリ（6 項目）

| # | 残点（出典） | 判定 | ドラフト |
|---|---|---|---|
| 1 | V-163a: FormIDToString `0x47E190` の契約（formType byte vs FormID 警告文の乖離） | **解消: 乖離は不存在** — 名称中の "FormID" は **TESForm の formType enum 型名**（TESForm.cpp の `cFormID` / `formEnumString` 表）。実引数 formType byte は enum 値そのもの。`"pActor->iFormID = %08X"` は**別物**（actor 複製系の debug 出力＝真の formID `+0xC` を %08X 表示。FormIDToString 呼出しを含まない） | §1 |
| 2 | V-163b: EDL extra `0x14` の用途名 | **確定: `ExtraStartingPosition`（RTTI 実測・class 名訂正あり）＝配置位置（destCell+位置+回転）の保存**。type `0x14` ノード（vtable `0xA35A70` = `.?AVExtraStartingPosition@@`、ctor `0x42AC40`・size 0x24: `+8`=destCell 系 ptr（ctor 0）、`+0xC..+0x14`=位置3float、`+0x18..+0x20`=回転3float）。writer 直接 **2 経路**（ロード時配置解決 `FUN_0045C4F0`／slot+0xF8 setter `0x4DD1A0`。実行時 `FUN_004DF820/004E2160` は vt+0xF8 経由で合流）、reader = slot+0xF0/+0xF4（`0x4D8210/0x4D8230`）＋presence 検査 **7 サイト**。**v163 §5.1 の「ExtraTeleport」表記は誤り** — `.?AVExtraTeleport@@`（`0xA357DC`）は **type 0x32・単一ポインタ payload** の別 extra（get-or-create `FUN_004234d0`） | §2 |
| 3 | V-163c: `0x2D0` 一般名の最終確定 | **確定: 「アイテム使用アクション起動 `StartItemAction`」** — イベント mask4 = **`OnDrop`（0x4）** を GT §4 マスク表×report §59.3 で実測照合（b16/V153 と 3 系統一致。0x5E4440 経路は container=0 で emit スキップ）、`FUN_00699190` = **埋め込み発動系コントローラ（this+0x5C）への SpellItem 供給**（`item[+0x64]` オブジェクトの `+0x18` を `vt+0x18()`(1/4) 分岐＋`MagicItem(0xB03280)→SpellItem(0xB03800)` dynamic_cast で vt+4/vt+0 へ供給、controller[2] に状態 5/6 書込）＝ enchantment 系 [確定寄り]、`FUN_0048FBC0` = **コンテナ内容物一括処理**（11引数・`param_1[2]=-1.0f`(0xA30634) センチネル・`vt+0x40(0x8000000)` 通知・`*param_1` リスト走査で `[elem+8]==param_3` マッチ）。b27 §4 ファミリ整合の正式名称案（Actor 版 StartItemAction / process 版 GetActionState）を提示 | §3 |
| 4 | V-163d: `[process+0x11D]` {0,1,4,6,9} と `+0x11C` {0,1,3,5,6} の意味 | **大幅縮小（残差あり）**: `+0x11D` = **家具（furniture）/idle 使用状態 byte** — `FUN_00650030`（vt+0x1B4 impl）が switch（cases 2/7・3/8…）で `vt+0x370` へ状態コード 0..10 を渡す状態機械、文脈文字列 `"Missing furniture dynamic idle for marker %d…"`、`+0x136`=marker index。値は **(n, n+5) ペア**（1/6・2/7・3/8・4/9、`FUN_00654f10` が `+0x11D=(cond?9:4)` に切替）。ゲート {0,4,9} = 「未使用(0) or 使用中(4/9) のみ update 続行」＝**家具使用中は通常 AI 更新保留** [確定寄り]。`+0x11C` = **Knocked/転倒系状態 byte**（slot+0x2E4 process 版 `0x64B080`、値 {0,1,3,5,6}、b10 GetKnockedState 整合）[有力]。隣接 byte だが**別枚挙** [確定] | §4 |
| 5 | V-163e: `FUN_004133C0` の `base[+0x58]` bit7 ガード／`HighProcess+0xF0` の正体 | **確定（構造）＋残差は名前のみ**: `FUN_004133C0` = **base フォーム bit7 ガード付き formID getter**（`base=[this+0x1C]`、`[base+0x58]` bit7 立ちなら **0**、否则 `[this+0xC]`。直接呼出し **9**＝v163 の 8 は skipdata 漏れの可能性）。対 setter `FUN_004133E0`（同ガード+arg>=0 → `[this+0xC]=arg`・`[this+0x20]=-1.0f`・戻り 1）。`base+0x58` は **複数 bit の flags dword**（bit7=本ガード/bit22=`FUN_00415180`/bit31=`FUN_004AE5A0`）[確定]、bit7 名は [要深掘]。`HighProcess+0xF0` = **+0xE4 ファミリ第 4 リスト先頭**（兄弟ゲッタ `0x64B240/270/2A0/2D0` → +0xE4/+0xE8/+0xEC/+0xF0、flag 引数付き条件取得）。元素型は **v171 残点2 に継承** | §5 |
| 6 | v121 V147 残り（0x188/0xF8/0x250 等との差分のみ） | **整理完了**: V153 で解消済み（0xF8=ExtraTeleport セット/0x188=IsMobile＋時間積算/0x2E4=delta 更新＋[0x11C] getter/0x250・0x254=v158 で犯罪メーター）を除く**実質残点は「GT/REPORT への反映ドラフト化」**（A–D 裁定は V171 で確定済み・GT §4.1.1 は「裁定待ち」のまま）。＋ b14/b27 側の訂正候補 6 件を列挙 | §6 |

---

## §1 V-163a: FormIDToString `0x47E190` の契約 — **乖離は不存在**（"FormID" = formType enum 型名）

### 1.1 裁定

**訂正前**（v163 §2.4 の残契約）
> 実引数は formType byte なのに警告文は FormIDToString — 「FormID→文字列」元の命名と実装の乖離（または formID の下位 byte を型として扱う経路）の確定 [要深掘]

**V177 訂正後（ドラフト）**
> **乖離は不存在**。`0x47E190` の実引数は `byte[form+4]`（formType）であり、これは **TESForm の formType enum 型**（TESForm.cpp で `FormID` と呼ばれる enum / 検証文字列 `"formEnumString[ %d ].cFormID in TESForm.cpp is out of order."`(0xA3BFD4) の `cFormID` メンバ）の**値そのもの**。名称 "FormIDToString" の "FormID" は **フォーム識別子（`TESForm+0xC` の u32）ではなく formType enum の型名**。警告文 `"Invalid FormID sent to FormIDToString().\r\n"`(0xA3D5C0) も同義（enum 範囲外 byte に対する警告）。真の formID（+0xC）を扱うのは別経路（下記 1.3）。

### 1.2 実装の全体像（本セッション実測・v163 §2.4 を補完）

**(a) エントリ `0x47E190`（未登録領域・int3 パディング後の独立エントリ）**

```
0x47E190: mov eax,[esp+4] / push eax / call 0x47d640 / add esp,4
          test eax,eax / jl 0x47e1a9            ; idx < 0 → 旧 enum 経路
          mov eax,[eax*4 + 0xb081d0] / ret      ; 新型名テーブル[idx]
0x47E1A9: mov cl,[esp+4] / movzx eax,cl / lea edx,[eax-3] / cmp edx,0x3a / ja 0x47e24e
          movzx edx,byte[edx + 0x47e2d8] / jmp [edx*4 + 0x47e27c]   ; 旧名 jumptable（22 名）
          … 0x47E1CA..0x47E24D = mov eax,[0xB06870..0xB068C4]; ret ×22
0x47E24E: cmp cl,0x45 / jb 0x47e26d
          lea eax,[eax+eax*2] / mov ecx,[eax*4 + 0xb05e04] / push ecx / push 0xa3d5c0
          call 0x4a7a60 / add esp,8 / xor cl,cl  ; ★警告印字＋cl=0 へクリア
0x47E26D: movzx eax,cl / lea edx,[eax+eax*2] / mov eax,[edx*4 + 0xb05e04] / ret
```

**(b) 分類器 `FUN_0047d640`（byte → 新型名 index）**

```
0x47D640: movzx eax,byte[esp+4] / add eax,-0xa / cmp eax,0x39 / ja 0x47d734
          movzx eax,byte[eax + 0x47d7cc] / jmp [eax*4 + 0x47d738]   ; code map 0x3A byte
0x47D734: or eax,-1 / ret                  ; 範囲外(0x0A..0x43 でない byte) → -1
各 target: mov eax, N; ret（N = 0..0x23）or xor eax,eax; ret（0）or or eax,-1; ret
```

- **戻り値集合 = {-1} ∪ {0..0x23}**（36 index）。byte 0x0A..0x43 が対象、範囲外は -1。
- code map 実測: `00 24 24 24 01 02 03 24 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 24×13 1e 1f 20 21 22 23`（`0x24` = -1 系）。

**(c) テーブル 3 系統（実測）**

| 表 | 内容 | 用途 |
|---|---|---|
| `0xB081D0` | 新型名 ptr 配列（`[0]="Activator", [1]="Apparatus", [2]="Armor", [3]="Book", [4]="Clothing", [5]="Container", [6]="Door", [7]="Ingredient", [8]="Light", [9]="MiscItem", [10]="Static", [11]="Grass" …`） | `idx>=0` 経路の戻り値 |
| `0xB06870..0xB068C4` | **22 名の旧型名キャッシュ**: "Race", "Class", "Birthsign", "Script", "Dialogue Topic", "Cell", "Faction", "Sound", "Global", "Region", "Game Setting", "Magic Effect", "World Space", "Climate", "Weather", "Skill", "Quest", "Hair", "Eyes", "Reference", "Idle Animation", "AI Package" | `cl-3<=0x3A` の jumptable 経路 |
| `0xB05E04` | 12B/エントリ `{型名 ptr, 0, type_id}`（実測例: entry0x0C={"MGEF",0,13} / 0x0D={"SCPT",0,14} / 0x0D+={"LTEX",…}）＝ **formEnumString 表**（`"formEnumString[ %d ].cFormID in TESForm.cpp is out of order."`0xA3BFD4 の参照先 `FUN_0046C460` が整合検証） | 最終フォールバック（`cl>=0x45` で警告→cl=0→entry0） |

### 1.3 呼出し 4 件（すべて `FUN_0044cdf0` 内）＋ `"pActor->iFormID = %08X"` の意味

- **直接 `call 0x47E190`（raw E8 全走査）= 4 件**: `0x44D189` / `0x44D237` / `0x44D28F` / `0x44D2F8` — すべて `FUN_0044cdf0` 内。
- `FUN_0044cdf0` = **テクスチャ/アイコン欠落検証＋警告印字**。各 site の文脈（実測）:
  ```
  movzx edi,byte[esi+4]        ; formType
  mov eax,[esi] / mov edx,[eax+0xd4] / mov ecx,esi / call edx   ; form->vt+0xD4() = フォーム名
  push eax / push edi / call 0x47e190 / add esp,4   ; 型名 = FormIDToString(formType)
  push eax / push fmt / call 0x4a7a60               ; sprintf/trace
  ```
  fmt = `"Texture missing for %s '%s'."`(0xA383D4) / `"Texture '%s' missing for %s '%s'."`(0xA383A0) / `"Icon missing for %s '%s'."`(0xA38384) / `"Icon '%s' missing for %s '%s'."`(0xA38364)。
- **`"pActor->iFormID = %08X"`(0xA65068) の参照 = 1 件のみ `0x55DCEC`**（台帳上 FUN_0055d8c0 範囲だが**台帳結合**の可能性大）:
  ```
  0x55DCD1: cmp dword[ebp+0x114],0 / je …
  0x55DCDA: push 0x20 / call 0x401f00        ; malloc(0x20)
  0x55DCE1: mov edx,[ebp+0x114] / mov ecx,[edx+0xc]   ; ★ [obj+0xC] = formID
  0x55DCEA: push ecx / push 0xa65068 / push eax / call 0x98208b  ; sprintf("pActor->iFormID = %08X", formID)
  ```
  近傍 `0x55DC05` にも別 debug 文字列 `0xA650A4`＋同一 sprintf（`0x98208b`）→ **actor 複製/検証系の debug トレース**（`FUN_0055d8c0` は `malloc(0x118)` → ctor `FUN_0055cd70` → `FUN_0055cfe0` の actor 複製ヘルパ）。表示値は **`[actor 関連オブジェクト+0xC]`（真の formID フィールド）** であり、FormIDToString（formType→型名）とは**別概念・別経路**。
- ⇒ 「`pActor->iFormID = %08X` 参照」は **formID=+0xC の第 5 の独立証拠**（v163 §2.2 の 4 系統に追加）であり、FormIDToString の契約には無関係。

### 1.4 GT/docs 影響

- GT §4.1 GetFormID 注記（L130「文字列化は未登録関数 `0x47E190`」）は**現状維持で正确**。追記するなら 1 行: 「FormIDToString の "FormID" は formType enum（TESForm::FormID）を指す（+0xC の formID フィールドではない）。警告は enum 範囲外 byte 発生時」。
- report 側に該当記述なし（訂正不要）。

---

> **重複統合の記録（2026-10-28 · V177-r）**: 本ファイルはタスク開始時は §0/§1 のみ（100 行）だったが、並行実行された V177 元セッション（中断復帰）と本継続セッション（V177-r）の双方が §2・§8 を追記した。**直後にあった「§2 …（テレポートではない）」〜「§8.2」ブロックは継続版（重複）**、**以下の「§2 …＝配置位置の保存」〜「§8.3」ブロックが元セッション版（正本）**。継続版の固有証拠はすべて正本へ統合済み — ENAM/SCRI/DATA FourCC 実測（`FUN_004B5440`）→ §3.2/§5.2 追補行、`FUN_006564C0` の 1/6 生成文脈 → §4.2 追補行、`+0x11C` byte writer 全走査（初期化 1 件・非保存）→ §4.2 追補行、`FUN_00484E80` 本体（連結リスト走査）→ §5.2 追補行、`FUN_0048FBC0` 容量検証 → §3.2 追補行、b27/v121 訂正候補⑦⑧ → §7。重複だった継続版ブロックのみを撤去し、**§0/§1 はバイト無改変**。**以降の §2・§8 が正本**。

## §2 V-163b: EDL extra `0x14` — **`ExtraStartingPosition`（RTTI 確定）＝配置位置の保存**

### 2.1 裁定

**訂正前**（v163 §5.1 の [有力]）
> extra 0x14 = ExtraTeleport（destCell=+0x8 / 位置=+0xC / 回転=+0x18）の位置・回転セットヘルパ [有力]。用途は「配置/保存位置の記録」[有力]、正式名 [要深掘]。

**V177 訂正後（ドラフト）**
> **class 名を訂正して確定**: type `0x14` ノードの vtable `0xA35A70` の RTTI は **`.?AVExtraStartingPosition@@`**（`cache/vtable_index.tsv` #716 と実測一致）。用途は v163 の推定どおり**配置位置（pos+rot）の保存**で確定 — (a) ESM ロード時の配置解決 `FUN_0045C4F0` が新規参照の現在 pos/rot を extra へ書き込む（スナップショット）、(b) teleport 適用 `FUN_0045BB30` が extra の pos/rot を参照へ反映して親セル解決、(c) ロード時破損検出 `FUN_0045E990`（`"Corrupt location found loading reference %08X, fixing it."`）が不在時に修復、(d) slot+0xF8 setter `0x4DD1A0` が更新。**`.?AVExtraTeleport@@`（vtable `0xA357DC`）は別 extra（type `0x32`・単一ポインタ payload `+0xC`）**であり v163 の名前当てはめは誤り。

### 2.2 実測（本セッション）

| 証拠 | 内容 |
|---|---|
| RTTI | vt `0xA35A70` → col `0xAB5FA4` → td `0xB03E34` → **`.?AVExtraStartingPosition@@`**（vtable_index #716 と 2/2 一致）。対照: vt `0xA357DC` = `.?AVExtraTeleport@@`（#718）、`0xA34D50` = `.?AVBSExtraData@@` |
| ctor（type 0x14） | factory `FUN_0041EA70`（`push 0x14; call 0x41E210` 無ければ `malloc(0x24)` → **`FUN_0042AC40`** → `0x41E0A0` で EDL へリンク）。`0x42AC40`: `[esi+4]=0x14`（type）、`[esi]=0xA35A70`（vtable）、`[esi+8]=0`、**`arg->vt+0x174`(GetPos) の3float を `[esi+0xC..+0x14]` へ、`[arg+0x20..+0x28]`（回転）を `[esi+0x18..+0x20]` へコピー**。ノード size 0x24 |
| ExtraTeleport（type 0x32） | ctor `FUN_00429C10`（唯一 caller `0x423553`＝get-or-create **`FUN_004234d0`**）: `[eax+4]=0x32`、`[eax]=0xA357DC`、**`[eax+0xC]=arg`（ヒープポインタ 1 本）**。dtor `FUN_00429C30` が `[+0xC]` を `0x7C88A0`（`mov [ecx],0; ret`＝vtable クリア）＋`free`。ノード size **0x10**。payload は位置/回転を**持たない** |
| アクセサ 4 本 | pos setter `0x41F7B0`（`ret 0x14`＝out+3float+?・`[node+0xC..+0x14]` へ書く）/ rot setter `0x41F770`（`[node+0x18..+0x20]`）/ rot getter `0x41F6F0`（`ret 8`）/ pos getter `0x41F730`（`ret 8`）。いずれも `push 0x14; call 0x41E210` → 無ければ factory |
| writer 全数（raw E8） | `call 0x41F7B0` = **2**: `0x45C769`（**`FUN_0045C4F0`** ロード時配置解決）/ `0x4DD228`（**slot+0xF8 setter `0x4DD1A0`** 台帳上は FUN_004dd130 範囲＝台帳結合罠）。`call 0x41F770` = **2**: `0x45C78C`（同）/ `0x4DD24B`（同） |
| reader 全数 | `call 0x41F6F0` = **1**: `0x4D821A`（slot+0xF0 rot getter `0x4D8210` 内）。`call 0x41F730` = **1**: `0x4D823A`（slot+0xF4 pos getter `0x4D8230` 内） |
| presence 検査全数 | `push 0x14; call 0x41E210` = **7 サイト**: factory 1＋アクセサ 4＋**`0x45BC31`**（`FUN_0045BB30`＝teleport 適用）＋**`0x45EA9B`**（`FUN_0045E990`＝破損修復）。これ以外に type 0x14 を直接を直接扱う経路なし |
| teleport 適用 `FUN_0045BB30` | 非 Actor 経路: extra 0x14 presence → `vt+0xF4`(pos)/`vt+0xF0`(rot) を取得 → **`FUN_004D8A30`/`FUN_004D89A0` で参照自身の `+0x2C..+0x34`(pos)・`+0x20..+0x28`(rot) へ反映＋`vt+0x40(4)` 通知** → `FUN_0041F7F0`（extra type **0x53** の `+0xC`＝セル系）→ `FUN_004DD4B0(ref, cell, world)` で親セル/ワールド解決 |
| 破損修復 `FUN_0045E990` | `vt+0x174`(GetPos) と `param_1[8..10]`（rot）を `FUN_00452430` で検証、破損時 `FUN_004A7A60("Corrupt location found loading reference %08X, fixing it.", formID)` → extra 0x14 無し時のみ origin 定数で修復 |
| copy 経路 | `CopyListForReference` switch case type 0x14 = `0x428C3C`（flag 設定時 `[node+8]` を追従＝**`+8` はポインタフィールド**（ctor は 0 初期化。正体は小残点）） |
| slot+0xF8 setter `0x4DD1A0` | v163 の実測を再確認: 引数 (x,y,z) が origin 定数 `0xB3F9A8/AC/B0` なら `vt+0x174`(GetPos)、否则引数 → `0x41F7B0` で pos、`this+0x20..+0x28`（rot）→ `0x41F770` |

### 2.3 GT/docs 訂正ドラフト（before→after・編集なし）

**GT §4.1.1 E 行（L140）へ追記する括弧（before→after）**
- before: `＝ **EDL(\`this+0x44\`) extra \`0x14\` の位置+回転保存セッター**（3 float 引数・\`ret 0xC\`。**死亡述語ではない**）`
- after: `＝ **EDL(\`this+0x44\`) extra \`0x14\` の位置+回転保存セッター**（3 float 引数・\`ret 0xC\`。**死亡述語ではない**。extra type 0x14 の RTTI は **\`.?AVExtraStartingPosition@@\`**（vtable 0xA35A70・ctor 0x42AC40・node 0x24B: +8 ptr/+0xC pos/+0x18 rot）。\`.?AVExtraTeleport@@\`(0xA357DC) は **type 0x32・+0xC 単一ポインタ** の別 extra（get-or-create 0x4234D0）で無関係。直接 writer は 0x45C4F0(ロード時配置スナップショット) と本 setter の 2 経路のみ・reader presence は 7 サイト（V177）`

### 2.4 残差（小）

- `node+8`（ポインタ・ctor 0 初期化・copy の flag 経路のみ読手）の正体。
- extra type **0x53** の `+0xC`（`FUN_0041F7F0` が返す＝teleport 適用時のセル解決キー）の class 名。

---

## §3 V-163c: slot `+0x2D0` 一般名 — **`StartItemAction`（アイテム使用アクション起動）に確定**

### 3.1 裁定

**訂正前**（GT §4.1 L126 / v163 §3.1）
> **アイテム使用系アクションの起動**（一般名 [要深掘]… event mask 4 …）

**V177 訂正後（ドラフト）**
> **一般名を `StartItemAction`（アイテム使用アクションの起動）に確定**（b27 §4 ファミリ整合名: Actor 版 = `StartItemAction(item, arg2)` / process 版 = `GetActionState(): int`）。確定要素: (1) イベント mask **4 = `OnDrop`**（GT §4 マスク表「§59.3 の14件」× report §59.3 `OnAdd=0x1/OnEquip=0x2/**OnDrop=0x4**/…` で実測照合。b16/V153 と 3 系統一致。**ただし `0x5E4440` 経路は `vt+0x100(item, 0, …)` と container 引数 0 で送出ゲートにより OnDrop は实际送出されない**＝共有ヘルパ `0x4D8760` の機能残留)、(2) `arg[+0x64]` は **TESBoundObject 埋め込み enchantment サブオブジェクト（TESEnchantableForm 系）** — `+0x18` に **MagicItem\*** を保持、null ガード必須、(3) `FUN_00699190` = **その enchantment を SpellItem へ dynamic_cast して埋め込み発動系コントローラ（`this+0x5C`）へ供給**（`MagicItem::vt+0x18()`==1→`controller.vt+4` / ==4→`controller.vt+0` で供給、受入不可時は `controller[+8]=5`（busy）/`=6` で拒否、受入時 `vt+0x34/0x3c` → `FUN_00414FE0` ゲート → `vt+0x10/0x14`）＝**読書による呪文習得/スクロール起動系の実装**、(4) `FUN_0048FBC0` = **キャッシュノード付きコンテナ内容物一括処理**（`param_1[2]=-1.0f`(0xA30634) センチネル、`param_1[1]`/`param_8` へ `vt+0x40(0x8000000)` 通知、`*param_1` リスト走査で `[elem+8]==param_3` マッチ）。b27 §4 の `+0x2D0..+0x2E0` アクション状態族（process 版 = 状態照会）と対で命名。

### 3.2 実測（本セッション）

| 証拠 | 内容 |
|---|---|
| mask4 = OnDrop | report §59.3 L10123 のイベントビット表（`OnDrop = 0x4`）を本セッションで実読。GT §4 L92「§59.3 の14件」と一致 |
| `FUN_004D8760`（vt+0x100）本体 | decompiled L170123-170146 全文: `param_2==0→return 0` → `iVar1=FUN_004D6D40()`（base formType 判定のコンテナ系リスト）→ **`FUN_004FBF90(param_1, param_3, 4)`（OnDrop emit）** → `FUN_00485E00`（キャッシュ get-or-create: `0x41E6F0` lookup → 無ければ `malloc(0x10)`+`FUN_00485D50`+`0x41EC10` 登録）→ `FUN_0048FBC0` へ 11 引数転送 |
| `FUN_0048FBC0` decompiled | L115490-: `param_1[2] = DAT_00A30634`（**-1.0f センチネル**・v171 D と同一定数）、`param_1[1]` と `param_8` へ `vt+0x40(0x8000000)` 通知、`*param_1` リストを `[elem+8]==param_3` で走査。**容量検証付き（V177 追補）**: L115577-115583 で `FUN_004845D0`/`FUN_00484620` のカウント値＋`iStack_2c`（`FUN_00469CA0(param_3)`）と `piStack_34[1]` の合計が上限比で超過する場合 `FUN_00484F20()` 再検証→`iStack_2c=0`（**重量/数量制限チェック**）→ do-while で `malloc(0xC)` 要素（`[2]=piStack_34[2]` 転送）。`param_8==g_thePlayer`(`0xB333C4`) 分岐あり |
| `FUN_00699190` decompiled | L464471-464513 全文（§3.1(3) の根拠）。`FUN_00415180()` 真 → `controller[2]=5` で拒否戻り。`param_2->vt+0x18()`==1 → `dynamic_cast<MagicItem(0xB03280)→SpellItem(0xB03800)>` → `controller->vt+4(cast結果)` / ==4 → `controller->vt+0(…)`。`controller->vt+0x30()` 非 0 → `controller[2]=6` 拒否。成功時 `vt+0x34/0x3c(param_2)` → `FUN_00414FE0()` ゲート → `vt+0x10()` or `vt+0x14()` |
| `arg[+0x64]` | **ENAM リンクで確定（V177 追補）**: TESObjectBOOK の ESM ロードハンドラ `FUN_004B5440` の FourCC switch 実測 — `ENAM`(`0x4D414E45`) ケース `0x4B55AB`: `FUN_004510E0`（`push 4; call 0x450C20` = 4-byte formID reader）の値を **`[esi+0x64]` へ格納**（TESBoundObject 直下の enchantment リンク。ロード解決後は EnchantmentItem* へ in-place 解決）。関連フィールドも併測: `DATA`(`0x41544144`) → `FUN_0046BDA0`（u16 flags reader）→ **`[esi+0x88]`**（＝ `FUN_005E4440` ガード `bit0` の所有）、`SCRI` → `[esi+0x58]`（§5.2 追補）。TESEnchantableForm は v153 §5.1.2(C) で vtable 登録 8 件（+0x10 = formID getter 0x943410）確認済み — **+0x18 は EnchantmentItem の MagicItem 系 [確定寄り]** |
| ファミリ整合 | b27 §4.3: process 版 `+0x2D0`=`0x628200`（`word[+0x1F4]` アクション状態照会、値 -1..6）＋`+0x2D4/2D8/2E0` 族。b27 §8 が提案した `StartAction`/`GetCurrentActionState` 分割命名を採用（"Item" を付して `StartItemAction` に具体化 — 本呼び出し実測は TESObjectBOOK、book case は `FUN_00617340` item-type switch case 6 で確認） |

### 3.3 GT 訂正ドラフト（before→after・編集なし）

**GT §4.1 `+0x2D0` 行（L126）**
- before: `| \`+0x2D0\` | **アイテム使用系アクションの起動**（一般名 [要深掘]。… **event mask 4** の送出）…`
- after: `| \`+0x2D0\` | **アイテム使用アクション起動 \`StartItemAction(item, arg2)\`**（process 版=\`GetActionState()\` と対。Actor 系は本体 \`ret 8\`。**GetFormTypeID ではない**） | … → \`0x4D8760\`（共有ヘルパ。**event mask 4 = OnDrop**（report §59.3）だが本経路は container 引数 0 で送出ゲートによりスキップ）→ \`FUN_0048FBC0\`（-1.0f センチネル＋\`vt+0x40(0x8000000)\` 通知のコンテナ一括処理）… \`this+0x5C\` 埋め込みコントローラへ \`FUN_00699190\`（**\`arg[+0x64]\`=enchantment サブオブジェクトの \`+0x18\` MagicItem を SpellItem へ dynamic_cast して供給**・受入時 controller 状態遷移、busy=5/拒否=6）＝読書による呪文習得/スクロール起動系…（V177）`

### 3.4 残差

- `MagicItem::vt+0x18()` 戻り値 1/4 の enum 名（1/4 が何を区別するか）。
- `controller`（`this+0x5C`）の class 名（`vt+0/4/0x10/0x14/0x30/0x34/0x3c` の各ハンドラ意味）。

---

## §4 V-163d: `[process+0x11D]` / `+0x11C` の値語義 — **家具使用状態 vs Knocked 状態**

### 4.1 裁定

**訂正前**（v163 §7 起票）
> `[process+0x11D]` 状態値 {0,1,4,6,9} と `+0x11C` 値 {0,1,3,5,6} の意味 [要深掘]

**V177 訂正後（ドラフト）**
> **`[HighProcess+0x11D]` = 家具（furniture）使用状態 byte [確定寄り]**: 状態機械は `FUN_00650030`（vt+0x1B4 impl）と `FUN_00654F10`/`FUN_00655590`/`FUN_00655BF0`。`FUN_00650030` は `switch(+0x11D)`（cases 2/7・3/8…）で `vt+0x370(ref, code, marker, markerIdx)` へ **code 0..10** を渡す遷移ディスパチャで、文脈文字列 **`"Missing furniture dynamic idle for marker %d in the editor's idle manager."`**（`+0x136` = marker index）を保持。値は **(n, n+5) 対**（vt+0x370 code の (1,6)/(2,7)/(3,8)/(5,10) 対と同型）: **0=未着座、1/6=着座遷移（通常/代替）、4/9=着座済み（通常/代替、`FUN_00654F10` が `+0x11D = (markerフラグ?5:0)+4` で生成）、5=一時値**。update ゲート {0,4,9}（`FUN_00629B70`）＝**着座済み/未着座のみ通常更新を続行し、遷移中(1/6)を除外**。保存される（serializer v153 確認）。
> **`[HighProcess+0x11C]` = Knocked/転倒系状態 byte [有力]**: slot+0x2E4 process 版 `0x64B080`（`movsx`）が返す値 {0,1,3,5,6}（17 読手の比較値実測）、b10 の GetKnockedState 系呼出しと整合。**隣接 byte だが +0x11D（家具）とは別枚挙**（vt+0x36C＝+0x11D を、vt+0x2E4＝+0x11C を各々返す）。

### 4.2 実測（本セッション）

| 証拠 | 内容 |
|---|---|
| `FUN_00650030` decompiled | L415084-415175: `+0x11D==0` 判定、`vt+0x370(param_2, code∈{0..10}, param_1[0x48], param_1[0x49])` ディスパッチ、`switch(+0x11D)` cases 2/7・3/8、fail 時 `+0x11D=0`、**`"Missing furniture dynamic idle for marker %d…"`＋`+0x136`（marker index）** |
| `FUN_00654F10` decompiled | L417474-417622: `cVar3=+0x11D`、`(cVar3==4 && cVar2)‖(cVar3==9 && !cVar2)` → return 0（陳腐ペア抑止）、`switch`: case 4/9 → `vt+0x370(…, cVar2?10:5, …)`、default → **`+0x11D = (-(cVar2!=0)&5)+4`**（cVar2 真→9 / 偽→4）。`cVar2 = FUN_004AE5D0(+0x136 の marker byte)` |
| writer 値 | v163/v153 の 12 writer（0/1/4/5/6/9）と整合。`FUN_006564C0`（load 側）が 1/6 を imm 書き。**1/6 の生成文脈（V177 追補）**: `FUN_006564C0` 内 switch が `mov bl,[esi+0x11d]`（旧値退避）→ `jmp switch(旧状態1..9)` → case で `[esi+0x17c]`→`[+0x98]`→`[+0x7c]`→`vt+0x4C`（global form `0xB06560/0xB06568`）→ `or word[eax+0x18],1` の後 `0x656F20: mov byte[esi+0x11d],1` ／ 別 case `0x656F29: mov …,6` → `[+0x11d]!=0` で `[esi+0x120]`(ptr)/`[esi+0x124]`(u8) を `FUN_004D7300(process,idx,1)` へ＋`g_thePlayer[+0x588]`→`FUN_0066C580` ＋ `vt+0x384` 通知 → `0x656FC3: cmp bl,4 / mov [esi+0x11d],bl`（旧値復元）＋ `bl∈{4,9}` でアニメ展開 = **1/6 は遷移中間値** |
| `+0x11C` 読手 | slot+0x2E4 process 版 `0x64B080` = `movsx eax,[ecx+0x11C]`。比較値 {0,1,3,5,6}（v163 §5.3 実測を維持）。95 decompiled 行中 vtable 本体は上記 |
| **`+0x11C` byte writer 全走査（V177 追補）** | disp32 `1C 01 00 00` window disasm＋decompiled `+0x11c` 95 行（大半は他クラスの無関係オフセット）→ process 系 byte 書き込みは **`FUN_0064B400` の `0x64B52D`（`+0xCC/+0x17C/+0x114/+0x115/+0x11C/+0x38` と同時に bl=0 で一括初期化）1 件のみ**。`FUN_006564C0`（load リーダー L418452 = `FUN_004534D0(+0x11d,1)` と同じ関数）に `+0x11c` 復元なし＝**セーブ非保存**（+0x11D と対照的）。getter 比較値 {1,3,5,6} の書手は未検出＝残点 V-177d |

### 4.3 残差

- 値 **5**（一時値）と **1/6 の対の厳密な区別**（「通常/代替」の実体: marker 属性か強制フラグか）。
- `+0x11C` の各値名（KnockedState enum の各定数名 — b10 呼出しとの突合は本タスク範囲外）。

---

## §5 V-163e: `FUN_004133C0` ガード／`HighProcess+0xF0`

### 5.1 裁定

**訂正前**（v163 §7 起票）
> `FUN_004133C0` の `base[+0x58]` bit7 ガードと `HighProcess+0xF0`（slot+0xF8 process 版が返すオブジェクト）の正体 [要深掘]

**V177 訂正後（ドラフト）**
> `FUN_004133C0` = **base フォーム flags bit7 ガード付き formID getter で確定**: `base=[this+0x1C]`(baseForm)、`[base+0x58]` bit7 立ちなら **0 を返す**、否则 `[this+0xC]`（formID）。直接呼出し **9 件**（raw E8。v163 の 8 は skipdata 漏れの可能性）。**対 setter `FUN_004133E0`**（6 caller）: 同ガード＋arg>=0 のとき `[this+0xC]=arg` かつ `[this+0x20]=-1.0f`(0xA30634) で戻り 1。**`[base+0x58]` は複数 bit を持つ flags dword** — bit7=本ガード（他 5 ファイルで同パターン）、**bit22=`FUN_00415180`**（`>>0x16&1`、FUN_00699190 内 busy ガード）、**bit31=`FUN_004AE5A0`**（`&0x80000000` 述語）。bit7 単体の意味名は [要深掘]（候補: 削除/一時/無効フォーム）。
> `HighProcess+0xF0` = **+0xE4 ファミリ第 4 リスト先頭 getter** — 兄弟ゲッタ **`0x64B240/270/2A0/2D0` → `[+0xE4]/[+0xE8]/[+0xEC]/[+0xF0]`**（いずれも `cmp byte[esp+4],0` の flag 引数付き条件取得、v171 で +0xE4＝「flags bit27/28 検査通過フォーム連結リスト」＝装備/エフェクト系 [有力]）。**元素型は v171 残点2 に継承**（本タスクでは process band 内 `[reg+0xF0]` 直接書手 0 件＝ctor 経由と推定、型決定には書手特定が必要）。

### 5.2 実測（本セッション）

| 証拠 | 内容 |
|---|---|
| `0x4133C0` 全文 | `mov eax,[ecx+0x1c] / mov edx,[eax+0x58] / shr edx,7 / test dl,1 / je 0x4133d1 / xor eax,eax / ret`（je 先 = `[ecx+0xc]` 返却＝v163 維持） |
| `0x4133E0` 全文 | 同ガード（bit7 立ち→`al=0`）＋`arg<0`→拒否 → `mov [ecx+0xc],edx`（formID 設定）＋`fld [0xa30634]; fstp [ecx+0x20]`（**-1.0f 替入**・0xA30634＝v171 D 確定の -1.0f 定数）＋`al=1; ret 4` |
| caller 全数（raw E8） | `0x4133C0` = **9**（0x413EF0, 0x5A03D0, 0x5A03F2, 0x5A116C, 0x68D7E7, 0x69A815, 0x69D68C, 0x6A2C66, 0x6A876C）/ `0x4133E0` = **6**（v153 の 6 と一致） |
| **`[base+0x58]` の所有（V177 追補）** | TESObjectBOOK ESM ロード `FUN_004B5440` の FourCC switch 実測 — **`SCRI`(`0x49524353`) ケース `0x4B550D`: `FUN_004510E0`（4-byte formID reader）の値を `[esi+0x58]` へ格納**（＋`[esi+0x54]` へ `FUN_0046F020` = script name 系）＝ **+0x58 = script リンク（解決前 formID / 解決後 TESForm*）**。bit7/22/31 の flags 的利用（別行）と如何に整合するかは残点（未解決 formID モードの下位ビット由来 [有力]、V-177c 継続） |
| 同ガード他读者 | decompiled grep `+ 0x58) >> 7` = 8 ファイル（`FUN_004133C0/004133E0/00413BE0/00413FC0`×2/`FUN_005A08E0`/`FUN_0068E670`）＋`& 0x80` = `FUN_004AE5A0`（bit31 述語）＋`>> 0x16` = `FUN_00415180`（bit22）＝ **同 dword に bit7/22/31 が分散テスト** |
| 兄弟ゲッタ 4 本 | `0x64B240`→`[esi+0xE4]` / `0x64B270`→`[+0xE8]` / `0x64B2A0`→`[+0xEC]` / `0x64B2D0`→`[+0xF0]`（本文完全一致・flag 引数 1）。v171 §1-C(4) の `0x64B240`＝process `+0xEC` impl（slot→field は 4B シフト）と整合 → slot+0xF8 process 版 = field +0xF0 |
| **`FUN_00484E80` 本体（V177 追補）** | `entry=[ecx]`（リスト先頭）→ 各ノードで `FUN_0041DF10(entry->form, arg)` を呼び真で 1 を返す**連結リスト走査検査**、末尾まで偽なら 0（v171 の「flags bit27/28 検査 `0x41DF10`」の本体）＝ flag 引数付きゲッタのガード機構を実証 |
| `[reg+0xF0]` 直接書手 | process band(0x60–0x6C) の `mov [reg+0xf0],imm32/reg`（disp8 形）= **0 件** → ctor/其它エンコーディング経由と推定。型決定は不可＝v171 残点2 へ |

### 5.3 残差

- `[base+0x58]` bit7/bit22/bit31 の各フラグ名（TESBoundObject flags の対応表）。
- `HighProcess+0xE4..+0xF0` リスト族の元素型（装備/エフェクト/魔法のどれか）＝ **v171 残点2 と同一課題**（本タスクで +0xF0 を同族に位置づけたことで候補が 4 リストに絞られた）。

---

## §6 v121 V147 残り（差分のみ・重複除外）

v121 §5 V147 =「§4 候補 A–F の GT 訂正（`+0x170`/`+0x164`/`+0xEC`/`+0x2C0`/`+0xF8`/process`+8`）— 本タスクでは未実施」。V153/V163/V171 での処理状況を突合した**残差のみ**:

| 候補 | 現在の状態 | 残作業（差分） |
|---|---|---|
| A `+0x170` | **解消**: GetBaseForm（v171 §1-A 確定、GT ドラフト G-B 準備済み） | GT §4.1.1 A 行を「裁定待ち」→確定行へ置換（v171 G-B 適用）＋report §41.3 D-C 適用（親セッション・承認制） |
| B `+0x164` | **解消**: GetParentCell（v171 §1-B、G-C 準備済） | 同上（GT G-C＋report D-B） |
| C `+0xEC` | **解消**: GetScale 系統別（v171 §1-C、G-D 準備済） | 同上（GT G-D＋report D-A） |
| D `+0x2C0` | **解消**: 系統別 3 語義（v171 §1-D、G-E 準備済） | 同上（GT G-E＋report D-D） |
| E `+0xF8` | **解消**: V163 で extra 0x14 setter 確定、GT §4.1.1 E 行＝確定・report 訂正注記適用済（GT L140 記載） | **なし**（本タスクで class 名を ExtraStartingPosition に補強＝§2.3 括弧追記のみ） |
| F process`+8` | **解消**: report §17.2 へ 4 値化訂正適用済（GT L141 記載） | **なし** |

**V153 で処理済みのため列挙除外**: `0x188`（IsMobile/時間積算・v153 §5.1.5）、`0x250/0x254`（犯罪メーター・v158 §3-8）、`0x2E4`（delta 更新/`[0x11C]` getter・v153）。
**V147 と重複しない v121 §5 の其它残点**（本タスク範囲外のため状態のみ）: V143-a bit14 → v158 §3-5 で解消、V143-d World::Update → v158 §3-6 で縮小（V-158d 継続）、V145 一般名 → **本タスク §3 で `StartItemAction` に確定**、V146 → V163 §4 で解消、V148 → V153/V163 で解消、V149 → GT §4.1 常設済（V143）、V153-a〜g 中 ①③ → V163/本タスクで反映、**②④（b14 §1.2 の 0x188 行・0x2E4「ret void」訂正、b14 §1.1「4候補はいずれも未登録」→2 件は登録関数）は b14 編集禁止のため親セッション残件**。

→ **V147 の実質残件は「V171 ドラフト（GT §4.1.1 A–D 行＋report §41.3 4 行）の適用待ち」に縮小**。

---

## §7 新規残点（起票粒度）

| ID | 内容 | 優先度 |
|---|---|---|
| V-177a | ExtraStartingPosition `node+8`（ポインタ・copy flag 経路のみ読手）と extra type **0x53** `+0xC`（teleport 適用のセル解決キー `FUN_0041F7F0`）の正体 | 低 |
| V-177b | `MagicItem::vt+0x18()` 戻り値 1/4 の区別語義と `this+0x5C` 埋め込みコントローラ（`FUN_00699190` の供給先）の class 名 | 低 |
| V-177c | `[base+0x58]` flags dword の bit7/22/31 の各フラグ名（TESBoundObject flags 対応表） | 低 |
| V-177d | `+0x11D` 家具状態の値 5 と (1,6) 対の厳密区別／`+0x11C` **KnockedState 各値名＋{1,3,5,6} の遷移書手**（byte writer は `FUN_0064B400` 初期化 1 件のみ・セーブ非保存を V177 追補で実測＝書手検出不能）（b10 突合） | 低 |
| V-177e | V171 残点2 との統合: `+0xE4..+0xF0` リスト族（4 リストに絞り込み済み）の元素型決定 | 中 |
| （継続・**高**） | V171 ドラフト（GT §4.1.1 A–D 確定行＋report §41.3 D-A..D-E）の親セッション適用 — V147 の実質残件 | **高** |
| （継続・中） | b14/b27/v121 側の訂正候補 **8 件**（①–⑥は編集禁止のため親セッション）: ①b14 §1.2 `0x188` 行「REFR 系→`0x6F7070`」→ 実測 REFR は `0x69D990`/`0x977C50`（`0x6F7070` は 0x188 に 0 件・v153）②b14 §1.2 `0x2E4`「ret void」→ `ret 4`（float1 引数・v153）③b14 §1.1「GetFormID 4 候補はいずれも未登録」→ `0x88D370`/`0x943410` は登録関数（V153-e④）④b14 §1.2 `0x154` 行「TESForm 系(30)→`0x4B3750`」列削除（V-27b・b27 §1.3）⑤b14 §1.1 `0x2C0` 行「High/**MidHigh**→`0x6285A0`」→ 実測 MidHigh=`0x60CF50`（v171 残点6）⑥b14 §5-6 castType「静的キャスト=1」削除（実測 1=0 件・V-27g=b27 §9.1 で根因解消済）⑦b27 §0 タスク要約「Actor 版 `0x5E4440` が movsx を持つ…経由」→ 字義通り不成立（兄弟実装であって呼出し経路ではない・b27 §(4) 自体は整合済み＝要約行のみ修正）⑧v121 §1.4 の `+0x36C` 値集合「0/1/3/6 系」→ 実測 **0/1/4/5/6/9**（GT 本文は V163 で修正済みだが v121 文書側が未訂正） | 中 |

---

## §8 証跡・自己検証

### 8.1 証跡（すべて `%TEMP%\opencode\`・既存 docs/GT/TODO/cache 非変更）

| ファイル | 役割 |
|---|---|
| `v177_1_formidtostring.py` / `.txt` | **V-163a**: `0x47E190` 全文・`FUN_0047D640` 本体・raw E8 直接呼出し 4 件＋文脈・文字列 xref（0xA3D5C0/0xA65068）・テーブル 3 系統（0xB081D0/0xB06870/0xB05E04）・`FUN_0044CDF0` decompiled |
| `v177_1b_formid_neg.py` / `.txt` | 負経路 `0x47E1A9..0x47E27A` 全文（22 名 jumptable＋`cl>=0x45` 警告）・`FUN_0047D640` 負経路（`or eax,-1`）・0x55DCEC の台帳 enclosing・jumptable コードマップ＋全ターゲット |
| `v177_2_extrateleport.py` / `.txt` | **V-163b**: ExtraTeleport RTTI・ctor refs・**アクセサ 4 本全文**・raw E8 全数（writer 2/2・reader 1/1）・`push 0x14` サイト 7 件・`0x4DD1A0` 全文 |
| `v177_2b_extrateleport2.py` / `.txt` | vtable 隣接 RTTI（ExtraLeveledCreature/BSExtraData）・**ExtraTeleport vtable[0]/[1] dtor/cast-thunk**・`FUN_00429C30` dtor 全文・`FUN_0041EA70` factory 全文（malloc 0x24）・`FUN_0045C4F0`/`FUN_004DD130`/`FUN_0045BB30`/`FUN_0045E990` decompiled・raw E8（0x45C4F0=4 callers） |
| `v177_3_extrateleport3.py` / `.txt` | **`FUN_0042AC40` ctor 全文（type 0x14/vtable 0xA35A70/pos+rot コピー）**・raw bytes @0x429C10（type 0x32 確認）・caller 全数（0x429C10=1/0x42AC40=1/0x41E210=257/0x41EA70=4）・`0x7C88A0` 実測・CopyListForReference switch case 0x14 |
| `v177_4_step4.py` / `.txt` | **RTTI: 0xA35A70=`ExtraStartingPosition`（vtable_index #716 と一致）**・0x429C10 caller 文脈・`FUN_0045BB30` 続き（pos/rot→参照反映→セル解決）・`FUN_004DD4B0`/`FUN_004D8A30`/`FUN_004D89A0`/`FUN_0041F7F0`（extra 0x53）・**V-163c**: `FUN_0048FBC0`/`FUN_00699190` decompiled・TESObjectBOOK vtable 9 本 |
| `v177_5_step5.py` / `.txt` | ExtraTeleport get-or-create `FUN_004234d0` decompiled（type 0x32 lookup/生成/置換）・`0x7C88A0` 精確デコード・`FUN_004D8760` 全文・`FUN_00485E00`・**V-163e**: `FUN_004133C0/E0` 全文＋caller 全数（9/6）・`base+0x58` bit7/22/31 reader grep |
| `v177_6_step6.py` / `.txt` | **V-163d**: `FUN_00654F10` decompiled（(cond?9:4) 生成）・`FUN_00650030` switch 文脈（furniture 文字列・vt+0x370 code 0..10）・`+0x11D/+0x11C` decompiled 全行・V-163e: 兄弟ゲッタ 4 本本文（+0xE4/E8/EC/F0）・process band `[reg+0xF0]` 書手走査（0 件） |

再現: `python "%TEMP%\opencode\v177_N*.py"`（順不同・読取のみ）。

### 8.2 自己検証

| 検証項目 | 期待 | 結果 |
|---|---|---|
| `probe_harness.py selftest` | 4/4 PASS | **4/4 PASS**（着手時: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| ExtraStartingPosition RTTI | vtable_index #716 と一致 | ✓（td 0xB03E34 実測一致） |
| accessor writer/reader 全数 | raw E8 | ✓ writer 2+2 / reader 1+1 / presence 7 |
| mask4 = OnDrop | report §59.3 と GT §4 マスク表 | ✓ 実読で一致（3 系統） |
| ENAM/SCRI/DATA FourCC | — | ✓ `FUN_004B5440` switch 実測: **ENAM→`[+0x64]` / SCRI→`[+0x58]` / DATA→`[+0x88]`**（V177 追補＝§3.2/§5.2 へ統合） |
| `+0x11C` byte writer | — | ✓ process 系 = `FUN_0064B400` の 1 件のみ・非保存（V177 追補＝§4.2 へ統合） |
| `FUN_004133C0` caller | v163=8 | **9**（raw E8。v163 の 8 は skipdata 漏れの可能性を注記） |
| `FUN_004133E0` caller | v153=6 | ✓ 6 |
| 禁止事項（docs/・GT・TODO・session-memory・plan・cache・git・既存 tmp/re 上書き） | 0 件 | **遵守**（新規は本 1 ファイル＋`%TEMP%\opencode\v177_*` のみ） |

### 8.3 GT §5 訂正履歴に貼る 1 行（テンプレ・未実施）

```
| 2026-10-28 | V177: GT slot 語義残り6件を確定 — FormIDToString `0x47E190` の「FormID」= formType enum 型名（+0xC の formID フィールドではない・乖離不存在）/ EDL extra 0x14 = **ExtraStartingPosition**（RTTI 0xA35A70。旧 v163「ExtraTeleport」表記は誤り — ExtraTeleport は type 0x32・単一ポインタの別 extra）/ `+0x2D0` 一般名 = **StartItemAction**（mask4=OnDrop 照合・arg[+0x64]=enchantment サブオブジェクト・FUN_00699190=SpellItem 供給）/ `[process+0x11D]` = **家具使用状態**（0/1・6/4・9 対・ゲート{0,4,9}=遷移除外）、`+0x11C` = Knocked 系 [有力] / `FUN_004133C0` = base flags bit7 ガード付き formID getter（setter 0x4133E0 対・base+0x58 は bit7/22/31 分散の flags dword）/ `HighProcess+0xF0` = +0xE4 ファミリ第4リスト（v171 残点2 統合）。出典 `tmp/re/v177_gt_slot_residuals2.md` |
```
