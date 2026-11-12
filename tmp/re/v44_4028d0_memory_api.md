# v44: FUN_004028D0 の意味確定（Oblivion.exe Steam v1.2.0416）

作成日: 2026-10-24 / 解析対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
変換式: `.text` VA = 0x400C00 + file_off（ImageBase=0x400000, .text Raw=0x400）、
`.rdata` VA=0xA28000 Raw=0x627200、`.data` VA=0xB02000 Raw=0x700C00（PE を struct 自前解析）

---

## 1. セルフテスト4件（実測値・すべて PASS）

| # | 検査 | 期待値 | 実測値 | 結果 |
|---|------|--------|--------|------|
| 1 | bytes@0x504050 | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| 2 | bytes@0x4FB580 | `578b7908` | `578b7908` | PASS |
| 3 | string@0xA7035C | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | PASS |
| 4 | opid 0x0015 impl（table base 0xB0AF48, stride 0x28, 532件） | `0x504560` | entry[21] impl=`0x00504560` flags=`0x00000001` | PASS |

セクション実測: .text VA=0x401000 vsize=0x626C39 raw=0x400 / .rdata VA=0xA28000 raw=0x627200 / .data VA=0xB02000 raw=0x700C00。

---

## 2. FUN_004028d0 デコンパイル本文（`docs\Oblivion_decompiled.c` 行番号つき・全引用）

定義行は 1-pass 走査で L1908 に位置特定 → islice 窓読み。関数境界は L1970 の `}`、
L1974 が次の関数マーカ `// FUN_004029d0 @ 0x4029d0`（本文は L1908〜L1970 の 63 行）。

```c
1908: bool __thiscall FUN_004028d0(undefined4 *param_1,char *param_2,uint param_3)
1909:
1910: {
1911:   char cVar1;
1912:   undefined4 uVar2;
1913:   undefined2 uVar3;
1914:   char *pcVar4;
1915:   undefined1 *puVar5;
1916:   uint uVar6;
1917:
1918:   if (param_2 != (char *)0x0) {
1919:     pcVar4 = param_2;
1920:     do {
1921:       cVar1 = *pcVar4;
1922:       pcVar4 = pcVar4 + 1;
1923:     } while (cVar1 != '\0');
1924:     uVar6 = (int)pcVar4 - (int)(param_2 + 1);
1925:     if (param_3 < (uint)((int)pcVar4 - (int)(param_2 + 1))) goto LAB_004028fa;
1926:   }
1927:   uVar6 = param_3;
1928: LAB_004028fa:
1929:   if (*(ushort *)((int)param_1 + 6) < uVar6) {
1930:     uVar2 = *param_1;
1931:     puVar5 = (undefined1 *)FUN_00401f00(uVar6 + 1);
1932:     *param_1 = puVar5;
1933:     if (puVar5 == (undefined1 *)0x0) {
1934:       uVar6 = 0;
1935:     }
1936:     else if (param_2 == (char *)0x0) {
1937:       *puVar5 = 0;
1938:     }
1939:     else {
1940:       FUN_004028b0(puVar5,param_2);
1941:     }
1942:     FUN_00401f20(uVar2);
1943:     uVar3 = (undefined2)uVar6;
1944:     if (0xffff < uVar6) {
1945:       uVar3 = 0xffff;
1946:     }
1947:     *(undefined2 *)((int)param_1 + 6) = uVar3;
1948:   }
1949:   else {
1950:     if (uVar6 == 0) {
1951:       FUN_00401f20(*param_1);
1952:       *(undefined2 *)(param_1 + 1) = 0;
1953:       *param_1 = 0;
1954:       *(undefined2 *)((int)param_1 + 6) = 0;
1955:       return false;
1956:     }
1957:     if (param_2 == (char *)0x0) {
1958:       *(undefined1 *)*param_1 = 0;
1959:     }
1960:     else {
1961:       FUN_004028b0(*param_1,param_2);
1962:     }
1963:   }
1964:   if (uVar6 < 0x10000) {
1965:     *(short *)(param_1 + 1) = (short)uVar6;
1966:     return uVar6 != 0;
1967:   }
1968:   *(undefined2 *)(param_1 + 1) = 0xffff;
1969:   return uVar6 != 0;
1970: }
```

### 機能の要約（本文から）

`this`(param_1) は 8 バイトの動的文字列オブジェクト `{ char* buf@+0x00; u16 len@+0x04; u16 capacity@+0x06 }`。

