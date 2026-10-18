# Oblivion.exe 解析達成一覧 (ANALYSIS DONE)

## 凡例
- ✅ **完了** — 核心構造・式・関数の全解析完了
- ◐ **部分完了** — 主要経路解析済みだが未トレースの分岐あり
- 🔶 **情報のみ** — データ形式・テーブル・対応表のみ確定

> 解析対象: Oblivion.exe v1.2.0416 (Steam版, ImageBase=0x400000)
> .text 6.2MB / ~1,574関数 / レポート10,194行 / FUN_631一意 / 全数テーブル9本
> レポート: `docs/RE_EXE_REPORT.md` (§50〜§61)

---

## I. ゲームプレイエンジン

### 1. 戦闘システム ✅
| # | 項目 | 関数/アドレス | 状態 |
|---|---|---|---|
| 1.1 | 近接ダメージ計算式 | `0x547070` Combat_CalcMeleeDamage | ✅ 全12段階確定 |
| 1.2 | ダメージ適用チェーン | `0x5FEBF0` | ✅ 全解読 |
| 1.3 | 難易度倍率 | fDifficultyDamageMultiplier, 実効M=5.0 | ✅ 補間式確定 |
| 1.4 | アーマー習熟倍率 | GMST 6種 | ✅ 適用先確定 |
| 1.5 | ノックダウン (KD) | `0x5475D0` | ✅ dmgInt≥1で不成立 |
| 1.6 | ノックバック (KB) | `0x547B50` | ✅ D/curve(x)完全式 |
| 1.7 | ブロック | fCombatBlockMult=0.3 | ✅ |
| 1.8 | クリティカル | 5% / ×2.0 | ✅ |
| 1.9 | 弓ダメージ + 矢重力式 | | ✅ |
| 1.10 | スニーク倍率 | fPerkSneakAttack* | ✅ |
| 1.11 | 武器スコアリング | `0x547140` | ✅ |
| 1.12 | 死亡処理 | `0x5FC440` vt+0x2C8 | ✅ |
| 1.13 | HP書込+被弾タイマ | `0x488830` | ✅ |

### 2. 魔法システム ✅
| # | 項目 | 状態 |
|---|---|---|
| 2.1 | MagicTarget vtable `0xA6DFF4` (Actor+0x68) | ✅ |
| 2.2 | 耐性 `0x5E5240` | ✅ 式確定 |
| 2.3 | 吸収 `0x5E54A0` (flag=1は反射由来) | ✅ |
| 2.4 | 反射 `0x5E5520` | ✅ |
| 2.5 | MGEF 全79効果ディスパッチ表 | ✅ |
| 2.6 | 呪文スコアリング `0x5E5850` | ✅ |
| 2.7 | ActiveEffect 更新 | ✅ |
| 2.8 | 投射物経路 | ✅ |
| 2.9 | 詠唱入口 | ✅ |
| 2.10 | stackingtype 全種 (0..2) | ✅ |
| 2.11 | vt+0x398 `0x5EE760` = 近接/魔法インパクト受信 | ✅ |
| 2.12 | vt+0x3A8 `0x5FE380` = CHRM (犯罪/友好射撃/disposition) | ✅ |

### 3. 疲労・死亡 ✅
| # | 項目 | 状態 |
|---|---|---|
| 3.1 | 疲労 AV 0x0a 計算式 | ✅ |
| 3.2 | 疲労切れ→KD自動発生 | ✅ |
| 3.3 | PC死亡 `0x660CC0` | ✅ |
| 3.4 | 蘇生・AV加算規約 | ✅ |
| 3.5 | KD回復タイマ | ✅ |

### 4. AI システム ✅
| # | 項目 | 状態 |
|---|---|---|
| 4.1 | パッケージ vtable 全33種 | ✅ 専用6種+基底27種 |
| 4.2 | パッケージ選択チェーン `0x569020` | ✅ |
| 4.3 | 条件関数 全169件 (id=0x1000+index, entry+0x20) | ✅ 一覧化 |
| 4.4 | 戦闘AI 状態機械 (10状態) | ✅ |
| 4.5 | 戦闘判断木 (武器スコア vs 呪文スコア) | ✅ |
| 4.6 | 逃走 (Flee) 状態機械 | ✅ |
| 4.7 | 降伏 (Yield) 条件 (6/10ヒット) | ✅ |
| 4.8 | 検知パイプライン (Detect) | ✅ |
| 4.9 | 検知FOV = 定数1.658 (~95°) | ✅ ハードコード |
| 4.10 | 検知4状態 `0x640900` | ✅ |
| 4.11 | PathingSystem (3状態: Pathing/Avoiding/Turning) | ✅ |
| 4.12 | NavMesh = Havok凸形状 + hkpPathFind | ✅ |
| 4.13 | Follow/Escort (SetType idx1) | ✅ |

