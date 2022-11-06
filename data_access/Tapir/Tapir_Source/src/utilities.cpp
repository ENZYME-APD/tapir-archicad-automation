#include <iostream>
#include <string>
#include <map>
#include "APIEnvir.h"
#include "ACAPinc.h"
#include <APIdefs_Goodies.h>
#include <APIdefs_Database.h>
#include "DGModule.hpp"
#include <exception>
#include "APIdefs_Elements.h"
#include "utilities.hpp"
#include "APICommon.h"
#include "ObjectState.hpp"
#include "StringConversion.hpp"
#include "UniString.hpp"
#include "Array.hpp"
#include "realnumber.h"
#include "Quantity.h"
#include "FontPopupDialog.hpp"


API_ElemTypeID el;

API_ElemTypeID  GetenumIndex(const char* s) {
    static std::map<std::string, API_ElemTypeID> string2week{
       { "API_ZombieElemID", API_ZombieElemID },// 0
       { "API_WallID", API_WallID }, //1
       { "API_ColumnID", API_ColumnID }, //2
       { "API_BeamID", API_BeamID }, //3
       { "API_WindowID", API_WindowID }, //4
       { "API_DoorID", API_DoorID }, //5
       { "API_ObjectID", API_ObjectID }, //6",  
       { "API_LampID", API_LampID }, //7
       { "API_SlabID", API_SlabID }, //8
       { "API_RoofID", API_RoofID }, //9
       { "API_MeshID", API_MeshID }, //10
       { "API_TextID", API_TextID }, //15
       { "API_LabelID", API_LabelID }, //16
       { "API_ZoneID", API_ZoneID }, //17
	   { "API_HatchID", API_HatchID},//18
	   { "API_LineID", API_LineID}//19
    }; 
    auto x = string2week.find(s);
    if (x != std::end(string2week)) {
        return x->second;
    }
   
}


GS::Array<GS::Array<GS::UniString>> Do_GetLibParams(GS::Array<GS::UniString>inParams)
{
	GS::Array<GS::Array<GS::UniString>> buf;
	GS::Array<GS::UniString> buf_str;
	GS::UniString inStr = "NoInput";
	buf_str.Push(inStr);
	buf.Push(buf_str);
	GS::UniString guid_str;
	guid_str = inParams[0];
	GS::Guid gsguid;
	gsguid.ConvertFromString(guid_str.ToCStr().Get());
	API_Guid apiguid;
	apiguid = GSGuid2APIGuid(gsguid);
	return buf;
}



GS::Array<GS::UniString> Do_GetElem(GS::Array< GS::UniString> params) {
	API_Guid guid;
	GS::UniString elemType;
	elemType = params[0];
	API_ElemTypeID w = GetenumIndex(elemType.ToCStr().Get());
	
	GS::Array<GS::UniString> buf ;
	char hold[254];
	sprintf(hold, "Click on a %s to acquire its Guid", elemType.ToCStr().Get());
	if (!ClickAnElem(hold, w, nullptr, nullptr, &guid)) {
		elemType = "No Element was clicked";
		buf.Push(elemType);
		return buf;
	}
	else {
		GS::Guid gu;
		gu = APIGuid2GSGuid(guid);
		char element[254];
		gu.ConvertToString(element);
		GS::UniString output(element);
		GS::UniString imput(elemType);
		buf.Push(output);
		buf.Push(elemType); // Load info
		return buf;
	}

}


GS::UniString Do_GetValue(GS::UniString res, GS::UniString param) {
	
	API_Guid guid;
	guid = APIGuidFromString(res.ToCStr().Get());
	GS::UniString value;
	API_ElemTypeID w = GetenumIndex(res.ToCStr().Get());
		GS::HashTable<GS::UniString, GS::UniString> properties{ GS::HashTable<GS::UniString, GS::UniString>() };
		GSErrCode err = ACAPI_Goodies(APIAny_GetPropertyAutoTextKeyTableID, &guid, &properties);
		if (err == NoError) {
			
			if (properties.ContainsKey(param)) {// changed here
				
				GS::UniString area = properties.Get(param); // changed from GS::UniString("Area")
				err = ACAPI_Goodies(APIAny_InterpretAutoTextID, &area, &guid, &value);
				if (err == NoError) {
					
				}
				else {
					ACAPI_WriteReport("Autotext for %s could not be resolved.",true,res.ToCStr().Get());
					return GS::UniString("Failed");
				}

			}
			else {
				ACAPI_WriteReport("Autotext key for %s could not be found.", true, res.ToCStr().Get());
				return GS::UniString("Failed");
			}
		}
		else {
			ACAPI_WriteReport("Error while retrieving the property autotext key table.",true);
			return GS::UniString("Failed");
		}
		
		
	
	return value;
}

