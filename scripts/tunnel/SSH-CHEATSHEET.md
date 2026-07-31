# SSH Cheatsheet (just what you need)

You don't need to become an SSH expert. Here's a tiny subset that covers
your crow project setup. Skim it once and you'll be fine.

---

## The single most useful command

```bash
ssh dockerlaptop
```

That's it. Type `ssh dockerlaptop`, hit enter, and you land in a shell
on the docker laptop. Type `exit` (or ctrl-D) and you're back.

The word `dockerlaptop` is just an **alias** defined in your SSH
config. Without that alias you'd have to remember the raw IP
`100.65.93.24` and the username `me`, like this:

```bash
ssh me@100.65.93.24
```

The alias saves your fingers and your memory.

---

## "I want to run one quick command there"

You can pass a command after the address:

```bash
ssh dockerlaptop 'docker ps'
ssh dockerlaptop 'cd crow_project && docker compose logs --tail=30 django'
ssh dockerlaptop 'cat .env | grep DJANGO_API_KEY'
```

Quotes are important — they keep your local shell from interpreting the
remote command.

## "I want my browser to see a port that's only open on dockerlaptop"

That's what **SSH port forwarding** is. The `crow-tunnel` script
already does it for you (8000, 8080, 5432). If you ever need to forward
some other port manually:

```bash
ssh -N -L 9000:localhost:9000 dockerlaptop
```

This means "open a tunnel: when something on my laptop connects to
localhost:9000, forward it to localhost:9000 on dockerlaptop". `-N`
means "don't open a remote shell — just keep the tunnel alive".

`-N` plus a local `ctrl-C` stops it. The `crow-tunnel` script uses
`-N` and saves the PID so it can cleanly stop the tunnel without
killing any other SSH sessions.

## "Why does it say `IdentitiesOnly yes`?"

It tells ssh "use ONLY the key in the next line, don't try others".
Without it, ssh may try every key in `~/.ssh/` and pick the wrong one,
which leads to "Permission denied" confusion.

## "What's an SSH key?"

A key file pair:

- **Private key** (`id_ed25519_dockerlaptop`) — stays on **your** laptop.
  Treat it like a password. Never share.
- **Public key** (`id_ed25519_dockerlaptop.pub`) — copied to the docker
  laptop's `~/.ssh/authorized_keys`. Safe to share.

When you ssh, the math proves you have the private key without
sending the key itself. That's why you don't type a password.

## "What if my key perms are too open?"

Some SSH servers refuse keys with too-permissive Unix permissions
(must be `chmod 600`). Windows OpenSSH servers don't always enforce
this, so you may have gotten away with `644`. To be safe:

```bash
chmod 600 ~/.ssh/id_ed25519_dockerlaptop
```

If you're on Windows with Git Bash and the chmod doesn't show effect
(Windows NTFS has its own permission model), don't worry — Windows
OpenSSH ignores the chmod for the most part and it should still work.

## "What if I get 'host key verification failed'?"

Either you really are connecting to a different machine, or someone
reinstalled the docker laptop and the key changed. To accept the new
key (only do this if you trust the machine!):

```bash
ssh -o StrictHostKeyChecking=accept-new dockerlaptop
```

To remove the old key entirely:

```bash
ssh-keygen -R 100.65.93.24
```

## "Common SSH troubleshooting checklist"

1. Can you reach the network? You ARE on the network — your IP is in
   the same `100.65.x.x` range, so yes.
2. Is the SSH server running on dockerlaptop? It is, by config.
3. Does your public key match what's in `~/.ssh/authorized_keys` on
   dockerlaptop? Test with `ssh dockerlaptop 'echo ok'`.
4. If you see "Permission denied (publickey)": your key isn't
   authorized. The maintainer of the docker laptop would need to add
   `id_ed25519_dockerlaptop.pub` to their `authorized_keys`.
5. If you see "Connection timed out": network is down or the remote
   machine is off.
6. If you see "Connection refused": SSH port (22) is closed on the
   remote.

## "Why don't I just put the stack on a public server?"

Because this is a Programming Clinic submission project that runs the
full stack locally for grading. The Tailscale-style mesh network is
the simplest way to share it between two laptops without deploying to
the public internet.

---

## Quick reference card

```
# Drop into docker laptop
ssh dockerlaptop

# Run one command there
ssh dockerlaptop '<command>'

# Tunnel ports back to my laptop
.\crow-tunnel.ps1 up          # PowerShell
bash crow-tunnel.sh up        # Bash

# Tear them down
.\crow-tunnel.ps1 down

# What's connected?
.\crow-tunnel.ps1 status

# Open the UI
.\crow-tunnel.ps1 open
```

That's the whole toolkit.
