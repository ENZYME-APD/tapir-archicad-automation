using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateViewsInViewMapComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateViewsInViewMap";

        public CreateViewsInViewMapComponent()
            : base(
                "CreateViewsInViewMap",
                "Create independent (non-clone) navigator views in the View Map by copying database and settings from source items.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the source navigator items whose database and settings are copied.");

            InGeneric(
                "ParentNavigatorItemGuid",
                "Identifier of the View Map folder to place the new views in. Optional; defaults to the View Map root.");

            InTexts(
                "Names",
                "Names for the new views (input only 1 to use the same name for all). Optional; defaults to the source item names.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> navWrappers))
            {
                return;
            }

            NavigatorGuid parentId = null;
            if (da.TryGet(
                    1,
                    out GH_ObjectWrapper parentWrapper) &&
                parentWrapper?.Value != null)
            {
                parentId = GuidObject<NavigatorGuid>.CreateFromWrapper(parentWrapper);
                if (parentId == null)
                {
                    this.AddError("Invalid ParentNavigatorItemGuid.");
                    return;
                }
            }

            da.TryGetList(
                2,
                out List<string> names);
            names = names ?? new List<string>();

            if (names.Count > 1 &&
                names.Count != navWrappers.Count)
            {
                this.AddError(
                    "The size of the input Names must be 0, 1 or equal to the size of the input NavigatorItemGuids.");
                return;
            }

            var items = new JArray();
            for (var i = 0; i < navWrappers.Count; i++)
            {
                var navId = GuidObject<NavigatorGuid>.CreateFromWrapper(navWrappers[i]);
                if (navId == null)
                {
                    this.AddError(
                        "Invalid navigator item identifier in the NavigatorItemGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["navigatorItemId"] = new JObject { ["guid"] = navId.Guid }
                };

                if (parentId != null)
                {
                    item["parentNavigatorItemId"] = new JObject { ["guid"] = parentId.Guid };
                }

                if (names.Count > 0)
                {
                    item["name"] = names[names.Count == 1 ? 0 : i];
                }

                items.Add(item);
            }

            var parameters = new JObject { ["viewsData"] = items };

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateViewsInViewMap;

        public override Guid ComponentGuid =>
            new Guid("9567b8cc-7510-4638-afee-f075bccc9a33");
    }
}
