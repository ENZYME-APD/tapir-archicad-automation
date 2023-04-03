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
        }
    })";
    return commonSchemaDefinitions;
}
