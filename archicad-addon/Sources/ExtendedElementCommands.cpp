// Model3D/MeshBody.hpp MUST be included before ExtendedElementCommands.hpp/ACAPinc.h, not after -
// confirmed live: including it afterward makes GDLDefs.h/PropertyListImp.hpp fail to parse
// ("'GS' n'est pas membre de 'GS'"), a GDL header ordering conflict between ACAPinc.h's own GDL
// declarations and the GDLWrapping.hpp pulled in transitively by MeshBody.hpp. Root cause not
// fully understood beyond "order matters"; isolated and confirmed via standalone cl.exe repro.
#include "Model3D/MeshBody.hpp"

#include "ExtendedElementCommands.hpp"

#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

#ifdef ServerMainVers_2900
#include	"ACAPI/Element/Opening/OpeningDefault.hpp"
#include	"ACAPI/Element/Opening/Opening.hpp"
#include "Polygon2D.hpp"
#endif

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>

namespace {

constexpr double DegreesToRadians = 3.14159265358979323846 / 180.0;

enum class StructureSelectionKind {
    Unspecified,
    Basic,
    Composite,
    Profile
};

struct StructureSelection {
    StructureSelectionKind kind = StructureSelectionKind::Unspecified;
    API_AttributeIndex buildingMaterial = APIInvalidAttributeIndex;
    API_AttributeIndex composite = APIInvalidAttributeIndex;
    API_AttributeIndex profile = APIInvalidAttributeIndex;
};

struct AssociativeDimensionPoint {
    API_Guid elementGuid = APINULLGuid;
    API_ElemTypeID elementType = API_ZombieElemID;
    bool line = false;
    Int32 inIndex = 0;
    char special = 0;
    short nodeType = 0;
    short nodeStatus = 0;
    UInt32 nodeId = 0;
};

enum class SectionAssociativeDimensionPreset {
    WallCompositeFaces,
    WallSkinBorders,
    SlabCompositeFaces,
    SlabSkinBorders,
    BeamOrColumnRefLineEndPoints,
    BeamOrColumnBoundingBoxCorners,
    DoorWindowWallHoleCorners,
    DoorWindowModelHotspots
};

GS::Optional<double> GetOptionalDouble (const GS::ObjectState& parameters, const char* fieldName)
{
    double value = 0.0;
    if (parameters.Get (fieldName, value)) {
        return value;
    }
    return {};
}

GS::Optional<GS::ObjectState> GetOptionalObjectState (const GS::ObjectState& parameters, const char* fieldName)
{
    const GS::ObjectState* value = parameters.Get (fieldName);
    if (value == nullptr) {
        return {};
    }
    return *value;
}

GS::Optional<API_Coord> GetOptionalCoordinate2D (const GS::ObjectState& parameters, const char* fieldName)
{
    const GS::ObjectState* coord = parameters.Get (fieldName);
    if (coord == nullptr) {
        return {};
    }
    return Get2DCoordinateFromObjectState (*coord);
}

GS::Optional<GS::UniString> GetElementArray (const GS::ObjectState& parameters, const char* fieldName, GS::Array<GS::ObjectState>& outArray)
{
    if (!parameters.Get (fieldName, outArray)) {
        return GS::UniString::Printf ("Missing required array field '%s'.", fieldName);
    }
    return {};
}

GS::Optional<API_Coord3D> GetOptionalCoordinate3D (const GS::ObjectState& parameters, const char* fieldName)
{
    const GS::ObjectState* coord = parameters.Get (fieldName);
    if (coord == nullptr) {
        return {};
    }
    return Get3DCoordinateFromObjectState (*coord);
}

bool ResolveAttributeIndex (const GS::ObjectState& attributeId, API_AttrTypeID attributeType, API_AttributeIndex& attributeIndex)
{
    API_Attribute attribute = {};
    attribute.header.typeID = attributeType;
    attribute.header.guid = GetGuidFromObjectState (attributeId);
    if (attribute.header.guid == APINULLGuid) {
        return false;
    }

    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return false;
    }

    attributeIndex = attribute.header.index;
    return true;
}

GS::Optional<GS::UniString> TryResolveAttributeField (
    const GS::ObjectState& parameters,
    const char* fieldName,
    API_AttrTypeID attributeType,
    bool& hasValue,
    API_AttributeIndex& outIndex)
{
    hasValue = false;

    const GS::ObjectState* attributeId = parameters.Get (fieldName);
    if (attributeId == nullptr) {
        return {};
    }

    hasValue = true;
    if (!ResolveAttributeIndex (*attributeId, attributeType, outIndex)) {
        return GS::UniString::Printf ("Invalid attribute reference in '%s'.", fieldName);
    }

    return {};
}

GS::Optional<GS::UniString> ParseStructureSelection (
    const GS::ObjectState& parameters,
    bool allowComposite,
    bool allowProfile,
    StructureSelection& selection)
{
    GS::UniString structureType;
    const bool hasStructureType = parameters.Get ("structureType", structureType);

    bool hasBuildingMaterial = false;
    bool hasComposite = false;
    bool hasProfile = false;

    {
        auto err = TryResolveAttributeField (parameters, "buildingMaterialId", API_BuildingMaterialID, hasBuildingMaterial, selection.buildingMaterial);
        if (err.HasValue ()) {
            return err;
        }
    }
    {
        auto err = TryResolveAttributeField (parameters, "compositeId", API_CompWallID, hasComposite, selection.composite);
        if (err.HasValue ()) {
            return err;
        }
    }
    {
        auto err = TryResolveAttributeField (parameters, "profileId", API_ProfileID, hasProfile, selection.profile);
        if (err.HasValue ()) {
            return err;
        }
    }

    const int explicitlyProvidedKinds = static_cast<int> (hasBuildingMaterial) + static_cast<int> (hasComposite) + static_cast<int> (hasProfile);
    if (explicitlyProvidedKinds > 1) {
        return "Only one of 'buildingMaterialId', 'compositeId' or 'profileId' may be provided at a time.";
    }

    if (hasComposite && !allowComposite) {
        return "'compositeId' is not supported for this element type.";
    }
    if (hasProfile && !allowProfile) {
        return "'profileId' is not supported for this element type.";
    }

    if (hasStructureType) {
        if (structureType == "Basic") {
            selection.kind = StructureSelectionKind::Basic;
        } else if (structureType == "Composite") {
            if (!allowComposite) {
                return "'structureType=Composite' is not supported for this element type.";
            }
            selection.kind = StructureSelectionKind::Composite;
        } else if (structureType == "Profile") {
            if (!allowProfile) {
                return "'structureType=Profile' is not supported for this element type.";
            }
            selection.kind = StructureSelectionKind::Profile;
        } else {
            return "Invalid 'structureType'. Use 'Basic', 'Composite' or 'Profile'.";
        }
    } else if (hasBuildingMaterial) {
        selection.kind = StructureSelectionKind::Basic;
    } else if (hasComposite) {
        selection.kind = StructureSelectionKind::Composite;
    } else if (hasProfile) {
        selection.kind = StructureSelectionKind::Profile;
    }

    if (selection.kind == StructureSelectionKind::Basic && (hasComposite || hasProfile)) {
        return "'structureType=Basic' cannot be combined with 'compositeId' or 'profileId'.";
    }
    if (selection.kind == StructureSelectionKind::Composite && (hasBuildingMaterial || hasProfile)) {
        return "'structureType=Composite' cannot be combined with 'buildingMaterialId' or 'profileId'.";
    }
    if (selection.kind == StructureSelectionKind::Profile && (hasBuildingMaterial || hasComposite)) {
        return "'structureType=Profile' cannot be combined with 'buildingMaterialId' or 'compositeId'.";
    }

    return {};
}

void SetOpeningSizeMask (API_Element& mask)
{
    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
}

bool DoesWallExist (const API_Guid& wallGuid)
{
    return DoesElementExist (wallGuid, API_WallID);
}

// Archicad cannot put an opening into a polygonal wall - the DevKit's own
// Do_CreateWindow refuses the case outright ("No way to put openings into polygonal
// walls") before it ever reaches ACAPI_Element_CreateExt. Creating one anyway does not
// fail: the window or door is placed at a fixed spot inside the wall and every
// centerOffset lands in the same place, which is what #453 reports. Refusing it here
// turns a silently wrong result into an answerable error.
bool IsPolygonalWall (const API_Guid& wallGuid)
{
    API_Element wall = {};
#ifdef ServerMainVers_2600
    wall.header.type = API_WallID;
#else
    wall.header.typeID = API_WallID;
#endif
    wall.header.guid = wallGuid;
    if (ACAPI_Element_Get (&wall) != NoError) {
        return false;
    }
    return wall.wall.type == APIWtyp_Poly;
}

// An opening's base polygon is built from width and height, so a missing one reaches
// Archicad as an empty polygon and comes back as "Can't use empty polygon!", which says
// nothing about the field that was left out (#453). Windows and doors are NOT checked
// this way on purpose: there the size is optional and the tool defaults or the named
// favorite supply it.
GS::Optional<GS::UniString> CheckOpeningSize (const GS::ObjectState& data)
{
    const auto width = GetOptionalDouble (data, "width");
    const auto height = GetOptionalDouble (data, "height");
    if (!width.HasValue () || !height.HasValue ()) {
        return GS::UniString ("Both 'width' and 'height' are required to create an opening.");
    }
    if (width.Get () <= 0.0 || height.Get () <= 0.0) {
        return GS::UniString ("'width' and 'height' must be greater than zero.");
    }
    return {};
}

// A window or a door is created together with its Main Marker sub-element, and that marker
// lives on the floor plan: ACAPI_Element_CreateExt refuses the whole create with
// APIERR_BADDATABASE (-2130313110) when the current database is anything else, whatever the
// caller sent. That is what #532 turned out to be - the very same script created walls,
// columns, slabs and openings from the 3D window without complaint and only CreateWindows /
// CreateDoors failed, and the identical payload worked as soon as the floor plan was active.
//
// So switch the CURRENT DATABASE - not the visible window - to the floor plan around the
// create, and let the caller put the previous one back. This is the same
// ACAPI_Database_ChangeCurrentDatabase dance CreateDrawingsCommand and ChangeWindowToViewCommand
// already do, and the note in ApplicationCommands.cpp calls the current database the one
// "used by ACAPI element creation".
//
// `switched` says whether anything has to be restored: when the floor plan is already the
// current database nothing is touched, so the path that works today stays exactly as it is.
GSErrCode SwitchCurrentDatabaseToFloorPlan (API_DatabaseInfo& previousDatabase, bool& switched)
{
    switched = false;

    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&previousDatabase);
    if (err != NoError) {
        return err;
    }
    if (previousDatabase.typeID == APIWind_FloorPlanID) {
        return NoError;
    }

    API_DatabaseInfo floorPlanDatabase = {};
    floorPlanDatabase.typeID = APIWind_FloorPlanID;
    err = ACAPI_Window_GetDatabaseInfo (&floorPlanDatabase);
    if (err != NoError) {
        return err;
    }

    err = ACAPI_Database_ChangeCurrentDatabase (&floorPlanDatabase);
    if (err != NoError) {
        return err;
    }

    switched = true;
    return NoError;
}

GSErrCode PrepareWindowOrDoorDefaults (API_ElemTypeID elemTypeId, API_Element& element, API_ElementMemo& memo, API_SubElement& marker)
{
    element = {};
    marker = {};
#ifdef ServerMainVers_2600
    element.header.type = elemTypeId;
#else
    element.header.typeID = elemTypeId;
#endif
    marker.subType = APISubElement_MainMarker;

    GSErrCode err = ACAPI_Element_GetDefaultsExt (&element, &memo, 1UL, &marker);
    if (err != NoError) {
        return err;
    }

    // `marker.subType` was NOT asked for with `APISubElement_NoParams`, so the call above
    // already returned the parameters of the TOOL DEFAULT marker - the window/door stamp
    // as the template, the user or a just-applied favorite configured it. Overwriting them
    // with the marker parent library part's factory values silently threw those settings
    // away (#551), and leaked the handle `GetDefaultsExt` had allocated. Keep them, and
    // leave the marker object's pen alone: `useObjPens` / `pen` are part of the very same
    // tool default. Compare `CreateSectionsCommand`, which passes the marker straight from
    // `GetDefaultsExt` to `CreateExt` without touching its parameters.
    const GSSize markerParamNum = marker.memo.params != nullptr
        ? BMGetHandleSize ((GSHandle) marker.memo.params) / sizeof (API_AddParType)
        : 0;
    if (markerParamNum > 0) {
        return NoError;
    }

    // No tool default marker parameters (a project where the Window / Door tool has never
    // been opened): fall back to the marker parent's library part, as the DevKit's own
    // window/door creation sample does. Without parameters `ACAPI_Element_CreateExt`
    // refuses the marker, so an unset pen is filled in here too.
    API_LibPart libPart = {};
#ifdef ServerMainVers_2700
    err = ACAPI_LibraryPart_GetMarkerParent (element.header.type, libPart);
#elif ServerMainVers_2600
    err = ACAPI_Goodies_GetMarkerParent (element.header.type, libPart);
#else
    err = ACAPI_Goodies (APIAny_GetMarkerParentID, (void*)&element.header.typeID, (void*)&libPart);
#endif
    if (err != NoError) {
        return NoError;
    }

    err = ACAPI_LibraryPart_Search (&libPart, false, true);
    delete libPart.location;
    if (err != NoError) {
        return err;
    }

    double a = 0.0;
    double b = 0.0;
    Int32 addParNum = 0;
    API_AddParType** markAddPars = nullptr;
    err = ACAPI_LibraryPart_GetParams (libPart.index, &a, &b, &addParNum, &markAddPars);
    if (err != NoError) {
        return err;
    }

    marker.memo.params = markAddPars;
    if (marker.subElem.object.pen <= 0) {
        marker.subElem.object.pen = 166;
    }
    marker.subElem.object.useObjPens = true;
    return NoError;
}

// Apply a named FAVORITE to the Door/Window tool defaults BEFORE
// the caller clones element/memo/marker via PrepareWindowOrDoorDefaults.
//
// The parameter is a FAVORITE name (as returned by `GetFavoritesByType
// (elementType=Door|Window)`), NOT a libpart `docu_UName`. Real
// libpart-by-name lookup against AC29's Door/Window library is not
// viable: the favorite name is the only stable identifier exposed to
// API callers.
//
// This is the workaround for `-2130313110 Failed to create door` /
// `Failed to create window` on a fresh project where the user has
// never opened the Door / Window tool — without the favorite-apply,
// `PrepareWindowOrDoorDefaults` clones empty/invalid tool defaults
// that AC's `ACAPI_Element_CreateExt` rejects. Calling this helper
// first pushes a known-good defaults snapshot into the tool, then
// the caller's PrepareWindowOrDoorDefaults clones the favorite-applied
// state (libpart, marker memo, all openingBase fields).
//
// Flow (mirrors `ApplyFavoritesToElementDefaultsCommand`):
//   1. `ACAPI_Favorite_Get(&favorite)` by name
//   2. `ACAPI_Element_ChangeDefaultsExt` with mask FULL — also passes
//      the favorite's `elementMarker` / `memoMarker` as a Main Marker
//      sub-element when present, since AC25+ recommends ChangeDefaultsExt
//      for markered element types (Door / Window).
//   3. Replay classifications, category values, user properties via
//      the existing TAPIR_Element_* helpers in `MigrationHelper.hpp`.
//      Required: some Door favorites (e.g. `Porte d'entrée`) still
//      return REFUSEDPAR after ChangeDefaultsExt alone if the
//      classifications are missing.
//
// IMPORTANT: must be invoked BEFORE PrepareWindowOrDoorDefaults so
// the marker built from `ACAPI_LibraryPart_GetMarkerParent` matches
// the favorite-applied libpart. Calling it AFTER leaves the outer
// marker stale and CreateExt fails with REFUSEDPAR.
//
// Returns NoError on success or when `favoriteName` is absent
// (caller falls back to the cloned tool defaults). Returns
// APIERR_REFUSEDPAR when the favorite resolves to a different
// element type than `expectedTypeId` (e.g. passing a Window favorite
// to CreateDoors). Otherwise returns the underlying Favorite_Get /
// ChangeDefaultsExt error code.
static GSErrCode ApplyWindowOrDoorFavoriteToDefaults (const GS::ObjectState& data, API_ElemTypeID expectedTypeId)
{
    GS::UniString favoriteName;
    if (!data.Get ("favoriteName", favoriteName)) {
        return NoError; // field absent — keep tool defaults as-is
    }
    if (favoriteName.IsEmpty ()) {
        return NoError;
    }

    API_Favorite favorite;
    favorite.name = favoriteName;
    favorite.memo.New ();
    favorite.elementMarker.New ();
    favorite.memoMarker.New ();
    favorite.properties.New ();
    favorite.classifications.New ();
    favorite.elemCategoryValues.New ();

    GSErrCode err = ACAPI_Favorite_Get (&favorite);
    const auto disposeFavoriteMemos = [&]() {
        ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
        if (favorite.memoMarker.HasValue ()) {
            ACAPI_DisposeElemMemoHdls (&favorite.memoMarker.Get ());
        }
    };
    if (err != NoError) {
        disposeFavoriteMemos ();
        return err;
    }

    // Guard against type mismatch: a Window favorite applied to the
    // Door tool defaults (or vice versa) would silently corrupt the
    // tool state and leave the caller's PrepareWindowOrDoorDefaults
    // operating on the wrong subtype. Reject early.
#ifdef ServerMainVers_2600
    const API_ElemTypeID favoriteTypeId = favorite.element.header.type.typeID;
#else
    const API_ElemTypeID favoriteTypeId = favorite.element.header.typeID;
#endif
    if (favoriteTypeId != expectedTypeId) {
        disposeFavoriteMemos ();
        return APIERR_REFUSEDPAR;
    }

    // Push the favorite's full element state into the Door/Window tool
    // defaults. AC25+ recommends `ACAPI_Element_ChangeDefaultsExt` for
    // markered element types (API_DoorID / API_WindowID) so the marker
    // sub-element is updated in lock-step with the main element. If the
    // favorite carries a marker (`elementMarker` / `memoMarker`), pass
    // it as a Main Marker sub-element; otherwise pass `nSubElems = 0`
    // and the existing tool marker is preserved.
    API_Element mask;
    ACAPI_ELEMENT_MASK_SETFULL (mask);

    API_SubElement markerSubElement = {};
    UInt32 nSubElems = 0;
    API_SubElement* subElemsPtr = nullptr;
    if (favorite.elementMarker.HasValue () && favorite.memoMarker.HasValue ()) {
        markerSubElement.subType = APISubElement_MainMarker;
        markerSubElement.subElem = favorite.elementMarker.Get ();
        markerSubElement.memo = favorite.memoMarker.Get ();
        ACAPI_ELEMENT_MASK_SETFULL (markerSubElement.mask);
        nSubElems = 1;
        subElemsPtr = &markerSubElement;
    }

    err = ACAPI_Element_ChangeDefaultsExt (&favorite.element, favorite.memo.GetPtr (), &mask, nSubElems, subElemsPtr);
    disposeFavoriteMemos ();
    if (err != NoError) {
        return err;
    }

    // Mirror the full ApplyFavoritesToElementDefaultsCommand flow so the
    // tool defaults include the favorite's classifications, category
    // values and user-defined properties. Some Door favorites (e.g.
    // "Porte d'entrée") will not survive a subsequent CreateExt without
    // this extra metadata: ChangeDefaults alone gets accepted, but the
    // create fails with APIERR_REFUSEDPAR because mandatory classification
    // or category fields remain unset on the tool defaults.
    for (const GS::Pair<API_Guid, API_Guid>& pair : *favorite.classifications) {
        TAPIR_Element_AddClassificationItemDefault (favorite.element.header, pair.second);
    }
    for (const API_ElemCategoryValue& categoryValue : *favorite.elemCategoryValues) {
        TAPIR_Element_SetCategoryValueDefault (favorite.element.header, categoryValue);
    }
    TAPIR_Element_SetPropertiesOfDefaultElem (favorite.element.header, *favorite.properties);

    return NoError;
}

void FillDimensionDefaults (API_Element& element, const API_Coord& referencePoint, const API_Vector& direction)
{
    element.dimension.dimAppear = APIApp_Normal;
    element.dimension.textPos = APIPos_Above;
    element.dimension.textWay = APIDir_Parallel;
    element.dimension.defStaticDim = false;
    element.dimension.usedIn3D = false;
    element.dimension.horizontalText = false;
    element.dimension.refC = referencePoint;
    element.dimension.direction = direction;
}

GS::Optional<GS::UniString> ParseAssociativeDimensionPoint (const GS::ObjectState& pointData, AssociativeDimensionPoint& point)
{
    const GS::ObjectState* elementId = pointData.Get ("elementId");
    if (elementId == nullptr) {
        return "Missing required field 'elementId'.";
    }

    point.elementGuid = GetGuidFromObjectState (*elementId);
    if (point.elementGuid == APINULLGuid) {
        return "Invalid element identifier for associative dimension point.";
    }

    API_Elem_Head elementHeader = {};
    if (!LoadElementHeaderByGuid (point.elementGuid, elementHeader)) {
        return "Failed to load referenced element for associative dimension point.";
    }
    point.elementType = GetElemTypeId (elementHeader);

    pointData.Get ("line", point.line);
    pointData.Get ("inIndex", point.inIndex);

    Int32 special = 0;
    if (pointData.Get ("special", special)) {
        point.special = static_cast<char> (special);
    }

    Int32 nodeType = 0;
    if (pointData.Get ("nodeType", nodeType)) {
        point.nodeType = static_cast<short> (nodeType);
    }

    Int32 nodeStatus = 0;
    if (pointData.Get ("nodeStatus", nodeStatus)) {
        point.nodeStatus = static_cast<short> (nodeStatus);
    }

    auto nodeId = GetOptionalDouble (pointData, "nodeId");

    if (nodeId.HasValue ()) {
        if (nodeId.Get () < 0.0 || nodeId.Get () > static_cast<double> (std::numeric_limits<UInt32>::max ())) {
            return "The 'nodeId' field must be between 0 and 4294967295.";
        }
        point.nodeId = static_cast<UInt32> (nodeId.Get ());
    }

    return {};
}

GS::Optional<GS::UniString> PopulateAssociativeDimensionMemo (
    const GS::Array<AssociativeDimensionPoint>& points,
    API_Element& element,
    API_ElementMemo& memo)
{
    element.dimension.nDimElem = static_cast<Int32> (points.GetSize ());
    memo.dimElems = reinterpret_cast<API_DimElem**> (BMhAllClear (element.dimension.nDimElem * sizeof (API_DimElem)));
    if (memo.dimElems == nullptr || *memo.dimElems == nullptr) {
        return "Failed to allocate associative dimension witness data.";
    }

    for (UIndex pointIndex = 0; pointIndex < points.GetSize (); ++pointIndex) {
        const AssociativeDimensionPoint& point = points[pointIndex];
        API_DimElem& dimElem = (*memo.dimElems)[pointIndex];
#ifdef ServerMainVers_2600
        dimElem.base.base.type = API_ElemType (point.elementType);
#else
        dimElem.base.base.typeID = point.elementType;
#endif
        dimElem.base.base.guid = point.elementGuid;
        dimElem.base.base.line = point.line;
        dimElem.base.base.inIndex = point.inIndex;
        dimElem.base.base.special = point.special;
        dimElem.base.base.node_typ = point.nodeType;
        dimElem.base.base.node_status = point.nodeStatus;
        dimElem.base.base.node_id = point.nodeId;
        dimElem.note = element.dimension.defNote;
        dimElem.witnessVal = element.dimension.defWitnessVal;
        dimElem.witnessForm = element.dimension.defWitnessForm;
    }

    return {};
}

void TryApplyDimensionFloorIndex (
    const GS::Array<AssociativeDimensionPoint>& points,
    const GS::Optional<double>& floorIndex,
    API_Element& element)
{
    if (floorIndex.HasValue ()) {
        element.header.floorInd = static_cast<short> (floorIndex.Get ());
        return;
    }

    for (const AssociativeDimensionPoint& point : points) {
        API_Elem_Head elementHeader = {};
        if (LoadElementHeaderByGuid (point.elementGuid, elementHeader)) {
            element.header.floorInd = elementHeader.floorInd;
            return;
        }
    }
}

GS::Optional<GS::UniString> ParseSectionAssociativeDimensionPreset (
    const GS::UniString& presetName,
    SectionAssociativeDimensionPreset& preset)
{
    if (presetName == "WallCompositeFaces") {
        preset = SectionAssociativeDimensionPreset::WallCompositeFaces;
    } else if (presetName == "WallSkinBorders") {
        preset = SectionAssociativeDimensionPreset::WallSkinBorders;
    } else if (presetName == "SlabCompositeFaces") {
        preset = SectionAssociativeDimensionPreset::SlabCompositeFaces;
    } else if (presetName == "SlabSkinBorders") {
        preset = SectionAssociativeDimensionPreset::SlabSkinBorders;
    } else if (presetName == "BeamOrColumnRefLineEndPoints") {
        preset = SectionAssociativeDimensionPreset::BeamOrColumnRefLineEndPoints;
    } else if (presetName == "BeamOrColumnBoundingBoxCorners") {
        preset = SectionAssociativeDimensionPreset::BeamOrColumnBoundingBoxCorners;
    } else if (presetName == "DoorWindowWallHoleCorners") {
        preset = SectionAssociativeDimensionPreset::DoorWindowWallHoleCorners;
    } else if (presetName == "DoorWindowModelHotspots") {
        preset = SectionAssociativeDimensionPreset::DoorWindowModelHotspots;
    } else {
        return "Invalid 'preset' value for section associative dimension.";
    }

    return {};
}

GS::Optional<GS::UniString> LoadSectionElementAndParent (
    const API_Guid& sectionElementGuid,
    API_Element& sectionElement,
    API_Element& parentElement)
{
    sectionElement = {};
    sectionElement.header.guid = sectionElementGuid;
    if (ACAPI_Element_Get (&sectionElement) != NoError || GetElemTypeId (sectionElement.header) != API_SectElemID) {
        return "The referenced 'sectionElementId' is not a valid section element.";
    }

    parentElement = {};
#ifdef ServerMainVers_2600
    parentElement.header.type = sectionElement.sectElem.parentType;
#endif
    parentElement.header.guid = sectionElement.sectElem.parentGuid;
    if (parentElement.header.guid == APINULLGuid || ACAPI_Element_Get (&parentElement) != NoError) {
        return "Failed to load the parent element for the referenced section element.";
    }

    return {};
}

void AddSectionAssociativePoint (
    GS::Array<AssociativeDimensionPoint>& points,
    const API_Guid& sectionElementGuid,
    bool line,
    short nodeType,
    short nodeStatus,
    UInt32 nodeId,
    Int32 inIndex = 0,
    char special = 0)
{
    AssociativeDimensionPoint point;
    point.elementGuid = sectionElementGuid;
    point.elementType = API_SectElemID;
    point.line = line;
    point.inIndex = inIndex;
    point.special = special;
    point.nodeType = nodeType;
    point.nodeStatus = nodeStatus;
    point.nodeId = nodeId;
    points.Push (point);
}

GS::Optional<GS::UniString> AppendSectionAssociativeDimensionPoints (
    const GS::ObjectState& data,
    SectionAssociativeDimensionPreset preset,
    const API_Guid& sectionElementGuid,
    GS::Array<AssociativeDimensionPoint>& points,
    API_Vector& defaultDirection)
{
    API_Element sectionElement = {};
    API_Element parentElement = {};
    {
        auto error = LoadSectionElementAndParent (sectionElementGuid, sectionElement, parentElement);
        if (error.HasValue ()) {
            return error;
        }
    }

    auto requireParentType = [&] (std::initializer_list<API_ElemTypeID> allowedTypes, const char* message) -> GS::Optional<GS::UniString> {
        const API_ElemTypeID parentTypeId = GetElemTypeId (parentElement.header);
        for (API_ElemTypeID allowedType : allowedTypes) {
            if (parentTypeId == allowedType) {
                return {};
            }
        }
        return message;
    };

    switch (preset) {
        case SectionAssociativeDimensionPreset::WallCompositeFaces: {
            auto error = requireParentType ({API_WallID}, "The 'WallCompositeFaces' preset requires a wall section element.");
            if (error.HasValue ()) {
                return error;
            }
            defaultDirection = {1.0, 0.0};
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 256, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 1024, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 512, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 768, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 131, 0, 0);
            break;
        }

        case SectionAssociativeDimensionPreset::WallSkinBorders: {
            auto error = requireParentType ({API_WallID}, "The 'WallSkinBorders' preset requires a wall section element.");
            if (error.HasValue ()) {
                return error;
            }
            GS::Array<Int32> skinBorderIndices;
            if (!data.Get ("skinBorderIndices", skinBorderIndices) || skinBorderIndices.IsEmpty ()) {
                return "The 'WallSkinBorders' preset requires a non-empty 'skinBorderIndices' array.";
            }
            defaultDirection = {1.0, 0.0};
            for (Int32 skinBorderIndex : skinBorderIndices) {
                AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 1280, static_cast<UInt32> (skinBorderIndex));
            }
            break;
        }

        case SectionAssociativeDimensionPreset::SlabCompositeFaces: {
            auto error = requireParentType ({API_SlabID}, "The 'SlabCompositeFaces' preset requires a slab section element.");
            if (error.HasValue ()) {
                return error;
            }
            defaultDirection = {0.0, 1.0};
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 256, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 1024, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 512, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 768, 0);
            AddSectionAssociativePoint (points, sectionElement.header.guid, true, 131, 0, 0);
            break;
        }

        case SectionAssociativeDimensionPreset::SlabSkinBorders: {
            auto error = requireParentType ({API_SlabID}, "The 'SlabSkinBorders' preset requires a slab section element.");
            if (error.HasValue ()) {
                return error;
            }
            GS::Array<Int32> skinBorderIndices;
            if (!data.Get ("skinBorderIndices", skinBorderIndices) || skinBorderIndices.IsEmpty ()) {
                return "The 'SlabSkinBorders' preset requires a non-empty 'skinBorderIndices' array.";
            }
            defaultDirection = {0.0, 1.0};
            for (Int32 skinBorderIndex : skinBorderIndices) {
                AddSectionAssociativePoint (points, sectionElement.header.guid, true, 130, 1280, static_cast<UInt32> (skinBorderIndex));
            }
            break;
        }

        case SectionAssociativeDimensionPreset::BeamOrColumnRefLineEndPoints: {
            auto error = requireParentType ({API_BeamID, API_ColumnID}, "The 'BeamOrColumnRefLineEndPoints' preset requires a beam or column section element.");
            if (error.HasValue ()) {
                return error;
            }
            defaultDirection = {1.0, 0.0};
            AddSectionAssociativePoint (points, sectionElement.header.guid, false, 0, 0, 1049586);
            AddSectionAssociativePoint (points, sectionElement.header.guid, false, 0, 0, 2099172);
            break;
        }

        case SectionAssociativeDimensionPreset::BeamOrColumnBoundingBoxCorners: {
            auto error = requireParentType ({API_BeamID, API_ColumnID}, "The 'BeamOrColumnBoundingBoxCorners' preset requires a beam or column section element.");
            if (error.HasValue ()) {
                return error;
            }
            bool beginPlane = true;
            data.Get ("beginPlane", beginPlane);
            bool totalSizePlane = false;
            data.Get ("totalSizePlane", totalSizePlane);

            defaultDirection = {1.0, 2.0};

            const UInt32 planePart = beginPlane ? 4128768U : 8257537U;
            const UInt32 offsets[] = {0U, 12U, 4U, 48U, 60U, 52U, 16U, 28U, 20U};
            for (UInt32 offset : offsets) {
                UInt32 nodeId = planePart + offset;
                if (totalSizePlane) {
                    nodeId += 2U;
                }
                AddSectionAssociativePoint (points, sectionElement.header.guid, false, 0, 0, nodeId);
            }
            break;
        }

        case SectionAssociativeDimensionPreset::DoorWindowWallHoleCorners: {
            auto error = requireParentType ({API_WindowID, API_DoorID}, "The 'DoorWindowWallHoleCorners' preset requires a door or window section element.");
            if (error.HasValue ()) {
                return error;
            }
            bool placeOnTop = false;
            data.Get ("placeOnTop", placeOnTop);

            defaultDirection = {1.0, 0.0};
            for (Int32 pointIndex = 0; pointIndex < 4; ++pointIndex) {
                const short nodeStatus = static_cast<short> (2 + 2 * pointIndex + (placeOnTop ? 1 : 0));
                AddSectionAssociativePoint (points, sectionElement.header.guid, false, 2100, nodeStatus, 0);
            }
            break;
        }

        case SectionAssociativeDimensionPreset::DoorWindowModelHotspots: {
            auto error = requireParentType ({API_WindowID, API_DoorID}, "The 'DoorWindowModelHotspots' preset requires a door or window section element.");
            if (error.HasValue ()) {
                return error;
            }
            defaultDirection = {1.0, 0.0};
            AddSectionAssociativePoint (points, sectionElement.header.guid, false, 0, 0, 11111);
            AddSectionAssociativePoint (points, sectionElement.header.guid, false, 0, 0, 11113);
            break;
        }
    }

    return {};
}

