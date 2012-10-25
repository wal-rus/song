 #include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
	#include <windows.h>
	#include <GL/gl.h>
#else
#if defined(__APPLE__) || defined(MACOSX)
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif
#endif
#include "verse.h"
#include "enough.h"


typedef struct{
	char *code;
	uint pos;
	uint alloc;
}PSExeCode;

void p_shader_extend_code_create(PSExeCode *c)
{
	c->pos = 0;
	c->alloc = 2048;
	c->code = malloc((sizeof *c->code) * c->alloc);
	c->code[0] = 0;
}

void p_shader_extend_code(PSExeCode *c, char *text)
{
	uint i;

	if(c->pos + 257 > c->alloc)
	{
		c->alloc *= 2;
		c->code = realloc(c->code, (sizeof *c->code) * c->alloc);
	}
	for(i = 0; text[i] != 0; i++)
		c->code[c->pos++] = text[i];
	c->code[c->pos] = 0;
}

uint16 p_shader_choose_alternative(ENode *node, VMatFrag *frag)
{
	if(e_nsm_get_fragment(node, frag->alternative.alt_a) != NULL)
		return frag->alternative.alt_a;
	else
		return frag->alternative.alt_b;
}

void p_shader_get_name(ENode *node, char *frag_name, uint fragment)
{
	switch(e_nsm_get_fragment_type(node, fragment))
	{
		case VN_M_FT_COLOR :
			sprintf(frag_name, "color_%u", fragment);
			return;
		case VN_M_FT_LIGHT :
			sprintf(frag_name, "light_%u", fragment);
			return;
		case VN_M_FT_REFLECTION :
			sprintf(frag_name, "reflection_%u", fragment);
			return;
		case VN_M_FT_TRANSPARENCY :
			sprintf(frag_name, "transparency_%u", fragment);
			return;
		case VN_M_FT_VIEW :
			sprintf(frag_name, "view_%u", fragment);
			return;
		case VN_M_FT_VOLUME :
			sprintf(frag_name, "volume_%u", fragment);
			return;
		case VN_M_FT_GEOMETRY :
			sprintf(frag_name, "geometry_%u", fragment);
			return;
		case VN_M_FT_TEXTURE :
			sprintf(frag_name, "texture_%u", fragment);
			return;
		case VN_M_FT_NOISE :
			sprintf(frag_name, "noise_%u", fragment);
			return;
		case VN_M_FT_BLENDER :
			sprintf(frag_name, "blender_%u", fragment);
			return;
		case VN_M_FT_CLAMP :
			sprintf(frag_name, "clamp_%u", fragment);
			return;
		case VN_M_FT_MATRIX :
			sprintf(frag_name, "matrix_%u", fragment);
			return;
		case VN_M_FT_RAMP :
			sprintf(frag_name, "ramp_%u", fragment);
			return;
		case VN_M_FT_ANIMATION :
			sprintf(frag_name, "anim_%u", fragment);
			return;
		case VN_M_FT_ALTERNATIVE :
			sprintf(frag_name, "alternative_%u", fragment);
			return;
		case VN_M_FT_OUTPUT :
			sprintf(frag_name, "output_%u", fragment);
			return;
	}
	sprintf(frag_name, "strange");
	return;
}

