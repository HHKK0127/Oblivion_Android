# v378 — `[this+0x44]` state 2/5 の意味と `0x6C9BA0`（Activate 系）の裁定（#150 残点・V-377-1）

- 採番: **v378**（#150 残点 V-377-1 消化、probe prefix v378_1）
- 対象: Oblivion.exe v1.2.0416 ＋先行裁定 v187_anim_residuals.md
- 依頼経緯: v377 の残点 **V-377-1（state 2 と 5 の意味）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| state **2** の意味 | **[確定(命令)]** | **フェード遷移中（flag=0・fade duration > 0）** — `0x6C9C2A: cmp [esi+0x44],0` → `0x6C9C2E: mov [esi+0x44],2`（分岐元 `0x6C9C03: cmp byte [esp+0x20],0; je 0x6C9C2A`＝**flag==0 のとき**）。`0 < arg2`（fade duration）のときのみ到達 |
| state **5** の意味 | **[確定(命令)]** | **フェード遷移中（flag≠0・fade duration > 0）** — `0x6C9C0A: cmp [esi+0x44],0` → `0x6C9C0E: mov [esi+0x44],5`（分岐元 `0x6C9C08: je 0x6C9C2A` の fallthrough＝**flag≠0 のとき**）。他に `0x6C9E72` でも設定。**caller `0x471A7B: cmp [eax+0x44],5` のときだけ `0x6C4480`（Manager 通知）を呼ぶ**＝**state 5 = 他 sequence とのブレンド遷移中** [推定高] |
| state **1** の意味 | **[確定(命令)]** | **即時再生（fade duration ≤ 0）** — `0x6C9BF6: fldz; fcomp [esp+0x18]; jp 0x6C9C64` → `0x6C9C64: cmp [esi+0x44],0` → `0x6C9C68: mov [esi+0x44],1`。他に `0x6C9D79`/`0x6C9EAC` |
| `0x6C9BA0` の identity | **[部分確定]** | **`ActivateSequence` 相当（state==0 のときのみ有効・`ret 0x18`＝6引数）** — `0x6C9BA3: cmp [esi+0x44],0; je` → `0x6C9BA9: xor al,al; ret 0x18`（**state!=0 なら false**）。arg1（`[esp+0x1C]`）＝**相手 sequence**（`[edi+0x58]` に記録・自己参照 `cmp eax,esi; je false`／`0x6C6110`・`0x6C6ED0` 検証）／arg2（`[esp+0x18]`）＝**fade duration（float）**／arg3（`[esp+0x20]`）＝**flag（byte・0→state2 / 1→state5）**。`[esi+0x1C]=arg?`（duration 記録）・`[esi+0x4C]=-const(0xA7DEB4)`（fade time）・`[esi+0x50]=arg2`（fade 期間）・`[esi+0x58]=相手`・`[esi+0x40]+0x4C`（sequence）へ `0x73A5E0` 通知 |

**state enum の完全図（v377＋v378）**: **0=停止**（`0x6C9D14`）→ **1=即時再生**（fade≤0）／**2=フェード遷移（flag=0）**／**5=フェード遷移（flag≠0・ブレンド中 [推定高]）** → **3/4=フェードアウト/停止**（Deactivate/Suspend `0x6C9CB0`）→ **6=サイクル**（v187「state6 のサイクルオフセット」）。読み手は `cmp [reg+0x44],0` が8件（全件 `sete al`＝新規遷移判定）で、**state 値を直接比較するのは `0x471A7B` の `cmp ,5` の1件のみ** [確定(否定側)]。

---

## §2 証拠

### §2.1 `0x6C9BA0`（Activate 系）の全体 decode（v378_1(a) unfiltered）

