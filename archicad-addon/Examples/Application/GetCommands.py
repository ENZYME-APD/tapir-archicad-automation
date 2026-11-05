"""Lists the commands of the Tapir Add-On with GetCommands and checks that a command exists before calling it."""
import aclib

commands = {command['name'] : command for command in aclib.RunTapirCommand ('GetCommands')['commands']}

if 'GetAddOnVersion' in commands:
    aclib.PrintJson (commands['GetAddOnVersion'])
    aclib.PrintJson (aclib.RunTapirCommand ('GetAddOnVersion'))
