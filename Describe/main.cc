#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <type_traits>
#include <iostream>

namespace custom
{
  template <typename T>
  struct remove_ref_cv
  {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
  };
  template <typename T>
  using remove_ref_cv_t = typename remove_ref_cv<T>::type;

  template <typename T, typename ...Os>
  struct one_of_them
  {
    constexpr static bool value = (std::is_same_v<T, Os> || ...);
  };
  template <typename T, typename ...Os>
  constexpr bool one_of_them_v = one_of_them<T, Os...>::value;

  template <typename T, typename ...Parents>
  struct one_is_parent
  {
    constexpr static bool value = (std::is_base_of_v<Parents, T> || ...);
  };
  template <typename T, typename ...Parents>
  constexpr bool one_is_parent_v = one_is_parent<T, Parents...>::value;


  template <typename T, typename CT = remove_ref_cv_t<T>>
  struct is_byte
  {
    constexpr static bool value = one_of_them_v<T, uint8_t, uint16_t, uint32_t, uint64_t>;
  };
  template <typename T>
  constexpr auto is_byte_v = is_byte<T>::value;

  template <typename T>
  struct inner_type;

  template <template <typename...> typename A, typename B>
  struct inner_type<A<B>>{
    using type = B;
  };

  template <typename T>
  using inner_type_t = typename inner_type<T>::type;


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

  template <typename FieldType>
  struct BitField
  {
    template <std::size_t Location>
    using Bit = custom::Bit<FieldType, Location>;
  };

  template <typename T>
  constexpr static bool is_bitfield_v = one_is_parent_v<remove_ref_cv_t<T>,
                                                        BitField<uint8_t>,
                                                        BitField<uint16_t>,
                                                        BitField<uint32_t>,
                                                        BitField<uint64_t>>;

  template <typename ByteType>
  ByteType& alloc_field(std::vector<uint8_t>& out) {
    static_assert(is_byte_v<ByteType>, "This is not byte type.");

    if constexpr (std::is_same_v<uint8_t, ByteType>) {
      out.push_back(0);
      return out[out.size() - 1];
    } else if constexpr (std::is_same_v<uint16_t, ByteType>) {
      out.resize(out.size() + 2);
      return *reinterpret_cast<uint16_t *>(&out[out.size() - 2]);
    } else if constexpr (std::is_same_v<uint32_t, ByteType>) {
      out.resize(out.size() + 4);
      return *reinterpret_cast<uint32_t *>(&out[out.size() - 4]);
    } else {                    // 64 bits
      out.resize(out.size() + 8);
      return *reinterpret_cast<uint64_t *>(&out[out.size() - 8]);
    }
  }


  template <std::size_t Bytes>      // bytes -> per 8 bits
  struct Padding final {};          // TODO: Handle Padding Serialization.
}


struct A
{
  custom::Byte<uint16_t> value1;
  custom::Byte<uint16_t> value2;

  // TODO : limit the number of Bit object. and never allow duplicated Bit<T>
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
          typename ByteType = custom::inner_type_t<typename T::BitField>,
          typename Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void serialize_bitfield(const T& bitfield, std::vector<uint8_t>& out) {
  ByteType& target_bitfield = custom::alloc_field<ByteType>(out);

  boost::mp11::mp_for_each<Md>([&](auto D) {
    auto& bit = bitfield.*D.pointer;
    using field_t = custom::remove_ref_cv_t<decltype(bit)>;

    target_bitfield |= (bit.value << field_t::location);

    std::cout << D.name << ' '
              << "Location: " << field_t::location
              << " value: " << std::boolalpha << bit.value << '\n';
  });
}

template <typename T,
          typename Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void serialize_impl(const T& target, std::vector<uint8_t>& out) {
  boost::mp11::mp_for_each<Md>([&](auto D) {
    auto& obj = target.*D.pointer;
    using field_t = custom::remove_ref_cv_t<decltype(obj)>;

    if constexpr (custom::is_bitfield_v<decltype(target.*D.pointer)>)
      {
        serialize_bitfield(obj, out);
      }
    else if constexpr (std::is_same_v<custom::Byte<uint8_t>, field_t>)
      {
        custom::alloc_field<uint8_t>(out) = obj.value;
      }
    else if constexpr (std::is_same_v<custom::Byte<uint16_t>, field_t>)
      {
        custom::alloc_field<uint16_t>(out) = obj.value;
      }
    else if constexpr (std::is_same_v<custom::Byte<uint32_t>, field_t>)
      {
        custom::alloc_field<uint32_t>(out) = obj.value;
      }
    else if constexpr (std::is_same_v<custom::Byte<uint64_t>, field_t>)
      {
        custom::alloc_field<uint64_t>(out) = obj.value;
      }
    else
      {
        std::cout << D.name << ' ' << obj.value << '\n';
      }
  });
}

template <typename T,
          typename Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void serialize(const T& target, std::vector<uint8_t>& out) {
  out.clear();
  serialize_impl(target, out);
}

int main(int argc, char *argv[])
{
  A a;

  a.value1.value = 0xABCD;
  a.value2.value = 0xDEFA;
  a.field1.flag1.value = false;
  a.field1.flag2.value = true;
  a.value3.value = 0xDEADC0DE;

  std::vector<uint8_t> tmp;
  serialize(a, tmp);

  for (uint32_t c : tmp) {
    std::cout << std::hex << c << ' ';
  }
  std::cout << '\n';
  
  return 0;
}
