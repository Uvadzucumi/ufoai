/**
 * @file cl_shader.c
 * @brief Parses and loads shaders and materials from files and passes them to the renderer.
 *
 *
 * Reads in a shader program from a file, puts it in a struct and tell the renderer about the location of this struct.
 * This is to allow the adding of shaders without recompiling the binaries.
 * You can add shaders for each texture; new maps can bring new textures.
 * Without a scriptable shader interface we have to rebuild the renderer to get a shader for a specific texture.
 * They are here and not in the renderer because the file parsers are part of the client.
 * @todo: Add command to re-initialise shaders to allow interactive debugging.
 */

/*
Copyright (C) 2002-2007 UFO: Alien Invasion team.

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


#include "client.h"

/* extern in client.h */
int r_numshaders;

/* shader_t is in client/ref.h */
/* r_shaders is external and is linked to refdef.shaders in cl_view.c V_RenderView */
shader_t r_shaders[MAX_SHADERS];

static const value_t shader_values[] = {
	{"filename", V_CLIENT_HUNK_STRING, offsetof(shader_t, filename), 0},
	{"normal", V_CLIENT_HUNK_STRING, offsetof(shader_t, normal), 0},
	{"distort", V_CLIENT_HUNK_STRING, offsetof(shader_t, distort), 0},
	{"frag", V_BOOL, offsetof(shader_t, frag), MEMBER_SIZEOF(shader_t, frag)},
	{"vertex", V_BOOL, offsetof(shader_t, vertex), MEMBER_SIZEOF(shader_t, vertex)},
	{"glsl", V_BOOL, offsetof(shader_t, glsl), MEMBER_SIZEOF(shader_t, glsl)},
	{"glmode", V_BLEND, offsetof(shader_t, glMode), MEMBER_SIZEOF(shader_t, glMode)},
	{"emboss", V_BOOL, offsetof(shader_t, emboss), MEMBER_SIZEOF(shader_t, emboss)},
	{"emboss_high", V_BOOL, offsetof(shader_t, embossHigh), MEMBER_SIZEOF(shader_t, embossHigh)},
	{"emboss_low", V_BOOL, offsetof(shader_t, embossLow), MEMBER_SIZEOF(shader_t, embossLow)},
	{"emboss_2", V_BOOL, offsetof(shader_t, emboss2), MEMBER_SIZEOF(shader_t, emboss2)},
	{"blur", V_BOOL, offsetof(shader_t, blur), MEMBER_SIZEOF(shader_t, blur)},
	{"light", V_BOOL, offsetof(shader_t, light), MEMBER_SIZEOF(shader_t, light)},
	{"edge", V_BOOL, offsetof(shader_t, edge), MEMBER_SIZEOF(shader_t, edge)},

	{NULL, 0, 0, 0}
};

/** @brief Valid terrain material parameter definitions from script files. */
static const value_t terrain_param_vals[] = {
	{"ceil", V_FLOAT, offsetof(materialStage_t, terrain.ceil), MEMBER_SIZEOF(terrain_t, ceil)},
	{"floor", V_FLOAT, offsetof(materialStage_t, terrain.floor), MEMBER_SIZEOF(terrain_t, floor)},
	{"blend", V_BLEND, offsetof(materialStage_t, blend), MEMBER_SIZEOF(materialStage_t, blend)},

	{NULL, 0, 0, 0}
};

/** @brief Valid rotate material parameter definitions from script files. */
static const value_t rotate_param_vals[] = {
	{"hz", V_FLOAT, offsetof(materialStage_t, rotate.hz), MEMBER_SIZEOF(rotate_t, hz)},
	{"blend", V_BLEND, offsetof(materialStage_t, blend), MEMBER_SIZEOF(materialStage_t, blend)},

	{NULL, 0, 0, 0}
};

/**
 * @brief Parses all shader script from ufo script files
 * @note Called from CL_ParseClientData
 * @sa CL_ParseClientData
 */
