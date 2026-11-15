# v370 — RaceSexMenu 名前入力の検証ルールと入力制約の裁定（#144）

- 採番: **v370**（#144 専用、probe prefix v370_1..v370_9）
- 対象: Oblivion.exe v1.2.0416 ＋ bsa 抽出済み `menus\chargen\race_sex_menu.xml`（705行）／`menus\Dialog\TextEditMenu.xml`
- 依頼: ai-todo #144（①エラー通知 0x5CCBB7 の帰結 ②空文字パス 0x5CCADF の挙動 ③最大長 ④文字種制限 ⑤検証対象文字列の正体）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または原本 XML のみ。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| ① エラー `0x5CCBB7` の帰結 | **[確定(命令)]** | **メッセージ表示（`0x579C10`）→ 画面遷移 tail `0x5CD9C5`（vfunc+0x154→`0x7C5E70`）を経由せず `ret 8` で即リターン = Done(90) はブロック（画面残留・遷移しない）** |
| ② 空文字パス `0x5CCADF` | **[確定(命令)]** | 空は「strcmp/フラグ検査へ**通過**」（ループ先頭脱出）→ **フラグ=1 残存でエラー経由 → 同じくブロック**。何もせず通過でも既定名の自動適用でもない。エラー文は `sNoNameCharacter`「You must first enter a valid name.」 |
| ③ 最大長 | **[部分確定]** | **`maxchars` 機構は EXE/XML 全体に不在 [確定(否定側)]・char 数の明示上限なし [確定(否定側)]**。実質制約＝**スタックバッファ 0x408(1032B・0x57FF50)** と**幅検証 callback `0x57DE00`（測定幅 vs `[this+0xc]`・超過で insert キャンセル）[推定]** |
| ④ 文字種制限 | **[部分確定(否定側)]** | 挿入パスの唯一の callback `0x57DE00` は**幅検証のみで文字種フィルタの形跡なし**。名前バンドの制約は「空/全空白拒否＋プレースホルダ一致拒否」の**2つだけ**。記号・数字・非 ASCII の拒否は EXE に不在 [否定側] |
| ⑤ 検証対象文字列の正体 | **[確定(登録)]** | **`[0xB39440]` = GMST `sEnterName` = "Enter character name."（＝プレースホルダ！）／`[0xB39438]` = GMST `sNoNameCharacter` = "You must first enter a valid name."**。v352 の「既存名」解釈は**訂正**。port 既定名 "Adventurer" の正典根拠は不在 [未確定] |

---

## §2 証拠

### §2.1 検証バンド `0x5CCA77` の全命令（v370_1）
```
5CCA77: ecx=[esi+0x30](race_name要素); push 0xFDE
5CCA7F: mov byte [esp+0x16], 1        ; ★フラグ初期化=1（push直後＝基準esp+0x12）
5CCA84: xor edi, edi
5CCA86: call 0x588C10                 ; get 0xFDE 文字列
5CCA99: sub eax,edx
5CCA9B: je 0x5CCADF      ; ★空文字 → ループを飛ばして strcmp/flag 検査へ
ループ 0x5CCAA0..0x5CCADD:
  5CCAAD: cmp byte [eax+edi], 0x20
  5CCAB1: je  0x5CCAB7                ; スペース → store スキップ
  5CCAB3: mov byte [esp+0x12], bl     ; ★非スペース発見 → フラグ=0 (bl=0)
  ... edi++ / strlen 比較 / jb loop
5CCADF: edi=[0xB39440]; strcmp(name, edi); test eax,eax; je 0x5CCBB7  ; ★一致=エラー
5CCB04: cmp byte [esp+0x12], bl(0); jne 0x5CCBB7 ; ★フラグ1残存(=非スペース無し) → エラー
成功: a/e/i/o/u/A/E/I/O/U 判定 → [0xB38660]/[0xB38668] 選択 → format '%s %s?'(0xA6B4D8)
      → 0x579C10(callback 0x5C2BA0=fn 0x5C2B50, FindMenu(0x40C)ログ系) → jmp 0x5CD9C5
5CCBB7: (エラー) push…call 0x579C10(sNoNameCharacter) → add esp,0x14 → SEH復元 → ret 8  ★tail不経由
5CD9C5: (成功 tail) [0xB3B4C9/C8]=0 → vfunc+0x154([0xB333C4]) → 0x7B4280 → 0x7C5E70 → 0x7C7EB0 → ret 8
```
- **フラグのスロット同一性**: 初期化は `push 0xFDE` の**直後**（esp−4）に `[esp+0x16]`、ループ store は call **後**（esp 復元）に `[esp+0x12]` ＝**同一スロット（基準 esp+0x12）** [確定(命令)]。bl は Done パス `0x5CC28F: xor ebx,ebx` で0・バンド内に bl 再書出なし → **bl=0 確定**。
- **flag 意味**: 初期1 → 非スペース発見で0 → 検査 `!= 0 → エラー` ＝ **「スペース以外を1文字も含まない（空/全空白）ならエラー」**。逆に **空白混入（"John Smith 等）は flag=0 で通過 [確定(命令)]**。
- **band 入口は Done のみ**: `0x5CC277: cmp ebx,0x5A(90); jne 別パス` → `0x5CC2D0: jnp 0x5CCA77` の1エントリのみ（v370_5(b) 全 xref 走査）[確定]。

