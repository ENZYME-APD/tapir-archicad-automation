import aclib

# Tracing hotlinked elements back to their source file.
#
# The elements inside a placed hotlink instance are proxies with guids of their
# own. GetHotlinkProxyElementTable gives, per instance, the element of the
# source file each proxy was made from. It takes the INSTANCES (elements of
# type Hotlink), not the nodes GetHotlinks returns.

instances = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Hotlink'})['elements']

if not instances:
    print ('No placed hotlink instances in this project.')
else:
    aclib.RunTapirCommand ('GetHotlinkProxyElementTable', {
        'hotlinkInstances': instances
    })
