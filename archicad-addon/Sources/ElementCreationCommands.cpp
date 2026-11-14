#include "ElementCreationCommands.hpp"
#include "ExtendedElementCommands.hpp"
#include "LibraryPartParameters.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"
#include "NotificationCommands.hpp"
#include "Base64Converter.hpp"
#include "File.hpp"
#include "Location.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

CreateElementsCommandBase::CreateElementsCommandBase (const GS::String& commandNameIn, API_ElemTypeID elemTypeIDIn, const GS::String& arrayFieldNameIn)
    : CommandBase (CommonSchema::Used)
    , commandName (commandNameIn)
    , elemTypeID (elemTypeIDIn)
    , arrayFieldName (arrayFieldNameIn)
{
}

GS::String CreateElementsCommandBase::GetName () const
{
    return commandName;
}

GS::Optional<GS::UniString> CreateElementsCommandBase::GetRawResponseSchema () const
{
    return R"({
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
    })";
}

GS::Optional<GS::ObjectState> CreateElementsCommandBase::CreateElement (API_Element& element, API_ElementMemo& memo, const GS::ObjectState& /*parameters*/) const
{
    const GSErrCode err = ACAPI_Element_Create (&element, &memo);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to create new " + GetElementTypeNonLocalizedName (elemTypeID));
    }
    return {};
}

GS::ObjectState	CreateElementsCommandBase::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> dataArray;
    parameters.Get (arrayFieldName, dataArray);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    const GS::UniString elemTypeName = GetElementTypeNonLocalizedName (elemTypeID);
    const Stories stories = GetStories ();

    // Optional top-level "databaseId" of the 2D Create* commands (#1602) and of CreateObjects
    // and CreateLamps (#2018, e.g. a legend of symbols in a worksheet): the elements are
    // created in that database (a worksheet, a detail, a layout, ...) instead of the current
    // one, and the current database is switched back afterwards, so the front window stays.
    // Only the commands whose schema names it receive it. The switch happens inside the
    // undoable command, as CopyElementsToDatabase does it.
    const GS::ObjectState* databaseIdOs = parameters.Get ("databaseId");
    API_DatabaseInfo startingDatabase = {};
    if (databaseIdOs != nullptr) {
        API_WindowInfo startingWindow = {};
        const bool hasStartingWindow = ACAPI_Window_GetCurrentWindow (&startingWindow) == NoError;
        if (OpenDatabaseIfNeverOpened (GetGuidFromObjectState (*databaseIdOs)) && hasStartingWindow) {
            ACAPI_Database_ChangeCurrentDatabase (&startingWindow);
            ACAPI_Window_ChangeWindow (&startingWindow);
        }
        const GSErrCode currentErr = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
        if (currentErr != NoError) {
            return CreateErrorResponse (currentErr, "Failed to get the current database.");
        }
    }
    GSErrCode databaseErr = NoError;

    API_NotifyElementType notification = {};
    notification.notifID = APINotifyElement_BeginEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    ACAPI_CallUndoableCommand ("Create " + elemTypeName, [&] () -> GSErrCode {
        if (databaseIdOs != nullptr) {
            databaseErr = ChangeCurrentDatabaseTo (GetGuidFromObjectState (*databaseIdOs));
            if (databaseErr != NoError) {
                ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
                return NoError;
            }
        }
        const GS::OnExit databaseGuard ([&] () {
            if (databaseIdOs != nullptr) {
                ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
            }
        });

        API_Element element = {};
        API_ElementMemo memo = {};
        const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });

#ifdef ServerMainVers_2600
        element.header.type   = elemTypeID;
#else
        element.header.typeID = elemTypeID;
#endif
        GSErrCode err = ACAPI_Element_GetDefaults (&element, &memo);

        bool savedAutoTextFlag;
        ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag);
        bool setAutoTextFlag = false;
        ACAPI_AutoText_ChangeAutoTextFlag (&setAutoTextFlag);

        // Per-item "favoriteName" support. Favorites can only be applied through the
        // tool defaults, which are global and outlive the command, so the defaults are
        // snapshotted here and restored at the end - and also before any item that does
        // NOT name a favorite, otherwise it would silently inherit the favorite of the
        // item before it. Untouched (no allocation, no API traffic) when no item names
        // a favorite, which keeps the common path exactly as it was.
        const bool anyFavoriteName = [&] () {
            for (const GS::ObjectState& data : dataArray) {
                GS::UniString name;
                if (data.Get ("favoriteName", name) && !name.IsEmpty ()) {
                    return true;
                }
            }
            return false;
        } ();

        API_Element defaultsSnapshot = element;
        API_ElementMemo defaultsSnapshotMemo = {};
        const GS::OnExit snapshotGuard ([&] () {
            if (anyFavoriteName) {
                ACAPI_DisposeElemMemoHdls (&defaultsSnapshotMemo);
            }
        });
        if (anyFavoriteName) {
            ACAPI_Element_GetDefaults (&defaultsSnapshot, &defaultsSnapshotMemo);
        }
        bool favoriteApplied = false;

        for (const GS::ObjectState& data : dataArray) {
            if (anyFavoriteName) {
                GS::UniString favoriteName;
                const bool hasFavoriteName = data.Get ("favoriteName", favoriteName) && !favoriteName.IsEmpty ();
                if (hasFavoriteName) {
                    const GSErrCode favoriteErr = ApplyFavoriteToElementDefaults (favoriteName, elemTypeID);
                    if (favoriteErr != NoError) {
                        // Say which of the two usual causes it was: ApplyFavoriteToElementDefaults
                        // answers APIERR_REFUSEDPAR for a favorite of another element type, and
                        // passes on the ACAPI_Favorite_Get error when no favorite has this name
                        // (a typo, trailing space or different case in the name is the usual reason).
                        const GS::UniString reason = favoriteErr == APIERR_REFUSEDPAR
                            ? GS::UniString ("it is not a " + elemTypeName + " favorite")
                            : GS::UniString ("no favorite with this exact name was found");
                        elements (CreateErrorResponse (favoriteErr,
                            "Failed to apply favoriteName '" + favoriteName + "' to the " + elemTypeName + " defaults: " + reason + "."));
                        continue;
                    }
                    favoriteApplied = true;
                } else if (favoriteApplied) {
                    API_Element restoreMask;
                    ACAPI_ELEMENT_MASK_SETFULL (restoreMask);
                    ACAPI_Element_ChangeDefaults (&defaultsSnapshot, &defaultsSnapshotMemo, &restoreMask);
                    favoriteApplied = false;
                }
            }

            // Every item starts from the (favorite-applied or restored) tool defaults.
            // SetTypeSpecificParameters only writes the fields the item actually names,
            // so without this the previous item's values survive into this one: confirmed
            // live that CreateBeams with [{slantAngle: 0.3}, {}] slants BOTH beams, and
            // CreateWalls with [{arcAngle: 0.5}, {}] curves both walls.
            ACAPI_DisposeElemMemoHdls (&memo);
            memo = {};
            element = {};
#ifdef ServerMainVers_2600
            element.header.type   = elemTypeID;
#else
            element.header.typeID = elemTypeID;
#endif
            err = ACAPI_Element_GetDefaults (&element, &memo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to read the " + elemTypeName + " defaults."));
                continue;
            }

            auto os = SetTypeSpecificParameters (element, memo, stories, data);
            if (os.HasValue ()) {
                elements (*os);
                continue;
            }

            // Archicad creates an element on a hidden layer (the layer of the tool defaults, a
            // favorite or layerIndex), but then refuses to write its id and to delete it again,
            // so the element would stay with an automatic id while the item answers that it was
            // not created (#2632). Refuse such an item before anything is created.
            GS::UniString id;
            const bool hasId = data.Get ("id", id);
            if (hasId) {
                GS::UniString layerReason;
                err = GetLayerChangeError (element.header.layer, layerReason);
                if (err != NoError) {
                    elements (CreateErrorResponse (err, "The " + elemTypeName + " was not created, because " + layerReason +
                        ", so its id cannot be set. Show or unlock the layer, or give the layerIndex of a visible, unlocked layer."));
                    continue;
                }
            }

            const API_Element requestedElement = element;
            const GS::Optional<GS::ObjectState> createError = CreateElement (element, memo, data);
            if (createError.HasValue ()) {
                elements (*createError);
                continue;
            }

            // As for the id below, a new element that could not be finished is deleted again.
            err = AfterCreate (requestedElement, element.header.guid, data);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to finish the new " + elemTypeName + ", " + DeleteUnfinishedElement (element.header.guid)));
                continue;
            }

            // Per-item "id" support: the Element ID is written right after the creation, in
            // the same undoable command, so the create call itself stays as it was. Without
            // an id the element keeps the next automatic ID of its tool. An id that cannot be
            // written leaves no half-done element behind: the new element is deleted again
            // and the item answers with the error, so the response never names an element
            // whose id is not the requested one.
            if (hasId) {
                err = ChangeElementId (element.header.guid, id);
                if (err != NoError) {
                    elements (CreateErrorResponse (err, "Failed to set the id of the new " + elemTypeName + ", " + DeleteUnfinishedElement (element.header.guid)));
                    continue;
                }
            }

            notification = {};
            notification.notifID = APINotifyElement_New;
            notification.elemHead = element.header;
            AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

            elements (CreateElementIdObjectState (element.header.guid));
        }

        // Leave the tool defaults as they were found: applying a favorite is a global
        // side effect that would otherwise change what the user's next manual placement
        // (or a later command) starts from.
        if (favoriteApplied) {
            API_Element restoreMask;
            ACAPI_ELEMENT_MASK_SETFULL (restoreMask);
            ACAPI_Element_ChangeDefaults (&defaultsSnapshot, &defaultsSnapshotMemo, &restoreMask);
        }

        ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag);

        return NoError;
    });

    notification = {};
    notification.notifID = APINotifyElement_EndEvents;
    AddElementNotificationClientCommand::ElementEventHandlerProc (&notification);

    if (databaseErr != NoError) {
        return CreateErrorResponse (databaseErr, "Failed to switch to the database given by databaseId, so nothing was created. A worksheet or detail whose window was never opened can be opened once with ChangeWindow by its databaseId.");
    }
    return response;
}

CreateColumnsCommand::CreateColumnsCommand () :
    CreateElementsCommandBase ("CreateColumns", API_ColumnID, "columnsData")
{
}

GS::Optional<GS::ObjectState> ApplyLayerIndexFromParameters (const GS::ObjectState& parameters, API_Element& element)
{
    Int32 layerIndex = 0;
    if (!parameters.Get ("layerIndex", layerIndex)) {
        return GS::NoValue;
    }
    API_Attribute layerAttr = {};
    layerAttr.header.typeID = API_LayerID;
    layerAttr.header.index = ACAPI_CreateAttributeIndex (layerIndex);
    if (layerIndex <= 0 || ACAPI_Attribute_Get (&layerAttr) != NoError) {
        return CreateErrorResponse (APIERR_BADINDEX, "The 'layerIndex' parameter does not refer to an existing layer.");
    }
    if ((layerAttr.header.flags & APILay_Locked) != 0) {
        return CreateErrorResponse (APIERR_LOCKEDLAY, "The layer given by the 'layerIndex' parameter is locked.");
    }
    element.header.layer = layerAttr.header.index;
    return GS::NoValue;
}

GS::Optional<GS::UniString> CreateColumnsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                        "id": {
                            "type": "string",
                            "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                        },
                        "layerIndex": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
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
                            "description": "Optional explicit column height. Sets relativeTopStory to 0, so it replaces the top link of the favorite or of the tool defaults. Cannot be combined with a non-zero relativeTopStory.",
                            "exclusiveMinimum": 0.0
                        },
                        "relativeTopStory": {
                            "type": "integer",
                            "description": "Optional link of the column's top to another story, relative to its home story (1 is the story above); 0 means an explicit height. With a non-zero value Archicad computes the height from the story levels and topOffset. The linked story must exist. Cannot be combined with height. When omitted, the top link of the favorite or of the tool defaults is kept."
                        },
                        "topOffset": {
                            "type": "number",
                            "description": "Optional offset of the column's top from the level of the linked top story (negative is below it). Only has an effect when the top is linked (relativeTopStory non-zero, given here or kept from the favorite or the tool defaults)."
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
                            "description": "Cross section width of the column, the diameter of a round one. Applied to all segments. A custom profile (profileId, or a tool default or favorite that uses one) sets the size itself and Archicad ignores the width; give buildingMaterialId to make a basic cross section of this size.",
                            "exclusiveMinimum": 0.0
                        },
                        "depth": {
                            "type": "number",
                            "description": "Cross section depth (height) of the column. Applied to all segments. Only effective for rectangular columns. A custom profile (profileId, or a tool default or favorite that uses one) sets the size itself and Archicad ignores the depth; give buildingMaterialId to make a basic cross section of this size.",
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
                            "description": "When true, width and depth are kept equal (a round cross section too): a width, or else a depth, given alone sets both. A width and a depth given together with different values make them independent, unless this is given too. When omitted, the tool default (or favoriteName) is used. Applied to all segments."
                        },
                        "buildingMaterialId": {
                            "$ref": "#/AttributeId",
                            "description": "Cross section building material (round or rectangular, per circleBased). Applied to all segments."
                        },
                        "profileId": {
                            "$ref": "#/AttributeId",
                            "description": "Switches the cross section to this custom extruded profile (circleBased becomes false). Applied to all segments."
                        },
                        "veneer": {
                            "$ref": "#/ColumnVeneer",
                            "description": "Veneer of the column (type, thickness, building material); the fields not given come from the tool defaults or favoriteName, thickness 0 creates the column without veneer. Applied to all segments."
                        },
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        },)") + ColumnSurfaceAndFillSchemaProperties + R"(
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
    })";
}

GS::Optional<GS::ObjectState> CreateColumnsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    GS::ObjectState coordinates;
    parameters.Get ("coordinates", coordinates);
    API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", apiCoordinate.z, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.column.bottomOffset = floorIndexAndOffset.second;
    element.column.origoPos.x = apiCoordinate.x;
    element.column.origoPos.y = apiCoordinate.y;

    // The top of the column: an explicit height (relativeTopStory 0), or a link to a story
    // relative to the home story with an offset from that story's level, as ModifyColumns
    // writes them (#2552). A non-zero link and a height in one item are refused, because
    // Archicad computes the height from the story levels once the top is linked.
    double height = 0.0;
    const bool hasHeight = parameters.Get ("height", height);
    double relativeTopStory = 0.0;
    const bool hasRelativeTopStory = parameters.Get ("relativeTopStory", relativeTopStory);
    const short topStoryLink = static_cast<short> (relativeTopStory);
    if (hasHeight && hasRelativeTopStory && topStoryLink != 0) {
        return CreateErrorResponse (APIERR_BADPARS, "Do not give 'height' together with a non-zero 'relativeTopStory': a column linked to a story gets its height from the story levels and 'topOffset'.");
    }
    if (hasRelativeTopStory && topStoryLink != 0 && stories.find (static_cast<short> (element.header.floorInd + topStoryLink)) == stories.end ()) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("'relativeTopStory' %d links the column to story %d, which does not exist.", static_cast<int> (topStoryLink), static_cast<int> (element.header.floorInd + topStoryLink)));
    }
    if (hasHeight) {
        element.column.height = height;
        element.column.relativeTopStory = 0;
    }
    if (hasRelativeTopStory) {
        element.column.relativeTopStory = topStoryLink;
    }
    parameters.Get ("topOffset", element.column.topOffset);
    parameters.Get ("axisRotationAngle", element.column.axisRotationAngle);

    GS::UniString coreAnchor;
    if (parameters.Get ("coreAnchor", coreAnchor)) {
        element.column.coreAnchor = ParseAnchorPointString (coreAnchor);
    }

    // Same rule as CreateBeams and SetDetailsOfElements: without isSlanted the column is created
    // vertical and the angle is discarded.
    double slantAngle = 0.0;
    if (parameters.Get ("slantAngle", slantAngle)) {
        element.column.slantAngle = slantAngle;
        element.column.isSlanted = (slantAngle != 0.0);
    }
    bool isSlanted = false;
    if (parameters.Get ("isSlanted", isSlanted)) {
        element.column.isSlanted = isSlanted;
    }
    double slantDirectionAngle = 0.0;
    if (parameters.Get ("slantDirectionAngle", slantDirectionAngle)) {
        element.column.slantDirectionAngle = slantDirectionAngle;
    }

    // The cut fill pens and the cover fill, written as ModifyColumns writes them (#2220);
    // ACAPI_Element_Create does not use the mask.
    API_Element unusedMask = {};
    ACAPI_ELEMENT_MASK_CLEAR (unusedMask);
    bool settingsChanged = false;
    const GS::Optional<GS::UniString> settingsError = ApplyColumnSurfaceAndFillSettings (element, unusedMask, parameters, settingsChanged);
    if (settingsError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, settingsError.Get ());
    }

    double width = 0.0;
    double depth = 0.0;
    bool hasWidth = parameters.Get ("width", width);
    bool hasDepth = parameters.Get ("depth", depth);

    bool circleBased = false;
    const bool hasCircleBased = parameters.Get ("circleBased", circleBased);
    bool isWidthAndHeightLinked = false;
    const bool hasIsWidthAndHeightLinked = parameters.Get ("isWidthAndHeightLinked", isWidthAndHeightLinked);
    const GS::ObjectState* buildingMaterialIdOs = parameters.Get ("buildingMaterialId");
    const GS::ObjectState* profileIdOs = parameters.Get ("profileId");

    const GS::ObjectState* veneerOs = parameters.Get ("veneer");

    if ((hasWidth || hasDepth || hasCircleBased || hasIsWidthAndHeightLinked || buildingMaterialIdOs != nullptr || profileIdOs != nullptr || veneerOs != nullptr) && memo.columnSegments != nullptr) {
        GSSize nSegments = BMGetPtrSize (reinterpret_cast<GSPtr>(memo.columnSegments)) / sizeof (API_ColumnSegmentType);
        for (GSSize i = 0; i < nSegments; ++i) {
            if (veneerOs != nullptr) {
                const GS::Optional<GS::UniString> veneerError = ApplyColumnVeneer (*veneerOs, memo.columnSegments[i]);
                if (veneerError.HasValue ()) {
                    return CreateErrorResponse (APIERR_BADPARS, veneerError.Get ());
                }
            }
            API_AssemblySegmentData& segment = memo.columnSegments[i].assemblySegmentData;
            ApplySectionSize (segment, hasWidth ? GS::Optional<double> (width) : GS::Optional<double> (), hasDepth ? GS::Optional<double> (depth) : GS::Optional<double> (), hasIsWidthAndHeightLinked, isWidthAndHeightLinked);
            if (hasCircleBased) {
                segment.circleBased = circleBased;
            }
            if (profileIdOs != nullptr) {
                segment.modelElemStructureType = API_ProfileStructure;
                segment.profileAttr = GetAttributeIndexFromGuid (API_ProfileID, GetGuidFromObjectState (*profileIdOs));
                segment.circleBased = false;
            } else if (buildingMaterialIdOs != nullptr) {
                segment.modelElemStructureType = API_BasicStructure;
                segment.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialIdOs));
            }
        }
    }

    return {};
}

CreateSlabsCommand::CreateSlabsCommand () :
    CreateElementsCommandBase ("CreateSlabs", API_SlabID, "slabsData")
{
}

GS::Optional<GS::UniString> CreateSlabsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "minimum": 1,
                        "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
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
                    },)") + SlabSurfaceAndFillSchemaProperties + R"(
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
})";
}

GS::Array<API_PolyArc> GetPolyArcs (const GS::Array<GS::ObjectState>& arcs, Int32 iStart)
{
    GS::Array<API_PolyArc> polyArcs;
    for (const GS::ObjectState& arc : arcs) {
        API_PolyArc polyArc = {};
        if (arc.Get ("begIndex", polyArc.begIndex) &&
            arc.Get ("endIndex", polyArc.endIndex) &&
            arc.Get ("arcAngle", polyArc.arcAngle)) {
            polyArc.begIndex += iStart;
            polyArc.endIndex += iStart;
            polyArcs.Push (polyArc);
        }
    }
    return polyArcs;
}

void AddPolyToMemo (const GS::Array<GS::ObjectState>& coords,
                           const GS::Array<GS::ObjectState>& arcs,
                           Int32&                            iCoord,
                           Int32&                            iArc,
                           Int32&                            iPends,
                           API_ElementMemo&                  memo,
                           const API_EdgeTrimID*             edgeTrimSideType,
                           const API_OverriddenAttribute*    sideMat,
                           bool                              processVertexIDs)
{
    Int32 iStart = iCoord;
    for (const GS::ObjectState& coord : coords) {
        (*memo.coords)[iCoord] = Get2DCoordinateFromObjectState (coord);
        if (edgeTrimSideType != nullptr) {
            (*memo.edgeTrims)[iCoord].sideType = *edgeTrimSideType;
        }
        if (sideMat != nullptr) {
            memo.sideMaterials[iCoord] = *sideMat;
        }
        if (memo.meshPolyZ != nullptr) {
            coord.Get ("z", (*memo.meshPolyZ)[iCoord]);
        }
        if (processVertexIDs && memo.vertexIDs != nullptr) {
            (*memo.vertexIDs)[iCoord] = (UInt32)iCoord;
        }
        ++iCoord;
    }
    (*memo.coords)[iCoord] = (*memo.coords)[iStart];
    if (memo.meshPolyZ != nullptr) {
        (*memo.meshPolyZ)[iCoord] = (*memo.meshPolyZ)[iStart];
    }
    if (processVertexIDs && memo.vertexIDs != nullptr) {
        (*memo.vertexIDs)[iCoord] = (*memo.vertexIDs)[iStart];
    }
    (*memo.pends)[iPends++] = iCoord;
    if (edgeTrimSideType != nullptr) {
        (*memo.edgeTrims)[iCoord].sideType = (*memo.edgeTrims)[iStart].sideType;
        (*memo.edgeTrims)[iCoord].sideAngle = (*memo.edgeTrims)[iStart].sideAngle;
    }
    if (sideMat != nullptr) {
        memo.sideMaterials[iCoord] = memo.sideMaterials[iStart];
    }
    ++iCoord;

    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, iStart);
    for (const API_PolyArc& a : polyArcs) {
        (*memo.parcs)[iArc++] = a;
    }
}

GS::Optional<GS::ObjectState> CreateSlabsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    double inputLevel = 0.0;
    parameters.Get ("level", inputLevel);
    const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", inputLevel, stories);
    element.header.floorInd = floorIndexAndOffset.first;
    element.slab.level = floorIndexAndOffset.second;
    parameters.Get ("thickness", element.slab.thickness);

    bool structureChanged = false;
    auto structureError = ApplySlabStructure (element, nullptr, parameters, structureChanged);
    if (structureError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, structureError.Get ());
    }

    GS::UniString referencePlaneLocation;
    if (parameters.Get ("referencePlaneLocation", referencePlaneLocation)) {
        if (referencePlaneLocation == "Top") {
            element.slab.referencePlaneLocation = APISlabRefPlane_Top;
        } else if (referencePlaneLocation == "CoreTop") {
            element.slab.referencePlaneLocation = APISlabRefPlane_CoreTop;
        } else if (referencePlaneLocation == "CoreBottom") {
            element.slab.referencePlaneLocation = APISlabRefPlane_CoreBottom;
        } else if (referencePlaneLocation == "Bottom") {
            element.slab.referencePlaneLocation = APISlabRefPlane_Bottom;
        }
    }

    // The surfaces, cut fill pens and floor fill, written as ModifySlabs writes them (#2220);
    // ACAPI_Element_Create does not use the mask. Before the polygon memo is built, because
    // BuildSlabMemoFromGeometry gives every edge the slab's side surface.
    API_Element unusedMask = {};
    ACAPI_ELEMENT_MASK_CLEAR (unusedMask);
    bool settingsChanged = false;
    const GS::Optional<GS::UniString> settingsError = ApplySlabSurfaceAndFillSettings (element, unusedMask, parameters, settingsChanged);
    if (settingsError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, settingsError.Get ());
    }

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> polygonArcs;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("polygonCoordinates", polygonCoordinates);
    parameters.Get ("polygonArcs", polygonArcs);
    parameters.Get ("holes", holes);

    auto slabMemoError = BuildSlabMemoFromGeometry (element, memo, polygonCoordinates, polygonArcs, holes);
    if (slabMemoError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid slab geometry: " + slabMemoError.Get ());
    }

    return {};
}

void AddZoneContourAndCoverFill (const API_ZoneType& zone, GS::ObjectState& details)
{
    GS::ObjectState contourLine;
    contourLine.Add ("use", zone.useContourLine);
    contourLine.Add ("pen", static_cast<Int32> (zone.floorContLPen));
    contourLine.Add ("lineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, zone.floorContLType)));
    details.Add ("contourLine", contourLine);

    GS::ObjectState coverFill;
    coverFill.Add ("use", zone.useFloorFill);
    coverFill.Add ("fillId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_FilltypeID, zone.floorFillInd)));
    coverFill.Add ("foregroundPen", static_cast<Int32> (zone.floorFillPen));
    coverFill.Add ("backgroundPen", static_cast<Int32> (zone.floorFillBGPen));
    coverFill.Add ("use3DHatching", zone.use3DHatching);
    coverFill.Add ("useLocalOrigo", zone.useLocalOrigo);
    coverFill.Add ("locOrigo", Create2DCoordinateObjectState (zone.locOrigo));
    coverFill.Add ("fillAngle", zone.fillAngle);
    details.Add ("coverFill", coverFill);
}

GS::Optional<GS::UniString> ApplyZoneContourAndCoverFill (const GS::ObjectState& details, API_ZoneType& zone, API_Element* mask)
{
    // The attribute ids and the pens are all checked before anything is changed.
    GS::ObjectState contourLine;
    const bool hasContourLine = details.Get ("contourLine", contourLine);
    GS::ObjectState coverFill;
    const bool hasCoverFill = details.Get ("coverFill", coverFill);

    GS::Optional<API_AttributeIndex> lineTypeIndex;
    GS::ObjectState lineTypeId;
    if (hasContourLine && contourLine.Get ("lineTypeId", lineTypeId)) {
        lineTypeIndex = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (lineTypeId));
        if (*lineTypeIndex == ACAPI_CreateAttributeIndex (-1)) {
            return GS::UniString ("No Line type attribute was found for contourLine.lineTypeId.");
        }
    }
    GS::Optional<API_AttributeIndex> fillIndex;
    GS::ObjectState fillId;
    if (hasCoverFill && coverFill.Get ("fillId", fillId)) {
        fillIndex = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (fillId));
        if (*fillIndex == ACAPI_CreateAttributeIndex (-1)) {
            return GS::UniString ("No Fill attribute was found for coverFill.fillId.");
        }
    }

    // SetDetailsOfElements does not hold typeSpecificDetails to the pen ranges of the schema.
    const auto isPenInRange = [] (const GS::ObjectState& os, const char* field, Int32 minimum) {
        Int32 pen = 0;
        return !os.Get (field, pen) || (pen >= minimum && pen <= 255);
    };
    if (hasContourLine && !isPenInRange (contourLine, "pen", 1)) {
        return GS::UniString ("contourLine.pen must be between 1 and 255.");
    }
    if (hasCoverFill && !isPenInRange (coverFill, "foregroundPen", 1)) {
        return GS::UniString ("coverFill.foregroundPen must be between 1 and 255.");
    }
    if (hasCoverFill && !isPenInRange (coverFill, "backgroundPen", -1)) {
        return GS::UniString ("coverFill.backgroundPen must be between -1 and 255.");
    }

    if (hasContourLine) {
        if (contourLine.Get ("use", zone.useContourLine) && mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, useContourLine);
        }
        Int32 pen = 0;
        if (contourLine.Get ("pen", pen)) {
            zone.floorContLPen = static_cast<short> (pen);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, floorContLPen);
        }
        if (lineTypeIndex.HasValue ()) {
            zone.floorContLType = *lineTypeIndex;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, floorContLType);
        }
    }

    if (hasCoverFill) {
        if (coverFill.Get ("use", zone.useFloorFill) && mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, useFloorFill);
        }
        if (fillIndex.HasValue ()) {
            zone.floorFillInd = *fillIndex;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, floorFillInd);
        }
        Int32 foregroundPen = 0;
        if (coverFill.Get ("foregroundPen", foregroundPen)) {
            zone.floorFillPen = static_cast<short> (foregroundPen);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, floorFillPen);
        }
        Int32 backgroundPen = 0;
        if (coverFill.Get ("backgroundPen", backgroundPen)) {
            zone.floorFillBGPen = static_cast<short> (backgroundPen);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, floorFillBGPen);
        }
        if (coverFill.Get ("use3DHatching", zone.use3DHatching) && mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, use3DHatching);
        }
        if (coverFill.Get ("useLocalOrigo", zone.useLocalOrigo) && mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, useLocalOrigo);
        }
        GS::ObjectState locOrigo;
        if (coverFill.Get ("locOrigo", locOrigo)) {
            // SetDetailsOfElements does not check typeSpecificDetails against the schema, so a
            // coordinate that is left out keeps its value instead of becoming 0.
            locOrigo.Get ("x", zone.locOrigo.x);
            locOrigo.Get ("y", zone.locOrigo.y);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, locOrigo);
        }
        if (coverFill.Get ("fillAngle", zone.fillAngle) && mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, fillAngle);
        }
    }

    return {};
}

void AddZoneSurface (const API_ZoneType& zone, GS::ObjectState& details)
{
    GS::ObjectState surface;
    surface.Add ("surfaceId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_MaterialID, zone.material)));
    surface.Add ("useOnAllSides", zone.oneMat);
    details.Add ("surface", surface);
}

GS::Optional<GS::UniString> ApplyZoneSurface (const GS::ObjectState& details, API_ZoneType& zone, API_Element* mask)
{
    GS::ObjectState surface;
    if (!details.Get ("surface", surface)) {
        return {};
    }

    // The attribute id is checked before anything is changed.
    GS::Optional<API_AttributeIndex> surfaceIndex;
    GS::ObjectState surfaceId;
    if (surface.Get ("surfaceId", surfaceId)) {
        surfaceIndex = GetAttributeIndexFromGuid (API_MaterialID, GetGuidFromObjectState (surfaceId));
        if (*surfaceIndex == ACAPI_CreateAttributeIndex (-1)) {
            return GS::UniString ("No Surface attribute was found for surface.surfaceId.");
        }
    }

    if (surfaceIndex.HasValue ()) {
        zone.material = *surfaceIndex;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, material);
    }
    if (surface.Get ("useOnAllSides", zone.oneMat) && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, oneMat);
    }

    return {};
}

// The zone polygon display (gross or net, show_found_poly), the uniform stamp pen
// (useStampPens and pen) and the area reduction (reducePercent) of a zone (#2631).
// show_found_poly true uses the reference line of the bounding walls (gross), false
// their inner edges (net); Archicad only uses it for zones whose boundary is the walls'
// reference line (refLineFlag).
void AddZoneDisplayStampPenAndReduction (const API_ZoneType& zone, GS::ObjectState& details)
{
    details.Add ("polygonDisplay", GS::UniString (zone.show_found_poly ? "Gross" : "Net"));

    GS::ObjectState stampPen;
    stampPen.Add ("uniform", !zone.useStampPens);
    stampPen.Add ("pen", static_cast<Int32> (zone.pen));
    details.Add ("stampPen", stampPen);

    details.Add ("reducePercent", zone.reducePercent);
}

GS::Optional<GS::UniString> ApplyZoneDisplayStampPenAndReduction (const GS::ObjectState& details, API_ZoneType& zone, API_Element* mask)
{
    // Everything is checked before anything is changed; SetDetailsOfElements does not hold
    // typeSpecificDetails to the ranges and enums of the schema.
    GS::UniString polygonDisplay;
    const bool hasPolygonDisplay = details.Get ("polygonDisplay", polygonDisplay);
    if (hasPolygonDisplay && polygonDisplay != "Gross" && polygonDisplay != "Net") {
        return GS::UniString ("polygonDisplay must be \"Gross\" or \"Net\".");
    }

    GS::ObjectState stampPen;
    const bool hasStampPen = details.Get ("stampPen", stampPen);
    Int32 pen = 0;
    const bool hasPen = hasStampPen && stampPen.Get ("pen", pen);
    if (hasPen && (pen < 1 || pen > 255)) {
        return GS::UniString ("stampPen.pen must be between 1 and 255.");
    }

    double reducePercent = 0.0;
    const bool hasReducePercent = details.Get ("reducePercent", reducePercent);
    if (hasReducePercent && (reducePercent < 0.0 || reducePercent > 100.0)) {
        return GS::UniString ("reducePercent must be between 0 and 100.");
    }

    if (hasPolygonDisplay) {
        zone.show_found_poly = (polygonDisplay == "Gross");
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, show_found_poly);
    }

    if (hasStampPen) {
        bool uniform = false;
        if (stampPen.Get ("uniform", uniform)) {
            zone.useStampPens = !uniform;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, useStampPens);
        }
        if (hasPen) {
            zone.pen = static_cast<short> (pen);
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, pen);
        }
    }

    if (hasReducePercent) {
        zone.reducePercent = reducePercent;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ZoneType, reducePercent);
    }

    return {};
}

// --- Zone overlap pre-check (#822) -------------------------------------------------
//
// Archicad answers ACAPI_Element_Create of a Zone whose area overlaps another Zone on
// the same story with a modal "overlapping zone areas" warning. A JSON command runs on
// the main thread, so the command does not return until someone clicks that dialog and
// every other API call is blocked meanwhile. The overlap is therefore looked for here,
// before the element is created, and the item fails with an error naming the other
// Zone instead; the per-item "allowOverlap" flag skips the check for callers who
// accept the dialog.
//
// The check uses the polygons as stored in the memo, so a curved edge is taken as its
// chord. Two Zones overlap when they share area: an edge of one properly crosses an
// edge of the other, or a point just inside an edge of one lies strictly inside the
// other. Zones that only touch (a shared edge or corner, the normal case for the rooms
// of a floor plan) do not overlap. For an automatic Zone only its reference point is
// known before creation, so it counts as overlapping when that point is inside an
// existing Zone.

namespace {

struct ZoneArea {
    std::vector<API_Coord>              outline;
    std::vector<std::vector<API_Coord>> holes;
};

// 1 mm, the same as IsSame2DCoordinate: closer than this to an edge is "on" it.
constexpr double ZoneBoundaryTolerance = 0.001;
// How far inside an edge its probe point is taken; above the tolerance so that the
// probe of a shared edge lands clearly outside the neighbouring Zone. Together the two
// mean that a sliver overlap narrower than about 3 mm (with no crossing edges) is
// taken as touching, not overlapping.
constexpr double ZoneProbeOffset = 0.002;

enum class RingPosition {
    Outside,
    OnBoundary,
    Inside
};

static double SquaredDistance (const API_Coord& a, const API_Coord& b)
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static bool IsPointOnSegment (const API_Coord& p, const API_Coord& a, const API_Coord& b)
{
    const double abx = b.x - a.x;
    const double aby = b.y - a.y;
    const double lengthSquared = abx * abx + aby * aby;
    double t = 0.0;
    if (lengthSquared > 0.0) {
        t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / lengthSquared;
        t = std::max (0.0, std::min (1.0, t));
    }
    const API_Coord closest = { a.x + t * abx, a.y + t * aby };
    return SquaredDistance (p, closest) <= ZoneBoundaryTolerance * ZoneBoundaryTolerance;
}

static RingPosition GetRingPosition (const std::vector<API_Coord>& ring, const API_Coord& p)
{
    const size_t n = ring.size ();
    if (n < 3) {
        return RingPosition::Outside;
    }
    bool inside = false;
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const API_Coord& a = ring[j];
        const API_Coord& b = ring[i];
        if (IsPointOnSegment (p, a, b)) {
            return RingPosition::OnBoundary;
        }
        // Even-odd rule: count the edges crossed by the ray from p towards +x.
        if ((a.y > p.y) != (b.y > p.y)) {
            const double xAtP = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (xAtP > p.x) {
                inside = !inside;
            }
        }
    }
    return inside ? RingPosition::Inside : RingPosition::Outside;
}

// Strictly inside the Zone's area: inside its outline and neither in nor on a hole.
static bool IsStrictlyInsideZone (const ZoneArea& zone, const API_Coord& p)
{
    if (GetRingPosition (zone.outline, p) != RingPosition::Inside) {
        return false;
    }
    for (const std::vector<API_Coord>& hole : zone.holes) {
        if (GetRingPosition (hole, p) != RingPosition::Outside) {
            return false;
        }
    }
    return true;
}

// Signed distance of p from the line through a and b, positive on the left of a->b.
static double SignedDistanceFromLine (const API_Coord& a, const API_Coord& b, const API_Coord& p)
{
    const double length = std::sqrt (SquaredDistance (a, b));
    if (length <= 0.0) {
        return 0.0;
    }
    return ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)) / length;
}

static bool SegmentsProperlyCross (const API_Coord& p1, const API_Coord& p2, const API_Coord& q1, const API_Coord& q2)
{
    // Only a proper crossing counts: an endpoint on the other segment (a shared corner
    // or a T-junction) is how neighbouring Zones touch, and that is not an overlap.
    const auto onOppositeSides = [] (double d1, double d2) {
        return (d1 > ZoneBoundaryTolerance && d2 < -ZoneBoundaryTolerance) ||
               (d1 < -ZoneBoundaryTolerance && d2 > ZoneBoundaryTolerance);
    };
    return onOppositeSides (SignedDistanceFromLine (q1, q2, p1), SignedDistanceFromLine (q1, q2, p2)) &&
           onOppositeSides (SignedDistanceFromLine (p1, p2, q1), SignedDistanceFromLine (p1, p2, q2));
}

static bool RingsProperlyCross (const std::vector<API_Coord>& ringA, const std::vector<API_Coord>& ringB)
{
    const size_t nA = ringA.size ();
    const size_t nB = ringB.size ();
    if (nA < 2 || nB < 2) {
        return false;
    }
    for (size_t i = 0, j = nA - 1; i < nA; j = i++) {
        for (size_t k = 0, l = nB - 1; k < nB; l = k++) {
            if (SegmentsProperlyCross (ringA[j], ringA[i], ringB[l], ringB[k])) {
                return true;
            }
        }
    }
    return false;
}

