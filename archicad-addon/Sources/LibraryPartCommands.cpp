#include "LibraryPartCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/GSMObject.hpp"
// GSMObject.hpp declares the picture and 2D symbol getters with these types; included here for their members.
#if __has_include ("GXImage.hpp")
#include "GXImage.hpp"
#elif __has_include ("GXImage/GXImage.hpp")
#include "GXImage/GXImage.hpp"
#endif
#if __has_include ("VectorImage.hpp")
#include "VectorImage.hpp"
#endif
#include "NativeImage.hpp"
#include "MemoryOChannel32.hpp"
#include "Base64Converter.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using ACAPI::Library::GSMObject;

// The custom section type is taken from GetCustomSection, so it does not depend on the namespace it is declared in.
using GSMSectionData = std::decay_t<decltype (std::declval<const GSMObject&> ().GetCustomSection (0, 0).Unwrap ())>;

using LibraryPartChange = std::pair<GS::UniString, std::function<ACAPI::Result<void> (GSMObject::Modifier&)>>;

// Calls function with the GSM object of the loaded library part, or returns the error.
// asExecutionResult selects the error shape: an execution result for the Set commands, an error response otherwise.
template <typename Function>
static GS::ObjectState WithGSMObject (const GS::UniString& name, bool asExecutionResult, const Function& function)
{
    const auto Fail = [&] (GSErrCode errorCode, const GS::UniString& errorMessage) {
        return asExecutionResult ? CreateFailedExecutionResult (errorCode, errorMessage) : CreateErrorResponse (errorCode, errorMessage);
    };

    auto libraryManager = ACAPI::Library::GetLibraryManager ();
    if (libraryManager.IsErr ()) {
        return Fail (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
    }

    auto libPart = libraryManager->GetLibPartByName (name);
    if (libPart.IsErr ()) {
        return Fail (libPart.UnwrapErr ().kind, "No loaded library part with the name '" + name + "'. Use the fileName (with extension) returned by GetAvailableLibraryParts.");
    }

    auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart.Unwrap ());
    if (gsmObject.IsErr ()) {
        return Fail (gsmObject.UnwrapErr ().kind, "Failed to get the GSM object of the library part '" + name + "'.");
    }

    return function (gsmObject);
}

// Makes the changes in one GSMObject::Modify, in an undoable command, and returns the execution result.
template <typename GSMObjectResult>
static GS::ObjectState ModifyLibraryPart (GSMObjectResult& gsmObject, const GS::UniString& name, const GS::UniString& undoString, const std::vector<LibraryPartChange>& changes)
{
    if (!gsmObject->IsEditable ()) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "The library part '" + name + "' is not editable, nothing was changed. Only a library part loaded from a writable file can be modified; a part of a library package or a read-only library must first be copied into a writable library.");
    }

    GS::ObjectState result = CreateSuccessfulExecutionResult ();
    ACAPI_CallUndoableCommand (undoString, [&]() -> GSErrCode {
        GS::UniString failedChange;
        GS::UniString failedText;
        ACAPI::Result<void> modifyResult = gsmObject->Modify ([&] (GSMObject::Modifier& modifier) -> GSErrCode {
            for (const LibraryPartChange& change : changes) {
                ACAPI::Result<void> changeResult = change.second (modifier);
                if (changeResult.IsErr ()) {
                    failedChange = change.first;
                    failedText = changeResult.UnwrapErr ().text.c_str ();
                    return changeResult.UnwrapErr ().kind;
                }
            }
            return NoError;
        });

        if (modifyResult.IsErr ()) {
            GS::UniString errorMessage = "Failed to modify the library part '" + name + "'";
            GS::UniString errorText = modifyResult.UnwrapErr ().text.c_str ();
            if (!failedChange.IsEmpty ()) {
                errorMessage += ": '" + failedChange + "' failed";
                errorText = failedText;
            }
            if (!errorText.IsEmpty ()) {
                errorMessage += " (" + errorText + ")";
            }
            errorMessage += ".";
            result = CreateFailedExecutionResult (modifyResult.UnwrapErr ().kind, errorMessage);
        }
        return NoError;
    });

    return result;
}

