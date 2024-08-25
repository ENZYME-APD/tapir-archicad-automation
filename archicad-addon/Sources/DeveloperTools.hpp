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
    CommandInfo (const GS::UniString& version, const GS::UniString& description, const std::shared_ptr<CommandBase>& command);

    GS::UniString version;
    GS::UniString description;
    std::shared_ptr<CommandBase> command;
};

class CommandGroup
{
public:
    CommandGroup (const GS::UniString& name);

    GS::UniString name;
    std::vector<CommandInfo> commands;
};

template <typename CommandType>
GSErrCode RegisterCommand (CommandGroup& group, const GS::UniString& version, const GS::UniString& description)
{
    GSErrCode err = ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler (GS::NewOwned<CommandType> ());
    if (err != NoError) {
        return err;
    }
    group.commands.push_back (CommandInfo (version, description, std::make_shared<CommandType> ()));
    return NoError;
}

void GenerateDocumentation (const IO::Location& folder, const std::vector<CommandGroup>& commandGroups);
