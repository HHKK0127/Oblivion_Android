# v540 — 静的 map インスタンス群の一覧化（**`FUN_0055E000`＝汎用ハッシュ map find の本体確定**（hash=vt[+4]／bucket=this[2]／node{next,key,val}／val 取出し）・**第 2 静的 map `0xB06164`（count `0xB06148+0x20=0xFA1`=4001、buckets `0xB0616C` の store は CRT init `0x9DBF80` のみ、vtable→`0xA3BF1C`）＝コンソール key 検索（447160 が thiscall）**）・`0xB06150`＝32 dword 配列（CRT init `0x9DBF50` が確保、read: 449301/46B2E7/46C669/46C6A9/thunk A18500）（V-540-1 完遂）・**`FUN_00589B70`＝ブック本文 ptr 配列アクセサ（ID 0x3E9..0x41B・`DAT_00b13974` 基点・count `DAT_00b1397A`）**／`FUN_005898F0`＝type 0x389 一致の `param[0x11]` 取出し／`FUN_00578D70`＝**`582160(0,1)+0xB0` バイト read＋0xFF リセット（pending 消費フラグ）**（V-540-2 完遂）・**FE90 の呼び元確定＝`FUN_0044AC20`（docs L58249 に実call・呼び元は `FUN_0044F3D0` 内 `0x44F6CB`＝プラグイン起動ドライバ）で init 順序は「44F3D0→AC20（20スロット実体化ループ→FE90→バケット 0x35=WRLD 再ハッシュ）」と閉じた**（V-539-3 併せて完遂・旧残点 V-406-3 継続第70弾）

