/* Italiano: the Share page (scripts/share/index.html). See ../README.md.
   Channel, label, file and code names, and the palette's Share command, stay English. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Share an Archicad script | Tapir': 'Condividi uno script per Archicad | Tapir',
    'Share an Archicad script you wrote: send it on Discord or as a pull request, and it is checked, adapted to work in any office and listed with your name.':
        'Condividi uno script per Archicad che hai scritto: invialo su Discord o come pull request, e viene verificato, adattato per funzionare in qualsiasi studio e pubblicato con il tuo nome.',
    '<a href="../../">Tapir</a> / <a href="../">Scripts</a> / Share': '<a href="../../">Tapir</a> / <a href="../">Script</a> / Condividi',
    'Share a script <em>from Archicad.</em>': 'Condividi uno script <em>da Archicad.</em>',
    'No GitHub account, no setup. Your name stays on it, and everyone who uses Tapir can install it.':
        'Nessun account GitHub, nessuna configurazione. Il tuo nome resta sullo script, e chiunque usi Tapir può installarlo.',

    // What happens
    'What happens': 'Cosa succede',
    '<b aria-hidden="true">1</b><strong>Send it</strong><span>Post the script in <code>#share-a-script</code> on Discord, with a few words on what it does. The Tapir palette is getting a Share button that does the same: you see the whole file before anything is sent. It arrives as a GitHub issue labelled <code>script-share</code>.</span>':
        '<b aria-hidden="true">1</b><strong>Invialo</strong><span>Pubblica lo script in <code>#share-a-script</code> su Discord, con due parole su cosa fa. La palette di Tapir sta per avere un pulsante Share che fa lo stesso: vedi l\'intero file prima che venga inviato qualsiasi cosa. Arriva come issue GitHub con l\'etichetta <code>script-share</code>.</span>',
    '<b aria-hidden="true">2</b><strong>Adapted for any office</strong><span>Tapir\'s bot, an AI, opens a pull request: hard-coded layers, properties and paths become settings, risky code is removed, and it works in every Archicad language, in metric and imperial.</span>':
        '<b aria-hidden="true">2</b><strong>Adattato a qualsiasi studio</strong><span>Il bot di Tapir, un\'IA, apre una pull request: layer, proprietà e percorsi scritti nel codice diventano impostazioni, il codice rischioso viene rimosso, e lo script funziona in ogni lingua di Archicad, nel sistema metrico e in quello imperiale.</span>',
    '<b aria-hidden="true">3</b><strong>You confirm, it\'s listed</strong><span>Try the adapted version and say it still does what you meant. Once the pull request is reviewed and merged, it appears on the <a href="../">Scripts</a> page with your name.</span>':
        '<b aria-hidden="true">3</b><strong>Tu confermi, e viene pubblicato</strong><span>Prova la versione adattata e di\' se fa ancora quello che intendevi. Quando la pull request è stata revisionata e unita, compare nella pagina <a href="../">Script</a> con il tuo nome.</span>',

    // Three ways
    'Three ways to send it': 'Tre modi per inviarlo',
    'Coming to the palette': 'Presto nella palette',
    'From the Tapir palette': 'Dalla palette di Tapir',
    'Right-click your script and choose Share: add a few words about it and send. Until your palette has it, use Discord.':
        'Fai clic destro sul tuo script e scegli Share: aggiungi due parole e invia. Finché la tua palette non lo ha, usa Discord.',
    'Easiest today': 'Il più semplice oggi',
    'On Discord': 'Su Discord',
    'Post the script in the <code>#share-a-script</code> channel, with a few words on what it does and which Archicad version you use.':
        'Pubblica lo script nel canale <code>#share-a-script</code>, con due parole su cosa fa e quale versione di Archicad usi.',
    'Join Discord': 'Entra su Discord',
    'If you use GitHub': 'Se usi GitHub',
    'As a pull request': 'Come pull request',
    'Add a folder <code>marketplace/&lt;name&gt;/</code> with the script, a <code>listing.json</code> and a README, as the marketplace README describes.':
        'Aggiungi una cartella <code>marketplace/&lt;name&gt;/</code> con lo script, un <code>listing.json</code> e un README, come descrive il README del marketplace.',
    'The marketplace folder': 'La cartella del marketplace',

    // The small print
    'The small print': 'Le clausole',
    'What is sent': 'Cosa viene inviato',
    'Only the script': 'Solo lo script',
    'The file, your one-line description, the name to credit, and your Archicad and Tapir versions. Nothing from the open project.':
        'Il file, la tua descrizione di una riga, il nome da citare e le tue versioni di Archicad e Tapir. Niente del progetto aperto.',
    'Free for everyone': 'Libero per tutti',
    'Every shared script is free and open source. Unless you choose another open-source licence, yours is published under the <a class="inline" href="https://opensource.org/license/mit">MIT licence</a>: anyone may use, change and share it, as long as they keep your name and the licence with it. It comes without warranty. You stay its author, and its page credits you.':
        'Ogni script condiviso è gratuito e open source. Se non scegli un\'altra licenza open source, il tuo viene pubblicato con <a class="inline" href="https://opensource.org/license/mit">licenza MIT</a>: chiunque può usarlo, modificarlo e condividerlo, purché mantenga il tuo nome e la licenza. È fornito senza garanzia. Ne resti l\'autore, e la sua pagina ti cita.',
    'Only share a script you wrote, or one whose author agreed to share it.':
        'Condividi solo uno script che hai scritto tu, o uno il cui autore ha accettato di condividerlo.',

    // What the AI changes
    'What the AI changes': 'Cosa cambia l\'IA',
    'What the AI <em>changes.</em>': 'Cosa cambia <em>l\'IA.</em>',
    'It keeps what your script does. It changes how, so the script is safe and works in other offices, and lists every change in plain words in the pull request.':
        'Mantiene quello che fa il tuo script. Ne cambia il modo, perché lo script sia sicuro e funzioni in altri studi, ed elenca ogni modifica con parole semplici nella pull request.',
    'Made safe': 'Reso sicuro',
    'Removed, or flagged for a maintainer': 'Rimosso, o segnalato a un maintainer',
    'connections to anything other than Archicad on your own computer;':
        'connessioni a qualsiasi cosa che non sia Archicad sul tuo computer;',
    'running other programs or code built from text (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);':
        'l\'avvio di altri programmi o di codice costruito da testo (<code>subprocess</code>, <code>os.system</code>, <code>eval</code>, <code>exec</code>);',
    'installing packages while it runs (<code>pip install</code>);':
        'l\'installazione di pacchetti durante l\'esecuzione (<code>pip install</code>);',
    'deleting or overwriting files, and writing anywhere but the project\'s folder and <code>Documents/Tapir</code>;':
        'l\'eliminazione o la sovrascrittura di file, e la scrittura altrove che nella cartella del progetto e in <code>Documents/Tapir</code>;',
    'passwords, tokens and keys;': 'password, token e chiavi;',
    'user names, <code>C:\\Users\\…</code> paths, and project, client and office names, in the code and its comments.':
        'nomi utente, percorsi <code>C:\\Users\\…</code>, e nomi di progetti, clienti e studi, nel codice e nei suoi commenti.',
    'Made to fit other offices': 'Adattato ad altri studi',
    'Settings instead of your names': 'Impostazioni al posto dei tuoi nomi',
    'Property names, layers, stories, paths and classification codes you typed in become settings at the top of the script, or fields of its window.':
        'Nomi di proprietà, layer, piani, percorsi e codici di classificazione scritti a mano diventano impostazioni all\'inizio dello script, o campi della sua finestra.',
    'Built-in properties are found by their ID, so the script works in every language of Archicad.':
        'Le proprietà predefinite vengono trovate tramite il loro ID, così lo script funziona in ogni lingua di Archicad.',
    'Lengths and areas work in metric and imperial units; paths work on Windows and macOS.':
        'Lunghezze e superfici funzionano in unità metriche e imperiali; i percorsi su Windows e macOS.',
    'Made safe to run': 'Sicuro da eseguire',
    'No surprises': 'Nessuna sorpresa',
    'It says what it is about to change, and does nothing, saying so, when nothing fits.':
        'Dice cosa sta per modificare, e non fa nulla, avvisandoti, quando niente corrisponde.',
    'It makes its changes in one Undo step where it can.':
        'Quando può, fa le sue modifiche in un unico passo di Annulla.',
    'When a property or element type it needs is missing, it shows a clear message instead of crashing.':
        'Quando manca una proprietà o un tipo di elemento di cui ha bisogno, mostra un messaggio chiaro invece di bloccarsi.',
    'Described': 'Descritto',
    'A README in plain words': 'Un README con parole semplici',
    'What it does, how to use it, its settings, what it changes, its limits, and <em>Original by</em> you.':
        'Cosa fa, come usarlo, le sue impostazioni, cosa modifica, i suoi limiti, e <em>Original by</em> con il tuo nome.',
    'Its category, the Archicad and Tapir versions it works with, and whether it changes the model, writes files or only reads.':
        'La sua categoria, le versioni di Archicad e Tapir con cui funziona, e se modifica il modello, scrive file o legge soltanto.',

    // Review
    'Review': 'Revisione',
    'Before it is merged': 'Prima dell\'unione',
    'What the review checks': 'Cosa controlla la revisione',
    'Every pull request is reviewed before it is merged, like any other code in Tapir. The review checks that:':
        'Ogni pull request viene revisionata prima di essere unita, come qualsiasi altro codice di Tapir. La revisione controlla che:',
    'nothing on the <em>Made safe</em> list is left in, unless a maintainer agreed to it;':
        'non resti nulla della lista <em>Reso sicuro</em>, a meno che un maintainer non l\'abbia accettato;',
    'the script still does what you wrote it for, and you confirmed that;':
        'lo script faccia ancora ciò per cui l\'hai scritto, e che tu l\'abbia confermato;',
    'the folder has the files and fields the <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">marketplace format</a> asks for;':
        'la cartella abbia i file e i campi richiesti dal <a class="inline" href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/marketplace/README.md">formato del marketplace</a>;',
    'the README is accurate, and the screenshots show no client, project or people.':
        'il README sia corretto, e gli screenshot non mostrino clienti, progetti o persone.',
    'Next step': 'Passo successivo',
    'Post your script': 'Pubblica il tuo script',
    'Share it in <code>#share-a-script</code> on Discord, and we take it from there.':
        'Condividilo in <code>#share-a-script</code> su Discord, e al resto pensiamo noi.',
    'Browse scripts': 'Sfoglia gli script'
});
