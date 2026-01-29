-- Add questing activity state persistence columns
-- These columns track the current quest being worked on and its substatus
-- so that questing activity can be restored after logout/login

ALTER TABLE `playerbots_questing_player_guide_progress`
    ADD COLUMN `activeQuestId` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Quest being worked on (persisted across logout)' AFTER `currentStep`,
    ADD COLUMN `activeSubStatus` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'QuestingSubStatus enum value (persisted across logout)' AFTER `activeQuestId`;
