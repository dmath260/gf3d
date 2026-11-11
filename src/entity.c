#include "simple_logger.h"

#include "entity.h"
#include "level.h"
#include "draw_3d.h"
#include "player.h"
#include "turn.h"

typedef struct
{
	Entity* entityList;
	Uint32 maxEnts;
	// OTHER STUFF HERE
}EntityManager;

static EntityManager entity_manager = {0};

void entity_close(void);

void entity_init(Uint32 maxEnts)
{
	if (!maxEnts)
	{
		slog("failed to initialize entity system: maxEnts is zero");
		return;
	}
	entity_manager.entityList = gfc_allocate_array(sizeof(Entity), maxEnts);
	if (!entity_manager.entityList)
	{
		slog("failed to allocate entity list for %i entities", maxEnts);
		return;
	}
	entity_manager.maxEnts = maxEnts;
	slog("Entity system initialized");
	atexit(entity_close);
}

Entity* entity_new()
{
	int i;
	if (!entity_manager.entityList) return NULL;
	for (i = 0; i < entity_manager.maxEnts; i++)
	{
		if (entity_manager.entityList[i]._inuse) continue;
		entity_manager.entityList[i]._inuse = 1;

		// ANY BOILERPLATE CODE GOES HERE
		entity_manager.entityList[i].scale = gfc_vector3d(1, 1, 1);
		entity_manager.entityList[i].colorMod = GFC_COLOR_WHITE;

		return &entity_manager.entityList[i];
	}
	return NULL;
}

void entity_free(Entity* ent)
{
	Level* level;
	Sint32 idx;
	if (!ent || !ent->_inuse) return;
	level = level_get_the();
	if (level)
	{
		idx = level_get_tile_index(level, ent->thinkPosTile.x, ent->thinkPosTile.y);
		if (idx >= 0) level->entityMap[idx] = NULL;
	}
	if (ent->free) ent->free(ent);
	model_free(ent->model);
	memset(ent, 0, sizeof(Entity));
}

void entity_think(Entity* self)
{
	if (!self || !self->_inuse || (self->team < 2 && !turn_is_my(self))) return;
	if (self->think)
	{
		if (self->think(self) == 1)
		{
			turn_increment(self);
			return;
		}
	}
}

void entity_system_think()
{
	int i;
	if (!entity_manager.entityList) return;
	for (i = 0; i < entity_manager.maxEnts; i++)
	{
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_think(&entity_manager.entityList[i]);
	}
}

void entity_update(Entity* self)
{
	if (!self || !self->_inuse) return;
	if (!entity_move_check(self, self->velocity))
	{
		gfc_vector3d_add(self->position, self->position, self->velocity);
		if (self->dir.x == -1 && self->position.x >= self->thinkPos.x ||
			self->dir.x == 1 && self->position.x <= self->thinkPos.x)
		{
			self->position.x = self->thinkPos.x;
			self->positionTile.x = self->thinkPosTile.x;
			//self->dir.x = 0;
			self->velocity.x = 0;
		}
		if (self->dir.y == -1 && self->position.y >= self->thinkPos.y ||
			self->dir.y == 1 && self->position.y <= self->thinkPos.y)
		{
			self->position.y = self->thinkPos.y;
			self->positionTile.y = self->thinkPosTile.y;
			//self->dir.y = 0;
			self->velocity.y = 0;
		}
	}

	if (self->update)
	{
		if (self->update(self) == 1) return;
	}
}

void entity_system_update()
{
	int i;
	if (!entity_manager.entityList) return;
	for (i = 0; i < entity_manager.maxEnts; i++)
	{
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_update(&entity_manager.entityList[i]);
	}
}

int entity_collision_other(Entity* self, Entity* other)
{
	GFC_Box a, b;
	if (!self || !other) return 0;
	a = self->bounds;
	b = other->bounds;
	gfc_vector3d_add(a, a, self->position);
	gfc_vector3d_add(b, b, other->position);
	return gfc_box_overlap(a, b);
}

int entity_move_check(Entity* self, GFC_Vector3D offset)
{
	GFC_Box aabb;
	if (!self) return 0;
	aabb = self->bounds;
	gfc_vector3d_add(aabb, self->bounds, self->position);
	gfc_vector3d_add(aabb, aabb, offset);
	return level_bounds_check(level_get_the(), aabb);
}

void entity_draw(Entity* ent)
{
	GFC_Color colorMod;
	GFC_Box aabb;
	if (!ent || !ent->_inuse) return;
	if (ent->draw)
	{
		if (ent->draw(ent) == 1) return;
	}
	gfc_matrix4_from_vectors(ent->mat, ent->position, ent->rotation, ent->scale);
	model_queue_render(ent->model, ent->mat, ent->colorMod);
	aabb = ent->bounds;
	aabb.x *= ent->scale.x;
	aabb.y *= ent->scale.y;
	aabb.z *= ent->scale.z;
	aabb.w *= ent->scale.x;
	aabb.h *= ent->scale.y;
	aabb.d *= ent->scale.z;
	gfc_vector3d_add(aabb,aabb,ent->position);
	if (ent == player_get_the()) colorMod = GFC_COLOR_YELLOW;
	else if (!ent->team) colorMod = GFC_COLOR_GREEN;
	else colorMod = GFC_COLOR_RED;
	draw_3d_cube(aabb, colorMod);
}

void entity_system_draw()
{
	int i;
	if (!entity_manager.entityList) return;
	for (i = 0; i < entity_manager.maxEnts; i++)
	{
		if (!entity_manager.entityList[i]._inuse) continue;
		entity_draw(&entity_manager.entityList[i]);
	}
}

void entity_close()
{
	int i;
	if (!entity_manager.entityList) return;
	for (i = 0; i < entity_manager.maxEnts; i++)
	{
		entity_free(&entity_manager.entityList[i]);
	}
	free(entity_manager.entityList);
	memset(&entity_manager, 0, sizeof(EntityManager));
	slog("Entity system closed");
}