static bool EncodeImageAsBase64Png (const GX::ImageRef& image, GS::UniString& base64)
{
    if (image == nullptr) {
        return false;
    }

    NewDisplay::NativeImage nativeImage = image->ToNativeImage ();
    GS::MemoryOChannel32 memChannel (GS::MemoryOChannel32::BMAllocation);
    if (!nativeImage.Encode (memChannel, NewDisplay::NativeImage::Encoding::PNG)) {
        return false;
    }

    base64 = Base64Converter::Encode (memChannel.GetDestination (), memChannel.GetDataSize ());
    base64.DeleteAll (GS::UniChar (char ('\n')));
    return true;
}

// Adds the pictures as a list of base64 PNG strings, or returns false when one cannot be encoded.
static bool AddPictureList (GS::ObjectState& os, const char* key, const std::vector<GX::ImageRef>& images)
{
    const auto& adder = os.AddList<GS::UniString> (key);
    for (const GX::ImageRef& image : images) {
        GS::UniString picture;
        if (!EncodeImageAsBase64Png (image, picture)) {
            return false;
        }
        adder (picture);
    }
    return true;
}

static GS::UniString EncodeBytesAsBase64 (const std::vector<std::byte>& bytes)
{
    if (bytes.empty ()) {
        return GS::UniString ();
    }

    GS::UniString base64 = Base64Converter::Encode (reinterpret_cast<const char*> (bytes.data ()), static_cast<USize> (bytes.size ()));
    base64.DeleteAll (GS::UniChar (char ('\n')));
    return base64;
}

static bool DecodeBase64 (const GS::UniString& base64, std::vector<std::byte>& bytes)
{
    const std::string text = base64.ToCStr ().Get ();
    bytes.clear ();

    UInt32 buffer = 0;
    int bitCount = 0;
    bool paddingReached = false;
    for (const char c : text) {
        int value = -1;
        if (c >= 'A' && c <= 'Z') {
            value = c - 'A';
        } else if (c >= 'a' && c <= 'z') {
            value = c - 'a' + 26;
        } else if (c >= '0' && c <= '9') {
            value = c - '0' + 52;
        } else if (c == '+') {
            value = 62;
        } else if (c == '/') {
            value = 63;
        } else if (c == '=') {
            paddingReached = true;
            continue;
        } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            continue;
        } else {
            return false;
        }

        if (paddingReached) {
            return false;
        }

        buffer = (buffer << 6) | static_cast<UInt32> (value);
        bitCount += 6;
        if (bitCount >= 8) {
            bitCount -= 8;
            bytes.push_back (static_cast<std::byte> ((buffer >> bitCount) & 0xFF));
            buffer &= (1u << bitCount) - 1;
        }
    }

    // A single leftover character (6 bits) cannot be valid base64.
    return bitCount < 6;
}

// Reads sectionId and index; on failure errorMessage is set.
static bool GetCustomSectionKey (const GS::ObjectState& parameters, uint32_t& sectionId, uint16_t& index, GS::UniString& errorMessage)
{
    UInt32 sectionIdValue = 0;
    if (!parameters.Get ("sectionId", sectionIdValue)) {
        errorMessage = "Invalid or missing 'sectionId' parameter.";
        return false;
    }

    UInt32 indexValue = 0;
    parameters.Get ("index", indexValue);
    if (indexValue > 0xFFFF) {
        errorMessage = "The 'index' parameter must be between 0 and 65535.";
        return false;
    }

    sectionId = static_cast<uint32_t> (sectionIdValue);
    index = static_cast<uint16_t> (indexValue);
    return true;
}
#endif

GetLibraryPartPicturesCommand::GetLibraryPartPicturesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetLibraryPartPicturesCommand::GetName () const
{
    return "GetLibraryPartPictures";
}

GS::Optional<GS::UniString> GetLibraryPartPicturesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartPicturesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "previewPicture": {
                "type": "string",
                "description": "The preview picture as a base64 encoded PNG. Left out when the library part has none."
            },
            "darkPreviewPicture": {
                "type": "string",
                "description": "The preview picture for the dark user interface as a base64 encoded PNG. Left out when the library part has none, and before Archicad 29."
            },
            "gdlPictures": {
                "type": "array",
                "description": "The pictures stored in the library part for its GDL scripts, as base64 encoded PNGs, in their stored order.",
                "items": {
                    "type": "string"
                }
            },
            "darkGdlPictures": {
                "type": "array",
                "description": "The GDL pictures for the dark user interface, as base64 encoded PNGs. Left out before Archicad 29.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "gdlPictures"
        ]
    })";
}

