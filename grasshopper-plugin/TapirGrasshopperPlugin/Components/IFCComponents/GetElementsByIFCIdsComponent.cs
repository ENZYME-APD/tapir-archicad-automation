using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetElementsByIFCIdsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementsByIFCIds";

        public GetElementsByIFCIdsComponent()
            : base(
                "GetElementsByIFCIds",
                "Retrieve the elements by the given IFC identifiers.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "IFCIds",
                "IFC identifiers to get the corresponding elements for.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Elements",
                "JSON object with the elements corresponding to the given IFC identifiers.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> ifcIds))
            {
                return;
            }

            var input = new GetElementsByIFCIdsParameters { IfcIds = ifcIds };

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementsByIFCIds;

        public override Guid ComponentGuid =>
            new Guid("96698d90-7896-4bd4-8e2e-de53363d2e12");
    }
}
