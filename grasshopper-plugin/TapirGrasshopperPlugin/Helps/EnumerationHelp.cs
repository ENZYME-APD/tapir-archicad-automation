using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Helps
{
    public static class EnumerationHelp
    {
        public static void MultiplyTo<T>(
            this List<T> values,
            int targetCount)
        {
            if (targetCount == values.Count)
            {
                return;
            }
            else if (values.Count == 1)
            {
                values = Enumerable
                    .Repeat(
                        values[0],
                        targetCount)
                    .ToList();
            }
            else
            {
                throw new Exception(
                    $"The input value count ({values.Count}) " +
                    $"neither equals the target count ({targetCount}) - " +
                    $"nor is it 1 to repeat as many times as the target count!" +
                    $"Input multiplication failed.");
            }
        }
    }
}