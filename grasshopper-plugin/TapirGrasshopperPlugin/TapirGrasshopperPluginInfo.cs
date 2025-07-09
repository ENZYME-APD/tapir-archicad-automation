using Grasshopper;
using Grasshopper.Kernel;
using System;
using System.Drawing;

namespace Tapir
{
    public class TapirGrasshopperPluginInfo : GH_AssemblyInfo
    {
        public override string Name => "Tapir";

        //Return a 24x24 pixel bitmap to represent this GHA library.
        public override Bitmap Icon => null;

        //Return a short string describing the purpose of this GHA library.
        public override string Description => "Grasshopper components for connecting to Archicad.";

        public override Guid Id => new Guid ("54b11162-627b-455b-b9c0-963e76b36dc7");

        //Return a string identifying you or your company.
        public override string AuthorName => "Enzyme APD";

        //Return a string representing your preferred contact details.
        public override string AuthorContact => "https://github.com/ENZYME-APD/tapir-archicad-automation";

        //Return a string representing the version.  This returns the same version as the assembly.
        public override string AssemblyVersion => "1.2.0";
    }
}
