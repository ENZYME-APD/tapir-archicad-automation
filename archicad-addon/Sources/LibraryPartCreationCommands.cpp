#include "LibraryPartCreationCommands.hpp"
#include "ElementGDLParameterCommands.hpp"
#include "MigrationHelper.hpp"
#include "ObjectState.hpp"
#include "Folder.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSUnID.hpp"
#include "Model.hpp"

#include <cstring>

// The library part functions these commands use, by their Archicad 27+ names, with the
// Archicad 25-26 names behind them. They are named apart from the ACAPI_ names so they
// never clash with a mapping MigrationHelper.hpp may get later.
namespace {

using LibPartSectionTypeId = decltype (API_LibPartSection::sectType);

GSErrCode SetUpSymbolDrawing ()
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_SetUpSect_2DDrawHdl ();
#else
    return ACAPI_LibPart_SetUpSect_2DDrawHdl ();
#endif
}

GSErrCode CloseSymbolDrawing (GSHandle* drawHdl)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_GetSect_2DDrawHdl (drawHdl);
#else
    return ACAPI_LibPart_GetSect_2DDrawHdl (drawHdl);
#endif
}

GSErrCode NewLibPartSection (const API_LibPartSection* section)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_NewSection (section);
#else
    return ACAPI_LibPart_NewSection (section);
#endif
}

GSErrCode WriteLibPartSection (Int32 nByte, GSConstPtr data)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_WriteSection (nByte, data);
#else
    return ACAPI_LibPart_WriteSection (nByte, data);
#endif
}

GSErrCode EndLibPartSection ()
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_EndSection ();
#else
    return ACAPI_LibPart_EndSection ();
#endif
}

GSErrCode SetLibPartDetails (const API_LibPart* libPart, GSHandle paramHdl, const API_LibPartDetails* details)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_SetDetails_ParamDef (libPart, paramHdl, details);
#else
    return ACAPI_LibPart_SetDetails_ParamDef (libPart, paramHdl, details);
#endif
}

GSErrCode SetLibPartOverwriteMode (bool overwriteOn)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryManagement_OverwriteLibPart (overwriteOn);
#else
    // The parameter is passed as a value, not as a pointer.
    return ACAPI_Environment (APIEnv_OverwriteLibPartID, (void*) (Int64) (overwriteOn ? 1 : 0));
#endif
}

GSErrCode AddLibraryFolder (IO::Location* folder)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryManagement_AddLibraries (folder);
#else
    return ACAPI_Environment (APIEnv_AddLibrariesID, folder);
#endif
}

GSErrCode GetLibPartSection (Int32 libInd, API_LibPartSection* section, GS::UniString* sectionStr)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_GetSection (libInd, section, nullptr, sectionStr);
#else
    return ACAPI_LibPart_GetSection (libInd, section, nullptr, sectionStr);
#endif
}

GSErrCode InterpretScript3D (const GS::UniString& script, API_AddParType** addPars, ModelerAPI::Model& model)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryManagement_InterpretGDLScript3D (&script, addPars, &model);
#else
    return ACAPI_Goodies (APIAny_InterpretGDLScript3DID, (void*) &script, (void*) addPars, &model);
#endif
}

template<std::size_t N>
void CopyToUCharBuffer (const GS::UniString& source, GS::uchar_t (&target)[N])
{
    const auto ustrObject = source.ToUStr ();
    const GS::uchar_t* sourceString = ustrObject;
    GS::ucsncpy (target, sourceString, N);
    target[N - 1] = 0;
}

// The loaded library part whose file name (with extension) or document name is the
// given one, as GetAvailableLibraryParts lists them. Its location is deleted here.
bool FindLoadedLibraryPartByName (const GS::UniString& name, API_LibPart& found)
{
    Int32 partCount = 0;
    if (ACAPI_LibraryPart_GetNum (&partCount) != NoError) {
        return false;
    }
    for (Int32 i = 1; i <= partCount; ++i) {
        API_LibPart libPart = {};
        libPart.index = i;
        if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
            continue;
        }
        delete libPart.location;
        libPart.location = nullptr;
        if (GS::UniString (libPart.file_UName) == name || GS::UniString (libPart.docu_UName) == name) {
            found = libPart;
            return true;
        }
    }
    return false;
}

