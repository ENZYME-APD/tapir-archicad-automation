#include "DesignOptionCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2700
#include "ACAPI/Result.hpp"
#include "ACAPI_Navigator.h"
#include "ACAPI/View.hpp"
#include "ACAPI/DesignOptionManager.hpp"
#endif

#ifdef ServerMainVers_2900
using namespace ACAPI::DesignOptions;
#elif defined (ServerMainVers_2800)
using DesignOptionManager = ACAPI::DesignOptions::v1::DesignOptionManager;
using namespace ACAPI::DesignOptions::v2;
#elif defined (ServerMainVers_2700)
using DesignOptionManager = ACAPI::v1::DesignOptionManager;
using namespace ACAPI::v2;
#endif

GetDesignOptionsCommand::GetDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDesignOptionsCommand::GetName () const
{
    return "GetDesignOptions";
}

GS::Optional<GS::UniString> GetDesignOptionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionDetails"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::ObjectState GetDesignOptionsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to retrieve design options.");
    }

    const auto options = manager->GetAllDesignOptions ();
    if (options.IsErr ()) {
        return CreateErrorResponse (options.UnwrapErr ().kind, "Unable to retrieve design options.");
    }

    GS::ObjectState response;
    const auto& designOptions = response.AddList<GS::ObjectState> ("designOptions");

    for (const auto& option : options.Unwrap ()) {
        GS::ObjectState designOption;
        designOption.Add ("designOptionId", CreateGuidObjectState (option.GetGuid ()));
        designOption.Add ("name", option.GetName ());
        designOption.Add ("id", option.GetId ());
        designOption.Add ("ownerSetName", option.GetOwnerSetName ());
        designOptions (designOption);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetDesignOptionSetsCommand::GetDesignOptionSetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDesignOptionSetsCommand::GetName () const
{
    return "GetDesignOptionSets";
}

GS::Optional<GS::UniString> GetDesignOptionSetsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionSetId": {
                            "$ref": "#/GuidId",
                            "description": "The guid identifier of the design option set."
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the design option set."
                        },
                        "designOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of design options in the set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionSetId",
                        "name",
                        "designOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
        ]
    })";
}

GS::ObjectState GetDesignOptionSetsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to retrieve design option sets.");
    }

    const auto optionSets = manager->GetDesignOptionSets ();
    if (optionSets.IsErr ()) {
        return CreateErrorResponse (optionSets.UnwrapErr ().kind, "Unable to retrieve design option sets.");
    }

    GS::ObjectState response;
    const auto& designOptionSets = response.AddList<GS::ObjectState> ("designOptionSets");

    for (const auto& optionSet : optionSets.Unwrap ()) {
        GS::ObjectState designOptionSet;
        designOptionSet.Add ("designOptionSetId", CreateGuidObjectState (optionSet.GetGuid ()));
        designOptionSet.Add ("name", optionSet.GetName ());
        const auto& options = designOptionSet.AddList<GS::ObjectState> ("designOptions");
        const auto designOptions = manager->GetDesignOptionsOfSet (optionSet);
        if (designOptions.IsOk ()) {
            for (const auto& option : designOptions.Unwrap ()) {
                options (CreateIdObjectState ("designOptionId", GSGuid2APIGuid (option.GetGuid ())));
            }
        }
        designOptionSets (designOptionSet);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetDesignOptionCombinationsCommand::GetDesignOptionCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDesignOptionCombinationsCommand::GetName () const
{
    return "GetDesignOptionCombinations";
}

GS::Optional<GS::UniString> GetDesignOptionCombinationsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/GuidId",
                            "description": "The guid identifier of the design option combination."
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the design option combination."
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination. Available from Archicad 29."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::ObjectState GetDesignOptionCombinationsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2700
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to get design option combinations.");
    }

#ifdef ServerMainVers_2900
    const auto optionCombinations = manager->GetDesignOptionCombinations ();
#else
    const auto optionCombinations = manager->GetAvailableDesignOptionCombinations ();
