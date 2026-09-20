# ESPHome config layout

## Naming

Device files are named for what the device *is*, not ESPHome's auto-generated
`esphome-web-<mac>.yaml`. Filename, `esphome.name`, and `friendly_name` should
match (kebab-case), e.g. `remote-keypad.yaml` / `name: remote-keypad`.

Two exceptions, both deliberate:

- **`breadboard-esp32.yaml`** is a permanent breadboard fixture, not a project.
  What's actually wired up on it rotates, so the filename names the *board*
  and a one-line comment at the top of the file says what it's currently
  testing. Update that comment whenever the breadboard gets repurposed —
  it's cheap, and it's the thing the old MAC-address filename never told you.
- **`bedframe.yaml`** is in daily use, so only the *filename* was changed
  (from `esphome-web-ee069c.yaml`). Its `esphome.name` is still
  `esphome-web-ee069c` internally — renaming that too would change the
  device's mDNS identity and could disturb existing Home Assistant entity
  IDs/automations tied to it. Renaming the internal identity is a separate,
  bigger decision, left for later if it's ever worth doing deliberately.

## Reusable C++ logic → `components/`

Shared C++ helpers (currently `custom_hash.h`, `fram_mb85rc256.h`) live in
`components/` and are pulled in via a relative `includes:` path, e.g.:

```yaml
esphome:
  includes:
    - components/fram_mb85rc256.h
```

`fram_mb85rc256.h` wraps raw FRAM (I2C) reads/writes in a small class
(`fram_mb85rc256::FramMB85RC256`). The convention going forward: when a
device's `lambda:` blocks are doing the same multi-line read/write/verify
dance in more than one place, add a method to the relevant header instead of
copy-pasting the C++ into the YAML. `remote-keypad.yaml` is the current
example of this pattern. `breadboard-esp32.yaml` used to duplicate this kind
of FRAM logic across two lambdas — if FRAM hardware goes back on that
breadboard, extract it into `fram_mb85rc256.h` methods rather than
re-inlining it.

Deliberately *not* done: extracting the common `wifi:`/`api:`/`ota:`/
`logger:`/`esp32:` boilerplate into a shared `packages:` file. Every device's
full YAML key set stays visible in its own file — worth the repetition.

## `packages/`

Exists, currently unused. Available later if a fragment is genuinely
identical across multiple devices in a way that doesn't hide any one
device's own config — not for the common top-level boilerplate (see above).

## Editing + compiling from VS Code

Two ways to reach this folder, both wired up to the same ESPHome Dashboard
add-on ("Device Builder"):

- **Remote-SSH** into the HA host — works well for editing and OTA
  compiling, but its extension host runs entirely on the HA host, so it has
  no access to hardware plugged into your own laptop/desktop.
- **Remote-WSL**, editing via the Samba share mounted inside WSL — every
  extension (including serial tools) then runs natively with real local
  hardware access. This is the primary workflow going forward.

Either way, install the official **ESPHome** VS Code extension
(`ESPHome.esphome-vscode`) — recommended automatically via
`.vscode/extensions.json`. `.vscode/settings.json` points it at the live
dashboard in "dashboard" validator mode:

```json
{
  "esphome.validator": "dashboard",
  "esphome.dashboardUri": "http://homeassistant.local:6052/"
}
```

This gives inline YAML validation/autocomplete matched to the addon's actual
ESPHome version, plus a command-palette action, **"ESPHome: Compile and
upload OTA"**. If `homeassistant.local` doesn't resolve from wherever you're
editing (e.g. from inside the SSH addon container specifically, try the
addon's internal Docker gateway address instead, re-derivable via
`ha addons info 5c53de3b_esphome --raw-json | grep ip_address` — it was
`172.30.32.1:6052` at time of writing), fall back to the HA host's real LAN
IP.

USB flashing (first adoption, or recovery) still goes through the Device
Builder's browser-based WebSerial flash in the HA web UI — that already
works from whichever machine the browser is running on, no new tooling
needed.

Known quirk: an Install can fail partway through, typically on a C-file
compile step — this happens during compilation itself, so it's not
specific to WebSerial/USB, OTA installs can hit it too, and it isn't
specific to any one device or file either. Just hit Install again — it
resumes from where it left off rather than starting a full rebuild, and
normally succeeds on the retry.

### Setting this up on a new/different machine

Everything below is per-machine — `.vscode/settings.json` in this repo
covers the ESPHome extension config, but the extension itself and the
mount/SSH connection have to be set up on each new machine you edit from.

