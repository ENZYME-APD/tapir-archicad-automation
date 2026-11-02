import aclib

# Read the dialog auto close state before changing it.
initialState = aclib.RunTapirCommand ('GetDialogAutoClose')
wasEnabled = bool (initialState and initialState.get ('enabled'))

# Let Archicad close its dialogs with their default button while the script works.
aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': True})
aclib.RunTapirCommand ('GetDialogAutoClose')

# Restore the state the user had before the script.
aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': wasEnabled})
aclib.RunTapirCommand ('GetDialogAutoClose')