static double SignedArea (const std::vector<API_Coord>& ring)
{
    double area = 0.0;
    const size_t n = ring.size ();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        area += ring[j].x * ring[i].y - ring[i].x * ring[j].y;
    }
    return area / 2.0;
}

// Takes a point ZoneProbeOffset inside the midpoint of each edge of the ring - on the
// ring's interior side for an outline, on its exterior side for a hole, so that the
// probe is always within the Zone's own area - and tells whether any of them lies
// strictly inside the other Zone. Catches identical and nested Zones, which have no
// properly crossing edges.
static bool HasProbePointInside (const std::vector<API_Coord>& ring, bool probeTowardsRingInterior, const ZoneArea& other)
{
    const size_t n = ring.size ();
    if (n < 3) {
        return false;
    }
    const bool leftIsInterior = SignedArea (ring) >= 0.0;
    const double sign = (leftIsInterior == probeTowardsRingInterior) ? 1.0 : -1.0;
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const API_Coord& a = ring[j];
        const API_Coord& b = ring[i];
        const double length = std::sqrt (SquaredDistance (a, b));
        if (length <= ZoneBoundaryTolerance) {
            continue;
        }
        // The left normal of a->b is (-(b.y - a.y), b.x - a.x).
        const API_Coord probe = {
            (a.x + b.x) / 2.0 - sign * (b.y - a.y) / length * ZoneProbeOffset,
            (a.y + b.y) / 2.0 + sign * (b.x - a.x) / length * ZoneProbeOffset
        };
        if (IsStrictlyInsideZone (other, probe)) {
            return true;
        }
    }
    return false;
}

static bool DoZonesOverlap (const ZoneArea& a, const ZoneArea& b)
{
    std::vector<const std::vector<API_Coord>*> ringsA { &a.outline };
    for (const std::vector<API_Coord>& hole : a.holes) {
        ringsA.push_back (&hole);
    }
    std::vector<const std::vector<API_Coord>*> ringsB { &b.outline };
    for (const std::vector<API_Coord>& hole : b.holes) {
        ringsB.push_back (&hole);
    }
    for (const std::vector<API_Coord>* ringA : ringsA) {
        for (const std::vector<API_Coord>* ringB : ringsB) {
            if (RingsProperlyCross (*ringA, *ringB)) {
                return true;
            }
        }
    }
    if (HasProbePointInside (a.outline, true, b) || HasProbePointInside (b.outline, true, a)) {
        return true;
    }
    for (const std::vector<API_Coord>& hole : a.holes) {
        if (HasProbePointInside (hole, false, b)) {
            return true;
        }
    }
    for (const std::vector<API_Coord>& hole : b.holes) {
        if (HasProbePointInside (hole, false, a)) {
            return true;
        }
    }
    return false;
}

static std::vector<API_Coord> GetRingFromObjectStates (const GS::Array<GS::ObjectState>& coords)
{
    std::vector<API_Coord> ring;
    ring.reserve (coords.GetSize ());
    for (const GS::ObjectState& coord : coords) {
        ring.push_back (Get2DCoordinateFromObjectState (coord));
    }
    return ring;
}

struct ExistingZone {
    API_Guid      guid;
    short         floorInd;
    GS::UniString name;
    GS::UniString numberStr;
    ZoneArea      area;
};

// The project's Zones, read once per CreateZones call rather than once per item: a
// batch of hundreds of Zones into a project with hundreds of Zones would otherwise
// read every Zone's memo for every item. Refresh picks up the Zones the earlier items
// of the same call created, which the later items must not overlap either. Commands
// run one at a time on the main thread, so a file-scope cache that CreateZones::Execute
// sets up and tears down is safe; it never outlives the call, so it cannot go stale.
struct ZoneOverlapCache {
    GS::HashSet<API_Guid>     knownGuids;
    std::vector<ExistingZone> zones;
};

ZoneOverlapCache zoneOverlapCache;

// Adds every Zone of the project not seen before, with its polygons as stored in the
// memo (the first sub-polygon is the outline, the others are holes; arcs as chords).
static void RefreshZoneOverlapCache ()
{
    GS::Array<API_Guid> zoneGuids;
    if (ACAPI_Element_GetElemList (API_ZoneID, &zoneGuids) != NoError) {
        return;
    }
    for (const API_Guid& guid : zoneGuids) {
        if (zoneOverlapCache.knownGuids.Contains (guid)) {
            continue;
        }
        zoneOverlapCache.knownGuids.Add (guid);
        API_Element zone = {};
        zone.header.guid = guid;
        if (ACAPI_Element_Get (&zone) != NoError) {
            continue;
        }
        std::vector<PolygonData> polygons = GetPolygonsFromMemoCoords (guid);
        if (polygons.empty () || polygons[0].coords.size () < 3) {
            continue;
        }
        ExistingZone existing;
        existing.guid = guid;
        existing.floorInd = zone.header.floorInd;
        existing.name = GS::UniString (zone.zone.roomName);
        existing.numberStr = GS::UniString (zone.zone.roomNoStr);
        existing.area.outline = std::move (polygons[0].coords);
        for (size_t i = 1; i < polygons.size (); ++i) {
            existing.area.holes.push_back (std::move (polygons[i].coords));
        }
        zoneOverlapCache.zones.push_back (std::move (existing));
    }
}

// The first existing Zone of the new Zone's story that it would share area with.
// For a manual Zone the polygon is compared; for an automatic one only its reference
// point is known before creation, so it counts when that point is inside a Zone.
// ModifyZones passes the Zone it changes as excludedGuid, which is not compared with itself.
static const ExistingZone* FindOverlappingZone (const API_ZoneType& newZone, short floorInd, const ZoneArea& newZoneArea, const API_Guid& excludedGuid = APINULLGuid)
{
    RefreshZoneOverlapCache ();
    for (const ExistingZone& existing : zoneOverlapCache.zones) {
        if (existing.floorInd != floorInd || existing.guid == excludedGuid) {
            continue;
        }
        const bool overlaps = newZone.manual
            ? DoZonesOverlap (newZoneArea, existing.area)
            : IsStrictlyInsideZone (existing.area, newZone.refPos);
        if (overlaps) {
            return &existing;
        }
    }
    return nullptr;
}

// After ModifyZones has changed the polygon of a Zone, the later items of the same call
// are compared with its new polygon, not with the one cached before the change.
static void UpdateZoneOverlapCacheArea (const API_Guid& guid, const ZoneArea& area)
{
    for (ExistingZone& existing : zoneOverlapCache.zones) {
        if (existing.guid == guid) {
            existing.area = area;
            return;
        }
    }
}

static GS::ObjectState CreateZoneOverlapErrorResponse (const ExistingZone& existing, bool changedPolygon = false)
{
    const GS::UniString subject = changedPolygon ? "The new polygon of the zone" : "The new zone";
    const GS::UniString action = changedPolygon ? "change the polygon" : "create the new zone";
    return CreateErrorResponse (APIERR_BADPARS,
        subject + " would overlap the existing zone '" + existing.numberStr + " " + existing.name + "' (" + APIGuidToString (existing.guid) +
        ") on the same story, which Archicad answers with a modal warning dialog that blocks the API. "
        "Move or delete that zone first, or set allowOverlap to true to " + action + " anyway. "
        "Curved edges are compared by their chords, so a zone that only follows the curved edge of that zone may be refused although nothing overlaps; allowOverlap allows it too.");
}

} // namespace

// The levels of a zone: its height, base offset, top link and floor (finish) thickness.
// Applied by CreateZones (no mask) and, through ApplyZoneSettings, by ModifyZones and
// SetDetailsOfElements. Same rule as for walls and columns: an explicit height unlinks
// the top (relativeTopStory 0), and with a non-zero relativeTopStory Archicad computes
// the height from the story levels and topOffset. When both are given, relativeTopStory
// is applied last and wins. A topOffset given together with a new top link is written
// once more after the change, by ReapplyZoneTopOffset (#1869).
static GS::Optional<GS::ObjectState> ApplyZoneLevels (const GS::ObjectState& details, API_ZoneType& zone, API_Element* mask)
{
    double height = 0.0;
    if (details.Get ("height", height)) {
        if (height <= 0.0) {
            return CreateErrorResponse (APIERR_BADPARS, "height must be greater than 0.");
        }
        zone.roomHeight = height;
        zone.relativeTopStory = 0;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, roomHeight);
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, relativeTopStory);
        }
    }

    double bottomOffset = 0.0;
    if (details.Get ("bottomOffset", bottomOffset)) {
        zone.roomBaseLev = bottomOffset;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, roomBaseLev);
        }
    }

    double topOffset = 0.0;
    if (details.Get ("topOffset", topOffset)) {
        zone.roomTopOffset = topOffset;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, roomTopOffset);
        }
    }

    double relativeTopStory = 0.0;
    if (details.Get ("relativeTopStory", relativeTopStory)) {
        zone.relativeTopStory = static_cast<short> (relativeTopStory);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, relativeTopStory);
        }
    }

    double floorThickness = 0.0;
    if (details.Get ("floorThickness", floorThickness)) {
        if (floorThickness < 0.0) {
            return CreateErrorResponse (APIERR_BADPARS, "floorThickness must not be negative.");
        }
        zone.roomFlThick = floorThickness;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET ((*mask), API_ZoneType, roomFlThick);
        }
    }

    return {};
}

// Reported live (#1869): when one ACAPI_Element_Change links the top of a zone with an
// explicit height to a story (relativeTopStory 0 -> non-zero) and also sets roomTopOffset,
// Archicad keeps the zone's top where it was and computes roomTopOffset from it, so the
// given topOffset is lost. The same topOffset in a change of its own is applied, so it is
// written again once the link is in place.
GSErrCode ReapplyZoneTopOffset (const GS::ObjectState& details, const API_Guid& zoneGuid)
{
    double topOffset = 0.0;
    if (!details.Contains ("relativeTopStory") || !details.Get ("topOffset", topOffset)) {
        return NoError;
    }

    API_Element element = {};
    element.header.guid = zoneGuid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }
    if (element.zone.relativeTopStory == 0 || std::fabs (element.zone.roomTopOffset - topOffset) < 1e-9) {
        return NoError;
    }

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    element.zone.roomTopOffset = topOffset;
    ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, roomTopOffset);
    return ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
}

GS::Optional<GS::ObjectState> ApplyZoneSettings (const GS::ObjectState& details, API_Element& element, API_Element& mask)
{
    if (details.Contains ("name")) {
        if (!SetUCharProperty (&details, "name", element.zone.roomName)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid name parameter.");
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, roomName);
    }

    if (details.Contains ("numberStr")) {
        if (!SetUCharProperty (&details, "numberStr", element.zone.roomNoStr)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid numberStr parameter.");
        }
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, roomNoStr);
    }

    const GS::ObjectState* categoryAttributeId = details.Get ("categoryAttributeId");
    if (categoryAttributeId != nullptr) {
        // Refused instead of silently setting an invalid category, as CreateZones would.
        API_Attribute category = {};
        category.header.typeID = API_ZoneCatID;
        category.header.guid = GetGuidFromObjectState (*categoryAttributeId);
        if (category.header.guid == APINULLGuid || ACAPI_Attribute_Get (&category) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "No Zone Category attribute was found for categoryAttributeId.");
        }
        element.zone.catInd = category.header.index;
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, catInd);
    }

    const GS::ObjectState* stampPosition = details.Get ("stampPosition");
    if (stampPosition != nullptr) {
        element.zone.pos = Get2DCoordinateFromObjectState (*stampPosition);
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, pos);
    }
    if (details.Get ("stampAngle", element.zone.stampAngle)) {
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, stampAngle);
    }
    if (details.Get ("fixedStampAngle", element.zone.fixedAngle)) {
        ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, fixedAngle);
    }

    // The same helper and key names as CreateZones.
    const GS::Optional<GS::UniString> contourAndFillError = ApplyZoneContourAndCoverFill (details, element.zone, &mask);
    if (contourAndFillError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *contourAndFillError);
    }
    const GS::Optional<GS::UniString> surfaceError = ApplyZoneSurface (details, element.zone, &mask);
    if (surfaceError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *surfaceError);
    }

    const GS::Optional<GS::UniString> displayError = ApplyZoneDisplayStampPenAndReduction (details, element.zone, &mask);
    if (displayError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *displayError);
    }

    return ApplyZoneLevels (details, element.zone, &mask);
}

CreateZonesCommand::CreateZonesCommand () :
    CreateElementsCommandBase ("CreateZones", API_ZoneID, "zonesData")
{
}

// An automatic zone - one placed by geometry.referencePosition - is found around its
// reference point on the story that is ACTIVE in the floor plan, not on the story its
// floorIndex names: with any other story active ACAPI_Element_Create answers APIERR_GENERAL
// ("Failed to create new Zone"), and the very same call succeeds once that story is
// activated with ChangeWindow (#853). A manual zone is created on its floorIndex from
// any story.
//
// So the items whose automatic zone names a story other than the active one are created
// with that story made active (APIStory_GoTo, which is what ChangeWindow and SetStories
// use), and the previously active story is put back afterwards. The story is switched
// outside the undoable command, as those two commands do, so the items are created in
// groups of one story each - one undoable command per group - and the responses are put
// back in the order of the request. A call with no such item runs exactly as before.
GS::ObjectState CreateZonesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const
{
    // The overlap pre-check (see SetTypeSpecificParameters) caches the project's Zones
    // for the duration of this call only, across all the story groups below: the Zones
    // created by an earlier group are picked up by the next refresh.
    zoneOverlapCache = ZoneOverlapCache {};
    const GS::OnExit cacheGuard ([] () { zoneOverlapCache = ZoneOverlapCache {}; });

    GS::Array<GS::ObjectState> zonesData;
    parameters.Get (arrayFieldName, zonesData);

    API_StoryInfo storyInfo = {};
    const GSErrCode storyErr = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
    if (storyErr != NoError) {
        return CreateElementsCommandBase::Execute (parameters, processControl);
    }
    const short originalActStory = storyInfo.actStory;

    // The story each item needs active: its floorIndex for an automatic zone, the active
    // story for everything else (a manual zone, or an automatic one without floorIndex,
    // which stays on the story of the Zone tool defaults).
    std::vector<short> itemStories;
    std::vector<short> storiesInOrder;
    itemStories.reserve (zonesData.GetSize ());
    for (const GS::ObjectState& data : zonesData) {
        short story = originalActStory;
        const GS::ObjectState* geometry = data.Get ("geometry");
        if (geometry != nullptr && geometry->Get ("referencePosition") != nullptr) {
            data.Get ("floorIndex", story);
        }
        itemStories.push_back (story);
        if (std::find (storiesInOrder.begin (), storiesInOrder.end (), story) == storiesInOrder.end ()) {
            storiesInOrder.push_back (story);
        }
    }

    const bool needsStorySwitch = std::any_of (itemStories.begin (), itemStories.end (),
        [&] (short story) { return story != originalActStory; });
    if (!needsStorySwitch) {
        return CreateElementsCommandBase::Execute (parameters, processControl);
    }

    short activeStory = originalActStory;
    const GS::OnExit restoreActiveStory ([&] () {
        if (activeStory != originalActStory) {
            API_StoryCmdType storyCmd = {};
            storyCmd.action = APIStory_GoTo;
            storyCmd.index  = originalActStory;
            ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        }
    });

    std::vector<GS::ObjectState> results (zonesData.GetSize ());
    for (const short story : storiesInOrder) {
        GS::ObjectState groupParameters;
        const auto& groupItems = groupParameters.AddList<GS::ObjectState> ("zonesData");
        std::vector<size_t> groupIndices;
        for (size_t i = 0; i < itemStories.size (); ++i) {
            if (itemStories[i] == story) {
                groupItems (zonesData[static_cast<GS::UIndex> (i)]);
                groupIndices.push_back (i);
            }
        }

        if (story != activeStory) {
            API_StoryCmdType storyCmd = {};
            storyCmd.action = APIStory_GoTo;
            storyCmd.index  = story;
            const GSErrCode err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                // Archicad would answer the create with the bare "Failed to create new Zone"
                // of the issue; say what it actually needs instead.
                const GS::UniString message = GS::UniString::Printf (
                    "Failed to activate story %d in the floor plan, where an automatic zone is found around its referencePosition; "
                    "see GetStories for the valid story indices, or activate the story with ChangeWindow first.",
                    static_cast<int> (story));
                for (const size_t i : groupIndices) {
                    results[i] = CreateErrorResponse (err, message);
                }
                continue;
            }
            activeStory = story;
        }

        const GS::ObjectState groupResponse = CreateElementsCommandBase::Execute (groupParameters, processControl);
        GS::Array<GS::ObjectState> groupElements;
        groupResponse.Get ("elements", groupElements);
        for (size_t k = 0; k < groupIndices.size (); ++k) {
            results[groupIndices[k]] = k < groupElements.GetSize ()
                ? groupElements[static_cast<GS::UIndex> (k)]
                : CreateErrorResponse (APIERR_GENERAL, "Failed to create new Zone");
        }
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const GS::ObjectState& result : results) {
        elements (result);
    }
    return response;
}

GS::Optional<GS::UniString> CreateZonesCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "floorIndex": {
                        "type": "number",
                        "description" : "Optional story index of the new zone. An automatic zone (geometry.referencePosition) is found around its reference point on this story, which is made active in the floor plan while the zone is created; the previously active story is restored afterwards."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "minimum": 1,
                        "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
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
                    "contourLine": {
                        "$ref": "#/ZoneContourLine",
                        "description": "Optional contour line of the zone. Fields that are left out keep the value of the favorite or of the Zone tool defaults."
                    },
                    "coverFill": {
                        "$ref": "#/ZoneCoverFill",
                        "description": "Optional cover fill of the zone. Fields that are left out keep the value of the favorite or of the Zone tool defaults."
                    },
                    "surface": {
                        "$ref": "#/ZoneSurface",
                        "description": "Optional 3D surface of the zone (the Model panel). Fields that are left out keep the value of the favorite or of the Zone tool defaults."
                    },
                    "polygonDisplay": {
                        "$ref": "#/ZonePolygonDisplay",
                        "description": "Optional zone polygon display: Gross uses the reference line of the bounding walls, Net their inner edges. Only has an effect for a zone whose boundary is the reference line of the walls. When omitted, the value of the favorite or of the Zone tool defaults is kept."
                    },
                    "stampPen": {
                        "$ref": "#/ZoneStampPen",
                        "description": "Optional pen of the zone stamp. Fields that are left out keep the value of the favorite or of the Zone tool defaults."
                    },
                    "reducePercent": {
                        "$ref": "#/ZoneReducePercent",
                        "description": "Optional percentage the calculated area of the zone is reduced by; 0 means no reduction. When omitted, the value of the favorite or of the Zone tool defaults is kept."
                    },
                    "allowOverlap": {
                        "type": "boolean",
                        "description" : "Optional, defaults to false. Archicad answers a new zone whose area overlaps an existing zone on the same story with a modal warning dialog that blocks the API until it is dismissed, so by default such an item fails with an error naming the overlapping zone and no element is created for it. Set it to true to skip this check and create the zone anyway; the dialog may then appear. Zones that only touch (a shared edge or corner) do not overlap; curved edges are checked by their chords, and an automatic zone counts as overlapping when its referencePosition lies inside an existing zone."
                    },

                    "height": {
                        "type": "number",
                        "exclusiveMinimum": 0.0,
                        "description" : "Optional height of the zone from its bottom. Sets relativeTopStory to 0 (explicit height); when omitted, the height and top link of the favorite or of the Zone tool defaults are kept. Do not combine with relativeTopStory."
                    },
                    "bottomOffset": {
                        "type": "number",
                        "description" : "Optional base level of the zone relative to its home story level."
                    },
                    "topOffset": {
                        "type": "number",
                        "description" : "Optional offset of the top of the zone from the linked top story level. Only has an effect when relativeTopStory is non-zero."
                    },
                    "relativeTopStory": {
                        "type": "number",
                        "description" : "Optional top link: non-zero links the top of the zone to another story, relative to its home story (1 is the story above), instead of an explicit height. Do not combine with height."
                    },
                    "floorThickness": {
                        "type": "number",
                        "minimum": 0.0,
                        "description" : "Optional thickness of the floor finish (subfloor thickness) of the zone."
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
})";
}

GS::Optional<GS::ObjectState> CreateZonesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorIndex", element.header.floorInd);

    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    const API_Guid categoryAttrGuid = GetGuidFromArrayItem ("categoryAttributeId", parameters);
    if (categoryAttrGuid != APINULLGuid) {
        element.zone.catInd = GetAttributeIndexFromGuid (API_ZoneCatID, categoryAttrGuid);
    }

    if (!SetUCharProperty (&parameters, "name", element.zone.roomName)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing name parameter.");
    }

    if (!SetUCharProperty (&parameters, "numberStr", element.zone.roomNoStr)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing numberStr parameter.");
    }

    GS::ObjectState geometry;
    if (!parameters.Get ("geometry", geometry)) {
        return CreateErrorResponse (APIERR_BADPARS, "geometry parameter is missing.");
    }

    GS::ObjectState stampPosition;
    parameters.Get ("stampPosition", stampPosition);
    parameters.Get ("stampAngle", element.zone.stampAngle);

    bool fixedAngle = false;
    parameters.Get ("fixedStampAngle", fixedAngle);
    element.zone.fixedAngle = fixedAngle;

    const GS::Optional<GS::UniString> contourAndFillError = ApplyZoneContourAndCoverFill (parameters, element.zone, nullptr);
    if (contourAndFillError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *contourAndFillError);
    }
    const GS::Optional<GS::UniString> surfaceError = ApplyZoneSurface (parameters, element.zone, nullptr);
    if (surfaceError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *surfaceError);
    }

    const GS::Optional<GS::UniString> displayError = ApplyZoneDisplayStampPenAndReduction (parameters, element.zone, nullptr);
    if (displayError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, *displayError);
    }

    // Filled for a manual zone; the overlap pre-check at the end compares it with the
    // zones already on the story (see the comment above the ZoneArea helpers).
    ZoneArea newZoneArea;

    auto levelsError = ApplyZoneLevels (parameters, element.zone, nullptr);
    if (levelsError.HasValue ()) {
        return levelsError;
    }

    GS::ObjectState referencePosition;
    if (geometry.Get ("referencePosition", referencePosition)) {
        element.zone.manual = false;

        element.zone.refPos = Get2DCoordinateFromObjectState (referencePosition);

        element.zone.pos = stampPosition.IsEmpty() ? element.zone.refPos : Get2DCoordinateFromObjectState (stampPosition);
    } else {
        element.zone.manual = true;

        GS::Array<GS::ObjectState> polygonCoordinates;
        GS::Array<GS::ObjectState> polygonArcs;
        GS::Array<GS::ObjectState> holes;
        if (!geometry.Get ("polygonCoordinates", polygonCoordinates)) {
            return CreateErrorResponse (APIERR_BADPARS, "polygonCoordinates parameter is missing in geometry.");
        }

        geometry.Get ("polygonArcs", polygonArcs);
        geometry.Get ("holes", holes);
        if (polygonCoordinates.GetSize () < 3) {
            return CreateErrorResponse (APIERR_BADPARS, "'polygonCoordinates' must contain at least 3 coordinates.");
        }
        if (IsSame2DCoordinate (polygonCoordinates.GetFirst (), polygonCoordinates.GetLast ())) {
            polygonCoordinates.Pop ();
        }
        auto holesError = ValidateHoles (holes);
        if (holesError.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, holesError.Get ());
        }
        element.zone.poly.nCoords	= polygonCoordinates.GetSize() + 1;
        element.zone.poly.nSubPolys	= 1;
        element.zone.poly.nArcs		= polygonArcs.GetSize ();
        newZoneArea.outline = GetRingFromObjectStates (polygonCoordinates);

        for (const GS::ObjectState& hole : holes) {
            GS::Array<GS::ObjectState> holePolygonOutline;
            GS::Array<GS::ObjectState> holePolygonArcs;
            if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
                element.zone.poly.nCoords += holePolygonOutline.GetSize () + 1;
                ++element.zone.poly.nSubPolys;
                element.zone.poly.nArcs += holePolygonArcs.GetSize ();
                newZoneArea.holes.push_back (GetRingFromObjectStates (holePolygonOutline));
            }
        }

        memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((element.zone.poly.nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
        memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((element.zone.poly.nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
        memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (element.zone.poly.nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));

        Int32 iCoord = 1;
        Int32 iArc = 0;
        Int32 iPends = 1;
        AddPolyToMemo(polygonCoordinates,
                      polygonArcs,
                      iCoord,
                      iArc,
                      iPends,
                      memo);

        for (const GS::ObjectState& hole : holes) {
            GS::Array<GS::ObjectState> holePolygonOutline;
            GS::Array<GS::ObjectState> holePolygonArcs;
            if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
                AddPolyToMemo (holePolygonOutline,
                              holePolygonArcs,
                              iCoord,
                              iArc,
                              iPends,
                              memo);
            }
        }

        element.zone.pos = stampPosition.IsEmpty() ? (*memo.coords)[1] : Get2DCoordinateFromObjectState (stampPosition);
    }

    // Fail an item that would overlap a zone of the same story before ACAPI_Element_Create
    // opens Archicad's modal overlap warning, which blocks the API until someone clicks it
    // (#822). The zones created by the earlier items of the same call are already in the
    // project, so they are compared too.
    bool allowOverlap = false;
    parameters.Get ("allowOverlap", allowOverlap);
    if (!allowOverlap) {
        const ExistingZone* overlapped = FindOverlappingZone (element.zone, element.header.floorInd, newZoneArea);
        if (overlapped != nullptr) {
            return CreateZoneOverlapErrorResponse (*overlapped);
        }
    }

    return {};
}

// The new polygon of a manual zone for ModifyZones: the same geometry (polygonCoordinates,
// polygonArcs, holes) and the same checks as a manual zone of CreateZones. The memo is built
// as for a Hatch, also a closed polygon with holes whose memo is changed in place
// (BuildHatchPolygonMemo); the arcs are indexed the same way as in CreateZones.
static GS::Optional<GS::UniString> BuildZonePolygonMemo (const GS::ObjectState& geometry, API_ZoneType& zone, API_ElementMemo& memo, ZoneArea& area)
{
    if (geometry.Get ("referencePosition") != nullptr) {
        return GS::UniString ("geometry.referencePosition cannot be changed: an automatic zone is placed around its reference point when it is created. "
            "Give the polygon of a manual zone (polygonCoordinates, polygonArcs, holes), or recreate the zone with CreateZones.");
    }

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> polygonArcs;
    GS::Array<GS::ObjectState> holes;
    if (!geometry.Get ("polygonCoordinates", polygonCoordinates)) {
        return GS::UniString ("polygonCoordinates parameter is missing in geometry.");
    }
    geometry.Get ("polygonArcs", polygonArcs);
    geometry.Get ("holes", holes);

    if (polygonCoordinates.GetSize () >= 2 && IsSame2DCoordinate (polygonCoordinates.GetFirst (), polygonCoordinates.GetLast ())) {
        polygonCoordinates.Pop ();
    }
    if (polygonCoordinates.GetSize () < 3) {
        return GS::UniString ("'polygonCoordinates' must contain at least 3 different coordinates.");
    }
    const GS::Optional<GS::UniString> holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return holesError;
    }

    area.outline = GetRingFromObjectStates (polygonCoordinates);
    area.holes.clear ();
    for (const GS::ObjectState& hole : holes) {
        GS::Array<GS::ObjectState> holePolygonOutline;
        GS::Array<GS::ObjectState> holePolygonArcs;
        if (GetHoleGeometry (hole, holePolygonOutline, holePolygonArcs)) {
            area.holes.push_back (GetRingFromObjectStates (holePolygonOutline));
        }
    }

    if (!BuildHatchPolygonMemo (polygonCoordinates, polygonArcs, holes, zone.poly, memo)) {
        return GS::UniString ("Out of memory while building the polygon of the zone.");
    }
    return {};
}

ModifyZonesCommand::ModifyZonesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyZonesCommand::GetName () const
{
    return "ModifyZones";
}

GS::Optional<GS::UniString> ModifyZonesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "zonesWithDetails": {
                "type": "array",
                "description": "Array of Zone elements to modify, with the fields to change. Only the given fields are changed and the omitted fields are kept; the polygon is kept unless geometry is given. The same fields as the ZoneSettings of SetDetailsOfElements, plus geometry and allowOverlap.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "name": { "type": "string", "description": "Name of the zone." },
                        "numberStr": { "type": "string", "description": "Zone number." },
                        "categoryAttributeId": { "$ref": "#/AttributeId", "description": "The identifier of the zone category attribute." },
                        "stampPosition": { "$ref": "#/Coordinate2D", "description": "Position of the origin of the zone stamp." },
                        "stampAngle": { "type": "number", "description": "Rotation angle of the zone stamp in radians." },
                        "fixedStampAngle": { "type": "boolean", "description": "If true, the zone stamp angle remains fixed when the element is rotated." },
                        "height": { "type": "number", "exclusiveMinimum": 0.0, "description": "Height of the zone from its bottom. Sets relativeTopStory to 0 (explicit height), so do not combine it with relativeTopStory in the same call: whichever is applied last wins." },
                        "bottomOffset": { "type": "number", "description": "Base level of the zone relative to its home story level." },
                        "topOffset": { "type": "number", "description": "Offset of the top of the zone from the linked top story level. Only has an effect when relativeTopStory is non-zero." },
                        "relativeTopStory": { "type": "number", "description": "Non-zero links the top of the zone to another story, relative to its home story (1 is the story above), instead of an explicit height. Do not set together with height in the same call, see the note on height." },
                        "floorThickness": { "type": "number", "minimum": 0.0, "description": "Thickness of the floor finish (subfloor thickness) of the zone." },
                        "contourLine": { "$ref": "#/ZoneContourLine", "description": "Contour line of the zone. Only the given fields are changed." },
                        "coverFill": { "$ref": "#/ZoneCoverFill", "description": "Cover fill of the zone. Only the given fields are changed." },
                        "surface": { "$ref": "#/ZoneSurface", "description": "3D surface of the zone (the Model panel). Only the given fields are changed." },
                        "polygonDisplay": { "$ref": "#/ZonePolygonDisplay" },
                        "stampPen": { "$ref": "#/ZoneStampPen", "description": "Pen of the zone stamp. Only the given fields are changed." },
                        "reducePercent": { "$ref": "#/ZoneReducePercent" },
                        "geometry": { "$ref": "#/ManualZoneGeometry", "description": "Optional new polygon of a manual zone (isManual true in GetDetailsOfElements): its outline, arcs and holes, the same geometry as a manual zone of CreateZones. The whole polygon is replaced and the zone keeps its GUID, properties and other settings; the stamp stays where it is unless stampPosition is given too. Refused for an automatic zone, whose polygon Archicad computes from its boundaries (see UpdateZones)." },
                        "allowOverlap": { "type": "boolean", "description": "Optional, defaults to false, only used with geometry. As in CreateZones: a new polygon that would overlap another zone on the same story fails with an error naming that zone instead of risking Archicad's modal overlap warning, which blocks the API. Set it to true to change the polygon anyway." }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["zonesWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyZonesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ModifyZonesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> zonesWithDetails;
    parameters.Get ("zonesWithDetails", zonesWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (zonesWithDetails);

    // The overlap check of a new polygon (geometry) reads the project's Zones once per call,
    // as CreateZones does; see ZoneOverlapCache.
    zoneOverlapCache = ZoneOverlapCache {};
    const GS::OnExit cacheGuard ([] () { zoneOverlapCache = ZoneOverlapCache {}; });

    ACAPI_CallUndoableCommand ("ModifyZones", [&] () -> GSErrCode {
        for (const GS::ObjectState& item : zonesWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_ZoneID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Zone."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);
            auto settingsError = ApplyZoneSettings (item, element, mask);
            if (settingsError.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*settingsError));
                continue;
            }

            // Without geometry no memo is passed, so the polygon is kept as it is.
            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            UInt64 memoMask = 0;
            ZoneArea newZoneArea;
            const GS::ObjectState* geometry = item.Get ("geometry");
            if (geometry != nullptr) {
                if (!element.zone.manual) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS,
                        "The polygon of an automatic zone cannot be given: Archicad computes it from the zone's boundaries (see UpdateZones). Only a manual zone (isManual true in GetDetailsOfElements) takes geometry."));
                    continue;
                }
                const GS::Optional<GS::UniString> geometryError = BuildZonePolygonMemo (*geometry, element.zone, memo, newZoneArea);
                if (geometryError.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, *geometryError));
                    continue;
                }

                // As in CreateZones (#822): fail before Archicad can open its modal overlap warning.
                bool allowOverlap = false;
                item.Get ("allowOverlap", allowOverlap);
                if (!allowOverlap) {
                    const ExistingZone* overlapped = FindOverlappingZone (element.zone, element.header.floorInd, newZoneArea, element.header.guid);
                    if (overlapped != nullptr) {
                        executionResults (CreateFailedExecutionResult (CreateZoneOverlapErrorResponse (*overlapped, true)));
                        continue;
                    }
                }

                ACAPI_ELEMENT_MASK_SET (mask, API_ZoneType, poly);
                memoMask = APIMemoMask_Polygon;
            }

            err = ACAPI_Element_Change (&element, &mask, memoMask != 0 ? &memo : nullptr, memoMask, true);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, geometry != nullptr ? "Failed to modify the zone or to change its polygon." : "Failed to modify the zone."));
                continue;
            }
            if (geometry != nullptr) {
                UpdateZoneOverlapCacheArea (element.header.guid, newZoneArea);
            }

            err = ReapplyZoneTopOffset (item, element.header.guid);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "The zone was modified, but Archicad did not apply topOffset."));
                continue;
            }

            changeReporter.MarkChanged (element.header.guid);
            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}

// Shared "line-family settings" properties for Line/PolyLine/Arc/Circle/Spline creation
// (API_LineType/API_PolyLineType/API_ArcType/API_SplineType all share this exact shape).
// NOTE: no penWeight field here - confirmed live that a raw penWeight override does not
// reliably take visible effect in Archicad; assign a linePenIndex whose pen already has the
// desired weight in the project's pen table instead.
static const char* LineFamilySettingsSchemaProperties = R"(
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
                    })";

// The arrowheads of Line/PolyLine/Arc/Spline creation (not Circle, which has no ends).
static const char* LineArrowsSchemaProperties = R"(
                    "startArrow": {
                        "$ref": "#/LineArrowhead",
                        "description": "Optional arrowhead at the start. Given, it is shown unless its visible is false; by default the tool's (or the favorite's) arrowhead is kept."
                    },
                    "endArrow": {
                        "$ref": "#/LineArrowhead",
                        "description": "Optional arrowhead at the end. Given, it is shown unless its visible is false; by default the tool's (or the favorite's) arrowhead is kept."
                    },)";

static GS::Optional<GS::ObjectState> SetLineArrowsParameters (const GS::ObjectState& parameters, const LineArrows& arrows)
{
    bool changed = false;
    const GS::Optional<GS::UniString> error = ApplyLineArrows (parameters, arrows, changed);
    if (error.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, error.Get ());
    }
    return {};
}

static void SetLineFamilySettingsParameters (const GS::ObjectState& parameters, API_ExtendedPenType& linePen, API_AttributeIndex& ltypeInd, bool& roomSeparator)
{
    short penIndex = 0;
    if (parameters.Get ("linePenIndex", penIndex)) {
        linePen.penIndex = penIndex;
        linePen.colorOverridePenIndex = 0;
    }
    const GS::ObjectState* lineTypeId = parameters.Get ("lineTypeId");
    if (lineTypeId != nullptr) {
        ltypeInd = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (*lineTypeId));
    }
    parameters.Get ("roomSeparator", roomSeparator);
}

CreatePolylinesCommand::CreatePolylinesCommand () :
    CreateElementsCommandBase ("CreatePolylines", API_PolyLineID, "polylinesData")
{
}

GS::Optional<GS::UniString> CreatePolylinesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new polyline. Polylines have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no polyline. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
                        "description" : "Line type attribute index of the polyline contour. Optional parameter, by default the current line type is used. Ignored when lineTypeId is given."
                    },
                    "lineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type attribute of the polyline contour, as CreateLineElements and SetDetailsOfElements take it. Takes precedence over lineTypeIndex. By default the current line type is used."
                    },
                    "penWeightMm": {
                        "type": "number",
                        "description" : "Optional pen weight override in mm."
                    },
                    "roomSeparator": {
                        "type": "boolean",
                        "description": "Is this a zone boundary line? Optional, defaults to false."
                    },
                    "drawSegmentMode": {
                        "$ref": "#/PolylineDrawSegmentMode"
                    },)") + LineArrowsSchemaProperties + R"(
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
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "polylinesData"
    ]
})";
}

static void AddPolyToMemo (const GS::Array<GS::ObjectState>& coordinates,
                           const GS::Array<GS::ObjectState>& arcs,
                           API_Polygon&                      poly,
                           API_ElementMemo& 				 memo)
{
    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);
    poly.nCoords	= coordinates.GetSize();
    poly.nSubPolys	= 1;
    poly.nArcs		= polyArcs.GetSize ();

    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((poly.nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.pends = reinterpret_cast<Int32**> (BMAllocateHandle ((poly.nSubPolys + 1) * sizeof (Int32), ALLOCATE_CLEAR, 0));
    memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (poly.nArcs * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));

    Int32 iCoord = 0;
    for (const GS::ObjectState& c : coordinates) {
        (*memo.coords)[++iCoord] = Get2DCoordinateFromObjectState (c);
    }
    (*memo.pends)[1] = iCoord;

    Int32 iArc = 0;
    for (const API_PolyArc& a : polyArcs) {
        (*memo.parcs)[iArc++] = a;
    }
}

