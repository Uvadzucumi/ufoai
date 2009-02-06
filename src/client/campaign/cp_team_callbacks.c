/**
 * @file cp_team_callbacks.c
 * @brief Menu related callback functions for the team menu
 */

/*
All original materal Copyright (C) 2002-2007 UFO: Alien Invasion team.

Original file from Quake 2 v3.21: quake2-2.31/client/cl_main.c
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

#include "../client.h"
#include "../cl_global.h"
#include "../cl_actor.h"
#include "../cl_team.h"
#include "../cl_ugv.h"
#include "../menu/m_nodes.h"	/**< menuInventory */

#include "cp_team.h"
#include "cp_team_callbacks.h"

linkedList_t *employeeList;	/* @sa E_GetEmployeeByMenuIndex */
int employeesInCurrentList;

/**
 * @brief Returns the aircraft for the team and soldier selection menus
 * @note Multiplayer and skirmish are using @c cls.missionaircraft, campaign mode is
 * using the current selected aircraft in base
 */
static inline aircraft_t *CL_GetTeamAircraft (base_t *base)
{
	if (!base)
		Sys_Error("CL_GetTeamAircraft: Called without base");
	return base->aircraftCurrent;
}

/***********************************************************
 * Bindings
 ***********************************************************/

/* cache soldier list */
static int soldier_list_size = 0;
static int soldier_list_pos = 0;

static qboolean CL_UpdateEmployeeList (employeeType_t employeeType, char *nodeTag, int beginIndex, int drawableListSize)
{
	aircraft_t *aircraft;
	linkedList_t *emplList;
	int id;

	/* Check if we are allowed to be here.
	 * We are only allowed to be here if we already set up a base. */
	if (!baseCurrent) {
		Com_Printf("No base set up\n");
		return qfalse;
	}

	aircraft = CL_GetTeamAircraft(baseCurrent);
	if (!aircraft) {
		return qfalse;
	}

	CL_UpdateActorAircraftVar(aircraft, employeeType);

	soldier_list_size = drawableListSize;
	soldier_list_pos = beginIndex;

	/* Populate employeeList - base might be NULL for none-campaign mode games */
	employeesInCurrentList = E_GetHiredEmployees(aircraft->homebase, employeeType, &employeeList);
	emplList = employeeList;
	id = 0;
	while (emplList) {
		const employee_t *employee = (employee_t*)emplList->data;
		qboolean alreadyInOtherShip;
		const aircraft_t *otherShip;
		int guiId = id - beginIndex;
		int j;

		assert(employee->hired);
		assert(!employee->transfer);

		/* id lower than viewable item */
		if (guiId < 0) {
			emplList = emplList->next;
			id++;
			continue;
		}
		/* id bigger than viewable item */
		if (guiId >= drawableListSize) {
			emplList = emplList->next;
			id++;
			continue;
		}

		/* Set name of the employee. */
		Cvar_ForceSet(va("mn_ename%i", guiId), employee->chr.name);

		/* Search all aircraft except the current one. */
		otherShip = AIR_IsEmployeeInAircraft(employee, NULL);
		alreadyInOtherShip = (otherShip != NULL) && (otherShip != aircraft);

		/* Update status */
		if (alreadyInOtherShip)
			MN_ExecuteConfunc("aircraft_%s_usedelsewhere %i", nodeTag, guiId);
		else if (AIR_IsEmployeeInAircraft(employee, aircraft))
			MN_ExecuteConfunc("aircraft_%s_assigned %i", nodeTag, guiId);
		else
			MN_ExecuteConfunc("aircraft_%s_unassigned %i", nodeTag, guiId);

		/* Check if the employee has something equipped. */
		for (j = 0; j < csi.numIDs; j++) {
			/** @todo Wouldn't it be better here to check for temp containers */
			if (j != csi.idFloor && j != csi.idEquip && employee->chr.inv.c[j])
				break;
		}
		if (j < csi.numIDs)
			MN_ExecuteConfunc("aircraft_%s_holdsequip %i", nodeTag, guiId);
		else
			MN_ExecuteConfunc("aircraft_%s_holdsnoequip %i", nodeTag, guiId);

		if (cl_selected->integer == id)
			MN_ExecuteConfunc("aircraft_%s_selected %i", nodeTag, guiId);

		emplList = emplList->next;
		id++;
	}

	MN_ExecuteConfunc("aircraft_%s_list_size %i", nodeTag, id);

	for (;id - beginIndex < drawableListSize; id++) {
		int guiId = id - beginIndex;
		MN_ExecuteConfunc("aircraft_%s_unusedslot %i", nodeTag, guiId);
		Cvar_ForceSet(va("mn_name%i", guiId), "");
	}

	return qtrue;
}