GS::Array<GS::Array<GS::UniString>>  Do_GetParams(GS::UniString input) {
	
	GS::Array<GS::Array<GS::UniString>> buf2;
	GS::Array<GS::UniString> buf;
	GS::UniString str;
	
	API_Guid guid;
	API_ElemTypeID w;
	guid = APIGuidFromString(input.ToCStr().Get());
	w = GetenumIndex(input.ToCStr().Get());
	
		GS::HashTable<GS::UniString, GS::UniString> properties;
		GSErrCode err;
		err = ACAPI_Goodies(APIAny_GetPropertyAutoTextKeyTableID, &guid, &properties);
		if (err == NoError) {
			
			properties.Enumerate(
				[&err, &guid,&buf,&str,&buf2](const GS::UniString& propertyName, GS::UniString& propertyKey) -> void {
				GS::UniString value;
				err = ACAPI_Goodies(APIAny_InterpretAutoTextID, &propertyKey, &guid, &value);
				if (err == NoError) {
					buf.Clear();
					str = propertyName.ToCStr().Get();
					buf.Push(str);
					str = value.ToCStr().Get();
					buf.Push(str);
					str = propertyKey.ToCStr().Get();
					buf.Push(str);
				}
				else {
					ACAPI_WriteReport("Autotext for %s could not be resolved.", true, propertyName.ToCStr().Get());
				}
				buf2.Push(buf);
			});
		}
		else {
			ACAPI_WriteReport("Error while retrieving the property autotext key table.",true);
		}
	
	return buf2;
}

GS::Array<GS::Array<GS::UniString>> Do_GetProjInfo(GS::UniString param) {
	GS::UniString value;
	GS::Array<GS::UniString> buf;
	GS::Array<GS::Array<GS::UniString>> buf2;
	char** keys;
	char** values;

	GSErrCode err = ACAPI_Goodies(APIAny_GetAutoTextKeysID, &keys, &values);
	if (err == NoError) {
		Int32 count = BMGetPtrSize(reinterpret_cast<GSPtr> (keys)) / sizeof(char*);
		//char temp[254];
		for (Int32 i = 0; i < count; i++) {
			if (keys[i] == nullptr) {
				BMKillPtr(&values[i]);
				continue;
			}
			//DBPrintf("AutoText[%03d] \"%s\" = \"%s\"\n", i, keys[i], values[i]);
			
			buf.Clear();
			char temp[254];
			GS::UniString value;
			sprintf(temp, "AutoText[%03d]",i);
			value = temp;
			buf.Push(value);
			sprintf(temp, "%s", keys[i]);
			value = temp;
			buf.Push(value);
			sprintf(temp, "%s", values[i]);
			value = temp;
			buf.Push(value);
			buf2.Push(buf);
			
			BMKillPtr(&keys[i]);
			BMKillPtr(&values[i]);
		}
	}

	BMKillPtr(reinterpret_cast<GSPtr*>(&keys));
	BMKillPtr(reinterpret_cast<GSPtr*>(&values));
	return buf2;
}

static void __ACENV_CALL	Get3DComponentCallBack(const API_Get3DComponentType* info)
{
	if (info != nullptr && info->guid != APINULLGuid) {
		char msg[1024];
		sprintf(msg, "guid: %s\ndbElemIdx: %ld\nbodyIdx: %ld\nipgon: %ld\nc.x: %f\nc.y: %f\nc.z: %f\nnormal.x: %f\nnormal.y: %f\nnormal.z: %f\n",
			(((APIGuid2GSGuid(info->guid)).ToUniString()).ToCStr()).Get(),
			(GS::LongForStdio)info->dbElemIdx, (GS::LongForStdio)info->bodyIdx, (GS::LongForStdio)info->ipgon,
			info->c3.x, info->c3.y, info->c3.z,
			info->normal.x, info->normal.y, info->normal.z);
		ACAPI_WriteReport(msg, false);
	}
}		// Get3DComponentCallBack


