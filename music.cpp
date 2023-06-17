#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <cstdint>
#include <deque>

#define S_DIVISIONS "divisions"
#define S_BEAT_TYPE "beat-type"
#define S_FIFTHS "fifths"
#define S_MODE "mode"
#define S_BEATS "beats"

#define S_CLOSE_ATTR "/attributes"

#define S_PART "part"
#define S_ID "id"
#define S_MEASURE "measure"
#define S_NUMBER "number"
#define S_NOTE "note"
#define S_PITCH "pitch"
#define S_STEP "step"
#define S_OCTAVE "octave"
#define S_ALTER "alter"
#define S_DURATION "duration"
#define S_VOICE "voice"
#define S_REST "rest"

#define S_CLOSE_NOTE "/note"
#define S_CLOSE_MEASURE "/measure"
#define S_CLOSE_PART "/part"

/**
 * This small C++ program converts MusicXMLs into readable formats for dataset processing.
 * That is, a piece is written in "inline" form.
 *
 * This program assumes that the rhythms and notes of the piece inputted are "not complex."
 * That is, they are in chorale form with normal counterpoint rhythmic rules (nothing
 * beyond the rhythmic constraints of fourth species). The program has been tested
 * on more complex works but should not be used on it.
 *
 * This program does NOT accurately translate the piece. That is, dots, slurs, ties, etc. are NOT
 * preserved. For the same of processing to the LLM, dotted notes are "repeated."
 * This is, after all, what we do mentally.
 *
 * There are no edge cases with this program. We assume that the input to the program
 * is a properly formatted MusicXML file with all of the appropriate parameters to give it
 * a key signature, time signature, etc.
 *
 * The format of the inline-music representation looks like this to handle multiple voices:
 * M: 4/4
 * K: F
 * M0: [F3A3C4F4F4] |
 * M1: [E3G3C4C5(G4,C4)] [F3A3C4A4(F4,F3)] [D3A3D4F4(A3,F3)] [A2A3F4C5(A3,C4)] |
 *
 * where each [] indicates a beat. Each note is a voice in the chord. Parentheses indicate subdivisions of the beat
 * i.e. notes with a shorter duration. This program only accepts pieces in time easily divisible by 2
 * (no durations with multiples of 3, for example, unfortunately).
 *
 * The command line is in the form ./a.out {filename} {flag}. If no flag is specified then the ML option is turned off.
 * If it is any other value it will turn on the ML option.
 */

typedef enum {
    TAG,
    BEATS, // # of Beats in a measure
    BEAT_TYPE, // Subdivisions
    DIVISIONS, // # Divisions Per "Duration"
    KEY_T, // KEY TONIC
    KEY_M // KEY MODALITY
} init_state;

typedef enum {
    PART, // Detect part
    PART_ID,
    MEASURE, // If <measure> tag is reached
    MEASURE_NUM, // Retrieving measure number
    NOTE, // When <note> tag is reached
    PITCH, // When <Pitch> tag is reached
    STEP, // When <step> tag is reached (indicates actual pitch)
    ALTER, // Indicates accidental.
    OCTAVE, // When <octave> tag is reached.
    DURATION, // Indicates length of note (use division by init_params)
    VOICE // Indicates what voice this is in if the voice itself is polyphonic.
} note_state;

typedef struct __initparams__ {
    uint16_t beats; // # Beats in a Measure
    uint16_t beat_type; // Beat Subdivision
    uint8_t division_count; // Division Count
    int8_t key_center; // Key Center, represented by # Fifths
    bool major; // Modality (Major = True, Minor = False)
} init_params;

typedef struct __note__ {
    uint8_t octave;
    uint8_t duration;
    uint8_t part;
    uint8_t voice;
    int8_t alter;
    char pitch;
} note;

typedef struct __notegroup__ {
    std::deque<note> voices; // Notes that are part of the chord. Can include passing tones.
    uint16_t duration; // The length of the chord. Should be a multiple of 2 but can be a multiple of 3.
    uint8_t part; // The part which this note group is in.
} chord;

