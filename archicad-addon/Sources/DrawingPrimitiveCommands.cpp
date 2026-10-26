#include "DrawingPrimitiveCommands.hpp"
#include "ElementCreationCommands.hpp"
#include "MigrationHelper.hpp"

// ── Primitive collector ───────────────────────────────────────────────────────

// The shape primitive callback has no user pointer, so - like CollectCutFillPolygons in
// ElementCommands.cpp - it writes into a collector that is only set while the decomposition runs.
struct DrawingPrimitiveCollectorContext {
    GS::Array<GS::ObjectState> primitives;

    struct CachedAttributeGuid {
        API_AttrTypeID     typeID;
        API_AttributeIndex index;
        API_Guid           guid;
    };
    GS::Array<CachedAttributeGuid> attributeGuids;

    // An element is drawn from many primitives, nearly all on the same layer, with a handful of
    // line types and fills, so each attribute is looked up once.
    API_Guid GetAttributeGuid (API_AttrTypeID typeID, const API_AttributeIndex& index)
    {
        for (const CachedAttributeGuid& cached : attributeGuids) {
            if (cached.typeID == typeID && cached.index == index) {
                return cached.guid;
            }
        }
        const API_Guid guid = GetAttributeGuidFromIndex (typeID, index);
        const CachedAttributeGuid cached = { typeID, index, guid };
        attributeGuids.Push (cached);
        return guid;
    }
};

static thread_local DrawingPrimitiveCollectorContext* tl_drawingPrimitiveCtx = nullptr;

static const char* PrimitiveTypeToString (API_PrimTypeID typeID)
{
    switch (typeID) {
        case API_PrimPointID:                    return "Point";
        case API_PrimLineID:                     return "Line";
        case API_PrimArcID:                      return "Arc";
        case API_PrimTextID:                     return "Text";
        case API_PrimPLineID:                    return "PLine";
        case API_PrimTriID:                      return "Tri";
        case API_PrimPolyID:                     return "Poly";
        case API_PrimPictID:                     return "Pict";
        case API_PrimCtrl_BegID:                 return "Ctrl_Beg";
        case API_PrimCtrl_HatchBorderBegID:      return "Ctrl_HatchBorderBeg";
        case API_PrimCtrl_HatchBorderEndID:      return "Ctrl_HatchBorderEnd";
        case API_PrimCtrl_HatchLinesBegID:       return "Ctrl_HatchLinesBeg";
        case API_PrimCtrl_HatchLinesEndID:       return "Ctrl_HatchLinesEnd";
        case API_PrimCtrl_HoledimLinesBegID:     return "Ctrl_HoledimLinesBeg";
        case API_PrimCtrl_HoledimLinesEndID:     return "Ctrl_HoledimLinesEnd";
        case API_PrimCtrl_ArrowBegID:            return "Ctrl_ArrowBeg";
        case API_PrimCtrl_ArrowEndID:            return "Ctrl_ArrowEnd";
        case API_PrimCtrl_ElementRefID:          return "Ctrl_ElementRef";
        case API_PrimCtrl_EndID:                 return "Ctrl_End";
        case API_PrimCtrl_PlacedBorderBegID:     return "Ctrl_PlacedBorderBeg";
        case API_PrimCtrl_PlacedBorderEndID:     return "Ctrl_PlacedBorderEnd";
        case API_PrimCtrl_TextBegID:             return "Ctrl_TextBeg";
        case API_PrimCtrl_TextEndID:             return "Ctrl_TextEnd";
        case API_PrimCtrl_CWallPanelBegID:       return "Ctrl_CWallPanelBeg";
        case API_PrimCtrl_CWallPanelEndID:       return "Ctrl_CWallPanelEnd";
#ifdef ServerMainVers_2800
        case API_PrimCtrl_BackgroundFillBegID:   return "Ctrl_BackgroundFillBeg";
        case API_PrimCtrl_BackgroundFillEndID:   return "Ctrl_BackgroundFillEnd";
        case API_PrimCtrl_FrameBegID:            return "Ctrl_FrameBeg";
        case API_PrimCtrl_FrameEndID:            return "Ctrl_FrameEnd";
#endif
        default:                                 return "Unknown";
    }
}