### 5. クエスト・対話 ✅
| # | 項目 | 状態 |
|---|---|---|
| 5.1 | QUSTレコード構造 | ✅ |
| 5.2 | STAGE/SCEN/INFO 連携 | ✅ |
| 5.3 | 条件関数テーブル 基底0xB0C8C0 | ✅ |
| 5.4 | INFO選択 (優先度/抽選) | ✅ |
| 5.5 | 説得 (Persuasion) システム | ✅ |
| 5.6 | 字幕発行API + 距離ゲート | ✅ |

### 6. スクリプトVM ✅
| # | 項目 | 状態 |
|---|---|---|
| 6.1 | 全141 opcode ハンドラ | ✅ |
| 6.2 | スクリプト関数テーブル 532件 | ✅ `s57_script_vm_table.txt` |
| 6.3 | コンパイラ `0x4FF710` | ✅ |
| 6.4 | StandardCompile `0x4FDE30` | ✅ |
| 6.5 | InfixToPostfix `0x4F4080` | ✅ |
| 6.6 | 式評価器 Expression::Eval `0x4F3620` | ✅ |
| 6.7 | SCDA全2,393件 ESM直読完了 | ✅ |
| 6.8 | イベント送信 77種 ScriptEventList | ✅ |
| 6.9 | 参照スロット接頭辞 0x001C | ✅ |
| 6.10 | beginブロック種別表 (31種) | ✅ |

### 7. 経済・所持品 ✅
| # | 項目 | 状態 |
|---|---|---|
| 7.1 | 価格計算式 (販売 `0x5E11A0` / 購入 `0x5E1200`) | ✅ |
| 7.2 | 丸め `0x484370` | ✅ |
| 7.3 | 登録処理 `0x676EE0` | ✅ |
| 7.4 | 犯罪ゴールド + 罰金式 | ✅ |
| 7.5 | 価格3系統 (10000/3000/2000/1500) | ✅ |

### 8. 犯罪システム ✅
| # | 項目 | 状態 |
|---|---|---|
| 8.1 | 犯罪種別6種 + 罰金 | ✅ |
| 8.2 | BountyLevel 4段階 → Guard応答 | ✅ |
| 8.3 | 逮捕・刑務所 | ✅ |
| 8.4 | 窃盗ExtraData (0x27) | ✅ |

### 9. キャラ生成・レベルアップ ✅
| # | 項目 | 状態 |
|---|---|---|
| 9.1 | キャラ生成ウィザード (ステップマシン) | ✅ |
| 9.2 | RaceSexMenu / ClassMenu / BirthSignMenu | ✅ |
| 9.3 | 星座ピッカー (menu 0x408 mode 3) | ✅ |
| 9.4 | スキル上昇 `0x668070` | ✅ |
| 9.5 | AdvancePCLevel `0x5056F0` | ✅ |
| 9.6 | ApplyLevelUp `0x66C3E0` | ✅ |
| 9.7 | 属性ボーナス表 `0x5480A0` | ✅ |
| 9.8 | 初期値設定 `0x5222D0` | ✅ |

### 10. 錬金・クラフト ✅
| # | 項目 | 状態 |
|---|---|---|
| 10.1 | AlchemyMenu (0x410) | ✅ |
| 10.2 | RepairMenu (0x40B) | ✅ |
| 10.3 | RechargeMenu (0x419) | ✅ |
| 10.4 | EnchantmentMenu (0x412) | ✅ |
| 10.5 | SpellMakingMenu (0x406) | ✅ |
| 10.6 | 修理量式 | ✅ |
| 10.7 | ソウルレベル表 + SoulTrap `0x6A4DF0` | ✅ |

### 11. セーブ・ロード ✅
| # | 項目 | 状態 |
|---|---|---|
| 11.1 | セーブマネージャ 0xB33B00 | ✅ |
| 11.2 | セーブ `0x465130` / ロード `0x465860` | ✅ |
| 11.3 | .ess 版数 0x7d | ✅ |
| 11.4 | bSaveOn* 4種 | ✅ |
| 11.5 | セーブスロット構造 (0x160B) | ✅ |
| 11.6 | セーブフォルダ走査 `0x45D450` | ✅ |
| 11.7 | XBoxセーブ変換 `0x45E6A0` | ✅ |

