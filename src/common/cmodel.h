/**
 * @file cmodel.h
 * @brief Common model code header (for bsp and others)
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

/*==============================================================
CMODEL
==============================================================*/
#include "../common/qfiles.h"
#include "pqueue.h"


extern vec3_t mapMin, mapMax;

void CM_LoadMap(const char *tiles, qboolean day, const char *pos, unsigned *checksum);
cBspModel_t *CM_InlineModel(const char *name);
void CM_SetInlineModelOrientation(const char *name, const vec3_t origin, const vec3_t angles);

int CM_NumClusters(void);
int CM_NumInlineModels(void);
const char *CM_EntityString(void);

/*==============================================================
CMODEL BOX TRACING
==============================================================*/

/** creates a clipping hull for an arbitrary box */
int CM_HeadnodeForBox(int tile, const vec3_t mins, const vec3_t maxs);
trace_t CM_CompleteBoxTrace(const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int tile, int headnode, int brushmask, int brushrejects, const vec3_t origin, const vec3_t angles);
trace_t CM_HintedTransformedBoxTrace(const int tile, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, const int headnode, const int brushmask, const int brushrejects, const vec3_t origin, const vec3_t angles, const vec3_t rmaShift, const float fraction);
qboolean CM_TestLineWithEnt(const vec3_t start, const vec3_t stop, const int levelmask, const char **entlist);
qboolean CM_TestLineDMWithEnt(const vec3_t start, const vec3_t stop, vec3_t end, const int levelmask, const char **entlist);
qboolean CM_EntTestLine(const vec3_t start, const vec3_t stop, const int levelmask);
qboolean CM_EntTestLineDM(const vec3_t start, const vec3_t stop, vec3_t end, const int levelmask);
trace_t CM_EntCompleteBoxTrace(const vec3_t start, const vec3_t end, const box_t* traceBox, int levelmask, int brushmask, int brushreject);
void CM_RecalcRouting(routing_t *map, const char *name, const char **list);

/*==========================================================
GRID ORIENTED MOVEMENT AND SCANNING
==========================================================*/

extern routing_t svMap[ACTOR_MAX_SIZE], clMap[ACTOR_MAX_SIZE];
extern pathing_t svPathMap;

void Grid_DumpWholeServerMap_f(void);
void Grid_DumpWholeClientMap_f(void);
void Grid_DumpClientRoutes_f(void);
void Grid_DumpServerRoutes_f(void);
void Grid_RecalcBoxRouting(routing_t *map, const pos3_t min, const pos3_t max);
void Grid_RecalcRouting(routing_t *map, const char *name);
void Grid_DumpDVTable(const pathing_t *path);
void Grid_MoveCalc(const routing_t *map, const actorSizeEnum_t actorSize, pathing_t *path, const pos3_t from, byte crouchingSstate, int distance, byte ** fb_list, int fb_length);
void Grid_MoveStore(pathing_t *path);
pos_t Grid_MoveLength(const pathing_t *path, const pos3_t to, byte crouchingState, qboolean stored);
int Grid_MoveNext(const pathing_t *path, const pos3_t toPos, byte crouchingState);
int Grid_Height(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos);
unsigned int Grid_Ceiling(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos);
int Grid_Floor(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos);
pos_t Grid_StepUp(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos, const int dir);
int Grid_GetTUsForDirection(int dir);
int Grid_Filled(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos);
pos_t Grid_Fall(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos);
void Grid_PosToVec(const routing_t *map, const actorSizeEnum_t actorSize, const pos3_t pos, vec3_t vec);

