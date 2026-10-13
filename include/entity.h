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
	GFC_Vector3D velocity;
	GFC_Color colorMod;

	int (*think)(struct Entity_S *self);
	int (*update)(struct Entity_S* self);
	int (*draw)(struct Entity_S* self);
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
* @brief queue renders on all active entities
*/
void entity_system_draw();

#endif