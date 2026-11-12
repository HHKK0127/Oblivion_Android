# V49 — 中央レジストリ `0xB35574` の型解明 と `FUN_00565CF0`（表C getter）の用途分類

対象: `Oblivion.exe` (Steam v1.2.0416) — `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
解析: capstone 5.0.7（バイナリ実機）+ Ghidraデコンパイル `docs\Oblivion_decompiled.c`（lowercased ラベル照合）
マッピング: PE を `struct` 自前解析。`.text` VA=0x401000 Raw=0x400、`.rdata` VA=0xA28000 Raw=0x627200、`.data` VA=0xB02000 Raw=0x700C00 RSz=0x30C00（→ ファイル実読は VA 0xB32C00 まで。以降は bss）。

---

## 1. セルフテスト（4件 PASS）

| # | 検証 | 期待 | 実測 | 結果 |
|---|------|------|------|------|
| ① | `bytes@0x504050` (8B) | `518b4c24188b5424` | `518b4c24188b5424` | **PASS** |
| ② | `bytes@0x4FB580` (4B) | `578b7908` | `578b7908` | **PASS** |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | `.\AI\CombatController.cpp` | **PASS** |
| ④ | オペコードテーブル opid `0x0015` impl==`0x504560` | impl一致 | idx21(VA 0xB0B290) name=`OnLoad`、opid u32 @+0x08=0x15、impl @+0x18=0x504560 | **PASS** |

ST4 レイアウト（補足）: 基底 `0xB0AF48`、stride `0x28`、532件。エントリ: `+0x00`=name ptr、`+0x08`=opid(u32)、`+0x18`=impl(実装VA)、`+0x1C`=第2関数ptr。

---

## 2. 中央レジストリ `0xB35574` の型解明

### 2.1 結論（判定）

`0xB35574` は **ハッシュテーブル（分離連結法 / separate chaining）** である。
**BST/赤黒木ではない**（左右子・親ポインタ・回転が一切無い）。**単一連結リストでもない**（バケット配列を哈希値でインデックス）。
キー比較・哈希は **仮想関数（vtable 経由）** でディスパッチされ、キー種別ごとに多重インスタンス化されたテンプレート的ハッシュマップクラス。設定(Setting)マップは**文字列キー**、同胞インスタンス（`0xb33508`）は**整数キー**。

### 2.2 オブジェクトレイアウト（this = 0xB35574）

| オフセット | アドレス | 意味 |
|-----------|---------|------|
| `+0x00` | `0xB35574` | vtable ポインタ（実行時にctorが書込） |
| `+0x04` | `0xB35578` | バケット数（bucket count） |
| `+0x08` | `0xB3557C` | バケット配列ポインタ（`node**`、malloc） |
| `+0x0C` | `0xB35580` | 要素数（element count） |

ノード構造体（12バイト、`malloc(0xC)`）:

| オフセット | 意味 |
|-----------|------|
| `+0x00` | `next`（バケット内の単方向連結） |
| `+0x04` | `key`（char* / uint） |
| `+0x08` | `value`（Setting* など） |

### 2.3 vtable スロット（仮想関数レイアウト）

| スロット | 意味 | 同胞(整数キー)例 |
|---------|------|-----------------|
| `+0x00` | dtor（`this` 破棄 + 任意で `free(this)`） | `0x417090` |
| `+0x04` | `hash(key) -> bucket_index` | `0x6A9060`（`key % bucket_count`） |
| `+0x08` | `compare(key, node_key) -> bool` | `0x763E80`（整数 `==`） |
| `+0x0C` | `node_init(node, key, value)` | `0x67F130`（`node->key=key; node->value=value`） |
| `+0x10` | `free_value(node)`（value解放、設定はno-op） | `0x68F970`（`ret 4`） |
| `+0x14` | `alloc_node() -> node*` | `0x46C0D0`（`malloc(0xC)`） |
| `+0x18` | `free_node(node)` | `0x573D00`（`free(node)`） |

### 2.4 判定根拠（証拠）

**(A) 検索 `FUN_0055E000`（capstone 実機 / decompiled L259922 一致）**

```
0x55E000: mov esi, ecx              ; this
          mov eax, [esi]            ; vtable
          mov edx, [eax+4]          ; hash
          push ebx(key); call edx   ; bucket = hash(key)
          mov ecx, [esi+8]          ; buckets
          mov edi, [ecx+eax*4]      ; node = buckets[bucket]   ← バケット配列を哈希で間接参照
  loop:   test edi,edi; je notfound
          mov eax,[edi+4]           ; node->key
          mov edx,[esi]; mov edx,[edx+8] ; compare
          push eax; push ebx; mov ecx,esi; call edx  ; compare(key, node->key)
          test al,al; jne found
          mov edi, [edi]            ; node = node->next       ← 単方向連結
          jmp loop
  found:  mov eax,[edi+8]; mov [out],eax ; return node->value
          mov al,1; ret 8
