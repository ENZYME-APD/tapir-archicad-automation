using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CreateLibraryPartComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLibraryPart";

        public CreateLibraryPartComponent()
            : base(
                "CreateLibraryPart",
                "Creates a new library part file (.gsm) of the given loaded subtype, inheriting its parameters, " +
                "and reports whether it is loaded with its index and names. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Path",
                "Full path of the new library part file. It must end with .gsm, must not exist yet, and its folder must exist.");

            InText(
                "AncestorSubtype",
                "Name of a loaded library part that is a subtype, with its extension, as the fileName of " +
                "GetAvailableLibraryParts. The new object is of this subtype and inherits its parameters.");
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsLoaded",
                "Whether the new library part is loaded in the project " +
                "(only when its folder is in a loaded library).");

            OutText(
                "LibraryPartGuid",
                "Guid of the new library part, as GetAvailableLibraryParts lists it (empty when it is not loaded).");

            OutInteger(
                "Index",
                "Index of the new library part in the loaded libraries (empty when it is not loaded).");

            OutText(
                "DocumentName",
                "Document name of the new library part (empty when it is not loaded).");

            OutText(
                "FileName",
                "File name of the new library part (empty when it is not loaded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string path = null;
            string ancestorSubtype = null;
            if (!da.GetData(0, ref path) ||
                !da.GetData(1, ref ancestorSubtype))
            {
                return;
            }

            var parameters = new JObject
            {
                ["path"] = path,
                ["ancestorSubtype"] = ancestorSubtype
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var libraryPart = response["libraryPart"];
            da.SetData(0, (bool?)response["isLoaded"] ?? false);
            da.SetData(1, libraryPart?["guid"]?.ToString());
            da.SetData(2, (int?)libraryPart?["index"]);
            da.SetData(3, libraryPart?["documentName"]?.ToString());
            da.SetData(4, libraryPart?["fileName"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLibraryPart;

        public override Guid ComponentGuid =>
            new Guid("82aefe0e-61be-4906-ae75-d70e2064f109");
    }
}
