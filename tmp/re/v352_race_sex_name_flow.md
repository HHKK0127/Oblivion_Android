# V352: RaceSexMenu 名前入力（race_name / id=10）のエンジン側保存先裁定（#123）

- 日付: 2026-11-11 / タスク: ai-todo **#123**（#97 §4.2 の未確定1件・main 起票） / 作成: RE レーン（opencode 直接解析）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- 手法: capstone 5.0.7（v307_common.py 読取形）／RTTI TD→COL→vtable 走査（v350 補正法）／プロパティ機構の set/get 逆展開／vtable slot 実読
- 中間証跡: `%TEMP%\opencode\v352_1..v352_34.py/.out`（**全 EXIT=0**。v352_10 初回のみ .bss 読取例外 → ガード追加で再実行）
- 制約: git / docs / GT / TODO 台帳・session-memory / 既存 `tmp/re/*.md` は**無編集**（本ファイル新規1件のみ）、ESM 未アクセス、L1

---

## §0 判定サマリ（#123 依頼4点）

| # | 依頼（v351 §4.2） | 判定 |
|---|---|---|
| ① | RaceSexMenu vtable 特定 | **[確定] vtable = 0xA6D38C（15 slots・RTTI 0xB13348・COL 0xAC2AF4）**。b07_vtable_index 行1198 と一致 |
| ② | id=10 ハンドラ特定 | **[確定] SetElement slot01 `0x5C26D0` が `id==0x0A → [this+0x30]` に race_name 要素ポインタを格納**。消費ハンドラ = 主ハンドラ `0x5CC200`（slot03）。id=90(Done) も同関数で `→ [this+0x3C]` |
| ③ | 保存先裁定 | **UI 側 [確定]**: 要素プロパティ **0xFDE(u16 4062)**。**engine 側 [確定(命令)]**: `0x5C30C0` が `g_thePlayer → vfunc+0x170()`（= **`[player+0x1C]`**）の結果の **`+0xA4` へ editor 文字列を 0x4028D0 で Set**。getter = **`0x4DA2A0`（TESFullName cast → +4）**。**`+0xA4 = TESFullName+4 = name char*` は [推定]**（レイアウト完全整合だが dynamic_cast の戻りオフセットが実行時計算） |
| ④ | Done(id90) コミット経路 | **[確定]**: `0x5CC277: cmp ebx,0x5A → jne` → `0x5C3110`（key→members 対応表）→ 0xFAE float 比較 → `jnp 0x5CCA77`（name 検証）→ 成功時 race 説明メッセージ表示 → `0x5CD9C5` epilogue（vfunc+0x154 呼び出し）。**リアルタイム書出は通知のたびの `0x5CC272 → 0x5C30C0`** [確定] |

**副次同定（追加価値）**:
- `0x9832E6` = **dynamic_cast 相当（cdecl 5引数: obj, 0, pTDBase, pTDDerived, 0 → 返り = cast 結果 or 0）** [確定]（TD は .data 側 TypeDescriptor、b07 §3.1 と整合）
- `0x4028D0` = **文字列 Set メンバ（std ret 8・this=ecx: {+0: char* buf, +4: u16 len, +6: u16 cap}・557 呼び出し）** [確定]
- `0x4DA2A0` = **フォーム名 getter**: `[g_thePlayer+0x1C]` → TESFullName dynamic_cast → `[+4]`（char*・空フォールバック 0xA2F7EC ""）[確定]。対の length getter `0x4D9890` は `[+8]` u16（0xFFFF なら strlen キャッシュ無効）[確定]
- `PlayerCharacter vfunc+0x170 (slot92) = 0x4D9B40 = mov eax,[ecx+0x1C]; ret` [確定] → **getter の入力と setter の入力が同一オブジェクト** [確定]
- **RaceSexMenu は TextEditMenu を使わない**（0x5DCF20 の caller は ClassMenu 系 0x5977FE の1件のみ）→ v351 §4.1 の記載と一致 [確定]。name 入力は **RaceSexMenu 内蔵テキストエディタ [this+0x8EC]** [確定]

---

## §1 依頼①: RaceSexMenu vtable [確定]

