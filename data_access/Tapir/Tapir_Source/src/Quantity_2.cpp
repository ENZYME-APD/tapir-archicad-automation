
#include  "APIdefs_Elements.h"
#include <APIdefs_Goodies.h>
#include <APIdefs_Database.h>
#include  "string.h"
#include <map>
#include "APICommon.h"
#include "ACAPinc.h"
#include  "Quantity_2.hpp"




static std::map<std::string, int> door2value{
       { "API_ZombieElemID", 0 },
       { "width1", 1},
       { "width2", 2 },
       { "height1", 3 },
       { "height2", 4 },
       { "surface1", 5 },
       { "surface2", 6 },
       { "nWidth1", 7 },
       { "nwidth2", 8 },
       { "nHeight1", 9 },
       { "nHeight2", 10 },
       { "nSurface1", 11 },
       { "nSurface2", 12 },
       { "volume", 13 },
       { "grossSurf", 14},
       { "grossVolume", 15 },
       { "surface", 16 },
       { "sillHeight", 17 },
       { "sillHeight1", 18},
       { "sillHeight2", 19 },
       { "headHeight", 20},
       { "headHeight1", 21 },
       { "headHeight2", 22 },
       { "sillHeight3", 23 },
       { "headHeight3", 24 }   
};


void GetDoorQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index) {
    int order = 0;
    std::string buffer(value.ToCStr().Get());
    auto x = door2value.find(buffer);
    if (x != std::end(door2value))
        order = x->second;

    switch (order) {
    case 1:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, width1);
        break;
    case 2:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, width2);
        break;
    case 3:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, height1);
        break;
    case 4:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, height2);
        break;
    case 5:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, surface1);
        break;
    case 6:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, surface2);
        break;
    case 7:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nWidth1);
        break;
    case 8:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nWidth2);
        break;
    case 9:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nHeight1);
        break;
    case 10:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nHeight2);
        break;
    case 11:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nSurface1);
        break;
    case 12:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, nSurface2);
        break;
    case 13:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, volume);
        break;
    case 14:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, grossSurf);
        break;
    case 15:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, grossVolume);
        break;
    case 16:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, surface);
        break;
    case 17:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, sillHeight);
        break;
    case 18:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, sillHeight1);
        break;
    case 19:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, sillHeight2);
        break;
    case 20:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, headHeight);
        break;
    case 21:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, headHeight1);
        break;
    case 22:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, headHeight2);
        break;
    case 23:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, sillHeight3);
        break;
    case 24:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, door, headHeight3);
        break;
    };
    index = order - 1;
    return;
}

void GetWindowQuantities(GS::UniString value, API_QuantitiesMask& mask, int& index) {
    int order = 0;
    std::string buffer(value.ToCStr().Get());
    auto x = door2value.find(buffer);
    if (x != std::end(door2value))
        order = x->second;

    switch (order) {
    case 1:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, width1);
        break;
    case 2:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, width2);
        break;
    case 3:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, height1);
        break;
    case 4:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, height2);
        break;
    case 5:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, surface1);
        break;
    case 6:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, surface2);
        break;
    case 7:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nWidth1);
        break;
    case 8:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nWidth2);
        break;
    case 9:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nHeight1);
        break;
    case 10:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nHeight2);
        break;
    case 11:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nSurface1);
        break;
    case 12:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, nSurface2);
        break;
    case 13:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, volume);
        break;
    case 14:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, grossSurf);
        break;
    case 15:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, grossVolume);
        break;
    case 16:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, surface);
        break;
    case 17:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, sillHeight);
        break;
    case 18:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, sillHeight1);
        break;
    case 19:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, sillHeight2);
        break;
    case 20:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, headHeight);
        break;
    case 21:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, headHeight1);
        break;
    case 22:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, headHeight2);
        break;
    case 23:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, sillHeight3);
        break;
    case 24:
        ACAPI_ELEMENT_QUANTITY_MASK_SET(mask, window, headHeight3);
        break;
    };
    index = order - 1;
    return;
}