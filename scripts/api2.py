import json
import urllib2 as rq
from collections import OrderedDict
import  xml.etree.ElementTree as e
# import requests
try: 
	import scriptcontext as sc
except:
	pass
	# print ('No sticky')

# """
# Archicad API 3.0 for grasshopper nodes and ARCHICAD 25 Python pallete

# """

def composeAddonCommand(commandName,commandParams={}):

	# '''
	# Composes json command for Custom Addon: AdditionalJsonCommands
	# if commands requires parameters provide it in form of dictionary
	# Parametername:value
	# '''
	json_command = {}
	json_command['command'] = 'API.ExecuteAddOnCommand'
    

	addOnCommandId  ={}
	addOnCommandId['commandNamespace'] = 'AdditionalJSONCommands'
	addOnCommandId['commandName'] = commandName


	addOnCommandParameters = {}    
	if len(commandParams) > 0:
		_parameters={}
		for command,value in commandParams.items():
			_parameters[command] = value
		addOnCommandParameters = _parameters
		json_command['parameters'] = {'addOnCommandId':addOnCommandId,'addOnCommandParameters':addOnCommandParameters}
	else:
		json_command['parameters'] = {'addOnCommandId':addOnCommandId}
	return json_command

def extract_xml_from_bimx(bimxFile):
	with open(bimxFile,'r') as file:
	# with open(bimxFile,'r',encoding='utf-8') as file:
		data = file.readlines()
	file.close()


	for i,j in enumerate(data):
		# print (i,j)
		if "<Metadata" in j:
			a = i 
		if "</Metadata" in j:
			b = i+1

	block = (a,b)
	xml = []
	for i in xrange(block[0],block[1]):
		xml.append(data[i])
		xmlString = ''.join(xml)
	return xmlString

def parseXML(xmlString):
	# parser = e.XMLParser(strip_cdata = False)
	# xmlParsed = e.parse(xmlString,parser)
	xmlParsed = e.fromstring(xmlString)
	root = xmlParsed
	return root

def addItem(array,item):
    array.append(item)
    
def openFolder(array,folder):
    for f in folder:
        if f.tag=='Item':addItem(array,f)
        if f.tag=='Folder':openFolder(array,f)

def getLayouts(itemsAndFolders):
    _layoutName = []
    _layoutID   = []
    _items= []
    for j,i in enumerate(itemsAndFolders[0]):
        if i.attrib.has_key('type') == False:
            if i.tag=='Item':addItem(_items,i)
            if i.tag=='Folder':openFolder(_items,i)
    for item in _items:
        
        _layoutName.append(item.attrib['title'])
        _layoutID.append(item.attrib['id'])
    sc.sticky['layoutNames'] = _layoutName
    sc.sticky['layoutID'] = _layoutID
    return _layoutName,_layoutID

def get_layouts():
	"""get bimx llayouts from sticky global"""
	try:
		_layoutName = sc.sticky['layoutNames']
		_layoutID = sc.sticky['layoutID']
		
		return dict(zip(_layoutName,_layoutID))
	except:
		return False
def set_global_toggle(on_off):
	"""
	global turnoff/on variable
	INPUT:
		on_off: list of integers
	"""
	try:
		if isinstance(on_off,bool):
			sc.sticky['toggle'] = on_off
		else: 
			sc.sticky['toggle'] = False
	except:
		print 'Run outside GH'


def get_global_toggle():
	"""Gets global toggle"""
	try:
		_toggle = sc.sticky['toggle']
		return _toggle
	except:
		return False

def set_active_port(ports):
	''' 
	'''
	if isinstance(ports,int):
		ports=[ports]
	try:
		sc.sticky['activeports'] = ports
	except:
		print 'Run outside GH'
def get_active_port():
	try:
		return sc.sticky['activeports']
	except:
		return [19723]

def set_ports(ports):
	"""
	Saves ports to GH Sticky Global variable accessible to any node in a scheme
	INPUT:
		ports: list of integers
	"""
	try:
		sc.sticky['ports'] = ports
		print('ports set')
	except:
		print 'Run outside GH'

