#pragma once

#include "DGModule.hpp"
#include "Location.hpp"

#include <vector>

// The "Share a script" dialog: shows the whole file that will be sent, asks for a sentence, the name to credit and
// the MIT licence, and sends it to the project's share service. While the service does not answer, it opens the
// website's share page instead and stores nothing.
class ShareDialog : public DG::ModalDialog,
    public DG::ButtonItemObserver,
    public DG::CheckItemObserver,
    public DG::TextEditBaseObserver
{
public:
    // Opens the dialog for the given script. Shows an alert instead when the file cannot be shared (unreadable,
    // too large or not UTF-8 text).
    static void Open (const IO::Location& script);
    // The "Share a Script..." menu item: asks for a .py file, starting in Documents/Tapir/custom-scripts.
    static void OpenWithFileDialog ();

    virtual ~ShareDialog ();

private:
    ShareDialog (const IO::Location& script, const GS::UniString& content);

    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
    virtual void CheckItemChanged (const DG::CheckItemChangeEvent& ev) override;
    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override;

    void UpdateShareButton ();
    void Share ();

    IO::Location      script;
    GS::UniString     content;
    bool              shared = false;

    DG::Button        shareButton;
    DG::Button        cancelButton;
    DG::TextEdit      summaryEdit;
    DG::TextEdit      creditNameEdit;
    DG::TextEdit      emailEdit;
    DG::CheckBox      licenceCheck;
    DG::LeftText      statusText;
    DG::Button        openIssueButton;
    DG::Button        yourSharesButton;
    DG::MultiLineEdit contentEdit;

    GS::UniString     issueUrl;
};

// The "Your shares" dialog: the shares stored under "shares" in Documents/Tapir/config.json, each with its file,
// date, issue link and its status read from the share service with the share's key. Shows the reviewer's questions
// with a box to answer them, and a Confirm button once the adapted script was accepted. While the service does not
// answer, the status is "unknown" and the issue link still works.
class YourSharesDialog : public DG::ModalDialog,
    public DG::PanelObserver,
    public DG::ButtonItemObserver,
    public DG::ListBoxObserver,
    public DG::TextEditBaseObserver
{
public:
    // The "My Shared Scripts..." menu item and the "Your Shares" button of the Share dialog.
    static void Open ();

    virtual ~YourSharesDialog ();

private:
    enum class Status {
        Checking,
        Unknown,
        Waiting,
        Questions,
        Accepted,
        Declined
    };

    struct ShareEntry {
        GS::UniString              shareId;
        GS::UniString              key;
        GS::UniString              issueUrl;
        GS::UniString              file;
        GS::UniString              sharedAt;
        Status                     status = Status::Checking;
        std::vector<GS::UniString> questions;
        GS::UniString              listingUrl;
        bool                       confirmed = false;
    };

    YourSharesDialog ();

    virtual void PanelOpened (const DG::PanelOpenEvent& ev) override;
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;
    virtual void ListBoxSelectionChanged (const DG::ListBoxSelectionEvent& ev) override;
    virtual void TextEditChanged (const DG::TextEditChangeEvent& ev) override;

    void LoadShares ();
    // Reads the status of every share; after the first share the service does not answer at all, the rest are
    // "unknown" without waiting for the timeout again.
    void RefreshAllStatuses ();
    // Reads the status of one share. Returns false when the service could not be reached.
    bool RefreshStatus (ShareEntry& share);
    void FillList ();
    void UpdateRow (short listItem);
    void UpdateDetails ();
    ShareEntry* GetSelectedShare ();
    void SendAnswer ();
    void Confirm ();

    std::vector<ShareEntry> shares;

    DG::Button           closeButton;
    DG::SingleSelListBox sharesList;
    DG::LeftText         detailsText;
    DG::Button           openIssueButton;
    DG::Button           openListingButton;
    DG::Button           refreshButton;
    DG::LeftText         questionsLabel;
    DG::LeftText         answerLabel;
    DG::Button           sendAnswerButton;
    DG::LeftText         messageText;
    DG::LeftText         confirmText;
    DG::Button           confirmButton;
    DG::MultiLineEdit    questionsEdit;
    DG::MultiLineEdit    answerEdit;
};
