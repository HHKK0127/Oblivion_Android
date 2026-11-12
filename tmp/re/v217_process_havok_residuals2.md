# v217_process_havok_residuals2.md — process/Havok 残点第2束（V-204-1..5 ＋ v205 残点5件）

- 日付: 2026-11-01 / 作成: opencode サブエージェント（TODO serial 208 / V208・中）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v204_process_residuals2.md` §8（V-204-1..5）＋ `tmp/re/v205_havok_ini_naming.md` §4（残点5件）
- 手法: capstone 5.0.7（`v172_common.py` / `probe_harness.py Ctx`）＋ `docs/Oblivion_decompiled.c`（部分読取）＋ `cache/{named_funcs,vtable_index,call_targets,opcode_table}`（参照のみ）
- 中間物: `%TEMP%\opencode\v217_util.py` および `v217_1..20_*.py` / `v217_*_out.txt`
- selftest: 着手時 **4/4 PASS**（`probe_harness.py selftest`）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v217_*` のみ。ESM 読取 0 回
- **重要・VA 換算の罠（本タスクで踏んだ轍）**: `v172_common.imm32_refs()` の第2リージョン `(0xA28000,0xB32C00)` は .rdata+.data を 1 連続チャンクとして走査するため、**.data 内の参照 VA が実値より 0x600 小さい**（.rdata raw 端 VA と .data raw 始 VA のギャップ）。セクション個別変換の `imm32_refs3()` を使うこと（v205 probe V/W と同種のバグ。本タスクで item3 表を誤検出して是正）

---

## §1 item1（V-204-1）process+0xE4 の二重性 — **解明: 「二重性」は2種のオブジェクトに分かれており、型衝突は不成立**

### 証拠A: +0xE4 は「WEAP 装備エントリ」スロット（0x18C クラス = MidHigh/High process）

1. **getter vtable slot+0xEC = `FUN_0064B240`**: flag==0 → `return [this+0xE4]`、flag≠0 → `FUN_00484E80([this+0xE4],0)`（**`[entry+0]` の {value,next} 連鎖を歩くフィルタ**）＝+0xE4 が 12B エントリであることをゲッタ側が要求（`v217_2_out.txt` §A）。
2. **setter vtable slot+0x104 = `FUN_00658550`**（High `0xA71918` / MidHigh `0xA72788` 両 vtable に実測配置、`v217_2_out.txt` §B）: 旧値 `[this+0xE4]` を `FUN_00484470`（list 連鎖 free＋head free）＋`free` で破棄 → `[this+0xE4]=arg` → `arg[+8]` を **フォーム**として読み `byte [form+0x90]`（weapon type）で `[+0xF4]`(Bow)/`[+0xF5]`(Staff) を設定。`ret 8`（第2引数 = hand 系）。
3. **equip 本体 `FUN_00489C30` case 0x21**（L111290-111334, `v217_2c_out.txt` §A）: `entry = vt+0xEC(1)` → 無ければ `malloc(0xC)` + `FUN_00484420(form,form)`（＝`{listhead@0, x@4, form@8}`）→ `vt+0x104(entry, uVar9)`。既存エントリ枝は `entry[+8]=form` 更新のみ。

### 証拠B: +0xE4 は「WEAP 3D アドオン記録」の第3スロット（0x154 クラス）

1. **アドオン記録の型**（FUN_00479780 家族 decompiled、`v217_1b/2b_out.txt`）:
   - WEAP: `FUN_00478780(&this[+0xDC],1,0)` → `[+0xDC]=form, [+0xE0]=form+0x30(TESModel), [+0xE4]=FUN_00479450(...)`（L97223-97238、asm `0x479858 mov [ebx+0xE4],eax`）
   - ARMO: record@+0x11C → addon = **+0x124**（L97576）
   - AMMO: record@+0x10C → addon 候補 +0x114（L97420-97432）
   - LIGH: record@+0x12C（+0x134 addon）、グローバル `0xB33C80+n*16` 群、`this+0x4C+n*0x10`（16 個、`FUN_004796F0` が一括破棄）
   ⇒ **記録型 = `{form@+0, TESModel@+4, NiAVObject*@+8}`（12B 実体・16B スロット）**。
2. **`FUN_00478780` は第3スロットを NiAVObject として扱う**: `[field+8]` を `g_[0xB42F54]` マネージャ経由（`FUN_007B4280(0)`→`FUN_007C5E70`＝仮想 +8 呼出し）でシーンから除去 → `[field+8]+0x1C`（parent）経由のデタッチ＋`InterlockedDecrement` → `FUN_00481350`（refcount 系）→ `field[1]`(TESModel) の仮想 +0x14 呼出し＋`FUN_004384E0`（解放）→ `field[2]=0`（`v217_1b_out.txt` §B、`v217_2b_out.txt` §D）。
3. **`FUN_00479450` の戻り値は Ni アドオンノード**: skeleton bone へ `(**(vt+0x84))(node,1)` アタッチ、失敗時ログ **`"Tyring to add skinned object '%s' as an add on to skeleton."`**（L97117）。