1. **Get the files onto that machine.** Either connect VS Code to them
   remotely — Remote-SSH into the HA host, or Remote-WSL over the Samba
   share mounted in WSL (see above) — or clone/pull this git repo directly
   if you just want a local copy without live device-builder integration
   (note: `secrets.yaml` is gitignored, so a fresh clone won't have it —
   copy it over separately, it's not something to commit).
2. **Install the ESPHome VS Code extension** (`ESPHome.esphome-vscode`).
   If you opened the folder via Remote-SSH/Remote-WSL, VS Code should
   prompt you to install it automatically (it's in
   `.vscode/extensions.json`); otherwise install it manually from the
   marketplace.
3. **Confirm the dashboard is reachable** from that machine:
   `curl http://homeassistant.local:6052/` (or open it in a browser). If
   `.local` mDNS doesn't resolve there, use the HA host's real LAN IP
   instead and update `esphome.dashboardUri` in `.vscode/settings.json`
   accordingly (that's a shared repo file, so if different machines need
   different addresses, override it locally in that machine's user/profile
   settings instead of editing the repo's copy).
4. **For local USB serial logs on that machine specifically**, see the
   WSL + `usbipd-win` steps below — those are Windows/WSL-specific and
   need to be repeated on each machine you plan to plug a device into.

## Local USB serial logs (short-wake devices)

Useful for devices like `remote-keypad.yaml` that deep-sleep and only wake
briefly — OTA/API often doesn't have time to connect before they go back to
sleep, so serial is the only way to see boot output. This has to happen on
your actual laptop/desktop, not the HA host, since a locally-plugged-in USB
device is invisible to a Remote-SSH session.

Recommended path — Samba mount + WSL. **Confirmed working end-to-end**
(flash → `usbipd attach` → live `esphome logs` streaming) using
`breadboard-esp32.yaml` as the test device:

1. Mount the HA Samba share inside WSL and open `esphome/` there via VS
   Code's **Remote - WSL** extension (not Remote-SSH).
2. WSL2 doesn't see Windows USB devices by default. Install `usbipd-win` on
   the Windows side (`winget install usbipd-win`), then:
   - `usbipd bind --busid <id>` (one-time per device, needs an **elevated**
     Windows PowerShell — this is a UAC prompt, not scriptable)
   - `usbipd attach --wsl --busid <id>` to attach it into the running WSL
     distro, after which it shows up as `/dev/ttyUSB0` (or similar) in WSL.
   - Gotcha: if the Device Builder web UI still has a browser tab open with
     its log viewer/WebSerial connection to the same device, `attach` fails
     with "Device busy (exported)". Close that tab first — no `--force`
     needed, it just needs the browser to let go of the port.
   - For permanent (no-sudo) access to `/dev/ttyUSB*` in WSL, add your user
     to the `dialout` group (`sudo usermod -aG dialout $USER`) — takes
     effect on next WSL login/restart. For an immediate one-off session,
     `sudo chmod 666 /dev/ttyUSB0` works without waiting for that.
3. From WSL: `esphome logs <file>.yaml --device /dev/ttyUSB0`, or a
   serial-monitor VS Code extension running in the WSL extension host.
   `esphome` needs a local install (`pip install esphome`, independent of
   the HA-hosted one) — on newer Ubuntu WSL images the system Python is
   "externally managed" and blocks a bare `pip install`, so install it into
   a venv instead (e.g. `python3 -m venv ~/.local/esphome-venv && source
   ~/.local/esphome-venv/bin/activate && pip install esphome`).

A VS Code extension trying to reach local hardware *through* a Remote-SSH
session was tried and doesn't reliably work — Microsoft's own Serial
Monitor extension has an open, unresolved issue for exactly that scenario,
and the repo is now archived. Don't fight that; use WSL instead.

## Follow-up work (not done in this pass)

- **`bedframe.yaml`**: its three movement scripts (`move_head_to_target`,
  `move_legs_to_target`, `move_to_target_script`) duplicate the same
  if/wait_until/switch logic three times. ESPHome scripts support typed
  `parameters:` — consolidate into one parameterized script, called with
  different axis/target arguments. Deserves dedicated testing since it's a
  daily-use device (an actual bed).
- **`bedframe.yaml`**'s `esphome.name` (still `esphome-web-ee069c`) — decide
  separately whether it's worth renaming to `bedframe`, given the HA
  entity-ID/automation impact.
- **`remote-keypad.yaml`** has a dangling comment at the top referencing
  `definitions/boards/esp32-c3-supermini/manifest.yaml`, which doesn't exist
  anywhere in this repo. Either write that manifest or remove the comment.
- **`breadboard-esp32.yaml`**: if FRAM hardware goes back on the breadboard,
  extract the read/write/verify logic into `fram_mb85rc256.h` methods
  instead of re-inlining it into lambdas.
