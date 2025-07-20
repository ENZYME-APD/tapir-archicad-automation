#include "Config.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "ObjectStateJSONConversion.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"

Config::Repository Config::Repository::FromOS (const GS::ObjectState& os) {
    Repository repo;
    os.Get ("displayName", repo.displayName);
    os.Get ("repoOwner", repo.repoOwner);
    os.Get ("repoName", repo.repoName);
    os.Get ("relativeLoc", repo.relativeLoc);
    os.Get ("token", repo.token);
    os.Get ("excludeFromDownloadPattern", repo.excludeFromDownloadPattern);
    os.Get ("includePattern", repo.includePattern);
    os.Get ("excludePattern", repo.excludePattern);
    return repo;
}

GS::ObjectState Config::Repository::ToOS () const {
    return {
        "displayName", displayName,
        "repoOwner", repoOwner,
        "repoName", repoName,
        "relativeLoc", relativeLoc,
        "token", token,
        "excludeFromDownloadPattern", excludeFromDownloadPattern,
        "includePattern", includePattern,
        "excludePattern", excludePattern
    };
}

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
        "builtin-scripts"
    });
    askUpdatingAddOnBeforeEachExecution = false;
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
        repositories.push_back (Config::Repository::FromOS (repoOS));
    }

    os.Get ("askUpdatingAddOnBeforeEachExecution", askUpdatingAddOnBeforeEachExecution);
}

void Config::SaveToFile (IO::File& file) const
{
    GS::ObjectState os;
    const auto& repositoriesArray = os.AddList<GS::ObjectState> ("repositories");
    for (auto& repo : repositories) {
        repositoriesArray (repo.ToOS ());
    }

    os.Add ("askUpdatingAddOnBeforeEachExecution", askUpdatingAddOnBeforeEachExecution);

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