#include "ViewAndPlaceSettingsCommands.hpp"
#include "ApplicationCommands.hpp"
#include "MigrationHelper.hpp"

// The grid settings of the current database (API_GridType), the 'Filter and Cut Elements in 3D'
// settings of the 3D window (API_3DFilterAndCutSettings), the search of a dimension standard by
// name and the place settings (API_PlaceInfo: location, date, time and time zone). Every settings
// field is a JSON key with the name of the struct field; the fillers are left out. The function
// forms used here exist from Archicad 27 on, so all of these commands need Archicad 27.

#ifdef ServerMainVers_2700

template <typename Struct>
struct BoolSetting {
    const char* name;
    bool Struct::* field;
};

template <typename Struct>
struct ShortSetting {
    const char* name;
    short Struct::* field;
};

template <typename Struct>
struct DoubleSetting {
    const char* name;
    double Struct::* field;
};

template <typename Struct>
struct UnsignedShortSetting {
    const char*            name;
    unsigned short Struct::* field;
    Int32                  minValue;
    Int32                  maxValue;
};

static const BoolSetting<API_GridType> GridBoolSettings[] = {
    { "gridSwitch", &API_GridType::gridSwitch }
};

static const ShortSetting<API_GridType> GridShortSettings[] = {
    { "nLineMainX", &API_GridType::nLineMainX },
    { "nLineMainY", &API_GridType::nLineMainY },
    { "nLineAuxX",  &API_GridType::nLineAuxX },
    { "nLineAuxY",  &API_GridType::nLineAuxY },
    { "status",     &API_GridType::status }
};

static const DoubleSetting<API_GridType> GridDoubleSettings[] = {
    { "spaceMainX", &API_GridType::spaceMainX },
    { "spaceMainY", &API_GridType::spaceMainY },
    { "spaceAuxX",  &API_GridType::spaceAuxX },
    { "spaceAuxY",  &API_GridType::spaceAuxY },
    { "snapOffsX",  &API_GridType::snapOffsX },
    { "snapOffsY",  &API_GridType::snapOffsY },
    { "snapStepX",  &API_GridType::snapStepX },
    { "snapStepY",  &API_GridType::snapStepY },
    { "gridAngle",  &API_GridType::gridAngle },
    { "gridOffsX",  &API_GridType::gridOffsX },
    { "gridOffsY",  &API_GridType::gridOffsY },
    { "gridOffsZ",  &API_GridType::gridOffsZ }
};

static const BoolSetting<API_3DFilterAndCutSettings> FilterAndCutBoolSettings[] = {
    { "allStories",       &API_3DFilterAndCutSettings::allStories },
    { "trimToStoryRange", &API_3DFilterAndCutSettings::trimToStoryRange }
};

static const ShortSetting<API_3DFilterAndCutSettings> FilterAndCutShortSettings[] = {
    { "firstStory3D", &API_3DFilterAndCutSettings::firstStory3D },
    { "lastStory3D",  &API_3DFilterAndCutSettings::lastStory3D }
};

static const BoolSetting<API_PlaceInfo> PlaceBoolSettings[] = {
    { "sumTime", &API_PlaceInfo::sumTime }
};

static const ShortSetting<API_PlaceInfo> PlaceShortSettings[] = {
    { "timeZoneInMinutes", &API_PlaceInfo::timeZoneInMinutes },
    { "timeZoneOffset",    &API_PlaceInfo::timeZoneOffset }
};

static const DoubleSetting<API_PlaceInfo> PlaceDoubleSettings[] = {
    { "longitude", &API_PlaceInfo::longitude },
    { "latitude",  &API_PlaceInfo::latitude },
    { "altitude",  &API_PlaceInfo::altitude },
    { "north",     &API_PlaceInfo::north }
};

