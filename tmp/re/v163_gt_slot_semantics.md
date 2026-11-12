# V163: GT slot 語義の残点 6 件 — 訂正前→訂正後ドラフト（分析成果物・編集なし）

作成: 2026-10-28（opencode サブエージェント V163 / **分析専任**）
対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / SHA256 `A8F31384…C2B7DC6`）
手法: capstone 5.0.7（`skipdata=True`, `md.detail=True`）、ImageBase `0x400000`、VA = `0x400C00` + file_off（.text）/ `0x400E00`（.rdata）/ `0x401400`（.data）
入力（読取のみ）: `tmp/re/v121_gt_corrections.md`（§1/§5）、`tmp/re/b27_slot_residuals.md`（§3/§4/§6）、`tmp/re/b14_slot_method_map.md`、`tmp/re/GT_CHEATSHEET.md`、`docs/RE_EXE_REPORT.md`（§17.2/§41.3 該当行のみ）、`tmp/re/cache/vtable_index.tsv`、`docs/Oblivion_decompiled.c`（部分検索）
状態: **分析完了・編集なし**（`docs/`・`GT_CHEATSHEET.md` はバイト1つも変更していない。TODO / session-memory / plan / cache / 既存 tmp/re も非変更。新規は本 1 ファイル＋`%TEMP%\opencode\v163_step1..19.py/.txt` のみ）

> **本稿の位置づけ**: 各件とも「訂正前（現行 GT/ハンドオーバ記述）→ 訂正後（反映可能なドラフト）」を示す。GT 本体の編集は行わない（残点 6 は設計案のみ）。
> 着手時 `probe_harness.py selftest` = **4/4 PASS**。

---

## §0 判定サマリ（6 残点）

| # | 残点（出典） | 判定 | ドラフト |
|---|---|---|---|
| 1 | `+8` フォーム比較/同期ヘルパの名称確定（v121 §5 V143） | **解消**: 名称を「**フォーム属性同期ヘルパ `CopyFormAttrs`**」に確定（コピーファミリ `+0xB4/+0xB8` の同居を発見）。呼出し元 843 は**全クラス共通スロット**であり、引数個数で **0引数 625 / 1引数 217** に分類 → TESForm 系候補は 217 上限 | §1 |
| 2 | GetFormID の所在（v121 §5 V144） | **解消**: **仮想関数ではない**。formID = `TESForm+0xC` の直接フィールド（実証済）。4 候補はすべて別物と否定。文字列化は**未登録関数 `0x47E190`（FormIDToString 警告の唯一参照先）** | §2 |
| 3 | `0x2D0` Actor 版 `0x5E4440` の用途（v121 §5 V145 / b27 §6 部分解消） | **大幅縮小（残差あり）**: arg = **TESBoundObject 系アイテム**（本呼び出しは `TESObjectBOOK`）＋`+0x64`≠0 ガード、`vt+0x100`→`0x4D8760` で**イベントマスク 4** を送出、`+0x628` に保持再起動。一般名は「**アイテム使用系アクションの起動**」に寄せる（mask4 のイベント名は [要深掘]） | §3 |
| 4 | `0x36C`「状態 vs 種別」の裁定（v121 §5 V146） | **裁定可 →「状態」に確定**: ゲート値 {0,4,9} が**書込値に実在**（`neg/sbb/and 5/add 4` で {4,9} を算出）することを実測、reader 定数も {0,4,9} 主導。種別値域(0x12..0x55)と重ならない | §4 |
| 5 | `0xF8` / `0x188` / `0x250` / `0x2E4` の正体（v121 §5 V148） | **解消（0x250/254 は b27 §3 済み）**: `0xF8` = **EDL extra `0x14`（位置+回転）保存セッター**（`+0xF0/+0xF4` と三兄弟を発見）／`0x188` = **系統別二重意味**（REFR 系クラス述語 vs Process 系の時間積算）／`0x2E4` = **process byte `+0x11C` の状態取得＋Actor 版 float 更新ヘルパ** | §5 |
| 6 | GT §4 slot 対応表の常設＋handover L43 非参照の運用化（v121 §5 V149） | **設計案**（GT 本体は編集しない） | §6 |

---

## §1 残点 1: `+8` フォーム比較/同期ヘルパの名称確定

### 1.1 訂正前 → 訂正後（ドラフト）

**訂正前**（出所: `tmp/re/handover_20261025/session_B_summary.md:43`。GT 本体には未収録）
> `vt+8=GetFormID`

**V121 訂正後（現行ドラフト・名称保留）**
> | `+0x08` | **フォーム比較/同期ヘルパ**（名称保留。**GetFormID ではない**） | …

**V163 訂正後（名称確定・GT §4.1 貼付用）**
> | `+0x08` | **フォーム属性同期ヘルパ `CopyFormAttrs(BaseFormComponent*)`**（`ret 4`・戻り値なし。**GetFormID ではない**） | TESForm 系 86 vtable → `0x46B800`（`FUN_0046b7a0+0x60`）: arg を `__RTDynamicCast(BaseFormComponent→TESForm)`、失敗なら無操作。成功時 (a) `this->flags bit1(0x2)` と arg の bit1 が異なる場合のみ `this->vt+0x90(arg_bit1)`（`0x46C640` = グローバルフォーム配列 `0xB0614C` への登録＋`flags\|=2`、arg==0 経路は登録解除＋`flags&=~2`）、(b) `this+4`(formType) ← arg の formType、(c) `this->flags` ← `(this & 0x4000) \| (arg & ~0x4000)`（**arg の flags を移植しつつ this の bit14(0x4000) のみ保持**）。`ret 4` で ID を返さない＝GetFormID ではない。**同一関数ブロックにコピ/比較系が同居**: `+0xB4`=`FUN_0046b7a0` は文字列 `"Trying to copy form \"%s\".\r\nNo copy function for %s form types.\r\n"`(0xA3BD20)、`+0xB8`=`0x46B7D0` は `"Trying to compare form \"%s\".\r\nNo copy function…"`(0xA3BD68) を印字する**フォーム複製/比較の不在警告**、`+0x0C`=`0x46B870` は **`IsDifferent(other)` 述語**（arg 非 TESForm→1 / formType 不一致→1 / 名前（`vt+0xD4`・長さ `FUN_00404E50`=strlen）不一致→1 / flags 差分 `test …,0xFFFFBFF4; setne al`＝**bit{0,1,3,14} を除く全ビット差で 1**）→ 名称は「フォーム複製時の属性同期」が本命 |

