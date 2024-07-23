#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['Link', 'Parameter', 'Command', 'CommandResult']

# - - - - - - - - BUILT-IN IMPORTS
import json
import sys
if sys.version_info.major == 3:
    from urllib.request import urlopen, Request
else:
    from urllib2 import urlopen, Request

# - - - - - - - - LOCAL IMPORTS
if sys.version_info.major == 3:
    from .utility import dotNETBase, JsonExtensions, RuntimeObject, debug
    from .parts import Element, ClassificationSystem, BoundingBox, ClassificationItem
else:
    from utility import dotNETBase, JsonExtensions, RuntimeObject, debug
    from parts import Element, ClassificationSystem, BoundingBox, ClassificationItem

# - - - - - - - - CLASS LIBRARY
class Link(dotNETBase):
    """Represents a Link to ArchiCAD, through which all commands are executed.
    
    Args:
        port (int): A valid port number between 19723 and 19743,
    
    """
    _PORT = range(19723, 19743)
    _HOST = 'http://127.0.0.1'
    
    @property
    def port(self):
        """:int: Port number this instance uses.

        Raises:
            Exception: If port is invalid.
        
        """
        return self._port

    @port.setter
    def port(self, port):
        if Link.is_valid(port):
            self._port = port
        else:
            raise Exception('Invalid port')
    
    @property
    def host(self):
        """str: Default localhost address.
        
        """
        return self._HOST

    @property
    def address(self):
        """str: Network address that is used by the Link instance.
        
        """
        return '{}:{}'.format(self.host, self.port)
    
    def __init__(self, port=19723):
        self.port = port

    @staticmethod
    def is_valid(port):
        """ Checks if the port is valid.
        
        Args:
            port (int): A valid port number between 19723 and 19743,
        
        Returns:
            bool: True if successful, False otherwise.
        
        """
        return port in Link._PORT

    @staticmethod
    def get_active_ports():
        active = []
        for port in Link._PORT:
            
            if Link(port).is_active():
                active.append(port)
            else:
                return active

    def is_alive(self):
        """Checks if a connection can be established with ArchiCAD.
    
        Returns:
            bool: True if successful, False otherwise.
        
        """
        return Command(self).IsAlive()

    @debug
    def post(self, data):
        """Sends data to ArchiCAD using the POST method.

        Args:
            data (dict): Data to be sent.
        
        Returns:
            CommandResult: Response from ArchiCAD.
        
        """
        connection_object = Request(self.address)
        connection_object.add_header('Content-Type', 'application/json')
        try:
            response = urlopen(connection_object, json.dumps(data).encode('utf8'))
            return CommandResult(response.read())
        except:
            return None

    def __str__(self):
        return 'Link : {}'.format(self.address)

class CommandResult(dotNETBase):

    @staticmethod
    def _format_response(data):
        remove_key = "addOnCommandResponse"
        
        # Check if command belonged to aAddOn.
        if remove_key in data["result"]:
            for key in data["result"][remove_key].keys():
                data["result"][key] = data["result"][remove_key][key]
        
            data["result"].pop(remove_key)
        return data
    
    def _unpack_reponse(self, response):
        #TODO: Check if deepcopy is necessary here
        data = json.loads(response, object_hook=JsonExtensions.strip_unicode)
        return CommandResult._format_response(data)

    def __init__(self, response):
        self._data = self._unpack_reponse(response)
        self.success = self._data.get('succeeded')
        self.result = self._data.get('result', {})
        self.error = self._data.get('error', {})

    def get_result(self):
        if not self.success:
            if self.error is not None:
                raise Exception('{}:\n{}'.format(self.error['code'], self.error['message']))
            else:
                raise Exception('Unknown error')
        return self.result

    def __str__(self):
        return '{} CommandResult'.format('Success' if self.success else 'Failed')

class Parameter(dotNETBase):

    @staticmethod
    def pack(parameters):
        if all([isinstance(param, Parameter) for param in parameters]):
            params = {}
            for param in parameters:
                params[param.key] = param.value
            return params
    
    def __init__(self, key, value):
        self.key = key
        self.value = value
    
    def ToDictionary(self):
        return {'parameters' : {self.key: self.value}}
    
    def __str__(self):
        return '<{} : {}>'.format(self.key, self.value)

