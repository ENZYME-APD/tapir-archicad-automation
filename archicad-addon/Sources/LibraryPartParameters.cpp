#include "LibraryPartParameters.hpp"
#include "MigrationHelper.hpp"

bool IsNumericParam (API_AddParID t)
{
    switch (t) {
        case APIParT_Integer:
        case APIParT_Length:
        case APIParT_Angle:
        case APIParT_RealNum:
        case APIParT_LightSw:
        case APIParT_ColRGB:
        case APIParT_Intens:
        case APIParT_LineTyp:
        case APIParT_Mater:
        case APIParT_FillPat:
        case APIParT_PenCol:
        case APIParT_Boolean:
        case APIParT_BuildingMaterial:
        case APIParT_Profile:
            return true;
        default:
            return false;
    }
}

API_AddParType* FindParameter (API_AddParType** params, const char* name)
{
    if (params == nullptr) {
        return nullptr;
    }
    const GSSize n = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < n; ++ii) {
        API_AddParType& p = (*params)[ii];
        if (p.typeMod != API_ParArray && IsNumericParam (p.typeID) && CHEqualCStringsInsensitive (p.name, name)) {
            return &p;
        }
    }
    return nullptr;
}

bool GetNumericParameter (API_AddParType** params, const char* name, double& value)
{
    const API_AddParType* p = FindParameter (params, name);
    if (p == nullptr) {
        return false;
    }
    value = p->value.real;
    return true;
}

bool SetNumericParameter (API_AddParType** params, const char* name, double value)
{
    API_AddParType* p = FindParameter (params, name);
    if (p == nullptr) {
        return false;
    }
    p->value.real = value;
    return true;
}

GSErrCode GetLibraryPartOwnSize (Int32 libInd, double& a, double& b)
{
    double libraryPartA = 0.0;
    double libraryPartB = 0.0;
    Int32 addParNum = 0;
    API_AddParType** addPars = nullptr;
    const GSErrCode err = ACAPI_LibraryPart_GetParams (libInd, &libraryPartA, &libraryPartB, &addParNum, &addPars);
    if (addPars != nullptr) {
        ACAPI_DisposeAddParHdl (&addPars);
    }
    if (err != NoError) {
        return err;
    }
    if (libraryPartA > 1e-6) {
        a = libraryPartA;
    }
    if (libraryPartB > 1e-6) {
        b = libraryPartB;
    }
    return NoError;
}

bool ApplyParameter (API_AddParType** params, const GS::ObjectState& p, GS::Array<GS::UniString>& warnings)
{
    GS::UniString name;
    if (!p.Get ("name", name) || params == nullptr) {
        warnings.Push ("parameter without a name was ignored");
        return false;
    }
    const GSSize n = BMGetHandleSize ((GSHandle) params) / sizeof (API_AddParType);
    for (GSIndex ii = 0; ii < n; ++ii) {
        API_AddParType& actParam = (*params)[ii];
        if (!CHEqualCStringsInsensitive (actParam.name, name.ToCStr ())) {
            continue;
        }
        if (actParam.typeMod == API_ParArray) {
            warnings.Push ("array parameter '" + name + "' is not supported");
            return false;
        }
        if (IsNumericParam (actParam.typeID)) {
            double d = 0.0;
            bool b = false;
            if (p.Get ("value", d)) {
                actParam.value.real = d;
            } else if (p.Get ("value", b)) {
                actParam.value.real = b ? 1.0 : 0.0;
            } else {
                warnings.Push ("parameter '" + name + "' needs a number or boolean value");
                return false;
            }
            return true;
        }
        if (actParam.typeID == APIParT_CString || actParam.typeID == APIParT_Title) {
            GS::UniString s;
            if (!p.Get ("value", s)) {
                warnings.Push ("parameter '" + name + "' needs a string value");
                return false;
            }
            GS::ucscpy (actParam.value.uStr, s.ToUStr (0, GS::Min (s.GetLength (), (USize) API_UAddParStrLen - 1)).Get ());
            return true;
        }
        warnings.Push ("parameter '" + name + "' has an unsupported type");
        return false;
    }
    warnings.Push ("unknown parameter '" + name + "'");
    return false;
}

GSErrCode LoadLibraryPartDefaults (API_Element& element, API_ElementMemo& memo)
{
    double a = 0.0;
    double b = 0.0;
    Int32 addParNum = 0;
    API_AddParType** addPars = nullptr;
    const GSErrCode err = ACAPI_LibraryPart_GetParams (element.object.libInd, &a, &b, &addParNum, &addPars);
    if (err != NoError) {
        if (addPars != nullptr) {
            ACAPI_DisposeAddParHdl (&addPars);
        }
        return err;
    }
    if (memo.params != nullptr) {
        ACAPI_DisposeAddParHdl (&memo.params);
    }
    memo.params = addPars;

    // xRatio/yRatio hold the size itself only with useXYFixSize on, otherwise the size divided by
    // the library part's own A/B, so the part's own size is a and b, or a ratio of 1.
    if (a > 1e-6) {
        element.object.xRatio = element.object.useXYFixSize ? a : 1.0;
        SetNumericParameter (memo.params, "A", a);
    }
    if (b > 1e-6) {
        element.object.yRatio = element.object.useXYFixSize ? b : 1.0;
        SetNumericParameter (memo.params, "B", b);
    }
    return NoError;
}
