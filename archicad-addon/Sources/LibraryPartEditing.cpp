#include "LibraryPartEditing.hpp"

#ifdef ServerMainVers_2800
#include "MigrationHelper.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSGuid.hpp"

#include <string>

using ACAPI::Library::GSMObject;
using ACAPI::Library::LibPart;
using ACAPI::Library::LibraryManager;

// Whether libPart is in a library of the local (or network) file system that Archicad loads as a
// plain folder: not the embedded library, not a library package, not a BIMcloud or built-in library.
// The file of such a part is the file itself; the others are copies Archicad manages (the embedded
// library's extracted folder, a package's or a BIMcloud library's cache), never written directly.
static bool IsLibraryPartInLocalFolderLibrary (const LibraryManager& libraryManager, const LibPart& libPart)
{
    auto library = libraryManager.GetLibraryOfLibPart (libPart);
    if (library.IsErr ()) {
        return false;
    }
    auto isLocal = library->IsLocal ();
    auto isEmbedded = library->IsEmbedded ();
    auto isPackage = library->IsPackage ();
    auto isServer = library->IsServer ();
    auto isBuiltIn = library->IsBuiltIn ();
    if (isLocal.IsErr () || isEmbedded.IsErr () || isPackage.IsErr () || isServer.IsErr () || isBuiltIn.IsErr ()) {
        return false;
    }
    return isLocal.Unwrap () && !isEmbedded.Unwrap () && !isPackage.Unwrap () && !isServer.Unwrap () && !isBuiltIn.Unwrap ();
}

static bool IsLibraryPartInEmbeddedLibrary (const LibraryManager& libraryManager, const LibPart& libPart)
{
    auto library = libraryManager.GetLibraryOfLibPart (libPart);
    if (library.IsErr ()) {
        return false;
    }
    auto isEmbedded = library->IsEmbedded ();
    return !isEmbedded.IsErr () && isEmbedded.Unwrap ();
}

// Whether the file at location and its folder can be written. GSMObject::IsEditable is true for any
// object loaded from the file system, and GSMObject::Modify does not report a failed write of the file
// (ModifyScope's FinalizeModification returns nothing), so a read-only file or folder would answer
// success and change nothing. The file is opened for writing without writing anything, and a probe file
// is created in the folder and deleted again.
static bool IsFileAndItsFolderWritable (const IO::Location& location)
{
    {
        IO::File file (location, IO::File::Fail);
        if (file.GetStatus () != NoError || file.Open (IO::File::ReadWriteMode) != NoError) {
            return false;
        }
        file.Close ();
    }

    IO::Location probeLocation = location;
    if (probeLocation.DeleteLastLocalName () != NoError ||
        probeLocation.AppendToLocal (IO::Name (".TapirWriteCheck_" + GS::Guid (GS::Guid::GenerateGuid).ToUniString () + ".tmp")) != NoError) {
        return false;
    }
    bool writable = false;
    {
        IO::File probe (probeLocation, IO::File::Create);
        writable = probe.GetStatus () == NoError && probe.Open (IO::File::WriteEmptyMode) == NoError;
        probe.Close ();
    }
    bool exists = false;
    if (IO::fileSystem.Contains (probeLocation, &exists) == NoError && exists) {
        IO::fileSystem.Delete (probeLocation);
    }
    return writable;
}

static ACAPI::Result<GSMObject> GSMObjectError (GSErrCode errorCode, const std::string& text)
{
    return { ACAPI::Error (errorCode, text), ACAPI_GetToken () };
}

LibraryPartFileChanges::~LibraryPartFileChanges ()
{
    if (temporaryFolder.has_value ()) {
        IO::fileSystem.Delete (*temporaryFolder);
    }
}

std::optional<size_t> LibraryPartFileChanges::FindFile (const IO::Location& partLocation) const
{
    for (size_t index = 0; index < files.size (); ++index) {
        if (files[index].partLocation == partLocation) {
            return index;
        }
    }
    return std::nullopt;
}

ACAPI::Result<GSMObject> LibraryPartFileChanges::GetGSMObjectToModify (const LibraryManager& libraryManager, const LibPart& libPart, FileIndex& fileIndex)
{
    fileIndex.reset ();
    auto gsmObject = libraryManager.GetGSMObjectOfLibPart (libPart);
    if (gsmObject.IsErr () || gsmObject->IsEditable ()) {
        return gsmObject;
    }

    auto location = libPart.GetLocation ();
    if (location.IsErr ()) {
        return gsmObject;
    }
    const IO::Location partLocation = location.Unwrap ();

    if (IsLibraryPartInEmbeddedLibrary (libraryManager, libPart)) {
        return GetTemporaryCopyToModify (libraryManager, gsmObject.Unwrap (), partLocation, fileIndex);
    }

    // A read-only file or folder is refused here: the library's object, which is not editable, is
    // returned, so the caller gives the "not editable" error instead of a success that wrote nothing.
    if (!IsLibraryPartInLocalFolderLibrary (libraryManager, libPart) || !IsFileAndItsFolderWritable (partLocation)) {
        return gsmObject;
    }
    auto fileObject = libraryManager.LoadGSMObjectFromFile (partLocation);
    if (fileObject.IsErr () || !fileObject->IsEditable ()) {
        return gsmObject;
    }
    fileIndex = FindFile (partLocation);
    if (!fileIndex.has_value ()) {
        files.push_back ({ partLocation, false, IO::Location (), {} });
        fileIndex = files.size () - 1;
    }
    return fileObject;
}

