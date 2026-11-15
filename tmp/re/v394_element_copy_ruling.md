# v394 — `0x6CC480`（`jmp 0x6CD3D0` thunk）の要素コピーの方向と内容の裁定（#153 ⑤・V-385-1）

- 採番: **v394**（ai-todo #153 束の第5弾、probe prefix v394_1..v394_5）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v385 の残点 **V-385-1（`0x6CC480`（= `jmp 0x6CD3D0`）の要素コピーの方向と内容: this=orig 要素／stack=new 要素／param=arg2 の意味。orig→new か双方向か）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| `0x6CC480` の identity | **[確定(命令＋decompiled)]** | **`FUN_006cd3d0` の tail-jump thunk（`jmp 0x6CD3D0`・5バイト）** — decompiled L500749 `// thunk_FUN_006cd3d0 @ 0x6cc480`／L500751 `void __thiscall thunk_FUN_006cd3d0(int param_1,int param_2,undefined4 param_3)`。E8 callers＝**`0x6D01B5` の1本のみ**（fn `0x6D0170`＝interpolator 間コピー内・v385 確定） |
| **方向** | **[確定(命令＋decompiled)]** | **`this`（ecx＝param_1＝orig）→ `stack1`（param_2＝new）の一方向（orig→new・双方向ではない）** — machine: `mov ebp,ecx` → `[stack1+0xc]←[ebp+0xc]` 系の全フィールドコピー、要素ループも `dst←src` のみで **orig 側は一切書き換わらない**。decompiled: `*(param_2+X)=*(param_1+X)`／`*piVar9=puVar6`（L501787-810）。**v385 の仮説「this=orig／stack=new／param=arg2」を確認・正** |
| call site `0x6D01B5` の配線 | **[確定(命令)]** | fn `0x6D0170` prologue: **`esi=ecx`（this＝orig）／`ebx=[esp+8]`（stack1＝new）／`ebp=[esp+0x10]`（stack2＝context）**。ループ: `ecx=[esi+0x3c]+i*0x30`（orig 要素）／`push ebp; push [ebx+0x3c]+i*0x30`（new 要素＝stack1）→ `call 0x6CC480`。先に `0x6D0010(ebx, this.[+0x44])` で **new 側配列を orig の count へ resize** |
| **内容** | **[確定(命令＋decompiled L501786-501830+)]** | ①`0x733850`（＝`jmp 0x700770` thunk）`(dest, context)` を最初に呼ぶ ②`dest[+0xc]←src[+0xc]`（flags byte）③`dest[+0xd]←src[+0xd]`（count byte）④**`dest[+0x14]←新配列`**: `new(count*0x18+4)`・ヘッダ dword=count・`_eh_vector_constructor_iterator_(0x18, count, FUN_006ccde0, FUN_007016a0)` でデフォルト構築 ⑤`dest[+0x1c]←src[+0x1c]`（f32）⑥**ゲート `if((src[+0xc]&1)==0)` のとき要素ループ**: 各0x18要素 `{refptr obj +0, f32 +4, f32 +8, u8 +0xc, f32 +0x10, f32 +0x14}` — `src_elem.obj→vtable[6](context)` の戻りを **refcount 遷移（旧 release＋新 InterlockedIncrement）で `dst_elem.obj` へ**、残り5フィールドは素コピー ⑦`dest[+0xe]/[+0xf]←`（byte）⑧`dest[+0x20]←`（f32）⑨`dest[+0x10]/[+0x11]←`（byte）。**`ret 8`**（thiscall・2 stack 引数）。**bit0 SET 時は⑥の要素ループを省略**（ヘッダ/フロートのみ） |
| **param_3（arg2＝context）の意味** | **[確定(命令)]** | (a) **`0x700770(dest, context)` の第2引数** (b) **各 orig 要素 obj の vtable slot6（+0x18）の引数** → 戻り値が new 要素に格納。`0x6D0170` 経由の値＝**`0x6D0170` の stack2（ebp）＝slot6 に渡された「arg」**（v385 の記述と一致）。その実体は呼び出し元次第＝残点 |
| Ghidra call 表示の癖 | **[確定(観察)]** | 本帯の decompiled call 表示は **hidden this（ecx）を省略**（例 L503845 `thunk_FUN_006cd3d0(param_2-elem, param_3)` ＝ (stack1, stack2) のみ）。**call site の判定は機械語の `push/push/mov ecx` が正** |

