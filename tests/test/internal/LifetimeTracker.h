/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <memory>

struct LifetimeContent { };
using LifetimeContentPtr = std::shared_ptr<LifetimeContent>;
using LifetimeContentWPtr = std::weak_ptr<LifetimeContent>;

class LifetimeData
{
public:
    LifetimeData() = default;
    LifetimeData(LifetimeContentWPtr content): m_content(content) {}
    LifetimeData(const LifetimeData&) noexcept = default;
    ~LifetimeData() = default;

    LifetimeData& operator= (const LifetimeData&) noexcept = default;
    LifetimeData& operator= (LifetimeData&&) noexcept = default;

    [[nodiscard]] bool destroyed() const { return m_content.expired(); }
    [[nodiscard]] auto count() const { return m_content.use_count(); }

private:
    LifetimeContentWPtr m_content;
};


class LifetimeTracker;
using LifetimeTrackerPtr = std::shared_ptr<LifetimeTracker>;

class LifetimeTracker
{
public:
    [[nodiscard]] LifetimeData getData() const { return LifetimeData(m_lifetimeContent); }
    [[nodiscard]] auto count() const { return m_lifetimeContent.use_count(); }

private:
    LifetimeContentPtr m_lifetimeContent { std::make_shared<LifetimeContent>() };
};