GS::Optional<GS::UniString> BuildSectionAssociativeDimensionPoints (
    const GS::ObjectState& data,
    GS::Array<AssociativeDimensionPoint>& points,
    API_Vector& defaultDirection)
{
    GS::Array<API_Guid> sectionElementGuids;
    const GS::ObjectState* sectionElementId = data.Get ("sectionElementId");
    GS::Array<GS::ObjectState> sectionElementIds;
    const bool hasSectionElementIds = data.Get ("sectionElementIds", sectionElementIds);
    if (sectionElementId != nullptr && hasSectionElementIds) {
        return "Only one of 'sectionElementId' and 'sectionElementIds' can be given.";
    }
    if (sectionElementId != nullptr) {
        sectionElementGuids.Push (GetGuidFromObjectState (*sectionElementId));
    } else {
        for (const GS::ObjectState& sectionElementIdItem : sectionElementIds) {
            sectionElementGuids.Push (GetGuidFromObjectState (sectionElementIdItem));
        }
    }
    if (sectionElementGuids.IsEmpty ()) {
        return "Missing required field 'sectionElementId' or 'sectionElementIds'.";
    }

    GS::UniString presetName;
    if (!data.Get ("preset", presetName)) {
        return "Missing required field 'preset'.";
    }

    SectionAssociativeDimensionPreset preset;
    {
        auto error = ParseSectionAssociativeDimensionPreset (presetName, preset);
        if (error.HasValue ()) {
            return error;
        }
    }

    for (const API_Guid& sectionElementGuid : sectionElementGuids) {
        auto error = AppendSectionAssociativeDimensionPoints (data, preset, sectionElementGuid, points, defaultDirection);
        if (error.HasValue ()) {
            return error;
        }
    }

    return {};
}

GS::ObjectState CreateElementListResponse (const GS::Array<GS::ObjectState>& elementResults)
{
    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const auto& result : elementResults) {
        elements (result);
    }
    return response;
}

GS::ObjectState CreateExecutionResultResponse (const GS::Array<GS::ObjectState>& results)
{
    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const auto& result : results) {
        executionResults (result);
    }
    return response;
}

template<typename Func>
GS::ObjectState ExecuteCreateWithElements (const GS::String& commandName, Func&& createFunc)
{
    GS::Array<GS::ObjectState> results;

    API_NotifyElementType notification = {};
    notification.notifID = APINotifyElement_BeginEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    ACAPI_CallUndoableCommand (commandName, [&]() -> GSErrCode {
        createFunc (results);
        return NoError;
    });

    notification = {};
    notification.notifID = APINotifyElement_EndEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    return CreateElementListResponse (results);
}

template<typename Func>
GS::ObjectState ExecuteModifyWithResults (const GS::String& commandName, Func&& modifyFunc)
{
    GS::Array<GS::ObjectState> results;

    ACAPI_CallUndoableCommand (commandName, [&]() -> GSErrCode {
        modifyFunc (results);
        return NoError;
    });

    return CreateExecutionResultResponse (results);
}

}

GS::Optional<GS::UniString> BuildSlabMemoFromGeometry (
    API_Element& element,
    API_ElementMemo& memo,
    GS::Array<GS::ObjectState>& polygonOutline,
    const GS::Array<GS::ObjectState>& polygonArcs,
    const GS::Array<GS::ObjectState>& holes)
{
    if (polygonOutline.GetSize () < 3) {
        return "'polygonOutline' must contain at least 3 coordinates.";
    }

    if (IsSame2DCoordinate (polygonOutline.GetFirst (), polygonOutline.GetLast ())) {
        polygonOutline.Pop ();
    }

    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    const API_Polygon oldPoly = element.slab.poly;
    element.slab.poly.nCoords = polygonOutline.GetSize () + 1;
    element.slab.poly.nSubPolys = 1;
    element.slab.poly.nArcs = polygonArcs.GetSize ();

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            element.slab.poly.nCoords += holePolygonOutline.GetSize () + 1;
            ++element.slab.poly.nSubPolys;
            element.slab.poly.nArcs += holePolygonArcs.GetSize ();
        }
    }

    // ACAPI_Element_GetDefaults does not always allocate the polygon memo handles for
    // slabs (e.g. the default slab reports nCoords but leaves memo.coords == nullptr).
    // The original size-change-only guards skipped allocation whenever the requested
    // polygon matched the default size, leaving memo.coords null and crashing
    // AddPolyToMemo with a null dereference.
    //
    // Each handle below is checked independently rather than bundled behind a single
    // coords-based guard: ACAPI_Element_GetMemo can legitimately return edgeTrims/
    // sideMaterials/vertexIDs as null even when coords itself is populated (e.g. a slab
    // that has never had a custom edge trim never materializes memo.edgeTrims), and
    // AddPolyToMemo below unconditionally writes through edgeTrims/sideMaterials whenever
    // a non-null override pointer is passed in (as it is here).
    //
    // On a real size change, existing handles are freed with BMKillHandle/BMKillPtr and
    // reallocated fresh, not resized in place with BMReallocHandle/BMReallocPtr - confirmed
    // via a real crash report (issue #452, "Fatal memory error in BMReallocPtr... Requested
    // memory size is 48 bytes" / BNValidWritePtr failure) that a handle coming back from
    // ACAPI_Element_GetMemo is not safe to hand to BMRealloc*, even though it reads back
    // non-null and its contents are valid. This is the same free-then-allocate pattern
    // already used for the Stair baseline memo rebuild elsewhere in this file.
    const Int32 nCoords    = element.slab.poly.nCoords;
    const Int32 nSubPolys  = element.slab.poly.nSubPolys;
    const Int32 nArcs      = element.slab.poly.nArcs;
    const bool  sizeChanged = (oldPoly.nCoords != nCoords);

    if (memo.coords != nullptr && sizeChanged) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.coords == nullptr) {
        memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    }
    if (memo.vertexIDs != nullptr && sizeChanged) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.vertexIDs));
    }
    if (memo.vertexIDs == nullptr) {
        memo.vertexIDs = reinterpret_cast<UInt32**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    }
    if (memo.edgeTrims != nullptr && sizeChanged) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.edgeTrims));
    }
    if (memo.edgeTrims == nullptr) {
        memo.edgeTrims = reinterpret_cast<API_EdgeTrim**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_EdgeTrim), ALLOCATE_CLEAR, 0));
    }
    if (memo.sideMaterials != nullptr && sizeChanged) {
        BMKillPtr (reinterpret_cast<GSPtr*> (&memo.sideMaterials));
    }
    if (memo.sideMaterials == nullptr) {
        memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*> (BMAllocatePtr ((nCoords + 1) * sizeof (API_OverriddenAttribute), ALLOCATE_CLEAR, 0));
    }
    const bool subPolysChanged = (oldPoly.nSubPolys != nSubPolys);
    if (memo.pends != nullptr && subPolysChanged) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.pends));
    }
    if (memo.pends == nullptr) {
        memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
    }
    if (nArcs > 0) {
        const bool arcsChanged = (oldPoly.nArcs != nArcs);
        if (memo.parcs != nullptr && arcsChanged) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
        }
        if (memo.parcs == nullptr) {
            memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
        }
    }
    // Always rewrite vertex IDs: with the free-then-allocate pattern above, any size change
    // yields freshly zeroed (and therefore always-stale) vertex ID slots, and a same-size
    // in-place reuse still needs correct IDs recomputed for the new geometry.
    const bool needToProcessVertexIDs = true;

    const API_EdgeTrimID edgeTrimSideType = APIEdgeTrim_Vertical;
    Int32 iCoord = 1;
    Int32 iArc = 0;
    Int32 iPends = 1;
    AddPolyToMemo (polygonOutline, polygonArcs, iCoord, iArc, iPends, memo, &edgeTrimSideType, &element.slab.sideMat, needToProcessVertexIDs);

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            AddPolyToMemo (holePolygonOutline, holePolygonArcs, iCoord, iArc, iPends, memo, &edgeTrimSideType, &element.slab.sideMat, needToProcessVertexIDs);
        }
    }

    // vertexIDs[0] must hold the max vertex ID used across the whole shape - an undocumented
    // ACAPI_Element_Change requirement (confirmed live in this repo's PolyLine/Hatch geometry
    // SET work) without which ACAPI_Element_Change rejects the polygon with APIERR_BADPOLY.
    // AddPolyToMemo never writes index 0 itself, so it is filled in here by scanning the IDs
    // it did write.
    if (memo.vertexIDs != nullptr) {
        UInt32 maxVertexID = 0;
        for (Int32 i = 1; i <= nCoords; ++i) {
            maxVertexID = std::max (maxVertexID, (*memo.vertexIDs)[i]);
        }
        (*memo.vertexIDs)[0] = maxVertexID;
    }

    return {};
}

// The new-style ACAPI_Polygon_InsertPolyNode/DeletePolyNode/InsertSubPoly/DeleteSubPoly functions
// (declared in ACAPI_Goodies.h) do not exist before Archicad 27 - confirmed via the DevKit headers
// directly, AC25/26 have no ACAPI_Goodies.h at all. Pre-27, the same operations are reached through
// the older, untyped ACAPI_Goodies (API_GoodiesID, void*, void*, void*, void*) dispatcher with the
// APIAny_*PolyNodeID/APIAny_*SubPolyID constants (APIdefs_Goodies.h) - these wrappers keep
// ApplySlabPolygonChange version-agnostic, mirroring the TAPIR_Browser_* pattern in
// ScriptUIPalette.cpp for the same AC25/26-vs-27+ split.
#ifdef ServerMainVers_2700
static GSErrCode TAPIR_Polygon_InsertPolyNode (API_ElementMemo* memo, Int32* nodeIndex, API_Coord* coord)
{
    return ACAPI_Polygon_InsertPolyNode (memo, nodeIndex, coord);
}
static GSErrCode TAPIR_Polygon_DeletePolyNode (API_ElementMemo* memo, Int32* nodeIndex)
{
    return ACAPI_Polygon_DeletePolyNode (memo, nodeIndex);
}
static GSErrCode TAPIR_Polygon_InsertSubPoly (API_ElementMemo* memo, API_ElementMemo* insMemo)
{
    return ACAPI_Polygon_InsertSubPoly (memo, insMemo);
}
static GSErrCode TAPIR_Polygon_DeleteSubPoly (API_ElementMemo* memo, Int32* subPolyIndex)
{
    return ACAPI_Polygon_DeleteSubPoly (memo, subPolyIndex);
}
#else
static GSErrCode TAPIR_Polygon_InsertPolyNode (API_ElementMemo* memo, Int32* nodeIndex, API_Coord* coord)
{
    return ACAPI_Goodies (APIAny_InsertPolyNodeID, memo, nodeIndex, coord);
}
static GSErrCode TAPIR_Polygon_DeletePolyNode (API_ElementMemo* memo, Int32* nodeIndex)
{
    return ACAPI_Goodies (APIAny_DeletePolyNodeID, memo, nodeIndex);
}
static GSErrCode TAPIR_Polygon_InsertSubPoly (API_ElementMemo* memo, API_ElementMemo* insMemo)
{
    return ACAPI_Goodies (APIAny_InsertSubPolyID, memo, insMemo);
}
static GSErrCode TAPIR_Polygon_DeleteSubPoly (API_ElementMemo* memo, Int32* subPolyIndex)
{
    return ACAPI_Goodies (APIAny_DeleteSubPolyID, memo, subPolyIndex);
}
#endif

// Applies a new outline/arcs/holes shape to an EXISTING slab's memo (already populated via
// ACAPI_Element_GetMemo), for use with ACAPI_Element_ChangeMemo. Uses Graphisoft's own
// polygon-editing primitives - TAPIR_Polygon_InsertPolyNode/DeletePolyNode/InsertSubPoly/
// DeleteSubPoly above, matching the DevKit's own Element_Test/Element_Modify_Polygon.cpp reference
// example for these exact element types (including API_SlabID) - to keep the coords/pends/parcs/
// vertexIDs handles obtained from GetMemo internally consistent under a point/hole count change.
// A from-scratch memo rebuild (the same approach BuildSlabMemoFromGeometry above uses for a fresh
// Create) reliably failed with APIERR_BADPOLY via ACAPI_Element_Change here whenever the point or
// subpoly count actually changed - even with every vertexIDs numbering scheme tried (sequential,
// per-contour, mirrored closing duplicates) - while these primitives, paired with
// ACAPI_Element_ChangeMemo and vertexIDs left untouched/zero for new vertices (see
// API_ElementMemo's own documented convention - existing IDs must never be renumbered, new ones
// get ID 0 for Archicad to assign), work correctly for every case tested (issue #452).
static GS::Optional<GS::UniString> ApplySlabPolygonChange (
    API_ElementMemo& memo,
    const API_OverriddenAttribute& sideMat,
    GS::Array<GS::ObjectState>& polygonOutline,
    const GS::Array<GS::ObjectState>& polygonArcs,
    const GS::Array<GS::ObjectState>& holes)
{
    if (polygonOutline.GetSize () < 3) {
        return "'polygonOutline' must contain at least 3 coordinates.";
    }
    if (IsSame2DCoordinate (polygonOutline.GetFirst (), polygonOutline.GetLast ())) {
        polygonOutline.Pop ();
    }
    if (memo.coords == nullptr || memo.pends == nullptr) {
        return "Slab has no polygon data to modify.";
    }
    // Validate before Step 1 below deletes the existing holes - a mis-shaped hole entry must fail
    // the item instead of being skipped after the original holes are already gone.
    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    // A from-scratch memo rebuild (matching CreateSlabs, and BuildMeshPolyMemoFromGeometry's own
    // proven-working pattern for ModifyMeshes) plus ACAPI_Element_Change reliably fails with
    // APIERR_BADPOLY here whenever nCoords changes at all - confirmed live on the plain point-count
    // case alone (no holes involved), so this is not a vertexIDs-numbering issue as first assumed
    // (tried: global sequential, global with the closing duplicate mirroring the first point, and
    // per-contour restart - all three failed identically). Graphisoft's own polygon-editing
    // primitives - ACAPI_Polygon_InsertPolyNode/DeletePolyNode/InsertSubPoly/DeleteSubPoly, used by
    // the DevKit's own reference example (Element_Test/Element_Modify_Polygon.cpp, targeting the
    // same element types including API_SlabID) together with ACAPI_Element_ChangeMemo - keep the
    // existing GetMemo-provided handles' internal consistency intact across a size change, which a
    // full replacement apparently cannot always reproduce. They document that memo's coords/pends/
    // parcs/vertexIDs handles "must be initialized"; ACAPI_Element_GetMemo can legitimately leave
    // vertexIDs/parcs null (e.g. a slab with no per-edge customization/arcs), so those are seeded
    // here first.
    if (memo.vertexIDs == nullptr) {
        // Per Graphisoft's own API_ElementMemo documentation: "If you retrieve an array of
        // vertices, edges or contours with ACAPI_Element_GetMemo, do not change the IDs in these
        // arrays. New vertices, edges and contours should be inserted with ID = 0." "Initialized"
        // (as InsertPolyNode/DeleteSubPoly/etc. require) means the handle must exist at the right
        // size, not that it must be pre-filled with a manually invented ID scheme - Archicad
        // itself assigns real IDs to zero-ID vertices. Earlier attempts at manually numbering this
        // (global sequential, per-contour restart, with/without mirroring the closing duplicate)
        // were all guesses at an ID scheme Archicad was never asking for.
        const Int32 existingNCoords = (Int32) (BMGetHandleSize ((GSHandle) memo.coords) / sizeof (API_Coord)) - 1;
        memo.vertexIDs = reinterpret_cast<UInt32**> (BMAllocateHandle ((existingNCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    }
    if (memo.parcs == nullptr) {
        memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (0, ALLOCATE_CLEAR, 0));
    }

    // Step 1: remove every existing hole (subpoly index >= 2), highest index first so earlier
    // deletions don't shift the indices of subpolys not yet processed.
    const Int32 nSubPolys = (Int32) (BMGetHandleSize ((GSHandle) memo.pends) / sizeof (Int32)) - 1;
    for (Int32 subPolyIndex = nSubPolys; subPolyIndex >= 2; --subPolyIndex) {
        Int32 idx = subPolyIndex;
        GSErrCode err = TAPIR_Polygon_DeleteSubPoly (&memo, &idx);
        if (err != NoError) {
            return "Failed to remove an existing slab hole.";
        }
    }

    // Step 2: resize the main outline (subpoly 1) to the requested point count by repeatedly
    // inserting/deleting node 1 - the coordinate value used for an inserted placeholder node does
    // not matter, every point is overwritten with the real final coordinates right after.
    Int32 outlineCount = (*memo.pends)[1] - 1; // real points, excluding the closing duplicate
    const Int32 desiredOutlineCount = (Int32) polygonOutline.GetSize ();
    while (outlineCount > desiredOutlineCount) {
        Int32 idx = 1;
        GSErrCode err = TAPIR_Polygon_DeletePolyNode (&memo, &idx);
        if (err != NoError) {
            return "Failed to adjust the slab outline's point count.";
        }
        --outlineCount;
    }
    while (outlineCount < desiredOutlineCount) {
        // Per Graphisoft's own DevKit reference example (Do_Poly_InsertNode), nodeIndex is set to
        // the clicked edge's begin-node index + 1 - i.e. the position the NEW node will occupy
        // (shifting what was there up by one), not "insert after this existing node" as the header
        // doc's wording alone suggests. To insert between existing nodes 1 and 2, that means
        // nodeIndex = 2, not 1.
        Int32 idx = 2;
        const API_Coord& p1 = (*memo.coords)[1];
        const API_Coord& p2 = (*memo.coords)[2];
        API_Coord placeholder = {(p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0};
        GSErrCode err = TAPIR_Polygon_InsertPolyNode (&memo, &idx, &placeholder);
        if (err != NoError) {
            return "Failed to adjust the slab outline's point count.";
        }
        ++outlineCount;
    }

    for (Int32 i = 0; i < desiredOutlineCount; ++i) {
        (*memo.coords)[i + 1] = Get2DCoordinateFromObjectState (polygonOutline[i]);
    }
    (*memo.coords)[desiredOutlineCount + 1] = (*memo.coords)[1];

    // Resized to the EXACT new arc count, not just overwritten in place: reusing a stale-sized
    // handle left old arc entries beyond the new count untouched (e.g. going from 2 arcs to 1, or
    // to 0, silently kept the old 2nd arc around) - confirmed live, arcs "stuck" across modify
    // calls that were supposed to remove/replace them.
    {
        const GS::Array<API_PolyArc> arcs = GetPolyArcs (polygonArcs, 1);
        if (memo.parcs != nullptr) {
            BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
        }
        if (!arcs.IsEmpty ()) {
            memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (arcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
            for (UIndex i = 0; i < arcs.GetSize (); ++i) {
                (*memo.parcs)[i] = arcs[i];
            }
        } else {
            memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (0, ALLOCATE_CLEAR, 0));
        }
    }

    // Step 3: add the requested holes back, each as a fresh subpoly.
    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (!GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs) || holePolygonOutline.GetSize () < 3) {
            continue;
        }
        const Int32 holeNCoords = (Int32) holePolygonOutline.GetSize ();
        API_ElementMemo insMemo = {};
        const GS::OnExit insCleanup ([&insMemo] () { ACAPI_DisposeElemMemoHdls (&insMemo); });
        // +2, not +1: index 0 is the unused dummy slot and index holeNCoords+1 holds the closing
        // duplicate point written below - allocating only holeNCoords+1 slots left that last write
        // one element past the end of the handle, corrupting adjacent heap memory (a delayed,
        // hard-to-diagnose C0000374 crash reported by Archicad much later during an unrelated
        // commit - confirmed live by reproducing it on a plain hole-add).
        insMemo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((holeNCoords + 2) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
        for (Int32 i = 0; i < holeNCoords; ++i) {
            (*insMemo.coords)[i + 1] = Get2DCoordinateFromObjectState (holePolygonOutline[i]);
        }
        (*insMemo.coords)[holeNCoords + 1] = (*insMemo.coords)[1];
        insMemo.pends = reinterpret_cast<Int32**> (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
        (*insMemo.pends)[0] = 0;
        (*insMemo.pends)[1] = holeNCoords + 1;
        if (!holePolygonArcs.IsEmpty ()) {
            const GS::Array<API_PolyArc> holeArcs = GetPolyArcs (holePolygonArcs, 1);
            insMemo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (holeArcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
            for (UIndex i = 0; i < holeArcs.GetSize (); ++i) {
                (*insMemo.parcs)[i] = holeArcs[i];
            }
        }
        GSErrCode err = TAPIR_Polygon_InsertSubPoly (&memo, &insMemo);
        if (err != NoError) {
            return "Failed to add a slab hole.";
        }
    }

    // edgeTrims/sideMaterials are not touched by the Insert/Delete primitives above (per their own
    // documentation - "other memo handles are not touched"), so they are resized fresh to match the
    // final coordinate count. edgeTrims is a plain Handle (BMKillHandle+BMAllocateHandle is safe);
    // sideMaterials is a raw Ptr, and BMReallocPtr on a GetMemo-provided one reliably corrupted the
    // heap (issue #452's original crash report) - so it is freed and reallocated fresh too, never
    // resized in place. Despite the DevKit's own Do_Poly_InsertNode/DeleteNode reference example
    // treating this as unnecessary for API_SlabID and using APIMemoMask_Polygon alone for
    // ACAPI_Element_ChangeMemo, using that narrower mask here reproducibly failed with
    // APIERR_BADPARS on this exact hole-removal case - live-confirmed; the combined mask plus this
    // rebuild is what actually works for hole add/remove, multi-hole, and arcs.
    const Int32 finalNCoords = (Int32) (BMGetHandleSize ((GSHandle) memo.coords) / sizeof (API_Coord)) - 1;
    if (memo.edgeTrims != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.edgeTrims));
    }
    memo.edgeTrims = reinterpret_cast<API_EdgeTrim**> (BMAllocateHandle ((finalNCoords + 1) * sizeof (API_EdgeTrim), ALLOCATE_CLEAR, 0));
    if (memo.sideMaterials != nullptr) {
        BMKillPtr (reinterpret_cast<GSPtr*> (&memo.sideMaterials));
    }
    memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*> (BMAllocatePtr ((finalNCoords + 1) * sizeof (API_OverriddenAttribute), ALLOCATE_CLEAR, 0));
    for (Int32 i = 1; i <= finalNCoords; ++i) {
        (*memo.edgeTrims)[i].sideType = APIEdgeTrim_Vertical;
        memo.sideMaterials[i] = sideMat;
    }

    return {};
}

namespace {

void AddAdditionalPolyToMemo (
    const GS::Array<GS::ObjectState>& coords,
    const GS::Array<GS::ObjectState>& arcs,
    Int32& iCoord,
    Int32& iArc,
    Int32& iPends,
    API_ElementMemo& memo)
{
    const Int32 startIndex = iCoord;
    for (const GS::ObjectState& coord : coords) {
        (*memo.additionalPolyCoords)[iCoord++] = Get2DCoordinateFromObjectState (coord);
    }

    (*memo.additionalPolyCoords)[iCoord] = (*memo.additionalPolyCoords)[startIndex];
    (*memo.additionalPolyPends)[iPends++] = iCoord;
    ++iCoord;

    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, startIndex);
    for (const API_PolyArc& polyArc : polyArcs) {
        (*memo.additionalPolyParcs)[iArc++] = polyArc;
    }
}

GS::Optional<GS::UniString> BuildRoofMemoFromGeometry (
    API_Element& element,
    API_ElementMemo& memo,
    GS::Array<GS::ObjectState>& polygonOutline,
    const GS::Array<GS::ObjectState>& polygonArcs,
    const GS::Array<GS::ObjectState>& holes)
{
    if (polygonOutline.GetSize () < 3) {
        return "'polygonOutline' must contain at least 3 coordinates.";
    }

    if (IsSame2DCoordinate (polygonOutline.GetFirst (), polygonOutline.GetLast ())) {
        polygonOutline.Pop ();
    }

    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    element.roof.u.polyRoof.pivotPolygon.nCoords = polygonOutline.GetSize () + 1;
    element.roof.u.polyRoof.pivotPolygon.nSubPolys = 1;
    element.roof.u.polyRoof.pivotPolygon.nArcs = polygonArcs.GetSize ();

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            element.roof.u.polyRoof.pivotPolygon.nCoords += holePolygonOutline.GetSize () + 1;
            ++element.roof.u.polyRoof.pivotPolygon.nSubPolys;
            element.roof.u.polyRoof.pivotPolygon.nArcs += holePolygonArcs.GetSize ();
        }
    }

    // GetDefaults typically leaves the roof pivot-polygon memo handles null, and
    // BMReallocHandle does not allocate from a null handle (it returns null), which would
    // crash AddAdditionalPolyToMemo with a null dereference. Allocate fresh when null
    // (same fix as the slab path); only resize an existing handle with BMReallocHandle.
    const Int32 roofNCoords   = element.roof.u.polyRoof.pivotPolygon.nCoords;
    const Int32 roofNSubPolys = element.roof.u.polyRoof.pivotPolygon.nSubPolys;
    const Int32 roofNArcs     = element.roof.u.polyRoof.pivotPolygon.nArcs;
    memo.additionalPolyCoords = reinterpret_cast<API_Coord**> (memo.additionalPolyCoords == nullptr
        ? BMAllocateHandle ((roofNCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0)
        : BMReallocHandle (reinterpret_cast<GSHandle> (memo.additionalPolyCoords), (roofNCoords + 1) * sizeof (API_Coord), REALLOC_CLEAR, 0));
    memo.additionalPolyPends = reinterpret_cast<Int32**> (memo.additionalPolyPends == nullptr
        ? BMAllocateHandle ((roofNSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0)
        : BMReallocHandle (reinterpret_cast<GSHandle> (memo.additionalPolyPends), (roofNSubPolys + 1) * sizeof (Int32), REALLOC_CLEAR, 0));
    if (roofNArcs > 0) {
        memo.additionalPolyParcs = reinterpret_cast<API_PolyArc**> (memo.additionalPolyParcs == nullptr
            ? BMAllocateHandle (roofNArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.additionalPolyParcs), roofNArcs * sizeof (API_PolyArc), REALLOC_CLEAR, 0));
    }

    Int32 iCoord = 1;
    Int32 iArc = 0;
    Int32 iPends = 1;
    AddAdditionalPolyToMemo (polygonOutline, polygonArcs, iCoord, iArc, iPends, memo);

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            AddAdditionalPolyToMemo (holePolygonOutline, holePolygonArcs, iCoord, iArc, iPends, memo);
        }
    }

    // A multi-plane roof (API_PolyRoofData) needs a pivot polygon (set above via the
    // additionalPoly* memo) and a contour polygon. With API_OffsetOverhang the contour
    // polygon is derived automatically by offsetting the pivot polygon (see
    // Do_CreatePolyRoof in the DevKit's Element_Test example), so no contour memo has to
    // be built here. The tool default can be API_ContourOverhang, which would require
    // explicit contour data - force the offset mode instead; eavesOverHang keeps the tool
    // default unless the optional 'eavesOverhang' parameter was applied.
    element.roof.u.polyRoof.overHangType = API_OffsetOverhang;

    return {};
}

GS::Optional<GS::UniString> BuildPlaneRoofMemoFromGeometry (
    API_Element& element,
    API_ElementMemo& memo,
    GS::Array<GS::ObjectState>& polygonOutline,
    const GS::Array<GS::ObjectState>& polygonArcs,
    const GS::Array<GS::ObjectState>& holes)
{
    if (polygonOutline.GetSize () < 3) {
        return "'polygonOutline' must contain at least 3 coordinates.";
    }

    if (IsSame2DCoordinate (polygonOutline.GetFirst (), polygonOutline.GetLast ())) {
        polygonOutline.Pop ();
    }

    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    element.roof.u.planeRoof.poly.nCoords = polygonOutline.GetSize () + 1;
    element.roof.u.planeRoof.poly.nSubPolys = 1;
    element.roof.u.planeRoof.poly.nArcs = polygonArcs.GetSize ();

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            element.roof.u.planeRoof.poly.nCoords += holePolygonOutline.GetSize () + 1;
            ++element.roof.u.planeRoof.poly.nSubPolys;
            element.roof.u.planeRoof.poly.nArcs += holePolygonArcs.GetSize ();
        }
    }

    // GetDefaults typically leaves the roof polygon memo handles null; BMReallocHandle
    // does not allocate from a null handle, so allocate fresh when null (same pattern as
    // the slab path). The edge trims / side materials are optional for roofs and left out.
    const Int32 nCoords   = element.roof.u.planeRoof.poly.nCoords;
    const Int32 nSubPolys = element.roof.u.planeRoof.poly.nSubPolys;
    const Int32 nArcs     = element.roof.u.planeRoof.poly.nArcs;
    memo.coords = reinterpret_cast<API_Coord**> (memo.coords == nullptr
        ? BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0)
        : BMReallocHandle (reinterpret_cast<GSHandle> (memo.coords), (nCoords + 1) * sizeof (API_Coord), REALLOC_CLEAR, 0));
    memo.pends = reinterpret_cast<Int32**> (memo.pends == nullptr
        ? BMAllocateHandle ((nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0)
        : BMReallocHandle (reinterpret_cast<GSHandle> (memo.pends), (nSubPolys + 1) * sizeof (Int32), REALLOC_CLEAR, 0));
    if (nArcs > 0) {
        memo.parcs = reinterpret_cast<API_PolyArc**> (memo.parcs == nullptr
            ? BMAllocateHandle (nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.parcs), nArcs * sizeof (API_PolyArc), REALLOC_CLEAR, 0));
    }

    Int32 iCoord = 1;
    Int32 iArc = 0;
    Int32 iPends = 1;
    AddPolyToMemo (polygonOutline, polygonArcs, iCoord, iArc, iPends, memo, nullptr, nullptr, false);

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            AddPolyToMemo (holePolygonOutline, holePolygonArcs, iCoord, iArc, iPends, memo, nullptr, nullptr, false);
        }
    }

    return {};
}

