#include "simple_logger.h"

#include "draw_3d.h"

typedef struct
{
	Model* cube;
	Model* sphere;
	//Model* cylinder;
} Draw3dData;

static Draw3dData draw_3d_data = { 0 };

void draw_3d_close()
{
	model_free(draw_3d_data.cube);
	model_free(draw_3d_data.sphere);
	//model_free(draw_3d_data.cylinder);
}

void draw_3d_init()
{
	draw_3d_data.cube = model_load("models/primitives/cube.model");
	draw_3d_data.sphere = model_load("models/primitives/sphere.model");
	//draw_3d_data.cylinder = model_load("models/primitives/cylinder.model");
	atexit(draw_3d_close);
}

void draw_3d_cube(GFC_Box cube, GFC_Color colorMod)
{
	GFC_Matrix4 mat;
	gfc_matrix4_from_vectors(
		mat,
		gfc_vector3d(cube.x + cube.w * 0.5, cube.y + cube.h * 0.5, cube.z + cube.d * 0.5),
		gfc_vector3d(0, 0, 0),
		gfc_vector3d(cube.w, cube.h, cube.d)
	);
	model_queue_render_highlight(draw_3d_data.cube, mat, colorMod);
}

void draw_3d_sphere(GFC_Sphere sphere, GFC_Color colorMod)
{

}

//void draw_3d_cylinder(GFC_Box cube, GFC_Color colorMod)
//{
//
//}