uint p_shader_write_pre_compute(ENode *node, PSExeCode *v_c, PSExeCode *f_c, boolean initialized)
{
	uint16 fragment;
	uint count = 0;
	VNMFragmentType type;
	VMatFrag *frag;
	char temp[256];
	boolean reflection = FALSE;

	for(fragment = e_nsm_get_fragment_next(node, 0); fragment != (uint16)-1; fragment = e_nsm_get_fragment_next(node, fragment + 1))
	{
		temp[0] = 0;
		type = e_nsm_get_fragment_type(node, fragment);
		frag = e_nsm_get_fragment(node, fragment);
		if(type == VN_M_FT_GEOMETRY)
		{
			sprintf(temp, "attribute vec4 attrib_geometry_%u;\n", fragment);
			p_shader_extend_code(v_c, temp);
			sprintf(temp, "varying vec4 geometry_%u;\n", fragment);
		}
		if(type == VN_M_FT_COLOR)
		{
			if(initialized)
				sprintf(temp, "uniform vec4 color_%u = vec4(%f, %f, %f, 0.0);\n", fragment, frag->color.red, frag->color.green, frag->color.blue);
			else
				sprintf(temp, "uniform vec4 color_%u;\n", fragment);
		}
		if(type == VN_M_FT_ANIMATION)			
			sprintf(temp, "uniform vec4 anim_%u;\n", fragment);
		if(type == VN_M_FT_MATRIX)
		{
			if(initialized)
				sprintf(temp, "uniform mat4 matrix_%u = mat4(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f);\n", fragment
					, frag->matrix.matrix[0], frag->matrix.matrix[1], frag->matrix.matrix[2], frag->matrix.matrix[3]
					, frag->matrix.matrix[4], frag->matrix.matrix[5], frag->matrix.matrix[6], frag->matrix.matrix[7]
					, frag->matrix.matrix[8], frag->matrix.matrix[9], frag->matrix.matrix[10], frag->matrix.matrix[11]
					, frag->matrix.matrix[12], frag->matrix.matrix[13], frag->matrix.matrix[14], frag->matrix.matrix[15]);
			else
				sprintf(temp, "uniform mat4 matrix_%u;\n", fragment);
		}
		if(type == VN_M_FT_TEXTURE)	
			sprintf(temp, "uniform sampler2D texture_%u;\n", fragment);
		if(type == VN_M_FT_RAMP)	
			sprintf(temp, "uniform sampler1D ramp_%u;\n", fragment);
		if((type == VN_M_FT_REFLECTION || type == VN_M_FT_LIGHT) && reflection == FALSE)
		{
			sprintf(temp, "attribute mat3 r_matrix;\n");
			p_shader_extend_code(v_c, temp);
			sprintf(temp, "varying vec3 r_normal;\n");
			reflection = TRUE;
		}
		p_shader_extend_code(f_c, temp);
		p_shader_extend_code(v_c, temp);
		count++;
	}
	return count;
}

