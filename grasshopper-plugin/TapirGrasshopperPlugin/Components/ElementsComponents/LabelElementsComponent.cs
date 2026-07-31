using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LabelElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLabels";

        public LabelElementsComponent()
            : base(
                "LabelElements",
                "Label elements",
                GroupNames.ElementCreation)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "IDs of Elements to create labels for.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "LabelGuids",
                "Identifiers of the created labels (null for failed items).");

            OutTexts(
                "ErrorMessages",
                "Error message for each item (empty when the label was created successfully).");
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

            var parameters = JObject.FromObject(
                new
                {
                    labelsData = elements
                        .Elements.Select(element =>
                            new { parentElementId = element.ElementId })
                        .ToList()
                });

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            CreateElementsComponentBase.SetCreatedElementsOutputs(da, response, 0, 1);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.LabelElements;

        public override Guid ComponentGuid =>
            new Guid("ecdb0a59-f928-4ed3-88e1-cd9aea737b39");
    }
}