### §2.2 文字列の正体（⑤）— static 登録で [確定(登録)]
- **`[0xB39440]` = `sEnterName` = "Enter character name."**（登録 `0x9F7B30 区`: `push 0xA63B00('Enter character name.'); push 0xA63AF4('sEnterName'); mov ecx,0xB39440; call 0x419A70`）→ strcmp は「**入力名 == プレースホルダ**」を拒否。**v352 §5.1 の「既存名照合」は誤読として訂正**。
  - **フィールド配置の実証（v370_10）**: `0x419A70` は `[esi+4]=arg1(name)`（`419AA0`）／**`[esi]=arg2(value)`**（`419AA3: mov dword [esi], ecx`）→ **読取 `mov edi,[0xB39440]` は value（プレースホルダ "Enter character name."）を指す** [確定(命令)]。name は +4 に格納され strcmp には使われない。
- **`[0xB39438]` = `sNoNameCharacter` = "You must first enter a valid name."**（登録 `0x9F7B1A`: `push 0xA63AD0; push 0xA63ABC('sNoNameCharacter'); mov ecx,0xB39438; call 0x419A70`）＝エラー通知文。
- **`[0xB38660]` = `sConfirmChoice1` = "Are you sure you want to be a"**（登録 `0x9F0DB0 区`）、`[0xB38668]` は同系列 vowel 版 [推定高]。format = **`'%s %s?'`**（0xA6B4D8）→ 「Are you sure you want to be a Nord?」型の確認文。
- race_sex_menu.xml L51 `<string> </string>`（初期空）＋ TextEditMenu.xml に prompt 専用要素 `textedit_prompt` → **sEnterName は prompt 表示用 GMST**、name==prompt 時の誤確定を防御 [推定高]。
- **port 既定名 "Adventurer" の正典根拠は EXE/XML に不在 [未確定]**（本バンドで確認された入力前文字列は prompt のみ）。

### §2.3 空文字パス（②）[確定(命令)]
- `je 0x5CCADF` は「strcmp/flag 検査への**通過**」でありエラー直行ではない → そこから: strcmp("", prompt) は不一致で落ち着く → **flag=1 残存（初期値のまま）→ `jne 0x5CCBB7` でエラー** → ①と同じく **Done ブロック**。
- → **空入力＝「You must first enter a valid name.」を出して遷移しない**。既定名の自動適用・無条件通過は**存在しない [確定負]**。

