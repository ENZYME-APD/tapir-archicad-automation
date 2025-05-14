var gCommands = [{
            "name": "Application Commands",
            "commands": [{
                "name": "GetAddOnVersion",
                "version": "0.1.0",
                "description": "Retrieves the version of the Tapir Additional JSON Commands Add-On.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "version": {
                "type": "string",
                "description": "Version number in the form of \"1.1.1\".",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "version"
        ]
    }
            },{
                "name": "GetArchicadLocation",
                "version": "0.1.0",
                "description": "Retrieves the location of the currently running Archicad executable.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "archicadLocation": {
                "type": "string",
                "description": "The location of the Archicad executable in the filesystem.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "archicadLocation"
        ]
    }
            },{
                "name": "QuitArchicad",
                "version": "0.1.0",
                "description": "Performs a quit operation on the currently running Archicad instance.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetCurrentWindowType",
                "version": "1.0.7",
                "description": "Returns the type of the current (active) window.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "currentWindowType": {
                "$ref": "#/WindowType"
            }
        },
        "additionalProperties": false,
        "required": [
            "currentWindowType"
        ]
    }
            }]
        },{
            "name": "Project Commands",
            "commands": [{
                "name": "GetProjectInfo",
                "version": "0.1.0",
                "description": "Retrieves information about the currently loaded project.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "isUntitled": {
                "type": "boolean",
                "description": "True, if the project is not saved yet."
            },
            "isTeamwork": {
                "type": "boolean",
                "description": "True, if the project is a Teamwork (BIMcloud) project."
            },
            "projectLocation": {
                "type": "string",
                "description": "The location of the project in the filesystem or a BIMcloud project reference.",
                "minLength": 1
            },
            "projectPath": {
                "type": "string",
                "description": "The path of the project. A filesystem path or a BIMcloud server relative path.",
                "minLength": 1
            },
            "projectName": {
                "type": "string",
                "description": "The name of the project.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "isUntitled",
            "isTeamwork"
        ]
    }
            },{
                "name": "GetProjectInfoFields",
                "version": "0.1.2",
                "description": "Retrieves the names and values of all project info fields.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "fields": {
                "type": "array",
                "description": "A list of project info fields.",
                "items": {
                    "type": "object",
                    "properties": {
                        "projectInfoId": {
                            "type": "string",
                            "description": "The id of the project info field."
                        },
                        "projectInfoName": {
                            "type": "string",
                            "description": "The name of the project info field visible on UI."
                        },
                        "projectInfoValue": {
                            "type": "string",
                            "description": "The value of the project info field."
                        }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "fields"
        ]
    }
            },{
                "name": "SetProjectInfoField",
                "version": "0.1.2",
                "description": "Sets the value of a project info field.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectInfoId": {
                "type": "string",
                "description": "The id of the project info field.",
                "minLength": 1
            },
            "projectInfoValue": {
                "type": "string",
                "description": "The new value of the project info field.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoId",
            "projectInfoValue"
        ]
    },
                "outputScheme": null
            },{
                "name": "GetStories",
                "version": "1.1.5",
                "description": "Retrieves information about the story sructure of the currently loaded project.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "firstStory": {
                "type": "integer",
                "description": "First story index."
            },
            "lastStory": {
                "type": "integer",
                "description": "Last story index."
            },
            "actStory": {
                "type": "integer",
                "description": "Actual (currently visible in 2D) story index."
            },
            "skipNullFloor": {
                "type": "boolean",
                "description": "Floor indices above ground-floor level may start with 1 instead of 0."
            },
            "stories": {
                "type": "array",
                "description": "A list of project stories.",
                "items": {
                    "type": "object",
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
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "firstStory",
            "lastStory",
            "actStory",
            "skipNullFloor",
            "stories"
        ]
    }
            },{
                "name": "SetStories",
                "version": "1.1.5",
                "description": "Sets the story sructure of the currently loaded project.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "stories": {
                "type": "array",
                "description": "A list of project stories.",
                "items": {
                    "type": "object",
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
                }
            }
        },
        "additionalProperties": true,
        "required": [
            "stories"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetHotlinks",
                "version": "0.1.0",
                "description": "Gets the file system locations (path) of the hotlink modules. The hotlinks can have tree hierarchy in the project.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "hotlinks": {
                "$ref": "#/Hotlinks"
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinks"
        ]
    }
            },{
                "name": "OpenProject",
                "version": "1.0.7",
                "description": "Opens the given project.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectFilePath": {
                "type": "string",
                "description": "The target project file to open."
            }
        },
        "additionalProperties": false,
        "required": [
            "projectFilePath"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        },{
            "name": "Element Commands",
            "commands": [{
                "name": "GetSelectedElements",
                "version": "0.1.0",
                "description": "Gets the list of the currently selected elements.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "GetElementsByType",
                "version": "1.0.7",
                "description": "Returns the identifier of every element of the given type on the plan. It works for any type. Use the optional filter parameter for filtering.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType"
            },
            "filters": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            },
            "databases": {
                 "$ref": "#/Databases"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "executionResultForDatabases": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "GetAllElements",
                "version": "1.0.7",
                "description": "Returns the identifier of all elements on the plan. Use the optional filter parameter for filtering.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "filters": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            },
            "databases": {
                 "$ref": "#/Databases"
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "executionResultForDatabases": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "ChangeSelectionOfElements",
                "version": "1.0.7",
                "description": "Adds/removes a number of elements to/from the current selection.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "addElementsToSelection": {
                "$ref": "#/Elements"
            },
            "removeElementsFromSelection": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResultsOfAddToSelection": {
                "$ref": "#/ExecutionResults"
            },
            "executionResultsOfRemoveFromSelection": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResultsOfAddToSelection",
            "executionResultsOfRemoveFromSelection"
        ]
    }
            },{
                "name": "FilterElements",
                "version": "1.0.7",
                "description": "Tests an elements by the given criterias.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "filters": {
                "type": "array",
                "items": {
                    "$ref": "#/ElementFilter"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "GetDetailsOfElements",
                "version": "1.0.7",
                "description": "Gets the details of the given elements (geometry parameters etc).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "detailsOfElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "Details of an element.",
                    "properties": {
                        "type": {
                            "$ref": "#/ElementType"
                        },
                        "id": {
                            "type": "string"
                        },
                        "floorIndex": {
                            "type": "number"
                        },
                        "layerIndex": {
                            "type": "number"
                        },
                        "drawIndex": {
                            "type": "number"
                        },
                        "details": {
                            "type": "object",
                            "oneOf": [
                                {
                                    "title": "WallDetails",
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
                                            "$ref": "#/2DCoordinate"
                                        },
                                        "endCoordinate": {
                                            "$ref": "#/2DCoordinate"
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
                                                "$ref": "#/2DCoordinate"
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
                                {
                                    "title": "SlabDetails",
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
                                                "$ref": "#/2DCoordinate"
                                            }
                                        },
                                        "holes": {
                                            "type": "array",
                                            "description": "Holes of the slab.",
                                            "items": {
                                                "type": "object",
                                                "properties": {
                                                    "polygonOutline": {
                                                        "type": "array",
                                                        "description": "Polygon outline of the hole.",
                                                        "items": {
                                                            "$ref": "#/2DCoordinate"
                                                        }
                                                    }
                                                },
                                                "required": [
                                                    "polygonOutline"
                                                ]
                                            }
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
                                {
                                    "title": "ColumnDetails",
                                    "properties": {
                                        "origin": {
                                            "$ref": "#/2DCoordinate"
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
                                        "height",
                                        "bottomOffset"
                                    ]
                                },
                                {
                                    "title": "DetailWorksheetDetails",
                                    "properties": {
                                        "basePoint": {
                                            "$ref": "#/2DCoordinate",
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
                                                "$ref": "#/2DCoordinate"
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
                                {
                                    "title": "LibPartBasedElementDetails",
                                    "properties": {
                                        "libPart": {
                                            "$ref": "#/LibPartDetails"
                                        },
                                        "ownerElementId": {
                                            "$ref": "#/ElementId"
                                        }
                                    },
                                    "required": [
                                        "libPart"
                                    ]
                                },
                                {
                                    "title": "NotYetSupportedElementTypeDetails",
                                    "properties": {
                                        "error": {
                                            "type": "string"
                                        }
                                    },
                                    "required": [
                                        "error"
                                    ]
                                }
                            ]
                        }
                    },
                    "required": [
                        "type",
                        "id",
                        "floorIndex",
                        "layerIndex",
                        "drawIndex",
                        "details"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "detailsOfElements"
        ]
    }
            },{
                "name": "SetDetailsOfElements",
                "version": "1.0.7",
                "description": "Sets the details of the given elements (floor, layer, order etc).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementsWithDetails": {
                "type": "array",
                "description": "The elements with parameters.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "details": {
                            "type": "object",
                            "description": "Details of an element.",
                            "properties": {
                                "floorIndex": {
                                    "type": "number"
                                },
                                "layerIndex": {
                                    "type": "number"
                                },
                                "drawIndex": {
                                    "type": "number"
                                },
                                "typeSpecificDetails": {
                                    "type": "object",
                                    "oneOf": [
                                        {
                                            "title": "WallDetails",
                                            "properties": {
                                                "begCoordinate": {
                                                    "$ref": "#/2DCoordinate"
                                                },
                                                "endCoordinate": {
                                                    "$ref": "#/2DCoordinate"
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
                                        }
                                    ]
                                }
                            },
                            "required": []
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "details"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithDetails"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "Get3DBoundingBoxes",
                "version": "1.1.2",
                "description": "Get the 3D bounding box of elements. The bounding box is calculated from the global origin in the 3D view. The output is the array of the bounding boxes respective to the input array of elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "type": "object",
            "properties": {
            "boundingBoxes3D": {
                "$ref": "#/BoundingBoxes3D"
            }
        },
        "additionalProperties": false,
        "required": [
            "boundingBoxes3D"
        ]
    }
            },{
                "name": "GetSubelementsOfHierarchicalElements",
                "version": "1.0.6",
                "description": "Gets the subelements of the given hierarchical elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "subelements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "Subelements grouped by type.",
                    "properties": {
                        "cWallSegments": {
                            "$ref": "#/Elements"
                        },
                        "cWallFrames": {
                            "$ref": "#/Elements"
                        },
                        "cWallPanels": {
                            "$ref": "#/Elements"
                        },
                        "cWallJunctions": {
                            "$ref": "#/Elements"
                        },
                        "cWallAccessories": {
                            "$ref": "#/Elements"
                        },
                        "stairRisers": {
                            "$ref": "#/Elements"
                        },
                        "stairTreads": {
                            "$ref": "#/Elements"
                        },
                        "stairStructures": {
                            "$ref": "#/Elements"
                        },
                        "railingNodes": {
                            "$ref": "#/Elements"
                        },
                        "railingSegments": {
                            "$ref": "#/Elements"
                        },
                        "railingPosts": {
                            "$ref": "#/Elements"
                        },
                        "railingRailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingRailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingToprailEnds": {
                            "$ref": "#/Elements"
                        },
                        "railingToprailConnections": {
                            "$ref": "#/Elements"
                        },
                        "railingRails": {
                            "$ref": "#/Elements"
                        },
                        "railingToprails": {
                            "$ref": "#/Elements"
                        },
                        "railingHandrails": {
                            "$ref": "#/Elements"
                        },
                        "railingPatterns": {
                            "$ref": "#/Elements"
                        },
                        "railingInnerPosts": {
                            "$ref": "#/Elements"
                        },
                        "railingPanels": {
                            "$ref": "#/Elements"
                        },
                        "railingBalusterSets": {
                            "$ref": "#/Elements"
                        },
                        "railingBalusters": {
                            "$ref": "#/Elements"
                        },
                        "beamSegments": {
                            "$ref": "#/Elements"
                        },
                        "columnSegments": {
                            "$ref": "#/Elements"
                        }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "subelements"
        ]
    }
            },{
                "name": "GetConnectedElements",
                "version": "1.1.4",
                "description": "Gets connected elements of the given elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "connectedElementType": {
                "$ref": "#/ElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "connectedElementType"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "connectedElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elements": {
                            "$ref": "#/Elements"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elements"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectedElements"
        ]
    }
            },{
                "name": "HighlightElements",
                "version": "1.0.3",
                "description": "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "highlightedColors": {
                "type": "array",
                "description": "A list of colors to highlight elements.",
                "items": {
                    "type": "array",
                    "description": "Color of the highlighted element as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                    "items": {
                        "type": "integer"
                    },
                    "minItems": 4,
                    "maxItems": 4
                }
            },
            "wireframe3D": {
                "type": "boolean",
                "description" : "Optional parameter. Switch non highlighted elements in the 3D window to wireframe."
            },
            "nonHighlightedColor": {
                "type": "array",
                "description": "Optional parameter. Color of the non highlighted elements as an [r, g, b, a] array. Each component must be in the 0-255 range.",
                "items": {
                    "type": "integer"
                },
                "minItems": 4,
                "maxItems": 4
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "highlightedColors"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "MoveElements",
                "version": "1.0.2",
                "description": "Moves elements with a given vector.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementsWithMoveVectors": {
                "type": "array",
                "description": "The elements with move vector pairs.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "moveVector": {
                            "type": "object",
                            "description" : "Move vector of a 3D point.",
                            "properties" : {
                                "x": {
                                    "type": "number",
                                    "description" : "X value of the vector."
                                },
                                "y" : {
                                    "type": "number",
                                    "description" : "Y value of the vector."
                                },
                                "z" : {
                                    "type": "number",
                                    "description" : "Z value of the vector."
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "x",
                                "y",
                                "z"
                            ]
                        },
                        "copy": {
                            "type": "boolean",
                            "description" : "Optional parameter. If true, then a copy of the element will be moved. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "moveVector"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsWithMoveVectors"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "GetGDLParametersOfElements",
                "version": "1.0.8",
                "description": "Gets all the GDL parameters (name, type, value) of the given elements.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "elements": {
            "$ref": "#/Elements"
        }
    },
    "additionalProperties": false,
    "required": [
        "elements"
    ]
},
                "outputScheme": {
    "type": "object",
    "properties": {
        "gdlParametersOfElements": {
            "type": "array",
            "description": "The GDL parameters of elements.",
            "items": {
                "$ref": "#/GDLParameterList"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "gdlParametersOfElements"
    ]
}
            },{
                "name": "SetGDLParametersOfElements",
                "version": "1.0.8",
                "description": "Sets the given GDL parameters of the given elements.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "elementsWithGDLParameters": {
            "type": "array",
            "description": "The elements with GDL parameters dictionary pairs.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "gdlParameters": {
                        "$ref": "#/GDLParameterList"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "gdlParameters"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "elementsWithGDLParameters"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "GetClassificationsOfElements",
                "version": "1.0.7",
                "description": "Returns the classification of the given elements in the given classification systems. It works for subelements of hierarchal elements also.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "classificationSystemIds": {
                "$ref": "#/ClassificationSystemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "classificationSystemIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elementClassifications": {
                "$ref": "#/ElementClassificationsOrErrors",
                "description": "The list of element classification item identifiers. Order of the ids are the same as in the input. Non-existing elements or non-existing classification systems are represented by error objects."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementClassifications"
        ]
    }
            },{
                "name": "SetClassificationsOfElements",
                "version": "1.0.7",
                "description": "Sets the classifications of elements. In order to set the classification of an element to unclassified, omit the classificationItemId field. It works for subelements of hierarchal elements also.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementClassifications": {
                "$ref": "#/ElementClassifications"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementClassifications"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "CreateColumns",
                "version": "1.0.3",
                "description": "Creates Column elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "columnsData": {
                "type": "array",
                "description": "Array of data to create Columns.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Column.",
                    "properties": {
                        "coordinates": {
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
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "coordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "columnsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateSlabs",
                "version": "1.0.3",
                "description": "Creates Slab elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "slabsData": {
            "type": "array",
            "description": "Array of data to create Slabs.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Slab.",
                "properties" : {
                    "level": {
                        "type": "number",
                        "description" : "The Z coordinate value of the reference line of the slab."	
                    },
                    "polygonCoordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the edge of the slab.",
                        "items": {
                            "$ref": "#/2DCoordinate"
                        }
                    },
                    "holes" : {
                        "type": "array",
                        "description": "Array of parameters of holes.",
                        "items": {
                            "type": "object",
                            "description" : "The parameters of the hole.",
                            "properties" : {
                                "polygonCoordinates": { 
                                    "type": "array",
                                    "description": "The 2D coordinates of the edge of the slab.",
                                    "items": {
                                        "$ref": "#/2DCoordinate"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required" : [
                                "polygonCoordinates"
                            ]
                        }
                    }
                },
                "additionalProperties": false,
                "required" : [
                    "level",
                    "polygonCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "slabsData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateObjects",
                "version": "1.0.3",
                "description": "Creates Object elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "objectsData": {
                "type": "array",
                "description": "Array of data to create Objects.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Object.",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description" : "The name of the library part to use."	
                        },
                        "coordinates": {
                            "$ref": "#/3DCoordinate"
                        },
                        "dimensions": {
                            "$ref": "#/3DDimensions"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates",
                        "dimensions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "objectsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            }]
        },{
            "name": "Favorites Commands",
            "commands": [{
                "name": "ApplyFavoritesToElementDefaults",
                "version": "1.1.2",
                "description": "Apply the given favorites to element defaults.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favorites": {
                "type": "array",
                "description": "The favorites to apply.",
                "items": {
                    "type": "string",
                    "description": "The name of a favorite."
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "favorites"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "CreateFavoritesFromElements",
                "version": "1.1.2",
                "description": "Create favorites from the given elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favoritesFromElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "The identifier of the element and the name of the new favorite.",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "favorite": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "favorite"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "favoritesFromElements"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            }]
        },{
            "name": "Property Commands",
            "commands": [{
                "name": "GetAllProperties",
                "version": "1.1.3",
                "description": "Returns all user defined and built-in properties.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "properties": {
                "type": "array",
                "description": "A list of property identifiers.",
                "items": {
                    "$ref": "#/PropertyDetails"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "properties"
        ]
    }
            },{
                "name": "GetPropertyValuesOfElements",
                "version": "1.0.6",
                "description": "Returns the property values of the elements for the given property. It works for subelements of hierarchal elements also.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "properties": {
                "$ref": "#/PropertyIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements",
            "properties"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "propertyValuesForElements": {
                "$ref": "#/PropertyValuesOrErrorArray",
                "description": "List of property value lists. The order of the outer list is that of the given elements. The order of the inner lists are that of the given properties."
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyValuesForElements"
        ]
    }
            },{
                "name": "SetPropertyValuesOfElements",
                "version": "1.0.6",
                "description": "Sets the property values of elements. It works for subelements of hierarchal elements also.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementPropertyValues": {
                "$ref": "#/ElementPropertyValues"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPropertyValues"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "CreatePropertyGroups",
                "version": "1.0.7",
                "description": "Creates Property Groups based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyGroups": {
                "type": "array",
                "description": "The parameters of the new property groups.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroup": {
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
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroup"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroups"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "propertyGroupIds": {
                "type": "array",
                "description": "The identifiers of the created property groups.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroupId": {
                            "$ref": "#/PropertyGroupId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroupId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupIds"
        ]
    }
            },{
                "name": "DeletePropertyGroups",
                "version": "1.0.9",
                "description": "Deletes the given Custom Property Groups.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyGroupIds": {
                "type": "array",
                "description": "The identifiers of property groups to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyGroupId": {
                            "$ref": "#/PropertyGroupId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyGroupId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyGroupIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            },{
                "name": "CreatePropertyDefinitions",
                "version": "1.0.9",
                "description": "Creates Custom Property Definitions based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyDefinitions": {
                "type": "array",
                "description": "The parameters of the new properties.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyDefinition": {
                            "type": "object",
                            "properties": {
                                "name": {
                                    "type": "string"
                                },
                                "description": {
                                    "type": "string"
                                },
                                "type": {
                                    "$ref": "#/PropertyType"
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
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyDefinition"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyDefinitions"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "propertyIds": {
                "$ref" : "#/PropertyIdOrErrorArray"
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyIds"
        ]
    }
            },{
                "name": "DeletePropertyDefinitions",
                "version": "1.0.9",
                "description": "Deletes the given Custom Property Definitions.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyIds": {
                "type": "array",
                "description": "The identifiers of properties to delete.",
                "items": {
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
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            }]
        },{
            "name": "Attribute Commands",
            "commands": [{
                "name": "GetAttributesByType",
                "version": "1.1.3",
                "description": "Returns the details of every attribute of the given type.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeType": {
                "$ref": "#/AttributeType"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeType"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "attributes" : {
                "type": "array",
                "description" : "Details of attributes.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeId": {
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "number",
                            "description": "Index of the attribute."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name of the attribute."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "attributeId",
                        "index",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributes"
        ]
    }
            },{
                "name": "CreateLayers",
                "version": "1.0.3",
                "description": "Creates Layer attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layerDataArray": {
                "type": "array",
                "description" : "Array of data to create new Layers.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Layer.",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Name."
                        },
                        "isHidden": {
                            "type": "boolean",
                            "description": "Hide/Show."
                        },
                        "isLocked": {
                            "type": "boolean",
                            "description": "Lock/Unlock."
                        },
                        "isWireframe": {
                            "type": "boolean",
                            "description": "Force the model to wireframe."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Layer if exists with the same name. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "layerDataArray"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    }
            },{
                "name": "CreateBuildingMaterials",
                "version": "1.0.1",
                "description": "Creates Building Material attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "buildingMaterialDataArray": {
                "type": "array",
                "description" : "Array of data to create new Building Materials.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Building Material.",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Name."
                        },
                        "id": {
                            "type": "string",
                            "description": "Identifier."
                        },
                        "manufacturer": {
                            "type": "string",
                            "description": "Manufacturer."
                        },
                        "description": {
                            "type": "string",
                            "description": "Decription."
                        },
                        "connPriority": {
                            "type": "integer",
                            "description": "Intersection priority."
                        },
                        "cutFillIndex": {
                            "type": "integer",
                            "description": "Index of the Cut Fill."
                        },
                        "cutFillPen": {
                            "type": "integer",
                            "description": "Cut Fill Foreground Pen."
                        },
                        "cutFillBackgroundPen": {
                            "type": "integer",
                            "description": "Cut Fill Background Pen."
                        },
                        "cutSurfaceIndex": {
                            "type": "integer",
                            "description": "Index of the Cut Surface."
                        },
                        "thermalConductivity": {
                            "type": "number",
                            "description": "Thermal Conductivity."
                        },
                        "density": {
                            "type": "number",
                            "description": "Density."
                        },
                        "heatCapacity": {
                            "type": "number",
                            "description": "Heat Capacity."
                        },
                        "embodiedEnergy": {
                            "type": "number",
                            "description": "Embodied Energy."
                        },
                        "embodiedCarbon": {
                            "type": "number",
                            "description": "Embodied Carbon."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Building Material if exists with the same name. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "buildingMaterialDataArray"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    }
            },{
                "name": "CreateComposites",
                "version": "1.0.2",
                "description": "Creates Composite attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "compositeDataArray": {
                "type": "array",
                "description" : "Array of data to create Composites.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Composite.",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Name."
                        },
                        "useWith": {
                            "type": "array",
                            "description" : "Array of types the composite can used with.",
                            "items": {
                                "type": "string",
                                "description": "Element type (Wall, Slab, Roof, or Shell)"
                            }
                        },
                        "skins": {
                            "type": "array",
                            "description" : "Array of skin data.",
                            "items" : {
                                "type": "object",
                                "description" : "Data to represent a skin.",
                                "properties" : {
                                    "type": {
                                        "type": "string",
                                        "description" : "Skin type (Core, Finish, or Other)"
                                    },
                                    "buildingMaterialId" : {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "framePen" : {
                                        "type": "integer",
                                        "description" : "Skin frame pen index."
                                    },
                                    "thickness" : {
                                        "type": "number",
                                        "description" : "Skin thickness (in meters)."
                                    }
                                },
                                "additionalProperties": false,
                                "required" : [
                                    "type",
                                    "buildingMaterialId",
                                    "framePen",
                                    "thickness"
                                ]
                            }
                        },
                        "separators": {
                            "type": "array",
                            "description" : "Array of skin separator data. The number of items must be the number of skins plus one.",
                            "items" : {
                                "type": "object",
                                "description" : "Data to represent a skin separator.",
                                "properties" : {
                                    "lineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "linePen" : {
                                        "type": "integer",
                                            "description" : "Separator line pen index."
                                    }
                                },
                                "additionalProperties": false,
                                "required" : [
                                    "lineTypeId",
                                    "linePen"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name",
                        "skins",
                        "separators"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description" : "Overwrite the Composite if exists with the same name. The default is false."
            }
        },
        "additionalProperties": false,
        "required" : [
            "compositeDataArray"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    }
            },{
                "name": "GetBuildingMaterialPhysicalProperties",
                "version": "0.1.3",
                "description": "Retrieves the physical properties of the given Building Materials.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "properties" : {
                "type": "array",
                "description" : "Physical properties list.",
                "items": {
                    "type": "object",
                    "properties": {
                        "properties": {
                            "type": "object",
                            "description": "Physical properties.",
                            "properties": {
                                "thermalConductivity": {
                                    "type": "number",
                                    "description": "Thermal Conductivity."
                                },
                                "density": {
                                    "type": "number",
                                    "description": "Density."
                                },
                                "heatCapacity": {
                                    "type": "number",
                                    "description": "Heat Capacity."
                                },
                                "embodiedEnergy": {
                                    "type": "number",
                                    "description": "Embodied Energy."
                                },
                                "embodiedCarbon": {
                                    "type": "number",
                                    "description": "Embodied Carbon."
                                }
                            }
                        }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "properties"
        ]
    }
            }]
        },{
            "name": "Library Commands",
            "commands": [{
                "name": "GetLibraries",
                "version": "1.0.1",
                "description": "Gets the list of loaded libraries.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "A list of project libraries.",
                "items": {
                    "type": "object",
                    "description": "Library",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Library name."
                        },
                        "path": {
                            "type": "string",
                            "description": "A filesystem path to library location."
                        },
                        "type": {
                            "type": "string",
                            "description": "Library type."
                        },
                        "available": {
                            "type": "boolean",
                            "description": "Is library not missing."
                        },
                        "readOnly": {
                            "type": "boolean",
                            "description": "Is library not writable."
                        },
                        "twServerUrl": {
                            "type": "string",
                            "description": "URL address of the TeamWork server hosting the library."
                        },
                        "urlWebLibrary": {
                            "type": "string",
                            "description": "URL of the downloaded Internet library."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "type",
                        "path"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    }
            },{
                "name": "ReloadLibraries",
                "version": "1.0.0",
                "description": "Executes the reload libraries command.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        },{
            "name": "Teamwork Commands",
            "commands": [{
                "name": "TeamworkSend",
                "version": "0.1.0",
                "description": "Performs a send operation on the currently opened Teamwork project.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "TeamworkReceive",
                "version": "0.1.0",
                "description": "Performs a receive operation on the currently opened Teamwork project.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "ReserveElements",
                "version": "1.1.4",
                "description": "Reserves elements in Teamwork mode.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "conflicts": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "user": {
                            "type": "object",
                            "properties": {
                                "userId": {
                                    "type": "number"
                                },
                                "userName": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "userId",
                                "userName"
                            ]
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "user"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    }
            },{
                "name": "ReleaseElements",
                "version": "1.1.4",
                "description": "Releases elements in Teamwork mode.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        },{
            "name": "Navigator Commands",
            "commands": [{
                "name": "PublishPublisherSet",
                "version": "0.1.0",
                "description": "Performs a publish operation on the currently opened project. Only the given publisher set will be published.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "publisherSetName": {
                "type": "string",
                "description": "The name of the publisher set.",
                "minLength": 1
            },
            "outputPath": {
                "type": "string",
                "description": "Full local or LAN path for publishing. Optional, by default the path set in the settings of the publiser set will be used.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "publisherSetName"
        ]
    },
                "outputScheme": null
            },{
                "name": "UpdateDrawings",
                "version": "1.1.4",
                "description": "Performs a drawing update on the given elements.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "elements": {
            "$ref": "#/Elements"
        }
    },
    "additionalProperties": false,
    "required": [
        "elements"
    ]
},
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetDatabaseIdFromNavigatorItemId",
                "version": "1.1.4",
                "description": "Gets the ID of the database associated with the supplied navigator item id",
                "inputScheme": {
    "type": "object",
    "properties": {
        "navigatorItemIds": {
            "$ref": "#/NavigatorItemIds"
        }
    },
    "additionalProperties": false,
    "required": [
        "navigatorItemIds"
    ]
},
                "outputScheme": {
    "type": "object",
    "properties": {
        "databases": {
            "$ref": "#/Databases"
        }
    },
    "additionalProperties": false,
    "required": [
        "databases"
    ]
}
            },{
                "name": "GetModelViewOptions",
                "version": "1.1.4",
                "description": "Gets all model view options",
                "inputScheme": null,
                "outputScheme": {
    "type": "object",
    "properties": {
        "modelViewOptions": {
            "type": "array",
            "item": {
                "type": "object",
                "description": "Represents the model view options.",
                "properties": {
                    "name": {
                        "type": "string"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "name"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "modelViewOptions"
    ]
}
            },{
                "name": "GetViewSettings",
                "version": "1.1.4",
                "description": "Gets the view settings of navigator items",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIds": {
                "$ref": "#/NavigatorItemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "navigatorItemIds"
        ]
    },
                "outputScheme": {
    "type": "object",
    "properties": {
        "viewSettings": {
            "type": "array",
            "item": {
                "type": "object",
                "description": "The settings of a navigator view or an error.",
                "oneOf": [
                    {
                        "$ref": "#/ViewSettings"
                    },
                    {
                        "$ref": "#/ErrorItem"
                    }
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "viewSettings"
    ]
}
            },{
                "name": "SetViewSettings",
                "version": "1.1.4",
                "description": "Sets the view settings of navigator items",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIdsWithViewSettings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId"
                        },
                        "viewSettings": {
                            "$ref": "#/ViewSettings"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "navigatorItemId",
                        "viewSettings"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "navigatorItemIdsWithViewSettings"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResults"
        ]
    }
            }]
        },{
            "name": "Issue Management Commands",
            "commands": [{
                "name": "CreateIssue",
                "version": "1.0.2",
                "description": "Creates a new issue.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the issue."
            },
            "parentIssueId": {
                "$ref": "#/IssueId"
            },
            "tagText": {
                "type": "string",
                "description": "Tag text of the issue, optional."
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    },
                "outputScheme": {
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
    }
            },{
                "name": "DeleteIssue",
                "version": "1.0.2",
                "description": "Deletes the specified issue.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "acceptAllElements": {
                "type": "boolean",
                "description": "Accept all creation/deletion/modification of the deleted issue. By default false."
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetIssues",
                "version": "1.0.2",
                "description": "Retrieves information about existing issues.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "issues": {
                "type": "array",
                "description": "A list of existing issues.",
                "items": {
                    "type": "object",
                    "properties": {
                        "issueId": {
                            "$ref": "#/IssueId"
                        },
                        "name": {
                            "type": "string",
                            "description": "Issue name"
                        },
                        "parentIssueId": {
                            "$ref": "#/IssueId"
                        },
                        "creaTime": {
                            "type": "integer",
                            "description": "Issue creation time"
                        },
                        "modiTime": {
                            "type": "integer",
                            "description": "Issue modification time"
                        },
                        "tagText": {
                            "type": "string",
                            "description": "Issue tag text - labels"
                        },
                        "tagTextElementId": {
                            "$ref": "#/ElementId"
                        },
                        "isTagTextElemVisible": {
                            "type": "boolean",
                            "description": "The visibility of the attached tag text element"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "issueId",
                        "name",
                        "parentIssueId",
                        "creaTime",
                        "modiTime",
                        "tagText",
                        "tagTextElementId",
                        "isTagTextElemVisible"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "issues"
        ]
    }
            },{
                "name": "AddCommentToIssue",
                "version": "1.0.6",
                "description": "Adds a new comment to the specified issue.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "author": {
                "type": "string",
                "description": "The author of the new comment."
            },
            "status": {
                "$ref": "#/IssueCommentStatus"
            },
            "text": {
                "type": "string",
                "description": "Comment text to add."
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "text"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetCommentsFromIssue",
                "version": "1.0.6",
                "description": "Retrieves comments information from the specified issue.",
                "inputScheme": {
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
                "outputScheme": {
        "type": "object",
        "properties": {
            "comments": {
                "type": "array",
                "description": "A list of existing comments.",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": {
                            "$ref": "#/Guid",
                            "description": "Comment identifier"
                        },
                        "author": {
                            "type": "string",
                            "description": "Comment author"
                        },
                        "text": {
                            "type": "string",
                            "description": "Comment text"
                        },
                        "status": {
                            "$ref": "#/IssueCommentStatus"
                        },
                        "creaTime": {
                            "type": "integer",
                            "description": "Comment creation time"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "guid",
                        "author",
                        "text",
                        "status",
                        "creaTime"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "comments"
        ]
    }
            },{
                "name": "AttachElementsToIssue",
                "version": "1.0.6",
                "description": "Attaches elements to the specified issue.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "elements": {
                "$ref": "#/Elements"
            },
            "type": {
                "$ref": "#/IssueElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "elements",
            "type"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "DetachElementsFromIssue",
                "version": "1.0.6",
                "description": "Detaches elements from the specified issue.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "elements"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetElementsAttachedToIssue",
                "version": "1.0.6",
                "description": "Retrieves attached elements of the specified issue, filtered by attachment type.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issueId": {
                "$ref": "#/IssueId"
            },
            "type": {
                "$ref": "#/IssueElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "issueId",
            "type"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": true,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "ExportIssuesToBCF",
                "version": "1.0.6",
                "description": "Exports specified issues to a BCF file.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "issues": {
                "$ref": "#/Issues",
                "description": "Leave it empty to export all issues."
            },
            "exportPath": {
                "type": "string",
                "description": "The os path to the bcf file, including it's name."
            },
            "useExternalId": {
                "type": "boolean",
                "description": "Use external IFC ID or Archicad IFC ID as referenced in BCF topics."
            },
            "alignBySurveyPoint": {
                "type": "boolean",
                "description": "Align BCF views by Archicad Survey Point or Archicad Project Origin."
            }
        },
        "additionalProperties": false,
        "required": [
            "exportPath",
            "useExternalId",
            "alignBySurveyPoint"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "ImportIssuesFromBCF",
                "version": "1.0.6",
                "description": "Imports issues from the specified BCF file.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "importPath": {
                "type": "string",
                "description": "The os path to the bcf file, including it's name."
            },
            "alignBySurveyPoint": {
                "type": "boolean",
                "description": "Align BCF views by Archicad Survey Point or Archicad Project Origin."
            }
        },
        "additionalProperties": false,
        "required": [
            "importPath",
            "alignBySurveyPoint"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        },{
            "name": "Developer Commands",
            "commands": [{
                "name": "GenerateDocumentation",
                "version": "1.0.7",
                "description": "Generates files for the documentation. Used by Tapir developers only.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "destinationFolder": {
                "type": "string",
                "description": "Destination folder for the generated documentation files.",
                "minLength": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "destinationFolder"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        }];