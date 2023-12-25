#pragma once

#include <SdFat.h>
#include <cstdint>
#include <cstring>

/*template<typename T, typename This>
class Serializer {
public:
    // virtual static void* serialize(T& obj);
    // virtual static T deserialize(void* data);
    void* serialize(T& object) {
        return static_cast<This*>(this)->serialize(object);
    }

    T deserialize(void* data) {
        return static_cast<This*>(this)->deserialize(data);
    }
};

class BooleanSerializer : public Serializer<bool, BooleanSerializer> {
    friend class Serializer<bool, BooleanSerializer>;

    public:
    void* serialize(bool& obj) {
        return &obj;
    }

    bool deserialize(void* data) {
        return *static_cast<bool*>(data);
    }
};*/

inline SdFs* sd;

constexpr auto last_used_name = "_last_used";

template<typename T, typename Serializer> class DataCollection {
private:
    const char* dir_name;
    T* current;

#define PATH_JOIN(var, name) char var[strlen(dir_name) + strlen(name) + 2]; \
    strcpy(var, dir_name); \
    strcat(var, "/"); \
    strcat(var, name);

public:
    uint8_t save_preset(const char* name, T* preset) {
        PATH_JOIN(default_file_name, name)
        auto default_file = sd->open(default_file_name, O_RDWR | O_CREAT | O_TRUNC);
        if(!default_file) {
            return 1;
        }
        auto serialized = Serializer::serialize(preset);
        default_file.write(serialized, sizeof(T));
        default_file.close();
        free(serialized);
        return 0;
    }

    uint8_t load_preset(const char* name) {
        PATH_JOIN(path, name)

        auto file = sd->open(path, O_RDONLY);
        if(!file) {
            return 1;
        }

        auto len = file.size();
        auto bytes = malloc(len);
        auto res = file.read(bytes, len);
        if(res != len) {
            return 2;
        }
        file.close();

        current = Serializer::deserialize(bytes, len);
        free(bytes);

        // write last used
        PATH_JOIN(last_used_path, last_used_name)
        auto last_used_file = sd->open(last_used_path, O_RDWR | O_CREAT | O_TRUNC);
        if(!last_used_file) {
            return 3;
        }
        last_used_file.write(name, strlen(name) + 1 /* \0 */);
        last_used_file.close();

        return 0;
    }

    DataCollection(const char* dir_name, T* default_val) : dir_name(dir_name) {
        // check if directory doesn't exist
        if (!sd->exists(dir_name)) {
            // create directory
            sd->mkdir(dir_name);
            // create default file
            save_preset("default", default_val);
        }

        // try to load last used preset - name saved in file _last_used
        PATH_JOIN(last_used_path, last_used_name)
        auto last_used_file = sd->open(last_used_path, O_RDONLY);
        if(last_used_file) {
            auto len = last_used_file.size();
            auto bytes = malloc(len);
            auto res = last_used_file.read(bytes, len);
            if(res == len) {
                last_used_file.close();
                load_preset(static_cast<const char *>(bytes));
                free(bytes);
                return;
            }
        }

        load_preset("default");
    }
};

#define KEYS_ROWS 6
#define KEYS_COLS 9

constexpr auto note_layout_len = KEYS_ROWS * KEYS_COLS;

using NoteLayout = float*;

class NoteLayoutSerializer {
public:
    // serialize to a user-editable string that's just the numbers separated by commas
    static void* serialize(NoteLayout frequencies) {
        String str;
        for(uint8_t i = 0; i < note_layout_len; i++) {
            str += frequencies[i];
            if(i != note_layout_len - 1) {
                str += ",";
            }
        }

        const auto len = str.length() + 1;
        const auto bytes = malloc(len);
        str.toCharArray(static_cast<char *>(bytes), len);
        return bytes;
    }

    // deserialize from a string
    static float* deserialize(void* bytes, size_t _len) {
        const auto str = static_cast<char *>(bytes);
        const auto layout = new float[note_layout_len];
        const char* token = strtok(str, ",");
        for(auto i = 0; token != nullptr; i++) {
            layout[i] = strtof(token, nullptr);
            token = strtok(nullptr, ",");
        }
        return layout;
    }
};

inline DataCollection<NoteLayout, NoteLayoutSerializer> note_layouts("note_layouts", &(new float[note_layout_len]) /* TODO default */);
