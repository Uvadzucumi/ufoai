#include "MRU.h"

#include "ieventmanager.h"
#include "iuimanager.h"
#include <gtk/gtkmenu.h>
#include <gtk/gtkcontainer.h>
#include "string/string.h"
#include "generic/callback.h"
#include "os/file.h"
#include "radiant_i18n.h"

#include "../../qe3.h"
#include "../../map/map.h"

namespace ui {

namespace {
	const std::string RKEY_MAP_ROOT = "user/ui/map";
	const std::string RKEY_MAP_MRUS = RKEY_MAP_ROOT + "/MRU";
	const std::string RKEY_MRU_LENGTH = RKEY_MAP_ROOT + "/numMRU";
	const std::string RKEY_LOAD_LAST_MAP = RKEY_MAP_ROOT + "/loadLastMap";

	const std::string RECENT_FILES_CAPTION = _("Recently used Maps");
}

MRU::MRU() :
	_numMaxFiles(GlobalRegistry().getInt(RKEY_MRU_LENGTH)),
	_loadLastMap(GlobalRegistry().get(RKEY_LOAD_LAST_MAP) == "1"),
	_list(_numMaxFiles),
	_emptyMenuItem(RECENT_FILES_CAPTION, *this, 0)
{
	GlobalRegistry().addKeyObserver(this, RKEY_MRU_LENGTH);

	// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
	GlobalPreferenceSystem().addConstructor(this);

	// Create _numMaxFiles menu items
	for (unsigned int i = 0; i < _numMaxFiles; i++) {

		_menuItems.push_back(MRUMenuItem(string::toString(i), *this, i + 1));

		MRUMenuItem& item = (*_menuItems.rbegin());

		const std::string commandName = std::string("MRUOpen") + string::toString(i + 1);

		GlobalEventManager().addCommand(
			commandName,
			MemberCaller<MRUMenuItem, &MRUMenuItem::activate>(item)
		);
	}
}

void MRU::loadRecentFiles() {
	// Loads the registry values from the last to the first (recentMap4 ... recentMap1) and
	// inserts them. After everything is loaded, the file list is sorted correctly.
	for (unsigned int i = _numMaxFiles; i > 0; i--) {

		const std::string key = RKEY_MAP_MRUS + "/map" + string::toString(i);
		const std::string fileName = GlobalRegistry().get(key);

		// Insert the filename
		insert(fileName);
	}

	if (_list.empty()) {
		_emptyMenuItem.show();
	}
}

void MRU::saveRecentFiles() {
	// Delete all existing MRU/element nodes
	GlobalRegistry().deleteXPath(RKEY_MAP_MRUS);

	unsigned int counter = 1;

	// Now wade through the list and save them in the correct order
	for (MRUList::iterator i = _list.begin(); i != _list.end(); counter++, i++) {

		const std::string key = RKEY_MAP_MRUS + "/map" + string::toString(counter);

		// Save the string into the registry
		GlobalRegistry().set(key, (*i));
	}
}

void MRU::loadMap(const std::string& fileName) {
	if (ConfirmModified(_("Open Map"))) {
		if (file_readable(fileName)) {
			// Shut down the current map
			Map_RegionOff();
			Map_Free();

			// Load the file
			Map_LoadFile(fileName);

			// Update the MRU list with this file
			insert(fileName);
		}
	}
}

void MRU::keyChanged() {
	// greebo: Don't load the new number of maximum files from the registry,
	// this would mess up the existing widgets, wait for the DarkRadiant restart instead
	//_numMaxFiles = GlobalRegistry().getInt(RKEY_MRU_LENGTH);
	_loadLastMap = (GlobalRegistry().get(RKEY_LOAD_LAST_MAP) == "1");
}

// Construct the orthoview preference page and add it to the given group
void MRU::constructPreferencePage(PreferenceGroup& group) {
	PreferencesPage* page(group.createPage(_("Map Files"), _("Map File Preferences")));

	page->appendEntry(_("Number of most recently used files"), RKEY_MRU_LENGTH);
	page->appendCheckBox("", _("Open last map on startup"), RKEY_LOAD_LAST_MAP);
}

bool MRU::loadLastMap() const {
	return _loadLastMap;
}

std::string MRU::getLastMapName() {
	if (_list.empty()) {
		return "";
	}
	else {
		MRUList::iterator i = _list.begin();
		return *i;
	}
}

void MRU::insert(const std::string& fileName) {

	if (fileName != "") {
		// Insert the item into the filelist
		_list.insert(fileName);

		// Hide the empty menu item, as we're having a MRU map now
		_emptyMenuItem.hide();

		// Update the widgets
		updateMenu();
	}
}

void MRU::updateMenu() {
	// Set the iterator to the first filename in the list
	MRUList::iterator i = _list.begin();

	// Now cycle through the widgets and load the values
	for (MenuItems::iterator m = _menuItems.begin(); m != _menuItems.end(); m++) {

		// The default string to be loaded into the widget (i.e. "inactive")
		std::string fileName = "";

		// If the end of the list is reached, do nothing, otherwise increase the iterator
		if (i != _list.end()) {
			fileName = (*i);
			i++;
		}

		// Set the label (widget is shown/hidden automatically)
		m->setLabel(fileName);
	}
}

void MRU::constructMenu() {
	// Get the menumanager
	IMenuManager* menuManager = GlobalUIManager().getMenuManager();

	// Create the "empty" MRU menu item (the desensitised one)
	GtkWidget* empty = menuManager->insert(
		"main/file/exit",
		"mruempty",
		ui::menuItem,
		RECENT_FILES_CAPTION,
		"", // empty icon
		"" // empty event
	);
	_emptyMenuItem.setWidget(empty);
	gtk_widget_hide(empty);

	// Add all the created widgets to the menu
	for (MenuItems::iterator m = _menuItems.begin(); m != _menuItems.end(); m++) {
		MRUMenuItem& item = (*m);

		const std::string commandName = std::string("MRUOpen") + string::toString(item.getIndex());

		// Create the toplevel menu item
		GtkWidget* menuItem = menuManager->insert(
			"main/file/exit",
			"MRU" + string::toString(item.getIndex()),
			ui::menuItem,
			item.getLabel(),
			"", // empty icon
			commandName
		);


		item.setWidget(menuItem);
	}

	// Insert the last separator to split the MRU file list from the "Exit" command.
	menuManager->insert(
		"main/file/exit",
		"mruseparator",
		ui::menuSeparator,
		"", // empty caption
		"", // empty icon
		"" // empty command
	);

	// Call the update routine to load the values into the widgets
	updateMenu();
}

// -------------------------------------------------------------------------------------

} // namespace ui

// The accessor function to the MRU instance
ui::MRU& GlobalMRU() {
	static ui::MRU _mruInstance;
	return _mruInstance;
}
