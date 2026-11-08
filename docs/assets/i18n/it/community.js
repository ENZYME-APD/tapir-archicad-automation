/* Italiano: the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': 'Community – automazione di Archicad con Tapir',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        'Chiedi su Discord, segnala un bug, prova una correzione nel tuo Archicad: come la community di Tapir e i suoi bot trasformano una segnalazione in una release, e come è cresciuto il progetto.',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / Community',
    'Ask. Report. <em>Test.</em>': 'Chiedi. Segnala. <em>Prova.</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        'Architetti, sviluppatori e maintainer in un unico posto. Bug e idee diventano correzioni che qualcuno con la tua versione di Archicad prova prima che vengano pubblicate.',

    // Where to find us
    'Where to find us': 'Dove trovarci',
    'Ask anything': 'Chiedi qualsiasi cosa',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        'Architetti, sviluppatori e maintainer, in un unico server. Mostra quello che hai fatto e parla con le persone che sviluppano Tapir.',
    'Join Discord': 'Entra su Discord',
    'Learn the API': 'Impara l\'API',
    'A video series from the first command to full workflows.': 'Una serie di video, dal primo comando a workflow completi.',
    'Watch the tutorials': 'Guarda i tutorial',
    'AI assistants': 'Assistenti IA',
    'Talk to your model': 'Parla con il tuo modello',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'I server MCP permettono a Claude e ad altri assistenti di leggere e modificare un progetto tramite Tapir.',

    // How the bots work
    'From report to release': 'Dalla segnalazione alla release',
    'How the bots <em>work.</em>': 'Come lavorano <em>i bot.</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        'La maggior parte delle correzioni è scritta dai bot e controllata dalle persone. I maintainer stabiliscono le regole e possono fermare qualsiasi modifica; sei tu a decidere se una correzione funziona nel tuo Archicad.',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>Pubblicalo</strong><span>Descrivi il bug o l\'idea su Discord o su GitHub. Su Discord non serve un account GitHub.</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>Una issue</strong><span>Ogni mezz\'ora, i messaggi su Discord che sembrano una segnalazione di bug o una richiesta di funzionalità diventano issue su GitHub, e il bot risponde con il link.</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>Una bozza di correzione</strong><span>Il bot delle issue risponde, chiede quello che gli manca, oppure scrive la correzione e apre una pull request. Viene compilata per ogni versione di Archicad e revisionata.</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>Tu provi</strong><span>Un commento sulla pull request mette il link all\'add-on compilato e chiede a chi ha fatto la segnalazione di provarlo.</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>Pubblicata</strong><span>Verde e revisionata, viene unita tre giorni dopo, a meno che un maintainer non la fermi, ed esce nella <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a> del lunedì.</span>',

    // Report and test
    'Report and test': 'Segnala e prova',
    'Report': 'Segnala',
    'How to report a bug': 'Come segnalare un bug',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        'Pubblicalo su Discord o apri una issue su GitHub; arrivano entrambi nello stesso posto. Una segnalazione che si può correggere in un colpo solo indica:',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        'la tua versione e build di Archicad, e la sua lingua (per esempio 29.0.0 (3100) INT);',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        'la tua versione di Tapir (la mostra la finestra <em>About Tapir</em>);',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        'cosa hai fatto: lo script o la definizione Grasshopper, oppure il comando e il JSON che hai inviato;',
    'what you expected and what happened instead, with the error message;':
        'cosa ti aspettavi e cosa è successo invece, con il messaggio di errore;',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        'se il progetto è un progetto Teamwork e quali librerie sono caricate, quando conta.',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Pubblico</strong> Le issue e i canali Discord sono pubblici. Non inserire nomi di clienti, nomi di progetti e percorsi di file che non vuoi condividere.',
    'Open an issue on GitHub': 'Apri una issue su GitHub',
    'Post on Discord': 'Scrivi su Discord',
    'Test': 'Prova',
    'How to test a fix': 'Come provare una correzione',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        'Si sa che una correzione funziona solo quando qualcuno l\'ha eseguita in Archicad. I bot possono compilare Tapir ma non eseguire Archicad, quindi la tua prova è ciò che conta di più.',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        'Apri la pull request collegata alla tua issue. Un commento elenca i download, uno per versione di Archicad e piattaforma. Per scaricare serve l\'accesso a GitHub.',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        'Decomprimi il download per la tua versione di Archicad. Su macOS contiene un altro zip con il <code>.bundle</code>.',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        'In Archicad, apri <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, rimuovi o disattiva il Tapir installato, poi aggiungi con <em>Add</em> il file <code>.apx</code> (Windows) o il <code>.bundle</code> (macOS) e fai clic su OK.',
    'Do what went wrong before, in a copy of your project.':
        'Rifai quello che prima non funzionava, in una copia del tuo progetto.',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        'Scrivi sulla pull request se funziona e risolve il tuo caso. Dopo, rimetti allo stesso modo il Tapir che avevi installato.',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        'Provi spesso? Il <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> prova le pull request aperte sul tuo Archicad mentre è chiuso, lascia al suo posto il tuo Tapir e pubblica il risultato. Funziona su Windows e richiede Python e un account GitHub.',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{sign}{diff} in {days} giorni',
    'Momentum': 'Slancio',
    'Commits': 'Commit',
    'Discord messages': 'Messaggi su Discord',
    'Discord members <span class="change"></span>': 'Membri di Discord <span class="change"></span>',
    'People taking part, last 12 months': 'Persone che partecipano, ultimi 12 mesi',
    'Top contributors': 'Principali contributori',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        'Si contano i commit (coautori inclusi), le issue segnalate che una correzione unita ha chiuso e le pull request provate in Archicad (riuscite o no).',
    '{count} commit': '{count} commit',
    '{count} commits': '{count} commit',
    '{count} report': '{count} segnalazione',
    '{count} reports': '{count} segnalazioni',
    '{count} test': '{count} test',
    '{count} tests': '{count} test',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        'Commit, issue e pull request, e messaggi su Discord al mese, impilati, con il numero di membri di Discord come linea, da {first} a {last}. La tabella sotto riporta tutti i numeri.',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': 'gen feb mar apr mag giu lug ago set ott nov dic',
    'Growth': 'Crescita',
    'Growth since <em>2022.</em>': 'La crescita dal <em>2022.</em>',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        'Tutto quello che succede intorno a Tapir ogni mese, dal primo commit in Enzyme a oggi. Si raccolgono solo numeri, mai il testo di un messaggio.',
    '<i class="swatch layer-commits"></i>Commits': '<i class="swatch layer-commits"></i>Commit',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>Issue e pull request',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Messaggi su Discord',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Membri di Discord, scala a destra',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        'Al mese, impilati, su una scala a radice quadrata perché i primi anni restino visibili. Commit di persone, senza merge e bot; i messaggi su Discord comprendono thread e post del forum. I membri di Discord sono le persone presenti nel server alla fine di ogni mese, contate in base a quando sono entrate; chi è uscito in seguito non è incluso.',
    'Show the numbers as a table': 'Mostra i numeri in una tabella',
    'Month': 'Mese',
    'Issues opened': 'Issue aperte',
    'Pull requests merged': 'Pull request unite',
    'Discord members': 'Membri di Discord',

    // Help build it
    'Help build it': 'Aiuta a costruirlo',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        'Hai fatto uno script che usa il tuo studio? Condividilo, e qualsiasi studio potrà installarlo dalla palette.',
    'How to share': 'Come condividere',
    'Ask for a feature': 'Chiedi una funzionalità',
    'How features are requested, and what is being asked for now.': 'Come si richiedono le funzionalità e cosa viene chiesto adesso.',
    'See the roadmap': 'Guarda la roadmap',
    'Code': 'Codice',
    'Write a command': 'Scrivi un comando',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        'L\'add-on è scritto in C++ sull\'API di Archicad. La guida allo sviluppo mostra come aggiungere un comando.',
    'Read the guide': 'Leggi la guida',
    'Get involved': 'Partecipa',
    'We\'re building a motivated, diverse community around Archicad automation.':
        'Stiamo costruendo una community motivata e varia intorno all\'automazione di Archicad.',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        'Passa a salutare su Discord e raccontaci cosa automatizzi in Archicad.',
    'Join the Discord': 'Entra nel Discord'
});
