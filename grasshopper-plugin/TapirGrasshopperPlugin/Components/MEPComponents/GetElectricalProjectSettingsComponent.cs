using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetElectricalProjectSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElectricalProjectSettings";

        public GetElectricalProjectSettingsComponent()
            : base(
                "GetElectricalProjectSettings",
                "Get the electrical project settings: the default voltage, the wiring types with their " +
                "cross-sectional area and the load classifications with their demand factor. " +
                "Available from Archicad 30.",
                GroupNames.MEP)
        {
        }

        protected override void AddOutputs()
        {
            outManager.AddNumberParameter(
                "Voltage",
                "Voltage",
                "Default voltage of the project.".WithTypeName("numberItem"),
                GH_ParamAccess.item);

            OutTexts(
                "WiringTypeNames",
                "Name of each wiring type.");

            OutNumberList(
                "CrossSectionalAreas",
                "Cross-sectional area of the wire of each wiring type in square millimeters.");

            OutTexts(
                "LoadClassificationNames",
                "Name of each load classification.");

            OutNumberList(
                "DemandFactors",
                "Demand factor of each load classification, between 0 and 1.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response) ||
                ElectricalComponentHelp.ReportResponseError(this, response))
            {
                return;
            }

            var wiringTypeNames = new List<object>();
            var crossSectionalAreas = new List<object>();
            foreach (var wiringType in JsonOutputHelp.Items(response, "wiringTypes"))
            {
                wiringTypeNames.Add(JsonOutputHelp.Scalar(wiringType, "name"));
                crossSectionalAreas.Add(JsonOutputHelp.Scalar(wiringType, "crossSectionalArea"));
            }

            var loadClassificationNames = new List<object>();
            var demandFactors = new List<object>();
            foreach (var loadClassification in JsonOutputHelp.Items(response, "loadClassifications"))
            {
                loadClassificationNames.Add(JsonOutputHelp.Scalar(loadClassification, "name"));
                demandFactors.Add(JsonOutputHelp.Scalar(loadClassification, "demandFactor"));
            }

            da.SetData(0, JsonOutputHelp.Scalar(response, "voltage"));
            da.SetDataList(1, wiringTypeNames);
            da.SetDataList(2, crossSectionalAreas);
            da.SetDataList(3, loadClassificationNames);
            da.SetDataList(4, demandFactors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElectricalProjectSettings;

        public override Guid ComponentGuid =>
            new Guid("f4bdd70d-19cf-4aea-845e-9c7996c80a91");
    }
}
