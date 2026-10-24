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
                "name": "GetPointFromUser",
                "version": "1.5.9",
                "description": "Asks the designer to click a point in the current window and returns it. Archicad waits for the click or for Escape, and every other JSON command queues behind this one until then; the call fails when the input is cancelled.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "prompt": {
                "type": "string",
                "description": "Shown in the control box while Archicad waits for the click. Single-byte text: the box takes a char field. Archicad's main thread waits for the click or for Escape, and every other JSON command queues behind this one until then."
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "position": {
                "$ref": "#/Coordinate3D",
                "description": "The clicked point in the project's coordinates."
            }
        },
        "additionalProperties": false,
        "required": [
            "position"
        ]
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
                "description": "Changes the current (active) window to the given window. With windowType 'FloorPlan' and a storyIndex it also activates that story on the floor plan.",
                "inputScheme": {
        "$ref": "#/NavigatorItemIdOrDatabaseIdAndWindowType"
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetUserGSID",
                "version": "1.5.6",
                "description": "Get the current registered User-GSID and OrganizationsID. Requires Archicad 27 or later.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "userId": {
                "type": "string",
                "description": "The stable GSID User ID of the logged-in user."
            },
            "organizationIds": {
                "type": "array",
                "items": {
                    "type": "string"
                },
                "description": "The list of organization IDs the user belongs to. Empty if not part of any organization or if the information cannot be retrieved."
            }
        },
        "additionalProperties": false,
        "required": [
            "userId"
        ]
    }
            },{
                "name": "ShowAlert",
                "version": "1.5.6",
                "description": "Display a dialog with up to three buttons.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "alertType": {
                "type": "string",
                "enum": ["information", "warning", "error"],
                "description": "The type of the alert dialog."
            },
            "title": {
                "type": "string",
                "description": "The title of the alert dialog."
            },
            "message": {
                "type": "string",
                "description": "The main message text."
            },
            "subMessage": {
                "type": "string",
                "description": "Optional smaller sub-message text below the main message."
            },
            "button1": {
                "type": "string",
                "description": "Label for the first (default) button."
            },
            "button2": {
                "type": "string",
                "description": "Label for the second button (e.g. Cancel)."
            },
            "button3": {
                "type": "string",
                "description": "Label for the optional third button."
            }
        },
        "additionalProperties": false,
        "required": ["alertType", "title", "message", "button1"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "clickedButton": {
                "type": "integer",
                "description": "Index of the button the user clicked: 1 = button1, 2 = button2, 3 = button3."
            }
        },
        "additionalProperties": false,
        "required": ["clickedButton"]
    }
            },{
                "name": "GetSpecialFolders",
                "version": "1.5.6",
                "description": "Retrieves the filesystem paths of the special folders of the running Archicad (preferences, cache, data, temporary, application, defaults, templates, help, embedded project library, etc.).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "folderTypes": {
                "type": "array",
                "description": "The types of the special folders to retrieve.",
                "items": {
                    "$ref": "#/SpecialFolderType"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "folderTypes"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "folderPaths": {
                "$ref": "#/SpecialFolderPathsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "folderPaths"
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
                "description": "The new value of the project info field. An empty string clears the field."
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
                    "$ref": "#/ProjectInfoFieldData"
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
                "description": "Retrieves information about the story structure of the currently loaded project: index, floor id, name, level, height and, from Archicad 28, the IFC GlobalId of each story.",
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
                "name": "GetAutoTextKeys",
                "version": "1.5.9",
                "description": "Retrieves the available autotext keys (name and embeddable key), optionally for a specific element. Embed a key in a Text or Label content by surrounding it with '<' and '>'.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementId": {
                "$ref": "#/ElementId",
                "description": "Optional. The element to retrieve context dependent autotext keys for (its own properties, plus the ones common to all element types, e.g. 'Element ID', 'Area'). When omitted, only the autotext keys common to all element types are returned."
            }
        },
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "autoTextKeys": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The autotext's name, as shown in the Insert Autotext dialog of Archicad."
                        },
                        "key": {
                            "type": "string",
                            "description": "The autotext's key. To embed it in the content of a Text or Label element, surround it with '<' and '>', e.g. '<PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548>'."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "key"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "autoTextKeys"
        ]
    }
            },{
                "name": "GetAutoTextName",
                "version": "1.5.9",
                "description": "Retrieves the display names of one or more autotext keys (as returned inside a '<...>' embedded key), with a direct guid lookup for property-based keys instead of enumerating every property definition.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "keys": {
                "type": "array",
                "description": "Autotext keys as returned by GetAutoTextKeys or GetProjectInfoFields (without the surrounding '<' and '>'), e.g. 'PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548' or 'PROJECTNAME'.",
                "items": {
                    "$ref": "#/AutoTextKeyValue"
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "keys"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "autoTextNames": {
                "$ref": "#/AutoTextNamesOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "autoTextNames"
        ]
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
                "name": "CreateHotlinkNodes",
                "version": "1.5.9",
                "description": "Creates hotlink module nodes from source files. A node that already points at the same file is returned instead of duplicated (Archicad 26 and later; 25 cannot see an unplaced node).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink module nodes to create. A node that already points at the same source file (compared case-insensitively) is returned as it is, with existing: true, and the name and story settings asked for are ignored. On Archicad 25 a node that has not been placed yet cannot be found, so a repeated request there creates a second node.",
                "items": {
                    "type": "object",
                    "properties": {
                        "sourceLocation": {
                            "type": "string",
                            "description": "Absolute path of the module source file (.mod or .pln)."
                        },
                        "name": {
                            "type": "string",
                            "description": "Optional display name of the node. Defaults to the file name. Ignored when a node for the same file already exists."
                        },
                        "storyRangeType": {
                            "type": "string",
                            "description": "Optional. Which stories of the source are placed: all of them, or the single reference story. Ignored when a node for the same file already exists.",
                            "enum": ["AllStories", "SingleStory"]
                        },
                        "refFloorIndex": {
                            "type": "integer",
                            "description": "Optional index of the reference story in the source file. Defaults to 0. Ignored when a node for the same file already exists."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "sourceLocation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "One item per requested node, in order: the node guid with its existing flag, or an error.",
                "items": {
                    "$ref": "#/HotlinkNodeCreatedOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
        ]
    }
            },{
                "name": "CreateHotlinkInstances",
                "version": "1.5.9",
                "description": "Places instances of hotlink module nodes at an origin, rotation and mirroring.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The hotlink instances to place.",
                "items": {
                    "$ref": "#/HotlinkInstanceCreation"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "ChangeHotlinkInstances",
                "version": "1.5.9",
                "description": "Moves, rotates or mirrors placed hotlink instances by changing their transformation. MoveElements and RotateElements do not work on hotlink instances.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "hotlinkInstances": {
                "type": "array",
                "description": "The placed hotlink instances to change. Every field but elementId is optional; a field that is omitted keeps its current value.",
                "items": {
                    "$ref": "#/HotlinkInstanceChange"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkInstances"
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
                "name": "UpdateHotlinks",
                "version": "1.7.1",
                "description": "Re-reads the source files of hotlink module nodes and refreshes their cached content in the project, as the Hotlink Manager's Update button does.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink nodes to update, from GetHotlinks or CreateHotlinkNodes. Each node's cached content is re-read from its source file, as the Hotlink Manager's Update button does.",
                "items": {
                    "type": "object",
                    "properties": {
                        "hotlinkNodeId": {
                            "$ref": "#/HotlinkNodeId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "hotlinkNodeId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
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
                "name": "ChangeHotlinkNodes",
                "version": "1.7.1",
                "description": "Repoints hotlink module nodes at other source files, as the Hotlink Manager's Relink button does. Follow it with UpdateHotlinks to re-read the content.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "hotlinkNodes": {
                "type": "array",
                "description": "The hotlink nodes to repoint at another source file, as the Hotlink Manager's Relink button does. The placed instances stay where they are. Follow it with UpdateHotlinks on the same nodes to re-read the content from the new file.",
                "items": {
                    "type": "object",
                    "properties": {
                        "hotlinkNodeId": {
                            "$ref": "#/HotlinkNodeId"
                        },
                        "sourceLocation": {
                            "type": "string",
                            "description": "Absolute path of the new module source file (.mod or .pln)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "hotlinkNodeId",
                        "sourceLocation"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "hotlinkNodes"
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
                "inputScheme": {
        "type": "object",
        "properties": {
            "projectPath": {
                "type": "string",
                "description": "Optional. Full path of a .pln file to save the project to, as File > Save As does: the open project then refers to that file. This saves an untitled project, or one opened from an older Archicad version, which cannot be saved in place. An existing file at the path is overwritten. When omitted, or when it is the project's own file, the project is saved in place, which keeps the refusal of a project opened read-only."
            }
        },
        "additionalProperties": false
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "SaveAsModuleFile",
                "version": "1.5.9",
                "description": "Saves the given elements, or the current selection, as a hotlink module (.mod) file.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "moduleFilePath": {
                "type": "string",
                "description": "Absolute path of the .mod file to write. An existing file is overwritten. The current window must be a floor plan, section, elevation or detail."
            },
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional. The elements that go into the module; omitted, the current selection does, as Save Selection as Module would. Pass GetAllElements for the whole project. Archicad 25 and 26 support the selection form only."
            }
        },
        "additionalProperties": false,
        "required": [
            "moduleFilePath"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "SaveProjectAsArchive",
                "version": "1.6.0",
                "description": "Saves the open project as an archive (.pla) file, with the library parts it uses inside.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "archiveFilePath": {
                "type": "string",
                "description": "Absolute path of the .pla archive to write. An existing file is overwritten. The archive becomes the open project, as Save As does."
            },
            "includeLibraryParts": {
                "type": "boolean",
                "description": "Optional, true by default. Whether the library parts the project uses go into the archive, which makes the archive usable as a linked library of another project."
            },
            "includeProperties": {
                "type": "boolean",
                "description": "Optional, true by default. Whether the properties go into the archive."
            },
            "includeTextures": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the linked textures go into the archive."
            },
            "includeBackgroundPicture": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the background picture goes into the archive."
            }
        },
        "additionalProperties": false,
        "required": [
            "archiveFilePath"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "SaveCurrentWindowAsImage",
                "version": "1.7.1",
                "description": "Saves what the current window shows as an image file (png, jpg, bmp, tiff or gif), as File > Save as does with a picture format.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "imageFilePath": {
                "type": "string",
                "description": "Absolute path of the image file to write. An existing file is overwritten."
            },
            "format": {
                "type": "string",
                "description": "Optional. The image file format; omitted, it comes from the extension of imageFilePath (.png, .jpg or .jpeg, .bmp, .tif or .tiff, .gif).",
                "enum": ["png", "jpg", "bmp", "tiff", "gif"]
            },
            "colorDepth": {
                "type": "string",
                "description": "Optional, trueColor by default. The color depth of the image, as in the Save as picture dialog.",
                "enum": ["blackAndWhite", "16Colors", "256Colors", "16Grays", "256Grays", "trueColor", "trueColorWithAlpha"]
            },
            "dithered": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the image is dithered when its color depth drops colors."
            },
            "entireDrawing": {
                "type": "boolean",
                "description": "Optional, false by default. Whether the image covers the entire drawing of a 2D window instead of its current zoom, as the Entire Drawing option of the Save as picture dialog does."
            }
        },
        "additionalProperties": false,
        "required": [
            "imageFilePath"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "imageFilePath": {
                "type": "string",
                "description": "The path of the written image file."
            },
            "zoom": {
                "$ref": "#/Zoom",
                "description": "The zoom of the window the image shows, as GetView2DTransformations reports it. Absent for the 3D window."
            },
            "rotation": {
                "type": "number",
                "description": "The orientation of the window in radian, as GetView2DTransformations reports it. Absent for the 3D window."
            }
        },
        "additionalProperties": false,
        "required": [
            "imageFilePath"
        ]
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
                "$ref": "#/ProjectLocation"
            },
            "surveyPoint": {
                "$ref": "#/SurveyPoint"
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
                "$ref": "#/ProjectLocationUpdate"
            },
            "surveyPoint": {
                "$ref": "#/SurveyPointUpdate"
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
                "version": "1.5.7",
                "description": "Gets the details of the given elements (geometry parameters etc). Use the optional fields parameter to return only the fields you need and skip the computation of the others (for example floorPlanPolygons).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "fields": {
                "type": "array",
                "description": "Optional filter for the fields to return for each element. When omitted, every field is returned. Fields not listed are not computed at all, so listing only what you need skips in particular the floorPlanPolygons extraction, which regenerates each element's 2D drawing primitives and can dominate the execution time of batch reads.",
                "items": {
                    "$ref": "#/ElementDetailsField"
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
            "detailsOfElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "Details of an element. When the optional fields filter is given in the input, only the requested fields are present; the required list below applies to unfiltered requests.",
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
                        "hotlinkId": {
                            "$ref": "#/ElementId",
                            "description": "The hotlink instance this element belongs to. Present only for elements that came in through a placed hotlink; such elements are read-only."
                        },
                        "details": {
                            "$ref": "#/TypeSpecificDetails"
                        },
                        "floorPlanPolygons": {
                            "type": "array",
                            "description": "Cut-fill polygons as drawn on the floor plan (wall joins resolved by ArchiCAD). Only collected for construction element types whose floor plan symbol comes from a real cut through the element - walls, columns, beams and their segments, slabs, roofs, shells, meshes, morphs, curtain walls and their segments/panels/frames, stairs and their risers/treads/structures. Absent for every other type, when the element has no cut fill, or when the floor plan database is not accessible.",
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
                "name": "Get3DGeometryOfElements",
                "version": "1.7.2",
                "description": "Gets the 3D geometry of elements as the solid bodies of their 3D model: a vertex list in world coordinates and the planar faces (with their holes) referencing it, each face with its surface attribute when that is a project attribute. The bodies of the subelements of a Stair, Railing or Curtain Wall are included. The output is aligned with the input array of elements. The 3D model of the elements is generated as needed, so many elements can take a while.",
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
            "geometries3D": {
                "type": "array",
                "description": "The 3D geometry of each element, aligned with the input: the solid bodies of its 3D model, or an error.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "bodies": {
                                    "type": "array",
                                    "description": "The solid bodies of the element's 3D model, including those of its subelements for a Stair, Railing or Curtain Wall. Empty when the element has no solid 3D model (for example a 2D element, or one filtered out of the 3D view).",
                                    "items": {
                                        "$ref": "#/Body3D"
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "bodies"
                            ]
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
            "geometries3D"
        ]
    }
            },{
                "name": "GetQuantitiesOfElements",
                "version": "1.7.2",
                "description": "Gets the quantities of the given elements calculated by Archicad (volumes, surfaces, lengths etc.), together with the volume and projected area per building material (one item per skin of composite and complex profile elements, one item for basic elements). Supported types: Wall, Column, Beam, Slab, Roof, Shell, Morph, Zone, ColumnSegment, BeamSegment. Use the optional minOpeningSize parameter to control the conditional values.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "minOpeningSize": {
                "type": "number",
                "minimum": 0,
                "description": "Optional minimum opening size in square meters. Openings smaller than this are not subtracted when the conditional values (for example volumeConditional of a Wall) are calculated. Archicad only uses it for walls. Default value is 0, so every opening is subtracted."
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
            "quantitiesOfElements": {
                "type": "array",
                "description": "The quantities of the elements, in the order of the given elements. Lengths are in meters, areas in square meters and volumes in cubic meters.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "type": {
                                    "$ref": "#/ElementType"
                                },
                                "quantities": {
                                    "$ref": "#/ElementQuantities"
                                },
                                "composites": {
                                    "type": "array",
                                    "description": "The quantities per building material, in the order Archicad reports them: one item for each skin of a composite or complex profile element, and a single item for a basic element. Empty for Zones and Morphs, for which Archicad reports no skins.",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "buildingMaterialId": {
                                                "$ref": "#/AttributeIdArrayItem"
                                            },
                                            "volume": {
                                                "type": "number",
                                                "description": "Net volume of the skin."
                                            },
                                            "projectedArea": {
                                                "type": "number",
                                                "description": "Projected area of the skin. Archicad reports 0 when the legacy intersection and surface calculation is turned on in the Project Preferences."
                                            },
                                            "isCore": {
                                                "type": "boolean",
                                                "description": "The skin is marked as core."
                                            },
                                            "isFinish": {
                                                "type": "boolean",
                                                "description": "The skin is marked as finish."
                                            },
                                            "subElementId": {
                                                "$ref": "#/ElementId",
                                                "description": "The sub-element (for example a column or beam segment) the skin belongs to. Present only when Archicad attaches the skin to a sub-element."
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "buildingMaterialId",
                                            "volume",
                                            "projectedArea",
                                            "isCore",
                                            "isFinish"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "type",
                                "quantities",
                                "composites"
                            ]
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
            "quantitiesOfElements"
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
                "description": "Gets the elements hosted by (connected to) the given owner elements, filtered to the given element type: for example the Windows or Doors of a Wall, the frames, panels, junctions and accessories of a Curtain Wall, the risers, treads and structures of a Stair, or the posts, rails and panels of a Railing.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The owner (host) elements whose connected elements are collected, e.g. Walls, Curtain Walls, Stairs or Railings."
            },
            "connectedElementType": {
                "$ref": "#/ElementType",
                "description": "The type of the connected elements to collect, e.g. Window or Door for a Wall owner, or a subelement type of a Curtain Wall, Stair or Railing owner."
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
                "name": "GetSectionElements",
                "version": "1.5.8",
                "description": "Gets the elements drawn in the given section, elevation or interior elevation databases, each with the owner element it was generated from. This is the only command that returns raw section element identifiers - every other listing command converts them to their owner - so it is the way to obtain the sectionElementId that CreateAssociativeDimensionsOnSection requires.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "databases": {
                "$ref": "#/Databases",
                "description": "The section, elevation or interior elevation databases to list the section elements of. If omitted, the current database is used."
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "sectionElements": {
                "type": "array",
                "description": "The elements drawn in the given databases, each with the owner element it was generated from.",
                "items": {
                    "type": "object",
                    "properties": {
                        "sectionElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the section element itself, accepted by CreateAssociativeDimensionsOnSection as sectionElementId."
                        },
                        "ownerElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of the owner element the section element was generated from - this is what every other listing command returns."
                        },
                        "ownerElementType": {
                            "$ref": "#/ElementType",
                            "description": "The type of the owner element. Only present when the owner's header is readable from the section database."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "sectionElementId",
                        "ownerElementId"
                    ]
                }
            },
            "executionResultForDatabases": {
                "$ref": "#/ExecutionResults"
            }
        },
        "additionalProperties": false,
        "required": [
            "sectionElements"
        ]
    }
            },{
                "name": "GetRelationsOfElements",
                "version": "1.5.7",
                "description": "Gets the type-specific relations of the given elements: endpoint and reference line connections of walls, beams and beam segments, boundary elements and boundary sections of zones, the zones on the two sides of windows, doors, skylights and curtain wall panels, and the zones connected to roofs and shells. Available from Archicad 26.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements"
            },
            "otherElementType": {
                "$ref": "#/ElementType",
                "description": "Optional filter: only relations to elements of this type are returned."
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
            "relations": {
                "type": "array",
                "description": "Type-specific relations of each element, aligned with the input.",
                "items": {
                    "$ref": "#/ElementRelationsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "relations"
        ]
    }
            },{
                "name": "GetZoneBoundaries",
                "version": "1.2.3",
                "description": "Gets the boundaries of the given Zones (connected elements, neighbour zones, etc.). Accepts either a single zoneElementId or a list of zones. Prefer the list: the expensive boundary recalculation runs once per call, so querying many Zones in one call is much faster than calling the command once per Zone.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "zoneElementId": {
                "$ref": "#/ElementId",
                "description": "The identifier of a single Zone. Prefer the zones array: querying many Zones in one call is much faster than one call per Zone."
            },
            "zones": {
                "$ref": "#/Elements",
                "description": "A list of Zones. Only one of zoneElementId and zones can be given."
            }
        },
        "additionalProperties": false,
        "required": [
        ]
    },
                "outputScheme": {
        "type": "object",
        "oneOf": [
            {
                "$ref": "#/ZoneBoundariesOrError"
            },
            {
                "$ref": "#/ZoneBoundariesOfZonesWrapper"
            }
        ]
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
                "description": "Moves elements with a given vector. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently.",
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
                "description": "Rotates elements around a reference point. An element that cannot be edited (its layer is hidden or locked, it is locked, or it is not available in teamwork) gets a failed execution result naming the reason instead of being skipped silently.",
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
                                    "$ref": "#/Coordinate2D",
                                    "description": "Starting point of the rotation arc."
                                },
                                "endPoint": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "End point of the rotation arc."
                                },
                                "origin": {
                                    "$ref": "#/Coordinate2D",
                                    "description": "Center of rotation."
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
                "description": "Deletes elements. Returns an execution result for each input element: an element that could not be deleted (for example because its layer is locked) gets a failed execution result instead of being skipped silently.",
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
                "version": "1.5.7",
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
                "version": "1.5.7",
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
                "name": "GetMarkerParametersOfElements",
                "version": "1.7.1",
                "description": "Gets the marker library part and the marker's GDL parameters (name, type, value) of the given Window and Door elements. The marker is the stamp drawn next to the opening on the floor plan; its parameters are separate from the opening's own GDL parameters, which GetGDLParametersOfElements returns.",
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
        "markerParametersOfElements": {
            "type": "array",
            "description": "The marker of each given element, in the order of the input, or an error for an element that is not a window or a door.",
            "items": {
                "type": "object",
                "oneOf": [
                    {
                        "type": "object",
                        "properties": {
                            "libPart": {
                                "$ref": "#/LibPartDetails",
                                "description": "The library part of the marker. Absent when it could not be read."
                            },
                            "parameters": {
                                "type": "array",
                                "description": "The GDL parameters of the marker.",
                                "items": {
                                    "type": "object",
                                    "properties": {
                                        "name": {
                                            "type": "string",
                                            "description": "The name of the parameter."
                                        },
                                        "displayName": {
                                            "type": "string",
                                            "description": "The display name of the parameter."
                                        },
                                        "index": {
                                            "type": "integer",
                                            "description": "The index of the parameter."
                                        },
                                        "type": {
                                            "type": "string",
                                            "description": "The type of the parameter."
                                        },
                                        "dimension1": {
                                            "type": "integer",
                                            "description": "The 1st dimension of array (in case of array value)."
                                        },
                                        "dimension2": {
                                            "type": "integer",
                                            "description": "The 2nd dimension of array (in case of array value)."
                                        },
                                        "value": {
                                            "description": "The value of the parameter."
                                        },
                                        "valueDescription": {
                                            "type": "string",
                                            "description": "The value description for numeric parameter."
                                        },
                                        "itemDescriptions": {
                                            "type": "array",
                                            "description": "Per-item text labels for an array-type parameter, one per array element in dim1xdim2 order. Only present when the library part defines them.",
                                            "items": {
                                                "type": "string"
                                            }
                                        },
                                        "flags": {
                                            "type": "array",
                                            "description": "The flags of the parameter.",
                                            "items": {
                                                "type": "string",
                                                "enum": [
                                                    "Hidden",
                                                    "HiddenFromScript",
                                                    "Disabled",
                                                    "Child",
                                                    "Unique",
                                                    "Fixed",
                                                    "BoldName",
                                                    "Open"
                                                ]
                                            }
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "name",
                                        "displayName",
                                        "index",
                                        "type",
                                        "value",
                                        "flags"
                                    ]
                                }
                            }
                        },
                        "additionalProperties": false,
                        "required": [
                            "parameters"
                        ]
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
        "markerParametersOfElements"
    ]
}
            },{
                "name": "SetMarkerParametersOfElements",
                "version": "1.7.1",
                "description": "Sets the given GDL parameters of the marker of the given Window and Door elements. The opening's own settings and the other marker parameters are left as they are.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "elementsWithMarkerParameters": {
            "type": "array",
            "description": "The elements with the marker parameters to change.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "markerParameters": {
                        "type": "array",
                        "description": "The marker parameters to change, each given by name or by index. The other marker parameters and the opening's own settings are left as they are.",
                        "items": {
                            "oneOf": [
                                {
                                    "type": "object",
                                    "properties": {
                                        "name": {
                                            "type": "string",
                                            "description": "The name of the marker parameter."
                                        },
                                        "value": {
                                            "description": "The new value of the marker parameter: a number for the numeric types (Integer, Length, Angle, RealNumber, PenColor, LineType, Material, FillPattern, BuildingMaterial, Profile, RGBColor, Intensity), a string for String and Title, true/false for Boolean and 'On'/'Off' for LightSwitch. Array parameters are not supported."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "name",
                                        "value"
                                    ]
                                },
                                {
                                    "type": "object",
                                    "properties": {
                                        "index": {
                                            "type": "integer",
                                            "description": "The index of the marker parameter, as GetMarkerParametersOfElements reports it."
                                        },
                                        "value": {
                                            "description": "The new value of the marker parameter, see the value given with a name."
                                        }
                                    },
                                    "additionalProperties": false,
                                    "required": [
                                        "index",
                                        "value"
                                    ]
                                }
                            ]
                        }
                    }
                },
                "additionalProperties": false,
                "required": [
                    "elementId",
                    "markerParameters"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "elementsWithMarkerParameters"
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
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
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
                        "slantAngle": {
                            "type": "number",
                            "description": "Optional slant angle in radians. A non-zero value also makes the column slanted, unless isSlanted is given explicitly."
                        },
                        "isSlanted": {
                            "type": "boolean",
                            "description": "Optional explicit slanted state. By default it is derived from slantAngle."
                        },
                        "slantDirectionAngle": {
                            "type": "number",
                            "description": "Optional rotation angle of the slanted column on the horizontal plane in radians."
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
                            "$ref": "#/ColumnCoreAnchor"
                        },
                        "circleBased": {
                            "type": "boolean",
                            "description": "True for a round column cross section, false for rectangular. Ignored if profileId is also given. Applied to all segments."
                        },
                        "isWidthAndHeightLinked": {
                            "type": "boolean",
                            "description": "When true (the default), Archicad keeps width and depth equal and setting one changes the other - set to false to give width/depth independent values. Applied to all segments."
                        },
                        "buildingMaterialId": {
                            "$ref": "#/AttributeId",
                            "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments."
                        },
                        "profileId": {
                            "$ref": "#/AttributeId",
                            "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments."
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
                "$ref": "#/ElementIdsOrErrors"
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
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a Wall favorite to base the new element on. Its settings (structure, reference line location, top link, offsets, ...) are applied first, then the explicitly given fields override them. With a favorite only begCoordinate and endCoordinate are needed."
                        },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Story index (as returned by GetStories). When provided, zCoordinate is interpreted as bottomOffset relative to the floor. Takes priority over zCoordinate for floor assignment." },
                        "zCoordinate": { "type": "number", "description": "Absolute Z when floorIndex is absent; bottomOffset relative to the floor when floorIndex is provided." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Optional explicit height. Sets relativeTopStory to 0, so it replaces the top link of the favorite or the tool default. When omitted, the height and top link of the favorite (or of the Wall tool defaults) are kept." },
                        "thickness": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Optional thickness. When omitted, the favorite's or the Wall tool default's thickness is kept. Only takes effect on a Basic (homogeneous) wall - a Composite or Profile wall takes its thickness from the composite or profile attribute. Without structureType the new wall keeps the structure of the favorite (or the wall tool default), which may be a composite; send structureType 'Basic' for the thickness to apply."
                        },
                        "offset": { "type": "number", "description": "Optional reference line offset. When omitted, the favorite's or the Wall tool default's offset is kept." },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero creates a curved wall (begCoordinate/endCoordinate are the chord endpoints)." },
                        "referenceLineLocation": {
                            "$ref": "#/WallReferenceLineLocation",
                            "description": "Optional. When omitted, the favorite's or the Wall tool default's reference line location is kept. The Core* values only have an effect on a Composite or Profile wall."
                        },
                        "structureType": {
                            "$ref": "#/WallStructureType",
                            "description": "Optional. When neither this nor one of the attribute ids below is given, the favorite's or the Wall tool default's structure is kept."
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" },
                        "contourPen": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen of the wall's own contour/reference outline (API_WallType::contPen) - distinct from cutFillPen, which is the cut fill pattern's pen."
                        },
                        "contourPen3D": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen of the wall's contour in 3D views (API_WallType::contPen3D)."
                        },
                        "contourLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type of the wall's own contour (API_WallType::contLtype) - pairs with contourPen, which only sets the pen, not the line type."
                        },
                        "belowViewLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type used when \"OutLines Only\" is selected from the \"Floor Plan Display\" popup (API_WallType::belowViewLineType)."
                        },
                        "aboveViewLinePen": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen used when \"Overhead All\" is selected from the \"Floor Plan Display\" popup."
                        },
                        "aboveViewLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type used when \"Overhead All\" is selected from the \"Floor Plan Display\" popup - pairs with aboveViewLinePen."
                        },
                        "displayOption": {
                            "$ref": "#/DisplayOption"
                        },
                        "viewDepthLimitation": {
                            "$ref": "#/ViewDepthLimitation",
                            "description": "Only has an effect on a multi-story wall (relativeTopStory/topOffset making it span more than one story)."
                        },
                        "useCompositePriority": {
                            "type": "boolean",
                            "description": "When true, the Composite's own priority is used at junctions instead of connectionPriority."
                        },
                        "linkToSettings": {
                            "$ref": "#/LinkToSettings",
                            "description": "When newCreationMode is true, homeStoryDifference overrides the home story selected by floorIndex or derived from zCoordinate."
                        },
                        "inheritEndSurface": {
                            "type": "boolean",
                            "description": "The end surface of the wall is inherited from the adjoining wall."
                        },
                        "alignTexture": {
                            "type": "boolean",
                            "description": "Align texture mapping to the wall's edges."
                        },
                        "polyCanChange": {
                            "type": "boolean",
                            "description": "The polygonal wall's corners can be changed in case of L and T connections. Only meaningful for a Polygonal wall (geometryType)."
                        },
                        "materialsChained": {
                            "type": "boolean",
                            "description": "Whether the surface materials are chained."
                        },
                        "logHeight": {
                            "type": "number",
                            "description": "Height of the log for log walls. 0 means a normal (non-log) wall."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate"]
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
                "$ref": "#/ElementIdsOrErrors"
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
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "floorIndex": { "type": "integer", "description": "Optional floor index. If omitted, derived from zCoordinate." },
                        "zCoordinate": { "type": "number" },
                        "offset": { "type": "number" },
                        "slantAngle": {
                            "type": "number",
                            "description": "Slant angle in radians. A non-zero value also switches the beam to slanted, unless isSlanted is given explicitly."
                        },
                        "isSlanted": {
                            "type": "boolean",
                            "description": "Optional explicit slanted state. By default it is derived from slantAngle."
                        },
                        "profileAngle": {
                            "type": "number",
                            "description": "Rotation angle of the profile around the beam's center line, in radians."
                        },
                        "arcAngle": { "type": "number", "description": "Optional arc angle in radians. A non-zero value makes a horizontally curved beam." },
                        "verticalCurveHeight": { "type": "number", "description": "Optional rise of a vertically curved beam. A non-zero value makes a vertically curved beam; it wins over arcAngle." },
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
                            "$ref": "#/BeamAnchorPoint"
                        },
                        "circleBased": {
                            "type": "boolean",
                            "description": "True for a round beam cross section, false for rectangular. Ignored if profileId is also given. Applied to all segments."
                        },
                        "isWidthAndHeightLinked": {
                            "type": "boolean",
                            "description": "When true (the default), Archicad keeps width and height equal and setting one changes the other - set to false to give width/height independent values. Applied to all segments."
                        },
                        "buildingMaterialId": {
                            "$ref": "#/AttributeId",
                            "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments."
                        },
                        "profileId": {
                            "$ref": "#/AttributeId",
                            "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments."
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
                "$ref": "#/ElementIdsOrErrors"
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
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
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
                        },
                        "finishVisible": {
                            "type": "boolean",
                            "description": "Optional. If false, the tread/riser finishes are hidden and only the stair structure (e.g. a monolith) is modeled."
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
                "$ref": "#/ElementIdsOrErrors"
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "level": {
                        "type": "number",
                        "description" : "The Z coordinate value of the reference line of the slab."	
                    },
                    "thickness": {
                        "type": "number",
                        "description": "Optional slab thickness. Only takes effect on a Basic (homogeneous) slab - a Composite slab takes its thickness from the composite attribute. Without structureType, buildingMaterialId or compositeId the new slab keeps the structure of the slab tool default (or the favorite), which may be a composite; send structureType 'Basic' for the thickness to apply.",
                        "exclusiveMinimum": 0.0
                    },
                    "structureType": {
                        "$ref": "#/SlabStructureType",
                        "description": "Optional structure of the slab. If omitted, derived from buildingMaterialId or compositeId when one is given, otherwise the slab tool default (or the favorite) is kept."
                    },
                    "buildingMaterialId": {
                        "$ref": "#/AttributeId",
                        "description": "Building material of a Basic slab. Cannot be combined with compositeId."
                    },
                    "compositeId": {
                        "$ref": "#/AttributeId",
                        "description": "Composite attribute of a Composite slab. Cannot be combined with buildingMaterialId."
                    },
                    "referencePlaneLocation": {
                        "$ref": "#/SlabReferencePlaneLocation",
                        "description": "For a basic (homogeneous) slab only 'Top' or 'Bottom' are valid."
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
                "$ref": "#/ElementIdsOrErrors"
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
                        "libraryPartName": {
                            "type": "string",
                            "description": "Optional. Localized library part name (docu_UName) to place instead of the tool default/favorite, e.g. a shutter part that has no favorite. It has to be a window library part. Applied to the tool defaults before cloning."
                        },
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
                "$ref": "#/ElementIdsOrErrors"
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
                        "libraryPartName": {
                            "type": "string",
                            "description": "Optional. Localized library part name (docu_UName) to place instead of the tool default/favorite, e.g. a shutter part that has no favorite. It has to be a door library part. Applied to the tool defaults before cloning."
                        },
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
                "$ref": "#/ElementIdsOrErrors"
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
                "$ref": "#/ElementIdsOrErrors"
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
                            "description": "Builds arbitrary geometry instead of a simple box. Mutually exclusive with `size` - give exactly one of the two."
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "xAxis": { "$ref": "#/Coordinate3D" },
                        "yAxis": { "$ref": "#/Coordinate3D" },
                        "zAxis": { "$ref": "#/Coordinate3D" },
                        "surfaceId": { "$ref": "#/AttributeId" },
                        "topExpansion": { "type": "number", "minimum": 0.0, "description": "Only used with `size` (box) morphs. Expands the top rectangle outwards on all four sides by this amount, creating a frustum (e.g. sloped excavation shape). 0 creates a cuboid." },
                        "castShadow": { "type": "boolean" },
                        "receiveShadow": { "type": "boolean" },
                        "isAutoOnStoryVisibility": { "type": "boolean" },
                        "showContour": { "$ref": "#/StoryVisibility" },
                        "showFill": { "$ref": "#/StoryVisibility" },
                        "linkToSettings": {
                            "$ref": "#/LinkToSettings"
                        },
                        "displayOption": {
                            "$ref": "#/DisplayOption"
                        },
                        "viewDepthLimitation": {
                            "$ref": "#/ViewDepthLimitation"
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
                            "$ref": "#/CoverFillOrientation"
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "$ref": "#/TextureProjectionType"
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateRoofs",
                "version": "1.4.0",
                "description": "Creates Roof elements based on footprint, level and roof profile data. Creates a multi-plane roof by default; pass 'pivotLine' (and optionally 'angle') to create a single-plane roof instead.",
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
                        "thickness": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Only takes effect on a Basic (homogeneous) roof - a Composite roof takes its thickness from the composite attribute. Without structureType the new roof keeps the structure of the roof tool default (or the favorite), which may be a composite; send structureType 'Basic' for the thickness to apply."
                        },
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
                        "pivotLine": {
                            "$ref": "#/RoofPivotLine",
                            "description": "If given, creates a single-plane roof instead of a multi-plane roof, tilted along this pivot line."
                        },
                        "angle": {
                            "type": "number",
                            "description": "Slope angle of the single-plane roof in radians. Only valid together with 'pivotLine'.",
                            "exclusiveMinimum": 0.0
                        },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "$ref": "#/RoofLevel"
                            }
                        },
                        "structureType": {
                            "$ref": "#/RoofStructureType"
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateCurtainWalls",
                "version": "1.7.2",
                "description": "Creates Curtain Wall elements along a reference polyline with a column (primary) and row (secondary) pattern. The frame and panel classes come from the favorite or from the Curtain Wall tool defaults; the pattern cells can assign them to the grid.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "curtainWallsData": {
            "type": "array",
            "description": "Array of data to create Curtain Walls.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Curtain Wall. Everything not given here - the frame and panel classes, the surfaces, the boundary and corner frames, the pattern - comes from the favorite or from the Curtain Wall tool defaults of the open project.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the reference line of the curtain wall on the floor plan. Each pair of consecutive coordinates becomes one straight segment, so two coordinates give a single-segment curtain wall.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 2
                    },
                    "zCoordinate": {
                        "type": "number",
                        "description": "Optional Z coordinate (absolute elevation) of the bottom of the curtain wall. When omitted, the base level of the favorite (or of the Curtain Wall tool defaults) relative to the floor is kept."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. When omitted, derived from zCoordinate if that is given, otherwise the current story."
                    },
                    "height": {
                        "type": "number",
                        "description": "Optional height of the curtain wall. When omitted, the height of the favorite (or of the Curtain Wall tool defaults) is kept.",
                        "exclusiveMinimum": 0.0
                    },
                    "primaryPattern": {
                        "$ref": "#/CurtainWallPattern",
                        "description": "Optional column pattern: the sizes of the columns along the segment (the primary grid). When omitted, the pattern of the favorite (or of the tool defaults) is kept."
                    },
                    "secondaryPattern": {
                        "$ref": "#/CurtainWallPattern",
                        "description": "Optional row pattern: the sizes of the rows from the bottom up (the secondary grid). When omitted, the pattern of the favorite (or of the tool defaults) is kept."
                    },
                    "cells": {
                        "type": "array",
                        "description": "Optional assignment of panel and frame classes to the cells of the pattern unit. The unit repeats along and up the segment, so an assignment applies to every repetition of that cell. A cell not listed here keeps the assignment of the favorite (or of the tool defaults) when the pattern is unchanged, otherwise the assignment of the first default cell.",
                        "items": {
                            "$ref": "#/CurtainWallPatternCell"
                        }
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "curtainWallsData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
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
                                "$ref": "#/AssociativeWitnessPoint"
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateAssociativeDimensionsOnSection",
                "version": "1.4.0",
                "description": "Creates associative linear dimensions on section elements using common wall, slab, beam, column and opening presets. The preset points of multiple section elements can be merged into one continuous dimension chain via sectionElementIds.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "dimensionsData": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "sectionElementId": {
                            "$ref": "#/ElementId",
                            "description": "The identifier of a single section element. Only one of sectionElementId and sectionElementIds can be given."
                        },
                        "sectionElementIds": {
                            "type": "array",
                            "items": { "$ref": "#/ElementId" },
                            "minItems": 1,
                            "description": "A list of section elements whose preset points are merged into one continuous dimension chain. Only one of sectionElementId and sectionElementIds can be given."
                        },
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
                    "required": ["referencePoint", "preset"]
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
                "$ref": "#/ElementIdsOrErrors"
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
                "$ref": "#/ElementIdsOrErrors"
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorIndex": {
                        "type": "number",
                        "description" : "Optional story index of the new zone. An automatic zone (geometry.referencePosition) is found around its reference point on this story, which is made active in the floor plan while the zone is created; the previously active story is restored afterwards."
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
                "$ref": "#/ElementIdsOrErrors"
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateLineElements",
                "version": "1.5.6",
                "description": "Creates Line elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "linesData": {
            "type": "array",
            "description": "Array of data to create Lines.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Line.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the line on. Optional parameter, by default the current layer is used."
                    },
                    "begCoordinate": {
                        "$ref": "#/Coordinate2D"
                    },
                    "endCoordinate": {
                        "$ref": "#/Coordinate2D"
                    },
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute. By default the current line type is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "begCoordinate",
                    "endCoordinate"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "linesData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateArcs",
                "version": "1.5.6",
                "description": "Creates Arc elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "arcsData": {
            "type": "array",
            "description": "Array of data to create Arcs.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Arc.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the arc on. Optional parameter, by default the current layer is used."
                    },
                    "origin": {
                        "$ref": "#/Coordinate2D"
                    },
                    "radius": {
                        "type": "number"
                    },
                    "begAngle": {
                        "type": "number",
                        "description": "Beginning angle of the arc in radians."
                    },
                    "endAngle": {
                        "type": "number",
                        "description": "End angle of the arc in radians."
                    },
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute. By default the current line type is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius",
                    "begAngle",
                    "endAngle"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "arcsData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateCircles",
                "version": "1.5.6",
                "description": "Creates Circle elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "circlesData": {
            "type": "array",
            "description": "Array of data to create Circles.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Circle.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the circle on. Optional parameter, by default the current layer is used."
                    },
                    "origin": {
                        "$ref": "#/Coordinate2D"
                    },
                    "radius": {
                        "type": "number"
                    },
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute. By default the current line type is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "circlesData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateHotspots",
                "version": "1.5.6",
                "description": "Creates Hotspot elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "hotspotsData": {
            "type": "array",
            "description": "Array of data to create Hotspots.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Hotspot.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the hotspot on. Optional parameter, by default the current layer is used."
                    },
                    "position": {
                        "$ref": "#/Coordinate2D"
                    },
                    "height": {
                        "type": "number"
                    },
                    "penIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "hotspotsData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateHatches",
                "version": "1.5.6",
                "description": "Creates Hatch elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "hatchesData": {
            "type": "array",
            "description": "Array of data to create Hatches.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Hatch.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the hatch on. Optional parameter, by default the current layer is used."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the hatch outline (single contour, no holes). Do not repeat the first point at the end.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "arcs": {
                        "type": "array",
                        "description": "The arcs of the hatch outline.",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "contourPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the contour. By default the current pen is used."
                    },
                    "fillPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the fill. By default the current pen is used."
                    },
                    "fillBackgroundPenIndex": {
                        "type": "integer"
                    },
                    "foregroundColor": {
                        "$ref": "#/ColorRGB",
                        "description": "Optional RGB color (components between 0.0 and 1.0) of the fill foreground, used instead of fillPenIndex. Takes precedence over fillPenIndex when both are given. The same format as the colors returned by GetZoneCategories and GetPenTables."
                    },
                    "backgroundColor": {
                        "$ref": "#/ColorRGB",
                        "description": "Optional RGB color (components between 0.0 and 1.0) of the fill background, used instead of fillBackgroundPenIndex. Takes precedence over fillBackgroundPenIndex when both are given."
                    },
                    "fillId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional fill attribute. By default the current fill is used."
                    },
                    "buildingMaterialId": {
                        "$ref": "#/AttributeId"
                    },
                    "roomSpecial": {
                        "type": "integer",
                        "description": "Special area percent in a room (negative means OFF)."
                    },
                    "showArea": {
                        "type": "boolean"
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "hatchesData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateSplines",
                "version": "1.5.6",
                "description": "Creates Spline elements based on the given parameters.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "splinesData": {
            "type": "array",
            "description": "Array of data to create Splines. Only auto-smoothed curves are supported (bezier handle positions are calculated automatically by Archicad from the point positions).",
            "items": {
                "type": "object",
                "description": "The parameters of the new Spline.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the spline on. Optional parameter, by default the current layer is used."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the spline points. Do not repeat the first point at the end even for a closed spline.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 3
                    },
                    "closed": {
                        "type": "boolean",
                        "description": "Is this a closed curve? Optional, defaults to false."
                    },
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "linePenIndex": {
                        "type": "integer",
                        "description": "Optional pen index. By default the current pen is used."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute. By default the current line type is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "splinesData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateObjects",
                "version": "1.5.7",
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
                        
        "angle": { "type": "number" },
        "pen": { "type": "integer" },
        "lineTypeId": { "$ref": "#/AttributeId" },
        "surfaceId": { "$ref": "#/AttributeId", "description": "Material/Surface override (API_ObjectType.mat)." },
        "sectionFillId": { "$ref": "#/AttributeId" },
        "sectionFillPen": { "type": "integer" },
        "sectionFillBackgroundPen": { "type": "integer" },
        "sectionContourPen": { "type": "integer" },
        "useObjectPens": { "type": "boolean" },
        "useObjectLineTypes": { "type": "boolean" },
        "useObjectMaterials": { "type": "boolean" },
        "useObjectSectionAttributes": { "type": "boolean" },
        "reflected": { "type": "boolean" },
        "useFixSize": { "type": "boolean" },
        "fixPoint": { "type": "integer", "description": "0-based index of the hotspot to keep fixed (raw API_ObjectType.fixPoint value, not 1-based)." },
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType/level." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "$ref": "#/StoryVisibility"
        },
        "linkToSettings": {
            "$ref": "#/LinkToSettings"
        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateLamps",
                "version": "1.5.7",
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
                        
        "angle": { "type": "number" },
        "pen": { "type": "integer" },
        "lineTypeId": { "$ref": "#/AttributeId" },
        "surfaceId": { "$ref": "#/AttributeId", "description": "Material/Surface override (API_ObjectType.mat)." },
        "sectionFillId": { "$ref": "#/AttributeId" },
        "sectionFillPen": { "type": "integer" },
        "sectionFillBackgroundPen": { "type": "integer" },
        "sectionContourPen": { "type": "integer" },
        "useObjectPens": { "type": "boolean" },
        "useObjectLineTypes": { "type": "boolean" },
        "useObjectMaterials": { "type": "boolean" },
        "useObjectSectionAttributes": { "type": "boolean" },
        "reflected": { "type": "boolean" },
        "useFixSize": { "type": "boolean" },
        "fixPoint": { "type": "integer", "description": "0-based index of the hotspot to keep fixed (raw API_ObjectType.fixPoint value, not 1-based)." },
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType/level." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "$ref": "#/StoryVisibility"
        },
        "linkToSettings": {
            "$ref": "#/LinkToSettings"
        },
        "lightColor": { "$ref": "#/ColorRGB", "description": "Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default light color). lightIsOn (the on/off state, as opposed to the color) does not have this problem." },
        "lightIsOn": { "type": "boolean" },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "GetGridElementDefaults",
                "version": "1.7.2",
                "description": "Returns the Grid Tool defaults: library part, size and GDL parameters of a new grid element. Needs Archicad 26 or later.",
                "inputScheme": {
        "type": "object",
        "properties": {},
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {
        "elementId": { "$ref": "#/ElementId", "description": "The grid element. The zero GUID for the tool defaults." },
        "variation": { "type": "string", "description": "Four-character variation of the object: 'GREL' for a grid element." },
        "floorIndex": { "type": "integer", "description": "Home story index." },
        "layerIndex": { "type": "integer", "description": "Layer attribute index." },
        "pos": { "$ref": "#/Coordinate2D", "description": "Insertion point: the start of the grid line." },
        "angle": { "type": "number", "description": "Direction of the grid line in radians." },
        "length": { "type": "number", "description": "Length of the grid line: the GDL parameter AC_Length of the Grid Tool's grid marker, or the object's A size for a library part without it." },
        "xRatio": { "type": "number", "description": "The object's xRatio as stored: its A size with useXYFixSize on, otherwise the A size divided by the library part's own A." },
        "yRatio": { "type": "number", "description": "The object's yRatio as stored, like xRatio for the B size." },
        "useXYFixSize": { "type": "boolean", "description": "Whether xRatio and yRatio hold the sizes themselves." },
        "level": { "type": "number", "description": "Elevation of the object from its home story." },
        "libInd": { "type": "integer", "description": "Library part index." },
        "libraryPartName": { "type": "string", "description": "Localized name of the library part." },
        "parameters": {
            "type": "array",
            "description": "GDL parameters, separators left out. Only when requested.",
            "items": {
                "type": "object",
                "properties": {
                    "name": { "type": "string" },
                    "displayName": { "type": "string" },
                    "type": { "type": "string", "description": "Integer, Length, Angle, RealNum, LightSwitch, ColorRGB, Intensity, LineType, Material, FillPattern, PenColor, String, Boolean, Title, BuildingMaterial, Profile, Dictionary or Unknown." },
                    "value": { "description": "Number for numeric and boolean types, string for String and Title. Not given for arrays." },
                    "dimension1": { "type": "integer", "description": "First dimension of an array parameter." },
                    "dimension2": { "type": "integer", "description": "Second dimension of an array parameter." }
                },
                "required": ["name", "displayName", "type"]
            }
        }
        },
        "required": ["variation", "floorIndex", "layerIndex", "pos", "angle", "length", "xRatio", "yRatio", "useXYFixSize", "level", "libInd", "libraryPartName", "parameters"]
    }
            },{
                "name": "GetGridElements",
                "version": "1.7.2",
                "description": "Lists the grid elements (placed with the Grid Tool or created by CreateGridElements) with their position, angle, size, library part and optionally GDL parameters. Needs Archicad 26 or later.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "includeParameters": {
                "type": "boolean",
                "description": "Also return the GDL parameters of each grid element. Default false."
            }
        },
        "additionalProperties": false
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "gridElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
        "elementId": { "$ref": "#/ElementId", "description": "The grid element. The zero GUID for the tool defaults." },
        "variation": { "type": "string", "description": "Four-character variation of the object: 'GREL' for a grid element." },
        "floorIndex": { "type": "integer", "description": "Home story index." },
        "layerIndex": { "type": "integer", "description": "Layer attribute index." },
        "pos": { "$ref": "#/Coordinate2D", "description": "Insertion point: the start of the grid line." },
        "angle": { "type": "number", "description": "Direction of the grid line in radians." },
        "length": { "type": "number", "description": "Length of the grid line: the GDL parameter AC_Length of the Grid Tool's grid marker, or the object's A size for a library part without it." },
        "xRatio": { "type": "number", "description": "The object's xRatio as stored: its A size with useXYFixSize on, otherwise the A size divided by the library part's own A." },
        "yRatio": { "type": "number", "description": "The object's yRatio as stored, like xRatio for the B size." },
        "useXYFixSize": { "type": "boolean", "description": "Whether xRatio and yRatio hold the sizes themselves." },
        "level": { "type": "number", "description": "Elevation of the object from its home story." },
        "libInd": { "type": "integer", "description": "Library part index." },
        "libraryPartName": { "type": "string", "description": "Localized name of the library part." },
        "parameters": {
            "type": "array",
            "description": "GDL parameters, separators left out. Only when requested.",
            "items": {
                "type": "object",
                "properties": {
                    "name": { "type": "string" },
                    "displayName": { "type": "string" },
                    "type": { "type": "string", "description": "Integer, Length, Angle, RealNum, LightSwitch, ColorRGB, Intensity, LineType, Material, FillPattern, PenColor, String, Boolean, Title, BuildingMaterial, Profile, Dictionary or Unknown." },
                    "value": { "description": "Number for numeric and boolean types, string for String and Title. Not given for arrays." },
                    "dimension1": { "type": "integer", "description": "First dimension of an array parameter." },
                    "dimension2": { "type": "integer", "description": "Second dimension of an array parameter." }
                },
                "required": ["name", "displayName", "type"]
            }
        }
                    },
                    "required": ["elementId", "variation", "floorIndex", "layerIndex", "pos", "angle", "length", "xRatio", "yRatio", "useXYFixSize", "level", "libInd", "libraryPartName"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["gridElements"]
    }
            },{
                "name": "CreateGridElements",
                "version": "1.7.2",
                "description": "Creates grid elements, the elements the Grid Tool places, from begin and end coordinates, with optional story, layer and GDL parameters. Needs Archicad 26 or later.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "gridElements": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "begCoordinate": { "$ref": "#/Coordinate2D", "description": "Start point of the grid line (the object's insertion point)." },
                        "endCoordinate": { "$ref": "#/Coordinate2D", "description": "End point of the grid line. Sets the direction and, unless lengthMode is 'none', the length." },
                        "lengthMode": { "type": "string", "enum": ["length", "none"], "description": "How the line length is applied. length (default): to the GDL parameter AC_Length when the library part has it, as the Grid Tool's grid marker does; otherwise to the object's A size, converted to xRatio as CreateObjects does. none: the length is left to 'parameters'." },
                        "floorIndex": { "type": "integer", "description": "Home story index. Default: the tool default." },
                        "layerIndex": { "type": "integer", "description": "Layer attribute index. Only a positive index takes effect. Default: the tool default." },
                        "parameters": {
                            "type": "array",
                            "description": "GDL parameters to set by name (read the names with GetGridElementDefaults). The value is a number, a boolean or a string. Unknown names and wrong value types are reported in warnings and skipped. The tool defaults can look different from a grid drawn with the Grid Tool (for example a short line with a leader to the bubble); to get the same look, copy the parameters of such a grid (GetGridElements with includeParameters).",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "name": { "type": "string" },
                                    "value": { "description": "A number or a boolean for a numeric parameter, a string for a String or Title parameter." }
                                },
                                "additionalProperties": false,
                                "required": ["name", "value"]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": ["begCoordinate", "endCoordinate"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["gridElements"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": { "$ref": "#/ElementIdsOrErrors" },
            "warnings": { "type": "array", "items": { "type": "string" }, "description": "Parameters that were not set, prefixed with the index of the grid element." }
        },
        "additionalProperties": false,
        "required": ["elements", "warnings"]
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
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
                    "buildingMaterialId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional building material of the mesh. Its top, side and bottom surface overrides are cleared, also one from the Mesh tool defaults, so the mesh shows the building material's own surface."
                    },
                    "ridges": {
                        "$ref": "#/MeshRidges"
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
                            "$ref": "#/MeshSubline"
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
                "$ref": "#/ElementIdsOrErrors"
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description" : "The parent element if the label is an associative label."
                    },
                    "labelClass": {
                        "$ref": "#/LabelClass",
                        "description": "Optional; if omitted, inherits the current Label tool default (which may silently resolve to either class - explicitly setting this avoids ambiguity)."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content if the label is a text label. Newlines create multiple lines; Windows-style CRLF (and bare CR) line endings are normalized to LF. Ignored if 'runs' is also given."
                    },
                    "runs": {
                        "type": "array",
                        "description": "Multi-style text content for a text label: an array of styled runs, concatenated in order. Takes precedence over 'text' if both are given.",
                        "items": { "$ref": "#/TextRunDetails" },
                        "minItems": 1
                    },
                    "style": {
                        "$ref": "#/TextStyleSettableDetails",
                        "description": "Ignored for symbol labels."
                    },
                    "symbolStyle": {
                        "$ref": "#/LabelSymbolStyleSettableDetails",
                        "description": "Ignored for text labels."
                    },
                    "leaderLine": {
                        "$ref": "#/LabelLeaderLineSettableDetails"
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
                "$ref": "#/ElementIdsOrErrors"
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
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "coordinate": {
                        "$ref": "#/Coordinate3D",
                        "description": "The placement position of the text. The z value is used to determine the floor when floorIndex is omitted."
                    },
                    "text": {
                        "type": "string",
                        "description": "The text content. Newlines create multiple lines; Windows-style CRLF (and bare CR) line endings are normalized to LF. Ignored if 'runs' is also given."
                    },
                    "runs": {
                        "type": "array",
                        "description": "Multi-style text content: an array of styled runs, concatenated in order. Takes precedence over 'text' if both are given.",
                        "items": { "$ref": "#/TextRunDetails" },
                        "minItems": 1
                    },
                    "height": {
                        "type": "number",
                        "description": "The character height in millimeters. Optional; defaults to the Text tool default. Equivalent to style.height."
                    },
                    "pen": {
                        "type": "integer",
                        "description": "Optional pen attribute index. Equivalent to style.penIndex."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Optional rotation angle in radians. Equivalent to style.angle."
                    },
                    "justification": {
                        "$ref": "#/Justification",
                        "description": "Optional; equivalent to style.justification."
                    },
                    "style": {
                        "$ref": "#/TextStyleSettableDetails",
                        "description": "height/pen/angle/justification above take precedence over the same fields here if both are given."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Optional layer attribute index. If omitted, the Text tool default layer is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinate"
                ],
                "anyOf": [
                    { "required": ["text"] },
                    { "required": ["runs"] }
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateLevelDimensions",
                "version": "1.7.2",
                "description": "Creates Level Dimension elements (the Level Dimension tool) based on the given parameters. With a parentElementId the dimension is associative: it shows the height of the parent element (Mesh, Slab, Roof ...) at its position and follows the element when it changes.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "levelDimensionsData": {
            "type": "array",
            "description": "Array of data to create Level Dimensions (the Level Dimension tool of the floor plan).",
            "items": {
                "type": "object",
                "description": "The parameters of the new Level Dimension.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a Level Dimension favorite to base the new element on. Its settings (marker, format, text style, layer, pen) are applied first, then the explicitly given fields override them."
                    },
                    "position": {
                        "$ref": "#/Coordinate2D",
                        "description": "The location of the marker on the floor plan. With a parentElementId it must lie on that element, the measured level is the parent's height at this point. Outside the parent's outline Archicad still creates the dimension, but without a height (GetDetailsOfElements reads a level of -110000000)."
                    },
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description": "Optional element the level dimension is associated with (Mesh, Slab, Roof, Shell, Morph, Beam, Column, Wall, Object, Stair ...). The dimension then shows the element's height at the position and follows the element when it changes. Without it the dimension measures the floor level, or the given static level."
                    },
                    "level": {
                        "type": "number",
                        "description": "Optional fixed level value in meters (absolute, from the project zero). Only used without parentElementId; the dimension then shows this value instead of the floor level."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "The index of the floor to place the dimension on. Optional parameter, by default the floor of the parent element, otherwise the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the dimension on. Optional parameter, by default the layer of the favorite or the Level Dimension tool default is used."
                    },
                    "pen": {
                        "type": "integer",
                        "description": "Pen index of the marker. Optional parameter."
                    },
                    "textHeight": {
                        "type": "number",
                        "description": "Height of the dimension text in millimeters (paper size). Optional parameter."
                    },
                    "textPen": {
                        "type": "integer",
                        "description": "Pen index of the dimension text. Optional parameter."
                    },
                    "dimForm": {
                        "type": "integer",
                        "minimum": 0,
                        "maximum": 19,
                        "description": "Marker type of the Level Dimension tool, 0-19 as Archicad stores it (read it back with GetDetailsOfElements from an existing level dimension or one placed with a favorite). Optional parameter."
                    },
                    "needPlus": {
                        "type": "boolean",
                        "description": "Show a plus sign for positive values. Optional parameter."
                    },
                    "markerSize": {
                        "type": "number",
                        "description": "Size of the marker in millimeters (paper size). Optional parameter."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Rotation angle of the marker in radians. Optional parameter."
                    },
                    "origin": {
                        "$ref": "#/DimensionOrigin",
                        "description": "The origin the level is measured from. Optional parameter."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "levelDimensionsData"
    ]
},
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
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
                        "geometryType": { "$ref": "#/WallModificationGeometryType", "description": "Polygonal is not settable here; it is read-only via GetDetailsOfElements. This is unrelated to slantAlpha/slantBeta - see profileType for the cross section shape those depend on." },
                        "begCoordinate": { "$ref": "#/Coordinate2D" },
                        "endCoordinate": { "$ref": "#/Coordinate2D" },
                        "arcAngle": { "type": "number", "description": "Arc angle in radians; non-zero makes the wall curved (begCoordinate/endCoordinate are the chord endpoints)." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Sets relativeTopStory to 0 (explicit height). Do not combine with relativeTopStory in the same call - whichever is applied last wins, and Archicad recomputes the actual height from the story elevations once relativeTopStory is non-zero." },
                        "thickness": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Only takes effect on a Basic (homogeneous) wall - a Composite or Profile wall takes its thickness from the composite or profile attribute. Send structureType 'Basic' in the same call to switch such a wall first."
                        },
                        "bottomOffset": { "type": "number" },
                        "offset": { "type": "number" },
                        "structureType": {
                            "$ref": "#/WallStructureType"
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "profileId": { "$ref": "#/AttributeId" },
                        "referenceLineLocation": {
                            "$ref": "#/WallReferenceLineLocation"
                        },
                        "profileType": { "$ref": "#/WallModificationProfileType", "description": "slantAlpha/slantBeta only have an effect once this is set to Slanted." },
                        "slantAlpha": { "type": "number", "description": "Only has an effect once profileType is set to Slanted or Trapez." },
                        "slantBeta": { "type": "number", "description": "Only has an effect once profileType is set to Slanted or Trapez." },
                        "topOffset": { "type": "number", "description": "Only has an effect when relativeTopStory is non-zero." },
                        "relativeTopStory": { "type": "number", "description": "Non-zero links the wall's top to another story instead of an explicit height - do not set together with 'height' in the same call, see the note on 'height' above." },
                        "zoneRel": {
                            "$ref": "#/WallZoneRelation"
                        },
                        "visibility": { "$ref": "#/StoryVisibility" },
                        "isAutoOnStoryVisibility": { "type": "boolean", "description": "When true (the default on a new wall), Archicad recomputes 'visibility' automatically from the wall's vertical extent and ignores any value set for it. Setting 'visibility' without also setting this field turns it off automatically." },
                        "referenceMaterial": { "$ref": "#/OverriddenMaterial" },
                        "oppositeMaterial": { "$ref": "#/OverriddenMaterial" },
                        "sideMaterial": { "$ref": "#/OverriddenMaterial" },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "contourPen": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen of the wall's own contour/reference outline (API_WallType::contPen) - distinct from cutFillPen, which is the cut fill pattern's pen. Not inheritable/overridable like cutFillPen, it is the wall's own pen index directly, the same field regardless of structureType."
                        },
                        "contourPen3D": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen of the wall's contour in 3D views (API_WallType::contPen3D)."
                        },
                        "contourLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type of the wall's own contour (API_WallType::contLtype) - pairs with contourPen, which only sets the pen, not the line type."
                        },
                        "belowViewLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type used when \"OutLines Only\" is selected from the \"Floor Plan Display\" popup (API_WallType::belowViewLineType)."
                        },
                        "aboveViewLinePen": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Pen used when \"Overhead All\" is selected from the \"Floor Plan Display\" popup."
                        },
                        "aboveViewLineTypeId": {
                            "$ref": "#/AttributeId",
                            "description": "Line type used when \"Overhead All\" is selected from the \"Floor Plan Display\" popup - pairs with aboveViewLinePen."
                        },
                        "displayOption": {
                            "$ref": "#/DisplayOption"
                        },
                        "viewDepthLimitation": {
                            "$ref": "#/ViewDepthLimitation",
                            "description": "Only has an effect on a multi-story wall (relativeTopStory/topOffset making it span more than one story)."
                        },
                        "useCompositePriority": {
                            "type": "boolean",
                            "description": "When true, the Composite's own priority is used at junctions instead of connectionPriority."
                        },
                        "linkToSettings": {
                            "$ref": "#/LinkToSettings"
                        },
                        "inheritEndSurface": {
                            "type": "boolean",
                            "description": "The end surface of the wall is inherited from the adjoining wall."
                        },
                        "alignTexture": {
                            "type": "boolean",
                            "description": "Align texture mapping to the wall's edges."
                        },
                        "polyCanChange": {
                            "type": "boolean",
                            "description": "The polygonal wall's corners can be changed in case of L and T connections. Only meaningful for a Polygonal wall (geometryType)."
                        },
                        "materialsChained": {
                            "type": "boolean",
                            "description": "Whether the surface materials are chained."
                        },
                        "logHeight": {
                            "type": "number",
                            "description": "Height of the log for log walls. 0 means a normal (non-log) wall."
                        }
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
                        "verticalCurveHeight": { "type": "number" },
                        "beamShape": { "type": "string", "enum": ["Straight", "HorizontallyCurved", "VerticallyCurved"] },
                        "isSlanted": { "type": "boolean" },
                        "isFlipped": { "type": "boolean" },
                        "profileAngle": { "type": "number" },
                        "anchorPoint": { "$ref": "#/BeamAnchorPoint" },
                        "width": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section width of the beam. Applied to all segments." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section height of the beam. Applied to all segments." },
                        "isWidthAndHeightLinked": { "type": "boolean", "description": "When true, Archicad keeps width and height equal and setting one changes the other - set to false first to give width/height independent values. Applied to all segments." },
                        "circleBased": { "type": "boolean", "description": "True for a round beam cross section, false for rectangular. Ignored once profileId switches the beam to a custom profile shape. Applied to all segments." },
                        "buildingMaterialId": { "$ref": "#/AttributeId", "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments." },
                        "profileId": { "$ref": "#/AttributeId", "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments." },
                        "holes": {
                            "type": "array",
                            "description": "Replaces all holes currently placed on the beam.",
                            "items": {
                                "$ref": "#/BeamHoleModification"
                            }
                        },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "coverFill": { "$ref": "#/CoverFill" }
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
                        "thickness": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Only takes effect on a Basic (homogeneous) slab - a Composite slab takes its thickness from the composite attribute. Send structureType 'Basic' in the same call to switch a composite slab first."
                        },
                        "structureType": {
                            "$ref": "#/SlabStructureType"
                        },
                        "buildingMaterialId": { "$ref": "#/AttributeId" },
                        "compositeId": { "$ref": "#/AttributeId" },
                        "referencePlaneLocation": {
                            "$ref": "#/SlabReferencePlaneLocation"
                        },
                        "polygonOutline": {
                            "type": "array",
                            "items": { "$ref": "#/Coordinate2D" },
                            "minItems": 3,
                            "description": "Replaces the slab's entire polygon, including its holes - resend the holes field too to keep them, otherwise they are removed."
                        },
                        "polygonArcs": {
                            "type": "array",
                            "items": { "$ref": "#/PolyArc" }
                        },
                        "holes": {
                            "$ref": "#/Holes2D",
                            "description": "Can be given on its own, without polygonOutline, to add/remove/clear holes in place (an empty array clears all holes) - the slab's current outline is reused unchanged."
                        },
                        "topMaterial": { "$ref": "#/OverriddenMaterial" },
                        "sideMaterial": { "$ref": "#/OverriddenMaterial" },
                        "bottomMaterial": { "$ref": "#/OverriddenMaterial" },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "floorFill": { "$ref": "#/FloorFill" }
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
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Sets relativeTopStory to 0 (explicit height). Do not combine with relativeTopStory in the same call - see the note on relativeTopStory below." },
                        "bottomOffset": { "type": "number" },
                        "axisRotationAngle": { "type": "number" },
                        "coreAnchor": { "$ref": "#/ColumnCoreAnchor" },
                        "isSlanted": { "type": "boolean" },
                        "slantAngle": { "type": "number" },
                        "slantDirectionAngle": { "type": "number" },
                        "isFlipped": { "type": "boolean", "description": "Has no visible effect on a circular column (circleBased cross section) - Archicad ignores it there." },
                        "wrapping": { "type": "boolean" },
                        "topOffset": { "type": "number" },
                        "relativeTopStory": { "type": "number", "description": "Non-zero links the column's top to another story instead of an explicit height - do not set together with 'height' in the same call, see the note on 'height' above." },
                        "width": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section width of the column. Applied to all segments." },
                        "depth": { "type": "number", "exclusiveMinimum": 0.0, "description": "Cross section depth (height) of the column. Applied to all segments." },
                        "isWidthAndHeightLinked": { "type": "boolean", "description": "When true, Archicad keeps width and depth equal and setting one changes the other - set to false first to give width/depth independent values. Applied to all segments." },
                        "circleBased": { "type": "boolean", "description": "True for a round column cross section, false for rectangular. Ignored once profileId switches the column to a custom profile shape. Applied to all segments." },
                        "buildingMaterialId": { "$ref": "#/AttributeId", "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments." },
                        "profileId": { "$ref": "#/AttributeId", "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments." },
                        "cutFillPen": { "$ref": "#/OverriddenPen" },
                        "cutFillBackgroundPen": { "$ref": "#/OverriddenPen" },
                        "coverFill": { "$ref": "#/CoverFill" }
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
                        "oSide": { "type": "boolean" },
                        "reveal": { "type": "boolean", "description": "Turn the reveal on or off." },
                        "revealDepthOffset": { "type": "number", "description": "Distance the frame plane is moved across the wall thickness, along the wall normal." }
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
                        "oSide": { "type": "boolean" },
                        "reveal": { "type": "boolean", "description": "Turn the reveal on or off." },
                        "revealDepthOffset": { "type": "number", "description": "Distance the frame plane is moved across the wall thickness, along the wall normal." }
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
                            "$ref": "#/LinkToSettings"
                        },
                        "displayOption": {
                            "$ref": "#/DisplayOption"
                        },
                        "viewDepthLimitation": {
                            "$ref": "#/ViewDepthLimitation"
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
                            "$ref": "#/CoverFillOrientation"
                        },
                        "useDistortedCoverFill": { "type": "boolean" },
                        "textureProjectionType": {
                            "$ref": "#/TextureProjectionType"
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
                        "thickness": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Only takes effect on a Basic (homogeneous) roof - a Composite roof takes its thickness from the composite attribute. Send structureType 'Basic' in the same call to switch a composite roof first."
                        },
                        "eavesOverhang": { "type": "number" },
                        "levels": {
                            "type": "array",
                            "minItems": 1,
                            "maxItems": 16,
                            "items": {
                                "$ref": "#/RoofLevel"
                            }
                        },
                        "structureType": {
                            "$ref": "#/RoofStructureType"
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
                        "$ref": "#/MeshModificationData"
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
                "name": "ModifyObjects",
                "version": "1.5.7",
                "description": "Modifies Object elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "objectsWithDetails": {
                "type": "array",
                "description": "Array of elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        
        "angle": { "type": "number" },
        "pen": { "type": "integer" },
        "lineTypeId": { "$ref": "#/AttributeId" },
        "surfaceId": { "$ref": "#/AttributeId", "description": "Material/Surface override (API_ObjectType.mat)." },
        "sectionFillId": { "$ref": "#/AttributeId" },
        "sectionFillPen": { "type": "integer" },
        "sectionFillBackgroundPen": { "type": "integer" },
        "sectionContourPen": { "type": "integer" },
        "useObjectPens": { "type": "boolean" },
        "useObjectLineTypes": { "type": "boolean" },
        "useObjectMaterials": { "type": "boolean" },
        "useObjectSectionAttributes": { "type": "boolean" },
        "reflected": { "type": "boolean" },
        "useFixSize": { "type": "boolean" },
        "fixPoint": { "type": "integer", "description": "0-based index of the hotspot to keep fixed (raw API_ObjectType.fixPoint value, not 1-based)." },
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType/level." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "$ref": "#/StoryVisibility"
        },
        "linkToSettings": {
            "$ref": "#/LinkToSettings"
        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "objectsWithDetails"
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
                "name": "ModifyLamps",
                "version": "1.5.7",
                "description": "Modifies Lamp elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "lampsWithDetails": {
                "type": "array",
                "description": "Array of elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        
        "angle": { "type": "number" },
        "pen": { "type": "integer" },
        "lineTypeId": { "$ref": "#/AttributeId" },
        "surfaceId": { "$ref": "#/AttributeId", "description": "Material/Surface override (API_ObjectType.mat)." },
        "sectionFillId": { "$ref": "#/AttributeId" },
        "sectionFillPen": { "type": "integer" },
        "sectionFillBackgroundPen": { "type": "integer" },
        "sectionContourPen": { "type": "integer" },
        "useObjectPens": { "type": "boolean" },
        "useObjectLineTypes": { "type": "boolean" },
        "useObjectMaterials": { "type": "boolean" },
        "useObjectSectionAttributes": { "type": "boolean" },
        "reflected": { "type": "boolean" },
        "useFixSize": { "type": "boolean" },
        "fixPoint": { "type": "integer", "description": "0-based index of the hotspot to keep fixed (raw API_ObjectType.fixPoint value, not 1-based)." },
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType/level." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "$ref": "#/StoryVisibility"
        },
        "linkToSettings": {
            "$ref": "#/LinkToSettings"
        },
        "lightColor": { "$ref": "#/ColorRGB", "description": "Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default light color). lightIsOn (the on/off state, as opposed to the color) does not have this problem." },
        "lightIsOn": { "type": "boolean" }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "lampsWithDetails"
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
                "name": "ModifyTexts",
                "version": "1.5.9",
                "description": "Modifies standalone Text elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "textsWithDetails": {
                "type": "array",
                "description": "Array of Text elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is. A change of the text, the runs, or a run-level style field (pen, font, faces, height, effects) rebuilds the content as one paragraph, which makes the element auto-width (word wrap off), as SetDetailsOfElements does, and on a multi-run text applies that style to every run; the other style fields leave the content as it is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "coordinate": {
                            "$ref": "#/Coordinate3D",
                            "description": "The new placement position. As in CreateTexts, the z value selects the floor when floorIndex is omitted."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional. Moves the text to this floor; when omitted and a coordinate is given, the floor is derived from its z value."
                        },
                        "text": { "type": "string", "description": "The new text content. Newlines create multiple lines; Windows-style CRLF (and bare CR) line endings are normalized to LF." },
                        "runs": {
                            "type": "array",
                            "items": { "$ref": "#/TextRunDetails" },
                            "minItems": 1
                        },
                        "style": { "$ref": "#/TextStyleSettableDetails" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["textsWithDetails"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    }
            },{
                "name": "ModifyLabels",
                "version": "1.5.9",
                "description": "Modifies Label elements based on the given parameters.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "labelsWithDetails": {
                "type": "array",
                "description": "Array of Label elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is. The label's class (Text/Symbol) cannot be changed after creation. A change of the text, the runs, or a run-level style field (pen, font, faces, height, effects) rebuilds the content as one paragraph, which makes the label's text auto-width (word wrap off), as SetDetailsOfElements does, and on a multi-run label applies that style to every run; the other style fields leave the content as it is.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "text": { "type": "string", "description": "The new text content. Newlines create multiple lines; Windows-style CRLF (and bare CR) line endings are normalized to LF." },
                        "runs": {
                            "type": "array",
                            "items": { "$ref": "#/TextRunDetails" },
                            "minItems": 1
                        },
                        "style": { "$ref": "#/TextStyleSettableDetails" },
                        "symbolStyle": { "$ref": "#/LabelSymbolStyleSettableDetails" },
                        "leaderLine": { "$ref": "#/LabelLeaderLineSettableDetails" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["labelsWithDetails"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
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
            },{
                "name": "GetGroupsOfElements",
                "version": "1.5.6",
                "description": "Gets the identifier of the group that directly contains each given element. Returns an error for elements that are not part of any group.",
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
            "groupGuids": {
                "type": "array",
                "description": "The identifier of the group that directly contains each given element, or an error for elements that are not part of any group.",
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
            },{
                "name": "GetElementsOfGroups",
                "version": "1.5.6",
                "description": "Gets the elements directly contained by each given group.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "groups": {
                "type": "array",
                "description": "The groups to get the elements of.",
                "items": {
                    "$ref": "#/GroupIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "groups"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elementsOfGroups": {
                "type": "array",
                "description": "The elements directly contained by each given group, or an error.",
                "items": {
                    "$ref": "#/ElementsWrapperOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsOfGroups"
        ]
    }
            },{
                "name": "GetSuspendGroupsMode",
                "version": "1.5.6",
                "description": "Gets the current state of the Suspend Groups mode.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "suspendGroups": {
                "type": "boolean",
                "description": "True if the Suspend Groups mode is currently on."
            }
        },
        "additionalProperties": false,
        "required": [
            "suspendGroups"
        ]
    }
            },{
                "name": "SetSuspendGroupsMode",
                "version": "1.5.6",
                "description": "Turns the Suspend Groups mode on or off. Suspend groups to perform operations on elements that are part of a group; remember to restore the previous state afterwards.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "suspendGroups": {
                "type": "boolean",
                "description": "Turn the Suspend Groups mode on or off."
            }
        },
        "additionalProperties": false,
        "required": [
            "suspendGroups"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
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
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "ApplyFavoritesToElements",
                "version": "1.5.4",
                "description": "Apply the given favorites to existing elements. Only settings-type parameters are changed - geometry (position, floor, and dimensions such as a Wall's height) is left untouched, so applying a Favorite never moves or resizes the target element. For the hierarchical types (Stair, Railing, Curtain Wall) the settings of the sub-elements are not applied, because they are inseparable from the Favorite's own geometry. By default settings, classifications, categories and properties are all applied; each can be opted out of individually.",
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
                "description": "Whether to apply the Favorite's settings-type parameters (structure, materials, pens, etc. - never geometry). For the hierarchical types (Stair, Railing, Curtain Wall) the settings of the sub-elements are not applied, because they are inseparable from the Favorite's own geometry. Default is true."
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
                    "$ref": "#/FavoritesFromElementUpdate"
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
                    "$ref": "#/FavoriteRename"
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
            },
            "propertyGroups": {
                "type": "array",
                "description": "Every property group, including empty ones.",
                "items": {
                    "$ref": "#/PropertyGroupDetails"
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
                "description": "The order of the outer list is that of the given elements. The order of the inner lists are that of the given properties."
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
                "description": "The order of the outer list is that of the given attributes. The order of the inner lists are that of the given properties."
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
                "description": "Updates existing Custom Property Definitions in place, keeping their guid: name, description, group, default value or expressions, availability, and enum options (add, rename, remove, reorder).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyDefinitions": {
                "type": "array",
                "description": "The property definitions to update. Only the fields given change; the definition keeps its guid, so element values survive.",
                "items": {
                    "$ref": "#/PropertyDefinitionUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "propertyDefinitions" ]
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
                "name": "UpdatePropertyGroups",
                "version": "1.6.0",
                "description": "Updates the name and/or description of existing Custom Property Groups, keeping their guid.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "propertyGroups": {
                "type": "array",
                "description": "The custom property groups to update. Only the fields given change.",
                "items": {
                    "$ref": "#/PropertyGroupUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "propertyGroups" ]
    },
                "outputScheme": {
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    }
            },{
                "name": "ImportPropertiesXml",
                "version": "1.6.0",
                "description": "Imports a Property Manager XML export, with the given policy for names that already exist. Returns the property definitions it created and removed.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "xml": { "type": "string", "description": "A Property Manager export (XML) to import." },
            "conflictPolicy": { "$ref": "#/PropertyImportConflictPolicy" }
        },
        "additionalProperties": false,
        "required": [ "xml", "conflictPolicy" ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResult": { "$ref": "#/ExecutionResult" },
            "created": { "type": "array", "items": { "$ref": "#/GuidId" } },
            "removed": { "type": "array", "items": { "$ref": "#/GuidId" } }
        },
        "additionalProperties": false,
        "required": [ "executionResult", "created", "removed" ]
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
                "description": "Order of the ids are the same as in the input. Non-existing elements or non-existing classification systems are represented by error objects."
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
            },{
                "name": "UpdateClassificationSystems",
                "version": "1.6.0",
                "description": "Updates the name, description, source, version and/or date of existing Classification Systems, keeping their guid.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "classificationSystems": {
                "type": "array",
                "description": "The classification systems to update. Only the fields given change.",
                "items": {
                    "$ref": "#/ClassificationSystemUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "classificationSystems" ]
    },
                "outputScheme": {
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    }
            },{
                "name": "UpdateClassificationItems",
                "version": "1.6.0",
                "description": "Updates the id (code), name and/or description of existing Classification Items, keeping their guid and so the elements classified with them. Items cannot be moved to another parent.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "classificationItems": {
                "type": "array",
                "description": "The classification items to update. Only the fields given change; items keep their guid and their parent.",
                "items": {
                    "$ref": "#/ClassificationItemUpdate"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "classificationItems" ]
    },
                "outputScheme": {
        "type": "object",
        "properties": { "executionResults": { "$ref": "#/ExecutionResults" } },
        "additionalProperties": false,
        "required": [ "executionResults" ]
    }
            },{
                "name": "ImportClassificationsXml",
                "version": "1.6.0",
                "description": "Imports a Classification Manager XML export, with the given policies for systems and items that already exist. Returns the systems and items it created and removed.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "xml": { "type": "string", "description": "A Classification Manager export (XML) to import." },
            "systemConflictPolicy": { "$ref": "#/ClassificationSystemConflictPolicy" },
            "itemConflictPolicy": { "$ref": "#/ClassificationItemConflictPolicy" }
        },
        "additionalProperties": false,
        "required": [ "xml", "systemConflictPolicy", "itemConflictPolicy" ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResult": { "$ref": "#/ExecutionResult" },
            "created": { "type": "array", "items": { "$ref": "#/GuidId" } },
            "removed": { "type": "array", "items": { "$ref": "#/GuidId" } }
        },
        "additionalProperties": false,
        "required": [ "executionResult", "created", "removed" ]
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                                "$ref": "#/LineDashItem"
                            }
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Symbol items describing one period (Symbol line type only).",
                            "items": {
                                "$ref": "#/LineSymbolItemData"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                            "description": "Used for Image and gradient fills. Only name, rotationAngle, xSize, ySize, mirrorX, and mirrorY are used for Fills.",
                            "$ref": "#/Texture"
                        },
                        "lineItems": {
                            "type": "array",
                            "description": "Vectorial fill line items (Vector fills only).",
                            "items": {
                                "$ref": "#/FillLineItem"
                            }
                        },
                        "symbolLines": {
                            "type": "array",
                            "description": "Line items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/FillSymbolLine"
                            }
                        },
                        "symbolArcs": {
                            "type": "array",
                            "description": "Arc items of the fill's repeating symbol pattern (Symbol fills only).",
                            "items": {
                                "$ref": "#/FillSymbolArc"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "description": "Creates or overwrites MEP System attributes (domain, pens, fill, center line type, surface and insulation surface) based on the given parameters.",
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
                            "$ref": "#/MEPDomain",
                            "description": "Only elements belonging to the same domain can use this system."
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
                        },
                        "materialId": {
                            "description": "Identifier of the surface (material) attribute applied to the 3D model of the elements of the system.",
                            "$ref": "#/AttributeIdArrayItem"
                        },
                        "insulationMaterialId": {
                            "description": "Identifier of the surface (material) attribute applied to the insulation of the elements of the system.",
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
                "$ref": "#/AttributeIdsOrErrors"
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
                                "$ref": "#/PenData"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                                            "$ref": "#/ProfileEdgeOverride"
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
                                "$ref": "#/ProfileSkinData"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                "$ref": "#/AttributeIdsOrErrors"
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
                    "$ref": "#/LineAttributeField"
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
                    "$ref": "#/FillAttributeField"
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
                    "$ref": "#/ZoneCategoryAttributeField"
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
                "description": "Returns the details of the given MEP System attributes (domain, pens, fill, center line type, surface and insulation surface).",
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
                    "$ref": "#/MEPSystemAttributeField"
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
                    "$ref": "#/PenTableAttributeField"
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
                    "$ref": "#/ProfileAttributeField"
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
                    "$ref": "#/CompositeAttributeField"
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
                    "$ref": "#/SurfaceAttributeField"
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
                    "$ref": "#/LayerAttributeField"
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
                    "$ref": "#/BuildingMaterialAttributeField"
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
            },{
                "name": "GetGraphicOverrideRules",
                "version": "1.7.2",
                "description": "Gets all graphic override rules of the project, with their rule group, criterion and style. Available from Archicad 27.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules of the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideRuleId": {
                            "$ref": "#/GraphicOverrideRuleId"
                        },
                        "name": {
                            "type": "string"
                        },
                        "ruleGroupName": {
                            "type": "string",
                            "description": "The name of the rule group the rule belongs to."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The criterion of the rule, in the XML format Archicad saves override rules in. Change its values and pass it to CreateGraphicOverrideRules or ModifyGraphicOverrideRules to make similar rules."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideRuleId",
                        "name",
                        "criterionXML",
                        "style"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    }
            },{
                "name": "CreateGraphicOverrideRules",
                "version": "1.7.2",
                "description": "Creates graphic override rules. The criterion is given as Archicad's criterion XML or as structured conditions. The rule group is created when no rule group has the given name. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the rule, unique within the project."
                        },
                        "ruleGroupName": {
                            "type": "string",
                            "description": "The name of the rule group to put the rule in. Created when it does not exist."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The criterion of the rule, in the XML format Archicad saves override rules in. Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from scratch without an error, and the rule then applies to all elements. Left out together with 'criterion': Archicad's default criterion for new rules."
                        },
                        "criterion": {
                            "$ref": "#/GraphicOverrideCriterion",
                            "description": "The criterion of the rule as structured conditions, turned into Archicad's criterion XML. Use it instead of criterionXML, not with it."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "ruleGroupName"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRuleIds": {
                "type": "array",
                "description": "The identifiers of the created rules, or errors, in the order of the input.",
                "items": {
                    "$ref": "#/GraphicOverrideRuleIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRuleIds"
        ]
    }
            },{
                "name": "ModifyGraphicOverrideRules",
                "version": "1.7.2",
                "description": "Modifies the name, criterion or style of graphic override rules. The criterion is given as Archicad's criterion XML or as structured conditions. A given style replaces the whole style of the rule. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRules": {
                "type": "array",
                "description": "The graphic override rules to modify.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideRuleId": {
                            "$ref": "#/GraphicOverrideRuleId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the rule."
                        },
                        "criterionXML": {
                            "type": "string",
                            "description": "The new criterion of the rule, in the XML format Archicad saves override rules in. Take it from GetGraphicOverrideRules and change its values: Archicad drops XML written from scratch without an error, and the rule then applies to all elements."
                        },
                        "criterion": {
                            "$ref": "#/GraphicOverrideCriterion",
                            "description": "The new criterion of the rule as structured conditions, turned into Archicad's criterion XML. Use it instead of criterionXML, not with it."
                        },
                        "style": {
                            "$ref": "#/GraphicOverrideStyle"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideRuleId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRules"
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
                "name": "DeleteGraphicOverrideRules",
                "version": "1.7.2",
                "description": "Deletes graphic override rules. They are also removed from the combinations that contain them. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRuleIds": {
                "type": "array",
                "description": "The graphic override rules to delete.",
                "items": {
                    "$ref": "#/GraphicOverrideRuleIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRuleIds"
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
                "name": "GetGraphicOverrideCombinations",
                "version": "1.7.2",
                "description": "Gets all graphic override combinations of the project with the rules they contain. Available from Archicad 27.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideCombinations": {
                "type": "array",
                "description": "The graphic override combinations of the project.",
                "items": {
                    "type": "object",
                    "properties": {
                        "graphicOverrideCombinationId": {
                            "$ref": "#/GraphicOverrideCombinationId"
                        },
                        "name": {
                            "type": "string"
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the combination, in priority order.",
                            "items": {
                                "$ref": "#/GraphicOverrideRuleIdArrayItem"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "graphicOverrideCombinationId",
                        "name",
                        "graphicOverrideRuleIds"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinations"
        ]
    }
            },{
                "name": "CreateGraphicOverrideCombinations",
                "version": "1.7.2",
                "description": "Creates graphic override combinations from existing rules. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideCombinations": {
                "type": "array",
                "description": "The graphic override combinations to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the combination, unique within the project."
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the combination, in priority order.",
                            "items": {
                                "$ref": "#/GraphicOverrideRuleIdArrayItem"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "graphicOverrideRuleIds"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinations"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideCombinationIds": {
                "type": "array",
                "description": "The identifiers of the created combinations, or errors, in the order of the input.",
                "items": {
                    "$ref": "#/GraphicOverrideCombinationIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinationIds"
        ]
    }
            },{
                "name": "DeleteGraphicOverrideCombinations",
                "version": "1.7.2",
                "description": "Deletes graphic override combinations. Their rules are kept. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideCombinationIds": {
                "type": "array",
                "description": "The graphic override combinations to delete. Their rules are kept.",
                "items": {
                    "$ref": "#/GraphicOverrideCombinationIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideCombinationIds"
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
                "name": "GetGraphicOverrideRuleGroups",
                "version": "1.7.2",
                "description": "Gets all graphic override rule groups of the project with the rules they contain, empty groups included. Available from Archicad 27.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "graphicOverrideRuleGroups": {
                "type": "array",
                "description": "The graphic override rule groups of the project, empty ones included.",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "graphicOverrideRuleIds": {
                            "type": "array",
                            "description": "The rules of the group.",
                            "items": {
                                "$ref": "#/GraphicOverrideRuleIdArrayItem"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "graphicOverrideRuleIds"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "graphicOverrideRuleGroups"
        ]
    }
            },{
                "name": "DeleteGraphicOverrideRuleGroups",
                "version": "1.7.2",
                "description": "Deletes graphic override rule groups, found by name. A group that still contains rules is kept unless its rules are moved to another group or deleted with it. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "ruleGroupNames": {
                "type": "array",
                "description": "The names of the rule groups to delete.",
                "items": {
                    "type": "string"
                }
            },
            "moveRulesToGroup": {
                "type": "string",
                "description": "The name of the rule group the rules of the deleted groups are moved to, so they are kept. Created when no rule group has this name."
            },
            "deleteRules": {
                "type": "boolean",
                "description": "Delete the rules of the deleted groups too. Without this or moveRulesToGroup, a group that still contains rules is not deleted."
            }
        },
        "additionalProperties": false,
        "required": [
            "ruleGroupNames"
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
            "name": "IFC Commands",
            "commands": [{
                "name": "IFCFileOperation",
                "version": "1.2.6",
                "description": "Executes an IFC file operation: opens or merges an IFC file, or saves the project as an IFC file. A save can name the export translator to use.",
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
            },
            "translatorName": {
                "type": "string",
                "description": "Only for the save method: the name of the IFC export translator to save with, as GetIFCExportTranslators lists them. Without it the save runs with the translator Archicad would offer in its own Save dialog. Needs a fileType of ifc or ifczip (ifc or ifcxml on Archicad 25 and 26)."
            },
            "elementsToExport": {
                "$ref": "#/IFCElementsToExport",
                "description": "Only for the save method, and only together with translatorName. The default is VisibleElementsOnAllStories."
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
                "name": "GetIFCExportTranslators",
                "version": "1.6.0",
                "description": "Lists the IFC export translators of the project, the preview translator first. Pass one of the names to IFCFileOperation as translatorName.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "translators": {
                "type": "array",
                "description": "The IFC export translators of the project, the preview translator first.",
                "items": {
                    "$ref": "#/IFCExportTranslator"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "translators"
        ]
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
            },{
                "name": "SetIFCPropertiesOfElements",
                "version": "1.7.0",
                "description": "Sets local IFC properties on elements by creating or modifying single value (IfcPropertySingleValue) properties in the given property sets. Available only in Archicad 25, 26 and 27 for now.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementIFCPropertyValues": {
                "type": "array",
                "description": "The IFC property values to set. Only single value (IfcPropertySingleValue) properties are supported.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "propertySetName": {
                            "type": "string",
                            "description": "The name of the property set, for example Pset_DoorCommon. Custom property set names are also accepted."
                        },
                        "propertyName": {
                            "type": "string",
                            "description": "The name of the property, for example FireRating."
                        },
                        "type": {
                            "type": "string",
                            "description": "The IFC type of the value, for example IfcLabel, IfcText, IfcIdentifier, IfcBoolean, IfcLogical, IfcInteger, IfcReal or a measure type like IfcThermalTransmittanceMeasure."
                        },
                        "value": {
                            "description": "The new value of the property. Provide a boolean for IfcBoolean and IfcLogical (IfcLogical also accepts the string Unknown), a number for the numeric types and a string for every other type."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "propertySetName",
                        "propertyName",
                        "type",
                        "value"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementIFCPropertyValues"
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
                    "$ref": "#/Library"
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
                "description": "Adds the given files into the embedded library. With the overwriteExisting flag an embedded library item already existing on an outputPath is replaced (Archicad 27 or newer).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "files": {
                "$ref": "#/LibraryFileAdditions"
            },
            "overwriteExisting": {
                "type": "boolean",
                "description": "Optional. When true, an embedded library item already existing on an outputPath is deleted first, so the newly added file replaces the loaded library part. By default false: the existing loaded part stays in use. Requires Archicad 27 or newer."
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
                "name": "DeleteEmbeddedLibraryItems",
                "version": "1.6.0",
                "description": "Deletes the given items from the embedded library. The path of an item is the same relative path that AddFilesToEmbeddedLibrary takes as outputPath. Available from Archicad 27.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "embeddedLibraryItems": {
                "type": "array",
                "description": "A list of embedded library items to delete.",
                "items": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "The relative path of the library item inside the embedded library, the same path AddFilesToEmbeddedLibrary takes as outputPath."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "path"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "embeddedLibraryItems"
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
                "name": "SetLibraries",
                "version": "1.5.9",
                "description": "Makes the given folders the project's local libraries; built-in, embedded, server and web libraries are kept. Set the libraries before opening a file that needs them and the missing-library dialog does not appear.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "Local library folders or container files, by absolute path.",
                "items": {
                    "$ref": "#/LibraryLocation"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "AddLibraries",
                "version": "1.5.9",
                "description": "Adds the given folders to the project's local libraries, skipping any already loaded.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "libraries": {
                "type": "array",
                "description": "Local library folders or container files, by absolute path.",
                "items": {
                    "$ref": "#/LibraryLocation"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "libraries"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
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
                            "$ref": "#/User"
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
            },{
                "name": "ReserveLockableObjectSet",
                "version": "1.7.1",
                "description": "Reserves a non-element lockable object set (attributes, favorites, project info, project preferences, etc.) in Teamwork mode. The object set is given by its Archicad API name, e.g. 'Composites', 'LayerSettingsDialog' or 'ProjectInfo'.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "objectSetName": {
                "type": "string",
                "description": "The name of the lockable object set, as the Archicad API documents it: BuildingMaterials, Cities, Composites, Favorites, FillTypes, LayerSettingsDialog (layers and layer combinations), LineTypes, MarkupStyles, MEPSystems, ModelViewOptions, OperationProfiles, PenTables, Profiles, ProjectInfo, GeoLocation (project location), PreferencesDialog (project preferences and dimension standards), GraphicOverrides (renovation override styles), Surfaces or ZoneCategories. Other names the Archicad API accepts work too."
            }
        },
        "additionalProperties": false,
        "required": [
            "objectSetName"
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
                "description": "The users whose reservation prevented reserving the object set.",
                "items": {
                    "$ref": "#/User"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    }
            },{
                "name": "ReleaseLockableObjectSet",
                "version": "1.7.1",
                "description": "Releases a non-element lockable object set reserved with ReserveLockableObjectSet in Teamwork mode.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "objectSetName": {
                "type": "string",
                "description": "The name of the lockable object set, as the Archicad API documents it: BuildingMaterials, Cities, Composites, Favorites, FillTypes, LayerSettingsDialog (layers and layer combinations), LineTypes, MarkupStyles, MEPSystems, ModelViewOptions, OperationProfiles, PenTables, Profiles, ProjectInfo, GeoLocation (project location), PreferencesDialog (project preferences and dimension standards), GraphicOverrides (renovation override styles), Surfaces or ZoneCategories. Other names the Archicad API accepts work too."
            }
        },
        "additionalProperties": false,
        "required": [
            "objectSetName"
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
    }
            },{
                "name": "GetLockableObjectSetStatus",
                "version": "1.7.1",
                "description": "Returns the Teamwork reservation status of a non-element lockable object set and the users who hold it.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "objectSetName": {
                "type": "string",
                "description": "The name of the lockable object set, as the Archicad API documents it: BuildingMaterials, Cities, Composites, Favorites, FillTypes, LayerSettingsDialog (layers and layer combinations), LineTypes, MarkupStyles, MEPSystems, ModelViewOptions, OperationProfiles, PenTables, Profiles, ProjectInfo, GeoLocation (project location), PreferencesDialog (project preferences and dimension standards), GraphicOverrides (renovation override styles), Surfaces or ZoneCategories. Other names the Archicad API accepts work too."
            }
        },
        "additionalProperties": false,
        "required": [
            "objectSetName"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "status": {
                "type": "string",
                "description": "Free: nobody has reserved it. Editable: the current user has reserved it. Locked: another user has reserved it (see conflicts). NotAvailable: the Teamwork server is offline or not available. NotExist: there is no Teamwork connection, for example the project is not a Teamwork project. An unknown object set name is reported as an error, not as a status.",
                "enum": ["Free", "Editable", "Locked", "NotAvailable", "NotExist"]
            },
            "conflicts": {
                "type": "array",
                "description": "The users who hold a reservation on the object set.",
                "items": {
                    "$ref": "#/User"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "status"
        ]
    }
            },{
                "name": "ReserveHotlinkCacheManagement",
                "version": "1.7.1",
                "description": "Reserves the Hotlink and XRef management in Teamwork mode, which is needed before changing hotlink nodes.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "executionResult": {
                "$ref": "#/ExecutionResult"
            },
            "conflicts": {
                "type": "array",
                "description": "The user whose reservation prevented reserving the Hotlink and XRef management.",
                "items": {
                    "$ref": "#/User"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "executionResult"
        ]
    }
            },{
                "name": "ReleaseHotlinkCacheManagement",
                "version": "1.7.1",
                "description": "Releases the Hotlink and XRef management in Teamwork mode.",
                "inputScheme": null,
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
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/DatabaseIdsOrErrors"}},"additionalProperties":false,"required":["databases"]}
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
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/DatabaseIdsOrErrors"}},"additionalProperties":false,"required":["databases"]}
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
                        "paperWidth":            { "type": "number", "description": "Optional paper width in mm, used together with paperHeight. Sets the size of the new layout, and of the master layout when this command creates it from masterLayoutName: a new master starts with the template's default size, which layoutParameters cannot change. An existing master keeps its size. Wins over layoutParameters.horizontalSize." },
                        "paperHeight":           { "type": "number", "description": "Optional paper height in mm, used together with paperWidth. See paperWidth. Wins over layoutParameters.verticalSize." },
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
                "outputScheme": {"type":"object","properties":{"databases":{"$ref":"#/DatabaseIdsOrErrors"}},"additionalProperties":false,"required":["databases"]}
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
                        "name": {
                            "type": "string",
                            "minLength": 1,
                            "description": "Custom title name of the new Drawing. Giving a name implies nameType CustomName unless nameType is set explicitly."
                        },
                        "nameType": {
                            "$ref": "#/DrawingNameType",
                            "description": "Defaults to CustomName when name is given, otherwise to the Drawing tool's current default."
                        },
                        "position": { "$ref": "#/Coordinate2D" },
                        "scale": {
                            "type": "number",
                            "exclusiveMinimum": 0.0,
                            "description": "Scale ratio applied to the drawing relative to its source view (API_DrawingType::ratio). Defaults to 1.0."
                        },
                        "angle": {
                            "type": "number",
                            "description": "Rotation angle of the drawing in radians. Defaults to the Drawing tool's current default."
                        },
                        "drawingScale": {
                            "type": "number",
                            "description": "The nominal scale of the drawing. Defaults to the Drawing tool's current default."
                        },
                        "modelOffset": {
                            "$ref": "#/Coordinate2D",
                            "description": "Offset of the model origin within the drawing. Defaults to the Drawing tool's current default."
                        },
                        "clipPolygon": { "type": "array", "items": { "$ref": "#/Coordinate2D" }, "minItems": 3 }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "position"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsData"]
    },
                "outputScheme": {"type":"object","properties":{"elements":{"$ref":"#/ElementIdsOrErrors"}},"additionalProperties":false,"required":["elements"]}
            },{
                "name": "ChangeDrawingLink",
                "version": "1.5.7",
                "description": "Relinks a Drawing to a different source navigator item. Archicad has no in-place relink API, so this recreates the Drawing against the new source and deletes the original - the returned elementId is a NEW guid, not the input one. The Drawing Title marker's own position is not preserved (undocumented Archicad limitation).",
                "inputScheme": {
        "type": "object",
        "properties": {
            "drawingsWithNewLinks": {
                "type": "array",
                "items": {
                    "type": "object",
                    "description": "An existing Drawing and the navigator item it should be relinked to.",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "navigatorItemId": { "$ref": "#/NavigatorItemId" },
                        "layoutDatabaseId": { "$ref": "#/DatabaseId" }
                    },
                    "additionalProperties": false,
                    "required": ["elementId", "navigatorItemId", "layoutDatabaseId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["drawingsWithNewLinks"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "One result per input item. On success, elementId is the NEW Drawing's identifier - relinking necessarily replaces the element, it cannot keep the original guid.",
                "items": {
                    "$ref": "#/ElementIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
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
                                "$ref": "#/LayoutCustomData"
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
                                "$ref": "#/LayoutCustomDataToSet"
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
                "name": "GetActiveGraphicalOverrideCombination",
                "version": "1.7.2",
                "description": "Gets the graphic override combination applied to the current window, with its id and name. The response is empty when no combination is applied. This is the combination of the window itself, not the one stored in a saved view (see GetViewSettings). Available from Archicad 30.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "description": "The graphic override combination applied to the current window. Empty when the current window has no combination applied.",
        "properties": {
            "combinationId": {
                "type": "object",
                "description": "The identifier of the graphic override combination.",
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
            "name": {
                "type": "string",
                "description": "The name of the graphic override combination. Missing when the combination cannot be read, e.g. it was deleted."
            }
        },
        "additionalProperties": false
    }
            },{
                "name": "SetActiveGraphicalOverrideCombination",
                "version": "1.7.2",
                "description": "Applies a graphic override combination, given by id or by name, to the current window. The current window must be a model window (floor plan, section, elevation, 3D, ...). To remove the overrides, apply the project's 'No Overrides' combination. The saved view is not changed (see SetViewSettings for that). Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "description": "The graphic override combination to apply, given by id or by name.",
        "properties": {
            "combinationId": {
                "type": "object",
                "description": "The identifier of the graphic override combination, as returned by GetActiveGraphicalOverrideCombination.",
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
            "name": {
                "type": "string",
                "description": "The name of the graphic override combination, as shown in Document > Graphic Overrides > Graphic Override Combinations."
            }
        },
        "additionalProperties": false,
        "oneOf": [
            {
                "required": [
                    "combinationId"
                ]
            },
            {
                "required": [
                    "name"
                ]
            }
        ]
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
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
                "description": "Clones Project Map viewpoints into a specified View Map folder.",
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
                            "description": "Navigator item ID of the View Map folder to place the clone in. Required - Archicad's CloneProjectMapItemToViewMap API leaves a clone created directly at the View Map root in a broken state (it cannot be deleted or moved afterwards, by API or by hand), so Tapir refuses to fall back to the root on your behalf. Create a View Map folder (CreateViewMapFolder) first if you don't already have one to target."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "parentNavigatorItemId"]
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
                "description": "Creates independent (non-clone) navigator views in a specified View Map folder, by copying database and settings from source items.",
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
                            "description": "View Map folder to place the new view in. Required - the underlying Archicad API used to create the item leaves it undeletable and unmovable afterwards when it is attached directly to the View Map root, so Tapir refuses to fall back to the root on your behalf. Create a View Map folder (CreateViewMapFolder) first if you don't already have one to target."
                        },
                        "name": {
                            "type": "string",
                            "description": "Name for the new view. Optional; defaults to the source item name."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["navigatorItemId", "parentNavigatorItemId"]
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
                "name": "Get3DProjectionSettings",
                "version": "1.7.1",
                "description": "Gets the 3D projection settings of the 3D window: whether it is perspective or axonometric, the camera and target position, view cone, roll angle, distance and azimuth of a perspective, or the transformation matrix of an axonometry.",
                "inputScheme": null,
                "outputScheme": {
    "type": "object",
    "properties": {
        "isPersp": {
            "type": "boolean",
            "description": "True for a perspective projection, false for an axonometric one."
        },
        "cameraElementId": {
            "$ref": "#/ElementId",
            "description": "The Camera element the projection was set from. Missing when the projection does not come from a camera."
        },
        "activeCameraSetElementId": {
            "$ref": "#/ElementId",
            "description": "The active Camera Set element. Missing when there is none."
        },
        "perspective": {
            "$ref": "#/PerspectiveProjectionParameters",
            "description": "The perspective parameters. Present only when isPersp is true."
        },
        "axonometric": {
            "type": "object",
            "description": "The parameters of an axonometric projection. Present only when isPersp is false.",
            "properties": {
                "transformationMatrix": {
                    "type": "array",
                    "description": "The 3x4 transformation matrix of the axonometric projection, row by row (12 numbers).",
                    "items": {
                        "type": "number"
                    },
                    "minItems": 12,
                    "maxItems": 12
                }
            },
            "additionalProperties": false,
            "required": [
                "transformationMatrix"
            ]
        }
    },
    "additionalProperties": false,
    "required": [
        "isPersp"
    ]
}
            },{
                "name": "Set3DProjectionSettings",
                "version": "1.7.1",
                "description": "Sets a perspective projection for the 3D window. Every perspective parameter is required: read a known good set with Get3DProjectionSettings and pass it back, with the values to change replaced.",
                "inputScheme": {
    "type": "object",
    "properties": {
        "perspective": {
            "$ref": "#/PerspectiveProjectionParameters"
        }
    },
    "additionalProperties": false,
    "required": [
        "perspective"
    ]
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    }
            },{
                "name": "CreateInteriorElevations",
                "version": "1.5.8",
                "description": "Creates Interior Elevation elements on the floor plan. Every consecutive pair of the given points becomes one segment, each with its own viewpoint - feed it the corner points of a room to get an interior elevation of that room.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "interiorElevationsData": {
                "type": "array",
                "description": "Array of data to create Interior Elevation elements.",
                "items": {
                    "$ref": "#/InteriorElevationData"
                }
            }
        },
        "additionalProperties": false,
        "required": ["interiorElevationsData"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
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
                "description": "Renames a navigator item or changes its ID. Fails when Archicad does not apply the new name (e.g. on Project Map schedule items, which are named by their scheme). With customName false the item drops its custom name and follows its Project Map source again.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemId": { "$ref": "#/NavigatorItemId" },
            "newName":         { "type": "string" },
            "newId":           { "type": "string" },
            "customName": {
                "type": "boolean",
                "description": "Set to false to drop the custom name of the item, so that it follows the name of its Project Map source again. Cannot be combined with newName."
            }
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
            },
            "publisherSetName": {
                "type": "string",
                "minLength": 1,
                "description": "The name of the publisher set to retrieve. Used only when navigatorMapId is PublisherSets. Without it the first publisher set is returned."
            }
        },
        "additionalProperties": false,
        "required": ["navigatorMapId"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "navigatorItemTree": {
                "$ref": "#/NavigatorItem"
            }
        },
        "additionalProperties": false,
        "required": ["navigatorItemTree"]
    }
            }]
        },{
            "name": "Graphic Override Commands",
            "commands": [{
                "name": "CreateGraphicalOverrideRule",
                "version": "1.7.2",
                "description": "Creates or overwrites a graphic override rule in a rule group: name, criterion XML, surface color and line pen. Needs Archicad 27 or later.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name of the override rule. Rule names are unique in the project: an existing rule with this name is overwritten, in whichever rule group it is."
            },
            "ruleGroupName": {
                "type": "string",
                "description": "Rule group of a new rule, created when it does not exist. Default: 'Tapir'. An overwritten rule stays in its own group."
            },
            "criterionXML": {
                "type": "string",
                "description": "Optional criterion XML. Archicad only takes XML in its own format: read it from a rule made in the GUI with GetGraphicalOverrideRules and change the values. XML written from scratch is dropped without an error, and so is an empty one: the rule then applies to all elements."
            },
            "surfaceRGB": {
                "type": "object",
                "description": "Optional color override of the cut and uncut surfaces. Components are in the 0..1 range.",
                "properties": {
                    "red": { "type": "number" },
                    "green": { "type": "number" },
                    "blue": { "type": "number" }
                },
                "required": ["red", "green", "blue"],
                "additionalProperties": false
            },
            "linePenIndex": {
                "type": "integer",
                "description": "Optional pen index override for lines, markers and texts. Only a positive index takes effect."
            },
            "overrideContours": {
                "type": "boolean",
                "description": "Optional. If true, the contours of the overridden surfaces are recolored too."
            }
        },
        "additionalProperties": false,
        "required": ["name"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "ruleId": {
                "$ref": "#/GuidId",
                "description": "The created or overwritten override rule."
            }
        },
        "additionalProperties": false,
        "required": ["ruleId"]
    }
            },{
                "name": "GetGraphicalOverrideRules",
                "version": "1.7.2",
                "description": "Returns every graphic override rule with its criterion XML. Needs Archicad 27 or later.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "rules": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "ruleId": { "$ref": "#/GuidId" },
                        "name": { "type": "string" },
                        "ruleGroupName": { "type": "string", "description": "Name of the rule group the rule belongs to." },
                        "criterionXML": { "type": "string", "description": "The criterion in Archicad's own XML format. Change its values and pass it to CreateGraphicalOverrideRule to make similar rules." }
                    },
                    "required": ["ruleId", "name", "ruleGroupName", "criterionXML"],
                    "additionalProperties": false
                }
            }
        },
        "additionalProperties": false,
        "required": ["rules"]
    }
            },{
                "name": "CreateGraphicalOverrideCombination",
                "version": "1.7.2",
                "description": "Creates a graphic override combination from rule names, or replaces the rules of an existing one. Needs Archicad 27 or later.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "Name of the override combination. When a combination with this name exists, its rule list is replaced."
            },
            "ruleNames": {
                "type": "array",
                "description": "Names of the override rules in the combination, in order. Rule names are unique in the project, whatever the rule group. An empty list makes a combination without rules.",
                "items": { "type": "string" }
            }
        },
        "additionalProperties": false,
        "required": ["name", "ruleNames"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "combinationId": {
                "$ref": "#/GuidId",
                "description": "The created or changed override combination."
            }
        },
        "additionalProperties": false,
        "required": ["combinationId"]
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
                    "$ref": "#/DesignOptionCombinationDetails"
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
                    "$ref": "#/DesignOptionIdArrayItem"
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
                    "$ref": "#/ElementsOfDesignOptionOrError"
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
                            "$ref": "#/ElementId"
                        },
                        "type": {
                            "$ref": "#/DesignOptionLinkType"
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
                    "$ref": "#/DesignOptionData"
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
                    "$ref": "#/DesignOptionCombinationData"
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
                        "designOptionId",
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
            },{
                "name": "RenameDesignOptions",
                "version": "1.7.1",
                "description": "Changes the name and/or the string id of the given design options. Available from Archicad 29.",
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
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option. The name is kept when it is not given."
                        },
                        "id": {
                            "type": "string",
                            "description": "The new string id of the design option. The id is kept when it is not given."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionId"
                    ]
                },
                "description": "The design options to rename, each with the new name and/or id. At least one of them must be given."
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
                "name": "RenameDesignOptionSets",
                "version": "1.7.1",
                "description": "Changes the name of the given design option sets. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionSetId": {
                            "$ref": "#/DesignOptionSetId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option set."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionSetId",
                        "name"
                    ]
                },
                "description": "The design option sets to rename, each with the new name."
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
                "name": "RenameDesignOptionCombinations",
                "version": "1.7.1",
                "description": "Changes the name of the given design option combinations. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "designOptionCombinationId": {
                            "$ref": "#/DesignOptionCombinationId"
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the design option combination."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "designOptionCombinationId",
                        "name"
                    ]
                },
                "description": "The design option combinations to rename, each with the new name."
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
                "name": "DeleteDesignOptions",
                "version": "1.7.1",
                "description": "Deletes the given design options. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptions": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionIdArrayItem"
                },
                "description": "The design options to delete."
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
                "name": "DeleteDesignOptionSets",
                "version": "1.7.1",
                "description": "Deletes the given design option sets. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionSets": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionSetIdArrayItem"
                },
                "description": "The design option sets to delete."
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
                "name": "DeleteDesignOptionCombinations",
                "version": "1.7.1",
                "description": "Deletes the given design option combinations. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "designOptionCombinations": {
                "type": "array",
                "items": {
                    "$ref": "#/DesignOptionCombinationIdArrayItem"
                },
                "description": "The design option combinations to delete."
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
            "name": "Keynote Commands",
            "commands": [{
                "name": "GetKeynoteTree",
                "version": "1.5.6",
                "description": "Retrieves the whole keynote folder and item hierarchy. The technical root folder is not included in the output; the top-level folders and items are returned directly. Available from Archicad 28.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "foldersInRoot": {
                "type": "array",
                "description": "The top-level keynote folders with their content recursively. The technical root folder itself is not included.",
                "items": {
                    "$ref": "#/KeynoteFolderDetails"
                }
            },
            "itemsInRoot": {
                "type": "array",
                "description": "The keynote items located directly in the technical root folder.",
                "items": {
                    "$ref": "#/KeynoteItemDetails"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "foldersInRoot",
            "itemsInRoot"
        ]
    }
            },{
                "name": "GetKeynoteAutoTexts",
                "version": "1.5.6",
                "description": "Retrieves the autotext tokens of the given keynote items. The tokens can be used as label text content to reference the fields of a keynote item. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "keynoteItems": {
                "type": "array",
                "description": "The keynote items to get the autotext tokens for.",
                "items": {
                    "$ref": "#/KeynoteItemIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItems"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "autoTexts": {
                "$ref": "#/KeynoteAutoTextTokensOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "autoTexts"
        ]
    }
            },{
                "name": "CreateKeynoteFolders",
                "version": "1.5.6",
                "description": "Creates keynote folders under the given parent folders (or under the root folder). Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "foldersData": {
                "type": "array",
                "description": "Array of data to create keynote folders.",
                "items": {
                    "type": "object",
                    "properties": {
                        "parentFolderId": {
                            "$ref": "#/KeynoteFolderId",
                            "description": "The parent folder. Optional; defaults to the root folder."
                        },
                        "key": {
                            "type": "string",
                            "minLength": 1
                        },
                        "title": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "key",
                        "title"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "foldersData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "keynoteFolderIdsOrErrors": {
                "$ref": "#/KeynoteFolderIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteFolderIdsOrErrors"
        ]
    }
            },{
                "name": "CreateKeynoteItems",
                "version": "1.5.6",
                "description": "Creates keynote items in the given parent folders (or in the root folder). Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "itemsData": {
                "type": "array",
                "description": "Array of data to create keynote items.",
                "items": {
                    "type": "object",
                    "properties": {
                        "parentFolderId": {
                            "$ref": "#/KeynoteFolderId",
                            "description": "The parent folder. Optional; defaults to the root folder."
                        },
                        "key": {
                            "type": "string",
                            "minLength": 1
                        },
                        "title": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "key"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "itemsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "keynoteItemIdsOrErrors": {
                "$ref": "#/KeynoteItemIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItemIdsOrErrors"
        ]
    }
            },{
                "name": "ModifyKeynoteFolders",
                "version": "1.5.6",
                "description": "Modifies the key, title or reference of the given keynote folders. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "foldersData": {
                "type": "array",
                "description": "Array of data to modify keynote folders. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteFolderId": {
                            "$ref": "#/KeynoteFolderId"
                        },
                        "key": {
                            "type": "string",
                            "minLength": 1
                        },
                        "title": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteFolderId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "foldersData"
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
                "name": "ModifyKeynoteItems",
                "version": "1.5.6",
                "description": "Modifies the key, title, description or reference of the given keynote items. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "itemsData": {
                "type": "array",
                "description": "Array of data to modify keynote items. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteItemId": {
                            "$ref": "#/KeynoteItemId"
                        },
                        "key": {
                            "type": "string",
                            "minLength": 1
                        },
                        "title": {
                            "type": "string"
                        },
                        "description": {
                            "type": "string"
                        },
                        "reference": {
                            "type": "string"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteItemId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "itemsData"
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
                "name": "DeleteKeynoteFolders",
                "version": "1.5.6",
                "description": "Deletes the given keynote folders including their content. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "keynoteFolderIds": {
                "type": "array",
                "description": "The keynote folders to delete.",
                "items": {
                    "$ref": "#/KeynoteFolderIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteFolderIds"
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
                "name": "DeleteKeynoteItems",
                "version": "1.5.6",
                "description": "Deletes the given keynote items. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "keynoteItemIds": {
                "type": "array",
                "description": "The keynote items to delete.",
                "items": {
                    "$ref": "#/KeynoteItemIdArrayItem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "keynoteItemIds"
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
                "name": "CreateKeynoteLabels",
                "version": "1.5.6",
                "description": "Creates Label elements that reference the given keynote items via autotext. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "labelsData": {
                "type": "array",
                "description": "Array of data to create keynote labels.",
                "items": {
                    "type": "object",
                    "properties": {
                        "keynoteItemId": {
                            "$ref": "#/KeynoteItemId"
                        },
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "The reference point of the label."
                        },
                        "contentFields": {
                            "type": "array",
                            "description": "The keynote fields to include in the label text as autotext. Optional; defaults to all fields.",
                            "items": {
                                "type": "string",
                                "enum": ["Key", "Title", "Description", "Reference"]
                            },
                            "minItems": 1
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "keynoteItemId",
                        "position"
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
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            }]
        },{
            "name": "MEP Commands",
            "commands": [{
                "name": "GetMEPElements",
                "version": "1.5.6",
                "description": "Retrieves the MEP (Mechanical, Electrical, Plumbing) elements of the project, optionally filtered by type and domain. MEP elements are ordinary elements, so the generic element commands work on them as well (for example they can be deleted with the DeleteElements command). Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementTypes": {
                "type": "array",
                "description": "Optional filter for the MEP element types.",
                "items": {
                    "$ref": "#/MEPElementType"
                }
            },
            "domains": {
                "type": "array",
                "description": "Optional filter for the MEP domains.",
                "items": {
                    "$ref": "#/MEPDomain"
                }
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "The MEP elements.",
                "items": {
                    "$ref": "#/MEPElement"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "GetMEPRoutingElements",
                "version": "1.5.6",
                "description": "Retrieves the details of the given MEP routing elements: domain, MEP system, route polyline, segments with cross section data (shape, size, preference table and reference id) and nodes. Available from Archicad 28.",
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
            "routingElements": {
                "$ref": "#/MEPRoutingElementDetailsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "routingElements"
        ]
    }
            },{
                "name": "GetMEPPorts",
                "version": "1.5.6",
                "description": "Retrieves the ports of the given MEP elements including position, shape, size and connection status. Available from Archicad 28.",
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
            "elementPorts": {
                "$ref": "#/MEPElementPortsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPorts"
        ]
    }
            },{
                "name": "GetMEPDistributionSystems",
                "version": "1.5.6",
                "description": "Retrieves the MEP distribution systems of the project with their domain, MEP system attribute and member elements. Available from Archicad 28.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "distributionSystems": {
                "type": "array",
                "description": "The distribution systems of the project.",
                "items": {
                    "$ref": "#/MEPDistributionSystem"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "distributionSystems"
        ]
    }
            },{
                "name": "CreateMEPRoutingElements",
                "version": "1.5.6",
                "description": "Creates MEP routing elements (duct, pipe or cable carrier routes) along the given polylines with optional cross section data (shape, size, preference table and reference id), MEP system and layer (also set on the segments, elbows and transitions). Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "routingElementsData": {
                "type": "array",
                "description": "Array of data to create MEP routing elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "domain": {
                            "$ref": "#/MEPDomain"
                        },
                        "nodeCoordinates": {
                            "type": "array",
                            "description": "The corner points of the route polyline.",
                            "items": {
                                "$ref": "#/Coordinate3D"
                            },
                            "minItems": 2
                        },
                        "crossSectionWidth": {
                            "type": "number",
                            "description": "Optional cross section width applied to all segments."
                        },
                        "crossSectionHeight": {
                            "type": "number",
                            "description": "Optional cross section height applied to all segments."
                        },
                        "crossSectionShape": {
                            "$ref": "#/MEPCrossSectionShape",
                            "description": "Optional; applied to all segments."
                        },
                        "crossSectionReferenceId": {
                            "type": "integer",
                            "description": "Optional cross section reference id of the segment preference table (used for circular cross sections). Resolved against preferenceTableId if given, otherwise against the table of the current routing tool default."
                        },
                        "preferenceTableId": {
                            "$ref": "#/Guid",
                            "description": "Optional guid of the segment preference table (see GetMEPPreferenceTables) applied to all segments. Not applicable to the CableCarrier domain."
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId",
                            "description": "Optional MEP system attribute."
                        },
                        "layerIndex": {
                            "type": "integer",
                            "description": "Optional layer attribute index of the routing element and all its parts (routing segments, rigid segments, elbows and transitions). Only a positive index takes effect; otherwise the routing tool default layer is used. A layer that does not exist or is locked gives an error for the item, and nothing is placed."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "domain",
                        "nodeCoordinates"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "routingElementsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateMEPElements",
                "version": "1.5.6",
                "description": "Creates MEP elements (Terminal, Accessory, Equipment or Fitting) at the given positions. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementsData": {
                "type": "array",
                "description": "Array of data to create MEP elements.",
                "items": {
                    "type": "object",
                    "properties": {
                        "type": {
                            "$ref": "#/MEPComponentType"
                        },
                        "domain": {
                            "$ref": "#/MEPDomain",
                            "description": "Required for all types except Equipment."
                        },
                        "position": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientationDirection": {
                            "$ref": "#/Coordinate3D",
                            "description": "Optional direction vector of the orientation. Defaults to (1, 0, 0)."
                        },
                        "orientationRotation": {
                            "$ref": "#/Coordinate3D",
                            "description": "Optional rotation vector of the orientation. Defaults to (0, 1, 0)."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "type",
                        "position"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "ModifyMEPRoutingElements",
                "version": "1.5.6",
                "description": "Modifies the given MEP routing elements: MEP system, cross section data (shape, size, preference table and reference id) of all segments and node positions. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "routingElementsData": {
                "type": "array",
                "description": "Array of data to modify MEP routing elements. Only provided fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId"
                        },
                        "crossSectionWidth": {
                            "type": "number",
                            "description": "New cross section width applied to all segments."
                        },
                        "crossSectionHeight": {
                            "type": "number",
                            "description": "New cross section height applied to all segments."
                        },
                        "crossSectionShape": {
                            "$ref": "#/MEPCrossSectionShape",
                            "description": "The new shape is applied to all segments."
                        },
                        "crossSectionReferenceId": {
                            "type": "integer",
                            "description": "New cross section reference id of the segment preference table applied to all segments (used for circular cross sections). Resolved against preferenceTableId if given, otherwise against the table the segments already use."
                        },
                        "preferenceTableId": {
                            "$ref": "#/Guid",
                            "description": "New segment preference table (guid as returned by GetMEPPreferenceTables) applied to all segments. Not applicable to the CableCarrier domain."
                        },
                        "nodePositions": {
                            "type": "array",
                            "description": "New positions of the routing nodes. The size must match the number of nodes of the route.",
                            "items": {
                                "$ref": "#/Coordinate3D"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "routingElementsData"
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
                "name": "ConnectMEPElements",
                "version": "1.5.6",
                "description": "Connects MEP routing elements to other MEP elements or routes. Merges routes, splits routes or creates branch elements as needed. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "connectionsData": {
                "type": "array",
                "description": "Array of connections to create.",
                "items": {
                    "type": "object",
                    "properties": {
                        "routingElementId": {
                            "$ref": "#/ElementId",
                            "description": "The routing element to connect."
                        },
                        "connectToId": {
                            "$ref": "#/ElementId",
                            "description": "The MEP element or routing element to connect to."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "routingElementId",
                        "connectToId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "connectionResults": {
                "$ref": "#/MEPConnectionResultsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionResults"
        ]
    }
            },{
                "name": "GetMEPPreferenceTables",
                "version": "1.5.7",
                "description": "Gets the circular cross section preference tables (referenceId, diameter, description) and the reference set of the Piping or Ventilation domain, optionally with the elbow, branch and transition preference tables. The angle set and the transition tables are available from Archicad 29. Available from Archicad 28.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "domain": {
                "$ref": "#/MEPPreferenceTableDomain"
            },
            "includeFittingTables": {
                "type": "boolean",
                "description": "Also return the elbow, branch and transition preference tables. Defaults to false."
            }
        },
        "additionalProperties": false,
        "required": ["domain"]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "tables": {
                "type": "array",
                "description": "The circular segment preference tables of the domain.",
                "items": {
                    "$ref": "#/MEPPreferenceTable"
                }
            },
            "referenceSet": {
                "$ref": "#/MEPReferenceSet"
            },
            "angleSet": {
                "$ref": "#/MEPAngleSet",
                "description": "Available from Archicad 29."
            },
            "elbowTables": {
                "type": "array",
                "description": "The elbow preference tables of the domain. Only when includeFittingTables is true.",
                "items": {
                    "$ref": "#/MEPElbowPreferenceTable"
                }
            },
            "branchTables": {
                "type": "array",
                "description": "The branch preference tables of the domain. Only when includeFittingTables is true.",
                "items": {
                    "$ref": "#/MEPBranchPreferenceTable"
                }
            },
            "transitionTables": {
                "type": "array",
                "description": "The transition preference tables of the domain. Only when includeFittingTables is true, available from Archicad 29.",
                "items": {
                    "$ref": "#/MEPTransitionPreferenceTable"
                }
            }
        },
        "additionalProperties": false,
        "required": ["tables", "referenceSet"]
    }
            },{
                "name": "GetMEPSystemGroups",
                "version": "1.7.2",
                "description": "Gets the MEP system groups of the project with their physical systems: name, root element and MEP system. Available from Archicad 29.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "systemGroups": {
                "type": "array",
                "description": "The system groups of the project with their physical systems.",
                "items": {
                    "$ref": "#/MEPSystemGroupDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroups"
        ]
    }
            },{
                "name": "CreateMEPSystemGroups",
                "version": "1.7.2",
                "description": "Creates MEP system groups. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "systemGroupsData": {
                "type": "array",
                "description": "Array of data to create system groups.",
                "items": {
                    "type": "object",
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
            "systemGroupsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "systemGroups": {
                "type": "array",
                "items": {
                    "$ref": "#/MEPSystemGroupIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "systemGroups"
        ]
    }
            },{
                "name": "CreateMEPPhysicalSystems",
                "version": "1.7.2",
                "description": "Creates physical systems in MEP system groups, optionally with a root element and an MEP system as their category. Available from Archicad 29.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "physicalSystemsData": {
                "type": "array",
                "description": "Array of data to create physical systems.",
                "items": {
                    "type": "object",
                    "properties": {
                        "systemGroupId": {
                            "$ref": "#/MEPSystemGroupId",
                            "description": "The system group the new physical system belongs to."
                        },
                        "name": {
                            "type": "string"
                        },
                        "rootElementId": {
                            "$ref": "#/ElementId",
                            "description": "The MEP element the system starts from. Archicad keeps it only while it is part of an MEP network: the root of a stand-alone element is cleared when the MEP networks are recalculated."
                        },
                        "mepSystemId": {
                            "$ref": "#/AttributeId",
                            "description": "The MEP system attribute used as the category of the physical system."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "systemGroupId",
                        "name"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystemsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "physicalSystems": {
                "type": "array",
                "items": {
                    "$ref": "#/MEPPhysicalSystemIdOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "physicalSystems"
        ]
    }
            },{
                "name": "GetElectricalPanels",
                "version": "1.7.2",
                "description": "Gets the electrical panels (distribution boards) of the project or the given ones: name, circuit naming, circuits, parent circuit, placement and the connected and demand loads in total and per phase. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical panels. Every electrical panel of the project is returned when it is not given."
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "electricalPanels": {
                "type": "array",
                "description": "The electrical panels, in the order of the given elements.",
                "items": {
                    "$ref": "#/ElectricalPanelDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalPanels"
        ]
    }
            },{
                "name": "GetElectricalCircuits",
                "version": "1.7.2",
                "description": "Gets the given electrical circuits, the circuits of the given panels, or every circuit of the project: name, description, owner panel, connected devices and panels, voltage, phases, wiring type, loads, voltage drop and estimated cable length. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical circuits."
            },
            "electricalPanels": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical panels whose circuits are returned. When neither elements nor electricalPanels is given, every circuit of every electrical panel of the project is returned."
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "electricalCircuits": {
                "type": "array",
                "description": "The given circuits first, in their order, then the circuits of the given panels, in the order of the panels.",
                "items": {
                    "$ref": "#/ElectricalCircuitDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuits"
        ]
    }
            },{
                "name": "GetElectricalDevices",
                "version": "1.7.2",
                "description": "Gets the electrical devices (lights, sockets and other consumers) of the project or the given ones: parent circuit, placement, apparent power, load classification, demand load and directly connected MEP elements. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "Optional list of electrical devices. Every electrical device of the project is returned when it is not given."
            }
        },
        "additionalProperties": false,
        "required": []
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "electricalDevices": {
                "type": "array",
                "description": "The electrical devices, in the order of the given elements.",
                "items": {
                    "$ref": "#/ElectricalDeviceDetailsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalDevices"
        ]
    }
            },{
                "name": "GetElectricalProjectSettings",
                "version": "1.7.2",
                "description": "Gets the electrical project settings: the default voltage, the wiring types with their cross-sectional area and the load classifications with their demand factor. Available from Archicad 30.",
                "inputScheme": null,
                "outputScheme": {
        "type": "object",
        "properties": {
            "voltage": {
                "type": "number",
                "description": "The default voltage of the project."
            },
            "wiringTypes": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "crossSectionalArea": {
                            "type": "number",
                            "description": "The cross-sectional area of the wire in square millimeters."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "crossSectionalArea"
                    ]
                }
            },
            "loadClassifications": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "demandFactor": {
                            "type": "number",
                            "description": "The demand factor, between 0 and 1."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "demandFactor"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "voltage",
            "wiringTypes",
            "loadClassifications"
        ]
    }
            },{
                "name": "CreateElectricalPanels",
                "version": "1.7.2",
                "description": "Places electrical panels with the default settings of the Electrical Panel tool, optionally with the given library part, size, rotation, mirroring, panel name and circuit naming. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalPanelsData": {
                "type": "array",
                "description": "Array of data to create electrical panels. The fields that are not given are taken from the default settings of the Electrical Panel tool.",
                "items": {
                    "type": "object",
                    "properties": {
                        "position": {
                            "$ref": "#/Coordinate3D",
                            "description": "The anchor point of the new panel."
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation",
                            "description": "Defaults to the direction (1, 0, 0) and the rotation (0, 1, 0)."
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        },
                        "panelName": {
                            "type": "string"
                        },
                        "circuitNamePrefix": {
                            "type": "string"
                        },
                        "circuitIdStyle": {
                            "type": "integer"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "position"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalPanelsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "CreateElectricalDevices",
                "version": "1.7.2",
                "description": "Places electrical devices with the default settings of the Electrical Device tool, optionally with the given library part, size, rotation, mirroring, apparent power and load classification, and connects them to a circuit. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalDevicesData": {
                "type": "array",
                "description": "Array of data to create electrical devices. The fields that are not given are taken from the default settings of the Electrical Device tool.",
                "items": {
                    "type": "object",
                    "properties": {
                        "position": {
                            "$ref": "#/Coordinate3D",
                            "description": "The anchor point of the new device."
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation",
                            "description": "Defaults to the direction (1, 0, 0) and the rotation (0, 1, 0)."
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        },
                        "apparentPower": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "The apparent power in VA."
                        },
                        "isApparentPowerEnabled": {
                            "type": "boolean"
                        },
                        "loadClassificationName": {
                            "type": "string",
                            "description": "One of the load classifications of the electrical project settings, or an empty string for none."
                        },
                        "circuitId": {
                            "$ref": "#/ElementId",
                            "description": "The circuit to connect the new device to."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "position"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalDevicesData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "ModifyElectricalPanels",
                "version": "1.7.2",
                "description": "Modifies the name, circuit naming and placement (anchor point, orientation, rotation, mirroring, library part, size) of electrical panels. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalPanelsData": {
                "type": "array",
                "description": "Array of data to modify electrical panels. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "panelName": {
                            "type": "string"
                        },
                        "circuitNamePrefix": {
                            "type": "string"
                        },
                        "circuitIdStyle": {
                            "type": "integer"
                        },
                        "anchorPoint": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalPanelsData"
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
                "name": "CreateElectricalCircuits",
                "version": "1.7.2",
                "description": "Adds new circuits to electrical panels, optionally with the given description, voltage, wiring type and used phases. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalCircuitsData": {
                "type": "array",
                "description": "Array of data to create electrical circuits. Each item adds one circuit to the given panel.",
                "items": {
                    "type": "object",
                    "properties": {
                        "electricalPanelId": {
                            "$ref": "#/ElementId",
                            "description": "The panel to add the new circuit to."
                        },
                        "description": {
                            "type": "string"
                        },
                        "voltage": {
                            "type": "number"
                        },
                        "wiringTypeName": {
                            "type": "string",
                            "description": "One of the wiring types of the electrical project settings, or an empty string for none."
                        },
                        "isPhaseAUsed": {
                            "type": "boolean"
                        },
                        "isPhaseBUsed": {
                            "type": "boolean"
                        },
                        "isPhaseCUsed": {
                            "type": "boolean"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "electricalPanelId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuitsData"
        ]
    },
                "outputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/ElementIdsOrErrors"
            }
        },
        "additionalProperties": false,
        "required": [
            "elements"
        ]
    }
            },{
                "name": "DeleteElectricalCircuits",
                "version": "1.7.2",
                "description": "Removes electrical circuits from their panels. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalCircuits": {
                "$ref": "#/Elements",
                "description": "The circuits to remove from their panels."
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuits"
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
                "name": "ModifyElectricalCircuits",
                "version": "1.7.2",
                "description": "Modifies the description, voltage, wiring type and used phases of electrical circuits. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalCircuitsData": {
                "type": "array",
                "description": "Array of data to modify electrical circuits. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "description": {
                            "type": "string"
                        },
                        "voltage": {
                            "type": "number"
                        },
                        "wiringTypeName": {
                            "type": "string",
                            "description": "One of the wiring types of the electrical project settings, or an empty string for none."
                        },
                        "isPhaseAUsed": {
                            "type": "boolean"
                        },
                        "isPhaseBUsed": {
                            "type": "boolean"
                        },
                        "isPhaseCUsed": {
                            "type": "boolean"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalCircuitsData"
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
                "name": "ModifyElectricalDevices",
                "version": "1.7.2",
                "description": "Modifies the apparent power, load classification and placement (anchor point, orientation, rotation, mirroring, library part, size) of electrical devices. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "electricalDevicesData": {
                "type": "array",
                "description": "Array of data to modify electrical devices. Only the given fields are changed.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "apparentPower": {
                            "type": "integer",
                            "minimum": 0,
                            "description": "The apparent power in VA."
                        },
                        "isApparentPowerEnabled": {
                            "type": "boolean"
                        },
                        "loadClassificationName": {
                            "type": "string",
                            "description": "One of the load classifications of the electrical project settings, or an empty string for none."
                        },
                        "anchorPoint": {
                            "$ref": "#/Coordinate3D"
                        },
                        "orientation": {
                            "$ref": "#/MEPOrientation"
                        },
                        "rotation": {
                            "type": "number",
                            "description": "The base rotation in radians."
                        },
                        "isReflected": {
                            "type": "boolean"
                        },
                        "libraryPartId": {
                            "$ref": "#/LibPartUnId",
                            "description": "The library part to use, as listed by GetAvailableLibraryParts."
                        },
                        "xSize": {
                            "type": "number",
                            "description": "The size along the local x axis. Some library parts calculate their own size, then the item fails."
                        },
                        "ySize": {
                            "type": "number",
                            "description": "The size along the local y axis. Some library parts calculate their own size, then the item fails."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "electricalDevicesData"
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
                "name": "ConnectToElectricalCircuits",
                "version": "1.7.2",
                "description": "Connects electrical devices, or panels as sub-panels, to electrical circuits. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "connectionsData": {
                "type": "array",
                "description": "Array of electrical devices or panels and the circuits to connect them to. A panel connected to a circuit becomes a sub-panel of the panel of that circuit.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId",
                            "description": "An electrical device or an electrical panel."
                        },
                        "circuitId": {
                            "$ref": "#/ElementId",
                            "description": "The electrical circuit to connect to."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "circuitId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "connectionsData"
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
                "name": "ModifyElectricalProjectSettings",
                "version": "1.7.2",
                "description": "Modifies the electrical project settings: sets the default voltage and adds or removes wiring types and load classifications. Available from Archicad 30.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "voltage": {
                "type": "number"
            },
            "wiringTypesToAdd": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "crossSectionalArea": {
                            "type": "number"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "crossSectionalArea"
                    ]
                }
            },
            "wiringTypesToRemove": {
                "type": "array",
                "description": "The names of the wiring types to remove.",
                "items": {
                    "type": "string"
                }
            },
            "loadClassificationsToAdd": {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string"
                        },
                        "demandFactor": {
                            "type": "number"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "name",
                        "demandFactor"
                    ]
                }
            },
            "loadClassificationsToRemove": {
                "type": "array",
                "description": "The names of the load classifications to remove.",
                "items": {
                    "type": "string"
                }
            }
        },
        "additionalProperties": false
    },
                "outputScheme": {
        "$ref": "#/ExecutionResult"
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
                    "$ref": "#/SolidLinkData"
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
                    "$ref": "#/SolidLinkReference"
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
                    "$ref": "#/SolidLinksOfElement"
                }
            }
        },
        "additionalProperties": false,
        "required": [ "solidLinks" ]
    }
            },{
                "name": "TrimElements",
                "version": "1.5.9",
                "description": "Trims construction elements with a roof or shell: the roofs and shells in the list, or one given trimming element with a trim type.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elements": {
                "$ref": "#/Elements",
                "description": "The construction elements to trim. Without trimmingElement the roofs and shells among them do the trimming."
            },
            "trimmingElement": {
                "$ref": "#/ElementId",
                "description": "Optional. The roof or shell that trims every element in the list."
            },
            "trimType": {
                "$ref": "#/TrimType",
                "description": "Used with trimmingElement; defaults to KeepInside."
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
                "name": "RemoveElementTrims",
                "version": "1.5.9",
                "description": "Removes the trim between an element and the roof or shell trimming it.",
                "inputScheme": {
        "type": "object",
        "properties": {
            "elementPairs": {
                "type": "array",
                "description": "The trimmed element and the roof or shell trimming it, per pair.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": {
                            "$ref": "#/ElementId"
                        },
                        "trimmingElementId": {
                            "$ref": "#/ElementId"
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "elementId",
                        "trimmingElementId"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementPairs"
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
                "name": "GetElementTrims",
                "version": "1.5.9",
                "description": "Which roofs and shells trim each queried element, with the trim type, and which elements it trims.",
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
            "elementTrims": {
                "type": "array",
                "description": "One item per queried element, in order. An unknown or deleted element is an error item, so it cannot be mistaken for an element that is simply not trimmed.",
                "items": {
                    "$ref": "#/ElementTrimsOrError"
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "elementTrims"
        ]
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
                "description": "Prevents the page from navigating away (e.g. clicking a link to another site) once it has loaded. Defaults to false."
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
                "description": "Retrieves and clears the result last submitted from the Script UI palette's page (via window.ACAPI.SubmitResult), if any, and reports whether the palette is still open, so a waiting script can stop when the user closes it.",
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
            },
            "isOpen": {
                "type": "boolean",
                "description": "True while the Script UI palette is open. False before the first ShowScriptUI call and once the user closed the palette with its close box, the page called window.ACAPI.ClosePalette or Archicad closed it; stays true while Archicad only temporarily hides all palettes (e.g. during a modal dialog). Lets a waiting script stop polling when the user closes the palette."
            }
        },
        "additionalProperties": false,
        "required": [
            "hasResult",
            "isOpen"
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