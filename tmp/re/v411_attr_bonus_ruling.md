# v411 — V-410-1【高】: `FUN_005222D0` の identity と attr/skill ボーナス計算の完全裁定（派生残点第3弾）

- 採番: **v411**（派生残点群・probe prefix **v411_1..v411_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v406 の残点 V-406-1→v409→v410 を経て **V-410-1【高】（属性+10 書手 `0xB362E4/EC` と `FUN_005222D0` attr 計算の解釈）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_005222D0` の identity** | **[確定(命令＋GMST 文言)]** | **Player の attribute/skill/race ボーナス基礎値「再計算＋SET」fn**（param_1＝PlayerCharacter・呼出し8箇所＝record load `0x4471A0`／opcode 系3／skill band `0x523F40`／**chargen `FUN_00596980`**／menu `FUN_005C28F0`×2）。**attr 段（0..7）**: `base=FUN_00468AD0(attr)` → **`attr==class属性1 → +_DAT_00B362E4`／`attr==class属性2 → +_DAT_00B362EC`** → `if (base>100) base=100` → **`FUN_00468AF0(attr, ROUND(base))`（byte setter＋`FUN_009832E6` 経由の変更通知）**。**skill 段（0..0x14）**: `base=iVar10` → **major なら `+25`（`0xA492B0`）／非 major なら `×0.1（0xA2FC80）+5`** → **class spec 一致なら `+0.5（0xA2FAA0）×base +5`** → **race skill bonus 加算**（`[Player+0xE8]+0x50` の `(skill_id, bonus)` ×7 ループ・L216868-216876）→ clamp100 → **`[Player + 0xEC + idx] = ROUND(base)`（byte skill 配列21本）** → `FUN_0046ABF0(0x200)` 通知 |
| **`0xB362E4`/`0xB362EC` の正体** | **[確定(実文字列)]** | **float 型 GMST のキャッシュ**: `0x9E3C20`/`0x9E3C50` の登録パターン（`fld [0xA31E2C]=既定5.0f; push key; mov ecx,global; call 0x41BAE0`）で **key `0xA540E0` = `fAttributeClassPrimaryBonus`（既定 5.0）**・**key `0xA540FC` = `fAttributeClassSecondaryBonus`（既定 5.0）**。dtor thunk は `0xA1C1E0`/`0xA1C1F0`（`mov ecx,global; jmp 0x4055E0`） |
| **「属性+10」仮説** | **[確定(否定側)]** | class の属性ボーナスは **上記2 GMST（既定 各5.0）のみ** — **+10 は存在しない**（GMST で可変・既定値5）。ただしこの加算は **bVar2 条件付き**: `[Player+0xc]==7（bVar18）かつ class formID == [0xB37D00]（default class）` で **bVar2=false にすると class ボーナス・減衰計算をスキップ** [確定(命令)] |
| **「specialization スキル +10」仮説** | **[確定(否定側)]** | skill 加算の実値は **major=+25／非 major=×0.1+5／spec 一致=+0.5×base+5**（＋race bonus）— **単純な +10 はどこにも存在しない**。→ **GMST `sSpecialization` 文言 "You will gain +10 in all specialization skills" と実装値の乖離＝[確定]**（文言は chargen step のヘルプ表示のみ・v406 で確定） |
| **Primary/Secondary 取得 getter** | **[確定]** | **`FUN_00780F70 = return [x+0x38]`（＝class DATA+0x00＝属性1＝primary）**／**`FUN_00422DE0 = return [x+0x3C]`（＝DATA+0x04＝属性2＝secondary）** — いずれも ICF 型 one-liner（v406 教訓どおり）、**v261 フィールド表と完全整合**。比較は `L216798-216801`（attr id と class 属性の等値） |
| **skill 値の2系統** | **[確定]** | **byte 系 `[Player+0xEC+idx]`（21本・race/class/spec ボーナス込み基礎値を `FUN_005222D0` が SET）** と **float 系 `[Player+0x130+idx*4]`（成長/使用値・v410 の4書手）** が併存 — **依頼②の「major skill 初期値と +10」への実体は byte 系**（ただし +10 という値ではなく major+25/race bonus/spec 計算式） |
| **`iVar10 = FUN_004677F0()-1` の意味** | **[未確定]**（残点 V-411-1） | `FUN_004677F0`＝`[param+0xe]`（ushort）＋PC `[0xB333C4] virtual+0x170 → [+0x32]` 加算＋`[+0x10]/[+0x12]` clamp（L81348-81371）＝**level/進捗系 [推定高]** — arg の identity と「-1」の意味が未確定 |

## §2 証拠（v411_1..v411_3）

### §2.1 GMST 登録と cache（v411_1(a)）
```
0x9E3C20: fld [0xA31E2C](=5.0f) → push 0xA540E0('fAttributeClassPrimaryBonus')
          mov ecx, 0xB362E4 → call 0x41BAE0（GMST 登録）→ dtor thunk 0xA1C1E0
0x9E3C50: fld [0xA31E2C]         → push 0xA540FC('fAttributeClassSecondaryBonus')
          mov ecx, 0xB362EC → call 0x41BAE0 → thunk 0xA1C1F0
