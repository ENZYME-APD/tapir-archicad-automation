using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetPropertyObjectsOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetPropertyObjectsOfElements";

        public GetPropertyObjectsOfElementsComponent()
            : base(
                "GetPropertyObjectsOfElements",
                "Gets the property objects of the given elements: the library parts of the old listing system " +
                "(used by projects migrated from older Archicad versions) linked to each element by criteria " +
                "or individually, with the library part's index (the same as the index returned by " +
                "GetAvailableLibraryParts), name and guid. The output is aligned with the input list of " +
                "elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "propertyObjectsOfElements";

        protected override void AddOutputs()
        {
            OutIntegerTree(
                "LibraryPartIndices",
                "The library part index of each property object (one branch per element).");

            OutTextTree(
                "Names",
                "The name of each property object (one branch per element).");

            OutTextTree(
                "LibraryPartGuids",
                "The library part guid of each property object (one branch per element).");

            OutBooleanTree(
                "IsIndividuallyAssigned",
                "Whether each property object is assigned to the element individually rather than by " +
                "criteria (one branch per element).");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var indices = new DataTree<object>();
            var names = new DataTree<object>();
            var guids = new DataTree<object>();
            var isIndividuallyAssigned = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                indices.EnsurePath(path);
                names.EnsurePath(path);
                guids.EnsurePath(path);
                isIndividuallyAssigned.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["propertyObjects"] is JArray propertyObjects))
                {
                    continue;
                }

                foreach (var propertyObject in propertyObjects)
                {
                    indices.Add(
                        JsonOutputHelp.Scalar(
                            propertyObject,
                            "libraryPartIndex"),
                        path);
                    names.Add(
                        JsonOutputHelp.Scalar(
                            propertyObject,
                            "name"),
                        path);
                    guids.Add(
                        JsonOutputHelp.Scalar(
                            propertyObject,
                            "guid"),
                        path);
                    isIndividuallyAssigned.Add(
                        JsonOutputHelp.Scalar(
                            propertyObject,
                            "isIndividuallyAssigned"),
                        path);
                }
            }

            da.SetDataTree(
                0,
                indices);
            da.SetDataTree(
                1,
                names);
            da.SetDataTree(
                2,
                guids);
            da.SetDataTree(
                3,
                isIndividuallyAssigned);
            da.SetDataList(
                4,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyObjectsOfElements;

        public override Guid ComponentGuid =>
            new Guid("4bacfe27-78f3-4aaf-a008-bfbfec6736c9");
    }
}
