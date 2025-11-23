using Grasshopper.Kernel;
using System;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public abstract class AbsTeamworkElementsComponent
        : AbsTeamworkBaseComponent
    {
        protected AbsTeamworkElementsComponent(
            string name,
            string description)
            : base(
                name,
                description)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements Guids to get detailList for.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            SolveByElementsInputResponse(da);
        }
    }

    public class ReserveElementsComponent : AbsTeamworkElementsComponent
    {
        public override string CommandName => "ReserveElements";

        public ReserveElementsComponent()
            : base(
                "ReserveElements",
                "Reserves elements in Teamwork mode.")
        {
        }

        public override Guid ComponentGuid =>
            new Guid("3c0e9944-2875-4a68-8794-ec16fa3235f5");
    }

    public class ReleaseElementsComponent : AbsTeamworkElementsComponent
    {
        public override string CommandName => "ReleaseElements";

        public ReleaseElementsComponent()
            : base(
                "ReleaseElements",
                "Releases elements in Teamwork mode.")
        {
        }

        public override Guid ComponentGuid =>
            new Guid("f0456a61-c0c7-445a-b670-009b2ae5d1af");
    }
}