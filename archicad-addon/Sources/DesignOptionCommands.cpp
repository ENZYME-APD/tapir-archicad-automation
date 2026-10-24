#include "DesignOptionCommands.hpp"
#include "MigrationHelper.hpp"

#ifdef ServerMainVers_2700
#include "ACAPI/Result.hpp"
#include "ACAPI_Navigator.h"
#include "ACAPI/View.hpp"
#include "ACAPI/DesignOptionManager.hpp"
#endif

#ifdef ServerMainVers_2900
#include "ACAPI/DesignOptionsManipulationRightsChecker.hpp"
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

#ifdef ServerMainVers_2900
// Finds the design option, set or combination with the given guid in the
// list the manager returned; nullptr when there is none.
template <typename ContainerT>
static auto FindDesignOptionItemByGuid (ContainerT& items, const GS::Guid& guid) -> decltype (&*items.begin ())
{
    for (auto&& item : items) {
        if (item.GetGuid () == guid) {
            return &item;
        }
    }
    return nullptr;
}

// Returns the error response with Archicad's own text when the user may not
// create design options, sets or combinations (e.g. a Teamwork project
// without the right); no value when creating them is allowed.
static GS::Optional<GS::ObjectState> CheckDesignOptionCreationRights ()
{
    const auto error =DesignOptionsManipulationRightsChecker::CheckCreationRights ();
    if (error.has_value ()) {
        return CreateErrorResponse (error->kind, GS::UniString (error->text.c_str ()));
    }
    return GS::NoValue;
}
#endif

GetDesignOptionsCommand::GetDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDesignOptionsCommand::GetName () const
{
    return "GetDesignOptions";
}

GS::Optional<GS::UniString> GetDesignOptionsCommand::GetRawResponseSchema () const
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

