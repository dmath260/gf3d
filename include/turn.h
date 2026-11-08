#ifndef __TURN_H__
#define __TURN_H__

#include "entity.h"

/**
* @brief initializes the turn order
* @param maxPlayers the maximum number of players supported
* @param maxEnts the maximum number of entities supported (including players)
* @param maxTurns the maximum number of turns for a single floor
*/
void turn_init(Uint8 maxPlayers, Uint8 maxEnts, Uint16 maxTurns);

/**
* @brief adds an entity to the turn order
* @param ent the entity to add
* @return 1 on success, 0 on failure
*/
Uint8 turn_order_add(Entity* ent);

/**
* @brief checks if it's an entity's turn
* @param ent the entity to add
* @return 1 if it's the entity's turn, 0 otherwise
*/
Uint8 turn_is_my(Entity* ent);

/**
* @brief increments to the turn order
* @param ent the entity trying to increment the turn order
* @return 0 if the entity doesn't exist or it's someone else's turn, 1 otherwise
*/
Uint8 turn_increment(Entity* ent);

/**
* @brief removes an entity from the turn order
* @param ent the entity to remove
* @return 1 on success, 0 on failure
*/
Uint8 turn_remove(Entity* ent);

/**
* @brief resets the turn counter
* @note call when arriving at a new floor
*/
void turn_reset();

/**
* @brief sets a player to become the current player (first in turn order)
* @param ent the entity to set as player
* @return 1 on success, 0 on failure
*/
Uint8 turn_set_player(Entity* ent);

/**
* @brief checks if any entities that can take turns are currently moving
* @return 0 if everybody's stopped, 1 otherwise
* @note call when controlling the player or when an NPC is about to attack
*/
Uint8 turn_anybody_moving();

/**
* @brief returns the current turn order
* @return the current turn order
*/
Entity** turn_get_active_entities();

#endif