// Fills parentUnID (and typeID, when the parent is a loaded library part) from
// "parentId" - the parent subtype's unique ID ("{main}-{revision}") or main GUID - or
// from "parentName", the parent subtype's document name, as CreateLibraryParts does.
GS::Optional<GS::ObjectState> ResolveParentSubtype (const GS::ObjectState& parameters, API_LibPart& newLibPart)
{
    GS::UniString parentId;
    GS::UniString parentName;
    parameters.Get ("parentId", parentId);
    parameters.Get ("parentName", parentName);

    if (!parentId.IsEmpty ()) {
        if (parentId.Contains ("}-{")) {
            CHTruncate (parentId.ToCStr ().Get (), newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
            return {};
        }

        const API_Guid parentApiGuid = APIGuidFromString (parentId.ToCStr ());
        if (parentApiGuid == APINULLGuid) {
            return CreateErrorResponse (APIERR_BADPARS, "parentId is neither a unique ID ({main}-{revision}) nor a GUID.");
        }
        const GS::Guid parentGuid = APIGuid2GSGuid (parentApiGuid);

        Int32 partCount = 0;
        if (ACAPI_LibraryPart_GetNum (&partCount) != NoError) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to enumerate library parts.");
        }
        for (Int32 i = 1; i <= partCount; ++i) {
            API_LibPart libPart = {};
            libPart.index = i;
            if (ACAPI_LibraryPart_Get (&libPart) != NoError) {
                continue;
            }
            delete libPart.location;
            if (GS::UnID (libPart.ownUnID).GetMainGuid () == parentGuid) {
                CHTruncate (libPart.ownUnID, newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
                newLibPart.typeID = libPart.typeID;
                return {};
            }
        }
        return CreateErrorResponse (APIERR_BADPARS, "No loaded library part has the GUID given as parentId.");
    }

    if (!parentName.IsEmpty ()) {
        API_LibPart libPart = {};
        CopyToUCharBuffer (parentName, libPart.docu_UName);
        const GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, false);
        delete libPart.location;
        if (err != NoError) {
            return CreateErrorResponse (err, GS::UniString::Printf ("Not found library part with name '%T' for parentName.", parentName.ToPrintf ()));
        }
        CHTruncate (libPart.ownUnID, newLibPart.parentUnID, sizeof (newLibPart.parentUnID));
        newLibPart.typeID = libPart.typeID;
        return {};
    }

    return CreateErrorResponse (APIERR_BADPARS, "Either parentId or parentName is required.");
}

// The folder of the new library part: the embedded library, or the given local library
// folder (added to the loaded libraries when it is not one of them yet), plus the given
// relative folder, created when missing.
GS::Optional<GS::ObjectState> GetTargetFolder (const GS::ObjectState& parameters, IO::Location& targetFolder)
{
    GS::UniString libraryFolderPath;
    if (parameters.Get ("libraryFolder", libraryFolderPath) && !libraryFolderPath.IsEmpty ()) {
        targetFolder = IO::Location (libraryFolderPath);
        if (IO::Folder (targetFolder).GetStatus () != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "The libraryFolder is not an existing folder.");
        }

        GS::Array<API_LibraryInfo> libraries;
        if (ACAPI_LibraryManagement_GetLibraries (&libraries) != NoError) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to read the loaded libraries.");
        }
        bool isLoaded = false;
        for (const API_LibraryInfo& library : libraries) {
            if (library.location == targetFolder) {
                isLoaded = true;
                break;
            }
        }
        if (!isLoaded) {
            IO::Location newLibrary = targetFolder;
            const GSErrCode err = AddLibraryFolder (&newLibrary);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to add the libraryFolder to the loaded libraries.");
            }
        }
    } else {
        auto folderId = API_SpecFolderID::API_EmbeddedProjectLibraryFolderID;
        if (ACAPI_ProjectSettings_GetSpecFolder (&folderId, &targetFolder) != NoError || IO::Folder (targetFolder).GetStatus () != NoError) {
            return CreateErrorResponse (APIERR_GENERAL, "Failed to get the embedded library folder.");
        }
    }

    GS::UniString folderPath;
    if (parameters.Get ("folder", folderPath) && !folderPath.IsEmpty ()) {
        folderPath.ReplaceAll ("\\", "/");
        GS::Array<GS::UniString> folderNames;
        folderPath.Split ("/", GS::UniString::SkipEmptyParts, &folderNames);
        if (folderPath.Contains (":")) {
            return CreateErrorResponse (APIERR_BADPARS, "The folder must be a relative path.");
        }
        for (const GS::UniString& folderName : folderNames) {
            if (folderName == "." || folderName == "..") {
                return CreateErrorResponse (APIERR_BADPARS, "The folder must not contain '.' or '..' parts.");
            }
            targetFolder.AppendToLocal (IO::Name (folderName));
        }
        if (IO::fileSystem.CreateFolderTree (targetFolder) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "Failed to create the folder.");
        }
    }

    return {};
}

