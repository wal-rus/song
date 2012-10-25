#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "verse.h"
#include "enough.h"
#include "seduce.h"
#include "persuade.h"
#include "deceive.h"

#include "co_vn_graphics.h"
#include "co_vn_handle.h"
#include "co_widgets.h"
#include "co_projection.h"

void draw_settings_test(BInputState *input);

static float view_cam_pos[3] = {0, 0, 4};
static float view_cam_pos_goal[3] = {0, 0, 4};
static float view_cam_move_start[2];
static float view_cam_speed = 0.1;
extern float co_background_color[3];
extern float co_line_color[3];

void co_handle_view(BInputState *input, float *pos, boolean active)
{
	if(active && input->mouse_button[1])
	{
		if(input->mouse_button[0])
		{
			view_cam_pos_goal[2] *= 1 + 2 * ((input->pointer_y - view_cam_move_start[1]) + (input->pointer_x - view_cam_move_start[0]));
			if(view_cam_pos_goal[2] < 0.5)
				view_cam_pos_goal[2] = 0.5;
			view_cam_speed = 0.1;
		}else
		{
			view_cam_pos_goal[0] += (input->pointer_x - view_cam_move_start[0]) * view_cam_pos[2];
			view_cam_pos_goal[1] += (input->pointer_y - view_cam_move_start[1]) * view_cam_pos[2];
			view_cam_speed = 0.3;
		}
	}else if(active && input->mouse_button[0])
	if(0.025 * 0.025 < (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y) && input->mode == BAM_EVENT)
	{
		if(input->click_pointer_x > 0.9 || input->click_pointer_x < -0.9)
		{
			view_cam_pos_goal[2] *= 1 + 2 * ((input->pointer_y - view_cam_move_start[1]) + (input->pointer_x - view_cam_move_start[0]));
			if(view_cam_pos_goal[2] < 0.5)
				view_cam_pos_goal[2] = 0.5;
			view_cam_speed = 0.1;
		}else
		{
			view_cam_pos_goal[0] += (input->pointer_x - view_cam_move_start[0]) * view_cam_pos[2];
			view_cam_pos_goal[1] += (input->pointer_y - view_cam_move_start[1]) * view_cam_pos[2];
			view_cam_speed = 0.3;
		}
	}	
	view_cam_move_start[0] = input->pointer_x;
	view_cam_move_start[1] = input->pointer_y;
	pos[0] = view_cam_pos[0] = (view_cam_pos_goal[0] * view_cam_speed) + (view_cam_pos[0] * (1.0 - view_cam_speed));
	pos[1] = view_cam_pos[1] = (view_cam_pos_goal[1] * view_cam_speed) + (view_cam_pos[1] * (1.0 - view_cam_speed));
	pos[2] = view_cam_pos[2] = (view_cam_pos_goal[2] * view_cam_speed) + (view_cam_pos[2] * (1.0 - view_cam_speed));
} 

float co_get_pos_x(float x)
{
	return x * view_cam_pos[2] - view_cam_pos[0];
}

float co_get_pos_y(float y)
{
	return y * view_cam_pos[2] - view_cam_pos[1];
}

float co_get_view_x(float x)
{
	return (x + view_cam_pos[0]) / view_cam_pos[2];
}

float co_get_view_y(float y)
{
	return (y + view_cam_pos[1]) / view_cam_pos[2];
}

COVerseNode *create_verse_node(ENode *node)
{
	COVerseNode *co_node;
	co_node = malloc(sizeof *co_node);
	co_node->node_id = e_ns_get_node_id(node);
	co_node->pos_x = co_get_pos_x(((float)e_ns_get_node_type(node) - ((float)(V_NT_NUM_TYPES) / 2) + 0.5) / V_NT_NUM_TYPES * 1.8);
	co_node->pos_y = co_get_pos_y(0);
	co_node->hidden = e_ns_get_node_owner(node) != VN_OWNER_MINE;
	co_node->search = TRUE;
	co_node->viewlock = FALSE;
	co_node->persuade = NULL;
	co_node->render_cash = NULL;
	e_ns_set_custom_data(node, CONNECTOR_ENOUGH_SLOT, co_node);
	return co_node;
}
void co_unhide_recursevly(uint node_id, float x, float y)
{
	COVerseNode *co_node;
	EObjLink *link;
	ENode *node;
	float dist = 0;
	for(node = e_ns_get_node_next(0, 0, V_NT_OBJECT); node != 0; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, V_NT_OBJECT))
	{
		co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
		if(co_node->hidden)
		{
			for(link = e_nso_get_next_link(node, 0); link != NULL; link = e_nso_get_next_link(node, e_nso_get_link_id(link) + 1))
			{
				if(node_id == e_nso_get_link_node(link))
				{
					co_node->hidden = FALSE;
					co_node->pos_x = x - 1.8;
					co_node->pos_y = y + dist;
					dist -= 1.0;
				}
			}
		}
	}
}