```
- 読取側は **`FUN_005222D0` 内2箇所のみ**（`0x5223AB fld [0xB362E4]` / `0x5223C2 fld [0xB362EC]` → `fadd [esp+0x14]`）＝**消費は class ボーナス加算のみ** [確定]。

### §2.2 attr/skill 計算の decompiled（v411_1(c)/v411_3(a)）
```c
// attr 段（L216786-216838）
iVar11 = FUN_00565BE0(0, i);                     // attr id 0..7
local_18 = (float)FUN_00468AD0(iVar11);          // base
if (bVar2) {
  iVar12 = FUN_00780F70();                       // = class[+0x38] 属性1
  fVar1 = _DAT_00B362E4;                         // fAttributeClassPrimaryBonus = 5
  if (iVar11 == iVar12 || (iVar12 = FUN_00422DE0(), fVar1 = _DAT_00B362EC, iVar11 == iVar12))
    local_18 += fVar1;                           // ★ primary/secondary に +5
  ... race-skill 関連 inner loop（major 判定 FUN_0051BF00 ×7 → 非 major は ×0.2 の inner）...
}
if (100.0f < local_18) local_18 = 100.0f;        // clamp
FUN_00468AF0(iVar11, ROUND(local_18));           // ★ attr byte SET
// skill 段（L216841-216883）
for (idx 0..0x14) {
  major = 7個 FUN_0051BF00 一致 ? 1 : -1;
  local_18 = major==1 ? iVar10 + 25.0(0xA492B0)
                      : iVar10 * 0.1(0xA2FC80) + 5.0;
  if (bVar2 && class_spec == FUN_006ECC80()) local_18 += iVar10*0.5(0xA2FAA0) + 5.0;
  for k in 0..6: if (race_bonus[k].id == skill) local_18 += race_bonus[k].val;  // [Player+0xE8]+0x50
  if (100 < local_18) local_18 = 100;
  [Player + 0xEC + idx] = (byte)ROUND(local_18); // ★★ byte skill SET
}
FUN_0046ABF0(0x200);
```
- 定数実測（v411_2(b)）: `0xA492B0=25`（double）／`0xA2FC80=0.1`／`0xA309F0=100`／`0xA2FE7C=100.0f`／`0xA3F3F0=5`（double・v406 一致）／`0xA2FAA0=0.5`／`0xA31E2C=5.0f`（GMST 既定）。
- `FUN_00468AF0`（L82387）: `idx=FUN_00565C50(0,id); [param_1+4+idx]=byte; FUN_009832E6(...)→virtual+0x40(8)` ＝**byte 値 setter＋変更通知** [確定]。

### §2.3 getter 2連（v411_3）
- `FUN_00780F70(param_1){ return [param_1+0x38]; }` ／`FUN_00422DE0(param_1){ return [param_1+0x3c]; }` — class form 経路（`mov ecx,[edi+0x104]` 帯で使用）で **DATA+0x00/+0x04 ＝ primary/secondary 属性**（v261 フィールド表と一致）。

## §3 残点 **V-411-1..4**
1. **【中】`iVar10 = FUN_004677F0()-1` の identity と「-1」**（arg は `[+0xe]` ushort＋PC `[virtual+0x170]→[+0x32]` 加算＋clamp 系＝level/進捗 [推定高]）— base の意味が解ければ skill 式の完全復元になる。
2. **【中】後続計算段**: `L216884 FUN_0046ABF0(0x200)` → `FUN_00547F80(attr0, attr5, iVar10, local_4, class_spec)` / `FUN_00548020(attr0, attr5)`（bVar18 分岐）/ `FUN_005479D0→FUN_004672B0(&actor[+0x24])`（param_2==0 分岐）— levelup/成長遷移の identity。
3. 【低】`bVar18 = [Player+0xc]==7` の formType==7 の意味（＋`DAT_00B37D00`＝default class formID の v261 継承確認）。
4. 【低】`[Player+0xE8]`＝race object ptr の裏取り（`[+0x50]` が race skill bonus 表であることの ctor/ロード側確認）。

## §4 検証・自己批評
- probe: **v411_1..v411_3 の3本全 EXIT=0**。
- **決定打の作り**: (i) **global cache の refs census（読み手＋登録部の両方）** → (ii) **登録部 machine の `push key` 実読**で GMST 名確定 → (iii) 計算 decompiled の定数を PE セクション実測（float/double 両方） → (iv) **ICF getter の body 実読（`[x+0x38]`/`[x+0x3C]`）を v261 フィールド表と突合** → (v) SET 行（`[Player+0xEC+idx]`）を body 終端で特定。
- 妥当性チェック: ①GMST 既定5⇔`fld [0xA31E2C]=5.0f` ⇔消費2箇所 — ②attr1/attr2 getter⇔v261 の +0x38/+0x3C 行 — ③major+25/spec+5/非 major×0.1+5 の定数⇔実測値 — ④byte 系/float 系の2配列⇔それぞれの SET/読取 fn — **四方向矛盾なし**。
- docs との不整合: **なし**（#151 依頼②の「属性+10／major 初期＋10／spec +10」3仮説はいずれも [確定(否定側)] に訂正される形で実体を確定）。docs/GT への反映は #47/#20 一括（v406/v409 の訂正事項も同様）。
- **教訓**:
  1. **.data の「未初期化グローバル」でも登録部（`push key; mov ecx,global; call`）を census すれば GMST 名と既定値が取れる** — v406 の「実行時グローバルで定数非保持」を**登録側から補完**できた。
  2. **2つの ICF one-liner getter（`[x+0x38]`/`[x+0x3C]`）は「呼び出し側の arg 経路」＋「既知フィールド表（v261）」の突合で型付け** — v406 の ICF 教訓を継続適用。
  3. **文言（GMST text）と実装値は別物** — "+10" 文言の実体は major+25/spec+5/attr+5 の複合計算。**仕様書・GMST 文言を実装値と断定しない**。
