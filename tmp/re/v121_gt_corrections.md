# V121: GT 不整合 5 件の訂正案（分析成果物・編集前レポート）

作成: 2026-10-26（opencode サブエージェント V121 / **分析専任**）
対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / SHA256 `A8F31384…C2B7DC6`）
手法: capstone **5.0.7**（`skipdata=True`, `md.detail=True`）、ImageBase `0x400000`、VA = `0x400C00` + file_off（.text）/ `0x400E00`（.rdata）/ `0x401400`（.data）
状態: **分析完了・編集なし**（`docs/` と `tmp/re/GT_CHEATSHEET.md` は一切変更していない。承認後に親セッションが §3 の手順で適用する）

---

## §0 前提の訂正（反映前に必ず読む）

「GT 不整合 4 件」の原文は **`tmp/re/GT_CHEATSHEET.md` には存在しない**（`GetFormID` / `RemoveFromList` /
`GetFormTypeID` / `GetFormType` の 4 語は GT 全 152 行・`docs/RE_EXE_REPORT.md` 全 12,318 行のどちらにも 0 件）。
誤記の実体は以下にある:

| 場所 | 現行記述（引用） | 扱い |
|---|---|---|
| `tmp/re/handover_20261025/session_B_summary.md` **L43** | `- **vtable offset → function mapping**: … vt+0x2D0=GetFormTypeID()→int, vt+0x36C=GetFormType()→int, vt+8=GetFormID, vt+0xA0=RemoveFromList(push 1), … vt+0x190=IsActor/IsReference, vt+0x198=IsDead/IsDisabled.` | **誤記の発生源**（ハンドオーバ履歴・編集対象外。GT からリンク注記のみ推奨） |
| `tmp/re/b14_slot_method_map.md` L108–113（§1.3） | `| **0x08** | 「+8 = GetFormID」 | … |` ほか 4 行 | 出典として正しく引用（**訂正不要**） |
| `tmp/re/TODO_re49.md` L275（行 119 / V121） | 訂正内容そのもの（タスク定義） | 適用後に `[x]` + detail 追記（親セッション） |
| `tmp/re/GT_CHEATSHEET.md` | slot 行は `vt+0x330` / `+0x334` の 1 行のみ（L112） | **追記型**の反映 |
| `docs/RE_EXE_REPORT.md` | slot を直接示す行は §17.2 / §41.3 のみ | **置換・追記**の反映 |

→ 結論: GT_CHEATSHEET は「訂正行の追加」、RE_EXE_REPORT は「§17.2 / §41.3 の現行行の訂正＋追記」が適用形態。

---

## §1 訂正 5 件（訂正前 → 訂正後）

訂正後テキストは GT_CHEATSHEET §4（または新設 §4.1）にそのまま貼れる形で書いた。
**数値・アドレスはすべて本セッションで再測定（capstone）し、b14 §1.3/§4-2/§5-1 と一致を確認済み。**

### 1.1 `+0x08`（byte offset 8 = スロット #2）

**訂正前**（出所: `handover_20261025/session_B_summary.md:43`。GT 本体には未収録）
> `vt+8=GetFormID`

**訂正後（GT 追記用本文）**
> | `+0x08` | **フォーム比較/同期ヘルパ**（名称保留。**GetFormID ではない**） | TESForm 系 86 vtable → `0x46B800`: `__RTDynamicCast(0xB031EC BaseFormComponent → 0xB02F9C TESForm)` 成功時、`[this+8]`(flags) と arg の **bit1(0x2)** を比較 → 差異なら `[this].vt+0x90` 呼出し、続けて arg の `formType(+4)` と flags の **bit14(0x4000)** を this へ同期 → `ret 4`。**TESForm vtable 55 スロット全域を走査しても GetFormID 相当（`mov eax,[ecx+0xC];ret`）は不在**（同パターン 4 候補 `0x4133D1`/`0x5204C9`/`0x88D370`/`0x943410` はいずれも未登録）。GetFormID は本スロットではない（非仮想か別スロット＝V143+ 残点） |

**根拠（capstone / vtable / 呼出し元）**
- vtable エントリ実測: TESForm `0xA3BE3C`+8 = TESObjectREFR `0xA46C44`+8 = Actor `0xA6E074`+8 =
  Character `0xA6FC9C`+8 = Creature `0xA710F4`+8 = PlayerCharacter `0xA73A0C`+8 = MobileObject `0xA732FC`+8 = **`0x46B800`**（b14 の「TESForm 系 86 vtable」と整合）。
- 本文（`0x46B800`、Ghidra ラベル `FUN_0046b7a0+0x60`）:
  ```
  0x46B800: mov eax,[esp+4] / push esi / push edi
            push 0 / push 0xB02F9C(TESForm) / push 0xB031EC(BaseFormComponent) / push 0 / push eax
            call 0x9832E6            ; __RTDynamicCast
            test edi,edi ; je 0x46B85D
            mov eax,[edi+8] ; mov ecx,[esi+8] ; shr eax,1 ; shr ecx,1 ; and al,1 ; and cl,1 ; cmp cl,al
            je 0x46B845 ; mov edx,[esi] ; push eax ; mov eax,[edx+0x90] ; mov ecx,esi ; call eax
            mov cl,[edi+4] ; mov edx,[esi+8] ; mov [esi+4],cl        ; formType コピー
            xor edx,[edi+8] ; and edx,0x4000 ; xor edx,[edi+8] ; mov [esi+8],edx  ; flag bit14 同期
            pop edi / pop esi / ret 4
  ```
