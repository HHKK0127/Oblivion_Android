# v645 — V-643-1 解決: `[esp+0x1c]` は `[esp+0x20]` と同一スロット（esp +4 シフト）

- **日付**: 2026-11-23
- **帯域**: HandleClick `FUN_005cc200`（menu id 0x40C RaceSexMenu）
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 探索 = rizin 0.9.1（`D:\re\r2q.sh`）。デコード最終確認 = capstone（`v307_common.py`）。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。範囲は半開区間。算術は python（probe u146_1〜u146_6）で検算。

## §0 結論（要旨）

- **V-643-1 は `[未確定]` → `[確定(命令)]` に解決**。`[esp+0x1c]` は未初期化ではない。`fstp qword [esp+0x20]`（保存）と `fcomp/fld qword [esp+0x1c]`（比較）は **実効アドレスが同一**（入口基準 `esp-0x1c`）である。差は **2 回目 GetFloat の `ret 4` が push されていない引数を pop するため esp が +4 ずれる**ことによる。
- したがって 19 fcomp サイトのガードは **`GetFloat(根tile, 0xFAE) == GetFloat(paneN, 0xFA8)`**（根 tile の user0 = 現在ページ id、pane の id）。
- `GetFloat`(0x588BD0) の実体 = `float GetFloat(Tile* ecx, int trait [esp+4])`、**stdcall `ret 4`**、戻り値 `fld dword [edx+4]`（32bit float）。

## §1 — #1: 48 件の内訳と前後分割 `[確定(命令)]`

範囲 `[0x5CC200, 0x5CDEF0)`。rizin `/x 241c` = 48 件。capstone で全件分類（`u146_1.py`）。

| 位置 | 件数 | 内訳 |
|---|---|---|
| `0x5CCB0E` より前 | **20** | `fcomp qword [esp+0x1c]` ×19（`0x5CC2C7`…`0x5CC807`）＋ `fld qword [esp+0x1c]` ×1（`0x5CC92B`） |
| `0x5CCB0E`（書込） | **1** | `mov dword [esp+0x1c], ebx`（`89 5c 24 1c`） |
| `0x5CCB0E` より後 | **28** | `fld qword [esp+0x1c]` ×27（`0x5CCC83`…`0x5CD986`）＋ `mov` ×1（上記） |

- **読 47 / 書 1**。書は 19 fcomp より後。`fstp qword [esp+0x1c]` は 0 件。

## §2 — #2: prologue 全命令（`0x5CC200..0x5CC2C8`）`[確定(命令)]`