### 1.1 RTTI 同定

- 文字列 `Data\Menus\CharGen\race_sex_menu.xml` = **0xA6973C**（u32 xref = `0x5CA066` のみ）
- RTTI TD `.?AVRaceSexMenu@@` = **0xB13348**（name @ +8、VT=0xAA3E80 単一・b07 §3.1 一致）
- 補正 COL 走査（TD=name−8）→ vtable **0xA6D38C**（vtable[-1]=0xAC2AF4=COL）

### 1.2 vtable 全 slot（v352_20）

| slot | +off | fn | 内容 |
|---|---|---|---|
| 00 | +0x00 | 0x5C4E20 | dtor/基底委譲（call 0x5C4BF0） |
| 01 | +0x04 | **0x5C26D0** | **SetElement(id, ptr)** — §2 |
| 02 | +0x08 | 0x5C2790 | スライダー/マウス追従（[esi+0x94+idx*4]・+0x898/+0x89c へ float） |
| 03 | +0x0C | **0x5CC200** | **メイン通知ハンドラ**（name 検証・Done 処理を含む・§4/§5） |
| 04/05 | +0x10/14 | 0x60CF60 | stub（ret 8） |
| 06/07 | +0x18/1C | 0x890B90 | stub（ret 0xC） |
| 08 | +0x20 | 0x5C3BE0 | （未深掘・残点 V-352-5） |
| 09 | +0x24 | 0x60CF60 | stub |
| 10 | +0x28 | 0x5C2870 | [this+0x34] 時の時間計算（0xFB7 float set） |
| 11 | +0x2C | 0x5CE570 | `push 0; push 0x3E9(1001)`（未深掘・残点） |
| 12 | +0x30 | 0x5C3DD0 | `[this+0x8EC]` へ arg 文字列 set → `0x5C30C0` 呼び出し（§3.3） |
| 13 | +0x34 | 0x5C4E10 | `mov eax,0x40C; ret` = **GetMenuID() → 0x40C**（v351 と一致） |
| 14 | +0x38 | 0x725DA0 | stub（xor al,al; ret 8） |

---

## §2 依頼②: id=10 ハンドラ [確定]

**`0x5C26D0`（slot01・stdcall ret 8・arg1=[esp+4]=id, arg2=[esp+8]=値）**:

```
id==0x01 → [this+0x28] = arg2
id==0x02 → [this+0x2C]
id==0x0A(10) → [this+0x30]   ← race_name 要素（v351 §4.1 の「id 取得推定」を確定に昇格）
id==0x14(20) → [this+0x34]
id==0x15(21) → [this+0x38]
id==0x5A(90) → [this+0x3C]   ← race_button_done（Done ボタン）
```

- **id=10 の取得元**: メニュー XML の `<id> 10 </id>` を base Menu が SetElement 通知として配布する方式（`push 10` 形式のコードは chargen 帯に不在＝v351 の probe 再現。id はハードコードされず通知引数で届く）
- **id=10 の消費**: 主ハンドラ `0x5CC200` 内で **`mov ecx,[esi+0x30]`** が name 要素の参照（§4.1）
- **id=10 分岐（通知受信）**: `0x5CC8A3: cmp ebx,0xA → jne 0x5CC277; call 0x5C2730; call 0x5C30C0`
  - `0x5C2730`: `[this+0x8EC]`（内蔵 editor）を空文字列でクリア → race_name 要素の float プロパティ 0xFD3/0xFD4（＝位置）を取得して editor を配置 → visible=1
  - `0x5C30C0`: §3.3 の書出処理

---

## §3 name の保存先（UI 側）[確定]

### 3.1 プロパティ機構（v352_6/7/8）

- **`0x58CED0` = set(u16 id → char\*)**（stdcall ret 8・[ecx+0x18] 先頭から node+0x18=id 比較・node+8=値・未存在=0xA2F7EC ""）
- **`0x588C10` = get(u16 id) → char\***（ret 4）
- **0xFDE(4062) = 「表示文字列」共通プロパティ**（text/label 要素が全員所持。race_name 要素・TextEditMenu・各ラベルが同一 id を使用）
- **TextEditMenu(0x41B) のコミット**: slot11 `0x5DD120` / slot12 `0x5DD160` が **this+0x34 バッファ → this+0x28 のターゲット obj へ 0xFDE set**（v352_5）。初期値も target の 0xFDE get → 0x57FF20 で this+0x34 へコピー（0x5DCF20 内 0x5DD06A）