typedef struct __measure__ {
    std::deque<chord> beat_content; // Contains the content for each beat.
    uint32_t measure_num; // measure number. This is used to index into measure list.
} measure;

// Global headers.
bool ml_flag = true;

std::vector<init_params> part_params;
std::deque<measure> measure_list;
std::map<int8_t, std::string> major_map = {
        {0, "C"}, {1, "G"}, {2, "D"}, {3, "A"}, {4, "E"}, {5, "B"}, {6, "F#"},
        {-1, "F"}, {-2, "B♭"}, {-3, "E♭"}, {-4, "A♭"}, {-5, "D♭"}, {-6, "G♭"}
};
std::map<int8_t, std::string> minor_map = {
        {0, "a"}, {1, "e"}, {2, "b"}, {3, "f#"}, {4, "c#"}, {5, "g#"}, {6, "d#"},
        {-1, "d"}, {-2, "g"}, {-3, "c"}, {-4, "f"}, {-5, "b♭"}, {-6, "e♭"}
};
std::map<int8_t, std::string> accidental_map = {
        {0, "♮"}, {1, "#"}, {-1, "♭"}, {2, "x"}, {-2, "♭♭"}
};

int init_parse(std::string* input);
int note_parse(std::string* input);

void display_part(init_params p);
void display_note(note nt);
void display_chord(chord notes);
void display_measure(measure msur);

uint16_t nearest_bin_power(uint16_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n = (++n) >> 1;

    return n;
}

int init_parse(std::string* input) {
    init_state state = TAG;
    init_params params;
    
    // Defaults
    params.beats = 4;
    params.beat_type = 4;
    params.key_center = 'C';
    params.major = 0;

    char* input_str = (char*) input->c_str();
    std::string tag_str;
    std::string value_str;

    bool tag_active = false;
    bool value_active = false;
    char c;
    while ((c = *input_str++) != '\0') {
        if (tag_active) tag_str.append(1, c);
        if (value_active) value_str.append(1, c);

        if (c == '<') {
            switch (state) {
                case BEATS:
                    params.beats = (uint16_t) std::stoi(value_str);
                    break;
                case BEAT_TYPE:
                    params.beat_type = (uint16_t) std::stoi(value_str);
                    break;
                case DIVISIONS:
                    params.division_count = (uint8_t) std::stoi(value_str);
                    break;
                case KEY_T:
                    params.key_center = (int8_t) std::stoi(value_str);
                    break;
                case KEY_M:
                    params.major = (value_str.compare("major") == 0);
                    break;
                default:
                    break;
            }

            state = TAG;
            tag_active = true;

            value_str.clear();
            value_active = false;
        }

        if (c == '>') {
            tag_str.erase(std::remove_if(tag_str.begin(), tag_str.end(), isspace), tag_str.end());
            tag_str.pop_back();

            if (tag_str.compare(S_DIVISIONS) == 0) {
                state = DIVISIONS;
                value_active = true;
            } else if (tag_str.compare(S_FIFTHS) == 0) {
                state = KEY_T;
                value_active = true;
            } else if (tag_str.compare(S_MODE) == 0) {
                state = KEY_M;
                value_active = true;
            } else if (tag_str.compare(S_BEATS) == 0) {
                state = BEATS;
                value_active = true;
            } else if (tag_str.compare(S_BEAT_TYPE) == 0) {
                state = BEAT_TYPE;
                value_active = true;
            } else if (tag_str.compare(S_CLOSE_ATTR) == 0) {
                part_params.push_back(params);
            }

            tag_str.clear();
            tag_active = false;
        }
    }

    return 0;
}

