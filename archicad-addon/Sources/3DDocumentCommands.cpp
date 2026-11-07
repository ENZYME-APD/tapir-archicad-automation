#include "3DDocumentCommands.hpp"
#include "MigrationHelper.hpp"

// Both commands mirror API_DocumentFrom3DType, the settings of a 3D Document. Every field is a
// JSON key of the same name, nested structs are objects with their own field names. Left out:
// the cut shapes handle of cutSetting (with nShapes), the sun settings of the projection,
// the marquee filter of AC26+ and the fillers. Enums whose enumerator names are not used
// anywhere else in the add-on (fillCutSurfacesWith, uncutSurfaceFillType, hiddenEdgeVisibility,
// onCutPlaneVisibility, viewSrc) are passed as their integer values.

template <typename EnumType>
struct EnumName {
    EnumType    value;
    const char* name;
};

static const EnumName<API_CutPlanePlanConn> CutPlanePlanConnNames[] = {
    { APICutPl_Online,  "Online" },
    { APICutPl_Offline, "Offline" },
    { APICutPl_Drawing, "Drawing" }
};

static const EnumName<API_DocumentSourceID> DocumentSourceNames[] = {
    { API_DDD,       "DDD" },
    { API_FloorPlan, "FloorPlan" }
};

static const EnumName<API_BoundaryDisplay> BoundaryDisplayNames[] = {
    { APIBound_UncutContours,    "UncutContours" },
    { APIBound_NoContours,       "NoContours" },
    { APIBound_OverrideContours, "OverrideContours" }
};

static const EnumName<API_3DFilterModeID> FilterModeNames[] = {
    { API_FilterByRules,           "FilterByRules" },
    { API_FilterBySelection,       "FilterBySelection" },
    { API_FilterByMarqueeAndRules, "FilterByMarqueeAndRules" }
};

template <typename EnumType, size_t N>
static GS::UniString EnumToString (EnumType value, const EnumName<EnumType> (&names)[N])
{
    for (const EnumName<EnumType>& item : names) {
        if (item.value == value) {
            return item.name;
        }
    }
    return "Unknown";
}

template <typename Struct>
struct BoolField {
    const char* name;
    bool Struct::* field;
};

template <typename Struct>
struct ShortField {
    const char* name;
    short Struct::* field;
};

template <typename Struct>
struct DoubleField {
    const char* name;
    double Struct::* field;
};

#ifdef ServerMainVers_2600
static const BoolField<API_3DFilterAndCutSettings> FilterAndCutBoolFields[] = {
    { "allStories",       &API_3DFilterAndCutSettings::allStories },
    { "trimToStoryRange", &API_3DFilterAndCutSettings::trimToStoryRange }
};

static const ShortField<API_3DFilterAndCutSettings> FilterAndCutShortFields[] = {
    { "firstStory3D", &API_3DFilterAndCutSettings::firstStory3D },
    { "lastStory3D",  &API_3DFilterAndCutSettings::lastStory3D }
};
#else
static const BoolField<API_3DImageInfo> FilterAndCutBoolFields[] = {
    { "allStories",       &API_3DImageInfo::allStories },
    { "inSelect",         &API_3DImageInfo::inSelect },
    { "trimToMark",       &API_3DImageInfo::trimToMark },
    { "wall",             &API_3DImageInfo::wall },
    { "colu",             &API_3DImageInfo::colu },
    { "symb",             &API_3DImageInfo::symb },
    { "light",            &API_3DImageInfo::light },
    { "ceil",             &API_3DImageInfo::ceil },
    { "mesh",             &API_3DImageInfo::mesh },
    { "roof",             &API_3DImageInfo::roof },
    { "shell",            &API_3DImageInfo::shell },
    { "curtainWall",      &API_3DImageInfo::curtainWall },
    { "wind",             &API_3DImageInfo::wind },
    { "door",             &API_3DImageInfo::door },
    { "skylight",         &API_3DImageInfo::skylight },
    { "morph",            &API_3DImageInfo::morph },
    { "beam",             &API_3DImageInfo::beam },
    { "room",             &API_3DImageInfo::room },
    { "trimToStoryRange", &API_3DImageInfo::trimToStoryRange }
};

static const ShortField<API_3DImageInfo> FilterAndCutShortFields[] = {
    { "firstStory3D",  &API_3DImageInfo::firstStory3D },
    { "lastStory3D",   &API_3DImageInfo::lastStory3D },
    { "markRectFloor", &API_3DImageInfo::markRectFloor },
    { "markVertType",  &API_3DImageInfo::markVertType }
};
#endif

static const BoolField<API_3DCutPlanesInfo> CutSettingBoolFields[] = {
    { "isCutPlanes", &API_3DCutPlanesInfo::isCutPlanes },
    { "useCustom",   &API_3DCutPlanesInfo::useCustom }
};

static const ShortField<API_3DCutPlanesInfo> CutSettingShortFields[] = {
    { "customPen", &API_3DCutPlanesInfo::customPen }
};

static const BoolField<API_PerspPars> PerspBoolFields[] = {
    { "isTwoPointPersp", &API_PerspPars::isTwoPointPersp }
};

static const DoubleField<API_PerspPars> PerspDoubleFields[] = {
    { "cameraZ",   &API_PerspPars::cameraZ },
    { "targetZ",   &API_PerspPars::targetZ },
    { "distance",  &API_PerspPars::distance },
    { "azimuth",   &API_PerspPars::azimuth },
    { "viewCone",  &API_PerspPars::viewCone },
    { "rollAngle", &API_PerspPars::rollAngle }
};

