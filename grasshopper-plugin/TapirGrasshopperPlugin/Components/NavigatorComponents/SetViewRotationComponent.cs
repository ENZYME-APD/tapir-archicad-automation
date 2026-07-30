using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetViewRotationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetViewRotation";

        public SetViewRotationComponent()
            : base(
                "SetViewRotation",
                "Set the rotation angle of 2D views via their floor plan database.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemGuids",
                "Identifiers of the navigator items to rotate.");

            InNumbers(
                "Rotations",
                "View rotation angles in radians (parallel to NavigatorItemGuids).");
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

            if (!da.TryGetList(
                    1,
                    out List<double> rotations))
            {
                return;
            }

            if (navWrappers.Count != rotations.Count)
            {
                this.AddError(
                    "The size of the inputs NavigatorItemGuids and Rotations must be equal.");
                return;
            }

            var input = new SetViewRotationParameters
            {
                NavigatorItemIdsWithRotation = new List<NavigatorItemWithRotation>()
            };

            for (var i = 0; i < navWrappers.Count; i++)
            {
                var id = GuidObject<NavigatorGuid>.CreateFromWrapper(navWrappers[i]);
                if (id == null)
                {
                    this.AddError("Invalid navigator item identifier.");
                    return;
                }

                input.NavigatorItemIdsWithRotation.Add(
                    new NavigatorItemWithRotation
                    {
                        NavigatorItemId = id,
                        Rotation = rotations[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewRotation;

        public override Guid ComponentGuid =>
            new Guid("f7320962-20f7-4227-b00e-b6077db47965");
    }
}
