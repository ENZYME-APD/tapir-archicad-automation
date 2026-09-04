#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "OnExit.hpp"

#include "ObjectState.hpp"
#include "BiHashTable.hpp"

#include <vector>
#include <map>

enum class CommonSchema
{
    Used,
    NotUsed
};

class CommandBase : public API_AddOnCommand
{
public:
    CommandBase (CommonSchema commonSchema);

    virtual GS::String GetNamespace () const override final;
    virtual API_AddOnCommandExecutionPolicy GetExecutionPolicy () const override final;
    virtual void OnResponseValidationFailed (const GS::ObjectState& response) const override final;
#ifdef ServerMainVers_2600
    virtual bool IsProcessWindowVisible () const override final;
#endif
    virtual GS::Optional<GS::UniString> GetSchemaDefinitions () const override final;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    // Archicad uses the response schema for exactly one thing: validating what
    // the command returns. A failing command answers {"error": {code, message}}
    // as the whole response, which no command's own schema accepts, so declaring
    // one makes Archicad discard the error and report schema validation error
    // 4009 instead - the caller never sees the message. Nothing else reads it:
    // the documented schema comes from GetRawResponseSchema. So the API-facing
    // one stays empty and the commands override GetRawResponseSchema.
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override final;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const;

private:
    CommonSchema mCommonSchema;
};

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const GS::UniString& errorMessage);
GS::ObjectState CreateFailedExecutionResult (GSErrCode errorCode, const GS::UniString& errorMessage);
GS::ObjectState CreateSuccessfulExecutionResult ();

API_Guid    GetGuidFromObjectState (const GS::ObjectState& os);
API_Guid    GetGuidFromArrayItem (const GS::String& idFieldName, const GS::ObjectState& os);
inline API_Guid GetGuidFromElementsArrayItem (const GS::ObjectState& os)        { return GetGuidFromArrayItem ("elementId", os); }
inline API_Guid GetGuidFromAttributesArrayItem (const GS::ObjectState& os)      { return GetGuidFromArrayItem ("attributeId", os); }
inline API_Guid GetGuidFromIssuesArrayItem (const GS::ObjectState& os)          { return GetGuidFromArrayItem ("issueId", os); }
inline API_Guid GetGuidFromNavigatorItemIdArrayItem (const GS::ObjectState& os) { return GetGuidFromArrayItem ("navigatorItemId", os); }
inline API_Guid GetGuidFromDatabaseArrayItem (const GS::ObjectState& os)        { return GetGuidFromArrayItem ("databaseId", os); }
bool   IsSame2DCoordinate (const API_Coord& c1, const API_Coord& c2);
bool   IsSame3DCoordinate (const API_Coord3D& c1, const API_Coord3D& c2);
bool   IsSame2DCoordinate (const GS::ObjectState& o1, const GS::ObjectState& o2);
bool   IsSame3DCoordinate (const GS::ObjectState& o1, const GS::ObjectState& o2);
API_Coord   Get2DCoordinateFromObjectState (const GS::ObjectState& objectState);
API_Coord3D Get3DCoordinateFromObjectState (const GS::ObjectState& objectState);
API_RGBColor GetColorFromObjectState (const GS::ObjectState& objectState);
bool GetColor (const GS::ObjectState& objectState, const GS::String& fieldName, API_RGBColor& outColor);
GS::ObjectState Create2DCoordinateObjectState (const API_Coord& c);
GS::ObjectState Create3DCoordinateObjectState (const API_Coord3D& c);
GS::ObjectState CreatePolyArcObjectState (const API_PolyArc& a);
inline GS::ObjectState CreateGuidObjectState (const API_Guid& guid) { return GS::ObjectState ("guid", APIGuidToString (guid)); }
inline GS::ObjectState CreateGuidObjectState (const GS::Guid& guid) { return GS::ObjectState ("guid", guid.ToUniString()); }
GS::ObjectState CreateIdObjectState (const GS::String& idFieldName, const API_Guid& guid);
inline GS::ObjectState CreateElementIdObjectState (const API_Guid& guid)   { return CreateIdObjectState ("elementId", guid); }
inline GS::ObjectState CreateAttributeIdObjectState (const API_Guid& guid) { return CreateIdObjectState ("attributeId", guid); }
inline GS::ObjectState CreateIssueIdObjectState (const API_Guid& guid)     { return CreateIdObjectState ("issueId", guid); }
inline GS::ObjectState CreateDatabaseIdObjectState (const API_Guid& guid)  { return CreateIdObjectState ("databaseId", guid); }

// A hotlink instance is placed by an API_Tranmat. Every caller of this add-on
// thinks in an origin, a rotation and a mirror flag, so the two directions of
// that conversion live here, shared by the hotlink commands and by
// GetDetailsOfElements. Mirroring reflects the module's local X axis before
// the rotation is applied; a reflection is an orthogonal matrix, which is what
// Archicad expects.
API_Tranmat CreateHotlinkTransformation (const API_Coord3D& origin, double rotationAngle, bool mirrored);
void        DecomposeHotlinkTransformation (const API_Tranmat& transformation, API_Coord3D& origin, double& rotationAngle, bool& mirrored);