static const BoolField<API_3DWindowInfo> WindowInfoBoolFields[] = {
    { "setWindowSize", &API_3DWindowInfo::setWindowSize },
    { "setZoom",       &API_3DWindowInfo::setZoom }
};

static const ShortField<API_3DWindowInfo> WindowInfoShortFields[] = {
    { "hSize", &API_3DWindowInfo::hSize },
    { "vSize", &API_3DWindowInfo::vSize }
};

static const DoubleField<API_3DWindowInfo> WindowInfoDoubleFields[] = {
    { "zoomScaleX", &API_3DWindowInfo::zoomScaleX },
    { "zoomScaleY", &API_3DWindowInfo::zoomScaleY },
    { "zoomDispX",  &API_3DWindowInfo::zoomDispX },
    { "zoomDispY",  &API_3DWindowInfo::zoomDispY }
};

static const BoolField<API_UniformAttributeOptions> AttributeOptionsBoolFields[] = {
    { "shouldUseUniformCutAttributes", &API_UniformAttributeOptions::shouldUseUniformCutAttributes },
    { "shouldUseUncutSurfaceFill",     &API_UniformAttributeOptions::shouldUseUncutSurfaceFill },
    { "shouldUseElemPen",              &API_UniformAttributeOptions::shouldUseElemPen }
};

static const ShortField<API_UniformAttributeOptions> AttributeOptionsShortFields[] = {
    { "uniformLineType",    &API_UniformAttributeOptions::uniformLineType },
    { "uniformFillPen",     &API_UniformAttributeOptions::uniformFillPen },
    { "uniformFillBkgPen",  &API_UniformAttributeOptions::uniformFillBkgPen },
    { "uncutSurfaceBkgPen", &API_UniformAttributeOptions::uncutSurfaceBkgPen },
    { "elemPen",            &API_UniformAttributeOptions::elemPen },
    { "shadFillPen",        &API_UniformAttributeOptions::shadFillPen },
    { "shadFillBkgPen",     &API_UniformAttributeOptions::shadFillBkgPen },
    { "shadContPen",        &API_UniformAttributeOptions::shadContPen },
    { "hiddenEdgeLinePen",  &API_UniformAttributeOptions::hiddenEdgeLinePen }
};

static const ShortField<API_SkeletonOptions> SkeletonOptionsShortFields[] = {
    { "floorIdx",    &API_SkeletonOptions::floorIdx },
    { "floorOffset", &API_SkeletonOptions::floorOffset }
};

static const DoubleField<API_SkeletonOptions> SkeletonOptionsDoubleFields[] = {
    { "cutHeight", &API_SkeletonOptions::cutHeight },
    { "cutOffset", &API_SkeletonOptions::cutOffset }
};

static const BoolField<API_DocumentFrom3DType> DocumentBoolFields[] = {
    { "cutaway3D",      &API_DocumentFrom3DType::cutaway3D },
    { "materialFrom3D", &API_DocumentFrom3DType::materialFrom3D },
    { "transparency",   &API_DocumentFrom3DType::transparency }
};

static const ShortField<API_DocumentFrom3DType> DocumentShortFields[] = {
    { "hiddenLineLen", &API_DocumentFrom3DType::hiddenLineLen },
    { "effects3D",     &API_DocumentFrom3DType::effects3D },
    { "boundaryPen",   &API_DocumentFrom3DType::boundaryPen }
};

// ---------------------------------------------------------------------------------------------
// Reading the settings into JSON
// ---------------------------------------------------------------------------------------------

template <typename Struct, size_t N>
static void AddFields (GS::ObjectState& os, const Struct& data, const BoolField<Struct> (&fields)[N])
{
    for (const BoolField<Struct>& field : fields) {
        os.Add (field.name, data.*(field.field));
    }
}

template <typename Struct, size_t N>
static void AddFields (GS::ObjectState& os, const Struct& data, const ShortField<Struct> (&fields)[N])
{
    for (const ShortField<Struct>& field : fields) {
        os.Add (field.name, static_cast<Int32> (data.*(field.field)));
    }
}

template <typename Struct, size_t N>
static void AddFields (GS::ObjectState& os, const Struct& data, const DoubleField<Struct> (&fields)[N])
{
    for (const DoubleField<Struct>& field : fields) {
        os.Add (field.name, data.*(field.field));
    }
}

// An attribute that is not found (e.g. index 0, "none") is left out.
static void AddAttribute (GS::ObjectState& os, const char* name, API_AttrTypeID typeID, API_AttributeIndex index)
{
    const API_Guid guid = GetAttributeGuidFromIndex (typeID, index);
    if (guid != APINULLGuid) {
        os.Add (name, CreateAttributeIdObjectState (guid));
    }
}

static GS::ObjectState CreateProjectionObjectState (const API_3DProjectionInfo& projection)
{
    GS::ObjectState os ("isPersp", projection.isPersp);
    if (projection.camGuid != APINULLGuid) {
        os.Add ("camGuid", CreateElementIdObjectState (projection.camGuid));
    }
    if (projection.actCamSet != APINULLGuid) {
        os.Add ("actCamSet", CreateElementIdObjectState (projection.actCamSet));
    }

    if (projection.isPersp) {
        const API_PerspPars& persp = projection.u.persp;
        GS::ObjectState perspOs;
        perspOs.Add ("pos", Create2DCoordinateObjectState (persp.pos));
        perspOs.Add ("target", Create2DCoordinateObjectState (persp.target));
        AddFields (perspOs, persp, PerspBoolFields);
        AddFields (perspOs, persp, PerspDoubleFields);
        os.Add ("persp", perspOs);
    } else {
        GS::ObjectState axonoOs;
        const auto& tranmat = axonoOs.AddList<double> ("tranmat");
        for (double value : projection.u.axono.tranmat.tmx) {
            tranmat (value);
        }
        os.Add ("axono", axonoOs);
    }

    return os;
}