void verse_node_create_func(ENode *node, ECustomDataCommand command)
{
	if(command == E_CDC_CREATE)
		create_verse_node(node);
	if(command == E_CDC_DESTROY)
		free(e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT));
}


void co_draw_bitmap(ENode *node);
void co_geometry_destroy(void *g);
void *co_geometry_draw(ENode *node, void *g, boolean fill, boolean scale, float red, float green, float blue, float alpha);

void p_render_object(ENode *node, boolean transparency);

void co_search_clear(void);
void co_search_update(char *search);


void co_node_draw(ENode *node, VNodeType type, boolean hidden)
{
	double time;
	uint32 seconds, fractions;
	betray_get_current_time(&seconds, &fractions);

	time = ((double)fractions / (double)0xffffffff + (double)(seconds % 4)) / 4.0;

	glDisable(GL_DEPTH_TEST);
	co_vng_ring();
	if(!hidden)
	{
		switch(type)
		{
			case V_NT_OBJECT :
				co_vng_sun();
				co_vng_sunrays();
//				co_vng_color(245.0 / 255.0, 243.0 / 255.0, 170.0 / 255.0);
#ifdef PERSUADE_H
				if(node != NULL && FALSE)	
				{
					ENode *g_node;
					EObjLink *link;
					double matrix[16];
					float col_one[4] = {0.5, 0.4, 0.3, 1}, pos_one[4] = {0.4, 0.6, -0.6, 1};
					float col_two[4] = {0.6, 0.8, 1, 1}, pos_two[4] = {-0.4, 0.6, -0.6, 1};
					float col_three[4] = {0.7, 0.7, 0.7, 1}, pos_three[4] = {0.4, 0.6, 0.6, 1};						
					egreal center[3], size;
					float pos[4] = {0, 0, 0, 1};
					glGetDoublev(GL_PROJECTION_MATRIX, matrix);
					for(link = e_nso_get_next_link(node, 0); link != NULL; link = e_nso_get_next_link(node, e_nso_get_link_id(link) + 1))
					{
						if((g_node = e_ns_get_node(0, e_nso_get_link_node(link))) != NULL && e_ns_get_node_type(g_node) == V_NT_GEOMETRY)
						{
							e_nsg_get_center(g_node, center);
							size = 0.7 / e_nsg_get_size(g_node);
						}
					}

					pos[0] = matrix[0] * pos_one[0] + matrix[4] * pos_one[1] + matrix[8] * pos_one[2] + matrix[12];
					pos[1] = matrix[1] * pos_one[0] + matrix[5] * pos_one[1] + matrix[9] * pos_one[2] + matrix[13];
					pos[2] = matrix[2] * pos_one[0] + matrix[6] * pos_one[1] + matrix[10] * pos_one[2] + matrix[14];
					glEnable(GL_LIGHT0);
					glLightfv(GL_LIGHT0, GL_POSITION, pos);
					glLightfv(GL_LIGHT0, GL_DIFFUSE, col_one);
					glEnable(GL_LIGHT1);
					pos[0] = matrix[0] * pos_two[0] + matrix[4] * pos_two[1] + matrix[8] * pos_two[2] + matrix[12];
					pos[1] = matrix[1] * pos_two[0] + matrix[5] * pos_two[1] + matrix[9] * pos_two[2] + matrix[13];
					pos[2] = matrix[2] * pos_two[0] + matrix[6] * pos_two[1] + matrix[10] * pos_two[2] + matrix[14];
					glLightfv(GL_LIGHT1, GL_POSITION, pos);
					glLightfv(GL_LIGHT1, GL_DIFFUSE, col_two);
					glEnable(GL_LIGHT2);
					pos[0] = matrix[0] * pos_three[0] + matrix[4] * pos_three[1] + matrix[8] * pos_three[2] + matrix[12];
					pos[1] = matrix[1] * pos_three[0] + matrix[5] * pos_three[1] + matrix[9] * pos_three[2] + matrix[13];
					pos[2] = matrix[2] * pos_three[0] + matrix[6] * pos_three[1] + matrix[10] * pos_three[2] + matrix[14];
					glLightfv(GL_LIGHT2, GL_POSITION, pos);
					glLightfv(GL_LIGHT2, GL_DIFFUSE, col_three);
					glEnableClientState(GL_NORMAL_ARRAY);
					glEnable(GL_DEPTH_TEST);
					glPushMatrix();
					glEnable(GL_NORMALIZE);
					glEnable(GL_LIGHTING);
					glScalef(size, size, size);
					glRotatef(time * 360, 0, 1, 0);
					glTranslatef(-center[0], -center[1], -center[2]);
					p_render_object(node, FALSE);
					glEnable(GL_BLEND);
					p_render_object(node, TRUE);
					glDisable(GL_BLEND);
					glDisable(GL_NORMALIZE);
					glPopMatrix();
					glDisable(GL_DEPTH_TEST);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisable(GL_LIGHT0);
					glDisable(GL_LIGHT1);
					glDisable(GL_LIGHT2);
				}
#endif
			break;
			case V_NT_GEOMETRY :
				co_vng_saturn_ring();
				co_vng_gas_planet();
				if(node != NULL)
				{
					COVerseNode *co_node;
					glPushMatrix();
					glRotatef(time * 360, 0, 1, 0);
					co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
					co_node->render_cash = co_geometry_draw(node, co_node->render_cash, FALSE, TRUE, co_line_color[0], co_line_color[1], co_line_color[2], 0.7);
					glPopMatrix();
				}
			break;
			case V_NT_MATERIAL :
				if(node != NULL)
					co_draw_material(node);
			break;
			case V_NT_BITMAP :
				if(node != NULL)
				{
					glPushMatrix();
					glScalef(0.5, 0.5, 0.5);
					co_draw_bitmap(node);
					glPopMatrix();
				}
				co_vng_dust_planet();
			break;
			case V_NT_TEXT :
				co_vng_geo_planet();
			break;
			case V_NT_CURVE :
				co_vng_dust_planet();
			break;
			default:
			break;
		}
	}
}

