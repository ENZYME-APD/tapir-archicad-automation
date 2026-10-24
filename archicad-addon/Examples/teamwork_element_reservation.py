import aclib

# Who has an element reserved can be asked without trying to reserve it: the status is read
# from the element header, so nothing changes in the project. In a solo project the command
# reports an error, because only Teamwork elements have a reservation.
walls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Wall'
    })['elements']
elements = walls[:1]

statuses = aclib.RunTapirCommand (
    'GetReservationStatusOfElements', {
        'elements': elements
    })

if statuses is not None and 'reservationStatuses' in statuses:
    for element, status in zip (elements, statuses['reservationStatuses']):
        if 'error' in status:
            print ('{}: {}'.format (element['elementId']['guid'], status['error']['message']))
        elif 'reservedBy' in status:
            print ('{}: {} (reserved by {})'.format (element['elementId']['guid'], status['status'], status['reservedBy']['userName']))
        else:
            print ('{}: {}'.format (element['elementId']['guid'], status['status']))

# Hand the elements over to a colleague by Teamwork user id: GetReservationStatusOfElements
# and the conflicts of ReserveElements report the ids of the users who hold reservations, and
# GetTeamworkProjectInfo the id of the current user. The elements must be reserved by the
# current user; no dialog is shown.
targetUserId = 2

aclib.RunTapirCommand (
    'GrantElements', {
        'elements': elements,
        'userId': targetUserId
    })