GS::Optional<GS::UniString> ApplyWallStructure (
    API_Element& element,
    API_Element* mask,
    const GS::ObjectState& details,
    bool& changed)
{
    StructureSelection selection;
    auto error = ParseStructureSelection (details, true, true, selection);
    if (error.HasValue ()) {
        return error;
    }

    if (selection.kind == StructureSelectionKind::Unspecified) {
        return {};
    }

    switch (selection.kind) {
        case StructureSelectionKind::Basic:
            element.wall.modelElemStructureType = API_BasicStructure;
            if (selection.buildingMaterial != APIInvalidAttributeIndex) {
                element.wall.buildingMaterial = selection.buildingMaterial;
            }
            element.wall.composite = APIInvalidAttributeIndex;
            element.wall.profileAttr = APIInvalidAttributeIndex;
            if (element.wall.type == APIWtyp_Poly) {
                element.wall.type = APIWtyp_Normal;
            }
            break;
        case StructureSelectionKind::Composite:
            element.wall.modelElemStructureType = API_CompositeStructure;
            if (selection.composite != APIInvalidAttributeIndex) {
                element.wall.composite = selection.composite;
            }
            element.wall.profileAttr = APIInvalidAttributeIndex;
            if (element.wall.type == APIWtyp_Poly) {
                element.wall.type = APIWtyp_Normal;
            }
            break;
        case StructureSelectionKind::Profile:
            element.wall.modelElemStructureType = API_ProfileStructure;
            if (selection.profile != APIInvalidAttributeIndex) {
                element.wall.profileAttr = selection.profile;
            }
            // Note: unlike Basic/Composite above, wall.type (the plan outline shape) is left
            // untouched here - it is orthogonal to modelElemStructureType (the cross section),
            // same relationship as profileType/slantAlpha for a Slanted/Trapez wall. Forcing
            // APIWtyp_Poly here previously made ACAPI_Element_Create/_Change fail outright,
            // since a Poly wall needs an actual polygon memo that neither CreateWalls nor
            // ModifyWalls builds.
            break;
        case StructureSelectionKind::Unspecified:
            break;
    }

    if (mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET ((*mask), API_WallType, modelElemStructureType);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_WallType, buildingMaterial);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_WallType, composite);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_WallType, profileAttr);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_WallType, type);
    }
    changed = true;
    return {};
}

GS::Optional<GS::UniString> ApplyRoofStructure (
    API_Element& element,
    API_Element* mask,
    const GS::ObjectState& details,
    bool& changed)
{
    StructureSelection selection;
    auto error = ParseStructureSelection (details, true, false, selection);
    if (error.HasValue ()) {
        return error;
    }

    if (selection.kind == StructureSelectionKind::Unspecified) {
        return {};
    }

    switch (selection.kind) {
        case StructureSelectionKind::Basic:
            element.roof.shellBase.modelElemStructureType = API_BasicStructure;
            if (selection.buildingMaterial != APIInvalidAttributeIndex) {
                element.roof.shellBase.buildingMaterial = selection.buildingMaterial;
            }
            element.roof.shellBase.composite = APIInvalidAttributeIndex;
            break;
        case StructureSelectionKind::Composite:
            element.roof.shellBase.modelElemStructureType = API_CompositeStructure;
            if (selection.composite != APIInvalidAttributeIndex) {
                element.roof.shellBase.composite = selection.composite;
            }
            break;
        case StructureSelectionKind::Profile:
        case StructureSelectionKind::Unspecified:
            break;
    }

    if (mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, shellBase.modelElemStructureType);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, shellBase.buildingMaterial);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, shellBase.composite);
    }
    changed = true;
    return {};
}

GS::Optional<GS::UniString> ApplySlabStructure (
    API_Element& element,
    API_Element* mask,
    const GS::ObjectState& details,
    bool& changed)
{
    StructureSelection selection;
    auto error = ParseStructureSelection (details, true, false, selection);
    if (error.HasValue ()) {
        return error;
    }

    if (selection.kind == StructureSelectionKind::Unspecified) {
        return {};
    }

    switch (selection.kind) {
        case StructureSelectionKind::Basic:
            element.slab.modelElemStructureType = API_BasicStructure;
            if (selection.buildingMaterial != APIInvalidAttributeIndex) {
                element.slab.buildingMaterial = selection.buildingMaterial;
            }
            element.slab.composite = APIInvalidAttributeIndex;
            break;
        case StructureSelectionKind::Composite:
            element.slab.modelElemStructureType = API_CompositeStructure;
            if (selection.composite != APIInvalidAttributeIndex) {
                element.slab.composite = selection.composite;
            }
            break;
        case StructureSelectionKind::Profile:
        case StructureSelectionKind::Unspecified:
            break;
    }

    if (mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET ((*mask), API_SlabType, modelElemStructureType);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_SlabType, buildingMaterial);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_SlabType, composite);
    }
    changed = true;
    return {};
}

bool ApplyWallDetails (API_Element& element, API_Element& mask, const GS::ObjectState& details)
{
    bool changed = false;
    GS::UniString geometryType;
    if (details.Get ("geometryType", geometryType)) {
        // Only Straight/Trapezoid are settable here: Polygonal would need a full outline/arc
        // memo (like ModifySlabs' polygonOutline), which the wall's Modify command does not
        // take as input at all yet. This is the plan outline shape (wall.type), unrelated to
        // profileType/slantAlpha/slantBeta below (the cross section shape).
        if (geometryType == "Trapezoid") {
            element.wall.type = APIWtyp_Trapez;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, type);
            changed = true;
        } else if (geometryType == "Straight") {
            element.wall.type = APIWtyp_Normal;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, type);
            changed = true;
        }
    }
    auto begCoordinate = GetOptionalCoordinate2D (details, "begCoordinate");
    if (begCoordinate.HasValue ()) {
        element.wall.begC = begCoordinate.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, begC);
        changed = true;
    }
    auto endCoordinate = GetOptionalCoordinate2D (details, "endCoordinate");
    if (endCoordinate.HasValue ()) {
        element.wall.endC = endCoordinate.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, endC);
        changed = true;
    }
    auto arcAngle = GetOptionalDouble (details, "arcAngle");
    if (arcAngle.HasValue ()) {
        element.wall.angle = arcAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, angle);
        changed = true;
    }
    auto height = GetOptionalDouble (details, "height");
    if (height.HasValue ()) {
        element.wall.height = height.Get ();
        element.wall.relativeTopStory = 0;
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, height);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, relativeTopStory);
        changed = true;
    }
    auto offset = GetOptionalDouble (details, "offset");
    if (offset.HasValue ()) {
        element.wall.offset = offset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, offset);
        changed = true;
    }
    auto thickness = GetOptionalDouble (details, "thickness");
    if (thickness.HasValue ()) {
        element.wall.thickness = thickness.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, thickness);
        changed = true;
    }
    auto bottomOffset = GetOptionalDouble (details, "bottomOffset");
    if (bottomOffset.HasValue ()) {
        element.wall.bottomOffset = bottomOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, bottomOffset);
        changed = true;
    }
    GS::UniString referenceLineLocationStr;
    if (details.Get ("referenceLineLocation", referenceLineLocationStr)) {
        element.wall.referenceLineLocation = WallReferenceLineLocationFromString (referenceLineLocationStr);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, referenceLineLocation);
        changed = true;
    }
    GS::UniString profileTypeStr;
    if (details.Get ("profileType", profileTypeStr)) {
        // Distinct from geometryType/wall.type above: this is the cross section shape
        // (APISect_*), not the plan outline. slantAlpha/slantBeta only have an effect once
        // this is set to Slanted (Poly is not settable here - it needs a profile attribute
        // wired through a separate mechanism, out of scope).
        if (profileTypeStr == "Slanted") {
            element.wall.profileType = APISect_Slanted;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, profileType);
            changed = true;
        } else if (profileTypeStr == "Trapez") {
            element.wall.profileType = APISect_Trapez;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, profileType);
            changed = true;
        } else if (profileTypeStr == "Normal") {
            element.wall.profileType = APISect_Normal;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, profileType);
            changed = true;
        }
    }
    auto slantAlpha = GetOptionalDouble (details, "slantAlpha");
    if (slantAlpha.HasValue ()) {
        element.wall.slantAlpha = slantAlpha.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, slantAlpha);
        changed = true;
    }
    auto slantBeta = GetOptionalDouble (details, "slantBeta");
    if (slantBeta.HasValue ()) {
        element.wall.slantBeta = slantBeta.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, slantBeta);
        changed = true;
    }
    auto topOffset = GetOptionalDouble (details, "topOffset");
    if (topOffset.HasValue ()) {
        element.wall.topOffset = topOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, topOffset);
        changed = true;
    }
    auto relativeTopStory = GetOptionalDouble (details, "relativeTopStory");
    if (relativeTopStory.HasValue ()) {
        element.wall.relativeTopStory = static_cast<short> (relativeTopStory.Get ());
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, relativeTopStory);
        changed = true;
    }
    GS::UniString zoneRelStr;
    if (details.Get ("zoneRel", zoneRelStr)) {
        element.wall.zoneRel = ZoneRelFromString (zoneRelStr);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, zoneRel);
        changed = true;
    }
    GS::ObjectState visibilityOs;
    if (details.Get ("visibility", visibilityOs)) {
        element.wall.visibility = GetStoryVisibilityFromObjectState (visibilityOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, visibility);
        changed = true;
        if (!details.Contains ("isAutoOnStoryVisibility")) {
            // isAutoOnStoryVisibility defaults to true on a wall, in which case Archicad
            // recomputes 'visibility' from the wall's vertical extent and silently discards
            // whatever was just requested above - explicitly setting 'visibility' only makes
            // sense once auto mode is off, so turn it off unless the caller says otherwise.
            element.wall.isAutoOnStoryVisibility = false;
            ACAPI_ELEMENT_MASK_SET (mask, API_WallType, isAutoOnStoryVisibility);
        }
    }
    bool isAutoOnStoryVisibility = false;
    if (details.Get ("isAutoOnStoryVisibility", isAutoOnStoryVisibility)) {
        element.wall.isAutoOnStoryVisibility = isAutoOnStoryVisibility;
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, isAutoOnStoryVisibility);
        changed = true;
    }
    GS::ObjectState referenceMaterialOs;
    if (details.Get ("referenceMaterial", referenceMaterialOs)) {
        element.wall.refMat = GetOverriddenMaterialFromObjectState (referenceMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, refMat);
        changed = true;
    }
    GS::ObjectState oppositeMaterialOs;
    if (details.Get ("oppositeMaterial", oppositeMaterialOs)) {
        element.wall.oppMat = GetOverriddenMaterialFromObjectState (oppositeMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, oppMat);
        changed = true;
    }
    GS::ObjectState sideMaterialOs;
    if (details.Get ("sideMaterial", sideMaterialOs)) {
        element.wall.sidMat = GetOverriddenMaterialFromObjectState (sideMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, sidMat);
        changed = true;
    }
#ifdef ServerMainVers_2700
    GS::ObjectState cutFillPenOs;
    if (details.Get ("cutFillPen", cutFillPenOs)) {
        element.wall.cutFillPen = GetOverriddenPenFromObjectState (cutFillPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, cutFillPen);
        changed = true;
    }
    GS::ObjectState cutFillBackgroundPenOs;
    if (details.Get ("cutFillBackgroundPen", cutFillBackgroundPenOs)) {
        element.wall.cutFillBackgroundPen = GetOverriddenPenFromObjectState (cutFillBackgroundPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_WallType, cutFillBackgroundPen);
        changed = true;
    }
#endif
    return changed;
}

GS::Optional<GS::UniString> ApplyRoofLevels (API_Element& element, API_Element* mask, const GS::ObjectState& details, bool& changed)
{
    GS::Array<GS::ObjectState> roofLevels;
    if (!details.Get ("levels", roofLevels)) {
        return {};
    }

    if (roofLevels.IsEmpty () || roofLevels.GetSize () > 16) {
        return "'levels' must contain between 1 and 16 level definitions.";
    }

    double previousHeight = -1.0e18;
    element.roof.u.polyRoof.levelNum = static_cast<short> (roofLevels.GetSize ());
    for (UIndex i = 0; i < roofLevels.GetSize (); ++i) {
        double levelHeight = 0.0;
        double levelAngle = 0.0;
        if (!roofLevels[i].Get ("levelHeight", levelHeight) || !roofLevels[i].Get ("levelAngle", levelAngle)) {
            return "Each roof level must contain 'levelHeight' and 'levelAngle'.";
        }
        if (levelAngle <= 0.0) {
            return "'levelAngle' must be greater than zero.";
        }
        if (levelHeight < previousHeight) {
            return "'levels' must be ordered by non-decreasing 'levelHeight'.";
        }

        element.roof.u.polyRoof.levelData[i].levelHeight = levelHeight;
        element.roof.u.polyRoof.levelData[i].levelAngle = levelAngle;
        previousHeight = levelHeight;
    }

    if (mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, u.polyRoof.levelNum);
        ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, u.polyRoof.levelData);
    }
    changed = true;
    return {};
}

bool ApplySlabDetails (API_Element& element, API_Element& mask, const GS::ObjectState& details, const Stories& stories)
{
    bool changed = false;
    auto thickness = GetOptionalDouble (details, "thickness");
    if (thickness.HasValue ()) {
        element.slab.thickness = thickness.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, thickness);
        changed = true;
    }

    auto zCoordinate = GetOptionalDouble (details, "zCoordinate");

    if (zCoordinate.HasValue ()) {
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (zCoordinate.Get (), stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.slab.level = floorIndexAndOffset.second;
        ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, level);
        changed = true;
    }

    GS::UniString referencePlaneLocationStr;
    if (details.Get ("referencePlaneLocation", referencePlaneLocationStr)) {
        element.slab.referencePlaneLocation = SlabReferencePlaneLocationFromString (referencePlaneLocationStr);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, referencePlaneLocation);
        changed = true;
    }

    GS::ObjectState topMaterialOs;
    if (details.Get ("topMaterial", topMaterialOs)) {
        element.slab.topMat = GetOverriddenMaterialFromObjectState (topMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, topMat);
        changed = true;
    }
    GS::ObjectState sideMaterialOs;
    if (details.Get ("sideMaterial", sideMaterialOs)) {
        element.slab.sideMat = GetOverriddenMaterialFromObjectState (sideMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, sideMat);
        changed = true;
    }
    GS::ObjectState bottomMaterialOs;
    if (details.Get ("bottomMaterial", bottomMaterialOs)) {
        element.slab.botMat = GetOverriddenMaterialFromObjectState (bottomMaterialOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, botMat);
        changed = true;
    }
#ifdef ServerMainVers_2700
    GS::ObjectState cutFillPenOs;
    if (details.Get ("cutFillPen", cutFillPenOs)) {
        element.slab.cutFillPen = GetOverriddenPenFromObjectState (cutFillPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, cutFillPen);
        changed = true;
    }
    GS::ObjectState cutFillBackgroundPenOs;
    if (details.Get ("cutFillBackgroundPen", cutFillBackgroundPenOs)) {
        element.slab.cutFillBackgroundPen = GetOverriddenPenFromObjectState (cutFillBackgroundPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, cutFillBackgroundPen);
        changed = true;
    }
#endif

    GS::ObjectState floorFillOs;
    if (details.Get ("floorFill", floorFillOs)) {
        floorFillOs.Get ("use", element.slab.useFloorFill);
        Int32 foregroundPen = 0;
        if (floorFillOs.Get ("foregroundPen", foregroundPen)) {
            element.slab.floorFillPen = static_cast<short> (foregroundPen);
        }
        Int32 backgroundPen = 0;
        if (floorFillOs.Get ("backgroundPen", backgroundPen)) {
            element.slab.floorFillBGPen = static_cast<short> (backgroundPen);
        }
        GS::ObjectState fillIdOs;
        if (floorFillOs.Get ("fillId", fillIdOs)) {
            element.slab.floorFillInd = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (fillIdOs));
        }
        floorFillOs.Get ("use3DHatching", element.slab.use3DHatching);
        GS::ObjectState orientationOs;
        if (floorFillOs.Get ("orientation", orientationOs)) {
            element.slab.hatchOrientation = GetHatchOrientationFromObjectState (orientationOs);
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, useFloorFill);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, floorFillPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, floorFillBGPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, floorFillInd);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, use3DHatching);
        ACAPI_ELEMENT_MASK_SET (mask, API_SlabType, hatchOrientation);
        changed = true;
    }

    return changed;
}

GS::Optional<GS::UniString> ApplyRoofDetails (
    API_Element& element,
    API_Element* mask,
    const GS::ObjectState& details,
    const Stories& stories,
    bool& changed)
{
    auto thickness = GetOptionalDouble (details, "thickness");
    if (thickness.HasValue ()) {
        element.roof.shellBase.thickness = thickness.Get ();
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, shellBase.thickness);
        }
        changed = true;
    }

    auto level = GetOptionalDouble (details, "level");

    if (level.HasValue ()) {
        const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (details, "floorIndex", level.Get (), stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.roof.shellBase.level = floorIndexAndOffset.second;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_Elem_Head, floorInd);
            ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, shellBase.level);
        }
        changed = true;
    }

    auto eavesOverhang = GetOptionalDouble (details, "eavesOverhang");

    if (eavesOverhang.HasValue ()) {
        element.roof.u.polyRoof.overHangType = API_OffsetOverhang;
        element.roof.u.polyRoof.eavesOverHang = eavesOverhang.Get ();
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, u.polyRoof.overHangType);
            ACAPI_ELEMENT_MASK_SET ((*mask), API_RoofType, u.polyRoof.eavesOverHang);
        }
        changed = true;
    }

    return ApplyRoofLevels (element, mask, details, changed);
}

bool ApplyColumnDetails (API_Element& element, API_Element& mask, const GS::ObjectState& details, const Stories& stories)
{
    bool changed = false;
    auto origin = GetOptionalCoordinate2D (details, "origin");
    if (origin.HasValue ()) {
        element.column.origoPos = origin.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, origoPos);
        changed = true;
    }
    auto zCoordinate = GetOptionalDouble (details, "zCoordinate");
    if (zCoordinate.HasValue ()) {
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (zCoordinate.Get (), stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.column.bottomOffset = floorIndexAndOffset.second;
        ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, bottomOffset);
        changed = true;
    }
    auto height = GetOptionalDouble (details, "height");
    if (height.HasValue ()) {
        element.column.height = height.Get ();
        element.column.relativeTopStory = 0;
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, height);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, relativeTopStory);
        changed = true;
    }
    auto bottomOffset = GetOptionalDouble (details, "bottomOffset");
    if (bottomOffset.HasValue ()) {
        element.column.bottomOffset = bottomOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, bottomOffset);
        changed = true;
    }
    auto axisRotationAngle = GetOptionalDouble (details, "axisRotationAngle");
    if (axisRotationAngle.HasValue ()) {
        element.column.axisRotationAngle = axisRotationAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, axisRotationAngle);
        changed = true;
    }
    GS::UniString coreAnchor;
    if (details.Get ("coreAnchor", coreAnchor)) {
        element.column.coreAnchor = static_cast<short> (AnchorIdFromString (coreAnchor));
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coreAnchor);
        changed = true;
    }
    bool isSlanted = false;
    if (details.Get ("isSlanted", isSlanted)) {
        element.column.isSlanted = isSlanted;
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, isSlanted);
        changed = true;
    }
    auto slantAngle = GetOptionalDouble (details, "slantAngle");
    if (slantAngle.HasValue ()) {
        element.column.slantAngle = slantAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, slantAngle);
        // Same as for beams: without isSlanted the column stays vertical and the angle is
        // discarded. An explicit isSlanted (handled above) wins over this default.
        bool explicitIsSlanted = false;
        if (!details.Get ("isSlanted", explicitIsSlanted)) {
            element.column.isSlanted = (slantAngle.Get () != 0.0);
            ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, isSlanted);
        }
        changed = true;
    }
    auto slantDirectionAngle = GetOptionalDouble (details, "slantDirectionAngle");
    if (slantDirectionAngle.HasValue ()) {
        element.column.slantDirectionAngle = slantDirectionAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, slantDirectionAngle);
        changed = true;
    }
    bool isFlipped = false;
    if (details.Get ("isFlipped", isFlipped)) {
        element.column.isFlipped = isFlipped;
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, isFlipped);
        changed = true;
    }
    bool wrapping = false;
    if (details.Get ("wrapping", wrapping)) {
        element.column.wrapping = wrapping;
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, wrapping);
        changed = true;
    }
    auto topOffset = GetOptionalDouble (details, "topOffset");
    if (topOffset.HasValue ()) {
        element.column.topOffset = topOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, topOffset);
        changed = true;
    }
    auto relativeTopStory = GetOptionalDouble (details, "relativeTopStory");
    if (relativeTopStory.HasValue ()) {
        element.column.relativeTopStory = static_cast<short> (relativeTopStory.Get ());
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, relativeTopStory);
        changed = true;
    }
#ifdef ServerMainVers_2700
    GS::ObjectState cutFillPenOs;
    if (details.Get ("cutFillPen", cutFillPenOs)) {
        element.column.cutFillPen = GetOverriddenPenFromObjectState (cutFillPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, cutFillPen);
        changed = true;
    }
    GS::ObjectState cutFillBackgroundPenOs;
    if (details.Get ("cutFillBackgroundPen", cutFillBackgroundPenOs)) {
        element.column.cutFillBackgroundPen = GetOverriddenPenFromObjectState (cutFillBackgroundPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, cutFillBackgroundPen);
        changed = true;
    }
#endif
    GS::ObjectState coverFillOs;
    if (details.Get ("coverFill", coverFillOs)) {
        coverFillOs.Get ("use", element.column.useCoverFill);
        coverFillOs.Get ("useFromSurface", element.column.useCoverFillFromSurface);
        coverFillOs.Get ("orientationComesFrom3D", element.column.coverFillOrientationComesFrom3D);
        GS::ObjectState fillIdOs;
        if (coverFillOs.Get ("fillId", fillIdOs)) {
            element.column.coverFillType = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (fillIdOs));
        }
        Int32 foregroundPen = 0;
        if (coverFillOs.Get ("foregroundPen", foregroundPen)) {
            element.column.coverFillForegroundPen = static_cast<short> (foregroundPen);
        }
        Int32 backgroundPen = 0;
        if (coverFillOs.Get ("backgroundPen", backgroundPen)) {
            element.column.coverFillBackgroundPen = static_cast<short> (backgroundPen);
        }
        GS::UniString transformationTypeStr;
        if (coverFillOs.Get ("transformationType", transformationTypeStr)) {
            element.column.coverFillTransformationType = CoverFillTransformationTypeFromString (transformationTypeStr);
        }
        GS::ObjectState transformationOs;
        if (coverFillOs.Get ("transformation", transformationOs)) {
            element.column.coverFillTransformation = GetCoverFillTransformationFromObjectState (transformationOs);
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, useCoverFill);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, useCoverFillFromSurface);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillOrientationComesFrom3D);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillType);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillForegroundPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillBackgroundPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillTransformationType);
        ACAPI_ELEMENT_MASK_SET (mask, API_ColumnType, coverFillTransformation);
        changed = true;
    }
    return changed;
}

bool ApplyBeamDetails (API_Element& element, API_Element& mask, const GS::ObjectState& details)
{
    bool changed = false;
    auto begCoordinate = GetOptionalCoordinate2D (details, "begCoordinate");
    if (begCoordinate.HasValue ()) {
        element.beam.begC = begCoordinate.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, begC);
        changed = true;
    }
    auto endCoordinate = GetOptionalCoordinate2D (details, "endCoordinate");
    if (endCoordinate.HasValue ()) {
        element.beam.endC = endCoordinate.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, endC);
        changed = true;
    }
    auto level = GetOptionalDouble (details, "level");
    if (level.HasValue ()) {
        element.beam.level = level.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, level);
        changed = true;
    }
    auto offset = GetOptionalDouble (details, "offset");
    if (offset.HasValue ()) {
        element.beam.offset = offset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, offset);
        changed = true;
    }
    auto slantAngle = GetOptionalDouble (details, "slantAngle");
    if (slantAngle.HasValue ()) {
        element.beam.slantAngle = slantAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, slantAngle);
        // Archicad keeps the beam in "Horizontal" mode and discards the angle unless isSlanted
        // is set too, so a slantAngle on its own used to report success and change nothing
        // (#508). Only derived when the caller does not state isSlanted explicitly - that one
        // is applied below and wins.
        bool explicitIsSlanted = false;
        if (!details.Get ("isSlanted", explicitIsSlanted)) {
            element.beam.isSlanted = (slantAngle.Get () != 0.0);
            ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, isSlanted);
        }
        changed = true;
    }
    auto arcAngle = GetOptionalDouble (details, "arcAngle");
    if (arcAngle.HasValue ()) {
        element.beam.curveAngle = arcAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, curveAngle);
        changed = true;
    }
    auto curveHeight = GetOptionalDouble (details, "verticalCurveHeight");
    if (curveHeight.HasValue ()) {
        element.beam.verticalCurveHeight = curveHeight.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, verticalCurveHeight);
        changed = true;
    }
    GS::UniString beamShapeStr;
    if (details.Get ("beamShape", beamShapeStr)) {
        if (beamShapeStr == "HorizontallyCurved") {
            element.beam.beamShape = API_HorizontallyCurvedBeam;
        } else if (beamShapeStr == "VerticallyCurved") {
            element.beam.beamShape = API_VerticallyCurvedBeam;
        } else {
            element.beam.beamShape = API_StraightBeam;
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, beamShape);
        changed = true;
    }
    bool isSlanted = false;
    if (details.Get ("isSlanted", isSlanted)) {
        element.beam.isSlanted = isSlanted;
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, isSlanted);
        changed = true;
    }
    bool isFlipped = false;
    if (details.Get ("isFlipped", isFlipped)) {
        element.beam.isFlipped = isFlipped;
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, isFlipped);
        changed = true;
    }
    auto profileAngle = GetOptionalDouble (details, "profileAngle");
    if (profileAngle.HasValue ()) {
        element.beam.profileAngle = profileAngle.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, profileAngle);
        changed = true;
    }
    GS::UniString anchorPointStr;
    if (details.Get ("anchorPoint", anchorPointStr)) {
        element.beam.anchorPoint = static_cast<short> (AnchorIdFromString (anchorPointStr));
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, anchorPoint);
        changed = true;
    }
#ifdef ServerMainVers_2700
    GS::ObjectState cutFillPenOs;
    if (details.Get ("cutFillPen", cutFillPenOs)) {
        element.beam.cutFillPen = GetOverriddenPenFromObjectState (cutFillPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, cutFillPen);
        changed = true;
    }
    GS::ObjectState cutFillBackgroundPenOs;
    if (details.Get ("cutFillBackgroundPen", cutFillBackgroundPenOs)) {
        element.beam.cutFillBackgroundPen = GetOverriddenPenFromObjectState (cutFillBackgroundPenOs);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, cutFillBackgroundPen);
        changed = true;
    }
#endif
    GS::ObjectState coverFillOs;
    if (details.Get ("coverFill", coverFillOs)) {
        coverFillOs.Get ("use", element.beam.useCoverFill);
        coverFillOs.Get ("useFromSurface", element.beam.useCoverFillFromSurface);
        coverFillOs.Get ("orientationComesFrom3D", element.beam.coverFillOrientationComesFrom3D);
        GS::ObjectState fillIdOs;
        if (coverFillOs.Get ("fillId", fillIdOs)) {
            element.beam.coverFillType = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (fillIdOs));
        }
        Int32 foregroundPen = 0;
        if (coverFillOs.Get ("foregroundPen", foregroundPen)) {
            element.beam.coverFillForegroundPen = static_cast<short> (foregroundPen);
        }
        Int32 backgroundPen = 0;
        if (coverFillOs.Get ("backgroundPen", backgroundPen)) {
            element.beam.coverFillBackgroundPen = static_cast<short> (backgroundPen);
        }
        GS::UniString transformationTypeStr;
        if (coverFillOs.Get ("transformationType", transformationTypeStr)) {
            element.beam.coverFillTransformationType = CoverFillTransformationTypeFromString (transformationTypeStr);
        }
        GS::ObjectState transformationOs;
        if (coverFillOs.Get ("transformation", transformationOs)) {
            element.beam.coverFillTransformation = GetCoverFillTransformationFromObjectState (transformationOs);
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, useCoverFill);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, useCoverFillFromSurface);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillOrientationComesFrom3D);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillType);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillForegroundPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillBackgroundPen);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillTransformationType);
        ACAPI_ELEMENT_MASK_SET (mask, API_BeamType, coverFillTransformation);
        changed = true;
    }
    return changed;
}

static bool ApplyColumnSectionToMemo (API_Guid elemGuid, const GS::ObjectState& details)
{
    auto width = GetOptionalDouble (details, "width");
    auto depth = GetOptionalDouble (details, "depth");
    bool circleBased = false;
    const bool hasCircleBased = details.Get ("circleBased", circleBased);
    bool isWidthAndHeightLinked = false;
    const bool hasIsWidthAndHeightLinked = details.Get ("isWidthAndHeightLinked", isWidthAndHeightLinked);
    const GS::ObjectState* buildingMaterialIdOs = details.Get ("buildingMaterialId");
    const GS::ObjectState* profileIdOs = details.Get ("profileId");
    if (!width.HasValue () && !depth.HasValue () && !hasCircleBased && !hasIsWidthAndHeightLinked && buildingMaterialIdOs == nullptr && profileIdOs == nullptr) {
        return true;
    }

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_ColumnSegment) != NoError || memo.columnSegments == nullptr) {
        return false;
    }

    const GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr> (memo.columnSegments)) / sizeof (API_ColumnSegmentType);
    for (GSSize i = 0; i < nSegments; ++i) {
        API_AssemblySegmentData& segment = memo.columnSegments[i].assemblySegmentData;
        if (hasIsWidthAndHeightLinked) {
            segment.isWidthAndHeightLinked = isWidthAndHeightLinked;
        }
        if (width.HasValue ()) {
            segment.nominalWidth = width.Get ();
        }
        if (depth.HasValue ()) {
            segment.nominalHeight = depth.Get ();
        }
        if (hasCircleBased) {
            segment.circleBased = circleBased;
        }
        if (profileIdOs != nullptr) {
            segment.modelElemStructureType = API_ProfileStructure;
            segment.profileAttr = GetAttributeIndexFromGuid (API_ProfileID, GetGuidFromObjectState (*profileIdOs));
            segment.circleBased = false;
        } else if (buildingMaterialIdOs != nullptr) {
            segment.modelElemStructureType = API_BasicStructure;
            segment.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialIdOs));
        }
    }

    return ACAPI_Element_ChangeMemo (elemGuid, APIMemoMask_ColumnSegment, &memo) == NoError;
}

