#include "speak.h"
#include <festival.h>

void festival_initialize() {
    int heap_size = 210000;  // default scheme heap size
    int load_init_files = 1; // we want the festival init files loaded
    festival_initialize(load_init_files,heap_size);
	//festival_eval_command("(voice_rab_diphone)");
    //festival_eval_command("(voice_ked_diphone)");
	//To get the default voice back
}

void speak(char* utterance) {
    festival_say_text(utterance);
}
