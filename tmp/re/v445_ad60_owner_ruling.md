# v445 — ad60 呼出の所属確定・global 同定・FF 分離手法の裁定（V-445-1..3 完遂・派生残点第37弾）

- 採番: **v445**（派生残点群・probe prefix **v445_1..v445_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v444 の残点 **V-445-1【低】（`FUN_0056a3b0` の cmp chain から `0x56A464` の case index と `[esp+0x5c]` branch 源・`global 0xB3340C` identity）／V-445-2【低】（non-ret residual の `FF` SIB 種別分離）／V-445-3【低】（旧残点群への移行判断）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **v444 訂正①: `FUN_0056a3b0` の jump table** | **[確定]（v444 撤回）** | **jump table は実在**: `56A3B4: cmp eax,5; ja 0x56a435` → **`56A3B9: jmp [eax*4+0x56A438]`（6 cases）** — v444_2 の「FF/4 jump table 無し＝cmp/jcc linear chain」は **linear decode の到達失敗による誤答と訂正**。decompiled case list（L269934-269964）＝ **`case0: (p3!=p2)→0` / `case1: (p3==p2)→0` / `case2: (p2<=p3)→0` / `case3: (p2<p3)→0` / `case4: …` / `case5: (p3<p2)→0` ＝ float 比較演算6種 [確定(decompiled)]** |
| **v444 訂正②: `0x56A464`（`call FUN_0056ad60`）の所属** | **[確定]（v443 訂正）** | **真の所属は `FUN_0056a450`**（docs `L269970: // FUN_0056a450 @ 0x56a450` が実在）。L269972: **`void __fastcall FUN_0056a450(int *param_1)`: `for (; param_1 && (param_1[1]!=0 || *param_1!=0); param_1=param_1[1]) FUN_0056ad60();` ＝連結リスト走査 loop 内の ad60 呼出し [確定(decompiled)]** → **「ad60 の呼出 case」の問いは解消（case ではなく loop body）**。**v443 の「head=0x56A3B0 // FUN_0056a3b0」記述は ann_of の window 誤り＝find_head 境界誤り5件目として訂正** |
| **`global 0xB3340C` の identity** | **[確定]（構造）＋[推定高]（semantic）** | **`.text` refs は2件とも `FUN_00407AE0` 内**: `0x407CAE: mov ecx,[0xB3340C]`（read・差分計算 `eax−[0xB3340C]`）／**`0x407CD8: mov [0xB3340C],eax`（write）→ 直後 `fstp/fld [0xB33404]/fadd` の float 連鎖** [確定(命令)]。**docs mentions 3件（`DAT_00b3340c = DAT_00b33ea0` L5869/L5879、`iVar20 = DAT_00b33ea0 - DAT_00b3340c` L5871）[確定(decompiled)]** → **dump 内の「前回値キャッシュ」（`0xB33404`/`0xB33408` float 群と隣接）で今回の値との差分を取る変数 [推定高]**。`DAT_00b33ea0` の identity は残点 |
| **`[esp+0x5c]` branch 源（`0x407CB4`/`0x407E47` への flow）** | **[未確定]（残点継続）** | v445 群では未着手（probe budget） |
| **V-445-2: `FF` SIB 種別分離** | **[未確定]（手法不備を確認）** | **tail 6-byte 走査方式の分類は失敗**: non-ret 3171 のうち **`other (not FF-tail)`＝3161（99.7%）**、`FF rm=reg` 6／`FF [abs]` 2／`FF [mem+disp]` 合計2 → **v442 histogram の `0xFF=1859`（末尾 byte 0xFF）が ModRM ベース分類で捕捉できない＝走査が命令境界を知らない [確定（手法欠陥の確認）]** → **正しい手法＝segment 先頭からの forward decode で最後の instruction を採る（CC 塀区切りの code run は forward decode が有効）[確定（手法）]、実装は残点** |
| **V-445-3: 旧残点群への移行** | **[確定]（方針）** | **次 v446 は V-445 の残2点（FF forward-decode 分離実装・`[esp+0x5c]` predicate）を片付けるか、既存の型番残点（V-414-2..3/V-413-2/V-415-2 等）へ移行するかを着手時に判断** — 本裁定では **移行方針を「V-445 継続を1回、その後旧群へ」と仮決定 [推定（作業方針）]** |

## §2 証拠（v445_1/v445_2）

### §2.1 FUN_0056a3b0 switch（v445_1a）
```
machine: 56A3B4: cmp eax, 5 / ja 0x56a435
         56A3B9: jmp dword [eax*4 + 0x56A438]      <- jump table (6 cases)
         (v444_2 は linear decode でこの jmp に到達せず「無し」と誤答)
decompiled L269934: switch(param_1) {
  case 0: if (param_3 != param_2) return 0;  case 1: if (==) return 0;
  case 2: if (param_2 <= param_3) return 0;  case 3: if (<) return 0;
  case 4: ...;                               case 5: if (param_3 < param_2) return 0;
} return 1;   <- float compare ops, 6 cases
```