### 1.2 根拠（本セッション実測）

- **vtable 登録 4 点**（TESForm `0xA3BE3C` 実測・v121_step2 と一致）: `+0xB4`=`0x46B7A0` / `+0xB8`=`0x46B7D0` / `+0x08`=`0x46B800` / `+0x0C`=`0x46B870` — 同一ブロック（Ghidra ラベル `FUN_0046b7a0+0x60/+0xD0`）。
- **`FUN_0046b7a0`（+0xB4）本文**（v163_step1）: `movzx eax,byte[ecx+4]`（formType）→ `table[formType]`（`0xB05E04`、12B/エントリ・先頭 dword = 型名 ptr）→ `arg->vt+0xD4()`（0 引数＝フォーム名）→ `0x4A7A60(fmt, 名前, 型名)` で警告印字。
- **`0x46B7D0`（+0xB8）**: 同型だが fmt = `0xA3BD68`（compare 警告）＋`xor al,al; ret 4`（不在時は「同一」=0 を返す保守既定）。
- **`0x46B800`（+8）本文**: v121_step2 の逆アセンブル（本セッションで vtable 再測一致）。
- **`0x46B870`（+0xC）本文**（v163_step1/2 で全文取得）: キャスト失敗→`mov al,1; ret 4`、formType 不一致→1、`FUN_00404e50`（`vt+0xD4()` の **strlen**、再入ガード `DAT_00b333f4`）で名前長比較→`strcmp(0x98262d)`→flags XOR `test ecx,0xffffbff4; setne al`。
- **Process 系は同一 offset で別物**（v121 §1.1 維持）: Base/High=`0x6F7070`(0) / Low=`0x73FD50`(3) / MidLow=`0x8CF6B0`(2) / MidHigh=`0x91FEA0`(1)。

### 1.3 呼出し元 843 箇所の分類（v121 の数値を再現・系統別に分解）

| 分類軸 | 結果（v163_step3/4/5 実測） |
|---|---|
| 厳格ディスパッチ（`vt=[r]`→`[vt+8]`→`call reg`、8 命令窓） | **843 箇所**（v121 と一致）／`call dword [reg+8]` 形は 318（v121 値を引用） |
| ⚠️ 性質 | **slot #2 は全クラス共通**（b14: slot `0x08` を持つ vtable 1,230 本 / distinct impl 220）。**843 は「TESForm 系 +8 の呼出し」ではなく全クラス第 3 仮想関数の総数上限** |
| **引数個数による系統分解**（call 直前の push/add esp ネット） | **0 引数 = 625（74%）**＝process 系レベル取得・Ni/Renderer 系等／**1 引数 = 217（26%）**＝`ret 4` 互換＝**TESForm 系 +8 候補（上限）**／-2 = 1（誤検出） |
| 1 引数 217 の分布 | **157 関数**に分散。上位: `FUN_00782040`×8 / `FUN_00781170`×4 / `FUN_00781350`×4 / `FUN_004043a0`×3 / `FUN_00480000`×3 / `FUN_006ac020`×3（以降 1〜2 件ずつ） |
| 全 843 の上位関数＋文字列ヒント（v163_step3-B） | `FUN_007ac2c0`×17、`FUN_0044ac20`×10（GameYear/Month/Day）、`FUN_00782040`×8、`FUN_00893b90`×7、`FUN_004e3490`×4（"Actor '%s' needs to have a 'skeleton*.nif'" 等の**フォーム検証**）、`FUN_006005f0`×3（死亡メッセージ）、`FUN_00668d00`×4（PlayerInventoryPC）、`FUN_00460710`×2（"Error saving actor %08X…"）… 上位 60 関数で 26.8%（**595 関数に分散＝slot #2 の汎用性の裏付け**） |
| decompiled 側の `+ 8))(` | **1,487 行**（上位: `fun_00462280`×23, `fun_00464ab0`×17, `fun_007ac2c0`×17 …） |

**分類からの結論**: TESForm 系 `+8`（`0x46B800`）の呼出しは 843 から**系統で絞り込む必要**があり、静的解析だけでは完全分離できない（実行時クラス依存）。確定できたのは「コピーファミリ同居＋本体挙動」であり、名称はこれで確定できる。呼出し元のうち 1 引数 217 が現実的な探索範囲（上限）。

---

## §2 残点 2: GetFormID の所在

### 2.1 訂正前 → 訂正後（ドラフト）

**訂正前**（handover L43）
> `vt+8=GetFormID`

**V121 訂正後（現行ドラフト）**
> GetFormID は本スロットではない（非仮想か別スロット＝V143+ 残点）

**V163 訂正後（ドラフト）**
> **GetFormID は仮想関数として存在しない。** formID は **`TESForm+0xC` の直接フィールド**で、コードは `[form+0xC]` をインライン読む。近い非仮想ヘルパは (a) `FUN_004133C0`（`base=[this+0x1C]` の `base+0x58` bit7 が立つと 0、否则 `[this+0xC]` を返す**ガード付き getter**、REFR 系 object から直接呼出し 8 件）、(b) **文字列化 `FormIDToString`＝未登録関数 `0x47E190..0x47E27A`**（文字列 `"Invalid FormID sent to FormIDToString().\r\n"`(0xA3D5C0) の**唯一の参照先**、直接呼出し 4 件すべて `FUN_0044CDF0` 内、`byte[form+4]`(formType) を入れると型名文字列を返す・`ret`（呼び出し側 `add esp,4`））。**v121 の 4 候補はすべて別物**（下表）。ASCII `GetFormID` はバイナリ内 0 件（スクリプト関数/コンソールでも不存在）。

### 2.2 `+0xC` = formID の実証（本セッション）

