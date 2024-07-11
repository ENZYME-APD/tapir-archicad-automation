#include "ElementCommands.hpp"
#include "MigrationHelper.hpp"

GetSelectedElementsCommand::GetSelectedElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetSelectedElementsCommand::GetName () const
{
    return "GetSelectedElements";
}

GS::Optional<GS::UniString> GetSelectedElementsCommand::GetResponseSchema () const
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

GS::ObjectState GetSelectedElementsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    API_SelectionInfo selectionInfo;
    GS::Array<API_Neig> selectedNeigs;
    GSErrCode err = ACAPI_Selection_Get (&selectionInfo, &selectedNeigs, false);
    if (err != NoError && err != APIERR_NOSEL) {
        return CreateErrorResponse (err, "Failed to retrieve selected elements.");
    }

    GS::ObjectState response;
    const auto& elementsList = response.AddList<GS::ObjectState> ("elements");
    if (err == APIERR_NOSEL || selectionInfo.typeID == API_SelEmpty) {
        return response;
    }

    GS::Array<API_Guid> selElemGuids;
    for (API_Neig& selectedNeig : selectedNeigs) {
        API_ElemTypeID elemTypeID = API_ZombieElemID;
#ifdef ServerMainVers_2600
        API_ElemType apiElemType;
        if (ACAPI_Element_NeigIDToElemType (selectedNeig.neigID, apiElemType) != NoError) {
            continue;
        }
        elemTypeID = apiElemType.typeID;
#else
        if (ACAPI_Goodies (APIAny_NeigIDToElemTypeID, &selectedNeig.neigID, &elemTypeID) != NoError) {
            continue;
        }
#endif

        API_Guid elemGuid = selectedNeig.guid;
        if (elemTypeID == API_SectElemID) {
            API_Element element = {};
            element.header.guid = selectedNeig.guid;
            if (ACAPI_Element_Get (&element) != NoError) {
                continue;
            }
            elemGuid = element.sectElem.parentGuid;
        }

        GS::ObjectState elemId;
        elemId.Add ("guid", APIGuidToString (elemGuid));

        GS::ObjectState elemIdArrayItem;
        elemIdArrayItem.Add ("elementId", elemId);

        elementsList (elemIdArrayItem);
    }

    return response;
}

MoveElementsCommand::MoveElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveElementsCommand::GetName () const
{
    return "MoveElements";
}

GS::Optional<GS::UniString> MoveElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsWithMoveVectors": {
                "type": "array",
                "description": "The elements with move vector pairs.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "moveVector": {
                            "type": "object",
                            "description" : "Move vector of a 3D point.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the vector."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the vector."
                                },
                                "z" : {
                                    "type": "number",
                                    "description" : "Z value of the vector."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y",
                                "z"
                            ]
                        },
                        "copy": {
                            "type": "boolean",
                            "description" : "Optional parameter. If true, then a copy of the element will be moved. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "moveVector"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithMoveVectors"
        ]
    })";
}

static GSErrCode MoveElement (const API_Guid& elemGuid, const API_Vector3D& moveVector, bool withCopy)
{
    GS::Array<API_Neig> elementsToEdit = { API_Neig (elemGuid) };

    API_EditPars editPars = {};
    editPars.typeID = APIEdit_Drag;
    editPars.endC = moveVector;
    editPars.withDelete = !withCopy;

    return ACAPI_Element_Edit (&elementsToEdit, editPars);
}