### 3.2 RaceSexMenu での name 入力

- **入力 UI = 内蔵テキストエディタ `[this+0x8EC]`**（TextEditMenu は ClassMenu 専用・0x5DCF20 caller は 0x5977FE の1件のみ）→ v351 §4.1 と一致
- id=10 通知で `0x5C2730` が editor を race_name 要素位置へ表示

### 3.3 反映（UI↔UI）と engine 書出 — **`0x5C30C0`（本裁定の核心）**

```asm
5C30C0: mov ecx,[esi+0x8EC]        ; 内蔵 editor
5C30CA: call 0x580120              ; eax = editor の現在文字列
5C30CF: mov ecx,[esi+0x30]         ; race_name 要素 (id=10)
5C30D2: push eax / push 0xFDE / call 0x58CED0   ; ① UI 表示へ反映
5C30DD: mov ecx,[0xB333C4]         ; g_thePlayer
5C30E5: call [vtable+0x170]        ; eax = [player+0x1C]（§6 で確定）
5C30F3: mov edi,eax                ; edi = character オブジェクト
5C30F5: call 0x438DB0              ; eax = [editor+0x18] = 入力文字列 char*
5C30FA: push 0 / push eax
5C30FD: lea ecx,[edi+0xA4]         ; ② engine 側オブジェクト +0xA4
5C3103: call 0x4028D0              ;   文字列 Set（§3.4）
```

- **callers は3件のみ**: `0x5C3DF4`（slot12・外部からの文字列 set 後に即反映）／`0x5CC272`（**通知のたびに毎回実行**＝リアルタイム書出）／`0x5CC8B5`（id=10 分岐）
- **chargen 帯で `lea [r+0xA4]` + 文字列 Set を行う箇所は 0x5C30C0 の1件のみ**（v352_34 全走査。他 +0xA4 アクセスは element 参照 `mov ecx,[esi+0xA4]` → `0x58CEB0` set_float＝別オブジェクト）[確定]

### 3.4 `0x4028D0` = 文字列 Set [確定]

```
thiscall/std ret 8: ecx=this, [esp+8]=src(char*), [esp+0xC]=maxlen
strlen(src) → len → max(len, maxlen) → this+6(cap) 以下なら copy、超過で realloc
this+0 = char* buf / this+4 = u16 len / this+6 = u16 cap
空指定なら free してリセット（0x401F20 = operator delete）
```

### 3.5 比較元グローバル `0xB39440` [部分確定]

- `0x5CCAE2: mov edi,[0xB39440]` → **入力 name と strcmp**（= 「既存名」との一致検査）
- **.text への u32 xref は4件のみ**: 読 2（0x5CBC62 / 0x5CCAE4）＋ **thunk 2**（`0xA22FA0: mov ecx,0xB39440; jmp 0x4055E0`＝グローバル文字列オブジェクトの dtor/機構系。0xB39430/438/440/448 が8バイト間隔で並ぶ）→ **実行時の書込サイトは .text に存在しない** [確定負（静的走査）]
- 0xB39440 は **.bss（セクション外・静的読取不可）** → 値の初期化元は未特定（残点 V-352-3）

---

## §4 engine 側保存先（依頼③の核心）

### 4.1 getter `0x4DA2A0` [確定]

```asm
4DA2A0: mov ecx,[ecx+0x1C]                 ; this(player)+0x1C のオブジェクト
        push 0 / push 0xB03298 / push 0xB02F9C / push 0 / push ecx
        call 0x9832E6                      ; dynamic_cast(obj, 0, TESForm, TESFullName, 0)
        test eax,eax / je → 0xA2F7EC("")   ; fallback 空文字列
        mov eax,[eax+4] / ret              ; **TESFullName+4 = name char***
```

