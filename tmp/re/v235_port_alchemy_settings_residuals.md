# v235_port_alchemy_settings_residuals.md — 移植設定・alchemy 残点束（3件）

- 作成: 2026-11-01 / 担当: TODO serial 228（V228・中）「移植設定・alchemy 残点束」
- 出典本体: `tmp/re/v233_relationship_residuals2.md` §8 新規残点 N-1..3
- 手法: 移植ソース読取のみ（`app/src/main/cpp/` 編集なし）＋ EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析）＋ 既存 RE 成果物（v208/v216/v218/v221/v227/v233・cache）の突合 ＋ UESP/CS Wiki（読取のみ）。ESM 本体は未アクセス（再スキャン禁止を遵守）。
- 結論早見: 項目1 **(a) 代入経路なしを再確認（解明）／(b) MGEF DATA 読出＝`FUN_00416100`（64B→obj+0x58・flags 位置確定）と、効果分類＝mgefCode(fourcc) 直書き比較（`FUN_0041B9A0` 等）を実測で解明／(c) 修正ドラフト提示** ／ 項目2 **現時点で kEntries から fGlobalTimeMultiplier 消失＝削除案が既適用とみられる（v233 読取時 41 件→現 40 件。経緯・影響範囲を整理）** ／ 項目3 **意図的二重登録と裁定（エンジンノード実名 'fQuestScriptDelayTime:MAIN' を実測、v218 の「':' 無し」記述と不一致）**
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## §1 項目1: MagicEffectData.effectType 未代入問題（N-1・移植バグ級）

### §1-1 (a) 代入経路が本当に無いことの再確認 [解明]

| 検査 | 結果 | 出典 |
|---|---|---|
| `effectType` 全 grep（`app/src/main/cpp` 全域） | ヒット **8 件のみ**: esm_reader.h:385（SpellData 用・宣言）/ :432（MagicEffectData 用・宣言）/ alchemy_system.cpp:88（コメント）/ :96（**読出のみ**）/ memory_pool.h:212,218（別構造体）/ tests/phase45_unit_tests.cpp:899,902（別構造体） | grep 実測 |
| MagicEffectData の生成点 | `ESMFile::decodeMagicEffect`（esm_reader.cpp:1243-1272）の**1 箇所のみ**。代入は formID / editorID / fullName / description / baseCost / actorValue↔linkedFormID / school / flags の 8 フィールド。**effectType への代入 0 件** | 読取 |
| decodeSpell の SPIT 読出が代入経路になるか | **ならない**: SPIT は `SpellData.spellType`（esm_reader.cpp:1157 `spell.spellType = ...`）に格納。MagicEffectData とは別構造体。SPIT にも要素分類情報は無い（+0=type, +4=cost, +8=level, +12 未初期化） | esm_reader.cpp:1153-1162 |
| デフォルト値 | `uint32_t effectType = 0;`（esm_reader.h:432）＝**常に 0** | 読取 |
| 消費点 | alchemy_system.cpp:96 のみ。`mgef->effectType == 4/1/2/3` は**常に false** → fire/frost/shock/drain 素材のポーションは else 分岐（:101 `healAmount +=`）へ落ちる。school==5（Restoration）分岐（:93）と unknown 分岐（:105）のみ機能 | alchemy_system.cpp:86-106 |

**判定: v233 N-1 の「代入 0 件＝常に 0」は裏取り成立。(a) は解明。**

### §1-2 (b) エンジン側（EXE）での MGEF DATA 読出と効果分類 [解明・本タスクの実測]

**b-1. MGEF レコードロード関数を特定: `FUN_00416100`（0x416100-0x416360）**

capstone 再逆アセンブル（本タスク実施）:

- 入口: `mov edi,[ebp+8]`（arg = TESFile*）/ `mov ebx,ecx`（this = EffectSetting*）→ `call 0x4511C0; cmp eax,0xC; je ...` = **レコード formType 0x0C（=EffectSetting/MGEF、v227 GT 確定値と一致）検証** → `push ebx; call 0x451210`（TESFile::ReadRecord）→ サブレコードループ（`call 0x4510B0` で次タグ取得 → `cmp eax,imm32` の二分岐木: DATA / MODB / DESC / ESCE / EDID）。
- **DATA 分岐（0x41617B-0x416229）逐行実測**:
  1. 事前保存: `mov edi,[ebx+0x58]`（旧 flags）/ `mov al,[esi+3]`（旧 flags 上位 byte → bit24）/ `mov eax,[ebx+0x60]`（旧 AV）
  2. `lea esi,[ebx+0x58]` → `push 0x40 / push 0 / push esi / call 0x981630` = **memset(obj+0x58, 0, 0x40)**
  3. `push 0x40 / push esi / mov ecx,[ebp+8] / call 0x450C20` = **TESFile::ReadData(obj+0x58, 0x40) ＝ MGEF DATA(64B) を obj+0x58..+0x97 へ生コピー**（EDID 分岐が同一ヘルパで文字列を読むことから「現在のサブレコード payload 先頭から」であることを確認）
  4. 直後の処理が **[esi]=DATA 先頭 dword を flags として扱う**: フラグ表 0xB03418/0xB0341C（mask,enable ペア群）＋ 0xB034C4 群で旧値(edi)と新 DATA[0] をマージ（`test [esi],ecx / or edi,ecx / not ecx / and edi,ecx`）→ `mov [esi],edi` → 旧 bit24 保持（`or edi,0x1000000`）→ **`and dword [esi], 0xFFDFFFFF`（bit21 = kMgefFlag_PCHasEffect をロード時にクリア）**。
  5. bit21 クリアは v193b「Player LoadGame set / FUN_004167E0 clear」・v216 の PCHasEffect 説明と**整合**＝[obj+0x58] が mgefFlags である独立証拠。DATA[0] を使わない限りこのマージは成立しないため **DATA+0x00 = Flags dword が確定**。
