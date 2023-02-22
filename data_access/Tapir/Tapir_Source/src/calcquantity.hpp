#pragma once
#ifndef __calcquantity__
#define __calcquantity__


#include "Definitions.hpp"
struct API_Property;
namespace GS {
	class UniString;
}

namespace PropertyUtils {

	GSErrCode	PropertyToString(const API_Property& property, GS::UniString& propertyValue);

}

GS::Array<GS::Array<GS::UniString>>	Do_CalcQuantities(GS::UniString,GS::UniString);
GS::Array<GS::Array<GS::UniString>> Do_CalcComponentQuantities(GS::UniString);
GS::Array<GS::Array<GS::UniString>>	Do_GetComponents(GS::UniString); // Get the components of a clicked element




#endif
