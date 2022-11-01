var gCommands = [
    {
        name : 'Application Commands',
        commands : [
            {
                name : "GetAddOnVersion",
                version : "0.1.0",
                description : "Retrieves the version of the Tapir Additional JSON Commands Add-On.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "GetArchicadLocation",
                version : "0.1.0",
                description : "Retrieves the location of the currently running Archicad executable.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "QuitArchicad",
                version : "0.1.0",
                description : "Performs a quit operation on the currently running Archicad instance.",
                inputScheme : null,
                outputScheme : null
            }
        ]
    },
    {
        name : 'Project Commands',
        commands : [
            {
                name : "GetProjectInfo",
                version : "0.1.0",
                description : "Retrieves information about the currently loaded project.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "GetHotlinks",
                version : "0.1.0",
                description : "Gets the file system locations (path) of the hotlink modules. The hotlinks can have tree hierarchy in the project.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "PublishPublisherSet",
                version : "0.1.0",
                description : "Performs a publish operation on the currently opened project. Only the given publisher set will be published.",
                inputScheme : {
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
                outputScheme : null
            },
        ]
    },
    {
        name : 'Element Commands',
        commands : [
            {
                name : "GetSelectedElements",
                version : "0.1.0",
                description : "Gets the list of the currently selected elements.",
                inputScheme : null,
                outputScheme : {
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
            },
        ]
    },
    {
        name : 'Teamwork Commands',
        commands : [
            {
                name : "TeamworkSend",
                version : "0.1.0",
                description : "Performs a send operation on the currently opened Teamwork project.",
                inputScheme : null,
                outputScheme : null             
            },        
            {
                name : "TeamworkReceive",
                version : "0.1.0",
                description : "Performs a receive operation on the currently opened Teamwork project.",
                inputScheme : null,
                outputScheme : null             
            },
        ]
    }
];
