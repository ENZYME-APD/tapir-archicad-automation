import time
import json
import aclib

print ('Fetching layers from the project...')
layersResp = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug=False)
layers = layersResp['attributes'] if layersResp else []
print ('Found {} layers.'.format (len (layers)))

layerRowsHtml = ''
for layer in layers:
    name = layer['name'].replace ('"', '&quot;')
    layerRowsHtml += '''
        <label class="layer-row">
            <input type="checkbox" class="layer-checkbox" value="{name}">
            <span>{name}</span>
        </label>
    '''.format (name=name)

HTML = '''
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Layers</title>
<style>
    :root {{
        --accent: #2f6fed;
        --bg: #f7f8fa;
        --border: #e1e4e8;
    }}
    body {{
        font-family: -apple-system, "Segoe UI", sans-serif;
        margin: 0;
        background: var(--bg);
        color: #1c1e21;
        display: flex;
        flex-direction: column;
        height: 100vh;
        box-sizing: border-box;
    }}
    header {{
        padding: 14px 18px 10px 18px;
        background: white;
        border-bottom: 1px solid var(--border);
    }}
    h1 {{
        font-size: 15px;
        margin: 0 0 10px 0;
        font-weight: 600;
    }}
    #search {{
        width: 100%;
        box-sizing: border-box;
        padding: 8px 10px;
        border: 1px solid var(--border);
        border-radius: 6px;
        font-size: 13px;
        outline: none;
    }}
    #search:focus {{
        border-color: var(--accent);
    }}
    #list {{
        flex: 1;
        overflow-y: auto;
        padding: 6px 10px;
    }}
    .layer-row {{
        display: flex;
        align-items: center;
        gap: 8px;
        padding: 7px 8px;
        border-radius: 6px;
        cursor: pointer;
        font-size: 13px;
        user-select: none;
    }}
    .layer-row:hover {{
        background: white;
    }}
    .layer-row.hidden {{
        display: none;
    }}
    .layer-checkbox {{
        accent-color: var(--accent);
        width: 15px;
        height: 15px;
    }}
    footer {{
        padding: 10px 18px;
        background: white;
        border-top: 1px solid var(--border);
        display: flex;
        justify-content: space-between;
        align-items: center;
    }}
    #count {{
        font-size: 12px;
        color: #666;
    }}
    button {{
        background: var(--accent);
        color: white;
        border: none;
        padding: 8px 16px;
        border-radius: 6px;
        font-size: 13px;
        cursor: pointer;
    }}
    button:hover {{
        opacity: 0.9;
    }}
    button:disabled {{
        background: #ccc;
        cursor: default;
    }}
</style>
</head>
<body>
    <header>
        <h1>Select layers ({count} total)</h1>
        <input id="search" type="text" placeholder="Filter layers..." oninput="filterLayers()">
    </header>
    <div id="list">
        {rows}
    </div>
    <footer>
        <span id="count">0 selected</span>
        <button id="submitBtn" onclick="submitSelection()" disabled>Apply</button>
    </footer>

    <script>
        function updateSelectedCount() {{
            var checked = document.querySelectorAll('.layer-checkbox:checked');
            document.getElementById('count').textContent = checked.length + ' selected';
            document.getElementById('submitBtn').disabled = checked.length === 0;
        }}

        function filterLayers() {{
            var query = document.getElementById('search').value.toLowerCase();
            var rows = document.querySelectorAll('.layer-row');
            rows.forEach(function (row) {{
                var text = row.textContent.toLowerCase();
                row.classList.toggle('hidden', text.indexOf(query) === -1);
            }});
        }}

        function submitSelection() {{
            var checked = document.querySelectorAll('.layer-checkbox:checked');
            var selected = Array.prototype.map.call(checked, function (cb) {{ return cb.value; }});
            ACAPI.SubmitResult(JSON.stringify(selected)).then(function () {{
                ACAPI.ClosePalette();
            }});
        }}

        document.querySelectorAll('.layer-checkbox').forEach(function (cb) {{
            cb.addEventListener('change', updateSelectedCount);
        }});
    </script>
</body>
</html>
'''.format (count=len (layers), rows=layerRowsHtml)

print ('Opening Script UI palette...')
aclib.RunTapirCommand ('ShowScriptUI', {
    'htmlContent': HTML,
    'width': 380,
    'height': 480,
    'title': 'Select Layers',
    'resizable': False,
    'zoomEnabled': False,
    'contextMenuEnabled': False,
}, debug=False)

print ('Waiting for the user to select layers and submit...')
result = None
while result is None:
    time.sleep (0.3)
    resp = aclib.RunTapirCommand ('GetScriptUIResult', {}, debug=False)
    if resp is not None and resp.get ('hasResult'):
        result = json.loads (resp['result'])

print ('Selected layers:')
print (json.dumps (result, indent=4))
