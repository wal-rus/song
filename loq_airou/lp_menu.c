
#include <math.h>
#include "st_matrix_operations.h"
#include "enough.h"
#include "seduce.h"
#include "deceive.h"

#include "lp_projection.h"
#include "lp_layer_groups.h"

extern void lp_brush_size_set(double size);
extern void lp_compute_int_color(float *color, uint i);
extern void la_compute_set_range(double start, double end);

void lp_init_slider(float *slider_vertex)
{
	slider_vertex[0] = 0;		/* The knob. */
	slider_vertex[1] = -0.01;
	slider_vertex[2] = -0.01;
	slider_vertex[3] = 0.01;

	slider_vertex[4] = 0;
	slider_vertex[5] = -0.01;
	slider_vertex[6] = 0.01;
	slider_vertex[7] = 0.01;

	slider_vertex[8] = -0.01;
	slider_vertex[9] = 0.01;
	slider_vertex[10] = 0.01;
	slider_vertex[11] = 0.01;

	slider_vertex[12] = -0.01;	/* The indicator line. */
	slider_vertex[13] = 0;
	slider_vertex[14] = -0.01;
	slider_vertex[15] = 0;

	slider_vertex[16] = 0.01;
	slider_vertex[17] = 0;
	slider_vertex[18] = 0.01;
	slider_vertex[19] = 0;
}

float lp_menu_slider(BInputState *input, float x, float y, float length, float pos, float pos_min, float pos_max,
		      float start_r, float start_g, float start_b, float end_r, float end_g, float end_b)
{
	float slider_vertex[10 * 2];
	float slider_color[10 * 3];
	float color[3], range = pos_max - pos_min;
	int i;

	lp_init_slider(slider_vertex);
	if(input->mode == BAM_DRAW)
	{
		color[0] = start_r * (1.0f - pos) + end_r * pos;
		color[1] = start_g * (1.0f - pos) + end_g * pos;
		color[2] = start_b * (1.0f - pos) + end_b * pos;
		for(i = 0; i < 30; i++)
			slider_color[i] = color[i % 3];
		slider_color[6 * 3 + 0] = start_r;
		slider_color[6 * 3 + 1] = start_g;
		slider_color[6 * 3 + 2] = start_b;
		slider_color[8 * 3 + 0] = end_r;
		slider_color[8 * 3 + 1] = end_g;
		slider_color[8 * 3 + 2] = end_b;
		slider_vertex[12] = -length * (pos - pos_min) / range - 0.01f;
		slider_vertex[16] = length * (pos_max - pos) / range + 0.01f;

		glPushMatrix();
		glTranslatef(x + (pos - pos_min) * length / range, y, 0.0f);
		sui_set_color_array_gl(slider_color, 10, 3);
		sui_draw_gl(GL_LINES, slider_vertex, 10, 2, 0, 0, 0, 1.0f);
		glPopMatrix();
	}
	else if(input->mouse_button[0])
	{
		if(input->click_pointer_x > x - 0.01 && input->click_pointer_x < x + 0.01 + length && input->click_pointer_y > y - 0.01 && input->click_pointer_y < y + 0.01)
		{
			pos = pos_min + (input->pointer_x - x) / length * range;
			if(pos > pos_max)
				pos = pos_max;
			if(pos < pos_min)
				pos = pos_min;
		}
	}
	return pos;
}

void lp_draw_pointer(BInputState *input, double brush_size)
{
	static float pointer[32 * 4];
	static boolean init = FALSE;
	if(!init)
	{
		uint i;
		for(i = 0; i < 32; i++)
		{
			pointer[i * 4] = sin((float)i * 2 * PI / 32.0);
			pointer[i * 4 + 1] = cos((float)i * 2 * PI / 32.0);
			pointer[i * 4 + 2] = sin((float)(i + 1) * 2 * PI / 32.0);
			pointer[i * 4 + 3] = cos((float)(i + 1) * 2 * PI / 32.0);
		}
		init = TRUE;
	}
	glPushMatrix();
	glTranslatef(input->pointer_x, input->pointer_y, 0.0f);
	glScaled(brush_size, brush_size, brush_size);
	sui_draw_gl(GL_LINES, pointer, 64, 2, 0, 0, 0, 1.0f);
	glPopMatrix();
}

static boolean type_is_integer(VNGLayerType type)
{
	return type == VN_G_LAYER_VERTEX_UINT32 || type == VN_G_LAYER_POLYGON_CORNER_UINT32 || type == VN_G_LAYER_POLYGON_FACE_UINT8 || type == VN_G_LAYER_POLYGON_FACE_UINT32;
}