/**
 * @brief Init the teamlist checkboxes
 * @sa CL_UpdateActorAircraftVar
 * @todo Make this function use a temporary list with all list-able employees
 * instead of using gd.employees[][] directly. See also CL_Select_f->SELECT_MODE_TEAM
 */
static void CL_UpdateSoldierList_f (void)
{
	qboolean resukt;
	int drawableListSize;
	int beginIndex;

	if (Cmd_Argc() != 3) {
		Com_Printf("Usage: %s <drawableSize> <firstIndex>\n", Cmd_Argv(0));
		return;
	}
	drawableListSize = atoi(Cmd_Argv(1));
	beginIndex = atoi(Cmd_Argv(2));

	resukt = CL_UpdateEmployeeList(EMPL_SOLDIER, "soldier", beginIndex, drawableListSize);
	if (!resukt)
		MN_PopMenu(qfalse);
}

/**
 * @brief Init the teamlist checkboxes
 * @sa CL_UpdateActorAircraftVar
 * @todo Make this function use a temporary list with all list-able employees
 * instead of using gd.employees[][] directly. See also CL_Select_f->SELECT_MODE_TEAM
 */
static void CL_UpdatePilotList_f (void)
{
	qboolean resukt;
	int drawableListSize;
	int beginIndex;
	if (Cmd_Argc() != 3) {
		Com_Printf("Usage: %s <drawableSize> <firstIndex>\n", Cmd_Argv(0));
		return;
	}
	drawableListSize = atoi(Cmd_Argv(1));
	beginIndex = atoi(Cmd_Argv(2));

	resukt = CL_UpdateEmployeeList(EMPL_PILOT, "pilot", beginIndex, drawableListSize);
	if (!resukt)
		MN_PopMenu(qfalse);
}

/**
 * @brief Displays actor equipment and unused items in proper (filter) category.
 * @note This function is called everytime the equipment screen for the team pops up.
 * @todo Do we allow EMPL_ROBOTs to be equipable? Or is simple buying of ammo enough (similar to original UFO/XCOM)?
 * In the first case the EMPL_SOLDIER stuff below needs to be changed.
 */
static void CL_UpdateEquipmentMenuParameters_f (void)
{
	int p;
	aircraft_t *aircraft;

	if (!baseCurrent)
		return;

	aircraft = CL_GetTeamAircraft(baseCurrent);
	if (!aircraft)
		return;

	/* no soldiers are assigned to the current aircraft. */
	if (!aircraft->teamSize) {
		MN_PopMenu(qfalse);
		return;
	}

	Cvar_ForceSet("cl_selected", "0");

	/** @todo Skip EMPL_ROBOT (i.e. ugvs) for now . */
	p = CL_UpdateActorAircraftVar(aircraft, EMPL_SOLDIER);
	for (; p < MAX_ACTIVETEAM; p++) {
		Cvar_ForceSet(va("mn_name%i", p), "");
		MN_ExecuteConfunc("equipdisable %i", p);
	}

	/* reset description */
	Cvar_Set("mn_itemname", "");
	Cvar_Set("mn_item", "");
	MN_MenuTextReset(TEXT_STANDARD);

	CL_CleanTempInventory(aircraft->homebase);
	CL_ReloadAndRemoveCarried(aircraft, &aircraft->homebase->storage);
}

