#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <type_traits>
#include <iostream>

namespace custom
{
  template <typename T>
  struct is_byte
  {
    constexpr static bool value = std::is_same_v<T, uint8_t> ||
      std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>;
  };
  template <typename T>
  constexpr auto is_byte_v = is_byte<T>::value;


  template <typename ByteType>
  struct Byte
  {
    static_assert(is_byte_v<ByteType>, "no byte type.");
    ByteType value{0};
  };

  template <typename FieldType, std::size_t Location>
  struct Bit
  {
    constexpr static auto location = Location;
    bool value{false};
  };

  struct _BitBase {};

  template <typename FieldType>
  struct BitField : public _BitBase
  {
    template <std::size_t Location>
    using Bit = custom::Bit<FieldType, Location>;
  };

  template <typename T>
  constexpr auto is_bitfield_v = std::is_base_of_v<_BitBase,
                                                   std::remove_cv_t<std::remove_reference_t<T>>>;

}


struct A
{
  custom::Byte<uint16_t> value1;
  custom::Byte<uint16_t> value2;

  struct MyField : public custom::BitField<uint8_t>
  {
    Bit<0> flag1;
    Bit<1> flag2;
  } field1;

  custom::Byte<uint32_t> value3;

};
BOOST_DESCRIBE_STRUCT(A, (), (value1, value2, field1, value3))
BOOST_DESCRIBE_STRUCT(A::MyField, (), (flag1, flag2))


template <typename T,
          typename Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void test(const T &target)
{
  boost::mp11::mp_for_each<Md>([&](auto D) {
    if constexpr (custom::is_bitfield_v<decltype(target.*D.pointer)>)
      {
        test(target.*D.pointer);
      }
    else if constexpr (custom::is_bitfield_v<T>)
      {
        using field_t = std::remove_cv_t<std::remove_reference_t<decltype(target.*D.pointer)>>;
        std::cout << D.name << ' '
                  << "Location: " << field_t::location << " value: " << (target.*D.pointer).value << '\n';
      }
    else
      {
        std::cout << D.name << ' ' << (target.*D.pointer).value << '\n';
      }
  });
}


int main(int argc, char *argv[])
{
  A a;

  a.value1.value = 123;
  a.value2.value = 321;
  a.field1.flag1.value = false;
  a.field1.flag2.value = true;
  a.value3.value = 777;

  test(a);

  
  return 0;
}