GS::ObjectState GetLibraryPartPicturesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    return WithGSMObject (name, false, [&] (auto& gsmObject) -> GS::ObjectState {
        GS::ObjectState response;
        GS::UniString picture;

        auto previewPicture = gsmObject->GetPreviewPicture ();
        if (!previewPicture.IsErr () && EncodeImageAsBase64Png (previewPicture.Unwrap (), picture)) {
            response.Add ("previewPicture", picture);
        }

        auto gdlPictures = gsmObject->GetGDLPictures ();
        if (gdlPictures.IsErr ()) {
            return CreateErrorResponse (gdlPictures.UnwrapErr ().kind, "Failed to get the GDL pictures of the library part '" + name + "'.");
        }
        if (!AddPictureList (response, "gdlPictures", gdlPictures.Unwrap ())) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to encode a GDL picture of the library part '" + name + "'.");
        }

#ifdef ServerMainVers_2900
        auto darkPreviewPicture = gsmObject->GetDarkPreviewPicture ();
        if (!darkPreviewPicture.IsErr () && EncodeImageAsBase64Png (darkPreviewPicture.Unwrap (), picture)) {
            response.Add ("darkPreviewPicture", picture);
        }

        auto darkGdlPictures = gsmObject->GetDarkGDLPictures ();
        if (darkGdlPictures.IsErr ()) {
            return CreateErrorResponse (darkGdlPictures.UnwrapErr ().kind, "Failed to get the dark GDL pictures of the library part '" + name + "'.");
        }
        if (!AddPictureList (response, "darkGdlPictures", darkGdlPictures.Unwrap ())) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to encode a dark GDL picture of the library part '" + name + "'.");
        }
#endif

        return response;
    });
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

SetLibraryPartPicturesCommand::SetLibraryPartPicturesCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetLibraryPartPicturesCommand::GetName () const
{
    return "SetLibraryPartPictures";
}

GS::Optional<GS::UniString> SetLibraryPartPicturesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
            },
            "deletePreviewPicture": {
                "type": "boolean",
                "description": "Optional. When true, the preview picture is deleted."
            },
            "deleteDarkPreviewPicture": {
                "type": "boolean",
                "description": "Optional. When true, the preview picture for the dark user interface is deleted. Requires Archicad 29 or newer."
            },
            "deleteGdlPictures": {
                "type": "boolean",
                "description": "Optional. When true, all GDL pictures are deleted."
            },
            "deleteDarkGdlPictures": {
                "type": "boolean",
                "description": "Optional. When true, all GDL pictures for the dark user interface are deleted. Requires Archicad 29 or newer."
            },
            "copy2DSymbolFrom": {
                "type": "string",
                "description": "Optional. The name of another loaded library part (fileName with extension); its 2D symbol (the drawn 2D vector image) replaces this part's 2D symbol."
            },
            "delete2DSymbol": {
                "type": "boolean",
                "description": "Optional. When true, the 2D symbol (the drawn 2D vector image) is deleted. Cannot be combined with copy2DSymbolFrom."
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> SetLibraryPartPicturesCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetLibraryPartPicturesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    bool deletePreviewPicture = false;
    bool deleteDarkPreviewPicture = false;
    bool deleteGdlPictures = false;
    bool deleteDarkGdlPictures = false;
    bool delete2DSymbol = false;
    GS::UniString copy2DSymbolFrom;
    parameters.Get ("deletePreviewPicture", deletePreviewPicture);
    parameters.Get ("deleteDarkPreviewPicture", deleteDarkPreviewPicture);
    parameters.Get ("deleteGdlPictures", deleteGdlPictures);
    parameters.Get ("deleteDarkGdlPictures", deleteDarkGdlPictures);
    parameters.Get ("delete2DSymbol", delete2DSymbol);
    parameters.Get ("copy2DSymbolFrom", copy2DSymbolFrom);

    if (delete2DSymbol && !copy2DSymbolFrom.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "'delete2DSymbol' and 'copy2DSymbolFrom' cannot be given together.");
    }

    std::vector<LibraryPartChange> changes;
    if (deletePreviewPicture) {
        changes.push_back ({ "deletePreviewPicture", [] (GSMObject::Modifier& modifier) { return modifier.DeletePreviewPicture (); } });
    }
    if (deleteDarkPreviewPicture) {
#ifdef ServerMainVers_2900
        changes.push_back ({ "deleteDarkPreviewPicture", [] (GSMObject::Modifier& modifier) { return modifier.DeleteDarkPreviewPicture (); } });
#else
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "'deleteDarkPreviewPicture' requires Archicad 29 or newer.");
#endif
    }
    if (deleteGdlPictures) {
        changes.push_back ({ "deleteGdlPictures", [] (GSMObject::Modifier& modifier) { return modifier.DeleteGDLPictures (); } });
    }
    if (deleteDarkGdlPictures) {
#ifdef ServerMainVers_2900
        changes.push_back ({ "deleteDarkGdlPictures", [] (GSMObject::Modifier& modifier) { return modifier.DeleteDarkGDLPictures (); } });
#else
        return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "'deleteDarkGdlPictures' requires Archicad 29 or newer.");
