# v352 — #123 裁定: RaceSexMenu 名前入力（race_name / id=10）のエンジン側保存先

日期: 2026-11-11（ReverseEngineering レーン・セッション ses_e55f5e1e1ffe の継続）
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（原本・読み取り専用）
走査スクリプト: `%TEMP%\opencode\v352_20.py .. v352_32.py`（出力 `.out` 同梱）。共通ヘルパ `v307_common`（capstone・HEAD キャッシュ）。

## 0. 結論（依頼①②③への裁定）

| 依頼 | 裁定 |
|---|---|
| ① id=10 処理ハンドラ | **[確定]** RaceSexMenu メインハンドラ `0x5CC200`（ret 8・arg1=通知 id）内の分岐 `cmp ebx,0x0A @0x5CC8A3` → `call 0x5C2730`（UI 更新）＋ `call 0x5C30C0`（エンジン commit） |
| ② 名前のエンジン側保存先 | **[確定]** `g_thePlayer[0xB333C4] → vtbl+0x170 仮想call → 返りobj+0xA4` の BSString スロット。`0x5C30C0 @0x5C3103` で `lea ecx,[edi+0xA4] → call 0x4028D0(str,0)`（string assign）。**TESFullName form 名への chargen 帯直接書出は [確定負]** |
| ③ Done(90) のコミット経路 | **[確定]** 分岐入口 `0x5CC25D`（je 0x5CC8A3 通過後に `call 0x5C30C0` で commit）→ `cmp ebx,0x5A @0x5CC277` → 一時 string assign（`[0xB38F70]`→`0x4028D0`）→ `call 0x5C3110`（key→メンバー対応表書出: `0xB38F70→[menu+0x40]`／`0xB38F78→[+0x44]`／`0xB38F88→[+0x48]`）→ 整合比較 → `0x5CCA77` name 検証 → 成功時は race 説明メッセージ（`0x58CED0(0xFB0)`）のみ |

## 1. id=10 / Done(90) 通知ディスパッチ（0x5CC200）

- v352_21/v352_22: slot01 `0x5C26D0 = SetElement(id, ptr)`。`id==0x0A → [this+0x30]`（race_name 要素）、`id==0x5A(90) → [this+0x3C]`（Done ボタン）。id 1/2/20/21 の格納先も対応付き。
- v352_30 (c1/c2):
  - `0x5CC8A3: cmp ebx,0xA → call 0x5C2730（UI）→ call 0x5C30C0（commit）→ 0xB38FF0/0xB38F80 の一時 string assign（0xFA8 系）…`
  - `0x5CC25D`（分岐合流）: `0x57DD90(0)`（editor 非表示）→ **`call 0x5C30C0`** → `cmp ebx,0x5A` 一致で ↑の Done 書出列。
  - つまり **id=10 通知と Done(90) 通知の両方が 0x5C30C0 を経由**する（0x5C30C0 = commit 本体）。
- v352_32: `callers_of 0x5C2730 = {0x5CC8AE}` のみ（id=10 分支専用）。第3トリガ `0x5C3DD0`（下 §4）は静的呼び出し元ゼロ＝**vtable 経由**（RaceSexMenu vtable 0xA6D38C の slot、focus-lost 系）。

## 2. commit 本体 0x5C30C0（依頼②の中核）

v352_28 で全文確認。逐語デコード:

```
5C30C0: esi = this（RaceSexMenu）
5C30CA: eax = call 0x580120([this+0x8EC])    ; editor ライブ text（BSStringText get）
5C30D8: call 0x58CED0([this+0x30], 0xFDE, eax) ; 要素 0xFDE へ text 設定（UI ミラー）
5C30DD: ecx = [0xB333C4]                      ; g_thePlayer
5C30EB: edi = call [ecx.vtbl+0x170]           ; player → form データ obj（仮想）
5C30F5: eax = call 0x438DB0(editor)           ; = [editor+0x18]（text data ptr）
5C30FD: lea ecx, [edi+0xA4]                   ; ★保存先スロット
5C3103: call 0x4028D0(dst=+0xA4, src=editor text, len=0) ; string assign（0x4028B0=strcpy, 0x401F00=alloc）
```

→ **名前のエンジン側保存先 = `g_thePlayer → vtbl+0x170 → [obj+0xA4]`（長さ u16[+0xA4+4]、容量 u16[+0xA4+6]、buf [+0xA4+0] の BSString）**。[確定]

- 0x4028D0 の body（v352_29a）: `[edi]=buf / [edi+4]=len / [edi+6]=cap` を管理、不足時 `0x401F00` で再確保 → classic string assign。総 callers=557（うち chargen 帯は多数＝UI 文字列群）。
- **TESFullName（form 名）の chargen 帯直接 store は [確定負]**:
  - v352_25: `call 0x9832E6`（TESForm/TESFullName TD cast）全169サイトの直後 store 走査 → chargen 帯（0x5C0000–0x5E0000）で `[obj+4]` store **0件**（全て ESM/form 構築系）。
  - v352_27: `.text` 全域で「`[x+4]` dword store + 48B 以内の `[x+8]` u16 store」= SetName 形式を機械走査 → 70件、**全て 0x40xxxx–0x51xxxx の form 構築/ESM 系**（例 0x409A4F・0x412B11・0x46A772）。chargen 帯ヒットなし。
  - 0x5C30C0 以外の chargen 帯で `lea ecx,[reg+0xA4] → 0x4028D0` はこの形のみ＝commit は 0x5C30C0 に集約（0x5C3103 の site 自体は lea パターン走査の mask バグで再走査は未実施。直接 body 確認で代替済み）。

