-- Human Leveling Guide (1-10) - Elwynn Forest + Dun Morogh Detour
-- Based on RestedXP-style guide - VERIFIED QUEST IDs
-- Zone IDs: 12=Elwynn Forest, 1=Dun Morogh, 1537=Ironforge, 1519=Stormwind, 38=Loch Modan

-- Delete existing guide if present (for re-import)
DELETE FROM `playerbots_questing_quest_guides` WHERE `guideId` = 1;
DELETE FROM `playerbots_questing_guide_steps` WHERE `guideId` = 1;
DELETE FROM `playerbots_questing_zone_transitions` WHERE `guideId` = 1;

-- Insert the guide definition
INSERT INTO `playerbots_questing_quest_guides` (`guideId`, `name`, `description`, `faction`, `startingRace`, `minLevel`, `maxLevel`, `priority`, `enabled`) VALUES
(1, 'Human 1-10 Optimal', 'Leveling guide for Human characters 1-10 with Dun Morogh detour for flight paths', 0, 1, 1, 10, 100, 1);

-- ============================================
-- NORTHSHIRE VALLEY (Level 1-6) - Zone 12
-- ============================================
INSERT INTO `playerbots_questing_guide_steps` (`guideId`, `stepOrder`, `questId`, `minLevel`, `maxLevel`, `zoneId`, `isOptional`, `skipIfCompleted`) VALUES
-- Initial quests at Northshire Abbey
(1, 1, 783, 1, 80, 12, 0, 1),    -- A Threat Within (starter from Deputy Willem)
(1, 2, 7, 1, 80, 12, 0, 1),      -- Kobold Camp Cleanup (from Marshal McBride)
(1, 3, 5261, 1, 80, 12, 0, 1),   -- Eagan Peltskinner (from Deputy Willem)
(1, 4, 33, 1, 80, 12, 0, 1),     -- Wolves Across the Border (from Eagan Peltskinner)

-- After killing wolves and kobolds, get more quests
(1, 5, 15, 1, 80, 12, 0, 1),     -- Investigate Echo Ridge (follow-up from McBride)
(1, 6, 18, 1, 80, 12, 0, 1),     -- Brotherhood of Thieves (from Deputy Willem)

-- Class letter quests (optional - one per class)
(1, 7, 3100, 1, 80, 12, 1, 1),   -- Simple Letter (Warrior)
(1, 8, 3101, 1, 80, 12, 1, 1),   -- Consecrated Letter (Paladin)
(1, 9, 3102, 1, 80, 12, 1, 1),   -- Encrypted Letter (Mage)
(1, 10, 3103, 1, 80, 12, 1, 1),  -- Hallowed Letter (Priest)
(1, 11, 3104, 1, 80, 12, 1, 1),  -- Glyphic Letter (Rogue)
(1, 12, 3105, 1, 80, 12, 1, 1),  -- Tainted Letter (Warlock)

-- Continue main questline
(1, 13, 21, 1, 80, 12, 0, 1),    -- Skirmish at Echo Ridge (after Investigate)
(1, 14, 3903, 1, 80, 12, 0, 1),  -- Milly Osworth (from Deputy Willem)
(1, 15, 6, 1, 80, 12, 0, 1),     -- Bounty on Garrick Padfoot (from Deputy Willem)
(1, 16, 3904, 1, 80, 12, 0, 1),  -- Milly's Harvest (from Milly Osworth)
(1, 17, 3905, 1, 80, 12, 0, 1),  -- Grape Manifest (from Milly after harvest)
(1, 18, 54, 5, 80, 12, 0, 1),    -- Report to Goldshire (breadcrumb from McBride)
(1, 19, 2158, 5, 80, 12, 0, 1),  -- Rest and Relaxation (from Falkhaan at gate)

-- ============================================
-- GOLDSHIRE (brief stop) - Zone 12
-- Turn in Report to Goldshire and Rest and Relaxation
-- Then teleport to Kharanos in Dun Morogh
-- ============================================

