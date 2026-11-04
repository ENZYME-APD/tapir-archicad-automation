"""Turns on vectorial sun shadows for new 3D Documents with SetDocumentFrom3DDefaults, then restores the defaults.

Existing 3D Documents are not changed.
"""
import aclib

original = aclib.RunTapirCommand ('GetDocumentFrom3DDefaults')

# Only the given fields change.
aclib.PrintJson (aclib.RunTapirCommand ('SetDocumentFrom3DDefaults', {'vectSunShadow': True}))
aclib.RunTapirCommand ('GetDocumentFrom3DDefaults')

aclib.PrintJson (aclib.RunTapirCommand ('SetDocumentFrom3DDefaults', original))
