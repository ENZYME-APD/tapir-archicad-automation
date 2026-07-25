#include "KeynoteCommands.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/KeynoteManager.hpp"

#include <optional>
#include <utility>

using namespace ACAPI::Keynote;
#endif

#ifdef ServerMainVers_2800

static void AddKeynoteItemToObjectState (const KeynoteItem& item, GS::ObjectState& os)
{
    os.Add ("keynoteItemId", CreateGuidObjectState (item.GetId ()));
    os.Add ("key", item.GetKey ());
    os.Add ("title", item.GetTitle ());
    os.Add ("description", item.GetDescription ());
    os.Add ("reference", item.GetReference ());
    os.Add ("uiText", item.GetUIText ());
}

static void AddKeynoteFolderToObjectState (const KeynoteFolder& folder, GS::ObjectState& os)
{
    os.Add ("keynoteFolderId", CreateGuidObjectState (folder.GetId ()));
    os.Add ("key", folder.GetKey ());
    os.Add ("title", folder.GetTitle ());
    os.Add ("reference", folder.GetReference ());
    os.Add ("uiText", folder.GetUIText ());

    const auto& subFoldersList = os.AddList<GS::ObjectState> ("subFolders");
    auto subFolders = folder.GetDirectSubFolders ();
    if (subFolders.IsOk ()) {
        for (const auto& subFolder : subFolders.Unwrap ()) {
            GS::ObjectState subFolderOs;
            AddKeynoteFolderToObjectState (subFolder, subFolderOs);
            subFoldersList (subFolderOs);
        }
    }

    const auto& itemsList = os.AddList<GS::ObjectState> ("items");
    auto items = folder.GetDirectItems ();
    if (items.IsOk ()) {
        for (const auto& item : items.Unwrap ()) {
            GS::ObjectState itemOs;
            AddKeynoteItemToObjectState (item, itemOs);
            itemsList (itemOs);
        }
    }
}

static std::optional<KeynoteFolder> FindKeynoteFolderById (const KeynoteFolder& folder, const API_Guid& folderId)
{
    if (folder.GetId () == folderId) {
        return folder;
    }
    auto subFolders = folder.GetDirectSubFolders ();
    if (subFolders.IsOk ()) {
        for (const auto& subFolder : subFolders.Unwrap ()) {
            auto found = FindKeynoteFolderById (subFolder, folderId);
            if (found.has_value ()) {
                return found;
            }
        }
    }
    return std::nullopt;
}

static std::optional<std::pair<KeynoteFolder, KeynoteItem>> FindKeynoteItemById (const KeynoteFolder& folder, const API_Guid& itemId)
{
    auto items = folder.GetDirectItems ();
    if (items.IsOk ()) {
        for (const auto& item : items.Unwrap ()) {
            if (item.GetId () == itemId) {
                return std::make_pair (folder, item);
            }
        }
    }
    auto subFolders = folder.GetDirectSubFolders ();
    if (subFolders.IsOk ()) {
        for (const auto& subFolder : subFolders.Unwrap ()) {
            auto found = FindKeynoteItemById (subFolder, itemId);
            if (found.has_value ()) {
                return found;
            }
        }
    }
    return std::nullopt;
}

// Resolves the parent folder of a create operation: the folder given by the
// optional parentFolderId field, or the root folder when the field is absent.
static std::optional<KeynoteFolder> ResolveParentFolder (const KeynoteFolder& rootFolder, const GS::ObjectState& os)
{
    const GS::ObjectState* parentFolderId = os.Get ("parentFolderId");
    if (parentFolderId == nullptr) {
        return rootFolder;
    }
    return FindKeynoteFolderById (rootFolder, GetGuidFromObjectState (*parentFolderId));
}

static std::optional<KeynoteManager::AutoTextTokenSelector> ParseAutoTextTokenSelector (const GS::UniString& name)
{
    if (name == "Key") {
        return KeynoteManager::AutoTextTokenSelector::Key;
    }
    if (name == "Title") {
        return KeynoteManager::AutoTextTokenSelector::Title;
    }
    if (name == "Description") {
        return KeynoteManager::AutoTextTokenSelector::Description;
    }
    if (name == "Reference") {
        return KeynoteManager::AutoTextTokenSelector::Reference;
    }
    return std::nullopt;
}

