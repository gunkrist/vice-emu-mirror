/*
 * uiedisk.c - emptydisk dialog for the Xaw widget set.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>

#ifndef ENABLE_TEXTFIELD
#include <X11/Xaw/AsciiText.h>
#else
#include "widgets/TextField.h"
#endif

#include "diskimage.h"
#include "machine.h"
#include "ui.h"
#include "uiedisk.h"
#include "utils.h"
#include "vdrive.h"

static Widget emptydisk_dialog;
static Widget emptydisk_dialog_pane;
static Widget file_name_form;
static Widget file_name_label;
static Widget file_name_field;
static Widget image_name_form;
static Widget image_name_label;
static Widget image_name_field;
static Widget browse_button;
static Widget options_form;
static Widget options_filling_box_left;

/* static Widget options_filling_box_right; */
static Widget disk_type_d64_button, disk_type_d67_button;
static Widget disk_type_d71_button;
static Widget disk_type_d81_button, disk_type_d80_button;
static Widget disk_type_d82_button, disk_type_g64_button;
static Widget disk_type_label;

static Widget button_box;
static Widget save_button;
static Widget cancel_button;

static char *edisk_file_name;

#define FILL_BOX_WIDTH          10
#define OPTION_LABELS_WIDTH     50
#define OPTION_LABELS_JUSTIFY   XtJustifyLeft

static UI_CALLBACK(browse_callback)
{
    ui_button_t button;
    char *filename;

    filename = ui_select_file(_("Save emptydisk file"), NULL, False, NULL, "*",
                              &button, 0, NULL);

    if (button == UI_BUTTON_OK)
        XtVaSetValues(file_name_field, XtNstring, filename, NULL);

    if (filename != NULL)
        free(filename);
}

static UI_CALLBACK(cancel_callback)
{
    ui_popdown(emptydisk_dialog);
}

static char *extensions[] = { "d64", "d67", "d71", "d81", "d80", "d82", "g64" };

static UI_CALLBACK(save_callback)
{
    int dtypes[] = { DISK_IMAGE_TYPE_D64, DISK_IMAGE_TYPE_D67,
    	             DISK_IMAGE_TYPE_D71,
                     DISK_IMAGE_TYPE_D81, DISK_IMAGE_TYPE_D80,
                     DISK_IMAGE_TYPE_D82, DISK_IMAGE_TYPE_G64 };
    char *filename;
    String name;
    String iname;
    int type_cnt;
    Boolean disk_type_flag;
    
    ui_popdown(emptydisk_dialog);

    type_cnt = 0;
    XtVaGetValues(disk_type_d64_button, XtNstate, &disk_type_flag, NULL);
    if (disk_type_flag == False) {
      type_cnt ++;
      XtVaGetValues(disk_type_d67_button, XtNstate, &disk_type_flag, NULL);
      if (disk_type_flag == False) {
        XtVaGetValues(disk_type_d71_button, XtNstate, &disk_type_flag, NULL);
        if (disk_type_flag == False) {
	    type_cnt ++;
            XtVaGetValues(disk_type_d81_button, XtNstate, 
							&disk_type_flag, NULL);
            if (disk_type_flag == False) {
	        type_cnt ++;
                XtVaGetValues(disk_type_d80_button, XtNstate, 
							&disk_type_flag, NULL);
                if (disk_type_flag == False) {
	            type_cnt ++;
                    XtVaGetValues(disk_type_d82_button, XtNstate, 
							&disk_type_flag, NULL);
                    if (disk_type_flag == False) {
	                type_cnt ++;
                        XtVaGetValues(disk_type_g64_button, XtNstate,
                                                        &disk_type_flag, NULL);
                        if (disk_type_flag == False) {
                            type_cnt ++;
                        }
		    }
		}
	    }
	}
      }
    }
  
    if (type_cnt < 0 || type_cnt > 5)
	return;
 
    XtVaGetValues(file_name_field, XtNstring, &name, NULL);
    XtVaGetValues(image_name_field, XtNstring, &iname, NULL);

    filename = stralloc(name);
    xadd_extension(&filename, extensions[type_cnt]);

    if (vdrive_internal_create_format_disk_image(filename, "VICE,01",
                                                 dtypes[type_cnt]) < 0)
        ui_error(_("Couldn't create disk image"));
    else
        strcpy(edisk_file_name, filename);

    free(filename);
}

