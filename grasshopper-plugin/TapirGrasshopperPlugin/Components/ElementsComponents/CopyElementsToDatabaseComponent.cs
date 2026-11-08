using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CopyElementsToDatabaseComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CopyElementsToDatabase";

        public CopyElementsToDatabaseComponent()
            : base(
                "CopyElementsToDatabase",
                "Copy 2D elements (lines, polylines, arcs, circles, splines, hotspots, texts, hatches, independent labels) " +
                "from one database into another, e.g. from a worksheet into another worksheet, a detail or a layout.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the 2D elements to copy.");

            InGeneric(
                "TargetDatabaseId",
                "Identifier of the database to copy the elements into.");

            InGeneric(
                "SourceDatabaseId",
                "Identifier of the database the elements are in. Optional, default is the current database.");

            inManager.AddVectorParameter(
                "Offset",
                "Offset",
                "Vector the copies are moved by in the target database (only X and Y are used). Optional, without it the copies keep the coordinates of the originals.",
                GH_ParamAccess.item);

            SetOptionality(new[] { 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of the copy of each given element.");

            OutErrorMessages(
                "Error message of each element (empty when it was copied).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!da.TryCreate(
                    1,
                    out DatabaseGuidObject targetDatabaseId))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = JObject.FromObject(elements)["elements"],
                ["targetDatabaseId"] = new JObject { ["guid"] = targetDatabaseId.Guid }
            };

            if (da.TryGet(
                    2,
                    out GH_ObjectWrapper sourceWrapper) &&
                sourceWrapper?.Value != null)
            {
                var sourceDatabaseId = GuidObject<DatabaseGuidObject>.CreateFromWrapper(sourceWrapper);
                if (sourceDatabaseId == null)
                {
                    this.AddError("Invalid SourceDatabaseId.");
                    return;
                }
                parameters["sourceDatabaseId"] = new JObject { ["guid"] = sourceDatabaseId.Guid };
            }

            var offset = Vector3d.Zero;
            if (da.GetData(3, ref offset))
            {
                parameters["offset"] = new JObject { ["x"] = offset.X, ["y"] = offset.Y };
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                parameters,
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        // No icon of its own yet: the one of MoveElements, which also copies inside one database.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("e97b3d20-6a4c-4c1f-b8e5-2d9f0a6c7b13");
    }
}
