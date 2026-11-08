/* Italiano: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'Controlla Archicad con gli assistenti IA (MCP) | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Controlla Archicad con Claude e altri assistenti IA tramite Tapir: i server MCP per Tapir, un prompt per iniziare e come far scrivere uno script a un\'IA.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">Documentazione</a> / IA',
    'Tapir and <em>AI.</em>': 'Tapir e <em>IA.</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'Un assistente IA può leggere il tuo progetto Archicad e modificarlo tramite Tapir. Tu chiedi con parole semplici; lui invia i comandi all\'Archicad aperto sul tuo computer.',

    // Two ways
    'Two ways to <em>connect.</em>': 'Due modi per <em>collegarsi.</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'Entrambi usano gli stessi comandi Tapir sulla porta 19723, sul tuo computer.',
    'While you chat': 'Mentre chatti',
    'An MCP server': 'Un server MCP',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'Lo strumento di IA chiama da sé i comandi di Tapir mentre chatti. Funziona con gli strumenti che supportano il Model Context Protocol (MCP), come Claude Desktop.',
    'Tapir MCP servers': 'Server MCP per Tapir',
    'Any AI tool': 'Qualsiasi strumento di IA',
    'A script the AI writes': 'Uno script scritto dall\'IA',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'Qualsiasi strumento di IA può scrivere un breve script Python che invia comandi Tapir. Tu lo leggi, poi lo esegui dalla palette di Tapir o da un terminale.',
    'How it works': 'Come funziona',

    // MCP servers
    'MCP servers': 'Server MCP',
    'Tapir MCP <em>servers.</em>': 'Server MCP <em>per Tapir.</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'Sono progetti della community, non fanno parte di Tapir. Ognuno descrive la propria installazione nel suo README. Quale consigli Tapir si sta ancora decidendo nella <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, quindi qui sono elencati senza una raccomandazione.',
    'By SzamosiMate': 'Di SzamosiMate',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'Il server MCP per Tapir da cui sono partiti gli altri. È pubblicato anche su PyPI come pacchetto <span class="mono">tapir-archicad-mcp</span>.',
    'Its repository': 'Il suo repository',
    'The package on PyPI': 'Il pacchetto su PyPI',
    'By the community': 'Dalla community',
    'Its forks': 'I suoi fork',
    'Versions of it that others changed and published. Check when each was last updated.':
        'Versioni modificate e pubblicate da altri. Controlla quando ciascuna è stata aggiornata l\'ultima volta.',
    'See the forks': 'Vedi i fork',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Attenzione</strong> Un server MCP può modificare il tuo modello. Provalo prima su una copia del progetto e leggi cosa sta per fare l\'IA prima di consentirlo.',

    // A prompt to start with
    'A prompt to start with': 'Un prompt per iniziare',
    'Start by <em>reading.</em>': 'Inizia <em>leggendo.</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Apri un progetto in Archicad, con l\'Add-On Tapir caricato, e dai questo prompt al tuo strumento di IA. Legge soltanto, quindi puoi provarlo senza rischi su qualsiasi progetto.',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        'Quando la risposta sembra corretta, chiedi una modifica con le stesse parole semplici e indica esattamente quali elementi può modificare: «Imposta la proprietà X a Y solo sui muri selezionati.»',
    'prompt': 'prompt',
    'read only': 'sola lettura',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'Usa Tapir per esaminare il progetto Archicad che ho aperto.\nNon modificare nulla; limitati a leggere.\n1. Elenca i piani con i loro nomi e le loro quote.\n2. Conta muri, solai, pilastri, porte e finestre su ogni piano.\n3. Mostra il risultato in un\'unica tabella, una riga per piano.\nDimmi quali comandi Tapir hai usato.',

    // When the AI writes a script
    'When the AI writes a script': 'Quando l\'IA scrive uno script',
    'Describe it, <em>run it.</em>': 'Descrivilo, <em>eseguilo.</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'Senza un server MCP, descrivi il compito, dai all\'IA l\'elenco dei comandi ed esegui lo script che scrive.',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>Descrivi cosa vuoi</strong><span>Per esempio: «Estrai tutti i muri esterni compositi del piano 2 e calcola il loro volume netto.»</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>Condividi l\'elenco dei comandi</strong><span>Dai all\'IA il <a href="../archicad-addon/">riferimento dei comandi</a> e l\'esempio funzionante qui sotto. Chiedi di usare solo la libreria standard di Python, sulla porta 19723.</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>Leggilo, poi eseguilo</strong><span>Leggi lo script prima di eseguirlo dalla palette di Tapir o da un terminale. Prova prima su una copia del progetto uno script che modifica il modello.</span>',
    'for Claude or any AI tool': 'per Claude o qualsiasi strumento di IA',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        'Scrivi uno script Python che usi solo la libreria\nstandard e invii comandi Tapir ad Archicad su\nhttp://127.0.0.1:19723, come l\'esempio che allego.\nUsa il riferimento dei comandi Tapir che allego.\n\nEstrai tutti i muri esterni compositi del piano 2\ne calcola il loro volume netto.\n\nLimitati a leggere il modello; non modificare nulla.',

    // Next step
    'Next step': 'Passo successivo',
    'Install Tapir': 'Installa Tapir',
    'Install Tapir, then give your AI tool the prompt above.': 'Installa Tapir, poi dai al tuo strumento di IA il prompt qui sopra.',
    'Get started': 'Inizia',
    'The command reference': 'Il riferimento dei comandi',
    'The list to give the AI: every command with its schema.': 'L\'elenco da dare all\'IA: ogni comando con il suo schema.',
    'Open the reference': 'Apri il riferimento',
    'Show what you built': 'Mostra cosa hai creato',
    'Ask about MCP servers and share your prompts.': 'Fai domande sui server MCP e condividi i tuoi prompt.',
    'Join Discord': 'Unisciti a Discord'
});