static GS::ObjectState CreateSettingsObjectState (const API_DocumentFrom3DType& settings)
{
    GS::ObjectState os;

#ifdef ServerMainVers_2600
    const API_3DFilterAndCutSettings& filter = settings.filterAndCutSettings;
    GS::ObjectState filterOs;
    AddFields (filterOs, filter, FilterAndCutBoolFields);
    AddFields (filterOs, filter, FilterAndCutShortFields);
    filterOs.Add ("filterMode", EnumToString (filter.filterMode, FilterModeNames));
    const auto& elemTypeFilter = filterOs.AddList<GS::ObjectState> ("elemTypeFilter");
    for (const auto& entry : filter.elemTypeFilter) {
        const GS::UniString typeName = GetElementTypeNonLocalizedName (entry.first);
        if (typeName != "Unknown") {
            elemTypeFilter (GS::ObjectState ("elementType", typeName, "show", entry.second));
        }
    }
    os.Add ("filterAndCutSettings", filterOs);
#else
    const API_3DImageInfo& filter = settings.window3DFilter;
    GS::ObjectState filterOs;
    AddFields (filterOs, filter, FilterAndCutBoolFields);
    AddFields (filterOs, filter, FilterAndCutShortFields);
    filterOs.Add ("filterMode", EnumToString (filter.filterMode, FilterModeNames));
    os.Add ("window3DFilter", filterOs);
#endif

    GS::ObjectState cutSettingOs;
    AddFields (cutSettingOs, settings.cutSetting, CutSettingBoolFields);
    AddFields (cutSettingOs, settings.cutSetting, CutSettingShortFields);
    AddAttribute (cutSettingOs, "customMater", API_MaterialID, settings.cutSetting.customMater);
    os.Add ("cutSetting", cutSettingOs);

    AddFields (os, settings, DocumentBoolFields);
    AddFields (os, settings, DocumentShortFields);

    os.Add ("projectionSetting", CreateProjectionObjectState (settings.projectionSetting));
    os.Add ("floorPlanRotationOrigo", Create2DCoordinateObjectState (settings.floorPlanRotationOrigo));

    GS::ObjectState windowInfoOs;
    AddFields (windowInfoOs, settings.window3DInfo, WindowInfoBoolFields);
    AddFields (windowInfoOs, settings.window3DInfo, WindowInfoShortFields);
    AddFields (windowInfoOs, settings.window3DInfo, WindowInfoDoubleFields);
    os.Add ("window3DInfo", windowInfoOs);

    AddAttribute (os, "hiddenLineType", API_LinetypeID, settings.hiddenLineType);

    // The coordinates of API_Rect are passed as numbers, whatever their C++ type.
    os.Add ("drawRect", GS::ObjectState (
        "left", static_cast<double> (settings.drawRect.left),
        "top", static_cast<double> (settings.drawRect.top),
        "right", static_cast<double> (settings.drawRect.right),
        "bottom", static_cast<double> (settings.drawRect.bottom)));

    const API_UniformAttributeOptions& options = settings.attributeOptions;
    GS::ObjectState optionsOs;
    AddFields (optionsOs, options, AttributeOptionsBoolFields);
    AddFields (optionsOs, options, AttributeOptionsShortFields);
    optionsOs.Add ("fillCutSurfacesWith", static_cast<Int32> (options.fillCutSurfacesWith));
    optionsOs.Add ("uncutSurfaceFillType", static_cast<Int32> (options.uncutSurfaceFillType));
    AddAttribute (optionsOs, "shadFill", API_FilltypeID, options.shadFill);
    optionsOs.Add ("hiddenEdgeVisibility", static_cast<Int32> (options.hiddenEdgeVisibility));
    AddAttribute (optionsOs, "hiddenEdgeLineType", API_LinetypeID, options.hiddenEdgeLineType);
    optionsOs.Add ("onCutPlaneVisibility", static_cast<Int32> (options.onCutPlaneVisibility));
    os.Add ("attributeOptions", optionsOs);

    os.Add ("currPlanConn", EnumToString (settings.currPlanConn, CutPlanePlanConnNames));
    os.Add ("intPlanConn", EnumToString (settings.intPlanConn, CutPlanePlanConnNames));
    os.Add ("docSrc", EnumToString (settings.docSrc, DocumentSourceNames));

    GS::ObjectState skeletonOs ("viewSrc", static_cast<Int32> (settings.skeletonOptions.viewSrc));
    AddFields (skeletonOs, settings.skeletonOptions, SkeletonOptionsShortFields);
    AddFields (skeletonOs, settings.skeletonOptions, SkeletonOptionsDoubleFields);
    os.Add ("skeletonOptions", skeletonOs);

    os.Add ("boundaryDisplay", EnumToString (settings.boundaryDisplay, BoundaryDisplayNames));
    AddAttribute (os, "boundaryLineType", API_LinetypeID, settings.boundaryLineType);

    return os;
}

// ---------------------------------------------------------------------------------------------
// Writing the given JSON keys into the settings
// ---------------------------------------------------------------------------------------------