static const char* RenovationStatusToString (API_RenovationStatusType status)
{
    switch (status) {
        case API_ExistingStatus:   return "Existing";
        case API_NewStatus:        return "New";
        case API_DemolishedStatus: return "Demolished";
        default:                   return "Undefined";
    }
}

static GS::ObjectState CreatePrimitiveHeadObjectState (const API_Prim_Head& head, DrawingPrimitiveCollectorContext& ctx)
{
    GS::ObjectState os ("type", PrimitiveTypeToString (head.typeID));
    const API_Guid layerGuid = ctx.GetAttributeGuid (API_LayerID, head.layer);
    if (layerGuid != APINULLGuid) {
        os.Add ("layer", CreateAttributeIdObjectState (layerGuid));
    }
    os.Add ("pen", head.pen.penIndex);
    os.Add ("drwIndex", head.drwIndex);
    os.Add ("renovationStatus", RenovationStatusToString (head.renovationStatus));
    return os;
}

static void AddLineTypeToObjectState (GS::ObjectState& os, const API_AttributeIndex& ltypeInd, DrawingPrimitiveCollectorContext& ctx)
{
    const API_Guid lineTypeGuid = ctx.GetAttributeGuid (API_LinetypeID, ltypeInd);
    if (lineTypeGuid != APINULLGuid) {
        os.Add ("lineType", CreateAttributeIdObjectState (lineTypeGuid));
    }
}

// coords is 1 based, as CollectCutFillPolygons reads it.
template <typename ListAdder>
static void AddCoordinatesToList (const ListAdder& list, const API_Coord* coords, Int32 begIndex, Int32 endIndex)
{
    for (Int32 i = begIndex; i <= endIndex; ++i) {
        list (Create2DCoordinateObjectState (coords[i]));
    }
}