GS::ObjectState	MoveElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithMoveVectors;
    parameters.Get ("elementsWithMoveVectors", elementsWithMoveVectors);

    API_Guid elemGuid;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Move Elements", [&] () -> GSErrCode {
        for (const GS::ObjectState& elementWithMoveVector : elementsWithMoveVectors) {
            const GS::ObjectState* elementId = elementWithMoveVector.Get ("elementId");
            const GS::ObjectState* moveVector = elementWithMoveVector.Get ("moveVector");
            if (elementId == nullptr || moveVector == nullptr) {
                continue;
            }

            elemGuid = GetGuidFromObjectState (*elementId);

            bool copy = false;
            elementWithMoveVector.Get ("copy", copy);

            const GSErrCode err = MoveElement (elemGuid,
                                               Get3DCoordinateFromObjectState (*moveVector),
                                               copy);
            if (err != NoError) {
                return err;
            }
        }

        return NoError;
    });

    if (err != NoError) {
        const GS::UniString errorMsg = GS::UniString::Printf ("Failed to move element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

GetGDLParametersOfElementsCommand::GetGDLParametersOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGDLParametersOfElementsCommand::GetName () const
{
    return "GetGDLParametersOfElements";
}

GS::Optional<GS::UniString> GetGDLParametersOfElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetGDLParametersOfElementsCommand::GetResponseSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "gdlParametersOfElements": {
            "type": "array",
            "description": "The GDL parameters of elements.",
            "items": {
                "$ref": "#/GDLParametersDictionary"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "gdlParametersOfElements"
    ]
})";
}

static GS::UniString ConvertAddParIDToString (API_AddParID addParID)
{
    switch (addParID) {
        case APIParT_Integer:			return "Integer";
        case APIParT_Length:			return "Length";
        case APIParT_Angle:				return "Angle";
        case APIParT_RealNum:			return "RealNum";
        case APIParT_LightSw:			return "LightSwitch";
        case APIParT_ColRGB:			return "ColorRGB";
        case APIParT_Intens:			return "Intens";
        case APIParT_LineTyp:			return "LineType";
        case APIParT_Mater:				return "Material";
        case APIParT_FillPat:			return "FillPattern";
        case APIParT_PenCol:			return "PenCol";
        case APIParT_CString:			return "String";
        case APIParT_Boolean:			return "Boolean";
        case APIParT_Separator:			return "Separator";
        case APIParT_Title:				return "Title";
        case APIParT_BuildingMaterial:	return "BuildingMaterial";
        case APIParT_Profile:			return "Profile";
        case APIParT_Dictionary:		return "Dictionary";
        default:						return "UNKNOWN";
    }
}

#ifndef ServerMainVers_2700
static API_AttrTypeID ConvertAddParIDToAttrTypeID (API_AddParID addParID)
{
    switch (addParID) {
        case APIParT_FillPat:			return API_FilltypeID;
        case APIParT_Mater:				return API_MaterialID;
        case APIParT_BuildingMaterial:	return API_BuildingMaterialID;
        case APIParT_Profile:			return API_ProfileID;
        case APIParT_LineTyp:			return API_LinetypeID;
        default:						return API_ZombieAttrID;
    }
}
#endif

#ifndef ServerMainVers_2700
static GS::UniString GetAttributeName (API_AttrTypeID	  typeID,
                                        API_AttributeIndex index)
{
    API_Attribute	attrib = {};

    GS::UniString name;
    attrib.header.typeID = typeID;
    attrib.header.index = index;
    attrib.header.uniStringNamePtr = &name;

    ACAPI_Attribute_Get (&attrib);

    if (typeID == API_MaterialID && attrib.material.texture.fileLoc != nullptr) {
        delete attrib.material.texture.fileLoc;
        attrib.material.texture.fileLoc = nullptr;
    }

    return name;
}
#endif

#ifndef ServerMainVers_2700
static GS::ObjectState GetAttributeObjectState (API_AttrTypeID	   typeID,
                                                    API_AttributeIndex index)
{
    GS::ObjectState attribute;
    attribute.Add ("index", index);
    attribute.Add ("name", GetAttributeName (typeID, index));
    return attribute;
}
#endif

static void AddValueInteger (GS::ObjectState& 	   gdlParameterDetails,
                             const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", static_cast<Int32> (actParam.value.real));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<Int32> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*)*actParam.value.array) [arrayIndex++]));
            }
        }
    }
}

