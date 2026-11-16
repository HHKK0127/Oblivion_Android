# v449 — `Player[+0x1C]` 書手不在・`+4` byte 書手群・`FUN_00511570` identity の裁定（V-449-1..3 完遂・旧残点群移行第3弾）

- 採番: **v449**（旧残点群移行・probe prefix **v449_1..v449_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-449-1..3（v448 の残点）** — 1=`Player[+0x1C]` identity と `+0x32 s16` の語／2=`TESActorBaseData+4` bit7 set 経路実検証と `0x523F40` int3 検査／3=docs 欠落 fn `FUN_00511570` の identity と default float AV の index 範囲を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。
- **訂正**: v449_2 の (c)「3fn とも docs annotation 欠落」は **`fn.lower()` バグ（`// fun_` 小文字 needle で恒に失敗）による誤答 → v449_3 で3件とも annotation 実在を確認して撤回**。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-449-1: `Player[+0x1C]` の書手** | **[確定]（否定側）** | **Player ctor 帯（`FUN_0066A740..0x66AD40`）の `[reg+0x1C]` 直書き＝0件**（`66A75D: lea eax,[esp+0x1c]` は stack 別物）＋ **docs の `FUN_0066a740` body（L430157..L430550）内の `param_1[7]`（＝+0x1C）代入も0件**（grep hit 17件は `param_1[0x1c1..0x1c7]`＝byte offset `+0x704..` の別 field）→ **`Player[+0x1C]` は Player ctor では書かない＝別 fn（Character/Actor 帯の ctor/初期化）が書く [確定（否定側）]、書手 fn は [未確定]＝残点**。`+0x32 s16` の語も継続 [未確定] |
| **V-449-2: `TESActorBaseData+4` の byte 書手群** | **[確定]** | **`mov byte [reg+4], ...` census（`0x467000..0x468400`）で5件/4fn**: **`46761F/467665`（`FUN_004675e0` 内）／`467DF9`（`FUN_00467ca0`）／`468186`（`FUN_00468180`＝byte AV setter）／`468291`（`FUN_00468280`）**。各 fn の decompiled（v449_3 正しい needle で取得）: **`FUN_00468280`（L81937）＝`*(u8*)(self+4)=v[0]; virtual+0x10(0x100)` → `*((u8*)self+5)=v[1]` → `+6=v[2]` ＝ 3-byte 一括 AV setter（`+4,+5,+6` 各 set＋notify）[確定(decompiled)]**／`FUN_004675E0`（L81195）＝list walk（`self+6`＝`+0x18` linked list）でマッチエントリの `*(char*)(piVar1+1)=v` 更新 [推定高]／`FUN_00467CA0`（L81610）＝byte param を受ける大 local の処理 fn [未確定（残点）]。**bit7 set 経路の候補はこの群（byte store が `+4` 全体／個別 byte に書き込む）[推定高]** |
| **V-449-2: `0x523F40` の境界** | **[確定]** | bytes `0x523F38..0x523F40`: `... B8 EC F7 A2 00 C3 B8 C0 39 A5 00 C3` `CC CC CC CC CC CC CC CC` → `0x523F40: 51 56 8B F1`（`push ecx/esi; mov esi,ecx`）＝**int3×8 境界の独立 fn entry＝docs annotation 欠落 [確定]**（v448 の `FUN_00511570` と同類型。直前 fn 群は `mov eax,0x00A2F7EC; ret` 型 const-return stub） |
| **V-449-3: `FUN_00511570` の identity** | **[確定相当]（機械一式）** | full decode: `fldz; [ebp]=0.0`（out double）→ `virtual [edi]+0x190()` → `call 0x5e02e0`（obj 取得）→ `movzx eax,byte[ebx+4]`（FormType）→ **`__RTDynamicCast`（`push 0; push 0xB03DE4; push 0xB02F9C; push 0; push eax; call 0x9832e6`）→ `0xB03DE4` の TD 実読＝`.?AVTESActorBase@@` [確定]** → **`virtual [esi]+0xB4`** → **`cmp edi,[0xB333C4]`（Player 同値判定）→ Player のとき: `call 0x5F1910(8)→FUN_00519C50`（AV8=Health SET）／`call 0x5F1910(0xA)→lea ebx,[esi+0x24]→FUN_004672B0`（AV10=Fatigue SET）／`call 0x5F1910(9)→FUN_00467290`（AV9=Magicka SET）→ `call 0x4d6670/0x6ecc80/virtual+0x174/0xB3333A98` 続行 [確定(機械)]** → **`FUN_00511570` ＝「obj を `TESActorBase` に RTTI cast し、Player の場合は Health/Magicka/Fatigue を一括 SET する AV 適用 fn」[確定相当]** |
| **V-449-3: dispatcher default の index 範囲** | **[確定]** | `FUN_00519F50` head: **`519F54: cmp eax,0x27; ja 0x51a094`** → **switch 範囲 `0..0x27`（AV index 0..39）、default は `index ≥ 0x28`（＝`[40]MagickaMultiplier` 以降）[確定]** → **default の float setter（`FUN_0065cb00`＋notify）は AV index 40..71 に適用 [確定相当]**（＋case で明示された 0x21..0x24 は byte setter、0x0B..0x20/0x25..0x27 は no-op） |
| **V-449-3: その他の実読** | **[確定]** | RTTI TD: **`0xB03778=.?AVTESObjectARMO@@`／`0xB036FC=.?AVTESObjectWEAP@@`／`0xB05DE0=.?AVTESObjectCLOT@@`**（`FUN_0051a120` の case が使う cast 先＝v448 引継ぎ）／**`FUN_005F1910`（L357654）＝`FUN_005EAD00(param)` → `FUN_009828C0()×2` の wrapper [確定(decompiled)]**（AV値取得の実体は `FUN_005EAD00`＝残点） |

