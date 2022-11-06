#include	"APIEnvir.h"
#include	"ACAPinc.h"					// also includes APIdefs.h

#include	"File.hpp"
#include	"StableArray.hpp"
//#include	"AutoPtr.hpp"
#include	"TextOProtocol.hpp"

#include	"Polygon2DData.h"
#include	"Polygon2D.hpp"
#include	"Polygon2DDataConv.h"

#include	"APICommon.h"
//#include	"Element_Test.h"
#include	"vaarray.hpp"
#include	"Pair.hpp"
#include    "calcquantity.hpp"
#include    "utilities.hpp"
#include    "APICommon.h"

GSErrCode PropertyUtils::PropertyToString(const API_Property& property, GS::UniString& propertyValue)
{
	if (property.status == API_Property_NotAvailable || property.status == API_Property_NotEvaluated) {
		return APIERR_BADPROPERTY;
	}

	return ACAPI_Property_GetPropertyValueString(property, &propertyValue);
}

// -----------------------------------------------------------------------------
// Get quantities of the clicked element
// -----------------------------------------------------------------------------

GS::Array<GS::Array<GS::UniString>>	Do_CalcQuantities(GS::UniString selected,GS::UniString elemtype)
{

	GS::Array<GS::Array<GS::UniString >> arr;
	GS::Array<GS::UniString> buf;
	GS::UniString str1("Failed");
	buf.Push(str1);
	arr.Push(buf);

	API_ElemTypeID		typeID;
	API_Guid			guid;
	API_ElementQuantity	quantity;
	API_QuantityPar		params;
	GSErrCode			err;
	API_ElemTypeID w = GetenumIndex(elemtype.ToCStr().Get());
	guid = APIGuidFromString(selected.ToCStr().Get());

	typeID = w;
	API_Element element;
	BNZeroMemory(&element, sizeof element);
	element.header.type = typeID;
	element.header.guid = guid;

	err = ACAPI_Element_Get(&element);
	if (err != NoError) {
		WriteReport_Alert("Unknown element");
		return arr;
	}

	BNZeroMemory(&params, sizeof(API_QuantityPar));
	params.minOpeningSize = EPS;
	
	GS::Array <API_CompositeQuantity>			composites;
	GS::Array <API_ElemPartQuantity>			elemPartQuantities;
	GS::Array <API_ElemPartCompositeQuantity>	elemPartComposites;

	GS::Array<API_Quantities>	quantities;
	GS::Array<API_Guid>			elemGuids;
	
	quantities.PushNew();
	quantities[0].elements = &quantity;
	quantities[0].composites = &composites;
	quantities[0].elemPartQuantities = &elemPartQuantities;
	quantities[0].elemPartComposites = &elemPartComposites;

	elemGuids.Push(element.header.guid);

	API_QuantitiesMask mask;
	ACAPI_ELEMENT_QUANTITIES_MASK_SETFULL(mask);
	err = ACAPI_Element_GetMoreQuantities(&elemGuids, &params, &quantities, &mask);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetQuantities", err);
		return arr ;
	}

	WriteReport("%s GUID:%s", ElemID_To_Name(typeID).ToCStr(CC_UTF8).Get(),
		APIGuidToString(element.header.guid).ToCStr().Get());

	char temp[254] = {};
	switch (typeID) {
	case API_WallID:
		
		arr.Clear();
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		
		WriteReport("  surface1: %lf", quantity.wall.surface1);
		buf.Clear();
		str1 = "surface1";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.surface1);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  surface2: %lf", quantity.wall.surface2);
		buf.Clear();
		str1 = "surface2";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.surface2);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  surface3: %lf", quantity.wall.surface3);
		buf.Clear();
		str1 = "surface3";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.surface3);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  length:   %lf", quantity.wall.length);
		buf.Clear();
		str1 = "length";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.length);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  windowsSurf:    %lf", quantity.wall.windowsSurf);
		buf.Clear();
		str1 = "windowsSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.windowsSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  doorsSurf:      %lf", quantity.wall.doorsSurf);
		buf.Clear();
		str1 = "doorssurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.doorsSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  emptyholesSurf: %lf", quantity.wall.emptyholesSurf);
		buf.Clear();
		str1 = "emptyHolesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.emptyholesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  windowsWidth:   %d", quantity.wall.windowsWidth);
		buf.Clear();
		str1 = "windowsWidth";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.windowsWidth);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  doorsWidth:     %lf", quantity.wall.doorsWidth);
		buf.Clear();
		str1 = "doorsWidth";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.wall.doorsWidth);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		for (UInt32 i = 0; i < composites.GetSize(); i++) {
			API_Attribute	attr;
			GS::UniString	cFlags;
			GSErrCode		err;

			BNZeroMemory(&attr, sizeof(API_Attribute));
			attr.header.typeID = API_BuildingMaterialID;
			attr.header.index = composites[i].buildMatIndices;

			err = ACAPI_Attribute_Get(&attr);
			if (err == NoError) {
				if ((composites[i].flags & APISkin_Core) != 0) {
					cFlags.Assign("Core");
				} if ((composites[i].flags & APISkin_Finish) != 0) {
					if (!cFlags.IsEmpty())
						cFlags.Append("+");
					cFlags.Append("Finish");
				}
			}

			WriteReport("  	composite:   %s[%d]		- volume: %lf		- projected area: %lf	- flag: %s", attr.header.name, composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea, static_cast<const char*> (cFlags.ToCStr()));
			sprintf(temp,"composite: %s[%d]- volume: %lf- projected area: %lf - flag: %s", attr.header.name, composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea, static_cast<const char*> (cFlags.ToCStr()));
			buf.Clear();
			buf.Push(temp);
			arr.Push(buf);
		}
		break;
	case API_ColumnID:
		arr.Clear();
		WriteReport("  coreSurface: %lf", quantity.column.coreSurface);
		buf.Clear();
		str1 = "coreSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.column.coreSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  veneSurface: %lf", quantity.column.veneSurface);
		buf.Clear();
		str1 = "veneSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.column.veneSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  coreVolume:  %lf", quantity.column.coreVolume);
		buf.Clear();
		str1 = "coreVolumn";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.column.coreVolume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  veneVolume:  %lf", quantity.column.veneVolume);
		buf.Clear();
		str1 = "veneVolume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.column.veneVolume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_BeamID:
		arr.Clear();
		WriteReport("  rightLength:      %lf", quantity.beam.rightLength);
		buf.Clear();
		str1 = "rightLength";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.rightLength);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  leftLength:       %lf", quantity.beam.leftLength);
		buf.Clear();
		str1 = "leftLength";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.leftLength);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  bottomSurface:    %lf", quantity.beam.bottomSurface);
		buf.Clear();
		str1 = "bottomSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.bottomSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  topSurface:       %lf", quantity.beam.topSurface);
		buf.Clear();
		str1 = "topSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.topSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurfaceLeft:  %lf", quantity.beam.edgeSurfaceLeft);
		buf.Clear();
		str1 = "edgeSurfaceLeft";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.edgeSurfaceLeft);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurfaceRight: %lf", quantity.beam.edgeSurfaceRight);
		buf.Clear();
		str1 = "edgeSurfaceRight";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.edgeSurfaceRight);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurface:      %lf", quantity.beam.edgeSurface);
		buf.Clear();
		str1 = "edgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.edgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurface:     %lf", quantity.beam.holesSurface);
		buf.Clear();
		str1 = "holesSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.holesSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesEdgeSurface: %lf", quantity.beam.holesEdgeSurface);
		buf.Clear();
		str1 = "holesEdgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.holesEdgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesNumber:      %d", quantity.beam.holesNumber);
		buf.Clear();
		str1 = "holesNumber";
		buf.Push(str1);
		sprintf(temp, "%d", quantity.beam.holesNumber);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:           %lf", quantity.beam.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesVolume:      %lf", quantity.beam.holesVolume);
		buf.Clear();
		str1 = "holesVolume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.beam.holesVolume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_DoorID:
	case API_WindowID:
	case API_SkylightID:
		arr.Clear();
		WriteReport("  surface:  %lf", quantity.window.surface);
		buf.Clear();
		str1 = "surface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.window.surface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:   %lf", quantity.window.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.window.volume);
		buf.Clear();
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_ObjectID:
	case API_LampID:
		arr.Clear();
		WriteReport("  surface:  %lf", quantity.symb.surface);
		buf.Clear();
		str1 = "surface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.symb.surface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:   %lf", quantity.symb.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.symb.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_SlabID:
		arr.Clear();
		WriteReport("  bottomSurface: %lf", quantity.slab.bottomSurface);
		buf.Clear();
		str1 = "bottomSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.bottomSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  topSurface:    %lf", quantity.slab.topSurface);
		buf.Clear();
		str1 = "topSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.topSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurface:   %lf", quantity.slab.edgeSurface);
		buf.Clear();
		str1 = "edgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.edgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:        %lf", quantity.slab.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:     %lf", quantity.slab.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurf:     %lf", quantity.slab.holesSurf);
		buf.Clear();
		str1 = "holesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.holesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:      %lf", quantity.slab.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.slab.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		for (UInt32 i = 0; i < composites.GetSize(); i++) {
			WriteReport("  	composite:   %d		- volume: %lf		- projected area: %lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			sprintf(temp,"composite: %d - volume: %lf - projected area: %lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			buf.Clear();
			str1 = temp;
			buf.Push(str1);
			arr.Push(buf);
		}
		break;
	case API_MeshID:
		arr.Clear();
		WriteReport("  bottomSurface: %lf", quantity.mesh.bottomSurface);
		buf.Clear();
		str1 = "bottomSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.bottomSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  topSurface:    %lf", quantity.mesh.topSurface);
		buf.Clear();
		str1 = "topSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.topSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurface:   %lf", quantity.mesh.edgeSurface);
		buf.Clear();
		str1 = "edgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.edgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:        %lf", quantity.mesh.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:     %lf", quantity.mesh.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurf:     %lf", quantity.mesh.holesSurf);
		buf.Clear();
		str1 = "holesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.holesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:      %lf", quantity.mesh.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.mesh.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_RoofID:
		arr.Clear();
		WriteReport("  bottomSurface: %lf", quantity.roof.bottomSurface);
		buf.Clear();
		str1 = "bottomSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.bottomSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  topSurface:    %lf", quantity.roof.topSurface);
		buf.Clear();
		str1 = "topSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.topSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurface:   %lf", quantity.roof.edgeSurface);
		buf.Clear();
		str1 = "edgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.edgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:        %lf", quantity.roof.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:     %lf", quantity.roof.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurf:     %lf", quantity.roof.holesSurf);
		buf.Clear();
		str1 = "holesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.holesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:      %lf", quantity.roof.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.roof.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		for (UInt32 i = 0; i < composites.GetSize(); i++) {
			WriteReport("  	composite:   %d		- volume: %lf		- projected area: %lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			sprintf(temp,"composite: % d - volume : % lf - projected area : % lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			buf.Clear();
			str1 = temp;
			buf.Push(str1);
			arr.Push(buf);
		}
		WriteReport("  elemPartQuantities:      %d", elemPartQuantities.GetSize());
		for (UInt32 i = 0; i < elemPartQuantities.GetSize(); i++) {
			WriteReport("[%d,%d] - volume: %lf", elemPartQuantities[i].partId.main, elemPartQuantities[i].partId.sub, elemPartQuantities[i].quantity.roof.volume);
			sprintf(temp,"[%d,%d] - volume: %lf", elemPartQuantities[i].partId.main, elemPartQuantities[i].partId.sub, elemPartQuantities[i].quantity.roof.volume);
			buf.Clear();
			str1 = temp;
			buf.Push(str1);
			arr.Push(buf);
		}
		WriteReport("  elemPartComposites:      %d", elemPartComposites.GetSize());
		for (UInt32 i = 0; i < elemPartComposites.GetSize(); i++) {
			WriteReport("  	[%d,%d]", elemPartComposites[i].partId.main, elemPartComposites[i].partId.sub);
			for (UInt32 j = 0; j < elemPartComposites[i].composites.GetSize(); j++) {
				WriteReport("  		composite:   %d		- volume: %lf		- projected area: %lf", elemPartComposites[i].composites[j].buildMatIndices, elemPartComposites[i].composites[j].volumes, elemPartComposites[i].composites[j].projectedArea);
				sprintf(temp, "composite: %d - volume: %lf - projected area: %lf", elemPartComposites[i].composites[j].buildMatIndices, elemPartComposites[i].composites[j].volumes, elemPartComposites[i].composites[j].projectedArea);
				buf.Clear();
				str1 = temp;
				buf.Push(str1);
				arr.Push(buf);
			}
		}
		break;
	case API_ShellID:
		arr.Clear();
		WriteReport("  referenceSurface: %lf", quantity.shell.referenceSurface);
		buf.Clear();
		str1 = "referenceSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.referenceSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  oppositeSurface:    %lf", quantity.shell.oppositeSurface);
		buf.Clear();
		str1 = "oppositeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.oppositeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  edgeSurface:   %lf", quantity.shell.edgeSurface);
		buf.Clear();
		str1 = "edgeSurface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.edgeSurface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:        %lf", quantity.shell.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:     %lf", quantity.shell.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurf:     %lf", quantity.shell.holesSurf);
		buf.Clear();
		str1 = "holesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.holesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:      %lf", quantity.shell.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.shell.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		for (UInt32 i = 0; i < composites.GetSize(); i++) {
			WriteReport("  	composite:   %d		- volume: %lf		- projected area: %lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			sprintf(temp,"composite: %d	- volume: %lf - projected area: %lf", composites[i].buildMatIndices, composites[i].volumes, composites[i].projectedArea);
			buf.Clear();
			str1 = temp;
			buf.Push(temp);
			arr.Push(buf);
		}
		break;
	case API_MorphID:
		arr.Clear();
		WriteReport("  surface:  %lf", quantity.morph.surface);
		buf.Clear();
		str1 = "surface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.morph.surface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  volume:   %lf", quantity.morph.volume);
		buf.Clear();
		str1 = "volume";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.morph.volume);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  elemPartQuantities:      %d", elemPartQuantities.GetSize());
		for (UInt32 i = 0; i < elemPartQuantities.GetSize(); i++) {
			WriteReport("  	floor[%d]		- volume: %lf, floorPlanArea: %lf, baseLevel: %lf, baseHeight: %lf, wholeHeight: %lf",
				elemPartQuantities[i].partId.floor,
				elemPartQuantities[i].quantity.morph.volume,
				elemPartQuantities[i].quantity.morph.floorPlanArea,
				elemPartQuantities[i].quantity.morph.baseLevel,
				elemPartQuantities[i].quantity.morph.baseHeight,
				elemPartQuantities[i].quantity.morph.wholeHeight);
			sprintf(temp, " floor[%d] - volume: %lf, floorPlanArea: %lf, baseLevel: %lf, baseHeight: %lf, wholeHeight: %lf", 
				elemPartQuantities[i].partId.floor,
				elemPartQuantities[i].quantity.morph.volume,
				elemPartQuantities[i].quantity.morph.floorPlanArea,
				elemPartQuantities[i].quantity.morph.baseLevel,
				elemPartQuantities[i].quantity.morph.baseHeight,
				elemPartQuantities[i].quantity.morph.wholeHeight);
			buf.Clear();
			str1 = temp;
			buf.Push(str1);
			arr.Push(buf);
		}
		break;
	case API_HatchID:
		arr.Clear();
		WriteReport("  surface:    %lf", quantity.hatch.surface);
		buf.Clear();
		str1 = "surface";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.hatch.surface);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:  %lf", quantity.hatch.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.hatch.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:   %lf", quantity.hatch.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.hatch.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesSurf:  %lf", quantity.hatch.holesSurf);
		buf.Clear();
		str1 = "holesSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.hatch.holesSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	case API_ZoneID:
		arr.Clear();
		WriteReport("  area:           %lf", quantity.zone.area);
		buf.Clear();
		str1 = "area";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.area);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  perimeter:      %lf", quantity.zone.perimeter);
		buf.Clear();
		str1 = "perimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.perimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  holesPrm:       %lf", quantity.zone.holesPrm);
		buf.Clear();
		str1 = "holesPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.holesPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  wallsPrm:       %lf", quantity.zone.wallsPrm);
		buf.Clear();
		str1 = "wallsPrm";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.wallsPrm);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  allCorners:     %d", quantity.zone.allCorners);
		buf.Clear();
		str1 = "allCorners";
		buf.Push(str1);
		sprintf(temp, "%f", (double) quantity.zone.allCorners);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  concaveCorners: %d", quantity.zone.concaveCorners);
		buf.Clear();
		str1 = "concaveCorners";
		buf.Push(str1);
		sprintf(temp, "%f", (double) quantity.zone.concaveCorners);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  wallsSurf:      %lf", quantity.zone.wallsSurf);
		buf.Clear();
		str1 = "wallsSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.wallsSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  doorsWidth:     %lf", quantity.zone.doorsWidth);
		buf.Clear();
		str1 = "doorsWidth";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.doorsWidth);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  doorsSurf:      %lf", quantity.zone.doorsSurf);
		buf.Clear();
		str1 = "doorsSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.doorsSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  windowsWidth:   %lf", quantity.zone.windowsWidth);
		buf.Clear();
		str1 = "windowsWidth";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.windowsWidth);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  windowsSurf:    %lf", quantity.zone.windowsSurf);
		buf.Clear();
		str1 = "windowsSurf";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.windowsSurf);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  baseLevel:      %lf", quantity.zone.baseLevel);
		buf.Clear();
		str1 = "baseLevel";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.baseLevel);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  floorThick:     %lf", quantity.zone.floorThick);
		buf.Clear();
		str1 = "floorThick";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.floorThick);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  height:         %lf", quantity.zone.height);
		buf.Clear();
		str1 = "height";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.height);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  netarea:        %lf", quantity.zone.netarea);
		buf.Clear();
		str1 = "netarea";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.netarea);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		WriteReport("  netperimeter:   %lf", quantity.zone.netperimeter);
		buf.Clear();
		str1 = "netperimeter";
		buf.Push(str1);
		sprintf(temp, "%f", quantity.zone.netperimeter);
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
		break;
	default:
		ACAPI_WriteReport("  Please click a 3D element", true);
		break;
	}

	GS::Array<API_Neig>		selNeigs;
	GS::Array<API_Guid>		coverElemGuids;
	API_SelectionInfo		selectionInfo;
	err = ACAPI_Selection_Get(&selectionInfo, &selNeigs, false);
	for (const API_Neig& neig : selNeigs) {
		coverElemGuids.Push(neig.guid);
	}

	GS::Array <API_ElemPartSurfaceQuantity>		elemPartSurfaces;
	//elemGuids.Pop();	// We have the same item 3 times. Have no idea why.

	err = ACAPI_Element_GetSurfaceQuantities(&elemGuids, &coverElemGuids, &elemPartSurfaces);
	if (err != NoError) {
		ErrorBeep("ACAPI_Element_GetSurfaceQuantities", err);
		return arr;
	}

	WriteReport("---------- Exposed Surfaces ----------");
	sprintf(temp, "---------- Exposed Surfaces ----------");
	buf.Clear();
	str1 = temp;
	buf.Push(str1);
	arr.Push(buf);
	WriteReport("%s GUID : %s", ElemID_To_Name(typeID).ToCStr(CC_UTF8).Get(),
		APIGuidToString(element.header.guid).ToCStr().Get());
	    sprintf(temp,"% s GUID : % s", ElemID_To_Name(typeID).ToCStr(CC_UTF8).Get(),
			APIGuidToString(element.header.guid).ToCStr().Get());
		buf.Clear();
		str1 = temp;
		buf.Push(str1);
		arr.Push(buf);
	for (UInt32 i = 0; i < elemPartSurfaces.GetSize(); i++) {
		GS::UniString surfaceQuantityString;
		char temp1[1024] = {};
		surfaceQuantityString += "Surface " + GS::ValueToUniString(i);
		surfaceQuantityString += "  GUID: " + APIGuidToString(elemPartSurfaces[i].elemGUID);
		surfaceQuantityString += "  MaterialIndex: " + GS::ValueToUniString(elemPartSurfaces[i].materialIndex);
		surfaceQuantityString += "  BuildMatIndex: " + GS::ValueToUniString(elemPartSurfaces[i].buildMatIdx);
		surfaceQuantityString += "  ExposedSurface: " + GS::ValueToUniString(elemPartSurfaces[i].exposedSurface);
		WriteReport("%s", surfaceQuantityString.ToCStr().Get());
		sprintf(temp1, "%s", surfaceQuantityString.ToCStr().Get());
		buf.Clear();
		str1 = temp1;
		buf.Push(str1);
		arr.Push(buf);
	}
	WriteReport("---------- Exposed Surfaces ----------");

	return arr;
}		// Do_CalcQuantities


