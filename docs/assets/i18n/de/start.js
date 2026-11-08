/* Deutsch: the Get started page (start/index.html). See ../README.md.
   The menu, palette and button names of Archicad and Tapir stay English, as the add-on shows them. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Automate Archicad with Python – get started | Tapir': 'Archicad mit Python automatisieren – erste Schritte | Tapir',
    'Install Tapir, run a script from the Tapir palette in Archicad, add more scripts, write a first Python script and install the Grasshopper plugin.':
        'Tapir installieren, ein Skript aus der Tapir-Palette in Archicad ausführen, weitere Skripte hinzufügen, ein erstes Python-Skript schreiben und das Grasshopper-Plugin installieren.',
    '<a href="../">Tapir</a> / Get started': '<a href="../">Tapir</a> / Loslegen',
    'Get <em>started.</em>': 'Jetzt <em>loslegen.</em>',
    'From download to your first script in about ten minutes. Steps 1 to 3 need no programming.':
        'Vom Download bis zum ersten Skript in etwa zehn Minuten. Die Schritte 1 bis 3 brauchen keine Programmierung.',
    '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Prefer watching? The <a href="../tutorials/">Tapir video tutorials</a> walk through installing Tapir and using it.':
        '<strong class="callout-label"><span aria-hidden="true">▶</span> Video</strong> Lieber zuschauen? Die <a href="../tutorials/">Video-Tutorials zu Tapir</a> zeigen, wie man Tapir installiert und benutzt (auf Englisch).',

    // 1. Install
    'Install Tapir': 'Tapir installieren',
    '<strong>With the installer (recommended).</strong> It finds every Archicad version on the computer and installs the matching add-on into each, so Archicad loads it on its next start.':
        '<strong>Mit dem Installer (empfohlen).</strong> Er findet jede Archicad-Version auf dem Computer und installiert in jede das passende Add-On, sodass Archicad es beim nächsten Start lädt.',
    'Download <span class="os">WINDOWS</span>': 'Herunterladen <span class="os">WINDOWS</span>',
    'Download <span class="os">MACOS</span>': 'Herunterladen <span class="os">MACOS</span>',
    '<strong>Windows:</strong> the installer asks for administrator rights, because it copies the add-on into the Archicad folder under Program Files.':
        '<strong>Windows:</strong> Der Installer fragt nach Administratorrechten, weil er das Add-On in den Archicad-Ordner unter Program Files kopiert.',
    '<strong>macOS:</strong> unzip the download and open the app.':
        '<strong>macOS:</strong> Den Download entpacken und die App öffnen.',
    '<strong>Prefer to install by hand?</strong> Download the <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> or <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> file for your Archicad version from the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">latest release</a>, then in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, browse to the file and click <em>OK</em>.':
        '<strong>Lieber von Hand installieren?</strong> Laden Sie die Datei <code>TapirAddOn_AC&lt;version&gt;_Win.apx</code> oder <code>TapirAddOn_AC&lt;version&gt;_Mac.zip</code> für Ihre Archicad-Version aus dem <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases/latest">neuesten Release</a> herunter, dann in Archicad: <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons &gt; Add</em>, die Datei auswählen und auf <em>OK</em> klicken.',
    'Tapir keeps itself up to date: it tells you when a new version is out and installs it for you.':
        'Tapir hält sich selbst aktuell: Es meldet, wenn eine neue Version erschienen ist, und installiert sie für Sie.',

    // 2. Run a script
    'Run your first script': 'Das erste Skript ausführen',
    'Tapir adds a <em>Tapir</em> menu to Archicad. Choose <em>Tapir Palette</em> from it, pick a script in the palette\'s list and click <em>Run Script</em>. The built-in scripts are there from the start: try <em>ProjectHealthCheck</em>, which lists elements without an ID, zones with the same number, unclassified elements and more. It only reads the model.':
        'Tapir fügt Archicad ein Menü <em>Tapir</em> hinzu. Wählen Sie dort <em>Tapir Palette</em>, wählen Sie ein Skript in der Liste der Palette und klicken Sie auf <em>Run Script</em>. Die mitgelieferten Skripte sind von Anfang an da: Probieren Sie <em>ProjectHealthCheck</em>, das Elemente ohne ID, Zonen mit derselben Nummer, nicht klassifizierte Elemente und mehr auflistet. Es liest das Modell nur.',
    'Tapir runs the scripts with <a href="https://docs.astral.sh/uv/">uv</a>, which brings the Python and the packages each script needs. If uv is not installed yet, Tapir offers to install it the first time you run a script.':
        'Tapir führt die Skripte mit <a href="https://docs.astral.sh/uv/">uv</a> aus, das Python und die Pakete mitbringt, die ein Skript braucht. Ist uv noch nicht installiert, bietet Tapir beim ersten Ausführen eines Skripts an, es zu installieren.',
    '<em>Manage Shortcuts</em> on the palette puts up to six scripts in the Tapir menu, as <em>Run Script Shortcut 1</em> to <em>6</em>.':
        '<em>Manage Shortcuts</em> in der Palette legt bis zu sechs Skripte ins Tapir-Menü, als <em>Run Script Shortcut 1</em> bis <em>6</em>.',

    // 3. More scripts
    'Add more scripts': 'Weitere Skripte hinzufügen',
    'The <a href="../scripts/">Scripts</a> page collects scripts shared by the community, each one reviewed before it is listed. The Tapir palette is getting a window to find and install them in one click.':
        'Die Seite <a href="../scripts/">Skripte</a> sammelt Skripte, die die Community teilt, jedes vor der Aufnahme geprüft. Die Tapir-Palette bekommt ein Fenster, um sie mit einem Klick zu finden und zu installieren.',
    '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Until then</strong> If your palette has no such window yet, add a script by hand, as below.':
        '<strong class="callout-label"><span aria-hidden="true">ⓘ</span> Bis dahin</strong> Hat Ihre Palette dieses Fenster noch nicht, fügen Sie ein Skript wie unten beschrieben von Hand hinzu.',
    'Download the scripts you want into a folder, then tell Tapir about that folder in its configuration file, <code>Documents/Tapir/config.json</code> in your user folder (Tapir writes it the first time it runs). Add the folder to its <code>localFolders</code> list, keeping the rest of the file as it is:':
        'Laden Sie die gewünschten Skripte in einen Ordner und nennen Sie Tapir diesen Ordner in seiner Konfigurationsdatei, <code>Documents/Tapir/config.json</code> in Ihrem Benutzerordner (Tapir legt sie beim ersten Start an). Fügen Sie den Ordner der Liste <code>localFolders</code> hinzu und lassen Sie den Rest der Datei, wie er ist:',
    '<code>path</code> is the folder (on macOS for example <code>/Users/me/Documents/Tapir scripts</code>). In JSON, a Windows path\'s backslashes are doubled.':
        '<code>path</code> ist der Ordner (unter macOS zum Beispiel <code>/Users/me/Documents/Tapir scripts</code>). In JSON werden die Backslashes eines Windows-Pfads verdoppelt.',
    '<code>displayName</code> is the name the palette shows for the folder\'s scripts; without it, the folder\'s name is used.':
        '<code>displayName</code> ist der Name, den die Palette für die Skripte des Ordners zeigt; ohne ihn wird der Name des Ordners verwendet.',
    'Optionally, <code>includePattern</code> and <code>excludePattern</code> are regular expressions that choose which files of the folder the palette lists.':
        'Optional wählen <code>includePattern</code> und <code>excludePattern</code>, reguläre Ausdrücke, welche Dateien des Ordners die Palette auflistet.',
    'The palette lists the folder\'s scripts next to the built-in ones the next time it loads its scripts.':
        'Die Palette listet die Skripte des Ordners neben den mitgelieferten, sobald sie ihre Skripte das nächste Mal lädt.',

    // 4. Python
    'Write a script of your own': 'Ein eigenes Skript schreiben',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (with several Archicads open, the next ones use 19724, 19725 and so on). Tapir\'s commands are called through Archicad\'s <code>API.ExecuteAddOnCommand</code> command, in the <code>TapirCommand</code> namespace. Python\'s standard library is enough; there are no packages to install.':
        'Archicad beantwortet JSON-Befehle unter <code>http://127.0.0.1:19723</code>, solange ein Projekt geöffnet ist (sind mehrere Archicads offen, nutzen die weiteren 19724, 19725 und so fort). Die Befehle von Tapir werden über den Archicad-Befehl <code>API.ExecuteAddOnCommand</code> aufgerufen, im Namespace <code>TapirCommand</code>. Die Standardbibliothek von Python genügt; es sind keine Pakete zu installieren.',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a> sends the same commands and finds the port by itself, if you prefer it. Put your script in a <a href="#marketplace">local folder</a> to run it from the Tapir palette.':
        'Das <a href="https://pypi.org/project/archicad/">Paket <code>archicad</code></a> von Graphisoft sendet dieselben Befehle und findet den Port selbst, falls Sie es bevorzugen. Legen Sie Ihr Skript in einen <a href="#marketplace">lokalen Ordner</a>, um es aus der Tapir-Palette auszuführen.',
    '<a href="../archicad-addon/">All commands, with schemas and examples</a>':
        '<a href="../archicad-addon/">Alle Befehle, mit Schemas und Beispielen</a>',

    // 5. Grasshopper
    'Use it from Grasshopper': 'Aus Grasshopper verwenden',
    'In Rhino, open the Package Manager (the <code>PackageManager</code> command, or <em>Tools &gt; Package Manager</em>), search for <span class="mono">tapir</span>, install it and restart Rhino. Its components are on the <em>Tapir</em> tab in Grasshopper and work with the Archicad that runs Tapir. The Walls, Doors, Windows and Railings components read and change Archicad elements with the names you know from Archicad\'s settings dialogs.':
        'Öffnen Sie in Rhino den Package Manager (den Befehl <code>PackageManager</code> oder <em>Tools &gt; Package Manager</em>), suchen Sie nach <span class="mono">tapir</span>, installieren Sie es und starten Sie Rhino neu. Die Komponenten liegen in Grasshopper auf dem Reiter <em>Tapir</em> und arbeiten mit dem Archicad, in dem Tapir läuft. Die Komponenten Walls, Doors, Windows und Railings lesen und ändern Archicad-Elemente mit den Namen, die Sie aus den Einstellungsdialogen von Archicad kennen.',
    '<a href="../grasshopper/">The Grasshopper guide</a>': '<a href="../grasshopper/">Die Grasshopper-Anleitung</a>',

    // Next step
    'Next step': 'Nächster Schritt',
    'Find a script': 'Ein Skript finden',
    'Scripts shared by architects, checked and adapted for any office.':
        'Von Architekten geteilte Skripte, geprüft und für jedes Büro angepasst.',
    'Browse scripts': 'Skripte durchsehen',
    'The command reference': 'Die Befehlsreferenz',
    'Every Tapir command with its parameters, its response and a Python example.':
        'Jeder Tapir-Befehl mit seinen Parametern, seiner Antwort und einem Python-Beispiel.',
    'Open the reference': 'Zur Referenz',
    'Stuck?': 'Nicht weiter?',
    'Ask on Discord': 'Auf Discord fragen',
    'Architects, developers and the maintainers answer there. Or open an issue on GitHub.':
        'Dort antworten Architekten, Entwickler und die Maintainer. Oder eröffnen Sie ein Issue auf GitHub.',
    'Join Discord': 'Discord beitreten'
});
