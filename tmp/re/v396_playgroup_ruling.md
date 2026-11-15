# v396 — `0x5109E7` の所属の裁定（#153 ⑥-2・V-383-3）

- 採番: **v396**（ai-todo #153 束の第7弾＝低10件の第2弾、probe prefix v396_1..v396_3）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v383 の残点 **V-383-3（`0x5109E7` 所属）** を消化（v392 で `0x6C5C70`（blend bridge）の callers 2本のうちの1本）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0x5109E7` の所属** | **[確定(命令＋文字列＋decompiled)]** | **fn `FUN_005107a0`（`0x5107A0`）＝コンソールコマンド「PlayGroup」の handler** — 三重証拠: ①decompiled **L204446 `// FUN_005107a0 @ 0x5107a0`**（L204451 シグネチャ: 8引数の静的 fn）②**.data のコマンドテーブル entry**（VA `0xB0CBB8`: name `0xA4FAC8`='**PlayGroup**' → ID **0x1013** → usage `0xA4FAB0`='**IE: PlayGroup SEQNAME 1**' → arg spec `0x00020001` → data `0xB0A918` → **handler `0x5107A0`** → 共通 helper `0x4FDE30`）③**"PlayGroup Error" 文字列3種**（`0xA4D210`/`0xA4D250`/`0xA4D290`）が本 fn のエラー経路（L204485/204495/204524）。**E8 callers＝0件＋.data 内 packed dword＝1件（file 0x70B7D0）＝テーブル登録 dispatch** [確定(命令)] |
| `0x5109E7` の文脈（何をしているか） | **[確定(命令＋decompiled)]** | **PlayGroup 実行の「対象 sequence 開始」経路**: ①NiControllerManager（ iVar6・[推定高] 下記）の **`[+0x40]` 配列（count `[+0x46]`）の全 sequence を走査**し、各 entry の名前（`[entry+8]` char*）を **`stricmp` で照合、不一致なら `Deactivate(entry, 0, 0)`（`0x6C9CB0`・v393）**（一致時は skip）②**一致対象（target seq＝stack から ebp へ再ロード）へ `0x6C5C70(seq, 0, fade, 0, 0)`（blend bridge・v392）で fade 付き開始**（fade＝`fild` int × `double[0xA2FC80]` の秒変換 [部分確定]）③else 経路は **`0x470B20(seq, 0, 0, 1.0, 0, 0)`（crossfade wrapper・v391）** ④**`[seq+0x48] = −FLT_MAX`（Offset リセット）＋ `or word [esi+8], 8`（manager の Update gate bit3 設置＝v395 のゲートと同 bit）** |
| 対象 object（esi/iVar6） | **[推定高]** | **NiControllerManager** — 読む `word[+0x46]`（count）/`[+0x40]`（配列）/`word[+8]` bit3（Update gate）が **v395 の ctor 初期化（`[+0x40]=0`/`[+0x46]=0`/`[+0x4a]=10`）と4/4一致**＋本経路の semantics（他 sequence を Deactivate して対象のみ開始）と整合。lookup 経路: `FUN_00560920(&DAT_00b3cac0, ...)`（iVar6）＋エラー用 `FUN_004da2a0(param_3[3])`（"No NiControllerManager found" の名前整形） |
| コマンドテーブルの位置づけ | **[確定(命令)]** | `.data` VA `0xB0CBB8` から **0x28 バイト entry**の連番テーブル: GetCurrentTime(0x1012) → **PlayGroup(0x1013)** → LoopGroup(0x1014・handler `0x501C40`) → SkipAnim(0x1015・`0x501CF0`) → StartCombat(0x1016) ... ＝**アニメ/シーケンス系コンソールコマンド群**。各 entry の `+0x1C`＝共通 helper `0x4FDE30` |

**V-383-3 への回答**: `0x5109E7` の所属は **`FUN_005107a0`（`0x5107A0`）＝「PlayGroup」コマンド handler [確定]**（.data コマンドテーブル登録・usage `IE: PlayGroup SEQNAME 1`）。v392 の「別システムからの Morph/blend 要求」は **PlayGroup（sequence 再生コマンド）からの要求**と具体化。

---

## §2 証拠

### §2.1 head と文脈（v396_1(a) unfiltered 0x510980..0x510A20）

```
510980: eax = [esi+0x40]（配列）; ebp = [eax+ebx*4]（entry＝sequence）
510986: ecx = [0xb102e0]（照合先名）; eax = [ebp+8]（entry の名前）
510991: call 0x98262d（stricmp）
51099B: test eax,eax; je 0x5109ac   ; ★一致 → Deactivate を skip
5109A5: mov ecx,ebp; call 0x6c9cb0  ; 不一致 → Deactivate(entry, 0, 0)
5109AC: loop: ebx+1 < word[esi+0x46]
5109B7: ebp = [esp+0x18]（★target seq へ再ロード）
5109C0:（fade>0 経路）fild [esp+0x18] × double[0xa2fc80] → fade
5109E7: call 0x6c5c70   ; ★seq, 0, fade, 0, 0（blend bridge）
5109EC: jmp 0x510a06
5109EE:（fade=0 経路）call 0x470b20  ; seq,0,0,1.0,0,0（crossfade）
510A06: [ebp+0x48] = −FLT_MAX（Offset リセット）
510A11: or word [esi+8], 8          ; ★manager gate 設置
```

