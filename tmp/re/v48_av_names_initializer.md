# V48: FUN_00565CC0 AV 名 idx 0..39 の初期化元と actorValue 名一覧の完成

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, Steam)
作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
解析: 自前 PE パーサ (struct) + capstone 5.0.7 + Ghidra デコンパイル `docs\Oblivion_decompiled.c`（lowercased 照合）
前作: `tmp\re\v41_effect_display_name_spec.md`（表B idx40..71 の32件確定）
作成日: 2026-10-24 / 新規1ファイル（既存ドキュメント無改変）

---

## 1. セルフテスト 4 件（実測・全 PASS）

| # | 検査 | 実測 | 結果 |
|---|---|---|---|
| ① | bytes@0x504050 | `518b4c24188b5424` | **PASS** |
| ② | bytes@0x4FB580 | `578b7908` | **PASS** |
| ③ | string@0xA7035C | `.\AI\CombatController.cpp` | **PASS** |
| ④ | opid 0x0015 impl | entry#21 @0xB0B290, impl(+0x18)=`0x504560` | **PASS** |

---

## 2. 初期化元の特定（過程と結果）

### 2.1 decompiled.c 走査（書く側探索）

- `ptr_dat_00b12758`（表A）= **読む側2行のみ**（L265538/265539、getter FUN_00565cc0 内）。書く側・配列初期化はデコンパイルに存在しない。
- `ptr_dat_00b12868`（表B）= L265543 の1行のみ、`ptr_dat_00b127f8`（表C）= L265555/265556 の2行のみ。
- bss 0xB3A034 / 0xB3A174 のラベル直接ヒット0件（**初期化が別関数群に分散**しているため）。

### 2.2 .text バイト列走査（決定打）

`.text` 全域から `imm32 = 0xB3A034 + 8*i`（i=0..39）を走査 → **40/40 ターゲットすべて 2 ヒット**（計80箇所）:

| 参照系統 | 範囲 | 構造 |
|---|---|---|
| **初期化 thunk** | 0x9F9660 + 0x20*i（i=0..39） | `push fallback / push key / mov ecx, 0xB3A034+8i / call 0x419A70 / push dtor_thunk / call 0x981FB4 / pop ecx / ret`（32バイト/個） |
| **dtor thunk** | 0xA238A0 + 0x10*i（i=0..39） | `mov ecx, 0xB3A034+8i / jmp 0x4055E0`（16バイト/個、atexit 経由で CRT 終了時に登録解除） |

### 2.3 初期化 thunk 全40件の検証（capstone、0/40 ミスマッチ）

- `this`（ecx）= 0xB3A034+8i が **40/40 一致**。
- 第1 call = **0x419A70** が **40/40 一致**（rel32 解決済み）。
- dtor push = 0xA238A0+0x10*i が 40/40 一致。

### 2.4 呼び出し元（初期化登録テーブル）

`.rdata` の **0xA2AA78 起点の連続関数ポインタ配列**（.CRT$XCU 相当）が thunk を順に登録:
- 0xA2AA78..0xA2AB14 = 0x9F9660+i*0x20（**表A 40件**）
- 0xA2AB18..0xA2AC2C+ = 0x9F9B60+i*0x20（**表C 系 thunk 71件以上**、末尾 0x9FA400/0x9FA420 まで連続確認）

### 2.5 FUN_00419A70 の実体（decompiled L16472-16501、V41 の記述を訂正）

```c
undefined4 * __thiscall FUN_00419a70(undefined4 *param_1, int param_2, undefined4 param_3)
{
  param_1[1] = param_2;              /* this[1] = key   (ローカライズキー) */
  *param_1   = param_3;              /* this[0] = value (既定の英名文字列) */
  if (param_2 != 0) {
    if (FUN_0055e000(param_2, &param_2) != 0) {   /* 中央レジストリ 0xB35574 で重複検査 */
      FUN_004a7a60("Setting key '%s' already used in map.\n...", param_1[1]);  /* @0xA32490 式 */
      return param_1;
    }
    FUN_00412d30(param_1[1], param_1);            /* レジストリへ key -> &entry を登録 */
  }
  return param_1;
}
```

**訂正（V41 表記の是正）**: 0xB3A034+ のエントリは **`String {buf,len,cap}` ではなく `{char* value@+0, char* key@+4}` の8バイトレジストリ登録オブジェクト**。getter `FUN_00565cc0` の `*表A[i]` は value（= thunk の fallback 英名）を返す。ローカライズ辞書の適用は中央レジストリ（FUN_00412D30 登録 / FUN_0055E000 検査）側で行われる。V47 の String 型（0x4028D0 等）は本オブジェクトには使われない。