void co_center(float x, float y)
{
	view_cam_pos[0] = x;
	view_cam_pos[1] = y;
	view_cam_pos[2] = 4;
	view_cam_speed = 0.9;
}

extern boolean co_serch_node(ENode *node, char *search);

typedef enum{
	COIM_NONE,
	COIM_MOVE,
	COIM_VIEW,
	COIM_OPEN,
	COIM_POPUP,
	COIM_CREATE,
	COIM_LINK
}COInteractMode;

extern boolean co_handle_head(BInputState *input, ENode *node, float *length);
extern void co_material_compute(uint lines);

boolean get_stop_sign(void);

extern void co_init_game(uint count);
extern void co_end_game(void);
extern void co_create_pebble(uint type, float pos_x, float pos_y, float scale);
extern boolean co_is_game_active(void);
extern void co_play_game(BInputState *input);
extern boolean co_search_node(ENode *node, char *search);

extern boolean co_draw_ships(BInputState *input);

extern void co_draw_3d_view(float x, float y);
extern boolean co_draw_3d_click_test_node(float x, float y);
extern void co_draw_3d_summary(void);
extern void co_draw_3d_node_lock(ENode *node);
extern void co_draw_3d_icon_pass(void);
extern void co_pu_empty(BInputState *input);
extern void update_bitmap_load(char *name);
extern void co_clone_node(ENode *node);
extern void co_clone_node_update(void);
extern void co_draw_settings(BInputState *input);
extern boolean co_get_settings(void);
extern float co_background_color[3];

static const char * compute_link_label(ENode *from, ENode *to)
{
	VNodeType	type = e_ns_get_node_type(to);

	if(type == V_NT_OBJECT)
		return "child";
	else if(type == V_NT_GEOMETRY)
		return "geometry";
	else if(type == V_NT_MATERIAL)
		return "material";
	return "link";
}

