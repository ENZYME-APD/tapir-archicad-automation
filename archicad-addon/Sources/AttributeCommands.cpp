#include "AttributeCommands.hpp"
#include "MigrationHelper.hpp"
#include "ProfileVectorImage.hpp"
#include "ProfileAdditionalInfo.hpp"
#include "VectorImageIterator.hpp"
#ifdef ServerMainVers_2700
#include "ADBAttributeIndex.hpp"
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

GS::Optional<GS::UniString> GetAttributesByTypeCommand::GetResponseSchema () const
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
        attributeDetails.Add ("name", GS::UniString (attr.header.name));
        attributes (attributeDetails);

        DisposeAttribute (attr);
    }

    return response;
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

GS::Optional<GS::UniString> GetBuildingMaterialPhysicalPropertiesCommand::GetResponseSchema () const
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

GS::Optional<GS::UniString> GetLayerCombinationsCommand::GetResponseSchema () const
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
                    "type": "string",
                    "enum": ["scaleWithPlan", "defineScale", "lineType", "period", "height", "dashItems", "lineItems"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetLinesCommand::GetResponseSchema () const
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

        API_Attribute attr = {};
        attr.header.typeID = API_LinetypeID;
        attr.header.guid = GSGuid2APIGuid (attributeGuid);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            lines (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState line;
        line.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        line.Add ("index", GetAttributeIndex (attr.header.index));
        line.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["subType", "scaleWithPlan", "useForWalls", "useForDraft", "useForCover", "horizontalSpacing", "verticalSpacing", "angle", "bitPattern", "gradientStart", "gradientEnd", "percent", "texture", "lineItems", "symbolLines", "symbolArcs", "symbolHotspots"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetFillsCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_FilltypeID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            fills (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState fill;
        fill.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        fill.Add ("index", GetAttributeIndex (attr.header.index));
        fill.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["categoryCode", "color", "stampName", "stampMainGuid", "stampRevGuid"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetZoneCategoriesCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_ZoneCatID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            zoneCategories (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState zoneCategory;
        zoneCategory.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        zoneCategory.Add ("index", GetAttributeIndex (attr.header.index));
        zoneCategory.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["domain", "contourPen", "fillPen", "fillBackgroundPen", "centerLinePen", "fillId", "centerLineTypeId"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetMEPSystemsCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_MEPSystemID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            mepSystems (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState mepSystem;
        mepSystem.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        mepSystem.Add ("index", GetAttributeIndex (attr.header.index));
        mepSystem.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["isActiveForModel", "isActiveForLayout", "pens"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetPenTablesCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_PenTableID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            penTables (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState penTable;
        penTable.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        penTable.Add ("index", GetAttributeIndex (attr.header.index));
        penTable.Add ("name", GS::UniString (attr.header.name));

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
        API_Attribute attr = {};
        attr.header.typeID = API_PenTableID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            penTables (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState penTable;
        penTable.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        penTable.Add ("index", GetAttributeIndex (attr.header.index));
        penTable.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["wallType", "beamType", "coluType", "handrailType", "otherGDLObjectType", "useWith", "width", "height", "minimumWidth", "minimumHeight", "widthStretchable", "heightStretchable", "hasCoreSkin", "profileModifiers", "skins", "skinOutlines"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetProfilesCommand::GetResponseSchema () const
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

        GS::ObjectState buildingMaterialId;
        if (override.Get ("buildingMaterialId", buildingMaterialId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
                hatch.SetBuildMatIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
            }
        }
        GS::ObjectState surfaceId;
        if (override.Get ("surfaceId", surfaceId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (surfaceId, API_MaterialID, idx)) {
                hatch.SetSurfaceIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
            }
        }
        GS::ObjectState fillId;
        if (override.Get ("fillId", fillId)) {
            API_AttributeIndex idx;
            if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, idx)) {
                hatch.SetFillIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
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
// The ProfileEdgeData vector is sized to match coords exactly (one entry per coordinate, including the
// reserved anchor and every contour-closing duplicate) rather than just the "real" edge count: index 0
// is confirmed reserved/inert (see ApplyProfileSkinOverrides), and over-sizing by a few unused slots at
// each contour's closing duplicate is harmless, whereas under-sizing risks silently dropping a real
// edge - safer to always have a slot for every coordinate index GetProfileEdgeData() can be asked for.
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

    outHatch.buildMatFlags = SyHatchFlag_BuildingMaterialHatch;
    GS::ObjectState buildingMaterialId;
    if (skinDef.Get ("buildingMaterialId", buildingMaterialId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (buildingMaterialId, API_BuildingMaterialID, idx)) {
            outHatch.SetBuildMatIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
        }
    }
    GS::ObjectState surfaceId;
    if (skinDef.Get ("surfaceId", surfaceId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (surfaceId, API_MaterialID, idx)) {
            outHatch.SetSurfaceIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
        }
    }
    GS::ObjectState fillId;
    if (skinDef.Get ("fillId", fillId)) {
        API_AttributeIndex idx;
        if (GetAttributeIndexFromAttributeId (fillId, API_FilltypeID, idx)) {
            outHatch.SetFillIdx (FromPlainAttrIndex (GetAttributeIndex (idx)));
        }
    }
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
    std::vector<ProfileEdgeData> edges (coords.GetSize ());
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
        API_Attribute attr = {};
        attr.header.typeID = API_ProfileID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            profiles (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState profile;
        profile.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        profile.Add ("index", GetAttributeIndex (attr.header.index));
        profile.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["useWith", "skins", "separators"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetCompositesCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_CompWallID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            composites (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState composite;
        composite.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        composite.Add ("index", GetAttributeIndex (attr.header.index));
        composite.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["materialType", "ambientReflection", "diffuseReflection", "specularReflection", "transparency", "shine", "transparencyAttenuation", "emissionAttenuation", "surfaceColor", "specularColor", "emissionColor", "fillId", "texture"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetSurfacesCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_MaterialID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            surfaces (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState surface;
        surface.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        surface.Add ("index", GetAttributeIndex (attr.header.index));
        surface.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["isHidden", "isLocked", "isWireframe", "intersectionGroupNr"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetLayersCommand::GetResponseSchema () const
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
        API_Attribute attr = {};
        attr.header.typeID = API_LayerID;
        attr.header.guid = GetGuidFromAttributesArrayItem (attributeIdItem);
        GSErrCode err = ACAPI_Attribute_Get (&attr);
        if (err != NoError) {
            layers (CreateErrorResponse (err, "Failed to retrieve attribute."));
            continue;
        }

        GS::ObjectState layer;
        layer.Add ("attributeId", CreateGuidObjectState (attr.header.guid));
        layer.Add ("index", GetAttributeIndex (attr.header.index));
        layer.Add ("name", GS::UniString (attr.header.name));

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
                    "type": "string",
                    "enum": ["id", "manufacturer", "description", "connPriority", "cutFillIndex", "cutFillPen", "cutFillBackgroundPen", "cutSurfaceIndex", "cutFillOrientation", "thermalConductivity", "density", "heatCapacity", "embodiedEnergy", "embodiedCarbon", "showUncutLines", "collisionDetection"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    })";
}

GS::Optional<GS::UniString> GetBuildingMaterialsCommand::GetResponseSchema () const
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

        // id/manufacturer/description are GS::UniString* out-parameters: Archicad only fills them in if the
        // caller points them at real storage before the Get call, mirroring header.uniStringNamePtr.
        GS::UniString id, manufacturer, description;
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
        buildingMaterial.Add ("name", GS::UniString (attr.header.name));

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

GS::Optional<GS::UniString> DeleteAttributesCommand::GetResponseSchema () const
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

GS::ObjectState DeleteAttributesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> attributesToDelete;
    parameters.Get ("attributesToDelete", attributesToDelete);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    for (const GS::ObjectState& item : attributesToDelete) {
        GS::UniString typeStr;
        item.Get ("attributeType", typeStr);
        API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);
        if (typeID == API_ZombieAttrID) {
            executionResults (CreateFailedExecutionResult (APIERR_BADPARS,
                GS::UniString::Printf ("Invalid attributeType '%T'.", typeStr.ToPrintf ())));
            continue;
        }

        GS::ObjectState attributeId;
        item.Get ("attributeId", attributeId);
        API_AttributeIndex index;
        if (!GetAttributeIndexFromAttributeId (attributeId, typeID, index)) {
            executionResults (CreateFailedExecutionResult (APIERR_BADID, "Attribute not found."));
            continue;
        }

        API_Attr_Head attrHead = {};
        attrHead.typeID = typeID;
        attrHead.index = index;
#ifdef ServerMainVers_2600
        GSErrCode err = ACAPI_Attribute_Delete (attrHead);
#else
        GSErrCode err = ACAPI_Attribute_Delete (&attrHead);
#endif
        if (err != NoError) {
            executionResults (CreateFailedExecutionResult (err, "Failed to delete."));
            continue;
        }

        executionResults (CreateSuccessfulExecutionResult ());
    }

    return response;
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

GS::Optional<GS::UniString> CreateAttributesCommandBase::GetResponseSchema () const
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

        SetTypeSpecificParameters (data, attr, attrDef);

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_Modify (&attr, &attrDef);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify."));
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_Create (&attr, &attrDef);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create."));
                continue;
            }
        }

	    ACAPI_DisposeAttrDefsHdls (&attrDef);

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

void CreateBuildingMaterialsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
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

void CreateLayersCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
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
                            "$ref": "#/LayersOfLayerCombination"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name",
                        "layers"
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

void CreateLayerCombinationsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
{
    GS::Array<GS::ObjectState> layers;
    parameters.Get ("layers", layers);

    attribute.layerComb.lNumb = layers.GetSize ();

#ifdef ServerMainVers_2700
	attributeDef.layer_statItems = new GS::HashTable<API_AttributeIndex, API_LayerStat> ();
#else
	attributeDef.layer_statItems = (API_LayerStat **) BMAllocateHandle (attribute.layerComb.lNumb * sizeof (API_LayerStat), ALLOCATE_CLEAR, 0);
#endif

    size_t i = 0;
    for (const GS::ObjectState& layer : layers) {
#ifdef ServerMainVers_2700
        UNUSED_VARIABLE(i);
		API_LayerStat layerStat = {};
#else
        API_LayerStat& layerStat = (*attributeDef.layer_statItems)[i++];
#endif
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

        Int32 intersectionGroupNr = 0;
        layer.Get ("intersectionGroupNr", intersectionGroupNr);
        layerStat.conClassId = intersectionGroupNr;

        API_AttributeIndex layerIndex;
        if (GetAttributeIndexFromAttributeId (layer, API_LayerID, layerIndex)) {
#ifdef ServerMainVers_2700
            attributeDef.layer_statItems->Add (layerIndex, layerStat);
#else
            layerStat.lInd = layerIndex;
#endif
        }
    }
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
                                "type": "object",
                                "properties": {
                                    "dash": {
                                        "type": "number",
                                        "description": "Length of the visible part of the item."
                                    },
                                    "gap": {
                                        "type": "number",
                                        "description": "Length of the invisible part of the item."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["dash", "gap"]
                            }
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Symbol items describing one period (Symbol line type only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "itemType": {
                                        "type": "string",
                                        "description": "Separator, CenterDot, CenterLine, Dot, RightAngle, Parallel, Line, Circle, or Arc."
                                    },
                                    "centerOffset": {
                                        "type": "number",
                                        "description": "Vertical distance from the origin. Used for Separator, CenterDot, and CenterLine item types."
                                    },
                                    "length": {
                                        "type": "number",
                                        "description": "Length of the item. Used for CenterLine, RightAngle, and Parallel item types."
                                    },
                                    "begPos": {
                                        "description": "Beginning position. Used for Dot, RightAngle, Parallel, Line, Circle, and Arc item types.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "endPos": {
                                        "description": "End position. Used for Line item type only.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "radius": {
                                        "type": "number",
                                        "description": "Radius. Used for Circle and Arc item types."
                                    },
                                    "beginAngle": {
                                        "type": "number",
                                        "description": "Beginning angle in radians, measured from the vertical axis. Used for Arc item type only."
                                    },
                                    "endAngle": {
                                        "type": "number",
                                        "description": "End angle in radians, measured from the vertical axis. Used for Arc item type only."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["itemType"]
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

void CreateLinesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
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
        return;
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
                        "percent": {
                            "type": "number",
                            "description": "Translucency percentage [0..1] (gradient and some Solid fills)."
                        },
                        "texture": {
                            "description": "Texture parameters (Image and gradient fills). Only name, rotationAngle, xSize, ySize, mirrorX, and mirrorY are used for Fills.",
                            "$ref": "#/Texture"
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Vectorial fill line items (Vector fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "frequency": {
                                        "type": "number",
                                        "description": "The distance between two instances of this item."
                                    },
                                    "direction": {
                                        "type": "number",
                                        "description": "The angle of the item, measured CCW from the horizontal axis, in radians."
                                    },
                                    "offsetLine": {
                                        "type": "number",
                                        "description": "The parallel offset of the item, measured from the (rotated) horizontal axis."
                                    },
                                    "offset": {
                                        "description": "The offset of the item, given by its coordinates.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "lineLengths": {
                                        "type": "array",
                                        "description": "Dash-gap length pairs describing this line item. Must contain an even number of items.",
                                        "items": {
                                            "type": "number"
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["frequency", "direction", "offsetLine", "offset"]
                            }
                        },
                        "symbolLines": {
                            "type": "array",
                            "description": "Line items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "begin": { "$ref": "#/Coordinate2D" },
                                    "end": { "$ref": "#/Coordinate2D" }
                                },
                                "additionalProperties": false,
                                "required": ["begin", "end"]
                            }
                        },
                        "symbolArcs": {
                            "type": "array",
                            "description": "Arc items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "begin": { "$ref": "#/Coordinate2D" },
                                    "origin": { "$ref": "#/Coordinate2D" },
                                    "angle": {
                                        "type": "number",
                                        "description": "Arc angle in radians, measured CCW."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["begin", "origin", "angle"]
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

GS::Optional<GS::UniString> CreateFillsCommand::GetResponseSchema () const
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
                            "description": "Document name of the zone stamp library part (GSM) to use, e.g. the value shown as StampDocumentName in an Attribute Manager XML export. Only used together with stampMainGuid/stampRevGuid; ignored otherwise."
                        },
                        "stampMainGuid": {
                            "type": "string",
                            "description": "Main GUID of the zone stamp library part to use, e.g. the value shown as MainGuid in an Attribute Manager XML export (can be copied from an existing Zone Category obtained another way). If omitted, the stamp is copied from the Zone Category being overwritten, or from the project's first Zone Category when creating a new one."
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

void CreateZoneCategoriesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const
{
    // ACAPI_Attribute_Create/Modify requires zone_addParItems (the zone stamp's GDL parameters) to be filled in
    // for API_ZoneCatID, otherwise it fails with APIERR_BADPARS. We always seed the stamp reference and its
    // parameters from an existing Zone Category: the one being overwritten (attribute.header.index is already
    // resolved at this point), or - when creating a brand new one - the always-present first Zone Category. On
    // overwrite this just reads back the attribute's own current stamp/parameters, so nothing is lost.
    API_AttributeIndex templateIndex = IsPositiveAttributeIndex (attribute.header.index) ? attribute.header.index : ACAPI_CreateAttributeIndex (1);

    API_Attribute templateAttr = {};
    templateAttr.header.typeID = API_ZoneCatID;
    templateAttr.header.index = templateIndex;
    if (ACAPI_Attribute_Get (&templateAttr) == NoError) {
        GS::ucscpy (attribute.zoneCat.stampName, templateAttr.zoneCat.stampName);
        attribute.zoneCat.stampMainGuid = templateAttr.zoneCat.stampMainGuid;
        attribute.zoneCat.stampRevGuid = templateAttr.zoneCat.stampRevGuid;
    }

    ACAPI_Attribute_GetDef (API_ZoneCatID, templateIndex, &attributeDef);

    // Explicit stamp reference (e.g. copied from another Zone Category's stampMainGuid/stampRevGuid/stampName,
    // as shown in an Attribute Manager XML export) overrides the template copied above. Note that
    // zone_addParItems (the GDL parameter values) still come from the template, not from this stamp - there is
    // no public API to fetch a library part's own default parameters to seed them correctly for a stamp that
    // differs from the template.
    GS::UniString stampMainGuid, stampRevGuid;
    if (parameters.Get ("stampMainGuid", stampMainGuid) && parameters.Get ("stampRevGuid", stampRevGuid)) {
        attribute.zoneCat.stampMainGuid = APIGuidFromString (stampMainGuid.ToCStr ());
        attribute.zoneCat.stampRevGuid = APIGuidFromString (stampRevGuid.ToCStr ());
        SetUCharProperty (&parameters, "stampName", attribute.zoneCat.stampName);
    }

    SetUCharProperty (&parameters, "categoryCode", attribute.zoneCat.catCode);
    GetColor (parameters, "color", attribute.zoneCat.rgb);
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
                            "type": "string",
                            "description": "Ventilation, Piping, or CableCarrier. Only elements belonging to the same domain can use this system."
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

void CreateMEPSystemsCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
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

void CreateSurfacesCommand::SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef&) const
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
                                "type": "object",
                                "properties": {
                                    "index": {
                                        "type": "integer",
                                        "description": "Index of the pen [1..255]."
                                    },
                                    "color": {
                                        "$ref": "#/ColorRGB"
                                    },
                                    "width": {
                                        "type": "number",
                                        "description": "Thickness of the pen defined in paper millimeters."
                                    },
                                    "description": {
                                        "type": "string",
                                        "description": "Textual description of the pen."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["index"]
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

GS::Optional<GS::UniString> CreatePenTablesCommand::GetResponseSchema () const
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
                    "description": "Data to create a Profile. Its geometry (the cross-section shape) comes from sourceAttributeId (an existing Profile's geometry, copied), from newSkins (AC27+ only, caller-supplied polygon geometry), or both combined - at least one of the two must be given. skinOverrides and newSkins' edgeOverrides target skins/edges by the identifiers/indices GetProfiles reports.",
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
                                            "type": "object",
                                            "properties": {
                                                "edgeIndex": {
                                                    "type": "integer"
                                                },
                                                "pen": {
                                                    "type": "integer"
                                                },
                                                "isVisibleLine": {
                                                    "type": "boolean"
                                                },
                                                "lineTypeId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["edgeIndex"]
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
                                "type": "object",
                                "description": "One new skin (hatch). Its shape is one or more closed polygon contours: the first is the outer boundary, any further ones are holes cut out of it - the same polygon+holes convention as e.g. CreateSlabs' polygonCoordinates/polygonArcs/holes, just expressed as a list of contours instead of a separate holes array.",
                                "properties": {
                                    "contours": {
                                        "type": "array",
                                        "description": "Closed polygon contours forming this skin's cross-section, in the Profile's local coordinate system. Each contour is closed automatically - do not repeat its first vertex at the end.",
                                        "items": {
                                            "type": "object",
                                            "properties": {
                                                "polygonCoordinates": {
                                                    "type": "array",
                                                    "description": "The 2D coordinates of this contour.",
                                                    "items": {
                                                        "$ref": "#/Coordinate2D"
                                                    },
                                                    "minItems": 3
                                                },
                                                "polygonArcs": {
                                                    "type": "array",
                                                    "description": "Optional arcs along this contour's edges. begIndex/endIndex are 0-based positions within this contour's own polygonCoordinates.",
                                                    "items": {
                                                        "$ref": "#/PolyArc"
                                                    }
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["polygonCoordinates"]
                                        },
                                        "minItems": 1
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
                                        "description": "Per-edge pen/visibility/line type, targeted by 0-based edge index. Edge indices follow the same order as this skin's contours/polygonCoordinates: the outer contour's edges first (one edge per vertex, wrapping around), then each hole's, in the order the contours were given. Verify exact indices for a created skin via a follow-up GetProfiles call's skins[].edges before relying on them.",
                                        "items": {
                                            "type": "object",
                                            "properties": {
                                                "edgeIndex": {
                                                    "type": "integer"
                                                },
                                                "pen": {
                                                    "type": "integer"
                                                },
                                                "isVisibleLine": {
                                                    "type": "boolean"
                                                },
                                                "lineTypeId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["edgeIndex"]
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["contours"]
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
                "description": "Overwrite the Profile if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "profileDataArray"
        ]
    })";
}

GS::Optional<GS::UniString> CreateProfilesCommand::GetResponseSchema () const
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
        if (!hasSource && newSkins.IsEmpty ()) {
            attributeIds (CreateErrorResponse (APIERR_BADPARS, "Either sourceAttributeId or newSkins (or both) is required."));
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
        if (!hasSource) {
            profileDefs.profile_vectorImageItems = &scratchImage;
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
                            "description" : "Array of types the composite can used with.",
                            "items": {
                                "type": "string",
                                "description": "Element type (Wall, Slab, Roof, or Shell)"
                            }
                        },
                        "skins": {
                            "type": "array",
                            "description" : "Array of skin data.",
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
                            "description" : "Array of skin separator data. The number of items must be the number of skins plus one.",
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
                        "name",
                        "skins",
                        "separators"
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

GS::Optional<GS::UniString> CreateCompositesCommand::GetResponseSchema () const
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

        GS::Array<GS::UniString> useWith;
        compositeData.Get ("useWith", useWith);
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

        GS::Array<GS::ObjectState> skins;
        compositeData.Get ("skins", skins);

        GS::Array<GS::ObjectState> separators;
        compositeData.Get ("separators", separators);

        if (skins.IsEmpty ()) {
            attributeIds (CreateErrorResponse (Error, "Skin array is empty."));
            continue;
        }
        if (separators.GetSize () != skins.GetSize () + 1) {
            attributeIds (CreateErrorResponse (Error, "Invalid separator count."));
            continue;
        }

        UInt32 componentCount = skins.GetSize ();
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