# Working in this directory

This is the ESPHome config tree for the house, edited from WSL over a Samba
mount (`/mnt/ha-config/esphome`) into the Home Assistant host. See
`docs/ARCHITECTURE.md` for file layout, naming conventions, and the
`components/` C++ sharing pattern — read it before restructuring anything
here. This file is about *how to operate* in this checkout, not layout.

## Running `esphome` commands

There's no system-wide `esphome` install. Use the local venv:

```bash
source ~/.local/esphome-venv/bin/activate
esphome config <file>.yaml   # validate
esphome logs <file>.yaml --device /dev/ttyUSB0   # serial
esphome run <file>.yaml --device <device-ip>     # OTA compile + upload
```

`esphome config` (validation only) is safe to run directly against files in
this Samba-mounted directory. **Do not run `esphome run`/`compile` here** —
compiling writes thousands of small object files, and doing that on the SMB
mount reliably hits `ninja: ... Input/output error` partway through a build
(confirmed 2026-09-20). If you need to actually compile+flash from WSL
(rather than via the VS Code "ESPHome: Compile and upload OTA" command,
which builds on the dashboard add-on host-side, not locally), copy the
target yaml + `secrets.yaml` + `components/` (and any device-specific
header folder, e.g. `remote-keypad/`) to a native WSL path (e.g.
`~/esphome-build/`) and run it from there instead. Clean up the copy
(including the copied `secrets.yaml`) when done.

## Watching logs after a compile/OTA push

After every `esphome run` (OTA compile+upload), give the user a
copy-pasteable command to attach to the device's live log themselves —
don't just stream logs into your own background process and describe them
secondhand; the user has no visibility into that.

The command is `esphome logs <file>.yaml --device <ip>` (API-based, not
serial — see "Network names don't resolve from WSL" below for getting
`<ip>`). Unlike `esphome run`/`compile`, this is safe to run directly
against the Samba mount: it only opens an API connection and prints
incoming log lines, it doesn't write build artifacts. It's also safe to run
concurrently with your own log-watching session if you started one during
the push — ESPHome's API server accepts multiple simultaneous connections
(see the `Max connections` line in the device's own boot log).

Example, after pushing to remote-keypad at 192.168.68.78:

```bash
source ~/.local/esphome-venv/bin/activate
esphome logs remote-keypad.yaml --device 192.168.68.78
```

## Network names don't resolve from WSL

`*.local` mDNS hostnames (`homeassistant.local`, `breadboard-esp32.local`,
etc.) don't resolve from inside WSL — WSL2's default NAT networking doesn't
pass multicast traffic. Use one of these instead:

- The device/host's real LAN IP (get it from the dashboard's `/devices`
  endpoint, e.g. `curl http://<ha-host>:6052/devices`, or from the router).
- Tailscale MagicDNS, if this machine is on the tailnet — plain hostnames
  like `homeassistant` resolve fine via `search tail4e0d65.ts.net` in
  `/etc/resolv.conf`, since that's ordinary unicast DNS, not mDNS.

Direct-IP TCP (API port 6053, OTA port 3232, dashboard port 6052) all work
fine from WSL despite the mDNS limitation — it's specifically multicast
discovery that's broken, not general LAN reachability.

## USB serial gotchas

- Confirm `usbipd attach` succeeded and check for stale processes before
  assuming a serial device is broken: `fuser /dev/ttyUSB0`. An old `esphome
  logs` process left running from a previous session will silently hold the
  port open and starve any new attempt of data. Kill it and retry.
- Expect a burst of garbled/noisy lines for the first ~1s after opening the
  port (reset transient) before real log output starts — not a sign of a
  bad connection.

## Test fixtures vs. live devices

- `breadboard-esp32.yaml` and `test-esp32c.yaml` are disposable test
  fixtures — safe to OTA-flash, reboot, or otherwise experiment on freely.
- `bedframe.yaml` and `remote-keypad.yaml` are in daily real-world use.
  Don't OTA-push, reboot, or otherwise touch the physical devices for these
  without explicit confirmation first.

## Secrets

`secrets.yaml` is gitignored and required for any compile (wifi/API
credentials). Never commit it, and never copy it outside this repo without
cleaning up the copy afterward.
