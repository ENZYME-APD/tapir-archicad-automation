using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ChangeAddOnViewpointsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeAddOnViewpoints";

        public ChangeAddOnViewpointsComponent()
            : base(
                "ChangeAddOnViewpoints",
                "Changes the ID, name, view settings flags or attached data (base64 encoded) of the given add-on " +
                "viewpoint items; only the given fields are changed. Roots and groups can't be changed in a " +
                "Teamwork project. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ViewpointIds",
                "Identifiers of the add-on viewpoint items to change.");

            AddOnViewpointFields.AddInputs(this);

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3,
                    4
                });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each item (empty when it was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> viewpointWrappers))
            {
                return;
            }

            var fields = AddOnViewpointFields.Read(
                this,
                da,
                1,
                "ViewpointIds",
                viewpointWrappers.Count);

            if (fields == null)
            {
                return;
            }

            var viewpoints = new JArray();

            for (var i = 0; i < viewpointWrappers.Count; i++)
            {
                var viewpointId = AddOnViewpointFields.IdOf(viewpointWrappers[i]);
                if (viewpointId == null)
                {
                    this.AddError(
                        "Invalid identifier in the ViewpointIds input.");
                    return;
                }

                var viewpoint = new JObject { ["viewpointId"] = JObject.FromObject(viewpointId) };

                fields.AddTo(
                    viewpoint,
                    i);

                viewpoints.Add(viewpoint);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["viewpoints"] = viewpoints },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeAddOnViewpoints;

        public override Guid ComponentGuid =>
            new Guid("95305cba-092b-43f3-bf40-35f783f24233");
    }
}