- TD 名（v352_19/24）: `0xB02F9C` = **`.?AVTESForm@@`**、`0xB03298` = **`.?AVTESFullName@@`**（他: 0xB0A020=`.?AVMenu@@`、0xB13B5C=`.?AVTextEditMenu@@`）
- **対の length getter `0x4D9890`**: 同 cast → `movzx ecx,[eax+8]; cmp cx,0xFFFF` → 0xFFFF なら `[+4]` の strlen、否则キャッシュ値を返す → **TESFullName レイアウト = {+4: char\* name, +8: u16 len}** [確定]
- v351 §4.2 の「フォーム名 getter 0x4DA2A0（244 呼び出し）」の記述と一致（本裁定で **TESFullName::name と確定**）

### 4.2 入力オブジェクトの同一性 [確定]（v352_33）

- **PlayerCharacter vtable = 0xA73A0C（b07 行1170・239 slots）**
- **slot92 (+0x170) = `0x4D9B40`: `mov eax,[ecx+0x1C]; ret`** ← **vfunc+0x170() は `[this+0x1C]` を返す**
- Character 基底 vtable 0xA6FC9C も同一 slot 値（継承一致）
- → **`0x4DA2A0` の入力（`[player+0x1C]`）と `0x5C30C0` の書出先 obj（`vfunc+0x170()`）は同一オブジェクト** [確定]
- slot100 (+0x190) = `0x977C50: mov al,1; ret`（0x4D7F80 のゲート。+0x170 結果の `[+4]==0x23` チェック付き）

### 4.3 保存先の同定

- 書出: `obj+0xA4` ← 0x4028D0（{char\*, u16 len, u16 cap} 形式）
- 読出: `cast<TESFullName>(obj)+4` ← char\*
- **整合解釈: TESFullName が obj+0xA0 に埋め込まれている場合、TESFullName+4 = obj+0xA4 = name char\*、TESFullName+8 = obj+0xA8 = u16 len（=0x4028D0 の this+4 と一致）** → **両者完全整合**
- **判定: `[character obj +0xA4]（= TESFullName.name）が engine 側保存先` [推定]** — vfunc+0x170==[player+0x1C] と 0x4028D0 の文字列形式は [確定]、TESFullName の埋め込みオフセット 0xA0 のみ dynamic_cast（0x9832E6）が実行時 RTTI で計算するため静的証明不可

### 4.4 負の証拠（静的走査）

- **chargen 帯（0x5C0000-0x5E0000）に `cast<TESFullName> → [+4] store` は0件**（0x9832E6 全1,819 サイトのうち store 付き169件はすべて0x40-0x51xxxx の ESM/form 構築系）
- **SetName 形式（[x+4] dword store + [x+8] u16 store）70候補も chargen 帯0件**（すべてリテラル/ctor 複写）
- → **chargen 内の name engine 書出は `0x5C30C0` 経由のみ** [確定]

---

## §5 依頼④: Done(id90) のコミット経路 [確定]

主ハンドラ `0x5CC200`（ret 8・[ebp+8]=通知 id = ebx）:

```
入口: [this+0x8EC] 有効？ → je 0x5CC8A3（id=10 判定へ）
      else editor 非表示 → call 0x5C30C0（現状反映）→ 0x5CC277
0x5CC277: cmp ebx,0x5A(90) → jne 0x5CC8BA（= id=10 以外の通知はデフォルト処理へ）
Done(90) 分岐:
      [0xB38F70] の key → 0x5C3110（key→members 対応表: 0xB38F70→[this+0x40]、0xB38F78→[+0x44]、0xB38F88→[+0x48]、他 [/+0x4C]/[+0x50]）
      → 0xFAE float を2つ比較 → jnp 0x5CCA77（= name 検証バンド）
      （不一致なら次の key 0xB38FE0/0xB38FE8 で再試行 → いずれも 0x5CCA09/0x5CCA77）
```

### 5.1 name 検証バンド `0x5CCA77`

