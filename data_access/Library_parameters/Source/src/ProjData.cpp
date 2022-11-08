#include	"ProjData.hpp"
#include	"ObjectState.hpp"
#include	"FileSystem.hpp"
#include	"OnExit.hpp"
#include    "APICommon.h"
#include    "utilities.hpp"
#include    "string.h"
#include    "StringConversion.hpp"
#include    "UniString.hpp"
#include    "calcquantity.hpp"
#include    "labels.hpp"
#include    "FontPopupDialog.hpp"
#include    "DGDialog.hpp"
#include    "StringConversion.hpp"


constexpr const char* AdditionalJSONCommandsNamespace = "AdditionalJSONCommands";


constexpr const char* ErrorResponseField = "error";
constexpr const char* ErrorCodeResponseField = "code";
constexpr const char* ErrorMessageResponseField = "message";


static GS::ObjectState CreateErrorResponse (APIErrCodes errorCode, const char* errorMessage)
{
	GS::ObjectState error;
	error.Add (ErrorCodeResponseField, errorCode);
	error.Add (ErrorMessageResponseField, errorMessage);
	return GS::ObjectState (ErrorResponseField, error);
}

void  Show_FontPopUpDialog(GS::UniString input)
{
	FontPopupDialog dialog;
	int xx = dialog.GetId();
	dialog.GetResourceId();
	dialog.SetText(input);
	dialog.SetGrowType(DG::Dialog::GrowType::HVGrow);
	if (DBERROR(dialog.GetId() == 0)){
	ACAPI_WriteReport("Can not obtain ID for FontPopup", true);
		return;
	}
	
	dialog.Invoke();
}



GS::String ProjDataCommand::GetName() const
{
	return "ProjData";
}
/*
GS::String ProjDataCommand::GetNamespace() const
{
	return "TapirCommand";
}
*/

GS::Optional<GS::UniString> ProjDataCommand::GetSchemaDefinitions() const
{
	//GSHandle myJSONFileContent = RSLoadResource('DATA', ACAPI_GetOwnResModule(), 1000);
	//return GS::UniString(*myJSONFileContent);

	GS::UniString resourceData;
	GSHandle data = RSLoadResource('DATA', ACAPI_GetOwnResModule(), 1000);
	GSSize handleSize = BMhGetSize(data);
	if (data != nullptr) {
		resourceData.Append(*data, handleSize);
		BMhKill(&data);
	}

	return resourceData;

}



GS::Optional<GS::UniString> ProjDataCommand::GetInputParametersSchema() const
{
	return GS::UniString(R"({
			"$ref": "#/MyInputParameters"
	})");
}


GS::Optional<GS::UniString> ProjDataCommand::GetResponseSchema() const
{
	return GS::UniString(R"({
			"$ref": "#/MyOutputParameters"
			},
			"error": {
				"$ref": "APITypes.json#/definitions/Error"
			})");

}


