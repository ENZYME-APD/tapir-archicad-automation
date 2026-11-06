using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibrariesOfLibPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibrariesOfLibParts";

        public GetLibrariesOfLibPartsComponent()
            : base(
                "GetLibrariesOfLibParts",
                "Gets the library of each given library part (by file name or guid): display name, location, " +
                "embedded/built-in/package/local/missing flags and the Library Manager tree path. The outputs list " +
                "the parts given by name first, then the ones given by guid. Available from Archicad 29.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "File name of each library part with its extension, as the fileName of GetAvailableLibraryParts. " +
                "Fails when it is ambiguous. Optional.");

            InTexts(
                "Guids",
                "The guid of each library part, as GetAvailableLibraryParts returns it. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibraryNames",
                "The display name of the library of each library part.");

            OutTexts(
                "LibraryPaths",
                "The location of the library of each library part.");

            OutBooleans(
                "IsEmbedded",
                "Whether the library is the embedded library of the project.");

            OutBooleans(
                "IsBuiltIn",
                "Whether the library is built into Archicad.");

            OutBooleans(
                "IsPackage",
                "Whether the library is a library package.");

            OutBooleans(
                "IsLocal",
                "Whether the library is a local library.");

            OutBooleans(
                "IsMissing",
                "Whether the library is missing.");

            OutTextTree(
                "LibraryTreePaths",
                "The path of each library part in the Library Manager tree, not a physical location (one branch " +
                "per library part).");

            OutErrorMessages(
                "Error message of each library part (empty when its library was found).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var names = da.OptionalList<string>(0);
            var guids = da.OptionalList<string>(1);

            var libraryParts = new JArray();
            foreach (var name in names)
            {
                libraryParts.Add(new JObject { ["name"] = name });
            }

            foreach (var guid in guids)
            {
                libraryParts.Add(new JObject { ["guid"] = guid });
            }

            if (libraryParts.Count == 0)
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["libraryParts"] = libraryParts },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var libraryNames = new List<object>();
            var libraryPaths = new List<object>();
            var isEmbedded = new List<object>();
            var isBuiltIn = new List<object>();
            var isPackage = new List<object>();
            var isLocal = new List<object>();
            var isMissing = new List<object>();
            var treePaths = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "libraries");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                treePaths.EnsurePath(path);

                var library = JsonOutputHelp.IsError(item) ? null : item["library"];
                libraryNames.Add(JsonOutputHelp.Scalar(library, "displayName"));
                libraryPaths.Add(JsonOutputHelp.Scalar(library, "path"));
                isEmbedded.Add(JsonOutputHelp.Scalar(library, "isEmbedded"));
                isBuiltIn.Add(JsonOutputHelp.Scalar(library, "isBuiltIn"));
                isPackage.Add(JsonOutputHelp.Scalar(library, "isPackage"));
                isLocal.Add(JsonOutputHelp.Scalar(library, "isLocal"));
                isMissing.Add(JsonOutputHelp.Scalar(library, "isMissing"));

                if (!JsonOutputHelp.IsError(item) &&
                    item["libraryTreePath"] is JArray treePath)
                {
                    foreach (var folder in treePath)
                    {
                        treePaths.Add(folder.ToString(), path);
                    }
                }

                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, libraryNames);
            da.SetDataList(1, libraryPaths);
            da.SetDataList(2, isEmbedded);
            da.SetDataList(3, isBuiltIn);
            da.SetDataList(4, isPackage);
            da.SetDataList(5, isLocal);
            da.SetDataList(6, isMissing);
            da.SetDataTree(7, treePaths);
            da.SetDataList(8, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraries;

        public override Guid ComponentGuid =>
            new Guid("79b0c97b-e1e9-414d-942f-d7e63e5c4607");
    }
}
