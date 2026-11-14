# v365 — RaceSexMenu で選択した種族/性別のエンジン側保存先の裁定（#134）

- 採番: **v365**（#134 専用、probe prefix v365_1..v365_10）
- 対象: Oblivion.exe v1.2.0416（ImageBase 0x400000、.text 0x401000..0xA27E00）
- 依頼: ai-todo #134（#123 のスコープ拡張。#99 欠落 C: ポートの Player に race フィールドなし・Create 後も種族が保存されない）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。docs/GT/ESM は触らない。

---

## §1 裁定サマリ（依頼の3候補）

| 候補 | 判定 | 根拠（要約） |
|---|---|---|
| **(a) PlayerCharacter の field offset に直接書き込む** | **[部分確定]** | メニュー側に `mov [engine-object+off], 値` 形式の race/sex 保存は**1件も存在しない**（stores 全走査＝§2.1）。保存はすべて **vfunc 呼出経由**。vfunc impl（0x602A70/0x5F5320/0x5F94B0/0x66A400）の冒頭 window にも永続 store は **A+0x644 のみ**。**実 field offset は下流 callee（0x65BA90/0x602050/0x6A2280/0x663C50）内にあり未特定＝残点 V-365-1**。 |
| **(b) TESNPC/TESRace フォーム参照として保持される** | **[部分確定]（本命）** | ①適用の中核 `0x602A70`（B→vfunc+0x5C impl）は **`npc = this->vfunc+0x170()` で base フォームを取得し `cmp [npc+4],0x24`（formType 36 gate）＋`[npc+0x104]==4` gate** を通過してから rebuild する（#123 と同一チェーン）＝§2.3。②race は **フォーム参照として流れ**: race handler の `A::0x66A400(A, race)`（race を引数に取る直接 setter・E8 caller は 0x5D5AA1 の1本のみ）と `[local+0xE8] = race form` store（0x5CDD59）、revert は `A->vfunc+0x170()` → `[form+0xE8]→+0xA8` のリストを `Characters\Eyes\EyeDefault.dds` 照合で走査＝§2.5。 |
| **(c) chargen 完了時の別メニュー（0x416 疑い）でコミット** | **[確定負]** | Done(id90) 帯の engine 書出＝**0件**（v352 再掲）。適用は**選択時に live**（sex toggle と race click の直後に適用チェーンが走る＝§2.2）→ EXE 内に「完了時コミット」は存在しない。※ESM chargen quest 側の SetRace は人間ゲート（#130-1）で未確認＝EXE 外。 |

**総合**: 保存先は「**base フォーム（formType 0x24）＋グローバル A（0xB333C4）経由の live 適用**」であり、**(b) フォーム参照保持を採用 [部分確定]**。実フィールド offset と A::0x66A400 の最終行き先が残点。

---

## §2 証拠

### §2.1 メニュー側に直接書出なし [確定]
- `0x5CDA30`（race 選択/表示ハンドラ）の stores-only 全走査（0x700）＝**string 構築（`[ecx]=ebx` 系）・stack・flag（0xB33D80）・グローバル 0xB3BB10 のみ**。race/sex を engine field へ書く `mov [reg+off], form` は**0件**（v365_4(e)）。
- `0x5C2F20`（sex toggle）も同様に direct store なし。flag 0xB33D80 の bracket と vfunc 連打のみ（v365_4(a) unfiltered）。
- Done(id90) 帯＝**engine 書出 0件**（v352 §5 の再掲）。

