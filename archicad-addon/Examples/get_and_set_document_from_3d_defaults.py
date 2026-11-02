import aclib

# The defaults a newly created 3D Document starts with. Existing 3D Documents keep their
# own settings; these only apply to the ones created afterwards.
defaults = aclib.RunTapirCommand ('GetDocumentFrom3DDefaults')

# Office defaults for site/height drawings: vectorial hatching and sun shadows, no
# boundary contours. Only the given fields change.
aclib.RunTapirCommand ('SetDocumentFrom3DDefaults', {
    'vect3DHatching': True,
    'vectSunShadow': True,
    'boundaryDisplay': 'NoContours',
    'attributeOptions': {
        'shouldUseUniformCutAttributes': True
    }
})

aclib.RunTapirCommand ('GetDocumentFrom3DDefaults')

# Restore the original defaults.
aclib.RunTapirCommand ('SetDocumentFrom3DDefaults', defaults)
