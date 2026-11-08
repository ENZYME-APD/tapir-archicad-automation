using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class RenameAttributesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameAttributes";

        public RenameAttributesComponent()
            : base(
                "RenameAttributes",
                "Renames attributes of any type (layers, layer combinations, composites, surfaces, ...). Each " +
                "attribute keeps its index, its id and its settings. A new name that another attribute of the same " +
                "type already has is refused, and so is renaming the attributes Archicad does not allow to change " +
                "(the built-in Archicad Layer, the Solid and Empty fills).",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "Identifiers of the attributes to rename.");

            InTexts(
                "NewNames",
                "The new name of each attribute. It must not be empty or the name of another attribute of the same type.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each attribute (empty when it was renamed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> attributeWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> newNames))
            {
                return;
            }

            if (newNames.Count != attributeWrappers.Count)
            {
                this.AddError(
                    "The count of NewNames must be the same as the count of AttributeGuids.");
                return;
            }

            var attributes = new JArray();
            for (var i = 0; i < attributeWrappers.Count; i++)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrappers[i]);
                if (attributeId == null)
                {
                    this.AddError(
                        "Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                attributes.Add(
                    new JObject
                    {
                        ["attributeId"] = new JObject { ["guid"] = attributeId.Guid },
                        ["newName"] = newNames[i] ?? ""
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["attributes"] = attributes },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameAttributes;

        public override Guid ComponentGuid =>
            new Guid("fa1414c6-c583-499f-946e-df4fd1f79096");
    }
}
