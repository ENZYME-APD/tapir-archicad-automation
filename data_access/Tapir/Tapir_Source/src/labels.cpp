#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h
#include	"APICommon.h"
#include    "APIdefs_Goodies.h"
#include    "APIdefs_Database.h"

#include	"labels.hpp"
#include    "APIdefs_Base.h"
#include	"GSUtilsDefs.h"
#include	"Point.hpp"
#include	"GSUnID.hpp"
#include    "API_Guid.hpp"

#include    "Point2D.hpp"
#include    "Point2DData.h"
#include    "BuiltInLibrary.hpp"
#include    "basicgeometry.h"
#include    "StringConversion.hpp"
#include    "GSUnID.hpp"


void	Do_CreateLabel_Associative(GS::UniString str,GS::UniString fontlength)
{
	API_ElemType	type;
	API_Guid		guid;
	API_Coord3D		begC3D;
	if (!ClickAnElem("Click the element to assign the Label to. The clicked point will be the \"begC\" of the Label.", API_ZombieElemID, nullptr, &type, &guid, &begC3D)) {
		WriteReport_Alert("No element was clicked.");
		return;
	}
	double fontsize;
	UniStringToValue(fontlength, fontsize);
	GS::UniString value;
	//GS::UniString autokey("Area");// This is wrong Interpret requires
	// a property not a name - use property key without <>
	// But -- to insert Autokey label use a key with <PROPERITY-key>
	GS::UniString autokey("PROPERTY-AC5CCA52-F79B-4850-92A9-BED7CB7C3847");
	GSErrCode error = ACAPI_Goodies(APIAny_InterpretAutoTextID, &autokey, &guid, &value);
	if (error) {
		ACAPI_WriteReport("Caught Error  %s", true, ErrID_To_Name(error));
	}
	bool	bAutoText;
	ACAPI_Goodies(APIAny_GetAutoTextFlagID, &bAutoText);

	if (bAutoText) {
		bool _bAutoText = false; // means use Key
		ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &_bAutoText);
	}

	GSErrCode		err;
	API_Element		element;
	API_ElementMemo	memo;

	BNZeroMemory(&element, sizeof(API_Element));
	element.header.type = API_LabelID;
	element.label.parentType = type;

	err = ACAPI_Element_GetDefaults(&element, &memo);
	

	if (bAutoText) {
		ACAPI_Goodies(APIAny_ChangeAutoTextFlagID, &bAutoText);
	}

	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetDefaults", err);
		return;
	}

	element.label.begC.x = begC3D.x;
	element.label.begC.y = begC3D.y;
	
	

	if (ClickAPoint("Click the \"midC\" of the Label. (Esc = default position)", &element.label.midC) &&
		ClickAPoint("Click the \"endC\" of the Label. (Esc = default position)", &element.label.endC))
		element.label.createAtDefaultPosition = false;
	else
		element.label.createAtDefaultPosition = true;

	element.label.parent = guid;

	if (element.label.labelClass == APILblClass_Text) {
		const char* predefinedContent = "Default text was empty.\n";

		if (memo.textContent == nullptr || Strlen32(*memo.textContent) < 2) {
			BMhKill(&memo.textContent);
			memo.textContent = BMhAllClear(str.GetLength() + 1);
			strcpy(*memo.textContent, str.ToCStr().Get());
			(*memo.paragraphs)[0].run[0].range = str.GetLength();
			(*memo.paragraphs)[0].run[0].from = 0;
			(*memo.paragraphs)[0].run[0].size = fontsize;
		}
		
		element.label.u.text.angle = 0;
		element.label.u.text.nonBreaking = true;
		element.label.u.text.nLine = 3;
		element.label.u.text.height = 350;
		element.label.u.text.width = 250;
		element.label.u.head.type = API_LabelID;
		element.label.labelClass = APILblClass_Text;	
	}
	err = ACAPI_CallUndoableCommand("Changing Label",
		[&]() -> GSErrCode {
		err = ACAPI_Element_Create(&element, &memo);
		return err;
	});
	if (err != NoError)
	ACAPI_WriteReport("ACAPI_Element_Create (Associative Label %s)",true, ErrID_To_Name(err));

	ACAPI_DisposeElemMemoHdls(&memo);
}		// Do_CreateLabel_Associative



