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
                "Highlight",
                "Highlight Elements.",
                "Elements"
            )
        {
        }

        public override void DocumentContextChanged (GH_Document document, GH_DocumentContext context) 
        {
            base.DocumentContextChanged (document, context);
            if (context == GH_DocumentContext.Close || context == GH_DocumentContext.Unloaded ) {
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
                MessageBox.Show("You cannot add this component more than one ...", "Highlight Elements");
            } else {
                base.AddedToDocument (document);
            }
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Elements to highlight.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("HighligtedColors", "Colors", "Colors for the Elements.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("NonHighligtedColor", "NHColor", "Color for the non-highlighted Elements.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("NonHighligtedWireframe", "NHWireframe3D", "Switch non-highlighted Elements in the 3D window to wireframe", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ClearHighlight", "Clear", "Clear highlight.", GH_ParamAccess.item);
            Params.Input[3].Optional = true;
            Params.Input[4].Optional = true;
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            bool clear = false;
            if (DA.GetData (4, ref clear) && clear) {
                ClearHighlight ();
                return;
            }

            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            List<GH_Colour> highlightedColors = new List<GH_Colour> ();
            if (!DA.GetDataList (1, highlightedColors)) {
                return;
            }

            GH_Colour nonHighlightedColor = new GH_Colour ();
            if (!DA.GetData (2, ref nonHighlightedColor)) {
                return;
            }

            bool wireframe3D = false;
            DA.GetData (3, ref wireframe3D);

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

        protected override void AppendAdditionalComponentMenuItems (ToolStripDropDown menu) {
            base.AppendAdditionalComponentMenuItems (menu);
            Menu_AppendItem (menu, "Clear Highlight", ClearButtonClicked);
        }

        private void ClearButtonClicked (object sender, EventArgs e)
        {
            ClearHighlight ();
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

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.HighlightElements;

        public override Guid ComponentGuid => new Guid ("9ba098dd-63c5-4126-b4cc-3caa56082c8f");
    }
}