| 証拠 | 内容 |
|---|---|
| 桁抽出 | `FUN_004CA5F0`: `if (*(byte*)(p+0x24) & 1) return (*(uint*)(p+0xC) & 0xffffff) % 10;` ／ `FUN_004CA640`: `… %100 / 10` — **formID の object index（下 24bit）から一桁目/十桁目を抽出**（else 分支はセルグリッド座標 `>>5` 経由＝世界座標系と対） |
| アロケータ | `FUN_00448D40`: `uVar2 = *(p+0x8C0)`（next formID）→ `(uVar2 & 0xffffff)==0xffffff` なら `(uVar2 & 0xff000000)+0x800`、否则 `+1` → ハッシュ表確認 →採番。＝**上位 byte = mod index / 下位 24bit = object index** の 32bit formID 管理 |
| 一時 formID | `FUN_00446B40`: `(param_2 & 0xff000000)==0xff000000 && param_2==*(p+0x8C0)-1 && (param_2&0xffffff)>0x7ff` で採番更新 ／ `FUN_00446B80`: `return 0xfeffffff < param_1`（modIndex 0xFF 判定） |
| 逆経路 | Player 版 `0x666820`: `mov eax,[edi+0xC]` → `0x4D8D70(this, arg, arg[+0xC])` → EDL type `0x1A` → `FUN_00487D20` → `FUN_00487C30` → **`FUN_00485E80(form, 1, formID)`**（formID をキーに解決） |
| 文字列 | `.rdata` に `"pActor->iFormID = %08X"`(0xA65068, 参照 `FUN_0055D8C0`) ／ `"Invalid FormID sent to FormIDToString().\r\n"`(0xA3D5C0) ／ `"formEnumString[ %d ].cFormID in TESForm.cpp is out of order."`(0xA3BFD4, 参照 `FUN_0046C460`＝TESForm vtable をセットする検証関数) |

### 2.3 v121 の 4 候補の否定（本セッションで各本体・呼出し元を実測）

| 候補 | 実測 | 判定 |
|---|---|---|
| `0x88D370` | `mov eax,[ecx+0xC]; ret`・直接呼出し **31**。**全呼出しで `lea/add ecx,[obj+0x1e0]`** → 読むのは **`[obj+0x1EC]`**。比較値は **0/2/4/5**（`FUN_005e1550` は `return … == 5`）、`FUN_00407AE0` では**配列インデックス**に使用 | **別物**（`+0x1E0` サブオブジェクトのアクション状態 dword。b27 §4.3 の `+0x1EC` リセット族と同一） |
| `0x943410` | 同本文・直接呼出し 13・vtable 登録 8 件 = **`TESEnchantableForm +0x10`** ＋ NiD3DShader 系(+0xC)/BSFaceGen 系(+0x54) の**オフセット偶然** | **別物**（TESEnchantableForm の +0xC フィールド getter） |
| `0x4133D1` | `FUN_004133C0+0x11` の内部ラベル: `base=[ecx+0x1C]; [base+0x58]>>7 &1` なら **0**、否则 `[ecx+0xC]`。直接 xref 0（`FUN_004133C0` 経由で呼出し 8 件、`[esi+0x94]`/`edi` 等の REF R 系 object） | **ガード付き GetFormID 相当**（本命の非仮想 getter。ガード `base+0x58` bit7 の意味は [要深掘]） |
| `0x5204C9` | `FUN_005204C0+0x9`: `obj=[ecx+0x3C]`（null→0）→ `[obj+0xC]`。xref 0 | **別物**（間接参照先の +0xC getter） |

- 補助: `.text` の `mov eax,[ecx+0xC]` = **168 箇所**、うち `…; ret` 直後は上記 4 のみ（`vtable 登録` は `0x46C700`(+0x74) / `0x482240` / `0x482260` / `0x55AE90` 等、いずれも複合本文＝formID getter ではない）。TESForm vtable 55 スロットに `mov eax,[ecx+0xC];ret` = **0 件**（v121_step8 と一致）。

### 2.4 FormIDToString 実装（新規特定・残契約あり）

- `0x47E182..0x47E31F` は **Ghidra 未登録領域**（`FUN_0047e140`＝rand ラッパーは `0x47E181` で `ret`、次の登録関数は `0x47E320`）。int3 パディングの後 **`0x47E190` がエントリ**。
- 本体: `idx = FUN_0047D640(arg)`（`byte-0x0A` の jumptable）→ `idx>=0` なら **`table 0xB081D0[idx]`**（例: `[0]= "Activator"`）を返す／負なら `cl=arg の下位 byte`、`cl-3<=0x3A` なら jumptable で**キャッシュ型名**（`0xB06870="Race"` … `0xB068C4="AI Package"`、21 個）／`cl>=0x45` なら **`"Invalid FormID sent to FormIDToString().\r\n"` を印字**して `table 0xB05E04[cl]`（4 文字型コード表）へフォールバック、否则 `table 0xB05E04[cl]` を返す。
- 直接呼出し **4 件すべて `FUN_0044CDF0`**: `movzx edi,byte[esi+4]`（formType）＋ `form->vt+0xD4()`（名前）を積み → `call 0x47E190; add esp,4` → 結果（型名）と名前をログ出力。
- **残契約 [要深掘]**: 実引数は formType byte なのに警告文は FormIDToString — 「FormID→文字列」元の命名と実装の乖離（または formID の下位 byte を型として扱う経路）の確定。

---

## §3 残点 3: `0x2D0` Actor 版 `0x5E4440` の用途（残差）

### 3.1 訂正前 → 訂正後（ドラフト）

**訂正前**（handover L43）
> `vt+0x2D0=GetFormTypeID()→int`

**V121 訂正後（現行ドラフト）**
> | `+0x2D0` | **攻撃・アイテム消費系アクション**（本体は `ret 8` ＝ 2 スタック引数・GetFormTypeID ではない） | …

**b27 §4 で確定した追加分**（引用のみ）: 同 slot の process 版は「**アクション状態照会**」（High `0x628200`=`word[+0x1F4]`、`+0x2D4/+0x2D8/+0x2E0` とペア、値集合 -1/0/2..6/8/0xB）。**`0x5E4440` と `0x628200` は兄弟実装**（呼出し経路ではない）。

