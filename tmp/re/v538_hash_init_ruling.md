# v538 — formID バケット配列の確保元確定（**CRT 静的 init `FUN_009DBF00`＝`alloc(131213*4)`→`[0xB06144]=ptr`＋`[0xB0613C]=0xA3BDD8` vtable 差替え（hash/compare は同一実装維持）**）・**v537 訂正: `0x597498` の削除先は `DataHandler+0x54`（topic リストではない＝topic リスト remove site は [確定負]・登録専用）**・ステージング clear/get ヘルパ（`FUN_0052ED10`/`FUN_0052ED50`）確定・**`mov ecx,[b33a98] + add ecx,imm` 包装サイトのリストオフセット表（`+0x6C`=43 push sites＝ファイルリスト最有力／`+0x7C`（topic）はラッパ `0x447530` のみ）**（V-538-1..3 完遂・旧残点 V-406-3 継続第68弾）

- 採番: **v538**（旧残点群消化・probe prefix **v538_1..5 全5本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-538-1..3（v537 残点）** — 1=`DAT_00b06144` バケット配列の確保/代入元／2=`FUN_00597340` の文脈＋`FUN_0052ED10/52ED50` 本体／3=`0x448F40`/`0x449040`/`0x4491D0` 等 DataHandler API 面。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実データ（静的 .data）のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-538-1: バケット配列の確保/代入元** | **[確定]（命令）CRT 静的初期化関数 `FUN_009DBF00`** | `alloc(0x2008D*4)`（overflow ガード付き mul）→ **`mov dword ptr [0xb06144], eax`**（v538_5 線形スイープ監査: `.text` の 0xB06144 含む 16 命令のうち **store は `0x9DBF2C` の 1 本のみ**、残りは `mov reg,[0xb06144]`×14＋`cmp ecx,0xb06144`×1 の読み）→ `FUN_00981630(ptr, 0, count*4)`＝memset → **`mov dword ptr [0xb0613C], 0xA3BDD8`**（ポリシー singleton の vtable を差替え）→ `FUN_00981FB4(thunk 0xA18520)` 登録。呼び出し元は無く（E8 0 件）、**アドレス `0xA28400` が CRT initializer テーブル（隣接 0x9D7E40../0x9DBF50/0x9E06B0..）に並ぶ静的 init**と判明。 |
| **V-538-1: vtable 差替えの実害** | **[確定]（命令）** | 静的 `.data` の `0xB0613C = 0xA3BC94` に対し、init 後は **`0xA3BDD8`** が有効。**両 vtable を実読すると `+0x04 hash=0x6A9060`・`+0x08 compare=0x763E80`・`+0x0C=0x67F130`・`+0x10=0x68F970` は同一**で、差は `+0x00`（destructor: 0x46B030→**0x46C220**（scalar deleting dtor: `FUN_0046C0F0()`＋delete フラグで `FUN_00401F20`））と `+0x14/+0x18/+0x1C`。**v537 のハッシュ/比較の確定は init 後も有効**。バケット数 `0xB06140=0x2008D`（131213）は不変。 |
| **V-538-2: topic リストからの削除** | **[確定・訂正] v537 の「0x59749B が topic 削除」は誤り（`add ecx,0x54` だった）→ topic リスト（+0x7c）の remove site 不在 [部分確定]** | 生バイト `0x597492: mov ecx,[0xb33a98] / 0x597498: add ecx,0x54 / 0x59749B: call FUN_0065C620`。走査（v538_4: `mov ecx,[global]` の 16B 以内に `81 C1 7C..`/`83 C1 7C` ＝**0 件**、`disp32=0xB33B14`（head アドレス直参照）＝**0 件**、v536 の head 全命令 walk でも store 0 件）。`+0x7c` を this にする remove は既知パターン全てで不在。→ **topic リストは `FUN_00447530`（+0x7c push ラッパ、caller=FE90 のみ）で増える専用**とみなせる（レジスタ経由の計算 lea 等の未走査形式が残っているため [部分確定]）。v537 の「唯一の削除 site」記述を訂正。 |
| **V-538-2: `FUN_00597340`（+0x54 リスト処理）の文脈** | **[確定]（命令＋docs L304372）** | DataHandler `+0x54` の {item,next} チェーンを歩き、各 item を `FUN_0051BEC0()`（有効判定）→ `FUN_005971E0(name=item+0x1c, item, idx, (item!=param_1[0xf])-1 & 2)` でスコア集計。`param_2` 真なら `FUN_00588BD0(0xFA8)`＋`vtable[+0xC](float(score))`。**親 `FUN_005973F0`** が `*(0xB333C4+0x654) = param_1+0x3c`（GameBH への参照設定）→ 文字列解放群 → **`FUN_0065C620(item->+0x40)`（+0x54 から当該 item を除去）** → `FUN_00416650` → `597340(1)`（再集計）。caller `0x597521`。→ **「（プラグイン/ファイルの）referrers リストの除去＋反映度スコア再計算」**の系（topic ではない）。 |
| **V-538-2: `FUN_0052ED10`/`FUN_0052ED50`（ステージング補助）** | **[確定]（命令＋docs L225018/L225045）** | 静的組 `0xB110F4`（要素数×8 バケット）/`0xB111B8`（entry ptr 配列）のアクセサ。**ED10＝全バケットの entry（12B stride）を 0 クリア**（**呼び元は `0x44963C`＝`FUN_004492E0`（DataHandler+0xCD4=1 のリセット経路、44C4D0 再ハッシュクラスタの頭）のみ**＝**ステージングの解放パス**）。**ED50＝`get(bucket, idx) = ptr[bucket] + idx*0xc`（範囲チェック付き）**。→ FE90（v537）が読む組の生成側は依然別（`0xB111B8` への書込み元は未追跡＝残点に引き継ぎ）。 |
| **V-538-3: DataHandler 公開 thiscall API 面（`mov ecx,[b33a98]; add ecx,imm` 包装サイトの全量表）** | **[確定]（機械走査・v538_3）** | `.text` の当該パターン全サイトを (disp→call 先) で集計: **`+0x6C → FUN_00446CB0 push × 43**（DataHandler 最大のリスト＝**ファイルリスト最有力**）／`+0xAC → push ×3`／`+0x2C → push ×2`／`+0xC → FUN_0067B1E0 append ×2`／**`+0x54 → FUN_00416650 ×1 ＋ FUN_0065C620 remove ×1`**（referrers リスト）／`+0x68 → FUN_00468260 ×1`。**`+0x7C`（topic）は直接 thiscall 形式が 0 件で、ラッパ `FUN_00447530`（`add ecx,0x7c; jmp 446CB0`）のみ**（再確認）。`0x447160`（60 sites）は全て head `0x4FD0A0` のジャンプテーブル経由。 |
| **V-538-3: `FUN_00449040`/`FUN_004491D0`** | **[確定]（命令＋docs L56106/L56248）** | 449040＝**DataHandler `+0xB4` の {item,next} チェーンを歩き `item+0x30 == key` の n 番目を返す**（＝**ファイルの mod index 検索**、plugin lookup）。4491D0＝**`this[0x337]`（+0xCDc）の解放**（`FUN_004894D0()` 後 `FUN_00401F20`）＋再アロケータ初期化系。448F40＝**`+0xCDc` が無ければ `FUN_00401F00(0x10)`＋`FUN_00485D50(0)` で生成、有れば `FUN_0048F180` リセット → `FUN_0041E6F0`/`FUN_0048E9A0`（HUD?）/`FUN_00420680`/`FUN_006ECC80`/`FUN_004CB2B0` 呼び**（＝コンソール/HUD バッファ初期化ラッパで、DataHandler 公開面の一つ。呼び元 `0x44D395`/`0x59A2EC`/`0x59AEDB`/`0x59B298`）。 |

## §2 証拠

### §2.1 `FUN_009DBF00`（静的 init）生バイト
```
9DBF00: xor ecx,ecx / mov eax,0x2008d / mov edx,4 / mul edx / seto cl / neg ecx / or ecx,eax
9DBF15: push ecx / call 0x401f00                 ; alloc(131213*4)
9DBF1B: mov ecx,[0xb06140] / lea edx,[ecx*4]
9DBF28: push edx / push 0 / push eax
9DBF2C: mov dword ptr [0xb06144], eax             ; ★ disp32 store は .text 全体でこの 1 箇所（v538_5 線形スイープ監査で確定）
9DBF31: call 0x981630                             ; memset(ptr,0,count*4)
9DBF36: push 0xa18520                             ; thunk: mov ecx,0xb0613c; jmp 0x46c0f0
9DBF3B: mov dword ptr [0xb0613c], 0xa3bdd8        ; ★ vtable 差替え
9DBF45: call 0x981fb4                             ; atexit 系登録
9DBF4D: ret
参照: E8 call 0 件 / .rdata 0xA28400 単一出現（隣接 0x9D7E40,0x9D7E70,0x9D7E90,0x9DAC50,[0x9DBF00],0x9DBF50,0x9DBF80,0x9E06B0,0x9E06E0 ＝ CRT initializer 配列）
0xB06144 の全 16 サイト = store 1（9DBF2C）＋ read 15（0x448D9E/449AB3/449B0E/0x44C4ED〜44CC7B の mov reg,[..] 14 本 ＋ 0x46AF9A cmp ecx,imm）。0xB06140 = read 4（449A98/449B01/44CFA1/9DBF1B）のみ・store 0（＝カウント 131213 は .data 静的値）。
姉妹: 0x9DBF50 は alloc(32*4)→mov [0xb06150],eax ＋ thunk 0xA18500
```

### §2.2 静的 `.data`（0xB06134..0xB06154）
```
0xB06134=0x00A3BA68（'TOFT' 名前? ptr）0xB06138=0（'TOFT' f1 領域）
0xB0613C=0x00A3BC94（policy vtable・init 前に静的）0xB06140=0x0002008D（131213）
0xB06144=0（バケット配列 ptr・init が埋める）0xB06148=0 0xB0614C=0x00A37AAC 0xB06150=0 0xB06154=0x20
```

### §2.3 v537 訂正（0x59749x 実バイト）
```
59748E: mov ecx,[esi+0x40] / push ecx
597492: mov ecx,[0xb33a98] / add ecx,0x54 / call 0x65c620
```
→ **`+0x54`**（referrers）であって `+0x7c`（topic）ではない。

### §2.4 v538_3 の集計表（`mov ecx,[b33a98]`+`add ecx,imm`+call）
```
disp 0x1  -> call —        1   | disp 0xC  -> 67B1E0  2   | disp 0x2C -> 446CB0  2
disp 0x44 -> call —        1   | disp 0x54 -> 416650  1 / 65C620 1               | disp 0x68 -> 468260 1
disp 0x6C -> 446CB0      43   | disp 0x7C -> （直接 0 件・ラッパ 447530 のみ）      | disp 0xAC -> 446CB0  3
```

## §3 判定の内訳

- **V-538-1 完遂**: バケット配列は **CRT 静的 init `0x9DBF00`** が確保・memset し、`0xB06144` への disp32 直書き（v538_4 監査: `.text` の 0xB06144 含む 16 命令のうち store は **`0x9DBF2C` の 1 本のみ**、残りは or/adc/sbb/cmp の被せ読み）。同時に policy vtable を `0xA3BC94→0xA3BDD8` へ差替えるが hash/compare virtual は同一で、v537 の確定は不変。
- **V-538-2 完遂＋訂正**: **topic リストは「増えるだけ」**（remove site 既知パターン全て不在 [部分確定]、`FUN_00597340` は別リスト `+0x54` の referrers 処理）。ED10/ED50 はステージング組の clear/get と確定（clear の呼び元はリセット経路 `0x44963C` のみ＝**FE90 生成→（データハンドラ初期化で）clear** の輪郭化。entry 配列自体の生成元は未追跡のまま残点へ）。
- **V-538-3 完遂**: `+0x6C` push 43 箇所＝DataHandler の最major リスト（ファイルリスト最有力）、`449040`＝mod index によるファイル検索で「+0xB4＝ファイルチェーン」と相互に整合。

## §4 残点（V-539-1..3）

1. `0xB111B8`（FE90 の entry ptr 配列）への**書込み元**（TESTopicInfo コンパイル側?）と `0xB110F4` の count 設定元【中】
2. `+0x54` referrers リストと `+0x6C` ファイルリストの関係（`416650`＝?、`0x65C620` のこの 2 リストへの全適用サイト整理）【低】
3. `FUN_00597521`（5973F0/597340 の呼び元クラスタ）＝リフレレータ処理の入口（コンソール `0x448F40` と同じ `0x59A2EC/0x59AEDB/0x59B298` 群）の identity【低】

## §5 後続への申し送り

- **移植側の formID ハッシュ**: 起動時（CRT ctor 相当）に `alloc(131213*4)`＋ゼロクリア＋`policy.vtable` 差替え（destructor だけ本物へ）を行う流れそのもの。**バケット数 131213 は固定**（`0xB06140` を読む virtual 経由で使う実装なら変更可）。
- **topic リスト（+0x7c）は追加専用**とみなせる（exe に remove 不在）。移植側で unload 時に消したくなるが、元 exe は消さない——`FUN_0052F690` の検索が「居れば一致」で動作する前提。
- **DataHandler のオフセット表が +=: `+0x54` referrers / `+0x6C` 最大リスト（ファイル） / `+0x7C` topic / `+0xAC` 小リスト / `+0xB4` ファイルチェーン（449040 検索） / `+0x8C4` 現ファイル / `+0xCD4` isCompiling / `+0xCDc` コンソール系バッファ**（移植側は構造体定義の一次資料にできる）。
- CRT initializer 配列（`0xA28400` 近辺）を歩くと、**静的単一tons の初期化一覧**が機械列挙できる（`0x9DBF50→0xB06150` のような姉妹 init も同じ窓に出る）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