static const UnsignedShortSetting<API_PlaceInfo> PlaceUnsignedShortSettings[] = {
    { "year",   &API_PlaceInfo::year,   0, 65535 },
    { "month",  &API_PlaceInfo::month,  1, 12 },
    { "day",    &API_PlaceInfo::day,    1, 31 },
    { "hour",   &API_PlaceInfo::hour,   0, 23 },
    { "minute", &API_PlaceInfo::minute, 0, 59 },
    { "second", &API_PlaceInfo::second, 0, 59 }
};

struct FilterModeName {
    API_3DFilterModeID value;
    const char*        name;
};

static const FilterModeName FilterModeNames[] = {
    { API_FilterByRules,           "FilterByRules" },
    { API_FilterBySelection,       "FilterBySelection" },
    { API_FilterByMarqueeAndRules, "FilterByMarqueeAndRules" }
};

// ---------------------------------------------------------------------------------------------
// Settings -> JSON
// ---------------------------------------------------------------------------------------------

template <typename Struct, size_t N>
static void AddSettings (GS::ObjectState& os, const Struct& data, const BoolSetting<Struct> (&settings)[N])
{
    for (const BoolSetting<Struct>& setting : settings) {
        os.Add (setting.name, data.*(setting.field));
    }
}

template <typename Struct, size_t N>
static void AddSettings (GS::ObjectState& os, const Struct& data, const ShortSetting<Struct> (&settings)[N])
{
    for (const ShortSetting<Struct>& setting : settings) {
        os.Add (setting.name, static_cast<Int32> (data.*(setting.field)));
    }
}

template <typename Struct, size_t N>
static void AddSettings (GS::ObjectState& os, const Struct& data, const DoubleSetting<Struct> (&settings)[N])
{
    for (const DoubleSetting<Struct>& setting : settings) {
        os.Add (setting.name, data.*(setting.field));
    }
}

template <typename Struct, size_t N>
static void AddSettings (GS::ObjectState& os, const Struct& data, const UnsignedShortSetting<Struct> (&settings)[N])
{
    for (const UnsignedShortSetting<Struct>& setting : settings) {
        os.Add (setting.name, static_cast<Int32> (data.*(setting.field)));
    }
}

static GS::ObjectState CreateRGBAColorObjectState (const API_RGBAColor& color)
{
    return GS::ObjectState ("red", color.f_red, "green", color.f_green, "blue", color.f_blue, "alpha", color.f_alpha);
}

static GS::ObjectState CreateGridSettingsObjectState (const API_GridType& grid)
{
    GS::ObjectState os;
    AddSettings (os, grid, GridBoolSettings);
    AddSettings (os, grid, GridShortSettings);
    AddSettings (os, grid, GridDoubleSettings);
    os.Add ("gridMode", static_cast<Int32> (grid.gridMode));
    os.Add ("backColor", CreateRGBAColorObjectState (grid.backColor));
    os.Add ("gridColor", CreateRGBAColorObjectState (grid.gridColor));
    return os;
}

static GS::ObjectState CreateFilterAndCutSettingsObjectState (const API_3DFilterAndCutSettings& filter)
{
    GS::ObjectState os;
    AddSettings (os, filter, FilterAndCutBoolSettings);
    AddSettings (os, filter, FilterAndCutShortSettings);

    GS::UniString filterMode = "Unknown";
    for (const FilterModeName& item : FilterModeNames) {
        if (item.value == filter.filterMode) {
            filterMode = item.name;
        }
    }
    os.Add ("filterMode", filterMode);

    const auto& elemTypeFilter = os.AddList<GS::ObjectState> ("elemTypeFilter");
    for (const auto& entry : filter.elemTypeFilter) {
        const GS::UniString typeName = GetElementTypeNonLocalizedName (GetElemTypeId (entry.first));
        if (typeName != "Unknown") {
            elemTypeFilter (GS::ObjectState ("elementType", typeName, "show", static_cast<bool> (entry.second)));
        }
    }
    return os;
}

