#pragma once

#include "ACAPinc.h"

#ifdef ServerMainVers_2800
#define CaseInsensitive GS::CaseInsensitive
#else
#define CaseInsensitive GS::UniString::CaseInsensitive
#endif

#ifndef ServerMainVers_2700

#define ACAPI_MenuItem_RegisterMenu ACAPI_Register_Menu
#define ACAPI_MenuItem_InstallMenuHandler ACAPI_Install_MenuHandler

#define ACAPI_Markup_Create ACAPI_MarkUp_Create
#define ACAPI_Markup_Delete ACAPI_MarkUp_Delete
#define ACAPI_Markup_GetList ACAPI_MarkUp_GetList
#define ACAPI_Markup_AttachElements ACAPI_MarkUp_AttachElements
#define ACAPI_Markup_DetachElements ACAPI_MarkUp_DetachElements
#define ACAPI_Markup_GetAttachedElements ACAPI_MarkUp_GetAttachedElements
#define ACAPI_Markup_AddComment ACAPI_MarkUp_AddComment
#define ACAPI_Markup_GetComments ACAPI_MarkUp_GetComments
#define ACAPI_Markup_ExportToBCF ACAPI_MarkUp_ExportToBCF
#define ACAPI_Markup_ImportFromBCF ACAPI_MarkUp_ImportFromBCF

#define ACAPI_AddOnAddOnCommunication_InstallAddOnCommandHandler ACAPI_Install_AddOnCommandHandler

#define ACAPI_Element_NeigIDToElemType ACAPI_Goodies_NeigIDToElemType

#define ACAPI_Teamwork_ReceiveChanges ACAPI_TeamworkControl_ReceiveChanges
#define ACAPI_Teamwork_SendChanges ACAPI_TeamworkControl_SendChanges
#define ACAPI_Teamwork_ReserveElements ACAPI_TeamworkControl_ReserveElements
#define ACAPI_Teamwork_ReleaseElements ACAPI_TeamworkControl_ReleaseElements
#define ACAPI_Teamwork_GetUsernameFromId ACAPI_TeamworkControl_GetUsernameFromId

#define ACAPI_UserInput_SetElementHighlight ACAPI_Interface_SetElementHighlight
#define ACAPI_UserInput_ClearElementHighlight ACAPI_Interface_ClearElementHighlight

#define ACAPI_Selection_Select ACAPI_Element_Select
#define ACAPI_Grouping_GetConnectedElements ACAPI_Element_GetConnectedElements

#define ACAPI_Command_GetHttpConnectionPort(par1) ACAPI_Goodies (APIAny_GetHttpConnectionPortID, par1)

#define ACAPI_Element_CalcBounds(par1,par2) ACAPI_Database (APIDb_CalcBoundsID, par1, par2)

inline API_AttributeIndex ACAPI_CreateAttributeIndex (Int32 index)
{
    return index;
}

inline GSErrCode ACAPI_ProjectOperation_Quit (Int32 magicCode)
{
    Int64 magicCode64 = magicCode;
    return ACAPI_Automate (APIDo_QuitID, reinterpret_cast<void*> (magicCode64));
}

inline GSErrCode ACAPI_ProjectOperation_Project (API_ProjectInfo* projectInfo)
{
    return ACAPI_Environment (APIEnv_ProjectID, projectInfo);
}

inline GSErrCode ACAPI_ProjectOperation_ReloadLibraries ()
{
    return ACAPI_Automate (APIDo_ReloadLibrariesID);
}

inline GSErrCode ACAPI_ProjectOperation_Publish (const API_PublishPars* publishPars, const GS::Array<API_Guid>* selectedLinks = nullptr)
{
    return ACAPI_Automate (APIDo_PublishID, (void*) publishPars, (void*) selectedLinks);
}

inline GSErrCode ACAPI_ProjectOperation_Open (const API_FileOpenPars* fileOpenPars)
{
    return ACAPI_Automate (APIDo_OpenID, (void*) fileOpenPars);
}

