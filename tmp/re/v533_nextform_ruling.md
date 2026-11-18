# v533 — ロード側レコード状態機械の全确定（`FUN_00451310`＝`TESFile::NextForm`（実文字列で自己同定）・`FUN_004511C0`/`FUN_00451210`/`FUN_004512A0`/`FUN_00451460`/`FUN_00451580` の役割と `TESFile+0x23c..+0x25c` フィールド表）・`FUN_004DB4A0` ゲート trio の identity（`FUN_0046B680`＝リスト先頭取得・`FUN_0044FAA0`＝`+0x3dc` bit0・`FUN_0045A500`＝v529 既出）・`FUN_0052FBA0`（TESTopic `+0x34`）の型文字分岐（V-533-1..3 完遂・旧残点 V-406-3 継続第63弾）

- 採番: **v533**（旧残点群消化・probe prefix **v533_1..5 全5本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-533-1..3（v532 残点）** — 1=`FUN_00451310` 本体＋ロード側 GRUP スタック（`file+0x23c/+0x240/+0x244`）の開閉状態機械／2=`FUN_004DB4A0` ゲート trio（`FUN_0046B680`/`FUN_0044FAA0`/`FUN_0045A500`）の identity／3=TESTopic `+0x34`=`FUN_0052FBA0` 本体と topic リスト（`DAT_00b33a98+0x7c`）の登録/解除 site。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-533-1: `FUN_00451310` の正体** | **[確定]（実文字列＋命令）`TESFile::NextForm`** | 中に **`"Trying to load a bad form in TESFile::NextForm.\r\nFile = %s\r\nOffset = %d"`** と **`"SetFilePointer() in NextForm failed with error:\n%s"`**（`FormatMessageA` 併用）を持つ＝自己同定。挙動: ①`+0x23c`（現レコード type）が `'GRUP'`/`'TOFT'` なら **`+0x25c += 0x14`**（ヘッダ分だけ先へ＝グループに入る）、else **`+0x25c += +0x240(dataSize) + 0x14`**（レコード全体をスキップ）②`+0x25c < +0x258(=600, fileSize)` なら seek して実位置を `+0x25c` に更新、`+0x23c..+0x24c` クリア、**`FUN_004511C0()`** で次ヘッダ読込 → 成功 1／bad form 警告 0 ③範囲外は全クリア 0。 |
| **V-533-1: `TESFile` ロード状態フィールド表** | **[確定]（命令）** | **`+0x23c`＝現レコード type（packed 4CC、0=無効）**／**`+0x240`＝dataSize**／**`+0x244`＝flags**／**`+0x248`＝formID**／**`+0x24c`＝vc（版本カウント）**／**`+0x250/+0x254`＝本體読込カーソル／残り長**（`FUN_00450F10` が读本體時使用）／**`+0x25c`＝現ファイルオフセット**／**`+0x260/+0x264`＝エラー／状態 scratch**／**`+0x258`＝ファイルサイズ**。`FUN_00451210` は同じヘッダを**フォーム側へ転写**する（`form+4 ← type index`、`form+8 ← flags`（**現 flags bit14 立上なら 0x4000 を立てて転写**）、`FUN_0046C300(+0x248,1)`＝formID 経由のファイル紐付け、`FUN_0046B6C0`＝現ファイルのフォームリストへ追加）。 |
| **V-533-1: ロード側 GRUP の開閉状態機械** | **[確定]（命令）** | **開**: `NextForm` が `+0x23c=='GRUP'` を見ると `+0x25c += 0x14` だけ進め、**グループ内部の次レコードを先頭から読む**（v531 で見た `FUN_004D1340` の `FUN_004CCD00` 走査が `NextForm`/`FUN_004518B0` で子をたどる）。**閉**: `FUN_00451580`＝`+0x23c='GRUP'` のとき **`+0x23c=0`・`+0x240 -= 0x14`** にして `NextForm` を呼ぶ → 次の `NextForm` は else 分岐で `+0x240+0x14`（＝**ちょうどグループ合計サイズ分**）進み**グループ末尾へ脱出**。`+0x244 & 0x40000`（v529 の TOFT 分岐）と `'TOFT'` 併記（`FUN_00451310` 冒頭）＝**'TOFT' も「中に入る」型**。'GRUP'/'TOFT' のみ header-only 前進という対称性。 |
| **V-533-1: 補助関数** | **[確定]（命令）** | `FUN_004511C0`＝「`+0x23c==0` なら `FUN_00450080`（ヘッダ読込）→ `+0x250/+0x254=0` → `FUN_00450F10`（本體展開）、最後に **`FUN_0046AF70(+0x23c)`＝型 index を返す**」＝**EnsureCurrentRecord**。`FUN_004512A0`＝全フィールド 0 クリア＋seek 0（**Reset/Rewind**、末尾で `FUN_004511C0` 初回読込可）。`FUN_00451460`＝`+0x414/+0x418`（一時バッファ）解放 → `+0x25c=param_2` seek → 全クリア（**SeekTo**）。`FUN_004518B0`/`FUN_00451920`/`FUN_00451A60`＝本體読込系（+0x23c..+0x254 を読む／進む）。 |
| **V-533-2: `FUN_0046B680`** | **[確定]（命令）「`this+0x10` の {item,next} 埋込リストの先頭（非空）要素を返す汎用 getter」** | `piVar3=this+0x10` から `*piVar3` が空なら次のセルへ進み、**`param_2==-1` なら最初の非空 item**、そうでなければ `param_2` 個スキップ後の item を返す（尽きたら 0）。caller は 40 箇所超（`0x406360`・`0x415960`・…で `push -1; mov ecx,<obj>; call`）。`DB4A0` では `ecx=this(form)`＝**form+0x10 のリスト（＝ファイルのフォームリスト連結構成）の先頭が居るか**を gate 条件にしている。 |
| **V-533-2: `FUN_0044FAA0`** | **[確定: 命令]／[推定: 名称]「`*(this+0x3dc) & 1`＝データ読込済フラグ getter」** | 本体 1 行。`0x44FAxx` 近傍は DataHandler クラスタ（`FUN_0044F3D0`＝プラグイン起動・`+0x8d0/+0x8d4` ファイル配列・`+0xCD4` isCompiling は v529 で確定済み）。`DB4A0` では **0（未読込）のときゲート true** に寄与。 |
| **V-533-2: `FUN_0045A500`** | **[確定]（v529 再確認）** | `GetCurrentThreadId()==DAT_00b33398+0x10` なら `+0x18` bit0、他 thread なら `+0x18 >> 0x12` を返す（manager の thread 依存バイト・v529 ⑩）。 |
| **V-533-2: `FUN_004DB4A0` 再統合** | **[確定]（命令）** | 「**現ロード／保存文脈で REFR が『配置保存』側か『変更追記』側かのゲート**」: `this+0x10` リスト先頭有 & TLS+0x184==0 &（`FUN_0045A500()`==0 or manager+0x18 bit4）& DataHandler+0xCD4==0 & `+0x3dc` bit0==0 なら **1**、else `flags>>10 & 0xFFFFFF01`（**bit10 か bit18↑ が立っているかのマスク**）。v532 の 8/9/10 分岐（8=DB4A0 真）と接続。 |
| **V-533-2: `FUN_0046B6C0`（NewForm で呼ばれる追加処理）** | **[確定]（命令）「現ファイルのフォームリストに form を追加／arg0 で末尾 pop」** | arg!=0: `FUN_0044FB90()`（末尾セル探索の副次）→ 未読込時 `this+0x10` を歩き一致があれば return、無ければ `FUN_0067B1E0(form)` 追加／読込中なら `FUN_004526E0()`／いずれでも無ければ **`FUN_00446CB0`（v530 確定の単方向リスト push）** で追加。arg==0: `this+0x10` 末尾の非空セルを見つけて link 外し free（**pop**）。 |
| **V-533-3: `FUN_0052FBA0`（TESTopic `+0x34`）** | **[確定]（命令＋docs）「subject の型文字で分岐する所属検証」** | `subject->type(byte+4)` が **`'9'`(0x39=DIAL)** → TESTopic 検索（TD `0xB05390`）→ **`this->+0xC（topic formID） < found->+0xC` なら true**（同一 DIAL 内の formID 順序＝後続 topic が自分の下に入る）／**`':'`(0x3A=INFO)** → TESTopicInfo 検索（TD `0xB0544C`）→ `FUN_0052F690(info)` の所有者 topic が**自分なら true**、別人なら `vtable[+0x34]` 委譲、所有者無し 0／**else → 基底 `FUN_0046B500`**（`this->type < subject->type`）。 |
| **V-533-3: topic リスト登録／解除** | **[部分確定]** | `DAT_00b33a98+0x7c`（DataHandler 埋込の TESTopic リスト）の読み手は `FUN_0052F690`（所有者検索）・`FUN_0052F6D0`（**`+0x1c` 名前 `__mbsicmp` によるバブルソート**）・`FUN_0052FAE0`（全 topic 走査で `FUN_0052F330` を呼ぶ＝**topic 破棄／リセット系**）と判明。**追加 site 自体は `+0x7c` を直接書く命令として未発見**（ノード経由の push らしく、`FUN_0052F5D0` は「topic の `+0x28` サブリストから formID 一致の子探す」検索側のみの caller）→ 登録 site 特定は残点。 |

