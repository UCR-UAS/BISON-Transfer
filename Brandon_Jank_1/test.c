/*
 * This is a test file for Brandon's random shenanegans. (Is that how you -
 * spell it?)
 */

#include <glib.h>
#include <stdio.h>
#include <sys/types.h>

int main ()
{
    printf("Program init.\n");

    GArray *array;
    int32_t i;

    array = g_array_new (FALSE, FALSE, sizeof(int32_t));
    for (i = 0; i < 1000; i++)
        g_array_append_val(array, i);
    for (i = 0; i < 1000; i++)
        if (g_array_index(array, int32_t, i) != i)
            printf("Error, got %i instead of %i.\n",
                g_array_index(array, int32_t, i), i);
    g_array_free(array, TRUE);
    printf("Array test worked!\n");
    return 0;
}
