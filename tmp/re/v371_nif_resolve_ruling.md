# v371 — NIF コントローラ参照解決（resolveControllerReferences 相当）の裁定（#145）

- 採番: **v371**（#145 専用、probe prefix v371_1..v371_10）
- 対象: Oblivion.exe v1.2.0416 ＋ port 実測（rust/crates/nif-core/src/block.rs:1203-1369・app assets/nif_parser.cpp:612 stub）
- 依頼: ai-todo #145（#45 N5b ブロッカー。①resolver 本体特定 ②アルゴリズム内容（node/clip/順序/失敗処理）③resolved_bone_index 経路 ④NiKeyframeData 配列対応）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令または原本アセットのみ。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| ① resolver 本体 | **[確定(文字列+命令)]** | **`0x6C9590` = `NiControllerSequence::StoreTargets`** — fn 内から**3つの format 文字列を push**（`0xA79CD8`/`0xA79B70`/`0xA79AA8`＝いずれも冒頭 `NiControllerSequence::StoreTargets '%s'...`）。**vtable slot ではなく直接 E8 呼び出し**（callers 4本: game側 `0x47334E` と NiControllerManager 系3本）[確定(命令)] |
| ② アルゴリズム | **[部分確定]** | **名前ベース解決**: u16 offset（**0xFFFF→NULL**）→ palette ベース加算 → **`virtual+0x4C(root, name)`＝FindChild 系 [推定高]** → 無ければ special-case（**Bip01→Bip02 再 lookup**・`strncmp(name,'Bow',3)`/`'Arrow',5)`）→ fail なら **3種の失敗ログを飛ばして次ブロックへ（解決継続）**。node 一致後は **node の子リスト `[+0x9C]` を走査し propertyType 名と inline strcmp で child を選択**。**順序 = sequence 前処理（target 名→node を `[seq+0x60]` へ store）→ 前提 fn `0x6C8220`（"NiTransformController" プロトタイプ登録チェック）→ sequence 単位の一括ループ**（count=`[owner+0xC]`・stride16・2配列同時走査）[確定(命令)] |
| ③ resolved_bone_index | **[未確定]** | StoreTargets の decode 窓に**骨テーブル照合なし**（AnimationPlayer 側で -1 を埋める経路が未特定）＝残点 V-371-3 |
| ④ keyframeData 配列対応 | **[未確定]** | `0x6FFE90`（成功パスが呼ぶ SEH 大 fn・`0x75FA90` を内部呼出）が配列対応の候補だが未 decode ＝残点 V-371-2 |

**最重要の一致（#45 への直接供給）**: 失敗ログに展開される識別子5つ＝**`m_pcAVObjectName / m_pcPropertyType / m_pcCtlrType / m_pcCtlrID / m_pcInterpolatorID`** は **port `KeyframeController` の palette offset 5項目（node_name_offset / property_type_offset / controller_type_offset / controller_id_offset / interpolator_id_offset）と完全1対1** [確定(文字列↔port実測)]。すなわち **StoreTargets = port stub `resolveControllerReferences` の正統な原本対応関数**である。

---

## §2 証拠

### §2.1 本体同定（①）[確定(文字列+命令)]
- **format 文字列3種**（すべて fn 0x6C9590 内から push）:
  - `0xA79CD8`: `"NiControllerSequence::StoreTargets '%s'failed to resolve dependencies for the interpolator with the following identifiers:\n\tm_pcAVObjectName\t%s\n\tm_pcPropertyType\t%s\nm_pcCtlrType\t\t%s\n\tm_pcCtlrID\t\t\t%s\n\tm_pcInterpolatorID\t%s\n"`（push @`0x6C9697`）
  - `0xA79B70`: 同冒頭 + `failed to find target with the following identifiers:`（4識別子）（push @`0x6C99B8`）
  - `0xA79AA8`: 同上（5識別子・InterpolatorID 付き）（push @`0x6C9ADF`）
