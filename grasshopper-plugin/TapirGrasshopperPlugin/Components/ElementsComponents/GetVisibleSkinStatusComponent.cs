using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetVisibleSkinStatusComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetVisibleSkinStatus";

        public GetVisibleSkinStatusComponent()
            : base(
                "GetVisibleSkinStatus",
                "Checks whether elements have a visible skin in a given Partial Structure Display mode " +
                "(EntireStructure, CoreOnly, WithoutFinishes or StructureOnly), e.g. a finish-only slab has " +
                "none in CoreOnly. The current window is not changed. The output is aligned with the input " +
                "list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to check.");

            InTextWithDefault(
                "StructureDisplay",
                "The Partial Structure Display mode: EntireStructure, CoreOnly, WithoutFinishes or " +
                "StructureOnly.",
                "EntireStructure");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "HasVisibleSkin",
                "Whether each element has a visible skin in the given mode.");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements) ||
                !da.TryGet(
                    1,
                    out string structureDisplay))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);
            parameters["structureDisplay"] = structureDisplay;

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var hasVisibleSkin = new List<object>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "visibleSkinStatuses"))
            {
                hasVisibleSkin.Add(
                    JsonOutputHelp.Scalar(
                        item,
                        "hasVisibleSkin"));
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(
                0,
                hasVisibleSkin);
            da.SetDataList(
                1,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetVisibleSkinStatus;

        public override Guid ComponentGuid =>
            new Guid("19166a28-896c-412a-a242-0694c03118e4");
    }
}
