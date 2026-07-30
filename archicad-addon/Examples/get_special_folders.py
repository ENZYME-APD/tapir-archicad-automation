import aclib

response = aclib.RunTapirCommand ('GetSpecialFolders', {
    'folderTypes': [
        'ApplicationPrefs',
        'GraphisoftPrefs',
        'GraphisoftHome',
        'Cache',
        'Data',
        'UserDocuments',
        'Temporary',
        'Application',
        'Defaults',
        'WebObjects',
        'Templates',
        'Help',
        'EmbeddedProjectLibrary',
        'EmbeddedProjectLibraryHotlink',
        'ProjectPreviews'
    ]
})