static bool IsValidAttributeIndex (const API_AttributeIndex& index)
{
#ifdef ServerMainVers_2700
    return index.IsPositive ();
#else
    return index > 0;
#endif
}

// Changes only the fields whose key is given; stops at the first invalid value.
class SettingsReader
{
public:
    bool Failed () const { return !error.IsEmpty (); }
    const GS::UniString& GetError () const { return error; }

    bool ReadObject (const GS::ObjectState& os, const char* name, GS::ObjectState& object)
    {
        if (Failed () || !os.Contains (name)) {
            return false;
        }
        if (!os.Get (name, object)) {
            Fail (name, "an object");
            return false;
        }
        return true;
    }

    void Read (const GS::ObjectState& os, const char* name, bool& target)
    {
        bool value = false;
        if (Failed () || !os.Contains (name)) {
            return;
        }
        if (!os.Get (name, value)) {
            Fail (name, "a boolean");
            return;
        }
        target = value;
    }

    void Read (const GS::ObjectState& os, const char* name, short& target)
    {
        Int32 value = 0;
        if (Failed () || !os.Contains (name)) {
            return;
        }
        if (!os.Get (name, value) || value < -32768 || value > 32767) {
            Fail (name, "an integer from -32768 to 32767");
            return;
        }
        target = static_cast<short> (value);
    }

    void Read (const GS::ObjectState& os, const char* name, double& target)
    {
        double value = 0.0;
        if (Failed () || !os.Contains (name)) {
            return;
        }
        if (!os.Get (name, value)) {
            Fail (name, "a number");
            return;
        }
        target = value;
    }

    template <typename NumberType>
    void ReadNumber (const GS::ObjectState& os, const char* name, NumberType& target)
    {
        double value = static_cast<double> (target);
        Read (os, name, value);
        target = static_cast<NumberType> (value);
    }

    void Read (const GS::ObjectState& os, const char* name, API_Coord& target)
    {
        GS::ObjectState coordinate;
        if (!ReadObject (os, name, coordinate)) {
            return;
        }
        double x = 0.0;
        double y = 0.0;
        if (!coordinate.Get ("x", x) || !coordinate.Get ("y", y)) {
            Fail (name, "a coordinate with x and y");
            return;
        }
        target = API_Coord { x, y };
    }

    void ReadElementGuid (const GS::ObjectState& os, const char* name, API_Guid& target)
    {
        GS::ObjectState elementId;
        if (!ReadObject (os, name, elementId)) {
            return;
        }
        const API_Guid guid = GetGuidFromElementsArrayItem (elementId);
        if (guid == APINULLGuid) {
            Fail (name, "an elementId");
            return;
        }
        target = guid;
    }

    void ReadAttribute (const GS::ObjectState& os, const char* name, API_AttrTypeID typeID, API_AttributeIndex& target)
    {
        GS::ObjectState attributeId;
        if (!ReadObject (os, name, attributeId)) {
            return;
        }
        const API_Guid guid = GetGuidFromAttributesArrayItem (attributeId);
        const API_AttributeIndex index = guid != APINULLGuid ? GetAttributeIndexFromGuid (typeID, guid) : ACAPI_CreateAttributeIndex (-1);
        if (!IsValidAttributeIndex (index)) {
            Fail (name, "the attributeId of an existing attribute of the right type");
            return;
        }
        target = index;
    }

    template <typename EnumType, size_t N>
    void ReadEnum (const GS::ObjectState& os, const char* name, EnumType& target, const EnumName<EnumType> (&names)[N])
    {
        GS::UniString value;
        if (Failed () || !os.Contains (name)) {
            return;
        }
        if (os.Get (name, value)) {
            for (const EnumName<EnumType>& item : names) {
                if (value == item.name) {
                    target = item.value;
                    return;
                }
            }
        }
        GS::UniString expected = "one of";
        for (const EnumName<EnumType>& item : names) {
            expected += GS::UniString (" ") + item.name;
        }
        Fail (name, expected);
    }

    template <typename EnumType>
    void ReadRawEnum (const GS::ObjectState& os, const char* name, EnumType& target)
    {
        Int32 value = 0;
        if (Failed () || !os.Contains (name)) {
            return;
        }
        if (!os.Get (name, value) || value < 0) {
            Fail (name, "a non-negative integer");
            return;
        }
        target = static_cast<EnumType> (value);
    }

    template <typename Struct, size_t N>
    void ReadFields (const GS::ObjectState& os, Struct& data, const BoolField<Struct> (&fields)[N])
    {
        for (const BoolField<Struct>& field : fields) {
            Read (os, field.name, data.*(field.field));
        }
    }

    template <typename Struct, size_t N>
    void ReadFields (const GS::ObjectState& os, Struct& data, const ShortField<Struct> (&fields)[N])
    {
        for (const ShortField<Struct>& field : fields) {
            Read (os, field.name, data.*(field.field));
        }
    }

    template <typename Struct, size_t N>
    void ReadFields (const GS::ObjectState& os, Struct& data, const DoubleField<Struct> (&fields)[N])
    {
        for (const DoubleField<Struct>& field : fields) {
            Read (os, field.name, data.*(field.field));
        }
    }

    void Fail (const char* name, const GS::UniString& expected)
    {
        if (!Failed ()) {
            error = GS::UniString ("Invalid parameter: ") + name + " must be " + expected + ".";
        }
    }

private:
    GS::UniString error;
};

