#include "TapirPalette.hpp"
#include "UvManager.hpp" 
#include "ResourceIds.hpp"
#include "VersionChecker.hpp"
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
#include "AddOnVersion.hpp"
#include "MigrationHelper.hpp"

#include <map>
#include <regex>

const GS::Guid        TapirPalette::paletteGuid("{2D42DF37-222F-40CD-BA86-B3279CCA1FEE}");
GS::Ref<TapirPalette> TapirPalette::instance;

static UShort GetConnectionPort ()
{
    UShort portNumber;
    ACAPI_Command_GetHttpConnectionPort (&portNumber);
    return portNumber;
}

static IO::Location GetTapirTemporaryFolder ()
{
    IO::Location tempFolder;
    IO::fileSystem.GetSpecialLocation (IO::FileSystem::TemporaryFolder, &tempFolder);
    tempFolder.AppendToLocal (IO::Name ("Tapir"));
    return tempFolder;
}

static IO::Location SaveBuiltInScript (const IO::Location& baseFolderLoc, const IO::RelativeLocation& relLoc, const GS::UniString& content)
{
    IO::Location fileLoc = baseFolderLoc;
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

static GS::UniString DownloadFileContent (const GS::UniString& fileDownloadUrl, const std::map<GS::UniString, GS::UniString>& headers = {})
{
    IO::URI::URI connectionUrl (fileDownloadUrl);
    HTTP::Client::ClientConnection clientConnection (connectionUrl);
    clientConnection.Connect ();

    HTTP::Client::Request getRequest (HTTP::MessageHeader::Method::Get, "");

    getRequest.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/54.0.2840.99 Safari/537.36");
    for (const auto& kv : headers) {
        getRequest.GetRequestHeaderFieldCollection ().Add(kv.first, kv.second);
    }
    clientConnection.Send (getRequest);

    HTTP::Client::Response response;
    GS::IChannelX channel (clientConnection.BeginReceive (response), GS::GetNetworkByteOrderIProtocolX ());

    GS::UniString body = GS::IBinaryChannelUtilities::ReadUniStringAsUTF8 (channel, GS::IBinaryChannelUtilities::StringSerializationType::NotTerminated);

    clientConnection.FinishReceive ();
    clientConnection.Close (false);

    return body;
}

static std::map<GS::UniString, GS::UniString> GetFilesFromGitHubInRelativeLocation (
    const Config::Repository& repository,
    const GS::UniString& relativeLoc = GS::EmptyUniString)
{
    std::map<GS::UniString, GS::UniString> files;

    try {
        IO::URI::URI connectionUrl ("https://api.github.com");
        HTTP::Client::ClientConnection clientConnection (connectionUrl);
        clientConnection.Connect ();

        const GS::UniString& relativeLocation = relativeLoc.IsEmpty () ? repository.relativeLoc : relativeLoc;
        HTTP::Client::Request request (HTTP::MessageHeader::Method::Get, "/repos/" + repository.repoOwner + "/" + repository.repoName + "/contents/" + relativeLocation);
        request.GetRequestHeaderFieldCollection ().Add (HTTP::MessageHeader::HeaderFieldName::UserAgent,
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 Safari/537.36");
        request.GetRequestHeaderFieldCollection ().Add ("Accept", "application/vnd.github+json");
        if (!repository.token.IsEmpty ()) {
            request.GetRequestHeaderFieldCollection ().Add ("Authorization", "Bearer " + repository.token);
        }
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
                    auto subFiles = GetFilesFromGitHubInRelativeLocation (repository, pathValue->Get ());
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
    , addScriptButton (GetReference (), 5)
    , delScriptButton (GetReference (), 6)
    , manageShortcutsButton (GetReference (), 7)
{
    Attach (*this);
    AttachToAllItems (*this);

    LoadScriptsToPopUp ();

    SetRunButtonIcon ();

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

void TapirPalette::PanelOpened (const DG::PanelOpenEvent&)
{
    auto itemToSelect = AddScriptsFromPreferences ();
    if (itemToSelect <= 0 || scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
        itemToSelect = scriptSelectionPopUp.GetSelectedItem ();
        if (scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
            itemToSelect = DG::PopUp::TopItem;
        }
    }
    scriptSelectionPopUp.SelectItem (itemToSelect);
    RefreshScriptListShortcutLabels ();
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

static void OpenFileInExplorer (const IO::Location& file)
{
    IO::Location folderLoc = file;
    folderLoc.DeleteLastLocalName ();

    GS::UniString pathStr;
    folderLoc.ToPath (&pathStr);
    const GS::UniString command = GS::UniString::Printf (
        "%s %T",
#ifdef WINDOWS
        "explorer",
#else
        "open",
#endif
        pathStr.ToPrintf ());
    system (command.ToCStr ().Get ());
}

void TapirPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
    if (ev.GetSource () == &runScriptButton) {
        if (IsProcessRunning ()) {
            process.Kill ();
        } else {
            if (Config::Instance().AskUpdatingAddOnBeforeEachExecution () && UpdateAddOn ()) {
                return;
            }

            GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (scriptSelectionPopUp.GetSelectedItem ()));
            if (popUpItemData != nullptr) {
                ExecuteScript (*popUpItemData);
            }
        }
    } else if (ev.GetSource () == &tapirButton) {
        OpenWebpage ("https://github.com/ENZYME-APD/tapir-archicad-automation");
    } else if (ev.GetSource () == &openScriptButton) {
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (scriptSelectionPopUp.GetSelectedItem ()));
        if (popUpItemData->repoRelLoc.IsEmpty ()) {
            OpenFileInExplorer (popUpItemData->fileLocation);
        } else {
            OpenWebpage ("https://github.com/" + popUpItemData->repo->repoOwner + "/" + popUpItemData->repo->repoName + "/blob/main/" + popUpItemData->repoRelLoc);
        }
    } else if (ev.GetSource () == &addScriptButton) {
        AddNewScript ();
    } else if (ev.GetSource () == &delScriptButton) {
        DeleteScriptFromPopUp ();
    } else if (ev.GetSource () == &manageShortcutsButton) {
        OpenShortcutsDialog ();
    }
}

bool TapirPalette::IsPopUpContainsFile (const IO::Location& fileLocation) const
{
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }

        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr && popUpItemData->fileLocation == fileLocation) {
            return true;
        }
    }
    return false;
}

