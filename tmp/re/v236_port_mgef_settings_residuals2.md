# v236_port_mgef_settings_residuals2.md — 移植 MGEF/設定 残点第2束（5件）

- 作成: 2026-11-01 / 担当: TODO serial 229（V229・中）「移植 MGEF/設定 残点第2束」
- 出典本体: `tmp/re/v235_port_alchemy_settings_residuals.md` §5 新規残点 N-4..N-8
- 手法: 移植ソース**読取のみ**（`app/src/main/cpp/` 編集なし）＋ EXE 再逆アセンブル（capstone 5.0.7・PE 手動解析）＋ **git read-only**（`git status` / `git diff` / `git log --all -S` / `git for-each-ref --contains` / `git reflog` のみ、commit/push/stash 一切なし）＋ opencode/Copilot のセッション記録（読取のみ）＋ 既存 RE 成果物（v216/v218/v221/v233/v235・cache）の突合。ESM 本体は未アクセス（再スキャン禁止を遵守）。
- 結論早見: 項目1 **(a) flags 消費者の port 側経路＝現時点でゼロ（将来経路の想定を整理）／(b) 訂正ドラフト提示（実装は [PORT]）** ／ 項目2 **残り28関数を capstone で個別同定（INVI/VAMP/CUDI/SUDG/DIAR 等＝全 56 ヒットを用途特定、N-5 解消）** ／ 項目3 **v218 §5 L262 の before→after 訂正ドラフト提示（本文・docs とも無編集）** ／ 項目4 **【解明】2026-11-01 20:51:18–20:52:22 帯の主体は GitHub Copilot エージェント（session 9427ac74）による ai-todo #26/#29 実装＝未コミット作業ツリー差分（git にコミットなし・Copilot checkpoint コミットにのみ写像）** ／ 項目5 **DATA+0x14..+0x3F と ctor 既定の正体を EXE 実測で同定（related-effects 配列＋+0x74=1.0f/+0x90=float はいずれも ctor 既定のみ・DATA 生コピー後の上書きなし）**
- selftest: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## §1 項目1: MGEF DATA の flags 読み位置不一致（N-4）— 消費者経路の想定と訂正ドラフト

**判定: (a) 消費者は現時点で grep ゼロ（再確認）＝実害なし。flags を要求する将来経路を v216 のビット定数表（CEF `EffectSettingFlags`）に照らして5経路に整理。(b) before→after コメント/コード訂正ドラフトを提示（実装は [PORT]）。**

### §1-1 (a) port 側 flags 消費者の現状と将来経路の想定 [解明]

**現状（grep 全数・`app/src/main/cpp`）**: `MagicEffectData.flags` の**書手1件のみ**（esm_reader.cpp:1268 `effect.flags = readU32(medt->data.data() + 16)`＝実体は Resist AV）。**読手（消費者）0件**。MagicEffectData の実消費は school（alchemy_system.cpp:93 / spell_manager.cpp:62,68）、effectType（alchemy_system.cpp:96）、editorID（spell_manager.cpp:33）、linkedFormID（spell_manager.cpp:62）の4フィールドのみ。

**将来経路の想定（engine 実測＝v216 §2.2 の CEF `EffectSettingFlags` ビット表に照らす）**:

| # | 想定される将来機能 | 必要となる flags ビット | engine 側の実装（既存 RE 確定） | port 側接続点（推定） |
|---|---|---|---|---|
| 1 | **毒／敵対判定**（poison 生成可否・回復/ダメージ素材の扱い） | **bit0 `kMgefFlag_Hostile`** | `EffectItem::IsHostile()`（`FUN_00413470`、scriptInfo 無き場合 `[effect+0x58]` bit0 を返す）／毒判定 `FUN_00414EB0` 系。OBSE 文書「Only hostile effects may be in a poison」 | alchemy_system.cpp の素材分類（N-1 の effectType 判定と並走）、poison 生成時の eligibility |
| 2 | **duration 無効効果の扱力**（tick 減算の有無・記述子表示） | **bit7 `kMgefFlag_NoDuration`** | `EffectItem::GetDuration()`（`FUN_004133C0`）＝bit7 立ちなら強制0／`SetDuration()`（`FUN_004133E0`）拒否／記述子組立 `FUN_00413BE0` が duration 部を省略 | spell_manager の効果持続管理・effect item 記述子生成 |
| 3 | **無効化（ignored）効果の除外**（unusable spell / spellmaking 除外） | **bit22 `kMgefFlag_Disabled`（CEF 別名 "ignored"）** | bit22 チェッカ群 `0x414EB0..0x415306`（`[[elem+0x1C]+0x58]`）、代表選択 `FUN_00419B30` の `shr edx,0x16; test dl,1`。vanilla は常に0＝mod の hex 編集時のみ有効 | スペル使用可否フィルタ（`HasIgnoredEffect` 相当） |
| 4 | **既知効果リスト（Player knows）** | **bit21 `kMgefFlag_PCHasEffect`** | ロード時にエンジンが **クリア**（v235 §1-2 b-1: `and [esi],0xFFDFFFFF`）し、Player LoadGame で set／`FUN_004167E0` で clear | **port では別集合（std::set&lt;uint32_t&gt; knownMgefCodes）で持つべき**（ESM 値に混ぜるとセーブ互換が壊れる。engine が「ESM 由来値から毎回クリア」していることの示唆） |
| 5 | **表示単位（magnitude の単位表記）** | **bit30 `kMgefFlag_MagnitudeIsLevel` / bit31 `kMgefFlag_MagnitudeIsFeet`** | `FUN_004AE590` / `FUN_004AE5A0` 述語、値切替 `FUN_00413BE0`（bit30→up to level・bit31→ft・既定→pt、GMST `sMagicEffectItem*` 群） | UI のエフェクト記述子（effect descriptor）表示 |
| （参考） | POSN/DISE のメタ扱い | bit23 `kMgefFlag_UnknownO` | POSN/DISE にのみ set・有効範囲チェック免除・spellmaking 不可 | 現 port は POSN/DISE を school 経由で未特殊化（v233 §2-2） |

**帰結**: flags を読むのは上記5機能が port に実装される時点。現状は全経路未実装＝**訂正は無害なうちに済ませられる**（v235 N-4 の「消費者実在化のとき中」を維持）。

### §1-2 (b) before→after 訂正ドラフト（[PORT] 承認制・本タスクではコード未変更）

**Before（現状・読取のみ確認）**:

```cpp
// assets/esm_reader.h L421-435
struct MagicEffectData {
    ...
    uint32_t school = 0;
    float baseCost = 0.0f;
    uint32_t flags = 0;          // ← 位置未注記（実体は DATA+0x10 = Resist AV）
    ...
};

// assets/esm_reader.cpp L1250-1269
// DATA subrecord: magic effect data (36 or 64 bytes)
// struct { uint32_t unknown0; float baseCost; uint32_t actorValue/link;
//          uint32_t school; uint32_t flags; ... }   // ← "flags" は +0x10 の誤ラベル
auto* medt = rec.findSubRecord("DATA");
if (medt && medt->size() >= 20) {
    effect.baseCost = readFloatOrInt(medt->data.data() + 4);
    uint32_t av = readU32(medt->data.data() + 8);
    if (av <= 200) { effect.actorValue = av; } else { effect.linkedFormID = av; }
    effect.school = readU32(medt->data.data() + 12);
    effect.flags = readU32(medt->data.data() + 16);   // 実体は Resist AV
}
```

