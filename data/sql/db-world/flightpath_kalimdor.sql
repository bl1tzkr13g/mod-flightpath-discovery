
-- ============================================================
-- Flight Path Discovery - Kalimdor
-- AzerothCore WotLK 3.3.5a
-- ============================================================
-- Curated neighboring flight-path discovery network.
--
-- Faction-specific nodes:
-- 48 = Bloodvenom Post (Horde)
-- 49 = Moonglade (Alliance)
-- 52 = Everlook (Alliance)
-- 53 = Everlook (Horde)
-- 65 = Talonbranch Glade (Alliance)
-- 69 = Moonglade (Horde)
--
-- Fixes:
-- 1. Remove Crossroads <-> Brackenwall shortcut.
-- 2. Preserve Horde Everlook <-> Bloodvenom Post.
-- 3. Preserve Horde Everlook <-> Horde Moonglade.
-- 4. Preserve Alliance Everlook <-> Alliance Moonglade.
-- 5. Preserve Alliance Everlook <-> Talonbranch Glade.
-- 6. Remove 24 flagged cross-faction directional routes.
-- 7. Preserve all destination prices.
--
-- Northrend and Outland are not included.
-- ============================================================


-- ------------------------------------------------------------
-- KALIMDOR ROUTES
-- ------------------------------------------------------------

INSERT INTO mod_flightpath_discovery_routes
(source_node, destination_node)
VALUES
(22, 29),
(22, 77),

(23, 25),
(23, 44),
(23, 80),

(25, 23),
(25, 29),
(25, 61),
(25, 77),
(25, 80),

(26, 27),
(26, 28),
(26, 33),
(26, 64),
(26, 65),
(26, 166),

(27, 26),

(28, 26),
(28, 33),
(28, 167),

(29, 22),
(29, 25),
(29, 38),

(30, 40),
(30, 77),
(30, 179),

(31, 37),
(31, 41),

(32, 179),

(33, 26),
(33, 28),
(33, 37),

(37, 31),
(37, 33),
(37, 41),

(38, 29),
(38, 42),

(39, 79),

(40, 30),
(40, 79),

(41, 31),
(41, 37),

(42, 38),

(44, 23),
(44, 61),

-- Bloodvenom Post (Horde)
(48, 53),
(48, 69),
(48, 166),

-- Moonglade (Alliance)
(49, 52),
(49, 65),

-- Everlook (Alliance)
(52, 49),
(52, 65),

-- Everlook (Horde)
(53, 48),
(53, 69),

-- Brackenwall Village (Horde)
(55, 80),
(55, 179),

-- Zoram'gar Outpost (Horde)
(58, 61),

-- Splintertree Post (Horde)
(61, 25),
(61, 44),
(61, 58),

-- Talrendis Point (Alliance)
(64, 26),
(64, 167),

-- Talonbranch Glade (Alliance)
(65, 26),
(65, 49),
(65, 52),
(65, 166),

-- Moonglade (Horde)
(69, 48),
(69, 53),

-- Cenarion Hold (Alliance)
(72, 79),

-- Cenarion Hold (Horde)
-- Node 73 retained in destination pricing.
-- No direct route to Alliance node 72.

(77, 22),
(77, 25),
(77, 30),

(79, 39),
(79, 40),
(79, 72),

(80, 23),
(80, 25),
(80, 55),

-- Emerald Sanctuary
(166, 26),
(166, 48),
(166, 65),

(167, 28),
(167, 64),

-- Mudsprocket
(179, 30),
(179, 32),
(179, 55)

ON DUPLICATE KEY UPDATE
destination_node = VALUES(destination_node);


-- ------------------------------------------------------------
-- REMOVE OBSOLETE OR INCORRECT ROUTES
-- ------------------------------------------------------------
-- Corrects databases where older SQL was already imported.
-- Removes the 24 flagged cross-faction directional routes,
-- plus previously identified obsolete connections.
-- ------------------------------------------------------------

DELETE FROM mod_flightpath_discovery_routes
WHERE (source_node, destination_node) IN
(
-- Crossroads <-> Brackenwall shortcut
(25, 55),
(55, 25),

-- Astranaar <-> Zoram'gar
(28, 58),
(58, 28),

-- Astranaar <-> Splintertree
(28, 61),
(61, 28),

-- Talrendis Point <-> Splintertree
(61, 64),
(64, 61),

-- Freewind <-> Thalanaar
(30, 31),
(31, 30),

-- Freewind <-> Alliance Gadgetzan
(30, 39),
(39, 30),

-- Thalanaar <-> Camp Mojache
(31, 42),
(42, 31),

-- Nijel's Point <-> Shadowprey
(37, 38),
(38, 37),

-- Feathermoon <-> Camp Mojache
(41, 42),
(42, 41),

-- Camp Mojache <-> Alliance Gadgetzan
(39, 42),
(42, 39),

-- Gadgetzan Alliance <-> Horde
(39, 40),
(40, 39),

-- Theramore <-> Brackenwall
(32, 55),
(55, 32),

-- Cenarion Hold Alliance <-> Horde
(72, 73),
(73, 72),

-- Previously removed Everlook faction crossover
(52, 53),
(53, 52),

-- Bloodvenom <-> Talonbranch
(48, 65),
(65, 48),

-- Horde Everlook <-> Alliance Moonglade
(53, 49),
(49, 53),

-- Bloodvenom <-> Alliance Moonglade
(48, 49),
(49, 48)
);


-- ------------------------------------------------------------
-- KALIMDOR DESTINATION PRICES
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
(22, 5000),
(23, 5000),
(25, 1000),
(26, 1000),
(27, 5000),
(28, 2500),
(29, 2500),
(30, 7500),
(31, 15000),
(32, 15000),
(33, 2500),
(37, 7500),
(38, 7500),
(39, 15000),
(40, 15000),
(41, 15000),
(42, 15000),
(44, 30000),
(48, 30000),
(49, 1000),
(52, 50000),
(53, 50000),
(55, 15000),
(58, 2500),
(61, 2500),
(64, 30000),
(65, 30000),
(69, 1000),
(72, 50000),
(73, 50000),
(77, 1000),
(79, 30000),
(80, 1000),
(166, 30000),
(167, 2500),
(179, 15000)

ON DUPLICATE KEY UPDATE
price_copper = VALUES(price_copper);

-- ============================================================
-- END OF KALIMDOR FLIGHT PATH DISCOVERY SQL
-- ============================================================
