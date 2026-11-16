# v424 — `[+0x1C]` の refcounted setter と attach 経路の裁定（V-424-1 部分解・派生残点第16弾）

- 採番: **v424**（派生残点群・probe prefix **v424_1..v424_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v423 の残点 **V-424-1【低】（`FUN_007074B0`（`[+0x1C]` setter）の E8 callers → `Bip01[+0x1C]` に格納される対象 → node `[+0xC]` との同一性）** を消化（v423 §1 の「single-line setter」表記は本裁定で**訂正**）。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`FUN_007074B0` の identity（v423 訂正）** | **[確定]**（v423「single-line setter」は**誤り・訂正**） | **refcounted pointer setter（`this[+0x1C] = new`・release 経路付き）**: `old=[esi+0x1c]` → null なら素通し store（`0x707517`）／非 null なら **`old->vtable slot34(+0x88)(&local, this)`（detach/query 呼出し）→ `InterlockedDecrement(&old->refcount)` → 0 なら `old->vtable[0](1)`（scalar dtor）→ store new`（`0x7074F0/0x70750A`）**。data refs=0＝virtual 登録なし（direct helper）[確定] |
| **E8 callers 6件の分類** | **[確定]** | ①**dtor 系（clear to 0）**: `FUN_006A0AC0`（`[esi+0x40][+0x1C]=0`＋`[esi+0x3c]=0`）／`FUN_006A1590`（同型）②**conditional replace**: `0x6A1B6A`（`mov ecx,[ebx]; cmp; push edi; call`）③**attach（`target[+0x1C] = this`）**: **`0x70AF00` fn** — `push ebx(this) / mov ecx,esi(arg1) / call 0x7074b0` → **`arg1[+0x1C] = this` [確定(命令)]**（直後 `lea ecx,[ebx+0xac]`＝this の別 container 使用）④**index-based set**: `FUN_0070B250` ×2 |
| **`FUN_0070B250` の semantics** | **[確定]** | `FUN_0070b250(param_1, out, index, new)`（L549983-550048）: **`child = *(param_1[+0xB0] + index*4)`（count=`u16[+0xB6]`・cap=`[+0xB4]`・offset=`u16[+0xBA]`）→ retain → `child[+0x1C] = 0`（`puVar1[7]=0`）→ `FUN_007074B0(param_1, param_4)`（＝`param_1[+0x1C] = param_4` refcounted）→ `FUN_004b34e0(index, &param_4)`（registry 更新）→ `*out = child`**／index 超過時は grow（`FUN_00523b10`）。**children 配列＝`[+0xB0]`（v405 の `children[+0xb0]` と整合）[確定]** |
| **`[+0x1C]` の性質** | **[確定]** | **refcounted な「関連オブジェクト ptr」フィールド**（attach／index-set／dtor clear の3経路で賦流・ctor0クリア）。**`FUN_0070B400` が `if ([param_1+0x1c]!=0)` で再帰的走査**（L550074-550083）＝`[+0x1C]` は連結構造の入口にもなる [確定] |
| **node `[+0xC]` と `Bip01[+0x1C][+0xC]` の同一性** | **[未確定]（質問を精緻化）** | `Bip01[+0x1C]` は **refcounted な「別のオブジェクト ptr」（self 也可能）** → 同一性は **「`Bip01[+0x1C] == Bip01`（self-attach）か否か」に絞られた**。self-attach の実在（`0x70AF00` fn の arg が self で呼ばれるか）が次の一手＝**残点 V-425-1** |

## §2 証拠（v424_1..v424_3）

### §2.1 `FUN_007074B0` body（machine 0x7074B0..0x707520・v424_1(c)）
```
7074B0: push ecx/esi; mov esi, ecx           ; this
7074B4: mov ecx, [esi+0x1c]                  ; old
7074B7: test / je 707517                     ; old==0 → 707517: store arg1 直通
7074BB: mov eax,[ecx]; mov eax,[eax+0x88]    ; old->slot34
7074C4: push esi; lea edx,[esp+0xc]; push edx; call eax  ; slot34(&local, this)
7074CC: mov edi,[esp+8] (result) / InterlockedDecrement(&refcount) →0 なら dtor(vtable[0](1))
7074F0/70750A/707517: mov [esi+0x1c], <arg>  ; store new（3経路）
7074FA: ret 4
```
- v424_1(a) の E8=6・(b) data refs=0。

### §2.2 callers の push 元（v424_2(a)）
- `0x6A0BB3`: `mov ecx,[esi+0x40]; push ebx(0); call` → **clear**／`0x6A1674` 同型。
- `0x6A1B6A`: `mov ecx,[ebx]; cmp ecx,edi; je; push edi; call` → **conditional replace**。
- `0x70B29D`/`0x70B354`（FUN_0070B250 内）: `push ebp; mov ecx,edi; call` → `edi[+0x1C]=ebp`。
- `0x70AF41`（fn `0x70AF00`）: `InterlockedIncrement(&arg1->refcount)` → `push ebx(this); mov ecx,esi; call` → **`arg1[+0x1C] = this`（attach）**・直後 `lea ecx,[ebx+0xac]`。

### §2.3 `FUN_0070B250` body（L549983-550048・v424_3(a)）
- `if (index < u16[param_1+0xb6]) { child = *(param_1[+0xb0] + index*4); retain; child[+0x1c]=0; FUN_007074b0(param_1, param_4); FUN_004b34e0(index,&param_4); *out = child; }` ／else 分岐は grow（`u16[+0xb4] <= index → FUN_00523b10(u16[+0xba]+index)`）。
- **callers**: `FUN_005640E0`×3（0x5641B7/0x56424F/0x56430B）＋ `0x724700`（v424_3(b)）。

### §2.4 `FUN_0070B400`（v424_3(a) L550057-550087）
- `if (*(param_1+0x1c) != 0) { uVar1 = FUN_0070b400(&local_14); FUN_0055e2a0(uVar1); ... }` → **`[+0x1C]` を再帰的に辿る処理が存在** [確定]。

## §3 残点 **V-425-1..2**
1. **【低】`0x70AF00` fn の identity（E8 callers・decompiled）と self-attach の実在** — `arg1[+0x1C]=this` の arg1 が self（==this）で呼ばれる経路があれば **`Bip01[+0x1C]==Bip01` が成立し V-423-1 の同一性が閉じる**。＋`this[+0xAC]` container の正体。
2. 【低】`FUN_007074B0` の `old->slot34(+0x88)`（detach/query）の identity と、`FUN_004b34e0`（index registry）の意味。

## §4 検証・自己批評
- probe: **v424_1..v424_3 の3本全 EXIT=0**（V-424-1 は [部分確定]＝setter identity・callers 分類・`FUN_0070B250` semantics まで・同一性は質問を精緻化して残点）。
- **決定打の作り**: (i) **call 直前の push 元を全 site で復元**（clear/replace/attach/set の4用途に分類）→ (ii) **`FUN_007074B0` の body 全経路（null 直通／dec→dtor／dec→残存）を machine で復元**して refcounted setter と確定 → (iii) **decompiled body 実読で `FUN_0070B250` の child 配列・`child[+0x1C]=0`・`param_1[+0x1C]=param_4` を確定** → (iv) data refs=0 で virtual 登録否定。
- 妥当性チェック: ①`FUN_007074B0` の store3経路⇔callers の clear/attach/set — ②`FUN_0070B250` の `[+0xB0]`/`[+0xB6]`⇔v405 の `children[+0xb0]` — ③`child[+0x1C]=0`⇔dtor clear 経路⇔ctor0初期化（v423）— **三方向矛盾なし**。
- docs との不整合: **v423 ruling §1「`FUN_007074B0`＝single-line setter」は本裁定で訂正**（実体は refcounted setter・`0x70750F/0x70751B` は tail 経路）。docs/GT 反映時は v423 注記を併せて整理・#47/#20 一括。
- **教訓**:
  1. **「single-line setter」判定は head だけでなく body 全経路（null ブランチ含む）を読む** — tail の `mov [reg+disp], reg; ret` だけを見ると refcounted 全体像を見落とす（v423 の訂正事例）。
  2. **call 直前の push 元の分類（0＝clear／this＝attach／idx＝set）で callers の用途が即分かる**。
  3. **`puVar1[7] = 0`（dword index）＝`[+0x1C]` clear** — decompiled のインデックス表記は byte×4 換算でフィールドに直訳する。