- **EDID 分岐（0x41624F-0x416285）**: `push 0x200 / push esi / call 0x450C20`（文字列読出）→ `mov edx,[ebx]; mov eax,[edx+0xD8]; push esi; mov ecx,ebx; call eax` = **vtable+0xD8 の仮想関数（SetEditorID 系）へ EDID を渡す**＝mgefCode（editorID 先頭4文字）は EDID から導出。

**b-2. EffectSetting 内 DATA ブロックの field 対応（ctor `FUN_00416430` ＋ runtime 生成 `FUN_00417220` の引数→フィールド写像で確定）**

runtime 生成側の実測（0x417381-0x4173D5・cdecl 11 引数）:

| obj オフセット | DATA オフセット | runtime 引数（実測） | 既定/実例 | 判定 |
|---|---|---|---|---|
| **+0x58** | **+0x00** | arg6（flags リテラル、例 FIDG=`0x21000475`） | ctor 既定 0 | **mgefFlags（確定）**。port は **未読取**（"unknown0" として破棄） |
| +0x5C | +0x04 | arg4（float・`fstp`）＝baseCost | ctor 既定 0.0f | **baseCost ＝ port と一致** |
| +0x60 | +0x08 | arg5（FIDG/SHDG/FRDG すべて **8 = Health**） | ctor 既定 0 | **related actorValue ＝ port と一致**（リンク先 formID は同スロット＝v216/CEF と整合） |
| +0x64 | +0x0C | arg3（FIDG/SHDG/FRDG すべて **2 = Destruction** school） | ctor 既定 **6**（0..5 域外＝"no school" センチネル） | **school ＝ port と一致**（ポジションは正しい） |
| +0x68 | +0x10 | arg7（FIDG=**61 / FRDG=62 / SHDG=68**、ABAT/FOAT/REAT 群=64） | ctor 既定 **0x48=72**（AV 範囲 0..71 域外＝センチネル） | **Resist 系 actorValue**（CS Wiki AV 表: 61=ResistFire / 62=ResistFrost / 68=ResistShock / 64=ResistMagic）。port はこれを **"flags" と誤ラベル**（消費者ゼロのため実害なし） |
| +0x6C..+0x97 | +0x14..+0x3F | arg8(=関連効果 count)＋stack 上の配列 arg9..11 → `call 0x415D40`（FIDG: count=3, {DSPL, RSFI, FISH}＝Dispel / Resist Fire / Fire Shield＝CS の related effects） | 他は未使用疑い | 関連効果配列＋未解明 36B |
| **+0x98** | （DATA 外） | **mgefCode**: runtime で `mov [esi+0x98], edi`（edi=arg1 fourcc）、ctor 既定 `-1` | 末尾 `mov ecx,0xB33508; call 0x452570`＝EffectSettingCollection 登録（v227 と一致） | **mgefCode は EDID 由来**（+0x98 ＝ DATA ブロック直後） |

→ **帰結**: port の `school(+0x0C)` / `baseCost(+0x04)` / `av(+0x08)` の**位置は正しい**。誤りは flags のみ（真の flags は +0x00（port 未読取）、port の "flags(+0x10)" は Resist AV）。v233 §8 N-1 の前提だった「flags は DATA+0x10」という port コメント（esm_reader.cpp:1251-1252 の struct 注記）は **EXE 実測と不一致**（範囲は flags 点のみに縮小して残点 N-4 として記録）。

**b-3. 効果分類（fire/frost/shock/drain/absorb/disintegrate）は DATA 由来ではなく mgefCode(fourcc) 直書き比較**

`%TEMP%\opencode\v235_fourcc_funcs.py`（出力 `v235_fourcc_funcs_out.txt`）: .text 全域に 45 コード（port 41 ＋ FOAT/FOSK/DIAT/DISK）の forward/reverse バイト列を全数走査 → **157 箇所・32 関数**（reverse 形のヒットは 0＝エンジンは常に LE 4 文字順で比較）。

