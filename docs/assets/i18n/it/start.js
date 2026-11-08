/* Italiano: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Automatizza Archicad con Python – inizia | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Installa Tapir, esegui uno script dalla palette di Tapir in Archicad, aggiungi altri script, scrivi un primo script Python e installa il plugin per Grasshopper.',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / Inizia',
    'Get <em>started.</em>': 'Per <em>iniziare.</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'Dal download al tuo primo script in circa dieci minuti. I passi da 1 a 3 non richiedono programmazione.',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Preferisci guardare? I <a href="../tutorials/">video tutorial di Tapir</a> (in inglese) mostrano come installare Tapir e usarlo.',

    // 1. Install
    'Install Tapir': 'Installa Tapir',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>Con l\'installer (consigliato).</strong> Trova ogni versione di Archicad sul computer e installa in ciascuna l\'add-on corrispondente, così Archicad lo carica al prossimo avvio.',
    'Download <span class="os">WINDOWS</span>': 'Scarica <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'Scarica <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows:</strong> l\'installer chiede i diritti di amministratore, perché copia l\'add-on nella cartella di Archicad sotto Program Files.',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS:</strong> decomprimi il file scaricato e apri l\'app.',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>Preferisci installare a mano?</strong> Scarica il file <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> o <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> per la tua versione di Archicad dall\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">ultima release</a>, poi in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, scegli il file e fai clic su <em>OK</em>.',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir si aggiorna da solo: ti avvisa quando esce una nuova versione e la installa per te.',

    // 2. Run a script
    'Run your first script': 'Esegui il tuo primo script',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir aggiunge ad Archicad un menu <em>Tapir</em>. Scegli <em>Tapir Palette</em>, seleziona uno script nell\'elenco della palette e fai clic su <em>Run Script</em>. Gli script integrati ci sono fin dall\'inizio: prova <em>ProjectHealthCheck</em>, che elenca gli elementi senza ID, le zone con lo stesso numero, gli elementi non classificati e altro. Legge soltanto il modello.',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir esegue gli script con <a href="https://docs.astral.sh/uv/">uv</a>, che porta il Python e i pacchetti di cui ogni script ha bisogno. Se uv non è ancora installato, Tapir propone di installarlo la prima volta che esegui uno script.',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        '<em>Manage Shortcuts</em> nella palette mette fino a sei script nel menu Tapir, come <em>Run Script Shortcut 1</em> fino a <em>6</em>.',

    // 3. More scripts
    'Add more scripts': 'Aggiungi altri script',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        'La pagina <a href="../scripts/">Script</a> raccoglie gli script condivisi dalla community, ognuno verificato prima di essere pubblicato. La palette di Tapir sta per avere una finestra per trovarli e installarli con un clic.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Nel frattempo</strong> Se la tua palette non ha ancora questa finestra, aggiungi uno script a mano, come qui sotto.',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        'Scarica gli script che vuoi in una cartella, poi indica quella cartella a Tapir nel suo file di configurazione, <code>Documents/Tapir/config.json</code> nella tua cartella utente (Tapir lo crea al primo avvio). Aggiungi la cartella al suo elenco <code>localFolders</code>, lasciando il resto del file com\'è:',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> è la cartella (su macOS per esempio <code>/Users/me/Documents/Tapir scripts</code>). In JSON, le barre rovesciate di un percorso Windows vanno raddoppiate.',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> è il nome che la palette mostra per gli script della cartella; se manca, si usa il nome della cartella.',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        'Facoltativamente, <code>includePattern</code> ed <code>excludePattern</code> sono espressioni regolari che scelgono quali file della cartella la palette elenca.',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        'La palette elenca gli script della cartella accanto a quelli integrati la prossima volta che carica i suoi script.',

    // 4. Python
    'Write a script of your own': 'Scrivi un tuo script',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'Archicad risponde ai comandi JSON su <code>http://127.0.0.1:19723</code> mentre un progetto è aperto (con più Archicad aperti, i successivi usano 19724, 19725 e così via). I comandi di Tapir si chiamano tramite il comando <code>API.ExecuteAddOnCommand</code> di Archicad, nel namespace <code>TapirCommand</code>. Basta la libreria standard di Python; non ci sono pacchetti da installare.',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'Il <a href="https://pypi.org/project/archicad/">pacchetto <code>archicad</code></a> di Graphisoft invia gli stessi comandi e trova la porta da solo, se lo preferisci. Metti il tuo script in una <a href="#marketplace">cartella locale</a> per eseguirlo dalla palette di Tapir.',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">Tutti i comandi, con schemi ed esempi</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'Usalo da Grasshopper',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'In Rhino, apri il Package Manager (il comando <code>PackageManager</code>, oppure <em>Tools &gt; Package Manager</em>), cerca <span class="mono">tapir</span>, installalo e riavvia Rhino. I suoi componenti sono nella scheda <em>Tapir</em> di Grasshopper e lavorano con l\'Archicad in cui gira Tapir. I componenti Walls, Doors, Windows e Railings leggono e modificano gli elementi di Archicad con i nomi che conosci dalle finestre di impostazione di Archicad.',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">La guida a Grasshopper</a>',

    // Next step
    'Next step': 'Prossimo passo',
    'Find a script': 'Trova uno script',
    'Scripts shared by architects, checked and adapted for any office.':
        'Script condivisi da architetti, verificati e adattati per qualsiasi studio.',
    'Browse scripts': 'Sfoglia gli script',
    'The command reference': 'La guida di riferimento dei comandi',
    'Every Tapir command with its parameters, its response and a Python example.':
        'Ogni comando di Tapir con i suoi parametri, la sua risposta e un esempio Python.',
    'Open the reference': 'Apri il riferimento',
    'Stuck?': 'Bloccato?',
    'Ask on Discord': 'Chiedi su Discord',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        'Lì rispondono architetti, sviluppatori e i maintainer. Oppure apri una issue su GitHub.',
    'Join Discord': 'Entra su Discord'
});
