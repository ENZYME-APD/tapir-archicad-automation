using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetHierarchicalElementOwnersComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetHierarchicalElementOwners";

        public GetHierarchicalElementOwnersComponent()
            : base(
                "GetHierarchicalElementOwners",
                "Gets the owner of each given element in its hierarchical element: the root (e.g. the Curtain " +
                "Wall, Stair or Railing of a frame, tread or post) or the immediate parent. Also returns " +
                "whether the given element is a main element, a subelement or not part of a hierarchical " +
                "element.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "owners";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements whose owners are looked up, e.g. Curtain Wall frames or panels, Stair treads " +
                "or Railing posts.");

            InText(
                "OwnerMode",
                "Optional. root (default): the root of the hierarchical element tree, e.g. the Curtain " +
                "Wall, Stair or Railing. parent: the immediate parent, which may itself be a subelement.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "OwnerElementGuids",
                "The owner of each element. For a main element or an element that is not part of a " +
                "hierarchical element, Archicad returns the input element itself.");

            OutTexts(
                "HierarchicalElementTypes",
                "The classification of each input element (not of its owner): MainElement, ChildElement " +
                "or SingleElement.");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);
            string ownerMode = null;
            if (da.GetData(
                    1,
                    ref ownerMode) &&
                !string.IsNullOrEmpty(ownerMode))
            {
                parameters["ownerMode"] = ownerMode;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    ResponseArrayKey));
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var owners = new List<object>();
            var types = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                owners.Add(
                    ElementIdOf(
                        item,
                        "ownerElementId"));
                types.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "hierarchicalElementType"));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(
                0,
                owners);
            da.SetDataList(
                1,
                types);
            da.SetDataList(
                2,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetHierarchicalElementOwners;

        public override Guid ComponentGuid =>
            new Guid("9e74c490-4d70-4347-b9b3-525f18f550ae");
    }
}
