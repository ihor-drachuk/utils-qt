#include <algorithm>
#include <iterator>

namespace UtilsQt {
namespace Internal {

template<typename Container>
void move(const Container& container, size_t first, size_t last, size_t dest)
{
    if (first == dest) return;

    const auto itBegin = container.begin() + first;
    const auto itEnd = container.begin() + last + 1;
    const auto amount = last - first + 1;

    Container temp;
    std::move(itBegin, itEnd, std::back_inserter(temp));
    container.erase(itBegin, itEnd);
}

} // namespace Internal
} // namespace UtilsQt
