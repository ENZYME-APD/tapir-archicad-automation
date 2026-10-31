#include "AttributeCommands.hpp"
#include "MigrationHelper.hpp"
#include "ObjectStateJSONConversion.hpp"
#include "GSUnID.hpp"
#include "ProfileVectorImage.hpp"
#include "ProfileAdditionalInfo.hpp"
#include "VectorImageIterator.hpp"
#include "FileSystem.hpp"
#include "UserDataPairs.hpp"
#ifdef ServerMainVers_2700
#include "ADBAttributeIndex.hpp"
#endif
#ifdef ServerMainVers_3000
#include <optional>
#endif

// Before AC27, API_AttributeIndex is a plain integer typedef with no IsPositive() method; AC27+ replaced it with
// a class. Kept local (rather than in the shared MigrationHelper shim) since it is only used in this file.
static bool IsPositiveAttributeIndex (const API_AttributeIndex& index)
{
#ifdef ServerMainVers_2700
    return index.IsPositive ();
#else
    return index > 0;
#endif
}

// Shared "fields" filter for detailed Get commands, matching the {ids, properties} pattern already used by
// GetPropertyValuesOfAttributes: when "fields" is omitted every field is returned (backward compatible default);
// when given, only the named fields are populated, so a caller reading e.g. hundreds of Lines can skip the
// potentially large dashItems/lineItems arrays and fetch only the scalars it actually needs.
class GetFieldsFilter {
public:
    explicit GetFieldsFilter (const GS::ObjectState& parameters)
    {
        hasFilter = parameters.Get ("fields", fields);
    }

    bool Wants (const char* fieldName) const
    {
        if (!hasFilter) {
            return true;
        }
        for (const GS::UniString& f : fields) {
            if (f == fieldName) {
                return true;
            }
        }
        return false;
    }

private:
    bool hasFilter = false;
    GS::Array<GS::UniString> fields;
};

static GS::Optional<GS::ObjectState> GetAttributeIdFromIndex (API_AttrTypeID attributeType, const API_AttributeIndex& attributeIndex)
{
    if (!IsPositiveAttributeIndex (attributeIndex)) {
        return GS::NoValue;
    }
    API_Attribute attribute = {};
    attribute.header.typeID = attributeType;
    attribute.header.index = attributeIndex;
    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return GS::NoValue;
    }
    return CreateAttributeIdObjectState (attribute.header.guid);
}

static bool GetAttributeIndexFromAttributeId (const GS::ObjectState& attributeId, API_AttrTypeID attributeType, API_AttributeIndex& attributeIndex)
{
    API_Attribute attribute = {};
    attribute.header.guid = GetGuidFromAttributesArrayItem (attributeId);
    attribute.header.typeID = attributeType;
    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return false;
    }

    attributeIndex = attribute.header.index;
    return true;
}

// API_Attr_Head::name is the legacy 8-bit name: non-ASCII characters come back garbled from it and the
// built-in Archicad Layer only has a control character there. Archicad hands out the real Unicode name
// through header.uniStringNamePtr, but only when it points at storage before the Get call, so an
// attribute that arrived through ACAPI_Attribute_GetAttributesByType has to be read once more.
static GS::UniString GetAttributeName (const API_Attr_Head& header)
{
    GS::UniString name;
    API_Attribute attr = {};
    attr.header.typeID = header.typeID;
    attr.header.index = header.index;
    attr.header.guid = header.guid;
    attr.header.uniStringNamePtr = &name;
    const bool found = (ACAPI_Attribute_Get (&attr) == NoError);
    DisposeAttribute (attr);
    return found ? name : GS::UniString (header.name);
}

#ifdef ServerMainVers_3000
// Archicad 30 records how, from where, by whom and when an attribute entered the project
// (ACAPI_Attribute_GetCreationInfo). The method names below mirror the API_AttrCreationMethod
// enumerators; the default branch keeps a method added by a later version from breaking the response.
static GS::UniString GetAttributeCreationMethodName (API_AttrCreationMethod method)
{
    switch (method) {
        case APIAttrCreationMethod_AddOn:         return "AddOn";
        case APIAttrCreationMethod_Merge:         return "Merge";
        case APIAttrCreationMethod_Paste:         return "Paste";
        case APIAttrCreationMethod_New:           return "New";
        case APIAttrCreationMethod_Import:        return "Import";
        case APIAttrCreationMethod_Explode:       return "Explode";
        case APIAttrCreationMethod_Duplicate:     return "Duplicate";
        case APIAttrCreationMethod_Hotlink:       return "Hotlink";
        case APIAttrCreationMethod_XRef:          return "XRef";
        case APIAttrCreationMethod_Library:       return "Library";
        case APIAttrCreationMethod_LibraryMaster: return "LibraryMaster";
        case APIAttrCreationMethod_Unspecified:
        default:                                  return "Unspecified";
    }
}

// Archicad hands back NoError with an empty optional, not an error, for an attribute it has no creation
// record of (legacy or imported attributes), so such an attribute simply gets no creationInfo.
static bool GetAttributeCreationInfo (const API_Attr_Head& header, GS::ObjectState& creationInfoOS)
{
    std::optional<API_AttributeCreationInfo> creationInfo;
    if (ACAPI_Attribute_GetCreationInfo (header.typeID, header.index, creationInfo) != NoError || !creationInfo.has_value ()) {
        return false;
    }

    creationInfoOS.Add ("method", GetAttributeCreationMethodName (creationInfo->methodID));
    creationInfoOS.Add ("methodDescription", creationInfo->methodDescription);
    creationInfoOS.Add ("source", creationInfo->source);
    creationInfoOS.Add ("creatorUserName", creationInfo->creatorUserName);
    // API_AttributeCreationInfo::creationTime is a GSTime (seconds since 1970-01-01 00:00:00 UTC), the
    // same scale as modificationTime.
    creationInfoOS.Add ("creationTime", static_cast<Int64> (creationInfo->creationTime));
    return true;
}
#endif

static API_AttrTypeID ConvertAttributeTypeStringToID (const GS::UniString& typeStr)
{
    if (typeStr == "Layer")
        return API_LayerID;
    if (typeStr == "Line")
        return API_LinetypeID;
    if (typeStr == "Fill")
        return API_FilltypeID;
    if (typeStr == "Composite")
        return API_CompWallID;
    if (typeStr == "Surface")
        return API_MaterialID;
    if (typeStr == "LayerCombination")
        return API_LayerCombID;
    if (typeStr == "ZoneCategory")
        return API_ZoneCatID;
    if (typeStr == "Profile")
        return API_ProfileID;
    if (typeStr == "PenTable")
        return API_PenTableID;
    if (typeStr == "MEPSystem")
        return API_MEPSystemID;
    if (typeStr == "OperationProfile")
        return API_OperationProfileID;
    if (typeStr == "BuildingMaterial")
        return API_BuildingMaterialID;
    return API_ZombieAttrID;
}

GetAttributesByTypeCommand::GetAttributesByTypeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAttributesByTypeCommand::GetName () const
{
    return "GetAttributesByType";
}

GS::Optional<GS::UniString> GetAttributesByTypeCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeType": {
                "$ref": "#/AttributeType"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeType"
        ]
    })";
}

GS::Optional<GS::UniString> GetAttributesByTypeCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/AttributeHeadersOrError"
    })";
}

GS::ObjectState GetAttributesByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString typeStr;
    parameters.Get ("attributeType", typeStr);

    API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);
    if (typeID == API_ZombieAttrID) {
        return CreateErrorResponse (APIERR_BADPARS,
            GS::UniString::Printf ("Invalid attributeType '%T'.", typeStr.ToPrintf ()));
    }

    GS::ObjectState response;
    const auto& attributes = response.AddList<GS::ObjectState> ("attributes");

    GS::Array<API_Attribute> attrs;
    ACAPI_Attribute_GetAttributesByType (typeID, attrs);

    for (API_Attribute& attr : attrs) {
        GS::ObjectState attributeDetails;
        attributeDetails.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        attributeDetails.Add ("index", GetAttributeIndex (attr.header.index));
        attributeDetails.Add ("name", GetAttributeName (attr.header));
        // API_Attr_Head::modiTime is a GSTime (seconds since 1970-01-01 00:00:00 UTC) in a UInt64,
        // the same stamp the Attribute Manager writes as ModiTime into its XML export.
        attributeDetails.Add ("modificationTime", static_cast<Int64> (attr.header.modiTime));
#ifdef ServerMainVers_3000
        GS::ObjectState creationInfo;
        if (GetAttributeCreationInfo (attr.header, creationInfo)) {
            attributeDetails.Add ("creationInfo", creationInfo);
        }
#endif
        attributes (attributeDetails);

        DisposeAttribute (attr);
    }

    return response;
}

GetAttributesFromProjectCommand::GetAttributesFromProjectCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetAttributesFromProjectCommand::GetName () const
{
    return "GetAttributesFromProject";
}

GS::Optional<GS::UniString> GetAttributesFromProjectCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "projectFilePath": {
                "type": "string",
                "description": "The absolute path of the project or template file (for example .pln or .tpl) to read the attributes from.",
                "minLength": 1
            },
            "attributeTypes": {
                "type": "array",
                "description": "The attribute types to read. Layer, Surface, BuildingMaterial and MEPSystem cannot be read from a file that is not open; they get an error item.",
                "items": {
                    "$ref": "#/AttributeType"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "projectFilePath",
            "attributeTypes"
        ]
    })";
}

GS::Optional<GS::UniString> GetAttributesFromProjectCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "oneOf": [
            {
                "type": "object",
                "properties": {
                    "attributesByType": {
                        "type": "array",
                        "description": "One item per requested attribute type, in the order of attributeTypes.",
                        "items": {
                            "type": "object",
                            "oneOf": [
                                {
                                    "type": "object",
                                    "properties": {
                                        "attributeType": {
                                            "$ref": "#/AttributeType"
                                        },
                                        "attributes": {
                                            "$ref": "#/AttributeHeaders"
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "attributeType",
                                        "attributes"
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
                    "attributesByType"
                ]
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    })";
}

#ifdef ServerMainVers_2700
// ACAPI_Attribute_GetAttributesFromProject returns NoError and an empty list for these types
// (tested on Archicad 29 with .pln and .tpl files that contain them, see #1582), so an empty list of
// one of them means "not read", not "the file has none". A non-empty list is still passed through,
// in case an Archicad version reads them.
static bool IsNotReadFromProjectFileWhenEmpty (API_AttrTypeID typeID)
{
    return typeID == API_LayerID ||
        typeID == API_MaterialID ||
        typeID == API_BuildingMaterialID ||
        typeID == API_MEPSystemID;
}
#endif

GS::ObjectState GetAttributesFromProjectCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString projectFilePath;
    if (!parameters.Get ("projectFilePath", projectFilePath) || projectFilePath.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "projectFilePath is missing.");
    }
    const IO::Location location (projectFilePath);
    bool exists = false;
    if (IO::fileSystem.Contains (location, &exists) != NoError || !exists) {
        return CreateErrorResponse (APIERR_BADPARS,
            GS::UniString::Printf ("The file '%T' does not exist.", projectFilePath.ToPrintf ()));
    }

    GS::Array<GS::UniString> attributeTypes;
    parameters.Get ("attributeTypes", attributeTypes);

    GS::ObjectState response;
    const auto& attributesByType = response.AddList<GS::ObjectState> ("attributesByType");

    for (const GS::UniString& typeStr : attributeTypes) {
        const API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);
        if (typeID == API_ZombieAttrID) {
            attributesByType (CreateErrorResponse (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid attributeType '%T'.", typeStr.ToPrintf ())));
            continue;
        }

        // Only reads the attributes of the other file; nothing is created in the open project.
        GS::Array<GS::Pair<API_Attribute, API_AttributeDefExt>> loaded;
        const GSErrCode err = ACAPI_Attribute_GetAttributesFromProject (location, typeID, &loaded);

        GS::ObjectState typeResult;
        typeResult.Add ("attributeType", typeStr);
        const auto& attributes = typeResult.AddList<GS::ObjectState> ("attributes");
        for (GS::Pair<API_Attribute, API_AttributeDefExt>& item : loaded) {
            const API_Attr_Head& header = item.first.header;
            GS::ObjectState attributeDetails;
            attributeDetails.Add ("attributeId", CreateGuidObjectState (header.guid));
            attributeDetails.Add ("index", GetAttributeIndex (header.index));
            // These attributes are not in the open project, so the Unicode name cannot be read back
            // through ACAPI_Attribute_Get as GetAttributeName does: only the legacy 8-bit name is available.
            attributeDetails.Add ("name", GS::UniString (header.name));
            attributeDetails.Add ("modificationTime", static_cast<Int64> (header.modiTime));
            attributes (attributeDetails);

            ACAPI_DisposeAttrDefsHdlsExt (&item.second);
            DisposeAttribute (item.first);
        }

        if (err != NoError) {
            attributesByType (CreateErrorResponse (err,
                GS::UniString::Printf ("Failed to read the %T attributes of the project file.", typeStr.ToPrintf ())));
        } else if (loaded.IsEmpty () && IsNotReadFromProjectFileWhenEmpty (typeID)) {
            attributesByType (CreateErrorResponse (APIERR_NOTSUPPORTED,
                GS::UniString::Printf ("Archicad returned no %T attributes from the project file. It does not read this type from a file that is not open; open the file and use GetAttributesByType instead.", typeStr.ToPrintf ())));
        } else {
            attributesByType (typeResult);
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_GENERAL, "GetAttributesFromProject needs Archicad 27 or later.");
#endif
}

GetBuildingMaterialPhysicalPropertiesCommand::GetBuildingMaterialPhysicalPropertiesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetBuildingMaterialPhysicalPropertiesCommand::GetName () const
{
    return "GetBuildingMaterialPhysicalProperties";
}

GS::Optional<GS::UniString> GetBuildingMaterialPhysicalPropertiesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetBuildingMaterialPhysicalPropertiesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "properties" : {
                "$ref": "#/BuildingMaterialPhysicalPropertiesList"
            }
        },
        "additionalProperties": false,
        "required": [
            "properties"
        ]
    })";
}

GS::ObjectState GetBuildingMaterialPhysicalPropertiesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("properties");
    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::ObjectState attributeId;
        attributeIdItem.Get ("attributeId", attributeId);

        GS::Guid attributeGuid;
        attributeId.Get ("guid", attributeGuid);

        API_Attribute buildMat = {};
        buildMat.header.typeID = API_BuildingMaterialID;
        buildMat.header.guid = GSGuid2APIGuid (attributeGuid);
        GSErrCode err = ACAPI_Attribute_Get (&buildMat);
        if (err != NoError) {
            listAdder (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState properties;
        properties.Add ("thermalConductivity", buildMat.buildingMaterial.thermalConductivity);
        properties.Add ("density", buildMat.buildingMaterial.density);
        properties.Add ("heatCapacity", buildMat.buildingMaterial.heatCapacity);
        properties.Add ("embodiedEnergy", buildMat.buildingMaterial.embodiedEnergy);
        properties.Add ("embodiedCarbon", buildMat.buildingMaterial.embodiedCarbon);

        GS::ObjectState propertiesObj;
        propertiesObj.Add ("properties", properties);

        listAdder (propertiesObj);
    }

    return response;
}

GetLayerCombinationsCommand::GetLayerCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLayerCombinationsCommand::GetName () const
{
    return "GetLayerCombinations";
}

GS::Optional<GS::UniString> GetLayerCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributes": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributes"
        ]
    })";
}

GS::Optional<GS::UniString> GetLayerCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layerCombinations" : {
                "type": "array",
                "description" : "A list of layer combinations.",
                "items": {
                    "$ref": "#/LayerCombinationAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "layerCombinations"
        ]
    })";
}

GS::ObjectState GetLayerCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributes", attributeIds);

    GS::ObjectState response;
    const auto& layerCombinations = response.AddList<GS::ObjectState> ("layerCombinations");
    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attribute = {};
        attribute.header.typeID = API_LayerCombID;
        attribute.header.guid = GetGuidFromAttributesArrayItem(attributeIdItem);
        attribute.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attribute);
        if (err != NoError) {
            layerCombinations (CreateErrorResponse (err, "Failed to retrieve layer combination attribute."));
            continue;
        }

        API_AttributeDef attributeDef = {};
        err = ACAPI_Attribute_GetDef (API_LayerCombID, attribute.header.index, &attributeDef);
        if (err != NoError) {
            layerCombinations (CreateErrorResponse (err, "Failed to retrieve details of layer combination attribute."));
	        ACAPI_DisposeAttrDefsHdls (&attributeDef);
            continue;
        }

        GS::ObjectState layerCombination;
        layerCombination.Add ("attributeId", CreateGuidObjectState (attribute.header.guid));
        layerCombination.Add ("attributeIndex", GetAttributeIndex (attribute.header.index));
        layerCombination.Add ("name", name);
        const auto& layers = layerCombination.AddList<GS::ObjectState> ("layers");
#ifdef ServerMainVers_2700
        for (const auto& kv : *attributeDef.layer_statItems) {
#ifdef ServerMainVers_2800
            const API_AttributeIndex& layerIndex = kv.key;
            const API_LayerStat& layerStat = kv.value;
#else
            const API_AttributeIndex& layerIndex = *kv.key;
            const API_LayerStat& layerStat = *kv.value;
#endif
#else
        for (Int32 i = 0; i < attribute.layerComb.lNumb; ++i) {
            const API_LayerStat& layerStat = (*attributeDef.layer_statItems)[i];
            const API_AttributeIndex& layerIndex = layerStat.lInd;
#endif
            layers (GS::ObjectState (
                "attributeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LayerID, layerIndex)),
                "isHidden", (layerStat.lFlags & APILay_Hidden) != 0,
                "isLocked", (layerStat.lFlags & APILay_Locked) != 0,
                "isWireframe", (layerStat.lFlags & APILay_ForceToWire) != 0,
                "intersectionGroupNr", layerStat.conClassId));
        }

        ACAPI_DisposeAttrDefsHdls (&attributeDef);

        layerCombinations (GS::ObjectState ("layerCombination", layerCombination));
    }

    return response;
}

GetCurrentLayerCombinationCommand::GetCurrentLayerCombinationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentLayerCombinationCommand::GetName () const
{
    return "GetCurrentLayerCombination";
}

GS::Optional<GS::UniString> GetCurrentLayerCombinationCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layerCombinationId": {
                "$ref": "#/AttributeId",
                "description": "The layer combination in effect in the current window. Missing when the current state of the layers matches no layer combination."
            },
            "name": {
                "type": "string",
                "description": "The name of the layer combination. Missing together with layerCombinationId."
            }
        },
        "additionalProperties": false,
        "required": []
    })";
}

GS::ObjectState GetCurrentLayerCombinationCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_AttributeIndex layerCombIndex = ACAPI_CreateAttributeIndex (0);
    GSErrCode err = ACAPI_Navigator_GetCurrLayerComb (&layerCombIndex);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the current layer combination.");
    }

    GS::ObjectState response;
    // Index 0 means the current state of the layers matches no layer combination.
    if (GetAttributeIndex (layerCombIndex) == 0) {
        return response;
    }

    GS::UniString name;
    API_Attribute attribute = {};
    attribute.header.typeID = API_LayerCombID;
    attribute.header.index = layerCombIndex;
    attribute.header.uniStringNamePtr = &name;
    err = ACAPI_Attribute_Get (&attribute);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to retrieve the current layer combination attribute.");
    }

    response.Add ("layerCombinationId", CreateGuidObjectState (attribute.header.guid));
    response.Add ("name", name);
    return response;
}

static GS::UniString LineItemTypeToString (API_LtypItemID itemType)
{
    switch (itemType) {
        case APILine_SeparatorItemType:  return "Separator";
        case APILine_CenterDotItemType:  return "CenterDot";
        case APILine_CenterLineItemType: return "CenterLine";
        case APILine_DotItemType:        return "Dot";
        case APILine_RightAngleItemType: return "RightAngle";
        case APILine_ParallelItemType:   return "Parallel";
        case APILine_CircItemType:       return "Circle";
        case APILine_ArcItemType:        return "Arc";
        case APILine_LineItemType:
        default:                         return "Line";
    }
}

GetLinesCommand::GetLinesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLinesCommand::GetName () const
{
    return "GetLines";
}

GS::Optional<GS::UniString> GetLinesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Line. If omitted, every field is returned. Requesting only the fields you need avoids fetching dashItems/lineItems, which can be large.",
                "items": {
                    "$ref": "#/LineAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetLinesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "lines": {
                "type": "array",
                "description": "A list of lines or errors.",
                "items": {
                    "$ref": "#/LineAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "lines"
        ]
    })";
}

GS::ObjectState GetLinesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& lines = response.AddList<GS::ObjectState> ("lines");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::ObjectState attributeId;
        attributeIdItem.Get ("attributeId", attributeId);
        GS::Guid attributeGuid;
        attributeId.Get ("guid", attributeGuid);

        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_LinetypeID;
        attr.header.guid = GSGuid2APIGuid (attributeGuid);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            lines (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState line;
        line.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        line.Add ("index", GetAttributeIndex (attr.header.index));
        line.Add ("name", name);

        if (wantsField.Wants ("scaleWithPlan")) {
            line.Add ("scaleWithPlan", (attr.header.flags & APILine_ScaleWithPlan) != 0);
        }
        if (wantsField.Wants ("defineScale")) {
            line.Add ("defineScale", attr.linetype.defineScale);
        }

        GS::UniString lineTypeStr = (attr.linetype.type == APILine_DashedLine) ? "Dashed" : (attr.linetype.type == APILine_SymbolLine) ? "Symbol" : "Solid";
        if (wantsField.Wants ("lineType")) {
            line.Add ("lineType", lineTypeStr);
        }
        if (wantsField.Wants ("period") && attr.linetype.type != APILine_SolidLine) {
            line.Add ("period", attr.linetype.period);
        }
        if (wantsField.Wants ("height") && attr.linetype.type == APILine_SymbolLine) {
            line.Add ("height", attr.linetype.height);
        }

        bool wantsDashItems = wantsField.Wants ("dashItems") && attr.linetype.type == APILine_DashedLine;
        bool wantsLineItems = wantsField.Wants ("lineItems") && attr.linetype.type == APILine_SymbolLine;
        if (wantsDashItems || wantsLineItems) {
            API_AttributeDef defs = {};
            if (ACAPI_Attribute_GetDef (API_LinetypeID, attr.header.index, &defs) == NoError) {
                // Clamp against the handle's real size rather than trusting attr.linetype.nItems blindly: if the
                // two ever disagree, indexing past the handle's actual allocation crashes ArchiCAD outright
                // instead of failing gracefully.
                if (wantsDashItems && defs.ltype_dashItems != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) defs.ltype_dashItems) / sizeof (API_DashItems));
                    count = GS::Min (count, attr.linetype.nItems);
                    const auto& dashItemsList = line.AddList<GS::ObjectState> ("dashItems");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_DashItems& item = (*defs.ltype_dashItems)[i];
                        dashItemsList (GS::ObjectState ("dash", item.dash, "gap", item.gap));
                    }
                }
                if (wantsLineItems && defs.ltype_lineItems != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) defs.ltype_lineItems) / sizeof (API_LineItems));
                    count = GS::Min (count, attr.linetype.nItems);
                    const auto& lineItemsList = line.AddList<GS::ObjectState> ("lineItems");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_LineItems& item = (*defs.ltype_lineItems)[i];
                        lineItemsList (GS::ObjectState (
                            "itemType", LineItemTypeToString (item.itemType),
                            "centerOffset", item.itemCenterOffs,
                            "length", item.itemLength,
                            "begPos", Create2DCoordinateObjectState (item.itemBegPos),
                            "endPos", Create2DCoordinateObjectState (item.itemEndPos),
                            "radius", item.itemRadius,
                            "beginAngle", item.itemBegAngle,
                            "endAngle", item.itemEndAngle));
                    }
                }
                ACAPI_DisposeAttrDefsHdls (&defs);
            }
        }

        lines (line);
    }

    return response;
}

static GS::UniString FillSubTypeToString (API_FillSubtype subType)
{
    switch (subType) {
        case APIFill_Solid:          return "Solid";
        case APIFill_Empty:          return "Empty";
        case APIFill_Symbol:         return "Symbol";
        case APIFill_LinearGradient: return "LinearGradient";
        case APIFill_RadialGradient: return "RadialGradient";
        case APIFill_Image:          return "Image";
        case APIFill_Vector:
        default:                     return "Vector";
    }
}

static GS::UniString GetBitPatternHex (const API_Pattern& pattern)
{
    char hex[17] = {};
    for (UInt32 i = 0; i < 8; ++i) {
        sprintf (hex + i * 2, "%02X", (unsigned char) pattern[i]);
    }
    return GS::UniString (hex);
}

GetFillsCommand::GetFillsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFillsCommand::GetName () const
{
    return "GetFills";
}

GS::Optional<GS::UniString> GetFillsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Fill. If omitted, every field is returned. Requesting only the fields you need avoids fetching lineItems/symbolLines/symbolArcs/symbolHotspots, which can be large.",
                "items": {
                    "$ref": "#/FillAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetFillsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fills": {
                "type": "array",
                "description": "A list of fills or errors.",
                "items": {
                    "$ref": "#/FillAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "fills"
        ]
    })";
}

GS::ObjectState GetFillsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& fills = response.AddList<GS::ObjectState> ("fills");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_FilltypeID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            fills (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState fill;
        fill.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        fill.Add ("index", GetAttributeIndex (attr.header.index));
        fill.Add ("name", name);

        if (wantsField.Wants ("subType")) {
            fill.Add ("subType", FillSubTypeToString (attr.filltype.subType));
        }
        if (wantsField.Wants ("scaleWithPlan")) {
            fill.Add ("scaleWithPlan", (attr.header.flags & APIFill_ScaleWithPlan) != 0);
        }
        if (wantsField.Wants ("useForWalls")) {
            fill.Add ("useForWalls", (attr.header.flags & APIFill_ForWall) != 0);
        }
        if (wantsField.Wants ("useForDraft")) {
            fill.Add ("useForDraft", (attr.header.flags & APIFill_ForPoly) != 0);
        }
        if (wantsField.Wants ("useForCover")) {
            fill.Add ("useForCover", (attr.header.flags & APIFill_ForCover) != 0);
        }
        if (wantsField.Wants ("horizontalSpacing")) {
            fill.Add ("horizontalSpacing", attr.filltype.hXSpac);
        }
        if (wantsField.Wants ("verticalSpacing")) {
            fill.Add ("verticalSpacing", attr.filltype.hYSpac);
        }
        if (wantsField.Wants ("angle")) {
            fill.Add ("angle", attr.filltype.hAngle);
        }
        if (wantsField.Wants ("bitPattern") && attr.filltype.subType == APIFill_Vector) {
            fill.Add ("bitPattern", GetBitPatternHex (attr.filltype.bitPat));
        }
        bool isGradient = (attr.filltype.subType == APIFill_LinearGradient || attr.filltype.subType == APIFill_RadialGradient);
        if (wantsField.Wants ("gradientStart") && isGradient) {
            fill.Add ("gradientStart", Create2DCoordinateObjectState (attr.filltype.c1));
        }
        if (wantsField.Wants ("gradientEnd") && isGradient) {
            fill.Add ("gradientEnd", Create2DCoordinateObjectState (attr.filltype.c2));
        }
        // For Symbol fills c1/c2 are the offset vectors of the repeating motif (X1/Y1 and X2/Y2 in the Fill Types
        // dialog); without them a copied symbol fill draws every motif on top of the other.
        bool isSymbol = (attr.filltype.subType == APIFill_Symbol);
        if (wantsField.Wants ("symbolOffset1") && isSymbol) {
            fill.Add ("symbolOffset1", Create2DCoordinateObjectState (attr.filltype.c1));
        }
        if (wantsField.Wants ("symbolOffset2") && isSymbol) {
            fill.Add ("symbolOffset2", Create2DCoordinateObjectState (attr.filltype.c2));
        }
        if (wantsField.Wants ("percent")) {
            fill.Add ("percent", attr.filltype.percent);
        }
        if (wantsField.Wants ("texture")) {
            GS::ObjectState texture (
                "name", GS::UniString (attr.filltype.textureName),
                "rotationAngle", attr.filltype.textureRotAng,
                "xSize", attr.filltype.textureXSize,
                "ySize", attr.filltype.textureYSize,
                "mirrorX", (attr.filltype.textureStatus & APITxtr_MirrorX) != 0,
                "mirrorY", (attr.filltype.textureStatus & APITxtr_MirrorY) != 0);
            fill.Add ("texture", texture);
        }

        bool wantsLineItems = wantsField.Wants ("lineItems") && attr.filltype.subType == APIFill_Vector;
        bool wantsSymbolGeometry = attr.filltype.subType == APIFill_Symbol &&
            (wantsField.Wants ("symbolLines") || wantsField.Wants ("symbolArcs") || wantsField.Wants ("symbolHotspots"));
        if (wantsLineItems || wantsSymbolGeometry) {
            API_AttributeDefExt fillDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_FilltypeID, attr.header.index, &fillDefs) == NoError) {
                if (wantsLineItems && fillDefs.fill_lineItems != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) fillDefs.fill_lineItems) / sizeof (API_FillLine));
                    count = GS::Min (count, attr.filltype.linNumb);
                    const auto& lineItemsList = fill.AddList<GS::ObjectState> ("lineItems");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_FillLine& item = (*fillDefs.fill_lineItems)[i];
                        GS::ObjectState lineItem (
                            "frequency", item.lFreq,
                            "direction", item.lDir,
                            "offsetLine", item.lOffsetLine,
                            "offset", Create2DCoordinateObjectState (item.lOffset));
                        if (item.lPartNumb > 0 && fillDefs.fill_lineLength != nullptr) {
                            Int32 lengthCount = (Int32) (BMGetHandleSize ((GSHandle) fillDefs.fill_lineLength) / sizeof (double));
                            const auto& lengthsList = lineItem.AddList<double> ("lineLengths");
                            for (short j = 0; j < item.lPartNumb && (item.lPartOffs + j) < lengthCount; ++j) {
                                lengthsList ((*fillDefs.fill_lineLength)[item.lPartOffs + j]);
                            }
                        }
                        lineItemsList (lineItem);
                    }
                }
                if (wantsField.Wants ("symbolLines") && attr.filltype.subType == APIFill_Symbol && fillDefs.sfill_Items.sfill_Lines != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) fillDefs.sfill_Items.sfill_Lines) / sizeof (API_SFill_Line));
                    count = GS::Min (count, attr.filltype.linNumb);
                    const auto& symbolLinesList = fill.AddList<GS::ObjectState> ("symbolLines");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_SFill_Line& item = (*fillDefs.sfill_Items.sfill_Lines)[i];
                        symbolLinesList (GS::ObjectState ("begin", Create2DCoordinateObjectState (item.begC), "end", Create2DCoordinateObjectState (item.endC)));
                    }
                }
                if (wantsField.Wants ("symbolArcs") && attr.filltype.subType == APIFill_Symbol && fillDefs.sfill_Items.sfill_Arcs != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) fillDefs.sfill_Items.sfill_Arcs) / sizeof (API_SFill_Arc));
                    count = GS::Min (count, attr.filltype.arcNumb);
                    const auto& symbolArcsList = fill.AddList<GS::ObjectState> ("symbolArcs");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_SFill_Arc& item = (*fillDefs.sfill_Items.sfill_Arcs)[i];
                        symbolArcsList (GS::ObjectState ("begin", Create2DCoordinateObjectState (item.begC), "origin", Create2DCoordinateObjectState (item.origC), "angle", item.angle));
                    }
                }
                if (wantsField.Wants ("symbolHotspots") && attr.filltype.subType == APIFill_Symbol && fillDefs.sfill_Items.sfill_HotSpots != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) fillDefs.sfill_Items.sfill_HotSpots) / sizeof (API_Coord));
                    count = GS::Min (count, attr.filltype.hotNumb);
                    const auto& symbolHotspotsList = fill.AddList<GS::ObjectState> ("symbolHotspots");
                    for (Int32 i = 0; i < count; ++i) {
                        symbolHotspotsList (Create2DCoordinateObjectState ((*fillDefs.sfill_Items.sfill_HotSpots)[i]));
                    }
                }
                ACAPI_DisposeAttrDefsHdlsExt (&fillDefs);
            }
        }

        fills (fill);
    }

    return response;
}

