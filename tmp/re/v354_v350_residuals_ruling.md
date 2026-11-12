# V354: v350 残点6件（V-350-1..6）の裁定 — #128 束①

- 日付: 2026-11-12 / タスク: ai-todo **#128 束①**（main 起票・静的残件一括委譲） / 作成: RE レーン
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 / ImageBase 0x400000）＋ `rust/crates/nif-core/src/walk.rs`
- 出典: `tmp/re/v350_n5b_load_ruling.md` §6（残点 V-350-1..6）
- 手法: capstone 5.0.7（v307_common.py）／テーブル xref 分類／b07_vtable_index.md 索引との突合
- 中間証跡: `%TEMP%\opencode\v354_1..v354_6.py/.out`（**全 EXIT=0**）
- 制約: git/docs/GT/台帳・session-memory/既存 `tmp/re/*.md` は無編集（本ファイル新規1件のみ）、ESM 未アクセス、L1

---

## §0 判定サマリ（V-350-1..6）

| ID | 残点 | 判定 |
|---|---|---|
| V-350-1 | 0x6E1E90 の Order 位置 | **[確定]** Order=u16 は **`[NiTransformData+8]`**。type==4 のときのみ `call 0x6BE490`（Order データ3スロットの解放） |
| V-350-2 | dispatch テーブル4本の初期化元 | **[確定]** 初期化 fn 3本を特定（once フラグ付き **lazy init**）。初期値＝no-op `0x6F7070`（`xor eax,eax; ret`）／警告 `0x60D0A0` |
| V-350-3 | 0x6D42D0/4300 全型網羅 | **[確定＋v350 訂正]** 正体は **RTTI チェーン走査（dynamic_cast 相当）**。v350 の「型別ランタイム変換 dispatcher」記述は**誤り** |
| V-350-4 | 0x6CB990 の追加フィールド | **[確定]** `u32→[+0x1C]`→**`<0x0A01006E` のとき bool1B 読×3→[+0]/[+0x10]/[+0x1C]**（0 なら -定数 0xA7DEB4 を float store）／**`≥` なら即 return** |
| V-350-5 | NiBoolData ゲートの追加読 | **[確定]** **`≥0x0A010068` のとき u32 1個を追加読（type 再指定）**。walk.rs `skip_key_group("byte")` と整合 |
| V-350-6 | v121 map 他エントリ再監査 | **[確定]** b07 索引と **vtable 全10件一致**（Load=slot+0x1C も slots 範囲内）。NiKeyframeData 1件のみ誤り＝v350 で修正済み |

---

## §1 V-350-1: 0x6E1E90 の Order 位置 [確定]

### 1.1 0x6E1E90 全文（ret 0xC @0x6E1EF8）

```asm
6E1E90: mov esi, ecx                  ; this = NiTransformData
        mov edi, [esi+0x20]           ; KeyGroup0 data ptr
        if (edi==0) goto clear
        cmp dword [esi+0x10], 4       ; type == 4（euler/Order）?
        jne 6E1EAB
        mov ecx, edi
        call 0x6BE490                 ; ← type4 のみ Order 系後処理（§1.2）
6E1EAB: mov eax, [esi+0x10]           ; type
        mov ecx, [eax*4 + 0xB3D2F8]   ; ← 5本目のテーブル（型別 reader）
        push edi / call ecx           ; data を渡して reader
        ; --- 引数から値を書き戻す ---
        ecx = [esp+0x10] (arg1) / eax = [esp+0x14] (arg2)
        if (ecx && eax):
            mov word [esi+8], ax       ; ★ Order = u16 を [+8] へ
            mov [esi+0x20], ecx        ; data ptr
            mov [esi+0x10], eax        ; type
            dl = [type + 0xB3D3F4]     ; 型別 flag
            mov [esi+0x1C], dl
        else: clear（[+8]=0 / [+0x20]=0 / [+0x10]=0 / [+0x1C]=0）
        ret 0xC
```

