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

	str = sj_object_get_string(pokemonDef, "type1");
	self->type1 = battle_type_to_enum(str);
	str = sj_object_get_string(pokemonDef, "type2");
	self->type2 = battle_type_to_enum(str);

	sj_object_get_int(pokemonDef, "hp", &self->maxHP);
	self->hp = self->maxHP;
	sj_object_get_uint8(pokemonDef, "attack", &self->attack);
	sj_object_get_uint8(pokemonDef, "defense", &self->defense);
	sj_object_get_uint8(pokemonDef, "specialAttack", &self->specialAttack);
	sj_object_get_uint8(pokemonDef, "specialDefense", &self->specialDefense);

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

	slog("%s:", self->name);
	for (i = 0; i < 4; i++)
	{
		if (self->moves[i]) slog("Move %i: %s", i, self->moves[i]->name);
		else slog("Move %i: undefined", i);
	}

	return self;
}

Uint8 pokemon_step_towards_player(Entity* self)
{
	GFC_Rect room_bounds;
	Level* level;
	Entity* player;
	Uint8 dist;
	Sint32 idx;
	GFC_Vector3D move, thinkPosOld;
	GFC_Vector2D thinkPosTileOld;

	if (!self) return 0;
	level = level_get_the();
	thinkPosOld = self->thinkPos;
	thinkPosTileOld = self->thinkPosTile;
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

	if (player->thinkPos.x > self->thinkPos.x) self->thinkPos.x = self->thinkPos.x + level->tileDim;
	else if (player->thinkPos.x < self->thinkPos.x) self->thinkPos.x = self->thinkPos.x - level->tileDim;
	if (player->thinkPos.y > self->thinkPos.y) self->thinkPos.y = self->thinkPos.y + level->tileDim;
	else if (player->thinkPos.y < self->thinkPos.y) self->thinkPos.y = self->thinkPos.y - level->tileDim;
	self->thinkPosTile = level_coords_to_tile(level, self->thinkPos);
	move = gfc_vector3d(
		thinkPosTileOld.x - self->thinkPosTile.x,
		thinkPosTileOld.y - self->thinkPosTile.y,
		0
	);
	self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
	if (!level_is_tile_free(level, self->thinkPosTile))
	{
		// NOTE: NPCs will not find alternate tiles that could work. Fix this in the future
		self->thinkPos = thinkPosOld;
		self->thinkPosTile = thinkPosTileOld;
		return 0;
	}

	idx = level_get_tile_index(level, thinkPosTileOld.x, thinkPosTileOld.y);
	level->entityMap[idx] = NULL;
	idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
	level->entityMap[idx] = self;
	self->dir = move;
	move.x /= -1;
	move.y /= -1;
	self->velocity = move;
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
	GFC_Vector3D move, thinkPosOld;
	GFC_Vector2D thinkPosTileOld;

	if (!self) return 0;
	level = level_get_the();
	thinkPosOld = self->thinkPos;
	thinkPosTileOld = self->thinkPosTile;
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

	if (!gfc_list_count(closest))
	{
		gfc_list_delete(closest);
		return 0;
	}
	i = (Uint8)(floor(gfc_random() * gfc_list_count(closest)));
	ent = gfc_list_get_nth(closest, i);
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
	if (!level_is_tile_free(level, self->thinkPosTile))
	{
		// NOTE: NPCs will not find alternate tiles that could work. Fix this in the future
		self->thinkPos = thinkPosOld;
		self->thinkPosTile = thinkPosTileOld;
		return 0;
	}

	gfc_list_delete(closest);
	idx = level_get_tile_index(level, thinkPosTileOld.x, thinkPosTileOld.y);
	level->entityMap[idx] = NULL;
	idx = level_get_tile_index(level, self->thinkPosTile.x, self->thinkPosTile.y);
	level->entityMap[idx] = self;
	self->dir = move;
	move.x /= -1;
	move.y /= -1;
	self->velocity = move;
	return 1;
}

int pokemon_ally_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	if (pokemon_step_towards_player(self)) return 1;
	if (pokemon_step_towards_npc(self, 1)) return 1;
	if (pokemon_step_towards_npc(self, 0)) return 1;
	// add additional movement code here
	return 1;
}

int pokemon_enemy_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	if (pokemon_step_towards_npc(self, 0)) return 1;
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
}