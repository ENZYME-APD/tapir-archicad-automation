#include "RenderingCommands.hpp"
#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"

// The rendering scenes are the entries of the Scene list of the PhotoRendering
// Settings palette (Document > Creative Imaging). ACAPI_Rendering_GetRenderingSceneNames
// lists their names only: the API has no call that tells which scene is the current
// one, so GetRenderingScenes cannot report it either.
static GSErrCode GetRenderingSceneNames (GS::Array<GS::UniString>& sceneNames)
{
    return ACAPI_Rendering_GetRenderingSceneNames (&sceneNames);
}

// Checks the name against the scene list first, so a typo gets a clear message
// instead of whatever Archicad answers to an unknown scene; then makes it current.
// The change is the same as selecting the scene in the palette: it stays current
// after the command, and is saved with the project.
static GS::Optional<GS::ObjectState> SetCurrentRenderingScene (const GS::UniString& sceneName)
{
    GS::Array<GS::UniString> sceneNames;
    const GSErrCode listErr = GetRenderingSceneNames (sceneNames);
    if (listErr != NoError) {
        return CreateErrorResponse (listErr, "Failed to get the rendering scenes.");
    }
    if (!sceneNames.Contains (sceneName)) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("There is no rendering scene named '%T'. GetRenderingScenes lists the names.", sceneName.ToPrintf ()));
    }

    // Not const: the pre-27 ACAPI_Environment form takes the name as a void* parameter.
    GS::UniString newCurrentSceneName = sceneName;
    const GSErrCode err = ACAPI_Rendering_SetCurrentRenderingScene (&newCurrentSceneName);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to set the current rendering scene.");
    }
    return GS::NoValue;
}

GetRenderingScenesCommand::GetRenderingScenesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetRenderingScenesCommand::GetName () const
{
    return "GetRenderingScenes";
}

GS::Optional<GS::UniString> GetRenderingScenesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "sceneNames": {
                "type": "array",
                "description": "The names of the rendering scenes of the project, in the order of the Scene list of the PhotoRendering Settings palette. The API does not tell which one is current.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "sceneNames"
        ]
    })";
}

GS::ObjectState GetRenderingScenesCommand::Execute (const GS::ObjectState& /*parameters*/, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::UniString> sceneNames;
    const GSErrCode err = GetRenderingSceneNames (sceneNames);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the rendering scenes: no project is open.");
    }

    GS::ObjectState response;
    const auto& sceneNamesAdder = response.AddList<GS::UniString> ("sceneNames");
    for (const GS::UniString& sceneName : sceneNames) {
        sceneNamesAdder (sceneName);
    }
    return response;
}

SetCurrentRenderingSceneCommand::SetCurrentRenderingSceneCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String SetCurrentRenderingSceneCommand::GetName () const
{
    return "SetCurrentRenderingScene";
}

GS::Optional<GS::UniString> SetCurrentRenderingSceneCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "sceneName": {
                "type": "string",
                "description": "The name of the rendering scene to make current, one of the names GetRenderingScenes returns."
            }
        },
        "additionalProperties": false,
        "required": [
            "sceneName"
        ]
    })";
}

GS::Optional<GS::UniString> SetCurrentRenderingSceneCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState SetCurrentRenderingSceneCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString sceneName;
    if (!parameters.Get ("sceneName", sceneName) || sceneName.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "sceneName is missing.");
    }

    const GS::Optional<GS::ObjectState> error = SetCurrentRenderingScene (sceneName);
    if (error.HasValue ()) {
        return CreateFailedExecutionResult (error.Get ());
    }
    return CreateSuccessfulExecutionResult ();
}

PhotoRenderCommand::PhotoRenderCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String PhotoRenderCommand::GetName () const
{
    return "PhotoRender";
}

GS::Optional<GS::UniString> PhotoRenderCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "imageFilePath": {
                "type": "string",
                "description": "Absolute path of the image file to write. An existing file is overwritten."
            },
            "format": {
                "type": "string",
                "description": "Optional. The image file format; omitted, it comes from the extension of imageFilePath (.png, .jpg or .jpeg, .bmp, .tif or .tiff, .gif).",
                "enum": ["png", "jpg", "bmp", "tiff", "gif"]
            },
            "sceneName": {
                "type": "string",
                "description": "Optional. The rendering scene to render with, one of the names GetRenderingScenes returns. It is made the current scene first, as SetCurrentRenderingScene does, and stays current afterwards. Omitted, the current scene is used."
            }
        },
        "additionalProperties": false,
        "required": [
            "imageFilePath"
        ]
    })";
}

GS::Optional<GS::UniString> PhotoRenderCommand::GetRawResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState PhotoRenderCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString imageFilePath;
    if (!parameters.Get ("imageFilePath", imageFilePath) || imageFilePath.IsEmpty ()) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "imageFilePath is missing.");
    }

    API_FTypeID fileTypeID = APIFType_PNGFile;
    GS::UniString format;
    if (parameters.Get ("format", format)) {
        if (!ParseImageFileType (format, fileTypeID)) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "format must be one of png, jpg, bmp, tiff and gif.");
        }
    } else if (!ParseImageFileTypeFromExtension (imageFilePath, fileTypeID)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, "format is missing and the extension of imageFilePath is not one of .png, .jpg, .jpeg, .bmp, .tif, .tiff and .gif.");
    }

    GS::UniString sceneName;
    if (parameters.Get ("sceneName", sceneName)) {
        if (sceneName.IsEmpty ()) {
            return CreateFailedExecutionResult (APIERR_BADPARS, "sceneName is empty.");
        }
        const GS::Optional<GS::ObjectState> error = SetCurrentRenderingScene (sceneName);
        if (error.HasValue ()) {
            return CreateFailedExecutionResult (error.Get ());
        }
    }

    // The size, quality and background of the image come from the current scene,
    // as for PhotoRender Projection; the API renders the 3D model as the 3D window
    // shows it, so the view to render has to be set up in the 3D window first.
    API_PhotoRenderPars photoRenderPars = {};
    photoRenderPars.fileLoc = IO::Location (imageFilePath);
    photoRenderPars.fileTypeID = fileTypeID;

    // A print parameter would print the image too; nullptr writes only the file.
    const GSErrCode err = ACAPI_Rendering_PhotoRender (&photoRenderPars, nullptr);
    if (err != NoError) {
        return CreateFailedExecutionResult (err, "Failed to render the 3D model: no project is open, the 3D model could not be generated (open the 3D window first), or the file cannot be written.");
    }
    return CreateSuccessfulExecutionResult ();
}
