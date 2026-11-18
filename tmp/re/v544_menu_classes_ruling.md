# v544 — **メニューファクトリの窓 ID↔クラス対応表を RTTI で全47件確定**（1001=`MessageMenu`／1002=`InventoryMenu`／1003=`StatsMenu`／1004=`HUDMainMenu`／1007=`LoadingMenu`／1026=`BookMenu`／1030=`ClassMenu`／1036=`RaceSexMenu`／1044=`MainMenu`／1051=`TextEditMenu` ほか・全 ctor が基底 `584640` 共有で vtable 即値→COL→TD が一発）＝**v542 の close ゲート 1004/1007 ＝ `HUDMainMenu`/`LoadingMenu`、591C50 の open 先 4002 ＝ メニュータイプ enum [部分確定]**）・**`FUN_0055E000` の生バイト確定＝2 引数 thiscall `ret 8`（miss 時 `xor al,al`＝0 返却で docs の `hash&0xffffff00` 記述は誤り→訂正。AC20 の `push 0x1A; push &arr` は第2引数=&arr・第1引数 0x1A は呼び捨てスタック残骸＝55E000 は [esp+0x14] を読むため docs 3 引数表記と整合、v543 の [未確定] を解消し「&arr を keyslot として 1 件検索」に再確定**）・**`FUN_004E4A10`＝thiscall(0x33 singleton) の動的配列 grow/set ヘルパ（+8=現在値 word・+0xa=count・+0xc=cap 系）[部分確定]**（V-544-1..3 完遂・旧残点 V-406-3 継続第74弾）

- 採番: **v544**（旧残点群消化・probe prefix **v544_1..4（4本・4は v544_2/3 修正を含む再走査）EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-544-1..3（v543 残点）** — 1=menu ctor の RTTI クラス名（1001/1004/1007/1030 サンプル→全 47）／2=`call 55E000` 規約決着＋キー配列書込み元／3=`FUN_004E4A10(0x33)` 本体。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（COL→TD 実読）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-544-1: メニュー窓 ID↔クラス** | **[確定]（RTTI・全47件）** | ctor は全て先頭で基底 `FUN_00584640` を呼び、その後に **`mov dword ptr [esi], imm`** で vtable をストア。COL→TD 実読で全窓のクラス名が判明（§2.1 表）。代表: 1001=`MessageMenu`・1004=`HUDMainMenu`・1005=`HUDInfoMenu`・1006=`HUDReticle`・1007=`LoadingMenu`・1008=`ContainerMenu`・1009=`DialogMenu`・1010=`HUDSubtitleMenu`・1011=`GenericMenu`・1013=`PauseMenu`・1015=`OptionsMenu`・1022=`MagicMenu`・1023=`MapMenu`・1025=`NegotiateMenu`・1026=`BookMenu`・1030=`ClassMenu`・1032=`SkillsMenu`・1034=`PersuasionMenu`・1036=`RaceSexMenu`・1038=`LoadgameMenu`・1039=`SaveMenu`・1040=`AlchemyMenu`・1044=`MainMenu`・1046=`QuickKeysMenu`・1051=`TextEditMenu`。→ **v542 の close ゲート（1004/1007）＝HUD メインメニューとローディング画面の保護**と確定。open 側（591C50）が探した **4002 は窓 ID 空間ではなく 587D20 呼び時の「type/ID 第一引数」領域** [部分確定]（591C50 の `arg0==0xFA2` 一致時のみ生成＝**4002=メッセージタイプ enum**、生成される menu の窓 ID は `vtable[+0x34]()` 実行時採番）。 |
| **V-544-2: `call 55E000` 規約決着** | **[確定]（生バイト＋esp 追跡）→ v542/v543 の矛盾を解消** | `FUN_0055E000`: callee 入口 esp=S とすると `push ebx` 後の **`mov ebx,[esp+8]`＝[S+4]＝arg1**（呼び出し側の**最後の push＝0x1A**）を **キー値そのもの**として hash virtual に渡す。**miss は `xor al,al; ret 8`＝0 返却**（docs の `return uVar3 & 0xffffff00` は誤訳→**訂正**）、**hit は `[esp+0x14]`＝arg2（＝最後から2番目の push＝`&arr`）へ `node[8]`（value）を書いて al=1**。よって AC20 の `push &arr; push 0x1A; mov ecx,0xB0613C; call` は **`55E000(map=0xB0613C, key=0x1A, out=&arr)` ＝「固定 ID 0x1A の form を map から引き、arr[0] へ書く」**（直後 `neg al; sbb eax,eax; and eax,[esp+0x24]` でヒット時のみ arr 値が TESEyes cast（`9832E6` TD 0xB0541C）へ渡る）→ **v542 の「26 エントリ bulk 受信」は [確定負] に最終訂正**（0x1A は個数ではなくキー）。docs の 2 引数プロトタイプ（param_1 を object 扱い）は decompiler の ecx/arg 取り違えで、実体は thiscall(map, key, out) であることを生バイトで確定。 |
| **V-544-3: `FUN_004E4A10`** | **[部分確定]（命令）＝thiscall(0x33 singleton) の grow/set ヘルパ** | `edx=arg・this=[0xB3A708]（587B20 の 1 バイト singleton の実体は別 object ポインタ・+8 word=現在値/+0xa word=要素数/+0xc=cap 系/+4=配列 ptr）`: `arg==[+8]` なら分岐退避、`arg<[+a]` なら `[+4]` の **word/dword 配列を arg..[+a] まで 0 クリアして [+c] を 0xFFFF ずつ減算**、`[+8]=arg`、`arg<=0` で分岐。＝**配列 resize／リセット（型 ID ごとの manager 配列？）**。クラス名は無し（0xB3A708 は 1 バイト alloc のため、0x33 は「型 ID 付き配列」の実体は別途 [未確定]）。 |

