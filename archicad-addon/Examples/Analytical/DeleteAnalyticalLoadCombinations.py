"""Creates a structural analysis load combination and deletes it again with DeleteAnalyticalLoadCombinations."""
import aclib

loadCombinationId = aclib.RunTapirCommand ('CreateAnalyticalLoadCombinations', {
    'loadCombinations': [{'name': 'Tapir Temporary', 'category': 'SLS'}]
})['loadCombinationIds'][0]['loadCombinationId']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteAnalyticalLoadCombinations', {
    'loadCombinationIds': [{'loadCombinationId': loadCombinationId}]
}))
