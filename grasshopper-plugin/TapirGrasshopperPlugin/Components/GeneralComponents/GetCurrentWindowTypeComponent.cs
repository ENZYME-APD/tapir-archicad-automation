using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Application;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetCurrentWindowTypeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCurrentWindowType";

        public GetCurrentWindowTypeComponent()
            : base(
                "GetCurrentWindowType",
                "Get the type of the current (active) window.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "CurrentWindowType",
                "The type of the current (active) window (e.g. FloorPlan, Section, 3DModel, Layout).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedCadValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<CurrentWindowTypeResponse>,
                    out CurrentWindowTypeResponse response))
            {
                return;
            }

            da.SetData(
                0,
                response.CurrentWindowType);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentWindowType;

        public override Guid ComponentGuid =>
            new Guid("1cac51a5-edbf-4dfe-a8f2-811963537e76");
    }
}
