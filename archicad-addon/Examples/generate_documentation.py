import os
import aclib

rootFolder = os.path.dirname (os.path.dirname (os.path.dirname (os.path.abspath (__file__))))
docFolder = os.path.join (rootFolder, 'docs', 'archicad-addon')

commandName = 'GenerateDocumentation'
commandParameters = {
    'destinationFolder' : docFolder
}

response = aclib.RunTapirCommand (commandName, commandParameters)
