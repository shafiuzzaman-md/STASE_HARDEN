import json
import re
import os
import glob

def extract_data_from_content(content):
    # Regex to extract the filename, line number, and error type directly from the postconditions
    file_line_pattern = r"KLEE: ERROR: (.+?):(\d+): ASSERTION FAIL: (.+)"
    match = re.search(file_line_pattern, content)
    if match:
        path, line, assertion_fail = match.groups()
        # Extract just the filename from the path
        file = os.path.basename(path)
    else:
        file, line, assertion_fail = '', '', ''

    # Initialize a set to capture variables, ensuring uniqueness
    variables_set = set()

    # Regex to capture standard variable declarations
    standard_var_pattern = r"(\w+) : \w+ = symbolic"
    variables_set.update(re.findall(standard_var_pattern, content))

    # Regex to specifically capture array declarations, including those with * or -> notations
    array_var_pattern = r"array ([\w\*]+(?:->\w+)?(?:\[\d+\])?) : \w+ -> \w+ = symbolic"
    array_variables = re.findall(array_var_pattern, content)
    for array_var in array_variables:
        variables_set.add(f"array {array_var}")

    # Ensure all captured variables are formatted as a list for JSON output
    formatted_variables = list(variables_set)

    output = {
        "type": assertion_fail.strip(),
        "file": file,
        "line": int(line) if line.isdigit() else 0,
        "variables": formatted_variables
    }
    return json.dumps(output, indent=4)

def convert_files(input_folder, output_folder):
    # Check if the output folder exists, create if not
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # List all files in the input directory
    input_files = glob.glob(os.path.join(input_folder, '*'))

    # Process each file
    for input_file in input_files:
        with open(input_file, 'r') as file:
            content = file.read()

        # Extract the data and convert to JSON
        converted_data = extract_data_from_content(content)
        
        # Determine the output file name
        output_file = os.path.join(output_folder, os.path.basename(input_file).replace('.txt', '.json'))
        
        # Write the JSON data to the output file
        with open(output_file, 'w') as file:
            file.write(converted_data)
        print(f'Formatted output saved to: {output_file}')

# Example usage
input_folder = 'stase_output'
output_folder = 'formatted_output'
convert_files(input_folder, output_folder)
