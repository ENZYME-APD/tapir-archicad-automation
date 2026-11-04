# Help test Tapir: a step-by-step guide

Tapir gets new features and fixes all the time. Before they go out to everyone, someone has to try
them in a real Archicad. The **Tapir test runner** does that on your computer, by itself, and
tells the developers whether each change works.

You don't need to know programming. Setting it up takes about 15 minutes, once.

---

## What it does on your computer

- It only works while Archicad is **closed**. It never touches an Archicad you have open, so it
  never gets in the way of your work.
- When there is something to test, it starts Archicad with an **empty, new project**, tries the
  new Tapir version in it for a minute or two, and closes Archicad again. Your own projects are
  never opened.
- Afterwards it **puts your own Tapir back** exactly as it was. Nothing in your Archicad
  installation is changed.
- It posts the result (pass or fail) on GitHub under your name, so the developers can see it.

## What you need

- Windows 10 or 11
- Archicad 26 or newer (any you have installed)
- A free GitHub account: <https://github.com/signup>
- Python, a free program the runner is written in. Step 1 installs it.

---

## Step 1: Install Python (once)

1. Go to <https://www.python.org/downloads/windows/> and click the top **Download Python 3.x** button.
2. Open the downloaded file.
3. On the first screen, **tick "Add python.exe to PATH"** at the bottom. This matters.
4. Click **Install Now** and wait until it says *Setup was successful*. Close it.

Already have Python 3.10 or newer? Skip this step.

## Step 2: Download the test runner

1. Go to <https://github.com/ENZYME-APD/tapir-archicad-automation>.
2. Click the green **Code** button, then **Download ZIP**.
3. Open your Downloads folder, right-click the ZIP file and choose **Extract All…**. Pick a place
   you will find again, for example your *Documents* folder.
4. In the extracted folder, open `tools`, then `community-test-runner`.

## Step 3: Start it

Double-click **`Start Tapir Test Runner.bat`**.

> If Windows shows *"Windows protected your PC"*, click **More info**, then **Run anyway**. This
> appears for any file downloaded from the internet that is not from the Store.

A black window opens and asks a few questions. Press **Enter** to accept the suggestion shown
in square brackets `[like this]`.

1. **Which Archicad versions to test.** It lists the Archicads it found. Press Enter to use all
   of them, or type the numbers you want, for example `30` or `29,30`.
2. **A GitHub token.** This lets the runner download the test versions and post results in your
   name. It is like a password only for this, which you can cancel at any time.
   - Press Enter to open the GitHub page. Sign in if asked.
   - **Note:** type `Tapir test runner`. **Expiration:** choose how long it should work (90 days
     is fine; you can make a new one later).
   - The box **public_repo** is already ticked. Don't tick anything else.
   - Scroll down and click **Generate token**. Click the copy icon next to the long code that
     starts with `ghp_`.
   - Back in the black window, right-click to paste it, and press Enter. **Nothing appears while
     you paste**; that is normal, it is hidden like a password.
   - It says *signed in as* your name. The token is kept safely in Windows (Credential Manager).
3. **Test pull requests from these forks.** Press Enter. (It means: also test changes from your
   own GitHub account. Leave it as it is unless the developers tell you otherwise.)
4. **Open the runner's window every time you log on?** Type `y` if you want it to start by
   itself with Windows, so it tests whenever your computer is on and Archicad is closed.
   Otherwise press Enter.
