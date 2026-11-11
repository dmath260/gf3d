#ifndef __BATTLE_H__
#define __BATTLE_H__

#include "gfc_text.h"

#include "entity.h"
#include "battle_types.h"

/**
* @brief initializes the battle system
*/
void battle_init();

/**
* @brief converts a type from string to enum
* @param type the name of the type as a string
* @return the enum corresponding to the type, or BT_NONE on error
*/
BattleType battle_type_to_enum(const char* type);

/**
* @brief uses a move
* @param user the entity using a move
* @param target the target of the move
* @param moveIndex the index of the move to use in the user's move list (0 to 3)
*/
void battle_use_move(Entity* user, Entity* target, Uint8 moveIndex);

/**
* @brief returns data for a move if found and loads it from config otherwise
* @param name the name of the move to find/load
* @return NULL on failure and the move otherwise
*/
BattleMove* battle_get_move(const char* name);

#endif
