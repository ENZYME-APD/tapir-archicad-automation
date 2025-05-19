#include "TapirPalette.hpp"
#include "ResourceIds.hpp"
#include "HTTP/Client/ClientConnection.hpp"
#include "HTTP/Client/Request.hpp"
#include "HTTP/Client/Response.hpp"
#include "JSON/Value.hpp"
#include "JSON/JDOMParser.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "IOBinProtocolXs.hpp"
#include "IChannelX.hpp"
#include "StringConversion.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "MessageLoopExecutor.hpp"
#include "MigrationHelper.hpp"

#include <map>

const GS::Guid        TapirPalette::paletteGuid("{2D42DF37-222F-40CD-BA86-B3279CCA1FEE}");
GS::Ref<TapirPalette> TapirPalette::instance;

static UShort GetConnectionPort ()
{
    UShort portNumber;
    ACAPI_Command_GetHttpConnectionPort (&portNumber);
    return portNumber;
}

static bool IsValidLocation (const IO::Location& location)
{
    if (location.IsEmpty ()) {
        return false;
    }

    bool exists = false;
    GSErrCode err = IO::fileSystem.Contains (location, &exists);
    return err == NoError && exists;
}

static IO::Location SaveBuiltInScript (const IO::RelativeLocation& relLoc, const GS::UniString& content)
{
    IO::Location fileLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &fileLoc);
    fileLoc.AppendToLocal (IO::Name ("Tapir"));
    fileLoc.AppendToLocal (relLoc);

    IO::Location folderToCreate = fileLoc;
    folderToCreate.DeleteLastLocalName ();

    IO::fileSystem.CreateFolderTree (folderToCreate);

    IO::File file (fileLoc, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::WriteEmptyMode) != NoError) {
        return {};
    }

    auto cStr = content.ToCStr ();
    file.WriteBin (cStr.Get (), (GS::USize)strlen (cStr.Get()));

    return fileLoc;
}

static GS::UniString DownloadFile (const GS::UniString& fileDownloadUrl)
{
	IO::URI::URI connectionUrl (fileDownloadUrl);
	HTTP::Client::ClientConnection clientConnection (connectionUrl);
	clientConnection.Connect ();

	HTTP::Client::Request getRequest (HTTP::MessageHeader::Method::Get, "");

	getRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
	clientConnection.Send (getRequest);

	HTTP::Client::Response response;
	GS::IChannelX channel (clientConnection.BeginReceive (response), GS::GetNetworkByteOrderIProtocolX ());

	GS::UniString body = GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (channel, GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);

	clientConnection.FinishReceive ();
	clientConnection.Close (false);

    return body;
}

static std::map<GS::UniString, GS::UniString> GetFilesFromGitHubInRelativeLocation (const GS::UniString& relLoc = {})
{
    std::map<GS::UniString, GS::UniString> files;

    try {
        IO::URI::URI connectionUrl ("https://api.github.com");
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.Connect ();

        HTTP::Client::Request request (HTTP::MessageHeader::Method::Get, "/repos/ENZYME-APD/tapir-archicad-automation/contents/" + relLoc);
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36");
        clientConnection.Send (request);

        HTTP::Client::Response response;
        JSON::JDOMParser parser;
        JSON::ValueRef parsed = parser.Parse (clientConnection.BeginReceive (response));

        if (response.GetStatusCode () == HTTP::MessageHeader::StatusCode::OK) {
            JSON::ArrayValueRef arrayValue = GS::DynamicCast<JSON::ArrayValue> (parsed);
            arrayValue->Enumerate ([&] (const JSON::ValueRef& assetValue) {
                JSON::ObjectValueRef objectValue = GS::DynamicCast<JSON::ObjectValue> (assetValue);
                JSON::StringValueRef typeValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("type"));
                JSON::StringValueRef pathValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("path"));

                if (typeValue->Get () == "dir") {
                    auto subFiles = GetFilesFromGitHubInRelativeLocation (pathValue->Get ());
                    files.insert (subFiles.begin (), subFiles.end ());
                } else {
                    JSON::StringValueRef downloadUrlValue = GS::DynamicCast<JSON::StringValue> (objectValue->Get ("download_url"));
                    files[pathValue->Get ()] = downloadUrlValue->Get ();
                }
            });
        }

        clientConnection.Close (false);
    } catch (...) {
    }

    return files;
}

