/* Français: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Documentation de l\'automatisation d\'Archicad | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'La documentation de Tapir : chaque commande JSON, le guide Grasshopper, la couverture de l\'API Archicad et celle de Grasshopper, et le fonctionnement d\'un appel à Tapir.',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / Docs',
    'The <em>reference.</em>': 'La <em>référence.</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'Chaque commande, ce que Tapir couvre de l\'API Archicad, et comment les composants Grasshopper s\'y rattachent.',
    'The docs': 'La documentation',

    // The four docs
    'Tapir commands': 'Commandes Tapir',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        'Cherchez par nom, filtrez par la version d\'apparition d\'une commande, et ouvrez n\'importe quelle commande pour voir ses schémas d\'entrée et de sortie et un exemple Python.',
    'Open the reference': 'Ouvrir la référence',
    'Grasshopper guide': 'Guide Grasshopper',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'Les composants d\'outil, un par outil d\'Archicad, avec des entrées nommées comme dans le dialogue de paramètres. Installation, une première définition, et le rôle des boutons.',
    'Read the guide': 'Lire le guide',
    'Archicad API': 'API Archicad',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> couverts',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Chaque fonction de l\'API Archicad de Graphisoft, et si Tapir l\'utilise, y travaille ou l\'a écartée, mis à jour toutes les heures.',
    'See the coverage': 'Voir la couverture',
    'Components': 'Composants',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'Quelles commandes Tapir ont un composant Grasshopper, et quelles entrées manquent encore à chaque composant.',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Ce que l’API Archicad ne sait pas faire, comme annuler, le presse-papiers ou les lignes d’un tableau, avec ce qu’il faut faire à la place et la preuve dans le DevKit.',
    'See the limits': 'Voir les limites',

    // How it works
    'How it works': 'Comment ça marche',
    'How it <em>works.</em>': 'Comment ça <em>marche.</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir est un add-on Archicad qui transforme les fonctions natives de l\'API en un serveur JSON sans état sur le port 19723. Tout ce qui peut envoyer une requête HTTP peut piloter Archicad.',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper, les scripts Python et les outils d\'IA envoient du JSON par HTTP à l\'add-on Tapir, qui appelle l\'API C++ d\'Archicad sur la maquette BIM.',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>Les composants tapir</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Scripts Python</strong><span>Ceux de la palette, ou les vôtres</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>IA et MCP</strong><span>Assistants et serveurs MCP</span>',
    '↓ JSON over HTTP ↓': '↓ JSON par HTTP ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Add-on Tapir</strong><span>Une couche JSON sans état : une commande par requête</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>La maquette BIM et son API C++</span>',

    // Anatomy of a call
    'Anatomy of a call': 'Anatomie d\'un appel',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'Chaque commande Tapir s\'envoie de la même façon ; seuls <code>commandName</code> et <code>addOnCommandParameters</code> changent. <a href="../start/#python">Votre premier script Python</a>',

    // More guides
    'More guides': 'Autres guides',
    'Install and run a script': 'Installer et lancer un script',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Installez Tapir, lancez un script depuis la palette et écrivez votre premier script Python.',
    'Get started': 'Démarrer',
    'AI': 'IA',
    'MCP servers and scripts an AI assistant writes for you.': 'Des serveurs MCP et des scripts qu\'un assistant IA écrit pour vous.',
    'Use Tapir with AI': 'Utiliser Tapir avec l\'IA',
    'Learn by watching': 'Apprendre en regardant',
    'Six videos, from installing Tapir to whole workflows.': 'Six vidéos, de l\'installation de Tapir à des flux de travail complets.',
    'Watch the tutorials': 'Voir les tutoriels'
});