static GS::ObjectState CreatePlaceSettingsObjectState (const API_PlaceInfo& placeInfo)
{
    GS::ObjectState os;
    AddSettings (os, placeInfo, PlaceDoubleSettings);
    AddSettings (os, placeInfo, PlaceUnsignedShortSettings);
    AddSettings (os, placeInfo, PlaceBoolSettings);
    AddSettings (os, placeInfo, PlaceShortSettings);
    os.Add ("sunAngXY", placeInfo.sunAngXY);
    os.Add ("sunAngZ", placeInfo.sunAngZ);
    return os;
}

// ---------------------------------------------------------------------------------------------
// JSON -> settings: only the given keys are changed, the first invalid value stops the reading
// ---------------------------------------------------------------------------------------------

class SettingsInput
{
public:
    explicit SettingsInput (const GS::ObjectState& input) : os (input) {}

    bool Failed () const { return !error.IsEmpty (); }
    const GS::UniString& GetError () const { return error; }

    template <typename Struct, size_t N>
    void Read (Struct& data, const BoolSetting<Struct> (&settings)[N])
    {
        for (const BoolSetting<Struct>& setting : settings) {
            bool value = false;
            if (Has (setting.name) && Check (os.Get (setting.name, value), setting.name, "a boolean")) {
                data.*(setting.field) = value;
            }
        }
    }

    template <typename Struct, size_t N>
    void Read (Struct& data, const ShortSetting<Struct> (&settings)[N])
    {
        for (const ShortSetting<Struct>& setting : settings) {
            Int32 value = 0;
            if (ReadInteger (setting.name, -32768, 32767, value)) {
                data.*(setting.field) = static_cast<short> (value);
            }
        }
    }

    template <typename Struct, size_t N>
    void Read (Struct& data, const DoubleSetting<Struct> (&settings)[N])
    {
        for (const DoubleSetting<Struct>& setting : settings) {
            double value = 0.0;
            if (Has (setting.name) && Check (os.Get (setting.name, value), setting.name, "a number")) {
                data.*(setting.field) = value;
            }
        }
    }

    template <typename Struct, size_t N>
    void Read (Struct& data, const UnsignedShortSetting<Struct> (&settings)[N])
    {
        for (const UnsignedShortSetting<Struct>& setting : settings) {
            Int32 value = 0;
            if (ReadInteger (setting.name, setting.minValue, setting.maxValue, value)) {
                data.*(setting.field) = static_cast<unsigned short> (value);
            }
        }
    }

    bool ReadInteger (const char* name, Int32 minValue, Int32 maxValue, Int32& value)
    {
        if (!Has (name)) {
            return false;
        }
        const bool valid = os.Get (name, value) && value >= minValue && value <= maxValue;
        return Check (valid, name, GS::UniString::Printf ("an integer from %d to %d", minValue, maxValue));
    }

    void ReadColor (const char* name, API_RGBAColor& color)
    {
        GS::ObjectState colorOs;
        if (!Has (name) || !Check (os.Get (name, colorOs), name, "an object")) {
            return;
        }
        API_RGBAColor newColor = color;
        const auto readComponent = [&] (const char* component, double& target) -> bool {
            double value = 0.0;
            if (!colorOs.Contains (component)) {
                return true;
            }
            if (!colorOs.Get (component, value) || value < 0.0 || value > 1.0) {
                return false;
            }
            target = value;
            return true;
        };
        const bool valid = readComponent ("red", newColor.f_red) &&
                           readComponent ("green", newColor.f_green) &&
                           readComponent ("blue", newColor.f_blue) &&
                           readComponent ("alpha", newColor.f_alpha);
        if (Check (valid, name, "a color whose red, green, blue and alpha are numbers from 0 to 1")) {
            color = newColor;
        }
    }

    void ReadFilterMode (API_3DFilterModeID& filterMode)
    {
        GS::UniString value;
        if (!Has ("filterMode")) {
            return;
        }
        if (os.Get ("filterMode", value)) {
            for (const FilterModeName& item : FilterModeNames) {
                if (value == item.name) {
                    filterMode = item.value;
                    return;
                }
            }
        }
        Check (false, "filterMode", "one of FilterByRules, FilterBySelection and FilterByMarqueeAndRules");
    }

