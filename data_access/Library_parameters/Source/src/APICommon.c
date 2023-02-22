// *****************************************************************************
// Helper functions for Add-On development
// *****************************************************************************

// ---------------------------------- Includes ---------------------------------

#include	<stdarg.h>

#include	"GSSystem.h"

#include	"APIEnvir.h"
#include	"ACAPinc.h"
#include	"APICommon.h"


// =============================================================================
//
//	Messages
//
// =============================================================================

// -----------------------------------------------------------------------------
// Write an error into an alert dialog
// -----------------------------------------------------------------------------

void	WriteReport_Err (const char* info, GSErrCode err)
{
	ACAPI_WriteReport ("%s: %s", true, info, ErrID_To_Name (err));
}		// WriteReport_Err


// -----------------------------------------------------------------------------
// Write an end report into the report window
// -----------------------------------------------------------------------------

void	WriteReport_End (GSErrCode err)
{
	if (err == NoError)
		ACAPI_WriteReport ("OK", false);
	else
		ACAPI_WriteReport ("Error: %s", false, ErrID_To_Name (err));
}		// WriteReport_End


// -----------------------------------------------------------------------------
// Write an error into an alert dialog and give beep
// -----------------------------------------------------------------------------

void	ErrorBeep (const char* info, GSErrCode err)
{
	WriteReport_Err (info, err);
	GSSysBeep ();
}		// ErrorBeep


// =============================================================================
//
//	Conversions
//
// =============================================================================

// -----------------------------------------------------------------------------
// Convert the NeigID to element type
// -----------------------------------------------------------------------------

API_ElemType	Neig_To_ElemID (API_NeigID neigID)
{
	API_ElemType	type;
	GSErrCode		err;

	err = ACAPI_Goodies_NeigIDToElemType (neigID, type);
	if (err != NoError)
		type = API_ZombieElemID;

	return type;
}		// Neig_To_ElemID


// -----------------------------------------------------------------------------
// Convert the element header to a neig
// -----------------------------------------------------------------------------

