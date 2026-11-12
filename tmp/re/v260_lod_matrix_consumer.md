# V260 — .lod エントリ→描画行列の合成消費者（Oblivion.exe v1.2.0416 Steam）

- 依頼: 実装セッションからの依頼1（`p65-objlod-render` ブロッカー）—「.lod エントリ（formId+pos+rot yaw+scale）から描画ノードの行列を最終的に組み立てる関数」の特定
- 手法: EXE 静的解析（capstone 5.0.7・**selftest 4/4 PASS 着手時**・VA 換算=セクション個別 imm32_refs3 準拠）＋既有成果物（v255/v258・再検証なしで足場化）
- クリーンルーム: 外部逆コンパイル由来コード未参照。結論は EXE 命令・実データからのみ導出
- 証跡: 生データ `tmp/re/v260_*.txt` / 台帳 `tmp/re/v260_lod_chain.csv` / スクリプト `%TEMP%\opencode\v260_{1..26}.py`
  - 注記: 依頼文はスクリプトも tmp/re 希望だが、本セッション規約（`%TEMP%\opencode\v260_*.py`）を優先した
  - `tmp/re/re_util.py` は無変更（利用のみ）。既存 tmp/re ファイルの上書きなし（本 md と v260_* はすべて新規）

## 判定サマリ（最終）

| 項目 | 判定 |
|---|---|
| 依頼1「最終行列組立関数」の特定 | **[確定]** consumer=`0x4BCEF0` / 合成実装=`0x4BAA80`（TESObjectSTAT::vft+0x11C）/ 実投入=`0x7B4010`+`0x812510`+`0x802AE0`（§2・§4） |
| pos/yaw/scale→行列の**合成順序** | **[確定] 回転なし：`M = T(pos)·S(scale)` の平行移動+一様スケールのみ。yaw（rot float3）は出荷エンジンで一切消費されない**（§4・否定機械検証付） |
| 回転中心（origin/pivot）・Z 軸回転可否 | **[確定] 適用対象が存在しない（回転不使用）ため概念として不在。pos はワールド絶対値を直使用。Z 軸回転も非実装**（§4） |
| バインド地点 | **[確定] 主経路 = ブロックグループ（NiTMap `0xB2C33C`・0x7B3BE0 解決）への 16B エントリ append + flush で D3D バンドノード化。副経路（NIF 既読込）= `0x4BA780` の NiNode クローン＋`0x405680`（AddChild）**（§3） |
| 子配列 `+0xB0`/`+0xB6` | **[確定] NiNode の children データポインタ / 子数(u16)（ヘッダ `+0xAC`）。追加は `0x405680`/`0x523B10` 系**（§3-1） |
| vft `0xA7E38C` の正体 | **[確定] `.?AVNiNode@@` = NiNode 本体（RTTI 解決）**（§0-1） |
| 候補6関数（0x70ACF0 他） | **[確定] NiNode 標準仮想（Update 伝播/比較/Clone）。合成関数ではない — v258 V-258-4a 仮説は否定**（§0-2） |
| ctx+0x1C / +0x20 の実体 | [部分確定] 参照カウント付きポインタ（setter `0x75FA90`）。+0x20 は `[+4]` 型値で分岐するリソース系オブジェクト。意味の最終確定は残点 |

---

## 0. 前提の訂正と新確定事項（命令レベル）

### 0-1. vft `0xA7E38C` = **NiNode クラス本体**（RTTI 解決）
- VS6 COL 規約（v259 確立）: `[0xA7E38C-4]=[0xA7E388]=0xAC8B84` → COL{0,0,0,TD@+C,CHD@+0x10}、TD=`0x0B030E8`、TD+8 インライン名 = **`.?AVNiNode@@`** [確定]
- v258 §4-3 の「RTTI 名取得不可（TD=0xABC204）」は COL/TD 取り違え（0xABC204 は別 vft 0xA45AF0 の COL）。
- よって「vft 0xA7E38C のメソッド群の中に行列合成がある」（v258 V-258-4a）は**追跡対象の誤り**だった。合成先は **form（TESObjectSTAT）の vft**（§2）。

