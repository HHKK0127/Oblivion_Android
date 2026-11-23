# v649 — family 28 サイト / 定数 / chain（最小記録）

- **日付**: 2026-11-23
- **帯域**: HandleClick `FUN_005cc200`（menu id 0x40C RaceSexMenu）
- **読み取り宣言**: 探索 = rizin 0.9.1（`D:\re\r2q.sh`）。デコード最終確認 = capstone。対象 md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`。範囲は半開区間。probe u150_1〜u150_5。

## 記録

- **`[確定(命令)]`** 28 サイトの全アドレスと pane/tile 対。欠けていた 1 組 = **`(Reset, Face)` @ `0x5CCC87`**（`0x5CCC1C mov eax,[0xb38ff8]`=Reset / `0x5CCC42 mov eax,[0xb38f80]`=Face）。

| pane | tile（fucompp） |
|---|---|
| Main | Age(`0x5CCD26`) / Face(`0x5CCF8B`) / Hair(`0x5CD136`) / Race(`0x5CD1B6`) |
| Race | Gender(`0x5CCF22`) |
| Hair | Red(`0x5CCD96`) / Green(`0x5CCDF7`) / Blue(`0x5CCE58`) / Length(`0x5CCEB9`) |
| Face | Shape(`0x5CD02A`) / Tone(`0x5CD0B0`) |
| Shape | General(Face)(`0x5CD236`) / Brow(`0x5CD2BC`) / Cheeks(`0x5CD342`) / Chin(`0x5CD3C8`) / Eyes(`0x5CD44E`) / Forehead(`0x5CD4D4`) / Jaw(`0x5CD55A`) / Mouth(`0x5CD5E0`) / Nose(`0x5CD666`) |
| Tone | General(Skin)(`0x5CD6EC`) / Beard(`0x5CD772`) / Eye shades(`0x5CD7F8`) / Eyebrows(`0x5CD87E`) / Mouth(Lips)(`0x5CD904`) / Nose shades(`0x5CD98A`) |
| Random | Face(`0x5CC92F`) |
| Reset | Face(`0x5CCC87`) |

- **`[確定(命令)]`** `0x5cd9c5` は定数を読まない（`[0xb3b4c9]=0`／条件付き `[0xb3b4c8]=0` → `[0xb333c4].vtable[0x154]()` → `0x7b4280`/`0x7c5e70`/`0x7c7eb0` → `ret 8`）。切替は **`0x5c9650`**（`0x5CCFCC`）= `SetFloat(race_button_done,0xFB3=user5,2.0)` ＋ `SetFloat(this->tile,0xFAE=user0,GetFloat(pane(name),0xFA8))`。
- **`[確定(命令)]`** 定数: `[esp+0x3c]` = 偶数 `0xa,0xc,…,0x30`（20 個）／`[esp+0x48]` = 奇数 `3,4,5,6,7,8,9,0xb,…,0x2f`（26 個）。合成で `3`〜`0x30` 欠番なし。**28 中 8 サイト（Random/Reset/Main-Age/Hair×4/Race-Gender）は定数を設定せず `jmp 0x5cd9c5` 直行**。
- **`[確定(命令)]`** ループではなく**直列チェーン**（`jp <次ブロック>` で連結、一致時のみ `0x5ccfb7`／`0x5cd9c5` へ脱出）。`[esp+0x3c]` の更新は各ブロックの一致分岐内で 1 回。
- **`[確定(命令)]`** `0x5c9650` は Main 以外のページでも到達（`0x5CCFCC` の到達元 = family ブロックの一致。Hair/Race/Shape/Tone/Face の各ページに存在）。
- **`[未確定]`** V-649-1: 定数 `[esp+0x3c]`/`[esp+0x48]` は HandleClick 内に読者 0（`24 3c`/`24 48` の全ヒットが `c74424xx imm` の書込のみ）。関数外（callers 側）か、未使用か。