static bool ApplyBeamSectionToMemo (API_Guid elemGuid, const GS::ObjectState& details)
{
    auto width = GetOptionalDouble (details, "width");
    auto height = GetOptionalDouble (details, "height");
    bool isWidthAndHeightLinked = false;
    const bool hasIsWidthAndHeightLinked = details.Get ("isWidthAndHeightLinked", isWidthAndHeightLinked);
    const GS::ObjectState* buildingMaterialIdOs = details.Get ("buildingMaterialId");
    const GS::ObjectState* profileIdOs = details.Get ("profileId");
    if (!width.HasValue () && !height.HasValue () && !hasIsWidthAndHeightLinked && buildingMaterialIdOs == nullptr && profileIdOs == nullptr) {
        return true;
    }

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_BeamSegment) != NoError || memo.beamSegments == nullptr) {
        return false;
    }

    const GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr> (memo.beamSegments)) / sizeof (API_BeamSegmentType);
    for (GSSize i = 0; i < nSegments; ++i) {
        API_AssemblySegmentData& segment = memo.beamSegments[i].assemblySegmentData;
        if (hasIsWidthAndHeightLinked) {
            segment.isWidthAndHeightLinked = isWidthAndHeightLinked;
        }
        if (width.HasValue ()) {
            segment.nominalWidth = width.Get ();
        }
        if (height.HasValue ()) {
            segment.nominalHeight = height.Get ();
        }
        if (profileIdOs != nullptr) {
            segment.modelElemStructureType = API_ProfileStructure;
            segment.profileAttr = GetAttributeIndexFromGuid (API_ProfileID, GetGuidFromObjectState (*profileIdOs));
        } else if (buildingMaterialIdOs != nullptr) {
            segment.modelElemStructureType = API_BasicStructure;
            segment.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialIdOs));
        }
    }

    return ACAPI_Element_ChangeMemo (elemGuid, APIMemoMask_BeamSegment, &memo) == NoError;
}

bool BuildCuboidMorphMemo (double sizeX, double sizeY, double sizeZ, API_AttributeIndex buildingMaterial, API_ElementMemo& memo)
{
    void* bodyData = nullptr;
    if (ACAPI_Body_Create (nullptr, nullptr, &bodyData) != NoError || bodyData == nullptr) {
        return false;
    }

    const GS::OnExit disposeBody ([&]() {
        if (bodyData != nullptr) {
            ACAPI_Body_Dispose (&bodyData);
        }
    });

    API_Coord3D coords[] = {
        {0.0,   0.0,   0.0},
        {sizeX, 0.0,   0.0},
        {sizeX, sizeY, 0.0},
        {0.0,   sizeY, 0.0},
        {0.0,   0.0,   sizeZ},
        {sizeX, 0.0,   sizeZ},
        {sizeX, sizeY, sizeZ},
        {0.0,   sizeY, sizeZ}
    };

    UInt32 vertices[8];
    for (UIndex i = 0; i < 8; ++i) {
        ACAPI_Body_AddVertex (bodyData, coords[i], vertices[i]);
    }

    Int32 edges[12];
    ACAPI_Body_AddEdge (bodyData, vertices[0], vertices[1], edges[0]);
    ACAPI_Body_AddEdge (bodyData, vertices[1], vertices[2], edges[1]);
    ACAPI_Body_AddEdge (bodyData, vertices[2], vertices[3], edges[2]);
    ACAPI_Body_AddEdge (bodyData, vertices[3], vertices[0], edges[3]);
    ACAPI_Body_AddEdge (bodyData, vertices[4], vertices[5], edges[4]);
    ACAPI_Body_AddEdge (bodyData, vertices[5], vertices[6], edges[5]);
    ACAPI_Body_AddEdge (bodyData, vertices[6], vertices[7], edges[6]);
    ACAPI_Body_AddEdge (bodyData, vertices[7], vertices[4], edges[7]);
    ACAPI_Body_AddEdge (bodyData, vertices[0], vertices[4], edges[8]);
    ACAPI_Body_AddEdge (bodyData, vertices[1], vertices[5], edges[9]);
    ACAPI_Body_AddEdge (bodyData, vertices[2], vertices[6], edges[10]);
    ACAPI_Body_AddEdge (bodyData, vertices[3], vertices[7], edges[11]);

#ifdef ServerMainVers_2700
    API_OverriddenAttribute material;
    material = buildingMaterial;
#else
    (void) buildingMaterial;
    API_OverriddenAttribute material = {};
#endif
    UInt32 polygon = 0;
    ACAPI_Body_AddPolygon (bodyData, {edges[0], edges[1], edges[2], edges[3]}, 0, material, polygon);
    ACAPI_Body_AddPolygon (bodyData, {edges[4], edges[5], edges[6], edges[7]}, 0, material, polygon);
    ACAPI_Body_AddPolygon (bodyData, {edges[0], edges[9], -edges[4], -edges[8]}, 0, material, polygon);
    ACAPI_Body_AddPolygon (bodyData, {edges[1], edges[10], -edges[5], -edges[9]}, 0, material, polygon);
    ACAPI_Body_AddPolygon (bodyData, {edges[2], edges[11], -edges[6], -edges[10]}, 0, material, polygon);
    ACAPI_Body_AddPolygon (bodyData, {edges[3], edges[8], -edges[7], -edges[11]}, 0, material, polygon);

    if (ACAPI_Body_Finish (bodyData, &memo.morphBody, &memo.morphMaterialMapTable) != NoError) {
        return false;
    }

    return true;
}

static GS::UniString MorphBodyTypeToString (API_MorphBodyTypeID bodyType)
{
    return bodyType == APIMorphBodyType_SolidBody ? "Solid" : "Surface";
}

static GS::UniString MorphEdgeTypeToString (API_MorphEdgeTypeID edgeType)
{
    switch (edgeType) {
        case APIMorphEdgeType_HardVisibleEdge: return "HardVisible";
        case APIMorphEdgeType_HardHiddenEdge:   return "HardHidden";
        case APIMorphEdgeType_SoftHiddenEdge:
        default:                                return "SoftHidden";
    }
}

static GS::UniString ElemDisplayOptionToString (API_ElemDisplayOptionsID displayOption)
{
    switch (displayOption) {
        case API_StandardWithAbstract: return "StandardWithAbstract";
        case API_CutOnly:              return "CutOnly";
        case API_OutLinesOnly:         return "OutLinesOnly";
        case API_AbstractAll:          return "AbstractAll";
        case API_CutAll:               return "CutAll";
        case API_Standard:
        default:                       return "Standard";
    }
}

static bool ElemDisplayOptionFromString (const GS::UniString& s, API_ElemDisplayOptionsID& out)
{
    if (s == "Standard")                { out = API_Standard; return true; }
    if (s == "StandardWithAbstract")     { out = API_StandardWithAbstract; return true; }
    if (s == "CutOnly")                  { out = API_CutOnly; return true; }
    if (s == "OutLinesOnly")             { out = API_OutLinesOnly; return true; }
    if (s == "AbstractAll")              { out = API_AbstractAll; return true; }
    if (s == "CutAll")                   { out = API_CutAll; return true; }
    return false;
}

static GS::UniString ViewDepthLimitationToString (API_ElemViewDepthLimitationsID viewDepthLimitation)
{
    switch (viewDepthLimitation) {
        case API_ToAbsoluteLimit: return "ToAbsoluteLimit";
        case API_EntireElement:   return "EntireElement";
        case API_ToFloorPlanRange:
        default:                  return "ToFloorPlanRange";
    }
}

static bool ViewDepthLimitationFromString (const GS::UniString& s, API_ElemViewDepthLimitationsID& out)
{
    if (s == "ToFloorPlanRange") { out = API_ToFloorPlanRange; return true; }
    if (s == "ToAbsoluteLimit")  { out = API_ToAbsoluteLimit; return true; }
    if (s == "EntireElement")    { out = API_EntireElement; return true; }
    return false;
}

static GS::UniString TextureProjectionTypeToString (API_TextureProjectionTypeID projectionType)
{
    switch (projectionType) {
        case APITextureProjectionType_Planar:   return "Planar";
        case APITextureProjectionType_Default:  return "Default";
        case APITextureProjectionType_Cylindric: return "Cylindric";
        case APITextureProjectionType_Spheric:  return "Spheric";
        case APITextureProjectionType_Box:      return "Box";
        case APITextureProjectionType_Invalid:
        default:                                 return "Invalid";
    }
}

static bool TextureProjectionTypeFromString (const GS::UniString& s, API_TextureProjectionTypeID& out)
{
    if (s == "Invalid")   { out = APITextureProjectionType_Invalid; return true; }
    if (s == "Planar")    { out = APITextureProjectionType_Planar; return true; }
    if (s == "Default")   { out = APITextureProjectionType_Default; return true; }
    if (s == "Cylindric") { out = APITextureProjectionType_Cylindric; return true; }
    if (s == "Spheric")   { out = APITextureProjectionType_Spheric; return true; }
    if (s == "Box")       { out = APITextureProjectionType_Box; return true; }
    return false;
}

// Named distinctly from CommandBase.hpp's HatchOrientationTypeToString/FromString (added
// alongside the Slab cover-fill work): that pair has a different signature (returns the value
// directly, with a fallback default) and cannot report an invalid string, whereas Morph's own
// coverFillOrientation handling below specifically needs to reject an invalid type string outright.
static GS::UniString MorphHatchOrientationTypeToString (API_HatchOrientationTypeID orientationType)
{
    switch (orientationType) {
        case API_HatchRotated:   return "Rotated";
        case API_HatchDistorted: return "Distorted";
        case API_HatchCentered:  return "Centered";
        case API_HatchGlobal:
        default:                 return "Global";
    }
}

static bool MorphHatchOrientationTypeFromString (const GS::UniString& s, API_HatchOrientationTypeID& out)
{
    if (s == "Global")     { out = API_HatchGlobal; return true; }
    if (s == "Rotated")    { out = API_HatchRotated; return true; }
    if (s == "Distorted")  { out = API_HatchDistorted; return true; }
    if (s == "Centered")   { out = API_HatchCentered; return true; }
    return false;
}

// Reuses (rather than re-adds) the edge between two vertices - ACAPI_Body_AddEdge's own doc is
// explicit that "the edge can be used with at most 2 polygons, and in that case it has to be
// passed with opposite directions", so a shared edge between two adjacent faces must resolve to
// the SAME edge index (sign-flipped for whichever face walks it in the other direction), not two
// separate edges - confirmed against BuildCuboidMorphMemo's own hand-written edge reuse pattern
// above (e.g. `-edges[4]`, `-edges[8]`), just generalized to arbitrary polygon counts.
static Int32 GetOrAddEdge (void* bodyData, UInt32 a, UInt32 b, std::map<std::pair<UInt32, UInt32>, Int32>& edgeCache)
{
    const bool flipped = a > b;
    const auto key = flipped ? std::make_pair (b, a) : std::make_pair (a, b);
    auto it = edgeCache.find (key);
    if (it != edgeCache.end ()) {
        return flipped ? -it->second : it->second;
    }
    Int32 idx = 0;
    ACAPI_Body_AddEdge (bodyData, key.first, key.second, idx);
    edgeCache[key] = idx;
    return flipped ? -idx : idx;
}

// Builds an arbitrary Morph body (any number of faces, holes, per-face building materials) from
// the "body" JSON shape documented by the "MorphBody" schema definition (CommonSchemaDefinitions.json)
// - the general-purpose counterpart to BuildCuboidMorphMemo above, shared by CreateMorphs and
// ModifyMorphs (the latter via a "replace the whole body" flag, mirroring CreateProfiles'
// replaceSkins). Does NOT set element.morph.bodyType/edgeType - the caller does that from the
// same "body" object's bodyType/edgeDefault fields, since those live on API_Element, not the memo.
bool BuildMorphBodyFromGeometry (const GS::ObjectState& bodyOS, API_ElementMemo& memo, GS::UniString& errorOut)
{
    GS::Array<GS::ObjectState> verticesOS;
    bodyOS.Get ("vertices", verticesOS);
    GS::Array<GS::ObjectState> polygonsOS;
    bodyOS.Get ("polygons", polygonsOS);
    GS::Array<GS::ObjectState> wireEdgesOS;
    bodyOS.Get ("wireEdges", wireEdgesOS);

    // A Solid body needs at least a tetrahedron (4 vertices, 4 faces) to be a valid closed
    // volume. A Surface body is an open shell - it has no closedness requirement at all, so a
    // single face (3 vertices, 1 polygon), or even just a couple of standalone wireEdges with no
    // face at all, is already a legitimate Morph. ACAPI_Body_* itself never enforces closedness
    // (an edge is only ever required to have *at most* two side polygons, per its own doc -
    // having zero or one is a normal open/boundary/standalone edge); the stricter floor below
    // only applies when the caller actually asked for a Solid.
    GS::UniString bodyTypeStr;
    bodyOS.Get ("bodyType", bodyTypeStr);
    const bool isSurfaceBody = (bodyTypeStr == "Surface");
    if (isSurfaceBody) {
        // Confirmed live: a lone vertex with no edge at all is rejected by Archicad itself
        // (ACAPI_Element_Create fails outright) - a Morph needs at least one edge to exist as a
        // valid element, whether that's a real face loop or just a single standalone wireEdge.
        if (verticesOS.GetSize () < 2 || (polygonsOS.GetSize () < 1 && wireEdgesOS.GetSize () < 1)) {
            errorOut = "A Morph body needs at least 2 vertices and 1 face or wire edge.";
            return false;
        }
    } else if (verticesOS.GetSize () < 4 || polygonsOS.GetSize () < 4) {
        errorOut = "A Solid Morph body needs at least 4 vertices and 4 faces to form a closed volume.";
        return false;
    }

    void* bodyData = nullptr;
    if (ACAPI_Body_Create (nullptr, nullptr, &bodyData) != NoError || bodyData == nullptr) {
        errorOut = "Failed to start building the morph body.";
        return false;
    }
    const GS::OnExit disposeBody ([&] () {
        if (bodyData != nullptr) {
            ACAPI_Body_Dispose (&bodyData);
        }
    });

    // GS::Array's single-integer constructor reserves CAPACITY only (GetSize() stays 0 until
    // elements are actually added) - confirmed live: indexed-assigning into a capacity-only array
    // left vertexIndices permanently empty, making every subsequent bounds check against its size
    // fail for every real vertex index ("a face or hole references a vertex index out of range").
    GS::Array<UInt32> vertexIndices;
    vertexIndices.SetSize (verticesOS.GetSize ());
    for (UIndex i = 0; i < verticesOS.GetSize (); ++i) {
        const API_Coord3D coord = Get3DCoordinateFromObjectState (verticesOS[i]);
        UInt32 idx = 0;
        ACAPI_Body_AddVertex (bodyData, coord, idx);
        vertexIndices[i] = idx;
    }

    const auto resolveLoopEdges = [&] (const GS::Array<Int32>& loopVertexIds, std::map<std::pair<UInt32, UInt32>, Int32>& edgeCache,
                                        GS::Array<Int32>& outEdgeIndices, GS::UniString& loopError) -> bool {
        for (UIndex i = 0; i < loopVertexIds.GetSize (); ++i) {
            const Int32 vFrom = loopVertexIds[i];
            const Int32 vTo = loopVertexIds[(i + 1) % loopVertexIds.GetSize ()];
            if (vFrom < 0 || (GS::UIndex) vFrom >= vertexIndices.GetSize () || vTo < 0 || (GS::UIndex) vTo >= vertexIndices.GetSize ()) {
                loopError = "A face or hole references a vertex index that is out of range.";
                return false;
            }
            outEdgeIndices.Push (GetOrAddEdge (bodyData, vertexIndices[vFrom], vertexIndices[vTo], edgeCache));
        }
        return true;
    };

    std::map<std::pair<UInt32, UInt32>, Int32> edgeCache;

    for (const GS::ObjectState& polygonOS : polygonsOS) {
        GS::Array<Int32> outerVertexIds;
        polygonOS.Get ("vertexIds", outerVertexIds);
        if (outerVertexIds.GetSize () < 3) {
            errorOut = "Every face needs at least 3 vertices.";
            return false;
        }

        GS::Array<Int32> edgeIndices;
        GS::UniString loopError;
        if (!resolveLoopEdges (outerVertexIds, edgeCache, edgeIndices, loopError)) {
            errorOut = loopError;
            return false;
        }

        bool filled = true;
        polygonOS.Get ("filled", filled);
        if (!filled) {
            // Wire-only loop: the edges above still get created (so they're shared/reused
            // correctly with any other face that also touches them), but no ACAPI_Body_AddPolygon
            // call means no face fill - Modeler::MeshBody computes IsWireBody() from exactly this
            // (a body with edges but no adjacent polygon on either side). holes/surfaceId make no
            // sense without a fill, so they are not read for a wire loop.
            continue;
        }

        GS::Array<GS::ObjectState> holesOS;
        if (polygonOS.Get ("holes", holesOS)) {
            for (const GS::ObjectState& holeOS : holesOS) {
                GS::Array<Int32> holeVertexIds;
                holeOS.Get ("vertexIds", holeVertexIds);
                if (holeVertexIds.GetSize () < 3) {
                    errorOut = "Every hole needs at least 3 vertices.";
                    return false;
                }
                edgeIndices.Push (0);
                if (!resolveLoopEdges (holeVertexIds, edgeCache, edgeIndices, loopError)) {
                    errorOut = loopError;
                    return false;
                }
            }
        }

#ifdef ServerMainVers_2700
        // This per-polygon override is a SURFACE (API_MaterialID), not a building material -
        // confirmed live: a Morph's building material is always a single whole-volume property
        // (element.morph.buildingMaterial), never per-face. Leaving `material` unset (hasValue
        // false, the default) means "no override, inherit the element's default appearance" -
        // do NOT fall back to the building material's own index here, since treating a building
        // material index as if it were a surface index picks an unrelated, coincidental surface.
        API_OverriddenAttribute material = {};
        auto faceSurfaceId = GetOptionalObjectState (polygonOS, "surfaceId");
        if (faceSurfaceId.HasValue ()) {
            API_AttributeIndex faceSurfaceIndex = APIInvalidAttributeIndex;
            if (!ResolveAttributeIndex (faceSurfaceId.Get (), API_MaterialID, faceSurfaceIndex)) {
                errorOut = "Invalid per-face surface.";
                return false;
            }
            material = faceSurfaceIndex;
        }
#else
        API_OverriddenAttribute material = {};
#endif

        UInt32 polygonIndex = 0;
        if (ACAPI_Body_AddPolygon (bodyData, edgeIndices, 0, material, polygonIndex) != NoError) {
            errorOut = "Failed to build one of the morph's faces - check winding order and vertex indices.";
            return false;
        }
    }

    // Standalone edges with no face at all - the input-side counterpart of the "wireEdges" Get
    // reports (see AddMorphBodyFromMemo): unlike a "filled": false polygon loop above (which is
    // still a whole closed cycle of edges), these are individual segments, so they reuse
    // GetOrAddEdge directly instead of going through resolveLoopEdges' loop-with-wraparound logic.
    for (const GS::ObjectState& wireEdgeOS : wireEdgesOS) {
        GS::Array<Int32> edgeVertexIds;
        wireEdgeOS.Get ("vertexIds", edgeVertexIds);
        if (edgeVertexIds.GetSize () != 2) {
            errorOut = "Every wire edge needs exactly 2 vertex indices.";
            return false;
        }
        const Int32 vFrom = edgeVertexIds[0];
        const Int32 vTo = edgeVertexIds[1];
        if (vFrom < 0 || (GS::UIndex) vFrom >= vertexIndices.GetSize () || vTo < 0 || (GS::UIndex) vTo >= vertexIndices.GetSize ()) {
            errorOut = "A wire edge references a vertex index that is out of range.";
            return false;
        }
        GetOrAddEdge (bodyData, vertexIndices[vFrom], vertexIndices[vTo], edgeCache);
    }

    if (ACAPI_Body_Finish (bodyData, &memo.morphBody, &memo.morphMaterialMapTable) != NoError) {
        errorOut = "Failed to finalize the morph body.";
        return false;
    }

    return true;
}

// Applies every API_MorphType field beyond geometry/body itself - placement rotation axes,
// shadows, per-story visibility, floor plan display/pens/line types/cover fill, texture
// projection, level - from the same flat MorphDetails JSON shape AddMorphBodyFromMemo reports.
// Shared by CreateMorphs (mask == nullptr, a freshly-defaulted element needs no mask) and
// ModifyMorphs (mask != nullptr, every touched field also needs its ACAPI_ELEMENT_MASK_SET bit).
// Returns false only on a genuine validation error (bad attribute reference, partial axis triple,
// unrecognized enum string); increments appliedCount for every recognized field found, so
// ModifyMorphs can tell whether ANY field was actually given.
bool ApplyMorphCosmeticDetails (const GS::ObjectState& details, API_Element& element, API_Element* mask, int& appliedCount, GS::UniString& errorOut)
{
    auto xAxisOS = GetOptionalObjectState (details, "xAxis");
    auto yAxisOS = GetOptionalObjectState (details, "yAxis");
    auto zAxisOS = GetOptionalObjectState (details, "zAxis");
    const int axisCount = (int) xAxisOS.HasValue () + (int) yAxisOS.HasValue () + (int) zAxisOS.HasValue ();
    if (axisCount != 0 && axisCount != 3) {
        errorOut = "Give all three of 'xAxis', 'yAxis' and 'zAxis' together, or none at all.";
        return false;
    }
    if (axisCount == 3) {
        const API_Coord3D xAxis = Get3DCoordinateFromObjectState (xAxisOS.Get ());
        const API_Coord3D yAxis = Get3DCoordinateFromObjectState (yAxisOS.Get ());
        const API_Coord3D zAxis = Get3DCoordinateFromObjectState (zAxisOS.Get ());
        double* tmx = element.morph.tranmat.tmx;
        tmx[0] = xAxis.x; tmx[1] = xAxis.y; tmx[2] = xAxis.z;
        tmx[4] = yAxis.x; tmx[5] = yAxis.y; tmx[6] = yAxis.z;
        tmx[8] = zAxis.x; tmx[9] = zAxis.y; tmx[10] = zAxis.z;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, tranmat);
        }
        ++appliedCount;
    }

    auto surfaceId = GetOptionalObjectState (details, "surfaceId");
    if (surfaceId.HasValue ()) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (surfaceId.Get (), API_MaterialID, idx)) {
            errorOut = "Invalid morph default surface ('surfaceId').";
            return false;
        }
#ifdef ServerMainVers_2700
        element.morph.material.hasValue = true;
        element.morph.material.value = idx;
#else
        element.morph.material.overridden = true;
        element.morph.material.attributeIndex = idx;
#endif
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, material);
        }
        ++appliedCount;
    }

    {
        bool v = false;
        if (details.Get ("castShadow", v)) {
            element.morph.castShadow = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, castShadow);
            ++appliedCount;
        }
    }
    {
        bool v = false;
        if (details.Get ("receiveShadow", v)) {
            element.morph.receiveShadow = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, receiveShadow);
            ++appliedCount;
        }
    }
    {
        bool v = false;
        if (details.Get ("isAutoOnStoryVisibility", v)) {
            element.morph.isAutoOnStoryVisibility = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, isAutoOnStoryVisibility);
            ++appliedCount;
        }
    }

    const auto applyStoryVisibility = [&] (const char* fieldName, API_StoryVisibility& target) {
        const GS::ObjectState* svOS = details.Get (fieldName);
        if (svOS == nullptr) {
            return;
        }
        API_StoryVisibility sv = target;
        svOS->Get ("showOnHome", sv.showOnHome);
        svOS->Get ("showAllAbove", sv.showAllAbove);
        svOS->Get ("showAllBelow", sv.showAllBelow);
        Int32 relAbove = sv.showRelAbove;
        Int32 relBelow = sv.showRelBelow;
        if (svOS->Get ("showRelAbove", relAbove)) {
            sv.showRelAbove = (short) relAbove;
        }
        if (svOS->Get ("showRelBelow", relBelow)) {
            sv.showRelBelow = (short) relBelow;
        }
        target = sv;
        ++appliedCount;
    };
    applyStoryVisibility ("showContour", element.morph.showContour);
    if (details.Get ("showContour") != nullptr && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, showContour);
    }
    applyStoryVisibility ("showFill", element.morph.showFill);
    if (details.Get ("showFill") != nullptr && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, showFill);
    }

    {
        const GS::ObjectState* linkOS = details.Get ("linkToSettings");
        if (linkOS != nullptr) {
            API_LinkToSettings link = element.morph.linkToSettings;
            Int32 homeDiff = link.homeStoryDifference;
            if (linkOS->Get ("homeStoryDifference", homeDiff)) {
                link.homeStoryDifference = (short) homeDiff;
            }
            linkOS->Get ("newCreationMode", link.newCreationMode);
            element.morph.linkToSettings = link;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, linkToSettings);
            ++appliedCount;
        }
    }

    {
        GS::UniString s;
        if (details.Get ("displayOption", s)) {
            API_ElemDisplayOptionsID v;
            if (!ElemDisplayOptionFromString (s, v)) {
                errorOut = "Invalid 'displayOption'.";
                return false;
            }
            element.morph.displayOption = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, displayOption);
            ++appliedCount;
        }
    }
    {
        GS::UniString s;
        if (details.Get ("viewDepthLimitation", s)) {
            API_ElemViewDepthLimitationsID v;
            if (!ViewDepthLimitationFromString (s, v)) {
                errorOut = "Invalid 'viewDepthLimitation'.";
                return false;
            }
            element.morph.viewDepthLimitation = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, viewDepthLimitation);
            ++appliedCount;
        }
    }

    {
        Int32 v = 0;
        if (details.Get ("cutFillPen", v)) {
#ifdef ServerMainVers_2700
            element.morph.cutFillPen.hasValue = true;
            element.morph.cutFillPen.value = (API_PenIndex) v;
#else
            element.morph.penOverride.cutFillPen = (short) v;
            element.morph.penOverride.overrideCutFillPen = true;
#endif
            if (mask != nullptr) {
#ifdef ServerMainVers_2700
                ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, cutFillPen);
#else
                ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, penOverride);
#endif
            }
            ++appliedCount;
        }
    }
    {
        Int32 v = 0;
        if (details.Get ("cutFillBackgroundPen", v)) {
#ifdef ServerMainVers_2700
            element.morph.cutFillBackgroundPen.hasValue = true;
            element.morph.cutFillBackgroundPen.value = (API_PenIndex) v;
#else
            element.morph.penOverride.cutFillBackgroundPen = (short) v;
            element.morph.penOverride.overrideCutFillBackgroundPen = true;
#endif
            if (mask != nullptr) {
#ifdef ServerMainVers_2700
                ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, cutFillBackgroundPen);
#else
                ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, penOverride);
#endif
            }
            ++appliedCount;
        }
    }

    const auto applyLineType = [&] (const char* fieldName, API_AttributeIndex API_MorphType::* member, const char* errorLabel) -> bool {
        auto lineTypeId = GetOptionalObjectState (details, fieldName);
        if (!lineTypeId.HasValue ()) {
            return true;
        }
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (lineTypeId.Get (), API_LinetypeID, idx)) {
            errorOut = GS::UniString::Printf ("Invalid '%s' line type reference.", errorLabel);
            return false;
        }
        element.morph.*member = idx;
        ++appliedCount;
        return true;
    };
    if (!applyLineType ("cutLineType", &API_MorphType::cutLineType, "cutLineType")) return false;
    if (details.Get ("cutLineType") != nullptr && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, cutLineType);
    if (!applyLineType ("uncutLineType", &API_MorphType::uncutLineType, "uncutLineType")) return false;
    if (details.Get ("uncutLineType") != nullptr && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, uncutLineType);
    if (!applyLineType ("overheadLineType", &API_MorphType::overheadLineType, "overheadLineType")) return false;
    if (details.Get ("overheadLineType") != nullptr && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, overheadLineType);

    const auto applyShortField = [&] (const char* fieldName, short API_MorphType::* member) {
        Int32 v = 0;
        if (details.Get (fieldName, v)) {
            element.morph.*member = (short) v;
            ++appliedCount;
            return true;
        }
        return false;
    };
    if (applyShortField ("cutLinePen", &API_MorphType::cutLinePen) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, cutLinePen);
    if (applyShortField ("uncutLinePen", &API_MorphType::uncutLinePen) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, uncutLinePen);
    if (applyShortField ("overheadLinePen", &API_MorphType::overheadLinePen) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, overheadLinePen);
    if (applyShortField ("coverFillPen", &API_MorphType::coverFillPen) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, coverFillPen);
    if (applyShortField ("coverFillBGPen", &API_MorphType::coverFillBGPen) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, coverFillBGPen);

    const auto applyBoolField = [&] (const char* fieldName, bool API_MorphType::* member) {
        bool v = false;
        if (details.Get (fieldName, v)) {
            element.morph.*member = v;
            ++appliedCount;
            return true;
        }
        return false;
    };
    if (applyBoolField ("useCoverFillType", &API_MorphType::useCoverFillType) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, useCoverFillType);
    if (applyBoolField ("outlineContourDisplay", &API_MorphType::outlineContourDisplay) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, outlineContourDisplay);
    if (applyBoolField ("use3DHatching", &API_MorphType::use3DHatching) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, use3DHatching);
    if (applyBoolField ("useDistortedCoverFill", &API_MorphType::useDistortedCoverFill) && mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, useDistortedCoverFill);

    {
        auto coverFillTypeId = GetOptionalObjectState (details, "coverFillType");
        if (coverFillTypeId.HasValue ()) {
            API_AttributeIndex idx = APIInvalidAttributeIndex;
            if (!ResolveAttributeIndex (coverFillTypeId.Get (), API_FilltypeID, idx)) {
                errorOut = "Invalid 'coverFillType' fill reference.";
                return false;
            }
            element.morph.coverFillType = idx;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, coverFillType);
            ++appliedCount;
        }
    }

    {
        const GS::ObjectState* orientOS = details.Get ("coverFillOrientation");
        if (orientOS != nullptr) {
            API_HatchOrientation orient = element.morph.coverFillOrientation;
            GS::UniString typeStr;
            if (orientOS->Get ("type", typeStr)) {
                API_HatchOrientationTypeID t;
                if (!MorphHatchOrientationTypeFromString (typeStr, t)) {
                    errorOut = "Invalid 'coverFillOrientation.type'.";
                    return false;
                }
                orient.type = t;
            }
            const GS::ObjectState* origoOS = orientOS->Get ("origo");
            if (origoOS != nullptr) {
                orient.origo = Get2DCoordinateFromObjectState (*origoOS);
            }
            orientOS->Get ("matrix00", orient.matrix00);
            orientOS->Get ("matrix10", orient.matrix10);
            orientOS->Get ("matrix01", orient.matrix01);
            orientOS->Get ("matrix11", orient.matrix11);
            orientOS->Get ("innerRadius", orient.innerRadius);
            element.morph.coverFillOrientation = orient;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, coverFillOrientation);
            ++appliedCount;
        }
    }

    {
        GS::UniString s;
        if (details.Get ("textureProjectionType", s)) {
            API_TextureProjectionTypeID v;
            if (!TextureProjectionTypeFromString (s, v)) {
                errorOut = "Invalid 'textureProjectionType'.";
                return false;
            }
            element.morph.textureProjectionType = v;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, textureProjectionType);
            ++appliedCount;
        }
    }
    {
        GS::Array<GS::ObjectState> coordsOS;
        if (details.Get ("textureProjectionCoords", coordsOS)) {
            if (coordsOS.GetSize () != 4) {
                errorOut = "'textureProjectionCoords' needs exactly 4 entries.";
                return false;
            }
            for (UIndex i = 0; i < 4; ++i) {
                element.morph.textureProjectionCoords[i] = Get3DCoordinateFromObjectState (coordsOS[i]);
            }
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, textureProjectionCoords);
            ++appliedCount;
        }
    }

    {
        double level = 0.0;
        if (details.Get ("level", level)) {
            element.morph.level = level;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_MorphType, level);
            ++appliedCount;
        }
    }

    return true;
}

