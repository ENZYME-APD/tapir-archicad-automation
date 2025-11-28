using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Security.Cryptography.X509Certificates;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class LabelData
    {
        [JsonProperty ("parentElementId")]
        public ElementIdObj ParentElementId;
    }

    public class CreateLabelsParameters
    {
        [JsonProperty ("labelsData")]
        public List<LabelData> LabelsData;
    }

    public class LabelElementsComponent : ArchicadExecutorComponent
    {
        public LabelElementsComponent ()
          : base (
                "Label Elements",
                "LabelElements",
                "Label elements",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element ids to create label for.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj elements = ElementsObj.Create (DA, 0);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            JObject parametersObj = JObject.FromObject (new CreateLabelsParameters () {
                LabelsData = elements.Elements.ConvertAll (element => new LabelData () { ParentElementId = element.ElementId })
            });
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "CreateLabels", parametersObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.LabelElements;

        public override Guid ComponentGuid => new Guid ("ecdb0a59-f928-4ed3-88e1-cd9aea737b39");
    }
}
