#include "simple_logger.h"

#include "gfc_config_def.h"

#include "player.h"
#include "item.h"

int item_think(Entity* self)
{
	if (!self) return;
	if (entity_collision_other(self, player_get_the()))
	{
		self->scale.x += 0.0005;
		self->scale.y += 0.0005;
		self->scale.z += 0.0005;
		//entity_free(self);
		//return;
	}
}

Entity* item_spawn(const char* itemName, GFC_Vector3D position)
{
	const char* str;
	SJson *itemDef;
	Entity* self;
	if (!itemName) return NULL;
	itemDef = gfc_config_def_get_by_name("items", itemName);
	if (!itemDef)
	{
		slog("cannot load item name %s", itemName);
		return NULL;
	}

	self = entity_new();
	if (!self) return NULL;

	str = sj_object_get_string(itemDef, "model");
	if (str) self->model = model_load(str);
	self->model = model_load(str);
	self->position = position;
	self->bounds = gfc_box_from_config(sj_object_get_value(itemDef, "bounds"));
	self->think = item_think;
	self->scale = gfc_vector3d(2, 2, 2);
	return self;
}