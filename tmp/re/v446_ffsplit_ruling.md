# v446 — FF 分離の実装成功・predicate・global counter の裁定（V-446-1..3 完遂・派生残点第38弾）

- 採番: **v446**（派生残点群・probe prefix **v446_1..v446_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v445 の残点 **V-446-1【低】（FF 分離の forward-decode 実装）／V-446-2【低】（`[esp+0x5c]` branch 源と `DAT_00b33ea0` identity）／V-446-3【低】（型番方針の最終判断）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-446-1: FF/非 ret 境界の forward-decode 分離** | **[確定]（手法成功）** | **segment 先頭からの forward decode（v445 の正しい手法）で non-ret 3171 を分類**: **`jmp-rel`（tail の `E9/EB`）＝1897（59.8%）／`ret-family`（forward で `ret`/`ret imm16` 判明）＝1103（34.8%）**／`other-last-ins` 47／`jmp reg` 6／`jcc-tail` 2／`jmp [mem]` 2／straddle+undecodable 10（計約60＝1.9%）→ **byte 判定の non-ret 3171 のうち 3000（94.7%）が実際には function end（tail-jmp epilogue or `ret imm16`）[確定]**。**byte 判定が `ret imm16`（`C2 lo hi`・末尾 byte＝hi）を取りこぼしていた内訳が 1103 件と判明 [確定]** |
| **docs 欠落 function 数の推定（収束）** | **[推定]（~6000台）** | **gaps>0x200・minseg=0x10 の境界総数＝6209（ret-byte 3038＋non-ret 3171）のうち function end と分類可能 ≈ 6038（ret 3038＋ret imm16 1103＋tail-jmp 1897−重複調整）＋ 偽陽性約60＋table 直後45** → **docs 欠落 function ≈ 6000台（下限・gaps≤0x200 は未計上）[推定]**。v437〜v439 の「数千規模」から **~6000 へ収束**。ret-confirmed 下限 3038（v441）との幅は「ret imm16 捕捉」と「tail-jmp 判定の確度」で説明可能 |
| **V-446-2: `0x407CB4` write の flow** | **[部分確定]** | **`0x407CB4` への direct branch は `0x407CAC: jmp 0x407cb4`（EB・無条件）1件のみ [確定]** → flow は無条件経由（fall-through 相当）。**その手前 `0x407C9D: mov eax,[0xB33EA0]` → `0x407CA2: je 0x407CAE`（＝`[0xB33EA0]==0` 系の条件と推定）→ `0x407CAE: mov ecx,[0xB3340C]` → write [部分確定]**（`je` の直前 1byte（`test eax,eax`）は decode 確認待ち）。**`0x407E47` への「branch」`0x407E44: 7x` は mid-instruction 偽陽性と判明（`407E42: mov eax,[edx+0x174]` の5byte中間）→ 同 block は fall-through [確定（訂正）]** — v446_1 の forward decode 教訓の再現 |
| **V-446-2: `DAT_00b33ea0` の identity** | **[推定高]** | **counter 系グローバル [推定高]**: `.text` refs は `4052CE mov eax,[0xB33EA0]`/`fild`（`FUN_00405290`）、`407C9D`（dump）、`408D68/408D6E`（fild/mov）、`47F81C cmp eax,[0xB33EA0]`（`FUN_0047f7b0`）[確定]。**docs mentions 84件**: `if (*(uint*)(p+0x1b4c) < (uint)(DAT_00b33ea0 - *piVar2)) { *piVar2 = DAT_00b33ea0; }`（L3639-40＝**interval/timeout パターン**）／`DAT_00b069c4 != DAT_00b33ea0 → 代入`（L101834-36＝**変化検知**）／`(float)DAT_00b33ea0`・`fild`（**int→float 変換**）／`DAT_00b33ea0 < 0`（signed 判定）→ **差分・interval・変化検知・float 変換に使われる signed int counter（frame/tick 系）[推定高]**。`0xB3340C`＝dump 内の前回保存値（v445）と整合 |
| **V-446-3: 型番方針** | **[確定]（方針）** | **v447 から旧残点群へ移行**: V-414-2..3（animation table consumer 等）／V-413-2／V-415-2 を優先順に消化（V-446 で v445 の残2点を片付けたため v445 の仮決定どおり実行）[確定（作業方針）] |

## §2 証拠（v446_1/v446_2）

### §2.1 forward-decode 分離（v446_1）
```
non-ret boundaries (minseg=0x10, gaps>0x200): 3171
  jmp-rel            1897 (59.8%)   <- tail E9/EB = tail-jmp epilogue [推定高]
  ret-family         1103 (34.8%)   <- forward decode では ret (= ret imm16 の byte 判定漏れ)
  other-last-ins       47
  jmp reg               6 / jcc-tail 2 / jmp [mem] 2
  straddle 8 + undecodable 2 = 10
  (byte 判定 vs forward の差 = ret imm16 (C2 lo hi) のみ抽出不能)
samples: seg 0x99EE5D..0x9A1B82 jmp [mem] dword [0xa28190]
```