**V163 訂正後（残差込み・GT 貼付用）**
> | `+0x2D0`（Actor 系） | **アイテム使用系アクションの起動（`StartItemAction` 系・一般名 [要深掘]）**: arg1 = **TESBoundObject 系アイテム**（本呼び出しは `TESBoundObject→TESObjectBOOK` に `dynamic_cast`、ガード `+0x64 ≠ 0`）、arg2 = 追加引数（`ret 8`）。`+0x88 bit0` 必須。通過後 (1) `this->vt+0x100(arg,0,1,0,0,0,0,0,1,0)` → **`0x4D8760`**（10 引数 `ret 0x28`）＝**`FUN_004FBF90(this, …, mask=4)`（イベント送出）＋`FUN_00485E00`＋`FUN_0048FBC0(this, item, 0,1,0,0,0,0,1,0,0)`（本体大物）**、(2) `FUN_005C1900()`（グローバル `0xB3B444` リスト走査の後始末）、(3) `(&this->+0x5C)`（埋め込みコントローラ）へ `FUN_00699190`＝**`item[+0x64]→+0x18` を `dynamic_cast<MagicItem→SpellItem>` して供給**（`vt+0x18` の 1/4 分岐＋`param_1[2]=5/6` 状態書込）、(4) `this==g_thePlayer` のとき `FUN_00664850(0)`（**`player+0x628 = item` を保持**、`item[+0x64]→+0x18` を `FUN_00664700` へ）。Player 版 `0x666820` は本関数＋`0x4D8D70(this, item, item[+0xC])`（EDL type `0x1A` → `FUN_00487D20` → `FUN_00485E80(item,1,formID)` の**使用記録**をヒープ生成し直後に dtor/free）＋`0x664850`。**再起動経路**: `FUN_00671620` が `actor[+0x628]` を `actor->vt+0x2D0(item,0)` で再発火。系統別: process 版（0 引数）は**状態照会**（-1/0/2..6/8/0xB）、Base 系 `0x6F7030`=`or eax,-1` |

### 3.2 根拠（本セッション実測）

- **呼出し 37 箇所（厳格）＝ decompiled `+ 0x2d0))(` 36 行の分類**:
  - **0 引数＝process 版の状態照会が大半**（`param_1[0x16]`＝`param_1+0x58`＝**process**、`**(int**)(param_1+0x58)+0x2d0`）。比較値: `-1,0,5,6,8,0xB, iVar3-2U<4(=2..5)`（b27 の値集合と完全一致）。
  - **2 引数＝Actor 版の起動が 2 箇所＋Player 内部**:
    - `L382774 FUN_00617340`（item type switch の **case 6**）: `iVar4 = __RTDynamicCast(*(param_2[1]+8), src=TESObjectBOOK? no: src=0xB03248 TESBoundObject → dst=**`0xB084F0=.?AVTESObjectBOOK@@`**)` → `*(iVar4+100)!=0` ガード → `(**(*(param_1+0x3C))+0x2d0))(iVar4, param_3)`。**同 switch の case 7 は `MagicItem→AlchemyItem(0xB0320C)` → `FUN_005E08C0`（別経路）、case 0/2/3 は `FUN_00699190` 直呼び** → **slot+0x2D0 は「本（case6）」系統で使用**。
    - `L435005 FUN_00671620`: `(**(*param_1 + 0x2d0))(param_1[0x18a], 0)` — `param_1[0x18a]` = **`actor+0x628`**（`FUN_00664850` が保持する item）。
- **`FUN_004D8760`（vt+0x100）decompiled**（L170124–170147）: `if (param_2==0) return 0; iVar1=FUN_004d6d40(); if (iVar1) { FUN_004fbf90(param_1, param_3, 4); FUN_00485e00(param_1,iVar1); return FUN_0048fbc0(param_1,param_2,param_5,param_4,param_3,param_6..param_11); }` — **`FUN_004FBF90` の第 3 引数 = イベントマスク `4`**（GT §4「送出(set 側)・node+4 へ OR」と整合。mask4 のイベント名は GT §59.3 参照＝本タスク入力制約により未特定 [要深掘]）。
- **`FUN_00699190`**（L464472–）: `param_2` の `vt+0x18` 結果 1/4 で分岐 → `FUN_009832e6(param_2,0,`**`0xB03280=.?AVMagicItem@@`**`,`**`0xB03800=.?AVSpellItem@@`**`,0)` ＝ `dynamic_cast<SpellItem*>` → `param_1(=&this->+0x5C)` の `vt+4/0/0x34/0x3C/0x10` へ供給。
- **TD 名実測**: `0xB02F9C=TESForm` / `0xB031EC=BaseFormComponent` / `0xB03248=TESBoundObject` / `0xB084F0=TESObjectBOOK` / `0xB0320C=AlchemyItem` / `0xB03298=TESFullName` / `0xB036FC=TESObjectWEAP`。
- **`FUN_00664850`**（L427161–）: `if (arg && arg[+0x64]==0) {player+0x628=0; return;} player+0x628=arg; FUN_00664700(*(arg+0x64)+0x18)`。
- **`FUN_004D8D70`**: `add ecx,0x44`（**REFR+0x44 = ExtraDataList**・b27 §3 と一致）→ `FUN_0041E6F0`（EDL type **`0x1A`** の値 `node+0xC` を返す・0 引数）→ null なら `ret 8` で 0、否则 `jmp FUN_00487D20`（`FUN_00487C30(extra, form, formID)` → `FUN_00485E80(form,1,formID)`）。
- **`[g_thePlayer+0x6E0]` は同型**: `FUN_00666670` 内で `TESBoundObject→TESObjectWEAP` キャスト・`"%s %s"` 表示・`player->vt+0x100(player[+0x6E0],…)` を同一パターンで呼ぶ。

### 3.3 残差（次タスク渡し）

1. イベントマスク `4` のイベント名（GT §59.3 の該当行 — 入力制約により未参照）。
2. `arg[+0x64]` の構造（`+0x18` が MagicItem サブオブジェクトの**親構造の正体**＝enchantment 系 [有力]）。
3. `FUN_0048FBC0`（本体大物・11 引数）の完全同定。

---

## §4 残点 4: `0x36C`「状態 vs 種別」の裁定

### 4.1 訂正前 → 訂正後（ドラフト）

