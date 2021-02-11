#ifndef SCHEMA_H_
#define SCHEMA_H_

#include <string>

// Event OffSets
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
    const unsigned O_RNG_GAME_START = 0x13D; //Whether the file is encoded with this program


    // Frame start event offsets
    const unsigned O_RNG_FS         = 0x05;

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
    const unsigned O_PHYS_L         = 0x33;
    const unsigned O_PHYS_R         = 0x37;

    // Post-frame event offsets
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
    const unsigned O_SELF_AIR_X     = 0x35;
    const unsigned O_SELF_AIR_Y     = 0x39;
    const unsigned O_ATTACK_X       = 0x3D;
    const unsigned O_ATTACK_Y       = 0x41;
    const unsigned O_SELF_GROUND_X  = 0x45;

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
    const unsigned O_ROLLBACK_FRAME = 0x05;
}

#endif /* SCHEMA_H_ */