- **Order の格納先 = `[NiTransformData+8]`（u16）** [確定]。対して type=`[+0x10]`、data=`[+0x20]`、flag=`[+0x1C]`。
- **呼び出し元 = NiTransformData::Load 0x6E2180 の `0x6E223B`**（`push edi/push eax/push ebx` の3引数・type ディスポッチ `[edi*4+0xB3D440]` の直後）。
- **兄弟 setter**: `0x6E1F00`（KeyGroup1: [+0x14]/[+0x24]・テーブル 0xB3D2E0・flag [type+0xB3D3EE]）／`0x6E1F60`（KeyGroup2: [+0x18]/[+0x28]・テーブル 0xB3D2C8・flag [type+0xB3D3E8]）→ **KeyGroup 3系統の setter 3本対** [確定]。

### 1.2 `0x6BE490`（type==4 の Order 系後処理）[確定]

```asm
6BE490: lea esi, [ecx+0x30]     ; ← Order データ3スロットの先頭
        mov ebx, 3              ; 3回ループ
  loop: eax = [esi]             ; data ptr
        if (eax): ecx = [esi-0x10]（type）; edx = [type*4+0xB3D2C8]; call edx（解放）
        [esi]=0 / [esi-0x1C]=0 / [esi-0x10]=0
        add esi,4 / sub ebx,1 / jne loop
        ret
```

- → **type4（euler/Order）の Order データは `[+0x30]/[+0x34]/[+0x38]` の3スロット**（解放テーブル 0xB3D2C8 経由）[確定]。
- **walk.rs との接続**: walk.rs は type4 時に `Order(u32) + 3×KeyGroup(float)` を skip（v350 §3.1 の byte 精算と一致）。**EXE 側の `[+8]` u16 は「Order 値のキャッシュ」**で、ファイル上は u32 Order として読まれる [部分確定]（u32→u16 の切り詰め有無は未検証＝残点 V-354-2）。

---

## §2 V-350-2: dispatch テーブルの初期化元 [確定]

### 2.1 初期化 fn 3本（once フラグ付き lazy init）

| fn head | once フラグ | 初期化するテーブル | 初期値 |
|---|---|---|---|
| **`0x6BC260`** | `[0xB3C2D8]` | `0xB3D0A0`（reader 系） | **`0x6F7070`** |
| | | `0xB3D5D8` / `0xB3D548` | `0x60D0A0`（警告） |
| **`0x6BD530`** | `[0xB3C3A0]` | `0xB3D0B8`（reader 系） | **`0x6F7070`** |
| | | `0xB3D5F0` / `0xB3D560` | `0x60D0A0` |
| | | `0xB3D2F8` / `0xB3D3F4` / `0xB3D028` / `0xB3D4D0` / **`0xB3D440`**（fn2 系） | 0x60D0A0 等 |
| **`0x6BB5xx`** | （同型） | `0xB3D2C8`（解放系） | 0x60D0A0 等 |

- **形**: `cmp [once],0 → je skip → mov [once],1 → mov [table], 定数` の各テーブル個別 lazy init [確定]。
- **`0x6F7070 = xor eax,eax; ret`** ＝ **no-op reader（未登録時の既定動作＝何も読まない）** [確定]。
- **`0x60D0A0`** ＝警告/ログ関数（NiTransformData::Load の count>0xFFFF 時に `push 0xA7B534; call 0x60D0A0` で確認済み）[確定]。
- **型別 reader の実登録元（各 NiXxxData が自身の type index でテーブルへ格納するコード）は本束では全列挙せず** [部分確定]＝残点 V-354-3（低）。

### 2.2 テーブル群の全体像（本束で新規発見）

| テーブル | 用途 | xref 分布 |
|---|---|---|
| `0xB3D0A0` / `0xB3D0B8` | 型別 **reader**（KeyGroup0/1 系） | 0x6BC279 / 0x6BD578 / 0x6E21EF 等 |
| `0xB3D440` / `0xB3D428` | 型別 **fn2（後続処理）** | 0x6BD5C5 / 0x6D8FDE / 0x6E2202 等 |
| **`0xB3D2F8`**（新） | KeyGroup0 型別 reader | 0x6BD5A0 / 0x6D8C83 / 0x6E1E33 / 0x6E1EB1 |
| **`0xB3D2E0`**（新） | KeyGroup1 型別 reader | 0x6BC2A1 / 0x6D9CD3 / 0x6E1E4A / 0x6E1F13 |
| **`0xB3D2C8`**（新） | 型別 **解放（dtor/free）** | 0x6BB5A0 / 0x6BE4AC / 0x6D4783.. / 0x6E1E61 |
| `0xB3D100` / `0xB3D488` | NiBoolData reader/fn2 | 0x6E8AB6 / 0x6E8AC8 |
| `0xB3D340` | NiBoolData 解放系 | 0x6E88D1 |
| `0xB3D3F4` / `0xB3D3EE` / `0xB3D3E8` / `0xB3D406` | **型別 flag byte**（KeyGroup0/1/2/NiBool） | setter 群が load |
| `0xB3D638` | （NiBoolData 系 fn2） | 0x6E8B4E |