**訂正前**（handover L43）
> `vt+0x36C=GetFormType()→int`

**V121 訂正後（現行ドラフト）**
> | `+0x36C` | **process 状態（種別）バイト取得**（GetFormType ではない） | …

**V163 訂正後（裁定確定・GT 貼付用）**
> | `+0x36C` | **process 状態バイト取得**（**「種別」表記は削除**・GetFormType ではない） | High/MidHigh → `0x64B090` = `movzx eax,byte[ecx+0x11D]; ret`。**`[process+0x11D]` は状態（遷移値）で確定**: (a) ゲート `FUN_00629b70`（`FUN_006299f0+0x186`、`esi`=process）は `[0x11D]∈{0,4,9}` のときのみ続行、(b) 同一集合チェックが HighProcess `+0x2E0`（アクション状態 switch 内 case）にも重複（b27 §4.3）、(c) **reader 定数**: `vt+0x36C` ディスパッチ 150 箇所の直後 first-check = **`test eax,eax` 70 / `cmp eax,4` 40 / `cmp eax,9` 32**（+3/8/6 各 1）、(d) **writer 12 箇所はすべて HighProcess 更新系**で、値は **0（一括クリア `FUN_0064b400`）/ 1・6（imm `FUN_006564c0`）/ 4・9（`FUN_00654f10`: `neg bl; sbb bl,bl; and bl,5; add bl,4` → **入力 0→4・非0→9**）/ コピー（`FUN_0064b650`）/ vt 戻り（`FUN_00650030`）**。formType（`TESForm+4`・値域 0x12..0x55・非仮想）とは値域も所属も無関係 |

### 4.2 根拠（本セッション再走査・v121/b27 との整合）

- **writer 全走査**（`[reg+0x11D]` を全エンコーディングで capstone 線形走査、`v163_step8-A`）: オブジェクト書き込みは**12 箇所のみ**（esp+0x11d8 等の誤検出・CRT `setSBUpLow` を除外）＝ v121 の 12 件と一致。**新規発見**: `0x6553B5` の `neg bl / sbb bl,bl / and bl,5 / add bl,4` は **{4,9} を生成**、`0x656FC6` は直前に `cmp bl,4` — **ゲート値 4/9 が書込値に実在**（v121 の「0/1/3/6 系」は imm のみを見てレジスタ経由を見逃した分を補正）。
- **reader 定数**（`v163_step8-B`）: 150 ディスパッチの first check = `test eax,eax`×70, `cmp4`×40, `cmp9`×32, `cmp3/8/6`×1 ずつ → **{0,4,9} で 142/148**。
- **ゲート本文**（v121_step3 と一致・本セッション再掲）: `FUN_00629b70` は `call [esi->vt+0x36C]` ×3、`test eax,eax` / `cmp eax,4` / `cmp eax,9`、不一致は `xor al,al; ret 8`。`esi` は process（`[esi+0xC0]` 等が兄弟 `FUN_0064F6A0`（High +0x188）と同一）。
- **b27 §4.3 との整合**: `+0x2E0` の switch case が `[ecx+0x11D]∈{0,4,9}` を同じ集合で判定＝**同一フィールドを process 状態として扱う独立 2 経路**。「状態」で一致。
- **「種別」否定**: (i) formType は `TESForm+4` の非仮想 byte（`FUN_0046B7A0` が `movzx eax,byte[ecx+4]` で copy 表を索引）、(ii) process はフォームではない、(iii) 値 {0,1,4,6,9} は遷移で書き換わる（種別は不変）。

---

## §5 残点 5: `0xF8` / `0x188` / `0x250` / `0x2E4` の正体

### 5.0 対象整理

- `0x250` / `0x254` は **b27 §3 で解消済み**（GT 用整形文のみ §5.4 に提示）。
- 残り `0xF8` / `0x188` / `0x2E4` を本セッションで確定。

### 5.1 `+0xF8` — 訂正前 → 訂正後（ドラフト）

**訂正前**（`docs/RE_EXE_REPORT.md` §41.3 L7500 ＋ §41.2 L7445 の利用例）
> `| +0xf8 | 死亡状態取得 |` ／ `(vtable+0xf8)()`

**V163 訂正後（GT 貼付用）**
> | `+0xF8` | **EDL extra `0x14`（位置+回転）の保存セッター**（REFR 系・3 float 引数・`ret 0xC`。**死亡述語ではない**） | Actor/Character/Creature/Player/MobileObject/REFR/Projectile 系 12 vtable → `0x4DD1A0`: 引数 `(x,y,z)` がグローバル既定ベクトル `0xB3F9A8/AC/B0`（＝origin）なら **`this->vt+0x174`(GetPos) の自位置**、否则 引数の点を採用 → **EDL(`this+0x44`) extra `0x14` の `+0xC..+0x14` へ位置を書く**（`FUN_0041F7B0`、`ret 0x14`・out バッファにも複写）→ **`this+0x20..+0x28`（自回転）を `+0x18..+0x20` へ書く**（`FUN_0041F770`）。**三兄弟**: `+0xF0`=`0x4D8210`=**回転 getter**（extra0x14 `+0x18..+0x20` を out へ、`ret 8`）／`+0xF4`=`0x4D8230`=**位置 getter**（`+0xC..+0x14`）／`+0xF8`=上記 setter — TESObjectREFR 系に vtable 登録（本セッション実測）。系統別: TESForm 系 33 vtable=`0x60D0A0`(空) / Process 系 `0x4943A0`(`xor eax,eax;ret 4`)・High・MidHigh `0x64B2D0`（1 byte 引数: `arg==0`→`[this+0xF0]`、否则 `[this+0xF0]` が非 null 且 `FUN_00484E80(obj,0)` 真なら `[this+0xF0]`、否则 0 ＝**条件付きポインタ getter・別物**）/ TESCombatStyle `0x4A9730`=`fld[ecx+0x2C]` / DefaultCombatStyle `0x625A00`=グローバル float / Ni 系・Renderer 系は無関係 |

