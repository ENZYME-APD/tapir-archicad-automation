"""Shows a non-blocking notification bubble in Archicad with ShowNotificationBubble and prints the bubble's settings.

Unlike ShowAlert, the script and the user go on at once; the bubble disappears after the timeout.
Without a coordinate or screenPoint it is shown at the center of the current 2D view. largeIcon is used
when "Use large alert cues" is on in Work Environment. The response is the bubble's text, icons, offset,
timeout and redraw count limit as Archicad reports them. redrawCountLimit and disabled (not used here)
stop the bubble from appearing until the user runs 'Enable all informational messages'.
Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ShowNotificationBubble', {
    'message': 'Tapir: the script has finished.',
    'icon': 'information',
    'timeoutMilliseconds': 3000,
    'largeIcon': 'information'
}))