bool	ElemHead_To_Neig (API_Neig				*neig,
						  const API_Elem_Head	*elemHead)
{
	*neig = {};
	neig->guid = elemHead->guid;

	API_ElemType type = elemHead->type;
	if (type == API_ZombieElemID && neig->guid != APINULLGuid) {
		API_Elem_Head elemHeadCopy = {};
		elemHeadCopy.guid = elemHead->guid;
		ACAPI_Element_GetHeader (&elemHeadCopy);
		type = elemHeadCopy.type;
	}

	switch (type.typeID) {
		case API_WallID:					neig->neigID = APINeig_Wall;				neig->inIndex = 1;	break;
		case API_ColumnID:					neig->neigID = APINeig_Colu;				neig->inIndex = 0;	break;
		case API_BeamID:					neig->neigID = APINeig_Beam;				neig->inIndex = 1;	break;
		case API_WindowID:					neig->neigID = APINeig_WindHole;			neig->inIndex = 0;	break;
		case API_DoorID:					neig->neigID = APINeig_DoorHole;			neig->inIndex = 0;	break;
		case API_ObjectID:					neig->neigID = APINeig_Symb;				neig->inIndex = 1;	break;
		case API_LampID:					neig->neigID = APINeig_Light;				neig->inIndex = 1;	break;
		case API_SlabID:					neig->neigID = APINeig_Ceil;				neig->inIndex = 1;	break;
		case API_RoofID:					neig->neigID = APINeig_Roof;				neig->inIndex = 1;	break;
		case API_MeshID:					neig->neigID = APINeig_Mesh;				neig->inIndex = 1;	break;

		case API_DimensionID:				neig->neigID = APINeig_DimOn;				neig->inIndex = 1;	break;
		case API_RadialDimensionID:			neig->neigID = APINeig_RadDim;				neig->inIndex = 1;	break;
		case API_LevelDimensionID:			neig->neigID = APINeig_LevDim;				neig->inIndex = 1;	break;
		case API_AngleDimensionID:			neig->neigID = APINeig_AngDimOn;			neig->inIndex = 1;	break;

		case API_TextID:					neig->neigID = APINeig_Word;				neig->inIndex = 1;	break;
		case API_LabelID:					neig->neigID = APINeig_Label;				neig->inIndex = 1;	break;
		case API_ZoneID:					neig->neigID = APINeig_Room;				neig->inIndex = 1;	break;

		case API_HatchID:					neig->neigID = APINeig_Hatch;				neig->inIndex = 1;	break;
		case API_LineID:					neig->neigID = APINeig_Line;				neig->inIndex = 1;	break;
		case API_PolyLineID:				neig->neigID = APINeig_PolyLine;			neig->inIndex = 1;	break;
		case API_ArcID:						neig->neigID = APINeig_Arc;					neig->inIndex = 1;	break;
		case API_CircleID:					neig->neigID = APINeig_Circ;				neig->inIndex = 1;	break;
		case API_SplineID:					neig->neigID = APINeig_Spline;				neig->inIndex = 1;	break;
		case API_HotspotID:					neig->neigID = APINeig_Hot;					neig->inIndex = 1;	break;

		case API_CutPlaneID:				neig->neigID = APINeig_CutPlane;			neig->inIndex = 1;	break;
		case API_ElevationID:				neig->neigID = APINeig_Elevation;			neig->inIndex = 1;	break;
		case API_InteriorElevationID:		neig->neigID = APINeig_InteriorElevation;	neig->inIndex = 1;	break;
		case API_CameraID:					neig->neigID = APINeig_Camera;				neig->inIndex = 1;	break;
		case API_CamSetID:					return false;

		case API_PictureID:					neig->neigID = APINeig_PictObj;				neig->inIndex = 1;	break;
		case API_DetailID:					neig->neigID = APINeig_Detail;				neig->inIndex = 1;	break;
		case API_WorksheetID:				neig->neigID = APINeig_Worksheet;			neig->inIndex = 1;	break;

		case API_SectElemID:				neig->neigID = APINeig_VirtSy;				neig->inIndex = 1;	break;
		case API_DrawingID:					neig->neigID = APINeig_DrawingCenter;		neig->inIndex = 1;	break;

		case API_CurtainWallID:				neig->neigID = APINeig_CurtainWall;			neig->inIndex = 1;	break;
		case API_CurtainWallSegmentID:		neig->neigID = APINeig_CWSegment;			neig->inIndex = 1;	break;
		case API_CurtainWallFrameID:		neig->neigID = APINeig_CWFrame;				neig->inIndex = 1;	break;
		case API_CurtainWallPanelID:		neig->neigID = APINeig_CWPanel;				neig->inIndex = 1;	break;
		case API_CurtainWallJunctionID:		neig->neigID = APINeig_CWJunction;			neig->inIndex = 1;	break;
		case API_CurtainWallAccessoryID:	neig->neigID = APINeig_CWAccessory;			neig->inIndex = 1;	break;

		case API_ShellID:					neig->neigID = APINeig_Shell;				neig->inIndex = 1;	break;
		case API_SkylightID:				neig->neigID = APINeig_SkylightHole;		neig->inIndex = 0;	break;
		case API_MorphID:					neig->neigID = APINeig_Morph;				neig->inIndex = 1;	break;
		case API_ChangeMarkerID:			neig->neigID = APINeig_ChangeMarker;		neig->inIndex = 1;	break;

		case API_StairID:						neig->neigID = APINeig_Stair;			neig->inIndex = 1;	break;
		case API_RiserID:						neig->neigID = APINeig_Riser;			neig->inIndex = 1;	break;
		case API_TreadID:						neig->neigID = APINeig_Tread;			neig->inIndex = 1;	break;
		case API_StairStructureID:				neig->neigID = APINeig_StairStructure;	neig->inIndex = 1;	break;

		case API_RailingID:						neig->neigID = APINeig_Railing;						neig->inIndex = 1;	break;
		case API_RailingToprailID:				neig->neigID = APINeig_RailingToprail;				neig->inIndex = 1;	break;
		case API_RailingHandrailID:				neig->neigID = APINeig_RailingHandrail;				neig->inIndex = 1;	break;
		case API_RailingRailID:					neig->neigID = APINeig_RailingRail;					neig->inIndex = 1;	break;
		case API_RailingPostID:					neig->neigID = APINeig_RailingPost;					neig->inIndex = 1;	break;
		case API_RailingInnerPostID:			neig->neigID = APINeig_RailingInnerPost;			neig->inIndex = 1;	break;
		case API_RailingBalusterID:				neig->neigID = APINeig_RailingBaluster;				neig->inIndex = 1;	break;
		case API_RailingPanelID:				neig->neigID = APINeig_RailingPanel;				neig->inIndex = 1;	break;
		case API_RailingSegmentID:				return false;
		case API_RailingNodeID:					return false;
		case API_RailingBalusterSetID:			return false;
		case API_RailingPatternID:				return false;
		case API_RailingToprailEndID:			neig->neigID = APINeig_RailingToprailEnd;			neig->inIndex = 1;	break;
		case API_RailingHandrailEndID:			neig->neigID = APINeig_RailingHandrailEnd;			neig->inIndex = 1;	break;
		case API_RailingRailEndID:				neig->neigID = APINeig_RailingRailEnd;				neig->inIndex = 1;	break;
		case API_RailingToprailConnectionID:	neig->neigID = APINeig_RailingToprailConnection;	neig->inIndex = 1;	break;
		case API_RailingHandrailConnectionID:	neig->neigID = APINeig_RailingHandrailConnection;	neig->inIndex = 1;	break;
		case API_RailingRailConnectionID:		neig->neigID = APINeig_RailingRailConnection;		neig->inIndex = 1;	break;
		case API_RailingEndFinishID:			neig->neigID = APINeig_RailingEndFinish;			neig->inIndex = 1;	break;

		case API_BeamSegmentID:					neig->neigID = APINeig_BeamSegment;					neig->inIndex = 1;	break;
		case API_ColumnSegmentID:				neig->neigID = APINeig_ColumnSegment;				neig->inIndex = 1;	break;
		case API_OpeningID:						return false;

		case API_GroupID:
		case API_HotlinkID:
		default:
				return false;
	}

	static_assert (API_LastElemType == API_OpeningID, "Do not forget to update ElemHead_To_Neig function after new element type was introduced!");

	return true;
}		// ElemHead_To_Neig


