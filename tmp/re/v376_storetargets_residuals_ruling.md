# v376 — #150 ②③の裁定: 0x6C6A50 他 callers の fn identity と `[esp+0x38]` 設定元（v372 残点）

- 採番: **v376**（#150 専用、probe prefix v376_1）
- 対象: Oblivion.exe v1.2.0416
- 依頼: ai-todo #150（v372 の残点3件。**①は v373 で解決済み**（registry＝`NiMultiTargetTransformController` singleton／48B＝`NiBlendTransformInterpolator`）→ 本 v376 は②③を裁定）
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| ② 0x6C6A50 他 callers（`0x6C9D70`/`0x6C9E55`）の fn identity | **[部分確定]** | 両者とも **head `0x6C9CB0`**（`xor edx,edx; cmp [ecx+0x44],edx; jne; xor al,al; ret 8`＝**`[this+0x44]` state を bool 判定して返す**・`fldz; fld [esp+4]; fcom st(1)`＝float 比較）。**E8 callers 15本**（`0x470B5E/0x471083/0x4715E0/0x471A70/0x471AA0/0x4732A1/0x473B35/0x4745ED/0x4746F6/0x475C08/0x47605D/0x4B7F15/0x4DE561/0x4DE5C2/0x4E30B6`＝**game 側 0x47xxxx animation 帯から広く呼ばれる**）→ **NiControllerSequence の activate/cycle 系 [推定高]**。中身: state 遷移（`[esi+0x44]` を 1/5/6 へ）＋ **field8 interpolator へ `0x6C6A50`（`virtual+0x98` 呼び出し）** ＋ `0x6C9BA0`（`ret 0x18` 系）＋ `0x73A5E0` 呼び出し |
| ③ `[esp+0x38]` special-case フラグの設定元 | **[確定(否定側)]** | StoreTargets 範囲（`0x6C9590..0x6C9BA0`）の**全 store 形式（C644/C684/88xx/C744/89xx）走査で writers = 1 件のみ**＝**prologue の `0x6C95CE: mov byte [esp+0x38],0`**。**1 に立てる書き手は存在しない** → `0x6C9B5D: jne 0x6C9B71`（Bip01 直行）は**死コード** [確定(否定側)]。実挙動＝**フラグ=0（常に）で Bip02 FindChild → 見つかれば Bip01 FindChild → Bip01 の子 `[+0x1C]` → `0x6C6910`** |

**#150 への回答**: ①は v373 で解決済み（registry＝`NiMultiTargetTransformController` singleton／48B＝`NiBlendTransformInterpolator`）。②は head `0x6C9CB0`（state 判定 bool＋activate 系 [推定高]）。③は **フラグの設定元が存在しない**（常に Bip02→Bip01 の順）＝v372 §2.2D の「[esp+0x38] 処理済みで Bip01」記述は**死コードのため実行されない**ことを補正。

---

## §2 証拠

### §2.1 ② `0x6C6A50` 他 callers の fn identity [部分確定]

v376_1(a)（両者とも head `0x6C9CB0` を返す）:
```
6C9CB0: xor edx, edx
6C9CB2: cmp dword [ecx+0x44], edx      ; ★[this+0x44] state チェック
6C9CB5: jne 0x6C9CBC
6C9CB7: xor al, al
6C9CB9: ret 8                          ; state==0 → false（2引数）
6C9CBC: fldz
6C9CBE: fld dword [esp+4]
6C9CC2: fcom st(1)                     ; float 比較（time 系 [推定]）
```
- **`0x6C9D70` 周辺**: `cmp [esi+0x44],0` → `mov [esi+0x58],0` → **`call 0x6C6A50`**（field8 interpolator へ `virtual+0x98`）→ `cmp [esi+0x44],0` → `mov [esi+0x44],1` → `call 0x73A5E0` → `push 1; push ecx; push 0; push edi; call 0x6C9BA0` → `push 1; push ecx; call 0x6C9CB0`
- **`0x6C9E55` 周辺**: `call 0x6C9CB0` → ... → **`call 0x6C6A50`** → `mov [esi+0x44],5` → `call 0x73A5E0` → `mov [esi+0x44],1` → `mov [edi+0x44],6` → `ret 0x14`
- **state 値 0/1/5/6** の遷移 ＋ `[+0x58]` ゼロ化 ＋ interpolator 呼び出し ＝ **`NiControllerSequence` の activate/cycle 系 [推定高]**（`[this+0x44]`＝cycle type/state、`[+0x58]`＝time/phase [推定]）。E8 callers 15本が game 側 animation 帯（0x47xxxx）であることと整合。

### §2.2 ③ `[esp+0x38]` writers = 1 件のみ [確定(否定側)]

v376_1(b)（StoreTargets `0x6C9590..0x6C9BA0` に全 store 形式を走査）:
```
C644 (byte [esp+0x38],0x00) @0x6C95CE :: 6C95CE: mov byte [esp+0x38],0; 6C95D3: mov [esp+0x1c],0; 6C95DB: jbe 0x6c9b30
(writers 1)
```
- **1 に立てる書き手は不在** → `0x6C9B5D: jne 0x6C9B71`（Bip01 直行）は**常に fallthrough＝死コード** [確定(否定側)]。
- **実挙動（v372 §2.2D の補正）**: **フラグ=0（常に）→ `root->FindChild('Bip02')`（0xA79CD0）→ 見つかれば（`je 0x6C9B94` で終了ではなく fallthrough）`FindChild('Bip01')`（0xA30968）→ その子 `[+0x1C]` → `0x6C6910(owner, 子)`**。
- → v372 の「[esp+0x38] 処理済みで Bip01」記述は**死コードのため実行されない**（Bip02 FindChild が null のときだけ `je 0x6C9B94` で終了）。

## §3 残点 **V-376-1..2**
1. **head `0x6C9CB0` の正式な fn 名と `[this+0x44]` state の意味**（cycle type enumか activation state か。b07 の `NiControllerSequence` vtbl slot との照合で切り分け可）【中】
2. **`0x73A5E0`（state 遷移時に呼ばれる）の identity**（コールバックか SEH か）【低】

## §4 検証・自己批評
- probe: **v376_1 全 EXIT=0**。
- **決定打の作り**: (i) `0x6C6A50` の he callers の find_head（両者 head `0x6C9CB0`）→ (ii) head の E8 callers census（15本＝game 側）→ (iii) **`[esp+0x38]` の全 store 形式走査で writers=1 件を証明**（死コード判定）。
- 妥当性チェック: ①`0x6C9CB0` の `[this+0x44]` チェック⇔中身の state 遷移（1/5/6）⇔ `0x6C6A50` 呼び出し — ②writers=1⇔ `jne` が常に fallthrough⇔ v372 の記述補正 — **矛盾なし**。
- docs との不整合: **v372 §2.2D の「[esp+0x38] 処理済みで Bip01」記述を補正**（死コードのため実行されない。v372 本文は歴史記録として温存、本 v376 §2.2 が正）。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
