using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class TrimElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "TrimElements";

        public TrimElementsComponent()
            : base(
                "TrimElements",
                "Trim construction elements to a roof or shell. Without a trimming element, " +
                "the roofs and shells among the given elements do the trimming.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to trim.");

            InGeneric(
                "TrimmingElementGuid",
                "Identifier of the roof or shell that trims every given element. Optional.");
            SetOptionality(1);

            InText(
                "TrimType",
                "Which side the elements keep: KeepInside, KeepOutside, KeepAll or No. " +
                "Used with TrimmingElementGuid; defaults to KeepInside. Optional.");
            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            var elements = new JArray();
            foreach (var wrapper in elementWrappers)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }
                elements.Add(new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                });
            }

            var parameters = new JObject { ["elements"] = elements };

            var trimmingWrapper = new GH_ObjectWrapper();
            if (da.GetData(1, ref trimmingWrapper) && trimmingWrapper?.Value != null)
            {
                var trimmingId = GuidObject<ElementGuid>.CreateFromWrapper(trimmingWrapper);
                if (trimmingId == null)
                {
                    this.AddError("Invalid element identifier in the TrimmingElementGuid input.");
                    return;
                }
                parameters["trimmingElement"] = new JObject { ["guid"] = trimmingId.Guid };
            }

            var trimType = string.Empty;
            if (da.GetData(2, ref trimType) && !string.IsNullOrWhiteSpace(trimType))
            {
                parameters["trimType"] = trimType;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TrimElements;

        public override Guid ComponentGuid =>
            new Guid("4ce14b06-7a83-97a5-6b18-0bdc36400305");
    }
}
