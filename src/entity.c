#include "simple_logger.h"

#include "entity.h"

typedef struct
{
	Entity* entityList;
	Uint32 maxEnts;
	// OTHER STUFF HERE
}EntityManager;

static EntityManager entity_manager = {0};

void entity_close();

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
	if (!ent || !ent->_inuse) return;
	if (ent->free) ent->free(ent);
	model_free(ent->model);
	memset(ent, 0, sizeof(Entity));
}

void entity_think(Entity* self)
{
	if (!self || !self->_inuse) return;
	if (self->think)
	{
		if (self->think(self) == 1) return;
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
	gfc_vector3d_add(self->position, self->position, self->velocity);

	// Dampening
	if (gfc_vector3d_magnitude_squared(self->velocity) > 0.01)
	{
		gfc_vector3d_scale(self->velocity, self->velocity, 0.8);
	}
	else gfc_vector3d_clear(self->velocity);

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
	if (!self || !other) return;
	a = self->bounds;
	b = other->bounds;
	gfc_vector3d_add(a, a, self->position);
	gfc_vector3d_add(b, b, other->position);
	return gfc_box_overlap(a, b);
}

void entity_draw(Entity* ent)
{
	if (!ent || !ent->_inuse) return;
	if (ent->draw)
	{
		if (ent->draw(ent) == 1) return;
	}
	gfc_matrix4_from_vectors(ent->mat, ent->position, ent->rotation, ent->scale);
	model_queue_render(ent->model, ent->mat, ent->colorMod);
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