int note_parse(std::string* input) {
    note_state state = PART;

    char* input_str = (char*) input->c_str();
    std::string tag_str;
    std::string value_str;

    measure measure_obj;
    measure_obj.measure_num = 0;

    chord notes;
    notes.duration = 0;
    notes.part = 0;

    note note_obj;
    note_obj.alter = INT8_MIN;
    note_obj.part = 0;
    note_obj.octave = 0;

    uint16_t div_count = 0;

    bool tag_active = false;
    bool value_active = false;
    char c;
    while ((c = *input_str++) != '\0') {
        if (c == '<') {
            switch (state) {
                case PART:
                case PART_ID:
                case MEASURE:
                case MEASURE_NUM:
                case NOTE:
                case PITCH:
                    tag_active = true;
                    continue;
                case DURATION:
                    note_obj.duration = (uint8_t) std::stoi(value_str);
                    state = PITCH;
                    break;
                case ALTER:
                    note_obj.alter = (int8_t) std::stoi(value_str);
                    state = PITCH;
                    break;
                case OCTAVE:
                    note_obj.octave = (uint8_t) std::stoi(value_str);
                    state = PITCH;
                    break;
                case STEP:
                    note_obj.pitch = value_str.c_str()[0];
                    state = PITCH;
                    break;
                case VOICE:
                    note_obj.voice = (uint8_t) std::stoi(value_str);
                    state = PITCH;
                    break;
            }

            value_str.clear();
            value_active = false;
        }

        if (tag_str.compare(S_MEASURE) == 0 && state == MEASURE) {
            state = MEASURE_NUM;
            tag_str.clear();
        }

        if (tag_str.compare(S_PART) == 0 && isspace(c)) {
            state = PART_ID;
            tag_str.clear();
        }

        if (c == '=') {
            tag_str.erase(std::remove_if(tag_str.begin(), tag_str.end(), isspace), tag_str.end());
            if ((state == MEASURE_NUM && tag_str.compare(S_NUMBER) == 0) || (state == PART_ID && tag_str.compare(S_ID) == 0)) {
                tag_str.clear();
                tag_active = false;

                // Hacking to find the value inside the quotes.
                while (c == '"' || isspace(c = *input_str++));
                while ((c = *input_str++) != '"') value_str.append(1, c);
                if (state == MEASURE_NUM) {
                    measure_obj.measure_num = (uint32_t) std::stoi(value_str);
                } else if (state == PART_ID) {
                    if (value_str.size() >= 2) value_str = value_str.substr(1, value_str.size() - 1);
                    note_obj.part = (uint8_t) std::stoi(value_str) - 1;
                    state = MEASURE;
                }
                value_str.clear();

                while ((c = *input_str++) != '>');
            }
        }

        if (c == '>') {
            tag_str.erase(std::remove_if(tag_str.begin(), tag_str.end(), isspace), tag_str.end());

            if (tag_str.find(S_NOTE) != std::string::npos) {
                if (tag_str.compare(S_CLOSE_NOTE) == 0) {
                    // Add note object to the note group
                    div_count += note_obj.duration;

                    notes.part = note_obj.part;
                    notes.voices.push_back(note_obj);

                    if (div_count >= part_params[note_obj.part].division_count) {
                        notes.duration = div_count;
                        measure_obj.beat_content.push_back(notes);
                        notes.voices.clear();
                        div_count %= part_params[note_obj.part].division_count;
                    }

                    // Reset note_obj.
                    note_obj.alter = INT8_MIN;
                }
                state = NOTE;
            } else if (tag_str.compare(S_PITCH) == 0) {
                state = PITCH;
            } else if (state == PITCH && tag_str.compare(S_DURATION) == 0) {
                value_active = true;
                state = DURATION;
            } else if (state == PITCH && tag_str.compare(S_OCTAVE) == 0) {
                value_active = true;
                state = OCTAVE;
            } else if (state == PITCH && tag_str.compare(S_STEP) == 0) {
                value_active = true;
                state = STEP;
            } else if (state == PITCH && tag_str.compare(S_ALTER) == 0) {
                value_active = true;
                state = ALTER;
            } else if (tag_str.compare(S_VOICE) == 0) {
                value_active = true;
                state = VOICE;
            } else if (tag_str.find(S_REST) != std::string::npos) {
                state = PITCH;
                note_obj.pitch = 'R';
                note_obj.octave = 0;
            } else if (tag_str.compare(S_CLOSE_MEASURE) == 0) {
                // Add the measure to the measure list.
                std::deque<measure>::iterator iter = measure_list.begin();
                for (; iter < measure_list.end(); iter++) {
                    if (iter->measure_num == measure_obj.measure_num) {
                        break;
                    }
                }
                measure_list.insert(iter, measure_obj);

                notes.voices.clear();
                measure_obj.beat_content.clear();
                state = MEASURE;
            } else if (tag_str.compare(S_CLOSE_PART) == 0) {
                state = PART;
                div_count = 0;
                measure_obj.measure_num = 0;
                note_obj.alter = INT8_MIN;
            }

            tag_str.clear();
            tag_active = false;
            continue;
        }

        if (tag_active) tag_str.append(1, c);
        if (value_active) value_str.append(1, c);
    }
    return 0;
}