**V-385-1 への回答**: **orig→new の一方向**（this=orig 要素・stack1=new 要素・param_3=arg2＝context は「orig 要素 obj の slot6 clone 引数」）。双方向ではない。コピー内容は上記①〜⑨の完全リスト。

---

## §2 証拠

### §2.1 thunk identity（v394_1/2/3）

- `0x6CC480: jmp 0x6CD3D0`（後続 int3 パディング＝独立 fn entry）。
- decompiled **L500749 `// thunk_FUN_006cd3d0 @ 0x6cc480`**／本体 def **L501759 `// FUN_006cd3d0 @ 0x6cd3d0`・L501761 `void __thiscall FUN_006cd3d0(int param_1,int param_2,undefined4 param_3)`**。
- E8→`0x6CC480`: **`0x6D01B5` のみ**。E8→`0x6CD3D0`: `0x6CC462, 0x6CF4C0, 0x6D280B, 0x6EA784, 0x6EACC0, 0x6EB318, 0x6EB81A`。

### §2.2 本体の prologue・引数スロット（v394_1(b)/v394_3(c)）

```
6CD3D0: push -1; push 0x9afaab; push fs:[0]; sub esp,8
6CD3E1: push ebx/ebp/esi/edi; push cookie(6CD3EC)   ; 合計 entry-0x28
6CD3F7: mov ebp, ecx              ; ★ebp = this = param_1（source）
6CD3F9: eax = [esp+0x30] = entry+8  = stack2（param_3 / context）
6CD3FD: ebx = [esp+0x2c] = entry+4  = stack1（param_2 / dest）
6CD401: push eax; push ebx; call 0x733850（jmp 0x700770）
6CD408: cl = [ebp+0xc] → [ebx+0xc]   ; ★dest ← this（方向の直接証明）
...
6CD55C: add esp,0x14; ret 8        ; thiscall・2 stack 引数
```

### §2.3 コピー内容の全文（decompiled L501786-501830 ＋ machine tail）

```
L501786: thunk_FUN_00700770(param_2, param_3)            ; 0x733850(dest, ctx)
L501787: dest[+0xc] = src[+0xc]                           ; flags
L501788: dest[+0xd] = src[+0xd]                           ; count(byte)
L501789-96: new(count*0x18+4); hdr=count;
            _eh_vector_constructor_iterator_(elems,0x18,count,FUN_006ccde0,FUN_007016a0)
L501798: dest[+0x14] = new_array
L501799: dest[+0x1c] = src[+0x1c]                         ; f32
L501801: if ((src[+0xc] & 1) == 0) {                      ; ★ゲート
L501803:   for i < src[+0xd]:
L501806:     src_elem = src[+0x14]+i*0x18; dst_elem = dest[+0x14]+i*0x18
L501810:     if (src_elem.obj) dst_elem.obj = src_elem.obj->vtable[6](param_3)
                        ; 旧 dst release(InterlockedDecrement→delete)／新 ptr Increment
L501818+:   dst_elem[+4,+8,+0xc,+0x10,+0x14] = src_elem 同（f32,f32,u8,f32,f32）
          }
machine tail: dest[+0xe]←src[+0xe]; dest[+0xf]←src[+0xf]; dest[+0x20]←src[+0x20](f32);
              dest[+0x10]←src[+0x10]; dest[+0x11]←src[+0x11]（byte）
```

### §2.4 call site `0x6D01B5`（v394_3(b)/v394_4(a)）

```
6D0170: push ebx; ebx=[esp+8]=stack1        ; ★ebx = arg1 = new
6D0175: push ebp; ebp=[esp+0x10]=stack2     ; ★ebp = arg2 = context
6D017E: mov esi, ecx                        ; ★esi = this = orig
6D0180: call 0x6D0530(this, new, ctx)       ; ベース側コピー [部分確定]
6D0185: push [esi+0x44]（orig count）; ecx=ebx; call 0x6D0010  ; new 配列 resize
6D01A0: loop: ecx=[ebx+0x3c]+i*0x30（new 要素）
6D01AE: push ebp（ctx）; push ecx（new 要素＝stack1）
6D01B0: ecx=[esi+0x3c]+i*0x30（orig 要素＝this）
6D01B5: call 0x6CC480                       ; ★orig→new
```

