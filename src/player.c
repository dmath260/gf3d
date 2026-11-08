#include "simple_logger.h"

#include "gfc_input.h"

#include "gf3d_vgraphics.h"

#include "player.h"
#include "level.h"
#include "turn.h"

static Entity* thePlayer = NULL;

typedef struct
{
	float cameraFollowDistance;
	GFC_Vector2D cameraRotation;

	// other player-specific data
	float xp;
	float level;
	float strength;
	float rizz;
	float drip;
	float skibidi;
}PlayerEntityData;

int player_think(Entity* self);
int player_update(Entity* self);
void player_free(Entity* self);

Entity* player_get_the()
{
	return thePlayer;
}

Entity* player_spawn(GFC_Vector3D position, const char* pokemonName)
{
	PlayerEntityData* data;
	Entity* self;
	if (thePlayer) return thePlayer;
	self = pokemon_spawn(position, pokemonName, 0);
	if (!self) return NULL;
	self->think = player_think;
	self->update = player_update;
	self->free = player_free;

	data = gfc_allocate_array(sizeof(PlayerEntityData), 1);
	if (data)
	{
		data->cameraFollowDistance = 120;
		data->cameraRotation.x = GFC_PI;
		data->cameraRotation.y = GFC_PI / 4;
	}
	self->data = data;
	thePlayer = self;
	turn_set_player(self);

	return self;
}

void player_free(Entity* self)
{
	PlayerEntityData* data;
	if (!self || !self->data) return;
	data = (PlayerEntityData*)self->data;
	// clean up any borrowed resources
	free(data);
	thePlayer = NULL;
	self->data = NULL;
}

int player_think(Entity* self)
{
	PlayerEntityData* data;
	GFC_Vector3D move = {0};
	Level* level;
	GFC_Vector2D tile = {0};
	int idx_new, idx_old;
	if (!self || !self->data || turn_anybody_moving()) return 0;
	data = (PlayerEntityData*)self->data;

	if (gfc_input_command_down("panup"))
	{
		//data->cameraFollowDistance -= 0.1;
		//data->cameraRotation.y -= 0.05;
	}
	if (gfc_input_command_down("pandown"))
	{
		//data->cameraFollowDistance += 0.1;
		//data->cameraRotation.y += 0.05;
	}
	if (gfc_input_command_down("panleft"))
	{
		//data->cameraRotation.x -= 0.05;
	}
	if (gfc_input_command_down("panright"))
	{
		//data->cameraRotation.x += 0.05;
	}

	if (gfc_input_command_down("item_up"))
	{
		move.y = 1;
	}
	if (gfc_input_command_down("item_down"))
	{
		move.y = -1;
	}
	if (gfc_input_command_down("item_left"))
	{
		move.x = -1;
	}
	if (gfc_input_command_down("item_right"))
	{
		move.x = 1;
	}

	if (move.x || move.y)
	{
		self->rotation.z = SDL_atan2f(move.y, move.x) - GFC_PI / 2;
		level = level_get_the();
		self->thinkPos.x = self->position.x - move.x * level->tileDim;
		self->thinkPos.y = self->position.y - move.y * level->tileDim;
		tile = level_coords_to_tile(level, self->thinkPos);
		idx_new = level_get_tile_index(level, tile.x, tile.y);
		if (tile.x == -1 || !level->tileMap[idx_new] ||
			(level->entityMap[idx_new] && level->entityMap[idx_new]->team) ||
			(move.x != 0 && move.y != 0 && (
			!level->tileMap[level_get_tile_index(level, tile.x + move.x, tile.y)] ||
			!level->tileMap[level_get_tile_index(level, tile.x, tile.y + move.y)]
		))) {
			self->thinkPos = self->position;
			return 0;
		}
		self->thinkPosTile = tile;
		tile = level_coords_to_tile(level, self->position);
		idx_old = level_get_tile_index(level, tile.x, tile.y);
		if (level->entityMap[idx_new] && !level->entityMap[idx_new]->team)
		{
			level->entityMap[idx_new]->thinkPos = self->position;
			level->entityMap[idx_new]->thinkPosTile = tile;
			level->entityMap[idx_new]->dir = gfc_vector3d(move.x * -1, move.y * -1, move.z);
			level->entityMap[idx_new]->velocity = move;
			level->entityMap[idx_old] = level->entityMap[idx_new];
		}
		else level->entityMap[idx_old] = NULL;
		level->entityMap[idx_new] = self;
		self->dir = move;
		//gfc_vector3d_normalize(&move);
		move.x /= -1;
		move.y /= -1;
		self->velocity = move;
		return 1;
	}
	return 0;
}

int player_update(Entity* self)
{
	PlayerEntityData* data;
	GFC_Matrix4* view;
	GFC_Vector3D offset = {0, -1, 0};
	if (!self || !self->data) return 0;
	data = (PlayerEntityData*)self->data;

	gfc_vector3d_scale(offset, offset, data->cameraFollowDistance);
	view = gf3d_vgraphics_get_view_matrix();
	gfc_vector3d_rotate_about_z(&offset, data->cameraRotation.x);
	gfc_vector3d_rotate_about_x(&offset, data->cameraRotation.y);
	gfc_vector3d_add(offset, offset, self->position);
	gfc_matrix4_view(
		*view,
		offset,
		self->position,
		gfc_vector3d(0, 0, 1)
	);

	return 1;
}