// The element types ACAPI_Element_Create can put into a 2D symbol: the DevKit allows only
// 2D drawing elements there.
bool IsSymbolDrawingElementType (API_ElemTypeID typeId)
{
    switch (typeId) {
        case API_LineID:
        case API_PolyLineID:
        case API_ArcID:
        case API_CircleID:
        case API_SplineID:
        case API_HatchID:
        case API_TextID:
        case API_HotspotID:
            return true;
        default:
            return false;
    }
}

struct SymbolElement {
    API_Element     element;
    API_ElementMemo memo;
};

// Writes a text section of the library part under creation. The DevKit's LibPart_Test
// example streams it with NewSection, WriteSection and EndSection; WriteSection takes
// bytes, and the DevKit does not say how text other than ASCII is encoded in them, so
// such text is added as a Unicode string with AddSection instead.
GSErrCode WriteTextSection (LibPartSectionTypeId sectType, const GS::UniString& text)
{
    API_LibPartSection section = {};
    section.sectType = sectType;

    const auto utf8 = text.ToCStr (CC_UTF8);
    const Int32 byteCount = static_cast<Int32> (strlen (utf8.Get ()));
    if (static_cast<USize> (byteCount) != text.GetLength ()) {
        GS::UniString sectionText = text;
        return ACAPI_LibraryPart_AddSection (&section, nullptr, &sectionText);
    }

    GSErrCode err = NewLibPartSection (&section);
    if (err != NoError) {
        return err;
    }
    err = WriteLibPartSection (byteCount, utf8.Get ());
    const GSErrCode endErr = EndLibPartSection ();
    return err != NoError ? err : endErr;
}

void AddBoundingBox3D (GS::ObjectState& os, const Box3D& box)
{
    GS::ObjectState boundingBox;
    boundingBox.Add ("xMin", box.xMin);
    boundingBox.Add ("yMin", box.yMin);
    boundingBox.Add ("zMin", box.zMin);
    boundingBox.Add ("xMax", box.xMax);
    boundingBox.Add ("yMax", box.yMax);
    boundingBox.Add ("zMax", box.zMax);
    os.Add ("boundingBox3D", boundingBox);
}

}

// ---------------------------------------------------------------------------
// CreateLibraryPartFromElements
// ---------------------------------------------------------------------------

CreateLibraryPartFromElementsCommand::CreateLibraryPartFromElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String CreateLibraryPartFromElementsCommand::GetName () const
{
    return "CreateLibraryPartFromElements";
}

GS::Optional<GS::UniString> CreateLibraryPartFromElementsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the new library part, as shown in the library and used by CreateObjects as libraryPartName.",
                "minLength": 1
            },
            "elements": {
                "type": "array",
                "description": "The 2D drawing elements (lines, polylines, arcs, circles, splines, fills, texts and hotspots) whose copies make up the 2D symbol of the new library part. They keep their plan coordinates, so the project origin becomes the origin of the symbol. The elements themselves are not changed.",
                "items": {
                    "$ref": "#/ElementIdArrayItem"
                },
                "minItems": 1
            },
            "parentId": {
                "type": "string",
                "description": "The parent subtype of the new library part: its unique ID ('{main}-{revision}') or its main GUID, as GetAvailableLibraryParts reports it. Either parentId or parentName is required."
            },
            "parentName": {
                "type": "string",
                "description": "The document name of the parent subtype, e.g. 'General Object' or 'Model Element'; used when parentId is not given."
            },
            "folder": {
                "type": "string",
                "description": "Optional. The folder of the new .gsm file, relative to the embedded library (or to libraryFolder), created when missing. By default the embedded library (or libraryFolder) itself."
            },
            "libraryFolder": {
                "type": "string",
                "description": "Optional. The absolute path of a local library folder to create the library part in instead of the embedded library. A folder that is not one of the loaded libraries yet is added to them first, and stays added."
            },
            "overwrite": {
                "type": "boolean",
                "description": "Optional. When true, a library part file of the same name in the folder is overwritten. By default false: the new library part then gets a unique name, which the response returns."
            },
            "placeable": {
                "type": "boolean",
                "description": "Optional. Whether the new library part can be placed. True by default."
            },
            "scripts": {
                "type": "object",
                "description": "Optional. The GDL scripts of the new library part.",
                "properties": {
                    "master": {
                        "type": "string",
                        "description": "The master script."
                    },
                    "2D": {
                        "type": "string",
                        "description": "The 2D script. When it is empty, the 2D symbol is drawn; a 2D script draws it with FRAGMENT2 ALL, 0."
                    },
                    "3D": {
                        "type": "string",
                        "description": "The 3D script."
                    },
                    "parameter": {
                        "type": "string",
                        "description": "The parameter script."
                    }
                },
                "additionalProperties": false
            },
            "comment": {
                "type": "string",
                "description": "Optional. The comment (description) of the library part."
            },
            "keywords": {
                "type": "string",
                "description": "Optional. The keywords of the library part."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "elements"
        ]
    })";
}

