#include <concepts>
#include <type_traits>

// Compile-time container that represents a list of types 
template <typename... Ts>
struct typelist {};

// Adds T to the front of typelist<...>. (forward declaration)
template <typename T, typename List>
struct prepend;

// Creates a new typelist with T inserted before the existing typelist of Ts; prepend<int, typelist<long, short>>::type would return typelist<int, long, short>
template <typename T, typename... Ts>
struct prepend<T, typelist<Ts...>> {
    using type = typelist<T, Ts...>;
};

// Concept: Pred<T> must be a template that takes one type, like std::is_integral<T>
template <template <typename> typename Pred>
concept UnaryTypeTrait = requires { 
    typename Pred<int>::type; // Pred<T> must have a nested type alias like std::is_integral<T>::type does
    { Pred<int>::value } -> std::convertible_to<bool>; // Pred<T>::value must exist and be usable as a bool type
};

// Pred: A template like std::is_integral
// T: The type being checked
// 3rd arg: Defaults to void but used for SFINAE
template <template <typename> typename Pred, typename T, typename = void>
struct predicate_value : std::false_type {};

// Overrides previous if Pred<T>::value is valid and sets predicate_value<Pred, T> to true/false based on that value
// std::void_t<decltype(Pred<T>::value): Tries to form a type from Pred<T>::value
// If Pred<T>::value, this becomes void but since the specialization is more specific it has precedence over previous template
template <template <typename> typename Pred, typename T>
struct predicate_value<Pred, T, std::void_t<decltype(Pred<T>::value)>>
    : std::bool_constant<static_cast<bool>(Pred<T>::value)> {}; // Inherits from std::true_type or std::false_type based on Pred<T>::value (which are compile-time true/false values)

// Filter metafunction.

// forward declaration of Filter: Filter<typelist<int, float>, Pred> where Pred is a template like std::is_integral
template <typename List, template <typename> typename Pred>
struct Filter;

// Filter<typelist<>, PredTemplate> stores an empty typelist as type
template <template <typename> typename Pred>
struct Filter<typelist<>, Pred> {
    using type = typelist<>;
};

// Requires the Pred template to be a UnaryTypeTrait (must have type alias and a bool convertible value alias)
// 
template <typename Head, typename... Tail, template <typename> typename Pred>
requires UnaryTypeTrait<Pred>
struct Filter<typelist<Head, Tail...>, Pred> {
private:
    // 
    using filtered_tail = typename Filter<typelist<Tail...>, Pred>::type; // recursive call w/o Head to Filter the rest of the typelist

public:
    // Checks if Head passes predicate_value<Pred, Head>
    // If true, set type to filtered_tail prepended with Head (2nd conditional_t arg)
    // If false, set type to filtered_tail (3rd conditional_t arg)
    using type = std::conditional_t<
        predicate_value<Pred, Head>::value,
        typename prepend<Head, filtered_tail>::type,
        filtered_tail>;
};

// Convenience Alias
// Filter_t<List, Pred> is the same as Filter<List, Pred>::type
template <typename List, template <typename> typename Pred>
using Filter_t = typename Filter<List, Pred>::type;

// Testing (should compile without error)
static_assert(
    std::is_same_v<Filter_t<typelist<long, float>, std::is_integral>,
                   typelist<long>>); // Should return typelist<long>

// Testing (should compile without error)
static_assert(
    std::is_same_v<Filter_t<typelist<int, double, short, bool, char>, std::is_integral>,
                   typelist<int, short, bool, char>>); // Should return typelist<int, short, bool, char>
