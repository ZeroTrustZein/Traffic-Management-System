#!/usr/bin/env bash
# crow-tunnel.sh — thin wrapper around the generic tunnel tool, pinned to
# the "crow" project. Full command reference: ~/devtools/bin/tunnel.sh help
# Examples:
#   bash crow-tunnel.sh up          bring the tunnel up
#   bash crow-tunnel.sh status      show listening ports
#   bash crow-tunnel.sh open        open Django UI in browser
#   bash crow-tunnel.sh down --all  tear down (also kill other ssh -N to this remote)
exec "$HOME/devtools/bin/tunnel.sh" "${1:-help}" crow "${2:-}"
