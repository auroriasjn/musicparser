import os
import sys

def extract_letters_between_parentheses(string):
    indices = []
    start = 0

    while True:
        opening = string.find("(", start)
        if opening == -1:
            break

        closing = string.find(")", opening + 1)
        if closing == -1:
            break

        letters = string[opening + 1:closing].strip()
        if len(letters) == 1 or len(letters) == 2:
            indices.append((opening, closing))

        start = closing + 1

    return indices

def note_spelling(key, is_major):
    sharp_arr = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    flat_arr = ["C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B", "C♭"]

    # Setting whether to use flat or sharp notes with respect to the key
    note_arr = sharp_arr
    if len(key) > 1 and key[1] == '♭':
        note_arr = flat_arr
    elif len(key) == 1 and is_major and key[0] == 'F':
        note_arr = flat_arr
    elif not is_major and len(key) == 1:
        sharp_min_keys = ['E', 'A', 'B']
        if key[0].upper() not in sharp_min_keys:
            note_arr = flat_arr

    # Find the index of the key.
    key_loc = note_arr.index(key)
    note_arr = note_arr[key_loc:] + note_arr[:key_loc]

    return note_arr

def create_key_map(orig_key_arr, new_key_arr):
    orig, new = orig_key_arr.copy(), new_key_arr.copy()

    if len(orig) > len(new):
        b_loc = [val.upper() for val in orig].index("B")
        new.insert(b_loc, new[b_loc % len(new)])
    elif len(orig) < len(new):
        b_loc = [val.upper() for val in new].index("B")
        orig.insert(b_loc, orig[b_loc % len(orig)])

    key_dict = dict(zip(orig, new))
    return key_dict

def transpose_roman_nums(indices, key_map, roman_nums):
    offset = 0
    for index_pair in indices:
        start_index = index_pair[0] + 1 + offset
        end_index = index_pair[1] + offset

        inner_string = roman_nums[start_index:end_index]
        new_str = key_map[inner_string.upper()]
        if inner_string[0].islower():
            new_str = new_str.lower()

        if len(new_str) != len(inner_string):
            offset += (len(new_str) - len(inner_string))

        roman_nums = roman_nums[:start_index - 1] + roman_nums[start_index - 1:].replace(inner_string, new_str, 1)

    return roman_nums

if __name__ == '__main__':
    roman_nums = ''
    orig_key = "A"
    is_major = True

    input_dir = 'flattened_outputs'
    output_dir = 'outputs'

    # Getting the initial file to open
    fil = sys.argv[1] + '.txt'
    with open(os.path.join(input_dir, fil), 'r') as infile:
        copy = False
        for line in infile:
            if line.strip() == '---':
                copy = True
                continue
            elif copy:
                roman_nums += line
            # Getting the key
            elif line.strip()[0] == 'K':
                orig_key = line[3:].strip()
                # Obtaining tonality
                if orig_key[0].islower():
                    is_major = False

                orig_key = orig_key.upper()
                continue

    print("ORIGINAL:")
    print(roman_nums)

    # Getting the original key scale
    orig_key_arr = note_spelling(orig_key, is_major)

    # Extracting the indices of the key changes
    indices = extract_letters_between_parentheses(roman_nums)

    # Stripping folders that do not have to do with the tonality
    folders = os.listdir(output_dir)

    folders = [folder for folder in folders if
               ((folder.endswith("maj") and is_major)
                or (folder.endswith("min") and not is_major))]

    # Iterating through the output directory
    for folder in folders:
        curr_path = os.path.join(output_dir, folder, fil)

        # Getting the key of the current name
        key_str = folder[:-4]
        key = []

        if is_major:
            key.append(key_str[0].upper())
        else:
            key.append(key_str[0].lower())

        if key_str.endswith("flat"):
            key.append("♭")
        elif key_str.endswith("sharp"):
            key.append("#")

        key = "".join(key)

        # Altering the string
        note_arr = note_spelling(key.upper(), is_major)
        scale_mapping = create_key_map(orig_key_arr, note_arr)
        new_roman_nums = transpose_roman_nums(indices, scale_mapping, roman_nums)

        '''
        with open(curr_path, 'a') as outfile:
            outfile.write(new_roman_nums)
        '''