void p_shader_write_lights(ENode *node, PSExeCode *f_c, uint lights)
{
	uint16 fragment;
	VMatFrag *frag;
	boolean direct = FALSE, ambient = FALSE, back_direct = FALSE, back_ambient = FALSE, volume = FALSE;
	char buf[256];
	uint i;

	for(fragment = e_nsm_get_fragment_next(node, 0); fragment != (uint16)-1; fragment = e_nsm_get_fragment_next(node, fragment + 1))
	{
		if(e_nsm_get_fragment_type(node, fragment) == VN_M_FT_LIGHT)
		{
			frag = e_nsm_get_fragment(node, fragment);
			switch(frag->light.type)
			{
				case VN_M_LIGHT_DIRECT :
				direct = TRUE;
				break;
				case VN_M_LIGHT_AMBIENT :
				ambient = TRUE;
				break;
				case VN_M_LIGHT_DIRECT_AND_AMBIENT :
				direct = TRUE;
				ambient = TRUE;
				break;
				case VN_M_LIGHT_BACK_DIRECT :
				back_direct = TRUE;
				break;
				case VN_M_LIGHT_BACK_AMBIENT :
				back_ambient = TRUE;
				break;
				case VN_M_LIGHT_BACK_DIRECT_AND_AMBIENT :
				back_direct = TRUE;
				back_ambient = TRUE;
				break;
			}
		}
		if(e_nsm_get_fragment_type(node, fragment) == VN_M_FT_VOLUME)
			volume = TRUE;
	}
	if(direct == FALSE && ambient == FALSE && back_direct == FALSE && back_ambient == FALSE && volume == FALSE)
		return;

	p_shader_extend_code(f_c, "\tvec3 v");
	if(direct)
		p_shader_extend_code(f_c, ", light = vec3(0, 0, 0)");
	if(ambient)
		p_shader_extend_code(f_c, ", ambient");
	if(back_direct)
		p_shader_extend_code(f_c, ", b_light");
	if(back_ambient)
		p_shader_extend_code(f_c, ", b_ambient");
	p_shader_extend_code(f_c, ";\n");




	if(direct || back_direct || volume)
		p_shader_extend_code(f_c, "\tfloat f, dist;\n");

	for(fragment = e_nsm_get_fragment_next(node, 0); fragment != (uint16)-1; fragment = e_nsm_get_fragment_next(node, fragment + 1))
	{
		if(e_nsm_get_fragment_type(node, fragment) == VN_M_FT_VOLUME)
		{
			char code[1024];
			frag = e_nsm_get_fragment(node, fragment);
			sprintf(code,
				"\tv = gl_LightSource[0].position.xyz - pixel_pos.xyz;\n"
				"\tdist = length(v);\n"
				"\tv = normalize(v);\n"
				"\tf = dot(normal, v);\n"
				"\tvolume_%u += vec4(gl_LightSource[0].diffuse.rgb * max(vec3(0, 0, 0), (vec3(f, f, f) + vec3(%f, %f, %f)) / vec3(%f, %f, %f)) / vec3(dist, dist, dist), 0.0);\n"
				"\tv = gl_LightSource[1].position.xyz - pixel_pos.xyz;\n"
				"\tdist = length(v);\n"
				"\tv = normalize(v);\n"
				"\tf = dot(normal, v);\n"
				"\tvolume_%u += vec4(gl_LightSource[1].diffuse.rgb * max(vec3(0, 0, 0), (vec3(f, f, f) + vec3(%f, %f, %f)) / vec3(%f, %f, %f)) / vec3(dist, dist, dist), 0.0);\n"
				"\tv = gl_LightSource[2].position.xyz - pixel_pos.xyz;\n"
				"\tdist = length(v);\n"
				"\tv = normalize(v);\n"
				"\tf = dot(normal, v);\n"
				"\tvolume_%u += vec4(gl_LightSource[2].diffuse.rgb * max(vec3(0, 0, 0), (vec3(f, f, f) + vec3(%f, %f, %f)) / vec3(%f, %f, %f)) / vec3(dist, dist, dist), 0.0);\n",
				(uint)fragment, frag->volume.col_r, frag->volume.col_g, frag->volume.col_b, frag->volume.col_r + 1.0, frag->volume.col_g + 1.0, frag->volume.col_b + 1.0,
				(uint)fragment, frag->volume.col_r, frag->volume.col_g, frag->volume.col_b, frag->volume.col_r + 1.0, frag->volume.col_g + 1.0, frag->volume.col_b + 1.0,
				(uint)fragment, frag->volume.col_r, frag->volume.col_g, frag->volume.col_b, frag->volume.col_r + 1.0, frag->volume.col_g + 1.0, frag->volume.col_b + 1.0);
			p_shader_extend_code(f_c, code);
		}
	}

	for(i = 0; i < lights; i++)
	{
		if(direct || back_direct)
		{
			sprintf(buf, "\tv = gl_LightSource[%u].position.xyz - pixel_pos.xyz;\n\tdist = length(v);\n\tv = normalize(v);\n", i);
			p_shader_extend_code(f_c, buf);
		}
		if(direct)
		{
			sprintf(buf, "\tf = max(0.0, dot(normal, v)) / dist;\n\tlight += gl_LightSource[%u].diffuse.rgb * vec3(f, f, f);\n", i);
			p_shader_extend_code(f_c, buf);
		}
		if(back_direct)
		{
			sprintf(buf, "\tf = max(0.0, dot(normal, vec3(0.0, 0.0, 0.0) - v)) / dist;\n\tlight += gl_LightSource[%u].diffuse.rgb * vec3(f, f, f);\n", i);
			p_shader_extend_code(f_c, buf);
		}
	}
	if(ambient)
		p_shader_extend_code(f_c, "\tambient = textureCube(diffuse_environment, (normal * gl_NormalMatrix)).xyz;\n");
	if(back_ambient)
		p_shader_extend_code(f_c, "\tb_ambient = textureCube(diffuse_environment, (gl_TextureMatrix[0] * (vec4(0.0, 0.0, 0.0, 0.0) - vec4(normal, 0.0))).xyz).xyz;\n");
}

