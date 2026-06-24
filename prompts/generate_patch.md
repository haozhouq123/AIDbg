# Generate Patch Prompt

## Task
Generate a byte-level patch for the specified address.

## Input

Target address: {{ADDRESS}}
Modification request: {{USER_REQUEST}}

Current bytes at the address:
{{CURRENT_BYTES}}

## Output format
1. **Original assembly**: <original instruction>
2. **Patched assembly**: <new instruction(s)>
3. **Original bytes** (hex): `XX XX XX ...`
4. **Patched bytes** (hex): `XX XX XX ...`
5. **XEDParse command**: `<asm snippet>` (e.g. `mov eax, 1; ret`)
6. **Explanation**: <why this patch achieves the requested modification>

## Constraints
- The patch must be at most as long as the original bytes (use NOP padding if shorter)
- Preserve the architecture (x86 vs x64)
- Don't change control flow unless explicitly requested
