﻿using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ElementsByTypeObj : AcceptsElementFilters
    {
        [JsonProperty ("elementType")]
        public string ElementType;

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


    public class GetElementsByTypeComponent : ArchicadAccessorComponent
    {
        public GetElementsByTypeComponent ()
          : base (
                "Elems By Type",
                "ElemsByType",
                "Get all elements by type.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Type", "Type", "Element type.", GH_ParamAccess.item);
            pManager.AddTextParameter ("Filter", "Filter", "Element filter.", GH_ParamAccess.list, @default: new List<string> { ElementFilter.NoFilter.ToString () });
            pManager.AddGenericParameter ("Databases", "Databases", "Databases to find elements.", GH_ParamAccess.list);

            Params.Input[2].Optional = true;
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "List of element Guids matching the type and the filter.", GH_ParamAccess.list);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new ElementTypeValueList ().AddAsSource (this, 0);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string elemType = "";
            if (!DA.GetData (0, ref elemType)) {
                return;
            }

            List<string> filters = new List<string> ();
            if (!DA.GetDataList (1, filters)) {
                return;
            }

            DatabasesObj databases = DatabasesObj.Create (DA, 2);

            ElementsByTypeObj elementsByType = new ElementsByTypeObj () {
                ElementType = elemType,
                Filters = filters is null || filters.Count == 0 ? null : filters,
                Databases = databases is null || databases.Databases.Count == 0 ? null : databases.Databases
            };
            JObject elementyByTypeObj = JObject.FromObject (elementsByType);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetElementsByType", elementyByTypeObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            ElementsObj elements = response.Result.ToObject<ElementsObj> ();
            DA.SetDataList (0, elements.Elements);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ElemsByType;

        public override Guid ComponentGuid => new Guid ("8075031e-b38d-4f3b-8e5c-8e740d13a091");
    }
}