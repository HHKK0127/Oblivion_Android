# v200_process_list_residuals.md — V187: process/list 束（KnockedState 値名・+0xE4..F0 元素型）

対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B / SHA256 A8F3…DC6）。
出発点: `tmp/re/v193_opcode_gt_residuals.md` §4-7/§4-8（＝§2-177d/§2-177e）、`tmp/re/v177_gt_slot_residuals2.md` §4/§5。
中間スクリプト: `%TEMP%\opencode\v200_1.py` / `v200_1b.py`（＋出力 `v200_1_out.txt` / `v200_1b_out.txt`）。

## §1 手法 / 入力

- probe_harness `Ctx`（decompiled 1,066,496 行の `dec_index`/`enclosing_fun` 部分検索のみ）、v172_common の capstone disasm・raw E8 一括走査（.text 全域 2,258,215 命令相当のバイト列）。
- `tmp/re/cache/call_targets.json` は参照のみ（再走査しない）。
- selftest 着手時 4/4 PASS。

### 課題

1. **[process+0x11C] KnockedState**: 値名の比較サイト文脈命名 / L286800 `0x80000000` センチネルの所属 / decompiled `+ 0x11c)` 全行走査。
2. **HighProcess +0xE4..+0xF0 リスト族の元素型**: 兄弟ゲッタ呼び出し元の deref / HighProcess ctor（FUN_0064B400 帯）全読 / alloc サイズ・vptr による裁定。

## §2 証拠・分析

### §2-1 [process+0x11C] の byte 書手カタログ（本タスクで確定）

raw バイト走査（C6/C7/88/89 + modrm disp8/disp32 = 0x11C）＋ decompiled `+ 0x11c)` 全行走査の結果、process 帯（High/MidHigh）の byte 書手は以下に確定（v177 の「byte writer は FUN_0064B400 初期化 1 件のみ」は**訂正**。byte 書手は 14 箇所）:

| VA | 値 | 文脈（所属・直前文脈） |
|---|---|---|
| `0x64B52D` | 0 | `FUN_0064B400`（HighProcess ctor）一括初期化（+0xCC/+0x17C/+0x114/+0x115/+0x11C/+0x38 同時） |
| `0x65454D` | **2** | `FUN_00654420`（vt+0x20 呼出し前。`[0xB333C4]`=g_thePlayer 比較帯） |
| `0x654631` | 0 | `FUN_006545E0`: `call 0x5E0DC0`（GetKnockedState 本体候補）→ `cmp eax,6; je` で「6（死亡）なら無変更」→ 0 書き |
| `0x6546D0` | **3** | `FUN_006545E0`: vt+0x1A0 テスト真（`push 0; push 1` の直前） |
| `0x6548F2` | **5** | `FUN_006545E0`: vt+0x9C 呼出し後（`pop ebx` の後） |
| `0x654A00` | **3** | `FUN_006545E0`: `0x5F0270`（アニメ再生系?）真 → 旧値を bl へ退避 → 3 → 途中 `0x5E12B0` 呼出し → **0x654A19 で bl（旧値）へ復元** |
| `0x654A3E` | **5** | `FUN_006545E0`: vt+0x9C 呼出し後（test eax 前） |
| `0x654BCA` | 0 | `FUN_006545E0`: `0x683CB0`（havok/物理?）呼び出し後 |
| `0x654C08` | **6** | `FUN_006545E0`: `lea ecx,[esi+0x44]; call 0x424870`（死亡/削除系）後 |
| `0x654C2B` | **3** | `FUN_006545E0`: `[ebp+0xCC]==0 && [ebp+0xD0]==0` ガード後 |
| `0x654C46` | 0 | `FUN_006545E0`: `call 0x472EA0`（何かの述語）真 |
| `0x654CE8` | 0 | `FUN_006545E0`: vt+0x9C 呼出し後（`[ebx]`=別プロセス） |
| `0x623460` | 0 (dword) | `FUN_006231E0`（dword 幅。`push 9; call 0x612DA0` → `call 0x61C6E0` 後） |
| `0x629444` | arg (byte) | `FUN_00629440`（setter 本体: `mov al,[esp+4]; mov [ecx+0x11c],al; ret 4`）＝**外部 setter 経路** |
| `0x65698E` | stack byte | `FUN_006564C0`（**load 側**）: `mov al,[esp+0x6c]; mov [esi+0x11c],al` → セーブ復元あり＝**v177 の「セーブ非保存」を訂正** |
| `0x59A3BD` | eax (dword) | `FUN_00599EE0`: g_thePlayer[+0x11C] へ dword 書き（player 特例。`call 0x422DC0`→`call 0x9828C0` の計算結果） |