GS::Array<GS::UniString> Do_GetPoint(GS::UniString param){
	GS::Array<GS::UniString> buf;
	if (param == "3D Point")
	{
		char prompt[254] = "click 3D Point";
		API_GetPointType	pointInfo = {};
	
		GSErrCode			err;

		pointInfo.enableQuickSelection = true;

		API_WindowInfo windowInfo;
		memset(&windowInfo, 0, sizeof(windowInfo));
		ACAPI_Database(APIDb_GetCurrentWindowID, &windowInfo, nullptr);

		CHTruncate(prompt, pointInfo.prompt, sizeof(pointInfo.prompt));
		pointInfo.changeFilter = false;
		pointInfo.changePlane = false;
		if (windowInfo.typeID == APIWind_3DModelID) {
			
			err = ACAPI_Interface(APIIo_GetPointID, &pointInfo,
				nullptr,
				(void*)(GS::IntPtr)&Get3DComponentCallBack);
		}
		if (err != NoError) {
			if (err != APIERR_CANCEL)
				ACAPI_WriteReport("Error in APIIo_GetPointID: %s", true, ErrID_To_Name(err));
			
		}

		GS::UniString str;
		str = GS::ValueToUniString(pointInfo.pos.x);
		buf.Push(str);
		str = GS::ValueToUniString(pointInfo.pos.y);
		buf.Push(str);
		str = GS::ValueToUniString(pointInfo.pos.z);
		buf.Push(str);
		return buf;
	}
	API_Coord c;
	
	ClickAPoint("Click selected point", &c);
	GS::UniString str;
	str = GS::ValueToUniString(c.x);
	buf.Push(str);
	str = GS::ValueToUniString(c.y);
	buf.Push(str);
	return buf;
	
}
GS::Array<GS::Array<GS::UniString>> Do_GetSelected() {

	GS::Array<GS::UniString> buf;
	GS::Array<GS::Array<GS::UniString>> buf2;
	GSErrCode            err;
	API_SelectionInfo selectionInfo;
	GS::Array<API_Neig> selNeigs;
	err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, true);
	BMKillHandle((GSHandle*)&selectionInfo.marquee.coords);
	if (err == APIERR_NOSEL)
		err = NoError;
	if (selectionInfo.typeID != API_SelEmpty) {
		
		for (const API_Neig& selNeig : selNeigs) {
			API_Guid guid;

			if (!ACAPI_Element_Filter(selNeig.guid, APIFilt_IsEditable))
				continue;
			GS::Guid GSGuid;
			
			GSGuid = APIGuid2GSGuid(selNeig.guid);
			guid = selNeig.guid;
			char fill[254];
			GSGuid.ConvertToString(fill);
			GS::UniString out(fill);
			GS::UniString val(out);
			buf.Clear();
			buf.Push(out);
			out = Do_GetValueFromGuid(val, GS::UniString("Layer"));
			buf.Push(out);
			out = Do_GetValueFromGuid(val, GS::UniString("Home Story Name"));
			buf.Push(out);
			out = Do_GetValueFromGuid(val, GS::UniString("Element ID"));
			buf.Push(out);
			out = Do_GetValueFromGuid(val, GS::UniString("Element Type"));
			buf.Push(out);
			buf2.Push(buf);
		}
		
	}
	if (buf2.IsEmpty()) {
		buf.Push(GS::UniString("Nothing Selected"));
		buf2.Push(buf);
	}
	return buf2;
}


GS::Array<GS::UniString>Do_SetProjInfo(GS::Array<GS::UniString> params) {

	GS::Array<GS::UniString> buf;
	GS::UniString dbkey = params[0];
	GS::UniString value =params[1];
	
	buf.Clear();
	GSErrCode err = ACAPI_Goodies(APIAny_SetAnAutoTextID, &dbkey, &value);
	if (err) {
		value = "Set Failed";
		buf.Push(value);
		return buf;
	}
	else {
		value = "Set Success";
		buf.Push(value);
		return buf;
	}


}

GS::Array<GS::UniString>Do_GetProjValue(GS::Array<GS::UniString> params) {
	GS::Array<GS::ArrayFB<GS::UniString, 3> > autotexts;
	API_AutotextType type = APIAutoText_Fixed;
	GS::UniString fail("Key Not Found");
	GS::Array<GS::UniString> buf;
	buf.Push(fail);
	GSErrCode err = ACAPI_Goodies(APIAny_GetAutoTextsID, &autotexts, (void*)type);
	if (err == NoError) {
		for (ULong i = 0; i < autotexts.GetSize(); i++) {
			if (params[0] == autotexts[i][1]) {
				buf.Clear();
				buf.Push(autotexts[i][0]);
				buf.Push(autotexts[i][1]);
				buf.Push(autotexts[i][2]);
				
			}
		}
	}
	return buf;
}

