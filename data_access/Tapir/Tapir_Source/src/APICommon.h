// *****************************************************************************
// Helper functions for Add-On development
// *****************************************************************************

#ifndef	_APICOMMON_H_
#define	_APICOMMON_H_

#include "GSRoot.hpp"
#include "UniString.hpp"
#include "AngleData.h"
#include "RealNumber.h"

#if PRAGMA_ENUM_ALWAYSINT
	#pragma enumsalwaysint on
#elif PRAGMA_ENUM_OPTIONS
	#pragma options enum=int
#endif


/* -- Messages ------------------------------- */

template<typename... Args>
void		WriteReport (const char* format, Args&&... args)
{
	ACAPI_WriteReport (format, false, std::forward<Args> (args)...);
}

template<typename... Args>
void		WriteReport_Alert (const char* format, Args&&... args)
{
	ACAPI_WriteReport (format, true, std::forward<Args> (args)...);
}

void		WriteReport_Err (const char* info, GSErrCode err);
void		WriteReport_End (GSErrCode err);

void		ErrorBeep (const char* info, GSErrCode err);


/* -- Conversions ---------------------------- */

API_ElemType			Neig_To_ElemID (API_NeigID neigID);

bool					ElemHead_To_Neig (API_Neig *neig, const API_Elem_Head *elemHead);

const char*				ErrID_To_Name  (GSErrCode err);
const char*				LibID_To_Name  (API_LibTypeID typeID);
const char*				AttrID_To_Name (API_AttrTypeID typeID);
const GS::UniString		ElemID_To_Name (const API_ElemType& type);


/* -- Interface support ---------------------- */

bool		ClickAPoint (const char		*prompt,
						 API_Coord		*c);

bool		GetAnArc (const char*	prompt,
					  API_Coord*	origin,
					  API_Coord*	startPos,
					  API_Coord*	endPos,
					  bool*			isArcNegative = nullptr);

bool		ClickAnElem (const char*			prompt,
						 const API_ElemType&	needType,
						 API_Neig*				neig = nullptr,
						 API_ElemType*			type = nullptr,
						 API_Guid*				guid = nullptr,
						 API_Coord3D*			c = nullptr,
						 bool					ignorePartialSelection = true);


GS::Array<API_Neig>	ClickElements_Neig (const char		*prompt,
										API_ElemTypeID	needTypeID);

GS::Array<API_Guid>	ClickElements_Guid (const char		*prompt,
										API_ElemTypeID	needTypeID);

bool		GetMenuItemMark (short menuResID, short itemIndex);
bool		InvertMenuItemMark (short menuResID, short itemIndex);
void		DisableEnableMenuItem (short menuResID, short itemIndex, bool disable);


/* -- Geometry support ----------------------- */

Int32		FindArc (const API_PolyArc *parcs, Int32 nArcs, Int32 node);
bool		ArcGetOrigo (const API_Coord *begC, const API_Coord *endC, double angle, API_Coord *origo);
double		ComputeFiPtr (const API_Coord *c1, const API_Coord *c2, bool enableNegativeAngle = false);
double		DistCPtr (const API_Coord *c1, const API_Coord *c2);


#if PRAGMA_ENUM_ALWAYSINT
	#pragma enumsalwaysint reset
#elif PRAGMA_ENUM_OPTIONS
	#pragma options enum=reset
#endif

#endif
