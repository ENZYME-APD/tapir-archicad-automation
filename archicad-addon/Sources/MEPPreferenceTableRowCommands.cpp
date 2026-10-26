#include "MEPPreferenceTableRowCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPPipeElbowPreferenceTable.hpp"
#include "ACAPI/MEPPipeBranchPreferenceTable.hpp"
#include "ACAPI/MEPDuctElbowPreferenceTable.hpp"
#include "ACAPI/MEPDuctBranchPreferenceTable.hpp"
#include "ACAPI/MEPPipeSegmentPreferenceTable.hpp"
#include "ACAPI/MEPDuctCircularSegmentPreferenceTable.hpp"
#include "ACAPI/MEPDuctRectangularSegmentPreferenceTable.hpp"
#include "ACAPI/MEPCableCarrierSegmentPreferenceTable.hpp"
#ifdef ServerMainVers_2900
#include "ACAPI/MEPPipeTransitionPreferenceTable.hpp"
#include "ACAPI/MEPDuctTransitionPreferenceTable.hpp"
#endif

#include <cmath>
#include <cstring>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

using namespace ACAPI::MEP;
#elif defined (ServerMainVers_2700)
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPEnums.hpp"
#include "ACAPI/MEPPreferenceTableBase.hpp"
#include "ACAPI/MEPPipePreferenceTableContainer.hpp"
#include "ACAPI/MEPDuctPreferenceTableContainer.hpp"
#include "ACAPI/MEPCableCarrierPreferenceTableContainer.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

using namespace ACAPI::MEP;
#endif

// The rows of the preference tables, found by their key: the reference ids and, for branch and elbow rows
// from Archicad 29, the angle. Archicad 28 finds the rows by the reference ids only. Archicad 27 has one
// kind of preference table, with a list of values for each shape; its rows are found by shape and index.

static const char* PreferenceTableRowKeySchema = R"({
                "type": "object",
                "description": "The key of a row. Segment tables: referenceId. Elbow tables: referenceId and angle. Branch tables: mainAxisReferenceId, branchAxisReferenceId and branchAxisAngle. Transition tables: beginReferenceId and endReferenceId. The angles are as GetMEPPreferenceTables returns them; Archicad 28 finds the rows by their reference ids only and rejects a key with an angle. Archicad 27: shape and index (AddMEPPreferenceTableRows: shape only).",
                "properties": {
                    "shape": {
                        "type": "string",
                        "description": "Archicad 27 only: the shape whose list of values the row is in.",
                        "enum": [
                            "Circular",
                            "Rectangular"
                        ]
                    },
                    "index": {
                        "type": "integer",
                        "description": "Archicad 27 only: the index of the row in the list of its shape, from 0.",
                        "minimum": 0
                    },
                    "referenceId": {
                        "type": "integer",
                        "minimum": 0
                    },
                    "angle": {
                        "type": "number"
                    },
                    "mainAxisReferenceId": {
                        "type": "integer",
                        "minimum": 0
                    },
                    "branchAxisReferenceId": {
                        "type": "integer",
                        "minimum": 0
                    },
                    "branchAxisAngle": {
                        "type": "number"
                    },
                    "beginReferenceId": {
                        "type": "integer",
                        "minimum": 0
                    },
                    "endReferenceId": {
                        "type": "integer",
                        "minimum": 0
                    }
                },
                "additionalProperties": false
            })";

static const char* PreferenceTableRowValuesSchema = R"({
                "type": "object",
                "description": "The values of a row, lengths in meters. Segment tables: diameter, wallThickness and description. Elbow tables: diameter, radius and description. Branch tables: mainAxisDiameter, branchAxisDiameter, length, branchOffset, connectionLength and description. Pipe transition tables: beginDiameter, endDiameter, length and description. Duct transition tables: beginDiameter, endDiameter, angle and description. Archicad 27: value and description.",
                "properties": {
                    "value": {
                        "type": "number",
                        "description": "Archicad 27 only: the value of the row."
                    },
                    "diameter": {
                        "type": "number"
                    },
                    "wallThickness": {
                        "type": "number"
                    },
                    "radius": {
                        "type": "number"
                    },
                    "mainAxisDiameter": {
                        "type": "number"
                    },
                    "branchAxisDiameter": {
                        "type": "number"
                    },
                    "length": {
                        "type": "number"
                    },
                    "branchOffset": {
                        "type": "number"
                    },
                    "connectionLength": {
                        "type": "number"
                    },
                    "beginDiameter": {
                        "type": "number"
                    },
                    "endDiameter": {
                        "type": "number"
                    },
                    "angle": {
                        "type": "number"
                    },
                    "description": {
                        "type": "string"
                    }
                },
                "additionalProperties": false
            })";

static const char* PreferenceTableIdSchema = R"({
                "type": "object",
                "description": "The guid of a preference table, as returned by GetMEPPreferenceTables (the branch, elbow and transition tables with includeFittingTables): a pipe or circular duct segment table, a branch or elbow table of the Piping or Ventilation domain, or, from Archicad 29, a transition table. Rectangular duct and cable carrier segment tables are found too, but their rows have no key. Archicad 27: any preference table, as returned by GetMEPPreferenceTableContainers.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            })";

#ifdef ServerMainVers_2800

template <typename TableT>
static constexpr bool IsBranchRowTable = std::is_same_v<TableT, PipeBranchPreferenceTable> || std::is_same_v<TableT, DuctBranchPreferenceTable>;

template <typename TableT>
static constexpr bool IsElbowRowTable = std::is_same_v<TableT, PipeElbowPreferenceTable> || std::is_same_v<TableT, DuctElbowPreferenceTable>;

// The circular segment tables have a fixed row for each reference id.
template <typename TableT>
static constexpr bool IsSegmentRowTable = std::is_same_v<TableT, PipeSegmentPreferenceTable> || std::is_same_v<TableT, DuctCircularSegmentPreferenceTable>;

// The rows of the rectangular duct and cable carrier segment tables have no key; they are ordered by their value.
template <typename TableT>
static constexpr bool IsKeylessRowTable = std::is_same_v<TableT, DuctRectangularSegmentPreferenceTable> || std::is_same_v<TableT, CableCarrierSegmentPreferenceTable>;

// The other tables are the transition tables, from Archicad 29.
#ifdef ServerMainVers_2900
template <typename TableT>
static constexpr bool IsPipeTransitionRowTable = std::is_same_v<TableT, PipeTransitionPreferenceTable>;
#else
template <typename TableT>
static constexpr bool IsPipeTransitionRowTable = false;
#endif

static const char* KeylessPreferenceTableRowsError = "The rows of rectangular duct and cable carrier segment tables have no key, so they cannot be found by one.";

struct PreferenceTableRowKey {
    uint32_t referenceId = 0;
    uint32_t mainAxisReferenceId = 0;
    uint32_t branchAxisReferenceId = 0;
    uint32_t beginReferenceId = 0;
    uint32_t endReferenceId = 0;
    // The angle of an elbow row or the branch axis angle of a branch row, from Archicad 29.
    double angle = 0.0;
};

template <typename ErrorT>
static GS::UniString PreferenceTableRowErrorText (const GS::UniString& what, const ErrorT& error)
{
    return "Failed to " + what + ": " + GS::UniString (error.text.c_str ());
}

template <typename TableT>
static GS::UniString PreferenceTableRowText ()
{
    if constexpr (IsBranchRowTable<TableT>) {
        return "a branch table row";
    } else if constexpr (IsElbowRowTable<TableT>) {
        return "an elbow table row";
    } else if constexpr (IsSegmentRowTable<TableT>) {
        return "a segment table row";
    } else {
        return "a transition table row";
    }
}