GS::Optional<GS::ObjectState> CreatePolylinesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories&, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    short linePenIndex = 0;
    if (parameters.Get ("linePenIndex", linePenIndex) && linePenIndex > 0) {
        element.polyLine.linePen.penIndex = linePenIndex;
        element.polyLine.linePen.colorOverridePenIndex = 0;
    }

    const GS::ObjectState* lineTypeId = parameters.Get ("lineTypeId");
    Int32 lineTypeIndex = 0;
    if (lineTypeId != nullptr) {
        element.polyLine.ltypeInd = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (*lineTypeId));
    } else if (parameters.Get ("lineTypeIndex", lineTypeIndex) && lineTypeIndex > 0) {
        element.polyLine.ltypeInd = ACAPI_CreateAttributeIndex (lineTypeIndex);
    }

    double penWeightMm = API_DefPenWeigth;
    if (parameters.Get ("penWeightMm", penWeightMm)) {
        element.polyLine.penWeight = penWeightMm;
    }

    parameters.Get ("roomSeparator", element.polyLine.roomSeparator);

    GS::UniString drawSegmentMode;
    if (parameters.Get ("drawSegmentMode", drawSegmentMode) &&
        !PolylineDrawSegmentModeFromString (drawSegmentMode, element.polyLine.drawSegmentMode)) {
        return CreateErrorResponse (APIERR_BADPARS, "drawSegmentMode must be SegmentBySegment or Continuous.");
    }

    GS::Optional<GS::ObjectState> arrowsError = SetLineArrowsParameters (parameters, GetLineArrows (element.polyLine));
    if (arrowsError.HasValue ()) {
        return arrowsError;
    }

    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("arcs", arcs);

    AddPolyToMemo(coordinates,
                  arcs,
                  element.polyLine.poly,
                  memo);

    return {};
}

CreateLineElementsCommand::CreateLineElementsCommand () :
    CreateElementsCommandBase ("CreateLineElements", API_LineID, "linesData")
{
}

GS::Optional<GS::UniString> CreateLineElementsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new line. Lines have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no line. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
                    },)") + LineArrowsSchemaProperties + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "begCoordinate",
                    "endCoordinate"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "linesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateLineElementsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState begCoordinate;
    parameters.Get ("begCoordinate", begCoordinate);
    element.line.begC = Get2DCoordinateFromObjectState (begCoordinate);

    GS::ObjectState endCoordinate;
    parameters.Get ("endCoordinate", endCoordinate);
    element.line.endC = Get2DCoordinateFromObjectState (endCoordinate);

    SetLineFamilySettingsParameters (parameters, element.line.linePen, element.line.ltypeInd, element.line.roomSeparator);

    return SetLineArrowsParameters (parameters, GetLineArrows (element.line));
}

CreateArcsCommand::CreateArcsCommand () :
    CreateElementsCommandBase ("CreateArcs", API_ArcID, "arcsData")
{
}

GS::Optional<GS::UniString> CreateArcsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new arc. Arcs have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no arc. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
                    "angle": {
                        "type": "number",
                        "description": "Optional angle of the ellipse's 'a' axis in radians. Defaults to 0.0."
                    },
                    "ratio": {
                        "type": "number",
                        "exclusiveMinimum": 0,
                        "description": "Optional 'a/b' ratio of the ellipse, as GetDetailsOfElements returns it. Defaults to 1.0, a circular arc."
                    },
                    "reflected": {
                        "type": "boolean",
                        "description": "Optional: is the ellipse reflected onto its smaller axis? By default the tool's (or the favorite's) setting is kept."
                    },)") + LineArrowsSchemaProperties + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius",
                    "begAngle",
                    "endAngle"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "arcsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateArcsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState origin;
    parameters.Get ("origin", origin);
    element.arc.origC = Get2DCoordinateFromObjectState (origin);

    parameters.Get ("radius", element.arc.r);
    element.arc.angle = 0.0;
    element.arc.ratio = 1.0;
    parameters.Get ("angle", element.arc.angle);
    parameters.Get ("ratio", element.arc.ratio);
    parameters.Get ("reflected", element.arc.reflected);
    parameters.Get ("begAngle", element.arc.begAng);
    parameters.Get ("endAngle", element.arc.endAng);

    SetLineFamilySettingsParameters (parameters, element.arc.linePen, element.arc.ltypeInd, element.arc.roomSeparator);

    return SetLineArrowsParameters (parameters, GetLineArrows (element.arc));
}

CreateCirclesCommand::CreateCirclesCommand () :
    CreateElementsCommandBase ("CreateCircles", API_CircleID, "circlesData")
{
}

GS::Optional<GS::UniString> CreateCirclesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new circle. Circles have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no circle. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
                    },)") + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "origin",
                    "radius"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "circlesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateCirclesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState origin;
    parameters.Get ("origin", origin);
    element.circle.origC = Get2DCoordinateFromObjectState (origin);

    parameters.Get ("radius", element.circle.r);
    element.circle.angle = 0.0;
    element.circle.ratio = 1.0;

    SetLineFamilySettingsParameters (parameters, element.circle.linePen, element.circle.ltypeInd, element.circle.roomSeparator);

    return {};
}

CreateHotspotsCommand::CreateHotspotsCommand () :
    CreateElementsCommandBase ("CreateHotspots", API_HotspotID, "hotspotsData")
{
}

GS::Optional<GS::UniString> CreateHotspotsCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new hotspot. Hotspots have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no hotspot. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "hotspotsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateHotspotsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::ObjectState position;
    parameters.Get ("position", position);
    element.hotspot.pos = Get2DCoordinateFromObjectState (position);

    parameters.Get ("height", element.hotspot.height);

    short penIndex = 0;
    if (parameters.Get ("penIndex", penIndex)) {
        element.hotspot.pen = penIndex;
    }

    return {};
}

// The image formats the Picture tool takes (API_PictureFormat), told apart by their first bytes.
enum class PictureFileFormat { Unknown, Bitmap, GIF, TIFF, JPEG, PNG };

static UInt32 ReadPictureUInt16 (const unsigned char* p, bool bigEndian)
{
    return bigEndian ? ((UInt32) p[0] << 8) | p[1] : ((UInt32) p[1] << 8) | p[0];
}

static UInt32 ReadPictureUInt32 (const unsigned char* p, bool bigEndian)
{
    return bigEndian
        ? ((UInt32) p[0] << 24) | ((UInt32) p[1] << 16) | ((UInt32) p[2] << 8) | p[3]
        : ((UInt32) p[3] << 24) | ((UInt32) p[2] << 16) | ((UInt32) p[1] << 8) | p[0];
}

// Reads the format and the size in pixels from the header of an image file. Returns false when
// the data is none of PNG, JPEG, GIF, TIFF and BMP or its size cannot be found. Only the header is
// read: the image itself is decoded by Archicad. The size is needed to keep the proportions when
// only the width or only the height is given.
static bool GetPictureFileInfo (const unsigned char* data, UInt64 size, PictureFileFormat& format, UInt32& width, UInt32& height)
{
    format = PictureFileFormat::Unknown;
    width = 0;
    height = 0;

    static const unsigned char pngSignature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    if (size >= 24 && memcmp (data, pngSignature, 8) == 0) {
        // The IHDR chunk comes first: its width and height follow the chunk length and type.
        format = PictureFileFormat::PNG;
        width = ReadPictureUInt32 (data + 16, true);
        height = ReadPictureUInt32 (data + 20, true);
    } else if (size >= 10 && (memcmp (data, "GIF87a", 6) == 0 || memcmp (data, "GIF89a", 6) == 0)) {
        format = PictureFileFormat::GIF;
        width = ReadPictureUInt16 (data + 6, false);
        height = ReadPictureUInt16 (data + 8, false);
    } else if (size >= 26 && data[0] == 'B' && data[1] == 'M') {
        format = PictureFileFormat::Bitmap;
        const UInt32 headerSize = ReadPictureUInt32 (data + 14, false);
        if (headerSize == 12) {
            // BITMAPCOREHEADER: 16-bit sizes.
            width = ReadPictureUInt16 (data + 18, false);
            height = ReadPictureUInt16 (data + 20, false);
        } else {
            // BITMAPINFOHEADER and later: signed 32-bit sizes, a negative height is a top-down bitmap.
            const Int32 signedWidth = (Int32) ReadPictureUInt32 (data + 18, false);
            const Int32 signedHeight = (Int32) ReadPictureUInt32 (data + 22, false);
            width = signedWidth < 0 ? 0 : (UInt32) signedWidth;
            height = signedHeight < 0 ? (UInt32) (-(Int64) signedHeight) : (UInt32) signedHeight;
        }
    } else if (size >= 4 && data[0] == 0xFF && data[1] == 0xD8) {
        format = PictureFileFormat::JPEG;
        // The size is in the first start-of-frame segment (SOF0-SOF15 but DHT, JPG and DAC).
        UInt64 pos = 2;
        while (pos + 4 <= size) {
            if (data[pos] != 0xFF) {
                return false;
            }
            const unsigned char marker = data[pos + 1];
            if (marker == 0xFF) {
                // Fill byte before a marker.
                ++pos;
                continue;
            }
            if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
                // Markers without a length.
                pos += 2;
                continue;
            }
            if (marker == 0xD9 || marker == 0xDA) {
                // End of image or start of scan before any frame header.
                return false;
            }
            const UInt32 length = ReadPictureUInt16 (data + pos + 2, true);
            if (length < 2) {
                return false;
            }
            if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
                if (pos + 9 > size) {
                    return false;
                }
                height = ReadPictureUInt16 (data + pos + 5, true);
                width = ReadPictureUInt16 (data + pos + 7, true);
                break;
            }
            pos += 2 + length;
        }
    } else if (size >= 8 && ((data[0] == 'I' && data[1] == 'I' && data[2] == 42 && data[3] == 0) ||
                             (data[0] == 'M' && data[1] == 'M' && data[2] == 0 && data[3] == 42))) {
        format = PictureFileFormat::TIFF;
        const bool bigEndian = data[0] == 'M';
        // ImageWidth (256) and ImageLength (257) of the first image file directory.
        const UInt64 ifdOffset = ReadPictureUInt32 (data + 4, bigEndian);
        if (ifdOffset + 2 > size) {
            return false;
        }
        const UInt32 entryCount = ReadPictureUInt16 (data + ifdOffset, bigEndian);
        for (UInt32 i = 0; i < entryCount; ++i) {
            const UInt64 entry = ifdOffset + 2 + (UInt64) i * 12;
            if (entry + 12 > size) {
                return false;
            }
            const UInt32 tag = ReadPictureUInt16 (data + entry, bigEndian);
            if (tag != 256 && tag != 257) {
                continue;
            }
            const UInt32 type = ReadPictureUInt16 (data + entry + 2, bigEndian);
            UInt32 value = 0;
            if (type == 3) {
                value = ReadPictureUInt16 (data + entry + 8, bigEndian);
            } else if (type == 4) {
                value = ReadPictureUInt32 (data + entry + 8, bigEndian);
            } else {
                return false;
            }
            if (tag == 256) {
                width = value;
            } else {
                height = value;
            }
        }
    } else {
        return false;
    }

    return width > 0 && height > 0;
}

static API_PictureFormat ToApiPictureFormat (PictureFileFormat format)
{
    switch (format) {
        case PictureFileFormat::Bitmap: return APIPictForm_Bitmap;
        case PictureFileFormat::GIF:    return APIPictForm_GIF;
        case PictureFileFormat::TIFF:   return APIPictForm_TIFF;
        case PictureFileFormat::JPEG:   return APIPictForm_JPEG;
        case PictureFileFormat::PNG:    return APIPictForm_PNG;
        default:                        return APIPictForm_Default;
    }
}

// Where the anchor point of a picture box of this size is, relative to the box's bottom left corner.
static API_Coord GetPictureAnchorOffset (API_AnchorID anchorPoint, double width, double height)
{
    API_Coord offset = { 0.0, 0.0 };
    if (anchorPoint == APIAnc_MT || anchorPoint == APIAnc_MM || anchorPoint == APIAnc_MB) {
        offset.x = width / 2.0;
    } else if (anchorPoint == APIAnc_RT || anchorPoint == APIAnc_RM || anchorPoint == APIAnc_RB) {
        offset.x = width;
    }
    if (anchorPoint == APIAnc_LM || anchorPoint == APIAnc_MM || anchorPoint == APIAnc_RM) {
        offset.y = height / 2.0;
    } else if (anchorPoint == APIAnc_LT || anchorPoint == APIAnc_MT || anchorPoint == APIAnc_RT) {
        offset.y = height;
    }
    return offset;
}

CreatePicturesCommand::CreatePicturesCommand () :
    CreateElementsCommandBase ("CreatePictures", API_PictureID, "picturesData")
{
}

GS::Optional<GS::UniString> CreatePicturesCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "picturesData": {
            "type": "array",
            "description": "Array of data to create Pictures.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Picture. Give the image either as a file (path) or as its bytes (imageData), not both.",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "The full path of a PNG, JPEG, GIF, TIFF or BMP image file. The image is copied into the project: the picture does not change when the file changes or is deleted later."
                    },
                    "imageData": {
                        "type": "string",
                        "description": "The PNG, JPEG, GIF, TIFF or BMP image file as base64 text, instead of path."
                    },
                    "name": {
                        "type": "string",
                        "description": "Optional name of the picture. By default the file name of path, or empty for imageData."
                    },
                    "position": {
                        "$ref": "#/Coordinate2D",
                        "description": "Where the anchor point of the picture is placed, in model units (on a layout in layout units)."
                    },
                    "anchorPoint": {
                        "$ref": "#/PictureAnchorPoint",
                        "description": "The point of the picture's box that is placed at position and that the picture is rotated around. BottomLeft by default."
                    },
                    "width": {
                        "type": "number",
                        "exclusiveMinimum": 0,
                        "description": "The width of the picture in model units. When only height is given, the width keeps the proportions of the image."
                    },
                    "height": {
                        "type": "number",
                        "exclusiveMinimum": 0,
                        "description": "The height of the picture in model units. When only width is given, the height keeps the proportions of the image."
                    },
                    "angle": {
                        "type": "number",
                        "description": "The rotation angle of the picture in radians, around its anchor point. 0 by default."
                    },
                    "mirrored": {
                        "type": "boolean",
                        "description": "Mirror the picture to its local vertical axis. false by default."
                    },
                    "transparent": {
                        "type": "boolean",
                        "description": "Draw the white pixels of the picture transparent. false by default."
                    },
                    "floorInd": {
                        "type": "number",
                        "description": "The identifier of the floor. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "minimum": 1,
                        "description": "Layer attribute index to place the picture on. Optional parameter, by default the layer of the Picture tool defaults is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "position"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the pictures in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the pictures are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "picturesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreatePicturesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    GS::UniString path;
    GS::UniString imageData;
    const bool hasPath = parameters.Get ("path", path) && !path.IsEmpty ();
    const bool hasImageData = parameters.Get ("imageData", imageData) && !imageData.IsEmpty ();
    if (hasPath == hasImageData) {
        return CreateErrorResponse (APIERR_BADPARS, "Give the image either as path or as imageData.");
    }

    // The picture is created from the bytes of the image file, as Graphisoft's Element_Test
    // example does it: the file content goes to memo.pictHdl and its format to storageFormat.
    GS::Array<char> bytes;
    GS::UniString name;
    if (hasPath) {
        IO::File file (IO::Location (path), IO::File::OnNotFound::Fail);
        if (file.GetStatus () != NoError || file.Open (IO::File::ReadMode) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "Failed to open the image file '" + path + "'.");
        }
        UInt64 length = 0;
        GSErrCode err = file.GetDataLength (&length);
        if (err == NoError && (length == 0 || length > 0x7FFFFFFF)) {
            err = APIERR_BADPARS;
        }
        if (err == NoError) {
            bytes.SetSize ((USize) length);
            USize read = 0;
            err = file.ReadBin (bytes.GetContent (), (USize) length, &read);
            if (err == NoError && read != (USize) length) {
                err = APIERR_GENERAL;
            }
        }
        file.Close ();
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to read the image file '" + path + "'.");
        }

        // The file name without its folder.
        UIndex nameStart = 0;
        for (UIndex i = 0; i < path.GetLength (); ++i) {
            if (path[i] == GS::UniChar ('/') || path[i] == GS::UniChar ('\\')) {
                nameStart = i + 1;
            }
        }
        name = path.GetSubstring (nameStart, path.GetLength () - nameStart);
    } else {
        bytes = Base64Converter::Decode (imageData);
        if (bytes.IsEmpty ()) {
            return CreateErrorResponse (APIERR_BADPARS, "imageData is not valid base64 text.");
        }
    }
    parameters.Get ("name", name);

    PictureFileFormat format = PictureFileFormat::Unknown;
    UInt32 pixelWidth = 0;
    UInt32 pixelHeight = 0;
    if (!GetPictureFileInfo (reinterpret_cast<const unsigned char*> (bytes.GetContent ()), bytes.GetSize (), format, pixelWidth, pixelHeight)) {
        return CreateErrorResponse (APIERR_BADPARS, "The image is not a PNG, JPEG, GIF, TIFF or BMP file, or its size cannot be read.");
    }

    double width = 0.0;
    double height = 0.0;
    const bool hasWidth = parameters.Get ("width", width);
    const bool hasHeight = parameters.Get ("height", height);
    if (!hasWidth && !hasHeight) {
        return CreateErrorResponse (APIERR_BADPARS, "Give the width, the height or both of the picture.");
    }
    if ((hasWidth && !(width > 0.0)) || (hasHeight && !(height > 0.0))) {
        return CreateErrorResponse (APIERR_BADPARS, "The width and the height of the picture must be greater than 0.");
    }
    if (!hasWidth) {
        width = height * pixelWidth / pixelHeight;
    } else if (!hasHeight) {
        height = width * pixelHeight / pixelWidth;
    }

    GS::ObjectState positionOS;
    parameters.Get ("position", positionOS);
    const API_Coord position = Get2DCoordinateFromObjectState (positionOS);

    GS::UniString anchorPointString ("BottomLeft");
    parameters.Get ("anchorPoint", anchorPointString);
    const API_AnchorID anchorPoint = AnchorIdFromString (anchorPointString, APIAnc_LB);

    // The box the picture fills, with its anchor point at position.
    const API_Coord anchorOffset = GetPictureAnchorOffset (anchorPoint, width, height);
    const double xMin = position.x - anchorOffset.x;
    const double yMin = position.y - anchorOffset.y;

    API_PictureType& picture = element.picture;
    // With usePixelSize false Archicad takes the size from the whole destBox; the pixel sizes are
    // ignored then, so they are only filled in (clamped to the short they are stored in).
    picture.usePixelSize = false;
    picture.directCreate = false;
    picture.destBox.xMin = xMin;
    picture.destBox.yMin = yMin;
    picture.destBox.xMax = xMin + width;
    picture.destBox.yMax = yMin + height;
    picture.pixelSizeX = static_cast<short> (GS::Min<UInt32> (pixelWidth, 32767));
    picture.pixelSizeY = static_cast<short> (GS::Min<UInt32> (pixelHeight, 32767));
    picture.anchorPoint = anchorPoint;
    picture.storageFormat = ToApiPictureFormat (format);
    picture.rotAngle = 0.0;
    parameters.Get ("angle", picture.rotAngle);
    picture.mirrored = false;
    parameters.Get ("mirrored", picture.mirrored);
    picture.transparent = false;
    parameters.Get ("transparent", picture.transparent);
    GS::ucscpy (picture.pictName, name.ToUStr (0, GS::Min (name.GetLength (), (USize) API_UniLongNameLen - 1)).Get ());

    if (memo.pictHdl != nullptr) {
        BMKillHandle ((GSHandle*) &memo.pictHdl);
    }
    memo.pictHdl = BMAllocateHandle ((GSSize) bytes.GetSize (), ALLOCATE_CLEAR, 0);
    if (memo.pictHdl == nullptr) {
        return CreateErrorResponse (APIERR_MEMFULL, "Not enough memory for the image.");
    }
    memcpy (*memo.pictHdl, bytes.GetContent (), bytes.GetSize ());

    return {};
}

CreateHatchesCommand::CreateHatchesCommand () :
    CreateElementsCommandBase ("CreateHatches", API_HatchID, "hatchesData")
{
}

GS::Optional<GS::UniString> CreateHatchesCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
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
                        "description": "The 2D coordinates of the hatch outline. The first point may be repeated at the end, as in the closed outline GetDetailsOfElements returns; that last point is then ignored.",
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
                    "holes": {
                        "$ref": "#/Holes2D",
                        "description": "Optional holes of the hatch, each with its polygonOutline and optional polygonArcs (arc indices are relative to the hole's own outline). The same format SetDetailsOfElements accepts and GetDetailsOfElements returns for hatches."
                    },
                    "contourPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the contour; 0 switches the contour off. By default the current pen is used."
                    },
                    "contourLineTypeId": {
                        "$ref": "#/AttributeId",
                        "description": "Optional line type of the contour. By default the current line type is used."
                    },
                    "fillPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the fill. By default the current pen is used."
                    },
                    "fillBackgroundPenIndex": {
                        "type": "integer",
                        "description": "Optional pen index for the fill background; 0 makes the background transparent and -1 gives it the window background color."
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
                        "description": "The percentage (1 to 100) of the hatch's area subtracted from the area of the zone it lies in (Subtract from zone area); a negative value switches it off."
                    },
                    "showArea": {
                        "type": "boolean"
                    },
                    "fillOrientation": {
                        "$ref": "#/HatchOrientation",
                        "description": "Optional orientation, origin and distortion of the fill pattern, as SetDetailsOfElements writes it. Omitted properties keep the default ones. A given origin detaches the fill from the project origin, and a Global fill becomes Rotated unless a type is given as well. Archicad ignores the orientation of Solid fills."
                    },
                    "fillCategory": {
                        "$ref": "#/HatchFillCategory",
                        "description": "Optional fill category: Drafting, Cut or Cover. By default the current one is used."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "coordinates"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "hatchesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateHatchesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("arcs", arcs);
    parameters.Get ("holes", holes);
    if (GetOpenHatchOutline (coordinates).GetSize () < 3) {
        return CreateErrorResponse (APIERR_BADPARS, "The hatch outline must contain at least 3 coordinates, not counting a last one that repeats the first.");
    }
    auto holesError = ValidateHoles (holes);
    if (holesError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, holesError.Get ());
    }

    // The same memo layout SetDetailsOfElements writes for a hatch with holes.
    if (!BuildHatchPolygonMemo (coordinates, arcs, holes, element.hatch.poly, memo)) {
        return CreateErrorResponse (APIERR_MEMFULL, "Failed to allocate the polygon of the hatch.");
    }

    short contourPenIndex = 0;
    if (parameters.Get ("contourPenIndex", contourPenIndex)) {
        element.hatch.contPen.penIndex = contourPenIndex;
        element.hatch.contPen.colorOverridePenIndex = 0;
    }
    // A given pen index clears the matching RGB override, which the tool defaults may carry,
    // so the pen actually shows. A given color sets the override and wins over the pen.
    short fillPenIndex = 0;
    if (parameters.Get ("fillPenIndex", fillPenIndex)) {
        element.hatch.fillPen.penIndex = fillPenIndex;
        element.hatch.fillPen.colorOverridePenIndex = 0;
        element.hatch.hatchFlags &= ~APIHatch_HasFgRGBColor;
    }
    if (parameters.Get ("fillBackgroundPenIndex", element.hatch.fillBGPen)) {
        element.hatch.hatchFlags &= ~APIHatch_HasBkgRGBColor;
    }
    if (GetColor (parameters, "foregroundColor", element.hatch.foregroundRGB)) {
        element.hatch.hatchFlags |= APIHatch_HasFgRGBColor;
    }
    if (GetColor (parameters, "backgroundColor", element.hatch.backgroundRGB)) {
        element.hatch.hatchFlags |= APIHatch_HasBkgRGBColor;
    }
    // The hatch type decides which of fillInd and buildingMaterial Archicad uses, so a given
    // attribute alone also switches the type; when both are given the default type is kept,
    // as SetDetailsOfElements does.
    const GS::ObjectState* fillId = parameters.Get ("fillId");
    const GS::ObjectState* buildingMaterialId = parameters.Get ("buildingMaterialId");
    if (fillId != nullptr) {
        element.hatch.fillInd = GetAttributeIndexFromGuid (API_FilltypeID, GetGuidFromObjectState (*fillId));
        if (buildingMaterialId == nullptr) {
            element.hatch.hatchType = API_FillHatch;
        }
    }
    if (buildingMaterialId != nullptr) {
        element.hatch.buildingMaterial = GetAttributeIndexFromGuid (API_BuildingMaterialID, GetGuidFromObjectState (*buildingMaterialId));
        if (fillId == nullptr) {
            element.hatch.hatchType = API_BuildingMaterialHatch;
        }
    }
    Int32 roomSpecial = 0;
    if (parameters.Get ("roomSpecial", roomSpecial)) {
        element.hatch.roomSpecial = (char) roomSpecial;
    }
    parameters.Get ("showArea", element.hatch.showArea);
    GS::ObjectState fillOrientationOs;
    if (parameters.Get ("fillOrientation", fillOrientationOs)) {
        ApplyHatchFillOrientation (fillOrientationOs, element.hatch.hatchOrientation);
    }
    GS::UniString fillCategory;
    if (parameters.Get ("fillCategory", fillCategory)) {
        HatchFillCategoryFromString (fillCategory, element.hatch.determination);
    }
    const GS::ObjectState* contourLineTypeId = parameters.Get ("contourLineTypeId");
    if (contourLineTypeId != nullptr) {
        element.hatch.ltypeInd = GetAttributeIndexFromGuid (API_LinetypeID, GetGuidFromObjectState (*contourLineTypeId));
    }

    return {};
}

CreateSplinesCommand::CreateSplinesCommand () :
    CreateElementsCommandBase ("CreateSplines", API_SplineID, "splinesData")
{
}

GS::Optional<GS::UniString> CreateSplinesCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new spline. Splines have no Element ID in Archicad 29: Archicad refuses it with error -2130313103 (APIERR_BADELEMENTTYPE, element has no info string), and the item then answers with that error and creates no spline. Leave it out unless your Archicad version keeps it (GetDetailsOfElements returns it as id)."
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
                    },)") + LineArrowsSchemaProperties + LineFamilySettingsSchemaProperties + R"(
                },
                "additionalProperties": false,
                "required": [
                    "coordinates"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "splinesData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateSplinesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    parameters.Get ("floorInd", element.header.floorInd);

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);
    parameters.Get ("closed", element.spline.closed);
    element.spline.autoSmooth = true; // bezierDirs handle-editing not supported by this command

    const Int32 nPoints = (Int32) coordinates.GetSize ();
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle (nPoints * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    if (memo.coords != nullptr) {
        for (Int32 i = 0; i < nPoints; ++i) {
            (*memo.coords)[i] = Get2DCoordinateFromObjectState (coordinates[i]);
        }
    }

    SetLineFamilySettingsParameters (parameters, element.spline.linePen, element.spline.ltypeInd, element.spline.roomSeparator);

    return SetLineArrowsParameters (parameters, GetLineArrows (element.spline));
}

// Shared schema builder for CreateObjects and CreateLamps.
//
// API_LampType is a typedef alias of API_ObjectType in APIdefs_Elements.h
// (verified across AC25..29), so the two commands differ only in name,
// elemTypeID, JSON array field name, and the singular noun / description
// used in the schema.
// The properties shared by CreateObjects/CreateLamps (on top of libraryPartName/coordinates) and
// ModifyObjects/ModifyLamps (all of these, nothing else) - kept as one string so the two schemas
// can't drift apart. lightColor/lightIsOn are only meaningful for Lamps, gated by `isLamp`.
static GS::UniString BuildObjectLampDetailFields (bool isLamp)
{
    GS::UniString fields = R"(
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
        "offset": { "$ref": "#/Coordinate2D", "description": "Offset of the symbol's origin from the insertion point. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default hotspot offset regardless of what is sent) - same class of read-only-in-practice field as Morph's bodyType/edgeType." },
        "useFixedAngle": { "type": "boolean", "description": "Use a fixed rotation angle. Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify." },
        "isAutoOnStoryVisibility": { "type": "boolean" },
        "visibility": {
            "$ref": "#/StoryVisibility"
        },
        "linkToSettings": {
            "$ref": "#/LinkToSettings"
        })";
    if (isLamp) {
        fields += R"(,
        "lightColor": { "$ref": "#/ColorRGB", "description": "Reported accurately on Get, but confirmed live that Archicad silently discards this value through both Create and Modify (Get always reports the library part's own default light color). lightIsOn (the on/off state, as opposed to the color) does not have this problem." },
        "lightIsOn": { "type": "boolean" })";
    }
    return fields;
}

// API_LampType is a typedef alias of API_ObjectType in APIdefs_Elements.h (verified across
// AC25..29), so Object and Lamp share one schema shape beyond the element-type-specific
// description strings.
//
// Built via plain GS::UniString concatenation rather than a single GS::UniString::Printf call -
// confirmed live (crash dumps in APICommandBridge.dll, ACCESS_VIOLATION) that feeding the large
// BuildObjectLampDetailFields blob through Printf's %s reliably crashed Archicad at add-on
// registration time (RegisterCommand<T> calls GetInputParametersSchema() at startup); isolated by
// bisection to the SIZE of that single %s argument, not its content (a 1-line stand-in placeholder
// through the same Printf call never crashed). Root cause not confirmed further (suspected fixed-size
// internal buffer in GS::UniString::Printf/PrintfFwd's va_arg handling) - concatenating instead
// avoids the vararg path entirely for the large piece.
static GS::UniString BuildLibraryPartBasedSchema (const char* arrayFieldName,
                                                  const char* elementSingularName,
                                                  const char* libraryPartNameDescription,
                                                  bool isLamp)
{
    GS::UniString schema = GS::UniString::Printf (R"({
        "type": "object",
        "properties": {
            "%s": {
                "type": "array",
                "description": "Array of data to create %ss.",
                "items": {
                    "type": "object",
                    "description": "The parameters of the new %s.",
                    "properties": {
                        "libraryPartName": {
                            "type": "string",
                            "description" : "%s"
                        },
                        "coordinates": {
                            "$ref": "#/Coordinate3D"
                        },
                        "dimensions": {
                            "$ref": "#/Dimensions3D"
                        },
                        )",
        arrayFieldName, elementSingularName, elementSingularName, libraryPartNameDescription);

    schema += BuildObjectLampDetailFields (isLamp);

    schema += GS::UniString::Printf (R"(,
                        "floorIndex": {
                            "type": "integer",
                            "description": "Optional floor index. If omitted, derived from the coordinate's z value."
                        },
                        "favoriteName": {
                            "type": "string",
                            "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                        },
                        "id": {
                            "type": "string",
                            "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                        },
                        "parameters": {
                            "type": "array",
                            "description": "Optional GDL parameters to set by name, after dimensions. The value is a number, a boolean or a string; array parameters are not supported. A or B also set the size. An unknown name or a value of the wrong type fails the item, and nothing is created for it.",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "name": { "type": "string" },
                                    "value": { "description": "A number or a boolean for a numeric parameter, a string for a String or Title parameter." }
                                },
                                "additionalProperties": false,
                                "required": ["name", "value"]
                            }
                        },
                        "layerIndex": {
                            "type": "integer",
                            "minimum": 1,
                            "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
                        }
                    },
                    "additionalProperties": false,
                    "required" : [
                        "libraryPartName",
                        "coordinates"
                    ]
                }
            },
            "databaseId": {
                "$ref": "#/DatabaseId",
                "description": "Optional database to place the elements in, e.g. a worksheet or a detail (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return), for a legend of symbols. The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are placed in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
            }
        },
        "additionalProperties": false,
        "required": [
            "%s"
        ]
    })", arrayFieldName);

    return schema;
}

CreateObjectsCommand::CreateObjectsCommand () :
    CreateElementsCommandBase ("CreateObjects", API_ObjectID, "objectsData")
{
}

GS::Optional<GS::UniString> CreateObjectsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("objectsData", "Object",
                                        "The name of the library part to use.", false);
}

constexpr const char* ParameterValueFieldName = "value";

static void SetParamValueInteger (API_AddParType&        addPar,
					              const GS::ObjectState& parameterDetails)
{
	Int32 value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = value;
}

static void SetParamValueDouble (API_AddParType&        addPar,
					             const GS::ObjectState&	parameterDetails)
{
	double value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = value;
}

static void SetParamValueOnOff (API_AddParType&         addPar,
				                const GS::ObjectState&	parameterDetails)
{
	GS::String value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = (value == "Off" ? 0 : 1);
}

static void SetParamValueBool (API_AddParType&        addPar,
				               const GS::ObjectState& parameterDetails)
{
	bool value;
	parameterDetails.Get (ParameterValueFieldName, value);
	addPar.value.real = (value ? 0 : 1);
}

static void SetParamValueString (API_AddParType&        addPar,
					             const GS::ObjectState&	parameterDetails)
{
	GS::UniString value;
	parameterDetails.Get (ParameterValueFieldName, value);

	GS::ucscpy (addPar.value.uStr, value.ToUStr (0, GS::Min(value.GetLength (), (USize)API_UAddParStrLen - 1)).Get ());
}

static void ChangeParams (API_AddParType**& params, const GS::HashTable<GS::String, GS::ObjectState>& changeParamsDictionary)
{
	const GSSize nParams = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
	for (GSIndex ii = 0; ii < nParams; ++ii) {
		API_AddParType& actParam = (*params)[ii];

		// Keys in upper case: GDL matches names without case, and a part's own parameter list
		// may spell them in lower case (Bed Double 01 has zzyzx). Parameter names are ASCII; the
		// standard library is used because GS::UniString renamed its case functions in Archicad 30.
		std::string upperName (actParam.name);
		std::transform (upperName.begin (), upperName.end (), upperName.begin (),
						[] (unsigned char c) { return static_cast<char> (std::toupper (c)); });
		const auto* value = changeParamsDictionary.GetPtr (GS::String (upperName.c_str ()));
		if (value == nullptr)
			continue;

		switch (actParam.typeID) {
			case APIParT_Integer:
			case APIParT_PenCol:			SetParamValueInteger (actParam, *value); break;
			case APIParT_ColRGB:
			case APIParT_Intens:
			case APIParT_Length:
			case APIParT_RealNum:
			case APIParT_Angle:				SetParamValueDouble (actParam, *value);	 break;
			case APIParT_LightSw:			SetParamValueOnOff (actParam, *value); 	 break;
			case APIParT_Boolean: 			SetParamValueBool (actParam, *value);	 break;
			case APIParT_LineTyp:
			case APIParT_Mater:
			case APIParT_FillPat:
			case APIParT_BuildingMaterial:
			case APIParT_Profile: 			SetParamValueInteger (actParam, *value); break;
			case APIParT_CString:
			case APIParT_Title: 			SetParamValueString (actParam, *value);	 break;
			default:
			case APIParT_Dictionary:
				// Not supported by the Archicad API yet
				break;
		}
	}
}

// Resolves 'libraryPartName' to element.object.libInd - Create only. Modify doesn't support
// swapping an existing instance's library part, only patching its placement/attribute fields.
static GS::Optional<GS::ObjectState> ResolveLibraryPartName (API_Element& element, const GS::ObjectState& parameters)
{
    GS::UniString uName;
    parameters.Get ("libraryPartName", uName);

    API_LibPart libPart = {};
    const USize maxNameLength = sizeof (libPart.docu_UName) / sizeof (libPart.docu_UName[0]) - 1;
    GS::ucscpy (libPart.docu_UName, uName.ToUStr (0, GS::Min (uName.GetLength (), maxNameLength)).Get ());

    GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, true);
    delete libPart.location;

    if (err != NoError) {
        return CreateErrorResponse (err, GS::UniString::Printf ("Not found library part with name '%T'", uName.ToPrintf ()));
    }

    element.object.libInd = libPart.index;
    return {};
}

static bool ResolveAttributeIndex (const GS::ObjectState& attributeId, API_AttrTypeID attributeType, API_AttributeIndex& attributeIndex)
{
    API_Attribute attribute = {};
    attribute.header.typeID = attributeType;
    attribute.header.guid = GetGuidFromObjectState (attributeId);
    if (attribute.header.guid == APINULLGuid) {
        return false;
    }

    if (ACAPI_Attribute_Get (&attribute) != NoError) {
        return false;
    }
    DisposeAttribute (attribute);

    attributeIndex = attribute.header.index;
    return true;
}