### 0-2. 候補6関数の実体 = NiNode の仮想スロット（合成関数ではない）— [確定]
vft 0xA7E38C スロット表（v260_1 dump）: `[06]=0x70B980 [07]=0x709EE0 [08]=0x70B530 [09]=0x70ACF0 [0A]=0x70B6A0 [0B]=0x70AD70 [0C]=0x70BAE0`（+クローン 0x70B800/0x70BA00 群）
- `0x70ACF0`: `[+0xB0]`×`[+0xB6]`(u16) で子の vft[+0x24] へ引数伝播 + `[+0xC0]` 連結リスト伝播（**Update 系プロパゲーション**）
- `0x70AD70`: 二者の子配列長・内容比較（**IsEqual**）
- `0x70B6A0`: 子配列を vft[+0x2c] 経由で比較コピー（**Clone 系**）/ `0x70B530`: 子の `[+0xD8]` 検査+`[+0xAC]` NiTArray 操作（**UpdateWorldBound/GEOM 系**）/ `0x70B980`=[0E]: `new 0xDC; call 0x70B780(childCount)`（**CreateClone**）/ `0x70BAE0`=[0C]: 更新データ転写
- entry 配列（+0x04/+0x14/+0x24/+0x30）を読む命令は**6 関数いずれにもなし**。`.text` 全域の「同一 base で entry-offset 2 つ以上」走査も 0 件（v260_2-A・否定証跡）— 消費者がマップ走査で**配列ポインタを直接積む**構造だったため（§1-2 で解明）。

### 0-3. エントリ 0x34B の実体 = **NiTArray ×3 ヘッダ直列＋n**（RTTI・ctor）
- ctor `0x4CC7C0`: `[eax]/[eax+0x10]/[eax+0x20] = 0xA45AF0`（3 配列とも同一 vft）+ 各ヘッダ {+4: data=0, +8/+A/+C word=0, +E word=1}、`+0x30`=n（パーサ書込・v255 継続）
- vft 0xA45AF0 RTTI: COL `0xABC204` → TD `0x0B087E0` → **`.?AV?$NiTArray@M@@`** — 「エントリの vft」ではなく **NiTArray\<unsigned\> の vft**（v255 の「opcode 0x34/vft 0xA45AF0 インスタンス化」表現の正体）
- ctor 呼び出し元は全 .text で 2 件のみ: `0x4D1852`（.lod パーサ内）/ `0x4D454C`（ESM ビルダ `0x4D43F0` 内）→ **エントリ生成経路は 2 のみ**（再確定）
- 挿入 map のノード = `{+0: next, +4: key=form*, +8: value=entry*}`（insert `0x452570`: arg1=キー・arg2=値の命令列確定・v260_1/16）

### 0-4. task+0x2C「ctx」= **ロード要求レコード**（NiNode ではない）— [確定・v258 訂正]
- Run `0x4BCC60`（v255_4）: `esi=[task+0x2C]` を `this=[esi+8], (esi.x, esi.y, &esi+0xC, esi+0x24, file)` で 0x4F1990 へ → **ctx={+0:x, +4:y, +8:ws mgr, +0xC:NiTMap(37bk), +0x24:mode, +0x28:loaded}**
- ctx ctor `0x4BCE00`（`0x4BD4FD`@ファクトリ 0x4BD430 から呼出・v260_4）: `[+0xC]=0xA45AD0`（map vft）`[+0x10]=37` `[+0x14]=buckets` `[+0x28]=0` ✓ 完全一致。`+0x1C/+0x20` は `0x75FA90`（addref/release 書換セッタ・v260_16）でファクトリ引数 4/5（`[0xB34424]` グローバル／パス系オブジェクト）を保持 [部分]

---

## 1. consumer の実体 = `FUN_004BCEF0`（[確定]・命令列・esp シミュレーション v260_20）

