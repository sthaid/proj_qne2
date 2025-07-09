#include <common.h>

// -----------------  HUMAN PLAYER - GET_MOVE -------------------------------

int human_get_move(board_t *b)
{
    // return values, these are all from a registered event 
    // having been selected
    // - MOVE r,c
    // - MOVE_PASS
    // - MOVE_GAME_RESET

    return sdl_get_event(-1);
}
