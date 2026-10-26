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
#endif

// The rows of the preference tables, found by their key: the reference ids and, for branch and elbow rows
// from Archicad 29, the angle. Archicad 28 finds the rows by the reference ids only.

static const char* PreferenceTableRowKeySchema = R"({
                "type": "object",
                "description": "The key of a row. Segment tables: referenceId. Elbow tables: referenceId and angle. Branch tables: mainAxisReferenceId, branchAxisReferenceId and branchAxisAngle. Transition tables: beginReferenceId and endReferenceId. The angles are as GetMEPPreferenceTables returns them; Archicad 28 finds the rows by their reference ids only and rejects a key with an angle.",
                "properties": {
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
                "description": "The values of a row, lengths in meters. Segment tables: diameter, wallThickness and description. Elbow tables: diameter, radius and description. Branch tables: mainAxisDiameter, branchAxisDiameter, length, branchOffset, connectionLength and description. Pipe transition tables: beginDiameter, endDiameter, length and description. Duct transition tables: beginDiameter, endDiameter, angle and description.",
                "properties": {
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
                "description": "The guid of a preference table, as returned by GetMEPPreferenceTables (the branch, elbow and transition tables with includeFittingTables): a pipe or circular duct segment table, a branch or elbow table of the Piping or Ventilation domain, or, from Archicad 29, a transition table. Rectangular duct and cable carrier segment tables are found too, but their rows have no key.",
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
    "referenceId", "angle", "mainAxisReferenceId", "branchAxisReferenceId", "branchAxisAngle", "beginReferenceId", "endReferenceId"
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
    "beginDiameter", "endDiameter", "angle"
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
                                    "description": "Whether the row is valid."
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
#else
GS::ObjectState GetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
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
#else
GS::ObjectState SetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
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
                "description": "The rows to add, with their key and optionally their values. Branch tables from Archicad 28, elbow and transition tables from Archicad 29. Segment tables have a row for each reference id, so no row can be added to them.",
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
                                    "description": "The index of the new row in the table."
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
#else
GS::ObjectState AddMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
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
                "description": "The keys of the rows to delete. Segment tables, and the elbow tables of Archicad 28, have a row for each reference id, which is emptied instead.",
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
#else
GS::ObjectState DeleteMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
}
#endif