// -----------------------------------------------------------------------------
// Return a descriptive name for an error code
// -----------------------------------------------------------------------------

const char*		ErrID_To_Name (GSErrCode err)
{
	const char	*str;

	switch (err) {
		case APIERR_GENERAL:			str = "APIERR_GENERAL";			break;
		case APIERR_MEMFULL:			str = "APIERR_MEMFULL";			break;
		case APIERR_CANCEL:				str = "APIERR_CANCEL";			break;

		case APIERR_BADID:				str = "APIERR_BADID";			break;
		case APIERR_BADINDEX:			str = "APIERR_BADINDEX";		break;
		case APIERR_BADNAME:			str = "APIERR_BADNAME";			break;
		case APIERR_BADPARS:			str = "APIERR_BADPARS";			break;
		case APIERR_BADPOLY:			str = "APIERR_BADPOLY";			break;
		case APIERR_BADDATABASE:		str = "APIERR_BADDATABASE";		break;
		case APIERR_BADWINDOW:			str = "APIERR_BADWINDOW";		break;
		case APIERR_BADKEYCODE:			str = "APIERR_BADKEYCODE";		break;
		case APIERR_BADPLATFORMSIGN:	str = "APIERR_BADPLATFORMSIGN";	break;
		case APIERR_BADPLANE:			str = "APIERR_BADPLANE";		break;
		case APIERR_BADUSERID:			str = "APIERR_BADUSERID";		break;
		case APIERR_BADVALUE:			str = "APIERR_BADVALUE";		break;
		case APIERR_BADELEMENTTYPE:		str = "APIERR_BADELEMENTTYPE";	break;
		case APIERR_IRREGULARPOLY:		str = "APIERR_IRREGULARPOLY";	break;

		case APIERR_NO3D:				str = "APIERR_NO3D";			break;
		case APIERR_NOMORE:				str = "APIERR_NOMORE";			break;
		case APIERR_NOPLAN:				str = "APIERR_NOPLAN";			break;
		case APIERR_NOLIB:				str = "APIERR_NOLIB";			break;
		case APIERR_NOLIBSECT:			str = "APIERR_NOLIBSECT";		break;
		case APIERR_NOSEL:				str = "APIERR_NOSEL";			break;
		case APIERR_NOTEDITABLE:		str = "APIERR_NOTEDITABLE";		break;
		case APIERR_NOTSUBTYPEOF:		str = "APIERR_NOTSUBTYPEOF";	break;
		case APIERR_NOTEQUALMAIN:		str = "APIERR_NOTEQUALMAIN";	break;
		case APIERR_NOTEQUALREVISION:	str = "APIERR_NOTEQUALREVISION";	break;
		case APIERR_NOTEAMWORKPROJECT:	str = "APIERR_NOTEAMWORKPROJECT";	break;

		case APIERR_NOUSERDATA:			str = "APIERR_NOUSERDATA";		break;
		case APIERR_MOREUSER:			str = "APIERR_MOREUSER";		break;
		case APIERR_LINKEXIST:			str = "APIERR_LINKEXIST";		break;
		case APIERR_LINKNOTEXIST:		str = "APIERR_LINKNOTEXIST";	break;
		case APIERR_WINDEXIST:			str = "APIERR_WINDEXIST";		break;
		case APIERR_WINDNOTEXIST:		str = "APIERR_WINDNOTEXIST";	break;
		case APIERR_UNDOEMPTY:			str = "APIERR_UNDOEMPTY";		break;
		case APIERR_REFERENCEEXIST:		str = "APIERR_REFERENCEEXIST";	break;
		case APIERR_NAMEALREADYUSED:	str = "APIERR_NAMEALREADYUSED";	break;

		case APIERR_ATTREXIST:			str = "APIERR_ATTREXIST";			break;
		case APIERR_DELETED:			str = "APIERR_DELETED";				break;
		case APIERR_LOCKEDLAY:			str = "APIERR_LOCKEDLAY";			break;
		case APIERR_HIDDENLAY:			str = "APIERR_HIDDENLAY";			break;
		case APIERR_INVALFLOOR:			str = "APIERR_INVALFLOOR";			break;
		case APIERR_NOTMINE:			str = "APIERR_NOTMINE";				break;
		case APIERR_NOACCESSRIGHT:		str = "APIERR_NOACCESSRIGHT";		break;
		case APIERR_BADPROPERTY:		str = "APIERR_BADPROPERTY";			break;
		case APIERR_BADCLASSIFICATION:	str = "APIERR_BADCLASSIFICATION";	break;

		case APIERR_MODULNOTINSTALLED:			str = "APIERR_MODULNOTINSTALLED";			break;
		case APIERR_MODULCMDMINE:				str = "APIERR_MODULCMDMINE";				break;
		case APIERR_MODULCMDNOTSUPPORTED:		str = "APIERR_MODULCMDNOTSUPPORTED";		break;
		case APIERR_MODULCMDVERSNOTSUPPORTED:	str = "APIERR_MODULCMDVERSNOTSUPPORTED";	break;
		case APIERR_NOMODULEDATA:				str = "APIERR_NOMODULEDATA";				break;

		case APIERR_PAROVERLAP:			str = "APIERR_PAROVERLAP";		break;
		case APIERR_PARMISSING:			str = "APIERR_PARMISSING";		break;
		case APIERR_PAROVERFLOW:		str = "APIERR_PAROVERFLOW";		break;
		case APIERR_PARIMPLICIT:		str = "APIERR_PARIMPLICIT";		break;

		case APIERR_RUNOVERLAP:			str = "APIERR_RUNOVERLAP";		break;
		case APIERR_RUNMISSING:			str = "APIERR_RUNMISSING";		break;
		case APIERR_RUNOVERFLOW:		str = "APIERR_RUNOVERFLOW";		break;
		case APIERR_RUNIMPLICIT:		str = "APIERR_RUNIMPLICIT";		break;
		case APIERR_RUNPROTECTED:		str = "APIERR_RUNPROTECTED";	break;

		case APIERR_EOLOVERLAP:			str = "APIERR_EOLOVERLAP";		break;

		case APIERR_TABOVERLAP:			str = "APIERR_TABOVERLAP";		break;

		case APIERR_NOTINIT:			str = "APIERR_NOTINIT";			break;
		case APIERR_NESTING:			str = "APIERR_NESTING";			break;
		case APIERR_NOTSUPPORTED:		str = "APIERR_NOTSUPPORTED";	break;
		case APIERR_REFUSEDCMD:			str = "APIERR_REFUSEDCMD";		break;
		case APIERR_REFUSEDPAR:			str = "APIERR_REFUSEDPAR";		break;
		case APIERR_READONLY:			str = "APIERR_READONLY";		break;
		case APIERR_SERVICEFAILED:		str = "APIERR_SERVICEFAILED";	break;
		case APIERR_COMMANDFAILED:		str = "APIERR_COMMANDFAILED";	break;
		case APIERR_NEEDSUNDOSCOPE:		str = "APIERR_NEEDSUNDOSCOPE";	break;

		case APIERR_MISSINGCODE:		str = "APIERR_MISSINGCODE";		break;
		case APIERR_MISSINGDEF:			str = "APIERR_MISSINGDEF";		break;
		default:						str = "Unknown Error";			break;
	}

	return str;
}		// ErrID_To_Name


