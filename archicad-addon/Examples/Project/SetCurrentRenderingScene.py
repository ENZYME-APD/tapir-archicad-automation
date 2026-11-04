"""Makes the last rendering scene the current one with SetCurrentRenderingScene, then restores the original scene."""
import aclib

scenes = aclib.RunTapirCommand ('GetRenderingScenes')
if not scenes['sceneNames']:
    raise SystemExit ('No rendering scene in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentRenderingScene', {'sceneName': scenes['sceneNames'][-1]}))
aclib.RunTapirCommand ('GetRenderingScenes')

if 'currentSceneName' in scenes:
    aclib.PrintJson (aclib.RunTapirCommand ('SetCurrentRenderingScene', {'sceneName': scenes['currentSceneName']}))