    // The listed element types are changed, the others keep their current entry.
    void ReadElemTypeFilter (API_3DFilterAndCutSettings& filter)
    {
        GS::Array<GS::ObjectState> entries;
        if (!Has ("elemTypeFilter") || !Check (os.Get ("elemTypeFilter", entries), "elemTypeFilter", "an array")) {
            return;
        }
        for (const GS::ObjectState& entry : entries) {
            GS::UniString typeName;
            bool show = false;
            const API_ElemTypeID typeID = entry.Get ("elementType", typeName) ? GetElementTypeFromNonLocalizedName (typeName) : API_ZombieElemID;
            if (!Check (typeID != API_ZombieElemID && entry.Get ("show", show), "elemTypeFilter", "an array of {elementType, show} with known element types")) {
                return;
            }
            filter.elemTypeFilter[typeID] = show;
        }
    }

private:
    bool Has (const char* name) const
    {
        return !Failed () && os.Contains (name);
    }

    bool Check (bool valid, const char* name, const GS::UniString& expected)
    {
        if (!valid && !Failed ()) {
            error = GS::UniString ("Invalid parameter: ") + name + " must be " + expected + ".";
        }
        return valid;
    }

    GS::ObjectState os;
    GS::UniString   error;
};

// The grid settings with the database they belong to, the database of the current window.
static GS::ObjectState CreateGridSettingsResponse (const API_GridType& grid)
{
    API_DatabaseInfo dbInfo = {};
    const GSErrCode err = ACAPI_Database_GetCurrentDatabase (&dbInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current database.");
    }
    GS::ObjectState database = CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (dbInfo));
    database.Add ("windowType", ConvertWindowTypeToString (dbInfo.typeID));

    return GS::ObjectState (
        "database", database,
        "settings", CreateGridSettingsObjectState (grid));
}

#else

static GS::ObjectState CreateRequiresArchicad27Response ()
{
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
}

#endif

// ---------------------------------------------------------------------------------------------
// GetGridSettings / SetGridSettings
// ---------------------------------------------------------------------------------------------

static const char* ColorSchema = R"({
                "type": "object",
                "description": "A color with transparency, each component from 0 to 1.",
                "properties": {
                    "red": { "type": "number", "minimum": 0, "maximum": 1 },
                    "green": { "type": "number", "minimum": 0, "maximum": 1 },
                    "blue": { "type": "number", "minimum": 0, "maximum": 1 },
                    "alpha": { "type": "number", "minimum": 0, "maximum": 1 }
                },
                "additionalProperties": false
            })";

static GS::UniString GetGridSettingsSchema (const GS::UniString& description)
{
    return GS::UniString (R"({
        "type": "object",
        "description": ")") + description + R"(",
        "properties": {
            "spaceMainX": { "type": "number", "description": "The horizontal spacing of the main grid, in meters." },
            "spaceMainY": { "type": "number", "description": "The vertical spacing of the main grid, in meters." },
            "nLineMainX": { "type": "integer", "description": "The number of main horizontal grid lines." },
            "nLineMainY": { "type": "integer", "description": "The number of main vertical grid lines." },
            "gridSwitch": { "type": "boolean", "description": "Whether the grid is visible." },
            "spaceAuxX": { "type": "number", "description": "The horizontal spacing of the auxiliary grid, in meters." },
            "spaceAuxY": { "type": "number", "description": "The vertical spacing of the auxiliary grid, in meters." },
            "nLineAuxX": { "type": "integer", "description": "The number of horizontal auxiliary grid lines." },
            "nLineAuxY": { "type": "integer", "description": "The number of vertical auxiliary grid lines." },
            "gridMode": { "type": "integer", "minimum": 0, "maximum": 2, "description": "0: none, 1: grid, 2: snap." },
            "snapOffsX": { "type": "number", "description": "The horizontal offset of the snap grid, in meters." },
            "snapOffsY": { "type": "number", "description": "The vertical offset of the snap grid, in meters." },
            "snapStepX": { "type": "number", "description": "The horizontal spacing of the snap grid, in meters." },
            "snapStepY": { "type": "number", "description": "The vertical spacing of the snap grid, in meters." },
            "status": { "type": "integer", "description": "The status of the grid, as Archicad stores it." },
            "gridAngle": { "type": "number", "description": "The rotation angle of the grid." },
            "gridOffsX": { "type": "number", "description": "The X offset of the grid." },
            "gridOffsY": { "type": "number", "description": "The Y offset of the grid." },
            "gridOffsZ": { "type": "number", "description": "The Z offset of the grid." },
            "backColor": )" + ColorSchema + R"(,
            "gridColor": )" + ColorSchema + R"(
        },
        "additionalProperties": false
    })";
}