GS::Array<GS::Array<GS::UniString>>	Do_CalcComponentQuantities(GS::UniString selected)
{
	
	
	API_Guid		elemGuid;

	GS::Array<GS::Array<GS::UniString>> arr;
	GS::Array<GS::UniString> buf;
	GS::UniString str1 = "Failed";
	buf.Push(str1);
	arr.Push(buf);
	char temp[254] = {};

	elemGuid = APIGuidFromString(selected.ToCStr().Get());
	
	GS::Array<API_ElemComponentID> elemComponents;
	if (ACAPI_Element_GetComponents(elemGuid, elemComponents) != NoError) {
		WriteReport_Alert("Failed to get components of element");
		return arr;
	}

	for (const auto& elemComponent : elemComponents) {
		const GS::UniString componentGuid = APIGuid2GSGuid(elemComponent.componentID.componentGuid).ToUniString();
		WriteReport("Properties of component with guid %s", componentGuid.ToCStr().Get());

		GS::Array<API_PropertyDefinition> propertyDefinitions;
		if (ACAPI_ElemComponent_GetPropertyDefinitions(elemComponent, API_PropertyDefinitionFilter_All, propertyDefinitions) != NoError) {
			WriteReport_Alert("Failed to get property definitions for component with guid %s", componentGuid.ToCStr().Get());
			return arr;
		}

		GS::Array<API_Property> properties;
		if (ACAPI_ElemComponent_GetPropertyValues(elemComponent, propertyDefinitions, properties) != NoError) {
			WriteReport_Alert("Failed to get property values for component with guid %s", componentGuid.ToCStr().Get());
			return arr;
		}

		arr.Clear();

		for (const API_Property& property : properties) {
			API_PropertyGroup group = {};
			group.guid = property.definition.groupGuid;
			if (ACAPI_Property_GetPropertyGroup(group) != NoError) {
				continue;
			}

			GS::UniString propertyValue;
			const GSErrCode error = PropertyUtils::PropertyToString(property, propertyValue);
			if (error == NoError) {
				WriteReport("(%s) %s: %s", group.name.ToCStr().Get(), property.definition.name.ToCStr().Get(), propertyValue.ToCStr().Get());
				sprintf(temp,"(%s) %s: %s", group.name.ToCStr().Get(), property.definition.name.ToCStr().Get(), propertyValue.ToCStr().Get());
				str1 = temp;
				buf.Clear();
				buf.Push(str1);
				arr.Push(buf);
			}
		}
	}
	return arr;
}


