#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "ObjectState.hpp"

class CommandBase : public API_AddOnCommand
{
public:
    virtual GS::String GetNamespace () const override;
    virtual GS::Optional<GS::UniString> GetSchemaDefinitions () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual API_AddOnCommandExecutionPolicy GetExecutionPolicy () const override;
    virtual void OnResponseValidationFailed (const GS::ObjectState& response) const override;
#ifdef ServerMainVers_2600
    virtual bool IsProcessWindowVisible () const override;
#endif
};

GS::ObjectState CreateErrorResponse (GSErrCode errorCode, const char* errorMessage);
