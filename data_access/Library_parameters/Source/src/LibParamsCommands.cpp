#include	"LibParamsCommands.hpp"
#include	"ObjectState.hpp"
#include	"FileSystem.hpp"
#include	"OnExit.hpp"
#include    "APICommon.h"

#include    "string.h"
#include    "StringConversion.hpp"
#include    "UniString.hpp"
#include    "GetParams.hpp"


GS::Array<GS::Array<GS::UniString>> arr;


constexpr const char* AdditionalJSONCommandsNamespace = "LibParamsCommands";


static GS::HashTable<GS::UniString, API_Guid> GetPublisherSetNameGuidTable ()
{
	GS::HashTable<GS::UniString, API_Guid> table;

	Int32 numberOfPublisherSets = 0;
	ACAPI_Navigator (APINavigator_GetNavigatorSetNumID, &numberOfPublisherSets);

	API_NavigatorSet set = {};
	for (Int32 ii = 0; ii < numberOfPublisherSets; ++ii) {
		set.mapId = API_PublisherSets;
		if (ACAPI_Navigator (APINavigator_GetNavigatorSetID, &set, &ii) == NoError) {
			table.Add (set.name, set.rootGuid);
		}
	}

	return table;
}


static GS::Optional<IO::Location>	GetApplicationLocation ()
{
	IO::Location applicationFileLocation;

	GSErrCode error = IO::fileSystem.GetSpecialLocation (IO::FileSystem::ApplicationFile, &applicationFileLocation);
	if (error != NoError) {
		return GS::NoValue;
	}

	return applicationFileLocation;
}


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


// --- LibCommand ----------------------------------------------------------------------------------

GS::String LibCommand::GetName() const
{
	return "GetParams";
}

/*
GS::String LibCommand::GetNamespace() const
{
	return "AdditionalJSONCommands";
}
*/

GS::Optional<GS::UniString> LibCommand::GetSchemaDefinitions() const
{
	GS::UniString resourceData;
	GSHandle data = RSLoadResource('DATA', ACAPI_GetOwnResModule(), 1000);
	GSSize handleSize = BMhGetSize(data);
	if (data != nullptr) {
		resourceData.Append(*data, handleSize);
		BMhKill(&data);
	}

	return resourceData;

}



GS::Optional<GS::UniString> LibCommand::GetInputParametersSchema() const
{
	return GS::UniString(R"({
			"$ref": "#/MyInputParameters"
	})");
}


GS::Optional<GS::UniString> LibCommand::GetResponseSchema() const
{
	return GS::UniString(R"({
			"$ref": "#/MyOutputParameters"
			},
			"error": {
				"$ref": "APITypes.json#/definitions/Error"
			})");

}


GS::ObjectState	LibCommand::Execute(const GS::ObjectState& parameters, GS::ProcessControl& /*processControl*/) const
{
	GS::UniString inCommand;
	GS::Array<GS::UniString> inParams;
	//API_Guid guid;
	GS::ObjectState response;
	GS::Array<GS::UniString> buf;
	GSErrCode err;
	GS::UniString status;

	parameters.Get("command", inCommand);
	parameters.Get("inParams", inParams);
	
	if (inCommand == "LibPart") {
		arr.Clear();
		arr = GetParams(inParams);
		inParams.Clear();
			inCommand = "";
	}
	else if (inCommand == "ChangeLibPart") {
		arr.Clear();
		err = ChangeParams(inParams);
			if (err) {
				status = ErrID_To_Name(err);
				buf.Push(status);
				inParams.Clear();
				inCommand = "";
				response.Add("outparams", buf); // return error message
				return response;
			}

	}
	else if (inCommand == "CheckLibPart") {
		char* toptype = "2E9E2CF0-3B67-4D99-841D-FE78BBC31295";// # Duct Fitting
		char subtype[254] = "ADE2B376-F688-4E48-BC3F-BC7BAB38A27B";// # Duct Bend Fitting
		char fitting[254] = "B1792C20-764A-4C69-B2A5-F3C181636CD2";//Bend Unique ID

		GS::UniString successorUnID = GS::UniString::Printf("{%T}", inParams[0].ToPrintf());
		GS::UniString predecessorUnID = GS::UniString::Printf("{%T}", inParams[1].ToPrintf());
		//GS::UniString successorUnID = GS::UniString::Printf("{%T}", inParams[0].ToPrintf());
		//GS::UniString predecessorUnID = GS::UniString::Printf("{%T}", inParams[1].ToPrintf());

		GSErrCode err = ACAPI_Goodies(APIAny_CheckLibPartSubtypeOfID,
			(void*)successorUnID.ToCStr().Get(), (void*)predecessorUnID.ToCStr().Get());

		//GSErrCode err = ACAPI_Goodies(APIAny_CheckLibPartSubtypeOfID, fitting, toptype);
		//ACAPI_WriteReport("params  %s %s", true, (char*)inParams[0].ToCStr().Get(), (char*)inParams[1].ToCStr().Get());
		if (err) {
			status = ErrID_To_Name(err);
			buf.Clear();
			buf.Push(status);
			inParams.Clear();
			inCommand = "";
			response.Clear();
			response.Add("outparams", buf); // return error message
			return response;
		}
		else {
			status = "Success";
			buf.Push(status);
			inParams.Clear();
			inCommand = "";
			response.Add("outparams", buf); // return error message
			return response;

		}

	}
	else {
			arr.Clear();
			buf.Clear();
			status = "Dialog Not Found";
			buf.Push(status);
			arr.Push(buf);
	}
	
	
	response.Add("outparams2", arr); // Load info
	return response;
}

