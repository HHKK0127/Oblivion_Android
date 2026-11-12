# v339 — #112 次束V（v334/#109継続6件）静的裁定

- 作成: 2026-11-09（L1・静的解析のみ）
- 対象: v334/#109 の継続残件。`679480/679630`、dir[0] writer、index/change列のslot分離、`[0xB333A0]` 身元、`FUN_004302B0` read functor。
- 主な参照: `tmp/re/v334_ess_next_bundle_T_ruling.md`、`tmp/re/v338_plus_residuals_ruling.md`、`tmp/re/v328_ess_phaseB_ruling.md`、`tmp/re/v309_saveheader_followup.md`、`tmp/re/v139_b17_table2_residuals.md`、`tmp/re/s69_hc92_disasm.txt`。
- 対象 EXE: Oblivion Steam v1.2.0416 相当、ImageBase `0x400000`。
- 制約: オリジナル EXE/ESM/.ess、解析原本、移植側ソースを変更しない。`0x4C0000-0x4E0000`、`679xxx`、`0x70E560` は深追いせず、既存の浅層または保存済み解析結果だけを使用する。実機・追加 `.ess`・#82 動的ゲートは実施しない。
- 判定語: `[確定]` / `[部分確定]` / `[未確定]` / `[確定負]`。機能上の記述を、正式な C++ 名や未観測の実行時意味へ昇格させない。

## 1. 判定一覧

| # | 対象 | 本束判定 | 結論 |
|---:|---|---|---|
| 1 | `0x679480/0x679630` の count/buffer 契約 | **[確定]**（帯外は浅層） | `679480` は linked list 2本の文字列長+1を合計する byte-size getter、空時 `2`。`679630` は要素を処理して u16件数を書き、空時 `0`。検体の `02 00 00 00 00 00` は `[u32 size=2][u16 count=0]`。2Bはbufferではなく count。 |
| 2 | `dir[0]=822982` の writer | **[確定]** | `0x462290` の `[[0xB33A98]+0x8C0]` read が dir[0]として4B出力する。822,982は45E220の開始位置と一致するが、`+0x8C0`へ値を生成・保存する上流writer本体は未閉合。 |
| 3 | index 3437 / change列5527のslot | **[部分確定]** | index 3437は `45E220` が `[SaveGame+0x74]` として出す第1配列（Numeric ID Array）。5527は `0x4622D2` が `TES+0x74` getter結果の `+0xC` をdir[1]へ出す値で、readerはu32をbyte3再マップしてGetFormへ渡す。数値一致はslot同一性を意味しない。 |
| 4 | `[0xB333A0]` 身元とGLOB/GMST命名 | **[部分確定]** | RTTI/vtableから `[0xB333A0]` は `TES` singleton。GLOB/GMST二表の正式な個体名・どちらがどの表かは、許可帯外のgetter/chainを浅層留保しているため未確定。 |
| 5 | save#2・複数セル・RGB・#82依存 | **[未確定]（待記録）** | save#2の`+0x94`凍結、複数検体で5527の正体とdir[0]上流、tail==0/Δ0x1C、RGB/BGR、dry bit9/bit10の実行時発火は追加証拠待ち。 |
| 6 | 帯内`0x430000`の`FUN_004302B0` read functor | **[確定]** | `0x4302B0..0x430347`はSEH/once-init、必要時open、`0x747E80`へのread委譲、`[this+0x148] += eax`、`ret 8`。ReadFile直接経路・out/flag到達はない。意味はCRT stdio＋自前buffer層を使う2引数read実装。 |

## 2. item1 — `679480/679630` count/buffer契約全文

### 2.1 `0x679480`（浅層留保帯）

既存 `s69_hc92_disasm.txt` の関数headは次のとおりである。