// Applies every optional API_ObjectType field beyond the library part itself - coordinates,
// dimensions, angle, pen/line type/surface/section attributes, fixed-size/angle behavior,
// per-story visibility, link-to-story, and (Lamp only) light color/on-off - shared by
// CreateObjects/CreateLamps (mask == nullptr, a freshly-defaulted element needs no mask) and
// ModifyObjects/ModifyLamps (mask != nullptr, every touched field also needs its
// ACAPI_ELEMENT_MASK_SET bit). element.object/.lamp alias the same union storage (API_LampType is
// a typedef of API_ObjectType across the AC25..29 SDKs we target), so writing through
// element.object.* is correct for both API_ObjectID and API_LampID.
static GS::Optional<GS::ObjectState> ApplyObjectLampDetails (
    API_Element& element, API_ElementMemo& memo, API_Element* mask,
    const Stories& stories, const GS::ObjectState& parameters)
{
    GS::ObjectState coordinates;
    if (parameters.Get ("coordinates", coordinates)) {
        const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (coordinates);

        element.object.pos.x = apiCoordinate.x;
        element.object.pos.y = apiCoordinate.y;

        const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", apiCoordinate.z, stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.object.level = floorIndexAndOffset.second;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_Elem_Head, floorInd);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, pos);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, level);
        }
    }

    GS::ObjectState dimensions;
    if (parameters.Get ("dimensions", dimensions)) {
        const API_Coord3D dims = Get3DCoordinateFromObjectState (dimensions);

        // xRatio/yRatio hold the size itself only with useXYFixSize on; otherwise they hold the size
        // divided by the library part's own A/B (as documented), so convert for the flag the element
        // ends up with.
        bool useFixSize = element.object.useXYFixSize;
        parameters.Get ("useFixSize", useFixSize);
        double libraryPartA = 1.0;
        double libraryPartB = 1.0;
        if (!useFixSize) {
            const GSErrCode err = GetLibraryPartOwnSize (element.object.libInd, libraryPartA, libraryPartB);
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to read the library part's own A and B, which 'dimensions' are converted with.");
            }
        }
        element.object.xRatio = dims.x / libraryPartA;
        element.object.yRatio = dims.y / libraryPartB;
        // A and B in the parameter memo have to agree with xRatio/yRatio: Archicad reconciles the two
        // on a change, and after CreateObjects the memo would keep the Object tool default's A and B,
        // which a later ModifyObjects (even one changing only the angle) then applied.
        ChangeParams (memo.params, {
            {"A", GS::ObjectState (ParameterValueFieldName, dims.x)},
            {"B", GS::ObjectState (ParameterValueFieldName, dims.y)},
            {"ZZYZX", GS::ObjectState (ParameterValueFieldName, dims.z)}
        });
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, xRatio);
            ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, yRatio);
        }
    }

    double angle = 0.0;
    if (parameters.Get ("angle", angle)) {
        element.object.angle = angle;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, angle);
    }

    Int32 penValue = 0;
    if (parameters.Get ("pen", penValue)) {
        element.object.pen = (short) penValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, pen);
    }

    const GS::ObjectState* lineTypeIdOS = parameters.Get ("lineTypeId");
    if (lineTypeIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*lineTypeIdOS, API_LinetypeID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'lineTypeId' line type reference.");
        }
        element.object.ltypeInd = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, ltypeInd);
    }

    const GS::ObjectState* surfaceIdOS = parameters.Get ("surfaceId");
    if (surfaceIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*surfaceIdOS, API_MaterialID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'surfaceId' surface reference.");
        }
        element.object.mat = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, mat);
    }

    const GS::ObjectState* sectionFillIdOS = parameters.Get ("sectionFillId");
    if (sectionFillIdOS != nullptr) {
        API_AttributeIndex idx = APIInvalidAttributeIndex;
        if (!ResolveAttributeIndex (*sectionFillIdOS, API_FilltypeID, idx)) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid 'sectionFillId' fill reference.");
        }
        element.object.sectFill = idx;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectFill);
    }

    Int32 shortValue = 0;
    if (parameters.Get ("sectionFillPen", shortValue)) {
        element.object.sectFillPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectFillPen);
    }
    if (parameters.Get ("sectionFillBackgroundPen", shortValue)) {
        element.object.sectBGPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectBGPen);
    }
    if (parameters.Get ("sectionContourPen", shortValue)) {
        element.object.sectContPen = (short) shortValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, sectContPen);
    }

    bool boolValue = false;
    if (parameters.Get ("useObjectPens", boolValue)) {
        element.object.useObjPens = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjPens);
    }
    if (parameters.Get ("useObjectLineTypes", boolValue)) {
        element.object.useObjLtypes = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjLtypes);
    }
    if (parameters.Get ("useObjectMaterials", boolValue)) {
        element.object.useObjMaterials = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjMaterials);
    }
    if (parameters.Get ("useObjectSectionAttributes", boolValue)) {
        element.object.useObjSectAttrs = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useObjSectAttrs);
    }
    if (parameters.Get ("reflected", boolValue)) {
        element.object.reflected = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, reflected);
    }
    if (parameters.Get ("useFixSize", boolValue)) {
        element.object.useXYFixSize = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, useXYFixSize);
    }

    Int32 fixPointValue = 0;
    if (parameters.Get ("fixPoint", fixPointValue)) {
        element.object.fixPoint = (short) fixPointValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, fixPoint);
    }

    const GS::ObjectState* offsetOS = parameters.Get ("offset");
    if (offsetOS != nullptr) {
        element.object.offset = Get2DCoordinateFromObjectState (*offsetOS);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, offset);
    }

    if (parameters.Get ("useFixedAngle", boolValue)) {
        element.object.fixedAngle = boolValue ? 1 : 0;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, fixedAngle);
    }
    if (parameters.Get ("isAutoOnStoryVisibility", boolValue)) {
        element.object.isAutoOnStoryVisibility = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, isAutoOnStoryVisibility);
    }

    GS::ObjectState colorOS;
    if (parameters.Get ("lightColor", colorOS)) {
        element.object.lightColor = GetColorFromObjectState (colorOS);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, lightColor);
    }
    if (parameters.Get ("lightIsOn", boolValue)) {
        element.object.lightIsOn = boolValue;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, lightIsOn);
    }

    const GS::ObjectState* visibilityOS = parameters.Get ("visibility");
    if (visibilityOS != nullptr) {
        API_StoryVisibility vis = element.object.visibility;
        visibilityOS->Get ("showOnHome", vis.showOnHome);
        visibilityOS->Get ("showAllAbove", vis.showAllAbove);
        visibilityOS->Get ("showAllBelow", vis.showAllBelow);
        Int32 relAbove = vis.showRelAbove;
        Int32 relBelow = vis.showRelBelow;
        if (visibilityOS->Get ("showRelAbove", relAbove)) {
            vis.showRelAbove = (short) relAbove;
        }
        if (visibilityOS->Get ("showRelBelow", relBelow)) {
            vis.showRelBelow = (short) relBelow;
        }
        element.object.visibility = vis;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, visibility);
    }

    const GS::ObjectState* linkOS = parameters.Get ("linkToSettings");
    if (linkOS != nullptr) {
        API_LinkToSettings link = element.object.linkToSettings;
        Int32 homeDiff = link.homeStoryDifference;
        if (linkOS->Get ("homeStoryDifference", homeDiff)) {
            link.homeStoryDifference = (short) homeDiff;
        }
        linkOS->Get ("newCreationMode", link.newCreationMode);
        element.object.linkToSettings = link;
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_ObjectType, linkToSettings);
    }

    return {};
}

// parameters and layerIndex, which only creation takes. A parameter that cannot be set fails the
// item: the response has no room for warnings, and a value quietly left out is worse than an error.
static GS::Optional<GS::ObjectState> ApplyCreationOnlyFields (API_Element& element, API_ElementMemo& memo, const GS::ObjectState& parameters)
{
    GS::Array<GS::ObjectState> gdlParameters;
    if (parameters.Get ("parameters", gdlParameters)) {
        GS::Array<GS::UniString> warnings;
        bool sizeChanged = false;
        for (const GS::ObjectState& p : gdlParameters) {
            GS::UniString name;
            p.Get ("name", name);
            if (ApplyParameter (memo.params, p, warnings) &&
                (CHEqualCStringsInsensitive (name.ToCStr (), "A") || CHEqualCStringsInsensitive (name.ToCStr (), "B"))) {
                sizeChanged = true;
            }
        }
        if (!warnings.IsEmpty ()) {
            GS::UniString message ("Failed to set the GDL parameters: ");
            for (UIndex ii = 0; ii < warnings.GetSize (); ++ii) {
                message += (ii > 0 ? "; " : "") + warnings[ii];
            }
            return CreateErrorResponse (APIERR_BADPARS, message + ".");
        }
        // As for dimensions: xRatio/yRatio have to agree with A and B in the memo.
        if (sizeChanged) {
            double libraryPartA = 1.0;
            double libraryPartB = 1.0;
            if (!element.object.useXYFixSize) {
                const GSErrCode err = GetLibraryPartOwnSize (element.object.libInd, libraryPartA, libraryPartB);
                if (err != NoError) {
                    return CreateErrorResponse (err, "Failed to read the library part's own A and B, which A and B in 'parameters' are converted with.");
                }
            }
            double a = 0.0;
            double b = 0.0;
            if (GetNumericParameter (memo.params, "A", a)) {
                element.object.xRatio = a / libraryPartA;
            }
            if (GetNumericParameter (memo.params, "B", b)) {
                element.object.yRatio = b / libraryPartB;
            }
        }
    }

    return ApplyLayerIndexFromParameters (parameters, element);
}

// The create path shared by CreateObjects and CreateLamps. The element and memo arrive as the tool
// defaults (or a favorite applied to them), so when libraryPartName names another part they still
// hold the size and parameter values of the part the defaults hold, which Archicad keeps on create
// wherever the names agree (live 21.10.2026: a Bed Double 01 came out 5.4 m wide and 0.2 m high,
// the Object tool's leftovers). The part's own defaults are loaded then, as the Object Settings
// dialog does when another part is picked; the same part keeps the settings it has.
static GS::Optional<GS::ObjectState> PrepareLibraryPartElement (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters)
{
    const Int32 defaultsLibInd = element.object.libInd;
    auto err = ResolveLibraryPartName (element, parameters);
    if (err.HasValue ()) {
        return err;
    }
    if (element.object.libInd != defaultsLibInd) {
        // The part's own size goes into xRatio/yRatio as the size itself or as a ratio of 1,
        // depending on useXYFixSize, so take the flag the element ends up with.
        bool useFixSize = element.object.useXYFixSize;
        parameters.Get ("useFixSize", useFixSize);
        element.object.useXYFixSize = useFixSize;
        const GSErrCode loadErr = LoadLibraryPartDefaults (element, memo);
        if (loadErr != NoError) {
            return CreateErrorResponse (loadErr, "Failed to read the default parameters of the library part.");
        }
    }
    auto detailsErr = ApplyObjectLampDetails (element, memo, nullptr, stories, parameters);
    if (detailsErr.HasValue ()) {
        return detailsErr;
    }
    return ApplyCreationOnlyFields (element, memo, parameters);
}

GS::Optional<GS::ObjectState> CreateObjectsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    return PrepareLibraryPartElement (element, memo, stories, parameters);
}

// CreateLamps — parallel to CreateObjects but for AC subtype Lamp.
// Without this command, lamp libparts cannot be placed via Tapir —
// CreateObjects rejects them with APIERR_NOTSUPPORTED. The schema and
// placement logic are shared via BuildLibraryPartBasedSchema and
// ApplyObjectLampDetails above.
CreateLampsCommand::CreateLampsCommand () :
    CreateElementsCommandBase ("CreateLamps", API_LampID, "lampsData")
{
}

GS::Optional<GS::UniString> CreateLampsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedSchema ("lampsData", "Lamp",
                                        "The name of the lamp library part to use.", true);
}

GS::Optional<GS::ObjectState> CreateLampsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    return PrepareLibraryPartElement (element, memo, stories, parameters);
}

// Shared Modify schema builder for ModifyObjects/ModifyLamps, mirroring
// BuildLibraryPartBasedSchema's Create-side counterpart - same field list via
// BuildObjectLampDetailFields, minus libraryPartName/coordinates being required (Modify only
// patches the fields actually given).
// Built via concatenation, not a single Printf %s - see BuildLibraryPartBasedSchema's comment for
// why (confirmed live crash feeding the large BuildObjectLampDetailFields blob through Printf).
static GS::UniString BuildLibraryPartBasedModifySchema (const char* arrayFieldName, bool isLamp)
{
    GS::UniString schema = GS::UniString::Printf (R"({
        "type": "object",
        "properties": {
            "%s": {
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
                        )",
        arrayFieldName);

    schema += BuildObjectLampDetailFields (isLamp);

    schema += GS::UniString::Printf (R"(
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
            "%s"
        ]
    })", arrayFieldName);

    return schema;
}

static GS::ObjectState ExecuteModifyObjectOrLamp (const GS::ObjectState& parameters, const char* arrayFieldName, API_ElemTypeID elemTypeID, const char* undoableCommandName)
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get (arrayFieldName, itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    const Stories stories = GetStories ();

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (itemsWithDetails);

    ACAPI_CallUndoableCommand (undoableCommandName, [&] () -> GSErrCode {
        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }

            if (GetElemTypeId (element.header) != elemTypeID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not of the expected type."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            ACAPI_Element_GetMemo (element.header.guid, &memo, APIMemoMask_AddPars);

            auto applyErr = ApplyObjectLampDetails (element, memo, &mask, stories, item);
            if (applyErr.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*applyErr));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, &memo, APIMemoMask_AddPars, true);
            if (err == NoError) {
                changeReporter.MarkChanged (element.header.guid);
            }
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify element."));
        }

        return NoError;
    });

    changeReporter.Report ();

    return response;
}

ModifyObjectsCommand::ModifyObjectsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyObjectsCommand::GetName () const
{
    return "ModifyObjects";
}

GS::Optional<GS::UniString> ModifyObjectsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedModifySchema ("objectsWithDetails", false);
}

GS::Optional<GS::UniString> ModifyObjectsCommand::GetRawResponseSchema () const
{
    return R"({
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
    })";
}

GS::ObjectState ModifyObjectsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteModifyObjectOrLamp (parameters, "objectsWithDetails", API_ObjectID, "ModifyObjects");
}

ModifyLampsCommand::ModifyLampsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLampsCommand::GetName () const
{
    return "ModifyLamps";
}

GS::Optional<GS::UniString> ModifyLampsCommand::GetInputParametersSchema () const
{
    return BuildLibraryPartBasedModifySchema ("lampsWithDetails", true);
}

GS::Optional<GS::UniString> ModifyLampsCommand::GetRawResponseSchema () const
{
    return R"({
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
    })";
}

GS::ObjectState ModifyLampsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    return ExecuteModifyObjectOrLamp (parameters, "lampsWithDetails", API_LampID, "ModifyLamps");
}

ModifyTextsCommand::ModifyTextsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyTextsCommand::GetName () const
{
    return "ModifyTexts";
}

GS::Optional<GS::UniString> ModifyTextsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "textsWithDetails": {
                "type": "array",
                "description": "Array of Text elements to modify, with the fields to change. Only provided fields are changed; omitted fields are left as-is. A change of the text, the runs, or a run-level style field (pen, font, faces, height, effects) rebuilds the content as one paragraph, as SetDetailsOfElements does, and on a multi-run text applies that style to every run; a text that wraps keeps its wrap width through the rebuild unless the same item gives a width. The other style fields and a width leave the content as it is.",
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
                        "style": { "$ref": "#/TextStyleSettableDetails" },
                        "width": {
                            "type": "number",
                            "minimum": 0.0,
                            "description": "Optional box width in mm (paper size, like height). Greater than 0 turns word wrap on: the text wraps within this width ('Wrap Text' in the Text settings, nonBreaking false). 0 turns word wrap off: the text is auto-width and grows with its content; on a wrapped text this rebuilds the content, so like a pen change it is refused for content with an autotext run. Given alone it leaves the content and its runs (and any autotext in them) as they are. When the content is rebuilt in the same item, the width is applied after the rebuild. When omitted, the current wrap setting is kept, also through a rebuild of the content. Read back by GetDetailsOfElements as style.boxWidth, with style.nonBreaking false."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["textsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyTextsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ModifyTextsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("textsWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");
    const Stories stories = GetStories ();

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (itemsWithDetails);

    ACAPI_CallUndoableCommand ("ModifyTexts", [&] () -> GSErrCode {
        // Same autotext flag handling as CreateTexts (CreateElementsCommandBase::Execute): with the
        // flag on, a content starting with a Project Info autotext key followed by a line break
        // failed to change or lost its later lines, while CreateTexts wrote it correctly (#1096).
        bool savedAutoTextFlag;
        ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag);
        bool setAutoTextFlag = false;
        ACAPI_AutoText_ChangeAutoTextFlag (&setAutoTextFlag);
        const GS::OnExit autoTextGuard ([&savedAutoTextFlag] () { ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag); });

        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_TextID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Text."));
                continue;
            }

            double wrapWidth = 0.0;
            const bool hasWrapWidth = item.Get ("width", wrapWidth);
            if (hasWrapWidth && wrapWidth < 0.0) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "width must be 0 (word wrap off) or greater (the box width of the wrapped text in mm). Omit it to keep the current wrap setting."));
                continue;
            }
            // The wrap the text has now, kept through a content rebuild when the item gives no width.
            const bool wasWrapped = !element.text.nonBreaking && element.text.width > 0.0;
            const double currentWrapWidth = element.text.width;

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* coordinateOS = item.Get ("coordinate");
            if (coordinateOS != nullptr) {
                const API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (*coordinateOS);
                element.text.loc.x = apiCoordinate.x;
                element.text.loc.y = apiCoordinate.y;
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, loc);
                // Same floor resolution as CreateTexts: an explicit floorIndex wins, otherwise z picks the floor.
                element.header.floorInd = ResolveFloorIndexAndOffset (item, "floorIndex", apiCoordinate.z, stories).first;
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
            } else if (item.Get ("floorIndex", element.header.floorInd)) {
                ACAPI_ELEMENT_MASK_SET (mask, API_Elem_Head, floorInd);
            }

            const GS::ObjectState* styleOS = item.Get ("style");
            if (styleOS != nullptr) {
                // Apply BEFORE touching content: for a multistyle element (paragraphs/runs -
                // which every Tapir-created/modified Text has), Archicad honours the per-run
                // pen/faceBits/font/size stored in the memo over these top-level fields, so a
                // change of those is written into the runs by the content rebuild below. The
                // element-level fields apply through their own masks without a rebuild, which
                // keeps the content, and any autotext reference in it, as it is.
                auto styleErr = TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.text, &mask, false);
                if (styleErr.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (*styleErr));
                    continue;
                }
            }
            const bool styleChanged = (styleOS != nullptr) && TextLabelDetails::StyleNeedsContentRebuild (*styleOS);

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            bool contentChanged = false;
            // item.Get ("text") != nullptr (the pointer-returning overload) never detects a plain
            // scalar/string field - only nested objects - so it always evaluated to nullptr here and
            // silently skipped every content edit. Use the value-returning overloads instead, exactly
            // like the check they gate against below.
            GS::UniString explicitContentTextCheck;
            GS::Array<GS::ObjectState> explicitContentRunsCheck;
            const bool explicitContent = item.Get ("text", explicitContentTextCheck) || item.Get ("runs", explicitContentRunsCheck);
            // Width 0 on a wrapped text: the line ends Archicad wrapped stay in the content when only
            // nonBreaking changes, so the content is rebuilt from its read-back, as for a style change.
            const bool unwrap = hasWrapWidth && wrapWidth <= 0.0 && wasWrapped;
            if (explicitContent || styleChanged || unwrap) {
                // Start from a BLANK memo, not one fetched via ACAPI_Element_GetMemo first: confirmed
                // live on Archicad 28 (by cross-checking against Archicad's own official fix, commit
                // a2a111b/54cbd36 upstream) that ACAPI_Element_Change only persists memo-based text
                // content when fed a from-scratch API_ElementMemo{} - fetching the existing memo first
                // silently failed to persist despite ApplyTextContent replacing textContent/paragraphs
                // correctly. The memo mask is version dependent, see TextContentMemoMask.
                GS::ObjectState contentParams = item;
                if (!explicitContent) {
                    // Style-only change (or width 0) on an existing multistyle element: re-fetch the current
                    // content (every run of it, so multi-run content is not collapsed) with the
                    // given style merged into the runs, and rebuild from that so the new style
                    // actually applies. A failed read must not turn into an empty rebuild.
                    auto readErr = TextLabelDetails::ReadContentForStyleOnlyModify (element.header.guid, styleOS != nullptr ? *styleOS : GS::ObjectState (), contentParams);
                    if (readErr.HasValue ()) {
                        executionResults (CreateFailedExecutionResult (*readErr));
                        continue;
                    }
                }
                auto applyErr = TextLabelDetails::ApplyTextContent (memo, element.text, contentParams);
                if (applyErr.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (*applyErr));
                    continue;
                }
                contentChanged = true;
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nLine);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, height);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, useEolPos);
#ifndef ServerMainVers_2800
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, charCode); // the content is UTF-16 (see ApplyTextContent)
#endif
            }

            // The factors as read, or as the style sets them: a content write resets them, see KeepTextFactors.
            const double widthFactor = element.text.widthFactor;
            const double charSpaceFactor = element.text.charSpaceFactor;

            // Without a content rebuild the width goes into this change, which carries no memo,
            // so the content is kept. A rebuild makes the text auto-width (see ApplyTextContent),
            // so then the wrap width is written after it, by SetTextWrapWidth: the given one, or
            // the one the text had, so that changing a pen or the content does not undo the wrap.
            if (hasWrapWidth && !contentChanged) {
                element.text.nonBreaking = wrapWidth <= 0.0;
                ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
                if (wrapWidth > 0.0) {
                    element.text.width = wrapWidth;
                    ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
                }
            }
            const double widthAfterRebuild = hasWrapWidth ? wrapWidth : (wasWrapped ? currentWrapWidth : 0.0);

            // withdel=false was tried and made things WORSE (even the style fields stopped
            // applying) - confirmed live. withdel=true is required here.
            err = ACAPI_Element_Change (&element, &mask, contentChanged ? &memo : nullptr, contentChanged ? TextContentMemoMask : 0, true);
            if (err == NoError && contentChanged) {
                err = TextLabelDetails::KeepTextFactors (element.header.guid, widthFactor, charSpaceFactor);
                if (err == NoError && widthAfterRebuild > 0.0) {
                    err = TextLabelDetails::SetTextWrapWidth (element.header.guid, widthAfterRebuild);
                }
            }
            if (err == NoError) {
                changeReporter.MarkChanged (element.header.guid);
            }
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify the Text."));
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}

ModifyPicturesCommand::ModifyPicturesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyPicturesCommand::GetName () const
{
    return "ModifyPictures";
}

GS::Optional<GS::UniString> ModifyPicturesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "picturesWithDetails": {
                "type": "array",
                "description": "Array of Picture elements to modify, with the settings to change. Only the given settings are changed; the others are left as they are. The image itself is not changed. Archicad cannot change a picture in place, so a picture with a change is replaced by a new picture with the same image, layer, story and ID, and the original is deleted: the new picture has a new elementId (returned in elements), and labels, dimensions, properties and classifications of the original are not carried over.",
                "items": {
                    "type": "object",
                    "properties": {
                        "elementId": { "$ref": "#/ElementId" },
                        "position": {
                            "$ref": "#/Coordinate2D",
                            "description": "Where the anchor point of the picture is moved to, in model units (on a layout in layout units). When omitted, the anchor point stays where it is."
                        },
                        "anchorPoint": {
                            "$ref": "#/PictureAnchorPoint",
                            "description": "The new anchor point: the point of the picture's box that is placed at position, that stays in place when the size changes, and that the picture is rotated around. When given without position, the picture's box stays where it is and only the anchor point changes; on a rotated picture the rotation is then around the new anchor point, which can move the picture. When omitted, the anchor point is kept."
                        },
                        "width": {
                            "type": "number",
                            "exclusiveMinimum": 0,
                            "description": "The new width of the picture in model units. When only width is given, the height changes in proportion, so the picture keeps its current proportions."
                        },
                        "height": {
                            "type": "number",
                            "exclusiveMinimum": 0,
                            "description": "The new height of the picture in model units. When only height is given, the width changes in proportion, so the picture keeps its current proportions."
                        },
                        "angle": {
                            "type": "number",
                            "description": "The new rotation angle of the picture in radians, around its anchor point."
                        },
                        "mirrored": {
                            "type": "boolean",
                            "description": "Mirror the picture to its local vertical axis. The picture's box stays where it is drawn."
                        },
                        "transparent": {
                            "type": "boolean",
                            "description": "Draw the white pixels of the picture transparent."
                        },
                        "name": {
                            "type": "string",
                            "description": "The new name of the picture."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            },
            "databaseId": {
                "$ref": "#/DatabaseId",
                "description": "Optional database the pictures are in, e.g. a worksheet, a detail or a layout (see GetDatabases). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the pictures are looked for in the current database. If Archicad cannot switch to it, nothing is changed and the call fails."
            }
        },
        "additionalProperties": false,
        "required": ["picturesWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyPicturesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "elements": {
                "type": "array",
                "description": "One result per item of picturesWithDetails, in the same order. On success elementId is the picture's identifier after the change: a new one when a setting was changed (the picture is replaced), the given one when nothing was to change.",
                "items": { "$ref": "#/ElementIdOrError" }
            }
        },
        "additionalProperties": false,
        "required": ["elements"]
    })";
}

GS::ObjectState ModifyPicturesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("picturesWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");

    // The optional databaseId is handled as CreatePictures does it (CreateElementsCommandBase::Execute):
    // the database is switched inside the undoable command and switched back afterwards.
    const GS::ObjectState* databaseIdOs = parameters.Get ("databaseId");
    API_DatabaseInfo startingDatabase = {};
    if (databaseIdOs != nullptr) {
        API_WindowInfo startingWindow = {};
        const bool hasStartingWindow = ACAPI_Window_GetCurrentWindow (&startingWindow) == NoError;
        if (OpenDatabaseIfNeverOpened (GetGuidFromObjectState (*databaseIdOs)) && hasStartingWindow) {
            ACAPI_Database_ChangeCurrentDatabase (&startingWindow);
            ACAPI_Window_ChangeWindow (&startingWindow);
        }
        const GSErrCode currentErr = ACAPI_Database_GetCurrentDatabase (&startingDatabase);
        if (currentErr != NoError) {
            return CreateErrorResponse (currentErr, "Failed to get the current database.");
        }
    }
    GSErrCode databaseErr = NoError;

    // ACAPI_Element_Change does not support pictures (API_PictureID is not in its list of element
    // types in ACAPinc.h, and Archicad answers APIERR_BADID), so a changed picture is replaced: a
    // copy with the new settings and the original's image (pictHdl memo) is created, then the
    // original is deleted, as ChangeDrawingLink replaces a Drawing.
    ACAPI_CallUndoableCommand ("ModifyPictures", [&] () -> GSErrCode {
        if (databaseIdOs != nullptr) {
            databaseErr = ChangeCurrentDatabaseTo (GetGuidFromObjectState (*databaseIdOs));
            if (databaseErr != NoError) {
                ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
                return NoError;
            }
        }
        const GS::OnExit databaseGuard ([&] () {
            if (databaseIdOs != nullptr) {
                ACAPI_Database_ChangeCurrentDatabase (&startingDatabase);
            }
        });

        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                elements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            const API_Guid originalGuid = GetGuidFromObjectState (*elementId);
            API_Element element = {};
            element.header.guid = originalGuid;
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_PictureID) {
                elements (CreateErrorResponse (APIERR_BADID, "Element is not a Picture."));
                continue;
            }

            API_PictureType& picture = element.picture;
            bool anyChange = false;

            GS::UniString anchorPointString;
            const bool hasAnchorPoint = item.Get ("anchorPoint", anchorPointString);
            const GS::ObjectState* positionOS = item.Get ("position");
            double width = 0.0;
            double height = 0.0;
            const bool hasWidth = item.Get ("width", width);
            const bool hasHeight = item.Get ("height", height);
            bool mirrored = picture.mirrored;
            item.Get ("mirrored", mirrored);
            if (hasAnchorPoint || positionOS != nullptr || hasWidth || hasHeight || mirrored != picture.mirrored) {
                if ((hasWidth && !(width > 0.0)) || (hasHeight && !(height > 0.0))) {
                    elements (CreateErrorResponse (APIERR_BADPARS, "The width and the height of the picture must be greater than 0."));
                    continue;
                }
                const double currentWidth = picture.destBox.xMax - picture.destBox.xMin;
                const double currentHeight = picture.destBox.yMax - picture.destBox.yMin;
                if ((hasWidth != hasHeight) && !(currentWidth > 0.0 && currentHeight > 0.0)) {
                    elements (CreateErrorResponse (APIERR_GENERAL, "The picture has no size to keep the proportions of: give both width and height."));
                    continue;
                }
                if (!hasWidth && !hasHeight) {
                    width = currentWidth;
                    height = currentHeight;
                } else if (!hasWidth) {
                    width = height * currentWidth / currentHeight;
                } else if (!hasHeight) {
                    height = width * currentHeight / currentWidth;
                }

                // The stored destBox of a mirrored picture is one width to the right of where the
                // picture is drawn (seen in Archicad 29, reported on #2667), so the maths is done on
                // the drawn box and the stored box is shifted back for a mirrored result.
                const double currentDrawnXMin = picture.destBox.xMin - (picture.mirrored ? currentWidth : 0.0);

                // The box: its anchor point is placed at position (the current anchor point when no
                // position is given) and it gets the new size, as CreatePictures computes it.
                const API_AnchorID anchorPoint = hasAnchorPoint ? AnchorIdFromString (anchorPointString, picture.anchorPoint) : picture.anchorPoint;
                API_Coord position;
                if (positionOS != nullptr) {
                    position = Get2DCoordinateFromObjectState (*positionOS);
                } else {
                    // The (new) anchor point on the current box.
                    const API_Coord currentOffset = GetPictureAnchorOffset (anchorPoint, currentWidth, currentHeight);
                    position.x = currentDrawnXMin + currentOffset.x;
                    position.y = picture.destBox.yMin + currentOffset.y;
                }

                const API_Coord anchorOffset = GetPictureAnchorOffset (anchorPoint, width, height);
                picture.destBox.xMin = position.x - anchorOffset.x + (mirrored ? width : 0.0);
                picture.destBox.yMin = position.y - anchorOffset.y;
                picture.destBox.xMax = picture.destBox.xMin + width;
                picture.destBox.yMax = picture.destBox.yMin + height;
                picture.anchorPoint = anchorPoint;
                picture.mirrored = mirrored;
                anyChange = true;
            }

            double angle = 0.0;
            if (item.Get ("angle", angle)) {
                anyChange = anyChange || angle != picture.rotAngle;
                picture.rotAngle = angle;
            }
            bool transparent = false;
            if (item.Get ("transparent", transparent)) {
                anyChange = anyChange || transparent != picture.transparent;
                picture.transparent = transparent;
            }
            GS::UniString name;
            if (item.Get ("name", name)) {
                anyChange = anyChange || name != GS::UniString (picture.pictName);
                GS::ucscpy (picture.pictName, name.ToUStr (0, GS::Min (name.GetLength (), (USize) API_UniLongNameLen - 1)).Get ());
            }

            if (!anyChange) {
                elements (CreateElementIdObjectState (originalGuid));
                continue;
            }

            // The image of the original, so the new picture shows the same one.
            API_ElementMemo originalMemo = {};
            const GS::OnExit memoGuard ([&originalMemo] () { ACAPI_DisposeElemMemoHdls (&originalMemo); });
            err = ACAPI_Element_GetMemo (originalGuid, &originalMemo, APIMemoMask_All);
            if (err != NoError || originalMemo.pictHdl == nullptr) {
                elements (CreateErrorResponse (err != NoError ? err : APIERR_GENERAL, "Failed to read the image of the Picture."));
                continue;
            }
            GS::UniString originalId;
            const bool hasOriginalId = GetElementId (originalGuid, originalId) == NoError && !originalId.IsEmpty ();

            // As CreatePictures creates a picture: the size is taken from the whole destBox. The
            // new memo only lends the image; originalMemo owns it and disposes of it.
            picture.usePixelSize = false;
            picture.directCreate = false;
            element.header.guid = APINULLGuid;
            API_ElementMemo newMemo = {};
            newMemo.pictHdl = originalMemo.pictHdl;
            err = ACAPI_Element_Create (&element, &newMemo);
            if (err != NoError) {
                elements (CreateErrorResponse (err, "Failed to create the changed Picture, so the original is kept as it was."));
                continue;
            }
            const API_Guid newGuid = element.header.guid;

            err = ACAPI_Element_Delete ({ originalGuid });
            if (err != NoError) {
                ACAPI_Element_Delete ({ newGuid });
                elements (CreateErrorResponse (err, "Failed to replace the Picture: the original could not be deleted, so it is kept as it was."));
                continue;
            }
            if (hasOriginalId) {
                ChangeElementId (newGuid, originalId);
            }
            elements (CreateElementIdObjectState (newGuid));
        }
        return NoError;
    });

    if (databaseErr != NoError) {
        return CreateErrorResponse (databaseErr, "Failed to switch to the database given by databaseId, so nothing was changed. A worksheet or detail whose window was never opened can be opened once with ChangeWindow by its databaseId.");
    }
    return response;
}

ModifyLabelsCommand::ModifyLabelsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLabelsCommand::GetName () const
{
    return "ModifyLabels";
}

GS::Optional<GS::UniString> ModifyLabelsCommand::GetInputParametersSchema () const
{
    return R"({
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
                        "leaderLine": { "$ref": "#/LabelLeaderLineSettableDetails" },
                        "textWay": {
                            "$ref": "#/LabelTextDirection",
                            "description": "The new text direction, for a Text or a Symbol label. Refused together with symbolStyle.textWay."
                        },
                        "begCoordinate": {
                            "$ref": "#/Coordinate2D",
                            "description": "The new begin coordinate of the leader line: the point the label points at (where the arrow is). Only the leader line's begin moves; the label's text or symbol stays where it is."
                        },
                        "midCoordinate": {
                            "$ref": "#/Coordinate2D",
                            "description": "The new mid coordinate of the leader line (its break point)."
                        },
                        "endCoordinate": {
                            "$ref": "#/Coordinate2D",
                            "description": "The new end coordinate of the leader line, where the label's text or symbol sits: the text or symbol moves by as much as this point moves. To move the whole label, give all three coordinates moved by the same offset. GetDetailsOfElements returns the current ones."
                        }
                    },
                    "additionalProperties": false,
                    "required": ["elementId"]
                }
            }
        },
        "additionalProperties": false,
        "required": ["labelsWithDetails"]
    })";
}

GS::Optional<GS::UniString> ModifyLabelsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ModifyLabelsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("labelsWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (itemsWithDetails);

    ACAPI_CallUndoableCommand ("ModifyLabels", [&] () -> GSErrCode {
        // Same autotext flag handling as CreateLabels, see ModifyTexts.
        bool savedAutoTextFlag;
        ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag);
        bool setAutoTextFlag = false;
        ACAPI_AutoText_ChangeAutoTextFlag (&setAutoTextFlag);
        const GS::OnExit autoTextGuard ([&savedAutoTextFlag] () { ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag); });

        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_LabelID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Label."));
                continue;
            }

            // The fields of the other label class are refused, not silently ignored.
            {
                GS::UniString classCheckText;
                GS::Array<GS::ObjectState> classCheckRuns;
                const bool hasTextFields = item.Get ("text", classCheckText) || item.Get ("runs", classCheckRuns) || item.Get ("style") != nullptr;
                const bool hasSymbolFields = item.Get ("symbolStyle") != nullptr;
                if (element.label.labelClass == APILblClass_Text && hasSymbolFields) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "symbolStyle applies to a Symbol label only; this is a Text label."));
                    continue;
                }
                if (element.label.labelClass != APILblClass_Text && hasTextFields) {
                    executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "text, runs and style apply to a Text label only; this is a Symbol label."));
                    continue;
                }
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* leaderLineOS = item.Get ("leaderLine");
            if (leaderLineOS != nullptr) {
                auto leaderLineErr = TextLabelDetails::ApplyLabelLeaderLineSettableDetails (*leaderLineOS, element.label, &mask);
                if (leaderLineErr.HasValue ()) {
                    executionResults (CreateFailedExecutionResult (*leaderLineErr));
                    continue;
                }
            }

            auto textWayErr = TextLabelDetails::ApplyLabelTextWay (item, element.label, &mask);
            if (textWayErr.HasValue ()) {
                executionResults (CreateFailedExecutionResult (*textWayErr));
                continue;
            }

            // Moving or re-pointing the label (#2666). The text or symbol sits at the end of the
            // leader line, so it moves with endC; the DevKit's Element_Test moves a label the same
            // way (UpdateOneLabel in Element_Tools.cpp: begC, midC, endC and u.text.loc or
            // u.symbol.pos, changed together with one mask).
            const GS::ObjectState* begCOS = item.Get ("begCoordinate");
            if (begCOS != nullptr) {
                element.label.begC = Get2DCoordinateFromObjectState (*begCOS);
                ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, begC);
            }
            const GS::ObjectState* midCOS = item.Get ("midCoordinate");
            if (midCOS != nullptr) {
                element.label.midC = Get2DCoordinateFromObjectState (*midCOS);
                ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, midC);
            }
            const GS::ObjectState* endCOS = item.Get ("endCoordinate");
            if (endCOS != nullptr) {
                const API_Coord newEndC = Get2DCoordinateFromObjectState (*endCOS);
                const double dx = newEndC.x - element.label.endC.x;
                const double dy = newEndC.y - element.label.endC.y;
                element.label.endC = newEndC;
                ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, endC);
                if (element.label.labelClass == APILblClass_Text) {
                    element.label.u.text.loc.x += dx;
                    element.label.u.text.loc.y += dy;
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.loc);
                } else {
                    element.label.u.symbol.pos.x += dx;
                    element.label.u.symbol.pos.y += dy;
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.symbol.pos);
                }
            }

            API_ElementMemo memo = {};
            const GS::OnExit memoGuard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
            bool contentChanged = false;

            if (element.label.labelClass == APILblClass_Text) {
                const GS::ObjectState* styleOS = item.Get ("style");
                if (styleOS != nullptr) {
                    // Same as ModifyTexts: applied before the content rebuild, which only the
                    // run-level fields need.
                    auto styleErr = TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.label.u.text, &mask, true);
                    if (styleErr.HasValue ()) {
                        executionResults (CreateFailedExecutionResult (*styleErr));
                        continue;
                    }
                }
                const bool styleChanged = (styleOS != nullptr) && TextLabelDetails::StyleNeedsContentRebuild (*styleOS);
                // See ModifyTexts for why the value-returning overloads are required here (the
                // pointer-returning Get never detects a plain scalar/array field).
                GS::UniString explicitContentTextCheck;
                GS::Array<GS::ObjectState> explicitContentRunsCheck;
                const bool explicitContent = item.Get ("text", explicitContentTextCheck) || item.Get ("runs", explicitContentRunsCheck);
                if (explicitContent || styleChanged) {
                    // See ModifyTexts for why: blank memo + non-Uni memomask, not a memo fetched
                    // via GetMemo first with the *Uni masks.
                    GS::ObjectState contentParams = item;
                    if (!explicitContent) {
                        // See ModifyTexts: the content read back with the style merged into its runs.
                        auto readErr = TextLabelDetails::ReadContentForStyleOnlyModify (element.header.guid, *styleOS, contentParams);
                        if (readErr.HasValue ()) {
                            executionResults (CreateFailedExecutionResult (*readErr));
                            continue;
                        }
                    }
                    auto applyErr = TextLabelDetails::ApplyTextContent (memo, element.label.u.text, contentParams);
                    if (applyErr.HasValue ()) {
                        executionResults (CreateFailedExecutionResult (*applyErr));
                        continue;
                    }
                    contentChanged = true;
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nLine);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.width);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.height);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.nonBreaking);
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.useEolPos);
#ifndef ServerMainVers_2800
                    ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.charCode); // the content is UTF-16 (see ApplyTextContent)
