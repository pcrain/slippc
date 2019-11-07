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
