"""Lists the commands of one command group of the installed Tapir Add-On with GetCommands, and checks with aclib.HasCommand that a command exists before calling it."""
import aclib

result = aclib.RunTapirCommand ('GetCommands', {'group': 'Application Commands', 'includeDescriptions': True})
print ('Tapir ' + result['version'])
commands = {command['name'] : command for command in result['commands']}
aclib.PrintJson (commands['GetAddOnVersion'])

if aclib.HasCommand ('GetAddOnVersion', introducedIn = '0.1.0'):
    aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnVersion'))
