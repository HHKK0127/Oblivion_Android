# v393 — state 3 と 4 の意味の差の裁定（#153 ④・V-384-1）

- 採番: **v393**（ai-todo #153 束の第4弾、probe prefix v393_1..v393_2）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v384 の残点 **V-384-1（state 3 と 4 の意味の差: Deactivate byte flag 0→3/1→4・`0x6C4480` が state4 を flag=0 で再 Deactivate→3 が何を完了させるか・`0x6C9CEF` 帯分岐と合わせて）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| state3/4 の生成規則 | **[確定(命令＋decompiled)]** | **`state = (flag≠0) + 3`（Deactivate の fade>0 パス）** — decompiled L498938 逐語 `*(uint*)(param_1+0x44) = (param_3 != '\0') + 3`。**flag=0 → state3**（通常 Deactivate: crossfade wrapper `0x6C4000`／`0x6C4480` 掃討）／**flag=1 → state4**（blend wrapper `0x6C9D30`/`0x6C9E00` の2回目 `Deactivate(fade>0,1)`・update 内 state6 ブロック） |
| Deactivate 全体の意味 | **[確定(decompiled)]** | L498931-498953: **state==0 → false**／**fade>0 → state=3+flag・`Start[+0x4C]=−FLT_MAX`・`End[+0x50]=fade`（期間値・update 初回に絶対時刻化＝v382）・true**／**fade<=0 → 即時停止**: `Offset[+0x48] += (Last[+0x34]≠−FLT_MAX ? WeightLast[+0x38]/Freq[+0x28] − Last : 0)`、state=0、partner=0、`[+0x54]=−FLT_MAX`、**Unregister(`0x6C6AC0`)**、true |
| update での扱いの差 | **[確定(命令)]** | **jump table `0x6CAC40`（state-1=0..5）**: **state3→`0x6CAA69`**（t=(End−now)/(End−Start) → **`[esp+4]` スロット**・期限切れ→`Deactivate(0.0,flag=0)`）／**state4→`0x6CAB23`**（同公式 → **`[esp+8]` スロット**・期限切れ→`Deactivate(0.0,flag=1)`）。**スロットは系統で分かれる: `[esp+4]`←state2/3（crossfade 系）／`[esp+8]`←state4/5/6（blend 系）**（state1=即時→両方 1.0 維持）[確定(命令)] |
| 読手の差 | **[確定(否定側)＋確定(命令)]** | **state3 の `cmp ,3` 読手＝0件**（0x6C0000..0x710000 全走査）／**state4 の読手は `0x6C4496` の1件のみ**（`0x6C4480` 管理者掃討）→ **state4 のみが「管理者から見える未完了 fade-out」**。**`0x6C4480` の掃討＝`Deactivate(fade=arg, flag=0)` → fade=0 なら即時停止（state0）／fade>0 なら state3（通常フェードアウトへ移行）** [確定(命令)] |
| （補足）blend wrapper `0x6C9D30` の全体 | **[部分確定(decompiled)]** | L498962-498994: **`Deactivate(0,1)`（即時停止）→ `if state==0 { partner=0; Register(param_5); state=1; set-add; if (Activate(target,…,s6=1)) { Deactivate(param_3>0, 1) → ★state4; `[+0x54]` 両 seq へ＝LastScaled/Freq・param_4/Freq; return 1 } } return 0`** — **param_3 は L498972-73 で <=0 なら正の既定値へ** → **state4 の正 Enter＝wrapper の2回目 Deactivate(fade>0, flag=1)** [確定(命令)]・param_1/param_2/param_5 の対象オブジェクト特定は残点 |

**V-384-1 への回答**: **state3 と state4 は同じ「フェードアウト中（t=(End−now)/(End−Start)）」だが、(1) 生成 flag（0/1）、(2) weight スロット（`[esp+4]` vs `[esp+8]`＝crossfade 系 vs blend 系）、(3) 期限切れ時の Deactivate flag、(4) 管理者 `0x6C4480` の可視性（state4 のみ）で異なる**。「state4→3 への再 Deactivate が完了させるもの」＝**fade>0 なら通常フェードアウト（state3）への移行、fade=0 なら即時停止** [確定(命令)]。

---

## §2 証拠

### §2.1 生成規則と Deactivate 全体（v393_1(d)/v393_2(b) decompiled L498931-498953）

```
L498934: if (state == 0) return 0;
L498937: if (0.0 < fade) {
L498938:   state = (flag != 0) + 3;          ★flag=0→3 / flag=1→4
L498939:   Start[+0x4C] = −FLT_MAX;
L498940:   End[+0x50]   = fade;              ★期間値（update 初回に now 加算＝v382）
L498941:   return 1;
L498943: } // fade <= 0 → 即時停止
L498944-46: if (Last[+0x34] != −FLT_MAX) Offset[+0x48] += (WeightLast[+0x38]/Freq[+0x28] − Last);
L498948: state = 0; L498949: partner = 0; L498950: [+0x54] = −FLT_MAX;
L498951: FUN_006c6ac0()（Unregister）; L498952: return 1;
```