```
0x4BCF03  cmp byte [ctx+0x28],0  → loaded ゲート（Run@0x4BCCA2 が 1 を書く）
0x4BCF0C..1C  ctx+0x20/ctx+0x1C 必須（null 時スキップ）
0x4BCF1E/22  y=ctx+4, x=ctx+0
0x4BCF29  call 0x7B2110(x,y) → key1 = ((x&~3)<<14 | (y>>2))   ; 4cell ブロックパック（純関数）
0x4BCF3B  call 0x4EF1D0(x,y) → key2 = (x<<16|y)               ; セルパック（純関数）
0x4BCF40  ebp = &ctx+0xC（map）; 0x4BCF4C begin=0x6A9030; 反復=0x452600(&it,&keyout,&valout)
   esi = valout = entry ; ecx = keyout = **form***（esp スロット S+0x24=S+0x24 検証済）
0x4BCF8E  eax = [esi+0x30]（n）; n==0 スキップ
   push 列（1→8 積順）: ctx+0x20, ctx+0x1C, key2, key1, n, [esi+0x24]=scale, [esi+4]=rot, [esi+0x14]=pos
0x4BCF98  mov edx,[ecx] ; edx=[edx+0x11C] ; call edx   ; ★★★ form->vft+0x11C ★★★
           ; this=form, args=(pos[], rot[], scale[], n, key1, key2, ctx+0x1C, ctx+0x20)（ret 0x20 ✓  callee 側 0x4BAC90 ret 0x20 と相互一致）
0x4BCFC4..  entry 3 配列 free → entry free（**使い切り**。vft 0xA45AF0 を戻してから free）
0x4BD006  map dtor（0x7C2A40）
0x4BD00D..  [ctx+0x20]+4 型値>2 なら mgr[0xB34424]->vft+0x84(1, obj) 通知 → 0x707370(0.0,1) → 0x707610（**再スキャン要求**）
0x4BD048  ret 4
```
- 起動元 = タスク完了ハンドラ `0x4BD1F0`（task vft `0xA45A68`+0x14・**唯一の呼び出し元**）: `call 0x4BCEF0(this=[task+0x28], arg=[task+0x2C]=ctx)` → 之後 `call [this vft+0x10](key2)`（v260_5）
- ESM 側（0x4D43F0）も同一エントリ・同一 map を生成するため、**この consumer は .lod ファイル/ESM 双方の合流点**（v258 件4-1 の対称性の延長・エントリ ctor 呼び出し元 2 件の一致）

---

## 2. 合成チェーン全体（v260 最重要成果・全 VA 命令実証）

### 2-0. 呼び出し先クラス = **TESObjectSTAT 本体**
- 消費者の `call [form vft+0x11C]` の form は formType 30 保証済（§0-3・`0x4D1800`）
- **STAT ctor = `0x4B9DA0`**（form ファクトリ `0x447DF0` のジャンプテーブル `0x448B60` idx26 = formType 30 → `new 0x80` + `0x4B9DA0`・v260_16/17）。ctor は `[esi+4]=0x1E`（formType）と primary vft **`[esi]=0xA44FB4`**（`0x4B9E02`・最後の [esi] 書換。0xA44F10/0xA44F94/0xA44F78/0xA44F6C は入れ子 +0x24/+0x3C/+0x48 サブオブジェクト vft）を書く
- **`vft 0xA44FB4 + 0x11C = 0x4BAA80`** = 依頼の最終行列組立（具現化）関数 = **TESObjectSTAT::vft+0x11C** [確定]

