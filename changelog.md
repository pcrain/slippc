### 2021-02-13
  * Officially added compression to slippc
  * Bumped parser version to 0.6.0
  * Updated parser to incorporate data from replays up to version 3.8.0
  * Parser now includes item data
  * Parser now includes game winner and end stock information
  * Bumped analyzer version to 0.6.0
  * Analyzer now includes end stock information
  * Added max GALINT to analyzer stats
  * Added action state changes to analyzer stats
  * Added counts for buttons pushed and stick movements to stats
  * Added actions per minute and action states per minute to stats
  * Fixed parser bug reading 4 bytes from physical buttons instead of 2 bytes

### 2020-10-07
  * Bumped analyzer version to 0.5.0
  * Bumped parser version to 0.5.0
  * Fixed GALINT measurements
  * Removed "Unknown event code warning" from stderr (now visible with debug level 1)
  * Added "tag_code" metadata to parser and analyzer outputs for new Slippi

### 2020-06-04
  * Bumped parser to version 0.4.0
  * Bumped analyzer to version 0.4.0
  * Rebuilt Windows binary

### 2020-05-20
  * Renamed stage enums to match Melee's internal stage name counterparts
  * No longer output analysis JSON if analysis isn't successful

### 2020-05-19
  * Added makefile for Windows
  * More tweaks to pressuring dynamic

### 2019-11-20
  * Changed pressuring definition to stay in pressuring state if opponent is sent into hitstun
  * Changed pressuring definition to transfer to punishing if opponent is hit

### 2019-11-19
  * Changed galint definition to use actually actionable frames (rather than Landing animation frames)

### 2019-11-18
  * Changed techchasing definition to lead into sharking if opponent is airborne but not in hitstun
  * Changed techchasing definition to lead into punishing if opponent gets hit while airborne and in hitstun
  * Changed sharking definition to lead into punishing if opponent gets hit again within shark window

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
