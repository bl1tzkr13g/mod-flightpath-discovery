Flight Path Discovery - configuration migration

1. BACK UP your existing module folder and worldserver configuration.
2. Replace the module source with this package and re-run CMake/build.
3. Install the updated mod-flightpath-discovery.conf.dist as mod-flightpath-discovery.conf
   in the active worldserver configuration directory (or merge into existing config).
4. Keep your existing gameobject board template in the world DB.
5. Restart worldserver after editing routes/prices.
6. Old custom MySQL route/price tables can remain; this code does not read them.
7. Do not delete the old tables until your new build has been tested.

Important: This package was adapted from the available v3 module snapshot,
with board lifecycle cleanup brought forward. It has not been compiled against
your exact AzerothCore master. Verify with a test build before deploying.

Price keys must be positive copper amounts <= 2147483647.
A missing price makes that destination unavailable for purchase.
