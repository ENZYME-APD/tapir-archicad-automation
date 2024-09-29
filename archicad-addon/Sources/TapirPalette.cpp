#include "TapirPalette.hpp"
#include "ResourceIds.hpp"
#include "IBinaryChannelUtilities.hpp"
#include "IOBinProtocolXs.hpp"
#include "FileSystem.hpp"
#include "MessageLoopExecutor.hpp"
#include "GSProcessControl.hpp"
#include "MigrationHelper.hpp"

#define PREFERENCES_VERSION 1

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
    tempFileLoc.AppendToLocal (IO::Name (RSGetIndString (ID_BUILTINSCRIPTS_NAMES, resId, ACAPI_GetOwnResModule ()) + ".py"));

    IO::File file (tempFileLoc, IO::File::OnNotFound::Create);
    if (file.Open (IO::File::WriteEmptyMode) != NoError) {
        return {};
    }

    file.WriteBin (*gsHandle, BMGetHandleSize (gsHandle));

    return tempFileLoc;
}

bool TapirPalette::EnumFILEResIDsCallback (GSResID resID, GSResType /*resType*/, GSResModule /*resModule*/, void* palette)
{
    reinterpret_cast<TapirPalette*>(palette)->AddScriptToPopUp (GetBuiltInScriptLocation (resID), DG::PopUp::BottomItem);
    return true;
}

