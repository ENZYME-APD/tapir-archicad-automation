#pragma once

void GetWallQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index);
void GetSlabQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index);
GSErrCode GetQuantityMask(GS::UniString elementType, GS::UniString value, API_QuantitiesMask& mask,int& index);