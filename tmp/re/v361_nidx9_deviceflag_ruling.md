# V361: NiDX9Renderer `[this+0x6F0]` ゲートの実体 — #140（V-358-4）

- 日付: 2026-11-13 / タスク: ai-todo **#140**【v358 残点】NiDX9Renderer vtable の `[this+0x6F0]` ゲート大群15スロットの実用途特定
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）
- 出典: `v358_bundle5_bss_nidx9_ruling.md` 残点 V-358-4（中）
- 手法: capstone 5.0.7（v307_common.py）／`.text` 全文 `disasm_range` census（v360 教訓の再同期付き）／RTTI（v359 校正の pTD=+0xC）／E8 rel32 逆算／定数比較の生バイト走査／**HRESULT 定数名は web 検証**（Microsoft d3d9 ミラー・DXERROR9 対応表）
- 中間証跡: `%TEMP%\opencode\v361_1..v361_6.py/.out`（**全 EXIT=0**）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM/.ess 未アクセス、EXE 読み取り専用、L1

---

## §0 判定サマリ

| 設問 | 判定 |
|---|---|
| ① `+0x6F0` への全書手/読手 | **[確定] 本フィールドは NiDX9Renderer の「デバイス使用不可」byte フラグ**。書手は NiDX9 側2関数のみ（**clear=fn `0x7641A0`**（唯一 caller=**NiDX9 ctor `0x769BE0` @0x769EE7**）／**set=fn `0x76B3F0`**（唯一 caller=**slot76 impl `0x76BE00`**））。読手は NiDX9 系17 site（ゲート群）＋**PC 系 dword 読1（`0x6AEF1C`）＝PlayerCharacter の同オフセット別フィールドで NiDX9 と無関係 [確定]**（RTTI で書手 `0x66A740`＝**`.?AVPlayerCharacter@@` ctor** を確認し分離） |
| ② ゲート否定時の共通 cleanup | **[確定] 共通 cleanup 呼び出しは不在**。否定＝**単純 early-out（本体 skip）**の3型: (i) `jne`→関数 epilogue 終端へ（slot80/93/88/94/0x764C70 系）(ii) `xor eax,eax; ret`（slot53）(iii) **`mov al,1; ret`（slot77＝否定で成功返却）**。返値の向きはスロットごと |
| ③ DX9 リソース作成/破棄との接続 | **[確定] flag 設定関数 `fn 0x76B3F0` 自身が接続点**: device vfunc+0x0C（TestCooperativeLevel 相当）の HRESULT で **`0x88760869`=D3DERR_DEVICENOTRESET → `0x76A970` 復元/リセット試行（成功で flag=0・戻り bool）**／**`0x88760868`=D3DERR_DEVICELOST → flag=1 ＋ リソース callback 列**（count=`[+0xAC2]`・fn 配列=`[+0xABC]`・arg 配列=`[+0xACC]`・各戻り bool 検証・失敗で警告 `0x60D0A0(文字列0xA89618)`）→ 常に return0。S_OK 等→ return1（flag 非変更）。**この2定数の比較は .text 全体で本関数1箇所のみ**（=デバイス協調レベル失敗判定は本関数が唯一）[確定] |

---

## §1 `+0x6F0` census（v361_1・全文2,258,077 insn）

### 1.1 書手（実質3関数・stack 2件は除外）

| 位置 | 命令 | 呼び手 | 意味 |
|---|---|---|---|
| `0x66A7CF`（fn 0x66A740 内） | `mov dword [esi+0x6F0], ebx(=0)` | startup `0x406793` / **world 再初期化 fn 0x464440 @0x46453C** | **PlayerCharacter ctor による dword ゼロ**（RTTI: vtable 0xA73A0C/0xA73A04/0xA739BC/0xA73988 すべて **`.?AVPlayerCharacter@@`**）→ **PC の own field。NiDX9 と別オブジェクト [確定]** |
| `0x764237`（fn 0x7641A0 内） | `mov byte [esi+0x6F0], bl(=0)` | **NiDX9 ctor `0x769BE0` @0x769EE7 のみ** | **state reset（flag クリア＋+0x5D4=0xFF808080 他多数初期化）＝ctor での初期化 [確定]** |
| `0x76B414` / `0x76B439`（fn 0x76B3F0 内） | `mov byte [esi+0x6F0], cl` / `,1` | **slot76 impl `0x76BE00` @0x76BE03 のみ** | **runtime 設定（§3）** |

- `[esp+0x6F0]` の2件（0x7D002F/0x807CB0）は**スタック局所**で無関係（除外）。