void TapirPalette::PopUpChanged (const DG::PopUpChangeEvent& ev)
{
    if (ev.GetSource () == &scriptSelectionPopUp) {
        const short selectedItem = scriptSelectionPopUp.GetSelectedItem ();

        if (selectedItem == scriptSelectionPopUp.GetItemCount ()) {
            LoadScriptsToPopUp ();

            if (!scriptSelectionPopUp.IsSeparator (ev.GetPreviousSelection ()) &&
                ev.GetPreviousSelection () < scriptSelectionPopUp.GetItemCount ()) {
                scriptSelectionPopUp.SelectItem (ev.GetPreviousSelection ());
            } else {
                scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
            }
        }

        SaveScriptsToPreferences ();
        SetDeleteScriptButtonStatus ();
    }
}

void TapirPalette::RunShortcutSlot (short slotIndex)
{
    // Uses DG_ERROR (pops an alert) rather than DG_WARNING (silently logged) for every failure
    // path here on purpose: a keyboard-shortcut-triggered action needs to be impossible to miss —
    // a report-log-only warning looks identical to "nothing happened" if the Report window isn't open.
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount || scriptShortcutSlots[slotIndex].IsEmpty ()) {
        WriteReport (DG_ERROR, "No script assigned to shortcut slot %d.", (int) (slotIndex + 1));
        return;
    }
    if (IsProcessRunning ()) {
        WriteReport (DG_ERROR, "A script is already running - wait for it to finish before using a shortcut slot.");
        return;
    }
    if (Config::Instance ().AskUpdatingAddOnBeforeEachExecution () && UpdateAddOn ()) {
        return;
    }

    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr && popUpItemData->fileLocation == scriptShortcutSlots[slotIndex]) {
            ExecuteScript (*popUpItemData);
            return;
        }
    }
    WriteReport (DG_ERROR, "Script assigned to shortcut slot %d could not be found (%d scripts currently listed).",
        (int) (slotIndex + 1), (int) (scriptSelectionPopUp.GetItemCount () - 1));
}

void TapirPalette::GetAvailableScripts (GS::Array<std::pair<GS::UniString, IO::Location>>& outScripts)
{
    outScripts.Clear ();
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData != nullptr) {
            outScripts.Push ({ scriptSelectionPopUp.GetItemText (i), popUpItemData->fileLocation });
        }
    }
}

IO::Location TapirPalette::GetShortcutSlot (short slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return IO::Location ();
    }
    return scriptShortcutSlots[slotIndex];
}

void TapirPalette::SetShortcutSlot (short slotIndex, const IO::Location& location)
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }
    // A script only ever occupies one slot — clear it from any other slot first.
    if (!location.IsEmpty ()) {
        for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
            if (scriptShortcutSlots[slot] == location) {
                scriptShortcutSlots[slot] = IO::Location ();
            }
        }
    }
    scriptShortcutSlots[slotIndex] = location;
    SaveScriptsToPreferences ();
    RefreshScriptListShortcutLabels ();
}

