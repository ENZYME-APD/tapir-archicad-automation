import os
import sys
import platform
import subprocess
import shutil
import codecs
import argparse
from pathlib import Path

class ResourceCompiler (object):
    def __init__ (self, devKitPath: Path, acVersion: str, languageCode: str, sourcesPath: Path, resourcesPath: Path, resourceObjectsPath: Path):
        self.devKitPath = devKitPath
        self.acVersion = acVersion
        self.languageCode = languageCode
        self.sourcesPath = sourcesPath
        self.resourcesPath = resourcesPath
        self.resourceObjectsPath = resourceObjectsPath
        self.resConvPath = None
        self.nativeResourceFileExtension = None

    def IsValid (self) -> bool:
        if self.resConvPath is None:
            return False
        if not self.resConvPath.exists ():
            return False
        return True

    def GetPrecompiledGRCResourceFilePath (self, grcFilePath: Path) -> Path:
        grcFileName = grcFilePath.name
        return self.resourceObjectsPath / f'{grcFileName}.i'

    def CompileLocalizedResources (self) -> None:
        locResourcesFolder = self.resourcesPath / f'R{self.languageCode}'
        grcFiles = locResourcesFolder.glob ('*.grc')
        for grcFilePath in grcFiles:
            assert self.CompileGRCResourceFile (grcFilePath), f'Failed to compile resource: {grcFilePath}'

    def CompileFixResources (self) -> None:
        fixResourcesFolder = self.resourcesPath / 'RFIX'
        grcFiles = fixResourcesFolder.glob ('*.grc')
        for grcFilePath in grcFiles:
            assert self.CompileGRCResourceFile (grcFilePath), f'Failed to compile resource: {grcFilePath}'

    def RunResConv (self, platformSign: str, codepage: str, inputFilePath: Path) -> bool:
        imageResourcesFolder = self.resourcesPath / 'RFIX' / 'Images'
        inputFileBaseName = inputFilePath.stem
        nativeResourceFilePath = self.resourceObjectsPath / (inputFileBaseName + self.nativeResourceFileExtension)
        colorChangeScriptPath = self.resConvPath.parent / 'SVGColorChange.py'
        call_params = [
            self.resConvPath,
            '-m', 'r',                      # resource compile mode
            '-T', platformSign,             # target platform
            '-q', 'utf8', codepage,         # code page conversion
            '-w', '2',                      # HiDPI image size list
            '-p', imageResourcesFolder,     # image search path
            '-i', inputFilePath,            # input path
            '-o', nativeResourceFilePath    # output path
        ]

        if (self.acVersion >= 29):
            call_params.extend (['-py', sys.executable])        # python executable
            call_params.extend (['-sc', colorChangeScriptPath]) # SVG color change script path for generating Dark Mode icons
        result = subprocess.call (call_params)
        if result != 0:
            return False
        return True

class WinResourceCompiler (ResourceCompiler):
    def __init__ (self, devKitPath: Path, acVersion: str, languageCode: str, sourcesPath: Path, resourcesPath: Path, resourceObjectsPath: Path):
        super (WinResourceCompiler, self).__init__ (devKitPath, acVersion, languageCode, sourcesPath, resourcesPath, resourceObjectsPath)
        self.resConvPath = devKitPath / 'Tools' / 'Win' / 'ResConv.exe'
        self.nativeResourceFileExtension = '.rc2'

    def GetPlatformDefine (self) -> str:
        return 'WINDOWS'

    def PrecompileGRCResourceFile (self, grcFilePath: Path) -> Path:
        precompiledGrcFilePath = self.GetPrecompiledGRCResourceFilePath (grcFilePath)
        result = subprocess.call ([
            'cl',
            '/nologo',
            '/X',
            '/EP',
            '/P',
            '/I', self.devKitPath / 'Inc',
            '/I', self.devKitPath / 'Modules' / 'DGLib',
            '/I', self.sourcesPath,
            '/I', self.resourceObjectsPath,
            '/D' + self.GetPlatformDefine (),
            '/source-charset:utf-8',
            '/execution-charset:utf-8',
            '/Fi{}'.format (precompiledGrcFilePath),
            grcFilePath,
        ])
        assert result == 0, f'Failed to precompile resource {grcFilePath}'
        return precompiledGrcFilePath

    def CompileGRCResourceFile (self, grcFilePath: Path) -> bool:
        precompiledGrcFilePath = self.PrecompileGRCResourceFile (grcFilePath)
        return self.RunResConv ('W', '1252', precompiledGrcFilePath)

    def GetNativeResourceFile (self) -> Path:
        defaultNativeResourceFile = self.resourcesPath / 'RFIX.win' / 'AddOnMain.rc2'
        if defaultNativeResourceFile.exists ():
            return defaultNativeResourceFile

        existingNativeResourceFile = next ((self.resourcesPath / 'RFIX.win').glob ('*.rc2'), None)
        assert existingNativeResourceFile is not None, 'Native resource file was not found at RFIX.win folder'

        return existingNativeResourceFile

    def CompileNativeResource (self, resultResourcePath: Path) -> None:
        nativeResourceFile = self.GetNativeResourceFile ()
        result = subprocess.call ([
            'rc',
            '/i', self.devKitPath / 'Inc',
            '/i', self.devKitPath / 'Modules' / 'DGLib',
            '/i', self.sourcesPath,
            '/i', self.resourceObjectsPath,
            '/fo', resultResourcePath,
            nativeResourceFile
        ])
        assert result == 0, f'Failed to compile native resource {nativeResourceFile}'

