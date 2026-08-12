#include "DeveloperTools.hpp"

#include "SchemaDefinitions.hpp"

#include "File.hpp"

static std::vector<CommandGroup> gCommandGroups;

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

static bool GenerateDocumentation (const IO::Location& folder, const std::vector<CommandGroup>& commandGroups)
{
    static const GS::UniString NullString ("null");
    IO::Location commonSchemaLocation = folder;
    commonSchemaLocation.AppendToLocal (IO::Name ("common_schema_definitions.js"));
    GS::UniString commonSchemaContent = "var gSchemaDefinitions = " + GetCommonSchemaDefinitions () + ";";
    if (!WriteStringToFile (commonSchemaLocation, commonSchemaContent)) {
        return false;
    }

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
    if (!WriteStringToFile (commandDefinitionLocation, commandDefinitionContent)) {
        return false;
    }

    return true;
}

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

void AddCommandGroup (const CommandGroup& commandGroup)
{
    gCommandGroups.push_back (commandGroup);
}

GenerateDocumentationCommand::GenerateDocumentationCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GenerateDocumentationCommand::GetName () const
{
    return "GenerateDocumentation";
}

GS::Optional<GS::UniString> GenerateDocumentationCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "destinationFolder": {
                "type": "string",
                "description": "Destination folder for the generated documentation files.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "destinationFolder"
        ]
    })";
}

GS::Optional<GS::UniString> GenerateDocumentationCommand::GetResponseSchema () const
{
    return R"({
        "$ref": "#/ExecutionResult"
    })";
}

GS::ObjectState GenerateDocumentationCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString destinationFolder;
    parameters.Get ("destinationFolder", destinationFolder);

    IO::Location destinationFolderLoc (destinationFolder);
    if (!GenerateDocumentation (destinationFolderLoc, gCommandGroups)) {
        return CreateFailedExecutionResult (APIERR_GENERAL, "Failed to generate documentation.");
    }
    return CreateSuccessfulExecutionResult ();
}