void p_shader_write_types(ENode *node, uint16 fragment, uint row_length, PSExeCode *c, uint *passed, uint *count)
{
	uint i;
	VMatFrag *frag;
	VNMFragmentType type;
	frag = e_nsm_get_fragment(node, fragment);

	type = e_nsm_get_fragment_type(node, fragment);

	for(i = 0; i < *count && passed[i * 2] != fragment; i++);
	if(i == *count)
	{
		passed[i * 2] = fragment;
		passed[i * 2 + 1] = 0;
		count[0]++;
	}else if(passed[i * 2 + 1] == 0)
	{
		char temp[256], temp2[256];
		p_shader_get_name(node, temp, fragment);
		sprintf(temp2, "\tvec4 t_%s;\n", temp);
		p_shader_extend_code(c, temp2);
		passed[i * 2 + 1] = 1;
		return;
	}

	if(!e_nsm_enter_fragment(node, fragment))
		return;
//p_shader_get_name(ENode *node, char *frag_name, uint fragment)
	if(type == VN_M_FT_MATRIX || type == VN_M_FT_BLENDER || type == VN_M_FT_CLAMP || type == VN_M_FT_NOISE || type == VN_M_FT_TEXTURE || type == VN_M_FT_RAMP || type == VN_M_FT_ALTERNATIVE || type == VN_M_FT_VOLUME)
	{
		if(type == VN_M_FT_VOLUME)
		{
			char temp[256], temp2[256];
			p_shader_get_name(node, temp, fragment);
			sprintf(temp2, "\tvec4 %s = vec4(0.0, 0.0, 0.0, 0.0);\n", temp);
			p_shader_extend_code(c, temp2);
		}else if(row_length > 5)
		{
			char temp[256], temp2[256];
			p_shader_get_name(node, temp, fragment);
			sprintf(temp2, "\tvec4 t_%s;\n", temp);
			p_shader_extend_code(c, temp2);
			row_length = 0;
		}
		if(type == VN_M_FT_TEXTURE)
			p_shader_write_types(node, frag->texture.mapping, ++row_length, c, passed, count);
		if(type == VN_M_FT_MATRIX)
			p_shader_write_types(node, frag->matrix.data, ++row_length, c, passed, count);
		if(type == VN_M_FT_NOISE)
			p_shader_write_types(node, frag->noise.mapping, ++row_length, c, passed, count);
		if(type == VN_M_FT_BLENDER)
		{
			p_shader_write_types(node, frag->blender.data_a, ++row_length, c, passed, count);
			p_shader_write_types(node, frag->blender.data_b, ++row_length, c, passed, count);
			if(frag->blender.type == VN_M_BLEND_FADE)
			{
				p_shader_write_types(node, frag->blender.control, ++row_length, c, passed, count);
				p_shader_write_types(node, frag->blender.control, ++row_length, c, passed, count);
			}
		}
		if(type == VN_M_FT_CLAMP)
			p_shader_write_types(node, frag->clamp.data, ++row_length, c, passed, count);
		if(type == VN_M_FT_RAMP)
			p_shader_write_types(node, frag->ramp.mapping, ++row_length, c, passed, count);
		if(type == VN_M_FT_ALTERNATIVE)
			p_shader_write_types(node, frag->noise.mapping, row_length, c, passed, count);
	}
	e_nsm_leave_fragment(node, fragment);
}


