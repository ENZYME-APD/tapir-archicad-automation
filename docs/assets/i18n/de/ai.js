/* Deutsch: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'Archicad mit KI-Assistenten steuern (MCP) | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Archicad über Tapir mit Claude und anderen KI-Assistenten steuern: die Tapir-MCP-Server, ein Prompt für den Anfang und wie Sie eine KI ein Skript schreiben lassen.',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI': '<a href="../">Tapir</a> / <a href="../docs/">Doku</a> / KI',
    'Tapir and <em>AI.</em>': 'Tapir und <em>KI.</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'Ein KI-Assistent kann Ihr Archicad-Projekt über Tapir lesen und ändern. Sie fragen in einfachen Worten; er sendet die Befehle an das Archicad auf Ihrem Computer.',

    // Two ways
    'Two ways to <em>connect.</em>': 'Zwei Wege zur <em>Verbindung.</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'Beide nutzen dieselben Tapir-Befehle auf Port 19723, auf Ihrem eigenen Computer.',
    'While you chat': 'Während Sie chatten',
    'An MCP server': 'Ein MCP-Server',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'Das KI-Werkzeug ruft die Befehle von Tapir selbst auf, während Sie chatten. Funktioniert mit Werkzeugen, die das Model Context Protocol (MCP) unterstützen, etwa Claude Desktop.',
    'Tapir MCP servers': 'Tapir-MCP-Server',
    'Any AI tool': 'Jedes KI-Werkzeug',
    'A script the AI writes': 'Ein Skript, das die KI schreibt',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'Jedes KI-Werkzeug kann ein kurzes Python-Skript schreiben, das Tapir-Befehle sendet. Sie lesen es und führen es dann aus der Tapir-Palette oder einem Terminal aus.',
    'How it works': 'So funktioniert es',

    // MCP servers
    'MCP servers': 'MCP-Server',
    'Tapir MCP <em>servers.</em>': 'Tapir-MCP-<em>Server.</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'Das sind Community-Projekte, nicht Teil von Tapir. Jedes beschreibt seine Installation in seiner README. Welches Tapir empfiehlt, wird noch in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">Issue #2070</a> entschieden, daher stehen sie hier ohne Empfehlung.',
    'By SzamosiMate': 'Von SzamosiMate',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'Der MCP-Server für Tapir, von dem die anderen ausgingen. Er ist auch auf PyPI als Paket <span class="mono">tapir-archicad-mcp</span> veröffentlicht.',
    'Its repository': 'Sein Repository',
    'The package on PyPI': 'Das Paket auf PyPI',
    'By the community': 'Von der Community',
    'Its forks': 'Seine Forks',
    'Versions of it that others changed and published. Check when each was last updated.':
        'Versionen davon, die andere geändert und veröffentlicht haben. Prüfen Sie, wann jede zuletzt aktualisiert wurde.',
    'See the forks': 'Zu den Forks',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Vorsicht</strong> Ein MCP-Server kann Ihr Modell ändern. Probieren Sie ihn zuerst an einer Kopie Ihres Projekts aus, und lesen Sie, was die KI vorhat, bevor Sie es erlauben.',

    // A prompt to start with
    'A prompt to start with': 'Ein Prompt für den Anfang',
    'Start by <em>reading.</em>': 'Erst <em>lesen.</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Öffnen Sie ein Projekt in Archicad mit geladenem Tapir-Add-On und geben Sie Ihrem KI-Werkzeug diesen Prompt. Er liest nur, Sie können ihn also gefahrlos an jedem Projekt ausprobieren.',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        'Wenn die Antwort stimmt, bitten Sie in denselben einfachen Worten um eine Änderung und sagen Sie genau, welche Elemente sie ändern darf: „Setze die Eigenschaft X nur bei den ausgewählten Wänden auf Y.“',
    'prompt': 'Prompt',
    'read only': 'nur lesen',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'Sieh dir mit Tapir das Archicad-Projekt an, das ich geöffnet habe.\nÄndere nichts; lies nur.\n1. Liste die Geschosse mit ihren Namen und Höhen auf.\n2. Zähle die Wände, Decken, Stützen, Türen und Fenster in jedem Geschoss.\n3. Zeige das Ergebnis als eine Tabelle, eine Zeile je Geschoss.\nSag mir, welche Tapir-Befehle du benutzt hast.',

    // When the AI writes a script
    'When the AI writes a script': 'Wenn die KI ein Skript schreibt',
    'Describe it, <em>run it.</em>': 'Beschreiben, <em>ausführen.</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'Ohne MCP-Server beschreiben Sie die Aufgabe, geben der KI die Befehlsliste und führen das Skript aus, das sie schreibt.',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>Beschreiben, was Sie wollen</strong><span>Zum Beispiel: „Finde alle mehrschichtigen Außenwände im 2. Geschoss und berechne ihr Nettovolumen.“</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>Die Befehlsliste teilen</strong><span>Geben Sie der KI die <a href="../archicad-addon/">Befehlsreferenz</a> und das funktionierende Beispiel unten. Verlangen Sie nur die Standardbibliothek von Python, auf Port 19723.</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>Lesen, dann ausführen</strong><span>Lesen Sie das Skript, bevor Sie es aus der Tapir-Palette oder einem Terminal ausführen. Probieren Sie eines, das das Modell ändert, zuerst an einer Kopie Ihres Projekts aus.</span>',
    'for Claude or any AI tool': 'für Claude oder jedes KI-Werkzeug',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        'Schreibe ein Python-Skript, das nur die\nStandardbibliothek nutzt und Tapir-Befehle an Archicad\nauf http://127.0.0.1:19723 sendet, wie das Beispiel im Anhang.\nNutze die Tapir-Befehlsreferenz im Anhang.\n\nFinde alle mehrschichtigen Außenwände im 2. Geschoss\nund berechne ihr Nettovolumen.\n\nLies das Modell nur; ändere nichts.',

    // Next step
    'Next step': 'Nächster Schritt',
    'Install Tapir': 'Tapir installieren',
    'Install Tapir, then give your AI tool the prompt above.': 'Installieren Sie Tapir und geben Sie Ihrem KI-Werkzeug dann den Prompt oben.',
    'Get started': 'Loslegen',
    'The command reference': 'Die Befehlsreferenz',
    'The list to give the AI: every command with its schema.': 'Die Liste für die KI: jeder Befehl mit seinem Schema.',
    'Open the reference': 'Zur Referenz',
    'Show what you built': 'Zeigen Sie, was Sie gebaut haben',
    'Ask about MCP servers and share your prompts.': 'Fragen Sie zu MCP-Servern und teilen Sie Ihre Prompts.',
    'Join Discord': 'Zum Discord'
});