static API_LeaderLineShapeID	GetNextLeaderLineShape(API_LeaderLineShapeID shape)
{
	switch (shape) {
	case API_Segmented:		return API_Splinear;
	case API_Splinear:		return API_SquareRoot;
	case API_SquareRoot:	return API_Segmented;
	default:				DBBREAK(); return API_Segmented;
	}
}

void	Do_Label_Edit(GS::UniString guid,GS::UniString str)
{
	API_Element			element, mask;
	GS::Guid gsGuid;
	gsGuid.ConvertFromString(guid.ToCStr().Get());
	API_Guid gu;
	gu = GSGuid2APIGuid(gsGuid);
	API_ElementMemo		memo;
	API_Neig			neig;
	char text[1024];
	strcpy(text,str.ToCStr().Get());
	BNZeroMemory(&element, sizeof(API_Element));
	BNZeroMemory(&memo, sizeof(API_ElementMemo));
	element.header.guid = gu;

	GSErrCode err = ACAPI_Element_Get(&element);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_Get", err);
		return;
	}

	err = ACAPI_Element_GetMemo(element.header.guid, &memo);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetMemo", err);
		return;
	}
	if (memo.textContent != nullptr)
		BMKillHandle(&memo.textContent);

	memo.textContent = BMhAllClear(Strlen32(text) + 1);
	if (memo.textContent == nullptr)
		return;
	CHCopyC(text, memo.textContent[0]);

	if (err == NoError) {
		ACAPI_ELEMENT_MASK_CLEAR(mask);
		if (element.label.labelClass == APILblClass_Text)
			ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.pen);
		if (element.label.labelClass == APILblClass_Text)
			ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, u.text.size);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, pen);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, textSize);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, textWay);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, font);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, faceBits);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, useBgFill);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, fillBgPen);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, effectsBits);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, ltypeInd);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, framed);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, hasLeaderLine);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, leaderShape);
		ACAPI_ELEMENT_MASK_SET(mask, API_LabelType, begC);

		
		element.label.labelClass = APILblClass_Text;
		element.label.textWay = APIDir_General;
		element.label.font = 20;
		element.label.faceBits = (element.label.effectsBits == APIFace_Plain) ? APIFace_Italic : APIFace_Plain;
		element.label.useBgFill = !element.label.useBgFill;
		element.label.fillBgPen = (element.label.fillBgPen + 3) % 256;
		element.label.effectsBits = ((element.label.effectsBits & APIEffect_StrikeOut) != 0) ? (element.label.effectsBits & ~APIEffect_StrikeOut) : (element.label.effectsBits | APIEffect_StrikeOut);
		element.label.ltypeInd = 1;
	
		err = ACAPI_CallUndoableCommand("Changing Label",
			[&]() -> GSErrCode {
			err = ACAPI_Element_Change(&element, &mask, &memo, APIMemoMask_TextContent, true);
			return err;
		});
	}

	ACAPI_DisposeElemMemoHdls(&memo);

	return; // Do_Label_Edit
}

static GSErrCode SetParagraph(API_ParagraphType** paragraph, UInt32 parNum, Int32 from, Int32 range, API_JustID just, double firstIndent,
	double indent, double rightIndent, double spacing, Int32 numOfTabs, Int32 numOfRuns,
	Int32 numOfeolPos)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize(reinterpret_cast<GSHandle> (paragraph)) / sizeof(API_ParagraphType)))
		return APIERR_BADPARS;

	if (numOfTabs < 1 || numOfRuns < 1 || numOfeolPos < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].from = from;
	(*paragraph)[parNum].range = range;
	(*paragraph)[parNum].just = just;
	(*paragraph)[parNum].firstIndent = firstIndent;
	(*paragraph)[parNum].indent = indent;
	(*paragraph)[parNum].rightIndent = rightIndent;
	(*paragraph)[parNum].spacing = spacing;

	(*paragraph)[parNum].tab = reinterpret_cast<API_TabType*> (BMAllocatePtr(numOfTabs * sizeof(API_TabType), ALLOCATE_CLEAR, 0));
	if ((*paragraph)[parNum].tab == nullptr)
		return APIERR_MEMFULL;

	(*paragraph)[parNum].run = reinterpret_cast<API_RunType*> (BMAllocatePtr(numOfRuns * sizeof(API_RunType), ALLOCATE_CLEAR, 0));
	if ((*paragraph)[parNum].run == nullptr)
		return APIERR_MEMFULL;

	if (numOfeolPos > 0) {
		(*paragraph)[parNum].eolPos = reinterpret_cast<Int32*> (BMAllocatePtr(numOfeolPos * sizeof(Int32), ALLOCATE_CLEAR, 0));
		if ((*paragraph)[parNum].eolPos == nullptr)
			return APIERR_MEMFULL;
	}

	return NoError;
}