#endif
    if (optionCombinations.IsErr ()) {
        return CreateErrorResponse (optionCombinations.UnwrapErr ().kind, "Unable to get design option combinations.");
    }

    GS::ObjectState response;
    const auto& designOptionCombinations = response.AddList<GS::ObjectState> ("designOptionCombinations");

    if (optionCombinations.Unwrap ().empty ()) {
        return response;
    }

    for (const auto& optionCombination : optionCombinations.Unwrap ()) {
        GS::ObjectState designOptionCombination;
        designOptionCombination.Add ("designOptionCombinationId", CreateGuidObjectState (optionCombination.GetGuid ()));
        designOptionCombination.Add ("name", optionCombination.GetName ());
#ifdef ServerMainVers_2900
        const auto& activeOptions = designOptionCombination.AddList<GS::ObjectState> ("activeDesignOptions");
        const auto activeDesignOptions = manager->GetActiveDesignOptionsOfCombination (optionCombination);
        if (activeDesignOptions.IsOk ()) {
            for (const auto& activeOption : activeDesignOptions.Unwrap ()) {
                activeOptions (CreateIdObjectState ("designOptionId", GSGuid2APIGuid (activeOption.GetGuid ())));
            }
        }
#endif
        designOptionCombinations (designOptionCombination);
    }

    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 27 or newer.");
#endif
}

GetElementsOfDesignOptionsCommand::GetElementsOfDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetElementsOfDesignOptionsCommand::GetName () const
{
    return "GetElementsOfDesignOptions";
}

GS::Optional<GS::UniString> GetElementsOfDesignOptionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionId": {
                            "$ref": "#/DesignOptionId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsOfDesignOptionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsOfDesignOptions": {
                "type": "array",
                "items": {
                    "#ref": "#/ElementsOfDesignOptionOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsOfDesignOptions"
        ]
    })";
}

GS::ObjectState GetElementsOfDesignOptionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to retrieve design options.");
    }
	auto allDesignOptions = manager->GetAllDesignOptions ();
	if (allDesignOptions.IsErr ()) {
        return CreateErrorResponse (allDesignOptions.UnwrapErr ().kind, "Unable to retrieve design options.");
	}

    GS::Array<GS::ObjectState> designOptions;
    if (!parameters.Get ("designOptions", designOptions)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptions' parameter.");
    }

    GS::ObjectState response;
    const auto& elementsOfDesignOptions = response.AddList<GS::ObjectState> ("elementsOfDesignOptions");

    for (auto&& designOption : designOptions) {
        DesignOption* optionPtr = nullptr;
        GS::Guid designOptionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", designOption));
        if (designOptionGuid.IsNull()) {
            elementsOfDesignOptions (CreateErrorResponse (APIERR_BADPARS, "Missing or invalid 'designOptionId' parameter"));
            continue;
        }
        for (auto&& option : *allDesignOptions) {
            if (option.GetGuid () == designOptionGuid) {
                optionPtr = &option;
                break;
            }
        }
        if (optionPtr == nullptr) {
            elementsOfDesignOptions (CreateErrorResponse (APIERR_BADID, "Not found design option with the given 'designOptionId' parameter"));
            continue;
        }

        GS::ObjectState designOptionElements;
        designOptionElements.Add ("designOptionId", CreateGuidObjectState (designOptionGuid));
        const auto& elements = designOptionElements.AddList<GS::ObjectState> ("elements");
        auto getElementsResult = manager->GetElementsOfDesignOption (*optionPtr);
        if (getElementsResult.IsOk ()) {
            for (const auto& element : getElementsResult.Unwrap ()) {
                elements (CreateElementIdObjectState (element));
            }
        }
        elementsOfDesignOptions (designOptionElements);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

GetDesignOptionForElementsCommand::GetDesignOptionForElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDesignOptionForElementsCommand::GetName () const
{
    return "GetDesignOptionForElements";
}

GS::Optional<GS::UniString> GetDesignOptionForElementsCommand::GetInputParametersSchema () const
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

GS::Optional<GS::UniString> GetDesignOptionForElementsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionForElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the element."
                        },
                        "type": {
                            "type": "string",
                            "description": "The type of the associated design option.",
                            "enum": [
                                "NotExistingElement",
                                "MissingDesignOption",
                                "NotLinkedToAnyDesignOption",
                                "LinkedToDesignOption"
                            ]
                        },
                        "designOption": {
                            "$ref": "#/DesignOptionDetails"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "type"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionForElements"
        ]
    })";
}

