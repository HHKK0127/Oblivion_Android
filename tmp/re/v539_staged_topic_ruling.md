# v539 — ステージングトピックマトリクスの正体確定（**`0xB110F4`/`0xB111B8` は .data の静的初期化値そのもの＝.text に書き込み元は存在しない [確定負]。7 グループ count={1,6,16,39,4,14,5(,0)}・entry 配列 ptrs→`0xB10DA0..0xB110C4`（12B スロット `{topic ptr@0, formID@4, name ptr@8}`）で、`FUN_0052FE90` は「EXE に静的コンパイルされた topic 定義の起動時実体化＋topic リスト登録＋name→topic map 登録」ループと決着**）・**`FUN_00416650`＝比較関数付き順序リスト挿入 [確定]**・**`FUN_00447160`＝静的 2 番目の map `0xB06164` の key 存在チェック [確定]**・console 系クラスタ（`5974E0`/`599EE0`/`59AEDB`/`59B298`）は [部分確定]（V-539-1..3 完遂・旧残点 V-406-3 継続第69弾）

- 採番: **v539**（旧残点群消化・probe prefix **v539_1..3 全3本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-539-1..3（v538 残点）** — 1=`0xB111B8` entry 配列／`0xB110F4` count の書込み元／2=`416650`/`447160`（`0x4FD0A0` テーブル）identity＋`+0x6C`/`+0x54` 適用サイト整理／3=`597521/59A2EC/59AEDB/59B298` クラスタ入口 identity。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・静的 .data 実測のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-539-1: 書込み元** | **[確定負]（機械走査）書き込むコードは存在しない。マトリクスは静的 .data** | `.text` の disp32 ∈ `[0xB11000, 0xB11400)` 全サイトを covering-sweep 監査（v539_3）: 触る head は **`FUN_0052ED10`（clear）/`FUN_0052ED50`（get）/`FUN_0052FE90`（build ループ）の 3 つのみ、store 0 件**。静的値実測: **count 配列 `0xB110F4..0xB11110` = {1, 6, 0x10, 0x27, 4, 0xE, 5, 0}**、**ptr 配列 `0xB111B8..0xB111D4` = {0xB10DA0, 0xB10DE8, 0xB10E30, 0xB10EF0, 0xB110C4, 0xB11110, 0xB10DAC, 0xAA3E80}**（全て .data）。→ **85 件の topic 定義（7 グループ）が EXE イメージに静的コンパイルされている**。「書込み元探索」という問い自体が誤りで、**FE90＝静的 topic テーブルの起動時実体化**と決着。 |
| **V-539-1: FE90 ループの意味（再確定）** | **[確定]（命令＋docs L226218）** | `for g in 0..6: for i in 0..count[g]: slot = ptrs[g] + i*0xc`（**12B スロット**）: **slot[0]==0 のとき** ①`FUN_00401F00(0x3C)`＝60B 確保 ②`FUN_0046C460()`（TESForm ctor・v535）→ `*puVar3 = vtable TESTopic 0xA5555C`／`[6]=TESTopic 補助 vtable 0xA55548` ③**`byte[+0x24]=g`（グループ index）**・**`byte[+1]=0x39`（type='9' DIAL）** ④slot[0]=topic ⑤`FUN_0046C300(slot[4]=formID, 1)`（formID→form 紐付け）⑥`slot->vtable[+0xD8](slot[8]=name)`（TESopic の名前設定系）⑦`FUN_004028D0(name,0)`（EditorID→form レジストリ登録）⑧**`FUN_00447530(topic)`（topic リスト +0x7c へ push＝v537 の登録ラッパ）** ⑨**`FUN_00412D30(name, topic)`（name→topic map 挿入、`0xB06164` クラスタ）**。→ **FE90 が topic リストの唯一の供給元であることが閉じた**（v537/538 と連結）。 |
| **V-539-2: `FUN_00416650`** | **[確定]（命令＋docs L14890）＝比較関数付き順序リスト挿入** | `__thiscall(list, item, cmp)`: `item==0` なら return。list を歩き ①空きスロットなら直セット ②`cmp(item, *cur) < 1`（挿入位置発見）なら **head 位置は `FUN_00446CB0`（push）／中間は 8B セル `{item, prev->next}` 新設で前挿入** ③末尾通過時は次へ。→ **`FUN_00597340` の親 `FUN_005973F0` が `65C620`（除去）直後に呼ぶ「並び順を保った再挿入」**（v538 の referrers 処理と連結）。 |
| **V-539-2: `FUN_00447160`** | **[確定]（命令＋docs）** | `bool find(key=*param)`: `FUN_0055E000(DAT_00b06164 の map インスタンス, key, &out)` のラッパ。→ **静的 map インスタンスは `0xB0613C`（formID ハッシュ・v537/538）だけでなく `0xB06164` に第 2 実例**（key→form 系、`55E000`＝v528 map 検索）。 |
| **V-539-2: `0x447160` の 60 sites ＝ `0x4FD0A0`** | **[部分確定]（命令）** | `FUN_004FD0A0`（docs L196553）: `this+0x44` の {name@0,...} チェーンを `__stricmp` で歩き、**一致时 `arg->+0x200=index`／`+0x210=entry+8`／`param_3!=0` なら `entry+0xC = arg->+0x20C` の逆写し**、不一致时 `0x10` ノード新設。head 解決が 60 sites 全て `0x4FD0A0` に収束するのは **0x4FD0xx が巨大なジャンプテーブル本体（コンソール系大関数）だから** [命令確認: find_head 収束]。identity は [部分確定]（名前キーのレーシングテーブル＝**race/BSFS 名前表**）。 |
| **V-539-3: `0x5974E0`（5973F0/597340 の caller）** | **[部分確定]（命令）** | `FUN_00589B70(0x406)`（=1030 の ID 検索）→ `FUN_005898F0(0, ..., 0xB0A018, 0xB13E78)` ＋ `FUN_009832E6`（型照合）→ **`FUN_00578D70() == 1` ゲート** → `ecx=form` → `5973F0`。0xB0A018/0xB13E78 は静的文字列バフ。**メッセージ／ブック系の UI ハンダ内で referrers 再集計を呼ぶ**位置 [部分確定]（クラスタ全体 identity は未確定）。 |
| **V-539-3: `0x599EE0`・`0x59AEDB/0x59B298`** | **[部分確定]（命令）** | `599EE0`（`448F40` caller の 59A2EC を内包）: `form->vtable[+0x170]()` の型 byte が **0x17(23=ACTI)/0x23(35=NPC_)/0x24(36=LVLF) のどれか**で `vtable[+0x198]` 分岐→… **アクタ／クリーチャ編集系のゲート**。`59AEDB/59B298` は head 解決不能＝**ジャンプテーブル内サイト**で `448F40`（コンソールバッファ初期化・v538）→`5992C0`→`jmp 0x59A574` の流れ＝**コンソールコマンド実装群**の一部。[部分確定]。 |

