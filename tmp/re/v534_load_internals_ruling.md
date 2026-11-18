# v534 — ロード系の内部確定（`FUN_00450080`＝**ヘッダ読込＋formID mod byte 補正**・`FUN_00450F10`＝**sub-record ヘッダ読込（'XXXX' 分割対応）**・`FUN_00451A60`＝**TESFile Open/Rewind（`+0x258`＝fileSize・先頭 'TES4' 検証）**）・`FUN_004DB4A0` else 分岐は **flags bit10 のみ**（v533 の「& 0xFFFFFF01 マスク」は decompiler 副産物→訂正）・`FUN_0044FAA0` の ecx は**リスト先頭 item**・`FUN_0067B1E0`＝末尾追加 append（V-534-1..3 部分完遂・旧残点 V-406-3 継続第64弾）

- 採番: **v534**（旧残点群消化・probe prefix **v534_1..3 全3本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-534-1..3（v533 残点）** — 1=form/manager `+0x10` リストの正体＋`flags>>10` マスク解釈／2=`FUN_00450080`/`FUN_00450F10`/`FUN_00451A60` 内部／3=topic リスト追加/削除 site。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-534-2: `FUN_00450080`（`EnsureCurrentRecord` 経由のヘッダ読込）** | **[確定]（命令＋docs）＝レコードヘッダ 20B 読込＋formID の mod byte 補正** | file stream（`this+0x10`）の `vtable[+4]`（read）で **`+0x23c..+0x24c` へ 0x14 バイト読み**、失敗時 5 dword クリア→0。読めたとき **type が 'GRUP'/'TOFT' 以外**なら formID 補正: mod index=`byte[+0x24b]+1`（+0x24c の vc dword 内バイト）を `FUN_0044FD60`（**プラグイン配列 accessor**: `this+0x3f4` ベースの `[n-1+idx*4]`）で引き、**該当プラグイン record の `+0x400` バイトを formID 最上位バイトに焼く**。配列が無ければ (a) formID 上位バイト==0xFF でないかつ下24bit が `0<n<0x800`（`FUN_0046ABD0`＝**base game formID 妥当性判定**）なら `+0x24b=0`（mod 0＝本体）、(b) else **自分の mod index `this+0x400`** を焼く。→ **ロード時に formID の mod byte を補正する処理**（ESM/ESM-flag ファイルの index 実装の核心）。 |
| **V-534-2: `FUN_00450F10`（本體展開の第一歩）** | **[確定]（命令）＝sub-record ヘッダ {4CC, size} の読込** | `type=='GRUP' \|\| !(flags & 0x40000)` なら **ファイルから 6 バイト読み** `+0x250`（sub type）／`+0x254`（sub size）へ。else（**compressed/TOFT 経路**）は `+0x414==0` なら `FUN_00450550`（解凍バッファ生成）→ **`+0x414`（解凍済み本体）＋`+0x260`（本体内オフセット）から 6 バ이트**読む。読み出し後 **sub type=='XXXX'（0x58585858）なら分割 sub-sub の先頭**として `FUN_00450C20`＋`FUN_0044FEA0` で再読（v529 の "XXXX" 分割の読込側対応）。失敗は `+0x250/+0x254=0`→0。 |
| **V-534-2: `FUN_0044FEA0`** | **[確定]（命令）＝sub-record スキップ（次 sub へ進む）** | `+0x260 += +0x254 + 6`；`+0x260 >= 本体長`（通常 `+0x240`、compressed は `+0x418`）なら本体クリア 0；非 compressed は seek（`+0x25c+0x14+歩み`）；`FUN_00450F10` で次 sub ヘッダ読→1。 |
| **V-534-2: `FUN_00451A60`** | **[確定]（命令＋docs）＝TESFile Open/Rewind＋先頭検証** | `FUN_0044FBC0()`（準備）→ stream 無ければ `FUN_004515B0(+0x120 バッファ, +0x1c ファイル名, 0, 0)` で open → stream あれば seek 0＋状態全クリア＋`FUN_004511C0`。**`*(this+600) = stream.vtable[+0x1C]()`（＝file size）＝`+0x258` の書き込み元を確定**（v533 の「+0x258=fileSize」の一次証拠）。→ **`+0x23c != 'TES4'`（`DAT_00b05e14`）なら失敗**。その後 `+0x3dc/0x3df` のビット組替（ロード状態ビット）、ループで `EnsureCurrentRecord`→型 index==1（'AGGR'）の間スキップ、`+0x28c` に結果。`FUN_004511C0` の返り 2/0 で分岐（'TES4'/'GRUP' 判定）。 |
| **V-534-1: `FUN_004DB4A0` else 分岐の正確な値** | **[確定]（生バイト）→ v533 の「`flags>>10 & 0xFFFFFF01`」は decompiler 副産物。正しくは `bit10 getter`** | 0x4DB507: `mov eax,[esi+8]; shr eax,0xa; and al,1; ret` ＝ **`(*this+8 >> 10) & 1` のみ**。これは **TESForm vtable `+0x78`＝`0x4D7030` と同一命令列**（v531 で bit10 getter と確定）。→ 「ゲート不成立なら form の flags bit10 を返す」。 |
| **V-534-1: `FUN_0044FAA0` に渡る引数** | **[確定]（生バイト）＝`ecx = edi`＝リスト先頭 item** | 0x4DB4F7: `mov ecx, edi`（`edi = FUN_0046B680(form,-1)` の返り）→ `call 0x44faa0`。→ ゲートは「**先頭 item が `+0x3dc` bit0 を立てていないこと**」を見る。item は 0x3DD バイト以上のオブジェクト（TESSaveLoadGame/プラグイン管理系と同一クラスタ・0x44Fxxx がメソッド群）。**item の正確な型は [未確定] のまま**（追加 site 側の確定待ち）。 |
| **V-534-1: `+0x10` リストの構造** | **[確定]（命令）＝ {item,next} 8B セルの連結（先頭ヘッダ this+0x10 に埋込）** | `FUN_0046B680`（先頭取得）／`FUN_0046B6C0`（**追加**: 歩き一致があれば return・末尾に `FUN_0067B1E0`（append）or 空なら **`FUN_00446CB0`（push）**、削除分岐は末尾セル free＝pop）／**`FUN_0067B1E0`（[確定]）＝末尾 append**（cell `{item,0}` を `FUN_00401F00(8)` で作り末尾の next に繋ぐ）。caller: `0x451210`（NextFormEx）と `0x46C460`。 |
| **V-534-3: topic リスト（`DAT_00b33a98+0x7c`）追加経路** | **[部分確定]** | 追加の直接 site は `+0x7c` を即値書きする命令を持たず、**先頭ポインタ経由の append（`FUN_0067B1E0`/`FUN_00446CB0`）で行われる**。TESTopic 領域（0x52F000..0x531FFF）の append caller: `0x52F830`（新会話生成: `FUN_0052F790→FUN_0052F010` で topic を作り `FUN_0067B1E0(iVar4)`）、`0x52FC40`、`0x530C40`、`0x530FB0`、`0x531290`。TESTopic ctor `FUN_0052FB40` は `*param_1 = &PTR_FUN_00a5555c`（TESTopic vtable）・`+0x24=0x39`（'9'＝DIAL 型）を設定。**リストそのものへの登録は会話生成系（`FUN_0052F010` 本体）側の追跡が必要で、そこは残点**。 |