// Per-edge display-status overrides (hidden/visible/smooth "aide a la courbe") were attempted
// here via a full MeshBody reconstruction (the only write path, since there is no ACAPI_Body_Set*
// for edge attributes - Modeler::MeshBody is read-only post-build; only the internal
// Cut::IMeshBody, never handed to add-ons, has mutators). The reconstruction itself worked
// without crashing, but the resulting edge attributes never survived ACAPI_Element_Create -
// confirmed against a GRAPHISOFT community thread (archicad-api forum, "Help with setting
// morph's hidden edges in the API", ManelCG/BerndSchwarzenbacher, 2023-2024): this is a CONFIRMED,
// still-unresolved Archicad SDK bug (reproduced independently by multiple developers across
// AC23-27) - element.morph.edgeType (and even bodyType, always coming back Solid) is silently
// discarded/reset by ACAPI_Element_Create/Change regardless of what's supplied, with no known
// workaround. Not fixable from an add-on. The element-wide edgeType/bodyType fields are still
// set on the element (CreateMorphsCommand/ModifyMorphsCommand below) since doing so is harmless
// and forward-compatible if GRAPHISOFT ever fixes this - but per-edge overrides were removed
// entirely rather than ship a reconstruction step with real risk (raw MeshBody manipulation) for
// zero actual effect. Reading back whatever Archicad itself currently reports (AddMorphBodyFromMemo
// in ElementCommands.cpp) remains fully correct and unaffected - only writing is impossible.

bool ApplyWindowOrDoorDetails (API_Element& element, API_Element& mask, const GS::ObjectState& details)
{
    bool changed = false;
    auto width = GetOptionalDouble (details, "width");
    if (width.HasValue ()) {
        element.window.openingBase.width = width.Get ();
        SetOpeningSizeMask (mask);
        changed = true;
    }
    auto height = GetOptionalDouble (details, "height");
    if (height.HasValue ()) {
        element.window.openingBase.height = height.Get ();
        SetOpeningSizeMask (mask);
        changed = true;
    }
    auto sillHeight = GetOptionalDouble (details, "sillHeight");
    if (sillHeight.HasValue ()) {
        element.window.lower = sillHeight.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, lower);
        changed = true;
    }
    auto centerOffset = GetOptionalDouble (details, "centerOffset");
    if (centerOffset.HasValue ()) {
        element.window.objLoc = centerOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, objLoc);
        changed = true;
    }
    bool reflected = false;
    if (details.Get ("reflected", reflected)) {
        element.window.openingBase.reflected = reflected;
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.reflected);
        changed = true;
    }
    bool refSide = false;
    if (details.Get ("refSide", refSide)) {
        element.window.openingBase.refSide = refSide;
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.refSide);
        changed = true;
    }
    bool oSide = false;
    if (details.Get ("oSide", oSide)) {
        element.window.openingBase.oSide = oSide;
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.oSide);
        changed = true;
    }
    bool reveal = false;
    if (details.Get ("reveal", reveal)) {
        element.window.reveal = reveal;
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, reveal);
        changed = true;
    }
    auto revealDepthOffset = GetOptionalDouble (details, "revealDepthOffset");
    if (revealDepthOffset.HasValue ()) {
        element.window.revealDepthOffset = revealDepthOffset.Get ();
        ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, revealDepthOffset);
        changed = true;
    }
    return changed;
}

}

// Reads a Morph's full body geometry (vertices/faces/holes/per-face material) plus its default
// edge display status and own building material, writing them into `typeSpecificDetails` in the
// shape the "MorphBody"/"MorphDetails" schema definitions (CommonSchemaDefinitions.json) expect -
// the same shape CreateMorphs/ModifyMorphs accept back via their "body" field. There is no
// ACAPI_Body_Get* family (confirmed live) - reading goes through ACAPI_Element_GetMemo's
// memo.morphBody (a Modeler::MeshBody), walked via its const read API. Declared in CommandBase.hpp
// and called from ElementCommands.cpp's GetDetailsOfElements switch, but defined HERE (not there) -
// Model3D/MeshBody.hpp cannot coexist with ElementCommands.cpp's existing ModelMeshBody.hpp
// include in the same translation unit (confirmed live: "'GS' n'est pas membre de 'GS'" GDL header
// conflict), so this stays a separate translation unit instead. Defined OUTSIDE the anonymous
// namespace above (unlike its GetOrAddEdge/BuildMorphBodyFromGeometry siblings) - it needs real
// external linkage to be callable from ElementCommands.cpp; confirmed live via LNK2019 that
// leaving it inside the anonymous namespace silently gives it internal linkage instead.
void AddMorphBodyFromMemo (const API_Element& elem, GS::ObjectState& typeSpecificDetails)
{
    typeSpecificDetails.Add ("origin", Create3DCoordinateObjectState (
        {elem.morph.tranmat.tmx[3], elem.morph.tranmat.tmx[7], elem.morph.tranmat.tmx[11]}));
    typeSpecificDetails.Add ("xAxis", Create3DCoordinateObjectState (
        {elem.morph.tranmat.tmx[0], elem.morph.tranmat.tmx[1], elem.morph.tranmat.tmx[2]}));
    typeSpecificDetails.Add ("yAxis", Create3DCoordinateObjectState (
        {elem.morph.tranmat.tmx[4], elem.morph.tranmat.tmx[5], elem.morph.tranmat.tmx[6]}));
    typeSpecificDetails.Add ("zAxis", Create3DCoordinateObjectState (
        {elem.morph.tranmat.tmx[8], elem.morph.tranmat.tmx[9], elem.morph.tranmat.tmx[10]}));
    if (elem.morph.buildingMaterial != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("buildingMaterialId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_BuildingMaterialID, elem.morph.buildingMaterial)));
    }
#ifdef ServerMainVers_2700
    if (elem.morph.material.hasValue) {
        typeSpecificDetails.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, elem.morph.material.value)));
    }
#else
    if (elem.morph.material.overridden) {
        typeSpecificDetails.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, elem.morph.material.attributeIndex)));
    }
#endif

    typeSpecificDetails.Add ("castShadow", elem.morph.castShadow);
    typeSpecificDetails.Add ("receiveShadow", elem.morph.receiveShadow);
    typeSpecificDetails.Add ("isAutoOnStoryVisibility", elem.morph.isAutoOnStoryVisibility);

    const auto addStoryVisibility = [&] (const char* fieldName, const API_StoryVisibility& sv) {
        GS::ObjectState svOS;
        svOS.Add ("showOnHome", sv.showOnHome);
        svOS.Add ("showAllAbove", sv.showAllAbove);
        svOS.Add ("showAllBelow", sv.showAllBelow);
        svOS.Add ("showRelAbove", (Int32) sv.showRelAbove);
        svOS.Add ("showRelBelow", (Int32) sv.showRelBelow);
        typeSpecificDetails.Add (fieldName, svOS);
    };
    addStoryVisibility ("showContour", elem.morph.showContour);
    addStoryVisibility ("showFill", elem.morph.showFill);

    {
        GS::ObjectState linkOS;
        linkOS.Add ("homeStoryDifference", (Int32) elem.morph.linkToSettings.homeStoryDifference);
        linkOS.Add ("newCreationMode", elem.morph.linkToSettings.newCreationMode);
        typeSpecificDetails.Add ("linkToSettings", linkOS);
    }

    typeSpecificDetails.Add ("displayOption", ElemDisplayOptionToString (elem.morph.displayOption));
    typeSpecificDetails.Add ("viewDepthLimitation", ViewDepthLimitationToString (elem.morph.viewDepthLimitation));

#ifdef ServerMainVers_2700
    if (elem.morph.cutFillPen.hasValue) {
        typeSpecificDetails.Add ("cutFillPen", (Int32) elem.morph.cutFillPen.value);
    }
    if (elem.morph.cutFillBackgroundPen.hasValue) {
        typeSpecificDetails.Add ("cutFillBackgroundPen", (Int32) elem.morph.cutFillBackgroundPen.value);
    }
#else
    if (elem.morph.penOverride.overrideCutFillPen) {
        typeSpecificDetails.Add ("cutFillPen", (Int32) elem.morph.penOverride.cutFillPen);
    }
    if (elem.morph.penOverride.overrideCutFillBackgroundPen) {
        typeSpecificDetails.Add ("cutFillBackgroundPen", (Int32) elem.morph.penOverride.cutFillBackgroundPen);
    }
#endif

    if (elem.morph.cutLineType != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("cutLineType", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.morph.cutLineType)));
    }
    typeSpecificDetails.Add ("cutLinePen", (Int32) elem.morph.cutLinePen);
    if (elem.morph.uncutLineType != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("uncutLineType", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.morph.uncutLineType)));
    }
    typeSpecificDetails.Add ("uncutLinePen", (Int32) elem.morph.uncutLinePen);
    if (elem.morph.overheadLineType != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("overheadLineType", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, elem.morph.overheadLineType)));
    }
    typeSpecificDetails.Add ("overheadLinePen", (Int32) elem.morph.overheadLinePen);

    typeSpecificDetails.Add ("useCoverFillType", elem.morph.useCoverFillType);
    typeSpecificDetails.Add ("outlineContourDisplay", elem.morph.outlineContourDisplay);
    if (elem.morph.coverFillType != APIInvalidAttributeIndex) {
        typeSpecificDetails.Add ("coverFillType", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, elem.morph.coverFillType)));
    }
    typeSpecificDetails.Add ("coverFillPen", (Int32) elem.morph.coverFillPen);
    typeSpecificDetails.Add ("coverFillBGPen", (Int32) elem.morph.coverFillBGPen);
    typeSpecificDetails.Add ("use3DHatching", elem.morph.use3DHatching);

    {
        GS::ObjectState orientOS;
        orientOS.Add ("type", MorphHatchOrientationTypeToString (elem.morph.coverFillOrientation.type));
        orientOS.Add ("origo", Create2DCoordinateObjectState (elem.morph.coverFillOrientation.origo));
        orientOS.Add ("matrix00", elem.morph.coverFillOrientation.matrix00);
        orientOS.Add ("matrix10", elem.morph.coverFillOrientation.matrix10);
        orientOS.Add ("matrix01", elem.morph.coverFillOrientation.matrix01);
        orientOS.Add ("matrix11", elem.morph.coverFillOrientation.matrix11);
        orientOS.Add ("innerRadius", elem.morph.coverFillOrientation.innerRadius);
        typeSpecificDetails.Add ("coverFillOrientation", orientOS);
    }
    typeSpecificDetails.Add ("useDistortedCoverFill", elem.morph.useDistortedCoverFill);

    typeSpecificDetails.Add ("textureProjectionType", TextureProjectionTypeToString (elem.morph.textureProjectionType));
    {
        const auto& coords = typeSpecificDetails.AddList<GS::ObjectState> ("textureProjectionCoords");
        for (Int32 i = 0; i < 4; ++i) {
            coords (Create3DCoordinateObjectState (elem.morph.textureProjectionCoords[i]));
        }
    }

    typeSpecificDetails.Add ("level", elem.morph.level);

    GS::ObjectState body;
    body.Add ("bodyType", MorphBodyTypeToString (elem.morph.bodyType));
    body.Add ("edgeDefault", MorphEdgeTypeToString (elem.morph.edgeType));

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (ACAPI_Element_GetMemo (elem.header.guid, &memo) != NoError || memo.morphBody == nullptr) {
        typeSpecificDetails.Add ("body", body);
        return;
    }
    const Modeler::MeshBody& meshBody = *memo.morphBody;
    body.Add ("isClosed", meshBody.IsClosedBody ());

    {
        const auto& vertices = body.AddList<GS::ObjectState> ("vertices");
        const ULong vertexCount = meshBody.GetVertexCount ();
        for (ULong i = 0; i < vertexCount; ++i) {
            const VERT& v = meshBody.GetConstVertex (i);
            vertices (Create3DCoordinateObjectState ({v.x, v.y, v.z}));
        }
    }

    {
        const auto& polygons = body.AddList<GS::ObjectState> ("polygons");
        const ULong polygonCount = meshBody.GetPolygonCount ();
        for (ULong i = 0; i < polygonCount; ++i) {
            GS::Array<ULong> vertIdxs;
            GS::Array<ULong> nextContourStartIdxs;
            meshBody.GetPolygonVertices (i, vertIdxs, &nextContourStartIdxs);

            GS::ObjectState polygon;
            const ULong outerEnd = nextContourStartIdxs.IsEmpty () ? vertIdxs.GetSize () : nextContourStartIdxs[0];
            {
                const auto& vertexIds = polygon.AddList<Int32> ("vertexIds");
                for (ULong k = 0; k < outerEnd; ++k) {
                    vertexIds (static_cast<Int32> (vertIdxs[k]));
                }
            }
            if (!nextContourStartIdxs.IsEmpty ()) {
                const auto& holes = polygon.AddList<GS::ObjectState> ("holes");
                for (UIndex h = 0; h < nextContourStartIdxs.GetSize (); ++h) {
                    const ULong start = nextContourStartIdxs[h];
                    const ULong end = (h + 1 < nextContourStartIdxs.GetSize ()) ? nextContourStartIdxs[h + 1] : vertIdxs.GetSize ();
                    GS::ObjectState hole;
                    const auto& holeVertexIds = hole.AddList<Int32> ("vertexIds");
                    for (ULong k = start; k < end; ++k) {
                        holeVertexIds (static_cast<Int32> (vertIdxs[k]));
                    }
                    holes (hole);
                }
            }

            // API_OverriddenAttribute's shape changed at AC27 (APIOptional<API_AttributeIndex>,
            // fields hasValue/value) vs before (a plain struct, fields overridden/attributeIndex) -
            // same version split BuildMorphBodyFromGeometry already handles on the write side.
            // This is a SURFACE override (API_MaterialID), not a building material - confirmed
            // live (see "surfaceId" field note on the MorphBody schema definition).
            if (memo.morphMaterialMapTable != nullptr) {
                const API_OverriddenAttribute& faceMaterial = memo.morphMaterialMapTable[i];
#ifdef ServerMainVers_2700
                if (faceMaterial.hasValue) {
                    polygon.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, faceMaterial.value)));
                }
#else
                if (faceMaterial.overridden) {
                    polygon.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, faceMaterial.attributeIndex)));
                }
#endif
            }

            polygons (polygon);
        }
    }

    {
        GS::Array<GS::ObjectState> overrides;
        GS::Array<GS::ObjectState> wireEdges;
        const ULong edgeCount = meshBody.GetEdgeCount ();
        for (ULong i = 0; i < edgeCount; ++i) {
            const EDGE& e = meshBody.GetConstEdge (i);
            const Modeler::EdgeAttributes& attrs = meshBody.GetConstEdgeAttributes (i);
            const bool hidden = attrs.IsInvisible ();
            const bool smooth = !attrs.IsSharp ();
            const bool silhouetteOnly = attrs.IsVisibleIfContour ();

            // An edge with no adjacent polygon at all belongs to a wire-only loop (a "filled":
            // false face in the input, or standalone edges some other tool created) - it never
            // shows up in the "polygons" list above (which only ever walks GetPolygonCount()
            // faces), so without this it would be entirely invisible to a caller: the vertices it
            // references still appear in "vertices" (Archicad keeps its own separate vertex
            // instances for wire geometry), but nothing would explain what they're for.
            if (e.GetPolygonCount () == 0) {
                GS::ObjectState wireEdge;
                const auto& vertexIds = wireEdge.AddList<Int32> ("vertexIds");
                vertexIds (static_cast<Int32> (e.vert1));
                vertexIds (static_cast<Int32> (e.vert2));
                wireEdges.Push (wireEdge);
            }

            // Only emit an entry when it differs from the element-wide default, keeping
            // edgeOverrides sparse as documented in the schema.
            const bool defaultHidden = elem.morph.edgeType != APIMorphEdgeType_HardVisibleEdge;
            const bool defaultSmooth = elem.morph.edgeType == APIMorphEdgeType_SoftHiddenEdge;
            if (hidden == defaultHidden && smooth == defaultSmooth && !silhouetteOnly) {
                continue;
            }
            GS::ObjectState override_;
            const auto& vertexIds = override_.AddList<Int32> ("vertexIds");
            vertexIds (static_cast<Int32> (e.vert1));
            vertexIds (static_cast<Int32> (e.vert2));
            override_.Add ("hidden", hidden);
            override_.Add ("smooth", smooth);
            override_.Add ("silhouetteOnly", silhouetteOnly);
            overrides.Push (override_);
        }
        if (!overrides.IsEmpty ()) {
            const auto& edgeOverrides = body.AddList<GS::ObjectState> ("edgeOverrides");
            for (const auto& o : overrides) {
                edgeOverrides (o);
            }
        }
        if (!wireEdges.IsEmpty ()) {
            const auto& wireEdgesList = body.AddList<GS::ObjectState> ("wireEdges");
            for (const auto& w : wireEdges) {
                wireEdgesList (w);
            }
        }
    }

    typeSpecificDetails.Add ("body", body);
}

CreateWallsCommand::CreateWallsCommand () :
    CreateElementsCommandBase ("CreateWalls", API_WallID, "wallsData")
{
}

GS::Optional<GS::UniString> CreateWallsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "wallsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Story index (as returned by GetStories). When provided, zCoordinate is interpreted as bottomOffset relative to the floor. Takes priority over zCoordinate for floor assignment." },
                        "zCoordinate": { "type": "number", "description": "Absolute Z when floorIndex is absent; bottomOffset relative to the floor when floorIndex is provided." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "offset": { "type": "number" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero creates a curved wall (begCoordinate/endCoordinate are the chord endpoints)." },
                        "referenceLineLocation": {
                            "type": "string",
                            "enum": ["Outside", "Center", "Inside", "CoreOutside", "CoreCenter", "CoreInside"]
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite", "Profile"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate", "height", "thickness"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["wallsData"]
    })";
}

GS::Optional<GS::ObjectState> CreateWallsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo&, const Stories& stories, const GS::ObjectState& parameters) const
{
    if (parameters.Get ("begCoordinate") == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing required 'begCoordinate' parameter.");
    }
    if (parameters.Get ("endCoordinate") == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing required 'endCoordinate' parameter.");
    }
    API_Coord begCoordinate = Get2DCoordinateFromObjectState (*parameters.Get ("begCoordinate"));
    API_Coord endCoordinate = Get2DCoordinateFromObjectState (*parameters.Get ("endCoordinate"));

    if (IsSame2DCoordinate (begCoordinate, endCoordinate)) {
        return CreateErrorResponse (APIERR_BADPARS, "Zero-length wall: 'begCoordinate' and 'endCoordinate' are identical.");
    }

    double zCoordinate = 0.0;
    double height = 0.0;
    double thickness = 0.0;
    parameters.Get ("zCoordinate", zCoordinate);
    parameters.Get ("height", height);
    parameters.Get ("thickness", thickness);

    element.wall.type = APIWtyp_Normal;
    element.wall.begC = begCoordinate;
    element.wall.endC = endCoordinate;
    auto arcAngle = GetOptionalDouble (parameters, "arcAngle");
    if (arcAngle.HasValue ()) {
        element.wall.angle = arcAngle.Get ();
    }
    element.wall.height = height;
    element.wall.relativeTopStory = 0;
    element.wall.thickness = thickness;
    element.wall.referenceLineLocation = APIWallRefLine_Center;
    GS::UniString referenceLineLocation;
    if (parameters.Get ("referenceLineLocation", referenceLineLocation)) {
        if (referenceLineLocation == "Outside") {
            element.wall.referenceLineLocation = APIWallRefLine_Outside;
        } else if (referenceLineLocation == "Center") {
            element.wall.referenceLineLocation = APIWallRefLine_Center;
        } else if (referenceLineLocation == "Inside") {
            element.wall.referenceLineLocation = APIWallRefLine_Inside;
        } else if (referenceLineLocation == "CoreOutside") {
            element.wall.referenceLineLocation = APIWallRefLine_CoreOutside;
        } else if (referenceLineLocation == "CoreCenter") {
            element.wall.referenceLineLocation = APIWallRefLine_CoreCenter;
        } else if (referenceLineLocation == "CoreInside") {
            element.wall.referenceLineLocation = APIWallRefLine_CoreInside;
        }
    }
    element.wall.modelElemStructureType = API_BasicStructure;
    element.wall.offset = 0.0;

    auto offset = GetOptionalDouble (parameters, "offset");

    if (offset.HasValue ()) {
        element.wall.offset = offset.Get ();
    }

    Int32 explicitFloorIndex = -1;
    if (parameters.Get ("floorIndex", explicitFloorIndex)) {
        element.header.floorInd   = static_cast<short> (explicitFloorIndex);
        element.wall.bottomOffset = zCoordinate;
    } else {
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (zCoordinate, stories);
        element.header.floorInd   = floorIndexAndOffset.first;
        element.wall.bottomOffset = floorIndexAndOffset.second;
    }

    bool structureChanged = false;
    auto error = ApplyWallStructure (element, nullptr, parameters, structureChanged);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return {};
}

CreateBeamsCommand::CreateBeamsCommand () :
    CreateElementsCommandBase ("CreateBeams", API_BeamID, "beamsData")
{
}

GS::Optional<GS::UniString> CreateBeamsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "beamsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from zCoordinate." },
                        "zCoordinate": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": {
                            "type": "number",
                            "description": "Slant angle in radians. A non-zero value also switches the beam to slanted, unless isSlanted is given explicitly."
                        },
                        "isSlanted": {
                            "type": "boolean",
                            "description": "Optional explicit slanted state. By default it is derived from slantAngle."
                        },
                        "profileAngle": {
                            "type": "number",
                            "description": "Rotation angle of the profile around the beam's center line, in radians."
                        },
                        "arcAngle": { "type": "number" },
                        "verticalCurveHeight": { "type": "number" },
                        "width": {
                            "type": "number",
                            "description": "Cross section width of the beam. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "height": {
                            "type": "number",
                            "description": "Cross section height of the beam. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "anchorPoint": {
                            "type": "string",
                            "description": "Optional anchor point of the beam cross section on a 3x3 grid.",
                            "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"]
                        },
                        "isWidthAndHeightLinked": {
                            "type": "boolean",
                            "description": "When true (the default), Archicad keeps width and height equal and setting one changes the other - set to false to give width/height independent values. Applied to all segments."
                        },
                        "buildingMaterialId": {
                            "$ref": "#/AttributeId",
                            "description": "Cross section building material. Applied to all segments."
                        },
                        "profileId": {
                            "$ref": "#/AttributeId",
                            "description": "Switches the cross section to this custom extruded profile. Applied to all segments."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate", "zCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["beamsData"]
    })";
}

GS::Optional<GS::ObjectState> CreateBeamsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    if (parameters.Get ("begCoordinate") == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing required 'begCoordinate' parameter.");
    }
    if (parameters.Get ("endCoordinate") == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing required 'endCoordinate' parameter.");
    }
    element.beam.begC = Get2DCoordinateFromObjectState (*parameters.Get ("begCoordinate"));
    element.beam.endC = Get2DCoordinateFromObjectState (*parameters.Get ("endCoordinate"));

    double zCoordinate = 0.0;
    parameters.Get ("zCoordinate", zCoordinate);
    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", zCoordinate, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.beam.level = floorIndexAndOffset.second;

    auto offset = GetOptionalDouble (parameters, "offset");

    if (offset.HasValue ()) {
        element.beam.offset = offset.Get ();
    }
    auto slantAngle = GetOptionalDouble (parameters, "slantAngle");
    if (slantAngle.HasValue ()) {
        element.beam.slantAngle = slantAngle.Get ();
        // Without isSlanted the new beam is created horizontal and the angle is discarded,
        // see ApplyBeamDetails (#508).
        element.beam.isSlanted = (slantAngle.Get () != 0.0);
    }
    bool isSlanted = false;
    if (parameters.Get ("isSlanted", isSlanted)) {
        element.beam.isSlanted = isSlanted;
    }
    auto profileAngle = GetOptionalDouble (parameters, "profileAngle");
    if (profileAngle.HasValue ()) {
        element.beam.profileAngle = profileAngle.Get ();
    }
    auto arcAngle = GetOptionalDouble (parameters, "arcAngle");
    if (arcAngle.HasValue ()) {
        element.beam.curveAngle = arcAngle.Get ();
    }
    auto curveHeight = GetOptionalDouble (parameters, "verticalCurveHeight");
    if (curveHeight.HasValue ()) {
        element.beam.verticalCurveHeight = curveHeight.Get ();
    }

    GS::UniString anchorPoint;
    if (parameters.Get ("anchorPoint", anchorPoint)) {
        element.beam.anchorPoint = ParseAnchorPointString (anchorPoint);
    }

    auto width = GetOptionalDouble (parameters, "width");
    auto height = GetOptionalDouble (parameters, "height");
    bool isWidthAndHeightLinked = false;
    const bool hasIsWidthAndHeightLinked = parameters.Get ("isWidthAndHeightLinked", isWidthAndHeightLinked);
    const GS::ObjectState* buildingMaterialIdOs = parameters.Get ("buildingMaterialId");
    const GS::ObjectState* profileIdOs = parameters.Get ("profileId");

    if ((width.HasValue () || height.HasValue () || hasIsWidthAndHeightLinked || buildingMaterialIdOs != nullptr || profileIdOs != nullptr) && memo.beamSegments != nullptr) {
        GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr>(memo.beamSegments)) / sizeof (API_BeamSegmentType);
        for (GSSize i = 0; i < nSegments; ++i) {
            API_AssemblySegmentData& segment = memo.beamSegments[i].assemblySegmentData;
            if (hasIsWidthAndHeightLinked) {
                segment.isWidthAndHeightLinked = isWidthAndHeightLinked;
            }
            if (width.HasValue ()) {
                segment.nominalWidth = width.Get ();
            }
            if (height.HasValue ()) {
                segment.nominalHeight = height.Get ();
            }
            if (profileIdOs != nullptr) {
                segment.modelElemStructureType = API_ProfileStructure;
                segment.profileAttr = GetAttributeIndexFromGuid (API_ProfileID, GetGuidFromObjectState (*profileIdOs));
            } else if (buildingMaterialIdOs != nullptr) {
                segment.modelElemStructureType = API_BasicStructure;
                segment.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialIdOs));
            }
        }
    }

    return {};
}

CreateStairsCommand::CreateStairsCommand () :
    CreateElementsCommandBase ("CreateStairs", API_StairID, "stairsData")
{
}

GS::Optional<GS::UniString> CreateStairsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "stairsData": {
                "type": "array",
                "description": "Array of data to create Stair elements.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Stair.",
                    "properties": {
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
                        "baseLinePoints": {
                            "type": "array",
                            "description": "2D coordinates defining the stair baseline polyline. Minimum 2 points for a straight stair, 3+ for L-shaped or U-shaped stairs.",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 2
                        },
                        "zCoordinate": {
                            "type": "number",
                            "description": "The Z coordinate (absolute elevation) of the stair base."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from zCoordinate."
                        },
                        "totalHeight": {
                            "type": "number",
                            "description": "Total height of the stair.",
                            "exclusiveMinimum": 0.0
                        },
                        "flightWidth": {
                            "type": "number",
                            "description": "Width of the stair flight.",
                            "exclusiveMinimum": 0.0
                        },
                        "stepNum": {
                            "type": "integer",
                            "description": "Number of risers (steps).",
                            "minimum": 1
                        },
                        "riserHeight": {
                            "type": "number",
                            "description": "Height of each riser.",
                            "exclusiveMinimum": 0.0
                        },
                        "treadDepth": {
                            "type": "number",
                            "description": "Depth (going) of each tread.",
                            "exclusiveMinimum": 0.0
                        },
                        "finishVisible": {
                            "type": "boolean",
                            "description": "Optional. If false, the tread/riser finishes are hidden and only the stair structure (e.g. a monolith) is modeled."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["baseLinePoints", "zCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["stairsData"]
    })";
}

GS::Optional<GS::ObjectState> CreateStairsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::Array<GS::ObjectState> baseLinePoints;
    parameters.Get ("baseLinePoints", baseLinePoints);
    if (baseLinePoints.GetSize () < 2) {
        return CreateErrorResponse (APIERR_BADPARS, "baseLinePoints must have at least 2 points.");
    }

    double zCoordinate = 0.0;
    parameters.Get ("zCoordinate", zCoordinate);
    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", zCoordinate, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.stair.basePlane.basePoint.z = floorIndexAndOffset.second;

    bool finishVisible = true;
    if (parameters.Get ("finishVisible", finishVisible)) {
        element.stair.finishVisible = finishVisible;
        for (int role = 0; role < API_StairPartRoleNum; ++role) {
            element.stair.tread[role].visible = finishVisible;
            element.stair.riser[role].visible = finishVisible;
        }
    }

    auto totalHeight = GetOptionalDouble (parameters, "totalHeight");
    if (totalHeight.HasValue ()) {
        element.stair.totalHeight = totalHeight.Get ();
    }
    auto flightWidth = GetOptionalDouble (parameters, "flightWidth");
    if (flightWidth.HasValue ()) {
        element.stair.flightWidth = flightWidth.Get ();
    }

    Int32 stepNum = 0;
    if (parameters.Get ("stepNum", stepNum)) {
        element.stair.stepNum = static_cast<UInt32> (stepNum);
    }

    auto riserHeight = GetOptionalDouble (parameters, "riserHeight");
    if (riserHeight.HasValue ()) {
        element.stair.riserHeight = riserHeight.Get ();
    }
    auto treadDepth = GetOptionalDouble (parameters, "treadDepth");
    if (treadDepth.HasValue ()) {
        element.stair.treadDepth = treadDepth.Get ();
    }

    // Build the baseline polyline in the memo
    const Int32 nCoords = static_cast<Int32> (baseLinePoints.GetSize ());

    // Free existing baseline handles from defaults
    if (memo.stairBaseLine.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*>(&memo.stairBaseLine.coords));
    }
    if (memo.stairBaseLine.pends != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*>(&memo.stairBaseLine.pends));
    }
    if (memo.stairBaseLine.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*>(&memo.stairBaseLine.parcs));
    }
    if (memo.stairBaseLine.edgeData != nullptr) {
        BMKillPtr (reinterpret_cast<GSPtr*>(&memo.stairBaseLine.edgeData));
    }
    if (memo.stairBaseLine.vertexData != nullptr) {
        BMKillPtr (reinterpret_cast<GSPtr*>(&memo.stairBaseLine.vertexData));
    }

    // Allocate new baseline: polyline (not polygon), so no closing vertex needed
    // Coords: index 1..nCoords (1-based), index 0 unused
    // edgeData/vertexData are intentionally left null: ACAPI_Element_Create derives them
    // from the baseline geometry (see the Element_Test example in the DevKit). Pre-filling
    // them marks every edge as a steps segment, which makes multi-segment (L/U-shaped)
    // baselines fail with -2130313215 (#444).
    memo.stairBaseLine.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.stairBaseLine.pends = reinterpret_cast<Int32**> (BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0));
    memo.stairBaseLine.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (0, ALLOCATE_CLEAR, 0));

    for (Int32 i = 0; i < nCoords; ++i) {
        (*memo.stairBaseLine.coords)[i + 1] = Get2DCoordinateFromObjectState (baseLinePoints[i]);
    }

    (*memo.stairBaseLine.pends)[1] = nCoords;

    memo.stairBaseLine.polygon.nCoords = nCoords;
    memo.stairBaseLine.polygon.nSubPolys = 1;
    memo.stairBaseLine.polygon.nArcs = 0;

    return {};
}