GS::Optional<GS::UniString> GetDesignOptionSetsCommand::GetRawResponseSchema () const
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
            },
            "teamworkId": {
                "$ref": "#/GuidId",
                "description": "The Teamwork identifier of the project's design options. Present only in Teamwork projects."
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

    API_ProjectInfo projectInfo = {};
    if (ACAPI_ProjectOperation_Project (&projectInfo) == NoError && projectInfo.teamwork) {
        const auto teamworkId = manager->GetTWID ();
        if (teamworkId.IsOk () && teamworkId.Unwrap () != APINULLGuid) {
            response.Add ("teamworkId", CreateGuidObjectState (teamworkId.Unwrap ()));
        }
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

GS::Optional<GS::UniString> GetDesignOptionCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionCombinationDetails"
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
                    "$ref": "#/DesignOptionIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> GetElementsOfDesignOptionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elementsOfDesignOptions": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementsOfDesignOptionOrError"
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

GS::Optional<GS::UniString> GetDesignOptionForElementsCommand::GetRawResponseSchema () const
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
                            "$ref": "#/ElementId"
                        },
                        "type": {
                            "$ref": "#/DesignOptionLinkType"
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

GS::Optional<GS::UniString> CreateDesignOptionSetsCommand::GetRawResponseSchema () const
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

    const GS::Optional<GS::ObjectState> rightsError = CheckDesignOptionCreationRights ();
    if (rightsError.HasValue ()) {
        return *rightsError;
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
                    "$ref": "#/DesignOptionData"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> CreateDesignOptionsCommand::GetRawResponseSchema () const
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

    const GS::Optional<GS::ObjectState> rightsError = CheckDesignOptionCreationRights ();
    if (rightsError.HasValue ()) {
        return *rightsError;
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
                    "$ref": "#/DesignOptionCombinationData"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> CreateDesignOptionCombinationsCommand::GetRawResponseSchema () const
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

    const GS::Optional<GS::ObjectState> rightsError = CheckDesignOptionCreationRights ();
    if (rightsError.HasValue ()) {
        return *rightsError;
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

DeactivateDesignOptionsInCombinationsCommand::DeactivateDesignOptionsInCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeactivateDesignOptionsInCombinationsCommand::GetName () const
{
    return "DeactivateDesignOptionsInCombinations";
}

GS::Optional<GS::UniString> DeactivateDesignOptionsInCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "deactivatedDesignOptionsInCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/DesignOptionCombinationId"
                        },
                        "designOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The design options to turn off in the combination. The other options of the combination are left as they are."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "designOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "deactivatedDesignOptionsInCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> DeactivateDesignOptionsInCombinationsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeactivateDesignOptionsInCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design option combinations.");
    }

    GS::Array<GS::ObjectState> deactivatedDesignOptionsInCombinations;
    if (!parameters.Get ("deactivatedDesignOptionsInCombinations", deactivatedDesignOptionsInCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'deactivatedDesignOptionsInCombinations' parameter.");
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

    ACAPI_CallUndoableCommand ("Deactivate Design Options In Combinations", [&]() -> GSErrCode {
        for (auto&& deactivatedDesignOptionsInCombination : deactivatedDesignOptionsInCombinations) {
            GS::Guid combinationGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionCombinationId", deactivatedDesignOptionsInCombination));
            if (combinationGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionCombinationId' parameter"));
                continue;
            }
            DesignOptionCombination* combinationPtr = FindDesignOptionItemByGuid (*allDesignOptionCombinations, combinationGuid);
            if (combinationPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option combination with the given 'designOptionCombinationId' parameter"));
                continue;
            }
            GS::Array<GS::ObjectState> designOptionsToDeactivate;
            if (!deactivatedDesignOptionsInCombination.Get ("designOptions", designOptionsToDeactivate)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptions' parameter"));
                continue;
            }

            // Every option is looked up before any is turned off, so an unknown id changes nothing in the combination.
            std::vector<DesignOption*> optionPtrs;
            GS::Optional<GS::ObjectState> lookupError;
            for (const GS::ObjectState& designOptionToDeactivate : designOptionsToDeactivate) {
                GS::Guid optionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", designOptionToDeactivate));
                if (optionGuid.IsNull ()) {
                    lookupError = CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionId' parameter");
                    break;
                }
                DesignOption* optionPtr = FindDesignOptionItemByGuid (*allDesignOptions, optionGuid);
                if (optionPtr == nullptr) {
                    lookupError = CreateFailedExecutionResult (APIERR_BADID, "Not found design option with the given 'designOptionId' " + optionGuid.ToUniString ());
                    break;
                }
                optionPtrs.push_back (optionPtr);
            }
            if (lookupError.HasValue ()) {
                executionResults (lookupError.Get ());
                continue;
            }

            GS::Optional<GS::ObjectState> deactivationError;
            for (DesignOption* optionPtr : optionPtrs) {
                auto result = manager->DeactivateDesignOptionInCombination (*combinationPtr, *optionPtr);
                if (result.IsErr ()) {
                    deactivationError = CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to deactivate design option " + optionPtr->GetGuid ().ToUniString () + " in the combination.");
                    break;
                }
            }
            executionResults (deactivationError.HasValue () ? deactivationError.Get () : CreateSuccessfulExecutionResult ());
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

GS::Optional<GS::UniString> SetActiveDesignOptionsInCombinationsCommand::GetRawResponseSchema () const
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

DuplicateDesignOptionCombinationsCommand::DuplicateDesignOptionCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DuplicateDesignOptionCombinationsCommand::GetName () const
{
    return "DuplicateDesignOptionCombinations";
}

GS::Optional<GS::UniString> DuplicateDesignOptionCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionCombinationIdArrayItem"
                },
                "description": "The design option combinations to duplicate."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> DuplicateDesignOptionCombinationsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinationIdsOrErrors": {
                "$ref": "#/DesignOptionCombinationIdsOrErrors",
                "description": "The identifiers of the new combinations, in the order of the input, or an error for each combination that could not be duplicated."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinationIdsOrErrors"
        ]
    })";
}

GS::ObjectState DuplicateDesignOptionCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to duplicate design option combinations.");
    }

    GS::Array<GS::ObjectState> designOptionCombinations;
    if (!parameters.Get ("designOptionCombinations", designOptionCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionCombinations' parameter.");
    }

    auto allDesignOptionCombinations = manager->GetDesignOptionCombinations ();
    if (allDesignOptionCombinations.IsErr ()) {
        return CreateErrorResponse (allDesignOptionCombinations.UnwrapErr ().kind, "Unable to duplicate design option combinations.");
    }

    GS::ObjectState response;
    const auto& designOptionCombinationIdsOrErrors = response.AddList<GS::ObjectState> ("designOptionCombinationIdsOrErrors");

    ACAPI_CallUndoableCommand ("Duplicate Design Option Combinations", [&]() -> GSErrCode {
        for (auto&& designOptionCombination : designOptionCombinations) {
            GS::Guid combinationGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionCombinationId", designOptionCombination));
            if (combinationGuid.IsNull ()) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing or invalid 'designOptionCombinationId' parameter"));
                continue;
            }
            DesignOptionCombination* combinationPtr = FindDesignOptionItemByGuid (*allDesignOptionCombinations, combinationGuid);
            if (combinationPtr == nullptr) {
                designOptionCombinationIdsOrErrors (CreateErrorResponse (APIERR_BADID, "Not found design option combination with the given 'designOptionCombinationId' parameter"));
                continue;
            }
            auto result = manager->DuplicateDesignOptionCombination (*combinationPtr);
            designOptionCombinationIdsOrErrors (result.IsOk () ? CreateIdObjectState ("designOptionCombinationId", GSGuid2APIGuid (result->GetGuid ())) : CreateErrorResponse (result.UnwrapErr ().kind, "Failed to duplicate design option combination."));
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

GS::Optional<GS::UniString> MoveElementsToDesignOptionsCommand::GetRawResponseSchema () const
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
                        "designOptionId",
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

GS::Optional<GS::UniString> MoveDesignOptionsToAnotherSetCommand::GetRawResponseSchema () const
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

RenameDesignOptionsCommand::RenameDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RenameDesignOptionsCommand::GetName () const
{
    return "RenameDesignOptions";
}

GS::Optional<GS::UniString> RenameDesignOptionsCommand::GetInputParametersSchema () const
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
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option. The name is kept when it is not given."
                        },
                        "id": {
                            "type": "string",
                            "description": "The new string id of the design option. The id is kept when it is not given."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionId"
                    ]
                },
                "description": "The design options to rename, each with the new name and/or id. At least one of them must be given."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> RenameDesignOptionsCommand::GetRawResponseSchema () const
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

GS::ObjectState RenameDesignOptionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design options.");
    }

    GS::Array<GS::ObjectState> designOptions;
    if (!parameters.Get ("designOptions", designOptions)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptions' parameter.");
    }

    auto allDesignOptions = manager->GetAllDesignOptions ();
    if (allDesignOptions.IsErr ()) {
        return CreateErrorResponse (allDesignOptions.UnwrapErr ().kind, "Unable to modify design options.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Rename Design Options", [&]() -> GSErrCode {
        for (auto&& designOption : designOptions) {
            GS::Guid designOptionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", designOption));
            if (designOptionGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionId' parameter"));
                continue;
            }
            DesignOption* optionPtr = FindDesignOptionItemByGuid (*allDesignOptions, designOptionGuid);
            if (optionPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option with the given 'designOptionId' parameter"));
                continue;
            }
            GS::UniString name;
            const bool hasName = designOption.Get ("name", name);
            GS::UniString id;
            const bool hasId = designOption.Get ("id", id);
            if (!hasName && !hasId) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'name' and 'id' parameters, at least one of them is required"));
                continue;
            }
            if (hasName) {
                auto result = optionPtr->SetName (name);
                if (result.IsErr ()) {
                    executionResults (CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to rename design option."));
                    continue;
                }
            }
            if (hasId) {
                auto result = optionPtr->SetId (id);
                if (result.IsErr ()) {
                    executionResults (CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to change the id of design option."));
                    continue;
                }
            }
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

RenameDesignOptionSetsCommand::RenameDesignOptionSetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RenameDesignOptionSetsCommand::GetName () const
{
    return "RenameDesignOptionSets";
}

GS::Optional<GS::UniString> RenameDesignOptionSetsCommand::GetInputParametersSchema () const
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
                            "$ref": "#/DesignOptionSetId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionSetId",
                        "name"
                    ]
                },
                "description": "The design option sets to rename, each with the new name."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
        ]
    })";
}

GS::Optional<GS::UniString> RenameDesignOptionSetsCommand::GetRawResponseSchema () const
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

GS::ObjectState RenameDesignOptionSetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design option sets.");
    }

    GS::Array<GS::ObjectState> designOptionSets;
    if (!parameters.Get ("designOptionSets", designOptionSets)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionSets' parameter.");
    }

    auto sets = manager->GetDesignOptionSets ();
    if (sets.IsErr ()) {
        return CreateErrorResponse (sets.UnwrapErr ().kind, "Unable to modify design option sets.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Rename Design Option Sets", [&]() -> GSErrCode {
        for (auto&& designOptionSet : designOptionSets) {
            GS::Guid setGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionSetId", designOptionSet));
            if (setGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionSetId' parameter"));
                continue;
            }
            DesignOptionSet* setPtr = FindDesignOptionItemByGuid (*sets, setGuid);
            if (setPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option set with the given 'designOptionSetId' parameter"));
                continue;
            }
            GS::UniString name;
            if (!designOptionSet.Get ("name", name)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'name' parameter"));
                continue;
            }
            auto result = setPtr->SetName (name);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to rename design option set."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

RenameDesignOptionCombinationsCommand::RenameDesignOptionCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String RenameDesignOptionCombinationsCommand::GetName () const
{
    return "RenameDesignOptionCombinations";
}

GS::Optional<GS::UniString> RenameDesignOptionCombinationsCommand::GetInputParametersSchema () const
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
                            "$ref": "#/DesignOptionCombinationId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "name"
                    ]
                },
                "description": "The design option combinations to rename, each with the new name."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> RenameDesignOptionCombinationsCommand::GetRawResponseSchema () const
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

GS::ObjectState RenameDesignOptionCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design option combinations.");
    }

    GS::Array<GS::ObjectState> designOptionCombinations;
    if (!parameters.Get ("designOptionCombinations", designOptionCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionCombinations' parameter.");
    }

    auto allDesignOptionCombinations = manager->GetDesignOptionCombinations ();
    if (allDesignOptionCombinations.IsErr ()) {
        return CreateErrorResponse (allDesignOptionCombinations.UnwrapErr ().kind, "Unable to modify design option combinations.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Rename Design Option Combinations", [&]() -> GSErrCode {
        for (auto&& designOptionCombination : designOptionCombinations) {
            GS::Guid combinationGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionCombinationId", designOptionCombination));
            if (combinationGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionCombinationId' parameter"));
                continue;
            }
            DesignOptionCombination* combinationPtr = FindDesignOptionItemByGuid (*allDesignOptionCombinations, combinationGuid);
            if (combinationPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option combination with the given 'designOptionCombinationId' parameter"));
                continue;
            }
            GS::UniString name;
            if (!designOptionCombination.Get ("name", name)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'name' parameter"));
                continue;
            }
            auto result = combinationPtr->SetName (name);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to rename design option combination."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

DeleteDesignOptionsCommand::DeleteDesignOptionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteDesignOptionsCommand::GetName () const
{
    return "DeleteDesignOptions";
}

GS::Optional<GS::UniString> DeleteDesignOptionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionIdArrayItem"
                },
                "description": "The design options to delete."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteDesignOptionsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteDesignOptionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to delete design options.");
    }

    GS::Array<GS::ObjectState> designOptions;
    if (!parameters.Get ("designOptions", designOptions)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptions' parameter.");
    }

    auto allDesignOptions = manager->GetAllDesignOptions ();
    if (allDesignOptions.IsErr ()) {
        return CreateErrorResponse (allDesignOptions.UnwrapErr ().kind, "Unable to delete design options.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Delete Design Options", [&]() -> GSErrCode {
        for (auto&& designOption : designOptions) {
            GS::Guid designOptionGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionId", designOption));
            if (designOptionGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionId' parameter"));
                continue;
            }
            DesignOption* optionPtr = FindDesignOptionItemByGuid (*allDesignOptions, designOptionGuid);
            if (optionPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option with the given 'designOptionId' parameter"));
                continue;
            }
            auto result = manager->DeleteDesignOption (*optionPtr);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to delete design option."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

DeleteDesignOptionSetsCommand::DeleteDesignOptionSetsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteDesignOptionSetsCommand::GetName () const
{
    return "DeleteDesignOptionSets";
}

GS::Optional<GS::UniString> DeleteDesignOptionSetsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionSetIdArrayItem"
                },
                "description": "The design option sets to delete."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteDesignOptionSetsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteDesignOptionSetsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to delete design option sets.");
    }

    GS::Array<GS::ObjectState> designOptionSets;
    if (!parameters.Get ("designOptionSets", designOptionSets)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionSets' parameter.");
    }

    auto sets = manager->GetDesignOptionSets ();
    if (sets.IsErr ()) {
        return CreateErrorResponse (sets.UnwrapErr ().kind, "Unable to delete design option sets.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Delete Design Option Sets", [&]() -> GSErrCode {
        for (auto&& designOptionSet : designOptionSets) {
            GS::Guid setGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionSetId", designOptionSet));
            if (setGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionSetId' parameter"));
                continue;
            }
            DesignOptionSet* setPtr = FindDesignOptionItemByGuid (*sets, setGuid);
            if (setPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option set with the given 'designOptionSetId' parameter"));
                continue;
            }
            auto result = manager->DeleteDesignOptionSet (*setPtr);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to delete design option set."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

DeleteDesignOptionCombinationsCommand::DeleteDesignOptionCombinationsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteDesignOptionCombinationsCommand::GetName () const
{
    return "DeleteDesignOptionCombinations";
}

GS::Optional<GS::UniString> DeleteDesignOptionCombinationsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionCombinationIdArrayItem"
                },
                "description": "The design option combinations to delete."
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteDesignOptionCombinationsCommand::GetRawResponseSchema () const
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

GS::ObjectState DeleteDesignOptionCombinationsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to delete design option combinations.");
    }

    GS::Array<GS::ObjectState> designOptionCombinations;
    if (!parameters.Get ("designOptionCombinations", designOptionCombinations)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'designOptionCombinations' parameter.");
    }

    auto allDesignOptionCombinations = manager->GetDesignOptionCombinations ();
    if (allDesignOptionCombinations.IsErr ()) {
        return CreateErrorResponse (allDesignOptionCombinations.UnwrapErr ().kind, "Unable to delete design option combinations.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Delete Design Option Combinations", [&]() -> GSErrCode {
        for (auto&& designOptionCombination : designOptionCombinations) {
            GS::Guid combinationGuid = APIGuid2GSGuid (GetGuidFromArrayItem ("designOptionCombinationId", designOptionCombination));
            if (combinationGuid.IsNull ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing or invalid 'designOptionCombinationId' parameter"));
                continue;
            }
            DesignOptionCombination* combinationPtr = FindDesignOptionItemByGuid (*allDesignOptionCombinations, combinationGuid);
            if (combinationPtr == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Not found design option combination with the given 'designOptionCombinationId' parameter"));
                continue;
            }
            auto result = manager->DeleteDesignOptionCombination (*combinationPtr);
            executionResults (result.IsOk () ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to delete design option combination."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

// A hotlink node, and each instance placed from it, carries a design option
// table: which of the source file's sets, options and combinations it shows,
// and whether it filters by them at all. Elements the filter hides are as
// invisible to a script as elements of an inactive design option.

#ifdef ServerMainVers_2900
// The filter mode's numeric values are fixed by the DevKit (MainModelOnly = 0,
// FilterByOptions = 1, FilterByCombination = 2); the enum's own type is taken
// from the getter so this does not depend on where the enum is declared.
static const char* const HotlinkFilterModeNames[] = { "MainModelOnly", "FilterByOptions", "FilterByCombination" };
static const int HotlinkFilterModeCount = 3;

template <typename TableT>
static GS::UniString GetHotlinkFilterModeName (const TableT& table)
{
    const int filterMode = static_cast<int> (table.GetFilterMode ());
    if (filterMode < 0 || filterMode >= HotlinkFilterModeCount) {
        return "Unknown";
    }
    return GS::UniString (HotlinkFilterModeNames[filterMode]);
}

static GS::Optional<int> GetHotlinkFilterModeValue (const GS::UniString& name)
{
    for (int filterMode = 0; filterMode < HotlinkFilterModeCount; ++filterMode) {
        if (name == GS::UniString (HotlinkFilterModeNames[filterMode])) {
            return filterMode;
        }
    }
    return GS::NoValue;
}

template <typename TableT>
static GS::ObjectState CreateHotlinkDesignOptionTableObjectState (const TableT& table)
{
    GS::ObjectState tableOS;
    tableOS.Add ("hotlinkNodeId", CreateGuidObjectState (table.GetNodeGuid ()));
    tableOS.Add ("filterMode", GetHotlinkFilterModeName (table));

    const auto& combinations = tableOS.AddList<GS::UniString> ("designOptionCombinations");
    for (const GS::UniString& combinationName : table.GetCombinationsNames ()) {
        combinations (combinationName);
    }
    const std::optional<GS::UniString> activeCombination = table.GetActiveCombination ();
    if (activeCombination.has_value ()) {
        tableOS.Add ("activeDesignOptionCombination", *activeCombination);
    }

    const auto& sets = tableOS.AddList<GS::ObjectState> ("designOptionSets");
    for (const GS::UniString& setName : table.GetSetsNames ()) {
        GS::ObjectState setOS;
        setOS.Add ("name", setName);
        const auto& options = setOS.AddList<GS::UniString> ("designOptions");
        for (const GS::UniString& optionName : table.GetOptionsNamesOfSet (setName)) {
            options (optionName);
        }
        const std::optional<GS::UniString> activeOption = table.GetActiveOptionInSet (setName);
        if (activeOption.has_value ()) {
            setOS.Add ("activeDesignOption", *activeOption);
        }
        sets (setOS);
    }

    return tableOS;
}

// Reads a placed hotlink instance; APIERR_BADELEMENTTYPE when the element is
// something else.
static GSErrCode GetHotlinkInstance (const GS::ObjectState& elementId, API_Element& element)
{
    element = {};
    element.header.guid = GetGuidFromObjectState (elementId);
    const GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }
    return GetElemTypeId (element.header) == API_HotlinkID ? NoError : APIERR_BADELEMENTTYPE;
}

static GS::UniString GetHotlinkInstanceErrorMessage (GSErrCode err)
{
    return err == APIERR_BADELEMENTTYPE ? "The element is not a hotlink instance" : "Failed to get the element";
}
#endif

GetHotlinkDesignOptionTablesCommand::GetHotlinkDesignOptionTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetHotlinkDesignOptionTablesCommand::GetName () const
{
    return "GetHotlinkDesignOptionTables";
}

GS::Optional<GS::UniString> GetHotlinkDesignOptionTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinks": {
                "type": "array",
                "description": "The hotlink nodes (hotlinkNodeId from GetHotlinks) and placed hotlink instances (elementId of a Hotlink element) to read the design option table of.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "properties": {
                                "hotlinkNodeId": {
                                    "$ref": "#/HotlinkNodeId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "hotlinkNodeId"
                            ]
                        },
                        {
                            "properties": {
                                "elementId": {
                                    "$ref": "#/ElementId"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "elementId"
                            ]
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinks"
        ]
    })";
}

GS::Optional<GS::UniString> GetHotlinkDesignOptionTablesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkDesignOptionTables": {
                "type": "array",
                "description": "The design option tables, in the order of the given hotlinks.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "hotlinkNodeId": {
                                    "$ref": "#/HotlinkNodeId",
                                    "description": "The hotlink node the table belongs to; for an instance, the node it is placed from."
                                },
                                "filterMode": {
                                    "type": "string",
                                    "enum": [
                                        "MainModelOnly",
                                        "FilterByOptions",
                                        "FilterByCombination",
                                        "Unknown"
                                    ],
                                    "description": "Which elements of the hotlinked module are shown: only its main model, the main model and the active option of each set, or the main model and the options of the active combination."
                                },
                                "designOptionCombinations": {
                                    "type": "array",
                                    "items": {
                                        "type": "string"
                                    },
                                    "description": "The names of the design option combinations of the hotlinked module."
                                },
                                "activeDesignOptionCombination": {
                                    "type": "string",
                                    "description": "The name of the active combination. Missing when no combination is active."
                                },
                                "designOptionSets": {
                                    "type": "array",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "name": {
                                                "type": "string"
                                            },
                                            "designOptions": {
                                                "type": "array",
                                                "items": {
                                                    "type": "string"
                                                },
                                                "description": "The names of the design options in the set."
                                            },
                                            "activeDesignOption": {
                                                "type": "string",
                                                "description": "The name of the active design option of the set. Missing when no option of the set is active."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "name",
                                            "designOptions"
                                        ]
                                    },
                                    "description": "The design option sets of the hotlinked module."
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "hotlinkNodeId",
                                "filterMode",
                                "designOptionCombinations",
                                "designOptionSets"
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
            "hotlinkDesignOptionTables"
        ]
    })";
}

GS::ObjectState GetHotlinkDesignOptionTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to retrieve design options.");
    }

    GS::Array<GS::ObjectState> hotlinks;
    if (!parameters.Get ("hotlinks", hotlinks)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'hotlinks' parameter.");
    }

    GS::ObjectState response;
    const auto& tables = response.AddList<GS::ObjectState> ("hotlinkDesignOptionTables");

    for (const GS::ObjectState& hotlink : hotlinks) {
        const GS::ObjectState* hotlinkNodeId = hotlink.Get ("hotlinkNodeId");
        const GS::ObjectState* elementId = hotlink.Get ("elementId");
        if (hotlinkNodeId != nullptr) {
            API_HotlinkNode node = {};
            node.guid = GetGuidFromObjectState (*hotlinkNodeId);
            bool enableUnplaced = true;     // the node may have been created and not placed yet
            if (ACAPI_Hotlink_GetHotlinkNode (&node, &enableUnplaced) != NoError) {
                tables (CreateErrorResponse (APIERR_BADID, "hotlinkNodeId is not a hotlink node"));
                continue;
            }
            auto table = manager->GetHotlinkDesignOptionTable (node);
            if (table.IsErr ()) {
                tables (CreateErrorResponse (table.UnwrapErr ().kind, "Failed to get the design option table of the hotlink node."));
                continue;
            }
            tables (CreateHotlinkDesignOptionTableObjectState (*table));
        } else if (elementId != nullptr) {
            API_Element element;
            const GSErrCode err = GetHotlinkInstance (*elementId, element);
            if (err != NoError) {
                tables (CreateErrorResponse (err, GetHotlinkInstanceErrorMessage (err)));
                continue;
            }
            auto table = manager->GetHotlinkInstanceDesignOptionTable (element.hotlink);
            if (table.IsErr ()) {
                tables (CreateErrorResponse (table.UnwrapErr ().kind, "Failed to get the design option table of the hotlink instance."));
                continue;
            }
            tables (CreateHotlinkDesignOptionTableObjectState (*table));
        } else {
            tables (CreateErrorResponse (APIERR_BADPARS, "Either 'hotlinkNodeId' or 'elementId' is required"));
        }
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 29 or newer.");
#endif
}

SetHotlinkDesignOptionTablesCommand::SetHotlinkDesignOptionTablesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetHotlinkDesignOptionTablesCommand::GetName () const
{
    return "SetHotlinkDesignOptionTables";
}

GS::Optional<GS::UniString> SetHotlinkDesignOptionTablesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The placed hotlink instances to change the design option table of. Every field but elementId is optional; what is omitted keeps its current value. The changes are applied in the order filterMode, activeDesignOptionCombination, activeDesignOptions.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "filterMode": {
                            "type": "string",
                            "enum": [
                                "MainModelOnly",
                                "FilterByOptions",
                                "FilterByCombination"
                            ],
                            "description": "Which elements of the hotlinked module are shown: only its main model, the main model and the active option of each set, or the main model and the options of the active combination."
                        },
                        "activeDesignOptionCombination": {
                            "type": "string",
                            "description": "The name of the combination to activate, one of designOptionCombinations from GetHotlinkDesignOptionTables. An empty string deactivates the active combination."
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "description": "The option to activate in each given set; the previously active option of the set is deactivated. Sets that are not listed keep their active option.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "setName": {
                                        "type": "string"
                                    },
                                    "designOptionName": {
                                        "type": "string",
                                        "description": "The name of the option to activate in the set. An empty string deactivates every option of the set."
                                    }
                                },
                                "additionalProperties": false,
                                "required": [
                                    "setName",
                                    "designOptionName"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
        ]
    })";
}