- decompiled での byte 書き換え式（L417366, `FUN_006545E0` 内）: `*(char*)(param_1+0x11c) = ((char)param_1[0x47] == '\x04') * '\x02' + '\x01'` ＝ **旧値 4 なら 3、否则 1**。ここが **値 1 の唯一の生成元**（setter 経由を除く）。

### §2-2 値の語義命名（比較サイト文脈）— [確定]

`FUN_006545E0`（vtable slot **+0x2F4** = knockdown 更新状態機械、High/MidHigh 共有）の decompiled 全読（L417231-417449、`v200_1b_out.txt` §B2）で、**状態遷移図と各値の文脈が完全に取得**できた。値名は遷移先・出口条件から命名:

| 値 | 命名 | 生成元（byte 書手） | 遷移先 / 出口条件 |
|---|---|---|---|
| **0** | **None（通常）** | ctor `0x64B52D` / `0x654631`（戦闘相手非死亡時）/ `0x654BCA`（物理解除後）/ `0x654C46`（`FUN_00472EA0` 真）/ `0x654CE8`（vt+0x9C 後） | 初期値・復帰値 |
| **1** | **KnockedDown（倒れ中・ragdoll 到達待ち）** | L417366 の計算書手 `((old=='\x04')*2)+1`（4 なら 3、否则 **1**）／L417315 で state1 は `FUN_004FBF90(0, actor+0x44, 0x40)`＝**OnKnockout イベント再送出** | case 1: vt+0x28 角度条件（`FUN_00402BD0` 比較・`_DAT_00A492B8`=2π ラップ）到達で 3→5 |
| **2** | **KnockBack（衝撃吹き飛ばし直後）** | `FUN_00654420` の `0x65454D`（vt+0x20 呼出し直前。impulse 受信ハンドラ、g_thePlayer 比較・`FUN_0088D070(1,1,0)`・`FUN_005364B0` で速度伝播） | L417315: state2 も OnKnockout 再送出 → case 2/4 で **(old==4)*2+1 ⇒ 2→1** |
| **3** | **GettingUpPrep（GetUp アニム探索中）** | `0x6546D0`（vt+0x1A0 真）/ `0x654A00`（`FUN_005F0270` 真・旧値 bl 退避→`0x654A19` で復元）/ `0x654C2B`（`[ebp+0xCC]==0 && [ebp+0xD0]==0`）/ case 5 の別経路 | case 3: **`FUN_00521450(actor,0)` = GetUp アニム探索** → 見つかれば vt+0x9C(0,0)→**5**、無ければ文字列 **`"No 'GetUp' animation found for actor '%s'."`**（0xA72D08）→ vt+0x2F8 |
| **4** | **KnockDown 開始（アニメなし ragdoll 開始）** | L417291（asm `0x65471F: mov [edi+0x11c], bl`、直前で bl=4 ロード）: state==0 かつ「alpha<0 ∨ vt+0x1A0 ∨ 0x5E0DC0==6」→ vt+0x1A0 **偽**なら 4 + `FUN_008AB440` 速度ベクトル + `FUN_004FBF90(...,0x40)`（**OnKnockout 送出**）+ `FUN_008A5580(0)` | case 4 → **(old==4)*2+1 ⇒ 4→3** |
| **5** | **GettingUp（起き上がりアニメ再生中）** | `0x6548F2` / `0x654A3E`（いずれも vt+0x9C 呼出し後）/ case 1・case 3 の出口 | case 5: `FUN_004711F0`＋`FUN_004706E0` アニム状態条件 → Bip01 Head/Neck/Spine1 探索 → `FUN_00683CB0` → **0** + vt+0x1E8(float) → `FUN_008A5580/008AB8A0/00424870` → **6**。条件不満なら `[ebp+0xCC]==0 && [ebp+0xD0]==0` で **3 へ戻る** |
| **6** | **Recovered（復帰処理中）** | `0x654C08`（`lea ecx,[esi+0x44]; call 0x424870` 直後） | case 6: `FUN_00472EA0()` 真で **0**。L417268/275 では `FUN_005E0DC0()==6` が「割り込み抑止」条件として頻出 |