static void AddValueDouble (GS::ObjectState& 	  gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", actParam.value.real);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<double> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (((double*)*actParam.value.array) [arrayIndex++]);
            }
        }
    }
}

#ifndef ServerMainVers_2700
static void AddValueAttribute (GS::ObjectState& 	 gdlParameterDetails,
                               const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value",
                                 GetAttributeObjectState (ConvertAddParIDToAttrTypeID (actParam.typeID),
                                                          static_cast<API_AttributeIndex> (actParam.value.real)));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<GS::ObjectState> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (GetAttributeObjectState (ConvertAddParIDToAttrTypeID (actParam.typeID),
                                                              static_cast<API_AttributeIndex> (((double*)*actParam.value.array) [arrayIndex++])));
            }
        }
    }
}
#endif

template<typename T>
static void AddValueTrueFalseOptions (GS::ObjectState& 	    gdlParameterDetails,
                                      const API_AddParType& actParam,
                                      T optionTrue,
                                      T optionFalse)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", static_cast<Int32> (actParam.value.real) == 0 ? optionFalse : optionTrue);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<T> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*)*actParam.value.array) [arrayIndex++]) == 0 ? optionFalse : optionTrue);
            }
        }
    }
}

static void AddValueOnOff (GS::ObjectState& 	 gdlParameterDetails,
                           const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, GS::String ("On"), GS::String ("Off"));
}

static void AddValueBool (GS::ObjectState& 	    gdlParameterDetails,
                          const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, true, false);
}

static void AddValueString (GS::ObjectState& 	  gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add ("value", GS::UniString (actParam.value.uStr));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<GS::UniString> ("value");
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                GS::uchar_t* uValueStr = (reinterpret_cast<GS::uchar_t*>(*actParam.value.array)) + arrayIndex;
                arrayIndex += GS::ucslen32 (uValueStr) + 1;
                arrayValueItemAdder (GS::UniString (uValueStr));
            }
        }
    }
}

constexpr const char* ParameterValueFieldName = "value";

static void SetParamValueInteger (API_ChangeParamType&   changeParam,
                                  const GS::ObjectState& parameterDetails)
{
    Int32 value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueDouble (API_ChangeParamType&   changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    double value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = value;
}

static void SetParamValueOnOff (API_ChangeParamType&   changeParam,
                                const GS::ObjectState& parameterDetails)
{
    GS::String value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value == "Off" ? 0 : 1);
}

static void SetParamValueBool (API_ChangeParamType&   changeParam,
                               const GS::ObjectState& parameterDetails)
{
    bool value;
    parameterDetails.Get (ParameterValueFieldName, value);
    changeParam.realValue = (value ? 0 : 1);
}

static void SetParamValueString (API_ChangeParamType&   changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    GS::UniString value;
    parameterDetails.Get (ParameterValueFieldName, value);

    constexpr USize MaxStrValueLength = 512;

    static GS::uchar_t strValuePtr[MaxStrValueLength];
    GS::ucscpy (strValuePtr, value.ToUStr (0, GS::Min(value.GetLength (), MaxStrValueLength)).Get ());

    changeParam.uStrValue = strValuePtr;
}

GS::ObjectState	GetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& listAdder = response.AddList<GS::ObjectState> ("gdlParametersOfElements");

    API_Guid elemGuid;
    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            continue;
        }

        elemGuid = GetGuidFromObjectState (*elementId);

        API_ParamOwnerType paramOwner = {};
        paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
        paramOwner.type   = API_ObjectID;
#else
        paramOwner.typeID = API_ObjectID;
