#pragma once
#ifndef __utilities__
#define __utilities

#include <string>
#include <map>
#include <APIdefs_Elements.h>


//API_ElemTypeID  GetenumIndex(const std::string s);
API_ElemTypeID  GetenumIndex(const char* s);
static std::map<std::string, API_ElemTypeID> string2week;
GS::UniString Do_GetValue(GS::UniString, GS::UniString);
GS::Array<GS::Array<GS::UniString>> Do_GetParams(GS::UniString); // Gets all parameters for a element
GS::Array<GS::UniString> Do_GetElem(GS::Array < GS::UniString> );
GS::Array<GS::Array<GS::UniString>> Do_GetProjInfo(GS::UniString);
GS::Array<GS::UniString> Do_GetPoint(GS::UniString);
GS::Array<GS::Array<GS::UniString>> Do_GetSelected();
GS::Array<GS::Array<GS::UniString>> Do_GetLibParams(GS::Array<GS::UniString>inParams);
GS::Array<GS::UniString> Do_SetProjInfo(GS::Array<GS::UniString>);
GS::Array<GS::UniString> Do_GetProjInfo(GS::Array<GS::UniString>);
GS::Array<GS::UniString> Do_GetProjValue(GS::Array<GS::UniString>);
GS::UniString Do_GetValueFromGuid(GS::UniString, GS::UniString);
GS::Array<GS::Array<GS::UniString>> Do_GetElemList(GS::Array<GS::UniString>);
GS::UniString Do_SetLabelText(GS::UniString, GS::UniString,GS::UniString,GS::UniString);
GS::UniString Do_GetAutoText(GS::UniString, GS::UniString);
GS::UniString Do_ChangeText(GS::UniString,GS::UniString,GS::UniString);
GS::Array<GS::UniString> Do_GetQuantity(GS::Array<GS::UniString>);
GS::UniString GetWallRef(GS::UniString value,GS::UniString objType);
GS::UniString ChangeCatCode(GS::UniString guid,GS::UniString code);
GS::Array<GS::Array<GS::UniString>> GetBMAtt(GS::UniString value);
GS::UniString SetMSG(GS::UniString msg);
GS::Array<GS::Array<GS::UniString>> Do_GetBMProperties(GS::UniString,GS::UniString);
GS::UniString GetActStoryName(void);
#endif // !__utilities__