- 戻り値は無く（`ret 4` = 1 引数破棄）、**ID を返さない** = GetFormID ではない。
- 呼び出される `vt+0x90` = **`0x46C640`（`FUN_0046c640`）**: `arg==0 なら既存 bit1/bit14 グローバル配列
  （`[0xB06150]`×`[0xB06158]`）への線形探索 → 未登録なら `call 0x449070`（`0xB0614C` へ挿入）→ `or dword [esi+8],2`
  ＝**「グローバル表へ登録してから flags bit1 を立てる」ヘルパ**。`+8` はその bit1/flag の差分同期を呼ぶ側。
- 呼出し元: 直接 `call 0x46B800` = **0 件**、レジスタ経由ディスパッチ（`call <reg>` で `<reg>=[vt+8]`、直前 8 命令に `mov dst,[基点]` を要求する厳格ヒューリスティック）= **843 箇所**、`call dword ptr [reg+8]` = **318 箇所**（※いずれもオブジェクトフィールド由来の誤検出を含む**上限値**。少なくとも高頻度に使われる第 3 仮想関数であることは確定）。
- Process 系は**同一 offset で別物**（b14 §1.4 の二重意味ルール）: Base/High → `0x6F7070`（0 返し）、Low → `0x73FD50`（`mov eax,3; ret`）、MidLow → `0x8CF6B0`（`mov eax,2; ret`）、MidHigh → `0x91FEA0`（`mov eax,1; ret`）＝**process レベル取得（0..3）**。

**誤記の成り立ち（推定）**: スロット #2 は「TESForm の最初のユーザ仮想関数」であり、Oblivion の TESForm API の先頭は GetFormID という慣例的知識を**未検証のまま割り当てた**とみられる（返り値を伴う実測が無い）。b14 §1.3 は「GetFormID は vtable 仮想関数として未確認」と保留のまま。

### 1.2 `+0xA0`

**訂正前**（同上 L43）
> `vt+0xA0=RemoveFromList(push 1)`

**訂正後（GT 追記用本文）**
> | `+0xA0` | **flags bit `0x20000` の bool setter（set/clear）**（引数 1。**RemoveFromList ではない**） | TESForm 系 86 vtable → `0x46AB00`: `cmp byte [esp+4],0; je +0x11; or dword [ecx+8],0x20000; ret 4` / else `and dword [ecx+8],0xFFFDFFFF; ret 4`。RemoveFromList 実装は `0x6162D0`(unlink) / `0x65C620`(Remove)（GT §4 リスト行）で本スロットとは無関係。Process 系は**別物**（`0x6292A0` = `fld [esp+4]; fstp [ecx+0x88]; ret 4` の float setter） |

**根拠**
- vtable 実測: TESForm / TESObjectREFR / Actor / Character / Creature / PlayerCharacter / MobileObject の +0xA0 すべて = **`0x46AB00`**。
- 本文（`0x46AB00` = `FUN_0046aa50+0xB0`、8 命令）: **if/else の両分岐を実測**:
  ```
  0x46AB00: cmp byte [esp+4],0 ; je 0x46AB11
            or  dword [ecx+8],0x20000 ; ret 4        ; arg != 0  → flags |= 0x20000
  0x46AB11: and dword [ecx+8],0xFFFDFFFF ; ret 4      ; arg == 0  → flags &= ~0x20000
  ```
  つまり **flags bit17(0x20000) の bool setter（set/clear）** であり、リスト操作（unlink / node 解除）は一切しない。
- **同一関数 `FUN_0046aa50` に兄弟実装が集まる**: TESForm `+0x94`=`FUN+0x30` / `+0x98`=`FUN+0x60` / `+0x9C`=`FUN+0x80` /
  `+0xA0`=`FUN+0xB0`（＝フラグ系アクセサのファミリ）。+0xA0 が「リスト削除」ではなく「フラグ setter」であることを
  vtable 構造レベルでも裏づける。
- 呼出し元: 直接 `call 0x46AB00` = **0 件**、ディスパッチ **35 箇所**。
- ⚠️ **タスク記載の「bit0x2000」は転記誤り**: 実測値は **`0x20000`（bit17）**（`81 49 08 00 00 02 00` = `or [ecx+8],0x20000` / `81 61 08 FF FF FD FF` = `and [ecx+8],0xFFFDFFFF`）。訂正後本文は `0x20000` で統一すること（b14 §1.3 も `0x20000`）。

**誤記の成り立ち（推定）**: 1 引数（push 1 / 0）の setter であることから、呼出し側が「リストから除外する用途」で使っていたことを**用途＝名前**として記載したとみられる。GT §4 の Remove = `0x65C620`（854 calls）とは impl が一致しない。

### 1.3 `+0x2D0`

**訂正前**（同上 L43）
> `vt+0x2D0=GetFormTypeID()→int`

**訂正後（GT 追記用本文）**
> | `+0x2D0` | **攻撃・アイテム消費系アクション**（本体は `ret 8` ＝ 2 スタック引数・**GetFormTypeID ではない**） | Actor/Character/Creature → `0x5E4440`（arg の `byte[+0x88]&1` と `dword[+0x64]` を検査 → `[this+0x100]` へ **10 値 thiscall** → `0x5C1900` / `this+0x5C` → `0x699190` / g_thePlayer 時 `0x664850`）、Player → `0x666820`（`0x5E4440` を内包）、Base/Low/MidLow/MidHigh → `0x6F7030`（`or eax,-1; ret`）、HighProcess → `0x628200`（`movsx eax,word [ecx+0x1F4]; ret` = word フィールド取得、型関連の可能性は[要深掘]）。Process 系には「GetFormType 系」の痕跡なし |

**根拠**
- vtable 実測: Actor `0xA6E074`+0x2D0 = Character `0xA6FC9C`+0x2D0 = Creature `0xA710F4`+0x2D0 = `0x5E4440` /
  PlayerCharacter `0xA73A0C`+0x2D0 = `0x666820` / Base `0xA6F464`+0x2D0 = Low `0xA720A4` = MidLow `0xA72D64` = MidHigh `0xA72684` = `0x6F7030` / High `0xA71814`+0x2D0 = `0x628200`。
- 本文（`0x5E4440` = `FUN_005e4440`、**本体は `ret 8` ＝ 2 スタック引数**）: 検査（`arg!=0` / `byte[arg+0x88]&1` / `dword[arg+0x64]!=0`）通過後、
  `[esi]`（vtable）+`0x100` の関数へ 10 値 thiscall（`0,1,0,0,0,0,0,1,0,arg`）→ `call 0x5C1900`
  → `lea ecx,[esi+0x5c]` のメンバオブジェクトへ `call 0x699190(…, this+0x5C)` → `this==g_thePlayer(0xB333C4)` のときのみ `call 0x664850(0)`。
  Player 版 `0x666820`（`FUN_00666670+0x1B0`）は `call 0x5E4440` → `0x4D8D70` → `0x664850`（＝攻撃/消費アクション連鎖）。
- 呼出し元: 直接 `call 0x5E4440` = **1 件のみ**（`0x66682F` = Player 版の内部呼出）、`call 0x666820` = 0 件、ディスパッチ **36 箇所**（`mov r32,[r+0x2D0]` ロード = 38 箇所）。
- 対照: GT/ハンドオーバの「GetFormTypeID」は HighProcess 版 `movsx eax,word [ecx+0x1F4]` に由来する可能性が高い（b14 §5-5「GT『GetFormTypeID』が Process 系にだけ部分的に当てはまる可能性（要深掘）」と同一指摘）。

**誤記の成り立ち（推定）**: 9 vtable（process 系）にしか存在しない slot であることに気づかず、HighProcess 版の「単一 word フィールドを返す挙動」を型 ID と解釈し、Actor 版まで同じ名前で広げた。

### 1.4 `+0x36C`

**訂正前**（同上 L43）
> `vt+0x36C=GetFormType()→int`

**訂正後（GT 追記用本文）**
> | `+0x36C` | **process 状態（種別）バイト取得**（**GetFormType ではない**） | Actor/Creature/PlayerCharacter → `0x68F970`（`ret 4` 空）、Character → `0x611B40`（`[this+0x370]`/`[+0x308]`/`[+0x330]` 連鎖の大物）、Base/Low/MidLow → `0x6F7070`（0）、MidHigh/High → `0x64B090`（`movzx eax,byte [ecx+0x11D]; ret`）。Actor 系 `+0x18C`（IsSleeping 系述語 `0x5E0EC0`）が `process[vt+0x36C]` へ **jmp 転送**する先が本スロット。**formType は TESForm+4 の byte フィールドであり仮想関数化されていない** |

**根拠**
- vtable 実測: Actor/Creature/Player `+0x36C` = `0x68F970`、Character = `0x611B40`、Base/Low/MidLow = `0x6F7070`、MidHigh/High = `0x64B090`。
- 転送の実物（`0x5E0EC0` = Actor 系 `+0x18C` impl, 7 命令）:
  ```
  0x5E0EC0: cmp dword [ecx+0x58],0 ; je 0x5e0ed3      ; this->process
            mov ecx,[ecx+0x58] ; mov eax,[ecx] ; mov edx,[eax+0x36C] ; jmp edx   ; process vt+0x36C へ委譲
  ```
- 本体（`0x64B090`）: `movzx eax, byte [ecx+0x11D]; ret`（process オブジェクトの byte フィールド 1 つを返すだけ）。
- **書き込み側の実測**: `[reg+0x11D] = imm8` 書き込みは **12 箇所**（`FUN_0064b400+0x136`→3 / `FUN_006545e0`→3 / `FUN_006564c0` 内 5 箇所→0x00,0x01,0x03,0x06 等）。**値集合は 0/1/3/6 系の小さな離散値**＝状態(種別)バイトとして整合。
- **ゲートの実物**（`FUN_00629b70` = アクター更新ラッパー、report §17.3-4 の出所）:
  ```
  0x629B70: mov eax,[esi] ; mov edx,[eax+0x36C] ; call edx ; test eax,eax ; je 0x629bab
            … mov edx,[eax+0x36C] ; call edx ; cmp eax,4 ; je …
            … mov edx,[eax+0x36C] ; call edx ; cmp eax,9 ; je …
            xor al,al ; ret 8          ; 0/4/9 以外は即 return false
  ```
  **`vt+0x36C` の戻り値が 0/4/9 の場合のみ処理が進む**（report L1079 の記述と一致）。返り値が「enum 的な種別/状態値」であることの直接証拠。
- 呼出し元: 直接 `call 0x68F970` = **81 件**、ディスパッチ **149 箇所**（`mov r32,[r+0x36C]` ロード = 153 箇所）。
- `docs/RE_EXE_REPORT.md` 側の既存記述も「process」寄り: §17.2 L1070「プロセス vtable の主要スロット: … `+0x36c` 種別取得」、§17.3-4 L1079「`FUN_00629b70` — アクター更新ラッパー。`vtbl+0x36c` が **0 / 4 / 9** の場合のみ本体を実行するゲート構造」。
- **GT の「Form」は process の「種別/状態」との混同**（厳密な語は「状態バイト or 種別バイト」で確定が分かれるため、GT 本文は「process 状態（種別）バイト」＋[要深掘]扱いが安全）。

**誤記の成り立ち（推定）**: `GetFormType` という名が既に Slot 名の候補としてメモされ、それが「Form」を含む同一 offset の別系統（process）に流用された。GT §4 の formType 相当は仮想関数ではないため対応先が存在しない。

### 1.5 `+0x190`

**訂正前**（同上 L43）
> `vt+0x190=IsActor/IsReference`

**訂正後（GT 追記用本文）**
> | `+0x190` | **IsActor**（bool。**"IsRef / IsReference" 表記は削除** — plain `TESObjectREFR` は **0** を返す） | Actor/Character/Creature/Player → `0x977C50`（`mov al,1; ret`）／ TESObjectREFR・MobileObject・Projectile 系 → `0x69D990`（`xor al,al; ret`）／ BaseProcess・Low・MidLow → `0x60D880`（**別物**: refcount release 型）／ MidHigh・High → `0x634910`（**別物**: NiPointer setter 型）。**ゲッタ系の型ガードとして最重要**（b10: 式ゲッタ 82 件のガード） |

**根拠**
- vtable 実測: Actor `0x977C50` / Character `0x977C50` / Creature `0x977C50` / PlayerCharacter `0x977C50`（＝1 を返す）／
  TESObjectREFR `0x69D990` / MobileObject `0x69D990`（＝0 を返す）／ Base `0x60D880`・Low `0x60D880`・MidLow `0x60D880`／
  MidHigh `0x634910`・High `0x634910`。
- 本文: `0x977C50: mov al,1; ret` / `0x69D990: xor al,al; ret`（1 命令述語）。
- **plain REFR が 0 を返す**ため「IsReference」は実装と矛盾（IsReference なら REFR で 1 になるべき）。
- 呼出し元: ディスパッチ **613 箇所**（厳格ヒューリスティックの上限値）、直接 `call 0x977C50` = 103 件・`call 0x69D990` = 3 件
  （※`0x977C50` は**共有スタブ**で、GT §67.9 はスクリプト台帳 7 opid の impl としても使われている＝直接呼出し数は slot0x190 専用ではない）。
- 用途証拠: `docs/RE_EXE_REPORT.md` L9769 の窃盗判定 `vt+0x190(target) && 0x4D7F80(type 0x23) && vt+0x198(0)==0`（＝「対象が Actor か」判定として使用）。
- **Process 系 offset は別物**（b14 §1.4）: この 2 系統を混同しないこと。

**誤記の成り立ち（推定）**: 「Actor なら 1／参考リファレンスでも 1」という想定で `IsReference` を並記したが、実装は plain REFR で 0。b14 §1.1 は「GT 名 "IsActor/IsRef" は要訂正」と既に指摘済み。

---

## §2 反映先の所在特定（**照合のみ・編集していない**）

### 2.1 `tmp/re/GT_CHEATSHEET.md`（11,458 B / 152 行 / **LF** / .bak なし）

| 見出し | 行 | 現行記述（引用） | 適用内容 |
|---|---|---|---|
| ヘッダ | L4 | `最終更新: 2026-10-25（opencode セッション A/B 引き継ぎ + V90〜V120 ラウンド）。` | 日付＋V121 追記 |
| `## 4. 主要地址（**引用前に必ず probe**）` → `### クラス / vtable` | L105–115 | `| 項目 | 値 |` 表。うち slot に関わる行は **L112 の 1 行のみ**: `` | `vt+0x330` / `+0x334` | 206スロット超の **9クラスのみ**。impl = `0x5E6190`(Actor系) / `0x6F7070`(Player) / `0x6455F0`(Process系) ほか |``（b14 §6 で**整合確認済み・訂正不要**） | 本表の直後に **`### 4.1 vtable slot 主要対応（V121 訂正込み）`** を新設し、§1 の 5 行を貼付（＋b14 確定 slot の 0x170/0x198/0x284/0x288/0x2C0/0x164/0x174/0xEC/0x154/0x18C/0x380/0x330/0x334 の対応も同テーブルに収容可） |
| `## 5. 訂正履歴` | L117–131 | 表の最終行 L131: `` \| 2026-10-25 \| `docs/RE_EXE_REPORT.md` へ上記9箇所を反映（`.bak15`、12,319行） \|`` | 末尾に **V121 行**を追加（文面は §4 のテンプレ参照） |
| `## 7. このリポジトリの周辺ファイル` | L149 | `` \| `tmp/re/handover_20261025/` \| セッションA/B のハンドオーバ資料 + MANIFEST + サマリ2件 \|`` | （任意）`session_B_summary.md:43` の slot map が**旧記載（4 件訂正済）**である旨を注記 |
| L150 / L152 | — | `` docs/RE_EXE_REPORT.md … 12,319行 / CRLF / .bak15 `` / `` round_verify.py … → `ALL OK` 必須 `` | 適用後の行数・bak 番号を更新 |