void display_part(init_params p) {
    printf("M: %d/%d\nK: %s\n", p.beats, p.beat_type, (p.major == 0 ? major_map[p.key_center].c_str() : minor_map[p.key_center].c_str()));
}

int display() {
    // Displaying the initial parameters.
    display_part(part_params[0]);

    // Displaying the measures themselves
    for (auto measure : measure_list) {
        display_measure(measure);
    }

    return 0;
}

void display_note(note nt) {
    if (!ml_flag) { // Display nicely.
        if (nt.alter == INT8_MIN) {
            printf("%c%d", nt.pitch, nt.octave);
        } else {
            printf("%c%s%d", nt.pitch, accidental_map[nt.alter].c_str(), nt.octave);
        }
    } else { // Tokenization form.
        if (nt.pitch == 'R') {
            printf("");
            return;
        }

        if (nt.alter == INT8_MIN) {
            printf("%c", nt.pitch);
        } else {
            printf("%c%s", nt.pitch, accidental_map[nt.alter].c_str());
        }
    }
}

void display_chord(chord notes) {
    uint8_t subdiv_count = 0;
    for (std::deque<note>::iterator iter = notes.voices.begin(); iter < notes.voices.end(); iter++) {
        uint8_t duration = iter->duration;
        uint8_t part_div_c = part_params[iter->part].division_count;
        if (duration < notes.duration) {
            std::deque<note>::iterator start_pos = iter;

            uint16_t max_sub_dur = 0;
            while (iter < notes.voices.end() && (subdiv_count += iter->duration) < notes.duration) {
                max_sub_dur = (iter->duration >= max_sub_dur) ? iter->duration : max_sub_dur;
                iter++;
            }

            chord temp;
            temp.voices.assign(start_pos, iter + 1);
            temp.duration = notes.duration / 2;

            if (iter != notes.voices.end()) {
                if (duration < part_div_c) printf("(");
                display_chord(temp);
                if (duration < part_div_c) printf(")");
                if ((iter + 1) != notes.voices.end()
                    && duration <= (iter + 1)->duration
                    && (iter + 1)->duration < part_div_c / 2) printf(",");
            }

            subdiv_count = 0;
        } else {
            display_note(*iter);
            if (iter->duration == notes.duration && iter->duration < part_div_c && iter != notes.voices.end() - 1) printf(",");
        }
    }
}

void display_measure(measure m) {
    printf("M%d: ", m.measure_num);
    for (auto crd : m.beat_content) {
        printf("[");
        display_chord(crd);
        printf("]");

        if (crd.duration > part_params[crd.part].division_count) printf("%d", crd.duration / part_params[crd.part].division_count);
        printf(" ");
    }
    printf("|\n");
}