### §2.2 適用は選択時に live [確定]
**Sex toggle `0x5C2F20`**（v365_4(a) unfiltered）:
```
5C2F5D: call 0x550240            ; formID('%08X')+'ModBody/ModUpperBodyMale/Female' を [0xB35300] へ登録
5C2F67: mov byte [0xb33d80], 1   ; sex-change 中フラグ ON
5C2F6E: mov ecx, [eax+0x58]      ; B = [A+0x58]（A=[0xB333C4]）
5C2F79: call eax                 ; B->vfunc+0x5C()        [impl 0x6710B0→0x602A70]
5C2F8E: call eax                 ; B->vfunc+0x31C(1)      [impl 0x5F94B0]
5C2FA1: call eax                 ; B->vfunc+0x318(A)      [impl 0x5F5320]
5C2FB3: call eax                 ; A->vfunc+0x134(0)      [impl 0x4DDFD0・ret4 一致]
5C2FCC: call eax                 ; A->vfunc+0x130()       [impl 0x4DDF90・ret4 一致]
5C3045: mov byte [0xb33d80], 0   ; フラグ OFF
```
**Race click の尾部 `0x5CDE5D..`**（v365_6(a) aligned）:
```
5CDE5D: mov byte [0xb33d80], 1
5CDE64: call 0x659f30            ; finder オブジェクト構築（vtable 0xA732FC・ctor 内で [esi]=0xa732fc）
5CDE79: call edx                 ; ebp->vfunc+0x84(finder, 1)   （ebp=[edx+0x1c]・別クラス＝残点 V-365-3）
5CDE88: call 0x6ff420            ; finder->method("Player")
5CDE99: call edx                 ; finder->vfunc+0x58("Camera01") → eax
5CDEA1: mov [0xb3bb10], eax      ; ★0xB3BB10 = Camera01 ノードのキャッシュ（保存先ではない＝§2.6）
5CDEAE: call edx                 ; A->vfunc+0x148()  [impl 0x65B750・0xD8 ローカルの大関数]
5CDEB0: call 0x5c2f20            ; ★共通適用チェーン（sex/race ともここを通る）
5CDEC6: call eax                 ; B->vfunc+0x384(A) → [B+0xD4] = A（impl 0x611EB0: mov [esi+0xd4],eax）＝相互ポインタ
5CDED2: call 0x5c7ba0            ; UI revert/リセット（§2.5）
```

### §2.3 適用の中核は base フォーム経由 [確定(命令)]
`0x602A70`（B→vfunc+0x5C impl、v365_6(e) unfiltered head ＋ v365_4(b) filter）:
```
602A97: mov esi, ecx                      ; this = B
602A99: mov ecx, [esi+0x58]               ; B の子オブジェクト
602AA2: mov edx, [eax+0x318] / push esi / call edx   ; child->vfunc+0x318(B)
602AB7: call 0x65ba90                     ; (this, arg1, arg2) へ委譲
602ABC: mov edx,[esi]; mov eax,[edx+0x170]; mov ecx,esi; call eax   ; ★npc = this->vfunc+0x170()
602AC8: cmp byte ptr [eax + 4], 0x24      ; ★formType 36（TESNPC 系）gate
602AE6: mov al, [eax+0x104] / cmp al, 4    ; ★[npc+0x104]==4 gate（意味は残点 V-365-5）
… 以降 rebuild 大量 call（0x5EF9B0/0x65A2C0/0x8AC0C0/0x6ECC80/0x4C97F0/0x424180/0x4440C0/0x532C80/0x419B30/0x69FD20/0x65D750…）
```
- **vfunc+0x170 = base フォーム取得**は #123（g_thePlayer→vfunc+0x170→TESFullName.name）と同一チェーン。
- 同じ `vfunc+0x170` は **revert `0x5C7BA0` でも A に対して呼ばれる**（`mov ecx,[0xb333c4]; mov edx,[ecx]; call [eax+0x170]` → `[eax+0xe8]→+0xA8` リスト走査、v365_9(d)）。
- impl 3本（0x602A70/0x5F5320/0x5F94B0）の冒頭 0x300..0x500 には**永続フィールド書出なし**（全て local alloc・`[esp+..]`・新規オブジェクト初期化）→ 実保存は更深い callee 内 [未確定＝V-365-1]。

### §2.4 オブジェクト A/B の正体
- **A = [0xB333C4]**（v365_6(c) writer: `0x4067A6`／`0x464553`）:
  - 生成: `new → call 0x66a740 (ctor) → push 1; push 0x14; mov [0xb333c4], eax; call 0x46c300`（**引数 0x14=20 ＝ player 系 FormID 0x00000014 の可能性 [推定]**）。
  - ctor `0x66A740`（v365_9(b)）: `call 0x60e4f0; mov [esi],0xa73a0c; mov [esi+0x18],0xa73a04; mov [esi+0x5c],0xa739bc; mov [esi+0x68],0xa73988` → **vtable = 0xA73A0C（4 つの埋め込み vptr）**。
  - base ctor `0x60E4F0`（v365_10(c)）: `call 0x5e15e0; fstp [esi+0x108]; mov [esi],0xa6fc9c…; **mov byte [esi+4], 0x32**; mov [esi+0x104],0` → **formType byte = 0x32(50)**（[form+4] 判定系と同型）。
  - **A の vtable=0xA73A0C は runtime 整合で確定**: 0x5C2F20 が `A->vfunc+0x134(0)` を `push 0`（1引数）で呼び、slot77 impl `0x4DDFD0` は `ret 4`（1 stack 引数）＝一致。