### 2-1. `FUN_004BAA80`（合成エントリ・thiscall・stack 8 引数 ret 0x20）
```
0x4BAACB..  null ゲート: ebx=arg1(pos), eax=arg3(scale), arg4(n), arg5 — いずれか 0 で打ち切り
0x4BAAFC  cmp byte [0xB125E8],0 でゲート（distant object LOD 有効フラグ）+ 0x4B9CB0(form) bool
0x4BAB1C  call 0x7B2A00(formID) — 「その NIF は既読込?」（NiTMap 0xB2C34C find・v260_22）
0x4BAB26  jne 0x4BAC11（既読込 → 具現化フェーズへ）
 (未読込:) 0x4B9C40(form,&name) = MODL→".dds" 変換 / 0x47D8F0 / [0xB35300]->vft+4（非同期ロード要求帯）/ 0x405070
   → new 0x1C → 0x4BA2D0(...)（§2-2）→ 0x438670(this=[0xB33A1C], &key, rec) 登録 → 終了
   → [esp+0x14]（0x405070/0xB35300 系統のロード要求ハンドル）非 null のとき 0x4BABEE: 0x4BA780(form,1)（即時 clone 具現化・§2-3）→ 成否で 0x7B4010 進
0x4BAC11  0x7B20B0(&local28) → 0x7B4010(this=&local28; args: pos[], n, formID(local+4), aux, cellPack, blockPack/ctx ptr 群, clone=edi 前置)（§2-4）
0x4BAC90  ret 0x20
```
- 分岐は 3 系統: ① NIF 既読込（0x7B2A00=true）→ 直接 0x7B4010 インスタンス投入 ② 未読込・ハンドル無 → rec(0x1C) 化 + 0x438670 登録のみ（非同期完了後が本命） ③ 未読込・ハンドル有 → 0x4BA780 clone 供給 → 0x7B4010（②③で rec 登録と具現化が分離する構造）
- **重要（機械検証）: arg2 = rot 配列ポインタのスロット `[esp+0x248]`（=arg2）へのアクセスは 0x4BAA80 全体で 0 回**（v260_24 シミュ出力 grep・一致 0 行）。rot は downstream（0x4BA2D0 のコピー元 2 本・0x7B4010 の配列 2 本）にも一切現れない → **rot 廃棄は composer フェーズ** [確定]

### 2-2. `FUN_0x4BA2D0`（0x1C レコード ctor・thiscall stack 7 引数 ret 0x1C）
```
[rec+0x00]=form* / [rec+0x04]/[rec+0x08]=共有 aux ptr ×2 / [rec+0x0C]=key 群
[rec+0x10]=n / [rec+0x14]=new(12·n)←memcpy(0x9812C0, pos[], 12n) / [rec+0x18]=new(4·n)←memcpy(scale[], 4n)
```
→ rec は **pos と scale を自前所有コピー**（NiTArray ヘッダ経由で寿命独立化）。**rot コピーなし**（プッシュ列に rot ポインタが存在しない）
- 登録先 `0xB33A1C`（0x438670 経由）はデータ/フォーム mgr 系コレクション（参照帯多数・v260_25）。rec の二次消費者一覧は残点 V-260-3

### 2-3. `FUN_004BA780`（未読込だがロード要求ハンドルあり＝即時具現化分岐・v260_22/25）
`new 0xC0→ctor 0x717570（vft 0xA7ED5C）` + 0x6FF420(push 0xA451F0/0xA451D4=名) + `new 0x30→0x7043B0` + **0x405680(parent, child) = NiNode::AddChild 系でツリー結合** + `new 0xE4→0x70B780`（**NiNode ctor**・後続が vft を `0xA45134` に上書き・`[+0xDC]=9` update flag）+ `[node vft+0x84](1, clone)`。**args=(form, 1) のみ — 配列類は受け取らない**（=この分岐でも rot/pos/scale 配列は走らない。走査は §2-4 で完結）

