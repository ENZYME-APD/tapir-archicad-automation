#include "AnalyticalCommands.hpp"
#include "MigrationHelper.hpp"

// The Structural Analytical Model functions are called with their ACAPI_Analytical_* names and the
// add-on's own token, which is how the Archicad 27 and newer DevKits declare them. Archicad 26 declares
// the same functions in ACAPI_Analytical.hpp as ACAPI_AnalyticalModel_*, and Archicad 25 in ACAPinc.h,
// without the token and with pointer outputs. The commands that only call functions of Archicad 27 and
// newer report "not supported" before; the others call the older names through the TAPIR_ wrappers below.
#ifdef ServerMainVers_2600
#include "ACAPI_Analytical.hpp"
#endif

namespace {

template <typename EnumType>
struct EnumName
{
    EnumType    value;
    const char* name;
};

template <typename EnumType, size_t N>
const char* EnumToString (const EnumName<EnumType> (&names)[N], EnumType value)
{
    for (const EnumName<EnumType>& item : names) {
        if (item.value == value) {
            return item.name;
        }
    }
    return nullptr;
}

template <typename EnumType, size_t N>
bool EnumFromString (const EnumName<EnumType> (&names)[N], const GS::UniString& name, EnumType& value)
{
    for (const EnumName<EnumType>& item : names) {
        if (name == GS::UniString (item.name)) {
            value = item.value;
            return true;
        }
    }
    return false;
}

#ifdef ServerMainVers_2600
const EnumName<API_AnalyticalMemberType> AnalyticalMemberTypeNames[] = {
    { API_AnalyticalMemberType::Unknown, "Unknown" },
    { API_AnalyticalMemberType::Beam,    "Beam" },
    { API_AnalyticalMemberType::Column,  "Column" },
    { API_AnalyticalMemberType::Roof,    "Roof" },
    { API_AnalyticalMemberType::Slab,    "Slab" },
    { API_AnalyticalMemberType::Wall,    "Wall" }
};

const EnumName<API_AM_VisibilityMode> AnalyticalVisibilityModeNames[] = {
    { API_AM_VisibilityMode::AnalyticalModelOnly,              "AnalyticalModelOnly" },
    { API_AM_VisibilityMode::AnalyticalModelWithinEntireModel, "AnalyticalModelWithinEntireModel" }
};

const EnumName<API_LoadVisibilityMode> AnalyticalLoadVisibilityModeNames[] = {
    { API_LoadVisibilityMode::API_LoadVisibilityMode_FilterByActive, "FilterByActive" },
    { API_LoadVisibilityMode::API_LoadVisibilityMode_HideAll,        "HideAll" },
    { API_LoadVisibilityMode::API_LoadVisibilityMode_ShowAll,        "ShowAll" }
};

struct VisibilityFlag
{
    bool API_AM_VisibilitySettings::* field;
    const char* name;
};

const VisibilityFlag AnalyticalVisibilityFlags[] = {
    { &API_AM_VisibilitySettings::isAnalyticalModelVisible,       "isAnalyticalModelVisible" },
    { &API_AM_VisibilitySettings::areMembersTransparent,          "areMembersTransparent" },
    { &API_AM_VisibilitySettings::areSymbolsTransparent,          "areSymbolsTransparent" },
    { &API_AM_VisibilitySettings::areNodesVisible,                "areNodesVisible" },
    { &API_AM_VisibilitySettings::areReleasesVisible,             "areReleasesVisible" },
    { &API_AM_VisibilitySettings::areCrossSectionsVisible,        "areCrossSectionsVisible" },
    { &API_AM_VisibilitySettings::areCoordinationProfilesVisible, "areCoordinationProfilesVisible" },
    { &API_AM_VisibilitySettings::areLocalCoordSystemsVisible,    "areLocalCoordSystemsVisible" }
};
#else
// Archicad 25 names the member types API_AnalyticalMember_<Name>Type; the JSON uses the names of Archicad 26 and newer.
const EnumName<API_AnalyticalMemberType> AnalyticalMemberTypeNames[] = {
    { API_AnalyticalMember_UnknownType, "Unknown" },
    { API_AnalyticalMember_BeamType,    "Beam" },
    { API_AnalyticalMember_ColumnType,  "Column" },
    { API_AnalyticalMember_RoofType,    "Roof" },
    { API_AnalyticalMember_SlabType,    "Slab" },
    { API_AnalyticalMember_WallType,    "Wall" }
};
#endif

GSErrCode TAPIR_GetAnalyticalModelVariation (const API_Guid& amGuid, API_AnalyticalModelVariation& variation)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalModelVariation (ACAPI_GetToken (), amGuid, variation);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalModelVariation (ACAPI_GetToken (), amGuid, variation);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalModelVariation (amGuid, &variation);
#endif
}

GSErrCode TAPIR_GetAnalyticalModel (const API_AnalyticalModelVariation& variation, API_Guid& amGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalModel (ACAPI_GetToken (), variation, amGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalModel (ACAPI_GetToken (), variation, amGuid);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalModel (variation, &amGuid);
#endif
}

GSErrCode TAPIR_GetComplementaryProjectMemberType (const API_Guid& elemGuid, API_AnalyticalMemberType& memberType)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetComplementaryProjectMemberType (ACAPI_GetToken (), elemGuid, memberType);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetComplementaryProjectMemberType (ACAPI_GetToken (), elemGuid, memberType);
#else
    return ACAPI_AnalyticalModel_GetComplementaryProjectMemberType (elemGuid, &memberType);
#endif
}

#ifdef ServerMainVers_2600
GSErrCode TAPIR_GetAnalyticalVisibilitySettings (API_AM_VisibilitySettings& settings)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetVisibilitySettings (ACAPI_GetToken (), settings);
#else
    return ACAPI_AnalyticalModel_GetVisibilitySettings (ACAPI_GetToken (), settings);
#endif
}

GSErrCode TAPIR_SetAnalyticalVisibilitySettings (const API_AM_VisibilitySettings& settings)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_SetVisibilitySettings (ACAPI_GetToken (), settings);
#else
    return ACAPI_AnalyticalModel_SetVisibilitySettings (ACAPI_GetToken (), settings);
#endif
}
#endif

// Reads the optional analyticalModelId parameter. Without it the model of the current view is used,
// which the API can only tell from Archicad 27 on. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalModelGuidFromParameters (const GS::ObjectState& parameters, API_Guid& amGuid)
{
    const GS::ObjectState* analyticalModelId = parameters.Get ("analyticalModelId");
    if (analyticalModelId != nullptr) {
        amGuid = GetGuidFromObjectState (*analyticalModelId);
        if (amGuid == APINULLGuid) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid analyticalModelId.");
        }
        return GS::NoValue;
    }

#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), amGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the current view.");
    }
    return GS::NoValue;
#else
    return CreateErrorResponse (APIERR_BADPARS, "analyticalModelId is required before Archicad 27.");
#endif
}

GS::ObjectState CreateAnalyticalModelVariationObjectState (const API_AnalyticalModelVariation& variation)
{
    GS::ObjectState variationOS;
#ifdef ServerMainVers_2700
    variationOS.Add ("amVariationGuid", CreateGuidObjectState (variation.amVariationGuid));
#else
    variationOS.Add ("renovationFilterGuid", CreateGuidObjectState (variation.renovationFilterGuid));
#ifdef ServerMainVers_2600
    const auto& classIds = variationOS.AddList<GS::ObjectState> ("layerConnectionClassIds");
    for (const auto& kv : variation.layerConnectionClassIds) {
        // Pre-2800 GS::HashTable's const range-for pair exposes pointer members.
        GS::ObjectState classId;
        classId.Add ("layerIndex", GetAttributeIndex (*kv.key));
        classId.Add ("connectionClassId", *kv.value);
        classIds (classId);
    }
#else
    const auto& classIds = variationOS.AddList<Int32> ("layerConnectionClassIds");
    for (Int32 classId : variation.layerConnectionClassIds) {
        classIds (classId);
    }
#endif
#endif
    return variationOS;
}

// Fills the variation from its JSON form. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalModelVariationFromObjectState (const GS::ObjectState& variationOS, API_AnalyticalModelVariation& variation)
{
#ifdef ServerMainVers_2700
    const GS::ObjectState* amVariationGuid = variationOS.Get ("amVariationGuid");
    if (amVariationGuid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation.amVariationGuid is required from Archicad 27.");
    }
    variation.amVariationGuid = GetGuidFromObjectState (*amVariationGuid);
#else
    const GS::ObjectState* renovationFilterGuid = variationOS.Get ("renovationFilterGuid");
    if (renovationFilterGuid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation.renovationFilterGuid is required before Archicad 27.");
    }
    variation.renovationFilterGuid = GetGuidFromObjectState (*renovationFilterGuid);
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> classIds;
    variationOS.Get ("layerConnectionClassIds", classIds);
    for (const GS::ObjectState& classId : classIds) {
        Int32 layerIndex = 0;
        Int32 connectionClassId = 0;
        if (!classId.Get ("layerIndex", layerIndex) || !classId.Get ("connectionClassId", connectionClassId)) {
            return CreateErrorResponse (APIERR_BADPARS, "Each item of variation.layerConnectionClassIds needs a layerIndex and a connectionClassId in Archicad 26.");
        }
        variation.layerConnectionClassIds.Put (ACAPI_CreateAttributeIndex (layerIndex), connectionClassId);
    }
#else
    variationOS.Get ("layerConnectionClassIds", variation.layerConnectionClassIds);
#endif
#endif
    return GS::NoValue;
}

#ifdef ServerMainVers_2600
GS::ObjectState CreateAnalyticalVisibilitySettingsObjectState (const API_AM_VisibilitySettings& settings)
{
    GS::ObjectState settingsOS;
    const char* visibilityMode = EnumToString (AnalyticalVisibilityModeNames, settings.visibilityMode);
    if (visibilityMode != nullptr) {
        settingsOS.Add ("visibilityMode", GS::UniString (visibilityMode));
    }
    for (const VisibilityFlag& flag : AnalyticalVisibilityFlags) {
        settingsOS.Add (flag.name, settings.*(flag.field));
    }
    const char* loadVisibilityMode = EnumToString (AnalyticalLoadVisibilityModeNames, settings.loadVisibilityMode);
    if (loadVisibilityMode != nullptr) {
        settingsOS.Add ("loadVisibilityMode", GS::UniString (loadVisibilityMode));
    }
    return settingsOS;
}
#endif

// GS::HashTable's const range-for pair holds references from Archicad 28 and pointers before.
template <typename Pair>
const auto& PairKey (const Pair& pair)
{
#ifdef ServerMainVers_2800
    return pair.key;
#else
    return *pair.key;
#endif
}

template <typename Pair>
const auto& PairValue (const Pair& pair)
{
#ifdef ServerMainVers_2800
    return pair.value;
#else
    return *pair.value;
#endif
}

template <typename EnumType, size_t N>
void AddEnumName (GS::ObjectState& os, const char* fieldName, const EnumName<EnumType> (&names)[N], EnumType value)
{
    const char* name = EnumToString (names, value);
    if (name != nullptr) {
        os.Add (fieldName, GS::UniString (name));
    }
}

const EnumName<API_AnalyticalCurveType> AnalyticalCurveTypeNames[] = {
    { API_AnalyticalCurve_LineType,    "Line" },
    { API_AnalyticalCurve_ArcType,     "Arc" },
    { API_AnalyticalCurve_SplineType,  "Spline" },
#ifdef ServerMainVers_2600
    { API_AnalyticalCurve_BezierType,  "Bezier" },
#endif
    { API_AnalyticalCurve_EllipseType, "Ellipse" }
};

const EnumName<API_AnalyticalCrossSectionType> AnalyticalCrossSectionTypeNames[] = {
    { API_CrossSection_Rectangle, "Rectangle" },
    { API_CrossSection_Circle,    "Circle" },
    { API_CrossSection_Attribute, "Attribute" },
    { API_CrossSection_Custom,    "Custom" }
};

const EnumName<API_Analytical1DFEMType> Analytical1DFEMTypeNames[] = {
    { API_Analytical1DFEM_Standard,        "Standard" },
    { API_Analytical1DFEM_AxialForceOnly,  "AxialForceOnly" },
    { API_Analytical1DFEM_CompressionOnly, "CompressionOnly" },
    { API_Analytical1DFEM_TensionOnly,     "TensionOnly" }
};

const EnumName<API_Analytical2DFEMType> Analytical2DFEMTypeNames[] = {
    { API_Analytical2DFEM_Plate, "Plate" },
    { API_Analytical2DFEM_Wall,  "Wall" },
    { API_Analytical2DFEM_Shell, "Shell" }
};

const EnumName<API_AnalyticalSurfaceType> AnalyticalSurfaceTypeNames[] = {
    { API_AnalyticalSurface_PlaneType,    "Plane" },
    { API_AnalyticalSurface_CylinderType, "Cylinder" }
};

#ifdef ServerMainVers_2600
const EnumName<API_AnalyticalAlignmentType> AnalyticalAlignmentTypeNames[] = {
    { API_AnalyticalAlignmentType::TopLeft,     "TopLeft" },
    { API_AnalyticalAlignmentType::Top,         "Top" },
    { API_AnalyticalAlignmentType::TopRight,    "TopRight" },
    { API_AnalyticalAlignmentType::Left,        "Left" },
    { API_AnalyticalAlignmentType::Center,      "Center" },
    { API_AnalyticalAlignmentType::Right,       "Right" },
    { API_AnalyticalAlignmentType::BottomLeft,  "BottomLeft" },
    { API_AnalyticalAlignmentType::Bottom,      "Bottom" },
    { API_AnalyticalAlignmentType::BottomRight, "BottomRight" },
    { API_AnalyticalAlignmentType::None,        "None" }
};

const EnumName<API_AnalyticalMemberPartRefType> AnalyticalMemberPartRefTypeNames[] = {
    { API_AMPartRef_BegVertex,     "BegVertex" },
    { API_AMPartRef_EndVertex,     "EndVertex" },
    { API_AMPartRef_CustomVertex,  "CustomVertex" },
    { API_AMPartRef_Edge,          "Edge" },
    { API_AMPartRef_RatioEdge,     "RatioEdge" },
    { API_AMPartRef_Surface,       "Surface" },
    { API_AMPartRef_CustomSurface, "CustomSurface" },
    { API_AMPartRef_CustomEdge,    "CustomEdge" }
};

const EnumName<API_AnalyticalRigidityType> AnalyticalRigidityTypeNames[] = {
    { API_FreeType,                  "Free" },
    { API_RigidType,                 "Rigid" },
    { API_CustomType,                "Custom" },
    { API_RigidCompressionOnlyType,  "RigidCompressionOnly" },
    { API_RigidTensionOnlyType,      "RigidTensionOnly" },
    { API_CustomCompressionOnlyType, "CustomCompressionOnly" },
    { API_CustomTensionOnlyType,     "CustomTensionOnly" }
};

struct ReleaseComponent
{
    API_AnalyticalRigidityType API_AnalyticalReleaseDescription::* rigidity;
    double API_AnalyticalReleaseDescription::* stiffness;
    const char* rigidityName;
    const char* stiffnessName;
};

const ReleaseComponent AnalyticalReleaseComponents[] = {
    { &API_AnalyticalReleaseDescription::ux,  &API_AnalyticalReleaseDescription::stiffnessX,   "ux",  "stiffnessX" },
    { &API_AnalyticalReleaseDescription::uy,  &API_AnalyticalReleaseDescription::stiffnessY,   "uy",  "stiffnessY" },
    { &API_AnalyticalReleaseDescription::uz,  &API_AnalyticalReleaseDescription::stiffnessZ,   "uz",  "stiffnessZ" },
    { &API_AnalyticalReleaseDescription::fix, &API_AnalyticalReleaseDescription::stiffnessFix, "fix", "stiffnessFix" },
    { &API_AnalyticalReleaseDescription::fiy, &API_AnalyticalReleaseDescription::stiffnessFiy, "fiy", "stiffnessFiy" },
    { &API_AnalyticalReleaseDescription::fiz, &API_AnalyticalReleaseDescription::stiffnessFiz, "fiz", "stiffnessFiz" }
};
#endif

