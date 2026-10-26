#include "LibraryPartSectionCommands.hpp"
#include "ObjectState.hpp"
#include "MigrationHelper.hpp"

// The section functions are ACAPI_LibraryPart_* from Archicad 27 (ACAPinc.h) and ACAPI_LibPart_*
// with the same parameters before, the split ElementCommands.cpp makes for TAPIR_Polygon_TriangulatePoly.
static GSErrCode TAPIR_LibraryPart_GetSection (Int32 libInd, API_LibPartSection* section, GSHandle* sectionHdl, GS::UniString* sectionStr)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_GetSection (libInd, section, sectionHdl, sectionStr);
#else
    return ACAPI_LibPart_GetSection (libInd, section, sectionHdl, sectionStr);
#endif
}

static GSErrCode TAPIR_LibraryPart_UpdateSection (Int32 libInd, const API_LibPartSection* section, GSHandle sectionHdl, GS::UniString* sectionStr)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_UpdateSection (libInd, section, sectionHdl, sectionStr);
#else
    return ACAPI_LibPart_UpdateSection (libInd, section, sectionHdl, sectionStr);
#endif
}

static GSErrCode TAPIR_LibraryPart_ShapePrims (Int32 libInd, const API_Guid& instanceElemGuid, short gdlContext, ShapePrimsProc* shapePrimsProc)
{
#ifdef ServerMainVers_2700
    return ACAPI_LibraryPart_ShapePrims (libInd, instanceElemGuid, gdlContext, shapePrimsProc);
#else
    return ACAPI_LibPart_ShapePrims (libInd, instanceElemGuid, gdlContext, shapePrimsProc);
#endif
}

// Finds a loaded library part by its name, as ResolveLibraryPartName in ElementCreationCommands.cpp
// does, but placeable or not. The name is the one without the file extension; a trailing ".gsm"
// is dropped when the full name finds nothing.
static GSErrCode FindLibraryPartIndexByName (const GS::UniString& name, Int32& libInd)
{
    GS::Array<GS::UniString> candidates;
    candidates.Push (name);
    const USize length = name.GetLength ();
    if (length > 4 && name.GetSubstring (length - 4, 4).Compare (".gsm", CaseInsensitive) == GS::UniString::Equal) {
        candidates.Push (name.GetSubstring (0, length - 4));
    }

    GSErrCode err = APIERR_BADNAME;
    for (const GS::UniString& candidate : candidates) {
        API_LibPart libPart = {};
        GS::ucscpy (libPart.docu_UName, candidate.ToUStr ());
        err = ACAPI_LibraryPart_Search (&libPart, false, false);
        delete libPart.location;
        if (err == NoError) {
            libInd = libPart.index;
            return NoError;
        }
    }
    return err;
}

// UpdateLibraryPartSections

struct LibraryPartTextSectionType
{
    const char* name;
    GSType      sectType;
};

// The text sections of a library part, by the section type constants of APIdefs_LibraryParts.h.
static const LibraryPartTextSectionType libraryPartTextSectionTypes[] = {
    { "MasterScript",    API_Sect1DScript },
    { "2DScript",        API_Sect2DScript },
    { "3DScript",        API_Sect3DScript },
    { "ParameterScript", API_SectVLScript },
    { "UIScript",        API_SectUIScript },
    { "Comment",         API_SectComText },
    { "Keywords",        API_SectKeywords },
    { "Copyright",       API_SectCopyright }
};

static bool GetLibraryPartTextSectionType (const GS::UniString& name, GSType& sectType)
{
    for (const LibraryPartTextSectionType& sectionType : libraryPartTextSectionTypes) {
        if (name == sectionType.name) {
            sectType = sectionType.sectType;
            return true;
        }
    }
    return false;
}

UpdateLibraryPartSectionsCommand::UpdateLibraryPartSectionsCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String UpdateLibraryPartSectionsCommand::GetName () const
{
    return "UpdateLibraryPartSections";
}

