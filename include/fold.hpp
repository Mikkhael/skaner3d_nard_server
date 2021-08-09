#include <utility>
#include <functional>
#include <ostream>

template<class F, class A0>
auto fold(F&&, A0&& a0) {
    return std::forward<A0>(a0);
}

template<class F, class A0, class...As>
auto fold(F&& f, A0&&a0, As&&...as) {
    return f(std::forward<A0>(a0), fold(f, std::forward<As>(as)...));
}

template<class A0, class...As>
auto& fold_ostream(std::ostream& os, A0&&a0 ) {
    return os << std::forward<A0>(a0);
}
template<class...As, class A0>
auto& fold_ostream(std::ostream& os, A0&&a0, As&&...as ) {
    return fold_ostream(os << std::forward<A0>(a0), std::forward<As>(as)...);
}