// -----------------------------------------------------------------------------
// Set a tab of a paragraph
// -----------------------------------------------------------------------------
static GSErrCode SetTab(API_ParagraphType** paragraph, UInt32 parNum, UInt32 tabNum, API_TabID	type, double pos)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize(reinterpret_cast<GSHandle> (paragraph)) / sizeof(API_ParagraphType)))
		return APIERR_BADPARS;

	if (tabNum >= BMGetPtrSize(reinterpret_cast<GSPtr> ((*paragraph)[parNum].tab)) / sizeof(API_TabType))
		return APIERR_BADPARS;

	(*paragraph)[parNum].tab[tabNum].type = type;
	(*paragraph)[parNum].tab[tabNum].pos = pos;

	return NoError;
}


// -----------------------------------------------------------------------------
// Set a run of a paragraph
// -----------------------------------------------------------------------------
static GSErrCode SetRun(API_ParagraphType** paragraph, UInt32 parNum, UInt32 runNum, Int32 from, Int32 range, short pen, unsigned short faceBits,
	short font, unsigned short effectBits, double size)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize(reinterpret_cast<GSHandle> (paragraph)) / sizeof(API_ParagraphType)))
		return APIERR_BADPARS;

	if (runNum >= BMGetPtrSize(reinterpret_cast<GSPtr> ((*paragraph)[parNum].run)) / sizeof(API_RunType))
		return APIERR_BADPARS;

	(*paragraph)[parNum].run[runNum].from = from;
	(*paragraph)[parNum].run[runNum].range = range;
	(*paragraph)[parNum].run[runNum].pen = pen;
	(*paragraph)[parNum].run[runNum].faceBits = faceBits;
	(*paragraph)[parNum].run[runNum].font = font;
	(*paragraph)[parNum].run[runNum].effectBits = effectBits;
	(*paragraph)[parNum].run[runNum].size = size;

	return NoError;
}


// -----------------------------------------------------------------------------
// Set EOL array of a paragraph
// -----------------------------------------------------------------------------
static GSErrCode SetEOL(API_ParagraphType** paragraph, UInt32 parNum, UInt32 eolNum, Int32 offset)
{
	if (paragraph == nullptr || parNum >= (BMhGetSize(reinterpret_cast<GSHandle> (paragraph)) / sizeof(API_ParagraphType)))
		return APIERR_BADPARS;

	if (eolNum >= BMGetPtrSize(reinterpret_cast<GSPtr> ((*paragraph)[parNum].eolPos)) / sizeof(Int32))
		return APIERR_BADPARS;

	if (offset < 0)
		return APIERR_BADPARS;

	(*paragraph)[parNum].eolPos[eolNum] = offset;

	return NoError;
}


static void	ReplaceEmptyTextWithPredefined(API_ElementMemo& memo)
{
	const char* predefinedContent = "Default text was empty.";

	if (memo.textContent != nullptr || Strlen32(*memo.textContent) < 2) {
		BMhKill(&memo.textContent);
		memo.textContent = BMhAllClear(Strlen32(predefinedContent) + 1);
		strcpy(*memo.textContent, predefinedContent);
		(*memo.paragraphs)[0].run[0].range = Strlen32(predefinedContent);
	}
}

