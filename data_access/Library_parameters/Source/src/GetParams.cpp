#include "GetParams.hpp"
#include  "DG.h"
#include  "GSRoot.hpp"
#include "GSGuid.hpp"
#include "UniString.hpp"
#include "APIdefs_Properties.h"
#include "ACAPinc.h"
#include "APIdefs.h"
#include "libparams.hpp"
#include "APICommon.h"
#include "StringConversion.hpp"

extern GS::Array<GS::Array<GS::UniString>> arr;

GS::Array<GS::Array<GS::UniString>> GetParams(GS::Array<GS::UniString> inParams) {

	GS::Array<GS::UniString> buf;
	GS::UniString str("Command Failure");

	API_Guid gu;
	GS::Guid gsguid;
	
	// Pass Library Part guid in UniString form
	gsguid.ConvertFromString(inParams[0].ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);

	API_Element element;
	API_LibPart 		libPart;
	GSErrCode			err;
	BNZeroMemory(&element, sizeof(API_Element));

	element.header.guid = gu;
	err = ACAPI_Element_Get(&element);
	if (err == NoError) {
		BNZeroMemory(&libPart, sizeof(API_LibPart));
		GS::UniString infoString;
		ACAPI_Database(APIDb_GetCompoundInfoStringID, &element.header.guid, &infoString);
		
		libPart.index = element.object.libInd;
	}
	err = ACAPI_LibPart_Get(&libPart);
	if (libPart.location != nullptr) {
		delete libPart.location;
		libPart.location = nullptr;
		str = libPart.docu_UName;
		char temp[254];
		sprintf(temp, "%d", libPart.index );
		buf.Push(str);
		str = temp;
		buf.Push(str);
		arr.Push(buf);
	}
	API_ElementMemo memo;
	BNZeroMemory(&memo, sizeof(API_ElementMemo));
	err = ACAPI_Element_GetMemo(gu, &memo);
	API_AddParType** addPars;
	double				a, b;
	Int32 				addParNum;
	Int32 libInd = element.lamp.libInd;
	err = ACAPI_LibPart_GetParams(libInd, &a, &b, &addParNum, &addPars);
	API_AddParType** params = memo.params;
	
	

	if (params == nullptr || *params == nullptr)
	{ 
		str = "Parameters Not Found";
		buf.Push(str);
		arr.Push(buf);
		return arr;
	}

	addParNum = BMGetHandleSize(reinterpret_cast<GSHandle> (params)) / sizeof(API_AddParType);
	for (Int32 i = 0; i < addParNum; i++) {
		if ((*params)[i].typeMod == API_ParSimple) {
			GS::UniString valueStr((*params)[i].value.uStr);
			
			WriteParameter((*params)[i].typeID, GS::UniString((*params)[i].name),
				(*params)[i].value.real, valueStr, 0, 0);
		}
		else {
		}

	}
	return arr;
}

GSErrCode ChangeParams(GS::Array<GS::UniString> inParams) {
	API_Guid guid;
	GSErrCode err = NoError;
	if (CHEqualASCII("NONE",inParams[0].ToCStr().Get())) {
		char hold[254];
		sprintf(hold, "Click on a element to acquire its Guid");
		
		API_ElemType z;
		z.typeID = API_ZombieElemID;
		bool x = ClickAnElem(hold, z, nullptr, nullptr, &guid);
		if (x == false) {
			err = APIERR_BADID;
			ACAPI_WriteReport("Click a point Failed", true);
			return err;
		}
	}
	else {
		GS::Guid gu;
		gu.ConvertFromString(inParams[0].ToCStr().Get());
		guid = GSGuid2APIGuid(gu);
	}

	API_Element element;
	API_LibPart 		libPart;
	BNZeroMemory(&element, sizeof(API_Element));
	element.header.guid = guid;
	err = ACAPI_Element_Get(&element);
	if (err == NoError) {
		BNZeroMemory(&libPart, sizeof(API_LibPart));
		libPart.index = element.object.libInd;
	}
	else {
		ACAPI_WriteReport("Element Get Failed", true);
		return err;
	}
	API_ElementMemo memo;
	BNZeroMemory(&memo, sizeof(API_ElementMemo));
	err = ACAPI_Element_GetMemo(guid, &memo);
	if (err) {
		ACAPI_WriteReport("GetMemo Failed", true);
		return err;
	}
	API_AddParType** addPars;
	double				a, b;
	Int32 				addParNum;
	Int32 libInd = element.lamp.libInd;
	err = ACAPI_LibPart_GetParams(libInd, &a, &b, &addParNum, &addPars);// not used
	if (err) {
		ACAPI_WriteReport("LibParams Get Failed - %s", true, ErrID_To_Name(err));
	}
	API_AddParType** params = memo.params;

	addParNum = BMGetHandleSize(reinterpret_cast<GSHandle> (params)) / sizeof(API_AddParType);
	for (Int32 i = 0; i < addParNum; i++) {
		if (CHEqualASCII(inParams[1].ToCStr().Get(), (*params)[i].name, CS_CaseInsensitive)) {
			if (inParams[3] == "string")
				GS::ucsncpy((*params)[i].value.uStr, inParams[2].ToUStr(), API_UAddParStrLen - 1);
			else if (inParams[3] == "int") {
				int value;
				UniStringToValue(inParams[2], value, GS::NotStrict);
				(*params)[i].value.real = value;
			}
			else if (inParams[3] == "real") {
				double value;
				UniStringToValue(inParams[2], value, GS::NotStrict);
				(*params)[i].value.real = value;
			}
			else
				continue;
		}
	}
	memo.params = params;
	UInt64  mask = {};
	return ACAPI_CallUndoableCommand("Property Definition API Function",
		[&]() -> GSErrCode {
		err = ACAPI_Element_ChangeMemo(guid, APIMemoMask_AddPars, &memo);
		return err;
	});
	if (err)
		ACAPI_WriteReport("Change memo Failed - %s",true,ErrID_To_Name(err));
	err = ACAPI_DisposeElemMemoHdls(&memo);
	return err;
}