> 補足: GT_CHEATSHEET に `GetFormID` 等 4 語は**存在しない**（`Select-String` で 0 件）。
> したがって「GT 記載の訂正」は厳密には **GT への訂正付き追記** である。

### 2.2 `docs/RE_EXE_REPORT.md`（1,029,892 B / 12,318 行 / **CRLF** / 現行 `.bak15`）

| 見出し | 行 | 現行記述（引用） | 適用内容 |
|---|---|---|---|
| `### 17.2 データレイアウト` | L1070 | `` - プロセス vtable の主要スロット: `+8` レベル取得（0=高 / 1=中 / 2=低）、`+0x18` update(actor, flag)、`+0x174` 現在パッケージ取得、`+0x178` 設定、`+0x184` パッケージ取得、`+0x194` 状態更新、`+0x36c` 種別取得、`+0x394` / `+0xbc` / `+0x38c` 状態リセット、`+0x21c` リセット、`+0x594` パッケージ開始。`` | (a) `+8` の値は実測で **0=High / 1=MidHigh / 2=MidLow / 3=Low / Base=0（0..3 の 4 値）**＝「0/1/2」は 3 を欠く → 訂正。(b) `+0x36c` に「GT『GetFormType』は誤り＝process 状態/種別バイト（High 版 `[ecx+0x11D]`）」の注記を追加 |
| `### 17.2 データレイアウト` | L1071 | `` - アクター vtable: `+0x170` 基本NPC取得、`+0x380` 乗騎/対象、`+0x44` 種別判定。`` | `+0x170`（GetBaseForm）/ `+0x380`（騎乗馬 getter）は **b14 と整合・訂正不要**（§1.5 相当の裏付けに引用可） |
| `### 41.3 近接経路 FUN_005febf0 の適用順序（§33.1 の整理）` 内 **「Actor vtable オフセット一覧（§33.1 の補強）」** | L7494–7525（表: L7496–7525） | 行例: `` \| +0x164 \| 戦闘状態 \|``（L7502）/ `` \| +0x170 \| 種別 \|``（L7503）/ `` \| +0x174 \| 座標 \|``（L7504）/ `` \| +0x198 / +0x19c \| 死亡判定 / 魔法チェック \|``（L7505）/ `` \| +0x284 / +0x288 \| AV 取得… \|``（L7511）/ `` \| +0x2c0 / +0x2c4 \| HP 更新通知 / HP 変動 \|``（L7512）/ `` \| +0xec \| レベル／HP 係数 \|``（L7499） | **5 件の反映先**: `+0x08` / `+0xA0` / `+0x190` / `+0x2D0` / `+0x36C` の 5 行を**新規追加**（現行表に行なし）。加えて §4 で挙げた別件候補（`+0x170` / `+0x164` / `+0xEC` / `+0x2C0` / `+0xF8`）を併せて訂正/注記 |
| `### 41.2 近接・反射ダメージ経路 FUN_005f3870` | L7369–7461（該当 L7429, L7439） | `` 005f39c7 MOV EDX,[ESI] / MOV EAX,[EDX+0x2c0] / CALL EAX   ; HP 更新通知（vtable +0x2c0）`` | （任意）`+0x2c0` の Actor 実装は `0x60D0A0` = **`ret` 空**である旨を注記（§4 候補 D） |
| `### §69.3 …（2026-10-25 に V98 で訂正）` | L11771–11773 | `` **【訂正 2026-10-25 (V98)**…`` | **訂正バナーの書式見本**。V121 も同型のバナー（`【訂正 2026-10-26 (V121)】`）を §41.3 直後または §41.7「残課題」に追加する運用が既存スタイルと一致 |
| `### 20.2 結果` | L1669 | `` 0x004d9b40（`__thiscall` `return *(param_1 + 0x1c)`、Actor vtable +0x170）は **67 個**の vtable に共有されている。`` | §41.3 の `+0x170 種別` がこれと矛盾（＝§4 候補 A）。訂正時の**根拠引用元** |

> 検索結果（現行ファイル）: `GetFormID` / `RemoveFromList` / `GetFormTypeID` / `GetFormType` / `IsActor` / `IsReference` / `GetFlags` / `GetScale` / `GetParentCell` は
> **いずれも 0 件**。よって report 側は「誤語の置換」ではなく「§17.2 の補足訂正」と「§41.3 への 5 行追加＋別件候補の訂正」が適用形態。

---

## §3 適用手順チェックリスト（親セッション用）

**A. 事前**
- [ ] 本ファイル（`tmp/re/v121_gt_corrections.md`）を読み、§1 の 5 件をユーザー承認する（+8 の新名称は保留扱いのまま、0xA0 の `0x20000` 表記で統一）。
- [ ] `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS** を確認（本ラウンドの実測は §6 のスクリプトで再現可）。
- [ ] 対象 2 ファイルの改行コードを再確認: `RE_EXE_REPORT.md` = **CRLF 必須** / `GT_CHEATSHEET.md` = **LF**。

**B. .bak 排番**
- [ ] `docs\RE_EXE_REPORT.md` → **`docs\RE_EXE_REPORT.md.bak16`** を作成（現行は `.bak15` のみ。`round_verify.py` は `baks=` を計数するので連番を壊さない）。
- [ ] `tmp\re\GT_CHEATSHEET.md` → **`tmp\re\GT_CHEATSHEET.md.bak1`** を作成（現行は .bak 0 件）。
- [ ] 既存 `.bak*` を上書きしない（`Get-ChildItem` で事前確認）。

**C. 編集（対象はこの 2 ファイルのみ）**
- [ ] `GT_CHEATSHEET.md`: L4 最終更新を `2026-10-26（V121）` に更新。
- [ ] `GT_CHEATSHEET.md`: §4 `### クラス / vtable` 表の直後に **`### 4.1 vtable slot 主要対応（V121 訂正込み）`** を新設し、§1.1〜§1.5 の「訂正後」5 行を貼付（+ b14 確定 slot の対応表も同枠へ収容）。
- [ ] `GT_CHEATSHEET.md`: §5 訂正履歴に V121 行を追加（テンプレ: 下記）。
- [ ] `GT_CHEATSHEET.md`: （任意）§7 の handover 行に「session_B_summary.md:43 の slot map は旧記載（V121 で 4 件訂正）」注記。
- [ ] `RE_EXE_REPORT.md`: §17.2 L1070 を `+8` レベル **0..3 の 4 値**に訂正＋`+0x36c` に GetFormType 誤り注記を追加。
- [ ] `RE_EXE_REPORT.md`: §41.3「Actor vtable オフセット一覧」に `+0x08` / `+0xA0` / `+0x190` / `+0x2D0` / `+0x36C` の 5 行を追加（§4 候補 A–E の訂正を併せて実施）。
- [ ] `RE_EXE_REPORT.md`: `【訂正 2026-10-26 (V121)】` バナーを §41.3 直後（または §41.7 残課題）に追加（書式見本 = L11771）。
- [ ] 編集は**文字単位の置換のみ**（CRLF を LF 化しない。VSCode/エディタの自動改行・BOM 追加に注意。BOM なし UTF-8 維持）。

