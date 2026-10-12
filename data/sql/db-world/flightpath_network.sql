-- V2: Explicit discovery adjacency and fixed per-node pricing.
-- The tables intentionally start EMPTY. Populate only verified taxi-node IDs.
CREATE TABLE IF NOT EXISTS mod_flightpath_discovery_routes (
    source_node INT UNSIGNED NOT NULL,
    destination_node INT UNSIGNED NOT NULL,
    PRIMARY KEY (source_node, destination_node)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS mod_flightpath_discovery_prices (
    taxi_node_id INT UNSIGNED NOT NULL PRIMARY KEY,
    price_copper INT UNSIGNED NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- After verifying IDs, e.g.:
-- INSERT INTO mod_flightpath_discovery_routes (source_node, destination_node)
-- VALUES (<source_id>, <destination_id>);
-- INSERT INTO mod_flightpath_discovery_prices (taxi_node_id, price_copper)
-- VALUES (<capital_taxi_node_id>, 5000);