CreateWindowsCommand::CreateWindowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateWindowsCommand::GetName () const
{
    return "CreateWindows";
}

GS::Optional<GS::UniString> CreateWindowsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "windowsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerWallId": { "$ref": "#/ElementId" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional. Name of an existing Window favorite (as returned by `GetFavoritesByType`). Applied to the Window tool defaults before the create."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["ownerWallId", "centerOffset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["windowsData"]
    })";
}

GS::Optional<GS::UniString> CreateWindowsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateWindowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> windowsData;
    auto error = GetElementArray (parameters, "windowsData", windowsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    // The create needs the floor plan as the current database - see
    // SwitchCurrentDatabaseToFloorPlan. The previous database is restored after the
    // undoable command has finished, on every exit path.
    API_DatabaseInfo previousDatabase = {};
    bool databaseSwitched = false;
    const GSErrCode databaseErr = SwitchCurrentDatabaseToFloorPlan (previousDatabase, databaseSwitched);
    const GS::OnExit restoreDatabase ([&]() {
        if (databaseSwitched) {
            ACAPI_Database_ChangeCurrentDatabase (&previousDatabase);
        }
    });
    if (databaseErr != NoError) {
        return CreateErrorResponse (databaseErr,
            "Failed to activate the floor plan database, which is needed to create a window. Activate the floor plan in Archicad and run the command again.");
    }

    return ExecuteCreateWithElements ("Create Windows", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : windowsData) {
            if (data.Get ("ownerWallId") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'ownerWallId' field."));
                continue;
            }
            const API_Guid wallGuid = GetGuidFromObjectState (*data.Get ("ownerWallId"));
            if (!DoesWallExist (wallGuid)) {
                elements.Push (CreateErrorResponse (APIERR_BADID, "Failed to load owner wall."));
                continue;
            }
            if (IsPolygonalWall (wallGuid)) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS,
                    "The owner wall is polygonal, and Archicad cannot place a window in one."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            API_SubElement marker = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
                ACAPI_DisposeElemMemoHdls (&marker.memo);
            });

            // Apply the favorite to the Window tool defaults FIRST so
            // that PrepareWindowOrDoorDefaults clones the favorite-applied
            // libpart and builds a matching marker. Calling
            // PrepareWindowOrDoorDefaults first and applying the favorite
            // after leaves the marker pointing at the previous libpart,
            // causing CreateExt to fail with -2130313110.
            GSErrCode err = ApplyWindowOrDoorFavoriteToDefaults (data, API_WindowID);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to resolve `favoriteName` for window."));
                continue;
            }

            err = PrepareWindowOrDoorDefaults (API_WindowID, element, memo, marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare window defaults."));
                continue;
            }

            double centerOffset = 0.0;
            data.Get ("centerOffset", centerOffset);
            element.window.owner = wallGuid;
            element.window.objLoc = centerOffset;
            auto sillHeight = GetOptionalDouble (data, "sillHeight");
            if (sillHeight.HasValue ()) {
                element.window.lower = sillHeight.Get ();
            }
            auto width = GetOptionalDouble (data, "width");
            if (width.HasValue ()) {
                element.window.openingBase.width = width.Get ();
            }
            auto height = GetOptionalDouble (data, "height");
            if (height.HasValue ()) {
                element.window.openingBase.height = height.Get ();
            }
            bool reflected = false;
            if (data.Get ("reflected", reflected)) {
                element.window.openingBase.reflected = reflected;
            }
            bool refSide = false;
            if (data.Get ("refSide", refSide)) {
                element.window.openingBase.refSide = refSide;
            }
            bool oSide = false;
            if (data.Get ("oSide", oSide)) {
                element.window.openingBase.oSide = oSide;
            }

            err = ACAPI_Element_CreateExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                // The switch above should have ruled this one out, but say what it means
                // if Archicad still refuses the database - #532 was reported for a year as
                // a parameter problem because the message named none of this.
                GS::UniString errorMessage = "Failed to create window.";
                if (err == APIERR_BADDATABASE) {
                    errorMessage = "Failed to create window: Archicad refused the current database. A window can only be created while the floor plan is the current database.";
                }
                elements.Push (CreateErrorResponse (err, errorMessage));
                continue;
            }
            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateDoorsCommand::CreateDoorsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDoorsCommand::GetName () const
{
    return "CreateDoors";
}

GS::Optional<GS::UniString> CreateDoorsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "doorsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerWallId": { "$ref": "#/ElementId" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional. Name of an existing Door favorite (as returned by `GetFavoritesByType`). Applied to the Door tool defaults before the create."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["ownerWallId", "centerOffset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["doorsData"]
    })";
}

GS::Optional<GS::UniString> CreateDoorsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateDoorsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> doorsData;
    auto error = GetElementArray (parameters, "doorsData", doorsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    // The create needs the floor plan as the current database - see
    // SwitchCurrentDatabaseToFloorPlan. The previous database is restored after the
    // undoable command has finished, on every exit path.
    API_DatabaseInfo previousDatabase = {};
    bool databaseSwitched = false;
    const GSErrCode databaseErr = SwitchCurrentDatabaseToFloorPlan (previousDatabase, databaseSwitched);
    const GS::OnExit restoreDatabase ([&]() {
        if (databaseSwitched) {
            ACAPI_Database_ChangeCurrentDatabase (&previousDatabase);
        }
    });
    if (databaseErr != NoError) {
        return CreateErrorResponse (databaseErr,
            "Failed to activate the floor plan database, which is needed to create a door. Activate the floor plan in Archicad and run the command again.");
    }

    return ExecuteCreateWithElements ("Create Doors", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : doorsData) {
            if (data.Get ("ownerWallId") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'ownerWallId' field."));
                continue;
            }
            const API_Guid wallGuid = GetGuidFromObjectState (*data.Get ("ownerWallId"));
            if (!DoesWallExist (wallGuid)) {
                elements.Push (CreateErrorResponse (APIERR_BADID, "Failed to load owner wall."));
                continue;
            }
            if (IsPolygonalWall (wallGuid)) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS,
                    "The owner wall is polygonal, and Archicad cannot place a door in one."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            API_SubElement marker = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
                ACAPI_DisposeElemMemoHdls (&marker.memo);
            });

            // Apply the favorite to the Door tool defaults FIRST so
            // that PrepareWindowOrDoorDefaults clones the favorite-applied
            // libpart and builds a matching marker. Calling
            // PrepareWindowOrDoorDefaults first and applying the favorite
            // after leaves the marker pointing at the previous libpart,
            // causing CreateExt to fail with -2130313110.
            GSErrCode err = ApplyWindowOrDoorFavoriteToDefaults (data, API_DoorID);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to resolve `favoriteName` for door."));
                continue;
            }

            err = PrepareWindowOrDoorDefaults (API_DoorID, element, memo, marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare door defaults."));
                continue;
            }

            double centerOffset = 0.0;
            data.Get ("centerOffset", centerOffset);
            element.window.owner = wallGuid;
            element.window.objLoc = centerOffset;
            auto sillHeight = GetOptionalDouble (data, "sillHeight");
            if (sillHeight.HasValue ()) {
                element.window.lower = sillHeight.Get ();
            }
            auto width = GetOptionalDouble (data, "width");
            if (width.HasValue ()) {
                element.window.openingBase.width = width.Get ();
            }
            auto height = GetOptionalDouble (data, "height");
            if (height.HasValue ()) {
                element.window.openingBase.height = height.Get ();
            }
            bool reflected = false;
            if (data.Get ("reflected", reflected)) {
                element.window.openingBase.reflected = reflected;
            }
            bool refSide = false;
            if (data.Get ("refSide", refSide)) {
                element.window.openingBase.refSide = refSide;
            }
            bool oSide = false;
            if (data.Get ("oSide", oSide)) {
                element.window.openingBase.oSide = oSide;
            }

            err = ACAPI_Element_CreateExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                // See the same spot in CreateWindowsCommand::Execute.
                GS::UniString errorMessage = "Failed to create door.";
                if (err == APIERR_BADDATABASE) {
                    errorMessage = "Failed to create door: Archicad refused the current database. A door can only be created while the floor plan is the current database.";
                }
                elements.Push (CreateErrorResponse (err, errorMessage));
                continue;
            }
            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateOpeningsCommand::CreateOpeningsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateOpeningsCommand::GetName () const
{
    return "CreateOpenings";
}

GS::Optional<GS::UniString> CreateOpeningsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "openingsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerElementId": { "$ref": "#/ElementId" },
                        "basePoint": { "$ref": "#/Coordinate3D" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 }
                    },
                    "additionalProperties": false,
                    "required": ["ownerElementId", "basePoint"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["openingsData"]
    })";
}

GS::Optional<GS::UniString> CreateOpeningsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

#ifndef ServerMainVers_2900
// How far above the opening's bottom edge its anchor point sits. anchorAltitude is
// measured to the anchor, so placing the bottom at a requested altitude means adding
// this back - APIAnc_?T anchors sit a full height above the bottom, APIAnc_?M half of
// it, APIAnc_?B on it. The horizontal half of the anchor is irrelevant here and is
// deliberately left alone, so the existing horizontal placement is unchanged.
static double GetAnchorHeightOffset (API_AnchorID anchor, double height)
{
    switch (anchor) {
        case APIAnc_LT:
        case APIAnc_MT:
        case APIAnc_RT:
            return height;
        case APIAnc_LM:
        case APIAnc_MM:
        case APIAnc_RM:
            return height / 2.0;
        default:
            return 0.0;
    }
}
#endif

GS::ObjectState CreateOpeningsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> openingsData;
    auto error = GetElementArray (parameters, "openingsData", openingsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

#ifndef ServerMainVers_2900
    const Stories stories = GetStories ();
#endif

    return ExecuteCreateWithElements ("Create Openings", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : openingsData) {
            if (data.Get ("basePoint") == nullptr || data.Get ("ownerElementId") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'basePoint' or 'ownerElementId' field."));
                continue;
            }
            const auto sizeError = CheckOpeningSize (data);
            if (sizeError.HasValue ()) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, sizeError.Get ()));
                continue;
            }
            const API_Coord3D basePoint = Get3DCoordinateFromObjectState (*data.Get ("basePoint"));

#ifndef ServerMainVers_2900
            API_Element element = {};
#ifdef ServerMainVers_2600
            element.header.type   = API_OpeningID;
#else
            element.header.typeID = API_OpeningID;
#endif
            GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare opening defaults."));
                continue;
            }

            element.opening.owner = GetGuidFromObjectState (*data.Get ("ownerElementId"));
            element.opening.extrusionGeometryData.frame.basePoint = basePoint;
            element.opening.extrusionGeometryData.frame.axisX = {-1.0, 0.0, 0.0};
            element.opening.extrusionGeometryData.frame.axisY = {0.0, 0.0, 1.0};
            element.opening.extrusionGeometryData.frame.axisZ = {0.0, 1.0, 0.0};

            API_OpeningExtrusionParameters& extrusionParameters = element.opening.extrusionGeometryData.parameters;
            data.Get ("width", extrusionParameters.width);
            data.Get ("height", extrusionParameters.height);

            // The extrusion frame's base point does NOT position the opening vertically:
            // with a horizontal or aligned constraint the vertical position comes from
            // anchorAltitude, measured from the home story up to the anchor point, and the
            // base point's Z is discarded. With the stock centre anchor that put every
            // opening's middle at the default altitude - the reporter of #533 measured
            // sill == storyLevel + 1.0 - height/2 on all 52 openings of a project, whatever
            // Z was sent. Convert the requested Z into the altitude the configured anchor
            // expects instead, so basePoint places the opening in all three axes, the way
            // the AC29 PlacePolygonal branch already treats it.
            if (extrusionParameters.constraint == API_OpeningForcedHorizontal ||
                extrusionParameters.constraint == API_OpeningAligned) {
                const double bottomAboveStory = basePoint.z - GetZPos (element.header.floorInd, 0.0, stories);
                extrusionParameters.anchorAltitude =
                    bottomAboveStory + GetAnchorHeightOffset (extrusionParameters.anchor, extrusionParameters.height);
            }

            err = ACAPI_Element_Create (&element, nullptr);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create opening."));
                continue;
            }
            elements.Push (CreateElementIdObjectState (element.header.guid));
#else
            ACAPI::Result<ACAPI::Element::OpeningDefault> openingDefault = ACAPI::Element::CreateOpeningDefault ();
            if (openingDefault.IsErr ()) {
                elements.Push (CreateErrorResponse (openingDefault.UnwrapErr ().kind, GS::UniString (openingDefault.UnwrapErr ().text.c_str ())));
                continue;
            }

            double width = 0.0;
            double height = 0.0;
            data.Get ("width", width);
            data.Get ("height", height);
            GS::Array<Point2D> polygonCorners { {0, 0}, {width, 0}, {width, height}, {0, height} };
            Geometry::Polygon2D polygon = Geometry::Polygon2D::Create (polygonCorners, 0 /*Geometry::PolyCreateFlags*/).PopLargest ();

            // PlacePolygonal hangs the base polygon BELOW the point it is given, so the point
            // ends up on the opening's top edge rather than its bottom one - measured on AC29,
            // the created opening's sill came back at exactly basePoint.z - height for every
            // height tried (0.5, 0.8, 1.0, 1.5, 2.0, 2.5). Raising the placement point by the
            // height puts the bottom edge on the requested Z, which is what basePoint means in
            // every other creation command and what the pre-AC29 branch now produces (#533).
            //
            // The correction is applied to the point rather than to the polygon because the
            // polygon's own offset is ignored: building the corners over -height..0 instead of
            // 0..height changed nothing at all, so PlacePolygonal evidently normalizes the
            // polygon and keeps only its extents.
            API_Coord3D placementPoint = basePoint;
            placementPoint.z += height;

            ACAPI::UniqueID parentElemId (APIGuid2GSGuid (GetGuidFromObjectState (*data.Get ("ownerElementId"))), ACAPI_GetToken ());
            ACAPI::Result<ACAPI::UniqueID> resultId = openingDefault->PlacePolygonal (parentElemId, placementPoint, polygon);
            if (resultId.IsErr ()) {
                elements.Push (CreateErrorResponse (resultId.UnwrapErr ().kind, GS::UniString (resultId.UnwrapErr ().text.c_str())));
                continue;
            }
            elements.Push (CreateElementIdObjectState (GSGuid2APIGuid (resultId.Unwrap ().GetGuid ())));
#endif
        }
    });
}

CreateMorphsCommand::CreateMorphsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateMorphsCommand::GetName () const
{
    return "CreateMorphs";
}

GS::Optional<GS::UniString> CreateMorphsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "morphsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "basePoint": { "$ref": "#/Coordinate3D" },
                        "size": {
                            "$ref": "#/Dimensions3D",
                            "description": "Builds a simple axis-aligned box of this size. Mutually exclusive with `body` - give exactly one of the two."
                        },
                        "body": {
                            "$ref": "#/MorphBody",
                            "description": "Builds arbitrary geometry (any number of faces, holes, per-face materials, edge display overrides) instead of a simple box. Mutually exclusive with `size` - give exactly one of the two."
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "xAxis": { "$ref": "#/Coordinate3D" },
                        "yAxis": { "$ref": "#/Coordinate3D" },
                        "zAxis": { "$ref": "#/Coordinate3D" },
                        "surfaceId": { "$ref": "#/AttributeId" },
                        "castShadow": { "type": "boolean" },
                        "receiveShadow": { "type": "boolean" },
                        "isAutoOnStoryVisibility": { "type": "boolean" },
                        "showContour": { "$ref": "#/StoryVisibility" },
                        "showFill": { "$ref": "#/StoryVisibility" },
                        "linkToSettings": {
                            "type": "object",
                            "properties": {
                                "homeStoryDifference": { "type": "integer" },
                                "newCreationMode": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        },
                        "displayOption": {
                            "type": "string",
                            "enum": ["Standard", "StandardWithAbstract", "CutOnly", "OutLinesOnly", "AbstractAll", "CutAll"]
                        },
                        "viewDepthLimitation": {
                            "type": "string",
                            "enum": ["ToFloorPlanRange", "ToAbsoluteLimit", "EntireElement"]
                        },
                        "cutFillPen": { "type": "integer" },
                        "cutFillBackgroundPen": { "type": "integer" },
                        "cutLineType": { "$ref": "#/AttributeId" },
                        "cutLinePen": { "type": "integer" },
                        "uncutLineType": { "$ref": "#/AttributeId" },
                        "uncutLinePen": { "type": "integer" },
                        "overheadLineType": { "$ref": "#/AttributeId" },
                        "overheadLinePen": { "type": "integer" },
                        "useCoverFillType": { "type": "boolean" },
                        "outlineContourDisplay": { "type": "boolean" },
                        "coverFillType": { "$ref": "#/AttributeId" },
                        "coverFillPen": { "type": "integer" },
                        "coverFillBGPen": { "type": "integer" },
                        "use3DHatching": { "type": "boolean" },
                        "coverFillOrientation": {
                            "type": "object",
                            "properties": {
                                "type": { "type": "string", "enum": ["Global", "Rotated", "Distorted", "Centered"] },
                                "origo": { "$ref": "#/Coordinate2D" },
                                "matrix00": { "type": "number" },
                                "matrix10": { "type": "number" },
                                "matrix01": { "type": "number" },
                                "matrix11": { "type": "number" },
                                "innerRadius": { "type": "number" }
                            },
                            "additionalProperties": false
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "type": "string",
                            "enum": ["Invalid", "Planar", "Default", "Cylindric", "Spheric", "Box"]
                        },
                        "textureProjectionCoords": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate3D" },
                            "minItems": 4,
                            "maxItems": 4
                        },
                        "level": { "type": "number" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from the basePoint's z value." }
                    },
                    "additionalProperties": false,
                    "required": ["basePoint"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["morphsData"]
    })";
}

GS::Optional<GS::UniString> CreateMorphsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateMorphsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> morphsData;
    auto error = GetElementArray (parameters, "morphsData", morphsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    const Stories stories = GetStories ();

    return ExecuteCreateWithElements ("Create Morphs", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : morphsData) {
            API_Element element = {};
            #ifdef ServerMainVers_2600
            element.header.type = API_MorphID;
            #else
            element.header.typeID = API_MorphID;
            #endif
            GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare morph defaults."));
                continue;
            }

            if (data.Get ("basePoint") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'basePoint' field."));
                continue;
            }
            const GS::ObjectState* sizeOS = data.Get ("size");
            const GS::ObjectState* bodyOS = data.Get ("body");
            if ((sizeOS == nullptr) == (bodyOS == nullptr)) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Give exactly one of 'size' (a simple box) or 'body' (arbitrary geometry)."));
                continue;
            }
            const API_Coord3D basePoint = Get3DCoordinateFromObjectState (*data.Get ("basePoint"));

            // GetDefaults leaves floorInd at whatever story is currently active in the UI,
            // regardless of basePoint's z - a morph built far above/below that story's own
            // elevation would get correctly placed in absolute 3D space (tmx below stays absolute)
            // but assigned to the wrong story for floor-plan/story-based queries. Only floorInd is
            // derived here; tmx[11] intentionally stays basePoint.z (absolute), matching how the
            // rest of this command already places the morph in world space.
            element.header.floorInd = ResolveFloorIndexAndOffset (data, "floorIndex", basePoint.z, stories).first;

            auto buildingMaterialId = GetOptionalObjectState (data, "buildingMaterialId");

            if (buildingMaterialId.HasValue ()) {
                API_AttributeIndex buildingMaterialIndex = APIInvalidAttributeIndex;
                if (!ResolveAttributeIndex (buildingMaterialId.Get (), API_BuildingMaterialID, buildingMaterialIndex)) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, "Invalid morph building material."));
                    continue;
                }
                element.morph.buildingMaterial = buildingMaterialIndex;
            }

            double* tmx = element.morph.tranmat.tmx;
            tmx[0] = 1.0;  tmx[4] = 0.0;  tmx[8] = 0.0;
            tmx[1] = 0.0;  tmx[5] = 1.0;  tmx[9] = 0.0;
            tmx[2] = 0.0;  tmx[6] = 0.0;  tmx[10] = 1.0;
            tmx[3] = basePoint.x;
            tmx[7] = basePoint.y;
            tmx[11] = basePoint.z;

            {
                int appliedCount = 0;
                GS::UniString cosmeticError;
                if (!ApplyMorphCosmeticDetails (data, element, nullptr, appliedCount, cosmeticError)) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, cosmeticError));
                    continue;
                }
            }

            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });

            if (sizeOS != nullptr) {
                const API_Coord3D size = Get3DCoordinateFromObjectState (*sizeOS);
                if (size.x <= 0.0 || size.y <= 0.0 || size.z <= 0.0) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, "Morph 'size' values must be positive."));
                    continue;
                }
                if (!BuildCuboidMorphMemo (size.x, size.y, size.z, element.morph.buildingMaterial, memo)) {
                    elements.Push (CreateErrorResponse (APIERR_GENERAL, "Failed to build morph body."));
                    continue;
                }
            } else {
                GS::UniString bodyError;
                if (!BuildMorphBodyFromGeometry (*bodyOS, memo, bodyError)) {
                    elements.Push (CreateErrorResponse (APIERR_GENERAL, bodyError));
                    continue;
                }
                GS::UniString bodyType;
                if (bodyOS->Get ("bodyType", bodyType) && bodyType == "Surface") {
                    element.morph.bodyType = APIMorphBodyType_SurfaceBody;
                } else {
                    element.morph.bodyType = APIMorphBodyType_SolidBody;
                }
                GS::UniString edgeDefault;
                if (bodyOS->Get ("edgeDefault", edgeDefault)) {
                    if (edgeDefault == "HardHidden") {
                        element.morph.edgeType = APIMorphEdgeType_HardHiddenEdge;
                    } else if (edgeDefault == "SoftHidden") {
                        element.morph.edgeType = APIMorphEdgeType_SoftHiddenEdge;
                    } else {
                        element.morph.edgeType = APIMorphEdgeType_HardVisibleEdge;
                    }
                }
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create morph."));
                continue;
            }

            // element.morph.material (the default-surface override, "surfaceId" on this command)
            // is confirmed live to be silently ignored by ACAPI_Element_Create - the exact same
            // value only takes effect through a follow-up ACAPI_Element_Change. Not a version-
            // specific issue (reproduced identically on AC27 and AC29): unlike every other
            // ApplyMorphCosmeticDetails field (which DOES take effect via Create), this one field
            // needs this extra step, done transparently here so CreateMorphs' own "surfaceId"
            // input behaves as documented.
            if (GetOptionalObjectState (data, "surfaceId").HasValue ()) {
                API_Element materialMask = {};
                ACAPI_ELEMENT_MASK_CLEAR (materialMask);
                ACAPI_ELEMENT_MASK_SET (materialMask, API_MorphType, material);
                ACAPI_Element_Change (&element, &materialMask, nullptr, 0, true);
            }

            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateRoofsCommand::CreateRoofsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateRoofsCommand::GetName () const
{
    return "CreateRoofs";
}

GS::Optional<GS::UniString> CreateRoofsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "roofsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "level": { "type": "number" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from level." },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "polygonCoordinates": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": { "$ref": "#/Holes2D" },
                        "pivotLine": {
                            "type": "object",
                            "description": "If given, a single-plane roof is created instead of a multi-plane roof: one plane tilted along this pivot line. The plane rises on the left side of the line direction (begCoordinate towards endCoordinate); flip the line to tilt towards the other side.",
                            "properties": {
                                "begCoordinate": { "$ref": "#/Coordinate2D" },
                                "endCoordinate": { "$ref": "#/Coordinate2D" }
                            },
                            "additionalProperties": false,
                            "required": ["begCoordinate", "endCoordinate"]
                        },
                        "angle": {
                            "type": "number",
                            "description": "Slope angle of the single-plane roof in radians. Only valid together with 'pivotLine'.",
                            "exclusiveMinimum": 0.0
                        },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "type": "object",
                                "properties": {
                                    "levelHeight": { "type": "number" },
                                    "levelAngle": { "type": "number", "exclusiveMinimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["levelHeight", "levelAngle"]
                            }
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["level", "polygonCoordinates"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["roofsData"]
    })";
}

GS::Optional<GS::UniString> CreateRoofsCommand::GetRawResponseSchema () const
{
    return CreateMorphsCommand ().GetRawResponseSchema ();
}

GS::ObjectState CreateRoofsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> roofsData;
    auto error = GetElementArray (parameters, "roofsData", roofsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    const Stories stories = GetStories ();

    return ExecuteCreateWithElements ("Create Roofs", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : roofsData) {
            API_Element element = {};
            #ifdef ServerMainVers_2600
            element.header.type = API_RoofID;
            #else
            element.header.typeID = API_RoofID;
            #endif
            GS::ObjectState pivotLine;
            const bool isSinglePlane = data.Get ("pivotLine", pivotLine);
            element.roof.roofClass = isSinglePlane ? API_PlaneRoofID : API_PolyRoofID;
            GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare roof defaults."));
                continue;
            }

            GS::Array<GS::ObjectState> roofLevels;
            if (isSinglePlane && (data.Get ("levels", roofLevels) || GetOptionalDouble (data, "eavesOverhang").HasValue ())) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "'levels' and 'eavesOverhang' are only valid for multi-plane roofs (omit 'pivotLine' to create one)."));
                continue;
            }
            if (!isSinglePlane && GetOptionalDouble (data, "angle").HasValue ()) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "'angle' is only valid for single-plane roofs (add 'pivotLine' to create one)."));
                continue;
            }

            bool changed = false;
            {
                auto error = ApplyRoofStructure (element, nullptr, data, changed);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }
            {
                auto error = ApplyRoofDetails (element, nullptr, data, stories, changed);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }

            GS::Array<GS::ObjectState> polygonOutline;
            GS::Array<GS::ObjectState> polygonArcs;
            GS::Array<GS::ObjectState> holes;
            data.Get ("polygonCoordinates", polygonOutline);
            data.Get ("polygonArcs", polygonArcs);
            data.Get ("holes", holes);

            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });
            {
                auto error = isSinglePlane
                    ? BuildPlaneRoofMemoFromGeometry (element, memo, polygonOutline, polygonArcs, holes)
                    : BuildRoofMemoFromGeometry (element, memo, polygonOutline, polygonArcs, holes);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }

            if (isSinglePlane) {
                GS::ObjectState begCoordinate;
                GS::ObjectState endCoordinate;
                if (!pivotLine.Get ("begCoordinate", begCoordinate) || !pivotLine.Get ("endCoordinate", endCoordinate)) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, "'pivotLine' must contain 'begCoordinate' and 'endCoordinate'."));
                    continue;
                }
                if (IsSame2DCoordinate (begCoordinate, endCoordinate)) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, "Zero-length pivot line: 'begCoordinate' and 'endCoordinate' are identical."));
                    continue;
                }
                element.roof.u.planeRoof.baseLine.c1 = Get2DCoordinateFromObjectState (begCoordinate);
                element.roof.u.planeRoof.baseLine.c2 = Get2DCoordinateFromObjectState (endCoordinate);
                element.roof.u.planeRoof.posSign = true;
                auto angle = GetOptionalDouble (data, "angle");
                if (angle.HasValue ()) {
                    element.roof.u.planeRoof.angle = angle.Get ();
                }
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create roof."));
                continue;
            }

            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateAssociativeDimensionsCommand::CreateAssociativeDimensionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAssociativeDimensionsCommand::GetName () const
{
    return "CreateAssociativeDimensions";
}

GS::Optional<GS::UniString> CreateAssociativeDimensionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "direction": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "number" },
                        "witnessPoints": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "elementId": { "$ref": "#/ElementId" },
                                    "line": { "type": "boolean" },
                                    "inIndex": { "type": "integer" },
                                    "special": { "type": "integer" },
                                    "nodeType": { "type": "integer" },
                                    "nodeStatus": { "type": "integer" },
                                    "nodeId": { "type": "number", "minimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["elementId"]
                            },
                            "minItems": 2
                        }
                    },
                    "additionalProperties": false,
                    "required": ["referencePoint", "direction", "witnessPoints"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    })";
}

GS::Optional<GS::UniString> CreateAssociativeDimensionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateAssociativeDimensionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> dimensionsData;
    auto error = GetElementArray (parameters, "dimensionsData", dimensionsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Associative Dimensions", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : dimensionsData) {
            GS::Array<GS::ObjectState> witnessPointsData;
            {
                auto error = GetElementArray (data, "witnessPoints", witnessPointsData);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }

            if (data.Get ("direction") == nullptr || data.Get ("referencePoint") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'direction' or 'referencePoint' field."));
                continue;
            }
            const API_Coord directionCoord = Get2DCoordinateFromObjectState (*data.Get ("direction"));
            if (directionCoord.x == 0.0 && directionCoord.y == 0.0) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Dimension direction must be non-zero."));
                continue;
            }

            GS::Array<AssociativeDimensionPoint> witnessPoints;
            bool invalidWitnessPoint = false;
            for (const auto& witnessPointData : witnessPointsData) {
                AssociativeDimensionPoint witnessPoint;
                auto error = ParseAssociativeDimensionPoint (witnessPointData, witnessPoint);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    invalidWitnessPoint = true;
                    break;
                }
                witnessPoints.Push (witnessPoint);
            }
            if (invalidWitnessPoint) {
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });

            #ifdef ServerMainVers_2600
            element.header.type = API_DimensionID;
            #else
            element.header.typeID = API_DimensionID;
            #endif
            GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare associative dimension defaults."));
                continue;
            }

            TryApplyDimensionFloorIndex (witnessPoints, GetOptionalDouble (data, "floorIndex"), element);
            FillDimensionDefaults (
                element,
                Get2DCoordinateFromObjectState (*data.Get ("referencePoint")),
                {directionCoord.x, directionCoord.y}
            );

            auto error = PopulateAssociativeDimensionMemo (witnessPoints, element, memo);
            if (error.HasValue ()) {
                elements.Push (CreateErrorResponse (APIERR_MEMFULL, error.Get ()));
                continue;
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create associative dimension."));
                continue;
            }

            // With a section/elevation window active, ACAPI_Element_Create can report success
            // and hand back a GUID although no dimension was created in any database (#510).
            // Verify the element really exists before reporting it to the caller.
            API_Element createdElement = {};
            createdElement.header.guid = element.header.guid;
            if (ACAPI_Element_Get (&createdElement) != NoError) {
                elements.Push (CreateErrorResponse (APIERR_GENERAL, "The dimension was not created (is a section or elevation window active? Activate a floor plan window and retry)."));
                continue;
            }

            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateAssociativeDimensionsOnSectionCommand::CreateAssociativeDimensionsOnSectionCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAssociativeDimensionsOnSectionCommand::GetName () const
{
    return "CreateAssociativeDimensionsOnSection";
}

