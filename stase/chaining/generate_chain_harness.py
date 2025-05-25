import json
import argparse
from pathlib import Path

TEMPLATE_HEADER = """\
#ifndef CHAIN_CFG_{name_upper}_H
#define CHAIN_CFG_{name_upper}_H

#include <stddef.h>
#include "chain_harness_template.h"
"""

TEMPLATE_POSTCHECK = """\
#define POST_CHECK(MEM, RANGE) \\
    do { \\
{asserts}
    } while (0)

#endif
"""

STEP_TEMPLATE = '    {{ STEP_{stype}, {from_idx}, {to_idx}, {fn} }},\n'

def generate_cfg_header(config_path, output_path):
    with open(config_path) as f:
        config = json.load(f)

    name = config['name']
    ranges = config['ranges']
    steps = config['steps']
    post_conditions = config['post_conditions']

    enum_defs = ", ".join([f"{r.upper()}_RANGE = {i}" for i, r in enumerate(ranges)])
    range_names = ', '.join([f'"{r}"' for r in ranges])

    out = TEMPLATE_HEADER.format(name_upper=name.upper())
    out += f"\n#define RANGE_CNT {len(ranges)}\n"
    out += f"#define RANGE_NAME(i) ((const char*[]){{ {range_names} }}[i])\n"
    out += f"enum {{ {enum_defs} }};\n\n"

    # Build step table
    out += "static const step_t STEP_TAB[] = {\n"
    for step in steps:
        if step['type'] == 'CALL':
            out += STEP_TEMPLATE.format(stype='CALL', from_idx=0, to_idx=0, fn=f"(void *){step['function']}")
        elif step['type'] == 'ASSUME_LINK':
            out += STEP_TEMPLATE.format(
                stype='ASSUME_LINK',
                from_idx=f"{step['from'].upper()}_RANGE",
                to_idx=f"{step['to'].upper()}_RANGE",
                fn="NULL"
            )
    out += "};\n"
    out += f"#define STEP_CNT (sizeof(STEP_TAB) / sizeof(STEP_TAB[0]))\n\n"

    # Postcondition
    assert_lines = []
    for p in post_conditions:
        line = f"        klee_assert({p['assert']});"
        assert_lines.append(line)
    joined_asserts = "\n".join(assert_lines)

    out += TEMPLATE_POSTCHECK.format(asserts=joined_asserts)

    output_file = Path(output_path) / f"chain_cfg_{name}.h"
    output_file.write_text(out)
    print(f"✔ Generated: {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", required=True, help="Path to chain_config.json")
    parser.add_argument("--out", required=True, help="Directory to write chain_cfg_<name>.h")
    args = parser.parse_args()

    Path(args.out).mkdir(parents=True, exist_ok=True)
    generate_cfg_header(args.config, args.out)