1. `[esi+0x30]` の 0xFDE get → **strlen** → 空なら 0x5CCADF
2. **`strcmp(name, [0xB39440])`**（既存名と一致なら `je 0x5CCBB7`＝エラー通知）
3. 空白を含むなら flag → エラー通知 `0x5CCBB7`（`0x579C10` + 文字列 0xB39438）
4. **成功系** `0x5CCB0E`:
   - `g_thePlayer → 0x5E32F0`（= `[vfunc+0x170()]+0xE8` を返す getter）→ `+0x18 → [+4]` を**読む**（race 名文字列）
   - 先頭文字の**母音判定**（aeiouAEIOU）→ `[0xB38660]`/`[0xB38668]`（"a "/"an "）→ sprintf `0xA6B4D8` → **`0x579C10` でメッセージ表示**（callback `0x5C2BA0`）
   - → **name 検証成功時の engine 書出は0件**（表示のみ）。**engine への保存は §3.3 のリアルタイム書出（0x5C30C0）が担う** [確定]

### 5.2 完了

- `0x5CD9C5`: `[0xB3B4C9]`/`[0xB3B4C8]` クリア → `g_thePlayer vfunc+0x154`（= `0x6600F0`）→ `0x7B4280`/`0x7C5E70`（管理系）→ `ret 8` epilogue
- **name の engine 書出はこの経路に無い**（§4.4 の負の証拠と一致）

---

## §6 表示同期（engine → UI）[確定]

`0x5CBBE0..0x5CBC74` 帯（v352_16）:

```
cmp [0xB14500],0 → jne skip
g_thePlayer → call 0x4DA2A0（name getter）→ strcmp vs [0xB3B4D0]（キャッシュ）
  差分あり → strcmp vs 0xA6D3E8 "Bendu Olo"（デフォルト名マーカー）→ 差分ありなら
    g_thePlayer name → 0xB3B4D0 へバイトコピー（ループ movsb 等価）
mov ecx,[0xB39440] → push → [esi+0x30](race_name 要素) → push 0xFDE → set
set float([esi+0x30], 0xFF0, 1.0) → call 0x5C7BA0(push 1)
```

- **`[esi+0x30]` は RaceSexMenu handler の name 要素参照（§2）と一致** [確定]
- 0xB3B4D0 は .bss のキャッシュバッファ（静的読取不可）

---

## §7 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧（%TEMP%\opencode） | v352_1..v352_34 **全 EXIT=0**（v352_10 初回のみ 0xB39440=.bss で例外 → safe_str ガードで是正） |
| 2 | RaceSexMenu vtable | RTTI 0xB13348（hex `.?AVRace...`）→ COL 0xAC2AF4 → vtable 0xA6D38C・slot13 `mov eax,0x40C` = b07 行1198 と一致 |
| 3 | id=10/90 の二重確認 | SetElement（0x5C26D0）と通知分岐（0x5CC8A3/0x5CC277）の**両方**で id が一致 |
| 4 | dynamic_cast の引数 | 0x9832E6 の5引数形（obj,0,TDBase,TD Derived,0）を TextEditMenu 呼び出し（0xB13B5C）で既知照合 → 0x4DA2A0 の引数（TESForm/TESFullName）へ適用 |
| 5 | 入力 obj の同一性 | PlayerCharacter slot92 = `mov eax,[ecx+0x1C]; ret`（Character 基底と同値）＝vfunc+0x170 と [player+0x1C] の一致を vtable 実読で確定 |
| 6 | 文字列 Set の形式 | 0x4028D0 全文（strlen/cap/copy/free・{char\*,u16,u16}）と 0x4D9890（TESFullName {+4:char\*, +8:u16}）の**レイアウト整合**を照合 |
| 7 | 負の証拠の網羅性 | 0x9832E6 全1,819サイトの store 検出（169件＝ESM系）／SetName 形式70候補（chargen 0件）／+0xA4 アクセス360件の ModRM 全網羅（mod=01/02・SIB 分岐修正後） |
| 8 | TextEditMenu 不使用 | 0x5DCF20 の caller 探索（E8 全走査）で ClassMenu 系0x5977FE の1件のみ → v351 §4.1 と一致 |

---

