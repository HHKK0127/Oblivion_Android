# v408 — `0x73A5E0` の他システム側 callers3本の `sub [global],1` の意味の裁定（#153 ⑥-12・V-389-1）

- 採番: **v408**（ai-todo #153 束の最終指定件・probe prefix **v408_1 全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v389 の残点 **V-389-1（`0x73A5E0`＝NiTSet::Add の callers のうち `sub [global],1` が直後にある3本＝`0x56BB1F`/`0x6EBFDF`/`0x75E39F`（各 0xB3A67C/0xB3ECFC/0xB41DFC をデクリメンタ）の意味）【低】** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`sub [global],1` の意味（3本共通）** | **[確定(decompiled＋machine)]** | **refcount ではない — `EnterCriticalSection` 対の再入深度（lock recursion depth）のデクリメント**。3本は**完全同一テンプレート**: `EnterCriticalSection(&DAT_set_CS)` → `_DAT_owner = GetCurrentThreadId()` → **`_DAT_depth += 1`** → `FUN_0073A5E0(&local)`（set 追加）→ **`_DAT_depth -= 1`（＝本残点の sub）** → `if (depth==0) _DAT_owner = 0` → `LeaveCriticalSection(&DAT_set_CS)`。machine と decompiled が1:1（例 `FUN_0056BAF0` L271230-271243） |
| **3本の fn の共通構造** | **[確定(命令)]** | 追加先 set（`FUN_0073A5E0` の this＝ecx）は **`0xB12B9C`（0x56BB1F）/ `0xB24FE8`（0x6EBFDF）/ `0xB28C5C`（0x75E39F）** のグローバル NiTSet（{array,cap,count}・v389 layout 一致）。関連グローバルは各 set で **CS(`0xB3A600`/`0xB3EC80`/`0xB41D80`)・owner(`0xB3A678`/`0xB3ECF8`/`0xB41DF8`)・depth(`0xB3A67C`/`0xB3ECFC`/`0xB41DFC`) の3点セット** |
| **depth カウンタの共有（副次）** | **[確定(命令)]** | 各 depth は **2つの fn で共有**される（census: 各4 refs＝同一 fn 内 +1/-1 が2組）— `FUN_0056BAF0`（Add ラッパ）と **lazy-init 兄弟 fn**（`0x56BBA1` 帯: `cmp [0xB12BA4],0 → mov eax,[0xB12BA8]; mov ecx,0xB12B9C` ＝セットの遅延初期化＋同じ depth 追跡） |
| **v389 の「グローバル refcount を減らす」記述** | **訂正 [確定]** | `sub [global],1` は refcount 減算ではなく **Enter/Leave 対の depth 通過**（+1→Add→-1 の対称）。加算側 `add [global],1` も同様に depth であり、リソース寿命管理ではない |

## §2 証拠（v408_1）

### §2.1 decompiled（3本とも同型）
```c
// FUN_0056BAF0 (L271230-271243) / FUN_006EBFB0 (L524766-524779) / FUN_0075E370 (L603102-603115)
EnterCriticalSection((LPCRITICAL_SECTION)&DAT_00B3A600);   // (CS は set ごとに別: 0xB3EC80 / 0xB41D80)
_DAT_00B3A678 = GetCurrentThreadId();                      // owner thread 記録
_DAT_00B3A67C = _DAT_00B3A67C + 1;                         // depth +1
FUN_0073a5e0(&param_1);                                    // NiTSet::Add（this は machine で ecx=set）
_DAT_00B3A67C = _DAT_00B3A67C + -1;                        // ★ depth -1（本残点の sub）
if (_DAT_00B3A67C == 0) { _DAT_00B3A678 = 0; }             // depth==0 で owner 解除
LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_00B3A600);
```
**owner クリア条件が depth==0 であること自体が「refcount ではなく lock depth」の証明**（refcount なら owner リセット概念が不要）。

