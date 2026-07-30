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
                "Clone Project Map viewpoints into the View Map, optionally into a specified folder.",
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
                "Identifier of the View Map folder to place the clones in. Optional; defaults to the View Map root.");

            SetOptionality(1);
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

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CloneProjectMapItemToViewMap;

        public override Guid ComponentGuid =>
            new Guid("7c0d6fef-4d4a-45d8-ac42-72dbda6a9919");
    }
}
