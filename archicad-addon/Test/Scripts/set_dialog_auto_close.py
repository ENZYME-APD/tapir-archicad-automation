import aclib

# Archicad closes its dialogs with their default button while the auto close is on.
aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': True})

# Turn it off again, so the dialogs wait for the user.
aclib.RunTapirCommand ('SetDialogAutoClose', {'enabled': False})