GetZoneCategoriesCommand::GetZoneCategoriesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetZoneCategoriesCommand::GetName () const
{
    return "GetZoneCategories";
}

GS::Optional<GS::UniString> GetZoneCategoriesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Zone Category. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/ZoneCategoryAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetZoneCategoriesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zoneCategories": {
                "type": "array",
                "description": "A list of zone categories or errors.",
                "items": {
                    "$ref": "#/ZoneCategoryAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneCategories"
        ]
    })";
}

GS::ObjectState GetZoneCategoriesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& zoneCategories = response.AddList<GS::ObjectState> ("zoneCategories");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_ZoneCatID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            zoneCategories (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState zoneCategory;
        zoneCategory.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        zoneCategory.Add ("index", GetAttributeIndex (attr.header.index));
        zoneCategory.Add ("name", name);

        if (wantsField.Wants ("categoryCode")) {
            zoneCategory.Add ("categoryCode", GS::UniString (attr.zoneCat.catCode));
        }
        if (wantsField.Wants ("color")) {
            const API_RGBColor& rgb = attr.zoneCat.rgb;
            zoneCategory.Add ("color", GS::ObjectState ("red", rgb.f_red, "green", rgb.f_green, "blue", rgb.f_blue));
        }
        if (wantsField.Wants ("stampName")) {
            zoneCategory.Add ("stampName", GS::UniString (attr.zoneCat.stampName));
        }
        if (wantsField.Wants ("stampMainGuid")) {
            zoneCategory.Add ("stampMainGuid", APIGuidToString (attr.zoneCat.stampMainGuid));
        }
        if (wantsField.Wants ("stampRevGuid")) {
            zoneCategory.Add ("stampRevGuid", APIGuidToString (attr.zoneCat.stampRevGuid));
        }

        zoneCategories (zoneCategory);
    }

    return response;
}

GetMEPSystemsCommand::GetMEPSystemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetMEPSystemsCommand::GetName () const
{
    return "GetMEPSystems";
}

GS::Optional<GS::UniString> GetMEPSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each MEP System. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/MEPSystemAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetMEPSystemsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "mepSystems": {
                "type": "array",
                "description": "A list of MEP systems or errors.",
                "items": {
                    "$ref": "#/MEPSystemAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "mepSystems"
        ]
    })";
}

GS::ObjectState GetMEPSystemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& mepSystems = response.AddList<GS::ObjectState> ("mepSystems");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_MEPSystemID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            mepSystems (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState mepSystem;
        mepSystem.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        mepSystem.Add ("index", GetAttributeIndex (attr.header.index));
        mepSystem.Add ("name", name);

        if (wantsField.Wants ("domain")) {
            const auto& domainList = mepSystem.AddList<GS::UniString> ("domain");
#ifdef ServerMainVers_2900
            switch (attr.mepSystem.domain) {
                case APIMEPDomain_Piping:       domainList ("Piping"); break;
                case APIMEPDomain_CableCarrier:  domainList ("CableCarrier"); break;
                case APIMEPDomain_Ventilation:
                default:                         domainList ("Ventilation"); break;
            }
#else
            if (attr.mepSystem.isForDuctwork) {
                domainList ("Ventilation");
            }
            if (attr.mepSystem.isForPipework) {
                domainList ("Piping");
            }
            if (attr.mepSystem.isForCabling) {
                domainList ("CableCarrier");
            }
#endif
        }
        if (wantsField.Wants ("contourPen")) {
            mepSystem.Add ("contourPen", attr.mepSystem.contourPen);
        }
        if (wantsField.Wants ("fillPen")) {
            mepSystem.Add ("fillPen", attr.mepSystem.fillPen);
        }
        if (wantsField.Wants ("fillBackgroundPen")) {
            mepSystem.Add ("fillBackgroundPen", attr.mepSystem.fillBgPen);
        }
        if (wantsField.Wants ("centerLinePen")) {
            mepSystem.Add ("centerLinePen", attr.mepSystem.centerLinePen);
        }
        if (wantsField.Wants ("fillId")) {
            GS::Optional<GS::ObjectState> fillId = GetAttributeIdFromIndex (API_FilltypeID, attr.mepSystem.fillInd);
            if (fillId.HasValue ()) {
                mepSystem.Add ("fillId", *fillId);
            }
        }
        if (wantsField.Wants ("centerLineTypeId")) {
            GS::Optional<GS::ObjectState> lineTypeId = GetAttributeIdFromIndex (API_LinetypeID, attr.mepSystem.centerLTypeInd);
            if (lineTypeId.HasValue ()) {
                mepSystem.Add ("centerLineTypeId", *lineTypeId);
            }
        }
        if (wantsField.Wants ("materialId")) {
            GS::Optional<GS::ObjectState> materialId = GetAttributeIdFromIndex (API_MaterialID, attr.mepSystem.materialInd);
            if (materialId.HasValue ()) {
                mepSystem.Add ("materialId", *materialId);
            }
        }
        if (wantsField.Wants ("insulationMaterialId")) {
            GS::Optional<GS::ObjectState> insulationMaterialId = GetAttributeIdFromIndex (API_MaterialID, attr.mepSystem.insulationMatInd);
            if (insulationMaterialId.HasValue ()) {
                mepSystem.Add ("insulationMaterialId", *insulationMaterialId);
            }
        }

        mepSystems (mepSystem);
    }

    return response;
}

GetPenTablesCommand::GetPenTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetPenTablesCommand::GetName () const
{
    return "GetPenTables";
}

GS::Optional<GS::UniString> GetPenTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Pen Table. If omitted, every field is returned. Requesting only the fields you need avoids fetching the 255-element pens array.",
                "items": {
                    "$ref": "#/PenTableAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetPenTablesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "penTables": {
                "type": "array",
                "description": "A list of pen tables or errors.",
                "items": {
                    "$ref": "#/PenTableAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "penTables"
        ]
    })";
}

#ifdef ServerMainVers_2700
GS::ObjectState GetPenTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& penTables = response.AddList<GS::ObjectState> ("penTables");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_PenTableID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            penTables (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState penTable;
        penTable.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        penTable.Add ("index", GetAttributeIndex (attr.header.index));
        penTable.Add ("name", name);

        if (wantsField.Wants ("isActiveForModel")) {
            penTable.Add ("isActiveForModel", attr.penTable.inEffectForModel);
        }
        if (wantsField.Wants ("isActiveForLayout")) {
            penTable.Add ("isActiveForLayout", attr.penTable.inEffectForLayout);
        }
        if (wantsField.Wants ("pens")) {
            API_AttributeDefExt penTableDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_PenTableID, attr.header.index, &penTableDefs) == NoError && penTableDefs.penTable_Items != nullptr) {
                const auto& pensList = penTable.AddList<GS::ObjectState> ("pens");
                for (const API_Pen& pen : *penTableDefs.penTable_Items) {
                    pensList (GS::ObjectState (
                        "index", (Int32) pen.index,
                        "color", GS::ObjectState ("red", pen.rgb.f_red, "green", pen.rgb.f_green, "blue", pen.rgb.f_blue),
                        "width", pen.width,
                        "description", GS::UniString (pen.description)));
                }
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
            }
        }

        penTables (penTable);
    }

    return response;
}
#else
GS::ObjectState GetPenTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& penTables = response.AddList<GS::ObjectState> ("penTables");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_PenTableID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            penTables (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState penTable;
        penTable.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        penTable.Add ("index", GetAttributeIndex (attr.header.index));
        penTable.Add ("name", name);

        if (wantsField.Wants ("isActiveForModel")) {
            penTable.Add ("isActiveForModel", attr.penTable.inEffectForModel);
        }
        if (wantsField.Wants ("isActiveForLayout")) {
            penTable.Add ("isActiveForLayout", attr.penTable.inEffectForLayout);
        }
        if (wantsField.Wants ("pens")) {
            API_AttributeDefExt penTableDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_PenTableID, attr.header.index, &penTableDefs) == NoError && penTableDefs.penTable_Items != nullptr) {
                Int32 count = (Int32) (BMGetHandleSize ((GSHandle) penTableDefs.penTable_Items) / sizeof (API_PenType));
                const auto& pensList = penTable.AddList<GS::ObjectState> ("pens");
                for (Int32 i = 0; i < count; ++i) {
                    const API_PenType& pen = (*penTableDefs.penTable_Items)[i];
                    pensList (GS::ObjectState (
                        "index", (Int32) pen.head.index,
                        "color", GS::ObjectState ("red", pen.rgb.f_red, "green", pen.rgb.f_green, "blue", pen.rgb.f_blue),
                        "width", pen.width,
                        "description", GS::UniString (pen.description)));
                }
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
            }
        }

        penTables (penTable);
    }

    return response;
}
#endif

GetCurrentPenTableCommand::GetCurrentPenTableCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCurrentPenTableCommand::GetName () const
{
    return "GetCurrentPenTable";
}

GS::Optional<GS::UniString> GetCurrentPenTableCommand::GetInputParametersSchema () const
{
    return GS::NoValue;
}

GS::Optional<GS::UniString> GetCurrentPenTableCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "index": {
                "type": "integer",
                "description": "The attribute index of the current Pen Table, as returned by Archicad."
            },
            "isPenTableAttribute": {
                "type": "boolean",
                "description": "True if the index refers to an existing Pen Table attribute. When false, attributeId and name are omitted; the pens in effect can not be read with GetPenTables."
            },
            "attributeId": {
                "$ref": "#/AttributeId"
            },
            "name": {
                "type": "string",
                "description": "The name of the current Pen Table attribute."
            },
            "penCount": {
                "type": "integer",
                "description": "The number of pens in the current Pen Table. Omitted when it can not be read."
            }
        },
        "additionalProperties": false,
        "required": [
            "index",
            "isPenTableAttribute"
        ]
    })";
}

GS::ObjectState GetCurrentPenTableCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_AttributeIndex penSetIndex = APIInvalidAttributeIndex;
    GSErrCode err = ACAPI_Attribute_GetCurrPenSet (&penSetIndex);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current pen table.");
    }

    GS::ObjectState response;
    response.Add ("index", GetAttributeIndex (penSetIndex));

    GS::UniString name;
    API_Attribute attr = {};
    attr.header.typeID = API_PenTableID;
    attr.header.index = penSetIndex;
    attr.header.uniStringNamePtr = &name;
    const bool isPenTableAttribute = IsPositiveAttributeIndex (penSetIndex) && ACAPI_Attribute_Get (&attr) == NoError;
    response.Add ("isPenTableAttribute", isPenTableAttribute);
    if (isPenTableAttribute) {
        response.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        response.Add ("name", name);
    }

#ifdef ServerMainVers_2700
    UInt32 penCount = 0;
    if (ACAPI_Attribute_GetPenNum (penCount) == NoError) {
        response.Add ("penCount", (Int32) penCount);
    }
#else
    // ACAPI_Attribute_GetPenNum is available since AC27; before that the pens of the attribute are counted.
    if (isPenTableAttribute) {
        API_AttributeDefExt penTableDefs = {};
        if (ACAPI_Attribute_GetDefExt (API_PenTableID, penSetIndex, &penTableDefs) == NoError && penTableDefs.penTable_Items != nullptr) {
            response.Add ("penCount", (Int32) (BMGetHandleSize ((GSHandle) penTableDefs.penTable_Items) / sizeof (API_PenType)));
            ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
        }
    }
#endif

    return response;
}

GetProfilesCommand::GetProfilesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetProfilesCommand::GetName () const
{
    return "GetProfiles";
}

GS::Optional<GS::UniString> GetProfilesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Profile. If omitted, every field is returned. Note the raw cross-section vector geometry itself is not exposed; width/height/minimumWidth/minimumHeight/widthStretchable/heightStretchable/hasCoreSkin/profileModifiers are derived measurements matching what the Profile Editor shows, computed from the profile's internal stretch/parameter data.",
                "items": {
                    "$ref": "#/ProfileAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetProfilesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "profiles": {
                "type": "array",
                "description": "A list of profiles or errors.",
                "items": {
                    "$ref": "#/ProfileAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "profiles"
        ]
    })";
}

// Perpendicular projection of refPoint onto the infinite line through edgeP1/edgeP2 (not clamped to the
// segment) - an edge-offset dimension measures distance to the skin boundary's line, not to the edge's
// endpoints, so this - not the edge's midpoint - is the correct resolved position for an edge-associative
// anchor.
static Coord ProjectPointOntoLine (const Coord& refPoint, const Coord& edgeP1, const Coord& edgeP2)
{
    double vx = edgeP2.x - edgeP1.x;
    double vy = edgeP2.y - edgeP1.y;
    double lenSq = vx * vx + vy * vy;
    if (lenSq < 1e-12) {
        return edgeP1;
    }
    double t = ((refPoint.x - edgeP1.x) * vx + (refPoint.y - edgeP1.y) * vy) / lenSq;
    Coord result;
    result.x = edgeP1.x + t * vx;
    result.y = edgeP1.y + t * vy;
    return result;
}

static bool GetHatchEdgeSegment (const PVI::HatchEdgeId& edgeId, const GS::HashTable<GS::Guid, HatchObject>& hatches, Coord& outP1, Coord& outP2)
{
    const HatchObject* hatch = nullptr;
    if (!hatches.Get (edgeId.GetHatchId (), &hatch) || hatch == nullptr) {
        return false;
    }
    const GS::Array<Coord>& coords = hatch->GetCoords ();
    USize coordCount = coords.GetSize ();
    UIndex edgeIndex = edgeId.GetEdgeIndex ();
    if (coordCount < 2 || edgeIndex >= coordCount) {
        return false;
    }
    outP1 = coords[edgeIndex];
    outP2 = coords[(edgeIndex + 1) % coordCount];
    return true;
}

// Resolves a profile anchor to a 2D position: a fixed canvas point, a point tied to an existing hatch
// vertex, or - for an edge-associative anchor - the perpendicular projection of projectionReference onto the
// associated edge's line (projectionReference is normally the position of the OTHER anchor in the same
// dimension, so this must be resolved after the other anchor). Bounding-box/stretch-zone anchor types are
// not resolved; callers treat an unresolved anchor as "value unavailable" rather than guessing.
static GS::Optional<Coord> GetProfileAnchorPosition (const ProfileVectorImage& pvi, const PVI::ProfileAnchorId& anchorId, const GS::HashTable<GS::Guid, HatchObject>& hatches, const GS::Optional<Coord>& projectionReference = GS::NoValue)
{
    GS::Optional<PVI::Anchor> anchor = pvi.GetAnchor (anchorId);
    if (!anchor.HasValue ()) {
        return GS::NoValue;
    }
    if (anchor->GetAnchorType () == PVI::Anchor::AnchorType::FixedToStretchCanvas) {
        return anchor->GetFixAnchorPosition ();
    }
    if (anchor->GetAnchorType () == PVI::Anchor::AnchorType::VertexAssociative) {
        const PVI::HatchVertexId& vertexId = anchor->GetAssociatedVertexId ();
        const HatchObject* hatch = nullptr;
        if (!hatches.Get (vertexId.GetHatchId (), &hatch) || hatch == nullptr) {
            return GS::NoValue;
        }
        const GS::Array<Coord>& coords = hatch->GetCoords ();
        UIndex vertexIndex = vertexId.GetVertexIndex ();
        if (vertexIndex >= coords.GetSize ()) {
            return GS::NoValue;
        }
        return coords[vertexIndex];
    }
    if (anchor->GetAnchorType () == PVI::Anchor::AnchorType::EdgeAssociative) {
        Coord p1, p2;
        if (!GetHatchEdgeSegment (anchor->GetAssociatedEdgeId (), hatches, p1, p2)) {
            return GS::NoValue;
        }
        if (projectionReference.HasValue ()) {
            return ProjectPointOntoLine (*projectionReference, p1, p2);
        }
        Coord mid;
        mid.x = (p1.x + p2.x) / 2.0;
        mid.y = (p1.y + p2.y) / 2.0;
        return mid;
    }
    return GS::NoValue;
}

// Computes the current value of an edge-offset profile parameter (a "profile modifier" in the official
// Archicad API) by resolving its dimension control tool's two anchors to real coordinates and measuring the
// distance between them - the same geometry the Profile Editor itself measures. (The parameter's
// user-authored name comes separately, from the profile_vectorImageParameterNames table in
// API_AttributeDefExt - see the caller.)
// Verified to exactly match ACAPI_GetProfileAttributes' own profileModifiers values across all 48 profiles
// in a real project (zero mismatches, including hasCoreSkin/width/height/minimum*/​*Stretchable too).
// Before AC27, HatchObject exposes profile skin/edge data via raw pointers (GetProfileItemPtr(),
// GetProfileEdgeData(UIndex) one at a time) instead of AC27+'s GS::Optional<HatchProfileData>-backed
// HasCoreProfileItem()/GetProfileItem()/GetProfileEdgeData() (whole vector); ProfileEdgeData/ProfileItem's
// material/lineType/cutEndLineType fields are also plain GSAttributeIndex pre-AC27 rather than
// ADB::AttributeIndex (which does not exist as a type before AC27 at all). This helper bridges both shapes
// so the calling code stays version-agnostic.
#ifdef ServerMainVers_2700
static Int32 ToPlainAttrIndex (const ADB::AttributeIndex& idx)
{
    // Renamed between AC27 and AC28 (ToGSAttributeIndex () -> ToGSAttributeIndex_Deprecated ()) - confirmed via
    // CI failure that AC28 already has AC29's shape, not AC27's; this is not a ServerMainVers_2900-only change.
#ifdef ServerMainVers_2800
    return idx.ToGSAttributeIndex_Deprecated ();
#else
    return idx.ToGSAttributeIndex ();
#endif
}
#else
static Int32 ToPlainAttrIndex (GSAttributeIndex idx)
{
    return (Int32) idx;
}
#endif

// Reverse of ToPlainAttrIndex, for writing a resolved attribute index back into a HatchObject/ProfileItem/
// ProfileEdgeData setter. AC27's AttributeIndex(GSAttributeIndex) constructor is public; AC28 made the
// equivalent constructor private and requires going through the CreateAttributeIndex() friend function (same
// CI-confirmed AC28, not AC29, boundary as ToPlainAttrIndex above).
#ifdef ServerMainVers_2700
static ADB::AttributeIndex FromPlainAttrIndex (Int32 idx)
{
#ifdef ServerMainVers_2800
    return ADB::CreateAttributeIndex (idx);
#else
    return ADB::AttributeIndex ((GSAttributeIndex) idx);
#endif
}
#else
static GSAttributeIndex FromPlainAttrIndex (Int32 idx)
{
    return (GSAttributeIndex) idx;
}
#endif

#ifdef ServerMainVers_2700
static bool HatchHasCoreProfileItem (const HatchObject& hatch) { return hatch.HasCoreProfileItem (); }
static bool HatchHasProfileInfo (const HatchObject& hatch) { return hatch.HasProfileInfo (); }
static const ProfileItem& GetHatchProfileItem (const HatchObject& hatch) { return hatch.GetProfileItem (); }
static ProfileItem* GetHatchProfileItemNonConst (HatchObject& hatch) { return hatch.HasProfileInfo () ? &hatch.GetProfileItemNonConst () : nullptr; }
#else
static bool HatchHasCoreProfileItem (const HatchObject& hatch) { const ProfileItem* item = hatch.GetProfileItemPtr (); return item != nullptr && item->IsCore (); }
static bool HatchHasProfileInfo (const HatchObject& hatch) { return hatch.GetProfileItemPtr () != nullptr; }
static const ProfileItem& GetHatchProfileItem (const HatchObject& hatch) { return *hatch.GetProfileItemPtr (); }
static ProfileItem* GetHatchProfileItemNonConst (HatchObject& hatch) { return hatch.GetProfileItemPtr (); }
#endif

// Applies caller-supplied overrides (from CreateProfiles' skinOverrides) to one skin of a profile's vector
// image, matched by the skinId GetProfiles reports. Mutation goes through Archicad's own HatchObject/
// ProfileItem setters - the same objects GetProfiles reads via their const getters - so this genuinely
// writes back into the profile, not just into our own copy of the data.
static void ApplyProfileSkinOverrides (ProfileVectorImage& pvi, const GS::ObjectState& override)
{
    GS::UniString skinIdStr;
    if (!override.Get ("skinId", skinIdStr)) {
        return;
    }
    GS::Guid skinId (skinIdStr);

    VectorImageAccessGuard guard (&pvi);
    VectorImage& vi = guard;
    for (VectorImageIterator it (vi); !it.IsEOI (); ++it) {
        if (it->item_Typ != SyHatch) {
            continue;
        }
        Sy_HatchType* hatchRec = it;
        HatchObject& hatch = pvi.GetHatchObject (*hatchRec);
        if (hatch.GetUniqueId () != skinId) {
            continue;
        }

        // buildMatFlags selects which of buildMatIdx/surfaceIdx actually gets rendered
        // (SyHatchFlag_BuildingMaterialHatch vs _SurfaceHatch are mutually exclusive modes) -
        // explicitly overriding one must switch the mode to match, or the new value is silently
        // ignored (Archicad keeps rendering with whichever mode was already active). See the
        // matching note in BuildHatchFromSkinDefinition for how this was found.
        GS::ObjectState buildingMaterialId;
        if (override.Get ("buildingMaterialId", buildingMaterialId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
                hatch.SetBuildMatIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
                hatch.buildMatFlags = (hatch.buildMatFlags | SyHatchFlag_BuildingMaterialHatch) & ~SyHatchFlag_SurfaceHatch;
            }
        }
        GS::ObjectState surfaceId;
        if (override.Get ("surfaceId", surfaceId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (surfaceId, API_MaterialID, idx)) {
                hatch.SetSurfaceIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
                hatch.buildMatFlags = (hatch.buildMatFlags | SyHatchFlag_SurfaceHatch) & ~SyHatchFlag_BuildingMaterialHatch;
            }
        }
        GS::ObjectState fillId;
        if (override.Get ("fillId", fillId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, idx)) {
                hatch.SetFillIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
                hatch.buildMatFlags |= SyHatchFlag_FillHatch;
            }
        }
        Int32 contourPen;
        if (override.Get ("contourPen", contourPen)) {
            hatch.SetContPen (VBAttr::ExtendedPen ((short) contourPen));
        }
        GS::ObjectState contourLineTypeId;
        if (override.Get ("contourLineTypeId", contourLineTypeId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (contourLineTypeId, API_LinetypeID, idx)) {
                hatch.SetContLType (FromPlainAttrIndex (GetAttributeIndex (idx)));
            }
        }

        ProfileItem* profileItem = GetHatchProfileItemNonConst (hatch);
        if (profileItem != nullptr) {
            bool isCore;
            if (override.Get ("isCore", isCore)) {
                profileItem->SetCore (isCore);
            }
            bool isFinish;
            if (override.Get ("isFinish", isFinish)) {
                profileItem->SetFinish (isFinish);
            }
            bool visibleCutEndLines;
            if (override.Get ("visibleCutEndLines", visibleCutEndLines)) {
                profileItem->SetVisibleCutEndLines (visibleCutEndLines);
            }
            Int32 cutEndLinePen;
            if (override.Get ("cutEndLinePen", cutEndLinePen)) {
                profileItem->SetCutEndLinePen ((short) cutEndLinePen);
            }
            GS::ObjectState cutEndLineTypeId;
            if (override.Get ("cutEndLineTypeId", cutEndLineTypeId)) {
                API_AttributeIndex idx;
                if (GetAttributeIndexFromAttributeId (cutEndLineTypeId, API_LinetypeID, idx)) {
                    profileItem->SetCutEndLineType (FromPlainAttrIndex (GetAttributeIndex (idx)));
                }
            }
        }

        GS::Array<GS::ObjectState> edgeOverrides;
        if (override.Get ("edgeOverrides", edgeOverrides)) {
            for (const GS::ObjectState& edgeOverride : edgeOverrides) {
                Int32 edgeIndex = -1;
                if (!edgeOverride.Get ("edgeIndex", edgeIndex) || edgeIndex < 0) {
                    continue;
                }
                ProfileEdgeData* edgeData = hatch.GetProfileEdgeData ((UIndex) edgeIndex);
                if (edgeData == nullptr) {
                    continue;
                }
                Int32 pen;
                if (edgeOverride.Get ("pen", pen)) {
                    edgeData->SetPen ((short) pen);
                }
                bool isVisibleLine;
                if (edgeOverride.Get ("isVisibleLine", isVisibleLine)) {
                    edgeData->SetVisibleLine (isVisibleLine);
                }
                GS::ObjectState lineTypeId;
                if (edgeOverride.Get ("lineTypeId", lineTypeId)) {
                    API_AttributeIndex idx;
                    if (GetAttributeIndexFromAttributeId (lineTypeId, API_LinetypeID, idx)) {
                        edgeData->SetLineType (FromPlainAttrIndex (GetAttributeIndex (idx)));
                    }
                }
                GS::ObjectState buildingMaterialId;
                if (edgeOverride.Get ("buildingMaterialId", buildingMaterialId)) {
                    API_AttributeIndex idx;
                    if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
                        edgeData->SetMaterial (FromPlainAttrIndex (GetAttributeIndex (idx)));
                        edgeData->SetSurfaceFromBuildMat (true);
                    }
                }
            }
        }

        return;
    }
}

// Converts caller-supplied polygon contours (same 0-based polygonCoordinates/polygonArcs convention
// used by e.g. CreateSlabs) into the coords/arcs/subPolyEnds triple a HatchObject's polygon geometry
// needs, hiding the internal reserved-slot conventions from callers:
//   - coords[0] is always a reserved anchor at (0,0), forming its own degenerate single-point
//     "subpoly" - never one of the polygon's real vertices (confirmed by inspecting every real skin's
//     outlineCoords via GetProfiles: coords[0] == (0,0) and subPolyEnds[0] == 0 without exception).
//   - each contour must close by repeating its own first vertex once more at the end.
//   - the first contour is the outer boundary; any further contours are holes, exactly like the
//     public API's polygon+holes convention (e.g. Hole2D) - this module has no separate "hole" concept,
//     a hatch's shape is simply however many closed subpolys its coords/subPolyEnds describe.
// arcs' begIndex/endIndex are taken as 0-based within their own contour's polygonCoordinates (matching
// the public API's PolyArc convention) and rebased onto the absolute coords array here.
#ifdef ServerMainVers_2700
static void BuildHatchPolygonGeometry (const GS::Array<GS::ObjectState>& contours, GS::Array<Coord>& outCoords, GS::Array<PolyArcRec>& outArcs, GS::Array<UInt32>& outSubPolyEnds)
{
    auto makeCoord = [] (double x, double y) { Coord c = {}; c.x = x; c.y = y; return c; };

    outCoords.Push (makeCoord (0.0, 0.0));  // [0] reserved anchor
    outSubPolyEnds.Push (0);

    for (const GS::ObjectState& contour : contours) {
        GS::Array<GS::ObjectState> polygonCoordinates;
        contour.Get ("polygonCoordinates", polygonCoordinates);
        if (polygonCoordinates.GetSize () < 3) {
            continue;
        }

        UIndex iStart = (UIndex) outCoords.GetSize ();
        for (const GS::ObjectState& coordOs : polygonCoordinates) {
            API_Coord c = Get2DCoordinateFromObjectState (coordOs);
            outCoords.Push (makeCoord (c.x, c.y));
        }
        outCoords.Push (outCoords[iStart]);  // close the contour by repeating its first vertex
        outSubPolyEnds.Push ((UInt32) (outCoords.GetSize () - 1));

        GS::Array<GS::ObjectState> polygonArcs;
        if (contour.Get ("polygonArcs", polygonArcs)) {
            for (const GS::ObjectState& arcOs : polygonArcs) {
                Int32 begIndex = 0, endIndex = 0;
                double arcAngle = 0.0;
                if (arcOs.Get ("begIndex", begIndex) && arcOs.Get ("endIndex", endIndex) && arcOs.Get ("arcAngle", arcAngle)) {
                    outArcs.Push (PolyArcRec (iStart + (UIndex) begIndex, iStart + (UIndex) endIndex, arcAngle));
                }
            }
        }
    }
}