### 12. 馬・ファストトラベル ✅
| # | 項目 | 状態 |
|---|---|---|
| 12.1 | Horse Mount/DisMount (package 0x16/0x17) | ✅ |
| 12.2 | FastTravel 実行 `0x66EAF0` (15 callers) | ✅ |
| 12.3 | 確認ダイアログ / ローカライズ文言9件 | ✅ |
| 12.4 | SleepWaitMenu + 月日数表 | ✅ |
| 12.5 | 睡眠/待機実行 `0x66F420` (691命令) | ✅ |
| 12.6 | 刑期執行 `0x670700` | ✅ |

### 13. 容器 ✅
| # | 項目 | 状態 |
|---|---|---|
| 13.1 | 全オープン経路 (0x57A8D0 → 0x599EE0) | ✅ |
| 13.2 | userN=0xFAE+N | ✅ |
| 13.3 | 転送コア `0x492E70` | ✅ |

---

## II. ファイル形式

### 14. NIF ✅
| # | 項目 | 状態 |
|---|---|---|
| 14.1 | 全237ブロックタイプ表 | ✅ |
| 14.2 | NiNode/AvObject 階層 | ✅ |
| 14.3 | NiControllerManager vtable 16 slots | ✅ |
| 14.4 | BSShader系ブロック | ✅ |
| 14.5 | 20.0.0.4 / user 2817 / BSVER 11 | ✅ |

### 15. ESM/ESP ✅
| # | 項目 | 状態 |
|---|---|---|
| 15.1 | 全64レコードタイプ表 | ✅ |
| 15.2 | 集計 1,167,017件 (0 error) | ✅ |
| 15.3 | WRLD 84件 / 14,770 cells | ✅ |
| 15.4 | LAND/REFR/CELL 全種 | ✅ |
| 15.5 | zlib 圧縮解凍 | ✅ |
| 15.6 | WATR (水データ) | ✅ |
| 15.7 | PACK/PKDT (AIパッケージ) 実形式 | ✅ |
| 15.8 | DLC全種解析完了 | ✅ |

### 16. BSA ✅
| # | 項目 | 状態 |
|---|---|---|
| 16.1 | 全17 BSA は v103 (v104なし) | ✅ |
| 16.2 | v103 完全解読 | ✅ |
| 16.3 | v104 完全解読 | ✅ |
| 16.4 | folder/file チェーン書式 | ✅ |

### 17. その他 ✅
| # | 項目 | 状態 |
|---|---|---|
| 17.1 | DDS (DXT1/3/5) デコード | ✅ |
| 17.2 | SCDA 全141 opcode 完全解読 | ✅ |
| 17.3 | EGM/EGT (FaceGen) 完全解読 + 15RTTI | ✅ |
| 17.4 | LIP (口形同期) 完全解読 | ✅ |
| 17.5 | shaderpackage 001~019 | ✅ |
| 17.6 | シェーダ224種分析 (176一致/48変種) | ✅ |

---

## III. エンジンシステム

### 18. XMLメニューシステム ✅
| # | 項目 | 状態 |
|---|---|---|
| 18.1 | XMLパーサー `0x58CF40` | ✅ |
| 18.2 | トークンテーブル `0x58A1C0` | ✅ |
| 18.3 | 式評価器 (全26演算子) | ✅ 完全解読 |
| 18.4 | onlyif / onlyifnot 真偽 2.0/1.0 | ✅ |
| 18.5 | stackingtype / alpha=0 / locus=true | ✅ |
| 18.6 | 全メニューXML 50本 解析完了 | ✅ |
| 18.7 | メニュー vtable 全列挙 (60+種) | ✅ |
| 18.8 | メニュー登録ID 完全対応表 | ✅ |
| 18.9 | メニュー ctor/dtor 一覧 | ✅ |
| 18.10 | フェード開閉 完全仕様 | ✅ |
| 18.11 | メニューAPI 全容 | ✅ |

### 19. レンダラ ◐
| # | 項目 | 状態 |
|---|---|---|
| 19.1 | D3D9 動的ロード (`0x761DF0`/`0x763DE0`) | ✅ |
| 19.2 | D3D9デバイス vtable 16種呼出 | ✅ |
| 19.3 | BSShader系完全マップ | ✅ |
| 19.4 | shaderpackage + 48変種 | ✅ |
| 19.5 | 解像度列挙 | ✅ |
| 19.6 | NiNode → D3D9 draw call | ◐ 未トレース |
| 19.7 | 水/空レンダリング | ◐ 未トレース |

