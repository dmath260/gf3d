#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "pokemon.h"

/**
* @brief spawns a new Pokemon and sets it to the player
* @param position the tile to spawn the Pokemon on
* @param pokemonName name of the Pokemon species to spawn
*/
Entity* player_spawn(GFC_Vector3D position, const char* pokemonName);

Entity* player_get_the();

#endif