```
0x5CC200  55                 push ebp
0x5CC201  8bec               mov  ebp, esp
0x5CC203  83e4f8             and  esp, 0xfffffff8      ; 16B アライン
0x5CC206  6aff               push -1                  ; SEH
0x5CC208  68381b9c00         push 0x9c1b38            ; SEH
0x5CC20D  64a100000000       mov  eax, fs:[0]
0x5CC213  50                 push eax                 ; SEH（3 push）
0x5CC214  83ec18             sub  esp, 0x18
0x5CC217  53                 push ebx
0x5CC218  56                 push esi
0x5CC219  57                 push edi
0x5CC21A  a1ac0ab300         mov  eax, [0xb30aac]
0x5CC21F  33c4               xor  eax, esp             ; cookie
0x5CC221  50                 push eax
0x5CC222  8d442428           lea  eax, [esp+0x28]
0x5CC226  64a300000000       mov  fs:[0], eax
0x5CC22C  8bf1               mov  esi, ecx            ; esi = this
0x5CC22E  8b4e30             mov  ecx, [esi+0x30]     ; race_name
0x5CC231  d9e8               fld1
0x5CC233  51                 push ecx
0x5CC234  d91c24             fstp dword [esp]         ; 1.0f
0x5CC237  68f00f0000         push 0xff0               ; xdefault
0x5CC23C  c644241b00         mov  byte [esp+0x1b], 0
0x5CC241  c686d008000000     mov  byte [esi+0x8d0], 0
0x5CC248  e8630cfcff         call 0x58ceb0            ; SetFloat(race_name,0xff0,1.0)
0x5CC24D  8b8eec080000       mov  ecx, [esi+0x8ec]
0x5CC253  e89810fbff         call 0x57d2f0
0x5CC258  84c0               test al, al
0x5CC25A  8b5d08             mov  ebx, [ebp+8]        ; arg1 = クリック id
0x5CC25D  0f8440060000       je   0x5cc8a3
0x5CC263  8b8eec080000       mov  ecx, [esi+0x8ec]
0x5CC269  6a00               push 0
0x5CC26B  e8201bfbff         call 0x57dd90
0x5CC270  8bce               mov  ecx, esi
0x5CC272  e8496effff         call 0x5c30c0
0x5CC277  83fb5a             cmp  ebx, 0x5a
0x5CC27A  0f853a060000       jne  0x5cc8ba
0x5CC280  a1708fb300         mov  eax, [0xb38f70]
0x5CC285  68a80f0000         push 0xfa8               ; ← 2 回目 GetFloat 用に残存
0x5CC28A  83ec08             sub  esp, 8
0x5CC28D  8bcc               mov  ecx, esp
0x5CC28F  33db               xor  ebx, ebx
0x5CC291  89642420           mov  [esp+0x20], esp
0x5CC295  53                 push ebx
0x5CC296  50                 push eax
0x5CC297  8919               mov  [ecx], ebx
0x5CC299  66895904           mov  word [ecx+4], bx
0x5CC29D  66895906           mov  word [ecx+6], bx
0x5CC2A1  e82a66e3ff         call 0x4028d0            ; ret 8
0x5CC2A6  8bce               mov  ecx, esi
0x5CC2A8  e8636effff         call 0x5c3110            ; ret 8
0x5CC2AD  8b4e04             mov  ecx, [esi+4]        ; Menu::tile（根 tile）
0x5CC2B0  68ae0f0000         push 0xfae               ; user0
0x5CC2B5  8bf8               mov  edi, eax            ; edi = paneN
0x5CC2B7  e814c9fbff         call 0x588bd0            ; GetFloat(根tile,0xFAE)
0x5CC2BC  dd5c2420           fstp qword [esp+0x20]    ; 保存
0x5CC2C0  8bcf               mov  ecx, edi
0x5CC2C2  e809c9fbff         call 0x588bd0            ; GetFloat(paneN,0xFA8) ← push 無し
0x5CC2C7  dc5c241c           fcomp qword [esp+0x1c]   ; 保存先と同一実効
```

- **push 15・`sub esp` 2**（`0x18` と `8`）。`[esp+0x1c]` 用の 8B 初期化は **存在しない**。
- `[esp+0x1c]` を書くのは `0x5CCB0E` の 1 件のみで、これは BSStringT temp（`{data@0x1c,len@0x20,cap@0x22}`）構築。

## §3 — #3: esp 差分積算と実効アドレス `[確定(命令)]`

callee の `ret N`（`u146_2.py` / `u146_3.py`）:

| 関数 | ret |
|---|---|
| `0x588BD0` GetFloat | **`ret 4`**（`0x588BF9 c20400`） |
| `0x58CEB0` SetFloat | `ret 8`（`0x58CECD`） |
| `0x57D2F0` | `ret 0`（`0x57D2F2 c3`） |
| `0x57DD90` | `ret 4` |
| `0x5C30C0` | `ret 0`（`0x5C310A c3`） |
| `0x4028D0` | `ret 8` |
| `0x5C3110` | `ret 8`（`0x5C316A`/`0x5C31A0`/`0x5C31D6`） |

スタック厳密シミュレーション（`u146_4.py`、push 値と esp オフセットを追跡）:

| 地点 | esp（入口基準） | 命令 | 実効アドレス |
|---|---|---|---|
| `0x5CC2BC` | **-60** | `fstp qword [esp+0x20]` | -60+0x20 = **-28** |
| `0x5CC2C7` | **-56** | `fcomp qword [esp+0x1c]` | -56+0x1c = **-28** |