| 関数 | ヒット | 内容（本タスクで復元） | 意味 |
|---|---|---|---|
| **`FUN_0041B9A0`** | 3 | `eax = [esp+4]`（=mgefCode）→ `cmp`木: **FIDG→1 / FRDG→2 / SHDG→4 / その他→0** | **要素分類器そのもの**（＝port `effectType` の engine 版）。戻り値は fire=1 / frost=2 / **shock=4**（port は shock=3 ＝enum 定義が異なる）。呼び出しは 1 箇所のみ `FUN_005EE510`（call@0x5EE57D） |
| **`FUN_0041B950`** ※ | 6 | **FISH(Fire Shield)→1 / FRSH(Frost Shield)→2 / LISH(Shock Shield)→4 / SHLD(Shield)→8 / REDG(Reflect Damage)→8 / RSNW(Resist Normal Weapons)→8** | シールド/反射/耐性系の要素ビット分類（1/2/4/8 ＝ビットマスク）。呼び出しは `FUN_005EE510` 1 箇所（同関数で 0x41B9A0 と連続呼び出し＝「ダメージ要素」と「シールド要素」を並列に解決）。※この6コードは fourcc スキャン対象 45 コード外＝本タスクのディスアセンブラ（0x41B950）で個別特定した点数（157 点には含まない） |
| `FUN_00413A90` | 7 | `mov eax,[ecx]`（EffectItem+0x00 = mgefCode、CEF 布局と一致）→ 二分木で **ABAT/DRAT/FOAT/DGAT / ABSK/DRSK/FOSK / REAT** を判定 → 各先端で TESGlobal 系 0xB334C0 / 0xB334C8 / 0xB334E0 を使い分け | **actorValue ハブ（FOAT/DRAT/ABAT/DGAT/FOSK/DRSK/ABSK/REAT）の AV 解決**。呼び出し 4 節（0x413CDE / 0x41CFA1 / 0x594BA4 / 0x5B336A） |
| `FUN_00417420` | 89 | `push imm32(=fourcc) … call 0x417220` の既定効果生成列（FIDG/SHDG/FRDG/全 DG*/DR*/AB*/FO*/RE*/WK*/DI*/CUDI/CUPO/CUPA/POSN/DISE/VAMP/STMA/SUDG/PARA/INVI/FOMM…）＋ school・baseCost・flags・**Resist AV**・related-effects 配列のリテラル | **既定 EffectSetting レジストリ**。分類語彙＝mgefCode の直書きで確定 |
| `FUN_00419B30` | 2 | 代表選択: type==5 → `push 0x4E534F50`（**POSN**）、type==1 → `push 0x45534944`（**DISE**）→ `call 0x416870`（mgefCode キー lookup）。中間で `mov edx,[esi+0x1c]; mov edx,[edx+0x58]; shr edx,0x16; test dl,1`＝**bit22(kMgefFlag_Disabled) チェック**、末尾 `cmp [esi],0x46464553`＝**"SEFF"（Script Effect）** | **v216/v230 の type1→DISE・type5→POSN・bit22 チェック・CEF EffectItem(+0x1C)/mgefFlags(+0x58) 布局を、独立コードで再三重確認**（v216 の 0x414EB0 呼び出しもこの関数内に存在） |
| 残り 28 関数 | 56 | INVI 専用述語 9 関数（各 1 点）/ `FUN_00459FA0`・`FUN_0045A020`・`FUN_005239C0`(2)・`FUN_006A27F0`（VAMP 計 5）/ `FUN_0041D4A0`・`FUN_00603320`(2)（REHE 計 3）/ `FUN_0041D590`・`FUN_00613BB0`（PARA 計 2）/ `FUN_00616BA0`(CUDI/CUPO 2) / `FUN_0066F420`・`FUN_006701B0`（SUDG 計 2）/ `FUN_006A0800`・`FUN_006A0D90`(2)（DIAR/DIWE 計 3）/ `FUN_006126B0`(Fortify 系 7) / `FUN_00613880`(Restore/Fortify 系 6) / `FUN_0068EA50`(Absorb/Cure/Para 9) / `FUN_00695400`(FIDG 1) / `FUN_006A8220`(ABHE 1) / `FUN_009FEA50`(6) | **個別効果の固有挙動は「mgefCode 直書き if」で実装**されている実態。内訳 9+5+3+2+2+2+3+7+6+9+1+1+6 = **56**、既知4関数 101 と合わせ **157 で全数一致**（関数数 28+4 = 32 も一致） |

（fourcc の同一性は UESP/CS Wiki **Magic Effects List**（oldid=40309・v233 と同一版・本タスク取得）と 45 コード全数突合。冒頭の "FISH/FRSH/LISH/SHLD/REDG/RSNW/DSPL/RSFI/REAT/SEFF" 等の不明だった即応値もすべて正式名で解決: Fire Shield / Frost Shield / Shock Shield / Shield / Reflect Damage / Resist Normal Weapons / Dispel / Resist Fire / Restore Attribute / Script Effect）。

**逆に、DATA(64B) に「0=other/1=fire/…/6=disintegrate」の分類列は存在しない**（b-2 の写像で DATA は flags/baseCost/av/school/ResistAV/related-effects/未解明 で埋まっている）。＝**port の `effectType(0..6)` は ESM 由来の列ではなく、editorID fourcc から導出するべき派生値**であり、v233 N-1 の修正方針（editorID fourcc からの分類充填）と EXE 実測が一致する。

**b-4. engine の「effect 系属性」読出まとめ（対応表）**