GS::ObjectState GetDesignOptionForElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to retrieve design options.");
    }

    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    const GS::Array<API_Guid> elemIds = elements.Transform<API_Guid> (GetGuidFromElementsArrayItem);
    GS::ObjectState response;
    const auto& designOptionForElements = response.AddList<GS::ObjectState> ("designOptionForElements");

    for (auto&& elemId : elemIds) {
        GS::ObjectState elementDesignOption = CreateElementIdObjectState (elemId);
        API_Elem_Head elemHeader;
        if (!LoadElementHeaderByGuid (elemId, elemHeader)) {
            elementDesignOption.Add ("type", "NotExistingElement");
            designOptionForElements (elementDesignOption);
            continue;
        }

        auto designOptionStatus = manager->GetDesignOptionStatusOf (elemId);
        if (designOptionStatus.IsErr ()) {
            elementDesignOption.Add ("type", "NotExistingElement");
            designOptionForElements (elementDesignOption);
        }

         elementDesignOption.Add ("elementId", CreateGuidObjectState (elemId));
		if (std::holds_alternative<MissingDesignOption> (*designOptionStatus)) {
            elementDesignOption.Add ("type", "MissingDesignOption");
            designOptionForElements (elementDesignOption);
        } else if (std::holds_alternative<MainModelDesignOption> (*designOptionStatus)) {
            elementDesignOption.Add ("type", "NotLinkedToAnyDesignOption");
            designOptionForElements (elementDesignOption);
        } else if (std::holds_alternative<DesignOption> (*designOptionStatus)) {
            const auto& designOption = std::get<DesignOption> (*designOptionStatus);
            GS::ObjectState designOptionDetails;
            designOptionDetails.Add ("designOptionId", CreateGuidObjectState (designOption.GetGuid ()));
            designOptionDetails.Add ("name", designOption.GetName ());
            designOptionDetails.Add ("id", designOption.GetId ());
            designOptionDetails.Add ("ownerSetName", designOption.GetOwnerSetName ());

            elementDesignOption.Add ("type", "LinkedToDesignOption");
            elementDesignOption.Add ("designOption", designOptionDetails);
            designOptionForElements (elementDesignOption);
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

CreateDesignOptionSetsCommand::CreateDesignOptionSetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDesignOptionSetsCommand::GetName () const
{
    return "CreateDesignOptionSets";
}

GS::Optional<GS::UniString> CreateDesignOptionSetsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
        ]
    })";
}

GS::Optional<GS::UniString> CreateDesignOptionSetsCommand::GetResponseSchema () const
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

GS::ObjectState CreateDesignOptionSetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to create design option sets.");
    }

    GS::Array<GS::UniString> designOptionSets;
    if (!parameters.Get ("designOptionSets", designOptionSets)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionSets' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Create Design Option Sets", [&]() -> GSErrCode {
        for (auto&& designOptionSet : designOptionSets) {
            auto result = manager->CreateDesignOptionSet ({}, designOptionSet);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to create design option set."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

CreateDesignOptionsCommand::CreateDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDesignOptionsCommand::GetName () const
{
    return "CreateDesignOptions";
}

GS::Optional<GS::UniString> CreateDesignOptionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the design option."
                        },
                        "id": {
                            "type": "string",
                            "description": "The string id of the design option."
                        },
                        "ownerSetName": {
                            "type": "string",
                            "description": "The name of the owner design option set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "id",
                        "ownerSetName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> CreateDesignOptionsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionIdsOrErrors": {
                "$ref": "#/DesignOptionIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionIdsOrErrors"
        ]
    })";
}

