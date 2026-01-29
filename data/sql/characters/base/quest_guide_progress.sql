-- Quest Guide Progress Table for mod-playerbots-betterquesting
-- Tracks player progress through quest guides

DROP TABLE IF EXISTS `playerbots_questing_player_guide_progress`;
CREATE TABLE `playerbots_questing_player_guide_progress` (
    `guid` INT UNSIGNED NOT NULL COMMENT 'Character GUID',
    `guideId` INT UNSIGNED NOT NULL COMMENT 'Active guide ID',
    `currentStep` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Current step in guide',
    `activeQuestId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Quest being worked on (persisted across logout)',
    `activeSubStatus` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'QuestingSubStatus enum value (persisted across logout)',
    PRIMARY KEY (`guid`),
    KEY `idx_guide` (`guideId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Player progress through quest guides';
