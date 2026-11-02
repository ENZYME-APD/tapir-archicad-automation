using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetZoneLabelingSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetZoneLabelingSettings";

        public GetZoneLabelingSettingsComponent()
            : base(
                "GetZoneLabelingSettings",
                "Returns whether the automatic labeling of Zones is on and the name of the Label Favorite it " +
                "places. Available from Archicad 28.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsAutomaticLabelingOn",
                "Whether the automatic labeling of Zones (placing a Label on each new Zone) is on.");

            OutText(
                "SelectedFavoriteName",
                "Name of the Label Favorite placed on each new Zone; empty when no Favorite is selected.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                response.Value<bool?>("isAutomaticLabelingOn"));
            da.SetData(
                1,
                response["selectedFavoriteName"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetZoneLabelingSettings;

        public override Guid ComponentGuid =>
            new Guid("5f447af9-25b4-4030-97b5-b0a03476dc44");
    }
}
