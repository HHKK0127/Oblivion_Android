# v606 — メニュー open/生成/notify 系 低優先残点クローズ（バンドル）: `0x587B20`・begin/end・名前/式解決・slot5 open・`0x42BB90`（ai-todo #156/#161/#166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u19_1**・EXIT=0・冒頭 Test-Path）。
先行: `tmp/re/v574_hud_open_path_ruling.md`・`v575_common_open_noop_ruling.md`・`v576_entity_mechanism_and_open_contradiction_ruling.md`・`v579_587b20_floatid_ruling.md`・`v580_open_eighth_system_ruling.md`・`v581_third_path_ruling.md`・`v582_command_chain_ruling.md`・`v583_third_path_firing_ruling.md`・`v584_chargen_gate_ruling.md`・`v586_laneC_reconcile_ruling.md`
判定: `[確定(命令)]` / `[部分確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-575-3 / V-580-2** `0x587B20(1,id)` の役割 | **[確定(命令)]** | `0x587B20` は**メニュースタック（id→生成済み配列）の初期化関数**。`arg1`（`[esp+0x18]`）が真かつ `[0xB3A708]` が NULL なら `0x401F00(1)` で確保→**`0x4E4A10(0xB13970, 0x33)`**（`ecx=0xB13970`＝メニュースタックのグローバル、引数 `0x33`＝要素数/型）を呼んで構築し `[0xB3A708]` に格納。→ **`0x587B20` = メニュースタック(`0xB13970`)の遅延初期化**。 |
| **V-574-3 / V-580-3** `0x42BB90` の実体 | **[確定(命令)]** | `0x42BB90` は `0x587B20` が生む**メニュースタック `0xB13970` に対する要素 accessor**（`0x4E4A10(0xB13970, count)` が構築、`0x42BB90` が id→生成済み要素の検索/登録）。→ **真の「メニュースタック」データ構造＝`0xB13970`**。 |
| **V-584-2** `0x583E60`(begin) / `0x583F40`(end) | **[確定(命令)]** | `0x583E60`（begin）: `[esi+8]==3` かつ `[esi+9]==0` のとき `[esi+8]=5`、`SetFloat(0xFA1, 1.0)`（visible）、`SetString(0xFE6, 'Menus\Misc\cursor.dds')`、`0x58E870`（描画更新）＝**カーソル表示＋メニュー表示開始**。`0x583F40`（end）: `[edi+9]=0`、子要素 `[+0x34]` ツリーを走査（`0x5898F0`/`0x583D50`）＝**メニュー終了処理**。→ **begin/end はメニューのライフサイクル開始/終了**。 |
| **V-582-3 / V-576-2** `0x588EF0` / `0x58BEE0` / `0x58D2F0` | **[確定(命令)]** | **`0x588EF0(name)`＝名前/記号解決関数**（`&name;` は `0xB3B0B4` の 16B ノード配列を `__mbsicmp` で探索→値返却、`_key` は `0x5F`、英字は大文字化して index 化）。**`0x58BEE0`＝XML 値/メニュー要素の値解決器**（`[+0x18]==0xFA2` type ゲート）。**`0x58D2F0`＝値解決のラッパ**（`0x588EF0` を呼び、`_` 始まりは `0x58B040` へ）。→ **`&...;` 参照と `_...` 記号の解決系**（v576/V-576-2 の entity 消費者）。 |
| **V-581-2** slot5=open の呼称 | **[部分確定]** | `0x591C50` は `TileMenu` vtable slot5 で、`0x4D7F80` 系の type ゲート（`cmp a1,0xFA2`＝XML type）を通して要素を可視化する。**「open」はメニュー要素の可視化を指す**（v550 の slot+0x14 open 仮想と整合）。 |
| **V-584-3** `0x5ABBC0` の menu 0x416 | **[部分確定]** | `0x5ABBC0` は menu `0x416`(1046)＝**`ChargeMenu`/`RechargeMenu` 系**を操作（v584 の通知 id 1..5）。**厳密名は [部分確定]**。 |
| **V-584-4** `0x578D70` の `al==1` vs `al==2` | **[確定(命令)]** | `0x578D70()` は**応答バイト読取＋クリア**（`[singleton+0xB0]` を読んで 0xFF に戻す）。**値は `[MessageMenu+0x60](base=1)+id-4`**（v594）。よって **`al==1`（id4）＝否定（No）／`al==2`（id5）＝肯定（Yes）**。v570 cb が `al==2` で実行・`0x5B8DC0` が `al==1` で実行という差は**ボタン別コールバックの割当差**（v592/v594 と整合）。 |
| **V-585-2/3** `0x57D940`/`0x584880` | **[部分確定]** | `0x57D940`＝begin/end 内の**HUD 表示サブヘルパ**（`0x57D640` の第1 caller）。`0x584880`＝`[+0xFA5]`(stackingtype) の 3 値比較を行う**メニュー種別判定**（v584 の `0x5ADCF0` が同型を使う）。 |
| **V-587-2** `0x579C10` の flag(arg3) | **[確定(命令)]** | `0x579C10(this, cb, cbarg, flag, a5)` の **arg3(flag) は `0x5BC8B0` の a5 に転送され `[entry+0x60]` に保存**（v592）。flag=1 と 0 は**ダイアログのボタン構成/既定応答**を切り替える。 |
| **V-587-4** `0x5AD980` の callers | **[確定(命令)]** | `0x5AD980`（LoadingMenu 進捗ポンプ）の callers は v597 で確定（`0x57E150`／`0x462B20`／`0x465860`／`0x578E00`／`0x5ADB40`）。**begin/execute からの直接呼出は無く、メニュー更新ループ経由**。 |
| **V-589-2/3** `0x5898F0` / `0x5A7CD0` | **[部分確定]** | `0x5898F0`＝**子要素探索**（`[+0x10]` チェーン）で基底メニューオブジェクト取得。`0x589D60`＝`call [vt+0xc]` の戻りを `cmp eax,0x386`（型 ID）で判定。`0x5A7CD0` の `[elem+0xc]→[+0x1c]` は**要素の id/コールバックフィールド**（v595 の要素リスト描画）。 |
| **V-583-3** `0xFA1` ブロック `0x586610` | **[部分確定]** | `0x586610` の `push 0xFA1` は **visible trait の設定ブロック**（type4001）。**`0xFA1`＝`visible`**（v557 で確定済）で、type4001 は trait 型 enum の1つ。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-575-3/V-580-2/V-574-3 メニュースタック

```
0x587B20(create):
  587B41 cmp byte [esp+0x18],0 ; 587B46 mov eax,[0xB3A708] ; 587B4B je 0x587B9C
  587B53 call 0x401F00(1)
  587B6F mov ecx,0xB13970 ; 587B74 call 0x4E4A10(0xB13970, 0x33)   ; ★ メニュースタック構築
  587B8C mov [0xB3A708], esi
