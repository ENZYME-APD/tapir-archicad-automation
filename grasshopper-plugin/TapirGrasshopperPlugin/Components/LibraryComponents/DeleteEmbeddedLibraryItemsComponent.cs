using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Commands;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class DeleteEmbeddedLibraryItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteEmbeddedLibraryItems";

        public DeleteEmbeddedLibraryItemsComponent()
            : base(
                "DeleteEmbeddedLibraryItems",
                "Delete the given items from the embedded library (Archicad 27 or newer).",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Paths",
                "Relative paths of the items inside the embedded library, " +
                "the same paths AddFilesToEmbeddedLibrary takes as OutputPaths.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each item (empty when it was deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> paths))
            {
                return;
            }

            var input = new DeleteEmbeddedLibraryItemsParameters
            {
                EmbeddedLibraryItems = paths
                    .Select(path => new EmbeddedLibraryItem { Path = path })
                    .ToList()
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteEmbeddedLibraryItems;

        public override Guid ComponentGuid =>
            new Guid("5a69ba1a-bba4-40fa-90c7-f7ec24b91039");
    }
}
