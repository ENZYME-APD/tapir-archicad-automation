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
			{
                name : "GetStoryInfo",
                version : "1.0.2",
                description : "Retrieves information about the story sructure of the currently loaded project.",
                inputScheme : null,
                outputScheme : {
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
									"uName": {
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
									"uName"
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
            }			
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
            },
            {
                name : "GetLibraries",
                version : "1.0.1",
                description : "Gets the list of loaded libraries.",
                inputScheme : null,
                outputScheme : {
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
            },
            {
                name : "HighlightElements",
                version : "1.0.3",
                description : "Highlights the elements given in the elements array. In case of empty elements array removes all previously set highlights.",
                inputScheme : {
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
                outputScheme : null
            },
            {
                name : "MoveElements",
                version : "1.0.2",
                description : "Moves elements with a given vector.",
                inputScheme : {
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
                                        "description" : "3D vector.",
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
                outputScheme : null             
            },
            {
                name : "GetGDLParametersOfElements",
                version : "1.0.2",
                description : "Get all the GDL parameters (name, type, value) of the given elements.",
                inputScheme : {
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
                outputScheme : {
                    "type": "object",
                    "properties": {
                        "gdlParametersOfElements": {
                            "type": "array",
                            "description": "The GDL parameters of elements.",
                            "items": {
                                "$ref": "#/GDLParametersDictionary"
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "gdlParametersOfElements"
                    ]
                }           
            },
            {
                name : "SetGDLParametersOfElements",
                version : "1.0.2",
                description : "Sets the given GDL parameters of the given elements.",
                inputScheme : {
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
                                        "$ref": "#/GDLParametersDictionary"
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
                outputScheme : null          
            }
        ]
    },
    {
        name : 'Element Commands',
        commands : [
            {
                name : "CreateColumns",
                version : "1.0.3",
                description : "Creates Column elements based on the given parameters.",
                inputScheme : {
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
            {
                name : "CreateSlabs",
                version : "1.0.3",
                description : "Creates Slab elements based on the given parameters.",
                inputScheme : {
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
                                            "type": "object",
                                            "description" : "Position of a 2D point.",
                                            "properties" : {
                                                "x": {
                                                    "type": "number",
                                                    "description" : "X value of the point."
                                                },
                                                "y" : {
                                                    "type": "number",
                                                    "description" : "Y value of the point."
                                                }
                                            },
                                            "additionalProperties": false,
                                            "required" : [
                                                "x",
                                                "y"
                                            ]
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
                                                        "type": "object",
                                                        "description" : "Position of a 2D point.",
                                                        "properties" : {
                                                            "x": {
                                                                "type": "number",
                                                                "description" : "X value of the point."
                                                            },
                                                            "y" : {
                                                                "type": "number",
                                                                "description" : "Y value of the point."
                                                            }
                                                        },
                                                        "additionalProperties": false,
                                                        "required" : [
                                                            "x",
                                                            "y"
                                                        ]
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
            {
                name : "CreateObjects",
                version : "1.0.3",
                description : "Creates Object elements based on the given parameters.",
                inputScheme : {
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
                version : "1.0.1",
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
                outputScheme : {
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
            },
            {
                name : "CreateLayers",
                version : "1.0.3",
                description : "Creates Layer attributes based on the given parameters.",
                inputScheme : {
					"type": "object",
					"properties": {
						"layerDataArray": {
							"type": "array",
							"description" : "Array of data to create Layers.",
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
                outputScheme : {
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
            },
            {
                name : "CreateComposites",
                version : "1.0.2",
                description : "Creates Composite attributes based on the given parameters.",
                inputScheme : {
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
                outputScheme : {
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
    },
    {
        name : 'Issue Management Commands',
        commands : [
            {
                name : "CreateIssue",
                version : "1.0.2",
                description : "Creates a new issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "name": {
			                "type": "string",
			                "description": "The name of the issue."
			            },
			            "parentId": {
			                "type": "string",
			                "description": "The id of the parent issue, optional."
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
                outputScheme : null
            },
            {
                name : "DeleteIssue",
                version : "1.0.2",
                description : "Deletes a chosen issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to delete."
			            }
			        },
			        "additionalProperties": false,
			        "required": [
			            "issueId"
			        ]
                },
                outputScheme : null
            },
            {
                name : "GetIssues",
                version : "1.0.2",
                description : "Retrieves information about existing issues.",
                inputScheme : null,
                outputScheme : {
			        "type": "object",
			        "properties": {
			            "issues": {
			                "type": "array",
			                "description": "A list of existing issues.",
			                "items": {
			                    "type": "object",
			                    "properties": {
			                        "guid": {
			                            "type": "string",
			                            "description": "Issue identifier"
			                        },
			                        "name": {
			                            "type": "string",
			                            "description": "Issue name"
			                        },
			                        "parentGuid": {
			                            "type": "string",
			                            "description": "The identifier of the parent issue"
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
			                        "tagTextElemGuid": {
			                            "type": "string",
			                            "description": "The identifier of the attached tag text element"
			                        },
			                        "isTagTextElemVisible": {
			                            "type": "boolean",
			                            "description": "The visibility of the attached tag text element"
			                        }
			                    },
			                    "additionalProperties": false,
			                    "required": [
			                        "guid",
			                        "name",
			                        "parentGuid",
			                        "creaTime",
			                        "modiTime",
			                        "tagText",
			                        "tagTextElemGuid",
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
            },
            {
                name : "AddComment",
                version : "1.0.2",
                description : "Adds a new comment to the chosen issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to add the comment."
			            },
			            "author": {
			                "type": "string",
			                "description": "The author of the new comment."
			            },
			            "status": {
			                "type": "integer",
			                "description": "Comment status type."
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
                outputScheme : null
            },
            {
                name : "GetComments",
                version : "1.0.2",
                description : "Retrieves comments information of the chosen issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to get the comments."
			            }
			        },
			        "additionalProperties": false,
			        "required": [
			            "issueId"
			        ]
                },
                outputScheme : {
			        "type": "object",
			        "properties": {
			            "comments": {
			                "type": "array",
			                "description": "A list of existing comments.",
			                "items": {
			                    "type": "object",
			                    "properties": {
			                        "guid": {
			                            "type": "string",
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
			                            "type": "string",
			                            "description": "Comment status"
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
            },
            {
                name : "AttachElements",
                version : "1.0.2",
                description : "Attaches elements to the chosen issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to attach elements."
			            },
			            "elementsIds": {
			                "$ref": "#/Elements"
			            },
			            "type": {
			                "type": "integer",
			                "description": "Attachment type status. (0: New, 1: Highlighted, 2: Deleted, 3: Modified"
			            }
			        },
			        "additionalProperties": false,
			        "required": [
			            "issueId",
			            "elementsIds",
			            "type"
			        ]
                },
                outputScheme : null
            },
            {
                name : "DetachElements",
                version : "1.0.2",
                description : "Detaches elements from the chosen issue.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to deattach elements."
			            },
			            "elementsIds": {
			                "$ref": "#/Elements"
			            }
			        },
			        "additionalProperties": false,
			        "required": [
			            "issueId",
			            "elementsIds"
			        ]
                },
                outputScheme : null
            },
            {
                name : "GetAttachedElements",
                version : "1.0.2",
                description : "Retrieves attached elements of the chosen issue, filtered by attachment type.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issueId": {
			                "type": "string",
			                "description": "The id of the issue to get elements."
			            },
			            "type": {
			                "type": "integer",
			                "description": "The attachment type to filter elements. (0: New, 1: Highlighted, 2: Deleted, 3: Modified)"
			            }
			        },
			        "additionalProperties": false,
			        "required": [
			            "issueId",
			            "type"
			        ]
                },
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
            {
                name : "ExportToBCF",
                version : "1.0.2",
                description : "Exports chosen issues to the bcf file.",
                inputScheme : {
			        "type": "object",
			        "properties": {
			            "issuesIds": {
			                "type": "array",
			                "description": "Issue Ids to export. Empty parameter will fetch all existing issues.",
			                "items": {
			                    "type": "string"
			                }
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
                outputScheme : null
            },
            {
                name : "ImportFromBCF",
                version : "1.0.2",
                description : "Imports issues from the chosen bcf file.",
                inputScheme : {
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
                outputScheme : null
            },
        ]
    }
];