- **補助実測**:
  - `FUN_00654420`（slot **+0x2F0**、vt+0x2F0）は入口ガード `state==0 ∧ vt+0x198(0)==0 ∧ vt+0x278()==0`。処理は vt+0x36C（家具状態）→ 家具脱出（`FUN_005E4140`/`FUN_005F0410`）→ **state=2** → vt+0x20 → g_thePlayer 判別で `FUN_00660110(0)`/`actor[0x3C]` → `FUN_0088D070(1,1,0)` → `FUN_00707370(0,0)` → `FUN_005364B0(node, impulse, power)`。
  - **event mask 0x40 = OnKnockout が確定**: opid 0x000C `OnKnockout` impl `0x5042D0` の本文で `push 0x40; call 0x4FB5F0`（check）を実測（`v200_5_out.txt` §F3）。state 1/2 の再送出・state 4 の初回送出はすべてこの mask。
  - `FUN_005E0DC0` = **`mov eax,[ecx+0xB0]; ret` の 2 命令 getter**（E6: 直接 caller 56 件）。`== 6` 比較は割り込み抑止ガード。[actor/process+0xB0] の正式名は [残]。
  - **vtable slot 対応（D5, High 0xA71814 / MidHigh 0xA72684 共有）**: `+0x2E4`=`0x64B080`(+0x11C getter) / `+0x2E8`=`0x629440`(+0x11C byte setter、`mov al,[esp+4]; mov [ecx+0x11c],al; ret 4`) / `+0x2F0`=`0x654420` / `+0x2F4`=`0x6545E0`。
  - **setter 0x629440 の発見は v177 の「setter 引数経路 L285538」とは別物**（L285538 は 0x57 帯の別クラス）。process 系 +0x11C の外部 setter は本 slot+0x2E8。

### §2-3 L286800 の `0x80000000` センチネル — [確定: **KnockedState と無関係・別クラス**]

- **所属関数 = `FUN_0057F7A0`**（`v200_1_out.txt` §T1-D で全文取得）。これは **UI/メニューの DirectInput キー入力処理**であり、process とは無関係:
  - `param_2==2`（キー離し）: `+0x118` のビット解除（DIK 0x2A/0x36=Shift → mask ~4 / 0x38/0xB8=Ctrl → ~1 / 0x1D/0x9D=Alt → ~2）、`+0x114=0`、`+0x120=DAT_00B33EA0`（tick）、`+0x124=0`、**`+0x11c=0`**。
  - `param_2==1`（キー押下）: `FUN_00403CF0(DIKコード, [obj+0x114])` の戻り値で分岐 — **0x8 → `+0x11c = 0x80000000`（=VK_BACK 8 の押下）**、0xD → return `0x80000008`（=VK_RETURN）、0x1B/0x7C は別枝。
  - **`FUN_0057DC60`（update）**: `+0x11c == -0x7FFFFFFF` かつ `FUN_004032D0(DIK_LEFT=0xCB,0)==0` → 0 クリア / `&DAT_80000002`∧DIK_RIGHT(0xCD) / `-0x7FFFFFFD`∧DIK_UP(0xC8=200) / `-0x7FFFFFFC`∧DIK_DOWN(0xD0) — つまり **`0x80000001..4 = Left/Right/Up/Down の押下中マーク`**、タイムアウトは `+0x120/+0x124` と `DAT_00B33EA0` で計算。
  - **`FUN_004032D0(key, mode)` = DirectInput キー状態テスト**: `test byte [eax+ecx+0x18f4], 0x80`（DIK 状態配列 +0x18F4、mode 0..3 の jump table）。
  - **setter**: `FUN_0057DC30` = `+0x120=tick / +0x124=0 / +0x11c=arg`。