GSErrCode TAPIR_GetCurveElements (const API_Guid& amGuid, GS::Array<API_Guid>& curveElements)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetCurveElements (ACAPI_GetToken (), amGuid, curveElements);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetCurveElements (ACAPI_GetToken (), amGuid, curveElements);
#else
    return ACAPI_AnalyticalModel_GetCurveElements (amGuid, &curveElements);
#endif
}

GSErrCode TAPIR_GetSurfaceElements (const API_Guid& amGuid, GS::Array<API_Guid>& surfaceElements)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetSurfaceElements (ACAPI_GetToken (), amGuid, surfaceElements);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetSurfaceElements (ACAPI_GetToken (), amGuid, surfaceElements);
#else
    return ACAPI_AnalyticalModel_GetSurfaceElements (amGuid, &surfaceElements);
#endif
}

GSErrCode TAPIR_GetCurveMember (const API_Guid& amGuid, const API_Guid& elemGuid, API_AnalyticalCurveMember& curveMember)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetCurveMember (ACAPI_GetToken (), amGuid, elemGuid, curveMember);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetCurveMember (ACAPI_GetToken (), amGuid, elemGuid, curveMember);
#else
    return ACAPI_AnalyticalModel_GetCurveMember (amGuid, elemGuid, &curveMember);
#endif
}

GSErrCode TAPIR_GetSurfaceMember (const API_Guid& amGuid, const API_Guid& elemGuid, API_AnalyticalSurfaceMember& surfaceMember, API_ExportIdMappingType& idMapping)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetSurfaceMember (ACAPI_GetToken (), amGuid, elemGuid, surfaceMember, idMapping);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetSurfaceMember (ACAPI_GetToken (), amGuid, elemGuid, surfaceMember, idMapping);
#else
    return ACAPI_AnalyticalModel_GetSurfaceMember (amGuid, elemGuid, &surfaceMember, idMapping);
#endif
}

GSErrCode TAPIR_GetSegmentedSurfaceMember (const API_Guid& amGuid, const API_Guid& elemGuid, GS::Array<API_AnalyticalSurfaceMember>& surfaceMembers, double centralAngle, API_ExportIdMappingType& idMapping)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetSegmentedSurfaceMember (ACAPI_GetToken (), amGuid, elemGuid, surfaceMembers, centralAngle, idMapping);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetSegmentedSurfaceMember (ACAPI_GetToken (), amGuid, elemGuid, surfaceMembers, centralAngle, idMapping);
#else
    return ACAPI_AnalyticalModel_GetSegmentedSurfaceMember (amGuid, elemGuid, &surfaceMembers, centralAngle, idMapping);
#endif
}

GSErrCode TAPIR_GetAnalyticalMemberConnections (const API_Guid& amGuid, const API_Guid& elemGuid, GS::HashSet<API_Guid>& connections)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalMemberConnections (ACAPI_GetToken (), amGuid, elemGuid, connections);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalMemberConnections (ACAPI_GetToken (), amGuid, elemGuid, connections);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalMemberConnections (amGuid, elemGuid, connections);
#endif
}

// Archicad 25 declares the functions that take an API_AnalyticalMemberPartRef too, but the fields of its
// part reference are not known here, so the commands using part references start with Archicad 26.
#ifdef ServerMainVers_2600
GSErrCode TAPIR_GetAnalyticalMemberPartRefCoord (const API_Guid& amGuid, const API_AnalyticalMemberPartRef& memberPartRef, API_AnalyticalNode& node)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetAnalyticalMemberPartRefCoord (ACAPI_GetToken (), amGuid, memberPartRef, node);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalMemberPartRefCoord (ACAPI_GetToken (), amGuid, memberPartRef, node);
#endif
}

GSErrCode TAPIR_GetAnalyticalMemberPartRefCurve (const API_Guid& amGuid, const API_AnalyticalMemberPartRef& memberPartRef, API_AnalyticalCurve& curve)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetAnalyticalMemberPartRefCurve (ACAPI_GetToken (), amGuid, memberPartRef, curve);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalMemberPartRefCurve (ACAPI_GetToken (), amGuid, memberPartRef, curve);
#endif
}

GSErrCode TAPIR_GetLCSOfAnalyticalMember (const API_AnalyticalMemberPartRef& memberPartRef, API_CoordinateSystem& coordinateSystem)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetLCSOfAnalyticalMember (ACAPI_GetToken (), memberPartRef, coordinateSystem);
#else
    return ACAPI_AnalyticalModel_GetLCSOfAnalyticalMember (ACAPI_GetToken (), memberPartRef, coordinateSystem);
#endif
}

GSErrCode TAPIR_GetLCSOfAnalyticalMemberInIntersection (const API_AnalyticalMemberPartRef& memberPartRef, const API_AnalyticalMemberPartRef& intersectingMemberPartRef, API_CoordinateSystem& coordinateSystem)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetLCSOfAnalyticalMemberInIntersection (ACAPI_GetToken (), memberPartRef, intersectingMemberPartRef, coordinateSystem);
#else
    return ACAPI_AnalyticalModel_GetLCSOfAnalyticalMemberInIntersection (ACAPI_GetToken (), memberPartRef, intersectingMemberPartRef, coordinateSystem);
#endif
}

GSErrCode TAPIR_GetAnalyticalRelease (const API_Guid& amGuid, const API_Guid& elemGuid, API_AnalyticalReleaseType& release)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_GetAnalyticalRelease (ACAPI_GetToken (), amGuid, elemGuid, release);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalRelease (ACAPI_GetToken (), amGuid, elemGuid, release);
#endif
}

GSErrCode TAPIR_SetAnalyticalRelease (const API_Guid& elemGuid, const API_AnalyticalReleaseType& release)
{
#ifdef ServerMainVers_2700
    return ACAPI_Analytical_SetAnalyticalRelease (ACAPI_GetToken (), elemGuid, release);
#else
    return ACAPI_AnalyticalModel_SetAnalyticalRelease (ACAPI_GetToken (), elemGuid, release);
#endif
}
#endif

// API_Vector3D is API_Coord3D from Archicad 27 and its own struct with the same fields before.
template <typename Vector>
GS::ObjectState CreateVector3DObjectState (const Vector& v)
{
    return GS::ObjectState ("x", v.x, "y", v.y, "z", v.z);
}

GS::ObjectState CreateAnalyticalNodeObjectState (const API_AnalyticalNode& node)
{
    return GS::ObjectState (
        "nodeId", static_cast<Int32> (node.nodeId),
        "coord", Create3DCoordinateObjectState (node.coord));
}

GS::ObjectState CreateAnalyticalCurveObjectState (const API_AnalyticalCurve& curve)
{
    GS::ObjectState curveOS;
    AddEnumName (curveOS, "type", AnalyticalCurveTypeNames, curve.type);
    curveOS.Add ("begNode", CreateAnalyticalNodeObjectState (curve.begNode));
    curveOS.Add ("endNode", CreateAnalyticalNodeObjectState (curve.endNode));
    const auto& midCoords = curveOS.AddList<GS::ObjectState> ("midCoords");
    for (const API_Coord3D& coord : curve.midCoords) {
        midCoords (Create3DCoordinateObjectState (coord));
    }
    return curveOS;
}

void AddAnalyticalNodes (GS::ObjectState& os, const char* fieldName, const GS::Array<API_AnalyticalNode>& nodes)
{
    const auto& nodesOS = os.AddList<GS::ObjectState> (fieldName);
    for (const API_AnalyticalNode& node : nodes) {
        nodesOS (CreateAnalyticalNodeObjectState (node));
    }
}

void AddAnalyticalCurves (GS::ObjectState& os, const char* fieldName, const GS::Array<API_AnalyticalCurve>& curves)
{
    const auto& curvesOS = os.AddList<GS::ObjectState> (fieldName);
    for (const API_AnalyticalCurve& curve : curves) {
        curvesOS (CreateAnalyticalCurveObjectState (curve));
    }
}

// The profile image of Archicad 26 and newer has no JSON form and is left out.
GS::ObjectState CreateAnalyticalCrossSectionObjectState (const API_AnalyticalCrossSection& crossSection)
{
    GS::ObjectState crossSectionOS;
    AddEnumName (crossSectionOS, "crossSectionType", AnalyticalCrossSectionTypeNames, crossSection.crossSectionType);
    crossSectionOS.Add ("width", crossSection.width);
    crossSectionOS.Add ("height", crossSection.height);
    crossSectionOS.Add ("veneerThickness", crossSection.veneerThickness);
    crossSectionOS.Add ("profileAttrIndex", GetAttributeIndex (crossSection.profileAttrIndex));
    const auto& materialAttrIndices = crossSectionOS.AddList<Int32> ("materialAttrIndices");
    for (const API_AttributeIndex& materialAttrIndex : crossSection.materialAttrIndices) {
        materialAttrIndices (GetAttributeIndex (materialAttrIndex));
    }
    const auto& profileParameters = crossSectionOS.AddList<GS::ObjectState> ("profileParameters");
    for (const auto& kv : crossSection.profileParameters) {
        profileParameters (GS::ObjectState (
            "parameterId", CreateGuidObjectState (PairKey (kv)),
            "value", PairValue (kv)));
    }
#ifdef ServerMainVers_2600
    AddEnumName (crossSectionOS, "alignment", AnalyticalAlignmentTypeNames, crossSection.alignment);
#endif
    return crossSectionOS;
}

GS::ObjectState CreateAnalyticalCurveMemberObjectState (const API_AnalyticalCurveMember& curveMember)
{
    GS::ObjectState curveMemberOS;
    AddEnumName (curveMemberOS, "analyticalMemberType", AnalyticalMemberTypeNames, curveMember.analyticalMemberType);
    curveMemberOS.Add ("elemGuid", CreateGuidObjectState (curveMember.elemGuid));
    curveMemberOS.Add ("crossSection", CreateAnalyticalCrossSectionObjectState (curveMember.crossSection));
    curveMemberOS.Add ("curve", CreateAnalyticalCurveObjectState (curveMember.curve));
    AddAnalyticalNodes (curveMemberOS, "internalNodes", curveMember.internalNodes);
    curveMemberOS.Add ("rotationAngle", curveMember.rotationAngle);
    curveMemberOS.Add ("ey", curveMember.ey);
    curveMemberOS.Add ("eyVector", CreateVector3DObjectState (curveMember.eyVector));
    curveMemberOS.Add ("ez", curveMember.ez);
    curveMemberOS.Add ("ezVector", CreateVector3DObjectState (curveMember.ezVector));
    curveMemberOS.Add ("layerAttrIndex", GetAttributeIndex (curveMember.layerAttrIndex));
    AddEnumName (curveMemberOS, "FEMType", Analytical1DFEMTypeNames, curveMember.FEMType);
#ifdef ServerMainVers_2600
    const auto& varyingSegmentList = curveMemberOS.AddList<GS::ObjectState> ("varyingSegmentList");
    for (const API_AnalyticalVaryingSegment& segment : curveMember.varyingSegmentList) {
        GS::ObjectState segmentOS;
        segmentOS.Add ("begProfile", CreateAnalyticalCrossSectionObjectState (segment.begProfile));
        if (segment.endProfile.HasValue ()) {
            segmentOS.Add ("endProfile", CreateAnalyticalCrossSectionObjectState (*segment.endProfile));
        }
        segmentOS.Add ("span", segment.span);
        varyingSegmentList (segmentOS);
    }
#endif
    return curveMemberOS;
}

GS::ObjectState CreateAnalyticalSurfaceObjectState (const API_AnalyticalSurface& surface)
{
    GS::ObjectState surfaceOS;
    AddEnumName (surfaceOS, "surfaceType", AnalyticalSurfaceTypeNames, surface.surfaceType);
    surfaceOS.Add ("planeSurface", GS::ObjectState (
        "basePoint", Create3DCoordinateObjectState (surface.planeSurface.basePoint),
        "normalVector", CreateVector3DObjectState (surface.planeSurface.normalVector)));
    surfaceOS.Add ("cylinderSurface", GS::ObjectState (
        "basePoint", Create3DCoordinateObjectState (surface.cylinderSurface.basePoint),
        "axisDirection", CreateVector3DObjectState (surface.cylinderSurface.axisDirection),
        "referenceRay", CreateVector3DObjectState (surface.cylinderSurface.referenceRay)));
    return surfaceOS;
}

GS::ObjectState CreateAnalyticalSurfaceMemberObjectState (const API_AnalyticalSurfaceMember& surfaceMember)
{
    GS::ObjectState surfaceMemberOS;
    AddEnumName (surfaceMemberOS, "analyticalMemberType", AnalyticalMemberTypeNames, surfaceMember.analyticalMemberType);
    surfaceMemberOS.Add ("elemGuid", CreateGuidObjectState (surfaceMember.elemGuid));
    surfaceMemberOS.Add ("surface", CreateAnalyticalSurfaceObjectState (surfaceMember.surface));
    AddAnalyticalCurves (surfaceMemberOS, "contour", surfaceMember.contour);
    const auto& holeContours = surfaceMemberOS.AddList<GS::ObjectState> ("holeContours");
    for (const GS::Array<API_AnalyticalCurve>& holeContour : surfaceMember.holeContours) {
        GS::ObjectState holeContourOS;
        AddAnalyticalCurves (holeContourOS, "contour", holeContour);
        holeContours (holeContourOS);
    }
    AddAnalyticalNodes (surfaceMemberOS, "internalNodes", surfaceMember.internalNodes);
    surfaceMemberOS.Add ("thickness", surfaceMember.thickness);
#ifdef ServerMainVers_2600
    surfaceMemberOS.Add ("lcsXVector", CreateVector3DObjectState (surfaceMember.lcsXVector));
#endif
    surfaceMemberOS.Add ("ez", surfaceMember.ez);
    surfaceMemberOS.Add ("ezVector", CreateVector3DObjectState (surfaceMember.ezVector));
    surfaceMemberOS.Add ("materialAttrIndex", GetAttributeIndex (surfaceMember.materialAttrIndex));
    surfaceMemberOS.Add ("layerAttrIndex", GetAttributeIndex (surfaceMember.layerAttrIndex));
    AddEnumName (surfaceMemberOS, "FEMType", Analytical2DFEMTypeNames, surfaceMember.FEMType);
    surfaceMemberOS.Add ("groupId", CreateGuidObjectState (surfaceMember.groupId));
    return surfaceMemberOS;
}

#ifdef ServerMainVers_2600
GS::ObjectState CreateInternalAnalyticalMemberPartIdObjectState (const API_InternalAnalyticalMemberPartId& partId)
{
    return GS::ObjectState (
        "contourIdx", static_cast<Int32> (partId.contourIdx),
        "edgeOrVertexIdx", static_cast<Int32> (partId.edgeOrVertexIdx),
        "edgeFragmentIndex", static_cast<Int32> (partId.edgeFragmentIndex));
}

GS::ObjectState CreateExportMappingTableObjectState (const API_ExportMappingTable& table)
{
    GS::ObjectState tableOS;
    const auto& items = tableOS.AddList<GS::ObjectState> ("items");
    for (const auto& kv : table) {
        GS::ObjectState item;
        item.Add ("partId", CreateInternalAnalyticalMemberPartIdObjectState (PairKey (kv)));
        const auto& elemPartIds = item.AddList<GS::ObjectState> ("elemPartIds");
        for (const API_AnalyticalElemPartId& elemPartId : PairValue (kv)) {
            elemPartIds (GS::ObjectState (
                "memberGuid", CreateGuidObjectState (elemPartId.memberGuid),
                "contourIndex", static_cast<Int32> (elemPartId.contourIndex),
                "edgeOrVertexIndex", static_cast<Int32> (elemPartId.edgeOrVertexIndex)));
        }
        items (item);
    }
    return tableOS;
}