### 2.6 idx 0..39 の (idx → bss → キー → fallback) 全40件

| idx | entry bss VA | ローカライズキー | fallback（=getter 返却既定名） | thunk VA |
|---|---|---|---|---|
| 0 | 0xB3A034 | sAttributeNameStrength | Strength | 0x9F9660 |
| 1 | 0xB3A03C | sAttributeNameIntelligence | Intelligence | 0x9F9680 |
| 2 | 0xB3A044 | sAttributeNameWillpower | Willpower | 0x9F96A0 |
| 3 | 0xB3A04C | sAttributeNameAgility | Agility | 0x9F96C0 |
| 4 | 0xB3A054 | sAttributeNameSpeed | Speed | 0x9F96E0 |
| 5 | 0xB3A05C | sAttributeNameEndurance | Endurance | 0x9F9700 |
| 6 | 0xB3A064 | sAttributeNamePersonality | Personality | 0x9F9720 |
| 7 | 0xB3A06C | sAttributeNameLuck | Luck | 0x9F9740 |
| 8 | 0xB3A074 | sDerivedAttributeNameHealth | Health | 0x9F9760 |
| 9 | 0xB3A07C | sDerivedAttributeNameMagicka | Magicka | 0x9F9780 |
| 10 | 0xB3A084 | sDerivedAttributeNameFatigue | Fatigue | 0x9F97A0 |
| 11 | 0xB3A08C | sDerivedAttributeNameEncumbrance | Encumbrance | 0x9F97C0 |
| 12 | 0xB3A094 | sSkillNameArmorer | Armorer | 0x9F97E0 |
| 13 | 0xB3A09C | sSkillNameAthletics | Athletics | 0x9F9800 |
| 14 | 0xB3A0A4 | sSkillNameBlade | Blade | 0x9F9820 |
| 15 | 0xB3A0AC | sSkillNameBlock | Block | 0x9F9840 |
| 16 | 0xB3A0B4 | sSkillNameBlunt | Blunt | 0x9F9860 |
| 17 | 0xB3A0BC | sSkillNameHandToHand | Hand To Hand | 0x9F9880 |
| 18 | 0xB3A0C4 | sSkillNameHeavyArmor | Heavy Armor | 0x9F98A0 |
| 19 | 0xB3A0CC | sSkillNameAlchemy | Alchemy | 0x9F98C0 |
| 20 | 0xB3A0D4 | sSkillNameAlteration | Alteration | 0x9F98E0 |
| 21 | 0xB3A0DC | sSkillNameConjuration | Conjuration | 0x9F9900 |
| 22 | 0xB3A0E4 | sSkillNameDestruction | Destruction | 0x9F9920 |
| 23 | 0xB3A0EC | sSkillNameIllusion | Illusion | 0x9F9940 |
| 24 | 0xB3A0F4 | sSkillNameMysticism | Mysticism | 0x9F9960 |
| 25 | 0xB3A0FC | sSkillNameRestoration | Restoration | 0x9F9980 |
| 26 | 0xB3A104 | sSkillNameAcrobatics | Acrobatics | 0x9F99A0 |
| 27 | 0xB3A10C | sSkillNameLightArmor | Light Armor | 0x9F99C0 |
| 28 | 0xB3A114 | sSkillNameMarksman | Marksman | 0x9F99E0 |
| 29 | 0xB3A11C | sSkillNameMercantile | Mercantile | 0x9F9A00 |
| 30 | 0xB3A124 | sSkillNameSecurity | Security | 0x9F9A20 |
| 31 | 0xB3A12C | sSkillNameSneak | Sneak | 0x9F9A40 |
| 32 | 0xB3A134 | sSkillNameSpeechcraft | Speechcraft | 0x9F9A60 |
| 33 | 0xB3A13C | sTraitNameAggression | Aggression | 0x9F9A80 |
| 34 | 0xB3A144 | sTraitNameConfidence | Confidence | 0x9F9AA0 |
| 35 | 0xB3A14C | sTraitNameEnergy | Energy | 0x9F9AC0 |
| 36 | 0xB3A154 | sTraitNameResponsibility | Responsibility | 0x9F9AE0 |
| 37 | 0xB3A15C | sVirtueNameBounty | Bounty | 0x9F9B00 |
| 38 | 0xB3A164 | sVirtueNameFame | Fame | 0x9F9B20 |
| 39 | 0xB3A16C | sVirtueNameInfamy | Infamy | 0x9F9B40 |

補強: 表A 配列 `0xB12758`（.data、**ファイル値として全40件が bss アドレスを静的初期化済み**）= `0xB3A034+8i` と 40/40 完全一致。

---

## 3. actorValue 一覧（73スロット）

