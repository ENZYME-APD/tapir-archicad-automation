#pragma once

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "MarketplaceInstalls.hpp"

#include <ctime>
#include <vector>

// The Tapir Script Manager (#2408): a resizable window for browsing the marketplace scripts listed on
// https://archicad-api.com/scripts/. It reads the catalogue that tools/generate_catalogue.py writes to the
// site-data branch, from raw.githubusercontent.com (never through GitHub's API), when the window opens and on
// Refresh; Documents/Tapir/config.json's marketplaceCatalogueUrl overrides the address for testing. The last good
// copy is kept in Documents/Tapir/marketplace/catalogue.json and shown when the fetch fails.
//
// Install downloads the newest version's entry file at its pinned commit, checks its SHA-256 against the catalogue
// before anything is written, asks once per author whether their scripts may run, and writes it to
// Documents/Tapir/marketplace/<slug>/<version>/ (see MarketplaceInstalls.hpp). Remove deletes that folder.
//
// Parts 1 and 2 of the plan in #2408 are done: the window, the catalogue, search, categories, the Available and
// Installed tabs, the details, Install, Remove and Enabled. Updates (part 3), the palette's Marketplace section
// (part 4) and the README and screenshots (part 5) come next, so the Updates tab is empty for now.
class ScriptManager final : public DG::Palette,
    public DG::PanelObserver,
    public DG::ButtonItemObserver,
    public DG::ListBoxObserver,
    public DG::TextEditBaseObserver,
    public DG::CheckItemObserver,
    public DG::RadioItemObserver
{
public:
    // A version of a script, as in the catalogue. commit, url and sha256 are empty for a version that was never the
    // newest in a commit (see tools/generate_catalogue.py).
    struct ScriptVersion {
        GS::UniString    version;
        std::vector<int> archicadVersions;
        GS::UniString    minTapirVersion;
        GS::UniString    changelog;
        GS::UniString    commit;
        GS::UniString    url;
        GS::UniString    sha256;
    };

    // A script of the catalogue: the fields of its marketplace/<slug>/listing.json, the changes key of its entry
    // file's # /// tapir header, its screenshots, and its versions, the newest first.
    struct Script {
        GS::UniString              slug;
        GS::UniString              name;
        GS::UniString              summary;
        GS::UniString              category;
        GS::Array<GS::UniString>   tags;
        GS::UniString              authorName;
        GS::UniString              authorGithub;
        GS::UniString              license;
        GS::UniString              entry;
        GS::UniString              changes;   // "model", "files", "read-only" or empty
        GS::Array<GS::UniString>   screenshots;
        std::vector<ScriptVersion> versions;
        // False for an installed script the catalogue no longer lists (or no catalogue could be loaded): it is only
        // on the Installed tab, with its installed version as its one version.
        bool                       inCatalogue = true;
    };

    virtual ~ScriptManager ();

    static bool           HasInstance ();
    static ScriptManager& Instance ();

    // Opens the window, or brings it to the front when it is open. Opening it fetches the catalogue.
    void Show ();
    void Hide ();

    static GSErrCode RegisterPaletteControlCallBack ();

private:
    enum class Tab {
        Available,
        Installed,
        Updates
    };

    DG::TextEdit         searchEdit;
    DG::RadioButton      availableTab;
    DG::RadioButton      installedTab;
    DG::RadioButton      updatesTab;
    DG::LeftText         catalogueTimeText;
    DG::Button           refreshButton;
    DG::SingleSelListBox categoryList;
    DG::CheckBox         fitsCheckBox;
    DG::SingleSelListBox scriptList;
    DG::LeftText         nameText;
    DG::LeftText         bylineText;
    DG::LeftText         summaryText;
    DG::LeftText         changesLabel;
    DG::LeftText         changesValue;
    DG::LeftText         archicadLabel;
    DG::LeftText         archicadValue;
    DG::LeftText         tapirLabel;
    DG::LeftText         tapirValue;
    DG::LeftText         licenceLabel;
    DG::LeftText         licenceValue;
    DG::LeftText         whatIsNewTitle;
    DG::LeftText         changelogText;
    DG::Button           shareButton;
    DG::Button           scriptsPageButton;
    DG::Button           closeButton;
    DG::CheckBox         enabledCheckBox;
    DG::Button           installButton;
    DG::Button           removeButton;

    std::vector<Script> scripts;            // the catalogue, in its order (by slug), then the installed scripts it lacks
    bool                hasCatalogue = false;
    std::time_t         catalogueTime = 0;   // when the shown catalogue was fetched
    std::vector<size_t> shownScripts;       // the index in scripts of each row of scriptList
    GS::UniString       selectedSlug;       // the script selected last, selected again when the list is rebuilt
    bool                hiddenByArchicad = false;
    MarketplaceInstalls::State installs;     // installed.json

    // Fetches the catalogue (falling back to the cached copy, see the top of the class) and fills the window.
    void LoadCatalogue ();
    // Re-reads installed.json and lists the installed scripts the catalogue lacks (see Script::inCatalogue).
    void LoadInstalls ();
    void MergeInstalledScripts ();

    const MarketplaceInstalls::InstalledScript* GetInstalled (const Script& script) const;
    // Downloads, checks and writes the newest version of the script, after asking about its author; false when it
    // was not installed (the reason was shown).
    bool Install (const Script& script);
    bool Remove (const Script& script);
    void ShowInstallError (const Script& script, const GS::UniString& reason) const;

    Tab GetCurrentTab () const;
    GS::UniString GetSelectedCategory () const;   // empty: All categories
    // Whether the script fits the running Archicad and Tapir; reason: why not, empty when it fits.
    bool FitsThisArchicad (const Script& script, GS::UniString& reason) const;
    bool MatchesSearch (const Script& script, const GS::UniString& lowerCasedSearch) const;
    // The scripts of the current tab and search, the Fits filter included, before the category filter.
    std::vector<size_t> GetScriptsOfTabAndSearch () const;

    void RefreshAll ();          // the tab counts, the categories and the list
    void RefreshTabTexts ();
    void RefreshCategoryList ();
    void RefreshScriptList ();
    void RefreshDetails ();
    void SetupScriptListColumns ();
    void LayoutScriptListColumns ();

    const Script* GetSelectedScript () const;

    virtual void PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;
    virtual void PanelResized (const DG::PanelResizeEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
    virtual void ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;
    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override;
    virtual void CheckItemChanged (const DG::CheckItemChangeEvent& ev) override;
    virtual void RadioItemChanged (const DG::RadioItemChangeEvent& ev) override;

    static GSErrCode PaletteControlCallBack (Int32 paletteId, API_PaletteMessageID messageID, GS::IntPtr param);

    static const GS::Guid          paletteGuid;
    static GS::Ref<ScriptManager>  instance;

    ScriptManager ();
};