GS::UniString TapirPalette::GetShortcutLabel (short slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return GS::EmptyUniString;
    }
    return scriptShortcutLabels[slotIndex];
}

void TapirPalette::SetShortcutLabel (short slotIndex, const GS::UniString& label)
{
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }
    if (scriptShortcutLabels[slotIndex] == label) {
        return;
    }
    scriptShortcutLabels[slotIndex] = label;
    ApplyShortcutMenuItemText (slotIndex);
    SaveScriptsToPreferences ();
}

// scriptSelectionPopUp items get "  [Shortcut N]" appended when the script backing them is assigned
// to a slot, so both "which slot is this" and "what's already taken" are visible while just browsing
// the normal script dropdown, not only inside the dedicated Shortcuts dialog.
void TapirPalette::RefreshScriptListShortcutLabels ()
{
    for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1; ++i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            continue;
        }
        GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
        if (popUpItemData == nullptr) {
            continue;
        }
        GS::UniString label = popUpItemData->baseDisplayText;
        for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
            if (!scriptShortcutSlots[slot].IsEmpty () && scriptShortcutSlots[slot] == popUpItemData->fileLocation) {
                label += GS::UniString::Printf ("   [Shortcut %d]", (int) (slot + 1));
                break;
            }
        }
        scriptSelectionPopUp.SetItemText (i, label);
    }
}

void TapirPalette::ApplyShortcutMenuItemText (short slotIndex)
{
    static const short menuResIds[ScriptShortcutSlotCount] = {
        ID_ADDON_MENU_RUNSCRIPT_1, ID_ADDON_MENU_RUNSCRIPT_2, ID_ADDON_MENU_RUNSCRIPT_3,
        ID_ADDON_MENU_RUNSCRIPT_4, ID_ADDON_MENU_RUNSCRIPT_5, ID_ADDON_MENU_RUNSCRIPT_6
    };
    if (slotIndex < 0 || slotIndex >= ScriptShortcutSlotCount) {
        return;
    }

    GS::UniString label = scriptShortcutLabels[slotIndex];
    if (label.IsEmpty ()) {
        label = GS::UniString::Printf ("Run Script Shortcut %d", (int) (slotIndex + 1));
    }

    API_MenuItemRef itemRef = {};
    itemRef.menuResID = menuResIds[slotIndex];
    itemRef.itemIndex = ID_ADDON_MENU_RUNSCRIPT_ITEM;
    ACAPI_MenuItem_SetMenuItemText (&itemRef, nullptr, &label);
}

void TapirPalette::ApplyAllShortcutMenuItemTexts ()
{
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        ApplyShortcutMenuItemText (slot);
    }
}

namespace {

// Dedicated dialog for assigning scripts to shortcut slots: each row shows the slot's currently
// assigned script directly in its own popup (and "(unassigned)" when empty), plus an editable label
// for the text shown in Archicad's menu/toolbar for that shortcut, so both "which slot is this" and
// "what's already taken" are visible in one screen, instead of blind slot numbers.
class TapirShortcutsDialog : public DG::ModalDialog,
    public DG::ButtonItemObserver,
    public DG::PopUpObserver,
    public DG::TextEditBaseObserver
{
public:
    TapirShortcutsDialog ()
        : DG::ModalDialog (ACAPI_GetOwnResModule (), ID_SHORTCUTS_DIALOG, ACAPI_GetOwnResModule ())
        , slotPopUp1 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_1)
        , slotPopUp2 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_2)
        , slotPopUp3 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_3)
        , slotPopUp4 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_4)
        , slotPopUp5 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_5)
        , slotPopUp6 (GetReference (), ID_SHORTCUTS_DIALOG_POPUP_6)
        , labelEdit1 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_1)
        , labelEdit2 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_2)
        , labelEdit3 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_3)
        , labelEdit4 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_4)
        , labelEdit5 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_5)
        , labelEdit6 (GetReference (), ID_SHORTCUTS_DIALOG_LABELEDIT_6)
        , closeButton (GetReference (), ID_SHORTCUTS_DIALOG_CLOSE_BUTTON)
    {
        slotPopUps[0] = &slotPopUp1;
        slotPopUps[1] = &slotPopUp2;
        slotPopUps[2] = &slotPopUp3;
        slotPopUps[3] = &slotPopUp4;
        slotPopUps[4] = &slotPopUp5;
        slotPopUps[5] = &slotPopUp6;

        labelEdits[0] = &labelEdit1;
        labelEdits[1] = &labelEdit2;
        labelEdits[2] = &labelEdit3;
        labelEdits[3] = &labelEdit4;
        labelEdits[4] = &labelEdit5;
        labelEdits[5] = &labelEdit6;

        closeButton.Attach (*this);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            slotPopUps[row]->Attach (*this);
            labelEdits[row]->Attach (*this);
        }

        TapirPalette::Instance ().GetAvailableScripts (scripts);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            BuildRowItems (*slotPopUps[row]);
        }
        RefreshAllRows ();
    }

    ~TapirShortcutsDialog ()
    {
        closeButton.Detach (*this);
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            slotPopUps[row]->Detach (*this);
            labelEdits[row]->Detach (*this);
        }
    }

