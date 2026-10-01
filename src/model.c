#include <stdio.h>

#include "simple_logger.h"
#include "simple_json.h"

#include "gfc_types.h"

#include "gf3d_buffers.h"
#include "gf3d_swapchain.h"
#include "gf3d_vgraphics.h"
#include "gf3d_pipeline.h"
#include "gf3d_commands.h"

#include "model.h"

#define MESH_ATTRIBUTE_COUNT 3

typedef struct
{
	Model* modelList;
	Uint32 modelCount;
    Pipeline* pipe;
    VkDevice device;
    Texture* defaultTexture;
}ModelManager;

static ModelManager model_manager = {0};

void model_init_system(Uint32 modelCount)
{
    if (model_manager.modelCount != 0)
    {
        slog("Cannot init model system, already initialized");
        return;
    }
    if (modelCount == 0)
    {
        slog("Cannot initialize model system for ZERO modeles");
        return;
    }
    model_manager.modelList = gfc_allocate_array(sizeof(Model), modelCount);
    model_manager.device = gf3d_vgraphics_get_default_logical_device();
    if (!model_manager.modelList) return;
    model_manager.modelCount = modelCount;

    model_manager.pipe = gf3d_pipeline_create_from_config(
        model_manager.device,
        "config/model_pipeline.cfg",
        gf3d_vgraphics_get_view_extent(),
        modelCount,
        gf3d_mesh_get_bind_description(),
        gf3d_mesh_get_attribute_descriptions(NULL),
        MESH_ATTRIBUTE_COUNT,
        sizeof(ModelUBO),
        VK_INDEX_TYPE_UINT16
    );
    if (!model_manager.pipe)
    {
        slog("Failed to make pipeline for models!");
        slog_sync();
        model_close();
        exit(-1);
        return;
    }

    model_manager.defaultTexture = gf3d_texture_load("images/default.png");
    if (!model_manager.defaultTexture)
    {
        slog("Default texture does not exist, crash incoming");
        slog_sync();
        model_close();
        exit(-1);
        return;
    }
    atexit(model_close);
}

Model* model_new()
{
    int i;
    for (i = 0; i < model_manager.modelCount; i++)
    {
        if ((model_manager.modelList[i]._refCount == 0) && (strlen(model_manager.modelList[i].filename) == 0))
        {
            model_manager.modelList[i]._refCount = 1;
            return &model_manager.modelList[i];
        }
    }
    for (i = 0; i < model_manager.modelCount; i++)
    {
        if (model_manager.modelList[i]._refCount == 0)
        {
            if (strlen(model_manager.modelList[i].filename) > 0)
            {
                model_delete(&model_manager.modelList[i]);
            }
            model_manager.modelList[i]._refCount = 1;
            return &model_manager.modelList[i];
        }
    }
    return NULL;
}

void model_free(Model *model)
{
    if (!model) return;
    model->_refCount--;
}

void model_delete(Model* model)
{
    if (!model) return;
    gf3d_texture_free(model->texture);
    gf3d_mesh_free(model->mesh);
    //model_free(model);
    memset(model, 0, sizeof(Model));
}

void model_close()
{
    int i;
    gf3d_texture_free(model_manager.defaultTexture);
    for (i = 0; i < model_manager.modelCount; i++)
    {
        model_free(&model_manager.modelList[i]);
    }
    free(model_manager.modelList);
    gf3d_pipeline_free(model_manager.pipe);
    memset(&model_manager, 0, sizeof(ModelManager));
}

Pipeline* model_get_pipeline()
{
    return model_manager.pipe;
}

ModelUBO model_get_ubo(GFC_Matrix4 modelMat, GFC_Color colorMod)
{
    ModelUBO ubo = {0};
    GFC_Matrix4* view;
    gfc_matrix4_copy(ubo.model, modelMat);
    view = gf3d_vgraphics_get_view_matrix();
    if (view) gfc_matrix4_copy(ubo.view, *view);
    gf3d_vgraphics_get_projection_matrix(&ubo.proj);
    ubo.color = gfc_color_to_vector4f(colorMod);
    return ubo;
}

Model* model_get_by_filename(const char* filename)
{
    if (!filename) return NULL;
    int i;
    for (i = 0; i < model_manager.modelCount; i++)
    {
        if (model_manager.modelList[i]._refCount == 0) continue;
        if (gfc_strlcmp(model_manager.modelList[i].filename, filename) == 0)
        {
            return model_manager.modelList[i].filename;
        }
    }
    return NULL;
}

Model* model_load(const char* filename)
{
    const char* str = NULL;
    const char* str2 = NULL;
    Mesh* mesh;
    Texture* texture;
    SJson* json, *data;
    Model* model;
    if (!filename) return NULL;
    model = model_get_by_filename(filename);
    if (model)
    {
        model->_refCount++;
        return model;
    }

    json = sj_load(filename);
    if (!json)
    {
        slog("Failed to load model %s", filename);
        return NULL;
    }
    data = sj_object_get_value(json, "model");
    if (!data)
    {
        slog("Failed to get model information from file %s", filename);
        sj_free(json);
        return NULL;
    }

    str = sj_object_get_string(data, "obj");
    if (!str)
    {
        slog("Failed to find obj data in file %s", filename);
        sj_free(json);
        return NULL;
    }
    mesh = gf3d_mesh_load_obj(str);
    if (!mesh)
    {
        slog("Failed to parse obj data for model file %s", filename);
        sj_free(json);
        return NULL;
    }

    slog_sync();
    str2 = sj_object_get_string(json, "texture");
    if (str2)
    {
        texture = gf3d_texture_load(str2);
        if (!texture) texture = model_manager.defaultTexture;
    }
    else texture = model_manager.defaultTexture;

    model = model_new();
    if (!model)
    {
        slog("Failed to get an empty space in memory for model %s", filename);
        sj_free(json);
        return NULL;
    }
    model->mesh = mesh;
    model->texture = texture;
    gfc_line_cpy(model->filename, filename);

    sj_free(json);
    return model;
}

void model_queue_render(Model* model, GFC_Matrix4 mat, GFC_Color colorMod)
{
    ModelUBO ubo;
    if (!model) return;
    ubo = model_get_ubo(mat, colorMod);
    gf3d_mesh_queue_render(model->mesh, model_manager.pipe, (void *)(&ubo), model->texture);
}

/*eol@eof*/