inline GSErrCode ACAPI_AutoText_GetAutoTexts (GS::Array<GS::ArrayFB<GS::UniString, 3>>* autotexts, API_AutotextType autotextType)
{
    return ACAPI_Goodies (APIAny_GetAutoTextsID, autotexts, (void*) (GS::IntPtr) autotextType);
}

inline GSErrCode ACAPI_AutoText_SetAnAutoText (const GS::UniString* autotextDbKey = nullptr, const GS::UniString* autotextValue = nullptr)
{
    return ACAPI_Goodies (APIAny_SetAnAutoTextID, (void*) autotextDbKey, (void*) autotextValue);
}

inline GSErrCode ACAPI_Hotlink_GetHotlinkNode (API_HotlinkNode* hotlinkNode, bool* enableUnplaced = nullptr)
{
    return ACAPI_Database (APIDb_GetHotlinkNodeID, hotlinkNode, enableUnplaced);
}

inline GSErrCode ACAPI_Hotlink_GetHotlinkRootNodeGuid (const API_HotlinkTypeID* type, API_Guid* rootNodeGuid)
{
    return ACAPI_Database (APIDb_GetHotlinkRootNodeGuidID, (void*) type, (void*) rootNodeGuid);
}

inline GSErrCode ACAPI_Hotlink_GetHotlinkNodeTree (const API_Guid* hotlinkNodeGuid, GS::HashTable<API_Guid, GS::Array<API_Guid>>* hotlinkNodeTree)
{
    return ACAPI_Database (APIDb_GetHotlinkNodeTreeID, (void*) hotlinkNodeGuid, (void*) hotlinkNodeTree);
}

inline GSErrCode ACAPI_Window_GetCurrentWindow (API_WindowInfo* windowInfo)
{
    return ACAPI_Database (APIDb_GetCurrentWindowID, (void*) windowInfo);
}

inline GSErrCode ACAPI_Navigator_GetNavigatorSetNum (Int32* setNum)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorSetNumID, setNum);
}

inline GSErrCode ACAPI_Navigator_GetNavigatorSet (API_NavigatorSet* navigatorSet, Int32* index = nullptr)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorSetID, navigatorSet, index);
}

inline GSErrCode ACAPI_Navigator_GetNavigatorView (API_NavigatorItem* navigatorItem, API_NavigatorView* navigatorView)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorViewID, navigatorItem, navigatorView);
}

inline GSErrCode ACAPI_Navigator_ChangeNavigatorView (API_NavigatorItem* navigatorItem, API_NavigatorView* navigatorView)
{
    return ACAPI_Navigator (APINavigator_ChangeNavigatorViewID, navigatorItem, navigatorView);
}

inline GSErrCode ACAPI_View_Redraw ()
{
    return ACAPI_Automate (APIDo_RedrawID);
}

inline GSErrCode ACAPI_Element_UI2ElemPriority (GS::Int32* uiPriority, GS::Int32* elemPriority)
{
    return ACAPI_Goodies (APIAny_UI2ElemPriorityID, uiPriority, elemPriority);
}

inline GSErrCode ACAPI_LibraryManagement_GetLibraries (GS::Array<API_LibraryInfo>* activeLibs, Int32* embeddedLibraryIndex = nullptr)
{
    return ACAPI_Environment (APIEnv_GetLibrariesID, activeLibs, embeddedLibraryIndex);
}

inline GSErrCode ACAPI_ProjectSetting_GetStorySettings (API_StoryInfo* storyInfo)
{
    return ACAPI_Environment (APIEnv_GetStorySettingsID, storyInfo, nullptr);
}

inline GSErrCode ACAPI_LibraryPart_Get (API_LibPart* libpart)
{
    return ACAPI_LibPart_Get (libpart);
}

inline GSErrCode ACAPI_LibraryPart_Search (API_LibPart* ancestor, bool createIfMissing, bool onlyPlaceable = false)
{
    return ACAPI_LibPart_Search (ancestor, createIfMissing, onlyPlaceable);
}