GS::Optional<GS::UniString> UpdateLibraryPartSectionsCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part, without the file extension (a trailing '.gsm' is accepted too)."
            },
            "sections": {
                "type": "array",
                "description": "The sections to replace. Each section must already exist in the library part; this command does not add new sections.",
                "items": {
                    "type": "object",
                    "properties": {
                        "type": {
                            "type": "string",
                            "enum": [
                                "MasterScript",
                                "2DScript",
                                "3DScript",
                                "ParameterScript",
                                "UIScript",
                                "Comment",
                                "Keywords",
                                "Copyright"
                            ],
                            "description": "The type of the text section."
                        },
                        "subIdent": {
                            "type": "integer",
                            "minimum": 0,
                            "maximum": 65535,
                            "description": "The secondary identifier of the section, for a library part with more sections of the same type. Default value is 0."
                        },
                        "text": {
                            "type": "string",
                            "description": "The new content of the section."
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "type",
                        "text"
                    ]
                },
                "minItems": 1
            }
        },
        "additionalProperties": false,
        "required": [
            "name",
            "sections"
        ]
    })";
}

GS::Optional<GS::UniString> UpdateLibraryPartSectionsCommand::GetRawResponseSchema () const
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

GS::ObjectState UpdateLibraryPartSectionsCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    GS::Array<GS::ObjectState> sections;
    if (!parameters.Get ("name", name) || name.IsEmpty () || !parameters.Get ("sections", sections)) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'name' or 'sections' parameter.");
    }

    Int32 libInd = 0;
    const GSErrCode searchErr = FindLibraryPartIndexByName (name, libInd);

    GS::ObjectState response;
    const auto& executionResults = response.AddList<GS::ObjectState> ("executionResults");

    ACAPI_CallUndoableCommand ("Update Library Part Sections", [&]() -> GSErrCode {
        for (const GS::ObjectState& sectionOS : sections) {
            if (searchErr != NoError) {
                executionResults (CreateFailedExecutionResult (searchErr, "No loaded library part with the name '" + name + "'."));
                continue;
            }

            GS::UniString typeName;
            GS::UniString text;
            API_LibPartSection section = {};
            if (!sectionOS.Get ("type", typeName) || !GetLibraryPartTextSectionType (typeName, section.sectType) || !sectionOS.Get ("text", text)) {
                executionResults (CreateFailedExecutionResult (APIERR_BADPARS, "Invalid or missing 'type' or 'text' parameter."));
                continue;
            }
            Int32 subIdent = 0;
            if (sectionOS.Get ("subIdent", subIdent)) {
                section.subIdent = static_cast<unsigned short> (subIdent);
            }

            // The current section is read first, so the update gets the same section header and
            // the handle Archicad returned for it along with the new text, and a section the part
            // does not have is reported instead of being written.
            GSHandle sectionHdl = nullptr;
            const GS::OnExit handleGuard ([&sectionHdl] () {
                if (sectionHdl != nullptr) {
                    BMKillHandle (&sectionHdl);
                }
            });
            GS::UniString currentText;
            GSErrCode err = TAPIR_LibraryPart_GetSection (libInd, &section, &sectionHdl, &currentText);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "The library part '" + name + "' has no '" + typeName + "' section with this subIdent. Only existing sections are updated."));
                continue;
            }

            err = TAPIR_LibraryPart_UpdateSection (libInd, &section, sectionHdl, &text);
            if (err != NoError) {
                executionResults (CreateFailedExecutionResult (err, "Failed to update the '" + typeName + "' section of the library part '" + name + "'. A part of a library package or a read-only library cannot be modified."));
                continue;
            }

            // Read back, so a section that Archicad accepted but did not change is not reported
            // as a success.
            GSHandle updatedHdl = nullptr;
            GS::UniString updatedText;
            err = TAPIR_LibraryPart_GetSection (libInd, &section, &updatedHdl, &updatedText);
            if (updatedHdl != nullptr) {
                BMKillHandle (&updatedHdl);
            }
            if (err != NoError || updatedText != text) {
                executionResults (CreateFailedExecutionResult (err != NoError ? err : APIERR_GENERAL, "The '" + typeName + "' section of the library part '" + name + "' does not have the new text after the update."));
                continue;
            }

            executionResults (CreateSuccessfulExecutionResult ());
        }
        return NoError;
    });

    return response;
}

// GetLibraryPart2DPrimitives

struct LibraryPart2DPrimitivesContext {
    GS::Array<GS::ObjectState> primitives;
    bool inHatchBorder = false;
};

static thread_local LibraryPart2DPrimitivesContext* tl_libPartPrimsCtx = nullptr;

