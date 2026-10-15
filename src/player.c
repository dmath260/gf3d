#include "simple_logger.h"

#include "gfc_input.h"

#include "gf3d_vgraphics.h"

#include "player.h"

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

Entity* player_spawn(GFC_Vector3D position)
{
	PlayerEntityData* data;
	Entity* self;
	self = entity_new();
	if (!self) return NULL;
	self->model = model_load("models/dino.model");
	self->position = position;
	self->think = player_think;
	self->update = player_update;
	self->free = player_free;
	data = gfc_allocate_array(sizeof(PlayerEntityData), 1);
	if (data)
	{
		data->cameraFollowDistance = 50;
		data->cameraRotation.x = 0;
		data->cameraRotation.y = -GFC_PI / 6;
	}
	self->data = data;

	return self;
}

void player_free(Entity* self)
{
	PlayerEntityData* data;
	if (!self || !self->data) return;
	data = (PlayerEntityData*)self->data;
	// clean up any borrowed resources
	free(data);
	self->data = NULL;
}

int player_think(Entity* self)
{
	PlayerEntityData* data;
	GFC_Vector3D move = {0};
	if (!self || !self->data) return 0;
	data = (PlayerEntityData*)self->data;

	if (gfc_input_command_down("panup"))
	{
		data->cameraFollowDistance -= 0.1;
		//data->cameraRotation.y -= 0.05;
	}
	if (gfc_input_command_down("pandown"))
	{
		data->cameraFollowDistance += 0.1;
		//data->cameraRotation.y += 0.05;
	}
	if (gfc_input_command_down("panleft"))
	{
		data->cameraRotation.x -= 0.05;
	}
	if (gfc_input_command_down("panright"))
	{
		data->cameraRotation.x += 0.05;
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
		gfc_vector3d_normalize(&move);
		gfc_vector3d_rotate_about_z(&move, data->cameraRotation.x);
		self->velocity = move;
		self->rotation.z = data->cameraRotation.x + GFC_PI;
	}

	return 1;
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