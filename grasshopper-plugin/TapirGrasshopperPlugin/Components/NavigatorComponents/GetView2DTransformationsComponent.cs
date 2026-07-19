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
    public class GetView2DTransformationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetView2DTransformations";

        public GetView2DTransformationsComponent()
            : base(
                "GetView2DTransformations",
                "Get the zoom and rotation of 2D views (given by navigator items and/or databases).",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the navigator items to query.");

            InGenerics(
                "DatabaseGuids",
                "Identifiers of the databases to query.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "Transformations",
                "JSON object with the 2D zoom and rotation of the given views.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> navWrappers) &&
                navWrappers.Count > 0)
            {
                var navIds = new List<NavigatorGuidWrapper>();
                foreach (var wrapper in navWrappers)
                {
                    var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid navigator item identifier.");
                        return;
                    }
                    navIds.Add(new NavigatorGuidWrapper { NavigatorId = id });
                }
                parameters["navigatorItemIds"] = JArray.FromObject(navIds);
            }

            if (da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> dbWrappers) &&
                dbWrappers.Count > 0)
            {
                var dbIds = new List<DatabaseGuidWrapper>();
                foreach (var wrapper in dbWrappers)
                {
                    var id = GuidObject<DatabaseGuidObject>.CreateFromWrapper(wrapper);
                    if (id == null)
                    {
                        this.AddError("Invalid database identifier.");
                        return;
                    }
                    dbIds.Add(new DatabaseGuidWrapper { DatabaseId = id });
                }
                parameters["databases"] = JArray.FromObject(dbIds);
            }

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
            Properties.Resources.GetView2DTransformations;

        public override Guid ComponentGuid =>
            new Guid("ca662346-a589-4273-aa2d-f88dacdfaca2");
    }
}