### 2-4. `FUN_007B4010`（★インスタンス実ループ・thiscall(&local28)+7 args）
```
0x7B4015/1D  ebp=arg7=n; add [0xB42D60], n（総インスタンスカウンタ）
0x7B4031..49  ループ: call 0x7B3BE0(cellPack, blockPack, ctx..., &grpOut)  ; ★ v258 件2 と同一 = cell→block÷8 の DistantLOD NIF グループ解決（所有者マップ 0xB2C33C）
0x7B4069-70   esi = posBase + 12*i + 8        ; rec/entry の float3 配列ベース
0x7B4079..97  fld [esi-8],[esi-4],[esi] → local x,y,z ／ 0x7B409B fld [auxBase + i*4] → 4th float
0x7B40A6      call 0x812510（this=grp）— §2-5
0x7B40C0      call 0x802AE0（this=grp）— flush（0x7B40CE の外側ループで cell×cell 走）
0x7B40D4..    (n==0 時) [arg3]->vft+0x78 / NiTMap 0xB2C33C find(key=[arg6+8]) / 0x5B1E20 add
```
### 2-5. `FUN_00812510`（1 インスタンス append・ret 8）— [確定]
```
0x812510  cnt=[grp+0xE] vs cap=[grp+0xC]（u16）— 満杯/未確保なら fail(ret 0)
0x812525  dst = [grp+0x10] + cnt*0x10
0x81252C..43  [dst+0,C] = arg1 の 4 float（= x,y,z,4th float）
0x81254D  [[grp+0x14] + cnt*4] = arg2（aux u32 — 内容=0x7B4010 arg6 由来 [部分・残点 V-260-1]）
0x812555  [grp+0xE] += 1
```
### 2-6. `FUN_00802AE0`（flush・v260_23）— [確定]
`cnt=[grp+0xE]` の走査で **16B エントリ（x,y,z,s）から AABB min/max**（`fld [ecx]/[ecx+4]/[ecx+8]; ecx+=0x10`・半径=定数 `[0xA9372C]`）。`edx=[grp+0x10]... imul` → `0x81A050(0x170 + 36*cap, ...)` で**バッチノード確保** → `new; vft=[0xB4351C]系; [obj]=0x813490; [obj+4]=0x67C0000...` **D3D 帯（0x80-0x81 万）の描画ノードを生成**。36B/インスタンスの内部訳は残点 V-260-2。

---

## 3. バインド地点と `+0xB0`/`+0xB6` の答え

### 3-1. [確定] `+0xB0`/`+0xB6` = **NiNode の children 配列**（データ ptr / 子数 u16・NiTArray ヘッダは `+0xAC`）
- 走査命令（0x70ACF0/0x70AD70/0x70B120 等の `mov eax,[n+0xB0]`×`movzx,[n+0xB6]`）＋拡張命令（`lea ecx,[n+0xAC]; call 0x523B10(resize)/0x4B34E0/0x4B3680(setAt)`・0x70B530@0x70B54F、0x70AF67/AF99（v260_25 で 0x70ACF0=slot[9] 更新実体も併せて確認））
- `[+0xB4]`（容量 u16）と `[+0xB6]`（現在数 u16）の対は 0x70AF77（v260_25）に同時出現 → レイアウト相互確認
- v260_2-B の `[+0xB6]` 直接書換走査 0 件は、追加が常に `+0xAC` ベースの NiTArray メソッド経由（オフセット +0xE/+0x10）であることと整合

### 3-2. バインドは 2 経路 [確定・命令]
1. **主経路（pos/scale 実データ）**: グループオブジェクト（`0xB2C33C` マップのブロック単位 DistantLOD NIF グループ・**land LOD と同一インフラ**）への `0x812510` append → `0x802AE0` flush で `0x170+36n` D3D バッチ化（vft `0x813490`、レンダラ状態 `[0xB4351C/20/24]`）。**NiNode ツリー追加ではない**
2. **NiNode ツリーバインド（分岐③ clone 供給）**: `0x4BA780` が `new 0xC0`（ctor 0x717570→**vft 0xA7ED5C**）+ `new 0x30`（0x7043B0）+ `new 0xE4`（**0x70B780 = NiNode ctor**・vft を `0xA45134` に書換・update flag `[+0xDC]=9`）で枝を組み、**`0x405680`（AddChild 系）で親 clone（0xA451F0/0xA451D4=名）へ接続**（0x4BA964/0x4BA97F・v260_22）。この clone は 0x7B4010 の前置 slot（`mov [esp+0x3c],edi`）経由でグループに紐づく。分岐①（NIF 既読込）では clone 作成を飛ばして直接 0x7B4010 へ
3. 可視性/距離適用: consumer 末尾の `0x707370([0xB34424],0.0,1)`+`0x707610` = シーン mgr 再スキャン要求のみ。**fFadeDistance はこの経路で渡されない**（v258 件3 の描画側消費と整合・[確定・命令+否定]）

