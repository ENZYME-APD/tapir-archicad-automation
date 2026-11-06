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
                "Get the type of the current (active) window and the database it shows.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "CurrentWindowType",
                "The type of the current (active) window (e.g. FloorPlan, Section, 3DModel, Layout).");

            OutGeneric(
                "DatabaseId",
                "The database shown in the current window; connect it to ChangeWindow to bring the window back. Empty when the window has no database, e.g. a report.");
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

            if (response.DatabaseId != null)
            {
                da.SetData(
                    1,
                    response.DatabaseId);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCurrentWindowType;

        public override Guid ComponentGuid =>
            new Guid("1cac51a5-edbf-4dfe-a8f2-811963537e76");
    }
}