// Collects the drawing primitives of the 2D symbol. The polygon handling is the one of
// CollectCutFillPolygons in ElementCommands.cpp: par1 is the 1-based coordinate array, par2 the
// 1-based end indices of the subpolygons, and a hatch's polygons arrive between its
// API_PrimCtrl_HatchBorderBegID and API_PrimCtrl_HatchBorderEndID primitives.
static GSErrCode CollectLibraryPart2DPrimitives (const API_PrimElement* primElem,
                                                 const void* par1, const void* par2, const void* /*par3*/)
{
    if (tl_libPartPrimsCtx == nullptr)
        return NoError;

    switch (primElem->header.typeID) {
        case API_PrimCtrl_HatchBorderBegID:
            tl_libPartPrimsCtx->inHatchBorder = true;
            break;
        case API_PrimCtrl_HatchBorderEndID:
            tl_libPartPrimsCtx->inHatchBorder = false;
            break;
        case API_PrimLineID: {
            GS::ObjectState line;
            line.Add ("type", "Line");
            line.Add ("begCoordinate", Create2DCoordinateObjectState (primElem->line.c1));
            line.Add ("endCoordinate", Create2DCoordinateObjectState (primElem->line.c2));
            tl_libPartPrimsCtx->primitives.Push (line);
            break;
        }
        case API_PrimArcID: {
            GS::ObjectState arc;
            arc.Add ("type", "Arc");
            arc.Add ("origin", Create2DCoordinateObjectState (primElem->arc.orig));
            arc.Add ("radius", primElem->arc.r);
            arc.Add ("begAngle", primElem->arc.begAng);
            arc.Add ("endAngle", primElem->arc.endAng);
            arc.Add ("isWhole", primElem->arc.whole);
            tl_libPartPrimsCtx->primitives.Push (arc);
            break;
        }
        case API_PrimPolyID: {
            const Int32 nCoords = primElem->poly.nCoords;
            const auto* coords = static_cast<const API_Coord*> (par1);
            const auto* pends  = static_cast<const Int32*> (par2);
            if (nCoords < 3 || coords == nullptr)
                break;

            GS::Array<GS::Array<API_Coord>> rings;
            Int32 start = 1;
            const Int32 nSubPolys = primElem->poly.nSubPolys;
            for (Int32 sub = 1; sub <= nSubPolys; ++sub) {
                Int32 end = (pends != nullptr) ? pends[sub] : nCoords;
                if (end > nCoords)
                    end = nCoords;
                GS::Array<API_Coord> ring;
                for (Int32 i = start; i <= end; ++i)
                    ring.Push (coords[i]);
                rings.Push (ring);
                start = end + 1;
            }
            if (rings.IsEmpty () || rings[0].GetSize () < 3)
                break;

            GS::ObjectState polygon;
            polygon.Add ("type", "Polygon");
            polygon.Add ("isFill", tl_libPartPrimsCtx->inHatchBorder);
            {
                const auto& outline = polygon.AddList<GS::ObjectState> ("coordinates");
                for (const API_Coord& c : rings[0])
                    outline (Create2DCoordinateObjectState (c));
            }
            if (rings.GetSize () > 1) {
                const auto& holes = polygon.AddList<GS::ObjectState> ("holes");
                for (UIndex h = 1; h < rings.GetSize (); ++h) {
                    if (rings[h].GetSize () < 3)
                        continue;
                    GS::ObjectState hole;
                    const auto& holeCoords = hole.AddList<GS::ObjectState> ("coordinates");
                    for (const API_Coord& c : rings[h])
                        holeCoords (Create2DCoordinateObjectState (c));
                    holes (hole);
                }
            }
            tl_libPartPrimsCtx->primitives.Push (polygon);
            break;
        }
        default:
            break;
    }
    return NoError;
}

GetLibraryPart2DPrimitivesCommand::GetLibraryPart2DPrimitivesCommand () :
    CommandBase (CommonSchema::Used)
{
}

GS::String GetLibraryPart2DPrimitivesCommand::GetName () const
{
    return "GetLibraryPart2DPrimitives";
}