private:
    DG::PopUp    slotPopUp1, slotPopUp2, slotPopUp3, slotPopUp4, slotPopUp5, slotPopUp6;
    DG::TextEdit labelEdit1, labelEdit2, labelEdit3, labelEdit4, labelEdit5, labelEdit6;
    DG::Button   closeButton;
    DG::PopUp*     slotPopUps[TapirPalette::ScriptShortcutSlotCount];
    DG::TextEdit*  labelEdits[TapirPalette::ScriptShortcutSlotCount];
    GS::Array<std::pair<GS::UniString, IO::Location>> scripts;

    void BuildRowItems (DG::PopUp& popUp)
    {
        popUp.AppendItem ();
        popUp.SetItemText (DG::PopUp::BottomItem, "(unassigned)");
        for (const auto& script : scripts) {
            popUp.AppendItem ();
            popUp.SetItemText (DG::PopUp::BottomItem, script.first);
        }
    }

    void RefreshAllRows ()
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            const IO::Location assigned = TapirPalette::Instance ().GetShortcutSlot (row);
            short itemToSelect = 1;   // "(unassigned)"
            if (!assigned.IsEmpty ()) {
                for (UIndex i = 0; i < scripts.GetSize (); ++i) {
                    if (scripts[i].second == assigned) {
                        itemToSelect = (short) (i + 2);
                        break;
                    }
                }
            }
            slotPopUps[row]->SelectItem (itemToSelect);

            GS::UniString label = TapirPalette::Instance ().GetShortcutLabel (row);
            if (label.IsEmpty ()) {
                label = GS::UniString::Printf ("Run Script Shortcut %d", (int) (row + 1));
            }
            labelEdits[row]->SetText (label);
        }
    }

    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override
    {
        if (ev.GetSource () == &closeButton) {
            PostCloseRequest (DG::ModalDialog::Accept);
        }
    }

    virtual void PopUpChanged (const DG::PopUpChangeEvent& ev) override
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            if (ev.GetSource () == slotPopUps[row]) {
                const short selectedItem = slotPopUps[row]->GetSelectedItem ();
                IO::Location newLocation;   // empty = unassigned
                if (selectedItem >= 2 && (UIndex) (selectedItem - 2) < scripts.GetSize ()) {
                    newLocation = scripts[selectedItem - 2].second;
                }
                TapirPalette::Instance ().SetShortcutSlot (row, newLocation);
                RefreshAllRows ();   // another row may have just lost this script to the dedup rule
                return;
            }
        }
    }

    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override
    {
        for (short row = 0; row < TapirPalette::ScriptShortcutSlotCount; ++row) {
            if (ev.GetSource () == labelEdits[row]) {
                TapirPalette::Instance ().SetShortcutLabel (row, labelEdits[row]->GetText ());
                return;
            }
        }
    }
};

} // namespace