// The copy is <temporary folder>/Tapir/LibraryPartCopies/<new GUID>/<number of the copy>/<file name of
// the part>: a folder of its own for each copy, so it keeps the file name it replaces.
ACAPI::Result<GSMObject> LibraryPartFileChanges::GetTemporaryCopyToModify (const LibraryManager& libraryManager, GSMObject& libraryObject, const IO::Location& partLocation, FileIndex& fileIndex)
{
    std::optional<size_t> index = FindFile (partLocation);
    if (!index.has_value ()) {
        if (!temporaryFolder.has_value ()) {
            IO::Location folder;
            GSErrCode err = IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &folder);
            if (err == NoError) {
                GS::Guid guid;
                guid.Generate ();
                folder.AppendToLocal (IO::Name ("Tapir"));
                folder.AppendToLocal (IO::Name ("LibraryPartCopies"));
                folder.AppendToLocal (IO::Name (guid.ToUniString ()));
                err = IO::fileSystem.CreateFolderTree (folder);
            }
            if (err != NoError) {
                return GSMObjectError (err, "Failed to make a temporary folder for a copy of the part of the embedded library.");
            }
            temporaryFolder = folder;
        }

        IO::Location copyLocation = *temporaryFolder;
        copyLocation.AppendToLocal (IO::Name (GS::UniString::Printf ("%u", static_cast<unsigned int> (files.size ()))));
        IO::Name fileName;
        GSErrCode err = partLocation.GetLastLocalName (&fileName);
        if (err == NoError) {
            err = IO::fileSystem.CreateFolder (copyLocation);
        }
        if (err != NoError) {
            return GSMObjectError (err, "Failed to make a temporary folder for a copy of the part of the embedded library.");
        }
        copyLocation.AppendToLocal (fileName);

        ACAPI::Result<void> copyResult = libraryObject.CopyTo (copyLocation, GSMObject::CopyMethod::KeepMainGuid);
        if (copyResult.IsErr ()) {
            return GSMObjectError (copyResult.UnwrapErr ().kind, "Failed to copy the part of the embedded library into a temporary folder: " + copyResult.UnwrapErr ().text);
        }
        files.push_back ({ partLocation, true, copyLocation, {} });
        index = files.size () - 1;
    }

    auto copyObject = libraryManager.LoadGSMObjectFromFile (files[*index].temporaryCopyLocation);
    if (copyObject.IsErr ()) {
        return GSMObjectError (copyObject.UnwrapErr ().kind, "Failed to load the temporary copy of the part of the embedded library: " + copyObject.UnwrapErr ().text);
    }
    if (!copyObject->IsEditable ()) {
        return GSMObjectError (APIERR_GENERAL, "The temporary copy of the part of the embedded library is not editable.");
    }
    fileIndex = index;
    return copyObject;
}

void LibraryPartFileChanges::Changed (const FileIndex& fileIndex, UIndex resultIndex)
{
    if (fileIndex.has_value ()) {
        files[*fileIndex].changedResults.push_back (resultIndex);
    }
}

void LibraryPartFileChanges::Finish (GS::Array<GS::ObjectState>& executionResults)
{
    bool changedAFile = false;
    for (const File& file : files) {
        if (file.changedResults.empty ()) {
            continue;
        }
        if (file.isTemporaryCopy) {
            IO::Location libraryFolder = file.partLocation;
            GS::Array<IO::Location> filesToCopy;
            filesToCopy.Push (file.temporaryCopyLocation);
            const bool overwriteIfExists = true;
            GSErrCode err = libraryFolder.DeleteLastLocalName ();
            if (err == NoError) {
                err = ACAPI_LibraryManagement_CopyFilesIntoLibrary (&libraryFolder, &filesToCopy, &overwriteIfExists);
            }
            if (err != NoError) {
                for (UIndex resultIndex : file.changedResults) {
                    if (resultIndex < executionResults.GetSize ()) {
                        executionResults[resultIndex] = CreateFailedExecutionResult (err, "The library part was changed in a temporary copy, but the copy could not be put back into the embedded library (ACAPI_LibraryManagement_CopyFilesIntoLibrary), so the part is unchanged. In a Teamwork project, reserve the embedded library first.");
                    }
                }
                continue;
            }
        }
        changedAFile = true;
    }

    if (changedAFile) {
        ACAPI_ProjectOperation_ReloadLibraries ();
    }
}

void LibraryPartFileChanges::Finish (GS::ObjectState& executionResult)
{
    GS::Array<GS::ObjectState> executionResults;
    executionResults.Push (executionResult);
    Finish (executionResults);
    executionResult = executionResults[0];
}
#endif
