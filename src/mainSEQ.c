#include "crackme.h"
#include <stdlib.h>
#include <stdio.h>

#define ASCII_START 0 // Don't touch.
#define ASCII_END 127 // Don't touch.

// Increases the numeric value for char* from right to left.
int right_to_left(char *guess, int size)
{
    // For loop for incrementing char value in the string guess from right to left
    for (int i = size - 1; i >= 0; i--)
    {
        // If the guess numeric value is below the ascii end we increment the char value
        if (guess[i] < ASCII_END)
        {
            // printf(" IF nr: %d char: %c\n", guess[i], guess[i]);
            guess[i]++;

            return 1;
        }
        // Else we reached the last char value we reset to start.
        // At this point we tried all numeric posibilities (ASCII START TO ASCII END).
        else
        {
            // printf(" ELSE nr: %d char: %c\n", guess[i], guess[i]);

            // Setting the position of the char to the start.
            guess[i] = ASCII_START;
        }
    }

    // End here when all posibilties tried.
    return 0;
}

int main(int argc, char **argv)
{
    // Take size as argument
    if (argc != 2)
    {
        fprintf(stderr, "Wrong usage: %s <size of password to generate>\n", argv[0]);
        return 1;
    }
    // Turn ASCII to integer.
    // Size of the string.
    // Can be >=1 and <=4 for this seq. prg. above 4 will take hours
    int size = atoi(argv[1]);

    // Allocating memory for my text string guess.
    char *guess = (char *)malloc(sizeof(char) * size + 1);
    if (guess == NULL)
    {
        fprintf(stderr, "No memory allocated for guess\n");
        return 1;
    }
    // Null terminate string.
    guess[size] = '\0';

    // Set guess to have a start characters. In this case it's ASCII start numeric value
    for (int i = 0; i < size; i++)
        guess[i] = ASCII_START;

    // // Variable for nth itera.
    // int nth = 1000000;

    int hit = 0; // Don't touch.
    // int count = 0; // Don't touch.

    // When found correct guess. Hit is set to 1 and the while loop stops.
    while (hit != 1)
    {
        // // Printing the guess im testing every nth iter.
        // if (count % nth == 0)
        // {
        //     printf("Guess: %s\n", guess);
        // }

        // If guess equals 0, correct guess is found.
        if (p(size, guess) == 0)
        {
            printf("Correct guess found: %s\n", guess);
            hit = 1;

            // Appending the correct guess to the solution file.
            FILE *fp = fopen("solution.txt", "a");
            fprintf(fp, "SEQ Guess found: %s with size: %d\n", guess, size);
            fclose(fp);
        }
        // Else try with the next guess, with the right to left function to increase the numeric value for char.
        else
        {
            // Increase the numeric value for the char value of the guess.
            // If it equals 0 means we have tried all posibilities
            if (right_to_left(guess, size) == 0)
            {
                printf("No correct guess found\n");
                break;
            }
        }
        // count++;
    }

    // Free up in the end.
    free(guess);

    return 0;
}