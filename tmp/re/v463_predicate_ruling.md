# v463 — predicate slot35 正式意味・`param_4` 語の裁定（V-463-1..3 完遂・旧残点群消化第7弾）

- 採番: **v463**（旧残点群消化・probe prefix **v463_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-463-1..3（v462 残点）** — 1=`FUN_006DF060` identity→predicate slot35 正式名／2=`FUN_005222D0` 一語名と `FUN_00668B30` 第4引数（0）の語／3=旧残点（V-406-3..5）への移行判断を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-463-1: `FUN_006DF060`（`NiLookAtInterpolator` idx35 override）identity** | **[確定相当]** | L515290: **`undefined4 __thiscall FUN_006df060(int self, int *palette)`: `if (*(int*)(self+0x10)==0)`（cache 未解決）` { iVar1 = virtual[palette]+0x4C(*(self+0x14))`（**name を palette で resolve**）` if (iVar1==0) return 0;`（**resolve 失敗→false**）` *(self+0x10)=iVar1;`（**cache 格納**）` } return 1;`（resolve 済み→true）[確定(構造)]** → **`FUN_006DF060`＝「interpolator の依存（`self+0x14`＝name）を palette で lazy resolve し、未解決なら0・解決済みなら1を返す predicate [確定相当]`** |
| **V-463-1: predicate slot35（`+0x8C`）の正式名** | **[確定相当]** | **「interpolator の依存解決（dependencies resolve）完了 predicate」[確定相当]** — 根拠: (i) 恒真 impl `0x6FDFF0`（27/28 クラス）＝**resolve 不要 interpolator**、(ii) `NiLookAtInterpolator`（look-at ターゲット名に依存）のみ override で name resolve を実行、(iii) **v405 の `"failed to resolve dependencies"` ログ（`0xA79CD8`）経路と完全整合**（predicate が false のときに出るログ）→ **slot35＝`NiInterpolator::CanResolveDependencies` 系 [確定相当]**（正確な英名は RTTI 由来で docs 無し＝名前自体は [未確定]） |
| **V-463-2: `FUN_00668B30` 第4引数 `param_4`（呼び出しでは0）の語** | **[確定相当]** | L429202-429218: **`if (param_4==0) { param_4 = FUN_00446af0(uVar5); if (param_4==0) goto LAB_00668c15; }`（0 なら `FUN_00446AF0` で解決・失敗で早期 return）` fVar6 = FUN_0065fad0(param_4); FUN_00668070(param_4, 0, param_5=='\0'); if (*(char*)(self+0x6e5) && param_5=='\0') …` [確定(構造)]** → **`param_4`＝「class/form ptr（0＝未解決 → `FUN_00446AF0` で lookup 解決）[確定相当]`**、`param_5`＝char flag（`FUN_00668070` 第3引数へ転送＝usage 更新の種別 [推定]） |
| **V-463-2: `FUN_005222D0` の一語名** | **[未確定]（残点）** | **string pushes 0件（error 文言なし）・docs に annotation はあるが名前は `FUN_` のまま [確定(否定側)]**。caller 8件は v406 の記載と完全一致（record load `FUN_00447300`／script opcode 系3（`FUN_0050A8A0/0050C430/0050CD30`）／skill band `FUN_00523EB0`／chargen `FUN_00596980`／menu `FUN_005C28F0`×2）→ **機能（class 属性→derived AV 初期適用＋class 適用）は v462 で確定相当、一語名は docs 由来で付けられない＝残点** |
| **V-463-3: 旧残点（V-406-3..5）への移行** | **[確定]（方針）** | **V-464 から旧残点（V-406-3 `FUN_006ECC80` 残り typed【中】／V-406-4 `DATA+0x2C` 意味／V-406-5 `PTR_DAT_00B10D90` 所有 fn）へ軸を移す [確定（作業方針）]** |

## §2 証拠（v463_1）

### §2.1 `FUN_006DF060`（v463_1a）
```
L515290: undefined4 __thiscall FUN_006df060(int param_1, int *param_2) {
  if (*(int*)(param_1 + 0x10) == 0) {                    // cache unresolved
    iVar1 = (**(code**)(*param_2 + 0x4c))(*(param_1 + 0x14));   // palette->Get(name)
    if (iVar1 == 0) return 0;                            // failed -> false
    *(int*)(param_1 + 0x10) = iVar1;                     // cache resolved
  }
  return 1; }                                            // resolved -> true
```