GS::UniString Do_GetValueFromGuid(GS::UniString gu, GS::UniString name) {
	//takes guid string and value name -- returns value
	API_Guid guid;
	GS::Guid gsguid;
	GS::UniString value("Failed");
	gsguid.ConvertFromString(gu);
	guid = GSGuid2APIGuid(gsguid);

	{
		GS::HashTable<GS::UniString, GS::UniString> properties{ GS::HashTable<GS::UniString, GS::UniString>() };
		GSErrCode err = ACAPI_Goodies(APIAny_GetPropertyAutoTextKeyTableID, &guid, &properties);
		if (err == NoError) {
			
			if (properties.ContainsKey(name)) {// changed here
				
				GS::UniString area = properties.Get(name); // changed from GS::UniString("Area")
				
				err = ACAPI_Goodies(APIAny_InterpretAutoTextID, &area, &guid, &value);
				if (err == NoError) {

				}
				else {
					ACAPI_WriteReport("Autotext for %s could not be resolved.", true, gu.ToCStr().Get());
					return GS::UniString("Failed");
				}
			}
			else {
				
				return GS::UniString("Failed");
			}
		}
		else {
			ACAPI_WriteReport("Error while retrieving the property autotext key table.", true);
			return GS::UniString("Failed");
		}
	}
	return value;
}

GS::Array<GS::Array<GS::UniString>> Do_GetElemList(GS::Array<GS::UniString> inParams) {
	GS::UniString str1("No Values");
	GS::Array<GS::UniString> buf;
	GS::Array<GS::Array<GS::UniString>> buf2;
	buf.Push(str1);
	GS::Array<API_Guid> guids = {};
	GS::Guid gsguid;
	API_ElemTypeID w;
	w = GetenumIndex(inParams[0].ToCStr().Get());
	GSErr err =    ACAPI_Element_GetElemList(w,&guids, APIFilt_OnActFloor);

	// Gets list of elements not available to Python such as text,label & equipment
	if (err == NoError) {
			
			for (API_Guid gu : guids) {
				buf.Clear();
				API_Element element = {};
				gsguid = APIGuid2GSGuid(gu);
				char temp[254];
				gsguid.ConvertToString(temp);
				str1 = temp;
				buf.Push(str1); // guid of element
				if (inParams[0] == GS::UniString("API_TextID") || inParams[0] == GS::UniString("API_LabelID")) {
					API_ElementMemo memo;
					element.header.guid = gu;
					err = ACAPI_Element_Get(&element);
					API_ElemType parentType = element.label.parentType;
					API_Guid parent = element.label.parent;
					
					gsguid = APIGuid2GSGuid(parent);
					gsguid.ConvertToString(temp);
					str1 = temp;
					buf.Push(str1); //parent guid
					GS::UniString tex;
					if (err == NoError && element.header.hasMemo) {
						err = ACAPI_Element_GetMemo(gu, &memo);
						tex = memo.textContent[0];
						buf.Push(tex); // textContent og text or label
						ACAPI_DisposeElemMemoHdls(&memo);
					}
					if (inParams[0] == GS::UniString("API_LabelID") ){
							GS::UniString result;
							GS::UniString valuename("Element ID");
							result = Do_GetValueFromGuid(str1, valuename);
							buf.Push(result);
							GS::UniString tex;
							if ( element.header.hasMemo) {
								err = ACAPI_Element_GetMemo(gu, &memo);
								tex = memo.textContent[0];
								
								ACAPI_DisposeElemMemoHdls(&memo);
								valuename = "Element Type";
								result = Do_GetValueFromGuid(str1, valuename);
								buf.Push(result);
								std::string stdstr = "abc";
								
								GS::UniString uniString(stdstr.c_str(), CC_UTF8);
								
								
							}
					}
				}
				buf2.Push(buf);
			}
			
	}
	return buf2;
}

static void	ReplaceEmptyTextWithPredefined(API_ElementMemo& memo)
{
	const char* predefinedContent = "Default text was empty.";

	if (memo.textContent == nullptr || Strlen32(*memo.textContent) < 2) {
		BMhKill(&memo.textContent);
		memo.textContent = BMhAllClear(Strlen32(predefinedContent) + 1);
		strcpy(*memo.textContent, predefinedContent);
		
	}
}