GS::Optional<GS::UniString> CreateAssociativeDimensionsOnSectionCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "sectionElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of a single section element. Only one of sectionElementId and sectionElementIds can be given."
                        },
                        "sectionElementIds": {
                            "type": "array",
                            "items": { "$ref": "#/ElementId" },
                            "minItems": 1,
                            "description": "A list of section elements whose preset points are merged into one continuous dimension chain. Only one of sectionElementId and sectionElementIds can be given."
                        },
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "preset": {
                            "type": "string",
                            "enum": [
                                "WallCompositeFaces",
                                "WallSkinBorders",
                                "SlabCompositeFaces",
                                "SlabSkinBorders",
                                "BeamOrColumnRefLineEndPoints",
                                "BeamOrColumnBoundingBoxCorners",
                                "DoorWindowWallHoleCorners",
                                "DoorWindowModelHotspots"
                            ]
                        },
                        "direction": { "$ref": "#/Coordinate2D" },
                        "skinBorderIndices": {
                            "type": "array",
                            "items": { "type": "integer" },
                            "minItems": 1
                        },
                        "beginPlane": { "type": "boolean" },
                        "totalSizePlane": { "type": "boolean" },
                        "placeOnTop": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["referencePoint", "preset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    })";
}

GS::Optional<GS::UniString> CreateAssociativeDimensionsOnSectionCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateAssociativeDimensionsOnSectionCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> dimensionsData;
    auto error = GetElementArray (parameters, "dimensionsData", dimensionsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Associative Dimensions On Section", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : dimensionsData) {
            GS::Array<AssociativeDimensionPoint> witnessPoints;
            API_Vector defaultDirection = {1.0, 0.0};
            {
                auto error = BuildSectionAssociativeDimensionPoints (data, witnessPoints, defaultDirection);
                if (error.HasValue ()) {
                    elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }

            API_Coord directionCoord = {defaultDirection.x, defaultDirection.y};
            auto overrideDirection = GetOptionalCoordinate2D (data, "direction");
            if (overrideDirection.HasValue ()) {
                directionCoord = overrideDirection.Get ();
            }
            if (directionCoord.x == 0.0 && directionCoord.y == 0.0) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Dimension direction must be non-zero."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });

            #ifdef ServerMainVers_2600
            element.header.type = API_DimensionID;
            #else
            element.header.typeID = API_DimensionID;
            #endif
            GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare section associative dimension defaults."));
                continue;
            }

            if (data.Get ("referencePoint") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'referencePoint' field."));
                continue;
            }
            FillDimensionDefaults (
                element,
                Get2DCoordinateFromObjectState (*data.Get ("referencePoint")),
                {directionCoord.x, directionCoord.y}
            );

            auto error = PopulateAssociativeDimensionMemo (witnessPoints, element, memo);

            if (error.HasValue ()) {
                elements.Push (CreateErrorResponse (APIERR_MEMFULL, error.Get ()));
                continue;
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create associative section dimension."));
                continue;
            }

            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateWallThicknessDimensionsCommand::CreateWallThicknessDimensionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateWallThicknessDimensionsCommand::GetName () const
{
    return "CreateWallThicknessDimensions";
}

GS::Optional<GS::UniString> CreateWallThicknessDimensionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "wallId": { "$ref": "#/ElementId" },
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "direction": { "$ref": "#/Coordinate2D" }
                    },
                    "additionalProperties": false,
                    "required": ["wallId", "referencePoint", "direction"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    })";
}

GS::Optional<GS::UniString> CreateWallThicknessDimensionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateWallThicknessDimensionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> dimensionsData;
    auto error = GetElementArray (parameters, "dimensionsData", dimensionsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Wall Thickness Dimensions", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : dimensionsData) {
            if (data.Get ("wallId") == nullptr || data.Get ("direction") == nullptr || data.Get ("referencePoint") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'wallId', 'direction', or 'referencePoint' field."));
                continue;
            }
            API_Element wall = {};
            wall.header.guid = GetGuidFromObjectState (*data.Get ("wallId"));
            GSErrCode err = ACAPI_Element_Get (&wall);
            if (err != NoError || GetElemTypeId (wall.header) != API_WallID) {
                elements.Push (CreateErrorResponse (APIERR_BADID, "Failed to load wall for associative dimension."));
                continue;
            }

            const API_Coord directionCoord = Get2DCoordinateFromObjectState (*data.Get ("direction"));
            if (directionCoord.x == 0.0 && directionCoord.y == 0.0) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Dimension direction must be non-zero."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });

            #ifdef ServerMainVers_2600
            element.header.type = API_DimensionID;
            #else
            element.header.typeID = API_DimensionID;
            #endif
            err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare dimension defaults."));
                continue;
            }

            element.header.floorInd = wall.header.floorInd;
            element.dimension.dimAppear = APIApp_Normal;
            element.dimension.textPos = APIPos_Above;
            element.dimension.textWay = APIDir_Parallel;
            element.dimension.defStaticDim = false;
            element.dimension.usedIn3D = false;
            element.dimension.horizontalText = false;
            element.dimension.refC = Get2DCoordinateFromObjectState (*data.Get ("referencePoint"));
            element.dimension.direction = {directionCoord.x, directionCoord.y};
            element.dimension.nDimElem = 2;

            memo.dimElems = reinterpret_cast<API_DimElem**> (BMhAllClear (element.dimension.nDimElem * sizeof (API_DimElem)));
            if (memo.dimElems == nullptr || *memo.dimElems == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate dimension witness data."));
                continue;
            }

            const Int32 wallInIndices[2] = {11, 21};
            for (Int32 dimElemIndex = 0; dimElemIndex < element.dimension.nDimElem; ++dimElemIndex) {
                API_DimElem& dimElem = (*memo.dimElems)[dimElemIndex];
#ifdef ServerMainVers_2600
                dimElem.base.base.type = API_ElemType (API_WallID);
#else
                dimElem.base.base.typeID = API_WallID;
#endif
                dimElem.base.base.guid = wall.header.guid;
                dimElem.base.base.line = true;
                dimElem.base.base.inIndex = wallInIndices[dimElemIndex];
                dimElem.base.base.special = 0;
                dimElem.base.base.node_id = 0;
                dimElem.base.base.node_status = 0;
                dimElem.base.base.node_typ = 0;
                dimElem.note = element.dimension.defNote;
                dimElem.witnessVal = element.dimension.defWitnessVal;
                dimElem.witnessForm = element.dimension.defWitnessForm;
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create wall thickness dimension."));
                continue;
            }

            // Same ghost-guid failure mode as CreateAssociativeDimensions (#510): with a
            // section/elevation window active the create reports success although nothing
            // was created in any database. Verify before reporting the id.
            API_Element createdElement = {};
            createdElement.header.guid = element.header.guid;
            if (ACAPI_Element_Get (&createdElement) != NoError) {
                elements.Push (CreateErrorResponse (APIERR_GENERAL, "The dimension was not created (is a section or elevation window active? Activate a floor plan window and retry)."));
                continue;
            }

            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

ModifyWallsCommand::ModifyWallsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyWallsCommand::GetName () const
{
    return "ModifyWalls";
}

GS::Optional<GS::UniString> ModifyWallsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "wallsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "geometryType": { "type": "string", "enum": ["Straight", "Trapezoid"], "description": "The wall's plan outline shape (Polygonal is not settable here, read-only via GetDetailsOfElements). This is unrelated to slantAlpha/slantBeta - see profileType for the cross section shape those depend on." },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero makes the wall curved (begCoordinate/endCoordinate are the chord endpoints)." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Sets relativeTopStory to 0 (explicit height). Do not combine with relativeTopStory in the same call - whichever is applied last wins, and Archicad recomputes the actual height from the story elevations once relativeTopStory is non-zero." },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "bottomOffset": { "type": "number" },
                        "offset": { "type": "number" },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite", "Profile"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" },
                        "referenceLineLocation": {
                            "type": "string",
                            "enum": ["Outside", "Center", "Inside", "CoreOutside", "CoreCenter", "CoreInside"],
                            "description": "The Core* values only have an effect on a Composite or Profile wall (structureType) - a Basic wall has no core skin."
                        },
                        "profileType": { "type": "string", "enum": ["Normal", "Slanted", "Trapez"], "description": "Cross section shape of the wall, distinct from geometryType (which is the plan outline). slantAlpha/slantBeta only have an effect once this is set to Slanted." },
                        "slantAlpha": { "type": "number", "description": "Only has an effect once profileType is set to Slanted or Trapez." },
                        "slantBeta": { "type": "number", "description": "Only has an effect once profileType is set to Slanted or Trapez." },
                        "topOffset": { "type": "number", "description": "Only has an effect when relativeTopStory is non-zero." },
                        "relativeTopStory": { "type": "number", "description": "Non-zero links the wall's top to another story instead of an explicit height - do not set together with 'height' in the same call, see the note on 'height' above." },
                        "zoneRel": {
                            "type": "string",
                            "enum": ["Boundary", "ReduceArea", "None", "SubtractFromZone"]
                        },
                        "visibility": { "$ref": "#/StoryVisibility" },
                        "isAutoOnStoryVisibility": { "type": "boolean", "description": "When true (the default on a new wall), Archicad recomputes 'visibility' automatically from the wall's vertical extent and ignores any value set for it. Setting 'visibility' without also setting this field turns it off automatically." },
                        "referenceMaterial": { "$ref": "#/OverriddenMaterial" },
                        "oppositeMaterial": { "$ref": "#/OverriddenMaterial" },
                        "sideMaterial": { "$ref": "#/OverriddenMaterial" },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["wallsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyWallsCommand::GetRawResponseSchema () const
{
    return R"({"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]})";
}

GS::ObjectState ModifyWallsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "wallsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteModifyWithResults ("Modify Walls", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load wall."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            bool changed = ApplyWallDetails (element, mask, item);
            auto error = ApplyWallStructure (element, &mask, item, changed);
            if (error.HasValue ()) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                continue;
            }
            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No wall fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify wall."));
        }
    });
}

ModifyBeamsCommand::ModifyBeamsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyBeamsCommand::GetName () const
{
    return "ModifyBeams";
}

GS::Optional<GS::UniString> ModifyBeamsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "beamsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "level": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": { "type": "number" },
                        "arcAngle": { "type": "number" },
                        "verticalCurveHeight": { "type": "number" },
                        "beamShape": { "type": "string", "enum": ["Straight", "HorizontallyCurved", "VerticallyCurved"] },
                        "isSlanted": { "type": "boolean" },
                        "isFlipped": { "type": "boolean" },
                        "profileAngle": { "type": "number" },
                        "anchorPoint": { "type": "string", "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"] },
                        "width": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section width of the beam. Applied to all segments." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section height of the beam. Applied to all segments." },
                        "isWidthAndHeightLinked": { "type": "boolean", "description": "When true, Archicad keeps width and height equal and setting one changes the other - set to false first to give width/height independent values. Applied to all segments." },
                        "buildingMaterialId": { "$ref": "#/AttributeId", "description": "Cross section building material. Applied to all segments." },
                        "profileId": { "$ref": "#/AttributeId", "description": "Switches the cross section to this custom extruded profile. Applied to all segments." },
                        "holes": {
                            "type": "array",
                            "description": "Replaces all holes currently placed on the beam.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "type": { "type": "string", "enum": ["Rectangular", "Circular"] },
                                    "showContour": { "type": "boolean" },
                                    "centerX": { "type": "number" },
                                    "centerZ": { "type": "number" },
                                    "width": { "type": "number", "exclusiveMinimum": 0.0 },
                                    "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Only used for the Rectangular type." }
                                },
                                "additionalProperties": false,
                                "required": ["type", "centerX", "centerZ", "width"]
                            }
                        },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "coverFill": { "$ref": "#/CoverFill" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["beamsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyBeamsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyBeamsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "beamsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteModifyWithResults ("Modify Beams", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load beam."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            bool changed = ApplyBeamDetails (element, mask, item);
            const bool hasSectionFields = item.Contains ("width") || item.Contains ("height") || item.Contains ("isWidthAndHeightLinked") || item.Contains ("buildingMaterialId") || item.Contains ("profileId");

            GS::Array<GS::ObjectState> holes;
            if (item.Get ("holes", holes)) {
                API_ElementMemo memo = {};
                const GS::OnExit cleanup ([&]() { ACAPI_DisposeElemMemoHdls (&memo); });
                const GSSize nHoles = holes.GetSize ();
                memo.beamHoles = reinterpret_cast<API_Beam_Hole**> (BMAllocateHandle (GS::Max<GSSize> (nHoles, 1) * sizeof (API_Beam_Hole), ALLOCATE_CLEAR, 0));
                if (memo.beamHoles == nullptr) {
                    results.Push (CreateFailedExecutionResult (APIERR_MEMFULL, "Failed to allocate memory for beam holes."));
                    continue;
                }
                for (GSIndex i = 0; i < nHoles; ++i) {
                    const GS::ObjectState& holeOs = holes[i];
                    API_Beam_Hole& hole = (*memo.beamHoles)[i];
                    hole = {};
                    hole.holeID = static_cast<Int32> (i + 1);
                    GS::UniString typeStr;
                    holeOs.Get ("type", typeStr);
                    hole.holeType = (typeStr == "Circular") ? APIBHole_Circular : APIBHole_Rectangular;
                    holeOs.Get ("showContour", hole.holeContureOn);
                    holeOs.Get ("centerX", hole.centerx);
                    holeOs.Get ("centerZ", hole.centerz);
                    holeOs.Get ("width", hole.width);
                    holeOs.Get ("height", hole.height);
                }

                err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_BeamHole, true);
                if (err != NoError) {
                    results.Push (CreateFailedExecutionResult (err, "Failed to modify beam holes."));
                    continue;
                }
                if (hasSectionFields) {
                    bool sectionOk = ApplyBeamSectionToMemo (element.header.guid, item);
                    results.Push (sectionOk ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify beam cross section."));
                    continue;
                }
                results.Push (CreateSuccessfulExecutionResult ());
                continue;
            }

            if (!changed && !hasSectionFields) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No beam fields to modify."));
                continue;
            }

            if (changed) {
                err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
                if (err != NoError) {
                    results.Push (CreateFailedExecutionResult (err, "Failed to modify beam."));
                    continue;
                }
            }

            if (hasSectionFields) {
                bool sectionOk = ApplyBeamSectionToMemo (element.header.guid, item);
                results.Push (sectionOk ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify beam cross section."));
                continue;
            }

            results.Push (CreateSuccessfulExecutionResult ());
        }
    });
}

ModifySlabsCommand::ModifySlabsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifySlabsCommand::GetName () const
{
    return "ModifySlabs";
}

GS::Optional<GS::UniString> ModifySlabsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "slabsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "zCoordinate": { "type": "number" },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "referencePlaneLocation": {
                            "type": "string",
                            "enum": ["Top", "CoreTop", "CoreBottom", "Bottom"]
                        },
                        "polygonOutline": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3,
                            "description": "Replaces the slab's entire polygon, including its holes - resend the holes field too to keep them, otherwise they are removed."
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": {
                            "$ref": "#/Holes2D",
                            "description": "Can be given on its own, without polygonOutline, to add/remove/clear holes in place (an empty array clears all holes) - the slab's current outline is reused unchanged."
                        },
                        "topMaterial": { "$ref": "#/OverriddenMaterial" },
                        "sideMaterial": { "$ref": "#/OverriddenMaterial" },
                        "bottomMaterial": { "$ref": "#/OverriddenMaterial" },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "floorFill": { "$ref": "#/FloorFill" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["slabsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifySlabsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifySlabsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "slabsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    const Stories stories = GetStories ();

    return ExecuteModifyWithResults ("Modify Slabs", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load slab."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            bool changed = ApplySlabDetails (element, mask, item, stories);
            auto error = ApplySlabStructure (element, &mask, item, changed);
            if (error.HasValue ()) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                continue;
            }

            GS::Array<GS::ObjectState> polygonOutline;
            const bool hasNewOutline = item.Get ("polygonOutline", polygonOutline);
            GS::Array<GS::ObjectState> holes;
            const bool hasHolesField = item.Get ("holes", holes);
            if (hasNewOutline || hasHolesField) {
                // holes can be given on its own (no polygonOutline) to add/remove/clear holes
                // in place - including an explicit empty array to clear all holes - without
                // forcing the caller to resend the (possibly large) outline unchanged. Previously
                // this whole branch was gated on polygonOutline alone, so a holes-only request
                // fell through to "No slab fields to modify." below (issue #452).
                GS::Array<GS::ObjectState> polygonArcs;
                if (hasNewOutline) {
                    if (polygonOutline.GetSize () < 3) {
                        results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "'polygonOutline' must contain at least 3 coordinates."));
                        continue;
                    }
                    item.Get ("polygonArcs", polygonArcs);
                } else {
                    GS::ObjectState currentGeometry;
                    AddPolygonWithHolesFromMemoCoords (element.header.guid, currentGeometry, "polygonOutline", "polygonArcs", "holes", "polygonOutline", "polygonArcs");
                    currentGeometry.Get ("polygonOutline", polygonOutline);
                    currentGeometry.Get ("polygonArcs", polygonArcs);
                }

                API_ElementMemo memo = {};
                const GS::OnExit cleanup ([&]() {
                    ACAPI_DisposeElemMemoHdls (&memo);
                });
                ACAPI_Element_GetMemo (element.header.guid, &memo);

                auto error = ApplySlabPolygonChange (memo, element.slab.sideMat, polygonOutline, polygonArcs, holes);
                if (error.HasValue ()) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                    continue;
                }

                // Non-geometry field changes (thickness/level/etc, accumulated into mask above)
                // are applied first via ACAPI_Element_Change; the polygon itself goes through
                // ACAPI_Element_ChangeMemo, matching the DevKit's own reference example
                // (ApplySlabPolygonChange mutates the memo via Graphisoft's polygon-editing
                // primitives, not a from-scratch replacement, so element.slab.poly's counts are
                // deliberately not touched here).
                if (changed) {
                    err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
                    if (err != NoError) {
                        results.Push (CreateFailedExecutionResult (err, "Failed to modify slab."));
                        continue;
                    }
                }

                API_Guid slabGuid = element.header.guid;
                err = ACAPI_Element_ChangeMemo (slabGuid, APIMemoMask_Polygon | APIMemoMask_SideMaterials | APIMemoMask_EdgeTrims, &memo);
                results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify slab geometry."));
                continue;
            }

            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No slab fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify slab."));
        }
    });
}

ModifyRoofsCommand::ModifyRoofsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyRoofsCommand::GetName () const
{
    return "ModifyRoofs";
}

GS::Optional<GS::UniString> ModifyRoofsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "roofsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "level": { "type": "number" },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "type": "object",
                                "properties": {
                                    "levelHeight": { "type": "number" },
                                    "levelAngle": { "type": "number", "exclusiveMinimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["levelHeight", "levelAngle"]
                            }
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "polygonOutline": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": { "$ref": "#/Holes2D" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["roofsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyRoofsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyRoofsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "roofsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    const Stories stories = GetStories ();

    return ExecuteModifyWithResults ("Modify Roofs", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load roof."));
                continue;
            }
            if (element.roof.roofClass != API_PolyRoofID) {
                results.Push (CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "Only multi-plane roofs are supported.")); 
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            bool changed = false;
            {
                auto error = ApplyRoofStructure (element, &mask, item, changed);
                if (error.HasValue ()) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                    continue;
                }
            }
            auto error = ApplyRoofDetails (element, &mask, item, stories, changed);
            if (error.HasValue ()) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                continue;
            }
\
            GS::Array<GS::ObjectState> polygonOutline;
            if (item.Get ("polygonOutline", polygonOutline)) {
                if (polygonOutline.GetSize () < 3) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "'polygonOutline' must contain at least 3 coordinates."));
                    continue;
                }

                API_ElementMemo memo = {};
                const GS::OnExit cleanup ([&]() {
                    ACAPI_DisposeElemMemoHdls (&memo);
                });
                ACAPI_Element_GetMemo (element.header.guid, &memo);

                GS::Array<GS::ObjectState> polygonArcs;
                GS::Array<GS::ObjectState> holes;
                item.Get ("polygonArcs", polygonArcs);
                item.Get ("holes", holes);
                auto error = BuildRoofMemoFromGeometry (element, memo, polygonOutline, polygonArcs, holes);
                if (error.HasValue ()) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                    continue;
                }

                err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_AdditionalPolygon, true);
                results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify roof geometry."));
                continue;
            }

            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No roof fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify roof."));
        }
    });
}

// ============================================================================
// GetDimensionData
// ============================================================================

GetDimensionDataCommand::GetDimensionDataCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDimensionDataCommand::GetName () const
{
    return "GetDimensionData";
}

GS::Optional<GS::UniString> GetDimensionDataCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The identifier of the dimension elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::Optional<GS::UniString> GetDimensionDataCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "$ref": "#/DimensionDataOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    })";
}

static GS::UniString WitnessFormToString (API_WitnessID witnessForm)
{
    switch (witnessForm) {
        case APIWtn_None:   return "None";
        case APIWtn_Small:  return "Small";
        case APIWtn_Large:  return "Large";
        case APIWtn_Fix:    return "Fix";
        default:            return "Unknown";
    }
}

GS::ObjectState GetDimensionDataCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& dimensionsData = response.AddList<GS::ObjectState> ("dimensionsData");

    for (const GS::ObjectState& elementObj : elements) {
        const GS::ObjectState* elementId = elementObj.Get ("elementId");
        if (elementId == nullptr) {
            dimensionsData (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Element element = {};
        element.header.guid = GetGuidFromObjectState (*elementId);
        GSErrCode err = ACAPI_Element_Get (&element);
        if (err != NoError) {
            dimensionsData (CreateErrorResponse (err, "Failed to get element"));
            continue;
        }

        const API_ElemTypeID typeID = GetElemTypeId (element.header);
        if (typeID != API_DimensionID) {
            dimensionsData (CreateErrorResponse (APIERR_BADID, "Element is not a Dimension"));
            continue;
        }

        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
        err = ACAPI_Element_GetMemo (element.header.guid, &memo);
        if (err != NoError) {
            dimensionsData (CreateErrorResponse (err, "Failed to get element memo"));
            continue;
        }

        GS::ObjectState dimensionData;
        dimensionData.Add ("elementId", CreateGuidObjectState (element.header.guid));
        dimensionData.Add ("direction", Create2DCoordinateObjectState (element.dimension.direction));
        dimensionData.Add ("dimensionLinePosition", Create2DCoordinateObjectState (element.dimension.refC));

        const auto& witnessPoints = dimensionData.AddList<GS::ObjectState> ("witnessPoints");

        if (memo.dimElems != nullptr && *memo.dimElems != nullptr) {
            for (Int32 i = 0; i < element.dimension.nDimElem; ++i) {
                const API_DimElem& dimElem = (*memo.dimElems)[i];

                GS::ObjectState witnessPoint;

                witnessPoint.Add ("coordinate", Create2DCoordinateObjectState (dimElem.base.loc));
                witnessPoint.Add ("coordinate3D", Create3DCoordinateObjectState (dimElem.base.loc3D));
                witnessPoint.Add ("dimensionPosition", Create2DCoordinateObjectState (dimElem.pos));
                witnessPoint.Add ("dimensionValue", dimElem.dimVal);
                witnessPoint.Add ("witnessForm", WitnessFormToString (dimElem.witnessForm));
                witnessPoint.Add ("witnessVal", dimElem.witnessVal);

                const API_Guid& baseGuid = dimElem.base.base.guid;
                if (baseGuid != APINULLGuid) {
                    witnessPoint.Add ("baseElementId", CreateGuidObjectState (baseGuid));
                }

                witnessPoints (witnessPoint);
            }
        }

        dimensionsData (dimensionData);
    }

    return response;
}

ModifyColumnsCommand::ModifyColumnsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyColumnsCommand::GetName () const
{
    return "ModifyColumns";
}

GS::Optional<GS::UniString> ModifyColumnsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "columnsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "origin": { "$ref": "#/Coordinate2D" },
                        "zCoordinate": { "type": "number" },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Sets relativeTopStory to 0 (explicit height). Do not combine with relativeTopStory in the same call - see the note on relativeTopStory below." },
                        "bottomOffset": { "type": "number" },
                        "axisRotationAngle": { "type": "number" },
                        "coreAnchor": { "type": "string", "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"] },
                        "isSlanted": { "type": "boolean" },
                        "slantAngle": { "type": "number" },
                        "slantDirectionAngle": { "type": "number" },
                        "isFlipped": { "type": "boolean", "description": "Has no visible effect on a circular column (circleBased cross section) - Archicad ignores it there." },
                        "wrapping": { "type": "boolean" },
                        "topOffset": { "type": "number" },
                        "relativeTopStory": { "type": "number", "description": "Non-zero links the column's top to another story instead of an explicit height - do not set together with 'height' in the same call, see the note on 'height' above." },
                        "width": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section width of the column. Applied to all segments." },
                        "depth": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section depth (height) of the column. Applied to all segments." },
                        "isWidthAndHeightLinked": { "type": "boolean", "description": "When true, Archicad keeps width and depth equal and setting one changes the other - set to false first to give width/depth independent values. Applied to all segments." },
                        "circleBased": { "type": "boolean", "description": "True for a round column cross section, false for rectangular. Ignored once profileId switches the column to a custom profile shape. Applied to all segments." },
                        "buildingMaterialId": { "$ref": "#/AttributeId", "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments." },
                        "profileId": { "$ref": "#/AttributeId", "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments." },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "coverFill": { "$ref": "#/CoverFill" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["columnsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyColumnsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyColumnsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "columnsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    const Stories stories = GetStories ();

    return ExecuteModifyWithResults ("Modify Columns", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load column."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            const bool changed = ApplyColumnDetails (element, mask, item, stories);
            const bool hasSectionFields = item.Contains ("width") || item.Contains ("depth") || item.Contains ("isWidthAndHeightLinked") || item.Contains ("circleBased") || item.Contains ("buildingMaterialId") || item.Contains ("profileId");
            if (!changed && !hasSectionFields) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No column fields to modify."));
                continue;
            }

            if (changed) {
                err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
                if (err != NoError) {
                    results.Push (CreateFailedExecutionResult (err, "Failed to modify column."));
                    continue;
                }
            }

            if (hasSectionFields) {
                bool sectionOk = ApplyColumnSectionToMemo (element.header.guid, item);
                results.Push (sectionOk ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify column cross section."));
                continue;
            }

            results.Push (CreateSuccessfulExecutionResult ());
        }
    });
}

ModifyWindowsCommand::ModifyWindowsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyWindowsCommand::GetName () const
{
    return "ModifyWindows";
}

GS::Optional<GS::UniString> ModifyWindowsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "windowsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "reveal": { "type": "boolean", "description": "Turn the reveal on or off." },
                        "revealDepthOffset": { "type": "number", "description": "Distance the frame plane is moved across the wall thickness, along the wall normal." }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["windowsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyWindowsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyWindowsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "windowsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteModifyWithResults ("Modify Windows", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load window."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            const bool changed = ApplyWindowOrDoorDetails (element, mask, item);
            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No window fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify window."));
        }
    });
}

ModifyDoorsCommand::ModifyDoorsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyDoorsCommand::GetName () const
{
    return "ModifyDoors";
}

GS::Optional<GS::UniString> ModifyDoorsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "doorsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "reveal": { "type": "boolean", "description": "Turn the reveal on or off." },
                        "revealDepthOffset": { "type": "number", "description": "Distance the frame plane is moved across the wall thickness, along the wall normal." }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["doorsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyDoorsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyDoorsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "doorsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteModifyWithResults ("Modify Doors", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load door."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            const bool changed = ApplyWindowOrDoorDetails (element, mask, item);
            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No door fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify door."));
        }
    });
}

ModifyMorphsCommand::ModifyMorphsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyMorphsCommand::GetName () const
{
    return "ModifyMorphs";
}