GS::Optional<GS::UniString> GetLibraryPart2DPrimitivesCommand::GetInputParametersSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "name": {
                "type": "string",
                "description": "The name of the loaded library part, without the file extension (a trailing '.gsm' is accepted too)."
            },
            "elementId": {
                "$ref": "#/ElementId",
                "description": "Optional. A placed instance of the library part whose parameters are used for drawing the symbol. Without it the default parameters of the library part are used."
            },
            "gdlContext": {
                "type": "integer",
                "description": "Optional. The GDL context the 2D symbol is drawn in, the value of the GLOB_CONTEXT GDL global (2 is the floor plan). Default value is 2."
            }
        },
        "additionalProperties": false,
        "required": [
            "name"
        ]
    })";
}

GS::Optional<GS::UniString> GetLibraryPart2DPrimitivesCommand::GetRawResponseSchema () const
{
    return R"({
        "type": "object",
        "properties": {
            "primitives": {
                "type": "array",
                "description": "The lines, arcs and polygons of the 2D symbol, in the order Archicad draws them, in the coordinate system of the library part. Other primitives (texts, hotspots, ...) are not returned.",
                "items": {
                    "type": "object",
                    "properties": {
                        "type": {
                            "type": "string",
                            "enum": [
                                "Line",
                                "Arc",
                                "Polygon"
                            ]
                        },
                        "begCoordinate": {
                            "$ref": "#/Coordinate2D",
                            "description": "The start point of a Line."
                        },
                        "endCoordinate": {
                            "$ref": "#/Coordinate2D",
                            "description": "The end point of a Line."
                        },
                        "origin": {
                            "$ref": "#/Coordinate2D",
                            "description": "The center of an Arc."
                        },
                        "radius": {
                            "type": "number",
                            "description": "The radius of an Arc."
                        },
                        "begAngle": {
                            "type": "number",
                            "description": "The start angle of an Arc, in radians."
                        },
                        "endAngle": {
                            "type": "number",
                            "description": "The end angle of an Arc, in radians."
                        },
                        "isWhole": {
                            "type": "boolean",
                            "description": "Whether an Arc is a full circle."
                        },
                        "isFill": {
                            "type": "boolean",
                            "description": "Whether a Polygon is the outline of a fill."
                        },
                        "coordinates": {
                            "type": "array",
                            "description": "The outline of a Polygon.",
                            "items": {
                                "$ref": "#/Coordinate2D"
                            }
                        },
                        "holes": {
                            "type": "array",
                            "description": "The holes of a Polygon.",
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
                                "required": [
                                    "coordinates"
                                ]
                            }
                        }
                    },
                    "additionalProperties": false,
                    "required": [
                        "type"
                    ]
                }
            }
        },
        "additionalProperties": false,
        "required": [
            "primitives"
        ]
    })";
}

GS::ObjectState GetLibraryPart2DPrimitivesCommand::Execute (const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
    GS::UniString name;
    if (!parameters.Get ("name", name) || name.IsEmpty ()) {
        return CreateErrorResponse (APIERR_BADPARS, "Invalid or missing 'name' parameter.");
    }

    Int32 libInd = 0;
    GSErrCode err = FindLibraryPartIndexByName (name, libInd);
    if (err != NoError) {
        return CreateErrorResponse (err, "No loaded library part with the name '" + name + "'.");
    }

    API_Guid instanceGuid = APINULLGuid;
    const GS::ObjectState* elementId = parameters.Get ("elementId");
    if (elementId != nullptr) {
        instanceGuid = GetGuidFromObjectState (*elementId);
        API_Elem_Head elemHead = {};
        elemHead.guid = instanceGuid;
        if (ACAPI_Element_GetHeader (&elemHead) != NoError) {
            return CreateErrorResponse (APIERR_BADID, "Failed to find the element given in 'elementId'.");
        }
    }

    Int32 gdlContext = 2;
    parameters.Get ("gdlContext", gdlContext);

    LibraryPart2DPrimitivesContext ctx;
    tl_libPartPrimsCtx = &ctx;
    const GS::OnExit ctxGuard ([&] () { tl_libPartPrimsCtx = nullptr; });

    err = TAPIR_LibraryPart_ShapePrims (libInd, instanceGuid, static_cast<short> (gdlContext), CollectLibraryPart2DPrimitives);
    if (err != NoError) {
        return CreateErrorResponse (err, "Failed to get the 2D primitives of the library part '" + name + "'.");
    }

    GS::ObjectState response;
    const auto& primitives = response.AddList<GS::ObjectState> ("primitives");
    for (const GS::ObjectState& primitive : ctx.primitives) {
        primitives (primitive);
    }
    return response;
}
