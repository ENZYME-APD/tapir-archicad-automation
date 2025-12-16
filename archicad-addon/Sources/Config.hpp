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
        Repository () = default;
        Repository (
            const GS::UniString& _displayName,
            const GS::UniString& _repoOwner,
            const GS::UniString& _repoName,
            const GS::UniString& _relativeLoc = GS::EmptyUniString,
            const GS::UniString& _token = GS::EmptyUniString,
            const GS::UniString& _excludeFromDownloadPattern = GS::EmptyUniString,
            const GS::UniString& _includePattern = GS::EmptyUniString,
            const GS::UniString& _excludePattern = GS::EmptyUniString)
            : displayName(_displayName)
            , repoOwner(_repoOwner)
            , repoName(_repoName)
            , relativeLoc(_relativeLoc)
            , token(_token)
            , excludeFromDownloadPattern(_excludeFromDownloadPattern)
            , includePattern(_includePattern)
            , excludePattern(_excludePattern)
        {}

        GS::UniString displayName;
        GS::UniString repoOwner;
        GS::UniString repoName;
        GS::UniString relativeLoc;
        GS::UniString token;
        GS::UniString excludeFromDownloadPattern;
        GS::UniString includePattern;
        GS::UniString excludePattern;

        GS::ObjectState ToOS () const;
        static Repository FromOS (const GS::ObjectState& os);
    };

public:
    static Config& Instance ();

    const std::vector<Repository>& Repositories () const { return repositories; }
    bool AskUpdatingAddOnBeforeEachExecution () const { return askUpdatingAddOnBeforeEachExecution; }
    const GS::UniString& uvLocation() const { return uvLocationStr; }

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
    GS::UniString uvLocationStr;
};