// Builds a brand-new HatchObject (profile skin) from a caller-supplied skin definition (CreateProfiles'
// "newSkins" items): arbitrary polygon geometry (via BuildHatchPolygonGeometry) plus the same
// material/pen/core/cut-end-line/edge properties ApplyProfileSkinOverrides can already mutate on an
// existing skin. Returns false (leaving outHatch untouched) if the skin definition has no usable
// geometry (fewer than 3 real vertices in its first contour).
//
// CRASH INVESTIGATION (2026-07-12): a live crash was found when writing an edgeOverride at index
// coords.GetSize()-1 with the edge vector sized to coords.GetSize(). Reading real Archicad-authored
// profiles back via GetProfiles proves edges.size() == coords.GetSize() IS the correct vector size
// (confirmed live against 4 real skins: HatchEdgeId's own coords[i] -> coords[(i+1) % coords.GetSize()]
// indexing, used for profile-modifier anchors, treats index 0 and index coords.GetSize()-1 as
// legitimate slots - edges bridging the reserved anchor to/from the real polygon). An earlier "fix"
// that shrank the vector by contours.GetSize() only avoided the crash by making the crashing index
// invalid (silently skipped) - not a root-cause fix, just an accidental workaround.
//
// The crash itself did NOT reproduce on retry with the identical payload/formula (confirmed live:
// same real-skin data, same coords.GetSize()-sized vector, same edgeIndex accessed - no crash the
// second time), meaning it is very likely a genuine Archicad-side memory-safety bug (heap-dependent
// UB), not a deterministic logic error reachable through this add-on's own code. Since we cannot fix
// Archicad's own compiled internals, and since index 0 and index coords.GetSize()-1 are provably
// never part of the rendered geometry anyway (subPolyEnds[0] == 0 marks a degenerate zero-vertex
// "anchor sub-polygon", distinct from the real rendered contour that follows - confirmed live: these
// two slots' pen/visibility on real profiles are just inherited defaults, never a deliberate user
// edit, since they have no visible effect to edit), the safe and honest choice is to keep the
// topologically-correct vector size but simply never let edgeOverrides write to these two
// structurally-inert bridge slots - not a silent-data-loss workaround, since nothing user-visible is
// ever lost by skipping them.
static bool BuildHatchFromSkinDefinition (const GS::ObjectState& skinDef, HatchObject& outHatch)
{
    GS::Array<GS::ObjectState> contours;
    if (!skinDef.Get ("contours", contours) || contours.IsEmpty ()) {
        return false;
    }

    GS::Array<Coord> coords;
    GS::Array<PolyArcRec> arcs;
    GS::Array<UInt32> subPolyEnds;
    BuildHatchPolygonGeometry (contours, coords, arcs, subPolyEnds);
    if (coords.GetSize () < 5) {  // reserved anchor + at least 3 real vertices + 1 closing duplicate
        return false;
    }
    outHatch.SetPolygonGeometry (coords, arcs, subPolyEnds);

    // buildMatFlags is a bitmask selecting which of buildMatIdx/surfaceIdx/fillIdx Archicad actually
    // renders with (SyHatchFlag_BuildingMaterialHatch / _SurfaceHatch / _FillHatch) - setting the
    // *Idx value alone is not enough, without the matching flag Archicad silently ignores it and
    // falls back to a default (confirmed live: a skin whose source had a fixed Surface override
    // came back as the generic building-material-driven surface after sync, because this was
    // unconditionally hardcoded to BuildingMaterialHatch regardless of which field was actually
    // present).
    unsigned short newBuildMatFlags = 0;
    GS::ObjectState buildingMaterialId;
    if (skinDef.Get ("buildingMaterialId", buildingMaterialId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
            outHatch.SetBuildMatIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
            newBuildMatFlags |= SyHatchFlag_BuildingMaterialHatch;
        }
    }
    GS::ObjectState surfaceId;
    if (skinDef.Get ("surfaceId", surfaceId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (surfaceId, API_MaterialID, idx)) {
            outHatch.SetSurfaceIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
            newBuildMatFlags |= SyHatchFlag_SurfaceHatch;
        }
    }
    GS::ObjectState fillId;
    if (skinDef.Get ("fillId", fillId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, idx)) {
            outHatch.SetFillIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
            newBuildMatFlags |= SyHatchFlag_FillHatch;
        }
    }
    if (newBuildMatFlags == 0) {
        newBuildMatFlags = SyHatchFlag_BuildingMaterialHatch;  // fallback: skin specified neither, keep the previous default
    }
    outHatch.buildMatFlags = newBuildMatFlags;
    Int32 contourPen = 1;
    skinDef.Get ("contourPen", contourPen);
    outHatch.SetContPen (VBAttr::ExtendedPen ((short) contourPen));
    outHatch.SetContVis (true);
    GS::ObjectState contourLineTypeId;
    if (skinDef.Get ("contourLineTypeId", contourLineTypeId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (contourLineTypeId, API_LinetypeID, idx)) {
            outHatch.SetContLType (FromPlainAttrIndex (GetAttributeIndex (idx)));
        }
    }

    bool isCore = false;
    skinDef.Get ("isCore", isCore);
    bool isFinish = !isCore;
    skinDef.Get ("isFinish", isFinish);
    bool visibleCutEndLines = true;
    skinDef.Get ("visibleCutEndLines", visibleCutEndLines);

    ProfileItem profileItem;
    profileItem.SetCore (isCore);
    profileItem.SetFinish (isFinish);
    profileItem.SetVisibleCutEndLines (visibleCutEndLines);
    Int32 cutEndLinePen;
    if (skinDef.Get ("cutEndLinePen", cutEndLinePen)) {
        profileItem.SetCutEndLinePen ((short) cutEndLinePen);
    }
    GS::ObjectState cutEndLineTypeId;
    if (skinDef.Get ("cutEndLineTypeId", cutEndLineTypeId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (cutEndLineTypeId, API_LinetypeID, idx)) {
            profileItem.SetCutEndLineType (FromPlainAttrIndex (GetAttributeIndex (idx)));
        }
    }

    HatchProfileData profileData;
    profileData.SetProfileItem (profileItem);
    // One edge slot per coords entry (see investigation note above) - matches what real
    // Archicad-authored profiles report via GetProfiles.
    USize edgeCount = coords.GetSize ();
    std::vector<ProfileEdgeData> edges (edgeCount);
    for (ProfileEdgeData& edge : edges) {
        edge.SetVisibleLine (true);
    }
    profileData.SetProfileEdgeData (edges);
    outHatch.SetProfileData (std::move (profileData));

    GS::Array<GS::ObjectState> edgeOverrides;
    if (skinDef.Get ("edgeOverrides", edgeOverrides)) {
        for (const GS::ObjectState& edgeOverride : edgeOverrides) {
            Int32 edgeIndex = -1;
            if (!edgeOverride.Get ("edgeIndex", edgeIndex) || edgeIndex < 0) {
                continue;
            }
            // Index 0 and edgeCount-1 are the anchor-bridge slots (see crash investigation note
            // above) - never part of the rendered geometry, and writing to them has demonstrated a
            // non-deterministic Archicad-side crash. Skip them rather than risk it.
            if ((UIndex) edgeIndex == 0 || (UIndex) edgeIndex == edgeCount - 1) {
                continue;
            }
            ProfileEdgeData* edgeData = outHatch.GetProfileEdgeData ((UIndex) edgeIndex);
            if (edgeData == nullptr) {
                continue;
            }
            Int32 pen;
            if (edgeOverride.Get ("pen", pen)) {
                edgeData->SetPen ((short) pen);
            }
            bool isVisibleLine;
            if (edgeOverride.Get ("isVisibleLine", isVisibleLine)) {
                edgeData->SetVisibleLine (isVisibleLine);
            }
            GS::ObjectState lineTypeId;
            if (edgeOverride.Get ("lineTypeId", lineTypeId)) {
                API_AttributeIndex idx;
                if (GetAttributeIndexFromAttributeId (lineTypeId, API_LinetypeID, idx)) {
                    edgeData->SetLineType (FromPlainAttrIndex (GetAttributeIndex (idx)));
                }
            }
            GS::ObjectState buildingMaterialId;
            if (edgeOverride.Get ("buildingMaterialId", buildingMaterialId)) {
                API_AttributeIndex idx;
                if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
                    edgeData->SetMaterial (FromPlainAttrIndex (GetAttributeIndex (idx)));
                    edgeData->SetSurfaceFromBuildMat (true);
                }
            }
        }
    }

    return true;
}
#endif

template <typename Func>
static void ForEachProfileEdge (const HatchObject& hatch, Func&& func)
{
#ifdef ServerMainVers_2700
    for (const ProfileEdgeData& edgeData : hatch.GetProfileEdgeData ()) {
        func (edgeData);
    }
#else
    UIndex edgeCount = hatch.GetCoords ().GetSize ();
    for (UIndex i = 0; i < edgeCount; ++i) {
        const ProfileEdgeData* edgeData = hatch.GetProfileEdgeData (i);
        if (edgeData != nullptr) {
            func (*edgeData);
        }
    }
#endif
}

static GS::Optional<double> GetProfileModifierValue (const ProfileVectorImage& pvi, const PVI::ProfileVectorImageParameter& param, const GS::HashTable<GS::Guid, HatchObject>& hatches)
{
    if (param.GetType () != PVI::ProfileVectorImageParameter::ParameterType::EdgeOffset) {
        return GS::NoValue;
    }
    const PVI::EdgeOffsetParameter& edgeParam = param.GetEdgeOffsetParameter ();
    GS::Optional<PVI::DimensionControlTool> dimTool = pvi.GetDimensionControlTool (edgeParam.GetDimControlToolID ());
    if (!dimTool.HasValue ()) {
        return GS::NoValue;
    }
    GS::Optional<PVI::Anchor> begAnchor = pvi.GetAnchor (dimTool->GetBegAnchorID ());
    GS::Optional<PVI::Anchor> endAnchor = pvi.GetAnchor (dimTool->GetEndAnchorID ());

    // Resolve whichever anchor is NOT edge-associative first, then resolve the edge-associative one (if any)
    // by projecting the already-resolved anchor onto that edge's line - an edge anchor's position is only
    // meaningful relative to where it is being measured from.
    bool begIsEdge = begAnchor.HasValue () && begAnchor->GetAnchorType () == PVI::Anchor::AnchorType::EdgeAssociative;
    bool endIsEdge = endAnchor.HasValue () && endAnchor->GetAnchorType () == PVI::Anchor::AnchorType::EdgeAssociative;

    GS::Optional<Coord> begPos, endPos;
    if (begIsEdge && !endIsEdge) {
        endPos = GetProfileAnchorPosition (pvi, dimTool->GetEndAnchorID (), hatches);
        if (endPos.HasValue ()) {
            begPos = GetProfileAnchorPosition (pvi, dimTool->GetBegAnchorID (), hatches, endPos);
        }
    } else if (endIsEdge && !begIsEdge) {
        begPos = GetProfileAnchorPosition (pvi, dimTool->GetBegAnchorID (), hatches);
        if (begPos.HasValue ()) {
            endPos = GetProfileAnchorPosition (pvi, dimTool->GetEndAnchorID (), hatches, begPos);
        }
    } else {
        begPos = GetProfileAnchorPosition (pvi, dimTool->GetBegAnchorID (), hatches);
        endPos = GetProfileAnchorPosition (pvi, dimTool->GetEndAnchorID (), hatches);
    }
    if (!begPos.HasValue () || !endPos.HasValue ()) {
        return GS::NoValue;
    }
    double dx = endPos->x - begPos->x;
    double dy = endPos->y - begPos->y;

    if (dimTool->IsEuclideanType ()) {
        return sqrt (dx * dx + dy * dy);
    }
    if (dimTool->IsProjectedToNominalDirType ()) {
        // "Nominal direction" is not itself stored on the tool; the profile's stretch-driven offset
        // dimensions always run along whichever axis has the larger displacement between the two anchors,
        // so that dominant axis is used as the projection direction.
        return (fabs (dx) >= fabs (dy)) ? fabs (dx) : fabs (dy);
    }
    if (dimTool->IsProjectedToAngleDirType ()) {
        double angle = dimTool->GetProjAngleRad ();
        return fabs (dx * cos (angle) + dy * sin (angle));
    }
    return GS::NoValue;
}

GS::ObjectState GetProfilesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& profiles = response.AddList<GS::ObjectState> ("profiles");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_ProfileID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            profiles (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState profile;
        profile.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        profile.Add ("index", GetAttributeIndex (attr.header.index));
        profile.Add ("name", name);

        if (wantsField.Wants ("wallType")) {
            profile.Add ("wallType", attr.profile.wallType);
        }
        if (wantsField.Wants ("beamType")) {
            profile.Add ("beamType", attr.profile.beamType);
        }
        if (wantsField.Wants ("coluType")) {
            profile.Add ("coluType", attr.profile.coluType);
        }
        if (wantsField.Wants ("handrailType")) {
            profile.Add ("handrailType", attr.profile.handrailType);
        }
        if (wantsField.Wants ("otherGDLObjectType")) {
            profile.Add ("otherGDLObjectType", attr.profile.otherGDLObjectType);
        }

        if (wantsField.Wants ("useWith")) {
            const auto& useWithList = profile.AddList<GS::UniString> ("useWith");
            if (attr.profile.wallType) {
                useWithList ("Wall");
            }
            if (attr.profile.beamType) {
                useWithList ("Beam");
            }
            if (attr.profile.coluType) {
                useWithList ("Column");
            }
            if (attr.profile.handrailType) {
                useWithList ("Handrail");
            }
            if (attr.profile.otherGDLObjectType) {
                useWithList ("Other");
            }
        }

        bool wantsGeometryDerivedFields = wantsField.Wants ("width") || wantsField.Wants ("height") ||
            wantsField.Wants ("minimumWidth") || wantsField.Wants ("minimumHeight") ||
            wantsField.Wants ("widthStretchable") || wantsField.Wants ("heightStretchable") ||
            wantsField.Wants ("hasCoreSkin") || wantsField.Wants ("profileModifiers") || wantsField.Wants ("skins");
        if (wantsGeometryDerivedFields) {
            API_AttributeDefExt profileDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_ProfileID, attr.header.index, &profileDefs) == NoError && profileDefs.profile_vectorImageItems != nullptr) {
                const ProfileVectorImage& pvi = *profileDefs.profile_vectorImageItems;

                Box2DData buildingBounds = pvi.GetBuildingBounds ();
                double width = buildingBounds.xMax - buildingBounds.xMin;
                double height = buildingBounds.yMax - buildingBounds.yMin;
                const PVI::StretchData& stretch = pvi.GetStretchData ();

                if (wantsField.Wants ("width")) {
                    profile.Add ("width", width);
                }
                if (wantsField.Wants ("height")) {
                    profile.Add ("height", height);
                }
                if (wantsField.Wants ("minimumWidth")) {
                    profile.Add ("minimumWidth", width - stretch.GetStretchZoneWidth ());
                }
                if (wantsField.Wants ("minimumHeight")) {
                    profile.Add ("minimumHeight", height - stretch.GetStretchZoneHeight ());
                }
                if (wantsField.Wants ("widthStretchable")) {
                    profile.Add ("widthStretchable", stretch.HasHorizontalZone ());
                }
                if (wantsField.Wants ("heightStretchable")) {
                    profile.Add ("heightStretchable", stretch.HasVerticalZone ());
                }

                if (wantsField.Wants ("hasCoreSkin") || wantsField.Wants ("profileModifiers") || wantsField.Wants ("skins")) {
                    const GS::HashTable<GS::Guid, HatchObject>& hatches = pvi.GetConstHatchObjects ();

                    if (wantsField.Wants ("hasCoreSkin")) {
                        bool hasCoreSkin = false;
                        hatches.EnumerateValuesConst ([&] (const HatchObject& hatch) {
                            if (HatchHasCoreProfileItem (hatch)) {
                                hasCoreSkin = true;
                            }
                        });
                        profile.Add ("hasCoreSkin", hasCoreSkin);
                    }

                    if (wantsField.Wants ("profileModifiers")) {
                        const auto& modifiersList = profile.AddList<GS::ObjectState> ("profileModifiers");
                        const auto& paramTable = pvi.GetParameterTable ();
                        // Parameter names are user-authored free text (assigned when the profile's author draws
                        // each dimension in the Profile Editor), not derived from any other profile data - they
                        // live in the sibling profile_vectorImageParameterNames table, keyed by the same
                        // ProfileParameterId as GetParameterTable().
                        paramTable.Enumerate ([&] (const PVI::ProfileParameterId& paramId, const GS::HashSet<PVI::ProfileParameterSetupId>& setupIds) {
                            if (setupIds.IsEmpty ()) {
                                return;
                            }
                            // Every setup in the set is a per-skin-instance variant of the same logical parameter;
                            // any one of them yields the parameter's current (shared) value.
                            PVI::ProfileParameterSetupId representativeSetupId = *setupIds.Begin ();
                            const PVI::ProfileVectorImageParameter& param = pvi.GetParameterDef (representativeSetupId);
                            GS::Optional<double> value = GetProfileModifierValue (pvi, param, hatches);
                            GS::ObjectState modifier;
                            if (profileDefs.profile_vectorImageParameterNames != nullptr) {
                                const GS::UniString* name = nullptr;
                                if (profileDefs.profile_vectorImageParameterNames->Get (paramId, &name) && name != nullptr) {
                                    modifier.Add ("name", *name);
                                }
                            }
                            if (value.HasValue ()) {
                                modifier.Add ("value", *value);
                            }
                            modifiersList (modifier);
                        });
                    }

                    if (wantsField.Wants ("skins")) {
                        const auto& skinsList = profile.AddList<GS::ObjectState> ("skins");
                        hatches.EnumerateValuesConst ([&] (const HatchObject& hatch) {
                            GS::ObjectState skin;

                            skin.Add ("skinId", hatch.GetUniqueId ().ToUniString ());

                            GS::Optional<GS::ObjectState> buildingMaterialId = GetAttributeIdFromIndex (API_BuildingMaterialID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (hatch.GetBuildMatIdx ())));
                            if (buildingMaterialId.HasValue ()) {
                                skin.Add ("buildingMaterialId", *buildingMaterialId);
                            }
                            GS::Optional<GS::ObjectState> surfaceId = GetAttributeIdFromIndex (API_MaterialID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (hatch.GetSurfaceIdx ())));
                            if (surfaceId.HasValue ()) {
                                skin.Add ("surfaceId", *surfaceId);
                            }
                            GS::Optional<GS::ObjectState> fillId = GetAttributeIdFromIndex (API_FilltypeID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (hatch.GetFillIdx ())));
                            if (fillId.HasValue ()) {
                                skin.Add ("fillId", *fillId);
                            }
                            skin.Add ("contourPen", (Int32) hatch.GetContPenVal ().GetIndex ());
                            GS::Optional<GS::ObjectState> contourLineTypeId = GetAttributeIdFromIndex (API_LinetypeID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (hatch.GetContLType ())));
                            if (contourLineTypeId.HasValue ()) {
                                skin.Add ("contourLineTypeId", *contourLineTypeId);
                            }

                            if (wantsField.Wants ("skinOutlines")) {
                                const auto& coordsList = skin.AddList<GS::ObjectState> ("outlineCoords");
                                for (const Coord& c : hatch.GetCoords ()) {
                                    coordsList (GS::ObjectState ("x", c.x, "y", c.y));
                                }
                                const auto& subPolyEndsList = skin.AddList<Int32> ("outlineSubPolyEnds");
                                for (UInt32 end : hatch.GetSubPolyEnds ()) {
                                    subPolyEndsList ((Int32) end);
                                }
                                const auto& arcsList = skin.AddList<GS::ObjectState> ("outlineArcs");
                                for (const PolyArcRec& arc : hatch.GetArcs ()) {
                                    arcsList (GS::ObjectState ("begIndex", (Int32) arc.begIndex, "endIndex", (Int32) arc.endIndex, "arcAngle", arc.arcAngle));
                                }
                            }

                            if (HatchHasProfileInfo (hatch)) {
                                const ProfileItem& profileItem = GetHatchProfileItem (hatch);
                                skin.Add ("isCore", profileItem.IsCore ());
                                skin.Add ("isFinish", profileItem.IsFinish ());
                                skin.Add ("visibleCutEndLines", profileItem.IsVisibleCutEndLines ());
                                skin.Add ("cutEndLinePen", (Int32) profileItem.GetCutEndLinePen ());
                                GS::Optional<GS::ObjectState> cutEndLineTypeId = GetAttributeIdFromIndex (API_LinetypeID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (profileItem.GetCutEndLineType ())));
                                if (cutEndLineTypeId.HasValue ()) {
                                    skin.Add ("cutEndLineTypeId", *cutEndLineTypeId);
                                }

                                const auto& edgesList = skin.AddList<GS::ObjectState> ("edges");
                                ForEachProfileEdge (hatch, [&] (const ProfileEdgeData& edgeData) {
                                    GS::ObjectState edge;
                                    GS::Optional<GS::ObjectState> edgeMaterialId = GetAttributeIdFromIndex (API_BuildingMaterialID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (edgeData.GetMaterial ())));
                                    if (edgeMaterialId.HasValue ()) {
                                        edge.Add ("buildingMaterialId", *edgeMaterialId);
                                    }
                                    edge.Add ("pen", (Int32) edgeData.GetPen ());
                                    GS::Optional<GS::ObjectState> edgeLineTypeId = GetAttributeIdFromIndex (API_LinetypeID, ACAPI_CreateAttributeIndex (ToPlainAttrIndex (edgeData.GetLineType ())));
                                    if (edgeLineTypeId.HasValue ()) {
                                        edge.Add ("lineTypeId", *edgeLineTypeId);
                                    }
                                    edge.Add ("isVisibleLine", edgeData.IsVisibleLine ());
                                    edge.Add ("isCutEndLine", edgeData.IsCutEndLine ());
                                    edge.Add ("isInnerLine", edgeData.IsInnerLine ());
                                    edgesList (edge);
                                });
                            }

                            skinsList (skin);
                        });
                    }
                }

                ACAPI_DisposeAttrDefsHdlsExt (&profileDefs);
            }
        }

        profiles (profile);
    }

    return response;
}

GetCompositesCommand::GetCompositesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetCompositesCommand::GetName () const
{
    return "GetComposites";
}

GS::Optional<GS::UniString> GetCompositesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Composite. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/CompositeAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetCompositesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "composites": {
                "type": "array",
                "description": "A list of composites or errors.",
                "items": {
                    "$ref": "#/CompositeAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "composites"
        ]
    })";
}

GS::ObjectState GetCompositesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& composites = response.AddList<GS::ObjectState> ("composites");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_CompWallID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            composites (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState composite;
        composite.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        composite.Add ("index", GetAttributeIndex (attr.header.index));
        composite.Add ("name", name);

        if (wantsField.Wants ("useWith")) {
            const auto& useWithList = composite.AddList<GS::UniString> ("useWith");
            if ((attr.header.flags & APICWall_ForWall) != 0) {
                useWithList ("Wall");
            }
            if ((attr.header.flags & APICWall_ForSlab) != 0) {
                useWithList ("Slab");
            }
            if ((attr.header.flags & APICWall_ForRoof) != 0) {
                useWithList ("Roof");
            }
            if ((attr.header.flags & APICWall_ForShell) != 0) {
                useWithList ("Shell");
            }
        }

        bool wantsSkins = wantsField.Wants ("skins");
        bool wantsSeparators = wantsField.Wants ("separators");
        if (wantsSkins || wantsSeparators) {
            API_AttributeDefExt compositeDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_CompWallID, attr.header.index, &compositeDefs) == NoError) {
                if (wantsSkins && compositeDefs.cwall_compItems != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) compositeDefs.cwall_compItems) / sizeof (API_CWallComponent));
                    count = GS::Min (count, (Int32) attr.compWall.nComps);
                    const auto& skinsList = composite.AddList<GS::ObjectState> ("skins");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_CWallComponent& compData = (*compositeDefs.cwall_compItems)[i];
                        GS::UniString type = "Other";
                        if ((compData.flagBits & APICWallComp_Core) != 0) {
                            type = "Core";
                        } else if ((compData.flagBits & APICWallComp_Finish) != 0) {
                            type = "Finish";
                        }
                        GS::ObjectState skin (
                            "type", type,
                            "framePen", compData.framePen,
                            "thickness", compData.fillThick);
                        GS::Optional<GS::ObjectState> buildingMaterialId = GetAttributeIdFromIndex (API_BuildingMaterialID, compData.buildingMaterial);
                        if (buildingMaterialId.HasValue ()) {
                            skin.Add ("buildingMaterialId", *buildingMaterialId);
                        }
                        skinsList (skin);
                    }
                }
                if (wantsSeparators && compositeDefs.cwall_compLItems != nullptr) {
                    Int32 count = (Int32) (BMGetHandleSize ((GSHandle) compositeDefs.cwall_compLItems) / sizeof (API_CWallLineComponent));
                    count = GS::Min (count, (Int32) attr.compWall.nComps + 1);
                    const auto& separatorsList = composite.AddList<GS::ObjectState> ("separators");
                    for (Int32 i = 0; i < count; ++i) {
                        const API_CWallLineComponent& lineData = (*compositeDefs.cwall_compLItems)[i];
                        GS::ObjectState separator ("linePen", lineData.linePen);
                        GS::Optional<GS::ObjectState> lineTypeId = GetAttributeIdFromIndex (API_LinetypeID, lineData.ltypeInd);
                        if (lineTypeId.HasValue ()) {
                            separator.Add ("lineTypeId", *lineTypeId);
                        }
                        separatorsList (separator);
                    }
                }
                ACAPI_DisposeAttrDefsHdlsExt (&compositeDefs);
            }
        }

        composites (composite);
    }

    return response;
}

static GS::UniString SurfaceTypeToString (API_MaterTypeID mtype)
{
    switch (mtype) {
        case APIMater_MatteID:   return "Matte";
        case APIMater_MetalID:   return "Metal";
        case APIMater_PlasticID: return "Plastic";
        case APIMater_GlassID:   return "Glass";
        case APIMater_GlowingID: return "Glowing";
        case APIMater_ConstID:   return "Constant";
        case APIMater_SimpleID:  return "Simple";
        case APIMater_GeneralID:
        default:                 return "General";
    }
}

GetSurfacesCommand::GetSurfacesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSurfacesCommand::GetName () const
{
    return "GetSurfaces";
}

GS::Optional<GS::UniString> GetSurfacesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Surface. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/SurfaceAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetSurfacesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "surfaces": {
                "type": "array",
                "description": "A list of surfaces or errors.",
                "items": {
                    "$ref": "#/SurfaceAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "surfaces"
        ]
    })";
}

GS::ObjectState GetSurfacesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& surfaces = response.AddList<GS::ObjectState> ("surfaces");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_MaterialID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            surfaces (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState surface;
        surface.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        surface.Add ("index", GetAttributeIndex (attr.header.index));
        surface.Add ("name", name);

        if (wantsField.Wants ("materialType")) {
            surface.Add ("materialType", SurfaceTypeToString (attr.material.mtype));
        }
        if (wantsField.Wants ("ambientReflection")) {
            surface.Add ("ambientReflection", (double) attr.material.ambientPc);
        }
        if (wantsField.Wants ("diffuseReflection")) {
            surface.Add ("diffuseReflection", (double) attr.material.diffusePc);
        }
        if (wantsField.Wants ("specularReflection")) {
            surface.Add ("specularReflection", (double) attr.material.specularPc);
        }
        if (wantsField.Wants ("transparency")) {
            surface.Add ("transparency", (double) attr.material.transpPc);
        }
        if (wantsField.Wants ("shine")) {
            surface.Add ("shine", (double) attr.material.shine);
        }
        if (wantsField.Wants ("transparencyAttenuation")) {
            surface.Add ("transparencyAttenuation", (double) attr.material.transpAtt);
        }
        if (wantsField.Wants ("emissionAttenuation")) {
            surface.Add ("emissionAttenuation", (double) attr.material.emissionAtt);
        }
        if (wantsField.Wants ("surfaceColor")) {
            const API_RGBColor& rgb = attr.material.surfaceRGB;
            surface.Add ("surfaceColor", GS::ObjectState ("red", rgb.f_red, "green", rgb.f_green, "blue", rgb.f_blue));
        }
        if (wantsField.Wants ("specularColor")) {
            const API_RGBColor& rgb = attr.material.specularRGB;
            surface.Add ("specularColor", GS::ObjectState ("red", rgb.f_red, "green", rgb.f_green, "blue", rgb.f_blue));
        }
        if (wantsField.Wants ("emissionColor")) {
            const API_RGBColor& rgb = attr.material.emissionRGB;
            surface.Add ("emissionColor", GS::ObjectState ("red", rgb.f_red, "green", rgb.f_green, "blue", rgb.f_blue));
        }
        if (wantsField.Wants ("fillId")) {
            GS::Optional<GS::ObjectState> fillId = GetAttributeIdFromIndex (API_FilltypeID, attr.material.ifill);
            if (fillId.HasValue ()) {
                surface.Add ("fillId", *fillId);
            }
        }
        if (wantsField.Wants ("texture")) {
            const API_Texture& texture = attr.material.texture;
            GS::ObjectState textureOS;
            textureOS.Add ("name", GS::UniString (texture.texName));
            textureOS.Add ("rotationAngle", texture.rotAng);
            textureOS.Add ("xSize", texture.xSize);
            textureOS.Add ("ySize", texture.ySize);
            textureOS.Add ("FillRectangle", (texture.status & APITxtr_FillRectNatur) != 0);
            textureOS.Add ("FitPicture", (texture.status & APITxtr_FitPictNatur) != 0);
            textureOS.Add ("mirrorX", (texture.status & APITxtr_MirrorX) != 0);
            textureOS.Add ("mirrorY", (texture.status & APITxtr_MirrorY) != 0);
            textureOS.Add ("useAlphaChannel", (texture.status & APITxtr_UseAlpha) != 0);
            textureOS.Add ("alphaChannelChangesTransparency", (texture.status & APITxtr_TransPattern) != 0);
            textureOS.Add ("alphaChannelChangesSurfaceColor", (texture.status & APITxtr_SurfacePattern) != 0);
            textureOS.Add ("alphaChannelChangesAmbientColor", (texture.status & APITxtr_AmbientPattern) != 0);
            textureOS.Add ("alphaChannelChangesSpecularColor", (texture.status & APITxtr_SpecularPattern) != 0);
            textureOS.Add ("alphaChannelChangesDiffuseColor", (texture.status & APITxtr_DiffusePattern) != 0);
            surface.Add ("texture", textureOS);
        }

        surfaces (surface);
    }

    return response;
}