1. 必要長 uVar6 = `param_2 ? max(strlen(param_2), param_3) : param_3`（L1918–1927）
2. **capacity 不足パス**（L1929–1947）: 旧 buf を退避 → `malloc(uVar6+1)` → 新 buf へコピー
   （param_2==NULL なら空文字）→ **旧 buf を free** → capacity = min(uVar6, 0xFFFF)
3. **capacity 十分パス**（L1949–1963）: uVar6==0 なら **free(buf) して全消去し return false**、
   それ以外は既存 buf へコピー（再確保なし）
4. len(+0x04) = min(uVar6, 0xFFFF)、戻り値 = `uVar6 != 0`（空文字設定なら false）

→ **malloc と free の両方を呼ぶ理由**: grow 時の「malloc 新 → コピー → free 旧」バッファ再確保と、
空文字化時の free。ハッシュマップ挿入ではなく文字列バッファの生命周期管理。

---

## 3. 被呼出先 / 呼び出し元

### 被呼出先（本文中・E8 は3件のみ。間接呼出・関数ポインタ参照なし）

| 実体 | 呼び出し箇所（decompile） | 役割 |
|------|---------------------------|------|
| `FUN_00401f00` (0x401F00) | L1931 | malloc 系（GT 済） |
| `FUN_00401f20` (0x401F20) | L1942, L1951 | free 系（GT 済） |
| `FUN_004028b0` (0x4028B0) | L1940, L1961 | strcpy 系（下記 §6） |

- 文字列比較（strcmp 等）: **なし**
- 関数ポインタ呼出: **なし**
- 構造体メンバアクセス: `param_1[0]`(+0x00 buf), `param_1+1` の u16(+0x04 len), `param_1+6` の u16(+0x06 capacity) のみ
- .rdata/.data のイミディエイト参照: **0 件**（capstone 逆アセンブルで imm オペランド全数を走査、§4）

### 呼び出し元（`docs\Oblivion_decompiled.c` 本文1パス走査、`// FUN_xxx @` マーカ帰属）

- **総呼出数: 530 箇所**（定義行 L1908 自体はカウントから除外。Ghidra が引数描画を落とした `FUN_004028d0();` 形も同一関数として計上）
- **呼び出し元関数: 221 個**（自己再帰 0 件 — 逆アセンブルでも自身への call なし）
- 戻り値の使われ方: 530 箇所すべて statement 位置（文単体）。**expression 位置0件 = 戻り値はどの呼び出し元も未使用**

上位10:

| # | 呼び出し元 | 件数 |
|---|-----------|------|
| 1 | FUN_005c7ba0 (0x5C7BA0) | 75 |
| 2 | FUN_005c34d0 (0x5C34D0) | 31 |
| 3 | FUN_005cdef0 (0x5CDEF0) | 24 |
| 4 | FUN_005c5f00 (0x5C5F00) | 13 |
| 5 | FUN_005cda30 (0x5CDA30) | 13 |
| 6 | FUN_009fbf20 (0x9FBF20) | 12 |
| 7 | FUN_00577c10 (0x577C10) | 10 |
| 8 | FUN_005c5c30 (0x5C5C30) | 9 |
| 9 | FUN_005cc200 (0x5CC200) | 9 |
| 10 | FUN_005c4920 (0x5C4920) | 8 |

（0x5C 系は UI/メニュー群。FUN_005c7ba0 内に `__stricmp(_Str1,"Characters\\Eyes\\EyeDefault.dds")` あり = メニューのパス比較処理）

呼び出し元の典型形（decompile）:

```c
L332201:   FUN_004028d0(DAT_00b38f70,0);          // 既存グローバル文字列を空化
L1035461:  FUN_004028d0("item_pickup",0);          // 文字列リテラルを代入
L2176:     FUN_004028d0(local_404,0);              // FUN_00402d00 経由の vsprintf 結果を代入
L5989:     FUN_004028d0(puVar23,0);                // name ポインタ（NULL なら空文字）を代入
```

---

## 4. capstone 逆アセンブル（0x4028D0〜、int3 / ret+0xCC 終端、101 命令、上限0x400内）