### §2.2 decompiled（v396_1(c)/v396_2(a)）

- **L204446 `// FUN_005107a0 @ 0x5107a0`**／L204451 `FUN_005107a0(param_1..param_8)`（8引数静的 fn）。
- L204492: `local_8[0] = FUN_004715a0((&PTR_DAT_00b102e0)[unaff_EDI * 9])`（**×36-byte PlayGroup 定義テーブルから fade 取得**・v391 の `0xB102E8` ×36 と**同テーブル族**）。
- L204499-204508: `[iVar6+0x46]` 回のループで **`__stricmp(entry名, PTR_DAT_00b102e0)` 不一致 → `FUN_006c9cb0(0,0)`（Deactivate）**。
- L204511-204516: `local_c==0 → FUN_00470b20(seq,0,0,1.0,0,0)` / else → **`FUN_006c5c70(fVar1,0,local_8[0],0,0)`**。
- L204518-204519: `[seq+0x48] = −FLT_MAX` ＋ **`[iVar6+8] |= 8`**。
- エラー3種: L204485 `"PlayGroup Error: No 3d or controllers found..."`／L204495 `"PlayGroup Error: Sequence '%s' not found..."`／L204524 `"PlayGroup Error: No NiControllerManager found for '%s' (%08X)."`（lookup `FUN_004da2a0(param_3[3])`）。

### §2.3 .data コマンドテーブル（v396_2(b)/v396_3）

- packed dword `0x005107A0`＝**全ファイル1件・file 0x70B7D0（.data・VA 0xB0CBD0）**。
- entry（**stride 0x28**・VA `0xB0CBB8` = PlayGroup entry 先頭）:

| offset | 値 | 意味 |
|---|---|---|
| +0x00 | `0xA4FAC8` → **"PlayGroup"** | コマンド名 |
| +0x04 | `0xA2F7EC` → 0 | （全 entry 共通・既定値 [未確定]） |
| +0x08 | **0x1013** | コマンド ID（連番: GetCurrentTime=0x1012, LoopGroup=0x1014, SkipAnim=0x1015, StartCombat=0x1016...） |
| +0x0C | `0xA4FAB0` → **"IE: PlayGroup SEQNAME 1"** | usage/help |
| +0x10 | 0x00020001 | arg spec [部分確定] |
| +0x14 | 0xB0A918 | data/arg descriptor [部分確定] |
| +0x18 | **0x5107A0** | ★handler |
| +0x1C | 0x4FDE30 | 共通 helper（全 entry 同一） |
| +0x20/+0x24 | 0/0 | — |

- 隣接 entry: GetCurrentTime / **PlayGroup** / LoopGroup（handler `0x501C40`）/ SkipAnim（`0x501CF0`）/ StartCombat ＝**アニメ/シーケンス系コマンド群**。

### §2.4 head machine（v396_2(d)）

`0x5107A0: sub esp,0xc; ... 8 push → call 0x4fae80（検証）; add esp,0x24; test al,al; je ret` — コマンド handler 形（引数検証 `FUN_004fae80` → 本体）。E8 callers＝**0件**（.data ポインタ経由 dispatch）。

## §3 残点 **V-396-1..3**
1. **`iVar6`（`FUN_00560920(&DAT_00b3cac0, ...)` の戻り）の同定**: NiControllerManager で確定する裏付け＋`FUN_00560920`/`FUN_004da2a0` の identity（0xB3CAC0 マップの内容）【中】
2. **コマンドテーブルの全体構造**: entry 先頭・件数・ID 0x1011 系の意味・`+0x10` arg spec/`+0x14` data/`0x4FDE30` helper の役割【低】
3. **`[0xB102E0]`（×36-byte PlayGroup 定義テーブル）の entry 構造**: field0=name・fade を読む field の位置（v391 の `0xB102E8` と同一テーブル族の統合レイアウト）【低】

## §4 検証・自己批評
- probe: **v396_1..v396_3 全 EXIT=0**。
- **決定打の作り**: (i) head＋文脈 unfiltered（stricmp→Deactivate→0x6C5C70→gate 設置を直視）→ (ii) **decompiled 注記（L204446）＋"PlayGroup Error" 文字列3種** → (iii) **packed dword census（.data 1件）→ entry 全体の dereference（name/ID/usage/handler）** → (iv) 隣接コマンド群でテーブル位置づけを確定。
- 妥当性チェック: ①machine の push 順⇔decompiled `FUN_006c5c70(fVar1,0,fade,0,0)` 逐語一致 — ②`or word [esi+8],8`⇔v395 の Update gate bit3 — ③`[+0x40]/[+0x46]`⇔v395 ctor layout — ④E8=0⇔.data ポインタ登録 — **四方向で矛盾なし**。
- docs との不整合: **なし**（v383 の V-383-3 を解消。v392 の「別システム」は「PlayGroup コマンド系」と具体化）。
- **教訓**: **登録型 fn（E8=0）は packed dword census → 見つかった .data/.rdata 位置の前後を stride で展開し name/ID/usage を dereference すると identity が一撃で決まる**。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