void TapirPalette::OpenShortcutsDialog ()
{
    TapirShortcutsDialog dialog;
    dialog.Invoke ();
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

void TapirPalette::ExecuteScript (const PopUpItemData& popUpItemData)
{
    GS::UniString filePath;
    popUpItemData.fileLocation.ToPath (&filePath);

    class UIUpdaterThread : public GS::Runnable
    {
        GS::Process& process;

        class IconUpdateTask : public GS::Runnable {
        public:
            IconUpdateTask () = default;
            virtual void Run ()
            {
                TapirPalette::Instance ().SetRunButtonIcon ();
            }
        };
        class OutputUpdateTask : public GS::Runnable {
            short type;
            GS::UniString text;
        public:
            OutputUpdateTask (short _type, const GS::UniString& _text) : text(_text), type(_type)
            {}
            virtual void Run ()
            {
                TapirPalette::Instance ().WriteReport (type, text);
            }
        };

        void ProcessStderrBlock (const GS::UniString& stderrContent)
        {
            if (stderrContent.IsEmpty ()) {
                return;
            }

            const GS::UniString lowerContent = stderrContent.ToLowerCase ();

            if (lowerContent.Contains ("error:") || lowerContent.Contains ("failed") || lowerContent.Contains ("traceback (most recent call last)")) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_ERROR, stderrContent));
                return;
            }
            if (lowerContent.Contains ("warning:")) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_WARNING, stderrContent));
                return;
            }
            GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, stderrContent));
        }

    public:
        explicit UIUpdaterThread (GS::Process& p) : process(p)
        {
        }
        GS::UniString ReadFromChannel (GS::IBinaryChannel& channel)
        {
            if (channel.GetAvailable () <= 0) {
                return GS::EmptyUniString;
            }

            const GS::USize uSize = static_cast<GS::USize> (channel.GetAvailable ());
            std::unique_ptr<char> buffer;
            buffer.reset (new char[uSize + 1]);

            GS::IBinaryChannelUtilities::ReadFully (channel, buffer.get (), uSize);
            return GS::UniString (buffer.get (), uSize, CC_UTF8);
        }
        void ReadFromChannels ()
        {
            const GS::UniString stdError = ReadFromChannel (process.GetStandardErrorChannel ());
            ProcessStderrBlock (stdError);

            const GS::UniString stdOutput = ReadFromChannel (process.GetStandardOutputChannel ());
            if (!stdOutput.IsEmpty ()) {
                GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, stdOutput));
            }
        }
        virtual void Run () override final
        {
            GS::MessageLoopExecutor ().Execute (new IconUpdateTask ());

            const GS::Timeout Timeout = {0, 0, 0, 10 /*ms*/};
            while (!process.WaitFor (Timeout)) {
                ReadFromChannels ();
            }

            const int exitCode = process.GetExitCode ();
            ReadFromChannels ();
            process = {}; // Reset the process to an invalid state

            GS::MessageLoopExecutor ().Execute (new IconUpdateTask ());
            GS::MessageLoopExecutor ().Execute (new OutputUpdateTask (DG_INFORMATION, "ExitCode: " + GS::ValueToUniString (exitCode)));
        }
    };

    try {
        WriteReport (DG_INFORMATION, "Executing %T with uv", filePath.ToPrintf ());

        GS::UniString command;
        GS::Array<GS::UniString> argv;
        if (filePath.EndsWith (".py")) {
            const GS::UniString uvCommand = uvManager.GetUvExecutablePath ();
            if (uvCommand.IsEmpty ()) {
                // An alert was already shown to the user inside the manager, so we just exit.
                return;
            }
            command = uvCommand;
            argv = {"run", "--script", filePath, "--port", GS::ValueToUniString (GetConnectionPort ())};
            if (popUpItemData.repo != nullptr && !popUpItemData.repo->token.IsEmpty ()) {
                argv.Append ({"--token", popUpItemData.repo->token});
            }
        } else {
            command = filePath;
            argv = {"--port", GS::ValueToUniString (GetConnectionPort ())};
        }

        constexpr bool redirectStandardOutput = true;
        constexpr bool redirectStandardInput = false;
        constexpr bool redirectStandardError = true;
        process = GS::Process::Create (command, argv, GS::Process::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);
        if (!process.IsValid ()) {
            WriteReport (DG_ERROR, "Failed to start uv process. Ensure it is installed and executable.");
        }

        executor.Execute (new UIUpdaterThread (process));
    } catch (const GS::ProcessException&) {
        WriteReport (DG_ERROR, "Failed to execute %T", filePath.ToPrintf ());
    } catch (...) {
        WriteReport (DG_ERROR, "Unexpected issue while executing %T", filePath.ToPrintf ());
    }
}

bool TapirPalette::AddScriptToPopUp (GS::Ref<PopUpItemData> popUpData, short index)
{
    if (popUpData == nullptr || !IsValidLocation (popUpData->fileLocation)) {
        return false;
    }

    IO::Name name;
    popUpData->fileLocation.GetLastLocalName (&name);

#if defined (macintosh)
    if (name.ToString ().BeginsWith ('.')) {
        return false;
    }
#endif

    scriptSelectionPopUp.InsertItem (index);
    if (popUpData->repo) {
        auto* repo = popUpData->repo;
        if (repo->displayName.IsEmpty ()) {
            popUpData->baseDisplayText = name.ToString () + " (" + repo->repoOwner + "/" + repo->repoName + ")";
        } else {
            popUpData->baseDisplayText = name.ToString () + " (" + repo->displayName + ")";
        }
    } else {
        popUpData->baseDisplayText = name.ToString ();
    }
    scriptSelectionPopUp.SetItemText (index, popUpData->baseDisplayText);
    scriptSelectionPopUp.SetItemObjectData (index, popUpData);

    return true;
}

