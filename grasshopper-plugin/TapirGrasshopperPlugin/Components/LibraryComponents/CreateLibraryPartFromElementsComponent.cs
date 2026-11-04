using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class CreateLibraryPartFromElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLibraryPartFromElements";

        // The optional text inputs after Name and ElementGuids, with their key.
        private static readonly (string Input, string Key, string Description)[] TextInputs =
        {
            ("ParentId", "parentId",
                "Parent subtype of the library part: its unique ID ('{main}-{revision}') or its main GUID, " +
                "as GetAvailableLibraryParts reports it. Either ParentId or ParentName is required"),
            ("ParentName", "parentName",
                "Document name of the parent subtype, e.g. 'General Object' or 'Model Element'; " +
                "used when ParentId is not given"),
            ("Folder", "folder",
                "Folder of the new .gsm file, relative to the embedded library (or to LibraryFolder), " +
                "created when missing. By default the embedded library (or LibraryFolder) itself. Optional"),
            ("LibraryFolder", "libraryFolder",
                "Absolute path of a local library folder to create the library part in instead of the embedded " +
                "library. A folder that is not loaded yet is added to the loaded libraries. Optional"),
            ("Comment", "comment", "Comment (description) of the library part. Optional"),
            ("Keywords", "keywords", "Keywords of the library part. Optional")
        };

        // The script inputs, with their key in the scripts object.
        private static readonly (string Input, string Key, string Description)[] ScriptInputs =
        {
            ("MasterScript", "master", "GDL master script"),
            ("Script2D", "2D", "GDL 2D script; when empty, the 2D symbol is drawn"),
            ("Script3D", "3D", "GDL 3D script"),
            ("ParameterScript", "parameter", "GDL parameter script")
        };

        private const int FirstTextInput = 2;
        private const int FirstScriptInput = FirstTextInput + 6;
        private const int OverwriteInput = FirstScriptInput + 4;
        private const int PlaceableInput = OverwriteInput + 1;

        public CreateLibraryPartFromElementsComponent()
            : base(
                "CreateLibraryPartFromElements",
                "Creates a library part whose 2D symbol is a copy of the given 2D drawing elements (lines, polylines, " +
                "arcs, circles, splines, fills, texts and hotspots), in the embedded library or a given local library " +
                "folder, under the given parent subtype, with optional scripts, comment and keywords. The elements keep " +
                "their plan coordinates, so the project origin becomes the origin of the symbol; they are not changed.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the new library part, as shown in the library and used by CreateObjects as libraryPartName.");

            InGenerics(
                "ElementGuids",
                "The 2D drawing elements whose copies make up the 2D symbol.");

            foreach (var input in TextInputs)
            {
                InText(
                    input.Input,
                    input.Description + ".");
            }

            foreach (var input in ScriptInputs)
            {
                InText(
                    input.Input,
                    $"The {input.Description}. Optional.");
            }

            InBoolean(
                "Overwrite",
                "Overwrite a library part file of the same name in the folder. When false the new library part " +
                "gets a unique name, returned in Name.",
                false);

            InBoolean(
                "Placeable",
                "Whether the new library part can be placed.",
                true);

            var optional = new int[PlaceableInput - FirstTextInput + 1];
            for (var i = 0; i < optional.Length; i++)
            {
                optional[i] = FirstTextInput + i;
            }
            SetOptionality(optional);
        }

        protected override void AddOutputs()
        {
            OutText(
                "Name",
                "Name of the created library part.");

            OutText(
                "FileName",
                "File name of the created library part, with its extension.");

            OutText(
                "LibraryPartGuid",
                "Main GUID of the created library part, as GetAvailableLibraryParts reports it.");

            OutText(
                "UnId",
                "Unique ID ('{main}-{revision}') of the created library part.");

            OutInteger(
                "Index",
                "Index of the created library part in the loaded libraries.");

            OutNumberList(
                "AB",
                "Default A and B (X and Y size) of the library part: the extent of its 2D symbol.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string name))
            {
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elements))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);
            parameters["name"] = name;

            for (var i = 0; i < TextInputs.Length; i++)
            {
                var value = da.GetOptional<string>(FirstTextInput + i, null);
                if (!string.IsNullOrEmpty(value))
                {
                    parameters[TextInputs[i].Key] = value;
                }
            }

            var scripts = new JObject();
            for (var i = 0; i < ScriptInputs.Length; i++)
            {
                var value = da.GetOptional<string>(FirstScriptInput + i, null);
                if (!string.IsNullOrEmpty(value))
                {
                    scripts[ScriptInputs[i].Key] = value;
                }
            }
            if (scripts.Count > 0)
            {
                parameters["scripts"] = scripts;
            }

            parameters["overwrite"] = da.GetOptional(OverwriteInput, false);
            parameters["placeable"] = da.GetOptional(PlaceableInput, true);

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, response["name"]?.ToString());
            da.SetData(1, response["fileName"]?.ToString());
            da.SetData(2, response["guid"]?.ToString());
            da.SetData(3, response["unId"]?.ToString());
            da.SetData(4, (int?)response["index"]);
            da.SetDataList(5, new[] { (double?)response["a"], (double?)response["b"] });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLibraryPartFromElements;

        public override Guid ComponentGuid =>
            new Guid("44e83f8e-a1d2-465d-9d26-6797ff985c0f");
    }
}