void p_shader_write_math(ENode *node, uint fragment, char *code, PSExeCode *c, uint *passed, uint *count)
{
	uint i;
	boolean	splitabel = TRUE;
	VMatFrag *frag;
	frag = e_nsm_get_fragment(node, fragment);
	code[0] = 0;
	
	if(frag == NULL || !e_nsm_enter_fragment(node, fragment))
	{
		sprintf(code, "vec4(0.0, 1.0, 0.0, 0.0)");
		return;
	}
	for(i = 0; passed[i * 2] != fragment; i++);

	if(passed[i * 2 + 1] < 2)
	{
		switch(e_nsm_get_fragment_type(node, fragment))
		{
			case VN_M_FT_COLOR :
			//	sprintf(code, "vec4(%f, %f, %f, 1)", (float)frag->color.red, (float)frag->color.green, (float)frag->color.blue);
				sprintf(code, "color_%u", fragment);
				splitabel = FALSE;
				break;
			case VN_M_FT_LIGHT :
			//	sprintf(code, "light_%u", fragment);
			//	sprintf(code, "vec4(dot(normalize(pixel_pos.xyz - vec3(0.2, 0.3, 1.0), vec3(0, 0, 1)))", fragment);
		//		sprintf(code, "vec4(dot(normal, vec3(1.0, 1.0, 1.0)), 1.0)", fragment); 
				switch(frag->light.type)
				{
					case VN_M_LIGHT_DIRECT :
					sprintf(code, "vec4(light, 0)");
					break;
					case VN_M_LIGHT_AMBIENT :
					sprintf(code, "vec4(ambient, 0)");
					break;
					case VN_M_LIGHT_DIRECT_AND_AMBIENT :
					sprintf(code, "vec4(light + ambient, 0)");
					break;
					case VN_M_LIGHT_BACK_DIRECT :
					sprintf(code, "vec4(b_light, 0)");
					break;
					case VN_M_LIGHT_BACK_AMBIENT :
					sprintf(code, "vec4(b_ambient, 0)");
					break;
					case VN_M_LIGHT_BACK_DIRECT_AND_AMBIENT :
					sprintf(code, "vec4(b_light + b_ambient, 0)");
					break;
				}
				break;
			case VN_M_FT_REFLECTION :
				sprintf(code, "vec4(textureCube(environment, reflect(pixel_pos.xyz, normal.xyz) * gl_NormalMatrix).xyz, 1)");
				break;
			case VN_M_FT_TRANSPARENCY :
				sprintf(code, "vec4(textureCube(environment, (normalize(pixel_pos.xyz) - normal * vec3(%f, %f, %f)) * gl_NormalMatrix).xyz, 1)", frag->transparency.refraction_index - 1.0, frag->transparency.refraction_index - 1.0, frag->transparency.refraction_index - 1.0);			
				break;
			case VN_M_FT_VIEW :
				sprintf(code, "vec4(normal.xyz, 1)");
				break;
			case VN_M_FT_VOLUME :
				sprintf(code, "volume_%u", fragment);
				break;
			case VN_M_FT_GEOMETRY :
				sprintf(code, "geometry_%u", fragment);
				splitabel = FALSE;
				break;
			case VN_M_FT_TEXTURE :
				{
					char input[5120];
					p_shader_write_math(node, frag->texture.mapping, input, c, passed, count);
					sprintf(code, "texture2D(texture_%u, %s.xy)", fragment, input);
				}
				break;
			case VN_M_FT_ANIMATION :
				sprintf(code, "anim_%u", fragment);
				splitabel = FALSE;
				break;
			case VN_M_FT_NOISE :
				{
					char input[5120];
					p_shader_write_math(node, frag->noise.mapping, input, c, passed, count);
					sprintf(code, "vec4(noise1((%s.xy * vec2(10.0))) + 0.5)", input);

				}
				break;
			case VN_M_FT_BLENDER :
				{
					switch(frag->blender.type)
					{
						case VN_M_BLEND_FADE :
						{
							char data_a[5120], data_b[5120], control[5120];
							p_shader_write_math(node, frag->blender.control, control, c, passed, count);
							p_shader_write_math(node, frag->blender.data_a, data_a, c, passed, count);
							p_shader_write_math(node, frag->blender.data_b, data_b, c, passed, count);
							sprintf(code, "(%s * %s + %s * (vec4(1.0, 1.0, 1.0, 1.0) - %s))", data_a, control, data_b, control);
						}
						break;
						case VN_M_BLEND_ADD:
						{
							char data_a[5120], data_b[5120];
							p_shader_write_math(node, frag->blender.data_a, data_a, c, passed, count);
							p_shader_write_math(node, frag->blender.data_b, data_b, c, passed, count);
							sprintf(code, "(%s + %s)", data_a, data_b);
						}
						break;
						case VN_M_BLEND_SUBTRACT:
						{
							char data_a[5120], data_b[5120];
							p_shader_write_math(node, frag->blender.data_a, data_a, c, passed, count);
							p_shader_write_math(node, frag->blender.data_b, data_b, c, passed, count);
							sprintf(code, "(%s - %s)", data_a, data_b);
						}
						break;
						case VN_M_BLEND_MULTIPLY:
						{
							char data_a[5120], data_b[5120];
							p_shader_write_math(node, frag->blender.data_a, data_a, c, passed, count);
							p_shader_write_math(node, frag->blender.data_b, data_b, c, passed, count);
							sprintf(code, "(%s * %s)", data_a, data_b);
						}
						break;
						case VN_M_BLEND_DIVIDE:
						{
							char data_a[5120], data_b[5120];
							p_shader_write_math(node, frag->blender.data_a, data_a, c, passed, count);
							p_shader_write_math(node, frag->blender.data_b, data_b, c, passed, count);
							sprintf(code, " (%s * %s)", data_a, data_b);
						}
						break;
					}
				}
				break;
			case VN_M_FT_CLAMP :
				{
					char input[5120];
					p_shader_write_math(node, frag->clamp.data, input, c, passed, count);
					if(frag->clamp.min)
						sprintf(code, "min(vec4(%f, %f, %f, 1.0), %s)", frag->clamp.red, frag->clamp.green, frag->clamp.blue, input);
					else
						sprintf(code, "max(vec4(%f, %f, %f, 1.0), %s)", frag->clamp.red, frag->clamp.green, frag->clamp.blue, input);
					splitabel = FALSE;
				}
				break;
			case VN_M_FT_MATRIX :
				{
					char input[5120];
					p_shader_write_math(node, frag->matrix.data, input, c, passed, count);
					sprintf(code, "(matrix_%u * %s)", fragment, input);
					splitabel = FALSE;
				}
				break;
			case VN_M_FT_RAMP :
				{
					char input[5120];
					p_shader_write_math(node, frag->ramp.mapping, input, c, passed, count);

					if(frag->ramp.channel == VN_M_RAMP_RED)
						sprintf(code, "(texture1D(ramp_%u, ((%s).r - %f) / %f))", (uint)fragment, input, frag->ramp.ramp[0].pos, frag->ramp.ramp[frag->ramp.point_count - 1].pos - frag->ramp.ramp[0].pos);
					else if(frag->ramp.channel == VN_M_RAMP_GREEN)
						sprintf(code, "(texture1D(ramp_%u, ((%s).g - %f) / %f))", (uint)fragment, input, frag->ramp.ramp[0].pos, frag->ramp.ramp[frag->ramp.point_count - 1].pos - frag->ramp.ramp[0].pos);
					else
						sprintf(code, "(texture1D(ramp_%u, ((%s).b - %f) / %f))", (uint)fragment, input, frag->ramp.ramp[0].pos, frag->ramp.ramp[frag->ramp.point_count - 1].pos - frag->ramp.ramp[0].pos);
				}

				break;
			case VN_M_FT_ALTERNATIVE :
				{
					char input[5120];
					p_shader_write_math(node, frag->alternative.alt_a, input, c, passed, count);
					sprintf(code, "%s", input);
				}
				break;
			case VN_M_FT_OUTPUT :
				{
					char input[5120];
					p_shader_write_math(node, frag->output.front, input, c, passed, count);
					sprintf(code, "%s", input);
				}
				break;
		}
	}
	if(passed[i * 2 + 1] == 1 && splitabel)
	{
		char temp[5120], name[5120];
		p_shader_get_name(node, name, fragment);
		sprintf(temp, "\tt_%s = %s;\n\n", name, code);
		p_shader_extend_code(c, temp);
		passed[i * 2 + 1]++;
	}
	if(passed[i * 2 + 1] > 1)
	{
		char name[5120];
		p_shader_get_name(node, name, fragment);
		sprintf(code, "t_%s", name);

	}
	e_nsm_leave_fragment(node, fragment);
}

