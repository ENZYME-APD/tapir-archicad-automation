using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

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
                "Identifier of navigator items to delete.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Json" + nameof(ViewSettings),
                "");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var ids = NavigatorItemIdsObj.Create(
                da,
                0);

            if (ids == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input NavigatorItemIds failed to collect data.");
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
                    ids,
                    out JObject jResponse))
            {
                return;
            }

            var response = ViewSettingsResponse.FromResponse(jResponse);

            da.SetData(
                0,
                JsonConvert.SerializeObject(
                    response,
                    Formatting.Indented));
        }

        public override Guid ComponentGuid =>
            new Guid("a0028d54-cab5-4427-9cb7-8b3ef1bb8a49");
    }
}