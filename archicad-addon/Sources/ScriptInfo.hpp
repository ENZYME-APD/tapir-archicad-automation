#pragma once

#include "UniString.hpp"
#include "Array.hpp"
#include "Location.hpp"

#include <string>

// What a palette script says about itself: the first paragraph of its module docstring and the keys of its
// `# /// tapir` header, read like the `# /// script` block of PEP 723 (TOML lines behind "# "):
//
//     # /// tapir
//     # category = "element-data-properties"
//     # archicad-versions = [28, 29, 30]
//     # min-tapir-version = "1.8.1"
//     # changes = "model"
//     # ///
//
// A missing or malformed header, or a value of the wrong type, is never an error: the key is just left empty.
struct ScriptInfo
{
    GS::UniString title;              // the docstring's first paragraph when it is a single short line followed by more text
    GS::UniString summary;            // the docstring's first paragraph after the title, its lines joined by spaces
    bool hasHeader = false;           // a well-formed `# /// tapir` block was found
    GS::UniString category;
    GS::Array<Int32> archicadVersions;
    GS::UniString minTapirVersion;
    GS::UniString changes;            // "model", "files" or "read-only"; empty when missing or anything else

    bool ChangesModel () const { return changes == "model"; }
    bool WritesFiles () const  { return changes == "files"; }
    bool IsReadOnly () const   { return changes == "read-only"; }
};

ScriptInfo ParseScriptInfo (const std::string& utf8Text);
ScriptInfo ReadScriptInfo (const IO::Location& scriptFile);   // reads only .py files; an empty ScriptInfo for others