**根拠**: 本体全文（`ret 0xC`＝3 引数）・`FUN_0041F7B0/0041F770` decompiled（extra type `0x14` の pos/rot セッター、`FUN_0041E210(0x14)` 検索→無ければ `FUN_0041EA70` 生成）・getter 2 本（`0x41F6F0`=rot / `0x41F730`=pos）・**書き込み経路 2 系統**: (i) **リロード時 `FUN_0045C4F0`**（"Reference to be loaded does not have a cell or worldspace." を印字する配置解決、直接呼出し 4 件＝`FUN_00463D70/00464440/00465860`×2 ＝ ESM/セル配置系）、(ii) **実行時スナップショット**: `FUN_004DF820`（`IsActor==0` かつ base formType ∉ {0x1C,0x1E..0x20} のとき既定ベクトルで呼ぶ＝位置修復/検証系、`param_1[0xb..0xd]`＝`+0x2C` 位置の NaN/有限チェックを伴う）と `FUN_004E2160`。**読み手**: `FUN_0045BB30/0045E990/00460710`（"Error saving actor…" 保存系）等 **decompiled `+0xf0` 47 / `+0xf4` 66 行**＝配置・保存経路が大半。→ 用途は「**配置/保存位置の記録**」[有力]、正式名 [要深掘]。
**報告書の `(vtable+0xf8)()`（0 引数利用）は REFR 版（3 引数）と引数不整合** → §41.3 の当該行は process 版を指すか引数記載欠落かの再確認要（訂正候補）。

### 5.2 `+0x188` — 訂正前 → 訂正後（ドラフト）

**訂正前**（b14 §1.2）
> | **0x188** | 状態述語（int） | [未同定] | REFR 系 → 0x6F7070（0）ほか 5 impl |

**V163 訂正後（GT 貼付用）**
> | `+0x188` | **系統別・別物（二重意味）**: (a) **REFR 系 12 vtable = クラス述語（恒定値）** — Actor/Character/Creature/Player/MobileObject/Projectile 系 11 本 → `0x977C50`（`mov al,1; ret`）／ plain `TESObjectREFR` → `0x69D990`（`xor al,al; ret`）＝「**移動体なら 1・plain REFR は 0**」（`+0x190`=IsActor と異なり **Projectile は 1**・IsActor は 0）。(b) **Process 系 5 vtable = 経過量の加算＋パッケージ進行（2 引数・`ret 8`）** — Base `0x60D030` = `[this+4] += arg2`、Low/MidLow `0x648120` / High/MidHigh `0x64F6A0` = `[this+4]`（`[this+0xC0]`≠0 なら `[this+0xCC]`）に加算し負なら 0 にクランプ → `vt+0x184` のパッケージと `FUN_00673980(pkg+0x18) <= vt+0x180()` で進行 → `table[pkgType][vt+0x180()]==0x2C` のとき **イベント mask `0x400`** を送出。GT には「系統別に書く」注記が必須 |

**根拠**: vtable 全 17 本の実測分類（5 impl）・`FUN_0064F6A0` decompiled（L414613–414646）・`FUN_00648120/0060D030` 本文（`add [reg+4],eax; ret 8`）・decompiled `+0x188` 呼出し **197 行**のうち大半は 0 引数 bool（クラス述語利用）、2 引数（`(param_1,1)` / `(param_2,0xffffffff)` 等）が process 系利用。

### 5.3 `+0x2E4` — 訂正前 → 訂正後（ドラフト）

**訂正前**（b14 §1.2）
> | **0x2E4** | 装備/状態切り替えヘルパ（arg 付き・ret void） | [未同定] | …

**V163 訂正後（GT 貼付用）**
> | `+0x2E4` | **系統別・別物**: (a) **process 版 = 状態バイト `+0x11C` 取得** — High/MidHigh `0x64B080` = `movsx eax,byte[ecx+0x11C]; ret`（0 引数・**`+0x36C` の `[+0x11D]` の隣接フィールド**）、Base/Low/MidLow `0x6F7070`(0)。呼出し 17 行の比較値は **{0,1,3,5,6}**（`GetKnockedState` 系 b10 呼び出しと整合）。(b) **Actor 版 = 1 float 引数の状態更新ヘルパ** — `0x5F43E0`: `(&this->+0x5C)`（埋め込みオブジェクト）の `vt+0x30()` 非 0 かつ `process->vt+0x2D4()`（アクション伴随 dword、b27 §4.3）非 0 のとき、その `+0x68` 先の `+8` byte（`FUN_0051AC70` = `return *(byte*)(p+8)`）が **0x14..0x27** かを検査（**jumptable は byte 表 `0x5F448C` で 0x17..0x21 が通過**＝範囲判定に還元）→ 通過時 `process->vt+0x2B8()` 真で `FUN_005F4190` → 埋め込みオブジェクトの float セッター 2 本（`0x699C10`/`0x699080`）と `this+0x68` メンバ（`0x6A2280`）へ引数 float を反映、`ret 4`。不通過時は `embedded->vt+0x34(0)` |

**根拠**: vtable 9 本/3 impl 実測・`0x5F43E0` 全文（`v163_step2`）・switch テーブル実測（byte 表 `0000000101010101010101010101000000000000`＝index3..13 が 1）・`FUN_0051AC70` decompiled 1 行・decompiled `+0x2e4` 17 行。

### 5.4 `+0x250` / `+0x254` の GT 追記文（b27 §3 の GT 用整形・参考）

> | `+0x250` | **ExtraData float メーター getter**（Character/Player のみ `0x60FBC0` = type `0x3D` float を [0,1] clamp、Player は `+0x116` ガードの `+0x700` で上書き）。系統別: High=`0x6298D0`(byte `[+0x2B8]`) / Actor・Creature=`0x6BF3D0`(0.0f) / Base 系=`0x69D990`(0) |
> | `+0x254` | **同メーター setter**（1 float・`ret 4`、`0x60FC20`。犯罪イベント関数からも呼出し） |

---

## §6 残点 6: GT §4 slot 対応表の常設＋handover L43 非参照の運用化（**設計案・GT 本体は編集しない**）

### 6.1 目的

- slot 語義の正本を **`tmp/re/GT_CHEATSHEET.md` §4.1** に集約し、以後の slot 名は **GT 経由のみ**とする。
- 誤記の発生源 `tmp/re/handover_20261025/session_B_summary.md:43` を**参照禁止**にし、旧 4 語（GetFormID / RemoveFromList / GetFormTypeID / GetFormType）の再流入を断つ。

