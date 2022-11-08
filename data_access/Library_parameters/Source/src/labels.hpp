#pragma once
#ifndef __labels__
#define __labels__

void	Do_CreateLabel_Associative(GS::UniString ,GS::UniString);
void	Do_Label_Edit(GS::UniString,GS::UniString); 
void	Do_CreateWord( API_Guid&, GS::UniString);
static  API_LeaderLineShapeID	GetNextLeaderLineShape(API_LeaderLineShapeID shape);
static  GSErrCode CreateMultiTextElement(const API_Coord& , GS::UniString ,double , const API_Guid& );
void    Do_CreateText(GS::UniString,GS::UniString,API_Coord);
#endif