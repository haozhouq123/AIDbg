# Deobfuscation Prompt

## Task
Identify obfuscation techniques and provide a deobfuscated version.

## Obfuscation categories
- VM-based protectors (VMProtect, Themida, etc.)
- Junk code insertion
- Opaque predicates
- Anti-debugging / anti-VM checks
- Control flow flattening
- String encryption
- Self-modifying code
- Indirect calls / imports hiding

## Output format
1. **Obfuscation detected**: list each technique found
2. **Affected addresses**: 0x...
3. **Deobfuscated logic**: plain description of what the code actually does
4. **Recommended action**: manual / automated deobfuscation steps

## Input

{{CONTEXT}}