### §2.2 machine（3本・テンプレート一致）
```
0x56BAF0: push 0xB3A600; call [0xA2806C](EnterCS); call [0xA2808C](GetThreadId)
          add [0xB3A67C],1; mov [0xB3A678],eax
          lea ecx,[esp+4]; mov eax,[esp+4]; push ecx; mov ecx,0xB12B9C; mov [esp+8],eax
0x56BB1F: call 0x73A5E0            ← this=0xB12B9C（グローバル set）
0x56BB24: sub [0xB3A67C],1; jne; mov [0xB3A678],0
0x56BB37: push 0xB3A600; call [0xA28074](LeaveCS); ret
（0x6EBFB0→CS 0xB3EC80/set 0xB24FE8・0x75E370→CS 0xB41D80/set 0xB28C5C も同一並び）
```

### §2.3 depth 共有 census（各4 refs）
- `0xB3A67C`: `0x56BB01(+1)`/`0x56BB26(-1)`＝FUN_0056BAF0 ＋ `0x56BBA1(+1)`/`0x56BBF3(-1)`＝**lazy-init 兄弟**（`cmp dword [0xB12BA4],0; jne; mov eax,[0xB12BA8]; mov ecx,0xB12B9C` = set 未初期化なら作るパターン）。
- `0xB3ECFC`（0x6EBF31/0x6EBF83＋0x6EBFC3/0x6EBFE6・兄弟の set 参照 0xB24FF0/0xB24FF4）、`0xB41DFC`（0x75E2F1/0x75E343＋0x75E381/0x75E3A6・0xB28C64/0xB28C68）も同一構造。

## §3 残点 **V-408-1..2**
1. 【低】3 fn（`FUN_0056BAF0`/`FUN_006EBFB0`/`FUN_0075E370`）の identity — `param_1`（登録値）の意味と呼び出し元（E8 callers）、および set 本体 `0xB12B9C`/`0xB24FE8`/`0xB28C5C` がどのシステムの登録簿か（CS の初期化元・lazy-init 兄弟の callers から逆算可）。
2. 【低】lazy-init 兄弟 fn（`0x56BB90` 帯/`0x6EBF20` 帯/`0x75E2E0` 帯・Ghidra 注記の有無未確認）の本体と `cmp [set+8]` フィールドの意味。

## §4 検証・自己批評
- probe: **v408_1 全 EXIT=0**（1本で充足）。
- **決定打の作り**: (i) 3 call site の ±UNFILTERED で「+1 → call → -1 → depth==0 で owner クリア → Leave」の対称構造を全件確認 → (ii) decompiled で `GetCurrentThreadId`/`LeaveCriticalSection` の存在を C 側で裏付け → (iii) depth カウンタの全 refs census（各4件＝2 fn 共有）で「refcount でない」ことの反証（owner リセットが depth==0 にある）→ (iv) this(ecx)＝set グローバルの実読。
- 妥当性チェック: ①machine(+1/-1/Leave)⇔decompiled(Enter/owner/+1/Add/-1/if0 owner=0/Leave) — ②3 site テンプレ一致（CS/owner/depth/set の4グローバルが set ごとに一貫）— ③`ret 4`（0x73A5E0）⇔1引数 push 形式 — **三方向矛盾なし**。
- docs との不整合: **v389 §残点の「グローバル refcount」表現は訂正**（実体＝lock 再入深度）。docs/GT への反映は #47/#20 一括。
- **教訓**: **「refcount のような見える `add/sub [global],1` は、その前後の Enter/Leave・GetCurrentThreadId・depth==0 クリアの有無で refcount と lock depth を切り分ける** — owner リセットが depth==0 に付くなら lock depth、Release/destroy が付くなら refcount。＋ Ghidra が thiscall の ecx（`mov ecx, imm`）を decompiled で落とすため、**set this の特定は machine の `mov ecx, imm` を見る**。
