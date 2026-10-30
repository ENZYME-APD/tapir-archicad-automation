#include "UserDataPairs.hpp"

#include <cstring>
#include <string>

void SetUserDataPair (UserDataPairs& pairs, const GS::UniString& key, const GS::UniString& value)
{
    for (auto& pair : pairs) {
        if (pair.first == key) {
            pair.second = value;
            return;
        }
    }
    pairs.emplace_back (key, value);
}

void DeleteUserDataPair (UserDataPairs& pairs, const GS::UniString& key)
{
    for (auto it = pairs.begin (); it != pairs.end (); ++it) {
        if (it->first == key) {
            pairs.erase (it);
            return;
        }
    }
}

GSHandle SerializeUserDataPairs (const UserDataPairs& pairs)
{
    std::string bytes;
    for (const auto& pair : pairs) {
        bytes.append (pair.first.ToCStr (CC_UTF8).Get ());
        bytes.push_back ('\0');
        bytes.append (pair.second.ToCStr (CC_UTF8).Get ());
        bytes.push_back ('\0');
    }
    if (bytes.empty ()) {
        bytes.push_back ('\0');
    }

    const GSSize size = static_cast<GSSize> (bytes.size ());
    GSHandle handle = BMAllocateHandle (size, ALLOCATE_CLEAR, 0);
    if (handle != nullptr) {
        memcpy (*handle, bytes.data (), size);
    }
    return handle;
}

UserDataPairs ParseUserDataPairs (GSHandle dataHdl)
{
    UserDataPairs pairs;
    if (dataHdl == nullptr) {
        return pairs;
    }

    const char* data = *dataHdl;
    const GSSize size = BMGetHandleSize (dataHdl);
    GSSize pos = 0;

    // Reads the next NUL terminated string; false when the data has ended.
    const auto readString = [&] (GS::UniString& str) -> bool {
        if (pos >= size) {
            return false;
        }
        GSSize end = pos;
        while (end < size && data[end] != '\0') {
            ++end;
        }
        str = GS::UniString (data + pos, static_cast<USize> (end - pos), CC_UTF8);
        pos = end + 1;
        return true;
    };

    GS::UniString key;
    GS::UniString value;
    while (readString (key)) {
        if (!readString (value)) {
            value = GS::EmptyUniString;
        }
        if (!key.IsEmpty ()) {
            SetUserDataPair (pairs, key, value);
        }
    }
    return pairs;
}

GS::ObjectState CreateUserDataPairObjectState (const GS::UniString& key, const GS::UniString& value)
{
    GS::ObjectState item;
    item.Add ("key", key);
    item.Add ("value", value);
    return item;
}

void AddUserDataPairsList (GS::ObjectState& os, const char* name, const UserDataPairs& pairs)
{
    const auto& list = os.AddList<GS::ObjectState> (name);
    for (const auto& pair : pairs) {
        list (CreateUserDataPairObjectState (pair.first, pair.second));
    }
}

bool GetUserDataPairChanges (const GS::ObjectState& parameters, UserDataPairs& pairsToSet, GS::Array<GS::UniString>& keysToDelete, GS::UniString& errorMessage)
{
    pairsToSet.clear ();
    keysToDelete.Clear ();

    GS::Array<GS::ObjectState> pairs;
    const bool hasPairs = parameters.Get ("pairs", pairs);
    const bool hasKeysToDelete = parameters.Get ("keysToDelete", keysToDelete);
    if (!hasPairs && !hasKeysToDelete) {
        errorMessage = "Invalid input: pairs or keysToDelete is required.";
        return false;
    }

    for (const GS::ObjectState& pair : pairs) {
        GS::UniString key;
        GS::UniString value;
        if (!pair.Get ("key", key) || key.IsEmpty () || !pair.Get ("value", value)) {
            errorMessage = "Invalid input: every item of pairs needs a non-empty key and a value.";
            return false;
        }
        SetUserDataPair (pairsToSet, key, value);
    }
    return true;
}

void ApplyUserDataPairChanges (UserDataPairs& pairs, const UserDataPairs& pairsToSet, const GS::Array<GS::UniString>& keysToDelete)
{
    for (const auto& pair : pairsToSet) {
        SetUserDataPair (pairs, pair.first, pair.second);
    }
    for (const GS::UniString& key : keysToDelete) {
        DeleteUserDataPair (pairs, key);
    }
}