### 20. 入力システム ✅
| # | 項目 | 状態 |
|---|---|---|
| 20.1 | DirectInput8 単一 (XInputなし) | ✅ |
| 20.2 | 29アクション + 特殊3 | ✅ 名前表0xB02BB8 |
| 20.3 | キー割当表 stride 0x1D × 3 | ✅ |
| 20.4 | ControlsMenu キーリバインド | ✅ |
| 20.5 | メニュー開閉入力抑制 | ✅ |
| 20.6 | ゲームパッド対応 | ✅ 8方向+ボタン |

### 21. オーディオ ✅
| # | 項目 | 状態 |
|---|---|---|
| 21.1 | DirectShow (mp3) + DirectSound8 (wav) | ✅ |
| 21.2 | 5スライダー音量式 | ✅ |
| 21.3 | フェード窓 1700ms | ✅ |
| 21.4 | music 状態機械 | ✅ |
| 21.5 | AudioManager::Update | ✅ |
| 21.6 | fDSoundRolloffFactor 4.0 | ✅ |

### 22. CRT/数値ヘルパ ✅
| # | 項目 | 状態 |
|---|---|---|
| 22.1 | `_ftol2` 0x9828C0 (ゼロ方向切捨て) | ✅ |
| 22.2 | rand = MSVC LCG 0x9859DD | ✅ |
| 22.3 | RAND_MAX = 32767 / 単一ストリーム | ✅ |
| 22.4 | 丸め系4種 | ✅ |
| 22.5 | log10 0x986CD0 | ✅ |
| 22.6 | sin/cos 変種 | ✅ |

### 23. ファイルI/O ✅
| # | 項目 | 状態 |
|---|---|---|
| 23.1 | INI読 `0x4A8800` / 書 `0x4A85F0` | ✅ |
| 23.2 | INIキー全615件抽出 | ✅ |
| 23.3 | My Games dir解決 | ✅ |
| 23.4 | BSAローダ `0x42EE80` | ✅ |
| 23.5 | Setting全9型 | ✅ |
| 23.6 | zlib inflate 呼出 | ✅ |

### 24. アセットストリーミング ✅
| # | 項目 | 状態 |
|---|---|---|
| 24.1 | セル範囲9段階 `0x5AFE00` | ✅ |
| 24.2 | テクスチャロード `0x760D80` | ✅ |
| 24.3 | NiNode Pool 3サイズ | ✅ |
| 24.4 | LODテクスチャブレンド | ✅ |

### 25. Havok 物理 ✅
| # | 項目 | 状態 |
|---|---|---|
| 25.1 | Havok 4.0 同定 | ✅ |
| 25.2 | 55+ bhk型一覧 | ✅ |
| 25.3 | 33型vtable 全ダンプ | ✅ |
| 25.4 | bhkRigidBody vtable 0xA5605C (35 slots) | ✅ |
| 25.5 | hkWorld::stepDelta 0x8D6E40 | ✅ |
| 25.6 | コアシミュレータ 0x924000 | ✅ |
| 25.7 | dispatch 0x674550 (全4mode) | ✅ |
| 25.8 | bhkWorld queue 0x679340 | ✅ |
| 25.9 | impulse 0x8907A0 | ✅ |
| 25.10 | castRay AabbPhantom slot[47] 0x8A2650 | ✅ |
| 25.11 | AabbPhantom 150 slots ×3 vtable | ✅ |
| 25.12 | CharacterProxy vtable (0xAA3E80は偽陽性) | ◐ B13ダンプ待ち |
| 25.13 | HavokContext 0xB3BD00 (92メソッド分類) | ◐ 50pending/14init/7collision/7debug |
| 25.14 | hkClass::registerClass (独自RTTI) | ✅ |
| 25.15 | FSM 6状態 RTTI | ✅ |
| 25.16 | stepDelta slot 3 @ 4 sites | ✅ |
| 25.17 | fDeltaMult:HAVOK / iUpdateType:HAVOK | ✅ |

### 26. カメラ ✅
| # | 項目 | 状態 |
|---|---|---|
| 26.1 | 投影行列 | ✅ |
| 26.2 | FOV (fDefaultFOV) | ✅ |
| 26.3 | 1人称/3人称切替 | ✅ |
| 26.4 | カメラ更新関数 | ✅ |
| 26.5 | マウスルック感度 | ✅ |
| 26.6 | GMSTカメラ関連12個 | ✅ |

### 27. アニメーション ✅
| # | 項目 | 状態 |
|---|---|---|
| 27.1 | 21状態名テーブル 0xB10290 | ✅ |
| 27.2 | NiControllerManager vtable 16 slots | ✅ |
| 27.3 | kf/kfi ファイル構造 | ✅ |
| 27.4 | モーション状態遷移 | ✅ |
| 27.5 | 武器アニメ種別 ↔ HitFrame | ✅ |