// -----------------------------------------------------------------------------
// Get the components of the clicked element
// -----------------------------------------------------------------------------

GS::Array<GS::Array<GS::UniString>>	Do_GetComponents(GS::UniString selected)
{
	
	
	API_Guid		elemGuid;

	GS::Array<GS::Array<GS::UniString>> arr;
	GS::Array<GS::UniString> buf;
	GS::UniString str1;
	str1 = "Failed";
	buf.Push(str1);
	arr.Push(buf);
	char temp[254] = {};

	elemGuid = APIGuidFromString(selected.ToCStr().Get());
	GS::Array<API_ElemComponentID> elemComponents;
	if (ACAPI_Element_GetComponents(elemGuid, elemComponents) != NoError) {
		WriteReport_Alert("Failed to get components of element");
		return arr;
	}

	WriteReport("Number of components: %d", elemComponents.GetSize());
	arr.Clear();
	WriteReport("Guids of components:");
	for (const auto& elemComponent : elemComponents) {
		WriteReport(APIGuidToString(elemComponent.componentID.componentGuid).ToCStr().Get());
		sprintf(temp,APIGuidToString(elemComponent.componentID.componentGuid).ToCStr().Get());
		str1 = temp;
		buf.Clear();
		buf.Push(str1);
		arr.Push(buf);
	}
	return arr;
}		// Do_GetComponents