/* Deutsch: the Community page (community/index.html, with the texts community/community.js reads
   from it). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Community – Archicad automation with Tapir': 'Community – Archicad-Automatisierung mit Tapir',
    'Ask on Discord, report a bug, test a fix in your Archicad: how Tapir\'s community and its bots turn a report into a release, and how the project grew.':
        'Auf Discord fragen, einen Fehler melden, eine Korrektur in Ihrem Archicad testen: wie die Community von Tapir und ihre Bots aus einer Meldung eine neue Version machen, und wie das Projekt gewachsen ist.',
    '<a href="../">Tapir</a> / Community': '<a href="../">Tapir</a> / Community',
    'Ask. Report. <em>Test.</em>': 'Fragen. Melden. <em>Testen.</em>',
    'Architects, developers and the maintainers in one place. Bugs and ideas become fixes that someone with your Archicad version tests before they ship.':
        'Architekten, Entwickler und die Maintainer an einem Ort. Aus Fehlern und Ideen werden Korrekturen, die jemand mit Ihrer Archicad-Version testet, bevor sie erscheinen.',

    // Where to find us
    'Where to find us': 'Wo Sie uns finden',
    'Ask anything': 'Fragen Sie alles',
    'Architects, developers and the maintainers, in one server. Show what you made and talk with the people who build Tapir.':
        'Architekten, Entwickler und die Maintainer auf einem Server. Zeigen Sie, was Sie gebaut haben, und sprechen Sie mit den Menschen, die Tapir entwickeln.',
    'Join Discord': 'Discord beitreten',
    'Learn the API': 'Die API lernen',
    'A video series from the first command to full workflows.': 'Eine Videoreihe vom ersten Befehl bis zu ganzen Arbeitsabläufen.',
    'Watch the tutorials': 'Die Tutorials ansehen',
    'AI assistants': 'KI-Assistenten',
    'Talk to your model': 'Sprechen Sie mit Ihrem Modell',
    'MCP servers let Claude and other assistants read and change a project through Tapir.':
        'Mit MCP-Servern können Claude und andere Assistenten ein Projekt über Tapir lesen und ändern.',

    // How the bots work
    'From report to release': 'Von der Meldung zur Version',
    'How the bots <em>work.</em>': 'Wie die Bots <em>arbeiten.</em>',
    'Most fixes are written by bots and checked by people. The maintainers set the rules and can stop any change; you decide whether a fix works in your Archicad.':
        'Die meisten Korrekturen schreiben Bots, und Menschen prüfen sie. Die Maintainer legen die Regeln fest und können jede Änderung stoppen; Sie entscheiden, ob eine Korrektur in Ihrem Archicad funktioniert.',
    '<b>01</b><strong>Post it</strong><span>Describe the bug or idea on Discord or GitHub. A GitHub account is not needed on Discord.</span>':
        '<b>01</b><strong>Posten</strong><span>Beschreiben Sie den Fehler oder die Idee auf Discord oder GitHub. Auf Discord brauchen Sie kein GitHub-Konto.</span>',
    '<b>02</b><strong>An issue</strong><span>Every half hour, Discord posts that read like a bug report or a feature request become GitHub issues, and the bot replies with the link.</span>':
        '<b>02</b><strong>Ein Issue</strong><span>Jede halbe Stunde werden Discord-Beiträge, die wie eine Fehlermeldung oder ein Funktionswunsch klingen, zu GitHub-Issues, und der Bot antwortet mit dem Link.</span>',
    '<b>03</b><strong>A draft fix</strong><span>The issue bot answers, asks for what it is missing, or writes the fix and opens a pull request. It is built for every Archicad version and reviewed.</span>':
        '<b>03</b><strong>Ein Korrekturentwurf</strong><span>Der Issue-Bot antwortet, fragt nach dem, was ihm fehlt, oder schreibt die Korrektur und öffnet einen Pull Request. Er wird für jede Archicad-Version gebaut und geprüft.</span>',
    '<b>04</b><strong>You test</strong><span>A comment on the pull request links the built add-on and asks the person who reported it to try it.</span>':
        '<b>04</b><strong>Sie testen</strong><span>Ein Kommentar im Pull Request verlinkt das gebaute Add-On und bittet die Person, die den Fehler gemeldet hat, es auszuprobieren.</span>',
    '<b>05</b><strong>Released</strong><span>Green and reviewed, it is merged three days later unless a maintainer stops it, and ships in Monday\'s <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">release</a>.</span>':
        '<b>05</b><strong>Veröffentlicht</strong><span>Grün und geprüft, wird er drei Tage später zusammengeführt, sofern kein Maintainer ihn stoppt, und erscheint in der <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/releases">Version</a> vom Montag.</span>',

    // Report and test
    'Report and test': 'Melden und testen',
    'Report': 'Melden',
    'How to report a bug': 'So melden Sie einen Fehler',
    'Post it on Discord or open an issue on GitHub; both reach the same place. A report that can be fixed in one go says:':
        'Posten Sie ihn auf Discord oder öffnen Sie ein Issue auf GitHub; beides kommt am selben Ort an. Eine Meldung, die sich in einem Zug beheben lässt, nennt:',
    'your Archicad version and build, and its language (for example 29.0.0 (3100) INT);':
        'Ihre Archicad-Version und den Build sowie die Sprache (zum Beispiel 29.0.0 (3100) INT);',
    'your Tapir version (the <em>About Tapir</em> dialog shows it);':
        'Ihre Tapir-Version (der Dialog <em>About Tapir</em> zeigt sie);',
    'what you did: the script or Grasshopper definition, or the command and the JSON you sent;':
        'was Sie getan haben: das Skript oder die Grasshopper-Definition, oder den Befehl und das JSON, das Sie gesendet haben;',
    'what you expected and what happened instead, with the error message;':
        'was Sie erwartet haben und was stattdessen passiert ist, mit der Fehlermeldung;',
    'whether the project is a Teamwork project, and which libraries are loaded, when it matters.':
        'ob das Projekt ein Teamwork-Projekt ist und welche Bibliotheken geladen sind, wenn es darauf ankommt.',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Public</strong> Issues and Discord channels are public. Leave out client names, project names and file paths you don\'t want to share.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> Öffentlich</strong> Issues und Discord-Kanäle sind öffentlich. Lassen Sie Namen von Bauherren, Projektnamen und Dateipfade weg, die Sie nicht teilen möchten.',
    'Open an issue on GitHub': 'Ein Issue auf GitHub öffnen',
    'Post on Discord': 'Auf Discord posten',
    'Test': 'Testen',
    'How to test a fix': 'So testen Sie eine Korrektur',
    'A fix is only known to work once someone has run it in Archicad. The bots can build Tapir but cannot run Archicad, so your test is what matters most.':
        'Ob eine Korrektur funktioniert, weiß man erst, wenn jemand sie in Archicad ausgeführt hat. Die Bots können Tapir bauen, aber Archicad nicht ausführen; darum zählt Ihr Test am meisten.',
    'Open the pull request linked from your issue. A comment on it lists the downloads, one per Archicad version and platform. Downloading needs a GitHub login.':
        'Öffnen Sie den Pull Request, der in Ihrem Issue verlinkt ist. Ein Kommentar darin listet die Downloads auf, einen pro Archicad-Version und Plattform. Zum Herunterladen brauchen Sie eine GitHub-Anmeldung.',
    'Unzip the download for your Archicad version. On macOS it holds another zip with the <code>.bundle</code>.':
        'Entpacken Sie den Download für Ihre Archicad-Version. Unter macOS enthält er ein weiteres Zip mit dem <code>.bundle</code>.',
    'In Archicad, open <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, remove or disable the installed Tapir, then <em>Add</em> the <code>.apx</code> (Windows) or <code>.bundle</code> (macOS) and click OK.':
        'Öffnen Sie in Archicad <em>Options &gt; Add-On Manager &gt; Edit List of Available Add-Ons</em>, entfernen oder deaktivieren Sie das installierte Tapir, fügen Sie dann mit <em>Add</em> die <code>.apx</code> (Windows) oder das <code>.bundle</code> (macOS) hinzu und klicken Sie auf OK.',
    'Do what went wrong before, in a copy of your project.':
        'Wiederholen Sie, was vorher schiefging, in einer Kopie Ihres Projekts.',
    'Say on the pull request whether it works and fixes your case. Afterwards, put your installed Tapir back the same way.':
        'Schreiben Sie im Pull Request, ob es funktioniert und Ihren Fall behebt. Setzen Sie danach Ihr installiertes Tapir auf dieselbe Weise wieder ein.',
    'Testing often? The <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">community test runner</a> tests open pull requests on your own Archicad while it is closed, leaves your Tapir in place and posts the result. It runs on Windows and needs Python and a GitHub account.':
        'Sie testen oft? Der <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/blob/main/tools/community-test-runner/GUIDE.md">Community Test Runner</a> testet offene Pull Requests in Ihrem eigenen Archicad, während es geschlossen ist, lässt Ihr Tapir an seinem Platz und veröffentlicht das Ergebnis. Er läuft unter Windows und braucht Python und ein GitHub-Konto.',

    // Momentum (community.js fills in the numbers)
    '{sign}{diff} in {days} days': '{sign}{diff} in {days} Tagen',
    'Momentum': 'Dynamik',
    'Commits': 'Commits',
    'Discord messages': 'Discord-Nachrichten',
    'Discord members <span class="change"></span>': 'Discord-Mitglieder <span class="change"></span>',
    'People taking part, last 12 months': 'Beteiligte Personen, letzte 12 Monate',
    'Top contributors': 'Die aktivsten Mitwirkenden',
    'Counted: commits (co-authors included), issues they reported that a merged fix closed, and pull requests they tested in Archicad (passed or failed).':
        'Gezählt werden Commits (Co-Autoren eingeschlossen), von ihnen gemeldete Issues, die ein zusammengeführter Fix geschlossen hat, und Pull Requests, die sie in Archicad getestet haben (bestanden oder nicht).',
    '{count} commit': '{count} Commit',
    '{count} commits': '{count} Commits',
    '{count} report': '{count} Meldung',
    '{count} reports': '{count} Meldungen',
    '{count} test': '{count} Test',
    '{count} tests': '{count} Tests',

    // Growth
    'Commits, issues and pull requests, and Discord messages per month, stacked, with the number of Discord members as a line, {first} to {last}. The table below lists every number.':
        'Commits, Issues und Pull Requests sowie Discord-Nachrichten pro Monat, gestapelt, mit der Zahl der Discord-Mitglieder als Linie, {first} bis {last}. Die Tabelle darunter listet jede Zahl auf.',
    'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec': 'Jan. Feb. März Apr. Mai Juni Juli Aug. Sep. Okt. Nov. Dez.',
    '{month} {year}': '{month} {year}',
    'Growth': 'Wachstum',
    'Growth since <em>2022.</em>': 'Wachstum seit <em>2022.</em>',
    'Everything that happens around Tapir each month, from the first commit at Enzyme to today. Only numbers are collected, never the text of a message.':
        'Alles, was rund um Tapir jeden Monat geschieht, vom ersten Commit bei Enzyme bis heute. Erfasst werden nur Zahlen, nie der Text einer Nachricht.',
    '<i class="swatch layer-commits"></i>Commits': '<i class="swatch layer-commits"></i>Commits',
    '<i class="swatch layer-github"></i>Issues and pull requests': '<i class="swatch layer-github"></i>Issues und Pull Requests',
    '<i class="swatch layer-discordMessages"></i>Discord messages': '<i class="swatch layer-discordMessages"></i>Discord-Nachrichten',
    '<i class="swatch swatch-line"></i>Discord members, right scale': '<i class="swatch swatch-line"></i>Discord-Mitglieder, rechte Skala',
    'Per month, stacked, on a square-root scale so the early years stay visible. Commits by people, without merges and bots; Discord messages include threads and forum posts. Discord members are the people in the server at the end of each month, counted by when they joined; people who later left are not included.':
        'Pro Monat, gestapelt, auf einer Quadratwurzel-Skala, damit die frühen Jahre sichtbar bleiben. Commits von Menschen, ohne Merges und Bots; Discord-Nachrichten schließen Threads und Forenbeiträge ein. Discord-Mitglieder sind die Personen auf dem Server am Ende jedes Monats, gezählt nach ihrem Beitrittsdatum; wer später gegangen ist, ist nicht enthalten.',
    'Show the numbers as a table': 'Die Zahlen als Tabelle zeigen',
    'Month': 'Monat',
    'Issues opened': 'Geöffnete Issues',
    'Pull requests merged': 'Zusammengeführte Pull Requests',
    'Discord members': 'Discord-Mitglieder',

    // Help build it
    'Help build it': 'Bauen Sie mit',
    'Made a script your office uses? Share it, and any office can install it from the palette.':
        'Haben Sie ein Skript geschrieben, das Ihr Büro nutzt? Teilen Sie es, und jedes Büro kann es aus der Palette installieren.',
    'How to share': 'So teilen Sie',
    'Ask for a feature': 'Eine Funktion wünschen',
    'How features are requested, and what is being asked for now.': 'Wie Funktionen gewünscht werden und was gerade gewünscht wird.',
    'See the roadmap': 'Zur Roadmap',
    'Code': 'Code',
    'Write a command': 'Einen Befehl schreiben',
    'The add-on is C++ on the Archicad API. The development guide shows how to add a command.':
        'Das Add-On ist C++ auf der Archicad-API. Der Entwicklungsleitfaden zeigt, wie Sie einen Befehl hinzufügen.',
    'Read the guide': 'Den Leitfaden lesen',
    'Get involved': 'Machen Sie mit',
    'We\'re building a motivated, diverse community around Archicad automation.':
        'Wir bauen eine motivierte, vielfältige Community rund um die Automatisierung von Archicad auf.',
    'Say hello on Discord, and tell us what you automate in Archicad.':
        'Sagen Sie Hallo auf Discord und erzählen Sie uns, was Sie in Archicad automatisieren.',
    'Join the Discord': 'Dem Discord beitreten'
});
