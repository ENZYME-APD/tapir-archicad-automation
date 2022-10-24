

ghenv.Component.Name = 'Get Classifications In System'
ghenv.Component.NickName = 'GetClassificationsInSystem'
ghenv.Component.Message = '0.1'
ghenv.Component.Category = 'ARCHICAD_API'
ghenv.Component.SubCategory = 'Classification'
ghenv.Component.Description = '''
Collects all classifications systems
INPUT:
	PickSystem    : int  		: Picks Property group
						  

OUTPUT:
	ClassificationSystems 	    : Message 	 
	Name 						: String
	Name 						: String
'''

#https://archicadapi.graphisoft.com/JSONInterfaceDocumentation/#GetAllClassificationsInSystem

import  api2 as api 
ports = api.get_active_port()
port = ports[0]

classifications_menu=[]
className 	= []
classId 	= []
classDesc 	= []
classGUID 	= []
if len(ports)>0 and PickClass is not None:
	# for port in ports:
	classes = api.GetAllClassificationsInSystem(port,ClassSystemID)
	#print(classes[0])

seed = classes
output = []
level = 0
def get_ids(seed,kids = 0 ):
    global level
    level += 1
    for item in seed:
        output.append( '   '*(level-1) +  item['classificationItem']['id'])
        classId.append(item['classificationItem']['id'])
        className.append(item['classificationItem']['name'])
        classDesc.append(item['classificationItem']['description'])
        classGUID.append(item['classificationItem']['classificationItemId']['guid'])
        
        if kids >0:
            kids -= 1
        
        if 'children' in item['classificationItem'].keys():
            depth = len(item['classificationItem']['children'])
            get_ids(item['classificationItem']['children'], kids = depth)
        else:
            if kids == 0:
                pass # Debug only
    level -= 1
    
get_ids(seed, kids = 0 )