- **B = [A+0x58]**: 0x5C2F20 が `mov edx,[ecx]; mov eax,[edx+0x5c]` で dispatch → slot23 impl `0x6710B0→0x602A70`（§2.3 の base フォーム取得＝Player 系動作と整合）。**A と B は同一クラス（vtable 0xA73A0C・formType 0x32）で相互ポインタ（A+0x58 ⇄ B+0xD4）** [部分確定]。
- slot impl 表（0xA73A0C、v365_3/8/10）: +0x5C=0x6710B0（→0x602A70）／+0x84=0x4CA690（`([ecx+8]>>17)&1` の bit getter＝**race tail の `ebp->vfunc+0x84` とは不一致→ ebp は別クラス**）／+0x130=0x4DDF90／+0x134=0x4DDFD0／+0x148=0x65B750／+0x318=0x5F5320／+0x31C=0x5F94B0／+0x384=0x611EB0（`[this+0xd4]=arg`）。

### §2.5 race の流れ（フォーム参照）[部分確定]
1. **race list 走査**: `0x5CDAD2: mov ecx,[0xb33a98]; add ecx,0x44`（playable bit 走査＝v353 再掲）。`[0xB33A98]` は form コンテナで、+0x44/+0x3C/+0x8C/+0x2C が型別サブリスト（v365_8(d) で band 内 17 ヒット、lookup 系 `0x447350/0x446AF0/0x447490/0x44D950` の ecx として使用）。
2. **`[local+0xE8] = race form`**: `0x5CDD59: mov [ecx+0xe8], edi`（直後に `edi` が form として `[edi+0x24]` inline vtable オブジェクトへ `vfunc+0x10('DESC', …)` を呼ぶ＝form 確認、v365_4(e)/v365_7 前後）。→ **フォーム参照の保持点候補 [部分確定]（obj の identity は残点 V-365-4）**。
3. **`A::0x66A400(A, race)`**: **race を引数に取る直接 setter**（v365_9(a)）。`call 0x6a2280 → mov [esi+0x644], eax`（**A+0x644 への永続格納＝[推定] 適用済みフォーム**）、`push edi; push 2; call 0x565be0 → call 0x663c50`、`cmp edi,0x15(=21)`（21 skills 連想 [推定]）、`push 0x666890; call 0x579c10`（callback 登録）。**E8 caller は `0x5D5AA1` の1本のみ**。
   - 呼び出し元 `0x5D5A50`（v365_9(c)）: `push …; push 0x408; call 0x589b70; call 0x5898f0` → `mov ecx,[0xb33a98]; call 0x447350`（race lookup）→ `mov ecx,[0xb333c4]; push eax; call 0x66a400` → `push 0xfaf; call 0x58ceb0` / `push 0xfc9; call 0x58ceb0`（**trait 0xFAF/0xFC9＝#133 の StatsMenu AV trait 0xFAF..0xFBE と一致** → StatsMenu 系コード [推定]）。所属 menu id 0x408 は残点 V-365-2。
4. **UI revert `0x5C7BA0`**（v365_7(f)/v365_9(d)）: `esi=this` → `A->vfunc+0x170()` → `eax=[form+0xe8]+0xa8` のリストを walk → `strcmp([entry+0x28], 0xA549D8='Characters\Eyes\EyeDefault.dds')` ＋ `0x51ED80`（**ESM レコード chunk 読取器**: 'FULL'/'DATA'/'EDID'/'ICON' FourCC 判定、v365_9(d2)）→ `menu+0x87c` を 0/既定値へリセット、trait 0..8 を `0x5C3440` で再設定。**＝復元先が base フォーム配下のフォーム参照構造であることを示す（(b) の根拠）**。