GS::UniString Do_SetLabelText(GS::UniString guid, GS::UniString type,GS::UniString key,GS::UniString fontsize) 
{
	//type reserved for future
	GS::UniString result("Success");
	API_Guid gu;
	API_Element mask;
	double fsize;
	UniStringToValue(fontsize, fsize);
	GS::Guid gsguid;
	gsguid.ConvertFromString(guid.ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	API_Element element;
	element.header.guid = gu;
	GSErrCode err;
	err = ACAPI_Element_Get(&element);
	if (type == "API_LabelID") {
		API_ElementMemo memo;
		err = ACAPI_Element_GetMemo(element.header.guid,&memo);
		if (memo.paragraphs != nullptr) {
			BMhKill((GSHandle*)&memo.paragraphs);
		}

			if (memo.textContent != nullptr ) {
				bool setAutoTextFlag = false;
				ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &setAutoTextFlag, nullptr);
				BMKillHandle(&memo.textContent);
				char input[254];
				strcpy(input, key.ToCStr().Get());
				const char* text = "<PROPERTY-AC5CCA52-F79B-4850-92A9-BED7CB7C3847>";
				//Area - Prop key (testing) -- not used --use argument key
				memo.textContent = BMhAll(Strlen32(input) + 1);
				strcpy(*memo.textContent,input);
				element.label.u.text.charCode = CC_UTF16;
				element.label.u.text.size = fsize;
				element.label.u.text.multiStyle = true;
				ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.charCode);
				ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.multiStyle);
			}
			err = ACAPI_CallUndoableCommand("Changing Label",
				[&]() -> GSErrCode {
				err = ACAPI_Element_Change(&element, &mask, &memo, APIMemoMask_TextContent, true);
				return err;
			});
		if (err) {
			ACAPI_WriteReport("Element Change Error - %s", true, ErrID_To_Name(err));
		}
		ACAPI_DisposeElemMemoHdls(&memo);
	}
	return result;
}

GS::UniString GetActStoryName()
{
	API_StoryInfo storyInfo = {};
	ACAPI_Environment(APIEnv_GetStorySettingsID, &storyInfo);

	const GS::UniString actStoryName((*storyInfo.data)[storyInfo.actStory - storyInfo.firstStory].uName);
	BMhKill(reinterpret_cast<GSHandle*> (&storyInfo.data));

	return actStoryName;
}


GS::UniString Do_GetAutoText(GS::UniString guid,GS::UniString status) {

	GS::UniString result("Success");

	// guid is API_Guid of label
	API_Guid gu;
	GS::Guid gsguid;
	gsguid.ConvertFromString(guid.ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	API_Element element;
	element.header.guid = gu;
	GSErrCode err;
	err = ACAPI_Element_Get(&element);

	bool currAutoTextFlag;
	ACAPI_Goodies(APIAny_GetAutoTextFlagID, &currAutoTextFlag, nullptr);
	bool setAutoTextFlag = false;
	ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &setAutoTextFlag, nullptr);

	API_ElementMemo memo;
	err = ACAPI_Element_GetMemo(element.header.guid, &memo);
	if (status == "TRUE") {
		setAutoTextFlag = true;
		ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &setAutoTextFlag, nullptr);
		err = ACAPI_Element_GetMemo(element.header.guid, &memo);
		result = *memo.textContent;
	}
	else if (status == "FALSE"){
		setAutoTextFlag = false;
		ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &setAutoTextFlag, nullptr);
		err = ACAPI_Element_GetMemo(element.header.guid, &memo);
		result = *memo.textContent;
	}
	else {
		result = "Flag not set";
	}
	return result;
}


GS::UniString Do_ChangeText(GS::UniString guid, GS::UniString type, GS::UniString key)
{
	//type reserved for future
	GS::UniString result("Success");
	API_Guid gu;
	API_Element mask;
	GS::Guid gsguid;
	gsguid.ConvertFromString(guid.ToCStr().Get());
	gu = GSGuid2APIGuid(gsguid);
	API_Element element;
	element.header.guid = gu;
	GSErrCode err;
	err = ACAPI_Element_Get(&element);
	if (type == "API_TextID") {
		API_ElementMemo memo;
		err = ACAPI_Element_GetMemo(element.header.guid, &memo);
		if (memo.paragraphs != nullptr) {
			BMhKill((GSHandle*)&memo.paragraphs);
		}

		if (memo.textContent != nullptr) {
			bool setAutoTextFlag = false;
			ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &setAutoTextFlag, nullptr);
			BMKillHandle(&memo.textContent);
			char input[254];
			strcpy(input, key.ToCStr().Get());
			const char* text = "<PROPERTY-AC5CCA52-F79B-4850-92A9-BED7CB7C3847>";
			//Area - revise!!!!!
			memo.textContent = BMhAll(Strlen32(text) + 1);
			strcpy(*memo.textContent, input);
			element.label.u.text.charCode = CC_UTF16;
			element.label.u.text.multiStyle = true;
			ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.charCode);
			ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.multiStyle);
		}
		err = ACAPI_CallUndoableCommand("Changing Label",
			[&]() -> GSErrCode {
			err = ACAPI_Element_Change(&element, &mask, &memo, APIMemoMask_TextContent, true);
			return err;
		});
		if (err) {
			ACAPI_WriteReport("Element Change Error - %s", true, ErrID_To_Name(err));
		}
		ACAPI_DisposeElemMemoHdls(&memo);
	}
	return result;
}