### 証拠C: 2 種は**別オブジェクト**（ここで v204 の前提が訂正される）

| | active process（0x18C） | 補助 process（0x154） |
|---|---|---|
| 格納先 | **`[REFR+0x58]`**（Actor ctor `FUN_005E15E0@0x5E16C0` が 0x90 オブジェクトで初期化、`FUN_005F1E68`/`FUN_00607672` が `malloc(0x18C)+FUN_0064B400` で差替え） | **`[Character+0x104]` / `[Player+0x5C8]`** |
| ctor | `FUN_0064B400`（MidHigh）/ `FUN_00628EE0`（High） | **`FUN_00478730`**（`push 0x154; memset; [+0x150]=owner;` アドオン記録 16 個領域） |
| 破棄 | （process dtor 経路） | **`FUN_0047AB80` = `jmp FUN_004796F0`**（16 記録一括破棄）＋`free` |
| ゲッタ | — | `FUN_006600D0(flag)` = `flag ? [this+0x5C8] : [this+0x104]`（実測 `0x6600D0`: `cmp [esp+4],0; je; mov eax,[ecx+0x5c8]; ... mov eax,[ecx+0x104]`） |
| +0xE4 の意味 | **WEAP 装備エントリ**（setter の this 検証: `[g_thePlayer+0x58]==this`） | **WEAP 3D アドオン記録第3スロット**（destroy helper の this 検証: `this==FUN_006600D0(0)`） |

- 装備コード（equip case 0x21）は `param_4[0x16]` = **`[REFR+0x58]`** の vtable を介して setter を呼ぶ（＝active process にエントリ）。
- 3D アタッチ `FUN_004E1900`（this = REFR、**g_thePlayer 限定**）→ `FUN_00479780(**FUN_006600D0(...)**, form)`（asm `0x4E1995` 帯: `call 0x6600d0; mov ecx,edi; call 0x479780`）＝**補助 process** にアドオン。
- サイズが異なる（0x18C vs 0x154）2 オブジェクトなので **+0xE4 は実运行で型衝突しない**。

### 裁定

> **V-204-1 解消（前提訂正つき）**: `process+0xE4` には (1) **active(Mid/High)process の WEAP 装備 12B エントリ**（setter `FUN_00658550`）と (2) **補助 process（`[Character+0x104]`/`[Player+0x5C8]`、0x154）の WEAP 3D アドオン記録第3スロット＝NiAVObject**（`FUN_00479780`/`FUN_00478780`）が**別オブジェクトとして**入る。v204 の「同一オフセット・同一クラス」のうち**「同一クラス」は誤り**（同一オフセットのみ）。setter は `this==[g_thePlayer+0x58]`、destroy helper は `this==FUN_006600D0(0)` と各自が対象インスタンスを明示的に検証している。

**判定: 解明（v204 前提を訂正）**（証拠 `v217_1_out.txt` / `v217_2_out.txt` §A-B / `v217_2b_out.txt` / `v217_2c_out.txt` §A / `v217_5..9_out.txt`）。

---

## §2 item2（V-204-2）Character vt+0x198 / vt+0x1A0 述語の正式名

### vt+0x198 = `0x5E33B0` — **解明: `IsDead`（opcode 表名 `GetDead`, opid `0x102E`）の二態述語**

- 本文（`v217_10_out.txt` §C4）: `eax=[this+0xB0]`(LifeState)、flag=`[esp+4]`
  - **flag≠0**: `LifeState ∈ {2(Dead), 1(Dying)} → 1`
  - **flag==0**: `LifeState ∈ {2, 1, 6(Normal)} → 1`
- **スクリプト接続の直接証拠**: opid `0x102E GetDead` の cond `FUN_004F4890`（`v217_13_out.txt` §B, `v217_14_out.txt` §A）: IsActor ガード（vt+0x190）→ `mov edx,[esi]; mov edx,[edx+0x198]; push **1**; call edx` → 真なら 1.0（トレース文字列 0xA488A8）。
  ⇒ **GECK の `IsDead` 条件は flag=1 モード（{Dead,Dying}）で評価される**。flag=0 モード（{Dead,Dying,Normal}）は FSM ガード等の内部利用（v204 §2 の knockdown クリア条件）。
- 近接関数 `0x5E33E0` = `return [this+0xB0]==1`（**IsDying 相当**、vt スロット未特定）。

### vt+0x1A0 = `0x5E17E0` — **構造解明（名称は[残]）: `this->vt+0x284(0x30) != 0` の内部述語**