#endif
                }
            } else {
                const GS::ObjectState* symbolStyleOS = item.Get ("symbolStyle");
                if (symbolStyleOS != nullptr) {
                    auto symbolStyleErr = TextLabelDetails::ApplyLabelSymbolStyleSettableDetails (*symbolStyleOS, element.label, &mask);
                    if (symbolStyleErr.HasValue ()) {
                        executionResults (CreateFailedExecutionResult (*symbolStyleErr));
                        continue;
                    }
                }
            }

            // The factors as read, or as the style sets them: a content write resets them, see KeepTextFactors.
            const double widthFactor = element.label.u.text.widthFactor;
            const double charSpaceFactor = element.label.u.text.charSpaceFactor;

            // withdel=true required - see ModifyTexts.
            err = ACAPI_Element_Change (&element, &mask, contentChanged ? &memo : nullptr, contentChanged ? TextContentMemoMask : 0, true);
            if (err == NoError && contentChanged) {
                err = TextLabelDetails::KeepTextFactors (element.header.guid, widthFactor, charSpaceFactor);
            }
            if (err == NoError) {
                changeReporter.MarkChanged (element.header.guid);
            }
            executionResults (err == NoError ? CreateSuccessfulExecutionResult () : CreateFailedExecutionResult (err, "Failed to modify the Label."));
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}

ChangeTextContentCommand::ChangeTextContentCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ChangeTextContentCommand::GetName () const
{
    return "ChangeTextContent";
}

GS::Optional<GS::UniString> ChangeTextContentCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "contentChanges": {
                "type": "array",
                "description": "The content changes, applied in order (several changes of one element see the content as the previous one left it). Unlike ModifyTexts/ModifyLabels, the rest of the content is not rebuilt: the other runs, their styles and any autotext in them stay as they are, and the inserted piece takes the style of the character at 'from'.",
                "items": {
                    "$ref": "#/TextContentChange"
                }
            }
        },
        "additionalProperties": false,
        "required": ["contentChanges"]
    })";
}

GS::Optional<GS::UniString> ChangeTextContentCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

GS::ObjectState ChangeTextContentCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> contentChanges;
    parameters.Get ("contentChanges", contentChanges);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    // ACAPI_Element_ChangeContent modifies the database, so it runs in an undo scope.
    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (contentChanges);

    ACAPI_CallUndoableCommand ("ChangeTextContent", [&] () -> GSErrCode {
        for (const GS::ObjectState& change : contentChanges) {
            const GS::ObjectState* elementId = change.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            const API_ElemTypeID typeId = GetElemTypeId (element.header);
            if (typeId != API_TextID && !(typeId == API_LabelID && element.label.labelClass == APILblClass_Text)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Text or a Label of the Text class."));
                continue;
            }

            Int32 from = 0;
            Int32 range = 0;
            GS::UniString content;
            change.Get ("from", from);
            change.Get ("range", range);
            change.Get ("content", content);
            if (from < 0 || range < 0) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "from and range must not be negative."));
                continue;
            }
            if (content.FindFirst (GS::UniChar (char ('\n'))) != MaxUIndex || content.FindFirst (GS::UniChar (char ('\r'))) != MaxUIndex) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "The content must not contain line ends."));
                continue;
            }

            // API_ContentPars has no count member: Archicad takes the number of pieces to change from the sizes
            // of the from, range, content and separator blocks, which the DevKit marks as GSPtr (the struct's
            // comments in the AC25 header), so they must be memory manager blocks from BMpAll. Stack and
            // std::vector memory made Archicad refuse every change, even an insertion at 0 (#1860). separator
            // holds the end (byte) position of each piece in the concatenated content; with one piece it is
            // the content's length. The content is passed as UTF-8, terminated; that Archicad reads it as UTF-8
            // and not in the system code page is unverified, so non-ASCII content must be checked in Archicad.
            const auto contentCStr = content.ToCStr (CC_UTF8);
            const Int32 contentLength = Strlen32 (contentCStr.Get ());
            Int32* fromBlock = reinterpret_cast<Int32*> (BMpAll (sizeof (Int32)));
            Int32* rangeBlock = reinterpret_cast<Int32*> (BMpAll (sizeof (Int32)));
            Int32* separatorBlock = reinterpret_cast<Int32*> (BMpAll (sizeof (Int32)));
            char* contentBlock = reinterpret_cast<char*> (BMpAll (contentLength + 1));
            const GS::OnExit blocksGuard ([&] () {
                for (GSPtr block : { reinterpret_cast<GSPtr> (fromBlock), reinterpret_cast<GSPtr> (rangeBlock), reinterpret_cast<GSPtr> (separatorBlock), reinterpret_cast<GSPtr> (contentBlock) }) {
                    if (block != nullptr) {
                        BMpFree (block);
                    }
                }
            });
            if (fromBlock == nullptr || rangeBlock == nullptr || separatorBlock == nullptr || contentBlock == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_MEMFULL, "Failed to allocate the content change."));
                continue;
            }
            *fromBlock = from;
            *rangeBlock = range;
            *separatorBlock = contentLength;
            std::copy (contentCStr.Get (), contentCStr.Get () + contentLength + 1, contentBlock);

            API_ContentPars contentPars = {};
            contentPars.head = element.header;
            contentPars.from = fromBlock;
            contentPars.range = rangeBlock;
            contentPars.content = contentBlock;
            contentPars.separator = separatorBlock;

            err = ACAPI_Element_ChangeContent (&contentPars);
            if (err == NoError) {
                changeReporter.MarkChanged (element.header.guid);
                executionResults (CreateSuccessfulExecutionResult ());
            } else if (err == APIERR_PAROVERLAP) {
                executionResults (CreateFailedExecutionResult (err, "The range reaches over a line end into another paragraph."));
            } else if (err == APIERR_PAROVERFLOW) {
                executionResults (CreateFailedExecutionResult (err, "from is past the end of the content."));
            } else {
                executionResults (CreateFailedExecutionResult (err, "Failed to change the content."));
            }
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}

CreateMeshesCommand::CreateMeshesCommand () :
    CreateElementsCommandBase ("CreateMeshes", API_MeshID, "meshesData")
{}

GS::Optional<GS::UniString> CreateMeshesCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "minimum": 1,
                        "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
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
                        "description": "Optional building material of the mesh. Its top, side and bottom surface overrides are cleared, also one from the Mesh tool defaults, so the mesh shows the building material's own surface; topMaterial, sideMaterial and bottomMaterial given in the same item are applied after that."
                    },
                    "topMaterial": {
                        "$ref": "#/OverriddenMaterial",
                        "description": "Optional override of the surface of the top of the mesh; overridden false clears the override."
                    },
                    "sideMaterial": {
                        "$ref": "#/OverriddenMaterial",
                        "description": "Optional override of the surface of the sides (skirt) of the mesh; overridden false clears the override."
                    },
                    "bottomMaterial": {
                        "$ref": "#/OverriddenMaterial",
                        "description": "Optional override of the surface of the bottom of the mesh; overridden false clears the override."
                    },
                    "materialsChained": {
                        "type": "boolean",
                        "description": "Optional: whether the surface overrides are chained (the chain button of the Model panel)."
                    },
                    "visibility": {
                        "$ref": "#/StoryVisibility",
                        "description": "Optional: the stories the contour of the mesh is shown on. Only the given fields change."
                    },
                    "fillVisibility": {
                        "$ref": "#/StoryVisibility",
                        "description": "Optional: the stories the cover fill of the mesh is shown on. Only the given fields change."
                    },
                    "cutFillPen": {
                        "$ref": "#/OverriddenPen",
                        "description": "Optional override of the pen of the cut fill (shown on solid meshes); overridden false clears the override."
                    },
                    "cutFillBackgroundPen": {
                        "$ref": "#/OverriddenPen",
                        "description": "Optional override of the background pen of the cut fill (shown on solid meshes); overridden false clears the override."
                    },
                    "sectionContourPen": {
                        "type": "integer",
                        "description": "Optional pen of the contour of the mesh where it is cut in sections."
                    },
                    "floorFill": {
                        "$ref": "#/MeshFloorFill",
                        "description": "Optional cover fill of the mesh in the floor plan. Only the given fields change."
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
})";
}

GS::Optional<GS::ObjectState> CreateMeshesCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& /*stories*/, const GS::ObjectState& parameters) const
{
    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    parameters.Get ("floorIndex", element.header.floorInd);
    parameters.Get ("level", element.mesh.level);
    parameters.Get ("skirtLevel", element.mesh.skirtLevel);
    GS::UniString skirtType;
    parameters.Get ("skirtType", skirtType);
    GS::UniString ridges;
    if (parameters.Get ("ridges", ridges)) {
        if (ridges == "AllSharp") {
            element.mesh.smoothRidges = APIRidge_AllSharp;
        } else if (ridges == "AllSmooth") {
            element.mesh.smoothRidges = APIRidge_AllSmooth;
        } else if (ridges == "UserDefined") {
            element.mesh.smoothRidges = APIRidge_UserSharp;
        }
    }

    bool showLines = false;
    if (parameters.Get ("showLines", showLines)) {
        element.mesh.showLines = showLines ? 1 : 0;
    }

    short contourPen = 0;
    if (parameters.Get ("contourPen", contourPen) && contourPen > 0) {
        element.mesh.contPen = contourPen;
    }

    short levelPen = 0;
    if (parameters.Get ("levelPen", levelPen) && levelPen > 0) {
        element.mesh.levelPen = levelPen;
    }

    Int32 lineTypeIndex = 0;
    if (parameters.Get ("lineTypeIndex", lineTypeIndex) && lineTypeIndex > 0) {
        element.mesh.ltypeInd = ACAPI_CreateAttributeIndex (lineTypeIndex);
    }

    if (skirtType == "SurfaceOnlyWithoutSkirt") {
        element.mesh.skirt = 3;
    } else if (skirtType == "WithSkirt") {
        element.mesh.skirt = 2;
    } else if (skirtType == "SolidBodyWithSkirt") {
        element.mesh.skirt = 1;
    }

    GS::Array<GS::ObjectState> polygonCoordinates;
    GS::Array<GS::ObjectState> polygonArcs;
    GS::Array<GS::ObjectState> holes;
    parameters.Get ("polygonCoordinates", polygonCoordinates);
    parameters.Get ("polygonArcs", polygonArcs);
    parameters.Get ("holes", holes);

    auto geoErr = BuildMeshPolyMemoFromGeometry (element, memo, polygonCoordinates, polygonArcs, holes);
    if (geoErr.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, geoErr.Get ());
    }

    GS::Array<GS::ObjectState> sublines;
    parameters.Get ("sublines", sublines);
    BuildMeshSublinesMemoFromGeometry (element, memo, sublines);

    // Building material and surfaces of the mesh, written as ModifyMeshes writes them (#2630);
    // ACAPI_Element_Create does not use the mask. A given building material clears the top, side
    // and bottom surface overrides of the tool defaults first, so the mesh shows the building
    // material's own surface unless the item gives its own overrides.
    API_Element unusedMask = {};
    ACAPI_ELEMENT_MASK_CLEAR (unusedMask);
    bool surfacesChanged = false;
    const GS::Optional<GS::UniString> surfacesError = ApplyMeshStructureAndSurfaces (element, unusedMask, parameters, true, surfacesChanged);
    if (surfacesError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, surfacesError.Get ());
    }

    // Floor plan and section settings, written as ModifyMeshes writes them (#2630).
    bool floorPlanChanged = false;
    const GS::Optional<GS::UniString> floorPlanError = ApplyMeshFloorPlanAndSection (element, unusedMask, parameters, floorPlanChanged);
    if (floorPlanError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, floorPlanError.Get ());
    }

    return {};
}

CreateRailingsCommand::CreateRailingsCommand () :
    CreateElementsCommandBase ("CreateRailings", API_RailingID, "railingsData")
{
}

// ACAPI_Element_Create puts a new railing on the story that is active in the floor plan, whatever
// header.floorInd says (confirmed live: with the basement active, floorIndex 0 and 1 both gave a railing
// homed on the basement, with bottomOffset recomputed from the vertex elevations). As CreateZones does for
// automatic zones (#853), the items are created in groups of one story each, with that story made active
// (APIStory_GoTo), and the previously active story is put back afterwards. The responses keep the order
// of the request. A call whose items all stay on the active story runs exactly as before.
GS::ObjectState CreateRailingsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const
{
    GS::Array<GS::ObjectState> railingsData;
    parameters.Get (arrayFieldName, railingsData);

    API_StoryInfo storyInfo = {};
    const GSErrCode storyErr = ACAPI_ProjectSetting_GetStorySettings (&storyInfo);
    BMKillHandle (reinterpret_cast<GSHandle*> (&storyInfo.data));
    if (storyErr != NoError) {
        return CreateElementsCommandBase::Execute (parameters, processControl);
    }
    const short originalActStory = storyInfo.actStory;

    std::vector<short> itemStories;
    std::vector<short> storiesInOrder;
    itemStories.reserve (railingsData.GetSize ());
    for (const GS::ObjectState& data : railingsData) {
        short story = originalActStory;
        // Without floorIndex, a railing made from templateElementId goes to the template's home story.
        const GS::ObjectState* templateId = data.Get ("templateElementId");
        if (!data.Get ("floorIndex", story) && templateId != nullptr) {
            API_Elem_Head templateHead = {};
            templateHead.guid = GetGuidFromObjectState (*templateId);
            if (ACAPI_Element_GetHeader (&templateHead) == NoError) {
                story = templateHead.floorInd;
            }
        }
        itemStories.push_back (story);
        if (std::find (storiesInOrder.begin (), storiesInOrder.end (), story) == storiesInOrder.end ()) {
            storiesInOrder.push_back (story);
        }
    }

    const bool needsStorySwitch = std::any_of (itemStories.begin (), itemStories.end (),
        [&] (short story) { return story != originalActStory; });
    if (!needsStorySwitch) {
        return CreateElementsCommandBase::Execute (parameters, processControl);
    }

    short activeStory = originalActStory;
    const GS::OnExit restoreActiveStory ([&] () {
        if (activeStory != originalActStory) {
            API_StoryCmdType storyCmd = {};
            storyCmd.action = APIStory_GoTo;
            storyCmd.index  = originalActStory;
            ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
        }
    });

    std::vector<GS::ObjectState> results (railingsData.GetSize ());
    for (const short story : storiesInOrder) {
        GS::ObjectState groupParameters;
        const auto& groupItems = groupParameters.AddList<GS::ObjectState> (arrayFieldName);
        std::vector<size_t> groupIndices;
        for (size_t i = 0; i < itemStories.size (); ++i) {
            if (itemStories[i] == story) {
                groupItems (railingsData[static_cast<GS::UIndex> (i)]);
                groupIndices.push_back (i);
            }
        }

        if (story != activeStory) {
            API_StoryCmdType storyCmd = {};
            storyCmd.action = APIStory_GoTo;
            storyCmd.index  = story;
            const GSErrCode err = ACAPI_ProjectSetting_ChangeStorySettings (&storyCmd);
            if (err != NoError) {
                const GS::UniString message = GS::UniString::Printf (
                    "Failed to activate story %d in the floor plan, where the railing is created; see GetStories for the valid story indices.",
                    static_cast<int> (story));
                for (const size_t i : groupIndices) {
                    results[i] = CreateErrorResponse (err, message);
                }
                continue;
            }
            activeStory = story;
        }

        const GS::ObjectState groupResponse = CreateElementsCommandBase::Execute (groupParameters, processControl);
        GS::Array<GS::ObjectState> groupElements;
        groupResponse.Get ("elements", groupElements);
        for (size_t k = 0; k < groupIndices.size (); ++k) {
            results[groupIndices[k]] = k < groupElements.GetSize ()
                ? groupElements[static_cast<GS::UIndex> (k)]
                : CreateErrorResponse (APIERR_GENERAL, "Failed to create the Railing.");
        }
    }

    GS::ObjectState response;
    const auto& elements = response.AddList<GS::ObjectState> ("elements");
    for (const GS::ObjectState& result : results) {
        elements (result);
    }
    return response;
}

GS::Optional<GS::UniString> CreateRailingsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "railingsData": {
            "type": "array",
            "description": "Array of data to create Railings.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Railing. Everything not given here - the posts, rails, handrails, toprails, panels and balusters - comes from the favorite or from the Railing tool defaults of the open project. Archicad generates one node per vertex and one segment per pair of consecutive vertices from them.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "templateElementId": {
                        "$ref": "#/ElementId",
                        "description": "Optional existing Railing whose settings the new railing takes instead of the tool defaults (or the favorite): its node and segment settings, from which the posts, rails, handrails, toprails, panels and balusters are generated, its story visibility, pens, bottom offset, layer and home story. floorIndex, bottomOffset and height given here override them. The reference line of a railing cannot be changed through the Archicad API, so a railing is given a new reference line by creating it again with itself as templateElementId and deleting the old one."
                    },
                    "coordinates": {
                        "type": "array",
                        "description": "The 2D coordinates of the reference line of the railing on the floor plan. Each pair of consecutive coordinates becomes one straight segment.",
                        "items": {
                            "$ref": "#/Coordinate2D"
                        },
                        "minItems": 2
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index (home story) of the railing. The story is made active in the floor plan while the railing is created and the previously active story is restored afterwards. When omitted, the current story."
                    },
                    "bottomOffset": {
                        "type": "number",
                        "description": "Optional base height of the railing relative to the floor level. When omitted, the value of the favorite (or of the Railing tool defaults) is kept."
                    },
                    "height": {
                        "type": "number",
                        "description": "Optional height of the railing segments. When omitted, the segment height of the favorite (or of the Railing tool defaults) is kept.",
                        "exclusiveMinimum": 0.0
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
        "railingsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateRailingsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);
    if (coordinates.GetSize () < 2) {
        return CreateErrorResponse (APIERR_BADPARS, "coordinates must have at least 2 points.");
    }
    for (UIndex i = 1; i < coordinates.GetSize (); ++i) {
        if (IsSame2DCoordinate (coordinates[i - 1], coordinates[i])) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Zero-length segment: coordinates[%u] and coordinates[%u] are identical.", i - 1, i));
        }
    }

    // templateElementId: the settings of an existing railing replace the tool defaults. Part of them is held by
    // API_RailingType (defNode, defSegment, the pens, the story visibility), but Archicad generates the posts,
    // rails, handrails, toprails, panels and balusters from the subelement defaults in the memo as well, so copying
    // the element alone gave a railing in between the template and the tool defaults (confirmed live). The template
    // is therefore made the Railing tool default for a moment, as picking up its parameters does in Archicad, the
    // defaults are read back as the new element, and the previous tool defaults are put back.
    const GS::ObjectState* templateId = parameters.Get ("templateElementId");
    if (templateId != nullptr) {
        API_Element templ = {};
        templ.header.guid = GetGuidFromObjectState (*templateId);
        if (ACAPI_Element_Get (&templ) != NoError) {
            return CreateErrorResponse (APIERR_BADID, "templateElementId: the element was not found.");
        }
        if (GetElemTypeId (templ.header) != API_RailingID) {
            return CreateErrorResponse (APIERR_BADPARS, "templateElementId: the element is not a Railing.");
        }
        API_ElementMemo templMemo = {};
        const GS::OnExit templMemoGuard ([&templMemo] () { ACAPI_DisposeElemMemoHdls (&templMemo); });
        GSErrCode err = ACAPI_Element_GetMemo (templ.header.guid, &templMemo);
        if (err != NoError) {
            return CreateErrorResponse (err, "templateElementId: the settings of the railing could not be read.");
        }

        API_Element previousDefaults = {};
        API_ElementMemo previousDefaultsMemo = {};
        const GS::OnExit previousDefaultsGuard ([&previousDefaultsMemo] () { ACAPI_DisposeElemMemoHdls (&previousDefaultsMemo); });
#ifdef ServerMainVers_2600
        previousDefaults.header.type = API_RailingID;
#else
        previousDefaults.header.typeID = API_RailingID;
#endif
        err = ACAPI_Element_GetDefaults (&previousDefaults, &previousDefaultsMemo);
        if (err != NoError) {
            return CreateErrorResponse (err, "Failed to read the Railing tool defaults.");
        }

        API_Element fullMask;
        ACAPI_ELEMENT_MASK_SETFULL (fullMask);
        err = ACAPI_Element_ChangeDefaults (&templ, &templMemo, &fullMask);
        if (err == NoError) {
            ACAPI_DisposeElemMemoHdls (&memo);
            memo = {};
            const API_Elem_Head head = element.header;
            element = {};
            element.header = head;
            err = ACAPI_Element_GetDefaults (&element, &memo);
        }
        ACAPI_Element_ChangeDefaults (&previousDefaults, &previousDefaultsMemo, &fullMask);
        if (err != NoError) {
            return CreateErrorResponse (err, "templateElementId: the settings of the railing could not be taken over.");
        }

        // The new railing keeps the story link of the tool defaults (the template's would place it relative to the
        // story it was created from), and takes the template's layer, bottom offset and home story.
        element.railing.linkToSettings = previousDefaults.railing.linkToSettings;
        element.railing.bottomOffset = templ.railing.bottomOffset;
        element.header.layer = templ.header.layer;
        element.header.floorInd = templ.header.floorInd;
    }

    short floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = floorIndex;
    }
    parameters.Get ("bottomOffset", element.railing.bottomOffset);
    // The height is a segment setting; defSegment is the segment every generated segment is made from.
    parameters.Get ("height", element.railing.defSegment.height);

    // Only the reference line is written, as in the DevKit's Element_Test (Do_CreateRailing): memo.coords
    // from index 1 (index 0 is unused) and one relative z per vertex. ACAPI_Element_Create generates the
    // nodes, segments, posts and rails from defNode/defSegment and the subelement defaults in the memo.
    const UInt32 nCoords = coordinates.GetSize ();
    if (memo.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
    }
    if (memo.polyZCoords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.polyZCoords));
    }
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    for (UIndex i = 0; i < nCoords; ++i) {
        (*memo.coords)[i + 1] = Get2DCoordinateFromObjectState (coordinates[i]);
    }
    // polyZCoords is the elevation of each vertex: Archicad takes the base of the railing from it and
    // derives bottomOffset from it, so every vertex gets the level of the home story plus bottomOffset.
    const double baseZ = GetZPos (element.header.floorInd, element.railing.bottomOffset, stories);
    memo.polyZCoords = reinterpret_cast<double**> (BMAllocateHandle ((nCoords + 1) * sizeof (double), ALLOCATE_CLEAR, 0));
    for (UIndex i = 1; i <= nCoords; ++i) {
        (*memo.polyZCoords)[i] = baseZ;
    }
    element.railing.nVertices = nCoords;

    return {};
}

GS::String CreateRailingsFromPolylineCommand::GetName () const
{
    return "CreateRailingsFromPolyline";
}

GS::Optional<GS::UniString> CreateRailingsFromPolylineCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "railingsData": {
            "type": "array",
            "description": "Array of data to create Railings.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Railing. Everything not given here - the posts, rails, handrails, toprails, panels and balusters - comes from the favorite or from the Railing tool defaults of the open project. Archicad generates one node per vertex and one segment per pair of consecutive vertices from them.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of a favorite to base the new element on. Its settings are applied first, then the explicitly given fields override them."
                    },
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "Optional floor index (home story) of the railing. The story is made active in the floor plan while the railing is created and the previously active story is restored afterwards. When omitted, the current story."
                    },
                    "polylineCoordinates": {
                        "type": "array",
                        "description": "The nodes of the reference line of the railing. z is the elevation of the node relative to the floor level of the home story, so the railing can climb or descend along its segments.",
                        "items": {
                            "$ref": "#/Coordinate3D"
                        },
                        "minItems": 2
                    },
                    "polylineArcs": {
                        "type": "array",
                        "description": "Optional arc segments of the reference line. begIndex and endIndex are indices into polylineCoordinates and must be consecutive (endIndex = begIndex + 1).",
                        "items": {
                            "$ref": "#/PolyArc"
                        }
                    },
                    "height": {
                        "type": "number",
                        "description": "Optional height of the railing segments. When omitted, the segment height of the favorite (or of the Railing tool defaults) is kept.",
                        "exclusiveMinimum": 0.0
                    }
                },
                "additionalProperties": false,
                "required": [
                    "polylineCoordinates"
                ]
            }
        }
    },
    "additionalProperties": false,
    "required": [
        "railingsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateRailingsFromPolylineCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::Array<GS::ObjectState> coordinates;
    GS::Array<GS::ObjectState> arcs;
    parameters.Get ("polylineCoordinates", coordinates);
    parameters.Get ("polylineArcs", arcs);
    const Int32 nCoords = (Int32) coordinates.GetSize ();
    if (nCoords < 2) {
        return CreateErrorResponse (APIERR_BADPARS, "polylineCoordinates must have at least 2 points.");
    }
    for (UIndex i = 1; i < coordinates.GetSize (); ++i) {
        if (IsSame2DCoordinate (coordinates[i - 1], coordinates[i])) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Zero-length segment: polylineCoordinates[%u] and polylineCoordinates[%u] have the same x and y.", i - 1, i));
        }
    }
    // The memo indices start at 1 (index 0 is unused), so the arcs are shifted by one.
    const GS::Array<API_PolyArc> polyArcs = GetPolyArcs (arcs, 1);
    for (const API_PolyArc& arc : polyArcs) {
        if (arc.begIndex < 1 || arc.endIndex > nCoords || arc.endIndex != arc.begIndex + 1) {
            return CreateErrorResponse (APIERR_BADPARS, "Each item of polylineArcs must connect two consecutive points of polylineCoordinates.");
        }
    }

    short floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = floorIndex;
    }
    parameters.Get ("height", element.railing.defSegment.height);

    // As in CreateRailings, only the reference line is written; the segment, post, rail and panel
    // definitions in the memo of the defaults are kept. polyZCoords holds the elevation of each vertex,
    // from which Archicad derives bottomOffset, so the z of the request is taken from the home story.
    if (memo.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
    }
    if (memo.polyZCoords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.polyZCoords));
    }
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    memo.polyZCoords = reinterpret_cast<double**> (BMAllocateHandle ((nCoords + 1) * sizeof (double), ALLOCATE_CLEAR, 0));
    for (Int32 i = 0; i < nCoords; ++i) {
        const API_Coord3D coord = Get3DCoordinateFromObjectState (coordinates[i]);
        (*memo.coords)[i + 1] = { coord.x, coord.y };
        (*memo.polyZCoords)[i + 1] = GetZPos (element.header.floorInd, coord.z, stories);
    }
    if (!polyArcs.IsEmpty ()) {
        memo.parcs = reinterpret_cast<API_PolyArc**> (BMAllocateHandle (polyArcs.GetSize () * sizeof (API_PolyArc), ALLOCATE_CLEAR, 0));
        for (UIndex i = 0; i < polyArcs.GetSize (); ++i) {
            (*memo.parcs)[i] = polyArcs[i];
        }
    }
    element.railing.nVertices = nCoords;

    return {};
}

CreateCurtainWallsCommand::CreateCurtainWallsCommand () :
    CreateElementsCommandBase ("CreateCurtainWalls", API_CurtainWallID, "curtainWallsData")
{
}

GS::Optional<GS::UniString> CreateCurtainWallsCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
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
})";
}

template <typename T>
static void ReplaceCWPatternArray (T*& array, const GS::Array<T>& values)
{
    if (array != nullptr) {
        BMpKill (reinterpret_cast<GSPtr*> (&array));
    }
    array = reinterpret_cast<T*> (BMpAllClear (values.GetSize () * sizeof (T)));
    for (UIndex i = 0; i < values.GetSize (); ++i) {
        array[i] = values[i];
    }
}

// Rewrites the column (primary) or row (secondary) pattern of the segment from the "sizes",
// "logic", "origin" and "flexibleIndices" fields. Returns the reason when the fields are invalid.
// Shared with ModifyCurtainWallsCommand (ExtendedElementCommands.cpp).
GS::Optional<GS::UniString> ApplyCurtainWallPattern (const GS::ObjectState& patternOS, const char* fieldName, API_CWSegmentPatternData& pattern)
{
    GS::Array<double> sizes;
    if (!GetDoubleList (patternOS, "sizes", sizes) || sizes.IsEmpty ()) {
        return GS::UniString (fieldName) + ".sizes must be a non-empty list of numbers.";
    }
    for (double size : sizes) {
        if (size <= 0.0) {
            return GS::UniString (fieldName) + ".sizes must only contain positive numbers.";
        }
    }

    GS::UniString logic;
    if (patternOS.Get ("logic", logic)) {
        if (logic == "FixedSizes") {
            pattern.logic = APICWSePL_FixedSizes;
        } else if (logic == "BestDivision") {
            pattern.logic = APICWSePL_BestDivision;
        } else {
            return GS::UniString (fieldName) + ".logic must be 'FixedSizes' or 'BestDivision'.";
        }
    } else if (pattern.logic == APICWSePL_NumberOfDivisions) {
        // The number-of-divisions logic of the favorite or the tool defaults has no meaning
        // for an explicitly given list of sizes.
        pattern.logic = APICWSePL_FixedSizes;
    }

    GS::UniString origin;
    if (patternOS.Get ("origin", origin)) {
        if (origin == "StartWithPattern") {
            pattern.gridOriginType = APICWSeGridOrigin_StartWithPattern;
        } else if (origin == "StartFromCenter") {
            pattern.gridOriginType = APICWSeGridOrigin_StartFromCenter;
        } else if (origin == "AlignToCenter") {
            pattern.gridOriginType = APICWSeGridOrigin_AlignToCenter;
        } else if (origin == "EndWithPattern") {
            pattern.gridOriginType = APICWSeGridOrigin_EndWithPattern;
        } else {
            return GS::UniString (fieldName) + ".origin must be 'StartWithPattern', 'StartFromCenter', 'AlignToCenter' or 'EndWithPattern'.";
        }
    }

    const UInt32 nPattern = sizes.GetSize ();

    // Which columns/rows may be stretched by the best-division logic: the given ones, or all of
    // them. Always rewritten, because the flexible indices of the defaults belong to the default
    // pattern and may be out of range for the new one.
    GS::Array<UInt32> flexibleIDs;
    if (patternOS.Contains ("flexibleIndices")) {
        GS::Array<double> flexibleIndices;
        if (!GetDoubleList (patternOS, "flexibleIndices", flexibleIndices)) {
            return GS::UniString (fieldName) + ".flexibleIndices must be a list of integers.";
        }
        for (double flexibleIndex : flexibleIndices) {
            if (flexibleIndex < 0.0 || flexibleIndex >= static_cast<double> (nPattern) || flexibleIndex != static_cast<double> (static_cast<Int64> (flexibleIndex))) {
                return GS::UniString (fieldName) + GS::UniString::Printf (".flexibleIndices must only contain indices from 0 to %u.", nPattern - 1);
            }
            const UInt32 id = static_cast<UInt32> (flexibleIndex);
            if (!flexibleIDs.Contains (id)) {
                flexibleIDs.Push (id);
            }
        }
    } else {
        for (UInt32 i = 0; i < nPattern; ++i) {
            flexibleIDs.Push (i);
        }
    }

    ReplaceCWPatternArray (pattern.pattern, sizes);
    pattern.nPattern = nPattern;
    pattern.endWithID = nPattern - 1;
    ReplaceCWPatternArray (pattern.flexibleIDs, flexibleIDs);
    pattern.nFlexibleIDs = flexibleIDs.GetSize ();
    pattern.nDivisions = 1;

    return {};
}

// Rewrites memo.cWSegPatternCells for the pattern in memo (nColumns x nRows cells) from the
// "cells" assignments. element is the curtain wall, whose panel and frame class counts bound the
// class indices (from the memo's class defaults when loaded, else from the element).
// Shared with ModifyCurtainWallsCommand (ExtendedElementCommands.cpp).
GS::Optional<GS::UniString> ApplyCurtainWallPatternCells (const GS::Array<GS::ObjectState>& cells, const API_Element& element, API_ElementMemo& memo, bool patternChanged)
{
    const UInt32 nColumns = memo.cWSegPrimaryPattern.nPattern;
    const UInt32 nRows = memo.cWSegSecondaryPattern.nPattern;
    const UInt32 nCells = nColumns * nRows;
    if (nCells == 0) {
        return GS::UniString ("The curtain wall has no pattern cells.");
    }
    if (cells.IsEmpty () && !patternChanged) {
        return {};
    }

    const UInt32 nDefaultCells = memo.cWSegPatternCells != nullptr
        ? static_cast<UInt32> (BMpGetSize (reinterpret_cast<GSPtr> (memo.cWSegPatternCells)) / sizeof (API_CWSegmentPatternCellData))
        : 0;
    const UInt32 nPanelClasses = memo.cWallPanelDefaults != nullptr
        ? static_cast<UInt32> (BMpGetSize (reinterpret_cast<GSPtr> (memo.cWallPanelDefaults)) / sizeof (API_CWPanelType))
        : element.curtainWall.nPanelDefaults;
    const UInt32 nFrameClasses = memo.cWallFrameDefaults != nullptr
        ? static_cast<UInt32> (BMpGetSize (reinterpret_cast<GSPtr> (memo.cWallFrameDefaults)) / sizeof (API_CWFrameType))
        : element.curtainWall.nFrameDefaults;

    // Every cell starts from the matching default cell when the pattern is the default one,
    // otherwise from the first default cell (or a plain cell when there is none).
    API_CWSegmentPatternCellData templateCell = {};
    templateCell.crossingFrameType = APICWCFT_NoCrossingFrame;
    templateCell.leftPanelID = APICWPanelClass_FirstCustomClass;
    templateCell.rightPanelID = APICWPanelClass_FirstCustomClass;
    templateCell.leftFrameID = APICWFrameClass_Division;
    templateCell.bottomFrameID = APICWFrameClass_Division;
    templateCell.crossingFrameID = APICWFrameClass_Division;
    if (nDefaultCells > 0) {
        templateCell = memo.cWSegPatternCells[0];
    }

    auto newCells = reinterpret_cast<API_CWSegmentPatternCellData*> (BMpAllClear (nCells * sizeof (API_CWSegmentPatternCellData)));
    for (UInt32 i = 0; i < nCells; ++i) {
        newCells[i] = (nDefaultCells == nCells) ? memo.cWSegPatternCells[i] : templateCell;
    }
    const GS::OnExit newCellsGuard ([&newCells] () {
        if (newCells != nullptr) {
            BMpKill (reinterpret_cast<GSPtr*> (&newCells));
        }
    });

    const auto isValidPanelClass = [&] (Int32 index) {
        return index == APICWPanelClass_Deleted ||
               (index >= APICWPanelClass_FirstCustomClass && static_cast<UInt32> (index - APICWPanelClass_FirstCustomClass) < nPanelClasses);
    };
    const auto isValidFrameClass = [&] (Int32 index) {
        return index == APICWFrameClass_Merged || index == APICWFrameClass_Division || index == APICWFrameClass_Corner || index == APICWFrameClass_Boundary ||
               (index >= APICWFrameClass_FirstCustomClass && static_cast<UInt32> (index - APICWFrameClass_FirstCustomClass) < nFrameClasses);
    };

    for (UIndex i = 0; i < cells.GetSize (); ++i) {
        const GS::ObjectState& cellOS = cells[i];
        Int32 columnIndex = -1;
        Int32 rowIndex = -1;
        cellOS.Get ("columnIndex", columnIndex);
        cellOS.Get ("rowIndex", rowIndex);
        if (columnIndex < 0 || static_cast<UInt32> (columnIndex) >= nColumns) {
            return GS::UniString::Printf ("cells[%u].columnIndex must be between 0 and %u (the pattern has %u columns).", i, nColumns - 1, nColumns);
        }
        if (rowIndex < 0 || static_cast<UInt32> (rowIndex) >= nRows) {
            return GS::UniString::Printf ("cells[%u].rowIndex must be between 0 and %u (the pattern has %u rows).", i, nRows - 1, nRows);
        }
        // Cells are stored row by row from the bottom row, each row from the start of the segment.
        API_CWSegmentPatternCellData& cell = newCells[static_cast<UInt32> (rowIndex) * nColumns + static_cast<UInt32> (columnIndex)];

        Int32 classIndex = 0;
        if (cellOS.Get ("panelClassIndex", classIndex)) {
            if (!isValidPanelClass (classIndex)) {
                return GS::UniString::Printf ("cells[%u].panelClassIndex %d is not a panel class of the curtain wall (0 or 1 to %u).", i, classIndex, nPanelClasses);
            }
            cell.leftPanelID = static_cast<UInt32> (classIndex);
            cell.rightPanelID = static_cast<UInt32> (classIndex);
        }
        if (cellOS.Get ("rightPanelClassIndex", classIndex)) {
            if (!isValidPanelClass (classIndex)) {
                return GS::UniString::Printf ("cells[%u].rightPanelClassIndex %d is not a panel class of the curtain wall (0 or 1 to %u).", i, classIndex, nPanelClasses);
            }
            cell.rightPanelID = static_cast<UInt32> (classIndex);
        }
        if (cellOS.Get ("leftFrameClassIndex", classIndex)) {
            if (!isValidFrameClass (classIndex)) {
                return GS::UniString::Printf ("cells[%u].leftFrameClassIndex %d is not a frame class of the curtain wall (0 to 3, or 4 to %u).", i, classIndex, APICWFrameClass_FirstCustomClass + nFrameClasses - 1);
            }
            cell.leftFrameID = static_cast<UInt32> (classIndex);
        }
        if (cellOS.Get ("bottomFrameClassIndex", classIndex)) {
            if (!isValidFrameClass (classIndex)) {
                return GS::UniString::Printf ("cells[%u].bottomFrameClassIndex %d is not a frame class of the curtain wall (0 to 3, or 4 to %u).", i, classIndex, APICWFrameClass_FirstCustomClass + nFrameClasses - 1);
            }
            cell.bottomFrameID = static_cast<UInt32> (classIndex);
        }
        if (cellOS.Get ("crossingFrameClassIndex", classIndex)) {
            if (!isValidFrameClass (classIndex)) {
                return GS::UniString::Printf ("cells[%u].crossingFrameClassIndex %d is not a frame class of the curtain wall (0 to 3, or 4 to %u).", i, classIndex, APICWFrameClass_FirstCustomClass + nFrameClasses - 1);
            }
            cell.crossingFrameID = static_cast<UInt32> (classIndex);
        }
        GS::UniString crossingFrameType;
        if (cellOS.Get ("crossingFrameType", crossingFrameType)) {
            if (crossingFrameType == "None") {
                cell.crossingFrameType = APICWCFT_NoCrossingFrame;
            } else if (crossingFrameType == "FromBottomLeftToTopRight") {
                cell.crossingFrameType = APICWCFT_FromBottomLeftToTopRight;
            } else if (crossingFrameType == "FromTopLeftToBottomRight") {
                cell.crossingFrameType = APICWCFT_FromTopLeftToBottomRight;
            } else {
                return GS::UniString::Printf ("cells[%u].crossingFrameType must be 'None', 'FromBottomLeftToTopRight' or 'FromTopLeftToBottomRight'.", i);
            }
        }
    }

    if (memo.cWSegPatternCells != nullptr) {
        BMpKill (reinterpret_cast<GSPtr*> (&memo.cWSegPatternCells));
    }
    memo.cWSegPatternCells = newCells;
    newCells = nullptr;

    return {};
}