void co_input_handler(BInputState *input, void *user_pointer)
{
	static COInteractMode mode = COIM_NONE;
	static VNodeID active = -1;
	static VNodeType link_id = -1;
	static float clear_color = 1, create_move = 0, popup_move = 0, type_count, create_scroll = 0;
	static char search[64];
	static int search_cursor = 0;

//	char *names[7] = {"OBJECT", "GEOMETRY", "MATERIAL", "BITMAP", "TEXT", "PARTICLE", "CURVE"};
	float create_length;
	ENode *node;
	COVerseNode *co_node;
	VNodeType type;
	float pos[3];

//	update_bitmap_load("head3.tga");
	if(input->mode == BAM_MAIN)
	{
		verse_callback_update(3000);
		co_material_compute(1);
		co_vng_update_time();
		co_clone_node_update();
#ifdef PERSUADE_H
		p_task_compute(1);
#endif
		return;
	}

	if(input->mode == BAM_DRAW)
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		glClearColor(co_background_color[0], co_background_color[1], co_background_color[2], 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glPushMatrix();
		p_view_set();
		co_draw_3d_view(input->pointer_x, input->pointer_y);
		glPopMatrix();
		if(mode == COIM_NONE)
			co_draw_3d_summary();
		co_draw_3d_icon_pass();
	}else if(co_get_settings())
	{
		co_draw_settings(input);
		return;
	}

	if(co_is_game_active())
	{
		betray_end_type_in_mode(TRUE);
		co_play_game(input);
		return;
	}else if(co_draw_ships(input))
	{
		uint i = 0;
		for(type = 0; type < V_NT_NUM_TYPES; type++)
			for(node = e_ns_get_node_next(0, 0, type); node != 0; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, type))
				if((co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT)) != NULL && co_node->hidden != TRUE)
					i++;
		co_init_game(i);
		for(type = 0; type < V_NT_NUM_TYPES; type++)
			for(node = e_ns_get_node_next(0, 0, type); node != 0; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, type))
				if((co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT)) != NULL && co_node->hidden != TRUE)
					co_create_pebble(type, co_get_view_x(co_node->pos_x), co_get_view_y(co_node->pos_y), 1 / view_cam_pos[2]);
		view_cam_pos[2] = 0;
		view_cam_speed = 0.01;
	}

	co_handle_view(input, pos, mode == COIM_NONE);
	

	if(input->mode == BAM_DRAW)
	{
		glPushMatrix();

		if((node = e_ns_get_node(0, active)) != NULL && (co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT)) != NULL)
			glTranslatef(pos[0] * (1 - popup_move) - popup_move * (co_node->pos_x + 1), pos[1] * (1 - popup_move) -  popup_move * co_node->pos_y, -pos[2] * (1 - popup_move) - popup_move);
		else
			glTranslatef(pos[0], pos[1], -pos[2]);
	}

	co_search_update(search);

	if(input->mode != BAM_DRAW && mode == COIM_NONE && input->last_mouse_button[1] == FALSE && input->mouse_button[1] == TRUE)
	{
		p_view_change_start(input);
		mode = COIM_VIEW;
	}

	create_length = create_scroll - 1;

	node = deceive_get_edit_node();
	if(node != NULL)
	{
		mode = COIM_OPEN;
		active = e_ns_get_node_id(node);
		co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
		co_node->hidden = FALSE;
		co_node->viewlock = FALSE;
		co_node->pos_x = co_get_pos_x(0);
		co_node->pos_y = co_get_pos_y(0);
	}

	for(type = 0; type < V_NT_NUM_TYPES; type++)
	{
		type_count = create_scroll - 1;

		for(node = e_ns_get_node_next(0, 0, type); node != 0; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, type))
		{
			if(mode != COIM_CREATE || co_search_node(node, search))
			{
				type_count++;
				co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
				if(co_node != NULL && co_node->hidden != TRUE && co_node->search)
				{
					if(input->mode == BAM_DRAW)
					{
						glPushMatrix();
						glTranslatef(co_get_pos_x(((float)type - ((float)(V_NT_NUM_TYPES) / 2) + 0.5) / V_NT_NUM_TYPES * 1.8) * create_move + co_node->pos_x * (1 - create_move),
						co_get_pos_y((float)type_count * -0.2) * create_move + co_node->pos_y * (1 - create_move), 0);
						glScalef(view_cam_pos[2] / 5 * create_move + (1 - create_move), view_cam_pos[2] / 5 * create_move + (1 - create_move), view_cam_pos[2] / 5 * create_move + (1 - create_move));
						if(mode == COIM_CREATE)
						{
							glPushMatrix();
							glScalef(4, 4, 4);
							sui_draw_text(sui_compute_text_length(SUI_T_SIZE, SUI_T_SPACE, e_ns_get_node_name(node)) * -0.5, 0, SUI_T_SIZE, SUI_T_SPACE, e_ns_get_node_name(node), co_line_color[0], co_line_color[1], co_line_color[2], 1);

							glPopMatrix();
						}
						co_node_draw(node, type, FALSE);
					}else
					{
						if(mode == COIM_NONE)
						{
							if(input->last_mouse_button[0] == FALSE && input->mouse_button[0] == TRUE)
							{
								if((co_get_pos_x(input->pointer_x) - co_node->pos_x) * (co_get_pos_x(input->pointer_x) - co_node->pos_x) + (co_get_pos_y(input->pointer_y) - co_node->pos_y) * (co_get_pos_y(input->pointer_y) - co_node->pos_y) < 0.4 * 0.4)
								{
									active = e_ns_get_node_id(node);
									co_node->viewlock = FALSE;
									mode = COIM_MOVE;
								}
							}
							if(input->last_mouse_button[2] == FALSE && input->mouse_button[2] == TRUE)
							{
								if((co_get_pos_x(input->pointer_x) - co_node->pos_x) * (co_get_pos_x(input->pointer_x) - co_node->pos_x) + (co_get_pos_y(input->pointer_y) - co_node->pos_y) * (co_get_pos_y(input->pointer_y) - co_node->pos_y) < 0.4 * 0.4)
								{
									active = e_ns_get_node_id(node);
									mode = COIM_OPEN;
								}
							}
							if(input->last_mouse_button[1] == FALSE && input->mouse_button[1] == TRUE)
							{
							/*	if((co_get_pos_x(input->pointer_x) - co_node->pos_x) * (co_get_pos_x(input->pointer_x) - co_node->pos_x) + (co_get_pos_y(input->pointer_y) - co_node->pos_y) * (co_get_pos_y(input->pointer_y) - co_node->pos_y) < 0.4 * 0.4)
								{
									co_center(co_node->pos_x, co_node->pos_y);
								}*/
								p_view_change_start(input);
								mode = COIM_VIEW;

							}
						}
						if(mode == COIM_MOVE && active == e_ns_get_node_id(node))
						{
							if(0.025 * 0.025 < (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y))
							{
								co_node->pos_x = co_get_pos_x(input->pointer_x);
								co_node->pos_y = co_get_pos_y(input->pointer_y);
							}
							if(input->last_mouse_button[0] == TRUE && input->mouse_button[0] == FALSE)
							{
								mode = COIM_NONE;
								if(0.025 * 0.025 > (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y))
									mode = COIM_OPEN;
								else
									co_draw_3d_node_lock(node);
							}
						}
					}
					if(type == V_NT_OBJECT && mode != COIM_CREATE && mode != COIM_OPEN)
					{
						uint i = 0;
						EObjLink *link;
						for(link = e_nso_get_next_link(node, 0); link != NULL; link = e_nso_get_next_link(node, e_nso_get_link_id(link) + 1))
						{
							COVerseNode *co_link = NULL;
							ENode *link_node;
							if((link_node = e_ns_get_node(0, e_nso_get_link_node(link))) != NULL)
								co_link = e_ns_get_custom_data(link_node, CONNECTOR_ENOUGH_SLOT);

							if(co_link != NULL && co_link->hidden == TRUE)
							{
								co_link->hidden = FALSE;
								co_link->pos_x = co_node->pos_x + 2;
								co_link->pos_y = co_node->pos_y + 2 - i;
							}

							if((co_link != NULL && co_vng_render_link(input, i, TRUE, -co_node->pos_x + co_get_pos_x(input->pointer_x), -co_node->pos_y + co_get_pos_y(input->pointer_y), -co_node->pos_x + co_link->pos_x, -co_node->pos_y + co_link->pos_y, e_nso_get_link_name(link)))
							||	(co_link == NULL && co_vng_render_link(input, i, FALSE, -co_node->pos_x + co_get_pos_x(input->pointer_x), -co_node->pos_y + co_get_pos_y(input->pointer_y), 0, 0, e_nso_get_link_name(link))))
							{
								mode = COIM_LINK;
								link_id = e_nso_get_link_id(link);
								active = e_ns_get_node_id(node);
							}
							i++;
						}
						if(co_vng_render_link(input, i, FALSE, -co_node->pos_x + co_get_pos_x(input->pointer_x), -co_node->pos_y + co_get_pos_y(input->pointer_y), 0, 0, "New"))
						{
							mode = COIM_LINK;
							link_id = -1;
							active = e_ns_get_node_id(node);
						}
					}
					if(type == V_NT_MATERIAL && mode != COIM_CREATE && mode != COIM_OPEN)
					{
						VNMFragmentID i;
						uint j = 0;
						VMatFrag *frag;

						for(i = e_nsm_get_fragment_next(node, 0); i != (VNMFragmentID)-1; i = e_nsm_get_fragment_next(node, i + 1))
						{
							if(VN_M_FT_TEXTURE == e_nsm_get_fragment_type(node, i))
							{
								COVerseNode *co_link = NULL;
								ENode *link_node;
								frag = e_nsm_get_fragment(node, i);
								if((link_node = e_ns_get_node(0, frag->texture.bitmap)) != NULL)
									co_link = e_ns_get_custom_data(link_node, CONNECTOR_ENOUGH_SLOT);

								if(co_link != NULL && co_link->hidden == TRUE)
								{
									co_link->hidden = FALSE;
									co_link->pos_x = co_node->pos_x + 2;
									co_link->pos_y = co_node->pos_y + 2 - i;
								}

								if((co_link != NULL && co_vng_render_link(input, j, TRUE, -co_node->pos_x + co_get_pos_x(input->pointer_x), -co_node->pos_y + co_get_pos_y(input->pointer_y), -co_node->pos_x + co_link->pos_x, -co_node->pos_y + co_link->pos_y, "Texture"))
								||	(co_link == NULL && co_vng_render_link(input, j, FALSE, -co_node->pos_x + co_get_pos_x(input->pointer_x), -co_node->pos_y + co_get_pos_y(input->pointer_y), 0, 0, "Texture")))
								{
									mode = COIM_LINK;
									link_id = i;
									active = e_ns_get_node_id(node);
								}
								j++;
							}
						}
					}
					if(mode != COIM_OPEN)
					{
						switch(co_vng_render_name(input, e_ns_get_node_name(node), co_node->pos_x, co_node->pos_y, co_get_pos_x(input->pointer_x), co_get_pos_y(input->pointer_y)))
						{
							case 1:
								verse_send_node_destroy(e_ns_get_node_id(node));
								mode = COIM_VIEW;
							break;
							case 2:
								co_node->hidden = TRUE;
								mode = COIM_VIEW;
							break;
							case 3:
							{
/*								uint i;
								ENode *link;
								for(i = 0; i < V_NT_NUM_TYPES; i++)
								{
									e_nso_get_link(node, i);
									link = e_ns_get_node(0, e_nso_get_link(node, i));
									if(link != NULL && (co_node = e_ns_get_custom_data(link, CONNECTOR_ENOUGH_SLOT)) != NULL)
										co_node->hidden = TRUE;
								}*/
								mode = COIM_VIEW;
							}
							break;
							case 4:
								co_clone_node(node);
								mode = COIM_VIEW;
							break;
						}
					}
					if(input->mode == BAM_DRAW)
						glPopMatrix();
				}else
				{
					if(input->mode == BAM_DRAW && mode == COIM_CREATE)
					{
						float y;
						y = /*co_get_pos_y*/((float)type_count * -0.2);
						if(y < 2 && y > -2)
						{
							glPushMatrix();
							glTranslatef(co_get_pos_x(((float)type - ((float)(V_NT_NUM_TYPES) / 2) + 0.5) / V_NT_NUM_TYPES * 1.8),
								co_get_pos_y((float)type_count * -0.2), 0);
							glScalef(view_cam_pos[2] / 5 * create_move + (1 - create_move), view_cam_pos[2] / 5 * create_move + (1 - create_move), view_cam_pos[2] / 5 * create_move + (1 - create_move));
							co_node_draw(node, type, FALSE);
							glScalef(4, 4, 4);
							{
								char text[46];
								sprintf(text, "id = %u", e_ns_get_node_id(node));
								sui_draw_text(sui_compute_text_length(SUI_T_SIZE, SUI_T_SPACE, text) * -0.5, -0.04, SUI_T_SIZE, SUI_T_SPACE, text, co_line_color[0], co_line_color[1], co_line_color[2], 1);
							}
							sui_draw_text(sui_compute_text_length(SUI_T_SIZE, SUI_T_SPACE, e_ns_get_node_name(node)) * -0.5, 0, SUI_T_SIZE, SUI_T_SPACE, e_ns_get_node_name(node), co_line_color[0], co_line_color[1], co_line_color[2], 0.5);
							glPopMatrix();
						}
					}
				}
			}
		}
		if(create_length < type_count)
			create_length = type_count;
	}
	create_length -= create_scroll - 1;
	create_length *= 0.5;

	if(mode == COIM_CREATE)
	{
		clear_color = clear_color - 0.001;
//		create_move = create_move * 0.95 + 0.05;
		create_move += betray_get_delta_time() / 0.4;
		if(create_move > 1.0)
			create_move = 1.0;

	}else
	{
//		create_move = create_move * 0.95;
		create_move -= betray_get_delta_time() / 0.4;
		if(create_move < 0.0)
			create_move = 0.0;
	}
	if(mode == COIM_CREATE)
	{
		float x, y;

		y = (1 - create_scroll) / 5;
		for(type = 0; type < V_NT_NUM_TYPES; type++)
		{
			x = ((float)type - ((float)(V_NT_NUM_TYPES) / 2) + 0.5) / V_NT_NUM_TYPES * 1.8;
			if(input->mode == BAM_DRAW)
			{
				char *names[8] = {"OBJECT", "GEOMETRY", "MATERIAL", "BITMAP", "TEXT", "CURVE", "AUDIO"};
				glPushMatrix();
				glTranslatef(co_get_pos_x(x), co_get_pos_y(y), 0);
				glScalef(view_cam_pos[2], view_cam_pos[2] , view_cam_pos[2]);
				sui_draw_text(sui_compute_text_length(SUI_T_SIZE, SUI_T_SPACE, "NEW") * -0.5, 0, SUI_T_SIZE, SUI_T_SPACE, "NEW", co_line_color[0], co_line_color[1], co_line_color[2], 1);
				sui_draw_text(sui_compute_text_length(SUI_T_SIZE, SUI_T_SPACE, names[type]) * -0.5, 0.15, SUI_T_SIZE, SUI_T_SPACE, names[type], co_line_color[0], co_line_color[1], co_line_color[2], 1);
				glScalef(0.2, 0.2, 0.2);
				co_node_draw(NULL, type, FALSE);
				glPopMatrix();
			}else
			{
				if(input->mouse_button[0] == FALSE && input->last_mouse_button[0] == TRUE)
				{
					if((input->pointer_x - x) * (input->pointer_x - x) + (input->pointer_y - y) * (input->pointer_y - y) < 0.2 * 0.2 * 0.4 * 0.4)
					{
						verse_send_node_create(-1, type, VN_OWNER_MINE);
						mode = COIM_NONE;
						search[0] = 0;
					}
				}
			}
		}
			
		if(input->mode != BAM_DRAW)
		{
			if(input->last_mouse_button[0] == TRUE)
				if(input->click_pointer_x < 0.95 && 0.05 * 0.05 < (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y))
					create_scroll -= input->delta_pointer_y * 5;
			if(0.05 * 0.05 > (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y) && ((input->mouse_button[0] == FALSE && input->last_mouse_button[0] == TRUE) || (input->mouse_button[2] == FALSE && input->last_mouse_button[2] == TRUE)))
			{
				for(type = 0; type < V_NT_NUM_TYPES; type++)
				{
					type_count = create_scroll;
					for(node = e_ns_get_node_next(0, 0, type); node != 0; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, type))
					{
						co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
						if(co_node != NULL && co_search_node(node, search))
						{
							x = ((float)type - ((float)(V_NT_NUM_TYPES) / 2) + 0.5) / V_NT_NUM_TYPES * 1.8;
							if((input->pointer_x - x) * (input->pointer_x - x) + (input->pointer_y - (type_count * -0.2)) * (input->pointer_y - (type_count * -0.2)) < 0.2 * 0.2 * 0.4 * 0.4)
							{
								
								co_node->hidden = FALSE;
								co_node->viewlock = FALSE;
								co_node->pos_x = co_get_pos_x(x);
								co_node->pos_y = co_get_pos_y(type_count * -0.2);
								co_unhide_recursevly(e_ns_get_node_id(node), co_node->pos_x, co_node->pos_y);
								
							}
							type_count++;
						}
					}
				}
				if(input->last_mouse_button[0] == TRUE && input->click_pointer_x < 0.95)
				{
					betray_end_type_in_mode(TRUE);
					mode = COIM_NONE;
					search[0] = 0;
					
				}
			}
		}
	}
	else if(input->mode != BAM_DRAW)
	{
		if(mode == COIM_NONE)
		{
			if(input->mouse_button[2] == TRUE && input->last_mouse_button[2] == FALSE)
				mode = COIM_POPUP;
			if(input->mouse_button[0] == FALSE && input->last_mouse_button[0] == TRUE)
				if(0.025 * 0.025 > (input->pointer_x - input->click_pointer_x) * (input->pointer_x - input->click_pointer_x) + (input->pointer_y - input->click_pointer_y) * (input->pointer_y - input->click_pointer_y))
					if(!co_draw_3d_click_test_node(input->pointer_x, input->pointer_y))
						mode = COIM_CREATE;
		}
		if(mode == COIM_CREATE)
			create_scroll = -5 * betray_get_screen_mode(NULL, NULL, NULL) + 2;
	}

	

	if(mode == COIM_POPUP)
	{
		co_pu_empty(input);
		if(input->mouse_button[2] == FALSE && input->last_mouse_button[2] == FALSE)
			mode = COIM_NONE;
	}

	if(mode == COIM_LINK)
	{
		if(input->mouse_button[0] == FALSE && input->last_mouse_button[0] == TRUE)
		{
			uint i;
			EObjLink *link;
			ENode *linker;
			uint32 found = ~0u;
			for(i = 0; found == (uint32) -1 && i < V_NT_NUM_TYPES; i++)
			{
				for(node = e_ns_get_node_next(0, 0, i); node != NULL; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, i))
				{
					co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
					if(co_node != NULL && co_node->hidden != TRUE && ((co_get_pos_x(input->pointer_x) - co_node->pos_x) * (co_get_pos_x(input->pointer_x) - co_node->pos_x) + (co_get_pos_y(input->pointer_y) - co_node->pos_y) * (co_get_pos_y(input->pointer_y) - co_node->pos_y) < 0.4 * 0.4))
					{
						found = co_node->node_id;
						break;
					}
				}	
			}

			linker = e_ns_get_node(0, active);
			if(linker != NULL)
			{
				if(V_NT_OBJECT == e_ns_get_node_type(linker))
				{
					if(found == ~0u)
						verse_send_o_link_destroy(active, link_id);
					else if((link = e_nso_get_link(linker, link_id)) != NULL)
						verse_send_o_link_set(active, link_id, found, e_nso_get_link_name(link), e_nso_get_link_target_id(link));
					else
						verse_send_o_link_set(active, link_id, found, compute_link_label(linker, node), 0);
				}
				if(V_NT_MATERIAL == e_ns_get_node_type(linker))
				{
					VMatFrag *frag, f;
					frag = e_nsm_get_fragment(linker, link_id);
					if(frag != NULL)
					{
						f = *frag;
						f.texture.bitmap = found;
						verse_send_m_fragment_create(active, link_id, VN_M_FT_TEXTURE, &f);
					}
				}
			}
			mode = COIM_NONE;
		}
	}

	if(mode == COIM_VIEW)
	{
		p_view_change(input);
		if(input->mouse_button[1] == FALSE && input->last_mouse_button[1] == FALSE)
			mode = COIM_NONE;
	}

	if(mode == COIM_OPEN)
	{
		clear_color = clear_color - 0.001;
		popup_move += betray_get_delta_time() / 0.4;
		if(popup_move > 1)
			popup_move = 1;

//		popup_move = popup_move * 0.95 + 0.05;
		if(input->mode == BAM_DRAW && (node = e_ns_get_node(0, active)) != NULL && (co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT)) != NULL)
		{
			float box[8] = {-0.45, 10, 10, 10, 10, -10, -0.45, -10};
			glTranslatef(co_node->pos_x + 1, co_node->pos_y, 0);
			sui_set_blend_gl(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			sui_draw_gl(GL_QUADS, box, 4, 2, co_background_color[0], co_background_color[1], co_background_color[2], popup_move * 0.5);
		}
		if((node = e_ns_get_node(0, active)) != NULL)
		{
			switch(e_ns_get_node_type(node))
			{
				case V_NT_OBJECT :
					if(!co_handle_object(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_GEOMETRY :
					if(!co_handle_geometry(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_MATERIAL :
					if(!co_handle_material(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_BITMAP :
					if(!co_handle_bitmap(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_TEXT :
					if(!co_handle_text(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_CURVE :
					if(!co_handle_curve(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				case V_NT_AUDIO :
					if(!co_handle_audio(input, node) && popup_move > 0.5)
						mode = COIM_NONE;
					break;
				default:
					break;
			}
		}

		if(mode == COIM_NONE && betray_is_type_in())
			betray_end_type_in_mode(TRUE);
	}else
	{
		popup_move -= betray_get_delta_time() / 0.4;
		if(popup_move < 0)
			popup_move = 0;
	}
	if(input->mode == BAM_DRAW)
		glPopMatrix();

	if(mode == COIM_CREATE)
	{
		if(!betray_is_type_in())
		{
			search_cursor = 0;
			betray_start_type_in(search, 64, NULL, &search_cursor, NULL);
		}
		if(input->mode == BAM_DRAW)
		{
			if(search[0] != 0)
			{
				float length;
				glPushMatrix();
				glTranslatef(0, 0, -1);
				length = sui_compute_text_length(SUI_T_SIZE * 2, SUI_T_SPACE, search);
				sui_draw_text(length * -0.5, 0.0, SUI_T_SIZE * 2, SUI_T_SPACE, search, co_line_color[0], co_line_color[1], co_line_color[2], 1.0);
				glPopMatrix();
			}
			glPushMatrix();
			glTranslatef(0, 0, -1);
		}
		create_scroll = co_w_scroll(input, create_scroll - (-5 * betray_get_screen_mode(NULL, NULL, NULL) + 2), create_length + 0.7) + (-5 * betray_get_screen_mode(NULL, NULL, NULL) + 2);
		if(input->mode == BAM_DRAW)
			glPopMatrix();
	}

	co_draw_settings(input);
	if(input->mode == BAM_DRAW)
		glEnable(GL_DEPTH_TEST);
}

void co_init_handle_verse_node(void)
{
	e_ns_set_custom_func(1, V_NT_OBJECT, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_GEOMETRY, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_MATERIAL, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_BITMAP, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_TEXT, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_CURVE, verse_node_create_func);
	e_ns_set_custom_func(1, V_NT_AUDIO, verse_node_create_func);
}
