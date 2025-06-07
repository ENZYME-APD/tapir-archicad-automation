using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementFiltersObj : AcceptsElementFilters
    {
        [JsonProperty ("filters", NullValueHandling = NullValueHandling.Ignore)]
        private List<string> filters;

        [JsonIgnore]
        public List<string> Filters
        {
            get => filters;
            set => filters = AcceptElementFilters (value);
        }

        [JsonProperty ("databases", NullValueHandling = NullValueHandling.Ignore)]
        public List<DatabaseIdItemObj> Databases;
    }


    public class GetAllElementsComponent : ArchicadAccessorComponent
    {
        public GetAllElementsComponent ()
          : base (
                "All Elements",
                "AllElems",
                "Get all elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Filter", "Filter", "Element filter.", GH_ParamAccess.list, @default: new List<string> { ElementFilter.NoFilter.ToString () });
            pManager.AddGenericParameter ("Databases", "Databases", "Databases to find elements.", GH_ParamAccess.list);

            Params.Input[1].Optional = true;
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "List of element Guids matching the filter.", GH_ParamAccess.list);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            List<string> filters = new List<string> ();
            if (!DA.GetDataList (0, filters)) {
                return;
            }
            DatabasesObj databases = DatabasesObj.Create (DA, 1);

            ElementFiltersObj elementFilters = new ElementFiltersObj () {
                Filters = filters.Count > 0 ? filters : null,
                Databases = databases is null || databases.Databases.Count == 0 ? null : databases.Databases
            };
            JObject elementFiltersObj = JObject.FromObject (elementFilters);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAllElements", elementFiltersObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ElementsObj elements = response.Result.ToObject<ElementsObj> ();
            DA.SetDataList (0, elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AllElems;

        public override Guid ComponentGuid => new Guid ("61085af7-4f11-49be-bd97-00effddf90af");
    }
}