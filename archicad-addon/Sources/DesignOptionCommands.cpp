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
                    "type": "object",
                    "properties": {
                        "designOptionId": {
                            "$ref": "#/GuidId",
                            "description": "The guid identifier of the design option."
                        },
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
                        "designOptionId",
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
        return CreateErrorResponse (manager.UnwrapErr ().kind, "This command requires Archicad 27 or newer.");
    }

#ifdef ServerMainVers_2900
    const auto optionCombinations = manager->GetDesignOptionCombinations ();
#else
    const auto optionCombinations = manager->GetAvailableDesignOptionCombinations ();
#endif
    if (optionCombinations.IsErr ()) {
        return CreateErrorResponse (optionCombinations.UnwrapErr ().kind, "This command requires Archicad 27 or newer.");
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