### 1.2 読手（18 site）

- **NiDX9 系17**: ゲート群 0x762603/762646/762BE6/7647E4/764C83/764E56/764FE6/765126/765999/766886/766F48/769341/7693E6/7694F6/769706/76B166（＋`fn 0x76B3F0` 内 0x76B423）。
- **PC 系1**: `0x6AEF1C` `cmp dword [eax+0x6F0],0`（eax=`[0xB333C4]`=PlayerCharacter・`[PC+0x6EC]` と対で時間条件 `[0xB33EA0]-[esi+0xC8]>0x1F4(500ms)` の後の分岐）→ **dword 形・別クラス・NiDX9 ゲートとは無関係 [確定]**。

---

## §2 ② ゲート否定時の挙動 [確定]（v361_4）

代表6 site のゲート直後に共通 cleanup 呼び出しは一切ない。3型:

| 型 | site 例 | 否定時（flag≠0）の挙動 |
|---|---|---|
| (i) epilogue skip | slot80 `jne 0x762bcd`／slot93 `jne 0x766f1b`／slot88 `jne 0x7694df`／slot94 `jne 0x76b305`／head 0x764C70 `jne 0x764e47` | 本体を飛ばして関数終端（ret/ret 8）へ → 既定値で返る |
| (ii) 即 false | slot53 `0x764E56: je 0x764e68 / xor eax,eax / ret 4` | **`xor eax,eax` で return 0** |
| (iii) 即 true | slot77 `0x762603: jne 0x762630` → `mov al,1 / ret` | **`return 1`（否定＝「不要/成功」）** |

- 正（flag==0）の時のみ本体が走る: device/リソース/描画操作（例: slot53 正パス＝`[this+0x880]` の vfunc+0x4C/0x50 呼出、slot77 正パス＝`[this+0x280]` device vfunc+0xA8 の状態確認＋`[this+0x8B0]`→`0x777A40`、slot94 正パス＝`new(0x30)` 他）。
- → **「ネガティブ時に呼ぶ共通 cleanup 経路」は存在しない＝否定は単純 early-out** [確定]（V-358-4 の設問②への回答）。

---

## §3 ③ flag 設定関数 `fn 0x76B3F0`＝デバイス協調レベル監視 [確定]（v361_2/3・web 検証）

```asm
76B3F0: eax = [this+0x280]（device ptr）
        edx = [vtable+0x0C] → call   ; ★ TestCooperativeLevel 相当（HRESULT 戻り）
76B401: cmp eax, 0x88760869          ; ★ D3DERR_DEVICENOTRESET（web 検証）
        jne 0x76B41C
76B408:   call 0x76A970              ; 復元/リセット試行（bool 戻り）
          cl = (result==0) → [this+0x6F0] = cl   ; ★ 失敗なら flag=1、成功で flag=0
          return result
76B41C: cmp eax, 0x88760868          ; ★ D3DERR_DEVICELOST（web 検証）
        jne 0x76B482                 ; S_OK 等 → return 1（flag 非変更）
76B423: if flag==0: flag = 1         ; ★ LOST → 必ず flag=1
        loop i<count [this+0xAC2]:   ; ★ リソース callback 列
          eax = [this+0xABC][i]（fn ptr）, ecx = [this+0xACC][i]（arg）
          call eax(ecx); if !al → push 警告文字列 0xA89618 → 0x60D0A0（警告 stub）→ return 0
        all ok → return 0
```

- **HRESULT 定数名**: `0x88760868 = D3DERR_DEVICELOST`／`0x88760869 = D3DERR_DEVICENOTRESET`（**web 検証**: GitHub VisualStudio リポジトリ d3d8.h ミラーの define 群、DXERROR9 対応表。`0x8876086A=D3DERR_NOTAVAILABLE` 系との整合）。
- **比較は .text 全体で各1件＝本関数のみ** [確定] → **デバイス協調レベル失敗判定はこの1関数が唯一**。
- **呼出し経路**: slot76 impl `0x76BE00`（`call 0x76B3F0; test al,al`）の **E8 直接0件・`call [reg+0x130]` FF 形0件・abs vtable slot (0xA88FD4) 参照0件** → 実際は**2命令 virtual 形 `mov r,[vptr]; call [r+0x130]`**（0x40F1D3/0x4AA87B/0x4AC1CA/0x4AC1D8/0x4AEEC5/0x4AEED3/0x4DC649/0x4EB8F3/0x515A26/0x525B69/0x529226 等 **11+ site**・+0x130 は他クラスと共有オフセットのため個別帰属は多義）→ **ポーラーの個別同定は [部分確定]（残点 V-361-1）**。

