[levoit-humidifiers][fix] Fan entity
the speed count and the no-Auto rule come from the core's per-model facts
(`speed_count()`, `has_auto()`) instead of a second table in the entity, which
had 4 speeds for the Superior 6000S where the core and the README have 9