GetLayersCommand::GetLayersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLayersCommand::GetName () const
{
    return "GetLayers";
}

GS::Optional<GS::UniString> GetLayersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Layer. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/LayerAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetLayersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layers": {
                "type": "array",
                "description": "A list of layers or errors.",
                "items": {
                    "$ref": "#/LayerAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "layers"
        ]
    })";
}

GS::ObjectState GetLayersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& layers = response.AddList<GS::ObjectState> ("layers");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        GS::UniString name;
        API_Attribute attr = {};
        attr.header.typeID = API_LayerID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        attr.header.uniStringNamePtr = &name;
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            layers (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState layer;
        layer.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        layer.Add ("index", GetAttributeIndex (attr.header.index));
        layer.Add ("name", name);

        if (wantsField.Wants ("isHidden")) {
            layer.Add ("isHidden", (attr.header.flags & APILay_Hidden) != 0);
        }
        if (wantsField.Wants ("isLocked")) {
            layer.Add ("isLocked", (attr.header.flags & APILay_Locked) != 0);
        }
        if (wantsField.Wants ("isWireframe")) {
            layer.Add ("isWireframe", (attr.header.flags & APILay_ForceToWire) != 0);
        }
        if (wantsField.Wants ("intersectionGroupNr")) {
            layer.Add ("intersectionGroupNr", attr.layer.conClassId);
        }

        layers (layer);
    }

    return response;
}

static GS::UniString CutFillOrientationToString (API_FillOrientationID orientation)
{
    switch (orientation) {
        case APIFillOrientation_ElementOrigin: return "ElementOrigin";
        case APIFillOrientation_FitToSkin:      return "FitToSkin";
        case APIFillOrientation_ProjectOrigin:
        default:                                return "ProjectOrigin";
    }
}

GetBuildingMaterialsCommand::GetBuildingMaterialsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetBuildingMaterialsCommand::GetName () const
{
    return "GetBuildingMaterials";
}

GS::Optional<GS::UniString> GetBuildingMaterialsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Building Material. If omitted, every field is returned.",
                "items": {
                    "$ref": "#/BuildingMaterialAttributeField"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetBuildingMaterialsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "buildingMaterials": {
                "type": "array",
                "description": "A list of building materials or errors.",
                "items": {
                    "$ref": "#/BuildingMaterialAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "buildingMaterials"
        ]
    })";
}

GS::ObjectState GetBuildingMaterialsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GetFieldsFilter wantsField (parameters);

    GS::ObjectState response;
    const auto& buildingMaterials = response.AddList<GS::ObjectState> ("buildingMaterials");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        API_Attribute attr = {};
        attr.header.typeID = API_BuildingMaterialID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);

        // name/id/manufacturer/description are GS::UniString* out-parameters: Archicad only fills them in if
        // the caller points them at real storage before the Get call.
        GS::UniString name, id, manufacturer, description;
        attr.header.uniStringNamePtr = &name;
        attr.buildingMaterial.id = &id;
        attr.buildingMaterial.manufacturer = &manufacturer;
        attr.buildingMaterial.description = &description;

        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            buildingMaterials (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState buildingMaterial;
        buildingMaterial.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        buildingMaterial.Add ("index", GetAttributeIndex (attr.header.index));
        buildingMaterial.Add ("name", name);

        if (wantsField.Wants ("id")) {
            buildingMaterial.Add ("id", id);
        }
        if (wantsField.Wants ("manufacturer")) {
            buildingMaterial.Add ("manufacturer", manufacturer);
        }
        if (wantsField.Wants ("description")) {
            buildingMaterial.Add ("description", description);
        }
        if (wantsField.Wants ("connPriority")) {
            // attr.buildingMaterial.connPriority is stored in Archicad's internal element-priority encoding,
            // not the 1-100 scale shown in the UI and accepted by CreateBuildingMaterials (which converts via
            // ACAPI_Element_UI2ElemPriority) - must convert back with the inverse function or this would
            // return a different number than what the caller sent in.
            Int32 elemPriority = attr.buildingMaterial.connPriority;
            Int32 uiPriority = elemPriority;
            ACAPI_Element_Elem2UIPriority (&elemPriority, &uiPriority);
            buildingMaterial.Add ("connPriority", uiPriority);
        }
        if (wantsField.Wants ("cutFillIndex")) {
            buildingMaterial.Add ("cutFillIndex", GetAttributeIndex (attr.buildingMaterial.cutFill));
        }
        if (wantsField.Wants ("cutFillPen")) {
            buildingMaterial.Add ("cutFillPen", (Int32) attr.buildingMaterial.cutFillPen);
        }
        if (wantsField.Wants ("cutFillBackgroundPen")) {
            buildingMaterial.Add ("cutFillBackgroundPen", (Int32) attr.buildingMaterial.cutFillBackgroundPen);
        }
        if (wantsField.Wants ("cutSurfaceIndex")) {
            buildingMaterial.Add ("cutSurfaceIndex", GetAttributeIndex (attr.buildingMaterial.cutMaterial));
        }
        if (wantsField.Wants ("cutFillOrientation")) {
            buildingMaterial.Add ("cutFillOrientation", CutFillOrientationToString (attr.buildingMaterial.cutFillOrientation));
        }
        if (wantsField.Wants ("thermalConductivity")) {
            buildingMaterial.Add ("thermalConductivity", attr.buildingMaterial.thermalConductivity);
        }
        if (wantsField.Wants ("density")) {
            buildingMaterial.Add ("density", attr.buildingMaterial.density);
        }
        if (wantsField.Wants ("heatCapacity")) {
            buildingMaterial.Add ("heatCapacity", attr.buildingMaterial.heatCapacity);
        }
        if (wantsField.Wants ("embodiedEnergy")) {
            buildingMaterial.Add ("embodiedEnergy", attr.buildingMaterial.embodiedEnergy);
        }
        if (wantsField.Wants ("embodiedCarbon")) {
            buildingMaterial.Add ("embodiedCarbon", attr.buildingMaterial.embodiedCarbon);
        }
        if (wantsField.Wants ("showUncutLines")) {
            buildingMaterial.Add ("showUncutLines", attr.buildingMaterial.showUncutLines);
        }
        if (wantsField.Wants ("collisionDetection")) {
            buildingMaterial.Add ("collisionDetection", !attr.buildingMaterial.doNotParticipateInCollDet);
        }

        buildingMaterials (buildingMaterial);
    }

    return response;
}

DeleteAttributesCommand::DeleteAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAttributesCommand::GetName () const
{
    return "DeleteAttributes";
}

GS::Optional<GS::UniString> DeleteAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributesToDelete": {
                "type": "array",
                "description" : "Array of attributes to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeType": {
                            "$ref": "#/AttributeType"
                        },
                        "attributeId": {
                            "$ref": "#/AttributeIdArrayItem"
                        }
                    },
                    "additionalProperties": false,
                    "required": ["attributeType", "attributeId"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributesToDelete"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAttributesCommand::GetRawResponseSchema () const
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

static GSErrCode DeleteAttribute (API_AttrTypeID typeID, const API_AttributeIndex& index)
{
    API_Attr_Head attrHead = {};
    attrHead.typeID = typeID;
    attrHead.index = index;
#ifdef ServerMainVers_2600
    return ACAPI_Attribute_Delete (attrHead);
#else
    return ACAPI_Attribute_Delete (&attrHead);
#endif
}

GS::ObjectState DeleteAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributesToDelete;
    parameters.Get ("attributesToDelete", attributesToDelete);

    struct AttributeToDelete {
        API_AttrTypeID typeID = API_ZombieAttrID;
        API_AttributeIndex index;
        GS::ObjectState attributeId;
    };

    // Resolve every item first, so the results keep the input order while the deletion is grouped by type.
    GS::Array<GS::ObjectState> results;
    GS::Array<AttributeToDelete> resolved;
    GS::Array<API_AttrTypeID> typeIDs;
    for (const GS::ObjectState& item : attributesToDelete) {
        AttributeToDelete attribute;
        GS::UniString typeStr;
        item.Get ("attributeType", typeStr);
        const API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);
        if (typeID == API_ZombieAttrID) {
            results.Push (CreateFailedExecutionResult (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid attributeType '%T'.", typeStr.ToPrintf ())));
        } else {
            item.Get ("attributeId", attribute.attributeId);
            if (!GetAttributeIndexFromAttributeId (attribute.attributeId, typeID, attribute.index)) {
                results.Push (CreateFailedExecutionResult (APIERR_BADID, "Attribute not found."));
            } else {
                attribute.typeID = typeID;
                results.Push (CreateSuccessfulExecutionResult ());
                if (!typeIDs.Contains (typeID)) {
                    typeIDs.Push (typeID);
                }
            }
        }
        resolved.Push (attribute);
    }

    // One ACAPI_Attribute_DeleteMore call per attribute type instead of one ACAPI_Attribute_Delete call per attribute.
    // Archicad 25 has an older ACAPI_Attribute_DeleteMore, so there every attribute is deleted on its own below.
#ifdef ServerMainVers_2600
    for (API_AttrTypeID typeID : typeIDs) {
        GS::Array<API_AttributeIndex> indexList;
        GS::Array<Int32> addedIndices;
        for (const AttributeToDelete& attribute : resolved) {
            if (attribute.typeID != typeID) {
                continue;
            }
            const Int32 index = GetAttributeIndex (attribute.index);
            if (!addedIndices.Contains (index)) {
                indexList.Push (attribute.index);
                addedIndices.Push (index);
            }
        }
        ACAPI_Attribute_DeleteMore (typeID, indexList);
    }
#endif

    // ACAPI_Attribute_DeleteMore returns one error code for the whole list, so look up each attribute by its guid
    // again: one that still exists (the call failed or skipped it) is deleted on its own to get its own result.
    for (UIndex i = 0; i < resolved.GetSize (); ++i) {
        const AttributeToDelete& attribute = resolved[i];
        if (attribute.typeID == API_ZombieAttrID) {
            continue;
        }

        API_AttributeIndex index;
        if (!GetAttributeIndexFromAttributeId (attribute.attributeId, attribute.typeID, index)) {
            continue;
        }

        GSErrCode err = DeleteAttribute (attribute.typeID, index);
        if (err != NoError) {
            results[i] = CreateFailedExecutionResult (err, "Failed to delete.");
        }
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    for (const GS::ObjectState& result : results) {
        executionResults (result);
    }

    return response;
}

#ifdef ServerMainVers_2600

// The folder names joined with '/', for messages only: a folder is always identified by its path array.
static GS::UniString GetAttributeFolderPathString (const GS::Array<GS::UniString>& path)
{
    GS::UniString result;
    for (const GS::UniString& folderName : path) {
        if (!result.IsEmpty ()) {
            result.Append ("/");
        }
        result.Append (folderName);
    }
    return result;
}

// Reads an {attributeType, path} item into an API_AttributeFolder. Returns the failed execution result for an
// invalid item, GS::NoValue when the folder description is valid (the folder itself may still not exist).
static GS::Optional<GS::ObjectState> GetAttributeFolderFromObjectState (const GS::ObjectState& os, API_AttributeFolder& folder)
{
    GS::UniString typeStr;
    os.Get ("attributeType", typeStr);
    const API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);
    if (typeID == API_ZombieAttrID) {
        return CreateFailedExecutionResult (APIERR_BADPARS,
            GS::UniString::Printf ("Invalid attributeType '%T'.", typeStr.ToPrintf ()));
    }

    GS::Array<GS::UniString> path;
    os.Get ("path", path);
    for (const GS::UniString& folderName : path) {
        if (folderName.IsEmpty ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "A folder name in path must not be empty.");
        }
    }

    folder.typeID = typeID;
    folder.path = path;
    folder.guid = GS::Guid ();
    return GS::NoValue;
}

// Resolves folder.guid (a GS::Guid, not an API_Guid) from its typeID and path; false when no folder exists on that path.
static bool FindAttributeFolder (API_AttributeFolder& folder)
{
    folder.guid = GS::Guid ();
    return ACAPI_Attribute_GetFolder (folder) == NoError && !folder.guid.IsNull ();
}

#endif

CreateAttributeFoldersCommand::CreateAttributeFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateAttributeFoldersCommand::GetName () const
{
    return "CreateAttributeFolders";
}

GS::Optional<GS::UniString> CreateAttributeFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeFolders": {
                "$ref": "#/AttributeFolders"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeFolders"
        ]
    })";
}

GS::Optional<GS::UniString> CreateAttributeFoldersCommand::GetRawResponseSchema () const
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

GS::ObjectState CreateAttributeFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> attributeFolders;
    parameters.Get ("attributeFolders", attributeFolders);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : attributeFolders) {
        API_AttributeFolder folder;
        const GS::Optional<GS::ObjectState> invalidItem = GetAttributeFolderFromObjectState (item, folder);
        if (invalidItem.HasValue ()) {
            executionResults (*invalidItem);
            continue;
        }
        if (folder.path.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "path must not be empty: the root folder always exists."));
            continue;
        }

        // ACAPI_Attribute_CreateFolder creates every missing folder along the full path and keeps the existing
        // ones (returning NoError when the whole path already exists), so one call per item is enough. It is
        // an in/out parameter that rewrites path and guid, so it is given a copy.
        API_AttributeFolder toCreate = folder;
        const GSErrCode err = ACAPI_Attribute_CreateFolder (toCreate);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err,
                GS::UniString::Printf ("Failed to create attribute folder '%T'.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Attribute folders require Archicad 26 or newer.");
#endif
}

DeleteAttributeFoldersCommand::DeleteAttributeFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteAttributeFoldersCommand::GetName () const
{
    return "DeleteAttributeFolders";
}

GS::Optional<GS::UniString> DeleteAttributeFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeFolders": {
                "$ref": "#/AttributeFolders"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeFolders"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteAttributeFoldersCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteAttributeFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> attributeFolders;
    parameters.Get ("attributeFolders", attributeFolders);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : attributeFolders) {
        API_AttributeFolder folder;
        const GS::Optional<GS::ObjectState> invalidItem = GetAttributeFolderFromObjectState (item, folder);
        if (invalidItem.HasValue ()) {
            executionResults (*invalidItem);
            continue;
        }
        if (folder.path.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The root folder cannot be deleted."));
            continue;
        }
        if (!FindAttributeFolder (folder)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID,
                GS::UniString::Printf ("Attribute folder '%T' not found.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        const GSErrCode err = ACAPI_Attribute_DeleteFolder (folder);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err,
                GS::UniString::Printf ("Failed to delete attribute folder '%T'.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Attribute folders require Archicad 26 or newer.");
#endif
}

RenameAttributeFoldersCommand::RenameAttributeFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RenameAttributeFoldersCommand::GetName () const
{
    return "RenameAttributeFolders";
}

GS::Optional<GS::UniString> RenameAttributeFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeFolders": {
                "type": "array",
                "description": "The folders to rename, each given by attribute type and path together with its new name.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeType": {
                            "$ref": "#/AttributeType"
                        },
                        "path": {
                            "type": "array",
                            "description": "The names of the folders from the root folder down to the folder to rename, the root folder itself not included.",
                            "items": {
                                "type": "string"
                            }
                        },
                        "newName": {
                            "type": "string",
                            "description": "The new name of the folder."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["attributeType", "path", "newName"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeFolders"
        ]
    })";
}

GS::Optional<GS::UniString> RenameAttributeFoldersCommand::GetRawResponseSchema () const
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

GS::ObjectState RenameAttributeFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::Array<GS::ObjectState> attributeFolders;
    parameters.Get ("attributeFolders", attributeFolders);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : attributeFolders) {
        API_AttributeFolder folder;
        const GS::Optional<GS::ObjectState> invalidItem = GetAttributeFolderFromObjectState (item, folder);
        if (invalidItem.HasValue ()) {
            executionResults (*invalidItem);
            continue;
        }
        if (folder.path.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The root folder cannot be renamed."));
            continue;
        }

        GS::UniString newName;
        item.Get ("newName", newName);
        if (newName.IsEmpty ()) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "newName must not be empty."));
            continue;
        }

        if (!FindAttributeFolder (folder)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID,
                GS::UniString::Printf ("Attribute folder '%T' not found.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        const GSErrCode err = ACAPI_Attribute_RenameFolder (folder, newName);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err,
                GS::UniString::Printf ("Failed to rename attribute folder '%T'.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Attribute folders require Archicad 26 or newer.");
#endif
}

MoveAttributesAndFoldersCommand::MoveAttributesAndFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveAttributesAndFoldersCommand::GetName () const
{
    return "MoveAttributesAndFolders";
}

GS::Optional<GS::UniString> MoveAttributesAndFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds",
                "description": "The attributes to move. They must have the attribute type of the target folder."
            },
            "attributeFolders": {
                "$ref": "#/AttributeFolders",
                "description": "The folders to move, with their contents. They must have the attribute type of the target folder."
            },
            "targetFolder": {
                "$ref": "#/AttributeFolder",
                "description": "The folder to move the attributes and folders into. An empty path is the root folder."
            }
        },
        "additionalProperties": false,
        "required": [
            "targetFolder"
        ]
    })";
}

GS::Optional<GS::UniString> MoveAttributesAndFoldersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeExecutionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per item of attributeIds, in the same order."
            },
            "folderExecutionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per item of attributeFolders, in the same order."
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeExecutionResults",
            "folderExecutionResults"
        ]
    })";
}

