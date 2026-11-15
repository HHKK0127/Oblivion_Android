# v382 — Activate の s3/s4 と RegisterInterpolator arg2/arg4 の接続の裁定（#152 ①・V-381-2）

- 採番: **v382**（ai-todo #152 束の第1弾、probe prefix v382_1..v382_4）
- 対象: Oblivion.exe v1.2.0416 ＋先行裁定 v187_anim_residuals.md
- 依頼経緯: v381 の残点 **V-381-2（`0x6CC5C0` の arg2/arg4（weight 0.0/ease 1.0）が Activate の s3/s4 とどう接続されるか）** を消化
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| arg2/arg4（weight 0.0/ease 1.0）と s3/s4 の接続 | **[確定(否定側)]＋[確定(命令)]** | **直接接続は存在しない**。arg2/arg4 は**登録時の初期値のみ**（`0x6C6A50` が全 block に weight=0.0/ease=1.0 を渡す＝v381）。**s3/s4 は別経路** — ①s3 → `[seq+0x1C]`、s4 → `[seq+0x50]`（fade 終点素材・v378/v380）を **fn `0x6CA950`（フェード/重み更新）**が毎フレーム消費し ②線形フェード係数と `[seq+0x1C]` を乗算して **`0x6C6DC0`（重み伝播）**へ渡す。**登録時 weight=0.0 は以後の更新で上書きされる** |
| `0x6CA950` の identity | **[確定(命令)]** | **NiControllerSequence のフェード/重み更新 fn**（`ret 8`＝(time float, flag int)）。state==0 で即退出 → `[+0x48]`（s2 ease フィールド）と `[+0x4C]`（fade start）が未設定（−const）なら **現在時刻で初期化（start=now／end=now+`[+0x50]`）** → `(time−start)/(end−start)` の**線形フェード係数**（state 1..6 で分岐 `lea eax,[ecx-1]; cmp eax,5`）→ `[+0x48]` 加算 → **`[+0x1C]`（s3 由来）を掛ける** → `0x6C6DC0` へ |
| s3/s4 の消費経路の駆動元 | **[確定(命令)]** | **fn `0x6C4200`**（`[this+8]` bit3 ゲート・**E8 callers=0**＝virtual/register 経由）＝**有効 sequence 配列 `[this+0x4C]`（count `[this+0x54]`）を走査し各 sequence へ `(time, flag=1)` で `0x6CA950` を呼び、`state==0` の sequence を配列から除去**（count--・前方詰め） |
| （付随）Deactivate の解除経路 | **[確定]（v187 接続）** | `0x6C6AC0`（E8 callers＝`0x6C9D25` の1本）＝各 block の **field8 → `virtual+0x9C([eax+0xC])`** ＝ **v187 L135 の `UnregisterInterpolator`（priority byte 解除）**。Activate 側の `0x6C6A50`（Register）と対 |

**結論**: **weight は「登録時 0.0 → 毎フレーム `0x6CA950` が s3×fade 係数で更新」の2段構え**。s3/s4 は RegisterInterpolator の引数ではなく、**sequence 自身のフェード状態（`[+0x1C]`/`[+0x4C]`/`[+0x50]`/`[+0x48]`）を介して毎フレームの重み計算に入る** [確定(命令)]。

---

## §2 証拠

### §2.1 登録時初期値（v381 再掲・否定側の根拠）

- `0x6C6A50`（v381 §2.1）: `push 1.0`(ease) `push 0.0`(weight) `push edx`(priority) `push eax`(arg1) → `field8->virtual+0x98`。**全 block が weight=0.0/ease=1.0 で登録される**。
- `0x6CC5C0`（v381 §2.3）: arg2 → `[ebp+4]`（m_fWeight）、arg4 → `[ebp+0x10]`（m_fEaseSpinner・`0x6CC664: fstp [ebp+0x10]`＝v382_1(d) で唯一の書手）。
- → **s3/s4 がここに渡ることはない**（渡し手が固定値）[確定(否定側)]。

### §2.2 `0x6CA950` = フェード/重み更新（v382_3(b)(d)/v382_4(d) unfiltered）

```
6CA950: sub esp,8; push esi; mov esi,ecx
6CA956: ecx = [esi+0x44]（state）
6CA959: test ecx,ecx; je 0x6CAC36        ; state==0 → 即退出
6CA961: fld [esi+0x48]（s2 ease フィールド）
6CA964: fld [0xA7DEB4]; fchs; fucompp    ; 未設定（-const）判定
6CA979: [esi+0x48] = -time               ; ★初回に ease フィールドを現在時刻基準で初期化
6CA980: fld [esi+0x4C]（fade start）
6CA983: fld -const; fucompp; jp 0x6CA99F ; 設定済みならスキップ
6CA994: fst [esi+0x4C]                   ; ★fade start = 現在時刻
6CA997: fld [esi+0x50]; fadd st(1); fstp [esi+0x50]
                                         ; ★fade end = fade 期間(=s4) + 現在時刻
…（state 1..6 分岐: 6CA9A1 lea eax,[ecx-1]; cmp eax,5; ja）
6CA9E7: fsub [esi+0x4C]                  ; time − start
6CA9EA: fld [esi+0x50]; fsub [esi+0x4C]  ; end − start
6CA9F0: fdivp st(1)                      ; ★t = (time−start)/(end−start)（0..1）
6CABE4: fadd [esi+0x48]                  ; + ease オフセット
6CAC16: fld [esi+0x1C]（★s3 由来）
6CAC19: fmul [esp+0x14]                  ; ★× fade 係数
6CAC1D..6CAC25: arg1 として積む
6CAC28: call 0x6C6DC0                    ; ★★重み伝播（E8 callers=1＝本経路のみ・v382_1(c)）
6CAC31: ret 8                            ; (time, flag)
```