GS::ObjectState CreateDesignOptionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to create design options.");
    }

    GS::Array<GS::ObjectState> designOptions;
    if (!parameters.Get ("designOptions", designOptions)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptions' parameter.");
    }

	auto sets = manager->GetDesignOptionSets ();
	if (sets.IsErr ()) {
        return CreateErrorResponse (sets.UnwrapErr ().kind, "Unable to create design options.");
	}

    GS::ObjectState response;
    const auto& designOptionIdsOrErrors = response.AddList<GS::ObjectState> ("designOptionIdsOrErrors");

    ACAPI_CallUndoableCommand ("Create Design Options", [&]() -> GSErrCode {
        for (auto&& designOption : designOptions) {
            GS::UniString name;
            if (!designOption.Get ("name", name)) {
                designOptionIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' parameter"));
                continue;
            }
            GS::UniString id;
            if (!designOption.Get ("id", id)) {
                designOptionIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'id' parameter"));
                continue;
            }
            GS::UniString ownerSetName;
            if (!designOption.Get ("ownerSetName", ownerSetName)) {
                designOptionIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'ownerSetName' parameter"));
                continue;
            }
            DesignOptionSet* setPtr = nullptr;
            for (auto&& set : *sets) {
                if (set.GetName () == ownerSetName) {
                    setPtr = &set;
                    break;
                }
            }
            if (setPtr == nullptr) {
                designOptionIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Not found design option set with the given 'ownerSetName'"));
                continue;
            }
            auto result = manager->CreateDesignOption (*setPtr, {}, name, id);
            designOptionIdsOrErrors (result.IsOk () ? CreateIdObjectState ("designOptionId", GSGuid2APIGuid (result->GetGuid ())) : CreateErrorResponse (result.UnwrapErr ().kind, "Failed to create design option."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

CreateDesignOptionCombinationsCommand::CreateDesignOptionCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateDesignOptionCombinationsCommand::GetName () const
{
    return "CreateDesignOptionCombinations";
}

GS::Optional<GS::UniString> CreateDesignOptionCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the design option combination."
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "activeDesignOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> CreateDesignOptionCombinationsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinationIdsOrErrors": {
                "$ref": "#/DesignOptionCombinationIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinationIdsOrErrors"
        ]
    })";
}

GS::ObjectState CreateDesignOptionCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to create design option combinations.");
    }

    GS::Array<GS::ObjectState> designOptionCombinations;
    if (!parameters.Get ("designOptionCombinations", designOptionCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionCombinations' parameter.");
    }

	auto allDesignOptions = manager->GetAllDesignOptions ();
	if (allDesignOptions.IsErr ()) {
        return CreateErrorResponse (allDesignOptions.UnwrapErr ().kind, "Unable to create design option combinations.");
	}

    GS::ObjectState response;
    const auto& designOptionCombinationIdsOrErrors = response.AddList<GS::ObjectState> ("designOptionCombinationIdsOrErrors");

    ACAPI_CallUndoableCommand ("Create Design Option Combinations", [&]() -> GSErrCode {
        for (auto&& designOptionCombination : designOptionCombinations) {
            GS::UniString name;
            if (!designOptionCombination.Get ("name", name)) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'name' parameter"));
                continue;
            }
            std::vector<DesignOption> designOptions;
            GS::Array<GS::ObjectState> activeDesignOptions;
            if (!designOptionCombination.Get ("activeDesignOptions", activeDesignOptions)) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'activeDesignOptions' parameter"));
                continue;
            }
            GS::Guid optionGuid;
            DesignOption* optionPtr = nullptr;
            for (GS::ObjectState& activeDesignOption : activeDesignOptions) {
                optionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", activeDesignOption));
                optionPtr = nullptr;
                for (auto&& option : *allDesignOptions) {
                    if (option.GetGuid () == optionGuid) {
                        optionPtr = &option;
                        break;
                    }
                }
                if (optionPtr == nullptr) {
                    break;
                }
                designOptions.push_back (*optionPtr);
            }
            if (!optionGuid.IsNull () && optionPtr == nullptr) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Not found design option with the given 'designOptionId' " + optionGuid.ToUniString ()));
                continue;
            }
            auto creationResult = manager->CreateDesignOptionCombination (name);
            if (creationResult.IsErr ()) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (creationResult.UnwrapErr ().kind, "Unable to create design option combination."));
                continue;
            }
            auto result = manager->SetActiveOptionsInCombination (*creationResult, designOptions);
            designOptionCombinationIdsOrErrors (CreateIdObjectState ("designOptionCombinationId", GSGuid2APIGuid (creationResult->GetGuid ())));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

