### 2019-11-14
  * Fixed computation for ledgedashes (accidentally counted most ledge options as ledgedashes before)
  * Added stats for damage done during each kind of interaction for each player

### 2019-11-11
  * Bumped parser to version 0.2.0
  * Added CPU level to parsed replay data
  * Added proper decoding for pseudo Shift-JIS tags

### 2019-11-10
  * Bumped analyzer version to 0.3.0
  * Changed how neutral wins, pokes, and counterattacks were computed to align better with punish data
  * Fixed command grabs not being counted as grabs for interaction purposes
  * Added stats for short hops / full hops
  * Added stats for l-cancel / autocancel / teeter cancel / edge cancel aerials
  * Added stats for shield damage taken, time in shield, lowest shield health
  * Added stats for aerial / special teeter cancels
  * Added stats for total openings, mean openings per kill, mean damage per opening, and mean damage per kill
  * Added stats for invincible ledgedashes and mean GALINT after ledgedashes

### 2019-11-06b
  * Changed analysis attack names to short names by default

### 2019-11-06a
  * Bumped parser version to 0.1.1
  * Bumped analyzer version to 0.2.0
  * Added individual move breakdown to analyzer output
  * Added option for outputting parsed Slippi replay to stdout
  * Reversed output order for player interactions (e.g., offense goes first)
  * Added color, team color, and player type to analysis output
  * Removed "trading" as a separate interaction (now just counts as "poking")
  * Removed airborne qualifier for punishes from poke state
  * Made offstage check for being airborne so opponents hit into the stage don't count as "below stage"
  * Reworked how various interactions and punishes are computed to be more sensible
  * Adjusted neutral win counter s.t. 0-hitstun attacks aren't counted (e.g., Fox lasers)
