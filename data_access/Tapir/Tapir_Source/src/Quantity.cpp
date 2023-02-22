#include  "APIdefs_Elements.h"
#include <APIdefs_Goodies.h>
#include <APIdefs_Database.h>
#include  "string.h"
#include <map>
#include "APICommon.h"
#include "ACAPinc.h"
#include  "Quantity.h"
#include  "Quantity_2.hpp"


    static std::map<std::string, int> string2value{
       { "API_ZombieElemID", 0 },
       { "volume", 1}, 
       { "volume_cond", 2 }, 
       { "volumeASkin", 3 }, 
       { "volumeBSkin", 4 }, 
       { "volumeASkin_cond", 5 },
       { "volumeBSkin_cond", 6 },  
       { "surface1", 7 }, 
       { "surface2", 8 }, 
       { "surface3", 9 }, 
       { "surface1_cond", 10 }, 
       { "surface2_cond", 11 }, 
       { "length", 12 }, 
       { "windowsSurf", 13 },
       { "doorsSurf", 14 },
       { "emptyholesSurf", 15},
       { "filler_0", 16 }, //int32 filler - packed as 8 bytes (double)
       { "columnsNumber", 17 },// this is a int32 - handled different - see code
     //  { "filler_1", 18 },// int32 here to pack as 8 bytes (double)
       { "windowsWidth", 18},
       { "doorsWidth", 19 }, 
       { "minHeight",20 },
       { "maxHeight",21 },
       { "minHeightASkin", 22 },
       { "maxHeightASkin", 23 },
       { "minHeightBSkin", 24 },
       { "maxHeightBSkin", 25 },
       { "centerLength", 26 },
       { "area", 27 },
       { "perimeter", 28 },
       { "grossVolume", 29 },
       { "grossSurf1", 30 }, 
       { "grossSurf2", 31 }, 
       { "emptyHolesVolume", 32 },
       { "emptyholesSurf1", 33 }, 
       { "emptyHolesSurf2", 34 },
       { "length12", 35 },
       { "length34", 36 },
       { "length12_cond", 37},
       { "length34_cond", 38 },
       { "insuThickness", 39 },
        { "airThickness",  40 },
       { "skinAThickness", 41 },
       { "skinBThibkness", 42 },
       { "refLineLength", 43 } 
      
    };
    static std::map<std::string, int> slab2value{
       { "API_ZombieElemID", 0 },
       { "bottomSurface", 1},
       { "topSurface", 2 },
       { "edgeSurface", 3 },
       { "bottonSurface_cond", 4 },
       { "topSurface_cond", 5 },
       { "volume", 6 },
       { "volume_cond", 7 },
       { "perimeter", 8 },
       { "holesSurf", 9 },
       { "holesPrm", 10 },
       { "grossBotSurf", 11 },
       { "grossTopSurf", 12 },
       { "grossEdgeSurf", 13 },
       { "grossVolume", 14},
       { "grossBotSurfWithHoles", 15 },
       { "grossTopSurfWithHoles", 16 },
       { "grossEdgeSurfWithHoles", 17 },
       { "grossVolumeWithHoles", 18}
    };
   


GSErrCode GetQuantityMask(GS::UniString elementType, GS::UniString value, API_QuantitiesMask& mask, int& index)
{
    GSErrCode err = 0;

    if (elementType == "API_WallID") 
         GetWallQuantities(value, mask, index);
    else if (elementType == "API_SlabID")
       GetSlabQuantities(value, mask, index);
    else if (elementType == "API_DoorID")
         GetDoorQuantities(value, mask, index);
    else if (elementType == "API_WindowID")
        GetWindowQuantities(value, mask, index);
    return err;
}

void GetWallQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index)
{
    int order = 0;
    std::string buffer(value.ToCStr().Get());
    auto x = string2value.find(buffer);
    if (x != std::end(string2value)) 
        order = x->second; 

    switch (order) {
    case 1:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volume);
        break;
    case 2:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volume_cond);
        break;
    case 3:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volumeASkin);
        break;
    case 4:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volumeBSkin);
        break;
    case 5:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volumeASkin_cond);
        break;
    case 6:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, volumeBSkin_cond);
        break;
    case 7:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, surface1);
        break;
    case 8:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, surface2);
        break;
    case 9:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, surface3);
        break;
    case 10:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, surface1_cond);
        break;
    case 11:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, surface2_cond);
        break;
    case 12:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, length);
        break;
    case 13:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, windowsSurf);
        break;
    case 14:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, doorsSurf);
        break;
    case 15:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, emptyholesSurf);
        break;
    case 16:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, filler_0);
        break;
    case 17:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, columnsNumber);
        break;
      
    case 18:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, windowsWidth);
        break;
    case 19:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, doorsWidth);
        break;
    case 20:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, minHeight);
        break;
    case 21:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, maxHeight);
        break;
    case 22:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, minHeightASkin);
        break;
    case 23:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, maxHeightASkin);
        break;
    case 24:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, minHeightBSkin);
        break;
    case 25:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, maxHeightBSkin);
        break;
    case 26:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, centerLength);
        break;
    case 27:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, area);
        break;
    case 28:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, perimeter);
        break;
    case 29:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, grossVolume);
        break;
    case 30:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, grossSurf1);
        break;
    case 31:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, grossSurf2);
        break;
    case 32:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, emptyHolesVolume);
        break;
    case 33:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, emptyHolesSurf1);
        break;
    case 34:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, emptyHolesSurf2);
        break;
    case 35:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, length12);
        break;
    case 36:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, length34);
        break;
    case 37:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, length12_cond);
        break;
    case 38:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, length34_cond);
        break;
    case 39:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, insuThickness);
        break;
    case 40:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, airThickness);
        break;
    case 41:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, skinAThickness);
        break;
    case 42:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, skinBThickness);
        break;
    case 43:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, wall, refLineLength);
        break;
    }
    index = order - 1;
}
void GetSlabQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index) {
    int order = 0;
    std::string buffer(value.ToCStr().Get());
    auto x = slab2value.find(buffer);
    if (x != std::end(slab2value))
        order = x->second;

    switch (order) {
    case 1:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, bottomSurface);
        break;
    case 2:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, topSurface);
        break;
    case 3:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, edgeSurface);
        break;
    case 4:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, bottomSurface_cond);
        break;
    case 5:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, topSurface_cond);
        break;
    case 6:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, volume);
        break;
    case 7:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, volume_cond);
        break;
    case 8:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, perimeter);
        break;
    case 9:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, holesSurf);
        break;
    case 10:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, holesPrm);
        break;
    case 11:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossBotSurf);
        break;
    case 12:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossTopSurf);
        break;
    case 13:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossEdgeSurf);
        break;
    case 14:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossVolume);
        break;
    case 15:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossBotSurfWithHoles);
        break;
    case 16:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossTopSurfWithHoles);
        break;
    case 17:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossEdgeSurfWithHoles);
        break;
    case 18:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, slab, grossVolumeWithHoles);
        break;
    };
    index = order - 1;
    return;
}