using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;
using System.Windows.Forms;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{

    public class HighlightElementsComponent : ArchicadAccessorComponent
    {
        public HighlightElementsComponent ()
          : base (
                "Highlight Elements",
                "HighlightElems",
                "Highlight Elements.",
                "Elements"
            )
        {
            Params.ParameterSourcesChanged += OnParameterSourcesChanged;
        }

        public override void DocumentContextChanged (GH_Document document, GH_DocumentContext context)
        {
            base.DocumentContextChanged (document, context);
            if (context == GH_DocumentContext.Close || context == GH_DocumentContext.Unloaded) {
                ClearHighlight ();
            }
        }

        public override void RemovedFromDocument (GH_Document document)
        {
            base.RemovedFromDocument (document);
            ClearHighlight ();
        }

        public override void AddedToDocument (GH_Document document)
        {
            int counter = 0;
            GH_Document doc = OnPingDocument ();
            if (doc != null) {
                foreach (IGH_DocumentObject obj in doc.Objects)
                    if (obj.ComponentGuid == ComponentGuid) {
                        counter++;
                        if (counter > 1) {
                            break;
                        }
                    }
            }

            if (counter > 1) {
                doc.RemoveObject (this, true);
                MessageBox.Show ("Only one instance can be created.", "Highlight Elements");
            } else {
                base.AddedToDocument (document);
            }
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddBooleanParameter ("Enable", "Enable", "Enable highlight.", GH_ParamAccess.item, @default: true);
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Elements to highlight.", GH_ParamAccess.list);
            pManager.AddColourParameter ("HighligtedColors", "Colors", "Colors for the Elements.", GH_ParamAccess.list);
            pManager.AddColourParameter ("NonHighligtedColor", "NHColor", "Color for the non-highlighted Elements.", GH_ParamAccess.item);
            pManager.AddBooleanParameter ("NonHighligtedWireframe", "NHWireframe3D", "Switch non-highlighted Elements in the 3D window to wireframe", GH_ParamAccess.item, @default: false);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            bool enabled = true;
            if (DA.GetData (0, ref enabled) && !enabled) {
                ClearHighlight ();
                return;
            }

            ElementsObj inputElements = ElementsObj.Create (DA, 1);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            List<GH_Colour> highlightedColors = new List<GH_Colour> ();
            if (!DA.GetDataList (2, highlightedColors)) {
                return;
            }

            GH_Colour nonHighlightedColor = new GH_Colour ();
            if (!DA.GetData (3, ref nonHighlightedColor)) {
                return;
            }

            bool wireframe3D = false;
            if (!DA.GetData (4, ref wireframe3D)) {
                return;
            }

            HighlightElementsObj highlightElements = new HighlightElementsObj () {
                Elements = inputElements.Elements,
                HighlightedColors = Utilities.Convert.ToColors (highlightedColors, inputElements.Elements.Count),
                NonHighlightedColor = Utilities.Convert.ToColor (nonHighlightedColor),
                Wireframe3D = wireframe3D
            };
            JObject highlightElementsObj = JObject.FromObject (highlightElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "HighlightElements", highlightElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        private void ClearHighlight ()
        {
            HighlightElementsObj highlightElements = new HighlightElementsObj ();
            JObject highlightElementsObj = JObject.FromObject (highlightElements);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "HighlightElements", highlightElementsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        public void OnParameterSourcesChanged (object source, EventArgs e)
        {
            foreach (IGH_Param param in Params.Input) {
                if (!param.Optional && param.SourceCount == 0) {
                    ClearHighlight ();
                    break;
                }
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.HighlightElements;

        public override Guid ComponentGuid => new Guid ("9ba098dd-63c5-4126-b4cc-3caa56082c8f");
    }
}
