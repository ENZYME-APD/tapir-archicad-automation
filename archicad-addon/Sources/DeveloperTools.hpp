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

void GenerateDocumentation (const IO::Location& folder, const std::vector<CommandGroup>& commandGroups);