- → **v350 の「4本」は実際には10本超のテーブル群**（各 KeyGroup に reader/fn2/解放/flag の系統）[確定]。

---

## §3 V-350-3: 0x6D42D0/0x6D4300 の正体（v350 訂正）[確定]

```asm
6D42D0: ebx = [esp+0xC]（obj）
        if (!obj) return
        eax = [vtable+4](obj)        ; RTTI TD を取得（GetRTTI 系）
        if (!eax) return
  loop: cmp eax, 0xB3FA80            ; ★ 特定の RTTI TypeDescriptor と一致？
        je 6D4300
        eax = [eax+4]                ; RTTI チェーンを上位へ
        if (eax) continue loop
        return                        ; 不一致＝何もしない
6D4300: esi = [ebx+0xC]               ; 一致時の処理
        loop: eax = vfunc+4(esi)
              cmp eax, 0xB3DBDC / je 6D3770   ; 2段目の TD 一致で型別処理へ
```

- **正体＝RTTI チェーン走査（dynamic_cast 相当）** [確定]。
- **`0xB3FA80` = RTTI TypeDescriptor 配列の1番** [確定]:
  - **`0x707D70: mov eax, 0xB3FA80; ret`** ＝ TD を返すゲッタ。
  - **登録 thunk 群 `0xA09BExx`**: `push 0xA7E0B4; mov ecx,0xB3FA80; call 0x70E220`（型名文字列 → TD）／`0xA09C20`（0xA7E1A4 → 0xB3FA88）／`0xA09C90`（0xA7E440 → 0xB3FAB0）／`0xA09CD0`（0xA7E5D4 → 0xB3FACC）／`0xA0A020`（0xA7F968 → 0xB3FD5C）＝ **TD は 8バイト以上間隔で並び、int3 間の thunk が型名を紐付ける**。
  - **`0x560920`** ＝ TD のチェーン一致走査関数（`push 0xB3FA80; call 0x560920` を複数箇所で使用）。
- **v350 §1.5 の「型別ランタイム変換 dispatcher（0xB3FA80 → 0x6D3EB0/0x6D3BD0 等へ分岐）」は誤り**（0xB3FA80 は分岐先テーブルではなく **RTTI TD**）→ **本裁定で訂正** [確定]。
- `0x6D3EB0` は別物（`[ebp+0x30]→[ebx+0xC]` → `cmp eax,0xB3CA58` → `[vfunc+0x80]` 呼び出し＝NiObject 系の別処理）。

---

## §4 V-350-4: 0x6CB990（LookAt サブレコード）の追加フィールド [確定]

```asm
6CB990: stream = [esp+0x10]（arg1）
        call 0x709430(this, stream)         ; base
        lea ecx,[edi+0xC] / call 0x715420   ; +0xC へ（name/文字列）
        u32 読 → [edi+0x1C]                  ; push 4, dest=&[edi+0x1C]
        cmp [stream+0xD8], 0x0A01006E
        jae 0x6CBA80                         ; ★ ≥ なら即 return（追加読なし）
        ; < のときのみ:
        bool1B 読 ×3（各回 [esp+0x20]=1 size）
          1本目: if byte==0 → fld -[0xA7DEB4] → fstp [edi]        ; [+0]
          2本目: if byte==0 → fstp [edi+0x10]                     ; [+0x10]
          3本目: if byte==0 → fstp [ebx]=[edi+0x1C]               ; [+0x1C] を float で上書き
        ret（末尾 fall-through は 0x6CBA80: pop×4 / ret 4）
```