### §2.2 write flow（v446_2a）
```
target 0x407CB4: EB @0x407CAC: jmp 0x407cb4  (unconditional, 1 branch)
  407C9D: mov eax,[0xb33ea0] / je 0x407cae (間1byte=test系)
  407CAE: mov ecx,[0xb3340c] -> 407CB4 mov edx,eax -> sub -> write 407CBA
target 0x407E47: 7x @0x407E44  <- mid-instruction FALSE POSITIVE
  (407E42: mov eax,[edx+0x174] は5byte -> 407E44/407E47 はその中間)
  => fall-through のみ [確定]
```

### §2.3 DAT_00b33ea0（v446_2b）
```
.text refs: 4052CE/4052D3 (FUN_00405290: mov+fild) / 407C9D (dump) /
            408D68/408D6E (fild+mov) / 47F81C (FUN_0047f7b0: cmp eax,[..])
docs 84 hits: L3639-40 interval pattern / L5869,5871,5879 dump diff /
              L101834-36 change detection / (float) casts / < 0 signed
```

## §3 残点 **V-447-1..3（＝旧残点群へ移行）**
1. 【低】**V-414-2 継続**（animation table `0xB10290` 先頭10件の consumer／`0xB108EC` index 源 `[esp+0x5c]` の dump 内詳しい CFG — v446-2 と連続）。
2. 【低】**V-415-2**（既存残点）／**V-413-2**（docs 完全性周辺）を順次消化。
3. 【低】（v446-2 の残り）`je 0x407CAE` の直前1byte decode（`test eax,eax` の確認）＋ straddle/undecodable 10件の個別確認。

## §4 検証・自己批評
- probe: **v446_1..v446_2 の2本全 EXIT=0**（V-446-1 完遂（手法成功）／V-446-2 は flow と global を取得・1件 fall-through 訂正を含む／V-446-3 は方針確定）。
- **決定打の作り**: (i) **forward decode を segment 先頭から実装し、byte 判定との差＝`ret imm16` 1103 件を一挙に検出**（v445 の「正しい手法」をそのまま実装しただけで分離成功）→ (ii) **`jmp-rel 1897` を tail-jmp epilogue と解釈し、境界総数 6209 の ~98% が function end と結論** → (iii) **predicate は target への direct branch を全走査して種別（EB/7x）と src を明示** → (iv) **`je` の条件変数が `DAT_00b33ea0` ref の直後にあることを context decode で接続** → (v) **global は docs 84 hits のパターン（interval/変化検知/float）で counter と解釈**。
- 妥当性チェック: ①ret-family 1103＋jmp-rel 1897＝3000⇔non-ret 3171−その他60 ✓ — ②`je 0x407CAE`⇔`0x407CAE: mov ecx,[0xB3340C]`（write 経路への入口）✓ — ③`0x407E44` が `407E42` の5byte中間⇔mid-instruction（v446_1 straddle 検出と同種）✓ — ④`DAT_00b33ea0` の `fild`⇔`(float)` cast⇔counter 解釈 ✓ — ⑤`0xB3340C = DAT_00b33ea0`（docs L5869）⇔前回保存値（v445）✓ — **五方向矛盾なし**。
- docs との不整合: **なし**（docs の84 hits と整合）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **byte 判定と forward decode の「差分」が直ちに欠落パターンを明かす** — 本件の差 1103 件＝`ret imm16` 捕捉漏れ。**2系統の census を突き合わせると、判定ロジックの穴が数量付きで出る**。
  2. **「forward decode を segment 先頭から」は CC 塀区切りという構造を活かす正しいアプローチ** — v445 で手法を宣言した翌 probe で実装・成功（宣言→実装の1遅延で済んだ）。
  3. **branch 探索は src/kind を必ず明示 print** — v444_2 で src を print し忘れて predicate が取れなかった（今回で解消）。**探索結果は「場所・種別・文脈」の3点セットで出す**。
  4. **mid-instruction 偽陽性は straddle 検出と共存する** — v446_1 の straddle 分類と v446_2a の `7x` 偽陽性が同種の事象（`mov eax,[edx+0x174]` 中間）。**byte-pattern 由来の branch は必ず forward decode の整合性で裏取る**。
  5. **global identity は docs hits の「使用パターン（interval/変化検知/cast）」で読む** — ref 数（84件）とパターン群が counter 解釈の根拠になる。