```text
00679480  sub esp,0x14
00679483  push ebx
00679484  push ebp
00679485  push esi
00679486  lea esi,[ecx+0x40]
00679489  test esi,esi
0067948B  push edi
0067948C  mov [esp+0x20],ecx
00679490  mov dword [esp+0x14],0
00679498  mov dword [esp+0x18],2
006794A0  je 0x67955d
006794A6  mov ebx,[esp+0x20]
```

保存済み v338 の直接再確認結果では、次の契約が閉じている。

1. `0x679480` は `this+0x40` 側のlinked listと、対応する第2 list（`+0x48`系）を走査する。
2. 初期値は `2`。各要素について文字列長に終端分を加算し、返す値は**バイトサイズ**である。
3. 両listが空なら返値は `2`。これは要素数2ではない。
4. `0x45FB50` はこの返値をsize=4でwriter callbackへ渡す。

したがって検体の `@822,976` の `02 00 00 00` は、`45FB50`経路の `u32 size=2` と確定する。

### 2.2 `0x679630`（浅層留保帯）

headと保存済み追跡は次のとおり。

```text
00679630  sub esp,0x18
00679633  push ebx
00679634  push ebp
00679635  push esi
00679636  push edi
00679637  mov edi,ecx
00679639  mov ecx,[0xB33B00]
0067963F  xor esi,esi
00679641  mov [esp+0x18],esi
00679645  mov eax,[ecx+0x14]
00679648  push 2
0067964A  lea edx,[esp+0x1c]
```

関数末尾の保存済み解析では、内部件数を0で初期化し、対象要素を1件処理するごとに増加させ、最終的に `mov word [out],dx` で**u16件数**を書き出す。空リストでは0になる。

よって `@822,980 = 00 00` は、境界パディングでも45E220の先頭でもなく、`679630`のu16 countである。item1の6Bは次のように確定する。

```text
@822,976  02 00 00 00  = u32 size=2（679480）
@822,980  00 00        = u16 element count=0（679630）
@822,982  6d 0d 00 00 = u32 3437（45E220の第1配列count）
```

**限定**: `679xxx`本体はユーザー指定どおり浅層留保であり、内部のC++ container型・各list要素の正式型名までは裁定しない。ここで確定するのはsize/countのI/O契約である。

## 3. item2 — `dir[0]=822982` を記録するwriter

`0x462280`のdir書出列（v328保存証拠）は次のとおり。

```text
0x462290: ecx = [[0xB33A98] + 0x8C0]
          4B write → dir[0]
0x4622D2: ecx = [0xB333A0]
          call 0x43F270       ; eax = [ecx+0x74]
          eax = [eax+0x0C]
          4B write → dir[1]
```

検体値は `dir[0]=822,982`、`dir[1]=5,527`。`822,982`は `45FB50`の6B直後、すなわち`45E220`第1配列の開始位置と一致する。従って、これは単に「末尾の値」と推測するより、**後続配列開始offsetをdir[0]へ記録している**ことが強く支持される。

- `[確定]`: `0x462290`が`[[0xB33A98]+0x8C0]`をdir[0]として書く。
- `[確定]`: 検体値822,982と45E220開始位置が一致する。
- `[未確定]`: `+0x8C0`へ822,982を作成・保存する上流writer（45F0F0生成リスト等）の因果。ここは指定帯・既存成果物の範囲を越えて追わない。

## 4. item3 — index 3437 と change列5527のslotを分離

### 4.1 index 3437は`[SaveGame+0x74]`の第1配列

`0x45E220`の出力順は、既存v334/v338の訂正を採用する。

```text
u32 count([SaveGame+0x74]+0x0C) = 3437
u32 entries([SaveGame+0x74])  = 3437件
u32 count([SaveGame+0x78]+0x0C) = 1
u32 entries([SaveGame+0x78])  = 1件（検体値0）
```

ログ文字列は第1配列が `Numeric ID Array(%i)`、第2配列が `WorldSpace ID Array(%i)`。検体では第1配列のentriesが `@822,986..836,734`、続く第2配列が `836,734..836,742`を消費し、EOFまでfitする。

