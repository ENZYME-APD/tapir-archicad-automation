using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPElements";

        public GetMEPElementsComponent()
            : base(
                "GetMEPElements",
                "Get the MEP elements of the project, optionally filtered by type and domain. " +
                "Available from Archicad 28.",
                GroupNames.MEP)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "TypeFilter",
                "Optional filter for the MEP element types: RoutingElement, RigidSegment, Elbow, Transition, " +
                "Branch, Terminal, Accessory, Equipment, Fitting, FlexibleSegment or TakeOff.");

            InTexts(
                "DomainFilter",
                "Optional filter for the MEP domains: Ventilation, Piping or CableCarrier.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each MEP element.");

            OutTexts(
                "Types",
                "Type of each MEP element.");

            OutTexts(
                "Domains",
                "MEP domain of each element (empty for domain-independent elements).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            da.TryGetList(0, out List<string> typeFilter);
            if (typeFilter != null &&
                typeFilter.Count > 0)
            {
                parameters["elementTypes"] = new JArray(typeFilter);
            }

            da.TryGetList(1, out List<string> domainFilter);
            if (domainFilter != null &&
                domainFilter.Count > 0)
            {
                parameters["domains"] = new JArray(domainFilter);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new List<object>();
            var types = new List<object>();
            var domains = new List<object>();

            if (response["elements"] is JArray elements)
            {
                foreach (var element in elements)
                {
                    var guid = element["elementId"]?["guid"]?.ToString();
                    elementGuids.Add(
                        guid == null
                            ? null
                            : new ElementGuidWrapper
                            {
                                ElementId = new ElementGuid { Guid = guid }
                            });
                    types.Add(element["type"]?.ToString());
                    domains.Add(element["domain"]?.ToString());
                }
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, types);
            da.SetDataList(2, domains);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPElements;

        public override Guid ComponentGuid =>
            new Guid("70442957-b7c5-474f-9948-352d1a30c2f9");
    }
}
