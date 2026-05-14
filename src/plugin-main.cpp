/*
uhohbs
Copyright (C) 2026 Owen Isenhart isenhart.owen@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QAction>
#include <QCoreApplication>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>
#include <QWidget>

#include "dump_dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

namespace {
QPointer<dump_dock> gDock;
obs_hotkey_id gDumpHotkeyId = OBS_INVALID_HOTKEY_ID;
QAction *gShowDockAction = nullptr;

void on_dump_hotkey(void *, obs_hotkey_id, obs_hotkey_t *, bool pressed)
{
	if (!pressed) {
		return;
	}

	auto *app = QCoreApplication::instance();
	if (!app) {
		return;
	}

	QMetaObject::invokeMethod(
		app,
		[]() {
			if (gDock) {
				gDock->trigger_dump_from_hotkey();
			}
		},
		Qt::QueuedConnection);
}
} // namespace

bool obs_module_load(void)
{
	gDock = new dump_dock();
	if (!obs_frontend_add_custom_qdock("uhohbs_dock", gDock.data())) {
		obs_log(LOG_ERROR, "failed to register dock with OBS frontend");
		delete gDock.data();
		gDock = nullptr;
		return false;
	}

	gShowDockAction =
		static_cast<QAction *>(obs_frontend_add_tools_menu_qaction(obs_module_text("menu.show_dock")));
	if (gShowDockAction) {
		QObject::connect(gShowDockAction, &QAction::triggered, []() {
			if (!gDock) {
				return;
			}

			auto *mainWindow =
				qobject_cast<QMainWindow *>(static_cast<QWidget *>(obs_frontend_get_main_window()));
			if (mainWindow) {
				if (mainWindow->dockWidgetArea(gDock.data()) == Qt::NoDockWidgetArea) {
					mainWindow->addDockWidget(Qt::LeftDockWidgetArea, gDock.data());
				}

				gDock->setFloating(false);
			}

			gDock->show();
			gDock->raise();
			gDock->activateWindow();
		});
	}

	gDumpHotkeyId =
		obs_hotkey_register_frontend("uhohbs.dump", obs_module_text("hotkey.dump"), on_dump_hotkey, nullptr);
	obs_log(LOG_INFO, "registered dump hotkey: id=%llu", static_cast<unsigned long long>(gDumpHotkeyId));

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	if (gDumpHotkeyId != OBS_INVALID_HOTKEY_ID) {
		obs_hotkey_unregister(gDumpHotkeyId);
		gDumpHotkeyId = OBS_INVALID_HOTKEY_ID;
	}

	if (gDock) {
		gDock->shutdown();
		obs_frontend_remove_dock("uhohbs_dock");
		delete gDock.data();
		gDock = nullptr;
	}

	if (gShowDockAction) {
		gShowDockAction = nullptr;
	}

	obs_log(LOG_INFO, "plugin unloaded");
}
