/* Deutsch: the heads, headings and introductions of the reference pages (archicad-addon/, api-coverage/,
   gh-coverage/), which stay mostly English (data-i18n-partial). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    // The command reference (archicad-addon/)
    'Archicad JSON API commands reference | Tapir': 'Referenz der Archicad JSON API-Befehle | Tapir',
    'Every JSON command the Tapir add-on adds to Archicad, with its parameters, its response and a Python example, and how to call one from Python.':
        'Jeder JSON-Befehl, den das Tapir Add-On zu Archicad hinzufügt, mit seinen Parametern, seiner Antwort und einem Python-Beispiel, und wie Sie einen davon aus Python aufrufen.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Commands': '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / Befehle',
    'Tapir <em>commands.</em>': 'Tapir-<em>Befehle.</em>',
    'The JSON commands the Tapir add-on registers in Archicad, in the <code>TapirCommand</code> namespace, next to Graphisoft\'s official commands. Each command shows its parameters, its response and an example.':
        'Die JSON-Befehle, die das Tapir Add-On in Archicad registriert, im Namensraum <code>TapirCommand</code>, neben den offiziellen Befehlen von Graphisoft. Jeder Befehl zeigt seine Parameter, seine Antwort und ein Beispiel.',
    'New to Tapir? <a href="../start/">Get started</a> shows how to install it and run a first script. Looking for something no command does? <a href="../limits/">What the Archicad API can\'t do</a> lists what Graphisoft\'s API does not offer, with what to do instead.':
        'Neu bei Tapir? <a href="../start/">Loslegen</a> zeigt, wie Sie es installieren und ein erstes Skript ausführen. Sie suchen etwas, das kein Befehl kann? <a href="../limits/">Was die Archicad-API nicht kann</a> listet, was die API von Graphisoft nicht bietet, und was Sie stattdessen tun können.',
    'Calling a <em>command.</em>': 'Einen Befehl <em>aufrufen.</em>',
    'Archicad answers JSON commands on <code>http://127.0.0.1:19723</code> while a project is open (the next Archicads opened use 19724, 19725 and so on). A Tapir command is sent through Archicad\'s <code>API.ExecuteAddOnCommand</code> command.':
        'Archicad beantwortet JSON-Befehle auf <code>http://127.0.0.1:19723</code>, solange ein Projekt geöffnet ist (weitere geöffnete Archicads nutzen 19724, 19725 und so weiter). Ein Tapir-Befehl wird über den Archicad-Befehl <code>API.ExecuteAddOnCommand</code> gesendet.',
    'Python standard library': 'Python-Standardbibliothek',
    'Graphisoft\'s <a href="https://pypi.org/project/archicad/"><code>archicad</code> package</a>':
        'Das <a href="https://pypi.org/project/archicad/"><code>archicad</code>-Paket</a> von Graphisoft',
    'The examples of the commands below use <code>aclib</code>, the small helper in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a> that sends commands the same way as the standard-library script.':
        'Die Beispiele der Befehle unten nutzen <code>aclib</code>, das kleine Hilfsmodul in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/tree/main/archicad-addon/Examples">archicad-addon/Examples</a>, das Befehle genauso sendet wie das Skript mit der Standardbibliothek.',
    'All <em>commands.</em>': 'Alle <em>Befehle.</em>',

    // The two coverage pages
    'Overview': 'Überblick',
    'By group': 'Nach Gruppe',
    'Checklist': 'Checkliste',

    // The Archicad API coverage (api-coverage/)
    'Archicad API coverage checklist | Tapir': 'Checkliste zur Abdeckung der Archicad-API | Tapir',
    'Which parts of Graphisoft\'s Archicad API the Tapir add-on uses, item by item, kept up to date from every issue, pull request and push to the repository.':
        'Welche Teile der Archicad-API von Graphisoft das Tapir Add-On nutzt, Punkt für Punkt, aktuell gehalten bei jedem Issue, Pull Request und Push in das Repository.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / API coverage': '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / API-Abdeckung',
    'Archicad API <em>coverage.</em>': 'Abdeckung der <em>Archicad-API.</em>',
    'What Tapir covers of Graphisoft\'s Archicad API, item by item.': 'Was Tapir von der Archicad-API von Graphisoft abdeckt, Punkt für Punkt.',
    'Which parts of the Archicad API (<span id="archicad-range">every supported Archicad version</span>: C functions with their older names and command ids, and the C++ API) the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> uses, updated within minutes of every issue and push to main, and within the hour of every pull request. See also the <a href="../archicad-addon/">command reference</a>.':
        'Welche Teile der Archicad-API (<span id="archicad-range">jede unterstützte Archicad-Version</span>: C-Funktionen mit ihren älteren Namen und Befehls-IDs, und die C++-API) das <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> nutzt, aktualisiert wenige Minuten nach jedem Issue und Push auf main und innerhalb einer Stunde nach jedem Pull Request. Siehe auch die <a href="../archicad-addon/">Befehlsreferenz</a>.',
    'By part of the API': 'Nach Teil der API',

    // The Grasshopper coverage (gh-coverage/)
    'Grasshopper coverage of the Tapir commands | Tapir': 'Grasshopper-Abdeckung der Tapir-Befehle | Tapir',
    'Which Tapir commands for Archicad the components of the Tapir Grasshopper plugin call, and whether they cover each command\'s parameters or still miss some.':
        'Welche Tapir-Befehle für Archicad die Komponenten des Tapir-Grasshopper-Plugins aufrufen und ob sie alle Parameter jedes Befehls abdecken oder ihnen noch einige fehlen.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / Grasshopper coverage': '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / Grasshopper-Abdeckung',
    'Grasshopper <em>coverage.</em>': 'Grasshopper-<em>Abdeckung.</em>',
    'Which Tapir commands the Grasshopper components call, and the parameters they still miss.':
        'Welche Tapir-Befehle die Grasshopper-Komponenten aufrufen und welche Parameter ihnen noch fehlen.',
    'Which JSON commands of the <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-On</a> a component of the Tapir Grasshopper plugin calls, and whether its components cover the command\'s parameters, updated within minutes of every issue and push to main, and within the hour of every pull request. This is a static check of the plugin\'s code: it shows which parameters the components name, not that each one works. See also the <a href="../archicad-addon/">command reference</a> and the <a href="../api-coverage/">Archicad API coverage</a>.':
        'Welche JSON-Befehle des <a href="https://github.com/ENZYME-APD/tapir-archicad-automation">Tapir Add-Ons</a> eine Komponente des Tapir-Grasshopper-Plugins aufruft und ob seine Komponenten die Parameter des Befehls abdecken, aktualisiert wenige Minuten nach jedem Issue und Push auf main und innerhalb einer Stunde nach jedem Pull Request. Dies ist eine statische Prüfung des Plugin-Codes: Sie zeigt, welche Parameter die Komponenten nennen, nicht, dass jeder funktioniert. Siehe auch die <a href="../archicad-addon/">Befehlsreferenz</a> und die <a href="../api-coverage/">Abdeckung der Archicad-API</a>.',
    'Parameters of the used commands': 'Parameter der genutzten Befehle'
});