static GS::UniString JoinPreferenceTableRowFieldNames (const std::vector<const char*>& names)
{
    GS::UniString text;
    for (size_t i = 0; i < names.size (); ++i) {
        if (i > 0) {
            text += (i + 1 == names.size ()) ? " and " : ", ";
        }
        text += GS::UniString ("'") + names[i] + "'";
    }
    return text;
}

static bool ContainsPreferenceTableRowFieldName (const std::vector<const char*>& names, const char* name)
{
    for (const char* n : names) {
        if (std::strcmp (n, name) == 0) {
            return true;
        }
    }
    return false;
}

static const char* const PreferenceTableRowKeyNames[] = {
    "referenceId", "angle", "mainAxisReferenceId", "branchAxisReferenceId", "branchAxisAngle", "beginReferenceId", "endReferenceId",
    "shape", "index"
};

// Reads the key of a row of a table of type TableT; gives the error message of a key that does not fit.
template <typename TableT>
static std::optional<GS::UniString> ParsePreferenceTableRowKey (const GS::ObjectState& keyData, PreferenceTableRowKey& key)
{
    std::vector<std::pair<const char*, uint32_t*>> referenceIds;
    const char* angleName = nullptr;
    if constexpr (IsBranchRowTable<TableT>) {
        referenceIds = { { "mainAxisReferenceId", &key.mainAxisReferenceId }, { "branchAxisReferenceId", &key.branchAxisReferenceId } };
        angleName = "branchAxisAngle";
    } else if constexpr (IsElbowRowTable<TableT>) {
        referenceIds = { { "referenceId", &key.referenceId } };
        angleName = "angle";
    } else if constexpr (IsSegmentRowTable<TableT>) {
        referenceIds = { { "referenceId", &key.referenceId } };
    } else {
        referenceIds = { { "beginReferenceId", &key.beginReferenceId }, { "endReferenceId", &key.endReferenceId } };
    }

    std::vector<const char*> referenceIdNames;
    for (const auto& referenceId : referenceIds) {
        referenceIdNames.push_back (referenceId.first);
    }
    std::vector<const char*> keyNames = referenceIdNames;
    if (angleName != nullptr) {
        keyNames.push_back (angleName);
    }
    for (const char* name : PreferenceTableRowKeyNames) {
        if (!ContainsPreferenceTableRowFieldName (keyNames, name) && keyData.Contains (name)) {
            return "The key of " + PreferenceTableRowText<TableT> () + " has " + JoinPreferenceTableRowFieldNames (keyNames) + ".";
        }
    }

    for (const auto& referenceId : referenceIds) {
        Int32 value = -1;
        if (!keyData.Get (referenceId.first, value) || value < 0) {
            return "The key of " + PreferenceTableRowText<TableT> () + " needs " + JoinPreferenceTableRowFieldNames (referenceIdNames) + ".";
        }
        *referenceId.second = static_cast<uint32_t> (value);
    }

    if (angleName != nullptr) {
#ifdef ServerMainVers_2900
        if (!keyData.Get (angleName, key.angle)) {
            return GS::UniString ("The key needs '") + angleName + "'.";
        }
#else
        if (keyData.Contains (angleName)) {
            return GS::UniString ("Archicad 28 finds the rows by their reference ids only, so the key cannot have '") + angleName + "'.";
        }
#endif
    }
    return std::nullopt;
}

#ifdef ServerMainVers_2900
// The angle of the row with the reference ids of the key, if it differs from the angle of the key only by
// the rounding of JSON, so that the row is still found. Only branch and elbow keys have an angle.
template <typename TableT>
static double GetExactPreferenceTableRowAngle (const TableT& table, const PreferenceTableRowKey& key)
{
    if constexpr (IsBranchRowTable<TableT> || IsElbowRowTable<TableT>) {
        for (uint32_t i = 0; i < table.GetSize (); ++i) {
            if constexpr (IsBranchRowTable<TableT>) {
                auto mainAxisReferenceId = table.GetMainAxisReferenceId (i);
                auto branchAxisReferenceId = table.GetBranchAxisReferenceId (i);
                auto angle = table.GetBranchAxisAngle (i);
                if (mainAxisReferenceId.IsOk () && branchAxisReferenceId.IsOk () && angle.IsOk () &&
                    *mainAxisReferenceId == key.mainAxisReferenceId && *branchAxisReferenceId == key.branchAxisReferenceId && std::abs (*angle - key.angle) < 1e-9) {
                    return *angle;
                }
            } else {
                auto referenceId = table.GetReferenceId (i);
                auto angle = table.GetAngle (i);
                if (referenceId.IsOk () && angle.IsOk () && *referenceId == key.referenceId && std::abs (*angle - key.angle) < 1e-9) {
                    return *angle;
                }
            }
        }
    } else {
        UNUSED_PARAMETER (table);
    }
    return key.angle;
}
#endif

// What the …ByKey functions and AddNewRow of the table take: its Key struct, or the reference id for the
// segment tables. From Archicad 29; Archicad 28 finds the rows by their reference ids.
template <typename TableT>
static auto MakePreferenceTableRowKey (const PreferenceTableRowKey& key)
{
    if constexpr (IsSegmentRowTable<TableT>) {
        return key.referenceId;
    } else {
        typename TableT::Key tableKey {};
        if constexpr (IsBranchRowTable<TableT>) {
            tableKey.mainAxisReferenceId = key.mainAxisReferenceId;
            tableKey.branchAxisReferenceId = key.branchAxisReferenceId;
#ifdef ServerMainVers_2900
            tableKey.branchAxisAngle = key.angle;
#endif
        } else if constexpr (IsElbowRowTable<TableT>) {
            tableKey.referenceId = key.referenceId;
            tableKey.angle = key.angle;
        } else {
            tableKey.beginReferenceId = key.beginReferenceId;
            tableKey.endReferenceId = key.endReferenceId;
        }
        return tableKey;
    }
}

static const char* const PreferenceTableRowValueNames[] = {
    "diameter", "wallThickness", "radius", "mainAxisDiameter", "branchAxisDiameter", "length", "branchOffset", "connectionLength",
    "beginDiameter", "endDiameter", "angle", "value"
};

// The values of a row of the table type besides its description.
template <typename TableT>
static std::vector<const char*> GetPreferenceTableRowValueNames ()
{
    if constexpr (IsBranchRowTable<TableT>) {
        return { "mainAxisDiameter", "branchAxisDiameter", "length", "branchOffset", "connectionLength" };
    } else if constexpr (IsElbowRowTable<TableT>) {
        return { "diameter", "radius" };
    } else if constexpr (IsSegmentRowTable<TableT>) {
        return { "diameter", "wallThickness" };
    } else if constexpr (IsPipeTransitionRowTable<TableT>) {
        // Pipe transitions are defined by their length, duct transitions by their angle.
        return { "beginDiameter", "endDiameter", "length" };
    } else {
        return { "beginDiameter", "endDiameter", "angle" };
    }
}

// Gives the error message of values that do not belong to the table type.
template <typename TableT>
static std::optional<GS::UniString> CheckPreferenceTableRowValues (const GS::ObjectState& values)
{
    const std::vector<const char*> valueNames = GetPreferenceTableRowValueNames<TableT> ();
    for (const char* name : PreferenceTableRowValueNames) {
        if (!ContainsPreferenceTableRowFieldName (valueNames, name) && values.Contains (name)) {
            return GS::UniString ("The value '") + name + "' does not belong to " + PreferenceTableRowText<TableT> () + ".";
        }
    }
    return std::nullopt;
}