GS::Optional<GS::UniString> CreateLibraryPartFromElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the created library part."
            },
            "fileName": {
                "type": "string",
                "description": "The file name of the created library part, with its extension."
            },
            "unId": {
                "type": "string",
                "description": "The unique ID of the created library part ('{main}-{revision}')."
            },
            "guid": {
                "type": "string",
                "description": "The main GUID of the created library part, as GetAvailableLibraryParts reports it."
            },
            "index": {
                "type": "integer",
                "description": "The index of the created library part in the loaded libraries."
            },
            "a": {
                "type": "number",
                "description": "The default A (X size) of the library part, the width of its 2D symbol."
            },
            "b": {
                "type": "number",
                "description": "The default B (Y size) of the library part, the depth of its 2D symbol."
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "unId",
            "guid",
            "index"
        ]
    })";
}

GS::ObjectState CreateLibraryPartFromElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }
    GS::Array<GS::ObjectState> elements;
    if (!parameters.Get ("elements", elements) || elements.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'elements' parameter.");
    }

    bool overwrite = false;
    parameters.Get ("overwrite", overwrite);
    bool placeable = true;
    parameters.Get ("placeable", placeable);

    API_LibPart libPart = {};
    libPart.typeID = APILib_ObjectID;
    libPart.isTemplate = false;
    libPart.isPlaceable = placeable;
#ifndef ServerMainVers_2800
    // Archicad 28 removed the platform signature from API_LibPart.
    libPart.platformSign = GS::Act_Platform_Sign;
