// *****************************************************************************
// Header file for UserItemDialog, used by example Add-Ons to draw native images
// *****************************************************************************

#if !defined (USERITEMDIALOG_H)
#define USERITEMDIALOG_H

#pragma once

#include	"DGModule.hpp"
#include	"NativeImage.hpp"


class UserItemDialog : public DG::ModalDialog,
					   public DG::ButtonItemObserver,
					   public DG::UserItemObserver,
					   public DG::CompoundItemObserver
{
public:
	using NativeImageRendererFunc = std::function<NewDisplay::NativeImage (const DG::UserItem&, const UIndex)>;

private:
	DG::Button							okButton;
	GS::Array<GS::Owner<DG::UserItem>>	userItems;
	GS::Array<NewDisplay::NativeImage>	nativeImages;

	virtual void	ButtonClicked (const DG::ButtonClickEvent&) override;
	virtual void	UserItemUpdate (const DG::UserItemUpdateEvent& ev) override;

public:
	UserItemDialog (short userItemCount,
					short userItemWidth,
					short userItemHeight,
					const NativeImageRendererFunc& rendererFunc);
	~UserItemDialog ();

	UIndex GetUserItemIndex (const DG::UserItem* userItem) const;
};

#endif // USERITEMDIALOG_H