## §2 証拠

### §2.1 `FUN_00451310`（= `TESFile::NextForm`）docs L62132 の要点
```c
if ((*(int *)(this+0x23c) == DAT_00b05e20/*GRUP*/) || (*(int *)(this+0x23c) == DAT_00b06138/*TOFT*/))
    this[0x25c] += 0x14;                          // group 系: ヘッダ分だけ前進（中に入る）
else
    this[0x25c] += this[0x240] + 0x14;            // 通常: dataSize+ヘッダ分スキップ
if (this[0x25c] < this[0x258]) {                  // 0x258 = fileSize
    file.vtable[+0xC](pos, DAT_00a853d0);         // seek BEGIN
    pos = file+0x30 (if -1 file+0x148); this[0x25c] = pos;
    if (pos != -1) {
        this[0x260]=0; this[0x264]=0;
        this[0x23c]=0; this[0x240]=0; this[0x244]=0; this[0x248]=0; this[0x24c]=0;
        FUN_004511c0(this);                        // EnsureCurrentRecord
        if (this[0x23c] != 0) return 1;
        FUN_004A7A60("Trying to load a bad form in TESFile::NextForm.\r\nFile = %s\r\nOffset = %d", this+0x1c, this[0x25c]);
        return 0;
    }
    FormatMessageA(...); FUN_004A7A60("SetFilePointer() in NextForm failed with error:\n%s", ...); return 0;
}
this[0x23c]=0; ... this[0x24c]=0; return 0;        // EOF
```

