"""Reads the library part, GDL parameters and marker stored in the Door Favorites with GetFavoriteSettings.

Needs a Door Favorite.
"""
import aclib

doorFavorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Door'})['favorites']
if not doorFavorites:
    raise SystemExit ('The project has no Door Favorite.')

response = aclib.RunTapirCommand ('GetFavoriteSettings', {'favorites': doorFavorites[:3]})
for settings in response['favoriteSettings']:
    if 'error' in settings:
        print (settings['error']['message'])
    else:
        print (settings['favorite'], settings.get ('libPart', {}).get ('name'), len (settings['parameters']), 'parameters')
