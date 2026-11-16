# v416 — `FUN_00700010`（map lookup）と `Bip01[+0x1C]` の identity の裁定（V-415-1 完遂・派生残点第8弾）

- 採番: **v416**（派生残点群・probe prefix **v416_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v415 の残点 **V-415-1【中】（`Bip01[+0x1C]` の正体＝`FUN_00700010` identity）** を消化し、**V-405-3 を [確定相当] まで到達**。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_00700010(this, key)` の identity** | **[確定(命令＋decompiled)]** | **マップ lookup（hash/list 形）**: `node = [this+0xC]`（bucket/リスト先頭）→ 外側ループで node を辿り（**`node = node[0xd]`（+0x34）＝次ノード**）、内側ループで **`list = node->virtual+4()` → `[+4]` next 走査** し **`list == key`（pointer 等値）ならその node を return**、尽きれば **0**（L541175-541193）。E8 callers **15件超（cap）**＝汎用 utility（`FUN_006c8220`＝NiTransformController 確認 fn・`FUN_006c6910`・`FUN_00473200`/`FUN_0047F750`/`FUN_00480770`/`FUN_00539A40` 等） |
| **epilogue の呼び出し（v415②の継続）** | **[確定(命令)]** | **`FUN_00700010(this = Bip01[+0x1C], key = 0xB3CD7C)`** — decompiled `puVar3 = FUN_00700010(&DAT_00b3cd7c)`（stack arg 表示）⇔machine `mov ecx,[esp+0x2c]（=Bip01[+0x1C]）; push 0xB3CD7C; call`。取得 node を **`InterlockedIncrement(node+1)`（NiPointer refcount）** して以降使用（L496379-496383）＝**取得物は refcounted オブジェクト** |
| **`Bip01[+0x1C]` の正体（V-415-1 主問）** | **[確定(命令)]** | **key→NiPointer のマップ/登録簿（NiTMap 系）**: `[this+0xC]`＝bucket・node は `[0..]` リスト（`virtual+4()` → `[+4]` next）＋`[0xd]`＝次 node・**key は pointer 等値比較** → **`NiAVObject[+0x1C]` は「（某キー→refcounted オブジェクト）の map」**。StoreTargets epilogue は **この map から `0xB3CD7C`（global object のアドレス）を key に node を取得**して NiPointer 扱いする [確定] |
| **`0xB3CD7C` global の正体** | **[未確定]**（残点 V-416-1） | `.data` 値は不定（実行時）・**.text refs 12件**: float 系ヘルパ帯 `0x473181/0x4732EB/0x47C9B4/0x47CA51/0x47CC96/0x47F763`・`0x4969E1`・`0x4E3EC7`・sequence band `0x6C5F51/0x6C693E/0x6C8252`・`0x6D0460` — 「何の登録簿の key か」は未同定 |
| **V-415-2（L498845 slot23 呼び出し）** | **[未確定]** | 本 probe 対象外で引継ぎ |

**V-405-3 への総合回答（v415+v416）**: ①「sequence layout を持つ矛盾」＝Ghidra ecx 落ち表示誤りで**不存在 [確定]** ②epilogue は `prereq==0` 時に `palette Get(Bip02)→Get(Bip01)→Bip01[+0x1C] の map から global 0xB3CD7C の node を NiPointer で取得 → `FUN_006c6910(seq, node)` [確定] ③`0xB3CD7C` の正体と slot23 は残点。

## §2 証拠（v416_1）

### §2.1 `FUN_00700010` body（L541175-541193）
```c
int *FUN_00700010(this, key) {
  node = *(this + 0xc);
  do {
    if (!node) return 0;
    for (v = node->virtual+4(); v; v = *(v+4))      // value list walk
      if (v == key) return node;                    // ★ pointer 等値 match    node = node[0xd];                               // ★ next node (+0x34)
  } while (true);
}
```

### §2.2 epilogue の利用（v415(v) と本 probe の突合）
- machine（v415_1）: `mov ecx,[esp+0xc]（seq）; push eax（Bip01[+0x1C]）; call FUN_006c6910`。
- fun 内（v415_1(c)）: `mov ecx,[esp+0x2c]（＝Bip01[+0x1C]）; push 0xB3CD7C; call 0x700010`。
- decompiled（L496379-383）: `puVar3 = FUN_00700010(&DAT_00b3cd7c); if (puVar3) InterlockedIncrement(puVar3+1);` ＝**refcounted 取得**。

### §2.3 `0xB3CD7C` refs（12件）
`0x473181` `0x4732EB` `0x47C9B4` `0x47CA51` `0x47CC96` `0x47F763` `0x4969E1` `0x4E3EC7` `0x6C5F51` `0x6C693E` `0x6C8252` `0x6D0460` — float ヘルパ帯（`0x47C930`＝v405 の NiNode float 伝播ヘルパ隣接）と sequence band に集中。

## §3 残点 **V-416-1..2**
1. **【中】`0xB3CD7C` global の正体** — refs 使用形（特に `0x47C9B4` 帯と `0x6C5F51`）から「何の key（＝何の登録簿 entry）か」を同定。NiFixedString static か float 系 singleton か data registry か。
2. **【低】V-415-2 引継ぎ**: `L498845 [manager+0x30]` deref の `slot23(+0x5C)` 呼び出しと `manager[+0x30]` 正体（v406 の Target/Node 候補 [部分確定] と統合）。

## §4 検証・自己批評
- probe: **v416_1 全 EXIT=0**（1本で V-415-1 完遂）。
- **決定打の作り**: (i) **fn の decompiled 実読（map lookup の2重ループ構造）** → (ii) **E8 callers census（汎用 utility の確認）** → (iii) **global refs census（key 使用者の帯分布）** → (iv) v415 の machine/decompiled 突合（this=stack arg1・key=global addr）。
- 妥当性チェック: ①map lookup body⇔`InterlockedIncrement`（refcounted node 取得）⇔v415 の `push 0xB3CD7C` — ②`[this+0xC]`/`[+0xd]` 布局⇔NiTMap 系（v405 の palette map `NiTMapBase` と同型）— ③E8=15+⇔汎用 utility — **三方向矛盾なし**。
- docs との不整合: **なし**（v405 ruling の V-405-3 を v415+v416 で[確定相当]に到達・docs/GT 反映時は v405 の残点表を更新・#47/#20 一括）。
- **教訓**:
  1. **「fn identity」は decompiled のループ構造（外側 node 走査＋内側 value list 走査＋等値比較）で即座に map lookup と判定できる** — 名前なしでも構造が語る。
  2. **global の「key 化」（アドレスが pointer 等値比較の key になる）パターン** — `&DAT_xxx` を map の key に使う構造は「その global が singleton 登録簿の目印」として機能する。
  3. **refcount increment（`InterlockedIncrement(node+1)`）が直後にある取得系呼び出しは「NiPointer 取得」の定番**（v404/v405 の refcount 読み解きを再利用）。