void lp_menu(BInputState *input, ENode *node, double *slider, uint *integer)
{
	static double brush_size = 0.03;
	static double start_range = 0, end_range = 1;
	float position = 0, color[3] = { 0, 0, 0 };
	VNGLayerType type;

	if(node != NULL)
	{
		position = betray_get_screen_mode(NULL, NULL, NULL) - 0.05;
		if(input->mode == BAM_DRAW)
		{
			glPushMatrix();
			glTranslatef(0, 0, -1);
			glDisable(GL_DEPTH_TEST);
		}
		if(NULL != lp_layer_get_red(lp_layer_current_get()))
			type = e_nsg_get_layer_type(lp_layer_get_red(lp_layer_current_get()));
		else if(NULL != lp_layer_get_green(lp_layer_current_get()))
			type = e_nsg_get_layer_type(lp_layer_get_green(lp_layer_current_get()));
		else
			type = e_nsg_get_layer_type(lp_layer_get_blue(lp_layer_current_get()));
		if(lp_layer_get_green(lp_layer_current_get()) != NULL ||
		   lp_layer_get_blue(lp_layer_current_get()) != NULL)
		{
			sui_draw_text(0.55, position - 0.02, SUI_T_SIZE, SUI_T_SPACE, "Color:", 0, 0, 0, 1.0);
			if(NULL != lp_layer_get_red(lp_layer_current_get()))
				slider[0] = lp_menu_slider(input, 0.55, position - 0.05, 0.4, slider[0], 0.0f, 1.0f,  0, slider[1], slider[2], 1, slider[1], slider[2]);
			else
				lp_menu_slider(input, 0.55, position - 0.05, 0.4, 0.5, 0.0f, 1.0f,  0.3, 0.3, 0.3, 0.3, 0.3, 0.3);
			if(NULL != lp_layer_get_green(lp_layer_current_get()))
				slider[1] = lp_menu_slider(input, 0.55, position - 0.1, 0.4, slider[1], 0.0f, 1.0f,  slider[0], 0, slider[2], slider[0], 1, slider[2]);
			else
				lp_menu_slider(input, 0.55, position - 0.1, 0.4, 0.5, 0.0f, 1.0f,  0.3, 0.3, 0.3, 0.3, 0.3, 0.3);
			if(NULL != lp_layer_get_blue(lp_layer_current_get()))
				slider[2] = lp_menu_slider(input, 0.55, position - 0.15, 0.4, slider[2], 0.0f, 1.0f,  slider[0], slider[1], 0, slider[0], slider[1], 1);
			else
				lp_menu_slider(input, 0.55, position - 0.15, 0.4, 0.5, 0.0f, 1.0f, 0.3, 0.3, 0.3, 0.3, 0.3, 0.3);
			position -= 0.2;
		}
		else
		{
			if(type_is_integer(type))
			{
				sui_draw_text(0.55, position - 0.02, SUI_T_SIZE, SUI_T_SPACE, "Integer:", 0, 0, 0, 1.0);
				lp_compute_int_color(color, *integer);
				sui_type_number_uint(input, 0.55, position - 0.05, 0.4, 0.5f, SUI_T_SIZE, (uint32 *) integer, integer, color[0], color[1], color[2]);
				position -= 0.1;
			}
			else
			{
				sui_draw_text(0.55, position - 0.02, SUI_T_SIZE, SUI_T_SPACE, "Displacement:", 0, 0, 0, 1.0);
				slider[0] = lp_menu_slider(input, 0.5, position - 0.05, 0.4, slider[0], -1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
				sui_type_number_double(input, 0.55, position - 0.1, 0.4, 0.5f, SUI_T_SIZE, &slider[0], &slider[0], color[0], color[1], color[2], 1.0);
				glPopMatrix();
				position -= 0.15;
			}		
		}
		if(!type_is_integer(type))
		{				
			sui_draw_text(0.55, position - 0.02, SUI_T_SIZE, SUI_T_SPACE, "Tone Range:", 0, 0, 0, 1.0);
			sui_type_number_double(input, 0.55, position - 0.05, 0.4, 0.5f, SUI_T_SIZE, &start_range, &start_range, 0, 0, 0, 1.0);
			sui_type_number_double(input, 0.75, position - 0.05, 0.4, 0.5f, SUI_T_SIZE, &end_range, &end_range, 0, 0, 0, 1.0);
			position -= 0.1;
		}

		sui_draw_text(0.55, position - 0.02, SUI_T_SIZE, SUI_T_SPACE, "Brush Size:", 0, 0, 0, 1.0);
		brush_size = lp_menu_slider(input, 0.55, position - 0.05, 0.4, brush_size / 0.2, 0.0f, 1.0f,  0, 0, 0, 1, 1, 1) * 0.2;
		lp_brush_size_set(brush_size);	
		position -= 0.1;

		if(input->mode == BAM_DRAW)
		{
			lp_draw_pointer(input, brush_size);
			glPopMatrix();
			glEnable(GL_DEPTH_TEST);
		}
	}
}