template <typename TableT>
static GS::ObjectState GetPreferenceTableRow (const TableT& table, const PreferenceTableRowKey& key)
{
    GS::ObjectState values;
    const auto addValue = [&] (const char* name, auto result) {
        if (result.IsOk ()) {
            values.Add (name, *result);
        }
    };

    GS::ObjectState row;
#ifdef ServerMainVers_2900
    PreferenceTableRowKey exactKey = key;
    exactKey.angle = GetExactPreferenceTableRowAngle (table, key);
    const auto tableKey = MakePreferenceTableRowKey<TableT> (exactKey);
    auto isValid = table.IsRowValidByKey (tableKey);
#else
    auto isValid = [&] () {
        if constexpr (IsBranchRowTable<TableT>) {
            return table.IsRowValidByReferenceIds (key.mainAxisReferenceId, key.branchAxisReferenceId);
        } else {
            return table.IsRowValidByReferenceId (key.referenceId);
        }
    } ();
#endif
    if (isValid.IsErr ()) {
        return CreateErrorResponse (isValid.UnwrapErr ().kind, PreferenceTableRowErrorText ("find the row", isValid.UnwrapErr ()));
    }
    row.Add ("valid", *isValid);

    if constexpr (IsBranchRowTable<TableT>) {
#ifdef ServerMainVers_2900
        addValue ("mainAxisDiameter", table.GetMainAxisDiameterByKey (tableKey));
        addValue ("branchAxisDiameter", table.GetBranchAxisDiameterByKey (tableKey));
        addValue ("length", table.GetLengthByKey (tableKey));
        addValue ("branchOffset", table.GetBranchOffsetByKey (tableKey));
        addValue ("connectionLength", table.GetConnectionLengthByKey (tableKey));
        addValue ("description", table.GetDescriptionByKey (tableKey));
#else
        const uint32_t mainId = key.mainAxisReferenceId;
        const uint32_t branchId = key.branchAxisReferenceId;
        addValue ("mainAxisDiameter", table.GetMainAxisDiameterByReferenceIds (mainId, branchId));
        addValue ("branchAxisDiameter", table.GetBranchAxisDiameterByReferenceIds (mainId, branchId));
        addValue ("length", table.GetLengthByReferenceIds (mainId, branchId));
        addValue ("branchOffset", table.GetBranchOffsetByReferenceIds (mainId, branchId));
        addValue ("connectionLength", table.GetConnectionLengthByReferenceIds (mainId, branchId));
        addValue ("description", table.GetDescriptionByReferenceIds (mainId, branchId));
#endif
    } else if constexpr (IsElbowRowTable<TableT>) {
#ifdef ServerMainVers_2900
        addValue ("diameter", table.GetDiameterByKey (tableKey));
        addValue ("radius", table.GetRadiusByKey (tableKey));
        addValue ("description", table.GetDescriptionByKey (tableKey));
#else
        addValue ("diameter", table.GetDiameterByReferenceId (key.referenceId));
        addValue ("radius", table.GetRadiusByReferenceId (key.referenceId));
        addValue ("description", table.GetDescriptionByReferenceId (key.referenceId));
#endif
    } else if constexpr (IsSegmentRowTable<TableT>) {
#ifdef ServerMainVers_2900
        addValue ("diameter", table.GetDiameterByKey (tableKey));
        addValue ("wallThickness", table.GetWallThicknessByKey (tableKey));
        addValue ("description", table.GetDescriptionByKey (tableKey));
#else
        addValue ("diameter", table.GetDiameterByReferenceId (key.referenceId));
        addValue ("wallThickness", table.GetWallThicknessByReferenceId (key.referenceId));
        addValue ("description", table.GetDescriptionByReferenceId (key.referenceId));
#endif
    } else {
#ifdef ServerMainVers_2900
        addValue ("beginDiameter", table.GetBeginDiameterByKey (tableKey));
        addValue ("endDiameter", table.GetEndDiameterByKey (tableKey));
        if constexpr (IsPipeTransitionRowTable<TableT>) {
            addValue ("length", table.GetLengthByKey (tableKey));
        } else {
            addValue ("angle", table.GetAngleByKey (tableKey));
        }
        addValue ("description", table.GetDescriptionByKey (tableKey));
#endif
    }
    row.Add ("values", values);
    return row;
}

// Sets the given values of a row inside the Modify of the table; gives the error message of the first
// value that could not be set, the values before it are set.
template <typename TableT>
static std::optional<std::pair<GSErrCode, GS::UniString>> SetPreferenceTableRowValues (typename TableT::Modifier& modifier, const PreferenceTableRowKey& key, const GS::ObjectState& values)
{
    std::optional<std::pair<GSErrCode, GS::UniString>> error;
    const auto call = [&] (const char* name, auto result) {
        if (!error.has_value () && result.IsErr ()) {
            error = std::make_pair (result.UnwrapErr ().kind, PreferenceTableRowErrorText (GS::UniString ("set the ") + name, result.UnwrapErr ()));
        }
    };

#ifdef ServerMainVers_2900
    const auto tableKey = MakePreferenceTableRowKey<TableT> (key);
#endif
    double number = 0.0;
    if constexpr (IsBranchRowTable<TableT>) {
#ifdef ServerMainVers_2900
        if (values.Get ("mainAxisDiameter", number)) {
            call ("mainAxisDiameter", modifier.SetMainAxisDiameterByKey (tableKey, number));
        }
        if (values.Get ("branchAxisDiameter", number)) {
            call ("branchAxisDiameter", modifier.SetBranchAxisDiameterByKey (tableKey, number));
        }
        if (values.Get ("length", number)) {
            call ("length", modifier.SetLengthByKey (tableKey, number));
        }
        if (values.Get ("branchOffset", number)) {
            call ("branchOffset", modifier.SetBranchOffsetByKey (tableKey, number));
        }
        if (values.Get ("connectionLength", number)) {
            call ("connectionLength", modifier.SetConnectionLengthByKey (tableKey, number));
        }
#else
        const uint32_t mainId = key.mainAxisReferenceId;
        const uint32_t branchId = key.branchAxisReferenceId;
        if (values.Get ("mainAxisDiameter", number)) {
            call ("mainAxisDiameter", modifier.SetMainAxisDiameterByReferenceIds (mainId, branchId, number));
        }
        if (values.Get ("branchAxisDiameter", number)) {
            call ("branchAxisDiameter", modifier.SetBranchAxisDiameterByReferenceIds (mainId, branchId, number));
        }
        if (values.Get ("length", number)) {
            call ("length", modifier.SetLengthByReferenceIds (mainId, branchId, number));
        }
        if (values.Get ("branchOffset", number)) {
            call ("branchOffset", modifier.SetBranchOffsetByReferenceIds (mainId, branchId, number));
        }
        if (values.Get ("connectionLength", number)) {
            call ("connectionLength", modifier.SetConnectionLengthByReferenceIds (mainId, branchId, number));
        }
#endif
    } else if constexpr (IsElbowRowTable<TableT>) {
#ifdef ServerMainVers_2900
        if (values.Get ("diameter", number)) {
            call ("diameter", modifier.SetDiameterByKey (tableKey, number));
        }
        if (values.Get ("radius", number)) {
            call ("radius", modifier.SetRadiusByKey (tableKey, number));
        }
#else
        if (values.Get ("diameter", number)) {
            call ("diameter", modifier.SetDiameterByReferenceId (key.referenceId, number));
        }
        if (values.Get ("radius", number)) {
            call ("radius", modifier.SetRadiusByReferenceId (key.referenceId, number));
        }
#endif
    } else if constexpr (IsSegmentRowTable<TableT>) {
#ifdef ServerMainVers_2900
        if (values.Get ("diameter", number)) {
            call ("diameter", modifier.SetDiameterByKey (tableKey, number));
        }
        if (values.Get ("wallThickness", number)) {
            call ("wallThickness", modifier.SetWallThicknessByKey (tableKey, number));
        }
#else
        if (values.Get ("diameter", number)) {
            call ("diameter", modifier.SetDiameterByReferenceId (key.referenceId, number));
        }
        if (values.Get ("wallThickness", number)) {
            call ("wallThickness", modifier.SetWallThicknessByReferenceId (key.referenceId, number));
        }
#endif
    } else {
#ifdef ServerMainVers_2900
        if (values.Get ("beginDiameter", number)) {
            call ("beginDiameter", modifier.SetBeginDiameterByKey (tableKey, number));
        }
        if (values.Get ("endDiameter", number)) {
            call ("endDiameter", modifier.SetEndDiameterByKey (tableKey, number));
        }
        if constexpr (IsPipeTransitionRowTable<TableT>) {
            if (values.Get ("length", number)) {
                call ("length", modifier.SetLengthByKey (tableKey, number));
            }
        } else {
            if (values.Get ("angle", number)) {
                call ("angle", modifier.SetAngleByKey (tableKey, number));
            }
        }
#endif
    }

    GS::UniString description;
    if (values.Get ("description", description)) {
#ifdef ServerMainVers_2900
        call ("description", modifier.SetDescriptionByKey (tableKey, description));
#else
        if constexpr (IsBranchRowTable<TableT>) {
            call ("description", modifier.SetDescriptionByReferenceIds (key.mainAxisReferenceId, key.branchAxisReferenceId, description));
        } else {
            call ("description", modifier.SetDescriptionByReferenceId (key.referenceId, description));
        }
#endif
    }
    return error;
}

