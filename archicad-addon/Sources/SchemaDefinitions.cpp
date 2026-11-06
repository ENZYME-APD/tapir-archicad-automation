#include "SchemaDefinitions.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "RS.hpp"
#include "ResourceIds.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

static GS::UniString GetResourceFileContent (short resId)
{
    GSHandle gsHandle = RSLoadResource ('FILE', ACAPI_GetOwnResModule (), resId);
    GS::UniString fileContent (*gsHandle, BMGetHandleSize (gsHandle));
    BMKillHandle (&gsHandle);
    return fileContent;
}

const GS::UniString& GetCommonSchemaDefinitions ()
{
    static const GS::UniString commonSchemaDefinitions = GetResourceFileContent (ID_COMMON_SCHEMA_DEFINITIONS_FILE);
    return commonSchemaDefinitions;
}

namespace {

// Walks a JSON text and collects the definition names of its "$ref": "#/<Name>" references. Any
// JSON it cannot read, or a reference it cannot be sure of, makes it fail, and the caller then falls
// back to the whole definitions file.
class SchemaReferenceScanner
{
public:
    explicit SchemaReferenceScanner (const GS::UniString& text) :
        mText (text),
        mLength (text.GetLength ()),
        mPos (0)
    {
    }

    UIndex GetPosition () const
    {
        return mPos;
    }

    void SkipWhitespace ()
    {
        while (mPos < mLength && IsWhitespace (At (mPos))) {
            ++mPos;
        }
    }

    bool IsAtEnd ()
    {
        SkipWhitespace ();
        return mPos >= mLength;
    }

    bool Consume (char expected)
    {
        SkipWhitespace ();
        if (mPos < mLength && At (mPos) == static_cast<Char> (expected)) {
            ++mPos;
            return true;
        }
        return false;
    }

    // Reads a string. content gets its characters when they are all ASCII and none is escaped;
    // hasEscape tells whether one is escaped, isAscii whether all are ASCII.
    bool ScanString (std::string& content, bool& hasEscape, bool& isAscii)
    {
        content.clear ();
        hasEscape = false;
        isAscii = true;
        if (!Consume ('"')) {
            return false;
        }
        while (mPos < mLength) {
            const Char c = At (mPos++);
            if (c == static_cast<Char> ('"')) {
                return true;
            }
            if (c == static_cast<Char> ('\\')) {
                hasEscape = true;
                if (mPos >= mLength) {
                    return false;
                }
                ++mPos;
            } else if (c > 127) {
                isAscii = false;
            } else {
                content.push_back (static_cast<char> (c));
            }
        }
        return false;
    }

    bool ScanValue (std::vector<std::string>& refs)
    {
        SkipWhitespace ();
        if (mPos >= mLength) {
            return false;
        }
        const Char c = At (mPos);
        if (c == static_cast<Char> ('{')) {
            return ScanObject (refs);
        } else if (c == static_cast<Char> ('[')) {
            return ScanArray (refs);
        } else if (c == static_cast<Char> ('"')) {
            std::string ignored;
            bool hasEscape;
            bool isAscii;
            return ScanString (ignored, hasEscape, isAscii);
        } else {
            return ScanLiteral ();
        }
    }

private:
    using Char = GS::UniChar::Layout;

    Char At (UIndex index) const
    {
        return static_cast<Char> (mText[index]);
    }

    static bool IsWhitespace (Char c)
    {
        // A resource may end in zero bytes; they are not part of the JSON.
        return c == static_cast<Char> (' ') || c == static_cast<Char> ('\t') || c == static_cast<Char> ('\n') ||
               c == static_cast<Char> ('\r') || c == 0;
    }

    bool ScanObject (std::vector<std::string>& refs)
    {
        if (!Consume ('{')) {
            return false;
        }
        if (Consume ('}')) {
            return true;
        }
        do {
            std::string key;
            bool keyHasEscape;
            bool keyIsAscii;
            if (!ScanString (key, keyHasEscape, keyIsAscii) || !Consume (':')) {
                return false;
            }
            if (keyHasEscape) {
                // It could be an escaped "$ref".
                return false;
            }
            if (keyIsAscii && key == "$ref") {
                std::string target;
                bool targetHasEscape;
                bool targetIsAscii;
                if (!ScanString (target, targetHasEscape, targetIsAscii) || targetHasEscape || !targetIsAscii) {
                    return false;
                }
                if (target.size () <= 2 || target.compare (0, 2, "#/") != 0) {
                    return false;
                }
                std::string name = target.substr (2);
                if (name.find ('/') != std::string::npos || name.find ('~') != std::string::npos) {
                    return false;
                }
                refs.push_back (name);
            } else if (!ScanValue (refs)) {
                return false;
            }
        } while (Consume (','));
        return Consume ('}');
    }

