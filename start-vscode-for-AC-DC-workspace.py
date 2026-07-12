#!/usr/bin/env python3
"""
Start VSCode for AC/DC workspace with window management using xdotool.

Install xdotool if not already present: `sudo apt install xdotool`
"""
import os
import sys
import subprocess
import time
import shutil
from screeninfo import get_monitors

version = 'v6'
CODE = '/usr/share/code/code'
CODE = 'code'  # If code is in PATH, otherwise specify the full path to the code executable
WORKSPACES = [f'AC/ser_{version}_AC.code-workspace', f'DC/ser_{version}_DC.code-workspace']


def open_console_with_device_listing():
    """Open a terminal, run device listing commands, and keep the terminal open."""
    terminal = shutil.which('x-terminal-emulator') or shutil.which('gnome-terminal') or shutil.which('konsole')
    if terminal is None:
        print('Warning: no supported terminal emulator found (x-terminal-emulator/gnome-terminal/konsole).')
        return

    # bash_cmd = (
    #     'ls -lisa /dev/ttyU* 2>/dev/null || true; '
    #     'ls -lisa /dev/esp* 2>/dev/null || true; '
    #     'ls -lisa /dev/ttyA* 2>/dev/null || true; '
    #     'echo; echo "Terminal remains open. Close it when done."; '
    #     'echo; echo "ll /dev/ttyU* ; ll /dev/esp* ; ll /dev/ttyA*"; '
    #     'exec bash'
    # )
    bash_cmd = (
        'ls -lisa /dev/ttyU* ; ls -lisa /dev/esp* ; ls -lisa /dev/ttyA* 2>/dev/null || true; '
        'echo; echo "ll /dev/ttyU* ; ll /dev/esp* ; ll /dev/ttyA*"; '
        'echo; echo "Terminal remains open. Close it when done."; '
        'exec bash'
    )

    # Start a separate terminal window and return immediately to Python.
    if os.path.basename(terminal) == 'gnome-terminal':
        subprocess.Popen([terminal, '--', 'bash', '-lc', bash_cmd])
    elif os.path.basename(terminal) == 'konsole':
        subprocess.Popen([terminal, '-e', 'bash', '-lc', bash_cmd])
    else:
        subprocess.Popen([terminal, '-e', 'bash', '-lc', bash_cmd])


def start_ac_dc(working_dir):
    """
    Start workspaces for code
    """
    instances = []
    for index, workspace in enumerate(WORKSPACES):
        workspace_path = os.path.join(working_dir, workspace)
        cwd = os.path.join(working_dir, workspace.rsplit('/', 1)[0])
        if os.path.exists(workspace_path):
            workspace_name = os.path.basename(workspace_path).replace('.code-workspace', '')
            print(f"\t- {workspace} in {cwd}")
            proc = subprocess.Popen([CODE, '--new-window', workspace_path], cwd=cwd)
            instances.append((workspace_name, proc))
        else:
            print(f"\t- Error: {workspace} not found in {cwd}")

    # # Wait for the windows to open
    # max_wait = 30  # seconds
    # p_wids = {}
    # for attempt in range(max_wait):
    #     for workspace_name, proc in instances:
    #         p_result = subprocess.run(['xdotool', 'search', '--name', workspace_name], capture_output=True, text=True)
    #         if p_result.returncode == 0 and p_result.stdout.strip():
    #             wids = p_result.stdout.strip().split('\n')
    #             p_wids[workspace_name] = wids[-1]
    #     if len(p_wids) == len(instances):
    #         print(f"After attempt {attempt}: found {len(p_wids)}/{len(instances)} workspace windows")
    #         break
    #     print(f"Attempt {attempt+1}: found {len(p_wids)}/{len(instances)} workspace windows")
    #     time.sleep(1)
    # else:
    #     print("Timed out waiting for VS Code windows to open.")
    #     for workspace_name in [name for name, _ in instances]:
    #         print(f"missing window for: {workspace_name}")

    # if len(p_wids) == len(instances):
    #     half_width = screen_width // len(instances) - 2
    #     for index, (workspace_name, proc) in enumerate(instances):
    #         p_wid = p_wids[workspace_name]
    #         x_position = monitor_x + (index * (half_width + 4))
    #         subprocess.run(['xdotool', 'windowsize', p_wid, str(half_width), str(screen_height)])
    #         subprocess.run(['xdotool', 'windowmove', p_wid, str(x_position), str(monitor_y)])
    # else:
    #     print("Could not find windows for the started programs.")
    #     for workspace_name, p_wid in p_wids.items():
    #         print(f"{workspace_name}: {p_wid}")

if __name__ == '__main__':
    print(f'Start multiple instances of code {sys.argv}:')
    # Get all monitors and select the one with the largest resolution
    monitors = get_monitors()
    monitor = max(monitors, key=lambda m: m.width * m.height)
    screen_width = monitor.width
    screen_height = monitor.height
    monitor_x = monitor.x
    monitor_y = monitor.y
    
    pwd = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()

    start_ac_dc(pwd)
    open_console_with_device_listing()
    #input('Finish with Enter key...')
