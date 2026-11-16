# v407 — `0x6CAA4A` の `Offset=[+0x54]−st1` の st1 出所の裁定（#153 ⑥-11・V-388-2）

- 採番: **v407**（ai-todo #153 束・probe prefix **v407_1・v407_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v388 の残点 **V-388-2（`0x6CAA4A` の `Offset=[+0x54]−st1` の st1 の出所）【低】** を消化。出発点は v388_seq_fields_ruling.md / v404_seq54_ruling.md（state2 出口の Offset 変換として0x6CAA4A を確定済み）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x6CAA4A` の st(1) の出所** | **[確定(命令)]** | **st1 ＝ `FUN_006ca950` の第2引数 `param_2`（＝state machine へ渡された現在時刻 time）** — prologue **`0x6CA970: fld dword ptr [esp+0x10]`**（thiscall 第2引数）で st0 へ積まれ、**switch 全経路を通じて st0 として維持**（途中の `fucompp` 比較系は st0/st1 を消費した後 `param_2` が st0 に復帰する構造）され、case5 の `0x6CAA4A: fld [esi+0x54]` で **st0=[+0x54] / st1=param_2** となった直後の `0x6CAA4D: fsub st(1)` → `0x6CAA4F: fstp [esi+0x48]` が **`Offset(+0x48) = [+0x54] − param_2(time)`** |

## §2 証拠（v407_1 / v407_2）

### §2.1 対象ブロック（`FUN_006ca950` 内 switch case5・machine 0x6CAA36..0x6CAA5D）
```
6CAA36: fld dword ptr [esi + 0x54]    ; st0=[+0x54], st1=param_2
6CAA39: fld dword ptr [0xa7deb4]      ; st0=FLT_MAX, st1=[+0x54], st2=param_2
6CAA3F: fchs                          ; st0=-FLT_MAX
6CAA41: fucompp                       ; [-FLT_MAX] vs [+0x54] → pop2（st0=param_2 復帰）
6CAA45: test ah, 0x44 / jnp 0x6CAA5D  ; 相等＝未設定(−FLT_MAX) なら skip
6CAA4A: fld dword ptr [esi + 0x54]    ; ★ st0=[+0x54], st1=param_2
6CAA4D: fsub st(1)                    ; ★ st0 = [+0x54] − param_2
6CAA4F: fstp dword ptr [esi + 0x48]   ; Offset = ...
6CAA52: fld [0xa7deb4] / fchs / fstp [esi+0x54]  ; [+0x54] = -FLT_MAX クリア
6CAA5D: mov dword [esi+0x44], 1       ; state=1 へ遷移（v404 残点 V-404 とは独立）
```
decompiled 対応（L499586-49989 case5）: `if (-_DAT_00a7deb4 != [+0x54]) { [+0x48] = [+0x54] - param_2; [+0x54] = -_DAT_00a7deb4; } [param_1+0x44]=1` — **machine の `fsub st(1)` の st1 は C の `param_2` に1:1 対応**。

### §2.2 st1 の供給源（prologue・v407_2）
```
6CA950: sub esp,8 / push esi / mov esi,ecx      ; thiscall: this=esi=sequence
6CA961: fld [esi+0x48]; fld -FLT; fchs; fucompp ; Offset 未設定チェック
6CA970: fld dword ptr [esp + 0x10]              ; ★ param_2（time 引数）を st0 へ積む
6CA974: test ah,0x44 / jp 0x6CA980
6CA979: fld st(0); fchs; fstp [esi+0x48]        ; Offset=-time（遅延初期化・v403 整合）
6CA980: fld [esi+0x4c]; fld -FLT; fchs; fucompp ; Begin 未設定チェック（pop2 → st0=param_2 復帰）
6CA994: fst [esi+0x4c]; fld [esi+0x50]; fadd st(1); fstp [esi+0x50]   ; End += param_2（st1=param_2 使用）
6CA99F: fld1 / lea eax,[ecx-1]; cmp eax,5; fst [esp+4]; fstp [esp+8]  ; st0=param_2 復帰
6CA9B5: jmp dword [eax*4 + 0x6CAC40]            ; switch jump（st0=param_2 持ち越し）
→ case5 エントリ 0x6CA9FB: fld [esi+0x50]; fcomp st(1)（st1=param_2）→ 0x6CAA36 → 0x6CAA4A
```
**維持経路の検証**: 各 `fld` は st0 に被せるだけで `fucompp`/`fcomp`/`fstp` の pop がその後に来るため、**st1（param_2）は switch 入口まで一切破壊されない**。`0x6CA99A fadd st(1)` が param_2 を使う箇所（End += time 更新）も含め、デコード範囲全体で矛盾なし。

## §3 残点 **V-407-1..2**
1. 【低】case5 エントリ `0x6CA9FB` の「`[+0x50](End) ≤ time` 時のみ Offset 変換に入る」分岐が state machine のどの状態遷移に対応するか（`[esi+0x44]` が state フィールドであることは `[+0x44]=1` 書込から [推定高]、遷移表の全状態名は v403/v404 の state 系残点と統合して再調査要）。
2. 【低】`0x6CA994` の `End(+0x50) += param_2` 更新（Begin 未設定時の遅延初期化帯）の正確な意味（v403 の Offset 初期化系と同族か）。

## §4 検証・自己批評
- probe: **v407_1・v407_2 全 EXIT=0**。
- **決定打の作り**: (i) 対象 machine の UNFILTERED 線形 decode（fld/fucompp のスタック深さを手計算で追跡）→ (ii) decompiled case5 との1:1 突合（`param_2`↔st1）→ (iii) **prologue まで遡って最初の `fld [esp+0x10]` を特定**し「switch 全経路で st0=param_2 維持」を pop 操作の順序で証明。
- 妥当性チェック: ①C の `Offset = [+0x54] − param_2` ⇔ `fsub st(1)`（st1=param_2）— ②`[-FLT_MAX] 比較` ⇔ `fucompp` パターン — ③`[+0x54] クリア` ⇔ `fld -FLT/fchs/fstp` — ④遅延初期化 `Offset=-time`（v403）⇔ `0x6CA979` — **四方向矛盾なし**。
- docs との不整合: **なし**（v388 V-388-2／v403・v404 の state2 出口記述を機械レベルで補強）。
- **教訓**: **x87 スタック出所の追跡は「prologue の最初の fld から pop 操作順に手計算」で追う**— `fucompp`/`fcomp`/`fstp` の pop 回数を数えると、どの時点まで st(n) が生きているかが機械的に確定する。case 入口の `fcomp st(1)` が「スタックが既に1段積まれている」ことの証明になる。