```
→ バケット配列インデックス＋連結リスト走査＝**分離連結法ハッシュ**。BSTの左右回転・親参照は無し。

**(B) 挿入 `FUN_00412D30`（ret 8, key/value 2引数）**
`hash(key)`→バケット走査で既存検索→無ければ `vtable[+0x14]` でノード確保、`vtable[+0x0C]` で初期化、**先頭にprepend**（`node->next = buckets[h]; buckets[h] = node`）、`[this+0xC]++`（要素数増）。

**(C) 削除 `FUN_004524C0`（ret 4, key 1引数）**
`hash(key)`→連結走査→一致で**unlink**（`buckets[h]`/prev の次ポインタ書き換え）→`vtable[+0x10]`(value解放)→`vtable[+0x18]`(free node)→`[this+0xC]--`。

**(D) 初期化 `FUN_00418DB0`（同胞マップ 0xb33508 の ctor）**
```
[esi+4]=0x25(37)            ; bucket count
[esi]=vtable                ; vtable
[esi+0xC]=0                 ; element count
eax=malloc(0x25*4)          ; バケット配列確保（401f00=malloc）
[esi+8]=eax                 ; buckets
memset(buckets,0,0x25*4)    ; 981630=memset
[esi]=vtable(derived)       ; 2段階vtable設定
call 0x417420               ; 追加init
```
→ バケット配列を heap に確保して初期化する、典型的ハッシュテーブル ctor。

**(E) 比較方法**：`compare` は **IAT の strcmp 直呼びではなく、vtable[+0x08] 経由の間接呼び出し**。同胞（整数キー）では `0x763E80`（`key1==key2`）。設定マップはキーが `char*`（`"sDeviceKeyboard"` 等）なので**文字列比較関数**が入る（下記 2.6 の未解決点参照）。

### 2.5 `0xB35574` の初期値

`0xB35574` は **`.data` の仮想領域外（bss）** — Raw=0x700C00 RSz=0x30C00 なのでファイル実読は VA `0xB32C00` まで。`0xB35574 > 0xB32C00` のため **ファイル初期値は 0（ゼロクリア）**。vtable / バケット数 / バケット配列 / 要素数はすべて**実行時に ctor が設定**する。`va2off(0xB35574) = None` を確認。

### 2.6 未解決（vtable 個別 VA）

同胞マップ `0xb33508` は ctor `0x418DB0` → vtable `0xA32C34`（整数キー: hash=`key%37`、compare=`==`）と特定。一方、**中央設定マップ `0xb35574` は文字列キー**なので別 vtable を使うべきだが、`mov ecx, 0xb35574; call ctor` 形式の直接 ctor 呼び出しは存在せず（19箇所すべて lookup/insert/remove）、**vtable は親オブジェクト経由か別パターンで実行時書込**と推定。文字列哈希/比較関数の具体 VA は本ラウンドで未特定（構造・仕組みは上記で完全に確定）。

---

## 3. 登録者数の見積（系統別）

中央マップへの登録は 2 種の setter 経由:

- **`FUN_00419A70`**（string/int setter, thiscall ret 8）: `[this]=value, [this+4]=key` → 重複検査(`0x55E000`) → 挿入(`0x412D30`)。呼び出し **1312 件**。
- **`FUN_0041BAE0`**（float setter, thiscall ret 8）: `fstp [this]`(float値), `[this+4]=key` → 同上。呼び出し **992 件**。

**合計 2304 件** が中央マップへ登録。

| 系統 | key プレフィックス | 件数 | getter |
|------|------------------|------|--------|
| float 設定 | `f*` | 978 | （直接 value 読出） |
| 文字列設定（UI/メニュー等） | `s*`（その他） | 970 | — |
| int 設定 | `i*` | 227 | — |
| **表C 説明文** | `*Desc*`（`sAttributeDesc*`/`sDerivedAttributeDesc*`/`sSkillDesc*`） | 45 | **FUN_00565CF0** |
| **表A AV表示名** | `sAttributeName*`/`sDerivedAttributeName*`/`sSkillName*`/`sTraitName*`/`sVirtueName*` | 36 | FUN_00565CC0 |
| **効果表示（接頭辞＋単位）** | `sMagicEffectItem*` | 17 | （sprintf書式） |
| **表C アイコン** | `sAttributeIcon*`/`sHandToHandIcon` 等 | 17 | FUN_00565D10 |
| （float/未分類） | — | 14 | — |

**thunk 群の合計数**: `call 0x419A70` = **1312**（`call 0x41BAE0` = 992 を加えた総登録 = 2304）。
登録 thunk は `.CRT$XCU`（静的初期化配列, `.rdata` 0xA283D4〜）に登録され、起動時に1件ずつ実行される。各 thunk は `push value; push key; mov ecx,<BSS内Settingオブジェクト>; call 0x419A70; push <dtor>; call 0x981fb4(atexit)` 形式。

補足: 効果表示の5接頭辞（Fortify/Restore/Drain/Damage/Absorb = スタブ `0x9D9EE0..0x9D9F6F`）は `sMagicEffectItem*` 17件の一部。残りは `...For/On/In/Seconds(Singular|Plural)/Percent/Points(Singular|Plural)` 等の UI 整形文字列。

---

## 4. 衝突警告（重複登録時）

`FUN_00419A70` / `FUN_0041BAE0` は挿入前に `FUN_0055E000`（key検索）で既存を確認し、衝突（`test al,al` 非ゼロ）すると:

```
push key ; push 0xA32490 ; call 0x4A7A60 ; add esp,8
```

**書式文字列 `0xA32490` の実読値**:

```
Setting key '%s' already used in map.
Setting keys must be unique.
```

`FUN_004A7A60` は警告ロガー（`push 1; push fmt; push args; call 0x4941A0` — severity=1 のコンソール/ログ出力）。既存登録は破棄され、新しい Setting オブジェクトは map に挿入されない（登録元は早期 return）。

---

## 5. `FUN_00565CF0`（表C getter）の用途分類

### 5.1 getter 本体（decompiled L265552）

```c
undefined4 FUN_00565cf0(uint param_1) {
  if ((param_1 < 0x21) && (PTR_DAT_00b127f8[param_1] != 0))
    return *PTR_DAT_00b127f8[param_1];   // Setting->value (char*) を返す
  return 0;
}
```
- 表C = **`0xB127F8`**（.data内, 33エントリ = idx 0..0x20）。各エントリは BSS 内 Setting オブジェクトへのポインタ。
- 返り値は Setting->value（INI解決済みの英語説明文 `char*`）。

### 5.2 全呼び出し元（lowercased `fun_00565cf0(` 1パス走査、件数 = 4）

| # | 行 | 呼び出し元 | 引数 | サブシステム |
|---|-----|-----------|------|-------------|
| 1 | L316714 | `FUN_005ace20` | `0`（定数: Strength=idx0） | **Level-Up メニュー生成**。`FUN_00565d10(0)`(Icon)→トークン0xfb2、`FUN_00565cf0(0)`(Desc)→トークン0xfb3。周辺に "Level Up Menu Creation Failed" 文言 |
| 2 | L339551 | `FUN_005d5b40` | `param_2`（動的AV idx） | **ステータス/スキル情報メニュー更新**。iVar2==1 分岐で `cf0(param_2)`→Desc トークン0xfaf、`d10(param_2)`→Icon トークン0xfb0 |
| 3 | L342781 | `FUN_005dbbd0` | `uVar5`（AV/スキル idx） | **スキル/レース系メニュー更新**（param_1==0x18 分岐）。`d10(uVar5)`→Icon トークン0xfb0、`cf0(uVar5)`→Desc トークン0xfb1 |
| 4 | L342789 | `FUN_005dbbd0` | `iVar7 + 4`（スキルidx＋4 オフセット） | 同関数 param_1==0xe 分岐。`cf0(iVar7+4)` の返りを `FUN_004028d0`（String代入）で取り込み、文言処理 |

**上位10**（全4件）: `FUN_005dbbd0`×2、`FUN_005d5b40`×1、`FUN_005ace20`×1。
**引数パターン**: 定数0×1 / `param_2`×1 / 変数×1 / 変数+4オフセット×1。

分類: いずれも **効果/ステータス説明 UI（メニュー）** — コンソールやワールド処理ではなく、レベルアップ・ステータス表示・スキル説明の各メニューが AV/スキル idx を取り、表Cから説明文（と対で表Iconからアイコン）を取得してメニュー文字列トークンに格納している。

### 5.3 表A getter `FUN_00565CC0` との使い分け（証拠つき）

| getter | テーブル | idx範囲 | 返り値 | 用途 |
|--------|---------|---------|--------|------|
| `FUN_00565CC0` | `0xB12758`(40) + `0xB12868`(32) | 0..71（AV数=72） | AV**表示名**（`char*`）「Strength」「Blade」等 | 表示名（ラベル） |
| `FUN_00565CF0` | `0xB127F8`(33) | 0..0x20 | AV/スキル**説明文**（`char*`） | 説明文（詳細文） |
| `FUN_00565D10` | `0xB12880`(33) | 0..0x20 | AV/スキル**アイコンパス**（`char*`） | アイコン |

**使い分け判定**: 表示名（`5CC0`）と説明文（`5CF0`）／アイコン（`5D10`）は**別テーブル・別getter**。表C（`5CF0`/`5D10`）は idx<0x21 の 33 エントリ = 属性8＋派生4＋スキル21（説明）、アイコンも同数。呼び出し元が **必ず `5D10`(Icon) と対で `5CF0`(Desc) を呼ぶ**（L316712-316715, L339553-339554, L342779-342781）ことで「説明文＋アイコン」セット取得しているのが決定的証拠。表A（`5CC0`）はメニューのラベル生成等で単独使用（L12100等）。つまり:

- **表示名 = 表A = `FUN_00565CC0`**
- **説明文 = 表C = `FUN_00565CF0`**、**アイコン = 表Icon = `FUN_00565D10`**（Desc/Icon は常に対）

### 5.4 表C/表Icon の登録キー（ thunk `0x9F9B60+0x20i` 実読）

- 説明（表C, 33件 → `0xB127F8`）: `[0-7]` `sAttributeDesc{Strength..Luck}`、`[8-11]` `sDerivedAttributeDesc{Health,Magicka,Fatigue,Encumbrance}`、`[12-32]` `sSkillDesc{Armorer..Speechcraft}`
- アイコン（表Icon, 33件 → `0xB12880`）: `[0]` `sHandToHandIcon`、`[1-]` `sAttributeIcon{Strength..}` 等（`Menus\Stats\stat_pop_icon_*.dds`）

表A AV名（`0x9F9660+0x20i`）: `[0-7]` `sAttributeName{Strength..Luck}`、`[8-11]` `sDerivedAttributeName{Health,Magicka,Fatigue,Encumbrance}` … （表示名）

---

## 6. データ関係性図（key→value の流れ）

```
                          ┌─────────────────────────────────────────────┐
                          │  静的初期化 .CRT$XCU（起動時 1件ずつ実行）      │
                          │  登録 thunk 群（stride 0x20, BSSにSetting*）    │
                          └───────────────┬─────────────────────────────┘
                                          │ call FUN_00419A70 / FUN_0041BAE0
                                          │ (key=value文字列, Setting obj)
                                          ▼
        ┌──────────────────────────────────────────────────────────────────┐
        │              中央レジストリ（Settings Hash Map） @0xB35574         │
        │  ┌──────────┬──────────┬────────────┬──────────┐                 │
        │  │ +0 vtable│+4 count  │+8 buckets  │+C nelems │                 │
        │  └──────────┴──────────┴────────────┴──────────┘                 │
        │  バケット: node*[] (malloc)   ノード: {next, key, value} (12B)      │
        │  hash= vtable[+4](key)      compare= vtable[+8](key,node_key)     │
        │  ※重複時: 警告 0xA32490 "Setting key '%s' already used in map."   │
        └──────────────────────────────────────────────────────────────────┘
             ▲ 登録(Insert)                          │ key検索(Lookup: 0x55E000)
             │                                       ▼
   ┌─────────┴──────────┐              ┌──────────────────────────────┐
   │ key 種別ごとの系統    │              │  静的テーブル（.data, Setting*配列）│
   ├────────────────────┤              │  各getterが idx→Setting*→value 返 │
   │ sMagicEffectItem*  │17件           └──────────────────────────────┘
   │  (効果接頭辞+単位)   │                        │
   │ s*Name* (表A AV名) │36件                     │ idx
   │ sAttributeDesc*    │  ┌──────────────────────┼─────────────────────┐
   │ sDerivedAttribute..│  │                      │                     │
   │ sSkillDesc* (表C)  │45件                     ▼                     ▼                     ▼
   │ sAttributeIcon*    │17件   表A getter          表C getter          表Icon getter
   │  (表Icon)          │      FUN_00565CC0       FUN_00565CF0        FUN_00565D10
   │ f* (float設定)     │978件  @0xB12758(40)     @0xB127F8(33)       @0xB12880(33)
   │ i* (int設定)       │227件  +@0xB12868(32)    idx<0x21            idx<0x21
   │ s* (その他文字列)   │970件  idx 0..71         → 説明文 char*       → アイコンパス char*
   └────────────────────┘  → 表示名 char*            │                     │
                                          （menu が Desc+Icon を対で取得）
```

**key→value フロー**: thunk が `{value=英名/書式, key=INI名}` を BSS の Setting オブジェクトに格納し中央マップへ Insert。getter は idx→静的テーブル→Setting*→`->value`（=INIで上書き可能な文字列）を返す。表A=表示名、表C=説明文、表Icon=アイコンで、Desc/Icon はメニューが対で取得。

---

## 7. 未解決点

1. **中央マップ `0xb35574` の個別 vtable VA と文字列 hash/compare 関数の VA** — 同胞 `0xb33508`（整数キー, vtable `0xA32C34`, hash `0x6A9060`=key%37, compare `0x763E80`==）は ctor `0x418DB0` で特定。中央マップは文字列キーなので別 vtable だが、`mov ecx,0xb35574; call ctor` 形式が無く（19参照すべて lookup/insert/remove）、親オブジェクト経由の実行時書込と推定。文字列哈希/比較関数の具体 VA は未特定（構造・仕組みは確定）。 → 確定 (2026-10-24, V63): GMST hash 表 vtable = `0xA40454`（hash=`FUN_004A80A0`=djb2、compare=`0x584D10`=`__stricmp`）。
2. **表A part2（`0xB12868`, idx 40..71 = 32件）の登録経路** — part1（40件）は BSS Setting オブジェクト経由だが、part2 は直接文字列ポインタの可能性。skill名の残りがどう格納されるかは未確認。
3. **float setter の未分類14件** — push 抓取パターン外（直接レジスタ渡し等）の key が分類不能。実影響は小さい。

---

*本ファイルは V49 の新規成果物。既存ドキュメントは未編集。スクリプトは `%TEMP%\opencode\` にのみ配置。*