## §2 証拠（v449_1..v449_3）

### §2.1 `Player[+0x1C]` 否定側（v449_1a）
```
machine 0x66A740..0x66AD40: [reg+0x1c] 直書き 0件 (lea [esp+0x1c] のみ)
docs FUN_0066a740 body L430157..L430550: param_1[7](=+0x1C) 代入 0件
  (grep hits: param_1[0x1c1..0x1c7] = +0x704.. の別 field)
```

### §2.2 `+4` byte 書手（v449_1b + v449_3a）
```
census 0x467000..0x468400: 46761F/467665 (FUN_004675e0), 467DF9 (FUN_00467ca0),
  468186 (FUN_00468180), 468291 (FUN_00468280)   = 5 hits / 4 fns
FUN_00468280 (L81937): *(u8*)(p+4)=v[0]; virtual+0x10(0x100); +5=v[1]; +6=v[2] ...
FUN_004675E0 (L81193, annotation EXISTS): list walk at p+6 -> *(char*)(piVar1+1)=v
FUN_00467CA0 (L81608, annotation EXISTS): void __thiscall(int, byte) big-locals
(v449_2 の「annotation 欠落」は fn.lower() バグで誤答 -> 撤回)
```

### §2.3 `FUN_00511570`（v449_2a）＋ RTTI（v449_3b）
```
511570 fldz / [ebp]=0.0 -> virtual [edi]+0x190 -> call 0x5e02e0 -> ebx=obj
5115AB movzx eax,[ebx+4] -> call 0x447df0
5115BF push 0xB03DE4 / push 0xB02F9C / push 0 / push eax / call 0x9832e6 (__RTDynamicCast)
       -> TD 0xB03DE4 = .?AVTESActorBase@@   [確定]
5115E0 virtual [esi]+0xB4
5115EB mov ecx,[0xB333C4]; cmp edi,ecx / jne 0x511631   (Player? )
  Player branch: 5F1910(8)  -> FUN_00519C50 (Health  SET, v413 case8)
                 5F1910(0xA)-> [esi+0x24] -> FUN_004672B0 (Fatigue SET)
                 5F1910(9) -> FUN_00467290 (Magicka SET)
  511631: call 0x4d6670 / 0x6ecc80 / virtual [edi]+0x174 / [0xb333a98] ...
RTTI TDs: 0xB03778=.?AVTESObjectARMO@@ 0xB036FC=.?AVTESObjectWEAP@@
          0xB05DE0=.?AVTESObjectCLOT@@
```