void TapirPalette::AddScriptsFromRepositories ()
{
    IO::Location tapirTempFolder = GetTapirTemporaryFolder ();
    IO::fileSystem.Delete (tapirTempFolder);

    const auto& repositories = Config::Instance ().Repositories ();
    for (auto& repo : repositories) {
        const std::unique_ptr<std::regex> excludeFromDownloadRegex = repo.excludeFromDownloadPattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.excludeFromDownloadPattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> excludeRegex = repo.excludePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.excludePattern.ToCStr ().Get ());
        const std::unique_ptr<std::regex> includeRegex = repo.includePattern.IsEmpty () ? nullptr : std::make_unique<std::regex> (repo.includePattern.ToCStr ().Get ());

        IO::RelativeLocation repoRelativeLoc (repo.repoOwner);
        repoRelativeLoc.Append (IO::Name (repo.repoName));
        IO::RelativeLocation repoFolderRelativeLoc (repoRelativeLoc);
        if (!repo.relativeLoc.IsEmpty ()) {
            repoFolderRelativeLoc.Append (IO::RelativeLocation (repo.relativeLoc));
        };
        std::map<GS::UniString, GS::UniString> headers;
        if (!repo.token.IsEmpty ()) {
            headers.emplace ("Authorization", "Bearer " + repo.token);
        }
        ACAPI_WriteReport ("Tapir is downloading content from GitHub repository: " + repo.repoOwner + "/" + repo.repoName, false);
        for (auto kv : GetFilesFromGitHubInRelativeLocation (repo)) {
            const GS::UniString repoLoc = repo.repoOwner + "/" + repo.repoName + "/" + kv.first;
            if (excludeFromDownloadRegex && std::regex_match (repoLoc.ToCStr ().Get (), *excludeFromDownloadRegex)) {
                ACAPI_WriteReport ("Skipping download of " + repoLoc + " due to exclude pattern", false);
                continue;
            }
            const auto content = DownloadFileContent (kv.second, headers);
            IO::RelativeLocation relLoc = repoRelativeLoc;
            relLoc.Append (IO::RelativeLocation (kv.first));
            const IO::Location fileLoc = SaveBuiltInScript (tapirTempFolder, relLoc, content);
            ACAPI_WriteReport ("Downloaded " + repoLoc, false);
            if (excludeRegex) {
                if (std::regex_match (kv.first.ToCStr ().Get (), *excludeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + repoLoc + " due to exclude pattern", false);
                    continue;
                }
            } else if (includeRegex) {
                if (!std::regex_match (kv.first.ToCStr ().Get (), *includeRegex)) {
                    ACAPI_WriteReport ("Skipping use of " + repoLoc + " due to include pattern", false);
                    continue;
                }
            } else {
                IO::Name fileName;
                if (relLoc.GetLength () != (1 + repoFolderRelativeLoc.GetLength ()) ||
                    fileLoc.GetLastLocalName (&fileName) != NoError ||
                    fileName.GetExtension ().ToLowerCase () != "py") {
                    continue;
                }
            }
            ACAPI_WriteReport ("Added to the popup: " + repoLoc, false);
            AddScriptToPopUp (GS::NewRef<PopUpItemData> (fileLoc, kv.first, &repo), DG::PopUp::BottomItem);
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
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (file));
    });
}

