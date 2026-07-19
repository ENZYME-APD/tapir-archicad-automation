using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class PrintViewComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PrintView";

        public PrintViewComponent()
            : base(
                "PrintView",
                "Print from the current view.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "Grid",
                "Print the grid.",
                false);

            InBoolean(
                "FixText",
                "Use fixed text size.",
                false);

            InInteger(
                "Scale",
                "Print scale.",
                100);

            InTextWithDefault(
                "PrintArea",
                "The area to print: currentView, entireDrawing or marquee.",
                "currentView");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(
                0,
                out bool grid);
            da.TryGet(
                1,
                out bool fixText);
            da.TryGet(
                2,
                out int scale);
            da.TryGet(
                3,
                out string printArea);

            var input = new PrintViewParameters
            {
                Grid = grid,
                FixText = fixText,
                Scale = scale,
                PrintArea = string.IsNullOrEmpty(printArea) ? "currentView" : printArea
            };

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PrintView;

        public override Guid ComponentGuid =>
            new Guid("c19f7cb7-182a-4c70-a8f9-449ab97bec5d");
    }
}
