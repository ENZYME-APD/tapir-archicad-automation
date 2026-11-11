#include "InputKeyCheck.hpp"
#include "SchemaDefinitions.hpp"

#include "Array.hpp"
#include "HashSet.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

// A JSON value of a schema: only what the check needs is kept (objects with their members, arrays,
// strings and booleans; numbers and null are Other).
class InputKeyChecker::Node
{
public:
    enum class Kind { Other, Bool, String, Array, Object };

    Kind                                               kind = Kind::Other;
    bool                                               boolValue = false;
    std::string                                        stringValue;
    std::vector<std::shared_ptr<const Node>>           items;
    std::map<std::string, std::shared_ptr<const Node>> members;

    const Node* Member (const char* name) const
    {
        if (kind != Kind::Object) {
            return nullptr;
        }
        const auto found = members.find (name);
        return found != members.end () ? found->second.get () : nullptr;
    }
};

namespace {

using Node = InputKeyChecker::Node;
using NodePtr = std::shared_ptr<const Node>;

const int MaxParseDepth = 256;
const int MaxRefChain = 32;
const int MaxCheckDepth = 64;

void AppendUtf8 (std::string& text, UInt32 codePoint)
{
    if (codePoint < 0x80) {
        text.push_back (static_cast<char> (codePoint));
    } else if (codePoint < 0x800) {
        text.push_back (static_cast<char> (0xC0 | (codePoint >> 6)));
        text.push_back (static_cast<char> (0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
        text.push_back (static_cast<char> (0xE0 | (codePoint >> 12)));
        text.push_back (static_cast<char> (0x80 | ((codePoint >> 6) & 0x3F)));
        text.push_back (static_cast<char> (0x80 | (codePoint & 0x3F)));
    } else {
        text.push_back (static_cast<char> (0xF0 | (codePoint >> 18)));
        text.push_back (static_cast<char> (0x80 | ((codePoint >> 12) & 0x3F)));
        text.push_back (static_cast<char> (0x80 | ((codePoint >> 6) & 0x3F)));
        text.push_back (static_cast<char> (0x80 | (codePoint & 0x3F)));
    }
}

// Reads a JSON text (a schema, or the common definitions file) into Nodes. Any JSON it cannot read
// makes Parse return nothing, and the check is then switched off.
class SchemaParser
{
public:
    explicit SchemaParser (const GS::UniString& text) :
        mText (text),
        mLength (text.GetLength ()),
        mPos (0),
        mFailed (false)
    {
    }

    NodePtr Parse ()
    {
        NodePtr root = ParseValue (0);
        SkipWhitespace ();
        if (mFailed || root == nullptr || mPos < mLength) {
            return nullptr;
        }
        return root;
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

    void SkipWhitespace ()
    {
        while (mPos < mLength && IsWhitespace (At (mPos))) {
            ++mPos;
        }
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

    bool ReadHex4 (UInt32& value)
    {
        value = 0;
        for (int i = 0; i < 4; ++i) {
            if (mPos >= mLength) {
                return false;
            }
            const Char c = At (mPos++);
            value <<= 4;
            if (c >= static_cast<Char> ('0') && c <= static_cast<Char> ('9')) {
                value |= static_cast<UInt32> (c - static_cast<Char> ('0'));
            } else if (c >= static_cast<Char> ('a') && c <= static_cast<Char> ('f')) {
                value |= static_cast<UInt32> (c - static_cast<Char> ('a') + 10);
            } else if (c >= static_cast<Char> ('A') && c <= static_cast<Char> ('F')) {
                value |= static_cast<UInt32> (c - static_cast<Char> ('A') + 10);
            } else {
                return false;
            }
        }
        return true;
    }

    // A UTF-16 code unit, or a surrogate pair, as one code point; first is the unit already read.
    bool ReadCodePoint (UInt32 first, bool escaped, UInt32& codePoint)
    {
        codePoint = first;
        if (first < 0xD800 || first > 0xDFFF) {
            return true;
        }
        if (first > 0xDBFF) {
            return false;
        }
        UInt32 second = 0;
        if (escaped) {
            if (mPos + 1 >= mLength || At (mPos) != static_cast<Char> ('\\') || At (mPos + 1) != static_cast<Char> ('u')) {
                return false;
            }
            mPos += 2;
            if (!ReadHex4 (second)) {
                return false;
            }
        } else {
            if (mPos >= mLength) {
                return false;
            }
            second = static_cast<UInt32> (At (mPos++));
        }
        if (second < 0xDC00 || second > 0xDFFF) {
            return false;
        }
        codePoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
        return true;
    }

    bool ParseString (std::string& content)
    {
        content.clear ();
        if (!Consume ('"')) {
            return false;
        }
        while (mPos < mLength) {
            const Char c = At (mPos++);
            if (c == static_cast<Char> ('"')) {
                return true;
            }
            UInt32 codePoint = 0;
            if (c == static_cast<Char> ('\\')) {
                if (mPos >= mLength) {
                    return false;
                }
                const Char e = At (mPos++);
                switch (e) {
                    case '"':  codePoint = '"';  break;
                    case '\\': codePoint = '\\'; break;
                    case '/':  codePoint = '/';  break;
                    case 'b':  codePoint = '\b'; break;
                    case 'f':  codePoint = '\f'; break;
                    case 'n':  codePoint = '\n'; break;
                    case 'r':  codePoint = '\r'; break;
                    case 't':  codePoint = '\t'; break;
                    case 'u': {
                        UInt32 unit = 0;
                        if (!ReadHex4 (unit) || !ReadCodePoint (unit, true, codePoint)) {
                            return false;
                        }
                        break;
                    }
                    default:
                        return false;
                }
            } else if (!ReadCodePoint (static_cast<UInt32> (c), false, codePoint)) {
                return false;
            }
            AppendUtf8 (content, codePoint);
        }
        return false;
    }

    NodePtr ParseValue (int depth)
    {
        if (depth > MaxParseDepth) {
            mFailed = true;
            return nullptr;
        }
        SkipWhitespace ();
        if (mPos >= mLength) {
            mFailed = true;
            return nullptr;
        }
        std::shared_ptr<Node> node = std::make_shared<Node> ();
        const Char c = At (mPos);
        if (c == static_cast<Char> ('{')) {
            node->kind = Node::Kind::Object;
            ++mPos;
            if (Consume ('}')) {
                return node;
            }
            do {
                std::string name;
                if (!ParseString (name) || !Consume (':')) {
                    mFailed = true;
                    return nullptr;
                }
                NodePtr value = ParseValue (depth + 1);
                if (value == nullptr) {
                    mFailed = true;
                    return nullptr;
                }
                node->members[name] = value;
            } while (Consume (','));
            if (!Consume ('}')) {
                mFailed = true;
                return nullptr;
            }
        } else if (c == static_cast<Char> ('[')) {
            node->kind = Node::Kind::Array;
            ++mPos;
            if (Consume (']')) {
                return node;
            }
            do {
                NodePtr value = ParseValue (depth + 1);
                if (value == nullptr) {
                    mFailed = true;
                    return nullptr;
                }
                node->items.push_back (value);
            } while (Consume (','));
            if (!Consume (']')) {
                mFailed = true;
                return nullptr;
            }
        } else if (c == static_cast<Char> ('"')) {
            node->kind = Node::Kind::String;
            if (!ParseString (node->stringValue)) {
                mFailed = true;
                return nullptr;
            }
        } else {
            // true, false, null or a number.
            std::string literal;
            while (mPos < mLength) {
                const Char l = At (mPos);
                if (IsWhitespace (l) || l == static_cast<Char> (',') || l == static_cast<Char> (':') ||
                    l == static_cast<Char> ('{') || l == static_cast<Char> ('}') ||
                    l == static_cast<Char> ('[') || l == static_cast<Char> (']') || l == static_cast<Char> ('"')) {
                    break;
                }
                if (l > 127) {
                    mFailed = true;
                    return nullptr;
                }
                literal.push_back (static_cast<char> (l));
                ++mPos;
            }
            if (literal.empty ()) {
                mFailed = true;
                return nullptr;
            }
            if (literal == "true" || literal == "false") {
                node->kind = Node::Kind::Bool;
                node->boolValue = (literal == "true");
            }
        }
        return node;
    }

    const GS::UniString& mText;
    const USize          mLength;
    UIndex               mPos;
    bool                 mFailed;
};

// The common definitions, read once; nullptr when the file cannot be read.
const Node* GetDefinitions ()
{
    static const NodePtr definitions = [] () -> NodePtr {
        NodePtr parsed = SchemaParser (GetCommonSchemaDefinitions ()).Parse ();
        if (parsed == nullptr || parsed->kind != Node::Kind::Object) {
            return nullptr;
        }
        return parsed;
    } ();
    return definitions.get ();
}

bool ContainsRef (const GS::UniString& schema)
{
    return schema.Contains (GS::UniString ("\"$ref\""));
}

struct UnknownKey
{
    std::string path;
    std::string key;
    // The object matches none of the alternatives (oneOf / anyOf) of its schema.
    bool        inAlternatives = false;
};

class Walker
{
public:
    explicit Walker (const Node& definitions) :
        mDefinitions (definitions)
    {
    }

    // The schema a node stands for, with its "$ref"s followed; nullptr when it is not an object or
    // a reference cannot be followed. Keys next to a "$ref" (a description) are ignored.
    const Node* Resolve (const Node* schema) const
    {
        for (int i = 0; schema != nullptr && i <= MaxRefChain; ++i) {
            if (schema->kind != Node::Kind::Object) {
                return nullptr;
            }
            const Node* ref = schema->Member ("$ref");
            if (ref == nullptr) {
                return schema;
            }
            if (ref->kind != Node::Kind::String || ref->stringValue.size () <= 2 || ref->stringValue.compare (0, 2, "#/") != 0) {
                return nullptr;
            }
            schema = mDefinitions.Member (ref->stringValue.substr (2).c_str ());
        }
        return nullptr;
    }

    // Whether the items of a list with this schema can be objects that are worth checking.
    bool IsObjectSchema (const Node* schema) const
    {
        const Node* resolved = Resolve (schema);
        return resolved != nullptr &&
               (resolved->Member ("properties") != nullptr || resolved->Member ("additionalProperties") != nullptr ||
                resolved->Member ("oneOf") != nullptr || resolved->Member ("anyOf") != nullptr);
    }

    // true when it found an unknown key in object, described in found.
    bool CheckObject (const Node* schemaNode, const GS::ObjectState& object, const std::string& path, int depth, UnknownKey& found) const
    {
        if (depth > MaxCheckDepth) {
            return false;
        }
        const Node* schema = Resolve (schemaNode);
        if (schema == nullptr) {
            return false;
        }

        // The keys any branch of a oneOf or anyOf lists are known here too, and the object is refused
        // only when every branch refuses a key.
        std::vector<const Node*> branchProperties;
        if (CheckBranches (schema, "object", depth, branchProperties, found, [&] (const Node* branch, UnknownKey& branchFound) {
                return CheckObject (branch, object, path, depth + 1, branchFound);
            })) {
            if (found.path == path) {
                found.inAlternatives = true;
            }
            return true;
        }

        const Node* properties = schema->Member ("properties");
        const Node* additional = schema->Member ("additionalProperties");
        const bool refusesOthers = additional != nullptr && additional->kind == Node::Kind::Bool && !additional->boolValue &&
                                   schema->Member ("patternProperties") == nullptr;
        const Node* othersSchema = (additional != nullptr && additional->kind == Node::Kind::Object) ? additional : nullptr;

        // Sorted, so the same request always names the same key.
        std::vector<std::string> keys;
        const GS::HashSet<GS::String> fieldNames = object.GetFieldNames ();
        for (const GS::String& fieldName : fieldNames) {
            keys.push_back (std::string (fieldName.ToCStr ()));
        }
        std::sort (keys.begin (), keys.end ());

        for (const std::string& key : keys) {
            const Node* keySchema = properties != nullptr ? properties->Member (key.c_str ()) : nullptr;
            if (keySchema == nullptr) {
                bool listedByBranch = false;
                for (const Node* branchProps : branchProperties) {
                    if (branchProps->Member (key.c_str ()) != nullptr) {
                        listedByBranch = true;
                        break;
                    }
                }
                if (listedByBranch) {
                    continue;
                }
                if (refusesOthers) {
                    found.path = path;
                    found.key = key;
                    return true;
                }
                keySchema = othersSchema;
            }
            if (keySchema != nullptr && CheckField (keySchema, object, GS::String (key.c_str ()), path + "/" + key, depth + 1, found)) {
                return true;
            }
        }
        return false;
    }

private:
    // Whether a branch of a oneOf or anyOf can take a value of this JSON type ("object" or "array"):
    // it names no type, or this one.
    bool BranchAcceptsType (const Node* branch, const char* valueType) const
    {
        const Node* resolved = Resolve (branch);
        const Node* type = resolved != nullptr ? resolved->Member ("type") : nullptr;
        if (type == nullptr) {
            return true;
        }
        if (type->kind == Node::Kind::String) {
            return type->stringValue == valueType;
        }
        if (type->kind == Node::Kind::Array) {
            for (const NodePtr& item : type->items) {
                if (item->kind == Node::Kind::String && item->stringValue == valueType) {
                    return true;
                }
            }
            return false;
        }
        return true;
    }

    // Runs check on every branch of the schema's oneOf and anyOf that can take a value of valueType.
    // Collects the "properties" of the branches in branchProperties. true (with found set to the most
    // specific branch error) only when there are branches, none of them accepts the value, and at
    // least one refused it for an unknown key; a value no branch takes for its type is not ours to refuse.
    template <typename CheckBranch>
    bool CheckBranches (const Node* schema, const char* valueType, int depth, std::vector<const Node*>& branchProperties, UnknownKey& found, const CheckBranch& check) const
    {
        bool hasBranches = false;
        bool everyBranchFailed = true;
        UnknownKey mostSpecific;
        const char* const combinators[] = { "oneOf", "anyOf" };
        for (const char* combinator : combinators) {
            const Node* branches = schema->Member (combinator);
            if (branches == nullptr || branches->kind != Node::Kind::Array) {
                continue;
            }
            for (const NodePtr& branch : branches->items) {
                hasBranches = true;
                const Node* resolvedBranch = Resolve (branch.get ());
                if (resolvedBranch != nullptr) {
                    const Node* props = resolvedBranch->Member ("properties");
                    if (props != nullptr && props->kind == Node::Kind::Object) {
                        branchProperties.push_back (props);
                    }
                }
                if (!everyBranchFailed || depth > MaxCheckDepth || !BranchAcceptsType (branch.get (), valueType)) {
                    continue;
                }
                UnknownKey branchFound;
                if (check (branch.get (), branchFound)) {
                    if (branchFound.path.size () >= mostSpecific.path.size ()) {
                        mostSpecific = branchFound;
                    }
                } else {
                    everyBranchFailed = false;
                }
            }
        }
        if (hasBranches && everyBranchFailed && !mostSpecific.key.empty ()) {
            found = mostSpecific;
            return true;
        }
        return false;
    }

    bool CheckList (const Node* schemaNode, const GS::Array<GS::ObjectState>& items, const std::string& path, int depth, UnknownKey& found) const
    {
        if (depth > MaxCheckDepth) {
            return false;
        }
        const Node* schema = Resolve (schemaNode);
        if (schema == nullptr) {
            return false;
        }
        std::vector<const Node*> unusedBranchProperties;
        if (CheckBranches (schema, "array", depth, unusedBranchProperties, found, [&] (const Node* branch, UnknownKey& branchFound) {
                return CheckList (branch, items, path, depth + 1, branchFound);
            })) {
            return true;
        }
        const Node* itemSchema = schema->Member ("items");
        if (itemSchema == nullptr || !IsObjectSchema (itemSchema)) {
            return false;
        }
        for (UIndex i = 0; i < items.GetSize (); ++i) {
            if (CheckObject (itemSchema, items[i], path + "/" + std::to_string (i), depth + 1, found)) {
                return true;
            }
        }
        return false;
    }

    // A field of owner: an object or a list of objects is checked, anything else is not.
    bool CheckField (const Node* schema, const GS::ObjectState& owner, const GS::String& fieldName, const std::string& path, int depth, UnknownKey& found) const
    {
        if (owner.IsObject (fieldName)) {
            const GS::ObjectState* object = owner.Get (fieldName);
            return object != nullptr && CheckObject (schema, *object, path, depth, found);
        }
        if (owner.IsList (fieldName) && AcceptsObjectItems (schema)) {
            GS::Array<GS::ObjectState> items;
            if (!owner.Get (fieldName, items)) {
                return false;
            }
            return CheckList (schema, items, path, depth, found);
        }
        return false;
    }

    // Whether a list with this schema may hold objects to check: its items, or the items of one of
    // its oneOf / anyOf branches, have an object schema. Lists of numbers, strings or lists are not read.
    bool AcceptsObjectItems (const Node* schemaNode) const
    {
        const Node* schema = Resolve (schemaNode);
        if (schema == nullptr) {
            return false;
        }
        const Node* itemSchema = schema->Member ("items");
        if (itemSchema != nullptr && IsObjectSchema (itemSchema)) {
            return true;
        }
        const char* const combinators[] = { "oneOf", "anyOf" };
        for (const char* combinator : combinators) {
            const Node* branches = schema->Member (combinator);
            if (branches == nullptr || branches->kind != Node::Kind::Array) {
                continue;
            }
            for (const NodePtr& branch : branches->items) {
                const Node* resolvedBranch = Resolve (branch.get ());
                const Node* branchItems = resolvedBranch != nullptr ? resolvedBranch->Member ("items") : nullptr;
                if (branchItems != nullptr && IsObjectSchema (branchItems)) {
                    return true;
                }
            }
        }
        return false;
    }

    const Node& mDefinitions;
};

}

InputKeyChecker::InputKeyChecker (const GS::UniString& inputSchema)
{
    if (!ContainsRef (inputSchema)) {
        return;
    }
    NodePtr schema = SchemaParser (inputSchema).Parse ();
    if (schema == nullptr || schema->kind != Node::Kind::Object) {
        return;
    }
    mSchema = schema;
}

InputKeyChecker::~InputKeyChecker ()
{
}

GS::Optional<GS::UniString> InputKeyChecker::FindUnknownKey (const GS::ObjectState& parameters) const
{
    const Node* definitions = GetDefinitions ();
    if (mSchema == nullptr || definitions == nullptr) {
        return GS::NoValue;
    }

    UnknownKey found;
    if (!Walker (*definitions).CheckObject (mSchema.get (), parameters, "#", 0, found)) {
        return GS::NoValue;
    }

    const std::string message = "Unknown key '" + found.key + "' at " + found.path +
        (found.inAlternatives ? ": none of the forms the schema allows for this object (oneOf) takes it together with the other keys given."
                              : ": the schema of this object does not list it.");
    return GS::UniString (message.c_str (), CC_UTF8);
}
