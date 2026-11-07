"""Lists the Interactive Schedules of the Project Map with GetSchedules.

Each line shows the folders the schedule is in, its ID and its name.
"""
import aclib

schedules = aclib.RunTapirCommand ('GetSchedules')['schedules']
for schedule in schedules:
    print ('{} | {} {}'.format (' / '.join (schedule['folderPath']), schedule['uiId'], schedule['name']))