### §2.2 `FUN_00451580` との往復（グループ閉じ）
```
FUN_00451580(this): if (this[0x23c]=='GRUP') { this[0x23c]=0; this[0x240]-=0x14; FUN_00451310(this); }
→ 次の NextForm: type==0 → else 分岐 → +0x25c += (size-0x14)+0x14 = size → グループ末尾へ（脱出）
```

### §2.3 `FUN_0046B680`（汎用リスト先頭 getter）docs L85047
```c
cell = this + 0x10;
do { if (cell==0) return 0; item = *cell; cell = cell[1];
     if (item!=0 && n != -1) { cnt++; if (cnt > n) return item; } } while (item==0 || n==-1);
```

### §2.4 `FUN_004DB4A0` 生バイト入口
```
4DB4A0: push esi / push edi / push -1 / mov esi,ecx / call 0x46b680   ; ecx=this → form+0x10 リスト先頭
```

### §2.5 `FUN_0052FBA0` docs L226018
```c
if (subject[4] == '9') {         // DIAL
  t = cast(subject, TESTopic);   // TD 0xB05390
  return t != 0 && this->formID < t->formID;
} else if (subject[4] == ':') {  // INFO
  i = cast(subject, TESTopicInfo); // TD 0xB0544C
  if (i) { owner = FUN_0052F690(i);
           if (owner) return owner==this ? 1 : this->vtable[+0x34](owner); }
  return 0;
}
return FUN_0046B500(subject);    // base: this->type < subject->type
```

