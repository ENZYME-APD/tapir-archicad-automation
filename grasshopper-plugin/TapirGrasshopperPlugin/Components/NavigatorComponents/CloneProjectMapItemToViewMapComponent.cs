using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CloneProjectMapItemToViewMapComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CloneProjectMapItemToViewMap";

        public CloneProjectMapItemToViewMapComponent()
            : base(
                "CloneProjectMapItemToViewMap",
                "Clone Project Map viewpoints into a specified View Map folder.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the Project Map viewpoints to clone.");

            InGeneric(
                "ParentNavigatorItemGuid",
                "Identifier of the View Map folder to place the clones in. Required - cloning directly onto the View Map root leaves the clone undeletable and unmovable afterwards, so the Add-On refuses it; create a View Map folder first if you don't already have one to target.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "NavigatorItemGuids",
                "Identifier of each created item.");

            OutErrorMessages(
                "Error message of each item (empty when it was created successfully).");
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

            if (!da.TryGet(
                    1,
                    out GH_ObjectWrapper parentWrapper) ||
                parentWrapper?.Value == null)
            {
                this.AddError("ParentNavigatorItemGuid is required.");
                return;
            }
            var parentId = GuidObject<NavigatorGuid>.CreateFromWrapper(parentWrapper);
            if (parentId == null)
            {
                this.AddError("Invalid ParentNavigatorItemGuid.");
                return;
            }

            var input = new CloneProjectMapItemToViewMapParameters
            {
                ViewsData = new List<ViewToClone>()
            };

            foreach (var wrapper in navWrappers)
            {
                var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid navigator item identifier.");
                    return;
                }

                input.ViewsData.Add(
                    new ViewToClone
                    {
                        NavigatorItemId = id,
                        ParentNavigatorItemId = parentId
                    });
            }

            SetCadValuesWithCreatedIds<NavigatorGuid>(
                CommandName,
                input,
                ToAddOn,
                da,
                "navigatorItems",
                "navigatorItemId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CloneProjectMapItemToViewMap;

        public override Guid ComponentGuid =>
            new Guid("7c0d6fef-4d4a-45d8-ac42-72dbda6a9919");
    }
}
