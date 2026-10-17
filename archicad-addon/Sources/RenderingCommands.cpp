#include "RenderingCommands.hpp"
#include "ProjectCommands.hpp"
#include "MigrationHelper.hpp"

// The rendering scenes are the entries of the Scene list of the PhotoRendering
// Settings palette (Document > Creative Imaging). ACAPI_Rendering_GetRenderingSceneNames
// lists their names in the order of recent usage, the most recently used scene first
// (so the DevKit documents it); the API has no other call that tells which scene is
// the current one, so GetRenderingScenes reports the first name as the current scene.
static GSErrCode GetRenderingSceneNames (GS::Array<GS::UniString>& sceneNames)
{
    return ACAPI_Rendering_GetRenderingSceneNames (&sceneNames);
}

// The image settings of the current rendering scene (the Size and Resolution fields
// of the PhotoRendering Settings palette, with the background): API_RendImage holds
// the size in pixels and the resolution in dpi as shorts, hence the upper bound of
// the PhotoRender parameters that change them.
static const Int32 MaxImageSettingValue = 32767;

static GSErrCode GetImageSettings (API_RendImage& imageSettings)
{
    return ACAPI_Rendering_GetRenderingSets (&imageSettings, APIRendSet_ImageID);
}

// Not const: the API takes the settings as a void* parameter.
static GSErrCode ChangeImageSettings (API_RendImage& imageSettings)
{
    return ACAPI_Rendering_ChangeRenderingSets (&imageSettings, APIRendSet_ImageID);
}

// Reads one of the optional image setting parameters of PhotoRender. False, with the
// message to answer, when the parameter is given but is not a positive integer that
// fits the settings.
static bool GetImageSettingParameter (const GS::ObjectState& parameters, const char* name, GS::Optional<Int32>& value, GS::UniString& errorMessage)
{
    Int32 givenValue = 0;
    if (!parameters.Get (name, givenValue)) {
        return true;
    }
    if (givenValue < 1 || givenValue > MaxImageSettingValue) {
        errorMessage = GS::UniString::Printf ("%s must be between 1 and %d.", name, MaxImageSettingValue);
        return false;
    }
    value = givenValue;
    return true;
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
                "description": "The names of the rendering scenes of the project (the Scene list of the PhotoRendering Settings palette), in the order of recent usage: the most recently used scene, the current one, first.",
                "items": {
                    "type": "string"
                }
            },
            "currentSceneName": {
                "type": "string",
                "description": "The name of the current rendering scene, the one selected in the PhotoRendering Settings palette: the first of sceneNames. Left out when the project has no rendering scene."
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
    if (!sceneNames.IsEmpty ()) {
        response.Add ("currentSceneName", sceneNames.GetFirst ());
    }
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
            },
            "width": {
                "type": "integer",
                "description": "Optional. The width of the image in pixels (the Size field of the PhotoRendering Settings palette). Omitted, the width of the scene is used. The image settings of the scene are changed for this render only and restored afterwards.",
                "minimum": 1,
                "maximum": 32767
            },
            "height": {
                "type": "integer",
                "description": "Optional. The height of the image in pixels (the Size field of the PhotoRendering Settings palette). Omitted, the height of the scene is used. The image settings of the scene are changed for this render only and restored afterwards.",
                "minimum": 1,
                "maximum": 32767
            },
            "resolution": {
                "type": "integer",
                "description": "Optional. The resolution of the image in dpi (the Resolution field of the PhotoRendering Settings palette). Omitted, the resolution of the scene is used. The image settings of the scene are changed for this render only and restored afterwards.",
                "minimum": 1,
                "maximum": 32767
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

    GS::Optional<Int32> width;
    GS::Optional<Int32> height;
    GS::Optional<Int32> resolution;
    GS::UniString errorMessage;
    if (!GetImageSettingParameter (parameters, "width", width, errorMessage) ||
        !GetImageSettingParameter (parameters, "height", height, errorMessage) ||
        !GetImageSettingParameter (parameters, "resolution", resolution, errorMessage)) {
        return CreateFailedExecutionResult (APIERR_BADPARS, errorMessage);
    }
    const bool changeImageSettings = width.HasValue () || height.HasValue () || resolution.HasValue ();

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

    // The size, quality and background of the image come from the current scene, as
    // for PhotoRender Projection. A size or resolution given here is written into the
    // image settings of the scene for the render, after the scene is made current, and
    // the previous settings are written back afterwards, so the scene is not changed
    // for good; a field left out keeps the value of the scene. API_RendImage is copied
    // whole, so the background and every other setting go back unchanged too.
    API_RendImage originalImageSettings = {};
    if (changeImageSettings) {
        const GSErrCode getErr = GetImageSettings (originalImageSettings);
        if (getErr != NoError) {
            return CreateFailedExecutionResult (getErr, "Failed to get the image settings of the current rendering scene.");
        }
        API_RendImage imageSettings = originalImageSettings;
        if (width.HasValue ()) {
            imageSettings.hSize = static_cast<short> (width.Get ());
        }
        if (height.HasValue ()) {
            imageSettings.vSize = static_cast<short> (height.Get ());
        }
        if (resolution.HasValue ()) {
            imageSettings.outResolution = static_cast<short> (resolution.Get ());
        }
        const GSErrCode changeErr = ChangeImageSettings (imageSettings);
        if (changeErr != NoError) {
            return CreateFailedExecutionResult (changeErr, "Failed to set the image size and resolution of the current rendering scene.");
        }
    }

    // The API renders the 3D model as the 3D window shows it, so the view to render
    // has to be set up in the 3D window first. API_PhotoRenderPars takes the output
    // file by pointer, like API_FileSavePars of SaveWindowAsImage.
    IO::Location imageFileLocation (imageFilePath);
    API_PhotoRenderPars photoRenderPars = {};
    photoRenderPars.fileTypeID = fileTypeID;
    photoRenderPars.file = &imageFileLocation;

    // A print parameter would print the image too; nullptr writes only the file.
    const GSErrCode err = ACAPI_Rendering_PhotoRender (&photoRenderPars, nullptr);

    // Restored whether or not the render succeeded.
    const GSErrCode restoreErr = changeImageSettings ? ChangeImageSettings (originalImageSettings) : NoError;

    const GS::UniString notRestoredMessage = "the image settings of the rendering scene could not be restored: the scene keeps the given size and resolution.";
    if (err != NoError) {
        GS::UniString message = "Failed to render the 3D model: no project is open, the 3D model could not be generated (open the 3D window first), or the file cannot be written.";
        if (restoreErr != NoError) {
            message += " Also " + notRestoredMessage;
        }
        return CreateFailedExecutionResult (err, message);
    }
    if (restoreErr != NoError) {
        return CreateFailedExecutionResult (restoreErr, "The image was rendered and written, but " + notRestoredMessage);
    }
    return CreateSuccessfulExecutionResult ();
}
