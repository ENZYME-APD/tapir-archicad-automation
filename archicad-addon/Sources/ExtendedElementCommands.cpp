#include "ExtendedElementCommands.hpp"

#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"

#ifdef ServerMainVers_2900
#include	"ACAPI/Element/Opening/OpeningDefault.hpp"
#include	"ACAPI/Element/Opening/Opening.hpp"
#include "Polygon2D.hpp"
#endif

#include <cmath>
#include <limits>

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
    marker.subElem.object.pen = 166;
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

GS::Optional<GS::UniString> BuildSectionAssociativeDimensionPoints (
    const GS::ObjectState& data,
    GS::Array<AssociativeDimensionPoint>& points,
    API_Vector& defaultDirection)
{
    const GS::ObjectState* sectionElementId = data.Get ("sectionElementId");
    if (sectionElementId == nullptr) {
        return "Missing required field 'sectionElementId'.";
    }

    API_Element sectionElement = {};
    API_Element parentElement = {};
    {
        auto error = LoadSectionElementAndParent (GetGuidFromObjectState (*sectionElementId), sectionElement, parentElement);
        if (error.HasValue ()) {
            return error;
        }
    }

    GS::UniString presetName;
    if (!data.Get ("preset", presetName)) {
        return "Missing required field 'preset'.";
    }

    SectionAssociativeDimensionPreset preset;
    auto error = ParseSectionAssociativeDimensionPreset (presetName, preset);
    if (error.HasValue ()) {
        return error;
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
    // AddPolyToMemo with a null dereference. BMReallocHandle does NOT allocate from a
    // null handle, so a null handle must be created fresh with BMAllocateHandle (as the
    // mesh/zone commands do); only an existing handle is resized with BMReallocHandle.
    const Int32 nCoords    = element.slab.poly.nCoords;
    const Int32 nSubPolys  = element.slab.poly.nSubPolys;
    const Int32 nArcs      = element.slab.poly.nArcs;
    const bool  coordsWereNull = (memo.coords == nullptr);

    if (coordsWereNull) {
        memo.coords        = reinterpret_cast<API_Coord**>             (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord),               ALLOCATE_CLEAR, 0));
        memo.vertexIDs     = reinterpret_cast<UInt32**>                (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord),               ALLOCATE_CLEAR, 0));
        memo.edgeTrims     = reinterpret_cast<API_EdgeTrim**>          (BMAllocateHandle ((nCoords + 1) * sizeof (API_EdgeTrim),            ALLOCATE_CLEAR, 0));
        memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*>(BMAllocatePtr    ((nCoords + 1) * sizeof (API_OverriddenAttribute), ALLOCATE_CLEAR, 0));
    } else if (oldPoly.nCoords != nCoords) {
        memo.coords        = reinterpret_cast<API_Coord**>             (BMReallocHandle (reinterpret_cast<GSHandle> (memo.coords),        (nCoords + 1) * sizeof (API_Coord),               REALLOC_CLEAR, 0));
        memo.vertexIDs     = reinterpret_cast<UInt32**>                (BMReallocHandle (reinterpret_cast<GSHandle> (memo.vertexIDs),     (nCoords + 1) * sizeof (API_Coord),               REALLOC_CLEAR, 0));
        memo.edgeTrims     = reinterpret_cast<API_EdgeTrim**>          (BMReallocHandle (reinterpret_cast<GSHandle> (memo.edgeTrims),     (nCoords + 1) * sizeof (API_EdgeTrim),            REALLOC_CLEAR, 0));
        memo.sideMaterials = reinterpret_cast<API_OverriddenAttribute*>(BMReallocPtr    (reinterpret_cast<GSPtr> (memo.sideMaterials), (nCoords + 1) * sizeof (API_OverriddenAttribute), REALLOC_CLEAR, 0));
    }
    if (memo.pends == nullptr) {
        memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
    } else if (oldPoly.nSubPolys != nSubPolys) {
        memo.pends = reinterpret_cast<Int32**> (BMReallocHandle (reinterpret_cast<GSHandle> (memo.pends), (nSubPolys + 1) * sizeof (Int32), REALLOC_CLEAR, 0));
    }
    if (nArcs > 0) {
        if (memo.parcs == nullptr) {
            memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
        } else if (oldPoly.nArcs != nArcs) {
            memo.parcs = reinterpret_cast<API_PolyArc**> (BMReallocHandle (reinterpret_cast<GSHandle> (memo.parcs), nArcs * sizeof (API_PolyArc), REALLOC_CLEAR, 0));
        }
    }
    const bool needToProcessVertexIDs =
        coordsWereNull ||
        oldPoly.nCoords != element.slab.poly.nCoords ||
        oldPoly.nSubPolys != element.slab.poly.nSubPolys;

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

    // A multi-plane roof (API_PolyRoofData) needs BOTH a pivot polygon (set above via the
    // additionalPoly* memo) AND a contour polygon (memo.coords/pends/parcs). The contour was
    // never populated, leaving the roof under-specified so ACAPI_Element_Create throws a
    // GSException. Use the same outline for the contour as for the pivot polygon by
    // duplicating the handles.
    element.roof.u.polyRoof.contourPolygon = element.roof.u.polyRoof.pivotPolygon;
    if (memo.additionalPolyCoords != nullptr) {
        BMHandleToHandle (reinterpret_cast<GSConstHandle> (memo.additionalPolyCoords), reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.additionalPolyPends != nullptr) {
        BMHandleToHandle (reinterpret_cast<GSConstHandle> (memo.additionalPolyPends), reinterpret_cast<GSHandle*> (&memo.pends));
    }
    if (memo.additionalPolyParcs != nullptr) {
        BMHandleToHandle (reinterpret_cast<GSConstHandle> (memo.additionalPolyParcs), reinterpret_cast<GSHandle*> (&memo.parcs));
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
            element.wall.type = APIWtyp_Poly;
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
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (level.Get (), stories);
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
    return changed;
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
    return changed;
}

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
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "zCoordinate": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": { "type": "number" },
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
    const auto floorIndexAndOffset = GetFloorIndexAndOffset (zCoordinate, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.beam.level = floorIndexAndOffset.second;

    auto offset = GetOptionalDouble (parameters, "offset");

    if (offset.HasValue ()) {
        element.beam.offset = offset.Get ();
    }
    auto slantAngle = GetOptionalDouble (parameters, "slantAngle");
    if (slantAngle.HasValue ()) {
        element.beam.slantAngle = slantAngle.Get ();
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

    if ((width.HasValue () || height.HasValue ()) && memo.beamSegments != nullptr) {
        GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr>(memo.beamSegments)) / sizeof (API_BeamSegmentType);
        for (GSSize i = 0; i < nSegments; ++i) {
            if (width.HasValue ()) {
                memo.beamSegments[i].assemblySegmentData.nominalWidth = width.Get ();
            }
            if (height.HasValue ()) {
                memo.beamSegments[i].assemblySegmentData.nominalHeight = height.Get ();
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
    const auto floorIndexAndOffset = GetFloorIndexAndOffset (zCoordinate, stories);
    element.header.floorInd = floorIndexAndOffset.first;

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

GS::Optional<GS::UniString> CreateWindowsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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
                elements.Push (CreateErrorResponse (err, "Failed to create window."));
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

GS::Optional<GS::UniString> CreateDoorsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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
                elements.Push (CreateErrorResponse (err, "Failed to create door."));
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

GS::Optional<GS::UniString> CreateOpeningsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState CreateOpeningsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl&) const
{
    GS::Array<GS::ObjectState> openingsData;
    auto error = GetElementArray (parameters, "openingsData", openingsData);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }

    return ExecuteCreateWithElements ("Create Openings", [&](GS::Array<GS::ObjectState>& elements) {
        for (const auto& data : openingsData) {
            if (data.Get ("basePoint") == nullptr || data.Get ("ownerElementId") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'basePoint' or 'ownerElementId' field."));
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

            data.Get ("width", element.opening.extrusionGeometryData.parameters.width);
            data.Get ("height", element.opening.extrusionGeometryData.parameters.height);

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

            ACAPI::UniqueID parentElemId (APIGuid2GSGuid (GetGuidFromObjectState (*data.Get ("ownerElementId"))), ACAPI_GetToken ());
            ACAPI::Result<ACAPI::UniqueID> resultId = openingDefault->PlacePolygonal (parentElemId, basePoint, polygon);
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
                        "size": { "$ref": "#/Dimensions3D" },
                        "buildingMaterialId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["basePoint", "size"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["morphsData"]
    })";
}

GS::Optional<GS::UniString> CreateMorphsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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

            if (data.Get ("basePoint") == nullptr || data.Get ("size") == nullptr) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Missing required 'basePoint' or 'size' field."));
                continue;
            }
            const API_Coord3D basePoint = Get3DCoordinateFromObjectState (*data.Get ("basePoint"));
            const API_Coord3D size = Get3DCoordinateFromObjectState (*data.Get ("size"));
            if (size.x <= 0.0 || size.y <= 0.0 || size.z <= 0.0) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, "Morph 'size' values must be positive."));
                continue;
            }

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

            API_ElementMemo memo = {};
            const GS::OnExit cleanup ([&]() {
                ACAPI_DisposeElemMemoHdls (&memo);
            });

            if (!BuildCuboidMorphMemo (size.x, size.y, size.z, element.morph.buildingMaterial, memo)) {
                elements.Push (CreateErrorResponse (APIERR_GENERAL, "Failed to build morph body."));
                continue;
            }

            err = ACAPI_Element_Create (&element, &memo);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to create morph."));
                continue;
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

GS::Optional<GS::UniString> CreateRoofsCommand::GetResponseSchema () const
{
    return CreateMorphsCommand ().GetResponseSchema ();
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
            element.roof.roofClass = API_PolyRoofID;
            GSErrCode err = ACAPI_Element_GetDefaults (&element, nullptr);
            if (err != NoError) {
                elements.Push (CreateErrorResponse (err, "Failed to prepare roof defaults."));
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
            auto error = BuildRoofMemoFromGeometry (element, memo, polygonOutline, polygonArcs, holes);
            if (error.HasValue ()) {
                elements.Push (CreateErrorResponse (APIERR_BADPARS, error.Get ()));
                continue;
            }

            // Multi-plane roof creation is NOT yet supported. ACAPI_Element_Create for an
            // API_PolyRoofID requires pivot-edge plane data (memo.pivotPolyEdges holding
            // per-edge API_RoofSegmentData keyed by the pivot polygon's edge unique IDs),
            // which is not built here. Calling Create without it makes the Archicad core
            // THROW a GSException AND pop a modal dialog that hangs the JSON API. Until that
            // plane setup is implemented, reject cleanly without ever calling Create so the
            // command can never crash or hang the application.
            (void) err;
            elements.Push (CreateErrorResponse (APIERR_GENERAL, "Multi-plane roof creation is not yet supported (incomplete pivot-edge plane setup)."));
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

GS::Optional<GS::UniString> CreateAssociativeDimensionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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
                        "sectionElementId": { "$ref": "#/ElementId" },
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
                    "required": ["sectionElementId", "referencePoint", "preset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    })";
}

GS::Optional<GS::UniString> CreateAssociativeDimensionsOnSectionCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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

GS::Optional<GS::UniString> CreateWallThicknessDimensionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero makes the wall curved (begCoordinate/endCoordinate are the chord endpoints)." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "bottomOffset": { "type": "number" },
                        "offset": { "type": "number" },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite", "Profile"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" }
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

GS::Optional<GS::UniString> ModifyWallsCommand::GetResponseSchema () const
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
                        "verticalCurveHeight": { "type": "number" }
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

GS::Optional<GS::UniString> ModifyBeamsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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
            const bool changed = ApplyBeamDetails (element, mask, item);
            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No beam fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify beam."));
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
        "required": ["slabsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifySlabsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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
                auto error = BuildSlabMemoFromGeometry (element, memo, polygonOutline, polygonArcs, holes);
                if (error.HasValue ()) {
                    results.Push (CreateFailedExecutionResult (APIERR_BADPARS, error.Get ()));
                    continue;
                }

                err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_Polygon | APIMemoMask_SideMaterials | APIMemoMask_EdgeTrims, true);
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

GS::Optional<GS::UniString> ModifyRoofsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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

GS::Optional<GS::UniString> GetDimensionDataCommand::GetResponseSchema () const
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
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "bottomOffset": { "type": "number" },
                        "axisRotationAngle": { "type": "number" }
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

GS::Optional<GS::UniString> ModifyColumnsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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
            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No column fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            results.Push (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify column."));
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
                        "oSide": { "type": "boolean" }
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

GS::Optional<GS::UniString> ModifyWindowsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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
                        "oSide": { "type": "boolean" }
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

GS::Optional<GS::UniString> ModifyDoorsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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
                        "buildingMaterialId": { "$ref": "#/AttributeId" }
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

GS::Optional<GS::UniString> ModifyMorphsCommand::GetResponseSchema () const
{
    return ModifyWallsCommand ().GetResponseSchema ();
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

            if (!changed) {
                results.Push (CreateFailedExecutionResult (APIERR_BADPARS, "No morph fields to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
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

GS::Optional<GS::UniString> CreateSectionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
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

GS::Optional<GS::UniString> ModifyMeshesCommand::GetResponseSchema () const
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