static GSErrCode CollectDrawingPrimitives (const API_PrimElement* primElem,
                                           const void* par1, const void* par2, const void* /*par3*/)
{
    if (tl_drawingPrimitiveCtx == nullptr || primElem == nullptr)
        return NoError;

    DrawingPrimitiveCollectorContext& ctx = *tl_drawingPrimitiveCtx;
    GS::ObjectState os = CreatePrimitiveHeadObjectState (primElem->header, ctx);

    switch (primElem->header.typeID) {
        case API_PrimPointID:
            os.Add ("loc", Create2DCoordinateObjectState (primElem->point.loc));
            break;
        case API_PrimLineID: {
            const API_PrimLine& line = primElem->line;
            AddLineTypeToObjectState (os, line.ltypeInd, ctx);
            os.Add ("determination", line.determination);
            os.Add ("penWeight", line.penWeight);
            os.Add ("c1", Create2DCoordinateObjectState (line.c1));
            os.Add ("c2", Create2DCoordinateObjectState (line.c2));
            break;
        }
        case API_PrimArcID: {
            const API_PrimArc& arc = primElem->arc;
            AddLineTypeToObjectState (os, arc.ltypeInd, ctx);
            os.Add ("solid", arc.solid);
            os.Add ("whole", arc.whole);
            os.Add ("determination", arc.determination);
            os.Add ("reflected", arc.reflected);
            os.Add ("penWeight", arc.penWeight);
            os.Add ("orig", Create2DCoordinateObjectState (arc.orig));
            os.Add ("r", arc.r);
            os.Add ("begAng", arc.begAng);
            os.Add ("endAng", arc.endAng);
            os.Add ("angle", arc.angle);
            os.Add ("ratio", arc.ratio);
            break;
        }
        case API_PrimTextID: {
            const API_PrimText& text = primElem->text;
            os.Add ("font", text.font);
            os.Add ("faceBits", static_cast<Int32> (text.faceBits));
            os.Add ("effectBits", text.effectBits);
            os.Add ("anchor", AnchorIdToString (text.anchor));
            os.Add ("just", JustificationToString (text.just));
            os.Add ("loc", Create2DCoordinateObjectState (text.loc));
            os.Add ("heightMM", text.heightMM);
            os.Add ("widthMM", text.widthMM);
            os.Add ("angle", text.angle);
            os.Add ("widthFactor", text.widthFactor);
            os.Add ("charSpaceFactor", text.charSpaceFactor);
            // The text content is not read: nothing in this add-on confirms what par1 carries
            // for a text primitive, and a wrong guess would crash Archicad.
            break;
        }
        case API_PrimPLineID: {
            const API_PrimPLine& pline = primElem->pline;
            AddLineTypeToObjectState (os, pline.ltypeInd, ctx);
            os.Add ("determination", pline.determination);
            os.Add ("drawSegmentMode", pline.drawSegmentMode);
            os.Add ("nCoords", pline.nCoords);
            os.Add ("nArcs", pline.nArcs);
            os.Add ("penWeight", pline.penWeight);
            // The vertices are not read: nothing in this add-on confirms how par1 is indexed for
            // a polyline primitive, and a wrong guess would read past the array.
            break;
        }
        case API_PrimTriID: {
            const API_PrimTri& tri = primElem->tri;
            AddLineTypeToObjectState (os, tri.ltypeInd, ctx);
            os.Add ("solid", tri.solid);
            os.Add ("determination", tri.determination);
            os.Add ("penWeight", tri.penWeight);
            const auto& coordinates = os.AddList<GS::ObjectState> ("coordinates");
            for (const API_Coord& c : tri.c) {
                coordinates (Create2DCoordinateObjectState (c));
            }
            break;
        }
        case API_PrimPolyID: {
            const API_PrimPoly& poly = primElem->poly;
            AddLineTypeToObjectState (os, poly.ltypeInd, ctx);
            os.Add ("solid", poly.solid);
            os.Add ("useRgb", poly.useRgb);
            os.Add ("determination", poly.determination);
            os.Add ("penWeight", poly.penWeight);
            os.Add ("nCoords", poly.nCoords);
            os.Add ("nSubPolys", poly.nSubPolys);
            os.Add ("nArcs", poly.nArcs);
            os.Add ("fillPen", poly.fillPen.penIndex);
            os.Add ("rgbColor", GS::ObjectState ("red", poly.rgbColor.f_red, "green", poly.rgbColor.f_green, "blue", poly.rgbColor.f_blue));
            const auto* coords = static_cast<const API_Coord*> (par1);
            const auto* pends  = static_cast<const Int32*> (par2);
            if (coords != nullptr && poly.nCoords > 0) {
                // The first sub polygon is the outline, the others are holes. pends is 1 based and
                // its last entry is nCoords; clamped as in CollectCutFillPolygons.
                const auto& polygons = os.AddList<GS::ObjectState> ("polygons");
                Int32 begIndex = 1;
                const Int32 nSubPolys = (pends != nullptr) ? poly.nSubPolys : 1;
                for (Int32 sub = 1; sub <= nSubPolys && begIndex <= poly.nCoords; ++sub) {
                    Int32 endIndex = (pends != nullptr) ? pends[sub] : poly.nCoords;
                    if (endIndex > poly.nCoords)
                        endIndex = poly.nCoords;
                    GS::ObjectState polygon;
                    const auto& coordinates = polygon.AddList<GS::ObjectState> ("coordinates");
                    AddCoordinatesToList (coordinates, coords, begIndex, endIndex);
                    polygons (polygon);
                    begIndex = endIndex + 1;
                }
            }
            break;
        }
        case API_PrimPictID: {
            const API_PrimPict& pict = primElem->pict;
            os.Add ("rotAngle", pict.rotAngle);
            os.Add ("destBox", GS::ObjectState (
                "xMin", pict.destBox.xMin,
                "yMin", pict.destBox.yMin,
                "xMax", pict.destBox.xMax,
                "yMax", pict.destBox.yMax));
            break;
        }
        case API_PrimCtrl_HatchBorderBegID: {
            // par1 read as CollectCutFillPolygons in ElementCommands.cpp reads it.
            const auto* border = static_cast<const API_PrimHatchBorder*> (par1);
            if (border != nullptr) {
                GS::ObjectState hatch;
                hatch.Add ("doFrame", border->doFrame);
                const API_Guid fillGuid = ctx.GetAttributeGuid (API_FilltypeID, border->fillInd);
                if (fillGuid != APINULLGuid) {
                    hatch.Add ("fill", CreateAttributeIdObjectState (fillGuid));
                }
                hatch.Add ("fillPen", border->fillPen.penIndex);
                hatch.Add ("fillbkgPen", border->fillbkgPen);
                hatch.Add ("determination", border->determination);
                os.Add ("hatchBorder", hatch);
            }
            break;
        }
        default:
            break;
    }

    ctx.primitives.Push (os);
    return NoError;
}