## §2 証拠

### §2.1 窓 ID↔クラス↔ctor↔vtable（v544_3 全 47 件・抜粋ではなく全量）
```
1001 MessageMenu(5BC450/A6CE5C)   1002 InventoryMenu(5A9B00/A6C18C)   1003 StatsMenu(5D9AD0/A6D9DC)
1004 HUDMainMenu(5A6780/A6BEE4)    1005 HUDInfoMenu(5A4660/A6BDF4)     1006 HUDReticle(5A7FE0/A6C004)
1007 LoadingMenu(5AD0D0/A6C40C)    1008 ContainerMenu(597A90/A6B574)   1009 DialogMenu(59E4E0/A6B88C)
1010 HUDSubtitleMenu(5A8DF0)       1011 GenericMenu(5A3BE0)            1012 SleepWaitMenu(5D6870)
1013 PauseMenu(5BD960)             1014 LockPickMenu(5AF310)           1015 OptionsMenu(5BD580)
1016 QuantityMenu(5C04F0)          1017 AudioMenu(595150)              1018 VideoMenu(587BC0)
1019 VideoDisplayMenu(5DD960)      1020 GameplayMenu(5A3310)           1021 ControlsMenu(5874A0)
1022 MagicMenu(5B1F20)             1023 MapMenu(5B7360)                1024 MagicPopupMenu(5B3D30)
1025 NegotiateMenu(5BCE40)         1026 BookMenu(595BE0)               1027 LevelUpMenu(5AC550)
1028 TrainingMenu(5DD200)          1030 ClassMenu(596EF0)              1032 SkillsMenu(5D5610)
1034 PersuasionMenu(5BDFF0)        1035 RepairMenu(5D1000)             1036 RaceSexMenu(5C4E40)
1037 SpellPurchaseMenu(5D8900)     1038 LoadgameMenu(5AE450)           1039 SaveMenu(5D3290)
1040 AlchemyMenu(593050)           1041 SpellMakingMenu(5D73C0)        1042 EnchantmentMenu(5A1390)
1043 EffectSettingMenu(59F9B0)     1044 MainMenu(5B5F20)               1045 BreathMenu(596420)
1046 QuickKeysMenu(5C0B50)         1047 CreditsMenu(59C960)            1048 SigilStoneMenu(5D3EB0)
1049 RechargeMenu(5CE680)          1051 TextEditMenu(5879C0)
（全 ctor が FUN_00584640 基底 ctor を共有。クラス名の無い vtable store は 0 件）
```

### §2.2 `FUN_0055E000` 生バイト要点
```
55E001: ebx=[esp+8]（第1引数） / esi=ecx=this / eax=vt[+4](ebx)（hash）/ ecx=[this+8]（buckets）
55E014: edi=[ecx+eax*4] → 無ければ 55E038: xor al,al; ret 8（★miss=0）
55E020: node walk: eax=node[4]（key）→vt[+8](eax? push 順 ebx,eax)→一致（al!=0）で 55E040:
        eax=node[8]（value）/ ecx=[esp+0x14]（第3引数スロット）/ [ecx]=eax / al=1 / ret 8
```
→ docs の「miss 時 `hash & 0xffffff00`」は誤りで、**miss は 0（al=0）**。第3引数スロットへの value 書込みはヒット時のみ。