### §2.6 「保存先ではない」と判明した候補 [確定]
- **`0xB3BB10`**: race handler は store するが、値は `finder->vfunc+0x58('Camera01')` の戻り＝**NiNode ポインタ**。文字列実測 `[0xA6D448]='Camera01'`、reader `0x667BE0` は `"Missing 'Camera01' on '%s'."` / `'Player1stPerson'` / `'Bip'` を使い float 座標演算（+0x8c/+0x90＝NiPoint3）→ **カメラ再取得用キャッシュ（0xB3BB0C/BB10/BB14 の3本組）**。
- **`0x550240`**: `'%08X'`（formID 書式）＋静的 `'ModBody'/'ModUpperBodyMale'/'ModUpperBodyFemale'` を `[0xB35300]` へ登録＝**モデル差分ノード名の登録（表示側）**。caller は sex toggle（0x5C2F5D）と flag 0xB33D80-gated の model rebuild `0x47BC40`（0xB33C80..8C へ formID キャッシュ）のみ。
- **`0x5C7070`**: UI 適用コア（`0x589B70(0x40C)`・`0x522230`×4（formType 0x23 の bool→float 0.0/1.0）・`0x553B30/0x555A00`）。caller 5 本すべて RaceSex band＝**表示/トグル更新側**。

### §2.7 補助事実
- `0x519D20`（v365_6(d)）: formType 0x23 専用フラグ getter/setter 家族（`[form+0x28]` bit0/bit4、`form+0x24` inline vtable オブジェクトの `vfunc+0x50(0x10)`、switch 0..0x27 → `0x468AD0(form+0x88, idx)`）。
- `0x5E91E0(0x1d, FourCC, flag)`: FourCC `'INVI'/'CHML'` を 0x5F5320（B→vfunc+0x318 impl）から登録（意味は残点 V-365-5）。
- `0x659F30`: finder の mid-function entry（head 0x659C90、caller 5 本: 0x5293FC/0x5CDE64/0x607711/0x60CCAC/0x667E7A）。
- `0x6FF420`: (this, string) を受ける汎用 134 callers（finder への "Player" 指定に使用）。

---

## §3 残点 **V-365-1..5**
1. **実保存 field offset**（中）: `0x602A70` 下流 `0x65BA90/0x602050`・`0x66A400` 下流 `0x6A2280/0x663C50` 内の store。base フォーム（type0x24）のどの offset に race/sex が書かれるか＝**#99 欠落 C の「race フィールド」に直結する最重要残点**。
2. **A::0x66A400 の race 引数の最終行き先と 0x5D5A50 の所属**（中）: `A+0x644=0x6A2280()` 結果の正体、menu id 0x408 の正体（StatsMenu 系 trait 同時更新＝#133 との接続点）。
3. **ebp=[edx+0x1c] の正体**（低）: race tail `ebp->vfunc+0x84(finder,1)` の this（vtable 0xA73A0C の slot33 とは不一致＝別クラス。カメラ/プレビュー系 [推定]）。
4. **local+0xE8 オブジェクトの identity**（低）: `0x5CDD59` の `[esp+0x18]`＝race form の保持主体（menu / chargen state）。
5. **enum・FourCC 名**（低）: formType 0x32(50)/0x24(36)/0x23(35) の enum 名、`[npc+0x104]==4` の意味、`'INVI'/'CHML'`（0x5E91E0(0x1d,..)）の意味。

## §4 検証・自己批評
- probe: **v365_1..v365_10 全 EXIT=0**（各 .out 保存済）。主な誤りの是正: (i) v365_4(f) の align decode が site を instruction start と誤って一部ガーベル → v365_6(b) で cover 方式（instruction が [site,site+4) を覆う start 探索）に切替、(ii) `mov reg,[abs]`（register←memory）は filter 左辺判定で落ちることを確認し unfiltered 窗で補完。
- 妥当性チェック: call site の引数個数と impl の `ret N` が一致（A->vfunc+0x134(0)⇔`ret 4`、B->vfunc+0x5C()⇔0 引数、0x66A400⇔1 引数）＝vtable/impl 対応の裏付け。
- **docs との不整合**: なし（本裁定は新規。#99 欠落 C は「保存先未特定」のままであり、本裁定はその原因＝メニューが直接書かない構造であることを示した）。
- L1 制約: docs/GT/ESM 変更なし。編集は tmp/re/・STATE・台帳・memory のみ。
