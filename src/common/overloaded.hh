#ifndef TYCHE_OVERLOADED_HH
#define TYCHE_OVERLOADED_HH

// used by std::visitor
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

#endif //TYCHE_OVERLOADED_HH