/**
 * @brief Adds or removes a pilot to/from an aircraft.
 * @sa E_EmployeeHire_f
 */
static void CL_AssignPilot_f (void)
{
	employee_t *employee;
	base_t *base = baseCurrent;
	aircraft_t *aircraft;
	int relativeId = 0;
	int num;
	const employeeType_t employeeType = EMPL_PILOT;

	/* check syntax */
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3) {
		Com_Printf("Usage: %s <num> <relative_id>\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 3)
		relativeId = atoi(Cmd_Argv(2));

	num = atoi(Cmd_Argv(1)) + relativeId;
	if (num >= E_CountHired(baseCurrent, employeeType))
		return;

	/* In case we didn't populate the list with E_GenerateHiredEmployeesList before. */
	if (!employeeList)
		return;

	employee = E_GetEmployeeByMenuIndex(num);
	if (!employee)
		Sys_Error("CL_AssignPilot_f: No employee at list-pos %i (base: %i)\n", num, baseCurrent->idx);

	aircraft = CL_GetTeamAircraft(base);
	if (!aircraft)
		return;

	if (aircraft->pilot == NULL) {
		aircraft->pilot = employee;
	} else if (aircraft->pilot == employee) {
		aircraft->pilot = NULL;
	}

	CL_UpdateActorAircraftVar(aircraft, employeeType);

	MN_ExecuteConfunc("aircraft_status_change");
	MN_ExecuteConfunc("pilot_select %i %i", num - relativeId, relativeId);

}

/**
 * @brief Adds or removes a soldier to/from an aircraft.
 * @sa E_EmployeeHire_f
 */
static void CL_AssignSoldier_f (void)
{
	base_t *base = baseCurrent;
	aircraft_t *aircraft;
	int relativeId = 0;
	int num;
	const employeeType_t employeeType =
		cls.displayHeavyEquipmentList
			? EMPL_ROBOT
			: EMPL_SOLDIER;

	/* check syntax */
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3) {
		Com_Printf("Usage: %s <num> <relative_id>\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 3)
		relativeId = atoi(Cmd_Argv(2));

	num = atoi(Cmd_Argv(1)) + relativeId;
	if (num >= E_CountHired(baseCurrent, employeeType))
		return;

	/* In case we didn't populate the list with E_GenerateHiredEmployeesList before. */
	if (!employeeList)
		return;

	aircraft = CL_GetTeamAircraft(base);
	if (!aircraft)
		return;

	AIM_AddEmployeeFromMenu(aircraft, num);
	CL_UpdateActorAircraftVar(aircraft, employeeType);

	MN_ExecuteConfunc("aircraft_status_change");
	Cbuf_AddText(va("team_select %i %i\n", num - relativeId, relativeId));
}

static void CL_ActorPilotSelect_f (void)
{
	employee_t *employee;
	character_t *chr;
	int num;
	int relativeId = 0;
	const employeeType_t employeeType = EMPL_PILOT;

	if (!baseCurrent)
		return;

	/* check syntax */
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3) {
		Com_Printf("Usage: %s <num> <relative_id>\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 3)
		relativeId = atoi(Cmd_Argv(2));

	num = atoi(Cmd_Argv(1)) + relativeId;
	if (num >= E_CountHired(baseCurrent, employeeType)) {
		CL_ResertCharacterCvars();
		return;
	}

	employee = E_GetEmployeeByMenuIndex(num);
	if (!employee)
		Sys_Error("CL_ActorPilotSelect_f: No employee at list-pos %i (base: %i)\n", num, baseCurrent->idx);

	chr = &employee->chr;
	if (!chr)
		Sys_Error("CL_ActorPilotSelect_f: No hired character at list-pos %i (base: %i)\n", num, baseCurrent->idx);

	/* now set the cl_selected cvar to the new actor id */
	Cvar_ForceSet("cl_selected", va("%i", num));

	/* set info cvars */
	CL_CharacterCvars(chr);
	MN_ExecuteConfunc("update_pilot_list %i %i", soldier_list_size, soldier_list_pos);
}

static void CL_ActorTeamSelect_f (void)
{
	employee_t *employee;
	character_t *chr;
	int num;
	int relativeId = 0;
	const employeeType_t employeeType = cls.displayHeavyEquipmentList
			? EMPL_ROBOT : EMPL_SOLDIER;

	if (!baseCurrent)
		return;

	/* check syntax */
	if (Cmd_Argc() < 2 || Cmd_Argc() > 3) {
		Com_Printf("Usage: %s <num> <relative_id>\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() == 3)
		relativeId = atoi(Cmd_Argv(2));

	num = atoi(Cmd_Argv(1)) + relativeId;
	if (num >= E_CountHired(baseCurrent, employeeType)) {
		CL_ResertCharacterCvars();
		return;
	}

	employee = E_GetEmployeeByMenuIndex(num);
	if (!employee)
		Sys_Error("CL_ActorTeamSelect_f: No employee at list-pos %i (base: %i)\n", num, baseCurrent->idx);

	chr = &employee->chr;
	if (!chr)
		Sys_Error("CL_ActorTeamSelect_f: No hired character at list-pos %i (base: %i)\n", num, baseCurrent->idx);

	/* now set the cl_selected cvar to the new actor id */
	Cvar_ForceSet("cl_selected", va("%i", num));

	/* set info cvars */
	if (chr->emplType == EMPL_ROBOT)
		CL_UGVCvars(chr);
	else
		CL_CharacterCvars(chr);
	MN_ExecuteConfunc("update_soldier_list %i %i", soldier_list_size, soldier_list_pos);
}

#ifdef DEBUG
static void CL_TeamListDebug_f (void)
{
	int i;
	base_t *base;
	aircraft_t *aircraft;

	base = CP_GetMissionBase();
	aircraft = cls.missionaircraft;

	if (!base) {
		Com_Printf("Build and select a base first\n");
		return;
	}

	if (!aircraft) {
		Com_Printf("Buy/build an aircraft first.\n");
		return;
	}

	Com_Printf("%i members in the current team", aircraft->teamSize);
	for (i = 0; i < aircraft->maxTeamSize; i++) {
		if (aircraft->acTeam[i]) {
			const character_t *chr = &aircraft->acTeam[i]->chr;
			Com_Printf("ucn %i - employee->idx: %i\n", chr->ucn, aircraft->acTeam[i]->idx);
		}
	}
}
#endif

void CP_TEAM_InitCallbacks (void)
{
	Cmd_AddCommand("team_updateequip", CL_UpdateEquipmentMenuParameters_f, NULL);
	Cmd_AddCommand("update_soldier_list", CL_UpdateSoldierList_f, NULL);
	Cmd_AddCommand("update_pilot_list", CL_UpdatePilotList_f, NULL);

	Cmd_AddCommand("team_hire", CL_AssignSoldier_f, "Add/remove already hired actor to the aircraft");
	Cmd_AddCommand("pilot_hire", CL_AssignPilot_f, "Add/remove already hired pilot to an aircraft");
	Cmd_AddCommand("team_select", CL_ActorTeamSelect_f, "Select a soldier in the team creation menu");
	Cmd_AddCommand("pilot_select", CL_ActorPilotSelect_f, "Select a pilot in the team creation menu");
#ifdef DEBUG
	Cmd_AddCommand("debug_teamlist", CL_TeamListDebug_f, "Debug function to show all hired and assigned teammembers");
#endif
}

void CP_TEAM_ShutdownCallbacks (void)
{
	Cmd_RemoveCommand("team_updateequip");
	Cmd_RemoveCommand("update_soldier_list");
	Cmd_RemoveCommand("update_pilot_list");
	Cmd_RemoveCommand("team_hire");
	Cmd_RemoveCommand("pilot_hire");
	Cmd_RemoveCommand("team_select");
	Cmd_RemoveCommand("pilot_select");
#ifdef DEBUG
	Cmd_RemoveCommand("debug_teamlist");
#endif
}
