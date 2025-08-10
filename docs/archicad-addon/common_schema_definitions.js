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
    "AttributeType": {
        "type": "string",
        "description": "The type of an attribute.",
        "enum": [
            "Layer",
            "Line",
            "Fill",
            "Composite",
            "Surface",
            "LayerCombination",
            "ZoneCategory",
            "Profile",
            "PenTable",
            "MEPSystem",
            "OperationProfile",
            "BuildingMaterial"
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
    "GDLParameterArray": {
        "type": "array",
        "description": "The list of GDL parameters.",
        "items": {
            "$ref": "#/GDLParameterDetails"
        }
    },
    "GDLParameterList": {
        "type": "object",
        "description": "The list of GDL parameters.",
        "properties": {
            "parameters": {
                "$ref": "#/GDLParameterArray"
            }
        },
        "required": [
            "parameters"
        ]
    },
    "GDLParameterDetails": {
        "type": "object",
        "description": "Details of a GDL parameter.",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the parameter."
            },
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
    "PolyArc": {
        "type": "object",
        "description": "Representation of an arc segment of a two dimensional polygon/polyline.",
        "properties": {
            "begIndex": {
                "type": "integer",
                "description": "Node index of one end point of the arc."
            },
            "endIndex": {
                "type": "integer",
                "description": "Node index of the other end point of the arc."
            },
            "arcAngle": {
                "type": "number",
                "description": "Angle of the arc; it is positive, if the arc is on the right-hand side of the straight segment."
            }
        },
        "additionalProperties": false,
        "required": [
            "begIndex",
            "endIndex",
            "arcAngle"
        ]
    },
    "Coordinate2D": {
        "type": "object",
        "description": "2D coordinate.",
        "properties": {
            "x": {
                "type": "number",
                "description": "X value of the coordinate."
            },
            "y": {
                "type": "number",
                "description": "Y value of the coordinate."
            }
        },
        "additionalProperties": false,
        "required": [
            "x",
            "y"
        ]
    },
    "Coordinate3D": {
        "type": "object",
        "description": "3D coordinate.",
        "properties": {
            "x": {
                "type": "number",
                "description": "X value of the coordinate."
            },
            "y": {
                "type": "number",
                "description": "Y value of the coordinate."
            },
            "z": {
                "type": "number",
                "description": "Z value of the coordinate."
            }
        },
        "additionalProperties": false,
        "required": [
            "x",
            "y",
            "z"
        ]
    },
    "Dimensions3D": {
        "type": "object",
        "description": "Dimensions in 3D.",
        "properties": {
            "x": {
                "type": "number",
                "description": "X dimension."
            },
            "y": {
                "type": "number",
                "description": "Y dimension."
            },
            "z": {
                "type": "number",
                "description": "Z dimension."
            }
        },
        "additionalProperties": false,
        "required": [
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
            "Dimension",
            "RadialDimension",
            "LevelDimension",
            "AngleDimension",
            "Text",
            "Label",
            "Zone",
            "Hatch",
            "Line",
            "PolyLine",
            "Arc",
            "Circle",
            "Spline",
            "Hotspot",
            "CutPlane",
            "Camera",
            "CamSet",
            "Group",
            "SectElem",
            "Drawing",
            "Picture",
            "Detail",
            "Elevation",
            "InteriorElevation",
            "Worksheet",
            "Hotlink",
            "CurtainWall",
            "CurtainWallSegment",
            "CurtainWallFrame",
            "CurtainWallPanel",
            "CurtainWallJunction",
            "CurtainWallAccessory",
            "Shell",
            "Skylight",
            "Morph",
            "ChangeMarker",
            "Stair",
            "Riser",
            "Tread",
            "StairStructure",
            "Railing",
            "RailingToprail",
            "RailingHandrail",
            "RailingRail",
            "RailingPost",
            "RailingInnerPost",
            "RailingBaluster",
            "RailingPanel",
            "RailingSegment",
            "RailingNode",
            "RailingBalusterSet",
            "RailingPattern",
            "RailingToprailEnd",
            "RailingHandrailEnd",
            "RailingRailEnd",
            "RailingToprailConnection",
            "RailingHandrailConnection",
            "RailingRailConnection",
            "RailingEndFinish",
            "BeamSegment",
            "ColumnSegment",
            "Opening",
            "Unknown"
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
    "IssueId": {
        "type": "object",
        "description": "The identifier of an issue.",
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
    "IssueIdArrayItem": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId"
        ]
    },
    "Issues": {
        "type": "array",
        "description": "A list of Issues.",
        "items": {
            "$ref": "#/IssueIdArrayItem"
        }
    },
    "IssueElementType": {
        "type": "string",
        "description": "The attachment type of an element component of an issue.",
        "enum": [
            "Creation",
            "Highlight",
            "Deletion",
            "Modification"
        ]
    },
    "IssueCommentStatus": {
        "type": "string",
        "description": "The status of an issue comment.",
        "enum": [
            "Error",
            "Warning",
            "Info",
            "Unknown"
        ]
    },
    "PropertyGroupId": {
        "type": "object",
        "description": "The identifier of a property group.",
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
    "PropertyGroupIdArrayItem": {
        "type": "object",
        "description": "A wrapper containing the property group identifier.",
        "properties": {
            "propertyGroupId": {
                "$ref": "#/PropertyGroupId"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupId"
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
        "description": "A wrapper containing the property identifier.",
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
    "PropertyDetails": {
        "type": "object",
        "description": "The details of the property.",
        "properties": {
            "propertyId": {
                "$ref": "#/PropertyId"
            },
            "propertyType": {
                "type": "string",
                "enum": [
                    "StaticBuiltIn",
                    "DynamicBuiltIn",
                    "Custom"
                ]
            },
            "propertyGroupName": {
                "type": "string"
            },
            "propertyName": {
                "type": "string"
            },
            "propertyCollectionType": {
                "type": "string",
                "enum": [
                    "Undefined",
                    "Single",
                    "List",
                    "SingleChoiceEnumeration",
                    "MultipleChoiceEnumeration"
                ]
            },
            "propertyValueType": {
                "type": "string",
                "enum": [
                    "Undefined",
                    "Integer",
                    "Real",
                    "String",
                    "Boolean",
                    "Guid"
                ]
            },
            "propertyMeasureType": {
                "type": "string",
                "enum": [
                    "Undefined",
                    "Default",
                    "Length",
                    "Area",
                    "Volume",
                    "Angle"
                ]
            },
            "propertyIsEditable": {
                "type": "boolean"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyId",
            "propertyType",
            "propertyGroupName",
            "propertyName",
            "propertyCollectionType",
            "propertyValueType",
            "propertyMeasureType",
            "propertyIsEditable"
        ]
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
    "PropertyValueArrayItem": {
        "type": "object",
        "description": "A wrapper containing the property value.",
        "properties": {
            "propertyValue": {
                "$ref": "#/PropertyValue"
            }
        },
        "additionalProperties": false,
        "required": [ "propertyValue" ]
    },
    "PropertyValueOrErrorItem": {
        "type": "object",
        "description": "A property value or an error",
        "oneOf": [
            {
                "$ref": "#/PropertyValueArrayItem"
            },
            {
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
    "PropertyValuesArrayItem": {
        "description": "A wrapper containing the property values.",
        "properties": {
            "propertyValues": {
                "$ref": "#/PropertyValues"
            }
        },
        "additionalProperties": false,
        "required": [ "propertyValues" ]
    },
    "PropertyValuesOrError": {
        "type": "object",
        "description": "A list of property values or an error.",
        "oneOf": [
            {
                "$ref": "#/PropertyValuesArrayItem"
            },
            {
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
    "PropertyIdOrError": {
        "type": "object",
        "description": "A propertyId or an error.",
        "oneOf": [
            {
                "$ref": "#/PropertyIdArrayItem"
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    },
    "PropertyIdOrErrorArray": {
        "type": "array",
        "description": "A list of property identifiers.",
        "items": {
            "$ref": "#/PropertyIdOrError"
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
                "$ref": "#/PropertyValue"
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
    "AttributePropertyValue": {
        "type": "object",
        "description": "A property value with the identifiers of the property and its owner attribute.",
        "properties": {
            "attributeId": {
                "$ref": "#/AttributeId"
            },
            "propertyId": {
                "$ref": "#/PropertyId"
            },
            "propertyValue": {
                "$ref": "#/PropertyValue"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeId",
            "propertyId",
            "propertyValue"
        ]
    },
    "AttributePropertyValues": {
        "type": "array",
        "description": "A list of attribute property values.",
        "items": {
            "$ref": "#/AttributePropertyValue"
        }
    },
    "PropertyDataType": {
        "type": "string",
        "enum": [
            "number",
            "integer",
            "string",
            "boolean",
            "length",
            "area",
            "volume",
            "angle",
            "numberList",
            "integerList",
            "stringList",
            "booleanList",
            "lengthList",
            "areaList",
            "volumeList",
            "angleList",
            "singleEnum",
            "multiEnum"
        ]
    },
    "DisplayValueEnumId": {
        "type": "object",
        "description": "An enumeration value identifier using the displayed value.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "displayValue" ]
            },
            "displayValue": {
                "type": "string"
            }
        },
        "additionalProperties": false,
        "required": [
            "type",
            "displayValue"
        ]
    },
    "NonLocalizedValueEnumId": {
        "type": "object",
        "description": "An enumeration value identifier using the nonlocalized value.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "nonLocalizedValue" ]
            },
            "nonLocalizedValue": {
                "type": "string"
            }
        },
        "additionalProperties": false,
        "required": [
            "type",
            "nonLocalizedValue"
        ]
    },
    "EnumValueId": {
        "type": "object",
        "description": "The identifier of a property enumeration value.",
        "oneOf": [
            {
                "$ref": "#/DisplayValueEnumId"
            },
            {
                "$ref": "#/NonLocalizedValueEnumId"
            }
        ]
    },
    "EnumValueIdArrayItem": {
        "type": "object",
        "description": "A wrapper containing the identifier of a property enumeration value.",
        "properties": {
            "enumValueId": {
                "$ref": "#/EnumValueId"
            }
        },
        "additionalProperties": false,
        "required": [ "enumValueId" ]
    },
    "EnumValueIds": {
        "type": "array",
        "description": "A list of enumeration identifiers.",
        "items": {
            "$ref": "#/EnumValueIdArrayItem"
        }
    },
    "NormalOrUserUndefinedPropertyValue": {
        "type": "object",
        "description": "A normal or a userUndefined property value.",
        "oneOf": [
            {
                "$ref": "#/NormalNumberPropertyValue"
            },
            {
                "$ref": "#/NormalIntegerPropertyValue"
            },
            {
                "$ref": "#/NormalStringPropertyValue"
            },
            {
                "$ref": "#/NormalBooleanPropertyValue"
            },
            {
                "$ref": "#/NormalLengthPropertyValue"
            },
            {
                "$ref": "#/NormalAreaPropertyValue"
            },
            {
                "$ref": "#/NormalVolumePropertyValue"
            },
            {
                "$ref": "#/NormalAnglePropertyValue"
            },
            {
                "$ref": "#/NormalNumberListPropertyValue"
            },
            {
                "$ref": "#/NormalIntegerListPropertyValue"
            },
            {
                "$ref": "#/NormalStringListPropertyValue"
            },
            {
                "$ref": "#/NormalBooleanListPropertyValue"
            },
            {
                "$ref": "#/NormalLengthListPropertyValue"
            },
            {
                "$ref": "#/NormalAreaListPropertyValue"
            },
            {
                "$ref": "#/NormalVolumeListPropertyValue"
            },
            {
                "$ref": "#/NormalAngleListPropertyValue"
            },
            {
                "$ref": "#/NormalSingleEnumPropertyValue"
            },
            {
                "$ref": "#/NormalMultiEnumPropertyValue"
            },
            {
                "$ref": "#/UserUndefinedPropertyValue"
            }
        ]
    },
    "PropertyValueDetails": {
        "type": "object",
        "description": "A normal, userUndefined, notAvailable or notEvaluated property value.",
        "oneOf": [
            {
                "$ref": "#/NormalOrUserUndefinedPropertyValue"
            },
            {
                "$ref": "#/NotAvailablePropertyValue"
            }
        ]
    },
    "UserUndefinedPropertyValue": {
        "type": "object",
        "description": "A userUndefined value means that there is no actual number/string/etc. value, but the user deliberately set an Undefined value: this is a valid value, too.",
        "properties": {
            "type": {
                "$ref": "#/PropertyDataType"
            },
            "status": {
                "type": "string",
                "enum": [ "userUndefined" ]
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status" ]
    },
    "NotAvailablePropertyValue": {
        "type": "object",
        "description": "A notAvailable value means that the property is not available for the property owner (and therefore it has no property value for it).",
        "properties": {
            "type": {
                "$ref": "#/PropertyDataType"
            },
            "status": {
                "type": "string",
                "enum": [ "notAvailable" ]
            }
        },
        "additionalProperties": false,
        "required": [
            "type",
            "status"
        ]
    },
    "NormalNumberPropertyValue": {
        "type": "object",
        "description": "A number property value containing a valid numeric value.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "number" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "number"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalIntegerPropertyValue": {
        "type": "object",
        "description": "An integer property value containing a valid integer number.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "integer" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "integer"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalStringPropertyValue": {
        "type": "object",
        "description": "A string property value containing a valid string.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "string" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "string"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalBooleanPropertyValue": {
        "type": "object",
        "description": "A boolean property value containing a valid boolean value.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "boolean" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "boolean"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalLengthPropertyValue": {
        "type": "object",
        "description": "A length property value containing a real length value. The value is measured in SI (meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "length" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "number"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalAreaPropertyValue": {
        "type": "object",
        "description": "An area property value containing a real area. The value is measured in SI (square meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "area" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "number"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalVolumePropertyValue": {
        "type": "object",
        "description": "A volume property value containing a real volume. The value is measured in SI (cubic meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "volume" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "number"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalAnglePropertyValue": {
        "type": "object",
        "description": "An angle property value containing a real angle. The value is measured in SI (radians).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "angle" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "number"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalNumberListPropertyValue": {
        "type": "object",
        "description": "A number list property value containing numbers in an array.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "numberList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalIntegerListPropertyValue": {
        "type": "object",
        "description": "An integer list property value containing integers in an array.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "integerList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "integer"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalStringListPropertyValue": {
        "type": "object",
        "description": "A string list property value containing strings in an array.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "stringList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalBooleanListPropertyValue": {
        "type": "object",
        "description": "A boolean list property value containing boolean values in an array.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "booleanList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "boolean"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalLengthListPropertyValue": {
        "type": "object",
        "description": "A length list property value containing length values in an array. The values are measured in SI (meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "lengthList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalAreaListPropertyValue": {
        "type": "object",
        "description": "An area list property value containing areas in an array. The values are measured in SI (square meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "areaList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalVolumeListPropertyValue": {
        "type": "object",
        "description": "A volume list property value containing volumes in an array. The values are measured in SI (cubic meters).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "volumeList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalAngleListPropertyValue": {
        "type": "object",
        "description": "An angle list property value containing angles in an array. The values are measured in SI (radians).",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "angleList" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "type": "array",
                "items": {
                    "type": "number"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalSingleEnumPropertyValue": {
        "type": "object",
        "description": "A single enumeration property value containing the ID of the selected enum value.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "singleEnum" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "$ref": "#/EnumValueId"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "NormalMultiEnumPropertyValue": {
        "type": "object",
        "description": "A multiple choice enumeration property value containing the IDs of the selected enum values in an array.",
        "properties": {
            "type": {
                "type": "string",
                "enum": [ "multiEnum" ]
            },
            "status": {
                "type": "string",
                "enum": [ "normal" ]
            },
            "value": {
                "$ref": "#/EnumValueIds"
            }
        },
        "additionalProperties": false,
        "required": [ "type", "status", "value" ]
    },
    "BasicDefaultValue": {
        "type": "object",
        "description": "Default value of the property in case of a basic property value (ie. not an expression).",
        "properties": {
            "basicDefaultValue": {
                "$ref": "#/PropertyValueDetails"
            }
        },
        "additionalProperties": false,
        "required": [
            "basicDefaultValue"
        ]
    },
    "ExpressionDefaultValue": {
        "type": "object",
        "description": "Default value of the property in case of an expression based property value.",
        "properties": {
            "expressions": {
                "type": "array",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "expressions"
        ]
    },
    "PropertyDefaultValue": {
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/BasicDefaultValue"
            },
            {
                "$ref": "#/ExpressionDefaultValue"
            }
        ]
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
    "ClassificationItemIdArrayItem": {
        "type": "object",
        "properties": {
            "classificationItemId": {
                "$ref": "#/ClassificationItemId"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationItemId"
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
    "ClassificationIdArrayItem": {
        "type": "object",
        "description": "A wrapper containing the classification identifier.",
        "properties": {
            "classificationId": {
                "$ref": "#/ClassificationId"
            }
        },
        "additionalProperties": false,
        "required": [ "classificationId" ]
    },
    "ClassificationIdOrError": {
        "type": "object",
        "description": "A classification identifier or an error.",
        "oneOf": [
            {
                "$ref": "#/ClassificationIdArrayItem"
            },
            {
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
    "ElementClassificationItemArray": {
        "description": "A wrapper containing a list of element classification identifiers or errors.",
        "properties": {
            "classificationIds": {
                "$ref": "#/ClassificationIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [ "classificationIds" ]
    },
    "ElementClassificationOrError": {
        "type": "object",
        "description": "Element classification identifiers or errors.",
        "oneOf": [
            {
                "$ref": "#/ElementClassificationItemArray"
            },
            {
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
    },
    "BoundingBox3D": {
        "type": "object",
        "description": "A 3D bounding box of an element.",
        "properties": {
            "xMin": {
                "type": "number",
                "description": "The minimum X value of the bounding box."
            },
            "yMin": {
                "type": "number",
                "description": "The minimum Y value of the bounding box."
            },
            "zMin": {
                "type": "number",
                "description": "The minimum Z value of the bounding box."
            },
            "xMax": {
                "type": "number",
                "description": "The maximum X value of the bounding box."
            },
            "yMax": {
                "type": "number",
                "description": "The maximum Y value of the bounding box."
            },
            "zMax": {
                "type": "number",
                "description": "The maximum Z value of the bounding box."
            }
        },
        "additionalProperties": false,
        "required": [
            "xMin",
            "yMin",
            "zMin",
            "xMax",
            "yMax",
            "zMax"
        ]
    },
    "BoundingBox3DArrayItem": {
        "type": "object",
        "description": "A wrapper containing a 3D bounding box.",
        "properties": {
            "boundingBox3D": {
                "$ref": "#/BoundingBox3D"
            }
        },
        "additionalProperties": false,
        "required": [
            "boundingBox3D"
        ]
    },
    "BoundingBox3DOrError": {
        "type": "object",
        "description": "A 3D bounding box or an error.",
        "oneOf": [
            {
                "$ref": "#/BoundingBox3DArrayItem"
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    },
    "BoundingBoxes3D": {
        "type": "array",
        "description": "A list of 3D bounding boxes.",
        "items": {
            "$ref": "#/BoundingBox3DOrError"
        }
    },
    "LibPartUnId": {
        "type": "object",
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
    "LibPartDetails": {
        "properties": {
            "name": {
                "type": "string"
            },
            "parentUnID": {
                "$ref": "#/LibPartUnId"
            },
            "ownUnID": {
                "$ref": "#/LibPartUnId"
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "parentUnID",
            "ownUnID"
        ]
    },
    "NavigatorItemIds": {
        "type": "array",
        "description": "A list of navigator item identifiers.",
        "items": {
            "$ref": "#/NavigatorItemIdArrayItem"
        }
    },
    "NavigatorItemIdArrayItem": {
        "type": "object",
        "properties": {
            "navigatorItemId": {
                "$ref": "#/NavigatorItemId"
            }
        },
        "additionalProperties": false,
        "required": [
            "navigatorItemId"
        ]
    },
    "NavigatorItemId": {
        "type": "object",
        "description": "The identifier of a navigator item.",
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
    "Databases": {
        "type": "array",
        "description": "A list of Archicad databases.",
        "items": {
            "$ref": "#/DatabaseIdArrayItem"
        }
    },
    "DatabaseIdArrayItem": {
        "type": "object",
        "properties": {
            "databaseId": {
                "$ref": "#/DatabaseId"
            }
        },
        "additionalProperties": false,
        "required": [
            "databaseId"
        ]
    },
    "DatabaseId": {
        "type": "object",
        "description": "The identifier of a database",
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
    "ViewSettings": {
        "type": "object",
        "description": "The settings of a navigator view",
        "properties": {
            "modelViewOptions": {
                "type": "string",
                "description": "The name of the model view options. If empty, the view has custom model view options."
            },
            "layerCombination": {
                "type": "string",
                "description": "The name of the layer combination. If empty, the view has custom layer combination."
            },
            "dimensionStyle": {
                "type": "string",
                "description": "The name of the dimension style. If empty, the view has custom dimension style."
            },
            "penSetName": {
                "type": "string",
                "description": "The name of the pen set. If empty, the view has custom pen set."
            },
            "graphicOverrideCombination": {
                "type": "string",
                "description": "The name of the graphic override combination. If empty, the view has custom graphic override combination."
            }
        },
        "additionalProperties": false,
        "required": []
    },
    "ViewSettingsOrError": {
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/ViewSettings"
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    },
    "ViewTransformations": {
        "type": "object",
        "properties": {
            "zoom": {
                "type": "object",
                "description": "The actual zoom parameters, rectangular region of the model.",
                "properties": {
                    "xMin": {
                        "type": "number",
                        "description": "The minimum X value of the zoom box."
                    },
                    "yMin": {
                        "type": "number",
                        "description": "The minimum Y value of the zoom box."
                    },
                    "xMax": {
                        "type": "number",
                        "description": "The maximum X value of the zoom box."
                    },
                    "yMax": {
                        "type": "number",
                        "description": "The maximum Y value of the zoom box."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "xMin",
                    "yMin",
                    "xMax",
                    "yMax"
                ]
            },
            "rotation": {
                "type": "number",
                "description": "The orientation in radian."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoom",
            "rotation"
        ]
    },
    "ViewTransformationsOrError": {
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/ViewTransformations"
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    },
    "Hole2D": {
        "type": "object",
        "description": "A 2D hole in an element defined by closed polylines",
        "properties": {
            "polygonCoordinates": {
                "type": "array",
                "description": "The 2D coordinates of the edge of the hole.",
                "items": {
                    "$ref": "#/Coordinate2D"
                },
                "minItems": 3
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the hole.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "polygonCoordinates"
        ]
    },
    "Holes2D": {
        "type": "array",
        "description": "A list of 2D holes in an element defined by closed polylines",
        "items": {
            "$ref": "#/Hole2D"
        }
    },
    "Hole3D": {
        "type": "object",
        "description": "A 3D hole in an element defined by closed polylines",
        "properties": {
            "polygonCoordinates": {
                "type": "array",
                "description": "The 3D coordinates of the polygon of the hole.",
                "items": {
                    "$ref": "#/Coordinate3D"
                },
                "minItems": 3
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the hole.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "polygonCoordinates"
        ]
    },
    "Holes3D": {
        "type": "array",
        "description": "A list of 3D holes in an element defined by closed polylines",
        "items": {
            "$ref": "#/Hole3D"
        }
    },
    "WallDetails": {
        "type": "object",
        "properties": {
            "geometryType": {
                "type": "string",
                "enum": [
                    "Straight",
                    "Trapezoid",
                    "Polygonal"
                ]
            },
            "begCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "endCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "zCoordinate": {
                "type": "number"
            },
            "height": {
                "type": "number",
                "description": "height relative to bottom"
            },
            "bottomOffset": {
                "type": "number",
                "description": "base level of the wall relative to the floor level"
            },
            "offset": {
                "type": "number",
                "description": "wall's base line's offset from ref. line"
            },
            "arcAngle": {
                "type": "number",
                "description": "The arc angle of the curved wall in radians."
            },
            "begThickness": {
                "type": "number",
                "description": "Thickness at the beginning in case of trapezoid wall"
            },
            "endThickness": {
                "type": "number",
                "description": "Thickness at the end in case of trapezoid wall"
            },
            "polygonOutline": {
                "type": "array",
                "description": "Polygon outline in case of polygonal wall",
                "items": {
                    "$ref": "#/Coordinate2D"
                }
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon arcs in case of polygonal wall",
                "items": {
                    "$ref": "#/PolyArc"
                }
            }
        },
        "required": [
            "geometryType",
            "begCoordinate",
            "endCoordinate",
            "zCoordinate",
            "height",
            "bottomOffset",
            "offset"
        ]
    },
    "BeamDetails": {
        "type": "object",
        "properties": {
            "begCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "endCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "zCoordinate": {
                "type": "number"
            },
            "level": {
                "type": "number",
                "description": "base height of the beam relative to the floor level"
            },
            "offset": {
                "type": "number",
                "description": "beam ref.line offset from the center"
            },
            "slantAngle": {
                "type": "number",
                "description": "The slant angle of the beam in radians."
            },
            "arcAngle": {
                "type": "number",
                "description": "The arc angle of the (horizontally) curved beam in radians."
            },
            "verticalCurveHeight": {
                "type": "number",
                "description": "The height of the vertical curve of the beam."
            }
        },
        "required": [
            "begCoordinate",
            "endCoordinate",
            "zCoordinate",
            "level",
            "offset",
            "slantAngle",
            "arcAngle",
            "verticalCurveHeight"
        ]
    },
    "SlabDetails": {
        "type": "object",
        "properties": {
            "thickness": {
                "type": "number",
                "description": "Thickness of the slab."
            },
            "level": {
                "type": "number",
                "description": "Distance of the reference level of the slab from the floor level."
            },
            "offsetFromTop": {
                "type": "number",
                "description": "Vertical distance between the reference level and the top of the slab."
            },
            "zCoordinate": {
                "type": "number"
            },
            "polygonOutline": {
                "type": "array",
                "description": "Polygon outline of the slab.",
                "items": {
                    "$ref": "#/Coordinate2D"
                }
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the slab.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            },
            "holes": {
                "$ref": "#/Holes2D"
            }
        },
        "required": [
            "thickness",
            "level",
            "offsetFromTop",
            "zCoordinate",
            "polygonOutline",
            "holes"
        ]
    },
    "ColumnDetails": {
        "type": "object",
        "properties": {
            "origin": {
                "$ref": "#/Coordinate2D"
            },
            "zCoordinate": {
                "type": "number"
            },
            "height": {
                "type": "number",
                "description": "height relative to bottom"
            },
            "bottomOffset": {
                "type": "number",
                "description": "base level of the column relative to the floor level"
            }
        },
        "required": [
            "origin",
            "zCoordinate",
            "height",
            "bottomOffset"
        ]
    },
    "DetailWorksheetDetails": {
        "type": "object",
        "properties": {
            "basePoint": {
                "$ref": "#/Coordinate2D",
                "description": "Coordinate of the base point"
            },
            "angle": {
                "type": "number",
                "description": "The rotation angle (radian) of the marker symbol"
            },
            "markerId": {
                "$ref": "#/ElementId",
                "description": "Guid of the marker symbol"
            },
            "detailName": {
                "type": "string",
                "description": "Name of the detail/worksheet"
            },
            "detailIdStr": {
                "type": "string",
                "description": "Reference ID of the detail/worksheet"
            },
            "isHorizontalMarker": {
                "type": "boolean",
                "description": "Marker symbol is always horizontal?"
            },
            "isWindowOpened": {
                "type": "boolean",
                "description": "Side (detail/worksheet) window is opened?"
            },
            "clipPolygon": {
                "type": "array",
                "description": "The clip polygon of the detail/worksheet",
                "items": {
                    "$ref": "#/Coordinate2D"
                }
            },
            "linkData": {
                "type": "object",
                "description": "The marker link data",
                "properties": {
                    "referredView": {
                        "$ref": "#/ElementId",
                        "description": "Guid of the referred view. Only if the marker refers to a view."
                    },
                    "referredDrawing": {
                        "$ref": "#/ElementId",
                        "description": "Guid of the referred drawing. Only if the marker refers to a drawing."
                    },
                    "referredPMViewPoint": {
                        "$ref": "#/ElementId",
                        "description": "Guid of the referred view point. Only if the marker refers to a view point."
                    }
                },
                "required": []
            }
        },
        "required": [
            "basePoint",
            "angle",
            "markerId",
            "detailName",
            "detailIdStr",
            "isHorizontalMarker",
            "isWindowOpened",
            "clipPolygon",
            "linkData"
        ]
    },
    "LibPartBasedElementDetails": {
        "type": "object",
        "properties": {
            "libPart": {
                "$ref": "#/LibPartDetails"
            },
            "ownerElementId": {
                "$ref": "#/ElementId"
            },
            "ownerElementType": {
                "$ref": "#/ElementType"
            }
        },
        "required": [
            "libPart"
        ]
    },
    "ObjectDetails": {
        "$ref": "#/LibPartBasedElementDetails",
        "properties": {
            "origin": {
                "$ref": "#/Coordinate3D"
            },
            "dimensions": {
                "$ref": "#/Coordinate3D"
            },
            "angle": {
                "type": "number"
            }
        },
        "required": [
            "origin",
            "dimensions",
            "angle"
        ]
    },
    "PolylineDetails": {
        "type": "object",
        "properties": {
            "coordinates": {
                "type": "array",
                "items": {
                    "$ref": "#/Coordinate2D"
                }
            },
            "arcs": {
                "type": "array",
                "description": "The arcs of the polyline.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            },
            "zCoordinate": {
                "type": "number"
            }
        },
        "required": [
            "coordinates",
            "zCoordinate"
        ]
    },
    "ZoneDetails": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name of the zone."
            },
            "numberStr": {
                "type": "string",
                "description": "Zone number."
            },
            "categoryAttributeId": {
                "$ref": "#/AttributeId",
                "description": "The identifier of the zone category attribute."
            },
            "stampPosition": {
                "$ref": "#/Coordinate2D",
                "description": "Position of the origin of the zone stamp."
            },
            "isManual": {
                "type": "boolean",
                "description": "Is the coordinates of the zone manually placed?"
            },
            "polygonCoordinates": {
                "type": "array",
                "description": "The 2D coordinates of the edge of the zone.",
                "items": {
                    "$ref": "#/Coordinate2D"
                },
                "minItems": 3
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the zone.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            },
            "holes": {
                "$ref": "#/Holes2D"
            },
            "zCoordinate": {
                "type": "number"
            }
        },
        "required": [
            "name",
            "numberStr",
            "categoryAttributeId",
            "stampPosition",
            "isManual",
            "polygonCoordinates",
            "zCoordinate"
        ]
    },
    "CurtainWallDetails": {
        "type": "object",
        "properties": {
            "height": {
                "type": "number"
            },
            "angle": {
                "type": "number",
                "description": "The rotation angle of the curtain wall in radians."
            }
        },
        "additionalProperties": false,
        "required": [
            "begCoordinate"
        ]
    },
    "CurtainWallSegmentDetails": {
        "type": "object",
        "properties": {
            "begCoordinate": {
                "$ref": "#/Coordinate3D"
            },
            "endCoordinate": {
                "$ref": "#/Coordinate3D"
            },
            "extrusionVector": {
                "$ref": "#/Coordinate3D"
            },
            "gridOrigin": {
                "$ref": "#/Coordinate3D"
            },
            "gridAngle": {
                "type": "number",
                "description": "The angle of the grid in radians."
            },
            "arcOrigin": {
                "$ref": "#/Coordinate3D"
            },
            "isNegativeArc": {
                "type": "boolean",
                "description": "Indicates if the arc is negative."
            }
        },
        "additionalProperties": false,
        "required": [
            "begCoordinate",
            "endCoordinate",
            "extrusionVector",
            "gridOrigin",
            "gridAngle"
        ]
    },
    "CurtainWallPanelDetails": {
        "type": "object",
        "properties": {
            "polygonCoordinates": {
                "type": "array",
                "description": "The 3D coordinates of the panel polygon.",
                "items": {
                    "$ref": "#/Coordinate3D"
                },
                "minItems": 3
            },
            "isHidden": {
                "type": "boolean",
                "description": "Indicates if the panel is hidden (deleted panels remain in the database)."
            },
            "segmentIndex": {
                "type": "number",
                "description": "The index of the curtain wall segment to which this panel belongs."
            },
            "className": {
                "type": "string"
            },
            "frames": {
                "type": "array",
                "description": "The surrounding frames.",
                "items": {
                    "$ref": "#/ElementIdArrayItem"
                },
                "minItems": 3
            }
        },
        "additionalProperties": false,
        "required": [
            "polygonCoordinates",
            "isHidden",
            "segmentIndex",
            "className",
            "frames"
        ]
    },
    "CurtainWallFrameDetails": {
        "type": "object",
        "properties": {
            "begCoordinate": {
                "$ref": "#/Coordinate3D"
            },
            "endCoordinate": {
                "$ref": "#/Coordinate3D"
            },
            "orientationVector": {
                "$ref": "#/Coordinate3D"
            },
            "panelConnectionHole": {
                "type": "object",
                "description": "The parameters of the panel connection hole.",
                "properties": {
                    "d": {
                        "type": "number",
                        "description": "Depth of the panel connection hole."
                    },
                    "w": {
                        "type": "number",
                        "description": "Width of the panel connection hole."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "d",
                    "w"
                ]
            },
            "frameContour": {
                "type": "object",
                "description": "The parameters of the frame contour.",
                "properties": {
                    "a1": {
                        "type": "number",
                        "description": "Width1 of the frame contour."
                    },
                    "a2": {
                        "type": "number",
                        "description": "Width2 of the frame contour."
                    },
                    "b1": {
                        "type": "number",
                        "description": "Length1 of the frame contour."
                    },
                    "b2": {
                        "type": "number",
                        "description": "Length2 of the frame contour."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "a1",
                    "a2",
                    "b1",
                    "b2"
                ]
            },
            "segmentIndex": {
                "type": "number",
                "description": "The index of the curtain wall segment to which this frame belongs."
            },
            "className": {
                "type": "string"
            },
            "type": {
                "type": "string",
                "enum": [
                    "Deleted",
                    "Division",
                    "Corner",
                    "Boundary",
                    "Custom"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "begCoordinate",
            "endCoordinate",
            "orientationVector",
            "panelConnectionHole",
            "frameContour",
            "segmentIndex",
            "className",
            "type"
        ]
    },
    "MeshSkirtType": {
        "type": "string",
        "description": "The type of the skirt structure.",
        "enum": [
            "SurfaceOnlyWithoutSkirt",
            "WithSkirt",
            "SolidBodyWithSkirt"
        ]
    },
    "MeshDetails": {
        "type": "object",
        "properties": {
            "level": {
                "type": "number",
                "description": "The Z reference level of coordinates."
            },
            "skirtType": {
                "$ref": "#/MeshSkirtType"
            },
            "skirtLevel": {
                "type": "number",
                "description": "The height of the skirt."
            },
            "polygonCoordinates": {
                "type": "array",
                "description": "The 3D coordinates of the outline polygon of the mesh.",
                "items": {
                    "$ref": "#/Coordinate3D"
                },
                "minItems": 3
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the mesh.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            },
            "holes": {
                "$ref": "#/Holes3D"
            },
            "sublines": {
                "type": "array",
                "description": "The leveling sublines inside the polygon of the mesh.",
                "items": {
                    "type": "object",
                    "properties": {
                        "coordinates": {
                            "type": "array",
                            "description": "The 3D coordinates of the leveling subline of the mesh.",
                            "items": {
                                "$ref": "#/Coordinate3D"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "coordinates"
                    ]
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "level",
            "skirtType",
            "skirtLevel",
            "polygonCoordinates"
        ]
    },
    "NotYetSupportedElementTypeDetails": {
        "type": "object",
        "properties": {
            "error": {
                "type": "string"
            }
        },
        "required": [
            "error"
        ]
    },
    "TypeSpecificDetails": {
        "description": "Represents the complete type-specific details of an element. Used as output from GET requests",
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/WallDetails"
            },
            {
                "$ref": "#/BeamDetails"
            },
            {
                "$ref": "#/SlabDetails"
            },
            {
                "$ref": "#/ColumnDetails"
            },
            {
                "$ref": "#/DetailWorksheetDetails"
            },
            {
                "$ref": "#/LibPartBasedElementDetails"
            },
            {
                "$ref": "#/PolylineDetails"
            },
            {
                "$ref": "#/ZoneDetails"
            },
            {
                "$ref": "#/CurtainWallDetails"
            },
            {
                "$ref": "#/CurtainWallSegmentDetails"
            },
            {
                "$ref": "#/CurtainWallPanelDetails"
            },
            {
                "$ref": "#/CurtainWallFrameDetails"
            },
            {
                "$ref": "#/MeshDetails"
            },
            {
                "$ref": "#/NotYetSupportedElementTypeDetails"
            }
        ]
    },
    "RevisionIssueId": {
        "type": "object",
        "description": "The identifier of a revision issue.",
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
    "DocumentRevisionId": {
        "type": "object",
        "description": "The identifier of a document revision.",
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
    "RevisionCustomSchemeData": {
        "type": "array",
        "items": {
            "type": "object",
            "properties": {
                "customSchemeKey": {
                    "$ref": "#/Guid"
                },
                "customSchemeValue": {
                    "type": "string"
                }
            },
            "additionalProperties": false,
            "required": [
                "customSchemeKey",
                "customSchemeValue"
            ]
        }
    },
    "DocumentRevisionReference": {
        "type": "object",
        "description": "A reference to a document revision belonging to the current issue",
        "properties": {
            "revisionId": {
                "$ref": "#/DocumentRevisionId"
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionId"
        ]
    },
    "DocumentRevisionReferences": {
        "type": "array",
        "description": "All document revisions belong to the current issue.",
        "items": {
            "$ref": "#/DocumentRevisionReference"
        }
    },
    "RevisionIssue": {
        "type": "object",
        "properties": {
            "revisionIssueId": {
                "$ref": "#/RevisionIssueId"
            },
            "id": {
                "type": "string"
            },
            "description": {
                "type": "string"
            },
            "issueTime": {
                "type": "string"
            },
            "issuedByUser": {
                "type": "string"
            },
            "overrideRevisionIDOfAllIncludedLayouts": {
                "type": "boolean"
            },
            "createNewRevisionInAllIncludedLayouts": {
                "type": "boolean"
            },
            "markersVisibleSinceIndex": {
                "type": "integer"
            },
            "isIssued": {
                "type": "boolean"
            },
            "documentRevisions": {
                "$ref": "#/DocumentRevisionReferences"
            },
            "customSchemeData": {
                "$ref": "#/RevisionCustomSchemeData"
            }
        },
        "required": [
            "revisionIssueId",
            "id",
            "description",
            "issueTime",
            "issuedByUser",
            "overrideRevisionIDOfAllIncludedLayouts",
            "createNewRevisionInAllIncludedLayouts",
            "isIssued"
        ]
    },
    "RevisionChange": {
        "type": "object",
        "properties": {
            "id": {
                "type": "string"
            },
            "description": {
                "type": "string"
            },
            "lastModifiedTime": {
                "type": "string"
            },
            "modifiedByUser": {
                "type": "string"
            },
            "isIssued": {
                "type": "boolean"
            },
            "firstRevisionIssueId": {
                "$ref": "#/RevisionIssueId",
                "description": "The identifier of the first issue in which the given change is issued."
            },
            "isArchived": {
                "type": "boolean"
            },
            "customSchemeData": {
                "$ref": "#/RevisionCustomSchemeData"
            }
        },
        "required": [
            "id",
            "description",
            "lastModifiedTime",
            "modifiedByUser",
            "isIssued",
            "isArchived"
        ]
    },
    "LayoutInfo": {
        "type": "object",
        "properties": {
            "id": {
                "type": "string"
            },
            "databaseId": {
                "$ref": "#/DatabaseId"
            },
            "name": {
                "type": "string"
            },
            "masterLayoutName": {
                "type": "string"
            },
            "width": {
                "type": "number"
            },
            "height": {
                "type": "number"
            },
            "subsetId": {
                "type": "string"
            },
            "subsetName": {
                "type": "string"
            },
            "ownerUser": {
                "type": "string"
            },
            "customSchemeData": {
                "$ref": "#/RevisionCustomSchemeData"
            }
        },
        "required": [
            "id",
            "databaseId",
            "name",
            "masterLayoutName",
            "width",
            "height",
            "subsetId",
            "subsetName",
            "ownerUser"
        ]
    },
    "DocumentRevision": {
        "type": "object",
        "properties": {
            "revisionId": {
                "$ref": "#/DocumentRevisionId"
            },
            "id": {
                "type": "string"
            },
            "finalId": {
                "type": "string"
            },
            "ownerUser": {
                "type": "string"
            },
            "status": {
                "type": "string",
                "enum": [
                    "Actual",
                    "Issued"
                ]
            },
            "changes": {
                "type": "array",
                "description": "All changes belong to the given document revision.",
                "items": {
                    "type": "object",
                    "properties": {
                        "id": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "id"
                    ]
                }
            },
            "layoutInfo": {
                "$ref": "#/LayoutInfo"
            }
        },
        "required": [
            "revisionId",
            "id",
            "finalId",
            "ownerUser",
            "status",
            "layoutInfo"
        ]
    },
    "RevisionChangesArrayItem": {
        "type": "object",
        "description": "A wrapper containing an array of revision changes",
        "properties": {
            "revisionChanges": {
                "type": "array",
                "items": {
                    "$ref": "#/RevisionChange"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionChange"
        ]
    },
    "RevisionChangesOfEntities": {
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/RevisionChangesArrayItem"
            },
            {
                "$ref": "#/ErrorItem"
            }
        ]
    },
    "StoryParameters": {
        "type": "object",
        "description": "Represents all parameters of a single project story, including its unique identifiers. Used in API responses.",
        "properties": {
            "index": {
                "type": "integer",
                "description": "The story index."
            },
            "floorId": {
                "type": "integer",
                "description": "Unique ID of the story."
            },
            "dispOnSections": {
                "type": "boolean",
                "description": "Story level lines should appear on sections and elevations."
            },
            "level": {
                "type": "number",
                "description": "The story level."
            },
            "name": {
                "type": "string",
                "description": "The name of the story."
            }
        },
        "additionalProperties": false,
        "required": [
            "index",
            "floorId",
            "dispOnSections",
            "level",
            "name"
        ]
    },
    "StoriesParameters": {
        "type": "array",
        "description": "A list of project stories, each with their complete parameters.",
        "items": {
            "$ref": "#/StoryParameters"
        }

    },
    "StorySettings": {
        "type": "object",
        "description": "Contains the configurable settings for creating or modifying a story. Used as input in API requests.",
        "properties": {
            "dispOnSections": {
                "type": "boolean",
                "description": "Story level lines should appear on sections and elevations."
            },
            "level": {
                "type": "number",
                "description": "The story level."
            },
            "name": {
                "type": "string",
                "description": "The name of the story."
            }
        },
        "additionalProperties": true,
        "required": [
            "dispOnSections",
            "level",
            "name"
        ]
    },
    "StoriesSettings": {
        "type": "array",
        "description": "A list of story settings, used as input for creating or modifying multiple stories.",
        "items": {
            "$ref": "#/StorySettings"
        }
    },
    "AutomaticZoneGeometry": {
        "type": "object",
        "description": "Automatic zone placement.",
        "properties": {
            "referencePosition": {
                "$ref": "#/Coordinate2D",
                "description": "Reference point to automatically find zone."
            }
        },
        "additionalProperties": false,
        "required": [
            "referencePosition"
        ]
    },
    "ManualZoneGeometry": {
        "type": "object",
        "description": "Manual zone placement.",
        "properties": {
            "polygonCoordinates": {
                "type": "array",
                "description": "The 2D coordinates of the edge of the zone.",
                "items": {
                    "$ref": "#/Coordinate2D"
                },
                "minItems": 3
            },
            "polygonArcs": {
                "type": "array",
                "description": "Polygon outline arcs of the zone.",
                "items": {
                    "$ref": "#/PolyArc"
                }
            },
            "holes": {
                "$ref": "#/Holes2D"
            }
        },
        "additionalProperties": false,
        "required": [
            "polygonCoordinates"
        ]
    },
    "WallSettings": {
        "type": "object",
        "description": "Settings for modifying a wall.",
        "properties": {
            "begCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "endCoordinate": {
                "$ref": "#/Coordinate2D"
            },
            "height": {
                "type": "number",
                "description": "height relative to bottom"
            },
            "bottomOffset": {
                "type": "number",
                "description": "base level of the wall relative to the floor level"
            },
            "offset": {
                "type": "number",
                "description": "wall's base line's offset from ref. line"
            },
            "begThickness": {
                "type": "number",
                "description": "Thickness at the beginning in case of trapezoid wall"
            },
            "endThickness": {
                "type": "number",
                "description": "Thickness at the end in case of trapezoid wall"
            }
        },
        "required": []
    },
    "TypeSpecificSettings": {
        "description": "Defines the modifiable type-specific settings for an element. Used as input for SET requests.",
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/WallSettings"
            }
        ]
    },
    "PropertyGroup": {
        "description": "Represents a property group.",
        "type": "object",
        "properties": {
            "name": {
                "type": "string"
            },
            "description": {
                "type": "string"
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    },
    "PropertyGroupArrayItem": {
        "description": "A wrapper containing a property group",
        "type": "object",
        "properties": {
            "propertyGroup": {
                "$ref": "#/PropertyGroup"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroup"
        ]
    },
    "PropertyDefinition": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string"
            },
            "description": {
                "type": "string"
            },
            "type": {
                "$ref": "#/PropertyDataType"
            },
            "isEditable": {
                "type": "boolean"
            },
            "defaultValue": {
                "$ref": "#/PropertyDefaultValue"
            },
            "possibleEnumValues": {
                "type": "array",
                "description": "The possible enum values of the property when the property type is enumeration.",
                "items": {
                    "type": "object",
                    "properties": {
                        "enumValue": {
                            "type": "object",
                            "description": "The description of an enumeration value.",
                            "properties": {
                                "enumValueId": {
                                    "$ref": "#/EnumValueId"
                                },
                                "displayValue": {
                                    "type": "string",
                                    "description": "Displayed value of the enumeration."
                                },
                                "nonLocalizedValue": {
                                    "type": "string",
                                    "description": "Nonlocalized value of the enumeration if there is one."
                                }
                            },
                            "required": [
                                "displayValue"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "enumValue"
                    ]
                }
            },
            "availability": {
                "type": "array",
                "description": "The identifiers of classification items the new property is available for.",
                "items": {
                    "$ref": "#/ClassificationItemIdArrayItem"
                }
            },
            "group": {
                "type": "object",
                "description": "The property group defined by name or id. If both fields exists the id will be used.",
                "properties": {
                    "propertyGroupId": {
                        "$ref": "#/PropertyGroupId"
                    },
                    "name": {
                        "type": "string"
                    }
                },
                "additionalProperties": false,
                "required": []
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "description",
            "type",
            "isEditable",
            "availability",
            "group"
        ]

    },
    "PropertyDefinitionArrayItem": {
        "description": "A wrapper containing a property definition",
        "type": "object",
        "properties": {
            "propertyDefinition": {
                "$ref": "#/PropertyDefinition"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyDefinition"
        ]
    }
};