void TapirPalette::LoadScriptsToPopUp ()
{
    scriptSelectionPopUp.DeleteItem (DG_ALL_ITEMS);

    hasCustomScript = false;
    hasAddedScript = false;

    AddScriptsFromCustomScriptsFolder ();
    auto itemToSelect = AddScriptsFromPreferences ();
    AddScriptsFromRepositories ();

    scriptSelectionPopUp.AppendSeparator ();
    scriptSelectionPopUp.AppendItem ();
    scriptSelectionPopUp.SetItemText (DG::PopUp::BottomItem, "Reload scripts...");

    if (itemToSelect <= 0 || scriptSelectionPopUp.IsSeparator (itemToSelect) || scriptSelectionPopUp.GetItemCount () - 1 <= itemToSelect) {
        itemToSelect = DG::PopUp::TopItem;
    }
    scriptSelectionPopUp.SelectItem (itemToSelect);

    RefreshScriptListShortcutLabels ();
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetRunButtonIcon ()
{
    short resId = ID_RUN_BUTTON_ICON;
    if (IsProcessRunning ()) {
        resId = ID_KILL_BUTTON_ICON;
    } else if (VersionChecker::IsUsingLatestVersion ()) {
        resId = ID_RUN_BUTTON_ICON;
    } else {
        resId = ID_RUNWITHUPDATE_BUTTON_ICON;
    }
    runScriptButton.SetIcon (DG::Icon (ACAPI_GetOwnResModule (), resId));
}

bool TapirPalette::UpdateAddOn ()
{
    if (VersionChecker::IsUsingLatestVersion ()) {
        return false;
    }

    short response = DGAlert (DG_WARNING,
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TITLE, ACAPI_GetOwnResModule ()),
        GS::UniString::Printf (RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT1, ACAPI_GetOwnResModule ()),
            ADDON_VERSION, VersionChecker::LatestVersion ().ToPrintf ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_TEXT2, ACAPI_GetOwnResModule ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON1, ACAPI_GetOwnResModule ()),
        RSGetIndString (ID_AUTOUPDATE_STRINGS, ID_AUTOUPDATE_NEWVERSION_ALERT_BUTTON2, ACAPI_GetOwnResModule ()));

    if (response != DG_OK) {
        return false;
    }

    const GS::UniString uvCommand = uvManager.GetUvExecutablePath ();
    if (uvCommand.IsEmpty ()) {
        DGAlert (DG_ERROR, "Update Failed", "The update process requires 'uv' to be installed.", "Please install 'uv' and try again.", "OK");
        return false;
    }

    constexpr const char* fileName = "update_addon_and_restart_archicad.py";
    const GS::UniString url = "https://raw.githubusercontent.com/ENZYME-APD/tapir-archicad-automation/main/archicad-addon/Tools/" + GS::UniString (fileName);
    const GS::UniString content = DownloadFileContent (url);
    if (content.GetLength () < 10) {
        response = DGAlert (DG_ERROR, "Tapir Update", "Failed to download the update script.", "Please check your internet connection and try again.", "OK", "Cancel");

        if (response != DG_OK) {
            return false;
        }

        return UpdateAddOn ();
    }
    const IO::Location fileLoc = SaveBuiltInScript (GetTapirTemporaryFolder (), IO::RelativeLocation (fileName), content);

    IO::Location addOnLocation;
    ACAPI_GetOwnLocation (&addOnLocation);
    GS::UniString addOnLocationStr;
    addOnLocation.ToPath (&addOnLocationStr);

    GS::UniString filePath;
    fileLoc.ToPath (&filePath);

    const GS::Array<GS::UniString> argv = { "run", "--script", filePath, "--port", GS::ValueToUniString (GetConnectionPort ()), "--downloadUrl", VersionChecker::LatestVersionDownloadUrl (), "--addOnLocation", addOnLocationStr};

    constexpr bool redirectStandardOutput = false;
    constexpr bool redirectStandardInput = false;
    constexpr bool redirectStandardError = false;
    process = GS::Process::Create (uvCommand, argv, GS::Process::CreateNoWindow, redirectStandardOutput, redirectStandardInput, redirectStandardError);

    SetRunButtonIcon ();

    return true;
}

#define PREFERENCES_VERSION 12
#define PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS 11
#define PREFERENCES_VERSION_BEFORE_SHORTCUT_SLOTS 10
static const GS::UniString ShortcutSlotEmptyMarker ("(unassigned)");   // never a valid file path; "" would be dropped by SkipEmptyParts
static const GS::UniString ShortcutLabelDefaultMarker ("(default)");   // means "use the default 'Run Script Shortcut N' text"; "" would be dropped by SkipEmptyParts

void TapirPalette::SaveScriptsToPreferences ()
{
    GS::UniString preferencesStr;
    if (hasAddedScript) {
        for (short i = 1; i <= scriptSelectionPopUp.GetItemCount () - 1 && !scriptSelectionPopUp.IsSeparator (i); ++i) {
            GS::Ref<PopUpItemData> popUpItemData = GS::DynamicCast<PopUpItemData> (scriptSelectionPopUp.GetItemObjectData (i));
            if (popUpItemData == nullptr) {
                continue;
            }

            GS::UniString pathStr;
            popUpItemData->fileLocation.ToPath (&pathStr);
            preferencesStr += pathStr + '\n';
        }
    }
    preferencesStr += GS::ValueToUniString (scriptSelectionPopUp.GetSelectedItem ());
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        GS::UniString slotPathStr = ShortcutSlotEmptyMarker;
        if (!scriptShortcutSlots[slot].IsEmpty ()) {
            scriptShortcutSlots[slot].ToPath (&slotPathStr);
        }
        preferencesStr += '\n' + slotPathStr;
    }
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        const GS::UniString& labelStr = scriptShortcutLabels[slot].IsEmpty () ? ShortcutLabelDefaultMarker : scriptShortcutLabels[slot];
        preferencesStr += '\n' + labelStr;
    }
    auto cStr = preferencesStr.ToCStr ();
    ACAPI_SetPreferences (PREFERENCES_VERSION, (GSSize)strlen (cStr.Get()), cStr.Get());
}

