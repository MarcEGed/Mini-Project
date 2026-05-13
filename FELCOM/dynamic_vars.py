Import("env")
import random
import string
import subprocess

def get_git_revision_hash():
    try:
        return subprocess.check_output(['git', 'describe', '--tags', '--always', '--dirty']).decode('utf-8').strip()
    except Exception:
        return "unknown"

# Generate random name (A-Z) representing the node ID
node_char = random.choice(string.ascii_uppercase)
node_id = ord(node_char) # Pass as integer ASCII value to avoid shell quoting issues

version = get_git_revision_hash()

print(f"\n*** Auto-generating Build Info: VERSION={version}, NODE_ID='{node_char}' ***\n")

# Append as build flags (macros)
env.Append(CPPDEFINES=[
    ("NODE_ID", node_id),
    ("FELCOM_VERSION", f'\\"{version}\\"')
])