static GSErrCode GetDrawingPrimitives (const API_Elem_Head& elemHead, DrawingPrimitiveCollectorContext& ctx)
{
    tl_drawingPrimitiveCtx = &ctx;
    const GS::OnExit ctxGuard ([&] () { tl_drawingPrimitiveCtx = nullptr; });

    // The same parameters as AddFloorPlanPolygonsIfAvailable in ElementCommands.cpp: draw the
    // whole element, whichever story is shown and whatever the window shows of it.
    API_ShapePrimsParams params = {};
    params.dontClip   = true;
    params.allStories = true;
    params.polygon    = nullptr;

#if defined(ServerMainVers_2700)
    return ACAPI_DrawingPrimitive_ShapePrimsExt (elemHead, CollectDrawingPrimitives, &params);
#else
    return ACAPI_Element_ShapePrimsExt (elemHead, CollectDrawingPrimitives, &params);
#endif
}

// ── GetDrawingPrimitivesOfElements ────────────────────────────────────────────

GetDrawingPrimitivesOfElementsCommand::GetDrawingPrimitivesOfElementsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetDrawingPrimitivesOfElementsCommand::GetName () const
{
    return "GetDrawingPrimitivesOfElements";
}

GS::Optional<GS::UniString> GetDrawingPrimitivesOfElementsCommand::GetInputParametersSchema () const
{
    return R"({
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
    })";
}

