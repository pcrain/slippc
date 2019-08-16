**slippc** - A Slippi replay (.slp) parser, JSON converter, and basic analysis program written in C++

## Requirements
  * _make_ and _g++_, for building _slippc_

## Usage
    Usage: slippc -i <infile> [-j <jsonfile>] [-a <analysisfile>] [-f] [-d <debuglevel>] [-h]:
      -i     Parse and analyze <infile> (not very useful on its own)
      -j     Output <infile> in .json format to <jsonfile>
      -a     Output an analysis of <infile> in .json format to <analysisfile> (use "-" for stdout)
      -f     When used with -j <jsonfile>, write full frame info (c.f. frame deltas)
      -d     Run in debug mode (show debug output)
      -h     Show this help message

## Basic Overview

_slippc_ aims to be a fast Slippi replay (.slp file) parser, with three primary functions. First, for those wishing to write their own analysis code in C++, it aims to be a library for storing and manipulating .slp replay files. Second, to aid with external analysis, it aims to be a converter between the .slp file's internal UBJSON format (which often requires external libraries to parse) into a more human-friendly and readily-parseable JSON format (which most modern languages support natively). Finally, for those wishing to analyze hundreds or thousands of .slp replay files relatively quickly, it aims to be a basic replay analysis tool in its own right.

**NOTE**: currently, slippc does not function as a standalone library, so manual confirguration is required to use the *SlippiReplay* struct

For all functions, the *Parser* class does the job of converting the raw .slp file from UBJSON into an internal data structure in the form of the *SlippiReplay* struct. Per Fizzi's specifications, the parser is backwards compatibile, handling and skipping unknown events gracefully. For the most part, a *SlippiReplay* contains the exact same data as is stored in the .slp file: the biggest changes from the raw .slp file include 1) conversions from big-endian types (as used by the Gamecube and stored in UBJSON) to little-endian types (as used in the x86 / x86_64 architectures most modern computers run on), 2) type changes for some other variables (e.g., the Slippi version number is stored as a string rather than as 4 separate bytes), and 3) reorganization of events.

The reorganization of events is the biggest difference between the .slp file structure and the internal *SlippiReplay* struct. In brief, data from the "Game Start" and "Game End" events -- as well as data from the "metadata" section of the replay file -- are combined and stored at the top level of the *SlippiReplay* struct. Similarly, the "Pre Frame Update" and "Post Frame Update" events at each frame are combined and stored as an array of *SlippiFrame*s for each player, which in turn are represented with the internal structure *SlippiPlayer*. Data that occurs in both the pre-frame and post-frame events have "\_pre" and "\_post" respectively appended to their field names within the *SlippiFrame* struct. Of additional note, the top-level *SlippiReplay* contains 8 slots for *SlippiPlayer*s, with slots 0-3 corresponding to ports 1-4 and slots 4-7 corresponding to the 2nd Ice Climber for ports 1-4 respectively. Unused slots (either empty ports for slots 0-3 or non-Icies for slots 4-7) contain undefined data.

## JSON Output

Passing the -j option to _slippc_ will output the .slp file specified with -i as a .json file, which may be opened in any text editor and inspected directly, or further parsed and analyzed using any JSON parser. Most data is presented in integer or float format, as stored in the .slp file. Major additions include the "game\_start\_raw" field, which is a base64 encoding of Melee's internal structure for initializing a new game, and the "parser\_version" field, which describes the semantic versioning version number of the _slippc_ parser used to generate the file. By default, to keep file sizes down, _slippc_ only records deltas between frames (i.e., fields that change) for each player; by passing the -f option, _slippc_ will output a .json with all data at each frame intact, including unchanged fields. The top-level "frame_count" field specifies the total number of frames in each player's "frames" field, with "first\_frame" designating Melee's internal frame counter for the first frame (always -123), and "last\_frame" designating the final frame of the game.

## Analysis

Passing the -a option to _slippc_ will perform a basic analysis of the .slp file specified with -i as a .json file (or directly to the console if "-" is passed instead of a filename). Most of the fields are fairly self-explanatory. The "punishes" field for each player contains a list of all combos / techchases / strings performed by the player throughout the duration of the match, along with some very basic statistics about each. The "interactions" field specifies the number of frames each player spent in each interaction state, as described below:

**NOTE**: The analyzer assumes each analyzed match is played according to singles tournament settings (2 players, 8 minute timer, 4 stocks, tournament legal stages, no glitch characters). Though it may still work in some cases, analyzer behavior is currently undefined for matches played with non-tournament settings or in teams mode.

**NOTE**: The output of some fields (such as neutral wins and combo / string counts) may be different than what is shown, e.g., in the Slippi Replay Browser. This discrepancy is due to differences in the way _slippc_ infers game dynamics and computes certain statistics compared to Fizzi's node.js analyzer.

### Neutral Interactions
  The following are considered neutral states; frame counts should be identical for both players:

  * NEUTRAL: generic neutral state; should always be 0
  * POSITIONING: players are far apart, and neither player has recently been in hitstun
  * FOOTSIES: players are close together, and neither player has recently been in hitstun
  * POKING: one or both players have recently entered hitstun from a single attack
  * TRADING: both players are currently in hitstun

### Offensive <-> Defensive interactions
  The following offensive states are listed with their corresponding defensive states; frame counts should be symmetric for both players:

  * OFFENSIVE <-> DEFENSIVE: generic offensive / defensive states; should always be 0
  * PRESSURING <-> PRESSURED: the pressuring player has landed a grab in neutral or on defense, or the pressured player is in shield stun
  * PUNISHING <-> PUNISHED: the player being punished has been in hitstun at least twice in a short period without landing a hit
  * SHARKING <-> GROUNDING: the grounding player has not touched the ground since last entering hitstun
  * TECHCHASING <-> ESCAPING: the techchasing player landed a grab while on offense, or the escaping player is in a tech or missed-tech state
  * EDGEGUARDING <-> RECOVERING: the recovering player has recently been in hitstun while off stage and has not since landed or grabbed ledge

## Future Plans
  * Compile to an actual library so the parser can be used by other programs
  * Make analyzer more robust (meta-analysis for multiple replays, full support for Ice Climbers, etc.)

## Credits
  * Fizzi, for creating Slippi in the first place and for writing the node.js code as a reference for how to parse and analyze the .slp files
  * Andrew Epstein, for helping debug and fix a number of crashes

## References
  * [Project Slippi's main GitHub page](https://github.com/project-slippi/project-slippi)
  * [Fizzi's .slp replay file specifications](https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec)
  * [SSBM 1.02 data sheet](https://docs.google.com/spreadsheets/d/1JX2w-r2fuvWuNgGb6D3Cs4wHQKLFegZe2jhbBuIhCG8/edit) (contains state info, internal game IDs, and other Melee stuff)
