Import("env")
import random
import string
import subprocess

def get_git_revision_hash():
    try:
        return subprocess.check_output(['git', 'describe', '--tags', '--always', '--dirty']).decode('utf-8').strip()
    except Exception:
        return "unknown"

# Generate random ID (1 to 255)
sender_id = random.randint(1, 255)
# Generate random name (A-Z)
node_char = random.choice(string.ascii_uppercase)
node_name = ord(node_char) # Pass as integer ASCII value to avoid shell quoting issues

version = get_git_revision_hash()

print(f"\n*** Auto-generating Build Info: VERSION={version}, SENDER_ID={sender_id}, NODE_NAME='{node_char}' ***\n")

# Append as build flags (macros)
env.Append(CPPDEFINES=[
    ("SENDER_ID", sender_id),
    ("NODE_NAME", node_name),
    ("FELCOM_VERSION", f'\\"{version}\\"')
])
