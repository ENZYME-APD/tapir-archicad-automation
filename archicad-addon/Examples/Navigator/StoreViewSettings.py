"""StoreViewSettings stores the current view settings (action Store) and puts them back (action Restore), for
example around a script that changes the view. The two come in pairs and cannot be stacked: a second Store
before a Restore fails.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('StoreViewSettings', {'action': 'Store'}))
aclib.PrintJson (aclib.RunTapirCommand ('StoreViewSettings', {'action': 'Store'}))

# ... change the view here, for example with SetCurrent3DStyle or ActivateLayerCombination ...

aclib.PrintJson (aclib.RunTapirCommand ('StoreViewSettings', {'action': 'Restore'}))