#endif
    CopyToUCharBuffer (name, libPart.docu_UName);

    // Everything the input can get wrong is checked before the 2D symbol is drawn.
    GS::Optional<GS::ObjectState> error = ResolveParentSubtype (parameters, libPart);
    if (error.HasValue ()) {
        return *error;
    }

    // The elements to copy, read before the 2D symbol is opened.
    GS::Array<SymbolElement> symbolElements;
    const GS::OnExit disposeMemos ([&symbolElements] () {
        for (SymbolElement& symbolElement : symbolElements) {
            ACAPI_DisposeElemMemoHdls (&symbolElement.memo);
        }
    });
    for (UIndex i = 0; i < elements.GetSize (); ++i) {
        SymbolElement symbolElement = {};
        symbolElement.element.header.guid = GetGuidFromElementsArrayItem (elements[i]);
        GSErrCode err = ACAPI_Element_Get (&symbolElement.element);
        if (err != NoError) {
            return CreateErrorResponse (err, GS::UniString::Printf ("Failed to get element %u of the elements.", static_cast<unsigned int> (i + 1)));
        }
        const API_ElemTypeID typeId = GetElemTypeId (symbolElement.element.header);
        if (!IsSymbolDrawingElementType (typeId)) {
            return CreateErrorResponse (APIERR_BADELEMENTTYPE,
                GS::UniString::Printf ("Element %u of the elements is of type %T; only lines, polylines, arcs, circles, splines, fills, texts and hotspots can be put into a 2D symbol.",
                                       static_cast<unsigned int> (i + 1), GetElementTypeNonLocalizedName (typeId).ToPrintf ()));
        }
        err = ACAPI_Element_GetMemo (symbolElement.element.header.guid, &symbolElement.memo);
        if (err != NoError) {
            ACAPI_DisposeElemMemoHdls (&symbolElement.memo);
            return CreateErrorResponse (err, GS::UniString::Printf ("Failed to get the memo of element %u of the elements.", static_cast<unsigned int> (i + 1)));
        }
        symbolElements.Push (symbolElement);
    }

    IO::Location targetFolder;
    error = GetTargetFolder (parameters, targetFolder);
    if (error.HasValue ()) {
        return *error;
    }

    GS::UniString masterScript;
    GS::UniString script2D;
    GS::UniString script3D;
    GS::UniString parameterScript;
    if (const GS::ObjectState* scripts = parameters.Get ("scripts")) {
        scripts->Get ("master", masterScript);
        scripts->Get ("2D", script2D);
        scripts->Get ("3D", script3D);
        scripts->Get ("parameter", parameterScript);
    }
    GS::UniString comment;
    GS::UniString keywords;
    parameters.Get ("comment", comment);
    parameters.Get ("keywords", keywords);

    // 1. The 2D symbol: while it is open, ACAPI_Element_Create puts the elements into it
    // instead of the project. It is closed whenever it was opened, even after a failed
    // create, as the DevKit requires.
    GSHandle drawHdl = nullptr;
    const GS::OnExit killDrawHdl ([&drawHdl] () {
        if (drawHdl != nullptr) {
            BMKillHandle (&drawHdl);
        }
    });
    GSErrCode drawErr = NoError;
    GS::UniString drawFailure;
    const GSErrCode undoErr = ACAPI_CallUndoableCommand ("Create Library Part From Elements", [&] () -> GSErrCode {
        drawErr = SetUpSymbolDrawing ();
        if (drawErr != NoError) {
            drawFailure = "Failed to open the 2D symbol of the library part.";
            return NoError;
        }
        for (UIndex i = 0; i < symbolElements.GetSize (); ++i) {
            API_Element element = symbolElements[i].element;
            element.header.guid = APINULLGuid;
            element.header.groupGuid = APINULLGuid;
            element.header.hotlinkGuid = APINULLGuid;
            const GSErrCode createErr = ACAPI_Element_Create (&element, &symbolElements[i].memo);
            if (createErr != NoError && drawErr == NoError) {
                drawErr = createErr;
                drawFailure = GS::UniString::Printf ("Failed to copy element %u of the elements into the 2D symbol.", static_cast<unsigned int> (i + 1));
            }
        }
        const GSErrCode closeErr = CloseSymbolDrawing (&drawHdl);
        if (closeErr != NoError && drawErr == NoError) {
            drawErr = closeErr;
            drawFailure = "Failed to close the 2D symbol of the library part.";
        }
        return NoError;
    });
    if (drawErr != NoError) {
        return CreateErrorResponse (drawErr, drawFailure);
    }
    if (undoErr != NoError) {
        return CreateErrorResponse (undoErr, "Failed to draw the 2D symbol of the library part.");
    }
    if (drawHdl == nullptr) {
        return CreateErrorResponse (APIERR_GENERAL, "The 2D symbol of the library part is empty.");
    }

    // 2. The library part, in the target folder; its file name comes from its name.
    libPart.location = new IO::Location (targetFolder);
    const GS::OnExit deleteLocation ([&libPart] () {
        delete libPart.location;
        libPart.location = nullptr;
    });

    SetLibPartOverwriteMode (overwrite);
    GSErrCode err = ACAPI_LibraryPart_Create (&libPart);
    SetLibPartOverwriteMode (false);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create the library part.");
    }

    // 3. Its sections. From here on the library part is under creation, and only
    // ACAPI_LibraryPart_Save ends that, so it is saved even after a failed section.
    GS::UniString failedStep;
    const struct {
        LibPartSectionTypeId  sectType;
        const GS::UniString*  text;
        bool                  always;
    } textSections[] = {
        { API_SectComText,   &comment,         false },
        { API_SectKeywords,  &keywords,        false },
        { API_Sect1DScript,  &masterScript,    true },
        { API_Sect2DScript,  &script2D,        false },
        { API_Sect3DScript,  &script3D,        false },
        { API_SectVLScript,  &parameterScript, false }
    };
    for (const auto& textSection : textSections) {
        if (!textSection.always && textSection.text->IsEmpty ()) {
            continue;
        }
        err = WriteTextSection (textSection.sectType, *textSection.text);
        if (err != NoError) {
            failedStep = "Failed to write a text section of the library part.";
            break;
        }
    }

    if (err == NoError) {
        API_LibPartSection section = {};
        section.sectType = API_Sect2DDraw;
        err = ACAPI_LibraryPart_AddSection (&section, drawHdl, nullptr);
        if (err != NoError) {
            failedStep = "Failed to write the 2D symbol of the library part.";
        }
    }

    // The parameter section: A and B are calculated from the 2D symbol.
    double a = 1.0;
    double b = 1.0;
    if (err == NoError) {
        GSHandle paramHdl = nullptr;
        err = ACAPI_LibraryPart_GetSect_ParamDef (&libPart, nullptr, &a, &b, drawHdl, &paramHdl);
        if (err == NoError) {
            API_LibPartDetails details = {};
            details.object.fixSize = false;
            details.object.autoHotspot = true;
            err = SetLibPartDetails (&libPart, paramHdl, &details);
        }
        if (err == NoError) {
            API_LibPartSection section = {};
            section.sectType = API_SectParamDef;
            err = ACAPI_LibraryPart_AddSection (&section, paramHdl, nullptr);
        }
        if (paramHdl != nullptr) {
            BMKillHandle (&paramHdl);
        }
        if (err != NoError) {
            failedStep = "Failed to write the parameter section of the library part.";
        }
    }

    // 4. Save, as the DevKit's LibPart_Test example does: the location is deleted before
    // and after it, since Save may give the library part a location of its own.
    delete libPart.location;
    libPart.location = nullptr;
    const GSErrCode saveErr = ACAPI_LibraryPart_Save (&libPart);
    delete libPart.location;
    libPart.location = nullptr;

    if (err != NoError) {
        if (saveErr == NoError) {
            failedStep += " The library part was still saved without that section: create it again with overwrite to replace it.";
        } else {
            failedStep += " Saving the incomplete library part failed too.";
        }
        return CreateErrorResponse (err, failedStep);
    }
    if (saveErr != NoError) {
        return CreateErrorResponse (saveErr, "Failed to save the library part.");
    }

    // The registered library part, by the index Save reports, or by name when it reports none.
    API_LibPart savedLibPart = {};
    savedLibPart.index = libPart.index;
    if (libPart.index > 0) {
        err = ACAPI_LibraryPart_Get (&savedLibPart);
    } else {
        CopyToUCharBuffer (name, savedLibPart.docu_UName);
        err = ACAPI_LibraryPart_Search (&savedLibPart, false, false);
    }
    delete savedLibPart.location;
    savedLibPart.location = nullptr;
    if (err != NoError) {
        return CreateErrorResponse (err, "The library part was saved but could not be found among the loaded library parts.");
    }

    GS::ObjectState response;
    response.Add ("name", GS::UniString (savedLibPart.docu_UName));
    response.Add ("fileName", GS::UniString (savedLibPart.file_UName));
    response.Add ("unId", GS::UniString (savedLibPart.ownUnID));
    response.Add ("guid", GS::UnID (savedLibPart.ownUnID).GetMainGuid ().ToUniString ());
    response.Add ("index", static_cast<Int32> (savedLibPart.index));
    response.Add ("a", a);
    response.Add ("b", b);
    return response;
}

