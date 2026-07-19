using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Set3DCutPlanesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DCutPlanes";

        public Set3DCutPlanesComponent()
            : base(
                "Set3DCutPlanes",
                "Set the 3D cut planes. Each plane is defined by the equation pa*x + pb*y + pc*z + pd = 0.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InNumbers(
                "PA",
                "Coefficient of x (x of the plane normal) for each cut plane.");

            InNumbers(
                "PB",
                "Coefficient of y (y of the plane normal) for each cut plane.");

            InNumbers(
                "PC",
                "Coefficient of z (z of the plane normal) for each cut plane.");

            InNumbers(
                "PD",
                "Constant term (distance of the plane from the origin along the normal) for each cut plane.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<double> pa) ||
                !da.TryGetList(1, out List<double> pb) ||
                !da.TryGetList(2, out List<double> pc) ||
                !da.TryGetList(3, out List<double> pd))
            {
                return;
            }

            if (pa.Count != pb.Count ||
                pa.Count != pc.Count ||
                pa.Count != pd.Count)
            {
                this.AddError(
                    "The size of the inputs PA, PB, PC and PD must be equal.");
                return;
            }

            var input = new Set3DCutPlanesParameters
            {
                CutPlanes = new List<CutPlane>()
            };

            for (var i = 0; i < pa.Count; i++)
            {
                input.CutPlanes.Add(
                    new CutPlane
                    {
                        Pa = pa[i],
                        Pb = pb[i],
                        Pc = pc[i],
                        Pd = pd[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DCutPlanes;

        public override Guid ComponentGuid =>
            new Guid("9253ed54-3be3-43ff-8fe2-2e1261e09f52");
    }
}
