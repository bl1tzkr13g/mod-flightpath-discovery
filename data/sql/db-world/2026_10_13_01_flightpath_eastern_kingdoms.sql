
-- ============================================================
-- Flight Path Discovery - Eastern Kingdoms
-- AzerothCore WotLK 3.3.5a
-- ============================================================
-- Curated neighboring flight-path discovery network.
--
-- Faction audit:
-- Removes routes connecting opposing Alliance/Horde
-- flight masters, including:
--   Tarren Mill <-> Southshore
--   Refuge Pointe <-> Hammerfall
--   Thelsamar <-> Kargath
--   Grom'gol <-> Rebel Camp
--
-- Neutral and special flight points still require testing.
-- Northrend and Outland are not included.
-- ============================================================

-- ------------------------------------------------------------
-- EASTERN KINGDOMS ROUTES
-- ------------------------------------------------------------

INSERT INTO mod_flightpath_discovery_routes
(source_node, destination_node)
VALUES
(2, 4),
(2, 5),
(2, 6),
(2, 12),

(4, 2),
(4, 12),
(4, 195),

(5, 2),
(5, 8),
(5, 12),
(5, 71),

(6, 2),
(6, 7),
(6, 8),
(6, 74),

(7, 6),
(7, 8),
(7, 14),
(7, 16),
(7, 43),

(8, 5),
(8, 6),
(8, 7),
(8, 74),

(9, 20),
(9, 195),

(10, 11),
(10, 13),

(11, 10),
(11, 13),

(12, 2),
(12, 4),
(12, 5),
(12, 45),
(12, 195),

(13, 10),
(13, 11),
(13, 17),

(14, 7),
(14, 16),
(14, 66),

(16, 7),
(16, 14),
(16, 43),

(17, 13),
(17, 76),

(20, 9),

(21, 70),
(21, 75),

(43, 7),
(43, 16),

(45, 12),
(45, 71),

(56, 70),

(66, 14),
(66, 67),

(67, 66),

(70, 21),
(70, 56),

(71, 5),
(71, 45),

(74, 6),
(74, 8),

(75, 21),

(76, 17),

(195, 4),
(195, 9),
(195, 12)

ON DUPLICATE KEY UPDATE
destination_node = VALUES(destination_node);


-- ------------------------------------------------------------
-- REMOVE INCORRECT CROSS-FACTION ROUTES
-- ------------------------------------------------------------
-- Also cleans up existing databases where the older
-- Eastern Kingdoms route SQL was previously imported.
-- ------------------------------------------------------------

DELETE FROM mod_flightpath_discovery_routes
WHERE (source_node, destination_node) IN
(
(8, 21),
(13, 14),
(13, 66),
(14, 13),
(16, 17),
(17, 16),
(20, 195),
(21, 8),
(21, 74),
(43, 76),
(45, 56),
(56, 45),
(56, 71),
(66, 13),
(67, 68),
(68, 67),
(70, 71),
(71, 56),
(71, 70),
(74, 21),
(74, 75),
(75, 74),
(76, 43),
(195, 20)
);


-- ------------------------------------------------------------
-- EASTERN KINGDOMS DESTINATION PRICES
-- ------------------------------------------------------------
-- Prices stored in copper.
--
-- 10 silver        =  1000 copper
-- 25 silver        =  2500 copper
-- 50 silver        =  5000 copper
-- 75 silver        =  7500 copper
-- 1 gold 50 silver = 15000 copper
-- 3 gold           = 30000 copper
-- 5 gold           = 50000 copper
-- ------------------------------------------------------------

INSERT INTO mod_flightpath_discovery_prices
(taxi_node_id, price_copper)
VALUES
(2, 5000),
(4, 1000),
(5, 2500),
(6, 5000),
(7, 2500),
(8, 1000),
(9, 7500),
(10, 1000),
(11, 5000),
(12, 2500),
(13, 2500),
(14, 2500),
(16, 7500),
(17, 7500),
(20, 7500),
(21, 15000),
(43, 15000),
(45, 30000),
(56, 15000),
(66, 30000),
(67, 50000),
(68, 50000),
(70, 30000),
(71, 30000),
(74, 30000),
(75, 30000),
(76, 15000),
(195, 7500)

ON DUPLICATE KEY UPDATE
price_copper = VALUES(price_copper);

-- ============================================================
-- END OF EASTERN KINGDOMS FLIGHT PATH DISCOVERY SQL
-- ============================================================
