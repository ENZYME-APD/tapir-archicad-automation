using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetGDLDateComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetGDLDate";

        public SetGDLDateComponent()
            : base(
                "SetGDLDate",
                "Sets the date GDL scripts see, in the integer format GetGDLDate returns. Useful to test " +
                "date-dependent library parts; set the value read with GetGDLDate back to restore it.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InInteger(
                "Date",
                "The date GDL scripts should see, in the integer format GetGDLDate returns.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(0, out int date))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["date"] = date },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IsUsableGDLString;

        public override Guid ComponentGuid =>
            new Guid("1fd1c6fb-d278-4a19-becd-29e25ea015e6");
    }
}
