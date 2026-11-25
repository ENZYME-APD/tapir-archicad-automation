#pragma once

#include "UniString.hpp"
#include "Location.hpp"
#include "ScriptInfo.hpp"

#include <functional>

// A script of the Tapir Palette's list, as the Script Manager's Scripts tab shows it.
struct LocalScript
{
    IO::Location  fileLocation;
    GS::UniString name;          // the docstring's title, or the file name when it has none
    GS::UniString fileName;
    GS::UniString source;        // Built-in, Marketplace, My scripts, or the folder's or repository's name
    GS::UniString webUrl;        // the script's page on GitHub for a repository's script; empty for a local file
    bool          isAdded = false;     // added with the Add script button (or dropped on the list): Remove removes it
    ScriptInfo    info;
};

// What the Script Manager asks of the Tapir Palette.
struct LocalScriptActions
{
    std::function<GS::Array<LocalScript> ()>                  getScripts;
    std::function<bool (const IO::Location&)>                 isRunning;     // the script runs now
    std::function<void (const IO::Location&)>                 run;           // starts it, as the bar's Run button does
    std::function<void (const IO::Location&)>                 stop;          // stops it with the processes it started
    std::function<void (const GS::UniString&)>                openWebpage;   // opens the URL in the browser
    std::function<void ()>                                    openShortcuts; // the Shortcuts dialog
    std::function<GS::UniString (const IO::Location&)>        shortcutKeys;  // the keys of the script's shortcut slot, e.g. "Ctrl+Alt+H"; empty = none
    std::function<void (const IO::Location&)>                 editFile;      // opens the file in a text editor
    std::function<void (const IO::Location&)>                 showInFolder;  // opens the file's folder
    std::function<void ()>                                    openScriptsFolder;   // opens the custom-scripts folder, made when missing
    std::function<void (const IO::Location&)>                 remove;        // removes a script the user added
    std::function<void (const GS::Array<IO::Location>&)>      addScripts;    // adds the files, as the Add script button does
    std::function<void ()>                                    addScriptWithDialog;   // asks for the files, then adds them
};
