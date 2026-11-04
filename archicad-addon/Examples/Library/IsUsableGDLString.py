"""Checks with IsUsableGDLString whether a few strings can be used in GDL, e.g. before writing them into a GDL parameter."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('IsUsableGDLString', {
    'strings': ['Plain text', 'Quote " inside', 'Line\nbreak']
}))