### §2.2 `FUN_00668B30` param_4（v463_1c）
```
L429187: void __thiscall FUN_00668b30(int self, float param_2, float param_3,
                                       int param_4, char param_5)
L429202:   if (param_4 == 0) {
L429204:     param_4 = FUN_00446af0(uVar5);       // resolve class/form
L429205:     if (param_4 == 0) goto LAB_00668c15;  // fail -> early return
L429214:   fVar6 = (float10)FUN_0065fad0(param_4);
L429216:     FUN_00668070(param_4, 0, param_5 == '\0');
L429218:   if ((*(char*)(self + 0x6e5) != 0) && (param_5 == 0)) { ...
```

### §2.3 `FUN_005222D0` callers（v463_1b）
```
string pushes: 0  (no error text)   [否定側]
callers of FUN_005222d0 (8, all == v406 listing):
  0x44733A FUN_00447300 (record load) / 0x50AB73 FUN_0050a8a0 /
  0x50C77C FUN_0050c430 / 0x50CF9D FUN_0050cd30 (script opcodes) /
  0x523FF4 FUN_00523eb0 (skill band) / 0x596AE5 FUN_00596980 (chargen) /
  0x5C29E6 + 0x5C2AC0 FUN_005c28f0 (menu x2)
```

## §3 残点 **V-464-1..3**
1. 【中】**旧残点 V-406-3**: `FUN_006ECC80`（CLAS getter・ICF 共用）の残り E8=500+ の CLAS 型再 typed（特に actor/chargen/menu 帯 `0x5Exxxxx/0x5Bxxxxx/0x58xxxxx`）。
2. 【低】**旧残点 V-406-4／V-406-5**: `DATA+0x2C`（form+0x64）と `DATA+0x31`（form+0x69）の意味（UESP 48/52byte 差分・裏取り待ち）／`PTR_DAT_00B10D90` の所有 fn と spec 名前テーブルの書手。
3. 【低】**`FUN_005222D0` 一語名の別法**（docs 名前無し・error 文言無し＝caller の script opcode 名（`FUN_0050CD30` 等の opcode 特定と接続）から付与を試みるか）と **predicate slot35 の英名**（RTTI 由来で docs 無し＝`CanResolveDependencies` [推定] の裏取り）。

## §4 検証・自己批評
- probe: **v463_1 の1本 EXIT=0**（V-463-1 完遂（`FUN_006DF060`＝lazy-resolve predicate＋slot35 正式意味）／V-463-2 部分（`param_4` 確定相当・一語名は残点）／V-463-3 方針確定）。
- **決定打の作り**: (i) **override fn の decompiled を読むだけ**で「cache 未解決→palette lookup→失敗で0／格納して1」の lazy-resolve 構造が露出（**`virtual+0x4C`＝palette lookup は v405/v456 と同型**）→ (ii) **恒真 impl（27/28）と override（1/28）の対比＋v405 の "failed to resolve dependencies" ログ**で slot35 の意味を確定 → (iii) **`param_4==0` 分岐の `FUN_00446AF0`（解決 fn）**で「class/form ptr（0＝未解決）」と即決 → (iv) `FUN_005222D0` は **string push 0件の否定側**で docs 名前なしを確定。
- 妥当性チェック: ①`virtual+0x4C(name)`⇔v405 slot19＝palette lookup（`NiTMapBase::Lookup`）— ②`self+0x10` cache⇔`self+0x14` name の対（resolve 結果と key）— ③27/28 恒真⇔LookAt のみ name 依存（look-at ターゲット名）— ④`FUN_00446AF0`＝解決 fn⇔`param_4==0` の fallback— ⑤caller 8件⇔v406 listing 完全一致 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_005222D0` は docs 名前なし＝命名されない fn として整合）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **override のみを decompiled で読めば slot の正式意味が分かる** — 恒真 impl（27/28）は中身が `mov al,1;ret` で情報ゼロ、**唯一の override（`FUN_006DF060`）の body が「何を判定しているか」を全部語る**。
  2. **「恒真 impl の多数派 vs override の少数派」の対比が slot semantic の決定打** — 多数派が何もしない＝「默认成功」、少数派が実処理＝「条件付き成功」。
  3. **過去のログ文言（v405 `"failed to resolve dependencies"`）と predicate false 経路の接続**で、slot の英名系（`CanResolveDependencies`）が裏付けられる（**古い裁定の error 文言を predicate 調査で再利用**）。
  4. **`if (arg==0) arg = resolve_fn(...)` の pattern は「引数＝ptr（0＝未解決）」の定番** — `param_4` の語がこれで即決。
  5. **一語名が docs に無い fn は string push 0件の否定側で「名付け不能」と確定し、残点の形を変える**（機能確定相当で残す／名前だけ残す）。