## §2 証拠

### §2.1 `FUN_00450080`（docs L61148 要約）
```c
stream = this->0x10; if (!stream) return 0;
if (stream->vtable[+4](stream, &this->0x23c, 0x14, &read=1, 1) == 0) { 0x23c..0x24c = 0; return 0; }
if (this->0x23c != 'GRUP' && this->0x23c != 'TOFT') {
    plugin = (this->0x3f4) ? FUN_0044FD60(this->0x24b + 1) : 0;   // plugin record
    if (plugin) { this->0x248 = plugin->0x400 << 24 | formID & 0xffffff; return 1; }
    if ((formID & 0xff000000) != 0xff000000 && FUN_0046ABD0(formID & 0xffffff)) { this->0x24b = 0; return 1; }
    this->0x248 = this->0x400 << 24 | formID & 0xffffff;          // 自分の mod index を焼く
}
return 1;
```

### §2.2 `FUN_00450F10` / `FUN_0044FEA0`（docs L61866/L61017 要約）
```
F10: if type=='GRUP' || !(flags&0x40000): read 6B from stream → {+0x250=type, +0x254=size}
     else: if (+0x414==0) FUN_00450550();  {type,size} = *(+0x414 + +0x260)
     +0x250/+0x254 反映; if (type=='XXXX') { FUN_00450C20(&n,0); FUN_0044FEA0(); +0x254=n; }
FEA0: +0x260 += +0x254 + 6; if (+0x260 >= (+0x240 | compressed なら +0x418)) {本体クリア; return 0;}
      非 compressed: seek(+0x25c + 0x14 + +0x260); FUN_00450F10(); return 1;
```

