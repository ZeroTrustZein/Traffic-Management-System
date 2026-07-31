<#
.SYNOPSIS
    Thin wrapper around the generic tunnel tool, pinned to the "crow" project.
.DESCRIPTION
    Full command reference: ~/devtools/bin/tunnel.ps1 help
.EXAMPLE
    PS> .\crow-tunnel.ps1 up          # bring the tunnel up
    PS> .\crow-tunnel.ps1 status      # check
    PS> .\crow-tunnel.ps1 open        # open Django UI in browser
    PS> .\crow-tunnel.ps1 down --all  # tear down (also kill other ssh.exe)
#>
param(
    [Parameter(Position=0)]
    [string]$Action = 'help',
    [Parameter(Position=1)]
    [string]$Extra = ''
)

& (Join-Path $env:USERPROFILE 'devtools\bin\tunnel.ps1') $Action crow $Extra
