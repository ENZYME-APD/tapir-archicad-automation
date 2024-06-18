#include "AttributeCommands.hpp"
#include "MigrationHelper.hpp"

static bool GetAttributeIndexFromAttributeId (const GS::ObjectState& attributeId, API_AttrTypeID attributeType, API_AttributeIndex& attributeIndex)
{
    GS::ObjectState attributeIdInner;
    if (!attributeId.Get ("attributeId", attributeIdInner)) {
        return false;
    }

    GS::String attributeGuidString;
    if (!attributeIdInner.Get ("guid", attributeGuidString)) {
        return false;
    }

    API_Attribute attribute = {};
    attribute.header.guid = APIGuidFromString (attributeGuidString.ToCStr ());
    attribute.header.typeID = attributeType;
    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return false;
    }

    attributeIndex = attribute.header.index;
    return true;
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
                "type": "array",
                "description" : "Physical properties list.",
                "items": {
                    "type": "object",
                    "properties": {
                        "properties": {
                            "type": "object",
                            "description": "Physical properties.",
                            "properties": {
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
                                }
                            }
                        }
                    }
                }
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

CreateBuildingMaterialsCommand::CreateBuildingMaterialsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateBuildingMaterialsCommand::GetName () const
{
    return "CreateBuildingMaterials";
}

GS::Optional<GS::UniString> CreateBuildingMaterialsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "buildingMaterialDataArray": {
                "type": "array",
                "description" : "Array of data to create Building Materials.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Building Material.",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Name."
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
                "description": "Overwrite the Building Material if exists with the same name. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "buildingMaterialDataArray"
        ]
    })";
}

GS::Optional<GS::UniString> CreateBuildingMaterialsCommand::GetResponseSchema () const
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

GS::ObjectState CreateBuildingMaterialsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> buildingMaterialDataArray;
    parameters.Get ("buildingMaterialDataArray", buildingMaterialDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeIds = response.AddList<GS::ObjectState> ("attributeIds");

    for (const GS::ObjectState& buildingMaterialData : buildingMaterialDataArray) {
        API_Attribute buildMat = {};
        buildMat.header.typeID = API_BuildingMaterialID;

        GS::UniString name;
        buildingMaterialData.Get ("name", name);
        buildMat.header.uniStringNamePtr = &name;

        bool doesExist = (ACAPI_Attribute_Get (&buildMat) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeIds (CreateErrorResponse (Error, "Building Material already exists."));
            continue;
        }

        GS::UniString id;
        if (buildingMaterialData.Get ("id", id)) {
            buildMat.buildingMaterial.id = &id;
        }

        GS::UniString manufacturer;
        if (buildingMaterialData.Get ("manufacturer", manufacturer)) {
            buildMat.buildingMaterial.manufacturer = &manufacturer;
        }

        GS::UniString description;
        if (buildingMaterialData.Get ("description", description)) {
            buildMat.buildingMaterial.description = &description;
        }

        Int32 cutFillIndex;
        if (buildingMaterialData.Get ("cutFillIndex", cutFillIndex)) {
            buildMat.buildingMaterial.cutFill = ACAPI_CreateAttributeIndex (cutFillIndex);
        }

        Int32 connPriority;
        if (buildingMaterialData.Get ("connPriority", connPriority)) {
            ACAPI_Element_UI2ElemPriority (&connPriority, &buildMat.buildingMaterial.connPriority);
        }

        short cutFillPen;
        if (buildingMaterialData.Get ("cutFillPen", cutFillPen)) {
            buildMat.buildingMaterial.cutFillPen = cutFillPen;
        }

        short cutFillBackgroundPen;
        if (buildingMaterialData.Get ("cutFill", cutFillBackgroundPen)) {
            buildMat.buildingMaterial.cutFillBackgroundPen = cutFillBackgroundPen;
        }

        Int32 cutSurfaceIndex;
        if (buildingMaterialData.Get ("cutSurfaceIndex", cutSurfaceIndex)) {
            buildMat.buildingMaterial.cutMaterial = ACAPI_CreateAttributeIndex (cutFillIndex);
        }

        double thermalConductivity;
        if (buildingMaterialData.Get ("thermalConductivity", thermalConductivity)) {
            buildMat.buildingMaterial.thermalConductivity = thermalConductivity;
        }

        double density;
        if (buildingMaterialData.Get ("density", density)) {
            buildMat.buildingMaterial.density = density;
        }

        double heatCapacity;
        if (buildingMaterialData.Get ("heatCapacity", heatCapacity)) {
            buildMat.buildingMaterial.heatCapacity = heatCapacity;
        }

        double embodiedEnergy;
        if (buildingMaterialData.Get ("embodiedEnergy", embodiedEnergy)) {
            buildMat.buildingMaterial.embodiedEnergy = embodiedEnergy;
        }

        double embodiedCarbon;
        if (buildingMaterialData.Get ("embodiedCarbon", embodiedCarbon)) {
            buildMat.buildingMaterial.embodiedCarbon = embodiedCarbon;
        }

        if (doesExist) {
            GSErrCode err = ACAPI_Attribute_Modify (&buildMat, nullptr);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to modify attribute."));
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_Create (&buildMat, nullptr);
            if (err != NoError) {
                attributeIds (CreateErrorResponse (err, "Failed to create attribute."));
                continue;
            }
        }

        GS::ObjectState attributeId;
        attributeId.Add ("guid", APIGuidToString (buildMat.header.guid));

        GS::ObjectState attributeIdArrayItem;
        attributeIdArrayItem.Add ("attributeId", attributeId);

        attributeIds (attributeIdArrayItem);
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
                    "description": "Data to create a Building Material.",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Name."
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
                "description" : "Overwrite the Composite if exists with the same name. The default is false."
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
        compositeData.Get ("name", name);
        composite.header.uniStringNamePtr = &name;

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

        GS::ObjectState attributeId;
        attributeId.Add ("guid", APIGuidToString (composite.header.guid));

        GS::ObjectState attributeIdArrayItem;
        attributeIdArrayItem.Add ("attributeId", attributeId);

        attributeIds (attributeIdArrayItem);
        ACAPI_DisposeAttrDefsHdlsExt (&compositeDefs);
    }

    return response;
}
