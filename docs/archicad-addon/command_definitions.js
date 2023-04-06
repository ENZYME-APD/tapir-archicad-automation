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
                name : "GetProjectInfoFields",
                version : "0.1.2",
                description : "Retrieves the names and values of all project info fields.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "SetProjectInfoField",
                version : "0.1.2",
                description : "Sets the value of a project info field.",
                inputScheme : {
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
                outputScheme : null
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
        name : 'Library Commands',
        commands : [
            {
                name : "ReloadLibraries",
                version : "1.0.0",
                description : "Executes the reload libraries command.",
                inputScheme : null,
                outputScheme : null             
            }        
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
            }
        ]
    },
    {
        name : 'Attribute Commands',
        commands : [
            {
                name : "CreateBuildingMaterials",
                version : "1.1.0",
                description : "Creates Building Material attributes based on the given parameters.",
                inputScheme : {
                    "type": "object",
                    "properties": {
                        "buildingMaterialDataArray": {
                            "type": "array",
                            "description" : "Array of data to create Building Materials.",
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
                                    "cutFill": {
                                        "type": "integer",
                                        "description": "Cut Fill."
                                    },
                                    "cutFillPen": {
                                        "type": "integer",
                                        "description": "Cut Fill Foreground Pen."
                                    },
                                    "cutFillBackgroundPen": {
                                        "type": "integer",
                                        "description": "Cut Fill Background Pen."
                                    },
                                    "cutMaterial": {
                                        "type": "integer",
                                        "description": "Cut Surface."
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
                outputScheme : {
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
                }
            },
            {
                name : "GetBuildingMaterialPhysicalProperties",
                version : "0.1.3",
                description : "Retrieves the physical properties of the given Building Materials.",
                inputScheme : {
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
                outputScheme : {
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
