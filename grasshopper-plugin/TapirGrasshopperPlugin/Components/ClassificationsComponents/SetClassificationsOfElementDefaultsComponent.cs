using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class SetClassificationsOfElementDefaultsComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName =>
            "SetClassificationsOfElementDefaults";

        public SetClassificationsOfElementDefaultsComponent()
            : base(
                "SetClassificationsOfDefaults",
                "Sets the classifications of the tool defaults (the settings new elements get) of element types, " +
                "so every element placed afterwards gets them. " +
                "Leave ClassificationItemGuids empty (or give the null guid) to set the defaults to unclassified in the system.",
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
                "Single Guid or list of Guids of Classification items to assign for the corresponding element types. " +
                "Empty sets every given default to unclassified.");

            InTexts(
                "ElementTypes",
                "Element types whose defaults to set the classification for.");

            SetOptionality(1);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            // AddAsSource<T> also calls base.AddedToDocument, so it is used
            // once and the second value list is connected directly.
            AddAsSource<ClassificationSystemValueList>(
                document,
                0);

            new ElementTypeValueList().AddAsSource(
                this,
                2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element type (empty when it succeeded).");
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

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> wrappers);

            var guids = new List<ClassificationGuid>();

            foreach (var wrapper in wrappers)
            {
                var itemId = ClassificationGuid.CreateFromWrapper(wrapper);
                if (itemId == null)
                {
                    this.AddError(
                        "Invalid classification item identifier in the ClassificationItemGuids input.");
                    return;
                }
                guids.Add(itemId);
            }

            if (!da.TryGetList(
                    2,
                    out List<string> elementTypes))
            {
                return;
            }

            if (guids.Count > 1 && elementTypes.Count != guids.Count)
            {
                this.AddError(
                    "The count of ClassificationItemGuids must be 0, 1 or the same as the count of ElementTypes.");
                return;
            }

            var input = new SetClassificationsOfElementDefaultsParameters
            {
                ElementDefaultClassifications =
                    new List<ElementDefaultClassificationObj>()
            };

            for (var i = 0; i < elementTypes.Count; i++)
            {
                var classificationItemId = guids.Count == 0
                    ? null
                    : guids.Count == 1
                        ? guids[0]
                        : guids[i];

                input.ElementDefaultClassifications.Add(
                    new ElementDefaultClassificationObj
                    {
                        ElementType = elementTypes[i],
                        Classification = new ClassificationObj
                        {
                            ClassificationSystemId = systemId,
                            ClassificationItemId =
                                classificationItemId == null ||
                                classificationItemId.IsNullGuid()
                                    ? null
                                    : classificationItemId
                        }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetClassifications;

        public override Guid ComponentGuid =>
            new Guid("a81d4c7e-26f3-4b90-9e5a-c37f0b8d6142");
    }
}