### §2.4 dispatcher bound（v449_2b）
```
FUN_00519F50: 519F54 cmp eax, 0x27 / ja 0x51a094   -> switch 0..39, default >= 0x28
```

## §3 残点 **V-450-1..3**
1. 【低】**`Player[+0x1C]` の真の書手 fn**（Character/Actor ctor 帯の `[reg+0x1c]` store census を `0x66A000..` 以外へ拡張）と **`+0x32 s16` の語**。
2. 【低】**`FUN_00467CA0` の identity 深部**（byte param の意味）と **`FUN_005EAD00`（AV値取得実体）**／`FUN_005F1910` 経由の AV fetch 経路。
3. 【低】**`0x523F40`（docs 欠落独立 fn）の identity**（machine 全文 decode）。

## §4 検証・自己批評
- probe: **v449_1..v449_3 の3本全 EXIT=0**（V-449-1 は否定側確定・書手は残点／V-449-2 完遂（書手5fn＋3fn 実読＋`0x523F40` 欠落確定）／V-449-3 完遂（`FUN_00511570` identity 相当＋default 範囲＋RTTI TD））。
- **決定打の作り**: (i) **`Player[+0x1C]` は「ctor 帯 machine＋docs body」の2系統で直書き検索**して否定側を確定 → (ii) **byte 書手は band census で5件一覧→各 fn を decompiled で identity** → (iii) **`FUN_00511570` は full decode で `__RTDynamicCast` の引数 TD（`0xB03DE4`）を実読 → `TESActorBase` と即決**、**Player 分岐の `FUN_00519C50/4672B0/467290`（v413 の AV8/10/9 setter）が揃って identity 完結** → (iv) **dispatcher default は head の `cmp eax,0x27; ja` の1行**。
- 妥当性チェック: ①`FUN_00468280` の `+4,+5,+6` 3-byte setter⇔`TESActorBaseData` の `+4` に byte AV 群 — ②`FUN_00511570` の AV SET 3本⇔v413 の dispatcher case8/9/10（Health/Magicka/Fatigue 実語と整合）— ③`cmp eax,0x27`⇔case 列挙が 0..0x27（v447/v448 の no-op/setter 分布と整合）— ④`0x523F40` int3×8⇔独立 fn entry⇔annotation 欠落 — ⑤`0xB03DE4` TD⇔`TESActorBase`（`[esi+0x24]`＝TESActorBaseData 埋め込みと整合）— **五方向矛盾なし**。
- docs との不整合: **①`FUN_00511570`／`0x523F40` の docs annotation 欠落（v437 類型・2件目確定）**／②**v449_2 (c) の「3fn annotation 欠落」は自己バグ（`fn.lower()`）で誤答 → v449_3 で撤回**（3件とも実在: `L81193/L81608/L81935`）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **docs needle は実データのケース（`// FUN_` 大文字＋addr 小文字）をそのまま使う** — `fn.lower()` 等の無加工 lowercase は「欠落」と誤認を生む（**結果が3件同時「欠落」なら needle 疑い**）。
  2. **否定側確定は「machine band 検索＋docs body 検索」の2系統**（`Player[+0x1C]` は両方0件で初めて「別 fn」が確定）。
  3. **`__RTDynamicCast` の identity は第4引数 TD の name 実読で1手**（`push 0xB03DE4` → `.?AVTESActorBase@@`）— caller fn の役割も cast 先でほぼ決まる。
  4. **AV 適用系 fn は「既知 setter（v413 の case8/9/10）がどこから呼ばれるか」の census が鍵** — 3本の setter が並ぶ分岐を見れば Player 判定と一括 SET が同時確定する。
  5. **switch の default 範囲は head の `cmp reg,N; ja` の1行で即決** — case 本文を全部読む前に bound を先に取る。
