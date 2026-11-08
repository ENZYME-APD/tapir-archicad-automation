/* Italiano: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Documentazione sull\'automazione di Archicad | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'La documentazione di Tapir: ogni comando JSON, la guida a Grasshopper, la copertura dell\'API di Archicad e quella di Grasshopper, e come funziona una chiamata a Tapir.',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / Docs',
    'The <em>reference.</em>': 'Il <em>riferimento.</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'Ogni comando, cosa copre Tapir dell\'API di Archicad, e come vi corrispondono i componenti di Grasshopper.',
    'The docs': 'La documentazione',

    // The four docs
    'Tapir commands': 'Comandi di Tapir',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        'Cerca per nome, filtra per la versione in cui è arrivato un comando, e apri qualsiasi comando per vederne lo schema di input e di output e un esempio Python.',
    'Open the reference': 'Apri il riferimento',
    'Grasshopper guide': 'Guida a Grasshopper',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'I componenti strumento, uno per ogni strumento di Archicad, con input chiamati come nella finestra di impostazione. Installazione, una prima definizione e cosa fanno i pulsanti.',
    'Read the guide': 'Leggi la guida',
    'Archicad API': 'API di Archicad',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> coperta',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Ogni funzione dell\'API di Archicad di Graphisoft e se Tapir la usa, ci sta lavorando o l\'ha esclusa, aggiornato ogni ora.',
    'See the coverage': 'Vedi la copertura',
    'Components': 'Componenti',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'Quali comandi di Tapir hanno un componente Grasshopper, e quali input mancano ancora a ogni componente.',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Cosa non può fare l’API di Archicad, come annulla, gli appunti o le righe di un abaco, con cosa fare invece e la prova nel DevKit.',
    'See the limits': 'Vedi i limiti',

    // How it works
    'How it works': 'Come funziona',
    'How it <em>works.</em>': 'Come <em>funziona.</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir è un add-on di Archicad che trasforma le funzioni native dell\'API in un server JSON senza stato sulla porta 19723. Qualsiasi cosa possa inviare una richiesta HTTP può pilotare Archicad.',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper, gli script Python e gli strumenti di IA inviano JSON via HTTP all\'add-on Tapir, che chiama l\'API C++ di Archicad sul modello BIM.',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>I componenti tapir</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Script Python</strong><span>Quelli della palette o i tuoi</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>IA e MCP</strong><span>Assistenti e server MCP</span>',
    '↓ JSON over HTTP ↓': '↓ JSON via HTTP ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Add-on Tapir</strong><span>Uno strato JSON senza stato: un comando per richiesta</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>Il modello BIM e la sua API C++</span>',

    // Anatomy of a call
    'Anatomy of a call': 'Anatomia di una chiamata',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'Ogni comando di Tapir si invia allo stesso modo; cambiano solo <code>commandName</code> e <code>addOnCommandParameters</code>. <a href="../start/#python">Il tuo primo script Python</a>',

    // More guides
    'More guides': 'Altre guide',
    'Install and run a script': 'Installa ed esegui uno script',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Installa Tapir, esegui uno script dalla palette e scrivi il tuo primo script Python.',
    'Get started': 'Inizia',
    'AI': 'IA',
    'MCP servers and scripts an AI assistant writes for you.': 'Server MCP e script che un assistente IA scrive per te.',
    'Use Tapir with AI': 'Usa Tapir con l\'IA',
    'Learn by watching': 'Impara guardando',
    'Six videos, from installing Tapir to whole workflows.': 'Sei video, dall\'installazione di Tapir a interi flussi di lavoro.',
    'Watch the tutorials': 'Guarda i tutorial'
});