static GS::UniString GetGridSettingsResponseSchema ()
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "database": {
            "type": "object",
            "description": "The database whose grid settings these are: the database of the current window. Each database has its own grid settings.",
            "properties": {
                "databaseId": {
                    "$ref": "#/DatabaseId"
                },
                "windowType": {
                    "$ref": "#/WindowType"
                }
            },
            "additionalProperties": false,
            "required": [
                "databaseId",
                "windowType"
            ]
        },
        "settings": )") + GetGridSettingsSchema ("The grid settings (API_GridType), with its field names.") + R"(
    },
    "additionalProperties": false,
    "required": [
        "database",
        "settings"
    ]
})";
}

GetGridSettingsCommand::GetGridSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetGridSettingsCommand::GetName () const
{
    return "GetGridSettings";
}

GS::Optional<GS::UniString> GetGridSettingsCommand::GetRawResponseSchema () const
{
    return GetGridSettingsResponseSchema ();
}

GS::ObjectState GetGridSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_GridType grid = {};
    GSErrCode err = ACAPI_Window_GetGridSettings (&grid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the grid settings.");
    }

    return CreateGridSettingsResponse (grid);
#else
    return CreateRequiresArchicad27Response ();
#endif
}

SetGridSettingsCommand::SetGridSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetGridSettingsCommand::GetName () const
{
    return "SetGridSettings";
}

GS::Optional<GS::UniString> SetGridSettingsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "settings": )") + GetGridSettingsSchema ("The grid settings to change, in the form GetGridSettings returns. Only the given keys are changed; a color may give only some of its components.") + R"(
    },
    "additionalProperties": false,
    "required": [
        "settings"
    ]
})";
}

GS::Optional<GS::UniString> SetGridSettingsCommand::GetRawResponseSchema () const
{
    return GetGridSettingsResponseSchema ();
}

GS::ObjectState SetGridSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::ObjectState settingsOs;
    if (!parameters.Get ("settings", settingsOs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: settings.");
    }

    // Start from the current settings so every value that is not given is kept.
    API_GridType grid = {};
    GSErrCode err = ACAPI_Window_GetGridSettings (&grid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current grid settings.");
    }

    SettingsInput input (settingsOs);
    input.Read (grid, GridBoolSettings);
    input.Read (grid, GridShortSettings);
    input.Read (grid, GridDoubleSettings);
    Int32 gridMode = 0;
    if (input.ReadInteger ("gridMode", 0, 2, gridMode)) {
        grid.gridMode = static_cast<unsigned char> (gridMode);
    }
    input.ReadColor ("backColor", grid.backColor);
    input.ReadColor ("gridColor", grid.gridColor);
    if (input.Failed ()) {
        return CreateErrorResponse (APIERR_BADPARS, input.GetError ());
    }

    err = ACAPI_CallUndoableCommand ("SetGridSettingsCommand", [&] () -> GSErrCode {
        return ACAPI_Window_ChangeGridSettings (&grid);
    });
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to change the grid settings.");
    }

    API_GridType newGrid = {};
    err = ACAPI_Window_GetGridSettings (&newGrid);
    if (err != NoError) {
        return CreateErrorResponse (err, "The grid settings were changed, but reading them back failed.");
    }

    return CreateGridSettingsResponse (newGrid);
#else
    UNUSED_PARAMETER (parameters);
    return CreateRequiresArchicad27Response ();
