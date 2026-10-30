import aclib

# Lists what the Wall favorites of the project would apply to a new element: their
# classifications, property values and element category values. A property value is
# only present when its status is HasValue; NotEvaluated and NotAvailable values are
# reported by their status alone. A favorite that cannot be read gets an error item.

favorites = aclib.RunTapirCommand ('GetFavoritesByType', {'elementType': 'Wall'}, debug = False)['favorites']

aclib.RunTapirCommand ('GetFavoriteDetails', {
    'favorites': favorites + ['Favorite that does not exist']
})
