#pragma once

#include "CommandBase.hpp"

#ifdef ServerMainVers_2800
// Declared in GDLParamInterface.hpp, included only where the parameters are used, as the
// order of the GDL headers and ACAPinc.h matters (see ExtendedElementCommands.cpp).
namespace GDL { class Parameters; }
#endif

class GetGDLParametersOfElementsCommand : public CommandBase
{
public:
    GetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class SetGDLParametersOfElementsCommand : public CommandBase
{
public:
    SetGDLParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;

private:
    struct ArrayParameterChange {
        GS::String name;
        API_AddParID typeID = APIParT_Integer;
        Int32 dim1 = 0;
        Int32 dim2 = 0;
        GS::Array<double> numberValues;
        GS::Array<GS::UniString> stringValues;
    };

    static GSErrCode SetOneGDLParameter (
        const GS::ObjectState& parameter,
        const API_Guid& elemGuid,
        const API_GetParamsType& getParams,
        const GS::HashTable<GS::String, API_AddParID>& gdlParametersTypeDictionary,
        const GS::HashTable<short, GS::String>& gdlParametersIndexNameDictionary,
        const GS::Optional<double>& xRatioPerA,
        const GS::Optional<double>& yRatioPerB,
        GS::Array<ArrayParameterChange>& pendingArrayChanges,
        GS::UniString& errMessage);

    static bool ParseArrayParameterValue (
        const GS::ObjectState& parameter,
        API_AddParID typeID,
        ArrayParameterChange& change);

    static GSErrCode ApplyArrayParameterChanges (
        API_GetParamsType& getParams,
        const GS::Array<ArrayParameterChange>& changes,
        const API_Guid& elemGuid,
        GS::UniString& errMessage);
};

// The marker (the stamp drawn next to a window or a door on the floor plan) is a Main
// Marker sub-element with a library part and GDL parameters of its own, separate from
// the opening's. Get/SetGDLParametersOfElements reach the opening only, so these two
// commands read and write the marker's parameters.
class GetMarkerParametersOfElementsCommand : public CommandBase
{
public:
    GetMarkerParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// Adds "libPart" (when the library part can be read) and "parameters" to `os` from a GDL
// parameter list held in a memo, without a parameter interface: how
// GetMarkerParametersOfElements reports a marker and GetFavoriteSettings a Favorite.
void AddLibPartAndGDLParameters (GS::ObjectState& os, Int32 libInd, API_AddParType** params);

class SetMarkerParametersOfElementsCommand : public CommandBase
{
public:
    SetMarkerParametersOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// Switches placed library part based elements (e.g. ones whose library part is missing) to
// another library part, keeping the elements themselves.
class ChangeLibraryPartOfElementsCommand : public CommandBase
{
public:
    ChangeLibraryPartOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class RunParameterScriptOfElementsCommand : public CommandBase
{
public:
    RunParameterScriptOfElementsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// The JSON schemas of a parameter list GDLParametersToObjectStates gives and of the changes
// SetGDLParametersFromObjectStates takes.
extern const char* const GDLParameterValuesSchema;
extern const char* const SetGDLParameterValuesSchema;

#ifdef ServerMainVers_2800
// The parameters of a GDL::Parameters list (such as an opening symbol's), in the format
// GetGDLParametersOfElements gives them: name, type, value and, for an array, dimension1 and
// dimension2. Separators, titles and dictionaries have no value read this way, so they are
// left out.
GS::Array<GS::ObjectState> GDLParametersToObjectStates (const GDL::Parameters& gdlParameters);

// Changes the parameters named in `parameters`, given in the format SetGDLParametersOfElements
// takes (name, value, index1, index2), in `gdlParameters`. Returns the error of the first one
// that cannot be set, with the ones before it already changed, so the caller works on a copy
// and only writes it back when this succeeds.
GSErrCode SetGDLParametersFromObjectStates (GDL::Parameters& gdlParameters, const GS::Array<GS::ObjectState>& parameters, GS::UniString& errMessage);
#endif