#endif
        paramOwner.guid   = elemGuid;

        API_ElementMemo memo = {};
        const GSErrCode err = ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_AddPars);
        if (err == NoError) {
            const GSSize nParams = BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType);
            GS::ObjectState gdlParametersDictionary;
            for (GSIndex ii = 0; ii < nParams; ++ii) {
                const API_AddParType& actParam = (*memo.params)[ii];

                if (actParam.typeID == APIParT_Separator) {
                    continue;
                }

                GS::ObjectState gdlParameterDetails;
                gdlParameterDetails.Add ("index", actParam.index);
                gdlParameterDetails.Add ("type", ConvertAddParIDToString (actParam.typeID));
                if (actParam.typeMod == API_ParArray) {
                    gdlParameterDetails.Add ("dimension1", actParam.dim1);
                    gdlParameterDetails.Add ("dimension2", actParam.dim2);
                }

                switch (actParam.typeID) {
                    case APIParT_Integer:
                    case APIParT_PenCol:			AddValueInteger (gdlParameterDetails, actParam);	break;
                    case APIParT_ColRGB:
                    case APIParT_Intens:
                    case APIParT_Length:
                    case APIParT_RealNum:
                    case APIParT_Angle:				AddValueDouble (gdlParameterDetails, actParam);		break;
                    case APIParT_LightSw:			AddValueOnOff (gdlParameterDetails, actParam); 		break;
                    case APIParT_Boolean: 			AddValueBool (gdlParameterDetails, actParam);		break;
                    case APIParT_LineTyp:
                    case APIParT_Mater:
                    case APIParT_FillPat:
                    case APIParT_BuildingMaterial:
#ifndef ServerMainVers_2700
                    case APIParT_Profile: 			AddValueAttribute (gdlParameterDetails, actParam);	break;
#endif
                    case APIParT_CString:
                    case APIParT_Title: 			AddValueString (gdlParameterDetails, actParam);		break;
                    default:
                    case APIParT_Dictionary:
                        // Not supported by the Archicad API yet
                        break;
                }

                gdlParametersDictionary.Add (actParam.name, gdlParameterDetails);
            }
            listAdder (gdlParametersDictionary);
            ACAPI_DisposeAddParHdl (&memo.params);
        } else {
            const GS::UniString errorMsg = GS::UniString::Printf ("Failed to get parameters of element with guid %T!", APIGuidToString (elemGuid).ToPrintf ());
            return CreateErrorResponse (err, errorMsg);
        }
    }

    return response;
}

SetGDLParametersOfElementsCommand::SetGDLParametersOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetGDLParametersOfElementsCommand::GetName () const
{
    return "SetGDLParametersOfElements";
}

GS::Optional<GS::UniString> SetGDLParametersOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "elementsWithGDLParameters": {
            "type": "array",
            "description": "The elements with GDL parameters dictionary pairs.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "gdlParameters": {
                        "$ref": "#/GDLParametersDictionary"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "gdlParameters"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "elementsWithGDLParameters"
    ]
})";
}

