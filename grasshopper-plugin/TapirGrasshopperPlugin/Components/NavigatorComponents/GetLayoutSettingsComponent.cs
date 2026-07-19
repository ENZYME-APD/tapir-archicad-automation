using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetLayoutSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLayoutSettings";

        public GetLayoutSettingsComponent()
            : base(
                "GetLayoutSettings",
                "Get settings of layouts, including Layout Info Panel custom data fields. " +
                "Identify each layout either by its database or by its navigator item.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LayoutDatabaseGuids",
                "Identifiers of the layout databases to query.");

            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the layout navigator items to query.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "LayoutSettings",
                "JSON object with the settings of the given layouts.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var items = new JArray();

            if (da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> dbWrappers))
            {
                foreach (var wrapper in dbWrappers)
                {
                    var id = GuidObject<DatabaseGuidObject>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid database identifier in the LayoutDatabaseGuids input.");
                        return;
                    }
                    items.Add(
                        new JObject
                        {
                            ["databaseId"] = new JObject { ["guid"] = id.Guid }
                        });
                }
            }

            if (da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> navWrappers))
            {
                foreach (var wrapper in navWrappers)
                {
                    var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid navigator item identifier in the NavigatorItemGuids input.");
                        return;
                    }
                    items.Add(
                        new JObject
                        {
                            ["navigatorItemId"] = new JObject { ["guid"] = id.Guid }
                        });
                }
            }

            if (items.Count == 0)
            {
                this.AddError(
                    "Provide at least one LayoutDatabaseGuid or NavigatorItemGuid.");
                return;
            }

            var parameters = new JObject { ["layoutDatabaseIds"] = items };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
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
            Properties.Resources.GetLayoutSettings;

        public override Guid ComponentGuid =>
            new Guid("305c2b76-1b7d-448e-9cd1-d4653dbbaa8b");
    }
}