従って、`index3437=[SaveGame+0x74]`は**SaveGame内のindex writer slot**として確定する。過去v328の「index=[SaveGame+0x78]」は、v334の45E220全文とEOF/dir fitにより訂正済みであり、本報告では採用しない。

### 4.2 5527は別slotのdir[1] / reader側GetForm入力

`0x4622D2`は、`[0xB333A0]`（TES）の`+0x74` getter結果の`+0xC`をdir[1]へ書く。reader `0x462B20`はこの4B値をbyte3再マップし、`GetForm`相当の`0x46B250`へ流す。

このため、5527について確定するのは次だけである。

- writerの供給slot: `TES`経路の`([0xB333A0]+0x74)+0x0C`。
- file上の位置: dir[1]（dir[0]の直後）。
- readerでの扱い: countとして保持するだけでなく、formID候補として再マップ/GetFormへ渡す。

`5527 == change record列の件数`は検体上の一致であり、`index 3437`と同一配列・同一slotであることを意味しない。`5527`が「countをformIDとして再利用する設計」なのか、別表の値が偶然一致したのかは、save#2・複数セル/別件数検体がないため**[未確定]**である。

## 5. item4 — `[0xB333A0]`正式身元とGLOB/GMST命名

### 5.1 `[0xB333A0]`の身元

`tmp/re/v139_b17_table2_residuals.md`のRTTI証拠を採用する。

- `0x44199E`: `mov [esi],0xA37470`。
- vtable `0xA37470`のRTTI: `.?AVTES@@`。
- 起動生成列: `0x40663F: mov [0xB333A0],eax`。`0x441970`のTES ctor結果を格納する。
- 全体参照はNavMesh専用に限定されず、セル/空間遷移、フレーム末flush、フォーム破棄、各種world処理でthisとして使われる。

したがって正式身元は **`TES` global instance** であり、「NavMesh管理オブジェクト」や「playerRef」という旧ラベルは棄却する。player singletonは`0xB333C4`である。

### 5.2 GLOB/GMST命名の判定

保存列の表①/表②について、次の命令・境界は確定している。

- 表①: `0x462456` → `0x441000`（u16 count=95）→ `0x4410D0` buffer（760B）→終端148,555。
- 後段表: count=212、終端150,364。`462280`の同一セクション連鎖に属する。
- `[0xB333A0]`は表①の供給側に現れ、dir[1]も同じTES経路から供給される。

しかし、`0x441000/4410D0`および後段の`542EA0/5406F0/540720/5A8250/57BE10/4A2FF0/677010`の正式な個体同定と、表①/表②がGLOB/GMSTのどちらに当たるかは、`0x4C0000-0x4E0000`等の指定留保帯に依存する。したがって、**GLOB/GMSTの正式命名は本束では未確定**とする。表のログ名・count・offsetをGLOB/GMST名へ読み替えることはしない。

## 6. item5 — 実機・追加検体・#82依存の待記録

次は静的証拠のみでは閉じないため、判定を更新せず待記録とする。

| 待項目 | 必要証拠 | 現判定 |
|---|---|---|
| save#2 `+0x94` | 2回目保存の実ファイル/実行時比較 | `[未確定]`。`+0xA4==0`時のGetSystemTime書込ゲートだけ確定 |
| 5527のcount/formID関係 | save#2、複数セル/別件数検体 | `[未確定]` |
| dir[0]上流 | `+0x8C0`への実writer追跡 | `[未確定]`。462290のsinkのみ確定 |
| tail==0 / Δ0x1C | 該当entryを含む動的・追加検体 | `[未確定]` |
| thumb RGB/BGR | 実機画素比較またはD3D format証拠 | `[未確定]`。`0x70E560`深解析は禁止帯 |
| dry bit9/bit10 | #82 runtime lane | `[未確定]`。既知の直接即値RMW不在は別途`[確定負]` |

これらを本束の静的判定から「解消済み」とは扱わない。

## 7. item6 — `FUN_004302B0` read functor全文と意味

