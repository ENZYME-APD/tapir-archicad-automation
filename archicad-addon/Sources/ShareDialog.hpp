#pragma once

#include "DGModule.hpp"
#include "Location.hpp"

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
    DG::MultiLineEdit contentEdit;

    GS::UniString     issueUrl;
};
