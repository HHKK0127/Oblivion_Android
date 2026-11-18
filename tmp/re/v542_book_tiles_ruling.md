# v542 — ブック系マネージャ singleton `0xB13970` の正体＝**`NiTArray<Tile*>` 継承クラス（vtable 0xA69680 RTTI 実測）＋要素追加ヘルパ `FUN_0042BB90`（word cap/count・array@+4 への `[ecx+4+idx*4]=ptr`）確定**・**`FUN_00591C50`＝`TileMenu`（vtable `0xA6AF14`・RTTI 実測）生成＋window ID 4002/4005/4011 管理（"ERROR: Cant find menu class!"）＝message book／Tile メニューシステムと確定**・**AC20 冒頭＝formID map `0xB0613C` から 26 エントリ（0x1A）静的検索＋`TESEyes`（TD `0xB0541C`）singleton 検索**（スロット窓 `[esp+0x6c]` の供給元＝静的 formID 配列と決着）・**V-541 訂正: `FUN_004B2FF0`＝`TESBoundObject` ctor（vtable `0xA4385C` RTTI 実測）＝TESObjectSTAT 基底チェーン確定／`FUN_0046D7E0` の `+0x24` ストアは `TESModel`（vtable `0xA3C24C` RTTI 実測）で「TESFullName 補助」は誤り**（V-542-1..3 完遂・旧残点 V-406-3 継続第72弾）