- 本文（`v217_10_out.txt` §C5）: `mov eax,[ecx]; mov edx,[eax+0x284]; push 0x30; call edx; test eax,eax; setne al`。
- Actor vt+0x284 = `0x5F1970`（`v217_14_out.txt` §A2）: `[this+0x58]`(process) ガードのうえ引数（9 / 0x30 / …）で分岐する**汎用「状態 id 取得」風の関数**（0x30 帯では vt+0x288(0x28) 呼出し＋浮動小数点比較）。
- 呼出し側は内部述語として広範に使用（`call [vtable+0x1A0]; test al,al` 形式で 30 箇所超、`v217_12_out.txt` §B）。**opcode_table 533 行の cond/impl のいずれも +0x1A0 を参照しない**（= スクリプト未公開）。
- v204 の「vt+0x284(0x30) 委譲」を確認。**正式名は未取得 [残]**（vt+0x284 の id 0x30 の意味づけが次段＝V-217-2）。

**判定: vt+0x198 = 解明（`IsDead`/GetDead 0x102E）／vt+0x1A0 = 部分解明（構造のみ・名称[残]）**（証拠 `v217_10_out.txt` §C / `v217_12_out.txt` §B / `v217_13_out.txt` §B / `v217_14_out.txt` §A-A2）。

---

## §3 item3（V-204-3）weapon type 4=Staff の独立証拠 — **解明: 型名ポインタテーブル `0xB086D0` を発見**

### 証拠: 型名 6 本の**連続ポインタテーブル**（.data `0xB086D0`、`imm32_refs3` 実測）

| idx | 文字列アドレス | 名前 |
|---|---|---|
| 0 | `0xA4522C` | `BladeOneHand` |
| 1 | `0xA4521C` | `BladeTwoHand` |
| 2 | `0xA4520C` | `BluntOneHand` |
| 3 | `0xA451FC` | `BluntTwoHand` |
| **4** | **`0xA348F8`** | **`Staff`** |
| 5 | `0xA451F8` | `Bow` |

- テーブル直前（0xB086C0..0xB086CC）には `{2,3,4,5}` の整数並びが隣接（2H 系 id リストと推定、`v217_13_out.txt` §A dump）。
- 追加: 部分表 `{Staff@0xB102D8, Bow@0xB102DC}` も同一 .data 帯に存在（"cannot be poisoned" 系と関連する集合と推定）。
- `"Staff"`(0xA348F8) の .text 参照は 0x9DA3A1 / 0x9F7F20（検証/ログ系）。`"Staffs cannot be poisoned."`(0xA5F5E0)、`sStaff`(0xA63DFD)、`WPNStaff`/`WPNBlockStaff`/`ITMWeaponStaff*` 等の周辺文字列も整合。

### 裁定

> **weapon type enum は `0=BladeOneHand, 1=BladeTwoHand, 2=BluntOneHand, 3=BluntTwoHand, 4=Staff, 5=Bow` で確定**（v204 の「逆順プーリングからの推定」に**独立証拠**を追加）。v204 §4 の `type==4 → [+0xF5]=1`＝「Staff 装備中フラグ」との二重照合も成立。

**判定: 解明**（証拠 `v217_11_out.txt` §A / `v217_13_out.txt` §A）。

---

## §4 item4（V-204-4）[player+0x104] / [player+0x5C8] の書手とレイアウト境界 — **解明**

### 書手カタログ（実測）

| 格納先 | 書手 | 経路 |
|---|---|---|
| `[Character+0x104]` 初期化 0 | **`FUN_0060E4F0`（Character ctor）** `param_1[0x41]=0`（asm `0x60E523`）／`FUN_0060E540`（ctor 別形、`0x60E56C`） | Character ctor は vtable `0xA6FC9C` を書く（`v217_9_out.txt` §A） |
| `[Character+0x104]` 代入 | **`FUN_00611B00` = Character/Player vtable slot+0x16C setter**: 旧値を `FUN_0047AB80`（=`FUN_004796F0` 16 記録破棄）＋`free` → `[this+0x104]=arg`（`v217_8_out.txt` §A） | 0x154 オブジェクトは `FUN_00521A40`/`FUN_00523170`（`malloc(0x154)`+`FUN_00478730`、`v217_7/8/9`）で生成され setter へ |
| `[Player+0x5C8]` 生成 | **`FUN_00667BE0` = PlayerCharacter vtable slot+0x14C**: `malloc(0x154)`+`FUN_00478730(obj, ebx, player)` → `mov [ebp+0x5C8], eax`（`0x667E2E`）、続けて `[+0x5CC]=malloc(0xDC)+FUN_00473EB0(...)`（`v217_5_out.txt` §A / `v217_8_out.txt` §D） | Player 固有 |
| `[Player+0x5C8]` クリア 0 | PlayerCharacter ctor `FUN_0066A740`（vptr `0xA73A0C`@`0x66A776`）帯 `0x66AB7E`、終了処理系 `0x65F9B4`/`0x66B3F1`（いずれも destroy=`FUN_0047AB80`+free のセット） | `v217_4/5/8_out.txt` |
| 参照 | **`FUN_006600D0`**（getter）: `flag ? [this+0x5C8] : [this+0x104]`（asm 実測、`v217_6_out.txt` §A）。`FUN_00660110` も `[+0x5D0]`（第3の補助オブジェクト、`FUN_00660110` = `flag ? [+0x5D0] : FUN_00612220()`）を返す | — |

