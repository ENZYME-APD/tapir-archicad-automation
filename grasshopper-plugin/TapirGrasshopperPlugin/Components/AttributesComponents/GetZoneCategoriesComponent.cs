using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetZoneCategoriesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetZoneCategories";

        public GetZoneCategoriesComponent()
            : base(
                "GetZoneCategories",
                "Get the details of the given Zone Category attributes.")
        {
        }

        protected override string ResponseArrayKey => "zoneCategories";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each zone category attribute.");

            OutIntegers(
                "Indices",
                "Index of each zone category attribute.");

            OutTexts(
                "Names",
                "Name of each zone category attribute.");

            OutTexts(
                "CategoryCodes",
                "Category code of each zone category.");

            OutColors(
                "Colors",
                "Color of each zone category.");

            OutTexts(
                "StampNames",
                "Zone stamp library part name of each zone category.");

            OutTexts(
                "StampMainGuids",
                "Main guid of the zone stamp library part.");

            OutTexts(
                "StampRevGuids",
                "Revision guid of the zone stamp library part.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var guids = new List<object>();
            var indices = new List<object>();
            var names = new List<object>();
            var categoryCodes = new List<object>();
            var colors = new List<object>();
            var stampNames = new List<object>();
            var stampMainGuids = new List<object>();
            var stampRevGuids = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    categoryCodes.Add(null);
                    colors.Add(null);
                    stampNames.Add(null);
                    stampMainGuids.Add(null);
                    stampRevGuids.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                categoryCodes.Add(JsonOutputHelp.Scalar(item, "categoryCode"));
                colors.Add(JsonOutputHelp.ColorOf(item["color"]));
                stampNames.Add(JsonOutputHelp.Scalar(item, "stampName"));
                stampMainGuids.Add(JsonOutputHelp.Scalar(item, "stampMainGuid"));
                stampRevGuids.Add(JsonOutputHelp.Scalar(item, "stampRevGuid"));
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, categoryCodes);
            da.SetDataList(4, colors);
            da.SetDataList(5, stampNames);
            da.SetDataList(6, stampMainGuids);
            da.SetDataList(7, stampRevGuids);
            da.SetDataList(8, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetZoneCategories;

        public override Guid ComponentGuid =>
            new Guid("7886626e-e76e-4d2a-a84d-75e49c34fee3");
    }
}