void handle_dots(measure& m) {
    uint8_t div_count = 0;
    for (std::deque<chord>::iterator beat = m.beat_content.begin(); beat < m.beat_content.end(); beat++) {
        for (std::deque<note>::iterator iter = beat->voices.begin(); iter < beat->voices.end(); iter++) {
            if ((iter->duration + div_count) > part_params[beat->part].division_count && (iter->duration + div_count) % part_params[beat->part].division_count != 0) {
                note copy_note = {iter->octave,
                                  (uint8_t) (iter->duration + div_count - part_params[beat->part].division_count),
                                  iter->part,
                                  iter->voice,
                                  iter->alter,
                                  iter->pitch};

                iter->duration = part_params[beat->part].division_count - div_count;
                beat->duration = part_params[beat->part].division_count;
                if (copy_note.duration >= part_params[beat->part].division_count) {
                    chord temp;
                    temp.voices.push_back(copy_note);
                    temp.duration = copy_note.duration;
                    m.beat_content.insert(beat + 1, temp);
                } else {
                    std::deque<chord>::iterator next = std::next(beat, 1);
                    if (next != m.beat_content.end()) {
                        next->voices.push_front(copy_note);
                    } else {
                        beat->voices.push_front(copy_note);
                    }
                }
            } else if ((iter->duration + div_count) < part_params[beat->part].division_count && (iter->duration + div_count) % 2 == 1) {
                note copy_note = *iter;

                uint16_t n = nearest_bin_power(iter->duration);
                copy_note.duration = iter->duration - n;

                if (n != 0) {
                    iter->duration = n;
                    beat->voices.insert(iter + 1, copy_note);
                }
            }

            div_count += iter->duration;
            if (div_count >= part_params[beat->part].division_count) div_count %= part_params[beat->part].division_count;
        }
    }
}

int compare_notes(const note& n1, const note& n2) {
    if (n1.octave < n2.octave) {
        return -1;
    } else if (n1.octave > n2.octave) {
        return 1;
    } else {
        uint8_t p2 = n2.pitch < 'C' ? n2.pitch + 10 : n2.pitch;
        if (n1.pitch < p2) {
            return -1;
        }
        return (n1.pitch > p2);
    }
}

void merge_beat_lists(std::deque<chord>& concat_beat_content, std::deque<chord> partn) {
    if (concat_beat_content.size() == 0) {
        concat_beat_content.insert(concat_beat_content.end(), partn.begin(), partn.end());
        return;
    }

    // Begin merging sequentially.
    for (uint8_t i = 0, j = 0; i < concat_beat_content.size() && j < partn.size(); ) {
        if (concat_beat_content[i].duration == partn[j].duration) {
            if (compare_notes(concat_beat_content[i].voices[0], partn[j].voices[0]) < 0) {
                concat_beat_content[i].voices.insert(concat_beat_content[i].voices.end(), partn[j].voices.begin(), partn[j].voices.end());
            } else {
                concat_beat_content[i].voices.insert(concat_beat_content[i].voices.begin(), partn[j].voices.begin(), partn[j].voices.end());
            }

            i++;
            j++;
        } else if (concat_beat_content[i].duration > partn[j].duration) {
            uint8_t div_count = 0;
            while (j < partn.size() && (div_count += partn[j].duration) < concat_beat_content[i].duration) {
                chord temp;

                for (auto& nt : concat_beat_content[i].voices) {
                    nt.duration -= partn[j].duration;
                    nt.part = 0;
                }
                concat_beat_content[i].duration -= partn[j].duration;

                temp.voices.insert(temp.voices.end(), concat_beat_content[i].voices.begin(), concat_beat_content[i].voices.end());
                temp.duration = partn[j].duration;
                temp.part = 0;

                if (compare_notes(concat_beat_content[i].voices[0], partn[j].voices[0]) < 0) {
                    temp.voices.insert(temp.voices.end(), partn[j].voices.begin(), partn[j].voices.end());
                } else {
                    temp.voices.insert(temp.voices.begin(), partn[j].voices.begin(), partn[j].voices.end());
                }

                concat_beat_content.insert(concat_beat_content.begin() + i, temp);

                j++;
                i++;
            }
        } else {
            for (auto& nt : partn[j].voices) {
                nt.duration -= concat_beat_content[i].duration;
                nt.part = 0;
            }

            partn[j].duration -= concat_beat_content[i].duration;
            partn[j].part = 0;

            if (compare_notes(concat_beat_content[i].voices[0], partn[j].voices[0]) < 0) {
                concat_beat_content[i].voices.insert(concat_beat_content[i].voices.end(), partn[j].voices.begin(), partn[j].voices.end());
            } else {
                concat_beat_content[i].voices.insert(concat_beat_content[i].voices.begin(), partn[j].voices.begin(), partn[j].voices.end());
            }
            i++;
        }
    }
}

