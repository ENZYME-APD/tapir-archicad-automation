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
#endif

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
