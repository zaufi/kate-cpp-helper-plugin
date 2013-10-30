template <typename>
struct foo {};

template <typename T>
struct foo<T*> {};

template <>
struct foo<int> {};