| 属性 | エンジン側読出 | 移植側対応 |
|---|---|---|
| mgefCode | EDID → vtable+0xD8（SetEditorID）で導出 → **obj+0x98**、EffectSettingCollection 0xB33508 のキー | `MagicEffectData.editorID`（読出済み。消費は mgefToEffectType のみ） |
| flags | **DATA+0x00 → obj+0x58**（ロード時 bit21 クローズ・OBME 表マージ、本タスク実測） | port は DATA+0x00 を unknown0 として**破棄**。同 +0x10 を "flags" と読むが実体は Resist AV（消費者ゼロ） |
| baseCost | DATA+0x04 → obj+0x5C | **一致** |
| related AV / link | DATA+0x08 → obj+0x60（FIDG=8 Health） | **一致** |
| school | DATA+0x0C → obj+0x64（FIDG/SHDG/FRDG=2 Destruction、ctor 既定 6） | **一致** |
| Resist AV | DATA+0x10 → obj+0x68（61/62/68/64、ctor 既定 72） | 誤ラベル"flags"（未消費） |
| 要素分類 | **DATA に列なし**。mgefCode 直書き比較（`FUN_0041B9A0` fire=1/frost=2/shock=4、`FUN_0041B950` シールド系、他 30 関数） | **未実装（＝N-1 の本体）** |

### §1-3 (c) 修正ドラフト（[PORT] 承認制・本タスクではコード未変更）

**方針**: engine 同様、分類は editorID fourcc から導出する（DATA に分類列は存在しない＝b-3）。既存の `mgefToEffectType()`（spell_manager.cpp:30-70、kDamageCodes 41 件照合済み＝v233 §2-2）と重複しないよう、**ESM デコード時に effectType を埋める**方式を採る（alchemy_system 以外の将来消費者にも効く）。

**Before（現状・読取のみ確認）**:

```cpp
// esm_reader.cpp (ESMFile::decodeMagicEffect, L1243-1272)
void ESMFile::decodeMagicEffect(const ESMRecord& rec) {
    MagicEffectData effect;
    effect.formID = rec.formID;
    effect.editorID = rec.getString("EDID");
    effect.fullName = rec.getString("FULL");
    effect.description = rec.getString("DESC");
    auto* medt = rec.findSubRecord("DATA");
    if (medt && medt->size() >= 20) {
        effect.baseCost = readFloatOrInt(medt->data.data() + 4);
        uint32_t av = readU32(medt->data.data() + 8);
        if (av <= 200) { effect.actorValue = av; } else { effect.linkedFormID = av; }
        effect.school = readU32(medt->data.data() + 12);
        effect.flags = readU32(medt->data.data() + 16);   // 実体は Resist AV（b-2 実測）
    }
    m_magicEffects.push_back(std::move(effect));          // effectType は 0 のまま
}

// game/alchemy_system.cpp L93-102（消費点）
if (mgef->school == 5) {
    potion->healAmount += static_cast<int32_t>(magnitude);
} else if (mgef->effectType == 4 || mgef->effectType == 1 ||
           mgef->effectType == 2 || mgef->effectType == 3) {
    potion->stats.damage += static_cast<int>(magnitude);   // 死にコード
} else {
    potion->healAmount += static_cast<int32_t>(magnitude); // 全 damage 系がここに落ちる
}
```

**After（ドラフト A: decodeMagicEffect で fourcc から充填・推奨）**:

```cpp
// esm_reader.h — MagicEffectData.effectType のコメントを仕様として確定
//   0=other, 1=fire, 2=frost, 3=shock, 4=drain, 5=absorb, 6=disintegrate
//   （ESM 由来の列ではなく editorID fourcc から導出する派生値。
//     エンジンは DATA に分類列を持たず mgefCode 直書き比較で分類する
//     （FUN_0041B9A0: FIDG->1/FRDG->2/SHDG->4、v235 §1-2）。
//     エンジンの要素値は fire=1/frost=2/shock=4 で本 enum(shock=3) とは
//     数え方が異なるため、本 enum は移植側仕様として独立に保つ。）

// esm_reader.cpp — decodeMagicEffect 内、rec.getString("DESC") の後に追加
// （rec から editorID を取った後でも可。lambdas 不使用・C++17）
static uint32_t deriveEffectType(const std::string& id) {
    if (id.size() != 4) return 0;
    struct Pair { const char* code; uint32_t type; };
    static const Pair kMap[] = {
        {"FIDG", 1}, {"FRDG", 2}, {"SHDG", 3},            // fire / frost / shock
        {"WKFI", 1}, {"WKFR", 2}, {"WKSH", 3},            // weakness も要素で分岐するか = [PORT]判断
        {"DGHE", 4}, {"DGFA", 4}, {"DGSP", 4}, {"DGAT", 4},
        {"DRHE", 4}, {"DRFA", 4}, {"DRSP", 4}, {"DRAT", 4}, {"DRSK", 4},
        {"DIAR", 6}, {"DIWE", 6}, {"SUDG", 4},
        {"ABHE", 5}, {"ABFA", 5}, {"ABSP", 5}, {"ABAT", 5}, {"ABSK", 5},
    };
    for (const Pair& p : kMap) { if (id == p.code) return p.type; }
    return 0;   // other（VAMP/STMA/POSN/FOHE 等は 0＝healAmount 側へ）
}
// decodeMagicEffect 内:
//   effect.effectType = deriveEffectType(effect.editorID);
```

※ 分類**境界**はドラフト案。弱点化(WK*)・POSN/VAMP/STMA・CUDI/CUPO/CUPA・FOHE 等をどの type に落とすかは **[PORT] 判断**（engine 自体が 0..6 の分類列を持たないため、移植 alchemy の望ましい挙動で決める＝v233 §2-2 の 41 コード表がそのまま判断材料）。

**After（ドラフト B: 消費点側で分類・最小変更）**:

