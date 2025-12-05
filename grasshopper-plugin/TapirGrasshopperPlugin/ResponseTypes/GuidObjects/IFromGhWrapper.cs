namespace TapirGrasshopperPlugin.ResponseTypes.GuidObjects
{
    public interface IFromGhWrapper
    {
        bool TryCreateFromGhWrapper(
            object wrapper);
    }
}