#endif
    }
    if (delete2DSymbol) {
        changes.push_back ({ "delete2DSymbol", [] (GSMObject::Modifier& modifier) { return modifier.Delete2DVectorImage (); } });
    }

    if (changes.empty () && copy2DSymbolFrom.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Nothing to change: give at least one of the delete options or 'copy2DSymbolFrom'.");
    }

    return WithGSMObject (name, true, [&] (auto& gsmObject) -> GS::ObjectState {
        if (copy2DSymbolFrom.IsEmpty ()) {
            return ModifyLibraryPart (gsmObject, name, "Set Library Part Pictures", changes);
        }

        return WithGSMObject (copy2DSymbolFrom, true, [&] (auto& sourceObject) -> GS::ObjectState {
            auto vectorImage = sourceObject->Get2DVectorImage ();
            if (vectorImage.IsErr ()) {
                return CreateFailedExecutionResult (vectorImage.UnwrapErr ().kind, "Failed to get the 2D symbol of the library part '" + copy2DSymbolFrom + "'.");
            }

            changes.push_back ({ "copy2DSymbolFrom", [&] (GSMObject::Modifier& modifier) { return modifier.Set2DVectorImage (vectorImage.Unwrap ()); } });
            return ModifyLibraryPart (gsmObject, name, "Set Library Part Pictures", changes);
        });
    });
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

GetLibraryPartCustomSectionCommand::GetLibraryPartCustomSectionCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String GetLibraryPartCustomSectionCommand::GetName () const
{
    return "GetLibraryPartCustomSection";
}

GS::Optional<GS::UniString> GetLibraryPartCustomSectionCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
            },
            "sectionId": {
                "type": "integer",
                "minimum": 0,
                "maximum": 4294967295,
                "description": "The type identifier of the custom section. A four-character code is given as its integer value, the first character in the highest byte (e.g. 'ABCD' is 1094861636)."
            },
            "index": {
                "type": "integer",
                "minimum": 0,
                "maximum": 65535,
                "description": "Optional. The index of the section among the sections with the same identifier. By default 0."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "sectionId"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPartCustomSectionCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "exists": {
                "type": "boolean",
                "description": "Whether the library part has this custom section."
            },
            "dataVersion": {
                "type": "integer",
                "description": "The data version stored with the section. Left out when the section does not exist."
            },
            "data": {
                "type": "string",
                "description": "The content of the section, base64 encoded. Left out when the section does not exist."
            }
        },
        "additionalProperties": false,
        "required": [
            "exists"
        ]
    })";
}

GS::ObjectState GetLibraryPartCustomSectionCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    uint32_t sectionId = 0;
    uint16_t index = 0;
    GS::UniString errorMessage;
    if (!GetCustomSectionKey (parameters, sectionId, index, errorMessage)) {
        return CreateErrorResponse (APIERR_BADPARS, errorMessage);
    }

    return WithGSMObject (name, false, [&] (auto& gsmObject) -> GS::ObjectState {
        if (!gsmObject->HasCustomSection (sectionId, index)) {
            return GS::ObjectState ("exists", false);
        }

        auto section = gsmObject->GetCustomSection (sectionId, index);
        if (section.IsErr ()) {
            return CreateErrorResponse (section.UnwrapErr ().kind, "Failed to get the custom section of the library part '" + name + "'.");
        }

        const GSMSectionData& sectionData = section.Unwrap ();
        return GS::ObjectState (
            "exists", true,
            "dataVersion", static_cast<Int32> (sectionData.dataVersion),
            "data", EncodeBytesAsBase64 (sectionData.data));
    });
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

