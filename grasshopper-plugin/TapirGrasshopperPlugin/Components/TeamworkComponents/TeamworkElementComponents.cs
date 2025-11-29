using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.ResponseTypes.Generic;

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

        protected abstract string GetMessage(
            JObject response);

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj input))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    input,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                GetMessage(response));
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

        protected override string GetMessage(
            JObject response)
        {
            return ExecutionResultResponse.Deserialize(response).Message();
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

        protected override string GetMessage(
            JObject response)
        {
            return ExecutionResult.Deserialize(response).Message();
        }

        public override Guid ComponentGuid =>
            new Guid("f0456a61-c0c7-445a-b670-009b2ae5d1af");
    }
}