- 採番: **v542**（旧残点群消化・probe prefix **v542_1 全1本 EXIT=0**（RTTI/窓走査は probe 内蔵））
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-542-1..3（v541 残点）** — 1=ブック本文配列の割当元メソッド（BMSG/DOBM?）＋`582160` クラス名／2=AC20 スロット配列 `[esp+0x6c]` の供給元／3=`4B2FF0` 基底 ctor チェーン＋`46D7E0` の確定。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・RTTI（COL→TD 実測）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-542-1: ブック本文配列の管理者と割当元** | **[確定]（命令＋RTTI）管理者＝`NiTArray<Tile*>` 派生の singleton `0xB13970`／割当は汎用 add ヘルパ `FUN_0042BB90` 経由** | vtable `0xA69680`（=静的 `0xB13970` の値）の COL→TD ＝ **`.?AV?$NiTArray@PAVTile@@@@`**（RTTI 実測）。**`FUN_0042BB90(this=配列, idx, &holder)`**: `count=this[+0xA](word)`・`if idx<count: this[+0xA]++（+0xC も条件付き++）; this[+4][idx]=*holder`（docs の `42bb90` と生バイト一致）＝**NiTArray::InsertAt**。`0xB13974`/`0xB1397A` が disp32 で増えない理由＝**すべて `[ecx+4]/[ecx+0xA]`（this=0xB13970）経由**で、埋め込まれた NiTArray サブオブジェクトが +0/+4/+0xA を占有するため（+0xC=cap 系、dtor `584C00` がバケット走査 free＝チェーン併用配列）。 |
| **V-542-1: `FUN_00591B40`/`FUN_00591C50` の意味** | **[確定]（命令）message book／Tile メニューの close/open 対** | **591B40（close 側）**: `this[+0x44]`（保持メニュー）の `vtable[+0x34]()`（window ID 取得）が **0x3EC(1004)/0x3EF(1007) でなければ `FUN_00589D60`（メニュー閉）へ**、1004/1007 なら何もしない（＝ブック本文窓 ID 1004/1007 を退避するゲート）。呼び出しは `thiscall(0xB13970)`。**591C50（open 側）**: arg0==**0xFA2(4002)** のときのみ: `0x9828C0(float arg8)`（フロア）→ `FUN_00587B20(1, floor)`（メニュークラス検索）→`FUN_00587D20`（インスタンス生成）→**`this[+0x44]=menu`**（無ければ **`"ERROR: Cant find menu class!"`（0xA6B178）**）→`584880(0)`（表示設定）→window ID 取得→`[menu+0x20]=id`→**`588BD0(0xFA5=4005)`**×2（4005/4011 の子窓検索・count<7/0x2A 系）→`584980` 浮動値→**`58CEB0(0xFAB=4011,...)`**→**`42BB90(0xB13970, floor-1001, &menu-ish)` で Tile 配列へ登録**。→ **「message book（Tile）を窓 1001..1051 に対応させ、メニュー（TileMenu）経由で開く」マネージャ**。BMSG/DOBM 直接対応は出なかった（本文配列は UI Tile で確定 [部分確定] への格上げ）。 |
| **V-542-1: `FUN_00582160`** | **[部分確定]（命令）** | SEH 付き `return (flag?[0xb3a6e0]:…)` 系（`DAT_00b3a6e0` の UI インタフェース singleton getter）。`578D70(=582160(0,1))->+0xB0`＝UI 状態 pending バイト。クラス名の一次証拠なし（残点へ）。 |
| **V-542-2: AC20 スロット配列 `[esp+0x6c]` の供給元** | **[確定]（命令）＝AC20 冒頭の formID map 静的検索** | `FUN_0044AC20` 入口: `lea eax,[esp+0x14]; push eax; push 0x1A; mov ecx,0xB0613C; call FUN_0055E000` ＝**formID マップ（0xB0613C・v536/540）から 0x1A=26 エントリを検索して `[esp+0x14]` 配列へ受ける**（value 書換機構＝v540 §1 の *keyslot 更新）。その後 **`FUN_009832E6(0, TESEyes TD 0xB0541C, TESForm TD 0xB02F9C, 0, slot?)`** で **TESEyes シングルトン**（「EYES」固定 form）を解決。19 周ループの slot 窓 `[esp+0x6c+idx*?]`（+0xc formID/+0x10 名前）はこの**静的 base-game form 配列由来**と決着（0x3f 種族 eyes 等）。 |
| **V-542-3: TESObjectSTAT 基底チェーン＋`46D7E0`** | **[確定]（RTTI 実測）→ v541 の「TESFullName 補助」を訂正** | `FUN_004B2FF0`（`4B9AA0` が呼ぶ基底 ctor）: `call FUN_0046C460`（TESForm ctor・v535）→**`mov [esi],0xA4385C`**＋`[esi+0x18/0x1c/0x20]=0`。**COL→TD 実測: `0xA4385C`＝`.?AVTESBoundObject@@`** ＝ 継承チェーン **TESObjectSTAT : TESBoundObject : TESForm**（type byte 0x1C=28='STAT'（表名 'STAT' 実測一致）と整合）。`FUN_0046D7E0`（`+0x24` に呼ばれる）: `mov [esi],0xA3C24C`＋モデル系フィールド初期化→**COL→TD 実測 `0xA3C24C`＝`.?AVTESModel@@`**。→ **v541 の「TESFullName 補助初期化」は誤り（正: TESModel サブオブジェクト ctor）**。 |

## §2 証拠

### §2.1 `FUN_0042BB90`（NiTArray::InsertAt 相当）生バイト
```
42BB90: movzx edx,[ecx+0xa]（count） / eax=[esp+4]（idx） / cmp eax,edx / jb…
42BB9C: lea edx,[eax+1] / mov [ecx+0xa],dx（count=idx+1）
42BBA3: edx=[esp+8]（&holder） / cmp [edx],0 / je 分岐
42BBAC: add word [ecx+0xc],1（cap? カウンタ）
42BBB1: ecx=[ecx+4]（array） / edx=[edx]（value） / mov [ecx+eax*4],edx
```

### §2.2 591B40/591C50 の要旨（§1 表に対応）
```
591B40: esi=ecx(=0xB13970) / menu=[esi+0x44] / id=menu->vtable[+0x34]()
        if (id!=0x3ec && id!=0x3ef) jmp 0x589d60（閉じ） / else ret
591C50: if (arg0==0xfa2):
        ebx=floor(9828C0(arg8)) / cls=587B20(1,ebx) / menu=587D20(cls)
        [this+0x44]=menu ; 無ければ log("ERROR: Cant find menu class!")
        584880(0,menu) / [menu+0x20]=menu->vtable[+0x34]()
        588BD0(0xfa5)×2（fcomp 0xA69778/0xA69768 境界比較）
        584980→fstp→58CEB0(0xfab,...)
        42BB90(0xB13970, ebx-0x3e9, ...)（=id-1001 の 0 起点変換。0xFFFFFC17==-0x3E9）
```