getter `FUN_00565cc0` の有効範囲は **idx 0..71（0x28/0x48 境界）= 72種**。**73種目は EXE に定義が存在しない**（idx≥72 は `return 0`。VM名テーブル 0xB0A1A8 の [72]='Begin' は AV 名テーブル外の隣接別データ）。充足 = **72/73（1スロットは仕様上なし）**。

根拠: A=表A初期化 thunk fallback（V48） / B=表B .rdata char* 実読（V41） / V=script-VM 名テーブル 0xB0A1A8 実読（V48、3系統突合済み）

| idx | 名前（fallback/英名） | 根拠 |
|---|---|---|
| 0 | Strength | A+V |
| 1 | Intelligence | A+V |
| 2 | Willpower | A+V |
| 3 | Agility | A+V |
| 4 | Speed | A+V |
| 5 | Endurance | A+V |
| 6 | Personality | A+V |
| 7 | Luck | A+V |
| 8 | Health | A+V |
| 9 | Magicka | A+V |
| 10 | Fatigue | A+V |
| 11 | Encumbrance | A+V |
| 12 | Armorer | A+V |
| 13 | Athletics | A+V |
| 14 | Blade | A+V |
| 15 | Block | A+V |
| 16 | Blunt | A+V |
| 17 | Hand To Hand (VM: HandToHand) | A+V |
| 18 | Heavy Armor (VM: HeavyArmor) | A+V |
| 19 | Alchemy | A+V |
| 20 | Alteration | A+V |
| 21 | Conjuration | A+V |
| 22 | Destruction | A+V |
| 23 | Illusion | A+V |
| 24 | Mysticism | A+V |
| 25 | Restoration | A+V |
| 26 | Acrobatics | A+V |
| 27 | Light Armor (VM: LightArmor) | A+V |
| 28 | Marksman | A+V |
| 29 | Mercantile | A+V |
| 30 | Security | A+V |
| 31 | Sneak | A+V |
| 32 | Speechcraft | A+V |
| 33 | Aggression | A+V |
| 34 | Confidence | A+V |
| 35 | Energy | A+V |
| 36 | Responsibility | A+V |
| 37 | Bounty | A+V |
| 38 | Fame | A+V |
| 39 | Infamy | A+V |
| 40 | Magicka Multiplier | B+V |
| 41 | Night Eye Bonus | B+V |
| 42 | Attack Bonus | B+V |
| 43 | Defend Bonus | B+V |
| 44 | Casting Penalty | B+V |
| 45 | Blindness | B+V |
| 46 | Chameleon | B+V |
| 47 | Invisibility | B+V |
| 48 | Paralysis | B+V |
| 49 | Silence | B+V |
| 50 | Confusion | B+V |
| 51 | Detect Item Range | B+V |
| 52 | Spell Absorb Chance | B+V |
| 53 | Spell Reflect Chance | B+V |
| 54 | Swim Speed Multiplier | B+V |
| 55 | Water Breathing | B+V |
| 56 | Water Walking | B+V |
| 57 | Stunted Magicka | B+V |
| 58 | Detect Life Range | B+V |
| 59 | Reflect Damage | B+V |
| 60 | Telekinesis | B+V |
| 61 | Resist Fire | B+V |
| 62 | Resist Frost | B+V |
| 63 | Resist Disease | B+V |
| 64 | Resist Magic | B+V |
| 65 | Resist Normal Weapons | B+V |
| 66 | Resist Paralysis | B+V |
| 67 | Resist Poison | B+V |
| 68 | Resist Shock | B+V |
| 69 | Vampirism | B+V |
| 70 | Darkness | B+V |
| 71 | Resist Water Damage | B+V |
| 72 | （範囲外・EXE に定義なし → getter は 0 返却） | getter 上限 0x48 |

---

## 4. 表A vs 表C vs 表B vs VM テーブル（突合）

| 表 | ベース | getter | 有効 idx | 形態 | 初期化 thunk | 判明用途 |
|---|---|---|---|---|---|---|
| **A** | 0xB12758（.data、ポインタ→0xB3A034+8i） | FUN_00565cc0 | 0..39 | entry{value,key} を間接参照し value を返す | **0x9F9660+0x20i（40件）** | **AV 表示名** |
| **C** | 0xB127F8（.data、ポインタ→0xB3A174+8i） | FUN_00565cf0 | 0..32 | 同上（間接） | **0x9F9B60+0x20i（Desc 33件 + Icon 系 71件以上続行）** | **AV 説明文**（idx0..7=sAttributeDesc* / 8..11=sDerivedAttributeDesc* / 12..32=sSkillDesc*）+ idx33+= sAttributeIcon*（fallback は `Menus\Stats\stat_pop_icon_*.dds` パス = アイコンパス表） |
| **B** | 0xB12868（.data） | FUN_00565cc0 | 40..71 | **.rdata の char* を直接返す**（ローカライズなし・英文固定） | 不要（静的） | 追加 AV 名（V41 確定32件） |
| **VM** | 0xB0A1A8（.data、char*72件） | script VM のデバッグ/ログ用（"GetActorValue: %s >> %0.2f" L191015/L191422、名前逆引き __stricmp L197698） | 0..71 | char* 直接 | 不要（静的） | **スクリプト VM 用英名72件**（A/B と idx 対応で完全一致。差異は空白のみ: Hand To Hand/Heavy Armor/Light Armor ↔ HandToHand/HeavyArmor/LightArmor） |

