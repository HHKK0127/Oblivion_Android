# v530 — グループの close とサイズ確定（`FUN_00450250`／`FUN_00450430`）・グループノード `+0x14`＝ヘッダ書込み位置・`FUN_00446CB0` はリスト push・`TESForm` vtable `+0x30` の厳密な不等号と `+0xBC`/`+0xC0` の top-level group（V-530-1..3 完遂・旧残点 V-406-3 継続第60弾）

- 採番: **v530**（旧残点群消化・probe prefix **v530_1..7 全7本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-530-1..3（v529 残点）** — 1=`TESFile+0x280` が常に 0＝dataSize 0 の件＋保存完了時のサイズ確定処理／2=グループノード `+0x14` の意味＋`FUN_00446CB0` identity／3=`groupType` 0..7 の各意味と `case 0` の不等号方向。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（vtable 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-530-1: `TESFile+0x280` は加算されるのか** | **[確定負] 加算サイトは存在しない** | `docs` 全域で `+0x280` への**書き込みは `FUN_00450980` の `= 0` の1箇所のみ**、**読み出しは `FUN_00450A80` の1箇所のみ**。`.text` を `disp32=0x280` で機械走査しても TESFile 系の書き込みは `0x4509FB`（`=edi=0`）だけ。→ **`+0x2C` の bit5 経路（ヘッダのみのレコード）で `dataSize = 0` は仕様どおり**（`+0x26C` も同様に 2 箇所だけ）。 |
| **V-530-1: 保存完了時のサイズ確定処理はどこか** | **[確定]（命令）グループヘッダ側＝`FUN_00450250`** | `FUN_00450250`（= `TESFile::CloseGroup` 相当）: `node = writer+0x284`（リスト先頭）→ `file->vtable[+0xC](0, DAT_00a853d8)`（seek 末尾）→ `node+4(サイズ) = 現在位置 − node+0x14` → `file->vtable[+0xC](node+0x14, DAT_00a853d0)`（ヘッダ位置へ戻す）→ `FUN_00450190(writer, node, 0x14)`（20B 再書き込み）→ `FUN_0044FFA0`（ノード pop）。**GRUP の size は「閉じるときに現在位置との差で確定」** [確定]。 |
| **V-530-1: 保存終了時の後始末** | **[確定] `FUN_00450430`** | `while (writer+0x284 != 0)`: `TLS[+0x184] == 0` なら **patch せず pop**、そうでなければ `FUN_00450250()`（patch して pop）。`TLS[+0x184]` は `FUN_0044F3D0` が保存開始時に `= 1` する保存中フラグ。 |
| **V-530-1: `+0x3D4` の意味** | **[確定] レコード数カウンタ（load/save 両用）** | 保存側は `FUN_004501F0`／`FUN_004502C0`／`FUN_00450A10`／`FUN_00450A80` が `+0x3D4++`。読み込み側では `FUN_0044F3D0` が **`DAT_00b33a90 += *(*piVar10 + 0x3d4)`**（プラグイン読込後にレコード数を合算）＝`DAT_00b33a90` は「読み込んだ総レコード数」。 |
| **V-530-2: グループノードのレイアウト** | **[確定] 0x18B＝20B ヘッダ＋位置** | `+0x00..+0x13` = **20 バイト GRUP ヘッダ** `{type, size, label, groupType, stamp}`、**`+0x14` = そのヘッダを書いたファイル位置**（close 時のサイズ patch の基準）。ノードは `FUN_00401F00(0x18)` で確保し `writer+0x284` のリストへ push。 |
| **V-530-2: `FUN_00446CB0` の正体** | **[確定] リスト push（ctor ではない）→ v529 の記述を訂正** | `__thiscall(listHead*, item)`: `item == 0` なら何もしない／`*list != 0` なら **8 バイトのセル `{oldHead, next}` を確保**し `*list = item; list[1] = cell`／`*list == 0` なら `*list = item` のみ。→ **単方向リストの先頭挿入**。`FUN_0044FF50`（`writer` の `ECX`）は 20B ヘッダから 0x18 ノードを作って push する。 |
| **V-530-2: `FUN_0044FFA0`** | **[確定] リスト pop** | `node = writer+0x284` → `writer+0x288`（セル）があれば `writer+0x284 = cell->item; writer+0x288 = cell->next; free(cell); free(node);`、セルが無ければ `writer+0x284 = 0; free(node);`。 |
| **V-530-3: `TESForm` vtable `+0x30`（`0x46B510`）の厳密な分岐** | **[確定]（生バイト＋ジャンプテーブル実読）→ v529 の `case 0` の不等号は誤り** | jump table `0x46B56C` の実値: `case 0 → 0x46B534`／`case 1,4,5 → 0x46B558`／`case 2,3,6 → 0x46B552`／`case 7 → 0x46B55E`。実命令は **`case 0: cmp type, FUN_0046AF70(label); jge → false`＝`type < index(label)` のとき true**（v529 の「`type >`」は誤り）。`case 1,4,5: type < 0x35`／`case 2,3,6: type < 0x30`／`case 7: type < 0x39`（`cmp` の直後は `jae → false`）。 |
| **V-530-3: `FUN_0046AF70` の戻り値** | **[確定]（命令）0 始まり index** | `0xB05E04` を stride 12B で `[ecx]`（f1=packed 4CC）と比較し、一致したときの `eax`（0 始まり）を返す。見つからなければ 0。結果は `0xB33C1C`（値）／`0xB33C20`（キー）に 1 件キャッシュ。**`index+1` ではない**（f2 は別フィールド）。 |
| **V-530-3: `groupType = 0` の正体** | **[確定] top-level グループ（ラベル＝レコード型 4CC）** | `TESForm` vtable **`+0xC0`＝`0x46B950`** が 20B ヘッダを `{'GRUP', 0, (&DAT_00b05e08)[type*3], 0, 0}` で組む（`+0x0C`＝groupType 0、`+0x08`＝**その form の型の packed 4CC**、size/stamp 0）。`ret 8`（`arg2 != 0` なら何もしない）。 |
| **V-530-3: `TESForm` vtable `+0xBC`（`0x46B910`）の意味** | **[確定]（命令）top-level group の membership 判定** | `(header, _, _) ret 0xC`: `header == 0 → false`／`header[0] != 'GRUP' → false`／**`header[3] != 0 → false`（groupType 0 のみ）**／`header[2]（label）== (&DAT_00b05e08)[this->type*3]` なら `true`。**第2・第3引数は未使用**（`FUN_004502C0` の `(node,1,1)` / `(node,0,1)` は同じ結果）。 |
| **V-530-3: `groupType = 6` の正体** | **[確定] cell children（ラベル＝cell の formID）** | `FUN_004D3460`（CELL 保存経路）は `virtual[+0x24]()` → `FUN_00450A10(writer, cell)`（本体）→ **スタックに `{'GRUP', 0, label=cell->+0xC(formID), 6, 0}` を組んで `FUN_004501F0(writer, &hdr)`**（ノード push＋位置記録＋20B 書き込み）。**ラベルが 4CC でなく formID** なので top-level（`+0xC0`）とは別系統＝groupType 6 は「セルの子グループ」。 |
| **V-530-3: groupType 0..7 の各意味** | **[推定]（0 と 6 は [確定]）** | 0=top（`0x46B950` が生成）／6=cell children（`FUN_004D3460` が生成）は実データで確定。1=world children／2=interior cell block／3=interior cell sub-block／4=exterior cell block／5=exterior cell sub-block／7=topic children（`+0x30` の閾値 0x35=WRLD 直前・0x30=CELL 直前・0x39=DIAL 直前という型 index 境界と整合）は **[推定]**。 |