// -----------------------------------------------------------------------------
// Return a descriptive name for a library part type
// -----------------------------------------------------------------------------

const char*		LibID_To_Name (API_LibTypeID typeID)
{
	static char libNames[][32] = {
		"Zombie",

		"Spec",

		"Window",
		"Door",
		"Object",
		"Lamp",
		"Room",

		"Property",

		"PlanSign",
		"Label",

		"Macro",
		"Pict",
		"List Scheme",
		"Skylight",
		"Opening"
	};

	if (DBERROR (typeID < API_ZombieLibID || typeID > APILib_OpeningSymbolID))
		return "Unknown library part type";

	return libNames[typeID];
}		// LibID_To_Name


// -----------------------------------------------------------------------------
// Return a descriptive name for an attribute type
// -----------------------------------------------------------------------------

const char*		AttrID_To_Name (API_AttrTypeID typeID)
{
	static char attrNames[][32] = {
		"Zombie",

		"Pen",
		"Layer",
		"Linetype",
		"Fill type",
		"Composite Wall",
		"Surface",
		"Layer Combination",
		"Zone Category",
		"Font",
		"Profile",
		"Pen table",
		"Dimension style",
		"Model View options",
		"MEP System",
		"Operation Profile",
		"Building Material",
	};

	static_assert (API_LastAttributeID == API_BuildingMaterialID, "Do not forget to update AttrID_To_Name function after new attribute type was introduced!");

	if (typeID < API_ZombieAttrID || typeID > API_LastAttributeID)
		return "Unknown attribute type";

	return attrNames[typeID];
}		// AttrID_To_Name


