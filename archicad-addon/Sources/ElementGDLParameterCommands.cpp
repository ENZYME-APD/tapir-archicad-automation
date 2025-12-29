#include "ElementGDLParameterCommands.hpp"
#include "MigrationHelper.hpp"

constexpr const char* ParameterValueFieldName = "value";

static GS::UniString ConvertAddParIDToString (API_AddParID addParID)
{
    switch (addParID) {
        case APIParT_Integer:			return "Integer";
        case APIParT_Length:			return "Length";
        case APIParT_Angle:				return "Angle";
        case APIParT_RealNum:			return "RealNumber";
        case APIParT_LightSw:			return "LightSwitch";
        case APIParT_ColRGB:			return "RGBColor";
        case APIParT_Intens:			return "Intensity";
        case APIParT_LineTyp:			return "LineType";
        case APIParT_Mater:				return "Material";
        case APIParT_FillPat:			return "FillPattern";
        case APIParT_PenCol:			return "PenColor";
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

static void AddValueInteger (GS::ObjectState& gdlParameterDetails,
                             const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add (ParameterValueFieldName, static_cast<Int32> (actParam.value.real));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<Int32> (ParameterValueFieldName);
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*) *actParam.value.array)[arrayIndex++]));
            }
        }
    }
}

static void AddValueDouble (GS::ObjectState& gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add (ParameterValueFieldName, actParam.value.real);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<double> (ParameterValueFieldName);
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (((double*) *actParam.value.array)[arrayIndex++]);
            }
        }
    }
}

template<typename T>
static void AddValueTrueFalseOptions (GS::ObjectState& gdlParameterDetails,
                                      const API_AddParType& actParam,
                                      T optionTrue,
                                      T optionFalse)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add (ParameterValueFieldName, static_cast<Int32> (actParam.value.real) == 0 ? optionFalse : optionTrue);
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<T> (ParameterValueFieldName);
        Int32 arrayIndex = 0;
        for (Int32 i1 = 1; i1 <= actParam.dim1; i1++) {
            for (Int32 i2 = 1; i2 <= actParam.dim2; i2++) {
                arrayValueItemAdder (static_cast<Int32> (((double*) *actParam.value.array)[arrayIndex++]) == 0 ? optionFalse : optionTrue);
            }
        }
    }
}

static void AddValueOnOff (GS::ObjectState& gdlParameterDetails,
                           const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, GS::String ("On"), GS::String ("Off"));
}

static void AddValueBool (GS::ObjectState& gdlParameterDetails,
                          const API_AddParType& actParam)
{
    AddValueTrueFalseOptions (gdlParameterDetails, actParam, true, false);
}

static void AddValueString (GS::ObjectState& gdlParameterDetails,
                            const API_AddParType& actParam)
{
    if (actParam.typeMod == API_ParSimple) {
        gdlParameterDetails.Add (ParameterValueFieldName, GS::UniString (actParam.value.uStr));
    } else {
        const auto& arrayValueItemAdder = gdlParameterDetails.AddList<GS::UniString> (ParameterValueFieldName);
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

static bool SetParamValueInteger (API_ChangeParamType& changeParam,
                                  const GS::ObjectState& parameterDetails)
{
    Int32 value;
    if (parameterDetails.Get (ParameterValueFieldName, value)) {
        changeParam.realValue = value;
        return true;
    }
    return false;
}

static bool SetParamValueDouble (API_ChangeParamType& changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    double value;
    if (parameterDetails.Get (ParameterValueFieldName, value)) {
        changeParam.realValue = value;
        return true;
    }
    return false;
}

static bool SetParamValueOnOff (API_ChangeParamType& changeParam,
                                const GS::ObjectState& parameterDetails)
{
    GS::String value;
    if (parameterDetails.Get (ParameterValueFieldName, value)) {
        changeParam.realValue = (value == "Off" ? 0 : 1);
        return true;
    }
    return false;
}

static bool SetParamValueBool (API_ChangeParamType& changeParam,
                               const GS::ObjectState& parameterDetails)
{
    bool value;
    if (parameterDetails.Get (ParameterValueFieldName, value)) {
        changeParam.realValue = (value ? 1 : 0);
        return true;
    }
    return false;
}

static bool SetParamValueString (API_ChangeParamType& changeParam,
                                 const GS::ObjectState& parameterDetails)
{
    GS::UniString value;
    if (parameterDetails.Get (ParameterValueFieldName, value)) {
        constexpr USize MaxStrValueLength = 512;

        static GS::uchar_t strValuePtr[MaxStrValueLength];
        GS::ucscpy (strValuePtr, value.ToUStr (0, GS::Min (value.GetLength (), MaxStrValueLength)).Get ());

        changeParam.uStrValue = strValuePtr;
        return true;
    }
    return false;
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
                "$ref": "#/GDLParameterList"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "gdlParametersOfElements"
    ]
})";
}

