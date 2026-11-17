# v492 — notify 本体 gated 実行・`FUN_004CBA80` arg3＝`1`・第2 marker singleton の裁定（V-492-1..3 完遂・旧残点 V-406-3 継続第22弾）

- 採番: **v492**（旧残点群消化・probe prefix **v492_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-492-1..3（v491 残点）** — 1=`FUN_0045B670`（notify 本体）本文 identity／2=`FUN_004CBA80` の `param_3` と `DAT_00B33AAC` identity／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-492-1: `FUN_0045B670`（bit5 notify 本体）本文** | **[確定相当]（gated 実行体）** | L72253 実読 [確定(decompiled)]: **`void __thiscall FUN_0045b670(int param_1, int param_2, undefined4 param_3)`: TLS（`*(FS+0x2c)+_tls_index*4`）取得 → `if (((TLS[+0x185]!=0) && (TLS[+0x184]==0)) && (*(u8*)(DAT_00b33a98+0xcd4)==0) && ((*(u32*)(param_2+8)>>14 & 1)==0)) { cVar2=FUN_0045a500(); if (cVar2==0) FUN_00452c20(param_2,param_3); else if (((*(u32*)(param_1+0x18)>>3 & 1)!=0) && (*(int*)(param_1+4)!=0)) FUN_00452c20(param_2,param_3); }` [確定(構造)]** → **`FUN_0045B670`＝「TLS状態2byte（`+0x184/+0x185`）＋global `DAT_00B33A98+0xCD4`＋`param_2+8` bit14 の3重 gate の下で `FUN_00452C20(param_2, param_3)`（処理本体 [推定]）を実行する gated 実行体 [確定相当]`**、`FUN_0045A500`（mode 判定 [推定]）と `param_1+0x18` bit3＋`param_1+4` の追加条件、**`FUN_00452C20` の identity は [未確定]＝残点** → **bit5（`+0x8` の `0x20`）semantic は「この gated 実行の許可フラグ」系 [推定]**（完全確定には `FUN_00452C20` 本文が次の手） |
| **V-492-2: `FUN_004CBA80` の `param_3`** | **[確定相当]** | **E8 callsites 2件の machine push 数を確認 [確定(機械)]: `0x4D0E74`（owning `FUN_004D0C20`）pushes＝`['1','ebp','1','eax']`（4push）／**`0x65FECF`（owning `FUN_0065FDA0`）pushes＝`['1','ecx']`（2push）＋`mov ecx,eax`** → **`0x65FEC4..0x65FECD` 構造: `mov ecx,[0xb33aac]` → `push 1` → `push ecx` → `mov ecx,eax`（this）→ `call 0x4CBA80` → thiscall（ecx＝param_1）＋stack（param_2＝`0xB33AAC`・**param_3＝`1`**）** → **docs L424183 表示 `FUN_004cba80(DAT_00b33aac, 1)` と完全一致 → `param_3 = 1` [確定相当]**（v491「docs 欠落＝machine 要確認」→ **machine で確認完遂**）。`0x4D0E74` site の4push は別呼び出し形（`FUN_004D0C20` 内・v478 既知 large fn）＝**こちらは引数数が異なるため残点 [部分確定]** |
| **V-492-2: `DAT_00B33AAC` identity** | **[確定相当]（marker 系 singleton）** | **refs 実読 [確定(機械)]: `FUN_0044AC20`（L57509・v466 既知 fn）内で `push 0x7c; call 0x401f00`（**alloc 0x7C**）→ `mov [0xb33aac], eax` → `call 0x46c300` → **`mov ecx,[0xb33aac]; mov edx,[ecx]; mov eax,[edx+0xd8]; push 0xa38094`（**`virtual+0xD8` に名前設定**）** → **`call 0x4b2b80`（v470 list 接続 fn）→ `mov ecx,[0xb33aac]` へ** → **`DAT_00B33AAC`＝alloc `0x7C` で構築・名前（`0xA38094` 文字列 ptr）を `+0xD8` に設定・list 接続する第2 marker 系 singleton [確定相当]**（**v470 の NorthMarker（`DAT_00B35EB8`）と同一 pattern**＝alloc→ctor→`+0xD8` set name→`FUN_004B2B80` list link）。**`0xA38094` の実文字列（第2 marker 名）は [未確定]＝残点**。docs mentions 8件（L58065..L58078） |
| **V-492-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v492_1）

### §2.1 `FUN_0045B670` body（v492_1a）
```
L72253: void __thiscall FUN_0045b670(int param_1, int param_2, undefined4 param_3) {
L72260:   iVar1 = *(int *)(*(int *)(unaff_FS_OFFSET + 0x2c) + _tls_index * 4);   // TLS
L72261:   if ((((*(char *)(iVar1 + 0x185) != 0) && (*(char *)(iVar1 + 0x184) == 0)) &&
L72262:       (*(char *)(DAT_00b33a98 + 0xcd4) == 0)) && ((*(uint *)(param_2 + 8) >> 14 & 1) == 0)) {
L72263:     cVar2 = FUN_0045a500();                       // mode check [推定]
L72264:     if (cVar2 == 0) { FUN_00452c20(param_2, param_3); }
L72267:     else if (((*(uint *)(param_1 + 0x18) >> 3 & 1) != 0) && (*(int *)(param_1 + 4) != 0))
L72268:            { FUN_00452c20(param_2, param_3); return; }
L72271:   } return; }
```

### §2.2 `FUN_004CBA80` push census（v492_1b）
```
sites: 2
  0x4D0E74 owning=FUN_004d0c20  pushes=4 ['1','ebp','1','eax']   (separate call shape, residual)
  0x65FECF owning=FUN_0065fda0  pushes=2 ['1','ecx']  + mov ecx,eax (this)