## §2 証拠

### §2.1 `TESFile+0x280` の全書き込み・読み出し（docs 全域＋生バイト走査）
```
docs: +0x280 への代入は 1 箇所のみ
  L61636  *(undefined4 *)(param_1 + 0x280) = 0;          ← FUN_00450980（レコードヘッダ開始）
docs: +0x280 の読みは 1 箇所のみ
  L61668  *(undefined4 *)(param_1 + 0x26c) = *(undefined4 *)(param_1 + 0x280);   ← FUN_00450A80
docs: +0x26c の代入は 2 箇所のみ（L61629 = 0 / L61668 = +0x280）
.text 生走査（disp32 = 0x280）: TESFile 系のヒットは 0x4509FB（mov [esi+0x280],edi=0）と
  0x450A85（mov eax,[esi+0x280]）のみ。加算・inc・add は 0 件。
```
→ **`+0x2C` の bit5 分岐（ヘッダのみ書くレコード）で dataSize 0 は仕様**。

### §2.2 グループの close とサイズ確定（`FUN_00450250`）
```c
void FUN_00450250(TESFile* this) {
  node = *(int*)(this + 0x284);
  if (node != 0) {
    if (*(int*)(this + 0x10) != 0) {                 // 実ファイルあり
      (**(code**)(**(int**)(this+0x10) + 0xc))(0, DAT_00a853d8);   // seek 末尾
      pos = *(int*)(*(int*)(this+0x10) + 0x30);
      if (pos == -1) pos = *(int*)(*(int*)(this+0x10) + 0x148);
      *(int*)(node + 4) = pos - *(int*)(node + 0x14);              // ★ size 確定
      (**(code**)(**(int**)(this+0x10) + 0xc))(*(int*)(node+0x14), DAT_00a853d0); // seek 戻し
      FUN_00450190(node, 0x14);                                    // 20B 再書き込み
    }
    FUN_0044ffa0();                                                // pop
  }
}
void FUN_00450430(TESFile* this) {
  for (node = *(int*)(this+0x284); node != 0; node = *(int*)(this+0x284)) {
    tl = *(char*)(TLS + 0x184);
    if (tl == 0) pop_without_patch();      // 保存中でなければサイズを書かない
    else         FUN_00450250();           // 保存中なら patch して pop
  }
}
```