GS::ObjectState	ProjDataCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
	GS::UniString inCommand;
	GS::Array<GS::UniString> inParams;
	//API_Guid guid;
	GS::ObjectState response;
	GS::Array<GS::UniString> buf;
	

	parameters.Get("command", inCommand);
	parameters.Get("inParams", inParams);
	
	if (inCommand == "GetElem") {
		buf = Do_GetElem(inParams);
	}
	else if (inCommand == "GetParams") {

		GS::Array<GS::Array<GS::UniString>> buf2;
		GS::Array<GS::UniString> buf;
		GS::UniString res;
		buf2.Clear();
		buf2 = Do_GetParams(inParams[0]); // selected element guid
		response.Add("outparams2", buf2); // Load info
		return response;
	}
	else if (inCommand == "GetValue") {
		GS::UniString ret;
		ret = Do_GetValue(inParams[0],inParams[1]);
		buf.Push(ret);
	}
	else if (inCommand == "GetProjInfo") {
		GS::UniString ret;
		GS::Array<GS::Array<GS::UniString>> buf2;
		buf2 = Do_GetProjInfo(inParams[0]);
		response.Add("outparams2", buf2); // Load info
		return response;
		//buf.Push(ret);
	}
	else if (inCommand == "GetPoint") {
		buf = Do_GetPoint(inParams[0]);
	}

	else if (inCommand == "GetSelected"){
		GS::Array<GS::Array<GS::UniString>> buf2;
		buf2 = Do_GetSelected();
		response.Add("outparams2", buf2); // Load info
		return response;
	}

	else if (inCommand == "SetProjInfo") {
		buf = Do_SetProjInfo(inParams);
	}
	else if (inCommand == "GetProjValue") {
		buf = Do_GetProjValue(inParams);
	}
	else if (inCommand == "GetValueFromGuid") {
		GS::UniString str;
		str = Do_GetValueFromGuid(inParams[0],inParams[1]);
		buf.Push(str);
	}
	else if (inCommand == "GetElemList") {
		GS::Array<GS::Array<GS::UniString>> buf;
		buf = Do_GetElemList(inParams);
		response.Add("outParams2", buf);
		return response;
	}
	else if (inCommand == "SetLabelText") {
		GS::UniString buf;
		buf = Do_SetLabelText(inParams[0],inParams[1],inParams[2],inParams[3]);
		response.Add("outParams", buf);
		return response;
	}
	else if (inCommand == "GetAutoText") {
		GS::UniString buf;
		buf = Do_GetAutoText(inParams[0], inParams[1]);
		response.Add("outParams", buf);
		return response;
	}
	else if (inCommand == "ChangeText") {

		API_Guid gu;
		GS::Guid gsguid;
		gsguid.ConvertFromString(inParams[0].ToCStr().Get());
		gu = GSGuid2APIGuid(gsguid);
		GS::UniString buf;
		buf = Do_ChangeText(inParams[0],inParams[1],inParams[2]);
		response.Add("outParams", buf);
		return response;
	}
	else if (inCommand == "CreateText") {

		GS::UniString buf;
		buf = "Created Text";
		double x = 0;
		double y = 0;
		UniStringToValue(inParams[2], x);
		UniStringToValue(inParams[3], y);
		API_Coord c;
		c.x = x;
		c.y = y;
		//x = inParams[2];
		Do_CreateText(inParams[0],inParams[1],c);
		response.Add("outParams", buf);
		return response;
	}
	else if (inCommand == "CreateLabel_Associative") {

		GS::UniString buf;
		buf = "Created Label";
		Do_CreateLabel_Associative(inParams[0],inParams[1]);
		response.Add("outParams", buf);
		return response;
	}
	
	else if (inCommand == "ChangeLabelText") {

		API_Guid gu;
		GS::Guid gsguid;
		gsguid.ConvertFromString(inParams[0].ToCStr().Get());
		gu = GSGuid2APIGuid(gsguid);
		GS::UniString buf;
		buf = "End of Label Change";
		 Do_Label_Edit(inParams[0],inParams[1]);
		//Do_CreateWord(APINULLGuid,inParams[1]);
		response.Add("outParams", buf);
		return response;
	}
	else if (inCommand == "GetQuantity") {
		GS::Array<GS::UniString> buf;
		buf = Do_GetQuantity(inParams);
		response.Add("outParams2", buf);
		return response;
	}
	else if (inCommand == "CalcQuantities") {
		GS::Array<GS::Array<GS::UniString>> arr;
		GS::Array<GS::UniString> buf;
		GS::UniString buffer("Success");
		buf.Push(buffer);
		 arr = Do_CalcQuantities(inParams[0],inParams[1]);
		response.Add("outParams2",arr);
		return response;
	}
	else if (inCommand == "CalcComponentQuantities") {
		GS::Array<GS::Array<GS::UniString>> arr;
		GS::Array<GS::UniString> buf;
		GS::UniString buffer("Success");
		buf.Push(buffer);
		arr = Do_CalcComponentQuantities(inParams[0]);
		response.Add("outParams2", arr);
		return response;
	}
	else if (inCommand == "GetComponents") {
		GS::Array<GS::Array<GS::UniString>> arr;
		GS::Array<GS::UniString> buf;
		GS::UniString buffer("Success");
		buf.Push(buffer);
		arr = Do_GetComponents(inParams[0]);
		response.Add("outParams2", arr);
		return response;
	}
	else if (inCommand == "GetWallRef") {

	API_Guid gu;
	GS::Guid gsguid;
	
	gsguid.ConvertFromString(inParams[0].ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	GS::UniString buf;
	buf = "End of Label Change";
	buf = GetWallRef(inParams[0],inParams[1]);
	response.Add("outParams", buf);
	return response;
	}
	else if (inCommand == "ChangeCatCode") {

	API_Guid gu;
	GS::Guid gsguid;
	
	gsguid.ConvertFromString(inParams[0].ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	GS::UniString buf;
	buf = "End of Category Change";
	buf = ChangeCatCode(inParams[0],inParams[1]);
	response.Add("outParams", buf);
	return response;
	}
	else if (inCommand == "GetBMAtt") {

	API_Guid gu;
	GS::Array<GS::Array<GS::UniString>>arrBuf;
	GS::Guid gsguid;
	
	gsguid.ConvertFromString(inParams[0].ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	GS::Array<GS::UniString> buf;
	arrBuf = GetBMAtt(inParams[0]);
	response.Add("outparams2", arrBuf);
	return response;
	}
	else if (inCommand == "SetMSG") {
	GS::UniString str;
	GS::Array<GS::UniString> Buf;
	str = SetMSG(inParams[0]);
	Buf.Push(str);
	response.Add("outparams", Buf);
	return response;
	}
	else if (inCommand == "GetDialog") {
		GS::UniString status("Success");
		if (inParams[0] == "FontPopup") {
			Show_FontPopUpDialog(inParams[1]);
			buf.Push(status);
		}
		else {
			status = "Dialog Not Found";
			buf.Push(status);
		}
	}
	else if (inCommand == "GetInfo") {
		GS::UniString status("Success");
		GS::UniString infoString;
		GS::Guid gsguid;
		API_Guid gu;
		
		gsguid.ConvertFromString(inParams[0].ToCStr().Get());
		gu = GSGuid2APIGuid(gsguid);
		GSErrCode error = ACAPI_Database(APIDb_GetElementInfoStringID, &gu, &infoString);
		if (error) {
			char temp[1024] = {};
			sprintf(temp, "info - %s", ErrID_To_Name(error));
			status = temp;
			buf.Clear();
			buf.Push(status);
		}
		else {
			buf.Clear();
			buf.Push(infoString);
		}
	}
	else if (inCommand == "GetLineLength") {
		GS::UniString status("Success");
		GS::Array<GS::Array<GS::UniString>>arrBuf;
		GS::UniString infoString;
		GS::Guid gsguid;
		API_Guid gu;
		
		gsguid.ConvertFromString(inParams[0].ToCStr().Get());
		gu = GSGuid2APIGuid(gsguid);
		API_Element element;
		API_ElementMemo memo;
		BNZeroMemory(&element, sizeof(API_Element));
		BNZeroMemory(&memo, sizeof(API_ElementMemo));
		GSErrCode err;
		element.header.guid = gu;
		err = ACAPI_Element_Get(&element);
		if (err) {
			char temp[1024] = {};
			sprintf(temp, "Can not get line - %s", ErrID_To_Name(err));
			status = temp;
			buf.Clear();
			buf.Push(status);
			response.Add("outparams", buf); // Load info
			return response;
		}
		else {

			
		}
		err = ACAPI_Element_GetMemo(element.header.guid, &memo);
		if (err) {
			char temp[1024] = {};
			sprintf(temp, "Can not get memo - %s", ErrID_To_Name(err));
			status = temp;
			buf.Clear();
			buf.Push(status);
			response.Add("outparams", buf); // Load info
			return response;
		}
		else {

			API_Coord** coord;
			coord = memo.coords;
			 int const ncoords = element.polyLine.poly.nCoords;
			double x;
			double y;
			response.Clear();
			buf.Clear();
			arrBuf.Clear();
			for (int i = 1; i <= ncoords; i++) {
				buf.Clear();
				x = (*memo.coords)[i].x;
				y = (*memo.coords)[i].y;
				char temp[254];
				sprintf(temp, "%f", x);
				buf.Push(temp);
				sprintf(temp, "%f", y);
				buf.Push(temp);
				arrBuf.Push(buf);
				
			}
			response.Add("outparams2", arrBuf);
			return response;
		}
	}
	else if (inCommand == "GetBMProperties") {
		GS::UniString status("Success");
		GS::UniString infoString;
		GS::Guid gsguid;
		API_Guid gu;
		//char ch[254];
		gu = APIGuidFromString(inParams[0].ToCStr().Get());
		GS::Array<GS::Array<GS::UniString>>arrBuf;
		arrBuf.Clear();
		arrBuf = Do_GetBMProperties(inParams[0],inParams[1]);
		response.Clear();
		response.Add("outparams2", arrBuf);
		return response;
	}
	else if (inCommand == "GetStoryName-not used yet") {
	GS::UniString status("Success");
	GS::UniString infoString;
	GS::Guid gsguid;
	API_Guid gu;
	gu = APIGuidFromString(inParams[0].ToCStr().Get());
	GS::Array<GS::Array<GS::UniString>>arrBuf;
	arrBuf.Clear();
	//arrBuf = Do_GetBMProperties(inParams[0], inParams[1]); -- Add function
	response.Clear();
	response.Add("outparams2", arrBuf);
	return response;
	}
	else if (inCommand == "Do_SetText--Fix for multiText") {
		GS::UniString status("Success");
		GS::UniString infoString;
		GS::Guid gsguid;
		API_Guid renFiltGuid = APINULLGuid;
		GS::UniString text = inParams[0];
		Do_CreateWord(renFiltGuid, text);
	}
	else {
		GS::UniString fail("Command Not Found");
		buf.Push(fail);
	}
	
	
	response.Add("outparams", buf); // Load info
	return response;
}

