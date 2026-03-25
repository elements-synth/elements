import os
import anthropic

KNOWN_ISSUES_PATH = "docs/known-issues.md"

# Read current known-issues.md
with open(KNOWN_ISSUES_PATH, "r") as f:
    current_docs = f.read()

# The features note content — update this manually when needed
NEXT_FEATURES = """
## Próximo release (v1.1)
- Preset save/load system
- Light source selection persistence
- Light on/off state persistence  
- Numeric display for Thickness and Deform sliders
- Geometric Deformer for Torus, Cube and Dodecahedron
- Windows VST3 build

## Features futuros (v1.2+)
- Karplus-Strong crystal resonance mode
- Expanded geometry library
- Nested materials
- Light polarization parameter
- More material types
"""

client = anthropic.Anthropic(api_key=os.environ["ANTHROPIC_API_KEY"])

prompt = f"""You are maintaining the documentation for Elements, a spectral wavetable synthesizer.

Here is the current known-issues.md page:
<current_docs>
{current_docs}
</current_docs>

Here is the current list of planned features:
<next_features>
{NEXT_FEATURES}
</next_features>

Your task:
1. Review the known-issues.md page
2. Check if the "planned features" section at the bottom is up to date with the next_features list
3. If there are differences, update the page to reflect the current planned features
4. If nothing has changed, return the document exactly as-is
5. Always maintain the existing page structure, formatting and tone
6. Return ONLY the full updated markdown content, no explanations or preamble

The page must always end with:
[← Back to Elements](index)
"""

message = client.messages.create(
    model="claude-sonnet-4-20250514",
    max_tokens=4096,
    messages=[{"role": "user", "content": prompt}]
)

updated_content = message.content[0].text

# Only write if content has changed
if updated_content.strip() != current_docs.strip():
    with open(KNOWN_ISSUES_PATH, "w") as f:
        f.write(updated_content)
    print("✅ Docs updated successfully")
else:
    print("✓ No changes needed")