### レイアウト境界の裁定

> - **`+0x104` は `Character` クラスのフィールド**（Character ctor が初期化、Character/Player 両 vtable の slot+0x16C が setter）。**TESObjectREFR ではなく Player 専用でもない**（NPC Character にも存在）。REFR ctor `FUN_004D9A70`（vptr `0xA46C44`@`0x4D9AA6`）は +0x104 を触らない。
> - **`+0x5C8`（および +0x5CC/+0x5D0）は `PlayerCharacter` 専用フィールド**（Player ctor `FUN_0066A740` で 0 初期化、Player vtable slot+0x14C が生成）。
> - 両者が指すのは **0x154 バイトの補助 process オブジェクト**（ctor `FUN_00478730`: memset 0x154 → `[+0x150]=owner ref`、+0x4C にアドオン記録 16 個）。これは **`[REFR+0x58]` の active process（0x18C、ctor `FUN_0064B400`/`FUN_00628EE0`）とは別クラス・別インスタンス**（§1 item1 と表裏）。
> - v204 の通称「1st/3rd person process」は、+0x104＝全 Character 共通の補助 process、+0x5C8＝Player 追加分、という**階層で置き換えるのが正確**（"1st/3rd" の厳密な割当は未証明 [残]＝V-217-7）。

**判定: 解明（境界＝Character/+0x104、Player/+0x5C8、実体＝0x154 補助 process）**（証拠 `v217_3_out.txt` / `v217_4_out.txt` §A / `v217_5_out.txt` §A / `v217_6_out.txt` §A / `v217_7_out.txt` / `v217_8_out.txt` / `v217_9_out.txt`）。

---

## §5 item5（V-204-5）LifeState（REFR/Actor+0xB0）の書手と ctor 既定値 — **解明（値3 書手のみ[残]）**

### setter 本体: **`FUN_005E6680` = `SetLifeState(actor, value)`**

- 本文（`v217_9_out.txt` §C）: `if ([this+0xB0] != arg) → this->vt+0x40(0x40)`（イベント送出、arg=0x40）→ `[this+0xB0] = arg`、`ret 4`。
- **直接 caller 21 件の即値実測**（`v217_10_out.txt` §A）:

| 値 | caller 例 | 意味 |
|---|---|---|
| **0** | `0x45C224`（HP 回復系）/ `0x5EB5D2`・`0x5EBCB0`・`0x5F1E39`（`[+0xB0]==6` のとき 0 へ）/ `0x5F60AE`・`0x5F6188`・`0x5FA6D0`・`0x6023F0`・`0x603EF7` | Alive 復帰 |
| **1** | `0x605BDD`（vt+0x278 偽）、`0x600B61` | Dying |
| **2** | `0x4DFA61`（ダメージ処理）、`0x5EA042`（process vt+0x370 呼出し直後）、`0x5FA7B3`、`0x6753C7`（vt+0x284(8)<=0） | Dead |
| **4** | `0x6A3EE9`（浮動小数点比較後） | Reanimate |
| **5** | `0x5E8E6C`（`FUN_005E8DE0`） | Restrained |
| **6** | `0x600751`（`FUN_006005F0`、家具/アニメ処理後） | Normal |

- **値 3（Unconscious）の直接書手は検出されず** [残]（＝V-217-1。GetUnconscious 側の cond は存在、エンジンからの即値 caller は本 EXE に不在）。

### ctor 既定値と保存

- **既定値 = 0（Alive）**: **Actor ctor `FUN_005E15E0`@`0x5E16D4` `mov [esi+0xB0], ebx(0)`**（vptr `0xA6E074` を書く Actor ctor 本体、`v217_9_out.txt` §A）。Character ctor `FUN_0060E4F0` → `FUN_005E15E0` 連鎖。
- **セーブ復元**: `FUN_006022F0`（Actor ロード系）@`0x602568-0x60257F`: ロード byte を `movzx` で dword 化し `[esi+0xB0]` へ（旧値∈{1,2,6} かつ変化あり → `call 0x5F87F0` 死亡系ハンドラ）。**LifeState は保存対象**。
- getter 族（既存確定）: `FUN_005E0DC0`=`mov eax,[ecx+0xB0]; ret`、Actor vt+0x198=`IsDead`（§2）。

**判定: 解明（値3 書手のみ[残]）**（証拠 `v217_4_out.txt` §D / `v217_9_out.txt` §A-C / `v217_10_out.txt` §A）。

---

## §6 item6（v205 残1）[0xB3BCF0] のエンジン内部呼称と値の意味 — **解明（算術・意味）: 「実時間/ゲーム時間 併走クロック」。呼称は露出なし [残]**

### 増分経路の算術（本タスクで全取得）

