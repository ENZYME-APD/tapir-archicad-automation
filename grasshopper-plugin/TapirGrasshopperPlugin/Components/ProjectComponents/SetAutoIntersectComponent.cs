using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetAutoIntersectComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetAutoIntersect";

        public SetAutoIntersectComponent()
            : base(
                "SetAutoIntersect",
                "Turns Auto-Intersection on or off. It is a setting of the running Archicad, not of the project; " +
                "remember to restore the previous state afterwards.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            inManager.AddBooleanParameter(
                "IsOn",
                "IsOn",
                "Turn Auto-Intersection on (true) or off (false).".WithTypeName("booleanItem"),
                GH_ParamAccess.item);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out bool isOn))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["isOn"] = isOn },
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                ProjectComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetAutoIntersect;

        public override Guid ComponentGuid =>
            new Guid("25d164bd-5cf6-479f-8ff1-85caf65a5a11");
    }
}
