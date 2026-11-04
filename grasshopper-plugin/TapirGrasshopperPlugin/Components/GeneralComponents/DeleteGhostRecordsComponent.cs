using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class DeleteGhostRecordsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteGhostRecords";

        public DeleteGhostRecordsComponent()
            : base(
                "DeleteGhostRecords",
                "Deletes the trace (ghost) record of each given parent database, which removes the trace " +
                "reference shown in that database's window. A database without a trace reference gets an " +
                "error message. Not undoable.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DatabaseIds",
                "The parent databases whose trace (ghost) record is deleted, for example the DatabaseIds " +
                "output of GetContainingDatabases.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each database (empty when its trace record was deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var databases = DatabasesObject.Create(
                da,
                0);
            if (databases?.Databases == null)
            {
                this.AddError("Invalid database identifier in the DatabaseIds input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["databases"] = JArray.FromObject(databases.Databases) },
                ToAddOn,
                da);
        }

        // Shares the icon of GetDatabases until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDatabases;

        public override Guid ComponentGuid =>
            new Guid("738c50b9-5186-4cdb-aa96-d157941a6d67");
    }
}
