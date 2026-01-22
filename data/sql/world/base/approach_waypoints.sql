-- mod-playerbots-betterquesting approach waypoints schema
-- This table stores pre-defined waypoints to help bots navigate to difficult-to-reach locations
-- such as cave entrances, underground areas, and locations where PathGenerator struggles

DROP TABLE IF EXISTS `playerbots_questing_approach_waypoints`;
CREATE TABLE `playerbots_questing_approach_waypoints` (
    `waypointId` INT UNSIGNED AUTO_INCREMENT PRIMARY KEY COMMENT 'Unique waypoint ID',
    `targetType` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=Zone, 1=NPC, 2=Object, 3=Quest, 4=Area',
    `targetId` INT UNSIGNED NOT NULL COMMENT 'Zone/NPC/Object/Quest/Area ID depending on targetType',
    `mapId` SMALLINT UNSIGNED NOT NULL COMMENT 'Map ID for this waypoint',
    `waypointOrder` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Order in waypoint chain (0 = first)',
    `x` FLOAT NOT NULL COMMENT 'World X coordinate',
    `y` FLOAT NOT NULL COMMENT 'World Y coordinate',
    `z` FLOAT NOT NULL COMMENT 'World Z coordinate (critical for caves)',
    `radius` FLOAT NOT NULL DEFAULT 5.0 COMMENT 'Radius at which waypoint is considered reached',
    `description` VARCHAR(100) DEFAULT NULL COMMENT 'Human-readable description',
    INDEX `idx_target` (`targetType`, `targetId`),
    INDEX `idx_map` (`mapId`),
    INDEX `idx_order` (`targetType`, `targetId`, `waypointOrder`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Approach waypoints for difficult navigation';

-- Target type reference:
-- 0 = Zone - waypoints apply to all objectives in the zone
-- 1 = NPC - waypoints apply to reaching a specific NPC entry
-- 2 = Object - waypoints apply to reaching a specific game object entry
-- 3 = Quest - waypoints apply to a specific quest's objectives
-- 4 = Area - waypoints apply to a specific area (AreaTable.dbc)

-- Example data for common problem caves
-- These are manually curated entrance waypoints for known problematic locations

-- Frostmane Hold (Dun Morogh) - multiple cave entrances
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 1, 0, 0, -5605.0, 613.0, 392.0, 8.0, 'Frostmane Hold - Main entrance approach'),
(0, 1, 0, 1, -5623.0, 593.0, 384.0, 5.0, 'Frostmane Hold - Cave entrance');

-- Fargodeep Mine (Elwynn Forest)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 12, 0, 0, -9807.0, 555.0, 37.0, 8.0, 'Fargodeep Mine - Entrance approach'),
(0, 12, 0, 1, -9827.0, 543.0, 33.0, 5.0, 'Fargodeep Mine - Mine entrance');

-- Jasperlode Mine (Elwynn Forest)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 12, 0, 2, -9117.0, -590.0, 59.0, 8.0, 'Jasperlode Mine - Entrance approach'),
(0, 12, 0, 3, -9136.0, -563.0, 57.0, 5.0, 'Jasperlode Mine - Mine entrance');

-- Echo Ridge Mine (Northshire Valley)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 9, 0, 0, -8910.0, -177.0, 81.0, 8.0, 'Echo Ridge Mine - Entrance approach'),
(0, 9, 0, 1, -8922.0, -161.0, 79.0, 5.0, 'Echo Ridge Mine - Mine entrance');

-- Coldridge Valley cave (Dun Morogh - starting area)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 1, 0, 2, -6203.0, 298.0, 389.0, 8.0, 'Coldridge Valley Troll Cave - Entrance approach'),
(0, 1, 0, 3, -6218.0, 325.0, 384.0, 5.0, 'Coldridge Valley Troll Cave - Cave entrance');

-- Wendigo Cave (Dun Morogh)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 1, 0, 4, -5730.0, 168.0, 393.0, 8.0, 'Wendigo Cave - Entrance approach'),
(0, 1, 0, 5, -5753.0, 156.0, 387.0, 5.0, 'Wendigo Cave - Cave entrance');