- **増分パス1**（`FUN_0065F770@0x65F78B-0x65F7BE` / `FUN_0066F420@0x66F879-0x66F8AC`・`0x66F98E-0x66F9BA`、`v217_15_out.txt` §A-A3）:
  ```
  T   = FUN_004029D0(g_0xB332E0)          ; = [[0xB332E0+0x14]+0x24]（float）
  d   = 3600.0(double 0xA2F938) / T       ; fdivr
  [0xB3BCF0] = FUN_00673B00() + d         ; getter+add+setter
  ```
- **増分パス2**（`FUN_00678510@0x678534-0x678556`、`v217_14_out.txt` §B3）:
  ```
  [0xB3BCF4] = 0x6E                       ; 隣接カウンタ
  [0xB3BCF0] = [0xB3BCF0] + [0xB33E9C]    ; 加算して setter
  ```
- **[0xB33E9C] の正体**: `FUN_00405090`（ゲーム時刻 API、`v217_18_out.txt` §D）: `if (arg0) return FUN_00578F60() ? 0 : [0xB33E9C];` `else if (arg1) return FUN_00402BD0(g_0xB332E0) × 60.0 × 60.0`（0xA2FCC8=60.0 double）＝ **ゲーム時刻（時間単位）×3600 = 経過ゲーム秒**。⇒ **[0xB33E9C] = 経過ゲーム秒のグローバル**。
- getter `FUN_00673B00` の 9 caller はすべて slot218（Actor 時間差分 vfunc）への `fstp [esp]` 引数供給（v195 §4.2 確定）。setter は NaN/Inf/100000 ガード（v205 確定）＋**セーブ対象**（`push 4; push 0xB3BCF0; call FUN_004534D0` = save-stream、`0x67711F-0x677133` 帯）。

### 裁定

> **[0xB3BCF0] = 「実時間ベース・ゲーム時間ベースの2 経路で加算される経過秒クロック」**（Actor slot218 への毎フレーム時間入力）。パス1は `3600.0 / [[0xB332E0+0x14]+0x24]`（尺度の逆数）、パス2は `[0xB33E9C]`（経過ゲーム秒 delta）。**エンジン内部呼称（識別子）は本 EXE に露出しない [残]** — INI 名なし（v205 確定）に加え文字列/ログ参照も皆無。当面の仮称は **"fGameTimeClock"**。厳密な単位解釈（パス1 の 3600/T のフレーム当たり意味）は V-217-3 に残置。

**判定: 解明（算術・系統）／呼称と単位の細部は [残]**（証拠 `v217_14_out.txt` §B / `v217_15_out.txt` §A / `v217_16_out.txt` §C / `v217_18_out.txt` §D）。

---

## §7 item7（v205 残2）増分パス2（3600/`[[0xB332E0+0x14]+0x24]`）の物理的意味と 0xB332E0 のクラス同定 — **解明（機能）**

### `0xB332E0` = **ゲームタイム・クロック singleton**（.data bss・埋め込みオブジェクト、`mov ecx,0xb332e0` で this として渡す）

| メソッド | 本文要点 | 意味 |
|---|---|---|
| **`FUN_004029D0`** | `mov eax,[ecx+0x14]; fld [eax+0x24]; ret`（3 命令） | **`T = [[this+0x14]+0x24]`**（task の式そのもの）＝尺度 float |
| **`FUN_004029E0(x)`** | `new = T * x / 3600.0(double 0xA2F938) + [[this+0xC]+0x24]` → clamp `[0xA2F930]=24.0` → `[this+8]/[+4]/[+0]` の各 `+0x24` へ伝播 | **時計を進める**: x 実秒 × 尺度 / 3600 = 現在時刻（時間）への加算、**24h wrap** |
| **`FUN_00402BD0`** | `eax=[this+0xC]; return [eax+0x24]`（無ければ `[0xA2F918]=12.0`） | **現在ゲーム時刻（時間、既定 12:00）** |
| `FUN_00402C40`/`0x402B40`/`0x402BA0`/`0x402B80` | `FUN_00407AE0` 帯の日付 sprintf（変換テーブル 0xA30A6C..）で使用 | 日時ヘルパ |

- 構造: `+0x0C` と `+0x14` は **各々 `+0x24` float を持つサブオブジェクト**（NiTimeController 相当レイアウト: `+0x24` = m_fElapsed 位相）。+0/+4/+8 にも同型スロット（FUN_004029E0 が4本まとめて伝播）。
- 周辺利用で単位を確定: `0x45A46C: FUN_00402BD0(); fdiv 24.0(double 0xA2F920)`（**時間→日数**）、`0x4052B6: FUN_00402BD0(); ×60.0×60.0`（**時間→秒**）、clamp=24.0（**1 日**）。

### 3600 の物理的意味

