#pragma once
#ifndef _GetParams_
#define _GetParams_

#include "Definitions.hpp"
#include "UniString.hpp"
#include "GSRoot.hpp"

void Do_LibParams(void);
GS::Array<GS::Array<GS::UniString>> GetParams(GS::Array<GS::UniString>);
GSErrCode ChangeParams(GS::Array<GS::UniString>);


#endif