-- ============================================
-- DUN MOROGH DETOUR (Level 6-10) - Zone 1
-- Set hearth in Kharanos, get Ironforge FP
-- ============================================
(1, 20, 384, 5, 80, 1, 0, 1),    -- Beer Basted Boar Ribs (Ragnar Thunderbrew, Kharanos)
(1, 21, 400, 5, 80, 1, 0, 1),    -- Tools for Steelgrill (Tharek Blackstone, Kharanos)
(1, 22, 5541, 5, 80, 1, 0, 1),   -- Ammo for Rumbleshot (Loslor Rudge, Steelgrill's Depot)
(1, 23, 317, 5, 80, 1, 0, 1),    -- Stocking Jetsteam (Pilot Bellowfiz, Steelgrill's Depot)
(1, 24, 313, 5, 80, 1, 0, 1),    -- The Grizzled Den (Pilot Stonegear, Steelgrill's Depot)

-- Do Wendigo cave, boar/bear kills, then turn in
(1, 25, 318, 6, 80, 1, 0, 1),    -- Evershine (follow-up from Stocking Jetsteam)

-- Brewnall Village quests
(1, 26, 310, 6, 80, 1, 0, 1),    -- Bitter Rivals (Marleth Barleybrew)
(1, 27, 315, 6, 80, 1, 0, 1),    -- The Perfect Stout (Rejold Barleybrew)
(1, 28, 319, 6, 80, 1, 0, 1),    -- A Favor for Evershine (Rejold Barleybrew)
(1, 29, 320, 6, 80, 1, 0, 1),    -- Return to Bellowfiz (after Favor for Evershine)
(1, 30, 311, 6, 80, 1, 0, 1),    -- Return to Marleth (after Bitter Rivals)
(1, 31, 413, 7, 80, 1, 0, 1),    -- Shimmer Stout (after Perfect Stout)

-- Kharanos area quests
(1, 32, 287, 6, 80, 1, 0, 1),    -- Frostmane Hold (Senir Whitebeard)
(1, 33, 412, 6, 80, 1, 0, 1),    -- Operation Recombobulation (Razzle Sprysprocket)

-- Frostmane Hold cave, then Gnomeregan for Leper Gnomes
(1, 34, 291, 7, 80, 1, 0, 1),    -- The Reports (after Frostmane Hold)

-- Amberstill Ranch
(1, 35, 314, 7, 80, 1, 0, 1),    -- Protecting the Herd (Rudra Amberstill)

-- Gol'Bolar Quarry
(1, 36, 433, 8, 80, 1, 0, 1),    -- The Public Servant (Senator Mehr Stonehallow)
(1, 37, 432, 8, 80, 1, 0, 1),    -- Those Blasted Troggs! (Foreman Stonebrow)

-- North Gate Outpost
(1, 38, 419, 8, 80, 1, 0, 1),    -- The Lost Pilot (Pilot Hammerfoot)
(1, 39, 417, 8, 80, 1, 0, 1),    -- A Pilot's Revenge (kill Mangeclaw)

-- South Gate Outpost -> Loch Modan
(1, 40, 414, 8, 80, 1, 0, 1),    -- Stout to Kadrell (Mountaineer Barleybrew)

-- ============================================
-- LOCH MODAN (brief FP grab) - Zone 38
-- Turn in Stout to Kadrell at Thelsamar, get flight path
-- ============================================

-- ============================================
-- IRONFORGE (turn-ins) - Zone 1537
-- Turn in The Reports to Senator Barin Redstone
-- ============================================

-- ============================================
-- RETURN TO GOLDSHIRE/ELWYNN - Zone 12
-- Bot will teleport back from Loch Modan/Ironforge
-- Now we pick up the quests we skipped earlier
-- ============================================
(1, 41, 47, 7, 80, 12, 0, 1),    -- Gold Dust Exchange (Remy "Two Times", Goldshire)
(1, 42, 60, 7, 80, 12, 0, 1),    -- Kobold Candles (William Pestle, Goldshire)
(1, 43, 85, 7, 80, 12, 0, 1),    -- Lost Necklace ("Auntie" Bernice Stonefield)
(1, 44, 112, 7, 80, 12, 0, 1),   -- Collecting Kelp (follow-up to Lost Necklace)
(1, 45, 40, 7, 80, 12, 0, 1),    -- A Fishy Peril (Remy "Two Times")
(1, 46, 151, 7, 80, 12, 1, 1),   -- Poor Old Blanchy (optional - oats quest)

-- Marshal Dughan quests
(1, 47, 37, 8, 80, 12, 0, 1),    -- Find the Lost Guards (Marshal Dughan)
(1, 48, 45, 8, 80, 12, 0, 1),    -- Discover Rolf's Fate (after Lost Guards)
(1, 49, 71, 8, 80, 12, 0, 1),    -- Report to Thomas (after Rolf's Fate)

-- Mine quests
(1, 50, 62, 8, 80, 12, 0, 1),    -- The Fargodeep Mine (Marshal Dughan)
(1, 51, 76, 9, 80, 12, 0, 1),    -- The Jasperlode Mine (follow-up)

-- Westbrook Garrison
(1, 52, 239, 9, 80, 12, 0, 1),   -- Westbrook Garrison Needs Help! (Marshal Dughan)
(1, 53, 11, 9, 80, 12, 0, 1),    -- Riverpaw Gnoll Bounty (Deputy Rainer)
(1, 54, 52, 9, 80, 12, 0, 1),    -- Protect the Frontier (Guard Thomas)

-- Final Elwynn quests
(1, 55, 176, 10, 80, 12, 0, 1),  -- Wanted: "Hogger" (Wanted Poster)
(1, 56, 114, 10, 80, 12, 0, 1),  -- The Escape (Maybell Maclure)

-- Breadcrumbs to Westfall
(1, 57, 36, 10, 80, 12, 0, 1),   -- Westfall Stew (Salma Saldean breadcrumb)
(1, 58, 12, 10, 80, 40, 0, 1);   -- The People's Militia (Gryan Stoutmantle, Westfall)

-- ============================================
-- ZONE TRANSITIONS
-- ============================================
INSERT INTO `playerbots_questing_zone_transitions` (`guideId`, `fromZoneId`, `toZoneId`, `triggerLevel`, `triggerQuestComplete`, `priority`) VALUES
-- Elwynn (Northshire) to Goldshire after finishing Northshire
(1, 12, 12, 5, 54, 100),         -- Internal Elwynn move (Northshire -> Goldshire)
-- Goldshire to Dun Morogh (bot will teleport due to distance)
(1, 12, 1, 5, 2158, 95),         -- After Rest and Relaxation, teleport to Kharanos
-- Dun Morogh to Loch Modan (for FP and turn-in)
(1, 1, 38, 8, 414, 80),          -- After getting Stout to Kadrell quest
-- Loch Modan back to Elwynn (bot will teleport back to Goldshire)
(1, 38, 12, 9, 0, 70),           -- After Loch Modan, teleport back to Goldshire
-- Elwynn to Westfall (final transition)
(1, 12, 40, 11, 176, 50);        -- After Hogger, head to Westfall