#endif

GetKeynoteTreeCommand::GetKeynoteTreeCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetKeynoteTreeCommand::GetName () const
{
    return "GetKeynoteTree";
}

GS::Optional<GS::UniString> GetKeynoteTreeCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "rootFolder": {
                "$ref": "#/KeynoteFolderDetails"
            }
        },
        "additionalProperties": false,
        "required": [
            "rootFolder"
        ]
    })";
}

GS::ObjectState GetKeynoteTreeCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::ObjectState rootFolderOs;
    AddKeynoteFolderToObjectState (rootFolder.Unwrap (), rootFolderOs);

    return GS::ObjectState ("rootFolder", rootFolderOs);
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetKeynoteAutoTextsCommand::GetKeynoteAutoTextsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetKeynoteAutoTextsCommand::GetName () const
{
    return "GetKeynoteAutoTexts";
}

GS::Optional<GS::UniString> GetKeynoteAutoTextsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keynoteItems": {
                "type": "array",
                "description": "The keynote items to get the autotext tokens for.",
                "items": {
                    "$ref": "#/KeynoteItemIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItems"
        ]
    })";
}

GS::Optional<GS::UniString> GetKeynoteAutoTextsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "autoTexts": {
                "type": "array",
                "description": "The autotext tokens of the keynote items. An autotext token can be used as the text content of a label to reference the field of the keynote item.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "keyToken": { "type": "string" },
                                "titleToken": { "type": "string" },
                                "descriptionToken": { "type": "string" },
                                "referenceToken": { "type": "string" }
                            },
                            "additionalProperties": false,
                            "required": [ "keyToken", "titleToken", "descriptionToken", "referenceToken" ]
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
            "autoTexts"
        ]
    })";
}

GS::ObjectState GetKeynoteAutoTextsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    GS::Array<GS::ObjectState> keynoteItems;
    if (!parameters.Get ("keynoteItems", keynoteItems)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'keynoteItems' parameter.");
    }

    GS::ObjectState response;
    const auto& autoTexts = response.AddList<GS::ObjectState> ("autoTexts");

    for (const GS::ObjectState& keynoteItem : keynoteItems) {
        const API_Guid itemId = GetGuidFromArrayItem ("keynoteItemId", keynoteItem);

        auto keyToken = manager->GetAutoTextTokenFor (itemId, KeynoteManager::AutoTextTokenSelector::Key);
        auto titleToken = manager->GetAutoTextTokenFor (itemId, KeynoteManager::AutoTextTokenSelector::Title);
        auto descriptionToken = manager->GetAutoTextTokenFor (itemId, KeynoteManager::AutoTextTokenSelector::Description);
        auto referenceToken = manager->GetAutoTextTokenFor (itemId, KeynoteManager::AutoTextTokenSelector::Reference);

        if (keyToken.IsErr () || titleToken.IsErr () || descriptionToken.IsErr () || referenceToken.IsErr ()) {
            autoTexts (CreateErrorResponse (APIERR_BADPARS, "Failed to get autotext tokens for the keynote item."));
            continue;
        }

        GS::ObjectState autoTextOs;
        autoTextOs.Add ("keyToken", keyToken.Unwrap ());
        autoTextOs.Add ("titleToken", titleToken.Unwrap ());
        autoTextOs.Add ("descriptionToken", descriptionToken.Unwrap ());
        autoTextOs.Add ("referenceToken", referenceToken.Unwrap ());
        autoTexts (autoTextOs);
    }

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateKeynoteFoldersCommand::CreateKeynoteFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateKeynoteFoldersCommand::GetName () const
{
    return "CreateKeynoteFolders";
}