def get_port():
	"""Gets list of ports saved in  GH sticky global variable or false if run outside GH """
	try:
		_ports = sc.sticky['ports']
		if isinstance(sc.sticky['ports'],int):
			_ports = [sc.sticky['ports']]
		return _ports
	except:
		return False



def connect(json_definition,port):
	"""
	Connects to ARCHICAD API
	"""
	try:
		request = rq.Request ('http://127.0.0.1:{}'.format(port))
		response = rq.urlopen (request, json.dumps (json_definition).encode ("UTF-8"))
		Result = json.loads (response.read ())
		return Result
	except :
		return False

def check_port(port):
	"""
	Checks if port is Alive
	"""
	command = {
	"command": "API.IsAlive"
	}
	try:
		resp_to_json  = connect(command,port)
		isSuccess = resp_to_json['succeeded']
	# print ('Status:{0}'.format(isSuccess))s
	# if isSuccess == False:
	except:
		# print ('Port not opened')
		isSuccess = False
		# print ('Error {0} - {1}'.format(resp_to_json['error']['code'],resp_to_json['error']['message']))
	return isSuccess

def open_ports():
	"""
	Scans scopes of ports and return list of open ports (open ARCHICAD copies)
	"""
	scope = (19723,19726)
	# scope = (19723,19743)
	ports = [i for i in range(scope[0],scope[1]+1)if check_port(i)]
	set_ports(ports)
	return ports

	


def pack(name,dictionary):
	"""
	Packing dictionary to json and dumps it to string
	INPUT:
		name: key name
		dictionary: data set to be send
	"""
	json_def = {name:dictionary}
	return json.dumps(json_def)
def packlist(_list):
	"""
	Packing list to json and dumps it to string
	INPUT:
		name: key name
		dictionary: data set to be send
	"""
	json_def = _list
	return json.dumps(json_def)

def unpack(json_def):
	"""
	Unpacking dumped json string into dictionary
	INPUT:
		json_def: string to be parsed into dictionary
	"""
	return json.loads(json_def)



# # wrapping_index
def get_wrapped_value(current_index,lista):
	'''
	wraps current_value to fit a lenght of a list
	'''
	wrapped_index = 0
	if len(lista):
		multiplication = int(current_index/len(lista)) 
		wrapped_index = current_index - (multiplication*len(lista))
	return int(wrapped_index)



def show_menu(header_name,lista,current_index):
	'''
		draws menu 
	'''
	header = '''
---------------
{}
---------------
	'''.format(header_name)



	rows=[]
	rows.append(header)	
	for i,j in enumerate(lista):
		
		if i == current_index:
			arrow = '<--pick ({})'.format(j)
		else:
			arrow = ''
		rows.append('{} | {}     {}'.format(i,j,arrow))

	show_content = '\n'.join(rows)
	# print show_content
	return show_content

def show_menu_two_inputs(header_name,lista,current_index,current_index2):
	'''
		draws menu 
	'''
	header = '''
---------------
{}
---------------
	'''.format(header_name)



	rows=[]
	rows.append(header)	
	for i,j in enumerate(lista):
		
		if i == current_index:
			arrow = '<--IdField ({})'.format(j)
		else:
			arrow = ''
		if i == current_index2:
			arrow2 = '<--ValueField ({})'.format(j)
		else:
			arrow2 = ''
		rows.append('{} | {}     {}     {}'.format(i,j,arrow,arrow2))

	show_content = '\n'.join(rows)
	# print show_content
	return show_content


def getBuiltInPropertiesOfType(builtInPropertyNames,port,propType = 'General'):
	"""
	Gets BuiltIn properties of Type
	input:
		propType: property type
		port: active port
	"""
	propsOfType =[]
	for prop in builtInPropertyNames:
		if propType in prop:
			propsOfType.append(prop)
	return propsOfType

