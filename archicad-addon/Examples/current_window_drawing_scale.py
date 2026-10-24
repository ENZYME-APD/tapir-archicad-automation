import aclib

# The drawing scale of the window that is open right now (the 1:100 in the bottom bar),
# without a saved view: GetViewSettings / SetViewSettings only see the scale of saved views.
# Each floor plan, section, elevation, detail and worksheet window has its own scale;
# layouts are always 1:1 and cannot be changed.
original = aclib.RunTapirCommand ('GetCurrentWindowDrawingScale')
originalScale = original['scale']

# Switch to 1:50 (the parameter is the denominator of the scale) and read it back.
aclib.RunTapirCommand ('SetCurrentWindowDrawingScale', {'scale': 50})
aclib.RunTapirCommand ('GetCurrentWindowDrawingScale')

# Restore the original scale, this time rebuilding the window afterwards.
aclib.RunTapirCommand ('SetCurrentWindowDrawingScale', {'scale': originalScale, 'rebuild': True})
aclib.RunTapirCommand ('GetCurrentWindowDrawingScale')
