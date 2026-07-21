using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.RevisionComponents
{
    public class GetCurrentRevisionChangesOfLayoutsComponent : RevisionChangesOfEntitiesComponent
    {
        public override string CommandName => "GetCurrentRevisionChangesOfLayouts";

        public GetCurrentRevisionChangesOfLayoutsComponent()
            : base(
                "GetCurrentRevisionChangesOfLayouts",
                "Retrieve all changes belonging to the last revision of the given layouts.")
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "LayoutDatabaseGuids",
                "Identifiers of the layout databases to query.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = new GetCurrentRevisionChangesOfLayoutsParameters
            {
                LayoutDatabaseIds = new List<DatabaseGuidWrapper>()
            };

            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<DatabaseGuidObject>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid database identifier in the LayoutDatabaseGuids input.");
                    return;
                }
                input.LayoutDatabaseIds.Add(
                    new DatabaseGuidWrapper { DatabaseId = id });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetChangeOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    "currentRevisionChangesOfLayouts"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentRevisionChangesOfLayouts;

        public override Guid ComponentGuid =>
            new Guid("862896fd-0180-48a3-ae81-28d09a83bb95");
    }
}