GS::ObjectState CreateExportIdMappingObjectState (const API_ExportIdMappingType& idMapping)
{
    return GS::ObjectState (
        "edges", CreateExportMappingTableObjectState (idMapping.edges),
        "vertices", CreateExportMappingTableObjectState (idMapping.vertices));
}

GS::ObjectState CreateAnalyticalMemberPartRefObjectState (const API_AnalyticalMemberPartRef& memberPartRef)
{
    GS::ObjectState memberPartRefOS;
    memberPartRefOS.Add ("elemGuid", CreateGuidObjectState (memberPartRef.elemGuid));
    AddEnumName (memberPartRefOS, "partRefType", AnalyticalMemberPartRefTypeNames, memberPartRef.partRefType);
    memberPartRefOS.Add ("partId", CreateInternalAnalyticalMemberPartIdObjectState (memberPartRef.partId));
    memberPartRefOS.Add ("customVertexData", GS::ObjectState ("customVertexCoord", Create3DCoordinateObjectState (memberPartRef.customVertexData.customVertexCoord)));
    memberPartRefOS.Add ("ratioEdgeData", GS::ObjectState (
        "ratioStart", memberPartRef.ratioEdgeData.ratioStart,
        "ratioEnd", memberPartRef.ratioEdgeData.ratioEnd));
    return memberPartRefOS;
}

// Fills the part reference from its JSON form; partId, customVertexData and ratioEdgeData are zero when
// left out. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalMemberPartRefFromObjectState (const GS::ObjectState& memberPartRefOS, API_AnalyticalMemberPartRef& memberPartRef)
{
    const GS::ObjectState* elemGuid = memberPartRefOS.Get ("elemGuid");
    if (elemGuid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "The member part reference has no elemGuid.");
    }
    memberPartRef.elemGuid = GetGuidFromObjectState (*elemGuid);

    GS::UniString partRefType;
    if (!memberPartRefOS.Get ("partRefType", partRefType)) {
        return CreateErrorResponse (APIERR_BADPARS, "The member part reference has no partRefType.");
    }
    if (!EnumFromString (AnalyticalMemberPartRefTypeNames, partRefType, memberPartRef.partRefType)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown partRefType '%T'.", partRefType.ToPrintf ()));
    }

    Int32 contourIdx = 0;
    Int32 edgeOrVertexIdx = 0;
    Int32 edgeFragmentIndex = 0;
    const GS::ObjectState* partId = memberPartRefOS.Get ("partId");
    if (partId != nullptr) {
        partId->Get ("contourIdx", contourIdx);
        partId->Get ("edgeOrVertexIdx", edgeOrVertexIdx);
        partId->Get ("edgeFragmentIndex", edgeFragmentIndex);
    }
    memberPartRef.partId.contourIdx = static_cast<UIndex> (contourIdx);
    memberPartRef.partId.edgeOrVertexIdx = static_cast<UIndex> (edgeOrVertexIdx);
    memberPartRef.partId.edgeFragmentIndex = static_cast<UIndex> (edgeFragmentIndex);

    memberPartRef.customVertexData.customVertexCoord = API_Coord3D {};
    const GS::ObjectState* customVertexData = memberPartRefOS.Get ("customVertexData");
    if (customVertexData != nullptr) {
        const GS::ObjectState* customVertexCoord = customVertexData->Get ("customVertexCoord");
        if (customVertexCoord != nullptr) {
            memberPartRef.customVertexData.customVertexCoord = Get3DCoordinateFromObjectState (*customVertexCoord);
        }
    }

    memberPartRef.ratioEdgeData.ratioStart = 0.0;
    memberPartRef.ratioEdgeData.ratioEnd = 0.0;
    const GS::ObjectState* ratioEdgeData = memberPartRefOS.Get ("ratioEdgeData");
    if (ratioEdgeData != nullptr) {
        ratioEdgeData->Get ("ratioStart", memberPartRef.ratioEdgeData.ratioStart);
        ratioEdgeData->Get ("ratioEnd", memberPartRef.ratioEdgeData.ratioEnd);
    }

    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalReleaseObjectState (const API_AnalyticalReleaseType& release)
{
    GS::ObjectState releaseOS;
    releaseOS.Add ("guid", CreateGuidObjectState (release.guid));
    const auto& releaseDescriptions = releaseOS.AddList<GS::ObjectState> ("releaseDescriptions");
    for (const API_AnalyticalReleaseDescription& description : release.releaseDescriptions) {
        GS::ObjectState descriptionOS;
        for (const ReleaseComponent& component : AnalyticalReleaseComponents) {
            AddEnumName (descriptionOS, component.rigidityName, AnalyticalRigidityTypeNames, description.*(component.rigidity));
            descriptionOS.Add (component.stiffnessName, description.*(component.stiffness));
        }
        descriptionOS.Add ("amPartRef", CreateAnalyticalMemberPartRefObjectState (description.amPartRef));
        releaseDescriptions (descriptionOS);
    }
    return releaseOS;
}

// Fills the release from its JSON form, as GetAnalyticalReleases returns it. Every rigidity is required,
// a stiffness left out is 0. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalReleaseFromObjectState (const GS::ObjectState& releaseOS, API_AnalyticalReleaseType& release)
{
    const GS::ObjectState* guid = releaseOS.Get ("guid");
    if (guid == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "The release has no guid.");
    }
    release.guid = GetGuidFromObjectState (*guid);

    GS::Array<GS::ObjectState> descriptionsOS;
    releaseOS.Get ("releaseDescriptions", descriptionsOS);
    release.releaseDescriptions.Clear ();
    for (const GS::ObjectState& descriptionOS : descriptionsOS) {
        API_AnalyticalReleaseDescription description {};
        for (const ReleaseComponent& component : AnalyticalReleaseComponents) {
            GS::UniString rigidity;
            if (!descriptionOS.Get (component.rigidityName, rigidity)) {
                return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("A release description has no %s.", component.rigidityName));
            }
            if (!EnumFromString (AnalyticalRigidityTypeNames, rigidity, description.*(component.rigidity))) {
                return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown %s '%T'.", component.rigidityName, rigidity.ToPrintf ()));
            }
            description.*(component.stiffness) = 0.0;
            descriptionOS.Get (component.stiffnessName, description.*(component.stiffness));
        }
        const GS::ObjectState* amPartRef = descriptionOS.Get ("amPartRef");
        if (amPartRef == nullptr) {
            return CreateErrorResponse (APIERR_BADPARS, "A release description has no amPartRef.");
        }
        const GS::Optional<GS::ObjectState> partRefError = GetAnalyticalMemberPartRefFromObjectState (*amPartRef, description.amPartRef);
        if (partRefError.HasValue ()) {
            return partRefError;
        }
        release.releaseDescriptions.Push (description);
    }
    return GS::NoValue;
}

GS::ObjectState CreateCoordinateSystemObjectState (const API_CoordinateSystem& coordinateSystem)
{
    return GS::ObjectState (
        "xVector", CreateVector3DObjectState (coordinateSystem.xVector),
        "yVector", CreateVector3DObjectState (coordinateSystem.yVector));
}

GS::ObjectState CreateImportMappingTableObjectState (const API_ImportMappingTable& table)
{
    GS::ObjectState tableOS;
    const auto& items = tableOS.AddList<GS::ObjectState> ("items");
    for (const auto& kv : table) {
        const API_AnalyticalElemPartId& elemPartId = PairKey (kv);
        items (GS::ObjectState (
            "elemPartId", GS::ObjectState (
                "memberGuid", CreateGuidObjectState (elemPartId.memberGuid),
                "contourIndex", static_cast<Int32> (elemPartId.contourIndex),
                "edgeOrVertexIndex", static_cast<Int32> (elemPartId.edgeOrVertexIndex)),
            "memberPartRef", CreateAnalyticalMemberPartRefObjectState (PairValue (kv))));
    }
    return tableOS;
}

GS::ObjectState CreateImportIdMappingObjectState (const API_ImportIdMappingType& idMapping)
{
    return GS::ObjectState (
        "edges", CreateImportMappingTableObjectState (idMapping.edges),
        "vertices", CreateImportMappingTableObjectState (idMapping.vertices));
}
#endif

// The members are created from the given structs: the elemGuid of the input is not passed on, the
// function fills in the GUID of the new element. Archicad 25 returns the GUID in an output parameter,
// Archicad 26 and newer in the elemGuid of the member (of a segment for a segmented member).
// Archicad 25 gets no profile image and no id mapping, whose part references are not known here.
GSErrCode TAPIR_CreateElementFromCurveMember (API_AnalyticalCurveMember& curveMember, const GS::Optional<GS::UniString>& favoriteName, API_Guid& elemGuid)
{
#if defined (ServerMainVers_2700)
    const GSErrCode err = ACAPI_Analytical_CreateElementFromCurveMember (ACAPI_GetToken (), curveMember, favoriteName);
    elemGuid = curveMember.elemGuid;
    return err;
#elif defined (ServerMainVers_2600)
    const GSErrCode err = ACAPI_AnalyticalModel_CreateElementFromCurveMember (ACAPI_GetToken (), curveMember, favoriteName);
    elemGuid = curveMember.elemGuid;
    return err;
#else
    return ACAPI_AnalyticalModel_CreateElementFromCurveMember (curveMember, nullptr, favoriteName.HasValue () ? *favoriteName : GS::UniString (), &elemGuid);
#endif
}

GSErrCode TAPIR_CreateElementFromSurfaceMember (API_AnalyticalSurfaceMember& surfaceMember, const GS::Optional<GS::UniString>& favoriteName, API_ImportIdMappingType& idMapping, API_Guid& elemGuid)
{
#if defined (ServerMainVers_2700)
    const GSErrCode err = ACAPI_Analytical_CreateElementFromSurfaceMember (ACAPI_GetToken (), surfaceMember, favoriteName, &idMapping);
    elemGuid = surfaceMember.elemGuid;
    return err;
#elif defined (ServerMainVers_2600)
    const GSErrCode err = ACAPI_AnalyticalModel_CreateElementFromSurfaceMember (ACAPI_GetToken (), surfaceMember, favoriteName, &idMapping);
    elemGuid = surfaceMember.elemGuid;
    return err;
#else
    UNUSED_PARAMETER (idMapping);
    return ACAPI_AnalyticalModel_CreateElementFromSurfaceMember (surfaceMember, favoriteName.HasValue () ? *favoriteName : GS::UniString (), &elemGuid, nullptr);
#endif
}

#ifdef ServerMainVers_2600
API_Guid GetFirstElemGuid (const GS::Array<API_AnalyticalSurfaceMember>& surfaceMembers)
{
    for (const API_AnalyticalSurfaceMember& surfaceMember : surfaceMembers) {
        if (surfaceMember.elemGuid != APINULLGuid) {
            return surfaceMember.elemGuid;
        }
    }
    return APINULLGuid;
}
#endif

GSErrCode TAPIR_CreateElementFromSegmentedSurfaceMember (GS::Array<API_AnalyticalSurfaceMember>& surfaceMembers, const GS::Optional<GS::UniString>& favoriteName, API_ImportIdMappingType& idMapping, API_Guid& elemGuid)
{
#if defined (ServerMainVers_2700)
    const GSErrCode err = ACAPI_Analytical_CreateElementFromSegmentedSurfaceMember (ACAPI_GetToken (), surfaceMembers, favoriteName, &idMapping);
    elemGuid = GetFirstElemGuid (surfaceMembers);
    return err;
#elif defined (ServerMainVers_2600)
    const GSErrCode err = ACAPI_AnalyticalModel_CreateElementFromSegmentedSurfaceMember (ACAPI_GetToken (), surfaceMembers, favoriteName, &idMapping);
    elemGuid = GetFirstElemGuid (surfaceMembers);
    return err;
#else
    UNUSED_PARAMETER (idMapping);
    return ACAPI_AnalyticalModel_CreateElementFromSegmentedSurfaceMember (surfaceMembers, favoriteName.HasValue () ? *favoriteName : GS::UniString (), &elemGuid, nullptr);
#endif
}

// Reads the enumerator named in the field when it is given, and leaves the value unchanged otherwise.
// Returns an error response for an unknown name.
template <typename EnumType, size_t N>
GS::Optional<GS::ObjectState> GetEnumFromObjectState (const GS::ObjectState& os, const char* fieldName, const EnumName<EnumType> (&names)[N], EnumType& value)
{
    GS::UniString name;
    if (os.Get (fieldName, name) && !EnumFromString (names, name, value)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown %s '%T'.", fieldName, name.ToPrintf ()));
    }
    return GS::NoValue;
}

template <typename EnumType, size_t N>
GS::Optional<GS::ObjectState> GetRequiredEnumFromObjectState (const GS::ObjectState& os, const char* fieldName, const EnumName<EnumType> (&names)[N], EnumType& value)
{
    if (!os.Contains (fieldName)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("%s is required.", fieldName));
    }
    return GetEnumFromObjectState (os, fieldName, names, value);
}

void GetCoord3DFromObjectState (const GS::ObjectState& os, const char* fieldName, API_Coord3D& coord)
{
    const GS::ObjectState* coordOS = os.Get (fieldName);
    if (coordOS != nullptr) {
        coord = Get3DCoordinateFromObjectState (*coordOS);
    }
}

// API_Vector3D is API_Coord3D from Archicad 27 and its own struct with the same fields before.
template <typename Vector>
void GetVector3DFromObjectState (const GS::ObjectState& os, const char* fieldName, Vector& v)
{
    const GS::ObjectState* vectorOS = os.Get (fieldName);
    if (vectorOS != nullptr) {
        vectorOS->Get ("x", v.x);
        vectorOS->Get ("y", v.y);
        vectorOS->Get ("z", v.z);
    }
}

void GetAttributeIndexFromObjectState (const GS::ObjectState& os, const char* fieldName, API_AttributeIndex& index)
{
    Int32 value = 0;
    if (os.Get (fieldName, value)) {
        index = ACAPI_CreateAttributeIndex (value);
    }
}

void GetAnalyticalNodeFromObjectState (const GS::ObjectState& nodeOS, API_AnalyticalNode& node)
{
    Int32 nodeId = 0;
    nodeOS.Get ("nodeId", nodeId);
    node.nodeId = static_cast<UInt32> (nodeId);
    GetCoord3DFromObjectState (nodeOS, "coord", node.coord);
}

void GetAnalyticalNodesFromObjectState (const GS::ObjectState& os, const char* fieldName, GS::Array<API_AnalyticalNode>& nodes)
{
    GS::Array<GS::ObjectState> nodesOS;
    os.Get (fieldName, nodesOS);
    nodes.Clear ();
    for (const GS::ObjectState& nodeOS : nodesOS) {
        API_AnalyticalNode node {};
        GetAnalyticalNodeFromObjectState (nodeOS, node);
        nodes.Push (node);
    }
}

