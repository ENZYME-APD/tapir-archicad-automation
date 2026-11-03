#pragma once

#include "CommandBase.hpp"

#include <utility>
#include <vector>

// Key/value pairs kept in the user data of an attribute, a group or an element. Layout version 1
// (the dataVersion of the user data) is the pairs as UTF-8 text, each key and each value followed
// by a NUL. UTF-8 bytes do not depend on the platform, so platformSign is only recorded, never
// swapped. User data with any other dataVersion is reported, never overwritten.

constexpr short UserDataPairsLayoutVersion = 1;

using UserDataPairs = std::vector<std::pair<GS::UniString, GS::UniString>>;

void SetUserDataPair (UserDataPairs& pairs, const GS::UniString& key, const GS::UniString& value);
void DeleteUserDataPair (UserDataPairs& pairs, const GS::UniString& key);

// Returns a new handle the caller disposes, or nullptr when out of memory. No pairs are stored as a
// single NUL, which reads back as no pairs.
GSHandle SerializeUserDataPairs (const UserDataPairs& pairs);
UserDataPairs ParseUserDataPairs (GSHandle dataHdl);

// The pairs as the JSON list [{key, value}].
GS::ObjectState CreateUserDataPairObjectState (const GS::UniString& key, const GS::UniString& value);
void AddUserDataPairsList (GS::ObjectState& os, const char* name, const UserDataPairs& pairs);

// Reads the optional "pairs" ([{key, value}]) and "keysToDelete" ([string]) parameters. Returns false
// with errorMessage filled when neither is given or a pair has an empty key or no value.
bool GetUserDataPairChanges (const GS::ObjectState& parameters, UserDataPairs& pairsToSet, GS::Array<GS::UniString>& keysToDelete, GS::UniString& errorMessage);

// Merges the changes into pairs: the given keys get their new values, then keysToDelete are removed.
void ApplyUserDataPairChanges (UserDataPairs& pairs, const UserDataPairs& pairsToSet, const GS::Array<GS::UniString>& keysToDelete);
