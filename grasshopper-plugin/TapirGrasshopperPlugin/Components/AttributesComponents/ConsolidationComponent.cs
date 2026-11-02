using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    // Shared base of ConsolidateLines and ConsolidateFills: both run a menu
    // command of Archicad through a JSON query and answer with an
    // executionResult and, when the query ran, Archicad's answer in result.
    public abstract class ConsolidationComponent : ArchicadExecutorComponent
    {
        protected ConsolidationComponent(
            string name,
            string description)
            : base(
                name,
                description,
                GroupNames.Attributes)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Success",
                "True if the menu command ran.");

            OutText(
                "ErrorMessage",
                "Error message when the menu command could not be run (empty when it ran).");

            OutText(
                "Result",
                "What Archicad answered, as it came. Its content is not documented by the Archicad API; " +
                "it may report a failure of the menu command even though it ran.");
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

            var executionResult = response["executionResult"];
            da.SetData(
                0,
                (bool?)executionResult?["success"] == true);

            da.SetData(
                1,
                (bool?)executionResult?["success"] == true
                    ? ""
                    : executionResult?["error"]?["message"]?.ToString() ?? "Unknown error.");

            da.SetData(
                2,
                response["result"]?.ToString());
        }
    }
}
