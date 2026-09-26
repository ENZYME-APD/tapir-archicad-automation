"""
Test des commandes Graphical Override de Tapir.

Lancer avec --cleanup pour supprimer les elements crees par ce test.
Lancer avec --dry-run pour seulement lire les overrides existants sans creer.
"""

import sys
sys.path.insert(0, r'D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples')
import aclib, json

CLEANUP  = '--cleanup'  in sys.argv
DRY_RUN  = '--dry-run'  in sys.argv
CREATED_FILE = '_graphical_override_test_ids.json'

passes = fails = 0

def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)

def check(label, expected, got):
    global passes, fails
    ok = (got == expected)
    print(f'  [{"PASS" if ok else "FAIL"}] {label}')
    if not ok:
        print(f'           expected={expected!r}  got={got!r}')
    if ok: passes += 1
    else:  fails  += 1

def check_true(label, got):
    check(label, True, bool(got))

# ---------------------------------------------------------------------------
# Mode nettoyage
# ---------------------------------------------------------------------------
if CLEANUP:
    print('==> Nettoyage ...')
    try:
        with open(CREATED_FILE) as f:
            ids = json.load(f)
        if ids.get('combinationIds'):
            run('DeleteGraphicalOverrideCombinations', {'combinationIds': ids['combinationIds']})
            print(f'  {len(ids["combinationIds"])} combinaison(s) supprimee(s).')
        if ids.get('ruleGroupIds'):
            run('DeleteGraphicalOverrideRuleGroups', {'ruleGroupIds': ids['ruleGroupIds']})
            print(f'  {len(ids["ruleGroupIds"])} groupe(s) supprime(s) (avec leurs regles).')
        import os; os.remove(CREATED_FILE)
    except FileNotFoundError:
        print('  Aucun fichier de IDs trouve.')
    sys.exit(0)

# ---------------------------------------------------------------------------
# STEP 1 — Lecture des overrides existants
# ---------------------------------------------------------------------------
print('STEP 1 -- Get (lecture)')

r_combinations = run('GetGraphicalOverrideCombinations') or {}
r_groups       = run('GetGraphicalOverrideRuleGroups')   or {}
r_rules        = run('GetGraphicalOverrideRules')        or {}

combinations   = r_combinations.get('combinations', [])
groups         = r_groups      .get('ruleGroups',   [])
rules          = r_rules       .get('rules',         [])

check_true('GetGraphicalOverrideCombinations repond', r_combinations)
check_true('GetGraphicalOverrideRuleGroups repond',   r_groups)
check_true('GetGraphicalOverrideRules repond',        r_rules)

print(f'  {len(combinations)} combinaison(s), {len(groups)} groupe(s), {len(rules)} regle(s) existantes.')

# Afficher le criterionXML de la premiere regle si disponible
if rules:
    first = rules[0]
    print(f'\n  Premiere regle existante : "{first["name"]}"')
    print(f'  criterionXML = {first["criterionXML"][:200]!r}')
    print(f'  style keys   = {list(first["style"].keys())}')

if DRY_RUN:
    print('\n--dry-run : fin.')
    sys.exit(0 if fails == 0 else 1)

# ---------------------------------------------------------------------------
# STEP 2 — Choisir un criterionXML de reference
# ---------------------------------------------------------------------------
# On copie le XML d'une regle existante. Si aucune n'existe, on utilise une
# chaine vide (ArchiCAD peut la rejeter — on verifie via executionResults).
ref_xml   = rules[0]['criterionXML'] if rules else ''
ref_style = rules[0]['style']        if rules else None

# Style minimal garanti : rien d'overridden
EMPTY_STYLE = {
    'lineType':                     {'isOverridden': False},
    'lineMarkerTextPen':            {'isOverridden': False},
    'fillOverride':                 {'isOverridden': False},
    'fillType':                     {'overrideCutFill': False, 'overrideCoverFill': False, 'overrideDraftingFill': False},
    'fillForegroundPenOverride':    {'isOverridden': False},
    'fillTypeForegroundPen':        {'overrideCutFill': False, 'overrideCoverFill': False, 'overrideDraftingFill': False},
    'fillBackgroundPenOverride':    {'isOverridden': False},
    'fillTypeBackgroundPen':        {'overrideCutFill': False, 'overrideCoverFill': False, 'overrideDraftingFill': False},
    'surfaceOverride':              {'isOverridden': False},
    'surfaceType':                  {'overrideCutSurface': False, 'overrideUncutSurface': False},
    'showSkinSeparators':           False,
    'overridePenColorAndThickness': False,
    'hiddenContours':               {'overrideCutSurface': False, 'overrideUncutSurface': False},
    'overrideContours':             False,
}
chosen_style = ref_style or EMPTY_STYLE

