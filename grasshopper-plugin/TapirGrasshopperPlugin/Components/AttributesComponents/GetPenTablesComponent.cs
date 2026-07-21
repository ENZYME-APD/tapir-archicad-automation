using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetPenTablesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetPenTables";

        public GetPenTablesComponent()
            : base(
                "GetPenTables",
                "Get the details of the given Pen Table attributes.")
        {
        }

        protected override string ResponseArrayKey => "penTables";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each pen table attribute.");

            OutIntegers(
                "Indices",
                "Index of each pen table attribute.");

            OutTexts(
                "Names",
                "Name of each pen table attribute.");

            OutBooleans(
                "IsActiveForModel",
                "True if the pen table is active for the model.");

            OutBooleans(
                "IsActiveForLayout",
                "True if the pen table is active for layouts.");

            OutIntegerTree(
                "PenIndices",
                "Index of each pen (one branch per pen table).");

            OutColorTree(
                "PenColors",
                "Color of each pen (one branch per pen table).");

            OutNumbers(
                "PenWidths",
                "Width of each pen (one branch per pen table).");

            OutTextTree(
                "PenDescriptions",
                "Description of each pen (one branch per pen table).");

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
            var isActiveForModel = new List<object>();
            var isActiveForLayout = new List<object>();
            var penIndices = new DataTree<object>();
            var penColors = new DataTree<object>();
            var penWidths = new DataTree<object>();
            var penDescriptions = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                penIndices.EnsurePath(path);
                penColors.EnsurePath(path);
                penWidths.EnsurePath(path);
                penDescriptions.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    isActiveForModel.Add(null);
                    isActiveForLayout.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                isActiveForModel.Add(JsonOutputHelp.Scalar(item, "isActiveForModel"));
                isActiveForLayout.Add(JsonOutputHelp.Scalar(item, "isActiveForLayout"));

                if (item["pens"] is JArray pens)
                {
                    foreach (var pen in pens)
                    {
                        penIndices.Add(
                            JsonOutputHelp.Scalar(pen, "index"),
                            path);
                        penColors.Add(
                            JsonOutputHelp.ColorOf(pen["color"]),
                            path);
                        penWidths.Add(
                            JsonOutputHelp.Scalar(pen, "width"),
                            path);
                        penDescriptions.Add(
                            JsonOutputHelp.Scalar(pen, "description"),
                            path);
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, isActiveForModel);
            da.SetDataList(4, isActiveForLayout);
            da.SetDataTree(5, penIndices);
            da.SetDataTree(6, penColors);
            da.SetDataTree(7, penWidths);
            da.SetDataTree(8, penDescriptions);
            da.SetDataList(9, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPenTables;

        public override Guid ComponentGuid =>
            new Guid("0e011975-ca64-496f-b9ba-f223adf1d2f9");
    }
}
