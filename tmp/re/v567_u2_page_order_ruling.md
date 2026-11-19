# v567 — V-563-1 クローズ: RaceSexMenu のページ順序の完全対応表（ai-todo #162 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v564_5 / v564_9 / v564_10 / v564_11**・全 EXIT=0）。
先行: `tmp/re/v558_racesexmenu_pages_ruling.md`（U2）／`tmp/re/v563_u2_page_switch_ruling.md`（V-558-1 クローズ）
判定: `[確定(命令)]` / `[部分確定]` / `[確定負]`

---

## 1. 結論

| 問い | 判定 | 回答 |
|---|---|---|
| ページごとの Done 後の行き先（V-563-1） | **[確定(命令)]** | §2 の完全対応表。**源19 ページ → 行き先5種**（Main 確認／Face／Shape／Tone／フォールスルー）。 |
| v563 の記述に誤りはあったか | **[確定負] 訂正** | v563 は「Shape／Tone／**General(Face)** → Face」と書いたが、実測では **General(Face) → Shape**（`0x5CC37E` の次の分岐は `0x5CC3C1` → `0x5CC9C4`=Shape）。**Shape→Face・Tone→Face は正しい**。 |
| 未同定ページ名 `0xB38F90` | **[確定(命令)]** | **`sEyes` / value "Eyes"**（静的登録 `0x9F687A`）。 |
| v558 の「21 ページ」は正しいか | **[部分確定] 補正** | **21 は `AddPane` の呼出数（生成される pane 数）として正しい**。一方、**静的登録バンド `0x9F6000..0x9F8000` には 155 件の名前登録**があり、そのうちページ/セクション名は 21 より多い（`sEyes`/`sAge`/`sComplexion`/`sGender`/`sSkin`/`sLips`/`sStyle` 等）。**「ページ名テーブル＝21」ではない**。残りはフェイスモーフィングのスライダーラベル（`sCheekboneslow` など数十件）とヘアカラー GMST（`sHairColor0..15`/`iHairColor00..`）。 |

---

## 2. ページ → Done 後の行き先（完全対応表）[確定(命令)]

`0x5CC200`（`cmp ebx,0x5A` = id 90 の通知ハンドラ）内で、**現在ページ（`[esi+4]` の `user0`）と各ページの `user0` を比較し、最初に一致した分岐へ飛ぶ**。各ソースの `mov eax/edx,[0xB38xxx]` と、その直後の条件分岐を機械ペアリング（v564_9）。

| # | ソース（現在ページ） | グローバル | 分岐先 | **行き先** |
|---|---|---|---|---|
| 1 | Main | `0xB38F70` | `0x5CC2D0` → `0x5CCA77` | **Main**（名前チェック → 確認ダイアログ → Main 維持） |
| 2 | Shape | `0xB38FE0` | `0x5CC324` → `0x5CCA09` | **Face** |
| 3 | Tone | `0xB38FE8` | `0x5CC378` → `0x5CCA09` | **Face** |
| 4 | General(Face) | `0xB39000` | `0x5CC3C1` → `0x5CC9C4` | **Shape** |
| 5 | Brow | `0xB39008` | `0x5CC40B` → `0x5CC9C4` | **Shape** |
| 6 | Cheeks | `0xB39010` | `0x5CC454` → `0x5CC9C4` | **Shape** |
| 7 | Chin | `0xB39018` | `0x5CC49E` → `0x5CC9C4` | **Shape** |
| 8 | **Eyes** | `0xB38F90` | `0x5CC4E7` → `0x5CC9C4` | **Shape** |
| 9 | Forehead | `0xB39020` | `0x5CC531` → `0x5CC9C4` | **Shape** |
| 10 | Jaw | `0xB39028` | `0x5CC57A` → `0x5CC9C4` | **Shape** |
| 11 | Brow（2 箇所目） | `0xB39008` | → `0x5CC9C4` | **Shape** |
| 12 | Mouth | `0xB39030` | → `0x5CC9C4` | **Shape** |
| 13 | Nose | `0xB39038` | → `0x5CC9C4` | **Shape** |
| 14 | General(Skin) | `0xB39040` | → `0x5CC97F` | **Tone** |
| 15 | Beard | `0xB39050` | → `0x5CC97F` | **Tone** |
| 16 | Eye shades | `0xB39320` | → `0x5CC97F` | **Tone** |
| 17 | Eyebrows | `0xB39058` | → `0x5CC97F` | **Tone** |
| 18 | Mouth(Lips) | `0xB39060` | → `0x5CC97F` | **Tone** |
| 19 | Nose shades | `0xB39328` | → `0x5CC97F` | **Tone** |

