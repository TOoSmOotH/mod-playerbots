-- BetterQuesting schema for playerbots database
-- Cave candidates table for cave entrance detection

CREATE TABLE IF NOT EXISTS `playerbots_questing_cave_candidates` (
    `caveId` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    `mapId` SMALLINT UNSIGNED NOT NULL,
    `zoneId` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    `centerX` FLOAT NOT NULL,
    `centerY` FLOAT NOT NULL,
    `caveZ` FLOAT NOT NULL COMMENT 'Average Z of underground spawns',
    `surfaceZ` FLOAT NOT NULL COMMENT 'Estimated surface Z',
    `depth` FLOAT NOT NULL COMMENT 'Depth below surface',
    `spawnCount` INT UNSIGNED NOT NULL COMMENT 'Number of underground spawns',
    `sampleNames` VARCHAR(255) DEFAULT NULL COMMENT 'Sample NPC/object names',
    `verified` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=unverified, 1=verified cave, 2=false positive',
    `entranceX` FLOAT DEFAULT NULL,
    `entranceY` FLOAT DEFAULT NULL,
    `entranceZ` FLOAT DEFAULT NULL,
    INDEX `idx_map_zone` (`mapId`, `zoneId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Cave candidates for approach waypoints';
