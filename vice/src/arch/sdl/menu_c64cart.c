/*
 * menu_c64cart.c - Implementation of the C64/C128 cartridge settings menu for the SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdlib.h>
#include <string.h>

#include "c64cart.h"
#include "cartridge.h"
#include "keyboard.h"
#include "lib.h"
#include "menu_common.h"
#include "menu_c64_common_expansions.h"
#include "menu_c64cart.h"
#include "resources.h"
#include "ui.h"
#include "uifilereq.h"
#include "uimenu.h"

static UI_MENU_CALLBACK(attach_c64_cart_callback)
{
    char *name = NULL;

    if (activated) {
        name = sdl_ui_file_selection_dialog("Select cartridge image", FILEREQ_MODE_CHOOSE_FILE);
        if (name != NULL) {
            if (cartridge_attach_image(vice_ptr_to_int(param), name) < 0) {
                ui_error("Cannot load cartridge image.");
            }
            lib_free(name);
        }
    }
    return NULL;
}

static const ui_menu_entry_t attach_raw_cart_menu[] = {
    { "Attach generic 8kB image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_GENERIC_8KB },
    { "Attach generic 16kB image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_GENERIC_16KB },
    { "Attach Ultimax image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ULTIMAX },
    { "Attach Action Replay image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ACTION_REPLAY },
    { "Attach Action Replay 2 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ACTION_REPLAY2 },
    { "Attach Action Replay 3 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ACTION_REPLAY3 },
    { "Attach Action Replay 4 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ACTION_REPLAY4 },
    { "Attach Atomic Power image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ATOMIC_POWER },
    { "Attach Capture image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_CAPTURE },
    { "Attach Comal-80 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_COMAL80 },
    { "Attach Diashow Maker image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_DIASHOW_MAKER },
    { "Attach Dinamic image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_DINAMIC },
    { "Attach EasyFlash image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_EASYFLASH },
    { "Attach Epyx Fastload image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_EPYX_FASTLOAD },
    { "Attach ExOS image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_EXOS },
    { "Attach Expert Cartridge image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_EXPERT },
    { "Attach Final Cartridge I image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FINAL_I },
    { "Attach Final Cartridge III image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FINAL_III },
    { "Attach Final Cartridge Plus image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FINAL_PLUS },
    { "Attach Freeze Frame image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FREEZE_FRAME },
    { "Attach Freeze Machine image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FREEZE_MACHINE },
    { "Attach Fun Play image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_FUNPLAY },
    { "Attach Game Killer image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_GAME_KILLER },
    { "Attach GS image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_GS },
    { "Attach IDE64 interface image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_IDE64 },
    { "Attach IEEE488 interface image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_IEEE488 },
    { "Attach KCS Power image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_KCS_POWER },
    { "Attach Mach5 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MACH5 },
    { "Attach Magic Desk image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MAGIC_DESK },
    { "Attach Magic Formel image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MAGIC_FORMEL },
    { "Attach Magic Voice image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MAGIC_VOICE },
    { "Attach Mikro Assembler image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MIKRO_ASSEMBLER },
    { "Attach MMC64 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MMC64 },
    { "Attach MMC Replay image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_MMC_REPLAY },
    { "Attach Ocean image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_OCEAN },
    { "Attach Prophet64 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_P64 },
    { "Attach Retro Replay image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_RETRO_REPLAY },
    { "Attach REX Utility image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_REX },
    { "Attach ROSS image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ROSS },
    { "Attach Simon's Basic image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SIMONS_BASIC },
    { "Attach Snapshot64 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SNAPSHOT64 },
    { "Attach StarDOS image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_STARDOS },
    { "Attach Structured BASIC image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_STRUCTURED_BASIC },
    { "Attach Super Explode V5 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SUPER_EXPLODE_V5 },
    { "Attach Super Games image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SUPER_GAMES },
    { "Attach Super Snapshot 4 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SUPER_SNAPSHOT },
    { "Attach Super Snapshot 5 image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_SUPER_SNAPSHOT_V5 },
    { "Attach Warpspeed image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_WARPSPEED },
    { "Attach Westermann image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_WESTERMANN },
    { "Attach Zaxxon image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_ZAXXON },
    SDL_MENU_LIST_END
};

static UI_MENU_CALLBACK(detach_c64_cart_callback)
{
    if (activated) {
        cartridge_detach_image(-1);
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_freeze_callback)
{
    if (activated) {
        keyboard_clear_keymatrix();
        cartridge_trigger_freeze();
    }
    return NULL;
}

static UI_MENU_CALLBACK(set_c64_cart_default_callback)
{
    if (activated) {
        cartridge_set_default();
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_flush_callback)
{
    if (activated) {
        int cartid = vice_ptr_to_int(param);

        if (cartridge_flush_image(cartid) < 0) {
            ui_error("Cannot save cartridge image.");
        }
    }
    return NULL;
}

static UI_MENU_CALLBACK(c64_cart_save_callback)
{
    if (activated) {
        int cartid = vice_ptr_to_int(param);
        char *name = NULL;

        name = sdl_ui_file_selection_dialog("Choose cartridge file", FILEREQ_MODE_SAVE_FILE);

        if (name != NULL) {
            if (cartridge_save_image(cartid, name) < 0) {
                ui_error("Cannot save cartridge image.");
            }
            lib_free(name);
        }

    }
    return NULL;
}


/* RAMCART */

