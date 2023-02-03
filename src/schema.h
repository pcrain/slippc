#ifndef SCHEMA_H_
#define SCHEMA_H_

#include <string>

// Event Offsets
namespace slip {
    // Common offsets
    const unsigned O_FRAME          = 0x01;
    const unsigned O_PLAYER         = 0x05;
    const unsigned O_FOLLOWER       = 0x06;

    // Game start event offsets
    const unsigned O_SLP_MAJ        = 0x01;
    const unsigned O_SLP_MIN        = 0x02;
    const unsigned O_SLP_REV        = 0x03;
    const unsigned O_SLP_ENC        = 0x04;  //Whether the file is encoded with this program
    const unsigned O_GAMEBITS_1     = 0x05;
    const unsigned O_GAMEBITS_2     = 0x06;
    const unsigned O_GAMEBITS_3     = 0x07;
    const unsigned O_GAMEBITS_4     = 0x08;
    const unsigned O_SUDDEN_DEATH   = 0x0B;
    const unsigned O_IS_TEAMS       = 0x0D;
    const unsigned O_ITEM_SPAWN     = 0x10;
    const unsigned O_SD_SCORE       = 0x11;
    const unsigned O_STAGE          = 0x13;
    const unsigned O_TIMER          = 0x15;
    const unsigned O_ITEMBITS_1     = 0x28;
    const unsigned O_ITEMBITS_2     = 0x29;
    const unsigned O_ITEMBITS_3     = 0x2A;
    const unsigned O_ITEMBITS_4     = 0x2B;
    const unsigned O_ITEMBITS_5     = 0x2C;
    const unsigned O_PLAYERDATA     = 0x65;  //Beginning of player info block
    const unsigned O_RNG_GAME_START = 0x13D;
    const unsigned O_DASHBACK       = 0x141; //Also contains shield drop at +0x04
    const unsigned O_NAMETAG        = 0x161;
    const unsigned O_IS_PAL         = 0x1A1;
    const unsigned O_PS_FROZEN      = 0x1A2;
    const unsigned O_SCENE_MIN      = 0x1A3;
    const unsigned O_SCENE_MAJ      = 0x1A4;
    const unsigned O_DISP_NAME      = 0x1A5;
    const unsigned O_CONN_CODE      = 0x221;
    const unsigned O_SLIPPI_UID     = 0x249;
    const unsigned O_LANGUAGE       = 0x2BD;
    const unsigned O_MATCH_ID       = 0x2BE;
    const unsigned O_GAME_NUMBER    = 0x2F1;
    const unsigned O_TIEBREAKER_NUMBER = 0x2F5;

    // Player data block offsets
    const unsigned O_PLAYER_ID      = 0x00;
    const unsigned O_PLAYER_TYPE    = 0x01;
    const unsigned O_START_STOCKS   = 0x02;
    const unsigned O_COLOR          = 0x03;
    const unsigned O_SHADE          = 0x07;
    const unsigned O_HANDICAP       = 0x08;
    const unsigned O_TEAM_ID        = 0x09;
    const unsigned O_PLAYER_BITS    = 0x0C;
    const unsigned O_CPU_LEVEL      = 0x0F;
    const unsigned O_OFFENSE        = 0x14;
    const unsigned O_DEFENSE        = 0x18;
    const unsigned O_SCALE          = 0x1C;

    // Frame start event offsets
    const unsigned O_RNG_FS         = 0x05;
    const unsigned O_SCENE_COUNT    = 0x09;

    // Pre-frame event offsets
    const unsigned O_RNG_PRE        = 0x07;
    const unsigned O_ACTION_PRE     = 0x0B;
    const unsigned O_XPOS_PRE       = 0x0D;
    const unsigned O_YPOS_PRE       = 0x11;
    const unsigned O_FACING_PRE     = 0x15;
    const unsigned O_JOY_X          = 0x19;
    const unsigned O_JOY_Y          = 0x1D;
    const unsigned O_CX             = 0x21;
    const unsigned O_CY             = 0x25;
    const unsigned O_TRIGGER        = 0x29;
    const unsigned O_PROCESSED      = 0x2D;
    const unsigned O_BUTTONS        = 0x31;
    const unsigned O_PHYS_L         = 0x33;
    const unsigned O_PHYS_R         = 0x37;
    const unsigned O_UCF_ANALOG     = 0x3B;
    const unsigned O_DAMAGE_PRE     = 0x3C;

    // Post-frame event offsets
    const unsigned O_INT_CHAR_ID    = 0x07;
    const unsigned O_ACTION_POST    = 0x08;
    const unsigned O_XPOS_POST      = 0x0A;
    const unsigned O_YPOS_POST      = 0x0E;
    const unsigned O_FACING_POST    = 0x12;
    const unsigned O_DAMAGE_POST    = 0x16;
    const unsigned O_SHIELD         = 0x1A;
    const unsigned O_LAST_HIT_ID    = 0x1E;
    const unsigned O_COMBO          = 0x1F;
    const unsigned O_LAST_HIT_BY    = 0x20;
    const unsigned O_STOCKS         = 0x21;
    const unsigned O_ACTION_FRAMES  = 0x22;
    const unsigned O_STATE_BITS_1   = 0x26;
    const unsigned O_STATE_BITS_2   = 0x27;
    const unsigned O_STATE_BITS_3   = 0x28;
    const unsigned O_STATE_BITS_4   = 0x29;
    const unsigned O_STATE_BITS_5   = 0x2A;
    const unsigned O_HITSTUN        = 0x2B;
    const unsigned O_AIRBORNE       = 0x2F;
    const unsigned O_GROUND_ID      = 0x30;
    const unsigned O_JUMPS          = 0x32;
    const unsigned O_LCANCEL        = 0x33;
    const unsigned O_HURTBOX        = 0x34;
    const unsigned O_SELF_AIR_X     = 0x35;
    const unsigned O_SELF_AIR_Y     = 0x39;
    const unsigned O_ATTACK_X       = 0x3D;
    const unsigned O_ATTACK_Y       = 0x41;
    const unsigned O_SELF_GROUND_X  = 0x45;
    const unsigned O_HITLAG         = 0x49;
    const unsigned O_ANIM_INDEX     = 0x4D;

    // Item event offsets
    const unsigned O_ITEM_TYPE      = 0x05;
    const unsigned O_ITEM_STATE     = 0x07;
    const unsigned O_ITEM_FACING    = 0x08;
    const unsigned O_ITEM_XVEL      = 0x0C;
    const unsigned O_ITEM_YVEL      = 0x10;
    const unsigned O_ITEM_XPOS      = 0x14;
    const unsigned O_ITEM_YPOS      = 0x18;
    const unsigned O_ITEM_DAMAGE    = 0x1C;
    const unsigned O_ITEM_EXPIRE    = 0x1E;
    const unsigned O_ITEM_ID        = 0x22;
    const unsigned O_ITEM_MISC      = 0x26;
    const unsigned O_ITEM_OWNER     = 0x2A;
    const unsigned O_ITEM_END       = 0x2B;

    // Bookend event offsets
    const unsigned O_BOOKEND_FRAME  = 0x01;
    const unsigned O_ROLLBACK_FRAME = 0x05;

    // Game end event offsets
    const unsigned O_END_METHOD     = 0x01;
    const unsigned O_LRAS           = 0x02;
}

#endif /* SCHEMA_H_ */
