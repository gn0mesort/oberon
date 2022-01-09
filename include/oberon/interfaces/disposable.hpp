#ifndef OBERON_INTERFACES_DISPOSABLE_HPP
#define OBERON_INTERFACES_DISPOSABLE_HPP

namespace oberon {
namespace interfaces {

  class disposable {
  private:
    virtual bool v_is_disposed() const noexcept = 0;
    virtual void v_dispose() noexcept = 0;
    virtual void v_set_disposed() noexcept = 0;
  public:
    inline virtual ~disposable() noexcept = 0;

    void dispose() noexcept;
    bool is_disposed() const noexcept;
  };

  disposable::~disposable() noexcept { }

}
}

#endif
