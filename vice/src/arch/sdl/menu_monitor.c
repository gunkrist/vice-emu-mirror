/*
 * menu_monitor.c - Monitor menu for SDL UI.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "menu_common.h"
#include "menu_monitor.h"
#include "monitor.h"
#include "types.h"
#include "ui.h"
#include "uimenu.h"

static UI_MENU_CALLBACK(monitor_callback)
{
    if (activated) {
        if (sdl_menu_state) {
            monitor_startup(e_default_space);
        } else {
            /* The monitor was activated with a hotkey.
               In this case, the trap is needed for the machine state to be
               properly imported. */
            monitor_startup_trap();
        }
        return sdl_menu_text_exit_ui;
    }
    return NULL;
}

#ifdef ALLOW_NATIVE_MONITOR
UI_MENU_DEFINE_TOGGLE(NativeMonitor)
#endif

#ifdef HAVE_NETWORK
UI_MENU_DEFINE_TOGGLE(MonitorServer)
UI_MENU_DEFINE_STRING(MonitorServerAddress)
UI_MENU_DEFINE_TOGGLE(BinaryMonitorServer)
UI_MENU_DEFINE_STRING(BinaryMonitorServerAddress)
#endif

UI_MENU_DEFINE_TOGGLE(MonitorLogEnabled)
UI_MENU_DEFINE_STRING(MonitorLogFileName)

#ifdef FEATURE_CPUMEMHISTORY
UI_MENU_DEFINE_INT(MonitorChisLines)
#endif

const ui_menu_entry_t monitor_menu[] = {
    { "Start monitor",
      MENU_ENTRY_OTHER,
      monitor_callback,
      NULL },
#ifdef ALLOW_NATIVE_MONITOR
    { "Native monitor",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_NativeMonitor_callback,
      NULL },
#endif
#ifdef HAVE_NETWORK
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Remote monitor"),
    { "Enable remote monitor",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MonitorServer_callback,
      NULL },
    { "Monitor address",
      MENU_ENTRY_RESOURCE_STRING,
      string_MonitorServerAddress_callback,
      (ui_callback_data_t)"Set remote monitor server address" },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Binary remote monitor"),
    { "Enable binary remote monitor",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_BinaryMonitorServer_callback,
      NULL },
    { "Monitor address",
      MENU_ENTRY_RESOURCE_STRING,
      string_BinaryMonitorServerAddress_callback,
      (ui_callback_data_t)"Set remote binary monitor server address" },
#endif
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Logging"),
    { "Enable logging to a file",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MonitorLogEnabled_callback,
      NULL },
    { "Logfile name",
      MENU_ENTRY_RESOURCE_STRING,
      string_MonitorLogFileName_callback,
      (ui_callback_data_t)"Set monitor logfile" },
#ifdef FEATURE_CPUMEMHISTORY
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("CPU History"),
    { "Number of lines",
      MENU_ENTRY_RESOURCE_INT,
      int_MonitorChisLines_callback,
      (ui_callback_data_t)"Set number of lines in CPU history" },
#endif
    SDL_MENU_LIST_END
};

