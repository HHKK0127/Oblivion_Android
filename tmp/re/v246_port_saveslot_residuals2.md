# v246_port_saveslot_residuals2.md — 移植セーブ経路残点第2束（N-28..N-31 裁定）

- 作成: 2026-11-02 / 担当: TODO serial 239（V239・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v244_port_saveslot_residuals.md` §5 新規残点 N-28..N-31（本体は読取のみ・無編集）。
- 手法: capstone 5.0.7 再逆アセンブル（PE 手動解析）＋ `docs/Oblivion_decompiled.c` 部分読取＋ `tmp/re/cache/` 突合。ESM 本体は未アクセス。git 不使用。
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。
- 状態: 分析中（項ごとに即時書込み）。

---

## §1 項目1（N-28）: Script/BirthSign のみ slot11 が 0x519320 サンクを持つ理由

**判定: 【実装意味は解明／ソース級の設計動機は未解明】2クラスの特殊性は、TESForm 基底 slot11 が持つ「form flags bit5(0x20) 時はペイロードを書かない削除レコード系の特別経路」を通らず、常に slot8 の通常フォーム書き出しへ委譲する点にある。したがって bit5 が立った Script/BirthSign でも、slot9→`FUN_00450A10` の実データ書き出しを行う。ただし「なぜこの2型だけが削除時も実データを必要とするか」という設計意図（参照保持、エディタ仕様等の採用理由）は retail バイナリだけからは確定できない。**

### §1-1 基底 slot11 の実体（v244 の関係付けを訂正）

- TESForm vtable `0xA3BE3C` slot#11(+0x2C) = `0x46B4C0`。ここは `FUN_0046B370`（ZLib flush）の mid-entry **ではなく**、0x46B4BC `ret` + int3 境界の後に置かれた独立関数。
- 実機械語（scanA）:
  - `0x46B4C0..0x46B4CF`: `this+8` の bit14(0x4000) が立っていれば `al=0; ret 4`。
  - `0x46B4D2..0x46B4F0`: bit5(0x20) が立っていれば `FUN_00450980(file,this)` → `FUN_00450A80(file)` → `al=1; ret 4`。
  - `0x46B4F3..0x46B4F8`: それ以外は `[this]->slot8(+0x20)` へ `jmp`。
- `FUN_00450980` は form type/form ID/flags を出力レコードヘッダへ設定して20Bを書き、`FUN_00450A80` はそのレコードを閉じる（decompiled :61618-61671）。一方 slot8 `FUN_0046B340` は bit14 を検査後、slot9(+0x24) で各型の payload を生成して `FUN_00450A10` で書き出す（:84816-84828、scanA 0x46B340..0x46B36A）。つまり基底 slot11 の bit5 経路は **payload なしの削除マーク用レコード系**、通常経路は **実データ付きレコード**と区別できる。

### §1-2 Script/BirthSign の差分と意味

- Script `0xA49DA4` / BirthSign `0xA52524` は、slot8 が両者とも基底 `0x46B340`、slot11 が両者とも `0x519320`。`0x519320` は int3 に挟まれた独立5B `jmp 0x46B340`（scanA）。
- したがってこの2型では、基底 slot11 の bit5 分岐自体が存在せず、bit14 ガードだけを slot8 側で受けて **bit5 の有無にかかわらず通常の実データ書き出し**となる。
- 既存の全イメージ imm32 走査（v244 scanD）では 0x519320 への参照はこの2 vtable のみ、direct E8/E9 は0件。今回の scanA でも、slot19/20 が基底実装のフォーム型のうち slot11 が基底と異なるのは Script/BirthSign と TESTopic だけで、TESTopic は別実装（slot8=0x52EDB0 / slot11=0x52EC60）だった。
- 2 vtable が同一物理アドレスを共有するのは、少なくとも機械語レベルでは **同一の「slot8 へそのまま委譲する」override を共有している**ため。同一関数折畳み（ICF）がアドレス共有に寄与した可能性はあるが、PE だけからはリンク時の折畳み自体は断定しない。

### §1-3 境界と [PORT] 含意

- 「基底と異なり bit5 時も実データを書く」という振る舞い上の理由までは確定。しかし、なぜ Script/BirthSign の削除表現に payload が必要なのか、単にエディタ上この2型を削除しない仕様への最適化なのかは、ソース/CS 側資料なしには分岐できず **未解明**。
- v241/v244 の結論どおり retail エンジン帯に slot11 仮想ディスパッチはなく、.ess 本体は slot19/20 を使うため、通常セーブ移植での追加実装は不要。TESFile 編集書き出しまで再現する場合だけ、この2型の slot11 は基底の bit5 特別経路へ戻さず slot8 委譲を再現すること。

---

## §2 項目2（N-29）: コンソール "save" ハンドラ FUN_005023D0 の内部

**判定: 【解明】"save" の配線は「名前→ID→ディスクリプタ→ハンドラ」の3段で、すべて静的データ表 0xB0B420（131 件・id 0x100..0x182・stride 0x28）で決まる。ハンドラ本体は (a) ParamInfo 0xB0A678 に従う引数解析（FUN_004FAE80）、(b) ファイル名の正規化（最初の '.' で切詰め＋".ess" 付加）、(c) FUN_00465130(this=*[0xB33B00], 0, name, bool) 呼出し、(d) 成否メッセージ、の4段のみ。bool はコンソール経路だけが 0/1 可変で、quicksave/autosave/メニューは常に 0。**

### §2-1 コマンド表（id 0x147 の配線）

- **表**: `0xB0B420` から **131 エントリ**（id 0x100..0x182）、**stride 0x28**（0xB0B420..0xB0C898）。エントリ構造（実測）:

  | off | 内容 | SaveGame の値 |
  |---|---|---|
  | +0x00 | 長名 `char*` | 0xA50A28 `"SaveGame"` |
  | +0x04 | 短名/別名 `char*` | 0xA50A20 `"save"` |
  | +0x08 | opcode(id) `dword` | `0x147` |
  | +0x0C | help `char*` | 0xA50A0C `"SaveGame <filename>"` |
  | +0x10 | flags `dword` | `0x20000` |
  | +0x14 | ParamInfo* | `0xB0A678` |
  | +0x18 | handler `code*` | `0x5023D0` |
  | +0x1C | 共通第2関数 | `0x4FDE30`（全エントリ共通） |
  | +0x20/+0x24 | 予備 | 0 |

  SaveGame エントリ実体は **`0xB0BF38`**（直後 0xB0BF60 が LoadGame id 0x148、0xB0BF88 が TestAllCells id 0x149）。
- **id → エントリ**: `FUN_004FCA30(opcode)`（decompiled :196123-196136）が
  `if (opcode-0x100 < 0x83) return &PTR_DAT_00b0b420 + (opcode*5-0x500)*2;`（= + (opcode-0x100)*0x28）。0x147 → 0x47 < 0x83 → **0xB0BF38** ✓。範囲外は第2表 0x1000.. の `0xB0C8C0`（`&PTR_s_MessageBox_00b0c8c0`, 0x171 件）。
  同関数はスクリプト VM の opcode 解決にも使われる（:194523 `iVar7 = FUN_004fca30((int)sVar1);` → :194534 `*(int*)(iVar7+0x14)`=params / :194544 `*(code**)(iVar7+0x18)`=handler を呼ぶ）。
- **名前 → id**: `FUN_004FCA70(name)` の第3ループ（:196180-196194）が `ppuVar3=&PTR_DAT_00b0b424` から stride 0x28（`ppuVar3 += 10`）で `ppuVar3[-1]`（長名）と `*ppuVar3`（短名）を `__stricmp` 照合し、一致時に `param_1[0x204]='X'` と `*(u32*)(param_1+0x208) = (&DAT_00b0b428)[i*10]`（= +0x08 の id）を書く。上限は `0xB0C89B`。⇒ `"save"` → **0x147**。
  （同関数の第1表 0xB0A2C8 は `Begin/End/Short/Long`=0x10..0x13、第2表 0xB0C8C0 は `MessageBox/GetDistance`=0x1000.. のスクリプト用。）
- **表の参照**: .text から表先頭への直接 imm 参照は Help ハンドラ `FUN_00501540`（`[esi+0xB0B420]`/`+424`/`+428`/`+42C` の反復、0x501553-0x50159B）等。ハンドラ 0x5023D0 自身は表の +0x18 経由でしか呼ばれず、`E8/E9 0x5023D0` は 0 件（v244 scanM M1 / 本 scanP P3: `imm32 0xB0BF50` の .text 参照 0 件）。

### §2-2 FUN_005023D0 本体（0x5023D0..0x5024F1、全 84 命令）

- プロローグ: `sub esp,0x310` + SEH + canary `0xB30AAC`。引数は **arg1..arg6, arg8**（arg7 は不使用）。
  - `arg1=[esp0+4]`（ParamInfo*）、`arg2=+8`、`arg3=+0xC`、`arg4=+0x10`、`arg5=+0x14`、`arg6=+0x18`、`arg8=+0x20`。`arg8` は 0x502419 でローカル `esp0-0x30C` に複写。
- **引数解析**: 9 引数で `FUN_004FAE80(arg1, arg2, arg8, arg3, arg4, arg5, arg6, buf=esp0-0x204, out=esp0-0x310)` を呼ぶ（0x5023D9-0x50243D。押下順は arg1,arg2,arg8,arg3,arg4,arg5,arg6,-0x204,-0x310）。
  - `al==0` → 0x5024D9（保存せず `al=0` で復帰）。
- **ファイル名構築**（0x50244D-0x5024A4）:
  1. `lstrcpyA(outName /*esp0-0x308*/, "")` — 空文字で初期化（src=0xA2F7EC=""、IAT `[0xA28164]`）。
  2. `p = _strchr(rawName /*esp0-0x204*/, '.')` — `0x982EC0` = `_strchr`（scanU U5 で本体一致）。
  3. `p != NULL` のとき: `*p=0` → `lstrcatA(outName, rawName)` → `lstrcatA(outName, ".ess")`（0xA3A0D4）→ `*p='.'`（raw を復元）。
  4. `p == NULL` のとき: `lstrcatA(outName, rawName)` のみ。
  ⇒ **拡張子は「付加」ではなく「置換」**: `save foo` → `foo.ess`、`save foo.bar` → `foo.ess`、`save foo.ess` → `foo.ess`。
  - `[0xA28160]`/`[0xA28164]` は **lstrcatA / lstrcpyA** の IAT スロット（.rdata 名表 0xB00E59 `lstrcatA`(hint 0x03BD) / 0xB00E65 `lstrcpyA`(hint 0x03C6) と対。scanAD）。静的初期値 0x700E56/0x700E62 が同一モジュール .text を指す点は N-32。
- **保存呼出し**（0x5024A4-0x5024BA）:
  `bool = ([esp0-0x310] != 0)`（`cmp dword[esp+0x10],0; setne cl`）→ `push bool`; `ecx = *[0xB33B00]`; `push outName`; `push 0`; `call 0x465130`。
  ⇒ `FUN_00465130(this=*[0xB33B00], param_2=0, param_3=outName, param_4=bool)`（`__thiscall`、decompiled :79503）。`param_3` が保存名であることは同関数が `"autosave"` と 9 文字比較する箇所（:79554-79566）で裏取り。
- **結果表示**: `al!=0` → `"Game Saved"`（0xA4BB18）、`al==0` → `"You cannot save right now"`（0xA4BAFC）を `FUN_00579B60`（コンソール出力）へ渡し、**戻り 1**。
- **LoadGame との対比**（`FUN_00502500`, scanU U2）: 骨格は同一で、`_strchr`/".ess" 付加も同一。相違は (a) 呼先が `FUN_00465860`（0x5025F7）、(b) bool の取得元が `[esp0-0x30C]`（0x5025D4）、(c) メッセージ文字列のみ。両者は **ParamInfo 0xB0A678 を共有**（= 1 個の必須 String）。

### §2-3 0xB0A678 サブテーブル（ParamInfo）の実体

- **12 バイト/件**: `{ char* typeName; u32 kind; u32 optional; }`。
  - `0xB0A678` `{'String', 0, 0}`（SaveGame の第1引数＝必須文字列）
  - `0xB0A684` `{'Integer (Optional)', 1, 1}`, `0xB0A690` `{'String', 0, 1}`, `0xB0A6C0` `{'Float(optional)', 2, 1}`, `0xB0A6CC` `{'Integer', 1, 0}`, `0xB0A744` `{'Float', 2, 0}` …
  - `kind`: 0=String / 1=Integer / 2=Float。`optional`: 0/1。**kind は文字列名と常に一致**（scanV V1 で 0xB0A660..0xB0A800 を全件照合）。
- 消費者 `FUN_004FAE80`（decompiled :194616-194629）: `puVar13 = (u32*)(param_1 + 4 + i*0xC)` → `+4` が kind。`switch(kind)` の case 0（String）は `_memcpy(dst, src, len); dst[len]=0`。kind 別属性バイト表 `(&DAT_00b0a54d)[kind*8]`（:194618）。
- 表は 1 本の共有プールで、各エントリの `+0x14` が「先頭件」を指す（HairTint 0x146 → 0xB0A6FC から `Integer`×3 が連続 = help "3 ints, RGB" と一致）。件数は呼出し側の引数個数で決まる（コンソールは入力トークン数）。

### §2-4 flags 0x20000 と他経路との差

- flags は SaveGame/LoadGame=`0x20000`、ToggleMapMarkers=`0x10000`、HairTint=`0x30000`、AddFaceAnimNote=`0x10001`。スクリプト・コンパイラ `FUN_004FDAF0` は `*(char*)(entry+0x10) != 0` を「クエスト外参照の明示要求」判定に使い（:197229、:197543）、`0x20000` はこのビットを含む（bit17）。ビット単位の意味分解は N-35。
- `FUN_00465130` の `param_4` は **コンソールだけ可変**:
  - quicksave `FUN_00466AD0` → `push 0`（0x466AF2）、autosave `FUN_00466B70` → `push 0`（0x466BCC）、SaveMenu `FUN_005D3230` → `push 0,0,0`（0x5D3240-0x5D3244）、`0x5D3438` 帯 → `push 0,0`、コンソール `FUN_005023D0` → **`setne` 由来の 0/1**（0x5024AD）。
  - `param_4 != 0` のとき `FUN_00465130` は 0x465182-0x4651C0 で **8B を確保**し `FUN_0045F0F0` の戻りを `this[0x10]` に格納（:79582-79593）＝追加メタ領域の生成。用途の意味付けは N-34。

### §2-5 [PORT] 含意

- 移植に必要な要素は 5 点: (1) 名前→id 索引（大小文字無視・長名/短名の 2 系統・上限 131）、(2) `id-0x100` の単純配列参照、(3) ParamInfo 12B の型変換（String/Integer/Float × optional）、(4) ファイル名正規化（最初の '.' で切詰め → `".ess"` 付加 → raw 復元）、(5) 保存本体。
- 正規化は **最初の '.'** 基準（`_strchr`）。`strrchr` ではないので `a.b.c` → `a.ess`。移植で `strrchr` を使うと挙動が変わる。
- bool 引数は「コンソール経由のみ true になり得る」フラグとして移植 API に残すと、後段のメタ領域生成差（8B）まで再現できる。

---

## §3 項目3（N-30）: FUN_00589B70 メニュー表の書手とエントリ構造

**判定: 【解明】`base=[0xB13974]` / `count=word[0xB1397A]` は独立グローバルではなく、**グローバル・コンテナ・オブジェクト `0xB13970` の +0x04 / +0x0A**。したがって「書手」は候補4関数のいずれでもなく、(a) `this=0xB13970` を受ける SetAt `FUN_0042BB90`（count と base[index] を書く）、(b) 配列の grow/初期化（未特定 = N-33）、(c) 破棄側 `0x5844D0` / `FUN_00A24910`。候補4関数はすべて**読み手**で、`0xB13974`/`0xB1397A` への直接 imm ストアは .text に 1 件も存在しない。**

### §3-1 FUN_00589B70 の実体

- decompiled :294976-294982:
  `if ((param_1-0x3E9U < 0x33) && (param_1-0x3E9U < (u32)DAT_00b1397a)) return *(u32*)(DAT_00b13974 + -0xFA4 + param_1*4); return 0;`
  `-0xFA4 = -(0x3E9*4)` なので index = `param_1 - 0x3E9`。範囲は **0x3E9..0x41B（51 件）**。
- 実機械語（scanW W2b）: `0x589B7F movzx ecx, word[0xB1397A]` / `0x589B90 mov edx, dword[0xB13974]`。
- 戻りは 4B のポインタ（Menu インスタンス）。未登録（範囲外 or 0 スロット）は 0。

### §3-2 実体はグローバル・コンテナ 0xB13970

- `0xB13970` は vptr。クラス vtable は **`0xA69680`**（scanX X1f / scanZ Z4）:

  | slot | 値 | 役割（実測） |
  |---|---|---|
  | +0x00 | 0x5844D0 | scalar deleting destructor（`[esi]`=0xA69680 再設定 → `[esi+4]` を free → 条件 free） |
  | +0x04 | 0xAC1454 | （データ/未走査） |
  | +0x08 | 0x584470 | 別 dtor 系（vptr 0xA69688、`[esi+8]` free） |
  | +0x0C | 0x6A9060 | hash（`key % [ecx+4]`） |
  | +0x10 | 0x763E80 | key 等値（`cmp; sete`） |
  | +0x14 | 0x67F130 | node 初期化（`[node+4]=key; [node+8]=value`） |
  | +0x18 | 0x68F970 | `ret 4`（no-op） |
  | +0x1C | 0x982D85 | 未走査 |

- **レイアウト**（`FUN_0042BB90` から実測）: `+0x00 vptr` / `+0x04 base(T**)` / `+0x0A word count` / `+0x0C word used`。
  すなわち `0xB13970=vptr, 0xB13974=base, 0xB1397A=count, 0xB1397C=used`。
- **SetAt = `FUN_0042BB90(index, const T** value)`**（0x42BB90-0x42BBB9、`ret 8`）:
  `movzx edx,word[ecx+0xA]; if (index >= count) count = index+1; if (*value != 0) { word[ecx+0xC]++; base[index] = *value; }`
  ⇒ **count（0xB1397A）を書く唯一の場所**。base 配列そのものへの書込みは `base[index]`（`[ecx+4]` を一度ロードしてから）。
- **呼出し例**（書手の入口）: `FUN_00585330` 0x58535A-0x585372 が `ecx=[esi+0x20]`（menu id）→ `sub ecx,0x3E9` → `push &null; push index; mov ecx,0xB13970; call 0x42BB90`（登録解除＝スロットを null 化。count は high-water なので減らない）。
- **破棄**: `0x5844D0`（vtable+0）が `push [esi+4]; mov [esi],0xA69680; call 0x401F20`（free）→ 条件 free。`FUN_00A24910`（0xA24910）は `mov eax,[0xB13974]; push eax; mov dword[0xB13970],0xA69680; call 0x401F20; ret` の **静的後始末（atexit 相当）**。

### §3-3 候補4関数の裁定

| 関数 | 0xB13974 / 0xB1397A の用法（実測） | 判定 |
|---|---|---|
| `FUN_0057F170` | 0x57F358 `cmp word[0xB1397A], 6` → 0x57F362 `mov eax,[0xB13974]` | **読み手**（メニュー 6 番の有無チェック） |
| `FUN_005821F0` | 0x5836A4/0x5836A7 `cmp word[..], bx` + `mov eax,[..]`、0x5836E3/0x5836E6/0x5836EC も同型 | **読み手**（2 箇所） |
| `FUN_005AC990` | 本体は jump table（0x5AC9ED `jmp [eax*4+0x5ACAF4]`）で 0xB383xx 系を触る。ヒットした 0x5ACB40 は**別関数**で `cmp word[0xB1397A],0x1A; jbe ret; mov eax,[0xB13974]; mov ecx,[eax+0x68]`（= index 0x1A の破棄） | **読み手**（＋ 0x5ACB43 は命令境界でない） |
| `FUN_00A24910` | `mov eax,[0xB13974]` → free、`[0xB13970]=0xA69680` 再設定 | **破棄**（書手ではない） |

- 0x5ACB43 の「ヒット」は byte-wise 走査の誤検出。実命令は 0x5ACB40 の `66 83 3D 7A 39 B1 00 1A`（`cmp word ptr [0xB1397A], 0x1A`）で、imm は 0x5ACB45 から始まる。**v244 以来の byte-wise imm 走査は「命令境界でない一致」を混ぜる**ため、本書では capstone のオペランド走査（scanW W2b）を正とした。

### §3-4 エントリ構造（menu-ID → Menu*）

- 表は **`id-0x3E9` の単純配列**（ハッシュでも連想でもない）。`T*`（4B）の連続で、`count` は high-water、`used` は非 null 件数。
- ID 帯 0x3E9..0x41B（51）。参照例: 0x3EF/0x3F0/0x3F1/0x3F2/0x3F4/0x3F5/0x3F6/0x402/0x40C/0x40E/0x40F/0x410/0x415 等（decompiled :282581-:343884 ほか）。0x40F = SaveMenu（v244 §4-2 の結論と一致、:337924 / :338249）。
- 登録値は Menu 派生インスタンスのポインタそのもの（`FUN_00585330` が `&null` を渡して解除するのが反証）。

---

## §4 項目4（N-31）: TESFlora MI のフォームマップ登録キー検証

**判定: 【整合】`[0xB0613C]` は `formID(dword) → TESForm*` のハッシュマップで、キーは **TESForm\* ポインタではなく formID の数値**。登録値は TESForm\* で、TESFlora の ctor は `lea esi,[ecx+0xC]` で TESForm サブオブジェクトを選んで基底 ctor に渡すため、値は自動的に **obj+0xC** になる。obj+0 を登録すると (a) formID/flags のフィールド位置が 0xC ずれ、(b) obj と obj+0xC の二重登録になり、(c) `TESForm*` を受ける全サイトが壊れる。MI 再現では「TESForm\* = obj+0xC」を不変条件にしてよい。**

### §4-1 マップ実体

- `0xB0613C` はオブジェクト先頭。`+0x00` vptr（値 = `0xA3BC94`）/ `+0x04` bucket 数 / `+0x08` bucket 配列。`[0xB06144]`（= +0x08）を直接ロードするサイトあり（scanAB AB1 0x448D9E `mov ecx,[0xB06144]; mov esi,[ecx+eax*4]`）。
- vtable `0xA3BC94`（scanX X2a / scanAB AB3）:

  | slot | 値 | 役割（実測） |
  |---|---|---|
  | +0x00 | 0x46B030 | deleting dtor（`[esi]`=0xA3BC94 再設定 → `[esi+8]` free → 条件 free） |
  | +0x04 | 0x6A9060 | hash = `key % [ecx+4]`（`div`） |
  | +0x08 | 0x763E80 | key 等値 = `cmp ecx,[esp+8]; sete al` |
  | +0x0C | 0x67F130 | node 初期化 = `[node+4]=key; [node+8]=value; ret 0xC` |
  | +0x10 | 0x68F970 | `ret 4`（no-op） |
  | +0x14/+0x18 | 0x982D85 | 未走査 |

  hash/compare/node の 3 スロットは §3 のメニュー・コンテナ vtable `0xA69680` と同一実装（共通ハッシュ表基底）。
- **find = `FUN_0055E000(this, key, &out)`**（0x55E000-0x55E04E、`ret 8`）:
  `h = (*[this])(+4)(key)` → `bucket = [this+8][h]` → chain `[node]` を `(*[this])(+8)(key, [node+4])` で走査 → 一致で `*out = [node+8]`・`al=1`。不一致 `al=0`。
  decompiled も同型（:55981 / :55984）: `(**(code**)(PTR_PTR_00b0613c+4))(key)` / `(**(code**)(...+8))(key, puVar3[1])`。

### §4-2 キーは formID（数値）

- `0x763E80` は 4B の等値のみ（`mov ecx,[esp+4]; cmp ecx,[esp+8]; sete al; ret 8`）。
- 決定的証拠は **formID 採番経路 `FUN_00448D40`**（0x448D40-0x448E1B）:
  1. `[ebx+0x8C0]` のカウンタを +1。`& 0x00FFFFFF == 0x00FFFFFF`（mod index 無し）のときは `0xFF000000` を維持して `+0x800` に飛ばす（0x448D60-0x448D85）＝ **formID 生成規則そのもの**。
  2. `mov edi, eax`（新 ID）→ `mov edx,[0xB0613C]; call [edx+4]`（hash）→ `[0xB06144 + eax*4]`（bucket）→ `call [vptr+8]`（compare）で**未使用 ID を検索**（0x448D8B-0x448DC6）。
  3. `FUN_00453450(this=*[0xB33B00], id)` で使用中判定（0x448DDA、0x448E07）。
  ⇒ 検索キーは採番された **formID の数値**であり、ポインタではない。
- 取得 API も数値キー: `FUN_004471A0(id)`（0x4471A0-0x4471CB）と `FUN_00447350(id)` は引数 dword を null 検査 → `mov ecx,0xB0613C; call 0x55E000` → 戻りは格納値（0 なら未登録）。
- `0xB0613C` の .text imm 参照は **96 件**（`mov ecx,0xB0613C` 72 / `mov eax,[0xB0613C]` 19 / `mov [0xB0613C],edx` 4 / その他 1）。後者 4 件（prev=0x15）だけがマップ先頭 dword の差替えで、残りは find 系。

### §4-3 値 = TESForm*（upcast 済み）で整合する理由

- 値は **node+8**。insert は vtable+0xC の `0x67F130` が `[node+4]=key; [node+8]=value` を書くだけ（値の型検査なし）。
- TESFlora ctor `FUN_004ADE70`（scanY Y4）:
  `lea esi,[ecx+0xC]` → `[ecx]=0xA41C40`（+0 vptr）、`[esi]=0xA41B1C`（+0xC vptr）、`[ecx+0x30]=0xA41B04`、`[ecx+0x3C]=0xA41AE4`、`[ecx+0x54]=0xA41AD0` → **`mov ecx,esi; call 0x46C2F0`（TESForm 基底 ctor）** → `mov ecx,esi; call 0x4B3FF0`。
  ⇒ 基底 ctor の `this` は **obj+0xC**。formID 設定・マップ登録はこの `this` を値にするため、登録値は obj+0xC で自動的に整合する。
- obj+0 を値にした場合の破綻: formID/flags のフィールド読みが 0xC ずれる／`TESForm*` を受ける全サイト（slot 呼出し・formFlags 判定）が壊れる／obj と obj+0xC が二重登録される。**したがって登録キーは formID 数値、登録値は必ず upcast 済み TESForm\*（obj+0xC）**。

### §4-4 [+0] vptr 経由 slot19/20 が禁止であることの裏取り

- +0 の vtable `0xA41C40` は **4 スロットのみ**（0x46E0A0 / 0x60D0A0 / 0x46E110 / 0x46E150）で、直後に ASCII 文字列（"ITMPOrganic"、"sFloraSuccessMessage" 等）が連結（v244 scanL L1）。slot19(+0x4C)/slot20(+0x50) を読むと**文字列バイト列を関数ポインタとして解釈**する。
- TESForm slot19/20 に到達できるのは 72 スロットの `0xA41B1C`（= obj+0xC）経由のみ。同 vtable の slot19/20 は `0x46AC50`/`0x46B5E0`（TESForm 基底）で TESFlora は INH（v244 §1-5）。
- ⇒ [PORT] では「TESForm\* = obj+0xC」を型不変条件として固定し、(1) フォームマップの値、(2) 全仮想呼出しの基点、(3) ダウンキャスト（`TESFlora* = form - 0xC`）を一貫させる。MI の 5 vptr（+0/+0xC/+0x30/+0x3C/+0x54）は v244 §1-5 の構成をそのまま写す。

---

## §5 まとめ・新規残点・自己検証

### §5-1 N-28..N-31 の裁定まとめ

| ID | 判定 | 要点 | 実装要否 |
|---|---|---|---|
| N-28 | 実装意味は解明／設計動機は未解明 | Script/BirthSign の slot11 = `0x519320`（`jmp 0x46B340`）で、基底 slot11 の bit5 特別経路を通らず slot8 へ委譲 | .ess 経路では不要 |
| N-29 | **解明** | 名前→id（0x147）→エントリ `0xB0BF38`→handler `0x5023D0`。filename は *最初の '.' で切詰め + ".ess"*。保存は `FUN_00465130(this=*[0xB33B00], 0, name, bool)` | 要（コンソール save 実装時） |
| N-30 | **解明** | 表は `0xB13970` コンテナの +4(base)/+0xA(count)。SetAt `FUN_0042BB90` が唯一の count 書手。候補4関数は全て読み手/破棄 | 要（メニュー ID 表） |
| N-31 | **整合** | マップ `0xB0613C` は formID→TESForm\*。キーは数値、値は obj+0xC の upcast 済み TESForm\* | 要（MI 再現時） |

### §5-2 新規残点（本タスクで発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-32 | `[0xA28160]`/`[0xA28164]` は名表 0xB00E59 `lstrcatA`（hint 0x03BD）/ 0xB00E65 `lstrcpyA`（hint 0x03C6）と対になる IAT スロットだが、**静的初期値が同一モジュール .text の 0x700E56 / 0x700E62**（関数先頭ではない）を指す。ローダ解決後の実アドレスは静的解析では観測不能 | 低 |
| N-33 | メニュー・コンテナ `0xB13970` の **base 配列（0xB13974）を確保する grow/ctor** の関数アドレス未特定。SetAt `FUN_0042BB90` は「既に確保済み」を前提に count/base[index] のみ書く | 低 |
| N-34 | `FUN_005023D0` の解析結果フラグ（→ `FUN_00465130` param_4）の意味。`param_4!=0` で 8B 確保＋`FUN_0045F0F0` の戻りを `this[0x10]` に格納する分岐（:79582-79593）の用途が未確定 | 低 |
| N-35 | コンソール表 flags（0x10000 / 0x20000 / 0x30000 / 0x10001）のビット単位意味分解。コンパイラが `entry+0x10 != 0` を「クエスト外参照の明示要求」判定に使う点のみ確認 | 低 |
| N-36 | メニュー・コンテナ vtable `0xA69680` の slot+0x04（0xAC1454 = .rdata 領域を指す）と slot+0x1C（0x982D85）の役割未走査 | 低 |

### §5-3 自己検証

- **selftest**: 着手時と完了時の 2 回実行し、いずれも **ALL PASS (4/4)**（`probe_harness.py selftest`）。
- **走査手法の是正**: byte-wise imm32 走査は命令境界外の一致を混ぜる（実例: 0x5ACB43 は `cmp word ptr [0xB1397A],0x1A` の途中、S1 の 0xB0BE83 等は `lea` の ModRM/disp 断片）。本書のアドレス断定は **capstone のオペランド走査**（scanW W2b / scanT T3）と、その場での逆アセで二重確認した。
- **機械語アンカー（再現可能）**:
  - `66 83 3D 7A 39 B1 00 1A` @0x5ACB40（count 読み）
  - `A1 74 39 B1 00` @0x5ACB4A / 0x57F362 / 0x5836AD（base 読み）
  - `89 4E 68`（`mov ecx,[eax+0x68]`）@0x5ACB4F
  - `C7 06 80 96 A6 00`（vptr=0xA69680）@0x5844D7
  - `A1 74 39 B1 00 50 C7 05 70 39 B1 00 80 96 A6 00` @0xA24910（free + vptr 再設定）
  - `B9 3C 61 B0 00`（`mov ecx,0xB0613C`）@0x4471B1 ほか 72 件
  - `8B 44 24 04 33 D2 F7 71 04` @0x6A9060（hash = key % [ecx+4]）
  - `8B 4C 24 04 33 C0 3B 4C 24 08 0F 94 C0` @0x763E80（key 等値）
- **使用証拠一覧**（すべて `%TEMP%\opencode\` 配下、`_out.txt` 付き）:
  - `v246_scanP.py`（P1: FUN_005023D0 全 84 命令 / P2: 0xB0BF20・0xB0A660 dump / P3: 表ポインタ参照 0 件）
  - `v246_scanQ.py`（Q1/Q2: 文字列解決・表 walk / Q3/Q4: 表参照・0x147 imm / Q5: ParamInfo 16B 仮説の棄却）
  - `v246_scanR.py`（R1: 0xB0BD80 級エントリ dump / R3: id 0x140..0x160 全 33 件 / R4: 0x4FDE30・0x579B60・0x465130 の同定）
  - `v246_scanS.py`（S1: 近傍 imm ヒストグラム＝byte-wise の誤検出例 / S4: 0x593226・0x52B9D5 の逆アセ）
  - `v246_scanT.py`（T1/T2: 表境界 **0xB0B420..0xB0C898・131 件** の確定）
  - `v246_scanU.py`（U1: IAT 値 / U2: LoadGame 全命令 / U3: スクリプト表 0xB0A2C8・0xB0C8C0）
  - `v246_scanV.py`（V1: ParamInfo 12B 全件 / V4: Help ハンドラの表反復）
  - `v246_scanW.py`（W1: 0x465130 呼出し 6 件の引数形 / W2a/W2b: 0xB1397x 参照 / W3: 候補4関数 / W5: 0xB0613C 参照）
  - `v246_scanX.py`（X1: 0x585330・0x587B20・0x585B60 / X1f: vtable 0xA69680 / X2: 0xA3BC94・0x4471A0）
  - `v246_scanY.py`（Y1: 0x5ACB40 実命令 / Y2: `mov ecx,0xB0613C` 40 件の後続 call / Y4: TESFlora ctor）
  - `v246_scanZ.py`（Z1: FUN_0055E000 / Z3: 0xB0613C 参照の opcode 分類 / Z4: 0xA69680）
  - `v246_scanAA.py`（AA1: key 等値 / AA2: `*out=[node+8]` / AA3: 0x42BBxx 帯）
  - `v246_scanAB.py`（AB1: FUN_00448D40 採番 / AB3: hash・node・no-op スロット）
  - `v246_scanAC.py` / `v246_scanAD.py`（IAT スロットと `lstrcatA`/`lstrcpyA` 名表の対）
  - `v246_scanAE.py`（AE1: コンテナ dtor / AE3: 登録解除 `FUN_00585330`）
  - 入力（読取のみ）: `tmp/re/v246_...md`（§1 無改変）, `v244_port_saveslot_residuals.md`, `v241_port_mgef_residuals6.md`, `tmp/re/cache/vtable_index.tsv` / `named_funcs.txt` / `call_targets`。
  - decompiled 部分読取: :79501-79640（FUN_00465130）, :194520-194700（FUN_004FAE80）, :196123-196260（FUN_004FCA30/FUN_004FCA70）, :197153-197303（FUN_004FDAF0）, :294974-294983（FUN_00589B70）, :55981・:55984（マップ hash/compare の decompiled 形）。
- **不変条件の再確認**: ESM 本体は未アクセス。git 不使用。docs/GT/TODO/session-memory/plan/cache/ソースの編集なし。**追記したのは本ファイル（v246）のみ**。
