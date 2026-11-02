using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FinishHierarchicalEditingComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName => "FinishHierarchicalEditing";

        public FinishHierarchicalEditingComponent()
            : base(
                "FinishHierarchicalEditing",
                "Finishes the current hierarchical editing mode (Curtain Wall, Stair, Railing, Beam or " +
                "Column): Accept true keeps the changes (OK), Accept false discards them (Cancel). The Accept " +
                "input is required, so changes are never discarded silently. Fails when Archicad is not in " +
                "such an editing mode. Requires Archicad 27 or later.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            // No default value: the caller has to decide whether the changes
            // are kept.
            inManager.AddBooleanParameter(
                "Accept",
                "Accept",
                "True keeps the changes made in the editing mode (OK), false discards them (Cancel)."
                    .WithTypeName("booleanItem"),
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out bool accept))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["accept"] = accept },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            HierarchicalEditingHelp.ReportFailure(
                this,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FinishHierarchicalEditing;

        public override Guid ComponentGuid =>
            new Guid("e5299fff-ca93-4ee0-bff5-1c7e6655cead");
    }
}
