#include "simple_logger.h"

#include "gfc_config.h"
#include "gfc_config_def.h"

#include "level.h"
#include "pokemon.h"
#include "player.h"
#include "turn.h"
#include "battle.h"

int pokemon_think(Entity* self);
void pokemon_free(Entity* self);

Entity* pokemon_spawn(GFC_Vector3D position, const char* pokemonName, Uint8 team)
{
	Entity* self;
	Level* level;
	SJson* pokemonDef, *moves, *moveName;
	const char* str;
	GFC_Vector2D tile = {0};
	GFC_Vector3D scale;
	int count, i;
	BattleMove* moveData;
	if (!pokemonName) return NULL;
	pokemonDef = gfc_config_def_get_by_name("pokemon", pokemonName);
	if (!pokemonDef)
	{
		slog("cannot load pokemon %s", pokemonName);
		return NULL;
	}

	self = entity_new();
	if (!self) return NULL;
	gfc_word_cpy(self->name, pokemonName);

	str = sj_object_get_string(pokemonDef, "model");
	if (str) self->model = model_load(str);

	self->team = team;
	level = level_get_the();
	self->position = gfc_vector3d(
		level->tileDim * position.x + level->tileDim / 2,
		level->tileDim * position.y + level->tileDim / 2,
		position.z
	);
	self->thinkPos = self->position;
	self->positionTile = gfc_vector2d(position.x, position.y);
	self->thinkPosTile = self->positionTile;
	tile = level_coords_to_tile(level, self->thinkPos);
	level->entityMap[level_get_tile_index(level, tile.x, tile.y)] = self;
	self->bounds = gfc_box_from_config(sj_object_get_value(pokemonDef, "bounds"));
	sj_object_get_vector3d(pokemonDef, "scale", &scale);
	self->scale = scale;
	self->rotation.z = GFC_PI;
	self->dir = gfc_vector3d(0, -1, 0);

	str = sj_object_get_string(pokemonDef, "type1");
	self->type1 = battle_type_to_enum(str);
	str = sj_object_get_string(pokemonDef, "type2");
	self->type2 = battle_type_to_enum(str);

	self->level = 10; // change all this when adding level-up stuff
	sj_object_get_int(pokemonDef, "hp", &self->maxHP);
	self->hp = self->maxHP;
	sj_object_get_uint8(pokemonDef, "attack", &self->attack);
	sj_object_get_uint8(pokemonDef, "defense", &self->defense);
	sj_object_get_uint8(pokemonDef, "specialAttack", &self->specialAttack);
	sj_object_get_uint8(pokemonDef, "specialDefense", &self->specialDefense);

	for (i = 0; i < 6; i++) self->statChanges[i] = 10;

	moves = sj_object_get_value(pokemonDef, "moves");
	if (sj_is_array(moves))
	{
		count = sj_array_count(moves);
		if (count > 4) count = 4;
		for (i = 0; i < count; i++)
		{
			moveName = sj_array_get_nth(moves, i);
			moveData = battle_get_move(sj_get_string_value(moveName));
			if (moveData)
			{
				self->moves[i] = moveData;
				self->currentPP[i] = moveData->maxPP;
			}
		}
	}

	self->think = pokemon_think;
	self->free = pokemon_free;
	turn_order_add(self);

	return self;
}

Entity* pokemon_enemy_spawn_random()
{
	int i, idx;
	GFC_Vector2D tile = {0};
	Level* level;
	GFC_TextWord enemyName;
	Uint8 dist;
	Entity* player;
	level = level_get_the();
	player = player_get_the();
	for (i = 0; i < 100; i++)
	{
		tile.x = floor(level->tileSize.x * gfc_random());
		tile.y = floor(level->tileSize.y * gfc_random());
		idx = level_get_tile_index(level, tile.x, tile.y);
		dist = max(
			abs(tile.x - player->thinkPosTile.x),
			abs(tile.y - player->thinkPosTile.y)
		);
		// spawn in rooms only, and don't spawn too close to the player
		if (!level_is_tile_free(level, tile) || level->tileMap[idx] < 2 || dist <= level->lightRange)
			continue;

		idx = floor(level->enemyCount * gfc_random());
		gfc_word_cpy(enemyName, level->enemyList[idx]);
		if (!enemyName) continue;
		return pokemon_spawn(gfc_vector3d(tile.x, tile.y, 0), enemyName, 1);
	}

	return NULL;
}

void pokemon_relegate_player()
{
	Entity* player;
	player = player_get_the();
	player->think = pokemon_think;
	//player->update = pokemon_update;
	player->update = NULL;
	player->free = pokemon_free;
	player->data = NULL;
}