class MacResourceCompiler (ResourceCompiler):
    def __init__ (self, devKitPath: Path, acVersion: str, languageCode: str, sourcesPath: Path, resourcesPath: Path, resourceObjectsPath: Path):
        super (MacResourceCompiler, self).__init__ (devKitPath, acVersion, languageCode, sourcesPath, resourcesPath, resourceObjectsPath)
        self.resConvPath = devKitPath / 'Tools' / 'OSX' / 'ResConv'
        self.nativeResourceFileExtension = '.ro'

    def GetPlatformDefine (self) -> str:
        return 'macintosh'

    def PrecompileGRCResourceFile (self, grcFilePath: Path) -> Path:
        precompiledGrcFilePath = self.GetPrecompiledGRCResourceFilePath (grcFilePath)
        result = subprocess.call ([
            'clang',
            '-x', 'c++',
            '-E',
            '-P',
            '-D' + self.GetPlatformDefine (),
            '-I', self.devKitPath / 'Inc',
            '-I', self.devKitPath / 'Modules' / 'DGLib',
            '-I', self.sourcesPath,
            '-I', self.resourceObjectsPath,
            '-o', precompiledGrcFilePath,
            grcFilePath,
        ])
        assert result == 0, f'Failed to precompile resource {grcFilePath}'
        return precompiledGrcFilePath

    def CompileGRCResourceFile (self, grcFilePath: Path) -> bool:
        precompiledGrcFilePath = self.PrecompileGRCResourceFile (grcFilePath)
        return self.RunResConv ('M', 'utf16', precompiledGrcFilePath)

    def CompileNativeResource (self, resultResourcePath: Path) -> None:
        resultLocalizedResourcePath = resultResourcePath / 'English.lproj'
        if not resultLocalizedResourcePath.exists ():
            resultLocalizedResourcePath.mkdir (parents=True)
        resultLocalizableStringsPath = resultLocalizedResourcePath / 'Localizable.strings'
        resultLocalizableStringsFile = codecs.open (resultLocalizableStringsPath, 'w', 'utf-16')
        for fileName in self.resourceObjectsPath.iterdir ():
            filePath = self.resourceObjectsPath / fileName
            extension = fileName.suffix.lower ()
            if extension == '.tif':
                shutil.copy (filePath, resultResourcePath)
            elif extension == '.rsrd':
                shutil.copy (filePath, resultLocalizedResourcePath)
            elif extension == '.strings':
                stringsFile = codecs.open (filePath, 'r', 'utf-16')
                resultLocalizableStringsFile.write (stringsFile.read ())
                stringsFile.close ()
        resultLocalizableStringsFile.close ()

def Main (argv):
    parser = argparse.ArgumentParser (description = 'Archicad Add-On Resource Compiler.')
    parser.add_argument ('languageCode', help = 'Language code of the Add-On.')
    parser.add_argument ('acVersion', help = 'Archicad version the Add-On is building for.')
    parser.add_argument ('devKitPath', help = 'Path of the Archicad Development Kit.')
    parser.add_argument ('sourcesPath', help = 'Path of the sources folder of the Add-On.')
    parser.add_argument ('resourcesPath', help = 'Path of the resources folder of the Add-On.')
    parser.add_argument ('resourceObjectsPath', help = 'Path of the folder to build resource objects.')
    parser.add_argument ('resultResourcePath', help = 'Path of the resulting resource.')
    args = parser.parse_args ()

    currentDir = Path (__file__).parent
    os.chdir (currentDir)

    languageCode = args.languageCode
    acVersion = int(args.acVersion)
    devKitPath = Path (args.devKitPath)
    sourcesPath = Path (args.sourcesPath)
    resourcesPath = Path (args.resourcesPath)
    resourceObjectsPath = Path (args.resourceObjectsPath)
    resultResourcePath = Path (args.resultResourcePath)

    resourceCompiler = None
    system = platform.system ()
    if system == 'Windows':
        resourceCompiler = WinResourceCompiler (devKitPath, acVersion, languageCode, sourcesPath, resourcesPath, resourceObjectsPath)
    elif system == 'Darwin':
        resourceCompiler = MacResourceCompiler (devKitPath, acVersion, languageCode, sourcesPath, resourcesPath, resourceObjectsPath)

    assert resourceCompiler, 'Platform is not supported'
    assert resourceCompiler.IsValid (), 'Invalid resource compiler'

    resourceCompiler.CompileLocalizedResources ()
    resourceCompiler.CompileFixResources ()
    resourceCompiler.CompileNativeResource (resultResourcePath)

    return 0

sys.exit (Main (sys.argv))