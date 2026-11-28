**2704 of 3746** fields of the Archicad element types that a command can write or read have a command key of their own (72 %; 3906 fields in all): ✏️ 1785 key, written, 👁️ 919 key, read, 🔧 1042 generic only, ➖ 48 not needed, 🔒 112 read only. On Archicad 29 and 30, GetElementFields and SetElementFields reach every field by its path.

| Element type | With a key | Fields | ✏️ | 👁️ | 🔧 | ➖ | 🔒 |
|---|---|---|---|---|---|---|---|
| [`API_StairType`](#api_stairtype) | 88 % | 833 | 341 | 386 | 96 |  | 10 |
| [`API_RailingType`](#api_railingtype) | 8 % | 228 | 13 | 2 | 182 |  | 31 |
| [`API_ShellType`](#api_shelltype) | 94 % | 189 | 107 | 71 | 11 |  |  |
| [`API_CurtainWallType`](#api_curtainwalltype) | 37 % | 187 | 42 | 6 | 81 | 48 | 10 |
| [`API_RailingNodeType`](#api_railingnodetype) | 8 % | 133 | 8 | 1 | 108 |  | 16 |
| [`API_AngleDimensionType`](#api_angledimensiontype) | 33 % | 113 | 35 | 1 | 73 |  | 4 |
| [`API_RoofType`](#api_rooftype) | 94 % | 108 | 78 | 24 | 6 |  |  |
| [`API_LabelType`](#api_labeltype) | 45 % | 107 | 47 |  | 58 |  | 2 |
| [`API_InteriorElevationType`](#api_interiorelevationtype) | 88 % | 102 | 20 | 65 | 12 |  | 5 |
| [`API_DimensionType`](#api_dimensiontype) | 44 % | 101 | 43 | 1 | 56 |  | 1 |
| [`API_CutPlaneType`](#api_cutplanetype) | 81 % | 99 | 67 | 12 | 19 |  | 1 |
| [`API_ElevationType`](#api_elevationtype) | 81 % | 99 | 67 | 12 | 19 |  | 1 |
| [`API_LevelDimensionType`](#api_leveldimensiontype) | 46 % | 89 | 41 |  | 48 |  |  |
| [`API_RailingSegmentType`](#api_railingsegmenttype) | 18 % | 81 | 9 | 3 | 54 |  | 15 |
| [`API_MorphType`](#api_morphtype) | 99 % | 72 | 55 | 16 | 1 |  |  |
| [`API_HatchType`](#api_hatchtype) | 60 % | 70 | 36 | 6 | 28 |  |  |
| [`API_RadialDimensionType`](#api_radialdimensiontype) | 60 % | 65 | 33 | 6 | 26 |  |  |
| [`API_WallType`](#api_walltype) | 90 % | 65 | 51 | 5 | 6 |  | 3 |
| [`API_BeamType`](#api_beamtype) | 87 % | 63 | 50 | 4 | 8 |  | 1 |
| [`API_ColumnType`](#api_columntype) | 94 % | 57 | 47 | 4 | 3 |  | 3 |
| [`API_DrawingType`](#api_drawingtype) | 68 % | 57 | 31 | 8 | 18 |  |  |
| [`API_SlabType`](#api_slabtype) | 98 % | 50 | 30 | 19 | 1 |  |  |
| [`API_StairStructureType`](#api_stairstructuretype) | 91 % | 47 | 11 | 31 | 4 |  | 1 |
| [`API_DoorType`](#api_doortype) | 100 % | 46 | 40 | 6 |  |  |  |
| [`API_WindowType`](#api_windowtype) | 100 % | 46 | 40 | 6 |  |  |  |
| [`API_RailingPatternType`](#api_railingpatterntype) | 100 % | 45 | 4 | 37 |  |  | 4 |
| [`API_ZoneType`](#api_zonetype) | 96 % | 45 | 42 | 1 | 2 |  |  |
| [`API_CWFrameType`](#api_cwframetype) | 60 % | 43 | 6 | 20 | 17 |  |  |
| [`API_MeshType`](#api_meshtype) | 93 % | 42 | 36 | 3 | 3 |  |  |
| [`API_LampType`](#api_lamptype) | 98 % | 41 | 36 | 4 | 1 |  |  |
| [`API_ObjectType`](#api_objecttype) | 98 % | 41 | 36 | 4 | 1 |  |  |
| [`API_SkylightType`](#api_skylighttype) | 95 % | 38 | 35 | 1 | 2 |  |  |
| [`API_TextType`](#api_texttype) | 100 % | 32 | 18 | 13 |  |  | 1 |
| [`API_CameraType`](#api_cameratype) | 87 % | 30 | 16 | 10 | 4 |  |  |
| [`API_ArcType`](#api_arctype) | 54 % | 24 | 12 | 1 | 11 |  |  |
| [`API_CWPanelType`](#api_cwpaneltype) | 75 % | 24 | 13 | 5 | 6 |  |  |
| [`API_CircleType`](#api_circletype) | 54 % | 24 | 12 | 1 | 11 |  |  |
| [`API_CWSegmentType`](#api_cwsegmenttype) | 83 % | 23 |  | 19 | 4 |  |  |
| [`API_DetailType`](#api_detailtype) | 91 % | 22 | 14 | 6 | 2 |  |  |
| [`API_WorksheetType`](#api_worksheettype) | 91 % | 22 | 14 | 6 | 2 |  |  |
| [`API_BeamSegmentType`](#api_beamsegmenttype) | 95 % | 21 | 12 | 8 | 1 |  |  |
| [`API_HotlinkType`](#api_hotlinktype) | 95 % | 21 | 20 |  | 1 |  |  |
| [`API_ColumnSegmentType`](#api_columnsegmenttype) | 95 % | 20 | 17 | 2 | 1 |  |  |
| [`API_PictureType`](#api_picturetype) | 75 % | 20 | 13 | 2 | 5 |  |  |
| [`API_LineType`](#api_linetype) | 53 % | 19 | 10 |  | 9 |  |  |
| [`API_PolyLineType`](#api_polylinetype) | 47 % | 19 | 9 |  | 10 |  |  |
| [`API_RailingBalusterSetType`](#api_railingbalustersettype) | 82 % | 18 | 12 | 2 | 3 |  | 1 |
| [`API_SplineType`](#api_splinetype) | 35 % | 17 | 5 | 1 | 11 |  |  |
| [`API_RailingRailEndType`](#api_railingrailendtype) | 93 % | 16 |  | 14 | 1 |  | 1 |
| [`API_RailingRailConnectionType`](#api_railingrailconnectiontype) | 100 % | 13 |  | 13 |  |  |  |
| [`API_StairTreadType`](#api_stairtreadtype) | 77 % | 13 |  | 10 | 3 |  |  |
| [`API_ChangeMarkerType`](#api_changemarkertype) | 100 % | 12 | 11 | 1 |  |  |  |
| [`API_CamSetType`](#api_camsettype) | 82 % | 11 | 9 |  | 2 |  |  |
| [`API_RailingPanelType`](#api_railingpaneltype) | 100 % | 11 | 10 | 1 |  |  |  |
| [`API_StairRiserType`](#api_stairrisertype) | 73 % | 11 |  | 8 | 3 |  |  |
| [`API_CWJunctionType`](#api_cwjunctiontype) | 60 % | 10 |  | 6 | 4 |  |  |
| [`API_CWAccessoryType`](#api_cwaccessorytype) | 62 % | 8 |  | 5 | 3 |  |  |
| [`API_RailingInnerPostType`](#api_railinginnerposttype) | 100 % | 8 | 6 | 2 |  |  |  |
| [`API_RailingHandrailType`](#api_railinghandrailtype) | 100 % | 6 | 4 | 2 |  |  |  |
| [`API_RailingRailType`](#api_railingrailtype) | 100 % | 6 | 4 | 2 |  |  |  |
| [`API_SectElemType`](#api_sectelemtype) | 100 % | 5 |  | 4 |  |  | 1 |
| [`API_HotspotType`](#api_hotspottype) | 100 % | 4 | 4 |  |  |  |  |
| [`API_RailingBalusterType`](#api_railingbalustertype) | 100 % | 4 |  | 4 |  |  |  |
| [`API_RailingPostType`](#api_railingposttype) | 100 % | 4 | 2 | 2 |  |  |  |
| [`API_RailingToprailType`](#api_railingtoprailtype) | 100 % | 3 | 1 | 2 |  |  |  |
| [`API_RailingFinishType`](#api_railingfinishtype) | 100 % | 2 |  | 2 |  |  |  |
| [`API_ExternalElemType`](#api_externalelemtype) | 0 % | 1 |  |  | 1 |  |  |

Fields of the Archicad 30 DevKit; an estimate read from the code, updated 2026-11-28 21:03 UTC.
