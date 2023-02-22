#include	<math.h>

#include	"Definitions.hpp"
#include	"ACAPinc.h"	
#include	"APIEnvir.h"
#include	"DG.h"
//#include	"PropertyUtils.hpp"

#define		EOLInFileWIN		"\xD\xA"
#define		EOLInFileWIN_LEN	2
#define		EOLInFileMAC		"\xA"
#define		EOLInFileMAC_LEN	1

const double	PI = 3.141592653589793;		/* The Ludolph's constant	*/
const double	RADDEG = 57.29577951308232;		/* Radian->degree conversion ( 180.0/PI ) */
const double	DEGRAD = 0.017453292519943295;	/* Degree->radian conversion ( PI/180.0 ) */

#ifdef WINDOWS
#define		EOLInFile			EOLInFileWIN
#define		EOLInFile_LEN		EOLInFileWIN_LEN
#endif

extern GS::Array<GS::Array<GS::UniString>> arr;
GS::Array<GS::UniString> buf;
GS::UniString str;

 enum  NewLineFlag {
	NoNewLine,
	WithNewLine
}x;

GSErrCode	WriteStr(const GS::UniString& val, NewLineFlag newLine /*= NoNewLine*/);

GSErrCode		lastErr;						// cummulated file i/o error
bool			isNewLine;
short 			tabcount;            			// tabulator counter
DGUnitData		unit;

GSErrCode Write(Int32 nBytes, GSPtr data)
{
	// code removed

	return lastErr;
}		// Write

void GetAttrName(char* name,
	API_AttrTypeID		typeID,
	API_AttributeIndex index,
	GS::UniString* uniName)
{
	API_Attribute	attrib;

	BNZeroMemory(&attrib, sizeof(API_Attribute));
	attrib.header.typeID = typeID;
	attrib.header.index = index;
	attrib.header.uniStringNamePtr = uniName;

	if (ACAPI_Attribute_Get(&attrib) == NoError) {
		if (name != nullptr)
			strcpy(name, attrib.header.name);
	}
	else if (name != nullptr)
		strcpy(name, "???");

	if (typeID == API_MaterialID && attrib.material.texture.fileLoc != nullptr) {
		delete attrib.material.texture.fileLoc;
		attrib.material.texture.fileLoc = nullptr;
	}
	return;
}		// GetAttrName

GSErrCode WriteStr(const GS::UniString& val, NewLineFlag newLine = NoNewLine)
{
	
	return lastErr;
}		// WriteStr


GSErrCode	WriteStr(const char* val, NewLineFlag newLine = NoNewLine)
{
	
	return lastErr;
}		// WriteStr

GSErrCode	WriteInt(const Int32 val, NewLineFlag newLine /* = NoNewLine*/)
{
	
	return lastErr;
}		// WriteInt


// -----------------------------------------------------------------------------
//	Write a float value into the file
// -----------------------------------------------------------------------------

GSErrCode	WriteFloat(const double value)
{
	
	return lastErr;
}		// WriteFloat


GSErrCode	WriteAngle(const double value)
{
	
	return lastErr;
}		// WriteAngle


GSErrCode WrNewLine(void)
{
	// Code Removed -- Not Needed
	return lastErr;
}		// WrNewLine


GSErrCode Write(const GS::UniString& val)
{
	//Removed Code
	return lastErr;
}		// Write



