import aclib
import os
import tempfile

# The new file must not exist yet, and its folder must exist. The new part is only
# loaded when the folder is in a loaded library, e.g. a local library folder.
newLibraryPartPath = os.path.join (tempfile.gettempdir (), 'Tapir New Object.gsm')
if os.path.exists (newLibraryPartPath):
    os.remove (newLibraryPartPath)

# A subtype from the loaded libraries, by its fileName (with extension) as returned by
# GetAvailableLibraryParts. Replace it with a subtype loaded in your project.
ancestorSubtype = 'General GDL Object.gsm'

response = aclib.RunTapirCommand ('CreateLibraryPart', {
    'path': newLibraryPartPath,
    'ancestorSubtype': ancestorSubtype
}, debug = False)
print (response)