---

## 4. 依頼の核心 — pos/yaw/scale → 行列の合成順序 [確定]

**結論: 出荷エンジンの .lod 具現化は回転を一切適用しない。ワールド変換は `M = T(pos)·S(scale)`（平行移動＋一様スケール、回転なし）。**

- rot 廃棄の機械検証: ① consumer→composer 引き渡しは pos/rot/scale/n の 4 ポインタ明証（§1 push 列）② composer `0x4BAA80` の arg2(rot) スロットアクセス **0 回**（grep・v260_24）③ rec コピーは pos(12n)+scale(4n) の 2 配列のみ（§2-2）④ `0x7B4010` ループの配列読みは 12 歩度 float3 と 4 歩度 float の 2 本のみ（4 回の `fld/fstp` が全読み・§2-4）⑤ 本チェーンに sin/cos・**D3DXMatrix\* 系 IAT エントリ 0 件**（v260_25・`D3DXMatrixTransformation`/`RotationZ` 等 import ゼロ = API 経由合成の否定）
- 回転中心: 回転が不在のため**概念として存在しない**。pos はワールド絶対値（v255 §1.5）をそのまま平行移動に使う（pivot 加算なし・`0x7B406D lea esi,[base+12i+8]` が唯一のアドレス算術）
- Z 軸回転: 非実装。データ仕様（slot[2]=yaw・45°倍数）は**ロード/保存専用**で描画未消費 — v255 §4 の [推定]=「Z 軸回転＋一様スケール」は**スケーリングのみが実装**と訂正
- NiMatrix3/NiQuaternion: 本チェーンに登場しない（エントリ→rec→16B エントリで float のまま直渡）。D3DX 系 API も不使用（上記⑤）

**疑似コード（エンジン等価）**:
```
// consumer（タスク完了時・0x4BCEF0）: for (form,entry) in map:
form->InstantiateDistant(pos, rot, scale, n, blockKey4, cellKey, ctx1C, ctx20)   // vft+0x11C = 0x4BAA80
    // rot はこの関数内で読まれない（廃棄）
rec = { form, n, posCopy[n](12n), scaleCopy[n](4n) }        // 0x4BA2D0
registry0xB33A1C.add(rec)                                    // 0x438670
for each cell block b of n:                                  // 0x7B4010
    grp = DistantGroups.find(cellToBlock8(b))                // 0x7B3BE0 / map 0xB2C33C
    for i in 0..n-1:
        grp.entries.append({ pos[i].x, pos[i].y, pos[i].z, scale[i] })   // 16B・0x812510
        total += 1 (0xB42D60)
    grp.flush()   // 0x802AE0: AABB(entries) → batch(0x170+36·cap) vft 0x813490
sceneMgr0xB34424.requestRescan(0.0f, 1)                      // 0x707370/0x707610
// 描画側がインスタンスごとに使う変換: world = translate(pos[i]) · uniformScale(scale[i])
```
**移植実装指針**: エンジン互換なら `{pos, scale}` のみ使用（yaw は無視）。yaw を尊重する拡張実装を採る場合は「エンジン非互換の追加仕様」として明記（v255 §8.2 の案はそのまま有効）。

---

