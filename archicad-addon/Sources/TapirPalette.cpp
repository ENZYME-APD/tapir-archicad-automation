#include "TapirPalette.hpp"
#include "ResourceIds.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "IOBinProtocolXs.hpp"
#include "FileSystem.hpp"
#include "Folder.hpp"
#include "MessageLoopExecutor.hpp"
#include "GSProcessControl.hpp"
#include "MigrationHelper.hpp"

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

static IO::Location GetBuiltInScriptLocation (GSResID resId)
{
    GSHandle gsHandle = RSLoadResource ('FILE', ACAPI_GetOwnResModule(), resId);
	API_LibPart	builtInLibpart = {};

    IO::Location tempFileLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &tempFileLoc);

    tempFileLoc.AppendToLocal (IO::Name ("TapirBuiltInScripts"));

    if (resId == ID_ACLIB_INIT_PY_FILE) {
        tempFileLoc.AppendToLocal (IO::Name ("aclib"));

        IO::fileSystem.CreateFolderTree (tempFileLoc);
        tempFileLoc.AppendToLocal (IO::Name ("__init__.py"));
    } else {
        GS::UniString scriptName = RSGetIndString (ID_BUILTINSCRIPTS_NAMES, resId, ACAPI_GetOwnResModule ());
        if (scriptName.IsEmpty ()) {
            return {};
        }

        IO::fileSystem.CreateFolderTree (tempFileLoc);
        tempFileLoc.AppendToLocal (IO::Name (scriptName));
    }

    IO::File file (tempFileLoc, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::WriteEmptyMode) != NoError) {
        return {};
    }

    file.WriteBin (*gsHandle, BMGetHandleSize (gsHandle));

    return tempFileLoc;
}

bool TapirPalette::EnumFILEResBuiltInScriptsCallback (GSResID resID, GSResType /*resType*/, GSResModule /*resModule*/, void* palette)
{
    const IO::Location tmpScriptLocation = GetBuiltInScriptLocation (resID);

    if (resID != ID_ACLIB_INIT_PY_FILE) {
        reinterpret_cast<TapirPalette*>(palette)->AddScriptToPopUp (tmpScriptLocation, DG::PopUp::BottomItem);
    } else {
        reinterpret_cast<TapirPalette*>(palette)->tmpACLibInitPyLoc = tmpScriptLocation;
    }

    return true;
}

TapirPalette::TapirPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , tapirIcon (GetReference (), 1)
    , scriptSelectionPopUp (GetReference (), 2)
    , runScriptButton (GetReference (), 3)
{
    Attach (*this);
    AttachToAllItems (*this);

    AddScriptsFromCustomScriptsFolder ();
    AddBuiltInScripts ();

    scriptSelectionPopUp.AppendSeparator ();
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Reload Custom Scripts...");

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
    }
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    const short selectedItem = scriptSelectionPopUp.GetSelectedItem ();
    if (selectedItem == scriptSelectionPopUp.GetItemCount ()) {
        AddScriptsFromCustomScriptsFolder ();

        scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
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

        case APIPalMsg_DisableItems_Begin:
            if (HasInstance () && Instance ().IsVisible ())
                Instance ().DisableItems ();
            break;

        case APIPalMsg_DisableItems_End:
            if (HasInstance () && Instance ().IsVisible ())
                Instance ().EnableItems ();
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

void TapirPalette::AddBuiltInScripts ()
{
	RSEnumResourceIDs (EnumFILEResBuiltInScriptsCallback, this, 'FILE', ACAPI_GetOwnResModule ());
}

void TapirPalette::AddScriptsFromCustomScriptsFolder ()
{
    if (hasCustomScript) {
        while (!scriptSelectionPopUp.IsSeparator (DG::PopUp::TopItem)) {
            scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        }
        scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        hasCustomScript = false;
    }

    IO::Location customScriptsFolderLoc;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::UserDocuments, &customScriptsFolderLoc);
    customScriptsFolderLoc.AppendToLocal (IO::Name ("TapirCustomScripts"));

    IO::Location acLibInitPyLoc = customScriptsFolderLoc;
    acLibInitPyLoc.AppendToLocal (IO::Name ("aclib"));

    IO::fileSystem.CreateFolderTree (acLibInitPyLoc);
    acLibInitPyLoc.AppendToLocal (IO::Name ("__init__.py"));
    IO::fileSystem.Copy (tmpACLibInitPyLoc, acLibInitPyLoc);

    IO::Folder customScriptsFolder (customScriptsFolderLoc);
    customScriptsFolder.Enumerate ([&] (const IO::Name& name, bool isFolder) {
        if (isFolder) {
            return;
        }

        IO::Location file (customScriptsFolderLoc, name);
        if (IsPopUpContainsFile (file)) {
            return;
        }

        if (!hasCustomScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasCustomScript = true;
        }

        AddScriptToPopUp (file);
    });
}