bool TapirPalette::IsValidLocation (const IO::Location& location)
{
    if (location.IsEmpty ()) {
        return false;
    }

    bool exists = false;
    GSErrCode err = IO::fileSystem.Contains (location, &exists);
    return err == NoError && exists;
}

short TapirPalette::AddScriptsFromPreferences ()
{
    if (hasAddedScript) {
        while (!scriptSelectionPopUp.IsSeparator (DG::PopUp::TopItem)) {
            scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        }
    }
    for (short slot = 0; slot < ScriptShortcutSlotCount; ++slot) {
        scriptShortcutSlots[slot] = IO::Location ();
        scriptShortcutLabels[slot] = GS::EmptyUniString;
    }

    Int32 version;
    GSSize nBytes;

    ACAPI_GetPreferences (&version, &nBytes, nullptr);
    if (nBytes <= 0 || (version != PREFERENCES_VERSION && version != PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS && version != PREFERENCES_VERSION_BEFORE_SHORTCUT_SLOTS)) {
        return DG::PopUp::TopItem;
    }

    std::unique_ptr<char> data(new char[nBytes]);
    ACAPI_GetPreferences (&version, &nBytes, data.get ());
    GS::Array<GS::UniString> scriptPathArray;
    GS::UniString (data.get ()).Split ("\n", GS::UniString::SkipEmptyParts, &scriptPathArray);

    if (version == PREFERENCES_VERSION && scriptPathArray.GetSize () >= 2 * ScriptShortcutSlotCount) {
        // Labels were appended last, so they must be popped first.
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString labelStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (labelStr != ShortcutLabelDefaultMarker) {
                scriptShortcutLabels[slot] = labelStr;
            }
        }
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString slotPathStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (slotPathStr != ShortcutSlotEmptyMarker) {
                scriptShortcutSlots[slot] = IO::Location (slotPathStr);
            }
        }
    } else if (version == PREFERENCES_VERSION_BEFORE_SHORTCUT_LABELS && scriptPathArray.GetSize () >= ScriptShortcutSlotCount) {
        for (short slot = ScriptShortcutSlotCount - 1; slot >= 0; --slot) {
            GS::UniString slotPathStr = scriptPathArray.GetLast ();
            scriptPathArray.DeleteLast ();
            if (slotPathStr != ShortcutSlotEmptyMarker) {
                scriptShortcutSlots[slot] = IO::Location (slotPathStr);
            }
        }
    }

    ApplyAllShortcutMenuItemTexts ();

    short selectedItemBeforeClose = -1;
    if (!scriptPathArray.IsEmpty () && GS::UniStringToValue<short> (scriptPathArray.GetLast (), selectedItemBeforeClose, GS::ToValueMode::Strict)) {
        scriptPathArray.DeleteLast();
    }
    for (auto&& scriptPath : scriptPathArray) {
        IO::Location ownScript (scriptPath);
        if (!IsValidLocation (ownScript)) {
            continue;
        }
        if (!hasAddedScript) {
            scriptSelectionPopUp.InsertSeparator (DG::PopUp::TopItem);
            hasAddedScript = true;
        }
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (ownScript));
    }
    return selectedItemBeforeClose;
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
        AddScriptToPopUp (GS::NewRef<PopUpItemData> (file));
        hasNew = true;
    }

    if (hasNew) {
        scriptSelectionPopUp.SelectItem (DG::PopUp::TopItem);
        SaveScriptsToPreferences ();
        SetDeleteScriptButtonStatus ();
    }

    return hasNew;
}

void TapirPalette::DeleteScriptFromPopUp ()
{
    short selectedItem = scriptSelectionPopUp.GetSelectedItem ();
    scriptSelectionPopUp.DeleteItem (selectedItem);
    if (scriptSelectionPopUp.IsSeparator (DG::PopUp::TopItem)) {
        scriptSelectionPopUp.DeleteItem (DG::PopUp::TopItem);
        hasAddedScript = false;
    }
    if (scriptSelectionPopUp.IsSeparator (scriptSelectionPopUp.GetSelectedItem ())) {
        scriptSelectionPopUp.SelectItem (scriptSelectionPopUp.GetSelectedItem () - 1);
    }

    SaveScriptsToPreferences ();
    SetDeleteScriptButtonStatus ();
}

void TapirPalette::SetDeleteScriptButtonStatus ()
{
    if (!hasAddedScript) {
        delScriptButton.Disable ();
        return;
    }

    for (short i = scriptSelectionPopUp.GetSelectedItem (); i >= DG_POPUP_TOP; --i) {
        if (scriptSelectionPopUp.IsSeparator (i)) {
            delScriptButton.Disable ();
            return;
        }
    }

    delScriptButton.Enable ();
}