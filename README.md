# **slippc** - A Slippi replay (.slp) parser, compressor, JSON converter, and basic analysis program written in C++

Supports replays between versions 0.1.0 and 3.12.0. Last updated 2022-02-19. [View Change Log](./changelog.md)

## Requirements
  * _make_ and _g++_, for building _slippc_
  * [optional] liblzma (a static v5.2.5 library is bundled, run `make static` to build)

## Usage
```
  Usage: slippc -i <infile> [-x | -X <zlpfle>] [-j <jsonfile>] [-a <analysisfile>] [-f] [-d <debuglevel>] [-h]:
    -i        Set input file (can be .slp, .zlp, or a whole directory)
    -j        Output <infile> in .json format to <jsonfile> (use "-" for stdout)
    -a        Output an analysis of <infile> in .json format to <analysisfile> (use "-" for stdout)
    -f        When used with -j <jsonfile>, write full frame info (instead of just frame deltas)
    -x        Compress or decompress a replay
    -X        Set output file name for compression
    -d        Run at debug level <debuglevel> (show debug output)
    -h        Show this help message
```

## Basic Overview

_slippc_ aims to be a fast Slippi replay (.slp file) parser, with four primary functions.
First, for those wishing to write their own analysis code in C++, it aims to be a library for storing and manipulating .slp replay files.
Second, to aid with external analysis, it aims to be a converter between the .slp file's internal UBJSON format (which often requires external libraries to parse) into a more human-friendly and readily-parseable JSON format (which most modern languages support natively).
Third, for those wishing to archive their replays, it aims to take advantage of replay file structure to efficiently compress .slp files faster and smaller than any general purpose compression tools.
Finally, for those wishing to analyze hundreds or thousands of .slp replay files relatively quickly, it aims to be a basic replay analysis tool in its own right.

**NOTE**: currently, slippc does not function as a standalone library, so manual confirguration is required to use the *SlippiReplay* struct

For all functions, the *Parser* class does the job of converting the raw .slp file from UBJSON into an internal data structure in the form of the *SlippiReplay* struct. Per Fizzi's specifications, the parser is backwards compatibile, handling and skipping unknown events gracefully. For the most part, a *SlippiReplay* contains the exact same data as is stored in the .slp file: the biggest changes from the raw .slp file include 1) conversions from big-endian types (as used by the Gamecube and stored in UBJSON) to little-endian types (as used in the x86 / x86_64 architectures most modern computers run on), 2) type changes for some other variables (e.g., the Slippi version number is stored as a string rather than as 4 separate bytes), and 3) reorganization of events.

The reorganization of events is the biggest difference between the .slp file structure and the internal *SlippiReplay* struct.
In brief, data from the "Game Start" and "Game End" events -- as well as data from the "metadata" section of the replay file -- are combined and stored at the top level of the *SlippiReplay* struct.
Similarly, the "Pre Frame Update" and "Post Frame Update" events at each frame are combined and stored as an array of *SlippiFrame*s for each player, which in turn are represented with the internal structure *SlippiPlayer*.
Data that occurs in both the pre-frame and post-frame events have "\_pre" and "\_post" respectively appended to their field names within the *SlippiFrame* struct.
Item data is likewise tracked separately by item spawn ID with the internal *SlippiItem* struct, each of which contains information about items at individual frames stored in the *SlippiItemFrame* struct.
The top-level *SlippiReplay* contains 8 slots for *SlippiPlayer*s, with slots 0-3 corresponding to ports 1-4 and slots 4-7 corresponding to the 2nd Ice Climber for ports 1-4 respectively. Unused slots (either empty ports for slots 0-3 or non-Icies for slots 4-7) contain undefined data.

## Compression

Passing the -x option to _slippc_ will compress an input .slp file specified with -i to a .zlp file. _slippc_ uses a combination of delta coding, predictive coding, event shuffling, and column shuffling to encode raw .slp files, then compresses them using LZMA compression. Passing the -x option to _slippc_ and a compressed .zlp file with -i will decompress the file to a normal .slp file. To explicitly specify an output filename, you can pass -X [outfilename].

_slippc_ validates all compressed files by decompressing them in memory and verifying the decoded file matches the original file. If for whatever reason this decode fails, no .zlp file will be created. As an additional failsafe, _slippc_ will never delete any original files, and will refuse to overwrite existing files if there is a filename conflict.

