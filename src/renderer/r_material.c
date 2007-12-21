/**
 * @file r_material.c
 * @brief Material related code
 */

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_local.h"
#include "r_error.h"


#define UPDATE_THRESHOLD 0.02
void R_UpdateMaterial (material_t *m)
{
	materialStage_t *s;

	if (refdef.time - m->time < UPDATE_THRESHOLD)
		return;

	m->time = refdef.time;

	for (s = m->stages; s; s = s->next) {
		if (s->flags & STAGE_ROTATE){
			s->rotate.dhz = refdef.time * s->rotate.hz * 6.28;
			s->rotate.dsin = sin(s->rotate.dhz);
			s->rotate.dcos = cos(s->rotate.dhz);
		}
	}
}

static void R_StageVertexColor (const materialStage_t *stage, const vec3_t v, vec4_t color)
{
	float a;

	VectorSet(color, 1.0, 1.0, 1.0);

	/* resolve alpha for vert based on z axis height */
	if (v[2] < stage->terrain.floor)
		a = 0.0;
	else if (v[2] > stage->terrain.ceil)
		a = 1.0;
	else
		a = (v[2] - stage->terrain.floor) / stage->terrain.height;

	color[3] = a;
}

static void R_StageVertex (const mBspSurface_t *surf, const materialStage_t *stage, const vec3_t in, vec3_t out)
{
	vec3_t tmp;

	/* translate from surface center */
	if (stage->flags & STAGE_STRETCH) {
		VectorSubtract(in, surf->center, tmp);
		VectorScale(tmp, stage->stretch.damp, tmp);
		VectorAdd(in, tmp, out);
	} else {  /* or simply copy */
		VectorCopy(in, out);
	}
}

static void R_StageTexcoord (const materialStage_t *stage, vec3_t v, vec2_t st)
{
	vec3_t tmp;
	float s, t, s0, t0;

	if (stage->flags & STAGE_ENVMAP) {  /* generate texcoords */
		tmp[0] = v[0] * r_world_matrix[0] + v[1] * r_world_matrix[4] +
				v[2] * r_world_matrix[8] + r_world_matrix[12];

		tmp[1] = v[0] * r_world_matrix[1] + v[1] * r_world_matrix[5] +
				v[2] * r_world_matrix[9] + r_world_matrix[13];

		tmp[2] = v[0] * r_world_matrix[2] + v[1] * r_world_matrix[6] +
				v[2] * r_world_matrix[10] + r_world_matrix[14];

		VectorNormalize(tmp);

		s = tmp[0];
		t = tmp[1];
	} else {  /* or use the ones from the vertex */
		s = v[3];
		t = v[4];
	}

	/* scale first... */
	if (stage->flags & STAGE_SCALE_S)
		s *= stage->scale.s;

	if (stage->flags & STAGE_SCALE_T)
		t *= stage->scale.t;

	/* ...then scroll */
	if (stage->flags & STAGE_SCROLL_S)
		s += stage->scroll.ds;

	if (stage->flags & STAGE_SCROLL_T)
		t += stage->scroll.dt;

	/* rotate */
	if (stage->flags & STAGE_ROTATE) {
		s0 = s - 0.5;
		t0 = t + 0.5;
		s = stage->rotate.dcos * s0 - stage->rotate.dsin * t0 + 0.5;
		t = stage->rotate.dsin * s0 + stage->rotate.dcos * t0 - 0.5;
	}

	st[0] = s;
	st[1] = t;
}

static void R_DrawMaterialSurface (mBspSurface_t *surf, materialStage_t *stage)
{
	int i, nv;
	float *v;

	if (stage->flags & STAGE_TERRAIN)
		R_EnableColorArray(qtrue);

	nv = surf->polys->numverts;
	R_Bind(stage->image->texnum);

	for (i = 0, v = surf->polys->verts[0]; i < nv; i++, v += VERTEXSIZE) {
		R_StageTexcoord(stage, v, &r_state.texture_texunit.texcoords[i * 2]);
		R_StageVertex(surf, stage, v, &r_state.vertex_array_3d[i * 3]);
		if (stage->flags & STAGE_TERRAIN)
			R_StageVertexColor(stage, v, &r_state.color_array[i * 4]);
	}

	qglDrawArrays(GL_POLYGON, 0, i);

	if (stage->flags & STAGE_TERRAIN)
		R_EnableColorArray(qfalse);

	R_CheckError();
}