### §2.3 グループノードとリスト（`FUN_0044FF50` / `FUN_00446CB0` / `FUN_0044FFA0`）
```
44FF50: push ebx / push edi / edi=[esp+0xc]（ヘッダ） / ebx=ecx（writer）
        push 0x18 / call 0x401f00                 ; node 確保
        push esi / lea ecx,[ebx+0x284] / call 0x446cb0   ; ★ list push
        esi = node; node[0..4] = header[0..4]     ; 20B コピー（+0x14 は未設定）
446CB0(listHead*, item) ret 4:                    ; ★ 単方向リスト push
        if (item != 0) { if (*list != 0) { cell = alloc(8); cell->item = *list;
                                          cell->next = list[1]; *list = item; list[1] = cell; }
                         else *list = item; }
44FFA0(writer):                                   ; ★ pop
        node = writer->0x284; cell = writer->0x288;
        if (cell) { writer->0x284 = cell->item; writer->0x288 = cell->next; free(cell); free(node); }
        else { writer->0x284 = 0; free(node); }
4501F0(writer, header20) ret 4:                   ; open group
        FUN_0044ff50(header20);                   ; node = clone + push
        if (writer->0x10) {
          file->vtable[+0xC](0, DAT_00a853d8);
          pos = CONCAT(file->0x30, file->0x148 if -1);
          node = writer->0x284;                   ; ★ push 済み先頭 = 新しいノード
          node->0x14 = pos;                       ; ★ ヘッダ書込み位置を記録
          FUN_00450190(node, 0x14);               ; 20B 書く
          writer->0x3D4++;
        }
```