// A row of the input whose key (and values) fit the table, or the failed result it gets.
struct PreferenceTableRowInput {
    PreferenceTableRowKey key;
    GS::ObjectState values;
    std::optional<GS::ObjectState> result;
};

template <typename TableT>
static std::vector<PreferenceTableRowInput> ParsePreferenceTableRowInputs (const TableT& table, const GS::Array<GS::ObjectState>& rowsData, bool hasKeyField)
{
    std::vector<PreferenceTableRowInput> inputs;
    for (const GS::ObjectState& rowData : rowsData) {
        PreferenceTableRowInput input;
        GS::ObjectState keyData;
        if (hasKeyField) {
            rowData.Get ("key", keyData);
            rowData.Get ("values", input.values);
        } else {
            keyData = rowData;
        }
        std::optional<GS::UniString> error = ParsePreferenceTableRowKey<TableT> (keyData, input.key);
        if (!error.has_value ()) {
            error = CheckPreferenceTableRowValues<TableT> (input.values);
        }
        if (error.has_value ()) {
            input.result = CreateFailedExecutionResult (APIERR_BADPARS, *error);
        }
#ifdef ServerMainVers_2900
        else {
            input.key.angle = GetExactPreferenceTableRowAngle (table, input.key);
        }
#else
        UNUSED_PARAMETER (table);
#endif
        inputs.push_back (input);
    }
    return inputs;
}

// The same failed result for each of count rows.
static GS::ObjectState CreateFailedPreferenceTableRowResults (UIndex count, GSErrCode errorCode, const GS::UniString& errorMessage)
{
    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (UIndex i = 0; i < count; ++i) {
        executionResults (CreateFailedExecutionResult (errorCode, errorMessage));
    }
    return response;
}