GS::ObjectState MoveAttributesAndFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2600
    GS::ObjectState targetFolderOs;
    parameters.Get ("targetFolder", targetFolderOs);
    API_AttributeFolder targetFolder;
    const GS::Optional<GS::ObjectState> invalidTarget = GetAttributeFolderFromObjectState (targetFolderOs, targetFolder);
    if (invalidTarget.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid targetFolder: the attributeType is unknown or a folder name in path is empty.");
    }
    // The root folder (empty path) always exists: its guid is looked up as well, and stays null when the API reports
    // none for it, so the target is identified either way.
    if (!FindAttributeFolder (targetFolder)) {
        if (!targetFolder.path.IsEmpty ()) {
            return CreateErrorResponse (APIERR_BADID,
                GS::UniString::Printf ("Target attribute folder '%T' not found.", GetAttributeFolderPathString (targetFolder.path).ToPrintf ()));
        }
        targetFolder.guid = GS::Guid ();
    }

    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);
    GS::Array<GS::ObjectState> attributeFolders;
    parameters.Get ("attributeFolders", attributeFolders);

    GS::ObjectState response;
    const auto& attributeExecutionResults = response.AddList<GS::ObjectState> ("attributeExecutionResults");
    const auto& folderExecutionResults = response.AddList<GS::ObjectState> ("folderExecutionResults");

    // Moved one by one, so that every item gets its own result like in the other attribute commands.
    // ACAPI_Attribute_Move takes the attributes to move as GS::Guids (not API_Guids or API_Attr_Heads).
    const GS::Array<API_AttributeFolder> noFolders;
    const GS::Array<GS::Guid> noAttributes;

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        // Looked up with the target folder's type, so an attribute of another type fails here instead of in the API.
        API_AttributeIndex index;
        if (!GetAttributeIndexFromAttributeId (attributeIdItem, targetFolder.typeID, index)) {
            attributeExecutionResults (CreateFailedExecutionResult (APIERR_BADID, "Attribute not found among the attributes of the target folder's type."));
            continue;
        }

        GS::Array<GS::Guid> attributesToMove;
        attributesToMove.Push (APIGuid2GSGuid (GetGuidFromAttributesArrayItem (attributeIdItem)));
        const GSErrCode err = ACAPI_Attribute_Move (noFolders, attributesToMove, targetFolder);
        if (err != NoError) {
            attributeExecutionResults (CreateFailedExecutionResult (err, "Failed to move the attribute."));
            continue;
        }

        attributeExecutionResults (CreateSuccessfulExecutionResult ());
    }

    for (const GS::ObjectState& item : attributeFolders) {
        API_AttributeFolder folder;
        const GS::Optional<GS::ObjectState> invalidItem = GetAttributeFolderFromObjectState (item, folder);
        if (invalidItem.HasValue ()) {
            folderExecutionResults (*invalidItem);
            continue;
        }
        if (folder.path.IsEmpty ()) {
            folderExecutionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The root folder cannot be moved."));
            continue;
        }
        if (folder.typeID != targetFolder.typeID) {
            folderExecutionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The attributeType of the folder differs from the target folder's."));
            continue;
        }

        // A folder's own subtree starts with its path: moving it there would make it its own ancestor.
        bool targetIsInsideFolder = targetFolder.path.GetSize () >= folder.path.GetSize ();
        for (UIndex i = 0; targetIsInsideFolder && i < folder.path.GetSize (); ++i) {
            targetIsInsideFolder = (targetFolder.path[i] == folder.path[i]);
        }
        if (targetIsInsideFolder) {
            folderExecutionResults (CreateFailedExecutionResult (APIERR_BADPARS,
                GS::UniString::Printf ("Attribute folder '%T' cannot be moved into itself or one of its subfolders.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        if (!FindAttributeFolder (folder)) {
            folderExecutionResults (CreateFailedExecutionResult (APIERR_BADID,
                GS::UniString::Printf ("Attribute folder '%T' not found.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        GS::Array<API_AttributeFolder> foldersToMove;
        foldersToMove.Push (folder);
        const GSErrCode err = ACAPI_Attribute_Move (foldersToMove, noAttributes, targetFolder);
        if (err != NoError) {
            folderExecutionResults (CreateFailedExecutionResult (err,
                GS::UniString::Printf ("Failed to move attribute folder '%T'.", GetAttributeFolderPathString (folder.path).ToPrintf ())));
            continue;
        }

        folderExecutionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "Attribute folders require Archicad 26 or newer.");
#endif
}

CreateAttributesCommandBase::CreateAttributesCommandBase (const GS::String& commandNameIn, API_AttrTypeID attrTypeIDIn, const GS::String& arrayFieldNameIn)
    : CommandBase (CommonSchema::Used)
    , commandName (commandNameIn)
    , attrTypeID (attrTypeIDIn)
    , arrayFieldName (arrayFieldNameIn)
{
}

GS::String CreateAttributesCommandBase::GetName () const
{
    return commandName;
}

GS::Optional<GS::UniString> CreateAttributesCommandBase::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::ObjectState CreateAttributesCommandBase::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> dataArray;
    parameters.Get (arrayFieldName, dataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& data : dataArray) {
        API_Attribute attr = {};
        attr.header.typeID = attrTypeID;
        API_AttributeDef attrDef = {};

        GS::UniString name;
        if (data.Get ("name", name)) {
            attr.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            attr.header.guid = GetGuidFromAttributesArrayItem (data);

            Int32 index = -1;
            if (data.Get ("index", index) && index >= 0) {
                attr.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&attr) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (APIERR_ATTREXIST, "Already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr (it's an in/out pointer, not read-only) - when overwriteExisting finds an
        // existing attribute by guid/index, this clobbers whatever new name the caller asked for
        // with the attribute's OLD name, silently turning an intended rename into a no-op. Confirmed
        // live: renaming via attributeId+overwriteExisting had zero effect until this re-assignment
        // was added. Re-apply the caller's requested name after Get, right before Modify/Create.
        if (data.Get ("name", name)) {
            attr.header.uniStringNamePtr = &name;
        }

        const GS::Optional<GS::ObjectState> parameterError = SetTypeSpecificParameters (data, attr, attrDef);
        if (parameterError.HasValue ()) {
            ACAPI_DisposeAttrDefsHdls (&attrDef);
            attributeIds (*parameterError);
            continue;
        }

        const GSErrCode err = doesExist ? ACAPI_Attribute_Modify (&attr, &attrDef) : ACAPI_Attribute_Create (&attr, &attrDef);
        ACAPI_DisposeAttrDefsHdls (&attrDef);
        if (err != NoError) {
            attributeIds (CreateErrorResponse (err, doesExist ? "Failed to modify." : "Failed to create."));
            continue;
        }

        attributeIds (CreateAttributeIdObjectState (attr.header.guid));
    }

    return response;
}

CreateBuildingMaterialsCommand::CreateBuildingMaterialsCommand () :
    CreateAttributesCommandBase ("CreateBuildingMaterials", API_BuildingMaterialID, "buildingMaterialDataArray")
{
}

GS::Optional<GS::UniString> CreateBuildingMaterialsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "buildingMaterialDataArray": {
                "type": "array",
                "description" : "Array of data to create new Building Materials.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Building Material.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Building Material to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Building Material to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Building Material with the given name will be overwritten."
                        },
                        "id": {
                            "type": "string",
                            "description": "Identifier."
                        },
                        "manufacturer": {
                            "type": "string",
                            "description": "Manufacturer."
                        },
                        "description": {
                            "type": "string",
                            "description": "Decription."
                        },
                        "connPriority": {
                            "type": "integer",
                            "description": "Intersection priority."
                        },
                        "cutFillIndex": {
                            "type": "integer",
                            "description": "Index of the Cut Fill."
                        },
                        "cutFillPen": {
                            "type": "integer",
                            "description": "Cut Fill Foreground Pen."
                        },
                        "cutFillBackgroundPen": {
                            "type": "integer",
                            "description": "Cut Fill Background Pen."
                        },
                        "cutSurfaceIndex": {
                            "type": "integer",
                            "description": "Index of the Cut Surface."
                        },
                        "thermalConductivity": {
                            "type": "number",
                            "description": "Thermal Conductivity."
                        },
                        "density": {
                            "type": "number",
                            "description": "Density."
                        },
                        "heatCapacity": {
                            "type": "number",
                            "description": "Heat Capacity."
                        },
                        "embodiedEnergy": {
                            "type": "number",
                            "description": "Embodied Energy."
                        },
                        "embodiedCarbon": {
                            "type": "number",
                            "description": "Embodied Carbon."
                        },
                        "showUncutLines": {
                            "type": "boolean",
                            "description": "Show Contours in Model Views."
                        },
                        "collisionDetection": {
                            "type": "boolean",
                            "description": "Whether the Building Material participates in collision detection."
                        },
                        "cutFillOrientation": {
                            "type": "string",
                            "description": "ProjectOrigin, ElementOrigin, or FitToSkin. Orientation of the cut fill."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Building Material if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "buildingMaterialDataArray"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateBuildingMaterialsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
{
    static GS::UniString id;
    if (parameters.Get ("id", id)) {
        attribute.buildingMaterial.id = &id;
    }

    static GS::UniString manufacturer;
    if (parameters.Get ("manufacturer", manufacturer)) {
        attribute.buildingMaterial.manufacturer = &manufacturer;
    }

    static GS::UniString description;
    if (parameters.Get ("description", description)) {
        attribute.buildingMaterial.description = &description;
    }

    Int32 cutFillIndex;
    if (parameters.Get ("cutFillIndex", cutFillIndex)) {
        attribute.buildingMaterial.cutFill = ACAPI_CreateAttributeIndex (cutFillIndex);
    }

    Int32 connPriority;
    if (parameters.Get ("connPriority", connPriority)) {
        ACAPI_Element_UI2ElemPriority (&connPriority, &attribute.buildingMaterial.connPriority);
    }

    short cutFillPen;
    if (parameters.Get ("cutFillPen", cutFillPen)) {
        attribute.buildingMaterial.cutFillPen = cutFillPen;
    }

    short cutFillBackgroundPen;
    if (parameters.Get ("cutFillBackgroundPen", cutFillBackgroundPen)) {
        attribute.buildingMaterial.cutFillBackgroundPen = cutFillBackgroundPen;
    }

    Int32 cutSurfaceIndex;
    if (parameters.Get ("cutSurfaceIndex", cutSurfaceIndex)) {
        attribute.buildingMaterial.cutMaterial = ACAPI_CreateAttributeIndex (cutSurfaceIndex);
    }

    double thermalConductivity;
    if (parameters.Get ("thermalConductivity", thermalConductivity)) {
        attribute.buildingMaterial.thermalConductivity = thermalConductivity;
    }

    double density;
    if (parameters.Get ("density", density)) {
        attribute.buildingMaterial.density = density;
    }

    double heatCapacity;
    if (parameters.Get ("heatCapacity", heatCapacity)) {
        attribute.buildingMaterial.heatCapacity = heatCapacity;
    }

    double embodiedEnergy;
    if (parameters.Get ("embodiedEnergy", embodiedEnergy)) {
        attribute.buildingMaterial.embodiedEnergy = embodiedEnergy;
    }

    double embodiedCarbon;
    if (parameters.Get ("embodiedCarbon", embodiedCarbon)) {
        attribute.buildingMaterial.embodiedCarbon = embodiedCarbon;
    }

    parameters.Get ("showUncutLines", attribute.buildingMaterial.showUncutLines);

    bool collisionDetection;
    if (parameters.Get ("collisionDetection", collisionDetection)) {
        attribute.buildingMaterial.doNotParticipateInCollDet = !collisionDetection;
    }

    GS::UniString cutFillOrientation;
    if (parameters.Get ("cutFillOrientation", cutFillOrientation)) {
        if (cutFillOrientation == "ElementOrigin")
            attribute.buildingMaterial.cutFillOrientation = APIFillOrientation_ElementOrigin;
        else if (cutFillOrientation == "FitToSkin")
            attribute.buildingMaterial.cutFillOrientation = APIFillOrientation_FitToSkin;
        else
            attribute.buildingMaterial.cutFillOrientation = APIFillOrientation_ProjectOrigin;
    }

    return {};
}

CreateLayersCommand::CreateLayersCommand () :
    CreateAttributesCommandBase ("CreateLayers", API_LayerID, "layerDataArray")
{
}

GS::Optional<GS::UniString> CreateLayersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layerDataArray": {
                "type": "array",
                "description" : "Array of data to create new Layers.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Layer.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Layer to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Layer to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Layer with the given name will be overwritten."
                        },
                        "isHidden": {
                            "type": "boolean",
                            "description": "Hide/Show."
                        },
                        "isLocked": {
                            "type": "boolean",
                            "description": "Lock/Unlock."
                        },
                        "isWireframe": {
                            "type": "boolean",
                            "description": "Force the model to wireframe."
                        },
                        "intersectionGroupNr": {
                            "type": "integer",
                            "description": "Intersection group. Elements on layers having the same group will be intersected."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Layer if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "layerDataArray"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateLayersCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
{
    bool hidden;
    if (parameters.Get ("isHidden", hidden)) {
        if (hidden)
            attribute.header.flags |= APILay_Hidden;
        else
            attribute.header.flags &= ~APILay_Hidden;
    }

    bool locked;
    if (parameters.Get ("isLocked", locked)) {
        if (locked)
            attribute.header.flags |= APILay_Locked;
        else
            attribute.header.flags &= ~APILay_Locked;
    }

    bool wireframe;
    if (parameters.Get ("isWireframe", wireframe)) {
        if (wireframe)
            attribute.header.flags |= APILay_ForceToWire;
        else
            attribute.header.flags &= ~APILay_ForceToWire;
    }

    parameters.Get ("intersectionGroupNr", attribute.layer.conClassId);

    return {};
}

CreateLayerCombinationsCommand::CreateLayerCombinationsCommand () :
    CreateAttributesCommandBase ("CreateLayerCombinations", API_LayerCombID, "layerCombinationDataArray")
{
}

GS::Optional<GS::UniString> CreateLayerCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layerCombinationDataArray": {
                "type": "array",
                "description" : "Array of data to create new Layer Combinations.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Layer Combination.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Layer Combination to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Layer Combination to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Layer Combination with the given name will be overwritten."
                        },
                        "layers": {
                            "$ref": "#/LayersOfLayerCombination",
                            "description": "Layer states to set. Layers not listed keep their current state when overwriting an existing Layer Combination (or get the default visible/unlocked state for a new one), so this can be omitted to e.g. only rename an existing Layer Combination."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Layer Combination if exists with the same guid/index/name. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "layerCombinationDataArray"
        ]
    })";
}

// A layer combination's stat table is NOT sparse: per the AC27 devkit's own remark on
// API_LayerCombType, "the number of layers (the lNumb field) has the same value in all layer
// combinations" - i.e. lNumb is the project's total layer count, not however many layers the
// caller happens to mention, and Archicad expects one API_LayerStat entry per project layer, not
// just for the ones being overridden. Confirmed live: populating layer_statItems with only the
// requested layer(s) still reports lNumb layers back once read (Archicad fills in the rest
// itself), but every layer we did NOT explicitly add came back with garbage/inconsistent flags
// (mostly isHidden=true, one random layer with isWireframe=true, scattered with no pattern
// matching the request) instead of the documented "new layers appear as unlocked and visible" -
// i.e. Archicad reads past our sparse table into uninitialized memory for any layer index we
// left out, rather than defaulting it. The fix is to always seed a full entry for every project
// layer (visible/unlocked/not wireframe/group 1, matching a brand new layer's real defaults) and
// only then apply the caller's overrides on top - mirroring how CreatePenTables/CreateProfiles
// already seed a full table from a source attribute before overriding parts of it.
static API_LayerStat DefaultLayerStat ()
{
    API_LayerStat layerStat = {};
    layerStat.conClassId = 1;
    return layerStat;
}

GS::Optional<GS::ObjectState> CreateLayerCombinationsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
{
    GS::Array<GS::ObjectState> layers;
    parameters.Get ("layers", layers);

    // Seed every project layer with a default stat, preferring the layer combination's own
    // current table (if we are modifying one that already exists) so that layers not mentioned
    // in this request keep whatever hidden/locked/wireframe/group state they already had, rather
    // than being reset to defaults on every edit.
    GS::HashTable<API_AttributeIndex, API_LayerStat> seedStats;
    API_AttributeDef existingDef = {};
    bool hasExistingDef = false;
    if (attribute.header.index != APIInvalidAttributeIndex && ACAPI_Attribute_GetDef (API_LayerCombID, attribute.header.index, &existingDef) == NoError) {
        hasExistingDef = true;
#ifdef ServerMainVers_2700
        for (const auto& kv : *existingDef.layer_statItems) {
#ifdef ServerMainVers_2800
            seedStats.Add (kv.key, kv.value);
#else
            seedStats.Add (*kv.key, *kv.value);
#endif
        }
#else
        for (Int32 i = 0; i < attribute.layerComb.lNumb; ++i) {
            const API_LayerStat& layerStat = (*existingDef.layer_statItems)[i];
            seedStats.Add (layerStat.lInd, layerStat);
        }
#endif
    }

    GS::Array<API_Attribute> allLayers;
    ACAPI_Attribute_GetAttributesByType (API_LayerID, allLayers);
    for (API_Attribute& layerAttr : allLayers) {
        if (!seedStats.ContainsKey (layerAttr.header.index)) {
            seedStats.Add (layerAttr.header.index, DefaultLayerStat ());
        }
        DisposeAttribute (layerAttr);
    }

    for (const GS::ObjectState& layer : layers) {
        API_LayerStat layerStat = DefaultLayerStat ();

        bool isHidden = false;
        layer.Get ("isHidden", isHidden);
        if (isHidden)
            layerStat.lFlags |= APILay_Hidden;

        bool isLocked = false;
        layer.Get ("isLocked", isLocked);
        if (isLocked)
            layerStat.lFlags |= APILay_Locked;

        bool isWireframe = false;
        layer.Get ("isWireframe", isWireframe);
        if (isWireframe)
            layerStat.lFlags |= APILay_ForceToWire;

        Int32 intersectionGroupNr = 1;
        layer.Get ("intersectionGroupNr", intersectionGroupNr);
        layerStat.conClassId = intersectionGroupNr;

        API_AttributeIndex layerIndex;
        if (GetAttributeIndexFromAttributeId (layer, API_LayerID, layerIndex)) {
            // Put, not Add: this layer's key was already seeded above with a default stat (every
            // project layer is seeded first), and GS::HashTable::Add is a no-op returning false
            // when the key already exists - it does NOT overwrite. Confirmed live: using Add here
            // silently dropped every override, leaving the seeded defaults in place even though
            // the request came back accepted. Put unconditionally replaces the existing entry.
            seedStats.Put (layerIndex, layerStat);
        }
    }

    attribute.layerComb.lNumb = static_cast<Int32> (seedStats.GetSize ());

#ifdef ServerMainVers_2700
    attributeDef.layer_statItems = new GS::HashTable<API_AttributeIndex, API_LayerStat> (seedStats);
#else
    attributeDef.layer_statItems = (API_LayerStat **) BMAllocateHandle (attribute.layerComb.lNumb * sizeof (API_LayerStat), ALLOCATE_CLEAR, 0);
    Int32 i = 0;
    for (const auto& kv : seedStats) {
        // Pre-2700 GS::HashTable's const range-for pair exposes pointer members (unlike 2800+'s
        // references) - matches the same distinction already handled above for existingDef.
        API_LayerStat& layerStat = (*attributeDef.layer_statItems)[i++];
        layerStat = *kv.value;
        layerStat.lInd = *kv.key;
    }
#endif

    if (hasExistingDef) {
        ACAPI_DisposeAttrDefsHdls (&existingDef);
    }

    return {};
}

CreateLinesCommand::CreateLinesCommand () :
    CreateAttributesCommandBase ("CreateLines", API_LinetypeID, "lineDataArray")
{
}

GS::Optional<GS::UniString> CreateLinesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "lineDataArray": {
                "type": "array",
                "description" : "Array of data to create new Lines.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Line.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Line to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Line to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Line with the given name will be overwritten."
                        },
                        "scaleWithPlan": {
                            "type": "boolean",
                            "description": "If true, the line type parameters are defined in meters at the given defineScale and scale on printout with the actual plan scale. If false (default), the parameters are fixed values in millimeters as the line will appear on the printout."
                        },
                        "defineScale": {
                            "type": "number",
                            "description": "The floor plan scale the line type is defined with. Only used if scaleWithPlan is true."
                        },
                        "lineType": {
                            "type": "string",
                            "description": "Solid, Dashed, or Symbol. Defaults to Solid."
                        },
                        "period": {
                            "type": "number",
                            "description": "The length of one period (Dashed and Symbol line types)."
                        },
                        "height": {
                            "type": "number",
                            "description": "The height of the symbol line (Symbol line type only)."
                        },
                        "dashItems": {
                            "type": "array",
                            "description": "Dash-gap pairs describing one period (Dashed line type only).",
                            "items": {
                                "$ref": "#/LineDashItem"
                            }
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Symbol items describing one period (Symbol line type only).",
                            "items": {
                                "$ref": "#/LineSymbolItemData"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Line if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "lineDataArray"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateLinesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
{
    bool scaleWithPlan = false;
    if (parameters.Get ("scaleWithPlan", scaleWithPlan)) {
        if (scaleWithPlan) {
            attribute.header.flags &= ~APILine_FixScale;
            attribute.header.flags |= APILine_ScaleWithPlan;
        } else {
            attribute.header.flags &= ~APILine_ScaleWithPlan;
            attribute.header.flags |= APILine_FixScale;
        }
    }

    parameters.Get ("defineScale", attribute.linetype.defineScale);

    // Seed the dash/symbol item Ext handles from the line being overwritten (if any), so a modify call that
    // doesn't touch lineType/dashItems/lineItems doesn't leave attribute.linetype.nItems (already preserved via
    // the base class's ACAPI_Attribute_Get) pointing at a null handle - that mismatch made ACAPI_Attribute_Modify
    // fail outright (same root cause as the equivalent CreateFills bug: an Ext geometry count preserved from the
    // existing attribute, paired with a freshly zero-initialized handle for this call, is an inconsistent state
    // Archicad rejects). Only relevant when overwriting an existing line; a brand new one has nothing to seed.
    if (IsPositiveAttributeIndex (attribute.header.index)) {
        API_AttributeDef existingDef = {};
        if (ACAPI_Attribute_GetDef (API_LinetypeID, attribute.header.index, &existingDef) == NoError) {
            attributeDef.ltype_dashItems = existingDef.ltype_dashItems;
            attributeDef.ltype_lineItems = existingDef.ltype_lineItems;
        }
    }

    // Only touch linetype.type (and the period/height/dashItems/lineItems that go with it) if lineType was
    // actually supplied, so an overwriteExisting call that only changes e.g. defineScale doesn't silently
    // convert an existing Dashed or Symbol line back to Solid (APILine_SolidLine is 0, i.e. the zero-initialized
    // default, so a brand new line with no lineType still correctly comes out Solid).
    GS::UniString lineType;
    if (!parameters.Get ("lineType", lineType)) {
        return {};
    }

    if (lineType == "Dashed") {
        attribute.linetype.type = APILine_DashedLine;
        parameters.Get ("period", attribute.linetype.period);

        // Only touch nItems/ltype_dashItems if dashItems was actually supplied. Leaving them alone when the
        // caller omits dashItems (e.g. an overwriteExisting call that only changes defineScale) preserves the
        // existing dash pattern instead of wiping it out (nItems would otherwise drop to 0).
        GS::Array<GS::ObjectState> dashItems;
        if (parameters.Get ("dashItems", dashItems)) {
            UInt32 nItems = dashItems.GetSize ();
            attribute.linetype.nItems = (Int32) nItems;

            if (nItems > 0) {
                attributeDef.ltype_dashItems = (API_DashItems**) BMAllocateHandle (nItems * sizeof (API_DashItems), ALLOCATE_CLEAR, 0);
                for (UInt32 i = 0; i < nItems; ++i) {
                    API_DashItems& item = (*attributeDef.ltype_dashItems)[i];
                    dashItems[i].Get ("dash", item.dash);
                    dashItems[i].Get ("gap", item.gap);
                }
            }
        }
    } else if (lineType == "Symbol") {
        attribute.linetype.type = APILine_SymbolLine;
        parameters.Get ("period", attribute.linetype.period);
        parameters.Get ("height", attribute.linetype.height);

        // Same reasoning as dashItems above: only rebuild lineItems/nItems if lineItems was actually supplied.
        GS::Array<GS::ObjectState> lineItems;
        if (parameters.Get ("lineItems", lineItems)) {
            UInt32 nItems = lineItems.GetSize ();
            attribute.linetype.nItems = (Int32) nItems;

            if (nItems > 0) {
                attributeDef.ltype_lineItems = (API_LineItems**) BMAllocateHandle (nItems * sizeof (API_LineItems), ALLOCATE_CLEAR, 0);
                for (UInt32 i = 0; i < nItems; ++i) {
                    API_LineItems& item = (*attributeDef.ltype_lineItems)[i];
                    const GS::ObjectState& itemData = lineItems[i];

                    GS::UniString itemType;
                    itemData.Get ("itemType", itemType);
                    if (itemType == "Separator")
                        item.itemType = APILine_SeparatorItemType;
                    else if (itemType == "CenterDot")
                        item.itemType = APILine_CenterDotItemType;
                    else if (itemType == "CenterLine")
                        item.itemType = APILine_CenterLineItemType;
                    else if (itemType == "Dot")
                        item.itemType = APILine_DotItemType;
                    else if (itemType == "RightAngle")
                        item.itemType = APILine_RightAngleItemType;
                    else if (itemType == "Parallel")
                        item.itemType = APILine_ParallelItemType;
                    else if (itemType == "Circle")
                        item.itemType = APILine_CircItemType;
                    else if (itemType == "Arc")
                        item.itemType = APILine_ArcItemType;
                    else
                        item.itemType = APILine_LineItemType;

                    itemData.Get ("centerOffset", item.itemCenterOffs);
                    itemData.Get ("length", item.itemLength);
                    if (const GS::ObjectState* begPos = itemData.Get ("begPos")) {
                        item.itemBegPos = Get2DCoordinateFromObjectState (*begPos);
                    }
                    if (const GS::ObjectState* endPos = itemData.Get ("endPos")) {
                        item.itemEndPos = Get2DCoordinateFromObjectState (*endPos);
                    }
                    itemData.Get ("radius", item.itemRadius);
                    itemData.Get ("beginAngle", item.itemBegAngle);
                    itemData.Get ("endAngle", item.itemEndAngle);
                }
            }
        }
    } else {
        attribute.linetype.type = APILine_SolidLine;
    }

    return {};
}

CreateFillsCommand::CreateFillsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateFillsCommand::GetName () const
{
    return "CreateFills";
}

GS::Optional<GS::UniString> CreateFillsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fillDataArray": {
                "type": "array",
                "description" : "Array of data to create new Fills.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Fill.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Fill to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Fill to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Fill with the given name will be overwritten."
                        },
                        "subType": {
                            "type": "string",
                            "description": "Vector, Solid, Empty, Symbol, LinearGradient, RadialGradient, or Image. Defaults to Vector. Only one Solid and one Empty fill may exist. Image fills use the texture field's name to reference the image library part."
                        },
                        "scaleWithPlan": {
                            "type": "boolean",
                            "description": "The fill is scale dependent."
                        },
                        "useForWalls": {
                            "type": "boolean",
                            "description": "This fill can be used for cut fills."
                        },
                        "useForDraft": {
                            "type": "boolean",
                            "description": "This fill can be used for drafting fills."
                        },
                        "useForCover": {
                            "type": "boolean",
                            "description": "This fill can be used for cover fills."
                        },
                        "horizontalSpacing": {
                            "type": "number",
                            "description": "The fill's spacing factor in the X direction (Vector fills)."
                        },
                        "verticalSpacing": {
                            "type": "number",
                            "description": "The fill's spacing factor in the Y direction (Vector fills)."
                        },
                        "angle": {
                            "type": "number",
                            "description": "The angle of the fill in radians (Vector, Symbol, and gradient fills)."
                        },
                        "bitPattern": {
                            "type": "string",
                            "description": "16 hex characters (8 bytes) describing the fill's bitmap pattern, one line of the pattern per byte, matching the Pattern field of the Attribute Manager XML export."
                        },
                        "gradientStart": {
                            "description": "Gradient start point (LinearGradient/RadialGradient fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "gradientEnd": {
                            "description": "Gradient end point (LinearGradient/RadialGradient fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "symbolOffset1": {
                            "description": "First offset vector of the repeating symbol motif, X1/Y1 in the Fill Types dialog (Symbol fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "symbolOffset2": {
                            "description": "Second offset vector of the repeating symbol motif, X2/Y2 in the Fill Types dialog (Symbol fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "percent": {
                            "type": "number",
                            "description": "Translucency percentage [0..1] (gradient and some Solid fills)."
                        },
                        "texture": {
                            "description": "Used for Image and gradient fills. Only name, rotationAngle, xSize, ySize, mirrorX, and mirrorY are used for Fills.",
                            "$ref": "#/Texture"
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Vectorial fill line items (Vector fills only).",
                            "items": {
                                "$ref": "#/FillLineItem"
                            }
                        },
                        "symbolLines": {
                            "type": "array",
                            "description": "Line items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/FillSymbolLine"
                            }
                        },
                        "symbolArcs": {
                            "type": "array",
                            "description": "Arc items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/FillSymbolArc"
                            }
                        },
                        "symbolHotspots": {
                            "type": "array",
                            "description": "Hotspot coordinates of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/Coordinate2D"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Fill if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "fillDataArray"
        ]
    })";
}

GS::Optional<GS::UniString> CreateFillsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

static void SetBitPattern (const GS::UniString& hex, API_Pattern& pattern)
{
    // hex.ToCStr() returns a temporary owning the buffer - it must be kept alive as a named local for as long as
    // hexCStr is used, or the pointer dangles past the end of this statement (caught by Clang's -Wdangling on Mac
    // CI, silently not diagnosed by MSVC on Windows).
    const auto hexCToken = hex.ToCStr ();
    const char* hexCStr = hexCToken.Get ();
    size_t len = strlen (hexCStr);
    for (UInt32 i = 0; i < 8 && (i * 2 + 1) < len; ++i) {
        char byteChars[3] = { hexCStr[i * 2], hexCStr[i * 2 + 1], 0 };
        pattern[i] = (unsigned char) strtoul (byteChars, nullptr, 16);
    }
}

GS::ObjectState CreateFillsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> fillDataArray;
    parameters.Get ("fillDataArray", fillDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& data : fillDataArray) {
        API_Attribute fill = {};
        API_AttributeDefExt fillDefs = {};
        fill.header.typeID = API_FilltypeID;

        GS::UniString name;
        if (data.Get ("name", name)) {
            fill.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            fill.header.guid = GetGuidFromAttributesArrayItem (data);

            Int32 index = -1;
            if (data.Get ("index", index) && index >= 0) {
                fill.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&fill) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (APIERR_ATTREXIST, "Already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr, clobbering a requested rename with the OLD name before Modify ever sees
        // it - same bug as CreateAttributesCommandBase, fixed there this session; re-apply here too.
        if (data.Get ("name", name)) {
            fill.header.uniStringNamePtr = &name;
        }

        // Seed the Ext geometry handles (lineItems for Vector fills, symbolLines/symbolArcs/symbolHotspots for
        // Symbol fills) from the fill being overwritten, so a partial modify that only changes e.g. angle
        // doesn't wipe out the existing pattern - mirrors the linNumb/arcNumb/hotNumb counts already preserved
        // via the ACAPI_Attribute_Get call above filling in fill.filltype. Below, each geometry kind's own
        // "if (data.Get (...))" block overwrites the corresponding seeded handle with a freshly allocated one
        // when the caller does supply that geometry, exactly like CreateProfiles' profile_vectorImageItems.
        API_AttributeDefExt existingDefs = {};
        if (doesExist) {
            ACAPI_Attribute_GetDefExt (API_FilltypeID, fill.header.index, &existingDefs);
            fillDefs.fill_lineItems = existingDefs.fill_lineItems;
            fillDefs.fill_lineLength = existingDefs.fill_lineLength;
            fillDefs.sfill_Items = existingDefs.sfill_Items;
        }

        // Only touch filltype.subType if subType was actually supplied, so an overwriteExisting call that omits
        // it doesn't silently convert an existing Solid/Empty/Symbol/Gradient fill back to Vector (APIFill_Vector
        // is 0, i.e. the zero-initialized default, so a brand new fill with no subType still comes out Vector).
        GS::UniString subType;
        if (data.Get ("subType", subType)) {
            if (subType == "Solid")
                fill.filltype.subType = APIFill_Solid;
            else if (subType == "Empty")
                fill.filltype.subType = APIFill_Empty;
            else if (subType == "Symbol")
                fill.filltype.subType = APIFill_Symbol;
            else if (subType == "LinearGradient")
                fill.filltype.subType = APIFill_LinearGradient;
            else if (subType == "RadialGradient")
                fill.filltype.subType = APIFill_RadialGradient;
            else if (subType == "Image")
                fill.filltype.subType = APIFill_Image;
            else
                fill.filltype.subType = APIFill_Vector;
        }

        bool scaleWithPlan;
        if (data.Get ("scaleWithPlan", scaleWithPlan)) {
            if (scaleWithPlan)
                fill.header.flags |= APIFill_ScaleWithPlan;
            else
                fill.header.flags &= ~APIFill_ScaleWithPlan;
        }

        bool useForWalls;
        if (data.Get ("useForWalls", useForWalls)) {
            if (useForWalls)
                fill.header.flags |= APIFill_ForWall;
            else
                fill.header.flags &= ~APIFill_ForWall;
        }

        bool useForDraft;
        if (data.Get ("useForDraft", useForDraft)) {
            if (useForDraft)
                fill.header.flags |= APIFill_ForPoly;
            else
                fill.header.flags &= ~APIFill_ForPoly;
        }

        bool useForCover;
        if (data.Get ("useForCover", useForCover)) {
            if (useForCover)
                fill.header.flags |= APIFill_ForCover;
            else
                fill.header.flags &= ~APIFill_ForCover;
        }

        data.Get ("horizontalSpacing", fill.filltype.hXSpac);
        data.Get ("verticalSpacing", fill.filltype.hYSpac);
        data.Get ("angle", fill.filltype.hAngle);
        data.Get ("percent", fill.filltype.percent);

        GS::UniString bitPattern;
        if (data.Get ("bitPattern", bitPattern)) {
            SetBitPattern (bitPattern, fill.filltype.bitPat);
        }

        if (const GS::ObjectState* gradientStart = data.Get ("gradientStart")) {
            fill.filltype.c1 = Get2DCoordinateFromObjectState (*gradientStart);
        }
        if (const GS::ObjectState* gradientEnd = data.Get ("gradientEnd")) {
            fill.filltype.c2 = Get2DCoordinateFromObjectState (*gradientEnd);
        }
        // Same c1/c2 fields, under the names GetFills returns them for Symbol fills.
        if (const GS::ObjectState* symbolOffset1 = data.Get ("symbolOffset1")) {
            fill.filltype.c1 = Get2DCoordinateFromObjectState (*symbolOffset1);
        }
        if (const GS::ObjectState* symbolOffset2 = data.Get ("symbolOffset2")) {
            fill.filltype.c2 = Get2DCoordinateFromObjectState (*symbolOffset2);
        }

        GS::ObjectState textureObj;
        if (data.Get ("texture", textureObj)) {
            SetUCharProperty (&textureObj, "name", fill.filltype.textureName);

            double rotationAngle;
            if (textureObj.Get ("rotationAngle", rotationAngle)) {
                fill.filltype.textureRotAng = rotationAngle;
            }
            double xSize;
            if (textureObj.Get ("xSize", xSize)) {
                fill.filltype.textureXSize = xSize;
            }
            double ySize;
            if (textureObj.Get ("ySize", ySize)) {
                fill.filltype.textureYSize = ySize;
            }
            bool mirrorX;
            if (textureObj.Get ("mirrorX", mirrorX)) {
                if (mirrorX)
                    fill.filltype.textureStatus |= APITxtr_MirrorX;
                else
                    fill.filltype.textureStatus &= ~APITxtr_MirrorX;
            }
            bool mirrorY;
            if (textureObj.Get ("mirrorY", mirrorY)) {
                if (mirrorY)
                    fill.filltype.textureStatus |= APITxtr_MirrorY;
                else
                    fill.filltype.textureStatus &= ~APITxtr_MirrorY;
            }
        }

        // Only touch linNumb/fill_lineItems if lineItems was actually supplied, so an overwriteExisting call that
        // only changes e.g. the angle doesn't wipe out the existing vector fill's line pattern (linNumb would
        // otherwise drop to 0).
        GS::Array<GS::ObjectState> lineItems;
        if (data.Get ("lineItems", lineItems)) {
            UInt32 nItems = lineItems.GetSize ();
            fill.filltype.linNumb = (Int32) nItems;

            if (nItems > 0) {
                fillDefs.fill_lineItems = (API_FillLine**) BMAllocateHandle (nItems * sizeof (API_FillLine), ALLOCATE_CLEAR, 0);

                UInt32 totalLengths = 0;
                for (const GS::ObjectState& itemData : lineItems) {
                    GS::Array<double> lineLengths;
                    itemData.Get ("lineLengths", lineLengths);
                    totalLengths += lineLengths.GetSize ();
                }
                if (totalLengths > 0) {
                    fillDefs.fill_lineLength = (double**) BMAllocateHandle (totalLengths * sizeof (double), ALLOCATE_CLEAR, 0);
                }

                Int32 lengthOffset = 0;
                for (UInt32 i = 0; i < nItems; ++i) {
                    const GS::ObjectState& itemData = lineItems[i];
                    API_FillLine& item = (*fillDefs.fill_lineItems)[i];

                    itemData.Get ("frequency", item.lFreq);
                    itemData.Get ("direction", item.lDir);
                    itemData.Get ("offsetLine", item.lOffsetLine);
                    if (const GS::ObjectState* offset = itemData.Get ("offset")) {
                        item.lOffset = Get2DCoordinateFromObjectState (*offset);
                    }

                    GS::Array<double> lineLengths;
                    itemData.Get ("lineLengths", lineLengths);
                    item.lPartNumb = (short) lineLengths.GetSize ();
                    item.lPartOffs = lengthOffset;
                    for (UInt32 j = 0; j < lineLengths.GetSize (); ++j) {
                        (*fillDefs.fill_lineLength)[lengthOffset + j] = lineLengths[j];
                    }
                    lengthOffset += (Int32) lineLengths.GetSize ();
                }
            }
        }

        // Symbol fill geometry (sfill_Lines/sfill_Arcs/sfill_HotSpots) only exists in API_AttributeDefExt, which
        // is why CreateFills uses the Ext create/modify functions instead of the CreateAttributesCommandBase
        // pattern used by the other simple attribute types. Solid-fill sub-polygons within a symbol fill
        // (sfill_SolidFills) are not supported - not used by any real-world example seen so far.
        GS::Array<GS::ObjectState> symbolLines;
        if (data.Get ("symbolLines", symbolLines)) {
            UInt32 nItems = symbolLines.GetSize ();
            fill.filltype.linNumb = (Int32) nItems;

            if (nItems > 0) {
                fillDefs.sfill_Items.sfill_Lines = (API_SFill_Line**) BMAllocateHandle (nItems * sizeof (API_SFill_Line), ALLOCATE_CLEAR, 0);
                for (UInt32 i = 0; i < nItems; ++i) {
                    API_SFill_Line& item = (*fillDefs.sfill_Items.sfill_Lines)[i];
                    if (const GS::ObjectState* begin = symbolLines[i].Get ("begin")) {
                        item.begC = Get2DCoordinateFromObjectState (*begin);
                    }
                    if (const GS::ObjectState* end = symbolLines[i].Get ("end")) {
                        item.endC = Get2DCoordinateFromObjectState (*end);
                    }
                }
            }
        }

        GS::Array<GS::ObjectState> symbolArcs;
        if (data.Get ("symbolArcs", symbolArcs)) {
            UInt32 nItems = symbolArcs.GetSize ();
            fill.filltype.arcNumb = (Int32) nItems;

            if (nItems > 0) {
                fillDefs.sfill_Items.sfill_Arcs = (API_SFill_Arc**) BMAllocateHandle (nItems * sizeof (API_SFill_Arc), ALLOCATE_CLEAR, 0);
                for (UInt32 i = 0; i < nItems; ++i) {
                    API_SFill_Arc& item = (*fillDefs.sfill_Items.sfill_Arcs)[i];
                    if (const GS::ObjectState* begin = symbolArcs[i].Get ("begin")) {
                        item.begC = Get2DCoordinateFromObjectState (*begin);
                    }
                    if (const GS::ObjectState* origin = symbolArcs[i].Get ("origin")) {
                        item.origC = Get2DCoordinateFromObjectState (*origin);
                    }
                    symbolArcs[i].Get ("angle", item.angle);
                }
            }
        }

        GS::Array<GS::ObjectState> symbolHotspots;
        if (data.Get ("symbolHotspots", symbolHotspots)) {
            UInt32 nItems = symbolHotspots.GetSize ();
            fill.filltype.hotNumb = (Int32) nItems;

            if (nItems > 0) {
                fillDefs.sfill_Items.sfill_HotSpots = (API_Coord**) BMAllocateHandle (nItems * sizeof (API_Coord), ALLOCATE_CLEAR, 0);
                for (UInt32 i = 0; i < nItems; ++i) {
                    (*fillDefs.sfill_Items.sfill_HotSpots)[i] = Get2DCoordinateFromObjectState (symbolHotspots[i]);
                }
            }
        }

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_ModifyExt (&fill, &fillDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify."));
                ACAPI_DisposeAttrDefsHdlsExt (&fillDefs);
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_CreateExt (&fill, &fillDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create."));
                ACAPI_DisposeAttrDefsHdlsExt (&fillDefs);
                continue;
            }
        }

        attributeIds (CreateAttributeIdObjectState (fill.header.guid));
        ACAPI_DisposeAttrDefsHdlsExt (&fillDefs);
    }

    return response;
}

CreateZoneCategoriesCommand::CreateZoneCategoriesCommand () :
    CreateAttributesCommandBase ("CreateZoneCategories", API_ZoneCatID, "zoneCategoryDataArray")
{
}

GS::Optional<GS::UniString> CreateZoneCategoriesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zoneCategoryDataArray": {
                "type": "array",
                "description" : "Array of data to create new Zone Categories.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Zone Category.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Zone Category to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Zone Category to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Zone Category with the given name will be overwritten."
                        },
                        "categoryCode": {
                            "type": "string",
                            "description": "Code of the Zone Category."
                        },
                        "color": {
                            "$ref": "#/ColorRGB"
                        },
                        "stampName": {
                            "type": "string",
                            "description": "Document name of the zone stamp library part (GSM) to use, e.g. the value returned by GetZoneCategories or shown as StampDocumentName in an Attribute Manager XML export. Only used together with stampMainGuid/stampRevGuid; ignored otherwise."
                        },
                        "stampMainGuid": {
                            "type": "string",
                            "description": "Main GUID of the zone stamp library part to use, e.g. the value returned by GetZoneCategories for an existing Zone Category or shown as MainGuid in an Attribute Manager XML export. If omitted, the stamp is copied from the Zone Category being overwritten, or from an existing Zone Category of the project when creating a new one."
                        },
                        "stampRevGuid": {
                            "type": "string",
                            "description": "Revision GUID of the zone stamp library part to use, e.g. the value shown as RevGuid in an Attribute Manager XML export. Required together with stampMainGuid."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Zone Category if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneCategoryDataArray"
        ]
    })";
}