// -----------------------------------------------------------------------------
// Create a multistyle text
// -----------------------------------------------------------------------------
static GSErrCode CreateMultiTextElement(const API_Coord& pos, GS::UniString text , double scale = 1.0, const API_Guid& renFiltGuid = APINULLGuid)
{
	API_Element			element = {};
	API_ElementMemo		memo = {};
	GSErrCode			err;
	//const GS::UniString text = L("This multistyle\nword w\u00E1s created\nby the Element Test example project.");
	Int32				numOfParagraphs = 3;

	memo.textContent = BMhAllClear((text.GetLength() + 1) * sizeof(GS::uchar_t));
	if (memo.textContent == nullptr)
		return APIERR_MEMFULL;

	GS::ucscpy(reinterpret_cast<GS::uchar_t*> (*memo.textContent), text.ToUStr());

	element.header.type = API_TextID;
	
	err = ACAPI_Element_GetDefaults(&element, nullptr);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetDefaults (Text)", err);
		ACAPI_DisposeElemMemoHdls(&memo);
		return err;
	}

	ReplaceEmptyTextWithPredefined(memo);

	element.header.renovationStatus = API_ExistingStatus;
	element.header.renovationFilterGuid = renFiltGuid;		// APINULLGuid is handled internally

	element.text.loc.x = pos.x;
	element.text.loc.y = pos.y;
	element.text.anchor = APIAnc_LB;
	element.text.multiStyle = false;
	element.text.nonBreaking = false;
	element.text.useEolPos = true;
	element.text.width = 150 * scale;
	element.text.charCode = CC_UniCode;

	/*
	err = SetParagraph(memo.paragraphs, 0, 0, 15, APIJust_Left, 2.0 * scale, 0, 0, -1.0, 1, 2, 1);
	if (err == NoError) {
		err = SetRun(memo.paragraphs, 0, 0, 0, 5, 3, APIFace_Plain, element.text.font, 0, 4.0 * scale);		// "This "
		if (err == NoError)
			err = SetRun(memo.paragraphs, 0, 1, 5, 10, 5, APIFace_Bold, element.text.font, 0, 4.5 * scale);	// "multistyle"
		if (err == NoError)
			err = SetTab(memo.paragraphs, 0, 0, APITab_Left, 2.0 * scale);
		if (err == NoError)
			err = SetEOL(memo.paragraphs, 0, 0, 14);
	}

	if (err == NoError) {
		err = SetParagraph(memo.paragraphs, 1, 16, 16, APIJust_Right, 2.0 * scale, 0, 0, -1.5, 1, 2, 1);
		if (err == NoError) {
			err = SetRun(memo.paragraphs, 1, 0, 0, 9, 7, APIFace_Plain, element.text.font, 0, 4.0 * scale);	// "word was "
			if (err == NoError)
				err = SetRun(memo.paragraphs, 1, 1, 9, 7, 9, APIFace_Italic, element.text.font, APIEffect_StrikeOut, 3.0 * scale);
			if (err == NoError)																					// "created"
				err = SetTab(memo.paragraphs, 1, 0, APITab_Left, 0.0);
			if (err == NoError)
				err = SetEOL(memo.paragraphs, 1, 0, 15);
		}
	}

	if (err == NoError) {
		err = SetParagraph(memo.paragraphs, 2, 33, 36, APIJust_Center, 0, 0, 0, -1.0, 1, 3, 2);
		if (err == NoError) {
			err = SetRun(memo.paragraphs, 2, 0, 0, 19, 11, APIFace_Underline, element.text.font, 0, 4.0 * scale);
			if (err == NoError)																					// "by the Element Test"
				err = SetRun(memo.paragraphs, 2, 1, 19, 9, 13, APIFace_Plain, element.text.font, APIEffect_StrikeOut, 6.0 * scale);
			if (err == NoError)																					// " example "
				err = SetRun(memo.paragraphs, 2, 2, 28, 8, 15, APIFace_Bold, element.text.font, APIEffect_SubScript, 5.0 * scale);
			if (err == NoError)																					// "project."
				err = SetTab(memo.paragraphs, 2, 0, APITab_Left, 0.0);
			if (err == NoError)
				err = SetEOL(memo.paragraphs, 2, 0, 27);
			if (err == NoError)
				err = SetEOL(memo.paragraphs, 2, 1, 35);
		}
	}
	*/

	if (err == NoError) {
		err = ACAPI_CallUndoableCommand("Create Label",
			[&]() -> GSErrCode {
			err = ACAPI_Element_Create(&element,&memo);
			return err;
		});
		if (err != NoError)
			ErrorBeep("ACAPI_Element_Create (text)", err);
	}

	ACAPI_DisposeElemMemoHdls(&memo);

	return err;
}		// CreateMultiTextElement


// -----------------------------------------------------------------------------
// Place a multistyle text element
// -----------------------------------------------------------------------------

static API_Guid	gLastRenFiltGuid = APINULLGuid;
static API_Guid	renFiltGuid = APINULLGuid;
void	Do_CreateWord(API_Guid& renFiltGuid,  GS::UniString text)
{
	API_Coord	c;

	if (!ClickAPoint("Enter the left bottom position of the text", &c))
		return;

	GSErrCode err =  CreateMultiTextElement(c, text, 1.0  );

	return;
}		// Do_CreateWord