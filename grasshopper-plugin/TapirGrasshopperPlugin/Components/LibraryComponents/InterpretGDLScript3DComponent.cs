using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class InterpretGDLScript3DComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "InterpretGDLScript3D";

        public InterpretGDLScript3DComponent()
            : base(
                "InterpretGDLScript3D",
                "Runs a GDL 3D script with the settings of the GDL editor and tells whether it ran without a GDL " +
                "error, how many 3D model elements it generated and their bounding box. Give the script, or a loaded " +
                "library part whose master and 3D scripts run with its default parameters. The API gives no GDL " +
                "error message or line number.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Script",
                "The GDL 3D script to run. Either Script or LibraryPartName is required.");

            InText(
                "LibraryPartName",
                "A loaded library part, by its fileName (with extension) or documentName as GetAvailableLibraryParts " +
                "returns them. Its default parameters are used; without Script its master and 3D scripts run.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Success",
                "True when the script ran to its end, false when the GDL interpreter stopped with an error.");

            OutInteger(
                "ElementCount",
                "Number of 3D model elements the script generated (empty when it failed).");

            OutBoxes(
                "BoundingBox",
                "Bounding box of the generated 3D model, in the local coordinates of the library part " +
                "(empty when it has no elements).");

            OutText(
                "ErrorMessage",
                "Error of the interpreter, with its code, when Success is false.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var script = da.GetOptional<string>(0, null);
            var libraryPartName = da.GetOptional<string>(1, null);
            if (script == null && string.IsNullOrEmpty(libraryPartName))
            {
                this.AddError("Give Script or LibraryPartName.");
                return;
            }

            var parameters = new JObject();
            if (script != null)
            {
                parameters["script"] = script;
            }
            if (!string.IsNullOrEmpty(libraryPartName))
            {
                parameters["libraryPartName"] = libraryPartName;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, (bool?)response["success"] ?? false);
            da.SetData(1, (int?)response["elementCount"]);

            if (response["boundingBox3D"] is JObject box)
            {
                da.SetDataList(2, new[]
                {
                    new Rhino.Geometry.Box(
                        new Rhino.Geometry.BoundingBox(
                            (double)box["xMin"],
                            (double)box["yMin"],
                            (double)box["zMin"],
                            (double)box["xMax"],
                            (double)box["yMax"],
                            (double)box["zMax"]))
                });
            }

            if (response["error"] is JObject error)
            {
                da.SetData(3, $"{error["message"]} (error code {error["code"]})");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.InterpretGDLScript3D;

        public override Guid ComponentGuid =>
            new Guid("264af90d-ea87-4c93-ab9e-4e4e190ad1a8");
    }
}
