#ifndef PMT_EXCEPTION_H_
#define PMT_EXCEPTION_H_

#include <exception>

namespace pmt {
class Exception : public std::exception {
 public:
  explicit Exception(const char *message = "") : message_(message) {}

  const char *what() const noexcept override { return message_; }

 private:
  const char *message_;
};
}  // namespace pmt

#endif  // PMT_EXCEPTION_H_