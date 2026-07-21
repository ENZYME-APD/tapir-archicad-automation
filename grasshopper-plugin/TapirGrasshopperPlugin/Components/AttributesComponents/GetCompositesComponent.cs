using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetCompositesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetComposites";

        public GetCompositesComponent()
            : base(
                "GetComposites",
                "Get the details of the given Composite attributes.")
        {
        }

        protected override string ResponseArrayKey => "composites";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each composite attribute.");

            OutIntegers(
                "Indices",
                "Index of each composite attribute.");

            OutTexts(
                "Names",
                "Name of each composite attribute.");

            OutTextTree(
                "UseWith",
                "Element types the composite can be used with (one branch per attribute).");

            OutTextTree(
                "SkinTypes",
                "Type of each skin: Core, Finish or Other (one branch per attribute).");

            OutGenericTree(
                "SkinBuildingMaterialGuids",
                "Building material identifier of each skin (one branch per attribute).");

            OutIntegerTree(
                "SkinFramePens",
                "Frame pen of each skin (one branch per attribute).");

            OutNumbers(
                "SkinThicknesses",
                "Thickness of each skin (one branch per attribute).");

            OutGenericTree(
                "SeparatorLineTypeGuids",
                "Line type identifier of each skin separator (one branch per attribute).");

            OutIntegerTree(
                "SeparatorLinePens",
                "Line pen of each skin separator (one branch per attribute).");

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
            var useWith = new DataTree<object>();
            var skinTypes = new DataTree<object>();
            var skinBuildingMaterialGuids = new DataTree<object>();
            var skinFramePens = new DataTree<object>();
            var skinThicknesses = new DataTree<object>();
            var separatorLineTypeGuids = new DataTree<object>();
            var separatorLinePens = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                useWith.EnsurePath(path);
                skinTypes.EnsurePath(path);
                skinBuildingMaterialGuids.EnsurePath(path);
                skinFramePens.EnsurePath(path);
                skinThicknesses.EnsurePath(path);
                separatorLineTypeGuids.EnsurePath(path);
                separatorLinePens.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));

                if (item["useWith"] is JArray useWithArray)
                {
                    foreach (var elementType in useWithArray)
                    {
                        useWith.Add(
                            elementType.ToString(),
                            path);
                    }
                }

                if (item["skins"] is JArray skins)
                {
                    foreach (var skin in skins)
                    {
                        skinTypes.Add(
                            JsonOutputHelp.Scalar(skin, "type"),
                            path);
                        skinBuildingMaterialGuids.Add(
                            WrappedAttributeIdOf(skin["buildingMaterialId"]),
                            path);
                        skinFramePens.Add(
                            JsonOutputHelp.Scalar(skin, "framePen"),
                            path);
                        skinThicknesses.Add(
                            JsonOutputHelp.Scalar(skin, "thickness"),
                            path);
                    }
                }

                if (item["separators"] is JArray separators)
                {
                    foreach (var separator in separators)
                    {
                        separatorLineTypeGuids.Add(
                            WrappedAttributeIdOf(separator["lineTypeId"]),
                            path);
                        separatorLinePens.Add(
                            JsonOutputHelp.Scalar(separator, "linePen"),
                            path);
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataTree(3, useWith);
            da.SetDataTree(4, skinTypes);
            da.SetDataTree(5, skinBuildingMaterialGuids);
            da.SetDataTree(6, skinFramePens);
            da.SetDataTree(7, skinThicknesses);
            da.SetDataTree(8, separatorLineTypeGuids);
            da.SetDataTree(9, separatorLinePens);
            da.SetDataList(10, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetComposites;

        public override Guid ComponentGuid =>
            new Guid("a2c4d80f-81c1-47cd-9e04-2bf5f92eb7d7");
    }
}