GS::Array<GS::UniString> Do_GetQuantity(GS::Array<GS::UniString>inParams) {
	GS::Array<GS::UniString> buf;
	GS::UniString output;
	API_Guid guid;
	GS::Guid gsguid;
	//ACAPI_WriteReport("ID - %s", true, inParams[0].ToCStr().Get());
	gsguid.ConvertFromString(inParams[0].ToCStr().Get());
	guid = GSGuid2APIGuid(gsguid);
	API_ElementQuantity quantity;
	API_Quantities      quantities;
	API_QuantitiesMask  mask;
	API_QuantityPar   params;
	char              s[256] = { '\0' };
	GSErrCode         err;
	
	ACAPI_ELEMENT_QUANTITY_MASK_CLEAR(mask);
	int index;
	err = GetQuantityMask(inParams[1], inParams[2],mask,index);

	quantities.elements = &quantity;
	params.minOpeningSize = EPS;
	err = ACAPI_Element_GetQuantities(guid, &params, &quantities, &mask);
	if (!err) {
		double* db;
		API_WallQuantity* wallptr;
		API_SlabQuantity* slabptr;
		API_DoorQuantity* doorptr;
		API_WindowQuantity* windowptr;
		if (inParams[1] == "API_WallID") {
			wallptr = &quantity.wall;
			db = reinterpret_cast<double*>(wallptr);
		}
		else if (inParams[1] == "API_SlabID") {
			slabptr = &quantity.slab;
			db = reinterpret_cast<double*>(slabptr);
		}
		else if (inParams[1] == "API_DoorID") {
			doorptr = &quantity.door;
			db = reinterpret_cast<double*>(doorptr);
		}
		else if (inParams[1] == "API_WindowID") {
			windowptr = &quantity.window;
			db = reinterpret_cast<double*>(windowptr);
		}
		else {
			output = "Invalid Type";
			buf.Push(output);
			return buf;
		}
		db += index;
		if (index == 16  && inParams[1] == "API_Wall") {
			int* intptr;
			intptr = reinterpret_cast<int*>(db);
			sprintf(s, "%s: %ld", inParams[2].ToCStr().Get(), *intptr);
		}
		else
			sprintf(s, "%s: %.2lf", inParams[2].ToCStr().Get(),*db);
		
		output = s;
		buf.Push(output);
	}
	else
		ACAPI_WriteReport("quanity Err %s", true, ErrID_To_Name(err));


	return buf;

}


void	Do_CreateText(GS::UniString text, GS::UniString fontsize,API_Coord c)
{

	GSErrCode			err;
	API_Element			element;
	API_ElementMemo		memo;
	GS::UniString text2 = "\u00B2 Text";
	BNZeroMemory(&element, sizeof(API_Element));
	element.header.type = API_TextID;

	err = ACAPI_Element_GetDefaults(&element, &memo);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetDefaults", err);
		return;
	}
	API_Attribute attrib;
	BNZeroMemory(&attrib, sizeof(API_Attribute));
	attrib.header.typeID = API_FontID;
	strcpy(attrib.header.name, "Segoe Script");

	err = ACAPI_Attribute_Search(&attrib.header);
	if (err)
		ACAPI_WriteReport("Failed", true);
	else {
		
	}
	std::string str(fontsize.ToCStr(CC_UTF8));
	double num;
	num = std::stod(str);
	element.text.loc.x = c.x;
	//element.text.font = 387;
	element.text.loc.y = c.y;
	element.text.multiStyle = false;
	element.text.size = num;
	element.text.usedFill = true;
	element.text.fillPen = 66;//yellow
	element.text.pen = 5; //red
	
	element.text.font = attrib.header.index;
	element.text.charCode = CP_UTF8;

	
	if (memo.textContent == nullptr || Strlen32(*memo.textContent) < 2) {
		BMhKill(&memo.textContent);
		memo.textContent = BMhAllClear(text.GetLength() +1);
		
		strcpy(*memo.textContent,text.ToCStr().Get());
		
	}

	err = ACAPI_CallUndoableCommand("Changing Label",
		[&]() -> GSErrCode {
		err = ACAPI_Element_Create(&element, &memo);
		return err;
	});
	if (err != NoError)
		ACAPI_WriteReport("ACAPI_Element_Create (Text) %s",true, ErrID_To_Name(err));

	ACAPI_DisposeElemMemoHdls(&memo);
}		// Do_CreateText

