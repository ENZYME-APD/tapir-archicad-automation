import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src'))
from tapir_py import core

ac = core.Command.create()

print('CreateIssue')
ac.CreateIssue('Python Issue', '', 'api')

# print('DeleteIssue')
# ac.DeleteIssue('B0D7E9D6-56E8-4432-AC74-DE7C3083015F')

# print('GetIssues')
# issues = ac.GetIssues()
# for issue in issues:
# 	print(' ')
# 	print('guid: {0}'.format(issue['guid']))
# 	print('name: {0}'.format(issue['name']))
# 	print('parentGuid: {0}'.format(issue['parentGuid']))
# 	print('creaTime: {0}'.format(issue['creaTime']))
# 	print('modiTime: {0}'.format(issue['modiTime']))
# 	print('tagText: {0}'.format(issue['tagText']))
# 	print('tagTextElemGuid: {0}'.format(issue['tagTextElemGuid']))
# 	print('isTagTextElemVisible: {0}'.format(issue['isTagTextElemVisible']))

# print('AddComment')
# ac.AddComment('B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 'Just a new comment', 'python', 1)

# print('GetComments')
# comments = ac.GetComments('B0D7E9D6-56E8-4432-AC74-DE7C3083015F')
# for comment in comments:
# 	print(' ')
# 	print('guid: {0}'.format(comment['guid']))
# 	print('author: {0}'.format(comment['author']))
# 	print('text: {0}'.format(comment['text']))
# 	print('status: {0}'.format(comment['status']))
# 	print('creaTime: {0}'.format(comment['creaTime']))

# print('AttachElements')
# ac.AttachElements('B0D7E9D6-56E8-4432-AC74-DE7C3083015F', ['22F534EF-F3CE-4A99-9E54-125E899E6AA2', '672D79F7-382A-4A10-A93C-BD28359BAD29', 'D2A0D664-1D90-4727-B766-65676B157689'], 1)

# print('DetachElements')
# ac.DetachElements('B0D7E9D6-56E8-4432-AC74-DE7C3083015F', ['22F534EF-F3CE-4A99-9E54-125E899E6AA2', '672D79F7-382A-4A10-A93C-BD28359BAD29', 'D2A0D664-1D90-4727-B766-65676B157689'])

# print('GetAttachedElements')
# elements = ac.GetAttachedElements('B0D7E9D6-56E8-4432-AC74-DE7C3083015F', 1)
# for element in elements:
# 	print('guid: {0}'.format(element['guid']))

# print('ExportToBCF (Selected Issues)')
# ac.ExportToBCF(['B0D7E9D6-56E8-4432-AC74-DE7C3083015F', '4765CCCA-D30E-4A46-92B3-F544F936E089'], 'C:\\Users\\i.yurasov\\Desktop\\dev\\issues_selected.bcfzip', False, True)

# print('ExportToBCF (All Issues)')
# ac.ExportToBCF(None, 'C:\\Users\\i.yurasov\\Desktop\\dev\\issues_all.bcfzip', False, True)

# print('ImportFromBCF')
# ac.ImportFromBCF('C:\\Users\\i.yurasov\\Desktop\\dev\\issues_test5.bcfzip', True)