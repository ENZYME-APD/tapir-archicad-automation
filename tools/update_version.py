import os
import sys
import json
import codecs
import re

def ReplaceRegexInFile (filePath, oldRegex, newRegex):
    fileContent = None
    with codecs.open (filePath, 'r', 'utf-8') as fileObject:
        fileContent = fileObject.read ()
    if len (re.findall (oldRegex, fileContent)) != 1:
        return False
    fileContent = re.sub (oldRegex, newRegex, fileContent)
    with codecs.open (filePath, 'w', 'utf-8') as fileObject:
        fileObject.write (fileContent)
    return True

VERSION_PATTERN = r'[0-9]+\.[0-9]+\.[0-9]+'


def ReplaceRules (rootPath, version, versionPattern = VERSION_PATTERN):
    """The files the version lives in, each with the regex that finds it there and the one that writes it.

    A caller that only wants to know what the rules are - tools/tests/test_update_version.py, which
    checks they still match - gets them from here, so they are written once."""
    return [
        {
            'filePath' : os.path.join (rootPath, 'archicad-addon', 'Sources', 'AddOnVersion.hpp'),
            'oldRegex' : r'#define ADDON_VERSION "{0}"'.format (versionPattern),
            'newRegex' : r'#define ADDON_VERSION "{0}"'.format (version)
        },
        {
            'filePath' : os.path.join (rootPath, 'grasshopper-plugin', 'TapirGrasshopperPlugin', 'TapirGrasshopperPlugin.csproj'),
            'oldRegex' : r'<Version>{0}</Version>'.format (versionPattern),
            'newRegex' : r'<Version>{0}</Version>'.format (version)
        },
        {
            'filePath' : os.path.join (rootPath, 'grasshopper-plugin', 'TapirGrasshopperPlugin', 'TapirGrasshopperPluginInfo.cs'),
            'oldRegex' : r'AssemblyVersion\s*\=\>\s*"{0}"'.format (versionPattern),
            'newRegex' : r'AssemblyVersion => "{0}"'.format (version)
        },
        {
            'filePath' : os.path.join (rootPath, 'grasshopper-plugin', 'YakPackage', 'manifest.yml'),
            'oldRegex' : r'version: {0}'.format (versionPattern),
            'newRegex' : r'version: {0}'.format (version)
        }
    ]


def Main ():
    currentPath = os.path.dirname (os.path.abspath (__file__))
    rootPath = os.path.dirname (currentPath)
    os.chdir (rootPath)
    
    packageInfoJsonPath = os.path.join (currentPath, 'package_info.json')
    packageInfoJson = None
    with codecs.open (packageInfoJsonPath, 'r', 'utf-8') as packageInfoJsonFile:
        packageInfoJson = json.load (packageInfoJsonFile)
    
    version = packageInfoJson['version']
    replaceRules = ReplaceRules (rootPath, version)

    for replaceRule in replaceRules:
        filePath = replaceRule['filePath']
        oldRegex = replaceRule['oldRegex']
        newRegex = replaceRule['newRegex']
        if not os.path.exists (filePath):
            print ('ERROR: file does not exist: ' + filePath)
            return 1
        if not ReplaceRegexInFile (filePath, oldRegex, newRegex):
            print ('ERROR: version not found in file: ' + filePath)
            return 1
    
    return 0


if __name__ == '__main__':
    sys.exit (Main ())
