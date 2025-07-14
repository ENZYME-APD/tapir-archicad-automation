#include "Config.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "ObjectStateJSONConversion.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

Config& Config::Instance ()
{
    static Config instance;
    if (IsFileNewer (*instance.configFilePtr, instance.configFileTimestamp)) {
        instance.LoadFromFile (*instance.configFilePtr);
    }
    return instance;
}

Config::Config ()
{
    const IO::Location configFileLoc = GetConfigFileLocation ();
    const bool exist = FileExist (configFileLoc);
    configFilePtr = std::make_unique<IO::File> (configFileLoc, IO::File::OnNotFound::Create);
    if (!exist) {
        GetDefaults ();
        SaveToFile (*configFilePtr);
        configFilePtr->GetModificationTime (&configFileTimestamp);
    }
}

void Config::GetDefaults ()
{
    repositories.push_back ({
        "Tapir",
        "ENZYME-APD",
        "tapir-archicad-automation",
        "builtin-scripts",
        ""
    });
}

void Config::LoadFromFile (IO::File& file)
{
    GS::ObjectState os;
    file.Open (IO::File::OpenMode::ReadMode);
    GSErrCode err = JSON::ConvertToObjectState (file, os);
    file.Close ();
    if (err != NoError || os.IsEmpty ()) {
        ACAPI_WriteReport (
            "Tapir::Config: Failed to load config file: %T",
            false,
            file.GetLocation().ToDisplayText().ToPrintf()
        );
        return;
    }

    GS::Array<GS::ObjectState> repositoriesArray;
    os.Get ("repositories", repositoriesArray);

    repositories.clear ();
    for (auto& repoOS : repositoriesArray) {
        repositories.emplace_back ();
        Config::Repository& repo = repositories.back ();
        repoOS.Get ("displayName", repo.displayName);
        repoOS.Get ("repoOwner", repo.repoOwner);
        repoOS.Get ("repoName", repo.repoName);
        repoOS.Get ("relativeLoc", repo.relativeLoc);
        repoOS.Get ("token", repo.token);
    }
}

void Config::SaveToFile (IO::File& file) const
{
    GS::ObjectState os;
    const auto& repositoriesArray = os.AddList<GS::ObjectState> ("repositories");
    for (auto& repo : repositories) {
        GS::ObjectState repoOS (
            "repoOwner", repo.repoOwner,
            "repoName", repo.repoName,
            "relativeLoc", repo.relativeLoc,
            "token", repo.token);
        if (!repo.displayName.IsEmpty()) repoOS.Add ("displayName", repo.displayName);
        repositoriesArray (repoOS);
    }

    constexpr bool prettyPrint = true;
    file.Open (IO::File::OpenMode::WriteEmptyMode);
    GSErrCode err = JSON::CreateFromObjectState (
        os,
        file,
        prettyPrint
    );
    file.Close ();
    if (err != NoError) {
        ACAPI_WriteReport (
            "Tapir::Config: Failed to save config file: %T",
            false,
            file.GetLocation().ToDisplayText().ToPrintf()
        );
    }
}

bool Config::FileExist (const IO::Location& fileLocation)
{
    bool exists = false;
    return IO::fileSystem.Contains (fileLocation, &exists) == NoError && exists;
}

IO::Location Config::GetConfigFileLocation ()
{
    IO::Location docsFolderLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &docsFolderLoc);
    IO::Location configFileLoc = docsFolderLoc;
    configFileLoc.AppendToLocal (IO::Name ("Tapir"));
    configFileLoc.AppendToLocal (IO::Name ("config.json"));
    return configFileLoc;
}

bool Config::IsFileNewer (const IO::File& file, GSTime& timestamp)
{
    GSTime newTimestamp = 0;
    file.GetModificationTime (&newTimestamp);
    if (newTimestamp <= timestamp) {
        return false;
    }

    timestamp = newTimestamp;
    return true;
}