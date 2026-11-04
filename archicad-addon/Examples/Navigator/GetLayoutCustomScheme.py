"""Lists the custom fields of the Layout Info Panel (Book Settings) with GetLayoutCustomScheme."""
import aclib

customScheme = aclib.RunTapirCommand ('GetLayoutCustomScheme')['customScheme']

for field in customScheme:
    print (field['customSchemeKey'], field['customSchemeName'])