def getBuiltInPropertyTypes(port):
	"""
	Gets BuiltIn property Types for grouping
	input:
		port: active port
	"""

	BuiltInPropertyTypes=[]
	json_definition = {
	"command": "API.GetAllPropertyNames"
	}

	result =  connect(json_definition,port)

	for _property in result['result']['properties']:
		if 'BuiltIn' in _property['type']:
			string= _property['nonLocalizedName']
			BuiltInPropertyTypes.append(string.split('_')[0])
		BuiltInPropertyTypes = list(set(BuiltInPropertyTypes))
	return BuiltInPropertyTypes

def getBuiltInPropertyNames(port):
	'''
	Gets list of 'BuiltIn' property names
	input: 
		port: active port 
	'''
	BuiltInPropertyNames =[]
	json_definition = {
    "command": "API.GetAllPropertyNames"
	}

	result =  connect(json_definition,port)
	for _property in result['result']['properties']:
		if 'BuiltIn' in _property['type']:
			string= _property['nonLocalizedName']
			BuiltInPropertyNames.append(string)
	return BuiltInPropertyNames

def getUserDefinedPropertyId(group,propertyName,port):
	'''
	input: 
		property name,
		port
	output:
		guid <str> of property
	'''
	command = {
	    "command": "API.GetPropertyIds",
	    "parameters": {
	        "properties": [
	            {
	                "type": "UserDefined",
	                "localizedName": [group,propertyName]
	        	}
	        ]
	            
	    }
	}

	id = connect(command,port)

	return id['result']['properties'][0]['propertyId']['guid']


def getBuiltInPropertyId(propertyName,port):
	'''
	input: 
		property name,
		port
	output:
		guid <str> of property
	'''
	command = {
	    "command": "API.GetPropertyIds",
	    "parameters": {
	        "properties": [
	            {
	                "type": "BuiltIn",
	                "nonLocalizedName": propertyName
	        	}
	        ]
	            
	    }
	}

	id = connect(command,port)

	return id['result']['properties'][0]['propertyId']['guid']


def getUserDefinedPropertyGroups(port):
	"""
	Get all avaliable user defined property groups
	INPUT:
		port
	Output (2 variables):
		groups: list of groups names
		properties: dictionary {group:list of properties names} 

	"""
	json_definition = {
    "command": "API.GetAllPropertyNames"
	}

	result =  connect(json_definition,port)
	userDefGroups =[]
	userDefProps =[]
	for i in result['result']['properties']:
		if i['type'] == 'UserDefined':
			userDefGroups.append(i['localizedName'][0])
			userDefProps.append(i['localizedName'])

	userDefGroups = list(set(userDefGroups))

	group_props={}
	for group in userDefGroups:
		_props=[]
		for prop in userDefProps:
			if prop[0] == group:
				_props.append(prop[1])
		group_props[group] = _props

	# return userDefProps
	# return userDefGroups
	return userDefGroups,group_props

def getUserDefinedPropertiesOfGroup(port,group):
	"""
	Get all avaliable user defined properties in group
	INPUT:
		port int port avaliable
		group: string name
	Output: list of properties in provided group
	"""
	print "props in group: {}".format(group)


def getPropertyValue(elements,propertyId,port):
	'''
	gets property value
	'''
	_elements = elements#[0]
	_propertyId = propertyId#[0]

	_el = []
	_prop = []
	for i in _elements:

		_el.append(
			{
     		 "elementId": 
     		 	{
             	"guid": i
                }
			}
			)

	for i in _propertyId:

		_prop.append(
			{
     		 "propertyId": 
     		 	{
             	"guid": i
                }
			}
			)

	json_definition = {
        "command": "API.GetPropertyValuesOfElements",
        "parameters": {
            "elements":_el,
            "properties": _prop
        }
	}

	result = connect(json_definition,port)
	val=[]
	
	if result['succeeded']:
		for i in result['result']['propertyValuesForElements']:
			# print i['propertyValues'][0]['propertyValue']
			__val = i['propertyValues'][0]['propertyValue']
			val.append(__val)
		return val
	else:
		print result['error']['message']
		return False
	
	# return result['result']
