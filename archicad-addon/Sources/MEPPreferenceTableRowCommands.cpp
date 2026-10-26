#include "MEPPreferenceTableRowCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/MEPAdapter.hpp"
#include "ACAPI/MEPPipeElbowPreferenceTable.hpp"
#include "ACAPI/MEPPipeBranchPreferenceTable.hpp"
#include "ACAPI/MEPDuctElbowPreferenceTable.hpp"
#include "ACAPI/MEPDuctBranchPreferenceTable.hpp"

#include <cmath>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

using namespace ACAPI::MEP;
#endif

// The rows of the branch and elbow preference tables, found by their key: the reference ids and, from
// Archicad 29, the angle. Archicad 28 finds them by the reference ids only.

static const char* PreferenceTableRowKeySchema = R"({
                "type": "object",
                "description": "The key of a row. Elbow tables: referenceId and angle. Branch tables: mainAxisReferenceId, branchAxisReferenceId and branchAxisAngle. The angles are as GetMEPPreferenceTables returns them; Archicad 28 finds the rows by their reference ids only and rejects a key with an angle.",
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
                    }
                },
                "additionalProperties": false
            })";

static const char* PreferenceTableRowValuesSchema = R"({
                "type": "object",
                "description": "The values of a row, lengths in meters. Elbow tables: diameter, radius and description. Branch tables: mainAxisDiameter, branchAxisDiameter, length, branchOffset, connectionLength and description.",
                "properties": {
                    "diameter": {
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
                    "description": {
                        "type": "string"
                    }
                },
                "additionalProperties": false
            })";

static const char* PreferenceTableIdSchema = R"({
                "type": "object",
                "description": "The guid of a branch or elbow preference table of the Piping or Ventilation domain, as returned by GetMEPPreferenceTables with includeFittingTables.",
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

struct PreferenceTableRowKey {
    uint32_t referenceId = 0;
    uint32_t mainAxisReferenceId = 0;
    uint32_t branchAxisReferenceId = 0;
    // The angle of an elbow row or the branch axis angle of a branch row, from Archicad 29.
    double angle = 0.0;
};

template <typename ErrorT>
static GS::UniString PreferenceTableRowErrorText (const GS::UniString& what, const ErrorT& error)
{
    return "Failed to " + what + ": " + GS::UniString (error.text.c_str ());
}

// Reads the key of a row of a table of type TableT; gives the error message of a key that does not fit.
template <typename TableT>
static std::optional<GS::UniString> ParsePreferenceTableRowKey (const GS::ObjectState& keyData, PreferenceTableRowKey& key)
{
    const auto getReferenceId = [&] (const char* name, uint32_t& referenceId) {
        Int32 value = -1;
        if (!keyData.Get (name, value) || value < 0) {
            return false;
        }
        referenceId = static_cast<uint32_t> (value);
        return true;
    };

    const char* angleName = nullptr;
    if constexpr (IsBranchRowTable<TableT>) {
        if (keyData.Contains ("referenceId") || keyData.Contains ("angle")) {
            return GS::UniString ("The key of a branch table row has mainAxisReferenceId, branchAxisReferenceId and branchAxisAngle.");
        }
        if (!getReferenceId ("mainAxisReferenceId", key.mainAxisReferenceId) || !getReferenceId ("branchAxisReferenceId", key.branchAxisReferenceId)) {
            return GS::UniString ("The key of a branch table row needs 'mainAxisReferenceId' and 'branchAxisReferenceId'.");
        }
        angleName = "branchAxisAngle";
    } else {
        if (keyData.Contains ("mainAxisReferenceId") || keyData.Contains ("branchAxisReferenceId") || keyData.Contains ("branchAxisAngle")) {
            return GS::UniString ("The key of an elbow table row has referenceId and angle.");
        }
        if (!getReferenceId ("referenceId", key.referenceId)) {
            return GS::UniString ("The key of an elbow table row needs 'referenceId'.");
        }
        angleName = "angle";
    }

#ifdef ServerMainVers_2900
    if (!keyData.Get (angleName, key.angle)) {
        return GS::UniString ("The key needs '") + angleName + "'.";
    }
#else
    if (keyData.Contains (angleName)) {
        return GS::UniString ("Archicad 28 finds the rows by their reference ids only, so the key cannot have '") + angleName + "'.";
    }
#endif
    return std::nullopt;
}

#ifdef ServerMainVers_2900
// The angle of the row with the reference ids of the key, if it differs from the angle of the key only by
// the rounding of JSON, so that the row is still found.
template <typename TableT>
static double GetExactPreferenceTableRowAngle (const TableT& table, const PreferenceTableRowKey& key)
{
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
    return key.angle;
}

// The Key struct of the table, from Archicad 29.
template <typename TableT>
static typename TableT::Key MakePreferenceTableRowKey (const PreferenceTableRowKey& key)
{
    typename TableT::Key tableKey {};
    if constexpr (IsBranchRowTable<TableT>) {
        tableKey.mainAxisReferenceId = key.mainAxisReferenceId;
        tableKey.branchAxisReferenceId = key.branchAxisReferenceId;
        tableKey.branchAxisAngle = key.angle;
    } else {
        tableKey.referenceId = key.referenceId;
        tableKey.angle = key.angle;
    }
    return tableKey;
}
#else
// Whether the Modifier of an Archicad 28 table adds a row by the reference ids of its axes.
template <typename ModifierT, typename = void>
struct HasAddNewRowByReferenceIds : std::false_type {};