static void build_emptydisk_dialog(void)
{
#ifndef ENABLE_TEXTFIELD
    static char *text_box_translations = "#override\n<Key>Return: no-op()";
#else
    static char *text_box_translations = "<Btn1Down>: select-start() focus-in()";
#endif

    if (emptydisk_dialog != NULL)
        return;

    emptydisk_dialog = ui_create_transient_shell(_ui_top_level,
                                                "emptydiskDialog");

    emptydisk_dialog_pane = XtVaCreateManagedWidget
        ("emptydiskDialogPane",
         panedWidgetClass, emptydisk_dialog,
         NULL);
    
    file_name_form = XtVaCreateManagedWidget
        ("fileNameForm",
         formWidgetClass, emptydisk_dialog_pane,
         XtNshowGrip, False, NULL);
    
    file_name_label = XtVaCreateManagedWidget
        ("fileNameLabel",
         labelWidgetClass, file_name_form,
         XtNjustify, XtJustifyLeft,
         XtNlabel, _("File name:"),
         XtNborderWidth, 0,
         NULL);

#ifndef ENABLE_TEXTFIELD
    file_name_field = XtVaCreateManagedWidget
        ("fileNameField",
         asciiTextWidgetClass, file_name_form,
         XtNfromHoriz, file_name_label,
         XtNwidth, 240,
         XtNtype, XawAsciiString,
         XtNeditType, XawtextEdit,
         NULL);
#else
    file_name_field = XtVaCreateManagedWidget
        ("fileNameField",
         textfieldWidgetClass, file_name_form,
         XtNfromHoriz, file_name_label,
         XtNwidth, 240,
         XtNstring, "",         /* Otherwise, it does not work correctly.  */
         NULL);
#endif
    XtOverrideTranslations(file_name_field,
                               XtParseTranslationTable(text_box_translations));

    browse_button = XtVaCreateManagedWidget
        ("browseButton",
         commandWidgetClass, file_name_form,
         XtNfromHoriz, file_name_field,
         XtNlabel, _("Browse..."),
         NULL);
    XtAddCallback(browse_button, XtNcallback, browse_callback, NULL);

    image_name_form =
	XtVaCreateManagedWidget
        ("imageNameForm",
         formWidgetClass, emptydisk_dialog_pane,
         XtNfromVert, file_name_form,
         XtNshowGrip, False, NULL);
    
    image_name_label = XtVaCreateManagedWidget
        ("imageNameLabel",
         labelWidgetClass, image_name_form,
/*
         XtNfromVert, file_name_form,
*/
         XtNjustify, XtJustifyLeft,
         XtNlabel, _("Disk name:"),
         XtNborderWidth, 0,
         NULL);

#ifndef ENABLE_TEXTFIELD
    image_name_field = XtVaCreateManagedWidget
        ("imageNameField",
         asciiTextWidgetClass, image_name_form,
         XtNfromHoriz, image_name_label,
/*
         XtNfromVert, file_name_form,
*/
         XtNwidth, 100,
         XtNtype, XawAsciiString,
         XtNeditType, XawtextEdit,
         NULL);
#else
    image_name_field = XtVaCreateManagedWidget
        ("imageNameField",
         textfieldWidgetClass, image_name_form,
         XtNfromHoriz, image_name_label,
/*
         XtNfromVert, file_name_form,
*/
         XtNwidth, 100,
         XtNstring, "",         /* Otherwise, it does not work correctly.  */
         NULL);
#endif
    XtOverrideTranslations(image_name_field,
                               XtParseTranslationTable(text_box_translations));

    options_form = XtVaCreateManagedWidget
        ("optionsForm",
         formWidgetClass, emptydisk_dialog_pane,
         XtNskipAdjust, True,
         NULL);

    disk_type_label = XtVaCreateManagedWidget
        ("ImageTypeLabel",
         labelWidgetClass, options_form,
         XtNborderWidth, 0,
         XtNfromHoriz, options_filling_box_left,
         XtNjustify, OPTION_LABELS_JUSTIFY,
         XtNwidth, OPTION_LABELS_WIDTH,
         XtNleft, XawChainLeft,
         XtNright, XawChainRight,
         XtNheight, 20,
         XtNlabel, _("Disk format:"),
         NULL);

    disk_type_d64_button = XtVaCreateManagedWidget
        ("ImageTypeD64Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_label,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D64",
         NULL);

    disk_type_d67_button = XtVaCreateManagedWidget
        ("ImageTypeD67Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d64_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D67",
         NULL);

    disk_type_d71_button = XtVaCreateManagedWidget
        ("ImageTypeD71Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d67_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D71",
         XtNradioGroup, disk_type_d64_button,
         NULL);

    disk_type_d81_button = XtVaCreateManagedWidget
        ("ImageTypeD81Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d71_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D81",
         XtNradioGroup, disk_type_d64_button,
         NULL);

    disk_type_d80_button = XtVaCreateManagedWidget
        ("ImageTypeD80Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d81_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D80",
         XtNradioGroup, disk_type_d64_button,
         NULL);

    disk_type_d82_button = XtVaCreateManagedWidget
        ("ImageTypeD82Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d80_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "D82",
         XtNradioGroup, disk_type_d64_button,
         NULL);

    disk_type_g64_button = XtVaCreateManagedWidget
        ("ImageTypeG64Button",
         toggleWidgetClass, options_form,
         XtNfromHoriz, disk_type_d82_button,
         XtNfromVert, browse_button,
         XtNwidth, 40,
         XtNheight, 20,
         XtNright, XtChainRight,
         XtNleft, XtChainRight,
         XtNlabel, "G64",
         XtNradioGroup, disk_type_d64_button,
         NULL);

    button_box = XtVaCreateManagedWidget
        ("buttonBox",
         boxWidgetClass, emptydisk_dialog_pane,
         XtNshowGrip, False, NULL);

    save_button = XtVaCreateManagedWidget
        ("saveButton",
         commandWidgetClass, button_box,
         XtNlabel, _("Save"),
         NULL);
    XtAddCallback(save_button, XtNcallback, save_callback, NULL);
    
    cancel_button = XtVaCreateManagedWidget
        ("cancelButton",
         commandWidgetClass, button_box,
         XtNlabel, _("Cancel"),
         NULL);
    XtAddCallback(cancel_button, XtNcallback, cancel_callback, NULL);

    XtVaSetValues(disk_type_d64_button, XtNstate, True, NULL);
/*
    XtVaSetValues(save_disk_off_button, XtNstate, True, NULL);
*/
    XtSetKeyboardFocus(emptydisk_dialog_pane, file_name_field);
}

int ui_empty_disk_dialog(char *name)
{
    edisk_file_name = name;
    *edisk_file_name= 0;
    
    build_emptydisk_dialog();
    ui_popup(emptydisk_dialog, _("Create empty disk"), True);
    return *name ? 0 : -1;
}

