#pragma once

#include "CommandBase.hpp"

// Shared with SetDetailsOfElementsCommand (ElementCommands.cpp): fills the memo's
// textContent/paragraphs handles and updates the API_TextType fields (nLine, useEolPos,
// nonBreaking, width, height) for the given content.
void SetTextContentAndParagraphs (API_ElementMemo& memo, API_TextType& textData, const GS::UniString& text);
// The memo mask for writing the content built by SetTextContentAndParagraphs/ApplyTextContent
// with ACAPI_Element_Change. Before Archicad 28 the content is UTF-16 with character offsets
// (charCode CC_UniCode), which the *Uni masks declare. With the plain masks every content or
// height change of a text with two or more characters failed on Archicad 26 (#759) while a
// one-character text and ACAPI_Element_Create, which takes no memo mask and goes by the
// element's charCode, worked: that is the pattern of ACAPI_Element_Change taking the handle as
// an 8-bit string, which ends at the first character's high zero byte, so only a one-character
// text has paragraph/run ranges that fit. From Archicad 28 the content is a GS::UniString and
// the plain masks are the only ones.
#ifdef ServerMainVers_2800
constexpr UInt64 TextContentMemoMask = APIMemoMask_TextContent | APIMemoMask_Paragraph;
#else
constexpr UInt64 TextContentMemoMask = APIMemoMask_TextContentUni | APIMemoMask_ParagraphUni;
#endif
const char* JustificationToString (API_JustID just);
// Shared with GetDetailsOfElementsCommand (ElementCommands.cpp): the origin a level dimension
// measures from, as the "DimensionOrigin" schema names it.
const char* DimensionOriginToString (API_DimOriginID origin);
bool ParseDimensionOriginString (const GS::UniString& origin, API_DimOriginID& result);
API_JustID ParseJustificationString (const GS::UniString& justification);

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
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
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

class CreateCurtainWallsCommand : public CreateElementsCommandBase
{
public:
    CreateCurtainWallsCommand ();
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

class CreateLevelDimensionsCommand : public CreateElementsCommandBase
{
public:
    CreateLevelDimensionsCommand ();
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

class ModifyTextsCommand : public CommandBase
{
public:
    ModifyTextsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

class ModifyLabelsCommand : public CommandBase
{
public:
    ModifyLabelsCommand ();
    virtual GS::String GetName () const override;
    virtual GS::Optional<GS::UniString> GetInputParametersSchema () const override;
    virtual GS::Optional<GS::UniString> GetRawResponseSchema () const override;
    virtual GS::ObjectState Execute (const GS::ObjectState& parameters, GS::ProcessControl& processControl) const override;
};

// Shared helpers for Text/Label style, content and leader-line fields - declared here so
// GetDetailsOfElementsCommand (ElementCommands.cpp) can read them back via AddXDetails.
namespace TextLabelDetails {

void AddTextStyleDetails (GS::ObjectState& os, const API_TextType& text, bool includeReadOnly);
// Returns an error response for a fontName no loaded font has.
GS::Optional<GS::ObjectState> ApplyTextStyleSettableDetails (const GS::ObjectState& details, API_TextType& text, API_Element* mask, bool isLabelUnion);

// Reads memo.textContent and every paragraph's runs back into "text" (flat concatenation),
// "paragraphCount" and, when the content has paragraphs, "runs" (array of TextRunDetails).
// Returns the error of the memo read; nothing is added then.
GSErrCode AddTextContent (GS::ObjectState& os, const API_Guid& elemGuid);
// The content to rebuild for a style-only modify: the element's own content read back, with the
// explicitly given per-run style fields (pen, font, faces, height, effects) applied to every run,
// as a multistyle element takes those from its runs rather than from the element-level fields.
// Returns an error response when the content cannot be read, or when it holds an autotext run
// (a protected run), which a rebuild from the read-back could replace with its resolved value.
GS::Optional<GS::ObjectState> ReadContentForStyleOnlyModify (const API_Guid& elemGuid, const GS::ObjectState& style, GS::ObjectState& contentParams);
// Whether a style change has to be written into the runs: only the fields a run carries (pen,
// font, faces, height, effects) need the content rebuilt; the element-level ones (angle, anchor,
// justification, frame, ...) apply through their own masks, leaving the content - and any
// autotext reference in it - untouched.
bool StyleNeedsContentRebuild (const GS::ObjectState& style);
// Builds memo.textContent/paragraphs from either a "runs" array or a plain "text" string.
GS::Optional<GS::ObjectState> ApplyTextContent (API_ElementMemo& memo, API_TextType& textData, const GS::ObjectState& parameters);

void AddLabelLeaderLineDetails (GS::ObjectState& os, const API_LabelType& label);
// Returns an error response for an attribute reference that cannot be resolved.
GS::Optional<GS::ObjectState> ApplyLabelLeaderLineSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask);

void AddLabelSymbolStyleDetails (GS::ObjectState& os, const API_LabelType& label);
// Returns an error response for a fontName no loaded font has.
GS::Optional<GS::ObjectState> ApplyLabelSymbolStyleSettableDetails (const GS::ObjectState& details, API_LabelType& label, API_Element* mask);

}
