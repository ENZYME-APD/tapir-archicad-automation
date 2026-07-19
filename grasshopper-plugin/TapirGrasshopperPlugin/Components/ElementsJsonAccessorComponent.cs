using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for accessor commands that take a list of elements and return
    // their response as a JSON string (used where the response schema is deeply
    // nested).
    public abstract class ElementsJsonAccessorComponent : ArchicadAccessorComponent
    {
        private readonly string _outputName;

        protected ElementsJsonAccessorComponent(
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

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
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
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
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
