#pragma once

#include "DGModule.hpp"

class AboutDialog :
    public DG::ModalDialog,
    public DG::PanelObserver,
    public DG::ButtonItemObserver,
    public DG::CompoundItemObserver
{
public:
    AboutDialog ();

private:
    virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override;

    DG::Button okButton;
    DG::CenterText versionText;
};