### 7.1 関数範囲と命令契約

`tmp/re/v309_saveheader_followup.md`の境界検証済み全文（`0x4302B0..0x430347`）を採用する。

```text
0x4302B0: SEH prologue、GS cookie（[0xB30AAC]）
0x4302D8: test byte [0xB33A00],1 / jne 0x43030D
0x4302E0: or [0xB33A00],1
0x4302E2: mov ecx,0xB33980 / call 0x401000
0x4302F8: push 0xA17B60 / call 0x981FB4 / add esp,4
0x43030D: cmp dword [esi+0x1C],0 / jne 0x430320
0x430313: mov eax,[esi] / mov edx,[eax+0x18]
           push 0 / push 0 / mov ecx,esi / call edx
           ; vtable+0x18 = open(this,0,0)
0x430320: mov eax,[esp+0x1C] ; arg2=n
           mov ecx,[esp+0x18] ; arg1=dst
           push eax / push ecx / mov ecx,esi / call 0x747E80
0x430331: add dword [esi+0x148],eax
0x430347: ret 8
```

### 7.2 直接意味

- **[確定]** 呼出契約は`read(this, dst, n)`相当の2引数転送で、戻り値`eax`は読取バイト数。
- **[確定]** 一度だけの初期化ガード後、FILE*未開ならvtable+0x18のopenを呼ぶ。
- **[確定]** 実読取は`0x747E80`へ委譲し、戻りバイト数を`[this+0x148]`へ加算する。`+0x148`はseek側の絶対設定と対になる現在位置バイト数。
- **[確定負]** 本体内にWin32 `ReadFile` IAT/間接呼出しはない。
- **[確定]** `out/flag`は`FUN_00430050`のthunkで転送されず、`FUN_004302B0`のstack契約にも到達しない。
- **[確定]** `0x747E80`は残量分を自前bufferから`memcpy`し、超過分を静的リンクCRTの`fread(dst,1,n,FILE*)`で読み、必要ならbufferを充填する。`[this+0x24]==0`時は0返却。
- **[確定]** 下位層はCRT stdio（`fopen/fread/fwrite/fseek`）＋自前buffer層であり、zlib `gz*`やWin32直叩きではない。

従って、P5のsize返却は「FUN_004302B0がoutへ書く」契約ではなく、readのdstへ書いたバイト列を戻り値として上位が消費する契約である。既存v328_19の`FUN_45DBC0` dst/return slot同一性の裁定とも矛盾しない。

## 8. 既存資料との差分・訂正

1. v334 item1の「続く2Bの所有未確定」は、`679630`のu16 count契約により**[確定]**へ更新する。`45E220`の開始は`@822,982`。
2. v328のfit探索`@822,978`および「index=[SaveGame+0x78]」は、v334 K2/K3の全文・EOF・dir[0]一致により旧記載化する。本束は`index 3437=[SaveGame+0x74]`を採用する。
3. v139の`[0xB333A0]=TES`訂正を採用し、NavMesh専用/Player説は採用しない。
4. v338はitem1、dir[1]、TES身元を更新済みだが、GLOB/GMST正式命名を未確定としている。本束も同じ境界を維持する。

## 9. 成果物・変更範囲・集計

- 成果物: `tmp/re/v339_ess_next_bundle_V_ruling.md`（新規、既存ファイル上書きなし）。
- 主判定: `[確定]` 3件（item1/2/6）、`[部分確定]` 2件（item3/4）、`[未確定]` 1件（item5）、主判定`[確定負]` 0件。
- 従属する`[確定負]`: `FUN_004302B0`のReadFile直叩き説、zlib gz*説、Win32直叩き説。旧v328のfit@822,978およびindex=[+0x78]は訂正対象。
- オリジナル EXE/ESM/.ess、解析原本、移植側ソース、docs、GT、`TODO.md`、`STATE.md`は変更していない。
- 推測実装・移植側ソース変更は行っていない。
