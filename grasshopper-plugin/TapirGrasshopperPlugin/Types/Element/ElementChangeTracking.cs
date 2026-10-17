using Newtonsoft.Json;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Element
{
    // Identifier of an element state captured by CaptureElementState. The
    // state lives in the add-on's memory only, so the identifier is valid in
    // the Archicad session and project it was captured in, until it is
    // released.
    public class ElementStateGuid : GuidObject<ElementStateGuid>
    {
    }

    public static class ElementStateTypes
    {
        public const string ModificationStamp = "ModificationStamp";
        public const string Model3D = "3DModel";
        public const string Context = "Context";

        // In the order of the command's enum; the first one is the default.
        public static readonly string[] All =
        {
            ModificationStamp,
            Model3D,
            Context
        };
    }

    public class CaptureElementStateParameters
    {
        [JsonProperty(
            "type",
            NullValueHandling = NullValueHandling.Ignore)]
        public string Type;
    }

    public class CaptureElementStateResponse
    {
        [JsonProperty("stateId")]
        public ElementStateGuid StateId;

        [JsonProperty("type")]
        public string Type;
    }

    public class GetElementChangesSinceParameters
    {
        [JsonProperty("stateId")]
        public ElementStateGuid StateId;

        [JsonProperty(
            "toStateId",
            NullValueHandling = NullValueHandling.Ignore)]
        public ElementStateGuid ToStateId;
    }

    public class ElementChangesObject
    {
        [JsonProperty("newElements")]
        public List<ElementGuidWrapper> NewElements;

        [JsonProperty("modifiedElements")]
        public List<ElementGuidWrapper> ModifiedElements;

        [JsonProperty("deletedElements")]
        public List<ElementGuidWrapper> DeletedElements;
    }

    public class ReleaseElementStateParameters
    {
        [JsonProperty("stateId")]
        public ElementStateGuid StateId;
    }
}