**D. 証跡テンプレ（GT §5 訂正履歴に貼る 1 行）**
```
| 2026-10-26 | V121: vtable slot 5 件を訂正 — `+8`≠GetFormID（=BaseFormComponent→TESForm キャスト＋flags bit1 同期ヘルパ `0x46B800`、ret 4 で戻り値なし）/ `0xA0`≠RemoveFromList（=flags **`0x20000`** setter `0x46AB00`。旧タスク記載 "bit0x2000" は転記誤り）/ `0x2D0`≠GetFormTypeID（=攻撃・消費系アクション `0x5E4440`、Player `0x666820`）/ `0x36C`≠GetFormType（=process 状態(種別)バイト、High `0x64B090` = `[ecx+0x11D]`、`+0x18C` の転送先）/ `0x190`=**IsActor** のみ（Actor `0x977C50`=1 / plain REFR `0x69D990`=0、"IsReference" 削除）。出典 `tmp/re/v121_gt_corrections.md`（b14 §1.3/§4-2/§5-1 を capstone で再裏取り） |
```

**E. 検証**
- [ ] `python "%TEMP%\opencode\round_verify.py"` → 末尾 **`== RESULT: ALL OK ==`** を確認。
  - 同スクリプトは `docs\RE_EXE_REPORT.md` の **CRLF=True** / `tmp\re\TODO_re49.md` の **CRLF=False** を検査するため、改行破壊はここで捕捉される。
  - `GT_CHEATSHEET.md` は検査対象外 → **手動で UTF-8(LF)・BOM なし・行数を別途確認**すること。
- [ ] （任意）`round_verify.py` の `files = [...]`（L5–14）に `tmp\re\v121_gt_corrections.md` を 1 行追加すると本成果物も自動検査対象になる（%TEMP% 配下の共有スクリプトのため、変更は親セッション判断）。
- [ ] `tmp\re\TODO_re49.md` 行 119 を `[x]` に更新し、detail に「V121 適用済 / GT `.bak1` / report `.bak16` / ALL OK」を追記（**私は編集していない**）。
- [ ] 適用後に `Select-String` で `GetFormID|RemoveFromList|GetFormTypeID` が GT/REPORT に**新規混入していない**こと（=訂正後本文に旧語を残さないこと。ただし「≠GetFormID」のような否定了表記は許容）を確認。

---

## §4 追加走査: 5 件以外の GT 不整合候補

b14 §1.3（4 slot）/ §4-2（結論 2）/ §5-1（0x190・0xA0 の GT 名）の元記述と、
`GT_CHEATSHEET.md`・`RE_EXE_REPORT.md` の現行記述を突合した結果、**5 件以外に 6 件の不整合/要整理を検出した**（いずれも**未編集・候補**）。

| # | 候補 | 現行記述（ファイル:行） | 実測による矛盾 | 確度 |
|---|---|---|---|---|
| A | `+0x170` の語義 | `RE_EXE_REPORT.md:7503` `` \| +0x170 \| 種別 \|`` | impl は `0x4D9B40` = `mov eax,[ecx+0x1C]; ret`（＝**GetBaseForm**）。report 自身の L1669（§20.2）・L1071（§17.2「基本NPC取得」）と**内部矛盾** | **高**（3 箇所で一致する対立証拠） |
| B | `+0x164` の語義 | `RE_EXE_REPORT.md:7502` `` \| +0x164 \| 戦闘状態 \|`` | Actor/Character の impl = `0x4D8370`（formType 0x32/0x33 → `process[vt+8]` → `*(process+0x17C)`＝**GetParentCell**、b14 §1.1 [確定]）。Player のみ別物 `0x65D720` | **高** |
| C | `+0xEC` の語義 | `RE_EXE_REPORT.md:7499` `` \| +0xec \| レベル／HP 係数 \|`` | Actor impl = `0x4D7260` = `[this+0x38]`(scale) ×（base formType==0x24 なら `[base+0x114]`）＝ **GetScale**（b14 [有力]）。§41.3 段1 は `(vtable+0x26c)()*(vtable+0xec)()` を難易度倍率として使用 → 「スケールがダメージ倍率」解釈も成立し得る | 中（[有力] のため要深掘） |
| D | `+0x2C0` の語義（二重意味） | `RE_EXE_REPORT.md:7512` `` \| +0x2c0 / +0x2c4 \| HP 更新通知 / HP 変動 \|`` ＋ ハンドオーバ L43 `vt+0x2C0=GetFlags()→word (bits 0x100/0x200)` | 実測は系統で 3 通り: Process = `0x60CF50`（`xor ax,ax; ret`＝**u16 返し**＝GT の GetFlags に整合）/ Actor・Creature = `0x60D0A0`（**`ret` 空**＝§41.2 の呼出しは no-op）/ Character・Player = `0x60E260`（`[this+0x108]` へ 0.0f 書込の **setter**）。GT と report は別系統の同名 offset を 1 名称で書いている | **高**（b14 §1.4「同一 offset 二重意味」の典型例。GT にも「系統別に書く」注記が要る） |
| E | `+0xF8` の語義 | `RE_EXE_REPORT.md:7500` `` \| +0xf8 \| 死亡状態取得 \|``（§41.3 段2 L7445 `(vtable+0xf8)()`） | Actor/Character/Player の impl = `0x4DD1A0` = **3 つの float 引数**をグローバル `0xB3F9A8/AC/B0` と `fucompp` 比較 → 失敗時にベクトル退避（`sub esp,0xc`…）＝死亡述語の体ではない。b14 では 0xF8 は「未同定」9 slot のうち | 中（**未同定 slot の正体候補**として V143+ に） |
| F | process `+8` のレベル列挙 | `RE_EXE_REPORT.md:1070` `` `+8` レベル取得（0=高 / 1=中 / 2=低） `` | 実測: High = `0x6F7070`(0) / MidHigh = `0x91FEA0`(1) / MidLow = `0x8CF6B0`(2) / **Low = `0x73FD50`(3)** / Base = `0x6F7070`(0)。**値 3 が欠落**（b14 ではなく本次の追加検出）。裏付け: `0x4D8370`（+0x164）が `process[vt+8]` の結果に `cmp eax,1; ja` で High/MidHigh のみ通す | **高**（4 個の vtable を直接読んだ実測） |

**GT_CHEATSHEET 自体について**: §4 の slot 行は L112 の `vt+0x330 / +0x334` 1 行のみで、
これは b14 §6 の実測（`0x5E6190` / `0x6F7070` / `0x6455F0` / `0x5E6110` / `0x6FE080` / `0x6296F0` / `0x68F970`）と
**完全一致＝不整合なし**。同ファイルに 5 件相当の誤記は存在しないため、GT 側の新たな不整合は**検出されなかった**（＝追記型で足りる）。

**ハンドオーバ資料の付随誤記（参考・編集対象外）**:
- `session_B_summary.md:44` 「**[TESObjectREFR+0x58] = base form pointer (TESForm)**」は誤り候補 — `+0x58` は **process ポインタ**
  （report L8846 `` [targetRef+0x58].vt+0x2F0 ``、b14 §1.1 の `0x4D8370` が `process[vt+8]` を呼ぶ経路、GT §4 の process 系 206 スロット超で裏付け）。baseForm は `+0x1C`（`0x4D9B40`）。
- `session_B_summary.md:43` の `vt+0x198=IsDead/IsDisabled` は IsDead のみ b14 [確定]、**IsDisabled は未検証**。
- `session_B_summary.md:49` の「Actor `0xA6E074` / HighProcess `0xA71814` は非整列」は GT §5（V103）で既に**誤りと判定済み**。

---

## §5 新規残点（V143+ への起票粒度）

| 起票案 | 内容 | 優先度の目安 |
|---|---|---|
| V143 | **`+8`（`0x46B800`）の名称確定**: BaseFormComponent→TESForm キャスト＋flags bit1 同期＋`vt+0x90` 呼出し＋`formType`/flag 0x4000 コピーの意味（誰が・何のために呼ぶのか）を呼出し元 843 箇所から分類して命名する。「フォーム比較/同期ヘルパ」の保留を解除 | 中（移植 API 名に直結） |
| V144 | **GetFormID の所在特定**: TESForm 55 スロットに `mov eax,[ecx+0xC];ret` 系は不在（候補 4 件は非登録）。非仮想のヘルパ／別レジスタ経由／inline のどれかを確定させる | 中 |
| V145 | **`+0x2D0` の Actor 版（`0x5E4440`）の用途確定**: `[arg+0x88]&1` / `[arg+0x64]` の条件と `[this+0x100]`（10 引数）の正体、`0x5C1900` / `0x664850` との関係。「攻撃・消費系アクション」の一般化名を決める | 中 |
| V146 | **`+0x36C` の「状態」か「種別」かの確定**: High 版 `[ecx+0x11D]` の**書き込み 12 箇所**（imm 値 0x00/0x01/0x03/0x06 系）と `FUN_00629b70` の**ゲート値 0/4/9** の対応を突き合わせ、GT 本文の語を「状態バイト」か「種別バイト」に寄せる | 低 |
| V147 | **§4 候補 A–F の GT 訂正**（`+0x170`/`+0x164`/`+0xEC`/`+0x2C0`/`+0xF8`/process`+8`）— 本タスクでは**未実施**（分析のみ）。A/B/F は証拠が固いので先行して 1 タスクにまとめられる | **高** |
| V148 | **`+0xF8`（`0x4DD1A0`）の正体**（b14 §3 の未同定 9 slot の 1 つ）＋ `+0x188` / `+0x250` / `+0x2E4` の名称確定 | 低 |
| V149 | **GT_CHEATSHEET §4 に slot 対応表を常設**（§4.1）し、以後の slot 名は GT 経由で only。handover の L43 を参照しない運用ルールを §6 作業ルールに明記 | 中 |
| — | b14 §0.2 の出典引用「`RE_EXE_REPORT` §23.5 / §67.7（既知 slot の根拠）」は**実際には slot 情報を含まない**（§23.5 = メニュー対応表、§67.7 = 残課題）。正しくは §20.2 / §41.3 / §17.2 → b14 側の引用訂正（b14 は編集禁止のため親セッション判断） | 低 |

