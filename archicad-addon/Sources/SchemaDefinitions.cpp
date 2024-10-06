#include "SchemaDefinitions.hpp"
#include "APIEnvir.h"
#include "ACAPinc.h"
#include "RS.hpp"
#include "ResourceIDs.hpp"

static GS::UniString GetResourceFileContent (short resId)
{
    GSHandle gsHandle = RSLoadResource ('FILE', ACAPI_GetOwnResModule(), resId);
    GS::UniString fileContent (*gsHandle, BMGetHandleSize (gsHandle));
    BMKillHandle (&gsHandle);
    return fileContent;
}

const GS::UniString& GetCommonSchemaDefinitions ()
{
    static const GS::UniString commonSchemaDefinitions = GetResourceFileContent (ID_COMMON_SCHEMA_DEFINITIONS_FILE);
    return commonSchemaDefinitions;
}