- **引数**: arg1=time（`[esp+0x10]`）、arg2=flag（caller `0x6C4228` は `push 1`、自己呼出し `0x6CABBD` は `push 0`）。
- **crossfade 相手の再帰更新**（v382_3(b)/v382_4(b)）: `6CABA6: fld [ecx+0x34]; fld [ecx+0x48]; fadd st(2); fucompp` → 不一致なら `push 0; push ecx; fstp [esp]; call 0x6CA950`（**相手 sequence（`[esi+0x58]`＝Activate の s5）も flag=0 で更新**）→ `6CABDB: call 0x6C6B30`（**v187 L140 のテキストキー同期**）[確定(命令)]。
- **`0x6C6DC0`（重み伝播）**: E8 callers＝**`0x6CAC28` の1本のみ**（v382_1(c)）。本体は `[eax+0xE]`（m_ucInterpCount）/`[+0xF]`（m_ucSingleIdx）比較と `[+0x24/28/2C]` キャッシュを `−0xA7DEB4`（無効値・v187 L131）へリセット → **blend unit の重み計算** [確定]。

### §2.3 駆動元 `0x6C4200`（v382_4(a) unfiltered）

```
6C4200: push esi; mov esi,ecx
6C4203: al = [esi+8]; shr al,3; test al,1; je 0x6C4250   ; ★bit3 ゲート
6C4210: cmp [esi+0x54],0; jbe exit                        ; count
6C4216: ecx = [esi+0x4C]（★sequence ptr 配列）
6C4219: fld [esp+0x10]（time）
6C421D: ebx = [ecx+edi*4]（sequence）
6C4220: push 1; ...; mov ecx,ebx; fstp [esp]; call 0x6CA950  ; ★(time, flag=1)
6C422D: cmp [ebx+0x44],0; jne next                        ; state==0 か?
6C4233: count--; 前方詰め（[eax+edi*4] = [eax+ecx*4]）      ; ★停止 sequence を除去
```
- **E8 callers＝0件**（v382_4(a)）＝virtual/register 経由で呼ばれ、直接呼び出しは無い [確定(否定側)]。挙動から **NiControllerManager の有効 sequence 配列 Update 相当 [推定高]**。

### §2.4 （付随）`0x6C6AC0` = Unregister 経路（v382_3(d) unfiltered）

```
6C6AC0: for i < [edi+0xC]（block 数・stride16）:
6C6AD5: if [eax+8]（field8）==0 skip
6C6ADB: ecx = [eax+8]（field8 interpolator）
6C6ADE: eax = [eax+0xC]（★priority byte＝v381 の結果 byte）
6C6AE4: edx = [ecx]→[edx+0x9C]       ; ★vt+0x9C
6C6AEA: push eax; ...; call edx      ; virtual+0x9C(priority)
6C6B0E..6C6B16: 結果ノードへ push 1; call（先頭 virtual）
```
- **vt+0x9C ＝ `0x6CC6E0` = `UnregisterInterpolator`（v187 L135）** [確定（v187 接続）]。E8 callers＝**`0x6C9D25`（Deactivate 内）の1本**。
- → **Activate（0x6C6A50=Register）／Deactivate（0x6C6AC0=Unregister）の対**が確定。

## §3 残点 **V-382-1..3**
1. **s3（`[seq+0x1C]`）の厳密な命名** — 本裁定で「重み伝播への倍率（`fmul`）」として消費されることを確認したが、**v378/v380 は duration と記載**（実 caller は s3=1.0 のため両義が一致してしまう）。NiControllerSequence フィールド名（duration か weight か）との突合で確定可【中】
2. **`0x6C5FC0`（time remap helper）の正式 identity** — E8 callers=`0x49F4C0`/`0x49F6A5`（game 側）/`0x6C6B4E`（テキストキー同期内）/`0x6CAC00`（重みパス内）。`[+0x34]/[+0x38]` を −const 比較・`[+0x28]` 倍率・`[+0x2C]/[+0x30]` 差分を扱う【中】
3. **`0x6C4200` の所属と初期化元** — NiControllerManager::Update 相当 [推定高]・`[this+8]` bit3・`[this+0x4C]`（sequence 配列）/`[this+0x54]`（count）の構築元【低】

## §4 検証・自己批評
- probe: **v382_1..v382_4 全 EXIT=0**。
- **決定打の作り**: (i) `0x6CC5C0` の E8 callers census（=1＝virtual とは別に Accum wrapper `0x6CE56A` が存在）→ (ii) `[seq+0x1C]/[+0x4C]/[+0x50]` の**読手 census で `0x6CA950` 帯のフェード算出を検出** → (iii) head unfiltered で初期化（start=now/end=now+s4）と `t=(time−start)/(end−start)` を直接確認 → (iv) 駆動元 `0x6C4200` と Deactivate の `0x6C6AC0`（Unregister）を接続。
- 妥当性チェック: ①s4→`[+0x50]`⇔ `0x6CA950` の end 初期化⇔ `fsub`/`fdivp` — ②s3→`[+0x1C]`⇔ `fmul` で重み引数へ — ③weight=0.0 登録⇔毎フレーム上書き（矛盾しない）— ④`0x6C6AC0` の vt+0x9C⇔v187 UnregisterInterpolator — **四方向で矛盾なし**。
- docs との不整合: **v378/v380 の s3=「duration」ラベルは命令上未裏付け**（本裁定で倍率用途を確認・命名は V-382-1 へ）。v187 L132/L135（Register/Unregister）は本裁定で activate/deactivate 経路に接続＝**補強**。
- L1 制約: docs/GT/ESM 変更なし・rust/ 側ソース変更なし。