UI_MENU_DEFINE_TOGGLE(RAMCART)
UI_MENU_DEFINE_TOGGLE(RAMCART_RO)
UI_MENU_DEFINE_RADIO(RAMCARTsize)
UI_MENU_DEFINE_FILE_STRING(RAMCARTfilename)
UI_MENU_DEFINE_TOGGLE(RAMCARTImageWrite)

static const ui_menu_entry_t ramcart_menu[] = {
    { "Enable RAMCART",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RAMCART_callback,
      NULL },
    { "RAMCART read-only",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RAMCART_RO_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Memory size"),
    { "64kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_RAMCARTsize_callback,
      (ui_callback_data_t)64 },
    { "128kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_RAMCARTsize_callback,
      (ui_callback_data_t)128 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "RAMCART image file",
      MENU_ENTRY_DIALOG,
      file_string_RAMCARTfilename_callback,
      (ui_callback_data_t)"Select RAMCART image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RAMCARTImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_RAMCART },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_RAMCART },
    SDL_MENU_LIST_END
};


/* REU */

UI_MENU_DEFINE_TOGGLE(REU)
UI_MENU_DEFINE_RADIO(REUsize)
UI_MENU_DEFINE_FILE_STRING(REUfilename)
UI_MENU_DEFINE_TOGGLE(REUImageWrite)

static const ui_menu_entry_t reu_menu[] = {
    { "Enable REU",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_REU_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Memory size"),
    { "128kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)128 },
    { "256kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)256 },
    { "512kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)512 },
    { "1024kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)1024 },
    { "2048kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)2048 },
    { "4096kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)4096 },
    { "8192kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)8192 },
    { "16384kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_REUsize_callback,
      (ui_callback_data_t)16384 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "REU image file",
      MENU_ENTRY_DIALOG,
      file_string_REUfilename_callback,
      (ui_callback_data_t)"Select REU image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_REUImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_REU },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_REU },
    SDL_MENU_LIST_END
};


/* Expert cartridge */

UI_MENU_DEFINE_TOGGLE(ExpertCartridgeEnabled)
UI_MENU_DEFINE_RADIO(ExpertCartridgeMode)
UI_MENU_DEFINE_FILE_STRING(Expertfilename)
UI_MENU_DEFINE_TOGGLE(ExpertImageWrite)

static const ui_menu_entry_t expert_cart_menu[] = {
    { "Enable Expert Cartridge",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_ExpertCartridgeEnabled_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Expert Cartridge mode"),
    { "Off",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_ExpertCartridgeMode_callback,
      (ui_callback_data_t)EXPERT_MODE_OFF },
    { "Prg",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_ExpertCartridgeMode_callback,
      (ui_callback_data_t)EXPERT_MODE_PRG },
    { "On",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_ExpertCartridgeMode_callback,
      (ui_callback_data_t)EXPERT_MODE_ON },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "Expert Cartridge image file",
      MENU_ENTRY_DIALOG,
      file_string_Expertfilename_callback,
      (ui_callback_data_t)"Select Expert image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_ExpertImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_EXPERT },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_EXPERT },
    SDL_MENU_LIST_END
};


/* Double Quick Brown Box */

UI_MENU_DEFINE_TOGGLE(DQBB)
UI_MENU_DEFINE_FILE_STRING(DQBBfilename)
UI_MENU_DEFINE_TOGGLE(DQBBImageWrite)

static const ui_menu_entry_t dqbb_cart_menu[] = {
    { "Enable DQBB",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_DQBB_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "DQBB image file",
      MENU_ENTRY_DIALOG,
      file_string_DQBBfilename_callback,
      (ui_callback_data_t)"Select DQBB image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_DQBBImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_DQBB },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_DQBB },
    SDL_MENU_LIST_END
};


/* ISEPIC */

UI_MENU_DEFINE_TOGGLE(IsepicCartridgeEnabled)
UI_MENU_DEFINE_TOGGLE(IsepicSwitch)
UI_MENU_DEFINE_FILE_STRING(Isepicfilename)
UI_MENU_DEFINE_TOGGLE(IsepicImageWrite)

static const ui_menu_entry_t isepic_cart_menu[] = {
    { "Enable ISEPIC cart",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_IsepicCartridgeEnabled_callback,
      NULL },
    { "Switch",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_IsepicSwitch_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "ISEPIC image file",
      MENU_ENTRY_DIALOG,
      file_string_Isepicfilename_callback,
      (ui_callback_data_t)"Select ISEPIC image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_IsepicImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_ISEPIC },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_ISEPIC },
    SDL_MENU_LIST_END
};


/* EasyFlash */

UI_MENU_DEFINE_TOGGLE(EasyFlashJumper)
UI_MENU_DEFINE_TOGGLE(EasyFlashWriteCRT)


static const ui_menu_entry_t easyflash_cart_menu[] = {
    { "Jumper",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_EasyFlashJumper_callback,
      NULL },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_EasyFlashWriteCRT_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_EASYFLASH },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_EASYFLASH },
    SDL_MENU_LIST_END
};


/* GEORAM */

UI_MENU_DEFINE_TOGGLE(GEORAM)
UI_MENU_DEFINE_RADIO(GEORAMsize)
UI_MENU_DEFINE_FILE_STRING(GEORAMfilename)
UI_MENU_DEFINE_TOGGLE(GEORAMImageWrite)

static const ui_menu_entry_t georam_menu[] = {
    { "Enable GEORAM",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_GEORAM_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Memory size"),
    { "64kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)64 },
    { "128kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)128 },
    { "256kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)256 },
    { "512kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)512 },
    { "1024kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)1024 },
    { "2048kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)2048 },
    { "4096kB",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_GEORAMsize_callback,
      (ui_callback_data_t)4096 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("RAM image"),
    { "GEORAM image file",
      MENU_ENTRY_DIALOG,
      file_string_GEORAMfilename_callback,
      (ui_callback_data_t)"Select GEORAM image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_GEORAMImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_GEORAM },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_GEORAM },
    SDL_MENU_LIST_END
};


/* MMC64 */

UI_MENU_DEFINE_RADIO(MMC64_sd_type)

static const ui_menu_entry_t mmc64_sd_type_menu[] = {
    { "Auto",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_sd_type_callback,
      (ui_callback_data_t)0 },
    { "MMC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_sd_type_callback,
      (ui_callback_data_t)1 },
    { "SD",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_sd_type_callback,
      (ui_callback_data_t)2 },
    { "SDHC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_sd_type_callback,
      (ui_callback_data_t)3 },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_TOGGLE(MMC64)
UI_MENU_DEFINE_RADIO(MMC64_revision)
UI_MENU_DEFINE_TOGGLE(MMC64_flashjumper)
UI_MENU_DEFINE_TOGGLE(MMC64_bios_write)
UI_MENU_DEFINE_FILE_STRING(MMC64BIOSfilename)
UI_MENU_DEFINE_TOGGLE(MMC64_RO)
UI_MENU_DEFINE_FILE_STRING(MMC64imagefilename)

static const ui_menu_entry_t mmc64_cart_menu[] = {
    { "Enable MMC64",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMC64_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMC64 revision"),
    { "Rev A",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_revision_callback,
      (ui_callback_data_t)0 },
    { "Rev B",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMC64_revision_callback,
      (ui_callback_data_t)1 },
    SDL_MENU_ITEM_SEPARATOR,
    { "BIOS flash jumper",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMC64_flashjumper_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMC64 BIOS image"),
    { "MMC64 BIOS image file",
      MENU_ENTRY_DIALOG,
      file_string_MMC64BIOSfilename_callback,
      (ui_callback_data_t)"Select MMC64 BIOS image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMC64_bios_write_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_MMC64 },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_MMC64 },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMC64 MMC/SD image"),
    { "MMC64 MMC/SD image file",
      MENU_ENTRY_DIALOG,
      file_string_MMC64imagefilename_callback,
      (ui_callback_data_t)"Select MMC64 MMC/SD image" },
    { "MMC64 image read-only",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMC64_RO_callback,
      NULL },
    { "Card type",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)mmc64_sd_type_menu },
    SDL_MENU_LIST_END
};


/* MMC Replay */

UI_MENU_DEFINE_RADIO(MMCRSDType)

static const ui_menu_entry_t mmcreplay_sd_type_menu[] = {
    { "Auto",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMCRSDType_callback,
      (ui_callback_data_t)0 },
    { "MMC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMCRSDType_callback,
      (ui_callback_data_t)1 },
    { "SD",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMCRSDType_callback,
      (ui_callback_data_t)2 },
    { "SDHC",
      MENU_ENTRY_RESOURCE_RADIO,
      radio_MMCRSDType_callback,
      (ui_callback_data_t)3 },
    SDL_MENU_LIST_END
};

UI_MENU_DEFINE_FILE_STRING(MMCRCardImage)
UI_MENU_DEFINE_FILE_STRING(MMCREEPROMImage)
UI_MENU_DEFINE_TOGGLE(MMCRCardRW)
UI_MENU_DEFINE_TOGGLE(MMCREEPROMRW)
UI_MENU_DEFINE_TOGGLE(MMCRRescueMode)
UI_MENU_DEFINE_TOGGLE(MMCRImageWrite)

static const ui_menu_entry_t mmcreplay_cart_menu[] = {
    { "Rescue mode",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMCRRescueMode_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMCR BIOS image"),
    { "MMCR BIOS image file",
      MENU_ENTRY_DIALOG,
      file_string_MMC64BIOSfilename_callback,
      (ui_callback_data_t)"Select MMC64 BIOS image" },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMCRImageWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_MMC_REPLAY },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_MMC_REPLAY },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMCR EEPROM image"),
    { "EEPROM image file",
      MENU_ENTRY_DIALOG,
      file_string_MMCREEPROMImage_callback,
      (ui_callback_data_t)"Select MMC Replay EEPROM image" },
    { "Enable writes to EEPROM image",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMCREEPROMRW_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("MMCR MMC/SD image"),
    { "Card image file",
      MENU_ENTRY_DIALOG,
      file_string_MMCRCardImage_callback,
      (ui_callback_data_t)"Select MMC Replay card image" },
    { "Enable writes to card image",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MMCRCardRW_callback,
      NULL },
    { "Card type",
      MENU_ENTRY_SUBMENU,
      submenu_radio_callback,
      (ui_callback_data_t)mmcreplay_sd_type_menu },
    SDL_MENU_LIST_END
};


/* Retro Replay */

UI_MENU_DEFINE_TOGGLE(RRBankJumper)
UI_MENU_DEFINE_TOGGLE(RRFlashJumper)
UI_MENU_DEFINE_TOGGLE(RRBiosWrite)

static const ui_menu_entry_t retroreplay_cart_menu[] = {
    { "Bank jumper",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RRBankJumper_callback,
      NULL },
    { "BIOS flash jumper",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RRFlashJumper_callback,
      NULL },
    { "Save image on detach",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_RRBiosWrite_callback,
      NULL },
    { "Save image now",
      MENU_ENTRY_OTHER,
      c64_cart_flush_callback,
      (ui_callback_data_t)CARTRIDGE_RETRO_REPLAY },
    { "Save image as",
      MENU_ENTRY_OTHER,
      c64_cart_save_callback,
      (ui_callback_data_t)CARTRIDGE_RETRO_REPLAY },
    SDL_MENU_LIST_END
};


/* Magic Voice */

UI_MENU_DEFINE_TOGGLE(MagicVoiceCartridgeEnabled)
UI_MENU_DEFINE_FILE_STRING(MagicVoiceImage)

static const ui_menu_entry_t magicvoice_cart_menu[] = {
    { "Enable Magic Voice",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_MagicVoiceCartridgeEnabled_callback,
      NULL },
    { "ROM image file",
      MENU_ENTRY_DIALOG,
      file_string_MagicVoiceImage_callback,
      (ui_callback_data_t)"Select Magic Voice ROM image" },
    SDL_MENU_LIST_END
};


/* Cartridge menu */

UI_MENU_DEFINE_TOGGLE(CartridgeReset)

const ui_menu_entry_t c64cart_menu[] = {
    { "Attach CRT image",
      MENU_ENTRY_DIALOG,
      attach_c64_cart_callback,
      (ui_callback_data_t)CARTRIDGE_CRT },
    { "Attach raw image",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)attach_raw_cart_menu },
    { "Detach cartridge image",
      MENU_ENTRY_OTHER,
      detach_c64_cart_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    { "Cartridge freeze",
      MENU_ENTRY_OTHER,
      c64_cart_freeze_callback,
      NULL },
    { "Set current cartridge as default",
      MENU_ENTRY_OTHER,
      set_c64_cart_default_callback,
      NULL },
    { "Reset on cartridge change",
      MENU_ENTRY_RESOURCE_TOGGLE,
      toggle_CartridgeReset_callback,
      NULL },
    SDL_MENU_ITEM_SEPARATOR,
    SDL_MENU_ITEM_TITLE("Cartridge specific settings"),
    { "RAMCART",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ramcart_menu },
    { "REU",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)reu_menu },
    { "GEORAM",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)georam_menu },
    { "IDE64",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)ide64_menu },
    { "Expert",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)expert_cart_menu },
    { "ISEPIC",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)isepic_cart_menu },
    { "Double Quick Brown Box",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)dqbb_cart_menu },
    { "EasyFlash",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)easyflash_cart_menu },
    { "MMC64",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)mmc64_cart_menu },
    { "MMC Replay",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)mmcreplay_cart_menu },
    { "Retro Replay",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)retroreplay_cart_menu },
    { "Magic Voice",
      MENU_ENTRY_SUBMENU,
      submenu_callback,
      (ui_callback_data_t)magicvoice_cart_menu },
    SDL_MENU_LIST_END
};
