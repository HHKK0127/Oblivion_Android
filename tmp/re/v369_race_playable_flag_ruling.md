# v369 — RACE playable フラグのレコード内位置と EXE 供給経路の裁定（#147）

- 採番: **v369**（#147 専用、probe prefix v369_1..v369_17）
- 対象: Oblivion.exe v1.2.0416 ＋ bsa 抽出済み `Data\Oblivion.esm`（Steam版・DISC版と同一: 277,504,985 bytes / SHA 同一サイズ）
- 依頼: ai-todo #147（#146 のブロッカー。①playable フラグのレコード内位置 ②EXE の [TESRace+0x70] 供給経路 ③+0x70 の残りビット ④表示対象の完全な条件）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または原本 ESM 実データのみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| ① レコード側の位置 | **[確定]** | **RACE の `DATA` サブレコード（36バイト）の byte32..35（u32 flags）の bit0** = playable。実データ15件で完全分離: playable 10件 = `0x00000001` / non-play 5件 = `0x00000000` |
| ② EXE の供給経路 | **[確定(命令)]** | TESRace **vtable slot7 = `0x52D800`**（RACE 専用 Load fn）の `DATA` case（`0x52D868`→`0x52D8AC`）が **`push 0x24; lea eax,[ebx+0x50]; call 0x46BDA0`** で **DATA 36バイトを [TESRace+0x50..0x73] へ全体コピー** → byte32 が **[TESRace+0x70] に直接乗る**。ビットセット命令は不要（構造体 memcpy が供給）。既定値 init は ctor 内の別経路で **[未確定]（残点 V-369-4）** |
| ③ +0x70 の残りビット | **[未確定]** | 実データ15件は全て u32 が `0x00000000` か `0x00000001` の2値のみ = **bit1..31 の出現例なし**。既知範囲で意味判明せず |
| ④ 表示対象の完全な条件 | **[確定(命令)]** | `0x5CDAF9: test byte ptr [edi + 0x70], 1` ＋ ノード null チェック（`cmp edi,ebx/je`）**のみ**。性別・他条件の併用なし |

