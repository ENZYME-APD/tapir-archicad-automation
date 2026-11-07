import aclib

# Works in every project: in a solo project isTeamwork is false and the other Teamwork
# fields are left out. In a Teamwork project it also tells whether the project is online
# and who the current user is, so a script can decide whether to reserve or hand over
# elements before it changes anything.
info = aclib.RunTapirCommand ('GetTeamworkProjectInfo')

if info is not None and info.get ('isTeamwork'):
    state = 'online' if info['isOnline'] else 'offline'
    print ('Teamwork project {} on {} is {}, current user: {} ({})'.format (
        info.get ('projectName', '?'), info.get ('serverUrl', '?'), state,
        info.get ('userName', '?'), info.get ('userId', '?')))
