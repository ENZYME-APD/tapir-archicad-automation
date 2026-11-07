using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateAddOnViewpointsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAddOnViewpoints";

        public CreateAddOnViewpointsComponent()
            : base(
                "CreateAddOnViewpoints",
                "Creates add-on viewpoint roots, groups and leaf nodes in the Project Map, with their ID, name, " +
                "view settings flags (leaf nodes only) and attached data (base64 encoded). A group or a leaf node " +
                "is created in the given parent root or group. Returns the identifier of each new item, or an " +
                "error for an item that could not be created. Roots and groups can't be created in a Teamwork " +
                "project. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ItemTypes",
                "Whether to create a Root, a Group or a Leaf node, one per new item. Roots and groups can't be " +
                "created in a Teamwork project.");

            InGenerics(
                "ParentViewpointIds",
                "The root or group to create each item in. Required for a group or a leaf node, ignored for a " +
                "root. The identifier of a new root or group is only known from the output, so create the " +
                "parents in an earlier run. " + AddOnViewpointFields.ListDoc);

            AddOnViewpointFields.AddInputs(this);

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3,
                    4,
                    5
                });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ViewpointIds",
                "The identifier of each created item (null for a failed item).");

            OutErrorMessages(
                "Error message of each item (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> itemTypes))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> parentWrappers);

            if (!NavigatorComponentHelp.CheckCount(
                    this,
                    "ParentViewpointIds",
                    parentWrappers.Count,
                    "ItemTypes",
                    itemTypes.Count))
            {
                return;
            }

            var fields = AddOnViewpointFields.Read(
                this,
                da,
                2,
                "ItemTypes",
                itemTypes.Count);

            if (fields == null)
            {
                return;
            }

            var viewpoints = new JArray();

            for (var i = 0; i < itemTypes.Count; i++)
            {
                var viewpoint = new JObject { ["itemType"] = itemTypes[i] };

                var parentWrapper = NavigatorComponentHelp.GetAt(
                    parentWrappers,
                    i);
                if (parentWrapper != null)
                {
                    var parentId = AddOnViewpointFields.IdOf(parentWrapper);
                    if (parentId == null)
                    {
                        this.AddError(
                            "Invalid identifier in the ParentViewpointIds input.");
                        return;
                    }

                    viewpoint["parentViewpointId"] = JObject.FromObject(parentId);
                }

                fields.AddTo(
                    viewpoint,
                    i);

                viewpoints.Add(viewpoint);
            }

            SetCadValuesWithCreatedIds<AddOnViewpointGuid>(
                CommandName,
                new JObject { ["viewpoints"] = viewpoints },
                ToAddOn,
                da,
                "viewpoints",
                "viewpointId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAddOnViewpoints;

        public override Guid ComponentGuid =>
            new Guid("bea0ca29-9d22-4842-a945-b65d5985c609");
    }
}
