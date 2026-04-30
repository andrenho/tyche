#ifndef TYCHE_LOCATION_HH
#define TYCHE_LOCATION_HH

#include <cstdint>

namespace tyche::vm {

struct Location
{
    uint32_t function_id;
    uint32_t pc;
};

}

#endif //TYCHE_LOCATION_HH