void WriteParameter(API_AddParID typeID,
	const GS::UniString& varname,
	double 				value,
	const GS::UniString& valueStr,
	Int32 					dim1,
	Int32 					dim2)
{
	if (dim1 < 0)
		return;

	GS::UniString head;
	GS::UniString name;
	buf.Clear();
	if (dim1 == 0 && dim2 == 0) {
		// Variable Name -- Common for all Parameters
		str = varname;
		buf.Push(str);
	}
	else {	
	}

	switch (typeID) {
	case APIParT_FillPat:
		GetAttrName(nullptr, API_FilltypeID, (short)value, &name);
		str = name;
		buf.Push(str);
		buf.Push("FillPat");
		break;

	case APIParT_Mater:
		GetAttrName(nullptr, API_MaterialID, (short)value, &name);
		str = name;
		buf.Push(str);
		buf.Push("Material");
		break;

	case APIParT_BuildingMaterial:

		GetAttrName(nullptr, API_BuildingMaterialID, (short)value, &name);
		str = name;
		buf.Push(str);
		buf.Push("BuildingMaterial");
		break;

	case APIParT_Profile:
		GetAttrName(nullptr, API_ProfileID, (short)value, &name);
		str = name;
		buf.Push(str);
		buf.Push("Profile");
		break;

	case APIParT_LineTyp:
		GetAttrName(nullptr, API_LinetypeID, (short)value, &name);
		str = name;
		buf.Push(str);
		buf.Push("LineType");
		break;

	case APIParT_Boolean:
	case APIParT_LightSw:
		str = ((Int32)value) != 0 ? "on" : "off";
		buf.Push(str);
		buf.Push("Boolean");
		break;

	case APIParT_Integer:
	case APIParT_PenCol:
	{
	char	buffer[256] = {};
	sprintf(buffer, "%ld ", (GS::LongForStdio)value);
	str = buffer;
	buf.Push(str);
	buf.Push("PenCol");
	}
		break;

	case APIParT_Angle:	
	{
		GS::UniString	buffer1 = DGDoubleToStringUnit(&unit, value * RADDEG, DG_ET_POLARANGLE);
		str = buffer1;
		buf.Push(str);
		buf.Push("Angle");
	}
		break;
	case APIParT_ColRGB:
	case APIParT_Intens:
	case APIParT_RealNum:
	case APIParT_Length:
	{
		unit.lengthDigits = 3;
		
		GS::UniString buffer2;
		buffer2 = DGDoubleToStringUnit(&unit, value, DG_ET_LENGTH);
		str = buffer2;
		buf.Push(str);
		buf.Push("RealNum");
	}
		break;

	case APIParT_CString:
		str = valueStr;
		buf.Push(str);
		buf.Push("String");
		break;

	case APIParT_Dictionary:
		DBBREAK_STR("Dictionaries are not handled here.");
		str = " (dictionary)";
		buf.Push(str);
		buf.Push("Dictionary");
		break;

	case APIParT_Separator:	
		str = "separator";
		buf.Push(str);
		buf.Push("Separator");
		break;

	case APIParT_Title:		
		str = "Title";
		buf.Push(str);
		buf.Push("Title");
		break;

	default:
		str = "Parameter Type Not Handled";
		buf.Push(str);
		buf.Push("N/A");
		break;

	}
	
	arr.Push(buf);
	
	return;
}		// WriteParameter

void WriteParams(API_AddParType** params)
{
	double	value;
	Int32 	i1, i2, ind;

	if (params == nullptr || *params == nullptr)
		return;

	UInt32 addParNum = BMGetHandleSize(reinterpret_cast<GSHandle> (params)) / sizeof(API_AddParType);

	for (UInt32 i = 0; i < addParNum; i++) {
		if ((*params)[i].typeMod == API_ParSimple) {
			GS::UniString valueStr((*params)[i].value.uStr);
			WriteParameter((*params)[i].typeID, GS::UniString((*params)[i].name),
				(*params)[i].value.real, valueStr, 0, 0);
		}
		else {
			if ((*params)[i].typeID == APIParT_Dictionary) {
				DBBREAK_STR("Dictionaries are not allowed in arrays.");
				continue;
			}

			ind = 0;
			WriteParameter((*params)[i].typeID, (*params)[i].name,
				0.0, "", -(*params)[i].dim1, -(*params)[i].dim2);

			for (i1 = 1; i1 <= (*params)[i].dim1; i1++) {

				for (i2 = 1; i2 <= (*params)[i].dim2; i2++) {
					if ((*params)[i].typeID != APIParT_CString) {
						value = (Int32)((double*)
							*(*params)[i].value.array)[ind];
						ind++;
						WriteParameter((*params)[i].typeID, "", value, "", i1, i2);
					}
					else {
						value = 0.0;
						GS::uchar_t* uValueStr = (reinterpret_cast<GS::uchar_t*>(*(*params)[i].value.array)) + ind;
						ind += GS::ucslen32(uValueStr) + 1;
						WriteParameter((*params)[i].typeID, "", value, GS::UniString(uValueStr), i1, i2);
					}
				}
			}
		}
	}
}		// WriteParams