## §8 新規残点（次束用）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-352-1 | [推定] `character obj +0xA0 = TESFullName 埋め込み` の静的証明（dynamic_cast 戻りオフセット 0xA0）。0x9832E6 のオフセット計算ロジック解析、または実行時 dump で確定 | **中** |
| V-352-2 | [未確定] グローバル `0xB39440`（char\*/文字列オブジェクト）の**書き手**（.bss・.text 書込なし＝初期化ルーチン or thunk 経由）と初期値 | 中 |
| V-352-3 | [未確定] `0xB3B4D0`（name キャッシュバッファ）の書き手（0x5CBC42 のバイトコピー以外）とサイズ | 低 |
| V-352-4 | [部分確定] `0x5E32F0` = `[vfunc+0x170()]+0xE8` → `+0x18 → [+4]` の obj（race 名文字列と解釈・母音判定文脈）— +0xE8 フィールドの同定が未了 | 低 |
| V-352-5 | [未確定] RaceSexMenu slot08 `0x5C3BE0` / slot11 `0x5CE570`（0x3E9=1001）の役割 | 低 |
| V-352-6 | [部分確定] `0x5C7BA0`（name反映後のヘルパ・vfunc+0x170 → `+0xE8+0xA8` 参照）の完全解 | 低 |

---

## §9 main への報告（v351 §4.2 の更新材料）

1. **§4.2「エンジン保存側 [未確定]」は本裁定で [確定(命令)] に更新可能**: 保存先 = **`[g_thePlayer → [player+0x1C] が指す character オブジェクト → +0xA4]`（TESFullName.name）**、書出 = **`0x5C30C0`（通知のたびに実行）**、getter = **`0x4DA2A0`**。
2. **v351 §4.1 の「TextEditMenu はカスタムクラス名専用・プレイヤー名には使わない」を再確認**（0x5DCF20 caller = ClassMenu の1件のみ）→ name 入力は **RaceSexMenu 内蔵 editor [this+0x8EC]**。
3. **id=10 推定の確定**: `SetElement 0x5C26D0` の `id==0x0A → [this+0x30]`。id=90(Done) → `[this+0x3C]`。
4. **port 側（§4.3 `createdCharacter_.name` handoff）との接続点**: engine 保存先（TESFullName.name）＝port の `CharacterStatus.name` に相当する原典フィールド。既定名 "Adventurer" は 0xA6D3E8 "Bendu Olo" とは別（0xB38658 等の比較先は残点 V-352-2 と接続）。
5. **#101 への材料**: name 入力はリアルタイムで engine へ反映される（Done を待たない）。Done(90) は検証＋メッセージ＋画面遷移（vfunc+0x154 → 0x7C5E70）。

---

### 参照（本タスクの証跡）

- `%TEMP%\opencode\v352_1..4.py/.out`（文字列探索・RTTI/COL 走査・vtable 初出）
- `v352_5/6/7/8/9`（TextEditMenu slot・0xFDE set/get 全136/20サイト・プロパティ機構 0x58CED0/0x588C10）
- `v352_10/11/12`（name 検証バンド・fn 0x5DD340・SetElement 初出・0xFDE 全 inventory・0x5DCF20 callers）
- `v352_13/14/15`（TextEditMenu 生成シーケンス・RaceSexMenu ハンドラ call 列挙・0xB39440/438 xref）
- `v352_16/17/18/19`（engine→UI 同期バンド・0x4DA2A0 getter 全文・TD 名確定・thunk 0xA22FA0）
- `v352_20/21/22/23`（RaceSexMenu vtable 全15 slot・SetElement 全文・通知分岐 cmp 列挙・Done/id=10 分岐本文）
- `v352_24/25/26/27/28`（TESFullName TD サイト全走査・0x9832E6 全1,819サイト store 検出・0x5DBB00（engine→UI 反映 fn）・SetName 形式70候補・0x46A5F0 非該当）
- `v352_29/30/31`（0x4028D0 前段・0x438DB0=editor+0x18 getter・0x5C30C0 callers 3件・0x4D7F80 gate）
- `v352_32/33/34`（**PlayerCharacter vtable slot92 = [this+0x1C]**・+0xA4 アクセス全走査（ModRM 修正版））