### §2.4 最大長・文字種（③④）
- **`maxchars` 系文字列は全セクション0件**（v370_3(d)/v370_7(c)）・race_sex_menu.xml/TextEditMenu.xml にも不在 → **maxchars 属性機構なし [確定(否定側)]**。
- 内蔵エディタ `0x57FF50`（TextEditMenu 系・`sub esp,0x408` の 1032B スタックバッファ）: key switch（表 `0x5800F0` 11ケース全 decode）＝ insert/delete/cursor 操作のみで **char 数上限 cmp なし [確定(否定側)]**。実質上限＝**バッファ 0x408 [推定]**。
- 挿入時の唯一の検証 callback **`0x57DE00`（E8 caller は `0x5800B9` の1本・`[ebp+0xC] != -1` のみ）**: `0x5739C0`（テキスト測定）→ `cmp eax,[esi+0xC]` → 超過なら `al=0` で **insert キャンセル（`0x5800C0: je 0x5800D4`）[推定高]**。＝**幅ベース制約（wrapwidth 相当）であって文字数/文字種制約ではない**。
- 名前バンド側の制約＝**「空/全空白拒否」「プレースホルダ一致拒否」の2つのみ**。記号・数字・非 ASCII の拒否は**EXE に不在 [確定(否定側)]**。
- （別物の注意）fn `0x5DC950` に**スペース→'_' 置換＋0x100(256) 上限**ループあり（`0x5DCA78`・trait 0xFAA へ）だが、これは class 系 trait の sanitize で**名前入力系ではない [部分確定]**。

### §2.5 port 実装への含意（#126③ のフォロー）
1. **Create ブロック条件**: 空 or 全空白 or 名前==prompt("Enter character name.") → **「You must first enter a valid name.」表示＋遷移しない**（現 port は無条件成功 → **不一致**）。
2. **空白混入は許可**（"Zara Storm" 可）— v352 の旧記述を訂正。
3. **最大長・文字種の port 検証は原本準拠で不要**（原本に存在しない）・ただし実輸入は 1024B バッファ＋幅検証の実質制約。
4. "Adventurer" 既定名は正典未証 [未確定]。

## §3 残点 **V-370-1..5**
1. **`0x579C10` の modality**（確認文表示が Yes/No 待ちか即時か＝成功 tail `jmp 0x5CD9C5` は無条件・モーダル性は未 decode）【中・#101 の Done 確認フローに必要】
2. **幅検証 `0x57DE00` の上限値 `[this+0xC]` の初期化元**（wrapwidth 425 と一致するか）と返却経路の完全 decode【中】
3. **`[0xB38668]`（sConfirmChoice2 候補）の実文字列**（vowel 版・登録 push のみ未読）【低】
4. **fn `0x5DC950`（space→'_' sanitize）の所属 class**（E8 callers 0x5DCCB1..0x5DCF03＝class_menu 系 [推定]）と trait 0xFAA の意味【低】
5. **"Adventurer" の正典性**（EXE/XML に根拠なし＝port 既定名の出所）【低】

## §4 検証・自己批評
- probe: **v370_1..v370_10 全 EXIT=0** ＋ XML 実測2回（race_sex_menu L48-65・TextEditMenu grep）。
- **自己発見と是正**: (i) v370_5 の `\x0F\x8` が Python エスケープエラー → 2バイト pattern 化＋パッチ .py 化（v370_5 は再実行成功）。(ii) v370_3 の byte-pattern 走査で site 内包誤検出 → cover 検証付きに是正。(iii) **v352 との矛盾2件を自己検出・訂正**: 「既存名照合→プレースホルダ照合」「空白混入→空/全空白拒否」（フラグ polarity を push 前後のスロット同一性まで遡って実証）。
- 妥当性チェック: エラー tail 不経由（ret 8）と成功 tail 経由（jmp 0x5CD9C5）の**分岐がバンド出口に唯一存在**し、①の結論と矛盾しない。文字列登録は 0x419A70 パターン（v366 の sCantQuickLoad と同一）で name/value 対が確定。
- docs との不整合: **v352_race_sex_name_flow.md §5.1 の2点（既存名・空白混入）が本裁定と矛盾 → 本裁定を正**（v352 本文は歴史記録として温存、本 v370 §2.1/§2.2 で正式訂正）。
- L1 制約: docs/GT/ESM 変更なし・移植側ソース変更なし。
