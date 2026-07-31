#pragma once

#include "CommandBase.hpp"

class CreateElementsCommandBase : public CommandBase
{
public:
    CreateElementsCommandBase (const GS::String& commandName, API_ElemTypeID elemTypeID, const GS::String& arrayFieldName);
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
protected:
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const = 0;
protected:
    GS::String     commandName;
    API_ElemTypeID elemTypeID;
    GS::String     arrayFieldName;
};

class CreateColumnsCommand : public CreateElementsCommandBase
{
public:
    CreateColumnsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateSlabsCommand : public CreateElementsCommandBase
{
public:
    CreateSlabsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateZonesCommand : public CreateElementsCommandBase
{
public:
    CreateZonesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreatePolylinesCommand : public CreateElementsCommandBase
{
public:
    CreatePolylinesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateLineElementsCommand : public CreateElementsCommandBase
{
public:
    CreateLineElementsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateArcsCommand : public CreateElementsCommandBase
{
public:
    CreateArcsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateCirclesCommand : public CreateElementsCommandBase
{
public:
    CreateCirclesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateHotspotsCommand : public CreateElementsCommandBase
{
public:
    CreateHotspotsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateHatchesCommand : public CreateElementsCommandBase
{
public:
    CreateHatchesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateSplinesCommand : public CreateElementsCommandBase
{
public:
    CreateSplinesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateObjectsCommand : public CreateElementsCommandBase
{
public:
    CreateObjectsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateLampsCommand : public CreateElementsCommandBase
{
public:
    CreateLampsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateMeshesCommand : public CreateElementsCommandBase
{
public:
    CreateMeshesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateLabelsCommand : public CreateElementsCommandBase
{
public:
    CreateLabelsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class CreateTextsCommand : public CreateElementsCommandBase
{
public:
    CreateTextsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::ObjectState> SetTypeSpecificParameters (API_Element& element, API_ElementMemo& memo, const Stories& stories, const GS::ObjectState& parameters) const override;
};

class ModifyObjectsCommand : public CommandBase
{
public:
    ModifyObjectsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class ModifyLampsCommand : public CommandBase
{
public:
    ModifyLampsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};