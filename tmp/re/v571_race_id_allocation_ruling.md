# v571 — V-570-3 クローズ: RaceSexMenu の要素 id 採番系の完全確定（200系カウンタ）と 28 ブロック照合の帰結（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v571_1〜v571_8**・8本 全 EXIT=0）＋ `race_sex_menu.xml`。
先行: `tmp/re/v570_racenotify_dispatch_ruling.md`（V-570-3 の起票元）／`tmp/re/v558_racesexmenu_pages_ruling.md`（§1.1 順序）
判定: `[確定(命令)]` / `[確定(XML+命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| ページ pane / 要素の id はどこで決まるか | **[確定(命令)]** | **単一カウンタ `[0xB3B5D4]` の自動採番**（§2）。**RaceSexMenu コンストラクタ `0x5C4E40` が `[0xB3B5D4]=0xC8(200)` にリセット**（`0x5C4F3C: mov dword [0xB3B5D4], 0xc8`）。**`AddPane 0x5C4340` は id 引数（a4）が -1 のときのみ `counter++` して採番、`SetFloat(pane, 0xFA8, id)` と `SetFloat(pane, 0xFAE=user0, id)` の両方に書込む**（明示 id のときは非消費）。 |
| 生成される id の範囲 | **[確定(命令)]** | init `0x5CA010` 内の **139 呼出**を消費型で機械復元（§3 の表・probe v571_7）。**Main ページのみ明示 id=0**、以降の採番は **counter=201..428（表の 0..228 に +200）**。消費型: `AddButton 0x5C4480`(+1)／`AddToggle 0x5C4630`(+1)／`AddText 0x5C4800`(+1)／**`Slider 0x5C93F0`(+2)**（`eax=c+1` を保存し `c+2` を確定 → スライダは2 id 分）。**21 ページの生成順は v558 §1.1 と完全一致**（Main/Race/Hair/Face/Shape/Tone/General(Face)/Brow/Cheeks/Chin/Eyes/Forehead/Jaw/Mouth/Nose/General(Skin)/Beard/Eye shades/Eyebrows/Mouth(Lips)/Nose shades）。 |
| これが v570 の 28 ブロックとどう整合するか | **[確定(命令)]＋[部分確定]** | **XML の `<id>`（1,2,10,20,90,100-104）と 200 系は非重複**（§4）。→ **id10 / id90 / id98 は 28 ブロックの act を経由しない**: id90 は先頭のページ判定で消費、**id10 はゲート act（名前同期）のみ**（ブロック照合は全て `id < tile_id` で epilogue）、**id98 は block1（Random）の less 経由 `0x5CCBF4` に到達する唯一の特別 act**（顔フォーム再構築）。**28 ブロックの act は id 201-428 の要素通知（ボタン/スライダ等）でのみ発火する**。要素 id→28 ブロック対応の機械表は §3.2 [部分確定]（`0x5C3440` の str1/str2 役割分担が §5 の写像で [部分確定] のため）。 |
| id98 の発生源 | **[未確定]（絞込み済）** | **200 系でも XML(1,2,10,20,90,100-104) でもない第三系統**。残る候補はコード明示の通知（`push 0x62` 経由）または別ハンドラ経由。act 内容（`vfunc+0x170→0x526F00→0x5C91E0`＝顔フォーム再構築系）は v570 で [確定(命令)] 済み。 |

---

## 2. 採番系の構造 [確定(命令)]

```
ctor 0x5C4E40（vtable 0xA6D38C = RaceSexMenu）:
  0x5C4F3C  mov dword [0xB3B5D4], 0xC8         ; counter = 200 にリセット
  0x5C4EA7  lea edi,[esi+0x94]; ecx=0x1F4      ; id→pane 配列 [menu+0x94..+0x190) を zero (125 個分)