# ---------------------------------------------------------------------------
# STEP 3 — CreateGraphicalOverrideRuleGroups
# ---------------------------------------------------------------------------
print('\nSTEP 3 -- CreateGraphicalOverrideRuleGroups')

r_create_groups = run('CreateGraphicalOverrideRuleGroups', {
    'ruleGroupNames': ['[Tapir Test] Groupe A', '[Tapir Test] Groupe B']
}) or {}

created_groups  = r_create_groups.get('ruleGroups', [])
group_results   = r_create_groups.get('executionResults', [])

check('2 groupes retournes',   2, len(created_groups))
check('2 executionResults',    2, len(group_results))
for i, res in enumerate(group_results):
    check(f'  groupe {i} success', True, res.get('success', False))

group_a_id = created_groups[0].get('ruleGroupId') if len(created_groups) > 0 else None
group_b_id = created_groups[1].get('ruleGroupId') if len(created_groups) > 1 else None
check_true('group_a_id non vide', group_a_id)
check_true('group_b_id non vide', group_b_id)
print(f'  group_a_id = {group_a_id}')
print(f'  group_b_id = {group_b_id}')

# ---------------------------------------------------------------------------
# STEP 4 — CreateGraphicalOverrideRules
# ---------------------------------------------------------------------------
print('\nSTEP 4 -- CreateGraphicalOverrideRules')

if not group_a_id:
    print('  SKIP: creation de groupe echouee.')
else:
    # Style synthetique valide pour la creation (evite la limitation penIndex=0).
    # Note AC API: fillBackgroundPenOverride.penIndex=0 (fond transparent) est lisible
    # via GetGraphicalOverrideRules mais rejete a la creation (APIERR_BADPARS).
    # Utiliser penIndex>0 ou rgbColor pour ce champ lors de la creation.
    SYNTH_STYLE = {
        'lineType':                     {'isOverridden': False},
        'lineMarkerTextPen':            {'isOverridden': True, 'penIndex': 2},
        'fillOverride':                 {'isOverridden': False},
        'fillType':                     {'overrideCutFill': True, 'overrideCoverFill': False, 'overrideDraftingFill': False},
        'fillForegroundPenOverride':    {'isOverridden': True, 'penIndex': 3},
        'fillTypeForegroundPen':        {'overrideCutFill': True, 'overrideCoverFill': True, 'overrideDraftingFill': False},
        'fillBackgroundPenOverride':    {'isOverridden': True, 'penIndex': 1},
        'fillTypeBackgroundPen':        {'overrideCutFill': True, 'overrideCoverFill': True, 'overrideDraftingFill': False},
        'surfaceOverride':              {'isOverridden': True, 'rgbColor': {'red': 0.5, 'green': 0.5, 'blue': 0.5}},
        'surfaceType':                  {'overrideCutSurface': True, 'overrideUncutSurface': False},
        'showSkinSeparators':           False,
        'overridePenColorAndThickness': False,
        'hiddenContours':               {'overrideCutSurface': False, 'overrideUncutSurface': False},
        'overrideContours':             False,
    }

    r_create_rules = aclib.RunTapirCommand('CreateGraphicalOverrideRules', {
        'rules': [
            {'name': '[Tapir Test] Regle 1', 'ruleGroupId': group_a_id,
             'criterionXML': ref_xml, 'style': SYNTH_STYLE},
            {'name': '[Tapir Test] Regle 2', 'ruleGroupId': group_a_id,
             'criterionXML': ref_xml, 'style': SYNTH_STYLE},
        ]
    }, debug=False) or {}

    created_rules  = r_create_rules.get('rules', [])
    rule_results   = r_create_rules.get('executionResults', [])

    check('2 regles retournees',  2, len(created_rules))
    check('2 executionResults',   2, len(rule_results))
    for i, res in enumerate(rule_results):
        check(f'  regle {i} success', True, res.get('success', False))

    rule_1_id = created_rules[0].get('ruleId') if len(created_rules) > 0 else None
    rule_2_id = created_rules[1].get('ruleId') if len(created_rules) > 1 else None
    check_true('rule_1_id non vide', rule_1_id)
    check_true('rule_2_id non vide', rule_2_id)
    print(f'  rule_1_id = {rule_1_id}')
    print(f'  rule_2_id = {rule_2_id}')