```
004028D0  53                        push ebx
004028D1  8b5c240c                  mov ebx, dword ptr [esp + 0xc]     ; ebx = size (param_3)
004028D5  55                        push ebp
004028D6  8b6c240c                  mov ebp, dword ptr [esp + 0xc]     ; ebp = str (param_2)
004028DA  85ed                      test ebp, ebp
004028DC  56                        push esi
004028DD  57                        push edi
004028DE  8bf9                      mov edi, ecx                       ; edi = this
004028E0  7416                      je 0x4028f8
004028E2  8bc5                      mov eax, ebp                       ; strlen ループ
004028E4  8d5001                    lea edx, [eax + 1]
004028E7  8a08                      mov cl, byte ptr [eax]
004028E9  83c001                    add eax, 1
004028EC  84c9                      test cl, cl
004028EE  75f7                      jne 0x4028e7
004028F0  2bc2                      sub eax, edx
004028F2  8bf0                      mov esi, eax                       ; esi = strlen
004028F4  3bf3                      cmp esi, ebx
004028F6  7702                      ja 0x4028fa                        ; strlen > size なら strlen 採用
004028F8  8bf3                      mov esi, ebx                       ; そうでなければ size 採用
004028FA  0fb74706                  movzx eax, word ptr [edi + 6]      ; capacity
004028FE  3bf0                      cmp esi, eax
00402900  7745                      ja 0x402947                        ; capacity < need → realloc
00402902  85f6                      test esi, esi                      ; --- capacity 十分パス ---
00402904  7419                      je 0x40291f                        ; need==0 → free して全消去
00402906  85ed                      test ebp, ebp
00402908  740e                      je 0x402918
0040290A  8b0f                      mov ecx, dword ptr [edi]
0040290C  55                        push ebp
0040290D  51                        push ecx
0040290E  e89dffffff                call 0x4028b0                       ; strcpy(buf, str)
00402913  83c408                    add esp, 8
00402916  eb77                      jmp 0x40298f
00402918  8b17                      mov edx, dword ptr [edi]
0040291A  c60200                    mov byte ptr [edx], 0              ; str==NULL → 空文字
0040291D  eb70                      jmp 0x40298f
0040291F  8b07                      mov eax, dword ptr [edi]
00402921  50                        push eax
00402922  e8f9f5ffff                call 0x401f20                       ; free(buf)
00402927  33f6                      xor esi, esi
00402929  0fb7c6                    movzx eax, si
0040292C  83c404                    add esp, 4
0040292F  66894704                  mov word ptr [edi + 4], ax          ; len = 0
00402933  8937                      mov dword ptr [edi], esi            ; buf = NULL
00402935  66897706                  mov word ptr [edi + 6], si          ; capacity = 0
00402939  5f                        pop edi
0040293A  33c0                      xor eax, eax
0040293C  85f6                      test esi, esi
0040293E  5e                        pop esi
0040293F  5d                        pop ebp
00402940  0f95c0                    setne al                            ; return false
00402943  5b                        pop ebx
00402944  c20800                    ret 8                               ; thiscall, 引数2個自前 pop
00402947  8b1f                      mov ebx, dword ptr [edi]            ; --- realloc パス ---
00402949  8d4e01                    lea ecx, [esi + 1]
0040294C  51                        push ecx
0040294D  e8aef5ffff                call 0x401f00                       ; malloc(need+1)
00402952  83c404                    add esp, 4
00402955  85c0                      test eax, eax
00402957  8907                      mov dword ptr [edi], eax            ; this->buf = new
00402959  7415                      je 0x402970                         ; malloc 失敗 → len=0
0040295B  85ed                      test ebp, ebp
0040295D  740c                      je 0x40296b
0040295F  55                        push ebp
00402960  50                        push eax
00402961  e84affffff                call 0x4028b0                       ; strcpy(new, str)
00402966  83c408                    add esp, 8
00402969  eb07                      jmp 0x402972
0040296B  c60000                    mov byte ptr [eax], 0
0040296E  eb02                      jmp 0x402972
00402970  33f6                      xor esi, esi                        ; need=0 扱い
00402972  53                        push ebx
00402973  e8a8f5ffff                call 0x401f20                       ; free(旧 buf)
00402978  83c404                    add esp, 4
0040297B  81feffff0000              cmp esi, 0xffff
00402981  0fb7c6                    movzx eax, si
00402984  7605                      jbe 0x40298b
00402986  b8ffff0000                mov eax, 0xffff                     ; capacity クランプ
0040298B  66894706                  mov word ptr [edi + 6], ax          ; capacity 記録
0040298F  81feffff0000              cmp esi, 0xffff
00402995  7715                      ja 0x4029ac
00402997  0fb7c6                    movzx eax, si
0040299A  66894704                  mov word ptr [edi + 4], ax          ; len 記録
0040299E  5f                        pop edi
0040299F  33c0                      xor eax, eax
004029A1  85f6                      test esi, esi
004029A3  5e                        pop esi
004029A4  5d                        pop ebp
004029A5  0f95c0                    setne al                            ; return (need != 0)
004029A8  5b                        pop ebx
004029A9  c20800                    ret 8
004029AC  b8ffff0000                mov eax, 0xffff
004029B1  66894704                  mov word ptr [edi + 4], ax          ; len = 0xFFFF クランプ
004029B5  5f                        pop edi
004029B6  33c0                      xor eax, eax
004029B8  85f6                      test esi, esi
004029BA  5e                        pop esi
004029BB  5d                        pop ebp
004029BC  0f95c0                    setne al
004029BF  5b                        pop ebx
004029C0  c20800                    ret 8                               ; 終端（次 byte 0xCC）
```