// Calls func with the preference table of the tableId parameter.
template <typename FuncT>
static GS::ObjectState WithRowPreferenceTable (const GS::ObjectState& parameters, const FuncT& func)
{
    GS::ObjectState tableIdData;
    parameters.Get ("tableId", tableIdData);
    const auto id = Adapter::UniqueID (GetGuidFromObjectState (tableIdData));

    if (auto table = PipeBranchPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = DuctBranchPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = PipeElbowPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = DuctElbowPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = PipeSegmentPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = DuctCircularSegmentPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
#ifdef ServerMainVers_2900
    if (auto table = PipeTransitionPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = DuctTransitionPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
#endif
    // The rectangular tables are in the same container as the circular ones; their Get picks them.
    if (auto table = DuctRectangularSegmentPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    if (auto table = CableCarrierSegmentPreferenceTable::Get (id); table.IsOk ()) {
        return func (*table);
    }
    return CreateErrorResponse (APIERR_BADID, "The tableId is not a segment, branch, elbow or transition preference table.");
}

// Runs change on every row that has no result yet inside one Modify of the table, so the whole call is
// one undo step, and fills in the results.
template <typename TableT, typename ChangeT>
static GS::ObjectState ModifyPreferenceTableRows (TableT& table, std::vector<PreferenceTableRowInput>& inputs, const GS::UniString& undoString, const ChangeT& change)
{
    // The rows rejected before the change keep their error; the others fail together if the Modify fails.
    std::vector<bool> isRejected;
    for (const PreferenceTableRowInput& input : inputs) {
        isRejected.push_back (input.result.has_value ());
    }
    ACAPI::Result<void> modifyResult = table.Modify ([&] (typename TableT::Modifier& modifier) {
        for (PreferenceTableRowInput& input : inputs) {
            if (!input.result.has_value ()) {
                input.result = change (modifier, input);
            }
        }
    }, undoString);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (size_t i = 0; i < inputs.size (); ++i) {
        if (modifyResult.IsErr () && !isRejected[i]) {
            executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, PreferenceTableRowErrorText ("modify the preference table", modifyResult.UnwrapErr ())));
        } else if (inputs[i].result.has_value ()) {
            executionResults (*inputs[i].result);
        } else {
            executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The preference table was not modified."));
        }
    }
    return response;
}

#elif defined (ServerMainVers_2700)

// Archicad 27 has one kind of preference table: for each shape a list of values with a description,
// whose rows are found by their index.
struct AC27PreferenceTableRowKey {
    PreferenceTableShape shape = PreferenceTableShape::Circular;
    uint32_t index = 0;
};

template <typename ErrorT>
static GS::UniString AC27PreferenceTableErrorText (const GS::UniString& what, const ErrorT& error)
{
    return "Failed to " + what + ": " + GS::UniString (error.text.c_str ());
}

static std::optional<PreferenceTableShape> AC27PreferenceTableShapeFromString (const GS::UniString& shapeStr)
{
    if (shapeStr == "Circular") {
        return PreferenceTableShape::Circular;
    }
    if (shapeStr == "Rectangular") {
        return PreferenceTableShape::Rectangular;
    }
    return std::nullopt;
}

static const char* const AC27UnusedPreferenceTableRowKeyNames[] = {
    "referenceId", "angle", "mainAxisReferenceId", "branchAxisReferenceId", "branchAxisAngle", "beginReferenceId", "endReferenceId"
};

static const char* const AC27UnusedPreferenceTableRowValueNames[] = {
    "diameter", "wallThickness", "radius", "mainAxisDiameter", "branchAxisDiameter", "length", "branchOffset", "connectionLength",
    "beginDiameter", "endDiameter", "angle"
};

// Reads the key of a row; withIndex is false for the rows to add, whose index the table gives.
static std::optional<GS::UniString> ParseAC27PreferenceTableRowKey (const GS::ObjectState& keyData, bool withIndex, AC27PreferenceTableRowKey& key)
{
    for (const char* name : AC27UnusedPreferenceTableRowKeyNames) {
        if (keyData.Contains (name)) {
            return GS::UniString ("Archicad 27 finds the rows of a preference table by 'shape' and 'index', so the key cannot have '") + name + "'.";
        }
    }

    GS::UniString shapeStr;
    keyData.Get ("shape", shapeStr);
    const std::optional<PreferenceTableShape> shape = AC27PreferenceTableShapeFromString (shapeStr);
    if (!shape.has_value ()) {
        return GS::UniString ("The key needs 'shape': 'Circular' or 'Rectangular'.");
    }
    key.shape = *shape;

    if (!withIndex) {
        if (keyData.Contains ("index")) {
            return GS::UniString ("The table gives the index of a new row, so the key cannot have 'index'.");
        }
        return std::nullopt;
    }
    Int32 index = -1;
    if (!keyData.Get ("index", index) || index < 0) {
        return GS::UniString ("The key needs 'index'.");
    }
    key.index = static_cast<uint32_t> (index);
    return std::nullopt;
}

static std::optional<GS::UniString> CheckAC27PreferenceTableRowValues (const GS::ObjectState& values)
{
    for (const char* name : AC27UnusedPreferenceTableRowValueNames) {
        if (values.Contains (name)) {
            return GS::UniString ("The rows of the preference tables of Archicad 27 have 'value' and 'description', not '") + name + "'.";
        }
    }
    return std::nullopt;
}

// The number of rows of each shape; a shape whose size cannot be read is left out.
static GS::ObjectState GetAC27PreferenceTableSizes (const PreferenceTableBase& table)
{
    GS::ObjectState sizes;
    const auto addSize = [&] (const char* name, PreferenceTableShape shape) {
        auto size = table.GetSize (shape);
        if (size.IsOk ()) {
            sizes.Add (name, static_cast<Int32> (*size));
        }
    };
    addSize ("Circular", PreferenceTableShape::Circular);
    addSize ("Rectangular", PreferenceTableShape::Rectangular);
    return sizes;
}

static GS::ObjectState GetAC27PreferenceTableRow (const PreferenceTableBase& table, const AC27PreferenceTableRowKey& key)
{
    auto size = table.GetSize (key.shape);
    if (size.IsErr ()) {
        return CreateErrorResponse (size.UnwrapErr ().kind, AC27PreferenceTableErrorText ("get the number of rows", size.UnwrapErr ()));
    }

    GS::ObjectState row;
    const bool isValid = key.index < *size;
    row.Add ("valid", isValid);
    GS::ObjectState values;
    if (isValid) {
        auto value = table.GetValue (key.shape, key.index);
        if (value.IsOk ()) {
            values.Add ("value", *value);
        }
        auto description = table.GetDescription (key.shape, key.index);
        if (description.IsOk ()) {
            values.Add ("description", *description);
        }
    }
    row.Add ("values", values);
    return row;
}

// Sets the given values of a row inside the Modify of the table: the description first, as SetValue gives
// the index the row has after its value is set, which index is updated to. Gives the error message of the
// first value that could not be set, the values before it are set.
static std::optional<std::pair<GSErrCode, GS::UniString>> SetAC27PreferenceTableRowValues (PreferenceTableBase::Modifier& modifier, PreferenceTableShape shape, uint32_t& index, const GS::ObjectState& values)
{
    GS::UniString description;
    if (values.Get ("description", description)) {
        ACAPI::Result<void> result = modifier.SetDescription (shape, index, description);
        if (result.IsErr ()) {
            return std::make_pair (result.UnwrapErr ().kind, AC27PreferenceTableErrorText ("set the description", result.UnwrapErr ()));
        }
    }
    double value = 0.0;
    if (values.Get ("value", value)) {
        ACAPI::Result<uint32_t> result = modifier.SetValue (shape, index, value);
        if (result.IsErr ()) {
            return std::make_pair (result.UnwrapErr ().kind, AC27PreferenceTableErrorText ("set the value", result.UnwrapErr ()));
        }
        index = *result;
    }
    return std::nullopt;
}

// A row of the input whose key (and values) fit, or the failed result it gets.
struct AC27PreferenceTableRowInput {
    AC27PreferenceTableRowKey key;
    GS::ObjectState values;
    std::optional<GS::ObjectState> result;
};

static std::vector<AC27PreferenceTableRowInput> ParseAC27PreferenceTableRowInputs (const GS::Array<GS::ObjectState>& rowsData, bool hasKeyField, bool withIndex)
{
    std::vector<AC27PreferenceTableRowInput> inputs;
    for (const GS::ObjectState& rowData : rowsData) {
        AC27PreferenceTableRowInput input;
        GS::ObjectState keyData;
        if (hasKeyField) {
            rowData.Get ("key", keyData);
            rowData.Get ("values", input.values);
        } else {
            keyData = rowData;
        }
        std::optional<GS::UniString> error = ParseAC27PreferenceTableRowKey (keyData, withIndex, input.key);
        if (!error.has_value ()) {
            error = CheckAC27PreferenceTableRowValues (input.values);
        }
        if (error.has_value ()) {
            input.result = CreateFailedExecutionResult (APIERR_BADPARS, *error);
        }
        inputs.push_back (input);
    }
    return inputs;
}

// Calls func with the preference table of the tableId parameter.
template <typename FuncT>
static GS::ObjectState WithAC27PreferenceTable (const GS::ObjectState& parameters, const FuncT& func)
{
    GS::ObjectState tableIdData;
    parameters.Get ("tableId", tableIdData);
    auto table = PreferenceTableBase::Get (Adapter::UniqueID (GetGuidFromObjectState (tableIdData)));
    if (table.IsErr ()) {
        return CreateErrorResponse (APIERR_BADID, "The tableId is not a preference table.");
    }
    // The Result of Archicad 27 gives its value by value, so it is kept in a local for the reference.
    PreferenceTableBase tableValue = *table;
    return func (tableValue);
}

// Runs change on the rows that have no result yet, in the given order, inside one Modify of the table, so
// the whole call is one undo step, and gives the results in the order of the input.
template <typename ChangeT>
static GS::ObjectState ModifyAC27PreferenceTableRows (PreferenceTableBase& table, std::vector<AC27PreferenceTableRowInput>& inputs, const std::vector<size_t>& order, const GS::UniString& undoString, const ChangeT& change)
{
    // The rows rejected before the change keep their error; the others fail together if the Modify fails.
    std::vector<bool> isRejected;
    for (const AC27PreferenceTableRowInput& input : inputs) {
        isRejected.push_back (input.result.has_value ());
    }
    ACAPI::Result<void> modifyResult = table.Modify ([&] (PreferenceTableBase::Modifier& modifier) {
        for (size_t i : order) {
            if (!inputs[i].result.has_value ()) {
                inputs[i].result = change (modifier, inputs[i]);
            }
        }
    }, undoString);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (size_t i = 0; i < inputs.size (); ++i) {
        if (modifyResult.IsErr () && !isRejected[i]) {
            executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, AC27PreferenceTableErrorText ("modify the preference table", modifyResult.UnwrapErr ())));
        } else if (inputs[i].result.has_value ()) {
            executionResults (*inputs[i].result);
        } else {
            executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The preference table was not modified."));
        }
    }
    return response;
}

static std::vector<size_t> GetAC27PreferenceTableRowInputOrder (const std::vector<AC27PreferenceTableRowInput>& inputs)
{
    std::vector<size_t> order;
    for (size_t i = 0; i < inputs.size (); ++i) {
        order.push_back (i);
    }
    return order;
}

// Calls func with the preference table container of the domain.
template <typename FuncT>
static GS::ObjectState WithAC27PreferenceTableContainer (const GS::UniString& domain, const FuncT& func)
{
    const auto callWith = [&] (auto container, const char* name) -> GS::ObjectState {
        if (container.IsErr ()) {
            return CreateErrorResponse (container.UnwrapErr ().kind, AC27PreferenceTableErrorText (GS::UniString ("get the ") + name + " preference table container", container.UnwrapErr ()));
        }
        auto containerValue = *container;
        return func (containerValue);
    };
    if (domain == "Piping") {
        return callWith (GetPipePreferenceTableContainer (), "pipe");
    }
    if (domain == "Ventilation") {
        return callWith (GetDuctPreferenceTableContainer (), "duct");
    }
    if (domain == "CableCarrier") {
        return callWith (GetCableCarrierPreferenceTableContainer (), "cable carrier");
    }
    return CreateErrorResponse (APIERR_BADPARS, "Invalid MEP domain: " + domain);
}

