#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "pokemon.h"

/**
* @brief spawns a new Pokemon and sets it to the player
* @param position the tile to spawn the Pokemon on
* @param pokemonName name of the Pokemon species to spawn
*/
Entity* player_spawn(GFC_Vector3D position, const char* pokemonName);

/**
* @brief gets a pointer to the player and returns it
* @return pointer to the player if it exists, NULL otherwise
*/
Entity* player_get_the();

/**
* @brief sets an entity to become the player
* @param self the entity to promote to player
*/
void player_set(Entity* self);

#endif
