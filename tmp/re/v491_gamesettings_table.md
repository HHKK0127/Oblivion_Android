# v491 — GameSetting static entry table（122件・nearest-push 正確版）

- 出典: v490 残点（key 台帳の row 化）／probe **v491_1**
- 方式: `mov ecx, <0xB37xxx>` ＋ `call 0x419A70`（insert）の site を列挙し、**fn 境界（CC/C3）から `mov ecx` 直前まで**の push のうち最後の rdata 文字列 push を key として取得（nearest-push＝v489 window ずれの訂正版）
- value: `mov ecx` 前の `push imm`（0..0xFFFF）を併記（取得できたもののみ）
- 型 prefix: `i`=int / `f`=float / `s`=string [推定相当]（v487/v488）

| addr | key | value(push imm) |
|---|---|---|
| 0xB37018 | iMarksmanFatigueBurnPerSecondSkill | 0x14 |
| 0xB370C0 | iAimingNumIterations | 0xA |
| 0xB370C8 | iArrowInventoryChance | 0x32 |
| 0xB370D0 | iArrowMaxRefCount | 0xF |
| 0xB37150 | sHitParticleStone |  |
| 0xB37158 | sHitParticleCloth |  |
| 0xB37160 | sHitParticleDirt |  |
| 0xB37168 | sHitParticleGlass |  |
| 0xB37170 | sHitParticleGrass |  |
| 0xB37178 | sHitParticleMetal |  |
| 0xB37180 | sHitParticleOrganic |  |
| 0xB37188 | sHitParticleSkin |  |
| 0xB37190 | sHitParticleWater |  |
| 0xB37198 | sHitParticleWood |  |
| 0xB371A0 | sHitParticleChain |  |
| 0xB371A8 | sHitParticleSnow |  |
| 0xB371B0 | sBloodParticleDefault |  |
| 0xB371B8 | sBloodParticleExtra1 |  |
| 0xB371C0 | sBloodParticleExtra2 |  |
| 0xB371C8 | sBloodTextureDefault |  |
| 0xB371D0 | sBloodTextureExtra1 |  |
| 0xB371D8 | sBloodTextureExtra2 |  |
| 0xB37200 | iAINPCRacePowerChance |  |
| 0xB37208 | sNormalWeaponsResisted |  |
| 0xB37210 | iCombatHighPriorityModifier | 0x64 |
| 0xB37228 | iPerkAttackDisarmChance |  |
| 0xB37230 | iPerkBlockDisarmChance |  |
| 0xB37238 | iPerkBlockStaggerChance |  |
| 0xB37240 | iPerkMarksmanKnockdownChance |  |
| 0xB37248 | iPerkMarksmanParalyzeChance |  |
| 0xB37250 | iPerkHandToHandBlockRecoilChance | 0x19 |
| 0xB37260 | iPerkLightArmorMasterMinSum |  |
| 0xB37270 | iCombatCastDrainMinimumValue |  |
| 0xB372B0 | iAICombatMaxAllySummonCount |  |
| 0xB372D0 | iAICombatRestoreHealthPercentage | 0x1E |
| 0xB372D8 | iAICombatRestoreMagickaPercentage | 0x1E |
| 0xB372E0 | iAICombatRestoreFatiguePercentage | 0x1E |
| 0xB372F0 | iAICombatMinDetection |  |
| 0xB372F8 | sActivateNPCCalmed |  |
| 0xB37300 | sActivateCreatureCalmed |  |
| 0xB37388 | iActorLuckSkillBase |  |
| 0xB373A0 | iArmorBaseSkill | 0x32 |
| 0xB374C0 | iPerkHeavyArmorJumpSum | 0x23 |
| 0xB374C8 | iPerkHeavyArmorSinkSum | 0x23 |
| 0xB37518 | iHorseTurnDegreesPerSecond | 0x2D |
| 0xB37520 | iHorseTurnDegreesRampUpPerSecond | 0x50 |
| 0xB375E0 | iBarterDispositionPenalty |  |
| 0xB375E8 | iPerkExtraBarterGoldMaster | 0xC8 |
| 0xB37618 | iLevelUpSkillCount | 0xA |
| 0xB37620 | iLevelUp01Mult |  |
| 0xB37628 | iLevelUp02Mult |  |
| 0xB37630 | iLevelUp03Mult |  |
| 0xB37638 | iLevelUp04Mult |  |
| 0xB37640 | iLevelUp05Mult |  |
| 0xB37648 | iLevelUp06Mult |  |
| 0xB37650 | iLevelUp07Mult |  |
| 0xB37658 | iLevelUp08Mult |  |
| 0xB37660 | iLevelUp09Mult |  |
| 0xB37668 | iLevelUp10Mult |  |
| 0xB37688 | iCrimeGoldAttack | 0x28 |
| 0xB37690 | iCrimeGoldMurder | 0x3E8 |
| 0xB376A0 | iCrimeGoldPickpocket | 0x19 |
| 0xB376A8 | iCrimeGoldTresspass |  |
| 0xB376B0 | iCrimeGoldStealHorse | 0x19 |
| 0xB376B8 | iCrimeGoldMinValue | 0xA |
| 0xB376F0 | iCrimeDaysInPrisonMod | 0x64 |
| 0xB37728 | iTrainingSkills |  |
| 0xB37850 | iPersuasionPower1 |  |
| 0xB37858 | iPersuasionPower2 | 0x14 |
| 0xB37860 | iPersuasionPower3 | 0x28 |
| 0xB37868 | iPersuasionAngleMin | 0xA |
| 0xB37870 | iPersuasionAngleMax | 0x5A |
| 0xB37878 | iPersuasionBribeGold | 0xA |
| 0xB37880 | iPersuasionDemandGold | 0xA |
| 0xB37888 | iPersuasionOuter | 0x32 |
| 0xB37890 | iPersuasionMiddle | 0x19 |
| 0xB37898 | iPersuasionInner | 0xF |
| 0xB378A0 | iPersuasionDemandDisposition |  |
| 0xB378B0 | iDeathDropWeaponChance | 0x1E |
| 0xB37958 | iPersuasionBribeScale |  |
| 0xB37960 | iPersuasionBribeRefuse | 0x3C |
| 0xB37968 | iPersuasionBribeCrime | 0x5A |
| 0xB37970 | iPersuasionDemandScale |  |
| 0xB37978 | iPersuasionDemandRefuse | 0x4B |
| 0xB37A70 | iPCStartSpellSkillLevel | 0x1E |
| 0xB37B38 | sSplashParticles |  |
| 0xB37BC0 | iMapMarkerRevealDistance | 0x3E8 |
| 0xB37BC8 | iMapMarkerVisibleDistance | 0x4E20 |
| 0xB37BD8 | iLowLevelNPCMaxLevel |  |
| 0xB37BE0 | iNPCBasePerLevelHealthMult |  |
| 0xB37C48 | iClassAcrobat |  |
| 0xB37C50 | iClassAgent |  |
| 0xB37C58 | iClassAssassin |  |
| 0xB37C60 | iClassPilgrim | 0x832 |
| 0xB37C68 | iClassKnight | 0x836 |
| 0xB37C70 | iClassHealer | 0x910 |
| 0xB37C78 | iClassMage |  |
| 0xB37C80 | iClassBard |  |
| 0xB37C88 | iPlayerCustomClass |  |
| 0xB37C90 | iClassPriest |  |
| 0xB37C98 | iClassWarrior |  |
| 0xB37CA0 | iClassBarbarian |  |
| 0xB37CA8 | iClassCrusader |  |
| 0xB37CB0 | iClassScout |  |
| 0xB37CB8 | iClassArcher |  |
| 0xB37CC0 | iClassRogue |  |
| 0xB37CC8 | iClassThief |  |
| 0xB37CD0 | iClassMonk |  |
| 0xB37CD8 | iClassSorcerer |  |
| 0xB37CE0 | iClassBattlemage |  |
| 0xB37CE8 | iClassWitchhunter |  |
| 0xB37CF0 | iClassSpellsword |  |
| 0xB37CF8 | iClassNightblade |  |
| 0xB37D00 | iClassCharactergenClass |  |
| 0xB37D18 | iNumberActorsGoThroughLoadDoorInCombat |  |
| 0xB37D30 | iActivatePickLength | 0x96 |
| 0xB37D48 | iRemoveExcessDeadCount | 0xF |
| 0xB37D50 | iRemoveExcessDeadTotalActorCount | 0x14 |
| 0xB37D60 | iRemoveExcessDeadComplexCount |  |
| 0xB37D68 | iRemoveExcessDeadComplexTotalActorCount | 0x14 |
| 0xB37D80 | iMerchantRespawnDay1 |  |
| 0xB37D88 | iMerchantRespawnDay2 |  |

- 件数: 122（insert-style 122 と一致）／key 取得 122 件
