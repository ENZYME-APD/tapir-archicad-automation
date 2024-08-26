var gSchemaDefinitions = {
        "Elements": {
            "type": "array",
            "description": "A list of elements.",
            "items": {
                "$ref": "#/ElementIdArrayItem"
            }
        },
        "ElementIdArrayItem": {
            "type": "object",
            "properties": {
                "elementId": {
                    "$ref": "#/ElementId"
                }
            },
            "additionalProperties": false,
            "required": [
                "elementId"
            ]
        },
        "ElementId": {
            "type": "object",
            "description": "The identifier of an element.",
            "properties": {
                "guid": {
                    "$ref": "#/Guid"
                }
            },
            "additionalProperties": false,
            "required": [
                "guid"
            ]
        },
        "AttributeIds": {
            "type": "array",
            "description": "A list of attributes.",
            "items": {
                "$ref": "#/AttributeIdArrayItem"
            }
        },
        "AttributeIdArrayItem": {
            "type": "object",
            "properties": {
                "attributeId": {
                    "$ref": "#/AttributeId"
                }
            },
            "additionalProperties": false,
            "required": [
                "attributeId"
            ]
        },
        "AttributeId": {
            "type": "object",
            "description": "The identifier of an attribute.",
            "properties": {
                "guid": {
                    "$ref": "#/Guid"
                }
            },
            "additionalProperties": false,
            "required": [
                "guid"
            ]
        },
        "Guid": {
            "type": "string",
            "description": "A Globally Unique Identifier (or Universally Unique Identifier) in its string representation as defined in RFC 4122.",
            "format": "uuid",
            "pattern": "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
        },
        "Hotlinks": {
            "type": "array",
            "description": "A list of hotlink nodes.",
            "items": {
                "$ref": "#/Hotlink"
            }
        },
        "Hotlink": {
            "type": "object",
            "description": "The details of a hotlink node.",
            "properties": {
                "location": {
                    "type": "string",
                    "description": "The path of the hotlink file."
                },
                "children": {
                    "$ref": "#/Hotlinks",
                    "description": "The children of the hotlink node if it has any."
                }
            },
            "additionalProperties": false,
            "required": [
                "location"
            ]
        },
        "GDLParametersDictionary": {
            "type": "object",
            "description": "The dictionary of GDL parameters. The name of the parameter is the key and the details of the parameter are in the value.",
            "additionalProperties": {
                "type": "#/GDLParameterDetails"
            }
        },
        "GDLParameterDetails": {
            "type": "object",
            "description": "Details of a GDL parameter.",
            "properties": {
                "index": {
                    "type": "string",
                    "description": "The index of the parameter."
                },
                "type": {
                    "type": "string",
                    "description": "The type of the parameter."
                },
                "dimension1": {
                    "type": "number",
                    "description": "The 1st dimension of array (in case of array value)."
                },
                "dimension2": {
                    "type": "number",
                    "description": "The 2nd dimension of array (in case of array value)."
                },
                "value": {
                    "description": "The value of the parameter."
                }
            },
            "additionalProperties": true,
            "required": [
                "index",
                "type",
                "value"
            ]
        },
        "2DCoordinate": {
            "type": "object",
            "description" : "2D coordinate.",
            "properties" : {
                "x": {
                    "type": "number",
                    "description" : "X value of the coordinate."
                },
                "y" : {
                    "type": "number",
                    "description" : "Y value of the coordinate."
                }
            },
            "additionalProperties": false,
            "required" : [
                "x",
                "y"
            ]
        },
        "3DCoordinate": {
            "type": "object",
            "description" : "3D coordinate.",
            "properties" : {
                "x": {
                    "type": "number",
                    "description" : "X value of the coordinate."
                },
                "y" : {
                    "type": "number",
                    "description" : "Y value of the coordinate."
                },
                "z" : {
                    "type": "number",
                    "description" : "Z value of the coordinate."
                }
            },
            "additionalProperties": false,
            "required" : [
                "x",
                "y",
                "z"
            ]
        },
        "3DDimensions": {
            "type": "object",
            "description" : "Dimensions in 3D.",
            "properties" : {
                "x": {
                    "type": "number",
                    "description" : "X dimension."
                },
                "y" : {
                    "type": "number",
                    "description" : "Y dimension."
                },
                "z" : {
                    "type": "number",
                    "description" : "Z dimension."
                }
            },
            "additionalProperties": false,
            "required" : [
                "x",
                "y",
                "z"
            ]
        },
        "Error": {
            "type": "object",
            "description": "The details of an error.",
            "properties": {
                "code": {
                    "type": "integer",
                    "description": "The code of the error."
                },
                "message": {
                    "type": "string",
                    "description": "The error message."
                }
            },
            "additionalProperties": false,
            "required": [
                "code",
                "message"
            ]
        },
        "ErrorItem": {
            "type": "object",
            "properties": {
                "error": {
                    "$ref": "#/Error"
                }
            },
            "additionalProperties": false,
            "required": [ "error" ]
        },
        "SuccessfulExecutionResult": {
            "type": "object",
            "description": "The result of a successful execution.",
            "properties": {
                "success": {
                    "type": "boolean",
                    "enum": [ true ]
                }
            },
            "additionalProperties": false,
            "required": [
                "success"
            ]
        },
        "FailedExecutionResult": {
            "type": "object",
            "description": "The result of a failed execution.",
            "properties": {
                "success": {
                    "type": "boolean",
                    "enum": [ false ]
                },
                "error": {
                    "$ref": "#/Error",
                    "description": "The details of an execution failure."
                }
            },
            "additionalProperties": false,
            "required": [
                "success",
                "error"
            ]
        },
        "ExecutionResult": {
            "type": "object",
            "description": "The result of the execution.",
            "oneOf": [
                {
                    "$ref": "#/SuccessfulExecutionResult"
                },
                {
                    "$ref": "#/FailedExecutionResult"
                }
            ]
        },
        "ExecutionResults": {
            "type": "array",
            "description": "A list of execution results.",
            "items": {
                "$ref": "#/ExecutionResult"
            }
        },
        "ElementType": {
            "type": "string",
            "description": "The type of an element.",
            "enum": [
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
        },
        "ElementFilter": {
            "type": "string",
            "description": "A filter type for an element.",
            "enum": [
                "IsEditable",
                "IsVisibleByLayer",
                "IsVisibleByRenovation",
                "IsVisibleByStructureDisplay",
                "IsVisibleIn3D",
                "OnActualFloor",
                "OnActualLayout",
                "InMyWorkspace",
                "IsIndependent",
                "InCroppedView",
                "HasAccessRight",
                "IsOverriddenByRenovation"
            ]
        },
        "WindowType": {
            "type": "string",
            "description": "The type of a window.",
            "enum": [
                "FloorPlan",
                "Section",
                "Details",
                "3DModel",
                "Layout",
                "Drawing",
                "CustomText",
                "CustomDraw",
                "MasterLayout",
                "Elevation",
                "InteriorElevation",
                "Worksheet",
                "Report",
                "3DDocument",
                "External3D",
                "Movie3D",
                "MovieRendering",
                "Rendering",
                "ModelCompare",
                "Interactive Schedule",
                "Unknown"
            ]
        },
        "PropertyId": {
            "type": "object",
            "description": "The identifier of a property.",
            "properties": {
                "guid": {
                    "$ref": "#/Guid"
                }
            },
            "additionalProperties": false,
            "required": [
                "guid"
            ]
        },
        "PropertyIdArrayItem": {
            "type": "object",
            "properties": {
                "propertyId": {
                    "$ref": "#/PropertyId"
                }
            },
            "additionalProperties": false,
            "required": [
                "propertyId"
            ]
        },
        "PropertyIds": {
            "type": "array",
            "description": "A list of property identifiers.",
            "items": {
                "$ref": "#/PropertyIdArrayItem"
            }
        },
        "PropertyValue": {
            "type": "object",
            "description": "The display string value of a property.",
            "properties": {
                "value": {
                    "type": "string"
                }
            },
            "additionalProperties": true,
            "required": [
                "value"
            ]
        },
        "PropertyValueOrErrorItem": {
            "type": "object",
            "description": "A property value or an error",
            "oneOf": [
                {
                    "title": "propertyValue",
                    "properties": {
                        "propertyValue": {
                            "$ref": "#/PropertyValue"
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "propertyValue" ]
                },
                {
                    "title": "error",
                        "$ref": "#/ErrorItem"
                }
            ]
        },
        "PropertyValues": {
            "type": "array",
            "description": "A list of property values.",
            "items": {
                "$ref": "#/PropertyValueOrErrorItem"
            }
        },
        "PropertyValuesOrError": {
            "type": "object",
            "description": "A list of property values or an error.",
            "oneOf": [
                {
                    "title": "propertyValues",
                    "properties": {
                        "propertyValues": {
                            "$ref": "#/PropertyValues"
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "propertyValues" ]
                },
                {
                    "title": "error",
                    "$ref": "#/ErrorItem"
                }
            ]
        },
        "PropertyValuesOrErrorArray": {
            "type": "array",
            "description": "A list of property value lists.",
            "items": {
                "$ref": "#/PropertyValuesOrError"
            }
        },
        "ElementPropertyValue": {
            "type": "object",
            "description": "A property value with the identifiers of the property and its owner element.",
            "properties": {
                "elementId": {
                    "$ref": "#/ElementId"
                },
                "propertyId": {
                    "$ref": "#/PropertyId"
                },
                "propertyValue": {
                    "type": "#/PropertyValue"
                }
            },
            "additionalProperties": false,
            "required": [
                "elementId",
                "propertyId",
                "propertyValue"
            ]
        },
        "ElementPropertyValues": {
            "type": "array",
            "description": "A list of element property values.",
            "items": {
                "$ref": "#/ElementPropertyValue"
            }
        },
        "ClassificationSystemId": {
            "type": "object",
            "description": "The identifier of a classification system.",
            "properties": {
                "guid": {
                    "$ref": "#/Guid"
                }
            },
            "additionalProperties": false,
            "required": [
                "guid"
            ]
        },
        "ClassificationSystemIdArrayItem": {
            "type": "object",
            "properties": {
                "classificationSystemId": {
                    "$ref": "#/ClassificationSystemId"
                }
            },
            "additionalProperties": false,
            "required": [
                "classificationSystemId"
            ]
        },
        "ClassificationSystemIds": {
            "type": "array",
            "description": "A list of classification system identifiers.",
            "items": {
                "$ref": "#/ClassificationSystemIdArrayItem"
            }
        },
        "ClassificationItemId": {
            "type": "object",
            "description": "The identifier of a classification item.",
            "properties": {
                "guid": {
                    "$ref": "#/Guid"
                }
            },
            "additionalProperties": false,
            "required": [
                "guid"
            ]
        },
        "ClassificationId": {
            "type": "object",
            "description": "The element classification identifier.",
            "properties": {
                "classificationSystemId": {
                    "$ref": "#/ClassificationSystemId"
                },
                "classificationItemId": {
                    "$ref": "#/ClassificationItemId",
                    "description": "The element's classification in the given system. If no value is specified here, the element is Unclassified in this system."
                }
            },
            "additionalProperties": false,
            "required": [
                "classificationSystemId"
            ]
        },
        "ClassificationIdOrError": {
            "type": "object",
            "description": "A classification identifier or an error.",
            "oneOf": [
                {
                    "title": "classificationId",
                    "properties": {
                        "classificationId": {
                            "$ref": "#/ClassificationId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "classificationId" ]
                },
                {
                    "title": "error",
                    "$ref": "#/ErrorItem"
                }
            ]   
        },
        "ClassificationIdsOrErrors": {
            "type": "array",
            "description": "A list of element classification identifiers or errors.",
            "items": {
                "$ref": "#/ClassificationIdOrError"
            }
        },
        "ElementClassificationOrError": {
            "type": "object",
            "description": "Element classification identifiers or errors.",
            "oneOf": [
                {
                    "title": "classificationIds",
                    "properties": {
                        "classificationIds": {
                            "$ref": "#/ClassificationIdsOrErrors"
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "classificationIds" ]
                },
                {
                    "title": "error",
                    "$ref": "#/ErrorItem"
                }
            ]
        },
        "ElementClassificationsOrErrors": {
            "type": "array",
            "description": "A list of element classification identifiers or errors.",
            "items": {
                "$ref": "#/ElementClassificationOrError"
            }
        },
        "ElementClassification": {
            "type": "object",
            "description": "The classification of an element.",
            "properties": {
                "elementId": {
                    "$ref": "#/ElementId"
                },
                "classificationId": {
                    "$ref": "#/ClassificationId"
                }
            },
            "additionalProperties": false,
            "required": [
                "elementId",
                "classificationId"
            ]
        },
        "ElementClassifications": {
            "type": "array",
            "description": "A list of element classification identifiers.",
            "items": {
                "$ref": "#/ElementClassification"
            }
        }};