#pragma once

#include "CommandBase.hpp"

class GetBuildingMaterialPhysicalPropertiesCommand : public CommandBase
{
public:
    GetBuildingMaterialPhysicalPropertiesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateSimpleAttributesCommandBase : public CommandBase
{
public:
    CreateSimpleAttributesCommandBase (const GS::String& commandName, API_AttrTypeID attrTypeID, const GS::String& arrayFieldName);
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
protected:
    virtual GS::UniString GetTypeSpecificParametersSchema () const = 0;
    virtual void SetTypeSpecificParameters (API_Attribute& attribute, const GS::ObjectState& parameters) const = 0;
protected:
    GS::String     commandName;
    API_AttrTypeID attrTypeID;
    GS::String     arrayFieldName;
};

class CreateBuildingMaterialsCommand : public CreateSimpleAttributesCommandBase
{
public:
    CreateBuildingMaterialsCommand ();
protected:
    virtual GS::UniString GetTypeSpecificParametersSchema () const override;
    virtual void SetTypeSpecificParameters (API_Attribute& attribute, const GS::ObjectState& parameters) const override;
};

class CreateLayersCommand : public CreateSimpleAttributesCommandBase
{
public:
    CreateLayersCommand ();
protected:
    virtual GS::UniString GetTypeSpecificParametersSchema () const override;
    virtual void SetTypeSpecificParameters (API_Attribute& attribute, const GS::ObjectState& parameters) const override;
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