SetActiveDesignOptionsInCombinationsCommand::SetActiveDesignOptionsInCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetActiveDesignOptionsInCombinationsCommand::GetName () const
{
    return "SetActiveDesignOptionsInCombinations";
}

GS::Optional<GS::UniString> SetActiveDesignOptionsInCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "activeDesignOptionsInCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/DesignOptionCombinationId"
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "activeDesignOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "activeDesignOptionsInCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> SetActiveDesignOptionsInCombinationsCommand::GetResponseSchema () const
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

GS::ObjectState SetActiveDesignOptionsInCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to list design option combinations.");
    }

    GS::Array<GS::ObjectState> activeDesignOptionsInCombinations;
    if (!parameters.Get ("activeDesignOptionsInCombinations", activeDesignOptionsInCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'activeDesignOptionsInCombinations' parameter.");
    }

	auto allDesignOptionCombinations = manager->GetDesignOptionCombinations ();
	if (allDesignOptionCombinations.IsErr ()) {
        return CreateErrorResponse (allDesignOptionCombinations.UnwrapErr ().kind, "Unable to list design option combinations.");
	}

	auto allDesignOptions = manager->GetAllDesignOptions ();
	if (allDesignOptions.IsErr ()) {
        return CreateErrorResponse (allDesignOptions.UnwrapErr ().kind, "Unable to list design options.");
	}

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set Active Design Options In Combinations", [&]() -> GSErrCode {
        for (auto&& activeDesignOptionsInCombination : activeDesignOptionsInCombinations) {
            GS::Guid combinationGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionCombinationId", activeDesignOptionsInCombination));
            if (combinationGuid.IsNull()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Missing or invalid 'designOptionCombinationId' parameter"));
                continue;
            }

            DesignOptionCombination* combinationPtr = nullptr;
            for (auto&& combination : *allDesignOptionCombinations) {
                if (combination.GetGuid () == combinationGuid) {
                    combinationPtr = &combination;
                    break;
                }
            }
            if (combinationPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option combination with the given 'designOptionCombinationId' parameter"));
                continue;
            }
            std::vector<DesignOption> designOptions;
            GS::Array<GS::ObjectState> activeDesignOptions;
            if (!activeDesignOptionsInCombination.Get ("activeDesignOptions", activeDesignOptions)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'activeDesignOptions' parameter"));
                continue;
            }
            GS::Guid optionGuid;
            DesignOption* optionPtr = nullptr;
            for (GS::ObjectState& activeDesignOption : activeDesignOptions) {
                optionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", activeDesignOption));
                optionPtr = nullptr;
                for (auto&& option : *allDesignOptions) {
                    if (option.GetGuid () == optionGuid) {
                        optionPtr = &option;
                        break;
                    }
                }
                if (optionPtr == nullptr) {
                    break;
                }
                designOptions.push_back (*optionPtr);
            }
            if (!optionGuid.IsNull () && optionPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Not found design option with the given 'designOptionId' " + optionGuid.ToUniString ()));
                continue;
            }
            auto result = manager->SetActiveOptionsInCombination (*combinationPtr, designOptions);
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

MoveElementsToDesignOptionsCommand::MoveElementsToDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveElementsToDesignOptionsCommand::GetName () const
{
    return "MoveElementsToDesignOptions";
}

GS::Optional<GS::UniString> MoveElementsToDesignOptionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementDesignOptionPairs": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "designOptionId": {
                            "description": "The identifier of the design option to move the element into. Use NULLGuid to remove the element from any design option and move it to the main model.",
                            "$ref": "#/DesignOptionId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "designOptionId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementDesignOptionPairs"
        ]
    })";
}

