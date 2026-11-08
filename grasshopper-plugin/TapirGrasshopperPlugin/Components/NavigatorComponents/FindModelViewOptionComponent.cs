using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class FindModelViewOptionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "FindModelViewOption";

        public FindModelViewOptionComponent()
            : base(
                "FindModelViewOption",
                "Finds a model view option combination by id or by name and returns its index, id, name and " +
                "flags. The index is the one Archicad uses to switch the model view options of a window to the " +
                "combination. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ModelViewOptionGuid",
                "The identifier (GUID) of the model view option combination to find. Give this or Name.");

            InText(
                "Name",
                "The name of the model view option combination to find, as shown in " +
                "Document > Model View > Model View Options. Give this or ModelViewOptionGuid.");

            SetOptionality(
                new[]
                {
                    0,
                    1
                });
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "Index",
                "The index of the model view option combination.");

            OutText(
                "ModelViewOptionGuid",
                "The identifier of the model view option combination.");

            OutText(
                "Name",
                "The name of the model view option combination.");

            OutInteger(
                "Flags",
                "The flags of the model view option combination, as Archicad returns them.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            da.TryGet(
                0,
                out string guid);

            da.TryGet(
                1,
                out string name);

            var hasGuid = !string.IsNullOrWhiteSpace(guid);
            var hasName = !string.IsNullOrEmpty(name);

            if (hasGuid == hasName)
            {
                this.AddError(
                    "Give exactly one of ModelViewOptionGuid and Name.");
                return;
            }

            var parameters = new JObject();

            if (hasGuid)
            {
                parameters["modelViewOptionId"] = NavigatorComponentHelp.IdObject(guid.Trim());
            }
            else
            {
                parameters["name"] = name;
            }

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
                response.Value<int?>("index"));

            da.SetData(
                1,
                JsonOutputHelp.GuidOf(response["modelViewOptionId"]));

            da.SetData(
                2,
                response["name"]?.ToString());

            da.SetData(
                3,
                response.Value<int?>("flags"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.FindModelViewOption;

        public override Guid ComponentGuid =>
            new Guid("3b26cdb4-00dc-4b67-8f4c-2d9372292b7c");
    }
}