### §5.1 V153 追記: GT slot 語義の残り（2026-10-27・追記型・上書きなし）

> 本タスク（`TODO_re49.md` serial 153）は本ファイル §5 への**追記**として実施。既存 §0〜§6 は**一切変更していない**。
> 並列タスク（V151/V154）との重複を避けるため、b27（V124）/ v158（V148 再検証）で**解消済み**の点は
> 「解消済み（出典）」とだけ記し、本タスクで**新たに**測定した点のみ証拠を書く。

#### 5.1.0 手法・入力（読取のみ）

- 手法: capstone 5.0.7（`skipdata=True`, `md.detail=True`）、ImageBase `0x400000`、VA = `0x400C00`+off(.text)/`0x400E00`(.rdata)/`0x401400`(.data)。
- 入力（すべて読取のみ）: `tmp/re/cache/{vtable_index.tsv(1,715行), call_targets.json, named_funcs.txt, opcode_table.json}`、
  `tmp/re/{b14_slot_method_map.md, b27_slot_residuals.md, v158_slot_opcode_map.md, GT_CHEATSHEET.md, TODO_re49.md}`、
  `docs/Oblivion_decompiled.c`（部分検索のみ）、`%TEMP%\opencode\v121_step5.py`（V121 の再現用スクリプトを**実行のみ**）。
- 中間スクリプト: `%TEMP%\opencode\v153_1.py`〜`v153_13.py`（＋ `.txt` 出力12本）。docs / GT_CHEATSHEET / TODO / 台帳 / git / Supermemory / sync は**編集0件**。

#### 5.1.1 先行解消の確認（重複禁止）

| 対象点 | 状態 | 出典 |
|---|---|---|
| `+0x2D0` HighProcess 版 = アクション状態 word getter（`[+0x1F4]`、値 -1..6） | **解消済み** | `b27_slot_residuals.md` §4.3 |
| `+0x2D0` Actor 版 `0x5E4440` → `vt+0x100` = `0x4D8760` 実測（一般名は未確定＝V145 継続） | **部分解消** | b27 §6 既存行（本タスクで 0x4D8760 本文を追加 → 5.1.3） |
| `+0x250` / `+0x254` = **犯罪 gold 系メーター（ExtraData type 0x3D float）** getter/setter | **解消済み** | `v158_slot_opcode_map.md` §3-8 / §8-3（本タスクでは**新規測定せず**） |
| `+8` ≠ GetFormID（`0x46B800` = キャスト＋flags 同期） | **解消済み**（名称のみ残） | V121 §1.1（本タスクで名称確定 → 5.1.2） |
| `+0x36C` の本文 `[ecx+0x11D]`・writer12・ゲート0/4/9 の存在 | 存在確認のみ | V121 §1.4（値集合を再測定 → 5.1.4） |

#### 5.1.2 `+8` の名称確定（843 caller 分類）と GetFormID の所在

**(A) 呼び出し本体（`0x46B800`）の再測定 — V121 §1.1 を全項目裏取り**

- 境界: `0x46B7CE..0x46B7CF` = `int3` パディング、その直前 `FUN_0046b7a0` は `0x46B7CB: ret 4` で終了、
  **`0x46B800` は直前の別ルーチンの `ret 4`（`c2 04 00` @`0x46B7FD`）の直後に無パディングで開始**（生バイト `v153_5.txt` §A で確認）。
  台帳 `named_funcs.txt` は次のエントリが `FUN_0046b910` なので、**0x46B800 は台帳上 `FUN_0046b7a0` の範囲内**（v158 §4 の「台帳結合」罠に該当。V121 の `FUN_0046b7a0+0x60` 表記は数値として正しい）。
- キャスト push 順の ground truth: `0x46CA20`（IsDead 内 `TESForm→TESHealthForm`）＝ `push 0 / push dstTD / push srcTD / push 0 / push pv`（v153_3 §A2）。
  これに当てはめ `0x46B800` は **src=`0xB031EC`(BaseFormComponent) → dst=`0xB02F9C`(TESForm)** の `__RTDynamicCast` ✓（b14 §2.4 と一致）。
- 本文（`0x46B800..0x46B85F`、`ret 4`）:
  ```
  pv=arg → RTDynamicCast(BaseFormComponent→TESForm) → 失敗なら ret 4（何もしない）
  if (arg->flags bit1 != this->flags bit1) → this->vt+0x90(引数= arg 側 bit1 の値)   ; 0x46C640 = master 配列登録ヘルパ（call 5件）
  this->formType(+4) := arg->formType(+4)
  this->flags(+8) bit14(0x4000) := arg->flags bit14                                 ; bit14 = v158 §3-5 の「管理外/一時」
  ```
- **裁定（名称）**: **「フォーム同化ヘルパ（AdoptFormIdentity）」= 実体が TESForm なら arg の formType と flags bit14 を this へ同化し、flags bit1 の登録不整合を `vt+0x90` で修復する**[有力〜確定寄り]。
  返り値なし（`ret 4`）・戻り値を返す GetFormID ではない点は V121 と完全一致。

**(B) 843 caller の分類（V143 相当の実測）**

| 計測 | 値 | 出典 |
|---|---|---|
| 厳格ヒューリスティック再現（`call <reg>`/`<reg>=[vt+8]`/`vt=[r]` 8命令内） | **843**（+0xA0=35 / +0x190=613 / +0x2D0=36 / +0x36C=149 も **V121 と 5/5 一致**） | `v153_2.py` = `v121_step5.py` と同一ロジック（本セッション2系統で再現） |
| 異なる無効化条件での再計測（出所・引数カウント付き） | 829（差 -14 は無効化条件の差。**GT は 843**） | `v153_4d.py` |
| 別れ先（enclosing function） | 595 関数 / **opcode impl 関数内の直撃 = 0 件** | `v153_2.py` |
| 出所（provenance, 829 件） | `field-load` 327 / `?` 321 / **`PROC[+0x58]` = 153（base が process → process 版 `+8` = レベルゲッタ系＝別物）** / `GLOBAL-obj` 28 | `v153_4d.py` |
| スタック引数（829 件） | **0引数 300 / 1引数 302** / 2引数 96 / 3以上 131 | `v153_4d.py` |
| TESForm 族候補（1引数 − PROC − GLOBAL） | **289 サイト / 193 関数** | `v153_4d.py` |
| `mov reg, vtable即値` の静的クラス確定サイト | **0 件**（vtable は必ず `[obj]` 経由） | `v153_4d.py` |
| 一次 vtable 1,481 本の slot+8 impl | distinct **439 種**、**`0x46B800` = 85 本**（b14 の「86 vtable」と 1 本差） | `v153_4c.py` |

- **結論**: 「843 caller」は **+8 仮想を全クラスで数えた上限値**であり、うち process / Ni 系・グローバル単体を除いた **TESForm 族候補は 289 に絞られる**。
  さらに **`mov reg, vtable即値` が 0 件**である以上、「843 全数をクラス同定する」ことは本バイナリのコード生成様式
  （`mov edx,[eax]; mov eax,[edx+8]; call eax`）上は**原理的に不可**である（V121 の「上限値」注記を定量的に裏付けた）。
- 対象外バンド（参考）: Ni/havok 帯（0x70–0x90）62、actor/process/player 帯 22、script/opcode 帯 8、core/form 帯 9。
- 上位関数は `FUN_007ac2c0`(17) / `FUN_0044ac20`(10, +0x90 も31) / `FUN_00782040`(8) 等で、`FUN_005123a0`(AddScriptPackage) が 4 件と**唯一の opcode impl 内ヒット**（impl 関数そのものでは 0 件、1 段下のヘルパ内）。

**(C) GetFormID の所在（55スロットに不在 → 所在を確定）**

1. **vtable 仮想関数としては TESForm 55スロットに不在**（`mov eax,[ecx+0xC]; ret` の vtable 登録 = 0 件。V121 の再確認と一致）。
2. **非仮想ヘルパが3種 実在**（本タスク新規）:
   | 関数 | 本文 | 直接 call |
   |---|---|---:|
   | `FUN_004133C0` | `if ([this+0x1C]->[+0x58] bit7) return 0; return [this+0xC];`（**条件付き formID getter**。jump 先 `0x4133D1` が scan 4候補の1つ） | **9** |
   | `FUN_004133E0` | 同一ガード下で `[this+0xC] = arg(≥0)` + `[this+0x20] = [0xA30634]` → al=1（**対の setter**） | **6** |
   | `FUN_0088D370` | `return [this+0xC];`（無条件） | **31** |
   | `FUN_00943410` | `return [this+0xC];`（無条件。scan 4候補の1つ） | **12** |
3. **`FUN_00943410` は vtable 登録 8 件**（`v153_13.py` 全一次 vtable 走査）:
   `TESEnchantableForm`(+0x10) / `BSFaceGenBaseMorphExtraData`(+0x54) / `BSFaceGenModelExtraData`(+0x54) /
   `NiD3DShaderProgram`・`NiD3DPixelShader`・`NiD3DVertexShader`・`NiD3DHLSLPixelShader`・`NiD3DHLSLVertexShader`（各 +0xC）。
   → **「formID 相当の読み取りが仮想化されているのは TESEnchantableForm 族のみ**（NiD3D 系は同パターンだが別フィールド）」。