### §2.2 update の jump table（v393_2(a) 実測 `0x6CAC40`）

| state-1 | state | 実行ブロック | t の格納先 | 期限切れ時 |
|---|---|---|---|---|
| 0 | **1**（即時） | `0x6CAB73`: `Start[+0x4C] = LastScaled[+0x3C]` | **両方 1.0 維持** | — |
| 1 | **2** | `0x6CA9BC`: (now−Start)/(End−Start) | **`[esp+4]`** | — |
| 2 | **3** | `0x6CAA69`: **(End−now)/(End−Start)** | **`[esp+4]`** | `Deactivate(0.0, flag=0)` → state0 |
| 3 | **4** | `0x6CAB23`: **(End−now)/(End−Start)** | **`[esp+8]`** | `Deactivate(0.0, flag=1)` → state0 |
| 4 | **5** | `0x6CA9FB`: (now−Start)/(End−Start) | **`[esp+8]`** | （`[+0x54]` 変換→**state1**） |
| 5 | **6** | `0x6CAABC`: パートナー Offset 加算＋`0x6C6B30` テキストキー同期＋**`state=4` へ遷移（6CAAFB）**→state4 ブロックへ合流 | **`[esp+8]`** | （state4 と同） |

- **`[esp+4]`←states 2/3（crossfade 系）／`[esp+8]`←states 4/5/6（blend 系）** [確定(命令)]。state1 は即時再生＝フェードなし（1.0 維持）。

### §2.3 読手 census（v393_1(a)(b)）

- **`cmp [reg+0x44],3` 読手＝0件**（0x6C0000..0x710000・83/7＋81/7 全走査）[確定(否定側)]。
- **`cmp [reg+0x44],4` 読手＝`0x6C4496` の1件のみ**（`0x6C4480` の掃討ループ）[確定(命令)]。

### §2.4 `0x6C4480` 掃討の帰結（v384 再掲＋本裁定で意味づけ）

`0x6C4480(fade)` は state4 の sequence に `Deactivate(fade, flag=0)` → **fade=0 → 即時停止（state0＋Unregister）／fade>0 → state3（通常フェードアウトへ降格）** [確定(命令)]。callers の大半は `fldz`（fade=0＝即時掃討）、`0x47106D` は実値（state4→state3 のフェード再開）。

### §2.5 blend wrapper `0x6C9D30`（v393_2(b) decompiled L498962-498994）

```
L498970: Deactivate(0, 1)                ; 即時停止（state0）
L498972-73: if (fade <= 0) fade = 既定値(>0)
L498975: if (state == 0) {
L498977:   Register(param_5); L498980: state = 1; L498981-83: set-add(0x73A5E0)
L498985:   if (Activate(target, …, s6=1)) {        ; → state5
L498987:     Deactivate(fade>0, 1) → ★state4
L498988-89:   [+0x54] = LastScaled/Freq（this）・param_4/Freq（他）
L498990:     return 1; } }
L498993: return 0;
```

## §3 残点 **V-393-1..3**
1. **`[esp+4]`/`[esp+8]` スロットの消費者と正確な意味**（tail `0x6CAB90..0x6CABE0` が未デコード・crossfade 用/blend 用の2 t 値と weight 計算への接続）【中】
2. **wrapper `0x6C9D30` の param_1/param_2/param_5 の対象オブジェクト特定**（machine の Deactivate/Activate の this・state1→state4 が同一オブジェクトで入るか）【低】
3. **state6→state4 遷移（`0x6CAAFB`）の意味**（サイクル完了時の handoff か・state6 の wrapper 設定元 `0x6C9EDD` との関係）【低】

## §4 検証・自己批評
- probe: **v393_1・v393_2 全 EXIT=0**。
- **決定打の作り**: (i) `cmp ,3`/`cmp ,4` 読手 census（0件/1件）→ (ii) **jump table `0x6CAC40` の6エントリ実測**で per-state ブロックを特定 → (iii) **decompiled の Deactivate 逐語**（flag→state・fade<=0 即停）→ (iv) wrapper 全体の decompiled。
- 妥当性チェック: ①`state=(flag≠0)+3`⇔machine `setne al; add eax,3`（v377）— ②End=fade⇔update 初回の End+=now（v382）— ③state4 のみ管理者可視⇔0x6C4496＝唯一の `cmp ,4` — ④スロット系統⇔v379/v383 の crossfade/blend wrapper 系統 — **四方向で矛盾なし**。
- docs との不整合: **なし**（v384 の V-384-1 を解消。v384 の「state4→3 は中断クリーンアップ」は『fade>0 なら state3 への移行／0 なら即停』と精緻化）。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
- **復元メモ**: 本ファイルは2026-11-15 の並行セッションの git マージ操作で一度消失し、会話内の原本から再作成（内容同一）。