### §2.3 AC20 冒頭（v542_1）
```
44AC51: lea eax,[esp+0x14] / push eax / push 0x1a / mov ecx,0xb0613c
44ACDF: mov [esp+0x1c],0 / call 0x55e000          ; ★ formID マップ 26 エントリ検索
44ACE4..: neg al/sbb/and（結果選択）→ push slot / call 0x9832e6（TESEyes TD 0xB0541C）
```

### §2.4 RTTI 実測（COL→TD・v542_1）
```
0xA69680 → COL 0xAC140C → '.?AV?$NiTArray@PAVTile@@@@'
0xA6AF14 → COL 0xAC1D98 → '.?AVTileMenu@@'
0xA4385C → COL 0xABADD4 → '.?AVTESBoundObject@@'
0xA3C24C → COL 0xAB8398 → '.?AVTESModel@@'
0xA44DEC/0xA44DC8 → 0xB054D4 = '.?AVTESObjectSTAT@@'（v541）
型表 index 28 name='STAT'・0xB0541C='.?AVTESEyes@@'
```

## §3 判定の内訳

- **V-542-1 完遂**: ブック本文配列＝`NiTArray<Tile*>`（singleton 0xB13970）で、割当は `42BB90::InsertAt` 経由（disp32 直書き不在の完全説明）。591C50/591B40＝TileMenu（窓 4002 系）open/close 対。BMSG/DOBM 対応は出ず（本文＝UI Tile 配列と確定）。`582160` は [部分確定]。
- **V-542-2 完遂**: AC20 スロット窓の供給元＝formID マップからの 26 エントリ静的検索（＋TESEyes singleton 解決）。
- **V-542-3 完遂＋訂正**: `TESObjectSTAT → TESBoundObject → TESForm` チェーン確定（RTTI）。`46D7E0` は **TESModel** ctor（v541 の「TESFullName 補助」を訂正）。

## §4 残点（V-543-1..3）

1. `FUN_00587B20`（メニュークラス検索（kind=1, id））と `FUN_00587D20`（生成）の本体／メニュークラス登録テーブル（"Cant find menu class" 対策の登録元）【中】
2. `DAT_00b3a6e0`（582160 singleton）のクラス名・生成元【低】
3. AC20 の 26 エントリ静的 formID 配列の**具体的な ID 一覧**（[esp+0x14..] のキー列＝0..25 か eyes 等固定 formID か）の確定【低】

## §5 後続への申し送り

- **移植側の message book UI**: `NiTArray<Tile*>`（cap/count word 構造・0x42BB90 相当 InsertAt）＋ TileMenu 生成（窓 4002／本文窓 1001..1051 基準オフセット）で再現。本文 ptr 配列（0xB13974）は ID 1001 起点（`589B70` の `-0xfa4`＝1004 補正）と対。
- **TESObjectSTAT のメモリレイアウト**: +0x00 vtable（TESObjectSTAT）／+0x04 type byte（0x1C）／+0x18..0x20 TESBoundObject フィールド 0 クリア／**+0x24 TESModel サブオブジェクト**（vtable 0xA3C24C）。移植側の STAT は「TESBoundObject 派生＋Model 内蔵」で足りる。
- **AC20＝「base game 固定 form 26 個の解決＋STAT activator 実体化」**と理解。移植側ロード後に同数の固定参照を解決する箇所を作る。
- **`[ecx+4]` 系メンバアクセスのグローバルは disp32 監査に出ない**（v541 §5 の教訓の一般形: singleton のサブ構造は thiscall 頭を辿る）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