### 関数本体が参照する .rdata 文字列

**0 件**。imm オペランドを全走査したが 0xA28000〜0xB01FFF / 0xB02000〜の参照は皆無。
ログ・フォーマット・ラベル文字列を持たない純アルゴリズム関数。

### 呼び出し元が渡す .rdata 文字列（実読値・代表）

FUN_009FBF20 の逆アセンブルから、呼び出し元が push するリテラルを実読:

| 参照 VA | 実読値 | 備考 |
|---------|--------|------|
| 0xA2F7EC | `""`（空文字、raw `00 00 00 00`） | 空文字定数。直後から `"MemoryHeap::"` が続く .rdata ブロックの先頭パディング |
| 0xA6C0C8 | `item_pickup` | UI アクション文字列 |
| 0xA6C0B8 | `open_container` | 同上 |
| 0xA6C0AC | `chair_sit` | 同上 |
| 0xA6C098 | `activate_pull_push` | 同上 |
| 0xA6C08C | `bed_sleep` | 同上 |
| 0xA6C078 | `book_scroll_read` | 同上 |

FUN_009FBF20 は 0xB3B370 から 8 バイト stride のグローバル文字列オブジェクト列（0xB3B370/78/80/88/90/98/A0…、
各オブジェクトを `{buf=0, len=0, cap=0}` に初期化してから `FUN_004028d0(リテラル, 0)` で値を流し込む =
**グローバル文字列定数テーブルの初期化**。ハッシュマップ構築ではなく単純な配列初期化）。

---

## 5. 0xB02020（heap manager）との関係・this 構造体メンバオフセット

### 0xB02020

- **FUN_004028D0 の本体（デコンパイル・逆アセンブル双方）: 0xB02020 への直接参照なし**
- **間接関係あり**: 呼び出し先 `FUN_00401F00`(malloc) @0x401F07 と `FUN_00401F20`(free) @0x401F29 が
  それぞれ `mov ecx, 0xb02020` を実行（heap manager オブジェクトを thiscall で呼ぶ）。
  → 4028D0 は heap manager を経由して間接的にのみ関与する。

### this（param_1）構造体 — 8 バイトの動的文字列

| オフセット | 型 | 用途（実証） |
|-----------|-----|-------------|
| +0x00 | `char*` buf | ヒープ文字列バッファ。malloc 代入(L1932/0x402957)、free(L1951/0x402922)、NULL 化(L1953/0x402933) |
| +0x04 | `u16` len | 現在の文字列長。min(need, 0xFFFF) 書き込み(L1965–1968/0x40299A, 0x4029B1)、0 初期化(L1952/0x40292F) |
| +0x06 | `u16` capacity | 確保済みバッファ容量。min(need, 0xFFFF) 書き込み(L1947/0x40298B)、0 初期化(L1954/0x402935) |

+0x08 以降へのアクセスは**この関数には存在しない**（呼び出し元はオブジェクト先頭8バイトのみ操作。
0x9FBF20 でも `mov [0xb3b378],esi / mov word [0xb3b37c],si / mov word [0xb3b37e],si` の3書き込みで +0/+4/+6 を初期化）。

---

## 6. 近隣3関数の役割（1行ずつ）