#endif
}

// ---------------------------------------------------------------------------------------------
// Get3DFilterAndCutSettings / Set3DFilterAndCutSettings
// ---------------------------------------------------------------------------------------------

static GS::UniString GetFilterAndCutSettingsSchema (const GS::UniString& description)
{
    return GS::UniString (R"({
        "type": "object",
        "description": ")") + description + R"(",
        "properties": {
            "firstStory3D": { "type": "integer", "description": "The first story shown." },
            "lastStory3D": { "type": "integer", "description": "The last story shown." },
            "allStories": { "type": "boolean", "description": "Whether all stories are shown; if false, firstStory3D to lastStory3D." },
            "trimToStoryRange": { "type": "boolean" },
            "filterMode": { "type": "string", "enum": ["FilterByRules", "FilterBySelection", "FilterByMarqueeAndRules"] },
            "elemTypeFilter": {
                "type": "array",
                "description": "Whether each element type is shown. On Set only the listed element types are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementType": { "$ref": "#/ElementType" },
                        "show": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["elementType", "show"]
                }
            }
        },
        "additionalProperties": false
    })";
}

static GS::UniString GetFilterAndCutResponseSchema ()
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "settings": )") + GetFilterAndCutSettingsSchema ("The 'Filter and Cut Elements in 3D' settings of the 3D window (API_3DFilterAndCutSettings), with its field names. The marquee filter is not included.") + R"(
    },
    "additionalProperties": false,
    "required": [
        "settings"
    ]
})";
}

Get3DFilterAndCutSettingsCommand::Get3DFilterAndCutSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Get3DFilterAndCutSettingsCommand::GetName () const
{
    return "Get3DFilterAndCutSettings";
}

GS::Optional<GS::UniString> Get3DFilterAndCutSettingsCommand::GetRawResponseSchema () const
{
    return GetFilterAndCutResponseSchema ();
}

GS::ObjectState Get3DFilterAndCutSettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_3DFilterAndCutSettings filter {};
    GSErrCode err = ACAPI_View_Get3DImageSets (&filter);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 3D filter and cut settings.");
    }

    return GS::ObjectState ("settings", CreateFilterAndCutSettingsObjectState (filter));
#else
    return CreateRequiresArchicad27Response ();
#endif
}

Set3DFilterAndCutSettingsCommand::Set3DFilterAndCutSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Set3DFilterAndCutSettingsCommand::GetName () const
{
    return "Set3DFilterAndCutSettings";
}

GS::Optional<GS::UniString> Set3DFilterAndCutSettingsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "settings": )") + GetFilterAndCutSettingsSchema ("The settings to change, in the form Get3DFilterAndCutSettings returns. Only the given keys are changed.") + R"(,
        "rebuild": {
            "type": "boolean",
            "description": "If true, Archicad drops the partial conversion data, so the next rebuild of the 3D window is a full one. Default: false."
        }
    },
    "additionalProperties": false,
    "required": [
        "settings"
    ]
})";
}

GS::Optional<GS::UniString> Set3DFilterAndCutSettingsCommand::GetRawResponseSchema () const
{
    return GetFilterAndCutResponseSchema ();
}

GS::ObjectState Set3DFilterAndCutSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::ObjectState settingsOs;
    if (!parameters.Get ("settings", settingsOs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: settings.");
    }
    bool mustConvert = false;
    if (parameters.Contains ("rebuild") && !parameters.Get ("rebuild", mustConvert)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: rebuild must be a boolean.");
    }

    // Start from the current settings so every value that is not given is kept.
    API_3DFilterAndCutSettings filter {};
    GSErrCode err = ACAPI_View_Get3DImageSets (&filter);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current 3D filter and cut settings.");
    }

    SettingsInput input (settingsOs);
    input.Read (filter, FilterAndCutBoolSettings);
    input.Read (filter, FilterAndCutShortSettings);
    input.ReadFilterMode (filter.filterMode);
    input.ReadElemTypeFilter (filter);
    if (input.Failed ()) {
        return CreateErrorResponse (APIERR_BADPARS, input.GetError ());
    }

    err = ACAPI_CallUndoableCommand ("Set3DFilterAndCutSettingsCommand", [&] () -> GSErrCode {
        return ACAPI_View_Change3DImageSets (&filter, &mustConvert);
    });
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to change the 3D filter and cut settings.");
    }

    API_3DFilterAndCutSettings newFilter {};
    err = ACAPI_View_Get3DImageSets (&newFilter);
    if (err != NoError) {
        return CreateErrorResponse (err, "The 3D filter and cut settings were changed, but reading them back failed.");
    }

    return GS::ObjectState ("settings", CreateFilterAndCutSettingsObjectState (newFilter));
