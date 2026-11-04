"""Prints the keywords of the autotexts available in dimension notes with GetNoteAutoTextKeywords.

Requires Archicad 26 or later.
"""
import aclib

noteAutotexts = aclib.RunTapirCommand ('GetNoteAutotexts', {'noteOwnerType': 'Dimension'})['noteAutotexts']
if not noteAutotexts:
    raise SystemExit ('No note autotexts found.')

aclib.PrintJson (aclib.RunTapirCommand ('GetNoteAutoTextKeywords', {
    'autoTextIds': [a['autotextId'] for a in noteAutotexts]
}))