Uint8 pokemon_find_valid_adjacent_tiles(Entity* self)
{
	GFC_Vector2D tile1 = {0};
	GFC_Vector2D tile2 = {0};
	Uint8 valid1, valid2;
	Level* level;

	if (!self) return 0;
	if (self->dir.x != -1.0 && self->dir.x != 1.0 && self->dir.y != -1.0 && self->dir.y != 1.0 ||
		self->dir.x < -1.0 || self->dir.x > 1.0 || self->dir.y < -1.0 || self->dir.x > 1.0)
		return 0;
	if (!self->dir.x)
	{
		tile1 = gfc_vector2d(self->thinkPosTile.x - 1, self->thinkPosTile.y);
		tile2 = gfc_vector2d(self->thinkPosTile.x + 1, self->thinkPosTile.y);
	}
	else if (!self->dir.y)
	{
		tile1 = gfc_vector2d(self->thinkPosTile.x, self->thinkPosTile.y - 1);
		tile2 = gfc_vector2d(self->thinkPosTile.x, self->thinkPosTile.y + 1);
	}
	else
	{
		tile1 = gfc_vector2d(self->thinkPosTile.x, self->thinkPosTile.y + self->dir.y);
		tile2 = gfc_vector2d(self->thinkPosTile.x + self->dir.x, self->thinkPosTile.y);
	}

	//slog("(%f, %f) | (%f, %f) | (%f, %f)",
	//	self->thinkPosTile.x, self->thinkPosTile.y, tile1.x, tile1.y, tile2.x, tile2.y);
	level = level_get_the();
	if (!level_is_tile_free(level, tile1)) valid1 = 0;
	else valid1 = 1;
	if (!level_is_tile_free(level, tile2)) valid2 = 0;
	else valid2 = 1;

	if (!valid1 && !valid2) return 0;
	else if (valid1 && (!valid2 || gfc_random() < 0.5))
	{
		self->thinkPosTile.x = tile1.x;
		self->thinkPosTile.y = tile1.y;
	}
	else
	{
		self->thinkPosTile.x = tile2.x;
		self->thinkPosTile.y = tile2.y;
	}
	return 1;
}

Uint8 pokemon_step_towards_player(Entity* self)
{
	GFC_Rect room_bounds;
	Level* level;
	Entity* player;
	Uint8 dist;
	Sint32 idx;
	GFC_Vector3D move, thinkPosOld, dirOld;
	GFC_Vector2D thinkPosTileOld;

	if (!self || !gfc_vector3d_compare(self->position, self->thinkPos)) return 0;
	level = level_get_the();
	thinkPosOld = self->thinkPos;
	thinkPosTileOld = self->thinkPosTile;
	dirOld = self->dir;
	room_bounds = level_get_room_bounds(level, self->thinkPos);

	player = player_get_the();
	if (!player) return 0;
	idx = level_get_tile_index(level, player->thinkPosTile.x, player->thinkPosTile.y);
	if (idx == -1 || !level->tileMap[idx]) return 0;
	dist = max(
		abs(self->thinkPosTile.x - player->thinkPosTile.x),
		abs(self->thinkPosTile.y - player->thinkPosTile.y)
	);
	if (level->tileMap[idx] == 2 && (
		player->thinkPosTile.x < room_bounds.x || player->thinkPosTile.x > room_bounds.w ||
		player->thinkPosTile.y < room_bounds.y || player->thinkPosTile.y > room_bounds.h
		) && dist > level->lightRange) return 0;
	if (level->tileMap[idx] == 1 && dist > level->lightRange) return 0;

	if (player->thinkPos.x > self->thinkPos.x) self->thinkPos.x += level->tileDim;
	else if (player->thinkPos.x < self->thinkPos.x) self->thinkPos.x -= level->tileDim;
	if (player->thinkPos.y > self->thinkPos.y) self->thinkPos.y += level->tileDim;
	else if (player->thinkPos.y < self->thinkPos.y) self->thinkPos.y -= level->tileDim;
	self->thinkPosTile = level_coords_to_tile(level, self->thinkPos);
	move = gfc_vector3d(
		thinkPosTileOld.x - self->thinkPosTile.x,
		thinkPosTileOld.y - self->thinkPosTile.y,
		0
	);
	self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
	self->dir = move;
	if (!level_is_tile_free(level, self->thinkPosTile))
	{
		if ((dist == 1 && (!move.x || !move.y)) || !pokemon_find_valid_adjacent_tiles(self))
		{
			self->thinkPos = thinkPosOld;
			self->thinkPosTile = thinkPosTileOld;
			self->dir = dirOld;
			self->rotation.z = SDL_atan2f(dirOld.y, dirOld.x) - GFC_PI / 2;
			return 1;
		}
		move = gfc_vector3d(
			thinkPosTileOld.x - self->thinkPosTile.x,
			thinkPosTileOld.y - self->thinkPosTile.y,
			0
		);
		self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
		self->dir = move;
		self->thinkPos.x = level->tileDim * (self->thinkPosTile.x + 0.5);
		self->thinkPos.y = level->tileDim * (self->thinkPosTile.y + 0.5);
	}

	idx = level_get_tile_index(level, thinkPosTileOld.x, thinkPosTileOld.y);
	level->entityMap[idx] = NULL;
	idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
	level->entityMap[idx] = self;
	move.x /= -1;
	move.y /= -1;
	self->velocity = move;
	//slog("%f %f --> %f %f", thinkPosTileOld.x, thinkPosTileOld.y, self->thinkPosTile.x, self->thinkPosTile.y);
	//slog("%f %f", self->thinkPos.x, self->thinkPos.y);
	return 1;
}