GS::Optional<GS::ObjectState> CreateCurtainWallsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    GS::Array<GS::ObjectState> coordinates;
    parameters.Get ("coordinates", coordinates);
    if (coordinates.GetSize () < 2) {
        return CreateErrorResponse (APIERR_BADPARS, "coordinates must have at least 2 points.");
    }
    for (UIndex i = 1; i < coordinates.GetSize (); ++i) {
        if (IsSame2DCoordinate (coordinates[i - 1], coordinates[i])) {
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString::Printf ("Zero-length segment: coordinates[%u] and coordinates[%u] are identical.", i - 1, i));
        }
    }

    // The bottom of the curtain wall. planeOffset is what ACAPI_Element_Create places the base
    // surface by (storyRelLevel is read back, but not applied on creation - confirmed live).
    // Without zCoordinate the base level of the favorite (or of the tool defaults) is kept,
    // on the given floorIndex or on the current story.
    double zCoordinate = 0.0;
    short floorIndex = 0;
    if (parameters.Get ("zCoordinate", zCoordinate)) {
        const auto floorIndexAndOffset = ResolveFloorIndexAndOffset (parameters, "floorIndex", zCoordinate, stories);
        element.header.floorInd = floorIndexAndOffset.first;
        element.curtainWall.planeOffset = floorIndexAndOffset.second;
        element.curtainWall.storyRelLevel = floorIndexAndOffset.second;
    } else if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = floorIndex;
    }

    // The element arrives filled from the Curtain Wall tool defaults, with the item's favorite
    // (favoriteName) already applied to them by CreateElementsCommandBase: the frame and panel
    // classes, surfaces, boundary and corner frames and the pattern all come from there. Only
    // the fields the item names are written here.
    double height = 0.0;
    if (parameters.Get ("height", height)) {
        element.curtainWall.height = height;
    }

    // The reference polyline: memo.coords holds the vertices from index 1 (index 0 is unused,
    // as in every polygon memo), and every pair of consecutive vertices is one segment.
    const UInt32 nCoords = coordinates.GetSize ();
    if (memo.coords != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.coords));
    }
    if (memo.parcs != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.parcs));
    }
    memo.coords = reinterpret_cast<API_Coord**> (BMAllocateHandle ((nCoords + 1) * sizeof (API_Coord), ALLOCATE_CLEAR, 0));
    for (UIndex i = 0; i < nCoords; ++i) {
        (*memo.coords)[i + 1] = Get2DCoordinateFromObjectState (coordinates[i]);
    }
    element.curtainWall.nSegments = nCoords - 1;

    bool patternChanged = false;
    const GS::ObjectState* primaryPattern = parameters.Get ("primaryPattern");
    if (primaryPattern != nullptr) {
        const auto error = ApplyCurtainWallPattern (*primaryPattern, "primaryPattern", memo.cWSegPrimaryPattern);
        if (error.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, error.Get ());
        }
        patternChanged = true;
    }
    const GS::ObjectState* secondaryPattern = parameters.Get ("secondaryPattern");
    if (secondaryPattern != nullptr) {
        const auto error = ApplyCurtainWallPattern (*secondaryPattern, "secondaryPattern", memo.cWSegSecondaryPattern);
        if (error.HasValue ()) {
            return CreateErrorResponse (APIERR_BADPARS, error.Get ());
        }
        patternChanged = true;
    }

    GS::Array<GS::ObjectState> cells;
    parameters.Get ("cells", cells);
    const auto cellsError = ApplyCurtainWallPatternCells (cells, element, memo, patternChanged);
    if (cellsError.HasValue ()) {
        return CreateErrorResponse (APIERR_BADPARS, cellsError.Get ());
    }

    return {};
}

CreateLabelsCommand::CreateLabelsCommand () :
    CreateElementsCommandBase ("CreateLabels", API_LabelID, "labelsData")
{
}

GS::Optional<GS::UniString> CreateLabelsCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "minimum": 1,
                        "description": "Optional layer attribute index to place the new element on. It must be an existing, unlocked layer. When omitted the element gets the layer of the favorite or of the tool defaults."
                    },
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description" : "The parent element if the label is an associative label."
                    },
                    "labelClass": {
                        "$ref": "#/LabelClass",
                        "description": "Optional; if omitted, inherits the current Label tool default (which may silently resolve to either class - explicitly setting this avoids ambiguity), or is 'Symbol' when libraryPartName is given. 'Symbol' starts from the Symbol label settings of the Label tool default or of the favorite given in favoriteName: when those are a Text label, the item fails instead of placing a label without content."
                    },
                    "libraryPartName": {
                        "type": "string",
                        "description": "Optional name of the label library part of a Symbol label, as shown in the library (e.g. 'Flexi_Zone_Label'); without it the part of the Label tool default or of the favorite is used. The label gets the part's own default GDL parameters. It makes a Symbol label, so labelClass must be omitted or 'Symbol', and the Label tool default or the favorite must be a Symbol label (its other settings are kept). To switch the part of an existing label, use ChangeLibraryPartOfElements."
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
                    "textWay": {
                        "$ref": "#/LabelTextDirection",
                        "description": "Optional text direction, for a Text or a Symbol label; when omitted the Label tool default (or the favorite) is kept. Refused together with symbolStyle.textWay."
                    },
                    "begCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The begin coordinate of leader line. Optional parameter, but either begCoordinate or parentElementId must be provided. Unless midCoordinate and endCoordinate are both given too, the label is created at its default position relative to the parent element (Archicad's 'createAtDefaultPosition'), so the label may not end up at begCoordinate - read it back with GetDetailsOfElements after creation."
                    },
                    "midCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The mid coordinate of leader line. Optional parameter; give it together with endCoordinate to place the label exactly where the coordinates say."
                    },
                    "endCoordinate": {
                        "$ref": "#/Coordinate2D",
                        "description": "The end coordinate of leader line. Optional parameter; give it together with midCoordinate to place the label exactly where the coordinates say."
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
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "labelsData"
    ]
})";
}

static GSErrCode SetParagraph (API_ParagraphType** paragraph, UInt32 parNum, Int32 from, Int32 range, Int32 numOfTabs, Int32 numOfRuns,
							   Int32 numOfeolPos)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (numOfTabs < 1 || numOfRuns < 1 || numOfeolPos < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].from = from;
	(*paragraph)[parNum].range = range;

	(*paragraph)[parNum].tab = reinterpret_cast<API_TabType*> (BMpAllClear (numOfTabs * sizeof (API_TabType)));
	(*paragraph)[parNum].run = reinterpret_cast<API_RunType*> (BMpAllClear (numOfRuns * sizeof (API_RunType)));

	if (numOfeolPos > 0) {
		(*paragraph)[parNum].eolPos = reinterpret_cast<Int32*> (BMpAllClear (numOfeolPos * sizeof (Int32)));
	}

	return NoError;
}

static GSErrCode SetRun (API_ParagraphType** paragraph, UInt32 parNum, UInt32 runNum, Int32 from, Int32 range, short pen, unsigned short faceBits,
						 short font, Int32 effectBits, double size)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (runNum >= BMGetPtrSize (reinterpret_cast<GSPtr> ((*paragraph)[parNum].run)) / sizeof (API_RunType))
		return APIERR_BADPARS;

	(*paragraph)[parNum].run[runNum].from	    = from;
	(*paragraph)[parNum].run[runNum].range	    = range;
	(*paragraph)[parNum].run[runNum].pen	    = pen;
	(*paragraph)[parNum].run[runNum].faceBits   = faceBits;
	(*paragraph)[parNum].run[runNum].font	    = font;
	(*paragraph)[parNum].run[runNum].effectBits = (unsigned short)effectBits;
	(*paragraph)[parNum].run[runNum].size	    = size;

	return NoError;
}

static GSErrCode SetEOL (API_ParagraphType** paragraph, UInt32 parNum, UInt32 eolNum, Int32 offset)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize (reinterpret_cast<GSHandle> (paragraph)) / sizeof (API_ParagraphType)))
		return APIERR_BADPARS;

	if (eolNum >= BMGetPtrSize (reinterpret_cast<GSPtr> ((*paragraph)[parNum].eolPos)) / sizeof (Int32))
		return APIERR_BADPARS;

	if (offset < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].eolPos[eolNum] = offset;

	return NoError;
}

// Fills the eolPos array of the single paragraph that holds the whole content: one entry per line,
// the position within the paragraph of the line's last character (the DevKit's Element_Test example
// writes 14 for "This multistyle" and 27, 35 for a paragraph wrapped after "example "), so the
// entries grow from line to line. They used to be each line's length minus one, which dropped as
// soon as a line was shorter than the one before it ("abcdefghij\nx" gave 9, then 1); ModifyTexts,
// ModifyLabels and SetDetailsOfElements then failed with "Failed to modify the Text" (#2461).
// An empty line ends at the line break before it, which keeps the entries growing; an empty first
// line ends at 0.
static void SetLineEnds (API_ParagraphType** paragraph, const GS::UniString& text, Int32 nLine)
{
    const GS::UniChar newlineChar = GS::UniChar (char ('\n'));
    const Int32 textLength = static_cast<Int32> (text.GetLength ());
    Int32 lineStart = 0;
    for (Int32 eolIndex = 0; eolIndex < nLine; ++eolIndex) {
        const UIndex newlinePos = (lineStart < textLength) ? text.FindFirst (newlineChar, static_cast<UIndex> (lineStart)) : MaxUIndex;
        const Int32 lineEnd = (newlinePos != MaxUIndex) ? static_cast<Int32> (newlinePos) : textLength;
        SetEOL (paragraph, 0, static_cast<UInt32> (eolIndex), GS::Max (lineEnd - 1, static_cast<Int32> (0)));
        lineStart = lineEnd + 1;
    }
}

API_JustID ParseJustificationString (const GS::UniString& justification)
{
    if (justification == "Center") {
        return APIJust_Center;
    } else if (justification == "Right") {
        return APIJust_Right;
    } else if (justification == "Full") {
        return APIJust_Full;
    }
    return APIJust_Left;
}

// The content writers below treat '\n' as the only line separator when counting nLine and
// filling eolPos, so a Windows-style "\r\n" (or a bare '\r') arriving through the JSON
// interface would leave stray CR characters inside memo.textContent while the line
// bookkeeping only accounts for the LFs - an inconsistent memo. Every write path normalizes
// the incoming content to '\n' separators first. (The "only the first character appears"
// report on Archicad 25-27 was the missing charCode = CC_UniCode, see ApplyTextContent.)
static GS::UniString NormalizeLineBreaks (const GS::UniString& text)
{
    GS::UniString normalized = text;
    normalized.ReplaceAll ("\r\n", "\n");
    normalized.ReplaceAll ("\r", "\n");
    return normalized;
}

// Used by SetDetailsOfElementsCommand's generic Text/Label write case (upstream official fix);
// TextLabelDetails::ApplyTextContent below is Tapir's own, richer equivalent used by
// CreateTexts/CreateLabels/ModifyTexts/ModifyLabels - both build memo.textContent/paragraphs but
// only ApplyTextContent supports multi-run content.
void SetTextContentAndParagraphs (API_ElementMemo& memo, API_TextType& textData, const GS::UniString& rawText)
{
    const GS::UniString text = NormalizeLineBreaks (rawText);
#ifdef ServerMainVers_2800
    delete memo.textContent;
    memo.textContent = new GS::UniString { text };
#else
    memo.textContent = BMhAllClear ((text.GetLength () + 1) * sizeof (GS::uchar_t));
    GS::ucscpy (reinterpret_cast<GS::uchar_t*> (*memo.textContent), text.ToUStr ());
    // See ApplyTextContent: the content above is UTF-16, which Archicad only reads as such with charCode CC_UniCode.
    textData.charCode = CC_UniCode;
#endif

    const GS::UniChar newlineChar = GS::UniChar (char ('\n'));
    textData.nLine = text.Count (newlineChar) + 1;
    const Int32 numOfParagraphs = 1;
    memo.paragraphs = reinterpret_cast<API_ParagraphType**> (BMhAll (numOfParagraphs * sizeof (API_ParagraphType)));
    SetParagraph (memo.paragraphs, 0, 0, text.GetLength (), 1, 1, textData.nLine);
    SetRun (memo.paragraphs, 0, 0, 0, text.GetLength (), textData.pen, textData.faceBits, textData.font, textData.effectsBits, textData.size);
    SetLineEnds (memo.paragraphs, text, textData.nLine);

    textData.width = 0;
    textData.height = 0;
    textData.nonBreaking = true;
    textData.multiStyle = true; // see ApplyTextContent
    textData.useEolPos = true;
}

const char* JustificationToString (API_JustID just)
{
    switch (just) {
        case APIJust_Center: return "Center";
        case APIJust_Right:  return "Right";
        case APIJust_Full:   return "Full";
        default:             return "Left";
    }
}

static const char* AnchorToString (API_AnchorID anchor)
{
    switch (anchor) {
        case APIAnc_MT: return "MiddleTop";
        case APIAnc_RT: return "RightTop";
        case APIAnc_LM: return "LeftMiddle";
        case APIAnc_MM: return "MiddleMiddle";
        case APIAnc_RM: return "RightMiddle";
        case APIAnc_LB: return "LeftBottom";
        case APIAnc_MB: return "MiddleBottom";
        case APIAnc_RB: return "RightBottom";
        default:        return "LeftTop";
    }
}

static API_AnchorID StringToAnchor (const GS::UniString& s)
{
    if (s == "MiddleTop")    return APIAnc_MT;
    if (s == "RightTop")     return APIAnc_RT;
    if (s == "LeftMiddle")   return APIAnc_LM;
    if (s == "MiddleMiddle") return APIAnc_MM;
    if (s == "RightMiddle")  return APIAnc_RM;
    if (s == "LeftBottom")   return APIAnc_LB;
    if (s == "MiddleBottom") return APIAnc_MB;
    if (s == "RightBottom")  return APIAnc_RB;
    return APIAnc_LT;
}

#ifdef ServerMainVers_2800
static const char* TextFrameShapeToString (API_TextFrameShapeTypeID shape)
{
    switch (shape) {
        case API_TextFrameShapeType_Circle:          return "Circle";
        case API_TextFrameShapeType_RoundedRectangle: return "RoundedRectangle";
        case API_TextFrameShapeType_Pill:             return "Pill";
        default:                                      return "Rectangle";
    }
}

static API_TextFrameShapeTypeID StringToTextFrameShape (const GS::UniString& s)
{
    if (s == "Circle")           return API_TextFrameShapeType_Circle;
    if (s == "RoundedRectangle") return API_TextFrameShapeType_RoundedRectangle;
    if (s == "Pill")             return API_TextFrameShapeType_Pill;
    return API_TextFrameShapeType_Rectangle;
}
#endif

static const char* LabelAnchorPointToString (API_LblAnchorID a)
{
    switch (a) {
        case APILbl_TopAnchor:    return "Top";
        case APILbl_BottomAnchor: return "Bottom";
        case APILbl_Underlined:   return "Underlined";
        default:                  return "Middle";
    }
}

static API_LblAnchorID StringToLabelAnchorPoint (const GS::UniString& s)
{
    if (s == "Top")        return APILbl_TopAnchor;
    if (s == "Bottom")     return APILbl_BottomAnchor;
    if (s == "Underlined") return APILbl_Underlined;
    return APILbl_MiddleAnchor;
}

static const char* LeaderShapeToString (API_LeaderLineShapeID s)
{
    switch (s) {
        case API_Splinear:   return "Splinear";
        case API_SquareRoot: return "SquareRoot";
        default:             return "Segmented";
    }
}

static API_LeaderLineShapeID StringToLeaderShape (const GS::UniString& s)
{
    if (s == "Splinear")   return API_Splinear;
    if (s == "SquareRoot") return API_SquareRoot;
    return API_Segmented;
}

static const char* LabelTextWayToString (API_DirID d)
{
    switch (d) {
        case APIDir_Horizontal: return "Horizontal";
        case APIDir_Vertical:   return "Vertical";
        case APIDir_General:    return "General";
        default:                return "Parallel";
    }
}

static API_DirID StringToLabelTextWay (const GS::UniString& s)
{
    if (s == "Horizontal") return APIDir_Horizontal;
    if (s == "Vertical")   return APIDir_Vertical;
    if (s == "General")    return APIDir_General;
    return APIDir_Parallel;
}

// API_ArrowID has 31 sequential values starting at APIArr_EmptyCirc (= 0, no explicit
// initialisers); this array is index-parallel to it. Checked against APIdefs_Elements.h of every
// supported DevKit (AC25 to AC29): the order is the same in all of them, and APIArr_SlashLine75
// is the LAST value, not adjacent to APIArr_SlashLine90, despite the two looking like they should
// pair up by name. #/LabelArrowType's enum in CommonSchemaDefinitions.json is documentation only
// (JSON Schema enum membership doesn't care about order) but is kept in this same order.
static const char* const kArrowTypeNames[] = {
    "EmptyCircle", "CrossCircle", "FullCircle",
    "SlashLine15", "OpenArrow15", "ClosedArrow15", "FullArrow15",
    "SlashLine30", "OpenArrow30", "ClosedArrow30", "FullArrow30",
    "SlashLine45", "OpenArrow45", "ClosedArrow45", "FullArrow45",
    "SlashLine60", "OpenArrow60", "ClosedArrow60", "FullArrow60",
    "SlashLine90",
    "PepitaCircle", "BandArrow",
    "HalfArrowCcw15", "HalfArrowCw15", "HalfArrowCcw30", "HalfArrowCw30",
    "HalfArrowCcw45", "HalfArrowCw45", "HalfArrowCcw60", "HalfArrowCw60",
    "SlashLine75"
};
constexpr int kArrowTypeCount = sizeof (kArrowTypeNames) / sizeof (kArrowTypeNames[0]);

static const char* ArrowTypeToString (API_ArrowID arrowType)
{
    const int idx = static_cast<int> (arrowType);
    return (idx >= 0 && idx < kArrowTypeCount) ? kArrowTypeNames[idx] : kArrowTypeNames[0];
}

static API_ArrowID StringToArrowType (const GS::UniString& s)
{
    for (int i = 0; i < kArrowTypeCount; ++i) {
        if (s == kArrowTypeNames[i]) {
            return static_cast<API_ArrowID> (i);
        }
    }
    return APIArr_EmptyCirc;
}

namespace TextLabelDetails {

// The font a style or a run gives: fontName resolved to the index of the loaded font of that name,
// otherwise fontIndex as given; fontName wins when both are present. A fontName no loaded font has
// is an error rather than a silently kept default (issue #844). NoValue when neither is given.
static GS::Optional<GS::ObjectState> GetFontIndexFromDetails (const GS::ObjectState& details, GS::Optional<short>& fontIndex)
{
    GS::UniString fontName;
    if (details.Get ("fontName", fontName)) {
        fontIndex = GetFontIndexFromName (fontName);
        if (!fontIndex.HasValue ()) {
            return CreateErrorResponse (APIERR_BADNAME, GS::UniString::Printf ("No loaded font is named '%T'. fontName takes the name of a Font attribute, as GetDetailsOfElements reports it in fontName; use fontIndex to give the font by its index instead.", fontName.ToPrintf ()));
        }
        return {};
    }

    short index = 0;
    if (details.Get ("fontIndex", index)) {
        fontIndex = index;
    }
    return {};
}

static void AddFontName (GS::ObjectState& os, short fontIndex)
{
    const GS::Optional<GS::UniString> fontName = GetFontNameFromIndex (fontIndex);
    if (fontName.HasValue ()) {
        os.Add ("fontName", *fontName);
    }
}

void AddTextStyleDetails (GS::ObjectState& os, const API_TextType& text, bool includeReadOnly)
{
    os.Add ("penIndex", text.pen);
    os.Add ("fontIndex", text.font);
    AddFontName (os, text.font);
    os.Add ("bold", (text.faceBits & APIFace_Bold) != 0);
    os.Add ("italic", (text.faceBits & APIFace_Italic) != 0);
    os.Add ("underline", (text.faceBits & APIFace_Underline) != 0);
    os.Add ("justification", JustificationToString (text.just));
    os.Add ("height", text.size);
    os.Add ("spacing", text.spacing);
    os.Add ("angle", text.angle);
    os.Add ("effectStrikeout", (text.effectsBits & APIEffect_StrikeOut) != 0);
    os.Add ("effectSuperscript", (text.effectsBits & APIEffect_SuperScript) != 0);
    os.Add ("effectSubscript", (text.effectsBits & APIEffect_SubScript) != 0);
    os.Add ("effectProtected", (text.effectsBits & APIEffect_Protected) != 0);
    os.Add ("widthFactor", text.widthFactor);
    os.Add ("charSpaceFactor", text.charSpaceFactor);
    os.Add ("fixedSize", text.fixedSize);
    os.Add ("usedContour", text.usedContour);
    os.Add ("usedFill", text.usedFill);
    os.Add ("contourPenIndex", text.contourPen);
    os.Add ("fillPenIndex", text.fillPen);
    os.Add ("anchor", AnchorToString (text.anchor));
    os.Add ("fixedAngle", text.fixedAngle);
    os.Add ("contourOffset", text.contourOffset);
    os.Add ("flipEnabled", text.flipEnabled);
#ifdef ServerMainVers_2800
    os.Add ("textFrameShape", TextFrameShapeToString (text.textFrame.shapeType));
    os.Add ("textFrameSizeFixed", text.textFrame.isSizeFixed);
    os.Add ("textFrameFixedWidth", text.textFrame.fixedWidth);
    os.Add ("textFrameFixedHeight", text.textFrame.fixedHeight);
#else
    os.Add ("textFrameShape", "Rectangle");
    os.Add ("textFrameSizeFixed", false);
    os.Add ("textFrameFixedWidth", 0.0);
    os.Add ("textFrameFixedHeight", 0.0);
#endif

    if (includeReadOnly) {
        os.Add ("lineCount", text.nLine);
        os.Add ("boxWidth", text.width);
        os.Add ("boxHeight", text.height);
    }
}

GS::Optional<GS::ObjectState> ApplyTextStyleSettableDetails (const GS::ObjectState& details, API_TextType& text, API_Element* mask, bool isLabelUnion)
{
    // Mask bits must target the real top-level union member (API_TextType for a standalone
    // Text element, API_LabelType::u.text for a text-class Label) - both share byte layout for
    // 'text' vs 'u.text' (the union starts at offset 0 of API_LabelType), but the macro needs
    // the exact type name, hence the isLabelUnion branch on every mask line.
#define TEXT_MASK_SET(fieldPath) \
    if (mask != nullptr) { \
        if (isLabelUnion) { ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, u.text.fieldPath); } \
        else { ACAPI_ELEMENT_MASK_SET (*mask, API_TextType, fieldPath); } \
    }

    if (details.Get ("penIndex", text.pen)) { TEXT_MASK_SET (pen) }
    {
        GS::Optional<short> fontIndex;
        const auto fontErr = GetFontIndexFromDetails (details, fontIndex);
        if (fontErr.HasValue ()) {
            return fontErr;
        }
        if (fontIndex.HasValue ()) {
            text.font = *fontIndex;
            TEXT_MASK_SET (font)
        }
    }

    bool bold = (text.faceBits & APIFace_Bold) != 0;
    bool italic = (text.faceBits & APIFace_Italic) != 0;
    bool underline = (text.faceBits & APIFace_Underline) != 0;
    bool faceChanged = false;
    faceChanged |= details.Get ("bold", bold);
    faceChanged |= details.Get ("italic", italic);
    faceChanged |= details.Get ("underline", underline);
    if (faceChanged) {
        text.faceBits = static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
        TEXT_MASK_SET (faceBits)
    }

    GS::UniString strVal;
    if (details.Get ("justification", strVal)) {
        text.just = ParseJustificationString (strVal);
        TEXT_MASK_SET (just)
    }
    if (details.Get ("height", text.size)) { TEXT_MASK_SET (size) }
    if (details.Get ("spacing", text.spacing)) { TEXT_MASK_SET (spacing) }
    if (details.Get ("angle", text.angle)) { TEXT_MASK_SET (angle) }

    bool strikeout = (text.effectsBits & APIEffect_StrikeOut) != 0;
    bool superscript = (text.effectsBits & APIEffect_SuperScript) != 0;
    bool subscript = (text.effectsBits & APIEffect_SubScript) != 0;
    bool protectedFlag = (text.effectsBits & APIEffect_Protected) != 0;
    bool effectsChanged = false;
    effectsChanged |= details.Get ("effectStrikeout", strikeout);
    effectsChanged |= details.Get ("effectSuperscript", superscript);
    effectsChanged |= details.Get ("effectSubscript", subscript);
    effectsChanged |= details.Get ("effectProtected", protectedFlag);
    if (effectsChanged) {
        text.effectsBits = (strikeout ? APIEffect_StrikeOut : 0) | (superscript ? APIEffect_SuperScript : 0) | (subscript ? APIEffect_SubScript : 0) | (protectedFlag ? APIEffect_Protected : 0);
        TEXT_MASK_SET (effectsBits)
    }

    if (details.Get ("widthFactor", text.widthFactor)) { TEXT_MASK_SET (widthFactor) }
    if (details.Get ("charSpaceFactor", text.charSpaceFactor)) { TEXT_MASK_SET (charSpaceFactor) }
    if (details.Get ("fixedSize", text.fixedSize)) { TEXT_MASK_SET (fixedSize) }
    if (details.Get ("usedContour", text.usedContour)) { TEXT_MASK_SET (usedContour) }
    if (details.Get ("usedFill", text.usedFill)) { TEXT_MASK_SET (usedFill) }
    if (details.Get ("contourPenIndex", text.contourPen)) { TEXT_MASK_SET (contourPen) }
    if (details.Get ("fillPenIndex", text.fillPen)) { TEXT_MASK_SET (fillPen) }
    if (details.Get ("anchor", strVal)) {
        text.anchor = StringToAnchor (strVal);
        TEXT_MASK_SET (anchor)
    }
    if (details.Get ("fixedAngle", text.fixedAngle)) { TEXT_MASK_SET (fixedAngle) }
    if (details.Get ("contourOffset", text.contourOffset)) { TEXT_MASK_SET (contourOffset) }
    if (details.Get ("flipEnabled", text.flipEnabled)) { TEXT_MASK_SET (flipEnabled) }

#ifdef ServerMainVers_2800
    // ACAPI_ELEMENT_MASK_SET flags one byte at the field's own address, so masking the whole
    // textFrame struct would flag its first member only (see arrowData below); every touched
    // sub-field is masked on its own.
    if (details.Get ("textFrameShape", strVal)) {
        text.textFrame.shapeType = StringToTextFrameShape (strVal);
        TEXT_MASK_SET (textFrame.shapeType)
    }
    if (details.Get ("textFrameSizeFixed", text.textFrame.isSizeFixed)) { TEXT_MASK_SET (textFrame.isSizeFixed) }
    if (details.Get ("textFrameFixedWidth", text.textFrame.fixedWidth)) { TEXT_MASK_SET (textFrame.fixedWidth) }
    if (details.Get ("textFrameFixedHeight", text.textFrame.fixedHeight)) { TEXT_MASK_SET (textFrame.fixedHeight) }
#endif

#undef TEXT_MASK_SET

    return {};
}

GS::Optional<GS::ObjectState> ApplyTextContent (API_ElementMemo& memo, API_TextType& textData, const GS::ObjectState& parameters)
{
    GS::Array<GS::ObjectState> runsData;
    const bool hasRuns = parameters.Get ("runs", runsData) && !runsData.IsEmpty ();

    GS::UniString text;
    GS::Array<GS::UniString> runTexts;
    GS::Array<short> runPens;
    GS::Array<short> runFonts;
    GS::Array<unsigned short> runFaceBits;
    GS::Array<Int32> runEffects;
    GS::Array<double> runSizes;

    if (hasRuns) {
        for (const GS::ObjectState& runOS : runsData) {
            GS::UniString runText;
            if (!runOS.Get ("text", runText)) {
                return CreateErrorResponse (APIERR_BADPARS, "Each entry in 'runs' requires a 'text' field.");
            }
            // Normalized per run, before the run lengths are taken, so the run ranges set
            // below stay consistent with the concatenated content.
            runText = NormalizeLineBreaks (runText);
            short pen = textData.pen;
            runOS.Get ("penIndex", pen);
            GS::Optional<short> runFont;
            const auto fontErr = GetFontIndexFromDetails (runOS, runFont);
            if (fontErr.HasValue ()) {
                return fontErr;
            }
            const short font = runFont.HasValue () ? *runFont : textData.font;
            bool bold = (textData.faceBits & APIFace_Bold) != 0;
            bool italic = (textData.faceBits & APIFace_Italic) != 0;
            bool underline = (textData.faceBits & APIFace_Underline) != 0;
            runOS.Get ("bold", bold);
            runOS.Get ("italic", italic);
            runOS.Get ("underline", underline);
            double size = textData.size;
            runOS.Get ("heightOverride", size);
            bool strikeout = (textData.effectsBits & APIEffect_StrikeOut) != 0;
            bool superscript = (textData.effectsBits & APIEffect_SuperScript) != 0;
            bool subscript = (textData.effectsBits & APIEffect_SubScript) != 0;
            bool protectedFlag = (textData.effectsBits & APIEffect_Protected) != 0;
            runOS.Get ("effectStrikeout", strikeout);
            runOS.Get ("effectSuperscript", superscript);
            runOS.Get ("effectSubscript", subscript);
            runOS.Get ("effectProtected", protectedFlag);

            runTexts.Push (runText);
            runPens.Push (pen);
            runFonts.Push (font);
            runFaceBits.Push (static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0)));
            runEffects.Push ((strikeout ? APIEffect_StrikeOut : 0) | (superscript ? APIEffect_SuperScript : 0) | (subscript ? APIEffect_SubScript : 0) | (protectedFlag ? APIEffect_Protected : 0));
            runSizes.Push (size);
            text += runText;
        }
    } else if (!parameters.Get ("text", text)) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'text' (or 'runs') parameter");
    } else {
        text = NormalizeLineBreaks (text);
    }

    // 'memo' may already carry content: the Create path gets its memo from ACAPI_Element_GetDefaults,
    // and a modify may pass one fetched via ACAPI_Element_GetMemo. Replace, never leak.
#ifdef ServerMainVers_2800
    delete memo.textContent;
    memo.textContent = new GS::UniString { text };
#else
    if (memo.textContent != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.textContent));
    }
    memo.textContent = BMhAllClear ((text.GetLength () + 1) * sizeof (GS::uchar_t));
    GS::ucscpy (reinterpret_cast<GS::uchar_t*> (*memo.textContent), text.ToUStr ());
    // Before Archicad 28 textContent is a char handle that Archicad reads as an 8-bit string
    // unless the element's charCode is CC_UniCode (APIdefs_Elements.h of the AC25-AC27 DevKits,
    // and their Element_Test example's CreateMultiTextElement, which writes UTF-16 the same way).
    // Without it the UTF-16 content above ends at the first character's high zero byte, so only
    // the first character of every text appeared. CC_UniCode also makes the paragraph/run/eolPos
    // offsets below character offsets, as they are computed.
    textData.charCode = CC_UniCode;
#endif

    const GS::UniChar newlineChar = GS::UniChar (char ('\n'));
    textData.nLine = text.Count (newlineChar) + 1;
    const Int32 numOfParagraphs = 1;
    const Int32 numOfRuns = hasRuns ? static_cast<Int32> (runTexts.GetSize ()) : 1;

    // Existing paragraphs (sized for the old content) and textLineStarts - a separate short** array
    // of line-start indices into textContent, distinct from each paragraph's own eolPos - must go
    // before the new content is set: leaving textLineStarts stale while textContent/paragraphs get
    // replaced produces an inconsistent memo that ACAPI_Element_Change silently refuses to persist
    // (confirmed live). Archicad rebuilds textLineStarts from the new paragraphs, as it does on a
    // brand new element. The paragraphs handle owns each paragraph's separately allocated run/tab/
    // eolPos arrays, so it is disposed with ACAPI_DisposeParagraphsHdl (the DevKit's own way, see
    // the Element_Test example), not with a plain BMKillHandle that would leak those inner arrays.
    if (memo.paragraphs != nullptr) {
        ACAPI_DisposeParagraphsHdl (&memo.paragraphs);
        memo.paragraphs = nullptr;
    }
    if (memo.textLineStarts != nullptr) {
        BMKillHandle (reinterpret_cast<GSHandle*> (&memo.textLineStarts));
    }

    memo.paragraphs = reinterpret_cast<API_ParagraphType**> (BMhAll (numOfParagraphs * sizeof (API_ParagraphType)));
    SetParagraph (memo.paragraphs, 0, 0, text.GetLength (), 1, numOfRuns, textData.nLine);
    // A paragraph carries its own justification (API_ParagraphType::just, see the DevKit's
    // Element_Test example); keep it in step with the element-level setting.
    (*memo.paragraphs)[0].just = textData.just;

    if (hasRuns) {
        // Each run carries its own pen/face/font/size/effects; a field a run does not give
        // defaults to the element-level style, as for a single-run content.
        Int32 runFrom = 0;
        for (Int32 i = 0; i < numOfRuns; ++i) {
            const Int32 runRange = runTexts[i].GetLength ();
            SetRun (memo.paragraphs, 0, static_cast<UInt32> (i), runFrom, runRange, runPens[i], runFaceBits[i], runFonts[i], runEffects[i], runSizes[i]);
            runFrom += runRange;
        }
    } else {
        SetRun (memo.paragraphs, 0, 0, 0, text.GetLength (), textData.pen, textData.faceBits, textData.font, textData.effectsBits, textData.size);
    }

    SetLineEnds (memo.paragraphs, text, textData.nLine);

    // Auto-width: the line ends are the ones set above. A wrap width of CreateTexts/ModifyTexts
    // is written after the content, without a memo (TextLabelDetails::SetTextWrapWidth).
    textData.width = 0;
    textData.height = 0;
    textData.nonBreaking = true;
    // Archicad reads the runs of memo.paragraphs, and the eolPos array, only for a multistyle text
    // (APIdefs_Elements.h: useEolPos is "only available for multistyle text"; the DevKit's Element_Test
    // example sets multiStyle with useEolPos). Without it ACAPI_Element_Create made one run in the
    // element-level style, so the bold, italic, pen or size of a run was lost.
    textData.multiStyle = true;
    textData.useEolPos = true;

    return {};
}

static GSErrCode GetTextContentMemo (const API_Guid& elemGuid, API_ElementMemo& memo)
{
#ifdef ServerMainVers_2800
    return ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_TextContent | APIMemoMask_Paragraph);
#else
    // Before Archicad 28 the plain masks may return the content as an 8-bit string with byte
    // offsets; the *Uni masks return it as UTF-16 with character offsets, as it is read below.
    return ACAPI_Element_GetMemo (elemGuid, &memo, APIMemoMask_TextContentUni | APIMemoMask_ParagraphUni);
#endif
}

static GS::UniString GetTextContentFromMemo (const API_ElementMemo& memo)
{
#ifdef ServerMainVers_2800
    return (memo.textContent != nullptr) ? *memo.textContent : GS::EmptyUniString;
#else
    GS::UniString content;
    if (memo.textContent != nullptr) {
        // The handle holds the characters and a terminator (the inverse of ApplyTextContent's
        // write). Its size bounds the read, so an unterminated handle cannot be over-read, and the
        // first terminator inside it ends the content, so any slack Archicad may allocate past the
        // terminator is not taken as text.
        const GSSize byteSize = BMGetHandleSize (reinterpret_cast<GSHandle> (memo.textContent));
        const USize charCount = static_cast<USize> (byteSize / sizeof (GS::uchar_t));
        const GS::uchar_t* chars = reinterpret_cast<const GS::uchar_t*> (*memo.textContent);
        USize contentCharCount = 0;
        while (contentCharCount < charCount && chars[contentCharCount] != 0) {
            ++contentCharCount;
        }
        content = GS::UniString (reinterpret_cast<const GS::UniChar::Layout*> (chars), contentCharCount);
    }
    return content;
#endif
}

// The content read with the autotext flag off, in which an autotext reads as its "<KEY>" instead
// of its value (the flag CreateElementsCommandBase also turns off, so a key given in the content
// of CreateTexts/CreateLabels is placed as an autotext). The flag is restored right after the
// read. Nothing is returned when the flag cannot be read or switched.
static GS::Optional<GS::UniString> GetRawTextContent (const API_Guid& elemGuid)
{
    bool savedAutoTextFlag = false;
    if (ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag) != NoError) {
        return {};
    }
    if (savedAutoTextFlag) {
        bool keysAutoTextFlag = false;
        if (ACAPI_AutoText_ChangeAutoTextFlag (&keysAutoTextFlag) != NoError) {
            return {};
        }
    }
    const GS::OnExit flagGuard ([&savedAutoTextFlag] () {
        if (savedAutoTextFlag) {
            ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag);
        }
    });

    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    if (GetTextContentMemo (elemGuid, memo) != NoError) {
        return {};
    }
    return GetTextContentFromMemo (memo);
}