// The default GDL parameters of the zone stamp library part with the given main GUID, or nullptr when the
// library part is not found in the loaded libraries or its parameters cannot be read. The caller owns the
// returned handle (ACAPI_DisposeAddParHdl / ACAPI_DisposeAttrDefsHdls). The library part is looked up the same
// way the element creation commands look up objects, by its unique ID (the "{main}-{revision}" string the
// API_LibPart::ownUnID field carries) and, when given, by its document name.
static API_AddParType** GetZoneStampParameters (const API_Guid& stampMainGuid, const API_Guid& stampRevGuid, const GS::uchar_t* stampName)
{
    API_LibPart libPart = {};
    libPart.typeID = APILib_RoomID;
    const GS::UniString unIdStr = GS::UniString::Printf ("{%T}-{%T}", APIGuidToString (stampMainGuid).ToPrintf (), APIGuidToString (stampRevGuid).ToPrintf ());
    CHTruncate (unIdStr.ToCStr ().Get (), libPart.ownUnID, sizeof (libPart.ownUnID));
    if (stampName != nullptr) {
        GS::ucscpy (libPart.docu_UName, stampName);
    }

    const GSErrCode searchErr = ACAPI_LibraryPart_Search (&libPart, false, false);
    delete libPart.location;
    if (searchErr != NoError || GS::UnID (libPart.ownUnID).GetMainGuid () != APIGuid2GSGuid (stampMainGuid)) {
        return nullptr;
    }

    double a = 0.0;
    double b = 0.0;
    Int32 addParNum = 0;
    API_AddParType** addPars = nullptr;
    if (ACAPI_LibraryPart_GetParams (libPart.index, &a, &b, &addParNum, &addPars) != NoError) {
        if (addPars != nullptr) {
            ACAPI_DisposeAddParHdl (&addPars);
        }
        return nullptr;
    }

    return addPars;
}

GS::Optional<GS::ObjectState> CreateZoneCategoriesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
{
    // ACAPI_Attribute_Create/Modify needs a zone stamp (stampName/stampMainGuid/stampRevGuid) and the stamp's GDL
    // parameters (zone_addParItems) for API_ZoneCatID, otherwise it refuses the whole attribute - that is the
    // "Failed to create." every CreateZoneCategories call came back with in #838. Both are copied from a template
    // Zone Category: the one being overwritten (attribute.header.index is already resolved at this point, so this
    // reads back its own current stamp and parameters and nothing is lost), or - when creating a brand new one -
    // an existing Zone Category of the project. That template used to be the Zone Category at the fixed index 1.
    // AC27+ never reuses the index of a deleted attribute, so a project whose original first Zone Category was
    // deleted (an office template evolved over the years) has nothing at index 1, the lookups failed silently and
    // Archicad refused the stampless result. Enumerate the project's Zone Categories instead, and report it when
    // there is nothing to copy from.
    GS::UniString stampMainGuidStr, stampRevGuidStr;
    const bool hasExplicitStamp = parameters.Get ("stampMainGuid", stampMainGuidStr) && parameters.Get ("stampRevGuid", stampRevGuidStr);
    const API_Guid explicitStampMainGuid = hasExplicitStamp ? APIGuidFromString (stampMainGuidStr.ToCStr ()) : APINULLGuid;

    API_Attribute templateAttr = {};
    bool hasTemplate = false;
    if (IsPositiveAttributeIndex (attribute.header.index)) {
        templateAttr.header.typeID = API_ZoneCatID;
        templateAttr.header.index = attribute.header.index;
        hasTemplate = (ACAPI_Attribute_Get (&templateAttr) == NoError);
    }
    if (!hasTemplate) {
        // Prefer a Zone Category that already uses the requested stamp: its parameters belong to that stamp.
        GS::Array<API_Attribute> zoneCategories;
        ACAPI_Attribute_GetAttributesByType (API_ZoneCatID, zoneCategories);
        for (const API_Attribute& zoneCategory : zoneCategories) {
            const bool usesRequestedStamp = hasExplicitStamp && zoneCategory.zoneCat.stampMainGuid == explicitStampMainGuid;
            if (!hasTemplate || usesRequestedStamp) {
                templateAttr = zoneCategory;
                hasTemplate = true;
            }
            if (!hasExplicitStamp || usesRequestedStamp) {
                break;
            }
        }
    }
    if (!hasTemplate) {
        return CreateErrorResponse (APIERR_BADPARS, "The project has no Zone Category to copy the zone stamp and its parameters from.");
    }

    GS::ucscpy (attribute.zoneCat.stampName, templateAttr.zoneCat.stampName);
    attribute.zoneCat.stampMainGuid = templateAttr.zoneCat.stampMainGuid;
    attribute.zoneCat.stampRevGuid = templateAttr.zoneCat.stampRevGuid;

    const GSErrCode defErr = ACAPI_Attribute_GetDef (API_ZoneCatID, templateAttr.header.index, &attributeDef);
    if (defErr != NoError || attributeDef.zone_addParItems == nullptr) {
        return CreateErrorResponse (defErr != NoError ? defErr : APIERR_GENERAL,
            GS::UniString::Printf ("Failed to get the zone stamp parameters of Zone Category '%T' to copy them (is its zone stamp in the loaded libraries?).", GetAttributeName (templateAttr.header).ToPrintf ()));
    }

    // Explicit stamp reference (e.g. copied from another Zone Category's stampMainGuid/stampRevGuid/stampName,
    // as shown by GetZoneCategories or in an Attribute Manager XML export) overrides the template's stamp. When
    // the template does not use that stamp, its parameters do not belong to it either, so they are replaced with
    // the stamp library part's own defaults; if that library part cannot be found the template's parameters are
    // kept, as before.
    if (hasExplicitStamp) {
        attribute.zoneCat.stampMainGuid = explicitStampMainGuid;
        attribute.zoneCat.stampRevGuid = APIGuidFromString (stampRevGuidStr.ToCStr ());
        const bool hasStampName = SetUCharProperty (&parameters, "stampName", attribute.zoneCat.stampName);

        if (templateAttr.zoneCat.stampMainGuid != explicitStampMainGuid) {
            const GS::uchar_t* stampName = hasStampName ? attribute.zoneCat.stampName : nullptr;
            API_AddParType** stampParams = GetZoneStampParameters (attribute.zoneCat.stampMainGuid, attribute.zoneCat.stampRevGuid, stampName);
            if (stampParams != nullptr) {
                ACAPI_DisposeAttrDefsHdls (&attributeDef);
                attributeDef.zone_addParItems = stampParams;
            }
        }
    }

    SetUCharProperty (&parameters, "categoryCode", attribute.zoneCat.catCode);
    GetColor (parameters, "color", attribute.zoneCat.rgb);

    return {};
}

CreateMEPSystemsCommand::CreateMEPSystemsCommand () :
    CreateAttributesCommandBase ("CreateMEPSystems", API_MEPSystemID, "mepSystemDataArray")
{
}

GS::Optional<GS::UniString> CreateMEPSystemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "mepSystemDataArray": {
                "type": "array",
                "description" : "Array of data to create new MEP Systems.",
                "items": {
                    "type": "object",
                    "description": "Data to create an MEP System.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing MEP System to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing MEP System to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing MEP System with the given name will be overwritten."
                        },
                        "domain": {
                            "$ref": "#/MEPDomain",
                            "description": "Only elements belonging to the same domain can use this system."
                        },
                        "contourPen": {
                            "type": "integer",
                            "description": "The index of the contour pen [1..255]."
                        },
                        "fillPen": {
                            "type": "integer",
                            "description": "The index of the fill (foreground) pen [1..255]."
                        },
                        "fillBackgroundPen": {
                            "type": "integer",
                            "description": "The index of the background pen [0..255]. 0 means transparent background."
                        },
                        "centerLinePen": {
                            "type": "integer",
                            "description": "The index of the center line pen [1..255]."
                        },
                        "fillId": {
                            "description": "Identifier of the fill pattern attribute.",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "centerLineTypeId": {
                            "description": "Identifier of the center line type attribute.",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "materialId": {
                            "description": "Identifier of the surface (material) attribute applied to the 3D model of the elements of the system.",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "insulationMaterialId": {
                            "description": "Identifier of the surface (material) attribute applied to the insulation of the elements of the system.",
                            "$ref": "#/AttributeIdArrayItem"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the MEP System if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "mepSystemDataArray"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateMEPSystemsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
{
    GS::UniString domain;
    if (parameters.Get ("domain", domain)) {
#ifdef ServerMainVers_2900
        if (domain == "Piping")
            attribute.mepSystem.domain = APIMEPDomain_Piping;
        else if (domain == "CableCarrier")
            attribute.mepSystem.domain = APIMEPDomain_CableCarrier;
        else
            attribute.mepSystem.domain = APIMEPDomain_Ventilation;
#else
        // Before AC29, API_MEPSystemType has no single domain enum - it allows multiple domains at once via
        // independent booleans. Only the requested domain's flag is touched, so a modify call doesn't
        // silently clear the other domains of an existing multi-domain MEP System.
        if (domain == "Ventilation")
            attribute.mepSystem.isForDuctwork = true;
        else if (domain == "Piping")
            attribute.mepSystem.isForPipework = true;
        else if (domain == "CableCarrier")
            attribute.mepSystem.isForCabling = true;
#endif
    }

    parameters.Get ("contourPen", attribute.mepSystem.contourPen);
    parameters.Get ("fillPen", attribute.mepSystem.fillPen);
    parameters.Get ("fillBackgroundPen", attribute.mepSystem.fillBgPen);
    parameters.Get ("centerLinePen", attribute.mepSystem.centerLinePen);

    GS::ObjectState fillId;
    if (parameters.Get ("fillId", fillId)) {
        API_AttributeIndex fillIndex;
        if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, fillIndex)) {
            attribute.mepSystem.fillInd = fillIndex;
        }
    }

    GS::ObjectState centerLineTypeId;
    if (parameters.Get ("centerLineTypeId", centerLineTypeId)) {
        API_AttributeIndex lineTypeIndex;
        if (GetAttributeIndexFromAttributeId (centerLineTypeId, API_LinetypeID, lineTypeIndex)) {
            attribute.mepSystem.centerLTypeInd = lineTypeIndex;
        }
    }

    GS::ObjectState materialId;
    if (parameters.Get ("materialId", materialId)) {
        API_AttributeIndex materialIndex;
        if (GetAttributeIndexFromAttributeId (materialId, API_MaterialID, materialIndex)) {
            attribute.mepSystem.materialInd = materialIndex;
        }
    }

    GS::ObjectState insulationMaterialId;
    if (parameters.Get ("insulationMaterialId", insulationMaterialId)) {
        API_AttributeIndex insulationMaterialIndex;
        if (GetAttributeIndexFromAttributeId (insulationMaterialId, API_MaterialID, insulationMaterialIndex)) {
            attribute.mepSystem.insulationMatInd = insulationMaterialIndex;
        }
    }

    return {};
}

CreateSurfacesCommand::CreateSurfacesCommand () :
    CreateAttributesCommandBase ("CreateSurfaces", API_MaterialID, "surfaceDataArray")
{
}

GS::Optional<GS::UniString> CreateSurfacesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "surfaceDataArray": {
                "type": "array",
                "description" : "Array of data to create new surfaces.",
                "items": {
                    "type": "object",
                    "description": "Data to create a surface.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Surface to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing surface to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing surface with the given name will be overwritten."
                        },
                        "materialType": {
                            "$ref": "#/SurfaceType"
                        },
                        "ambientReflection": {
                            "type": "number",
                            "description": "Ambient percentage [0..100]."
                        },
                        "diffuseReflection": {
                            "type": "number",
                            "description": "Diffuse percentage [0..100]."
                        },
                        "specularReflection": {
                            "type": "number",
                            "description": "Specular percentage [0..100]."
                        },
                        "transparency": {
                            "type": "number",
                            "description": "Transparency percentage [0..100]."
                        },
                        "shine": {
                            "type": "number",
                            "description": "The shininess factor multiplied by 100 [0..10000]."
                        },
                        "transparencyAttenuation": {
                            "type": "number",
                            "description": "Transparency attenuation multiplied by 100 [0..10000]."
                        },
                        "emissionAttenuation": {
                            "type": "number",
                            "description": "Emission attenuation multiplied by 100 [0..10000]."
                        },
                        "surfaceColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "specularColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "emissionColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "fillId": {
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "texture": {
                            "$ref": "#/Texture"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Surface if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "surfaceDataArray"
        ]
    })";
}

GS::Optional<GS::ObjectState> CreateSurfacesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
{
    GS::UniString typeStr;
    if (parameters.Get ("materialType", typeStr)) {
        if (typeStr == "Matte")
            attribute.material.mtype = APIMater_MatteID;
        else if (typeStr == "Metal")
            attribute.material.mtype = APIMater_MetalID;
        else if (typeStr == "Plastic")
            attribute.material.mtype = APIMater_PlasticID;
        else if (typeStr == "Glass")
            attribute.material.mtype = APIMater_GlassID;
        else if (typeStr == "Glowing")
            attribute.material.mtype = APIMater_GlowingID;
        else if (typeStr == "Constant")
            attribute.material.mtype = APIMater_ConstID;
        else if (typeStr == "Simple")
            attribute.material.mtype = APIMater_SimpleID;
        else
            attribute.material.mtype = APIMater_GeneralID;
    }

    // Extracted as double, not short, even though the underlying API_MaterialType fields are short: the JSON
    // schema types these fields "number" (matching the rest of this codebase's convention for numeric fields),
    // and GS::ObjectState::Get<short>() silently fails (leaves the local untouched, "if" never entering) against
    // a JSON number stored internally as double - none of these fields were ever actually applied before this
    // fix, confirmed live (a Surface created with ambientReflection/shine/etc. all came back 0 regardless of
    // what was sent).
    double ambientReflection;
    if (parameters.Get ("ambientReflection", ambientReflection)) {
        attribute.material.ambientPc = (short) ambientReflection;
    }

    double diffuseReflection;
    if (parameters.Get ("diffuseReflection", diffuseReflection)) {
        attribute.material.diffusePc = (short) diffuseReflection;
    }

    double specularReflection;
    if (parameters.Get ("specularReflection", specularReflection)) {
        attribute.material.specularPc = (short) specularReflection;
    }

    double transparency;
    if (parameters.Get ("transparency", transparency)) {
        attribute.material.transpPc = (short) transparency;
    }

    double shine;
    if (parameters.Get ("shine", shine)) {
        attribute.material.shine = (short) shine;
    }

    double transparencyAttenuation;
    if (parameters.Get ("transparencyAttenuation", transparencyAttenuation)) {
        attribute.material.transpAtt = (short) transparencyAttenuation;
    }

    double emissionAttenuation;
    if (parameters.Get ("emissionAttenuation", emissionAttenuation)) {
        attribute.material.emissionAtt = (short) emissionAttenuation;
    }

    GetColor (parameters, "surfaceColor", attribute.material.surfaceRGB);
    GetColor (parameters, "specularColor", attribute.material.specularRGB);
    GetColor (parameters, "emissionColor", attribute.material.emissionRGB);

    GS::ObjectState fillId;
    if (parameters.Get ("fillId", fillId)) {
        API_AttributeIndex fillIndex;
        if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, fillIndex)) {
            attribute.material.ifill = fillIndex;
        }
    }

    GS::ObjectState textureObj;
    if (parameters.Get ("texture", textureObj)) {
        attribute.material.texture.status |= APITxtr_LinkMat;
        SetUCharProperty(&textureObj, "name", attribute.material.texture.texName);

        double rotationAngle;
        if (textureObj.Get ("rotationAngle", rotationAngle)) {
            attribute.material.texture.rotAng = rotationAngle;
        }
        double xSize;
        if (textureObj.Get ("xSize", xSize)) {
            attribute.material.texture.xSize = xSize;
        }
        double ySize;
        if (textureObj.Get ("ySize", ySize)) {
            attribute.material.texture.ySize = ySize;
        }
        bool fillRectangle;
        if (textureObj.Get ("FillRectangle", fillRectangle) && fillRectangle) {
            attribute.material.texture.status |= APITxtr_FillRectNatur;
        }
        bool fitPicture;
        if (textureObj.Get ("FitPicture", fitPicture) && fitPicture) {
            attribute.material.texture.status |= APITxtr_FitPictNatur;
        }
        bool mirrorX;
        if (textureObj.Get ("mirrorX", mirrorX) && mirrorX) {
            attribute.material.texture.status |= APITxtr_MirrorX;
        }
        bool mirrorY;
        if (textureObj.Get ("mirrorY", mirrorY) && mirrorY) {
            attribute.material.texture.status |= APITxtr_MirrorY;
        }
        bool useAlphaChannel;
        if (textureObj.Get ("useAlphaChannel", useAlphaChannel) && useAlphaChannel) {
            attribute.material.texture.status |= APITxtr_UseAlpha;
        }
        bool alphaChannelChangesTransparency;
        if (textureObj.Get ("alphaChannelChangesTransparency", alphaChannelChangesTransparency) && alphaChannelChangesTransparency) {
            attribute.material.texture.status |= APITxtr_TransPattern;
        }
        bool alphaChannelChangesSurfaceColor;
        if (textureObj.Get ("alphaChannelChangesSurfaceColor", alphaChannelChangesSurfaceColor) && alphaChannelChangesSurfaceColor) {
            attribute.material.texture.status |= APITxtr_SurfacePattern;
        }
        bool alphaChannelChangesAmbientColor;
        if (textureObj.Get ("alphaChannelChangesAmbientColor", alphaChannelChangesAmbientColor) && alphaChannelChangesAmbientColor) {
            attribute.material.texture.status |= APITxtr_AmbientPattern;
        }
        bool alphaChannelChangesSpecularColor;
        if (textureObj.Get ("alphaChannelChangesSpecularColor", alphaChannelChangesSpecularColor) && alphaChannelChangesSpecularColor) {
            attribute.material.texture.status |= APITxtr_SpecularPattern;
        }
        bool alphaChannelChangesDiffuseColor;
        if (textureObj.Get ("alphaChannelChangesDiffuseColor", alphaChannelChangesDiffuseColor) && alphaChannelChangesDiffuseColor) {
            attribute.material.texture.status |= APITxtr_DiffusePattern;
        }
    }

    return {};
}

CreatePenTablesCommand::CreatePenTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreatePenTablesCommand::GetName () const
{
    return "CreatePenTables";
}

GS::Optional<GS::UniString> CreatePenTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "penTableDataArray": {
                "type": "array",
                "description" : "Array of data to create new Pen Tables.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Pen Table.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Pen Table to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Pen Table to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Pen Table with the given name will be overwritten."
                        },
                        "isActiveForModel": {
                            "type": "boolean",
                            "description": "Make this the active Pen Table for the model window. Defaults to false for a new Pen Table, or to the current value when overwriting an existing one."
                        },
                        "isActiveForLayout": {
                            "type": "boolean",
                            "description": "Make this the active Pen Table for layouts. Defaults to false for a new Pen Table, or to the current value when overwriting an existing one."
                        },
                        "sourceAttributeId": {
                            "description": "Identifier of the Pen Table whose 255 pens are used as the starting point, before the pens listed in the pens array are applied on top. Defaults to the Pen Table being overwritten itself (so unlisted pens keep their current color/width/description), or an arbitrary existing Pen Table in the project when creating a brand new one (or a plain black, 0.1 mm pen for all 255 if the project has no Pen Table at all yet).",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "pens": {
                            "type": "array",
                            "description": "The pens to set in the Pen Table, on top of the 255 pens copied from sourceAttributeId (or the current Pen Table, or an arbitrary existing one - see sourceAttributeId). Only list the pens you actually want to change.",
                            "items": {
                                "$ref": "#/PenData"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Pen Table if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "penTableDataArray"
        ]
    })";
}

GS::Optional<GS::UniString> CreatePenTablesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

#ifdef ServerMainVers_2700
GS::ObjectState CreatePenTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> penTableDataArray;
    parameters.Get ("penTableDataArray", penTableDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& penTableData : penTableDataArray) {
        API_Attribute penTable = {};
        API_AttributeDefExt penTableDefs = {};
        penTable.header.typeID = API_PenTableID;

        GS::UniString name;
        if (penTableData.Get ("name", name)) {
            penTable.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            penTable.header.guid = GetGuidFromAttributesArrayItem (penTableData);

            Int32 index = -1;
            if (penTableData.Get ("index", index) && index >= 0) {
                penTable.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&penTable) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (APIERR_ATTREXIST, "Already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr, clobbering a requested rename with the OLD name before Modify ever sees
        // it - same bug as CreateAttributesCommandBase, fixed there this session; re-apply here too.
        if (penTableData.Get ("name", name)) {
            penTable.header.uniStringNamePtr = &name;
        }

        penTableData.Get ("isActiveForModel", penTable.penTable.inEffectForModel);
        penTableData.Get ("isActiveForLayout", penTable.penTable.inEffectForLayout);

        // Every Pen Table always contains all 255 pens, so the ones not explicitly listed by the caller need a
        // starting value from somewhere. Despite ACAPI_Attribute_GetDefExt's documentation claiming it only
        // supports lines/fills/composites/layers/zone categories, it works for Pen Tables too (confirmed
        // empirically) - so we use it to seed the full 255-pen array from a real source: the Pen Table itself
        // when overwriting (preserves everything not explicitly changed), the explicit sourceAttributeId when
        // given, or an arbitrary existing Pen Table in the project as a last resort for a brand new one.
        API_AttributeIndex sourceIndex = APIInvalidAttributeIndex;
        GS::ObjectState sourceAttributeId;
        if (penTableData.Get ("sourceAttributeId", sourceAttributeId)) {
            if (!GetAttributeIndexFromAttributeId (sourceAttributeId, API_PenTableID, sourceIndex)) {
                attributeIds (CreateErrorResponse (APIERR_BADID, "Source Pen Table not found."));
                continue;
            }
        } else if (doesExist) {
            sourceIndex = penTable.header.index;
        } else {
            GS::Array<API_Attribute> existingPenTables;
            ACAPI_Attribute_GetAttributesByType (API_PenTableID, existingPenTables);
            if (!existingPenTables.IsEmpty ()) {
                sourceIndex = existingPenTables[0].header.index;
            }
            for (API_Attribute& existingPenTable : existingPenTables) {
                DisposeAttribute (existingPenTable);
            }
        }

        penTableDefs.penTable_Items = new GS::Array<API_Pen> ();
        if (sourceIndex.IsPositive ()) {
            API_AttributeDefExt sourceDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_PenTableID, sourceIndex, &sourceDefs) == NoError && sourceDefs.penTable_Items != nullptr) {
                for (const API_Pen& sourcePen : *sourceDefs.penTable_Items) {
                    penTableDefs.penTable_Items->Push (sourcePen);
                }
                ACAPI_DisposeAttrDefsHdlsExt (&sourceDefs);
            }
        }
        for (short i = (short) penTableDefs.penTable_Items->GetSize (); i < 255; ++i) {
            API_Pen pen = {};
            pen.index = i + 1;
            pen.rgb = { 0.0, 0.0, 0.0 };
            pen.width = 0.1;
            penTableDefs.penTable_Items->Push (pen);
        }

        GS::Array<GS::ObjectState> pens;
        penTableData.Get ("pens", pens);
        for (const GS::ObjectState& penData : pens) {
            Int32 penIndex = 0;
            penData.Get ("index", penIndex);
            if (penIndex < 1 || penIndex > 255) {
                continue;
            }

            API_Pen& pen = (*penTableDefs.penTable_Items)[penIndex - 1];
            GetColor (penData, "color", pen.rgb);
            penData.Get ("width", pen.width);
            SetCharProperty (&penData, "description", pen.description);
        }

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_ModifyExt (&penTable, &penTableDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify."));
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_CreateExt (&penTable, &penTableDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create."));
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
                continue;
            }
        }

        attributeIds (CreateAttributeIdObjectState (penTable.header.guid));
        ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
    }

    return response;
}
#else
// Before AC27, the pen array element type is API_PenType (not API_Pen): the pen index lives in head.index
// (not a plain index field), description is a plain char[128] (not GS::uchar_t[128]), and penTable_Items is an
// old-style handle of 255 contiguous elements (not a GS::Array). Otherwise the logic mirrors the AC27+ version
// above exactly, including the auto-seeding of unlisted pens from an existing Pen Table.
GS::ObjectState CreatePenTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> penTableDataArray;
    parameters.Get ("penTableDataArray", penTableDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& penTableData : penTableDataArray) {
        API_Attribute penTable = {};
        API_AttributeDefExt penTableDefs = {};
        penTable.header.typeID = API_PenTableID;

        GS::UniString name;
        if (penTableData.Get ("name", name)) {
            penTable.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            penTable.header.guid = GetGuidFromAttributesArrayItem (penTableData);

            Int32 index = -1;
            if (penTableData.Get ("index", index) && index >= 0) {
                penTable.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&penTable) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (APIERR_ATTREXIST, "Already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr, clobbering a requested rename with the OLD name before Modify ever sees
        // it - same bug as CreateAttributesCommandBase, fixed there this session; re-apply here too.
        if (penTableData.Get ("name", name)) {
            penTable.header.uniStringNamePtr = &name;
        }

        // isActiveForModel/isActiveForLayout have no equivalent in the pre-AC27 API_PenTableType, so they are
        // silently ignored on these older versions.

        API_AttributeIndex sourceIndex = APIInvalidAttributeIndex;
        GS::ObjectState sourceAttributeId;
        if (penTableData.Get ("sourceAttributeId", sourceAttributeId)) {
            if (!GetAttributeIndexFromAttributeId (sourceAttributeId, API_PenTableID, sourceIndex)) {
                attributeIds (CreateErrorResponse (APIERR_BADID, "Source Pen Table not found."));
                continue;
            }
        } else if (doesExist) {
            sourceIndex = penTable.header.index;
        } else {
            GS::Array<API_Attribute> existingPenTables;
            ACAPI_Attribute_GetAttributesByType (API_PenTableID, existingPenTables);
            if (!existingPenTables.IsEmpty ()) {
                sourceIndex = existingPenTables[0].header.index;
            }
            for (API_Attribute& existingPenTable : existingPenTables) {
                DisposeAttribute (existingPenTable);
            }
        }

        penTableDefs.penTable_Items = (API_PenType**) BMAllocateHandle (255 * sizeof (API_PenType), ALLOCATE_CLEAR, 0);

        bool seeded = false;
        if (IsPositiveAttributeIndex (sourceIndex)) {
            API_AttributeDefExt sourceDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_PenTableID, sourceIndex, &sourceDefs) == NoError && sourceDefs.penTable_Items != nullptr) {
                for (UInt32 i = 0; i < 255; ++i) {
                    (*penTableDefs.penTable_Items)[i] = (*sourceDefs.penTable_Items)[i];
                }
                seeded = true;
                ACAPI_DisposeAttrDefsHdlsExt (&sourceDefs);
            }
        }
        if (!seeded) {
            for (short i = 0; i < 255; ++i) {
                API_PenType& pen = (*penTableDefs.penTable_Items)[i];
                pen.head.index = ACAPI_CreateAttributeIndex (i + 1);
                pen.width = 0.1;
            }
        }

        GS::Array<GS::ObjectState> pens;
        penTableData.Get ("pens", pens);
        for (const GS::ObjectState& penData : pens) {
            Int32 penIndex = 0;
            penData.Get ("index", penIndex);
            if (penIndex < 1 || penIndex > 255) {
                continue;
            }

            API_PenType& pen = (*penTableDefs.penTable_Items)[penIndex - 1];
            GetColor (penData, "color", pen.rgb);
            penData.Get ("width", pen.width);
            SetCharProperty (&penData, "description", pen.description);
        }

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_ModifyExt (&penTable, &penTableDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify."));
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_CreateExt (&penTable, &penTableDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create."));
                ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
                continue;
            }
        }

        attributeIds (CreateAttributeIdObjectState (penTable.header.guid));
        ACAPI_DisposeAttrDefsHdlsExt (&penTableDefs);
    }

    return response;
}
#endif

