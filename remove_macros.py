import re

def comment_out_static_assert(file_path):
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()

        # Comment out lines starting with STATIC_ASSERT
        for i, line in enumerate(lines):
            if re.match(r'\s*STATIC_ASSERT', line):
                lines[i] = f'// {line.strip()}\n'  # Comment out the line

        # Write the modified content back to the file
        with open(file_path, 'w') as file:
            file.writelines(lines)

        print(f"Lines starting with STATIC_ASSERT commented out in {file_path}")
    except Exception as e:
        print(f"An error occurred: {str(e)}")

if __name__ == "__main__":
    file_path = "eval2_edk2-main/MdePkg/Include/Base.h"

    comment_out_static_assert(file_path)