- 採番: **v540**（旧残点群消化・probe prefix **v540_1..4 全4本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-540-1..3（v539 残点）** — 1=`FUN_0055E000` と静的 map インスタンス群の一覧化／2=`589B70(0x406)`/`5898F0`/`578D70` の identity／3=FE90 の呼び元（`0x44C5A2` の所属）と init 順序。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・decompiled.c・静的 .data 実測のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-540-1: `FUN_0055E000`（map find API）** | **[確定]（命令＋docs L259922）** | `__thiscall(map, &keyslot)`: `uVar3 = vt[+4](key)`（バケット index）→ `node = this[2][bucket]`（**buckets ptr = map+8**）→ チェーン `*node` を歩き `vt[+8](key, node[1])` 一致で **`node[2]`（value）を `*keyslot` へ書込んで 1**、枯れたら `uVar3 & 0xffffff00` を返す（＝**0 以外＝見つからずで key 派生値**）。node 構造 `{next@0, key@4, val@8}`（v536 CRUD の `452570` が `vtable[+C](node,key,key)` で key=val 同値で埋めることと整合＝**キーと値が同一オブジェクトのセット map**）。 |
| **V-540-1: 静的 map インスタンス一覧** | **[確定]（生バイト＋静的 .data）** | **①`0xB0613C` formID map**: count `0xB06140=0x2008D`(131213)・buckets `0xB06144`（CRT init `0x9DBF00` が確保/ゼロ埋め＋vtable `0xA3BC94→0xA3BDD8`）＝v537/538 済み。**②`0xB06164` 第2 map**: count `0xB06168=0xFA1`(4001)・buckets `0xB0616C`・フラグ `0xB06174`（**store 唯一＝CRT init `FUN_0x9DBF80`**: alloc(0xFA1*4)→`mov [0xb0616c]`→memset→`mov byte[0xb06174],0`→**`mov [0xb06164],0xA3BF1C`**（vtable 差替え）→`981FB4` アトイグゼイト登録 thunk `0xA18530`）。**読み手＝`FUN_00447160`（`mov ecx,0xb06164; call 55E000`＝key 存在チェック）**。→ **この map は「コンソールコマンド名→？」検索系**（447160 は 0x4FD0A0 の大ジャンプテーブル経由で 60 箇所から呼ばれる＝**キー文字列の一意性検査**）。**③補助配列群**: `0xB06150`＝**32 dword 配列**（CRT init `0x9DBF50` が alloc(32*4)＋thunk `0xA18500`。read: `449301`（4492E0 DataHandler リセット）/`46B2E7`（46B2C0＝vtable+0x20 DeleteForm）/`46C669/46C6A9`（46C460 TESForm ctor 内）/`A18500` thunk）＝**form→index 変換の 32 エントリルックアップ**（ctor 側は「ロード済みファイルの登録配列」として読む）。`0xB06148`（=0xA37AAC・ptr）の disp32 参照は 0 件（thunk/間接のみ）。 |
| **V-540-2: ブック／メッセージ UI クラスタ** | **[確定]（命令＋docs L294976/L294790/L280928）** | **`FUN_00589B70(id)`＝「ブックの本文（書物テキスト）ポインタ配列アクセサ」**: `(id-0x3E9) < 0x33` かつ `< DAT_00b1397A`（count）なら **`[DAT_00b13974 - 0xFA4 + id*4]`** を返す（＝**message/book テーブル：ID 1001..1051 の 51 スロット・テキスト ptr 配列**）。`0x406`（1030）は該当。呼び元 `5974E0` の `9832E6` 型照合＋`578D70()==1` ゲート → `5973F0`（referrers 再集計）＝**「ブック ID 1030 の UI 更新パス」**。**`FUN_005898F0`**: 二重 `param[4]` 歩き（末端ノード取得）→ **`vt[+0xC]() == 0x389`（905）なら `param[0x11]` を返す**（メッセージ系オブジェクトのペイロード取出し）。**`FUN_00578D70`＝「消費式 pending フラグ」**: `582160(0,1)->+0xB0` を読み** 0xFF を書き戻して旧値を返す**（1 回だけ効くリトライ／無効化フラグ）。 |
| **V-540-3: FE90 の呼び元／init 順序** | **[確定]（docs＋E8 走査）** | `0x44C5A2` の所属関数＝**`FUN_0044AC20`**（0x44AC20..0x44CD80。docs L58249 に `FUN_0052fe90();` の直接呼び、E8 走査でも `0x44C5A2` が唯一）。`FUN_0044AC20` の呼び元＝**`0x44F6CB`（head `FUN_0044F3D0`＝v530 のプラグイン起動ドライバ）**のみ。AC20 内の順序: **①`alloc(0x3C)`×0x1C 進む 20 回ループ（60B＝TESTopic と同サイズの実体 19 個→`4B9AA0` ctor→`46C300(id,1)`→`vtable[+0xD8](name)`→`4B2B80(edi=this+?...)`→`vtable[+0x90](0)`）→②`call FUN_0052FE90`（静的 topic 85 件の実体化＋リスト登録・v539）→③`0xB0613C` の hash(vt[+4]) で**`0x35`(=53=WRLD) バケット**を_walk する再ハッシュ郡（`44C4ED..44CC7B` の [0xB06144] read 連発＝v536 の衝突検査と同 API）_。→ **DataHandler compile（44F3D0）→ AC20（ファイル単位の起動後処理：テーブル実体化→topic 実体化→WRLD バケット走査）の順**と閉じた。 |
| **おまけ [確定]** | **[確定]（静的表照合）** | `DAT_00b06084`＝表 index **53 の f1＝packed 'WRLD'**（エントリ名 'WRLD' を実測で再確認）。読み site: `FUN_004CE1A0`（0x4CE259・CELL builder exterior kind0）／`FUN_004E8BA0`（0x4E8BC1・Road builder）／**`FUN_004478B0`（0x447956/0x4479A1＝新出：DataHandler 側の WRLD 検索）**。 |

## §2 証拠

### §2.1 `FUN_0055E000`（docs L259922）
```c
uint __thiscall FUN_0055e000(int *map, undefined4 *keyslot) {
  uVar3 = map->vt[+4](keyslot);                 // bucket index (hash)
  puVar1 = map[2][uVar3];                        // buckets = map+8, chains
  for (;; puVar1 = *puVar1) {
    if (!puVar1) return uVar3 & 0xffffff00;      // 見つからず（非0）
    if (map->vt[+8](keyslot, puVar1[1])) break;  // key 一致
  }
  *keyslot = puVar1[2];                          // value 取出し
  return 1;
}
```

### §2.2 第 2 静的 map の CRT init（`0x9DBF80` 生バイト）
```
9DBF80: alloc(0xFA1*4) → mov [0xb0616c],eax ; memset(0,0xFA1*4)
9DBFB6: push 0xa18530（atexit 系 thunk）
9DBFBB: mov byte [0xb06174],0
9DBFC2: mov dword [0xb06164], 0xa3bf1c           ; ★ vtable 差替え（静的 A3BCB4 → A3BF1C）
9DBFCC: call 0x981fb4
姉妹（0x9DBF50）: alloc(32*4) → mov [0xb06150],eax ; thunk 0xA18500
```

