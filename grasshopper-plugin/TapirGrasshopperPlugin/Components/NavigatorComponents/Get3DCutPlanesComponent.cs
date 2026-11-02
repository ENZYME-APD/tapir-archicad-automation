using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DCutPlanesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DCutPlanes";

        public Get3DCutPlanesComponent()
            : base(
                "Get3DCutPlanes",
                "Gets whether the 3D cut planes are on and the cut planes (pa, pb, pc, pd) of the 3D window, in " +
                "the form accepted by Set3DCutPlanes.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "IsCutPlanes",
                "True when the 3D cut planes are switched on.");

            OutBoolean(
                "UseCustom",
                "True when the custom cut settings are used.");

            OutNumberList(
                "PA",
                "Coefficient of x (x of the plane normal) of each cut plane: pa*x + pb*y + pc*z + pd = 0.");

            OutNumberList(
                "PB",
                "Coefficient of y (y of the plane normal) of each cut plane.");

            OutNumberList(
                "PC",
                "Coefficient of z (z of the plane normal) of each cut plane.");

            OutNumberList(
                "PD",
                "Constant term of each cut plane, in meters.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var pa = new List<double>();
            var pb = new List<double>();
            var pc = new List<double>();
            var pd = new List<double>();

            foreach (var plane in JsonOutputHelp.Items(
                         response,
                         "cutPlanes"))
            {
                pa.Add(plane.Value<double?>("pa") ?? 0.0);
                pb.Add(plane.Value<double?>("pb") ?? 0.0);
                pc.Add(plane.Value<double?>("pc") ?? 0.0);
                pd.Add(plane.Value<double?>("pd") ?? 0.0);
            }

            da.SetData(
                0,
                response.Value<bool?>("isCutPlanes") ?? false);

            da.SetData(
                1,
                response.Value<bool?>("useCustom") ?? false);

            da.SetDataList(2, pa);
            da.SetDataList(3, pb);
            da.SetDataList(4, pc);
            da.SetDataList(5, pd);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DCutPlanes;

        public override Guid ComponentGuid =>
            new Guid("cb403cb5-9c91-4a50-af3c-bc683ed72d18");
    }
}