GS::Optional<GS::UniString> MoveElementsToDesignOptionsCommand::GetResponseSchema () const
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

GS::ObjectState MoveElementsToDesignOptionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design options.");
    }

    GS::Array<GS::ObjectState> elementDesignOptionPairs;
    if (!parameters.Get ("elementDesignOptionPairs", elementDesignOptionPairs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elementDesignOptionPairs' parameter.");
    }

	auto designOptions = manager->GetAllDesignOptions ();
	if (designOptions.IsErr ()) {
        return CreateErrorResponse (designOptions.UnwrapErr ().kind, "Unable to modify design options.");
	}

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Create Design Option Sets", [&]() -> GSErrCode {
        for (auto&& elementDesignOptionPair : elementDesignOptionPairs) {
            API_Guid elemGuid = GetGuidFromElementsArrayItem (elementDesignOptionPair);
            if (elemGuid == APINULLGuid) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'elementId' parameter"));
                continue;
            }
            DesignOption* optionPtr = nullptr;
            GS::Guid designOptionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", elementDesignOptionPair));
            if (!designOptionGuid.IsNull()) {
                for (auto&& designOption : *designOptions) {
                    if (designOption.GetGuid () == designOptionGuid) {
                        optionPtr = &designOption;
                        break;
                    }
                }
                if (optionPtr == nullptr) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option with the given 'designOptionId' parameter"));
                    continue;
                }
            }
            auto result = manager->RelinkElementToDesignOption (elemGuid, optionPtr == nullptr ? std::optional<DesignOption>() : *optionPtr);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to move element to design option."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

MoveDesignOptionsToAnotherSetCommand::MoveDesignOptionsToAnotherSetCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MoveDesignOptionsToAnotherSetCommand::GetName () const
{
    return "MoveDesignOptionsToAnotherSet";
}

GS::Optional<GS::UniString> MoveDesignOptionsToAnotherSetCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionAndSetPairs": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionId": {
                            "description": "The identifier of the design option to move.",
                            "$ref": "#/DesignOptionId"
                        },
                        "setName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "setName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionAndSetPairs"
        ]
    })";
}

GS::Optional<GS::UniString> MoveDesignOptionsToAnotherSetCommand::GetResponseSchema () const
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

GS::ObjectState MoveDesignOptionsToAnotherSetCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design options.");
    }

    GS::Array<GS::ObjectState> designOptionAndSetPairs;
    if (!parameters.Get ("designOptionAndSetPairs", designOptionAndSetPairs)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionAndSetPairs' parameter.");
    }

	auto designOptions = manager->GetAllDesignOptions ();
	if (designOptions.IsErr ()) {
        return CreateErrorResponse (designOptions.UnwrapErr ().kind, "Unable to modify design options.");
	}

	auto sets = manager->GetDesignOptionSets ();
	if (sets.IsErr ()) {
        return CreateErrorResponse (sets.UnwrapErr ().kind, "Unable to modify design options.");
	}

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Move Design Options to Another Set", [&]() -> GSErrCode {
        for (auto&& designOptionAndSetPair : designOptionAndSetPairs) {
            DesignOption* optionPtr = nullptr;
            GS::Guid designOptionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", designOptionAndSetPair));
            if (!designOptionGuid.IsNull()) {
                for (auto&& designOption : *designOptions) {
                    if (designOption.GetGuid () == designOptionGuid) {
                        optionPtr = &designOption;
                        break;
                    }
                }
                if (optionPtr == nullptr) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option with the given 'designOptionId' parameter"));
                    continue;
                }
            }
            GS::UniString setName;
            if (!designOptionAndSetPair.Get ("setName", setName)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Missing or invalid 'setName' parameter"));
            }
            DesignOptionSet* setPtr = nullptr;
            for (auto&& set : *sets) {
                if (set.GetName () == setName) {
                    setPtr = &set;
                    break;
                }
            }
            if (setPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADNAME, "Not found design option set with the given 'setName'"));
                continue;
            }
            auto result = manager->MoveDesignOptionToOtherSet (*optionPtr, *setPtr);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to move design option."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}