-- Shadowthread Cave (Teldrassil)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 141, 1, 0, 10340.0, 856.0, 1324.0, 8.0, 'Shadowthread Cave - Entrance approach'),
(0, 141, 1, 1, 10356.0, 875.0, 1321.0, 5.0, 'Shadowthread Cave - Cave entrance');

-- Gnarlpine Hold (Teldrassil)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 141, 1, 2, 9735.0, 1027.0, 1248.0, 8.0, 'Gnarlpine Hold - Entrance approach'),
(0, 141, 1, 3, 9751.0, 1044.0, 1243.0, 5.0, 'Gnarlpine Hold - Cave entrance');

-- Skulk Rock (Hinterlands)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 47, 0, 0, -41.0, -2509.0, 116.0, 8.0, 'Skulk Rock - Entrance approach'),
(0, 47, 0, 1, -34.0, -2535.0, 108.0, 5.0, 'Skulk Rock - Cave entrance');

-- Palemane Rock (Mulgore)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 215, 1, 0, -1453.0, 86.0, 23.0, 8.0, 'Palemane Rock - Entrance approach'),
(0, 215, 1, 1, -1481.0, 99.0, 17.0, 5.0, 'Palemane Rock - Cave entrance');

-- Venture Co Mine (Mulgore)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 215, 1, 2, -1084.0, -1787.0, 58.0, 8.0, 'Venture Co Mine - Entrance approach'),
(0, 215, 1, 3, -1100.0, -1805.0, 52.0, 5.0, 'Venture Co Mine - Mine entrance');

-- Skull Rock (Durotar)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 14, 1, 0, -208.0, -3845.0, 42.0, 8.0, 'Skull Rock - Entrance approach'),
(0, 14, 1, 1, -195.0, -3866.0, 35.0, 5.0, 'Skull Rock - Cave entrance');

-- Thunder Ridge (Durotar)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 14, 1, 2, 78.0, -4595.0, 28.0, 8.0, 'Thunder Ridge - Entrance approach'),
(0, 14, 1, 3, 66.0, -4618.0, 22.0, 5.0, 'Thunder Ridge - Cave entrance');

-- Drygulch Ravine / Dustwind Cave (Durotar)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 14, 1, 4, 724.0, -4545.0, 11.0, 8.0, 'Dustwind Cave - Entrance approach'),
(0, 14, 1, 5, 744.0, -4531.0, 5.0, 5.0, 'Dustwind Cave - Cave entrance');

-- Sarkoth Cave (Deathknell - Tirisfal Glades)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 85, 0, 0, 1866.0, 1627.0, 98.0, 8.0, 'Sarkoth Cave - Entrance approach'),
(0, 85, 0, 1, 1848.0, 1611.0, 93.0, 5.0, 'Sarkoth Cave - Cave entrance');

-- Agamand Family Crypts (Tirisfal Glades)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 85, 0, 2, 2253.0, 277.0, 35.0, 8.0, 'Agamand Crypts - Main entrance approach'),
(0, 85, 0, 3, 2269.0, 295.0, 30.0, 5.0, 'Agamand Crypts - Crypt entrance');

-- Garren's Haunt (Tirisfal Glades)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 85, 0, 4, 2742.0, 704.0, 105.0, 8.0, 'Garren Haunt Cave - Entrance approach'),
(0, 85, 0, 5, 2759.0, 722.0, 99.0, 5.0, 'Garren Haunt Cave - Cave entrance');

-- Stillwater Pond mine (Tirisfal)
INSERT INTO `playerbots_questing_approach_waypoints` (`targetType`, `targetId`, `mapId`, `waypointOrder`, `x`, `y`, `z`, `radius`, `description`) VALUES
(0, 85, 0, 6, 2019.0, 1022.0, 65.0, 8.0, 'Stillwater Pond Mine - Entrance approach'),
(0, 85, 0, 7, 2037.0, 1044.0, 58.0, 5.0, 'Stillwater Pond Mine - Mine entrance');