GS::ObjectState	GetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& elemGdlParameterListAdder = response.AddList<GS::ObjectState> ("gdlParametersOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            elemGdlParameterListAdder (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_ParamOwnerType paramOwner = {};
        paramOwner.libInd = -1;
        paramOwner.guid = GetGuidFromObjectState (*elementId);

        API_Element element = {};
        element.header.guid = paramOwner.guid;
        GSErrCode err = ACAPI_Element_Get (&element);
        if (err != NoError) {
            const GS::UniString errorMsg = GS::UniString::Printf ("Not found element with guid %T!", APIGuidToString (paramOwner.guid).ToPrintf ());
            elemGdlParameterListAdder (CreateErrorResponse (err, errorMsg));
            continue;
        }

#ifdef ServerMainVers_2600
        paramOwner.type = element.header.type;
#else
        paramOwner.typeID = element.header.typeID;
#endif

        API_GetParamsType getParams = {};
        err = ACAPI_LibraryPart_OpenParameters (&paramOwner);
        if (err == NoError) {
            err = ACAPI_LibraryPart_GetActParameters (&getParams);
        }

        if (err != NoError) {
            const GS::UniString errorMsg = GS::UniString::Printf ("Failed to get parameters of element with guid %T!", APIGuidToString (paramOwner.guid).ToPrintf ());
            elemGdlParameterListAdder (CreateErrorResponse (err, errorMsg));
            continue;
        }

        Int32 libInd = -1;
        if (GetElemTypeId (element.header) == API_ObjectID) {
            libInd = element.object.libInd;
        } else if (GetElemTypeId (element.header) == API_LampID) {
            libInd = element.lamp.libInd;
        } else if (GetElemTypeId (element.header) == API_WindowID) {
            libInd = element.window.openingBase.libInd;
        } else if (GetElemTypeId (element.header) == API_DoorID) {
            libInd = element.door.openingBase.libInd;
        } else if (GetElemTypeId (element.header) == API_SkylightID) {
            libInd = element.skylight.openingBase.libInd;
        } else if (GetElemTypeId (element.header) == API_ZoneID) {
            libInd = element.zone.libInd;
        } else if (GetElemTypeId (element.header) == API_LabelID) {
            libInd = element.label.u.symbol.libInd;
        } else if (GetElemTypeId (element.header) == API_DrawingID) {
            libInd = element.drawing.title.libInd;
        } else if (GetElemTypeId (element.header) == API_CurtainWallFrameID) {
            libInd = element.cwFrame.libInd;
        } else if (GetElemTypeId (element.header) == API_CurtainWallPanelID) {
            libInd = element.cwPanel.libInd;
        } else if (GetElemTypeId (element.header) == API_CurtainWallJunctionID) {
            libInd = element.cwJunction.libInd;
        } else if (GetElemTypeId (element.header) == API_CurtainWallAccessoryID) {
            libInd = element.cwAccessory.libInd;
        }

        double a;
        double b;
        Int32 addParNum;
        API_AddParType** addPars;
        ACAPI_LibraryPart_GetParams (libInd, &a, &b, &addParNum, &addPars);

        const GSSize nParams = BMGetHandleSize ((GSHandle) getParams.params) / sizeof (API_AddParType);
        GS::ObjectState gdlParameters;
        const auto& parameterListAdder = gdlParameters.AddList<GS::ObjectState> ("parameters");
        for (GSIndex ii = 0; ii < nParams; ++ii) {
            const API_AddParType& actParam = (*getParams.params)[ii];
            const API_AddParType& actLibPartParam = (*addPars)[ii];

            if (actParam.typeID == APIParT_Separator) {
                continue;
            }

            API_GetParamValuesType getValues = {};
            getValues.index = actParam.index;
            ACAPI_LibraryPart_GetParamValues (&getValues);

            GS::ObjectState gdlParameterDetails;
            gdlParameterDetails.Add ("name", actParam.name);
            gdlParameterDetails.Add ("displayName", GS::UniString (actLibPartParam.uDescname));
            gdlParameterDetails.Add ("index", actParam.index);
            gdlParameterDetails.Add ("type", ConvertAddParIDToString (actParam.typeID));
            if (actParam.typeMod == API_ParArray) {
                gdlParameterDetails.Add ("dimension1", actParam.dim1);
                gdlParameterDetails.Add ("dimension2", actParam.dim2);
            }

            switch (actParam.typeID) {
                case APIParT_Integer:
                case APIParT_PenCol:
                case APIParT_LineTyp:
                case APIParT_Mater:
                case APIParT_FillPat:
                case APIParT_BuildingMaterial:
                case APIParT_Profile:
                    AddValueInteger (gdlParameterDetails, actParam);
                    break;
                case APIParT_ColRGB:
                case APIParT_Intens:
                case APIParT_Length:
                case APIParT_RealNum:
                case APIParT_Angle:
                    AddValueDouble (gdlParameterDetails, actParam);
                    break;
                case APIParT_LightSw:
                    AddValueOnOff (gdlParameterDetails, actParam);
                    break;
                case APIParT_Boolean:
                    AddValueBool (gdlParameterDetails, actParam);
                    break;
                case APIParT_CString:
                case APIParT_Title:
                    AddValueString (gdlParameterDetails, actParam);
                    break;
                default:
                case APIParT_Dictionary:
                    // Not supported by the Archicad API yet
                    break;
            }

            GS::UniString valueDescription (actParam.valueDescription);
            if (!valueDescription.IsEmpty ()) {
                gdlParameterDetails.Add ("valueDescription", valueDescription);
            }

            gdlParameterDetails.Add ("isLocked", getValues.locked);

            const auto& flags = gdlParameterDetails.AddList<GS::UniString> ("flags");
            if (actParam.flags & API_ParFlg_Hidden) {
                flags ("Hidden");
            }
            if (actParam.flags & API_ParFlg_SHidden) {
                flags ("HiddenFromScript");
            }
            if (actParam.flags & API_ParFlg_Disabled) {
                flags ("Disabled");
            }
            if (actParam.flags & API_ParFlg_Child) {
                flags ("Child");
            }
            if (actParam.flags & API_ParFlg_Unique) {
                flags ("Unique");
            }
            if (actParam.flags & API_ParFlg_Fixed) {
                flags ("Fixed");
            }

            if (getValues.nVals > 0) {
                if (actParam.typeID == APIParT_PenCol && getValues.nVals == 255) {
                    BMhFree ((GSHandle)getValues.realValues);
                } else {
                    if (getValues.uStrValues != nullptr) {
                        const auto& possibleValues = gdlParameterDetails.AddList<GS::UniString> ("possibleValues");
                        GS::uchar_t *strPos = *getValues.uStrValues;
                        for (GSIndex valIdx = 0; valIdx < getValues.nVals; ++valIdx) {
                            GS::UniString tmpUStr (strPos);
                            possibleValues (tmpUStr);
                            strPos += GS::ucslen (strPos) + 1;
                        }
                        BMhFree ((GSHandle)getValues.uStrValues);
                    } else if (getValues.realValues != nullptr) {
                        const auto& possibleValues = gdlParameterDetails.AddList<GS::ObjectState> ("possibleValues");
                        for (GSIndex valIdx = 0; valIdx < getValues.nVals; ++valIdx) {
                            const auto& possibleValue = (*getValues.realValues)[valIdx];
                             GS::ObjectState os;
                            if (possibleValue.flags) {
                                if (possibleValue.flags == APIVLVal_LowerLimit) {
                                    os = GS::ObjectState ("value", possibleValue.lowerLimit, "flag", "GreaterThan");
                                } else if (possibleValue.flags & APIVLVal_LowerEqual) {
                                    os = GS::ObjectState ("value", possibleValue.lowerLimit, "flag", "GreaterThanOrEqual");
                                } else if (possibleValue.flags == APIVLVal_UpperLimit) {
                                    os = GS::ObjectState ("value", possibleValue.upperLimit, "flag", "LessThan");
                                } else if (possibleValue.flags & APIVLVal_UpperEqual) {
                                    os = GS::ObjectState ("value", possibleValue.upperLimit, "flag", "LessThanOrEqual");
                                } else if (possibleValue.flags == APIVLVal_Step) {
                                    os = GS::ObjectState ("value", possibleValue.stepBeg, "flag", "StepBegin");
                                    os = GS::ObjectState ("value", possibleValue.stepVal, "flag", "StepValue");
                                }
                            } else {
                                os = GS::ObjectState ("value", possibleValue.value);
                            }
                            GS::UniString valueDescription (possibleValue.valueDescription);
                            if (!valueDescription.IsEmpty ()) {
                                os.Add ("description", valueDescription);
                            }
                            possibleValues (os);
                        }
                        BMhFree ((GSHandle)getValues.realValues);
                    }
                    gdlParameterDetails.Add ("canHaveCustomValue", getValues.custom);
                }
            }

            parameterListAdder (gdlParameterDetails);
        }
        elemGdlParameterListAdder (gdlParameters);
        ACAPI_DisposeAddParHdl (&getParams.params);
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
                        "$ref": "#/SetGDLParameterArray"
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

GS::Optional<GS::UniString> SetGDLParametersOfElementsCommand::GetResponseSchema () const
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

GS::ObjectState	SetGDLParametersOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> elementsWithGDLParameters;
    parameters.Get ("elementsWithGDLParameters", elementsWithGDLParameters);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set GDL Parameters of Elements", [&]() -> GSErrCode {
        for (const GS::ObjectState& elementWithGDLParameters : elementsWithGDLParameters) {
            GSErrCode err = NoError;
            GS::UniString errMessage;
            const GS::ObjectState* elementId = elementWithGDLParameters.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const API_Guid elemGuid = GetGuidFromObjectState (*elementId);
            API_ParamOwnerType paramOwner = {};
            paramOwner.libInd = 0;
#ifdef ServerMainVers_2600
            paramOwner.type = API_ObjectID;
#else
            paramOwner.typeID = API_ObjectID;
#endif
            paramOwner.guid = elemGuid;

            GS::Array<GS::ObjectState> elemGdlParameters;
            elementWithGDLParameters.Get ("gdlParameters", elemGdlParameters);

            err = ACAPI_LibraryPart_OpenParameters (&paramOwner);
            if (err == NoError) {
                API_GetParamsType getParams = {};
                err = ACAPI_LibraryPart_GetActParameters (&getParams);
                if (err == NoError) {
                    const GSSize nParams = BMGetHandleSize ((GSHandle) getParams.params) / sizeof (API_AddParType);
                    GS::HashTable<GS::String, API_AddParID> gdlParametersTypeDictionary;
                    GS::HashTable<short, GS::String> gdlParametersIndexNameDictionary;
                    for (GSIndex ii = 0; ii < nParams; ++ii) {
                        const API_AddParType& actParam = (*getParams.params)[ii];
                        if (actParam.typeID != APIParT_Separator) {
                            auto name = GS::String (actParam.name);
                            gdlParametersIndexNameDictionary.Add (actParam.index, name);
                            gdlParametersTypeDictionary.Add (name, actParam.typeID);
                        }
                    }

                    for (const GS::ObjectState& elemGdlParametersItem : elemGdlParameters) {
                        API_ChangeParamType changeParam = {};
                        GS::Array<GS::ObjectState> parameters;
                        if (elemGdlParametersItem.Get ("parameters", parameters)) {
                            // Legacy mode: old schema had nested list for parameters
                            for (const GS::ObjectState& parameter : parameters) {
                                err = SetOneGDLParameter (parameter, elemGuid, changeParam, gdlParametersTypeDictionary, gdlParametersIndexNameDictionary, errMessage);
                                if (err != NoError) {
                                    break;
                                }

                                ACAPI_DisposeAddParHdl (&getParams.params);
                                ACAPI_LibraryPart_GetActParameters (&getParams);
                            }
                        } else {
                            err = SetOneGDLParameter (elemGdlParametersItem, elemGuid, changeParam, gdlParametersTypeDictionary, gdlParametersIndexNameDictionary, errMessage);
                            if (err != NoError) {
                                break;
                            }

                            ACAPI_DisposeAddParHdl (&getParams.params);
                            ACAPI_LibraryPart_GetActParameters (&getParams);
                        }
                    }

                    if (err == NoError) {
                        API_Element	element = {};
                        element.header.guid = elemGuid;

                        err = ACAPI_Element_Get (&element);
                        if (err == NoError) {
                            API_Element 	mask = {};
                            API_ElementMemo memo = {};

                            ACAPI_ELEMENT_MASK_CLEAR (mask);
                            switch (GetElemTypeId (element.header)) {
                                case API_ObjectID:
                                    element.object.xRatio = getParams.a;
                                    element.object.yRatio = getParams.b;
                                    ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, xRatio);
                                    ACAPI_ELEMENT_MASK_SET (mask, API_ObjectType, yRatio);
                                    break;
                                case API_WindowID:
                                case API_DoorID:
                                    element.window.openingBase.width = getParams.a;
                                    element.window.openingBase.height = getParams.b;
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.width);
                                    ACAPI_ELEMENT_MASK_SET (mask, API_WindowType, openingBase.height);
                                    break;
                                case API_SkylightID:
                                    element.skylight.openingBase.width = getParams.a;
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
                }
                ACAPI_LibraryPart_CloseParameters ();
                ACAPI_DisposeAddParHdl (&getParams.params);
            }

            if (err != NoError) {
                if (errMessage.IsEmpty ()) {
                    executionResults (CreateFailedExecutionResult (err, GS::UniString::Printf ("Failed to change parameters of element with guid %T", APIGuidToString (elemGuid).ToPrintf ())));
                } else {
                    executionResults (CreateFailedExecutionResult (err, errMessage));
                }
            } else {
                executionResults (CreateSuccessfulExecutionResult ());
            }
        }

        return NoError;
    });

    return response;
}

