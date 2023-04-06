#include "AttributeCommands.hpp"

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
                        "cutFill": {
                            "type": "integer",
                            "description": "Cut Fill."
                        },
                        "cutFillPen": {
                            "type": "integer",
                            "description": "Cut Fill Foreground Pen."
                        },
                        "cutFillBackgroundPen": {
                            "type": "integer",
                            "description": "Cut Fill Background Pen."
                        },
                        "cutMaterial": {
                            "type": "integer",
                            "description": "Cut Surface."
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

GS::ObjectState CreateBuildingMaterialsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> buildingMaterialDataArray;
    parameters.Get ("buildingMaterialDataArray", buildingMaterialDataArray);

    bool overwriteExisting = false;
    parameters.Get ("overwriteExisting", overwriteExisting);

    GS::ObjectState response;
    const auto& attributeList = response.AddList<GS::ObjectState> ("attributes");

    for (const GS::ObjectState& buildingMaterialData : buildingMaterialDataArray) {
        API_Attribute buildMat = {};
        buildMat.header.typeID = API_BuildingMaterialID;

        GS::UniString name;
        buildingMaterialData.Get ("name", name);
        buildMat.header.uniStringNamePtr = &name;

        bool doesExist = (ACAPI_Attribute_Get (&buildMat) == NoError);
        if (doesExist && !overwriteExisting) {
            attributeList (CreateErrorResponse (Error, "Building Material already exists."));
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

        API_AttributeIndex cutFill;
        if (buildingMaterialData.Get ("cutFill", cutFill)) {
            buildMat.buildingMaterial.cutFill = cutFill;
        }

        Int32 connPriority;
        if (buildingMaterialData.Get ("connPriority", connPriority)) {
            ACAPI_Goodies (APIAny_UI2ElemPriorityID, (void*) &connPriority, &buildMat.buildingMaterial.connPriority);
        }

        short cutFillPen;
        if (buildingMaterialData.Get ("cutFillPen", cutFillPen)) {
            buildMat.buildingMaterial.cutFillPen = cutFillPen;
        }

        short cutFillBackgroundPen;
        if (buildingMaterialData.Get ("cutFill", cutFillBackgroundPen)) {
            buildMat.buildingMaterial.cutFillBackgroundPen = cutFillBackgroundPen;
        }

        API_AttributeIndex cutMaterial;
        if (buildingMaterialData.Get ("cutMaterial", cutMaterial)) {
            buildMat.buildingMaterial.cutMaterial = cutMaterial;
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
                attributeList (CreateErrorResponse (err, "Failed to modify attribute."));
                continue;
            }
        } else {
            GSErrCode err = ACAPI_Attribute_Create (&buildMat, nullptr);
            if (err != NoError) {
                attributeList (CreateErrorResponse (err, "Failed to create attribute."));
                continue;
            }
        }

        GS::ObjectState attributeId;
        attributeId.Add ("guid", APIGuidToString (buildMat.header.guid));

        GS::ObjectState attributeIdArrayItem;
        attributeIdArrayItem.Add ("attributeId", attributeId);

        attributeList (attributeIdArrayItem);
    }

    return response;
}
