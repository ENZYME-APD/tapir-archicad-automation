"""Prints the paths of Archicad's preferences, temporary and templates folders with GetSpecialFolders."""
import aclib

aclib.RunTapirCommand ('GetSpecialFolders', {
    'folderTypes': ['ApplicationPrefs', 'Temporary', 'Templates']
})
