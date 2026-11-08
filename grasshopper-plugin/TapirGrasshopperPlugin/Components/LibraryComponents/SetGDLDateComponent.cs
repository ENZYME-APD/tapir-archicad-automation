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
                "Sets the date GDL scripts see: year, month, day, hour, minute and second, as GetGDLDate " +
                "returns them. Useful to test date-dependent library parts; set the date read with GetGDLDate " +
                "back to restore it.",
                GroupNames.Library)
        {
        }

        // The inputs and the keys of the command's "date" object they fill.
        private static readonly (string Input, string Key)[] DateComponents =
        {
            ("Year", "year"), ("Month", "month"), ("Day", "day"),
            ("Hour", "hour"), ("Minute", "minute"), ("Second", "second")
        };

        protected override void AddInputs()
        {
            foreach (var (input, key) in DateComponents)
            {
                InInteger(
                    input,
                    $"The {key} of the date GDL scripts should see.");
            }
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
            var date = new JObject();
            for (int i = 0; i < DateComponents.Length; i++)
            {
                if (!da.TryGet(i, out int value))
                {
                    return;
                }

                date[DateComponents[i].Key] = value;
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
            Properties.Resources.SetGDLDate;

        public override Guid ComponentGuid =>
            new Guid("1fd1c6fb-d278-4a19-becd-29e25ea015e6");
    }
}
