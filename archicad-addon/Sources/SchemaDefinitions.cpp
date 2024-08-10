#include "SchemaDefinitions.hpp"
#include "ObjectState.hpp"

const GS::UniString& GetCommonSchemaDefinitions ()
{
    static const GS::UniString commonSchemaDefinitions = R"({
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
        }
    })";
    return commonSchemaDefinitions;
}
