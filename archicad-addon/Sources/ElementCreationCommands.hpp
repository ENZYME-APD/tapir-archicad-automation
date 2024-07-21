#pragma once

#include "CommandBase.hpp"

class CreateElementsCommandBase : public CommandBase
{
public:
    CreateElementsCommandBase (const GS::String& commandName);
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
protected:
    GS::String commandName;
};

class CreateColumnsCommand : public CreateElementsCommandBase
{
public:
    CreateColumnsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateSlabsCommand : public CreateElementsCommandBase
{
public:
    CreateSlabsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateObjectsCommand : public CreateElementsCommandBase
{
public:
    CreateObjectsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};