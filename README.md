# Flight Path Discovery — AzerothCore WotLK 3.3.5a

Flight Path Discovery adds interactive **Discovery Boards** beside eligible flight masters. A player can pay to permanently learn a **configured neighboring flight path** without visiting that destination first. This does **not** transport the player: normal flight masters and taxi travel still handle transportation.

**Release status:** source package, not a compiled or tested Windows binary. Build and verify against your AzerothCore revision before deploying to a live server. The earlier source package referenced core commit `9c565309b`; this package has not been independently verified against that commit.

## Features

- Discovery Boards summoned near flight masters; placement uses the NPC's orientation and a configurable offset.
- Manually curated **directional** routes for **Kalimdor** and **Eastern Kingdoms**.
- Routes and destination-specific prices editable in `mod-flightpath-discovery.conf` — **no MySQL route or price queries** in this version.
- Location, faction and destination annotations in the configuration.
- Alliance/Horde destination filtering using AzerothCore's native taxi masks; map checks and already-known destination checks.
- Purchase confirmation, copper deduction and permanent unlock using the character's existing taxi mask.
- Board association tracking and cleanup when the flight master or board leaves the world.
- No core source edits or character database schema changes required.

## Requirements

- AzerothCore WotLK 3.3.5a source and a compatible 3.3.5a client.
- Ability to configure and compile an AzerothCore C++ module.
- AzerothCore world database with the Discovery Board gameobject template installed.

## Installation — new installation

1. Place the `mod-flightpath-discovery` directory in your AzerothCore `modules` directory.
2. Install `conf/mod-flightpath-discovery.conf.dist` as **`mod-flightpath-discovery.conf`** in the active worldserver configuration location used by your AzerothCore installation. Confirm it is loaded; editing the `.dist` file alone may not change the active settings.
3. Ensure `data/sql/db-world/2026_10_11_00_flightpath_board.sql` has been applied to `acore_world` **once** (either by your updater or manually). Do not assume your particular updater automatically discovers module SQL. The board uses gameobject entry `900100`, type `10`, display ID `2491`.
4. Configure/generate with CMake, build AzerothCore `worldserver`, and deploy the newly built executable and active configuration.
5. Restart `worldserver` and verify the boards and purchase workflow in game.

### Upgrading an existing SQL-driven installation

1. **Back up** the module folder, the active configuration, your working `worldserver` binary, and relevant databases before making changes.
2. Preserve the existing Discovery Board gameobject template. If already installed, it does not need to be recreated.
3. Replace the module C++ source and install/merge the new `.conf` from this package. The included configuration is based on the **published GitHub Kalimdor and Eastern Kingdoms SQL**; it is not an export of any separately modified live database.
4. Reconfigure CMake, rebuild, deploy the new executable, and restart `worldserver`.
5. **Leave** the old `mod_flightpath_discovery_routes` and `mod_flightpath_discovery_prices` tables in place during testing. This version does not read them; keeping them makes rollback easier.
6. Verify Alliance and Horde routes, purchases, persistence after relog, and board behavior after NPC unload/reload.

**Important:** The old SQL-driven version and this configuration-driven version are not interchangeable without rebuilding the C++ module. Editing the new configuration does not affect an old binary.

## Configuration

The active configuration uses AzerothCore's `[worldserver]` section.

| Setting | Purpose |
| --- | --- |
| `FlightPathDiscovery.Enable` | Enable or disable the module (`1` / `0`). |
| `FlightPathDiscovery.TestCreatureEntry` | Restrict boards to one flight-master creature entry for testing; `0` allows all eligible masters. |
| `FlightPathDiscovery.BoardEntry` | Discovery Board gameobject entry, default `900100`; must match the installed template. |
| `FlightPathDiscovery.BoardDistance` | Board placement offset in game units, default `2.5`. |
| `FlightPathDiscovery.EnableNorthrend` | Permit map 571 for board/map checks when enabled. **No Northrend routes or prices are provided in this package.** |
| `FlightPathDiscovery.UnknownZoneLevel` | Legacy setting retained in the config for compatibility; **not used** by the current configuration-priced C++ implementation. |

### Kalimdor routes and prices

```ini
# Bloodvenom Post - Felwood - Horde (taxi node 48)
# Destinations: 53 Everlook (Horde), 69 Moonglade (Horde), 166 Emerald Sanctuary
FlightPathDiscovery.Kalimdor.Routes.48 = "53,69,166"

# Price to discover Bloodvenom Post (30,000 copper = 3 gold)
FlightPathDiscovery.Kalimdor.Price.48 = 30000
```

### Eastern Kingdoms routes and prices

```ini
# Stormwind - Alliance (taxi node 2)
FlightPathDiscovery.EasternKingdoms.Routes.2 = "4,5,6,12"

# Price to discover Stormwind (5,000 copper = 50 silver)
FlightPathDiscovery.EasternKingdoms.Price.2 = 5000
```