- **裁定**: L286800 の `0x80000000` は **UI キー入力オブジェクトの +0x11c（押下中特殊キー記憶、VK_BACK=8 に 0x80000000 を OR した形式）**。KnockedState（byte 幅・0..6）とは**オフセットが偶然一致した別クラスの dword フィールド**。v193 §2-177d の「Knocked 系の特殊値である可能性 [推定]」は**否定**で確定。オーナークラスの正式名（Menu/Console 系 0x57 帯、caller 0x5834E3 等）は [残]。

### §2-3b process 帯 `+ 0x11c)` 全行走査の整理（タスク1c）

v193 §4-7 の行リスト（95 行）を本タスクで再取得し、process 系に絞った結果（`v200_1_out.txt` §T1-E）:

- **process 帯の実フィールド操作**: L412405（fun_0064b650 copy: `*(char*)(param_1+0x11c) = (char)param_2[0x47]`）、L417177/L417276/L417319/L417344/L417366（§2-2 の状態機械）、L430235（fun_0066a740: `param_1[0x47]=0`）。
- **その他バンドの混在（≠HighProcess）**: L97658（vtable デリファ経由・別クラス）、L110442/L326108/L341588/L346241（`DAT_00b333c4+0x11c` = **player オブジェクト**側の同名 dword、float 係数として乗算＝process とは別物）、L285538-L289345（0x57 帯 UI、§2-3）、L386064（fun_0061c9a0 = CombatController 帯）、L390528/L390599（fun_006231e0）、他セクション多数（Ni/Havok/UI の +0x11C）。
- `param_1[0x47]`（int* インデックス = byte +0x11C）で process 帯命中は上記 7 行のみ（L385471 fun_0061bb60 = CombatController ctor の別物込み）。

### §2-4 +0xE4..+0xF0 リスト族の元素型 — [確定: **装備アイテム（ARMO/WEAP/LIGH/AMMO）の4リスト**]

#### (1) vtable slot 対応（D5: High 0xA71814 / MidHigh 0xA72684 共有・358 slots）

| getter slot | impl | field | setter slot | setter impl |
|---|---|---|---|---|
| `+0xEC` (idx59) | `0x64B240` | **+0xE4** | `+0x104` (idx65) | **`0x658550`** |
| `+0xF0` (idx60) | `0x64B270` | **+0xE8** | `+0x108` (idx66) | `0x64AE90` |
| `+0xF4` (idx61) | `0x64B2A0` | **+0xEC** | `+0x10C` (idx67) | `0x64AED0` |
| `+0xF8` (idx62) | `0x64B2D0` | **+0xF0** | `+0x110` (idx68) | `0x64AF10` |

- 旧「+0xE4 の setter は未特定」は解消: **`FUN_00658550`（slot+0x104）が +0xE4 setter**（本文: 旧エントリ `0x484470`+`free` → `[esi+0xE4]=arg` → `[esi+0xF4]=[esi+0xF5]=0` → `arg[+8]`(form) の `byte [+0x90]` が 4 なら `[+0xF5]=1`、5 なら `[+0xF4]=1` — **form+0x90（ARMO/WEAP の手扱い系 byte）で process の2つの状態 byte を更新**）。
- 0x64AE90/AED0/AF10 は同型（`0x64AE50` は 3 引数 wrapper `call 0x69BE00` で **+0xE4 setter ではない** — 旧候補を撤回）。

#### (2) formType ↔ リスト対応（確定・getter/setter 両面一致）

