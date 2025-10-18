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
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;

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

struct PolygonData {
    std::vector<API_Coord>   coords;
    std::vector<API_PolyArc> arcs;
    std::vector<double>      zCoords;
};
std::vector<PolygonData> GetPolygonsFromMemoCoords (const API_Guid& elemGuid, bool includeZCoords = false);
void AddPolygonFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName = {});
void AddPolygonWithHolesFromMemoCoords (const API_Guid& elemGuid, GS::ObjectState& os, const GS::String& coordsFieldName, const GS::Optional<GS::String>& arcsFieldName, const GS::String& holesArrayFieldName, const GS::String& holeCoordsFieldName, const GS::Optional<GS::String>& holeArcsFieldName, bool includeZCoords = false);
bool GetHoleGeometry (const GS::ObjectState& holeOs, GS::Array<GS::ObjectState>& outCoords, GS::Array<GS::ObjectState>& outArcs);

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
GS::UniString GetElementTypeNonLocalizedName (API_ElemTypeID typeID);
API_ElemTypeID GetElementTypeFromNonLocalizedName (const GS::UniString& typeStr);

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