AddPane 0x5C4340 (ret 0x14 = 5引数, ecx=this):
  a1 (P+0)   = instantiate のリスト引数       ([esp+0x34])
  a2 (P+4)   = ページ名文字列                 ('%s' sprintf の src、[esp+0x38])
  a4 (P+0xc) = id 引数                        ([esp+0x40])  ; prologue は 0x30 分の退避（push -1/sehp/fs + sub 0x10 + 4push + cookie）
  if (a4 == -1) { a4 = ++[0xB3B5D4]; } else { /* 非消費 */ }
  SetFloat(pane, 0xFA8 /*id*/, float(a4))
  SetFloat(pane, 0xFAE /*user0*/, float(a4))    ; v567 のページキーはこの user0=id
  SetFloat(pane, 0xFAA /*listindex*/, GetFloat(list, 0xFD0) - 1.0)   ; 定数 [0xa2f928]=1.0
  pane → [this + a4*4 + 0x94] に格納           ; ctor の配列と一致
  instantiate 'race_template_pane'
```

**子要素生成4関数（全員カウンタ採番）**:

| 関数 | ret | テンプレ | 消費 |
|---|---|---|---|
| `0x5C4480` | — | `race_template_button` | **+1**（`eax=[c];+1;存;使用`） |
| `0x5C4630` | — | `race_template_toggle` | **+1** |
| `0x5C4800` | — | `race_template_text` | **+1** |
| `0x5C93F0` | — | `race_template_slider` | **+2**（`c+1` を保存、`c+2` を確定使用） |

テンプレ名は race_sex_menu.xml の `<template name=…>` と完全一致 [確定(XML)]。

---

## 3. init の 139 呼出から復元した id 表 [確定(命令)]

### 3.1 構造（順序・消費は v558 §1.1 と一致）

`0x5CA010` 内の呼出は常に **[Page → そのページの子要素群]** のブロック（probe v571_6 の E8 全列挙 139 件）。カウンタは 200 から開始し、**Main のみ id=0 明示**（非消費）。

| ページ（生成順） | id（200系） | 子要素（消費） |
|---|---|---|
| 1 Main | **0**（明示） | Button×3・Toggle×1・Slider×2（=2+4=+6 → c=206） |
| 2 Race | 209 | Toggle×2・Text×1（+3） |
| 3 Hair | 213 | Toggle×2・Slider×4（+2+8） |
| 4 Face | 224 | Button×3（+3）※Shape/Tone/Reset 系 |
| 5 Shape | 229 | Button×9（+9） |
| 6 Tone | 239 | Button×6（+6） |
| 7 General(Face) | 246 | Slider×5（+10） |
| 8 Brow | 257 | Slider×3（+6） |
| 9 Cheeks | 264 | Slider×5（+10） |
| 10 Chin | 275 | Slider×7（+14） |
| 11 Eyes | 290 | Slider×4（+8） |
| 12 Forehead | 299 | Slider×3（+6） |
| 13 Jaw | 306 | Slider×4（+8） |
| 14 Mouth | 315 | Slider×10（+20） |
| 15 Nose | 336 | Slider×14（+28） |
| 16 General(Skin) | 365 | Slider×5（+10） |
| 17 Beard | 376 | Slider×6（+12） |
| 18 Eye shades | 389 | Slider×6（+12） |
| 19 Eyebrows | 402 | Slider×7（+14） |
| 20 Mouth(Lips) | 417 | Slider×3（+6） |
| 21 Nose shades | 424 | Slider×2（+4）→ 最終 c=**428** |

（ページ id = 200 + v571_7 表の counter 列。子要素 id も同オフセット。例: Eyes のスライダ4個 = (291,292),(293,294),(295,296),(297,298)）

### 3.2 28 ブロックへの適用 [部分確定]

* **tile = `0x5C3440(str1, str2, flag)` → `GetPage(str1)` 配下の str2 要素**（§5 の写像による）→ **tile id = 200系の要素 id**。
* block1(Random)／block2(Reset)／block3(Age)／block4-6(Red/Green/Blue)／block7(Length)／block8(Gender) は §3.1 の Face/Main/Hair/Race の要素一覧と機械突合すると **名前が一致しない箇所がある**（例: Face ページの子は Button×3 のみで Random の名前が直近 glob 判定と一致しない等）→ **呼び出し側 str 構築の順序と `0x5C3440` の a1/a3 の割当を完全に突合し終えるまで [部分確定]**。
* block9-28 は機械突合が一致（例: block13 Race∈Main＝Main ページの Race ボタン id 201、block10 Shape∈Face＝225 等）— §3.1 の+200 適用で **201系**。

---

## 4. XML `<id>` との非重複と帰結 [確定(XML+命令)]

| 系統 | id 群 | 28 ブロック act |
|---|---|---|
| XML 明示（race_sex_menu.xml `<id>` 10件） | 1(window)／2(grab)／**10(race_name)**／20(scroll)／**90(Done)**／100-104(テンプレ) | **どのブロックとも一致しない**（tile id は 201-428）→ act なし |
| カウンタ採番（コード生成 139要素） | **201-428**（Main ページのみ id=0 明示） | **act はこの系統の通知のみで発火** |

* **id90**: `0x5CC277` で先頭に分離 → ページ Done 処理のみ（v561/v567 済）。
* **id10**: ゲート `al==0` のときだけ受理 → 名前同期 act → dispatch へ入っても全ブロック less → epilogue（act なし）。
* **id98**: `block1(Random) の less` → `0x5CCBF4 cmp ebx,0x62` → **98 の特別 act のみ**。**98 < 201 なので全ブロック less が保証される**（＝98 専用経路が構造上必ず走る）。
* → **v570 の残点 V-570-3 の核心はこれで解消**: 98 は「200系でも XML もない第三系統の通知」（発生源の特定のみ残る）。

---

## 5. `0x5C3440` の引数写像（prologue 算術・[部分確定]）

`0x5C4340` と同型の退避で計算すると、**`0x5C3440`（ret 0x10 = 5引数）は prologue 0x20 分の退避**（push -1/sehp/fs/ecx/ebx/ebp/esi/edi + cookie… 実測 8push+cookie）→ `[esp+0x24]=P=a1`、`[esp+0x2c]=P+8=a3`、`[esp+0x40]=P+0x1c=a7??` — **中身は `GetPage(構築 str)` → `0x589930(page, 残り str)`**。呼び出し側は `push flag` → str 構築×2 → で **a1=最後に構築した str（= v570 表の strB）**、a3=先の str（strA）と読めば **`GetPage(strB) 配下から strA`** で v570 の表（strA=要素, strB=親）と整合するが、**a3 が str object の後半（len）と衝突する可能性が残る** → **`0x589930` のシグネチャ実測が必要**＝[部分確定]。

---

## 6. 移植への示唆

1. **id は静的採番でない**。原版はメニューオブジェクトごとの**動的カウンタ（初期 200）**で id を振る。移植が id を静的に固定するなら、**「id ≥ 201 の要素通知」だけが dispatch act に入る**という役割分担を再現する必要がある。
2. **Main ページ id=0 のみ特別**（唯一の明示 id）。user0=id のページキーとしても 0 になる。
3. **スライダは 2 id 消費**（marker/bar の2要素分）。移植でスライダ id を数えると原版と 1 つずれる。
4. **id10/90/98 は 200系と構造的に分離**している（v570 の act 5 種は 200系要素用、10/98 は特別経路）— 移植の通知設計もこの2系統を分けるのが原版準拠。

---

## 7. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-571-1 | **id98 の発生源**（200系外・XML外の第三系統。`push 0x62` を送る呼び出し元の特定） | **中** |
| V-571-2 | `0x589930` のシグネチャと `0x5C3440` の a1/a3 確定 → §3.2 の block1-8 を [確定] 化 | 中 |
| V-571-3 | `0x5C4F3E` 以外のカウンタ書手（他メニューでも 200 リセットを使うか）と `[0xB3B5D4]` を触る 5 関数のうち `0x5C4E40`(ctor) の残り参照（0x5C4F3C 以外） | 低 |
| V-571-4 | AddPane の a3/a5（未使用か、別の trait か）の意味 | 低 |

---

## 8. probe 一覧

`v571_1`（AddPane 0x5C4340 全 decode・init 内 push 0xfa8）／`v571_2`（21 AddPane 呼出の push 列・counter refs・定数 [0xa2f928]=1.0）／`v571_3`（呼出直前のページ名 glob 21 件＝生成順）／`v571_4`（init の edi 書き手5件＝`or edi,-1` を特定）／`v571_5`（counter=.bss・E8 全列挙・heads 3関数）／`v571_6`（`0x5C93F0`=slider+2・`0x5C4E40`=RaceSexMenu ctor・E8 全139件）／`v571_7`（**139 呼出の id 復元表＋28 ブロック機械突合**）／`v571_8`（AddPane prologue byte 級＝a4=id を確定）。全 8 本 EXIT=0。
