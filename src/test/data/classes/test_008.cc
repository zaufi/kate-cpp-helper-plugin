template <
    template <typename A> class TT
  , int B
  >
class foo
{
    template <typename T>
    using type = typename TT<T>::type;

    typedef typename TT<int>::type some;

    using bar = typename TT<char>::type;
};
