import matplotlib.pyplot as plt
import matplotlib.patches as patches
import os

def generate_infographic(output_paths):
    # Set dark theme background matching Discord dark mode
    plt.style.use('dark_background')
    fig, ax = plt.subplots(figsize=(16, 9), dpi=150)
    fig.patch.set_facecolor('#18191c')
    ax.set_facecolor('#18191c')
    ax.set_xlim(0, 16)
    ax.set_ylim(0, 9)
    ax.axis('off')

    # Top Header Banner
    header_box = patches.FancyBboxPatch(
        (0.6, 7.4), 14.8, 1.2,
        boxstyle="round,pad=0.1,rounding_size=0.2",
        facecolor='#232428', edgecolor='#383a40', linewidth=1.5
    )
    ax.add_patch(header_box)

    ax.text(0.9, 8.15, "TAPIR ARCHICAD v1.7.3 — AI CONTEXT & CRITERIA EVALUATOR", 
            fontsize=20, fontweight='bold', color='#ffffff', va='center')
    ax.text(0.9, 7.7, "Special Benchmark on 1,000 Elements in Archicad 30 | Full Client Dumps vs Tapir Server-Side Optimization", 
            fontsize=12, color='#949ba4', va='center')
    ax.text(14.8, 8.0, "PR READY", 
            fontsize=12, fontweight='bold', color='#10b981', ha='right', va='center',
            bbox=dict(boxstyle="round,pad=0.3", fc="#10b98122", ec="#10b981", lw=1))

    # 4 Key Stat Metric Cards
    cards_data = [
        {"title": "PROPERTY SEARCH", "value": "-99.90%", "sub": "617k -> 641 tokens", "color": "#10b981", "x": 0.6},
        {"title": "ELEMENT COUNTING", "value": "~8 TOKENS", "sub": "30B (99.92% cut)", "color": "#38bdf8", "x": 4.4},
        {"title": "CRITERIA FILTERING", "value": "37 ms", "sub": "3.7x faster in C++", "color": "#a855f7", "x": 8.2},
        {"title": "BULK FAVORITES", "value": "1.32 sec", "sub": "300 walls, 1 undo", "color": "#f59e0b", "x": 12.0}
    ]

    for c in cards_data:
        box = patches.FancyBboxPatch(
            (c["x"], 5.7), 3.4, 1.4,
            boxstyle="round,pad=0.1,rounding_size=0.15",
            facecolor='#2b2d31', edgecolor='#383a40', linewidth=1.2
        )
        ax.add_patch(box)
        ax.text(c["x"] + 0.2, 6.75, c["title"], fontsize=9, fontweight='bold', color='#949ba4', va='center')
        ax.text(c["x"] + 0.2, 6.3, c["value"], fontsize=22, fontweight='black', color=c["color"], va='center')
        ax.text(c["x"] + 0.2, 5.92, c["sub"], fontsize=9.5, color='#dbdee1', va='center')

    # Main Visual Comparison Section
    # Left Box: Benchmark Comparison Bars
    bench_box = patches.FancyBboxPatch(
        (0.6, 0.7), 8.8, 4.7,
        boxstyle="round,pad=0.1,rounding_size=0.2",
        facecolor='#232428', edgecolor='#383a40', linewidth=1.5
    )
    ax.add_patch(bench_box)

    ax.text(0.9, 5.0, "1,000 Elements Benchmark: Token & Payload Reduction", 
            fontsize=13, fontweight='bold', color='#ffffff', va='center')

    # Bar 1: Property Discovery
    ax.text(0.9, 4.45, "Property Discovery (Finding 'Fire' rating):", fontsize=10, fontweight='bold', color='#dbdee1')
    
    # Baseline bar
    b1_naive = patches.FancyBboxPatch((0.9, 3.9), 8.0, 0.35, boxstyle="round,pad=0.03", fc='#ef4444', ec='none')
    ax.add_patch(b1_naive)
    ax.text(1.1, 4.07, "Full Dump GetAllProperties: 617,838 tokens (2.35 MB)", fontsize=9, color='#ffffff', fontweight='bold', va='center')

    # Tapir bar
    b1_tapir = patches.FancyBboxPatch((0.9, 3.4), 0.2, 0.35, boxstyle="round,pad=0.03", fc='#10b981', ec='none')
    ax.add_patch(b1_tapir)
    ax.text(1.3, 3.57, "Tapir SearchProperties: 641 tokens (2.4 KB)  [-99.90%]", fontsize=9, color='#10b981', fontweight='bold', va='center')

    # Bar 2: Element Counting
    ax.text(0.9, 2.85, "Element Counting (Count 600 walls in project):", fontsize=10, fontweight='bold', color='#dbdee1')
    
    b2_naive = patches.FancyBboxPatch((0.9, 2.3), 8.0, 0.35, boxstyle="round,pad=0.03", fc='#ef4444', ec='none')
    ax.add_patch(b2_naive)
    ax.text(1.1, 2.47, "Full GUID List Dump: 10,267 tokens (39 KB)", fontsize=9, color='#ffffff', fontweight='bold', va='center')

    b2_tapir = patches.FancyBboxPatch((0.9, 1.8), 0.1, 0.35, boxstyle="round,pad=0.03", fc='#38bdf8', ec='none')
    ax.add_patch(b2_tapir)
    ax.text(1.3, 1.97, "Tapir countOnly: true: 8 tokens (30 bytes)  [-99.92%]", fontsize=9, color='#38bdf8', fontweight='bold', va='center')

    # Latency / Bulk note
    b3_note = patches.FancyBboxPatch((0.9, 1.0), 8.2, 0.55, boxstyle="round,pad=0.05", fc='#2b2d31', ec='#4e5058', lw=1)
    ax.add_patch(b3_note)
    ax.text(1.1, 1.28, "⚡ Criteria Evaluator: 1,000 elements filtered server-side in 37 ms without JSON bloat", fontsize=9, color='#dbdee1', va='center')

    # Right Box: 4 New Endpoints
    feat_box = patches.FancyBboxPatch(
        (9.8, 0.7), 5.6, 4.7,
        boxstyle="round,pad=0.1,rounding_size=0.2",
        facecolor='#232428', edgecolor='#383a40', linewidth=1.5
    )
    ax.add_patch(feat_box)

    ax.text(10.1, 5.0, "4 New Core Tapir Commands", fontsize=13, fontweight='bold', color='#ffffff', va='center')

    features = [
        {"num": "1", "name": "SearchProperties", "desc": "Fast keyword & group search. No context overflow.", "col": "#38bdf8"},
        {"num": "2", "name": "SearchFavorites", "desc": "Cross-type favorite search with folder hierarchy.", "col": "#10b981"},
        {"num": "3", "name": "GetElementsByCriteria", "desc": "Native C++ Criteria Evaluator with countOnly.", "col": "#a855f7"},
        {"num": "4", "name": "ApplyFavoritesToElements", "desc": "Bulk apply favorite to 300+ elements in 1 step.", "col": "#f59e0b"}
    ]

    y_pos = 4.2
    for f in features:
        # circle number
        circle = patches.Circle((10.3, y_pos + 0.1), 0.22, fc=f["col"], ec='none')
        ax.add_patch(circle)
        ax.text(10.3, y_pos + 0.1, f["num"], color='#18191c', fontsize=9, fontweight='black', ha='center', va='center')
        ax.text(10.7, y_pos + 0.2, f["name"], color='#ffffff', fontsize=10.5, fontweight='bold', va='center')
        ax.text(10.7, y_pos - 0.1, f["desc"], color='#949ba4', fontsize=8.5, va='center')
        y_pos -= 0.95

    # Save to all requested paths
    for p in output_paths:
        os.makedirs(os.path.dirname(os.path.abspath(p)), exist_ok=True)
        plt.savefig(p, facecolor=fig.get_facecolor(), edgecolor='none', bbox_inches='tight')
        print(f"Saved infographic to: {p}")

    plt.close()

if __name__ == '__main__':
    desktop_path = os.path.join(os.environ.get('USERPROFILE', ''), 'Desktop', 'Tapir_AI_Optimization_Benchmark.png')
    artifact_path = r'C:\Users\Admin\.gemini\antigravity\brain\5291ebb9-5af7-4436-83ff-d9cbbcc41c80\Tapir_AI_Optimization_Benchmark.png'
    repo_path = r'c:\Users\Admin\Documents\tapir no gravity\docs\Tapir_AI_Optimization_Benchmark.png'
    generate_infographic([desktop_path, artifact_path, repo_path])