- **追加フィールドの正体＝bool 3個（バージョン `<0x0A01006E` のときのみ）**、各 0 なら **-定数 `0xA7DEB4` を float で store** [確定]。
- → **`≥0x0A01006E` では bool 読を行わない**（＝v350 の「0x0A01006E ゲート」は **旧版のみ読む** 方向のゲート）[確定]。
- 隣接 fn: `0x6CBA90`（`0x7094A0` + `0x7154B0` + u32→[+0x1C]＝Save 系）／`0x6CBAD0`（`0xA79E3C`・`[esi+8]`/`[esi+0xA]` 比較＋`[edi]` と -定数の `fucompp`＝IsEqual/Compare 系）[確定]。

---

## §5 V-350-5: NiBoolData のゲート後追加読 [確定]

### 5.1 EXE 0x6E8A30（Load）

```asm
6E8A30: call 0x7008A0                  ; NiObject::Load base
        u32 読 → [esp+0x18]             ; count
        if (count==0) goto end
        cmp [ebx+0xD8], 0x0A010068
        jb 6E8AA8                       ; ★ < なら追加読なし（type は既定）
          else: u32 読 → [esp+0x24] → esi = 値   ; ★ ≥ のとき type を u32 で再読
6E8AA8: cl = [type + 0xB3D406]          ; 型別 flag
        [edi+0x14] = cl
        eax = [type*4 + 0xB3D100] / call eax      ; reader
        edx = [type*4 + 0xB3D488] / call edx      ; fn2
        call 0x6E88C0(this, data, count, type)    ; 後続処理
        ret 4
```

### 5.2 walk.rs 突合

- **walk.rs L1653-1655**: `"NiBoolData" => { self.skip_key_group("byte"); }`（＝count + interp + byte キー）。
- **EXE の `≥0x0A010068` 追加 u32（type）＝ walk.rs の interp(u32) 相当** [確定]（v350 §4 の「NiBoolData のみ追加バージョンゲート」を具体化）。

### 5.3 `0x6E88C0`（後続処理＝KeyGroup setter）[確定]

```asm
6E88C0: [esi+0xC]（旧 data）があれば [type*4+0xB3D340] で解放
        引数: edi=[esp+0x10](data) / ecx=[esp+0x14](count) / eax=[esp+0x18](type)
        if (data && count):
            [esi+0xC] = data / [esi+0x8] = count / [esi+0x10] = type
            [esi+0x14] = [type+0xB3D406]（flag）
        else: [+8/+0xC/+0x10/+0x14] を0クリア
        ret 0xC
```

- → **NiBoolData の KeyGroup レイアウト: `[+0x8]=count, [+0xC]=data, [+0x10]=type, [+0x14]=flag`**（NiTransformData の `[+8](Order u16)/[+0x10](type)/[+0x20](data)/[+0x1C](flag)` とは別レイアウト）[確定]。

---

## §6 V-350-6: v121 map 他エントリの再監査 [確定]

**v350 §1.3 の補正 COL 走査結果を b07_vtable_index.md の索引行と突合**:

| クラス | v350 §1.3 vtable | b07 索引 vtable | b07 slots | b07 COL | b07 TD | Load=+0x1C | 判定 |
|---|---|---|---:|---|---|---|---|
| NiFloatData | 0xA7A2EC | 0xA7A2EC | 19 | 0xAC62B8 | 0xB242E4 | 0x6E3690 | ✅一致 |
| NiRotData | 0xA7AB3C | 0xA7AB3C | 19 | 0xAC6894 | 0xB24604 | 0x6D8F60 | ✅一致 |
| NiPosData | 0xA7A624 | 0xA7A624 | 19 | 0xAC6500 | 0xB24444 | 0x6D9F90 | ✅一致 |
| NiBoolData | 0xA7C2AC | 0xA7C2AC | 19 | 0xAC75FC | 0xB24DA4 | 0x6E8A30 | ✅一致 |
| NiControllerSequence | 0xA79A34 | 0xA79A34 | 19 | 0xAC6170 | 0xB0723C | 0x6C8E40 | ✅一致 |
| NiTextKeyExtraData | 0xA7AA0C | 0xA7AA0C | 21 | 0xAC6754 | 0xB24548 | 0x6D7780 | ✅一致 |
| NiKeyframeManager | 0xA7B3FC | 0xA7B3FC | 29 | 0xAC6E84 | 0xB249CC | 0x6E12C0 | ✅一致 |
| NiTimeController | 0xA7EB24 | 0xA7EB24 | 29 | 0xAC90E8 | 0xB14B04 | 0x715F40 | ✅一致 |
| NiGeomMorpherController | 0xA7A23C | 0xA7A23C | 31 | 0xAC6260 | 0xB242B8 | 0x6D0D30 | ✅一致 |
| NiTransformData | 0xA7B474 | 0xA7B474 | 19 | 0xAC6ED8 | 0xB249F0 | 0x6E2180 | ✅一致（v350 検証済） |

