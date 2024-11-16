using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class FilterElementsObj : ElementFiltersObj
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;
    }

    public class FilterElementsComponent : ArchicadAccessorComponent
    {
        public FilterElementsComponent ()
          : base (
                "Filter Elements",
                "FilterElems",
                "Filter elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to filter.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Filter", "Filter", "Element filter.", GH_ParamAccess.list, @default: new List<string> { ElementFilter.NoFilter.ToString () });
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "List of element ids matching the filter.", GH_ParamAccess.list);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new ElementFilterValueList ().AddAsSource (this, 1);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            List<string> filters = new List<string> ();
            if (!DA.GetDataList (1, filters)) {
                return;
            }

            FilterElementsObj filterElements = new FilterElementsObj () {
                Elements = inputElements.Elements,
                Filters = filters
            };

            if (filterElements.Filters == null) {
                DA.SetDataList (0, filterElements.Elements);
                return;
            }

            JObject filterElementsObj = JObject.FromObject (filterElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "FilterElements", filterElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ElementsObj elements = response.Result.ToObject<ElementsObj> ();
            DA.SetDataList (0, elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.FilterElems;

        public override Guid ComponentGuid => new Guid ("133ab85c-53f7-466d-8271-31c5518085e2");
    }
}