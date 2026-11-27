[levoit][enhancement] Filter life
is counted in speed-weighted seconds on every loop pass instead of whole cubic
metres once a minute, and saved once per hour of wear and at shutdown instead
of every minute; an existing counter is converted once at the first boot, so no
filter jumps back to 100 %