### §2.3 `FUN_00589B70`（ブック本文アクセサ）docs L294976
```c
if ((id - 0x3e9) < 0x33 && (id - 0x3e9) < DAT_00b1397a)
    return *(int *)(DAT_00b13974 + param_1 * 4 - 0xfa4);
return 0;   // テーブル: DAT_00b13974（text ptr 配列）/ DAT_00b1397a（count）
```

### §2.4 `FUN_00578D70`（消費式 pending フラグ）docs L280928
```c
iVar2 = FUN_00582160(0,1); uVar1 = *(byte*)(iVar2 + 0xb0);
iVar2 = FUN_00582160(0,1); *(byte*)(iVar2 + 0xb0) = 0xff;
return uVar1;
```

### §2.5 FE90 呼び元確定（docs L58249・E8 走査）
```
L58249（FUN_0044ac20 内）: FUN_0052fe90();
E8 callers 0x44AC20: {0x44F6CB}（head FUN_0044F3D0・L60520 に対応）
AC20 ループ 0x44C4D0..0x44C5A2: [esp+0x14] を 4 ずつ 0x50 まで → 19 周 alloc(0x3C)+4B9AA0+46C300(+0xD8) →
  0x44C5A2 call 52FE90 → 0x44C5A7.. hash 0x35（=WRLD）バケット walk（[0xb06144] read 群＝v536 衝突 API）
```

## §3 判定の内訳

- **V-540-1 完遂**: map find（55E000）の本体と、**静的インスタンス 3 系統（0xB0613C/131213・0xB06164/4001・0xB06150/32）**の確保元（CRT init 0x9DBF00/0x9DBF80/0x9DBF50）を一覧化。各 buckets ptr への disp32 store は対応 init の 1 本ずつのみ（v538_5 の store 単独性監査の一般化）。
- **V-540-2 完遂**: 5974E0 のクラスタは**ブック本文テーブル（message book、ID 0x3E9..0x41B）経由の UI 更新パス**。5898F0/578D70 も命令で確定。
- **V-540-3 完遂**: FE90 の呼び元＝**AC20（プラグイン起動後処理）→親 44F3D0（v530 のロードドライバ）**。init 順序（テーブル実体化 19 → topic 実体化 → WRLD バケット再ハッシュ walk）まで閉じた（v539 の残点 3 を含む）。

## §4 残点（V-541-1..3）

1. `FUN_0044AC20` の 20 周ループが実体化する `alloc(0x3C)+4B9AA0` 群の**クラス正体**（60B・`vtable[+0xD8](name)`・`+0x90(0)` を持つ＝TESTopic と同サイズ別クラスか）＋スロット元 `param_1+0xc4?`（docs の [esp+0x14] 窓 0x6c）の初期化元【中】
2. `DAT_00b13974`（ブック本文配列）の**設定元**（誰がテキスト ptr を埋めるか＝BMSG/DOBM レコード?）と `582160(0,1)`（interface 取得）の identity【中】
3. `0xB06164` map（4001 バケット・第 2 静的）の**挿入側 call site**（55E000/55E060? の thiscall で ecx=0xB06164 を探す）＝キー文字列の登録元【低】

## §5 後続への申し送り

- **移植側のハッシュ map 実装は 1 クラス 2 インスタンスで足りる**: formID マップ（131213・mod ハッシュ）＋key 文字列マップ（4001・447160 検索）。node は `{next,key,val}` 12B。**返り値規約＝「見つかれば 1（＋ `*keyslot` に value を書換）／見つからなければ非 0 のゴミ値（`hash & 0xffffff00`）」**——したがって呼び出し側は `453450` 型の **bool 化（戻り==1 か否か）ではなく「見つかるときだけ `*keyslot` が更新される」性質**で判定している（v536 採番器は `55E000(...)==0 でなし＋453450 再検査`、`447160`/`52F690` は `!=0` を in として使うが、これは miss 時に `*keyslot` が 0 のまま＋戻りが bucket 派生であることに依存する箇所があるため、移植時は **「戻り 1 かつ *keyslot 更新」を正**として再実装すると安全）。
- **CRT init が 3 個の静的単一tons を確保している**（131213/4001/32）。移植側は起動シーケンス（CRT ctor 相当）で同じ順序・同じバケット数で確保を。
- **ブック本文テーブル（0xB13974・ID 1001..1051）**はゲーム内「書物」の text 引用。移植側で book を表示するパスはここを模す。
- **FE90／topic 実体化はプラグイン起動後処理（AC20）の一部**。DataHandler compile が走る前に topic を要求するコードは無い。
- docs/GT 反映は #47/#20 の一括承認待ち（本裁定も L1 report-only）。
