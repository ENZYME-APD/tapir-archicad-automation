#pragma once

#include "ObjectState.hpp"
#include "UniString.hpp"
#include "Optional.hpp"

#include <memory>

// Unknown keys inside the shared schema definitions (#2623).
//
// Archicad checks every request against the command's input schema before Execute runs, and it
// refuses a key that an object of the schema does not list ("additionalProperties": false), but only
// in the objects written inline in the command's schema. It does not apply the rules of a shared
// definition reached through "$ref": "#/<Name>" (CommonSchemaDefinitions.json), so a key a definition
// does not list was accepted, and the command reported success without writing it.
//
// This check is the missing part, and only that: it walks the parameters along the input schema,
// following the $refs into the common definitions, and finds the first key that an object with
// "additionalProperties": false does not list. It checks nothing else (types, required keys and
// enums stay as Archicad checks them), and whenever it is not sure, it lets the key pass:
//  - a key listed by any branch of a "oneOf" or "anyOf" counts as listed, and the object is refused
//    only when every branch refuses a key;
//  - an object whose schema has no "additionalProperties": false, a value of another type than the
//    schema expects, a list of things other than objects and anything deeper than 64 levels are
//    not checked;
//  - a schema or a definition that cannot be read switches the check off for the command.

class InputKeyChecker
{
public:
    // inputSchema is the command's input schema (GetInputParametersSchema). A schema without any
    // "$ref" is not checked: Archicad checks all of it.
    explicit InputKeyChecker (const GS::UniString& inputSchema);
    ~InputKeyChecker ();

    // The error message naming the first unknown key and its place, e.g. "Unknown key 'fooBar' at
    // #/meshesData/0/meshData: the schema of this object does not list it."; no value when there is none.
    GS::Optional<GS::UniString> FindUnknownKey (const GS::ObjectState& parameters) const;

    class Node;

private:
    std::shared_ptr<const Node> mSchema;
};
