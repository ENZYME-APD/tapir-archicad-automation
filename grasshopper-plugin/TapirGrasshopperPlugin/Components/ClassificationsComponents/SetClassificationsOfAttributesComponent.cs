using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class SetClassificationsOfAttributesComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName => "SetClassificationsOfAttributes";

        public SetClassificationsOfAttributesComponent()
            : base(
                "SetClassificationsOfAttributes",
                "Sets the classifications of attributes (building materials, composites, surfaces, etc.). " +
                "The item replaces the attribute's current item in the same classification system. " +
                "In order to set the classification of an attribute to unclassified in the system, " +
                "input an empty (null) Guid as the classification item.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ClassificationSystemGuid",
                "The Guid of a Classification system.");

            InGenerics(
                "ClassificationItemGuids",
                "The Guids of Classification items to assign for the given attributes (input only 1 to use the same item for all attributes).");

            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to set the classification for.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ClassificationSystemValueList>(
                document,
                0);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each attribute (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out ClassificationGuid systemId))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> itemWrappers))
            {
                return;
            }

            var itemIds = new List<ClassificationGuid>();

            foreach (var wrapper in itemWrappers)
            {
                var itemId = ClassificationGuid.CreateFromWrapper(wrapper);
                if (itemId != null)
                {
                    itemIds.Add(itemId);
                }
            }

            if (!da.TryGetList(
                    2,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            if (itemIds.Count != 1 && attributeWrappers.Count != itemIds.Count)
            {
                this.AddError(
                    "The size of the input ClassificationItemGuids must be 1 or equal to the size of the input AttributeGuids.");
                return;
            }

            var input = new SetClassificationsOfAttributesParameters
            {
                AttributeClassifications = new List<AttributeClassificationObj>()
            };

            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var attributeId =
                    GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (attributeId == null)
                {
                    this.AddError("Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                var itemId = itemIds[itemIds.Count == 1 ? 0 : i];

                input.AttributeClassifications.Add(
                    new AttributeClassificationObj
                    {
                        AttributeId = attributeId,
                        Classification = new ClassificationObj
                        {
                            ClassificationSystemId = systemId,
                            ClassificationItemId = itemId.IsNullGuid()
                                ? null
                                : itemId
                        }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // Shares the icon of SetClassificationsOfElements.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("c3a9d2e6-7b14-4f58-9e0d-2a6f81b5c7d3");
    }
}