- **両者 -28 で一致**。差 +4 は `0x5CC2C2` の GetFloat（`ret 4`）が push されていない引数を pop するため。
- 各 GetFloat の引数（シミュレータ出力）:
  - `0x5CC2B7`: **arg1 = `0xfae`**（`0x5CC2B0 push 0xfae`）、`ecx = [esi+4]`（根 tile）
  - `0x5CC2C2`: **arg1 = `0xfa8`**（`0x5CC285 push 0xfa8` が残存）、`ecx = edi`（paneN）
- → ガード = **`GetFloat(根tile, 0xFAE=user0) == GetFloat(paneN, 0xFA8=id)`**。不一致で `jnp` skip。

## §4 — #4: 後方ジャンプ `[確定(命令)]`

`0x5CC200..0x5CDEF0` 内の jmp/jcc で target も同範囲内のもの（`u146_6.py`）: **後方 30 件**。

- **`[0x5CC2C8, 0x5CC820)`（fcomp 帯）を target にする後方ジャンプは 0 件** → 19 fcomp サイトは「読が全部先・ループ無し」で確定。
- ただし **fld 帯（`0x5CCC83..0x5CD988`）はループ本体**。`0x5CD057`/`0x5CD0DD`/`0x5CD263`/… の 19 個の `jmp 0x5CCFB7`（および `0x5CD15D`/`0x5CD1DD` の `jmp 0x5CCFB2`）で反復する。
- `0x5CC92B` の `fld qword [esp+0x1c]` は別ガード:
  ```
  0x5CC915  fild dword [ebp+8]        ; arg1（クリック id）を double 化
  0x5CC91A  fstp dword [esp+0x18]
  0x5CC91E  fld  dword [esp+0x18]
  0x5CC922  fstp qword [esp+0x20]     ; 保存（arg1 の double）
  0x5CC926  call 0x588bd0             ; GetFloat(ecx, …)（ret 4・push 無し）
  0x5CC92B  fld  qword [esp+0x1c]     ; ← +4 シフトで保存先と同一
  0x5CC92F  fucompp
  ```
  → 「**GetFloat(tile, trait) == float(arg1=クリック id)**」の比較（v642 のページ選択モデルと整合）。

## §5 — #5: id 98 のディスパッチャ `[確定(命令)]`

- `0x89957A`: `call dword [eax+0xc]`（汎用 vtable slot3 呼出。`ebp` フレーム・別クラス。RaceSexMenu 帯 0x5Cxxxx ではない）。直後 `mov [ebp+0x1c], eax`。
- `push 0xfa8` in `[0x899000,0x89A000)`: **0 件**。
- `push 0x62` 全 `.text`: **0 件**。

→ id 98 は静的定数として存在しない（`[部分確定]` 維持）。`/r 0x5cc200` = 0 hits（vtable `0xA6D398` slot3 経由のみ）。

## §6 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-643-1 | `[esp+0x1c]` は `[esp+0x20]` と同一（esp +4 シフト）＝未初期化でない | **解決 `[確定(命令)]`** |
| V-642-1 | id 98（定数なし＝実行時 id か入力コード） | `[部分確定]` |
| V-641-1 | 24 サイト × クリック id の完全対応 | `[部分確定]` |
| V-645-1 | fld 帯（`0x5CCC83..0x5CD988`）の反復ループが何を走査するか（19 `jmp 0x5CCFB7`） | `[未確定]` |
| V-645-2 | `0x5CC92B` の `GetFloat(ecx, trait)` の trait 実体（`ecx=0x5c3440` 戻り） | `[未確定]` |

## §7 — 検証手段（コピペ可）

```sh
sh /mnt/d/re/r2q.sh -c "e search.from=0x5CC200" -c "e search.to=0x5CDEF0" -c "/x 241c" -c "q" /mnt/d/re/Oblivion.exe
sh /mnt/d/re/r2q.sh -c "pD 0xa8 @ 0x5cc200" -c "q" /mnt/d/re/Oblivion.exe
sh /mnt/d/re/r2q.sh -c "pD 0x60 @ 0x5cca77" -c "q" /mnt/d/re/Oblivion.exe
python D:\re\u146_1.py   # 48 件分類 + prologue
python D:\re\u146_4.py   # スタック厳密シミュレーション
python D:\re\u146_6.py   # 後方ジャンプ
```