inline GSErrCode ACAPI_LibraryPart_OpenParameters (API_ParamOwnerType* paramOwner)
{
    return ACAPI_Goodies (APIAny_OpenParametersID, paramOwner);
}

inline GSErrCode ACAPI_LibraryPart_GetActParameters (API_GetParamsType* theParams)
{
    return ACAPI_Goodies (APIAny_GetActParametersID, theParams);
}

inline GSErrCode ACAPI_LibraryPart_ChangeAParameter (API_ChangeParamType* changeParamType)
{
    return ACAPI_Goodies (APIAny_ChangeAParameterID, changeParamType);
}

inline GSErrCode ACAPI_LibraryPart_CloseParameters ()
{
    return ACAPI_Goodies (APIAny_CloseParametersID);
}

inline GSErrCode ACAPI_Navigator_GetNavigatorItem (const API_Guid* par1, API_NavigatorItem* par2)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorItemID, (void*) par1, (void*) par2);
}

inline GSErrCode ACAPI_Database_ChangeCurrentDatabase (API_DatabaseInfo* par1)
{
    return ACAPI_Database (APIDb_ChangeCurrentDatabaseID, (void*) par1);
}

inline GSErrCode ACAPI_Database_GetCurrentDatabase (API_DatabaseInfo* par1)
{
    return ACAPI_Database (APIDb_GetCurrentDatabaseID, (void*) par1);
}

inline GSErrCode ACAPI_Window_GetDatabaseInfo (API_DatabaseInfo* par1)
{
    return ACAPI_Database (APIDb_GetDatabaseInfoID, (void*) par1, nullptr);
}

#endif

#ifndef ServerMainVers_2600

inline GSErrCode ACAPI_IFC_GetIFCRelationshipData (API_IFCTranslatorIdentifier ifcTranslator, API_IFCRelationshipData ifcRelationshipData)
{
    return ACAPI_Goodies (APIAny_GetIFCRelationshipDataID, &ifcTranslator, &ifcRelationshipData);
}

inline GSErrCode ACAPI_IFC_GetIFCExportTranslatorsList (GS::Array<API_IFCTranslatorIdentifier> ifcExportTranslators)
{
    return ACAPI_Goodies (APIAny_GetIFCExportTranslatorsListID, &ifcExportTranslators);
}

inline GSErrCode ACAPI_MarkUp_ExportToBCF (const IO::Location& bcfFileLoc, const GS::Array<API_Guid>& issueIds, const bool useExternalId, const bool alignBySurveyPoint)
{
    (void) alignBySurveyPoint;
    return ACAPI_MarkUp_ExportToBCF (bcfFileLoc, issueIds, useExternalId);
}

inline GSErrCode ACAPI_MarkUp_ImportFromBCF (const IO::Location& bcfFileLoc, const bool silentMode, APIIFCRelationshipDataProc* ifcRelationshipDataProc, const void* par1, const bool openMarkUpPalette, const bool alignBySurveyPoint)
{
    (void) alignBySurveyPoint;
    return ACAPI_MarkUp_ImportFromBCF (bcfFileLoc, silentMode, ifcRelationshipDataProc, par1, openMarkUpPalette);
}

#endif

/*
    Proposed workaround for functions that aren't normally compatible between versions,
    it'provides an unified approach and function output for different versions.
    TAPIR_ index should distinguish the overriden ones from GS vanilla's API.
*/

#ifndef ServerMainVers_2600
typedef enum
{
    APIMarkUpComponent_Creation = 0,
    APIMarkUpComponent_Highlight,
    APIMarkUpComponent_Deletion,
    APIMarkUpComponent_Modification
} API_MarkUpComponentTypeID;

static bool IsMarkUpComponentShowsAsCorrected (API_MarkUpComponentTypeID type)
{
    bool asCorrected = false;
    switch (type) {
        case APIMarkUpComponent_Creation:
        case APIMarkUpComponent_Highlight:
        case APIMarkUpComponent_Deletion:
            asCorrected = false;
            break;
        case APIMarkUpComponent_Modification:
            asCorrected = true;
            break;
    }
    return asCorrected;
}
#endif