static void ReadProjection (SettingsReader& reader, const GS::ObjectState& os, API_3DProjectionInfo& projection)
{
    reader.ReadElementGuid (os, "camGuid", projection.camGuid);
    reader.ReadElementGuid (os, "actCamSet", projection.actCamSet);

    bool isPersp = projection.isPersp;
    reader.Read (os, "isPersp", isPersp);
    if (reader.Failed ()) {
        return;
    }
    if (isPersp != projection.isPersp) {
        // The perspective and the axonometric parameters share a union: switching starts the new
        // one from zero, keeping only the sun position, as Set3DProjectionSettings does.
        const API_SunAngleSettings sunAngSets = projection.isPersp ? projection.u.persp.sunAngSets : projection.u.axono.sunAngSets;
        projection.isPersp = isPersp;
        if (isPersp) {
            projection.u.persp = API_PerspPars {};
            projection.u.persp.sunAngSets = sunAngSets;
        } else {
            projection.u.axono = API_AxonoPars {};
            projection.u.axono.sunAngSets = sunAngSets;
        }
    }

    GS::ObjectState persp;
    if (reader.ReadObject (os, "persp", persp)) {
        if (!projection.isPersp) {
            reader.Fail ("persp", "left out when isPersp is false (give axono instead)");
            return;
        }
        reader.Read (persp, "pos", projection.u.persp.pos);
        reader.Read (persp, "target", projection.u.persp.target);
        reader.ReadFields (persp, projection.u.persp, PerspBoolFields);
        reader.ReadFields (persp, projection.u.persp, PerspDoubleFields);
    }

    GS::ObjectState axono;
    if (reader.ReadObject (os, "axono", axono)) {
        if (projection.isPersp) {
            reader.Fail ("axono", "left out when isPersp is true (give persp instead)");
            return;
        }
        if (axono.Contains ("tranmat")) {
            GS::Array<double> tranmat;
            const USize tranmatSize = sizeof (API_Tranmat::tmx) / sizeof (double);
            if (!axono.Get ("tranmat", tranmat) || tranmat.GetSize () != tranmatSize) {
                reader.Fail ("tranmat", "an array of 12 numbers");
                return;
            }
            for (USize i = 0; i < tranmatSize; ++i) {
                projection.u.axono.tranmat.tmx[i] = tranmat[i];
            }
        }
    }
}

static void ReadSettings (SettingsReader& reader, const GS::ObjectState& os, API_DocumentFrom3DType& settings)
{
    GS::ObjectState filterOs;
#ifdef ServerMainVers_2600
    if (reader.ReadObject (os, "filterAndCutSettings", filterOs)) {
        API_3DFilterAndCutSettings& filter = settings.filterAndCutSettings;
        reader.ReadFields (filterOs, filter, FilterAndCutBoolFields);
        reader.ReadFields (filterOs, filter, FilterAndCutShortFields);
        reader.ReadEnum (filterOs, "filterMode", filter.filterMode, FilterModeNames);
        // The listed element types are changed, the others keep their current entry.
        if (!reader.Failed () && filterOs.Contains ("elemTypeFilter")) {
            GS::Array<GS::ObjectState> entries;
            if (!filterOs.Get ("elemTypeFilter", entries)) {
                reader.Fail ("elemTypeFilter", "an array");
            }
            for (const GS::ObjectState& entry : entries) {
                GS::UniString typeName;
                bool show = false;
                const API_ElemTypeID typeID = entry.Get ("elementType", typeName) ? GetElementTypeFromNonLocalizedName (typeName) : API_ZombieElemID;
                if (typeID == API_ZombieElemID || !entry.Get ("show", show)) {
                    reader.Fail ("elemTypeFilter", "an array of {elementType, show} with known element types");
                    break;
                }
                filter.elemTypeFilter[typeID] = show;
            }
        }
    }
#else
    if (reader.ReadObject (os, "window3DFilter", filterOs)) {
        API_3DImageInfo& filter = settings.window3DFilter;
        reader.ReadFields (filterOs, filter, FilterAndCutBoolFields);
        reader.ReadFields (filterOs, filter, FilterAndCutShortFields);
        reader.ReadEnum (filterOs, "filterMode", filter.filterMode, FilterModeNames);
    }
#endif

    GS::ObjectState cutSettingOs;
    if (reader.ReadObject (os, "cutSetting", cutSettingOs)) {
        reader.ReadFields (cutSettingOs, settings.cutSetting, CutSettingBoolFields);
        reader.ReadFields (cutSettingOs, settings.cutSetting, CutSettingShortFields);
        reader.ReadAttribute (cutSettingOs, "customMater", API_MaterialID, settings.cutSetting.customMater);
    }

    reader.ReadFields (os, settings, DocumentBoolFields);
    reader.ReadFields (os, settings, DocumentShortFields);

    GS::ObjectState projectionOs;
    if (reader.ReadObject (os, "projectionSetting", projectionOs)) {
        ReadProjection (reader, projectionOs, settings.projectionSetting);
    }

    reader.Read (os, "floorPlanRotationOrigo", settings.floorPlanRotationOrigo);

    GS::ObjectState windowInfoOs;
    if (reader.ReadObject (os, "window3DInfo", windowInfoOs)) {
        reader.ReadFields (windowInfoOs, settings.window3DInfo, WindowInfoBoolFields);
        reader.ReadFields (windowInfoOs, settings.window3DInfo, WindowInfoShortFields);
        reader.ReadFields (windowInfoOs, settings.window3DInfo, WindowInfoDoubleFields);
    }

    reader.ReadAttribute (os, "hiddenLineType", API_LinetypeID, settings.hiddenLineType);

    GS::ObjectState drawRectOs;
    if (reader.ReadObject (os, "drawRect", drawRectOs)) {
        reader.ReadNumber (drawRectOs, "left", settings.drawRect.left);
        reader.ReadNumber (drawRectOs, "top", settings.drawRect.top);
        reader.ReadNumber (drawRectOs, "right", settings.drawRect.right);
        reader.ReadNumber (drawRectOs, "bottom", settings.drawRect.bottom);
    }

    GS::ObjectState optionsOs;
    if (reader.ReadObject (os, "attributeOptions", optionsOs)) {
        API_UniformAttributeOptions& options = settings.attributeOptions;
        reader.ReadFields (optionsOs, options, AttributeOptionsBoolFields);
        reader.ReadFields (optionsOs, options, AttributeOptionsShortFields);
        reader.ReadRawEnum (optionsOs, "fillCutSurfacesWith", options.fillCutSurfacesWith);
        reader.ReadRawEnum (optionsOs, "uncutSurfaceFillType", options.uncutSurfaceFillType);
        reader.ReadAttribute (optionsOs, "shadFill", API_FilltypeID, options.shadFill);
        reader.ReadRawEnum (optionsOs, "hiddenEdgeVisibility", options.hiddenEdgeVisibility);
        reader.ReadAttribute (optionsOs, "hiddenEdgeLineType", API_LinetypeID, options.hiddenEdgeLineType);
        reader.ReadRawEnum (optionsOs, "onCutPlaneVisibility", options.onCutPlaneVisibility);
    }

    reader.ReadEnum (os, "currPlanConn", settings.currPlanConn, CutPlanePlanConnNames);
    reader.ReadEnum (os, "intPlanConn", settings.intPlanConn, CutPlanePlanConnNames);
    reader.ReadEnum (os, "docSrc", settings.docSrc, DocumentSourceNames);

    GS::ObjectState skeletonOs;
    if (reader.ReadObject (os, "skeletonOptions", skeletonOs)) {
        reader.ReadRawEnum (skeletonOs, "viewSrc", settings.skeletonOptions.viewSrc);
        reader.ReadFields (skeletonOs, settings.skeletonOptions, SkeletonOptionsShortFields);
        reader.ReadFields (skeletonOs, settings.skeletonOptions, SkeletonOptionsDoubleFields);
    }

    reader.ReadEnum (os, "boundaryDisplay", settings.boundaryDisplay, BoundaryDisplayNames);
    reader.ReadAttribute (os, "boundaryLineType", API_LinetypeID, settings.boundaryLineType);
}