// Fills the curve from its JSON form; the type is Line when left out. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalCurveFromObjectState (const GS::ObjectState& curveOS, API_AnalyticalCurve& curve)
{
    curve.type = API_AnalyticalCurve_LineType;
    const GS::Optional<GS::ObjectState> typeError = GetEnumFromObjectState (curveOS, "type", AnalyticalCurveTypeNames, curve.type);
    if (typeError.HasValue ()) {
        return typeError;
    }

    const GS::ObjectState* begNode = curveOS.Get ("begNode");
    const GS::ObjectState* endNode = curveOS.Get ("endNode");
    if (begNode == nullptr || endNode == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "A curve needs a begNode and an endNode.");
    }
    GetAnalyticalNodeFromObjectState (*begNode, curve.begNode);
    GetAnalyticalNodeFromObjectState (*endNode, curve.endNode);

    GS::Array<GS::ObjectState> midCoords;
    curveOS.Get ("midCoords", midCoords);
    curve.midCoords.Clear ();
    for (const GS::ObjectState& midCoord : midCoords) {
        curve.midCoords.Push (Get3DCoordinateFromObjectState (midCoord));
    }
    return GS::NoValue;
}

GS::Optional<GS::ObjectState> GetAnalyticalCurvesFromObjectState (const GS::ObjectState& os, const char* fieldName, GS::Array<API_AnalyticalCurve>& curves)
{
    GS::Array<GS::ObjectState> curvesOS;
    os.Get (fieldName, curvesOS);
    curves.Clear ();
    for (const GS::ObjectState& curveOS : curvesOS) {
        API_AnalyticalCurve curve {};
        const GS::Optional<GS::ObjectState> curveError = GetAnalyticalCurveFromObjectState (curveOS, curve);
        if (curveError.HasValue ()) {
            return curveError;
        }
        curves.Push (curve);
    }
    return GS::NoValue;
}

// Fills the cross section from its JSON form, as GetAnalyticalMemberDetails returns it. The profile image
// of Archicad 26 and newer stays empty. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalCrossSectionFromObjectState (const GS::ObjectState& crossSectionOS, API_AnalyticalCrossSection& crossSection)
{
    GS::Optional<GS::ObjectState> enumError = GetEnumFromObjectState (crossSectionOS, "crossSectionType", AnalyticalCrossSectionTypeNames, crossSection.crossSectionType);
    if (enumError.HasValue ()) {
        return enumError;
    }
    crossSectionOS.Get ("width", crossSection.width);
    crossSectionOS.Get ("height", crossSection.height);
    crossSectionOS.Get ("veneerThickness", crossSection.veneerThickness);
    GetAttributeIndexFromObjectState (crossSectionOS, "profileAttrIndex", crossSection.profileAttrIndex);

    GS::Array<Int32> materialAttrIndices;
    crossSectionOS.Get ("materialAttrIndices", materialAttrIndices);
    crossSection.materialAttrIndices.Clear ();
    for (Int32 materialAttrIndex : materialAttrIndices) {
        crossSection.materialAttrIndices.Push (ACAPI_CreateAttributeIndex (materialAttrIndex));
    }

    GS::Array<GS::ObjectState> profileParameters;
    crossSectionOS.Get ("profileParameters", profileParameters);
    crossSection.profileParameters.Clear ();
    for (const GS::ObjectState& profileParameter : profileParameters) {
        const GS::ObjectState* parameterId = profileParameter.Get ("parameterId");
        double value = 0.0;
        if (parameterId == nullptr || !profileParameter.Get ("value", value)) {
            return CreateErrorResponse (APIERR_BADPARS, "Each profile parameter needs a parameterId and a value.");
        }
        crossSection.profileParameters.Put (GetGuidFromObjectState (*parameterId), value);
    }

#ifdef ServerMainVers_2600
    enumError = GetEnumFromObjectState (crossSectionOS, "alignment", AnalyticalAlignmentTypeNames, crossSection.alignment);
    if (enumError.HasValue ()) {
        return enumError;
    }
#endif
    return GS::NoValue;
}

// Fills the curve member from its JSON form, as GetAnalyticalMemberDetails returns it. analyticalMemberType
// and curve are required, the other fields keep their value when left out. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalCurveMemberFromObjectState (const GS::ObjectState& curveMemberOS, API_AnalyticalCurveMember& curveMember)
{
    GS::Optional<GS::ObjectState> error = GetRequiredEnumFromObjectState (curveMemberOS, "analyticalMemberType", AnalyticalMemberTypeNames, curveMember.analyticalMemberType);
    if (error.HasValue ()) {
        return error;
    }

    const GS::ObjectState* crossSection = curveMemberOS.Get ("crossSection");
    if (crossSection != nullptr) {
        error = GetAnalyticalCrossSectionFromObjectState (*crossSection, curveMember.crossSection);
        if (error.HasValue ()) {
            return error;
        }
    }

    const GS::ObjectState* curve = curveMemberOS.Get ("curve");
    if (curve == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "curve is required.");
    }
    error = GetAnalyticalCurveFromObjectState (*curve, curveMember.curve);
    if (error.HasValue ()) {
        return error;
    }

    GetAnalyticalNodesFromObjectState (curveMemberOS, "internalNodes", curveMember.internalNodes);
    curveMemberOS.Get ("rotationAngle", curveMember.rotationAngle);
    curveMemberOS.Get ("ey", curveMember.ey);
    GetVector3DFromObjectState (curveMemberOS, "eyVector", curveMember.eyVector);
    curveMemberOS.Get ("ez", curveMember.ez);
    GetVector3DFromObjectState (curveMemberOS, "ezVector", curveMember.ezVector);
    GetAttributeIndexFromObjectState (curveMemberOS, "layerAttrIndex", curveMember.layerAttrIndex);
    error = GetEnumFromObjectState (curveMemberOS, "FEMType", Analytical1DFEMTypeNames, curveMember.FEMType);
    if (error.HasValue ()) {
        return error;
    }

#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> varyingSegmentList;
    curveMemberOS.Get ("varyingSegmentList", varyingSegmentList);
    curveMember.varyingSegmentList.Clear ();
    for (const GS::ObjectState& segmentOS : varyingSegmentList) {
        API_AnalyticalVaryingSegment segment {};
        const GS::ObjectState* begProfile = segmentOS.Get ("begProfile");
        if (begProfile == nullptr) {
            return CreateErrorResponse (APIERR_BADPARS, "Each varying segment needs a begProfile.");
        }
        error = GetAnalyticalCrossSectionFromObjectState (*begProfile, segment.begProfile);
        if (error.HasValue ()) {
            return error;
        }
        const GS::ObjectState* endProfile = segmentOS.Get ("endProfile");
        if (endProfile != nullptr) {
            API_AnalyticalCrossSection endCrossSection {};
            error = GetAnalyticalCrossSectionFromObjectState (*endProfile, endCrossSection);
            if (error.HasValue ()) {
                return error;
            }
            segment.endProfile = endCrossSection;
        }
        segmentOS.Get ("span", segment.span);
        curveMember.varyingSegmentList.Push (segment);
    }
#endif

    curveMember.elemGuid = APINULLGuid;
    return GS::NoValue;
}

GS::Optional<GS::ObjectState> GetAnalyticalSurfaceFromObjectState (const GS::ObjectState& surfaceOS, API_AnalyticalSurface& surface)
{
    const GS::Optional<GS::ObjectState> typeError = GetEnumFromObjectState (surfaceOS, "surfaceType", AnalyticalSurfaceTypeNames, surface.surfaceType);
    if (typeError.HasValue ()) {
        return typeError;
    }
    const GS::ObjectState* planeSurface = surfaceOS.Get ("planeSurface");
    if (planeSurface != nullptr) {
        GetCoord3DFromObjectState (*planeSurface, "basePoint", surface.planeSurface.basePoint);
        GetVector3DFromObjectState (*planeSurface, "normalVector", surface.planeSurface.normalVector);
    }
    const GS::ObjectState* cylinderSurface = surfaceOS.Get ("cylinderSurface");
    if (cylinderSurface != nullptr) {
        GetCoord3DFromObjectState (*cylinderSurface, "basePoint", surface.cylinderSurface.basePoint);
        GetVector3DFromObjectState (*cylinderSurface, "axisDirection", surface.cylinderSurface.axisDirection);
        GetVector3DFromObjectState (*cylinderSurface, "referenceRay", surface.cylinderSurface.referenceRay);
    }
    return GS::NoValue;
}

// Fills the surface member from its JSON form, as GetAnalyticalMemberDetails returns it. analyticalMemberType
// and contour are required, the other fields keep their value when left out. Returns an error response on failure.
GS::Optional<GS::ObjectState> GetAnalyticalSurfaceMemberFromObjectState (const GS::ObjectState& surfaceMemberOS, API_AnalyticalSurfaceMember& surfaceMember)
{
    GS::Optional<GS::ObjectState> error = GetRequiredEnumFromObjectState (surfaceMemberOS, "analyticalMemberType", AnalyticalMemberTypeNames, surfaceMember.analyticalMemberType);
    if (error.HasValue ()) {
        return error;
    }

    const GS::ObjectState* surface = surfaceMemberOS.Get ("surface");
    if (surface != nullptr) {
        error = GetAnalyticalSurfaceFromObjectState (*surface, surfaceMember.surface);
        if (error.HasValue ()) {
            return error;
        }
    }

    error = GetAnalyticalCurvesFromObjectState (surfaceMemberOS, "contour", surfaceMember.contour);
    if (error.HasValue ()) {
        return error;
    }
    if (surfaceMember.contour.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "contour is required and must not be empty.");
    }

    GS::Array<GS::ObjectState> holeContours;
    surfaceMemberOS.Get ("holeContours", holeContours);
    surfaceMember.holeContours.Clear ();
    for (const GS::ObjectState& holeContourOS : holeContours) {
        GS::Array<API_AnalyticalCurve> holeContour;
        error = GetAnalyticalCurvesFromObjectState (holeContourOS, "contour", holeContour);
        if (error.HasValue ()) {
            return error;
        }
        surfaceMember.holeContours.Push (holeContour);
    }

    GetAnalyticalNodesFromObjectState (surfaceMemberOS, "internalNodes", surfaceMember.internalNodes);
    surfaceMemberOS.Get ("thickness", surfaceMember.thickness);
#ifdef ServerMainVers_2600
    GetVector3DFromObjectState (surfaceMemberOS, "lcsXVector", surfaceMember.lcsXVector);
#endif
    surfaceMemberOS.Get ("ez", surfaceMember.ez);
    GetVector3DFromObjectState (surfaceMemberOS, "ezVector", surfaceMember.ezVector);
    GetAttributeIndexFromObjectState (surfaceMemberOS, "materialAttrIndex", surfaceMember.materialAttrIndex);
    GetAttributeIndexFromObjectState (surfaceMemberOS, "layerAttrIndex", surfaceMember.layerAttrIndex);
    error = GetEnumFromObjectState (surfaceMemberOS, "FEMType", Analytical2DFEMTypeNames, surfaceMember.FEMType);
    if (error.HasValue ()) {
        return error;
    }
    const GS::ObjectState* groupId = surfaceMemberOS.Get ("groupId");
    if (groupId != nullptr) {
        surfaceMember.groupId = GetGuidFromObjectState (*groupId);
    }

    surfaceMember.elemGuid = APINULLGuid;
    return GS::NoValue;
}

// Creates the element of one item of CreateElementsFromAnalyticalMembers and returns its result item.
GS::ObjectState CreateElementFromAnalyticalMember (const GS::ObjectState& memberOS)
{
    GS::Optional<GS::UniString> favoriteName;
    GS::UniString favoriteNameValue;
    if (memberOS.Get ("favoriteName", favoriteNameValue)) {
        favoriteName = favoriteNameValue;
    }

    const GS::ObjectState* curveMemberOS = memberOS.Get ("curveMember");
    const GS::ObjectState* surfaceMemberOS = memberOS.Get ("surfaceMember");
    GS::Array<GS::ObjectState> segmentsOS;
    const bool segmented = memberOS.Get ("segmentedSurfaceMembers", segmentsOS);
    if ((curveMemberOS != nullptr ? 1 : 0) + (surfaceMemberOS != nullptr ? 1 : 0) + (segmented ? 1 : 0) != 1) {
        return CreateErrorResponse (APIERR_BADPARS, "Each member needs exactly one of curveMember, surfaceMember and segmentedSurfaceMembers.");
    }

    API_Guid elemGuid = APINULLGuid;
    API_ImportIdMappingType idMapping;
    GSErrCode err = NoError;
    bool hasIdMapping = false;

    if (curveMemberOS != nullptr) {
        API_AnalyticalCurveMember curveMember {};
        const GS::Optional<GS::ObjectState> memberError = GetAnalyticalCurveMemberFromObjectState (*curveMemberOS, curveMember);
        if (memberError.HasValue ()) {
            return *memberError;
        }
        err = TAPIR_CreateElementFromCurveMember (curveMember, favoriteName, elemGuid);
    } else if (surfaceMemberOS != nullptr) {
        API_AnalyticalSurfaceMember surfaceMember {};
        const GS::Optional<GS::ObjectState> memberError = GetAnalyticalSurfaceMemberFromObjectState (*surfaceMemberOS, surfaceMember);
        if (memberError.HasValue ()) {
            return *memberError;
        }
        err = TAPIR_CreateElementFromSurfaceMember (surfaceMember, favoriteName, idMapping, elemGuid);
        hasIdMapping = true;
    } else {
        if (segmentsOS.IsEmpty ()) {
            return CreateErrorResponse (APIERR_BADPARS, "segmentedSurfaceMembers must not be empty.");
        }
        GS::Array<API_AnalyticalSurfaceMember> surfaceMembers;
        for (const GS::ObjectState& segmentOS : segmentsOS) {
            API_AnalyticalSurfaceMember surfaceMember {};
            const GS::Optional<GS::ObjectState> memberError = GetAnalyticalSurfaceMemberFromObjectState (segmentOS, surfaceMember);
            if (memberError.HasValue ()) {
                return *memberError;
            }
            surfaceMembers.Push (surfaceMember);
        }
        err = TAPIR_CreateElementFromSegmentedSurfaceMember (surfaceMembers, favoriteName, idMapping, elemGuid);
        hasIdMapping = true;
    }

    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create the element from the analytical member.");
    }
    if (elemGuid == APINULLGuid) {
        return CreateErrorResponse (APIERR_GENERAL, "The element was created, but Archicad did not return its identifier.");
    }

    GS::ObjectState result = CreateElementIdObjectState (elemGuid);
#ifdef ServerMainVers_2600
    if (hasIdMapping) {
        result.Add ("idMapping", CreateImportIdMappingObjectState (idMapping));
    }
#else
    UNUSED_VARIABLE (hasIdMapping);
#endif
    return result;
}

GS::HashSet<API_Guid> CreateGuidSet (const GS::Array<API_Guid>& guids)
{
    GS::HashSet<API_Guid> result;
    for (const API_Guid& guid : guids) {
        result.Add (guid);
    }
    return result;
}

}

GetCurrentAnalyticalModelCommand::GetCurrentAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentAnalyticalModelCommand::GetName () const
{
    return "GetCurrentAnalyticalModel";
}

GS::Optional<GS::UniString> GetCurrentAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model of the current view."
            }
        },
        "additionalProperties": false,
        "required": [
            "analyticalModelId"
        ]
    })";
}

GS::ObjectState GetCurrentAnalyticalModelCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the current view.");
    }

    return GS::ObjectState ("analyticalModelId", CreateGuidObjectState (analyticalModelGuid));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