GS::Optional<GS::UniString> GetDrawingPrimitivesOfElementsCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "drawingPrimitivesOfElements": {
                "type": "array",
                "description": "The drawing primitives of each element, in the order of the input elements.",
                "items": {
                    "oneOf": [
                        {
                            "type": "object",
                            "properties": {
                                "primitives": {
                                    "type": "array",
                                    "description": "The primitives in the order Archicad draws them. The Ctrl_* items mark the beginning and the end of sub-parts (hatch borders, hatch lines, texts, curtain wall panels, ...).",
                                    "items": {
                                        "type": "object",
                                        "properties": {
                                            "type": {
                                                "type": "string",
                                                "description": "The API_PrimTypeID name without its API_Prim prefix and ID suffix.",
                                                "enum": [
                                                    "Point", "Line", "Arc", "Text", "PLine", "Tri", "Poly", "Pict",
                                                    "Ctrl_Beg", "Ctrl_HatchBorderBeg", "Ctrl_HatchBorderEnd", "Ctrl_HatchLinesBeg", "Ctrl_HatchLinesEnd",
                                                    "Ctrl_HoledimLinesBeg", "Ctrl_HoledimLinesEnd", "Ctrl_ArrowBeg", "Ctrl_ArrowEnd", "Ctrl_ElementRef", "Ctrl_End",
                                                    "Ctrl_PlacedBorderBeg", "Ctrl_PlacedBorderEnd", "Ctrl_TextBeg", "Ctrl_TextEnd", "Ctrl_CWallPanelBeg", "Ctrl_CWallPanelEnd",
                                                    "Ctrl_BackgroundFillBeg", "Ctrl_BackgroundFillEnd", "Ctrl_FrameBeg", "Ctrl_FrameEnd", "Unknown"
                                                ]
                                            },
                                            "layer": {
                                                "$ref": "#/AttributeIdArrayItem",
                                                "description": "The layer of the element."
                                            },
                                            "pen": {
                                                "type": "integer",
                                                "description": "The pen index of the primitive."
                                            },
                                            "drwIndex": {
                                                "type": "integer"
                                            },
                                            "renovationStatus": {
                                                "type": "string",
                                                "enum": ["Undefined", "Existing", "New", "Demolished"]
                                            },
                                            "lineType": {
                                                "$ref": "#/AttributeIdArrayItem",
                                                "description": "Line, Arc, PLine, Tri and Poly: the line type."
                                            },
                                            "determination": {
                                                "type": "integer",
                                                "description": "How the type of the line or fill is determined (origin, inner line, contour line)."
                                            },
                                            "penWeight": {
                                                "type": "number"
                                            },
                                            "solid": {
                                                "type": "boolean"
                                            },
                                            "loc": {
                                                "$ref": "#/Coordinate2D",
                                                "description": "Point: the location. Text: the left baseline position."
                                            },
                                            "c1": {
                                                "$ref": "#/Coordinate2D"
                                            },
                                            "c2": {
                                                "$ref": "#/Coordinate2D"
                                            },
                                            "whole": {
                                                "type": "boolean"
                                            },
                                            "reflected": {
                                                "type": "boolean",
                                                "description": "The arc goes clockwise."
                                            },
                                            "orig": {
                                                "$ref": "#/Coordinate2D"
                                            },
                                            "r": {
                                                "type": "number"
                                            },
                                            "begAng": {
                                                "type": "number"
                                            },
                                            "endAng": {
                                                "type": "number"
                                            },
                                            "angle": {
                                                "type": "number"
                                            },
                                            "ratio": {
                                                "type": "number"
                                            },
                                            "font": {
                                                "type": "integer"
                                            },
                                            "faceBits": {
                                                "type": "integer"
                                            },
                                            "effectBits": {
                                                "type": "integer"
                                            },
                                            "anchor": {
                                                "type": "string"
                                            },
                                            "just": {
                                                "type": "string"
                                            },
                                            "heightMM": {
                                                "type": "number"
                                            },
                                            "widthMM": {
                                                "type": "number"
                                            },
                                            "widthFactor": {
                                                "type": "number"
                                            },
                                            "charSpaceFactor": {
                                                "type": "number"
                                            },
                                            "drawSegmentMode": {
                                                "type": "integer"
                                            },
                                            "nCoords": {
                                                "type": "integer"
                                            },
                                            "nSubPolys": {
                                                "type": "integer"
                                            },
                                            "nArcs": {
                                                "type": "integer",
                                                "description": "The number of curved segments. Their geometry is not returned."
                                            },
                                            "useRgb": {
                                                "type": "boolean"
                                            },
                                            "fillPen": {
                                                "type": "integer"
                                            },
                                            "rgbColor": {
                                                "type": "object",
                                                "properties": {
                                                    "red": { "type": "number" },
                                                    "green": { "type": "number" },
                                                    "blue": { "type": "number" }
                                                },
                                                "additionalProperties": false,
                                                "required": ["red", "green", "blue"]
                                            },
                                            "coordinates": {
                                                "type": "array",
                                                "description": "Tri: the three corners.",
                                                "items": {
                                                    "$ref": "#/Coordinate2D"
                                                }
                                            },
                                            "polygons": {
                                                "type": "array",
                                                "description": "Poly: the sub polygons, the first one is the outline, the others are holes.",
                                                "items": {
                                                    "type": "object",
                                                    "properties": {
                                                        "coordinates": {
                                                            "type": "array",
                                                            "items": {
                                                                "$ref": "#/Coordinate2D"
                                                            }
                                                        }
                                                    },
                                                    "additionalProperties": false,
                                                    "required": ["coordinates"]
                                                }
                                            },
                                            "rotAngle": {
                                                "type": "number"
                                            },
                                            "destBox": {
                                                "type": "object",
                                                "description": "Pict: the bounding box of the picture.",
                                                "properties": {
                                                    "xMin": { "type": "number" },
                                                    "yMin": { "type": "number" },
                                                    "xMax": { "type": "number" },
                                                    "yMax": { "type": "number" }
                                                },
                                                "additionalProperties": false,
                                                "required": ["xMin", "yMin", "xMax", "yMax"]
                                            },
                                            "hatchBorder": {
                                                "type": "object",
                                                "description": "Ctrl_HatchBorderBeg: the fill that follows.",
                                                "properties": {
                                                    "doFrame": { "type": "boolean" },
                                                    "fill": { "$ref": "#/AttributeIdArrayItem" },
                                                    "fillPen": { "type": "integer" },
                                                    "fillbkgPen": { "type": "integer" },
                                                    "determination": { "type": "integer" }
                                                },
                                                "additionalProperties": false,
                                                "required": ["doFrame", "fillPen", "fillbkgPen", "determination"]
                                            }
                                        },
                                        "additionalProperties": false,
                                        "required": [
                                            "type",
                                            "pen",
                                            "drwIndex",
                                            "renovationStatus"
                                        ]
                                    }
                                }
                            },
                            "additionalProperties": false,
                            "required": [
                                "primitives"
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
            "drawingPrimitivesOfElements"
        ]
    })";
}

GS::ObjectState GetDrawingPrimitivesOfElementsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    API_DatabaseInfo currentDatabase = {};
    GSErrCode err = ACAPI_Database_GetCurrentDatabase (&currentDatabase);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the current database.");
    }
    if (currentDatabase.typeID != APIWind_FloorPlanID) {
        return CreateErrorResponse (APIERR_BADDATABASE, "The floor plan must be the current database. Switch to the floor plan window first, for example with ChangeWindow.");
    }

    GS::Array<GS::ObjectState> elements;
    parameters.Get ("elements", elements);

    GS::ObjectState response;
    const auto& drawingPrimitivesOfElements = response.AddList<GS::ObjectState> ("drawingPrimitivesOfElements");

    for (const GS::ObjectState& element : elements) {
        const GS::ObjectState* elementId = element.Get ("elementId");
        if (elementId == nullptr) {
            drawingPrimitivesOfElements (CreateErrorResponse (APIERR_BADPARS, "elementId is missing"));
            continue;
        }

        API_Elem_Head elemHead = {};
        elemHead.guid = GetGuidFromObjectState (*elementId);
        err = ACAPI_Element_GetHeader (&elemHead);
        if (err != NoError) {
            drawingPrimitivesOfElements (CreateErrorResponse (err, "Failed to find element in Archicad"));
            continue;
        }

        DrawingPrimitiveCollectorContext ctx;
        err = GetDrawingPrimitives (elemHead, ctx);
        if (err != NoError) {
            drawingPrimitivesOfElements (CreateErrorResponse (err, "Failed to get the drawing primitives of the element"));
            continue;
        }

        GS::ObjectState result;
        const auto& primitives = result.AddList<GS::ObjectState> ("primitives");
        for (const GS::ObjectState& primitive : ctx.primitives) {
            primitives (primitive);
        }
        drawingPrimitivesOfElements (result);
    }

    return response;
}
