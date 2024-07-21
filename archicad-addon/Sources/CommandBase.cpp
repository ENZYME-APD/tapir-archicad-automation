#include "CommandBase.hpp"

#include "SchemaDefinitions.hpp"
#include "MigrationHeader.hpp"

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

API_Coord3D Get3DCoordinateFromObjectState (const GS::ObjectState& objectState)
{
    API_Coord3D coordinate = {};
    objectState.Get ("x", coordinate.x);
    objectState.Get ("y", coordinate.y);
    objectState.Get ("z", coordinate.z);
    return coordinate;
}

GS::Array<GS::Pair<short, double>> GetStoryLevels ()
{
    GS::Array<GS::Pair<short, double>> storyLevels;
    API_StoryInfo storyInfo = {};

    GSErrCode err = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);

    if (err == NoError) {
        const short numberOfStories = storyInfo.lastStory - storyInfo.firstStory + 1;
        for (short i = 0; i < numberOfStories; ++i) {
            storyLevels.PushNew ((*storyInfo.data)[i].index, (*storyInfo.data)[i].level);
        }
        BMKillHandle ((GSHandle*) &storyInfo.data);
    }

    return storyLevels;
}

short GetFloorIndexAndOffset (double zPos, const GS::Array<GS::Pair<short, double>>& storyLevels, double& zOffset)
{
    if (storyLevels.IsEmpty ()) {
        zOffset = zPos;
        return 0;
    }

    auto* lastStoryIndexAndLevel = &storyLevels[0];
    for (const auto& storyIndexAndLevel : storyLevels) {
        if (storyIndexAndLevel.second > zPos) {
            break;
        }
        lastStoryIndexAndLevel = &storyIndexAndLevel;
    }

    zOffset = zPos - lastStoryIndexAndLevel->second;
    return lastStoryIndexAndLevel->first;
}