#include "la_includes.h"

#include "st_matrix_operations.h"
#include "la_geometry_undo.h"

boolean *selection_fill(uint poly)
{
    uint32 vertex_count, polygon_count, *ref, i;
	boolean *select, found = TRUE;
    udg_get_geometry(&vertex_count, &polygon_count, NULL, &ref, NULL);
    select = malloc((sizeof *select) * vertex_count);
    for(i = 0; i < vertex_count; i++)
        select[i] = FALSE;
	select[ref[poly * 4]] = TRUE;
	select[ref[poly * 4 + 1]] = TRUE;
	select[ref[poly * 4 + 2]] = TRUE;
	if(ref[poly * 4 + 3] < vertex_count)
		select[ref[poly * 4 + 3]] = TRUE;
	polygon_count *= 4;
    while(found == TRUE)
    {
        found = FALSE;
        for(i = 0; i < polygon_count; i += 4)
        {
            if(ref[i] < vertex_count)
            {
                if(select[ref[i]] || select[ref[i + 1]] || select[ref[i + 2]] || (ref[i + 3] < vertex_count && select[ref[i +3]]))
                {
                    if(select[ref[i]] == FALSE || select[ref[i + 1]] == FALSE|| select[ref[i + 2]] == FALSE || (ref[i + 3] < vertex_count && select[ref[i +3]] == FALSE))
                    {
                        select[ref[i]] = TRUE;
                        select[ref[i + 1]] = TRUE;
                        select[ref[i + 2]] = TRUE;
                        if(ref[i + 3] < vertex_count)
							select[ref[i +3]] = TRUE;
                        found = TRUE;
                    }
                }
            }
        }
    }
	return select;
}

void la_t_polygon_select_fill(uint poly)
{
    uint i, vertex_count;
    boolean *select;
    udg_get_geometry(&vertex_count, NULL, NULL, NULL, NULL);    
    select = selection_fill(poly);
    for(i = 0; i < vertex_count; i++)
        if(select[i] == TRUE)
            udg_set_select(i, 1);
    free(select);
}

typedef struct{
	uint	poly;
	uint32	v_count;
	uint32	p_count;
	uint32	*ref;
	uint32	*crease;
	uint32	*vertex_id;
	double	*vertex;
	double	origo[3];
	double	vectors[12];
}LATDeployParam;

void create_poly_vectors(double	*origo, double	*vectors, uint *ref, uint poly)
{
	uint i;
	double r, vertex[12];
	udg_get_vertex_pos(vertex, ref[0]);
	udg_get_vertex_pos(&vertex[3], ref[1]);
	udg_get_vertex_pos(&vertex[6], ref[2]);
	if(poly == 4)
	{
		udg_get_vertex_pos(&vertex[9], ref[3]);
		origo[0] = (vertex[0] + vertex[3] + vertex[6] + vertex[9]) * 0.25;
		origo[1] = (vertex[1] + vertex[4] + vertex[7] + vertex[10]) * 0.25;
		origo[2] = (vertex[2] + vertex[5] + vertex[8] + vertex[11]) * 0.25;
	}else
	{
		origo[0] = (vertex[0] + vertex[3] + vertex[6]) / 3.0;
		origo[1] = (vertex[1] + vertex[4] + vertex[7]) / 3.0;
		origo[2] = (vertex[2] + vertex[5] + vertex[8]) / 3.0;
	}
	for(i = 0; i < poly; i++)
	{
		vectors[i * 3 + 0] = (vertex[i * 3 + 0] + vertex[((i + 1) % poly) * 3 + 0]) / 2.0 - origo[0];
		vectors[i * 3 + 1] = (vertex[i * 3 + 1] + vertex[((i + 1) % poly) * 3 + 1]) / 2.0 - origo[1];
		vectors[i * 3 + 2] = (vertex[i * 3 + 2] + vertex[((i + 1) % poly) * 3 + 2]) / 2.0 - origo[2];
		r = sqrt(vectors[i * 3 + 0] * vectors[i * 3 + 0] + vectors[i * 3 + 1] * vectors[i * 3 + 1] + vectors[i * 3 + 2] * vectors[i * 3 + 2]);
		vectors[i * 3 + 0] /= r;		
		vectors[i * 3 + 1] /= r;
		vectors[i * 3 + 2] /= r;
	}
}

uint select_poly_rotate(double *matrix, double *v_a, double *pos_a, double *v_b, double *pos_b, uint poly)
{
	double r, matrix2[16], matrix3[16], pos[] = {0, 0, 0}, best = -1;
	uint dir_a = 0, dir_b = 0, i, j;

	for(i = 0; i < poly; i++)
	{
		for(j = 0; j < poly; j++)
		{
			r = v_a[i * 3 + 0] * v_b[j * 3 + 0] + v_a[i * 3 + 1] * v_b[j * 3 + 1] + v_a[i * 3 + 2] * v_b[j * 3 + 2];
			if(r > best)
			{
				best = r;
				dir_a = i;
				dir_b = j;
			}
		}
	}
	create_matrix_normalized(matrix2, pos, &v_a[dir_a * 3], &v_a[((poly + dir_a - 1) % poly) * 3]);
	create_matrix_normalized(matrix3, pos, &v_b[dir_b * 3], &v_b[((poly + dir_b + 1) % poly) * 3]);
	negate_matrix(matrix2);
	matrix_multiply(matrix3, matrix2, matrix);

	return (poly + dir_a + dir_b + 2) % poly;
}

