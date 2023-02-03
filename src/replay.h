#ifndef REPLAY_H_
#define REPLAY_H_

#include <iostream>
#include <fstream>

#include "enums.h"
#include "util.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

const unsigned MAX_ITEMS     = 1024; //Max number of items to output
const int32_t  MAX_ITEM_LIFE = 1024; //Max number of frames to track an item

namespace slip {

struct SlippiFrame {
  //Parser stuff
  bool     alive         = false;  //For checking if this frame was actually set

  //Pre-frame stuff
  int32_t  frame         = 0;      //In-game frame number corresponding to this SlippiFrame (starts at -123)
  uint8_t  player        = 0;      //Port id of the player (0 = Port 1, 1 = Port 2, etc.)
  bool     follower      = false;  //Whether this player is a follower (e.g., 2nd climber)
  uint32_t seed          = 0;      //RNG seed at the beginning of the frame
  uint16_t action_pre    = 0;      //Action state at the beginning of the frame
  float    pos_x_pre     = 0.0f;   //X position at the beginning of the frame
  float    pos_y_pre     = 0.0f;   //Y position at the beginning of the frame
  float    face_dir_pre  = 0.0f;   //Facing direction at the beginning of the frame (-1 = left, +1 = right)
  float    joy_x         = 0.0f;   //Joystick X position
  float    joy_y         = 0.0f;   //Joystick Y position
  float    c_x           = 0.0f;   //C stick X position
  float    c_y           = 0.0f;   //C stick Y position
  float    trigger       = 0.0f;   //Analog trigger position (max of L or R)
  uint16_t buttons       = 0;      //Physical buttons pressed
  float    phys_l        = 0.0f;   //Physical L analog value (0-1)
  float    phys_r        = 0.0f;   //Physical R analog value (0-1)
  uint8_t  ucf_x         = 0;      //Raw UCF value
  float    percent_pre   = 0.0f;   //Percent / damage at the beginning of the frame