**突合結果**:
- 表A fallback ≡ VM[0..39]（空白差を除き 40/40 一致）→ idx 割当の完全相互検証。
- 表B[40..71] ≡ VM[40..71]（空白差を除き 32/32 一致）。
- 表C[0..32] は表A[0..32] と**同一 AV 体系の別用途**（名前 vs 説明文）。キー命名系: Name→Desc の置換だけで対応（sAttributeNameStrength→sAttributeDescStrength 等 33/33 対応）。
- 表C 配列はメモリ上で 0xB12868 以降を表B と共有するが、表C 有効 idx≤32 / 表B 有効 idx≥40 のため実運用で衝突しない（表C[33]==表B[5]==0 を実測一致確認）。

---

## 5. 関係性図

```
[rdata 初期化テーブル 0xA2AA78.. (CRT$XCU 相当, 111+ エントリ)]
        |  _initterm 相当で順に実行
        v
[初期化 thunk 0x9F9660+0x20i (i=0..39)]          [dtor thunk 0xA238A0+0x10i]
  push fallback(英名)   <- .rdata 'Strength' 等      mov ecx, entry
  push key              <- .rdata 'sAttributeName*' 等  jmp 0x4055E0 (String-dtor風 tail)
  mov ecx, entry        <- bss 0xB3A034+8i               ^ atexit(0x981FB4) で登録
  call 0x419A70  --------+
  push dtor_thunk; call 0x981FB4(atexit); pop; ret
        |
        v
[FUN_00419A70  登録オブジェクト初期化]
  entry = { value=fallback, key }  (8バットではなく8バイトの2ポインタ)
  FUN_0055E000: 中央レジストリ 0xB35574 で key 重複検査
  FUN_00412D30: レジストリへ key -> &entry 登録 (衝突時 0x4A7A60 警告)
        |
        v
[表A 0xB12758 (40 ptr, 静的初期化済み)] --[i]--> [entry 0xB3A034+8i {value,key}]
        |
        v
[FUN_00565CC0(idx)] -- idx<0x28: *表A[idx] = value(英名/ローカライズ解決後)
        |             -- 40<=idx<0x48: 表B[0xB12868+idx*4] = .rdata char* 直返
        +--> 消費者: FUN_00413A90 表示器 (sprintf "%s %s", prefix=V41プレフィックス, AV名)
        +--> 消費者: UI / MGEF 効果名 / コンソール

[表C 0xB127F8 (33 ptr)] --> [entry 0xB3A174+8i] <- thunk 0x9F9B60+0x20i (Desc)
        --> FUN_00565CF0(idx<=0x20) = AV 説明文 (状態画面ヘルプ)
[VM 0xB0A1A8 (72 char*)] --> script VM (GetActorValue/GetBaseActorValue ログ、名前逆引き)
[ESM/コンソール] --> actorValue フィールドは idx 数値で授与。名前解決は FUN_00565CC0 経由のみ
```

---

## 6. 未解決点

1. **表C 系 thunk の総枚数**: CRT テーブルは最低111エントリ（表A 40 + 表C系 71+、0x9FA400/0x9FA420 まで連続確認）。表C 系の末尾（sAttributeIconSmall* 以降の詳細キー）は本タスクの主対象（表A idx0..39）外のため未抽出。用途（アイコンパス表/別UI）は fallback が dds パスであることのみ判明。
2. **中央レジストリ 0xB35574 の翻訳適用経路**: FUN_00412D30 登録以降、外部から value を差し替える経路（ローカライズ辞書ローダ）は FUN_0055E000/FUN_00412D30 の内部解析が未実施のため未同定。
3. **VM テーブル [72]='Begin' [73]='' [74]=0x10 の正体**: AV 名テーブル（0..71）の直後に隣接する別データ構造。本タスクの範囲外。

---

（本レポートは新規作成1ファイル。既存ドキュメント・%TEMP% 外への書込みなし。スクリプトは `%TEMP%\opencode\v48_*.py` のみ。）