// -----------------------------------------------------------------------------
// Return a descriptive name for an element type
// -----------------------------------------------------------------------------

const GS::UniString		ElemID_To_Name (const API_ElemType& type)
{
	GS::UniString	elemNameStr;

	ACAPI_Goodies_GetElemTypeName (type, elemNameStr);

	return elemNameStr;
}		// ElemID_To_Name



// =============================================================================
//
//	Interface support
//
// =============================================================================

// -----------------------------------------------------------------------------
// Convert an API_Coord3D to an API_Coord
// -----------------------------------------------------------------------------

inline API_Coord ToCoord (const API_Coord3D& inCoo)
{
	API_Coord coo = { inCoo.x, inCoo.y };
	return coo;
}

// -----------------------------------------------------------------------------
// Ask the user to click a point
// -----------------------------------------------------------------------------

bool	ClickAPoint (const char		*prompt,
					 API_Coord		*c)
{
	API_GetPointType	pointInfo = {};
	GSErrCode			err;

	CHTruncate (prompt, pointInfo.prompt, sizeof (pointInfo.prompt));
	pointInfo.changeFilter = false;
	pointInfo.changePlane  = false;
	err = ACAPI_Interface (APIIo_GetPointID, &pointInfo, nullptr);
	if (err != NoError) {
		if (err != APIERR_CANCEL) {
			WriteReport_Err ("Error in APIIo_GetPointID", err);
		}

		return false;
	}

	c->x = pointInfo.pos.x;
	c->y = pointInfo.pos.y;

	return true;
}		// ClickAPoint


