#ifndef __GF3D_MESH_H__
#define __GF3D_MESH_H__

#include <stdalign.h>
#include <vulkan/vulkan.h>

#include "gfc_vector.h"
#include "gfc_list.h"
#include "gfc_text.h"
#include "gfc_matrix.h"
#include "gfc_primitives.h"

#include "gf3d_pipeline.h"

//forward declaration:
typedef struct ObjData_S ObjData;

//absolute basics of the mesh information sent to the graphics card
typedef struct
{
    GFC_Matrix4     model;
    GFC_Matrix4     view;
    GFC_Matrix4     proj;
    GFC_Vector4D    color;
    GFC_Vector4D    camera;
}MeshUBO;

typedef struct
{
    GFC_Vector3D vertex;
    GFC_Vector3D normal;
    GFC_Vector2D texel;
}Vertex;

typedef struct
{
    Uint16  verts[3]; // change to Uint32 if it crashes for too many polygons
}Face;

typedef struct
{
    Uint32          vertexCount;
    VkBuffer        vertexBuffer;
    VkDeviceMemory  vertexBufferMemory;
    Uint32          faceCount;
    VkBuffer        faceBuffer;
    VkDeviceMemory  faceBufferMemory;
    ObjData        *objData;
}MeshPrimitive;

typedef struct
{
    GFC_TextLine        filename;
    Uint32              _refCount;
    GFC_List           *primitives;
    GFC_Box             bounds;
}Mesh;

/**
 * @brief initializes the mesh system / configures internal data about mesh based rendering
 * @param mesh_max the maximum allowed simultaneous meshes supported at once.  Must be > 0
 * @note keep in mind that many models will be comprised of multiple sub meshes.  So this number may need to be very large
 */
void gf3d_mesh_init(Uint32 mesh_max);

/**
 * @brief get a new empty model
 * @return NULL on error, or an empty model
 */
Mesh *gf3d_mesh_new();

/**
 * @brief load mesh data from an obj filename.
 * @note: currently only supporting obj files
 * @note this free's the intermediate data loaded from the obj file, no longer needed for most applications
 * @param filename the name of the file to load
 * @return NULL on error or Mesh data
 */
Mesh *gf3d_mesh_load_obj(const char *filename);

/**
 * @brief make an exact, but separate copy of the input mesh
 * @param in the mesh to duplicate
 * @return NULL on error, or a copy of in
 */
Mesh *gf3d_mesh_copy(Mesh *in);

/**
 * @brief move all of the vertices of the mesh by offset at the buffer level
 * @param in the mesh to move
 * @param offset how much to move it
 * @param rotation apply this rotation to the vertices and normals
 */
void gf3d_mesh_move_vertices(Mesh *in, GFC_Vector3D offset,GFC_Vector3D rotation);

/**
 * @brief allocate a zero initialized mesh primitive
 * @return NULL on error or the primitive
 */
MeshPrimitive *gf3d_mesh_primitive_new();


/**
 * @brief get the input attribute descriptions for mesh based rendering
 * @param count (optional, output) the number of attributes
 * @return a pointer to a vertex input attribute description array
 */
VkVertexInputAttributeDescription * gf3d_mesh_get_attribute_descriptions(Uint32 *count);

/**
 * @brief get the binding description for mesh based rendering
 * @return vertex input binding descriptions compatible with mesh data
 */
VkVertexInputBindingDescription * gf3d_mesh_get_bind_description();

/**
 * @brief free a mesh that has been loaded from memory
 */
void gf3d_mesh_free(Mesh *mesh);

/**
 * @brief queue up a render for the current draw frame
 * @param mesh the mesh to render
 * @param pipe the pipeline
 * @param uboData buffer data
 * @param texture the texture
 */
void gf3d_mesh_queue_render(Mesh* mesh, Pipeline* pipe, void* uboData, Texture* texture);

#endif
