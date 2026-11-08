/* हिन्दी: the Docs page (docs/index.html). See ../README.md. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Archicad automation docs | Tapir': 'Archicad automation दस्तावेज़ | Tapir',
    'Tapir\'s documentation: every JSON command, the Grasshopper guide, the Archicad API coverage and the Grasshopper coverage, and how a call to Tapir works.':
        'Tapir के दस्तावेज़: हर JSON कमांड, Grasshopper गाइड, Archicad API कवरेज और Grasshopper कवरेज, और Tapir को भेजा गया एक call कैसे काम करता है।',
    '<a href="../">Tapir</a> / Docs': '<a href="../">Tapir</a> / दस्तावेज़',
    'The <em>reference.</em>': '<em>संदर्भ।</em>',
    'Every command, what Tapir covers of the Archicad API, and how the Grasshopper components map to it.':
        'हर कमांड, Tapir, Archicad API का कितना हिस्सा cover करता है, और Grasshopper components उससे कैसे जुड़ते हैं।',
    'The docs': 'दस्तावेज़',

    // The four docs
    'Tapir commands': 'Tapir कमांड',
    'Search by name, filter by the version a command came in, and open any command for its input and output schema and a Python example.':
        'नाम से खोजें, जिस संस्करण में कमांड आया उससे filter करें, और किसी भी कमांड को खोलकर उसका input और output schema और एक Python उदाहरण देखें।',
    'Open the reference': 'संदर्भ खोलें',
    'Grasshopper guide': 'Grasshopper गाइड',
    'The tool components, one per Archicad tool, with inputs named after the settings dialog. Install, a first definition, and what the buttons do.':
        'Tool components, हर Archicad tool के लिए एक, जिनके inputs के नाम settings dialog जैसे हैं। इंस्टॉल करना, पहली definition, और buttons क्या करते हैं।',
    'Read the guide': 'गाइड पढ़ें',
    'Archicad API': 'Archicad API',
    '· <span data-live-value="coverage"></span> covered': '· <span data-live-value="coverage"></span> कवर',
    'Every function of Graphisoft\'s Archicad API and whether Tapir uses it, is working on it, or skipped it, updated every hour.':
        'Graphisoft के Archicad API का हर function, और Tapir उसे इस्तेमाल करता है, उस पर काम चल रहा है, या उसे छोड़ दिया गया है, हर घंटे अपडेट होता है।',
    'See the coverage': 'कवरेज देखें',
    'Components': 'कंपोनेंट्स',
    'Which Tapir commands have a Grasshopper component, and which inputs each component still lacks.':
        'किन Tapir कमांड का Grasshopper component है, और हर component में अभी कौन-से inputs बाक़ी हैं।',

    // The API limits teaser (its title is in common.js)
    'What the Archicad API can\'t do, such as undo, the clipboard or the rows of a schedule, with what to do instead and the DevKit evidence.':
        'Archicad API क्या नहीं कर सकता, जैसे undo, clipboard या किसी schedule की rows, साथ में उसकी जगह क्या करें और DevKit का सबूत।',
    'See the limits': 'सीमाएँ देखें',

    // How it works
    'How it works': 'यह कैसे काम करता है',
    'How it <em>works.</em>': 'यह कैसे <em>काम करता है।</em>',
    'Tapir is an Archicad add-on that turns native API functions into a stateless JSON server on port 19723. Anything that can send an HTTP request can drive Archicad.':
        'Tapir एक Archicad add-on है जो native API functions को port 19723 पर एक stateless JSON server में बदल देता है। जो कुछ भी HTTP request भेज सकता है, वह Archicad को चला सकता है।',
    'Grasshopper, Python scripts and AI tools send JSON over HTTP to the Tapir Add-On, which calls Archicad\'s C++ API on the BIM model.':
        'Grasshopper, Python scripts और AI tools, HTTP पर Tapir Add-On को JSON भेजते हैं, जो BIM मॉडल पर Archicad का C++ API बुलाता है।',
    '<strong>Grasshopper</strong><span>The tapir components</span>': '<strong>Grasshopper</strong><span>tapir components</span>',
    '<strong>Python scripts</strong><span>The palette, or your own</span>': '<strong>Python की scripts</strong><span>Palette की, या आपकी अपनी</span>',
    '<strong>AI and MCP</strong><span>Assistants and MCP servers</span>': '<strong>AI और MCP</strong><span>AI सहायक और MCP सर्वर</span>',
    '↓ JSON over HTTP ↓': '↓ HTTP पर JSON ↓',
    '<strong>Tapir Add-On</strong><span>A stateless JSON layer: one command per request</span>':
        '<strong>Tapir का Add-On</strong><span>एक stateless JSON layer: हर request पर एक कमांड</span>',
    '<strong>Archicad</strong><span>The BIM model and its C++ API</span>': '<strong>Archicad</strong><span>BIM मॉडल और उसका C++ API</span>',

    // Anatomy of a call
    'Anatomy of a call': 'एक call की बनावट',
    'Every Tapir command is sent the same way; only <code>commandName</code> and <code>addOnCommandParameters</code> change. <a href="../start/#python">Your first Python script</a>':
        'हर Tapir कमांड एक ही तरीके से भेजा जाता है; सिर्फ़ <code>commandName</code> और <code>addOnCommandParameters</code> बदलते हैं। <a href="../start/#python">आपकी पहली Python script</a>',

    // More guides
    'More guides': 'और गाइड',
    'Install and run a script': 'इंस्टॉल करें और एक script चलाएँ',
    'Install Tapir, run a script from the palette and write your first Python script.':
        'Tapir इंस्टॉल करें, palette से एक script चलाएँ और अपनी पहली Python script लिखें।',
    'Get started': 'शुरू करें',
    'AI': 'AI',
    'MCP servers and scripts an AI assistant writes for you.': 'MCP सर्वर, और वे scripts जो एक AI assistant आपके लिए लिखता है।',
    'Use Tapir with AI': 'AI के साथ Tapir इस्तेमाल करें',
    'Learn by watching': 'देखकर सीखें',
    'Six videos, from installing Tapir to whole workflows.': 'छह वीडियो, Tapir इंस्टॉल करने से लेकर पूरे workflows तक।',
    'Watch the tutorials': 'ट्यूटोरियल देखें'
});
