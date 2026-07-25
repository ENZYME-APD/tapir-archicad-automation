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
            },{
                "name": "ChangeWindow",
                "version": "1.3.1",
                "description": "Changes the current (active) window to the given window.",
                "inputScheme": {
        "$ref": "#/NavigatorItemIdOrDatabaseIdAndWindowType"
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
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
                "$ref": "#/ProjectInfoFields"
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
                "name": "CreateProjectInfoFields",
                "version": "1.5.2",
                "description": "Creates one or more custom project info fields.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectInfoFields": {
                "type": "array",
                "description": "Array of custom project info fields to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "projectInfoName": {
                            "type": "string",
                            "description": "Display name of the project info field.",
                            "minLength": 1
                        },
                        "projectInfoValue": {
                            "type": "string",
                            "description": "Initial value of the project info field."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "projectInfoName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoFields"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "fields": {
                "$ref": "#/ProjectInfoFields"
            }
        },
        "additionalProperties": false,
        "required": [
            "fields"
        ]
    }
            },{
                "name": "DeleteProjectInfoFields",
                "version": "1.5.4",
                "description": "Deletes one or more custom project info fields. Hardcoded fields cannot be deleted.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectInfoIds": {
                "type": "array",
                "description": "List of project info field ids to delete. Only custom fields (ids starting with 'autotext-') can be deleted.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "projectInfoIds"
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
                "$ref": "#/StoriesParameters"
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
                "$ref": "#/StoriesSettings"
            }
        },
        "additionalProperties": false,
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
            },{
                "name": "CloseProject",
                "version": "1.3.1",
                "description": "Closes the currently opened project.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "SaveProject",
                "version": "1.3.1",
                "description": "Saves the currently opened project.",
                "inputScheme": null,
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetCalculationUnits",
                "version": "1.4.0",
                "description": "Gets the project calculation units.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "length": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/LengthType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for length values."
                    },
                    "roundInch": {
                        "type": "integer",
                        "description": "Fractional inches."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "area": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/AreaType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for area values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "volume": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/VolumeType"
                    },
                    "accuracy": {
                        "$ref": "#/AccuracyType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for volume values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "accuracy",
                    "decimals"
                ]
            },
            "angle": {
                "type": "object",
                "properties": {
                    "unit": {
                        "$ref": "#/AngleType"
                    },
                    "decimals": {
                        "type": "integer",
                        "description": "Number of decimals to display for angle values."
                    },
                    "accuracy": {
                        "type": "integer",
                        "description": "Accuracy for angle values."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "unit",
                    "decimals",
                    "accuracy"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "length",
            "area",
            "volume",
            "angle"
        ]
    }
            },{
                "name": "GetGeoLocation",
                "version": "1.1.6",
                "description": "Gets the project location details.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "projectLocation": {
                "type": "object",
                "properties": {
                    "longitude": {
                        "type": "number",
                        "description": "longitude in degrees"
                    },
                    "latitude": {
                        "type": "number",
                        "description": "latitude in degrees"
                    },
                    "altitude": {
                        "type": "number",
                        "description": "altitude in meters"
                    },
                    "north": {
                        "type": "number",
                        "description": "north direction in radians"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "longitude",
                    "latitude",
                    "altitude",
                    "north"
                ]
            },
            "surveyPoint": {
                "type": "object",
                "properties": {
                    "position": {
                        "type": "object",
                        "properties": {
                            "eastings": {
                                "type": "number",
                                "description": "Location along the easting of the coordinate system of the target map coordinate reference system."
                            },
                            "northings": {
                                "type": "number",
                                "description": "Location along the northing of the coordinate system of the target map coordinate reference system."
                            },
                            "elevation": {
                                "type": "number",
                                "description": "Orthogonal height relative to the vertical datum specified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "eastings",
                            "northings",
                            "elevation"
                        ]
                    },
                    "geoReferencingParameters": {
                        "type": "object",
                        "properties": {
                            "crsName": {
                                "type": "string",
                                "description": "Name by which the coordinate reference system is identified."
                            },
                            "description": {
                                "type": "string",
                                "description": "Informal description of this coordinate reference system."
                            },
                            "geodeticDatum": {
                                "type": "string",
                                "description": "Name by which this datum is identified."
                            },
                            "verticalDatum": {
                                "type": "string",
                                "description": "Name by which the vertical datum is identified."
                            },
                            "mapProjection": {
                                "type": "string",
                                "description": "Name by which the map projection is identified."
                            },
                            "mapZone": {
                                "type": "string",
                                "description": "Name by which the map zone, relating to the MapProjection, is identified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "crsName",
                            "description",
                            "geodeticDatum",
                            "verticalDatum",
                            "mapProjection",
                            "mapZone"
                        ]
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position",
                    "geoReferencingParameters"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "projectLocation",
            "surveyPoint"
        ]
    }
            },{
                "name": "SetGeoLocation",
                "version": "1.2.9",
                "description": "Sets the project location details.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectLocation": {
                "type": "object",
                "properties": {
                    "longitude": {
                        "type": "number",
                        "description": "longitude in degrees"
                    },
                    "latitude": {
                        "type": "number",
                        "description": "latitude in degrees"
                    },
                    "altitude": {
                        "type": "number",
                        "description": "altitude in meters"
                    },
                    "north": {
                        "type": "number",
                        "description": "north direction in radians"
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            },
            "surveyPoint": {
                "type": "object",
                "properties": {
                    "position": {
                        "type": "object",
                        "properties": {
                            "eastings": {
                                "type": "number",
                                "description": "Location along the easting of the coordinate system of the target map coordinate reference system."
                            },
                            "northings": {
                                "type": "number",
                                "description": "Location along the northing of the coordinate system of the target map coordinate reference system."
                            },
                            "elevation": {
                                "type": "number",
                                "description": "Orthogonal height relative to the vertical datum specified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                        ]
                    },
                    "geoReferencingParameters": {
                        "type": "object",
                        "properties": {
                            "crsName": {
                                "type": "string",
                                "description": "Name by which the coordinate reference system is identified."
                            },
                            "description": {
                                "type": "string",
                                "description": "Informal description of this coordinate reference system."
                            },
                            "geodeticDatum": {
                                "type": "string",
                                "description": "Name by which this datum is identified."
                            },
                            "verticalDatum": {
                                "type": "string",
                                "description": "Name by which the vertical datum is identified."
                            },
                            "mapProjection": {
                                "type": "string",
                                "description": "Name by which the map projection is identified."
                            },
                            "mapZone": {
                                "type": "string",
                                "description": "Name by which the map zone, relating to the MapProjection, is identified."
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                        ]
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "PrintView",
                "version": "1.3.1",
                "description": "Prints from the current view.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "grid": {
                "type": "boolean",
                "description": "Print the grid. The default is false."
            },
            "fixText": {
                "type": "boolean",
                "description": "Use fixed text size. The default is false."
            },
            "scale": {
                "type": "integer",
                "description": "Print scale. The default is 100."
            },
            "printArea": {
                "type": "string",
                "description": "The area to print. The default is 'currentView'.",
                "enum": ["currentView", "entireDrawing", "marquee"]
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "RebuildView",
                "version": "1.5.0",
                "description": "Rebuilds the current view.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "regenerate": {
                "type": "boolean",
                "description": "Regenerate the view. The default is false, meaning the view will not be regenerated, but rebuilt."
            }
        },
        "additionalProperties": false,
        "required": [
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
        "$ref": "#/ElementsWithExecutionResultsOrError"
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
        "$ref": "#/ElementsWithExecutionResultsOrError"
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
                            "$ref": "#/TypeSpecificDetails"
                        },
                        "floorPlanPolygons": {
                            "type": "array",
                            "description": "Cut-fill polygons as drawn on the floor plan (wall joins resolved by ArchiCAD). Available for elements with a cut-fill representation (walls, columns, beams). Absent when the element has no cut fill or when the floor plan database is not accessible.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "coordinates": {
                                        "type": "array",
                                        "items": {
                                            "$ref": "#/Coordinate2D"
                                        }
                                    }
                                }
                            }
                        }
                    },
                    "additionalProperties": false,
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
                                    "$ref": "#/TypeSpecificSettings"
                                }
                            },
                            "additionalProperties": false,
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
                    },
                    "additionalProperties": false,
                    "required": []
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
        "$ref": "#/ConnectedElementsOrError"
    }
            },{
                "name": "GetZoneBoundaries",
                "version": "1.2.3",
                "description": "Gets the boundaries of the given Zone (connected elements, neighbour zones, etc.).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "zoneElementId": {
                "$ref": "#/ElementId"
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneElementId"
        ]
    },
                "outputScheme": {
        "$ref": "#/ZoneBoundariesOrError"
    }
            },{
                "name": "UpdateZones",
                "version": "1.5.4",
                "description": "Updates all Zones (recalculates their geometry, updates their Zone Stamps and the connected elements).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "keepStampPosition": {
                "type": "boolean",
                "description": "Keep the position of the Zone Stamps. The default is true."
            },
            "undoTopTrim": {
                "type": "boolean",
                "description": "Undo the trimming of the top of the Zones. The default is false."
            },
            "undoBottomTrim": {
                "type": "boolean",
                "description": "Undo the trimming of the bottom of the Zones. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetCollisions",
                "version": "1.2.2",
                "description": "Detect collisions between the given two groups of elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementsGroup1": {
                "$ref": "#/Elements"
            },
            "elementsGroup2": {
                "$ref": "#/Elements"
            },
            "settings": {
                "type": "object",
                "properties": {
                    "volumeTolerance": {
                        "type": "number",
                        "description": "Intersection body volume greater then this value will be considered as a collision. Default value is 0.001."
                    },
                    "performSurfaceCheck": {
                        "type": "boolean",
                        "description": "Enables surface collision check. If disabled the surfaceTolerance value will be ignored. By default it's false."
                    },
                    "surfaceTolerance": {
                        "type": "number",
                        "description": "Intersection body surface area greater then this value will be considered as a collision. Default value is 0.001."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "volumeTolerance",
                    "performSurfaceCheck",
                    "surfaceTolerance"
                ]
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsGroup1",
            "elementsGroup2"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "collisions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId1": {
                            "$ref": "#/ElementId"
                        },
                        "elementId2": {
                            "$ref": "#/ElementId"
                        },
                        "hasBodyCollision": {
                            "type": "boolean"
                        },
                        "hasClearenceCollision": {
                            "type": "boolean"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId1",
                        "elementId2",
                        "hasBodyCollision",
                        "hasClearenceCollision"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "collisions"
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
                "name": "RotateElements",
                "version": "1.5.3",
                "description": "Rotates elements around a reference point.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementsWithRotations": {
                "type": "array",
                "description": "The elements with rotation settings.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "rotation": {
                            "type": "object",
                            "description": "Rotation parameters for an element.",
                            "properties": {
                                "beginPoint": {
                                    "type": "object",
                                    "description": "Starting point of the rotation arc.",
                                    "properties": {
                                        "x": { "type": "number" },
                                        "y": { "type": "number" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["x", "y"]
                                },
                                "endPoint": {
                                    "type": "object",
                                    "description": "End point of the rotation arc.",
                                    "properties": {
                                        "x": { "type": "number" },
                                        "y": { "type": "number" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["x", "y"]
                                },
                                "origin": {
                                    "type": "object",
                                    "description": "Center of rotation.",
                                    "properties": {
                                        "x": { "type": "number" },
                                        "y": { "type": "number" }
                                    },
                                    "additionalProperties": false,
                                    "required": ["x", "y"]
                                }
                            },
                            "additionalProperties": false,
                            "required": ["beginPoint", "endPoint", "origin"]
                        },
                        "copy": {
                            "type": "boolean",
                            "description": "Optional parameter. If true, a copy of the element is rotated. By default it's false."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId", "rotation"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["elementsWithRotations"]
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
                "name": "DeleteElements",
                "version": "1.2.1",
                "description": "Deletes elements.",
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
                "name": "LockElements",
                "version": "1.5.2",
                "description": "Locks the given elements. Manual lock, not teamwork!",
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
                "name": "UnlockElements",
                "version": "1.5.2",
                "description": "Unlocks the given elements. Manual lock, not teamwork!",
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
                        "$ref": "#/SetGDLParameterArray"
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
                        },
                        "height": {
                            "type": "number",
                            "description": "Optional column height.",
                            "exclusiveMinimum": 0.0
                        },
                        "axisRotationAngle": {
                            "type": "number",
                            "description": "Optional column rotation angle in radians."
                        },
                        "width": {
                            "type": "number",
                            "description": "Cross section width of the column. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "depth": {
                            "type": "number",
                            "description": "Cross section depth (height) of the column. Applied to all segments. Only effective for rectangular columns.",
                            "exclusiveMinimum": 0.0
                        },
                        "coreAnchor": {
                            "type": "string",
                            "description": "Optional anchor point of the column core on a 3x3 grid.",
                            "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"]
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
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
                "name": "CreateWalls",
                "version": "1.4.0",
                "description": "Creates Wall elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "wallsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Story index (as returned by GetStories). When provided, zCoordinate is interpreted as bottomOffset relative to the floor. Takes priority over zCoordinate for floor assignment." },
                        "zCoordinate": { "type": "number", "description": "Absolute Z when floorIndex is absent; bottomOffset relative to the floor when floorIndex is provided." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "offset": { "type": "number" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero creates a curved wall (begCoordinate/endCoordinate are the chord endpoints)." },
                        "referenceLineLocation": {
                            "type": "string",
                            "enum": ["Outside", "Center", "Inside", "CoreOutside", "CoreCenter", "CoreInside"]
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite", "Profile"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate", "height", "thickness"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["wallsData"]
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
                "name": "CreateBeams",
                "version": "1.4.0",
                "description": "Creates Beam elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "beamsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from zCoordinate." },
                        "zCoordinate": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": { "type": "number" },
                        "arcAngle": { "type": "number" },
                        "verticalCurveHeight": { "type": "number" },
                        "width": {
                            "type": "number",
                            "description": "Cross section width of the beam. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "height": {
                            "type": "number",
                            "description": "Cross section height of the beam. Applied to all segments.",
                            "exclusiveMinimum": 0.0
                        },
                        "anchorPoint": {
                            "type": "string",
                            "description": "Optional anchor point of the beam cross section on a 3x3 grid.",
                            "enum": ["TopLeft", "TopCenter", "TopRight", "MiddleLeft", "Center", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight"]
                        }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate", "zCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["beamsData"]
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
                "name": "CreateStairs",
                "version": "1.5.0",
                "description": "Creates Stair elements based on the given baseline and parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "stairsData": {
                "type": "array",
                "description": "Array of data to create Stair elements.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Stair.",
                    "properties": {
                        "baseLinePoints": {
                            "type": "array",
                            "description": "2D coordinates defining the stair baseline polyline. Minimum 2 points for a straight stair, 3+ for L-shaped or U-shaped stairs.",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 2
                        },
                        "zCoordinate": {
                            "type": "number",
                            "description": "The Z coordinate (absolute elevation) of the stair base."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from zCoordinate."
                        },
                        "totalHeight": {
                            "type": "number",
                            "description": "Total height of the stair.",
                            "exclusiveMinimum": 0.0
                        },
                        "flightWidth": {
                            "type": "number",
                            "description": "Width of the stair flight.",
                            "exclusiveMinimum": 0.0
                        },
                        "stepNum": {
                            "type": "integer",
                            "description": "Number of risers (steps).",
                            "minimum": 1
                        },
                        "riserHeight": {
                            "type": "number",
                            "description": "Height of each riser.",
                            "exclusiveMinimum": 0.0
                        },
                        "treadDepth": {
                            "type": "number",
                            "description": "Depth (going) of each tread.",
                            "exclusiveMinimum": 0.0
                        }
                    },
                    "additionalProperties": false,
                    "required": ["baseLinePoints", "zCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["stairsData"]
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
                    "thickness": {
                        "type": "number",
                        "description": "Optional slab thickness.",
                        "exclusiveMinimum": 0.0
                    },
                    "referencePlaneLocation": {
                        "type": "string",
                        "description": "Optional location of the slab reference plane. For a basic (homogeneous) slab only 'Top' or 'Bottom' are valid.",
                        "enum": ["Top", "CoreTop", "CoreBottom", "Bottom"]
                    },
                    "polygonCoordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the edge of the slab.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "polygonArcs": {
                        "type": "array",
                        "description": "Polygon outline arcs of the slab.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "holes" : {
                        "$ref": "#/Holes2D"
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from level."
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
                "name": "CreateWindows",
                "version": "1.4.0",
                "description": "Creates Window elements in host walls based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "windowsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerWallId": { "$ref": "#/ElementId" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional. Name of an existing Window favorite (as returned by `GetFavoritesByType`). Applied to the Window tool defaults before the create."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["ownerWallId", "centerOffset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["windowsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateDoors",
                "version": "1.4.0",
                "description": "Creates Door elements in host walls based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "doorsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerWallId": { "$ref": "#/ElementId" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional. Name of an existing Door favorite (as returned by `GetFavoritesByType`). Applied to the Door tool defaults before the create."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["ownerWallId", "centerOffset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["doorsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateOpenings",
                "version": "1.4.0",
                "description": "Creates Opening elements in the given host elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "openingsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ownerElementId": { "$ref": "#/ElementId" },
                        "basePoint": { "$ref": "#/Coordinate3D" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 }
                    },
                    "additionalProperties": false,
                    "required": ["ownerElementId", "basePoint"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["openingsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateMorphs",
                "version": "1.4.0",
                "description": "Creates Morph elements from simple box definitions.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "morphsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "basePoint": { "$ref": "#/Coordinate3D" },
                        "size": {
                            "$ref": "#/Dimensions3D",
                            "description": "Builds a simple axis-aligned box of this size. Mutually exclusive with `body` - give exactly one of the two."
                        },
                        "body": {
                            "$ref": "#/MorphBody",
                            "description": "Builds arbitrary geometry (any number of faces, holes, per-face materials, edge display overrides) instead of a simple box. Mutually exclusive with `size` - give exactly one of the two."
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "xAxis": { "$ref": "#/Coordinate3D" },
                        "yAxis": { "$ref": "#/Coordinate3D" },
                        "zAxis": { "$ref": "#/Coordinate3D" },
                        "surfaceId": { "$ref": "#/AttributeId" },
                        "castShadow": { "type": "boolean" },
                        "receiveShadow": { "type": "boolean" },
                        "isAutoOnStoryVisibility": { "type": "boolean" },
                        "showContour": { "$ref": "#/StoryVisibility" },
                        "showFill": { "$ref": "#/StoryVisibility" },
                        "linkToSettings": {
                            "type": "object",
                            "properties": {
                                "homeStoryDifference": { "type": "integer" },
                                "newCreationMode": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        },
                        "displayOption": {
                            "type": "string",
                            "enum": ["Standard", "StandardWithAbstract", "CutOnly", "OutLinesOnly", "AbstractAll", "CutAll"]
                        },
                        "viewDepthLimitation": {
                            "type": "string",
                            "enum": ["ToFloorPlanRange", "ToAbsoluteLimit", "EntireElement"]
                        },
                        "cutFillPen": { "type": "integer" },
                        "cutFillBackgroundPen": { "type": "integer" },
                        "cutLineType": { "$ref": "#/AttributeId" },
                        "cutLinePen": { "type": "integer" },
                        "uncutLineType": { "$ref": "#/AttributeId" },
                        "uncutLinePen": { "type": "integer" },
                        "overheadLineType": { "$ref": "#/AttributeId" },
                        "overheadLinePen": { "type": "integer" },
                        "useCoverFillType": { "type": "boolean" },
                        "outlineContourDisplay": { "type": "boolean" },
                        "coverFillType": { "$ref": "#/AttributeId" },
                        "coverFillPen": { "type": "integer" },
                        "coverFillBGPen": { "type": "integer" },
                        "use3DHatching": { "type": "boolean" },
                        "coverFillOrientation": {
                            "type": "object",
                            "properties": {
                                "type": { "type": "string", "enum": ["Global", "Rotated", "Distorted", "Centered"] },
                                "origo": { "$ref": "#/Coordinate2D" },
                                "matrix00": { "type": "number" },
                                "matrix10": { "type": "number" },
                                "matrix01": { "type": "number" },
                                "matrix11": { "type": "number" },
                                "innerRadius": { "type": "number" }
                            },
                            "additionalProperties": false
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "type": "string",
                            "enum": ["Invalid", "Planar", "Default", "Cylindric", "Spheric", "Box"]
                        },
                        "textureProjectionCoords": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate3D" },
                            "minItems": 4,
                            "maxItems": 4
                        },
                        "level": { "type": "number" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from the basePoint's z value." }
                    },
                    "additionalProperties": false,
                    "required": ["basePoint"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["morphsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateRoofs",
                "version": "1.4.0",
                "description": "Creates multi-plane Roof elements based on footprint, level and roof profile data.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "roofsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "level": { "type": "number" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from level." },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "polygonCoordinates": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": { "$ref": "#/Holes2D" },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "type": "object",
                                "properties": {
                                    "levelHeight": { "type": "number" },
                                    "levelAngle": { "type": "number", "exclusiveMinimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["levelHeight", "levelAngle"]
                            }
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["level", "polygonCoordinates"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["roofsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateAssociativeDimensions",
                "version": "1.4.0",
                "description": "Creates associative linear dimensions from explicit witness point references.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "direction": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "number" },
                        "witnessPoints": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "elementId": { "$ref": "#/ElementId" },
                                    "line": { "type": "boolean" },
                                    "inIndex": { "type": "integer" },
                                    "special": { "type": "integer" },
                                    "nodeType": { "type": "integer" },
                                    "nodeStatus": { "type": "integer" },
                                    "nodeId": { "type": "number", "minimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["elementId"]
                            },
                            "minItems": 2
                        }
                    },
                    "additionalProperties": false,
                    "required": ["referencePoint", "direction", "witnessPoints"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateAssociativeDimensionsOnSection",
                "version": "1.4.0",
                "description": "Creates associative linear dimensions on section elements using common wall, slab, beam, column and opening presets.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "sectionElementId": { "$ref": "#/ElementId" },
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "preset": {
                            "type": "string",
                            "enum": [
                                "WallCompositeFaces",
                                "WallSkinBorders",
                                "SlabCompositeFaces",
                                "SlabSkinBorders",
                                "BeamOrColumnRefLineEndPoints",
                                "BeamOrColumnBoundingBoxCorners",
                                "DoorWindowWallHoleCorners",
                                "DoorWindowModelHotspots"
                            ]
                        },
                        "direction": { "$ref": "#/Coordinate2D" },
                        "skinBorderIndices": {
                            "type": "array",
                            "items": { "type": "integer" },
                            "minItems": 1
                        },
                        "beginPlane": { "type": "boolean" },
                        "totalSizePlane": { "type": "boolean" },
                        "placeOnTop": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["sectionElementId", "referencePoint", "preset"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateWallThicknessDimensions",
                "version": "1.4.0",
                "description": "Creates associative wall thickness dimensions for the given walls.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "wallId": { "$ref": "#/ElementId" },
                        "referencePoint": { "$ref": "#/Coordinate2D" },
                        "direction": { "$ref": "#/Coordinate2D" }
                    },
                    "additionalProperties": false,
                    "required": ["wallId", "referencePoint", "direction"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "GetDimensionData",
                "version": "1.5.0",
                "description": "Gets witness point data (coordinates, measured values) from existing dimension chains.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The identifier of the dimension elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "$ref": "#/DimensionDataOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["dimensionsData"]
    }
            },{
                "name": "CreateZones",
                "version": "1.1.8",
                "description": "Creates Zone elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "zonesData": {
            "type": "array",
            "description": "Array of data to create Zones.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Zone.",
                "properties" : {
                    "floorIndex": {
                        "type": "number"
                    },
                    "name": {
                        "type": "string",
                        "description" : "Name of the zone."
                    },
                    "numberStr": {
                        "type": "string",
                        "description" : "Zone number."	
                    },
                    "categoryAttributeId": {
                        "$ref": "#/AttributeId",
                        "description" : "The identifier of the zone category attribute."	
                    },
                    "stampPosition": {
                        "$ref": "#/Coordinate2D",
                        "description" : "Position of the origin of the zone stamp."
                    },
                    "stampAngle": {
                        "type": "number",
                        "description" : "Optional zone stamp rotation angle in radians."
                    },
                    "fixedStampAngle": {
                        "type": "boolean",
                        "description" : "If true, the zone stamp angle remains fixed when the element is rotated."
                    },
                    "geometry": {
                        "$ref": "#/ZoneCreationGeometry"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "name",
                    "numberStr",
                    "geometry"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "zonesData"
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
                "name": "CreatePolylines",
                "version": "1.1.5",
                "description": "Creates Polyline elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "polylinesData": {
            "type": "array",
            "description": "Array of data to create Polylines.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Polyline.",
                "properties" : {
                    "floorInd": {
                        "type": "number",
                        "description" : "The identifier of the floor. Optional parameter, by default the current floor is used."	
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description" : "Layer attribute index to place the polyline on. Optional parameter, by default the current layer is used."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description" : "Pen index of the polyline contour. Optional parameter, by default the current pen is used."
                    },
                    "lineTypeIndex": {
                        "type": "integer",
                        "description" : "Line type attribute index of the polyline contour. Optional parameter, by default the current line type is used."
                    },
                    "penWeightMm": {
                        "type": "number",
                        "description" : "Optional pen weight override in mm."
                    },
                    "coordinates": { 
                        "type": "array",
                        "description": "The 2D coordinates of the polyline.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 2
                    },
                    "arcs": { 
                        "type": "array",
                        "description": "The arcs of the polyline.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
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
        "polylinesData"
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
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates"
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
            },{
                "name": "CreateLamps",
                "version": "1.5.0",
                "description": "Creates Lamp elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "lampsData": {
                "type": "array",
                "description": "Array of data to create Lamps.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new Lamp.",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description" : "The name of the lamp library part to use."
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "lampsData"
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
                "name": "CreateMeshes",
                "version": "1.1.9",
                "description": "Creates Mesh elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "meshesData": {
            "type": "array",
            "description": "Array of data to create Meshes.",
            "items": {
                "type": "object",
                "description" : "The parameters of the new Mesh.",
                "properties" : {
                    "floorIndex": {
                        "type": "integer"
                    },
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
                    "ridges": {
                        "type": "string",
                        "description": "How ridges between mesh facets are displayed in 3D: 'AllSharp' shows all ridges, 'AllSmooth' hides them, 'UserDefined' shows only ridges along user-defined level lines (the drawing-set look for contour-line topography).",
                        "enum": ["AllSharp", "AllSmooth", "UserDefined"]
                    },
                    "showLines": {
                        "type": "boolean",
                        "description": "Whether to show secondary mesh lines (level lines other than the user-defined ones) on plan."
                    },
                    "contourPen": {
                        "type": "integer",
                        "description": "Optional pen attribute index for the mesh's contour line."
                    },
                    "levelPen": {
                        "type": "integer",
                        "description": "Optional pen attribute index for the mesh's level lines."
                    },
                    "lineTypeIndex": {
                        "type": "integer",
                        "description": "Optional line type attribute index for the mesh's contour."
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
                    "holes" : {
                        "$ref": "#/Holes3D"
                    },
                    "sublines": {
                        "type": "array",
                        "description": "The leveling sublines inside the polygon of the mesh.",
                        "items": {
                            "type": "object",
                            "properties" : {
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
                    "polygonCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "meshesData"
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
                "name": "CreateLabels",
                "version": "1.2.5",
                "description": "Creates Label elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "labelsData": {
            "type": "array",
            "description": "Array of data to create Labels.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Label.",
                "properties": {
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description" : "The parent element if the label is an associative label."	
                    },
                    "text": { 
                        "type": "string",
                        "description": "The text content if the label is a text label."
                    },
                    "begCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The begin coordinate of leader line. Optional parameter, but either begCoordinate or parentElementId must be provided."
                    },
                    "midCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The mid coordinate of leader line. Optional parameter."
                    },
                    "endCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The end coordinate of leader line. Optional parameter."
                    },

                    "floorInd": {
                        "type": "number",
                        "description" : "The identifier of the floor. Optional parameter, by default the current floor or the floor of the parent element is used."	
                    }
                },
                "additionalProperties": false,
                "required": [
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "labelsData"
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
                "name": "CreateTexts",
                "version": "1.5.0",
                "description": "Creates standalone Text elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "textsData": {
            "type": "array",
            "description": "Array of data to create Texts.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Text element.",
                "properties": {
                    "coordinate": {
                        "$ref": "#/Coordinate3D",
                        "description": "The placement position of the text. The z value is used to determine the floor when floorIndex is omitted."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content. Newlines create multiple lines."
                    },
                    "height": {
                        "type": "number",
                        "description": "The character height in millimeters. Optional; defaults to the Text tool default."
                    },
                    "pen": {
                        "type": "integer",
                        "description": "Optional pen attribute index."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Optional rotation angle in radians."
                    },
                    "justification": {
                        "type": "string",
                        "description": "Optional text justification.",
                        "enum": ["Left", "Center", "Right", "Full"]
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinate",
                    "text"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "textsData"
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
                "name": "ModifyWalls",
                "version": "1.4.0",
                "description": "Modifies Wall elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "wallsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero makes the wall curved (begCoordinate/endCoordinate are the chord endpoints)." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "bottomOffset": { "type": "number" },
                        "offset": { "type": "number" },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite", "Profile"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["wallsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyBeams",
                "version": "1.4.0",
                "description": "Modifies Beam elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "beamsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "level": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": { "type": "number" },
                        "arcAngle": { "type": "number" },
                        "verticalCurveHeight": { "type": "number" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["beamsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifySlabs",
                "version": "1.4.0",
                "description": "Modifies Slab elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "slabsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "zCoordinate": { "type": "number" },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "polygonOutline": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": { "$ref": "#/Holes2D" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["slabsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyColumns",
                "version": "1.4.0",
                "description": "Modifies Column elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "columnsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "origin": { "$ref": "#/Coordinate2D" },
                        "zCoordinate": { "type": "number" },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "bottomOffset": { "type": "number" },
                        "axisRotationAngle": { "type": "number" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["columnsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyWindows",
                "version": "1.4.0",
                "description": "Modifies Window elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "windowsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["windowsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyDoors",
                "version": "1.4.0",
                "description": "Modifies Door elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "doorsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0 },
                        "height": { "type": "number", "exclusiveMinimum": 0.0 },
                        "sillHeight": { "type": "number" },
                        "centerOffset": { "type": "number", "minimum": 0.0 },
                        "reflected": { "type": "boolean" },
                        "refSide": { "type": "boolean" },
                        "oSide": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["doorsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyMorphs",
                "version": "1.4.0",
                "description": "Modifies Morph elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "morphsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "translation": { "$ref": "#/Coordinate3D" },
                        "rotationDegreesZ": { "type": "number" },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "body": {
                            "$ref": "#/MorphBody",
                            "description": "When given, discards the Morph's ENTIRE existing geometry and rebuilds it from this (mirrors CreateProfiles' replaceSkins) - not a partial edit."
                        },
                        "xAxis": { "$ref": "#/Coordinate3D", "description": "Replaces the rotation part of the placement transform outright. Give all three of xAxis/yAxis/zAxis together. If rotationDegreesZ is also given in the same call, it is applied first and this then overwrites its result." },
                        "yAxis": { "$ref": "#/Coordinate3D" },
                        "zAxis": { "$ref": "#/Coordinate3D" },
                        "surfaceId": { "$ref": "#/AttributeId" },
                        "castShadow": { "type": "boolean" },
                        "receiveShadow": { "type": "boolean" },
                        "isAutoOnStoryVisibility": { "type": "boolean" },
                        "showContour": { "$ref": "#/StoryVisibility" },
                        "showFill": { "$ref": "#/StoryVisibility" },
                        "linkToSettings": {
                            "type": "object",
                            "properties": {
                                "homeStoryDifference": { "type": "integer" },
                                "newCreationMode": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        },
                        "displayOption": {
                            "type": "string",
                            "enum": ["Standard", "StandardWithAbstract", "CutOnly", "OutLinesOnly", "AbstractAll", "CutAll"]
                        },
                        "viewDepthLimitation": {
                            "type": "string",
                            "enum": ["ToFloorPlanRange", "ToAbsoluteLimit", "EntireElement"]
                        },
                        "cutFillPen": { "type": "integer" },
                        "cutFillBackgroundPen": { "type": "integer" },
                        "cutLineType": { "$ref": "#/AttributeId" },
                        "cutLinePen": { "type": "integer" },
                        "uncutLineType": { "$ref": "#/AttributeId" },
                        "uncutLinePen": { "type": "integer" },
                        "overheadLineType": { "$ref": "#/AttributeId" },
                        "overheadLinePen": { "type": "integer" },
                        "useCoverFillType": { "type": "boolean" },
                        "outlineContourDisplay": { "type": "boolean" },
                        "coverFillType": { "$ref": "#/AttributeId" },
                        "coverFillPen": { "type": "integer" },
                        "coverFillBGPen": { "type": "integer" },
                        "use3DHatching": { "type": "boolean" },
                        "coverFillOrientation": {
                            "type": "object",
                            "properties": {
                                "type": { "type": "string", "enum": ["Global", "Rotated", "Distorted", "Centered"] },
                                "origo": { "$ref": "#/Coordinate2D" },
                                "matrix00": { "type": "number" },
                                "matrix10": { "type": "number" },
                                "matrix01": { "type": "number" },
                                "matrix11": { "type": "number" },
                                "innerRadius": { "type": "number" }
                            },
                            "additionalProperties": false
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "type": "string",
                            "enum": ["Invalid", "Planar", "Default", "Cylindric", "Spheric", "Box"]
                        },
                        "textureProjectionCoords": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate3D" },
                            "minItems": 4,
                            "maxItems": 4
                        },
                        "level": { "type": "number" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["morphsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyRoofs",
                "version": "1.4.0",
                "description": "Modifies multi-plane Roof elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "roofsWithDetails": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "level": { "type": "number" },
                        "thickness": { "type": "number", "exclusiveMinimum": 0.0 },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "type": "object",
                                "properties": {
                                    "levelHeight": { "type": "number" },
                                    "levelAngle": { "type": "number", "exclusiveMinimum": 0.0 }
                                },
                                "additionalProperties": false,
                                "required": ["levelHeight", "levelAngle"]
                            }
                        },
                        "structureType": {
                            "type": "string",
                            "enum": ["Basic", "Composite"]
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "polygonOutline": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": { "$ref": "#/Holes2D" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["roofsWithDetails"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "ModifyMeshes",
                "version": "1.5.4",
                "description": "Modifies the attributes of Mesh elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "meshesData": {
            "type": "array",
            "description": "Array of meshes to modify.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "meshData": {
                        "type": "object",
                        "description": "The fields to modify on the Mesh. Only provided fields are changed; omitted fields are left as-is.",
                        "properties": {
                            "floorIndex": {
                                "type": "integer"
                            },
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
                            "ridges": {
                                "type": "string",
                                "description": "How ridges between mesh facets are displayed in 3D.",
                                "enum": ["AllSharp", "AllSmooth", "UserDefined"]
                            },
                            "showLines": {
                                "type": "boolean",
                                "description": "Whether to show secondary mesh lines on plan."
                            },
                            "contourPen": {
                                "type": "integer",
                                "description": "Pen attribute index for the mesh contour line."
                            },
                            "levelPen": {
                                "type": "integer",
                                "description": "Pen attribute index for the mesh level lines."
                            },
                            "lineTypeIndex": {
                                "type": "integer",
                                "description": "Line type attribute index for the mesh contour."
                            },
                            "polygonCoordinates": {
                                "type": "array",
                                "description": "The 3D coordinates of the outline polygon of the mesh. Replaces the existing boundary entirely.",
                                "items": { "$ref": "#/Coordinate3D" },
                                "minItems": 3
                            },
                            "polygonArcs": {
                                "type": "array",
                                "description": "Polygon outline arcs of the mesh.",
                                "items": { "$ref": "#/PolyArc" }
                            },
                            "holes": {
                                "$ref": "#/Holes3D"
                            },
                            "sublines": {
                                "type": "array",
                                "description": "The leveling sublines inside the polygon of the mesh. Replaces existing sublines entirely.",
                                "items": {
                                    "type": "object",
                                    "properties": {
                                        "coordinates": {
                                            "type": "array",
                                            "description": "The 3D coordinates of the leveling subline.",
                                            "items": { "$ref": "#/Coordinate3D" }
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": ["coordinates"]
                                }
                            }
                        },
                        "additionalProperties": false
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "meshData"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "meshesData"
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
                "name": "GetElementPreviewImage",
                "version": "1.2.7",
                "description": "Returns the preview image of the given element.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId"
            },
            "imageType": {
                "type": "string",
                "description": "The type of the preview image. Default is 3D.",
                "enum": ["2D", "Section", "3D"]
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 128."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 128."
            }
        },
        "additionalProperties": false,
        "required": [
            "elementId"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "previewImage": {
                "type": "string",
                "description": "The base64 encoded preview image."
            }
        },
        "additionalProperties": false,
        "required": [
            "previewImage"
        ]
    }
            },{
                "name": "GetRoomImage",
                "version": "1.2.7",
                "description": "Returns the room image of the given zone.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "zoneId": {
                "$ref": "#/ElementId"
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 256."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 256."
            },
            "offset": {
                "type": "number",
                "description": "Offset of the clip polygon from the edge of the zone. Default is 0.001."
            },
            "scale": {
                "type": "number",
                "description": "Scale of the view (e.g. 0.005 for 1:200). Default is 0.005."
            },
            "backgroundColor": {
                "$ref": "#/ColorRGB",
                "description": "Background color of the generated image. Default is white (1.0, 1.0, 1.0)."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneId"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "roomImage": {
                "type": "string",
                "description": "The base64 encoded room image."
            }
        },
        "additionalProperties": false,
        "required": [
            "roomImage"
        ]
    }
            },{
                "name": "SetElementNotificationClient",
                "version": "1.2.8",
                "description": "Sets up a new notification client to receive element events.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "host": {
                "type": "string",
                "description": "The host address of the notification client. If not provided, localhost is used."
            },
            "port": {
                "type": "integer",
                "description": "The port number of the notification client."
            },
            "notifyOnNewElement": {
                "type": "boolean",
                "description": "Notify on creation of a new element. Optional parameter, by default true."
            },
            "notifyOnModificationOfAnElement": {
                "type": "boolean",
                "description": "Notify on modification/deletion of an element. Optional parameter, by default true."
            },
            "notifyOnReservationChanges": {
                "type": "boolean",
                "description": "Notify on reservation changes of an element. Optional parameter, by default true."
            }
        },
        "additionalProperties": false,
        "required": [
            "port"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "RemoveElementNotificationClient",
                "version": "1.2.8",
                "description": "Removes an element notification client.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "host": {
                "type": "string",
                "description": "The host address of the notification client. If not provided, localhost is used."
            },
            "port": {
                "type": "integer",
                "description": "The port number of the notification client."
            }
        },
        "additionalProperties": false,
        "required": [
            "port"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            }]
        },{
            "name": "Element grouping Commands",
            "commands": [{
                "name": "CreateGroups",
                "version": "1.4.0",
                "description": "Creates groups of the passed elements",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementGroups": {
                "type": "array",
                "description": "A list of element groups to create.",
                "items": {
                    "$ref": "#/ElementGroupParameters"
                }
            }
        },
        "additionalProperties": false,
        "required":[
            "elementGroups"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "groupGuids": {
                "type": "array",
                "description": "The results of the group creation operations.",
                "items": {
                    "$ref": "#/GroupIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "groupGuids"
        ]
    }
            }]
        },{
            "name": "Favorites Commands",
            "commands": [{
                "name": "GetFavoritesByType",
                "version": "1.2.2",
                "description": "Returns a list of the names of all favorites with the given element type",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementType": {
                "$ref": "#/ElementType"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementType"
        ]
    },
                "outputScheme": {
        "$ref": "#/FavoritesOrError"
    }
            },{
                "name": "GetFavoritePreviewImage",
                "version": "1.2.7",
                "description": "Returns the preview image of the given favorite.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favorite": {
                "type": "string",
                "description": "The name of the favorite."
            },
            "imageType": {
                "type": "string",
                "description": "The type of the preview image. Default is 3D.",
                "enum": ["2D", "Section", "3D"]
            },
            "format": {
                "type": "string",
                "description": "The image format. Default is png.",
                "enum": ["png", "jpg"]
            },
            "width": {
                "type": "integer",
                "description": "The width of the preview image in pixels. Default is 128."
            },
            "height": {
                "type": "integer",
                "description": "The height of the preview image in pixels. Default is 128."
            }
        },
        "additionalProperties": false,
        "required": [
            "favorite"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "previewImage": {
                "type": "string",
                "description": "The base64 encoded preview image."
            }
        },
        "additionalProperties": false,
        "required": [
            "previewImage"
        ]
    }
            },{
                "name": "ApplyFavoritesToElementDefaults",
                "version": "1.1.2",
                "description": "Apply the given favorites to element defaults.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favorites": {
              "$ref": "#/Favorites"
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
                        },
                        "folder": {
                            "type": "array",
                            "description": "Optional folder hierarchy in the Favorites palette to place the new favorite under. Empty/omitted = root.",
                            "items": { "type": "string" }
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
            },{
                "name": "ImportFavorites",
                "version": "1.5.0",
                "description": "Import Favorites from a .prefs file or folder into the current project.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "Absolute path on the AC host to a Favorites file (.prefs) or folder."
            },
            "targetFolder": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Folder hierarchy under which to import. Empty = root."
            },
            "importFolders": {
                "type": "boolean",
                "description": "If true and `path` is a folder, the folder structure is preserved."
            },
            "conflictPolicy": {
                "type": "string",
                "enum": ["Error", "Skip", "Overwrite", "Append"],
                "description": "How to resolve name conflicts. Default Overwrite."
            }
        },
        "required": ["path"],
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "firstConflictName": {
                "type": "string",
                "description": "Set when conflictPolicy=Error and a name collided; absent otherwise."
            }
        },
        "additionalProperties": false
    }
            },{
                "name": "ExportFavorites",
                "version": "1.5.0",
                "description": "Export the project's Favorites to a .prefs file or folder.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "description": "Absolute path on the AC host. If extension matches the Favorite binary format (.prefs), writes a single file; otherwise treats as folder."
            },
            "names": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Optional subset of Favorites to export. Default: export all."
            }
        },
        "required": ["path"],
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {},
        "additionalProperties": false
    }
            },{
                "name": "ApplyFavoritesToElements",
                "version": "1.5.4",
                "description": "Apply the given favorites to existing elements. Only settings-type parameters are changed - geometry (position, floor, and dimensions such as a Wall's height) is left untouched, so applying a Favorite never moves or resizes the target element. By default settings, classifications, categories and properties are all applied; each can be opted out of individually.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favoritesToApply": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "The identifier of the element and the name of the Favorite to apply to it.",
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
            },
            "applySettings": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's settings-type parameters (structure, materials, pens, etc. - never geometry). Default is true."
            },
            "applyClassifications": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's classifications. Default is true."
            },
            "applyCategories": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's element categories (e.g. IFC categories). Default is true."
            },
            "applyProperties": {
                "type": "boolean",
                "description": "Whether to apply the Favorite's property values. Default is true."
            }
        },
        "additionalProperties": false,
        "required": [
            "favoritesToApply"
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
                "name": "UpdateFavoritesFromElements",
                "version": "1.5.4",
                "description": "Update existing favorites from the given elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favoritesFromElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "The identifier of the element and the name of the existing Favorite to update from it.",
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
            },{
                "name": "RenameFavorites",
                "version": "1.5.4",
                "description": "Rename existing favorites.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "renames": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "oldName": { "type": "string" },
                        "newName": { "type": "string" }
                    },
                    "additionalProperties": false,
                    "required": ["oldName", "newName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["renames"]
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
                "name": "DeleteFavorites",
                "version": "1.5.4",
                "description": "Delete existing favorites.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "favorites": {
                "$ref": "#/Favorites"
            }
        },
        "additionalProperties": false,
        "required": ["favorites"]
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
                "name": "GetPropertyValuesOfAttributes",
                "version": "1.1.8",
                "description": "Returns the property values of the attributes for the given property.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "properties": {
                "$ref": "#/PropertyIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributeIds",
            "properties"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "propertyValuesForAttributes": {
                "$ref": "#/PropertyValuesOrErrorArray",
                "description": "List of property value lists. The order of the outer list is that of the given attributes. The order of the inner lists are that of the given properties."
            }
        },
        "additionalProperties": false,
        "required": [
            "propertyValuesForAttributes"
        ]
    }
            },{
                "name": "SetPropertyValuesOfAttributes",
                "version": "1.1.8",
                "description": "Sets the property values of attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributePropertyValues": {
                "$ref": "#/AttributePropertyValues"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributePropertyValues"
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
                    "$ref": "#/PropertyGroupArrayItem"
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
                    "$ref": "#/PropertyGroupIdArrayItem"
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
                    "$ref": "#/PropertyGroupIdArrayItem"
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
                    "$ref" : "#/PropertyDefinitionArrayItem"
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
                    "$ref": "#/PropertyIdArrayItem"
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
            },{
                "name": "UpdatePropertyDefinitions",
                "version": "1.5.4",
                "description": "Updates the expression(s) of existing expression-based Custom Property Definitions.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyDefinitions": {
                "type": "array",
                "description": "The list of expression-based property definitions to update.",
                "items": {
                    "type": "object",
                    "properties": {
                        "propertyId": {
                            "$ref": "#/PropertyId"
                        },
                        "expressions": {
                            "type": "array",
                            "description": "The new expression strings for the property.",
                            "items": {
                                "type": "string"
                            },
                            "minItems": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "propertyId",
                        "expressions"
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
            "name": "Classification Commands",
            "commands": [{
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
                "name": "CreateClassificationSystems",
                "version": "1.5.0",
                "description": "Creates Classification Systems including Classification Items based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "classificationSystemsWithItems": {
                "$ref": "#/ClassificationSystemsWithItems"
            }
        },
        "additionalProperties": false,
        "required": [
           "classificationSystemsWithItems"
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
                "name": "CreateClassificationItems",
                "version": "1.5.0",
                "description": "Creates Classification Items in the given Classification Systems based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "newClassificationItems": {
                "$ref": "#/NewClassificationItems"
            }
        },
        "additionalProperties": false,
        "required": [
           "newClassificationItems"
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
                "name": "DeleteClassificationSystems",
                "version": "1.5.2",
                "description": "Deletes the given Classification Systems.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "classificationSystemIds": {
                "$ref": "#/ClassificationSystemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationSystemIds"
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
                "name": "DeleteClassificationItems",
                "version": "1.5.2",
                "description": "Deletes the given Classification Items.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "classificationItemIds": {
                "$ref": "#/ClassificationItemIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "classificationItemIds"
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
        "$ref": "#/AttributeHeadersOrError"
    }
            },{
                "name": "DeleteAttributes",
                "version": "1.5.4",
                "description": "Deletes the given attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributesToDelete": {
                "type": "array",
                "description" : "Array of attributes to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "attributeType": {
                            "$ref": "#/AttributeType"
                        },
                        "attributeId": {
                            "$ref": "#/AttributeIdArrayItem"
                        }
                    },
                    "additionalProperties": false,
                    "required": ["attributeType", "attributeId"]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "attributesToDelete"
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
                "name": "CreateLayers",
                "version": "1.0.3",
                "description": "Creates or overwrites Layer attributes based on the given parameters.",
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
                        "attributeId": {
                            "description": "Indentifier of the existing Layer to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Layer to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Layer with the given name will be overwritten."
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
                        },
                        "intersectionGroupNr": {
                            "type": "integer",
                            "description": "Intersection group. Elements on layers having the same group will be intersected."
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
                "description": "Overwrite the Layer if exists with the same name, or if index is given with the same index. The default is false."
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
                "name": "CreateLayerCombinations",
                "version": "1.2.4",
                "description": "Creates or overwrites Layer Combination attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layerCombinationDataArray": {
                "type": "array",
                "description" : "Array of data to create new Layer Combinations.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Layer Combination.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Layer Combination to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Layer Combination to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Layer Combination with the given name will be overwritten."
                        },
                        "layers": {
                            "$ref": "#/LayersOfLayerCombination"
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "name",
                        "layers"
                    ]
                }
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Overwrite the Layer Combination if exists with the same guid/index/name. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "layerCombinationDataArray"
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
                "name": "CreateLines",
                "version": "1.5.4",
                "description": "Creates or overwrites Line attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "lineDataArray": {
                "type": "array",
                "description" : "Array of data to create new Lines.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Line.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Line to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Line to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Line with the given name will be overwritten."
                        },
                        "scaleWithPlan": {
                            "type": "boolean",
                            "description": "If true, the line type parameters are defined in meters at the given defineScale and scale on printout with the actual plan scale. If false (default), the parameters are fixed values in millimeters as the line will appear on the printout."
                        },
                        "defineScale": {
                            "type": "number",
                            "description": "The floor plan scale the line type is defined with. Only used if scaleWithPlan is true."
                        },
                        "lineType": {
                            "type": "string",
                            "description": "Solid, Dashed, or Symbol. Defaults to Solid."
                        },
                        "period": {
                            "type": "number",
                            "description": "The length of one period (Dashed and Symbol line types)."
                        },
                        "height": {
                            "type": "number",
                            "description": "The height of the symbol line (Symbol line type only)."
                        },
                        "dashItems": {
                            "type": "array",
                            "description": "Dash-gap pairs describing one period (Dashed line type only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "dash": {
                                        "type": "number",
                                        "description": "Length of the visible part of the item."
                                    },
                                    "gap": {
                                        "type": "number",
                                        "description": "Length of the invisible part of the item."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["dash", "gap"]
                            }
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Symbol items describing one period (Symbol line type only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "itemType": {
                                        "type": "string",
                                        "description": "Separator, CenterDot, CenterLine, Dot, RightAngle, Parallel, Line, Circle, or Arc."
                                    },
                                    "centerOffset": {
                                        "type": "number",
                                        "description": "Vertical distance from the origin. Used for Separator, CenterDot, and CenterLine item types."
                                    },
                                    "length": {
                                        "type": "number",
                                        "description": "Length of the item. Used for CenterLine, RightAngle, and Parallel item types."
                                    },
                                    "begPos": {
                                        "description": "Beginning position. Used for Dot, RightAngle, Parallel, Line, Circle, and Arc item types.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "endPos": {
                                        "description": "End position. Used for Line item type only.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "radius": {
                                        "type": "number",
                                        "description": "Radius. Used for Circle and Arc item types."
                                    },
                                    "beginAngle": {
                                        "type": "number",
                                        "description": "Beginning angle in radians, measured from the vertical axis. Used for Arc item type only."
                                    },
                                    "endAngle": {
                                        "type": "number",
                                        "description": "End angle in radians, measured from the vertical axis. Used for Arc item type only."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["itemType"]
                            }
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
                "description": "Overwrite the Line if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "lineDataArray"
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
                "name": "CreateFills",
                "version": "1.5.4",
                "description": "Creates or overwrites Fill attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "fillDataArray": {
                "type": "array",
                "description" : "Array of data to create new Fills.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Fill.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Fill to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Fill to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Fill with the given name will be overwritten."
                        },
                        "subType": {
                            "type": "string",
                            "description": "Vector, Solid, Empty, Symbol, LinearGradient, RadialGradient, or Image. Defaults to Vector. Only one Solid and one Empty fill may exist. Image fills use the texture field's name to reference the image library part."
                        },
                        "scaleWithPlan": {
                            "type": "boolean",
                            "description": "The fill is scale dependent."
                        },
                        "useForWalls": {
                            "type": "boolean",
                            "description": "This fill can be used for cut fills."
                        },
                        "useForDraft": {
                            "type": "boolean",
                            "description": "This fill can be used for drafting fills."
                        },
                        "useForCover": {
                            "type": "boolean",
                            "description": "This fill can be used for cover fills."
                        },
                        "horizontalSpacing": {
                            "type": "number",
                            "description": "The fill's spacing factor in the X direction (Vector fills)."
                        },
                        "verticalSpacing": {
                            "type": "number",
                            "description": "The fill's spacing factor in the Y direction (Vector fills)."
                        },
                        "angle": {
                            "type": "number",
                            "description": "The angle of the fill in radians (Vector, Symbol, and gradient fills)."
                        },
                        "bitPattern": {
                            "type": "string",
                            "description": "16 hex characters (8 bytes) describing the fill's bitmap pattern, one line of the pattern per byte, matching the Pattern field of the Attribute Manager XML export."
                        },
                        "gradientStart": {
                            "description": "Gradient start point (LinearGradient/RadialGradient fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "gradientEnd": {
                            "description": "Gradient end point (LinearGradient/RadialGradient fills only).",
                            "$ref": "#/Coordinate2D"
                        },
                        "percent": {
                            "type": "number",
                            "description": "Translucency percentage [0..1] (gradient and some Solid fills)."
                        },
                        "texture": {
                            "description": "Texture parameters (Image and gradient fills). Only name, rotationAngle, xSize, ySize, mirrorX, and mirrorY are used for Fills.",
                            "$ref": "#/Texture"
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Vectorial fill line items (Vector fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "frequency": {
                                        "type": "number",
                                        "description": "The distance between two instances of this item."
                                    },
                                    "direction": {
                                        "type": "number",
                                        "description": "The angle of the item, measured CCW from the horizontal axis, in radians."
                                    },
                                    "offsetLine": {
                                        "type": "number",
                                        "description": "The parallel offset of the item, measured from the (rotated) horizontal axis."
                                    },
                                    "offset": {
                                        "description": "The offset of the item, given by its coordinates.",
                                        "$ref": "#/Coordinate2D"
                                    },
                                    "lineLengths": {
                                        "type": "array",
                                        "description": "Dash-gap length pairs describing this line item. Must contain an even number of items.",
                                        "items": {
                                            "type": "number"
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["frequency", "direction", "offsetLine", "offset"]
                            }
                        },
                        "symbolLines": {
                            "type": "array",
                            "description": "Line items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "begin": { "$ref": "#/Coordinate2D" },
                                    "end": { "$ref": "#/Coordinate2D" }
                                },
                                "additionalProperties": false,
                                "required": ["begin", "end"]
                            }
                        },
                        "symbolArcs": {
                            "type": "array",
                            "description": "Arc items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "begin": { "$ref": "#/Coordinate2D" },
                                    "origin": { "$ref": "#/Coordinate2D" },
                                    "angle": {
                                        "type": "number",
                                        "description": "Arc angle in radians, measured CCW."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["begin", "origin", "angle"]
                            }
                        },
                        "symbolHotspots": {
                            "type": "array",
                            "description": "Hotspot coordinates of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/Coordinate2D"
                            }
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
                "description": "Overwrite the Fill if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "fillDataArray"
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
                "name": "CreateZoneCategories",
                "version": "1.5.4",
                "description": "Creates or overwrites Zone Category attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "zoneCategoryDataArray": {
                "type": "array",
                "description" : "Array of data to create new Zone Categories.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Zone Category.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Zone Category to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Zone Category to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Zone Category with the given name will be overwritten."
                        },
                        "categoryCode": {
                            "type": "string",
                            "description": "Code of the Zone Category."
                        },
                        "color": {
                            "$ref": "#/ColorRGB"
                        },
                        "stampName": {
                            "type": "string",
                            "description": "Document name of the zone stamp library part (GSM) to use, e.g. the value shown as StampDocumentName in an Attribute Manager XML export. Only used together with stampMainGuid/stampRevGuid; ignored otherwise."
                        },
                        "stampMainGuid": {
                            "type": "string",
                            "description": "Main GUID of the zone stamp library part to use, e.g. the value shown as MainGuid in an Attribute Manager XML export (can be copied from an existing Zone Category obtained another way). If omitted, the stamp is copied from the Zone Category being overwritten, or from the project's first Zone Category when creating a new one."
                        },
                        "stampRevGuid": {
                            "type": "string",
                            "description": "Revision GUID of the zone stamp library part to use, e.g. the value shown as RevGuid in an Attribute Manager XML export. Required together with stampMainGuid."
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
                "description": "Overwrite the Zone Category if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneCategoryDataArray"
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
                "name": "CreateMEPSystems",
                "version": "1.5.4",
                "description": "Creates or overwrites MEP System attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "mepSystemDataArray": {
                "type": "array",
                "description" : "Array of data to create new MEP Systems.",
                "items": {
                    "type": "object",
                    "description": "Data to create an MEP System.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing MEP System to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing MEP System to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing MEP System with the given name will be overwritten."
                        },
                        "domain": {
                            "type": "string",
                            "description": "Ventilation, Piping, or CableCarrier. Only elements belonging to the same domain can use this system."
                        },
                        "contourPen": {
                            "type": "integer",
                            "description": "The index of the contour pen [1..255]."
                        },
                        "fillPen": {
                            "type": "integer",
                            "description": "The index of the fill (foreground) pen [1..255]."
                        },
                        "fillBackgroundPen": {
                            "type": "integer",
                            "description": "The index of the background pen [0..255]. 0 means transparent background."
                        },
                        "centerLinePen": {
                            "type": "integer",
                            "description": "The index of the center line pen [1..255]."
                        },
                        "fillId": {
                            "description": "Identifier of the fill pattern attribute.",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "centerLineTypeId": {
                            "description": "Identifier of the center line type attribute.",
                            "$ref": "#/AttributeIdArrayItem"
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
                "description": "Overwrite the MEP System if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "mepSystemDataArray"
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
                "name": "CreatePenTables",
                "version": "1.5.4",
                "description": "Creates or overwrites Pen Table attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "penTableDataArray": {
                "type": "array",
                "description" : "Array of data to create new Pen Tables.",
                "items": {
                    "type": "object",
                    "description": "Data to create a Pen Table.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Pen Table to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Pen Table to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Pen Table with the given name will be overwritten."
                        },
                        "isActiveForModel": {
                            "type": "boolean",
                            "description": "Make this the active Pen Table for the model window. Defaults to false for a new Pen Table, or to the current value when overwriting an existing one."
                        },
                        "isActiveForLayout": {
                            "type": "boolean",
                            "description": "Make this the active Pen Table for layouts. Defaults to false for a new Pen Table, or to the current value when overwriting an existing one."
                        },
                        "sourceAttributeId": {
                            "description": "Identifier of the Pen Table whose 255 pens are used as the starting point, before the pens listed in the pens array are applied on top. Defaults to the Pen Table being overwritten itself (so unlisted pens keep their current color/width/description), or an arbitrary existing Pen Table in the project when creating a brand new one (or a plain black, 0.1 mm pen for all 255 if the project has no Pen Table at all yet).",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "pens": {
                            "type": "array",
                            "description": "The pens to set in the Pen Table, on top of the 255 pens copied from sourceAttributeId (or the current Pen Table, or an arbitrary existing one - see sourceAttributeId). Only list the pens you actually want to change.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "index": {
                                        "type": "integer",
                                        "description": "Index of the pen [1..255]."
                                    },
                                    "color": {
                                        "$ref": "#/ColorRGB"
                                    },
                                    "width": {
                                        "type": "number",
                                        "description": "Thickness of the pen defined in paper millimeters."
                                    },
                                    "description": {
                                        "type": "string",
                                        "description": "Textual description of the pen."
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["index"]
                            }
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
                "description": "Overwrite the Pen Table if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "penTableDataArray"
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
                "name": "CreateProfiles",
                "version": "1.5.4",
                "description": "Creates or overwrites Profile attributes as a copy of an existing Profile's geometry, based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "profileDataArray": {
                "type": "array",
                "description" : "Array of data to create new Profiles.",
                "items": {
                    "type": "object",
                    "description": "Data to create or modify a Profile. Its geometry (the cross-section shape) comes from sourceAttributeId (an existing Profile's geometry, copied), from newSkins (AC27+ only, caller-supplied polygon geometry), or both combined. When creating a brand-new Profile (overwriteExisting false, or true but no existing match), at least one of the two must be given. When overwriteExisting targets an existing Profile, both are optional - the existing Profile's own current geometry is preserved by default (e.g. to change only wallType, or to add newSkins on top of the unchanged existing shape). skinOverrides and newSkins' edgeOverrides target skins/edges by the identifiers/indices GetProfiles reports.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Profile to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Profile to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Profile with the given name will be overwritten."
                        },
                        "sourceAttributeId": {
                            "description": "Identifier of an existing Profile whose geometry (cross-section shape) will be copied as the starting point. Optional if newSkins is given: omit it to build the Profile's geometry entirely from newSkins instead of copying anything (AC27+ only).",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "wallType": {
                            "type": "boolean",
                            "description": "Profile available for walls. Defaults to the source Profile's value."
                        },
                        "beamType": {
                            "type": "boolean",
                            "description": "Profile available for beams. Defaults to the source Profile's value."
                        },
                        "coluType": {
                            "type": "boolean",
                            "description": "Profile available for columns. Defaults to the source Profile's value."
                        },
                        "handrailType": {
                            "type": "boolean",
                            "description": "Profile available for handrails. Defaults to the source Profile's value."
                        },
                        "otherGDLObjectType": {
                            "type": "boolean",
                            "description": "Profile available for other GDL based objects. Defaults to the source Profile's value."
                        },
                        "skinOverrides": {
                            "type": "array",
                            "description": "Modifications to apply to specific skins of the copied geometry, e.g. to change a skin's building material without rebuilding the profile's shape. Each skinId comes from a prior GetProfiles call's skins[].skinId on the source Profile (or, when overwriteExisting is true, on the Profile being overwritten).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "skinId": {
                                        "type": "string",
                                        "description": "Identifies which skin to modify, from GetProfiles' skins[].skinId."
                                    },
                                    "buildingMaterialId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "surfaceId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "fillId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "contourPen": {
                                        "type": "integer"
                                    },
                                    "contourLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "isCore": {
                                        "type": "boolean"
                                    },
                                    "isFinish": {
                                        "type": "boolean"
                                    },
                                    "visibleCutEndLines": {
                                        "type": "boolean"
                                    },
                                    "cutEndLinePen": {
                                        "type": "integer"
                                    },
                                    "cutEndLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "edgeOverrides": {
                                        "type": "array",
                                        "description": "Modifications to specific edges of this skin, targeted by their position (0-based) in GetProfiles' skins[].edges.",
                                        "items": {
                                            "type": "object",
                                            "properties": {
                                                "edgeIndex": {
                                                    "type": "integer"
                                                },
                                                "pen": {
                                                    "type": "integer"
                                                },
                                                "isVisibleLine": {
                                                    "type": "boolean"
                                                },
                                                "lineTypeId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                },
                                                "buildingMaterialId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["edgeIndex"]
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["skinId"]
                            }
                        },
                        "newSkins": {
                            "type": "array",
                            "description": "AC27+ only. Adds brand-new skins built from caller-supplied polygon geometry, instead of (or in addition to) whatever was copied from sourceAttributeId. Combine with sourceAttributeId to add skins to a copied Profile, or omit sourceAttributeId to build a Profile's geometry entirely from newSkins.",
                            "items": {
                                "type": "object",
                                "description": "One new skin (hatch). Its shape is one or more closed polygon contours: the first is the outer boundary, any further ones are holes cut out of it - the same polygon+holes convention as e.g. CreateSlabs' polygonCoordinates/polygonArcs/holes, just expressed as a list of contours instead of a separate holes array.",
                                "properties": {
                                    "contours": {
                                        "type": "array",
                                        "description": "Closed polygon contours forming this skin's cross-section, in the Profile's local coordinate system. Each contour is closed automatically - do not repeat its first vertex at the end.",
                                        "items": {
                                            "type": "object",
                                            "properties": {
                                                "polygonCoordinates": {
                                                    "type": "array",
                                                    "description": "The 2D coordinates of this contour.",
                                                    "items": {
                                                        "$ref": "#/Coordinate2D"
                                                    },
                                                    "minItems": 3
                                                },
                                                "polygonArcs": {
                                                    "type": "array",
                                                    "description": "Optional arcs along this contour's edges. begIndex/endIndex are 0-based positions within this contour's own polygonCoordinates.",
                                                    "items": {
                                                        "$ref": "#/PolyArc"
                                                    }
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["polygonCoordinates"]
                                        },
                                        "minItems": 1
                                    },
                                    "buildingMaterialId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "surfaceId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "fillId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "contourPen": {
                                        "type": "integer"
                                    },
                                    "contourLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "isCore": {
                                        "type": "boolean"
                                    },
                                    "isFinish": {
                                        "type": "boolean"
                                    },
                                    "visibleCutEndLines": {
                                        "type": "boolean"
                                    },
                                    "cutEndLinePen": {
                                        "type": "integer"
                                    },
                                    "cutEndLineTypeId": {
                                        "$ref": "#/AttributeIdArrayItem"
                                    },
                                    "edgeOverrides": {
                                        "type": "array",
                                        "description": "Per-edge pen/visibility/line type, targeted by 0-based edge index. Edge indices follow the same order as this skin's contours/polygonCoordinates: the outer contour's edges first (one edge per vertex, wrapping around), then each hole's, in the order the contours were given. Verify exact indices for a created skin via a follow-up GetProfiles call's skins[].edges before relying on them.",
                                        "items": {
                                            "type": "object",
                                            "properties": {
                                                "edgeIndex": {
                                                    "type": "integer"
                                                },
                                                "pen": {
                                                    "type": "integer"
                                                },
                                                "isVisibleLine": {
                                                    "type": "boolean"
                                                },
                                                "lineTypeId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                },
                                                "buildingMaterialId": {
                                                    "$ref": "#/AttributeIdArrayItem"
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required": ["edgeIndex"]
                                        }
                                    }
                                },
                                "additionalProperties": false,
                                "required": ["contours"]
                            }
                        },
                        "replaceSkins": {
                            "type": "boolean",
                            "description": "AC27+ only. When overwriteExisting targets an existing Profile, discard every one of its existing skins (and any sourceAttributeId geometry given alongside it) before applying newSkins, instead of adding newSkins on top of the preserved existing geometry. Use this to fully replace a Profile's cross-section with a caller-authored shape while keeping its guid and scalar fields (wallType etc.) - e.g. to sync a Profile's geometry from another project, where sourceAttributeId can't be used because it only resolves within the same file. Also discards any existing profileModifiers, since those are tied to the specific geometry being replaced. Ignored when creating a brand-new Profile (there is nothing to discard yet)."
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
                "description": "Overwrite the Profile if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "profileDataArray"
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
                "description": "Creates or overwrites Building Material attributes based on the given parameters.",
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
                        "attributeId": {
                            "description": "Indentifier of the existing Building Material to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Building Material to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Building Material with the given name will be overwritten."
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
                        },
                        "showUncutLines": {
                            "type": "boolean",
                            "description": "Show Contours in Model Views."
                        },
                        "collisionDetection": {
                            "type": "boolean",
                            "description": "Whether the Building Material participates in collision detection."
                        },
                        "cutFillOrientation": {
                            "type": "string",
                            "description": "ProjectOrigin, ElementOrigin, or FitToSkin. Orientation of the cut fill."
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
                "description": "Overwrite the Building Material if exists with the same name, or if index is given with the same index. The default is false."
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
                "description": "Creates or overwrites Composite attributes based on the given parameters.",
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
                        "attributeId": {
                            "description": "Indentifier of the existing Composite to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing Composite to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing Composite with the given name will be overwritten."
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
                "description": "Overwrite the Composite if exists with the same name, or if index is given with the same index. The default is false."
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
                "name": "CreateSurfaces",
                "version": "1.2.2",
                "description": "Creates or overwrites Surface attributes based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "surfaceDataArray": {
                "type": "array",
                "description" : "Array of data to create new surfaces.",
                "items": {
                    "type": "object",
                    "description": "Data to create a surface.",
                    "properties": {
                        "attributeId": {
                            "description": "Indentifier of the existing Surface to overwrite, ignored if overwriteExisting is false.",
                            "$ref": "#/AttributeId"
                        },
                        "index": {
                            "type": "string",
                            "description": "Index of the existing surface to overwrite, ignored if overwriteExisting is false."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name. If overwriteExisting is true, then the existing surface with the given name will be overwritten."
                        },
                        "materialType": {
                            "$ref": "#/SurfaceType"
                        },
                        "ambientReflection": {
                            "type": "number",
                            "description": "Ambient percentage [0..100]."
                        },
                        "diffuseReflection": {
                            "type": "number",
                            "description": "Diffuse percentage [0..100]."
                        },
                        "specularReflection": {
                            "type": "number",
                            "description": "Specular percentage [0..100]."
                        },
                        "transparency": {
                            "type": "number",
                            "description": "Transparency percentage [0..100]."
                        },
                        "shine": {
                            "type": "number",
                            "description": "The shininess factor multiplied by 100 [0..10000]."
                        },
                        "transparencyAttenuation": {
                            "type": "number",
                            "description": "Transparency attenuation multiplied by 100 [0..10000]."
                        },
                        "emissionAttenuation": {
                            "type": "number",
                            "description": "Emission attenuation multiplied by 100 [0..10000]."
                        },
                        "surfaceColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "specularColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "emissionColor": {
                            "$ref": "#/ColorRGB"
                        },
                        "fillId": {
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "texture": {
                            "$ref": "#/Texture"
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
                "description": "Overwrite the Surface if exists with the same name, or if index is given with the same index. The default is false."
            }
        },
        "additionalProperties": false,
        "required": [
            "surfaceDataArray"
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
                "$ref": "#/BuildingMaterialPhysicalPropertiesList"
            }
        },
        "additionalProperties": false,
        "required": [
            "properties"
        ]
    }
            },{
                "name": "GetLayerCombinations",
                "version": "1.2.4",
                "description": "Returns the details of layer combination attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributes": {
                "$ref": "#/AttributeIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "attributes"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "layerCombinations" : {
                "type": "array",
                "description" : "A list of layer combinations.",
                "items": {
                    "$ref": "#/LayerCombinationAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "layerCombinations"
        ]
    }
            },{
                "name": "GetLines",
                "version": "1.5.4",
                "description": "Returns the details of the given Line attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Line. If omitted, every field is returned. Requesting only the fields you need avoids fetching dashItems/lineItems, which can be large.",
                "items": {
                    "type": "string",
                    "enum": ["scaleWithPlan", "defineScale", "lineType", "period", "height", "dashItems", "lineItems"]
                }
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
            "lines": {
                "type": "array",
                "description": "A list of lines or errors.",
                "items": {
                    "$ref": "#/LineAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "lines"
        ]
    }
            },{
                "name": "GetFills",
                "version": "1.5.4",
                "description": "Returns the details of the given Fill attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Fill. If omitted, every field is returned. Requesting only the fields you need avoids fetching lineItems/symbolLines/symbolArcs/symbolHotspots, which can be large.",
                "items": {
                    "type": "string",
                    "enum": ["subType", "scaleWithPlan", "useForWalls", "useForDraft", "useForCover", "horizontalSpacing", "verticalSpacing", "angle", "bitPattern", "gradientStart", "gradientEnd", "percent", "texture", "lineItems", "symbolLines", "symbolArcs", "symbolHotspots"]
                }
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
            "fills": {
                "type": "array",
                "description": "A list of fills or errors.",
                "items": {
                    "$ref": "#/FillAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "fills"
        ]
    }
            },{
                "name": "GetZoneCategories",
                "version": "1.5.4",
                "description": "Returns the details of the given Zone Category attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Zone Category. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["categoryCode", "color", "stampName", "stampMainGuid", "stampRevGuid"]
                }
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
            "zoneCategories": {
                "type": "array",
                "description": "A list of zone categories or errors.",
                "items": {
                    "$ref": "#/ZoneCategoryAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "zoneCategories"
        ]
    }
            },{
                "name": "GetMEPSystems",
                "version": "1.5.4",
                "description": "Returns the details of the given MEP System attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each MEP System. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["domain", "contourPen", "fillPen", "fillBackgroundPen", "centerLinePen", "fillId", "centerLineTypeId"]
                }
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
            "mepSystems": {
                "type": "array",
                "description": "A list of MEP systems or errors.",
                "items": {
                    "$ref": "#/MEPSystemAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "mepSystems"
        ]
    }
            },{
                "name": "GetPenTables",
                "version": "1.5.4",
                "description": "Returns the details of the given Pen Table attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Pen Table. If omitted, every field is returned. Requesting only the fields you need avoids fetching the 255-element pens array.",
                "items": {
                    "type": "string",
                    "enum": ["isActiveForModel", "isActiveForLayout", "pens"]
                }
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
            "penTables": {
                "type": "array",
                "description": "A list of pen tables or errors.",
                "items": {
                    "$ref": "#/PenTableAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "penTables"
        ]
    }
            },{
                "name": "GetProfiles",
                "version": "1.5.4",
                "description": "Returns the details of the given Profile attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Profile. If omitted, every field is returned. Note the raw cross-section vector geometry itself is not exposed; width/height/minimumWidth/minimumHeight/widthStretchable/heightStretchable/hasCoreSkin/profileModifiers are derived measurements matching what the Profile Editor shows, computed from the profile's internal stretch/parameter data.",
                "items": {
                    "type": "string",
                    "enum": ["wallType", "beamType", "coluType", "handrailType", "otherGDLObjectType", "useWith", "width", "height", "minimumWidth", "minimumHeight", "widthStretchable", "heightStretchable", "hasCoreSkin", "profileModifiers", "skins", "skinOutlines"]
                }
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
            "profiles": {
                "type": "array",
                "description": "A list of profiles or errors.",
                "items": {
                    "$ref": "#/ProfileAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "profiles"
        ]
    }
            },{
                "name": "GetComposites",
                "version": "1.5.4",
                "description": "Returns the details of the given Composite attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Composite. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["useWith", "skins", "separators"]
                }
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
            "composites": {
                "type": "array",
                "description": "A list of composites or errors.",
                "items": {
                    "$ref": "#/CompositeAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "composites"
        ]
    }
            },{
                "name": "GetSurfaces",
                "version": "1.5.4",
                "description": "Returns the details of the given Surface attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Surface. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["materialType", "ambientReflection", "diffuseReflection", "specularReflection", "transparency", "shine", "transparencyAttenuation", "emissionAttenuation", "surfaceColor", "specularColor", "emissionColor", "fillId", "texture"]
                }
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
            "surfaces": {
                "type": "array",
                "description": "A list of surfaces or errors.",
                "items": {
                    "$ref": "#/SurfaceAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "surfaces"
        ]
    }
            },{
                "name": "GetLayers",
                "version": "1.5.4",
                "description": "Returns the details of the given Layer attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Layer. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["isHidden", "isLocked", "isWireframe", "intersectionGroupNr"]
                }
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
            "layers": {
                "type": "array",
                "description": "A list of layers or errors.",
                "items": {
                    "$ref": "#/LayerAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "layers"
        ]
    }
            },{
                "name": "GetBuildingMaterials",
                "version": "1.5.4",
                "description": "Returns the details of the given Building Material attributes.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "attributeIds": {
                "$ref": "#/AttributeIds"
            },
            "fields": {
                "type": "array",
                "description": "Names of the fields to return for each Building Material. If omitted, every field is returned.",
                "items": {
                    "type": "string",
                    "enum": ["id", "manufacturer", "description", "connPriority", "cutFillIndex", "cutFillPen", "cutFillBackgroundPen", "cutSurfaceIndex", "cutFillOrientation", "thermalConductivity", "density", "heatCapacity", "embodiedEnergy", "embodiedCarbon", "showUncutLines", "collisionDetection"]
                }
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
            "buildingMaterials": {
                "type": "array",
                "description": "A list of building materials or errors.",
                "items": {
                    "$ref": "#/BuildingMaterialAttributeOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "buildingMaterials"
        ]
    }
            }]
        },{
            "name": "IFC Commands",
            "commands": [{
                "name": "IFCFileOperation",
                "version": "1.2.6",
                "description": "Executes an IFC file operation.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "method": {
                "type": "string",
                "description": "The file operation method to use.",
                "enum": ["save", "merge", "open"]
            },
            "ifcFilePath": {
                "type": "string",
                "description": "The target IFC file to use."
            },
            "fileType": {
                "type": "string",
                "description": "The type of the IFC file. The default is 'ifc'.",
                "enum": ["ifc", "ifcxml", "ifczip", "ifcxmlzip"]
            }
        },
        "additionalProperties": false,
        "required": [
            "method",
            "ifcFilePath"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetElementsByIFCIds",
                "version": "1.5.1",
                "description": "Retrieves the elements by the given IFC identifiers.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "ifcIds": {
                "type": "array",
                "description": "A list of IFC identifiers to get the corresponding elements for.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "ifcIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elementsByIFCIds": {
                "$ref": "#/ElementsByIFCIds"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsByIFCIds"
        ]
    }
            },{
                "name": "GetIFCIdsOfElements",
                "version": "1.5.1",
                "description": "Retrieves the IFC identifiers of the given elements.",
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
            "elementIFCIds": {
                "$ref": "#/ElementIFCIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCIds"
        ]
    }
            },{
                "name": "GetIFCTypeOfElements",
                "version": "1.5.1",
                "description": "Retrieves the IFC types of the given elements.",
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
            "elementIFCTypes": {
                "$ref": "#/ElementIFCTypesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCTypes"
        ]
    }
            },{
                "name": "GetIFCPropertiesOfElements",
                "version": "1.5.1",
                "description": "Retrieves the IFC properties of the given elements.",
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
            "elementIFCProperties": {
                "$ref": "#/ElementIFCPropertiesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCProperties"
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
            },{
                "name": "AddFilesToEmbeddedLibrary",
                "version": "1.2.2",
                "description": "Adds the given files into the embedded library.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "files": {
                "$ref": "#/LibraryFileAdditions"
            }
        },
        "additionalProperties": false,
        "required": [
            "files"
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
                "name": "GetAvailableLibraryParts",
                "version": "1.5.0",
                "description": "Lists library parts currently available to the project. Filter by typeId (e.g. 'Door', 'Window', 'Object', 'Lamp').",
                "inputScheme": {
        "type": "object",
        "properties": {
            "filterByTypeId": {
                "$ref": "#/LibraryPartType",
                "description": "Optional. Filter by libpart type (matches the value returned by LibPartTypeIdToString)."
            }
        },
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "libraryParts": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "guid": { "type": "string" },
                        "index": { "type": "integer" },
                        "documentName": { "type": "string" },
                        "fileName": { "type": "string" },
                        "typeId": { "$ref": "#/LibraryPartType" }
                    }
                }
            },
            "skippedCount": {
                "type": "integer",
                "description": "Library parts that ACAPI_LibraryPart_Get failed to read. Non-zero means the inventory is partial."
            },
            "skippedSample": {
                "type": "array",
                "description": "First five failed indices with their ACAPI error code, for diagnostic.",
                "items": {
                    "type": "object",
                    "properties": {
                        "index": { "type": "integer" },
                        "code": { "type": "integer" }
                    }
                }
            }
        },
        "additionalProperties": false,
        "required": ["libraryParts", "skippedCount"]
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
                "description": "Full local or LAN path for publishing. Optional, by default the path set in the settings of the publisher set will be used.",
                "minLength": 1
            },
            "selectedNavigatorItemIds": {
                "$ref": "#/NavigatorItemIds",
                "description": "Optional publisher-tree navigator items to publish instead of the whole publisher set."
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
                "name": "CreateDetails",
                "version": "1.4.0",
                "description": "Creates independent Detail databases.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "detailsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string", "minLength": 1 },
                        "referenceId": { "type": "string", "minLength": 1 }
                    },
                    "additionalProperties": false,
                    "required": ["name", "referenceId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["detailsData"]
    },
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/Databases"}},"additionalProperties":false,"required":["databases"]}
            },{
                "name": "CreateWorksheets",
                "version": "1.4.0",
                "description": "Creates independent Worksheet databases.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "worksheetsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string", "minLength": 1 },
                        "referenceId": { "type": "string", "minLength": 1 }
                    },
                    "additionalProperties": false,
                    "required": ["name", "referenceId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["worksheetsData"]
    },
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/Databases"}},"additionalProperties":false,"required":["databases"]}
            },{
                "name": "CreateLayout",
                "version": "1.4.0",
                "description": "Creates Layouts and their backing master layouts.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layoutsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "masterLayoutName":      { "type": "string", "minLength": 1 },
                        "masterNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutName":            { "type": "string", "minLength": 1 },
                        "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutParameters": {
                            "type": "object",
                            "properties": {
                                "horizontalSize":           { "type": "number" },
                                "verticalSize":             { "type": "number" },
                                "leftMargin":               { "type": "number" },
                                "topMargin":                { "type": "number" },
                                "rightMargin":              { "type": "number" },
                                "bottomMargin":             { "type": "number" },
                                "customLayoutNumber":       { "type": "string" },
                                "customLayoutNumbering":    { "type": "boolean" },
                                "doNotIncludeInNumbering":  { "type": "boolean" },
                                "displayMasterLayoutBelow": { "type": "boolean" }
                            },
                            "additionalProperties": false
                        }
                    },
                    "additionalProperties": false,
                    "required": ["layoutName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutsData"]
    },
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/Databases"}},"additionalProperties":false,"required":["databases"]}
            },{
                "name": "CreateLayoutSubset",
                "version": "1.4.0",
                "description": "Creates Layout Book subsets.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "subsetsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name":                  { "type": "string", "minLength": 1 },
                        "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "ownPrefix":             { "type": "string" },
                        "customNumber":          { "type": "string" },
                        "numberingStyle":        { "type": "string", "enum": ["Undefined", "abc", "ABC", "1", "01", "001", "0001", "noID"] },
                        "startAt":               { "type": "integer" },
                        "continueNumbering":     { "type": "boolean" },
                        "useUpperPrefix":        { "type": "boolean" },
                        "includeToIDSequence":   { "type": "boolean" },
                        "customNumbering":       { "type": "boolean" },
                        "addOwnPrefix":          { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required": ["name"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["subsetsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "navigatorItems": {
                "type": "array",
                "items": {
                    "$ref": "#/NavigatorItemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItems"]
    }
            },{
                "name": "CreateDrawings",
                "version": "1.4.0",
                "description": "Creates Drawing elements on the specified or active layout from navigator items.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "drawingsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutDatabaseId": { "$ref": "#/DatabaseId" },
                        "name": { "type": "string", "minLength": 1 },
                        "position": { "$ref": "#/Coordinate2D" },
                        "scale": { "type": "number", "exclusiveMinimum": 0.0 },
                        "clipPolygon": { "type": "array", "items": { "$ref": "#/Coordinate2D" }, "minItems": 3 }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "name", "position"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsData"]
    },
                "outputScheme": {"type":"object","properties":{"elements":{"$ref":"#/Elements"}},"additionalProperties":false,"required":["elements"]}
            },{
                "name": "GetLayoutSettings",
                "version": "1.1.7",
                "description": "Gets settings of layouts, including Layout Info Panel custom data fields.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layoutDatabaseIds": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "databaseId":      { "$ref": "#/DatabaseId" },
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutDatabaseIds"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "layoutSettings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "layoutName":               { "type": "string" },
                        "horizontalSize":           { "type": "number" },
                        "verticalSize":             { "type": "number" },
                        "leftMargin":               { "type": "number" },
                        "topMargin":                { "type": "number" },
                        "rightMargin":              { "type": "number" },
                        "bottomMargin":             { "type": "number" },
                        "customLayoutNumber":       { "type": "string" },
                        "customLayoutNumbering":    { "type": "boolean" },
                        "doNotIncludeInNumbering":  { "type": "boolean" },
                        "displayMasterLayoutBelow": { "type": "boolean" },
                        "customData": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "customSchemeKey":   { "type": "string" },
                                    "customSchemeName":  { "type": "string" },
                                    "customSchemeValue": { "type": "string" }
                                },
                                "required": ["customSchemeKey", "customSchemeValue"],
                                "additionalProperties": false
                            }
                        }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutSettings"]
    }
            },{
                "name": "SetLayoutSettings",
                "version": "1.1.7",
                "description": "Sets settings of layouts, including Layout Info Panel custom data fields.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layoutsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "layoutDatabaseId":         { "$ref": "#/DatabaseId" },
                        "layoutNavigatorItemId":    { "$ref": "#/NavigatorItemId" },
                        "layoutName":               { "type": "string" },
                        "horizontalSize":           { "type": "number" },
                        "verticalSize":             { "type": "number" },
                        "leftMargin":               { "type": "number" },
                        "topMargin":                { "type": "number" },
                        "rightMargin":              { "type": "number" },
                        "bottomMargin":             { "type": "number" },
                        "customLayoutNumber":       { "type": "string" },
                        "customLayoutNumbering":    { "type": "boolean" },
                        "doNotIncludeInNumbering":  { "type": "boolean" },
                        "showMasterBelow":          { "type": "boolean" },
                        "customData": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "customSchemeKey":   { "type": "string" },
                                    "customSchemeName":  { "type": "string" },
                                    "customSchemeValue": { "type": "string" }
                                },
                                "required": ["customSchemeValue"],
                                "additionalProperties": false
                            }
                        }
                    },
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["layoutsData"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "GetLayoutCustomScheme",
                "version": "1.1.7",
                "description": "Gets the Layout Info Panel custom field definitions (name and key) from Book Settings.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "customScheme": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "customSchemeKey":  { "type": "string" },
                        "customSchemeName": { "type": "string" }
                    },
                    "required": ["customSchemeKey", "customSchemeName"],
                    "additionalProperties": false
                }
            }
        },
        "required": ["customScheme"],
        "additionalProperties": false
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
            "items": {
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
            "items": {
                "$ref": "#/ViewSettingsOrError"
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
            },{
                "name": "GetView2DTransformations",
                "version": "1.1.7",
                "description": "Get zoom and rotation of 2D views",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIds": {
                "$ref": "#/NavigatorItemIds"
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
        "transformations": {
            "type": "array",
            "items": {
                "$ref": "#/ViewTransformationsOrError"
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "transformations"
    ]
}
            },{
                "name": "SetViewRotation",
                "version": "1.1.7",
                "description": "Set the rotation angle of 2D views via their floor plan database.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIdsWithRotation": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "View rotation angle in radians."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "rotation"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIdsWithRotation"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    }
            },{
                "name": "CloneProjectMapItemToViewMap",
                "version": "1.1.7",
                "description": "Clones Project Map viewpoints into the View Map, optionally into a specified folder.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "viewsData": {
                "type": "array",
                "description": "Array of views to clone from the Project Map to the View Map.",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Navigator item ID of the Project Map viewpoint to clone."
                        },
                        "parentNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Navigator item ID of the View Map folder to place the clone in. Optional; defaults to the View Map root."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "navigatorItems": {
                "type": "array",
                "items": {
                    "$ref": "#/NavigatorItemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItems"]
    }
            },{
                "name": "CreateViewsInViewMap",
                "version": "1.1.7",
                "description": "Creates independent (non-clone) navigator views in the View Map by copying database and settings from source items.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "viewsData": {
                "type": "array",
                "description": "Array of views to create as independent (non-clone) items in the View Map.",
                "items": {
                    "type": "object",
                    "properties": {
                        "navigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "Source navigator item whose database and settings are copied."
                        },
                        "parentNavigatorItemId": {
                            "$ref": "#/NavigatorItemId",
                            "description": "View Map folder to place the new view in. Optional; defaults to View Map root."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name for the new view. Optional; defaults to the source item name."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["viewsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "navigatorItems": {
                "type": "array",
                "items": {
                    "$ref": "#/NavigatorItemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItems"]
    }
            },{
                "name": "CreateViewMapFolder",
                "version": "1.1.7",
                "description": "Creates a new folder in the View Map.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "folderName": {
                "type": "string",
                "description": "Name of the new folder to create in the View Map."
            },
            "parentNavigatorItemId": {
                "$ref": "#/NavigatorItemId",
                "description": "Navigator item ID of the parent View Map folder. Optional; defaults to the View Map root."
            }
        },
        "additionalProperties": false,
        "required": ["folderName"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemId": {
                "$ref": "#/NavigatorItemId"
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemId"]
    }
            },{
                "name": "Set3DCutPlanes",
                "version": "1.3.1",
                "description": "Sets the 3D cut planes.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "cutPlanes": {
            "type": "array",
            "minItems": 1,
            "items": {
                "type": "object",
                "description": "Defines a 3D cut plane using the plane equation: pa*x + pb*y + pc*z + pd = 0",
                "properties": {
                    "pa": {
                        "type": "number",
                        "description": "Coefficient of x in the plane equation. The x coordinate of the normal vector of the plane."
                    },
                    "pb": {
                        "type": "number",
                        "description": "Coefficient of y in the plane equation. The y coordinate of the normal vector of the plane."
                    },
                    "pc": {
                        "type": "number",
                        "description": "Coefficient of z in the plane equation. The z coordinate of the normal vector of the plane."
                    },
                    "pd": {
                        "type": "number",
                        "description": "Constant term in the plane equation. The distance of the plane from the origin along the normal vector."
                    }
                },
                "required": [
                    "pa",
                    "pb",
                    "pc",
                    "pd"
                ],
                "additionalProperties": false
            }
        }
    },
    "additionalProperties": false
},
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "FitInWindow",
                "version": "1.3.1",
                "description": "Zooms to the given elements or fits everything in the window.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "CreateSections",
                "version": "1.5.0",
                "description": "Creates Section elements on the floor plan.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "sectionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "startCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "depth": { "type": "number" },
                        "name": { "type": "string" },
                        "floorIndex": { "type": "integer" }
                    },
                    "additionalProperties": false,
                    "required": ["startCoordinate", "endCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["sectionsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "MoveNavigatorItem",
                "version": "1.1.7",
                "description": "Moves a navigator item to a new parent in the navigator tree.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIdToMove": { "$ref": "#/NavigatorItemId" },
            "parentNavigatorItemId": { "$ref": "#/NavigatorItemId" },
            "previousNavigatorItemId": { "$ref": "#/NavigatorItemId" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIdToMove", "parentNavigatorItemId"]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "RenameNavigatorItem",
                "version": "1.1.7",
                "description": "Renames a navigator item or changes its ID.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemId": { "$ref": "#/NavigatorItemId" },
            "newName":         { "type": "string" },
            "newId":           { "type": "string" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemId"]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "DeleteNavigatorItems",
                "version": "1.1.7",
                "description": "Deletes navigator items from the navigator tree.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemIds": { "$ref": "#/NavigatorItemIds" }
        },
        "additionalProperties": false,
        "required": ["navigatorItemIds"]
    },
                "outputScheme": {"type":"object","properties":{"executionResults":{"$ref":"#/ExecutionResults"}},"additionalProperties":false,"required":["executionResults"]}
            },{
                "name": "GetNavigatorItemTree",
                "version": "1.1.7",
                "description": "Returns the full navigator item tree for the specified map.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorMapId": {
                "type": "string",
                "enum": ["PublicViewMap", "ProjectMap", "LayoutBook", "PublisherSets"],
                "description": "The navigator map to retrieve."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorMapId"]
    },
                "outputScheme": null
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
        "additionalProperties": false,
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
            "name": "Revision Management Commands",
            "commands": [{
                "name": "GetRevisionIssues",
                "version": "1.1.9",
                "description": "Retrieves all issues.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "revisionIssues": {
                "type": "array",
                "items": {
                    "$ref": "#/RevisionIssue"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionIssues"
        ]
    }
            },{
                "name": "GetRevisionChanges",
                "version": "1.1.9",
                "description": "Retrieves all changes.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
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
            "revisionChanges"
        ]
    }
            },{
                "name": "GetDocumentRevisions",
                "version": "1.1.9",
                "description": "Retrieves all document revisions.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "documentRevisions": {
                "type": "array",
                "items": {
                    "$ref": "#/DocumentRevision"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "documentRevisions"
        ]
    }
            },{
                "name": "GetCurrentRevisionChangesOfLayouts",
                "version": "1.1.9",
                "description": "Retrieves all changes belong to the last revision of the given layouts.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "layoutDatabaseIds": {
                "$ref": "#/Databases"
            }
        },
        "additionalProperties": false,
        "required": [
            "layoutDatabaseIds"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "currentRevisionChangesOfLayouts": {
                "$ref": "#/RevisionChangesOfEntities"
            }
        },
        "additionalProperties": false,
        "required": [
            "currentRevisionChangesOfLayouts"
        ]
    }
            },{
                "name": "GetRevisionChangesOfElements",
                "version": "1.1.9",
                "description": "Retrieves the changes belong to the given elements.",
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
            "revisionChangesOfElements": {
                "$ref": "#/RevisionChangesOfEntities"
            }
        },
        "additionalProperties": false,
        "required": [
            "revisionChangesOfElements"
        ]
    }
            }]
        },{
            "name": "Design Options Commands",
            "commands": [{
                "name": "GetDesignOptions",
                "version": "1.2.9",
                "description": "Retrieves information about existing design options. Available from Archicad 29.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionDetails"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    }
            },{
                "name": "GetDesignOptionSets",
                "version": "1.2.9",
                "description": "Retrieves information about existing design option sets. Available from Archicad 29.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionSetId": {
                            "$ref": "#/GuidId",
                            "description": "The guid identifier of the design option set."
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the design option set."
                        },
                        "designOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of design options in the set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionSetId",
                        "name",
                        "designOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
        ]
    }
            },{
                "name": "GetDesignOptionCombinations",
                "version": "1.2.9",
                "description": "Retrieves information about existing design option combinations.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/GuidId",
                            "description": "The guid identifier of the design option combination."
                        },
                        "name": {
                            "type": "string",
                            "description": "The name of the design option combination."
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination. Available from Archicad 29."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    }
            },{
                "name": "GetElementsOfDesignOptions",
                "version": "1.5.1",
                "description": "Retrieves the elements associated with the given design options. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionId": {
                            "$ref": "#/DesignOptionId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elementsOfDesignOptions": {
                "type": "array",
                "items": {
                    "#ref": "#/ElementsOfDesignOptionOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsOfDesignOptions"
        ]
    }
            },{
                "name": "GetDesignOptionForElements",
                "version": "1.5.1",
                "description": "Retrieves the design option association for the specified elements. Available from Archicad 29.",
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
            "designOptionForElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the element."
                        },
                        "type": {
                            "type": "string",
                            "description": "The type of the associated design option.",
                            "enum": [
                                "NotExistingElement",
                                "MissingDesignOption",
                                "NotLinkedToAnyDesignOption",
                                "LinkedToDesignOption"
                            ]
                        },
                        "designOption": {
                            "$ref": "#/DesignOptionDetails"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "type"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionForElements"
        ]
    }
            },{
                "name": "CreateDesignOptionSets",
                "version": "1.5.1",
                "description": "Creates new design option sets with the given names. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionSets"
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
                "name": "CreateDesignOptions",
                "version": "1.5.1",
                "description": "Creates new design options with the given parameters. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the design option."
                        },
                        "id": {
                            "type": "string",
                            "description": "The string id of the design option."
                        },
                        "ownerSetName": {
                            "type": "string",
                            "description": "The name of the owner design option set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "id",
                        "ownerSetName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptions"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "designOptionIdsOrErrors": {
                "$ref": "#/DesignOptionIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionIdsOrErrors"
        ]
    }
            },{
                "name": "CreateDesignOptionCombinations",
                "version": "1.5.1",
                "description": "Creates new design option combinations with the given parameters. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the design option combination."
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "activeDesignOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinations"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "designOptionCombinationIdsOrErrors": {
                "$ref": "#/DesignOptionCombinationIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionCombinationIdsOrErrors"
        ]
    }
            },{
                "name": "SetActiveDesignOptionsInCombinations",
                "version": "1.5.1",
                "description": "Sets active design options in the given combinations. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "activeDesignOptionsInCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/DesignOptionCombinationId"
                        },
                        "activeDesignOptions": {
                            "type": "array",
                            "items": {
                                "$ref": "#/DesignOptionIdArrayItem"
                            },
                            "description": "The list of active design options in the combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "activeDesignOptions"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "activeDesignOptionsInCombinations"
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
                "name": "MoveElementsToDesignOptions",
                "version": "1.5.1",
                "description": "Moves the given elements into the given design options. Use NULLGuid for design option to remove the element from any design options and move it to the main model. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementDesignOptionPairs": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "designOptionId": {
                            "description": "The identifier of the design option to move the element into. Use NULLGuid to remove the element from any design option and move it to the main model.",
                            "$ref": "#/DesignOptionId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "designOptionId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementDesignOptionPairs"
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
                "name": "MoveDesignOptionsToAnotherSet",
                "version": "1.5.1",
                "description": "Moves the given design options to another sets. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionAndSetPairs": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionId": {
                            "description": "The identifier of the design option to move.",
                            "$ref": "#/DesignOptionId"
                        },
                        "setName": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "setName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "designOptionAndSetPairs"
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
            "name": "Solid Element Operation Commands",
            "commands": [{
                "name": "CreateSolidElementLinks",
                "version": "1.5.4",
                "description": "Creates solid element operation links between target and operator elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "solidLinks": {
                "type": "array",
                "description": "List of solid element operation links to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "targetId": {
                            "$ref": "#/ElementId",
                            "description": "The element to be cut or modified."
                        },
                        "operatorId": {
                            "$ref": "#/ElementId",
                            "description": "The element performing the operation."
                        },
                        "operation": {
                            "$ref": "#/SolidOperationType"
                        },
                        "linkFlags": {
                            "$ref": "#/SolidLinkFlags"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "targetId",
                        "operatorId",
                        "operation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "solidLinks"
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
                "name": "RemoveSolidElementLinks",
                "version": "1.5.4",
                "description": "Removes solid element operation links between target and operator elements.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "solidLinks": {
                "type": "array",
                "description": "List of solid element operation links to remove.",
                "items": {
                    "type": "object",
                    "properties": {
                        "targetId": {
                            "$ref": "#/ElementId"
                        },
                        "operatorId": {
                            "$ref": "#/ElementId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "targetId",
                        "operatorId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "solidLinks"
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
                "name": "GetSolidElementLinks",
                "version": "1.5.4",
                "description": "Returns solid element operation links for each queried element, grouped by role (target or operator).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Elements to query. Returns all solid links where each element is a target or an operator."
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
            "solidLinks": {
                "type": "array",
                "description": "For each input element, the solid links where it acts as target and where it acts as operator.",
                "items": {
                    "type": "object",
                    "properties": {
                        "solidLinksWithTheGivenTarget": {
                            "type": "array",
                            "description": "Links where the given element is the target (being cut or modified).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "operatorId": { "$ref": "#/ElementId" },
                                    "operation":  { "$ref": "#/SolidOperationType" },
                                    "linkFlags":  { "$ref": "#/SolidLinkFlags" }
                                },
                                "additionalProperties": false,
                                "required": [ "operatorId", "operation", "linkFlags" ]
                            }
                        },
                        "solidLinksWithTheGivenOperator": {
                            "type": "array",
                            "description": "Links where the given element is the operator (performing the cut).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "targetId":  { "$ref": "#/ElementId" },
                                    "operation": { "$ref": "#/SolidOperationType" },
                                    "linkFlags": { "$ref": "#/SolidLinkFlags" }
                                },
                                "additionalProperties": false,
                                "required": [ "targetId", "operation", "linkFlags" ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [ "solidLinksWithTheGivenTarget", "solidLinksWithTheGivenOperator" ]
                }
            }
        },
        "additionalProperties": false,
        "required": [ "solidLinks" ]
    }
            }]
        },{
            "name": "Script UI Commands",
            "commands": [{
                "name": "ShowScriptUI",
                "version": "1.5.4",
                "description": "Shows the given HTML content in a native Archicad palette (a tkinter alternative for Python scripts).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "htmlContent": {
                "type": "string",
                "description": "The full HTML document to display in the Script UI palette.",
                "minLength": 1
            },
            "width": {
                "type": "integer",
                "description": "Optional palette client width in pixels. Leave unset to keep the current size.",
                "minimum": 1
            },
            "height": {
                "type": "integer",
                "description": "Optional palette client height in pixels. Leave unset to keep the current size.",
                "minimum": 1
            },
            "title": {
                "type": "string",
                "description": "Optional palette window title. Leave unset to keep the current title.",
                "minLength": 1
            },
            "resizable": {
                "type": "boolean",
                "description": "Whether the user can drag-resize the palette. Defaults to true."
            },
            "zoomEnabled": {
                "type": "boolean",
                "description": "Whether Ctrl+scroll/pinch zooming is allowed in the page. Defaults to true."
            },
            "zoomLevel": {
                "type": "number",
                "description": "Optional initial zoom level (1.0 = 100%). Leave unset to keep the current zoom."
            },
            "scrollBarsVisible": {
                "type": "boolean",
                "description": "Whether the browser's own scrollbars are shown. Defaults to true."
            },
            "contextMenuEnabled": {
                "type": "boolean",
                "description": "Whether right-click shows the browser's context menu (e.g. Inspect Element). Defaults to true."
            },
            "navigationDisabled": {
                "type": "boolean",
                "description": "Prevents the page from navigating away (e.g. clicking a link to another site). Defaults to false."
            },
            "allowSelfSignedCertificates": {
                "type": "boolean",
                "description": "Only relevant if htmlContent links to a remote https:// resource with a self-signed certificate. Defaults to false."
            },
            "clearCookies": {
                "type": "boolean",
                "description": "Deletes all browser cookies before loading htmlContent. Defaults to false."
            },
            "autoHeight": {
                "type": "boolean",
                "description": "Automatically resizes the palette's height to fit the page content as it changes. Defaults to false."
            }
        },
        "additionalProperties": false,
        "required": [
            "htmlContent"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetScriptUIResult",
                "version": "1.5.4",
                "description": "Retrieves and clears the result last submitted from the Script UI palette's page (via window.ACAPI.SubmitResult), if any.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "hasResult": {
                "type": "boolean",
                "description": "True if the Script UI page has submitted a result since the last call."
            },
            "result": {
                "type": "string",
                "description": "The submitted content (as passed to window.ACAPI.SubmitResult). Only present when hasResult is true."
            }
        },
        "additionalProperties": false,
        "required": [
            "hasResult"
        ]
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