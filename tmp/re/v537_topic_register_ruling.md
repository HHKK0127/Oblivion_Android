# v537 — topic リスト登録/削除の call site 決着（**`FUN_00447530`＝「DataHandler+0x7c への push ラッパ」で唯一の登録元＝`FUN_0052FE90`（TESTopic 組立最終段）**・削除は `FUN_00597340` の 1 箇所）・formID ハッシュのポリシー object 確定（**静的 `0xB0613C`＝vtable `0xA3BC94`、hash=`id % [0xB06140](=0x2008D)`、compare=`==`**）・TESTopic `+0x28` の {arr,count} 操作関数確定（`FUN_00446C50` 要素設定/`FUN_00452910` リサイズ）（V-537-1..3 完遂・旧残点 V-406-3 継続第67弾）

- 採番: **v537**（旧残点群消化・probe prefix **v537_1..4 全4本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-537-1..3（v536 残点）** — 1=topic リスト（`DAT_00b33a98+0x7c`）登録 call site 探索／2=`PTR_PTR_00b0613c` ポリシー object 実体・`DAT_00b06144` バケット配列の生成者／3=TESTopic `+0x28` の {arr,count} への要素追加/削除命令。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・実文字列のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-537-1: topic リストへの登録** | **[確定]（生バイト・完全決着）** | ラッパ **`FUN_00447530`（12 バイト）**: `arg==0 → ret 4`／else `[esp+4]=arg; ecx += 0x7c; jmp FUN_00446CB0` ＝ **this（=DataHandler）の +0x7c head へ v530 確定の単方向 push を行う専用 wrapper**。**唯一の呼び出し元 = `0x52FFBA`（`FUN_0052FE90` 内）**: `mov ecx,[0xb33a98]; push eax; call 0x447530`（eax=その直前に組まれた TESTopic）。→ **登録 site 特定完了（v536 [確定負] との整合: disp32 store でなく jmp 経由の push だった）**。 |
| **V-537-1: `FUN_0052FE90` の役割** | **[確定]（命令）** | **TESTopic 組立の最終段（ロード／コンパイル後）**。`0xB110F4`（件数）×`0xB111B8`（entry 配列）のステージング組を歩き、各 entry から `vtable[+0xD8]` 呼び・`FUN_004028D0`（文字列確保）・`FUN_0046C300`（form↔ファイル紐付け、v535）を経て **TESTopic を `FUN_00447530` で topic リストへ登録**。同一グローバル組を使う関数は `FUN_0052ED10`/`FUN_0052ED50`/`FUN_0052FE90` の 3 つのみ（＝組立系の一連）。呼び出し元は `0x44C5A2`（`[esp+0x14]` を 4 ずつ 0x50 まで進めるループの直後＝v536 §のバケット関連処理のクラスタ内）。 |
| **V-537-1: topic リストからの削除** | **[確定]（命令）1 箇所のみ** | `0x59749B`（head `FUN_00597340`）: `mov ecx,[0xb33a98]` → `call FUN_0065C620`（v529 確定のセンチネル払い出し削除）。→ 実行時 topic リストは **FE90 が足し、597340 が引く**最小構成。 |
| **V-537-2: `0xB0613C` ポリシー object** | **[確定]（命令）＝静的インスタンス `{vtable 0xA3BC94, bucketCount@+4}`** | `0xB0613C = 0xA3BC94`（vtable）・**`0xB06140 = 0x2008D`（=131213＝バケット数）**。**hash virtual `vt[+4]=0x6A9060`: `eax=id; div dword [ecx+4]; return edx`＝`id % 131213`**。**compare virtual `vt[+8]=0x763E80`: `return a == b`（key 直接一致）**。→ **v536 の採番器 `FUN_00448D40` が呼ぶ 2 virtual の意味が完全に確定**（衝突＝同一 id のノード在否のみ）。 |
| **V-537-2: `DAT_00b06144`（バケット配列ポインタ）** | **[確定負]（命令）disp32 による書込みサイトは .text に 0 件**（=read のみ 13 箇所: 448DA0/449AB5/449B0E/44C4ED/…/46AF9A）。ディスク上値 0。**実行時にポインタ計算経由でのみ代入される**（誰が握るか＝残点 V-538-1。ただし**この表を走査する関数群は 0x44C4D0〜0x44CCC0 の「コンパイル後再ハッシュパス」クラスタと `FUN_00448D40`/`FUN_004492E0` に限定**される）。 |
| **V-537-3: TESTopic `+0x28` の構造と操作** | **[確定]（命令＋docs）** | `+0x28` は**ノードの単方向リスト**（walk: `piVar=(this+0x28); piVar=(int*)piVar[1]`）。各ノード（0x24B、`FUN_0052FC40` が生成）は **`{topic formID?@0, vtable/scalar dtor ptr@4, …, entry 配列 arr@+8, entry 数 count@+0x10, …, next?@+0x1C, flag byte@+0x20}`**。**entry = TESTopicInfo form**（`+0xC`=formID＝`F5D0` の検索キー、**`+0x3d`=priority バイト**＝`F330` ソートのキー）。操作関数: **`FUN_00446C50(nodeStruct+8?, idx, &val)`＝要素設定**（idx>=count なら count=idx+1、非空数を +0x10 で加算、`arr[idx]=val`。docs L54200）。**`FUN_00452910(&vec, newSize)`＝リサイズ**（容量 +0x8・使用数 +0xC・非空 +0x10、縮小時は後方クリア＋非空数減、`FUN_00401F00(newSize*4)` で新確保＋コピー。docs L63371）。FC40 は topic+0x28 を歩き**該当ノード/槽找不到時 0x24 ノードを新規生成して `FUN_0067B1E0`（末尾 append・v536）で接続**。 |
| **V-537-3: entry の並び** | **[確定]** | `FUN_0052F330`＝ノード配列の **priority（entry+0x3d byte）昇順バブルソート**（v536）。`FUN_0052F5D0`/`FUN_0052F520`/`FUN_0052F570`＝配列走査検索（formID 一致／子一致／topic 自身返却）。`FUN_0052F650`＝`52EFA0(param)` 後、返り +4 配列の各要素 +0x20 に index を書き直す（＝**INFO の「並び番号」再採番**、docs L225661）。 |

