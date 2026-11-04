using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class IFCFileOperationComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "IFCFileOperation";

        public IFCFileOperationComponent()
            : base(
                "IFCFileOperation",
                "Execute an IFC file operation (save, merge or open).",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Method",
                "The file operation method to use: save, merge or open.");

            InText(
                "IfcFilePath",
                "The target IFC file to use.");

            InTextWithDefault(
                "FileType",
                "The type of the IFC file: ifc, ifcxml, ifczip or ifcxmlzip.",
                "ifc");

            // Appended after the original inputs, so saved definitions keep
            // their wiring.
            InText(
                "TranslatorName",
                "Name of an IFC export translator of the project to save " +
                "with; see GetIFCExportTranslators. Only valid with the save " +
                "method. Optional.");

            InText(
                "ElementsToExport",
                "The elements to export: EntireProject, " +
                "VisibleElementsOnAllStories, AllElementsOnCurrentStory, " +
                "VisibleElementsOnCurrentStory or SelectedElementsOnly. Only " +
                "valid together with TranslatorName. Optional.");

            // No default value: without a StoryIndex the merge keeps
            // using the active story.
            inManager.AddIntegerParameter(
                "StoryIndex",
                "StoryIndex",
                ("The story the first storey of the IFC file is placed on; " +
                 "see GetStories. Only valid with the merge method. The " +
                 "active story and window are restored afterwards. Optional.")
                .WithTypeName("integerItem"),
                GH_ParamAccess.item);

            SetOptionality(new[] { 3, 4, 5 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string method) ||
                string.IsNullOrEmpty(method))
            {
                this.AddError("Method is required.");
                return;
            }

            if (!da.TryGet(
                    1,
                    out string ifcFilePath) ||
                string.IsNullOrEmpty(ifcFilePath))
            {
                this.AddError("IfcFilePath is required.");
                return;
            }

            da.TryGet(
                2,
                out string fileType);

            da.TryGet(
                3,
                out string translatorName);

            da.TryGet(
                4,
                out string elementsToExport);

            int? storyIndex = null;
            if (da.TryGet(
                    5,
                    out int storyIndexValue))
            {
                storyIndex = storyIndexValue;
            }

            var input = new IFCFileOperationParameters
            {
                Method = method,
                IfcFilePath = ifcFilePath,
                FileType = string.IsNullOrEmpty(fileType) ? null : fileType,
                TranslatorName = string.IsNullOrEmpty(translatorName)
                    ? null
                    : translatorName,
                ElementsToExport = string.IsNullOrEmpty(elementsToExport)
                    ? null
                    : elementsToExport,
                StoryIndex = storyIndex
            };

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.IFCFileOperation;

        public override Guid ComponentGuid =>
            new Guid("64403220-e7e3-4eda-8e6a-33c0e79080a0");
    }
}
