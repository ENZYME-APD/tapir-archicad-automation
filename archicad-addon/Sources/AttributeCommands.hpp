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

class GetLinesCommand : public CommandBase
{
public:
    GetLinesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetFillsCommand : public CommandBase
{
public:
    GetFillsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetZoneCategoriesCommand : public CommandBase
{
public:
    GetZoneCategoriesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetMEPSystemsCommand : public CommandBase
{
public:
    GetMEPSystemsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// Before AC27, the pen array element type is API_PenType (not API_Pen) - see CreatePenTablesCommand. Execute ()
// has two implementations (see AttributeCommands.cpp) to cover both shapes.
class GetPenTablesCommand : public CommandBase
{
public:
    GetPenTablesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetProfilesCommand : public CommandBase
{
public:
    GetProfilesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetCompositesCommand : public CommandBase
{
public:
    GetCompositesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetSurfacesCommand : public CommandBase
{
public:
    GetSurfacesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetLayersCommand : public CommandBase
{
public:
    GetLayersCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class GetBuildingMaterialsCommand : public CommandBase
{
public:
    GetBuildingMaterialsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class DeleteAttributesCommand : public CommandBase
{
public:
    DeleteAttributesCommand ();
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

class CreateLinesCommand : public CreateAttributesCommandBase
{
public:
    CreateLinesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateFillsCommand : public CommandBase
{
public:
    CreateFillsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateZoneCategoriesCommand : public CreateAttributesCommandBase
{
public:
    CreateZoneCategoriesCommand ();
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual void SetTypeSpecificParameters (const GS::ObjectState& parameters, API_Attribute& attribute, API_AttributeDef& attributeDef) const override;
};

class CreateMEPSystemsCommand : public CreateAttributesCommandBase
{
public:
    CreateMEPSystemsCommand ();
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

// Before AC27, API_Pen was named API_PenType with a different shape (index nested in an API_Attr_Head, char
// description instead of GS::uchar_t, penTable_Items is an old-style handle array instead of a GS::Array).
// Execute () has two implementations (see AttributeCommands.cpp) to cover both shapes.
class CreatePenTablesCommand : public CommandBase
{
public:
    CreatePenTablesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class CreateProfilesCommand : public CommandBase
{
public:
    CreateProfilesCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
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