// -----------------------------------------------------------------------------
// Input an arc
// -----------------------------------------------------------------------------

bool	GetAnArc (const char*	prompt,
				  API_Coord*	origin,
				  API_Coord*	startPos,
				  API_Coord*	endPos,
				  bool*			isArcNegative /*= nullptr*/)
{
	API_GetPointType	pointInfo = {};
	API_GetLineType		lineInfo = {};
	API_GetArcType		arcInfo = {};
	GSErrCode			err;

	CHTruncate (prompt, pointInfo.prompt, sizeof (pointInfo.prompt));
	err = ACAPI_Interface (APIIo_GetPointID, &pointInfo, nullptr);
	if (err != NoError) {
		return false;
	}

	CHTruncate (prompt, lineInfo.prompt, sizeof (lineInfo.prompt));
	lineInfo.startCoord = pointInfo.pos;						// line starts with the clicked point
	lineInfo.disableDefaultFeedback = false;					// draw the default thick rubber line

	err = ACAPI_Interface (APIIo_GetLineID, &lineInfo, nullptr);
	if (err != NoError) {
		return false;
	}

	CHTruncate (prompt, arcInfo.prompt, sizeof (arcInfo.prompt));
	arcInfo.origo = lineInfo.startCoord;						// set arc origo
	arcInfo.startCoord = lineInfo.pos;							// arc starts with the second clicked point
	arcInfo.startCoordGiven = true;
	arcInfo.disableDefaultFeedback = false;						// draw the default thick rubber line

	err = ACAPI_Interface (APIIo_GetArcID, &arcInfo, nullptr);
	if (err != NoError) {
		return false;
	}

	if (origin != nullptr)
		*origin = ToCoord (arcInfo.origo);
	if (startPos != nullptr)
		*startPos = ToCoord (arcInfo.startCoord);
	if (endPos != nullptr)
		*endPos = ToCoord (arcInfo.pos);
	if (isArcNegative != nullptr)
		*isArcNegative = arcInfo.negArc ? true : false;

	return true;
}		// GetAnArc


// -----------------------------------------------------------------------------
// Ask the user to click an element
// 'needTypeID' specifies the requested element type
//	- API_ZombieElemID: all types pass
//	- API_XXXID: 		only this type will pass
// Return:
//	true:	the user clicked the correct element
//	false:	the input is canceled or wrong type of element was clicked
// -----------------------------------------------------------------------------

