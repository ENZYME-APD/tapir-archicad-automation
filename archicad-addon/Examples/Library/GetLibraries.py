"""Lists the libraries loaded in the project with GetLibraries: name, type, path and, from Archicad 28, package information."""
import aclib

libraries = aclib.RunTapirCommand ('GetLibraries')['libraries']
for library in libraries:
    print (library['type'], library['name'], library['path'])
