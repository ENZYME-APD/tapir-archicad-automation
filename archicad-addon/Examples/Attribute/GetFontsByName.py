"""Finds the index of the Arial font, as Text and Label elements reference it, with GetFontsByName."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetFontsByName', {'fontNames': ['Arial']}))
