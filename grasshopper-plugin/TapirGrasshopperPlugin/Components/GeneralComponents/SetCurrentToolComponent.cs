using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class SetCurrentToolComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCurrentTool";

        public SetCurrentToolComponent()
            : base(
                "SetCurrentTool",
                "Activates a tool of the Toolbox, given as GetCurrentTool and GetToolboxItems return it: an " +
                "element type with an optional variation, or the class id of an add-on's tool. With no input " +
                "connected it activates the Arrow. Archicad switches the tool once it processes its events, so " +
                "it may take a moment to show in the Toolbox.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "The element type of the tool to activate (e.g. Wall, Door, Object). Optional; leave it and " +
                "the other inputs empty to activate the Arrow.");

            InText(
                "Variation",
                "The four-character subtype of the tool (e.g. SKYL, CRWD), as GetCurrentTool or " +
                "GetToolboxItems return it. Optional; needs ElementType or ClassId.");

            InText(
                "ClassId",
                "The class id (a guid) of the tool of an add-on. Optional; when given, ElementType is " +
                "not needed. Archicad 26 and later.");

            SetOptionality(new[] { 0, 1, 2 });
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
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out string elementType) &&
                !string.IsNullOrEmpty(elementType))
            {
                parameters["elementType"] = elementType;
            }

            if (da.TryGet(
                    1,
                    out string variation) &&
                !string.IsNullOrEmpty(variation))
            {
                parameters["variation"] = variation;
            }

            if (da.TryGet(
                    2,
                    out string classId) &&
                !string.IsNullOrEmpty(classId))
            {
                parameters["classId"] = classId;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
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
            Properties.Resources.ChangeWindow;

        public override Guid ComponentGuid =>
            new Guid("b7d2e5a1-8c3f-4e09-a6d4-2f9e1b0c7a35");
    }
}