## §2 証拠

### §2.1 登録ラッパと呼び出し元（生バイト）
```
447530: mov eax,[esp+4] / test eax,eax / je ret4
        mov [esp+4],eax / add ecx,0x7c / jmp 0x446cb0      ; ★ push(DataHandler+0x7c, topic)
52FFB0: mov edx,[ebp] / mov eax,[edx+edi]（組立済み TESTopic）
52FFB3: mov ecx,[0xb33a98] / push eax / call 0x447530      ; ★ 唯一の登録 site
597492: mov ecx,[0xb33a98] / ... / call 0x65c620           ; ★ 唯一の削除 site（head 0x597340）
```

### §2.2 ハッシュポリシー（生バイト）
```
6A9060 (vt+4): mov eax,[esp+4]; xor edx,edx; div [ecx+4]; mov eax,edx; ret 4    ; id % 0x2008D
763E80 (vt+8): mov ecx,[esp+4]; xor eax,eax; cmp ecx,[esp+8]; sete al; ret 8    ; key == key
0xB0613C = 00A3BC94（vtable） 0xB06140 = 0002008D（=131213）
DAT_00b06144 への disp32 store: .text 全走査で 0 件（read 13 箇所のみ）
```

### §2.3 `FUN_00446C50` / `FUN_00452910`（docs L54200/L63371 要約）
```
446C50(vec, idx, &v): if idx < count(+0xC): 既存非空なら +0x10-- / v!=0 かつ既存 0 なら +0x10++・arr[idx]=v
                      else: count=idx+1、v!=0 なら +0x10++
                      arr[idx]=v（詰めの書込み）
452910(vec, n): if n != cap(+0x8): n<count(+0xC) なら [n..count) を 0 クリア＆非空数減、count=n
                newbuf=FUN_00401F00(n*4); 旧 [0..count) をコピー; 旧 buf free（後続）
```