## §2 証拠

### §2.1 マトリクス窓走査（v539_3・covering-sweep・窓 0xB11000..0xB11400）
```
heads: 52ED10（cmp/mov [eax+0xB110F4]×2, [eax+0xB111B8]）
       52ED50（cmp [ecx*4+0xB110F4], mov [ecx*4+0xB111B8]）
       52FE90（cmp [eax*4+0xB110F4], lea ecx,[eax*4+0xB110F4], lea ebp,[eax*4+0xB111B8]）
store: 0 件（窓内の disp32 を含む命令は全て上記 read/cmp/lea のみ）
静的 .data 実測:
  0xB110F4: 1 6 10 27 4 E 5 0            （7 グループ count、計 85）
  0xB111B8: B10DA0 B10DE8 B10E30 B10EF0 B110C4 B11110 B10DAC AA3E80（entry 配列 ptr、最終は map?）
```

### §2.2 `FUN_0052FE90` docs L226240..226280 要点（v537 の窓と統合）
```c
for (g = 0; g < 7; g++) {
  for (i = 0, off = 0; i < count[g]; i++, off += 0xc) {
    if (ptrs[g][off/4 /*slot[0]*/] == 0) {
      t = FUN_00401F00(0x3c);
      FUN_0046c460(t);                    // TESForm ctor（v535）
      t->vtable = &TESTopic_vt 0xA5555C; t[6] = &0xA55548;
      *(byte*)(t+0x24) = g;               // グループ index
      *(byte*)(t+1) = 0x39;               // type '9'（DIAL）
      slot[0] = t;
      FUN_0046c300(slot[4], 1);           // formID 紐付け（slot+4=静的 formID）
      slot0->vtable[+0xD8](slot[8]);      // name（slot+8=静的 EditorID ptr）
      FUN_004028d0(slot[8], 0);           // EditorID→form レジストリ
      FUN_00447530(slot[0]);              // ★ topic リスト +0x7c へ push（v537）
      FUN_00412D30(slot[8], slot[0]);     // name→topic map 挿入
    }
  }
}
```

