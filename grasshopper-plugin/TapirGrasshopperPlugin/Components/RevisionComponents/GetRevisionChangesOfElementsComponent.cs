using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetRevisionChangesOfElementsComponent : RevisionChangesOfEntitiesComponent
    {
        public override string CommandName => "GetRevisionChangesOfElements";

        public GetRevisionChangesOfElementsComponent()
            : base(
                "GetRevisionChangesOfElements",
                "Retrieve the revision changes belonging to the given elements.")
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
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

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetChangeOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    "revisionChangesOfElements"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetRevisionChangesOfElements;

        public override Guid ComponentGuid =>
            new Guid("14da2b8e-a89e-4b0c-9460-2d8199517c90");
    }
}