  //Post-frame stuff
  uint8_t  char_id       = 0;      //Internal character ID
  uint16_t action_post   = 0;      //Action state at the end of the frame
  float    pos_x_post    = 0.0f;   //X position at the end of the frame
  float    pos_y_post    = 0.0f;   //Y position at the end of the frame
  float    face_dir_post = 0.0f;   //Facing direction at the end of the frame (-1 = left, +1 = right)
  float    percent_post  = 0.0f;   //Percent / damage at the end of the frame
  float    shield        = 0.0f;   //Shield health (0-60)
  uint8_t  hit_with      = 0;      //Move ID of this player's last move that connected with an opponent
  uint8_t  combo         = 0;      //In-game combo counter
  uint8_t  hurt_by       = 0;      //Payer ID of the last opponent that hit this player
  uint8_t  stocks        = 0;      //Number of stocks remaining
  float    action_fc     = 0.0f;   //Action state frame counter
  uint8_t  flags_1       = 0;      //Player state bit flags 1
  uint8_t  flags_2       = 0;      //Player state bit flags 2
  uint8_t  flags_3       = 0;      //Player state bit flags 3
  uint8_t  flags_4       = 0;      //Player state bit flags 4
  uint8_t  flags_5       = 0;      //Player state bit flags 5
  float    hitstun       = 0.0f;   //Hitstun remaining, among other things
  bool     airborne      = false;  //Whether the player is airborne
  uint16_t ground_id     = 0;      //ID of the last ground the character stood on
  uint8_t  jumps         = 0;      //Number of jumps remaining
  uint8_t  l_cancel      = 0;      //L-cancel status (0 = N/A, 1 = success, 2 = failure)
  uint8_t  hurtbox       = 0;      //Hurtbox state (0 = vulnerable, 1 = invulnerable, 2 = intangible)
  float    self_air_x    = 0;      //Self-induced aerial X-velocity
  float    self_air_y    = 0;      //Self-induced aerial Y-velocity
  float    attack_x      = 0;      //Attack-induced X-velocity
  float    attack_y      = 0;      //Attack-induced Y-velocity
  float    self_grd_x    = 0;      //Self-induced grounded X-velocity
  float    hitlag        = 0;      //Number of hitlag frames remaining
  uint32_t anim_index    = 0;      //Animation index (used for Wait)
};

struct SlippiItemFrame {
  int32_t  frame         = 0;  //In-game frame number corresponding to this SlippiItemFrame
  uint8_t  state         = 0;  //Item state (undocumented)
  float    face_dir      = 0;  //Item facing direction  (-1 = left, +1 = right)
  float    xvel          = 0;  //Item X-velocity
  float    yvel          = 0;  //Item Y-velocity
  float    xpos          = 0;  //Item X-position
  float    ypos          = 0;  //Item Y-position
  uint16_t damage        = 0;  //Item damage taken
  float    expire        = 0;  //Number of frames until item expires
  uint8_t  flags_1       = 0;  //Item state flags 1
  uint8_t  flags_2       = 0;  //Item state flags 2
  uint8_t  flags_3       = 0;  //Item state flags 3
  uint8_t  flags_4       = 0;  //Item state flags 4
  int8_t   owner         = 0;  //Port ID of player that owns the item (-1 = unowned)
};

struct SlippiItem {
  uint16_t         type       = 0;           //Type of item this is
  uint32_t         spawn_id   = MAX_ITEMS+1; //ID of this item
  uint32_t         num_frames = 0;           //Number of frames this item was active
  SlippiItemFrame* frame      = nullptr;     //Pointer to array of data for item's individual frames
};

struct SlippiPlayer {
  uint8_t      ext_char_id  = 0;       //External character ID
  uint8_t      player_type  = 3;       //0 = human, 1 = CPU, 2 = demo, 3 = empty
  uint8_t      start_stocks = 0;       //Starting stocks for this player
  uint8_t      end_stocks   = 0;       //Ending stocks for this player
  uint8_t      color        = 0;       //Costume / color index
  uint8_t      team_id      = 0;       //Team index (0 = red, 1 = blue, 2 = green)
  uint8_t      cpu_level    = 0;       //CPU level for this player (only applicable for CPU players)
  uint32_t     dash_back    = 0;       //Type of dash-back fix enabled (0 = off, 1 = UCF, 2 = Arduino)
  uint32_t     shield_drop  = 0;       //Type of shield drop fix enabled (0 = off, 1 = UCF, 2 = Arduino)
  uint8_t      shade        = 0;       //Player costume shade (0 = normal, 1 = light, 2 = dark)
  uint8_t      handicap     = 0;       //Player handicap setting
  float        offense      = 0;       //Knockback multiplier when hitting another player
  float        defense      = 0;       //Knockback multiplier when getting hit by another player
  float        scale        = 0;       //Character's model scale
  bool         stamina      = false;   //Whether this character has a stamina bar
  bool         silent       = false;   //Whether this character is silent (no sound effects???)
  bool         low_gravity  = false;   //Whether this character has low gravity (bunny hood???)
  bool         invisible    = false;   //Whether this character is invisible
  bool         black_stock  = false;   //Whether this character has black stock icons
  bool         metal        = false;   //Whether this character is metal
  bool         warp_in      = false;   //Whether this character should warp in when spawning
  bool         rumble       = false;   //Whether this player has rumble enabled
  std::string  tag          = "";      //Player display tag grabbed from metadata
  std::string  tag_code     = "";      //Player connect code (as of 3.9.0, in both game start block and metadata)
  std::string  tag_css      = "";      //Player tag entered on character select screen
  std::string  disp_name    = "";      //Display name used on Slippi Online
  std::string  slippi_uid   = "";      //Firebase UID of Slippi player
  SlippiFrame* frame        = nullptr; //Pointer to array of data for player's individual frames
};

struct SlippiReplay {
  unsigned        errors              = 0;          //Number of errors that occurred during parsing
  uint32_t        slippi_version_raw  = 0;          //Raw Slippi version number
  std::string     slippi_version      = "";         //String representation of Slippi version number
  std::string     parser_version      = "";         //SlippC parser version number
  std::string     original_file       = "";         //Name of the input file used to generate this replay
  std::string     game_start_raw      = "";         //Base64-encoded game start block data
  std::string     metadata            = "";         //JSON metadata stored in the Slippi Replay
  std::string     played_on           = "";         //Platform this replay was played on (dolphin, console, or network)
  std::string     start_time          = "";         //Timestamp for when this game was played
  std::string     match_id            = "";         //An ID consisting of the mode and time the match started (e.g. mode.unranked-2022-12-20T06:52:39.18-0). Max 50 characters + null terminator
  int8_t          winner_id           = -1;         //Port ID of the game's winner
  bool            teams               = false;      //Whether this was a teams match
  uint16_t        stage               = 0;          //Stage ID for this game
  uint32_t        seed                = 0;          //Starting RNG seed
  uint32_t        game_number         = 0;          //For the given Match ID, starts at 1
  uint32_t        tiebreaker_number   = 0;          //For the given Game Number, will be 0 if not a tiebreak game
  bool            pal                 = false;      //Whether this was played on the PAL version of Melee
  bool            frozen_stadium      = false;      //Whether Pokemon Stadium is frozen or not
  uint8_t         scene_min           = 0;          //Minor scene number
  uint8_t         scene_maj           = 0;          //Major scene number
  uint8_t         end_type            = 0;          //Game end type (1 = TIME!, 2 = GAME!, 7 = No Contest)
  int8_t          lras                = -1;         //Port ID of player who initiated LRAStart
  int32_t         first_frame         = LOAD_FRAME; //Index of first frame of the game (always -123)
  int32_t         last_frame          = 0;          //Index of the last frame of the game
  uint32_t        frame_count         = 0;          //Total number of frames the game lasted (always == last_frame+123)
  uint8_t         timer               = 0;          //Number of minutes the timer started at
  int8_t          items_on            = 0;          //Item spawn rate (-1 = disabled, 0 = very low, 1 = low, etc.)
  int8_t          sd_score            = 0;          //How many points a player loses for SDing
  uint8_t         timer_behav         = 0;          //Timer behavior (0 =  off, 2 = count down, 3 = count up)
  uint8_t         ui_chars            = 0;          //Number of UI slots for characters on the in-game HUD (usually 4)
  uint8_t         game_mode           = 0;          //Game mode (0 = time, 1 = stock, 2 = coin, 3 = bonus)
  bool            friendly_fire       = false;      //Whether friendly fire is enabled
  bool            demo_mode           = false;      //Whether we are in the title screen or BTT dmeo
  bool            classic_adv         = false;      //Whether we are in classic or adventure mode
  bool            hrc_event           = false;      //Whether we are in HRC or some other event
  bool            allstar_wait1       = false;      //Whether we are in the all-star wait area
  bool            allstar_game1       = false;      //Same as above???
  bool            allstar_wait2       = false;      //Whether we are in an all-star game
  bool            allstar_game2       = false;      //Same as above???
  bool            single_button       = false;      //Whether we are in single button mode
  bool            pause_timer         = false;      //Whether the timer continues to count down when game is paused
  bool            pause_nohud         = false;      //Whether the HUD is shown when game is paused
  bool            pause_lras          = false;      //Whether LRAS is enabled when game is paused
  bool            pause_off           = false;      //Whether pause is disabld
  bool            pause_zretry        = false;      //Whether we can press Z to retry when game is paused
  bool            pause_analog        = false;      //Whether the analog stick is shown when game is paused
  bool            pause_score         = false;      //Whether the score is shown when game is paused
  uint8_t         items1              = 0;          //Item enabled / disabled bitfield 1
  uint8_t         items2              = 0;          //Item enabled / disabled bitfield 2
  uint8_t         items3              = 0;          //Item enabled / disabled bitfield 3
  uint8_t         items4              = 0;          //Item enabled / disabled bitfield 4
  uint8_t         items5              = 0;          //Item enabled / disabled bitfield 5
  bool            sudden_death        = false;      //Whether bombs start dropping after 20 seconds
  uint32_t        num_items           = 0;          //Number of distinct item IDs encountered during the game
  uint8_t         language            = 0;          //Language option (0 = Japanese, 1 = English)
  SlippiPlayer    player[8]           = {};         //Array of SlippiPlayers (1 main + follower for each port)
  SlippiItem      item[MAX_ITEMS]     = {};         //Array of SlippiItems (can track up to MAX_ITEMS per game)

  void setFrames(int32_t max_frames);
  void cleanup();
  std::string replayAsJson(bool delta);
};


}

#endif /* REPLAY_H_ */