### §2.4 `TESForm` vtable 0xA3BE3C（再実測）— `+0xBC`/`+0xC0` の実体
```
+0x00 0x60D0A0  +0x04 0x60D0A0  +0x08 0x46B800  +0x0C 0x46B870
+0x10 0x46C620  +0x14 0x46C2E0  +0x18 0x46C2F0  +0x1C 0x6FDFF0
+0x20 0x46B340  +0x24 0x60D0A0  +0x28 0x412960  +0x2C 0x46B4C0
+0x30 0x46B510  +0x34 0x46B500  +0x38 0x46C360  +0x3C 0x68F970
+0x40 0x46ABF0  +0x44 0x46AC10  +0x48 0x46AC30  +0x4C 0x46AC50
+0x50 0x46B5E0  +0x54 0x46B600  +0x58 0x60CF60  +0x5C 0x60CF60
+0x68 0x6FE080  +0x6C 0x60D0A0  +0x70 0x412970  +0x74 0x46C700
...
+0xBC 0x46B910  +0xC0 0x46B950  +0xC4 0x69D990  +0xC8 0x6FE080
+0xD4 0x4129A0（GetName）
```
→ v528 の 30 slot 記述と一致。**`FUN_004502C0` の `(*form + 0xBC)` は `0x46B910`（top-level 判定）で、`+0x30`（`0x46B510`）とは別物**（v529 の「`+0x30` が `+0xBC` の被呼び先」という含意は誤解を招くので本裁定で分離）。

### §2.5 `+0x30`（`0x46B510`）の実命令とジャンプテーブル
```
46B510: eax=[esp+4]（header） ; bl=0
46B517: if (header==0) return 0
46B51B: if (header[0] != DAT_00b05e20('GRUP')) return 0
46B525: edx=header[0xC]（groupType） ; if (7 < edx) return 0 ; jmp [edx*4+0x46B56C]
jump table 実値:
  case 0 -> 0x46B534    case 1 -> 0x46B558   case 2 -> 0x46B552   case 3 -> 0x46B552
  case 4 -> 0x46B558    case 5 -> 0x46B558   case 6 -> 0x46B552   case 7 -> 0x46B55E
case 0 (46B534): esi = this->type(byte[ecx+4]); eax = FUN_0046AF70(header[2]=label)
                 cmp esi,eax ; jge -> return 0 ; → return 1 iff type < index(label)
case 2/3/6 (46B552): cmp byte[ecx+4],0x30 ; jae -> 0 ; → 1 iff type < 0x30
case 1/4/5 (46B558): cmp byte[ecx+4],0x35 ; jae -> 0 ; → 1 iff type < 0x35
case 7    (46B55E): cmp byte[ecx+4],0x39 ; jae -> 0 ; → 1 iff type < 0x39
```
`FUN_0046AF70(label4CC)`: `0xB05E04` を 12B stride で `[ecx]` と比較し **0 始まり index** を返す（`0xB33C1C/0xB33C20` に1件キャッシュ、不一致は 0）。

### §2.6 `+0xBC`（`0x46B910`）と `+0xC0`（`0x46B950`）
```
46B910(header, _, _) ret 0xC:
   if (header == 0) return 0
   if (header[0] != 'GRUP') return 0
   if (header[0xC] != 0) return 0            ; groupType 0 のみ
   return header[8] == (&DAT_00b05e08)[this->type*3]   ; label == 自分の型の 4CC
46B950(header*, flag) ret 8:
   if (header == 0 || flag != 0) return
   header[0] = 'GRUP' ; header[0xC] = 0 ; header[4] = 0 ; header[0x10] = 0
   header[8] = (&DAT_00b05e08)[this->type*3]        ; label = 型 4CC
```
→ `+0xC0` が組むヘッダは必ず `+0xBC` が true を返す（自己整合）。

