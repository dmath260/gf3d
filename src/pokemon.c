#include "simple_logger.h"

#include "gfc_config_def.h"

#include "level.h"
#include "pokemon.h"
#include "player.h"

int pokemon_think(Entity* self);
void pokemon_free(Entity* self);

Entity* pokemon_spawn(GFC_Vector3D position, const char* pokemonName, Uint8 team)
{
	Entity* self;
	Level* level;
	SJson* pokemonDef;
	const char* str;
	GFC_Vector2D tile = {0};
	GFC_Vector3D scale;
	if (!pokemonName) return NULL;
	pokemonDef = gfc_config_def_get_by_name("items", pokemonName);
	if (!pokemonDef)
	{
		slog("cannot load item name %s", pokemonName);
		return NULL;
	}

	self = entity_new();
	if (!self) return NULL;

	str = sj_object_get_string(pokemonDef, "model");
	if (str) self->model = model_load(str);

	self->team = team;
	level = level_get_the();
	self->position = gfc_vector3d(
		level->tileDim * position.x - level->tileDim / 2,
		level->tileDim * position.y - level->tileDim / 2,
		position.z
	);
	self->thinkPos = self->position;
	tile = coords_to_tile(level, self->thinkPos.x, self->thinkPos.y);
	level->entityMap[level_get_tile_index(level, tile.x, tile.y)] = self;
	self->bounds = gfc_box_from_config(sj_object_get_value(pokemonDef, "bounds"));
	sj_object_get_vector3d(pokemonDef, "scale", &scale);
	self->scale = scale;

	self->think = pokemon_think;
	self->free = pokemon_free;

	return self;
}

int ally_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	return 1;
}

int enemy_think(Entity* self)
{
	Entity* player;
	if (!self) return 0;
	player = player_get_the();
	return 1;
}

int pokemon_think(Entity* self)
{
	if (!self) return 0;
	if (self->team == 0) return ally_think(self);
	else return enemy_think(self);
}

void pokemon_free(Entity* self)
{
	if (!self) return;
}