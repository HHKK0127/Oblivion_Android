# v628 — V-625-1: `0x5C4340` の引数対応と 21 サイトの引数出所（id は未確定）

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u89_1 / u90_1**（EXIT=0）＋既存逆コンパイル `FUN_005ca010`。
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. `0x5C4340` prologue の引数対応 [確定(命令)]

`ret 0x14` = 5 引数。prologue の読み:

| 引数 | スロット | 用途 | 根拠 |
|---|---|---|---|
| arg1 | `[esp+0x34]`（entry+4） | **parent tile** | `0x5C4369 mov eax,[esp+0x34]` → `0x585410` へ、`0x5C43FF` で `GetFloat(parent,0xFD0)` |
| arg2 | `[esp+0x38]`（entry+8） | **name（文字列）** | `0x5C43AD mov ebp,[esp+0x38]` → `sprintf` の `%s` 引数 |
| arg3 | `[esp+0x3C]` | **未使用**（読み無し） | — |
| arg4 | `[esp+0x40]`（entry+0x10） | **id** | `0x5C4381 mov eax,[esp+0x40]; 0x5C4385 cmp eax,-1` |
| arg5 | `[esp+0x44]` | **未使用**（読み無し） | — |

- id==-1 → `0x5C438A mov eax,[0xB3B5D4]; add eax,1; mov [0xB3B5D4],eax`（採番）。

## 2. 21 サイトの引数出所 [部分確定]

各 pane 生成の直前（例: Cheeks `0x5CAE1B`）:
```
0x5CAE04 push edi          ; arg5
0x5CAE05 push edi          ; arg4 (= id)
0x5CAE06 sub esp, 8        ; arg3,arg2 = 一時 BSStringT（name）
0x5CAE0B mov [esp+0x34], esp
0x5CAE0F push edx          ; name（0x419B10 が ret 4 で消費）
0x5CAE10 call 0x419B10
0x5CAE15 mov eax,[esi+0x28]
0x5CAE18 push eax          ; arg1 = parent
0x5CAE1B call 0x5C4340
```
| 引数 | 出所 | 21 サイト共通か |
|---|---|---|
| arg1 | **レジスタ**（`[esi+0x28]` = window tile） | 共通 |
| arg2, arg3 | **スタックローカル**（`sub esp,8` の一時 BSStringT = name） | サイトごとに name が違う |
| arg4（id） | **レジスタ `edi`**（`push edi`） | 21 サイトで同値 |
| arg5 | レジスタ `edi` | 共通 |

- **`edi` の代入は builder 内で 3 箇所のみ**: `0x5CA0B3 mov edi,eax` / `0x5CA0C5 mov edi,[edi+8]` /
  **`0x5CA1C1 mov edi,[ebp+0xe8]`**（`ebp` = `(*[0xB333C4]+0x170)()` = player）。→ **`edi` = `Character[0xE8]`**（条件ブロック）。
- **→ arg4（id）は 21 サイトで同一値**（`Character[0xE8]`、条件によっては前段の値）。

## 3. V-625-1 の結論と未確定点

- **prologue の引数対応は確定**（§1）。
- しかし **arg4（id）が 21 サイトで同値**となり、**pane ごとに異なる id** という前提と矛盾する。
  → **arg4 が id である**ことは確定だが、**その供給源が `edi`（定数）**である点が説明できない。
- **`[未確定] V-628-1`**: pane ごとの id 値。**完全な symbolic stack simulation**（各 call の `ret N` と
  `sub/add esp` を反映してスタック内容を追う）が要る。後方 esp 追跡（u89_1）と `edi` 追跡（u90_1）は
  いずれも決定的でなかった。**推測で埋めない。**

## 4. 副産物: item builder `0x5C93F0` と counter [確定(命令)]

- `0x5C93F0` は `ret 0x18` = **6 引数**。item index を **`push imm`** で渡す
  （`push 0,1,2,…,0x36`。逆コンパイル `uStack_4` と一致）。
- **`0x5C93F0` も counter `0xB3B5D4` を使い、+2 する**（`0x5C941E add eax,1; 0x5C9425 add eax,1; 0x5C942D mov [0xB3B5D4],eax`）。
- → counter は **pane で +1 / item で +2** の混在。`0xB3B5D4` のリセットは `0x5C4F3C`（`mov [0xB3B5D4],0xC8`＝200）。
- `0x5C4480` は `ret 0x1C`（7 引数）、`0x5C4800` は `ret 0xC`（3 引数）、`0x585410` は `ret 0xC`（3 引数）。

## 5. 残点

- **V-628-1【中・未確定】**: pane ごとの id（arg4 の供給源）。symbolic stack simulation が必要。
- （継続）V-626-2（`menu+0x94` の読み手）/ V-626-3（item builder と template）。

---

## 6. 検証

- probe **u89_1**（21 サイトの後方 esp 追跡＝不確定）。
- probe **u90_1**（builder 前方走査で `edi` 追跡＝`Character[0xE8]`）。
- probe 追加（`0x5C93F0` 本体＝counter +2 と item index `push imm`、`ret N` 一覧、`push imm` 全列挙）。
