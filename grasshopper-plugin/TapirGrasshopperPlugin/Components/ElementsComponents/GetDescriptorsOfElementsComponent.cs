using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetDescriptorsOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetDescriptorsOfElements";

        public GetDescriptorsOfElementsComponent()
            : base(
                "GetDescriptorsOfElements",
                "Gets the descriptors of the given elements: the listing descriptors in the property objects " +
                "of each element, with the library part that declares them (index, name and guid), their " +
                "listing database set and index, and their status (Normal, Deleted or Local). The output is " +
                "aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "descriptorsOfElements";

        protected override void AddOutputs()
        {
            OutIntegerTree(
                "LibraryPartIndices",
                "The index of the library part that declares each descriptor (one branch per element).");

            OutTextTree(
                "Names",
                "The name of the library part that declares each descriptor (one branch per element).");

            OutTextTree(
                "LibraryPartGuids",
                "The guid of the library part that declares each descriptor (one branch per element).");

            OutIntegerTree(
                "SetIndices",
                "The listing database set of each descriptor (one branch per element).");

            OutIntegerTree(
                "Indices",
                "The index of each descriptor in its listing database (one branch per element).");

            OutTextTree(
                "Statuses",
                "The status of each descriptor: Normal, Deleted, Local or Unknown (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var keys = new[]
            {
                "libraryPartIndex",
                "name",
                "guid",
                "setIndex",
                "index",
                "status"
            };
            var trees = new DataTree<object>[keys.Length];
            for (var k = 0; k < keys.Length; k++)
            {
                trees[k] = new DataTree<object>();
            }
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                foreach (var tree in trees)
                {
                    tree.EnsurePath(path);
                }

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["descriptors"] is JArray descriptors))
                {
                    continue;
                }

                foreach (var descriptor in descriptors)
                {
                    for (var k = 0; k < keys.Length; k++)
                    {
                        trees[k].Add(
                            JsonOutputHelp.Scalar(
                                descriptor,
                                keys[k]),
                            path);
                    }
                }
            }

            for (var k = 0; k < trees.Length; k++)
            {
                da.SetDataTree(
                    k,
                    trees[k]);
            }
            da.SetDataList(
                trees.Length,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDescriptorsOfElements;

        public override Guid ComponentGuid =>
            new Guid("54cfc1db-7f26-4103-a54c-1dcac88671e9");
    }
}