```

### 2.2 V-584-2 begin/end

```
0x583E60(begin): [esi+8]==3 && [esi+9]==0 → [esi+8]=5
  SetFloat(0xFA1, 1.0) ; SetString(0xFE6, 'Menus\Misc\cursor.dds') ; 0x58E870
0x583F40(end): [edi+9]=0 ; 子要素 [+0x34] ツリー走査（0x5898F0/0x583D50）
```

### 2.3 V-582-3/V-576-2 名前/式解決

```
0x588EF0(name): [name]=='&' → 0xB3B0B4 の 16B ノードを __mbsicmp で探索
                '_' → 0x5F ; 英字 → 大文字化 index
0x58BEE0: cmp word [esi+0x18],0xFA2   ; XML type ゲート
0x58D2F0: call 0x588EF0 ; '_' 始まり → 0x58B040
```

### 2.4 V-584-4 応答値

```
0x578D70(): bl=[singleton+0xB0] ; [singleton+0xB0]=0xFF ; ret al=bl
  （値 = base(1)+id-4 → id4=1(No) / id5=2(Yes)）
```

### 2.5 V-587-2 flag

```
0x579C10(this,cb,cbarg,flag,a5) → 0x5BC8B0 → [entry+0x60]=a5(=flag)  ; flag がボタン構成を切替
```

---

## 3. 残点（更新後）

- **V-576-3・V-577-2・V-579-2・V-581-3・V-582-2・V-583-2・V-585-2・V-589-3・V-591-2/3・V-592-2・V-593-3/4・V-596-1/2/3【低】**: identity/細部（いずれも実装影響なし）。
- **V-568-1・V-573-2・V-599-2・V-598-4・V-601-1【低】**: 動的領域/実用価値低（打切り継続）。

---

## 4. 検証

- probe: **u19_1（1本）EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x587B20`（`0x4E4A10(0xB13970,0x33)`）／`0x583E60`（`0xFA1`/`cursor.dds`）／`0x583F40`／`0x588EF0`（`&name;` 解決）／`0x58BEE0`（`0xFA2` type）／`0x58D2F0`／`0x578D70`（応答値）／`0x579C10`（flag）。
