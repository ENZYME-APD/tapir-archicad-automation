using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetBuildingMaterialPhysicalPropertiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetBuildingMaterialPhysicalProperties";

        public GetBuildingMaterialPhysicalPropertiesComponent()
            : base(
                "GetBuildingMaterialPhysicalProperties",
                "Get the physical properties of the given Building Materials.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the Building Material attributes.");
        }

        protected override void AddOutputs()
        {
            outManager.AddNumberParameter(
                "ThermalConductivity", "ThermalConductivity",
                "Thermal conductivity of each Building Material.", GH_ParamAccess.list);

            outManager.AddNumberParameter(
                "Density", "Density",
                "Density of each Building Material.", GH_ParamAccess.list);

            outManager.AddNumberParameter(
                "HeatCapacity", "HeatCapacity",
                "Heat capacity of each Building Material.", GH_ParamAccess.list);

            outManager.AddNumberParameter(
                "EmbodiedEnergy", "EmbodiedEnergy",
                "Embodied energy of each Building Material.", GH_ParamAccess.list);

            outManager.AddNumberParameter(
                "EmbodiedCarbon", "EmbodiedCarbon",
                "Embodied carbon of each Building Material.", GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AttributeGuidItemsObject input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    JHelp.Deserialize<GetBuildingMaterialPhysicalPropertiesResponse>,
                    out GetBuildingMaterialPhysicalPropertiesResponse response))
            {
                return;
            }

            var items = response.Properties ?? new List<BuildingMaterialPhysicalPropertiesItem>();
            var props = items.Select(i => i.Properties).ToList();

            da.SetDataList(0, props.Select(p => p?.ThermalConductivity ?? 0.0));
            da.SetDataList(1, props.Select(p => p?.Density ?? 0.0));
            da.SetDataList(2, props.Select(p => p?.HeatCapacity ?? 0.0));
            da.SetDataList(3, props.Select(p => p?.EmbodiedEnergy ?? 0.0));
            da.SetDataList(4, props.Select(p => p?.EmbodiedCarbon ?? 0.0));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBuildingMaterialPhysicalProperties;

        public override Guid ComponentGuid =>
            new Guid("516b4328-e06c-4805-82cd-49ee78497b89");
    }
}