#else
    UNUSED_PARAMETER (parameters);
    return CreateRequiresArchicad27Response ();
#endif
}

// ---------------------------------------------------------------------------------------------
// FindDimensionStandard
// ---------------------------------------------------------------------------------------------

FindDimensionStandardCommand::FindDimensionStandardCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String FindDimensionStandardCommand::GetName () const
{
    return "FindDimensionStandard";
}

GS::Optional<GS::UniString> FindDimensionStandardCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "name": {
            "type": "string",
            "description": "The name of the dimension standard, as listed in Options > Project Preferences > Dimensions.",
            "minLength": 1
        }
    },
    "additionalProperties": false,
    "required": [
        "name"
    ]
})";
}

GS::Optional<GS::UniString> FindDimensionStandardCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "index": {
            "type": "integer",
            "description": "The index of the dimension standard."
        },
        "name": {
            "type": "string",
            "description": "The name of the dimension standard."
        },
        "dimensionStandardId": {
            "$ref": "#/GuidId",
            "description": "The guid of the dimension standard."
        }
    },
    "additionalProperties": false,
    "required": [
        "index",
        "name",
        "dimensionStandardId"
    ]
})";
}

GS::ObjectState FindDimensionStandardCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: name.");
    }

    // Archicad searches by the name uniStringNamePtr points at and fills in the index and the
    // guid, the way ACAPI_Attribute_Search does for attributes.
    GS::UniString foundName = name;
    API_DimensionStandards_Head head = {};
    head.uniStringNamePtr = &foundName;
    CHTruncate (name.ToCStr ().Get (), head.name, sizeof (head.name));
    GSErrCode err = ACAPI_Navigator_DimStand_Search (&head);
    if (err != NoError) {
        return CreateErrorResponse (err, "No dimension standard was found with this name.");
    }

    return GS::ObjectState (
        "index", static_cast<Int32> (head.index),
        "name", foundName.IsEmpty () ? GS::UniString (head.name) : foundName,
        "dimensionStandardId", CreateGuidObjectState (head.guid));
#else
    UNUSED_PARAMETER (parameters);
    return CreateRequiresArchicad27Response ();
#endif
}

// ---------------------------------------------------------------------------------------------
// SetPlaceSettings
// ---------------------------------------------------------------------------------------------

SetPlaceSettingsCommand::SetPlaceSettingsCommand () :
    CommandBase (CommonSchema::NotUsed)
{}

GS::String SetPlaceSettingsCommand::GetName () const
{
    return "SetPlaceSettings";
}

GS::Optional<GS::UniString> SetPlaceSettingsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "placeInfo": {
            "type": "object",
            "description": "The place settings to change (API_PlaceInfo), with its field names. Only the given keys are changed; an empty object changes nothing and returns the current settings.",
            "properties": {
                "longitude": { "type": "number", "description": "Longitude in degrees." },
                "latitude": { "type": "number", "description": "Latitude in degrees." },
                "altitude": { "type": "number", "description": "Altitude in meters." },
                "north": { "type": "number", "description": "The north direction in radians." },
                "year": { "type": "integer", "minimum": 0, "maximum": 65535 },
                "month": { "type": "integer", "minimum": 1, "maximum": 12 },
                "day": { "type": "integer", "minimum": 1, "maximum": 31 },
                "hour": { "type": "integer", "minimum": 0, "maximum": 23 },
                "minute": { "type": "integer", "minimum": 0, "maximum": 59 },
                "second": { "type": "integer", "minimum": 0, "maximum": 59 },
                "sumTime": { "type": "boolean", "description": "Whether summer time (+1 hour) is on." },
                "timeZoneInMinutes": { "type": "integer", "description": "The time zone, in minutes from Greenwich." },
                "timeZoneOffset": { "type": "integer", "description": "Tells apart time zones with the same timeZoneInMinutes." }
            },
            "additionalProperties": false
        }
    },
    "additionalProperties": false,
    "required": [
        "placeInfo"
    ]
})";
}

