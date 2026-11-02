#pragma once

#include "CommandBase.hpp"

// Script state kept on an element, invisible in the Archicad UI: string values under keys, stored
// as the user data of this add-on (ACAPI_Element_SetUserData / GetUserData) and saved with the
// project. Project-wide script state is kept by the project data commands (ProjectDataCommands).

class SetElementUserDataCommand : public CommandBase
{
public:
    SetElementUserDataCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetElementUserDataCommand : public CommandBase
{
public:
    GetElementUserDataCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
