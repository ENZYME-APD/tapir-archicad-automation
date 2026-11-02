using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetStructureDisplayComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetStructureDisplay";

        public SetStructureDisplayComponent()
            : base(
                "SetStructureDisplay",
                "Sets the Partial Structure Display (EntireStructure, CoreOnly, WithoutFinishes or StructureOnly) " +
                "of the current window, e.g. the floor plan, a section or the 3D window. The change cannot be " +
                "undone. The saved view is not changed (see SetViewSettings for that). Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "StructureDisplay",
                "The Partial Structure Display mode to apply to the current window: EntireStructure, CoreOnly, " +
                "WithoutFinishes or StructureOnly.");
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
            if (!da.TryGet(
                    0,
                    out string structureDisplay))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["structureDisplay"] = structureDisplay },
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

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetStructureDisplay;

        public override Guid ComponentGuid =>
            new Guid("a18d1c95-35ec-4abf-a5ce-4e4db51a62c8");
    }
}
