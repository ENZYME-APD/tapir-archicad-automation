"""Prints the Magic Wand settings (how curves are approximated with segments, and the trace method) with GetMagicWandSettings."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetMagicWandSettings'))
