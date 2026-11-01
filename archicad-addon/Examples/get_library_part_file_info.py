import aclib

# Absolute paths of .gsm files; they are read without loading them into the project.
files = ['C:\\Library\\My Object.gsm']

response = aclib.RunTapirCommand ('GetLibraryPartFileInfo', {
    'files': files,
    # Leave out 'scripts' to read all five; give 'password' for protected parts.
    'scripts': ['master', '2d', '3d']
}, debug = False)

for file in response['files']:
    if 'error' in file:
        print (file['error']['message'])
        continue
    print (file['path'], file.get ('unID'), file.get ('subTypeUnID'))
    for key, script in file['scripts'].items ():
        if script['present']:
            print ('---', key, '---')
            print (script['script'])
        else:
            print ('---', key, '(not present) ---')