TapirPalette::TapirPalette ()
    : DG::Palette (ACAPI_GetOwnResModule (), ID_PALETTE, ACAPI_GetOwnResModule (), paletteGuid)
    , tapirIcon (GetReference (), 1)
    , scriptSelectionPopUp (GetReference (), 2)
    , runScriptButton (GetReference (), 3)
    , console (GetReference (), DG::Rect (DG::Point (0, 25), 250, 0), DG::RichEdit::HVScroll, DG::RichEdit::ReadOnly, DG::RichEdit::NoFrame)
{
    Attach (*this);
    AttachToAllItems (*this);

    short countOfOwnScripts = GetScriptsFromPreferences ();
    if (countOfOwnScripts > 0) {
        hasOwnScript = true;
        scriptSelectionPopUp.AppendSeparator ();
    } else {
        hasOwnScript = false;
    }

	RSEnumResourceIDs (EnumFILEResIDsCallback, this, 'FILE', ACAPI_GetOwnResModule ());

    scriptSelectionPopUp.AppendSeparator ();
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Add new script files...");

    console.Show ();

    BeginEventProcessing ();

    SetMinClientHeight (24);
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

void TapirPalette::PanelResized (const DG::PanelResizeEvent& ev)
{
    BeginMoveResizeItems ();
    scriptSelectionPopUp.Resize (ev.GetHorizontalChange (), 0);
    runScriptButton.Move (ev.GetHorizontalChange (), 0);
    console.Resize (ev.GetHorizontalChange (), ev.GetVerticalChange ());
    EndMoveResizeItems ();
}

void TapirPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent&, bool* accepted)
{
    Hide ();
    *accepted = true;
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
            if (executorThread) {
                process.Kill ();
            }
        }
    }
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    const short selectedItem = scriptSelectionPopUp.GetSelectedItem ();
    if (selectedItem == scriptSelectionPopUp.GetItemCount ()) {
        FTM::FileTypeManager	ftMan ("Python Script");
        FTM::GroupID			gid = ftMan.AddGroup ("Python Script");
        ftMan.AddType (FTM::FileType ("Python Script", "py", '    ', '    ', 0), gid);
        DG::FileDialog dlg (DG::FileDialog::OpenMultiFile);
        dlg.AddFilter (gid);
        dlg.SetHeader (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_TITLE, ACAPI_GetOwnResModule ()));
        dlg.SetOKButtonText (RSGetIndString (ID_PALETTE_STRINGS, ID_PALETTE_STRINGS_SELECT_SCRIPTS_BUTTON, ACAPI_GetOwnResModule ()));
        if (!dlg.Invoke ()) {
            return;
        }
        bool hasNew = false;
        for (UIndex i = 0; i < dlg.GetSelectionCount (); ++i) {
            IO::Location file = dlg.GetSelectedFile (i);
            if (IsPopUpContainsFile (file)) {
                continue;
            }
            if (!hasOwnScript) {
                scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
                hasOwnScript = true;
            }
            AddScriptToPopUp (file);
            hasNew = true;
        }
        if (hasNew) {
            scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
            SaveScriptsToPreferences ();
        } else {
            scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
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

    IO::Name fileName;
    fileLocation.GetLastLocalName (&fileName);

    if (!IsValidLocation (fileLocation)) {
        UpdateConsol (GS::UniString::Printf ("Failed to execute %T", fileName.ToString ().ToPrintf ()));
        return;
    }

    const GS::Array<GS::UniString> args = {"-X", "utf8=1", filePath, "--port", GS::ValueToUniString (GetConnectionPort ())};

    try {
        class IconUpdateTask : public GS::Runnable {
            short resId;
        public:
            IconUpdateTask (short _resId) : resId(_resId)
            {
            }
            virtual void Run ()
            {
                TapirPalette::Instance ().runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
            }
        };
        class ConsolUpdateTask : public GS::Runnable {
            GS::UniString text;
        public:
            ConsolUpdateTask (const GS::UniString& _text) : text(_text)
            {
            }
            virtual void Run ()
            {
                TapirPalette::Instance ().UpdateConsol (text);
            }
        };
        class PythonExecutionTask : public GS::Runnable
        {
        protected:
            GS::Process& process;

        public:
            explicit PythonExecutionTask (GS::Process& p) : process(p)
            {
            }
            virtual void Run () override final
            {
                GS::MessageLoopExecutor ().Execute (new IconUpdateTask (ID_KILL_BUTTON_ICON));
                process.GetExitCode ();
                auto output = GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (process.GetStandardErrorChannel (), GS::GetBinIProtocolX (), GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);
                if (!output.IsEmpty ()) {
                    output += '\n';
                }
                output += GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (process.GetStandardOutputChannel (), GS::GetBinIProtocolX (), GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);
                GS::MessageLoopExecutor ().Execute (new ConsolUpdateTask (output));
                GS::MessageLoopExecutor ().Execute (new IconUpdateTask (ID_RUN_BUTTON_ICON));
            }
        };
        process = GS::Process::Create ("python", args, GS::Process::CreateNoWindow, true, false, true);
	    executorThread = GS::NewRef<GS::Thread> (new PythonExecutionTask (process), "PythonProcess");
        executorThread->Start ();
    } catch (const GS::ProcessException&) {
        UpdateConsol (GS::UniString::Printf ("Failed to execute %T", fileName.ToString ().ToPrintf ()));
    } catch (...) {
        UpdateConsol (GS::UniString::Printf ("Unexpected issue while executing %T", fileName.ToString ().ToPrintf ()));
    }
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

void TapirPalette::SaveScriptsToPreferences ()
{
    GS::UniString preferencesStr;
    if (hasOwnScript) {
        for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1 && !scriptSelectionPopUp.IsSeparator (i); ++i) {
            GS::Ref<IO::Location> fileRef = GS::DynamicCast<IO::Location> (scriptSelectionPopUp.GetItemObjectData (i));
            if (fileRef == nullptr) {
                continue;
            }

            preferencesStr += fileRef->ToDisplayText () + '\n';
        }
    }
    auto cStr = preferencesStr.ToCStr ();
	ACAPI_SetPreferences (PREFERENCES_VERSION, (GSSize)strlen (cStr.Get()), cStr.Get());
}

short TapirPalette::GetScriptsFromPreferences ()
{
    Int32 version;
    GSSize nBytes;

    ACAPI_GetPreferences (&version, &nBytes, nullptr);
    if (version != PREFERENCES_VERSION || nBytes <= 0) {
        return 0;
    }

    std::unique_ptr<char> data(new char[nBytes]);
    short n = 0;

    ACAPI_GetPreferences (&version, &nBytes, data.get ());
    GS::Array<GS::UniString> ownScriptPathArray;
    GS::UniString (data.get ()).Split ("\n", GS::UniString::SkipEmptyParts, &ownScriptPathArray);
    for (auto&& scriptPath : ownScriptPathArray) {
        IO::Location ownScript (scriptPath);
        if (AddScriptToPopUp (ownScript, DG::PopUp::BottomItem)) {
            ++n;
        }
    }

    return n;
}

void TapirPalette::UpdateConsol (const GS::UniString& text)
{
    console.SetText (text);
    auto lastChar = console.GetLength () - 1;
    console.SetSelection (DG::CharRange (lastChar, lastChar));
}