### §2.3 `FUN_00416650`（順序挿入）docs L14890 要点
```
if (item) { prev=null; node=list;
  while (node) { if (!*node) { *node=item; done; }
    else if (cmp(item,*node) < 1) {
      if (!prev) FUN_00446CB0(item);                  // head → push
      else { cell=alloc(8); cell->item=item; prev->next=cell; cell->next=node; }  // 前挿入
      done;
    }
    prev=node; node=node[1]; } }
```

### §2.4 `FUN_00447160`
```
447160: push ecx（out）; eax=[esp+8]（&key slot）; if (eax && *eax[0字符]) { lea ecx,[esp]; push ecx; push eax; mov ecx,0xb06164; call 0x55e000; if(al) out=[esp] }  ; key 存在チェック
```

## §3 判定の内訳

- **V-539-1 完遂**: 「書込み元」は**不在**（[確定負]）。マトリクスは静的 .data（EXE 埋め込みの topic 定義 85 件・7 グループ）。FE90 が read して実体化＋topic リスト/map に流し込む唯一の供給元と閉じた（v537 ラッパ・v538 ED10 clear と全連結）。
- **V-539-2 完遂**: `416650`＝比較関数付き順序リスト挿入、`447160`＝`0xB06164`（第 2 静的 map）の key 存在チェック、`4FD0A0`＝名前キーのレーシングテーブル（race/BSFS 系・巨大ジャンプテーブル本体）と確定/部分確定。
- **V-539-3 部分完遂**: `5974E0`＝メッセージ／ブック系ハンダ、`599EE0`＝ACTI/NPC_/LVLF ゲートの編集系、`59AEDB/59B298`＝コンソールコマンド実装群のジャンプテーブル内、と位置づけ。完全 identity は未確定。

## §4 残点（V-540-1..3）

1. `FUN_0055E000`（`55E000`＝map 検索 API）と `0xB0613C`/`0xB06164`/`0xB06150` 静的 map インスタンス群の一覧化（バケット数・寿命・生成元）【中】
2. `0x5974E0` の `589B70(0x406)`／`5898F0`（メッセージ系）と `578D70` ゲートの identity（ブック／メッセージ UI クラスタ）【中】
3. `FUN_0052FE90` の呼び元 `0x44C5A2`（44C5D0 再ハッシュクラスタ内・CRT 系?）と init 順序（DataHandler 構築→FE90→447530）の確定【低】

## §5 後続への申し送り

- **移植側**: 静的 topic テーブル（7 グループ 85 エントリ・`{formID, EditorID}` 12B スロット）を**データとして移植**し、起動時 `TESTopic` 実体化→`46C300/4028D0/447530/412D30` 相当の 4 登録を通す。**「コンパイルで生成される」のではなく「EXE に埋まっている」**点が重要（ESM の DIAL/QUST/INFO とは無関係に、base game の対話トピックは exe 内蔵）。
- **順序リスト（416650）と push（446CB0）の使い分け**: referrers は**挿入（cmp 順）**、topic／ファイルリストは**push（LIFO）**。移植時に混同すると並びが変わる。
- **name→topic map は `0xB06164` 系列**、formID ハッシュは `0xB0613C` 系列。静的 map インスタンスが複数並ぶ前提で設計する。
- **type '9'(0x39)=DIAL 固定の TESTopic が 85 件**（v532 の '9'/':' 分岐の供給元がここに確定）。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
