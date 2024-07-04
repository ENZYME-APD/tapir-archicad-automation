#pragma once

#include "ACAPinc.h"

#ifndef ServerMainVers_2700

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

#define ACAPI_UserInput_SetElementHighlight ACAPI_Interface_SetElementHighlight
#define ACAPI_UserInput_ClearElementHighlight ACAPI_Interface_ClearElementHighlight

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

inline GSErrCode ACAPI_Navigator_GetNavigatorSetNum (Int32* setNum)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorSetNumID, setNum);
}

inline GSErrCode ACAPI_Navigator_GetNavigatorSet (API_NavigatorSet* navigatorSet, Int32* index = nullptr)
{
    return ACAPI_Navigator (APINavigator_GetNavigatorSetID, navigatorSet, index);
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

inline GSErrCode TAPIR_MarkUp_AttachElements (const API_Guid& issueId, const GS::Array<API_Guid>& elemIds, int type)
{
#ifdef ServerMainVers_2600
    API_MarkUpComponentTypeID cType = static_cast<API_MarkUpComponentTypeID>(type);
    return ACAPI_Markup_AttachElements (issueId, elemIds, cType);
#else
    int cType[] = { 0, 0, 0, 1 }; // AC25: corrected / highlighted
    return ACAPI_Markup_AttachElements (issueId, elemIds, cType[type]);
#endif
}

inline GSErrCode TAPIR_MarkUp_GetAttachedElements (API_Guid issueId, int attachType, GS::Array<API_Guid>& elemIds)
{
    GSErrCode err;
#ifdef ServerMainVers_2600
    API_MarkUpComponentTypeID elemType = static_cast<API_MarkUpComponentTypeID>(attachType);
    err = ACAPI_Markup_GetAttachedElements (issueId, elemType, elemIds);
#else
    GS::Array<GS::Array<API_Guid>> elemTypes;
    elemTypes.SetSize (4);
    err = ACAPI_Markup_GetAttachedElements (issueId, &elemTypes[3], &elemTypes[1]);
    elemIds = elemTypes[attachType];
#endif
    return err;

}