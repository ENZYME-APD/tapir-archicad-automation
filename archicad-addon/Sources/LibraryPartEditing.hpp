#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2800
#include "ACAPI/Result.hpp"
#include "ACAPI/Library/LibraryManager.hpp"
#include "ACAPI/Library/Library.hpp"
#include "ACAPI/Library/LibPart.hpp"
#include "ACAPI/Library/GSMObject.hpp"

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

// The GSM object of libPart to modify. GetGSMObjectOfLibPart gives the object of the loaded library,
// which Archicad does not always report editable: in a saved project not even for a part of a local
// library folder written by Archicad itself (#2776). GSMObject.hpp says that only GSM objects loaded
// from file can be modified, so when the library's object is not editable and the part is in a local
// folder library (IsLibraryPartInLocalFolderLibrary), the object is loaded from the part's own file
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

    auto location = libPart.GetLocation ();
    if (location.IsErr ()) {
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
