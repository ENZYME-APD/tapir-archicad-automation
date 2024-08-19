#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"
#include "MigrationHelper.hpp"

constexpr const char* CommandNamespace = "TapirCommand";

CommandBase::CommandBase (CommonSchema commonSchema) :
    mCommonSchema (commonSchema)
{
}

GS::String CommandBase::GetNamespace () const
{
    return CommandNamespace;
}

API_AddOnCommandExecutionPolicy CommandBase::GetExecutionPolicy () const
{
    return API_AddOnCommandExecutionPolicy::ScheduleForExecutionOnMainThread;
}

void CommandBase::OnResponseValidationFailed (const GS::ObjectState& /*response*/) const
{

}

#ifdef ServerMainVers_2600
bool CommandBase::IsProcessWindowVisible () const
{
    return false;
}
#endif

GS::Optional<GS::UniString> CommandBase::GetSchemaDefinitions () const
{
    if (mCommonSchema == CommonSchema::Used) {
        return GetCommonSchemaDefinitions ();
    } else {
        return {};
    }
}

GS::Optional<GS::UniString> CommandBase::GetInputParametersSchema () const
{
    return {};
}

GS::Optional<GS::UniString> CommandBase::GetResponseSchema () const
{
    return {};
}

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const GS::UniString& errorMessage)
{
    GS::ObjectState error;
    error.Add ("code", errorCode);
    error.Add ("message", errorMessage.ToCStr ().Get ());
    return GS::ObjectState ("error", error);
}

GS::ObjectState CreateFailedExecutionResult (GSErrCode errorCode, const GS::UniString& errorMessage)
{
    return GS::ObjectState (
        "success", false,
        "error", CreateErrorResponse (errorCode, errorMessage));
}

GS::ObjectState CreateSuccessfulExecutionResult ()
{
    return GS::ObjectState (
        "success", true);
}

API_Guid GetGuidFromObjectState (const GS::ObjectState& os)
{
	GS::String guid;
	os.Get ("guid", guid);
	return APIGuidFromString (guid.ToCStr ());
}

API_Coord Get2DCoordinateFromObjectState (const GS::ObjectState& objectState)
{
    API_Coord coordinate = {};
    objectState.Get ("x", coordinate.x);
    objectState.Get ("y", coordinate.y);
    return coordinate;
}

GS::ObjectState Create2DCoordinateObjectState (const API_Coord& c)
{
    return GS::ObjectState ("x", c.x, "y", c.y);
}

API_Coord3D Get3DCoordinateFromObjectState (const GS::ObjectState& objectState)
{
    API_Coord3D coordinate = {};
    objectState.Get ("x", coordinate.x);
    objectState.Get ("y", coordinate.y);
    objectState.Get ("z", coordinate.z);
    return coordinate;
}

Stories GetStories ()
{
    Stories stories;
    API_StoryInfo storyInfo = {};

    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);

    if (err == NoError) {
        const short numberOfStories = storyInfo.lastStory - storyInfo.firstStory + 1;
        for (short i = 0; i < numberOfStories; ++i) {
            stories.PushNew ((*storyInfo.data)[i].index, (*storyInfo.data)[i].level);
        }
        BMKillHandle ((GSHandle*) &storyInfo.data);
    }

    return stories;
}

GS::Pair<short, double> GetFloorIndexAndOffset (const double zPos, const Stories& stories)
{
    if (stories.IsEmpty ()) {
        return { 0, zPos };
    }

    const Story* storyPtr = &stories[0];
    for (const auto& story : stories) {
        if (story.level > zPos) {
            break;
        }
        storyPtr = &story;
    }

    return { storyPtr->index, zPos - storyPtr->level };
}