GS::Optional<GS::UniString> ModifyMorphsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "morphsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "translation": { "$ref": "#/Coordinate3D" },
                        "rotationDegreesZ": { "type": "number" },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "body": {
                            "$ref": "#/MorphBody",
                            "description": "When given, discards the Morph's ENTIRE existing geometry and rebuilds it from this (mirrors CreateProfiles' replaceSkins) - not a partial edit."
                        },
                        "xAxis": { "$ref": "#/Coordinate3D", "description": "Replaces the rotation part of the placement transform outright. Give all three of xAxis/yAxis/zAxis together. If rotationDegreesZ is also given in the same call, it is applied first and this then overwrites its result." },
                        "yAxis": { "$ref": "#/Coordinate3D" },
                        "zAxis": { "$ref": "#/Coordinate3D" },
                        "surfaceId": { "$ref": "#/AttributeId" },
                        "castShadow": { "type": "boolean" },
                        "receiveShadow": { "type": "boolean" },
                        "isAutoOnStoryVisibility": { "type": "boolean" },
                        "showContour": { "$ref": "#/StoryVisibility" },
                        "showFill": { "$ref": "#/StoryVisibility" },
                        "linkToSettings": {
                            "type": "object",
                            "properties": {
                                "homeStoryDifference": { "type": "integer" },
                                "newCreationMode": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        },
                        "displayOption": {
                            "type": "string",
                            "enum": ["Standard", "StandardWithAbstract", "CutOnly", "OutLinesOnly", "AbstractAll", "CutAll"]
                        },
                        "viewDepthLimitation": {
                            "type": "string",
                            "enum": ["ToFloorPlanRange", "ToAbsoluteLimit", "EntireElement"]
                        },
                        "cutFillPen": { "type": "integer" },
                        "cutFillBackgroundPen": { "type": "integer" },
                        "cutLineType": { "$ref": "#/AttributeId" },
                        "cutLinePen": { "type": "integer" },
                        "uncutLineType": { "$ref": "#/AttributeId" },
                        "uncutLinePen": { "type": "integer" },
                        "overheadLineType": { "$ref": "#/AttributeId" },
                        "overheadLinePen": { "type": "integer" },
                        "useCoverFillType": { "type": "boolean" },
                        "outlineContourDisplay": { "type": "boolean" },
                        "coverFillType": { "$ref": "#/AttributeId" },
                        "coverFillPen": { "type": "integer" },
                        "coverFillBGPen": { "type": "integer" },
                        "use3DHatching": { "type": "boolean" },
                        "coverFillOrientation": {
                            "type": "object",
                            "properties": {
                                "type": { "type": "string", "enum": ["Global", "Rotated", "Distorted", "Centered"] },
                                "origo": { "$ref": "#/Coordinate2D" },
                                "matrix00": { "type": "number" },
                                "matrix10": { "type": "number" },
                                "matrix01": { "type": "number" },
                                "matrix11": { "type": "number" },
                                "innerRadius": { "type": "number" }
                            },
                            "additionalProperties": false
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "type": "string",
                            "enum": ["Invalid", "Planar", "Default", "Cylindric", "Spheric", "Box"]
                        },
                        "textureProjectionCoords": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate3D" },
                            "minItems": 4,
                            "maxItems": 4
                        },
                        "level": { "type": "number" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["morphsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyMorphsCommand::GetRawResponseSchema () const
{
    return ModifyWallsCommand ().GetRawResponseSchema ();
}

GS::ObjectState ModifyMorphsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> items;
    auto error = GetElementArray (parameters, "morphsWithDetails", items);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteModifyWithResults ("Modify Morphs", [&](GS::Array<GS::ObjectState>& results) {
        for (const auto& item : items) {
            if (item.Get ("elementId") == nullptr) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Missing required 'elementId' field."));
                continue;
            }
            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*item.Get ("elementId"));
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                results.Push (CreateFailedExecutionResult (err, "Failed to load morph."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            bool changed = false;

            auto translation = GetOptionalCoordinate3D (item, "translation");

            if (translation.HasValue ()) {
                element.morph.tranmat.tmx[3] += translation->x;
                element.morph.tranmat.tmx[7] += translation->y;
                element.morph.tranmat.tmx[11] += translation->z;
                ACAPI_ELEMENT_MASK_SET (mask, API_MorphType, tranmat);
                changed = true;
            }

            auto rotationDegrees = GetOptionalDouble (item, "rotationDegreesZ");

            if (rotationDegrees.HasValue ()) {
                const double radians = rotationDegrees.Get () * DegreesToRadians;
                const double cosAngle = std::cos (radians);
                const double sinAngle = std::sin (radians);
                const API_Tranmat originalTransform = element.morph.tranmat;
                for (Int32 column = 0; column < 4; ++column) {
                    element.morph.tranmat.tmx[column] = cosAngle * originalTransform.tmx[column] + sinAngle * originalTransform.tmx[8 + column];
                    element.morph.tranmat.tmx[8 + column] = -sinAngle * originalTransform.tmx[column] + cosAngle * originalTransform.tmx[8 + column];
                }
                ACAPI_ELEMENT_MASK_SET (mask, API_MorphType, tranmat);
                changed = true;
            }

            auto buildingMaterialId = GetOptionalObjectState (item, "buildingMaterialId");

            if (buildingMaterialId.HasValue ()) {
                API_AttributeIndex buildingMaterialIndex = APIInvalidAttributeIndex;
                if (!ResolveAttributeIndex (buildingMaterialId.Get (), API_BuildingMaterialID, buildingMaterialIndex)) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid morph building material."));
                    continue;
                }
                element.morph.buildingMaterial = buildingMaterialIndex;
                ACAPI_ELEMENT_MASK_SET (mask, API_MorphType, buildingMaterial);
                changed = true;
            }

            API_ElementMemo bodyMemo = {};
            const GS::OnExit bodyMemoGuard ([&bodyMemo] () { ACAPI_DisposeElemMemoHdls (&bodyMemo); });
            bool replacingBody = false;

            const GS::ObjectState* bodyOS = item.Get ("body");
            if (bodyOS != nullptr) {
                GS::UniString bodyError;
                if (!BuildMorphBodyFromGeometry (*bodyOS, bodyMemo, bodyError)) {
                    results.Push (CreateFailedExecutionResult (APIERR_GENERAL, bodyError));
                    continue;
                }
                GS::UniString bodyType;
                if (bodyOS->Get ("bodyType", bodyType) && bodyType == "Surface") {
                    element.morph.bodyType = APIMorphBodyType_SurfaceBody;
                } else {
                    element.morph.bodyType = APIMorphBodyType_SolidBody;
                }
                ACAPI_ELEMENT_MASK_SET (mask, API_MorphType, bodyType);
                GS::UniString edgeDefault;
                if (bodyOS->Get ("edgeDefault", edgeDefault)) {
                    if (edgeDefault == "HardHidden") {
                        element.morph.edgeType = APIMorphEdgeType_HardHiddenEdge;
                    } else if (edgeDefault == "SoftHidden") {
                        element.morph.edgeType = APIMorphEdgeType_SoftHiddenEdge;
                    } else {
                        element.morph.edgeType = APIMorphEdgeType_HardVisibleEdge;
                    }
                    ACAPI_ELEMENT_MASK_SET (mask, API_MorphType, edgeType);
                }
                replacingBody = true;
                changed = true;
            }

            {
                int appliedCount = 0;
                GS::UniString cosmeticError;
                if (!ApplyMorphCosmeticDetails (item, element, &mask, appliedCount, cosmeticError)) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, cosmeticError));
                    continue;
                }
                if (appliedCount > 0) {
                    changed = true;
                }
            }

            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No morph fields to modify."));
                continue;
            }

            err = replacingBody
                ? ACAPI_Element_Change (&element, &mask, &bodyMemo, APIMemoMask_All, true)
                : ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify morph."));
        }
    });
}

CreateSectionsCommand::CreateSectionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateSectionsCommand::GetName () const
{
    return "CreateSections";
}

GS::Optional<GS::UniString> CreateSectionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "sectionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "startCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "depth": { "type": "number" },
                        "name": { "type": "string" },
                        "floorIndex": { "type": "integer" }
                    },
                    "additionalProperties": false,
                    "required": ["startCoordinate", "endCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["sectionsData"]
    })";
}

GS::Optional<GS::UniString> CreateSectionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateSectionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> sectionsData;
    auto error = GetElementArray (parameters, "sectionsData", sectionsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Sections", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : sectionsData) {
            API_Element element = {};
            API_ElementMemo memo = {};
            API_SubElement marker = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
                ACAPI_DisposeElemMemoHdls (&marker.memo);
            });

#ifdef ServerMainVers_2600
            element.header.type = API_CutPlaneID;
#else
            element.header.typeID = API_CutPlaneID;
#endif
            marker.subType = static_cast<API_SubElementType> (APISubElement_MainMarker);

            GSErrCode err = ACAPI_Element_GetDefaultsExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare section defaults."));
                continue;
            }

            if (data.Get ("startCoordinate") == nullptr || data.Get ("endCoordinate") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'startCoordinate' or 'endCoordinate' field."));
                continue;
            }
            const API_Coord startCoord = Get2DCoordinateFromObjectState (*data.Get ("startCoordinate"));
            const API_Coord endCoord = Get2DCoordinateFromObjectState (*data.Get ("endCoordinate"));

            short floorIndex = 0;
            if (data.Get ("floorIndex", floorIndex)) {
                element.header.floorInd = floorIndex;
            }

            GS::UniString name;
            if (data.Get ("name", name)) {
                GS::ucscpy (element.cutPlane.segment.cutPlName, name.ToUStr ().Get ());
            }

            const double dx = endCoord.x - startCoord.x;
            const double dy = endCoord.y - startCoord.y;
            const double lineLength = sqrt (dx * dx + dy * dy);
            if (lineLength < 1e-6) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Start and end coordinates are too close."));
                continue;
            }

            double depth = 1.0;
            data.Get ("depth", depth);

            const double nx = -dy / lineLength;
            const double ny = dx / lineLength;

            element.cutPlane.segment.nMainCoord = 2;
            element.cutPlane.segment.nDepthCoord = 2;
            element.cutPlane.linkData.sourceMarker = true;
            marker.subType = APISubElement_MainMarker;

            if (memo.sectionSegmentMainCoords != nullptr) {
                BMpFree (reinterpret_cast<GSPtr> (memo.sectionSegmentMainCoords));
            }
            memo.sectionSegmentMainCoords = reinterpret_cast<API_Coord*> (BMpAll (2 * sizeof (API_Coord)));
            memo.sectionSegmentMainCoords[0] = startCoord;
            memo.sectionSegmentMainCoords[1] = endCoord;

            if (memo.sectionSegmentDepthCoords != nullptr) {
                BMpFree (reinterpret_cast<GSPtr> (memo.sectionSegmentDepthCoords));
            }
            memo.sectionSegmentDepthCoords = reinterpret_cast<API_Coord*> (BMpAll (2 * sizeof (API_Coord)));
            memo.sectionSegmentDepthCoords[0].x = startCoord.x + nx * depth;
            memo.sectionSegmentDepthCoords[0].y = startCoord.y + ny * depth;
            memo.sectionSegmentDepthCoords[1].x = endCoord.x + nx * depth;
            memo.sectionSegmentDepthCoords[1].y = endCoord.y + ny * depth;

            err = ACAPI_Element_CreateExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create section."));
                continue;
            }
            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

CreateInteriorElevationsCommand::CreateInteriorElevationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateInteriorElevationsCommand::GetName () const
{
    return "CreateInteriorElevations";
}

GS::Optional<GS::UniString> CreateInteriorElevationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "interiorElevationsData": {
                "type": "array",
                "description": "Array of data to create Interior Elevation elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "nodeCoordinates": {
                            "type": "array",
                            "description": "The corner points of the connected segment chain. Each consecutive pair of points becomes one segment, so a room with four walls needs five points to be closed, or four to be left open.",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 2
                        },
                        "depth": {
                            "type": "number",
                            "description": "How far each segment looks. Applied to every segment. Defaults to 1.0.",
                            "exclusiveMinimum": 0.0
                        },
                        "name": {
                            "type": "string",
                            "description": "Name of the interior elevation. Each segment is named after it."
                        },
                        "id": {
                            "type": "string",
                            "description": "ID string of the interior elevation."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "The story to place the interior elevation on. Defaults to the current story."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["nodeCoordinates"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["interiorElevationsData"]
    })";
}

GS::Optional<GS::UniString> CreateInteriorElevationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateInteriorElevationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> interiorElevationsData;
    auto error = GetElementArray (parameters, "interiorElevationsData", interiorElevationsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Interior Elevations", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : interiorElevationsData) {
            GS::Array<GS::ObjectState> nodeCoordinates;
            data.Get ("nodeCoordinates", nodeCoordinates);
            if (nodeCoordinates.GetSize () < 2) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "'nodeCoordinates' must contain at least 2 coordinates."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            API_SubElement marker = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
                ACAPI_DisposeElemMemoHdls (&marker.memo);
            });

#ifdef ServerMainVers_2600
            element.header.type = API_InteriorElevationID;
#else
            element.header.typeID = API_InteriorElevationID;
#endif
            // The marker's parameters are fetched separately below, so they are not asked
            // for here - this mirrors the DevKit's own Do_CreateInteriorElevation.
            marker.subType = static_cast<API_SubElementType> (APISubElement_MainMarker | APISubElement_NoParams);

            GSErrCode err = ACAPI_Element_GetDefaultsExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare interior elevation defaults."));
                continue;
            }

            // The marker's parameters come from the marker's OWN library part, not from a
            // separately resolved marker parent: taking them from anywhere else pairs a
            // parameter list with a library part it does not belong to, which is what
            // ACAPI_Element_CreateExt rejects with APIERR_REFUSEDPAR (see #413).
            double a = 0.0;
            double b = 0.0;
            Int32 addParNum = 0;
            API_AddParType** markAddPars = nullptr;
#ifdef ServerMainVers_2700
            err = ACAPI_LibraryPart_GetParams (marker.subElem.object.libInd, &a, &b, &addParNum, &markAddPars);
#else
            err = ACAPI_LibPart_GetParams (marker.subElem.object.libInd, &a, &b, &addParNum, &markAddPars);
#endif
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to read the interior elevation marker parameters."));
                continue;
            }
            marker.memo.params = markAddPars;
            marker.subElem.object.useObjPens = true;

            short floorIndex = 0;
            if (data.Get ("floorIndex", floorIndex)) {
                element.header.floorInd = floorIndex;
            }

            GS::UniString name;
            if (data.Get ("name", name)) {
                GS::ucscpy (element.interiorElevation.segment.cutPlName, name.ToUStr ().Get ());
            }
            GS::UniString idStr;
            if (data.Get ("id", idStr)) {
                GS::ucscpy (element.interiorElevation.segment.cutPlIdStr, idStr.ToUStr ().Get ());
            }

            double depth = 1.0;
            data.Get ("depth", depth);
            element.interiorElevation.segment.ieCreationSegmentDepth = depth;

            // The nodes form ONE polyline shared by every segment: nMainCoord counts the
            // nodes, sectionSegmentMainCoords holds them, and intElevSegments carries one
            // entry per gap between them. nSegments, nLineCoords and poly are filled in by
            // Archicad from these, and must not be set here.
            const USize nodeCount = nodeCoordinates.GetSize ();
            const USize segmentCount = nodeCount - 1;

            GS::Array<API_Coord> nodes;
            for (const GS::ObjectState& node : nodeCoordinates) {
                nodes.Push (Get2DCoordinateFromObjectState (node));
            }
            for (USize i = 0; i + 1 < nodeCount; ++i) {
                const double dx = nodes[i + 1].x - nodes[i].x;
                const double dy = nodes[i + 1].y - nodes[i].y;
                if (sqrt (dx * dx + dy * dy) < EPS) {
                    err = APIERR_BADPARS;
                    break;
                }
            }
            if (err != NoError) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Two consecutive coordinates of 'nodeCoordinates' are too close."));
                continue;
            }

            element.interiorElevation.segment.nMainCoord = static_cast<UInt32> (nodeCount);

            if (memo.sectionSegmentMainCoords != nullptr) {
                BMpFree (reinterpret_cast<GSPtr> (memo.sectionSegmentMainCoords));
            }
            memo.sectionSegmentMainCoords = reinterpret_cast<API_Coord*> (
                BMAllocatePtr (nodeCount * sizeof (API_Coord), ALLOCATE_CLEAR, 0));

            if (memo.intElevSegments != nullptr) {
                BMpFree (reinterpret_cast<GSPtr> (memo.intElevSegments));
            }
            memo.intElevSegments = reinterpret_cast<API_SectionSegment*> (
                BMAllocatePtr (segmentCount * sizeof (API_SectionSegment), ALLOCATE_CLEAR, 0));

            if (memo.sectionSegmentMainCoords == nullptr || memo.intElevSegments == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate the interior elevation segments."));
                continue;
            }

            for (USize i = 0; i < nodeCount; ++i) {
                memo.sectionSegmentMainCoords[i] = nodes[i];
            }

            marker.subType = APISubElement_MainMarker;
            err = ACAPI_Element_CreateExt (&element, &memo, 1UL, &marker);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create interior elevation."));
                continue;
            }
            elements.Push (CreateElementIdObjectState (element.header.guid));
        }
    });
}

GS::Optional<GS::UniString> BuildMeshPolyMemoFromGeometry (
    API_Element&                       elem,
    API_ElementMemo&                   memo,
    GS::Array<GS::ObjectState>&        polygonCoordinates,
    const GS::Array<GS::ObjectState>&  polygonArcs,
    const GS::Array<GS::ObjectState>&  holes)
{
    if (polygonCoordinates.GetSize () < 3) {
        return "'polygonCoordinates' must contain at least 3 coordinates.";
    }
    if (IsSame2DCoordinate (polygonCoordinates.GetFirst (), polygonCoordinates.GetLast ())) {
        polygonCoordinates.Pop ();
    }

    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    elem.mesh.poly.nCoords   = polygonCoordinates.GetSize () + 1;
    elem.mesh.poly.nSubPolys = 1;
    elem.mesh.poly.nArcs     = polygonArcs.GetSize ();

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holeCoords;
        GS::Array<GS::ObjectState> holeArcs;
        if (GetHoleGeometry (hole, holeCoords, holeArcs)) {
            elem.mesh.poly.nCoords += holeCoords.GetSize () + 1;
            ++elem.mesh.poly.nSubPolys;
            elem.mesh.poly.nArcs += holeArcs.GetSize ();
        }
    }

    const Int32 nCoords   = elem.mesh.poly.nCoords;
    const Int32 nSubPolys = elem.mesh.poly.nSubPolys;
    const Int32 nArcs     = elem.mesh.poly.nArcs;

    memo.coords = reinterpret_cast<API_Coord**> (
        memo.coords == nullptr
            ? BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.coords), (nCoords + 1) * sizeof (API_Coord), REALLOC_CLEAR, 0));

    memo.meshPolyZ = reinterpret_cast<double**> (
        memo.meshPolyZ == nullptr
            ? BMAllocateHandle ((nCoords + 1) * sizeof (double), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.meshPolyZ), (nCoords + 1) * sizeof (double), REALLOC_CLEAR, 0));

    memo.pends = reinterpret_cast<Int32**> (
        memo.pends == nullptr
            ? BMAllocateHandle ((nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.pends), (nSubPolys + 1) * sizeof (Int32), REALLOC_CLEAR, 0));

    if (nArcs > 0) {
        memo.parcs = reinterpret_cast<API_PolyArc**> (
            memo.parcs == nullptr
                ? BMAllocateHandle (nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0)
                : BMReallocHandle (reinterpret_cast<GSHandle> (memo.parcs), nArcs * sizeof (API_PolyArc), REALLOC_CLEAR, 0));
    } else if (memo.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
        memo.parcs = nullptr;
    }

    if (memo.vertexIDs != nullptr) {
        memo.vertexIDs = reinterpret_cast<UInt32**> (
            BMReallocHandle (reinterpret_cast<GSHandle> (memo.vertexIDs), (nCoords + 1) * sizeof (UInt32), REALLOC_CLEAR, 0));
    }

    Int32 iCoord = 1;
    Int32 iArc   = 0;
    Int32 iPends = 1;
    AddPolyToMemo (polygonCoordinates, polygonArcs, iCoord, iArc, iPends, memo);

    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holeCoords;
        GS::Array<GS::ObjectState> holeArcs;
        if (GetHoleGeometry (hole, holeCoords, holeArcs)) {
            AddPolyToMemo (holeCoords, holeArcs, iCoord, iArc, iPends, memo);
        }
    }

    return {};
}

void BuildMeshSublinesMemoFromGeometry (
    API_Element&                      elem,
    API_ElementMemo&                  memo,
    const GS::Array<GS::ObjectState>& sublines)
{
    Int32 nTotalCoords = 0;
    Int32 nSubLines    = 0;
    for (const GS::ObjectState& subline : sublines) {
        GS::Array<GS::ObjectState> coords;
        if (subline.Get ("coordinates", coords) && !coords.IsEmpty ()) {
            nTotalCoords += (Int32) coords.GetSize ();
            ++nSubLines;
        }
    }

    elem.mesh.levelLines.nCoords   = nTotalCoords;
    elem.mesh.levelLines.nSubLines = nSubLines;

    memo.meshLevelCoords = reinterpret_cast<API_MeshLevelCoord**> (
        memo.meshLevelCoords == nullptr
            ? BMAllocateHandle (nTotalCoords * sizeof (API_MeshLevelCoord), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.meshLevelCoords), nTotalCoords * sizeof (API_MeshLevelCoord), REALLOC_CLEAR, 0));
    memo.meshLevelEnds = reinterpret_cast<Int32**> (
        memo.meshLevelEnds == nullptr
            ? BMAllocateHandle (nSubLines * sizeof (Int32), ALLOCATE_CLEAR, 0)
            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.meshLevelEnds), nSubLines * sizeof (Int32), REALLOC_CLEAR, 0));

    Int32 iCoord = 0;
    Int32 iLine  = 0;
    for (const GS::ObjectState& subline : sublines) {
        GS::Array<GS::ObjectState> coords;
        if (!subline.Get ("coordinates", coords) || coords.IsEmpty ())
            continue;
        for (const GS::ObjectState& c : coords) {
            (*memo.meshLevelCoords)[iCoord].c = Get3DCoordinateFromObjectState (c);
            ++iCoord;
        }
        (*memo.meshLevelEnds)[iLine++] = iCoord;
    }
}

ModifyMeshesCommand::ModifyMeshesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String ModifyMeshesCommand::GetName () const
{
    return "ModifyMeshes";
}

GS::Optional<GS::UniString> ModifyMeshesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "meshesData": {
            "type": "array",
            "description": "Array of meshes to modify.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "meshData": {
                        "type": "object",
                        "description": "The fields to modify on the Mesh. Only provided fields are changed; omitted fields are left as-is.",
                        "properties": {
                            "floorIndex": {
                                "type": "integer"
                            },
                            "level": {
                                "type": "number",
                                "description": "The Z reference level of coordinates."
                            },
                            "skirtType": {
                                "$ref": "#/MeshSkirtType"
                            },
                            "skirtLevel": {
                                "type": "number",
                                "description": "The height of the skirt."
                            },
                            "ridges": {
                                "type": "string",
                                "description": "How ridges between mesh facets are displayed in 3D.",
                                "enum": ["AllSharp", "AllSmooth", "UserDefined"]
                            },
                            "showLines": {
                                "type": "boolean",
                                "description": "Whether to show secondary mesh lines on plan."
                            },
                            "contourPen": {
                                "type": "integer",
                                "description": "Pen attribute index for the mesh contour line."
                            },
                            "levelPen": {
                                "type": "integer",
                                "description": "Pen attribute index for the mesh level lines."
                            },
                            "lineTypeIndex": {
                                "type": "integer",
                                "description": "Line type attribute index for the mesh contour."
                            },
                            "polygonCoordinates": {
                                "type": "array",
                                "description": "The 3D coordinates of the outline polygon of the mesh. Replaces the existing boundary entirely.",
                                "items": { "$ref": "#/Coordinate3D" },
                                "minItems": 3
                            },
                            "polygonArcs": {
                                "type": "array",
                                "description": "Polygon outline arcs of the mesh.",
                                "items": { "$ref": "#/PolyArc" }
                            },
                            "holes": {
                                "$ref": "#/Holes3D"
                            },
                            "sublines": {
                                "type": "array",
                                "description": "The leveling sublines inside the polygon of the mesh. Replaces existing sublines entirely.",
                                "items": {
                                    "type": "object",
                                    "properties": {
                                        "coordinates": {
                                            "type": "array",
                                            "description": "The 3D coordinates of the leveling subline.",
                                            "items": { "$ref": "#/Coordinate3D" }
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": ["coordinates"]
                                }
                            }
                        },
                        "additionalProperties": false
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "meshData"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "meshesData"
    ]
})";
}

GS::Optional<GS::UniString> ModifyMeshesCommand::GetRawResponseSchema () const
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

GS::ObjectState ModifyMeshesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> meshesData;
    parameters.Get ("meshesData", meshesData);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("ModifyMeshes", [&] () -> GSErrCode {
        for (const GS::ObjectState& meshEntry : meshesData) {
            const GS::ObjectState* elementId = meshEntry.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element elem = {};
            elem.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&elem);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }

#ifdef ServerMainVers_2600
            if (elem.header.type.typeID != API_MeshID) {
#else
            if (elem.header.typeID != API_MeshID) {
#endif
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Mesh."));
                continue;
            }

            const GS::ObjectState* meshData = meshEntry.Get ("meshData");
            if (meshData == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "meshData is missing"));
                continue;
            }

            GS::Array<GS::ObjectState> polygonCoordinates;
            GS::Array<GS::ObjectState> polygonArcs;
            GS::Array<GS::ObjectState> holes;
            GS::Array<GS::ObjectState> sublines;
            const bool hasPolyGeom        = meshData->Get ("polygonCoordinates", polygonCoordinates);
            const bool hasSublines        = meshData->Get ("sublines", sublines);
            const bool isClearingSublines = hasSublines && sublines.IsEmpty ();
            if (hasPolyGeom) {
                meshData->Get ("polygonArcs", polygonArcs);
                meshData->Get ("holes", holes);
            }

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

            {
                // When clearing sublines without a polygon change, also load the current polygon
                // to compute the bounding box for the out-of-bounds dummy sublines (see below).
                const UInt64 loadMask =
                    ((hasPolyGeom || isClearingSublines) ? (APIMemoMask_Polygon | APIMemoMask_MeshPolyZ) : 0) |
                    ((hasSublines && !isClearingSublines) ? APIMemoMask_MeshLevel : 0);
                if (loadMask != 0) {
                    err = ACAPI_Element_GetMemo (elem.header.guid, &memo, loadMask);
                    if (err != NoError) {
                        executionResults (CreateFailedExecutionResult (err, "Failed to get mesh memo"));
                        continue;
                    }
                }
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            if (meshData->Get ("floorIndex", elem.header.floorInd)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
            }
            if (meshData->Get ("level", elem.mesh.level)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, level);
            }
            if (meshData->Get ("skirtLevel", elem.mesh.skirtLevel)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, skirtLevel);
            }
            GS::UniString skirtType;
            if (meshData->Get ("skirtType", skirtType)) {
                if (skirtType == "SurfaceOnlyWithoutSkirt") {
                    elem.mesh.skirt = 3;
                } else if (skirtType == "WithSkirt") {
                    elem.mesh.skirt = 2;
                } else if (skirtType == "SolidBodyWithSkirt") {
                    elem.mesh.skirt = 1;
                }
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, skirt);
            }
            GS::UniString ridges;
            if (meshData->Get ("ridges", ridges)) {
                if (ridges == "AllSharp") {
                    elem.mesh.smoothRidges = APIRidge_AllSharp;
                } else if (ridges == "AllSmooth") {
                    elem.mesh.smoothRidges = APIRidge_AllSmooth;
                } else if (ridges == "UserDefined") {
                    elem.mesh.smoothRidges = APIRidge_UserSharp;
                }
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, smoothRidges);
            }
            bool showLines = false;
            if (meshData->Get ("showLines", showLines)) {
                elem.mesh.showLines = showLines ? 1 : 0;
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, showLines);
            }
            short contourPen = 0;
            if (meshData->Get ("contourPen", contourPen) && contourPen > 0) {
                elem.mesh.contPen = contourPen;
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, contPen);
            }
            short levelPen = 0;
            if (meshData->Get ("levelPen", levelPen) && levelPen > 0) {
                elem.mesh.levelPen = levelPen;
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, levelPen);
            }
            Int32 lineTypeIndex = 0;
            if (meshData->Get ("lineTypeIndex", lineTypeIndex) && lineTypeIndex > 0) {
                elem.mesh.ltypeInd = ACAPI_CreateAttributeIndex (lineTypeIndex);
                ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, ltypeInd);
            }

            UInt64 memoChangeMask = 0;

            if (hasPolyGeom) {
                auto geoErr = BuildMeshPolyMemoFromGeometry (elem, memo, polygonCoordinates, polygonArcs, holes);
                if (geoErr.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, geoErr.Get ()));
                    continue;
                }
                memoChangeMask |= APIMemoMask_Polygon | APIMemoMask_MeshPolyZ;
            }

            if (hasSublines) {
                if (isClearingSublines) {
                    // Clearing level lines: ACAPI_Element_Change ignores null/empty handles for
                    // APIMemoMask_MeshLevel. Workaround: send two valid sublines placed just
                    // outside the polygon bounding box; ArchiCAD clips them out automatically,
                    // resulting in nSubLines == 0 stored in the element.
                    // memo.coords is guaranteed valid here (loaded above for this case).
                    const Int32 nPolyCoords = elem.mesh.poly.nCoords;
                    double xMin = (*memo.coords)[1].x;
                    double yMin = (*memo.coords)[1].y;
                    for (Int32 j = 2; j < nPolyCoords; ++j) {
                        if ((*memo.coords)[j].x < xMin) xMin = (*memo.coords)[j].x;
                        if ((*memo.coords)[j].y < yMin) yMin = (*memo.coords)[j].y;
                    }

                    const double kOffset = 1.0;
                    const double kStep   = 0.5;
                    const double ox = xMin - kOffset;
                    const double oy = yMin - kOffset;

                    memo.meshLevelCoords = reinterpret_cast<API_MeshLevelCoord**> (
                        memo.meshLevelCoords == nullptr
                            ? BMAllocateHandle (4 * sizeof (API_MeshLevelCoord), ALLOCATE_CLEAR, 0)
                            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.meshLevelCoords), 4 * sizeof (API_MeshLevelCoord), REALLOC_CLEAR, 0));
                    memo.meshLevelEnds = reinterpret_cast<Int32**> (
                        memo.meshLevelEnds == nullptr
                            ? BMAllocateHandle (2 * sizeof (Int32), ALLOCATE_CLEAR, 0)
                            : BMReallocHandle (reinterpret_cast<GSHandle> (memo.meshLevelEnds), 2 * sizeof (Int32), REALLOC_CLEAR, 0));

                    (*memo.meshLevelCoords)[0].c.x = ox;          (*memo.meshLevelCoords)[0].c.y = oy;          (*memo.meshLevelCoords)[0].c.z = 0.0;
                    (*memo.meshLevelCoords)[1].c.x = ox + kStep;  (*memo.meshLevelCoords)[1].c.y = oy;          (*memo.meshLevelCoords)[1].c.z = 0.0;
                    (*memo.meshLevelCoords)[2].c.x = ox;          (*memo.meshLevelCoords)[2].c.y = oy + kStep;  (*memo.meshLevelCoords)[2].c.z = 0.0;
                    (*memo.meshLevelCoords)[3].c.x = ox + kStep;  (*memo.meshLevelCoords)[3].c.y = oy + kStep;  (*memo.meshLevelCoords)[3].c.z = 0.0;
                    (*memo.meshLevelEnds)[0] = 2;
                    (*memo.meshLevelEnds)[1] = 4;

                    elem.mesh.levelLines.nCoords   = 4;
                    elem.mesh.levelLines.nSubLines = 2;
                    ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, levelLines);
                    memoChangeMask |= APIMemoMask_MeshLevel;
                } else {
                    BuildMeshSublinesMemoFromGeometry (elem, memo, sublines);
                    ACAPI_ELEMENT_MASK_SET (mask, API_MeshType, levelLines);
                    memoChangeMask |= APIMemoMask_MeshLevel;
                }
            }

            err = ACAPI_Element_Change (&elem, &mask, memoChangeMask != 0 ? &memo : nullptr, memoChangeMask, true);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to modify mesh"));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }

        return NoError;
    });

    return response;
}

// The element-type agnostic counterpart of ApplyWindowOrDoorFavoriteToDefaults, used by
// the per-item "favoriteName" support of CreateElementsCommandBase. Windows and Doors keep
// their own variant above: they are markered types that need ChangeDefaultsExt with the
// favorite's marker sub-element, and their create path clones the tool defaults itself.
GSErrCode ApplyFavoriteToElementDefaults (const GS::UniString& favoriteName, API_ElemTypeID expectedTypeId)
{
    if (favoriteName.IsEmpty ()) {
        return NoError;
    }

    API_Favorite favorite;
    favorite.name = favoriteName;
    favorite.memo.New ();
    favorite.properties.New ();
    favorite.classifications.New ();
    favorite.elemCategoryValues.New ();

    GSErrCode err = ACAPI_Favorite_Get (&favorite);
    const auto disposeFavoriteMemos = [&] () {
        ACAPI_DisposeElemMemoHdls (&favorite.memo.Get ());
    };
    if (err != NoError) {
        disposeFavoriteMemos ();
        return err;
    }

#ifdef ServerMainVers_2600
    const API_ElemTypeID favoriteTypeId = favorite.element.header.type.typeID;
#else
    const API_ElemTypeID favoriteTypeId = favorite.element.header.typeID;
#endif
    if (favoriteTypeId != expectedTypeId) {
        disposeFavoriteMemos ();
        return APIERR_REFUSEDPAR;
    }

    API_Element mask;
    ACAPI_ELEMENT_MASK_SETFULL (mask);
    err = ACAPI_Element_ChangeDefaults (&favorite.element, favorite.memo.GetPtr (), &mask);
    disposeFavoriteMemos ();
    if (err != NoError) {
        return err;
    }

    for (const GS::Pair<API_Guid, API_Guid>& pair : *favorite.classifications) {
        TAPIR_Element_AddClassificationItemDefault (favorite.element.header, pair.second);
    }
    for (const API_ElemCategoryValue& categoryValue : *favorite.elemCategoryValues) {
        TAPIR_Element_SetCategoryValueDefault (favorite.element.header, categoryValue);
    }
    TAPIR_Element_SetPropertiesOfDefaultElem (favorite.element.header, *favorite.properties);

    return NoError;
}
