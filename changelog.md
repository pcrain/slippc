### 2022-02-19
  * Added support for parsing, analyzing, and compressing replays up to 3.12.0
  * Added support for parsing, analyzing, and compressing replays down to 0.x.x
  * Added support for parsing and analyzing (not compressing) partially corrupt replays
  * Added support for entire-directory inputs for parsing, analyzing, and compression (not decompression yet)
  * Added constraint on output filenames during compression ensuring all compressed replays end with .zlp and all decompressed replays end with .slp
  * Added a testing program, test suites, and test replay files to ensure backwards compatibility, forwards compatibility, and data integrity
  * Integrated cbartsch's changes into parser and analyzer
  * Improved the way the compression algorithm handles items for better compression (most noticeable for Yoshi's Story games)
  * Improved compatibility for compressing replays with lots of items or rollback'd frames that previously could not be compressed
  * Improved resource management during compression to reduce memory footprint and increase compression speed
  * Improved debug, warning, and error outputs
  * Fixed several memory leaks so everything runs cleanly through valgrind
  * Errors compressing files in directory mode are now written to a log file
  * Bumped parser, analyzer, and compression versions to 0.8.0

### 2021-03-21
  * Added a bunch of missing game start block info to parser output
  * Added a bunch of missing player start block info to parser output
  * Added documentation to all SlippiReplay struct attributes in replay.h
  * Added original input file name to output of parser and analyzer
  * Merged cbartsch's fix for item counting
  * Renamed and reorganized a few fields in output for parser and analyzer
  * Bumped parser and analyzer versions to 0.7.0

### 2021-03-13
  * Added new outputs to parser:
    * timer start time (in minutes)
  * Added new stats to analyzer:
    * move accuracy
    * average act out of hitstun
    * average act out of shieldstun
    * average act out of wait
    * neutral wins per minute
    * mean death percent
  * Fixed some stats computations:
    * Adjusted mean kill / death percent to exclude damage taken on last stock
    * Edgeguard checks now check if y < -10 (rather than 0) to account for ECB shenanigans
    * Total moves landed no longer count bubble damage
    * Getting shield poked now counts as getting pressured
    * Probably some more tweaks I'm forgetting
  * Enable -O3 optimizations when compiling by default (why were they off?)
  * Merged b3nd3r-ssbm's changes for Windows-compatible endian-swap

### 2021-02-16
  * Fixed massive heap allocation for SlippiItemFrame wasting a ton of RAM
  * Bumper parser and analyzer versions to 0.6.1

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
  * Added ability to parse and analyze .zlp compressed replays

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
