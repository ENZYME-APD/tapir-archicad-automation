using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
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
                "Identifiers of the navigator items to delete.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Json" + nameof(ViewSettings),
                "JSON object of the retrieved view settings.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            //if (!NavigatorItemIdsObj.TryCreate(
            //        da,
            //        0,
            //        out NavigatorItemIdsObj input))
            //{
            //    return;
            //}

            if (!da.TryGetItems(
                    0,
                    out List<string> ids))
            {
                return;
            }

            var main = new NavItemInputStructure();
            main.NavItemIds = new NavItemIds();

            foreach (var id in ids)
            {
                var arrayItem = new NavItemIdArrayItem
                {
                    NavItemId = new NavItemId { Guid = id }
                };
                main.NavItemIds.Add(arrayItem);
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    main,
                    SendToAddOn,
                    ViewSettingsResponse.FromResponse,
                    out ViewSettingsResponse response))
            {
                return;
            }

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