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
        public static string Doc =>
            "Gets the view settings of navigator items.";

        public override string CommandName => "GetViewSettings";

        public GetViewSettingsComponent()
            : base(
                nameof(ViewSettings),
                nameof(ViewSettings),
                Doc,
                GroupNames.Navigator)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "NavigatorItemIds",
                "NavigatorItemIds",
                "Identifier of navigator items to delete.",
                GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Json" + nameof(ViewSettings),
                "",
                "",
                GH_ParamAccess.item);
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