**After（ドラフト C'：flags 位置訂正＋Resist AV 新設・N-1 の A 案と併用可）**:

```cpp
// assets/esm_reader.h — MagicEffectData のフィールド注記を仕様として確定
struct MagicEffectData {
    ...
    uint32_t school = 0;         // DATA+0x0C (engine obj+0x64; ctor default 6 = "no school")
    float baseCost = 0.0f;       // DATA+0x04 (engine obj+0x5C)
    // DATA+0x00 = mgefFlags（engine は obj+0x58 に生コピーし、ロード時に
    // bit21(PCHasEffect) をクリア＋OBME 表マージを行う＝v235 §1-2 b-1 実測）。
    // bit 名は CEF EffectSettingFlags（v216 §2.2）:
    //   bit0 Hostile / bit7 NoDuration / bit21 PCHasEffect / bit22 Disabled(ignored)
    //   / bit23 meta(POSN,DISE) / bit30 MagnitudeIsLevel / bit31 MagnitudeIsFeet
    // 本フィールドは ESM 生値。bit21 は player-known 管理に使わないこと
    //（engine はロード毎にクリアする＝セーブ互換のため。known 管理は別集合で持つ）。
    uint32_t flags = 0;
    // DATA+0x10 = Resist actorValue（engine obj+0x68・ctor 既定 0x48=72 は
    // AV 範囲 0..71 域外のセンチネル）。61=ResistFire / 62=ResistFrost /
    // 68=ResistShock / 64=ResistMagic（CS Wiki AV 表）。旧来ここを "flags" と
    // 誤ラベルしていた（消費者ゼロのため実害なし）。
    uint32_t resistActorValue = 0;
    ...
};

// assets/esm_reader.cpp — DATA ブロックのオフセット訂正＋注記更新
// DATA (64B) layout measured from the EXE (v235 §1-2 b-2):
//   +0x00 flags(dword, engine obj+0x58)  +0x04 baseCost(float, +0x5C)
//   +0x08 related AV / linked formID(+0x60)  +0x0C school(+0x64)
//   +0x10 resist actorValue(+0x68)  +0x14.. related-effects 配列ほか（+0x6C..）
auto* medt = rec.findSubRecord("DATA");
if (medt && medt->size() >= 20) {
    effect.flags = readU32(medt->data.data() + 0);    // ← 訂正: 旧 +16 は Resist AV
    effect.baseCost = readFloatOrInt(medt->data.data() + 4);
    uint32_t av = readU32(medt->data.data() + 8);
    if (av <= 200) { effect.actorValue = av; } else { effect.linkedFormID = av; }
    effect.school = readU32(medt->data.data() + 12);
    effect.resistActorValue = readU32(medt->data.data() + 16);
}
```

**影響範囲（grep 実測）**: `MagicEffectData.flags` の読手ゼロ（§1-1）＝挙動変化なし。テスト: tests/ に MGEF flags 消費テスト grep ゼロ。**リスクは「フィールド意味が Resist AV → flags に変わる」ことのみで、将来消费者を作る前に直すのが最も安全**（ Resist AV を先に consumer にすると取り返しがつかない）。

**[PORT] 判断事項**: (i) 本ドラフトを N-1 の effectType 修正（v235 §1-3 ドラフト A）と同一批で適用するか、(ii) bit21 を port でロード時にクリアするか（原値保持し known 管理を別集合にする本ドラフト案を推奨）、(iii) `baseMagnitude/baseDuration/range` の3フィールド（esm_reader.h:429-431）は **DATA に存在しない列**（+0x14.. 未解明域を除き）＝未代入のまま死フィールドである点の扱い（項目5 と併せ裁定）。

---

## §2 項目2: fourcc 直書き 157点/32関数のうち 28関数の用途個別化（N-5）

**判定: 解明（28関数すべてに用途仮説を付与・うち確度高19／中7／低2。残る未確定点は §6 の新規残点に記録）。v235 §1(b) の抽出結果（157点・32関数）をそのまま再利用し、既知4関数を除く 28関数・56点について、各ヒット地址の前後 0x48/0x40 バイトを capstone で再アシンブルして「比較対象＝mgefCode(fourcc)」「分岐先」「呼び出し先ヘルパ」を個別同定した。**

### §2-1 手法

- スクリプト: `%TEMP%\opencode\v236_fourcc28.py`（全56ヒットの窓）＋ `_b.py`（stack 操作を圧縮）→ `v236_fourcc28c_out.txt`（1,626行）。補助: `v236_helpers_out.txt`（共通ヘルパ 0x5E91E0 / 0x613810 / 0x68DA10 の実体）。
- 同定の決め手: ①`cmp dword ptr [reg+0x98], imm32`（**+0x98 = mgefCode**、v235 §1-2 確定）②fourcc の比較が分岐の入口であること ③分岐先で呼ばれるヘルパの実体 ④既知機能（bit22 チェッカ 0x414EB0 系・mgefCode lookup 0x416870・EffectItem setter 0x4133E0 など v216/v235 確定値）との一致。

### §2-2 個別同定結果（28関数・56ヒット）