GSErrCode
SetGDLParametersOfElementsCommand::SetOneGDLParameter (
    const GS::ObjectState& parameter,
    const API_Guid& elemGuid,
    API_ChangeParamType& changeParam,
    const GS::HashTable<GS::String, API_AddParID>& gdlParametersTypeDictionary,
    const GS::HashTable<short, GS::String>& gdlParametersIndexNameDictionary,
    GS::UniString& errMessage)
{
    GS::String name;
    if (parameter.Get ("name", name)) {
        CHTruncate (name.ToCStr (), changeParam.name, API_NameLen);
        if (!gdlParametersTypeDictionary.ContainsKey (changeParam.name)) {
            errMessage = GS::UniString::Printf ("Invalid input: %s is not a GDL parameter of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
            return APIERR_BADPARS;
        }
    } else {
        short index;
        if (parameter.Get ("index", index)) {
            if (gdlParametersIndexNameDictionary.ContainsKey (index)) {
                CHTruncate (gdlParametersIndexNameDictionary[index].ToCStr (), changeParam.name, API_NameLen);
            } else {
                errMessage = GS::UniString::Printf ("Invalid input: no GDL parameter with index %d for element %T", index, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
        } else {
            errMessage = "Invalid input: both name and index are missing, one of them must be set!";
            return APIERR_BADPARS;
        }
    }

    if (!parameter.Contains (ParameterValueFieldName)) {
        errMessage = GS::UniString::Printf ("Invalid input: value is missing for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
        return APIERR_BADPARS;
    }

    switch (gdlParametersTypeDictionary[changeParam.name]) {
        case APIParT_Integer:
        case APIParT_PenCol:
        case APIParT_LineTyp:
        case APIParT_Mater:
        case APIParT_FillPat:
        case APIParT_BuildingMaterial:
        case APIParT_Profile:
            if (!SetParamValueInteger (changeParam, parameter)) {
                errMessage = GS::UniString::Printf ("Invalid input: the given value is not an integer for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
            break;
        case APIParT_ColRGB:
        case APIParT_Intens:
        case APIParT_Length:
        case APIParT_RealNum:
        case APIParT_Angle:
            if (!SetParamValueDouble (changeParam, parameter)) {
                errMessage = GS::UniString::Printf ("Invalid input: the given value is not a real number for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
            break;
        case APIParT_LightSw:
            if (!SetParamValueOnOff (changeParam, parameter)) {
                errMessage = GS::UniString::Printf ("Invalid input: the given value is not 'On' or 'Off' for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
            break;
        case APIParT_Boolean:
            if (!SetParamValueBool (changeParam, parameter)) {
                errMessage = GS::UniString::Printf ("Invalid input: the given value is not a boolean for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
            break;
        case APIParT_CString:
        case APIParT_Title:
            if (!SetParamValueString (changeParam, parameter)) {
                errMessage = GS::UniString::Printf ("Invalid input: the given value is not a string for parameter %s of element %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
                return APIERR_BADPARS;
            }
            break;
        default:
        case APIParT_Dictionary:
            // Not supported by the Archicad API yet
            break;
    }

    GSErrCode err = ACAPI_LibraryPart_ChangeAParameter (&changeParam);
    if (err != NoError) {
        errMessage = GS::UniString::Printf ("Failed to change parameter %s of element with guid %T", changeParam.name, APIGuidToString (elemGuid).ToPrintf ());
        return err;
    }

    return err;
}
