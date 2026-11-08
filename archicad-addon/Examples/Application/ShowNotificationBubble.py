"""Shows a non-blocking notification bubble in Archicad with ShowNotificationBubble and prints the result.

Unlike ShowAlert, the script and the user go on at once; the bubble disappears after the timeout.
Without a coordinate or screenPoint it is shown at the center of the current 2D view. Requires Archicad 29 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ShowNotificationBubble', {
    'message': 'Tapir: the script has finished.',
    'icon': 'information',
    'timeoutMilliseconds': 3000
}))
