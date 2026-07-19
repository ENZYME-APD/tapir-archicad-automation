using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetClassificationsOfElements";

        public GetClassificationsOfElementsComponent()
            : base(
                "GetClassificationsOfElements",
                "Get the classifications of the given elements in the given classification systems.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");

            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to query.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Classifications",
                "JSON object with the classifications of the given elements.");
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

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> systemWrappers))
            {
                return;
            }

            var systemIds = new List<ClassificationSystemGuidWrapper>();
            foreach (var wrapper in systemWrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification system identifier in the ClassificationSystemGuids input.");
                    return;
                }
                systemIds.Add(
                    new ClassificationSystemGuidWrapper { ClassificationSystemId = id });
            }

            var parameters = JObject.FromObject(elements);
            parameters["classificationSystemIds"] = JArray.FromObject(systemIds);

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
                response.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("44498153-a3d4-4b72-91ce-9cdbc236af0c");
    }
}
