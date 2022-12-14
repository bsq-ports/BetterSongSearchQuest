#include "Util/Random.hpp"

namespace BetterSongSearch::Util {
    // Generate random number 
    // Stolen from https://stackoverflow.com/questions/7560114/random-number-c-in-some-range
    int random(int min, int max) //range : [min, max]
    {
        static bool first = true;
        if (first) 
        {  
            srand( time(NULL) ); //seeding for the first time only!
            first = false;
        }
        return min + rand() % (( max + 1 ) - min);
    }
}