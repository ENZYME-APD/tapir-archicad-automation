using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class EditHierarchicalElementComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "EditHierarchicalElement";

        public EditHierarchicalElementComponent()
            : base(
                "EditHierarchicalElement",
                "Opens the given hierarchical element (Curtain Wall, Stair, Railing, Beam or Column) in its " +
                "hierarchical 3D editing mode, as the Edit button of the element does. Fails when Archicad is " +
                "already in an editing mode. Leave the mode with FinishHierarchicalEditing. Requires Archicad " +
                "27 or later.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ElementGuid",
                "The Curtain Wall, Stair, Railing, Beam or Column to edit.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out ElementGuid elementId))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elementId"] = JObject.FromObject(elementId) },
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
            Properties.Resources.EditHierarchicalElement;

        public override Guid ComponentGuid =>
            new Guid("f69a2993-b987-4b22-aee4-c2fbfee47ca7");
    }

    public static class HierarchicalEditingHelp
    {
        // The editing commands answer with a single execution result.
        public static void ReportFailure(
            GH_ActiveObject component,
            JObject response)
        {
            if ((bool?)response?["success"] == false)
            {
                component.AddError(
                    response["error"]?["message"]?.ToString() ??
                    "The command failed.");
            }
        }
    }
}