> **3600.0（double `0xA2F938`）は「時間単位(時) ↔ 秒」の換算係数**。`FUN_004029E0` では `秒 × 尺度 / 3600 → 時間(時)`、増分パス1では `3600 / 尺度` を [0xB3BCF0] へ加算する。尺度 `T=[[0xB332E0+0x14]+0x24]` は `FUN_004029E0` の代数（`hours_delta = T·x/3600`）から **ゲーム秒/実秒（fTimeScale 相当）** か **フレーム長系** のいずれか（V-217-3 に単位の最終解釈を残置）。隣接の 24.0/60.0/60.0 定数群と整合し、**0xB332E0 は「日時＋時間尺度を保持するゲーム時計 singleton」で確定**（RTTI 名なし・クラス正式名は [残]）。

**判定: 解明（機能・算術）／クラス正式名は [残]**（証拠 `v217_14_out.txt` §C / `v217_15_out.txt` §B / `v217_16_out.txt` §A-F / `v217_18_out.txt` §D）。

---

## §8 item8（v205 残3）hk3AxisSweep vtable `0xA9A710` 18 スロットの仮想順名前 — **部分解明**

### 18 スロット実測（`v217_17_out.txt` §A-A2）

| slot | impl | 名称（本タスク裁定） | 根拠 |
|---|---|---|---|
| 0 | `0x8E6460` | **dtor**（scalar、15 命令） | 短関数・先頭スロット |
| 1 | `0x8E3960` | 未名称（'3AxisSweep'/'Nodes' 文字列参照、大関数）＝**ノード/登録操作系** [推定] | 文字列 |
| 2-8 | `0x8E1660`/`0x8E5130`/`0x8E19C0`/`0x8E5630`/`0x8E4590`/`0x8E61E0`/`0x8E1D90` | **未名称**（addEntity/removeEntity/updateEntity/castAabb/castSphere/collide/overflow 系と推定）[残=V-217-5] | — |
| 9 | `0x8E1E00` | 小関数（7 命令、ゲッタ系）[残] | — |
| 10 | `0x8E1E20` | **querySingleAabb 系**（`'LtquerySingleAabb'` 文字列＋slot16 を呼ぶ）[確度高] | v205 §2-1 |
| 11-13 | `0x8E14E0`/`0x8E3A90`/`0x8E1160` | **未名称** [残] | — |
| **14** | `0x8E24A0` | **castRay**（v195/v205 確定、発射端 `call [edx+0x38]` と一致） | 既存 |
| 15 | `0x8E1040` | **getAabbCount**（`mov eax,[ecx+0x44]; ret`＝ハッシュテーブル要素数、5 命令）[確度高] | v195 §3.3 の `[edi+0x44]` count と整合 |
| 16 | `0x8E5950` | querySingleAabb **実装本体**（slot10 の下位）[確度中] | v205 |
| 17 | `0x8E2F20` | **クラス名/トレース**（`'3AxisSweep'` を push、23 収束ループの ctor 共通部と同一）[確度中] | v205 §2-1 |

- v205 の「'Lthk' 接頭辞」は slot1/17 帯のプロファイル文字列断片（`Lthk3AxisSweep`/`LtquerySingleAabb`）で、 Havok の**プロファイラ埋め込み名（"Lt"=linked trace 系）**と解するのが最有力（正式証明は [残]）。

**判定: 部分解明（14/15/10/16/17/0 は根拠付き命名、2-9/11-13 は[残]）**（証拠 `v217_17_out.txt` §A）。

---

## §9 item9（v205 残4）vtable `0xA96B78` の b13 142 ブロック表への再分類要否 — **解明: 再分類不要（#73 の派生 view として注記追加が適切）**

### 実測（`v217_17_out.txt` §B-B2）

- バイト列: **連続コードポインタは `0xA96B58` から 10 スロット**（`0x899300, 0x68F970, 0x8992F0, 0x899380, 0x899390, 0x8993E0, 0x927A70, 0x8993D0, 0x8993A0, 0x68F970`）→ `0xA96B80` 以降は ASCII 文字列（`StCallbacks`/`StBroadphase`/`Island`/`LtAddEntity`/`StIsland`/`Broadphase`/`LtRemoveM…`）。
- **`0xA96B78` = 上記ランの slot8 起点**（末尾 2 スロット `{0x8993A0, 0x68F970(no-op)}` を指す短 vtable）。
- vptr 書込: `0xA96B58` ← `0x89945A`（`mov [esi],0xa96b58`、FUN_008993F0）。`0xA96B78` ← `0x899356`（ctor `FUN_00899330`: `*p=0xA96B78`）/`0x89A831`（ahkWorld ctor `FUN_0089A230`）/`0x8DA23C`/`0xA12BD3` の **offset-0 書込 4 件**。加えて `FUN_00899330` は +8/+0xC/+0x10/+0x14 に `0xA96B64/68/70`（同一ラン内の別 view）を書く＝**1 オブジェクト内に複数 interface vptr を並置する Havok 標準構造**（b26 filter 系と同型）。

### 裁定

