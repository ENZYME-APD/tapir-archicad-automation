# - - - - IMPORTS

import os, platform, shutil

# - - - - GLOBALS

target_files = ['api2.py']
user_object_folder_name = 'archicad_api'
rhino_version = '7.0'

# - - - - RUNSCRIPT

def get_target_path():
    os_name = platform.system()
    if os_name == 'Darwin':
        return os.path.join(os.environ['HOME'],'Library/Application Support/McNeel/Rhinoceros', rhino_version, 
    'scripts')
        return r''
    elif os_name == 'Windows':
        return  os.path.join(os.environ['APPDATA'], 'McNeel', 'Rhinoceros', rhino_version, 'scripts')
    else:
        raise ValueError('Unsupported operating system')


if __name__ == '__main__':

    source_directory = os.path.dirname(os.path.abspath(__file__))
    target_directory = get_target_path()

    for file in target_files:
        source = os.path.join(source_directory, file)
        target = os.path.join(target_directory, file)
        shutil.copy2(source, target)
        print('{} copied to {}'.format(file, target_directory))
    