**port への直接示唆 (#146)**: `RaceData` に `DATA`(36) の **byte32 u32 flags** を読み、**bit0** でフィルタする。EyesData/HairData の先行設計（`flags: 0x01 = playable`・esm_reader.h:557/567）と**同一構造**（EYES/HAIR 同様の回答）。

---

## §2 証拠

### §2.1 レコード側（①）— 実データ差分 [確定]
- **サンプル完全性**: GRUP RACE @0x1DAC7 size=0x883D → **RACE レコード15件のみ**（v369_17 でグループ境界を直走査し EDID 全列挙: Sheogorath, GoldenSaint, DarkSeducer, VampireRace, Dremora, Argonian, Nord, Breton, WoodElf, Khajiit, DarkElf, Orc, HighElf, Redguard, Imperial）。ファイル全走査でも同じ15件（v369_1/2）。
- **DATA は全15件に存在（36バイト一意）**: `DATA sizes: {36: 15}`。他のサブレコード（DNAM/PNAM/UNAM/VNAM/XNAM 等）は**欠落あり**（DNAM=12件, PNAM=7, UNAM=7, VNAM=5, XNAM=9）→ **playable を表せるのは DATA のみ**（存在一致でもない）。
- **byte32..35 の u32 で完全分離**（v369_2）:
  ```
  NONP Sheogorath   word[8]=0x00000000     PLAY Argonian    word[8]=0x00000001
  NONP GoldenSaint  word[8]=0x00000000     PLAY Nord        word[8]=0x00000001
  NONP DarkSeducer  word[8]=0x00000000     PLAY Breton      word[8]=0x00000001
  NONP VampireRace  word[8]=0x00000000     PLAY WoodElf     word[8]=0x00000001
  NONP Dremora      word[8]=0x00000000     PLAY Khajiit     word[8]=0x00000001
                                          PLAY DarkElf     word[8]=0x00000001
                                          PLAY Orc         word[8]=0x00000001
                                          PLAY HighElf     word[8]=0x00000001
                                          PLAY Redguard    word[8]=0x00000001
                                          PLAY Imperial    word[8]=0x00000001
  ```
  **15/15 一致・誤分離ゼロ**（#146 の emu 観測「Sheogorath が既定 index0 で並ぶ」= 本フラグ欠落と矛盾しない）。
- DATA のバイト構成（実データ）: byte0..15 = u16×8（skill boost 群）／byte16..31 = f32×4（weight/height 系）／**byte32..35 = u32 flags**。

### §2.2 EXE 供給経路（②）— [確定(命令)]
- **TESRace vtable = 0xA548FC**（RTTI `.?AVTESRace@@`・b07 表 1466）の **slot7 = `0x52D800`**（v369_12 で全55スロット実測）。
- **`0x52D800` = RACE 専用 Load fn**（v369_10/13）:
  - prologue 正常（`push ebp; mov ebp,esp; sub esp,0x30`）・gate `call 0x4511C0; cmp eax, 9; je`（`0x4511C0` = レコード型判定系 [推定]・E8 callers 73）。
  - **FourCC dispatch が RACE のサブレコード集合と一致**: `CNAM`/`DATA`/`NAM0`/`NAM1`/`FGGA`/`DESC`/`EDID`/`FULL`/`SPLO`/`FGTS`/`ATTR`/`FGGS`（v369_10/13 実測）。**`ATTR`/`FGTS`/`FGGS`/`NAM0`/`NAM1` は RACE 固有** → これが RACE の Load であることは FourCC で確定。
- **供給命令（決定打）**（v369_10(c)）:
  ```
  52D868: cmp esi, 0x41544144 ('DATA')
  52D86E: jg  0x52d8bf
  52D870: je  0x52d8ac          ; DATA case
  ...
  52D8AC: push 0x24             ; size = 36
  52D8AE: lea eax, [ebx + 0x50] ; dest = TESRace + 0x50
  52D8B1: push eax
  52D8B2: push edi              ; レコード読取コンテキスト
  52D8B3: mov ecx, ebx          ; this = TESRace
  52D8B5: call 0x46BDA0         ; サブレコード読取ヘルパ（E8 callers 30＝汎用）
  52D8BA: jmp 0x52dd4c
  ```
  → **DATA 36バイトが [TESRace+0x50..0x73] へ全体コピー**され、**byte32 = [TESRace+0x70]** に flags が直接配置される。したがって walk の `test [edi+0x70],1` は **DATA.flags.bit0 を読んでいる**ことになる（レコード⇔メモリ対応が1対1で確定）。
- **構造の裏付け**: b13（TESRace コンポーネント: TESFullName +0x18 / TESDescription +0x24 / TESSpellList +0x2C / TESReactionForm +0x40）→ コンポーネント終端 ≒ +0x50 で、**DATA の格納先 +0x50 は TESRace 固有フィールドの先頭と整合**。
- **既定値（未設定時）**: ctor `0x52C870`（slot46・vtable store `0x52CC12` で TESRace ctor を確定）内に `[ebp+0x70]` への書出が1件ある（`0x52CDC4: mov dword [ebp+0x70], eax`・eax = 直前 `call 0x519D20` の結果）。ただし `0x519D20` は **formType==0x23 のとき `[ecx+0x28]&1`、それ以外は -1** を返す別系統の値で、ebp/esi の identity が未解読 → **既定値の正体は [未確定]（残点 V-369-4）**。+0x70 の zero-init 明示は ctor 内に未発見（v369_13(e)）。**ただし実データの15件は全て DATA を保持するため、vanilla では常に flags がセットされる [確定(実データ)]**。
- `[TESRace+0x70]` の**全体コピー**は `0x52C5E6: mov edx,[esi+0x70]; mov [edi+0x70],edx`（v369_14(b)）にも存在 → race 間の複製系で転送される。

### §2.3 walk の完全条件（④）— [確定(命令)]
`0x5CDA30` の race list ウォーク（v369_3(c) 全命令実測）:
```
5CDAD0: mov ecx, [0xB33A98]     ; フォームコンテナ
5CDAD6: add ecx, 0x44           ; RACE サブリスト先頭
5CDAE7: loop:
5CDAF3: mov edi, [ecx]          ; node value
5CDAF5: cmp edi, ebx            ; null チェック
5CDAF7: je  next
5CDAF9: test byte ptr [edi+0x70], 1   ; ★playable bit のみ
5CDAFD: je  next                       ; 0 ならスキップ
5CDAFF: add edx, 1                     ; カウント
5CDB06: mov ecx, [ecx+4]               ; 次ノード
  ...
```
- **条件 = ノード非 null ＋ bit0 のみ。性別対応・XNAM・他サブレコード条件の併用なし [確定(否定側)]**。
- 同型の bit0 テストは他に `0x5C82A5`（別カウンタ）にも存在（v369_8(b)）＝ **bit0 が唯一の表示フィルタ**。
- 選択後の分岐（`0x5CDC33` 以降: `[local+0xE8]=edi`・name/desc 反映）は表示後の処理で、フィルタ条件ではない。

### §2.4 残りビット（③）— [未確定]
- 実データ15件の flags は `0x00000000` / `0x00000001` の2値のみ → **bit1..31 は出現例ゼロ**。既知範囲で意味不明。
- EXE 側で `[reg+0x70]` の **bit1+ を test する命令は不在**（`test …,1` のみ・imm 2/4/8 等の検索は 0x98xxxx 帯の別クラスのみ＝v369_8(b)）。

### §2.5 用語・整理（教訓含む）
- **TES4 レコード/GRUP ヘッダは20バイト**（TES5 の24ではない）— v369_1 の初回 walk が破損した原因（24前提で type 境界を踏み外し→ストップ）。20バイトに是正して15件全取得。
- TESRace ctor = `0x52C870`（slot46）／deleting dtor = `0x52D0E0`（slot4・`call 0x52CBE0`＋delete パターン）。
- `0x46BDA0` = 汎用サブレコード読取ヘルパ（E8 callers 30・`cmp ax,cx` のサイズ検証付き＝v369_10(b)）。

---

## §3 残点 **V-369-1..5**
1. **ctor の [+0x70] 初期化経路の正体**（`0x52CDC4` で `0x519D20` の結果を格納・ebp/esi identity 未解読）＝ DATA 不在時に playable がどう既定化されるか【中・port のフォールバック実装に必要】
2. **Load fn の gate `cmp eax,9` の 9 の意味**（`0x4511C0` 戻り値＝レコード型 enum か formType か）【低・FourCC で RACE は確定済み】
3. **`0x519D20` の formType 0x23 と `[form+0x28]` bit0/bit4 の正体**（RaceSex UI 帯 0x522230 系で使用＝v365 §2.7 と同一系統）【低】
4. **DATA byte0..15 の u16×8 と byte16..31 の f32×4 の厳密な意味**（skill boost 8値/weight-height 4値の対応）【低・playable 判定には無関係】
5. **bit1..31**（③）— 実データに出現例なし・mod エンリッチの可能性【低・依頼③の答案＝[未確定] で完結】

## §4 検証・自己批評
- probe: **v369_1..v369_17 全 EXIT=0**。
- **自己発見と是正**: (i) 初回 walk がレコードヘッダ24バイト前提で破損（0件取得）→ TES4 は**20バイト**と実データから特定して是正。(ii) PowerShell の `<` 禁則で inline patch が ParserError → パッチを .py ファイル化して回避。(iii) GRUP 直走査（v369_17）でサンプル完全性（15件）を独立検証。
- 妥当性チェック: ①実データ15/15完全分離 ⇔ ②構造体 memcpy の対応（+0x50+32=+0x70）⇔ ④walk の test bit0 — **三点が互いに独立に一致**。b13 のコンポーネント終端 +0x50 とも整合。
- docs との不整合: なし（本裁定は新規。#146 の修正材料に直接供給）。
- L1 制約: docs/GT/ESM 変更なし（ESM は読み取りのみ・移植側ソース変更なし）。