`FUN_00489C30`（equip 系、decompiled L111195 `switch(*(param_2+4))` = formType 分岐）と `FUN_00489820`（L110893 同型 switch）の両方で一貫:

| formType | 名前（表 `0xB05E00` stride12 実測） | getter slot | **field** | setter slot |
|---|---|---|---|---|
| `0x14` (20) | **ARMO**（防具） | `+0xF8` | **+0xF0** | `+0x110` |
| `0x1A` (26) | **LIGH**（光源/松明） | `+0xF0` | **+0xE8** | `+0x108` |
| `0x21` (33) | **WEAP**（武器） | `+0xEC` | **+0xE4** | `+0x104` |
| `0x22` (34) | **AMMO**（矢） | `+0xF4` | **+0xEC** | `+0x10C` |

- `0x16` = **CLOT**（服）は **リスト化されない**（case 0x16 = L111238: slot+0x320 ガード + `FUN_004DCFF0`/`FUN_004DCE60` へ別経路委譲）。
- formType 表は 12B/レコード `{id, nameptr, 0}`（`0xB05E00 + ft*12 + 4` = nameptr。0x12=ACTI / 0x17=CONT / 0x18=DOOR / 0x23=NPC_ / 0x24=CREA / 0x31=REFR / 0x3E=CSTY まで実測一致）。
- **裁定: 4 リスト = 装備/携行アイテム4種への参照リスト**（装備・エフェクト・魔法のどれか → **「装備アイテム系」で確定**。エフェクト/魔法リストは +0xFC..+0x110 の別ファミリ（ctor で別途ゼロ化）に属し本4リストとは無関係 [推定]）。

#### (3) 要素/エントリ構造（タスク2a: deref パターン）

- **エントリ = 12B（`malloc(0xC)`、fun_00489c30 の L111206/111259/111293/111339 で全 case 共通）**、`FUN_00484420(elem, x)`（全文 `v200_6_out.txt` §G2）で構築: `H[+8]=elem`（フォーム）／`H[+0]=malloc(8)` の空 `{value=0, next=0}` ノード／`H[+4]=x`。
- **deref パターン（getter 呼び出し元）**:
  1. **null → 新規構築**: `H = getter(flag=1); if (H==0) { H = FUN_00484420(form, form); push; setter(H) } else { clear; push; H[+8] = form 更新 }`（F4b: L111204-111229）。
  2. **同一代入照合**: `H2 = getter(1); if (local_entry[+8] == H2[+8]) …`（F4: L110916-110919 — **エントリ +8 = フォームポインタ**として比較）。
  3. **flags 走査**: getter の flag≠0 経路 `0x484E80(H, flag)` = `H[+0]`（listヘッダ）から `{value@0, next@4}` 連鎖を歩き、各 value に `0x41DF10(value, flag)` → **`[value+0x0B]` = TESForm::flags(+0x8) の MSB byte**（flag=0 → bits 27\|28 = `0x18000000`、flag≠0 → bit28 = `0x10000000`）→ **元素 = TESForm 派生フォームで確定**。
- **挿入経路**: process 帯の `lea [reg+0xE4..F0]`+`call 0x446CB0` は **0 件**（F7）→ 挿入は **slot setter 経由のみ**（エントリ単位の差し替え）。`0x446CB0`（347 callers）はエントリ内 listヘッダへの push に使用（fun_00489c30 内 L111268/111282 等）。

#### (4) ctor の初期化（タスク2b: FUN_0064B400 帯 全読）