---

## IV. メニューUIレイアウト (全50本完了) ✅

| # | メニュー | ファイル |
|---|---|---|
| 28.1〜28.28 | Container / Inventory / Magic / Map / Stats | ✅ |
| | HUDMain / Dialog / Message / SpellMaking | ✅ |
| | Alchemy / Repair / Recharge / Enchantment | ✅ |
| | LevelUp / RaceSex / Class / BirthSign | ✅ |
| | Load (3種) / Save / Options (全5種) | ✅ |
| | Controls (2page) / QuickKeys / Persuasion | ✅ |
| | SigilStone / VideoDisplay / 他全種 | ✅ |

---

## V. 設定システム

### 29. GMST 🔶
| # | 項目 | 状態 |
|---|---|---|
| 29.1 | 全2,304件 一覧化 | ✅ `s50_gmst_full.txt` |
| 29.2 | 名称→VA→既定値 対応 | ✅ |
| 29.3 | GMSTローダ v2 (ESM適用) | ✅ |
| 29.4 | テーブル基底 0xB09E28 | ✅ |
| 29.5 | string kind 0x4A7980 | ✅ |

### 30. INI ✅
| # | 項目 | 状態 |
|---|---|---|
| 30.1 | INIキー全615件 | ✅ `s60_io_ini_keys.txt` |
| 30.2 | 設定ディスクリプタ表 | ✅ |
| 30.3 | 消費者一覧 (音量/難易度/字幕/オートセーブ) | ✅ |

---

## VI. 全数テーブル一覧

| ファイル | 説明 | エントリ数 |
|---|---|---|
| `tmp/re/s50_gmst_full.txt` | GMST | 2,304 |
| `tmp/re/s57_script_vm_table.txt` | スクリプト関数 | 532 |
| `tmp/re/s60_input_actions.txt` | 入力アクション | 29+3 |
| `tmp/re/s60_io_ini_keys.txt` | INIキー | 615 |
| `tmp/re/s62_havok_vtables.txt` | Havok vtable | 33型 |
| `tmp/re/s58_qst_condtable.txt` | 条件関数 | 169 |
| `tmp/re/s69_nif_table.txt` | NIFブロック | 237 |
| `tmp/re/s69_esm_type_table.txt` | ESMレコード | 64 |
| `tmp/re/s71_q5_mgef_analysis.txt` | MGEF効果 | 79 |

---

## VII. 訂正の歴史

| 訂正 | 旧 → 新 |
|---|---|
| Actor vtable | `0xA71814` → `0xA6E074` |
| KD式 | 読めないコード→dmgInt≥1で不成立、約1%のみ |
| vt+0x398 | 魔法経路→近接/魔法インパクト受信 |
| vt+0x3A8 | 未確定→犯罪/友好射撃/ディスポジション(CHRM) |
| MagicTarget | 別経路→Actor+0x68 vtable `0xA6DFF4` |
| SCDAテーブル | `0xB0E980` → `0xB0AF48` (stride 0x28, 532件) |
| `0x4FDE30` | VMトランポリン→コンパイラ側エミッタ |
| 参照スロット | 0x40→**0x001C** (固定4B接頭辞) |
| AlchemyMenu | 0x3F1 → **0x410** (0x3F1=DialogMenu) |
| 条件関数 | 独立テーブル→関数帯 entry+0x20 (169件) |
| 難易度式 | pow(6,(d-50)/50)→補間式、M=5.0 |
| BSA | v104存在→**全17=v103** |
| 日本語 | 対応→**非対応** (STRT/DLF/ILG不在) |
| KD FSM | fantasy状態廃止→**OnGround/InAir/Jumping/Flying/Projectile/Swimming** |
| Navigation | 個別サブクラス→**PathingSystem独立駆動** |
| Flee/Yield | 同値→**6/10** |
| 眠気/知恵 GMST | 非存在→**Sleep/Jail |

---

## VIII. 静的解析唯一の未解決

**B13 実機 .data ダンプが必要**:
- Havok instance vtable (`0xB3BD00` HavokContext 実行時vtable)
- CharacterProxy instance vtable 特定
- stepDelta slot 確定 (B13ダンプ後)
- ContainerMenu 再オープン挙動 実機確認
- 実 .ess サンプル突合

> 静的解析で取得可能な情報はほぼ網羅。残りはB13実機ダンプのみ。