## 3. 表示・反映経路（name の読み側）

- `0x4DA2A0` = **form 名 getter**: `mov ecx,[ecx+0x1C]` → `call 0x9832E6`（TD cast）→ `mov eax,[eax+4]`、NULL 時 `0xA2F7EC`（空 string）。callers=244（うち chargen 帯 6: `0x5CBC00/1D/3A/48・0x5DBB0A・0x5DC07F`）。v352_31。
- `0x5DBB00`（v352_26/27）= **engine→UI 反映関数**: `0x5DC07F` で `call 0x4DA2A0 → cmp byte[eax],'-' → je 0x5DC4F8（早期 return）`、`call set(0xFAF, name)`。**name の engine 書出ではない**と判定（0x4D9890 も `0x4D98E0` の ret 4 群と並び GetNameLength 系＝読み専用 getter。唯一 caller `0x4DE9D9` は form タイプ [eax+4]（0x12..0x1B）ディスパッチ内の length 評価・v352_28b/29c）。
- id=10 UI 側（v352_28）: `0x5C2730` = editor([this+0x8EC]) に `0x57FF20`（text get 0xA2F7EC 既定）・`0x57D2D0`（配置）・`0x57DD90(1)`（表示）— エディタを race_name 要素位置へ出す処理。テキストの engine 反映は §2 の 0x5C30C0 が唯一。

## 4. 触发子（commit を呼ぶ3経路）

1. **id=10 通知**（0x5CC8A3 分支）→ 0x5C2730 + 0x5C30C0
2. **Done(90) 通知**（0x5CC25D→0x5CC272）→ 0x5C30C0 + 0x5C3110 + 検証 0x5CCA77
3. **editor focus-lost 系** `0x5C3DD0`（v352_30d）: `0x57D2F0`（editor focus テスト）→ `0x57FF50`（editor text set）→ `0x5C30C0`（commit）。vtable 経由（静的 callers なし）。

## 5. 残した未確定（次依頼へ）

- **[未確定1]** `vtbl+0x170` 返り obj の正式型名（TESNPC data 連鎖か PlayerCharacter 内 form ptr か）。+0x104（0x5E3320）/+0xE8（0x5EA720）併用から「player → data obj」の同型は明瞭だが、型名確定は Ghidra アノテーション/実機（#82 ゲート）待ち。
- **[未確定2]** `+0xA4` への commit から form 名（TESFullName `obj+0x1C→cast→+4`）/ セーブ書出（s58_save.md L107: 0x4DA2A0 連鎖 → 0x464C04 以降）への同期経路。静的には chargen 帯に直接 store がない以上、**セーブ時 getter が同一オブジェクトの name を読む**形で結合するが、同期点（0x5C3110 の [menu+0x40] 群が Done 時の player 反映候補）は複数 .ess 検体依存のため実機側 #82 で確認。
- **[未確定3]** RaceSexMenu vtable slot04..09/14（NULL／未参照）の用途。slot01=0x5C26D0(SetElement)/02=0x5C2790/03=0x5CC200(main handler)/10=0x5C2870/12=0x5C3DD0(focus-lost commit)/13=0x5C40C0? getter(getName)/11=0x5CE570。slot12 本文（v352_30）＝ commit トリガ群の中核。
- UI→engine の name 書出は「0x5C30C0 経由で player data obj+0xA4」に**集約**という裁定は3経路全一致で [確定]。ただし Done(90) の `0x5C3110`（key→member 対応表 `0xB38F70/78/88 → [menu+0x40/44/48]`）が「editor → player 反映」の**第二の書出候補**。本文は v352_28 で走査したが、各 entry が何を指すか（race / sex / 派生 flag）は vtable slot03 呼び筋からの追跡が要り [#124] の page 表と併せて次回。

## 6. 成果物・判定語彙の対応

- v352_20: RaceSexMenu vtable 0xA6D38C 確定（15 slot・slot13=0x40C getter）[確定]
- v352_21: SetElement(id→slot) 対応 [確定]
- v352_22: 通知 id ディスパッチ（cmp ebx,0x5A @0x5CC277 / cmp ebx,0xA @0x5CC8A3）[確定]
- v352_23: Done 分岐 body＝0x5C3110 経由の key→member 対応表＋整合比較＋name 検証 [確定]、id=10 分岐＝0x5C2730（UI 配置）[確定]
- v352_24: 0x4DA2A0=form 名 getter [確定]、0x4D9890=読み専用 getter [確定]
- v352_25: chargen 帯の TESFullName store 0件 [確定負]
- v352_26/27: 0x5DBB00=engine→UI 反映のみ [確定]、SetName 形式70件=全て form 構築系 [確定]
- v352_28: commit 本体 0x5C30C0 の保存先=player vtbl+0x170 obj+0xA4 [確定]
- v352_29: 0x4028D0=string assign・557 callers [確定]
- v352_30: 触发3経路 [確定]、0x438DB0=[editor+0x18] getter [確定]
- v352_31/32: 補助確認（strcpy helper・呼び出し元表）

関連: #97（v351 解析メモ）／#101（仕様裁定材料）／#124（同レーン次依頼：page 対応表）／#128（静的残件束・優先順 #123→#124）／#82（実機ゲート・本件の [未確定2] 参照）
