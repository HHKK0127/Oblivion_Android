# v589 — Lane B 統合: id98 送信元は静的定数として不存在（v573 の「`call [reg+0x0c]` 247件＝通知入口」を [確定負] に訂正）・真の通知ディスパッチ形式の確定（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（Lane B probe **v600_1〜v600_21**・全 EXIT=0・冒頭 Test-Path、`.text` 全域 linear 逆アセンブル 2,258,077 命令）。
先行: `tmp/re/v573_id98_source_survey_ruling.md`（本裁定で一部訂正）／`tmp/re/v570_racenotify_dispatch_ruling.md`（id98 act）
判定: `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| 通知 id は通知ハンドラへどう渡るか | **[確定(命令)]** | **call 直前に push される第1引数**（メッセージ構造体のメンバではない）。`0x5CC200` は `ret 8`（2引数）で id は `[ebp+8]`。 |
| v573 の「`call [reg+0x0c]` 247件＝通知ハンドラ入口」は正しいか | **[確定負（v573 訂正）]** | **誤り**。`call dword ptr [reg+0x0c]` の**直接形 247件はすべて 0x80xxxx〜0x9Axxxx 帯**（別クラス群）で、**UI 帯 0x500000〜0x610000 には 0件**。真の通知ディスパッチは **2段形式**（§2）。 |
| 真の通知ディスパッチ形式 | **[確定(命令)]** | `mov reg,[obj]; mov reg2,[reg+0xc]; push <arg2>; push <id>; mov ecx,obj; call reg2`（§2）。全 **695 サイト**を機械抽出。 |
| id98（`0x62`）は静的定数として存在するか | **[確定(命令)（negative）]** | **不存在**。全695 notify サイトの id 引数（直前 push の最後）に即値 `0x62` は **0件**（実在する即値は `0x16, 4, 0, 1, 0x14, 0x26, 0x63, 0x2a…`）。`push 0x62`=3件・`push imm32 0x62`=0件・`mov reg,0x62`=0件（v573 と同じ）。 |
| RaceSexMenu(0x40C) へ id98 を送る静的サイトはあるか | **[確定(命令)（negative）]** | **0件**。`push 0x40C` は 20件あるが、**すべて前方45命令以内に `call [reg+0xc]`（通知）へ到達しない**（用途は FindMenu/getchild でのプロパティ操作・閉鎖・再構築）。`push imm32; 0x589B70(FindMenu); …; notify` 形で到達するのは **menuid `0x40E`（id=0）と `0x3EA`（id=ebp）のみ**。 |
| id98 の発生源 | **[未確定]（絞込み済）** | **実行時に決まる id（UI 要素のクリック→親メニュー通知の中央ループ由来の変数）**。候補＝`0x579970`（メニューマネージャ tick）／`0x5821F0`（frame 更新）／`0x5A5E00`〜`0x5A6040`（HUD 通知サブハンドラ）の上位。→ **V-589-1 として継承**（V-573-1 を再定義）。 |

---

## 2. 真の通知ディスパッチ形式 [確定(命令)]

```
mov  reg,  dword ptr [obj]        ; vptr
mov  reg2, dword ptr [reg + 0xc]  ; vtable[3] = notify ハンドラ
push <arg2>
push <id>                         ; ★ 通知 id はここで push（第1引数）
mov  ecx, obj                     ; this
call reg2
```

実例（逐語）:
| サイト | head | id | 備考 |
|---|---|---|---|
| `0x5A1349` | `0x5A08E0` | `0x16`（定数） | |
| `0x59DA1D` | `0x59D9D0` | `4`（定数） | |
| `0x46576C` | `0x465130` | 変数（`ecx=[esp+0x18]`） | |
| `0x583929` | `0x5821F0` | `0` | menu `0x40E` へ |
| `0x5C0940` | `0x5C07D0` | `ebp`（変数） | menu `0x3EA` へ |

* **`0x5CC200` の E8 直接 caller＝0件**。他メニューの notify エントリ（HUD `0x5A68C0`／Dialog `0x59F010`／Options `0x5BD880`／TileMenu `0x5900D0`／Generic `0x5A3D80`／Message `0x5BCDD0`／Class `0x596FE0`／TextEdit `0x5DD1B0`／Save `0x5D3D20`／Load `0x5AEED0`／Video `0x5DF160`）も**全て E8 caller 0件** → **通知ハンドラは必ず間接呼び出しのみ**。
* **`0x5CC200` の dword は `.rdata 0xA6D398`（＝RaceSexMenu vtable slot[3]）1件のみ**、`.text` に即値は0件。

---

## 3. 副次成果: 汎用「メニューへ id を送る」ヘルパ群 [確定(命令)]

```
0x57CDE0: push ebx; mov bl,[esp+8]; push 0x3EB; call 0x589B70; …; push 0; push ecx(bl); call [vt+0xc]  ; → StatsMenu 0x3EB
0x57CE20: 同形, push 0x3EA                                                                            ; → InventoryMenu 0x3EA
0x57CE60: 同形, push 0x3FE                                                                            ; → MagicMenu 0x3FE
0x57CEA0: 同形, push 0x3FF                                                                            ; → MapMenu 0x3FF
```
* 呼び出し元: `0x57B990`（←`0x57C000`）／`0x57C0A0`（←`0x463700`）。両者とも `0x582160(1,0)`（`[0xB3A6E0]` singleton）経由。
* **id は `bl`（呼び出し元が設定する変数）**で渡る → **id の値は呼び出し側の文脈で決まる**（＝id98 もこの型の実行時経路）。
* `mov bl,0x62` は `.text` 全域で `0x5F6320`（head `0x5F6020`）の1件のみで、**この系には無い**。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-589-1** | **id98 の実行時発生源＝「要素→親メニュー通知」の中央ループ**の特定: `0x579970`（メニューマネージャ tick）／`0x5821F0`（frame 更新・FindMenu 7連発）／`0x5A5E00`〜`0x5A6040`（HUD 通知サブハンドラ）の全命令を走査し「要素クリック→対象メニュー vtable[3] へ push id」を抽出 | **中**（V-573-1 を再定義して継承） |
| V-589-2 | `0x5898F0`（子要素探索 `[+0x10]` チェーン）と `0x589D60`（`call [vt+0xc]` の戻りを `cmp eax,0x386`）の役割区別＝**notify が id を返す用途か** | 低 |
| V-589-3 | `0x5A7CD0` の `mov ebx,[edx+0x1c]`（`[elem+0xc]→[+0x1c]`）の用途（要素のコールバック or id フィールド） | 低 |

---

## 5. probe 一覧

**Lane B**: `v600_1`〜`v600_21`（`0x5CC200` 全域 linear（`ret 8`／`[ebp+8]`）／`0x5CC200` の E8 caller 0件＋他11 notify エントリの E8 caller 0件／`0x5CC200` dword の `.rdata 0xA6D398` 1件／`call [reg+0x0c]` 247件の帯域分布（UI 0件）／**真の notify ディスパッチ 695 サイトの機械抽出と id 引数の全列挙**／`push 0x40C` 20件の前方45命令走査／汎用 notify ヘルパ `0x57CDE0` 系の同定）。全 EXIT=0。