class Command(dotNETBase):
    
    @staticmethod
    def FormatCommand(command, parameters = ""):
        cmd = {'command' : command}
        if parameters !="":
            cmd['parameters'] = parameters
        return cmd

    @staticmethod
    def FormatAddOnCommand(command, parameters = ""):
        cmd = {
        'command' : 'API.ExecuteAddOnCommand',
        'parameters': {
            'addOnCommandId': {
                'commandNamespace': 'TapirCommand',
                'commandName': command
            },
            'addOnCommandParameters': parameters }
        }
        return cmd
    
    @classmethod
    def create(cls, port=19723):
        return cls(Link(port))
    
    def __init__(self, link):
        self.link = link
        self.description = self.GetProjectInfo().projectName if self.IsAlive() else 'ArchiCAD Command Object'

    #region Basic Commands
    def IsAlive(self):
        """Checks if a connection can be established with ArchiCAD.
        
        Returns:
            bool: True if successful, False otherwise.
        
        """
        cmd = {'command' : 'API.IsAlive'}
        try:
            response = self.link.post(cmd)
            if response is None:
                return False
            else:
                return response.success
        except Exception as ex:
            if type(ex).__name__ == 'URLError':
                return False
            else:
                raise ex

    def GetProductInfo(self):
        """Accesses ArchiCAD's version information.
        
        Returns:
            int: Version number.
            int: Build number.
            int: Language code.
        
        Raises:
            Exception: If command was unsuccessful.

        """
        cmd = {'command' : 'API.GetProductInfo'}
        response = self.link.post(cmd)
        result = response.get_result()
        return result['version'], result['buildNumber'], result['languageCode']
    
    def HighlightElements(self, elements, highlightedColors, wireframe3D = False, nonHighlightedColor = [150, 0, 0, 100]):
        """Highlights specified elements in current ArchiCAD document.

        Args:
            elements (:obj:`list` of :obj:`Element`): A list of elements.
            highlightedColors (:obj:`list` of :int:): RGBA Colors for highlighted elements.
            wireframe3D (:bool:): Optional parameter. Switch non-highlighted elements in the 3D window to wireframe
            nonHighlightedColor (:obj:`list` of :int:): Optional parameter. RGBA Color for non-highlighted elements.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """
        param_selected_color = Parameter("highlightedColors", highlightedColors)
        param_wireframe3D = Parameter("wireframe3D", wireframe3D)
        param_unselected_color = Parameter("nonHighlightedColor", nonHighlightedColor)
        param_elements = Parameter("elements", [element.ToDictionary() for element in elements])
        packed_params = Parameter.pack([param_elements, param_selected_color, param_wireframe3D, param_unselected_color])
        
        cmd = Command.FormatAddOnCommand("HighlightElements", packed_params)

        response = self.link.post(cmd)
        if not response:
            raise Exception("Script Error")
    #endregion Basic Commands

    #region Element Listing Commands
    def GetAllElements(self):
        """Returns all elements in the current document.
        
        Returns:
            :obj:`list` of :obj:`Element`: A list of Element instances that represent the elements in the current document.

        Raises:
            Exception: If command was unsuccessful.
        
        """
        cmd = {'command' : 'API.GetAllElements'}
        response = self.link.post(cmd)
        return Element.list_from_command_result(response.get_result())

    def GetElementsByType(self, element_type_id):
        """Returns all elements in the current document that match the specified element type.
        
        Args:
            element_type_id (str): The element type to match.
            
        Returns:
            :obj:`list` of :obj:`Element`: A list of elements in the current document of specified type.
        
        Raises:
            TypeError: If element type is invalid.
            Exception: If command was unsuccessful.
        
        """
        for item in Element._TYPES:
            if item.lower() == element_type_id.lower():
                element_type_id = item
                break
        else:
            raise TypeError("Couldn't find element type: {}".format(element_type_id))
        
        cmd = { 'command' : 'API.GetElementsByType',
                'parameters' : {'elementType': element_type_id}
                }
        response = self.link.post(cmd)
        return Element.list_from_command_result(response.get_result())

    def GetElementsByClassification(self, classification_system_id):
        """Returns all elements in the current document that match the specified classification system.
        
        Args:
            classification_system_id (str): The id of the classification system to match.
            
        Returns:
            :obj:`list` of :obj:`Element`: A list of elements in the current document of specified classification system.
        
        Raises:
            Exception: If command was unsuccessful.
        
        """
        cmd = { 'command' : 'API.GetElementsByClassification',
                'parameters' : { 'classificationItemId' : {'guid' : classification_system_id}}
                }
        response = self.link.post(cmd) #TODO: Check format
        return Element.list_from_command_result(response.get_result())
    
    def GetSelectedElements(self):
        """Returns all elements in the current document.
        
        Returns:
            :obj:`list` of :obj:`Element`: A list of Element instances that represent the elements in the current document.

        Raises:
            Exception: If command was unsuccessful.
        
        """
        cmd = Command.FormatAddOnCommand("GetSelectedElements")
        response = self.link.post(cmd)
        return Element.list_from_command_result(response.get_result())
    #endregion Element Listing Commands

    #region Classification Commands
    def GetAllClassificationSystems(self):
        """Returns all classification systems in the current document.
        
        Returns:
            :obj:`list` of :obj:`ClassificationSystem`: A list of ClassificationSystem instances that represent the classification systems in the current document.

        Raises:
            Exception: If command was unsuccessful.
        
        """
        cmd = {'command' : 'API.GetAllClassificationSystems'}
        response = self.link.post(cmd)
        return ClassificationSystem.list_from_command_result(response.get_result())
    
    def GetAllClassificationsInSystem(self, classification_system):
        """ Return the tree of classifications in the given classification system

        Args:
            Classification_system_id (str): The id of the classification system

        Returns:
            :obj:`list` of :obj:`ClassificationItem`: A Tree of classificationItems in the given classification system.
        """
        cmd = {'command':'API.GetAllClassificationsInSystem',
            'parameters':{'classificationSystemId':{'guid':classification_system.guid}}}
        
        response = self.link.post(cmd)
        return ClassificationItem.list_from_command_result(response.get_result())
    
    def GetDetailsOfClassificationItems(self,classification_item_guids):
        """Returns the detail of classification items
        
        Arg:
            :obj:`list` of Classification_item_guid (str): A list of classification item guids

        Returns:
            :obj:`list` of :obj: `ClassificationItem` : A list of classification items.

        """
        
        idList=[]
        for guid in classification_item_guids:
            idList.append({'classificationItemId':{'guid':guid}})
        cmd = {'command':'API.GetDetailsOfClassificationItems',
        'parameters' : {'classificationItemIds':idList}}

        response = self.link.post(cmd)
        return ClassificationItem.list_from_command_result(response.get_result())

    #endregion Classification Commands

    #region Element Geometry Commands
    def Get2DBoundingBoxes(self, elements):
        """Returns a list of bounding boxes for the specified elements.

        Note:
            The 2D bounding box is calculated from the global origin on the floor plan view. Only works for elements detailled in <i>Element Information</i>.
        
        Args:
            elements (:obj:`list` of :obj:`Element`): A list of elements.
        
        Returns:
            :obj:`list` of :obj:`BoundingBox`: A list of BoundingBox instances that represent the elements provided.
        
        """
        element_list = []
        for element in elements:
            element_list.append(element.ToDictionary())
        
        cmd = {'command' : 'API.Get2DBoundingBoxes'}
        cmd.update(Parameter('elements', element_list).ToDictionary())
        response = self.link.post(cmd)
        return BoundingBox.list_from_command_result(response.get_result())

    def Get3DBoundingBoxes(self, elements):
        """Returns a list of bounding boxes for the specified elements.

        Note:
            The 3D bounding box is calculated from the global origin in the 3D view. Only works for elements detailled in <i>Element Information</i>.
        
        Args:
            elements (:obj:`list` of :obj:`Element`): A list of elements.
        
        Returns:
            :obj:`list` of :obj:`BoundingBox`: A list of BoundingBox instances that represent the elements provided.
        
        """
        element_list = []
        for element in elements:
            element_list.append(element.ToDictionary())
        
        cmd = {'command' : 'API.Get3DBoundingBoxes'}
        cmd.update(Parameter('elements', element_list).ToDictionary())
        response = self.link.post(cmd)
        return BoundingBox.list_from_command_result(response.get_result())
    
    #endregion Element Geometry Commands

    #region Project Commands
    def GetProjectInfo(self):
        """Retrieves details of the current archicad file.

        Attributes:
            str: projectPath
            str: projectName
            str: projectLocation
            bool: isTeamwork
            bool: isUntitled

        Returns:
            obj: ProjectInfo
        """
        cmd = Command.FormatAddOnCommand("GetProjectInfo")
        response = self.link.post(cmd)
        return RuntimeObject(response.get_result(), 'ProjectInfo')

    def GetStoryInfo(self):
        """Retrieves details of the file's story structure.

        Attributes:
            int: index
            float: floorId
            bool: dispOnSections
            str: level
            str: uName
            list: stories

        Returns:
            obj: StoryInfo
        """
        cmd = Command.FormatAddOnCommand("GetStoryInfo")
        response = self.link.post(cmd)
        if response.success:
            return response.object(response._data["result"], "StoryInfo")
        else:
            return response.exception()

    def GetHotlinks(self): # TODO: Create Hotlink Class
        """Performs a publish operation on the currently opened project. Only the given publisher set will be published.

        Attributes:
            :obj:`list` of :obj:`dict`: A list of dictionaries that represent hotlink nodes.

        Returns:
            obj: HotlinkCollection
        """
        cmd = Command.FormatAddOnCommand("GetHotlinks")
        response = self.link.post(cmd)
        return RuntimeObject(response.get_result(), 'HotlinkCollection')
    
    #endregion Project Commands

    #region Application Commands
    def GetAddOnVersion(self):
        """Retrieves the version of the Tapir Additional JSON Commands Add-On.

        Returns:
            str: Version of the Tapir Additional JSON Commands Add-On.
        """
        cmd = Command.FormatAddOnCommand("GetAddOnVersion")
        response = self.link.post(cmd)
        return response.get_result()['version']
    
    def GetArchicadLocation(self):
        """Retrieves the location of the currently running Archicad executable.

        Returns:
            str: Location of current Archicad executable.
        """
        cmd = Command.FormatAddOnCommand("GetArchicadLocation")
        response = self.link.post(cmd)
        return response.get_result()['archicadLocation']
    
    def QuitArchicad(self):
        """Performs a quit operation on the currently running Archicad instance.

        Returns:
            bool: True if successful, False otherwise.
        """
        cmd = Command.FormatAddOnCommand("QuitArchicad")
        response = self.link.post(cmd)
        return response.success
    #endregion Application Commands

    #region Teamwork Commands
    def TeamworkSend(self):
        """Performs a send operation on the currently opened Teamwork project.

        Returns:
            bool: True if successful, False otherwise.
        """
        cmd = Command.FormatAddOnCommand("TeamworkSend")
        response = self.link.post(cmd)
        return response.success

    def TeamworkReceive(self):
        """Performs a receive operation on the currently opened Teamwork project.

        Returns:
            bool: True if successful, False otherwise.
        """
        cmd = Command.FormatAddOnCommand("TeamworkReceive")
        response = self.link.post(cmd)
        return response.success
    #endregion Teamwork Commands

    def __str__(self):
        return self.description

    # ISSUE MANAGEMENT

    #region Issue Commands
    def CreateIssue(self, name, parentId="", tagText=""):
        """Creates new issue.

        Args:
            name (string):      The name of the issue.
            parentId (string):  UUID of parent issue, optional.
            tagText (string):   Issue text tag, optional.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_name = Parameter("name", name)
        param_parentId = Parameter("parentId", parentId)
        param_tagText = Parameter("tagText", tagText)
        packed_params = Parameter.pack([param_name, param_parentId, param_tagText])

        cmd = Command.FormatAddOnCommand("CreateIssue", packed_params)
        response = self.link.post(cmd)
        return response

    def DeleteIssue(self, issueId):
        """Creates new issue.

        Args:
            issueId (string):  UUID of parent issue, optional.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issueId = Parameter("issueId", issueId)
        packed_params = Parameter.pack([param_issueId,])

        cmd = Command.FormatAddOnCommand("DeleteIssue", packed_params)
        response = self.link.post(cmd)
        return response

    def GetIssues(self):
        """Retrieves issues from the open project.

        Returns:
           `list` of :obj:`Issue`: A list of issue instances

        """
        cmd = Command.FormatAddOnCommand("GetIssues")
        response = self.link.post(cmd)
        if response.success:
            return response.get_result()['issues']
        else:
            return response.exception()

    def AddComment(self, issueId, text, author=None, status=3):
        """Creates new comment for the chosen issue.

        Args:
            issueId (string):  Issue Id.
            author (string):  Issue author, optional.
            status (int):  Comment status type.
            text (string):  Comment text to add.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issueId = Parameter("issueId", issueId)
        param_author = Parameter("author", author)
        param_status = Parameter("status", status)
        param_text = Parameter("text", text)
        packed_params = Parameter.pack([param_issueId, param_author, param_status, param_text])

        cmd = Command.FormatAddOnCommand("AddComment", packed_params)
        response = self.link.post(cmd)
        return response

    def GetComments(self, issueId):
        """Retrieves comments from the chosen issue.

        Returns:
           `list` of :obj:`Comment`: A list of issue comments

        """
        param_issueId = Parameter("issueId", issueId)
        packed_params = Parameter.pack([param_issueId,])
        cmd = Command.FormatAddOnCommand("GetComments", packed_params)
        response = self.link.post(cmd)
        if response.success:
            return response.get_result()['comments']
        else:
            return response.exception()

    def AttachElements(self, issueId, elementsIds, type):
        """Attaches chosen elements to the issue.

        Args:
            issueId (string):               Issue Id.
            elementsIds (list of string):   Ids of elements.
            type (integer):                 Attachment type.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issueId = Parameter("issueId", issueId)
        param_elementsIds = Parameter("elementsIds", elementsIds)
        param_type = Parameter("type", type)
        packed_params = Parameter.pack([param_issueId, param_elementsIds, param_type])

        cmd = Command.FormatAddOnCommand("AttachElements", packed_params)
        response = self.link.post(cmd)
        return response

    def DetachElements(self, issueId, elementsIds):
        """Detaches chosen elements to the issue.

        Args:
            issueId (string):               Issue Id.
            elementsIds (list of string):   Ids of elements.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issueId = Parameter("issueId", issueId)
        param_elementsIds = Parameter("elementsIds", elementsIds)
        packed_params = Parameter.pack([param_issueId, param_elementsIds])

        cmd = Command.FormatAddOnCommand("DetachElements", packed_params)
        response = self.link.post(cmd)
        return response

    def GetAttachedElements(self, issueId, type):
        """Retrieves elements attached the chosen issue, filtered by attachment type.

        Args:
            issueId (string):               Issue Id.
            type (integer):                 Attachment type.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issueId = Parameter("issueId", issueId)
        param_type = Parameter("type", type)
        packed_params = Parameter.pack([param_issueId, param_type])

        cmd = Command.FormatAddOnCommand("GetAttachedElements", packed_params)
        response = self.link.post(cmd)
        if response.success:
            return response.get_result()['elements']
        else:
            return response.exception()

    def ExportToBCF(self, issuesIds, exportPath, useExternalId=False, alignBySurveyPoint=True):
        """Detaches chosen elements to the issue.

        Args:
            issuesIds (list of strings):    List of issue's Ids to export.
            exportPath (string):            System path to the export file, including name.
            useExternalId (bool):           Use external IFC ID or Archicad IFC ID as referenced in BCF topics.
            alignBySurveyPoint (bool):      Align BCF views by Archicad Survey Point or Archicad Project Origin.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_issuesIds = Parameter("issuesIds", issuesIds)
        param_exportPath = Parameter("exportPath", exportPath)
        param_useExternalId = Parameter("useExternalId", useExternalId)
        param_alignBySurveyPoint = Parameter("alignBySurveyPoint", alignBySurveyPoint)
        packed_params = Parameter.pack([param_issuesIds, param_exportPath, param_useExternalId, param_alignBySurveyPoint])

        cmd = Command.FormatAddOnCommand("ExportToBCF", packed_params)
        response = self.link.post(cmd)
        return response

    def ImportFromBCF(self, importPath, alignBySurveyPoint=True):
        """Detaches chosen elements to the issue.

        Args:
            importPath (string):            System path to the import file, including name.
            alignBySurveyPoint (bool):      Align BCF views by Archicad Survey Point or Archicad Project Origin.

        Returns:
            None
            
        Raises:
            Exception: If command was unsuccessful.
        """

        param_importPath = Parameter("importPath", importPath)
        param_alignBySurveyPoint = Parameter("alignBySurveyPoint", alignBySurveyPoint)
        packed_params = Parameter.pack([param_importPath, param_alignBySurveyPoint])

        cmd = Command.FormatAddOnCommand("ImportFromBCF", packed_params)
        response = self.link.post(cmd)
        return response

    #endregion Issue Commands