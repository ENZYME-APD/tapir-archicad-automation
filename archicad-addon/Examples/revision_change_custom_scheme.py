import aclib

change_custom_scheme = aclib.RunTapirCommand('GetRevisionChangeCustomScheme')

field_names = {field['customSchemeKey']: field['customSchemeName'] for field in change_custom_scheme['customScheme']}

revision_changes = aclib.RunTapirCommand('GetRevisionChanges')
for change in revision_changes['revisionChanges']:
    for data in change.get('customSchemeData', []):
        key = data['customSchemeKey']
        print('{}: {} = {}'.format(change['id'], field_names.get(key, key), data['customSchemeValue']))