GSErrCode AddTextContent (GS::ObjectState& os, const API_Guid& elemGuid)
{
    API_ElementMemo memo = {};
    const GS::OnExit guard ([&memo] () { ACAPI_DisposeElemMemoHdls (&memo); });
    const GSErrCode err = GetTextContentMemo (elemGuid, memo);
    if (err != NoError) {
        return err;
    }

    const GS::UniString content = GetTextContentFromMemo (memo);
    os.Add ("text", content);
    const GS::Optional<GS::UniString> rawContent = GetRawTextContent (elemGuid);
    if (rawContent.HasValue ()) {
        os.Add ("rawText", *rawContent);
    }

    const UInt32 nParagraphs = (memo.paragraphs != nullptr) ? static_cast<UInt32> (BMhGetSize (reinterpret_cast<GSHandle> (memo.paragraphs)) / sizeof (API_ParagraphType)) : 0;
    os.Add ("paragraphCount", static_cast<int> (nParagraphs));
    if (nParagraphs == 0) {
        return NoError;
    }

    // Every paragraph is read, not only the first: a text placed from the UI may carry one paragraph
    // per line, and a style-only ModifyTexts/ModifyLabels rebuilds the content from what is returned
    // here, so anything skipped would be lost. A run's `from` is relative to its paragraph (DevKit
    // Element_Test example: paragraph 1 starts at 16, its runs at 0 and 9), and neither the runs nor
    // the paragraph ranges cover the line-end characters between them, so each run's text extends
    // to the start of the next run - concatenating the runs gives back the whole content.
    struct RunSpan { UIndex start; const API_RunType* run; };
    GS::Array<RunSpan> spans;
    for (UInt32 p = 0; p < nParagraphs; ++p) {
        const API_ParagraphType& paragraph = (*memo.paragraphs)[p];
        const UInt32 nRuns = (paragraph.run != nullptr) ? static_cast<UInt32> (BMGetPtrSize (reinterpret_cast<GSPtr> (paragraph.run)) / sizeof (API_RunType)) : 0;
        for (UInt32 i = 0; i < nRuns; ++i) {
            spans.Push (RunSpan { static_cast<UIndex> (paragraph.from + paragraph.run[i].from), &paragraph.run[i] });
        }
    }
    // Every run is reported, a single one too: a run created with its own pen/font/face/size keeps
    // that only in the run, so leaving it out would hide the override and a style-only modify,
    // which rebuilds from this read-back, would revert it to the element-level fields.
    if (spans.IsEmpty ()) {
        return NoError;
    }

    const USize contentLength = content.GetLength ();
    const auto& runList = os.AddList<GS::ObjectState> ("runs");
    for (UIndex i = 0; i < spans.GetSize (); ++i) {
        const UIndex start = (i == 0) ? 0 : GS::Min (spans[i].start, contentLength);
        const UIndex end = (i + 1 < spans.GetSize ()) ? GS::Min (spans[i + 1].start, contentLength) : contentLength;
        const API_RunType& run = *spans[i].run;
        GS::ObjectState runOS;
        runOS.Add ("text", GS::UniString (content.GetSubstring (start, end > start ? end - start : 0)));
        runOS.Add ("penIndex", run.pen);
        runOS.Add ("fontIndex", run.font);
        AddFontName (runOS, run.font);
        runOS.Add ("bold", (run.faceBits & APIFace_Bold) != 0);
        runOS.Add ("italic", (run.faceBits & APIFace_Italic) != 0);
        runOS.Add ("underline", (run.faceBits & APIFace_Underline) != 0);
        runOS.Add ("heightOverride", run.size);
        runOS.Add ("effectStrikeout", (run.effectBits & APIEffect_StrikeOut) != 0);
        runOS.Add ("effectSuperscript", (run.effectBits & APIEffect_SuperScript) != 0);
        runOS.Add ("effectSubscript", (run.effectBits & APIEffect_SubScript) != 0);
        runOS.Add ("effectProtected", (run.effectBits & APIEffect_Protected) != 0);
        runList (runOS);
    }

    return NoError;
}

bool StyleNeedsContentRebuild (const GS::ObjectState& style)
{
    static const char* const runLevelFields[] = { "penIndex", "fontIndex", "fontName", "bold", "italic", "underline", "height",
                                                  "effectStrikeout", "effectSuperscript", "effectSubscript", "effectProtected" };
    for (const char* field : runLevelFields) {
        if (style.Contains (GS::String (field))) {
            return true;
        }
    }
    return false;
}

GS::Optional<GS::ObjectState> ReadContentForStyleOnlyModify (const API_Guid& elemGuid, const GS::ObjectState& style, GS::ObjectState& contentParams)
{
    GS::ObjectState readBack;
    const GSErrCode err = AddTextContent (readBack, elemGuid);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to read the content of the element.");
    }

    GS::UniString text;
    readBack.Get ("text", text);
    contentParams.Add ("text", text);

    GS::Array<GS::ObjectState> runs;
    if (!readBack.Get ("runs", runs) || runs.IsEmpty ()) {
        // No run information at all (a memo without paragraphs): the content is rebuilt as one
        // run from the element-level fields, which already carry the style. A single styled run
        // is reported by AddTextContent and merged below like any other.
        return {};
    }

    // A protected run is an autotext reference. Whether the memo reads such a run back as its
    // key or as its resolved value is not established, so the rebuild is refused rather than
    // risk turning the reference into static text.
    for (const GS::ObjectState& run : runs) {
        bool protectedRun = false;
        if (run.Get ("effectProtected", protectedRun) && protectedRun) {
            return CreateErrorResponse (APIERR_REFUSEDCMD, "The content has an autotext run; a change of pen, font, faces, height or effects would rebuild it from its read-back. Give the content explicitly (text or runs), or change only the element-level style fields.");
        }
    }

    // Every run keeps its own values except the ones the style names: those are set on all runs,
    // otherwise a style-only "bold": true would be overridden by the runs' read-back faces.
    // The style's font comes as fontName or fontIndex; the runs get it as the index either way.
    GS::Optional<short> styleFontIndex;
    const auto fontErr = GetFontIndexFromDetails (style, styleFontIndex);
    if (fontErr.HasValue ()) {
        return fontErr;
    }
    static const char* const boolFields[] = { "bold", "italic", "underline",
                                              "effectStrikeout", "effectSuperscript", "effectSubscript", "effectProtected" };
    const auto& runList = contentParams.AddList<GS::ObjectState> ("runs");
    for (const GS::ObjectState& run : runs) {
        GS::ObjectState mergedRun;
        GS::UniString runText;
        run.Get ("text", runText);
        mergedRun.Add ("text", runText);
        short penIndex = 0;
        if (style.Get ("penIndex", penIndex) || run.Get ("penIndex", penIndex)) {
            mergedRun.Add ("penIndex", penIndex);
        }
        short fontIndex = 0;
        if (styleFontIndex.HasValue ()) {
            mergedRun.Add ("fontIndex", *styleFontIndex);
        } else if (run.Get ("fontIndex", fontIndex)) {
            mergedRun.Add ("fontIndex", fontIndex);
        }
        for (const char* field : boolFields) {
            bool value = false;
            if (style.Get (field, value) || run.Get (field, value)) {
                mergedRun.Add (field, value);
            }
        }
        double height = 0.0;
        if (style.Get ("height", height) || run.Get ("heightOverride", height)) {
            mergedRun.Add ("heightOverride", height);
        }
        runList (mergedRun);
    }
    return {};
}

GSErrCode KeepTextFactors (const API_Guid& elemGuid, double widthFactor, double charSpaceFactor)
{
    API_Element element = {};
    element.header.guid = elemGuid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    API_TextType* text = nullptr;
    if (GetElemTypeId (element.header) == API_TextID) {
        text = &element.text;
        ACAPI_ELEMENT_MASK_SET (mask, API_TextType, widthFactor);
        ACAPI_ELEMENT_MASK_SET (mask, API_TextType, charSpaceFactor);
    } else if (GetElemTypeId (element.header) == API_LabelID && element.label.labelClass == APILblClass_Text) {
        text = &element.label.u.text;
        ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.widthFactor);
        ACAPI_ELEMENT_MASK_SET (mask, API_LabelType, u.text.charSpaceFactor);
    } else {
        return NoError;
    }

    if (text->widthFactor == widthFactor && text->charSpaceFactor == charSpaceFactor) {
        return NoError;
    }
    text->widthFactor = widthFactor;
    text->charSpaceFactor = charSpaceFactor;
    // No memo: a change of these two fields alone is kept (#1585), see the declaration.
    return ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
}

GSErrCode SetTextWrapWidth (const API_Guid& elemGuid, double width)
{
    API_Element element = {};
    element.header.guid = elemGuid;
    GSErrCode err = ACAPI_Element_Get (&element);
    if (err != NoError) {
        return err;
    }
    if (GetElemTypeId (element.header) != API_TextID) {
        return APIERR_BADID;
    }

    API_Element mask = {};
    ACAPI_ELEMENT_MASK_CLEAR (mask);
    element.text.nonBreaking = false;
    element.text.width = width;
    ACAPI_ELEMENT_MASK_SET (mask, API_TextType, nonBreaking);
    ACAPI_ELEMENT_MASK_SET (mask, API_TextType, width);
    // No memo, as in KeepTextFactors: the content and its runs (and any autotext in them) are kept.
    return ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
}

void AddLabelLeaderLineDetails (GS::ObjectState& os, const API_LabelType& label)
{
    os.Add ("penIndex", label.pen);
    os.Add ("lineTypeId", CreateGuidObjectState (GetAttributeGuidFromIndex (API_LinetypeID, label.ltypeInd)));
    os.Add ("contourOffset", label.contourOffset);
    os.Add ("framed", label.framed);
    os.Add ("hasLeaderLine", label.hasLeaderLine);
    os.Add ("anchorPoint", LabelAnchorPointToString (label.anchorPoint));
    os.Add ("leaderShape", LeaderShapeToString (label.leaderShape));
    os.Add ("squareRootAngle", label.squareRootAngle);
    os.Add ("arrowType", ArrowTypeToString (label.arrowData.arrowType));
#ifdef ServerMainVers_2900
    os.Add ("arrowVisible", label.arrowData.arrowVisibility);
#else
    // A label's arrow head sits at the leader line's beginning: the DevKit's own label example
    // (Element_Test, Do_CreateLabel_FixedFrame) sets begArrow and leaves endArrow alone.
    os.Add ("arrowVisible", label.arrowData.begArrow);
#endif
    os.Add ("arrowPenIndex", label.arrowData.arrowPen);
    os.Add ("arrowSize", label.arrowData.arrowSize);
    os.Add ("hideWithBaseElem", label.hideWithBaseElem);
    os.Add ("begCoordinate", Create2DCoordinateObjectState (label.begC));
    os.Add ("midCoordinate", Create2DCoordinateObjectState (label.midC));
    os.Add ("endCoordinate", Create2DCoordinateObjectState (label.endC));
}

GS::Optional<GS::ObjectState> ApplyLabelLeaderLineSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask)
{
    if (details.Get ("penIndex", label.pen)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, pen);
    }
    {
        const GS::ObjectState* attrId = details.Get ("lineTypeId");
        if (attrId != nullptr) {
            if (!ResolveAttributeIndex (*attrId, API_LinetypeID, label.ltypeInd)) {
                return CreateErrorResponse (APIERR_BADPARS, "Invalid 'lineTypeId' line type reference.");
            }
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, ltypeInd);
        }
    }
    if (details.Get ("contourOffset", label.contourOffset)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, contourOffset);
    }
    if (details.Get ("framed", label.framed)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, framed);
    }
    if (details.Get ("hasLeaderLine", label.hasLeaderLine)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, hasLeaderLine);
    }
    GS::UniString strVal;
    if (details.Get ("anchorPoint", strVal)) {
        label.anchorPoint = StringToLabelAnchorPoint (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, anchorPoint);
    }
    if (details.Get ("leaderShape", strVal)) {
        label.leaderShape = StringToLeaderShape (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, leaderShape);
    }
    if (details.Get ("squareRootAngle", label.squareRootAngle)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, squareRootAngle);
    }
    // NOTE: mask each arrowData sub-field individually - ACAPI_ELEMENT_MASK_SET only marks a
    // single byte at the given field's own address, so masking the whole 'arrowData' struct only
    // ever flags its first member (arrowType) as changed; arrowVisibility/arrowPen/arrowSize
    // further into the struct were silently ignored by ACAPI_Element_Change without their own
    // mask entries. That alone was NOT enough either though: confirmed live that even with its own
    // individual mask entry, arrowVisibility changes were still dropped on ModifyLabels unless
    // EVERY other arrowData sub-field is ALSO masked (and reasserted with its current value) in the
    // same call - Archicad's internal validation for this nested struct seems to require the whole
    // unit asserted together, not just the one field actually changing. So: once any arrowData
    // sub-field is touched, mask all of them, re-populating the untouched ones from the element as
    // already fetched via ACAPI_Element_Get (so their value doesn't actually change).
    bool arrowDataTouched = false;
    if (details.Get ("arrowType", strVal)) {
        label.arrowData.arrowType = StringToArrowType (strVal);
        arrowDataTouched = true;
    }
#ifdef ServerMainVers_2900
    if (details.Get ("arrowVisible", label.arrowData.arrowVisibility)) {
        arrowDataTouched = true;
    }
#else
    // begArrow is the label's arrow head (see AddLabelLeaderLineDetails); endArrow is left as it is.
    if (details.Get ("arrowVisible", label.arrowData.begArrow)) {
        arrowDataTouched = true;
    }
#endif
    if (details.Get ("arrowPenIndex", label.arrowData.arrowPen)) {
        arrowDataTouched = true;
    }
    if (details.Get ("arrowSize", label.arrowData.arrowSize)) {
        arrowDataTouched = true;
    }
    if (arrowDataTouched && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowType);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowPen);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowSize);
#ifdef ServerMainVers_2900
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.arrowVisibility);
#else
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.begArrow);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, arrowData.endArrow);
#endif
    }
    if (details.Get ("hideWithBaseElem", label.hideWithBaseElem)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, hideWithBaseElem);
    }

    return {};
}

void AddLabelSymbolStyleDetails (GS::ObjectState& os, const API_LabelType& label)
{
    os.Add ("textWay", LabelTextWayToString (label.textWay));
    os.Add ("fontIndex", label.font);
    AddFontName (os, label.font);
    os.Add ("bold", (label.faceBits & APIFace_Bold) != 0);
    os.Add ("italic", (label.faceBits & APIFace_Italic) != 0);
    os.Add ("underline", (label.faceBits & APIFace_Underline) != 0);
    os.Add ("flipEnabled", label.flipEnabled);
    os.Add ("nonBreaking", label.nonBreaking);
    os.Add ("textSize", label.textSize);
    os.Add ("useBackgroundFill", label.useBgFill);
    os.Add ("backgroundFillPenIndex", label.fillBgPen);
    os.Add ("effectStrikeout", (label.effectsBits & APIEffect_StrikeOut) != 0);
    os.Add ("effectSuperscript", (label.effectsBits & APIEffect_SuperScript) != 0);
    os.Add ("effectSubscript", (label.effectsBits & APIEffect_SubScript) != 0);
    os.Add ("effectProtected", (label.effectsBits & APIEffect_Protected) != 0);
}

GS::Optional<GS::ObjectState> ApplyLabelSymbolStyleSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask)
{
    GS::UniString strVal;
    if (details.Get ("textWay", strVal)) {
        label.textWay = StringToLabelTextWay (strVal);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, textWay);
    }
    {
        GS::Optional<short> fontIndex;
        const auto fontErr = GetFontIndexFromDetails (details, fontIndex);
        if (fontErr.HasValue ()) {
            return fontErr;
        }
        if (fontIndex.HasValue ()) {
            label.font = *fontIndex;
            if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, font);
        }
    }
    bool bold = (label.faceBits & APIFace_Bold) != 0;
    bool italic = (label.faceBits & APIFace_Italic) != 0;
    bool underline = (label.faceBits & APIFace_Underline) != 0;
    bool faceChanged = false;
    faceChanged |= details.Get ("bold", bold);
    faceChanged |= details.Get ("italic", italic);
    faceChanged |= details.Get ("underline", underline);
    if (faceChanged) {
        label.faceBits = static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, faceBits);
    }
    if (details.Get ("flipEnabled", label.flipEnabled)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, flipEnabled);
    }
    if (details.Get ("nonBreaking", label.nonBreaking)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, nonBreaking);
    }
    if (details.Get ("textSize", label.textSize)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, textSize);
    }
    if (details.Get ("useBackgroundFill", label.useBgFill)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, useBgFill);
    }
    if (details.Get ("backgroundFillPenIndex", label.fillBgPen)) {
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, fillBgPen);
    }
    bool strikeout = (label.effectsBits & APIEffect_StrikeOut) != 0;
    bool superscript = (label.effectsBits & APIEffect_SuperScript) != 0;
    bool subscript = (label.effectsBits & APIEffect_SubScript) != 0;
    bool protectedFlag = (label.effectsBits & APIEffect_Protected) != 0;
    bool effectsChanged = false;
    effectsChanged |= details.Get ("effectStrikeout", strikeout);
    effectsChanged |= details.Get ("effectSuperscript", superscript);
    effectsChanged |= details.Get ("effectSubscript", subscript);
    effectsChanged |= details.Get ("effectProtected", protectedFlag);
    if (effectsChanged) {
        label.effectsBits = (strikeout ? APIEffect_StrikeOut : 0) | (superscript ? APIEffect_SuperScript : 0) | (subscript ? APIEffect_SubScript : 0) | (protectedFlag ? APIEffect_Protected : 0);
        if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, effectsBits);
    }

    return {};
}

// API_LabelType::textWay is one field of the label, documented "for textual labels", and the
// Symbol label settings have the same direction choice, which symbolStyle.textWay has always
// written. The label-level "textWay" writes it for either class (#2666). Of API_DirID, labels take
// only Parallel, Horizontal, Vertical and General (APIdefs_Elements.h, AC25 to AC30); General is the
// custom angle, which a Text label takes from style.angle.
void AddLabelTextWay (GS::ObjectState& os, const API_LabelType& label)
{
    os.Add ("textWay", LabelTextWayToString (label.textWay));
}

GS::Optional<GS::ObjectState> ApplyLabelTextWay (const GS::ObjectState& item, API_LabelType& label, API_Element* mask)
{
    GS::UniString textWay;
    if (!item.Get ("textWay", textWay)) {
        return {};
    }
    const GS::ObjectState* symbolStyleOS = item.Get ("symbolStyle");
    GS::UniString symbolTextWay;
    if (symbolStyleOS != nullptr && symbolStyleOS->Get ("textWay", symbolTextWay)) {
        return CreateErrorResponse (APIERR_BADPARS, "Give textWay either on the label or in symbolStyle, not in both.");
    }
    label.textWay = StringToLabelTextWay (textWay);
    if (mask != nullptr) ACAPI_ELEMENT_MASK_SET (*mask, API_LabelType, textWay);
    return {};
}

} // namespace TextLabelDetails

// For a Slab, ACAPI_Element_CalcBounds goes through Archicad's 2D bound calculation, which
// dereferences the current window's draw environment - null when the slab is not drawn in
// that window (another story is displayed, its layer is hidden, or a non-floor-plan window
// is active), so Archicad dies with a SIGSEGV instead of returning an error (#686). A Slab's
// 2D extent is the extent of its polygon, so take it from the element memo instead - that is
// available no matter what the current window shows. Curved slab edges bulge slightly beyond
// the polygon vertices, but for a default label position the vertex extent is close enough.
static void CalcLabelParentBounds (const API_Elem_Head& parentElemHead, API_Box3D& box)
{
    if (GetElemTypeId (parentElemHead) != API_SlabID) {
        // const_cast: before AC27 ACAPI_Element_CalcBounds maps to ACAPI_Database, which takes void*
        ACAPI_Element_CalcBounds (const_cast<API_Elem_Head*> (&parentElemHead), &box);
        return;
    }

    bool isFirstCoord = true;
    for (const PolygonData& polygon : GetPolygonsFromMemoCoords (parentElemHead.guid)) {
        for (const API_Coord& coord : polygon.coords) {
            if (isFirstCoord || coord.x < box.xMin) box.xMin = coord.x;
            if (isFirstCoord || coord.x > box.xMax) box.xMax = coord.x;
            if (isFirstCoord || coord.y < box.yMin) box.yMin = coord.y;
            if (isFirstCoord || coord.y > box.yMax) box.yMax = coord.y;
            isFirstCoord = false;
        }
    }
}

// API_AddParType cannot carry Dictionary parameter values ("Dictionary parameters are currently
// inaccessible with this structure", APIdefs_LibraryParts.h): the Label tool defaults list them, but
// with an empty value. Passed on to ACAPI_Element_Create, the label is placed with an empty
// dictionary, and a library part that reads one (Flexi_Zone_Label's hidden "dict_words") fails -
// it is drawn as its hotspot only, and ACAPI_LibraryPart_OpenParameters on it answers
// APIERR_GENERAL, so Get/SetGDLParametersOfElements refuse it (#742, found and tested on AC29 by
// the reporter). Left out of memo.params, Archicad fills them in from the library part.
static void RemoveDictionaryParameters (API_AddParType**& params)
{
    if (params == nullptr) {
        return;
    }
    const GSSize count = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    GSSize kept = 0;
    for (GSSize ii = 0; ii < count; ++ii) {
        if ((*params)[ii].typeID != APIParT_Dictionary) {
            ++kept;
        }
    }
    if (kept == count) {
        return;
    }
    // A fresh handle instead of resizing in place: a handle from the API is not safe to hand to
    // BMReallocHandle (#452). The kept entries move over as they are, with the handles of their
    // array values, so only the outer block is freed here.
    API_AddParType** filtered = reinterpret_cast<API_AddParType**> (BMAllocateHandle (kept * sizeof (API_AddParType), ALLOCATE_CLEAR, 0));
    if (filtered == nullptr) {
        return;
    }
    GSSize target = 0;
    for (GSSize ii = 0; ii < count; ++ii) {
        if ((*params)[ii].typeID != APIParT_Dictionary) {
            (*filtered)[target++] = (*params)[ii];
        }
    }
    BMKillHandle (reinterpret_cast<GSHandle*> (&params));
    params = filtered;
}

GS::Optional<GS::ObjectState> CreateLabelsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories&, const GS::ObjectState& parameters) const
{
    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    parameters.Get ("floorInd", element.header.floorInd);

    const GS::ObjectState* begCOS = parameters.Get ("begCoordinate");
    const GS::ObjectState* midCOS = parameters.Get ("midCoordinate");
    const GS::ObjectState* endCOS = parameters.Get ("endCoordinate");

    element.label.parent = GetGuidFromArrayItem ("parentElementId", parameters);
    API_Elem_Head parentElemHead = {};
    if (element.label.parent != APINULLGuid) {
        parentElemHead.guid = element.label.parent;
        if (ACAPI_Element_GetHeader (&parentElemHead) == NoError) {
#ifdef ServerMainVers_2600
            element.label.parentType = parentElemHead.type;
#else
            element.label.parentType = parentElemHead.typeID;
#endif
        } else {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid parent element GUID");
        }

        element.header.floorInd = parentElemHead.floorInd;
    }

    if (begCOS != nullptr) {
        element.label.begC = Get2DCoordinateFromObjectState (*begCOS);
    } else if (parentElemHead.guid != APINULLGuid) {
        API_Box3D box = {};
        CalcLabelParentBounds (parentElemHead, box);
        element.label.begC.x = (box.xMin + box.xMax) / 2.0;
        element.label.begC.y = (box.yMin + box.yMax) / 2.0;
    } else {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'begCoordinate' parameter");
    }


    if (midCOS != nullptr && endCOS != nullptr) {
        element.label.midC = Get2DCoordinateFromObjectState (*midCOS);
        element.label.endC = Get2DCoordinateFromObjectState (*endCOS);
        element.label.createAtDefaultPosition = false;
    } else {
        element.label.createAtDefaultPosition = true;
    }

    // The class the Label tool defaults (or the favorite applied to them) came with: only a Symbol
    // default names a symbol library part in u.symbol, see the Symbol branch below.
    const API_LblClassID defaultLabelClass = element.label.labelClass;
    GS::UniString labelClassStr;
    const bool hasLabelClass = parameters.Get ("labelClass", labelClassStr);
    if (hasLabelClass) {
        element.label.labelClass = (labelClassStr == "Symbol") ? APILblClass_Symbol : APILblClass_Text;
    }

    // libraryPartName names the symbol label's library part, so it makes a Symbol label when
    // labelClass is omitted, and contradicts an explicit 'Text'.
    GS::UniString libraryPartName;
    const bool hasLibraryPartName = parameters.Get ("libraryPartName", libraryPartName) && !libraryPartName.IsEmpty ();
    if (hasLibraryPartName) {
        if (hasLabelClass && element.label.labelClass != APILblClass_Symbol) {
            return CreateErrorResponse (APIERR_BADPARS, "libraryPartName names the library part of a Symbol label, but labelClass is 'Text'. Omit labelClass or set it to 'Symbol'.");
        }
        element.label.labelClass = APILblClass_Symbol;
    }

    const GS::ObjectState* leaderLineOS = parameters.Get ("leaderLine");
    if (leaderLineOS != nullptr) {
        auto leaderLineErr = TextLabelDetails::ApplyLabelLeaderLineSettableDetails (*leaderLineOS, element.label, nullptr);
        if (leaderLineErr.HasValue ()) {
            return leaderLineErr;
        }
    }

    auto textWayErr = TextLabelDetails::ApplyLabelTextWay (parameters, element.label, nullptr);
    if (textWayErr.HasValue ()) {
        return textWayErr;
    }

    if (element.label.labelClass == APILblClass_Text) {
        // Style MUST be applied before content: CreateExt builds paragraphs/runs from
        // element.label.u.text.pen/faceBits/font/size at the time ApplyTextContent runs, and
        // Archicad ignores those top-level fields afterwards for a multistyle (paragraph-based)
        // element - only the run's own copies matter once paragraphs exist.
        const GS::ObjectState* styleOS = parameters.Get ("style");
        if (styleOS != nullptr) {
            auto styleErr = TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.label.u.text, nullptr, true);
            if (styleErr.HasValue ()) {
                return styleErr;
            }
        }
        auto err = TextLabelDetails::ApplyTextContent (memo, element.label.u.text, parameters);
        if (err.HasValue ()) {
            return err;
        }
    } else {
        const GS::ObjectState* symbolStyleOS = parameters.Get ("symbolStyle");
        if (symbolStyleOS != nullptr) {
            auto symbolStyleErr = TextLabelDetails::ApplyLabelSymbolStyleSettableDetails (*symbolStyleOS, element.label, nullptr);
            if (symbolStyleErr.HasValue ()) {
                return symbolStyleErr;
            }
        }

        // u.symbol shares its storage with u.text: when the defaults are a Text label, every u.symbol
        // field (libInd, but also its pens, line type, surface and fill indices) holds bytes of the
        // text settings, which may by chance be valid indices of something unrelated. Setting
        // libInd alone from libraryPartName would leave the rest of them as that garbage, so refuse
        // instead of placing a label from it (or nothing, as it did before).
        if (defaultLabelClass != APILblClass_Symbol) {
            GS::UniString favoriteName;
            const bool fromFavorite = parameters.Get ("favoriteName", favoriteName) && !favoriteName.IsEmpty ();
            return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("A Symbol label needs Symbol label settings to start from, but the ") + (fromFavorite ? "favorite '" + favoriteName + "'" : GS::UniString ("Label tool default")) + " is a Text label - apply a Symbol label favorite (favoriteName) or set the Label tool to a symbol label first. libraryPartName then chooses the library part.");
        }

        if (hasLibraryPartName) {
            // The named part replaces the one of the defaults, with its own parameter list: the
            // defaults' list belongs to their part. Only label library parts are searched.
            API_LibPart libPart = {};
            libPart.typeID = APILib_LabelID;
            const USize maxNameLength = sizeof (libPart.docu_UName) / sizeof (libPart.docu_UName[0]) - 1;
            GS::ucscpy (libPart.docu_UName, libraryPartName.ToUStr (0, GS::Min (libraryPartName.GetLength (), maxNameLength)).Get ());
            GSErrCode err = ACAPI_LibraryPart_Search (&libPart, false, true);
            delete libPart.location;
            if (err != NoError) {
                return CreateErrorResponse (err, "Failed to resolve 'libraryPartName': no label library part with the name '" + libraryPartName + "' in the loaded libraries.");
            }
            double a = 0.0;
            double b = 0.0;
            Int32 addParNum = 0;
            API_AddParType** addPars = nullptr;
            err = ACAPI_LibraryPart_GetParams (libPart.index, &a, &b, &addParNum, &addPars);
            if (err != NoError || addPars == nullptr) {
                if (addPars != nullptr) {
                    ACAPI_DisposeAddParHdl (&addPars);
                }
                return CreateErrorResponse (err != NoError ? err : APIERR_BADINDEX, "Failed to read the parameters of the 'libraryPartName' library part.");
            }
            if (memo.params != nullptr) {
                ACAPI_DisposeAddParHdl (&memo.params);
            }
            memo.params = addPars;
            element.label.u.symbol.libInd = libPart.index;
        }

        // A Symbol label is a library part instance, so ACAPI_Element_Create needs the library
        // part's parameter list in memo.params. The Label tool defaults (or the favorite applied
        // to them) normally carry it - for Flexi_Zone_Label all 140 parameters, checked on AC29
        // (#742) - and those values are kept. Only defaults that carry none get the library
        // part's own list, the way Window/Door markers (PrepareWindowOrDoorDefaults) and
        // interior elevation markers get theirs.
        const GSSize paramNum = memo.params != nullptr ? BMGetHandleSize ((GSHandle) memo.params) / sizeof (API_AddParType) : 0;
        if (paramNum == 0) {
            double a = 0.0;
            double b = 0.0;
            Int32 addParNum = 0;
            API_AddParType** libPartPars = nullptr;
            const GSErrCode paramsErr = ACAPI_LibraryPart_GetParams (element.label.u.symbol.libInd, &a, &b, &addParNum, &libPartPars);
            if (paramsErr != NoError || libPartPars == nullptr) {
                if (libPartPars != nullptr) {
                    ACAPI_DisposeAddParHdl (&libPartPars);
                }
                return CreateErrorResponse (paramsErr != NoError ? paramsErr : APIERR_BADINDEX, "Failed to read the parameters of the symbol label's library part. The Label tool defaults do not name a valid symbol library part - apply a Symbol label favorite (favoriteName) or set the Label tool to a symbol label first.");
            }
            if (memo.params != nullptr) {
                ACAPI_DisposeAddParHdl (&memo.params);
            }
            memo.params = libPartPars;
        }

        // Whichever list it is, it holds the Dictionary parameters with no value: see
        // RemoveDictionaryParameters for why the label is broken when they are passed on.
        RemoveDictionaryParameters (memo.params);
    }

    return {};
}

GSErrCode CreateLabelsCommand::AfterCreate (const API_Element& requestedElement, const API_Guid& newElementGuid, const GS::ObjectState& /*parameters*/) const
{
    if (requestedElement.label.labelClass != APILblClass_Text) {
        return NoError;
    }
    // The factors of the style, the favorite or the tool defaults: the create with the content memo leaves them at 1.
    return TextLabelDetails::KeepTextFactors (newElementGuid, requestedElement.label.u.text.widthFactor, requestedElement.label.u.text.charSpaceFactor);
}

CreateTextsCommand::CreateTextsCommand () :
    CreateElementsCommandBase ("CreateTexts", API_TextID, "textsData")
{
}

GS::Optional<GS::UniString> CreateTextsCommand::GetInputParametersSchema () const
{
    return R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
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
                    "width": {
                        "type": "number",
                        "exclusiveMinimum": 0.0,
                        "description": "Optional box width in mm (paper size, like height). Turns word wrap on: the text wraps within this width ('Wrap Text' in the Text settings, nonBreaking false). When omitted the text is auto-width (word wrap off) and grows with its content. Read back by GetDetailsOfElements as style.boxWidth, with style.nonBreaking false."
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
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a worksheet, a detail or a layout (see GetDatabases, or the databaseId CreateWorksheets and CreateDetails return). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "textsData"
    ]
})";
}

GS::Optional<GS::ObjectState> CreateTextsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const
{
    const GS::ObjectState* coordinateOS = parameters.Get ("coordinate");
    if (coordinateOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'coordinate' parameter");
    }
    API_Coord3D apiCoordinate = Get3DCoordinateFromObjectState (*coordinateOS);

    short floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = floorIndex;
    } else {
        const auto floorIndexAndOffset = GetFloorIndexAndOffset (apiCoordinate.z, stories);
        element.header.floorInd = floorIndexAndOffset.first;
    }

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    element.text.loc.x = apiCoordinate.x;
    element.text.loc.y = apiCoordinate.y;

    const GS::ObjectState* styleOS = parameters.Get ("style");
    if (styleOS != nullptr) {
        auto styleErr = TextLabelDetails::ApplyTextStyleSettableDetails (*styleOS, element.text, nullptr, false);
        if (styleErr.HasValue ()) {
            return styleErr;
        }
    }

    // Top-level height/pen/angle/justification take precedence over 'style' for backward
    // compatibility with the original CreateTexts shape.
    parameters.Get ("height", element.text.size);
    parameters.Get ("pen", element.text.pen);
    parameters.Get ("angle", element.text.angle);
    GS::UniString justification;
    if (parameters.Get ("justification", justification)) {
        element.text.just = ParseJustificationString (justification);
    }

    // Checked before the create; AfterCreate applies it to the new text.
    double width = 0.0;
    if (parameters.Get ("width", width) && width <= 0.0) {
        return CreateErrorResponse (APIERR_BADPARS, "width must be greater than 0 (the box width of the wrapped text in mm). Omit it for an auto-width text.");
    }

    auto err = TextLabelDetails::ApplyTextContent (memo, element.text, parameters);
    if (err.HasValue ()) {
        return err;
    }

    return {};
}

GSErrCode CreateTextsCommand::AfterCreate (const API_Element& requestedElement, const API_Guid& newElementGuid, const GS::ObjectState& parameters) const
{
    // See CreateLabelsCommand::AfterCreate.
    GSErrCode err = TextLabelDetails::KeepTextFactors (newElementGuid, requestedElement.text.widthFactor, requestedElement.text.charSpaceFactor);
    double width = 0.0;
    if (err == NoError && parameters.Get ("width", width)) {
        // The text is created auto-width, as without a width (see ApplyTextContent); word wrap
        // is turned on afterwards, the same way ModifyTexts does it for a width alone.
        err = TextLabelDetails::SetTextWrapWidth (newElementGuid, width);
    }
    return err;
}

const char* DimensionOriginToString (API_DimOriginID origin)
{
    switch (origin) {
        case APIDimOrigin_RefLevel1:    return "ReferenceLevel1";
        case APIDimOrigin_RefLevel2:    return "ReferenceLevel2";
        case APIDimOrigin_StoredOrigin: return "StoredOrigin";
        case APIDimOrigin_Altitude:     return "Altitude";
        case APIDimOrigin_ProjectZero:
        default:                        return "ProjectZero";
    }
}

bool ParseDimensionOriginString (const GS::UniString& origin, API_DimOriginID& result)
{
    if (origin == "ProjectZero") {
        result = APIDimOrigin_ProjectZero;
    } else if (origin == "ReferenceLevel1") {
        result = APIDimOrigin_RefLevel1;
    } else if (origin == "ReferenceLevel2") {
        result = APIDimOrigin_RefLevel2;
    } else if (origin == "StoredOrigin") {
        result = APIDimOrigin_StoredOrigin;
    } else if (origin == "Altitude") {
        result = APIDimOrigin_Altitude;
    } else {
        return false;
    }
    return true;
}

// The settings of a Level Dimension that CreateLevelDimensions and ModifyLevelDimensions both take,
// written by ApplyLevelDimensionSettings (#2220).
static const char* LevelDimensionSettingsSchemaProperties = R"(
                    "pen": {
                        "type": "integer",
                        "minimum": 1,
                        "maximum": 255,
                        "description": "Pen index of the marker. Optional parameter."
                    },
                    "textHeight": {
                        "type": "number",
                        "exclusiveMinimum": 0,
                        "description": "Height of the dimension text in millimeters (paper size), applied to both texts of the marker. Optional parameter."
                    },
                    "textPen": {
                        "type": "integer",
                        "minimum": 1,
                        "maximum": 255,
                        "description": "Pen index of the dimension text, applied to both texts of the marker. Optional parameter."
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
                        "exclusiveMinimum": 0,
                        "description": "Size of the marker in millimeters (paper size). Optional parameter."
                    },
                    "angle": {
                        "type": "number",
                        "description": "Rotation angle of the marker in radians. Optional parameter."
                    },
                    "origin": {
                        "$ref": "#/DimensionOrigin",
                        "description": "The origin the level is measured from. Optional parameter."
                    },
                    "fontName": {
                        "type": "string",
                        "description": "Font of the dimension text (Text Style of the Level Dimension settings), applied to both texts of the marker: the name of a Font attribute, as GetDetailsOfElements reports it in fontName, e.g. 'Arial'. The command fails when no loaded font has the name. Takes precedence over fontIndex if both are given. Optional parameter."
                    },
                    "fontIndex": {
                        "type": "integer",
                        "description": "Font of the dimension text given by its Font attribute index, applied to both texts of the marker. Optional parameter."
                    },
                    "bold": {
                        "type": "boolean",
                        "description": "Bold dimension text. Optional parameter, the other styles are kept."
                    },
                    "italic": {
                        "type": "boolean",
                        "description": "Italic dimension text. Optional parameter, the other styles are kept."
                    },
                    "underline": {
                        "type": "boolean",
                        "description": "Underlined dimension text. Optional parameter, the other styles are kept."
                    },
                    "hideWithBaseElem": {
                        "type": "boolean",
                        "description": "Hide the dimension together with its associated element (the 'Hide with associated element' setting). Available from Archicad 29; on earlier versions the command fails when it is given. Optional parameter."
                    },
                    "textContent": {
                        "type": "string",
                        "enum": ["Measured", "Custom"],
                        "description": "Content of the dimension text: 'Measured' shows the level value, 'Custom' shows customText. Optional parameter; giving customText alone also switches to 'Custom'."
                    },
                    "customText": {
                        "type": "string",
                        "description": "The custom content of the dimension text, own text and Autotext keys, e.g. 'FBOK <MEASUREDVALUE>'. Write the Autotext keys as GetDetailsOfElements returns them in customText for a level dimension set up in Archicad with the wanted Autotext (they are read with the autotext flag off). Sets textContent to 'Custom'; fails together with textContent 'Measured'. Optional parameter."
                    },
                    "secondTextContent": {
                        "type": "string",
                        "enum": ["Measured", "Custom"],
                        "description": "Content of the second text, shown by the marker types with two texts: 'Measured' or 'Custom' (secondCustomText). Optional parameter; giving secondCustomText alone also switches to 'Custom'."
                    },
                    "secondCustomText": {
                        "type": "string",
                        "description": "The custom content of the second text, own text and Autotext keys as for customText. Sets secondTextContent to 'Custom'; fails together with secondTextContent 'Measured'. Optional parameter."
                    })";