> **b13 表への再分類（143 ブロック目への追加）は不要**。`0xA96B78` は #73（`0xA96B58`、10 スロット）の**同一バイト列に対する派生 view**であり、独立ブロックとして数えると 1,934 スロットの二重計上になる。推奨対応は **b13 #73 に注記を1行追加**: 「`0xA96B78` = +0x20 派生 vptr（4 ヶ所 offset-0 書込、末尾 2 スロット view）」。これは b26 の "base phase / own phase" パターンと同種の**インターフェイス分割**と解する。

**判定: 解明（再分類不要・注記推奨）**（証拠 `v217_17_out.txt` §B-B2、`b13_nonrtti_vtable_classes.md` §3.5 #73）。

---

## §10 item10（v205 残5）「bhkCollisionFilter 差替経路」`FUN_0088B070` の上流追跡 — **解明（前提訂正）: 実体は Havok メモリ＋エラーハンドラのブートストラップ**

### `FUN_008B070` 本体（`v217_17_out.txt` §C / `v217_18_out.txt` §B）

1. ガード `if ([0xBA8040]==0)`。
2. **グローバル関数ポインタ表の実行時パッチ**: `[0xB2EB04]=0x889770`, `[0xB2EB08]=0x6078C0`（b13 §3.7 が「実行時パッチされるグローバル表」と記録した `0xB2EB04` の**書手を本タスクで特定**）。
3. `malloc(0x2C)` → ctor `FUN_008A7060` → vtable **`0xA95C70` = `.?AVbhkMemory@@`**（vtable_index row 1627）→ **`FUN_008BBA80(bhkMemory, 0, _printf, 0)`**＝printf デバッグ出力付きアロケータ初期化 → refcount リリース → vt+0x34 dtor。
4. TLS 経由で既存マネージャ参照 → `[0xBA7904] = <0x24B アロケータで確保＋ctor FUN_008A570 のオブジェクト>`。
5. **`FUN_00712590(fn, name)` を 15 回**（名前 0xA96100/0xA960EC/…、関数 0x8B7E10/0x8C9C80/…）＝ **Havok のコールバック/型レジストリ登録ブロック**。

### 上流（1 経路のみ）

> `FUN_0088B070` ← **`FUN_005350F0`@`0x535129`（唯一の caller）** ← `FUN_00441970` ← `FUN_004419FF`。
> `FUN_005350F0` 本体はブートストラップ直後に: `[0xBA7D98]` アロケータで size **0x124** を確保 → ctor **`FUN_00534430`**（vtable **`0xA56214` = `.?AVHavokError@@`**、row 750）→ 旧 `[0xBA7FB0]` を refcount リリースして **`[0xBA7FB0] = 新 HavokError`**（**実行時差替え**）。getter/setter は `FUN_00534020`。続けて `[0xBA7FB4]` にも 8B オブジェクト（vtable `0xA561E0`）を差替え。`[0xBA7FB0]` の消費者は ahkWorld ctor `FUN_0089A230`（0x89A43B/0x89AC2D/0x89AC7C）他 43 件。

### 裁定

> **v205 時点の「bhkCollisionFilter の実行時差替え経路」という位置づけは訂正**: `FUN_0088B070` は **Havok メモリ（bhkMemory）＋レジストリのブートストラップ**であり、直後の `FUN_005350F0` が差替えるのは **`[0xBA7FB0]` = グローバル HavokError ハンドラ（hkpError 相当、0x124B）**。bhkCollisionFilter 本体（vtable `0xA95CE4` 系、row 1603-1607）とは**別オブジェクト**で、経路上に collision filter への書込みは存在しない。

**判定: 解明（前提訂正）**（証拠 `v217_17_out.txt` §C / `v217_18_out.txt` §A-C / `v217_19_out.txt` / `v217_20_out.txt`）。

---

## §11 新規残点（本タスクから起票）

| ID | 内容 | 優先 |
|---|---|---|
| **V-217-1** | LifeState=3（Unconscious）の**書手**未検出（SetLifeState `FUN_005E6680` の即値 caller 21 件は {0,1,2,4,5,6} のみ）。間接（setter 経由/レジスタ経由）の有無を追う | 中 |
| **V-217-2** | Actor vt+0x1A0 述語の語義: `vt+0x284(0x30)`（Actor impl `0x5F1970`、引数分岐する汎用状態取得）の id 0x30 の意味と正式名 | 低 |
| **V-217-3** | [0xB3BCF0] 増分パス1 の単位: `3600.0 / [[0xB332E0+0x14]+0x24]` の T が fTimeScale（ゲーム秒/実秒）かフレーム長かの最終確定（FUN_004029E0 の `T·x/3600` 代数は時間(時)を生成、clamped at 24） | 中 |
| **V-217-4** | 0xB332E0 ゲーム時計 singleton のクラス正式名（RTTI 無し）と +0/+4/+8/+0xC/+0x14 のサブオブジェクト（NiTimeController 相当）配線の全容 | 低 |
| **V-217-5** | hk3AxisSweep vtable slot2-9/11-13 の名称（addEntity/removeEntity/updateEntity/castAabb/castSphere/collide/overflow/plotAabbs 系の対応付け） | 低 |
| **V-217-6** | `0xA96B78` の 4 vptr 書手（`FUN_00899330`/`FUN_0089A230` 内/`FUN_008DA1D0`/`FUN_00A12BC0`）の各クラス役割と +8/+0xC/+0x10/+0x14 に並置される `0xA96B64/68/70` view の対応 | 低 |
| **V-217-7** | 補助 process（0x154、ctor `FUN_00478730`）のクラス名（LowProcess/MiddleLowProcess のどれか）と vtable（ctor は vptr 書きなし＝後段で設定？）。+0x104/+0x5C8 の "1st/3rd person" 割当の実証 | 中 |
| （v204 継続） | 外部 setter `0x629440` 経由の KnockedState 値1 生成の実在性 | 低 |

