#pragma once

#include "UniString.hpp"
#include "File.hpp"
#include "Location.hpp"

#include <vector>
#include <memory>

class Config
{
public:
    struct Repository {
        GS::UniString displayName;
        GS::UniString repoOwner;
        GS::UniString repoName;
        GS::UniString relativeLoc;
        GS::UniString token;
    };

public:
    static Config& Instance ();

    const std::vector<Repository>& Repositories () const { return repositories; }
    bool AskUpdatingAddOnBeforeEachExecution () const { return askUpdatingAddOnBeforeEachExecution; }

private:
    Config ();
    void GetDefaults ();
    void LoadFromFile (IO::File& file);
    void SaveToFile (IO::File& file) const;
    static bool FileExist (const IO::Location& fileLocation);
    static IO::Location GetConfigFileLocation ();
    static bool IsFileNewer (const IO::File& file, GSTime& timestamp);

private:
    std::unique_ptr<IO::File> configFilePtr;
    GSTime configFileTimestamp = 0;
    std::vector<Repository> repositories;
    bool askUpdatingAddOnBeforeEachExecution;
};