#include <string>
#include <vector>

std::vector<rodvin::string> rodvin::string::split(uint32_t separator) const {
  std::vector<rodvin::string> v;
  auto it = begin();

  while (it != end() && *it == separator) 
    ++it;

  auto cur = it;
  while (it != end()) {
    if (*it == separator) {
      v.emplace_back(cur, it);
      while (*it == separator) ++it;
      cur = it;
    } else {
      ++it;
    }
  }

  if (it != cur)
    v.emplace_back(cur, it);

  return v;
}