Compression should work for all replays between version 0.1.0 and 3.12.0, thought it cannot and will not compress corrupt replay files (if you have a non-corrupt replay that won't compress, please create an issue with the replay attached). Typical compression rates range from 93-97% for most normal replays. Compressed .zlp files may be loaded through _slippc_ for parsed JSON and analysis JSON output.

## JSON Output

Passing the -j option to _slippc_ will output the .slp file specified with -i as a .json file, which may be opened in any text editor and inspected directly, or further parsed and analyzed using any JSON parser. Most data is presented in integer or float format, as stored in the .slp file. Major additions include the "game\_start\_raw" field, which is a base64 encoding of Melee's internal structure for initializing a new game, and the "parser\_version" field, which describes the semantic versioning version number of the _slippc_ parser used to generate the file. By default, to keep file sizes down, _slippc_ only records deltas between frames (i.e., fields that change) for each player; by passing the -f option, _slippc_ will output a .json with all data at each frame intact, including unchanged fields. The top-level "frame_count" field specifies the total number of frames in each player's "frames" field, with "first\_frame" designating Melee's internal frame counter for the first frame (should always be -123), and "last\_frame" designating the final frame of the game.

## Analysis

Passing the -a option to _slippc_ will perform a basic analysis of the .slp file specified with -i as a .json file (or directly to the console if "-" is passed instead of a filename). Most of the fields are fairly self-explanatory. The "punishes" field for each player contains a list of all combos / techchases / strings performed by the player throughout the duration of the match, along with some very basic statistics about each. The "interactions" field specifies the number of frames each player spent in each interaction state, as described below:

**NOTE**: The analyzer assumes each analyzed match is played according to singles tournament settings (2 players, 8 minute timer, 4 stocks, tournament legal stages, no glitch characters). Though it may still work in some cases, analyzer behavior is currently undefined for matches played with non-tournament settings or in teams mode.

**NOTE**: The output of some fields (such as APM, neutral wins, and combo / string counts) may be different than what is shown in the Slippi Replay Browser. This discrepancy is due to differences in the way _slippc_ infers game dynamics and computes certain statistics compared to Fizzi's node.js analyzer.

## Directory Mode

By passing a directory as the input file with the -i flag, _slippc_ will operate in directory mode, where it will (non-recursively) scan an entire directory for valid .slp files. In directory mode, at least one of the -j, -a, or -X options must be specified. Each of these options must also be a valid writeable directory path (e.g., not an existing file and not a read-only directory). Directories will be created if they do not exist. Assuming the base name of each input file is _input.slp_, files will be named in each output directory according to the following naming schemes:

  * -j : _input_.json
  * -a : _input_-analysis.json
  * -X : _input_.zlp

In directory mode, any errors during compression are written to an _\_errors.txt_ file in the directory specified with -X. Due to logistical overhead for parsing directories containing both raw and slippc-compressed files, directory mode currently does not have functionality to decompress all compressed files in a directory.

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
  * Make analyzer more robust (meta-analysis for multiple replays, full support for Ice Climbers, analyzing games with more than 2 players, etc.)

## Credits
  * [Fizzi](https://github.com/JLaferri), for creating Slippi in the first place and for writing the node.js code as a reference for how to parse and analyze the .slp files
  * [Nikki](https://github.com/NikhilNarayana), for substantial work on Slippi and help answering several questions related to the Slippi code base
  * [Andrew Epstein](https://github.com/andrew-epstein), for helping debug and fix a number of crashes
  * [melkor](https://github.com/hohav), for the original idea to shift replays to a columnar representation for better compression
  * [b3nd3r-ssbm](https://github.com/b3nd3r-ssbm), for code contributions to make sure things work on Windows
  * [cbartsch](https://github.com/cbartsch), for miscellaneous code fixes
  * [abatilo](https://github.com/abatilo), for contributing a Docker file and GitHub Action workflow + misc. fixes

## References
  * [Project Slippi's main GitHub page](https://github.com/project-slippi/project-slippi)
  * [Fizzi's .slp replay file specifications](https://github.com/project-slippi/slippi-wiki/blob/master/SPEC.md)
  * [SSBM 1.02 data sheet](https://docs.google.com/spreadsheets/d/1JX2w-r2fuvWuNgGb6D3Cs4wHQKLFegZe2jhbBuIhCG8/edit) (contains state info, internal game IDs, and other Melee stuff)