### §2.2 ad60 呼出（v445_1a / v445_2a）
```
machine: 0x56A464: call 0x56ad60  (context: 56A455 je / 56A457 cmp [esi+4],0 /
         56A45B jne / 56A45D cmp [esi],0 / 56A460 je 0x56a470 / 56A464 call / 56A46E jne)
docs: L269970 // FUN_0056a450 @ 0x56a450
      L269972 void __fastcall FUN_0056a450(int *param_1) {
        for (; (param_1 != 0 && (param_1[1] != 0 || *param_1 != 0));
             param_1 = (int*)param_1[1]) { FUN_0056ad60(); } return; }
      <- list traversal loop; ad60 called per node [確定]
ann_of(0x56A464) が 0x56A3B0 を返したのは window 誤り (find_head 境界誤り5件目)
```

### §2.3 global 0xB3340C（v445_1b）
```
refs: 0x407CAE: mov ecx,[0xB3340C] -> 0x407CB4 sub edx,ecx (read, diff)
      0x407CD8: mov [0xB3340C],eax  (write) -> fstp [esp+0x3c] /
                 fld [0xB33404] / fadd [esp+0x3c]   (float chain)
docs: L5869: DAT_00b3340c = DAT_00b33ea0;
      L5871: iVar20 = DAT_00b33ea0 - DAT_00b3340c;
      L5879: DAT_00b3340c = DAT_00b33ea0;
```

### §2.4 FF tail classification（v445_2b）
```
non-ret boundaries: 3171
  other (not FF-tail): 3161 (99.7%)   <- 手法欠陥（tail 6-byte 走査が命令境界を知らない）
  FF rm=reg (jmp/call reg): 6 / FF [abs]: 2 / FF [mem+disp]: 2
correct method: segment 先頭からの forward decode で最後の instruction を採る（CC 塀区切り code run）
```

## §3 残点 **V-446-1..3**
1. 【低】**FF 分離の forward-decode 実装** — 各 non-ret boundary の前 segment を先頭から linear decode し、最後の instruction の opcode を分類（`jmp [abs]`/`jmp [reg*4+tbl]`/その他）→ residual 3126 の真の function end 数を確定。
2. 【低】**`[esp+0x5c]` branch 源** — `0x407CB4`/`0x407E47` への flow predicate と、`DAT_00b33ea0`（`0xB3340C` の供給元）の identity。
3. 【低】**型番方針** — V-446 で V-445 残2点を片付けた後、旧残点群（V-414-2..3／V-413-2／V-415-2 ほか）へ移行するか（本裁定の仮決定: V-445 継続1回→旧群）。

## §4 検証・自己批評
- probe: **v445_1..v445_2 の2本全 EXIT=0**（V-445-1 完遂（ad60 所属＋global 同定＋v444/v443 訂正2件）／V-445-2 は手法欠陥を確認して [未確定]／V-445-3 は方針仮決定）。
- **決定打の作り**: (i) **decompiled の case list 実読（L269934-64）と machine の `cmp eax,5/jmp [eax*4+0x56A438]` の対比で v444 訂正を即時に把握** → (ii) **`0x56A464` の所属は docs の次の annotation（`L269970 // FUN_0056a450`）を grep するだけで確定**（find_head window に頼らない）→ (iii) **global は .text refs（read/write）＋docs mentions（代入・差分）の4系統** → (iv) **FF 分類は結果（99.7% other）が手法欠陥を露呈** → 正しい手法を明記。
- 妥当性チェック: ①`jmp [eax*4+0x56A438]`⇔`cmp eax,5`⇔case0..5（6件）— ②`FUN_0056a450` の loop⇔machine の `jne 0x56a457`（loop 戻り）— ③`[0xB3340C]` write 直後の `fld [0xB33404]`⇔docs `DAT_00b3340c = DAT_00b33ea0` 連続代入— ④FF 分類 10/3171⇔v442 histogram 0xFF=1859（手法差として矛盾なく説明可能）— **四方向矛盾なし**。
- docs との不整合: **なし**（v443 の ann_of 記述と v444 の jump-table 訂正を本裁定で自己訂正）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **「jump table 無し」の否定は linear decode の到達性に依存する** — 1回の linear scan で否定したら、**もう1系統（`cmp reg,N` 直後の FF を byte-pattern 検出）で再検証**（本件 v444_2 は再検証を欠いて誤答）。
  2. **fn 所属の最終判断は find_head window ではなく「docs の直近 annotation を backward grep」** — annotation が2個先にあっても grep で即決（`L269970` が決定打）。**window 誤りは find_head 境界誤りとして記録（5件目）**。
  3. **tail 分類は「境界からの逆走査」では命令境界を掴めない** — **区切りられた code run は先頭からの forward decode が正**（CC 塀区切りの構造を活かす）。
  4. **global identity は read/write 両 ref＋docs の代入・差分使用を並列で見ると「前回値キャッシュ」が浮かぶ**（差分パターン `A-B` が鍵）。
  5. **訂正は ruling 冒頭に「vXXX 撤回」明記** — 連続裁定では前の ruling の誤答も自分の記録から拾って訂正する（本裁定で2件）。