GS::Optional<GS::UniString> SetHotlinkDesignOptionTablesCommand::GetRawResponseSchema () const
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

GS::ObjectState SetHotlinkDesignOptionTablesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2900
    ACAPI::Result<DesignOptionManager> manager = CreateDesignOptionManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Unable to modify design options.");
    }

    GS::Array<GS::ObjectState> hotlinkInstances;
    if (!parameters.Get ("hotlinkInstances", hotlinkInstances)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'hotlinkInstances' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Set Hotlink Design Option Tables", [&]() -> GSErrCode {
        for (const GS::ObjectState& instanceData : hotlinkInstances) {
            const GS::ObjectState* elementId = instanceData.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'elementId' parameter"));
                continue;
            }
            API_Element element;
            GSErrCode err = GetHotlinkInstance (*elementId, element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, GetHotlinkInstanceErrorMessage (err)));
                continue;
            }
            auto table = manager->GetHotlinkInstanceDesignOptionTable (element.hotlink);
            if (table.IsErr ()) {
                executionResults (CreateFailedExecutionResult (table.UnwrapErr ().kind, "Failed to get the design option table of the hotlink instance."));
                continue;
            }

            GS::UniString filterModeName;
            GS::Optional<int> filterMode;
            if (instanceData.Get ("filterMode", filterModeName)) {
                filterMode = GetHotlinkFilterModeValue (filterModeName);
                if (!filterMode.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid 'filterMode' parameter: " + filterModeName));
                    continue;
                }
                table->SetFilterMode (static_cast<decltype (table->GetFilterMode ())> (filterMode.Get ()));
            }

            GS::Optional<GS::UniString> failure;
            GS::UniString combinationName;
            const bool hasCombination = instanceData.Get ("activeDesignOptionCombination", combinationName);
            if (hasCombination) {
                if (combinationName.IsEmpty ()) {
                    table->DeactivateCombination ();
                } else {
                    const auto result = table->ActivateCombination (combinationName);
                    if (result.IsErr ()) {
                        err = result.UnwrapErr ().kind;
                        failure = "Failed to activate the design option combination " + combinationName;
                    }
                }
            }

            GS::Array<GS::ObjectState> activeDesignOptions;
            GS::HashTable<GS::UniString, GS::UniString> requestedOptionOfSet;
            if (!failure.HasValue () && instanceData.Get ("activeDesignOptions", activeDesignOptions)) {
                for (const GS::ObjectState& activeDesignOption : activeDesignOptions) {
                    GS::UniString setName;
                    GS::UniString optionName;
                    if (!activeDesignOption.Get ("setName", setName) || !activeDesignOption.Get ("designOptionName", optionName)) {
                        err = APIERR_BADPARS;
                        failure = GS::UniString ("Missing 'setName' or 'designOptionName' parameter");
                        break;
                    }
                    const auto result = optionName.IsEmpty () ? table->DeactivateOptionsInSet (setName) : table->ActivateOptionInSet (setName, optionName);
                    if (result.IsErr ()) {
                        err = result.UnwrapErr ().kind;
                        failure = "Failed to change the active design option of the set " + setName;
                        break;
                    }
                    requestedOptionOfSet.Put (setName, optionName);
                }
            }
            if (failure.HasValue ()) {
                executionResults (CreateFailedExecutionResult (err, failure.Get ()));
                continue;
            }

            const auto result = manager->SetHotlinkDesignOptionTable (element.hotlink, *table);
            if (result.IsErr ()) {
                executionResults (CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to set the design option table of the hotlink instance."));
                continue;
            }

            // Read the instance back, so a table that was not stored is
            // reported instead of claimed as done.
            API_Element storedElement;
            err = GetHotlinkInstance (*elementId, storedElement);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to read back the hotlink instance."));
                continue;
            }
            auto stored = manager->GetHotlinkInstanceDesignOptionTable (storedElement.hotlink);
            if (stored.IsErr ()) {
                executionResults (CreateFailedExecutionResult (stored.UnwrapErr ().kind, "Failed to read back the design option table of the hotlink instance."));
                continue;
            }
            if (filterMode.HasValue () && static_cast<int> (stored->GetFilterMode ()) != filterMode.Get ()) {
                executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The filter mode of the hotlink instance did not change."));
                continue;
            }
            if (hasCombination && stored->GetActiveCombination ().value_or (GS::UniString ()) != combinationName) {
                executionResults (CreateFailedExecutionResult (APIERR_GENERAL, "The active design option combination of the hotlink instance did not change."));
                continue;
            }
            for (const GS::ObjectState& activeDesignOption : activeDesignOptions) {
                GS::UniString setName;
                activeDesignOption.Get ("setName", setName);
                if (stored->GetActiveOptionInSet (setName).value_or (GS::UniString ()) != requestedOptionOfSet[setName]) {
                    failure = "The active design option of the set " + setName + " did not change.";
                    break;
                }
            }
            if (failure.HasValue ()) {
                executionResults (CreateFailedExecutionResult (APIERR_GENERAL, failure.Get ()));
                continue;
            }
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
