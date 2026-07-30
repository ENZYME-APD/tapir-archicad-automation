using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class DeleteAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteAttributes";

        public DeleteAttributesComponent()
            : base(
                "DeleteAttributes",
                "Delete the given attributes.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "Type of the attributes to delete (e.g. Layer, Line, Fill, Composite, Surface, ZoneCategory, Profile, PenTable, MEPSystem, BuildingMaterial).");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to delete.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string attributeType))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            var input = new DeleteAttributesParameters
            {
                AttributesToDelete = new List<AttributeToDelete>()
            };

            foreach (var wrapper in attributeWrappers)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(wrapper);
                if (attributeId == null)
                {
                    this.AddError(
                        "Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                input.AttributesToDelete.Add(
                    new AttributeToDelete
                    {
                        AttributeType = attributeType,
                        AttributeId = new AttributeGuidWrapper { AttributeId = attributeId }
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteAttributes;

        public override Guid ComponentGuid =>
            new Guid("6ba592aa-c1ca-457c-a6fd-72a161d5fd2c");
    }
}