#endif

GetMEPPreferenceTableRowsCommand::GetMEPPreferenceTableRowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPPreferenceTableRowsCommand::GetName () const
{
    return "GetMEPPreferenceTableRows";
}

GS::Optional<GS::UniString> GetMEPPreferenceTableRowsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "tableId": )") + PreferenceTableIdSchema + R"(,
            "keys": {
                "type": "array",
                "description": "The keys of the rows to get.",
                "items": )" + PreferenceTableRowKeySchema + R"(
            }
        },
        "additionalProperties": false,
        "required": [
            "tableId",
            "keys"
        ]
    })";
}

GS::Optional<GS::UniString> GetMEPPreferenceTableRowsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "exists": {
                "type": "boolean",
                "description": "Whether the table exists."
            },
            "name": {
                "type": "string",
                "description": "Archicad 27 only: the name of the table."
            },
            "sizes": {
                "type": "object",
                "description": "Archicad 27 only: the number of rows of each shape.",
                "properties": {
                    "Circular": {
                        "type": "integer"
                    },
                    "Rectangular": {
                        "type": "integer"
                    }
                },
                "additionalProperties": false
            },
            "rows": {
                "type": "array",
                "description": "The rows in the order of the keys.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "key": )") + PreferenceTableRowKeySchema + R"(,
                                "valid": {
                                    "type": "boolean",
                                    "description": "Whether the row is valid. Archicad 27: whether the list of the shape has a row with the index."
                                },
                                "values": )" + PreferenceTableRowValuesSchema + R"(
                            },
                            "additionalProperties": false,
                            "required": [
                                "key",
                                "valid",
                                "values"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "exists",
            "rows"
        ]
    })";
}

#ifdef ServerMainVers_2800
GS::ObjectState GetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> keys;
    parameters.Get ("keys", keys);

    return WithRowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        GS::ObjectState response;
        response.Add ("exists", table.Exists ());
        const auto& rows = response.AddList<GS::ObjectState> ("rows");
        if constexpr (IsKeylessRowTable<TableT>) {
            for (UIndex i = 0; i < keys.GetSize (); ++i) {
                rows (CreateErrorResponse (APIERR_NOTSUPPORTED, KeylessPreferenceTableRowsError));
            }
        } else {
            for (const GS::ObjectState& keyData : keys) {
                PreferenceTableRowKey key;
                const std::optional<GS::UniString> error = ParsePreferenceTableRowKey<TableT> (keyData, key);
                if (error.has_value ()) {
                    rows (CreateErrorResponse (APIERR_BADPARS, *error));
                    continue;
                }
                GS::ObjectState row = GetPreferenceTableRow (table, key);
                if (!row.Contains ("error")) {
                    row.Add ("key", keyData);
                }
                rows (row);
            }
        }
        return response;
    });
}
#elif defined (ServerMainVers_2700)
GS::ObjectState GetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> keys;
    parameters.Get ("keys", keys);

    return WithAC27PreferenceTable (parameters, [&] (PreferenceTableBase& table) -> GS::ObjectState {
        GS::ObjectState response;
        response.Add ("exists", table.Exists ());
        response.Add ("name", table.GetName ());
        response.Add ("sizes", GetAC27PreferenceTableSizes (table));
        const auto& rows = response.AddList<GS::ObjectState> ("rows");
        for (const GS::ObjectState& keyData : keys) {
            AC27PreferenceTableRowKey key;
            const std::optional<GS::UniString> error = ParseAC27PreferenceTableRowKey (keyData, true, key);
            if (error.has_value ()) {
                rows (CreateErrorResponse (APIERR_BADPARS, *error));
                continue;
            }
            GS::ObjectState row = GetAC27PreferenceTableRow (table, key);
            if (!row.Contains ("error")) {
                row.Add ("key", keyData);
            }
            rows (row);
        }
        return response;
    });
}
#else
GS::ObjectState GetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
}
#endif

SetMEPPreferenceTableRowsCommand::SetMEPPreferenceTableRowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetMEPPreferenceTableRowsCommand::GetName () const
{
    return "SetMEPPreferenceTableRows";
}

GS::Optional<GS::UniString> SetMEPPreferenceTableRowsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "tableId": )") + PreferenceTableIdSchema + R"(,
            "rows": {
                "type": "array",
                "description": "The rows to change. Only the given values are set.",
                "items": {
                    "type": "object",
                    "properties": {
                        "key": )" + PreferenceTableRowKeySchema + R"(,
                        "values": )" + PreferenceTableRowValuesSchema + R"(
                    },
                    "additionalProperties": false,
                    "required": [
                        "key",
                        "values"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "tableId",
            "rows"
        ]
    })";
}

GS::Optional<GS::UniString> SetMEPPreferenceTableRowsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "type": "array",
                "description": "One result per row, in the order of the rows.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "success": {
                                    "type": "boolean",
                                    "enum": [ true ]
                                },
                                "index": {
                                    "type": "integer",
                                    "description": "Archicad 27 only: the index of the row after its values were set, which can differ from the index of its key when its value was set. Later rows of the same call are found by their index at that time."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "success"
                            ]
                        },
                        {
                            "$ref": "#/FailedExecutionResult"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

#ifdef ServerMainVers_2800
GS::ObjectState SetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> rowsData;
    parameters.Get ("rows", rowsData);

    return WithRowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        if constexpr (IsKeylessRowTable<TableT>) {
            UNUSED_PARAMETER (table);
            return CreateFailedPreferenceTableRowResults (rowsData.GetSize (), APIERR_NOTSUPPORTED, KeylessPreferenceTableRowsError);
        } else {
            std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, rowsData, true);
            return ModifyPreferenceTableRows (table, inputs, "Set MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
                const auto error = SetPreferenceTableRowValues<TableT> (modifier, input.key, input.values);
                return error.has_value ()
                    ? CreateFailedExecutionResult (error->first, error->second + " The values before it were set.")
                    : CreateSuccessfulExecutionResult ();
            });
        }
    });
}
#elif defined (ServerMainVers_2700)
GS::ObjectState SetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> rowsData;
    parameters.Get ("rows", rowsData);

    return WithAC27PreferenceTable (parameters, [&] (PreferenceTableBase& table) -> GS::ObjectState {
        std::vector<AC27PreferenceTableRowInput> inputs = ParseAC27PreferenceTableRowInputs (rowsData, true, true);
        return ModifyAC27PreferenceTableRows (table, inputs, GetAC27PreferenceTableRowInputOrder (inputs), "Set MEP Preference Table Rows", [] (PreferenceTableBase::Modifier& modifier, const AC27PreferenceTableRowInput& input) {
            uint32_t index = input.key.index;
            const auto error = SetAC27PreferenceTableRowValues (modifier, input.key.shape, index, input.values);
            if (error.has_value ()) {
                return CreateFailedExecutionResult (error->first, error->second + " The values before it were set.");
            }
            GS::ObjectState result = CreateSuccessfulExecutionResult ();
            result.Add ("index", static_cast<Int32> (index));
            return result;
        });
    });
}
#else
GS::ObjectState SetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
}
#endif

AddMEPPreferenceTableRowsCommand::AddMEPPreferenceTableRowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AddMEPPreferenceTableRowsCommand::GetName () const
{
    return "AddMEPPreferenceTableRows";
}