GS::ObjectState	SetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithGDLParameters;
    parameters.Get ("elementsWithGDLParameters", elementsWithGDLParameters);

    bool invalidParameter = false;
    bool notAbleToChangeParameter = false;
    GS::String badParameterName;

    API_Guid elemGuid;
    const GSErrCode err = ACAPI_CallUndoableCommand ("Set GDL Parameters of Elements", [&] () -> GSErrCode {
        GSErrCode err = NoError;
        for (const GS::ObjectState& elementWithGDLParameters : elementsWithGDLParameters) {
            const GS::ObjectState* elementId = elementWithGDLParameters.Get ("elementId");
            if (elementId == nullptr) {
                continue;
            }

            const GS::ObjectState* gdlParameters = elementWithGDLParameters.Get ("gdlParameters");
            if (gdlParameters == nullptr) {
                continue;
            }

            elemGuid = GetGuidFromObjectState (*elementId);

            API_ParamOwnerType   paramOwner = {};

            paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
            paramOwner.type   = API_ObjectID;
#else
            paramOwner.typeID  = API_ObjectID;
#endif
            paramOwner.guid   = elemGuid;

            err = ACAPI_LibraryPart_OpenParameters (&paramOwner);
            if (err == NoError) {
                API_GetParamsType getParams = {};
                err = ACAPI_LibraryPart_GetActParameters (&getParams);
                if (err == NoError) {
                    const GSSize nParams = BMGetHandleSize ((GSHandle) getParams.params) / sizeof (API_AddParType);
                    GS::HashTable<GS::String, API_AddParID> gdlParametersTypeDictionary;
                    for (GSIndex ii = 0; ii < nParams; ++ii) {
                        const API_AddParType& actParam = (*getParams.params)[ii];

                        if (actParam.typeID != APIParT_Separator) {
                            gdlParametersTypeDictionary.Add (GS::String (actParam.name), actParam.typeID);
                        }
                    }

                    GS::HashTable<GS::String, const GS::ObjectState*> changeParamsDictionary;
                    gdlParameters->EnumerateFields ([&] (const GS::String& parameterName) {
                        changeParamsDictionary.Add (parameterName, gdlParameters->Get (parameterName));
                    });

                    API_ChangeParamType changeParam = {};
                    for (const auto& kv : changeParamsDictionary) {
#ifdef ServerMainVers_2800
                        const GS::String& parameterName = kv.key;
                        const GS::ObjectState& parameterDetails = *kv.value;
#else
                        const GS::String& parameterName = *kv.key;
                        const GS::ObjectState& parameterDetails = **kv.value;
#endif

                        if (!gdlParametersTypeDictionary.ContainsKey (parameterName)) {
                            invalidParameter = true;
                            badParameterName = parameterName;
                            return APIERR_BADPARS;
                        }

                        CHTruncate (parameterName.ToCStr (), changeParam.name, sizeof (changeParam.name));
                        if (parameterDetails.Contains ("index1")) {
                            parameterDetails.Get ("index1", changeParam.ind1);
                            if (parameterDetails.Contains ("index2")) {
                                parameterDetails.Get ("index2", changeParam.ind2);
                            }
                        }

                        switch (gdlParametersTypeDictionary[parameterName]) {
                            case APIParT_Integer:
                            case APIParT_PenCol:			SetParamValueInteger (changeParam, parameterDetails); break;
                            case APIParT_ColRGB:
                            case APIParT_Intens:
                            case APIParT_Length:
                            case APIParT_RealNum:
                            case APIParT_Angle:				SetParamValueDouble (changeParam, parameterDetails);  break;
                            case APIParT_LightSw:			SetParamValueOnOff (changeParam, parameterDetails);   break;
                            case APIParT_Boolean: 			SetParamValueBool (changeParam, parameterDetails);    break;
                            case APIParT_LineTyp:
                            case APIParT_Mater:
                            case APIParT_FillPat:
                            case APIParT_BuildingMaterial:
                            case APIParT_Profile: 			SetParamValueInteger (changeParam, parameterDetails); break;
                            case APIParT_CString:
                            case APIParT_Title: 			SetParamValueString (changeParam, parameterDetails);  break;
                            default:
                            case APIParT_Dictionary:
                                // Not supported by the Archicad API yet
                                break;
                        }

                        err = ACAPI_LibraryPart_ChangeAParameter (&changeParam);
                        if (err != NoError) {
                            notAbleToChangeParameter = true;
                            badParameterName = parameterName;
                            return APIERR_BADPARS;
                        }

                        ACAPI_DisposeAddParHdl (&getParams.params);
                        ACAPI_LibraryPart_GetActParameters (&getParams);
                    }

                    API_Element	element = {};
                    element.header.guid = elemGuid;

                    err = ACAPI_Element_Get (&element);
                    if (err == NoError) {
                        API_Element 	mask = {};
                        API_ElementMemo memo = {};

                        ACAPI_ELEMENT_MASK_CLEAR (mask);
#ifdef ServerMainVers_2600
                        switch (element.header.type.typeID) {
#else
                        switch (element.header.typeID) {
#endif
                            case API_ObjectID:
                                element.object.xRatio = getParams.a;
                                element.object.yRatio = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, xRatio);
                                ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, yRatio);
                                break;
                            case API_WindowID:
                            case API_DoorID:
                                element.window.openingBase.width  = getParams.a;
                                element.window.openingBase.height = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
                                ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
                                break;
                            case API_SkylightID:
                                element.skylight.openingBase.width  = getParams.a;
                                element.skylight.openingBase.height = getParams.b;
                                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.width);
                                ACAPI_ELEMENT_MASK_SET (mask, API_SkylightType, openingBase.height);
                                break;
                            default:
                                // Not supported yet
                                break;
                        }

                        memo.params = getParams.params;
                        err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_AddPars, true);
                    }
                }
                ACAPI_LibraryPart_CloseParameters ();
                ACAPI_DisposeAddParHdl (&getParams.params);
            }
        }

        return err;
    });

    if (err != NoError) {
        GS::UniString errorMsg;
        if (invalidParameter) {
            errorMsg = GS::UniString::Printf ("Invalid input: %s is not a GDL parameter of element %T", badParameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
        } else if (notAbleToChangeParameter) {
            errorMsg = GS::UniString::Printf ("Failed to change parameter %s of element with guid %T", badParameterName.ToCStr (), APIGuidToString (elemGuid).ToPrintf ());
        } else {
            errorMsg = GS::UniString::Printf ("Failed to change parameters of element with guid %T", APIGuidToString (elemGuid).ToPrintf ());
        }

        return CreateErrorResponse (err, errorMsg);
    }

    return {};
}