```
6C9BA0: push esi; mov esi, ecx
6C9BA3: cmp dword [esi+0x44], 0        ; ★state==0 のときのみ有効
6C9BA7: je 0x6C9BAF
6C9BA9: xor al, al; pop esi; ret 0x18   ; state!=0 → false（6引数）
6C9BAF: push edi
6C9BB0: edi = [esp+0x1C]                ; arg1 = 相手 sequence
6C9BB4: test edi,edi
6C9BB6: [esi+0x58] = 0                  ; time/phase ゼロ
6C9BBD: je 0x6C9BE3
6C9BBF: eax = [edi+0x58]                ; 相手の time
6C9BC2: test; je 0x6C9BD4
6C9BC6: cmp eax, esi; je 0x6C9C23       ; ★自己参照 → false
6C9BCA: call 0x6C6110（比較 [推定]）
6C9BD0: test al,al; je 0x6C9C23         ; false
6C9BD4: call 0x6C6ED0（検証）
6C9BDC: test al,al; je 0x6C9C23         ; false
6C9BE0: [esi+0x58] = edi                ; ★相手 sequence を記録
6C9BE3: call 0x6C6A50                   ; field8 interpolator へ
6C9BEF: [esi+0x1C] = [esp+0x14]         ; duration 設定
6C9BF6: fldz; fcomp [esp+0x18]          ; ★0 と fade duration 比較
6C9BFE: test ah,5; jp 0x6C9C64          ; !(0 < arg2) → state=1 側
6C9C03: cmp byte [esp+0x20], 0          ; ★flag
6C9C08: je 0x6C9C2A                     ; flag==0 → state=2
6C9C0A: cmp [esi+0x44],0
6C9C0E: [esi+0x44] = 5                  ; ★flag≠0 → state=5
6C9C15: sete al（state が 0 だったか＝新規遷移）
6C9C1C..6C9C4B: [esi+0x40]+0x4C へ push → call 0x73A5E0（通知）
6C9C2A: cmp [esi+0x44],0
6C9C2E: [esi+0x44] = 2                  ; ★flag==0 → state=2
6C9C50: [esi+0x4C] = -0xA7DEB4（fade time）
6C9C5B: [esi+0x50] = [esp+0x18]（fade 期間）
6C9C64: cmp [esi+0x44],0
6C9C68: [esi+0x44] = 1                  ; ★fade≤0 → state=1
6C9C8A: cmp byte [esp+0x10],0; je; [esi+0x48] = -0xA7DEB4（別 fade）
6C9C9C: pop edi; mov al,1; pop esi; ... ; true
```

### §2.2 state 5 の caller（v378_1(c) unfiltered `0x471A60..0x471ABC`）

```
471A66: fldz; push 0; push ecx; fstp [esp]     ; fade=0.0
471A70: call 0x6C9CB0                          ; Deactivate/Suspend（即時）
471A75: eax = [ebx+0xAC]                       ; sequence ptr
471A7B: cmp dword [eax+0x44], 5                ; ★state==5（フェード遷移中）か?
471A7F: jne 0x471A92
471A81: fldz; mov ecx,[ebx+0x98]; fstp [esp]
471A8D: call 0x6C4480                          ; ★state5 のときだけ Manager 通知
471A92: fldz; push 0; ... ; call 0x6C9CB0      ; Deactivate（即時）
471AA5: eax=0xFF
471AAA: [ebx+0xAC] = 0                         ; sequence 解放
471AB4: word [ebx+0x42]/[ebx+0x76] = 0xFF
471ABC: dword [ebx+0x54] = -1
```
→ **state 5 のときだけ Manager（`0x6C4480`）へ通知**＝**他 sequence とのブレンド遷移中** [推定高]。state 2 にはこの通知経路が無い。

### §2.3 state 読手 census（v378_1(d)）

- `cmp [reg+0x44],0` が **8件**（`0x6C9BA3`/`0x6C9C0A`/`0x6C9C2A`/`0x6C9C64`/`0x6C9D58`/`0x6C9D79`/`0x6C9E41`/`0x6C9E6E`/`0x6C9EA8`/`0x6C9ED9`）— **全件 `sete al`（新規遷移判定）か `jne`（スキップ）**。
- **state 値を直接比較するのは `0x471A7B: cmp [eax+0x44],5` の1件のみ** [確定(否定側)]。

## §3 残点 **V-378-1..2**
1. **flag 引数（`[esp+0x20]`）の実体**（`0x6C9BA0` の arg3・state5（ブレンド）と state2（単独フェード）を分ける条件。caller 側の値（0/1）の設定元 decode で確定可）【中】
2. **`0x6C4480`（state5 のときの Manager 通知）の identity**（ActivateSequence の逆操作/ブレンド登録か）【中】

## §4 検証・自己批評
- probe: **v378_1 全 EXIT=0**。
- **決定打の作り**: (i) state 2/5 設定 site の find_head → (ii) **`0x6C9BA0` fn の unfiltered 全 decode で分岐構造（flag/fade duration）を直接確認**→ (iii) caller `0x471A7B` の `cmp ,5` 文脈で state5 の特殊性を確認 → (iv) state 読手 census で「値比較は1件のみ」を証明。
- 妥当性チェック: ①`0x6C9C08: je 0x6C9C2A`（flag=0→state2）⇔ fallthrough（flag≠0→state5）⇔ caller の state5 専用通知 — ②`jp 0x6C9C64`（fade≤0→state1）⇔ v377 の state1 設定 site — **矛盾なし**。
- docs との不整合: **なし**（v377 の残点を解消。v187 の cycle type（byte+8 bit1-2）とは**別フィールド**であることを確認＝state `+0x44` は NiControllerSequence の dword）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