// ---------------------------------------------------------------------------------------------
// Shared parts of the commands
// ---------------------------------------------------------------------------------------------

// Owns the settings read from Archicad: GetDocumentFrom3DSettings allocates the cut shapes handle
// of cutSetting, which is passed back unchanged on a Set and disposed of here.
class DocumentFrom3DSettings
{
public:
    DocumentFrom3DSettings () = default;
    DocumentFrom3DSettings (const DocumentFrom3DSettings&) = delete;
    DocumentFrom3DSettings& operator= (const DocumentFrom3DSettings&) = delete;
    ~DocumentFrom3DSettings ()
    {
        if (settings.cutSetting.shapes != nullptr) {
            BMKillHandle ((GSHandle*) &(settings.cutSetting.shapes));
        }
    }

    API_DocumentFrom3DType settings {};
};

static GS::Optional<GS::ObjectState> GetDatabaseOfNavigatorItem (const GS::ObjectState& parameters, API_DatabaseUnId& databaseUnId)
{
    GS::ObjectState navigatorItemId;
    API_Guid navGuid = APINULLGuid;
    if (parameters.Get ("navigatorItemId", navigatorItemId)) {
        navGuid = GetGuidFromObjectState (navigatorItemId);
    }
    if (navGuid == APINULLGuid) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: navigatorItemId.");
    }

    API_NavigatorItem navigatorItem = {};
    GSErrCode err = ACAPI_Navigator_GetNavigatorItem (&navGuid, &navigatorItem);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the navigator item.");
    }
    if (navigatorItem.itemType != API_DocumentFrom3DNavItem) {
        return CreateErrorResponse (APIERR_BADPARS, "The navigator item is not a 3D Document.");
    }

    databaseUnId = navigatorItem.db.databaseUnId;
    return GS::NoValue;
}

static const char* NavigatorItemIdSchema = R"(
        "navigatorItemId": {
            "$ref": "#/NavigatorItemId",
            "description": "The navigator item of the 3D Document, from the Project Map or the View Map."
        })";

#ifdef ServerMainVers_2600
static const char* FilterSchema = R"(
        "filterAndCutSettings": {
            "type": "object",
            "description": "Which elements and stories the 3D Document shows (API_3DFilterAndCutSettings). The marquee filter is not included.",
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
        },)";