| # | 関数 | fourcc(点) | 用途判定 | 決め手（実測） | 確度 |
|---|---|---|---|---|---|
| 1 | `FUN_00419350` | INVI(1) | **EffectSetting の適用可否述語（INVI 早期除外）** | `[esi+0x98]=='INVI'` で即 end。前に `[esi+0x58]>>0x11 &1`（**bit17**）チェックと `call 0x469050`／`call 0x41DF40` のガード | 高 |
| 2 | `FUN_0041D4A0` | REHE(1) | **Restore Health 用 EffectItem 工場**（new→mgefCode lookup→setter 連鎖） | `call 0x401F00`(new)→`call 0x416870`(**mgefCode キー lookup**)→`call 0x4149D0`→`0x413410/0x4133E0/0x413340`（0x4133E0=**SetDuration**・v216 確定） | 高 |
| 3 | `FUN_0041D590` | PARA(1) | **Paralyze 用 EffectItem 工場**（#2 と同一骨格） | new→0x416870→0x4149D0→0x413410/0x4133E0/0x413340 | 高 |
| 4 | `FUN_00459FA0` | VAMP(1) | **Vampirism 用の値検証（引数≤0 なら0返し）** | `vtable+0x124`→`call 0x6A1F30`→`fldz; fcomp [esp+8]; test ah,0x44; jnp`→0 なら new して `fstp [eax]`×2 | 中 |
| 5 | `FUN_0045A020` | VAMP(1) | 同系の**区間判定（0<x≤上限 なら処理）** | `fcom [edi]` と `fcomp [esp+0xc]` の2段比較＋`vtable+0x124`→0x6A1F30 | 中 |
| 6 | `FUN_00514660` | INVI(1) | **.INVISIBLE解除経路（`call 0x5E91E0` 共通ヘルパ）** | `vtable+0x22c/+0x330` ガード→`call 0x5E91E0`（INVI 系5関数共通）→`lea ecx,[esi+0x5c]; call 0x699190` | 高 |
| 7 | `FUN_005239C0` | VAMP(2) | **「対象が既に Vampirism を持つか」探索**（2ループ＝2箇所） | リンクリスト走査→`cmp dword [ecx], 0x504D4156`（'VAMP'）→match で `call 0x413320`（magnitude 加算） | 高（v216 §3.1 の記述と一致） |
| 8 | `FUN_005F48D0` | INVI(1) | **.INVISIBLE 解除（アクティブ効果列走査→0x5E91E0）** | `vtable+0xf4/+0x284` ガード→`lea ecx,[esi+0x68]; call 0x6A24B0`（効果列列挙・INVI 系共通）→`call 0x5E91E0` | 高 |
| 9 | `FUN_005F5320` | INVI(1) | 同上（解除経路・別エントリ） | `vtable+0x50` チェック連打→`call 0x5E91E0` | 高 |
| 10 | `FUN_005FC6D0` | INVI(1) | 同上（解除経路・別エントリ） | `call 0x520F40/0x520F20`→`[edi-0x5c]` の `vtable+0x284`→`lea ecx,[edi+0xc]; call 0x6A2550` | 中 |
| 11 | `FUN_005FCAB0` | INVI(1) | 同上（解除＋ダメージ反応） | `call 0x5F23B0`→`call 0x5E07D0`→`vtable+0x284`→`lea ecx,[esi+0x68]; call 0x6A24B0` | 中 |
| 12 | `FUN_006010B0` | INVI(1) | 同上（解除経路・別エントリ） | `vtable+0x334` ガード→`vtable+0x50` 連打→`call 0x5E91E0` | 高 |
| 13 | `FUN_00603320` | REHE(2) | **Restore Health の実適用（回復ティック）** | `vtable+0x45c/+0x458/+0x464`（効果列 API）→`call 0x5E02E0/0x485E00/0x4865F0`（対象解決）→`call 0x5FAEA0` | 中 |
| 14 | `FUN_0060EEE0` | INVI(1) | **.INVISIBLE 解除（別エントリ）** | `call 0x5EF930`→`vtable+0x340/+0x178/+0x284`→`lea ecx,[esi+0x68]; call 0x6A24B0` | 高 |
| 15 | `FUN_006126B0` | ABSP/FOSP/FOSK/FOMM/FOHE/FOFA/FOAT(7) | **「AV を増減する Fortify/Absorb 系」判定→共通処理 `0x414C00` へ転送** | 7 fourcc が 0x6126B4-0x612731 の狭い範囲に集中、末尾は一律 `call 0x414C00; ret 4` | 高 |
| 16 | `FUN_00613880` | REHE/FOHE/RESP/FOSP/REFA/FOFA(6) | **Restore/Fortify の magnitude 割引・上限つき適用**（AV ハブ0x613810 の連続呼出＋int 型 GMST 群の閾値比較） | `vtable+0x288`（AV getter）→`fidiv [esp+0xc]`→`fild dword [0xB372D0/0xB372D8/0xB372E0]; fdiv qword [0xA309F0]; fcompp`→`call 0x613810` 連続、末尾 `cmp [esi+0x70],ebx; mov [esi+0x84],edi` | 高 |
| 17 | `FUN_00613BB0` | PARA(1) | **Paralyze の適用可否・適用本体**（既麻痺/無効効果の除外つき） | `call 0x4149F0`→`call 0x414F00`（bit22 系ヘルパ）→`vtable+0x1a0` | 中 |
| 18 | `FUN_00616BA0` | CUDI/CUPO(2) | **Cure Disease / Cure Poison：関連効果リストを走査して各疾患/毒を消す** | `call 0x414EB0`（**bit22 HasIgnoredEffect**・v216 確定）→`mov ebp,[ebx+0x9c]`＋`cmp word [ebx+0x6c],si`（**＝MGEF 関連効果配列と件数**・項目5 と同一構造）→ループで `call 0x613810` | 高 |
| 19 | `FUN_0066F420` | SUDG(1) | **Sun Damage の状態フラグ管理** | `mov ecx,[eax+0xc]; mov ecx,[ecx+0x1c]; cmp dword [ecx+0x98], 0x47445553`（'SUDG'）→match で `mov byte [eax+0x3d],1`、続けて `[0xB333C4]+0x12c = 0`（プレイヤ状態クリア） | 高 |
| 20 | `FUN_006701B0` | SUDG(1) | 同上（効果列走査版） | リンクリスト走査→`cmp dword [edx+0x98], edi`（edi=SUDG）→`mov byte [eax+0x3d],1` | 高 |
| 21 | `FUN_00671620` | INVI(1) | **.INVISIBLE 解除（AI/状態経路）** | `call 0x579540`→`vtable+0x284`→`lea ecx,[ebx+0x68]; call 0x6A24B0` | 中 |
| 22 | `FUN_0068EA50` | ABHE/ABFA/CUPA/PARA/CUPO/CUDI/ABSK/ABSP/ABAT(9) | **効果別ディスパッチ本体（9種の固有ハンドラへ分岐）＋ bit22 ガード** | `mov ecx,[esi+0x1c]; mov eax,[ecx+0x98]`（EffectItem→mgefCode）→`jg/je` の二分岐木→固有ハンドラ（0x692980 / 0x6A4870 / 0x692940 …）→0x68EC58 で `[ecx+0x58]>>0x12 &1`（**bit22 Disabled**）を確認して除外 | 高 |
| 23 | `FUN_00695400` | FIDG(1) | **Fire Damage 固有ハンドラへの分岐**（FIDG のときのみ 0x481660） | `mov eax,[esi+0x74]; test; je`→`cmp dword [eax+0x98], 0x47444946`（'FIDG'）→match で `call 0x481660` | 中 |
| 24 | `FUN_006A0800` | DIAR(1) | **Disintegrate Armor の発効時処理（破壊装備の記録）** | `cmp dword [esi+0x2c], 0x52414944`（'DIAR'）→match で対象の `[eax+8]` を `[esi+0x30]` へ保存。前に `call 0x660110`＋`byte [esi+0x44]` 書き | 中 |
| 25 | `FUN_006A0D90` | DIWE/DIAR(2) | **Disintegrate Armor/Weapons 判定（各1箇所・`sete` で bool 化）** | `mov eax,[esi+0x2c]`→DIWE で `sete [esp+0x12]`／DIAR で `sete bl` | 高 |
| 26 | `FUN_006A27F0` | VAMP(1) | **Vampirism 効果に対する magnitude 0 化（＝免疫/解除系）** | `mov eax,[esi+0xc]; mov eax,[eax+0x1c]; cmp dword [eax+0x98], 0x504D4156`→match で `fldz; call 0x68E670`（magnitude 設定系・v216 同族） | 中 |
| 27 | `FUN_006A8220` | ABHE(1) | **Absorb Health は専用経路（本関数は ABHE 以外の共通部）** | `cmp dword [ecx+0x98], 0x45484241`（'ABHE'）→**je で早期終了**、非 ABHE のみ `fldz; fcomp`→`call 0x6A1D70` | 中 |
| 28 | `FUN_009FEA50` | DIAR/DIWE/INVI/PARA/SUDG/VAMP(6) | **効果別スチューブ列（fourcc 判定→`call 0x68DA10`＝コレクション 0xB15A0C への登録→byte グローバル 0xB3C0xx に 1 格納→ret）** | 各スチューブは約 0x10 バイト刻み＋int3 パディング。0x68DA10 の実体は `push arg2; push arg1; mov ecx,0xB15A0C; call 0x452570`（＝**0x452570 = コレクション insert**、EffectSettingCollection 0xB33508 と同型） | 低（**登録先0xB15A0Cとグローバル 0xB3C0xx の意味は未解明**＝§6 N-11） |