GS::Optional<GS::UniString> CreateKeynoteFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "foldersData": {
                "type": "array",
                "description": "Array of data to create keynote folders.",
                "items": {
                    "type": "object",
                    "properties": {
                        "parentFolderId": {
                            "$ref": "#/KeynoteFolderId",
                            "description": "The parent folder. Optional; defaults to the root folder."
                        },
                        "key": {
                            "type": "string"
                        },
                        "title": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "key",
                        "title"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "foldersData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateKeynoteFoldersCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keynoteFolderIdsOrErrors": {
                "$ref": "#/KeynoteFolderIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteFolderIdsOrErrors"
        ]
    })";
}

GS::ObjectState CreateKeynoteFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> foldersData;
    if (!parameters.Get ("foldersData", foldersData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'foldersData' parameter.");
    }

    GS::ObjectState response;
    const auto& keynoteFolderIdsOrErrors = response.AddList<GS::ObjectState> ("keynoteFolderIdsOrErrors");

    ACAPI_CallUndoableCommand ("Create Keynote Folders", [&]() -> GSErrCode {
        for (const GS::ObjectState& folderData : foldersData) {
            GS::UniString key;
            GS::UniString title;
            if (!folderData.Get ("key", key) || !folderData.Get ("title", title)) {
                keynoteFolderIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'key' or 'title' parameter."));
                continue;
            }

            std::optional<KeynoteFolder> parentFolder = ResolveParentFolder (rootFolder.Unwrap (), folderData);
            if (!parentFolder.has_value ()) {
                keynoteFolderIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Failed to find the parent folder."));
                continue;
            }

            auto newFolder = parentFolder->AddSubFolder (key, title);
            if (newFolder.IsErr ()) {
                keynoteFolderIdsOrErrors (CreateErrorResponse (newFolder.UnwrapErr ().kind, "Failed to create the keynote folder."));
                continue;
            }

            keynoteFolderIdsOrErrors (CreateIdObjectState ("keynoteFolderId", newFolder->GetId ()));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateKeynoteItemsCommand::CreateKeynoteItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateKeynoteItemsCommand::GetName () const
{
    return "CreateKeynoteItems";
}

GS::Optional<GS::UniString> CreateKeynoteItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "itemsData": {
                "type": "array",
                "description": "Array of data to create keynote items.",
                "items": {
                    "type": "object",
                    "properties": {
                        "parentFolderId": {
                            "$ref": "#/KeynoteFolderId",
                            "description": "The parent folder. Optional; defaults to the root folder."
                        },
                        "key": {
                            "type": "string"
                        },
                        "title": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "key"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "itemsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateKeynoteItemsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keynoteItemIdsOrErrors": {
                "$ref": "#/KeynoteItemIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItemIdsOrErrors"
        ]
    })";
}

GS::ObjectState CreateKeynoteItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> itemsData;
    if (!parameters.Get ("itemsData", itemsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'itemsData' parameter.");
    }

    GS::ObjectState response;
    const auto& keynoteItemIdsOrErrors = response.AddList<GS::ObjectState> ("keynoteItemIdsOrErrors");

    ACAPI_CallUndoableCommand ("Create Keynote Items", [&]() -> GSErrCode {
        for (const GS::ObjectState& itemData : itemsData) {
            GS::UniString key;
            if (!itemData.Get ("key", key)) {
                keynoteItemIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Missing 'key' parameter."));
                continue;
            }

            std::optional<KeynoteFolder> parentFolder = ResolveParentFolder (rootFolder.Unwrap (), itemData);
            if (!parentFolder.has_value ()) {
                keynoteItemIdsOrErrors (CreateErrorResponse (APIERR_BADPARS, "Failed to find the parent folder."));
                continue;
            }

            auto newItem = parentFolder->AddItem (key);
            if (newItem.IsErr ()) {
                keynoteItemIdsOrErrors (CreateErrorResponse (newItem.UnwrapErr ().kind, "Failed to create the keynote item."));
                continue;
            }

            GS::UniString title;
            if (itemData.Get ("title", title)) {
                newItem->SetTitle (title);
            }
            GS::UniString description;
            if (itemData.Get ("description", description)) {
                newItem->SetDescription (description);
            }
            GS::UniString reference;
            if (itemData.Get ("reference", reference)) {
                newItem->SetReference (reference);
            }

            keynoteItemIdsOrErrors (CreateIdObjectState ("keynoteItemId", newItem->GetId ()));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ModifyKeynoteFoldersCommand::ModifyKeynoteFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyKeynoteFoldersCommand::GetName () const
{
    return "ModifyKeynoteFolders";
}

GS::Optional<GS::UniString> ModifyKeynoteFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "foldersData": {
                "type": "array",
                "description": "Array of data to modify keynote folders. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteFolderId": {
                            "$ref": "#/KeynoteFolderId"
                        },
                        "key": {
                            "type": "string"
                        },
                        "title": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteFolderId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "foldersData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyKeynoteFoldersCommand::GetResponseSchema () const
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

GS::ObjectState ModifyKeynoteFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> foldersData;
    if (!parameters.Get ("foldersData", foldersData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'foldersData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Modify Keynote Folders", [&]() -> GSErrCode {
        for (const GS::ObjectState& folderData : foldersData) {
            const GS::ObjectState* folderId = folderData.Get ("keynoteFolderId");
            if (folderId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'keynoteFolderId' parameter."));
                continue;
            }

            std::optional<KeynoteFolder> folder = FindKeynoteFolderById (rootFolder.Unwrap (), GetGuidFromObjectState (*folderId));
            if (!folder.has_value ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to find the keynote folder."));
                continue;
            }

            bool success = true;
            GS::UniString key;
            if (folderData.Get ("key", key)) {
                success &= folder->SetKey (key).IsOk ();
            }
            GS::UniString title;
            if (folderData.Get ("title", title)) {
                success &= folder->SetTitle (title).IsOk ();
            }
            GS::UniString reference;
            if (folderData.Get ("reference", reference)) {
                success &= folder->SetReference (reference).IsOk ();
            }

            executionResults (success
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify the keynote folder."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

ModifyKeynoteItemsCommand::ModifyKeynoteItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyKeynoteItemsCommand::GetName () const
{
    return "ModifyKeynoteItems";
}

GS::Optional<GS::UniString> ModifyKeynoteItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "itemsData": {
                "type": "array",
                "description": "Array of data to modify keynote items. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteItemId": {
                            "$ref": "#/KeynoteItemId"
                        },
                        "key": {
                            "type": "string"
                        },
                        "title": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteItemId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "itemsData"
        ]
    })";
}

GS::Optional<GS::UniString> ModifyKeynoteItemsCommand::GetResponseSchema () const
{
    return ModifyKeynoteFoldersCommand ().GetResponseSchema ();
}

GS::ObjectState ModifyKeynoteItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> itemsData;
    if (!parameters.Get ("itemsData", itemsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'itemsData' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Modify Keynote Items", [&]() -> GSErrCode {
        for (const GS::ObjectState& itemData : itemsData) {
            const GS::ObjectState* itemId = itemData.Get ("keynoteItemId");
            if (itemId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Missing 'keynoteItemId' parameter."));
                continue;
            }

            std::optional<std::pair<KeynoteFolder, KeynoteItem>> found = FindKeynoteItemById (rootFolder.Unwrap (), GetGuidFromObjectState (*itemId));
            if (!found.has_value ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to find the keynote item."));
                continue;
            }

            KeynoteItem& item = found->second;
            bool success = true;
            GS::UniString key;
            if (itemData.Get ("key", key)) {
                success &= item.SetKey (key).IsOk ();
            }
            GS::UniString title;
            if (itemData.Get ("title", title)) {
                success &= item.SetTitle (title).IsOk ();
            }
            GS::UniString description;
            if (itemData.Get ("description", description)) {
                success &= item.SetDescription (description).IsOk ();
            }
            GS::UniString reference;
            if (itemData.Get ("reference", reference)) {
                success &= item.SetReference (reference).IsOk ();
            }

            executionResults (success
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (APIERR_GENERAL, "Failed to modify the keynote item."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

DeleteKeynoteFoldersCommand::DeleteKeynoteFoldersCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteKeynoteFoldersCommand::GetName () const
{
    return "DeleteKeynoteFolders";
}

GS::Optional<GS::UniString> DeleteKeynoteFoldersCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keynoteFolderIds": {
                "type": "array",
                "description": "The keynote folders to delete.",
                "items": {
                    "$ref": "#/KeynoteFolderIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteFolderIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteKeynoteFoldersCommand::GetResponseSchema () const
{
    return ModifyKeynoteFoldersCommand ().GetResponseSchema ();
}

GS::ObjectState DeleteKeynoteFoldersCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> keynoteFolderIds;
    if (!parameters.Get ("keynoteFolderIds", keynoteFolderIds)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'keynoteFolderIds' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Delete Keynote Folders", [&]() -> GSErrCode {
        for (const GS::ObjectState& keynoteFolderId : keynoteFolderIds) {
            const API_Guid folderId = GetGuidFromArrayItem ("keynoteFolderId", keynoteFolderId);

            if (folderId == rootFolder.Unwrap ().GetId ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The root folder cannot be deleted."));
                continue;
            }

            std::optional<KeynoteFolder> folder = FindKeynoteFolderById (rootFolder.Unwrap (), folderId);
            if (!folder.has_value ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to find the keynote folder."));
                continue;
            }

            ACAPI::Result<KeynoteFolder> parentFolder = folder->GetParentFolder ();
            if (parentFolder.IsErr ()) {
                executionResults (CreateFailedExecutionResult (parentFolder.UnwrapErr ().kind, "Failed to get the parent of the keynote folder."));
                continue;
            }

            ACAPI::Result<void> result = parentFolder->RemoveSubFolder (folder->GetKey (), folder->GetTitle ());
            executionResults (result.IsOk ()
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to delete the keynote folder."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

DeleteKeynoteItemsCommand::DeleteKeynoteItemsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String DeleteKeynoteItemsCommand::GetName () const
{
    return "DeleteKeynoteItems";
}

GS::Optional<GS::UniString> DeleteKeynoteItemsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "keynoteItemIds": {
                "type": "array",
                "description": "The keynote items to delete.",
                "items": {
                    "$ref": "#/KeynoteItemIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItemIds"
        ]
    })";
}

GS::Optional<GS::UniString> DeleteKeynoteItemsCommand::GetResponseSchema () const
{
    return ModifyKeynoteFoldersCommand ().GetResponseSchema ();
}

GS::ObjectState DeleteKeynoteItemsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    ACAPI::Result<KeynoteFolder> rootFolder = manager->GetRootFolder ();
    if (rootFolder.IsErr ()) {
        return CreateErrorResponse (rootFolder.UnwrapErr ().kind, "Failed to get keynote root folder.");
    }

    GS::Array<GS::ObjectState> keynoteItemIds;
    if (!parameters.Get ("keynoteItemIds", keynoteItemIds)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'keynoteItemIds' parameter.");
    }

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Delete Keynote Items", [&]() -> GSErrCode {
        for (const GS::ObjectState& keynoteItemId : keynoteItemIds) {
            std::optional<std::pair<KeynoteFolder, KeynoteItem>> found = FindKeynoteItemById (rootFolder.Unwrap (), GetGuidFromArrayItem ("keynoteItemId", keynoteItemId));
            if (!found.has_value ()) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Failed to find the keynote item."));
                continue;
            }

            ACAPI::Result<void> result = found->first.RemoveItem (found->second.GetKey ());
            executionResults (result.IsOk ()
                ? CreateSuccessfulExecutionResult ()
                : CreateFailedExecutionResult (result.UnwrapErr ().kind, "Failed to delete the keynote item."));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateKeynoteLabelsCommand::CreateKeynoteLabelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateKeynoteLabelsCommand::GetName () const
{
    return "CreateKeynoteLabels";
}

GS::Optional<GS::UniString> CreateKeynoteLabelsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "labelsData": {
                "type": "array",
                "description": "Array of data to create keynote labels.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteItemId": {
                            "$ref": "#/KeynoteItemId"
                        },
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "The reference point of the label."
                        },
                        "contentFields": {
                            "type": "array",
                            "description": "The keynote fields to include in the label text as autotext. Optional; defaults to all fields.",
                            "items": {
                                "type": "string",
                                "enum": ["Key", "Title", "Description", "Reference"]
                            },
                            "minItems": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteItemId",
                        "position"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "labelsData"
        ]
    })";
}

GS::Optional<GS::UniString> CreateKeynoteLabelsCommand::GetResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The identifiers of the created label elements or errors.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/ElementIdArrayItem"
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
            "elements"
        ]
    })";
}

GS::ObjectState CreateKeynoteLabelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    ACAPI::Result<KeynoteManager> manager = CreateKeynoteManager ();
    if (manager.IsErr ()) {
        return CreateErrorResponse (manager.UnwrapErr ().kind, "Failed to get Keynote Manager.");
    }

    GS::Array<GS::ObjectState> labelsData;
    if (!parameters.Get ("labelsData", labelsData)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'labelsData' parameter.");
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    ACAPI_CallUndoableCommand ("Create Keynote Labels", [&]() -> GSErrCode {
        for (const GS::ObjectState& labelData : labelsData) {
            const GS::ObjectState* keynoteItemId = labelData.Get ("keynoteItemId");
            const GS::ObjectState* position = labelData.Get ("position");
            if (keynoteItemId == nullptr || position == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Missing 'keynoteItemId' or 'position' parameter."));
                continue;
            }
            const API_Guid itemId = GetGuidFromObjectState (*keynoteItemId);

            GS::Array<GS::UniString> contentFields;
            if (!labelData.Get ("contentFields", contentFields) || contentFields.IsEmpty ()) {
                contentFields = { "Key", "Title", "Description", "Reference" };
            }

            GS::UniString textContent;
            bool tokenError = false;
            for (const GS::UniString& contentField : contentFields) {
                std::optional<KeynoteManager::AutoTextTokenSelector> selector = ParseAutoTextTokenSelector (contentField);
                if (!selector.has_value ()) {
                    tokenError = true;
                    break;
                }
                auto token = manager->GetAutoTextTokenFor (itemId, selector.value ());
                if (token.IsErr ()) {
                    tokenError = true;
                    break;
                }
                if (!textContent.IsEmpty ()) {
                    textContent += "\n";
                }
                textContent += token.Unwrap ();
            }
            if (tokenError) {
                elements (CreateErrorResponse (APIERR_BADPARS, "Failed to get autotext tokens for the keynote item."));
                continue;
            }

            API_Element element = {};
            API_ElementMemo memo = {};
            element.header.type = API_LabelID;

            GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to get the default label."));
                ACAPI_DisposeElemMemoHdls (&memo);
                continue;
            }

            const API_Coord coordinate = Get2DCoordinateFromObjectState (*position);
            element.label.labelClass = APILblClass_Text;
            element.label.parent     = APINULLGuid;
            element.label.begC       = coordinate;
            element.label.midC.x     = coordinate.x + 1.0;
            element.label.midC.y     = coordinate.y + 0.5;
            element.label.endC.x     = coordinate.x + 3.0;
            element.label.endC.y     = coordinate.y + 0.5;

            delete memo.textContent;
            memo.textContent = new GS::UniString { textContent };
            (*memo.paragraphs)[0].range = Strlen32 (textContent.ToCStr ().Get ());
            (*memo.paragraphs)[0].run[0].range = Strlen32 (textContent.ToCStr ().Get ());

            err = ACAPI_Element_Create (&element, &memo);
            ACAPI_DisposeElemMemoHdls (&memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to create the label."));
                continue;
            }

            elements (CreateElementIdObjectState (element.header.guid));
        }
        return NoError;
    });

    return response;
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
