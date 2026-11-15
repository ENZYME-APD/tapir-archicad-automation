#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/Library.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/GSMObject.hpp"
#include "File.hpp"
#include "FileSystem.hpp"
#include "GSGuid.hpp"

// Whether libPart is in a library of the local (or network) file system that Archicad loads as a
// plain folder: not the embedded library, not a library package, not a BIMcloud or built-in library.
// The file of such a part is the file itself; the others are copies Archicad manages (the embedded
// library's extracted folder, a package's or a BIMcloud library's cache), never written directly.
inline bool IsLibraryPartInLocalFolderLibrary (const ACAPI::Library::LibraryManager& libraryManager, const ACAPI::Library::LibPart& libPart)
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

// Whether the file at location and its folder can be written. GSMObject::IsEditable is true for any
// object loaded from the file system, and GSMObject::Modify does not report a failed write of the file
// (ModifyScope's FinalizeModification returns nothing), so a read-only file or folder would answer
// success and change nothing. The file is opened for writing without writing anything, and a probe file
// is created in the folder and deleted again.
inline bool IsFileAndItsFolderWritable (const IO::Location& location)
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

// The GSM object of libPart to modify. GetGSMObjectOfLibPart gives the object of the loaded library,
// which Archicad does not always report editable: in a saved project not even for a part of a local
// library folder written by Archicad itself (#2776). GSMObject.hpp says that only GSM objects loaded
// from file can be modified, so when the library's object is not editable and the part is in a local
// folder library (IsLibraryPartInLocalFolderLibrary) whose file and folder are writable
// (IsFileAndItsFolderWritable), the object is loaded from the part's own file
// (LoadGSMObjectFromFile) and loadedFromFile is set. Modifying that object writes the file, but the
// loaded library keeps the old part until the libraries are reloaded, so the caller then calls
// ACAPI_ProjectOperation_ReloadLibraries, after the GSM objects are released. In every other case
// the library's object is returned as it is, and the caller refuses it when it is not editable.
template <typename LibraryManagerResult>
auto GetGSMObjectToModify (LibraryManagerResult& libraryManager, const ACAPI::Library::LibPart& libPart, bool& loadedFromFile)
{
    loadedFromFile = false;
    auto gsmObject = libraryManager->GetGSMObjectOfLibPart (libPart);
    if (gsmObject.IsErr () || gsmObject->IsEditable () || !IsLibraryPartInLocalFolderLibrary (libraryManager.Unwrap (), libPart)) {
        return gsmObject;
    }

    // A read-only file or folder is refused here: the library's object, which is not editable, is
    // returned, so the caller gives the "not editable" error instead of a success that wrote nothing.
    auto location = libPart.GetLocation ();
    if (location.IsErr () || !IsFileAndItsFolderWritable (location.Unwrap ())) {
        return gsmObject;
    }
    auto fileObject = libraryManager->LoadGSMObjectFromFile (location.Unwrap ());
    if (fileObject.IsErr () || !fileObject->IsEditable ()) {
        return gsmObject;
    }
    loadedFromFile = true;
    return fileObject;
}
#endif