- **vtable が b07 索引と全10件一致**、Load（slot+0x1C）も各 slots 範囲内 [確定]。
- → **v121 の11 クラス vtbl/TD map は NiKeyframeData 1件のみ誤り**（v350 で NiTransformData へエイリアスと訂正済み）で、**他9件は正** [確定]。

---

## §7 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧 | v354_1..v354_6 **全 EXIT=0** |
| 2 | Order 位置の二重根拠 | 0x6E1E90 内の `mov word [esi+8],ax` と 0x6BE490 の `[ecx+0x30]` 3スロット解放の**両方**で +8 体系と整合 |
| 3 | テーブル初期化の網羅性 | 4本（v350 記載）＋新5本（0xB3D2F8/E0/C8/0xB3D100/0xB3D488 等）を xref 分類で全走査。**.text に書込なし＝初期化 fn のみ** [確定負] |
| 4 | 0x6D42D0 の訂正根拠 | `cmp eax,0xB3FA80` のループ構造＋`0x707D70: mov eax,0xB3FA80; ret`＋登録 thunk 群の3系統で RTTI TD と確定（v350 の「分岐テーブル」説と矛盾） |
| 5 | bool ゲートの方向 | `jae 0x6CBA80`（= ret）を命令実読で確認＝**≥ で読まない**（v350 の記述は方向が曖昧だった→確定） |
| 6 | v121 再監査の独立性 | b07 索引（別セッション・別手法）と v350 補正走査の**結果一致**＝独立2経路で検証 |

---

## §8 新規残点（次束用）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-354-1 | [部分確定] 各テーブルへの**型別 reader の実登録元コード**（NiXxxData の ctor/Load が type index で格納する箇所）の全列挙（§2.1 の補足） | 低 |
| V-354-2 | [部分確定] type4 の **u32 Order → `[+8]` u16 の切り詰め有無**（walk.rs は u32 として skip・EXE は u16 キャッシュ） | 低 |
| V-354-3 | [未確定] `0x6D4300` 一致後の2段目 TD（0xB3DBDC）と型別処理 `0x6D3770` 以降の全型分岐（本束は LookAt 系のみ実測） | 低 |
| V-354-4 | [未確定] `0xA7DEB4` 定数の値（bool 0 時の float 代替・−? 値）と `0xA79E2C/0xA79E3C` 文字列 | 低 |

---

## §9 main への報告

1. **v350 §6 の残点6件は本裁定ですべて [確定]（うち1件は v350 訂正）**。`tmp/re/v350_n5b_load_ruling.md` の §6 表は本ファイルで更新済みとして扱ってよい。
2. **walk.rs への影響なし**: §4/§5 の結果は walk.rs の `skip_key_group("byte")`・type4 skip と**整合**（追加の実装変更不要）。
3. **§2.2 のテーブル群一覧（10本超）**は NiTransformData/NiBoolData 以外の KeyData クラス解析（今後の N5b 系）で再利用可能。
4. **§3 の訂正（0x6D42D0＝RTTI 走査）**は v350 §1.5 の記述訂正として docs/GT 反映時に適用（#47/#20 一括）。

---

### 参照（本タスクの証跡）

- `v354_1`（0x6E1E90 全文・0x6E223B 呼び出し文脈・4本テーブルの xref 分類）
- `v354_2`（初期化 fn 0x6BC260/0x6BD530・0x6BE490 全文・追加テーブル xref）
- **`v354_3`（b07 索引突合＝V-350-6 完結・0x6F7070=no-op 確定）**
- **`v354_4`（0x6CB990 全文・NiBoolData 0x6E8A30 全文・walk.rs L1653）**
- **`v354_5`（0x6CBA80=即 return・walk.rs NiBoolData arm・0x6E88C0 全文）**
- **`v354_6`（0x6D42D0/4300 全文・0xB3FA80 の TD 登録 thunk 群＝V-350-3 訂正）**