4. `FUN_005204C0`（scan 残 1 件 `0x5204C9`）= `mov ecx,[ecx+0x3c]; test; je; mov eax,[ecx+0xC]; ret` → **this ではなく入れ子オブジェクトの +0xC**（formID ではない）。
5. **インライン読取 `mov r32,[r+0xC]` = 12,066 箇所**（.text 全体）→ 大半は仮想化せず直接フィールド読み。
6. **スクリプトオペコード表 533 行に GetFormID 相当は 0 件**（`GetIsID`=opid `0x1048` / `GetFurnitureMarkerID`=`0x10A0` のみ＝引数 formID と比較するもの）。

- **裁定**: **GetFormID は「TESForm の仮想関数」ではなく、(a) 非仮想ヘルパ 3種（`0x4133C0`/`0x88D370`/`0x943410`）＋対 setter `0x4133E0`、(b) インライン `[form+0xC]` 読み取り、(c) `TESEnchantableForm::vt+0x10` の部分仮想化、の3系統で提供される**[確定寄り]。

#### 5.1.3 `+0x2D0` Actor 版 `0x5E4440` の用途（b27 との整合 + `vt+0x100` 本文）

- **整合確認（b27 §4 と全一致）**: 厳格ディスパッチ `+0x2D0` = **36 件**（V121/v153_2/v153_10 の3系統で 36）。
  内訳（`v153_10.py`, provenance × 引数）: **`PROC[+0x58]` 0引数 = 22**（= process 版 `0x628200` の状態照会、b27 §4.3 と一致）/
  `PROC` 1〜2引数 = 6 / 不明 0引数 = 5 / その他 3。
  → **36 件の大半（22件）が process 状態照会**であり、Actor 起動系（非 PROC・1引数以上）は **4 件程度の少数**。
  直接 `call 0x628200` / `0x64B090` / `0x46B800` はいずれも **0 件**（cache 実測＝全て仮想経由）。
- **`vt+0x100` = `0x4D8760`（TESObjectREFR 族 12 クラス共有、`ret 0x28` = 10 引数）の本文（新規）**:
  ```
  ebp = arg1 → 0 なら即 return 0
  ebx = FUN_004d6d40(this)  ; this->vt+0x170(GetBaseForm) の結果の formType(+4) 判定:
                            ;   ==0x17 → base+0x24 / (0x23..0x24) → base+0x44 を返す「容器系リスト取得」/ その他 0
  edi = arg2
  call 0x4FBF90(key=this, container=arg2, mask=0x4)   ; mask 4 = **OnDrop**（b16 §2-3 #5 と一致）
                                                       ;   arg2==0 なら emit 本体が container ゲートで**送信されない**
  eax = 0x485E00(param)   ; キャッシュ（0x41E6F0 lookup → 無ければ malloc(0x10)→FUN_00485D50→0x41EC10 登録）
  edi = 0x48FBC0(cached, 11引数)  ; `param_1[2]=0xA30634` / `vt+0x40(0x8000000)` 通知 / `*param_1` リスト while 走査 = **コンテナ内容物の一括処理**
  return edi
  ```
- **`0x5E4440`（decompiled L348801 と asm 完全一致）**: `arg` の `[+0x88]&1` と `[+0x64]!=0` を検査 →
  `this->vt+0x100(arg, 0, 1, 0,0,0,0,0, 1, 0)`（**arg2=0 のため OnDrop emit は必ずスキップ**）→ `0x5C1900`（call 16件）
  → `FUN_00699190([arg+0x64]+0x18, 0, 0)`（内部: `0x415180` 成功時 **`[obj+8] = 5`** にして 0 返し / call 13件）
  → `this == g_thePlayer` なら `FUN_00664850(0)`（**`[player+0x628] = 0`** + `FUN_00664700(0)`、call 14件）。
- **裁定**: `0x5E4440` = **「引数フォームのアクション状態を 5 へ遷移させ、g_thePlayer の `+0x628`（追跡/ロック相当）をクリアするアクション起動系」**。
  b27 の「攻撃・アイテム消費系アクション」を、**状態遷移 + Player ロック解除**へ具体化したもの（一般名 `StartAction(...)` の確定＝V145 継続）。
  **b27 §4.2「0x5E4440 は movsx を持たず 0x628200 経由ではない」は本タスクでも維持**（兄弟実装であって呼出し経路ではない）。

#### 5.1.4 `+0x36C`「状態 vs 種別」の裁定（writer12 vs ゲート0/4/9）

- **所有クラス確定**: getter `0x64B090` は **HighProcess / MiddleHighProcess の vtable +0x36C**、
  writer/reader 関数も High/MidHigh の vtable エントリ（`FUN_00650030`=+0x1B4、`FUN_006545e0`=+0x2F4、
  `FUN_00655590`=+0x560、`FUN_00655bf0`=+0x564、`FUN_006564c0`=MidHigh +0x3F8、`FUN_00651140`=MidHigh +0x3F4）→ **process オブジェクトのフィールド**で確定。
- **writer = 12 件（V121 の「writer12」と完全一致）**: `0x64B536 / 0x64B894 / 0x650279 / 0x654E7E / 0x6553B5 / 0x656F20 / 0x656F29 / 0x656FC6 / 0x657169 / 0x6571C3 / 0x657246 / 0x6572F8`（stack 書き込み3・CRT `setSBUpLow` 2 を除く実数。`v153_6.py`/`v153_13.py` で2系統一致）。
- **値集合の実測（新規・V121 §1.4 の「0/1/3/6」は訂正要）**:
  | 値 | 生成箇所 | 根拠 |
  |---|---|---|
  | **0** | `0x657169`（前段で旧値を bl へ退避 → クリア → 末尾 `0x6571C3` で復元）、`0x6572F8`、`0x657246`（eax==0 判定後）、`0x64B536`（bl=0 の一括初期化: `+0xCC/+0x17C/+0x114/+0x115/+0x11C/+0x38` と同時） | asm aligned context |
  | **1** | `0x656F20: mov byte [esi+0x11d],1`（即値） | asm |
  | **6** | `0x656F29: mov byte [esi+0x11d],6`（即値） | asm |
  | **4 / 9** | `0x6553AB..0x6553B2`: `neg bl; sbb bl,bl; and bl,5; add bl,4` → 条件偽=**4** / 真=**9**。decompiled L417613 `*(byte*)(param_1+0x11d) = (-(cVar2 != '\0') & 5U) + 4;`。加えて `FUN_006545e0` の `0x65464F: mov bl,4` | asm + decompiled |
  | **引数由来** | `0x654E7E`(bl) / `0x6553B5`(bl) / `0x656FC6`(bl, 直後に `cmp bl,4`) / `0x6571C3`(bl 復元) / `0x650279`・`0x657246`(al) / `0x64B894`(別オブジェクトからの**複写** decompiled L412443) | asm |
  | **5** | decompiled L418034 `if (... != '\x05')`（比較のみ＝他からの書込み経路が存在） | decompiled |
- **ゲートとの矛盾は不在**: ゲート `FUN_00629b70` の **0/4/9 はすべて上記 writer で生成される値**（0=クリア、4/9=上記式、4=即値）。
  さらに High 版 `0x628240`（vt+0x2E0）の switch も **`[this+0x11D] ∈ {0,4,9}` を case 条件**に使う（b27 §4.3 と同一トリプル）。
  → **「writer12（0/1/3/6 系） vs ゲート0/4/9」の対立は成立しない**（V121 の値集合記載が不正確だった）。
- **ゲート本体の所有**: `FUN_00629b70` = **HighProcess vtable +0x18**（= GT §17.2 の `update(actor, flag)` スロット）。
  本文: `this->vt+0x36C()` を3回呼び、**0 / 4 / 9 に一致しない場合は `xor al,al; ret 8` で即 false**、一致時のみ `[this+0xC0]`（CC）判定と `vt+0x178` 呼出し、`0x5E0380`（process getter）等へ続く。
- **セーブ永続化（決定的証拠）**: decompiled **L415677（`FUN_00651140`）`FUN_0045b9a0(param_1 + 0x11d, 1)` = save 書出**、
  **L418452（`FUN_006564c0`）`FUN_004534d0(...)` = load 読出**（v158 §3-5 の serializer 2本と同一）。
  さらに L415132 `switch(*(byte*)(param_1+0x11d))`、L418004 `== '\x04'`、L415084 `== '\0'` の分岐実在。
- **裁定**: **`[process+0x11D]` = 「状態バイト」で確定（種別ではない）**。根拠4点 — ①可変（12 writer／値 0,1,4,5,6,9）、
  ②**セーブゲームに保存・復元される**（種別なら保存しない）、③`switch` の対象、④オブジェクト複写時にコピーされる（`0x64B894`）。
  種別（formType）は `TESForm+4` の非仮想 byte（V121 §1.4 と一致）。
  → GT 追記時の語は **「process 状態バイト（High/MidHigh、保存される可変値 0/1/4/5/6/9）」** に寄せる。

#### 5.1.5 `+0xF8` / `+0x188` / `+0x250` / `+0x2E4` の正体（b14 §3 未同定9 slot との突合）

