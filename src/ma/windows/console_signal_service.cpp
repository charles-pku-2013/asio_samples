//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/shared_ptr_factory.hpp>
#include <ma/windows/console_signal_service.hpp>

namespace ma {
namespace windows {

class console_signal_service::owning_handler_list : public handler_list
{
public:
  ~owning_handler_list();
}; // class console_signal_service::owning_handler_list

class console_signal_service::system_handler : private boost::noncopyable
{
private:
  typedef system_handler this_type;

public:
  static system_handler_ptr get_instance();

  void add_service(console_signal_service&);
  void remove_service(console_signal_service&);

protected:
  system_handler();
  ~system_handler();

private:
}; // class console_signal_service::system_handler

} // namespace windows
} // namespace ma

namespace {

} // anonymous namespace

namespace ma {
namespace windows {

console_signal_service::owning_handler_list::~owning_handler_list()
{
  for (handler_base* handler = this->front(); handler; )
  {
    handler_base* next = this->next(*handler);
    handler->destroy();
    handler = next;
  }
}

#if defined(MA_TYPE_ERASURE_USE_VURTUAL)

console_signal_service::handler_base::handler_base()
{
}

#else

void console_signal_service::handler_base::destroy()
{
  destroy_func_();
}

void console_signal_service::handler_base::post(
    const boost::system::error_code& error)
{
  post_func_(error);
}

console_signal_service::handler_base::handler_base(
    destroy_func_type destroy_func, post_func_type post_func)
  : destroy_func_(destroy_func)
  , post_func_(post_func)
{
}

#endif // defined(MA_TYPE_ERASURE_USE_VURTUAL)

console_signal_service::handler_base::~handler_base()
{
}

console_signal_service::handler_base::handler_base(const this_type& other)
  : base_type(other)
#if !defined(MA_TYPE_ERASURE_USE_VURTUAL)
  , destroy_func_(other.destroy_func_)
  , post_func_(other.post_func_)
#endif
{
}

console_signal_service::system_handler_ptr
console_signal_service::system_handler::get_instance()
{
  typedef ma::shared_ptr_factory_helper<this_type> helper;
  return detail::sp_singleton<this_type>::get_instance(
      &boost::make_shared<helper>);
}

void console_signal_service::system_handler::add_service(
    console_signal_service&)
{
  //todo
}

void console_signal_service::system_handler::remove_service(
    console_signal_service&)
{
  //todo
}

console_signal_service::system_handler::system_handler()
{
  //todo
}

console_signal_service::system_handler::~system_handler()
{
  //todo
}

console_signal_service::console_signal_service(
    boost::asio::io_service& io_service)
  : detail::service_base<console_signal_service>(io_service)  
{
  //todo
}

console_signal_service::~console_signal_service()  
{
  //todo
}

void console_signal_service::construct(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }

  // Add implementation to the list of active implementations.
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    impl_list_.push_front(impl);
  }
}

void console_signal_service::destroy(implementation_type& impl)
{
  if (shutdown_)
  {
    return;
  }
    
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    // Take ownership of waiting handlers
    handlers.push_front_reversed(impl.handlers_);
    // Remove implementation from the list of active implementations.
    impl_list_.erase(impl);
  }

  // Cancel all waiting handlers.
  while (handler_base* handler = handlers.front())
  {
    handlers.pop_front();
    handler->post(boost::asio::error::operation_aborted);
  }  
}

std::size_t console_signal_service::cancel(implementation_type& impl,
    boost::system::error_code& error)
{
  // Take ownership of waiting handlers
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    handlers.push_front_reversed(impl.handlers_);
  }

  // Post all handlers to signal operation was aborted  
  std::size_t handler_count = 0;
  while (handler_base* handler = handlers.front())
  {
    handlers.pop_front();
    handler->post(boost::asio::error::operation_aborted);
    ++handler_count;
  }
  
  error = boost::system::error_code();
  return handler_count;
}

void console_signal_service::shutdown_service()
{
  // Restrict usage of service.
  shutdown_ = true;  
  // Take ownership of all still active handlers.
  owning_handler_list handlers;
  {
    lock_guard impl_list_lock(impl_list_mutex_);
    for (impl_base* impl = impl_list_.front(); impl;
        impl = impl_list_.next(*impl))
    {      
      handlers.push_front_reversed(impl->handlers_);
    }
  }
}

} // namespace windows
} // namespace ma
