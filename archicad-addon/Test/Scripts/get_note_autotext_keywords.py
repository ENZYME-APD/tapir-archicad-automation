import aclib

# Look up the keywords of note autotexts (the autotexts of dimension, label and other element
# notes) by the numeric values of the API_NoteAutotextID enumeration. The keywords depend on the
# Archicad version and the UI language, so the script prints what it checked rather than the
# raw keywords. Archicad 25 does not have this API and answers with an error.

failed = 0
def check (label, ok):
    global failed
    print ('{}: {}'.format (label, 'OK' if ok else 'FAILED'))
    failed += 0 if ok else 1

autoTextIds = list (range (0, 10))
keywords = aclib.RunTapirCommand ('GetNoteAutoTextKeywords', {'autoTextIds': autoTextIds}, debug = False)['keywords']
check ('one result per id', len (keywords) == len (autoTextIds))
check ('every result is a keyword or an error', all (('keyword' in k) != ('error' in k) for k in keywords))
check ('some ids have a keyword', any ('keyword' in k for k in keywords))

print ('get_note_autotext_keywords: {} check(s) failed'.format (failed))