### 6.2 GT §4.1 の構成案（親セッションが GT に貼るときの骨子 — 本タスクでは未実施）

```
### 4.1 vtable slot 主要対応（系統別・V121/V124/V163 込み）

A. TESForm 系（+0x08/+0x0C/+0xA0/+0x90/+0xB4/+0xB8/+0xEC/+0x154 …）
   — コピー/比較/フラグ系を先頭に（+8=CopyFormAttrs、+0xC=IsDifferent、+0xB4/+0xB8=不在警告）
B. TESObjectREFR / Actor 系（+0x164/+0x170/+0x174/+0x18C/+0x190/+0x198/+0xF0/+0xF4/+0xF8 …）
C. Process 系（+8=レベル 0..3、+0x188=時間積算、+0x2C0、+0x2D0..+0x2E0=アクション状態族、
   +0x2E4/+0x36C=byte フィールド状態、+0x330/+0x334 …）
D. 二重意味ルール: 同一 byte offset でも系統が違えば別メソッド（0xEC/0x154/0x164/0x190/0x2C0/0x330/0xF8/0x188/0x2E4 …）
E. 共有スタブ注記: 0x6F7070(=0) / 0x69D990 / 0x977C50(=1) / 0x68F970 / 0x60D0A0 … 「存在＝使えるではない」
F. 罠: (1) capstone skipdata は 64KB チャンク境界の命令を .byte 化して落とす（呼出し数は skipdata=False/生バイトで確認）
       (2) capstone の小さい即値は 10 進表示（`startswith("0x")` フィルタの罠）
       (3) 誤記は handover L43 系から再生する — GT §4.1 が唯一の正本
```

### 6.3 運用ルール案（GT §6「既知の作業ルール」への追記文 — 未実施）

> - **slot 名は `GT_CHEATSHEET §4.1` 経由で only**。`tmp/re/handover_20261025/session_B_summary.md:43` の slot map は**旧記載（V121 で 4 件訂正済）＝参照禁止**。handover は履歴資料であり GT が正本。
> - 新しい slot を確定したら **§4.1 に追記**し、§5 訂正履歴に 1 行残す（V番号＋出典ファイル）。§4.1 未収録の slot 名を成果物に書かない。
> - 同一 offset の系統混在は **A/B/C 別行**で書く（1 行に 1 系統）。stub impl は「恒定値」列を併記。

### 6.4 反映手順（承認後・親セッション用のチェックリスト草案）

1. `GT_CHEATSHEET.md.bak1` を作成（現行 .bak 0 件）→ LF/BOM なし UTF-8 維持。
2. §4 `### クラス / vtable` 表の直後に **§4.1** を新設（上記 A–F、§1/§2/§3/§4/§5 の「訂正後」行を貼付）。
3. §5 訂正履歴に `2026-10-28 | V163: …` 行を追加（本文テンプレは §8.3）。
4. §6 作業ルールに §6.3 の追記文を追加。
5. `round_verify.py` → `ALL OK`＋GT は手動で UTF-8(LF)/BOM なし/行数確認。

---

## §7 新規残点（起票粒度）

| 起票案 | 内容 | 優先度 |
|---|---|---|
| V-163a | **`FormIDToString` 実装 `0x47E190`（未登録）の契約確定**: 実引数が formType byte なのに警告文は FormID — formID 経路（下位 byte 起点？）と型名返却の関係、`FUN_0044CDF0` の 4 呼出しのログ形式、`FUN_0047D640` jumptable の戻り値集合 | 中 |
| V-163b | **EDL extra `0x14` の用途名確定**（配置/保存位置 [有力]）: `FUN_0045C4F0`（ロード時書込）と `FUN_004DF820/004E2160`（実行時スナップショット、base formType ∉{0x1C,0x1E..0x20} ガード）の意味、getter 系（`+0xF0/+0xF4`）の主要読み手の特定 | 中 |
| V-163c | **`0x2D0` 一般名の最終確定**: イベントマスク `4` のイベント名（GT §59.3 参照）＋`arg[+0x64]` 先の構造（`+0x18`=MagicItem の親＝enchantment 系 [有力]）＋`FUN_0048FBC0`（11 引数大物）の同定 | 中 |
| V-163d | **`[process+0x11D]` の状態値 0/1/4/6/9 の意味**（ゲート {0,4,9}=許可、`FUN_00654f10` が {4,9} を生成）と HighProcess `+0x11C`（slot+0x2E4 版の値 {0,1,3,5,6}）との関係 | 低 |
| V-163e | **`FUN_004133C0` のガード `base[+0x58]` bit7 の意味**（GetFormID を 0 化する base フラグ）と `HighProcess+0xF0`（slot+0xF8 process 版が返すオブジェクト）の正体 | 低 |
| （継続） | v121 V147（§4 候補 A–F の GT 訂正: `+0x170/+0x164/+0xEC/+0x2C0/+0xF8/+process+8`）— 本タスクでは `+0xF8` のみ §5.1 で実体確定、他は未実施 | **高**（親セッション） |
| （継続） | b14 §5-6 / V-27g の反映（b14 は編集禁止） | 低 |

---

## §8 証跡・自己検証

### 8.1 証跡（すべて `%TEMP%\opencode\`・既存 docs 非変更）