SetLibraryPartCustomSectionCommand::SetLibraryPartCustomSectionCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String SetLibraryPartCustomSectionCommand::GetName () const
{
    return "SetLibraryPartCustomSection";
}

GS::Optional<GS::UniString> SetLibraryPartCustomSectionCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part file with its extension, as returned in the fileName field of GetAvailableLibraryParts (e.g. 'My Door.gsm')."
            },
            "sectionId": {
                "type": "integer",
                "minimum": 0,
                "maximum": 4294967295,
                "description": "The type identifier of the custom section. A four-character code is given as its integer value, the first character in the highest byte (e.g. 'ABCD' is 1094861636)."
            },
            "index": {
                "type": "integer",
                "minimum": 0,
                "maximum": 65535,
                "description": "Optional. The index of the section among the sections with the same identifier. By default 0."
            },
            "dataVersion": {
                "type": "integer",
                "minimum": -32768,
                "maximum": 32767,
                "description": "Optional. The data version stored with the section. By default 0. Only used with data."
            },
            "data": {
                "type": "string",
                "description": "The new content of the section, base64 encoded. The section is created when it does not exist, otherwise replaced. Give either data or delete."
            },
            "delete": {
                "type": "boolean",
                "description": "When true, the section is deleted. Give either data or delete."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "sectionId"
        ]
    })";
}

GS::Optional<GS::UniString> SetLibraryPartCustomSectionCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetLibraryPartCustomSectionCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    uint32_t sectionId = 0;
    uint16_t index = 0;
    GS::UniString errorMessage;
    if (!GetCustomSectionKey (parameters, sectionId, index, errorMessage)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, errorMessage);
    }

    bool deleteSection = false;
    parameters.Get ("delete", deleteSection);
    GS::UniString data;
    const bool hasData = parameters.Get ("data", data);
    if (deleteSection == hasData) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "Give either 'data' or 'delete': true.");
    }

    GSMSectionData sectionData = {};
    if (hasData) {
        Int32 dataVersion = 0;
        parameters.Get ("dataVersion", dataVersion);
        if (dataVersion < -32768 || dataVersion > 32767) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The 'dataVersion' parameter must be between -32768 and 32767.");
        }
        if (!DecodeBase64 (data, sectionData.data)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "The 'data' parameter is not valid base64.");
        }
        sectionData.dataVersion = static_cast<int16_t> (dataVersion);
    }

    return WithGSMObject (name, true, [&] (auto& gsmObject) -> GS::ObjectState {
        std::vector<LibraryPartChange> changes;
        if (deleteSection) {
            if (!gsmObject->HasCustomSection (sectionId, index)) {
                return CreateFailedExecutionResult (APIERR_BADPARS, "The library part '" + name + "' has no custom section with the given sectionId and index.");
            }
            changes.push_back ({ "delete", [&] (GSMObject::Modifier& modifier) { return modifier.DeleteCustomSection (sectionId, index); } });
        } else {
            changes.push_back ({ "data", [&] (GSMObject::Modifier& modifier) { return modifier.SetCustomSection (sectionId, index, sectionData); } });
        }
        return ModifyLibraryPart (gsmObject, name, "Set Library Part Custom Section", changes);
    });
#else
    UNUSED_PARAMETER (parameters);
    return CreateFailedExecutionResult (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}

CreateLibraryPartCommand::CreateLibraryPartCommand () :
    CommandBase (CommonSchema::Used)
{}

GS::String CreateLibraryPartCommand::GetName () const
{
    return "CreateLibraryPart";
}

GS::Optional<GS::UniString> CreateLibraryPartCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "The full path of the new library part file. It must end with .gsm, must not exist yet, and its folder must exist."
            },
            "ancestorSubtype": {
                "type": "string",
                "description": "The name of a loaded library part that is a subtype, with its extension, as returned in the fileName field of GetAvailableLibraryParts. The new object is of this subtype and inherits its parameters."
            }
        },
        "additionalProperties": false,
        "required": [
            "path",
            "ancestorSubtype"
        ]
    })";
}

