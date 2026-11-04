"""Lists the custom fields of revision changes with GetRevisionChangeCustomScheme."""
import aclib

customScheme = aclib.RunTapirCommand ('GetRevisionChangeCustomScheme')['customScheme']

for field in customScheme:
    print (field['customSchemeKey'], field['customSchemeName'])
