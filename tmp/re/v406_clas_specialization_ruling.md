# v406 — CLAS DATA+0x08（specialization enum）と原版 chargen 適用経路の裁定（#151）

- 採番: **v406**（ai-todo #151 専用予約・probe prefix **v406_1..v406_24 の24本全 EXIT=0**（v406_19 は実行がサーバ再起動で中断も **.out 3086B 完全生成**））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）＋ `tmp/nif_oracle` 外の実データ参照なし（EXE/decompiled のみ）＋ 実データ111 CLAS の21/21一致（依頼記載を継承）
- 依頼経緯: ai-todo #151 の3件（①CLAS DATA+0x08 の確定＝enum 方向/EXE 消費者/GMST 関係/v261 訂正要否・②原版 chargen の適用経路と定数・③+0x30/+0x2C 消費者）。出発点は v261_clas_service_flags.md。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| # | 問い | 判定 | 核心 |
|---|---|---|---|
| **①(a)** | enum の向き | **[確定]** | **0=Combat / 1=Magic / 2=Stealth** — chargen の specialization 選択 step（`FUN_005D6390` param_2==2）が **`addOption(sCombatName, 0)` / `(sMagicName, 1)` / `(sStealthName, 2)` の順**で付与（L339977-339979・machine `0x5D673C..0x5D6765`）＋ GMST 登録表実読（`sCombatName='Combat'→0xB385D8` / `sMagicName='Magic'→0xB385E0` / `sStealthName='Stealth'→0xB385E8`・machine `0x9F0BB0..0x9F0C04`）→ **実データ111件の21/21一致と同一方向** |
| **①(b)** | EXE の消費者 | **[確定]**（主要経路）＋残点 | **reader 本体＝`FUN_006ECC80` = `return *(x+0x40)`**（decompiled L525595-525599）— **MSVC ICF（identical code folding）で1アドレスに一本化**され .rdata の Ni 系 vtable slot35（NiExtraDataController/NiPSys 群）にも同アドレスが登録・**E8=500件超（capped）の多型共用**。**CLAS 型と machine で typed された消費者4経路**: (1) `FUN_005DBBD0` stats メニュー＝`FUN_005E3320(player)→[base+0x104]=class` → getter → `FUN_0052E7E0(spec)→名前` → `sprintf('%s\n\n%s%s')` → menu **0xFB1**（machine 0x5DC244-0x5DC26D）(2) `FUN_00596CF0` **chargen レビュー**＝esi=class form → getter → name→**0xFB6**・raw spec→**0xFC0**・major skill 7個→0xFAF+i/0xFB9+i（machine 0x596D60-0x596D78）(3) `FUN_005222D0` 成長/再計算 fn＝getter ×2（`cmp spec,ebp → += double 5[0xA3F3F0]`／spec を `FUN_00547F80` へ引数）machine 0x522540/0x522662・**呼出し8件に chargen `FUN_00596980` を含む** (4) `FUN_006684B0` levelup レポート（class 系 reads・major getter） |
| **①(c)** | GMST `sSpecialization` 等との関係 | **[確定]** | **`sSpecialization`**(key 0xA5E194・既定 0xA5E1A8="You will gain +10 in all specialization skills. Those skills will also increase faster."）＝**chargen の specialization 選択 step のヘルプテキスト（menu 0xFB3）**（L339976）。**`sClassSpecialization`**(0xA5D558・既定 0xA5D570='Specialization: '）＝**stats メニューのラベル**（0xFB1 sprintf・L343005-343010）。**`sSpecNameCombat/Magic/Stealth`**→globals **0xB364E8/F0/F8** ＝ **`PTR_DAT_00B10D90[spec]` 名前テーブル**で `FUN_0052E7E0(spec)` が `*global[spec]`（＝GMST value 文言）を返す（machine 0x52E7E0-0x52E7F9・table 内容 v406_8(d) 実測）。登録方式＝起動時登録表 fn（head 0x9EFEB0）の `push default; push key; mov ecx, global; call 0x419A70`（`FUN_00419A70` = GameSetting 登録: `{[0]=default value, [4]=key}` + map insert・L16472-16501）＋ dtor thunk（`mov ecx,global; jmp 0x4055E0`）を `push thunk; call 0x981FB4`(atexit) で登録 |
| **①(d)** | v261「+0x40 未読」判定の訂正要否 | **訂正要 [確定]** | v261 は **inline `mov [x+0x40]` スキャンのみ**（GetClass 直後38箇所＋窓内）で **call 経由 getter を見落とし**。実際は `FUN_006ECC80` 経由で上記4経路が読む。**＋ `form+0x64`(DATA+0x2C) の「未読」も同様に訂正**: `FUN_005222D0` L216914 `FUN_00468230(*(param_1+0x104)+100)`＝machine `0x522706: mov eax,[edi+0x104]; mov ecx,[eax+0x64]`（eax=class form 確定）→ actor 側へコピー [確定] |
| **②** | 原版 chargen の適用経路と定数 | **[部分確定]** | **PlayerCharacter 構造 [確定]**（ctor `FUN_0066A740`・vtable 0xA73A0C RTTI `.?AVPlayerCharacter@@`）: `+0x130`=**skill 値 float×21**／`+0x5B0`=**保留増分 array ptr**／`+0x5B8..+0x5BA`=**spec 別 advancement カウンタ3本**（Combat/Magic/Stealth・report "Specialization Counts" が実読 L428006-428016）／`+0x158+idx*4`=per-skill usage／`+0x184`=major 達成カウンタ／`+0x660`=リセット系／`+0x7A4`=skill 配列2。**適用の肯定系＝`FUN_005222D0` tail の class→actor 3連鎖 [確定(命令)]**: `class[+0x64]`→`FUN_00468230`／`class[+0x68]`(trainer・`FUN_0051BEB0`)→`FUN_004684C0`／`class[+0x69]`(`FUN_004A9700`=byte[+0x69])→`FUN_00468270`、いずれも `lea esi,[edi+0x68]`（actor[+0x68] オブジェクト）へ・**class 無し時は 0 セット**（L216913-216921）。`FUN_005222D0` の呼出し8件＝record load `0x4471A0`・script opcode 系3・skill band `0x523F40`・**chargen `FUN_00596980`**・menu `FUN_005C28F0`×2。**skill 値の書手は4系統のみ [確定(否定側)＋肯定側)]**: ①`FUN_0065FA40` setter（**E8=2 のみ**: `FUN_0050D020`=advance 系 fn（isClassSkill 判定で `[player+0x184]++`）・`FUN_006684B0`=levelup レポート）②`FUN_00668030` 全リセット(=0)（Player ctor と `FUN_0066C200`）③`FUN_00668070` 使用更新④`FUN_0066C200` **保留適用**（`[+0x5B0][idx]` を加算・spec カウンタ reset L431191-431250）。**増分の唯一入口 [確定]**: `FUN_00668C30`（skill record の **`[+0x38 + param_2*4]` float テーブル** ×倍率 → `FUN_00668B30` で `[+0x5B0][idx] +=`）→ **E8=0＝`.rdata 0xA73DA8` のテーブル登録＝間接（opcode/vtable）呼出し**。**否定 [確定(否定側)]**: **major skill getter `FUN_0051BF00` の E8=5 は growth(`FUN_005222D0`×2)/review(`FUN_00596CF0`×2)/report(`FUN_006684B0`) のみ＝major をアクターへ書き込む経路は存在しない**。**「属性+10／specialization スキル+10／major 初期値」の明示適用 fn は24 probe で未特定 → [未確定]**（残点 V-406-1）。**「faster」側 [確定(命令)]**: `FUN_005222D0` が class spec を比較し **`fadd qword [0xA3F3F0] = +5`**（spec 一致時・0x52254D）／**非 major 減衰 `×0.2`（`_DAT_00A38538`・L216823-24 `if (local_c != 1) fVar1 *= 0.2`）**／spec を `FUN_00547F80` へ引数（0x522662）。`0xB362E4/0xB362EC`（attr ボーナス候補）は **.data の実行時グローバルで定数非保持 [確定(否定側)]**（PE 初期値が不定・書手未追跡＝残点）。**flags bit0 autocalcs** は v261 の7消費者調査（ロード窓 `FUN_0044CDF0`／`FUN_00505810`／クラス一覧 count `FUN_00596BC0`／staging `FUN_00597340/73F0/7651`／`FUN_005DBBD0`）を**継承・本件新規調査なし [部分確定(v261 出典)]** |
| **③** | +0x30 消費者と +0x2C の意味 | **+0x30=[確定]／+0x2C 消費=[確定]・意味=[未確定]** | **+0x30 trainer skill index**: getter `FUN_0051BEB0`（`movzx [ecx+0x68]; add 0xC`）・**E8=1 で唯一消費者＝`FUN_005222D0`@0x522726** → `FUN_004684C0(&actor[+0x68], skillId)` で actor へコピー（v261 の記述を E8 全数で再確認＝一致）。**+0x2C(form+0x64)**: 消費者＝**`FUN_005222D0` L216914 `FUN_00468230(class[+0x64])` [確定]**（v261「未読」は**誤り・訂正**）＋ class 無し時 `FUN_00468230(0)`（L216921）→ **意味（DATA+0x2C の正体）は [未確定]**（残点・UESP 48/52byte 差分の末尾領域と推定されるが裏取り不可）。付帯: **`class[+0x69]`(DATA+0x31) も `FUN_004A9700`→`FUN_00468270` で actor へ [確定(命令)]** |

---

## §2 証拠

### §2.1 ① enum 方向と消费者チェーン（v406_2..v406_13）

**chargen specialization step（`FUN_005D6390` param_2==2・L339972-339980）**:
```c
FUN_0058ceb0(0xfb1, _DAT_00a379b4);        // ラベル表示
FUN_0058ced0(0xfb3, DAT_00b38640);         // sSpecialization ヘルプ → 0xFB3
FUN_005d6270(DAT_00b385d8, 0);             // sCombatName  → value 0
FUN_005d6270(DAT_00b385e0, 1);             // sMagicName   → value 1
FUN_005d6270(DAT_00b385e8, 2);             // sStealthName → value 2
FUN_005d5d40();
```
**GMST 登録表実読（machine 0x9F0BB0..）**: `push 'Combat'; push 'sCombatName'; mov ecx,0xB385D8; call 0x419A70` ／ `push 'Magic'; push 'sMagicName'; mov ecx,0xB385E0` ／ `push 'Stealth'; push 'sStealthName'; mov ecx,0xB385E8` ＋隣接 `sCombatDescription→0xB385F0`。→ **(sCombatName,sMagicName,sStealthName) = (0,1,2)** [確定]。

**getter 本体**: decompiled L525595-525599 `FUN_006ECC80(param_1){ return *(param_1+0x40); }`。vtable 登録は Ni 系 slot35（v406_8(b)）＝**ICF 共用**。E8 は cap500 で打ち切り（v406_9/v406_12）。

**typed 消費者（machine 確認済み）**:
- `FUN_00596CF0`（v406_10(c)）: `mov ecx,esi; call 0x6ecc80; push eax; call 0x52e7e0; push 0xfb6` — esi = arg（class form・`[esp+0x10]`、無ければ `[edi+0x3c]`）。
- `FUN_005DBBD0`（v406_3）: `mov ecx,[0xb333c4](player)→call 0x5e3320→mov ecx,eax→call 0x6ecc80→call 0x52e7e0` ＋ `uVar5=DAT_00b383d8`(sClassSpecialization ラベル) → `sprintf("%s\n\n%s%s")` → `FUN_0058ced0(0xfb1,...)`。
- `FUN_005222D0`（v406_13(c)）: `0x522540 call 0x6ecc80 → cmp eax,ebp → jne; fld [esp+0x14]; fadd qword[0xA3F3F0](=5.0)` ／ `0x522662 mov ecx,ebp; call → push eax → call 0x547F80`。
- `FUN_005E3320`（L347984-347998）: `if (FUN_004D7F80()) { obj = virtual+0x170(param_1); if (obj) return [obj+0x104]; } return 0` — **actor→class form 経路 [確定]**（v261 既報と一致）。

**FUN_0052E7E0**（machine 0x52E7E0）: `cmp eax,2; ja →0; mov eax,[eax*4+0xB10D90]; test; je→0; mov eax,[eax]; ret` — テーブル内容 = `0xB364E8/F0/F8`（＝`sSpecNameCombat/Magic/Stealth` の global cache）→ **`[eax]` = GMST value 文言** [確定]。

### §2.2 ② PlayerCharacter skill 構造と適用（v406_15..v406_23）

- **ctor `FUN_0066A740`**: `*param_1=&PTR_FUN_00a73a0c` → RTTI **`.?AVPlayerCharacter@@`** [確定]（v406_20(b)）。`FUN_00668030`（skill 全リセット）の caller＝本 ctor と `FUN_0066C200`。
- **保留適用 `FUN_0066C200`（L431178-431251）**: `[p+0x660]=0` → usage リセット（`FUN_00663C50`×21）→ `FUN_00668030`（値全0）→ `[p+0x5B8](word)/[+0x5BA](byte)=0`（spec カウンタ reset）→ **skill 0x0C..0x20 ループ: `inc = [[p+0x5B0]+idx*4]` → `[p+0x130+idx*4] += clamp(inc)`（L431222-431231）** → class 経路 `[p+0x7A4+idx*4]` と比較し閾値未満なら `FUN_00668070`。
- **増分入口 `FUN_00668C30`（L429232-429257）**: `rec=FUN_00446AF0(FUN_00565C50(2,skill))` → `amount=[rec+0x38+param_2*4]`（float テーブル）→ `if (FUN_005F1910(skill)<100) amount=(param_3==0)?amount:param_3*amount` → `FUN_00668B30(skill, amount, rec, 0)`。**`FUN_00668B30` の E8=1（=FUN_00668C30）**、**`FUN_00668C30` の E8=0・.rdata 0xA73DA8 参照＝間接登録** [確定]。
- **`FUN_0050D020`（v406_20(c) machine）**: opcode 検証（`call 0x4FAE80` 9引数）→ `FUN_00446AF0([0xB33A98]+skill)` で skill record → 値 get/set（`FUN_0065FA90`/`FUN_0065FA40`）→ `[player+0x158+idx*4]++` → **`FUN_005E3320(player)`→`FUN_0051C090(class,skill)`→ true なら `[player+0x184]++`（major 達成カウンタ）** [確定(命令)]。decompiled 未収録（Ghidra 注記なし）。
- **class→actor 3連鎖（`FUN_005222D0` tail・v406_23）**: machine `0x522706 mov eax,[edi+0x104]; test; je; mov ecx,[eax+0x64]; lea esi,[edi+0x68]; push ecx; mov ecx,esi; call 0x468230` / `mov ecx,[edi+0x104]; call 0x51beb0; push; mov ecx,esi; call 0x4684c0` / `mov ecx,[edi+0x104]; call 0x4a9700; push; call 0x468270` — decompiled L216913-216921 と一致（class 無し時 `FUN_00468230(0)`）。
- **major getter 全数**: `FUN_0051BF00` E8＝**5件のみ**（v406_10(a)）＝`FUN_005222D0`×2／`FUN_00596CF0`×2／`FUN_006684B0` — **書き手0件 [確定(否定側)]**。
- **定数実測（v406_24）**: `0xA3F3F0`=**double 5.0**（spec 一致時の成長加算）／`0xA38538`=**double 0.2**（非 major 減衰）／`0xA2FAA0`=0.5／`0xB02E24`=int **10**（menu 配置ステップ・+10 初期適用とは無関係）／`0xB362E4/0xB362EC`＝.data 実行時グローバル（初期値不定＝定数ではない）。

### §2.3 ③ +0x30 / +0x2C（v406_17・v406_23）

- `FUN_0051BEB0` **E8=1（0x522726）のみ** [確定]（v406_17(d)）。context: `mov ecx,[edi+0x104]; call 0x51beb0; push eax; mov ecx,esi(&actor+0x68); call 0x4684c0`。
- `class[+0x64]`: `mov eax,[edi+0x104]`（直前・typed）→ `mov ecx,[eax+0x64]` → `FUN_00468230(&actor[+0x68], value)`、else `FUN_00468230(0)` [確定]。**v261 フィールド表の +0x40/+0x64「未読」行はいずれも訂正**。

## §3 残点 **V-406-1..6**

1. **【高】「+10 初期適用」（属性/spec/major 初期値）の fn 未特定** — `FUN_00668C30` の間接登録テーブル（`.rdata 0xA73DA8`）の同定（PlayerCharacter vtable 領域か opcode 表か）と opcode id、`FUN_0050D020` の opcode 特定（9引数検証 `FUN_004FAE80` の arg 型）。＋「sSpecialization 文言の +10 が実際に実行される経路」の有無。
2. **【中】`FUN_005222D0` の正式名と属性計算ブロック** — param_2==0 分岐の `FUN_005479D0(FUN_00468AD0(0,5,3,2)) → FUN_004672B0(&actor[+0x24])` 経路の意味、実行時グローバル `0xB362E4/0xB362EC` の書手。
3. **【中】`FUN_006ECC80` の残り E8=500+ の CLAS 型再 typed**（特に actor/chargen/menu 帯 0x5Exxxxx/0x5Bxxxxx/0x58xxxxx）。
4. **【低】DATA+0x2C（form+0x64）と DATA+0x31（form+0x69）の意味** — UESP 48/52byte 差分の末尾領域候補だが本文未取得（503）→ 裏取り待ち。
5. **【低】`PTR_DAT_00B10D90` の所有 fn と spec 名前テーブルの書手**（起動時初期化の所在）。
6. **【低】`FUN_00668B30` の第4引数(0)と `FUN_0066C200` の `[p+0x7A4]` 配列の意味**（skill 閾値/累積か）。

## §4 検証・自己批評

- probe: **v406_1..v406_24 の24本全 EXIT=0**（v406_19 はサーバ再起動で実行中断も .out 3086B 完全）。v 採番 v406 消費（次=v407=#153⑥-11 以降・着手時に再確認）。
- **決定打の作り**: (i) **文字列 xref から GMST 登録表を実読**（key/default の push 実読で global cache を特定）→ (ii) **global cache の direct 読取 census** で runtime consumer を特定 → (iii) **getter の decompiled 実読（`return [x+0x40]`）＋ICF 共用の認識** → (iv) **E8 全数＋machine の arg 経路（ecx=this の source 追跡）で型付け** → (v) **書き手側（setter/reset/update/adder）の E8 全数と間接登録の否定確認** → (vi) **定数の PE セクション実測**。
- 妥当性チェック: ①enum 順序＝(登録表・chargen 付与・実データ21/21) 3方向一致 — ②消費チェーン＝(decompiled・machine・GMST テーブル内容) 3方向一致 — ③major getter の否定＝E8 全数＝5 件で3系統（growth/review/report）に一致 — ④+0x30 は v261「唯一消費者 0x522726」を E8 全数で再確認＝一致 — **矛盾なし**。
- docs との不整合: **v261_clas_service_flags.md のフィールド表2行（+0x40/+0x64「実行時コードから未読」）は本裁定で訂正要**（call 経由 getter の見落とし）。docs/GT への反映は #47/#20 一括承認待ち。
- **教訓**:
  1. **MSVC ICF（identical code folding）**: `return [x+disp]` 型の trivial getter は**1アドレスに一本化**され、別クラスの vtable slot にも同アドレスが登録される → 「1 fn ＝ 1 意味」仮説は禁止。**型は caller の arg 経路（ecx/this の source 追跡）で typed** する。
  2. **「実行時未読」判定は call 経由 getter でも再検証**: inline `mov [x+disp]` スキャンだけでは getter 経由の読手を取りこぼす（本件で v261 の2行が誤りと判明）。
  3. **GMST consumer 特定の3段手順**: 登録表の key/default 実読 → global cache 特定 → **その global の direct 読取 census**（文字列 xref は登録部1件で runtime consumer は全部 global 経由）。
  4. **PE セクションで「定数」と「実行時グローバル」を区別**: .rdata＝定数実測可、.data/.bss 未初期化は初期値不定＝「定数」と断定しない。
  5. **decompiled 未収録 fn（Ghidra 注記なし）は machine 直 decode で補完**（`FUN_0050D020`/`FUN_00597540`/`FUN_0059FF60` が該当）。