GS::UniString LevelDimensionContentTypeToString (API_NoteContentType contentType)
{
    return contentType == API_NoteContent_Custom ? "Custom" : "Measured";
}

// Applies textContent/customText (or the second text's keys) to one note. The custom text is
// passed through contentUStr, as the DevKit's Element_Test does in Do_LevelDimension_Edit.
static GS::Optional<GS::ObjectState> ApplyLevelDimensionNoteContent (
    const GS::ObjectState& parameters,
    const char* contentTypeKey,
    const char* customTextKey,
    API_NoteType& note,
    GS::UniString& textStorage,
    bool& contentTypeChanged,
    bool& textChanged)
{
    contentTypeChanged = false;
    textChanged = false;

    const GS::UniString contentTypeName (contentTypeKey);
    const GS::UniString customTextName (customTextKey);

    GS::UniString contentTypeStr;
    const bool hasContentType = parameters.Get (contentTypeKey, contentTypeStr);
    const bool isMeasured = hasContentType && contentTypeStr == "Measured";
    const bool isCustom = hasContentType && contentTypeStr == "Custom";
    if (hasContentType && !isMeasured && !isCustom) {
        return CreateErrorResponse (APIERR_BADPARS, "'" + contentTypeName + "' must be 'Measured' or 'Custom', not '" + contentTypeStr + "'.");
    }
    const bool hasCustomText = parameters.Get (customTextKey, textStorage);
    if (hasCustomText && isMeasured) {
        return CreateErrorResponse (APIERR_BADPARS, "'" + customTextName + "' cannot be given with '" + contentTypeName + "' 'Measured'.");
    }

    if (hasContentType || hasCustomText) {
        note.contentType = (hasCustomText || isCustom) ? API_NoteContent_Custom : API_NoteContent_Measured;
        contentTypeChanged = true;
    }
    if (hasCustomText) {
        note.contentUStr = &textStorage;
        textChanged = true;
    }
    return {};
}

GS::Optional<GS::ObjectState> ApplyLevelDimensionSettings (const GS::ObjectState& parameters, API_Element& element, API_Element* mask, LevelDimensionTexts& texts)
{
    Int32 pen = 0;
    if (parameters.Get ("pen", pen)) {
        if (pen < 1 || pen > 255) {
            return CreateErrorResponse (APIERR_BADPARS, "'pen' must be between 1 and 255");
        }
        element.levelDimension.pen = static_cast<short> (pen);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, pen);
        }
    }

    double textHeight = 0.0;
    if (parameters.Get ("textHeight", textHeight)) {
        if (textHeight <= 0.0) {
            return CreateErrorResponse (APIERR_BADPARS, "'textHeight' must be greater than 0");
        }
        element.levelDimension.note1.noteSize = textHeight;
        element.levelDimension.note2.noteSize = textHeight;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.noteSize);
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.noteSize);
        }
    }

    Int32 textPen = 0;
    if (parameters.Get ("textPen", textPen)) {
        if (textPen < 1 || textPen > 255) {
            return CreateErrorResponse (APIERR_BADPARS, "'textPen' must be between 1 and 255");
        }
        element.levelDimension.note1.notePen = static_cast<short> (textPen);
        element.levelDimension.note2.notePen = static_cast<short> (textPen);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.notePen);
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.notePen);
        }
    }

    Int32 dimForm = 0;
    if (parameters.Get ("dimForm", dimForm)) {
        // Archicad 29 knows the marker types 0-19 and silently replaces any other value by 0.
        if (dimForm < 0 || dimForm > 19) {
            return CreateErrorResponse (APIERR_BADPARS, "'dimForm' must be between 0 and 19");
        }
        element.levelDimension.dimForm = static_cast<short> (dimForm);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, dimForm);
        }
    }

    if (parameters.Get ("needPlus", element.levelDimension.needPlus) && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, needPlus);
    }

    double markerSize = 0.0;
    if (parameters.Get ("markerSize", markerSize)) {
        if (markerSize <= 0.0) {
            return CreateErrorResponse (APIERR_BADPARS, "'markerSize' must be greater than 0");
        }
        element.levelDimension.markerSize = markerSize;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, markerSize);
        }
    }

    if (parameters.Get ("angle", element.levelDimension.angle) && mask != nullptr) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, angle);
    }

    GS::UniString originStr;
    if (parameters.Get ("origin", originStr)) {
        // The schema can't reject an unknown value behind "$ref": "#/DimensionOrigin".
        API_DimOriginID origin = APIDimOrigin_ProjectZero;
        if (!ParseDimensionOriginString (originStr, origin)) {
            return CreateErrorResponse (APIERR_BADPARS, "Unknown 'origin' value: " + originStr);
        }
        element.levelDimension.origin = origin;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, origin);
        }
    }

    // The Text Style of the settings dialog: one font and style for both texts of the marker,
    // as textHeight and textPen (#2656).
    GS::Optional<short> fontIndex;
    const auto fontErr = TextLabelDetails::GetFontIndexFromDetails (parameters, fontIndex);
    if (fontErr.HasValue ()) {
        return fontErr;
    }
    if (fontIndex.HasValue ()) {
        element.levelDimension.note1.noteFont = *fontIndex;
        element.levelDimension.note2.noteFont = *fontIndex;
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.noteFont);
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.noteFont);
        }
    }

    const unsigned short faceBits = element.levelDimension.note1.faceBits;
    bool bold = (faceBits & APIFace_Bold) != 0;
    bool italic = (faceBits & APIFace_Italic) != 0;
    bool underline = (faceBits & APIFace_Underline) != 0;
    bool faceChanged = false;
    faceChanged |= parameters.Get ("bold", bold);
    faceChanged |= parameters.Get ("italic", italic);
    faceChanged |= parameters.Get ("underline", underline);
    if (faceChanged) {
        // Only the three styles of the dialog are set; any other bit Archicad keeps there stays.
        const unsigned short styleBits = static_cast<unsigned short> (APIFace_Bold | APIFace_Italic | APIFace_Underline);
        const unsigned short newBits = static_cast<unsigned short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
        element.levelDimension.note1.faceBits = static_cast<unsigned short> ((element.levelDimension.note1.faceBits & ~styleBits) | newBits);
        element.levelDimension.note2.faceBits = static_cast<unsigned short> ((element.levelDimension.note2.faceBits & ~styleBits) | newBits);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.faceBits);
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.faceBits);
        }
    }

    if (parameters.Contains ("hideWithBaseElem")) {
#ifdef ServerMainVers_2900
        parameters.Get ("hideWithBaseElem", element.levelDimension.hideWithBaseElem);
        if (mask != nullptr) {
            ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, hideWithBaseElem);
        }
#else
        // The Level Dimension of the Archicad 25-28 API has no hideWithBaseElem field.
        return CreateErrorResponse (APIERR_NOTSUPPORTED, "'hideWithBaseElem' of a level dimension is available from Archicad 29.");
#endif
    }

    // The Text content of the settings dialog, for the main and the second text (#2656).
    bool contentTypeChanged = false;
    bool textChanged = false;
    auto contentErr = ApplyLevelDimensionNoteContent (parameters, "textContent", "customText", element.levelDimension.note1, texts.text1, contentTypeChanged, textChanged);
    if (contentErr.HasValue ()) {
        return contentErr;
    }
    if (mask != nullptr && contentTypeChanged) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.contentType);
    }
    if (mask != nullptr && textChanged) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.content);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note1.contentUStr);
    }

    contentErr = ApplyLevelDimensionNoteContent (parameters, "secondTextContent", "secondCustomText", element.levelDimension.note2, texts.text2, contentTypeChanged, textChanged);
    if (contentErr.HasValue ()) {
        return contentErr;
    }
    if (mask != nullptr && contentTypeChanged) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.contentType);
    }
    if (mask != nullptr && textChanged) {
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.content);
        ACAPI_ELEMENT_MASK_SET (*mask, API_LevelDimensionType, note2.contentUStr);
    }

    return {};
}

// The note's content: the string Archicad filled through contentUStr when it did, else its char
// content (at most 64 bytes, not always terminated).
static GS::UniString GetLevelDimensionNoteContent (const API_NoteType& note, const GS::UniString& ownString)
{
    if (note.contentUStr == &ownString && !ownString.IsEmpty ()) {
        return ownString;
    }
    if (note.contentUStr != nullptr && note.contentUStr != &ownString) {
        return *note.contentUStr;
    }
    USize length = 0;
    while (length < sizeof (note.content) && note.content[length] != '\0') {
        ++length;
    }
    return GS::UniString (note.content, length, CC_UTF8);
}

LevelDimensionTexts GetLevelDimensionTexts (const API_Element& element)
{
    LevelDimensionTexts texts;
    texts.text1 = GetLevelDimensionNoteContent (element.levelDimension.note1, texts.text1);
    texts.text2 = GetLevelDimensionNoteContent (element.levelDimension.note2, texts.text2);

    // Read again with the autotext flag off, in which an autotext reads as its key (as
    // GetRawTextContent does for texts), asking for the content as UniString through contentUStr.
    bool savedAutoTextFlag = false;
    if (ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag) != NoError) {
        return texts;
    }
    if (savedAutoTextFlag) {
        bool keysAutoTextFlag = false;
        if (ACAPI_AutoText_ChangeAutoTextFlag (&keysAutoTextFlag) != NoError) {
            return texts;
        }
    }
    const GS::OnExit flagGuard ([&savedAutoTextFlag] () {
        if (savedAutoTextFlag) {
            ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag);
        }
    });

    GS::UniString content1;
    GS::UniString content2;
    API_Element reread = {};
    reread.header.guid = element.header.guid;
    reread.levelDimension.note1.contentUStr = &content1;
    reread.levelDimension.note2.contentUStr = &content2;
    if (ACAPI_Element_Get (&reread) != NoError) {
        return texts;
    }
    texts.text1 = GetLevelDimensionNoteContent (reread.levelDimension.note1, content1);
    texts.text2 = GetLevelDimensionNoteContent (reread.levelDimension.note2, content2);
    return texts;
}

CreateLevelDimensionsCommand::CreateLevelDimensionsCommand () :
    CreateElementsCommandBase ("CreateLevelDimensions", API_LevelDimensionID, "levelDimensionsData")
{
}

GS::Optional<GS::UniString> CreateLevelDimensionsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
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
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
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
                    },)") + LevelDimensionSettingsSchemaProperties + R"(
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
})";
}

GS::Optional<GS::ObjectState> CreateLevelDimensionsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories&, const GS::ObjectState& parameters) const
{
    const GS::ObjectState* positionOS = parameters.Get ("position");
    if (positionOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'position' parameter");
    }
    element.levelDimension.loc = Get2DCoordinateFromObjectState (*positionOS);

    // An associative level dimension refers to its parent by type and GUID; Archicad then
    // measures the parent's height at loc itself and keeps the value up to date (the same
    // way the Level Dimension tool does when clicked on a mesh or slab). Without a parent
    // the dimension refers to the floor (API_ZombieElemID), or holds a static level.
    const API_Guid parentGuid = GetGuidFromArrayItem ("parentElementId", parameters);
    if (parentGuid != APINULLGuid) {
        API_Elem_Head parentElemHead = {};
        parentElemHead.guid = parentGuid;
        if (ACAPI_Element_GetHeader (&parentElemHead) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid parent element GUID");
        }
#ifdef ServerMainVers_2600
        element.levelDimension.parentType = parentElemHead.type;
#else
        element.levelDimension.parentType = parentElemHead.typeID;
#endif
        element.levelDimension.parentGuid = parentGuid;
        element.levelDimension.staticLevel = false;
        element.header.floorInd = parentElemHead.floorInd;
    } else {
        element.levelDimension.parentType = API_ZombieElemID;
        element.levelDimension.parentGuid = APINULLGuid;
        double level = 0.0;
        if (parameters.Get ("level", level)) {
            element.levelDimension.staticLevel = true;
            element.levelDimension.level = level;
        }
    }

    Int32 floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = static_cast<short> (floorIndex);
    }

    Int32 layerIndex = 0;
    if (parameters.Get ("layerIndex", layerIndex) && layerIndex > 0) {
        element.header.layer = ACAPI_CreateAttributeIndex (layerIndex);
    }

    // ACAPI_Element_Create reads the custom texts after this returns, so they are kept in the command.
    customTexts = {};
    return ApplyLevelDimensionSettings (parameters, element, nullptr, customTexts);
}

CreateAngleDimensionsCommand::CreateAngleDimensionsCommand () :
    CreateElementsCommandBase ("CreateAngleDimensions", API_AngleDimensionID, "angleDimensionsData")
{
}

GS::Optional<GS::UniString> CreateAngleDimensionsCommand::GetInputParametersSchema () const
{
    return R"({
    "type": "object",
    "properties": {
        "angleDimensionsData": {
            "type": "array",
            "description": "Array of data to create Angle Dimensions (the Angle Dimension tool). They are created in the current database (the floor plan, or the section, elevation, interior elevation, worksheet or detail whose window is active), or in the one given by databaseId.",
            "items": {
                "type": "object",
                "description": "The parameters of the new Angle Dimension: the angle between two lines, each given by two points. The dimension is static: it is not associated with the elements the points lie on.",
                "properties": {
                    "favoriteName": {
                        "type": "string",
                        "description": "Optional name of an Angle Dimension favorite to base the new element on. Its settings (layer, pens, markers, text style) are applied first, then the explicitly given fields override them."
                    },
                    "id": {
                        "type": "string",
                        "description": "Optional Element ID of the new element (the ID field of the Settings dialog, shown in schedules and labels; the same string GetDetailsOfElements returns as id). When omitted the element gets the next automatic ID of its tool."
                    },
                    "firstLine": {
                        "$ref": "#/AngleDimensionLine",
                        "description": "The first measured line."
                    },
                    "secondLine": {
                        "$ref": "#/AngleDimensionLine",
                        "description": "The second measured line. It must not be parallel to the first one."
                    },
                    "arcPosition": {
                        "$ref": "#/Coordinate2D",
                        "description": "A point the dimension arc passes through. Its distance from the intersection of the two lines is the radius of the arc, and the side it lies on chooses which of the angles between the lines is measured, as clicking the arc's position does with the Angle Dimension tool."
                    },
                    "smallArc": {
                        "type": "boolean",
                        "description": "Measure the smaller angle between the lines (inner dimension) rather than the larger one (outer dimension). Optional parameter, by default the favorite's or the tool default's setting."
                    },
                    "floorIndex": {
                        "type": "integer",
                        "description": "The index of the floor to place the dimension on, when it is created on the floor plan. Optional parameter, by default the current floor is used."
                    },
                    "layerIndex": {
                        "type": "integer",
                        "description": "Layer attribute index to place the dimension on. Optional parameter, by default the layer of the favorite or the Angle Dimension tool default is used."
                    },
                    "pen": {
                        "type": "integer",
                        "minimum": 1,
                        "maximum": 255,
                        "description": "Pen index of the dimension and witness lines. Optional parameter."
                    },
                    "textHeight": {
                        "type": "number",
                        "exclusiveMinimum": 0,
                        "description": "Height of the dimension text in millimeters (paper size). Optional parameter."
                    },
                    "textPen": {
                        "type": "integer",
                        "minimum": 1,
                        "maximum": 255,
                        "description": "Pen index of the dimension text. Optional parameter."
                    }
                },
                "additionalProperties": false,
                "required": [
                    "firstLine",
                    "secondLine",
                    "arcPosition"
                ]
            }
        },
        "databaseId": {
            "$ref": "#/DatabaseId",
            "description": "Optional database to create the elements in, e.g. a section, an elevation, a worksheet or a detail (see GetDatabases). The front window stays as it is: a worksheet or detail whose window was never opened is opened once on Archicad 27 and later, and the front window is brought back. Without it the elements are created in the current database. If Archicad cannot switch to it, nothing is created and the call fails."
        }
    },
    "additionalProperties": false,
    "required": [
        "angleDimensionsData"
    ]
})";
}

static GS::Optional<GS::ObjectState> GetAngleDimensionLine (const GS::ObjectState& parameters, const char* name, API_Coord& beg, API_Coord& end)
{
    const GS::ObjectState* lineOS = parameters.Get (name);
    const GS::ObjectState* begOS = lineOS != nullptr ? lineOS->Get ("begCoordinate") : nullptr;
    const GS::ObjectState* endOS = lineOS != nullptr ? lineOS->Get ("endCoordinate") : nullptr;
    if (begOS == nullptr || endOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Missing '") + name + "' with 'begCoordinate' and 'endCoordinate'.");
    }
    beg = Get2DCoordinateFromObjectState (*begOS);
    end = Get2DCoordinateFromObjectState (*endOS);
    if (std::hypot (end.x - beg.x, end.y - beg.y) < 1e-9) {
        return CreateErrorResponse (APIERR_BADPARS, GS::UniString ("Zero-length '") + name + "': 'begCoordinate' and 'endCoordinate' are identical.");
    }
    return {};
}

GS::Optional<GS::ObjectState> CreateAngleDimensionsCommand::SetTypeSpecificParameters (API_Element& element, API_ElementMemo& /*memo*/, const Stories&, const GS::ObjectState& parameters) const
{
    API_Coord a1 = {}, a2 = {}, b1 = {}, b2 = {};
    {
        auto error = GetAngleDimensionLine (parameters, "firstLine", a1, a2);
        if (error.HasValue ()) {
            return error;
        }
    }
    {
        auto error = GetAngleDimensionLine (parameters, "secondLine", b1, b2);
        if (error.HasValue ()) {
            return error;
        }
    }

    const GS::ObjectState* arcPositionOS = parameters.Get ("arcPosition");
    if (arcPositionOS == nullptr) {
        return CreateErrorResponse (APIERR_BADPARS, "Missing 'arcPosition' parameter.");
    }
    const API_Coord arcPosition = Get2DCoordinateFromObjectState (*arcPositionOS);

    // The center of the dimension arc is the intersection of the two (infinite) lines,
    // as the Angle Dimension tool puts it.
    const double d1x = a2.x - a1.x, d1y = a2.y - a1.y;
    const double d2x = b2.x - b1.x, d2y = b2.y - b1.y;
    const double cross = d1x * d2y - d1y * d2x;
    if (std::fabs (cross) < 1e-9 * std::hypot (d1x, d1y) * std::hypot (d2x, d2y)) {
        return CreateErrorResponse (APIERR_BADPARS, "'firstLine' and 'secondLine' are parallel, so they have no angle to dimension.");
    }
    const double t = ((b1.x - a1.x) * d2y - (b1.y - a1.y) * d2x) / cross;
    API_Coord origo = {};
    origo.x = a1.x + t * d1x;
    origo.y = a1.y + t * d1y;
    if (std::hypot (arcPosition.x - origo.x, arcPosition.y - origo.y) < 1e-9) {
        return CreateErrorResponse (APIERR_BADPARS, "'arcPosition' is the intersection of the two lines; it must be away from it, on the dimension arc.");
    }

    // A static angle dimension, as Do_CreateStaticAngleDimension of the DevKit's Element_Test
    // example creates it: the four reference points, the arc position and its center.
    element.angleDimension.base[0].loc = a1;
    element.angleDimension.base[1].loc = a2;
    element.angleDimension.base[2].loc = b1;
    element.angleDimension.base[3].loc = b2;
    element.angleDimension.pos = arcPosition;
    element.angleDimension.origo = origo;

    parameters.Get ("smallArc", element.angleDimension.smallArc);

    Int32 floorIndex = 0;
    if (parameters.Get ("floorIndex", floorIndex)) {
        element.header.floorInd = static_cast<short> (floorIndex);
    }

    const GS::Optional<GS::ObjectState> layerError = ApplyLayerIndexFromParameters (parameters, element);
    if (layerError.HasValue ()) {
        return layerError;
    }

    Int32 pen = 0;
    if (parameters.Get ("pen", pen)) {
        if (pen < 1 || pen > 255) {
            return CreateErrorResponse (APIERR_BADPARS, "'pen' must be between 1 and 255");
        }
        element.angleDimension.linPen = static_cast<short> (pen);
    }

    double textHeight = 0.0;
    if (parameters.Get ("textHeight", textHeight)) {
        if (textHeight <= 0.0) {
            return CreateErrorResponse (APIERR_BADPARS, "'textHeight' must be greater than 0");
        }
        element.angleDimension.note.noteSize = textHeight;
    }

    Int32 textPen = 0;
    if (parameters.Get ("textPen", textPen)) {
        if (textPen < 1 || textPen > 255) {
            return CreateErrorResponse (APIERR_BADPARS, "'textPen' must be between 1 and 255");
        }
        element.angleDimension.note.notePen = static_cast<short> (textPen);
    }

    return {};
}

// With a section or elevation window active, ACAPI_Element_Create of a dimension can report
// success and hand back a GUID although no element was created (#510): the new element is
// read back, so the caller gets an error instead of an id that resolves to nothing.
GSErrCode CreateAngleDimensionsCommand::AfterCreate (const API_Element& /*requestedElement*/, const API_Guid& newElementGuid, const GS::ObjectState& /*parameters*/) const
{
    API_Element created = {};
    created.header.guid = newElementGuid;
    return ACAPI_Element_Get (&created) == NoError ? NoError : APIERR_GENERAL;
}

ModifyLevelDimensionsCommand::ModifyLevelDimensionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String ModifyLevelDimensionsCommand::GetName () const
{
    return "ModifyLevelDimensions";
}

GS::Optional<GS::UniString> ModifyLevelDimensionsCommand::GetInputParametersSchema () const
{
    return GS::UniString (R"({
    "type": "object",
    "properties": {
        "levelDimensionsWithDetails": {
            "type": "array",
            "description": "The Level Dimensions to change, each with the settings to change. Only the given settings are changed.",
            "items": {
                "type": "object",
                "properties": {
                    "elementId": {
                        "$ref": "#/ElementId"
                    },
                    "position": {
                        "$ref": "#/Coordinate2D",
                        "description": "The new location of the marker on the floor plan. An associative level dimension then shows the height of its element at the new position. Optional parameter."
                    },
                    "level": {
                        "type": "number",
                        "description": "New fixed level value in meters (absolute, from the project zero), as CreateLevelDimensions takes it. The dimension becomes static (the Static checkbox on) and shows this value; an owner element is kept, so staticLevel false later shows its height again. Cannot be given with parentElementId or with staticLevel false. Optional parameter."
                    },
                    "staticLevel": {
                        "type": "boolean",
                        "description": "The Static checkbox. true fixes the value the dimension shows now; false makes it associative again, so it shows the height of its owner element at the position, or the floor level when it has none. Optional parameter."
                    },
                    "parentElementId": {
                        "$ref": "#/ElementId",
                        "description": "The new owner element (Mesh, Slab, Roof, Shell, Morph, Beam, Column, Wall, Object, Stair ...), for example after the old one was replaced. The dimension becomes associative and shows the new owner's height at the position, so the position must lie on it (give position too when it does not). Cannot be given with level or with staticLevel true. Optional parameter."
                    },)") + LevelDimensionSettingsSchemaProperties + R"(
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
        "levelDimensionsWithDetails"
    ]
})";
}

GS::Optional<GS::UniString> ModifyLevelDimensionsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "executionResults": { "$ref": "#/ExecutionResults" }
        },
        "additionalProperties": false,
        "required": ["executionResults"]
    })";
}

// The static level, the Static checkbox and the owner element of an existing level dimension (#2656):
// level makes it static with that value (the owner is kept), staticLevel switches the checkbox, and
// parentElementId sets a new owner the way CreateLevelDimensions does and makes it associative.
static GS::Optional<GS::ObjectState> ApplyLevelDimensionLevelAndOwner (const GS::ObjectState& item, API_Element& element, API_Element& mask)
{
    double level = 0.0;
    const bool hasLevel = item.Get ("level", level);
    bool staticLevel = false;
    const bool hasStaticLevel = item.Get ("staticLevel", staticLevel);
    const bool hasParent = item.Contains ("parentElementId");

    if (hasLevel && hasStaticLevel && !staticLevel) {
        return CreateErrorResponse (APIERR_BADPARS, "level makes the level dimension static, it cannot be given with staticLevel false.");
    }
    if (hasParent && (hasLevel || (hasStaticLevel && staticLevel))) {
        return CreateErrorResponse (APIERR_BADPARS, "parentElementId makes the level dimension associative, it cannot be given with level or with staticLevel true.");
    }

    if (hasParent) {
        const API_Guid parentGuid = GetGuidFromArrayItem ("parentElementId", item);
        API_Elem_Head parentElemHead = {};
        parentElemHead.guid = parentGuid;
        if (parentGuid == APINULLGuid || parentGuid == element.header.guid || ACAPI_Element_GetHeader (&parentElemHead) != NoError) {
            return CreateErrorResponse (APIERR_BADPARS, "Invalid parent element GUID");
        }
#ifdef ServerMainVers_2600
        element.levelDimension.parentType = parentElemHead.type;
#else
        element.levelDimension.parentType = parentElemHead.typeID;
#endif
        element.levelDimension.parentGuid = parentGuid;
        element.levelDimension.staticLevel = false;
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, parentType);
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, parentGuid);
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, staticLevel);
    }

    if (hasLevel) {
        element.levelDimension.level = level;
        element.levelDimension.staticLevel = true;
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, level);
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, staticLevel);
    } else if (hasStaticLevel) {
        element.levelDimension.staticLevel = staticLevel;
        ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, staticLevel);
    }

    return {};
}

// The settings go through ApplyLevelDimensionSettings, as in CreateLevelDimensions; the position
// through loc, which ACAPI_Element_Change accepts for level dimensions (as the DevKit's Element_Test
// does in Do_LevelDimension_Edit). The font and style go to both texts, like the text height and pen.
// The texts' content is written with the autotext flag off, as CreateElementsCommandBase does, so an
// Autotext key is kept as a key. The static level, the Static checkbox and the owner element go through
// ApplyLevelDimensionLevelAndOwner. The element is read back, so a value Archicad did not take is
// reported as a failure.
GS::ObjectState ModifyLevelDimensionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> itemsWithDetails;
    parameters.Get ("levelDimensionsWithDetails", itemsWithDetails);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ElementChangeReporter changeReporter;
    changeReporter.AddElementIdsOf (itemsWithDetails);

    ACAPI_CallUndoableCommand ("ModifyLevelDimensions", [&] () -> GSErrCode {
        bool savedAutoTextFlag = false;
        ACAPI_AutoText_GetAutoTextFlag (&savedAutoTextFlag);
        bool setAutoTextFlag = false;
        ACAPI_AutoText_ChangeAutoTextFlag (&setAutoTextFlag);
        const GS::OnExit autoTextGuard ([&savedAutoTextFlag] () { ACAPI_AutoText_ChangeAutoTextFlag (&savedAutoTextFlag); });

        for (const GS::ObjectState& item : itemsWithDetails) {
            const GS::ObjectState* elementId = item.Get ("elementId");
            if (elementId == nullptr) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "elementId is missing"));
                continue;
            }

            API_Element element = {};
            element.header.guid = GetGuidFromObjectState (*elementId);
            GSErrCode err = ACAPI_Element_Get (&element);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to find the element"));
                continue;
            }
            if (GetElemTypeId (element.header) != API_LevelDimensionID) {
                executionResults (CreateFailedExecutionResult (APIERR_BADID, "Element is not a Level Dimension."));
                continue;
            }

            API_Element mask = {};
            ACAPI_ELEMENT_MASK_CLEAR (mask);

            const GS::ObjectState* positionOS = item.Get ("position");
            if (positionOS != nullptr) {
                element.levelDimension.loc = Get2DCoordinateFromObjectState (*positionOS);
                ACAPI_ELEMENT_MASK_SET (mask, API_LevelDimensionType, loc);
            }

            auto levelError = ApplyLevelDimensionLevelAndOwner (item, element, mask);
            if (levelError.HasValue ()) {
                executionResults (CreateFailedExecutionResult (levelError.Get ()));
                continue;
            }

            LevelDimensionTexts customTexts;
            auto settingsError = ApplyLevelDimensionSettings (item, element, &mask, customTexts);
            if (settingsError.HasValue ()) {
                executionResults (CreateFailedExecutionResult (settingsError.Get ()));
                continue;
            }
            if (item.GetFieldCount () <= 1) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "No level dimension settings to modify."));
                continue;
            }

            err = ACAPI_Element_Change (&element, &mask, nullptr, 0, true);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to modify the level dimension."));
                continue;
            }

            // Read back, so a value Archicad did not take is reported instead of a success.
            API_Element changed = {};
            changed.header.guid = element.header.guid;
            err = ACAPI_Element_Get (&changed);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to read the level dimension back after the change."));
                continue;
            }
            const double tolerance = 1e-6;
            GS::UniString notApplied;
            if (positionOS != nullptr &&
                (std::abs (changed.levelDimension.loc.x - element.levelDimension.loc.x) > tolerance ||
                 std::abs (changed.levelDimension.loc.y - element.levelDimension.loc.y) > tolerance)) {
                notApplied += " position";
            }
            if (item.Contains ("level") && std::abs (changed.levelDimension.level - element.levelDimension.level) > tolerance) {
                notApplied += " level";
            }
            if (item.Contains ("staticLevel") && changed.levelDimension.staticLevel != element.levelDimension.staticLevel) {
                notApplied += " staticLevel";
            }
            if (item.Contains ("parentElementId") && changed.levelDimension.parentGuid != element.levelDimension.parentGuid) {
                notApplied += " parentElementId";
            }
            if (item.Contains ("pen") && changed.levelDimension.pen != element.levelDimension.pen) {
                notApplied += " pen";
            }
            if (item.Contains ("dimForm") && changed.levelDimension.dimForm != element.levelDimension.dimForm) {
                notApplied += " dimForm";
            }
            if (item.Contains ("textPen") && changed.levelDimension.note1.notePen != element.levelDimension.note1.notePen) {
                notApplied += " textPen";
            }
            if (item.Contains ("textHeight") && std::abs (changed.levelDimension.note1.noteSize - element.levelDimension.note1.noteSize) > tolerance) {
                notApplied += " textHeight";
            }
            if (item.Contains ("markerSize") && std::abs (changed.levelDimension.markerSize - element.levelDimension.markerSize) > tolerance) {
                notApplied += " markerSize";
            }
            if (item.Contains ("origin") && changed.levelDimension.origin != element.levelDimension.origin) {
                notApplied += " origin";
            }
            if ((item.Contains ("fontName") || item.Contains ("fontIndex")) && changed.levelDimension.note1.noteFont != element.levelDimension.note1.noteFont) {
                notApplied += " font";
            }
            const unsigned short styleBits = static_cast<unsigned short> (APIFace_Bold | APIFace_Italic | APIFace_Underline);
            if ((item.Contains ("bold") || item.Contains ("italic") || item.Contains ("underline")) &&
                (changed.levelDimension.note1.faceBits & styleBits) != (element.levelDimension.note1.faceBits & styleBits)) {
                notApplied += " text style";
            }
#ifdef ServerMainVers_2900
            if (item.Contains ("hideWithBaseElem") && changed.levelDimension.hideWithBaseElem != element.levelDimension.hideWithBaseElem) {
                notApplied += " hideWithBaseElem";
            }
#endif
            if ((item.Contains ("textContent") || item.Contains ("customText")) && changed.levelDimension.note1.contentType != element.levelDimension.note1.contentType) {
                notApplied += " textContent";
            }
            if ((item.Contains ("secondTextContent") || item.Contains ("secondCustomText")) && changed.levelDimension.note2.contentType != element.levelDimension.note2.contentType) {
                notApplied += " secondTextContent";
            }
            // The custom text itself is not compared: Archicad may write an Autotext key differently
            // from how it was given. GetDetailsOfElements reads it back in customText.
            if (!notApplied.IsEmpty ()) {
                executionResults (CreateFailedExecutionResult (APIERR_REFUSEDPAR, "Archicad did not apply:" + notApplied + "."));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
            changeReporter.MarkChanged (element.header.guid);
        }
        return NoError;
    });

    changeReporter.Report ();

    return response;
}

MeasureTextsCommand::MeasureTextsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String MeasureTextsCommand::GetName () const
{
    return "MeasureTexts";
}

GS::Optional<GS::UniString> MeasureTextsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "texts": {
                "type": "array",
                "description": "The texts to measure, each with the font, style and height it would be placed with. Nothing is created.",
                "items": {
                    "type": "object",
                    "properties": {
                        "text": { "type": "string", "description": "The text to measure. Newlines separate lines, each measured on its own; Windows-style CRLF (and bare CR) line endings are treated as LF." },
                        "fontIndex": { "type": "integer", "description": "The index of the font, as GetDetailsOfElements and GetFontsByName report it in fontIndex. One of fontName and fontIndex is required." },
                        "fontName": { "type": "string", "description": "Name of the font as Archicad's font list carries it (GetDetailsOfElements reports it in fontName), which is not always the operating system's font name; GetFontsByName tells whether a name is found. Resolved to the font's index (exact match first, then case-insensitive, then Archicad's font search as GetFontsByName does, so a family name such as 'Arial' finds 'Arial Western'); the text fails when no loaded font has the name. Takes precedence over fontIndex if both are given. One of fontName and fontIndex is required." },
                        "height": { "type": "number", "description": "The character height in millimetres, as a Text's height." },
                        "bold": { "type": "boolean", "description": "Bold style. Optional, default false." },
                        "italic": { "type": "boolean", "description": "Italic style. Optional, default false." },
                        "underline": { "type": "boolean", "description": "Underline style. Optional, default false." },
                        "slant": { "type": "number", "description": "The slant angle of the characters in radians; pi/2 (the default) is upright text." },
                        "inModelUnits": { "type": "boolean", "description": "When true, Archicad applies the drawing scale correction and the lengths are model distances at the current drawing scale; when false (the default), they are lengths on paper in millimetres." }
                    },
                    "additionalProperties": false,
                    "required": ["text", "height"],
                    "anyOf": [
                        { "required": ["fontIndex"] },
                        { "required": ["fontName"] }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["texts"]
    })";
}

GS::Optional<GS::UniString> MeasureTextsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "measurements": {
                "type": "array",
                "description": "One item per input text, in the same order. The measurement takes no width factor or character spacing into account.",
                "items": {
                    "type": "object",
                    "oneOf": [
                        {
                            "$ref": "#/TextMeasurement"
                        },
                        {
                            "$ref": "#/ErrorItem"
                        }
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": ["measurements"]
    })";
}

GS::ObjectState MeasureTextsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::Array<GS::ObjectState> texts;
    parameters.Get ("texts", texts);

    GS::ObjectState response;
    const auto& measurements = response.AddList<GS::ObjectState> ("measurements");

    for (const GS::ObjectState& item : texts) {
        GS::UniString text;
        double height = 0.0;
        if (!item.Get ("text", text) || !item.Get ("height", height)) {
            measurements (CreateErrorResponse (APIERR_BADPARS, "Each text requires 'text' and 'height'."));
            continue;
        }
        if (height <= 0.0) {
            measurements (CreateErrorResponse (APIERR_BADPARS, "'height' must be greater than zero."));
            continue;
        }

        GS::Optional<short> fontIndex;
        const auto fontErr = TextLabelDetails::GetFontIndexFromDetails (item, fontIndex);
        if (fontErr.HasValue ()) {
            measurements (*fontErr);
            continue;
        }
        if (!fontIndex.HasValue ()) {
            measurements (CreateErrorResponse (APIERR_BADPARS, "Each text requires 'fontName' or 'fontIndex'."));
            continue;
        }

        bool bold = false;
        bool italic = false;
        bool underline = false;
        item.Get ("bold", bold);
        item.Get ("italic", italic);
        item.Get ("underline", underline);
        double slant = 1.5707963267948966;
        item.Get ("slant", slant);
        bool inModelUnits = false;
        item.Get ("inModelUnits", inModelUnits);

        // Every line is measured on its own as a string without a line terminator (index -1),
        // so the result does not depend on how the API picks or reports the longest line.
        const GS::UniString normalized = NormalizeLineBreaks (text);
        GS::Array<GS::UniString> lines;
        UIndex lineStart = 0;
        for (UIndex i = 0; i < normalized.GetLength (); ++i) {
            if (normalized[i] == GS::UniChar ('\n')) {
                lines.Push (GS::UniString (normalized.GetSubstring (lineStart, i - lineStart)));
                lineStart = i + 1;
            }
        }
        lines.Push (GS::UniString (normalized.GetSubstring (lineStart, normalized.GetLength () - lineStart)));

        GS::Array<double> lineLengths;
        GSErrCode err = NoError;
        for (GS::UniString& line : lines) {
            double lineLength = 0.0;
            if (!line.IsEmpty ()) {
                API_TextLinePars textLinePars = {};
                textLinePars.index = -1;
                textLinePars.drvScaleCorr = inModelUnits;
                textLinePars.wantsLongestIndex = false;
                textLinePars.wFont = *fontIndex;
                textLinePars.wFace = static_cast<short> ((bold ? APIFace_Bold : 0) | (italic ? APIFace_Italic : 0) | (underline ? APIFace_Underline : 0));
                textLinePars.wSlant = slant;
                textLinePars.wSize = height;
                textLinePars.lineStr = nullptr;
                textLinePars.lineUniStr = &line;
                err = ACAPI_Element_GetTextLineLength (&textLinePars, &lineLength);
                if (err != NoError) {
                    break;
                }
            }
            lineLengths.Push (lineLength);
        }
        if (err != NoError) {
            measurements (CreateErrorResponse (err, "Failed to measure the text."));
            continue;
        }

        double longest = 0.0;
        UIndex longestLineIndex = 0;
        for (UIndex i = 0; i < lineLengths.GetSize (); ++i) {
            if (lineLengths[i] > longest) {
                longest = lineLengths[i];
                longestLineIndex = i;
            }
        }

        GS::ObjectState measurement;
        measurement.Add ("length", longest);
        measurement.Add ("longestLineIndex", static_cast<Int32> (longestLineIndex));
        const auto& lineLengthList = measurement.AddList<double> ("lineLengths");
        for (double lineLength : lineLengths) {
            lineLengthList (lineLength);
        }

        short sizeFactorFont = *fontIndex;
        double capitalSizeFactor = 0.0;
        if (ACAPI_Element_GetTextSizeFactor (&sizeFactorFont, &capitalSizeFactor) == NoError) {
            measurement.Add ("capitalSizeFactor", capitalSizeFactor);
        }

        measurements (measurement);
    }

    return response;
}