VNMFragmentID p_shader_compute_get_fragment_color_front(ENode *node, uint *dest, uint *src)
{
	VMatFrag *frag;
	VNMFragmentID fragment, a, b;
	VNMFragmentType ta, tb;
	VMatFrag *fa, *fb;
	VNMBlendType type;
	*dest = GL_ZERO;
	*src = GL_ONE;
	fragment = e_nsm_get_fragment_color_front(node);
	if(fragment != (uint16)-1)
	{
		if(e_nsm_get_fragment_type(node, fragment) == VN_M_FT_BLENDER)
		{
			frag = e_nsm_get_fragment(node, fragment);
			a = frag->blender.data_a;
			b = frag->blender.data_b;
			type = frag->blender.type;
			ta = e_nsm_get_fragment_type(node, a);
			tb = e_nsm_get_fragment_type(node, b);
			fa = e_nsm_get_fragment(node, a);
			fb = e_nsm_get_fragment(node, b);
			if(ta == VN_M_FT_TRANSPARENCY && fa->transparency.refraction_index > 0.99 && fa->transparency.refraction_index < 1.01 && tb == VN_M_FT_TRANSPARENCY && fb->transparency.refraction_index > 0.99 && fb->transparency.refraction_index < 1.01)
			{
				if(type == VN_M_BLEND_MULTIPLY)
				{
					*dest = GL_SRC_COLOR;
					*src = GL_ZERO;
				}
				if(type == VN_M_BLEND_ADD)
				{
					*dest = GL_ONE;
					*src = GL_ONE;
				}
				if(type == VN_M_BLEND_SUBTRACT)
				{
					*dest = GL_ZERO;
					*src = GL_ZERO;
				}
				if(type == VN_M_BLEND_DIVIDE)
				{
					*dest = GL_ZERO;
					*src = GL_ONE;
				}
				if(type == VN_M_BLEND_FADE)
				{
					*dest = GL_ONE;
					*src = GL_ZERO;
				}
				return -1;
			}
			if((ta == VN_M_FT_TRANSPARENCY && fa->transparency.refraction_index > 0.99 && fa->transparency.refraction_index < 1.01) || (tb == VN_M_FT_TRANSPARENCY && fb->transparency.refraction_index > 0.99 && fb->transparency.refraction_index < 1.01))
			{
				if(frag->blender.type == VN_M_BLEND_MULTIPLY)
				{
					*dest = GL_SRC_COLOR;
					*src = GL_ZERO;
				}
				if(frag->blender.type == VN_M_BLEND_ADD)
				{
					*dest = GL_ONE;
					*src = GL_ONE;
				}
				if(ta == VN_M_FT_TRANSPARENCY)
				{
					return b;
				}else
				{
					return a;
				}
			}

		}
	}
	return fragment;
}