```cpp
// game/alchemy_system.cpp — mgef 取得直後（L87 の if (mgef) 内）に分類を導出し
// effectType を使わず school + editorID で判定する
} else if (mgef->school == 5) {
    potion->healAmount += static_cast<int32_t>(magnitude);
} else if (isDamageEffect(mgef->editorID)) {     // 新規 static ヘルパ（fourcc 判定）
    potion->stats.damage += static_cast<int>(magnitude);
} else {
    potion->healAmount += static_cast<int32_t>(magnitude);
}
```

**（参考・別ドラフト C: DATA flags の正しい位置で読む）** esm_reader.cpp の `effect.flags = readU32(data+16)` を `readU32(data+0)` に変更し、+0x10 分を `resistActorValue` 新設へ移す変更案。**ただし `MagicEffectData.flags` の消費者は grep ゼロ**（本タスク）なので、N-1 の修正とセットでなくとも実害なし。[PORT] 判断。

**影響範囲（grep 実測）**: `effectType` の消費者は alchemy_system.cpp:96 の 1 箇所のみ。SpellData.effectType（esm_reader.h:385・legacy）は**別フィールドで読出・消費ともにゼロ**（v233 §6 の spellType と同じ扱い）。テスト: tests/ に ALCH/alchemy の effectType 挜動テストは grep ゼロ（phase45 の hit は別構造体）。→ 変更リスクは alchemy ポーション生成の damage/heal 振り分け**のみ**＝低。

**残す判断（[PORT]）**: (i) A 案（decode 時充填・将来消費者に有利）か B 案（消費点のみ）か、(ii) カテゴリ境界（WK* / POSN / VAMP / STMA / CUDI・CUPO・CUPA / Fortify 群）、(iii) `healAmount` フォールバック側の school 判定の継続可否、(iv) ドラフト C（flags 位置訂正）の要不要。

---

## §2 項目2: kEntries の fGlobalTimeMultiplier 混入（N-2）

**判定: 混入の経緯と影響範囲は解明。ただし現時点のソースでは kEntries に該当行が存在しない＝v233 読取時（41 件）から 40 件へ減少しており、**削除案が既に適用されたとみられる**（実行主体は git 不使用のため未確認）。以下は判断材料と現状事実。**

### §2-1 現状事実（本タスク grep・読取）

| 検査 | 結果 | 出典 |
|---|---|---|
| `fGlobalTimeMultiplier` 全 grep（`app/src/main/cpp`） | **2 件のみ**: jni_bridge.cpp:254（コメント内・RE 54.3 説明）／ oblivion_ini_defaults.cpp:444（{"General", ..., "1.0000", kFloat, 0x03}＝INI 既定表の登録行） | grep 実測 |
| kEntries 全数 | **40 件**（fatigue 16 + combat/block/knockback/perk 22 + speak 2）。v233 §1-2 の「41 件（:60 に fGlobalTimeMultiplier）」から **-1**。現在の 60 行目は `fArmorRatingMax` | gmst_defaults.h:40-86 |
| getLive 消費 | `gmst::getLive("fGlobalTimeMultiplier")` は**存在しない**（grep 全 **54** ヒットの内訳: combat_formulas.h 22 / fatigue_system.h 16 / combat_formulas_tests.cpp 10 / gmst_defaults.h 4・コメント / asset_manager.cpp 1・コメント / jni_bridge.cpp 1・コメント。実消費 38 は fBlock*/fPerk*/fKnock*/fCombatSpeak*/fFatigue* のみ）。jni_bridge.cpp:254-259 のコメントが「The former frame-dt multiply via gmst::getLive() was removed accordingly.」と明記 | grep 実測（再計測） |
| settings_manager.h | v233 が引用した `settings_manager.h:39-44 / 46-51` の fGlobalTimeMultiplier NOTE は**現在存在しない**（現 39-44 は fAIMinGreetingDistance、46-51 は perk MULTIPLIER の doc）。v233 P0 の行番号（:54-61 / :64-70 / :72-79）は現物（:36-44 / :46-53 / :55-62）と **+18 行ずれ**＝NOTE ブロック相当の削除があったことを示す | 読取＋v233 突合 |
| ファイル改定時刻 | gmst_defaults.h / jni_bridge.cpp / esm_reader.h / oblivion_ini_defaults.h / combat_formulas_tests.cpp = **2026-11-01 20:51:24（同一秒）**、settings_manager.h = **20:52:22**。v233 本体の保存は 20:52:39 → **v233 の読取後に 6 ファイルが同一帯で編集**されたことを示す（git 不使用のため改定履歴・実行主体は未確認） | Get-Item 実測 |

### §2-2 混入の経緯（突合）

1. **s50 由来ではない**: kEntries のヘッダは "s50_gmst_full.txt subset"（gmst_defaults.h:38）だが、v208 は `fGlobalTimeMultiplier:General` が **GameSetting-map GMST 非登録（0xB35574 に無し）＝INI レジストリノード専用**（node 0xB06704 / 文字列 0xA3D150、静的値 1.0f）であることを確定済み（v208 L12/L81-82/L92）。v233 §1-2 でも engine float 登録 992 件に同名なしを再確認。本タスクでも EXE 実測で node 0xB06704 の name = `'fGlobalTimeMultiplier:General'` を確認（§3-3 の同一スクリプト）。
2. **誤分類の起点（推論）**: RE_EXE_REPORT.md §43 / rel6 §6 が本キーを「GMST」と誤記していたこと（v208 L92 の訂正表）が、移植側の kEntries 追加（＝GMST 扱い）に流入したと考えられる（改定履歴未確認のため時系列は未証）。
3. **結果の矛盾**: kEntries に混入しても `publishGmstOverrides`（asset_manager.cpp:134-146）は `esm.findGameSetting(key)` 必経＝ESM に GMST が存在しないため**恒常未命中 → setOverride 不発 → getLive が恒常 1.0**（v233 §3-1②）。＝混入は「登録だけして効かない」状態だった。