CreateProfilesCommand::CreateProfilesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateProfilesCommand::GetName () const
{
    return "CreateProfiles";
}

GS::Optional<GS::UniString> CreateProfilesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "profileDataArray": {
                "type": "array",
                "description" : "Array of data to create new Profiles.",
                "items": {
                    "type": "object",
                    "description": "Data to create or modify a Profile. Its geometry (the cross-section shape) comes from sourceAttributeId (an existing Profile's geometry, copied), from newSkins (AC27+ only, caller-supplied polygon geometry), or both combined. When creating a brand-new Profile (overwriteExisting false, or true but no existing match), at least one of the two must be given. When overwriteExisting targets an existing Profile, both are optional - the existing Profile's own current geometry is preserved by default (e.g. to change only wallType, or to add newSkins on top of the unchanged existing shape). skinOverrides and newSkins' edgeOverrides target skins/edges by the identifiers/indices GetProfiles reports.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Profile to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Profile to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Profile with the given name will be overwritten."
                        },
                        "sourceAttributeId": {
                            "description": "Identifier of an existing Profile whose geometry (cross-section shape) will be copied as the starting point. Optional if newSkins is given: omit it to build the Profile's geometry entirely from newSkins instead of copying anything (AC27+ only).",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "wallType": {
                            "type": "boolean",
                            "description": "Profile available for walls. Defaults to the source Profile's value."
                        },
                        "beamType": {
                            "type": "boolean",
                            "description": "Profile available for beams. Defaults to the source Profile's value."
                        },
                        "coluType": {
                            "type": "boolean",
                            "description": "Profile available for columns. Defaults to the source Profile's value."
                        },
                        "handrailType": {
                            "type": "boolean",
                            "description": "Profile available for handrails. Defaults to the source Profile's value."
                        },
                        "otherGDLObjectType": {
                            "type": "boolean",
                            "description": "Profile available for other GDL based objects. Defaults to the source Profile's value."
                        },
                        "skinOverrides": {
                            "type": "array",
                            "description": "Modifications to apply to specific skins of the copied geometry, e.g. to change a skin's building material without rebuilding the profile's shape. Each skinId comes from a prior GetProfiles call's skins[].skinId on the source Profile (or, when overwriteExisting is true, on the Profile being overwritten).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "skinId": {
                                        "type": "string",
                                        "description": "Identifies which skin to modify, from GetProfiles' skins[].skinId."
                                    },
                                    "buildingMaterialId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "surfaceId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "fillId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "contourPen": {
                                        "type": "integer"
                                    },
                                    "contourLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "isCore": {
                                        "type": "boolean"
                                    },
                                    "isFinish": {
                                        "type": "boolean"
                                    },
                                    "visibleCutEndLines": {
                                        "type": "boolean"
                                    },
                                    "cutEndLinePen": {
                                        "type": "integer"
                                    },
                                    "cutEndLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "edgeOverrides": {
                                        "type": "array",
                                        "description": "Modifications to specific edges of this skin, targeted by their position (0-based) in GetProfiles' skins[].edges.",
                                        "items": {
                                            "$ref": "#/ProfileEdgeOverride"
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["skinId"]
                            }
                        },
                        "newSkins": {
                            "type": "array",
                            "description": "AC27+ only. Adds brand-new skins built from caller-supplied polygon geometry, instead of (or in addition to) whatever was copied from sourceAttributeId. Combine with sourceAttributeId to add skins to a copied Profile, or omit sourceAttributeId to build a Profile's geometry entirely from newSkins.",
                            "items": {
                                "$ref": "#/ProfileSkinData"
                            }
                        },
                        "replaceSkins": {
                            "type": "boolean",
                            "description": "AC27+ only. When overwriteExisting targets an existing Profile, discard every one of its existing skins (and any sourceAttributeId geometry given alongside it) before applying newSkins, instead of adding newSkins on top of the preserved existing geometry. Use this to fully replace a Profile's cross-section with a caller-authored shape while keeping its guid and scalar fields (wallType etc.) - e.g. to sync a Profile's geometry from another project, where sourceAttributeId can't be used because it only resolves within the same file. Also discards any existing profileModifiers, since those are tied to the specific geometry being replaced. Ignored when creating a brand-new Profile (there is nothing to discard yet)."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Profile if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "profileDataArray"
        ]
    })";
}

GS::Optional<GS::UniString> CreateProfilesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::ObjectState CreateProfilesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> profileDataArray;
    parameters.Get ("profileDataArray", profileDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& profileData : profileDataArray) {
        API_Attribute profile = {};
        profile.header.typeID = API_ProfileID;

        GS::UniString name;
        if (profileData.Get ("name", name)) {
            profile.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            profile.header.guid = GetGuidFromAttributesArrayItem (profileData);

            Int32 index = -1;
            if (profileData.Get ("index", index) && index >= 0) {
                profile.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&profile) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (APIERR_ATTREXIST, "Already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr, clobbering a requested rename with the OLD name before Modify ever sees
        // it - same bug as CreateAttributesCommandBase, fixed there this session; re-apply here too.
        if (profileData.Get ("name", name)) {
            profile.header.uniStringNamePtr = &name;
        }

        GS::ObjectState sourceAttributeId;
        bool hasSource = profileData.Get ("sourceAttributeId", sourceAttributeId);

        GS::Array<GS::ObjectState> newSkins;
        profileData.Get ("newSkins", newSkins);

#ifndef ServerMainVers_2700
        if (!hasSource) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "sourceAttributeId is required on this Archicad version."));
            continue;
        }
        if (!newSkins.IsEmpty ()) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "newSkins requires Archicad 27 or later."));
            continue;
        }
#else
        if (!hasSource && newSkins.IsEmpty () && !doesExist) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "Either sourceAttributeId or newSkins (or both) is required when creating a new Profile."));
            continue;
        }
#endif

        API_AttributeDefExt sourceDefs = {};
        bool hasSourceDefs = false;
        if (hasSource) {
            API_AttributeIndex sourceIndex;
            if (!GetAttributeIndexFromAttributeId (sourceAttributeId, API_ProfileID, sourceIndex)) {
                attributeIds (CreateErrorResponse (APIERR_BADID, "Source Profile not found."));
                continue;
            }

            API_Attribute sourceAttr = {};
            sourceAttr.header.typeID = API_ProfileID;
            sourceAttr.header.index = sourceIndex;
            if (ACAPI_Attribute_Get (&sourceAttr) != NoError) {
                attributeIds (CreateErrorResponse (APIERR_BADID, "Source Profile not found."));
                continue;
            }

            if (ACAPI_Attribute_GetDefExt (API_ProfileID, sourceIndex, &sourceDefs) != NoError) {
                attributeIds (CreateErrorResponse (Error, "Failed to read source Profile geometry."));
                continue;
            }
            hasSourceDefs = true;

            profile.profile.wallType = sourceAttr.profile.wallType;
            profile.profile.beamType = sourceAttr.profile.beamType;
            profile.profile.coluType = sourceAttr.profile.coluType;
            profile.profile.handrailType = sourceAttr.profile.handrailType;
            profile.profile.otherGDLObjectType = sourceAttr.profile.otherGDLObjectType;
        } else if (doesExist) {
            // Pure metadata-only modify (e.g. just wallType), or newSkins added on top of the existing geometry,
            // with no sourceAttributeId given: preserve the Profile's own current geometry instead of requiring
            // a redundant self-reference. Scalar fields (wallType etc.) are already correctly preserved by the
            // ACAPI_Attribute_Get existence check above (it populates `profile` in place from the existing
            // attribute) - only the Ext geometry handles need explicit re-seeding here, same root cause and fix
            // as the CreateFills/CreateLines Ext-geometry-preserve bug found via exhaustive testing.
            if (ACAPI_Attribute_GetDefExt (API_ProfileID, profile.header.index, &sourceDefs) == NoError) {
                hasSourceDefs = true;
            }
        }

        profileData.Get ("wallType", profile.profile.wallType);
        profileData.Get ("beamType", profile.profile.beamType);
        profileData.Get ("coluType", profile.profile.coluType);
        profileData.Get ("handrailType", profile.profile.handrailType);
        profileData.Get ("otherGDLObjectType", profile.profile.otherGDLObjectType);

        // Geometry starts as whatever was copied from sourceAttributeId (nullptr if none was given).
        // Individual copied skins can be retargeted via skinOverrides (ApplyProfileSkinOverrides mutates
        // profile_vectorImageItems in place), and newSkins (AC27+ only) adds brand-new, caller-authored
        // skins on top - to either the copied image, or to scratchImage if there was nothing to copy.
        API_AttributeDefExt profileDefs = {};
        profileDefs.profile_vectorImageItems = sourceDefs.profile_vectorImageItems;
        profileDefs.profile_vectorImageParameterNames = sourceDefs.profile_vectorImageParameterNames;

#ifdef ServerMainVers_2700
        ProfileVectorImage scratchImage;
        GS::HashTable<PVI::ProfileParameterId, GS::UniString> emptyParameterNames;
        bool replaceSkins = false;
        profileData.Get ("replaceSkins", replaceSkins);
        if (!hasSourceDefs || replaceSkins) {
            // Nothing to preserve (brand-new Profile relying solely on newSkins, the self-seed GetDefExt above
            // failed, or the caller explicitly asked to discard every existing skin via replaceSkins) - start
            // from a genuinely empty image instead of a null geometry pointer. replaceSkins also drops any
            // existing profileModifiers (profile_vectorImageParameterNames): a dimension-control-tool modifier
            // is tied to specific vertices/edges of the OLD geometry, so keeping it around once that geometry
            // is fully discarded would leave it dangling against a completely different hatch set.
            //
            // Crash found live (2026-07-12): passing nullptr here to mean "no modifiers" reliably crashed
            // Archicad when the existing Profile actually had modifiers - null apparently means "leave the
            // existing parameter table untouched" (matching this whole module's general null-means-preserve
            // convention), not "clear it", so Archicad kept the OLD dimension-control-tools around and crashed
            // trying to reconcile them against the brand-new, structurally unrelated geometry. Passing a real
            // but EMPTY table explicitly communicates "zero modifiers" instead, which does not crash.
            profileDefs.profile_vectorImageItems = &scratchImage;
            profileDefs.profile_vectorImageParameterNames = &emptyParameterNames;
        }
#endif

        GS::Array<GS::ObjectState> skinOverrides;
        if (profileData.Get ("skinOverrides", skinOverrides) && profileDefs.profile_vectorImageItems != nullptr) {
            for (const GS::ObjectState& skinOverride : skinOverrides) {
                ApplyProfileSkinOverrides (*profileDefs.profile_vectorImageItems, skinOverride);
            }
        }

#ifdef ServerMainVers_2700
        if (!newSkins.IsEmpty () && profileDefs.profile_vectorImageItems != nullptr) {
            for (const GS::ObjectState& skinDef : newSkins) {
                HatchObject hatch;
                if (!BuildHatchFromSkinDefinition (skinDef, hatch)) {
                    continue;
                }
                Sy_HatchType hatchRef;
                profileDefs.profile_vectorImageItems->AddHatch (hatchRef, hatch);
            }
        }
#endif

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_ModifyExt (&profile, &profileDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify."));
                if (hasSourceDefs) {
                    ACAPI_DisposeAttrDefsHdlsExt (&sourceDefs);
                }
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_CreateExt (&profile, &profileDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create."));
                if (hasSourceDefs) {
                    ACAPI_DisposeAttrDefsHdlsExt (&sourceDefs);
                }
                continue;
            }
        }

        attributeIds (CreateAttributeIdObjectState (profile.header.guid));
        if (hasSourceDefs) {
            ACAPI_DisposeAttrDefsHdlsExt (&sourceDefs);
        }
    }

    return response;
}

CreateCompositesCommand::CreateCompositesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateCompositesCommand::GetName () const
{
    return "CreateComposites";
}

GS::Optional<GS::UniString> CreateCompositesCommand::GetInputParametersSchema () const
{
    return R"SCHEMA({
        "type": "object",
        "properties": {
            "compositeDataArray": {
                "type": "array",
                "description" : "Array of data to create Composites.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Composite.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Composite to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Composite to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Composite with the given name will be overwritten."
                        },
                        "useWith": {
                            "type": "array",
                            "description" : "Array of types the composite can used with. If omitted when overwriting an existing Composite, its current setting is kept.",
                            "items": {
                                "type": "string",
                                "description": "Element type (Wall, Slab, Roof, or Shell)"
                            }
                        },
                        "skins": {
                            "type": "array",
                            "description" : "Array of skin data. Required when creating a new Composite. If omitted when overwriting an existing Composite, its current skins are kept.",
                            "items" : {
                                "type": "object",
                                "description" : "Data to represent a skin.",
                                "properties" : {
                                    "type": {
                                        "type": "string",
                                        "description" : "Skin type (Core, Finish, or Other)"
                                    },
                                    "buildingMaterialId" : {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "framePen" : {
                                        "type": "integer",
                                        "description" : "Skin frame pen index."
                                    },
                                    "thickness" : {
                                        "type": "number",
                                        "description" : "Skin thickness (in meters)."
                                    }
                                },
                                "additionalProperties": false,
                                "required" : [
                                    "type",
                                    "buildingMaterialId",
                                    "framePen",
                                    "thickness"
                                ]
                            }
                        },
                        "separators": {
                            "type": "array",
                            "description" : "Array of skin separator data. The number of items must be the number of skins plus one. Required when creating a new Composite. If omitted when overwriting an existing Composite, its current separators are kept (only possible if the number of skins does not change).",
                            "items" : {
                                "type": "object",
                                "description" : "Data to represent a skin separator.",
                                "properties" : {
                                    "lineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "linePen" : {
                                        "type": "integer",
                                            "description" : "Separator line pen index."
                                    }
                                },
                                "additionalProperties": false,
                                "required" : [
                                    "lineTypeId",
                                    "linePen"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Composite if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required" : [
            "compositeDataArray"
        ]
    })SCHEMA";
}

GS::Optional<GS::UniString> CreateCompositesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::ObjectState CreateCompositesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> compositeDataArray;
    parameters.Get ("compositeDataArray", compositeDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& compositeData : compositeDataArray) {
        API_Attribute composite = {};
        API_AttributeDefExt	compositeDefs = {};
        composite.header.typeID = API_CompWallID;

        GS::UniString name;
        if (compositeData.Get ("name", name)) {
            composite.header.uniStringNamePtr = &name;
        }

        if (overwriteExisting) {
            composite.header.guid = GetGuidFromAttributesArrayItem (compositeData);

            Int32 index = -1;
            if (compositeData.Get ("index", index) && index >= 0) {
                composite.header.index = ACAPI_CreateAttributeIndex (index);
            }
        }

        bool doesExist = (ACAPI_Attribute_Get (&composite) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (Error, "Composite already exists."));
            continue;
        }

        // ACAPI_Attribute_Get writes the FOUND attribute's current name back through
        // uniStringNamePtr, clobbering a requested rename with the OLD name before Modify ever sees
        // it - same bug as CreateAttributesCommandBase, fixed there this session; re-apply here too.
        if (compositeData.Get ("name", name)) {
            composite.header.uniStringNamePtr = &name;
        }

        // useWith is only applied when given: a modify that omits it (e.g. a pure rename) keeps the
        // Composite's current Wall/Slab/Roof/Shell flags, already loaded by ACAPI_Attribute_Get above,
        // instead of clearing all of them.
        GS::Array<GS::UniString> useWith;
        if (compositeData.Get ("useWith", useWith)) {
            composite.header.flags &= ~APICWall_ForWall;
            composite.header.flags &= ~APICWall_ForSlab;
            composite.header.flags &= ~APICWall_ForRoof;
            composite.header.flags &= ~APICWall_ForShell;
            for (const GS::UniString& useWithStr : useWith) {
                if (useWithStr == "Wall") {
                    composite.header.flags |= APICWall_ForWall;
                } else if (useWithStr == "Slab") {
                    composite.header.flags |= APICWall_ForSlab;
                } else if (useWithStr == "Roof") {
                    composite.header.flags |= APICWall_ForRoof;
                } else if (useWithStr == "Shell") {
                    composite.header.flags |= APICWall_ForShell;
                }
            }
        }

        GS::Array<GS::ObjectState> skins;
        bool hasSkins = compositeData.Get ("skins", skins);

        GS::Array<GS::ObjectState> separators;
        bool hasSeparators = compositeData.Get ("separators", separators);

        // skins/separators are only required for a brand new Composite. When overwriting an existing
        // one, whichever of the two is omitted is taken from its current definition, so a rename or a
        // useWith-only change does not have to resend the whole skin structure. nComps/totalThick are
        // already the existing values here (filled by ACAPI_Attribute_Get above).
        if (!doesExist && (!hasSkins || !hasSeparators)) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "skins and separators are required when creating a new Composite."));
            continue;
        }

        UInt32 componentCount = hasSkins ? skins.GetSize () : (UInt32) composite.compWall.nComps;
        if (componentCount == 0) {
            attributeIds (CreateErrorResponse (Error, "Skin array is empty."));
            continue;
        }
        if (hasSeparators && separators.GetSize () != componentCount + 1) {
            attributeIds (CreateErrorResponse (Error, "Invalid separator count."));
            continue;
        }
        if (!hasSeparators && componentCount != (UInt32) composite.compWall.nComps) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "separators are required when the number of skins changes."));
            continue;
        }

        if (!hasSkins || !hasSeparators) {
            API_AttributeDefExt existingDefs = {};
            if (ACAPI_Attribute_GetDefExt (API_CompWallID, composite.header.index, &existingDefs) != NoError) {
                attributeIds (CreateErrorResponse (Error, "Failed to read the existing Composite's skins."));
                continue;
            }
            // Take ownership of the kept handles (nulled in existingDefs so they are not disposed twice),
            // compositeDefs is disposed as a whole once the Composite has been modified.
            if (!hasSkins) {
                compositeDefs.cwall_compItems = existingDefs.cwall_compItems;
                existingDefs.cwall_compItems = nullptr;
            }
            if (!hasSeparators) {
                compositeDefs.cwall_compLItems = existingDefs.cwall_compLItems;
                existingDefs.cwall_compLItems = nullptr;
            }
            ACAPI_DisposeAttrDefsHdlsExt (&existingDefs);
        }

        if (hasSkins) {
            double totalThickness = 0.0;
            compositeDefs.cwall_compItems = (API_CWallComponent**) BMAllocateHandle (componentCount * sizeof (API_CWallComponent), ALLOCATE_CLEAR, 0);
            for (UInt32 i = 0; i < componentCount; i++) {
                const GS::ObjectState& skinData = skins[i];
                API_CWallComponent& compData = (*compositeDefs.cwall_compItems)[i];

                GS::UniString type;
                skinData.Get ("type", type);
                if (type == "Core") {
                    compData.flagBits |= APICWallComp_Core;
                } else if (type == "Finish") {
                    compData.flagBits |= APICWallComp_Finish;
                }

                GS::ObjectState buildingMaterialId;
                skinData.Get ("buildingMaterialId", buildingMaterialId);
                API_AttributeIndex buildingMaterialIndex;
                if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, buildingMaterialIndex)) {
                    compData.buildingMaterial = buildingMaterialIndex;
                }

                skinData.Get ("framePen", compData.framePen);
                skinData.Get ("thickness", compData.fillThick);
                totalThickness += compData.fillThick;
            }

            composite.compWall.nComps = (short) componentCount;
            composite.compWall.totalThick = totalThickness;
        }

        if (hasSeparators) {
            compositeDefs.cwall_compLItems = (API_CWallLineComponent**) BMAllocateHandle ((componentCount + 1) * sizeof (API_CWallLineComponent), ALLOCATE_CLEAR, 0);
            for (UInt32 i = 0; i < componentCount + 1; i++) {
                const GS::ObjectState& separatorData = separators[i];
                API_CWallLineComponent& lineData = (*compositeDefs.cwall_compLItems)[i];

                GS::ObjectState lineTypeId;
                separatorData.Get ("lineTypeId", lineTypeId);
                API_AttributeIndex lineTypeIndex;
                if (GetAttributeIndexFromAttributeId (lineTypeId, API_LinetypeID, lineTypeIndex)) {
                    lineData.ltypeInd = lineTypeIndex;
                }

                separatorData.Get ("linePen", lineData.linePen);
            }
        }

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_ModifyExt (&composite, &compositeDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify attribute."));
                ACAPI_DisposeAttrDefsHdlsExt (&compositeDefs);
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_CreateExt (&composite, &compositeDefs);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create attribute."));
                ACAPI_DisposeAttrDefsHdlsExt (&compositeDefs);
                continue;
            }
        }

        attributeIds (CreateAttributeIdObjectState (composite.header.guid));
        ACAPI_DisposeAttrDefsHdlsExt (&compositeDefs);
    }

    return response;
}

// ACAPI_Database_JsonQuery answers a query string with a string. The queries used here: "lineConsolidation"
// and "fillNormalization" run the Line Consolidation and Fill Consolidation menu commands, "usedAttributes"
// reports the attributes in use. AC25 and AC26 declare APIDb_JsonQueryID with other parameter types, so
// these commands are Archicad 27+ only.
#ifdef ServerMainVers_2700
static GSErrCode RunJsonQuery (const GS::UniString& query, GS::UniString& result)
{
    return ACAPI_Database_JsonQuery (&query, &result);
}
#endif

ConsolidationCommandBase::ConsolidationCommandBase (const GS::String& commandNameIn, const GS::UniString& queryIn, const GS::UniString& menuCommandNameIn)
    : CommandBase (CommonSchema::Used)
    , commandName (commandNameIn)
    , query (queryIn)
    , menuCommandName (menuCommandNameIn)
{
}

GS::String ConsolidationCommandBase::GetName () const
{
    return commandName;
}

GS::Optional<GS::UniString> ConsolidationCommandBase::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "result": {
                "type": "string",
                "description": "What Archicad answered for the JSON query, as it came. Present when the query ran. The Archicad API does not document its content; it may report a failure of the menu command even though the query itself ran."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    })";
}

GS::ObjectState ConsolidationCommandBase::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString result;
    const GSErrCode err = RunJsonQuery (query, result);
    if (err != NoError) {
        return GS::ObjectState ("executionResult", CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to run the %T menu command.", menuCommandName.ToPrintf ())));
    }

    // The answer is undocumented and may say that the menu command failed although the query ran,
    // so it is passed on for the caller to see.
    GS::ObjectState response ("executionResult", CreateSuccessfulExecutionResult ());
    response.Add ("result", result);
    return response;
#else
    return GS::ObjectState ("executionResult", CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer."));
#endif
}

ConsolidateLinesCommand::ConsolidateLinesCommand ()
    : ConsolidationCommandBase ("ConsolidateLines", "lineConsolidation", "Line Consolidation")
{
}

ConsolidateFillsCommand::ConsolidateFillsCommand ()
    : ConsolidationCommandBase ("ConsolidateFills", "fillNormalization", "Fill Consolidation")
{
}

GetUsedAttributesCommand::GetUsedAttributesCommand () :
    CommandBase (CommonSchema::NotUsed)
{
}

GS::String GetUsedAttributesCommand::GetName () const
{
    return "GetUsedAttributes";
}

GS::Optional<GS::UniString> GetUsedAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "description": "The attributes in use, as Archicad reports them for the usedAttributes JSON query. The Archicad API does not document the layout of the report, so it is passed on as it comes: parsed into usedAttributes when it is a JSON object, otherwise as text in raw.",
        "properties": {
            "usedAttributes": {
                "type": "object",
                "description": "The report parsed as a JSON object."
            },
            "raw": {
                "type": "string",
                "description": "The report as text, when it is not a JSON object."
            }
        },
        "additionalProperties": false
    })";
}

GS::ObjectState GetUsedAttributesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::UniString result;
    const GSErrCode err = RunJsonQuery ("usedAttributes", result);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the used attributes.");
    }

    // The layout of the report is undocumented; an ObjectState only holds a JSON object, so anything
    // else (an array, plain text) goes back as it came.
    GS::ObjectState usedAttributes;
    if (JSON::ConvertToObjectState (result, usedAttributes) == NoError) {
        return GS::ObjectState ("usedAttributes", usedAttributes);
    }
    return GS::ObjectState ("raw", result);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetFontsByNameCommand::GetFontsByNameCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetFontsByNameCommand::GetName () const
{
    return "GetFontsByName";
}

GS::Optional<GS::UniString> GetFontsByNameCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fontNames": {
                "type": "array",
                "description": "Names of the fonts to find, as the Text tool's font list shows them and GetDetailsOfElements reports them in fontName, e.g. 'Arial'.",
                "items": {
                    "type": "string"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "fontNames"
        ]
    })";
}

GS::Optional<GS::UniString> GetFontsByNameCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "fonts": {
                "type": "array",
                "description": "One item per name, in the order given: the font found, or an error when no loaded font has the name.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "fontIndex": {
                                    "type": "integer",
                                    "description": "Index of the font (from 1), as Text and Label fields reference it in fontIndex."
                                },
                                "fontName": {
                                    "type": "string",
                                    "description": "Name of the font as Archicad carries it."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "fontIndex",
                                "fontName"
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
            "fonts"
        ]
    })";
}

GS::ObjectState GetFontsByNameCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> fontNames;
    parameters.Get ("fontNames", fontNames);

    GS::ObjectState response;
    const auto& fonts = response.AddList<GS::ObjectState> ("fonts");

    for (const GS::UniString& fontName : fontNames) {
        // Archicad searches by the name uniStringNamePtr points at and reports the index of the font
        // it found in the head, the way ACAPI_Attribute_Search does for attributes - which fonts
        // were before Archicad 27 (API_FontID), with their own API (ACAPI_Font_SearchFont) since.
        GS::UniString nameToSearch = fontName;
        short fontIndex = 0;
#ifdef ServerMainVers_2700
        API_FontType font = {};
        font.head.uniStringNamePtr = &nameToSearch;
        const GSErrCode err = ACAPI_Font_SearchFont (font);
        fontIndex = static_cast<short> (font.head.index);
#else
        API_Attr_Head attrHead = {};
        attrHead.typeID = API_FontID;
        attrHead.uniStringNamePtr = &nameToSearch;
        const GSErrCode err = ACAPI_Attribute_Search (&attrHead);
        fontIndex = static_cast<short> (GetAttributeIndex (attrHead.index));
#endif
        if (err != NoError) {
            fonts (CreateErrorResponse (err, GS::UniString::Printf ("No loaded font is named '%T'.", fontName.ToPrintf ())));
            continue;
        }

        // The name is read back from the font found rather than echoed, so the caller sees it as
        // Archicad carries it even when the search matched a differently written name.
        const GS::Optional<GS::UniString> foundName = GetFontNameFromIndex (fontIndex);

        GS::ObjectState fontOS;
        fontOS.Add ("fontIndex", fontIndex);
        fontOS.Add ("fontName", foundName.HasValue () ? *foundName : fontName);
        fonts (fontOS);
    }

    return response;
}

// The user data of an attribute: ACAPI_UserData_GetUserData / SetUserData from AC27, ACAPI_Attribute_GetUserData /
// SetUserData before (see MigrationHelper.hpp). Only dataVersion, platformSign and dataHdl are used, the fields the
// AC25-AC30 API_AttributeUserData share. Setting is a non-undoable modifier, so it runs outside an undo scope.

static bool FindAttributeForUserData (const GS::ObjectState& attributeIdItem, API_Attr_Head& attrHead)
{
    const GS::ObjectState* attributeId = attributeIdItem.Get ("attributeId");
    if (attributeId == nullptr) {
        return false;
    }
    attrHead = GetAttributeHeadFromGuid (GetGuidFromObjectState (*attributeId));
    return attrHead.typeID != API_ZombieAttrID;
}

// Reads the pairs stored on the attribute: none when it has no user data. A failure fills errorMessage.
static GSErrCode ReadAttributeUserDataPairs (API_Attr_Head& attrHead, UserDataPairs& pairs, GS::UniString& errorMessage)
{
    pairs.clear ();

    API_AttributeUserData userData = {};
    GSErrCode err = ACAPI_UserData_GetUserData (&attrHead, &userData);
    if (err == APIERR_NOUSERDATA) {
        BMKillHandle (&userData.dataHdl);
        return NoError;
    }
    if (err != NoError) {
        BMKillHandle (&userData.dataHdl);
        errorMessage = "Failed to get the user data of the attribute.";
        return err;
    }
    if (userData.dataHdl == nullptr) {
        return NoError;
    }

    // Data in a layout these commands do not write is reported, never overwritten.
    if (userData.dataVersion != UserDataPairsLayoutVersion) {
        errorMessage = GS::UniString::Printf ("The attribute holds user data in another layout (version %d).", static_cast<int> (userData.dataVersion));
        err = APIERR_BADPARS;
    } else {
        pairs = ParseUserDataPairs (userData.dataHdl);
    }

    BMKillHandle (&userData.dataHdl);
    return err;
}

GetUserDataOfAttributesCommand::GetUserDataOfAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetUserDataOfAttributesCommand::GetName () const
{
    return "GetUserDataOfAttributes";
}

GS::Optional<GS::UniString> GetUserDataOfAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetUserDataOfAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "userDataOfAttributes": {
                "type": "array",
                "description": "One item per attribute, in the input order: the pairs stored on the attribute (none when it holds no user data), or an error when the attribute was not found or holds user data in another layout.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "pairs": {
                                    "$ref": "#/UserDataPairs"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "pairs"
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
            "userDataOfAttributes"
        ]
    })";
}