TapirPalette::TapirPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , tapirButton (GetReference (), 1)
    , scriptSelectionPopUp (GetReference (), 2)
    , runScriptButton (GetReference (), 3)
    , openScriptButton (GetReference (), 4)
{
    Attach (*this);
    AttachToAllItems (*this);

    LoadScriptsToPopUp ();

    EnableIdleEvent (true);
    BeginEventProcessing ();
}

TapirPalette::~TapirPalette ()
{
    DetachFromAllItems (*this);
    EndEventProcessing ();
}

bool TapirPalette::HasInstance ()
{
    return instance != nullptr;
}

TapirPalette& TapirPalette::Instance ()
{
    if (!HasInstance ()) {
        instance = new TapirPalette ();
    }
    return *instance;
}

void TapirPalette::Show ()
{
    DG::Palette::Show ();
    SetMenuItemCheckedState (true);
}

void TapirPalette::Hide ()
{
    DG::Palette::Hide ();
    SetMenuItemCheckedState (false);
}

void TapirPalette::SetMenuItemCheckedState (bool isChecked)
{
    API_MenuItemRef    itemRef = {};
    GSFlags            itemFlags = {};

    itemRef.menuResID = ID_ADDON_MENU_FOR_PALETTE;
    itemRef.itemIndex = ID_ADDON_MENU_PALETTE;

    ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
    if (isChecked)
        itemFlags |= API_MenuItemChecked;
    else
        itemFlags &= ~API_MenuItemChecked;
    ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}

void TapirPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
}

void TapirPalette::PanelIdle (const DG::PanelIdleEvent&)
{
    if (runScriptButton.GetIcon ().GetResourceId () == ID_RUN_BUTTON_ICON) {
        return;
    }

    if (process.IsTerminated ()) {
        auto stdError = GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (process.GetStandardErrorChannel (), GS::GetBinIProtocolX (), GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);
        if (!stdError.IsEmpty ()) {
            WriteReport (DG_ERROR, stdError);
        }

        auto stdOutput = GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (process.GetStandardOutputChannel (), GS::GetBinIProtocolX (), GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);
        WriteReport (DG_INFORMATION, stdOutput);

        runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), ID_RUN_BUTTON_ICON));
    }
}

static void OpenWebpage (const GS::UniString& webpage)
{
    const GS::UniString command = GS::UniString::Printf (
        "%s %T",
#ifdef WINDOWS
        "start",
#else
        "open",
#endif
        webpage.ToPrintf ());
    system (command.ToCStr ().Get ());
}

void TapirPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &runScriptButton) {
        if (runScriptButton.GetIcon ().GetResourceId () == ID_RUN_BUTTON_ICON) {
            GS::Ref<IO::Location> fileRef = GS::DynamicCast<IO::Location> (scriptSelectionPopUp.GetItemObjectData (scriptSelectionPopUp.GetSelectedItem ()));
            if (fileRef != nullptr) {
                ExecuteScript (*fileRef);
            }
        } else {
            process.Kill ();
        }
    } else if (ev.GetSource () == &tapirButton) {
        OpenWebpage ("https://github.com/ENZYME-APD/tapir-archicad-automation");
    } else if (ev.GetSource () == &openScriptButton) {
        OpenWebpage ("https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/builtin-scripts"));
    }
}

bool TapirPalette::IsPopUpContainsFile (const IO::Location& fileLocation) const
{
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }

        GS::Ref<IO::Location> fileRef = GS::DynamicCast<IO::Location> (scriptSelectionPopUp.GetItemObjectData (i));
        if (fileRef != nullptr && *fileRef == fileLocation) {
            return true;
        }
    }
    return false;
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    const short selectedItem = scriptSelectionPopUp.GetSelectedItem ();

    if (selectedItem == scriptSelectionPopUp.GetItemCount ()) {
        if (AddNewScript ()) {
            scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
            SaveScriptsToPreferences ();
        } else {
            scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
        }
    } else if (selectedItem == scriptSelectionPopUp.GetItemCount () - 1) {
        LoadScriptsToPopUp ();

        scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
    } else {
        size_t countOfSeparatorsAfterSelection = 0;
        for (short i = selectedItem; i <= scriptSelectionPopUp.GetItemCount (); ++i) {
            if (scriptSelectionPopUp.IsSeparator (i)) {
                countOfSeparatorsAfterSelection++;
            }
        }
        const bool isSelectedScriptFromGitHub = countOfSeparatorsAfterSelection == 1;
        if (isSelectedScriptFromGitHub) {
            openScriptButton.Enable ();
        } else {
            openScriptButton.Disable ();
        }
    }
}