void CL_ParseShaders (const char *name, const char **text)
{
	shader_t *entry;
	const value_t *v;
	const char *errhead = "CL_ParseShaders: unexpected end of file (names ";
	const char *token;
	int i;

	for (i = 0; i < r_numshaders; i++) {
		if (!Q_strcmp(r_shaders[i].name, name)) {
			Com_Printf("CL_ParseShaders: Second shader with same name found (%s) - second ignored\n", name);
			return;
		}
	}

	if (r_numshaders >= MAX_SHADERS) {
		Com_Printf("CL_ParseShaders: shader \"%s\" ignored - too many shaders\n", name);
		return;
	}

	/* get name list body body */
	token = COM_Parse(text);
	if (!*text || *token != '{') {
		Com_Printf("CL_ParseShaders: shader \"%s\" without body ignored\n", name);
		return;
	}

	/* new entry */
	entry = &r_shaders[r_numshaders++];
	memset(entry, 0, sizeof(shader_t));

	/* default value */
	entry->glMode = BLEND_FILTER;

	entry->name = Mem_PoolStrDup(name, cl_genericPool, CL_TAG_NONE);
	do {
		/* get the name type */
		token = COM_EParse(text, errhead, name);
		if (!*text)
			break;
		if (*token == '}')
			break;

		/* get values */
		for (v = shader_values; v->string; v++)
			if (!Q_strcmp(token, v->string)) {
				/* found a definition */
				token = COM_EParse(text, errhead, name);
				if (!*text)
					return;

				switch (v->type) {
				case V_NULL:
					break;
				case V_CLIENT_HUNK_STRING:
					Mem_PoolStrDupTo(token, (char**) ((char*)entry + (int)v->ofs), cl_genericPool, CL_TAG_NONE);
					break;
				default:
					Com_ParseValue(entry, token, v->type, v->ofs, v->size);
					break;
				}
				break;
			}

		if (!v->string) {
			materialStage_t *stage, *stageLoop;
			char textureName[MAX_QPATH];
			int flag;
			if (!Q_strcmp(token, "terrain")) {
				flag = STAGE_TERRAIN;
				v = terrain_param_vals;
			} else if (!Q_strcmp(token, "rotate")) {
				flag = STAGE_ROTATE;
				v = rotate_param_vals;
			} else {
				Com_Printf("CL_ParseShaders: unknown token \"%s\" ignored (entry %s)\n", token, name);
				continue;
			}

			assert(v);
			token = COM_EParse(text, errhead, name);
			Q_strncpyz(textureName, token, sizeof(textureName));

			token = COM_EParse(text, errhead, name);
			if (!*text || *token != '{') {
				Com_Printf("CL_ParseShaders: Invalid param value for shader: %s\n", name);
				return;
			}

			if (!entry->material)
				entry->material = Mem_PoolAlloc(sizeof(material_t), cl_genericPool, CL_TAG_NONE);

			stage = Mem_PoolAlloc(sizeof(materialStage_t), cl_genericPool, CL_TAG_NONE);
			stage->textureName = Mem_PoolStrDup(textureName, cl_genericPool, CL_TAG_NONE);
			stage->next = NULL;
			stage->flags |= flag;

			assert(entry->material);
			entry->material->num_stages++;
			if (entry->material->stages) {
				stageLoop = entry->material->stages;
				while (stageLoop) {
					if (!stageLoop->next) {
						stageLoop->next = stage;
						break;
					}
					stageLoop = stageLoop->next;
				}
			} else {
				entry->material->stages = stage;
			}
			do {
				token = COM_EParse(text, errhead, name);
				if (!*text)
					break;
				if (*token == '}')
					break;
				for (; v->string; v++)
					if (!Q_strcmp(token, v->string)) {
						/* found a definition */
						token = COM_EParse(text, errhead, name);
						if (!*text)
							return;
						Com_ParseValue(stage, token, v->type, v->ofs, v->size);
						break;
					}
				if (!v->string)
					Com_Printf("CL_ParseShaders: Ignoring unknown param value '%s'\n", token);
			} while (*text); /* dummy condition */

			if (stage->flags & STAGE_TERRAIN)
				stage->terrain.height = stage->terrain.ceil - stage->terrain.floor;
		}

	} while (*text);
}

/**
 * @brief List all loaded shaders via console
 */
void CL_ShaderList_f (void)
{
	int i;

	for (i = 0; i < r_numshaders; i++) {
		Com_Printf("Shader %s\n", r_shaders[i].name);
		Com_Printf("..filename: %s\n", r_shaders[i].filename);
		Com_Printf("..frag %i\n", (int) r_shaders[i].frag);
		Com_Printf("..vertex %i\n", (int) r_shaders[i].vertex);
		Com_Printf("..emboss %i\n", (int) r_shaders[i].emboss);
		Com_Printf("..emboss low %i\n", (int) r_shaders[i].embossLow);
		Com_Printf("..emboss high %i\n", (int) r_shaders[i].embossHigh);
		Com_Printf("..emboss #2 %i\n", (int) r_shaders[i].emboss2);
		Com_Printf("..blur %i\n", (int) r_shaders[i].blur);
		Com_Printf("..light blur %i\n", (int) r_shaders[i].light);
		Com_Printf("..edge %i\n", (int) r_shaders[i].edge);
		Com_Printf("..glMode '%s'\n", Com_ValueToStr(&r_shaders[i], V_BLEND, offsetof( shader_t, glMode)));
		if (r_shaders[i].material) {
			materialStage_t *stage = r_shaders[i].material->stages;
			Com_Printf("..num_stages %i\n", r_shaders[i].material->num_stages);
			while (stage) {
				Com_Printf("..terrain texture: '%s'\n", stage->textureName);
				Com_Printf("..terrain ceil: '%.2f'\n", stage->terrain.ceil);
				Com_Printf("..terrain floor: '%.2f'\n", stage->terrain.floor);
				Com_Printf("..terrain height: '%.2f'\n", stage->terrain.height);
				Com_Printf("..rotate hz: '%.2f'\n", stage->rotate.hz);
				stage = stage->next;
			}
		}
		Com_Printf("\n");
	}
}
