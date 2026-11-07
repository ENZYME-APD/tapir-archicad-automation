"""Lists the elements of every design option of the project with GetElementsOfDesignOptions.

Requires Archicad 29 or later.
"""
import aclib

designOptions = aclib.RunTapirCommand ('GetDesignOptions')['designOptions']
if not designOptions:
    raise SystemExit ('No design option in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsOfDesignOptions', {
    'designOptions': [{'designOptionId': d['designOptionId']} for d in designOptions]
}))