GSErrCode TapirPalette::PaletteControlCallBack (Int32, API_PaletteMessageID messageID, GS::IntPtr param)
{
    switch (messageID) {
        case APIPalMsg_OpenPalette:
            Instance ().Show ();
            break;

        case APIPalMsg_ClosePalette:
            if (!HasInstance ())
                break;
            Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_Begin:
            if (HasInstance () && Instance ().IsVisible ())
                Instance ().Hide ();
            break;

        case APIPalMsg_HidePalette_End:
            if (HasInstance () && !Instance ().IsVisible ())
                Instance ().Show ();
            break;

        case APIPalMsg_IsPaletteVisible:
            *(reinterpret_cast<bool*> (param)) = HasInstance () && Instance ().IsVisible ();
            break;

        default:
            break;
    }

    return NoError;
}

GSErrCode TapirPalette::RegisterPaletteControlCallBack ()
{
    return ACAPI_RegisterModelessWindow (
                    GS::CalculateHashValue (paletteGuid),
                    PaletteControlCallBack,
                    API_PalEnabled_FloorPlan + API_PalEnabled_Section + API_PalEnabled_Elevation +
                    API_PalEnabled_InteriorElevation + API_PalEnabled_3D + API_PalEnabled_Detail +
                    API_PalEnabled_Worksheet + API_PalEnabled_Layout + API_PalEnabled_DocumentFrom3D,
                    GSGuid2APIGuid (paletteGuid));
}

void TapirPalette::ExecuteScript (const IO::Location& fileLocation)
{
    GS::UniString filePath;
    fileLocation.ToPath (&filePath);

    class PythonExecutionTask : public GS::Runnable
    {
    protected:
        GS::Process& process;
        const GS::UniString filePath;

    public:
        explicit PythonExecutionTask (
            GS::Process& p,
            const GS::UniString filePath)
            : process(p)
            , filePath(filePath)
        {
        }

        void Run () override final
        {
            try {
                TapirPalette::Instance ().WriteReport (DG_INFORMATION, "Executing %T", filePath.ToPrintf ());

                GS::UniString command;
                GS::Array<GS::UniString> argv;
                if (filePath.EndsWith (".py")) {
                    command = "python";
                    argv = {"-X", "utf8=1", filePath, "--port", GS::ValueToUniString (GetConnectionPort ())};
                } else {
                    command = filePath;
                    argv = {"--port", GS::ValueToUniString (GetConnectionPort ())};
                }
                constexpr bool redirectStandardOutput = true;
                constexpr bool redirectStandardInput = false;
                constexpr bool redirectStandardError = true;
                process = GS::Process::Create (command, argv, GS::Process::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);

                TapirPalette::Instance ().runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), ID_KILL_BUTTON_ICON));
            } catch (const GS::ProcessException&) {
                TapirPalette::Instance ().WriteReport (DG_ERROR, "Failed to execute %T", filePath.ToPrintf ());
            } catch (...) {
                TapirPalette::Instance ().WriteReport (DG_ERROR, "Unexpected issue while executing %T", filePath.ToPrintf ());
            }
        }
    };

    GS::MessageLoopExecutor ().Execute (new PythonExecutionTask (process, filePath));
}

bool TapirPalette::AddScriptToPopUp (const IO::Location& fileLocation, short index)
{
    if (!IsValidLocation (fileLocation)) {
        return false;
    }

    IO::Name name;
    fileLocation.GetLastLocalName (&name);
    scriptSelectionPopUp.InsertItem (index);
    scriptSelectionPopUp.SetItemText (index, name.ToString ());
    scriptSelectionPopUp.SetItemObjectData (index, GS::NewRef<IO::Location> (fileLocation));

    return true;
}