### §2.3 `FUN_00451A60`（docs L62512 要約）
```
FUN_0044FBC0();
if (!this->0x10) { if (!FUN_004515B0(this->0x120, this->0x1c, 0, 0)) return; }  // open
else { seek 0; 0x25c/0x260/0x264/0x23c..0x24c = 0; FUN_004511C0(); }
this->0x258 = stream->vtable[+0x1C]();                       // ★ file size（docs の "param_1+600"）
if (this->0x23c != 'TES4' /*DAT_00b05e14*/) return;           // 先頭検証
... +0x3dc |= (bVar1=+0x3df)<<0x18 等、ビット組替
loop: EnsureCurrentRecord(); index = FUN_0046AF70(+0x23c);
      if (index != 1 /*AGGR*/) { +0x28c = (index==2||0); seek 0; 全クリア; FUN_004511C0(); return; }
      FUN_004518B0(1);                                        // AGGR を読み飛ばす
```

### §2.4 `FUN_004DB4A0` 生バイト（else 分岐訂正）
```
4DB507: mov eax,[esi+8] / shr eax,0xa / and al,1 / ret        ; = bit10 getter（0x4D7030 と同一）
4DB4F7: mov ecx, edi / call 0x44faa0                          ; ecx=リスト先頭 item
```

### §2.5 `FUN_0067B1E0`（docs L441554）＝末尾 append
```c
if (item) { head = this; while (head->next) head = head->next;
            if (head->item) { cell = alloc(8); cell->item = item; head->next = cell; }
            else head->item = item; }
```

## §3 判定の内訳

- **V-534-2 完遂**: ロード系の 3 関数（ヘッダ読込・sub ヘッダ読込・open）が確定。特に **formID mod byte 補正（`FUN_00450080`）** と **`+0x258`=fileSize の書き込み元（open 時の `vtable[+0x1C]`）** は移植 ESМ ローダに必須。'XXXX' 分割 sub-record の読込側処理（`FUN_00450F10` の再帰）も v529 書込側と対で確定。
- **V-534-1 部分完遂**: else 分岐＝bit10 のみと訂正（v533 のマスク記述は decompiler 副産物）。`FUN_0044FAA0` の引数＝先頭 item も確定。**item の型（＝リストのドメイン）は未確定**のまま残点。
- **V-534-3 部分完遂**: append 機構（`FUN_0067B1E0`）と TESTopic 側の caller 群は確定。`+0x7c` ヘッドへの直接登録命令は存在せず、`FUN_0052F010`（会話生成）内部のリスト操作に埋もれている → 本体追跡を残点化。

## §4 残点（V-535-1..3）

1. `DAT_00b33a98+0x7c` topic リストの**登録 call site**（`FUN_0052F010` 本体と `FUN_0067B1E0` の ecx が `b33a98+0x7c` になる経路）の確定【中】
2. `FUN_004DB4A0` が参照する `form/manager +0x10` リストの **item 型**（`+0x3dc` bit0 意味との対応: TESFile 説 vs DataHandler 子オブジェクト説）の決着【中】
3. `FUN_004518B0`（AGGR スキップの読み進め）と `FUN_00451920` の本体（v529 probe で名称未確定のまま）【低】

## §5 後続への申し送り

- **移植側 ESM ローダ**: ヘッダ 20B 読 → **mod byte 補正**（vc の `+0x24b` バイトが指すプラグイン配列 index → そのファイルの mod index を formID 上位バイトに焼く）→ sub-record ループ（`+0x250/+0x254`、6B ヘッダ、'XXXX' で分割）→ `+0x260` 歩み → 次。compress フラグ `0x40000` は **ファイル全体で一度だけ +0x414 に解凍**して内部オフセットで読む方式。
- **`+0x258`（fileSize）は open 時に `vtable[+0x1C]`（getSize）で埋まる**。NextForm の EOF 判定（v533）と対。
- **'AGGR'（型 index 1）は open 中に読み飛ばされる**（`FUN_00451A60` のループ）。移植時に AGGR を form として扱ってはいけない。
- **bit10（`flags>>10`）は「配置系」マーカー**（v531 の `+0x78` getter・v532 の `FUN_004E0000` 分岐・本件 else 分岐の 3 箇所で相互裏付け）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