UpdateAnalyticalModelCommand::UpdateAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateAnalyticalModelCommand::GetName () const
{
    return "UpdateAnalyticalModel";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model to update. The model of the current view is updated when it is omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> UpdateAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState UpdateAnalyticalModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::ObjectState* analyticalModelId = parameters.Get ("analyticalModelId");
    if (analyticalModelId != nullptr) {
        analyticalModelGuid = GetGuidFromObjectState (*analyticalModelId);
        if (analyticalModelGuid == APINULLGuid) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid analyticalModelId.");
        }
    } else {
        const GSErrCode err = ACAPI_Analytical_GetCurrentAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
        if (err != NoError) {
            return CreateFailedExecutionResult (err, "Failed to get the Structural Analytical Model of the current view.");
        }
    }

    const GSErrCode err = ACAPI_CallUndoableCommand ("Update Analytical Model", [&] () -> GSErrCode {
        return ACAPI_Analytical_UpdateAnalyticalModel (ACAPI_GetToken (), analyticalModelGuid);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to update the Structural Analytical Model.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_VARIABLE (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

SetAnalyticalGenerationSettingsToNoRuleCommand::SetAnalyticalGenerationSettingsToNoRuleCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalGenerationSettingsToNoRuleCommand::GetName () const
{
    return "SetAnalyticalGenerationSettingsToNoRule";
}

GS::Optional<GS::UniString> SetAnalyticalGenerationSettingsToNoRuleCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetAnalyticalGenerationSettingsToNoRuleCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    const GSErrCode err = ACAPI_CallUndoableCommand ("Set Analytical Generation Settings To No Rule", [&] () -> GSErrCode {
        return ACAPI_Analytical_SetGenerationSettingsToNoRule (ACAPI_GetToken ());
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the Structural Analytical Model generation settings to no rule.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetAnalyticalModelCheckTypeIdCommand::GetAnalyticalModelCheckTypeIdCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelCheckTypeIdCommand::GetName () const
{
    return "GetAnalyticalModelCheckTypeId";
}

GS::Optional<GS::UniString> GetAnalyticalModelCheckTypeIdCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "modelCheckTypeId": {
                "$ref": "#/GuidId",
                "description": "The type identifier of the Structural Analytical Model check."
            }
        },
        "additionalProperties": false,
        "required": [
            "modelCheckTypeId"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelCheckTypeIdCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    API_Guid typeId = APINULLGuid;
    const GSErrCode err = ACAPI_Analytical_GetModelCheckTypeId (ACAPI_GetToken (), typeId);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model check type identifier.");
    }

    return GS::ObjectState ("modelCheckTypeId", CreateGuidObjectState (typeId));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetAnalyticalModelVariationCommand::GetAnalyticalModelVariationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelVariationCommand::GetName () const
{
    return "GetAnalyticalModelVariation";
}

GS::Optional<GS::UniString> GetAnalyticalModelVariationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; before Archicad 27 it is required."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetAnalyticalModelVariationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "variation": {
                "$ref": "#/AnalyticalModelVariation"
            }
        },
        "additionalProperties": false,
        "required": [
            "variation"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelVariationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    API_AnalyticalModelVariation variation = {};
    const GSErrCode err = TAPIR_GetAnalyticalModelVariation (analyticalModelGuid, variation);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the variation of the Structural Analytical Model.");
    }

    return GS::ObjectState ("variation", CreateAnalyticalModelVariationObjectState (variation));
}

GetAnalyticalModelCommand::GetAnalyticalModelCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalModelCommand::GetName () const
{
    return "GetAnalyticalModel";
}

GS::Optional<GS::UniString> GetAnalyticalModelCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "variation": {
                "$ref": "#/AnalyticalModelVariation"
            }
        },
        "additionalProperties": false,
        "required": [
            "variation"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalModelCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model of the variation."
            }
        },
        "additionalProperties": false,
        "required": [
            "analyticalModelId"
        ]
    })";
}

GS::ObjectState GetAnalyticalModelCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    const GS::ObjectState* variationOS = parameters.Get ("variation");
    if (variationOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "variation is missing.");
    }

    API_AnalyticalModelVariation variation = {};
    const GS::Optional<GS::ObjectState> variationError = GetAnalyticalModelVariationFromObjectState (*variationOS, variation);
    if (variationError.HasValue ()) {
        return *variationError;
    }

    API_Guid analyticalModelGuid = APINULLGuid;
    const GSErrCode err = TAPIR_GetAnalyticalModel (variation, analyticalModelGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model of the variation.");
    }

    return GS::ObjectState ("analyticalModelId", CreateGuidObjectState (analyticalModelGuid));
}

GetAnalyticalVisibilitySettingsCommand::GetAnalyticalVisibilitySettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalVisibilitySettingsCommand::GetName () const
{
    return "GetAnalyticalVisibilitySettings";
}

GS::Optional<GS::UniString> GetAnalyticalVisibilitySettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "visibilitySettings": {
                "$ref": "#/AnalyticalVisibilitySettings",
                "description": "The Structural Analytical Model visibility settings, with every field."
            }
        },
        "additionalProperties": false,
        "required": [
            "visibilitySettings"
        ]
    })";
}

GS::ObjectState GetAnalyticalVisibilitySettingsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    API_AM_VisibilitySettings settings;
    const GSErrCode err = TAPIR_GetAnalyticalVisibilitySettings (settings);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the Structural Analytical Model visibility settings.");
    }

    return GS::ObjectState ("visibilitySettings", CreateAnalyticalVisibilitySettingsObjectState (settings));
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

SetAnalyticalVisibilitySettingsCommand::SetAnalyticalVisibilitySettingsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalVisibilitySettingsCommand::GetName () const
{
    return "SetAnalyticalVisibilitySettings";
}

GS::Optional<GS::UniString> SetAnalyticalVisibilitySettingsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "visibilitySettings": {
                "$ref": "#/AnalyticalVisibilitySettings",
                "description": "The fields to change. The fields left out keep their current value."
            }
        },
        "additionalProperties": false,
        "required": [
            "visibilitySettings"
        ]
    })";
}

GS::Optional<GS::UniString> SetAnalyticalVisibilitySettingsCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetAnalyticalVisibilitySettingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    const GS::ObjectState* settingsOS = parameters.Get ("visibilitySettings");
    if (settingsOS == nullptr) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "visibilitySettings is missing.");
    }

    API_AM_VisibilitySettings settings;
    GSErrCode err = TAPIR_GetAnalyticalVisibilitySettings (settings);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to get the Structural Analytical Model visibility settings.");
    }

    GS::UniString visibilityMode;
    if (settingsOS->Get ("visibilityMode", visibilityMode) && !EnumFromString (AnalyticalVisibilityModeNames, visibilityMode, settings.visibilityMode)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Unknown visibilityMode '%T'.", visibilityMode.ToPrintf ()));
    }
    for (const VisibilityFlag& flag : AnalyticalVisibilityFlags) {
        settingsOS->Get (flag.name, settings.*(flag.field));
    }
    GS::UniString loadVisibilityMode;
    if (settingsOS->Get ("loadVisibilityMode", loadVisibilityMode) && !EnumFromString (AnalyticalLoadVisibilityModeNames, loadVisibilityMode, settings.loadVisibilityMode)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("Unknown loadVisibilityMode '%T'.", loadVisibilityMode.ToPrintf ()));
    }

    err = ACAPI_CallUndoableCommand ("Set Analytical Visibility Settings", [&] () -> GSErrCode {
        return TAPIR_SetAnalyticalVisibilitySettings (settings);
    });
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to set the Structural Analytical Model visibility settings.");
    }

    return CreateSuccessfulExecutionResult ();
#else
    UNUSED_VARIABLE (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

GetAnalyticalMemberTypesOfElementsCommand::GetAnalyticalMemberTypesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalMemberTypesOfElementsCommand::GetName () const
{
    return "GetAnalyticalMemberTypesOfElements";
}

GS::Optional<GS::UniString> GetAnalyticalMemberTypesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalMemberTypesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "memberTypesOfElements": {
                "type": "array",
                "description": "The analytical member type of each element, in the order of the input. An element the type cannot be got for is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "memberType": {
                                    "$ref": "#/AnalyticalMemberType"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "memberType"
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
            "memberTypesOfElements"
        ]
    })";
}

GS::ObjectState GetAnalyticalMemberTypesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& memberTypesOfElements = response.AddList<GS::ObjectState> ("memberTypesOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            memberTypesOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_AnalyticalMemberType memberType = {};
        const GSErrCode err = TAPIR_GetComplementaryProjectMemberType (GetGuidFromObjectState (*elementId), memberType);
        if (err != NoError) {
            memberTypesOfElements (CreateErrorResponse (err, "Failed to get the analytical member type of the element."));
            continue;
        }

        const char* memberTypeName = EnumToString (AnalyticalMemberTypeNames, memberType);
        memberTypesOfElements (GS::ObjectState ("memberType", GS::UniString (memberTypeName != nullptr ? memberTypeName : "Unknown")));
    }

    return response;
}

GetAnalyticalMembersCommand::GetAnalyticalMembersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalMembersCommand::GetName () const
{
    return "GetAnalyticalMembers";
}

GS::Optional<GS::UniString> GetAnalyticalMembersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; before Archicad 27 it is required."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetAnalyticalMembersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "curveMembers": {
                "$ref": "#/Elements",
                "description": "The elements of the curve members (beams, columns) of the model."
            },
            "surfaceMembers": {
                "$ref": "#/Elements",
                "description": "The elements of the surface members (walls, slabs, roofs) of the model."
            }
        },
        "additionalProperties": false,
        "required": [
            "curveMembers",
            "surfaceMembers"
        ]
    })";
}

GS::ObjectState GetAnalyticalMembersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    GS::Array<API_Guid> curveElements;
    GSErrCode err = TAPIR_GetCurveElements (analyticalModelGuid, curveElements);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the curve members of the Structural Analytical Model.");
    }

    GS::Array<API_Guid> surfaceElements;
    err = TAPIR_GetSurfaceElements (analyticalModelGuid, surfaceElements);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the surface members of the Structural Analytical Model.");
    }

    GS::ObjectState response;
    const auto& curveMembers = response.AddList<GS::ObjectState> ("curveMembers");
    for (const API_Guid& elemGuid : curveElements) {
        curveMembers (CreateElementIdObjectState (elemGuid));
    }
    const auto& surfaceMembers = response.AddList<GS::ObjectState> ("surfaceMembers");
    for (const API_Guid& elemGuid : surfaceElements) {
        surfaceMembers (CreateElementIdObjectState (elemGuid));
    }
    return response;
}

GetAnalyticalMemberDetailsCommand::GetAnalyticalMemberDetailsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalMemberDetailsCommand::GetName () const
{
    return "GetAnalyticalMemberDetails";
}

GS::Optional<GS::UniString> GetAnalyticalMemberDetailsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; before Archicad 27 it is required."
            },
            "elements": {
                "$ref": "#/Elements"
            },
            "segmentationCentralAngle": {
                "type": "number",
                "description": "When given, surface members are read as segmented surface members (for curved walls), each segment spanning at most this central angle, in radians."
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalMemberDetailsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalMembers": {
                "type": "array",
                "description": "The analytical member of each element, in the order of the input. An element that is no member of the model, or whose member cannot be read, is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "curveMember": {
                                    "$ref": "#/AnalyticalCurveMember"
                                },
                                "connections": {
                                    "$ref": "#/AnalyticalMemberConnections"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "curveMember",
                                "connections"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "surfaceMember": {
                                    "$ref": "#/AnalyticalSurfaceMember"
                                },
                                "idMapping": {
                                    "$ref": "#/AnalyticalExportIdMapping"
                                },
                                "connections": {
                                    "$ref": "#/AnalyticalMemberConnections"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "surfaceMember",
                                "connections"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "segmentedSurfaceMembers": {
                                    "type": "array",
                                    "description": "The segments of the surface member.",
                                    "items": {
                                        "$ref": "#/AnalyticalSurfaceMember"
                                    }
                                },
                                "idMapping": {
                                    "$ref": "#/AnalyticalExportIdMapping"
                                },
                                "connections": {
                                    "$ref": "#/AnalyticalMemberConnections"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "segmentedSurfaceMembers",
                                "connections"
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
            "analyticalMembers"
        ]
    })";
}

GS::ObjectState GetAnalyticalMemberDetailsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    double centralAngle = 0.0;
    const bool segmented = parameters.Get ("segmentationCentralAngle", centralAngle);

    GS::Array<API_Guid> curveElements;
    GSErrCode err = TAPIR_GetCurveElements (analyticalModelGuid, curveElements);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the curve members of the Structural Analytical Model.");
    }
    GS::Array<API_Guid> surfaceElements;
    err = TAPIR_GetSurfaceElements (analyticalModelGuid, surfaceElements);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the surface members of the Structural Analytical Model.");
    }
    const GS::HashSet<API_Guid> curveElementSet = CreateGuidSet (curveElements);
    const GS::HashSet<API_Guid> surfaceElementSet = CreateGuidSet (surfaceElements);

    GS::ObjectState response;
    const auto& analyticalMembers = response.AddList<GS::ObjectState> ("analyticalMembers");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);
        if (elemGuid == APINULLGuid) {
            analyticalMembers (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        GS::ObjectState memberOS;
        if (curveElementSet.Contains (elemGuid)) {
            API_AnalyticalCurveMember curveMember;
            err = TAPIR_GetCurveMember (analyticalModelGuid, elemGuid, curveMember);
            if (err != NoError) {
                analyticalMembers (CreateErrorResponse (err, "Failed to get the curve member of the element."));
                continue;
            }
            memberOS.Add ("curveMember", CreateAnalyticalCurveMemberObjectState (curveMember));
        } else if (surfaceElementSet.Contains (elemGuid)) {
            API_ExportIdMappingType idMapping;
            if (segmented) {
                GS::Array<API_AnalyticalSurfaceMember> surfaceMembers;
                err = TAPIR_GetSegmentedSurfaceMember (analyticalModelGuid, elemGuid, surfaceMembers, centralAngle, idMapping);
                if (err != NoError) {
                    analyticalMembers (CreateErrorResponse (err, "Failed to get the segmented surface member of the element."));
                    continue;
                }
                const auto& segmentsOS = memberOS.AddList<GS::ObjectState> ("segmentedSurfaceMembers");
                for (const API_AnalyticalSurfaceMember& surfaceMember : surfaceMembers) {
                    segmentsOS (CreateAnalyticalSurfaceMemberObjectState (surfaceMember));
                }
            } else {
                API_AnalyticalSurfaceMember surfaceMember;
                err = TAPIR_GetSurfaceMember (analyticalModelGuid, elemGuid, surfaceMember, idMapping);
                if (err != NoError) {
                    analyticalMembers (CreateErrorResponse (err, "Failed to get the surface member of the element."));
                    continue;
                }
                memberOS.Add ("surfaceMember", CreateAnalyticalSurfaceMemberObjectState (surfaceMember));
            }
#ifdef ServerMainVers_2600
            memberOS.Add ("idMapping", CreateExportIdMappingObjectState (idMapping));
#endif
        } else {
            analyticalMembers (CreateErrorResponse (APIERR_BADID, "The element is not a member of the Structural Analytical Model."));
            continue;
        }

        GS::HashSet<API_Guid> connections;
        err = TAPIR_GetAnalyticalMemberConnections (analyticalModelGuid, elemGuid, connections);
        if (err != NoError) {
            analyticalMembers (CreateErrorResponse (err, "Failed to get the connections of the analytical member."));
            continue;
        }
        const auto& connectionsOS = memberOS.AddList<GS::ObjectState> ("connections");
        for (const API_Guid& connection : connections) {
            connectionsOS (CreateGuidObjectState (connection));
        }

        analyticalMembers (memberOS);
    }

    return response;
}

GetAnalyticalReleasesCommand::GetAnalyticalReleasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalReleasesCommand::GetName () const
{
    return "GetAnalyticalReleases";
}

GS::Optional<GS::UniString> GetAnalyticalReleasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; in Archicad 26 it is required."
            },
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalReleasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "releases": {
                "type": "array",
                "description": "The release of each element, in the order of the input. An element the release cannot be got for is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "release": {
                                    "$ref": "#/AnalyticalRelease"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "release"
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
            "releases"
        ]
    })";
}