Uint8 pokemon_step_towards_npc(Entity* self, Uint8 team)
{
	GFC_Rect room_bounds;
	Level* level;
	Entity** ents, *ent;
	GFC_List* closest;
	Uint8 i, min, maxEnts, dist;
	Sint32 idx;
	GFC_Vector3D move, thinkPosOld, dirOld;
	GFC_Vector2D thinkPosTileOld;

	if (!self || !gfc_vector3d_compare(self->position, self->thinkPos)) return 0;
	level = level_get_the();
	thinkPosOld = self->thinkPos;
	thinkPosTileOld = self->thinkPosTile;
	dirOld = self->dir;
	room_bounds = level_get_room_bounds(level, self->thinkPos);
	ents = turn_get_active_entities();
	maxEnts = sizeof(ents) / sizeof(Entity*);
	closest = gfc_list_new();
	min = -1;

	for (i = 0; i < maxEnts; i++)
	{
		ent = ents[i];
		if (!ent || ent->team != team) continue;
		idx = level_get_tile_index(level, ent->thinkPosTile.x, ent->thinkPosTile.y);
		if (idx == -1 || !level->tileMap[idx]) continue;
		dist = max(
			abs(self->thinkPosTile.x - ent->thinkPosTile.x),
			abs(self->thinkPosTile.y - ent->thinkPosTile.y)
		);
		if (level->tileMap[idx] == 2 && (
			ent->thinkPosTile.x < room_bounds.x || ent->thinkPosTile.x > room_bounds.w ||
			ent->thinkPosTile.y < room_bounds.y || ent->thinkPosTile.y > room_bounds.h
			) && dist > level->lightRange) return 0;
		if (dist > min || level->tileMap[idx] == 1 && dist > level->lightRange) continue;
		if (dist < min)
		{
			min = dist;
			gfc_list_clear(closest);
		}
		gfc_list_append(closest, ent);
	}

	while (gfc_list_count(closest))
	{
		i = (Uint8)(floor(gfc_random() * gfc_list_count(closest)));
		ent = gfc_list_get_nth(closest, i);
		gfc_list_delete_nth(closest, i);
		if (ent->thinkPos.x > self->thinkPos.x) self->thinkPos.x = self->thinkPos.x + level->tileDim;
		else if (ent->thinkPos.x < self->thinkPos.x) self->thinkPos.x = self->thinkPos.x - level->tileDim;
		if (ent->thinkPos.y > self->thinkPos.y) self->thinkPos.y = self->thinkPos.y + level->tileDim;
		else if (ent->thinkPos.y < self->thinkPos.y) self->thinkPos.y = self->thinkPos.y - level->tileDim;
		self->thinkPosTile = level_coords_to_tile(level, self->thinkPos);
		move = gfc_vector3d(
			thinkPosTileOld.x - self->thinkPosTile.x,
			thinkPosTileOld.y - self->thinkPosTile.y,
			0
		);
		self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
		self->dir = move;
		if (!level_is_tile_free(level, self->thinkPosTile))
		{
			if (!pokemon_find_valid_adjacent_tiles(self))
				continue;
			else if (dist == 1 && (!move.x || !move.y))
			{
				self->thinkPos = thinkPosOld;
				self->thinkPosTile = thinkPosTileOld;
				self->dir = dirOld;
				self->rotation.z = SDL_atan2f(dirOld.y, dirOld.x) - GFC_PI / 2;
				return 1;
			}
			move = gfc_vector3d(
				thinkPosTileOld.x - self->thinkPosTile.x,
				thinkPosTileOld.y - self->thinkPosTile.y,
				0
			);
			self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
			self->dir = move;
			self->thinkPos.x = level->tileDim * (self->thinkPosTile.x + 0.5);
			self->thinkPos.y = level->tileDim * (self->thinkPosTile.y + 0.5);
		}

		gfc_list_delete(closest);
		idx = level_get_tile_index(level, thinkPosTileOld.x, thinkPosTileOld.y);
		level->entityMap[idx] = NULL;
		idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
		level->entityMap[idx] = self;
		move.x /= -1;
		move.y /= -1;
		self->velocity = move;
		return 1;
	}
	self->thinkPos = thinkPosOld;
	self->thinkPosTile = thinkPosTileOld;
	self->dir = dirOld;
	self->rotation.z = SDL_atan2f(dirOld.y, dirOld.x) - GFC_PI / 2;
	gfc_list_delete(closest);
	return 0;
}