void merge_beats(measure& m) {
    std::vector<uint8_t> iter_locs;
    std::deque<chord> concat_beat_content;

    // Getting all locations of overlap.
    uint8_t part_num = part_params.size() - 1;
    uint8_t voice_num = 1;
    for (uint8_t i = 0; i < m.beat_content.size(); i++) {
        if (m.beat_content[i].part == part_num) {
            iter_locs.push_back(i);
            part_num--;
            voice_num++;
        } else if (m.beat_content[i].voices[0].voice == voice_num) {
            iter_locs.push_back(i);
            voice_num++;
        }
    }
    iter_locs.push_back(m.beat_content.size());

    // Merging all beat lists together.
    for (uint8_t i = 0; i < iter_locs.size() - 1; i++) {
        std::deque<chord> temp(m.beat_content.begin() + iter_locs[i], m.beat_content.begin() + iter_locs[i + 1]);
        merge_beat_lists(concat_beat_content, temp);
    }
    m.beat_content = concat_beat_content;
}

void merge_measures() {
    // Insert into measures based on lexicographical ordering.
    std::deque<measure> consolidated_list;
    measure temp_measure;
    temp_measure.measure_num = 0;

    // Couple all like measures together.
    for (auto& measure : measure_list) {
        handle_dots(measure);
        if (measure.measure_num != temp_measure.measure_num) {
            if (!temp_measure.beat_content.empty()) {
                consolidated_list.push_back(temp_measure);
            }

            temp_measure.measure_num = measure.measure_num;
            temp_measure.beat_content.clear();
        }
        temp_measure.beat_content.insert(temp_measure.beat_content.end(), measure.beat_content.begin(), measure.beat_content.end());
    }
    consolidated_list.push_back(temp_measure);

    // Standardizing durations
    uint8_t max_duration = 0;
    for (auto& param : part_params) {
        max_duration = (param.division_count >= max_duration) ? param.division_count : max_duration;
    }

    for (auto& measure : consolidated_list) {
        for (auto& chord : measure.beat_content) {
            uint8_t factor = max_duration / part_params[chord.part].division_count;
            for (auto& nt : chord.voices) {
                nt.duration *= factor;
            }
            chord.duration *= factor;
        }
    }

    for (auto& param : part_params) {
        param.division_count = max_duration;
    }

    // For each now concatenated measure, add together with respective beats.
    for (auto& measure : consolidated_list) {
        // Merge the beats together.
        merge_beats(measure);
    }

    // Replacing our old list with our new one!
    measure_list = consolidated_list;
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    if (argc == 2 || atoi(argv[2]) == 0) ml_flag = false;

    std::ifstream file(argv[1]);
    std::string* fil_str = new std::string();

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            fil_str->append(line.c_str());
        }
        file.close();
    } else {
        return 1;
    }

    if (init_parse(fil_str) == 0) {
        note_parse(fil_str);
        merge_measures();
    }

    delete fil_str;
    return display();
}
