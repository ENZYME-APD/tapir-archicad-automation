using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class AddFilesToEmbeddedLibraryComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddFilesToEmbeddedLibrary";

        public AddFilesToEmbeddedLibraryComponent()
            : base(
                "AddFilesToEmbeddedLibrary",
                "Add the given files into the embedded library.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "InputPaths",
                "Absolute paths of the input files to add.");

            InTexts(
                "OutputPaths",
                "Relative paths of the new files inside the embedded library.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> inputPaths))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> outputPaths))
            {
                return;
            }

            if (inputPaths.Count != outputPaths.Count)
            {
                this.AddError(
                    "The size of the inputs InputPaths and OutputPaths must be equal.");
                return;
            }

            var input = new AddFilesToEmbeddedLibraryParameters
            {
                Files = new List<LibraryFileAddition>()
            };

            for (var i = 0; i < inputPaths.Count; i++)
            {
                input.Files.Add(
                    new LibraryFileAddition
                    {
                        InputPath = inputPaths[i],
                        OutputPath = outputPaths[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddFilesToEmbeddedLibrary;

        public override Guid ComponentGuid =>
            new Guid("82299434-d631-4d15-8a6a-ca0fc1db4f44");
    }
}