### §2.4 組立系グローバル
```
0xB110F4（件数 dword）/ 0xB111B8（entry ptr 配列）を触る head = 0x52ED10, 0x52ED50, 0x52FE90 のみ
FE90 ループ: while (i<count) { e=[0xB111B8+i*4]; ... 46C300(紐付け) ... e->vtable[+0xD8](arg) ...
             4028D0(文字列) ... mov ecx,[b33a98]; push topic; call 447530 }
```

## §3 判定の内訳

- **V-537-1 完遂**: 登録＝`FUN_00447530`（+0x7c push ラッパ、呼び元は `FUN_0052FE90` の 1 箇所）／削除＝`FUN_00597340` の 1 箇所（`65C620`）。v535/536 の「head 直書きなし [確定負]」は保持したまま、**jmp 経由 push という経路**で決着。
- **V-537-2 完遂（生成者のみ残し）**: ポリシー object は静的 `0xB0613C`（mod ハッシュ＋キー一致比較、バケット数 131213）。バケット配列 `0xB06144` への disp32 書込みは不在 [確定負] を確認（実行時ポインタ計算で初期化）。走査側の全 callers（再ハッシュクラスタ 0x44C4D0〜0x44CCC0 他）を列挙済み。
- **V-537-3 完遂**: ノード{arr@8,count@0x10}の操作は `446C50`（set）と `452910`（resize）と確定。priority=entry+0x3d・index 再採番（+0x20 short）＝`52F650` も追加確定。

## §4 残点（V-538-1..3）

1. `DAT_00b06144` バケット配列の**確保/代入元**（disp32 書込み不在のため、`0xB06140`（=131213）を乗算に使う確保呼び出しや `0xB0613C` 参照クラスタからのポインタ計算を洗う）【中】
2. `FUN_00597340` の文脈（topic リストから何を条件に外すか＝プラグイン unload? セーブ?）＋ `0x52ED10/0x52ED50`（FE90 とグローバル共有の組立系 2 関数）の本体【中】
3. `FUN_00448F40`/`FUN_00449040`/`FUN_004491D0`（`mov ecx,[b33a98]; call` 経由の隣接 API・v537_2 の callee 表に出た 0x448Fxx 群）の identity＝DataHandler 公開 API 面【低】

## §5 後続への申し送り

- **移植側の topic リスト**: 生成期に `TESTopic` 組立（FE90 相当）が 1 回だけ push、破棄時（597340 相当）に remove。**登録 API は「DataHandler を this に +0x7c へ push」だけ**なので、list-of-TESTopic は「ロード済み topic 全集」セマンティクス（v533 の `FUN_0052F690` 所有者検索がこの list を歩くのと整合）。
- **formID ハッシュの移植**: `id % 131213`＋キー一致のみの素朴な表。衝突解決はチェーン連結（ノード `*puVar3` next、`node[1]`=key/form）。バケット数を変更すると元の ID 採番列がずれる点に注意（移植側は 131213 固定で再現を）。
- **TESTopic+0x28 ノードの {arr,count} は「entry 配列」で、`+0x10` は非空数、`+0xC` は使用数、`+0x8` は容量**（446C50/452910 の増減規則をそのまま移植）。entry の **priority=+0x3d・index 再採番先=+0x20（short）**。
- 組立ステージング（0xB110F4/0xB111B8）は**コンパイル〜FE90 の間だけ有効な一時配列**。移植側はロード結果の topic 中間表現として保持すれば足りる。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