- **callers（E8 4本）**: `0x47334E`（game側 fn `0x473200` 内）・`0x6C428B`・`0x6C4C7A`・`0x6C57B7`（NiControllerManager バンド 0x6C42xx-0x6C57xx）→ **virtual slot 経由でなく直接 call** [確定(命令)]（v371_5）。
- **game側 fn `0x473200`**: trait get ×4（`0x470FC0`）→ Manager 呼び出し（`0x6C4480`）→ プロトタイプレジストリ照合（`push 0xB3CD7C; call 0x700010`＝v348 attach パイプラインと**同一レジストリ**）→ `strcmp(?, '__TempBlendSequence__'@0xA3C808)` → **`call 0x6C9590`**。**E8 callers = `0x5C4D62/78/8E/A4`（chargen バンド）＋`0x5E141D/47/6D/96`＋`0x473BE0`** → **chargen/RaceSex 系からも呼ばれる** [確定(命令)]。

### §2.2 アルゴリズム（②）[部分確定]

**A. sequence 前処理（ループ前）** [確定(命令)]:
```
6C9596: eax=[seq+0x40] → edi=[eax+0x7c]           ; root? (下記 identity は残点)
6C959D: eax=[seq+0x5c]（target 名系） / test
6C95AC: edi->virtual+0x4C(name) → 6C95B6: [seq+0x60] = eax   ; ★target 名→node 解決・結果を [seq+0x60] へ store
6C95C1: call 0x6C8220(seq, arg) → bool → [esp+0xA]  ; 前提チェック
```
- `0x6C8220` の中身: プロトタイプ名のバイト比較 **`cmp byte [eax+2],0x54('T')` / `cmp byte [eax+7],0x66('f')`** ＝ **"NiTr...f..."＝"NiTransformController" の登録確認** [確定(文字列・2byte 实測)]（v371_8(d)）。`new(0x48)`・`0x6C5EE0` 等を経由し bool を返す [部分確定]。

**B. ループ（sequence 単位の一括）** [確定(命令)]:
```
6C95DB: count = ? → jbe skip            ; 0件ならループ自体をスキップ
6C95E3: for i: eax=[esp+0x24]; shl eax,4 (i*16)
        ecx = [ [owner+0x14] + i*16 ]   ; ★配列A（block 構造・stride16）
        je → skip (null)
        ebx = [seq+0x18] + i*16          ; ★配列B（palette-name 記録・stride16）
        al = block->virtual+0x8C(arg)    ; ★predicate
        test al,al → jne 0x6C96B0        ; true → C(node解決)へ
        (false) → 5識別子を u16 resolve（0xFFFF→NULL）→ 0x748530(format 0xA79CD8)＝
                  "failed to resolve dependencies" ログ → 次ブロックへ [確定(命令)]
6C9B16: i++ / cmp [owner+0xC] / jb 0x6C95E3   ; 終端 = count [確定(命令)]
```

**C. node 解決パス（0x6C96B0）** [確定(命令)+推定高]:
```
6C96B3: cmp [esi+ecx+8],0 → jne skip    ; 既に解決済みならスキップ
6C96BE: name = resolve([ebx+4] node_name_offset)   ; 0xFFFF→NULL
6C96DB: eax = [root]->virtual+0x4C(name)           ; ★名前→node lookup（FindChild 系 [推定高]）
6C96E0: ebp = eax; test; je 0x6C976F               ; 見つからず → D(failure)
6C96EE: propertyType = resolve([ebx+6]) → 0xFFFF なら 0x6C9767
6C9702: edi = [ebp+0x9C]（node の子リスト [推定]）
6C9710..6C975B: 子リスト走査（inline strcmp 2バイト単位）で child 名 == propertyType 名
6C975F: 一致 → [esp+0x1C] = child
6C9767: ebp(解決結果) → test; jne 0x6C9852（成功）
```

**D. failure パス（0x6C976F）+ special-case** [確定(命令+文字列)]:
```
6C9785: [esp+0x40] フラグ !=0 → 既処理済みとして skip
6C9790: strcmp(name, 'Bip01') ==0 → root->virtual+0x4C('Bip02')  ; ★Bip01→Bip02 フォールバック
6C97C0: strncmp(name, 'Bow',  3) → 命中時 strncmp(name,'Arrow',5) ほか → 0x6C97DB 以降も文字列 fallback
（未命中 → 0x748530 ログ or skip・詳細 decode は残点 V-371-4）
```

