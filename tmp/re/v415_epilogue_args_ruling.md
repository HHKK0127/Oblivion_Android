# v415 — StoreTargets epilogue `FUN_006c6910` 呼び出しの引数構造の裁定（V-405-3 部分解・派生残点第7弾）

- 採番: **v415**（派生残点群・probe prefix **v415_1・v415_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v405 の残点 **V-405-3【中】（`Get("Bip01")[+0x1C]` の identity と「`FUN_006c6910` が sequence layout を this として読むのに Bip01 node+0x1C から来る」矛盾・L498845 slot23 呼び出し）** を消化（矛盾解消まで [確定]・Bip01[+0x1C] 正体と slot23 は残点）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **decompiled `FUN_006c6910(*(Bip01+0x1c))` 表示の正体** | **[確定]** | **Ghidra の fastcall ecx 落ちによる表示誤り** — 実体は **`__fastcall` 2引数**: **ecx ＝ `[esp+0xc]`（＝StoreTargets `FUN_006c9590` prologue の `mov [esp+0xc], esi`＝**this(seq) の spill**）**・**stack 引数 ＝ `Bip01[+0x1C]`**（machine `0x6C9B8A mov ecx,[esp+0xc]; 0x6C9B8E push eax; 0x6C9B8F call 0x6c6910`）→ **`NiAVObject[+0x1C]` に sequence が入る構造は存在しない [確定(否定側)] ＝ V-405-3 の「矛盾」は解消** |
| **epilogue の ecx 経路** | **[確定(命令)]** | prologue: `sub esp,2C → push esi → mov esi,ecx（this＝seq）→ push edi → **mov [esp+0xc], esi**`（0x6C9590-0x6C95A6）→ call 群は push/pop balanced で epilogue 直前まで esp 復帰 → **`mov ecx,[esp+0xc]` ＝ seq** → `FUN_006c6910(this=seq, arg=Bip01[+0x1C])` が成立（fun 内 `edi=ecx` が sequence layout を読む従来観測と整合） |
| **`Bip01[+0x1C]` の使用先** | **[部分確定]** | fun 内 `mov ecx,[esp+0x2c]（＝Bip01[+0x1C]）→ push 0xB3CD7C → call 0x700010`（decompiled L496379 `puVar3 = FUN_00700010(&DAT_00b3cd7c)` も **ecx 落ち表示**）→ **`Bip01[+0x1C]` は `FUN_00700010` の ecx 引数へ**。`FUN_00700010` の identity 未同定 → **`Bip01[+0x1C]` の正体は残点 V-415-1** |
| **epilogue 全体の確定済み挙動** | **[確定(命令)]** | prereq flag `cStack_2a==0` のときのみ `palette->Get("Bip02")` → `palette->Get("Bip01")` → `Bip01[+0x1C]` を検証して非0なら `FUN_006c6910(seq, Bip01[+0x1C])` → `return 1`（機械 0x6C9B54-0x6C9B9B・v372 骨解決 epilogue と完全整合） |
| **L498845 `[manager+0x30]->slot23` 呼び出し** | **[未確定]** | 本2 probe では未調査 → 残点 V-415-2 |

## §2 証拠（v415_1 / v415_2）

### §2.1 epilogue machine（v415_1(a)）
```
6C9B54: cmp [esp+0x38],0 ; prereq (cStack_2a)
6C9B5D: jne 0x6C9B71 ; true → Bip02 検索 skip
6C9B5F: mov eax,[esi]; mov edx,[eax+0x4c]; push 0xA79CD0("Bip02"); mov ecx,esi; call edx ; palette->Get("Bip02")
6C9B6D: test/je fail6C9B71: ... push 0xA30968("Bip01") ... ; palette->Get("Bip01")
6C9B7F: test/je fail6C9B83: mov eax,[eax+0x1c] ; Bip01[+0x1C]6C9B86: test/je fail6C9B8A: mov ecx,[esp+0xc] ; ★ ecx (fun 内 param_1)6C9B8E: push eax ; ★ stack arg = Bip01[+0x1C]6C9B8F: call 0x6c69106C9B94...: pop 群 / mov al,1 / add esp,0x2c / ret 4
```

### §2.2 signature と this（v415_1(b)(c)）
- decompiled: `void __fastcall FUN_006c6910(int param_1)`（**fastcall・1引数表示＝stack と ecx のうち stack を表示、ecx 落ち**）。
- fun head machine: `6C6937: mov edi, ecx`（edi＝param_1＝ecx）／`6C6939: mov ecx,[esp+0x2c]`（stack arg1＝Bip01[+0x1C]）→ `push 0xB3CD7C; call 0x700010`。

### §2.3 this の spill（v415_2）
```
6C9590: sub esp, 0x2c
6C9593: push esi
6C9594: mov esi, ecx            ; this = seq6C9596: mov eax, [esi+0x40]
6C9599: push edi
6C95A2: mov dword [esp+0xc], esi ; ★ seq の spill（後段 ecx 側はこのスロットを読む）
6C95A6: mov dword [esp+0x10], edi ; palette の spill
...
（call 引数群は push/add で balanced）
```
→ **epilogue の `mov ecx,[esp+0xc]` ＝ seq** [確定(命令)]。

## §3 残点 **V-415-1..2**
1. **【中】`Bip01[+0x1C]` の正体** — `FUN_00700010(ecx=Bip01[+0x1C], arg=0xB3CD7C)` の identity（v405 で「keyframe global 取得/初期化」と記録したが未同定）→ `FUN_00700010` の decompiled 実読と arg の意味で Bip01[+0x1C] を確定。
2. **【低】L498845 `[manager+0x30]->slot23(+0x5C)` 呼び出し**（`prereq==1` 時の本命処理）の identity — manager[+0x30] の正体（v406 の「Target/Node 候補 [部分確定]」）と slot23 の意味。

## §4 検証・自己批評
- probe: **v415_1・v415_2 の2本全 EXIT=0**。
- **決定打の作り**: (i) epilogue machine の `mov ecx,[esp+0xc]; push eax; call` を unfiltered で取得 → (ii) **decompiled signature（fastcall/1引数）と machine（stack1）の齟齬を「ecx 落ち」と判定** → (iii) **prologue 直後の spill（`mov [esp+0xc], esi`＝this）**を発見して ecx の正体を確定 → (iv) fun 内 `mov ecx,[esp+0x2c]`＝stack arg1 と decompiled 表示を突合。
- 妥当性チェック: ①fun 内 `edi=ecx` が sequence layout を読む観測（v405/v406）⇔ecx＝seq spill — ②`push eax`（Bip01[+0x1C]）⇔decompiled `FUN_006c6910(...)` の表示引数 — ③call balanced⇔epilogue esp 復帰 — **三方向矛盾なし**。
- docs との不整合: **なし**（v405 ruling の V-405-3 内「sequence layout を持つ必要＝矛盾」記述を**解消**（実際は表示誤り）。docs/GT 反映時に v405 の該当注記を併せて整理・#47/#20 一括）。
- **教訓**:
  1. **fastcall の ecx 落ちは「this が消える」「stack 引数が this に見える」の両方向で起きる** — thiscall/fastcall 判別は **call 直前の `mov ecx,...` で必ず実施**（v414 の fn 内教訓を caller 側へ展開した実例）。
  2. **prologue 直後の `mov [esp+N], reg` spill を打っておくと、epilogue の reg 読みを逆算して ecx/reg の正体を機械的に確定できる**。
  3. **「fn がこの layout を読むのにこの値が渡る」型の矛盾は、まず Ghidra 表示の引数落ちを疑え**。
