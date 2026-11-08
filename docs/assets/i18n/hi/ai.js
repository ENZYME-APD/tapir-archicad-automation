/* हिन्दी: the Tapir and AI page (ai/index.html). See ../README.md.
   The two prompts are translated too (an AI tool understands them in any language), the Python
   example stays as it is. */
(window.TAPIR_I18N = window.TAPIR_I18N || []).push ({
    'Control Archicad with AI assistants (MCP) | Tapir': 'AI assistants (MCP) से Archicad नियंत्रित करें | Tapir',
    'Control Archicad with Claude and other AI assistants through Tapir: the Tapir MCP servers, a prompt to start with, and how to let an AI write a script.':
        'Tapir के ज़रिए Claude और दूसरे AI assistants से Archicad नियंत्रित करें: Tapir के MCP सर्वर, शुरुआत के लिए एक प्रॉम्प्ट, और AI से script कैसे लिखवाएँ।',
    '<a href="../">Tapir</a> / <a href="../docs/">Docs</a> / AI':
        '<a href="../">Tapir</a> / <a href="../docs/">दस्तावेज़</a> / AI',
    'Tapir and <em>AI.</em>': 'Tapir और <em>AI।</em>',
    'An AI assistant can read your Archicad project and change it through Tapir. You ask in plain words; it sends the commands to the Archicad running on your computer.':
        'एक AI assistant, Tapir के ज़रिए आपका Archicad प्रोजेक्ट पढ़ और बदल सकता है। आप सीधे शब्दों में पूछते हैं; वह आपके कंप्यूटर पर चल रहे Archicad को कमांड भेजता है।',

    // Two ways
    'Two ways to <em>connect.</em>': 'जोड़ने के <em>दो तरीके।</em>',
    'Both talk to the same Tapir commands on port 19723, on your own computer.':
        'दोनों आपके अपने कंप्यूटर पर, port 19723 पर, उन्हीं Tapir कमांड से बात करते हैं।',
    'While you chat': 'Chat करते हुए',
    'An MCP server': 'एक MCP सर्वर',
    'The AI tool calls Tapir\'s commands itself, while you chat. Works with tools that support the Model Context Protocol (MCP), such as Claude Desktop.':
        'आपके chat करते समय AI tool ख़ुद Tapir के कमांड चलाता है। यह उन tools के साथ काम करता है जो Model Context Protocol (MCP) को support करते हैं, जैसे Claude Desktop।',
    'Tapir MCP servers': 'Tapir के MCP सर्वर',
    'Any AI tool': 'कोई भी AI tool',
    'A script the AI writes': 'AI की लिखी script',
    'Any AI tool can write a short Python script that sends Tapir commands. You read it, then run it from the Tapir palette or a terminal.':
        'कोई भी AI tool एक छोटी Python script लिख सकता है जो Tapir कमांड भेजे। आप उसे पढ़ते हैं, फिर Tapir palette या terminal से चलाते हैं।',
    'How it works': 'यह कैसे काम करता है',

    // MCP servers
    'MCP servers': 'MCP सर्वर',
    'Tapir MCP <em>servers.</em>': 'Tapir के MCP <em>सर्वर।</em>',
    'These are community projects, not part of Tapir. Each has its own install steps in its README. Which one Tapir recommends is still being decided in <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a>, so they are listed here without a recommendation.':
        'ये समुदाय के projects हैं, Tapir का हिस्सा नहीं। हर एक के README में उसके अपने install steps हैं। Tapir किसकी सिफ़ारिश करेगा, यह अभी <a href="https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2070">issue #2070</a> में तय हो रहा है, इसलिए इन्हें यहाँ बिना सिफ़ारिश के दिया गया है।',
    'By SzamosiMate': 'SzamosiMate द्वारा',
    'The MCP server for Tapir that the others started from. It is also published on PyPI as the <span class="mono">tapir-archicad-mcp</span> package.':
        'Tapir का वह MCP सर्वर जिससे बाक़ी सब शुरू हुए। यह PyPI पर <span class="mono">tapir-archicad-mcp</span> पैकेज के रूप में भी प्रकाशित है।',
    'Its repository': 'इसकी repository',
    'The package on PyPI': 'PyPI पर package',
    'By the community': 'समुदाय द्वारा',
    'Its forks': 'इसके forks',
    'Versions of it that others changed and published. Check when each was last updated.':
        'इसके वे संस्करण जिन्हें दूसरों ने बदलकर प्रकाशित किया। देख लें कि हर एक आख़िरी बार कब अपडेट हुआ।',
    'See the forks': 'Forks देखें',
    '<strong class="callout-label"><span aria-hidden="true">⚠</span> Careful</strong> An MCP server can change your model. Try it on a copy of your project first, and read what the AI is about to do before you allow it.':
        '<strong class="callout-label"><span aria-hidden="true">⚠</span> सावधान</strong> MCP सर्वर आपका मॉडल बदल सकता है। पहले इसे अपने प्रोजेक्ट की एक copy पर आज़माएँ, और AI जो करने वाला है, उसकी अनुमति देने से पहले उसे पढ़ लें।',

    // A prompt to start with
    'A prompt to start with': 'शुरुआत के लिए एक प्रॉम्प्ट',
    'Start by <em>reading.</em>': 'शुरुआत <em>पढ़ने से।</em>',
    'Open a project in Archicad, with the Tapir Add-On loaded, and give your AI tool this prompt. It only reads, so it is safe to try on any project.':
        'Tapir Add-On load करके Archicad में एक प्रोजेक्ट खोलें, और अपने AI tool को यह प्रॉम्प्ट दें। यह सिर्फ़ पढ़ता है, इसलिए किसी भी प्रोजेक्ट पर आज़माना सुरक्षित है।',
    'When the answer looks right, ask for a change in the same plain words, and say exactly which elements it may change: "Set the property X to Y on the selected walls only."':
        'जब जवाब सही लगे, तो उन्हीं सीधे शब्दों में बदलाव माँगें, और साफ़ बताएँ कि वह कौन-से elements बदल सकता है: "सिर्फ़ चुनी गई walls पर property X को Y पर सेट करो।"',
    'prompt': 'प्रॉम्प्ट',
    'read only': 'सिर्फ़ पढ़ना',
    'Use Tapir to look at the Archicad project I have open. Do not change anything; only read. 1. List the stories with their names and levels. 2. Count the walls, slabs, columns, doors and windows on each story. 3. Show the result as one table, one row per story. Tell me which Tapir commands you used.':
        'मेरे खुले हुए Archicad प्रोजेक्ट को देखने के लिए Tapir का इस्तेमाल करो।\nकुछ भी मत बदलो; सिर्फ़ पढ़ो।\n1. Stories को उनके नाम और levels के साथ सूचीबद्ध करो।\n2. हर story पर walls, slabs, columns, doors और windows गिनो।\n3. नतीजा एक table में दिखाओ, हर story के लिए एक row।\nमुझे बताओ कि तुमने कौन-से Tapir कमांड इस्तेमाल किए।',

    // When the AI writes a script
    'When the AI writes a script': 'जब AI script लिखता है',
    'Describe it, <em>run it.</em>': 'बताएँ, <em>चलाएँ।</em>',
    'Without an MCP server, describe the task, give the AI the command list, and run the script it writes.':
        'MCP सर्वर के बिना: काम बताएँ, AI को कमांड की सूची दें, और उसकी लिखी script चलाएँ।',
    '<b>01</b><strong>Describe what you want</strong><span>For example: "Extract all composite exterior walls on level 2 and calculate their net volume."</span>':
        '<b>01</b><strong>बताएँ कि आपको क्या चाहिए</strong><span>उदाहरण के लिए: "Level 2 की सभी composite exterior walls निकालो और उनका net volume निकालो।"</span>',
    '<b>02</b><strong>Share the command list</strong><span>Give the AI the <a href="../archicad-addon/">command reference</a> and the working example below. Ask for Python\'s standard library only, on port 19723.</span>':
        '<b>02</b><strong>कमांड की सूची दें</strong><span>AI को <a href="../archicad-addon/">कमांड संदर्भ</a> और नीचे दिया गया काम करने वाला उदाहरण दें। सिर्फ़ Python की standard library और port 19723 माँगें।</span>',
    '<b>03</b><strong>Read it, then run it</strong><span>Read the script before you run it, from the Tapir palette or a terminal. Try one that changes the model on a copy of your project first.</span>':
        '<b>03</b><strong>पढ़ें, फिर चलाएँ</strong><span>Script को Tapir palette या terminal से चलाने से पहले पढ़ें। जो script मॉडल बदलती है, उसे पहले अपने प्रोजेक्ट की copy पर आज़माएँ।</span>',
    'for Claude or any AI tool': 'Claude या किसी भी AI tool के लिए',
    'Write a Python script that uses only the standard library and sends Tapir commands to Archicad on http://127.0.0.1:19723, like the example I attach. Use the Tapir command reference I attach. Extract all composite exterior walls on level 2 and calculate their net volume. Only read the model; change nothing.':
        'एक Python script लिखो जो सिर्फ़ standard library\nइस्तेमाल करे और http://127.0.0.1:19723 पर\nArchicad को Tapir कमांड भेजे, साथ दिए उदाहरण की तरह।\nसाथ दिया गया Tapir कमांड संदर्भ इस्तेमाल करो।\n\nLevel 2 की सभी composite exterior walls निकालो\nऔर उनका net volume निकालो।\n\nमॉडल को सिर्फ़ पढ़ो; कुछ मत बदलो।',

    // Next step
    'Next step': 'अगला क़दम',
    'Install Tapir': 'Tapir इंस्टॉल करें',
    'Install Tapir, then give your AI tool the prompt above.':
        'Tapir इंस्टॉल करें, फिर अपने AI tool को ऊपर वाला प्रॉम्प्ट दें।',
    'Get started': 'शुरू करें',
    'The command reference': 'कमांड संदर्भ',
    'The list to give the AI: every command with its schema.':
        'AI को देने वाली सूची: हर कमांड, उसके schema के साथ।',
    'Open the reference': 'संदर्भ खोलें',
    'Show what you built': 'जो बनाया, वह दिखाएँ',
    'Ask about MCP servers and share your prompts.': 'MCP सर्वर के बारे में पूछें और अपने प्रॉम्प्ट शेयर करें।',
    'Join Discord': 'Discord से जुड़ें'
});