| 関数 | シグネチャ | 役割 |
|------|-----------|------|
| `FUN_004028B0` @0x4028B0 | `void FUN_004028b0(int dst, char* src)`（cdecl、ret 無 imm・呼出側 `add esp,8`） | strcpy 系。`src-dst` オフセット差分によるインクリメントコピーで NUL 含め複写（4028D0 の L1940/L1961 が呼ぶ）。 |
| `FUN_00402D00` @0x402D00 | `void __thiscall FUN_00402d00(char* fmt, va_list ap)`（ret 8、this=文字列オブジェクト） | 書式付きセット関数。1KB スタックバッファへ `_vsprintf`(0x982820) して `this->FUN_004028d0(buf, 0)`（0x402D40）。GS cookie 保護。= String::Format 相当。 |
| `FUN_00407AE0` @0x407AE0 | `void __fastcall FUN_00407ae0(int param_1)`（巨大関数・ローカル多数） | 4028D0 を1箇所（decompile L5989）呼ぶ: `piVar11[7]`（name ポインタ、NULL なら空文字定数 0xA2F7EC）を文字列へ代入。周辺に vtable+0xDC 呼出と `"UNKNOWN"` フォールバックがあり、フォーム/エントリの名称取得系処理の一部。 |

呼び出し関係: 4028B0 と 402D00 は 4028D0 の**下位**（実装ヘルパ）、407AE0 は 4028D0 の**上位**（呼び出し元）。
ground truth 記載の「4028D0 が 402D00/407AE0/412FA0/413130 へ展開」は方向が逆で、これらは**呼び出し元**である
（402D00: L2176、407AE0: L5989、412FA0: L11314 で 4028D0 を呼ぶ）。

---

## 7. 判定

### **(d) その他 — 動的文字列クラスの Set/代入メソッド**（バッファ再確保つき `String::Set(const char*, size_t)`）

背景仮説 (a)/(b)/(c) はいずれも証拠により棄却。根拠:

1. **ハッシュ・比較・テーブル走査が皆無**（§3–4）: strcmp/ハッシュ計算/バケット走査/ノード確保が存在せず、
   呼び出しも malloc/free/strcpy の3関数のみ。(b) のハッシュマップ挿入は構造的に不可能。
2. **this 構造体は 8 バイト `{char*, u16 len, u16 cap}`**（§5）: ラベル/キー用の別フィールドがなく、
   malloc されたバッファには「渡された文字列そのもの」がコピーされる。(a) の名前付きアロケーションでは
   バッファ自身が名前の本体であり、名前付きレジストリへの登録は行われない。
3. **戻り値 `len != 0` を221関数530箇所すべてが無視**（§3）: 「登録成功フラグ」として使われていない。
   (c) の変数登録APIなら成功/失敗分岐が伴うはず。
4. **呼び出し元が全バイナリに分散**（UI 0x5C 系 75+31+24…件、定数テーブル初期化 0x9FBF20、
   vsprintf ラップ 0x402D00、名称取得 0x407AE0）: 特定サブシステム専用ではなくエンジン共通の文字列型。
5. **0x9FBF20 の実読リテラル**（§4）: `item_pickup` / `open_container` / `chair_sit` 等を
   8バイト stride のグローバル配列へ順に代入 = 文字列定数テーブルの初期化であり、挿入/検索構造ではない。
6. **grow 時の「malloc 新→コピー→free 旧」**（§2 L1931–1942）が malloc/free 両呼出の正体。

### 判定できなかった点・留保

- クラスの正式名（NiString / BSString 等のエンジン型名）は本 EXE に RTTI が無く特定不能（挙動は動的文字列で一致）。
- `len` が実文字列長ではなく `max(strlen, size)` を保存する点（capacity と同値に近い挙動）の設計意図は不明。
  L1925 の条件分岐により `size > strlen` の場合は余剰長が len に記録される。
- 戻り値を読む呼び出し元が0件であるため、`bool` 戻り値は実運用上デッドな可能性がある（未使用＝設計上の予備と推定）。
- 上位10 呼び出し元（0x5C 系 UI メニュー群）の各関数の詳細役割は本タスクの範囲外（未解析）。

---

## 付録: 検証メタ情報

- スクリプト: `%TEMP%\opencode\v44_selftest.py`, `v44_decompile_scan.py`, `v44_callers.py`, `v44_disasm.py`, `v44_neighbors.py`, `v44_ctx.py`, `v44_ctx2.py`, `v44_disasm2.py`
- decompile 本文窓: L1908（定義）〜L1970（`}`）、次関数マーカ L1974 `// FUN_004029d0 @ 0x4029d0`
- 逆アセンブル終端: `ret 8` @0x4029C0（次 byte 0xCC = int3 境界）、命令数101