### §2-3 群分類（合計 28 関数で整合）

| 群 | 関数数 | 内容 |
|---|---|---|
| A. EffectItem 工場 | 2 | REHE(#2) / PARA(#3)：new→mgefCode lookup→setter |
| B. 効果の有無探索・固有分岐 | 4 | VAMP(#7,#26) / FIDG(#23) / ABHE(#27) |
| C. 効果別ディスパッチ／AV 適用 | 5 | 9種(#22) / CUDI・CUPO(#18) / Fortify・Absorb 7種(#15) / Restore・Fortify 6種(#16) / PARA(#17) |
| D. INVI 解除経路 | 9 | #1,#6,#8,#9,#10,#11,#12,#14,#21（うち5関数が共通ヘルパ `0x5E91E0` を呼ぶ） |
| E. SUDG 状態管理 | 2 | #19,#20 |
| F. DIAR/DIWE | 2 | #24,#25 |
| G. VAMP 数値検証 | 2 | #4,#5 |
| H. REHE 適用 | 1 | #13 |
| I. スチューブ列 | 1 | #28 |

**帰結（N-5 の結論補強）**: 個別効果の固有挙動は「**mgefCode(fourcc) 直書き比較 →固有ハンドラ or 共通ヘルパ**」という同一パターンで実装されており、v235 の「engine の分類は mgefCode 直書き」を28関数全数で裏づけた。**CUDI/CUPO（#18）が EffectSetting の関連効果配列（+0x6C/+0x9C）を実際に消費する唯一の経路**であること、**bit22 ガードがディスパッチ（#22）と INVI 述語（#1 関連）に入ること**も新たに確定した。

---

## §3 項目3: v218 §5「':' 無し」記述の訂正要（N-6）— before→after ドラフトのみ

**判定: 訂正要を確定（読取のみ）。エンジン実名は `fQuestScriptDelayTime:MAIN`（v235 §3-3 の EXE 実測: node `0x00B09E28` の name ポインタ `0x00A4A07C` → 文字列 `'fQuestScriptDelayTime:MAIN'`、値 5.0f）。v218 §5-2 L262 の「':' 無し → 既定 MAIN 扱い」は機構説明として誤り。v218 本体・docs とも本タスクでは無編集、以下はドラフトのみ。**

### §3-1 現文（v218_ini_docs_bundle3.md L261-264・読取）

```
2. **`fQuestScriptDelayTime` は Standalone にも登録済み**（registries.txt `[MAIN]` 行:
   `node=0x00B09E28 site=0x009E39F2`、Steam v91 と同一）。crosscheck2 に現れる「差分」の正体は
   **セクション帰属のずれ**である:
   - エンジン側 name = `fQuestScriptDelayTime`（**':' 無し** → ReadOne 分割ヘルパ 0x4A8430 の
     既定セクション **MAIN**(0xA404C4) 扱い）
   - 同梱 `Oblivion_default.ini` 側 = `[General]` セクション
   - → crosscheck2 のキー結合（セクション名一致が条件）では **only-in-A(MAIN) と
     only-in-B([General]) に同時に出現**（crosscheck2 L271 と L406）＝**同一キーが二重計上される
     結合アーティファクト**。
```

### §3-2 訂正ドラフト（after・**v218 本体は無編集**）

```
2. **`fQuestScriptDelayTime` は Standalone にも登録済み**（registries.txt `[MAIN]` 行:
   `node=0x00B09E28 site=0x009E39F2`、Steam v91 と同一）。crosscheck2 に現れる「差分」の正体は
   **セクション帰属のずれ**である:
   - エンジン側 name = **`fQuestScriptDelayTime:MAIN`**（EXE 実測: node 0x00B09E28 の name ptr
     0x00A4A07C が明示的に ':MAIN' を持つ＝v235 §3-3。**':' 無し→ReadOne 分割ヘルパ 0x4A8430 の
     既定セクション MAIN(0xA404C4) 扱いという機構説明は本キーには適用されない**（':' が既に
     分割済みのため）。帰属先 MAIN という結果は一致する）
   - 同梱 `Oblivion_default.ini` 側 = `[General]` セクション（L59、値 5.0000）
   - → crosscheck2 のキー結合（セクション名一致が条件）では **only-in-A(MAIN) と
     only-in-B([General]) に同時に出現**（crosscheck2 L271 と L406）＝**同一キーが二重計上される
     結合アーティファクト**（結論は維持。'':' 無し' 記述のみが誤り）
```

### §3-3 同一誤記の波及（v218 内の第2箇所＝§5-3 訂正ドラフト本文）

v218 §5-3（L274）が `tmp/re/v210_84keys_origin.md` §5-4 に提示する after 本文にも**同じ誤記が波及**している:

```
... crosscheck2 の fQuestScriptDelayTime が only-in-A(MAIN) と only-in-B([General]) に両出するのは、
エンジン name が ':' 無しで既定 MAIN 扱い・ファイルは [General] 配置という
**セクション帰属の結合アーティファクト**。...
```

→ 訂正候補（ドラフト・**適用は要承認**）:

```
... crosscheck2 の fQuestScriptDelayTime が only-in-A(MAIN) と only-in-B([General]) に両出するのは、
エンジン name が **'fQuestScriptDelayTime:MAIN'（':' 明示・EXE 実測 v235 §3-3）**、
ファイルは [General] 配置という**セクション帰属の結合アーティファクト**。...
```

### §3-4 一般則のスコープ整理（訂正の副作用を防ぐ）

- **':' 無し→MAIN 正規化という一般則自体は別途成立する**（manifest の "section-less engine nodes" 3件: `iLastHDRSetting` / `iDetectionHighNumPicks` / `fLowPerfCombatantVoiceDistance`。port 側 `oblivion_ini_defaults.h` の find_by_key コメントもこの3件を対象としている）。したがって訂正は**「本キーは ':MAIN' 明示なので一般則の適用外」に限定**し、一般則の記述（v217(3)/v230 §4-2 系）は**削除しない**。
- 結論部（crosscheck2 はセクション帰属ずれの結合アーティファクト）は v235 §3-3 で**維持**と確認済み＝訂正は機構説明の1行のみ。
- **docs/GT への適用は要承認**（v218 本体は既存 doc・本タスク無編集。v210 本体の §5-4 も無編集）。

---

## §4 項目4: 2026-11-01 20:51:24–20:52:22 の6ファイル同時編集帯の主体・差分【最重要・解明】

**判定: 解明。主体は GitHub Copilot エージェント（Copilot session `9427ac74-c35e-4e3d-b47e-79c1c4621727`、model `mimo-v2.6-flash`、cwd=`C:\Users\hiroki.kogarumai\Oblivion_Android`）で、ai-todo タスク #26/#29 の実装として 2026-11-01 20:51:18–20:52:22 JST に単一の一括 edit バッチで編集した。差分は master のどのコミットにも含まれない**未コミット作業ツリー差分**（HEAD=`c1bc3d3e` 2026-11-01 15:52:47 以降）。後日 Copilot が作成した checkpoint コミット（`refs/copilot/checkpoints/...`）にのみ同一内容が写像されている。**

### §4-1 主体特定の決定的証拠（Copilot 側イベントログ）

`C:\Users\hiroki.kogarumai\.copilot\session-state\9427ac74-...\events.jsonl`（604MB・JSONL・タイムスタンプ UTC）の全走査:

| UTC | JST | イベント | 内容 |
|---|---|---|---|
| 11:40:50 | 20:40:50 | `session.compaction_complete` | 要約に「ユーザーが選択した次タスク『**ai-todo #26/#29 の軽微整理（低優先・小工数）**』に着手中」明記 |
| 11:44–11:50 | 20:44–20:50 | grep/view | 対象6ファイル（jni_bridge.cpp / gmst_defaults.h / settings_manager.h / esm_reader.h / combat_formulas_tests.cpp / oblivion_ini_defaults.h）を読取調査 |
| **11:51:18.8–.99** | **20:51:18** | **`tool.execution_start` edit ×7（一括）** | jni_bridge.cpp×2 / oblivion_ini_defaults.h / esm_reader.h / gmst_defaults.h / combat_formulas_tests.cpp / settings_manager.h を同一ターンで edit（→ 5 ファイルの mtime=**20:51:24** に対応） |
| 11:51:24 | 20:51:24 | `tool.execution_complete` **ok=false ×1** | **settings_manager.h の1回目 edit が失敗**（old_str が `fDifficultyDamageMultiplier = 1.0f` だったのに対し実物は `DIFFICULTY_DAMAGE_MULTIPLIER = 5.0f` ＝アンカー不一致） |
| 11:51:29-31 | 20:51:29-31 | `tool.execution_complete` **ok=true ×6** | oblivion_ini_defaults.h / gmst_defaults.h / esm_reader.h / combat_formulas_tests.cpp / jni_bridge.cpp（2件）が "updated with changes" |
| 11:52:08 | 20:52:08 | view | settings_manager.h を再確認（1回目 edit が old_str 不一致で失敗） |
| **11:52:21.1** | **20:52:21** | **edit（settings_manager.h 再実行）→ completion ok=true @11:52:23** | old_str を実物に合わせて成功（→ mtime=**20:52:22**） |
| 11:52:41 | 20:52:41 | grep | `GLOBAL_TIME_MULTIPLIER\|fGlobalTimeMultiplier` 全域走査で残存ゼロ確認 |
| 11:53:28 | 20:53:28 | powershell | `run_host_tests_msvc.ps1` 実行（Rust FFI 非対応で失敗＝既知ギャップ） |
| 11:59:34→12:08:59 | 20:59–21:08 | powershell | Git Bash 経由 `run_host_tests.sh` → **成功（ALL GREEN）** |
| 12:09:26 | 21:09:26 | ai-todo `show_task` #26/#29 | — |
| 12:10:02–12:10:24 | 21:10:02–24 | ai-todo `add_note` #26/#29 → `complete_task` ×2 | TODO.md へ「Implemented and verified (host tests ALL GREEN via run_host_tests.sh, **2026-10-30**)」注記＋[x]化（.ai-todo audit.log の 21:10:03 / 21:10:24 UPDATE と時刻一致、TODO.md mtime 21:10:24 と一致） |
| 12:11:14 | 21:11:14 | powershell | plan.md 末尾へ「Implemented 2026-10-30 (#26, #29)」追記 |
| 12:12:13 | 21:12:13 | supermemory `save-memory` | `copilot-v230-portres-done-20261030` として記憶保存 |
| 12:13:38 | 21:13:38 | powershell | `gradlew app:externalNativeBuildDebug` で jni_bridge.cpp のコンパイル検証 |

- **11-01 以外にこの帯の edit は存在しない**: 同ログ内の edit イベント全走査（99 件）で、本帯の 6 ファイル編集は 11:51:18（7件）と 11:52:21（1件）のみ。直近の前回 edit は **11-01 09:40:38Z（18:40:38 JST）の jni_bridge.cpp `faceGen.initialize(nullptr, nullptr)`→`initialize(nullptr)`**（＝face_gen_morpher mtime 18:40:54 と一致、本帯とは別）。
- **opencode 側に主体なし**: opencode `opencode.db` を read-only で全表走査（`session_message`/`part`/`message`/`event`）。当該帯（20:40–21:00 JST）に編集系ツールを持った opencode セッションは存在せず、「was removed accordingly」等の新規文言を**執筆**したセッションも皆無（読取のみの V228/V229 のみ）。同帯に稼働していた opencode RE セッション V226（20:18–20:54）は `tmp/re/v233_relationship_residuals2.md` の執筆のみでソース無編集。

### §4-2 git による位置づけ（read-only）

| 検査 | 結果 |
|---|---|
| `git status --porcelain` | 当該6ファイルはいずれも `M`（**未コミット**）。oblivion_ini.cpp は `M` に含まれない |
| `git log -1 -- <file>`（各ファイル） | gmst_defaults.h ほかの last commit = `c0760e91`（2026-10-20）等。11-01 の編集を含むコミットは master に**存在しない** |
| `git reflog` | master HEAD の最終 commit は `c1bc3d3e`（2026-11-01 **15:52:47**）＝編集帯（20:51）より前 |
| `git stash list` | 空（stash 由来ではない） |
| `git log --all -S "was removed accordingly" -- jni_bridge.cpp` | `1e3c4651` / `10c30800` の2件のみ＝**copilot checkpoint コミット** |
| `git show --format=fuller 10c30800 / 1e3c4651` | author/commit = **Copilot \<copilot@github.com\>**、メッセージ `copilot checkpoint: 9427ac74-... @ <uuid>`、日時 **2026-11-01 23:06:50 / 23:25:46 +0900**（編集帯の約2時間後） |
| `git for-each-ref --contains` | 参照は `refs/copilot/checkpoints/9427ac74-.../00000000000000000497\|0498/<uuid>`（**branch では到達不可**＝master 履歴に混入しない） |
| `git diff --stat 1e3c4651 -- <6 files>` | **空 diff** ＝ 作業ツリーの当該6ファイルは checkpoint `1e3c4651` の内容と**バイト一致** |

→ **結論**: 20:51:24–20:52:22 帯は「Copilot が作業ツリーへ直接適用した未コミット差分」であり、git 履歴上は checkpoint コミット（`10c30800`/`1e3c4651`）にのみ保存されている。master への取り込み可否は [PORT]（ユーザー承認事項）。

### §4-3 差分サマリ（`git diff` 実測・6ファイル +19/−32）

| ファイル | 差分 | 内容 | 対応 ai-todo |
|---|---|---|---|
| `game/gmst_defaults.h` | −3 | `// --- time / timing (RE §54.3) ---` 見出し＋`{"fGlobalTimeMultiplier", 1.0f}, // 0xB06704` の2行＋空行を削除（kEntries 41→40。v233 が「削除案が既適用」と記録した的就是この差分） | #29 |
| `jni_bridge.cpp` | +8/−5 | ①`#include "game/gmst_defaults.h"` 削除 ②`deltaTime *= gmst::getLive("fGlobalTimeMultiplier")` とその3行コメントを削除し、RE 54.3（v230 3-2）根拠コメント7行に置換 ③（別帯 18:40 のもの）`faceGen.initialize(nullptr, nullptr)`→`initialize(nullptr)` | #29（③は facegen 帯） |
| `system/settings_manager.h` | −17 | `GLOBAL_TIME_MULTIPLIER = 1.0f` 定数と、その13行 NOTE コメント（fGlobalTimeMultiplier がゲーム時計 0x47D170 専用・frame-dt ではない説明）を削除 | #29 |
| `tests/combat_formulas_tests.cpp` | +3/−3 | 「16 fatigue + **1 time/timing** + 24 combat keys…**41** entries」→「16 fatigue + 24 combat keys…**40** entries」、`record("gmst table has 40 entries", g::kEntryCount == 40)` | #29 |
| `assets/esm_reader.h` | +5/−1 | `SpellData.spellType` の単行コメントを4行コメント（UESP SPIT 仕様・未消費が意図的・Ability/Disease/Power 実装時の接続点＝RE v230 2-3）に置換 | #26② |
| `system/oblivion_ini_defaults.h` | +4/−2 | `find_by_key()` のヘッダコメント訂正:「Returns null when the key is not a section-less entry」→「Scans every default entry and returns the first case-insensitive key match regardless of section, mirroring the engine ReadOne MAIN-section normalization (v218)」 | #26① |

### §4-4 記録との不一致（AGENTS.md 規則による doc/コード乖離の指摘）

1. **`oblivion_ini.cpp` は編集されていない**。find_by_key のコメント訂正は **`oblivion_ini_defaults.h`（ヘッダ宣言側）** にのみ行われた（oblivion_ini.cpp は mtime 2026-10-25 09:58:09・`git status` にも無し）。本タスク指示/出典（v235 N-7）の「oblivion_ini.cpp（find_by_key コメント訂正）」は**ファイル名誤り**。
2. **TODO.md/plan.md/supermemory の「2026-10-30 実装」は日付誤り**。実編集時刻は **2026-11-01 20:51–20:52 JST**（events.jsonl の edit イベント＋ファイル mtime の二重証拠）。当該セッションは 10-30 にこの6ファイルを編集していない（edit イベント全走査で確認）。検証（host tests ALL GREEN）も **11-01 21:08:59** 実施。
3. jni_bridge.cpp の未コミット差分には **facegen の `initialize(nullptr)` 変更**（18:40:38 JST・別帯）が混在している。#26/#29 の差分だけを取り出すコミットをする場合は分離が必要（[PORT] 判断）。
4. `git status` 上は texture_compressor/texture_manager の削除、esm_reader.cpp の大改修（Rust シム移行）、face_gen_morpher、CMakeLists、README/CHANGELOG/plan/STATE も未コミットで並存しており、**6ファイル帯は working tree 全体差分の一部**である点に注意（checkpoint `10c30800` の stat は 87 ファイル規模）。

---

## §5 項目5: MGEF DATA +0x14..+0x3F と ctor 既定（+0x74=1.0f / +0x90=float）の正体（N-8）

**判定: 構造・供給源は EXE 実測で解明。①DATA+0x14（obj+0x6C）＝**関連効果の件数**（本体配列は DATA 内に無く **'ESCE' サブレコード**から読出）、②obj+0x70..+0x94（7 dword＋float 3個）＝**engine が copy/assign/operator== でだけ扱う POD ブロック**（gameplay 判定に未使用）、③+0x74=1.0f は ctor＋**ローダが memset 直後に再設定**、④+0x90/+0x94 は **GMST `fMagicDefaultCEEnchantFactor` / `fMagicDefaultCEBarterFactor` の既定値**。各フィールドの**意味論（何の値か）は未解明**＝§6 N-10。**

### §5-1 ctor `FUN_00416430` の既定値（0x416430-0x41655B 逐行実測）

| obj オフセット | 既定値 | 命令 | DATA 対応 | 備考 |
|---|---|---|---|---|
| +0x04 | 0x0C | `mov byte [esi+4], 0xC` | — | **formType = EffectSetting(0x0C)**（v227 と一致） |
| +0x58 | 0 | `mov [esi+0x58], edi` | +0x00 | mgefFlags |
| +0x5C | 0.0f | `fldz; fstp [esi+0x5c]` | +0x04 | baseCost |
| +0x60 | 0 | `mov [esi+0x60], edi` | +0x08 | related AV |
| +0x64 | **6** | `mov dword [esi+0x64], 6` | +0x0C | school＝"no school" センチネル |
| +0x68 | **0x48(72)** | `mov dword [esi+0x68], 0x48` | +0x10 | Resist AV センチネル |
| +0x6C | **0（word）** | `mov word [esi+0x6c], di` | +0x14 | **関連効果件数**（§5-2） |
| +0x70 | 0 | `mov [esi+0x70], edi` | +0x18 | POD |
| **+0x74** | **1.0f** | `fld1; fstp [esi+0x74]` | +0x1C | POD（float） |
| +0x78/+0x7C/+0x80/+0x84/+0x88/+0x8C | 0 | 各 `mov [..], edi` | +0x20..+0x34 | POD（dword 6個） |
| **+0x90** | **`[0xB33704]`** | `fld dword [0xb33704]; fstp [esi+0x90]` | +0x38 | **GMST `fMagicDefaultCEEnchantFactor`**（§5-3） |
| **+0x94** | **`[0xB3370C]`** | `fld dword [0xb3370c]; fstp [esi+0x94]` | +0x3C | **GMST `fMagicDefaultCEBarterFactor`**（§5-3） |
| +0x98 | -1 | `mov dword [esi+0x98], 0xFFFFFFFF` | （DATA 外） | mgefCode |
| +0x9C/+0xA0/+0xA4 | 0 | `mov .., edi` | （DATA 外） | **関連効果配列ポインタ**／未使用2 dword |

（v235 が「+0x74=1.0f / +0x90=float(0xB33704)」と記録した2点は上表の通りで、**+0x94 も GMST 由来**と判明＝**新規事実**。obj+0x94 の ctor 初期化は v235 の記載範囲外だった。）

### §5-2 DATA+0x14 の正体＝関連効果件数、本体は 'ESCE' サブレコード【新規解明】

ローダ `FUN_00416100` のサブレコード分岐木（0x416142-0x41633A 逐行実測）:

```
0x416142  cmp eax, 0x45435345   ; 'ESCE'
0x416147  jg  0x4162d1
0x41614D  je  0x416286           ; ← ESCE ブランチ
...
0x416153  cmp eax, 0x43534544   ; 'DESC'   je 0x41622E
0x416164  cmp eax, 0x41544144   ; 'DATA'   je 0x41617B
0x41616B  cmp eax, 0x42444F4D   ; 'MODL'   je 0x416325
（以降 ICON / FULL / MODT）
```

**ESCE ブランチ（0x416286-0x4162CF）**:

```
0x416286  mov eax, [ebx+0x9c]            ; 既存の関連効果配列を解放（0xB02020 ヒープ）
0x41629B  movzx eax, word [ebx+0x6c]     ; ← 件数 = obj+0x6C = DATA+0x14
0x41629F  test ax, ax / je 0x41633a      ; 0 なら何もしない
0x4162A8  movsx esi, ax; shl esi, 2      ; bytes = count * 4
0x4162B2  mov ecx, 0xB02020; call 0x401E20 ; 配列確保
0x4162BC  mov [ebx+0x9c], eax            ; 配列ポインタ格納
0x4162C8  push esi; push eax; mov ecx,edi; call 0x450C20   ; Read(配列, count*4)＝ESCE から読出
```

→ **DATA+0x14（word）＝関連効果件数、その formID 配列は DATA に無く ESCE サブレコードに在る**。runtime 生成側も同構造（`FUN_00417220` が件数 arg を見て `call 0x415D40`、同関数が `mov word [esi+0x6c], bx` と `[esi+0x9c]` を書く＝v235 の「関連効果配列＋未解明 36B」のうち**配列部分は確定**）。
port への示唆: port の `decodeMagicEffect` は **ESCE を一切読まない**（grep: ESCE は esm_reader に非在＝esm_reader.cpp の DATA 分岐のみ）。＝**CUDI/CUPO の「関連効果を消す」挙動（§2-2 #18）を再現するなら ESCE 読出が必要**。

### §5-3 +0x74 / +0x90 / +0x94 の供給源

| 項目 | 供給源（実測） | 内容 |
|---|---|---|
| **+0x74 = 1.0f** | ①ctor `fld1`（0x4164A9-0x4164B1）②**ローダも memset の直後に `fld1; fstp [ebx+0x74]`**（0x416199-0x41619E、ReadData の**前**） | 1.0f。ただし直後の `ReadData(obj+0x58, 0x40)` が **DATA+0x1C で上書き**するため、64B DATA が在れば ESM 値が優先。runtime 生成系（0x417220）は上書きしないため **1.0f のまま残る** |
| **+0x90** | `fld [0xB33704]`。参照は `.text` に3箇所のみ（ctor 0x416514／CRT 登録 0x9DA740／getter スタブ 0xA178F0） | **GMST `fMagicDefaultCEEnchantFactor`**（0x9DA740: `fld1 → fstp [esp]; mov ecx,0xB33704; push "fMagicDefaultCEEnchantFactor"; call 0x41BAE0`＝float GMST 登録ヘルパ・**既定 1.0f**） |
| **+0x94** | `fld [0xB3370C]`（同型の参照3箇所） | **GMST `fMagicDefaultCEBarterFactor`**（0x9DA770・**既定 1.0f**） |

- 補足: `.data` のファイル上 0xB33704/0xB3370C の値はいずれも **0.0**（CRT init が 1.0f を書く前の値）。隣接 0xB336FC = **`fChameleonMinRefraction`**（同じ float GMST 帯＝0xB336xx/0xB337xx は GMST スロット）。
- **語義の推定（未確証）**: "CE" は enchantment（作成 enchant のコスト係数）と barter（売却価格係数）を指すとみられるが、**EffectSetting がなぜこの2係数を保持するのかは未解明**（§6 N-10）。

### §5-4 obj+0x6C..+0x97 の取扱い実態（EffectSetting コードクラスタ 0x412F00-0x41C400 の disp走査）

capstone detail による `disp ∈ {0x6C..0x97}` の全走査（`v236_slots2.py` → `v236_slots2_out.txt` 493ヒット）から **EffectSetting 本体に関係するものだけ**を抽出:

| VA | 内容 | 意味 |
|---|---|---|
| 0x41583F-0x415928 | `[edi+0x70..+0x8C]` → `[esi+...]` の7 dword ＋ `movzx [esi+0x6c]` | **代入（CopyFrom / operator=）** |
| 0x415C4F-0x415CF4 | `[edi+X]` と `[esi+X]` の **等値比較**（dword 7個＋`fld/fld` で float +0x74/+0x90/+0x94、word +0x6C） | **operator==** |
| 0x416056-0x4160BC | 同ブロックの再代入（+0x90/+0x94/+0x74 を float で） | **operator=** |
| 0x415D40 / 0x415DC0 | `mov word [esi+0x6c], bx` ＋ `[esi+0x9c]` アロケータ | **関連効果 setter**（runtime・ESCE の双方が使用） |
| 0x41619E / 0x41629B | `fstp [ebx+0x74]`（1.0f）／`movzx [ebx+0x6c]` | ローダの既定設定／ESCE 件数読出 |

→ **gameplay 判定（分岐・コスト・magnitude）でこのブロックを読む命令は EffectSetting クラスタ内に存在しない**＝「CS 保存用 tail を生コピーし、engine 内では複製・一致比較にのみ使用」。ただし **0x612/0x613/0x68E/0x5EE 帯にも同一 disp のヒットが多数ある**が、これらは **MagicItem/SpellItem/Actor 等の別構造体**（`[esi+0x70]` がリンク先、`[esi+0x90]` が type code 等）であり **オフセット一致の偶然**。誤読防止のため明記する。

### §5-5 port への影響（[PORT] 材料）

1. port は DATA を**20 バイト分しか読まない**（esm_reader.cpp:1257 `size() >= 20`）＝**+0x14 以降（件数・POD tail）と ESCE が丸ごと未読取**。現状の port 消費者は school/baseCost/av/flags のみで**実害ゼロ**。
2. 将来 (a) CUDI/CUPO 相当の「関連効果を消す」挙動、(b) engine 同等の EffectSetting 等価比較を再現する場合に限り、**ESCE 読出＋件数 word が前提**になる（§2-2 #18 の実測が根拠）。
3. `baseMagnitude/baseDuration/range`（esm_reader.h:429-431）は **DATA に存在しない列**（+0x14.. には件数 word と POD が在るだけで、magnitude/duration/range の値は DATA に無い）＝**未代入の死フィールド**。項目1 の [PORT] 判断 (iii) とセットで扱う。

---

## §6 新規残点（本タスクで新たに発生）

| ID | 内容 | 優先度 |
|---|---|---|
| N-9 | **TODO.md / plan.md / supermemory の「2026-10-30 実装」日付が実態と不一致**: ai-todo #26/#29 の実編集は **2026-11-01 20:51:18-20:52:22 JST**（Copilot events.jsonl＋ファイル mtime の二重証拠）、host tests 実行も 11-01 21:08:59。注記・記憶の「2026-10-30」は**日付誤り**。docs（TODO/plan/記憶）は要承認で訂正 | 低（doc 訂正要・要承認） |
| N-10 | **MGEF DATA tail POD（obj+0x70..+0x94）の意味論未解明**: +0x90/+0x94 が GMST `fMagicDefaultCEEnchantFactor`/`fMagicDefaultCEBarterFactor`（既定1.0f）で初期化される理由、+0x18..+0x37 の7 dword が何か。engine 内では copy/assign/operator== のみで gameplay 未使用（§5-4）＝**UESP が unknown と記す領域**。UESP 404 のため EXE 以外の裏付けなし | 低 |
| N-11 | **`FUN_009FEA50` スチューブ列の登録先が未解明**: 各スチューブは fourcc 判定→`call 0x68DA10`（コレクション **0xB15A0C** への insert）→byte グローバル **0xB3C0AA..0xB3C0EB** に1を格納。0xB15A0C の集合体と flag の用途（UI/状態判定？）が未特定 | 低 |
| N-12 | **port が MGEF の 'ESCE'（関連効果 formID 列）を読まない**: engine は件数 word（DATA+0x14）＋ESCE から配列を構築し、**CUDI/CUPO がそれを消費**（§2-2 #18・§5-2）。port に Cure Disease/Cure Poison 挙動を作るなら ESCE 読出が前提。現状は consumers ゼロで実害なし | 低〜中（cure 挙動実装時） |
| N-13 | **`MagicEffectData.baseMagnitude/baseDuration/range` の3フィールドが DATA に存在しない列（死フィールド）**: 未代入のまま宣言のみ（esm_reader.h:429-431）。削除するか spec を定めるかは [PORT] | 低 |
| N-14 | **v235 N-7 の対象ファイル名誤り**: 「oblivion_ini.cpp（find_by_key コメント訂正）」は実際には **oblivion_ini_defaults.h**（oblivion_ini.cpp は mtime 2026-10-25・未編集）。v235 §5 の記述訂正要（本体は既存 doc・無編集） | 低（doc 訂正要・要承認） |

（N-4..N-8 の扱い: **N-4 は §1 で解消＝ドラフト提示、N-5 は §2 で解消、N-6 は §3 で解消（適用は要承認）、N-7 は §4 で解消、N-8 は §5 で構造面を解消（意味論のみ N-10 として残す）。**）

---

## §7 検証情報

- **selftest**: 着手時に `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（bytes×2 / string / opid）。
- **EXE**: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（PE 手動解析: image_base 0x400000 / .text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00）。capstone 5.0.7（skipdata＋detail）。
- **本タスクの再逆アセンブル対象**: 0x416100-0x416360（MGEF ローダ＝DATA/ESCE/EDID 分岐・+0x74=1.0f 再設定）/ 0x416430-0x416560（ctor 既定値全数）/ 0x417220-0x417420（runtime 生成）/ 0x415D40-0x415E44（関連効果 setter）/ 0x412F00-0x41C400・0x5EE000-0x5EF200・0x612000-0x614200・0x68E000-0x68F200・0x5F4000-0x602000（disp 0x6C..0x97 走査）/ 28関数の fourcc ヒット前後窓（0x48/0x40）/ 共通ヘルパ 0x5E91E0・0x613810・0x68DA10 / GMST 登録 0x9DA700-0x9DA7C0・0xA178C0-0xA17940 / .text 全域の 0xB33704・0xB3370C 参照走査。
- **スクリプト（新規のみ・`%TEMP%\opencode\`）**: `v236_ctor.py`(+`_out.txt`) / `v236_slots.py` / `v236_slots2.py`(+`_out.txt`) / `v236_fourcc28.py`(+`_out.txt`) / `v236_fourcc28b.py` / `v236_fourcc28c_out.txt`（圧縮版）/ `v236_loader_out.txt` / `v236_dispatch_out.txt` / `v236_ctor_tail.txt` / `v236_helpers_out.txt` / `v236_refs.py` / `v236_regname.py` / `v236_db_*.py`（opencode DB read-only 走査8本）/ `v236_events_*.py`（Copilot events.jsonl 走査5本）。v235 の `v235_disasm.py`・`v235_fourcc_funcs_out.txt` は**再利用（改変なし）**。
- **git（read-only のみ）**: `git status --porcelain` / `git diff -- <6 files>` / `git diff --stat <6 files>` / `git diff --stat 1e3c4651 -- <6 files>` / `git log -1 -- <file>` / `git log --all -S "<string>" -- <file>` / `git show --stat --format=fuller 10c30800|1e3c4651` / `git for-each-ref --contains` / `git reflog -15` / `git branch -a` / `git worktree list` / `git stash list`（空）。**commit/add/push/stash/checkout/reset は一切未実行。**
- **セッション記録（読取のみ）**: `C:\Users\hiroki.kogarumai\.local\share\opencode\opencode.db`（mode=ro で open・全表走査）／`C:\Users\hiroki.kogarumai\.copilot\session-state\9427ac74-...\events.jsonl`（604MB・全行走査）／`.ai-todo\state\audit.log`・`TODO.md`／`%TEMP%\opencode\archive_re_sessions.py`（読取）。
- **移植ソース（読取のみ・編集なし）**: `assets/esm_reader.h/.cpp` / `game/gmst_defaults.h` / `jni_bridge.cpp` / `system/settings_manager.h` / `system/oblivion_ini_defaults.h` / `system/oblivion_ini.cpp` / `game/alchemy_system.cpp` / `game/spell_manager.cpp` / `tests/combat_formulas_tests.cpp`。
- **RE 成果物（読取のみ）**: `tmp/re/v235_port_alchemy_settings_residuals.md`（本体・§5）/ `v218_ini_docs_bundle3.md`（§5 L248-275）/ `v216_opcode_gt_names7.md`（§2.1-2.2 mgefFlags ビット表）/ `v221_opcode_gt_names8.md` / `TODO_re49.md`（serial 207/215 行）/ `tmp/re/cache/named_funcs.txt`・`call_targets.json`・`README.md`。
- **ESM 本体: 未アクセス**（再スキャン禁止を遵守）。web 資料の取得も本タスクでは未実施（N-8 は EXE 実測のみで完結）。
- **git 差分の内容（§4-3）**: `git diff` で6ファイル +19/−32 を全数取得、`git diff 1e3c4651 -- <6 files>` が空＝checkpoint と一致することを確認。