#else
static const char* FilterSchema = R"(
        "window3DFilter": {
            "type": "object",
            "description": "Which elements and stories the 3D Document shows (API_3DImageInfo).",
            "properties": {
                "firstStory3D": { "type": "integer", "description": "The first story shown." },
                "lastStory3D": { "type": "integer", "description": "The last story shown." },
                "allStories": { "type": "boolean", "description": "Whether all stories are shown; if false, firstStory3D to lastStory3D." },
                "inSelect": { "type": "boolean", "description": "True: what is inside the marquee is shown, false: what is outside." },
                "trimToMark": { "type": "boolean", "description": "Whether the elements are trimmed to the marquee." },
                "wall": { "type": "boolean" },
                "colu": { "type": "boolean" },
                "symb": { "type": "boolean" },
                "light": { "type": "boolean" },
                "ceil": { "type": "boolean" },
                "mesh": { "type": "boolean" },
                "roof": { "type": "boolean" },
                "shell": { "type": "boolean" },
                "curtainWall": { "type": "boolean" },
                "wind": { "type": "boolean" },
                "door": { "type": "boolean" },
                "skylight": { "type": "boolean" },
                "morph": { "type": "boolean" },
                "beam": { "type": "boolean" },
                "room": { "type": "boolean" },
                "trimToStoryRange": { "type": "boolean" },
                "markRectFloor": { "type": "integer", "description": "The story of a single-story marquee." },
                "markVertType": { "type": "integer", "description": "Whether the marquee is single-story or all-story." },
                "filterMode": { "type": "string", "enum": ["FilterByRules", "FilterBySelection", "FilterByMarqueeAndRules"] }
            },
            "additionalProperties": false
        },)";
#endif

static const char* SettingsSchema = R"(
        "cutSetting": {
            "type": "object",
            "description": "The 3D cut of the 3D Document (API_3DCutPlanesInfo). The cut planes themselves are not included.",
            "properties": {
                "isCutPlanes": { "type": "boolean" },
                "useCustom": { "type": "boolean" },
                "customPen": { "type": "integer" },
                "customMater": { "$ref": "#/AttributeIdArrayItem", "description": "The surface of the cut. Missing when there is none." }
            },
            "additionalProperties": false
        },
        "cutaway3D": { "type": "boolean" },
        "projectionSetting": {
            "type": "object",
            "description": "The projection of the 3D Document (API_3DProjectionInfo). persp is present when isPersp is true, axono otherwise. Changing isPersp starts the new projection from zero, give persp or axono with it.",
            "properties": {
                "isPersp": { "type": "boolean" },
                "camGuid": { "$ref": "#/ElementIdArrayItem", "description": "The Camera the projection comes from. Missing when there is none." },
                "actCamSet": { "$ref": "#/ElementIdArrayItem", "description": "The active Camera Set. Missing when there is none." },
                "persp": {
                    "type": "object",
                    "properties": {
                        "pos": { "$ref": "#/Coordinate2D", "description": "The position of the camera." },
                        "target": { "$ref": "#/Coordinate2D", "description": "The position of the target." },
                        "isTwoPointPersp": { "type": "boolean" },
                        "cameraZ": { "type": "number" },
                        "targetZ": { "type": "number" },
                        "distance": { "type": "number" },
                        "azimuth": { "type": "number" },
                        "viewCone": { "type": "number" },
                        "rollAngle": { "type": "number" }
                    },
                    "additionalProperties": false
                },
                "axono": {
                    "type": "object",
                    "properties": {
                        "tranmat": {
                            "type": "array",
                            "description": "The 3x4 transformation matrix, row by row.",
                            "items": { "type": "number" },
                            "minItems": 12,
                            "maxItems": 12
                        }
                    },
                    "additionalProperties": false
                }
            },
            "additionalProperties": false
        },
        "floorPlanRotationOrigo": { "$ref": "#/Coordinate2D" },
        "window3DInfo": {
            "type": "object",
            "description": "API_3DWindowInfo of the 3D Document.",
            "properties": {
                "setWindowSize": { "type": "boolean" },
                "hSize": { "type": "integer" },
                "vSize": { "type": "integer" },
                "setZoom": { "type": "boolean" },
                "zoomScaleX": { "type": "number" },
                "zoomScaleY": { "type": "number" },
                "zoomDispX": { "type": "number" },
                "zoomDispY": { "type": "number" }
            },
            "additionalProperties": false
        },
        "hiddenLineType": { "$ref": "#/AttributeIdArrayItem", "description": "A line type. Missing when there is none." },
        "hiddenLineLen": { "type": "integer" },
        "effects3D": { "type": "integer" },
        "drawRect": {
            "type": "object",
            "properties": {
                "left": { "type": "number" },
                "top": { "type": "number" },
                "right": { "type": "number" },
                "bottom": { "type": "number" }
            },
            "additionalProperties": false
        },
        "attributeOptions": {
            "type": "object",
            "description": "The uniform attributes of cut and uncut surfaces (API_UniformAttributeOptions). Pens and uniformLineType are indices; the enums fillCutSurfacesWith, uncutSurfaceFillType, hiddenEdgeVisibility and onCutPlaneVisibility are their integer values.",
            "properties": {
                "shouldUseUniformCutAttributes": { "type": "boolean" },
                "uniformLineType": { "type": "integer" },
                "uniformFillPen": { "type": "integer" },
                "uniformFillBkgPen": { "type": "integer" },
                "shouldUseUncutSurfaceFill": { "type": "boolean" },
                "fillCutSurfacesWith": { "type": "integer", "minimum": 0 },
                "uncutSurfaceFillType": { "type": "integer", "minimum": 0 },
                "uncutSurfaceBkgPen": { "type": "integer" },
                "shouldUseElemPen": { "type": "boolean" },
                "elemPen": { "type": "integer" },
                "shadFillPen": { "type": "integer" },
                "shadFill": { "$ref": "#/AttributeIdArrayItem", "description": "A fill. Missing when there is none." },
                "shadFillBkgPen": { "type": "integer" },
                "shadContPen": { "type": "integer" },
                "hiddenEdgeVisibility": { "type": "integer", "minimum": 0 },
                "hiddenEdgeLineType": { "$ref": "#/AttributeIdArrayItem", "description": "A line type. Missing when there is none." },
                "hiddenEdgeLinePen": { "type": "integer" },
                "onCutPlaneVisibility": { "type": "integer", "minimum": 0 }
            },
            "additionalProperties": false
        },
        "currPlanConn": { "type": "string", "enum": ["Online", "Offline", "Drawing"] },
        "intPlanConn": { "type": "string", "enum": ["Online", "Offline", "Drawing"] },
        "materialFrom3D": { "type": "boolean" },
        "docSrc": { "type": "string", "enum": ["DDD", "FloorPlan"], "description": "Whether the 3D Document comes from the 3D window or from the floor plan." },
        "skeletonOptions": {
            "type": "object",
            "description": "API_SkeletonOptions of a 3D Document made from the floor plan. viewSrc is the integer value of API_ViewSourceID.",
            "properties": {
                "viewSrc": { "type": "integer", "minimum": 0 },
                "floorIdx": { "type": "integer" },
                "cutHeight": { "type": "number" },
                "floorOffset": { "type": "integer" },
                "cutOffset": { "type": "number" }
            },
            "additionalProperties": false
        },
        "boundaryDisplay": { "type": "string", "enum": ["UncutContours", "NoContours", "OverrideContours"] },
        "boundaryPen": { "type": "integer" },
        "boundaryLineType": { "$ref": "#/AttributeIdArrayItem", "description": "A line type. Missing when there is none." },
        "transparency": { "type": "boolean" }
    },
    "additionalProperties": false
})";