GS::ObjectState GetUserDataOfAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GS::ObjectState response;
    const auto& userDataOfAttributes = response.AddList<GS::ObjectState> ("userDataOfAttributes");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        API_Attr_Head attrHead = {};
        if (!FindAttributeForUserData (attributeIdItem, attrHead)) {
            userDataOfAttributes (CreateErrorResponse (APIERR_BADID, "Attribute not found."));
            continue;
        }

        UserDataPairs pairs;
        GS::UniString errorMessage;
        const GSErrCode err = ReadAttributeUserDataPairs (attrHead, pairs, errorMessage);
        if (err != NoError) {
            userDataOfAttributes (CreateErrorResponse (err, errorMessage));
            continue;
        }

        GS::ObjectState item;
        AddUserDataPairsList (item, "pairs", pairs);
        userDataOfAttributes (item);
    }

    return response;
}

SetUserDataOfAttributesCommand::SetUserDataOfAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetUserDataOfAttributesCommand::GetName () const
{
    return "SetUserDataOfAttributes";
}

GS::Optional<GS::UniString> SetUserDataOfAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "pairs": {
                "$ref": "#/UserDataPairs",
                "description": "The key-value pairs to store on each attribute. A key the attribute already holds gets the new value; other keys are left as they are."
            },
            "keysToDelete": {
                "$ref": "#/UserDataKeysToDelete"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> SetUserDataOfAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per attribute, in the input order."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState SetUserDataOfAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    UserDataPairs pairsToSet;
    GS::Array<GS::UniString> keysToDelete;
    GS::UniString errorMessage;
    if (!GetUserDataPairChanges (parameters, pairsToSet, keysToDelete, errorMessage)) {
        return CreateErrorResponse (APIERR_BADPARS, errorMessage);
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        API_Attr_Head attrHead = {};
        if (!FindAttributeForUserData (attributeIdItem, attrHead)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "Attribute not found."));
            continue;
        }

        UserDataPairs pairs;
        GSErrCode err = ReadAttributeUserDataPairs (attrHead, pairs, errorMessage);
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, errorMessage));
            continue;
        }

        ApplyUserDataPairChanges (pairs, pairsToSet, keysToDelete);

        API_AttributeUserData userData = {};
        userData.dataVersion = UserDataPairsLayoutVersion;
        userData.platformSign = GS::Act_Platform_Sign;
        userData.dataHdl = SerializeUserDataPairs (pairs);
        if (userData.dataHdl == nullptr) {
            executionResults (CreateFailedExecutionResult (APIERR_MEMFULL, "Failed to store the user data of the attribute."));
            continue;
        }

        // The API copies the data, the handle stays ours to free.
        err = ACAPI_UserData_SetUserData (&attrHead, &userData);
        BMKillHandle (&userData.dataHdl);
        executionResults (err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to store the user data of the attribute."));
    }

    return response;
}

DeleteUserDataOfAttributesCommand::DeleteUserDataOfAttributesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteUserDataOfAttributesCommand::GetName () const
{
    return "DeleteUserDataOfAttributes";
}

GS::Optional<GS::UniString> DeleteUserDataOfAttributesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteUserDataOfAttributesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults",
                "description": "One result per attribute, in the input order."
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    })";
}

GS::ObjectState DeleteUserDataOfAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributeIds;
    parameters.Get ("attributeIds", attributeIds);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& attributeIdItem : attributeIds) {
        API_Attr_Head attrHead = {};
        if (!FindAttributeForUserData (attributeIdItem, attrHead)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "Attribute not found."));
            continue;
        }

        // A null dataHdl deletes the user data; the struct pointer itself must not be null (APIERR_BADPARS).
        API_AttributeUserData userData = {};
        userData.platformSign = GS::Act_Platform_Sign;
        userData.dataHdl = nullptr;
        const GSErrCode err = ACAPI_UserData_SetUserData (&attrHead, &userData);
        executionResults (err == NoError
            ? CreateSuccessfulExecutionResult ()
            : CreateFailedExecutionResult (err, "Failed to delete the user data of the attribute."));
    }

    return response;
}

ReindexLayersCommand::ReindexLayersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ReindexLayersCommand::GetName () const
{
    return "ReindexLayers";
}

GS::Optional<GS::UniString> ReindexLayersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layers": {
                "type": "array",
                "description": "The layers to move to a new index, handled in the given order.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeId": {
                            "$ref": "#/AttributeId",
                            "description": "The layer to move."
                        },
                        "newIndex": {
                            "type": "integer",
                            "description": "The index the layer gets. It must not be used by another layer. Index 1 (the Archicad Layer) cannot be used or moved.",
                            "minimum": 2
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "attributeId",
                        "newIndex"
                    ]
                }
            },
            "dryRun": {
                "type": "boolean",
                "description": "Only list what would change (the elements on each layer, the tools whose default layer it is, the elements that block the move) without changing anything. The default is false."
            },
            "maxPlaceholders": {
                "type": "integer",
                "description": "Every free index below newIndex is filled with a temporary layer first (about 20 ms each), so a newIndex far above the highest index in use takes long. A layer that needs more temporary layers than this is not moved. The default is 1000.",
                "minimum": 0
            }
        },
        "additionalProperties": false,
        "required": [
            "layers"
        ]
    })";
}

GS::Optional<GS::UniString> ReindexLayersCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "layers": {
                "type": "array",
                "description": "One item per requested layer, in the same order.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "attributeId": {
                                    "$ref": "#/AttributeId",
                                    "description": "The layer at its new index. Archicad cannot renumber an attribute in place, so this is a new attribute with a new id; with dryRun it is the layer's current id."
                                },
                                "oldIndex": {
                                    "type": "integer"
                                },
                                "newIndex": {
                                    "type": "integer"
                                },
                                "elements": {
                                    "$ref": "#/Elements",
                                    "description": "The elements on the layer, in the floor plan and in every section, elevation, interior elevation, detail, worksheet, 3D document, layout and master layout. Moved to the new layer unless dryRun is true. The model elements that sections, elevations, interior elevations and 3D documents show are not listed (see viewsToRebuild)."
                                },
                                "blockedElements": {
                                    "$ref": "#/Elements",
                                    "description": "Only with dryRun: elements on the layer that cannot be moved, because they come from a hotlinked module. Without dryRun such a layer is not moved."
                                },
                                "toolDefaults": {
                                    "type": "array",
                                    "description": "The element types whose tool default is on this layer. They are set to the new layer unless dryRun is true.",
                                    "items": {
                                        "$ref": "#/ElementType"
                                    }
                                },
                                "placeholders": {
                                    "type": "integer",
                                    "description": "The number of temporary layers needed for the free indices below newIndex."
                                },
                                "warnings": {
                                    "type": "array",
                                    "description": "What did not work after the layer was moved: its name and flags, or the move into its attribute folder.",
                                    "items": {
                                        "type": "string"
                                    }
                                },
                                "viewsToRebuild": {
                                    "$ref": "#/Databases",
                                    "description": "The sections, elevations, interior elevations and 3D documents that show elements of the layer. The model elements they show are deleted with the old layer and come back on the new layer when the view is rebuilt: Archicad does that when the view is opened, unless it is set to be rebuilt manually. To rebuild one at once, call ChangeWindow with its databaseId and then RebuildCurrentDatabase."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "attributeId",
                                "oldIndex",
                                "newIndex",
                                "elements",
                                "toolDefaults",
                                "placeholders",
                                "viewsToRebuild"
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
            "layers"
        ]
    })";
}

#ifdef ServerMainVers_2700

namespace {

struct ElementInDatabase {
    API_DatabaseInfo database;
    API_Guid         guid;
};

// The floor plan and every database of the views and layouts: together they hold every element that has a layer.
GSErrCode GetElementDatabases (GS::Array<API_DatabaseInfo>& databases)
{
    API_DatabaseInfo floorPlan = {};
    floorPlan.typeID = APIWind_FloorPlanID;
    GSErrCode err = ACAPI_Window_GetDatabaseInfo (&floorPlan);
    if (err != NoError) {
        return err;
    }
    databases.Push (floorPlan);

    const struct {
        API_WindowTypeID typeID;
        GSErrCode (*getDatabases) (API_DatabaseUnId**, GS::Array<API_DatabaseUnId>*);
    } databaseTypes[] = {
        { APIWind_SectionID,           ACAPI_Database_GetSectionDatabases },
        { APIWind_ElevationID,         ACAPI_Database_GetElevationDatabases },
        { APIWind_InteriorElevationID, ACAPI_Database_GetInteriorElevationDatabases },
        { APIWind_DetailID,            ACAPI_Database_GetDetailDatabases },
        { APIWind_WorksheetID,         ACAPI_Database_GetWorksheetDatabases },
        { APIWind_DocumentFrom3DID,    ACAPI_Database_GetDocumentFrom3DDatabases },
        { APIWind_LayoutID,            ACAPI_Database_GetLayoutDatabases },
        { APIWind_MasterLayoutID,      ACAPI_Database_GetMasterLayoutDatabases }
    };
    for (const auto& databaseType : databaseTypes) {
        GS::Array<API_DatabaseUnId> databaseUnIds;
        err = databaseType.getDatabases (nullptr, &databaseUnIds);
        if (err != NoError) {
            return err;
        }
        for (const API_DatabaseUnId& databaseUnId : databaseUnIds) {
            API_DatabaseInfo database = {};
            database.typeID = databaseType.typeID;
            database.databaseUnId = databaseUnId;
            err = ACAPI_Window_GetDatabaseInfo (&database);
            if (err != NoError) {
                return err;
            }
            databases.Push (database);
        }
    }
    return NoError;
}

void AddDatabaseOnce (GS::Array<API_DatabaseInfo>& databases, const API_DatabaseInfo& database)
{
    for (const API_DatabaseInfo& listed : databases) {
        if (listed.databaseUnId.elemSetId == database.databaseUnId.elemSetId) {
            return;
        }
    }
    databases.Push (database);
}

// Lists the elements on the layer in every database. Any database that cannot be read fails the whole scan,
// because the scan decides whether the layer may be deleted, and deleting a layer deletes its elements.
// The model elements shown in a section, elevation, interior elevation or 3D document (API_SectElemID) are not
// listed: changing their layer reports success but keeps it, and a rebuild of their view recreates them from the
// element they show, on that element's layer (tested on Archicad 29). Their views are added to sectionElementViews.
GSErrCode CollectElementsOnLayer (const GS::Array<API_DatabaseInfo>& databases, const API_AttributeIndex& layer, GS::Array<ElementInDatabase>& elements, GS::Array<API_DatabaseInfo>* sectionElementViews = nullptr)
{
    elements.Clear ();
    for (const API_DatabaseInfo& database : databases) {
        API_DatabaseInfo current = database;
        GSErrCode err = ACAPI_Database_ChangeCurrentDatabase (&current);
        if (err != NoError) {
            return err;
        }
        GS::Array<API_Guid> guids;
        err = ACAPI_Element_GetElemList (API_ZombieElemID, &guids);
        if (err != NoError) {
            return err;
        }
        for (const API_Guid& guid : guids) {
            API_Elem_Head head = {};
            head.guid = guid;
            if (ACAPI_Element_GetHeader (&head) != NoError || head.layer != layer) {
                continue;
            }
            if (GetElemTypeId (head.type) == API_SectElemID) {
                if (sectionElementViews != nullptr) {
                    AddDatabaseOnce (*sectionElementViews, database);
                }
                continue;
            }
            elements.Push ({ database, guid });
        }
    }
    return NoError;
}

GSErrCode SetLayerOfElementInDatabase (const ElementInDatabase& element, const API_AttributeIndex& layer)
{
    API_DatabaseInfo database = element.database;
    GSErrCode err = ACAPI_Database_ChangeCurrentDatabase (&database);
    if (err != NoError) {
        return err;
    }
    API_Element elem = {};
    elem.header.guid = element.guid;
    err = ACAPI_Element_Get (&elem);
    if (err != NoError) {
        return err;
    }
    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
    elem.header.layer = layer;
    return ACAPI_Element_Change (&elem, &mask, nullptr, 0, true);
}

// Reads a layer with its Unicode name (see GetAttributeName for why the name needs uniStringNamePtr).
GSErrCode GetLayer (API_Attribute& layer, GS::UniString& name)
{
    layer.header.typeID = API_LayerID;
    layer.header.uniStringNamePtr = &name;
    return ACAPI_Attribute_Get (&layer);
}

// The type of API_Attr_Head::flags: short before Archicad 29, GSFlags from 29.
using LayerFlags = decltype (API_Attr_Head::flags);

LayerFlags WithoutHiddenAndLocked (LayerFlags flags)
{
    return static_cast<LayerFlags> (flags & ~(APILay_Hidden | APILay_Locked));
}

GSErrCode SetLayerFlags (const API_AttributeIndex& index, LayerFlags flags)
{
    GS::UniString name;
    API_Attribute layer = {};
    layer.header.index = index;
    GSErrCode err = GetLayer (layer, name);
    if (err != NoError) {
        return err;
    }
    layer.header.flags = flags;
    return ACAPI_Attribute_Modify (&layer, nullptr);
}

GSErrCode DeleteLayer (const API_AttributeIndex& index)
{
    API_Attr_Head head = {};
    head.typeID = API_LayerID;
    head.index = index;
    return ACAPI_Attribute_Delete (head);
}

// A name no layer has, for the placeholders and for the copy until the old layer is gone.
GS::UniString GetTemporaryLayerName (const GS::UniString& prefix)
{
    GS::Guid guid;
    guid.Generate ();
    return prefix + " " + guid.ToUniString ();
}

// Every layer combination gets the same state (hidden, locked, wireframe, intersection group) for the new layer as for the old one.
GSErrCode CopyLayerStateInCombinations (const API_AttributeIndex& from, const API_AttributeIndex& to)
{
    GS::Array<API_Attribute> combinations;
    GSErrCode err = ACAPI_Attribute_GetAttributesByType (API_LayerCombID, combinations);
    if (err != NoError) {
        return err;
    }
    for (API_Attribute& listed : combinations) {
        const API_AttributeIndex combinationIndex = listed.header.index;
        DisposeAttribute (listed);

        GS::UniString name;
        API_Attribute combination = {};
        combination.header.typeID = API_LayerCombID;
        combination.header.index = combinationIndex;
        combination.header.uniStringNamePtr = &name;
        err = ACAPI_Attribute_Get (&combination);
        if (err != NoError) {
            return err;
        }
        API_AttributeDef def = {};
        err = ACAPI_Attribute_GetDef (API_LayerCombID, combinationIndex, &def);
        if (err == NoError && def.layer_statItems != nullptr && def.layer_statItems->ContainsKey (from)) {
            def.layer_statItems->Put (to, def.layer_statItems->Get (from));
            combination.layerComb.lNumb = static_cast<Int32> (def.layer_statItems->GetSize ());
            err = ACAPI_Attribute_Modify (&combination, &def);
        }
        ACAPI_DisposeAttrDefsHdls (&def);
        if (err != NoError) {
            return err;
        }
    }
    return NoError;
}

// The element types whose tool default is on the layer. With a new layer, those defaults are set to it.
GS::Array<API_ElemTypeID> UpdateToolDefaultsOnLayer (const API_AttributeIndex& layer, const GS::Optional<API_AttributeIndex>& newLayer)
{
    GS::Array<API_ElemTypeID> types;
    for (Int32 type = API_FirstElemType; type <= API_LastElemType; ++type) {
        const API_ElemTypeID typeID = static_cast<API_ElemTypeID> (type);
        API_Element defaults = {};
        defaults.header.type = typeID;
        if (ACAPI_Element_GetDefaults (&defaults, nullptr) != NoError || defaults.header.layer != layer) {
            continue;
        }
        if (newLayer.HasValue ()) {
            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, layer);
            defaults.header.layer = newLayer.Get ();
            if (ACAPI_Element_ChangeDefaults (&defaults, nullptr, &mask) != NoError) {
                continue;
            }
        }
        types.Push (typeID);
    }
    return types;
}

// The attribute folder that holds the layer, or no value for the root folder (where a new layer is created).
GS::Optional<API_AttributeFolder> FindFolderOfLayer (const API_AttributeFolder& folder, const GS::Guid& layerGuid, bool isRoot)
{
    API_AttributeFolderContent content;
    if (ACAPI_Attribute_GetFolderContent (folder, content) != NoError) {
        return GS::NoValue;
    }
    if (content.attributeIds.Contains (layerGuid)) {
        return isRoot ? GS::Optional<API_AttributeFolder> () : GS::Optional<API_AttributeFolder> (folder);
    }
    for (const API_AttributeFolder& subFolder : content.subFolders) {
        GS::Optional<API_AttributeFolder> found = FindFolderOfLayer (subFolder, layerGuid, false);
        if (found.HasValue ()) {
            return found;
        }
    }
    return GS::NoValue;
}

GS::Array<GS::ObjectState> GetElementIds (const GS::Array<ElementInDatabase>& elements)
{
    GS::Array<GS::ObjectState> ids;
    for (const ElementInDatabase& element : elements) {
        ids.Push (CreateElementIdObjectState (element.guid));
    }
    return ids;
}

GS::Array<GS::ObjectState> GetDatabaseIds (const GS::Array<API_DatabaseInfo>& databases)
{
    GS::Array<GS::ObjectState> ids;
    for (const API_DatabaseInfo& database : databases) {
        ids.Push (CreateDatabaseIdObjectState (DatabaseIdResolver::Instance ().GetIdOfDatabase (database)));
    }
    return ids;
}

GS::UniString ListElementGuids (const GS::Array<ElementInDatabase>& elements)
{
    GS::UniString list;
    for (UIndex i = 0; i < elements.GetSize () && i < 5; ++i) {
        list += (i == 0 ? "" : ", ") + APIGuidToString (elements[i].guid);
    }
    if (elements.GetSize () > 5) {
        list += GS::UniString::Printf (" and %u more", elements.GetSize () - 5);
    }
    return list;
}

// Moves one layer to newIndex, as described in the command's registration. On any failure, everything done so far
// for this layer is undone, and the old layer with its elements is kept.
GS::ObjectState ReindexLayer (const GS::Array<API_DatabaseInfo>& databases, const API_Guid& layerGuid, Int32 newIndexValue, bool dryRun, Int32 maxPlaceholders)
{
    GS::UniString name;
    API_Attribute oldLayer = {};
    oldLayer.header.guid = layerGuid;
    if (GetLayer (oldLayer, name) != NoError) {
        return CreateErrorResponse (APIERR_BADID, "The layer was not found.");
    }
    const API_AttributeIndex oldIndex = oldLayer.header.index;
    const API_AttributeIndex newIndex = ACAPI_CreateAttributeIndex (newIndexValue);
    if (GetAttributeIndex (oldIndex) == 1) {
        return CreateErrorResponse (APIERR_BADPARS, "The Archicad Layer (index 1) cannot be moved.");
    }
    if (newIndexValue < 2) {
        return CreateErrorResponse (APIERR_BADPARS, "newIndex must be 2 or higher: index 1 is the Archicad Layer.");
    }

    // Archicad gives a new layer the lowest free index, so every free index below newIndex gets a placeholder first.
    GS::Array<API_Attribute> layers;
    GSErrCode err = ACAPI_Attribute_GetAttributesByType (API_LayerID, layers);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to list the layers.");
    }
    GS::HashSet<Int32> usedIndices;
    for (API_Attribute& layer : layers) {
        usedIndices.Add (GetAttributeIndex (layer.header.index));
        DisposeAttribute (layer);
    }
    if (oldIndex != newIndex && usedIndices.Contains (newIndexValue)) {
        API_Attribute other = {};
        other.header.index = newIndex;
        GS::UniString otherName;
        GetLayer (other, otherName);
        return CreateErrorResponse (APIERR_ATTREXIST, GS::UniString::Printf ("Index %d is used by the layer '%T'. Move that layer first.", newIndexValue, otherName.ToPrintf ()));
    }

    GS::Array<ElementInDatabase> elements;
    GS::Array<API_DatabaseInfo> sectionElementViews;
    err = CollectElementsOnLayer (databases, oldIndex, elements, &sectionElementViews);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the elements of every view and layout, so the layer was not moved.");
    }
    GS::Array<ElementInDatabase> blocked;
    for (const ElementInDatabase& element : elements) {
        API_DatabaseInfo database = element.database;
        ACAPI_Database_ChangeCurrentDatabase (&database);
        API_Elem_Head head = {};
        head.guid = element.guid;
        if (ACAPI_Element_GetHeader (&head) == NoError && head.hotlinkGuid != APINULLGuid) {
            blocked.Push (element);
        }
    }

    Int32 placeholderCount = 0;
    for (Int32 index = 1; index < newIndexValue && oldIndex != newIndex; ++index) {
        if (!usedIndices.Contains (index)) {
            ++placeholderCount;
        }
    }

    GS::ObjectState result;
    result.Add ("oldIndex", GetAttributeIndex (oldIndex));
    result.Add ("newIndex", newIndexValue);
    result.Add ("placeholders", placeholderCount);

    if (dryRun || oldIndex == newIndex) {
        result.Add ("attributeId", CreateGuidObjectState (layerGuid));
        result.Add ("elements", GetElementIds (elements));
        if (dryRun) {
            result.Add ("blockedElements", GetElementIds (blocked));
        }
        result.Add ("viewsToRebuild", GetDatabaseIds (dryRun ? sectionElementViews : GS::Array<API_DatabaseInfo> ()));
        const auto& toolDefaults = result.AddList<GS::UniString> ("toolDefaults");
        for (API_ElemTypeID typeID : UpdateToolDefaultsOnLayer (oldIndex, GS::NoValue)) {
            toolDefaults (GetElementTypeNonLocalizedName (typeID));
        }
        return result;
    }
    if (placeholderCount > maxPlaceholders) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("The layer was not moved: index %d needs %d temporary layers for the free indices below it, more than maxPlaceholders (%d). Give a larger maxPlaceholders if this index is intended.", newIndexValue, placeholderCount, maxPlaceholders));
    }
    if (!blocked.IsEmpty ()) {
        return CreateErrorResponse (APIERR_GENERAL, GS::UniString::Printf ("The layer was not moved: %u of its elements come from a hotlinked module and cannot be moved (%T).", blocked.GetSize (), ListElementGuids (blocked).ToPrintf ()));
    }

    // Elements on a hidden or locked layer cannot be changed, so the old layer is shown and unlocked while its
    // elements are moved. Its own flags are put back on every path that keeps it. Archicad keeps these flags per
    // open window: they are read and written in the current one.
    const LayerFlags oldFlags = oldLayer.header.flags;
    err = SetLayerFlags (oldIndex, WithoutHiddenAndLocked (oldFlags));
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to show and unlock the layer for the move.");
    }

    GS::Array<API_AttributeIndex> placeholders;
    const auto deletePlaceholders = [&] () {
        for (const API_AttributeIndex& placeholder : placeholders) {
            DeleteLayer (placeholder);
        }
        placeholders.Clear ();
    };
    const auto fail = [&] (GSErrCode failErr, const GS::UniString& message) {
        deletePlaceholders ();
        SetLayerFlags (oldIndex, oldFlags);
        return CreateErrorResponse (failErr, message);
    };

    for (Int32 index = 1; index < newIndexValue; ++index) {
        if (usedIndices.Contains (index)) {
            continue;
        }
        GS::UniString placeholderName = GetTemporaryLayerName ("Tapir ReindexLayers placeholder");
        API_Attribute placeholder = {};
        placeholder.header.typeID = API_LayerID;
        placeholder.header.uniStringNamePtr = &placeholderName;
        err = ACAPI_Attribute_Create (&placeholder, nullptr);
        if (err != NoError) {
            return fail (err, "Failed to create a placeholder layer for a free index below newIndex.");
        }
        placeholders.Push (placeholder.header.index);
    }

    GS::UniString copyName = GetTemporaryLayerName (name);
    API_Attribute copy = oldLayer;
    copy.header.guid = APINULLGuid;
    copy.header.index = APIInvalidAttributeIndex;
    copy.header.flags = WithoutHiddenAndLocked (oldFlags);
    copy.header.uniStringNamePtr = &copyName;
    err = ACAPI_Attribute_Create (&copy, nullptr);
    if (err != NoError) {
        return fail (err, "Failed to create the layer at the new index.");
    }
    deletePlaceholders ();
    if (copy.header.index != newIndex) {
        DeleteLayer (copy.header.index);
        return fail (APIERR_GENERAL, GS::UniString::Printf ("Archicad created the new layer at index %d instead of %d, so the layer was not moved.", GetAttributeIndex (copy.header.index), newIndexValue));
    }
    const auto failAndDeleteCopy = [&] (GSErrCode failErr, const GS::UniString& message) {
        // The copy is only deleted when it holds no element, since deleting a layer deletes its elements.
        GS::Array<ElementInDatabase> onCopy;
        if (CollectElementsOnLayer (databases, newIndex, onCopy) == NoError && onCopy.IsEmpty ()) {
            DeleteLayer (newIndex);
        }
        return fail (failErr, message);
    };

    err = CopyLayerStateInCombinations (oldIndex, newIndex);
    if (err != NoError) {
        return failAndDeleteCopy (err, "Failed to copy the layer's state into the layer combinations.");
    }

    GS::Array<ElementInDatabase> moved;
    GS::Array<ElementInDatabase> notMoved;
    for (const ElementInDatabase& element : elements) {
        if (SetLayerOfElementInDatabase (element, newIndex) == NoError) {
            moved.Push (element);
        } else {
            notMoved.Push (element);
        }
    }
    GS::Array<ElementInDatabase> left;
    err = notMoved.IsEmpty () ? CollectElementsOnLayer (databases, oldIndex, left) : NoError;
    if (!notMoved.IsEmpty () || err != NoError || !left.IsEmpty ()) {
        for (const ElementInDatabase& element : moved) {
            SetLayerOfElementInDatabase (element, oldIndex);
        }
        if (!notMoved.IsEmpty ()) {
            return failAndDeleteCopy (APIERR_GENERAL, GS::UniString::Printf ("The layer was not moved: %u of its elements could not be moved to the new layer (they may be locked, or not reserved in Teamwork): %T.", notMoved.GetSize (), ListElementGuids (notMoved).ToPrintf ()));
        }
        if (err != NoError) {
            return failAndDeleteCopy (err, "The layer was not moved: the views and layouts could not be read again to check that the old layer is empty.");
        }
        return failAndDeleteCopy (APIERR_GENERAL, GS::UniString::Printf ("The layer was not moved: %u elements were still on the old layer after the move: %T.", left.GetSize (), ListElementGuids (left).ToPrintf ()));
    }

    const GS::Array<API_ElemTypeID> toolDefaults = UpdateToolDefaultsOnLayer (oldIndex, newIndex);

    // The folder is looked up before the old layer is deleted; the copy is moved there afterwards.
    API_AttributeFolder root = {};
    root.typeID = API_LayerID;
    GS::Optional<API_AttributeFolder> folder;
    if (ACAPI_Attribute_GetFolder (root) == NoError) {
        folder = FindFolderOfLayer (root, APIGuid2GSGuid (layerGuid), true);
    }

    // Only now, with the old layer empty in every view and layout, is it deleted.
    err = DeleteLayer (oldIndex);
    if (err != NoError) {
        UpdateToolDefaultsOnLayer (newIndex, oldIndex);
        for (const ElementInDatabase& element : moved) {
            SetLayerOfElementInDatabase (element, oldIndex);
        }
        return failAndDeleteCopy (err, "Failed to delete the old layer, so the layer was not moved.");
    }

    GS::UniString finalName = name;
    API_Attribute finalLayer = {};
    finalLayer.header.index = newIndex;
    GS::UniString readName;
    GetLayer (finalLayer, readName);
    finalLayer.header.uniStringNamePtr = &finalName;
    finalLayer.header.flags = oldFlags;
    // The layer is moved at this point, so what fails from here on is reported as a warning with the result.
    GS::Array<GS::UniString> warnings;
    if (ACAPI_Attribute_Modify (&finalLayer, nullptr) != NoError) {
        warnings.Push (GS::UniString::Printf ("Its name and flags could not be set back; it is called '%T'.", copyName.ToPrintf ()));
    }
    if (folder.HasValue () && ACAPI_Attribute_Move ({}, { APIGuid2GSGuid (finalLayer.header.guid) }, folder.Get ()) != NoError) {
        warnings.Push ("It could not be moved into the attribute folder of the old layer, so it is in the root folder.");
    }

    result.Add ("attributeId", CreateGuidObjectState (finalLayer.header.guid));
    result.Add ("elements", GetElementIds (moved));
    result.Add ("viewsToRebuild", GetDatabaseIds (sectionElementViews));
    const auto& toolDefaultNames = result.AddList<GS::UniString> ("toolDefaults");
    for (API_ElemTypeID typeID : toolDefaults) {
        toolDefaultNames (GetElementTypeNonLocalizedName (typeID));
    }
    if (!warnings.IsEmpty ()) {
        result.Add ("warnings", warnings);
    }
    return result;
}

}

#endif

GS::ObjectState ReindexLayersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    GS::Array<GS::ObjectState> layers;
    parameters.Get ("layers", layers);
    bool dryRun = false;
    parameters.Get ("dryRun", dryRun);
    Int32 maxPlaceholders = 1000;
    parameters.Get ("maxPlaceholders", maxPlaceholders);

    API_DatabaseInfo startingDatabase = {};
    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current database.");
    }
    GS::Array<API_DatabaseInfo> databases;
    err = GetElementDatabases (databases);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to list the views and layouts.");
    }

    GS::ObjectState response;
    const auto& results = response.AddList<GS::ObjectState> ("layers");
    const auto reindexAll = [&] () -> GSErrCode {
        for (const GS::ObjectState& layer : layers) {
            Int32 newIndex = 0;
            layer.Get ("newIndex", newIndex);
            results (ReindexLayer (databases, GetGuidFromAttributesArrayItem (layer), newIndex, dryRun, maxPlaceholders));
        }
        return NoError;
    };
    if (dryRun) {
        reindexAll ();
    } else {
        ACAPI_CallUndoableCommand ("Reindex Layers", reindexAll);
    }
    ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "ReindexLayers requires Archicad 27 or newer.");
#endif
}
