"""Calculates the sun's azimuth and altitude at noon on 21 June at the project location with CalculateSunPosition.

Fields left out keep the project's value. The project is not changed.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CalculateSunPosition', {
    'dateTime': {'month': 6, 'day': 21, 'hour': 12, 'minute': 0, 'second': 0}
}))
