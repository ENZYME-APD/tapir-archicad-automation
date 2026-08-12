#pragma once

#include "MigrationHelper.hpp"
#include "CommandBase.hpp"

#include "UniString.hpp"
#include "Location.hpp"

#include <vector>
#include <string>

class CommandInfo
{
public:
    CommandInfo (const GS::UniString& name, const GS::UniString& description, const GS::UniString& version, const GS::Optional<GS::UniString>& inputScheme, const GS::Optional<GS::UniString>& outputScheme);

    GS::UniString name;
    GS::UniString description;
    GS::UniString version;
    GS::Optional<GS::UniString> inputScheme;
    GS::Optional<GS::UniString> outputScheme;
};

class CommandGroup
{
public:
    CommandGroup (const GS::UniString& name);

    GS::UniString name;
    std::vector<CommandInfo> commands;
};

class GenerateDocumentationCommand : public CommandBase
{
public:
    GenerateDocumentationCommand ();

    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

void AddCommandGroup (const CommandGroup& commandGroup);