**未比較（フォールスルー）**: `Race` / `Hair` / `Face` / `Hair Color` はこの連鎖に無く、末尾 `0x5CC8BA` へ（v563 で「Random 側」と記した経路）。

**読み方（[部分確定]）**: 行き先は「**1 階層上へ戻る**」構造に見える——顔の詳細ページ（Brow/Cheeks/Chin/Eyes/Forehead/Jaw/Mouth/Nose）→ **Shape**、肌・髪の詳細ページ（General(Skin)/Beard/Eye shades/Eyebrows/Mouth(Lips)/Nose shades）→ **Tone**、Shape・Tone → **Face**、Main → 確認。**ただしこの解釈は [部分確定]**（表示順と一致するかは XML/表示側の追加確認が必要）。

---

## 3. 名前テーブルの実測（v564_11）

静的登録バンド `0x9F6000..0x9F8000` で `mov ecx, <0xB38F00..0xB39400>` を**155 件**機械抽出。登録は `push value; push name; mov ecx,<global>; call 0x419A70` の形。

* **ページ/セクション名（一部）**: `sMain`/`sRace`/`sFace`/`sHair`/`sEyes`/`sAge`/`sComplexion`/`sGender`/`sRaceDescription`/`sStyle`/`sRed`/`sGreen`/`sBlue`/`sLength`/`sShape`/`sTone`/`sRandom`/`sReset`/`sGeneralFace`/`sBrow`/`sCheeks`/`sChin`/`sForehead`/`sJaw`/`sMouth`/`sNose`/`sGeneralSkin`/`sSkin`/`sBeard`/`sEyebrows`/`sMouthLips`/`sLips`/`sEyesTex`/`sNoseTex`/`sColor`
* **フェイスモーフィングのスライダーラベル（多数）**: `sFacebrow`「Face brow/nose/chin ratio」/`sCheekboneslow`「Cheekbones low/high」/`sChinforward`/`sNosebridgeshallow`/`sMouthlipslarge` …（計数十件）
* **ヘアカラー**: `sHairColor0..15` ＋ `iHairColor00..`（int 型 GMST）
* **その他 UI 文字列**: `sEntryInstructions1/2`/`sPotion`/`sClass`/`sEnchantedItem`/`sPlayer`/`sSpell`/`sTheElderScrolls`/`sOblivionCaps` …

→ **「ページ名＝21」ではなく、「生成される pane＝21（AddPane 呼出数）」＋「名前登録は155件」が実測値**。v558 の §1.1 は **AddPane の呼出数**として正しいが、**名前テーブルの総数としては [部分確定] 訂正**。

---

## 4. 移植への示唆

1. ページ遷移は**「現在ページ → 行き先」の固定表**（§2）で、**矢印キー等の直接移動は無い**（v563 で確定）。移植がタブ UI を使うならこの表を「タブの前後関係」に写像する必要がある。
2. 行き先は「1 階層上へ戻る」型が大半（顔詳細→Shape、肌詳細→Tone、Shape/Tone→Face）。**「Done＝次へ」ではなく「Done＝1 階層戻る／Main では確認」**。
3. ページ名は GMST（`sEyes` 等）なので、**スライダーラベル（`sCheekboneslow` 等）と混同しないこと**——後者はページではなく**行ラベル**。

---

## 5. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-567-1 | §2 の「1 階層上へ戻る」解釈の確認（表示順 XML / ページの親子構造） | 中 |
| V-567-2 | `0x5CC580`（Brow の2 箇所目）と `0x5CC829`/`0x5CC863`（Main の2 箇所目）が同一コード内で重複する理由 | 低 |
| V-567-3 | フォールスルー先 `0x5CC8BA` の完全 decode（v563 残点 V-563-2 と統合） | 低 |
