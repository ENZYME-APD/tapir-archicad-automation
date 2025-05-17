using Grasshopper.Kernel;
using Grasshopper.Kernel.Special;
using Grasshopper.GUI;
using System;
using System.Drawing;
using System.Windows.Forms;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.Components.IssuesComponents
{
    public enum IssueElementType
    {
        Creation,
        Highlight,
        Deletion,
        Modification
    }

    public class IssueElementTypeValueList : ValueList
    {
        public IssueElementTypeValueList () :
            base ("IssueElementType", "", "Value List for Issue Element Types.", "Issues")
        {
        }

        public override void RefreshItems ()
        {
            ListItems.Clear ();
            AddEnumItems (defaultSelected: IssueElementType.Highlight);
            ExpireSolution (true);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.IssueElementType;

        public override Guid ComponentGuid => new Guid ("b40e2d8e-4aa4-467e-b0f8-af2f65fb2df4");
    }
}