def getElementsTypes():
	types = [
	"Wall",
	"Column",
	"Beam",
	"Window",
	"Door",
	"Object",
	"Lamp",
	"Slab",
	"Roof",
	"Mesh",
	"Zone",
	"CurtainWall",
	"Shell",
	"Skylight",
	"Morph",
	"Stair",
	"Railing",
	"Opening"
	]
	return types

def getAllElements(port):
	'''
	'''
	command = {
		"command": "API.GetAllElements"
		}

	result = connect(command,port)
	if result['succeeded']:
		elements=[]
		for el in result['result']['elements']:
			# pass
			elements.append(el['elementId']['guid'])
		return json.dumps({'elements':elements})
	else:
		return False

def getElementsByType(elementType,port):
	command = {
		"command": "API.GetElementsByType",
			"parameters": {"elementType": elementType
			}
		}
	result = connect(command,port)
	elements=[]
	for e in result['result']['elements']:
		elements.append(e['elementId']['guid'])
	_elements = json.dumps({'type':elementType,'elements':elements})
	# _elements = json.dumps({'elements':elements})
	return _elements


def getClassificationSystems(port):
	"""
	"""
	jsn = {
	"command": "API.GetAllClassificationSystems"
	}
	result = connect(jsn,port)
	systems = [name for name in result['result']['classificationSystems']]
	names = [name['name'] for name in systems]
	guids = [guid['classificationSystemId']['guid'] for guid in systems]
	return dict(zip(names,guids))

def convert_types(value,type_name):
	# print value,type_name
	if type_name=='number':return float(value)
	if type_name=='string':return str(value)
	if type_name=='integer':

		return int(value)
	if type_name=='boolean':return bool(value)
	if type_name=='area':return float(value)
	if type_name=='volume':return float(value)
	if type_name=='angle':return float(value)
	if type_name=='length':return float(value)

def prop_type(value,other_types=''):
	if other_types=='':
		if isinstance(value, str):
			if value == 'True'	 or value == 'False':
				return'boolean'
			else:
				return'string'.lower()
		if isinstance(value, int):return'integer'
		if isinstance(value, float):return'number'
		if isinstance(value, dict):return'singleEnum'
		if isinstance(value, list):return'multiEnum'
	if other_types=='area':return'area'
	if other_types=='volume':return'volume'
	if other_types=='length':return'length'
	if other_types=='angle':return'angle'
	if other_types=='boolean':return'boolean'

def setPropertyValue(e,p,v,port):


	json_def = {}
	json_def['command'] =  "API.SetPropertyValuesOfElements"
	json_def['parameters'] =  {}

 	item = {}

 	
 	
 	items_list =[]

 	k=0

 	for i in e:
		
 		# items_list.append({'elementId':{'guid':i},'propertyId':{'guid':p},'propertyValue':v})
 		items_list.append({'elementId':{'guid':i},'propertyId':{'guid':p},'propertyValue':{'type':v[k]['type'],'status':v[k]['status'],'value':v[k]['value']}})
 		k=k+1

	json_def['parameters']['elementPropertyValues'] =  items_list
	result = connect(json_def,port)
	# print json_def
	# print result['succeeded']		

	return 'Set new Parameter value to {0} elements:{1} '.format(len(e),str(result['succeeded']))
	# # return result
	# # return json.dumps(json_def,indent=4,sort_keys=True)



#-------- CLASSIFICATION SYSTEM

def getAllClassificationSystems(port):
	json_def = {}
	json_def['command'] =  "API.GetAllClassificationSystems"
	result = connect(json_def,port)
	try:
		response = result['succeeded']
	except:
		response = result
	if response:
		_system_details = result['result']['classificationSystems']
		_systems = []
		for i in _system_details:
			_system  = {}	
			_system['name']			= i['name']
			_system['source']		= i['source']  
			_system['version']		= i['version']  
			_system['date']			= i['date']  
			_system['description']	= i['description']  
			_system['guid']			= i['classificationSystemId']['guid']  
			_systems.append(_system)
		return _systems
	else:
		return False