5. **Claude Code skill?** Press Enter (that's for developers).

Then the runner's window opens. From now on, double-clicking `Start Tapir Test Runner.bat`
opens the window directly.

---

## Using the window

The window has a list at the top, a row of buttons, and a log at the bottom that tells you what
is happening.

- **The list** shows the changes (*pull requests*) waiting on GitHub: their number and title,
  whose they are, how many checks they have, which Archicad versions are ready to test, and the
  last results. **Gray** ones can't be tested by you; that's fine.
- **Automatic testing:** every 15 minutes it tests what the developers marked for testing. The
  line at the top shows when the next round is. Tick **Pause automatic testing** to stop that.
- **Test selected:** click one or more rows (hold Ctrl for several), then this button.
- **Test everything due:** tests what is marked for testing, now.
- **Stop after this test:** finishes the current test, then stops.
- **Put my add-on back:** use this if a test was interrupted (see *If something goes wrong*).
- **Benchmark open Archicad:** measures how fast your Tapir answers and how big its answers are.
  Open Archicad with a new, empty project (don't save it) first. It takes a few minutes, adds
  up to 5000 elements and deletes them again, then opens a report and says what changed since
  your last benchmark. Nothing is posted.
- **Post results on GitHub:** untick it to try the runner without posting anything.
- **Run every check:** also runs the general checks, not only the change's own.
- **Settings:** asks the setup questions again.
- **Help:** opens this guide.
- Double-click a row to see the change on GitHub.

**Remember: close Archicad when you want tests to run.** While it is open, the log says
*"AC30 is open - skipped"* and nothing happens. That's on purpose.

A test takes about two minutes per Archicad version. You'll see Archicad open, nothing happen
on screen for a bit (the checks run inside), and Archicad close again.

### Reading the results

In the log, and on GitHub:

- `#1088 AC30 PASS 12/12` — every check passed in Archicad 30.
- `#1088 AC30 FAIL 10/12` — two checks failed; the lines below it say which. That's useful
  information for the developers, not a problem on your side.
- `skipped: no finished AC30 build` — GitHub hasn't built that version yet. It will be tested
  in a later round.
- `skipped: no check files` — the change doesn't have checks yet.
- `skipped: already reported` — you already tested this exact version.

---

## If something goes wrong

**Nothing seems to happen.** Read the last line of the log. It says how many changes are marked
for testing and whether Archicad is open. *"0 marked 'needs-test'"* means there is nothing for
automatic testing to do yet; that's normal, and you can still select a row and click **Test
selected**. *"Archicad 30 is open"* means tests wait until you close it. While the list is being
read, the top line counts up (*Reading pull requests 43/100*); that takes about half a minute.

**Archicad shows a question when the runner starts it** (for example *Project Recovery* after an
earlier crash, or a licence message). The runner can't answer questions for you. Answer it
yourself, close Archicad, then click **Put my add-on back**. Choose *Cancel* in Project Recovery
if you want to keep the recovery for later; it will then ask again next time, so it's better to
open or delete it once yourself.

**The log says "did not quit" or "stopped".** Close Archicad yourself (don't save the *Untitled*
project), then click **Put my add-on back**.

**Archicad crashed.** The crash report window is from Archicad. Send it if you like. Then click
**Put my add-on back**. The result on GitHub notes the crash.

**My own Tapir is missing in Archicad.** Close Archicad and click **Put my add-on back** (or
double-click `Start Tapir Test Runner.bat` and use the button). This always works, even after a
power cut, because the runner saves your setup before every test.

**"GitHub's request limit is used up"** or **"no GitHub token"**: click **Settings** and paste
a token again (Step 3.2). Tokens expire after the time you chose.

**The black window closes immediately or says Python was not found:** do Step 1 again and make
sure "Add python.exe to PATH" is ticked.

## Updating

Download the ZIP again (Step 2) and extract it over the old folder, or into a new one. Your
answers and token are kept, so it opens the window directly.

## Removing it

1. Close Archicad and the runner's window.
2. Open the `community-test-runner` folder, click the address bar, type `cmd` and press Enter.
3. Type `python tapir_test_runner.py uninstall` and press Enter. It puts your Tapir back and
   removes the token, the logon task and the runner's files.
4. Delete the folder you extracted, and cancel the token on <https://github.com/settings/tokens>.

## Questions

Ask in the Tapir Discord (link on <https://github.com/ENZYME-APD/tapir-archicad-automation>), or
open an issue on GitHub.