The supplied `.conf.dist` contains the full curated route network and destination prices from the published SQL, separated by continent. The descriptions are comments only; **numeric taxi node IDs control behavior**. Some neutral/special-node descriptions are marked for verification against the server's TaxiNodes DBC.

**Rules:**

- `Routes.<source>` is a comma-separated list of destination taxi node IDs. **Connections are one-way**; add the reverse connection separately if desired.
- `Price.<destination>` is the cost **to unlock that destination**, regardless of the source board. `100` copper = `1` silver; `10000` copper = `1` gold.
- A destination with no valid positive configured price **cannot be purchased**. There is **no automatic zone-level price fallback** in this build.
- The loader accepts taxi node IDs `1`–`256` that exist in the DBC and match the configured continent; invalid IDs, self-links, and cross-map destinations are ignored. Duplicate destinations are deduplicated.
- Configured prices must be positive and at most `2147483647` copper to be accepted safely by the signed money deduction.
- Routes and prices are read **once during module startup**. **Restart worldserver after changing them.** This build does not dynamically reload the network.
- Existing faction-mask filtering still applies. Config comments alone do not enforce faction eligibility.

### Known network details

- The published Kalimdor SQL intentionally excludes the Crossroads ↔ Brackenwall shortcut and known cross-faction connections.
- Alliance and Horde Everlook/Moonglade taxi nodes are distinct and remain distinct.
- Kalimdor taxi node `73` (Horde Cenarion Hold) has a configured destination price but **no outgoing route** in the published SQL. This package preserves that data as-is pending a separate route review.
- Eastern Kingdoms and Kalimdor are the only supplied curated networks. Outland is not supported; Northrend is not populated even if its map option is enabled.

## Gameplay

1. Approach an eligible flight master and interact with the nearby Discovery Board.
2. The board lists configured neighboring destinations that your character can learn, with their configured prices.
3. Select a destination, review the confirmation, and confirm purchase.
4. If eligible and sufficiently funded, the character pays and learns the destination taxi node.
5. Use a flight master normally to travel. Learned taxi nodes use AzerothCore's existing character persistence; verify after relog.

The module does not teleport or automatically fly the player. It also does not verify that every curated unlock corresponds to a usable flight path in the core taxi graph; gameplay testing is recommended.

## Board lifecycle

Boards are summoned beside eligible flight masters when they enter the world, with **zero timed expiration** in this build. A board is removed when its associated flight master leaves the world. Removal callbacks clear board/master association records. Board placement is automatic and can overlap terrain or structures in unusual locations; inspect problematic spawns in game. An independently removed board is **not guaranteed to respawn immediately** while its flight master remains loaded; it can reappear when that master is loaded again.

## Testing checklist

1. Start with `FlightPathDiscovery.TestCreatureEntry` set to a single known flight-master creature entry.
2. Verify a board appears, can be opened, and lists only configured, unknown, faction-eligible neighbors.
3. Check prices and confirmation; verify insufficient funds do not grant unlocks.
4. Buy one route and verify the exact copper deduction and newly learned taxi node.
5. Relog and verify the unlock persists; restart worldserver and retest.
6. Repeat with an Alliance and a Horde character, including faction-specific nodes.
7. Test flight-master unload/reload and board cleanup. Confirm no duplicate boards.
8. Set `TestCreatureEntry = 0`, restart, and inspect several locations on both continents.

## Troubleshooting

- **No boards:** confirm the rebuilt worldserver is deployed, the active config is loaded, module enabled, flight master eligible, and board gameobject entry exists in `acore_world.gameobject_template`.
- **Board has no destinations:** verify `Routes.<source>` and `Price.<destination>` entries, faction eligibility, same-continent node IDs, and whether destinations are already known.
- **Config edits have no effect:** restart `worldserver` and verify you edited the **active `.conf`**, not only the `.conf.dist` template.
- **Old SQL changes have no effect:** expected; this version does not query the custom route/price tables.
- **Board placement issues:** adjust `BoardDistance` or review the NPC's orientation/location.
- **Compile errors:** the package has not been compiled against your exact AzerothCore checkout; capture the first compiler error and your core commit before making API compatibility adjustments.

## Files in this package

- `src/FlightPathDiscovery.cpp` — board scripts, route/price config loader, purchase handling and lifecycle tracking.
- `conf/mod-flightpath-discovery.conf.dist` — full route network, destination prices and settings.
- `data/sql/db-world/2026_10_11_00_flightpath_board.sql` — board gameobject template; no route/price SQL tables required.
- `CMakeLists.txt` — module build integration.
- `README.md` — consolidated current documentation.

## License and author

Original project: [bl1tzkr13g/mod-flightpath-discovery](https://github.com/bl1tzkr13g/mod-flightpath-discovery). License: **AGPL-3.0** (per upstream repository). Preserve the upstream license file and notices when publishing a distribution.
