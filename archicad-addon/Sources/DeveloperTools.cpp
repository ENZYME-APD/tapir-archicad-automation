#include "DeveloperTools.hpp"

#include "SchemaDefinitions.hpp"

#include "File.hpp"

CommandInfo::CommandInfo (const GS::UniString& name, const GS::UniString& description, const GS::UniString& version, const GS::Optional<GS::UniString>& inputScheme, const GS::Optional<GS::UniString>& outputScheme) :
    name (name),
    description (description),
    version (version),
    inputScheme (inputScheme),
    outputScheme (outputScheme)
{
}

CommandGroup::CommandGroup (const GS::UniString& name) :
    name (name),
    commands ()
{
}

static bool WriteStringToFile (const IO::Location& location, const GS::UniString& content)
{
    IO::File file (location, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::OpenMode::WriteEmptyMode) != NoError) {
        return false;
    }

    std::string contentString (content.ToCStr (CC_UTF8).Get ());
    if (file.WriteBin ((char*) contentString.c_str (), (GS::USize) contentString.size ()) != NoError) {
        return false;
    }

    file.Close ();
    return true;
}

void GenerateDocumentation (const IO::Location& folder, const std::vector<CommandGroup>& commandGroups)
{
    static const GS::UniString NullString ("null");
    IO::Location commonSchemaLocation = folder;
    commonSchemaLocation.AppendToLocal (IO::Name ("common_schema_definitions.js"));
    GS::UniString commonSchemaContent = "var gSchemaDefinitions = " + GetCommonSchemaDefinitions () + ";";
    WriteStringToFile (commonSchemaLocation, commonSchemaContent);

    IO::Location commandDefinitionLocation = folder;
    commandDefinitionLocation.AppendToLocal (IO::Name ("command_definitions.js"));
    GS::UniString commandDefinitionContent = "var gCommands = [";
    for (size_t groupIndex = 0; groupIndex < commandGroups.size (); groupIndex++) {
        const CommandGroup& group = commandGroups[groupIndex];
        GS::UniString groupCommandsContent;
        for (size_t commandIndex = 0; commandIndex < group.commands.size (); commandIndex++) {
            const CommandInfo& command = group.commands[commandIndex];
            groupCommandsContent += GS::UniString::Printf (R"({
                "name": "%T",
                "version": "%T",
                "description": "%T",
                "inputScheme": %T,
                "outputScheme": %T
            })",
                command.name.ToPrintf (),
                command.version.ToPrintf (),
                command.description.ToPrintf (),
                command.inputScheme.HasValue () ? command.inputScheme.Get ().ToPrintf () : NullString.ToPrintf (),
                command.outputScheme.HasValue () ? command.outputScheme.Get ().ToPrintf () : NullString.ToPrintf ()
            );
            if (commandIndex < group.commands.size () - 1) {
                groupCommandsContent += ",";
            }
        }
        commandDefinitionContent += GS::UniString::Printf (R"({
            "name": "%T",
            "commands": [%T]
        })",
            group.name.ToPrintf (),
            groupCommandsContent.ToPrintf ()
        );
        if (groupIndex < commandGroups.size () - 1) {
            commandDefinitionContent += ",";
        }
    }
    commandDefinitionContent += "];";
    WriteStringToFile (commandDefinitionLocation, commandDefinitionContent);
}
