using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetViewSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetViewSettings";

        public GetViewSettingsComponent()
            : base(
                nameof(ViewSettings),
                "Gets the view settings of navigator items.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifiers of the navigator items to delete.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Json" + nameof(ViewSettings),
                "JSON object of the retrieved view settings.");

            OutTexts(nameof(ViewSettings.ModelViewOptions));
            OutTexts(nameof(ViewSettings.LayerCombination) + "s");
            OutTexts(nameof(ViewSettings.DimensionStyle) + "s");
            OutTexts(nameof(ViewSettings.PenSetName) + "s");
            OutTexts(nameof(ViewSettings.GraphicOverrideCombination) + "s");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ViewSettingsData.FromResponse,
                    out ViewSettingsData response))
            {
                return;
            }

            da.SetData(
                0,
                JsonConvert.SerializeObject(
                    response,
                    Formatting.Indented));

            da.SetDataList(
                1,
                response.ViewSettings.Select(x =>
                    ((ViewSettings)x).ModelViewOptions));

            da.SetDataList(
                2,
                response.ViewSettings.Select(x =>
                    ((ViewSettings)x).LayerCombination));

            da.SetDataList(
                3,
                response.ViewSettings.Select(x =>
                    ((ViewSettings)x).DimensionStyle));

            da.SetDataList(
                4,
                response.ViewSettings.Select(x =>
                    ((ViewSettings)x).PenSetName));

            da.SetDataList(
                5,
                response.ViewSettings.Select(x =>
                    ((ViewSettings)x).GraphicOverrideCombination));
        }

        public override Guid ComponentGuid =>
            new Guid("a0028d54-cab5-4427-9cb7-8b3ef1bb8a49");
    }
}