typedef struct{
	PSExeCode	f_c;
	PSExeCode	v_c;
	uint		count; 
	uint		*data;
	uint		stage;
	VNMFragmentID start;
}PCodeGenTemp;

extern uint p_light_count;

void p_render_set_light_count(uint lights)
{
	if(lights < 16)
		p_light_count = lights;
}

uint p_render_get_light_count(void)
{
	return p_light_count;
}

void *p_shader_write(ENode *node, char **v_code, GLint *v_length, char **f_code, GLint *f_length, uint *dest, uint *src, PCodeGenTemp *t, boolean initialized)
{
	uint16 fragment;
	char code[5120], temp[5120];

	if(e_nsm_get_fragment(node, e_nsm_get_fragment_color_front(node)) != NULL)
	{
		if(t == NULL)
		{
			t = malloc(sizeof *t);
			t->stage = 0;
			p_shader_extend_code_create(&t->f_c);
			p_shader_extend_code_create(&t->v_c);
//			p_shader_extend_code(&t->f_c, "uniform gl_LightSourceParameters gl_LightSource[3];\n");
			p_shader_extend_code(&t->v_c, "varying vec4 pixel_pos;\nvarying vec3 normal;\nvarying vec3 env_normal;\n");
			p_shader_extend_code(&t->f_c, "varying vec4 pixel_pos;\nvarying vec3 normal;\nvarying vec3 env_normal;\n");
			p_shader_extend_code(&t->f_c, "uniform samplerCube environment;\n");
			p_shader_extend_code(&t->f_c, "uniform samplerCube diffuse_environment;\n");
			t->start = p_shader_compute_get_fragment_color_front(node, dest, src);
		}else
		{
			switch(t->stage)
			{
				case 0 :
					t->count = p_shader_write_pre_compute(node, &t->v_c, &t->f_c, initialized);
					if(t->count != 0)
						t->data = malloc((sizeof *t->data) * t->count * 4);
					else
						t->data = NULL;
					p_shader_extend_code(&t->v_c, "\nvoid main()\n{\n");
					p_shader_extend_code(&t->f_c, "\nvoid main()\n{\n");
					t->count = 0;
					break;
				case 1 :
					p_shader_write_types(node, t->start, 1, &t->f_c, t->data, &t->count);
					p_shader_write_lights(node, &t->f_c, p_light_count);
					p_shader_extend_code(&t->v_c, "\n\tnormal = normalize(gl_NormalMatrix * gl_Normal);\n");
					break;
				case 2 :
					p_shader_write_math(node, t->start, temp, &t->f_c, t->data, &t->count);
					sprintf(code, "\tgl_FragColor = vec4(vec3((%s).rgb), 1.0);\n", temp);
					p_shader_extend_code(&t->f_c, code);
					break;
				case 3 :
					for(fragment = e_nsm_get_fragment_next(node, 0); fragment != (uint16)-1; fragment = e_nsm_get_fragment_next(node, fragment + 1))
					{
						if(e_nsm_get_fragment_type(node, fragment) == VN_M_FT_GEOMETRY)
						{
							sprintf(temp, "\tgeometry_%u = attrib_geometry_%u;\n", fragment, fragment);
							p_shader_extend_code(&t->v_c, temp);
						}			
					}

					p_shader_extend_code(&t->v_c, "\tpixel_pos = gl_ModelViewMatrix * gl_Vertex;\n\n");
					p_shader_extend_code(&t->v_c, "\tgl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n}\n");

					p_shader_extend_code(&t->f_c, "}\n");
					if(t->data != NULL)
						free(t->data);
					*v_code = t->v_c.code;
					*v_length = t->v_c.pos + 1;
					*f_code = t->f_c.code;
					*f_length = t->f_c.pos + 1;	
					free(t);
					return NULL;
			}
			t->stage++;
		}
	}else
	{
		*v_code = NULL;
		*v_length = 0;
		*f_code = NULL;
		*f_length = 0;
		return NULL;
	}
	return t;
}

void p_shader_code_write(ENode *node, char **v_code, GLint *v_length, char **f_code, GLint *f_length, uint *dest, uint *src, boolean initialized)
{
	PCodeGenTemp *t = NULL;

	while((t = p_shader_write(node, v_code, v_length, f_code, f_length, dest, src, t, initialized)) != NULL)
		;
}

void p_shader_write_destroy_temp(PCodeGenTemp *t)
{
	free(t->v_c.code);
	free(t->f_c.code);
	if(t->stage > 0)
		free(t->data);
}