def GetAllClassificationsInSystem(port,_classSystemID):
	json_def = {}
	json_def['command'] =  "API.GetAllClassificationsInSystem"
	json_def['parameters'] =  {
		"classificationSystemId": {
            "guid": _classSystemID
		}
	}
	result = connect(json_def,port)

	try:
		response = result['succeeded']
	except:
		response = result
		return['message']['error']
	if response:
		return result['result']['classificationItems']


# def SetClassificationsOfElements():
def SetClassificationsOfElements(port,_elements,_systemID,_classificationID):
	json_def = {}
	json_def['command'] =  "API.SetClassificationsOfElements"
	_elementClassifications = []
	for _el in _elements:
		_element_set = OrderedDict()
		_element_set["elementId"]={"guid":_el}
		_element_set["classificationId"]={
			"classificationItemId":		{"guid":_classificationID},
			"classificationSystemId":	{"guid":_systemID}
		}
		# _element_set["classificationSystemId"]={"guid":_systemID}
		
		_elementClassifications.append(_element_set)

	json_def['parameters'] = {"elementClassifications":_elementClassifications}

	# result = connect(json_def,port)
	
	return json_def

def GetElementsByClassification(port,_classificationID):
	json_def = {
    "command": "API.GetElementsByClassification",
    "parameters": {
        "classificationItemId": {
            "guid": _classificationID
	        }
	    }
	}
	result = connect(json_def,port)
	if result['succeeded']:
		elements=[]
		for el in result['result']['elements']:
			# pass
			elements.append(el['elementId']['guid'])
		return json.dumps(elements)
	else:
		return False
def attributes_names():
	att = [
	'BuildingMaterial',
	'Composite',
	'Fill',
	'Layer',
	'LayerCombination',
	'Line',
	'PenTable',
	'Profile',
	'Surface',
	'ZoneCategory'
	]
	return att
def GetAttributes(att_type,guid):

	commands = {
	'BuildingMaterial':"API.GetBuildingMaterialAttributes",
	'Composite':"API.GetCompositeAttributes",
	'Fill':"API.GetFillAttributes",
	'Layer':"API.GetLayerAttributes",
	'LayerCombination':"API.GetLayerCombinationAttributes",
	'Line':"API.GetLineAttributes",
	'PenTable':"API.GetPenTableAttributes",
	'Profile':"API.GetProfileAttributes",
	'Surface':"API.GetSurfaceAttributes",
	'ZoneCategory':"API.GetZoneCategoryAttributes"
	}


	_guids=[]
	if isinstance(guid,str) or isinstance(guid,unicode):
		guid = [guid]


	for g in guid:
		_guids.append({"attributeId": {"guid": g}})

	att_def= {
    "command": commands[att_type],
    "parameters": {"attributeIds": _guids}
    # "parameters": {"attributeIds": [{"attributeId": {"guid": guid}}]}
    }
	return att_def

def GetAttributesByType(att_type,port):


	json_def = {
    "command": "API.GetAttributesByType",
    "parameters": {
        "attributeType": att_type
    	}
	}
	result = connect(json_def,port)
# 	result = connect(json_def,port)
	ids =[id['attributeId']['guid'] for id in result['result']['attributeIds']]
	return ids
	# print ids
# 	js = GetAttributes(att_type,ids[0])
# 	att = connect(js,19723) 
	
	# return att['result']['attributes'][0]

def getAttributeDetails(attr,port):
	_surface_guid = api.GetAttributesByType(attr,port)
	f = api.GetAttributes(attr,_surface_guid)
	result = api.connect(f,port)
	return result['result']


if __name__ == '__main__':

	# print r

	for attr in attributes_names():
	# attr = 'Fill'
		attrByType = GetAttributesByType(attr, 19723)
		c = GetAttributes(attr,attrByType[0])
		r=connect(c,19723)
		print '-'*10 + attr
		print r
