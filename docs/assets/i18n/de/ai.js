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

    // Check the names first
    'Check the names first': 'Zuerst die Namen prüfen',
    'Check names, <em>don\'t guess.</em>': 'Namen prüfen, <em>nicht raten.</em>',
    'Your words rarely match the names in the project: "fire rating" may be a property called <span class="mono">FireRating</span> in one template and <span class="mono">Feuerwiderstand</span> in another, and "level 2" may be the story "02 First Floor". An AI that guesses the name still gets an answer, only a wrong one: no values found, so "no doors have a fire rating".':
        'Ihre Worte stimmen selten mit den Namen im Projekt überein: „Brandschutzklasse“ kann in einer Vorlage eine Eigenschaft namens <span class="mono">FireRating</span> sein und in einer anderen <span class="mono">Feuerwiderstand</span>, und „Ebene 2“ kann das Geschoss „02 First Floor“ sein. Eine KI, die den Namen rät, bekommt trotzdem eine Antwort, nur eine falsche: keine Werte gefunden, also „keine Tür hat eine Brandschutzklasse“.',
    '<b aria-hidden="true">1</b><strong>Look the words up first</strong><span>Before a command whose parameters come from your words (a story, property, classification, layer, building material, composite, surface, zone category or element type), the AI sends those words to <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, with the kind of each when it knows it. It only reads.</span>':
        '<b aria-hidden="true">1</b><strong>Zuerst die Wörter nachschlagen</strong><span>Vor einem Befehl, dessen Parameter aus Ihren Worten stammen (ein Geschoss, eine Eigenschaft, Klassifizierung, Ebene, ein Baustoff, mehrschichtiges Bauteil, eine Oberfläche, Zonenkategorie oder ein Elementtyp), schickt die KI diese Wörter an <a href="../archicad-addon/#ResolveNames">ResolveNames</a>, mit der Art jedes Worts, wenn sie sie kennt. Der Befehl liest nur.</span>',
    '<b aria-hidden="true">2</b><strong>Use what it returns</strong><span><span class="mono">found</span>: the item named exactly so, with the ids the other commands take (story index, attribute id, elementType, propertyId, classification ids). <span class="mono">closeMatches</span>: only similar names, best first; the AI asks you which one you meant instead of picking one itself.</span>':
        '<b aria-hidden="true">2</b><strong>Verwenden, was er zurückgibt</strong><span><span class="mono">found</span>: das Element mit genau diesem Namen, mit den IDs, die die anderen Befehle nehmen (Geschossindex, Attribut-ID, elementType, propertyId, Klassifizierungs-IDs). <span class="mono">closeMatches</span>: nur ähnliche Namen, die besten zuerst; die KI fragt Sie, welchen Sie meinten, statt selbst einen auszuwählen.</span>',
    '<b aria-hidden="true">3</b><strong>Say "not in this project"</strong><span><span class="mono">notFound</span>: nothing in the project is close, so the project has no such item. The AI says "this project has no X" instead of guessing a name or reporting an empty result as the answer.</span>':
        '<b aria-hidden="true">3</b><strong>„Nicht in diesem Projekt“ sagen</strong><span><span class="mono">notFound</span>: Nichts im Projekt kommt dem nahe, das Projekt hat also kein solches Element. Die KI sagt „dieses Projekt hat kein X“, statt einen Namen zu raten oder ein leeres Ergebnis als Antwort zu melden.</span>',
    'Add these lines to your prompt, or to the instructions of your MCP server, so the AI checks the names every time. They work the same when the AI writes a script: the script sends <span class="mono">ResolveNames</span> first and stops with a message on <span class="mono">notFound</span>.':
        'Fügen Sie diese Zeilen Ihrem Prompt oder den Anweisungen Ihres MCP-Servers hinzu, damit die KI die Namen jedes Mal prüft. Sie wirken genauso, wenn die KI ein Skript schreibt: Das Skript sendet zuerst <span class="mono">ResolveNames</span> und bricht bei <span class="mono">notFound</span> mit einer Meldung ab.',
    'Before a change, ask for a preview. The commands that delete or replace, such as <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> and <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, take <span class="mono">"dryRun": true</span>: nothing changes, and the response lists what would. The command reference says which commands have it.':
        'Bitten Sie vor einer Änderung um eine Vorschau. Die Befehle, die löschen oder ersetzen, etwa <a href="../archicad-addon/#DeleteElements">DeleteElements</a>, <a href="../archicad-addon/#DeleteAttributes">DeleteAttributes</a> und <a href="../archicad-addon/#DeleteAndReplaceAttributes">DeleteAndReplaceAttributes</a>, nehmen <span class="mono">"dryRun": true</span>: Nichts ändert sich, und die Antwort listet auf, was sich ändern würde. Die Befehlsreferenz sagt, welche Befehle das haben.',
    'add to any request': 'zu jeder Anfrage hinzufügen',
    'Before you use a name taken from my words (a story, property, classification, layer, building material, composite, surface, zone category or element type), check it with the Tapir command ResolveNames. - found: use the ids it returns. - closeMatches: ask me which one I mean. - notFound: tell me this project has no such item. Never guess a name. Before a command that deletes or replaces, run it with "dryRun": true if it has that input, and show me what it would change.':
        'Bevor du einen Namen aus meinen Worten verwendest (ein Geschoss,\neine Eigenschaft, Klassifizierung, Ebene, einen Baustoff,\nein mehrschichtiges Bauteil, eine Oberfläche, Zonenkategorie\noder einen Elementtyp), prüfe ihn mit dem Tapir-Befehl\nResolveNames.\n- found: verwende die IDs, die er zurückgibt.\n- closeMatches: frag mich, welchen ich meine.\n- notFound: sag mir, dass dieses Projekt so etwas nicht hat.\n  Rate nie einen Namen.\nFühre einen Befehl, der löscht oder ersetzt, mit\n"dryRun": true aus, wenn er diese Eingabe hat, und zeig mir,\nwas er ändern würde.',

    // When the AI writes a script
    'When the AI writes a script': 'Wenn die KI ein Skript schreibt',
    'Describe it, <em>run it.</em>': 'Beschreiben, <em>ausführen.</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'Ohne MCP-Server beschreiben Sie die Aufgabe, geben der KI die Befehlsliste und führen das Skript aus, das sie schreibt.',
    '<b aria-hidden="true">1</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b aria-hidden="true">1</b><strong>Beschreiben, was Sie wollen</strong><span>Zum Beispiel: „Finde alle mehrschichtigen Außenwände im 2. Geschoss und berechne ihr Nettovolumen.“</span>',
    '<b aria-hidden="true">2</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b aria-hidden="true">2</b><strong>Die Befehlsliste teilen</strong><span>Geben Sie der KI die <a href="../archicad-addon/">Befehlsreferenz</a> und das funktionierende Beispiel unten. Verlangen Sie nur die Standardbibliothek von Python, auf Port 19723.</span>',
    '<b aria-hidden="true">3</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b aria-hidden="true">3</b><strong>Lesen, dann ausführen</strong><span>Lesen Sie das Skript, bevor Sie es aus der Tapir-Palette oder einem Terminal ausführen. Probieren Sie eines, das das Modell ändert, zuerst an einer Kopie Ihres Projekts aus.</span>',
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
