/* Français: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'Piloter Archicad avec des assistants IA (MCP) | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Piloter Archicad avec Claude et d\'autres assistants IA grâce à Tapir : les serveurs MCP pour Tapir, un prompt pour commencer, et comment faire écrire un script à une IA.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / IA',
    'Tapir and <em>AI.</em>': 'Tapir et l\'<em>IA.</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'Un assistant IA peut lire votre projet Archicad et le modifier grâce à Tapir. Vous demandez avec des mots simples ; il envoie les commandes à l\'Archicad ouvert sur votre ordinateur.',

    // Two ways
    'Two ways to <em>connect.</em>': 'Deux façons de se <em>connecter.</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'Les deux utilisent les mêmes commandes Tapir sur le port 19723, sur votre propre ordinateur.',
    'While you chat': 'Pendant la conversation',
    'An MCP server': 'Un serveur MCP',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'L\'outil d\'IA appelle lui-même les commandes de Tapir pendant que vous discutez. Fonctionne avec les outils qui prennent en charge le Model Context Protocol (MCP), comme Claude Desktop.',
    'Tapir MCP servers': 'Serveurs MCP pour Tapir',
    'Any AI tool': 'Tout outil d\'IA',
    'A script the AI writes': 'Un script écrit par l\'IA',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'Tout outil d\'IA peut écrire un court script Python qui envoie des commandes Tapir. Vous le lisez, puis vous le lancez depuis la palette Tapir ou un terminal.',
    'How it works': 'Comment ça marche',

    // MCP servers
    'MCP servers': 'Serveurs MCP',
    'Tapir MCP <em>servers.</em>': 'Serveurs MCP <em>pour Tapir.</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'Ce sont des projets de la communauté, qui ne font pas partie de Tapir. Chacun décrit son installation dans son README. Celui que Tapir recommande est encore en discussion dans l\'<a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a> ; ils sont donc listés ici sans recommandation.',
    'By SzamosiMate': 'Par SzamosiMate',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'Le serveur MCP pour Tapir dont les autres sont partis. Il est aussi publié sur PyPI sous le nom de paquet <span class="mono">tapir-archicad-mcp</span>.',
    'Its repository': 'Son dépôt',
    'The package on PyPI': 'Le paquet sur PyPI',
    'By the community': 'Par la communauté',
    'Its forks': 'Ses forks',
    'Versions of it that others changed and published. Check when each was last updated.':
        'Des versions que d\'autres ont modifiées et publiées. Vérifiez la date de dernière mise à jour de chacune.',
    'See the forks': 'Voir les forks',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Attention</strong> Un serveur MCP peut modifier votre maquette. Essayez-le d\'abord sur une copie de votre projet, et lisez ce que l\'IA s\'apprête à faire avant de l\'autoriser.',

    // A prompt to start with
    'A prompt to start with': 'Un prompt pour commencer',
    'Start by <em>reading.</em>': 'Commencez par <em>lire.</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Ouvrez un projet dans Archicad, avec l\'Add-On Tapir chargé, et donnez ce prompt à votre outil d\'IA. Il ne fait que lire : vous pouvez l\'essayer sans risque sur n\'importe quel projet.',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        'Quand la réponse semble juste, demandez une modification avec les mêmes mots simples, en précisant quels éléments elle peut modifier : « Mets la propriété X à Y uniquement sur les murs sélectionnés. »',
    'prompt': 'prompt',
    'read only': 'lecture seule',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'Utilise Tapir pour examiner le projet Archicad que j\'ai ouvert.\nNe modifie rien ; lis seulement.\n1. Liste les étages avec leurs noms et leurs niveaux.\n2. Compte les murs, dalles, poteaux, portes et fenêtres de chaque étage.\n3. Présente le résultat dans un seul tableau, une ligne par étage.\nDis-moi quelles commandes Tapir tu as utilisées.',

    // When the AI writes a script
    'When the AI writes a script': 'Quand l\'IA écrit un script',
    'Describe it, <em>run it.</em>': 'Décrivez-le, <em>lancez-le.</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'Sans serveur MCP, décrivez la tâche, donnez à l\'IA la liste des commandes et lancez le script qu\'elle écrit.',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>Décrivez ce que vous voulez</strong><span>Par exemple : « Extrais tous les murs extérieurs composites du niveau 2 et calcule leur volume net. »</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>Partagez la liste des commandes</strong><span>Donnez à l\'IA la <a href="../archicad-addon/">référence des commandes</a> et l\'exemple fonctionnel ci-dessous. Demandez uniquement la bibliothèque standard de Python, sur le port 19723.</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>Lisez-le, puis lancez-le</strong><span>Lisez le script avant de le lancer, depuis la palette Tapir ou un terminal. Essayez d\'abord sur une copie de votre projet un script qui modifie la maquette.</span>',
    'for Claude or any AI tool': 'pour Claude ou tout outil d\'IA',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        'Écris un script Python qui n\'utilise que la\nbibliothèque standard et envoie des commandes Tapir\nà Archicad sur http://127.0.0.1:19723, comme\nl\'exemple joint.\nUtilise la référence des commandes Tapir jointe.\n\nExtrais tous les murs extérieurs composites du\nniveau 2 et calcule leur volume net.\n\nLis seulement la maquette ; ne modifie rien.',

    // Next step
    'Next step': 'Étape suivante',
    'Install Tapir': 'Installer Tapir',
    'Install Tapir, then give your AI tool the prompt above.': 'Installez Tapir, puis donnez le prompt ci-dessus à votre outil d\'IA.',
    'Get started': 'Démarrer',
    'The command reference': 'La référence des commandes',
    'The list to give the AI: every command with its schema.': 'La liste à donner à l\'IA : chaque commande avec son schéma.',
    'Open the reference': 'Ouvrir la référence',
    'Show what you built': 'Montrez ce que vous avez fait',
    'Ask about MCP servers and share your prompts.': 'Posez vos questions sur les serveurs MCP et partagez vos prompts.',
    'Join Discord': 'Rejoindre Discord'
});
