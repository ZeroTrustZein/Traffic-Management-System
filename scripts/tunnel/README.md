# Crow project — Connect to the docker laptop

This folder has everything you need to reach the **Traffic Management
System** running on your **docker laptop** (`100.65.93.24`) from your
**main laptop**.

The Docker stack on the docker laptop exposes:

| Port | Service                              |
| ---- | ------------------------------------ |
| 8000 | Django gateway (UI + API proxy)      |
| 8080 | Crow C++ REST backend                |
| 5432 | PostgreSQL database                  |

Normally you'd only see those ports on the docker laptop itself. The
scripts in this folder set up SSH tunnels so the same ports appear on
**your main laptop** at `localhost`. That way your browser, Postman,
and `psql` work the same as if the stack were running locally.

---

## One-time setup (already done for you)

Your `~/.ssh/config` already has:

```sshconfig
Host dockerlaptop dl 100.65.93.24 me@100.65.93.24
    HostName 100.65.93.24
    User me
    IdentityFile ~/.ssh/id_ed25519_dockerlaptop
    IdentitiesOnly yes
    ServerAliveInterval 30
    ServerAliveCountMax 3
```

That means any of these commands work:

```bash
ssh dockerlaptop                # shell on docker laptop
ssh dockerlaptop 'docker ps'    # one-off command
ssh 100.65.93.24                # same thing, by IP
ssh dl                          # short alias
```

---

## Daily use — the `crow-tunnel` script

Pick the version that matches your shell:

| Shell                | File                  | How to run                                |
| -------------------- | --------------------- | ----------------------------------------- |
| PowerShell / Windows | `crow-tunnel.ps1`     | `.\crow-tunnel.ps1 up`                    |
| Git Bash / WSL / Mac | `crow-tunnel.sh`      | `bash crow-tunnel.sh up`                  |

Both have the same six commands:

| Command   | What it does                                                         |
| --------- | -------------------------------------------------------------------- |
| `up`      | Start tunnels **8000, 8080, 5432** as a background process          |
| `down`    | Stop those tunnels cleanly                                           |
| `status`  | Show whether ssh is running and which ports are listening            |
| `open`    | Open http://localhost:8000 in your default browser                   |
| `urls`    | Print every URL reachable from your main laptop                      |
| `help`    | Show the help page                                                   |

### Typical workflow

```powershell
PS C:\Users\zeind\crow_project\scripts\tunnel> .\crow-tunnel.ps1 up
==[ Bringing up SSH tunnel to dockerlaptop (100.65.93.24) ]==

==[ Tunnel UP (ssh PID 12345) ]==

PS> .\crow-tunnel.ps1 status
==[ Tunnel status ]==
tracked PIDs       : 12345
ssh.exe running    : 12345
  :8000  Django gateway         LISTENING (tunnel up)
  :8080  Crow C++ API           LISTENING (tunnel up)
  :5432  PostgreSQL             LISTENING (tunnel up)

PS> .\crow-tunnel.ps1 open   # opens Django in your browser

PS> .\crow-tunnel.ps1 down
==[ Tearing down SSH tunnel ]==
==[ Tunnel DOWN ]==
```

Equivalent in Git Bash:

```bash
$ bash crow-tunnel.sh up
$ bash crow-tunnel.sh status
$ bash crow-tunnel.sh open
$ bash crow-tunnel.sh down
```

---

## What "the docker laptop" is

It's a separate Windows machine on your Tailscale-style mesh network
(routed `100.65.x.x`). It runs `docker compose` for the Traffic
Management System. The README at `crow_project/README.md` has the full
project writeup.

You usually interact with it in two ways:

1. **Through the tunnels** (this script): as if the stack were local.
2. **Direct shell**: `ssh dockerlaptop` — for inspecting containers,
   viewing logs, restarting services, etc.

### Useful docker commands via SSH

```bash
ssh dockerlaptop 'cd crow_project && docker compose ps'
ssh dockerlaptop 'cd crow_project && docker compose logs --tail=50 django'
ssh dockerlaptop 'cd crow_project && docker compose restart django'
```

---

## Postman / API access

The Postman collection in `crow_project/postman/` points at
`http://localhost:8000`. Once you've run `crow-tunnel up`, Postman just
works — fill in the `django_api_key` from the live `.env` on the docker
laptop (it's regenerated locally each run; `Get-Content .env` over SSH
if you need it):

```bash
ssh dockerlaptop 'cat crow_project/.env | findstr DJANGO_API_KEY'
```

---

## Troubleshooting

**First-time PowerShell only** — if you get *"running scripts is disabled on this
system"* when you run `.\\crow-tunnel.ps1`, run this once:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

It allows local unsigned scripts (like this one) and remote signed ones. It is
per-user and reversible.

**If group policy blocks that** — use the bypass flag once per invocation, no policy
change required:

```powershell
powershell -ExecutionPolicy Bypass -File .\crow-tunnel.ps1 up
powershell -ExecutionPolicy Bypass -File .\crow-tunnel.ps1 down
```

The bypass is one-shot; nothing on the machine is permanently changed.

| Symptom                                          | Likely cause / fix                                                                 |
| ------------------------------------------------ | ---------------------------------------------------------------------------------- |
| `ssh dockerlaptop` says "Permission denied"      | Public key on docker laptop not authorized, or key perms too open                   |
| `up` says "Address already in use"              | Old tunnel didn't get killed — run `crow-tunnel down` then `up` again              |
| `status` shows ports not listening               | Tunnel down. Run `up` again, or check `ssh dockerlaptop 'docker compose ps'`      |
| `status` says ssh NOT running                    | Tunnel crashed. Run `up` again. PIDs are tracked in `~/.ssh/crow-tunnel.pid`       |
| Browser sticks on old page                       | Hard refresh (ctrl-F5). The Django UI renders Bootstrap in the browser             |
| Can hit `/api/...` but get 403                   | Need `X-API-Key` header. See "Postman / API access" above                          |

**Find tunnel PIDs manually:**

- PowerShell: `Get-Process ssh`
- Git Bash:  `pgrep -af ssh`

**Kill tunnel PIDs manually (last resort):**

- PowerShell: `Get-Process ssh | Stop-Process -Force`
- Git Bash:  `pkill -f 'ssh.*-N.*100.65.93.24'`

---

## Files in this folder

| File                  | Purpose                                                |
| --------------------- | ------------------------------------------------------ |
| `crow-tunnel.ps1`     | PowerShell tunnel manager (Windows / native)           |
| `crow-tunnel.sh`      | Bash tunnel manager (Git Bash / WSL / Mac)             |
| `SSH-CHEATSHEET.md`   | Plain-English SSH commands you might find handy        |
| `README.md`           | This file                                              |
