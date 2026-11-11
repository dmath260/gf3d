#ifndef __POKEMON_H__
#define __POKEMON_H__

#include "entity.h"

/**
* @brief spawns a new Pokemon
* @param position the tile to spawn the Pokemon on
* @param pokemonName name of the Pokemon species to spawn
* @param team 0 for ally, 1 for enemy
*/
Entity* pokemon_spawn(GFC_Vector3D position, const char* pokemonName, Uint8 team);

/**
* @brief relegates the player to a regular pokemon
* @note this means replacing its think/update/free/etc. functions
*/
void pokemon_relegate_player();

#endif
