#ifndef TEST_MOCKS_EEPROM_H
#define TEST_MOCKS_EEPROM_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

class EEPROMClass {
 public:
  bool begin(std::size_t size) {
    storage_.assign(size, 0);
    committed_ = false;
    return true;
  }

  template <typename T>
  void get(int address, T& value) {
    const std::size_t offset = checkedAddress(address);
    if (offset + sizeof(T) > storage_.size()) {
      std::memset(&value, 0, sizeof(T));
      return;
    }

    std::memcpy(&value, storage_.data() + offset, sizeof(T));
  }

  template <typename T>
  void put(int address, const T& value) {
    const std::size_t offset = checkedAddress(address);
    if (offset + sizeof(T) > storage_.size()) {
      storage_.resize(offset + sizeof(T), 0);
    }

    std::memcpy(storage_.data() + offset, &value, sizeof(T));
  }

  bool commit() {
    committed_ = true;
    return true;
  }

  bool wasCommitted() const {
    return committed_;
  }

 private:
  static std::size_t checkedAddress(int address) {
    return address < 0 ? 0 : static_cast<std::size_t>(address);
  }

  std::vector<std::uint8_t> storage_;
  bool committed_ = false;
};

extern EEPROMClass EEPROM;

#endif