void TapirPalette::AddBuiltInScriptsFromGithub ()
{
    for (auto kv : GetFilesFromGitHubInRelativeLocation ("builtin-scripts")) {
        const IO::RelativeLocation relLoc (kv.first);
        const IO::Location fileLoc = SaveBuiltInScript (relLoc, DownloadFile (kv.second));
        if (relLoc.GetLength () == 2) {
            AddScriptToPopUp (fileLoc, DG::PopUp::BottomItem);
        }
    }
}

void TapirPalette::AddScriptsFromCustomScriptsFolder ()
{
    IO::Location docsFolderLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &docsFolderLoc);
    IO::Location customScriptsFolderLoc = docsFolderLoc;
    customScriptsFolderLoc.AppendToLocal (IO::Name ("Tapir"));
    customScriptsFolderLoc.AppendToLocal (IO::Name ("custom-scripts"));

    IO::fileSystem.CreateFolderTree (customScriptsFolderLoc);

    IO::Folder customScriptsFolder (customScriptsFolderLoc);
    customScriptsFolder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
        if (isFolder) {
            return;
        }

        if (!hasCustomScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasCustomScript = true;
        }

        IO::Location file (customScriptsFolderLoc, name);
        AddScriptToPopUp (file);
    });
}

void TapirPalette::LoadScriptsToPopUp ()
{
    scriptSelectionPopUp.DeleteItem (DG_ALL_ITEMS);

    hasCustomScript = false;
    hasAddedScript = false;

    AddScriptsFromCustomScriptsFolder ();
    AddScriptsFromPreferences ();
    AddBuiltInScriptsFromGithub ();

    scriptSelectionPopUp.AppendSeparator ();
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Reload scripts...");
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Add new script...");
}

#define PREFERENCES_VERSION 10

void TapirPalette::SaveScriptsToPreferences ()
{
    GS::UniString preferencesStr;
    if (hasAddedScript) {
        for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1 && !scriptSelectionPopUp.IsSeparator (i); ++i) {
            GS::Ref<IO::Location> fileRef = GS::DynamicCast<IO::Location> (scriptSelectionPopUp.GetItemObjectData (i));
            if (fileRef == nullptr) {
                continue;
            }

            GS::UniString pathStr;
            fileRef->ToPath (&pathStr);
            preferencesStr += pathStr + '\n';
        }
    }
    auto cStr = preferencesStr.ToCStr ();
	ACAPI_SetPreferences (PREFERENCES_VERSION, (GSSize)strlen (cStr.Get()), cStr.Get());
}

void TapirPalette::AddScriptsFromPreferences ()
{
    Int32 version;
    GSSize nBytes;

    ACAPI_GetPreferences (&version, &nBytes, nullptr);
    if (version != PREFERENCES_VERSION || nBytes <= 0) {
        return;
    }

    std::unique_ptr<char> data(new char[nBytes]);
    ACAPI_GetPreferences (&version, &nBytes, data.get ());
    GS::Array<GS::UniString> scriptPathArray;
    GS::UniString (data.get ()).Split ("\n", GS::UniString::SkipEmptyParts, &scriptPathArray);
    for (auto&& scriptPath : scriptPathArray) {
        IO::Location ownScript (scriptPath);
        if (!IsValidLocation (ownScript)) {
            continue;
        }
        if (!hasAddedScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasAddedScript = true;
        }
        AddScriptToPopUp (ownScript);
    }
}

bool TapirPalette::AddNewScript ()
{
    FTM::FileTypeManager	ftMan ("Python Script");
    FTM::GroupID			gid = ftMan.AddGroup ("Python Script");
    ftMan.AddType (FTM::FileType ("Python Script", "py", '    ', '    ', 0), gid);
    DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
    dlg.AddFilter (gid);
    dlg.SetHeader (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_TITLE, ACAPI_GetOwnResModule ()));
    dlg.SetOKButtonText (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_BUTTON, ACAPI_GetOwnResModule ()));
    if (!dlg.Invoke ()) {
        return false;
    }
    bool hasNew = false;
    for (UIndex i = 0; i < dlg.GetSelectionCount (); ++i) {
        IO::Location file = dlg.GetSelectedFile (i);
        if (IsPopUpContainsFile (file)) {
            continue;
        }
        if (!hasAddedScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasAddedScript = true;
        }
        AddScriptToPopUp (file);
        hasNew = true;
    }

    return hasNew;
}