- **`FUN_0064B400` = MiddleHighProcess ctor（本体）**（`0x64B431: mov [esi], 0xA72684` = MidHigh vtable）。**HighProcess ctor = `FUN_00628EE0`**（`0x628F0D: call 0x64B400` → `0x628F14: mov [esi], 0xA71814` で High vtable へ上書き、+0x190 等を追加初期化）。**High ⊂ MidHigh の継承で共有 init**（GT §4.1 の slot 共有と整合）。
- ctor 内 `+0xE4..+0xF0` は **`ebx`(0) で 4 dword 一括ゼロ**（D6: `0x64B48C..0x64B49E`）＋隣接 `+0xFC/+0x100/+0x104/+0x108/+0x10C/+0x110` もゼロ（**計10 dword 連続**）＋ `+0xF4/+0xF5` byte ゼロ＋ `+0xF8 = -1.0f`（`0xA30634`）。**ctor への alloc/挿入は 0 件**（v193 の「alloc 直書き 0 件」を裏付け、**全フィールドは ctor で NULL 初期化のみ**）。
- ctor は別途 `push 8; call 0x401F00` → `{0,0}` を **`+0x174`** へ格納（D6 `0x64B4E9-0x64B506`）— +0x174 は本4リストと別系統の8Bヘッダ。
- **オブジェクトサイズ = `push 0x18C; call 0x401F00`**（ctor 呼び出し元 5 件中 4 件が 0x18C、`v200_2_out.txt` §C6）＝ **High/MidHigh process = 0x18C (492B)**。

#### (5) GetKnockedState script 実装（副産物・タスク1a の読手確定）

`opid 0x106B GetKnockedState impl 0x504950` → **`FUN_004F76B0`**（`v200_8/9_out.txt`）:
- `ref.vt+0x190`（IsActor ガード）→ `[ref+0x58]`（**process ポインタ**）→ `__RTDynamicCast(0xB05908/0xB09DFC = High/MidHigh TypeDesc,0x9832E6)` → **`process.vt+0x2E4` = `[+0x11C]` getter**（D5 と完全一致）→ `cmp eax,6; ja → 0.0`（**0..6 のみ有効**）。
- 戻り値 jump table（byte `0x4F7748` / dword `0x4F773C` 実測）: state→case = `{0:0, 1:2, 2:2, 3:1, 4:1, 5:0, 6:0}`、case0=`fldz`(0.0) / case1=`fld1`(1.0) / case2=skip(0.0)。
- **→ GetKnockedState は state ∈ {3, 4} のとき 1.0、他（0,1,2,5,6）は 0.0**（bool 表現）。トレース文字列 `"Get Knocked State >> %0.2f"`（0xA49218）。

## §3 結論（3行）

1. **KnockedState 課題は確定**: `[process+0x11C]` byte 書手は **14 箇所**（v177「1件のみ」を訂正）で、`FUN_006545E0`（vt+0x2F4）の switch が {0,1,2,3,4,5,6} の遷移を全て生成 — 0=None / 1=KnockedDown(倒れ中) / 2=KnockBack(impulse直後、`FUN_00654420` vt+0x2F0) / 3=GettingUpPrep(GetUp アニム探索、失敗時 `"No 'GetUp' animation found for actor '%s'."`) / 4=KnockDown開始(animなし ragdoll) / 5=GettingUp(アニメ再生) / 6=Recovered(復帰処理)。event **mask 0x40 = OnKnockout**（impl `0x5042D0` の `push 0x40` で確定）。script `GetKnockedState`（`0x4F76B0`）は state∈{3,4} のとき 1.0 を返す bool 表現。+0x11C は **セーブ保存**（`0x65698E` の stack 復元、v177「非保存」を訂正）。
2. **L286800 の `0x80000000` は KnockedState と無関係**: 所属は `FUN_0057F7A0` = **UI/メニューの DirectInput キー入力処理**（DIK 状態配列 `[obj+0x18F4]`、`FUN_004032D0`=キー押下テスト、`0x80000001..4` = Left/Right/Up/Down 押下マーク、`0x80000000` = VK_BACK、`0x80000008` = VK_RETURN、+0x118 = Shift/Ctrl/Alt フラグ）。process とは**同一オフセットの偶然**。
3. **+0xE4..F0 リスト族の元素型は確定**: **装備/携行アイテム4種（`0x14`=ARMO / `0x1A`=LIGH / `0x21`=WEAP / `0x22`=AMMO）への TESForm 参照リスト**（formType 表 `0xB05E00` stride12 で実測、getter/setter 両面の slot 対応が一致）。要素は 12B エントリ `{list_head(8B {value,next}連鎖), x, form*}`、flags bit27/28（`[form+0x0B]`）で有効性フィルタ。挿入は equip 系 `FUN_00489C30` → slot setter 経由のみ（process 帯 lea+push = 0 件）。ctor（`FUN_0064B400` = MidHigh ctor、High ctor `FUN_00628EE0` が継承）は 10 dword 連続ゼロのみ＝alloc/挿入 0 件。オブジェクトサイズ **0x18C**。