GS::ObjectState GetAnalyticalReleasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& releases = response.AddList<GS::ObjectState> ("releases");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);
        if (elemGuid == APINULLGuid) {
            releases (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_AnalyticalReleaseType release;
        const GSErrCode err = TAPIR_GetAnalyticalRelease (analyticalModelGuid, elemGuid, release);
        if (err != NoError) {
            releases (CreateErrorResponse (err, "Failed to get the release of the element."));
            continue;
        }
        releases (GS::ObjectState ("release", CreateAnalyticalReleaseObjectState (release)));
    }

    return response;
#else
    UNUSED_VARIABLE (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

SetAnalyticalReleasesCommand::SetAnalyticalReleasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetAnalyticalReleasesCommand::GetName () const
{
    return "SetAnalyticalReleases";
}

GS::Optional<GS::UniString> SetAnalyticalReleasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "releases": {
                "type": "array",
                "description": "The element and its new release, as GetAnalyticalReleases returns it. The release replaces the element's release as a whole.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "release": {
                            "$ref": "#/AnalyticalRelease"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "release"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "releases"
        ]
    })";
}

GS::Optional<GS::UniString> SetAnalyticalReleasesCommand::GetRawResponseSchema () const
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

GS::ObjectState SetAnalyticalReleasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> releases;
    parameters.Get ("releases", releases);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

#ifdef ServerMainVers_2600
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("Set Analytical Releases", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : releases) {
            const API_Guid elemGuid = GetGuidFromElementsArrayItem (item);
            if (elemGuid == APINULLGuid) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }
            const GS::ObjectState* releaseOS = item.Get ("release");
            if (releaseOS == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "release is missing"));
                continue;
            }

            API_AnalyticalReleaseType release;
            const GS::Optional<GS::ObjectState> releaseError = GetAnalyticalReleaseFromObjectState (*releaseOS, release);
            if (releaseError.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*releaseError));
                continue;
            }

            const GSErrCode err = TAPIR_SetAnalyticalRelease (elemGuid, release);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to set the release of the element."));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });
    if (undoErr != NoError) {
        return CreateErrorResponse (undoErr, "Failed to set the releases.");
    }
#else
    for (UIndex i = 0; i < releases.GetSize (); ++i) {
        executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer."));
    }
#endif

    return response;
}

GetCustomCurveMemberStretchCommand::GetCustomCurveMemberStretchCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCustomCurveMemberStretchCommand::GetName () const
{
    return "GetCustomCurveMemberStretch";
}

GS::Optional<GS::UniString> GetCustomCurveMemberStretchCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> GetCustomCurveMemberStretchCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "stretches": {
                "type": "array",
                "description": "The custom stretch of the curve member of each element, in the order of the input. An element the stretch cannot be got for is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "begAdditionalLength": {
                                    "type": "number",
                                    "description": "The length added to the curve member at its start."
                                },
                                "endAdditionalLength": {
                                    "type": "number",
                                    "description": "The length added to the curve member at its end."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "begAdditionalLength",
                                "endAdditionalLength"
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
            "stretches"
        ]
    })";
}

GS::ObjectState GetCustomCurveMemberStretchCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& stretches = response.AddList<GS::ObjectState> ("stretches");

    for (const GS::ObjectState& element : elements) {
        const API_Guid elemGuid = GetGuidFromElementsArrayItem (element);
        if (elemGuid == APINULLGuid) {
            stretches (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        double begAdditionalLength = 0.0;
        double endAdditionalLength = 0.0;
        const GSErrCode err = ACAPI_Analytical_GetCustomCurveMemberStretchValues (ACAPI_GetToken (), elemGuid, begAdditionalLength, endAdditionalLength);
        if (err != NoError) {
            stretches (CreateErrorResponse (err, "Failed to get the custom stretch of the curve member."));
            continue;
        }
        stretches (GS::ObjectState (
            "begAdditionalLength", begAdditionalLength,
            "endAdditionalLength", endAdditionalLength));
    }

    return response;
#else
    UNUSED_VARIABLE (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

SetCustomCurveMemberStretchCommand::SetCustomCurveMemberStretchCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetCustomCurveMemberStretchCommand::GetName () const
{
    return "SetCustomCurveMemberStretch";
}

GS::Optional<GS::UniString> SetCustomCurveMemberStretchCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "stretches": {
                "type": "array",
                "description": "The element of the curve member and the lengths to add to it.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "begAdditionalLength": {
                            "type": "number",
                            "description": "The length added to the curve member at its start."
                        },
                        "endAdditionalLength": {
                            "type": "number",
                            "description": "The length added to the curve member at its end."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "begAdditionalLength",
                        "endAdditionalLength"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "stretches"
        ]
    })";
}

GS::Optional<GS::UniString> SetCustomCurveMemberStretchCommand::GetRawResponseSchema () const
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

GS::ObjectState SetCustomCurveMemberStretchCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> stretches;
    parameters.Get ("stretches", stretches);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

#ifdef ServerMainVers_2900
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("Set Custom Curve Member Stretch", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : stretches) {
            const API_Guid elemGuid = GetGuidFromElementsArrayItem (item);
            double begAdditionalLength = 0.0;
            double endAdditionalLength = 0.0;
            if (elemGuid == APINULLGuid || !item.Get ("begAdditionalLength", begAdditionalLength) || !item.Get ("endAdditionalLength", endAdditionalLength)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId, begAdditionalLength and endAdditionalLength are required"));
                continue;
            }

            const GSErrCode err = ACAPI_Analytical_SetCustomCurveMember (ACAPI_GetToken (), elemGuid, begAdditionalLength, endAdditionalLength);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to set the custom stretch of the curve member."));
                continue;
            }
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });
    if (undoErr != NoError) {
        return CreateErrorResponse (undoErr, "Failed to set the custom stretches.");
    }
#else
    for (UIndex i = 0; i < stretches.GetSize (); ++i) {
        executionResults (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer."));
    }
#endif

    return response;
}

GetLCSOfAnalyticalMembersCommand::GetLCSOfAnalyticalMembersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLCSOfAnalyticalMembersCommand::GetName () const
{
    return "GetLCSOfAnalyticalMembers";
}

GS::Optional<GS::UniString> GetLCSOfAnalyticalMembersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "memberParts": {
                "type": "array",
                "description": "The member parts to get the local coordinate system of.",
                "items": {
                    "type": "object",
                    "properties": {
                        "memberPartRef": {
                            "$ref": "#/AnalyticalMemberPartRef"
                        },
                        "intersectingMemberPartRef": {
                            "$ref": "#/AnalyticalMemberPartRef",
                            "description": "When given, the local coordinate system of memberPartRef in its intersection with this member part is returned."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "memberPartRef"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "memberParts"
        ]
    })";
}

GS::Optional<GS::UniString> GetLCSOfAnalyticalMembersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "localCoordinateSystems": {
                "type": "array",
                "description": "The local coordinate system of each member part, in the order of the input. A member part the coordinate system cannot be got for is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "localCoordinateSystem": {
                                    "$ref": "#/AnalyticalCoordinateSystem"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "localCoordinateSystem"
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
            "localCoordinateSystems"
        ]
    })";
}

GS::ObjectState GetLCSOfAnalyticalMembersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> memberParts;
    parameters.Get ("memberParts", memberParts);

    GS::ObjectState response;
    const auto& localCoordinateSystems = response.AddList<GS::ObjectState> ("localCoordinateSystems");

    for (const GS::ObjectState& memberPart : memberParts) {
        const GS::ObjectState* memberPartRefOS = memberPart.Get ("memberPartRef");
        if (memberPartRefOS == nullptr) {
            localCoordinateSystems (CreateErrorResponse (APIERR_BADPARS, "memberPartRef is missing"));
            continue;
        }
        API_AnalyticalMemberPartRef memberPartRef;
        GS::Optional<GS::ObjectState> partRefError = GetAnalyticalMemberPartRefFromObjectState (*memberPartRefOS, memberPartRef);
        if (partRefError.HasValue ()) {
            localCoordinateSystems (*partRefError);
            continue;
        }

        API_CoordinateSystem coordinateSystem;
        GSErrCode err = NoError;
        const GS::ObjectState* intersectingMemberPartRefOS = memberPart.Get ("intersectingMemberPartRef");
        if (intersectingMemberPartRefOS != nullptr) {
            API_AnalyticalMemberPartRef intersectingMemberPartRef;
            partRefError = GetAnalyticalMemberPartRefFromObjectState (*intersectingMemberPartRefOS, intersectingMemberPartRef);
            if (partRefError.HasValue ()) {
                localCoordinateSystems (*partRefError);
                continue;
            }
            err = TAPIR_GetLCSOfAnalyticalMemberInIntersection (memberPartRef, intersectingMemberPartRef, coordinateSystem);
        } else {
            err = TAPIR_GetLCSOfAnalyticalMember (memberPartRef, coordinateSystem);
        }
        if (err != NoError) {
            localCoordinateSystems (CreateErrorResponse (err, "Failed to get the local coordinate system of the member part."));
            continue;
        }
        localCoordinateSystems (GS::ObjectState ("localCoordinateSystem", CreateCoordinateSystemObjectState (coordinateSystem)));
    }

    return response;
#else
    UNUSED_VARIABLE (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

GetAnalyticalMemberPartGeometriesCommand::GetAnalyticalMemberPartGeometriesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalMemberPartGeometriesCommand::GetName () const
{
    return "GetAnalyticalMemberPartGeometries";
}

GS::Optional<GS::UniString> GetAnalyticalMemberPartGeometriesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "analyticalModelId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the Structural Analytical Model. The model of the current view is used when it is omitted; in Archicad 26 it is required."
            },
            "memberPartRefs": {
                "type": "array",
                "description": "The member parts to get the geometry of: vertices (BegVertex, EndVertex, CustomVertex) give a node, edges (Edge, RatioEdge, CustomEdge) a curve.",
                "items": {
                    "$ref": "#/AnalyticalMemberPartRef"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "memberPartRefs"
        ]
    })";
}

GS::Optional<GS::UniString> GetAnalyticalMemberPartGeometriesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "geometries": {
                "type": "array",
                "description": "The node or curve of each member part, in the order of the input. A member part the geometry cannot be got for, a surface among them, is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "node": {
                                    "$ref": "#/AnalyticalNode"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "node"
                            ]
                        },
                        {
                            "type": "object",
                            "properties": {
                                "curve": {
                                    "$ref": "#/AnalyticalCurve"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "curve"
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
            "geometries"
        ]
    })";
}

GS::ObjectState GetAnalyticalMemberPartGeometriesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    API_Guid analyticalModelGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> modelError = GetAnalyticalModelGuidFromParameters (parameters, analyticalModelGuid);
    if (modelError.HasValue ()) {
        return *modelError;
    }

    GS::Array<GS::ObjectState> memberPartRefs;
    parameters.Get ("memberPartRefs", memberPartRefs);

    GS::ObjectState response;
    const auto& geometries = response.AddList<GS::ObjectState> ("geometries");

    for (const GS::ObjectState& memberPartRefOS : memberPartRefs) {
        API_AnalyticalMemberPartRef memberPartRef;
        const GS::Optional<GS::ObjectState> partRefError = GetAnalyticalMemberPartRefFromObjectState (memberPartRefOS, memberPartRef);
        if (partRefError.HasValue ()) {
            geometries (*partRefError);
            continue;
        }

        switch (memberPartRef.partRefType) {
            case API_AMPartRef_BegVertex:
            case API_AMPartRef_EndVertex:
            case API_AMPartRef_CustomVertex: {
                API_AnalyticalNode node;
                const GSErrCode err = TAPIR_GetAnalyticalMemberPartRefCoord (analyticalModelGuid, memberPartRef, node);
                if (err != NoError) {
                    geometries (CreateErrorResponse (err, "Failed to get the node of the member part."));
                } else {
                    geometries (GS::ObjectState ("node", CreateAnalyticalNodeObjectState (node)));
                }
                break;
            }
            case API_AMPartRef_Edge:
            case API_AMPartRef_RatioEdge:
            case API_AMPartRef_CustomEdge: {
                API_AnalyticalCurve curve;
                const GSErrCode err = TAPIR_GetAnalyticalMemberPartRefCurve (analyticalModelGuid, memberPartRef, curve);
                if (err != NoError) {
                    geometries (CreateErrorResponse (err, "Failed to get the curve of the member part."));
                } else {
                    geometries (GS::ObjectState ("curve", CreateAnalyticalCurveObjectState (curve)));
                }
                break;
            }
            default:
                geometries (CreateErrorResponse (APIERR_BADPARS, "Only vertices and edges have a node or a curve."));
                break;
        }
    }

    return response;
#else
    UNUSED_VARIABLE (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 26 or newer.");
#endif
}

CreateElementsFromAnalyticalMembersCommand::CreateElementsFromAnalyticalMembersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateElementsFromAnalyticalMembersCommand::GetName () const
{
    return "CreateElementsFromAnalyticalMembers";
}

GS::Optional<GS::UniString> CreateElementsFromAnalyticalMembersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "members": {
                "type": "array",
                "description": "The analytical members to create elements from, in the form GetAnalyticalMemberDetails returns them, so a member read there can be passed on as it is. The elemGuid of a member is ignored. A curve member needs analyticalMemberType and curve, a surface member (or segment) analyticalMemberType and contour; the other fields are zero when left out.",
                "items": {
                    "type": "object",
                    "properties": {
                        "curveMember": {
                            "$ref": "#/AnalyticalCurveMember",
                            "description": "A curve member, creating a beam or a column."
                        },
                        "surfaceMember": {
                            "$ref": "#/AnalyticalSurfaceMember",
                            "description": "A surface member, creating a wall, a slab or a roof."
                        },
                        "segmentedSurfaceMembers": {
                            "type": "array",
                            "description": "The segments of a segmented surface member (a curved wall), creating one element.",
                            "items": {
                                "$ref": "#/AnalyticalSurfaceMember"
                            },
                            "minItems": 1
                        },
                        "favoriteName": {
                            "type": "string",
                            "description": "The name of the favorite whose settings the new element gets."
                        }
                    },
                    "additionalProperties": false,
                    "oneOf": [
                        {
                            "required": [
                                "curveMember"
                            ]
                        },
                        {
                            "required": [
                                "surfaceMember"
                            ]
                        },
                        {
                            "required": [
                                "segmentedSurfaceMembers"
                            ]
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "members"
        ]
    })";
}

GS::Optional<GS::UniString> CreateElementsFromAnalyticalMembersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The new element of each member, in the order of the input. A member no element could be created from is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId"
                                },
                                "idMapping": {
                                    "$ref": "#/AnalyticalImportIdMapping"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId"
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
            "elements"
        ]
    })";
}

GS::ObjectState CreateElementsFromAnalyticalMembersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> members;
    parameters.Get ("members", members);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("Create Elements From Analytical Members", [&] () -> GSErrCode {
        for (const GS::ObjectState& member : members) {
            elements (CreateElementFromAnalyticalMember (member));
        }
        return NoError;
    });
    if (undoErr != NoError) {
        return CreateErrorResponse (undoErr, "Failed to create the elements from the analytical members.");
    }

    return response;
}