void R_DrawMaterialSurfaces (mBspSurface_t *surfs)
{
	mBspSurface_t *surf;
	material_t *m;
	materialStage_t *s;
	vec4_t color;
	int i;

	if (!r_materials->integer || r_wire->integer)
		return;

	if (!surfs)
		return;

	qglEnable(GL_POLYGON_OFFSET_FILL);  /* all stages use depth offset */

	for (surf = surfs; surf; surf = surf->materialchain) {
		m = &surf->texinfo->image->material;

		R_UpdateMaterial(m);

		i = -1;
		for (s = m->stages; s; s = s->next, i--) {
			if (!(s->flags & STAGE_RENDER))
				continue;

			qglPolygonOffset(0, i);  /* increase depth offset for each stage */

			R_Bind(s->image->texnum);

			if (s->flags & STAGE_BLEND)
				R_BlendFunc(s->blend.src, s->blend.dest);
			else
				R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			VectorSet(color, 1.0, 1.0, 1.0);

			if (s->flags & STAGE_COLOR)
				VectorCopy(s->color, color);
#if 0
			else if (s->flags & STAGE_ENVMAP)
				VectorCopy(surf->color, color);
#endif

			if (s->flags & STAGE_PULSE)
				color[3] = s->pulse.dhz;
			else
				color[3] = 1.0;

			R_Color(color);

			R_DrawMaterialSurface(surf, s);
		}
	}

	R_Color(NULL);

	qglDisable(GL_POLYGON_OFFSET_FILL);

	R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief Translate string into glmode
 */
static GLenum R_ConstByName (const char *c)
{
	if (!strcmp(c, "GL_ONE"))
		return GL_ONE;
	if (!strcmp(c, "GL_SRC_ALPHA"))
		return GL_SRC_ALPHA;
	if (!strcmp(c, "GL_ONE_MINUS_SRC_ALPHA"))
		return GL_ONE_MINUS_SRC_ALPHA;

	Com_Printf("R_ConstByName: Failed to resolve: %s\n", c);
	return GL_ZERO;
}

/**
 * @brief Material stage parser
 * @sa R_LoadMaterials
 */
static int R_ParseStage (materialStage_t *s, const char **buffer)
{
	const char *c;
	int i;

	while (qtrue) {
		c = COM_Parse(buffer);

		if (!strlen(c))
			break;

		if (!strcmp(c, "texture")) {
			c = COM_Parse(buffer);
			s->image = R_FindImage(va("textures/%s", c), it_material);

			if (s->image == r_notexture) {
				Com_Printf("R_ParseStage: Failed to resolve texture: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_TEXTURE;
			continue;
		}

		if (!strcmp(c, "envmap")) {
			c = COM_Parse(buffer);
			i = atoi(c);

			if (i > -1 && i < MAX_ENVMAPTEXTURES)
				s->image = r_envmaptextures[i];
			else
				s->image = R_FindImage(va("envmaps/%s", c), it_static);

			if (s->image == r_notexture) {
				Com_Printf("R_ParseStage: Failed to resolve envmap: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_ENVMAP;
			continue;
		}

		if (!strcmp(c, "blend")) {
			c = COM_Parse(buffer);
			s->blend.src = R_ConstByName(c);

			if (s->blend.src == -1) {
				Com_Printf("R_ParseStage: Failed to resolve blend src: %s\n", c);
				return -1;
			}

			c = COM_Parse(buffer);
			s->blend.dest = R_ConstByName(c);

			if (s->blend.dest == -1) {
				Com_Printf("R_ParseStage: Failed to resolve blend dest: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_BLEND;
			continue;
		}

		if (!strcmp(c, "color")) {
			for (i = 0; i < 3; i++) {
				c = COM_Parse(buffer);
				s->color[i] = atof(c);

				if (s->color[i] < 0.0 || s->color[i] > 1.0) {
					Com_Printf("R_ParseStage: Failed to resolve color: %s\n", c);
					return -1;
				}
			}

			s->flags |= STAGE_COLOR;
			continue;
		}

		if (!strcmp(c, "pulse")) {
			c = COM_Parse(buffer);
			s->pulse.hz = atof(c);

			if (s->pulse.hz < 0) {
				Com_Printf("R_ParseStage: Failed to resolve frequency: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_PULSE;
			continue;
		}

		if (!strcmp(c, "stretch")) {
			c = COM_Parse(buffer);
			s->stretch.amp = atof(c);

			if (s->stretch.amp < 0) {
				Com_Printf("R_ParseStage: Failed to resolve amplitude: %s\n", c);
				return -1;
			}

			c = COM_Parse(buffer);
			s->stretch.hz = atof(c);

			if (s->stretch.hz < 0) {
				Com_Printf("R_ParseStage: Failed to resolve frequency: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_STRETCH;
			continue;
		}

		if (!strcmp(c, "rotate")) {
			c = COM_Parse(buffer);
			s->rotate.hz = atof(c);

			if (s->rotate.hz < 0) {
				Com_Printf("R_ParseStage: Failed to resolve rotate: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_ROTATE;
			continue;
		}

		if (!strcmp(c, "scroll.s")) {
			c = COM_Parse(buffer);
			s->scroll.s = atof(c);

			s->flags |= STAGE_SCROLL_S;
			continue;
		}

		if (!strcmp(c, "scroll.t")) {
			c = COM_Parse(buffer);
			s->scroll.t = atof(c);

			s->flags |= STAGE_SCROLL_T;
			continue;
		}

		if (!strcmp(c, "scale.s")) {
			c = COM_Parse(buffer);
			s->scale.s = atof(c);

			s->flags |= STAGE_SCALE_S;
			continue;
		}

		if (!strcmp(c, "scale.t")) {
			c = COM_Parse(buffer);
			s->scale.t = atof(c);

			s->flags |= STAGE_SCALE_T;
			continue;
		}

		if (!strcmp(c, "terrain")) {
			c = COM_Parse(buffer);
			s->terrain.floor = atof(c);

			c = COM_Parse(buffer);
			s->terrain.ceil = atof(c);

			s->terrain.height = s->terrain.ceil - s->terrain.floor;

			if (s->terrain.height == 0){
				Com_Printf("R_ParseStage: Zero height terrain specified for %s\n",
					(s->image ? s->image->name : NULL));
				return -1;
			}

			s->flags |= STAGE_TERRAIN;
		}

#if 0
		if (!strcmp(c, "flare")) {
			c = COM_Parse(buffer);
			i = atoi(c);

			if (i > -1 && i < NUM_FLARETEXTURES)
				s->image = r_flaretextures[i];
			else
				s->image = R_FindImage(va("flares/%s", c), it_material);

			if (s->image == r_notexture) {
				Com_Warn("R_ParseStage: Failed to resolve flare: %s\n", c);
				return -1;
			}

			s->flags |= STAGE_FLARE;
			continue;
		}
#endif

		if (*c == '}') {
			Com_DPrintf(DEBUG_RENDERER, "Parsed stage\n"
					"  flags: %d\n"
					"  image: %s\n"
					"  blend: %d %d\n"
					"  color: %3f %3f %3f\n"
					"  pulse: %3f\n"
					"  stretch: %3f %3f\n"
					"  rotate: %3f\n"
					"  scroll.s: %3f\n"
					"  scroll.t: %3f\n"
					"  scale.s: %3f\n"
					"  scale.t: %3f\n"
					"  terrain.floor: %5f\n"
					"  terrain.ceil: %5f\n",
					s->flags, (s->image ? s->image->name : "null"),
					s->blend.src, s->blend.dest,
					s->color[0], s->color[1], s->color[2],
					s->pulse.hz, s->stretch.amp, s->stretch.hz,
					s->rotate.hz, s->scroll.s, s->scroll.t,
					s->scale.s, s->scale.t, s->terrain.floor, s->terrain.ceil);

			/* a texture or envmap means render it */
			if (s->flags & (STAGE_TEXTURE | STAGE_ENVMAP))
				s->flags |= STAGE_RENDER;

			return 0;
		}
	}

	Com_Printf("R_ParseStage: Malformed stage\n");
	return -1;
}

/**
 * @brief Load material definitions for each map that has one
 * @param[in] map the base name of the map to load the material for
 */
void R_LoadMaterials (const char *map)
{
	char path[MAX_QPATH];
	const char *c;
	byte *fileBuffer;
	const char *buffer;
	qboolean inmaterial;
	image_t *image;
	material_t *m;
	materialStage_t *s, *ss;
	int i;

	/* clear previously loaded materials */
	R_ImageClearMaterials();

	/* load the materials file for parsing */
	Com_sprintf(path, sizeof(path), "materials/%s.mat", COM_SkipPath(map));

	if ((i = FS_LoadFile(path, &fileBuffer)) < 1){
		Com_DPrintf(DEBUG_RENDERER, "Couldn't load %s\n", path);
		return;
	}

	buffer = (const char *)fileBuffer;

	inmaterial = qfalse;
	image = NULL;
	m = NULL;

	while (qtrue) {
		c = COM_Parse(&buffer);

		if (!strlen(c))
			break;

		if (*c == '{' && !inmaterial) {
			inmaterial = qtrue;
			continue;
		}

		if (!strcmp(c, "material")) {
			c = COM_Parse(&buffer);
			image = R_FindImage(va("textures/%s", c), it_wall);

			if (image == r_notexture){
				Com_Printf("R_LoadMaterials: Failed to resolve texture: %s\n", c);
				image = NULL;
			}

			continue;
		}

		if (!image)
			continue;

		m = &image->material;

		if (*c == '{' && inmaterial) {
			s = (materialStage_t *)VID_TagAlloc(vid_imagePool, sizeof(materialStage_t), 0);

			if (R_ParseStage(s, &buffer) == -1) {
				VID_MemFree(s);
				continue;
			}

			/* append the stage to the chain */
			if (!m->stages)
				m->stages = s;
			else {
				ss = m->stages;
				while (ss->next)
					ss = ss->next;
				ss->next = s;
			}

			m->flags |= s->flags;
			m->num_stages++;
			continue;
		}

		if (*c == '}' && inmaterial) {
			Com_DPrintf(DEBUG_RENDERER, "Parsed material %s with %d stages\n", image->name, m->num_stages);
			inmaterial = qfalse;
			image = NULL;
		}
	}

	FS_FreeFile(fileBuffer);
}
