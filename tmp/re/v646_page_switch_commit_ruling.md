# v646 — ページ切替の実体: 19 サイトの commit ブロックと `0x5CC92B` ガード

- **日付**: 2026-11-23
- **帯域**: HandleClick `FUN_005cc200`（menu id 0x40C RaceSexMenu）
- **判定語彙**: `[確定(命令)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: 探索 = rizin 0.9.1（`D:\re\r2q.sh`）。デコード最終確認 = capstone（`v307_common.py`）。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。範囲は半開区間。算術は python（probe u147_1〜u147_4）。

## §0 `jnp` の向き（v645 の訂正）`[確定(命令)]`

`test ah,0x44` 後の `jnp` は **一致でジャンプ**（v645 の「不一致で skip」は誤り）。

| fcomp 結果 | C3 C2 C0 | `ah & 0x44` | PF | `jnp` |
|---|---|---|---|---|
| 等しい | 1 0 0 | 0x40（1 bit） | 0 | **飛ぶ** |
| 不等 | 0 0 0 | 0x00（0 bit） | 1 | 落ちる |
| 順序外 | 1 1 0 | 0x44（2 bit） | 1 | 落ちる |

→ 19 サイトのガードは **「paneN が現在ページなら commit ブロックへ」**。

## §1 — #1: `this->tile`(+4) の writer `[確定(命令)]（negative）`

band `[0x5C2000, 0x5CE600)` で `mov dword [reg+4], reg` を全走査（`u147_2.py`、221 件）。うち base が `esi` のものは **2 件のみ**:

| addr | 命令 | 実体 |
|---|---|---|
| `0x5C2C57` | `mov dword [esi+4], ecx` | `mov [esi],eax; mov [esi+4],ecx; mov [esi+8],edx` の 3 dword コピー（ベクトル/構造体。menu ではない） |
| `0x5C2D2B` | `mov dword [esi+4], eax` | 同上系（構造体コピー） |

- 残りの `[ecx+4]` は **BSStringT temp**（`ecx = esp`）が大半。
- → **RaceSexMenu band に「切替時に `this->tile` を新しい pane へ代入する」store は存在しない**。`this->tile` は menu 生成時に確定（framework）。
- ページ切替は `this->tile` の **user0 (0xFAE)** を書き換える（§3 の commit ブロック）。→ `this->tile.user0 = 現在ページ id`。

## §2 — #2: `0x5CC92B` のガード `[確定(命令)]`

到達経路: `0x5CC25D je 0x5cc8a3` → `0x5CC8A3 cmp ebx,0xa; jne 0x5cc277`（ebx==10 で `0x5CC8AC call 0x5c2730; call 0x5c30c0`）→ `0x5CC8BA`（デフォルト経路も合流）。

```
0x5CC8BA mov  eax, [0xb38ff0]     ; 名前
0x5CC8BF push 0xfa8               ; ← GetFloat 用に残存
0x5CC8C4 sub  esp,8               ; BSStringT temp 構築
0x5CC8DB call 0x4028d0            ; ret 8
0x5CC8E0 mov  eax, [0xb38f80]
0x5CC8E5 sub  esp,8
0x5CC902 call 0x4028d0            ; ret 8
0x5CC907 or   edi, -1
0x5CC90A mov  ecx, esi
0x5CC90C mov  [esp+0x44], edi
0x5CC910 call 0x5c3440            ; ret 0x10（4 args）→ eax = tile
0x5CC915 fild dword [ebp+8]       ; arg1（クリック id）→ ST0
0x5CC918 mov  ecx, eax
0x5CC91A fstp dword [esp+0x18]
0x5CC91E fld  dword [esp+0x18]
0x5CC922 fstp qword [esp+0x20]    ; 保存（double(arg1)）
0x5CC926 call 0x588bd0            ; GetFloat(tile, 0xFA8)  ← 引数 0xfa8（残存）
0x5CC92B fld  qword [esp+0x1c]    ; +4 シフトで保存先と同一
0x5CC92F fucompp
0x5CC931 fnstsw ax
0x5CC933 test ah, 0x44
0x5CC936 jp   0x5ccbf4            ; 不等/順序外 → 0x5ccbf4
0x5CC93C cmp  byte [0xb3b4c8], 0  ; 一致時
0x5CC943 jne  0x5ccbe5
0x5CC966 call 0x579c10            ; ダイアログ登録
0x5CC97A jmp  0x5cd9c5            ; finalize
```

- **trait 定数 = `0xFA8`（id）**（`u147_3.py` の stack sim: `0x5CC926 CALL arg1=[-4]=0xfa8`）。
- **ガード = `GetFloat(tile_0x5c3440, 0xFA8) == double(arg1=クリック id)`**。一致 → `0x5CC93C`（確認ダイアログ `0x579c10`）。不一致/順序外 → `0x5ccbf4`。
- `0x5c3440`（ret 0x10、ecx=this、4 引数）は BSStringT を構築し `0x4028d0` を呼んで tile を返す。→ このガードが「クリック id を持つ tile の選択/確認」の公算（遷移表の基準候補）。

## §3 — #3: 19 サイトの `jnp` 先と guard 直後 `[確定(命令)]`

19 fcomp サイトの `jnp` 先（`u147_1.py`）:

| サイト | パネル | jnp 先 |
|---|---|---|
| 1 | `0x5CC2A8`（Main） | `0x5cca77` |
| 2-3 | `0x5CC2FC` / `0x5CC350` | `0x5cca09` |
| 4-13 | `0x5CC399`…`0x5CC697` | `0x5cc9c4` |
| 14-19 | `0x5CC6E1`…`0x5CC807` | `0x5cc97f` |

- **guard 直後は「次のサイトのセットアップ」**（site1: `0x5CC2D6 mov eax,[0xb38fe0]; push 0xfa8; …; call 0x5c3110`）。＝ 19 サイトは**直列チェーン**（paneN が現在ページなら commit へ、そうでなければ次サイトへ）。
- **jnp 先 4 ブロック（commit）は `SetFloat(this->tile, 0xFAE, GetFloat(paneX, 0xFA8))`** を実行（`u147_3.py` sim）:
  - `0x5CC97F`: `GetFloat(pane,0xFA8)` → `SetFloat(this->tile, 0xFAE, その値)`（`0x5CC9A4` / `0x5CC9B4`）
  - `0x5CC9C4` / `0x5CCA09`: 同型（name `[0xb38fe0]` / `[0xb38f80]`）
  - `0x5CCA77`: 名前検証（`GetString(race_name, 0xFDE)` の空白チェック）＋ Done 経路
- **site19 の guard 直後（`0x5CC816`）= フォールスルー末尾**: `SetFloat(race_button_done, 0xFB3, 1.0)` → `SetFloat(this->tile, 0xFAE, GetFloat(Main, 0xFA8))`（Main へ切替）。
- → **19 サイトは「非表示処理」ではなく、paneN==現在ページ時の commit（`this->tile.user0` の設定）＋ 未一致時の Main 復帰**。`this->tile.user0` が現在ページ id の保持位置。

## §4 — #4: fld ループ（V-645-1）`[部分確定]`

- `jmp 0x5CCFB7` の行き先 `0x5CCFB7`:
  ```
  0x5CCFB2 mov  eax,[0xb38f70]      ; Main
  0x5CCFB7 sub  esp,8
  0x5CCFC0 push eax
  0x5CCFC1 call 0x419b10            ; BSStringT
  0x5CCFC6 mov  ecx,esi
  0x5CCFC8 mov  [esp+0x44],edi
  0x5CCFCC call 0x5c9650            ; Return 系
  0x5CCFD1 jmp  0x5cd9c5            ; finalize
  ```
- 反復ブロック（`0x5CD05C` / `0x5CD0E2` / … 19 個）は各々:
  ```
  0x5CD062 push 0xfa8; …; call 0x419b10
  0x5CD085 mov  [esp+0x48], 0xd     ; ← 反復ごとに増える定数（0xb,0xc,0xd,0xe,…）
  0x5CD098 call 0x5c3440            ; eax = tile
  0x5CD0A7 call 0x588bd0            ; GetFloat(tile, 0xFA8)
  0x5CD0AC fld  qword [esp+0x1c]
  0x5CD0B0 fucompp
  0x5CD0B4 test ah,0x44
  0x5CD0B7 jp   0x5cd0e2            ; 不等 → 次ブロック
  0x5CD0D5 mov  [esp+0x3c], 0xe
  0x5CD0DD jmp  0x5ccfb7            ; 一致 → 共通 finalize
  ```
- → 走査対象は **`0x5c3440` が返す tile**（`+0x40` 配列 / `+0x94` の直接読ではない）。1 反復ごとに **増分定数（0xb,0xc,0xd,0xe,…）を `[esp+0x3c/0x48]` に設定**。`visible` / `user0` の直接設定は本帯では未確認 → `[部分確定]`。
- 共通 finalize `0x5CD9C5`: `[0xb3b4c9]=0`（条件付き `[0xb3b4c8]=0`）→ `[0xb333c4].vtable[0x154]()` → `0x7b4280`/`0x7c5e70`/`0x7c7eb0`。

## §5 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-646-1 | `0x5c3440` の 4 引数と戻り tile の実体（`+0x40` 配列？） | `[未確定]` |
| V-646-2 | commit ブロックの `paneX` が site の paneN と異なる理由（自己代入でない） | `[未確定]` |
| V-646-3 | fld 反復の増分定数（0xb..）の用途 | `[未確定]` |
| V-645-1 | fld ループの走査対象 | `[部分確定]`（tile=0x5c3440 戻り） |

## §6 — 検証手段（コピペ可）

```sh
python D:\re\u147_1.py   # jnp 先 + guard 直後
python D:\re\u147_2.py   # trait sim + jnp 先領域 + [reg+4] store 走査
python D:\re\u147_3.py   # 各ブロック引数 sim + site0x5CC7E8 + fld ループ + this->tile writer
python D:\re\u147_4.py   # 0x5c3440 / fld ループ出口 / 0x5C9650
```
