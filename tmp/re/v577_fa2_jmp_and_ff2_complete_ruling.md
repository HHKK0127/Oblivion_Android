# v577 — V-576-1 の (a)(b) 解消: `0xFA2` 3件目は `jmp` オペランド・`ff /2` 全エンコードで UI=0件、および probe ハーネス2回目の消失と復元（ai-todo #166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v577_1〜v577_3**・3本 全 EXIT=0、加えて `v307_verify.py` 復旧検証）。
先行: `tmp/re/v576_entity_mechanism_and_open_contradiction_ruling.md`（3命題の矛盾＝V-576-1）
判定: `[確定(命令)]` / `[部分確定]` / `[未確定]（継続）`

---

## 1. 結論

| 問い | 判定 | 結果 |
|---|---|---|
| (a) 3件目の `0xFA2` バイト列（`0x42703E`）の実体 | **[確定(命令)]** | **`0x42703D: e9 a2 0f 00 00` ＝ `jmp 0x427FE4`（rel32 のオペランド内部）** — head `0x426A90` から**線形 decode 368 insn 目で到達**（prev=`0x427038 call 0x423720`）。**`0xFA2` 即値ではない**（`a2` を opcode `mov al,moffs32` と誤 decode したのはオペランド内部を起点にした偽命令＝v577_1 の back=0 decode が誤、v577_2 線形が正）。→ **`.text` の実質 `0xFA2` 即値は2件のみ**（entity 登録 `push 0xfa2` @`0x58A9A6`／open 内 `cmp [esp+4],0xfa2` @`0x591C54`）。 |
| (b) `call [mem]`（`ff /2`）の全エンコードで UI に slot5 呼び出しはあるか | **[確定(命令)]** | **全 modrm（reg==010）×SIB×disp8/disp32 の完全列挙＋capstone 検証 → validated 282件（従来281＋新規1件 `0x714240`＝NIF/ストリーム系・UI 外）**。**UI-range（head 0x40-0x6E）は0件** → **メニュー/TileMenu 系コードは indirect call 形式で slot5 open を呼ばない**。従来走査の欠落（SIB mod=01 `ff 54 .. 14` と mod=10+`ff 94`）を補完済み。 |
| 矛盾の現状 | **[未確定]（継続・絞込み済み）** | 3命題＋本裁定の2命題が同時に成立: ①open 成功＝a1==0xFA2 固定 ②`0xFA2` 即値＝2件のみ ③`E8/E9` 直接0件 ④**`ff /2` 全エンコード UI=0件** ⑤`0x587D20` は open 内1件のみ → **「open を呼んで 0xFA2 を渡す場所」は既知の全系統に存在しない**。残る手は **(c) 引数規約の再検証**（`[esp+4]`=a1 の確定と、`ret 0xc` の呼び出し側 push 列の実測）と **resolver `0x58B040` の2消費者（`0x58BEE0`/`0x58D2F0`）の後続深掘り＝entity id を push する経路** → **V-577-1 へ**。 |

---

## 2. probe ハーネス2回目の消失と復元 [運用・重要]

* **2026-11-20、`%TEMP%\opencode\v307_common.py` が再び消失**（2026-11-18 に一度消えて復元済み、memory 記録の「以後 probe は最初に Test-Path で存在確認」が発動）。他の probe 235個は残存＝**単ファイルのみが掃除される**型。
* **復元**（memory 記載どおり）:
  * EXE = `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（**7,898,624 B** を実測一致）
  * `find_head` = `tmp/re/cache/named_funcs.txt`（**21,597 heads**・`hexaddr,name` 形式）を **bisect（va 以下の最大 head）** — CC-run フィルタ済みリストのため int3 帯は前 head に吸着（v574 の誤帰属挙動と一致）
  * `md` = capstone x86/32・`sections` = PE32 パース5要素タプル・`find_off`/`bytes_at`/`callers_of`（E8）
* **復旧検証3件すべて OK**: `callers_of(0x4E4A10)=1043`（v546 一致）／`callers_of(0x591C50)=0`（v545 一致）／`find_head(0x42703E)=0x426A90`（v547/v576 一致）。加えて `find_head(0x591C76)=0x591B70`（int3 帯誤帰属の再現＝挙動一貫）と disasm smoke（`cmp [esp+4],0xfa2`）も一致。
* **以後の運用**: probe 作成時に `Test-Path v307_common.py` を最初に実行する習慣（memory の1回目教訓と同）。

---

## 3. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| **V-577-1** | **矛盾解消 (c)＋entity id push 経路**: ①`0x591C50` 呼び出し側の push 列実測（`[esp+4]`=a1 と `ret 0xc` 規約の最終確認・`0x58B040` resolver の id を直接 push する場所＝`0x58D2F0` 後続の完全 decode）②`0x58BEE0`（連番テンプレ展開）の open 経路接続 | **中**（V-576-1 を再定義して継承） |
| V-577-2 | 新規 hit `0x714240`（`call [reg+0x14]` の SIB 形式・0x71 帯）の同定 | 低 |

---

## 4. probe 一覧

`v577_1`（`0x42703E` の境界探索＝back=0 偽命令検出・head ウィンドウ）／**`v577_2`（head `0x426A90` から線形368 insn で `0x42703D: jmp 0x427FE4` を到達＝決定打）**／**`v577_3`（`ff /2` 全 modrm+SIB+disp 完全列挙＝raw 6,945 候補→capstone 検証282件・UI=0件）**／`v307_verify.py`（ハーネス復旧検証3件）。全 EXIT=0。