bool	ClickAnElem (const char*			prompt,
					 const API_ElemType&	needType,
					 API_Neig*				neig /*= nullptr*/,
					 API_ElemType*			type /*= nullptr*/,
					 API_Guid*				guid /*= nullptr*/,
					 API_Coord3D*			c /*= nullptr*/,
					 bool					ignorePartialSelection /*= true*/)
{
	API_GetPointType	pointInfo = {};
	API_ElemType		clickedType;
	GSErrCode			err;

	CHTruncate (prompt, pointInfo.prompt, sizeof (pointInfo.prompt));
	pointInfo.changeFilter = false;
	pointInfo.changePlane  = false;
	err = ACAPI_Interface (APIIo_GetPointID, &pointInfo, nullptr);
	if (err != NoError) {
		if (err != APIERR_CANCEL) {
			WriteReport_Err ("Error in APIIo_GetPointID", err);
		}

		return false;
	}

	if (pointInfo.neig.neigID == APINeig_None) {		// try to find polygonal element clicked inside the polygon area
		API_Elem_Head		elemHead = {};
		API_ElemSearchPars	pars = {};
		pars.type = needType;
		pars.loc.x = pointInfo.pos.x;
		pars.loc.y = pointInfo.pos.y;
		pars.z = 1.00E6;
		pars.filterBits = APIFilt_OnVisLayer | APIFilt_OnActFloor;
		err = ACAPI_Goodies (APIAny_SearchElementByCoordID, &pars, &elemHead.guid);
		if (err == NoError) {
			elemHead.type = pars.type;
			ElemHead_To_Neig (&pointInfo.neig, &elemHead);
		}
	}

	if (pointInfo.neig.elemPartType != APINeigElemPart_None && ignorePartialSelection) {
		pointInfo.neig.elemPartType = APINeigElemPart_None;
		pointInfo.neig.elemPartIndex = 0;
	}

	clickedType = Neig_To_ElemID (pointInfo.neig.neigID);

	if (neig != nullptr)
		*neig = pointInfo.neig;
	if (type != nullptr)
		*type = clickedType;
	if (guid != nullptr)
		*guid = pointInfo.neig.guid;
	if (c != nullptr)
		*c = pointInfo.pos;

	if (clickedType == API_ZombieElemID)
		return false;

	bool good = (needType == API_ZombieElemID || needType == clickedType);

	if (!good && clickedType == API_SectElemID) {
		API_Element element = {};
		element.header.guid = pointInfo.neig.guid;
		if (ACAPI_Element_Get (&element) == NoError)
			good = (needType == element.sectElem.parentType);
	}

	return good;
}		// ClickAnElem


// -----------------------------------------------------------------------------
// Ask the user to click several elements of the requested type
//	return the neigs
// -----------------------------------------------------------------------------

GS::Array<API_Neig>	ClickElements_Neig (const char		*prompt,
										API_ElemTypeID	needTypeID)
{
	API_Neig			theNeig;
	GS::Array<API_Neig> neigs;

	while (ClickAnElem (prompt, needTypeID, &theNeig) && theNeig.neigID != APINeig_None) {
		neigs.Push (theNeig);
	}

	return neigs;
}		// ClickElements_Neig


// -----------------------------------------------------------------------------
// Ask the user to click several elements of the requested type
//	return element guids
// -----------------------------------------------------------------------------

GS::Array<API_Guid>	ClickElements_Guid (const char		*prompt,
										API_ElemTypeID	needTypeID)
{
	return ClickElements_Neig (prompt, needTypeID).Transform<API_Guid> ([] (const API_Neig& neig) { return neig.guid; });
}		// ClickElements_Guid


// -----------------------------------------------------------------------------
// Return the state of a checked menu item
// -----------------------------------------------------------------------------

bool		GetMenuItemMark (short menuResID, short itemIndex)
{
	API_MenuItemRef		itemRef = {};
	GSFlags				itemFlags = 0;

	itemRef.menuResID = menuResID;
	itemRef.itemIndex = itemIndex;

	ACAPI_Interface (APIIo_GetMenuItemFlagsID, &itemRef, &itemFlags);

	return (itemFlags & API_MenuItemChecked) != 0;
}		// GetMenuItemMark


// -----------------------------------------------------------------------------
// Toggle a checked menu item
// -----------------------------------------------------------------------------