namespace {

// The load enums are plain C enums with the same enumerators in Archicad 25 to 30; the JSON uses the
// enumerator names without their API_AnalyticalLoad*_ prefix.
const EnumName<API_AnalyticalLoadGroupType> AnalyticalLoadGroupTypeNames[] = {
    { API_AnalyticalLoadGroup_PermanentLoad,  "PermanentLoad" },
    { API_AnalyticalLoadGroup_VariableLoad,   "VariableLoad" },
    { API_AnalyticalLoadGroup_AccidentalLoad, "AccidentalLoad" },
    { API_AnalyticalLoadGroup_SeismicLoad,    "SeismicLoad" },
    { API_AnalyticalLoadGroup_MovingLoad,     "MovingLoad" },
    { API_AnalyticalLoadGroup_TensioningLoad, "TensioningLoad" },
    { API_AnalyticalLoadGroup_FireLoad,       "FireLoad" }
};

const EnumName<API_AnalyticalLoadGroupRelation> AnalyticalLoadGroupRelationNames[] = {
    { API_AnalyticalLoadGroup_ExclusiveRelation, "ExclusiveRelation" },
    { API_AnalyticalLoadGroup_StandardRelation,  "StandardRelation" },
    { API_AnalyticalLoadGroup_TogetherRelation,  "TogetherRelation" }
};

const EnumName<API_AnalyticalLoadCaseActionType> AnalyticalLoadCaseActionTypeNames[] = {
    { API_AnalyticalLoadCase_PermanentAction,  "PermanentAction" },
    { API_AnalyticalLoadCase_VariableAction,   "VariableAction" },
    { API_AnalyticalLoadCase_AccidentalAction, "AccidentalAction" }
};

const EnumName<API_AnalyticalLoadCaseLoadType> AnalyticalLoadCaseLoadTypeNames[] = {
    { API_AnalyticalLoadCase_SelfWeightLoad,  "SelfWeightLoad" },
    { API_AnalyticalLoadCase_PrestressLoad,   "PrestressLoad" },
    { API_AnalyticalLoadCase_DynamicLoad,     "DynamicLoad" },
    { API_AnalyticalLoadCase_StaticLoad,      "StaticLoad" },
    { API_AnalyticalLoadCase_TemperatureLoad, "TemperatureLoad" },
    { API_AnalyticalLoadCase_WindLoad,        "WindLoad" },
    { API_AnalyticalLoadCase_SnowLoad,        "SnowLoad" },
    { API_AnalyticalLoadCase_MaintenanceLoad, "MaintenanceLoad" },
    { API_AnalyticalLoadCase_FireLoad,        "FireLoad" },
    { API_AnalyticalLoadCase_MovingLoad,      "MovingLoad" },
    { API_AnalyticalLoadCase_SeismicLoad,     "SeismicLoad" },
    { API_AnalyticalLoadCase_StandardLoad,    "StandardLoad" },
    { API_AnalyticalLoadCase_OthersLoad,      "OthersLoad" }
};

const EnumName<API_AnalyticalLoadCaseDuration> AnalyticalLoadCaseDurationNames[] = {
    { API_AnalyticalLoadCase_LongDuration,          "LongDuration" },
    { API_AnalyticalLoadCase_MediumDuration,        "MediumDuration" },
    { API_AnalyticalLoadCase_ShortDuration,         "ShortDuration" },
    { API_AnalyticalLoadCase_InstantaneousDuration, "InstantaneousDuration" }
};

const EnumName<API_AnalyticalLoadCombinationCategory> AnalyticalLoadCombinationCategoryNames[] = {
    { API_AnalyticalLoadCombination_ULS,        "ULS" },
    { API_AnalyticalLoadCombination_SLS,        "SLS" },
    { API_AnalyticalLoadCombination_ALS,        "ALS" },
    { API_AnalyticalLoadCombination_ANS,        "ANS" },
    { API_AnalyticalLoadCombination_NotDefined, "NotDefined" }
};

const EnumName<API_AnalyticalLoadCombinationNationalStandard> AnalyticalLoadCombinationNationalStandardNames[] = {
    { API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetB,       "ENULSSTRGEOSetB" },
    { API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetC,       "ENULSSTRGEOSetC" },
    { API_AnalyticalLoadNationalStandard_ENAccidental1,         "ENAccidental1" },
    { API_AnalyticalLoadNationalStandard_ENAccidental2,         "ENAccidental2" },
    { API_AnalyticalLoadNationalStandard_ENSeismic,             "ENSeismic" },
    { API_AnalyticalLoadNationalStandard_ENSLSCharacteristic,   "ENSLSCharacteristic" },
    { API_AnalyticalLoadNationalStandard_ENSLSFrequent,         "ENSLSFrequent" },
    { API_AnalyticalLoadNationalStandard_ENSLSQuasipermanent,   "ENSLSQuasipermanent" },
    { API_AnalyticalLoadNationalStandard_IBCLRFDultimate,       "IBCLRFDultimate" },
    { API_AnalyticalLoadNationalStandard_IBCASDultimate,        "IBCASDultimate" },
    { API_AnalyticalLoadNationalStandard_IBCASDserviceability,  "IBCASDserviceability" },
    { API_AnalyticalLoadNationalStandard_IBCASDseismic,         "IBCASDseismic" },
    { API_AnalyticalLoadNationalStandard_IBCLRFDseismic,        "IBCLRFDseismic" }
};

#ifdef ServerMainVers_2700
const EnumName<API_LoadCombinationType> AnalyticalLoadCombinationTypeNames[] = {
    { API_LoadCombinationType::API_LoadCombinationType_Envelope,  "Envelope" },
    { API_LoadCombinationType::API_LoadCombinationType_Linear,    "Linear" },
    { API_LoadCombinationType::API_LoadCombinationType_Nonlinear, "Nonlinear" }
};
#endif

GSErrCode TAPIR_GetAnalyticalLoadGroups (GS::Array<API_Guid>& loadGroups)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadGroups (ACAPI_GetToken (), loadGroups);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroups (ACAPI_GetToken (), loadGroups);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroups (&loadGroups);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadGroup (const API_Guid& loadGroupGuid, API_AnalyticalLoadGroup& loadGroup)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid, loadGroup);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid, loadGroup);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadGroup (loadGroupGuid, &loadGroup);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadGroup (API_AnalyticalLoadGroup& loadGroup)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadGroup (ACAPI_GetToken (), loadGroup);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadGroup (ACAPI_GetToken (), loadGroup);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadGroup (loadGroup);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadGroup (const API_Guid& loadGroupGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadGroup (ACAPI_GetToken (), loadGroupGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadGroup (loadGroupGuid);
#endif
}

GSErrCode TAPIR_GetAllAnalyticalLoadCases (GS::Array<API_Guid>& loadCases)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAllAnalyticalLoadCases (ACAPI_GetToken (), loadCases);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAllAnalyticalLoadCases (ACAPI_GetToken (), loadCases);
#else
    return ACAPI_AnalyticalModel_GetAllAnalyticalLoadCases (&loadCases);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCases (const API_Guid& loadGroupGuid, GS::Array<API_Guid>& loadCases)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCases (ACAPI_GetToken (), loadGroupGuid, loadCases);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCases (ACAPI_GetToken (), loadGroupGuid, loadCases);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCases (loadGroupGuid, &loadCases);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCase (const API_Guid& loadCaseGuid, API_AnalyticalLoadCase& loadCase)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid, loadCase);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid, loadCase);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCase (loadCaseGuid, &loadCase);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadCase (API_AnalyticalLoadCase& loadCase)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadCase (ACAPI_GetToken (), loadCase);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCase (ACAPI_GetToken (), loadCase);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCase (loadCase);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadCase (const API_Guid& loadCaseGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCase (ACAPI_GetToken (), loadCaseGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCase (loadCaseGuid);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCombinations (GS::Array<API_Guid>& loadCombinations)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCombinations (ACAPI_GetToken (), loadCombinations);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombinations (ACAPI_GetToken (), loadCombinations);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombinations (&loadCombinations);
#endif
}

GSErrCode TAPIR_GetAnalyticalLoadCombination (const API_Guid& loadCombinationGuid, API_AnalyticalLoadCombination& loadCombination)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_GetAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid, loadCombination);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid, loadCombination);
#else
    return ACAPI_AnalyticalModel_GetAnalyticalLoadCombination (loadCombinationGuid, &loadCombination);
#endif
}

GSErrCode TAPIR_CreateAnalyticalLoadCombination (API_AnalyticalLoadCombination& loadCombination)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_CreateAnalyticalLoadCombination (ACAPI_GetToken (), loadCombination);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCombination (ACAPI_GetToken (), loadCombination);
#else
    return ACAPI_AnalyticalModel_CreateAnalyticalLoadCombination (loadCombination);
#endif
}

GSErrCode TAPIR_DeleteAnalyticalLoadCombination (const API_Guid& loadCombinationGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_DeleteAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCombination (ACAPI_GetToken (), loadCombinationGuid);
#else
    return ACAPI_AnalyticalModel_DeleteAnalyticalLoadCombination (loadCombinationGuid);
#endif
}

GSErrCode TAPIR_AddLoadCaseToLoadCombination (const API_AnalyticalLoadFactor& loadFactor, const API_Guid& loadCombinationGuid)
{
#if defined (ServerMainVers_2700)
    return ACAPI_Analytical_AddLoadCaseToLoadCombination (ACAPI_GetToken (), loadFactor, loadCombinationGuid);
#elif defined (ServerMainVers_2600)
    return ACAPI_AnalyticalModel_AddLoadCaseToLoadCombination (ACAPI_GetToken (), loadFactor, loadCombinationGuid);
#else
    return ACAPI_AnalyticalModel_AddLoadCaseToLoadCombination (loadFactor, loadCombinationGuid);
#endif
}

template <typename EnumType, size_t N>
void AddEnumField (GS::ObjectState& os, const char* key, const EnumName<EnumType> (&names)[N], EnumType value)
{
    const char* name = EnumToString (names, value);
    if (name != nullptr) {
        os.Add (key, GS::UniString (name));
    }
}

// Reads a required enum field. Returns an error response when it is missing or unknown.
template <typename EnumType, size_t N>
GS::Optional<GS::ObjectState> GetRequiredEnumField (const GS::ObjectState& os, const char* key, const EnumName<EnumType> (&names)[N], EnumType& value)
{
    GS::UniString name;
    if (!os.Get (key, name)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("%s is missing.", key));
    }
    if (!EnumFromString (names, name, value)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Unknown %s '%T'.", key, name.ToPrintf ()));
    }
    return GS::NoValue;
}

// Reads a guid object field. Returns an error response when it is missing or invalid.
GS::Optional<GS::ObjectState> GetRequiredGuidField (const GS::ObjectState& os, const char* key, API_Guid& guid)
{
    const GS::ObjectState* guidOS = os.Get (key);
    if (guidOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("%s is missing.", key));
    }
    guid = GetGuidFromObjectState (*guidOS);
    if (guid == APINULLGuid) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Invalid %s.", key));
    }
    return GS::NoValue;
}

// Reads the guids of the given id key from a list of objects like [{"loadGroupId": {"guid": ...}}].
// An item without a valid id gets APINULLGuid.
GS::Array<API_Guid> GetGuidsFromIdList (const GS::ObjectState& parameters, const char* listKey, const char* idKey)
{
    GS::Array<GS::ObjectState> items;
    parameters.Get (listKey, items);

    GS::Array<API_Guid> guids;
    for (const GS::ObjectState& item : items) {
        const GS::ObjectState* idOS = item.Get (idKey);
        guids.Push (idOS != nullptr ? GetGuidFromObjectState (*idOS) : APINULLGuid);
    }
    return guids;
}