---

## §12 自己検証・証跡

- **`probe_harness.py selftest` 4/4 PASS**（着手時）。
- 内部整合（独立一致 12 件）:
  1. weapon type テーブル `0xB086D0`（6 本連続）と v204 の逆順プーリング推定（Bow=5 / Staff=4）が一致。
  2. `FUN_006600D0` の 2 分岐（+0x5C8/+0x104）と `FUN_00611B00`（Character vtable slot+0x16C）/`FUN_00667BE0`（Player slot+0x14C）の 0x154 ctor が整合。
  3. `FUN_00478730` の `[+0x150]=owner` と destroy 経路 `FUN_0047AB80`(=`FUN_004796F0` 16 記録ループ) が「補助 process＝アドオン記録の容器」と整合。
  4. GetDead cond `0x4F4890` の `push 1` と Actor vt+0x198 の flag≠0 モード（{Dead,Dying}）が一致。
  5. SetLifeState `FUN_005E6680` の caller 即値 {0,1,2,4,5,6} と LifeState 名前テーブル（v204 §2-1）が整合（3 のみ不在＝V-217-1）。
  6. `FUN_004029D0` の 3 命令本文が task 記載の式 `[[0xB332E0+0x14]+0x24]` と**逐語一致**。
  7. [0xA2F938]=3600.0 / [0xA2F920]=24.0 / [0xA2FCC8]=60.0 / [0xA2F930]=24.0(float) / [0xA2F918]=12.0 の定数群が「時-秒-日」換算系として整合。
  8. 3600.0 の .text 参照は fld 経由（imm32 はアドレス 0xA71E4C）で、`push 0xE10` は 0 件 — raw 即値走査の取りこぼしを `imm32_refs3(0xA71E4C)` で是正。
  9. vtable `0xA9A710` slot14 = `0x8E24A0`（v195/v205 の castRay 確定値と一致）、slot17 = `0x8E2F20`（'3AxisSweep' 参照）が v205 §2-1 と一致。
  10. `0xA96B58` ランがちょうど 10 スロットで文字列に接続（0xA96B80）＝ b13 #73 の len=10 と一致。
  11. `FUN_0088B070` の唯一 caller が `FUN_005350F0@0x535129`（E8 生走査 1 件）、その差替え対象 `[0xBA7FB0]` の ctor vtable が `.?AVHavokError@@` であることを vtable_index で照合。
  12. `FUN_0060E4F0` が Character vtable `0xA6FC9C` と二次 vtable 3 本（+0x18/+0x5C/+0x68）を書くことを b13 §4.6 の Character 行（15-17）と照合。
- 制約遵守: 新規ファイルは本 1 件＋`%TEMP%\opencode/v217_*`（20 スクリプト＋20 出力）。docs/・既存 tmp/re・GT・TODO・session-memory・plan・cache・git は編集 0 件。ESM 読取 0 回。

---

## §13 Android 移植への示唆（実装しない）

- 装備エントリ（active process）と 3D アドオン記録（補助 process）は**別コンテナ**として分離実装すること（§1）。同一フィールドへの二重割当は再現しない。
- `IsDead(flag)` は flag=1（{Dying,Dead}）を公開、flag=0（+Normal）を FSM ガード用に残す。
- weapon type 名テーブル（6 本、`0xB086D0`）は enum→文字列表としてそのまま移植可能。
- LifeState は byte ではなく dword、既定 0、セーブ保存あり。SetLifeState は状態変化時にイベント送出する契約を再現すること。値 3 の設定経路は移植版で明示的に定義する（原版は未特定）。
- ゲーム時計（0xB332E0 相当）は「現在時刻(時, 0..24 wrap)＋時間尺度」の2フィールドとし、経過秒クロック（0xB3BCF0 相当）へは実時間/ゲーム時間の両パスで加算する設計を再現。INI キーは持ちたない runtime 値として扱う。
- Havok 起動順序: メモリ（bhkMemory＋printf）→ レジストリ登録 → HavokError グローバル差替え → world 構築（ahkWorld ctor が `[0xBA7FB0]` 等のグローバルを読む）の順を保つこと。
