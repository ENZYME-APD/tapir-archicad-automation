#pragma once
#ifndef _LIBPARAMS_
#define _LIBPARAMS_

#include "UniString.hpp"
#include "APIdefs_LibraryParts.h"

void WriteParameter(API_AddParID typeID, const GS::UniString& varname,double value,
	const GS::UniString& valueStr,Int32 dim1,Int32 	dim2);

#endif