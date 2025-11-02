#include "AttributeCommands.hpp"
#include "MigrationHelper.hpp"

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
        "type": "object",
        "properties": {
            "attributes" : {
                "type": "array",
                "description" : "Details of attributes.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeId": {
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "number",
                            "description": "Index of the attribute."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name of the attribute."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "attributeId",
                        "index",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributes"
        ]
    })";
}

GS::ObjectState GetAttributesByTypeCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString typeStr;
    parameters.Get ("attributeType", typeStr);

    API_AttrTypeID typeID = ConvertAttributeTypeStringToID (typeStr);

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
            GSErrCode err = ACAPI_Attribute_Create (&attr, nullptr);
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
    if (parameters.Get ("cutFill", cutFillBackgroundPen)) {
        attribute.buildingMaterial.cutFillBackgroundPen = cutFillBackgroundPen;
    }

    Int32 cutSurfaceIndex;
    if (parameters.Get ("cutSurfaceIndex", cutSurfaceIndex)) {
        attribute.buildingMaterial.cutMaterial = ACAPI_CreateAttributeIndex (cutFillIndex);
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
                        "name",
                        "materialType",
                        "ambientReflection",
                        "diffuseReflection",
                        "specularReflection",
                        "transparency",
                        "shine",
                        "transparencyAttenuation",
                        "emissionAttenuation",
                        "surfaceColor",
                        "specularColor",
                        "emissionColor"
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

    short ambientReflection;
    if (parameters.Get ("ambientReflection", ambientReflection)) {
        attribute.material.ambientPc = ambientReflection;
    }

    short diffuseReflection;
    if (parameters.Get ("diffuseReflection", diffuseReflection)) {
        attribute.material.diffusePc = diffuseReflection;
    }

    short specularReflection;
    if (parameters.Get ("specularReflection", specularReflection)) {
        attribute.material.specularPc = specularReflection;
    }

    short transparency;
    if (parameters.Get ("transparency", transparency)) {
        attribute.material.transpPc = transparency;
    }

    short shine;
    if (parameters.Get ("shine", shine)) {
        attribute.material.shine = shine;
    }

    short transparencyAttenuation;
    if (parameters.Get ("transparencyAttenuation", transparencyAttenuation)) {
        attribute.material.transpAtt = transparencyAttenuation;
    }

    short emissionAttenuation;
    if (parameters.Get ("emissionAttenuation", emissionAttenuation)) {
        attribute.material.emissionAtt = emissionAttenuation;
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