# 	j = {
#     "command": "API.GetAttributesByType",
#     "parameters": {
#         "attributeType": "Layer"
#     }
# }
# 	layers=[]
# 	r= connect(j,19723)
# 	for i in r['result']['attributeIds']:
# 		layers.append(i['attributeId']['guid'])
# 	i = 0 
# 	for i in range(len(layers)):
# 		f= {
# 	    "command": "API.GetLayerAttributes",
# 	    "parameters": {
# 	        "attributeIds": [
# 	            {
# 	                "attributeId": {
# 	                    "guid": layers[i]
# 	                }
# 	            }
# 	        ]
# 	    }
# 	}
# 		rr =connect(f,19723)
# 		print rr['result']['attributes'][0]['layerAttribute']['name']
 	# print prop
# # class Wrap():
# # 	"""
# # 	Wrap type and guid to an object to send it wherever
# # 	"""
# # 	def __init__(self,typ,guid,value=None):
# # 		self.Type = typ
# # 		self.Guid = guid
# # 		self.Value = value


	




# # def get_element_type_guid(element):
# # 	# if isinstance(element, str) ==False:
# # 	element_dict = {element.Type:element.Guid}
# # 	return element_dict











# # def userDefined_properties(Result):
# # 	userdefined_properies ={}
# # 	groups=[]
# # 	properties=[]
# # 	for i in Result['result']['properties']:
# # 		if 'UserDefined' in i['type']:
# # 			groups.append(i['localizedName'][0])
# # 			properties.append(i['localizedName'][1])
# # 			# print i['localizedName'][0],'  |  ',i['localizedName'][1]
# # 	groups = set(groups)
# # 	groups = list(groups)




# # def builtIn_properties(Result):
# # 	builtin_properties =[]
# # 	for i in Result['result']['properties']:
# # 		if 'BuiltIn' in i['type']:
# # 			builtin_properties.append(i['nonLocalizedName'])


# # 	typ = []
# # 	for j in builtin_properties:
# # 		typ.append(j.split('_')[0])
# # 	typ = list(set(typ))
# # 	typ.sort()

# # 	builtin_types = {}
# # 	for _typ in typ:
# # 		props_of_type =[]
# # 		for prop in builtin_properties:
# # 			if prop.split('_')[0] == _typ:
# # 				props_of_type.append(prop)
# # 		builtin_types[_typ] = props_of_type

# # 	return builtin_types




# # def inputNormalizer(elementInput):
# # 	'''
# # 	normalise two kind of element inputs:
# # 	1. AC_Type passed by contener node
# # 	2. archicad_api Wrap class
# # 	'''
# # 	t = str(elementInput[0].Type)
# # 	print 't: ',t 
# # 	splited = t.split('_')
# # 	if len(splited) > 1:
# # 		name = splited[1]
# # 		_elements = [str(el.Guid).upper() for el in elementInput]
# # 		# print 'contaner',_elements
# # 		return Wrap(name,_elements)
# # 	else:
# # 		name = splited[0]
# # 		# _elements = [el.Guid for el in elementInput]
# # 		_elements = elementInput[0].Guid
# # 		# print 'wrap',_elements
# # 		return Wrap(name,_elements)





# # def AddInputParam(name,desc='Description'):
# #     param = kernel.Parameters.Param_String()
# #     param.NickName = name
# #     param.Name = name
# #     param.Description = desc
# #     param.Access = kernel.GH_ParamAccess.list
# #     index = self.Params.Input.Count
# #     self.Params.RegisterInputParam(param,index)
# #     self.Params.OnParametersChanged()



# # def AddOutputParam(name,desc='Description'):
# #     param = kernel.Parameters.Param_String()
# #     param.NickName = name
# #     param.Name = name
# #     param.Description = desc
# #     param.Access = kernel.GH_ParamAccess.list
# #     index = self.Params.Output.Count
  
# #     self.Params.RegisterOutputParam(param,index)
# #     self.Params.OnParametersChanged()


	
# 		