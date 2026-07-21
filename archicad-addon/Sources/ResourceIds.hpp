#pragma once

#define ID_ADDON_INFO                   32000
#define ID_ADDON_INFO_NAME                  1
#define ID_ADDON_INFO_DESC                  2

#define ID_ADDON_MENU                   32001
#define ID_ADDON_MENU_ABOUT                 1

#define ID_ADDON_MENU_FOR_PALETTE       32002
#define ID_ADDON_MENU_PALETTE               1

#define ID_ADDON_MENU_FOR_UPDATE        32003
#define ID_ADDON_MENU_UPDATE                1

#define ID_SCRIPTUI_PALETTE             32041

// Each shortcut slot is its own top-level menu group (own resID, exactly one item) — confirmed
// working end to end (menu display AND click dispatch running the assigned script).
#define ID_ADDON_MENU_RUNSCRIPT_1       32020
#define ID_ADDON_MENU_RUNSCRIPT_2       32021
#define ID_ADDON_MENU_RUNSCRIPT_3       32022
#define ID_ADDON_MENU_RUNSCRIPT_4       32023
#define ID_ADDON_MENU_RUNSCRIPT_5       32024
#define ID_ADDON_MENU_RUNSCRIPT_6       32025
#define ID_ADDON_MENU_RUNSCRIPT_ITEM        1

#define ID_SHORTCUTS_DIALOG                  32030
#define ID_SHORTCUTS_DIALOG_POPUP_1               4
#define ID_SHORTCUTS_DIALOG_LABELEDIT_1           5
#define ID_SHORTCUTS_DIALOG_POPUP_2               7
#define ID_SHORTCUTS_DIALOG_LABELEDIT_2           8
#define ID_SHORTCUTS_DIALOG_POPUP_3              10
#define ID_SHORTCUTS_DIALOG_LABELEDIT_3          11
#define ID_SHORTCUTS_DIALOG_POPUP_4              13
#define ID_SHORTCUTS_DIALOG_LABELEDIT_4          14
#define ID_SHORTCUTS_DIALOG_POPUP_5              16
#define ID_SHORTCUTS_DIALOG_LABELEDIT_5          17
#define ID_SHORTCUTS_DIALOG_POPUP_6              19
#define ID_SHORTCUTS_DIALOG_LABELEDIT_6          20
#define ID_SHORTCUTS_DIALOG_CLOSE_BUTTON         21

#define ID_PALETTE_STRINGS              32010
#define ID_PALETTE_STRINGS_SELECT_SCRIPTS_TITLE  1
#define ID_PALETTE_STRINGS_SELECT_SCRIPTS_BUTTON 2

#define ID_AUTOUPDATE_STRINGS           32011
#define ID_AUTOUPDATE_NEWVERSION_ALERT_TITLE 1
#define ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT1 2
#define ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT2 3
#define ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON1 4
#define ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON2 5
#define ID_AUTOUPDATE_LATESTVERSION_ALERT_TITLE  6
#define ID_AUTOUPDATE_LATESTVERSION_ALERT_TEXT   7
#define ID_AUTOUPDATE_LATESTVERSION_ALERT_BUTTON 8

#define ID_BUILTINSCRIPTS_NAMES         32004

#define ID_TAPIR_LOGO                   32502
#define ID_TAPIR_LOGO_MINI              32503
#define ID_RUN_BUTTON_ICON              32504
#define ID_KILL_BUTTON_ICON             32505
#define ID_OPENSCRIPT_BUTTON_ICON       32506
#define ID_ADDSCRIPT_BUTTON_ICON        32507
#define ID_DELSCRIPT_BUTTON_ICON        32508
#define ID_RUNWITHUPDATE_BUTTON_ICON    32509
#define ID_UPDATE_ICON                  32510
#define ID_SHORTCUTS_BUTTON_ICON        32511

#define ID_ABOUT_DIALOG                 32003
#define ID_PALETTE                      32004

#define ID_COMMON_SCHEMA_DEFINITIONS_FILE 32001
#define ID_ACLIB_INIT_PY_FILE             32000