### §2-3 削除案 vs 例外化案の影響範囲（[PORT] 判断材料）

| 案 | 内容 | 影響範囲（grep 全数） | エンジン整合 |
|---|---|---|---|
| **削除案（＝現状がこれと一致）** | kEntries から当該行を削除＋frame-dt 乗算の削除 | ① kEntries 40 件（kEntryCount 連動で g_overrides[40] も縮小、ビルド整合）② publishGmstOverrides の走査対象から自動除外 ③ jni_bridge は deltaTime 恒等（RE54.3 と一致）④ テストは fGlobalTimeMultiplier 参照ゼロ（combat_formulas_tests は fBlockMax 中心） | **一致**（エンジンは INI ノード 0xB06704 をゲーム時計 FUN_0047D170 にのみ折り込む。frame-dt 倍率はエンジン非在） |
| **例外化案（混入を維持＋INI 専用としてマーク）** | kEntries に残しつつ、publishGmstOverrides で対象外にする等の注釈 | ① getLive 消費者が既にゼロ＝**維持しても参照ゼロ** ② 例外分岐を asset_manager に追加するコード増 ③ 破壊条件（同名 GMST を持つプラグインを積む → setOverride 発火 → frame dt 倍率化）は**残存** | 不一致（破壊条件が残る） |
| （参考）INI 経路 | oblivion_ini_defaults.cpp:444 の {"General", fGlobalTimeMultiplier, "1.0000", 0x03} は**消費者ゼロ**（resolve 経由の読出なし）＝ユーザー設定は依然死データ（v233 §3-1③を維持）。これは kEntries とは別の残点 | — | エンジンは INI ノードとして実装＝**INI 行の削除は不要** |

**裁定材料**: 削除案は (i) エンジン整合、(ii) 破壊条件の消滅、(iii) 既存 grep で参照ゼロ、の3点で有利。例外化案の唯一の意義は「将来 getLive 消費を戻す場合」だが、それ自体が RE54.3 違反になるため**実質的な選択肢は削除案**。現時点のソースは既に削除案と一致している（§2-1）。**[PORT] として残るのは「既適用が正しい差分だったかの承認」と「INI 経路（resolve）未接続という別残点の扱い」のみ。**

---

## §3 項目3: kDefaults の fQuestScriptDelayTime 二重登録（N-3）

**判定: 解明。二重登録は「生成器が (section, key) を結合キーとするマージの必然」＝意図的（事実として正しい2行）であり重複事故ではない。engine 側セクション帰属は本タスクの EXE 実測で「MAIN」に確定（ただし v218 の機構説明「':' 無し」は実測と不一致＝doc 訂正要）。消費者ゼロのため実害なし。**

### §3-1 現物（読取）

```cpp
// system/oblivion_ini_defaults.cpp（AUTO-GENERATED・L444-445, L633）
{"General", "fGlobalTimeMultiplier", "1.0000", IniValueType::kFloat, 0x03},
{"General", "fQuestScriptDelayTime", "5.0000", IniValueType::kFloat, 0x01},  // ← L445 file 側
...
{"MAIN",    "fQuestScriptDelayTime", "5",      IniValueType::kFloat, 0x02},  // ← L633 engine 側
```

- provenance: **0x01 = kFromFile のみ / 0x02 = kFromEngine のみ / 0x03 = kFromBoth**（oblivion_ini_defaults.h:32-36）。
- L445 は file のみ・L633 は engine のみ＝**同一 (section,key) ではなく、2 出典が別セクションに持つ同一 bare キー**。

### §3-2 生成器のマージ規則（意図性の決定的証拠）

`tmp/re/gen_oblivion_ini_defaults.py`（読取）:

- L110-113: engine 行のセクション = `name.partition(":")` の第3成分（':' 無しなら `""`）
- **L226 / L245: `key = (r["section"].lower(), r["key"].lower())` ＝結合キーは (section, key)**
- L253-255: 同一 (section,key) で file が二重の場合のみ dropped_dupes に計上（該当は `[General] STestFile1` の 1 件のみ・manifest L30-31）
- L271-274: 値は「file があれば file 値、なければ engine 値」

→ したがって **セクション違いの同名キーは必ず 2 行になる**（`fBlurRadius` が BlurShader / BlurShaderHDR / BlurShaderHDRInterior の 3 行 all 0x03 と共通パターン）。v233 §5-2 でも**セクション跨ぎ重複は 15 キー**存在する実勢を確認済み＝本キーは特別ではない。

### §3-3 エンジン側の実セクション帰属（EXE 実測・本タスク）

`%TEMP%\opencode\v235_node_check.py`（PE 手動解析 + .data/.rdata リード）:

- node **0x00B09E28**: bits = `0x40A00000`（**5.0f**）、name pointer (node+4) = 0x00A4A07C → 文字列リテラル = **`'fQuestScriptDelayTime:MAIN'`**
- （対照）node 0x00B06704 の name = `'fGlobalTimeMultiplier:General'`（v208 の記載と完全一致）
- → **エンジンの登録ノード名は ':' を明示して "MAIN" を持つ**。値も file 側 "5.0000" と同値 5.0f。

**突合結果**:

| 記述 | 内容 | 実測との関係 |
|---|---|---|
| v218 §5 L261-264 | 「エンジン側 name = `fQuestScriptDelayTime`（**':' 無し** → ReadOne 分割ヘルパ 0x4A8430 の既定セクション MAIN(0xA404C4) 扱い）」 | **機構の記述は不一致**: 実名は ':MAIN' 明示で、':' 無し→MAIN の既定化は本キーに適用されない（帰属結果 MAIN は一致） |
| v218 §5 crosscheck2 アーティファクト | only-in-A(MAIN) と only-in-B([General]) に同時出現＝キー結合のアーティファクト | **維持**（A=engine コンテナ側は ':MAIN' 由来で MAIN、B=ファイルは [General] 配置） |
| 生成器 partition 処理 | ':' 無しなら section="" | 本ノードは ':' あり → "MAIN" ＝表の L633 と整合。manifest の "section-less engine nodes" は 3 件のみ（iLastHDRSetting / iDetectionHighNumPicks / fLowPerfCombatantVoiceDistance）で本キーは含まない＝**生成器側は一貫** |
| ファイル側 | `Oblivion_default.ini:59` に `fQuestScriptDelayTime=5.0000`（[General] 配置、L1 が [General]。本タスクで実ファイル読取） | L445 と整合 |

### §3-4 意図性の裁定

1. **意図的（生成器の設計どおり）**: 結合キーが (section,key) である以上、2 出典のセクション帰属が異なるキーは必然的に二重登録となる。provenance が 0x01 / 0x02 に分かれていること、同型の跨セクション重複が他 14 キーにも存在すること、dropped_dupes に計上されていないことが根拠。
2. **MAIN 行は「':' 無し正規化（MAIN 正規化）前提の表記」ではなく、エンジンノード実名に忠実な転記**: v217(3)/v230 §4-2 の「':' 無し→MAIN 正規化」は ReadOne の一般則として別途成立しうるが、**本キーはそもそも ':MAIN' なのでその適用を受けない**。→ 「MAIN=':'無し正規化を前提とした意図的二重」ではなく「**エンジン実名 ':MAIN' とファイル配置 [General] の差をそのまま表に出した結果**」である。
3. **実害**: 消費者ゼロ（`fQuestScriptDelayTime` は oblivion_ini_defaults.cpp 内の 2 行のみ、他の grep 0 件）。`find(section,key)`（バイナリ探索）は両行へ個別到達可能、`find_by_key`（線形・最初の一致＝v233 §5）は並び順で General 行（"5.0000"）を返すが、値は同値（5.0f）で型も両方 kFloat＝**結果差なし**。
4. **doc 訂正要（v218）**: v218 §5 L262 の「':' 無し」は EXE 実測と矛盾。v235 以降の再参照時は「エンジン実名 'fQuestScriptDelayTime:MAIN'」に置き換えるべき（crosscheck2 アーティファクト＝セクション帰属ずれという結論自体は正しい）。

---