| slot | 裁定 | 主証拠 |
|---|---|---|
| **`+0x250` / `+0x254`** | **解消済み（v158 踏襲・本タスクは再測定せず）**: 犯罪 gold 系メーター＝ExtraData type `0x3D` float | `v158` §3-8/§8-3 |
| **`+0xF8`** | **3系統・二重意味**（一次 vtable 56本）: ①**TESForm 系 33本 = `0x60D0A0`（`ret` 空 no-op）** ②**REFR/Mobile 系 12本 = `0x4DD1A0`（`ret 0x0C` = float3引数）** ③Process 系（Base/Low/MidLow = `0x4943A0` = `xor eax,eax; ret 4`、**High/MidHigh = `0x64B2D0` = `cmp byte[esp+4],0; jne; mov eax,[esi+0xF0]; ret 4` = フラグ引数付き `[process+0xF0]` ゲッタ**） | `v153_8.py` |
| ↳ ②の本体 | (x,y,z) がグローバル `0xB3F9A8/AC/B0`（bss＝初期 (0,0,0)、`0x4B3750` が NiTransform translate へ転写する同一定数）と一致なら **`vt+0x174`(GetPos) で this の現在位置**、違えば引数位置 → **`this+0x44`(BaseExtraList) の extra type `0x14` へ 3 float を `+0xC` に書く**（`0x41F7B0`: `push 0x14; call 0x41E210` → 存在しなければ `0x41EA70` で生成、payload `+0xC/+0x10/+0x14`）、続けて **`this+0x20..0x28`（回転3float）を `+0x18/+0x1C/+0x20` に書く**（`0x41F770` も同一で type `0x14`）→ **ExtraTeleport（destCell=+0x8 / 位置=+0xC / 回転=+0x18）の位置・回転セットヘルパ** [有力] | `v153_8/v153_9/v153_10` |
| ↳ 使用側 | 厳格ディスパッチ **47 件**、うち **`PROC`+1引数 = 32**（＝process 版が主用途）、top = `FUN_005febf0`(5)（report §41.3 の近接経路） → **report §41.3 の「`(vtable+0xf8)()` = 死亡状態取得」は不成立**（[訂正候補]） | `v153_10.py` |
| **`+0x188`** | **二重意味**（一次17本）: ①**MobileObject 系 11本 = `0x977C50`（`mov al,1; ret`）／plain `TESObjectREFR` のみ `0x69D990`（0）** = **「移動する参照（MobileObject 以上）」述語** [有力]（n=1 クラス一覧: Actor, ArrowProjectile, Character, Creature, MagicBall/Bolt/Fog/Spray/MagicProjectile, MobileObject, PlayerCharacter） ②**Process 系 6本は別物**: Base `0x60D030` = `add [ecx+4],eax; ret 8`（2引数カウンタ加算）、Low/MidLow `0x648120`（加算＋負なら0クリア＋`vt+0x180` 通知）、MidHigh/High `0x64F6A0`（`vt+0x184`/`vt+0x570` 呼出し＋`[esi+0xCC]` 加算） | `v153_8/v153_10` |
| ↳ 使用側 | 厳格 **262 件**、**narg=2 が 208 件**（process 版が大半、top = `FUN_0063a020`16 等の HighProcess AI 帯）、narg=0 は 14 件（REFR 述語部）。**b14 §1.2 の「REFR 系 → `0x6F7070`(0)」は実測と不一致（`0x6F7070` は 0x188 に 0 件）** → b14 訂正候補 | `v153_8/v153_10` |
| **`+0x2E4`** | **二重意味**（一次9本）: ①**Actor 系 4本 = `0x5F43E0`（`ret 4` = float 1引数）** = `(this+0x5C).vt+0x30` ガード → `process[vt+0x2D4]` → `[obj+0x68]` 経由 `0x51AC70` の戻り値 `0x14..0x27` を **20ケース→2分岐**に畳む switch（code map `[0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0]`、target = `0x5F442F` / `0x5F4424`）→ `process[vt+0x2B8]` ガードで `0x5F4190` → `0x699C10`/`0x699080`/`0x6A2280` へ float 引数 = **経過時間(delta)による更新系** ②**MidHigh/High = `0x64B080` = `movsx eax,[ecx+0x11C]; ret`（0引数・符号付き byte 取得）** ③Base 系 3本 = `0x6F7070`(0) | `v153_8/v153_10` |
| ↳ 使用側 | 厳格 **20 件**、**`PROC`+0引数 = 17**（＝process 版 `[0x11C]` ゲッタが主用途。b10 の GetKnockedState 系呼出しと整合 [有力]）。Actor 版の float 更新系は 1〜3 件と少数。**b14 §1.2 の「ret void」は `ret 4`（float1引数）→ 訂正候補** | `v153_8/v153_10` |
| （参考）`+0x100` | TESObjectREFR 族 12本 = `0x4D8760`（本文は 5.1.3）。厳格ディスパッチ **7 件** | `v153_10.py` |

**b14 §3「未同定9 slot」の残数**: 0x250 → 解消済(v158) / **0xF8・0x188・0x2E4 → 本次で同定** / 残り **0x180, 0xA4, 0x7C, 0xD4, 0xF0 の 5 slot は未走査**（本タスク範囲外）。

#### 5.1.6 結論（3行）

1. **`+8` = フォーム同化ヘルパ**（arg の formType/bit14 を this へ同化＋`vt+0x90` で bit1 登録修復）で名称確定寄り。843 は全クラス上限値で、TESForm 族候補は **289（193関数）**、静的クラス確定は vtable 即値 0 件により**原理的に不可**。
2. **GetFormID は仮想 GetFormID ではなく**、非仮想ヘルパ 3種（`0x4133C0` 条件付き9件 / `0x88D370` 31件 / `0x943410` 12件・**vtable 登録8件=TESEnchantableForm 族のみ仮想化**）＋対 setter `0x4133E0`（6件）＋インライン 12,066 箇所で提供される。
3. **`+0x36C` の `[process+0x11D]` は「状態」で確定**（12 writer・値 0/1/4/5/6/9・**セーブ保存**・switch 対象）でゲート0/4/9 と矛盾なし（V121 の「0/1/3/6」は訂正要）。`+0xF8`=ExtraTeleport セット/`[+0xF0]` getter、`+0x188`=IsMobile 相当/`process` カウンタ、`+0x2E4`=delta 更新/`[0x11C]` getter に裁定。

#### 5.1.7 残留不明点（起票粒度）

| 起票案 | 内容 | 優先度 |
|---|---|---|
| V153-a | **`+8` 呼出し元側の目的**: 289 候補（193関数）のシステム分類（どの経路が「同化」を呼ぶのか。`FUN_0044ac20` の ESM ロード系10件等） | 中 |
| V153-b | **`FUN_0088D370` / `FUN_00943410` の所有クラス**（無条件 `[this+0xC]` getter、直接 31/12 件。`0x943410` は vtable 登録 8 件を特定済み・残り 43 サイトの caller 分類） | 中 |
| V153-c | **`0x4D8760` の arg9(=1)・`0x48FBC0` の一括処理の中身**（何を容器へ振り分けるか）＝ V145「一般名」の確定 | 中 |
| V153-d | **`[process+0x11D]` の値語彙**（0/1/4/5/6/9 の各意味。4/9 は `(cond?5:0)+4` で生成＝真偽ペアの可能性） | 低 |
| V153-e | **訂正の親セッション反映**: ①V121 §1.4 の「値集合 0/1/3/6」→ **0/1/4/5/6/9** ②b14 §1.2 の 0x188 行（REFR は `0x69D990`/`0x977C50`、`0x6F7070` は0件）と 0x2E4「ret void」→ `ret 4` ③report §41.3 の `(vtable+0xf8)()` = 死亡状態取得 ④b14 §1.1「4候補はいずれも未登録」→ **2件（`0x88D370`/`0x943410`）は登録関数** | **高** |
| V153-f | **`FUN_00699190` の書き先 `([arg+0x64]+0x18)` の所有オブジェクト同定**（`[obj+8]=5` の意味、`arg+0x64`/`arg+0x88 bit0` の所有クラス） | 中 |
| V153-g | `0x5204C0` の入れ子 `+0xC` 読み（`[this+0x3C]` の指す先）の所有 | 低 |

#### 5.1.8 自己検証

| 検証項目 | 期待 | 結果 |
|---|---|---|
| `probe_harness.py selftest` | 4/4 PASS | **4/4 PASS**（着手時・完了時の2回: `bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`） |
| V121 スクリプト `v121_step5.py` の再実行 | +8=843 / +0xA0=35 / +0x190=613 / +0x2D0=36 / +0x36C=149 | **5/5 一致**（本タスク `v153_2.py` も同一5値で2系統一致） |
| `+0x11D` のオブジェクト書込み数 | V121「writer12」 | **12**（2系統一致: `v153_6` aligned 17 − stack3 − CRT2 = 12、`v153_13` 再集計 = 12） |
| 一次 vtable 数 / slot+8 distinct impl / `0x46B800` | 1,481 / — / b14「86」 | **1,481 / 439 / 85**（b14 比 ±1） |
| slot 系統別 impl 分布の合計 | — | `+0xF8` 56 = 33+12+3+2+2+1+1+1+1 ✓ / `+0x188` 17 = 11+2+2+1+1 ✓ / `+0x2E4` 9 = 4+3+2 ✓ |
| cache 突合 | `call 0x4FBF90`=77 | ✓ 77（他: `0x5C1900`=16 / `0x664850`=14 / `0x699190`=13 / `0x46C640`=5 / `0x4133C0`=9 / `0x4133E0`=6 / `0x88D370`=31 / `0x943410`=12 / `0x628200`=0 / `0x64B090`=0 / `0x46B800`=0） |
| `0x628240` switch 実装バイト | b27 の「-1..6 を網羅」 | code map `[0,2,2,1,1,1,1,0]`＋3 target 実測 ✓ |
| 禁止事項（docs / GT / TODO / 台帳 / git / Supermemory / sync の編集） | 0 件 | **遵守**（本ファイル §5.1 追記＋`%TEMP%\opencode\v153_*` のみ） |

