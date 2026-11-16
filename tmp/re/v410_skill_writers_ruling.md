# v410 — V-409-1【高】: Player skill 値の書手経路の完全確定と「+10 初期適用」裁定（派生残点第2弾）

- 採番: **v410**（派生残点群・probe prefix **v410_1..v410_5 の5本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v406 の残点 **V-406-1** → v409 で部分解 → 本 v410 で **V-409-1【高】（+10 初期適用 fn）** に最終リーチ（skill 側は経路完全確定、属性+10 は残点へ切り分け）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **Player skill 値（`[Player+0x130+idx*4]` float×21）の書手と呼出し元の全数** | **[確定]** | **書手4系統は間接（vtable/virtual）不在を含め全数確定**: ①setter `FUN_0065FA40` ← **E8=2: script opcode `FUN_0050D020`（AdvancePCSkill/ModPCSkill）＋ `FUN_00668B30`**（data refs 0 件＝vtable 登録なし）②全リセット `FUN_00668030`(=0) ← Player ctor `FUN_0066A740` ＋ `FUN_0066C200` ③使用更新 `FUN_00668070` ← **E8=4: `FUN_0066C200`／`FUN_00668B30`／`FUN_0066A3D0`@0x66A3DC／head 0x4B5720@0x4B585B** ④保留適用 `FUN_0066C200` ← **E8=2: chargen 帯 `0x596B4B`（head FUN_00596980）＋ menu `FUN_005C28F0`@0x5C2A54**。**4書手とも data refs=0 件（v410_1(a)）＝virtual/vtable 書手なし**・decompiled `+ 0x130)` 直接 form も全件が virtual disp（slot76 等）か別クラスフィールドで **Player skill 配列の追加書手なし（v410_1(b)）**＝**5系統目の否定** |
| **`call@0x668BB3` の所属（v406/v409 訂正）** | **[確定]（v406/v409 表記を訂正）** | 該当 call は **`FUN_00668B30` の本体内**（Ghidra の `find_head` が真 head `0x668B30` を飛び越し `FUN_006684B0` を返した精度誤り）。**v406 ruling「setter caller＝FUN_006684B0」および v409 同様の記述は誤り → `FUN_00668B30` に訂正**。`FUN_006684B0` は **LevelUpData_1.txt への debug ダンプ**（L429140-429174: virtual+0x2C で write・`[Player+0x658..+0x6C7]`（28 dwords）を出力）で **setter 呼び出しなし** |
| **`FUN_00668B30` の実体** | **[確定(命令＋decompiled)]** | **skill 値の統合加算 fn（thiscall5引数: player, skill, amount, rec, flag）**: `FUN_00565C50(2,skill)`→idx → **`current = [player+0x130+idx*4]` 読み** → `+ amount` → **`FUN_0065FA40(player, skill, new)` で即時 setter** → `FUN_0065FAD0`（しきい値比較）→ 超えたら **`FUN_00668070`（usage 更新）** → **余り分を `[player+0x5B0][idx] +=`（保留増分へ再加算）**（machine 0x668B74-0x668C0F・decompiled L429187 起） |
| **保留増分 `[+0x5B0]` の供給状態** | **[推定高: 実質0]` | 供給元は `FUN_00668B30` のみ（v406_19 全 grep）で、その呼出しは **E8=1（`FUN_00668C30` ＝ Player vtable slot231 dead slot・v409 六系統否定）＋ data refs 0（v410_1）** → **保留増分は実行時に供給されず、`FUN_0066C200` の適用も常に0増分**（funnel 断定。残る唯一の供給経路の否定は v409 で済み） |
| **chargen の適用呼び出しの実体** | **[確定(命令)]** | `0x596B45: mov ecx,[0xB333C4]（player global）→0x596B4B: call FUN_0066C200` → `call FUN_0057A7D0` → `FUN_005F2530(player, 1.0)`（`fld [0xA379B4]=1.0f`）→ `FUN_005F1910(player, 9)` → `fild` — **virtual+0x2DC の list walk 直後の player 一括処理**（0x596A50..0x596BC0 帯・decompiled 未収録の chargen fn） |
| **「+10 初期適用 fn」への最終判定** | **[部分確定]: skill 側=[推定高]不在／属性側=[未確定] 残置** | **(skill) 本系統（書手4系統）に chargen の +10 加算は不在** — chargen 経由は `FUN_0066C200`（供給0）のみ、定数加算を伴う実行系は opcode `ModPCSkill`（script/コンソール・`[modpcs Blunt, 10]` の +10 指定可）だけ → **「chargen が class/spec 由来で skill に+10 を加える fn は存在しない [推定高]**（＝GMST 文言の "+10" は display のみか opcode 解釈の問題に帰着）。**(attribute) `FUN_005222D0` の attr 計算（`_DAT_00B362E4`/`_DAT_00B362EC` 実行時グローバル加算・L216798-216802）は残存** — 書手未調査 → **[未確定]（残点 V-410-1）** |

## §2 証拠（v410_1..v410_5）

### §2.1 間接書手の否定（v410_1）
- data refs: `FUN_0065FA40` / `FUN_0066C200` / `FUN_00668070` / `FUN_00668B30` の **4者とも0件** → vtable/ディスパッチ登録なし（v409 教訓の逆方向確認＝**direct E8 のみ**）。
- decompiled `+ 0x130)` 全ヒット: virtual disp（`*X + 0x130)`＝slot76 系）と別クラスの field/ptr のみで **Player skill 配列の直書き増分なし**。

### §2.2 `FUN_00668B30` の body（v410_3(a) machine from head）
```
668B82: push ebp; push 2; call 0x565C50(2, skill)   ; idx668B90: fld [esi+ecx*4+0x130]            ; current value668B9F: fadd [esp+0x20]（amount 引数）
668BB3: call 0x65FA40（esi=player, ebp=skill, new）; ★ 即時 setter
668BC3: call 0x65FAD0 / fcomp（しきい値比較）
668BE2: call 0x668070（超過時の usage 更新）
668BF4: push ebp; push 2; call 0x565C50; [esi+0x5b0][idx] += arg  ; ★ 余りを保留増分へ
```
- decompiled: `FUN_00668b30(int param_1, float param_2, float param_3, int param_4, char param_5)` L429187。

### §2.3 保留適用の callers（v410_4）
- `FUN_0066C200` E8=**2**: `0x596B4B`（head `FUN_00596980`・chargen 帯）／`0x5C2A54`（head `FUN_005C28F0`・menu 帯・FUN_005222D0 の caller と同 fn）。
- `FUN_00668070` E8=**4**: `0x4B585B`（head 0x4B5720・未注記）／`0x668BE2`（FUN_00668B30 内）／`0x66A3DC`（`FUN_0066A3D0`）／`0x66C3A4`（FUN_0066C200 内）。

### §2.4 chargen 側 machine（v410_5(c)）
```
596B36: mov eax,[edx+0x2dc]; call eax      ; virtual+0x239 list要素処理（walk: [esi+4]）
596B45: mov ecx,[0xb333c4]                 ; player global
596B4B: call 0x66c200                      ; ★ player へ保留適用596B50: call 0x57a7d0
596B55: fld [0xa379b4](1.0f) → FUN_005f2530(player, 1.0)
596B6A: push 9; call 0x5f1910 → fild        ; FUN_005F1910(player, 9) を float 化
```
- 注: `FUN_00596980` の decompiled body は L303994-304020（30行・message handler）のみで **machine 0x596B4B を含む帯が Ghidra 切り出し外** — v406 教訓「decompiled 未収録帯は machine 補完」を再適用。

## §3 残点 **V-410-1..3**
1. **【高】属性+10 の書手** — `_DAT_00B362E4`/`_DAT_00B362EC`（.data 実行時グローバル）の**書き元**特定と `FUN_005222D0` attr 計算ブロック（`FUN_005479D0(4 attr)→FUN_004672B0(&actor[+0x24])` 経路）の解釈。＋「skill+10 は本系統に不在 [推定高]」の最終裏取り（残2 update caller `0x4B585B`/`FUN_0066A3D0` の型付け）。
2. 【低】chargen 帯の未注記 fn（head 0x596980 実体の長い側・0x596A50-0x596BC0）の identity と `FUN_00579C10` callback（`0x5974E0`）の意味。
3. 【低】`FUN_004FDE30`（opcode 第2 fn）と opcode flags `0x20000`/container `0xB0A900`（v409 から継続）。

## §4 検証・自己批評
- probe: **v410_1..v410_5 の5本全 EXIT=0**。
- **決定打の作り**: (i) 書手4者の **data refs census（vtable 逆方向の否定）** → (ii) decompiled 直接 form の virtual disp 棄却 → (iii) **E8 全数で呼出し元を確定**（setter/update/apply の3段）→ (iv) **call 境界の find_head 誤りを decompiled body 突合で発見・所属訂正** → (v) chargen fn の machine 補完。
- 妥当性チェック: ①`FUN_00668B30` machine（setter→threshold→update→再保留）⇔decompiled5引数 — ②供給 funnel（唯一供給元 dead）⇔保留適用 callers=2 — ③`FUN_006684B0`＝file write 系（virtual+0x2C write・`[+0x658..+0x6C7]` ダンプ）⇔setter 呼び出し不在 — **三方向矛盾なし**。
- docs との不整合: **v406 ruling §1/§2 の「`FUN_0065FA40` の2番目の caller ＝ `FUN_006684B0`」および v409 ruling の同種記述は本裁定で訂正（正＝`FUN_00668B30`）** — docs/GT 反映時に #47/#20 一括で併せて反映。
- **教訓**:
  1. **`find_head` は直前の int3/境界を飛び越える場合がある → caller の所属は decompiled body 突合（または annotation range）で必ず再確認**（本件 v406/v409 の2 ruling が誤記を引きずった）。
  2. **書手の網羅は「E8＋data refs＋decompiled 直接 form」の3系統**（vtable 書手・virtual 書手・直書きを同時に否定できる）。
  3. **供給 funnel の証明**（供給元 dead → 適用も0）は「唯一供給元の全系統否定」が揃って初めて [推定高] にできる。