### §2.7 `groupType = 6` の実書き込み（`FUN_004D3460`）
```
4D3460: mov ebp,ecx ; if (this->flags bit5) return 0
...
4D3503: eax=[ebp] ; edx=[eax+0x24] ; ecx=ebp ; call edx        ; virtual[+0x24]（no-op）
4D350D: push ebp ; mov ecx,ebx(writer) ; call 0x450a10          ; 本体を書く
4D3515: ecx=[ebp+0xC]（formID） ; eax=[0xB05E20]('GRUP')
4D3521: [esp+0x1c] = ecx        ; header+0x08 = formID（label）
4D352A: [esp+0x18] = eax        ; header+0x00 = 'GRUP'
4D3536: [esp+0x1c] = esi(0)     ; header+0x04 = 0（size は close 時に patch）
4D352E: [esp+0x24] = 6          ; header+0x0C = groupType 6
4D353A: [esp+0x28] = 0          ; header+0x10 = stamp
4D353E: mov ecx,ebx ; call 0x4501f0                             ; グループを開く
```

## §3 判定の内訳

- **V-530-1 完遂**: `+0x280` の加算は存在しない [確定負]（docs の代入1/読1＋生バイト走査）。サイズ確定は **GRUP ヘッダ側**の `FUN_00450250`（`size = curPos - node+0x14` → seek → 20B 再書き込み → pop）と、保存終了時の `FUN_00450430`（`TLS[+0x184]` で patch 有無を分岐）で [確定]。`+0x3D4` はレコード数カウンタ（load 側の集計 `DAT_00b33a90 += file->+0x3D4` で裏取り）。
- **V-530-2 完遂**: ノード 0x18B の `+0x14` は **ヘッダ書込み位置** [確定]。`FUN_00446CB0` は **リスト push** [確定]（v529 の「ctor」は誤り）。`FUN_0044FFA0` は pop [確定]。
- **V-530-3 完遂**: jump table 実値と実命令で **`case 0` は `type < index(label)`**（v529 の「`type >`」は誤り→訂正）。`FUN_0046AF70` は 0 始まり index [確定]。`groupType 0` = top-level（label＝型 4CC、`0x46B950` が生成、`0x46B910` が判定）[確定]、`groupType 6` = cell children（label＝cell formID）[確定]。1..5,7 の名称は [推定]。

## §4 残点（V-531-1..3）

1. `TESForm` vtable `+0x30`（`0x46B510`）の**実際の call site**の特定（vtable 経由なので静的 call が無い）と、`case 0` の `type < index(label)` が「順序比較」なのか「所属判定」なのかの決着【中】
2. `groupType` 1..5,7 の実データ確認（world children / interior block / sub-block / exterior block / sub-block / topic children）— 生成側の `FUN_0046B950` 以外のヘッダ組立関数を探す【中】
3. `TESForm` vtable `+0x78`〜`+0xB8` の未読 slot（`0x4D7030`／`0x412980`／`0x412990`／`0x4CA690`／`0x46A9E0` 等）の identity【低】

## §5 後続への申し送り

- **保存の後始末は「開いたグループを逆順に閉じてサイズを patch」**（`FUN_00450250`）。移植側で `TESFile` 相当を実装するときは、**グループを開いた時点のファイル位置をノード `+0x14` に保存し、閉じる時に `size = 現在位置 − +0x14` を書く**という順序を守らないと GRUP サイズが壊れる。
- **`+0x2C`（bit5）経路はヘッダのみで dataSize 0** が正常（`+0x280` を加算する経路が存在しない）。移植側で「dataSize が 0 だから壊れている」と判断してはならない。
- **`+0xBC`（top-level 判定）と `+0xC0`（top-level ヘッダ生成）は対**。`groupType 0` のラベルは型 4CC、`groupType 6` のラベルは formID（cell）と**ラベルの意味が groupType で変わる**。
- **`+0x30` は `case 0` のみ `label` を型 index に変換して比較**する（他は固定閾値）。v528/v529 の「`type >`」記述は本裁定で訂正済み。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
