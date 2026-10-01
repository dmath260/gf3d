#ifndef __MODEL_H__
#define __MODEL_H__

#include "simple_json.h"

#include "gfc_types.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_text.h"

#include "gf3d_pipeline.h"
#include "gf3d_mesh.h"

typedef struct
{
    GFC_Matrix4     model;
    GFC_Matrix4     view;
    GFC_Matrix4     proj;
    GFC_Vector4D    color;
}ModelUBO;

typedef struct
{
    int                         _refCount;
    GFC_TextLine                filename;               /**<the name of the file used to create the sprite*/
    Texture                    *texture;
    Mesh                       *mesh;                   /**<GPU handles for mesh data*/
    VkDescriptorSet            *descriptorSet;
}Model;

void model_init_system(Uint32 modelCount);
void model_close();
Model* model_load(const char* filename);
void model_free(Model* model);

/**
 * @brief deletes a specified model
 */
void model_delete(Model* model);

/**
 * @brief get the pipeline that is used to render basic 3d models
 * @return NULL on error or the pipeline in question
 */
Pipeline* model_get_pipeline();

/**
 * @brief queue up a render for the current draw frame
 * @param model the model to render
 * @param mat the model matrix
 * @param colorMod the color modifier for the model
 */
void model_queue_render(Model* model, GFC_Matrix4 mat, GFC_Color colorMod);

/**
 * @brief given a model matrix and basic color, build the modelUBO needed to render a model
 * @param modelMat the model Matrix
 * @param colorMod the color for the UBO
 */
ModelUBO model_get_ubo(
    GFC_Matrix4 modelMat,
    GFC_Color colorMod);

#endif
