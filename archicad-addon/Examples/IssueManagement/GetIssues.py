"""Lists the issues of the project with GetIssues."""
import aclib

issues = aclib.RunTapirCommand ('GetIssues')['issues']

for issue in issues:
    print (issue['name'], '-', issue['tagText'])