GS::Optional<GS::UniString> AddMEPPreferenceTableRowsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "tableId": )") + PreferenceTableIdSchema + R"(,
            "rows": {
                "type": "array",
                "description": "The rows to add, with their key and optionally their values. Branch tables from Archicad 28, elbow and transition tables from Archicad 29. Segment tables have a row for each reference id, so no row can be added to them. Archicad 27: the key has the shape only, the table gives the index.",
                "items": {
                    "type": "object",
                    "properties": {
                        "key": )" + PreferenceTableRowKeySchema + R"(,
                        "values": )" + PreferenceTableRowValuesSchema + R"(
                    },
                    "additionalProperties": false,
                    "required": [
                        "key"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "tableId",
            "rows"
        ]
    })";
}

GS::Optional<GS::UniString> AddMEPPreferenceTableRowsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "type": "array",
                "description": "One result per row, in the order of the rows.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "success": {
                                    "type": "boolean",
                                    "enum": [ true ]
                                },
                                "index": {
                                    "type": "integer",
                                    "description": "The index of the new row in the table. Archicad 27: in the list of its shape, after its values were set."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "success",
                                "index"
                            ]
                        },
                        {
                            "$ref": "#/FailedExecutionResult"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

#ifdef ServerMainVers_2800
GS::ObjectState AddMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> rowsData;
    parameters.Get ("rows", rowsData);

    return WithRowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        if constexpr (IsKeylessRowTable<TableT>) {
            UNUSED_PARAMETER (table);
            return CreateFailedPreferenceTableRowResults (rowsData.GetSize (), APIERR_NOTSUPPORTED, KeylessPreferenceTableRowsError);
        } else if constexpr (IsSegmentRowTable<TableT>) {
            UNUSED_PARAMETER (table);
            return CreateFailedPreferenceTableRowResults (rowsData.GetSize (), APIERR_NOTSUPPORTED, "Segment tables have a row for each reference id, so no row can be added; set its values with SetMEPPreferenceTableRows.");
        }
#ifndef ServerMainVers_2900
        else if constexpr (IsElbowRowTable<TableT>) {
            // The elbow tables of Archicad 28 have a fixed row for each reference id.
            UNUSED_PARAMETER (table);
            return CreateFailedPreferenceTableRowResults (rowsData.GetSize (), APIERR_NOTSUPPORTED, "Adding rows to elbow tables is available from Archicad 29; Archicad 28 has a row for each reference id, set its values with SetMEPPreferenceTableRows.");
        }
#endif
        else {
            std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, rowsData, true);
            return ModifyPreferenceTableRows (table, inputs, "Add MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
#ifdef ServerMainVers_2900
                ACAPI::Result<uint32_t> index = modifier.AddNewRow (MakePreferenceTableRowKey<TableT> (input.key));
#else
                // Archicad 28 adds the branch rows by their reference ids.
                ACAPI::Result<uint32_t> index = modifier.AddNewRow (input.key.mainAxisReferenceId, input.key.branchAxisReferenceId);
#endif
                if (index.IsErr ()) {
                    return CreateFailedExecutionResult (index.UnwrapErr ().kind, PreferenceTableRowErrorText ("add the row", index.UnwrapErr ()));
                }
                const auto error = SetPreferenceTableRowValues<TableT> (modifier, input.key, input.values);
                if (error.has_value ()) {
                    return CreateFailedExecutionResult (error->first, error->second + " The row was added.");
                }
                GS::ObjectState result = CreateSuccessfulExecutionResult ();
                result.Add ("index", static_cast<Int32> (*index));
                return result;
            });
        }
    });
}
#elif defined (ServerMainVers_2700)
GS::ObjectState AddMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> rowsData;
    parameters.Get ("rows", rowsData);

    return WithAC27PreferenceTable (parameters, [&] (PreferenceTableBase& table) -> GS::ObjectState {
        std::vector<AC27PreferenceTableRowInput> inputs = ParseAC27PreferenceTableRowInputs (rowsData, true, false);
        return ModifyAC27PreferenceTableRows (table, inputs, GetAC27PreferenceTableRowInputOrder (inputs), "Add MEP Preference Table Rows", [] (PreferenceTableBase::Modifier& modifier, const AC27PreferenceTableRowInput& input) {
            ACAPI::Result<uint32_t> newIndex = modifier.AddNewPreference (input.key.shape);
            if (newIndex.IsErr ()) {
                return CreateFailedExecutionResult (newIndex.UnwrapErr ().kind, AC27PreferenceTableErrorText ("add the row", newIndex.UnwrapErr ()));
            }
            uint32_t index = *newIndex;
            const auto error = SetAC27PreferenceTableRowValues (modifier, input.key.shape, index, input.values);
            if (error.has_value ()) {
                return CreateFailedExecutionResult (error->first, error->second + " The row was added.");
            }
            GS::ObjectState result = CreateSuccessfulExecutionResult ();
            result.Add ("index", static_cast<Int32> (index));
            return result;
        });
    });
}
#else
GS::ObjectState AddMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
}
#endif

DeleteMEPPreferenceTableRowsCommand::DeleteMEPPreferenceTableRowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteMEPPreferenceTableRowsCommand::GetName () const
{
    return "DeleteMEPPreferenceTableRows";
}

GS::Optional<GS::UniString> DeleteMEPPreferenceTableRowsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "tableId": )") + PreferenceTableIdSchema + R"(,
            "keys": {
                "type": "array",
                "description": "The keys of the rows to delete. Segment tables, and the elbow tables of Archicad 28, have a row for each reference id, which is emptied instead. Archicad 27: the indices are those before the call; the rows are deleted from the highest index down.",
                "items": )" + PreferenceTableRowKeySchema + R"(
            }
        },
        "additionalProperties": false,
        "required": [
            "tableId",
            "keys"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteMEPPreferenceTableRowsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

#ifdef ServerMainVers_2800
GS::ObjectState DeleteMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> keys;
    parameters.Get ("keys", keys);

    return WithRowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        if constexpr (IsKeylessRowTable<TableT>) {
            UNUSED_PARAMETER (table);
            return CreateFailedPreferenceTableRowResults (keys.GetSize (), APIERR_NOTSUPPORTED, KeylessPreferenceTableRowsError);
        } else {
            std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, keys, false);
            return ModifyPreferenceTableRows (table, inputs, "Delete MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
#ifdef ServerMainVers_2900
                ACAPI::Result<void> result = [&] () {
                    if constexpr (IsSegmentRowTable<TableT>) {
                        // The segment tables have a fixed row for each reference id.
                        return modifier.EmptyRowByKey (input.key.referenceId);
                    } else {
                        return modifier.DeleteRowByKey (MakePreferenceTableRowKey<TableT> (input.key));
                    }
                } ();
#else
                ACAPI::Result<void> result = [&] () {
                    if constexpr (IsBranchRowTable<TableT>) {
                        return modifier.DeleteRowByReferenceIds (input.key.mainAxisReferenceId, input.key.branchAxisReferenceId);
                    } else {
                        // The segment tables, and the elbow tables of Archicad 28, have a fixed row for each reference id.
                        return modifier.EmptyRowByReferenceId (input.key.referenceId);
                    }
                } ();
#endif
                return result.IsErr ()
                    ? CreateFailedExecutionResult (result.UnwrapErr ().kind, PreferenceTableRowErrorText ("delete the row", result.UnwrapErr ()))
                    : CreateSuccessfulExecutionResult ();
            });
        }
    });
}
#elif defined (ServerMainVers_2700)
GS::ObjectState DeleteMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> keys;
    parameters.Get ("keys", keys);

    return WithAC27PreferenceTable (parameters, [&] (PreferenceTableBase& table) -> GS::ObjectState {
        std::vector<AC27PreferenceTableRowInput> inputs = ParseAC27PreferenceTableRowInputs (keys, false, true);
        // Deleting a row moves the rows after it, so a row given twice would delete another one.
        for (size_t i = 0; i < inputs.size (); ++i) {
            for (size_t j = 0; j < i && !inputs[i].result.has_value (); ++j) {
                if (!inputs[j].result.has_value () && inputs[j].key.shape == inputs[i].key.shape && inputs[j].key.index == inputs[i].key.index) {
                    inputs[i].result = CreateFailedExecutionResult (APIERR_BADPARS, "The row is given more than once.");
                }
            }
        }
        // From the highest index down, so the indices of the rows still to delete stay those before the call.
        std::vector<size_t> order = GetAC27PreferenceTableRowInputOrder (inputs);
        std::stable_sort (order.begin (), order.end (), [&] (size_t a, size_t b) {
            return inputs[a].key.index > inputs[b].key.index;
        });
        return ModifyAC27PreferenceTableRows (table, inputs, order, "Delete MEP Preference Table Rows", [] (PreferenceTableBase::Modifier& modifier, const AC27PreferenceTableRowInput& input) {
            ACAPI::Result<void> result = modifier.Delete (input.key.shape, input.key.index);
            return result.IsErr ()
                ? CreateFailedExecutionResult (result.UnwrapErr ().kind, AC27PreferenceTableErrorText ("delete the row", result.UnwrapErr ()))
                : CreateSuccessfulExecutionResult ();
        });
    });
}
#else
GS::ObjectState DeleteMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
}
#endif