    bool ScanArray (std::vector<std::string>& refs)
    {
        if (!Consume ('[')) {
            return false;
        }
        if (Consume (']')) {
            return true;
        }
        do {
            if (!ScanValue (refs)) {
                return false;
            }
        } while (Consume (','));
        return Consume (']');
    }

    // true, false, null or a number.
    bool ScanLiteral ()
    {
        const UIndex start = mPos;
        while (mPos < mLength) {
            const Char c = At (mPos);
            if (IsWhitespace (c) || c == static_cast<Char> (',') || c == static_cast<Char> (':') ||
                c == static_cast<Char> ('{') || c == static_cast<Char> ('}') ||
                c == static_cast<Char> ('[') || c == static_cast<Char> (']') || c == static_cast<Char> ('"')) {
                break;
            }
            ++mPos;
        }
        return mPos > start;
    }

    const GS::UniString& mText;
    const USize          mLength;
    UIndex               mPos;
};

struct CommonSchemaDefinition
{
    UIndex                   valueStart = 0;
    USize                    valueLength = 0;
    std::vector<std::string> refs;
};

struct ParsedCommonSchemaDefinitions
{
    bool                                                    isValid = false;
    std::vector<std::string>                                namesInFileOrder;
    std::unordered_map<std::string, CommonSchemaDefinition> definitions;
};

bool ParseCommonSchemaDefinitions (const GS::UniString& text, ParsedCommonSchemaDefinitions& parsed)
{
    SchemaReferenceScanner scanner (text);
    if (!scanner.Consume ('{')) {
        return false;
    }
    if (scanner.Consume ('}')) {
        return scanner.IsAtEnd ();
    }
    do {
        std::string name;
        bool nameHasEscape;
        bool nameIsAscii;
        if (!scanner.ScanString (name, nameHasEscape, nameIsAscii) || nameHasEscape || !nameIsAscii || name.empty ()) {
            return false;
        }
        if (!scanner.Consume (':')) {
            return false;
        }
        CommonSchemaDefinition definition;
        scanner.SkipWhitespace ();
        definition.valueStart = scanner.GetPosition ();
        if (!scanner.ScanValue (definition.refs)) {
            return false;
        }
        definition.valueLength = scanner.GetPosition () - definition.valueStart;
        if (!parsed.definitions.emplace (name, std::move (definition)).second) {
            // A name defined twice: which one counts is up to the JSON reader.
            return false;
        }
        parsed.namesInFileOrder.push_back (name);
    } while (scanner.Consume (','));
    return scanner.Consume ('}') && scanner.IsAtEnd ();
}

const ParsedCommonSchemaDefinitions& GetParsedCommonSchemaDefinitions ()
{
    static const ParsedCommonSchemaDefinitions parsedDefinitions = [] () {
        ParsedCommonSchemaDefinitions parsed;
        parsed.isValid = ParseCommonSchemaDefinitions (GetCommonSchemaDefinitions (), parsed);
        return parsed;
    } ();
    return parsedDefinitions;
}

}

GS::Optional<GS::UniString> GetCommonSchemaDefinitionsFor (const GS::Array<GS::UniString>& schemas)
{
    const GS::UniString& commonSchemaDefinitions = GetCommonSchemaDefinitions ();
    const ParsedCommonSchemaDefinitions& parsed = GetParsedCommonSchemaDefinitions ();
    if (!parsed.isValid) {
        return commonSchemaDefinitions;
    }

    std::vector<std::string> pending;
    for (const GS::UniString& schema : schemas) {
        SchemaReferenceScanner scanner (schema);
        if (!scanner.ScanValue (pending) || !scanner.IsAtEnd ()) {
            return commonSchemaDefinitions;
        }
    }

    std::unordered_set<std::string> needed;
    while (!pending.empty ()) {
        const std::string name = pending.back ();
        pending.pop_back ();
        if (needed.count (name) > 0) {
            continue;
        }
        const auto found = parsed.definitions.find (name);
        if (found == parsed.definitions.end ()) {
            return commonSchemaDefinitions;
        }
        needed.insert (name);
        pending.insert (pending.end (), found->second.refs.begin (), found->second.refs.end ());
    }

    if (needed.empty ()) {
        return GS::NoValue;
    }

    GS::UniString result ("{");
    bool isFirst = true;
    for (const std::string& name : parsed.namesInFileOrder) {
        if (needed.count (name) == 0) {
            continue;
        }
        const CommonSchemaDefinition& definition = parsed.definitions.at (name);
        result += isFirst ? "\n    \"" : ",\n    \"";
        result += GS::UniString (name.c_str ());
        result += "\": ";
        result += GS::UniString (commonSchemaDefinitions.GetSubstring (definition.valueStart, definition.valueLength));
        isFirst = false;
    }
    result += "\n}";
    return result;
}
