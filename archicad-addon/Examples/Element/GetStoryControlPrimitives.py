"""Reads the story lines and their markers drawn in the first section with GetStoryControlPrimitives.

Needs at least one Section.
"""
import aclib

sections = aclib.RunTapirCommand ('GetDatabases', {'types': ['Section']}, debug = False)['databases']
if not sections:
    raise SystemExit ('No section in the project.')

aclib.RunTapirCommand ('GetStoryControlPrimitives', {'databaseId': sections[0]['databaseId']})