struct PolygonData {
    std::vector<API_Coord>   coords;
    std::vector<API_PolyArc> arcs;
    std::vector<double>      zCoords;
};
std::vector<PolygonData> GetPolygonsFromMemoCoords (const API_Guid& elemGuid, bool includeZCoords = false);
void AddPolygonFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName = {});
void AddPolygonWithHolesFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName, const GS::String& holesArrayFieldName, const GS::String& holeCoordsFieldName, const GS::Optional<GS::String>& holeArcsFieldName, bool includeZCoords = false);
bool GetHoleGeometry (const GS::ObjectState& holeOs, GS::Array<GS::ObjectState>& outCoords, GS::Array<GS::ObjectState>& outArcs);
GS::Optional<GS::UniString> ValidateHoles (const GS::Array<GS::ObjectState>& holes);
void AddBeamHolesFromMemo (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& holesFieldName);
void AddColumnSectionFromMemo (const API_Guid& elemGuid, GS::ObjectState& os);
void AddBeamSectionFromMemo (const API_Guid& elemGuid, GS::ObjectState& os);
GS::UniString AnchorIdToString (API_AnchorID anchorId);
API_AnchorID AnchorIdFromString (const GS::UniString& str, API_AnchorID defaultValue = APIAnc_MM);
GS::UniString WallReferenceLineLocationToString (API_WallReferenceLineLocationID location);
API_WallReferenceLineLocationID WallReferenceLineLocationFromString (const GS::UniString& str, API_WallReferenceLineLocationID defaultValue = APIWallRefLine_Outside);
GS::UniString ZoneRelToString (API_ZoneRelID zoneRel);
API_ZoneRelID ZoneRelFromString (const GS::UniString& str, API_ZoneRelID defaultValue = APIZRel_Boundary);
GS::ObjectState CreateStoryVisibilityObjectState (const API_StoryVisibility& visibility);
API_StoryVisibility GetStoryVisibilityFromObjectState (const GS::ObjectState& os);
GS::UniString SlabReferencePlaneLocationToString (API_SlabReferencePlaneLocationID location);
API_SlabReferencePlaneLocationID SlabReferencePlaneLocationFromString (const GS::UniString& str, API_SlabReferencePlaneLocationID defaultValue = APISlabRefPlane_Top);
GS::ObjectState CreateOverriddenMaterialObjectState (const API_OverriddenAttribute& attr);
API_OverriddenAttribute GetOverriddenMaterialFromObjectState (const GS::ObjectState& os);
#ifdef ServerMainVers_2700
GS::ObjectState CreateOverriddenPenObjectState (const API_OverriddenPen& pen);
API_OverriddenPen GetOverriddenPenFromObjectState (const GS::ObjectState& os);
#endif
GS::UniString CoverFillTransformationTypeToString (API_CoverFillTransformationTypeID type);
API_CoverFillTransformationTypeID CoverFillTransformationTypeFromString (const GS::UniString& str, API_CoverFillTransformationTypeID defaultValue = API_CoverFillTransformationType_Global);
GS::ObjectState CreateCoverFillTransformationObjectState (const API_CoverFillTransformation& transformation);
API_CoverFillTransformation GetCoverFillTransformationFromObjectState (const GS::ObjectState& os);
GS::ObjectState CreateCoverFillObjectState (bool use, bool useFromSurface, bool orientationComesFrom3D, API_AttributeIndex fillIndex, short foregroundPen, short backgroundPen, API_CoverFillTransformationTypeID transformationType, const API_CoverFillTransformation& transformation);
GS::UniString HatchOrientationTypeToString (API_HatchOrientationTypeID type);
API_HatchOrientationTypeID HatchOrientationTypeFromString (const GS::UniString& str, API_HatchOrientationTypeID defaultValue = API_HatchGlobal);
GS::ObjectState CreateHatchOrientationObjectState (const API_HatchOrientation& orientation);
API_HatchOrientation GetHatchOrientationFromObjectState (const GS::ObjectState& os);

// Defined in ExtendedElementCommands.cpp (not ElementCommands.cpp, where it's called from) -
// reading a Morph's body needs Model3D/MeshBody.hpp, which cannot be included in the same
// translation unit as ModelMeshBody.hpp (already required by ElementCommands.cpp for zone
// boundaries) without a "'GS' n'est pas membre de 'GS'" GDL header conflict - confirmed live,
// root cause not fully understood, kept as two separate translation units instead.
void AddMorphBodyFromMemo (const API_Element& elem, GS::ObjectState& typeSpecificDetails);

struct Story {
    Story (short _index, double _level)
        : index (_index)
        , level (_level)
    {}

