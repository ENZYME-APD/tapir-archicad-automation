#pragma once

#include "CommandBase.hpp"

class GetAttributesByTypeCommand : public CommandBase
{
public:
    GetAttributesByTypeCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetBuildingMaterialPhysicalPropertiesCommand : public CommandBase
{
public:
    GetBuildingMaterialPhysicalPropertiesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetLayerCombinationsCommand : public CommandBase
{
public:
    GetLayerCombinationsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateAttributesCommandBase : public CommandBase
{
public:
    CreateAttributesCommandBase (const GS::String& commandName, API_AttrTypeID attrTypeID, const GS::String& arrayFieldName);
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
protected:
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const = 0;
protected:
    GS::String     commandName;
    API_AttrTypeID attrTypeID;
    GS::String     arrayFieldName;
};

class CreateBuildingMaterialsCommand : public CreateAttributesCommandBase
{
public:
    CreateBuildingMaterialsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateLayersCommand : public CreateAttributesCommandBase
{
public:
    CreateLayersCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateLayerCombinationsCommand : public CreateAttributesCommandBase
{
public:
    CreateLayerCombinationsCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateSurfacesCommand : public CreateAttributesCommandBase
{
public:
    CreateSurfacesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateCompositesCommand : public CommandBase
{
public:
    CreateCompositesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};