# ---------------------------------------------------------------------------
# STEP 5 — CreateGraphicalOverrideCombinations
# ---------------------------------------------------------------------------
print('\nSTEP 5 -- CreateGraphicalOverrideCombinations')

rule_ids_for_combo = [rid for rid in [rule_1_id, rule_2_id] if rid]

r_create_combos = run('CreateGraphicalOverrideCombinations', {
    'combinations': [
        {'name': '[Tapir Test] Combo A', 'ruleIds': rule_ids_for_combo},
        {'name': '[Tapir Test] Combo B', 'ruleIds': []},
    ]
}) or {}

created_combos = r_create_combos.get('combinations', [])
combo_results  = r_create_combos.get('executionResults', [])

check('2 combos retournees',  2, len(created_combos))
check('2 executionResults',   2, len(combo_results))
for i, res in enumerate(combo_results):
    check(f'  combo {i} success', True, res.get('success', False))

combo_a_id = created_combos[0].get('combinationId') if len(created_combos) > 0 else None
combo_b_id = created_combos[1].get('combinationId') if len(created_combos) > 1 else None
check_true('combo_a_id non vide', combo_a_id)
print(f'  combo_a_id = {combo_a_id}')

# ---------------------------------------------------------------------------
# STEP 6 — Verification via Get
# ---------------------------------------------------------------------------
print('\nSTEP 6 -- Verification via Get')

r2_groups = run('GetGraphicalOverrideRuleGroups') or {}
r2_rules  = run('GetGraphicalOverrideRules')      or {}
r2_combos = run('GetGraphicalOverrideCombinations') or {}

groups2 = r2_groups.get('ruleGroups',   [])
rules2  = r2_rules .get('rules',         [])
combos2 = r2_combos.get('combinations', [])

n_new_groups = len(groups2) - len(groups)
n_new_rules  = len(rules2)  - len(rules)
n_new_combos = len(combos2) - len(combinations)

check('+2 groupes dans Get', 2, n_new_groups)
check(f'+{len(rule_ids_for_combo)} regles dans Get', len(rule_ids_for_combo), n_new_rules)
check('+2 combos dans Get',  2, n_new_combos)

# Verifier que combo_a contient les bons ruleIds
if combo_a_id:
    found_combo = next((c for c in combos2 if c.get('combinationId') == combo_a_id), None)
    if found_combo:
        check('combo_a contient les bonnes regles',
              sorted(rule_ids_for_combo),
              sorted(found_combo.get('ruleIds', [])))
    else:
        check('combo_a trouvee dans Get', True, False)

# ---------------------------------------------------------------------------
# Sauvegarder les IDs pour --cleanup
# ---------------------------------------------------------------------------
all_group_ids = [gid for gid in [group_a_id, group_b_id] if gid]
all_combo_ids = [cid for cid in [combo_a_id, combo_b_id] if cid]

with open(CREATED_FILE, 'w') as f:
    json.dump({'ruleGroupIds': all_group_ids, 'combinationIds': all_combo_ids}, f)

# ---------------------------------------------------------------------------
# Resume
# ---------------------------------------------------------------------------
print(f'\n{passes} PASS | {fails} FAIL')
if fails == 0:
    print('Tout est OK. Verifiez les substitutions dans ArchiCAD.')
    print(f'Nettoyage : python test_graphical_override_commands.py --cleanup')
else:
    print('Des tests ont echoue. Voir les messages ci-dessus.')

sys.exit(0 if fails == 0 else 1)
