using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for parameterless accessor commands whose response is returned
    // as a JSON string (used where the response schema is deeply nested).
    public abstract class NoInputJsonAccessorComponent : ArchicadAccessorComponent
    {
        private readonly string _outputName;

        protected NoInputJsonAccessorComponent(
            string name,
            string description,
            string subCategory,
            string outputName)
            : base(
                name,
                description,
                subCategory)
        {
            _outputName = outputName;
        }

        protected override void AddOutputs()
        {
            OutText(
                _outputName,
                "JSON object with the response of the command.");
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

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }
    }
}
