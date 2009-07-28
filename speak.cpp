/*Author: Chris Tralie
 *Project: Duke REU Fellowship 2009: Robotic navigation with RFID Waypoints
 *Purpose: To provide a command line wrapper to the Festival speech synthesis system*/


#include "speak.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void speak(char* utterance) {
	char* s1 = "echo \"";
	char* s3 = "\" | festival --tts";
	char* command = (char*)calloc(strlen(s1) + strlen(utterance) + strlen(s3) + 1, sizeof(char));
	strcpy(command, s1);
	strcat(command, utterance);
	strcat(command, s3);
	system(command);
	printf("%s\n", command);
	free(command);
}