static GS::UniString GetSettingsSchema (const GS::UniString& description)
{
    return GS::UniString (R"({
    "type": "object",
    "description": ")") + description + R"(",
    "properties": {)" + FilterSchema + SettingsSchema;
}

// ---------------------------------------------------------------------------------------------
// Get3DDocumentSettings
// ---------------------------------------------------------------------------------------------

Get3DDocumentSettingsCommand::Get3DDocumentSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Get3DDocumentSettingsCommand::GetName () const
{
    return "Get3DDocumentSettings";
}

GS::Optional<GS::UniString> Get3DDocumentSettingsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {)") + NavigatorItemIdSchema + R"(
    },
    "additionalProperties": false,
    "required": [
        "navigatorItemId"
    ]
})";
}

GS::Optional<GS::UniString> Get3DDocumentSettingsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "settings": )") + GetSettingsSchema ("The settings of the 3D Document, with the field names of API_DocumentFrom3DType.") + R"(
    },
    "additionalProperties": false,
    "required": [
        "settings"
    ]
})";
}

GS::ObjectState Get3DDocumentSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_DatabaseUnId databaseUnId {};
    const GS::Optional<GS::ObjectState> databaseError = GetDatabaseOfNavigatorItem (parameters, databaseUnId);
    if (databaseError.HasValue ()) {
        return *databaseError;
    }

    DocumentFrom3DSettings document;
    GSErrCode err = ACAPI_View_GetDocumentFrom3DSettings (&databaseUnId, &document.settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the settings of the 3D Document.");
    }

    return GS::ObjectState ("settings", CreateSettingsObjectState (document.settings));
}

// ---------------------------------------------------------------------------------------------
// Set3DDocumentSettings
// ---------------------------------------------------------------------------------------------

Set3DDocumentSettingsCommand::Set3DDocumentSettingsCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String Set3DDocumentSettingsCommand::GetName () const
{
    return "Set3DDocumentSettings";
}

GS::Optional<GS::UniString> Set3DDocumentSettingsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {)") + NavigatorItemIdSchema + R"(,
        "settings": )" + GetSettingsSchema ("The settings to change, in the form Get3DDocumentSettings returns. Only the given keys are changed; arrays other than elemTypeFilter are replaced.") + R"(
    },
    "additionalProperties": false,
    "required": [
        "navigatorItemId",
        "settings"
    ]
})";
}

GS::Optional<GS::UniString> Set3DDocumentSettingsCommand::GetRawResponseSchema () const
{
    return Get3DDocumentSettingsCommand ().GetRawResponseSchema ();
}

GS::ObjectState Set3DDocumentSettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::ObjectState settingsOs;
    if (!parameters.Get ("settings", settingsOs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid parameter: settings.");
    }

    API_DatabaseUnId databaseUnId {};
    const GS::Optional<GS::ObjectState> databaseError = GetDatabaseOfNavigatorItem (parameters, databaseUnId);
    if (databaseError.HasValue ()) {
        return *databaseError;
    }

    // Start from the current settings so every value that is not given is kept.
    DocumentFrom3DSettings document;
    GSErrCode err = ACAPI_View_GetDocumentFrom3DSettings (&databaseUnId, &document.settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current settings of the 3D Document.");
    }

    SettingsReader reader;
    ReadSettings (reader, settingsOs, document.settings);
    if (reader.Failed ()) {
        return CreateErrorResponse (APIERR_BADPARS, reader.GetError ());
    }

    // Called outside ACAPI_CallUndoableCommand, as issue #1048 asks.
    err = ACAPI_View_ChangeDocumentFrom3DSettings (&databaseUnId, &document.settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to change the settings of the 3D Document.");
    }

    DocumentFrom3DSettings newDocument;
    err = ACAPI_View_GetDocumentFrom3DSettings (&databaseUnId, &newDocument.settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "The settings of the 3D Document were changed, but reading them back failed.");
    }

    return GS::ObjectState ("settings", CreateSettingsObjectState (newDocument.settings));
}