// ---------------------------------------------------------------------------
// InterpretGDLScript3D
// ---------------------------------------------------------------------------

InterpretGDLScript3DCommand::InterpretGDLScript3DCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String InterpretGDLScript3DCommand::GetName () const
{
    return "InterpretGDLScript3D";
}

GS::Optional<GS::UniString> InterpretGDLScript3DCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "script": {
                "type": "string",
                "description": "The GDL 3D script to run. Either script or libraryPartName is required."
            },
            "libraryPartName": {
                "type": "string",
                "description": "A loaded library part, by its fileName (with extension) or documentName as GetAvailableLibraryParts returns them. Its default parameters are used; without script, its master script followed by its 3D script is run."
            }
        },
        "additionalProperties": false
    })";
}

GS::Optional<GS::UniString> InterpretGDLScript3DCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "success": {
                "type": "boolean",
                "description": "Whether the script ran to its end. False when the GDL interpreter stopped with an error; the API gives no message or line number of the GDL error."
            },
            "error": {
                "$ref": "#/Error",
                "description": "The error code the interpreter returned, when success is false."
            },
            "elementCount": {
                "type": "integer",
                "description": "The number of 3D model elements the script generated, when success is true; 0 for a script that generates no 3D model."
            },
            "boundingBox3D": {
                "$ref": "#/BoundingBox3D",
                "description": "The bounding box of the generated 3D model, in the local coordinates of the library part, when it has elements."
            }
        },
        "additionalProperties": false,
        "required": [
            "success"
        ]
    })";
}