    short  index;
    double level;
};
using Stories = std::map<short, Story>;

Stories GetStories ();
GS::Pair<short, double> GetFloorIndexAndOffset (const double zPos, const Stories& stories);
double GetZPos (const short floorIndex, const double offset, const Stories& stories);
// Same result as GetFloorIndexAndOffset (zPos, stories), except an explicit floorIndex value under
// floorIndexFieldName in `parameters` (when present) picks the floor directly instead of it being
// derived from zPos - the offset is still computed against zPos, just relative to the chosen
// floor's own base level. Use this wherever a Create/Modify command currently derives floorInd
// purely from a Z coordinate, to let callers pin the floor explicitly instead of relying on a
// guess that can land on the wrong floor for a Z value shared by more than one story.
GS::Pair<short, double> ResolveFloorIndexAndOffset (const GS::ObjectState& parameters, const char* floorIndexFieldName, const double zPos, const Stories& stories);
GS::UniString GetElementTypeNonLocalizedName (API_ElemTypeID typeID);
API_ElemTypeID GetElementTypeFromNonLocalizedName (const GS::UniString& typeStr);
short ParseAnchorPointString (const GS::UniString& anchorPoint);

API_Guid GetAttributeGuidFromIndex (API_AttrTypeID typeID, API_AttributeIndex index);
API_Attr_Head GetAttributeHeadFromGuid (API_Guid guid);
API_AttributeIndex GetAttributeIndexFromGuid (API_AttrTypeID typeID, API_Guid guid);

class DatabaseIdResolver {
public:
    static const DatabaseIdResolver& Instance ();

    API_Guid         GetIdOfDatabase(const API_DatabaseInfo& database) const;
    API_DatabaseInfo GetDatabaseWithId(const API_Guid& id) const;

private:
    DatabaseIdResolver();
    DatabaseIdResolver(const DatabaseIdResolver&) = delete;

    GS::BiHashTable<API_WindowTypeID, API_Guid> databaseTypeToIdTable;
};

GSErrCode ExecuteActionForEachDatabase (
    const GS::Array<API_Guid>& databaseIds,
    const std::function<GSErrCode ()>& action,
    const std::function<void ()>& actionSuccess,
    const std::function<void (GSErrCode, const GS::UniString&)>& actionFailure);

template<std::size_t N>
bool SetCharProperty (const GS::ObjectState* os, const char* propertyKey, char (&targetProperty)[N])
{
    GS::UniString propertyValue;
    if (os->Get (propertyKey, propertyValue)) {
        CHTruncate (propertyValue.ToCStr ().Get (), targetProperty, N);
        return true;
    }

    return false;
};

template<std::size_t N>
bool SetUCharProperty (const GS::ObjectState* os, const char* propertyKey, GS::uchar_t (&targetProperty)[N])
{
    GS::UniString propertyValue;
    if (os->Get (propertyKey, propertyValue)) {

        const auto ustrObject = propertyValue.ToUStr ();
        const GS::uchar_t* sourceString = ustrObject;

        GS::ucsncpy (targetProperty, sourceString, N);
        targetProperty[N - 1] = 0;
        return true;
    }

    return false;
}

GS::Array<API_PolyArc> GetPolyArcs (const GS::Array<GS::ObjectState>& arcs, Int32 startIndex);

void AddPolyToMemo (
    const GS::Array<GS::ObjectState>& coords,
    const GS::Array<GS::ObjectState>& arcs,
    Int32& iCoord,
    Int32& iArc,
    Int32& iPends,
    API_ElementMemo& memo,
    const API_EdgeTrimID* edgeTrimSideType = nullptr,
    const API_OverriddenAttribute* sideMaterial = nullptr,
    bool processVertexIDs = false);

GS::Optional<GS::UniString> BuildSlabMemoFromGeometry (
    API_Element& element,
    API_ElementMemo& memo,
    GS::Array<GS::ObjectState>& polygonOutline,
    const GS::Array<GS::ObjectState>& polygonArcs,
    const GS::Array<GS::ObjectState>& holes);

// Pushes the named favorite into the tool defaults of expectedTypeId, so a subsequent
// ACAPI_Element_GetDefaults returns the favorite's settings as the baseline for a new
// element. Mirrors ApplyFavoritesToElementDefaultsCommand: the element state plus the
// favorite's classifications, category values and user properties (some favorites are
// rejected by the create call without that extra metadata). Returns APIERR_REFUSEDPAR
// when the favorite belongs to a different element type. Defined in
// ExtendedElementCommands.cpp, next to the Window/Door specific variant.
GSErrCode ApplyFavoriteToElementDefaults (const GS::UniString& favoriteName, API_ElemTypeID expectedTypeId);

bool LoadElementHeaderByGuid (const API_Guid& elementGuid, API_Elem_Head& elementHeader);
bool DoesElementExist (const API_Guid& elementGuid, API_ElemTypeID expectedTypeId);
