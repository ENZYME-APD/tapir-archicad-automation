#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "ObjectState.hpp"

enum class CommonSchema
{
    Used,
    NotUsed
};

class CommandBase : public API_AddOnCommand
{
public:
    CommandBase (CommonSchema commonSchema);

    virtual GS::String GetNamespace () const override final;
    virtual API_AddOnCommandExecutionPolicy GetExecutionPolicy () const override final;
    virtual void OnResponseValidationFailed (const GS::ObjectState& response) const override final;
#ifdef ServerMainVers_2600
    virtual bool IsProcessWindowVisible () const override final;
#endif
    virtual GS::Optional<GS::UniString> GetSchemaDefinitions () const override final;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;

private:
    CommonSchema mCommonSchema;
};

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const GS::UniString& errorMessage);

API_Guid    GetGuidFromObjectState (const GS::ObjectState& os);
API_Coord   Get2DCoordinateFromObjectState (const GS::ObjectState& objectState);
API_Coord3D Get3DCoordinateFromObjectState (const GS::ObjectState& objectState);

GS::Array<GS::Pair<short, double>> GetStoryLevels ();
short GetFloorIndexAndOffset (double zPos, const GS::Array<GS::Pair<short, double>>& storyLevels, double& zOffset);