sequence @0x65FEC4: mov ecx,[0xb33aac] / push 1 / push ecx / mov ecx,eax / call 0x4cba80
  => thiscall: ecx=param_1(eax), param_2=0xB33AAC, param_3=1
  docs L424183 display: FUN_004cba80(DAT_00b33aac, 1)  -> matches (2 shown, this implicit)
  => param_3 = 1  [確定相当]
```

### §2.3 `DAT_00B33AAC`（v492_1c）
```
FUN_0044AC20 (L57509): push 0x7c / call 0x401f00 (alloc 0x7C) / mov [0xb33aac], eax /
                        call 0x46c300 /
                        mov ecx,[0xb33aac] / mov edx,[ecx] / mov eax,[edx+0xd8] / push 0xa38094 /
                        call ... (SetName) / call 0x4b2b80 (list link, v470 pattern)
docs mentions: 8 (L58065..L58078)
=> 2nd marker-style singleton (alloc 0x7C -> ctor -> +0xD8 name -> list link)  [確定相当]
name string 0xA38094: unread -> residual
```

## §3 残点 **V-493-1..3**
1. 【中】**`FUN_00452C20`（gated 実行体の本体）identity** → bit5 semantic と notify chain（slot `+0x40`）の機能が確定する（V-492-1 の残り）。
2. 【低】**`0xA38094`（第2 marker 名）実文字列** ＋ `0x4D0E74` site（4push 形）の引数確認 ＋ `FUN_0045A500`（mode 判定）identity。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v492_1 の1本 EXIT=0**（V-492-1 gated 実行体を確定相当・`FUN_00452C20` は残点／V-492-2 `param_3=1` を machine 確認・`DAT_00B33AAC`＝第2 marker singleton 確定相当／V-492-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_0045B670` decompiled が gate 3連＋`FUN_00452C20` 呼び出し**で「gated 実行体」と即決 → (ii) **callsite の machine push 数と `mov ecx,[global]`/`push`/`push`/`mov ecx,eax` の列**で **arg3＝`1` と docs 表示一致**（v491 残点の machine 確認を完遂）→ (iii) **`DAT_00B33AAC` の構築 pattern（alloc 0x7C→`call 0x46C300`→`virtual+0xD8(名前)`→`FUN_004B2B80` list link）が v470 NorthMarker と同一**で第2 marker と即決。
- 妥当性チェック: ①`DAT_00B33A98+0xCD4`⇔global gate（v474 TLS pattern と同型の per-thread/global 混在）— ②`param_2+8` bit14⇔v473 bit 判定系（`>>14 &1`）— ③`push 1; push ecx; mov ecx,eax`⇔thiscall 3引数（docs 表示2引数＋implicit this）— ④alloc `0x7C`⇔v470 NorthMarker alloc 系（`0x7C`=124B）— ⑤`FUN_004B2B80`⇔v470 list link fn — **五方向矛盾なし**。
- docs との不整合: **①v491 ruling「`param_3` は docs 欠落＝machine 要確認」→ 本裁定で machine 確認完遂（`param_3=1`・docs 表示は2引数＋implicit this の形式で矛盾なし）／②docs L424183 表示 `(DAT_00b33aac, 1)` の引数順は machine（param_2, param_3）と一致 [確認済み]**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **gate 3連（TLS byte2＋global＋flag bit）＋本体呼び出しの fn は「gated 実行体」の定型**（bit 変更→notify→gated 実行の流れが完成）。
  2. **callsite の machine push 並び（`mov ecx,global; push imm; push reg; mov ecx,eax`）が引数対応を確定させる** — **docs 表示（2引数）と machine（thiscall 3引数）の対応を突合**（v491 残点の解消法）。
  3. **singleton 構築 pattern（alloc→ctor→`+0xD8` set name→list link）が再現**されるたびに「marker 系 singleton」と即分かる（v470→v492 の再現）。
  4. **push 数 census（4push vs 2push）が別呼び出し形を分離**する（`0x4D0E74` site の残点化根拠）。
  5. **mode 判定 fn（`FUN_0045A500`）が gate 群の中にある**と分かれば、次の probe で mode/bitable が揃う（残点の方向を明示）。