void grabb_marked_mesh(LATDeployParam *param, uint poly)
{
	uint32 vertex_count, polygon_count, *ref, i, j, k, *crease;
	boolean *select;
	egreal *vertex;

	param->v_count = 0;
	param->p_count = 0;
	udg_get_geometry(&vertex_count, &polygon_count, &vertex, &ref, &crease);
	param->poly = 3;
	if(ref[poly * 4 + 3] < vertex_count)
		param->poly = 4;
	select = selection_fill(poly);
	for(i = 0; i < vertex_count; i++)
		if(select[i])
			param->v_count++;
	for(i = 0; i < polygon_count; i++)
		if(ref[i * 4] < vertex_count && select[ref[i * 4]])
			param->p_count++;
	param->ref = malloc((sizeof *param->ref) * param->p_count * 4);
	param->crease = malloc((sizeof *param->crease) * param->p_count * 4);
	param->vertex_id = malloc((sizeof *param->vertex_id) * param->v_count);
	param->vertex = malloc((sizeof *param->vertex) * param->v_count);
	j = 0;
	polygon_count *= 4;
	poly *= 4;
	param->ref[3] = -1;
	for(i = 0; i < 3 || (i == 3 && ref[poly + 3] < vertex_count); i++)
	{
		param->vertex_id[i] = ref[poly + i];
		param->vertex[i * 3] = vertex[ref[poly + i] * 3];
		param->vertex[i * 3 + 1] = vertex[ref[poly + i] * 3 + 1];
		param->vertex[i * 3 + 2] = vertex[ref[poly + i] * 3 + 2];
		param->ref[i]  = ref[poly + i];
		param->crease[i] = crease[i];
	}
	param->v_count = i;
	param->p_count = 4;
	for(i = 0; i < polygon_count; i += 4)
	{
		if(ref[i] < vertex_count && select[ref[i]] && i != poly)
		{
			for(j = 0 ; j < 4; j++)
			{
				if(j == 3 && ref[i + j] >= vertex_count)
					k = -1;
				else
				{
					for(k = 0; param->vertex_id[k] != ref[i + j] && k < param->v_count; k++);
					if(k == param->v_count)
					{
						param->vertex_id[param->v_count] = ref[i + j];
						param->vertex[param->v_count] = vertex[ref[i + j]];
						param->v_count++;
					}
				}
				param->ref[param->p_count] = k;
				param->crease[param->p_count++] = crease[i + j];
			}
		}
	}
	create_poly_vectors(param->origo, param->vectors, &ref[poly], param->poly);
	param->p_count /= 4;
}

/*
udg_vertex_set(uint32 id, double *state, double x, double y, double z);
extern void udg_vertex_move(uint32 id, double x, double y, double z);
extern void udg_vertex_delete(uint32 id);
extern void udg_get_vertex_pos(double *pos, uint vertex_id);

*/

void copy_marked_mech(LATDeployParam *param, uint poly)
{
    uint32 *ref, i, vertex_count, rot;
	double vertex[3], origo[3], matrix[16], vectors[12];
	udg_get_geometry(&vertex_count, NULL, NULL, &ref, NULL);
	create_poly_vectors(origo, vectors, &ref[poly * 4], param->poly);
	rot = select_poly_rotate(matrix, param->vectors, param->origo, vectors, origo, param->poly);	
	for(i = 0; i < param->poly; i++)
		param->vertex_id[param->poly - i - 1] = ref[poly * 4 + ((i + rot) % param->poly)];

	udg_get_vertex_pos(vertex, ref[poly * 4]);
	vertex[0] -= param->vertex[0];
	vertex[1] -= param->vertex[1];
	vertex[2] -= param->vertex[2];
	for(; i < param->v_count; i++)
	{
		param->vertex_id[i] = udg_find_empty_slot_vertex();
		vertex[0] = param->vertex[i * 3] - param->origo[0];
		vertex[1] = param->vertex[i * 3 + 1] - param->origo[1];
		vertex[2] = param->vertex[i * 3 + 2] - param->origo[2];
/*		printf("vertex pre %f %f %f\n", vertex[0], vertex[1], vertex[2]);*/
		point_threw_matrix3(matrix, &vertex[0], &vertex[1], &vertex[2]);
/*		printf("vertex post %f %f %f\n", vertex[0], vertex[1], vertex[2]);*/
		udg_vertex_set(param->vertex_id[i], NULL, vertex[0] + origo[0], vertex[1] + origo[1], vertex[2] + origo[2]);
	}
	for(i = 1; i < param->p_count; i++)
	{
		if(param->ref[i * 4 + 3] < param->v_count)
			udg_polygon_set(poly, param->vertex_id[param->ref[i * 4]], param->vertex_id[param->ref[i * 4 + 1]], param->vertex_id[param->ref[i * 4 + 2]], param->vertex_id[param->ref[i * 4 + 3]]);
		else
			udg_polygon_set(poly, param->vertex_id[param->ref[i * 4]], param->vertex_id[param->ref[i * 4 + 1]], param->vertex_id[param->ref[i * 4 + 2]], -1);
		udg_crease_set(poly, param->crease[i * 4], param->crease[i * 4 + 1], param->crease[i * 4 + 2], param->crease[i * 4 + 3]);
		poly = udg_find_empty_slot_polygon();
	}
}

void la_t_deploy(uint poly)
{
	LATDeployParam param;
    uint32 vertex_count, polygon_count, *ref, i, j;
	grabb_marked_mesh(&param, poly);
    udg_get_geometry(&vertex_count, &polygon_count, NULL, &ref, NULL);
	for(i = 0; i < polygon_count; i++)
	{
		if(ref[i * 4] < vertex_count)
		{
			for(j = 0; j < param.poly && ref[i * 4 + j] < vertex_count && udg_get_select(ref[i * 4 + j]) > 0.01; j++);
			if(j == param.poly)
				copy_marked_mech(&param, i);
		}
	}
}