GS::Optional<GS::UniString> SetPlaceSettingsCommand::GetRawResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "placeInfo": {
            "type": "object",
            "description": "The place settings read back after the change. sunAngXY and sunAngZ are the sun position Archicad calculated for the place, date and time, in radians.",
            "properties": {
                "longitude": { "type": "number" },
                "latitude": { "type": "number" },
                "altitude": { "type": "number" },
                "north": { "type": "number" },
                "year": { "type": "integer" },
                "month": { "type": "integer" },
                "day": { "type": "integer" },
                "hour": { "type": "integer" },
                "minute": { "type": "integer" },
                "second": { "type": "integer" },
                "sumTime": { "type": "boolean" },
                "timeZoneInMinutes": { "type": "integer" },
                "timeZoneOffset": { "type": "integer" },
                "sunAngXY": { "type": "number" },
                "sunAngZ": { "type": "number" }
            },
            "additionalProperties": false,
            "required": [
                "longitude",
                "latitude",
                "altitude",
                "north",
                "year",
                "month",
                "day",
                "hour",
                "minute",
                "second",
                "sumTime",
                "timeZoneInMinutes",
                "timeZoneOffset",
                "sunAngXY",
                "sunAngZ"
            ]
        }
    },
    "additionalProperties": false,
    "required": [
        "placeInfo"
    ]
})";
}

GS::ObjectState SetPlaceSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::ObjectState placeInfoOs;
    if (!parameters.Get ("placeInfo", placeInfoOs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: placeInfo.");
    }

    // Start from the current settings so every value that is not given is kept.
    API_PlaceInfo placeInfo = {};
    GSErrCode err = ACAPI_GeoLocation_GetPlaceSets (&placeInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current place settings.");
    }

    SettingsInput input (placeInfoOs);
    input.Read (placeInfo, PlaceDoubleSettings);
    input.Read (placeInfo, PlaceUnsignedShortSettings);
    input.Read (placeInfo, PlaceBoolSettings);
    input.Read (placeInfo, PlaceShortSettings);
    if (input.Failed ()) {
        return CreateErrorResponse (APIERR_BADPARS, input.GetError ());
    }

    if (!placeInfoOs.IsEmpty ()) {
        // Without a dialog: only asks whether the place settings can be changed now
        // (in a Teamwork project they may be reserved by someone else).
        bool withDialog = false;
        err = ACAPI_GeoLocation_CanChangePlaceSets (&withDialog);
        if (err != NoError) {
            return CreateErrorResponse (err, "The place settings cannot be changed now (in a Teamwork project, reserve them first).");
        }

        err = ACAPI_CallUndoableCommand ("SetPlaceSettingsCommand", [&] () -> GSErrCode {
            return ACAPI_GeoLocation_ChangePlaceSets (&placeInfo);
        });
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to change the place settings.");
        }
    }

    API_PlaceInfo newPlaceInfo = {};
    err = ACAPI_GeoLocation_GetPlaceSets (&newPlaceInfo);
    if (err != NoError) {
        return CreateErrorResponse (err, "The place settings were changed, but reading them back failed.");
    }

    return GS::ObjectState ("placeInfo", CreatePlaceSettingsObjectState (newPlaceInfo));
#else
    UNUSED_PARAMETER (parameters);
    return CreateRequiresArchicad27Response ();
#endif
}
