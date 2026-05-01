Import("env")
import random
import string

# Generate random ID (1 to 255)
sender_id = random.randint(1, 255)
# Generate random name (A-Z)
node_char = random.choice(string.ascii_uppercase)
node_name = ord(node_char) # Pass as integer ASCII value to avoid shell quoting issues

print(f"\n*** Auto-generating Node Info: SENDER_ID={sender_id}, NODE_NAME='{node_char}' ***\n")

# Append as build flags (macros)
env.Append(CPPDEFINES=[
    ("SENDER_ID", sender_id),
    ("NODE_NAME", node_name)
])