GS::Optional<GS::UniString> CreateLibraryPartCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "isLoaded": {
                "type": "boolean",
                "description": "Whether the new library part is loaded in the project. It is only loaded when its folder is in a loaded library; otherwise only the file was created."
            },
            "libraryPart": {
                "type": "object",
                "description": "The new library part, in the same form as in GetAvailableLibraryParts. Left out when it is not loaded.",
                "properties": {
                    "guid": {
                        "type": "string"
                    },
                    "index": {
                        "type": "integer"
                    },
                    "documentName": {
                        "type": "string"
                    },
                    "fileName": {
                        "type": "string"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "guid",
                    "index",
                    "documentName",
                    "fileName"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "isLoaded"
        ]
    })";
}

GS::ObjectState CreateLibraryPartCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GS::UniString path;
    GS::UniString ancestorSubtype;
    if (!parameters.Get ("path", path) || path.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'path' parameter.");
    }
    if (!parameters.Get ("ancestorSubtype", ancestorSubtype) || ancestorSubtype.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'ancestorSubtype' parameter.");
    }

    const IO::Location fileLocation (path);
    IO::Name lastLocalName;
    if (fileLocation.GetLastLocalName (&lastLocalName) != NoError ||
        lastLocalName.GetExtension ().Compare ("gsm", CaseInsensitive) != GS::UniString::Equal) {
        return CreateErrorResponse (APIERR_BADPARS, "The 'path' parameter must be the path of a .gsm file.");
    }

    bool exists = false;
    if (IO::fileSystem.Contains (fileLocation, &exists) != NoError || exists) {
        return CreateErrorResponse (APIERR_BADPARS, "A file already exists on the given path.");
    }

    IO::Location folderLocation = fileLocation;
    if (folderLocation.DeleteLastLocalName () != NoError || IO::Folder (folderLocation).GetStatus () != NoError) {
        return CreateErrorResponse (APIERR_BADPARS, "The folder of the given path does not exist.");
    }

    return WithGSMObject (ancestorSubtype, false, [&] (auto& ancestorObject) -> GS::ObjectState {
        auto libraryManager = ACAPI::Library::GetLibraryManager ();
        if (libraryManager.IsErr ()) {
            return CreateErrorResponse (libraryManager.UnwrapErr ().kind, "Failed to get the Library Manager.");
        }

        GS::ObjectState response;
        ACAPI_CallUndoableCommand ("Create Library Part", [&]() -> GSErrCode {
            auto newObject = GSMObject::CreateNew (fileLocation, ancestorObject.Unwrap ());
            if (newObject.IsErr ()) {
                GS::UniString errorMessage = "Failed to create the library part";
                const GS::UniString errorText = newObject.UnwrapErr ().text.c_str ();
                if (!errorText.IsEmpty ()) {
                    errorMessage += " (" + errorText + ")";
                }
                errorMessage += ". The ancestorSubtype must be a subtype.";
                response = CreateErrorResponse (newObject.UnwrapErr ().kind, errorMessage);
                return NoError;
            }

            const bool isLoaded = !libraryManager->GetLibPartByGSMObject (newObject.Unwrap ()).IsErr ();
            response.Add ("isLoaded", isLoaded);
            if (!isLoaded) {
                return NoError;
            }

            // The index, name and guid are read with the C API, finding the part by its file location.
            Int32 partCount = 0;
            if (ACAPI_LibraryPart_GetNum (&partCount) != NoError) {
                return NoError;
            }
            for (Int32 i = partCount; i >= 1; --i) {
                API_LibPart libPart = {};
                libPart.index = i;
                if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
                    continue;
                }
                const bool found = libPart.location != nullptr && *libPart.location == fileLocation;
                if (found) {
                    response.Add ("libraryPart", GS::ObjectState (
                        "guid", GS::UnID (libPart.ownUnID).GetMainGuid ().ToUniString (),
                        "index", static_cast<Int32> (libPart.index),
                        "documentName", GS::UniString (libPart.docu_UName),
                        "fileName", GS::UniString (libPart.file_UName)));
                }
                delete libPart.location;
                libPart.location = nullptr;
                if (found) {
                    break;
                }
            }
            return NoError;
        });

        return response;
    });
#else
    UNUSED_PARAMETER (parameters);
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