## §4 残留不明点

1. **値 1 の唯一の生成元** L417366（`((old==4)*2)+1`）の asm が `0x65471F: mov [edi+0x11c], bl`（bl=4 ロード直後）であることは確認したが、`(old==4)*2+1` の**算術そのもの**（setcc/lea 命令列）は disasm ウィンドウのオフセットずれで未取得（値 1 は setter `0x629440` 経由でも生成し得る）。
2. `FUN_005E0DC0` = `mov eax,[ecx+0xB0]; ret` の **[+0xB0] フィールドの正式名**（`==6` が割り込み抑止ガードとして頻出。KnockedState とは独立の値 0..6 系 [推定]）。
3. UI キー入力クラス（0x57 帯、`FUN_0057F7A0`/`FUN_0057DC30`/`FUN_0057DC60` の owner、caller 0x5834E3 等）の**正式クラス名**。
4. **`form+0x90` byte が 4/5 の意味**（`FUN_00658550` が `[+0xF4]`/`[+0xF5]` を設定。ARMO/WEAP の手扱い/スロット系 byte [推定]）と、process `+0xF4/+0xF5` の語義。
5. `+0xFC..+0x110` の6ポインタ（ctor で別途ゼロ化）が**エフェクト/魔法系の第2ファミリ**であることの直接証拠（本タスクは +0xE4..F0 の4件のみに確定を限定）。
6. `FUN_00479780` の `param_1+0xe4` 直接代入（L97237/97347、`FUN_00479450` 経由）が process+0xE4 と**同一フィールドか**（param_1 の class同定が未完了。0x47 帯の他クラス同名オフセットの可能性 [有力]）。
7. GetKnockedState の bool 表現 {3,4}→1.0 が **GECK ドキュメントの int 0..6 戻り値とどう対応するか**（本 EXE の script impl は double/bool で返す — VM 側の型変換経路は未深掘）。

## §5 自己検証