#ifdef ServerMainVers_2600

HighlightElementsCommand::HighlightElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String HighlightElementsCommand::GetName () const
{
    return "HighlightElements";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "highlightedColor": {
                "type": "array",
                "description": "Color of the highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            },
            "nonHighlightedColor": {
                "type": "array",
                "description": "Color of the non highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "highlightedColor",
            "nonHighlightedColor"
        ]
    })";
}

GS::Optional<GS::UniString> HighlightElementsCommand::GetResponseSchema () const
{
    return {};
}

GS::ObjectState HighlightElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementIdArray;
    parameters.Get ("elements", elementIdArray);

    GS::Array<GS::Int32> highlightedColorArray;
    parameters.Get ("highlightedColor", highlightedColorArray);
    API_RGBAColor highlightedColor {
        highlightedColorArray[0] / 255.0,
        highlightedColorArray[1] / 255.0,
        highlightedColorArray[2] / 255.0,
        highlightedColorArray[3] / 255.0
    };

    GS::Array<GS::Int32> nonHighlightedColorArray;
    parameters.Get ("nonHighlightedColor", nonHighlightedColorArray);
    API_RGBAColor nonHighlightedColor {
        nonHighlightedColorArray[0] / 255.0,
        nonHighlightedColorArray[1] / 255.0,
        nonHighlightedColorArray[2] / 255.0,
        nonHighlightedColorArray[3] / 255.0
    };

    GS::HashTable<API_Guid, API_RGBAColor> elements;
    for (const GS::ObjectState& elementIdArrayItem : elementIdArray) {
        GS::ObjectState elementId;
        elementIdArrayItem.Get ("elementId", elementId);

        GS::UniString guidStr;
        elementId.Get ("guid", guidStr);

        GS::Guid guid (guidStr);
        API_Guid apiGuid = GSGuid2APIGuid (guid);
        elements.Add (apiGuid, highlightedColor);
    }

    if (!elements.IsEmpty ()) {
        ACAPI_UserInput_SetElementHighlight (elements, GS::NoValue, nonHighlightedColor);
    } else {
        ACAPI_UserInput_ClearElementHighlight ();
    }

    // need to call redraw for changes to take effect
    ACAPI_View_Redraw ();

    return {};
}

#endif