---

## §4 ①③の統合像 [確定]

1. **`[NiDX9+0x6F0]` ＝「デバイス使用不可（LOST/NOTRESET）」byte フラグ**。
2. **ライフサイクル**: ctor `0x769BE0` → `0x7641A0`（flag=0・状態初期化）→ frame 中の virtual slot76 ポーラー → `0x76B3F0` が HRESULT で0/1を設定（NOTRESET 成功で復帰、LOST でリソース callback 実行）。
3. **ゲート大群15スロットは flag≠0 の間、本体（リソース/描画）を early-out で飛ばす**（返値はスロット既定値）→ **デバイス喪失中の描画/リソース操作ブロック [確定]**。
4. **PC（PlayerCharacter）の `+0x6F0` dword は無関係の別フィールド**（0x66A740 ctor ゼロ・0x6AEF1C 読手）→ **同オフセットの混同を防ぐ訂正情報 [確定]**。

---

## §5 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v361_1..v361_6 **全 EXIT=0** |
| 2 | census の網羅性 | `.text` 全文 `disasm_range`（v360 教訓の再同期付き）2.26M insn・書手3/読手18 を全列挙、stack 2件を正しく除外 |
| 3 | クラス分離 | RTTI（pTD=+0xC 校正）で書手 `0x66A740`=PlayerCharacter を**実名で確定**し、PC dword と NiDX9 byte の**同オフセット混同を排除** |
| 4 | 定数名 | HRESULT 定数は web 検証（2独立ソース）で命名・推測を排した |
| 5 | 呼出し経路の否定的検証 | E8/FF-disp32/abs の3方向0件を確認 → 2命令 virtual 形の存在を逆推出（11+ site 列挙） |
| 6 | v358 との整合 | V-358-4 の「device lost/初期化/描画可否のどれか」設問＝**device lost/notreset 系**で回答（初期化は ctor 経路・描画可否はゲート結果として接続） |

---

## §6 新規残点

| ID | 内容 | 深刻度 |
|---|---|---|
| V-361-1 | [部分確定] slot76(+0x130) ポーラーの個別 caller 同定（2命令 virtual 形 11+ site の型帰属） | 中 |
| V-361-2 | [未確定] `0x76A970`（NOTRESET 分岐の復元/リセット試行）の本体詳細（bool 戻りの契約は確定） | 低 |
| V-361-3 | [未確定] リソース callback 列（`[+0xABC]`/`[+0xACC]`/count`[+0xAC2]`）の要素関数一覧と登録元 | 低 |
| V-361-4 | [未確定] `0x60D0A0` 警告の文字列 `0xA89618` の実文言（走査は未実施） | 低 |

---

## §7 main への報告

1. **V-358-4 完全クローズ**: `[+0x6F0]`＝**NiDX9 のデバイス喪失 byte フラグ**（書手2・読手17 を全特定）、否定＝**単純 early-out（共通 cleanup 不在）**、flag 設定＝**fn 0x76B3F0 が TestCooperativeLevel（DEVICELOST/DEVICENOTRESET 定数を web 検証）で管理＋LOST 時にリソース callback 列を実行**＝DX9 リソース系接続点。
2. **訂正情報**: `[PC+0x6F0/+0x6EC]`（PlayerCharacter・0x66A740 ctor/0x6AEF1C 読手）は **NiDX9 と無関係の別フィールド** — 後続の混同防止に記録。
3. NiDX9 は Android 移植対象外（#51 維持）・分類表は参照用。docs/GT 反映候補（デバイスフラグ・HRESULT 定数対応）は #47/#20 一括（L1）。

---

### 参照（本タスクの証跡）

- v361_1（全文 census: 書手6/読手18・head ヒストグラム）
- v361_2（fn 0x76B3F0 全体・fn 0x7641A0 state reset・fn 0x66A740 ctor・0x6AEF1C コンテキスト）
- v361_3（RTTI range バグ修正版の前準備・E8 callers（0x7641A0←ctor/0x76B3F0←slot76 のみ）・0x76B3F0 tail・HRESULT 定数各1件）
- v361_4（ゲート否定挙動6代表＝early-out 3型）
- v361_5（0x66A740 の RTTI 実名=.?AVPlayerCharacter@@・FF 形0件の確認）
- v361_6（disp32=0x130 走査＝2命令 virtual 形11+ site・abs slot 参照0件）