| ファイル | 役割 |
|---|---|
| `v163_step1.py` / `.txt` | TESForm `+0x0C` 本文・FUN_0046b7a0 ブロック・4 候補の vtable/rel32/imm32・ASCII 検索・slot 0xF8/0x188/0x2E4 の全 impl 分類 |
| `v163_step2.py` / `.txt` | TD 名10 種・`0x46B870` 全文・4 候補本文・`mov eax,[ecx+0xC]` 168 件のフィルタ・key impl 全文・decompiled `FUN_005e4440/004d8d70/005f43e0` |
| `v163_step3.py` / `.txt` | **+8 厳格ディスパッチ 843 の関数別分類＋文字列ヒント**・decompiled `+ 8))(`1,487 行・GetFormID 候補の decompiled 呼出し・vtable メンバシップ・copy-family 本文 |
| `v163_step4.py` / `.txt` | **+8 呼出しの引数個数分類（625/217/1）**・`0x46C697`(vt+0x90 clear)・`FUN_00404E50`(strlen)・`FUN_0041E6F0/00487D20`・`FUN_0088D370` 呼出し文脈14 件・formID 書き込み/`>>0x18` 検索 |
| `v163_step5.py` / `.txt` | **`FUN_0088D370` 全 31 呼出しの ecx 設定（全件 `+0x1E0`）**・`FUN_004133C0/005204C0` 本文・TD `TESObjectREFR/MobileObject`・`+ 8))(` 分類・copy/compare dispatcher xref |
| `v163_step6.py` / `.txt` | ASCII FormID 4 件の文脈・digit getter decompiled・`FUN_00448D40` アロケータ・`0x4D8D70` 連鎖・slot 0x2D0 ディスパッチ 37 件・`0x4D8760` 本文/decompiled |
| `v163_step7.py` / `.txt` | `FormID` 文字列生バイト・`FUN_004CA5F0/004CA640`・`base+0x58` 読手・decompiled `+0x2d0` 36 行・`FUN_0048FBC0` 先頭 |
| `v163_step8.py` / `.txt` | **`[reg+0x11D]` 全走査（writer 17→実12＋誤検出）・`vt+0x36C` reader 定数ヒストグラム**・`0x4DD1A0` 全文・`0x188` 全 17 vtable・`0x2E4` switch テーブル・`FUN_0051AC70`・`+0x2e4` 呼出し 17 行・`FUN_00617340/00671620` 文脈 |
| `v163_step9.py` / `.txt` | **TD `0xB084F0=TESObjectBOOK`**・`+0x11D` レジスタ書き込み8 件の値文脈（**`neg/sbb/and5/add4`→{4,9}**）・`test/cmp` 集計・`FUN_0041F7B0/0041F770` decompiled（extra0x14 セッター）・`FUN_00617340` 先頭 |
| `v163_step10.py` / `.txt` | ASCII FormID 生バイト4 件・decompiled `+0xf8` 47 / `+0x188` 197 行・EnchantmentItem TD 検索 |
| `v163_step11.py` / `.txt` | **`formEnumString…cFormID in TESForm.cpp`/`pActor->iFormID = %08X` の実 VA と参照**・`HighProcess+0xF8` else 経路・extra0x14 ファミリ 5 サイト・`FUN_004DF820` 文脈 |
| `v163_step12.py` / `.txt` | FormIDToString 文字列 walkback・**EDL extra0x14 の get/set4 本**・`0x41F6F0` 呼出し |
| `v163_step13.py` / `.txt` | **extra0x14 アクセサの vtable 登録（`+0xF0`/`+0xF4` 三兄弟）**・setter 呼出し（`FUN_0045C4F0`/`FUN_004DD130`）・slot+0xF8 呼出し文脈 |
| `v163_step14.py` / `.txt` | `FUN_0047E140`(rand) と0x47E25F の不整合検出・`FUN_0045C4F0`（配置解決）・vtable 登録全走査 |
| `v163_step15.py` / `.txt` | `0x47E25F` が real xref（`table[formType]` 積み→警告印字）・decompiled `+0xf0` 47/`+0xf4` 66 行 |
| `v163_step16..19.py` / `.txt` | **未登録領域 `0x47E190..0x47E27A`（FormIDToString 実装候補）の全文・`FUN_0047D640`・表 `0xB081D0="Activator"`/`0xB06870="Race"`・4 呼出しの文脈（`FUN_0044CDF0`）** |

再現: `python "%TEMP%\opencode\v163_stepN.py"`（順不同・読取のみ）。

### 8.2 自己検証

| 検証項目 | 期待 | 結果 |
|---|---|---|
| `probe_harness.py selftest` | 4/4 PASS | **4/4 PASS**（着手時: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |
| +8 厳格ディスパッチ数 | v121=843 | **843 で一致**（本セッション独立再走査） |
| `[reg+0x11D]` writer 数 | v121=12 | **実 12 で一致**（esp/CRT 誤検出を除外後） |
| slot 0x188 / 0xF8 / 0x2E4 の vtable 分類 | — | **17 / 56 / 9 本**を全数実測（b14 の「0x188=REFR→0x6F7070」は実測と相違＝`0x69D990`、b14 訂正候補） |
| TESForm vtable `+0x08/+0x0C/+0xB4/+0xB8` | v121_step2 | **一致**（`0x46B800/0x46B870/0x46B7A0/0x46B7D0`） |
| formID=+0xC | — | **4 系統の独立証拠**（桁抽出/アロケータ/逆経路/文字列）で一致 |
| 禁止事項（docs/・GT・TODO・session-memory・plan・cache・git の編集） | 0 件 | **遵守**（新規は本 1 ファイル＋`%TEMP%\opencode\v163_*` のみ） |

### 8.3 GT §5 訂正履歴に貼る 1 行（テンプレ・未実施）

```
| 2026-10-28 | V163: slot 残点6件を確定 — `+8`=**フォーム属性同期ヘルパ CopyFormAttrs**（`0x46B800`、同ブロックに copy/compare 警告 `+0xB4/+0xB8`・`IsDifferent` `+0xC`）/ **GetFormID=仮想関数なし＝`TESForm+0xC` 直接アクセス**（4候補否定: `0x88D370`=[obj+0x1EC]・`0x943410`=TESEnchantableForm+0x10・`0x4133D1`=ガード付きgetter・`0x5204C9`=別物、文字列化=未登録 `0x47E190`）/ `+0x2D0`=**アイテム使用系アクション起動**（arg=TESBoundObject 系・event mask4・`+0x628` 保持）/ `+0x36C`=**状態バイト確定**（ゲート{0,4,9}=書込値に実在 `FUN_00654f10`）/ `+0xF8`=**EDL extra 0x14 の位置+回転保存**（`+0xF0/+0xF4` 三兄弟）・`+0x188`=REFR 系クラス述語 vs Process 系時間積算・`+0x2E4`=process `+0x11D` 隣接 `+0x11C` 状態取得＋Actor 版 float 更新。出典 `tmp/re/v163_gt_slot_semantics.md` |
```