- **`probe_harness.py selftest` 4/4 PASS**（着手時・完成直前の 2 回: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（独立一致 12 件）:
  1. **slot 対応の両面一致**: getter `+0xEC..+0xF8` → field `+0xE4..+0xF0` と setter `+0x104..+0x110` が、formType switch（fun_00489c30 / fun_00489820）の4 case と**1:1 で完全一致**（ARMO→+0xF0 / LIGH→+0xE8 / WEAP→+0xE4 / AMMO→+0xEC）。
  2. **formType 表の独立一致**: stride12 実測 `0x22=AMMO / 0x21=WEAP / 0x1A=LIGH / 0x31=REFR` が既存実測（v161 L148）と一致、`0x14=ARMO / 0x16=CLOT` が s90 L72 と一致（stride4/8 での初回抽出失敗 → stride12 で正、レコード `{id,nameptr,0}`。
  3. **GetKnockedState 経路**: impl `0x504950` → `0x4F76B0` → RTTI cast（High/MidHigh typedesc）→ `vt+0x2E4` → `0x64B080` = `[+0x11C]` getter（D5 の slot 行と一致）→ `cmp eax,6; ja 0`。
  4. **event mask 0x40 = OnKnockout**: opid 0x000C impl `0x5042D0` の `push 0x40; call 0x4FB5F0` と、state 1/2 の再送出 `FUN_004FBF90(0, actor+0x44, 0x40)` が一致。
  5. **byte 書手 14 箇所**: raw disp32 スキャン（C6/C7/88/89）と decompiled `param_1[0x47]` 行の7行が相互に整合（v177「1件」の訂正根拠）。
  6. **セーブ保存の訂正**: `0x65698E: mov al,[esp+0x6c]; mov [esi+0x11c],al` が FUN_006564C0（load 側）内にあることを disasm で直接確認。
  7. **オブジェクトサイズ 0x18C**: ctor 呼び出し元 5 件中 4 件の `push 0x18C`（残1件0x683366 は直後の別オブジェクト `push 0x2EC` と混在しないことを文脈 disasm で確認）。
  8. **ctor 初期化**: D6 asm の `0x64B48C..0x64B49E` 4 dword ゼロ＋ `0x64B52D` の `[+0x11c]=bl` が v177 §4.2 の「FUN_0064B400 一括初期化」と一致。
  9. **エントリ 12B**: fun_00489c30 の全 case（0x14/0x1A/0x21/0x22）が `malloc(0xC)` + `FUN_00484420` で一致（0x16 CLOT のみ別経路）。
  10. **`FUN_00484420` 構造**: asm（`[esi+8]=arg / node=malloc(8){0,0} / [esi]=node / [esi+4]=arg3`）と decompiled L106045-106056 が一致。
  11. **L286800 の別クラス裁定**: `FUN_0057F7A0` の DIK コード（0x2A/0x36/0x38/0xB8/0x1D/0x9D）と `FUN_004032D0` の `[+0x18F4]` 状態配列が、process 帯（0x60-0x6C）と無関係な 0x57 帯にあることを enclosing で確認。
  12. **+0xE4..F0 alloc 直書き 0 件**: ctor D6 全読（150 行 disasm）で +0xE4..F0 への `call 0x401F00` が存在せず、`push 8` の結果が +0x174 へ向くことのみを確認（v193 §2-177e と再現一致）。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・既存 tmp/re・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**。ESM 読取 0 回。中間物は `%TEMP%\opencode/v200_1..9_*`（9 スクリプト＋9 出力）。

## §6 Android 移植への示唆（実装しない）

- **ノックダウン状態機械**: 7 値 FSM を移植版のアクター状態として再現する場合、要点は (a) impulse 受信で 2 → 1（倒れ）→ 3（GetUp 探索）→ 5（アニメ再生）→ 0/6（復帰）という**直列のライフサイクル**と、anim なし ragdoll 経路（4→3）の分岐、(b) 途中割り込みは「相手死亡（vt+0x198）」「既に state6」「GetKnockedState==6」の3ガードで遮断される設計、(c) OnKnockout イベント（mask 0x40）は state 进入時と state1/2 の更新時に**再送出**されること。GetUp アニム欠落時はエラーログ＋vt+0x2F8（後始末）へフォールバックするため、アニメ資産が無い環境でも**状態がスタックしない**。値の bool 化（{3,4}→true）はスクリプト公開面のため再現する価値があるが、内部は 0..6 の整数で保持すること。
- **装備4リスト**: Android 側では `enum class EquipList { Armor, Light, Weapon, Ammo }` を `array<unique_ptr<FormList>,4>` として保持し、formType で振り分ける既存の equip ルートにそのまま合流させられる（CLOT はリスト化しない例外も再現）。各エントリは「代表フォーム＋変更ノード連鎖」の12B相当で、flags bit27/28 フィルタは「フォーム有効ビット」のゲートとして移植する。**シリアライズは 4 フォームの formID のみ**（ノード連鎖は runtime再構築）で、ビルド後オブジェクトは ctor と同様 NULL 初期化のみにする（部分初期化の事故を防ぐ）。
- **キー入力の sentinel 値**: `0x80000000 | VK` 形式の保留マーカーは移植版でも「押下中特殊キー」のスタックとして使えるが、+0x11C というオフセット依存の暗黙共有は避け、専用の `pendingKey` フィールドへ分離すること（本 EXE でも process と UI の同名オフセットは無関係＝この教训の実例）。