bool		InvertMenuItemMark (short menuResID, short itemIndex)
{
	API_MenuItemRef		itemRef = {};
	GSFlags				itemFlags = 0;

	itemRef.menuResID = menuResID;
	itemRef.itemIndex = itemIndex;

	ACAPI_Interface (APIIo_GetMenuItemFlagsID, &itemRef, &itemFlags);

	if ((itemFlags & API_MenuItemChecked) == 0)
		itemFlags |= API_MenuItemChecked;
	else
		itemFlags &= ~API_MenuItemChecked;

	ACAPI_Interface (APIIo_SetMenuItemFlagsID, &itemRef, &itemFlags);

	return (itemFlags & API_MenuItemChecked) != 0;
}		// InvertMenuItemMark


// -----------------------------------------------------------------------------
// Disable/Enable a menu item
// -----------------------------------------------------------------------------

void		DisableEnableMenuItem (short menuResID, short itemIndex, bool disable)
{
	API_MenuItemRef		itemRef = {};
	GSFlags				itemFlags = 0;

	itemRef.menuResID = menuResID;
	itemRef.itemIndex = itemIndex;

	ACAPI_Interface (APIIo_GetMenuItemFlagsID, &itemRef, &itemFlags);

	if (disable)
		itemFlags |= API_MenuItemDisabled;
	else
		itemFlags &= ~API_MenuItemDisabled;

	ACAPI_Interface (APIIo_SetMenuItemFlagsID, &itemRef, &itemFlags);

	return;
}		// DisEnableMenuItem


// =============================================================================
//
//	Geometry support
//
// =============================================================================

// -----------------------------------------------------------------------------
// Tell whether an arc starts from the given node in the polygon
//	Return:
//		-1		no arc starts from the given node
//		(long)	the index into the polygon arcs array
// -----------------------------------------------------------------------------

Int32		FindArc (const API_PolyArc *parcs, Int32 nArcs, Int32 node)
{
	Int32		i;

	if (parcs == nullptr)
		return (-1);
	for (i = 0; i < nArcs; i++)
		if (parcs [i].begIndex == node)
			return (i);
	return (-1);
}		// FindArc


// -----------------------------------------------------------------------------
// Return the origin of the given arc
// -----------------------------------------------------------------------------

bool		ArcGetOrigo (const API_Coord	*begC,
						 const API_Coord	*endC,
						 double				angle,
						 API_Coord			*origo)
{
	double	xm, ym, m;

	if (fabs (angle) < EPS) {
		origo->x = 0.0;
		origo->y = 0.0;
		return false;
	}
	xm = begC->x + endC->x;
	ym = begC->y + endC->y;
	if (fabs (fabs (angle) - PI) < EPS) {
		origo->x = xm / 2;
		origo->y = ym / 2;
	} else {
		m = 1.0 / tan (angle / 2.0);
		origo->x = (xm - m * (endC->y - begC->y)) / 2;
		origo->y = (ym + m * (endC->x - begC->x)) / 2;
	}
	return true;
}		// ArcGetOrigo


// -----------------------------------------------------------------------------
// Return the angle of the line between the two 2D points
// -----------------------------------------------------------------------------

double		ComputeFiPtr (const API_Coord *c1, const API_Coord *c2, bool enableNegativeAngle /*= false*/)
{
	double		fi;
	double		dx,dy;

	dx = c2->x - c1->x;
	dy = c2->y - c1->y;
	if (fabs (dx) < EPS && fabs (dy) < EPS)
		fi = 0.0;
	else {
		fi = atan2 (dy, dx);
		if (fi < 0.0 && !enableNegativeAngle)
			fi = fi + 2.0 * PI;
	}
	return (fi);
}		// ComputeFiPtr


// -----------------------------------------------------------------------------
// Return the distance of the two 2D points
// -----------------------------------------------------------------------------

double		DistCPtr (const API_Coord *c1, const API_Coord *c2)
{
	double		dx, dy;

	dx = c1->x - c2->x;
	dy = c1->y - c2->y;

	return (sqrt (dx * dx + dy * dy));
}		// DistCPtr

