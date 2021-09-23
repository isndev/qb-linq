#ifndef IENUMERABLE_H_
# define IENUMERABLE_H_

namespace linq {
    template<typename Handle>
    class Enumerable : private Handle {
        typedef typename Handle::iterator iterator_type;
        typedef decltype(*std::declval<iterator_type>()) out_t;

    public:
        ~Enumerable() = default;

        Enumerable() = delete;
        Enumerable(Enumerable const& rhs)
            : Handle(static_cast<Handle const&>(rhs)) {}
        Enumerable(Handle const& rhs)
            : Handle(rhs) {}

        auto begin() const noexcept {
            return static_cast<Handle&>(*this).begin();
        }

        auto end() const noexcept {
            return static_cast<Handle&>(*this).end();
        }

        auto First() const noexcept {
            return static_cast<Handle const&>(*this).first();
        }

        auto FirstOrDefault() const noexcept {
            return static_cast<Handle const&>(*this).firstOrDefault();
        }

        auto Last() const noexcept {
            return static_cast<Handle const&>(*this).last();
        }

        auto LastOrDefault() const noexcept {
            return static_cast<Handle const&>(*this).lastOrDefault();
        }

        auto Reverse() const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).reverse())>;
            return ret_t(static_cast<Handle const&>(*this).reverse());
        }

        template<typename Func>
        auto Select(Func&& nextloader_) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).select(std::forward<Func>(nextloader_)))>;
            return ret_t(static_cast<Handle const&>(*this).select(std::forward<Func>(nextloader_)));
        }

        template<typename... Funcs>
        auto SelectMany(Funcs &&...keys) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).selectMany(std::forward<Funcs>(keys)...))>;
            return ret_t(static_cast<Handle const&>(*this).selectMany(std::forward<Funcs>(keys)...));
        }

        template<typename Func>
        auto Where(Func&& nextfilter_) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).where(std::forward<Func>(nextfilter_)))>;
            return ret_t(static_cast<Handle const&>(*this).where(std::forward<Func>(nextfilter_)));
        }

        template<typename... Funcs>
        auto GroupBy(Funcs &&...keys) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).groupBy(std::forward<Funcs>(keys)...))>;
            return ret_t(static_cast<Handle const&>(*this).groupBy(std::forward<Funcs>(keys)...));
        }

        template<typename... Funcs>
        auto OrderBy(Funcs &&...keys) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).orderBy(std::forward<Funcs>(keys)...))>;
            return ret_t(static_cast<Handle const&>(*this).orderBy(std::forward<Funcs>(keys)...));
        }

        auto Asc() const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).asc())>;
            return ret_t(static_cast<Handle const&>(*this).asc());
        }

        auto Desc() const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).desc())>;
            return ret_t(static_cast<Handle const&>(*this).desc());
        }

        auto Skip(std::size_t const offset) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).skip(offset))>;
            return ret_t(static_cast<Handle const&>(*this).skip(offset));
        }

        template<typename Func>
        auto SkipWhile(Func&& func) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).skip_while(std::forward<Func>(func)))>;
            return ret_t(static_cast<Handle const&>(*this).skip_while(std::forward<Func>(func)));
        }

        auto Take(int const limit) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).take(limit))>;
            return ret_t(static_cast<Handle const&>(*this).take(limit));
        }

        template<typename Func>
        auto TakeWhile(Func&& func) const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).take_while(std::forward<Func>(func)))>;
            return ret_t(static_cast<Handle const&>(*this).take_while(std::forward<Func>(func)));
        }

        template<typename Func>
        auto& Each(Func&& pred) noexcept {
            static_cast<Handle const&>(*this).each(std::forward<Func>(pred));
            return *this;
        }

        template<typename Func>
        auto& Each(Func&& pred) const noexcept {
            static_cast<Handle const&>(*this).each(std::forward<Func>(pred));
            return *this;
        }

        template<typename T>
        bool Contains(T const& elem) const noexcept {
            return static_cast<Handle const&>(*this).contains(elem);
        }

        bool Any() const noexcept {
            return static_cast<Handle const&>(*this).any();
        }

        auto Count() const noexcept {
            return static_cast<Handle const&>(*this).count();
        }

        auto All() const noexcept {
            using ret_t = Enumerable<decltype(static_cast<Handle const&>(*this).all())>;
            return ret_t(static_cast<Handle const&>(*this).all());
        }

        auto Min() const noexcept {
            return static_cast<Handle const&>(*this).min();
        }

        auto Max() const noexcept {
            return static_cast<Handle const&>(*this).max();
        }

        auto Sum() const noexcept {
            return static_cast<Handle const&>(*this).sum();
        }

        template<typename Key>
        auto& operator[](Key const& key) const {
            return static_cast<Handle const&>(*this).operator[](key);
        }

    };
}

#endif // !IENUMERABLE_H_