GS::ObjectState CreateAnalyticalLoadGroupObjectState (const API_AnalyticalLoadGroup& loadGroup)
{
    GS::ObjectState loadGroupOS;
    loadGroupOS.Add ("loadGroupId", CreateGuidObjectState (loadGroup.guid));
    loadGroupOS.Add ("name", loadGroup.name);
    AddEnumField (loadGroupOS, "groupType", AnalyticalLoadGroupTypeNames, loadGroup.groupType);
    AddEnumField (loadGroupOS, "relation", AnalyticalLoadGroupRelationNames, loadGroup.relation);
    loadGroupOS.Add ("loadType", loadGroup.loadType);
    return loadGroupOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadGroupFromObjectState (const GS::ObjectState& loadGroupOS, API_AnalyticalLoadGroup& loadGroup)
{
    if (!loadGroupOS.Get ("name", loadGroup.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadGroupOS, "groupType", AnalyticalLoadGroupTypeNames, loadGroup.groupType);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredEnumField (loadGroupOS, "relation", AnalyticalLoadGroupRelationNames, loadGroup.relation);
    if (error.HasValue ()) {
        return error;
    }
    loadGroupOS.Get ("loadType", loadGroup.loadType);
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadCaseObjectState (const API_AnalyticalLoadCase& loadCase)
{
    GS::ObjectState loadCaseOS;
    loadCaseOS.Add ("loadCaseId", CreateGuidObjectState (loadCase.loadCaseGuid));
    loadCaseOS.Add ("name", loadCase.name);
    loadCaseOS.Add ("description", loadCase.description);
    AddEnumField (loadCaseOS, "actionType", AnalyticalLoadCaseActionTypeNames, loadCase.actionType);
    loadCaseOS.Add ("loadGroupId", CreateGuidObjectState (loadCase.loadGroupGuid));
    AddEnumField (loadCaseOS, "loadType", AnalyticalLoadCaseLoadTypeNames, loadCase.loadType);
    if (loadCase.duration.HasValue ()) {
        AddEnumField (loadCaseOS, "duration", AnalyticalLoadCaseDurationNames, loadCase.duration.Get ());
    }
    return loadCaseOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadCaseFromObjectState (const GS::ObjectState& loadCaseOS, API_AnalyticalLoadCase& loadCase)
{
    if (!loadCaseOS.Get ("name", loadCase.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    loadCaseOS.Get ("description", loadCase.description);
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadCaseOS, "actionType", AnalyticalLoadCaseActionTypeNames, loadCase.actionType);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredGuidField (loadCaseOS, "loadGroupId", loadCase.loadGroupGuid);
    if (error.HasValue ()) {
        return error;
    }
    error = GetRequiredEnumField (loadCaseOS, "loadType", AnalyticalLoadCaseLoadTypeNames, loadCase.loadType);
    if (error.HasValue ()) {
        return error;
    }
    if (loadCaseOS.Contains ("duration")) {
        API_AnalyticalLoadCaseDuration duration = API_AnalyticalLoadCase_LongDuration;
        error = GetRequiredEnumField (loadCaseOS, "duration", AnalyticalLoadCaseDurationNames, duration);
        if (error.HasValue ()) {
            return error;
        }
        loadCase.duration = duration;
    }
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadFactorObjectState (const API_AnalyticalLoadFactor& loadFactor)
{
    GS::ObjectState loadFactorOS;
    loadFactorOS.Add ("loadCaseId", CreateGuidObjectState (loadFactor.loadCaseGuid));
    if (loadFactor.factor.HasValue ()) {
        loadFactorOS.Add ("factor", loadFactor.factor.Get ());
    }
    loadFactorOS.Add ("multiplier", loadFactor.multiplier);
    return loadFactorOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadFactorFromObjectState (const GS::ObjectState& loadFactorOS, API_AnalyticalLoadFactor& loadFactor)
{
    GS::Optional<GS::ObjectState> error = GetRequiredGuidField (loadFactorOS, "loadCaseId", loadFactor.loadCaseGuid);
    if (error.HasValue ()) {
        return error;
    }
    double factor = 0.0;
    if (loadFactorOS.Get ("factor", factor)) {
        loadFactor.factor = factor;
    }
    if (!loadFactorOS.Get ("multiplier", loadFactor.multiplier)) {
        return CreateErrorResponse (APIERR_BADPARS, "multiplier is missing.");
    }
    return GS::NoValue;
}

GS::ObjectState CreateAnalyticalLoadCombinationObjectState (const API_AnalyticalLoadCombination& loadCombination)
{
    GS::ObjectState loadCombinationOS;
    loadCombinationOS.Add ("loadCombinationId", CreateGuidObjectState (loadCombination.guid));
    loadCombinationOS.Add ("name", loadCombination.name);
    loadCombinationOS.Add ("description", loadCombination.description);
    AddEnumField (loadCombinationOS, "category", AnalyticalLoadCombinationCategoryNames, loadCombination.category);
    if (loadCombination.nationalStandard.HasValue ()) {
        AddEnumField (loadCombinationOS, "nationalStandard", AnalyticalLoadCombinationNationalStandardNames, loadCombination.nationalStandard.Get ());
    }
#ifdef ServerMainVers_2700
    if (loadCombination.type.HasValue ()) {
        AddEnumField (loadCombinationOS, "type", AnalyticalLoadCombinationTypeNames, loadCombination.type.Get ());
    }
#endif
    const auto& loadFactors = loadCombinationOS.AddList<GS::ObjectState> ("loadFactors");
    for (const API_AnalyticalLoadFactor& loadFactor : loadCombination.loadFactors) {
        loadFactors (CreateAnalyticalLoadFactorObjectState (loadFactor));
    }
    return loadCombinationOS;
}

GS::Optional<GS::ObjectState> GetAnalyticalLoadCombinationFromObjectState (const GS::ObjectState& loadCombinationOS, API_AnalyticalLoadCombination& loadCombination)
{
    if (!loadCombinationOS.Get ("name", loadCombination.name)) {
        return CreateErrorResponse (APIERR_BADPARS, "name is missing.");
    }
    loadCombinationOS.Get ("description", loadCombination.description);
    GS::Optional<GS::ObjectState> error = GetRequiredEnumField (loadCombinationOS, "category", AnalyticalLoadCombinationCategoryNames, loadCombination.category);
    if (error.HasValue ()) {
        return error;
    }
    if (loadCombinationOS.Contains ("nationalStandard")) {
        API_AnalyticalLoadCombinationNationalStandard nationalStandard = API_AnalyticalLoadNationalStandard_ENULSSTRGEOSetB;
        error = GetRequiredEnumField (loadCombinationOS, "nationalStandard", AnalyticalLoadCombinationNationalStandardNames, nationalStandard);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.nationalStandard = nationalStandard;
    }
    if (loadCombinationOS.Contains ("type")) {
#ifdef ServerMainVers_2700
        API_LoadCombinationType type = API_LoadCombinationType::API_LoadCombinationType_Linear;
        error = GetRequiredEnumField (loadCombinationOS, "type", AnalyticalLoadCombinationTypeNames, type);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.type = type;
#else
        return CreateErrorResponse (APIERR_BADPARS, "type is available from Archicad 27.");
#endif
    }
    GS::Array<GS::ObjectState> loadFactors;
    loadCombinationOS.Get ("loadFactors", loadFactors);
    for (const GS::ObjectState& loadFactorOS : loadFactors) {
        API_AnalyticalLoadFactor loadFactor = {};
        error = GetAnalyticalLoadFactorFromObjectState (loadFactorOS, loadFactor);
        if (error.HasValue ()) {
            return error;
        }
        loadCombination.loadFactors.Push (loadFactor);
    }
    return GS::NoValue;
}

// Deletes the objects of the given ids in one undo step, with one execution result per id.
GS::ObjectState DeleteAnalyticalLoadObjects (const GS::Array<API_Guid>& guids, const char* undoName, const char* idKey, const char* failureMessage, GSErrCode (*deleteFunction) (const API_Guid&))
{
    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand (undoName, [&] () -> GSErrCode {
        for (const API_Guid& guid : guids) {
            if (guid == APINULLGuid) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, GS::UniString::Printf ("%s is missing or invalid.", idKey)));
                continue;
            }
            const GSErrCode err = deleteFunction (guid);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, failureMessage));
        }
        return NoError;
    });

    return response;
}

}

GetAnalyticalLoadGroupsCommand::GetAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadGroupsCommand::GetName () const
{
    return "GetAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> GetAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroups": {
                "type": "array",
                "description": "The load groups of the project. A load group that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadGroup"
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
            "loadGroups"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadGroupGuids;
    const GSErrCode err = TAPIR_GetAnalyticalLoadGroups (loadGroupGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load groups.");
    }

    GS::ObjectState response;
    const auto& loadGroups = response.AddList<GS::ObjectState> ("loadGroups");
    for (const API_Guid& loadGroupGuid : loadGroupGuids) {
        API_AnalyticalLoadGroup loadGroup = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadGroup (loadGroupGuid, loadGroup);
        if (getErr != NoError) {
            loadGroups (CreateErrorResponse (getErr, "Failed to get the analytical load group."));
            continue;
        }
        loadGroups (CreateAnalyticalLoadGroupObjectState (loadGroup));
    }

    return response;
}

GetAnalyticalLoadCasesCommand::GetAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadCasesCommand::GetName () const
{
    return "GetAnalyticalLoadCases";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the load group to list the load cases of. Every load case of the project is listed when it is omitted."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCases": {
                "type": "array",
                "description": "The load cases. A load case that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadCase"
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
            "loadCases"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadCaseGuids;
    GSErrCode err = NoError;
    if (parameters.Contains ("loadGroupId")) {
        API_Guid loadGroupGuid = APINULLGuid;
        const GS::Optional<GS::ObjectState> error = GetRequiredGuidField (parameters, "loadGroupId", loadGroupGuid);
        if (error.HasValue ()) {
            return *error;
        }
        err = TAPIR_GetAnalyticalLoadCases (loadGroupGuid, loadCaseGuids);
    } else {
        err = TAPIR_GetAllAnalyticalLoadCases (loadCaseGuids);
    }
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load cases.");
    }

    GS::ObjectState response;
    const auto& loadCases = response.AddList<GS::ObjectState> ("loadCases");
    for (const API_Guid& loadCaseGuid : loadCaseGuids) {
        API_AnalyticalLoadCase loadCase = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadCase (loadCaseGuid, loadCase);
        if (getErr != NoError) {
            loadCases (CreateErrorResponse (getErr, "Failed to get the analytical load case."));
            continue;
        }
        loadCases (CreateAnalyticalLoadCaseObjectState (loadCase));
    }

    return response;
}

GetAnalyticalLoadCombinationsCommand::GetAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAnalyticalLoadCombinationsCommand::GetName () const
{
    return "GetAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> GetAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinations": {
                "type": "array",
                "description": "The load combinations of the project. A load combination that cannot be read is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/AnalyticalLoadCombination"
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
            "loadCombinations"
        ]
    })";
}

GS::ObjectState GetAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<API_Guid> loadCombinationGuids;
    const GSErrCode err = TAPIR_GetAnalyticalLoadCombinations (loadCombinationGuids);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the analytical load combinations.");
    }

    GS::ObjectState response;
    const auto& loadCombinations = response.AddList<GS::ObjectState> ("loadCombinations");
    for (const API_Guid& loadCombinationGuid : loadCombinationGuids) {
        API_AnalyticalLoadCombination loadCombination = {};
        const GSErrCode getErr = TAPIR_GetAnalyticalLoadCombination (loadCombinationGuid, loadCombination);
        if (getErr != NoError) {
            loadCombinations (CreateErrorResponse (getErr, "Failed to get the analytical load combination."));
            continue;
        }
        loadCombinations (CreateAnalyticalLoadCombinationObjectState (loadCombination));
    }

    return response;
}

CreateAnalyticalLoadGroupsCommand::CreateAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadGroupsCommand::GetName () const
{
    return "CreateAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroups": {
                "type": "array",
                "description": "The load groups to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "groupType": {
                            "$ref": "#/AnalyticalLoadGroupType"
                        },
                        "relation": {
                            "$ref": "#/AnalyticalLoadGroupRelation"
                        },
                        "loadType": {
                            "type": "string",
                            "description": "The load type of the group as free text. Empty when it is omitted."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "groupType",
                        "relation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadGroups"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupIds": {
                "type": "array",
                "description": "The identifiers of the new load groups, in the order of the input. A load group that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadGroupId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadGroupId"
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
            "loadGroupIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadGroups;
    parameters.Get ("loadGroups", loadGroups);

    GS::ObjectState response;
    const auto& loadGroupIds = response.AddList<GS::ObjectState> ("loadGroupIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Groups", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadGroupOS : loadGroups) {
            API_AnalyticalLoadGroup loadGroup = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadGroupFromObjectState (loadGroupOS, loadGroup);
            if (error.HasValue ()) {
                loadGroupIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadGroup (loadGroup);
            if (err != NoError) {
                loadGroupIds (CreateErrorResponse (err, "Failed to create the analytical load group."));
                continue;
            }
            loadGroupIds (GS::ObjectState ("loadGroupId", CreateGuidObjectState (loadGroup.guid)));
        }
        return NoError;
    });

    return response;
}

CreateAnalyticalLoadCasesCommand::CreateAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadCasesCommand::GetName () const
{
    return "CreateAnalyticalLoadCases";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCases": {
                "type": "array",
                "description": "The load cases to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string",
                            "description": "Empty when it is omitted."
                        },
                        "actionType": {
                            "$ref": "#/AnalyticalLoadCaseActionType"
                        },
                        "loadGroupId": {
                            "$ref": "#/GuidId",
                            "description": "The identifier of the load group the load case belongs to."
                        },
                        "loadType": {
                            "$ref": "#/AnalyticalLoadCaseLoadType"
                        },
                        "duration": {
                            "$ref": "#/AnalyticalLoadCaseDuration"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "actionType",
                        "loadGroupId",
                        "loadType"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCases"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCaseIds": {
                "type": "array",
                "description": "The identifiers of the new load cases, in the order of the input. A load case that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadCaseId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadCaseId"
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
            "loadCaseIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadCases;
    parameters.Get ("loadCases", loadCases);

    GS::ObjectState response;
    const auto& loadCaseIds = response.AddList<GS::ObjectState> ("loadCaseIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Cases", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadCaseOS : loadCases) {
            API_AnalyticalLoadCase loadCase = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadCaseFromObjectState (loadCaseOS, loadCase);
            if (error.HasValue ()) {
                loadCaseIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadCase (loadCase);
            if (err != NoError) {
                loadCaseIds (CreateErrorResponse (err, "Failed to create the analytical load case."));
                continue;
            }
            loadCaseIds (GS::ObjectState ("loadCaseId", CreateGuidObjectState (loadCase.loadCaseGuid)));
        }
        return NoError;
    });

    return response;
}

CreateAnalyticalLoadCombinationsCommand::CreateAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAnalyticalLoadCombinationsCommand::GetName () const
{
    return "CreateAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinations": {
                "type": "array",
                "description": "The load combinations to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string",
                            "description": "Empty when it is omitted."
                        },
                        "category": {
                            "$ref": "#/AnalyticalLoadCombinationCategory"
                        },
                        "nationalStandard": {
                            "$ref": "#/AnalyticalLoadCombinationNationalStandard"
                        },
                        "type": {
                            "$ref": "#/AnalyticalLoadCombinationType"
                        },
                        "loadFactors": {
                            "type": "array",
                            "description": "The load cases of the combination with their factors.",
                            "items": {
                                "$ref": "#/AnalyticalLoadFactor"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "category"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationIds": {
                "type": "array",
                "description": "The identifiers of the new load combinations, in the order of the input. A load combination that could not be created is represented by an error object.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "loadCombinationId": {
                                    "$ref": "#/GuidId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "loadCombinationId"
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
            "loadCombinationIds"
        ]
    })";
}

GS::ObjectState CreateAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> loadCombinations;
    parameters.Get ("loadCombinations", loadCombinations);

    GS::ObjectState response;
    const auto& loadCombinationIds = response.AddList<GS::ObjectState> ("loadCombinationIds");

    ACAPI_CallUndoableCommand ("Create Analytical Load Combinations", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadCombinationOS : loadCombinations) {
            API_AnalyticalLoadCombination loadCombination = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadCombinationFromObjectState (loadCombinationOS, loadCombination);
            if (error.HasValue ()) {
                loadCombinationIds (*error);
                continue;
            }
            const GSErrCode err = TAPIR_CreateAnalyticalLoadCombination (loadCombination);
            if (err != NoError) {
                loadCombinationIds (CreateErrorResponse (err, "Failed to create the analytical load combination."));
                continue;
            }
            loadCombinationIds (GS::ObjectState ("loadCombinationId", CreateGuidObjectState (loadCombination.guid)));
        }
        return NoError;
    });

    return response;
}

DeleteAnalyticalLoadGroupsCommand::DeleteAnalyticalLoadGroupsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadGroupsCommand::GetName () const
{
    return "DeleteAnalyticalLoadGroups";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadGroupsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadGroupIds": {
                "type": "array",
                "description": "The load groups to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadGroupId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadGroupId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadGroupIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadGroupsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteAnalyticalLoadGroupsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadGroupIds", "loadGroupId"),
        "Delete Analytical Load Groups", "loadGroupId", "Failed to delete the analytical load group.", TAPIR_DeleteAnalyticalLoadGroup);
}

DeleteAnalyticalLoadCasesCommand::DeleteAnalyticalLoadCasesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadCasesCommand::GetName () const
{
    return "DeleteAnalyticalLoadCases";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCasesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCaseIds": {
                "type": "array",
                "description": "The load cases to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadCaseId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadCaseId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCaseIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCasesCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState DeleteAnalyticalLoadCasesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadCaseIds", "loadCaseId"),
        "Delete Analytical Load Cases", "loadCaseId", "Failed to delete the analytical load case.", TAPIR_DeleteAnalyticalLoadCase);
}

DeleteAnalyticalLoadCombinationsCommand::DeleteAnalyticalLoadCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAnalyticalLoadCombinationsCommand::GetName () const
{
    return "DeleteAnalyticalLoadCombinations";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationIds": {
                "type": "array",
                "description": "The load combinations to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "loadCombinationId": {
                            "$ref": "#/GuidId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "loadCombinationId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinationIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAnalyticalLoadCombinationsCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState DeleteAnalyticalLoadCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return DeleteAnalyticalLoadObjects (GetGuidsFromIdList (parameters, "loadCombinationIds", "loadCombinationId"),
        "Delete Analytical Load Combinations", "loadCombinationId", "Failed to delete the analytical load combination.", TAPIR_DeleteAnalyticalLoadCombination);
}

AddLoadCasesToLoadCombinationCommand::AddLoadCasesToLoadCombinationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String AddLoadCasesToLoadCombinationCommand::GetName () const
{
    return "AddLoadCasesToLoadCombination";
}

GS::Optional<GS::UniString> AddLoadCasesToLoadCombinationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "loadCombinationId": {
                "$ref": "#/GuidId",
                "description": "The identifier of the load combination to add the load cases to."
            },
            "loadFactors": {
                "type": "array",
                "description": "The load cases to add, with their factors.",
                "items": {
                    "$ref": "#/AnalyticalLoadFactor"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "loadCombinationId",
            "loadFactors"
        ]
    })";
}

GS::Optional<GS::UniString> AddLoadCasesToLoadCombinationCommand::GetRawResponseSchema () const
{
    return DeleteAnalyticalLoadGroupsCommand ().GetRawResponseSchema ();
}

GS::ObjectState AddLoadCasesToLoadCombinationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_Guid loadCombinationGuid = APINULLGuid;
    const GS::Optional<GS::ObjectState> combinationError = GetRequiredGuidField (parameters, "loadCombinationId", loadCombinationGuid);
    if (combinationError.HasValue ()) {
        return *combinationError;
    }

    GS::Array<GS::ObjectState> loadFactors;
    parameters.Get ("loadFactors", loadFactors);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Add Load Cases To Load Combination", [&] () -> GSErrCode {
        for (const GS::ObjectState& loadFactorOS : loadFactors) {
            API_AnalyticalLoadFactor loadFactor = {};
            const GS::Optional<GS::ObjectState> error = GetAnalyticalLoadFactorFromObjectState (loadFactorOS, loadFactor);
            if (error.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*error));
                continue;
            }
            const GSErrCode err = TAPIR_AddLoadCaseToLoadCombination (loadFactor, loadCombinationGuid);
            executionResults (err == NoError
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (err, "Failed to add the load case to the load combination."));
        }
        return NoError;
    });

    return response;
}