template <typename ModifierT>
struct HasAddNewRowByReferenceIds<ModifierT, std::void_t<decltype (std::declval<ModifierT&> ().AddNewRow (uint32_t (), uint32_t ()))>> : std::true_type {};
#endif

static const char* const BranchRowValueNames[] = { "mainAxisDiameter", "branchAxisDiameter", "length", "branchOffset", "connectionLength" };
static const char* const ElbowRowValueNames[] = { "diameter", "radius" };

// Gives the error message of values that do not belong to the table type.
template <typename TableT>
static std::optional<GS::UniString> CheckPreferenceTableRowValues (const GS::ObjectState& values)
{
    if constexpr (IsBranchRowTable<TableT>) {
        for (const char* name : ElbowRowValueNames) {
            if (values.Contains (name)) {
                return GS::UniString ("The value '") + name + "' does not belong to a branch table row.";
            }
        }
    } else {
        for (const char* name : BranchRowValueNames) {
            if (values.Contains (name)) {
                return GS::UniString ("The value '") + name + "' does not belong to an elbow table row.";
            }
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
    } else {
#ifdef ServerMainVers_2900
        addValue ("diameter", table.GetDiameterByKey (tableKey));
        addValue ("radius", table.GetRadiusByKey (tableKey));
        addValue ("description", table.GetDescriptionByKey (tableKey));
#else
        addValue ("diameter", table.GetDiameterByReferenceId (key.referenceId));
        addValue ("radius", table.GetRadiusByReferenceId (key.referenceId));
        addValue ("description", table.GetDescriptionByReferenceId (key.referenceId));
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
    } else {
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

// Calls func with the branch or elbow preference table of the tableId parameter.
template <typename FuncT>
static GS::ObjectState WithBranchOrElbowPreferenceTable (const GS::ObjectState& parameters, const FuncT& func)
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
    return CreateErrorResponse (APIERR_BADID, "The tableId is not a branch or elbow preference table.");
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
            "rows"
        ]
    })";
}

#ifdef ServerMainVers_2800
GS::ObjectState GetMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> keys;
    parameters.Get ("keys", keys);

    return WithBranchOrElbowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        GS::ObjectState response;
        const auto& rows = response.AddList<GS::ObjectState> ("rows");
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

    return WithBranchOrElbowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, rowsData, true);
        return ModifyPreferenceTableRows (table, inputs, "Set MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
            const auto error = SetPreferenceTableRowValues<TableT> (modifier, input.key, input.values);
            return error.has_value ()
                ? CreateFailedExecutionResult (error->first, error->second + " The values before it were set.")
                : CreateSuccessfulExecutionResult ();
        });
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
                "description": "The rows to add, with their key and optionally their values. Archicad 28 can add rows to branch tables only.",
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

    return WithBranchOrElbowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
#ifndef ServerMainVers_2900
        if constexpr (!IsBranchRowTable<TableT> || !HasAddNewRowByReferenceIds<typename TableT::Modifier>::value) {
            // The elbow tables of Archicad 28 have a fixed row for each reference id.
            const char* message = "Adding rows to elbow tables is available from Archicad 29; Archicad 28 has a row for each reference id, set its values with SetMEPPreferenceTableRows.";
            if constexpr (IsBranchRowTable<TableT>) {
                message = "Adding rows to branch tables is not supported in Archicad 28.";
            }
            GS::ObjectState response;
            const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
            for (UIndex i = 0; i < rowsData.GetSize (); ++i) {
                executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, message));
            }
            return response;
        } else
#endif
        {
            std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, rowsData, true);
            return ModifyPreferenceTableRows (table, inputs, "Add MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
#ifdef ServerMainVers_2900
                ACAPI::Result<uint32_t> index = modifier.AddNewRow (MakePreferenceTableRowKey<TableT> (input.key));
#else
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
                "description": "The keys of the rows to delete. The elbow tables of Archicad 28 have a row for each reference id, which is emptied instead.",
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

    return WithBranchOrElbowPreferenceTable (parameters, [&] (auto& table) -> GS::ObjectState {
        using TableT = std::decay_t<decltype (table)>;
        std::vector<PreferenceTableRowInput> inputs = ParsePreferenceTableRowInputs (table, keys, false);
        return ModifyPreferenceTableRows (table, inputs, "Delete MEP Preference Table Rows", [] (typename TableT::Modifier& modifier, const PreferenceTableRowInput& input) {
#ifdef ServerMainVers_2900
            ACAPI::Result<void> result = modifier.DeleteRowByKey (MakePreferenceTableRowKey<TableT> (input.key));
#else
            ACAPI::Result<void> result = [&] () {
                if constexpr (IsBranchRowTable<TableT>) {
                    return modifier.DeleteRowByReferenceIds (input.key.mainAxisReferenceId, input.key.branchAxisReferenceId);
                } else {
                    // The elbow tables of Archicad 28 have a fixed row for each reference id.
                    return modifier.EmptyRowByReferenceId (input.key.referenceId);
                }
            } ();
#endif
            return result.IsErr ()
                ? CreateFailedExecutionResult (result.UnwrapErr ().kind, PreferenceTableRowErrorText ("delete the row", result.UnwrapErr ()))
                : CreateSuccessfulExecutionResult ();
        });
    });
}
#else
GS::ObjectState DeleteMEPPreferenceTableRowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
}
#endif