## 5. 否定根拠・試したアドレス一覧（追跡経路の完全記録）
- 候補6関数（0x70ACF0/0x70AD70/0x70B530/0x70B6A0/0x70BAE0/0x70B980）→ NiNode 標準仮想と役目特定（§0-2）。**合成なし**
- `.text` 全域 2 条件 entry-offset sweep 0 件（v260_2-A）／`[+0xB6]` word 書込 sweep 0 件（v260_2-B）— いずれも構造の解説後に矛盾解消（§0-2/§3-1）
- `0x4D6760`（NiTMap byte-find）・caller 8 件（v258_14）= 本チェーン外（.cmp/land/clone 帯）— 0x4BCEF0 は map を直接 walk するため find を経由しない（**v258「find 8 件に見当りなし」の理由を説明**）
- 0x4D1A90（単独 caller 0x405122・STAT 一覧の距離走査／hide 経路）／0x4F1AC0（v255 の Get3D 系はここ）／0x7B2110・0x4EF1D0（パック純関数）／0x6A9030・0x452600（map begin/next）／0x4028D0・0x4CC7C0・0x4BCE00（ctor 群）— 全通過点命令確定
- formType 30 ファクトリ: `0x447DF0` switch（jmp table `0x448B60`・idx26→0x4481C1→new 0x80+ctor 0x4B9DA0）— **STAT 实例 vft=0xA44FB4 の確定経路**
- RTTI 直接追跡（COL→TD `.?AVTESObjectSTAT@@`=TD 0x0B054D4・refs=コード 9 件+COL 0xABB9CC/ABBA2C）: vft 0xA819D0（STAT 基底 view の副 vft・slot+0x11C=0x70B120 RemoveChild 系）も確定済みだが、**インスタンスの primary は 0xA44FB4**（§2-0）

## 6. 残点
- V-260-1 [中]: `0x812510` aux dword（grp+0x14）の各インスタンス内容（0x7B4010 arg6 の同一性）。composer 中間の `call eax`（0x4BAB61・ret クリーン未知）を跨ぐため slot 対応を静的確定できず。**移植には不要**
- V-260-2 [中]: flush バッチ `0x170+36n`（vft 0x813490）の 36B/instance 内訳と頂点/インデックス展開（0x81A050/0x813490 D3D クラスタは範囲外）
- V-260-3 [低]: rec（0xB33A1C 登録）の二次消費者一覧と ESM 側（0x4D43F0）経路の合流確認
- V-260-4 [低]: ctx+0x1C/+0x20 の保持クラス確定（0x75FA90 で入る 2 引数 = ファクトリ arg4 `[0xB34424]` 系 / arg5 パス obj）
- V-260-5 [低]: 0x4BAA80 内 0x4B9CB0 / 0x405070 の意味（ゲートとポインタ書換と推定・命令は dump 済み §2-1 参照）

## 7. 成果物一覧（新規）
- `tmp/re/v260_lod_matrix_consumer.md`（本ファイル）
- `tmp/re/v260_lod_chain.csv`（ステージ×VA×役割×根拠の台帳）
- `tmp/re/v260_rtti_entry_ctor.txt`（RTTI 解決・エントリ ctor/dtor・map insert/find・NiNode slot 表）
- `tmp/re/v260_consumer_parser_disasm.txt`（0x4D16D0/0x4BCEF0 の esp シミュ付きフル）
- `tmp/re/v260_composer_4baa80_disasm.txt`（0x4BAA80 フル）
- `tmp/re/v260_composer_chain_disasm.txt`（0x4BAA80/0x4BA2D0/0x7B4010/0x812510 シミュ・S タグ付）
- `tmp/re/v260_recctor_register_disasm.txt`（0x4BA2D0/0x4BA780/0x7B4010/0x7B2A00/0x4B9C40）
- `tmp/re/v260_flush_802ae0_disasm.txt`（0x812510/0x802AE0）
- スクリプト `%TEMP%\opencode\v260_1.py`〜`v260_26.py`（生 out 同所）

### v255/v258 への影響（実装セッション向け訂正要点）
1. v255 §4「rot の与え方 [推定]=Z 軸回転+一様スケール」→ **[確定] スケールのみ・rot はエンジン描画未消費**（§4）
2. v258 §4-3「ctx=task+0x2C・vft 0xA7E38C 0xDC NiNode 派生」→ **task+0x2C は要求レコード（0x2C、非 NiNode）。0xA7E38C=RTTI NiNode 本体**（§0-1/0-4）
3. v258 件4「行列組立 [未確定]（V-258-4a）」→ **解決**（0x4BCEF0→0x4BAA80→0x7B4010/0x812510/0x802AE0・§1-§2）
4. 0xB2C33C/0x7B3BE0（v258 件2 の land LOD 基盤）は **land と object distant で共用**（§2-4/§3-2）