- decompiled L503833 `// FUN_006d0170 @ 0x6d0170`・L503845 `thunk_FUN_006cd3d0(*(param_2+0x3c)+i*0x30, param_3)`（**表示は stack 引数のみ＝hidden this 省略**・機械語と整合）。

### §2.5 factory `FUN_006cc400`（v394_4(b)/v394_5(b)）— 方向の傍証

```
6CC423: mov edi, ecx          ; ★entry ecx（＝呼び出し元の orig）を保持
6CC425: push 0x30; call new;  ; fresh = new(0x30)
6CC441: ecx=esi; call 0x6CC4E0（ctor）; [esi]=vtable 0xA79E54
6CC452: eax=[esp+0x20]=stack1（context）
6CC456: push eax; push esi    ; stack1=fresh(DEST), stack2=context
6CC458: mov ecx, edi          ; ★ecx = orig（SOURCE）
6CC462: call 0x6CD3D0
6CC467: mov eax, esi          ; return fresh
```
→ **factory＝`orig から fresh へ deep copy して返す`**（方向の傍証）。

### §2.6 補助 fn

- `0x733850`: **`jmp 0x700770`**（5バイト thunk・E8 7本）＝decompiled 名 `thunk_FUN_00700770`。役割は [未確定]（残点）。
- `FUN_006ccde0`（要素デフォルト ctor・head `0x6CCCC0`）／`FUN_007016a0`（dtor 側・`[ecx]` null チェック＋`Interlocked` 系）＝`_eh_vector_constructor_iterator_` の対。

## §3 残点 **V-394-1..3**
1. **context（param_3/arg2）の実体**: slot6 呼び出し元から何が渡されるか（v385 の slot6「arg」の中身）＋**`0x700770`（`0x733850` jmp 先）の役割**（dest への事前処理の意味）【中】
2. **`[+0xc]` bit0 ゲートの意味**（set 時に deep copy をスキップする条件の正体）＋**container（0x30-byte interpolator・`[+0x44]` count word・`[+0x3c]` 配列）と0x18要素 `{obj*,f32,f32,u8,f32,f32}` の正式型名**【中】
3. **factory `FUN_006cc400` の callers と vtable `0xA79E54` の型**（どのクラスの Clone factory か）【低】

## §4 検証・自己批評
- probe: **v394_1..v394_5 全 EXIT=0**。
- **決定打の作り**: (i) thunk の実バイト＋decompiled thunk 注記 → (ii) **本体 prologue の SEH push 数から引数スロット逆算**（cookie 漏れに注意）→ (iii) `mov cl,[ebp+0xc]; mov [ebx+0xc],cl` で方向を直接確認 → (iv) **decompiled 全文でコピー内容を確定** → (v) **call site `0x6D01B5` prologue（esi/ebx/ebp）で orig/new/context を確定** → (vi) factory（ecx 保持→fresh 返却）で傍証。
- 妥当性チェック: ①方向 machine⇔decompiled（param_1→param_2）一致 — ②`ret 8`⇔decompiled 3引数（this＋2 stack）— ③`0x6D0010` resize⇔new 側 count 整合 — ④Ghidra call 表示（this 省略）⇔機械語の push 順が全て一致 — ⑤v385 仮説（this=orig/stack=new/param=arg2）と**三方向で矛盾なし**。
- docs との不整合: **なし**（v385 の V-385-1 を解消。仮説は正しく確認）。
- **教訓**: ①**thiscall call site は `push A; push B; mov ecx, Z; call` の機械語で判定**（Ghidra の call 表示は hidden this を省略しうる）②**SEH prologue（-1/seh/fs/cookie）を数えて引数スロットを逆算**する。
- L1 制約: docs/GT/ESM 変更なし（decompiled.c 読み取りのみ）・rust/ 側ソース変更なし。