**E. 成功パス（0x6C9852）** [部分確定]:
- `virtual+4`（GetName 系）⇔ `0x6C63C0`/`0x6C63E0`（＝**u16 offset アクセサ群**: `movzx eax,[ecx+8/+0xA]; cmp 0xFFFF→0 else base+off` — 0x6C63A0(+6)/0x6C63C0(+8)/0x6C63E0(+0xA)・各 E8 callers=2 [確定(命令)]）と **inline strcmp** で名前整合確認 → `0x6FFE90` 呼出（SEH 大 fn・内部で `0x75FA90`（E8 callers=40＝汎用リスト/マップ helper [推定]））→ **5識別子 resolve → `0x748530(format 0xA79B70 "failed to find target")`**（条件分岐の下）→ `virtual+0x58/0x80/0x7C/0x84/0x98` を sequence/manager オブジェクトへ連打 → **`mov [esi+eax+8], edi`（0x6C9B12・結果の格納 [推定]）**→ ループ継続。
- 失敗ログ3種はすべて **0x748530**（先頭 `mov eax,[esp+8]`→テーブル `[0xB40610 + id*12]` byte flag／`[0xB40614]` −1 sentinel・32 エントリ・IAT `0x7009D6` 経由 CRT ＝ **NiError/ログ エミッタ [推定高]**）。

### §2.3 既知の後方整合（port との対応）
1. **5識別子 ↔ port 5 palette offset** [確定]（§1 表）— port が parse 時に保持する field 名が m_pc* と完全一致。
2. **`0xFFFF` = "なし" sentinel**: EXE アクセサ群3本＋ループ内 resolve すべてが `cmp ax,0xFFFF→NULL` [確定(命令)] — port 側の off 値が u32 のまま 0xFFFF 以内なら同一語彙。
3. **`clip`/`target_node_index` を「StoreTargets 実行時に埋める」**構造は port コメント（block.rs:1242-1246「the original assigns both here and never touches `clip`」）と整合するが、**EXE 側で clip に相当する store を特定できておらず [未確定]**（V-371-2）。
4. **順序**: Load（parse 時）はターゲット未解決のまま保持 → **Load 完了後の StoreTargets（Manager/game 呼び出し）で解決** = port の「parse 後に resolveControllerReferences を呼ぶ」設計と同型 [推定高]。

## §3 残点 **V-371-1..5**
1. **predicate `virtual+0x8C`（block の controller オブジェクト）の戻り値の意味**（何が false で "failed to resolve dependencies" になるか）【中】
2. **clip の構築元と NiKeyframeData 配列対応** — 成功パスの `0x6FFE90`（SEH 大 fn・0x75FA90 内部呼出）と `0x75FA90`（callers 40）の decode【**高・#45 N5b の④に必須**】
3. **resolved_bone_index の解決経路**（骨名→スケルトン index 照合がどこか。StoreTargets 窓には不在）【**高・#45 N5b の③に必須**】
4. **結果格納の最終対応**: `mov [esi+eax+8], edi`（0x6C9B12）の esi/eax/edi identity と `virtual+0x58/0x84/0x98` の先（=port の target_node_index / keyframe_data_index / resolved_bone_index のどれに当たるか）【中】
5. **identity 類**: `[seq+0x40]`/`[+0x7C]`/`[seq+0x5C]`/`[seq+0x60]`（AccumRootName? root node? manager?）、fn `0x473200` の正式名、`0x748530` の IAT 先【低】

## §4 検証・自己批評
- probe: **v371_1..v371_10 全 EXIT=0**（**v371_6 初回のみ `strings_in(cap=)` TypeError → kwarg 除去で修正後成功**＝自己是正）。
- **決定打の作り**: (i) vtable slot 列挙（NiControllerSequence 0xA79A34 / NiControllerManager 0xA79804 / NiStringPalette 0xA79934 の全 slot 実測）→ (ii) palette アクセサ群（+4/+6/+8/+0xA の u16+0xFFFF パターン）を helpers から特定 → (iii) **0xFFFF センチネル連打バンドを先頭から遡って head 同定** → (iv) **fn 内 format 文字列の実読（str_at）で関数名を直接確定**（推測ゼロ）。
- 妥当性チェック: format 文字列の識別子5つ ⇔ port field 5項目の1:1、0xFFFF sentinel ⇔ port の offset 語彙、load→resolve 分離 ⇔ port の parse/resolve 分離 — **三方向で矛盾なし**。
- docs との不整合: **なし**（本裁定は新規。v348 の「0x5A3520 座標外」裁定は維持・本裁定が代替座標を確定）。
- L1 制約: docs/GT/ESM 変更なし・移植側ソース変更なし。
