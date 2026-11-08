/* Deutsch: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Dokumentation zur Archicad-Automatisierung | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'Die Dokumentation von Tapir: jeder JSON-Befehl, die Grasshopper-Anleitung, die Abdeckung der Archicad-API und von Grasshopper, und wie ein Aufruf an Tapir funktioniert.',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / Doku',
    'The <em>reference.</em>': 'Die <em>Referenz.</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'Jeder Befehl, was Tapir von der Archicad-API abdeckt, und wie die Grasshopper-Komponenten dazu passen.',
    'The docs': 'Die Doku',

    // The four docs
    'Tapir commands': 'Tapir-Befehle',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        'Nach Namen suchen, nach der Version filtern, in der ein Befehl kam, und jeden Befehl mit seinem Ein- und Ausgabeschema und einem Python-Beispiel öffnen.',
    'Open the reference': 'Zur Referenz',
    'Grasshopper guide': 'Grasshopper-Anleitung',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'Die Werkzeug-Komponenten, eine pro Archicad-Werkzeug, mit Eingängen, die wie im Einstellungsdialog heißen. Installation, eine erste Definition und was die Schaltflächen tun.',
    'Read the guide': 'Zur Anleitung',
    'Archicad API': 'Archicad-API',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> abgedeckt',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Jede Funktion der Archicad-API von Graphisoft und ob Tapir sie nutzt, daran arbeitet oder sie ausgelassen hat, stündlich aktualisiert.',
    'See the coverage': 'Zur Abdeckung',
    'Components': 'Komponenten',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'Welche Tapir-Befehle eine Grasshopper-Komponente haben und welche Eingänge jeder Komponente noch fehlen.',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Was die Archicad-API nicht kann, etwa Rückgängig, die Zwischenablage oder die Zeilen einer Liste, mit dem, was Sie stattdessen tun können, und dem Nachweis im DevKit.',
    'See the limits': 'Zu den Grenzen',

    // How it works
    'How it works': 'So funktioniert es',
    'How it <em>works.</em>': 'So <em>funktioniert es.</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir ist ein Archicad-Add-On, das native API-Funktionen zu einem zustandslosen JSON-Server auf Port 19723 macht. Alles, was eine HTTP-Anfrage senden kann, kann Archicad steuern.',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper, Python-Skripte und KI-Werkzeuge senden JSON über HTTP an das Tapir Add-On, das die C++-API von Archicad auf dem BIM-Modell aufruft.',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>Die tapir-Komponenten</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Python-Skripte</strong><span>Die der Palette oder eigene</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>KI und MCP</strong><span>Assistenten und MCP-Server</span>',
    '↓ JSON over HTTP ↓': '↓ JSON über HTTP ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Tapir Add-On</strong><span>Eine zustandslose JSON-Schicht: ein Befehl pro Anfrage</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>Das BIM-Modell und seine C++-API</span>',

    // Anatomy of a call
    'Anatomy of a call': 'Anatomie eines Aufrufs',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'Jeder Tapir-Befehl wird auf dieselbe Weise gesendet; nur <code>commandName</code> und <code>addOnCommandParameters</code> ändern sich. <a href="../start/#python">Ihr erstes Python-Skript</a>',

    // More guides
    'More guides': 'Weitere Anleitungen',
    'Install and run a script': 'Installieren und ein Skript ausführen',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Tapir installieren, ein Skript aus der Palette ausführen und Ihr erstes Python-Skript schreiben.',
    'Get started': 'Loslegen',
    'AI': 'KI',
    'MCP servers and scripts an AI assistant writes for you.': 'MCP-Server und Skripte, die ein KI-Assistent für Sie schreibt.',
    'Use Tapir with AI': 'Tapir mit KI nutzen',
    'Learn by watching': 'Lernen durch Zuschauen',
    'Six videos, from installing Tapir to whole workflows.': 'Sechs Videos, von der Installation von Tapir bis zu ganzen Arbeitsabläufen.',
    'Watch the tutorials': 'Die Tutorials ansehen'
});
