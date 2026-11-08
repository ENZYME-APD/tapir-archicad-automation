using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Components.AttributesComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class PickAttributeComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PickAttribute";

        public PickAttributeComponent()
            : base(
                "PickAttribute",
                "Opens a small dialog with Archicad's attribute picker for the given attribute type, waits for the " +
                "user and returns the chosen attribute, or cancelled. Archicad waits for the user, so it only runs " +
                "on the Execute button. Available from Archicad 26.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "AttributeType",
                "The type of the attribute to pick: Layer, BuildingMaterial, Composite, Profile or Surface; Fill " +
                "and Line need Archicad 27 or newer. Composite lists the composites only.");

            InGeneric(
                "AttributeGuid",
                "The attribute selected when the dialog opens. Optional: without it the picker starts empty and " +
                "OK is enabled once the user has picked an attribute.");

            InText(
                "Title",
                "The title of the dialog. Optional; default: Pick Attribute.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user closed the dialog with Cancel, false when with OK.");

            OutText(
                "AttributeType",
                "The type of the chosen attribute. Empty when cancelled.");

            OutGeneric(
                "AttributeGuid",
                "The chosen attribute. Empty when cancelled.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<AttributeTypeValueList>(
                document,
                0);
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

            var parameters = new JObject { ["attributeType"] = attributeType };

            GH_ObjectWrapper attributeWrapper = null;
            if (da.GetData(1, ref attributeWrapper) &&
                attributeWrapper != null)
            {
                var attributeId = GuidObject<AttributeGuidObject>.CreateFromWrapper(attributeWrapper);
                if (attributeId == null)
                {
                    this.AddError(
                        "Invalid attribute identifier in the AttributeGuid input.");
                    return;
                }

                parameters["attributeId"] = new JObject { ["guid"] = attributeId.Guid };
            }

            string title = null;
            if (da.GetData(2, ref title) &&
                !string.IsNullOrEmpty(title))
            {
                parameters["title"] = title;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (bool?)response["cancelled"]);

            da.SetData(
                1,
                response["attributeType"]?.ToString());

            var guid = JsonOutputHelp.GuidOf(response["attributeId"]);
            da.SetData(
                2,
                guid == null
                    ? null
                    : new AttributeGuidWrapper
                    {
                        AttributeId = new AttributeGuidObject { Guid = guid }
                    });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PickAttribute;

        public override Guid ComponentGuid =>
            new Guid("c11e5954-4f26-450e-82e2-7658b82a65c9");
    }
}