GS::UniString GetWallRef(GS::UniString value,GS::UniString objType) {
	GS::UniString buf;
	char ch[254];
	API_Guid gu;
	strcpy(ch, value.ToCStr().Get());
	gu = APIGuidFromString(value.ToCStr().Get());

	API_Element element;
	BNZeroMemory(&element, sizeof(API_Element));
	element.header.guid = gu;
	GSErrCode err;
	err = ACAPI_Element_Get(&element);
	if (err) {
		ACAPI_WriteReport("Element Not Found", true);
		return buf;
	}
	else {
		if (objType == "API_WindowID)")
			gu = element.window.owner;
		else
			gu = element.door.owner;
		buf = APIGuidToString(gu);
		strcpy(ch, buf.ToCStr().Get());
		return buf;
	}
}
GS::UniString ChangeCatCode(GS::UniString value, GS::UniString code) {
	GS::UniString buf("No Attribute Found");
	API_Attr_Head attrHead = {};
	strcpy(attrHead.name, code.ToCStr().Get());
	attrHead.typeID = API_ZoneCatID;

	if (ACAPI_Attribute_Search(&attrHead) != NoError) {
		
		return buf;
	}
	else {
		
	}

	GS::Array<API_Neig>	selNeigs;
	GSErrCode err;
	
	API_Element element = {}, mask;
	API_Guid gu;
	gu = APIGuidFromString(value.ToCStr().Get());
	element.header.guid = gu;
	
	element.zone.catInd = attrHead.index;
	ACAPI_ELEMENT_MASK_CLEAR(mask);
	ACAPI_ELEMENT_MASK_SET(mask, API_ZoneType, catInd);
	
	err = ACAPI_CallUndoableCommand("Changing Category Code",
		[&]() -> GSErrCode {
		err = ACAPI_Element_Change(&element, &mask, nullptr, 0, true);
		if (err){
			ACAPI_WriteReport("Error-- % s", true, ErrID_To_Name(err));
			buf = "No Success";
		}
		else {
			buf = "Success";
		}
		return err;
	});
	return buf;
}
GS::Array<GS::Array<GS::UniString>> GetBMAtt(GS::UniString value) {
	GS::Array<GS::Array<GS::UniString>> arrBuf;
	GS::Array<GS::UniString> strBuf;
	GS::UniString item = "Attribute Failure";
	API_Attr_Head attrHead = {};
	strcpy(attrHead.name, value.ToCStr().Get());
	attrHead.typeID = API_BuildingMaterialID;

	if (ACAPI_Attribute_Search(&attrHead) != NoError) {
		return arrBuf;
	}
	else {
	}
	API_Attribute attr = {};
	attr.header = attrHead;
	GSErrCode error = ACAPI_Attribute_Get(&attr);
	if (error) {
		strBuf.Push(item);
		arrBuf.Push(strBuf);
		return arrBuf;
	}
	else {
			API_Attr_Head  attributeHeader;
			BNZeroMemory(&attributeHeader, sizeof(API_Attr_Head));
			attributeHeader.typeID = attrHead.typeID;
			API_PropertyDefinitionFilter def = API_PropertyDefinitionFilter_UserDefined;
			attributeHeader.index = attrHead.index;
			GS::Array<API_PropertyDefinition> definitions ;
			GS::Array<GS::UniString> names;
			error = ACAPI_Attribute_GetPropertyDefinitions(attributeHeader, def,definitions);
			API_Property property;
			GS::UniString result;
			if (error == NoError) {
				for (UInt32 i = 0; i < definitions.GetSize(); i++) {
					ACAPI_Attribute_GetPropertyValue(attrHead, definitions[i].guid, property);
					if (property.value.singleVariant.variant.type == API_PropertyStringValueType) {
						strBuf.Clear();
						item = property.value.singleVariant.variant.uniStringValue;
						strBuf.Push(definitions[i].name);
						strBuf.Push(item);
						arrBuf.Push(strBuf);
					}
					if (property.value.singleVariant.variant.type == API_PropertyRealValueType) {
						strBuf.Clear();
						double dbl = property.value.singleVariant.variant.doubleValue;
						strBuf.Push(definitions[i].name);
						strBuf.Push(GS::UniString::Printf("%f", property.value.singleVariant.variant.doubleValue));
						arrBuf.Push(strBuf);
					}
					if (property.value.singleVariant.variant.type == API_PropertyIntegerValueType) {
						strBuf.Clear();
						int integer = property.value.singleVariant.variant.intValue;
						strBuf.Push(definitions[i].name);
						strBuf.Push(GS::UniString::Printf("%d", integer));
						arrBuf.Push(strBuf);
					}
					if (property.value.singleVariant.variant.type == API_PropertyBooleanValueType) {
						strBuf.Clear();
						bool status = property.value.singleVariant.variant.boolValue;
						strBuf.Push(definitions[i].name);
						strBuf.Push(GS::UniString::Printf("%d", status));
						arrBuf.Push(strBuf);
					}
				}
			}
		strBuf.Clear();
		item ="Thermal Conductivity";
		strBuf.Push(item);
		strBuf.Push(GS::UniString::Printf("%f", attr.buildingMaterial.thermalConductivity));
		arrBuf.Push(strBuf);
		strBuf.Clear();
		item = "Density";
		strBuf.Push(item);
		strBuf.Push(GS::UniString::Printf("%f", attr.buildingMaterial.density));
		arrBuf.Push(strBuf);
		strBuf.Clear();
		item = "Heat Capacity";
		strBuf.Push(item);
		strBuf.Push(GS::UniString::Printf("%f", attr.buildingMaterial.heatCapacity));
		arrBuf.Push(strBuf);
		strBuf.Clear();
		item = "EmbodiedEnergy";
		strBuf.Push(item);
		strBuf.Push(GS::UniString::Printf("%f", attr.buildingMaterial.embodiedEnergy));
		arrBuf.Push(strBuf);
		strBuf.Clear();
		item = "Embodied Carbon";
		strBuf.Push(item);
		strBuf.Push(GS::UniString::Printf("%f", attr.buildingMaterial.embodiedCarbon));
		arrBuf.Push(strBuf);
	}
	return arrBuf;
}