Uint8 pokemon_attack_random(Entity* self)
{
	int i, j;
	Level* level;
	Entity* ent;
	GFC_List* adj;

	if (!self || !gfc_vector3d_compare(self->position, self->thinkPos)) return 0;
	level = level_get_the();
	adj = gfc_list_new();

	for (i = self->thinkPosTile.x - 1; i <= self->thinkPosTile.x + 1; i++)
	{
		for (j = self->thinkPosTile.y - 1; j <= self->thinkPosTile.y + 1; j++)
		{
			ent = level->entityMap[j * level->tileSize.x + i];
			if (ent && ent->team != self->team)
			{
				gfc_list_append(adj, ent);
			}
		}
	}

	while (gfc_list_count(adj))
	{
		i = (Uint8)(floor(gfc_random() * gfc_list_count(adj)));
		ent = gfc_list_get_nth(adj, i);
		gfc_list_delete_nth(adj, i);

		self->dir = gfc_vector3d(
			self->thinkPosTile.x - ent->thinkPosTile.x,
			self->thinkPosTile.y - ent->thinkPosTile.y,
			0
		);
		self->rotation.z = SDL_atan2f(self->dir.y, self->dir.x) - GFC_PI / 2;
		while (1)
		{
			if (self->currentPP[0] + self->currentPP[1] + self->currentPP[2] + self->currentPP[3] < 1)
			{
				slog("No PP left for current moves");
				break;
			}
			i = (Uint8)(floor(gfc_random() * 4));
			if (!self->currentPP[i]) continue;
			battle_use_move(self, ent, i);
			break;
		}

		gfc_list_delete(adj);
		return 1;
	}
	gfc_list_delete(adj);
	return 0;
}

Uint8 pokemon_wander_random(Entity* self)
{
	int i, j, idx, count;
	Level* level;
	GFC_Vector2D tileList[9];

	if (!self || !gfc_vector3d_compare(self->position, self->thinkPos)) return 0;
	level = level_get_the();
	count = 0;

	for (i = self->thinkPosTile.x - 1; i <= self->thinkPosTile.x + 1; i++)
	{
		for (j = self->thinkPosTile.y - 1; j <= self->thinkPosTile.y + 1; j++)
		{
			tileList[count] = gfc_vector2d(i, j);
			if (!level_is_tile_free(level, tileList[count])) continue;
			count++;
		}
	}

	while (count)
	{
		i = (Uint8)(floor(gfc_random() * count));

		self->dir = gfc_vector3d(
			self->thinkPosTile.x - tileList[i].x,
			self->thinkPosTile.y - tileList[i].y,
			0
		);
		self->rotation.z = SDL_atan2f(self->dir.y, self->dir.x) - GFC_PI / 2;
		idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
		level->entityMap[idx] = NULL;

		self->thinkPosTile.x = tileList[i].x;
		self->thinkPosTile.y = tileList[i].y;
		self->thinkPos.x = level->tileDim * (self->thinkPosTile.x + 0.5);
		self->thinkPos.y = level->tileDim * (self->thinkPosTile.y + 0.5);
		idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
		level->entityMap[idx] = self;
		self->velocity = gfc_vector3d(self->dir.x * -1, self->dir.y * -1, 0);

		/**
		for (i++; i < count; i++)
		{
			tileList[i - 1] = tileList[i];
		}
		count--;
		*/

		return 1;
	}
	return 0;
}

int pokemon_ally_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	if (pokemon_attack_random(self)) return 1;
	if (pokemon_step_towards_player(self)) return 1;
	if (pokemon_step_towards_npc(self, 1)) return 1;
	if (pokemon_step_towards_npc(self, 0)) return 1;
	if (pokemon_wander_random(self)) return 1;
	// add additional movement code here
	return 1;
}

int pokemon_enemy_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	if (pokemon_attack_random(self)) return 1;
	if (pokemon_step_towards_npc(self, 0)) return 1;
	if (pokemon_wander_random(self)) return 1;
	// add additional movement code here
	return 1;
}

int pokemon_think(Entity* self)
{
	if (!self) return 0;
	if (self->team == 0) return pokemon_ally_think(self);
	else return pokemon_enemy_think(self);
}

void pokemon_free(Entity* self)
{
	if (!self) return;
	turn_remove(self);
}