static const char* AC27PreferenceTableDomainSchema = R"({
                "type": "string",
                "description": "The MEP domain of a preference table container.",
                "enum": [
                    "Piping",
                    "Ventilation",
                    "CableCarrier"
                ]
            })";

static const char* AC27PreferenceTableIdSchema = R"({
                "type": "object",
                "description": "The id of a preference table, for the tableId of the preference table row commands.",
                "properties": {
                    "guid": {
                        "$ref": "#/Guid"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid"
                ]
            })";

GetMEPPreferenceTableContainersCommand::GetMEPPreferenceTableContainersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPPreferenceTableContainersCommand::GetName () const
{
    return "GetMEPPreferenceTableContainers";
}

GS::Optional<GS::UniString> GetMEPPreferenceTableContainersCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "domains": {
                "type": "array",
                "description": "The domains whose preference tables to list. Defaults to all three.",
                "items": )") + AC27PreferenceTableDomainSchema + R"(
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> GetMEPPreferenceTableContainersCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "containers": {
                "type": "array",
                "description": "The preference table container of each domain, in the order of the domains.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "domain": )") + AC27PreferenceTableDomainSchema + R"(,
                                "exists": {
                                    "type": "boolean",
                                    "description": "Whether the container exists."
                                },
                                "tables": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "tableId": )" + AC27PreferenceTableIdSchema + R"(,
                                            "name": {
                                                "type": "string"
                                            },
                                            "exists": {
                                                "type": "boolean"
                                            },
                                            "sizes": {
                                                "type": "object",
                                                "description": "The number of rows of each shape.",
                                                "properties": {
                                                    "Circular": {
                                                        "type": "integer"
                                                    },
                                                    "Rectangular": {
                                                        "type": "integer"
                                                    }
                                                },
                                                "additionalProperties": false
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "tableId",
                                            "name",
                                            "exists",
                                            "sizes"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "domain",
                                "exists",
                                "tables"
                            ]
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "containers"
        ]
    })";
}

#if defined (ServerMainVers_2700) && !defined (ServerMainVers_2800)
GS::ObjectState GetMEPPreferenceTableContainersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> domains;
    if (!parameters.Get ("domains", domains)) {
        domains.Push ("Piping");
        domains.Push ("Ventilation");
        domains.Push ("CableCarrier");
    }

    GS::ObjectState response;
    const auto& containers = response.AddList<GS::ObjectState> ("containers");
    for (const GS::UniString& domain : domains) {
        containers (WithAC27PreferenceTableContainer (domain, [&] (const auto& container) -> GS::ObjectState {
            GS::ObjectState containerOS;
            containerOS.Add ("domain", domain);
            containerOS.Add ("exists", container.Exists ());
            const auto& tables = containerOS.AddList<GS::ObjectState> ("tables");
            for (const auto& id : container.GetPreferenceTables ()) {
                auto table = PreferenceTableBase::Get (id);
                if (table.IsErr ()) {
                    continue;
                }
                GS::ObjectState tableOS;
                tableOS.Add ("tableId", GS::ObjectState ("guid", id.GetGuid ().ToUniString ()));
                tableOS.Add ("name", table->GetName ());
                tableOS.Add ("exists", table->Exists ());
                tableOS.Add ("sizes", GetAC27PreferenceTableSizes (*table));
                tables (tableOS);
            }
            return containerOS;
        }));
    }
    return response;
}
#else
GS::ObjectState GetMEPPreferenceTableContainersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
#ifdef ServerMainVers_2800
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is for Archicad 27; from Archicad 28 use GetMEPPreferenceTables.");
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27.");
#endif
}
#endif

CreateMEPPreferenceTablesCommand::CreateMEPPreferenceTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMEPPreferenceTablesCommand::GetName () const
{
    return "CreateMEPPreferenceTables";
}

GS::Optional<GS::UniString> CreateMEPPreferenceTablesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "domain": )") + AC27PreferenceTableDomainSchema + R"(,
            "tables": {
                "type": "array",
                "description": "The tables to add, empty, to the preference table container of the domain.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "domain",
            "tables"
        ]
    })";
}

GS::Optional<GS::UniString> CreateMEPPreferenceTablesCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "type": "array",
                "description": "One result per table, in the order of the tables.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "success": {
                                    "type": "boolean",
                                    "enum": [ true ]
                                },
                                "tableId": )") + AC27PreferenceTableIdSchema + R"(
                            },
                            "additionalProperties": false,
                            "required": [
                                "success",
                                "tableId"
                            ]
                        },
                        {
                            "$ref": "#/FailedExecutionResult"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

#if defined (ServerMainVers_2700) && !defined (ServerMainVers_2800)
GS::ObjectState CreateMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString domain;
    parameters.Get ("domain", domain);
    GS::Array<GS::ObjectState> tablesData;
    parameters.Get ("tables", tablesData);

    return WithAC27PreferenceTableContainer (domain, [&] (auto& container) -> GS::ObjectState {
        std::vector<std::optional<GS::ObjectState>> results (tablesData.GetSize ());
        // One Modify of the container, so the whole call is one undo step.
        ACAPI::Result<void> modifyResult = container.Modify ([&] (PreferenceTableContainerBase::Modifier& modifier) {
            for (UIndex i = 0; i < tablesData.GetSize (); ++i) {
                GS::UniString name;
                tablesData[i].Get ("name", name);
                auto id = modifier.AddNewTable (name);
                if (id.IsErr ()) {
                    results[i] = CreateFailedExecutionResult (id.UnwrapErr ().kind, AC27PreferenceTableErrorText ("add the table", id.UnwrapErr ()));
                    continue;
                }
                GS::ObjectState result = CreateSuccessfulExecutionResult ();
                result.Add ("tableId", GS::ObjectState ("guid", id->GetGuid ().ToUniString ()));
                results[i] = result;
            }
        }, "Create MEP Preference Tables");

        GS::ObjectState response;
        const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
        for (const std::optional<GS::ObjectState>& result : results) {
            if (modifyResult.IsErr ()) {
                executionResults (CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, AC27PreferenceTableErrorText ("modify the preference table container", modifyResult.UnwrapErr ())));
            } else if (result.has_value ()) {
                executionResults (*result);
            } else {
                executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The preference table container was not modified."));
            }
        }
        return response;
    });
}
#else
GS::ObjectState CreateMEPPreferenceTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
#ifdef ServerMainVers_2800
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command is for Archicad 27; from Archicad 28 the preference tables are added by other commands.");
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27.");
#endif
}
#endif
