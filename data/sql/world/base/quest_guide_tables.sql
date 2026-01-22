-- Quest Guide System Tables for mod-playerbots-betterquesting
-- These tables define leveling guides for bots to follow

-- Guide Definitions
DROP TABLE IF EXISTS `playerbots_questing_quest_guides`;
CREATE TABLE `playerbots_questing_quest_guides` (
    `guideId` INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(100) NOT NULL,
    `description` VARCHAR(255) DEFAULT '',
    `faction` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Alliance, 1=Horde, 2=Both',
    `startingRace` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=any, 1=Human, 2=Orc, 3=Dwarf, 4=NightElf, 5=Undead, 6=Tauren, 7=Gnome, 8=Troll, 10=BloodElf, 11=Draenei',
    `minLevel` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `maxLevel` TINYINT UNSIGNED NOT NULL DEFAULT 80,
    `priority` TINYINT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Higher priority guides selected first',
    `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    PRIMARY KEY (`guideId`),
    KEY `idx_faction_race` (`faction`, `startingRace`),
    KEY `idx_enabled` (`enabled`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Quest guide definitions';

-- Guide Steps
DROP TABLE IF EXISTS `playerbots_questing_guide_steps`;
CREATE TABLE `playerbots_questing_guide_steps` (
    `guideId` INT UNSIGNED NOT NULL,
    `stepOrder` INT UNSIGNED NOT NULL,
    `questId` INT UNSIGNED NOT NULL,
    `minLevel` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `maxLevel` TINYINT UNSIGNED NOT NULL DEFAULT 80,
    `zoneId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Expected zone for this quest (0=any)',
    `isOptional` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Can be skipped if unavailable',
    `skipIfCompleted` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Skip step if already completed',
    PRIMARY KEY (`guideId`, `stepOrder`),
    KEY `idx_guide_quest` (`guideId`, `questId`),
    KEY `idx_quest` (`questId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Quest steps within guides';

-- Zone Transitions
DROP TABLE IF EXISTS `playerbots_questing_zone_transitions`;
CREATE TABLE `playerbots_questing_zone_transitions` (
    `guideId` INT UNSIGNED NOT NULL,
    `fromZoneId` INT UNSIGNED NOT NULL,
    `toZoneId` INT UNSIGNED NOT NULL,
    `triggerLevel` TINYINT UNSIGNED NOT NULL COMMENT 'Level to trigger transition',
    `triggerQuestComplete` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Quest that triggers transition (0=level only)',
    `priority` TINYINT UNSIGNED NOT NULL DEFAULT 100 COMMENT 'Higher priority transitions checked first',
    PRIMARY KEY (`guideId`, `fromZoneId`, `toZoneId`, `triggerLevel`),
    KEY `idx_guide_from` (`guideId`, `fromZoneId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Zone transition triggers within guides';