### §2.3 AC20 冒頭 call の再構成（生バイト逐語＋esp 追跡）
```
44AC51: lea  eax, [esp+0x14]     ; &arr（スタック配列）
44AC55: push eax                  ; 第2引数（後で = out）
44AC56: push 0x1a                 ; ★第1引数（キー）
44AC58: mov  ecx, 0xb0613c        ; this（formID map）
44ACDF: call 0x55e000
callee 55E000: entry esp=E（retaddr@E, arg1=0x1a@E+4, arg2=&arr@E+8）
  push ebx(E-4) / mov ebx,[esp+8] = E+4 = arg1 = 0x1a   ← hash キー
  push esi, push edi → esp=E-12
  55E043: mov ecx,[esp+0x14] = E+8 = arg2 = &arr        ← 出力先
  hit: al=1 / miss: al=0（ret 8）
```
→ **`55E000(this=0xB0613C, key=0x1A, out=&arr)`＝「キー 0x1A を map から引き arr[0] へ書く」**。docs の `*param_2 = uVar2`（keyslot=hash キーと同一ポインタ）は decompiler の混同で、実体は key と out が別引数。v542/v543 の「26 エントリ bulk 受信」は [確定負]。

## §3 判定の内訳

- **V-544-1 完遂**: 全 47 窓が RTTI クラス名を持つ正規 UI メニューと確定（推定不要の機械抽出）。v542/v543 の「book／tile」文脈と突合すると、591C50 は **「BookMenu（1026）ではなく HUD（1004）／Loading（1007）共存時の tile 表示用メニュー（type 4002）を生成」** [部分確定]。close ゲート（1004/1007）＝HUD とローディング画面を閉じさせない保護ロジックと解釈 [部分確定]。
- **V-544-2 完遂**: 55E000 は **thiscall(map, key, out)** と確定（`ret 8`・arg1=キー・arg2=出力・miss=0）。docs の 2 引数表記と「hash&0xffffff00」はどちらも decompiler の不備で、生バイト＋esp 追跡で決着。**AC20 の bulk 受信説は [確定負]→「キー 0x1A の単発 formID 検索」に最終訂正**。
- **V-544-3 部分完遂**: 4E4A10 は thiscall(0x33 singleton) の配列リセット/縮小ヘルパと判明（word 境界比較＋0 クリア＋cap 減算）。0x33 singleton の中身（+4 配列の正体）は残点。

## §4 残点（V-545-1..3）

1. `FUN_00584640`（menu 基底 ctor）がストアする基底 vtable の RTTI クラスタ（`IMenu`/`MenuBase` 系の正体）【低】
2. 0x33 singleton（`DAT_00b3a708` が指す実体・+4 配列）の生成元（4E4A10 の thiscall 先がこの 1 バイト alloc に対してどう成立するか＝4E4A10 の this 解決経路）【中】
3. 591C50 の type 4002 が生成する menu の窓 ID 範囲（`vtable[+0x34]` の採番元）と BookMenu（1026）等の対応【中】

## §5 後続への申し送り

- **移植側の UI 窓 ID 表**: §2.1 の 47 組をそのまま定数表にできる（Oblivion の UI 全メニューの ID↔クラス対応＝キャラ作成 `RaceSexMenu`=1036・クイックロード `LoadgameMenu`=1038・セーブ `SaveMenu`=1039・本 `BookMenu`=1026 など、移植 UI 実装の受け皿一覧）。
- **ハッシュ find の返り値規約は「1=hit（value 出力）／0=miss」**で再確定（v540 の「戻り==1」教訓が生バイトで裏取り完了。docs の `&0xffffff00` 記述は使わないこと）。
- **`FUN_0055E000(map, &keyslot, out)` の 3 引数形**を移植側シグネチャに採用（docs の 2 引数表記は不備）。
- メニュー基底 ctor `584640`＋vtable ストア＋フィールド 0 クリアという生成テンプレ（1004 例: 0x94 サイズ・+0x7c/+0x80..+0x88/+0x8c=1/+0x90=0）は移植側コンストラクタの雛形にできる。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