inline GSErrCode TAPIR_MarkUp_AttachElements (const API_Guid& issueId, const GS::Array<API_Guid>& elemIds, API_MarkUpComponentTypeID type)
{
#ifdef ServerMainVers_2600
    return ACAPI_Markup_AttachElements (issueId, elemIds, type);
#else
    return ACAPI_Markup_AttachElements (issueId, elemIds, IsMarkUpComponentShowsAsCorrected (type));
#endif
}

inline GSErrCode TAPIR_MarkUp_GetAttachedElements (API_Guid issueId, API_MarkUpComponentTypeID attachType, GS::Array<API_Guid>& elemIds)
{
    GSErrCode err;
#ifdef ServerMainVers_2600
    err = ACAPI_Markup_GetAttachedElements (issueId, attachType, elemIds);
#else
    GS::Array<API_Guid> correctedElements;
    GS::Array<API_Guid> highlightedElements;
    err = ACAPI_Markup_GetAttachedElements (issueId, &correctedElements, &highlightedElements);
    elemIds = IsMarkUpComponentShowsAsCorrected (attachType) ? correctedElements : highlightedElements;
#endif
    return err;
}

inline API_ElemTypeID GetElemTypeId (const API_Elem_Head& elemHead)
{
#ifdef ServerMainVers_2600
    return elemHead.type.typeID;
#else
    return elemHead.typeID;
#endif
}

inline GSErrCode TAPIR_Element_AddClassificationItemDefault (const API_Elem_Head& elemHead, const API_Guid& itemGuid)
{
#ifdef ServerMainVers_2600
    return ACAPI_Element_AddClassificationItemDefault (elemHead.type, itemGuid);
#else
    return ACAPI_Element_AddClassificationItemDefault (elemHead.typeID, elemHead.variationID, itemGuid);
#endif
}

inline GSErrCode TAPIR_Element_SetCategoryValueDefault (const API_Elem_Head& elemHead, const API_ElemCategoryValue& categoryValue)
{
#ifdef ServerMainVers_2700
    return ACAPI_Category_SetCategoryValueDefault (elemHead.type, categoryValue.category, categoryValue);
#else
#if defined ServerMainVers_2600
    return ACAPI_Element_SetCategoryValueDefault (elemHead.type, categoryValue.category, categoryValue);
#else
    return ACAPI_Element_SetCategoryValueDefault (elemHead.typeID, elemHead.variationID, categoryValue.category, categoryValue);
#endif
#endif
}

inline GSErrCode TAPIR_Element_SetPropertiesOfDefaultElem (const API_Elem_Head& elemHead, const GS::Array<API_Property>& properties)
{
#ifdef ServerMainVers_2600
    return ACAPI_Element_SetPropertiesOfDefaultElem (elemHead.type, properties);
#else
    return ACAPI_Element_SetPropertiesOfDefaultElem (elemHead.typeID, elemHead.variationID, properties);
#endif
}

inline Int32 GetAttributeIndex (const API_AttributeIndex& index)
{
#ifdef ServerMainVers_2700
    return index.ToInt32_Deprecated ();
#else
    return index;
#endif
}

#ifndef ServerMainVers_2700
inline GSErrCode ACAPI_Attribute_GetAttributesByType (API_AttrTypeID typeID, GS::Array<API_Attribute>& attributes)
{
    API_AttributeIndex count;
    GSErrCode err = ACAPI_Attribute_GetNum (typeID, &count);

    for (API_AttributeIndex i = 1; i <= count; ++i) {
        API_Attribute attr = {};
        attr.header.typeID = typeID;
        attr.header.index = i;

        if (ACAPI_Attribute_Get (&attr) == NoError) {
            attributes.Push (attr);
        }
    }

    return err;
}
#endif

inline void DisposeAttribute (API_Attribute& attr)
{
    if (attr.header.typeID == API_MaterialID) {
        delete attr.material.texture.fileLoc;
    }
}