
from archicad import ACConnection
import archicad
import sys

conn = ACConnection.connect()

acc = conn.commands
act = conn.types
acu = conn.utilities

parameters = {}

obj = acc.GetSelectedElements()

obj = str(obj[0].elementId.guid)

parameters["command"] = "LibPart"
parameters["inParams"] = [obj]

response = acc.ExecuteAddOnCommand(act.AddOnCommandId('TapirCommand','LibraryParams'),parameters)
for out in response['outparams2']:
     print(out)
         
sys.exit()