GS::ObjectState InterpretGDLScript3DCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString script;
    const bool hasScript = parameters.Get ("script", script);
    GS::UniString libraryPartName;
    parameters.Get ("libraryPartName", libraryPartName);
    if (!hasScript && libraryPartName.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Either script or libraryPartName is required.");
    }

    API_AddParType** addPars = nullptr;
    const GS::OnExit disposeAddPars ([&addPars] () {
        if (addPars != nullptr) {
            ACAPI_DisposeAddParHdl (&addPars);
        }
    });

    if (!libraryPartName.IsEmpty ()) {
        API_LibPart libPart = {};
        if (!FindLoadedLibraryPartByName (libraryPartName, libPart)) {
            return CreateErrorResponse (APIERR_BADPARS, "No loaded library part with the name '" + libraryPartName + "'. Use the fileName or documentName returned by GetAvailableLibraryParts.");
        }

        double a = 0.0;
        double b = 0.0;
        Int32 addParNum = 0;
        const GSErrCode err = ACAPI_LibraryPart_GetParams (libPart.index, &a, &b, &addParNum, &addPars);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to get the parameters of the library part.");
        }

        if (!hasScript) {
            // A library part without one of the sections simply has no such script.
            API_LibPartSection section = {};
            section.sectType = API_Sect1DScript;
            GS::UniString masterScript;
            GetLibPartSection (libPart.index, &section, &masterScript);
            section = {};
            section.sectType = API_Sect3DScript;
            GS::UniString script3D;
            GetLibPartSection (libPart.index, &section, &script3D);
            script = masterScript + "\n" + script3D;
        }
    }

    ModelerAPI::Model model;
    const GSErrCode err = InterpretScript3D (script, addPars, model);

    GS::ObjectState response;
    response.Add ("success", err == NoError);
    if (err != NoError) {
        GS::ObjectState errorObject;
        errorObject.Add ("code", err);
        errorObject.Add ("message", GS::UniString ("The GDL interpreter stopped with an error."));
        response.Add ("error", errorObject);
        return response;
    }

    const Int32 elementCount = model.GetElementCount ();
    response.Add ("elementCount", elementCount);
    if (elementCount > 0) {
        AddBoundingBox3D (response, model.GetBounds ());
    }
    return response;
}

// ---------------------------------------------------------------------------
// GetGDLUserGlobals
// ---------------------------------------------------------------------------

GetGDLUserGlobalsCommand::GetGDLUserGlobalsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetGDLUserGlobalsCommand::GetName () const
{
    return "GetGDLUserGlobals";
}

GS::Optional<GS::UniString> GetGDLUserGlobalsCommand::GetRawResponseSchema () const
{
    return GS::UniString (R"({
        "type": "object",
        "properties": {
            "globals": )") + GDLParameterValuesSchema + R"(
        },
        "additionalProperties": false,
        "required": [
            "globals"
        ]
    })";
}

GS::ObjectState GetGDLUserGlobalsCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
#ifdef ServerMainVers_2800
    GDL::Parameters* userGlobals = nullptr;
    const GSErrCode err = ACAPI_LibraryManagement_CreateCopyOfGdlUserGlobalsHandle (&userGlobals);
    if (err != NoError || userGlobals == nullptr) {
        return CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to get the GDL globals.");
    }

    GS::ObjectState response;
    const auto& globalAdder = response.AddList<GS::ObjectState> ("globals");
    for (const GS::ObjectState& global : GDLParametersToObjectStates (*userGlobals)) {
        globalAdder (global);
    }

    ACAPI_LibraryManagement_DestroyCopyOfGdlUserGlobalsHandle (&userGlobals);
    return response;
#else
    return CreateErrorResponse (APIERR_NOTSUPPORTED, "This command requires Archicad 28 or newer.");
#endif
}
