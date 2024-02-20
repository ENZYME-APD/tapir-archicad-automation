#pragma once

#include "ACAPinc.h"

#ifndef ServerMainVers_2700

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

/**/
inline GSErrCode ACAPI_ProjectSetting_GetStorySettings (API_StoryInfo* storyInfo)
{
    return ACAPI_Environment (APIEnv_GetStorySettingsID, storyInfo, nullptr);
}

#endif