### §2.6 `FUN_0052F6D0`（topic ソート）と `FUN_0052FAE0`（topic 全体走査）
```
F6D0: list=DAT_00b33a98+0x7c を要素数だけバブルソート（比較キー = form->+0x1c 名前の __mbsicmp）
FAE0: list を歩き各 topic の +0x28 サブリストで param_1 に一致する子があれば FUN_0052F330(topic) を呼ぶ
F5D0(topic, formID, reverse): topic+0x28 の {item,next} を順／逆引きで formID 一致の子を返す
```

## §3 判定の内訳

- **V-533-1 完遂**: `FUN_00451310`＝**`TESFile::NextForm`**（実文字列で確定）。ロード側の「開く＝`+0x25c += 0x14`・閉じる＝`FUN_00451580` の `+0x240 -= 0x14` 経由でグループサイズ分スキップ」の対称状態機械と、`TESFile+0x23c..+0x25c` の全フィールド意味を確定。`FUN_004511C0`（EnsureCurrentRecord・型 index 返却）／`FUN_00451210`（ヘッダ→form 転写＋bit14→0x4000 変換＋フォームリスト追加）／`FUN_004512A0`（Reset）／`FUN_00451460`（SeekTo）まで一括解決。
- **V-533-2 完遂**: ゲート trio は `FUN_0046B680`＝{item,next} 埋込リスト先頭 getter、`FUN_0044FAA0`＝`+0x3dc` bit0（読込済フラグ getter・名称推定）、`FUN_0045A500`＝v529 確定の thread バイト。`FUN_004DB4A0` はこれらを合成した「配置保存 vs 変更追記」ゲートと確定（`+0x10` リストの意味詳細は form クラスタ側に残点）。`FUN_0046B6C0`＝現ファイルのフォームリスト push（`FUN_00446CB0` 再利用）/pop も確定。
- **V-533-3 完遂（登録 site のみ残し）**: `FUN_0052FBA0` は DIAL='9'/INFO=':' の型文字分岐と formID 順序／所有者照合で確定。topic リスト（`DataHandler+0x7c`）の読み手 3 関数（検索・ソート・走査）を特定、**追加 site は未発見**（残点 V-534-3）。

## §4 残点（V-534-1..3）

1. `FUN_004DB4A0` が条件に使う **form（REFR）`+0x10` リストの正体**（`FUN_0046B680` の this が REFR の場合の {item,next} 列＝何のキューか）と、else 側 `flags>>10 & 0xFFFFFF01` マスクの意味（bit10＋bit18↑ の何れか）【中】
2. `FUN_00450F10`（本體展開）と `FUN_00450080`（ヘッダ読込）の内部（zlib/`+0x414` バッファ関係）＋`FUN_00451A60`（+0x23c.. を読む 5 行の正体＝ReadRecordHeader 候補）の確定【中】
3. `DataHandler+0x7c` TESTopic リストへの追加／削除 site 特定（`FUN_0052FAE0` 系が走査専用だったため、登録は quest ロード経由とみられるが命令追跡未了）【低】

## §5 後続への申し送り

- **移植側の `TESFile` ロールは「+0x23c..+0x25c の 8+2 dword 状態機械」で再現できる**: レコード走査＝`NextForm`（'GRUP'/'TOFT' は +0x14 のみ前進、それ以外 +0x240+0x14 前進）、グループ離脱＝size-0x14 補正後の NextForm。保存側（v529/v530）の `+0x268` 群とは別セットである点に注意（読 0x23c.. / 書 0x268..）。
- **`+0x244 & 0x40000`（'TOFT' 相当フラグ）と `'TOFT'` 型は「中に入られるもう一方の型」**。移植時に GRUP だけの特殊扱いだと TOFT（to-hit 集約?）で壊れる。
- **`FUN_00451210` の flags 転写は bit14→0x4000 変換を伴う**（v527 の「bit14=deleted」がロード側で 0x4000 として可視化される）。
- **`FUN_0046B680` はフォームファイルリスト汎用 accessor**（DataHandler・manager・form の各 `+0x10` に同じセル構造が再利用されている）。移植側は単一の list-getter 実装で足りる。
- **TESTopic 系は型 index を ASCII 文字そのもの（'9'/'…'）で比較している**（DIAL=0x39・INFO=0x3A）。移植時に enum ではなく数値 0x39/0x3A で判定すると元の分岐と一致する。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