GS::UniString SetMSG(GS::UniString str) {
	GS::UniString ret("Success");
	//ACAPI_WriteReport("capacity - %d", true, str.GetCapacity());
	ACAPI_WriteReport( str.ToCStr().Get(), true);
	
	return ret;
}
GS::Array<GS::Array<GS::UniString>> Do_GetBMProperties(GS::UniString inGuid,GS::UniString objType) {
	GSErrCode			err;
	GS::Array<GS::UniString> buf;
	GS::UniString str;
	API_Guid gu;
	gu = APIGuidFromString(inGuid.ToCStr().Get());
	GS::Array<GS::Array<GS::UniString>> ret;

	GS::Array <API_CompositeQuantity>			composites;
	GS::Array <API_ElemPartQuantity>			elemPartQuantities;
	GS::Array <API_ElemPartCompositeQuantity>	elemPartComposites;
	API_ElementQuantity	quantity;
	API_QuantityPar		params;
	GS::Array<API_Quantities>	quantities;
	GS::Array<API_Guid>			elemGuids;
	
	API_ElemType type;
	if (objType == "API_WallID")
		type.typeID = API_WallID;
	else if (objType == "API_SlabID")
		type.typeID = API_SlabID;
	else if (objType == "API_RoofID")
		type.typeID = API_RoofID;
	else {
		str = "Type Not Supported";
		buf.Clear();
		ret.Clear();
		buf.Push(str);
		ret.Push(buf);
		return ret;
	}

	API_Element element;
	BNZeroMemory(&element, sizeof element);
	element.header.type = type;
	element.header.guid = gu;
	

	err = ACAPI_Element_Get(&element);
	if (err != NoError) {
		ACAPI_WriteReport("Unknown element",true);
	}
	elemGuids.Push(element.header.guid);

	BNZeroMemory(&params, sizeof(API_QuantityPar));
	params.minOpeningSize = EPS;

	quantities.Push(API_Quantities());
	quantities[0].elements = &quantity;
	quantities[0].composites = &composites;
	quantities[0].elemPartQuantities = &elemPartQuantities;
	
	quantities.Push(quantities[0]);
	

	API_QuantitiesMask mask;
	ACAPI_ELEMENT_QUANTITIES_MASK_SETFULL(mask);
	GS::Array <API_ElemPartSurfaceQuantity>		elemPartSurfaces;
	
	GS::Array<API_Neig>		selNeigs;
	GS::Array<API_Guid>		coverElemGuids;
	API_SelectionInfo		selectionInfo;
	err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, false);
	for (const API_Neig& neig : selNeigs) {
		coverElemGuids.Push(neig.guid);
	}
	err = ACAPI_Element_GetSurfaceQuantities(&elemGuids, &coverElemGuids, &elemPartSurfaces);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetSurfaceQuantities", err);
	}
	int index = elemPartSurfaces.GetSize();
	
	API_Attribute attribute;
	BNZeroMemory(&attribute, sizeof(API_Attribute));
	buf.Clear();
	ACAPI_WriteReport("composite size %d", true, elemPartSurfaces.GetSize());
	for (UInt32 i = 0; i < elemPartSurfaces.GetSize(); i++) {

		attribute.header.index = elemPartSurfaces[i].buildMatIdx;
		attribute.header.typeID = API_BuildingMaterialID;
		ACAPI_Attribute_Get(&attribute);

		str = attribute.header.name;
		buf.Push(str);
	}
	ret.Push(buf);
	return ret;
}
