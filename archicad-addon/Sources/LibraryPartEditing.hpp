#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/Library.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/GSMObject.hpp"
#include "Location.hpp"

#include <optional>
#include <vector>

// The library parts one call of a Set command changes in a file instead of in the loaded library, and
// what has to follow once its undoable command has ended.
//
// GetGSMObjectOfLibPart gives the object of the loaded library, which Archicad does not always report
// editable, and GSMObject.hpp says that only GSM objects loaded from file can be modified. So when the
// library's object is not editable, GetGSMObjectToModify loads one from a file:
// - A part of a local folder library (in a saved project Archicad reports even a part of a local library
//   folder written by Archicad itself not editable, #2776) is loaded from its own file. Modifying the
//   object writes that file, so a part whose file or folder is read-only is not loaded from it: the
//   library's object is returned, and the caller refuses it as not editable.
// - A part of the embedded library (in a saved project the embedded library is not a folder of files:
//   its Emb_<n> location is an empty file, so Archicad reports none of its parts editable) is copied with
//   GSMObject::CopyTo, keeping its main GUID so the elements placed from it keep it, into a temporary
//   folder, and loaded from the copy. Every change of the same part in one call is made in that copy.
//   Finish puts the copy back over the part with ACAPI_LibraryManagement_CopyFilesIntoLibrary, the way
//   the DevKit's LibraryManagement_Test example puts the objects it makes in a temporary folder into the
//   embedded library. The file in the embedded library is never written directly.
// The loaded library keeps the old part until Finish reloads the libraries.
class LibraryPartFileChanges
{
public:
    // The file an object of GetGSMObjectToModify was loaded from, for Changed; empty for the object of
    // the loaded library.
    using FileIndex = std::optional<size_t>;

    LibraryPartFileChanges () = default;
    LibraryPartFileChanges (const LibraryPartFileChanges&) = delete;
    LibraryPartFileChanges& operator= (const LibraryPartFileChanges&) = delete;

    // Deletes the temporary copies, so declare it before the GSM objects it gives.
    ~LibraryPartFileChanges ();

    // The GSM object of libPart to modify, and in fileIndex the file it was loaded from. When the
    // library's object is not editable and no file can stand in for it, that object is returned, and the
    // caller refuses it as not editable. An error is returned when the temporary copy of an embedded part
    // cannot be made or loaded.
    ACAPI::Result<ACAPI::Library::GSMObject> GetGSMObjectToModify (const ACAPI::Library::LibraryManager& libraryManager, const ACAPI::Library::LibPart& libPart, FileIndex& fileIndex);

    // Records that the object loaded from fileIndex was modified, for the execution result at resultIndex.
    // Does nothing for the object of the loaded library.
    void Changed (const FileIndex& fileIndex, UIndex resultIndex);

    // Call once, after the undoable command. Puts each changed temporary copy back into the embedded
    // library; the execution results of the changes made in a copy that cannot be put back become
    // failures. Then reloads the libraries when a part was changed in a file, so the loaded library reads it.
    void Finish (GS::Array<GS::ObjectState>& executionResults);

    // Finish for a command with a single execution result, the one Changed was given index 0 for.
    void Finish (GS::ObjectState& executionResult);

private:
    struct File {
        IO::Location        partLocation;           // the file of the part in its library
        bool                isTemporaryCopy;        // a copy of a part of the embedded library, else the part's own file
        IO::Location        temporaryCopyLocation;  // the copy, named as the part's file
        std::vector<UIndex> changedResults;         // the execution results of the changes made in this file
    };

    std::optional<size_t> FindFile (const IO::Location& partLocation) const;
    ACAPI::Result<ACAPI::Library::GSMObject> GetTemporaryCopyToModify (const ACAPI::Library::LibraryManager& libraryManager, ACAPI::Library::GSMObject& libraryObject, const IO::Location& partLocation, FileIndex& fileIndex);

    std::vector<File> files;
    std::optional<IO::Location> temporaryFolder;  // made for the first temporary copy, deleted with the object
};
#endif
