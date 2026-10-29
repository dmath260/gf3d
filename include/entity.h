#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "model.h"

typedef struct Entity_S
{
	Uint8 _inuse;
	Model* model;
	GFC_Vector3D position;
	GFC_Vector3D rotation;
	GFC_Vector3D scale;
	GFC_Matrix4 mat;

	GFC_Box bounds;

	GFC_Vector3D velocity;
	GFC_Color colorMod;

	int (*think)(struct Entity_S *self);
	int (*update)(struct Entity_S* self);
	int (*draw)(struct Entity_S* self);

	void (*free)(struct Entity_S* self); // cleans up custom data
	void* data; // used for custom entity data. If set, you must specify a FREE function.
}Entity;

/**
* @brief initialize the entity manager and pre-allocate a bunch
* @param maxEnts how many entities can exist at any single moment
*/
void entity_init(Uint32 maxEnts);

/**
* @brief get a blank entity from the system
* @return NULL on error or out of memory, a valid pointer otherwise
*/
Entity* entity_new();

/**
* @brief call to clean up an entity after it is no longer needed
* @param ent the entity to clean up
*/
void entity_free(Entity* ent);

/**
* @brief run think functions on all active entities
*/
void entity_system_think();

/**
* @brief run updates on all active entities
*/
void entity_system_update();

/**
* @brief check if two entities collide
*/
int entity_collision_other(Entity* self, Entity* other);

/**
* @brief check if an entity is violating level bounds
*/
int entity_move_check(Entity* self, GFC_Vector3D offset);

/**
* @brief queue renders on all active entities
*/
void entity_system_draw();

#endif