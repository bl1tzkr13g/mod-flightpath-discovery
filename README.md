# Flight Path Discovery

An [AzerothCore](https://www.azerothcore.org/) (WotLK 3.3.5a) module that introduces **Flight Path Discovery Boards** near flight masters, allowing players to permanently unlock neighboring flight paths by purchasing them with gold instead of traveling to each location first.

Features **faction-specific flight paths**, configurable destination prices based on zone level, and separate discovery networks for Kalimdor and Eastern Kingdoms. Preserves traditional flight travel, including boats and zeppelins between continents, while reducing the grind of discovering flight paths.

**No character database modifications required.**

## Features

- **Flight Path Discovery Boards** — Interactable boards located near flight masters.
- **Permanent Unlocks** — Purchase a destination once to permanently unlock its flight path for that character.
- **Faction-Aware Discovery** — Alliance and Horde characters only see eligible flight destinations.
- **Neighboring Flight Paths** — Discover nearby connected flight points rather than unlocking an entire continent at once.
- **Configurable Prices** — Customize individual destination prices through the world database.
- **Separate Continental Networks** — Kalimdor and Eastern Kingdoms maintain independent discovery networks.
- **Traditional Travel Preserved** — Boats and zeppelins remain necessary for intercontinental travel.
- **Native Taxi System** — Uses AzerothCore's existing flight-path unlock system.
- **No Character Database Changes** — Works without modifying character database tables.

## Requirements

- [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) WotLK 3.3.5a
- World of Warcraft 3.3.5a (12340) client
- Ability to compile AzerothCore with custom modules
- MySQL-compatible AzerothCore world database

## Installation

1. Navigate to your AzerothCore `modules` directory.
2. Clone the repository:

   ```bash
   git clone https://github.com/bl1tzkr13g/mod-flightpath-discovery.git
   ```

3. Reconfigure AzerothCore using CMake.
4. Rebuild your AzerothCore server.
5. Apply the module's SQL migrations in `data/sql/db-world` to the world database if they are not applied automatically.
6. Start the worldserver.

Discovery Boards should appear near supported flight masters.

## Configuration

The module uses two world database tables.

### Flight Path Routes

`mod_flightpath_discovery_routes`

Defines which neighboring flight points can be discovered from each location.

| Column | Description |
|---|---|
| `source_node` | Flight point where the purchase is offered |
| `destination_node` | Flight point that can be unlocked |

Routes are directional. To allow discovery in both directions, configure both connections.

### Flight Path Prices

`mod_flightpath_discovery_prices`

Defines the purchase cost for individual flight destinations.

| Column | Description |
|---|---|
| `taxi_node_id` | Destination flight point ID |
| `price_copper` | Unlock price in copper |

### Default Pricing

| Zone Level | Unlock Price |
|---|---|
| Capital Cities | 50 silver |
| 1–10 | 10 silver |
| 11–20 | 25 silver |
| 21–30 | 75 silver |
| 31–40 | 1 gold 50 silver |
| 41–50 | 3 gold |
| 51–60 | 5 gold |

Prices are assigned to destinations and can be modified independently of the zone-level guidelines.

## Supported Continents

### Eastern Kingdoms

Includes curated neighboring flight-path connections for Alliance and Horde destinations.

### Kalimdor

Includes curated neighboring flight-path connections for Alliance and Horde destinations, including faction-specific flight points at shared locations.

### Future Support

Outland and Northrend are not currently included in the curated discovery networks.

## How It Works

1. Visit a flight master and locate the nearby Flight Path Discovery Board.
2. Interact with the board to view available neighboring destinations.
3. Select a destination and pay the displayed price.
4. The flight point is permanently unlocked for your character.
5. Speak with the flight master to use the newly discovered flight path.

Destinations already known to the character are excluded from purchase options. Faction-ineligible destinations are filtered automatically.

Purchasing a flight point unlocks it; it does not automatically transport the player.

## Author

**[bl1tzkr13g](https://github.com/bl1tzkr13g)**

## License

AGPL-3.0