## §4 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: image_base 0x400000 / .text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00 / .tls / .rsrc / .bind ほか）。
- capstone 5.0.7（skipdata）。本タスクの再逆アセンブル対象: **0x44E330-0x44E410（MGEF ディスパッチ帯: EDID 判別→new(0xA8)→ctor 0x416430→ReadRecord 0x451210→0x446CB0 追加）** / **0x451210-0x451330（TESFile::ReadRecord）** / **0x46B6C0-0x46B7C0** / **0x416430-0x416520（EffectSetting ctor＝field 既定値）** / **0x416100-0x416280（EffectSetting レコードロード＝DATA 64B→+0x58・bit21 クリアの実測点）** / **0x417220-0x417420（runtime 生成＝引数→field 写像・mgefCode=+0x98・0xB33508 登録の実測点）** / **0x417560-0x417660・0x413A40-0x413B50（fourcc 直書き比較帯）** / **0x41B950-0x41B9A0・0x41B9A0-0x41BA50（要素分類器）** / **0x419B30-0x419C60（代表選択 DISE/POSN・bit22・SEFF）**。
- selftest: 着手時に `probe_harness.py selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）。
- スクリプト（新規のみ・%TEMP%opencode/）: `v235_node_check.py`（INI ノード実名・値読出） / `v235_disasm.py`（汎用範囲ディスアセンブラ） / `v235_mgef_scan.py`（＋`_out.txt`・DATA タグ/fourcc 全走査） / `v235_data_push.py`（＋`_out.txt`・push DATA 全8点） / `v235_data_all.py`（＋`_out.txt`・DATA バイト全51点） / `v235_fourcc_funcs.py`（＋`_out.txt`・fourcc 157 点→32 関数） / `v235_callers.py`（E8 呼出し先スキャン）。
- ESM 本体: **未アクセス**（再スキャン禁止を遵守）。ファイル読取: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion_default.ini`（[General] L59 / [MAIN] L285-286 / fGlobalTimeMultiplier L34 の確認）。
- 移植ソース: 読取のみ（esm_reader.h / esm_reader.cpp / alchemy_system.cpp / gmst_defaults.h / asset_manager.cpp / jni_bridge.cpp / settings_manager.h / oblivion_ini_defaults.h / oblivion_ini_defaults.cpp / oblivion_ini.cpp / spell_manager.cpp / combat_formulas.h / fatigue_system.h、いずれも編集なし）。
- RE 成果物（読取のみ）: v233 本体（§1/§3/§5/§8） / v218_ini_docs_bundle3.md（§5 crosscheck2・fQuestScriptDelayTime） / v208_gmst_mixed_names.md（fGlobalTimeMultiplier は INI ノード） / v216（EffectSetting +0x58 = mgefFlags・CEF・bit テーブル） / v221 / v227（MGEF ディスパッチ・formType 0x0C・runtime 登録） / v229（変換橋） / cache/named_funcs.txt・cache/vtable_index.tsv（EffectSetting vtable 0xA32B14/55 slots 等）・cache/README.md / tmp/re/gen_oblivion_ini_defaults.py / tmp/re/oblivion_ini_defaults_manifest.txt。
- web 資料（読取のみ）: **cs.uesp.net/wiki/Magic_Effects_List（oldid=40309・取得成功・v233 と同一版）** ＝ 45 コード全数突合の根拠（FISH=Fire Shield 等の即応値解決） / **cs.uesp.net/wiki/Category:Actor_Values（oldid=2221・取得成功）** ＝ 61=ResistFire / 62=ResistFrost / 68=ResistShock / 64=ResistMagic / 71=ResistWaterDamage（DATA+0x10・ctor 既定 72 の解明） / 取得失敗: en.uesp.net/wiki/Oblivion_Mod:MGEF(404)・en.uesp.net/wiki/Oblivion_Mod:Mod_File_Format(404)・geckwiki.com/index.php?title=MGEF(403)・cs.uesp.net/wiki/MGEF(404)・en.uesp.net/wiki/Oblivion:Actor_Value(404)。

## §5 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-4 | **MGEF DATA の flags 読み位置が port と engine で不一致**（b-2 実測で縮小・確定）: engine は **DATA+0x00 = mgefFlags**（obj+0x58、ロード時 bit21 クローズ＋OBME 表マージ）。port は DATA+0x00 を "unknown0" として**破棄**し、**DATA+0x10（実体＝Resist AV: 61/62/68/64）を "flags" と誤ラベル**して読む。school(+0x0C)/baseCost(+0x04)/av(+0x08) の位置は engine と一致（正しい）。**現状 flags の消費者は grep ゼロ＝実害なし**が、将来 hostile(bit0)/bit22(bit Disabled) など flags を使う機能を作る際はドラフト C の訂正が前提。（＋ DATA+0x14..+0x3F の残り 36B は未解明） | 低〜中（消費者実在化のとき中） |
| N-5 | **fourcc 直書き 157 点・32 関数のうち 28 関数の用途未個別化**（＋`FUN_0041B950` はスキャン対象外6コードで個別特定済み）: `FUN_0041B9A0`（要素: fire=1/frost=2/shock=4）/ `FUN_0041B950`（シールド系 1/2/4/8）/ `FUN_00413A90`（AV ハブ）/ `FUN_00417420`（既定生成 89 点）/ `FUN_00419B30`（DISE/POSN 代表・bit22・SEFF）は解明済み。残る INVI 9・VAMP 5・CUDI/CUPO・SUDG・DIAR/DIWE・REHE・Fortify 系等の個別述語（耐性付与・ヒット時挙動等）が未特定。「engine の分類は mgefCode 直書き」という結論の補強材料になる | 低 |
| N-6 | **v218 §5 L262「':' 無し」記述の訂正**: エンジン実名は `fQuestScriptDelayTime:MAIN`（本タスク EXE 実測）。結論（crosscheck2 はセクション帰属ずれの結合アーティファクト）は正しいが機構説明が誤り。v218 は既存 doc のため本タスクでは上書きせず指摘のみ | 低（参考） |
| N-7 | **2026-11-01 20:51:24-20:52:22 の 6 ファイル同時編集の主体・内容の確定**: gmst_defaults.h（kEntries -1）/ jni_bridge.cpp（frame-dt 乗算削除）/ settings_manager.h（NOTE -18 行相当削除）/ oblivion_ini_defaults.h（find_by_key コメント訂正＝v233 §5-3 とは別文言での訂正）/ esm_reader.h（+4 行相当・SpellData school/actorValue 追加帯の疑い）/ combat_formulas_tests.cpp。git 不使用のため内容差分は未取得。v233 が引用した旧行番号（esm_reader.h:428 / gmst_defaults.h:60 / settings_manager.h:39-51・54-79 等）と現物のずれはすべてこの編集帯で説明可能 | 中（履歴確認用） |
| N-8 | **Data +0x14..+0x3F（36B）と ctor 既定 +0x74=1.0f / +0x90=float(0xB33704) の正体未解明**: 生成経路の related-effects 配列（+0x6C..?）を除き未使用疑い。UESP の MGEF DATA 仕様書（Mod_File_Format 系）が本タスクでは 404/403 で取得できず、EXE 実測のみ | 低 |