#### 5.1.9 証跡一覧（すべて `%TEMP%\opencode\`・既存 docs 非変更）

| ファイル | 役割 |
|---|---|
| `v153_1.py/.txt` | 残 slot の vtable 実測（12クラス×10 slot）＋主要 impl 逆アセンブル＋cache 呼出し数＋`[+0x11D]` 全走査＋GetFormID パターン走査 |
| `v153_2.py/.txt` | 厳格ディスパッチ再現（**843** 確認）＋enclosing function 分布（595）＋slot 併存集計＋GetFormID 候補の所属 |
| `v153_3.py/.txt` | `0x46B7A0` 境界・キャスト push 順 ground truth・+8 サイト文脈サンプル10・opcode impl 内ヒット・depth-1 caller 分類 |
| `v153_4d.py/.txt` | +8 分類本体（provenance × 引数カウント、829 件） |
| `v153_5.py/.txt` | `0x46B800` 本体全文・`[+0x11D]` writer ウィンドウ・decompiled 突合 |
| `v153_6.py/.txt` | **aligned 全アクセス走査（store12 / read30）**＋`FUN_00629b70` 全文＋`0x628240` switch 表 |
| `v153_7.py/.txt` | decompiled 行→FUN 対応・writer 関数の vtable 所有・imm8 ロード・`FUN_00629b70` caller |
| `v153_8.py/.txt` | `+0xF8/+0x188/+0x2E4` の系統別 impl 分布と本体（一次 vtable 全読み） |
| `v153_9.py/.txt` | `0x4DD1A0` tail（`ret 0x0C`）・クラス一覧・`0x4D8760` 本体・`0x5E4440/0x699190/0x664850` decompiled |
| `v153_10.py/.txt` | 5 slot の厳格ディスパッチ分類（provenance×narg）＋ExtraData helper `0x41F7B0/0x41F770/0x41E210` 本文 |
| `v153_11.py/.txt` | `0x4D6D40`（formType→容器リスト）/`0x485E00`（キャッシュ生成）/`0x48FBC0`（一括処理）decompiled |
| `v153_12.py/.txt` | GetFormID 4候補の decompiled 本文＋直接 caller 一覧（31/12 件） |
| `v153_13.py/.txt` | **最終検証**: ヘルパ本文・call 数・全一次 vtable への getter 登録走査（8件）・writer12 再集計 |

#### 5.1.10 Android 移植 (`app/src/main/cpp/script/`) への示唆

（実装はしない・移植担当への指示）`+8` は「他フォームからの formType/bit14 同化＋bit1 登録修復」であり、移植版で
`TESForm::Adopt(other)` 相当が必要かは呼び出し元 289 の分類（V153-a）次第 — まずは不要と見なし、GetFormID は
**仮想ではなく `form->formID` フィールド直接読み（＋`baseForm+0x58 bit7` なら 0 という `FUN_004133C0` の条件）**として実装する。
`+0x36C` は process の**状態バイト**（保存対象・値 0/1/4/5/6/9、High/MidHigh のみ）として別メソッドにし、
`update()` ゲート（0/4/9 のみ進行）もそのまま再現する。`+0xF8/+0x188/+0x2E4` は b14 §7 の二重意味ルールどおり
**REFR 系と Process 系で別メソッド**（ExtraTeleport セット / IsMobile 相当 / delta 更新 vs `[+0xF0]`・`[0x11C]` ゲッタ）に分ける。

---

## §6 証跡（すべて `%TEMP%\opencode\`、既存 docs 非変更）

| ファイル | 生成時刻 | 役割 |
|---|---|---|
| `v121_probe.py`（2,545 B） | 16:23（今回） | 8 クラス vtable × 7 slot のエントリ読出し＋impl 逆アセンブル（§1 の主証跡） |
| `v121_step2.py`（3,643 B） | 16:25（今回） | process 系 `+8`/`+0x2C0`/`+0x2D0`/`+0x36C` 実測、§41.3 用 Actor 系 slot 実測、.text 全域 disasm による直接 E8 呼出しカウント（call 総数 137,421） |
| `v121_step3.py`（2,189 B） | 16:28（今回） | `+0x2C0` 3 系統の本文、`0x5F39C7` 呼出し窓、`+0xF8`/`+0x44`/`+0xEC`/`+0x170`/`+0x164` 本文、FormID getter パターン走査 |
| `v121_step4.py`（1,386 B） | 16:30（今回） | ディスパッチ呼出しの**緩和**ヒューリスティック計数（参考値） |
| `v121_step5.py`（2,236 B） | 16:33（今回） | ディスパッチ呼出しの**厳格**ヒューリスティック計数（`vt=[r]` → `[vt+off]` → `call reg`、§1 の呼出し元数値） |
| `v121_step6.py`（1,160 B） | 16:33（今回） | `call dword ptr [reg+off]` 形のメモリ間接呼出し計数 |
| `v121_step7.py`（1,157 B） | 16:36（今回） | MobileObject / Low / MidLow / MidHigh の残 slot 読出し（0x190・0x18C・0xA0 の系統別確定） |
| `v121_step8.py`（1,573 B） | 16:36（今回） | TESForm vtable 全 55 スロットの先頭ダンプ＋GetFormID パターン不在証明＋`+0x10..+0x24` 本文 |
| `v121_step1.py` / `.txt`（3,633 B / 12,233 B） | **14:59（今回以前の先行実行・私が作成していない）** | 6 slot × 全一次 vtable の impl グループ化＋key impl 逆アセンブル。本分析と**矛盾せず補強**（下記 §6.1） |
| `v121_step2.txt`（12,317 B） | 15:04（先行実行） | **`0x46AB00` の else 分支**（`and [ecx+8],0xFFFDFFFF`）／TESForm vtable 全 55 スロットの Ghidra ラベル付きダンプ／`vt+0x90`=`0x46C640` 本文／`0x5E4440` 128B 全文（§1.1–1.3 の追加証拠はここから） |
| `v121_step3.txt`（21,317 B） | 15:10（先行実行） | `0x5C1900`/`0x699190`/`0x664850` の decompiled 本文、`mov r32,[r+off]` 計数（0x2D0=38 / 0x36C=153 / 0x190=618）、`[reg+0x11D]` writer 12 件、`FUN_00629b70` 逆アセンブル（§1.3–1.4 の追加証拠はここから） |

> **注記（上書きの申告）**: 先行実行（14:59–15:10）の `v121_step2.py` / `v121_step3.py` を、同名で**今回私が上書きした**。
> 先行分の出力 `.txt` は保存されており、内容は本レポートと一致する（むしろ else 分支・Ghidra ラベル・writer 走査で**補強**している）。
> `.txt` は未変更。docs / GT_CHEATSHEET / TODO / memory は一切触っていない。

### 6.1 先行実行 `.txt` から取り込んだ追加証拠

1. `0x46AB00` の **else 分支**（clear 側）— §1.2 の setter 判定を if/else 完全にした。
2. TESForm vtable 全 55 スロットの Ghidra ラベル（`+0x8`=`FUN_0046b7a0+0x60`、`+0xA0`=`FUN_0046aa50+0xB0` ほか）と
   「`mov eax,[ecx+0xC];ret` パターンの vtable 登録 = **0 件**」の独立再確認。
3. `0x5E4440` の 128B 全文（`ret 8`＝2 引数、`this+0x5c` → `0x699190`）。
4. `FUN_00629b70` の `vt+0x36C` 3 重ゲート（0/4/9）と `[reg+0x11D]` writer 12 件。

再現: `python "%TEMP%\opencode\v121_probe.py"` → `v121_step2.py` → `v121_step3.py` → `v121_step5.py` → `v121_step6.py` → `v121_step7.py` → `v121_step8.py`（順不同・読取のみ）。

### 自己検証

| 検証項目 | 期待 | 結果 |
|---|---|---|
| `probe_harness.py selftest` | 4/4 PASS | **4/4 PASS**（2026-10-26 本セッションで実行: `bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`） |
| 5 slot の vtable エントリが b14 §1.3 と一致 | 一致 | **一致**（8 vtable × 7 slot を直接読出し。§1 各表に記載） |
| `0x46B800` / `0x46AB00` / `0x5E4440` / `0x64B090` / `0x977C50` / `0x69D990` の本文 | b14 の記載と一致 | **一致**（§1 に逆アセンブル原文） |
| `+0x18C` → `process[vt+0x36C]` 転送 | b14 §1.2 の主張 | **一致**（`0x5E0EC0` 本文で `jmp edx` を確認） |
| GT_CHEATSHEET に旧 4 語が存在するか | あり→置換 | **0 件**（＝追記型と判定） |
| RE_EXE_REPORT に旧 4 語＋ IsActor が存在するか | あり→置換 | **0 件**（該当は §17.2 / §41.3 の行のみ） |
| round_verify.py の改行検査 | 編集後も ALL OK | 未実施（編集していないため。§3-E に手順を記載） |
| 禁止事項（docs/ / GT / TODO / memory / git） | 編集 0 件 | **遵守**（新規ファイルは本 1 件＋`%TEMP%\opencode\` のスクリプト 8 件のみ） |
